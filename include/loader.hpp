#pragma once

// Embark Core Engine
#include "entity.hpp"
#include "components.hpp"
#include "context.hpp"

// TinyTMX
#include "tinytmx.hpp"

// STL
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <optional>
#include <utility>
#include <fstream>
using std::vector;
using std::string;
using std::unordered_map;
using std::optional;
using std::pair;

// SDL
#include <SDL.h>
#include <SDL_image.h>

// soley for logging loading processes

class Loader {
    // resource directory (workaround for lack of std::filesystem)
    std::string resDir;
    // tile & entity metas
    std::unordered_map<std::string,tmx::tilemap> tilemaps;
    std::unordered_map<std::string,std::vector<SDL_Texture*>> layers;
    std::unordered_map<std::string,std::shared_ptr<Context>> contexts;
public:
    Loader(const std::string& resourceDirectory);
    void destroySDLTextures();
    // populate tilemaps & entity metas (basically, just know what to do with textures)
    void loadTilemap(const std::string& filename, const std::string& mapname);
    // create textures based on tilemap & entity metas (using the passed renderer)
    void populateTilemap(const std::string& mapname, SDL_Renderer* renderer);
    // instantiate a context, and return it
    std::shared_ptr<Context> getTilemapContext(const std::string& mapname);
    // get size of a tilemap
    std::pair<unsigned,unsigned> getTilemapSize(const std::string& mapname);
};