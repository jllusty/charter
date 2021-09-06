#include "loader.hpp"

#include "utility.hpp"

#include <cassert>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <streambuf>
#include <bitset>
#include <string>
#include <map>
#include <memory>
#include <optional>
using std::string;
using std::unordered_map;

#include "logger.hpp"

// auxillary objects / functions
extern Logger glog; // instantiated in main.cpp

/*
static void transposeImage(sf::Image& img) {
    int w = img.getSize().x, h = img.getSize().y;
    for(int i = 0; i < w; ++i) {
        for(int j = i; j < h; ++j) {
            sf::Color col1 = img.getPixel(i,j);
            sf::Color col2 = img.getPixel(j,i);
            img.setPixel(i,j,col2);
            img.setPixel(j,i,col1);
        }
    }
};
*/
Loader::Loader(const string& resourceDirectory) {
    // set our resource directory
    resDir = resourceDirectory;
    // set resource directory for tinytmx (where to find referenced .TSX / .PNG files)
    tmx::setResourceDirectory(resDir);
    // set logging stream for tinytmx
    tmx::setLoggingStream(glog.get());
}
void Loader::destroySDLTextures() {
    std::unordered_map<std::string,std::vector<SDL_Texture*>> layers;
    for(auto kv : layers) {
        for(SDL_Texture* t : kv.second) {
            if(t != nullptr) {
                SDL_DestroyTexture(t);
            }
        }
    }
}
// populate tilemaps & entity metas (basically, just know what to do with textures)
void Loader::loadTilemap(const string& filename, const string& mapname) {
    string filepath = filename;
    glog.get() << "[loader]: requested load tilemap of '" << filepath << "':\n";
    tilemaps[mapname] = tmx::loadTilemap(filepath);
    glog.get() << tmx::say(tilemaps[mapname]);
    glog.get().flush();
}
// create textures based on tilemap & entity metas (using the passed renderer)
void Loader::populateTilemap(const string& mapname, SDL_Renderer* renderer) {
    assert(renderer != nullptr);
    tmx::tilemap tm = tilemaps[mapname];
    glog.get() << "[loader]: requested populate tilemap of '" << mapname << "':\n";
    // iterate over tilesets, associate each tileset with a parent SDL_Texture and rect<unsigned>
    //     of the parent SDL_Texture
    // parent SDL_Texture
    vector<SDL_Texture*> settextures;
    // each set has a "firstgid" of any tiles on the overall tileset
    unordered_map<string,unsigned> setnames;
    // tilerects[i] pertains to the collection of tilerect for the i-th tileset
    vector<vector<rectu>> tilerects;
    for(tmx::tileset& ts : tm.tilesets) {
        // source image information
        std::string path = resDir + "//" + ts.img.source;
        SDL_Surface* img = IMG_Load(path.c_str());
        if(img != nullptr) {
            glog.get() << "[loader]: source image '" << path << "' loaded successfully!\n";
        }
        else {
            glog.get() << "[loader]: source image '" << path << "' loaded successfully!\n";
        }
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer,img);
        if(tex != nullptr) {
            glog.get() << "[loader]: texture creation from loaded image successful!\n";
        }
        else {
            glog.get() << "[loader]: texture creation from loaded image failed!\n";
        }
        settextures.push_back(tex);
        unsigned w = ts.img.width;
        unsigned tw = ts.tilewidth;
        unsigned th = ts.tileheight;
        glog.get() << "[loader]: tileset " << ts.name << " loaded, tile dims = "
                  << tw << " x " << th << "\n";
        glog.get() << "[loader]: image " << ts.img.source << " loaded, dims = "
                  << ts.img.width << " x " << ts.img.height << "\n";
        // generate textures
        tilerects.emplace_back();
        setnames[ts.name] = tilerects.size()-1;
        // these textures will contain sub-rectangles of the 
        // source image (16x16) which contain each 16x16 tile
        for(unsigned i = 0; i < ts.tilecount; ++i) {
            unsigned x = ((i * tw) % w), y = ((i * tw) / w) * th;
            tilerects.back().emplace_back(x,y,tw,th);
            // be specific
            // glog.get() << "[loader]: added a rectangle: {x = " << x << ", y =" << y << ", w = " << tw << ", h = " << th << "}\n";
        }
        glog.get() << "[loader]: created " << tilerects.back().size() << " rect<unsigned>'s!\n";
    }
    glog.get().flush();
    // make tilemap image from layers & texture pointers
    // get tile's gid -> SDL_Texture & source rect
    using TilePtr = std::pair<SDL_Texture*,rectu>;
    auto getTilePtr = [&](unsigned gid) -> TilePtr {
        unsigned set = 1;
        for(; set < tm.tilesets.size(); ++set) {
            if(tm.tilesets[set].firstgid > gid) break;
        }
        --set;
        gid -= tm.tilesets.at(set).firstgid;
        SDL_Texture* tex = settextures.at(set);
        rectu r = tilerects.at(set).at(gid); 
        return TilePtr{tex,r};
    };
    // create a texture for rendering into
    unsigned mw = tm.width*tm.tilewidth, mh = tm.height*tm.tileheight;
    for(tmx::layer l : tm.layers) {
        glog.get() << "[loader]: instantiating a render target texture for layer '" << l.name << "' of size " << mw << "x" << mh << "\n";
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
                //log.get() << "\n";
            }
        }
        layers[mapname].push_back(layerTexture);
        // retarget default
        SDL_SetRenderTarget(renderer,nullptr);
    }
    // load objectgroups into entities
    auto cxt = std::make_shared<Context>();
    contexts[mapname] = cxt;
    // todo: should do a first pass: tilemap id -> entity id
    std::map<unsigned,entity> eids;
    for(tmx::objectgroup& group : tm.objectgroups) {
        for(tmx::object& obj : group.objects) {
            entity e = cxt->addEntity();
            eids[obj.id] = e;
            float x = obj.x, y = obj.y;
            float w = obj.width, h = obj.height;
            cxt->addComponent<position>(e,x,y);
            for(tmx::property& prop : obj.properties) {
                if(prop.name == "input" && prop.value == "true") {
                    cxt->addComponent<input>(e,0);
                }
                else if(prop.name == "velocity") {
                    cxt->addComponent<velocity>(e,0,0);
                }
                else if(prop.name == "sprite") {
                    std::string sheetname = prop.value;
                    unsigned setid = setnames[sheetname];
                    SDL_Texture* tex = settextures[setid];
                    rectu tr = tilerects[setid][0];
                    cxt->addComponent<sprite>(e,tex,tr,1);
                }
                else if(prop.name == "direction") {
                    cxt->addComponent<direction>(e,prop.value);
                }
                else if (prop.name == "volume" && prop.value == "true") {
                    cxt->addComponent<volume>(e,w,h);
                }
                else if(prop.name == "view") {
                    cxt->addComponent<view>(e,eids[stoul(prop.value)]);
                }
                // hacky, please change
                else if(prop.name == "zoom") {
                    cxt->getComponent<view>(e)->zoom = stof(prop.value);
                }
                else if(prop.name == "combat") {
                    cxt->addComponent<combat>(e,4);
                }
                else if(prop.name == "enemy") {
                    cxt->addComponent<enemy>(e,enemy::state::passive);
                }
            }
        }
    }
}

std::shared_ptr<Context> Loader::getTilemapContext(const std::string& mapname) 
{
    assert(contexts.count(mapname) > 0);
    auto cxt = contexts[mapname];
    // create background entity
    entity e = cxt->addEntity();
    SDL_Texture* t = layers[mapname].back();
    auto [w, h] = getTilemapSize(mapname);
    cxt->addComponent<position>(e,0,0);
    cxt->addComponent<sprite>(e,t,rectu(0,0,w,h),0);
    glog.get() << "added background entity w/ size " << w << " x " << h << "\n";
    // create other entities
    return cxt;
}

pair<unsigned,unsigned> Loader::getTilemapSize(const string& mapname) {
    // query texture
    Uint32 format;
    int access;
    int w, h;
    int err = SDL_QueryTexture(layers[mapname].back(), &format, &access, &w, &h);
    return std::make_pair(w,h);
}