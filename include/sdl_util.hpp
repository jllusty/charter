// Of all my files, this is the one that needs the most work.
// There is too much state within the xMeta structures - I need to 
// really separate it into smaller chunks for better transformation / composition into components.

#pragma once
#include "utility.hpp"

#include <vector>
#include <unordered_map>
#include <memory>

#include <SDL.h>

struct tileMeta {
    unsigned id;
    std::vector<rectf> boxes;
    tileMeta() {}
};
using tileMetaPtr = std::shared_ptr<tileMeta>;

// set up for easy query of subrect & collision information
struct tilesetMeta {
    // collision state
    std::unordered_map<unsigned,tileMetaPtr> tileMetas;
    // renderable state
    // parent texture of overall tileset
    SDL_Texture* tex;
    unsigned numCols;
    unsigned numRows;
    unsigned tilewidth;
    unsigned tileheight;
    tilesetMeta() {}
    inline SDL_Rect get(unsigned i, unsigned j) {
        SDL_Rect r;
        r.x = j*tilewidth;
        r.y = i*tileheight;
        r.w = tilewidth;
        r.h = tileheight;
        return r;
    }
};
using tilesetMetaPtr = std::shared_ptr<tilesetMeta>;