#include "loader.hpp"
#include "utility.hpp"
#include "logger.hpp"

// SDL_Image
#include <SDL.h>
#include "sdl_util.hpp"

// STL
#include <cassert>
#include <iterator>
#include <algorithm>
#include <streambuf>
#include <bitset>
#include <map>

// auxillary objects / functions
extern Logger glog; // gamelog - instantiated in main.cpp

// init resource directory, init tinytmx lib
Loader::Loader(const std::string& resourceDirectory) {
    // set our resource directory
    resDir = resourceDirectory;
    // set resource directory for tinytmx (where to find referenced .TSX / .PNG files)
    tmx::setResourceDirectory(resDir);
    // set logging stream for tinytmx
    tmx::setLoggingStream(glog.get());
}

// populate tilemaps & entity metas
void Loader::loadTilemap(const std::string& filename, const std::string& mapname) {
    assert(tilemapMetas.count(mapname) == 0);
    auto tmPtr = std::make_shared<tilemapMeta>();
    tilemapMetas.emplace(mapname,tmPtr);
    std::string filepath = filename;
    glog.get() << "[loader]: requested load tilemap of '" << filepath << "':\n";
    glog.get().flush();
    tilemapMetas[mapname]->tm = tmx::loadTilemap(filepath);
    // debug out
    glog.get() << tmx::say(tilemapMetas[mapname]->tm);
    glog.get().flush();
}

tileMetaPtr Loader::loadTile(tmx::tile t) {
    tileMetaPtr pT = std::make_shared<tileMeta>();
    pT->id = t.id;
    for(tmx::object& o : t.objs.objects) {
        if(o.type == "collision") {
            pT->boxes.emplace_back(o.x,o.y,o.width,o.height);
        }
    }
    return pT;
}

tilesetMetaPtr Loader::loadTileset(tmx::tileset ts, SDL_Renderer* renderer)
{
    assert(renderer != nullptr);
    glog.get() << "[loader]: requested load tileset of '" << ts.img.source << "':\n";
    glog.get().flush();
    auto tmPtr = std::make_shared<tilesetMeta>();
    // get collision information from tiles
    for(tmx::tile t : ts.tiles) {
        tmPtr->tileMetas[t.id] = loadTile(t);
    }
    // get image source information, create textures with renderer
    std::string path = resDir + "//" + ts.img.source;
    SDL_Surface* img = IMG_Load(path.c_str());
    if(img != nullptr) {
        glog.get() << "[loader]: source image '" << path << "' loaded successfully!\n";
    }
    else {
        glog.get() << "[loader]: source image '" << path << "' load failed!\n";
    }
    glog.get().flush();
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer,img);
    if(tex == nullptr) {
        glog.get() << "[loader]: texture creation from loaded image failed!\n";
    }
    glog.get() << "[loader]: tileset " << ts.name << " loaded, tile dims = "
              << ts.tilewidth << " x " << ts.tileheight << "\n";
    glog.get() << "[loader]: image " << ts.img.source << " loaded, dims = "
              << ts.img.width << " x " << ts.img.height << "\n";
    glog.get().flush();
    tmPtr->tex = tex;
    tmPtr->numCols = ts.columns;
    tmPtr->numRows = ts.tilecount/ts.columns;
    tmPtr->tilewidth = ts.tilewidth;
    tmPtr->tileheight = ts.tileheight;
    return tmPtr;
}

