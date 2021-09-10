#pragma once

// Charter Core Engine
#include "entity.hpp"
#include "components.hpp"
#include "context.hpp"

// TinyTMX library
#include "tinytmx.hpp"

// STL
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

// SDL
#include <SDL_image.h>

struct tilemapMeta {
    // tmx::tilemap containing all metadata from TMX file
    tmx::tilemap tm;
    // finished tilemap layers
    std::vector<SDL_Texture*> layers;
    tilemapMeta() {}
};
using tilemapMetaPtr = std::shared_ptr<tilemapMeta>;

class Loader {
    // resource directory (workaround for lack of std::filesystem)
    std::string resDir;
    // tile & entity metas
    std::unordered_map<std::string,std::vector<tileMetaPtr>> tileMetas;
    std::unordered_map<std::string,tilemapMetaPtr> tilemapMetas;
    // created contexts
    std::unordered_map<std::string,std::shared_ptr<Context>> contexts;
public:
    // init resource directory, init tinytmx lib
    Loader(const std::string& resourceDirectory);
    // populate tilemaps & entity metas (basically, just know what to do with textures)
    void loadTilemap(const std::string& filename, const std::string& mapname);
    // create textures based on tilemap & entity metas (using the passed renderer)
    void populateTilemap(const std::string& mapname, SDL_Renderer* renderer);
    // de-allocate any textures manually
    void destroySDLTextures();
    // instantiate a context, and return it
    std::shared_ptr<Context> getTilemapContext(const std::string& mapname);
    // get size of a tilemap's base layer
    std::pair<unsigned,unsigned> getTilemapSize(const std::string& mapname);
private:
    // create collision boxes
    tileMetaPtr loadTile(tmx::tile t);
    // parse tileset into into SDL_Texture
    tilesetMetaPtr loadTileset(tmx::tileset ts, SDL_Renderer* renderer);
};