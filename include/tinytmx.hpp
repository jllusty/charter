#pragma once

#include <iostream>
#include <cassert>
#include <vector>
#include <ostream>
#include <sstream>

#include "tinyxml2.h"

namespace tmx {
    struct image {
        std::string source{};
        unsigned width{0};
        unsigned height{0};
    };

    struct layer {
        // base attr
        unsigned id{0};
        std::string name{};
        unsigned width{0};
        unsigned height{0};
        std::string encoding{};
        // children
        std::vector<std::vector<unsigned>> data{};
    };

    struct property {
        std::string name{};
        std::string type{"string"};
        std::string value{};
    };

    struct object {
        unsigned id{0};
        std::string name{};
        std::string type{};
        float x{0};
        float y{0};
        std::vector<property> properties;
        // optional tile information
        float width{0};
        float height{0};
        unsigned gid{0};
    };

    struct objectgroup {
        unsigned id{0};
        std::string name{};
        std::vector<object> objects{};
    };
    
    struct tile {
        unsigned id{0};
        std::vector<property> properties{};
        objectgroup objs;
    };

    struct tileset {
        // base attr
        unsigned firstgid{0};
        std::string source{};
        std::string name{};
        unsigned tilewidth{0};
        unsigned tileheight{0};
        unsigned spacing{0};
        unsigned margin{0};
        unsigned tilecount{0};
        unsigned columns{0};
        std::string objectalignment{};
        // children
        image img;
        std::vector<tile> tiles;
    };

    struct tilemap {
        // base attr
        unsigned width;
        unsigned height;
        unsigned tilewidth;
        unsigned tileheight;
        // children
        std::vector<tileset> tilesets;
        std::vector<layer> layers;
        std::vector<objectgroup> objectgroups;
    };

    // methods
    std::string say(property p);
    std::string say(tile t);
    std::string say(tileset ts);
    std::string say(layer l);
    std::string say(object obj);
    std::string say(objectgroup group);
    std::string say(tilemap t);
    void setLoggingStream(std::ostream& loggingStream);
    void setResourceDirectory(const std::string& resourceDirectory);
    tileset loadTileset(const std::string& filename);
    tilemap loadTilemap(const std::string& filename);
}