// create textures based on tilemap & entity metas (using the passed renderer)
// NOTE: this is a fucking mess of a function
void Loader::populateTilemap(const std::string& mapname, SDL_Renderer* renderer) {
    assert(renderer != nullptr);
    assert(tilemapMetas.count(mapname) == 1);
    //
    auto tmMeta = tilemapMetas[mapname];
    auto cxt = std::make_shared<Context>();
    contexts[mapname] = cxt;
    tmx::tilemap& tm = tmMeta->tm;
    glog.get() << "[loader]: requested populate tilemap: '" << mapname << "':\n";
    glog.get().flush();
    std::unordered_map<std::string,tilesetMetaPtr> tilesetMetas;
    for(tmx::tileset& ts : tm.tilesets) {
        tilesetMetas[ts.name] = loadTileset(ts,renderer);
        glog.get() << "\t-> loaded tileset: '" << ts.name << "'\n";
        glog.get().flush();
    }
    // make tilemap image from layers & texture pointers
    // get tile's gid -> SDL_Texture & source rect
    auto getSetAndId = [&](unsigned gid) -> std::pair<unsigned,unsigned> {
        unsigned set = 1;
        for(; set < tm.tilesets.size(); ++set) {
            if(tm.tilesets[set].firstgid > gid) break;
        }
        --set;
        return {set, gid - tm.tilesets.at(set).firstgid};
    };
    using TilePtr = std::pair<SDL_Texture*,SDL_Rect>;
    auto getTilePtr = [&](unsigned gid) -> TilePtr {
        auto [set, id] = getSetAndId(gid);
        unsigned i = id / tm.tilesets.at(set).columns;
        unsigned j = id % tm.tilesets.at(set).columns;
        SDL_Texture* tex = tilesetMetas.at(tm.tilesets.at(set).name)->tex;
        SDL_Rect r = tilesetMetas.at(tm.tilesets.at(set).name)->get(i,j);
        return TilePtr{tex,r};
    };
    // create a texture for rendering into & tilemap collision entities
    unsigned mw = tm.width*tm.tilewidth, mh = tm.height*tm.tileheight;
    for(tmx::layer l : tm.layers) {
        glog.get() << "[loader]: instantiating a render target texture for layer '" << l.name << "' of size " << mw << "x" << mh << "\n";
        glog.get().flush();
        SDL_Texture* layerTexture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mw, mh);
        SDL_SetTextureBlendMode(layerTexture, SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(renderer, layerTexture);
        for(unsigned i = 0; i < l.data.size(); ++i) {
            for(unsigned j = 0; j < l.data[0].size(); ++j) {
                unsigned gid = l.data.at(i).at(j); 
                // unsigned highest bits
                std::bitset<32> b(gid);
                // store transforms
                bool flipH = b[31], flipV = b[30], flipD = b[29];
                b[31] = false; b[30] = false; b[29] = false;
                gid = b.to_ulong();
                auto [tex, sourceRect] = getTilePtr(gid);
                // render tile into a smaller texture temporarilly for transfomation processing (3-pass flipping)
                SDL_RendererFlip flip1 = SDL_FLIP_NONE;
                SDL_RendererFlip flip2 = SDL_FLIP_NONE;
                SDL_RendererFlip flip3 = SDL_FLIP_NONE;
                SDL_Texture* tileTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, sourceRect.w, sourceRect.h);
                SDL_SetTextureBlendMode(tileTexture, SDL_BLENDMODE_BLEND);
                SDL_SetRenderTarget(renderer, tileTexture);
                double angle1 = 0.0;
                SDL_Point p1;
                p1.x = 0; p1.y = 0;
                if(flipD) {
                    flip1 = SDL_FLIP_HORIZONTAL;
                    angle1 = 270.0;
                    p1.x = sourceRect.w/2;
                    p1.y = sourceRect.h/2;
                    //log.get() << "[loader]: flipping diagonally\n";
                }
                // first pass: flip diagonally
                SDL_Rect source1;
                source1.x = sourceRect.x; source1.y = sourceRect.y;
                source1.w = sourceRect.w; source1.h = sourceRect.h;
                SDL_Rect dest1;
                dest1.x = 0; dest1.y = 0;
                dest1.w = sourceRect.w; dest1.h = sourceRect.h;
                SDL_RenderCopyEx(renderer, tex, &source1, &dest1, angle1, &p1, flip1);
                // second pass: flip horizontally
                if(flipH) {
                    flip2 = SDL_FLIP_HORIZONTAL;
                    //log.get() << "[loader]: flipping horizontally\n";
                }
                SDL_Rect source2;
                source2.x = 0; source2.y = 0;
                source2.w = sourceRect.w; source2.h = sourceRect.h;
                SDL_Rect dest2;
                dest2.x = 0; dest2.y = 0;
                dest2.w = sourceRect.w; dest2.h = sourceRect.h;
                SDL_Point p2;
                p2.x = 0; p2.y = 0;
                SDL_RenderCopyEx(renderer, tileTexture, &source2, &dest2, 0.0, &p2, flip2);
                SDL_SetRenderTarget(renderer, layerTexture);
                // third pass: flip vertically
                if(flipV) {
                    flip3 = SDL_FLIP_VERTICAL;
                    //log.get() << "[loader]: flipping vertically\n";
                }
                SDL_Rect source3;
                source3.x = 0; source3.y = 0;
                source3.w = sourceRect.w; source3.h = sourceRect.h;
                SDL_Rect dest3;
                dest3.x = j*tm.tilewidth; dest3.y = i*tm.tileheight; 
                dest3.w = source2.w; dest3.h = source2.h;
                SDL_Point p3;
                p3.x = 0; p3.y = 0;
                SDL_RenderCopyEx(renderer, tileTexture, &source3, &dest3, 0.0, &p3, flip3);
                SDL_DestroyTexture(tileTexture);
                // collision boxes?
                auto [set, id] = getSetAndId(gid);
                if(tilesetMetas[tm.tilesets[set].name]->tileMetas.count(id) != 0) {
                    for(rectf& box : tilesetMetas[tm.tilesets[set].name]->tileMetas[id]->boxes) {
                        entity e = cxt->addEntity();
                        float x = j*tm.tilewidth + box.x;
                        float y = i*tm.tileheight + box.y;
                        cxt->addComponent<position>(e,x,y);
                        cxt->addComponent<volume>(e,box.w,box.h);
                        glog.get() << "added a environment collision at (" << x << "," << y << ")\n";
                        glog.get().flush();
                    }
                }
                //log.get() << "\n";
            }
        }
        glog.get() << "\t done w/ layer render!\n"; glog.get().flush();
        // push finished layer texture
        tmMeta->layers.push_back(layerTexture);
        // retarget default
        SDL_SetRenderTarget(renderer,nullptr);
    }
    // load objectgroups into entities
    glog.get() << "\n[loader]: loading objectgroups into entities\n";
    glog.get().flush();
    std::unordered_map<std::string,tilesetMetaPtr> spriteMetas;
    // do first pass to catch all possible references between objects
    std::vector<std::vector<entity>> es;
    std::map<unsigned,entity> eids;
    for(tmx::objectgroup& group : tm.objectgroups) {
        es.emplace_back();
        for(tmx::object& obj : group.objects) {
            entity e = cxt->addEntity();
            eids[obj.id] = e;
            es.back().push_back(e);
        }
    }
    for(unsigned i = 0; i < tm.objectgroups.size(); ++i) {
        tmx::objectgroup& group = tm.objectgroups.at(i);
        for(unsigned j = 0; j < group.objects.size(); ++j) {
            tmx::object& obj = group.objects.at(j);
            entity e = es.at(i).at(j);
            float x = obj.x, y = obj.y;
            float w = obj.width, h = obj.height;
            // camera
            if(obj.type == "camera") {
                entity target = 0;
                float zoom = 1.0f;
                for(tmx::property& prop : obj.properties) {
                    if(prop.name == "target") {
                        target = eids[stoull(prop.value)];
                    }
                    else if(prop.name == "zoom") {
                        zoom = stof(prop.value);
                    }
                }
                cxt->addComponent<camera>(e,target,zoom);
                continue;
            }
            cxt->addComponent<position>(e,x,y);
            cxt->addComponent<volume>(e,w,h);
            // not camera
            for(tmx::property& prop : obj.properties) {
                if(prop.name == "input" && prop.value == "true") {
                    cxt->addComponent<input>(e,0);
                }
                else if(prop.name == "velocity") {
                    cxt->addComponent<velocity>(e,0,0);
                }
                else if(prop.name == "sprite") {
                    std::string sheetname = prop.value;
                    glog.get() << "[loader]: read 'sprite' property: need sheetname:'" << sheetname << "'\n";
                    glog.get().flush();
                    tmx::tileset ts = tmx::loadTileset(sheetname);
                    // TODO: don't load more than you need to
                    if(tilesetMetas.count(ts.name) != 0) {
                        spriteMetas[ts.name] = tilesetMetas[ts.name];
                    }
                    else if(spriteMetas.count(ts.name) == 0) {
                        spriteMetas[ts.name] = loadTileset(ts,renderer);
                    }
                    cxt->addComponent<sprite<idle>>(e,spriteMetas[ts.name],0,0,1,idle(0));
                }
                else if(prop.name == "collide") {
                    cxt->addComponent<collide>(e);
                }
                else if(prop.name == "direction") {
                    direction::facing dir;
                    if(prop.value == "left") {
                        dir = direction::facing::left;
                    }
                    else if(prop.value == "right") {
                        dir = direction::facing::right;
                    }
                    else if(prop.value == "down") {
                        dir = direction::facing::down;
                    }
                    else if(prop.value == "up") {
                        dir = direction::facing::up;
                    }
                    cxt->addComponent<direction>(e,dir);
                }
                else if(prop.name == "combat") {
                    cxt->addComponent<combat>(e,400);
                }
                else if(prop.name == "enemy") {
                    cxt->addComponent<enemy>(e,enemy::state::passive);
                }
                else if(prop.name == "layer") {
                    cxt->addComponent<layer>(e);
                }
                else if(prop.name == "cursor") {
                    cxt->addComponent<cursor>(e,0.0f,0.0f);
                }
                else if(prop.name == "cursorsprite") {
                    std::string sheetname = prop.value;
                    glog.get() << "[loader]: read 'cursorsprite' property: need sheetname:'" << sheetname << "'\n";
                    glog.get().flush();
                    tmx::tileset ts = tmx::loadTileset(sheetname);
                    // TODO: don't load more than you need to
                    if(tilesetMetas.count(ts.name) != 0) {
                        spriteMetas[ts.name] = tilesetMetas[ts.name];
                    }
                    else if(spriteMetas.count(ts.name) == 0) {
                        spriteMetas[ts.name] = loadTileset(ts,renderer);
                    }
                    cxt->addComponent<sprite<cursor>>(e,spriteMetas[ts.name],0,0,0,cursor(0.0f,0.0f));
                }
            }
        }
    }
}

// de-allocate any textures manually
void Loader::destroySDLTextures() {
    for(auto st : tilemapMetas) {
        for(SDL_Texture * l : st.second->layers) {
            if(l != nullptr) {
                SDL_DestroyTexture(l);
            }
        }
    }
}

// instantiate a context, and return it
std::shared_ptr<Context> Loader::getTilemapContext(const std::string& mapname) 
{
    assert(contexts.count(mapname) == 1);
    auto cxt = contexts[mapname];
    // create background entity
    entity e = cxt->addEntity();
    SDL_Texture* t = tilemapMetas[mapname]->layers.back();
    auto [w, h] = getTilemapSize(mapname);
    // add position component
    cxt->addComponent<position>(e,0,0);
    // add sprite component
    auto pTS = std::make_shared<tilesetMeta>();
    pTS->tex = t;
    pTS->numCols = 1;
    pTS->numRows = 1;
    pTS->tilewidth = w;
    pTS->tileheight = h;
    cxt->addComponent<sprite<idle>>(e,pTS,0,0,0,idle(0));
    glog.get() << "[loader]: added background entity w/ size " << w << " x " << h << "\n";
    glog.get().flush();
    return cxt;
}

// get size of a tilemap's base layer
std::pair<unsigned,unsigned> Loader::getTilemapSize(const std::string& mapname) {
    assert(contexts.count(mapname) == 1);
    // query texture
    Uint32 format;
    int access;
    int w, h;
    SDL_QueryTexture(tilemapMetas[mapname]->layers.back(), &format, &access, &w, &h);
    return std::make_pair(w,h);
}