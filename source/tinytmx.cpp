#include "tinytmx.hpp"

#include <memory>

// local namespace (bound to this translation unit)
namespace {
    //  logging output stream
    std::ostream* log;
    //  resource directory (workaround lack of std::filesystem)
    std::string resDir = "";

    // auxillary methods: careful reading using caught exceptions
    // read string from attribute, but catch nullptr in case it is missing
    std::string readAttrStr(tinyxml2::XMLElement* xml, const std::string& attr) {
        std::string result = "";
        if(xml != nullptr) {
            if(xml->Attribute(attr.c_str()) != nullptr) {
                result = std::string(xml->Attribute(attr.c_str()));
            }
            else {
                std::string name;
                if(xml->Value() != nullptr) {
                    name = std::string(xml->Value());
                }
                else {
                    name = "?";
                }
                if(log != nullptr) {
                    *log << "[tmx]: element <" << name << "> is missing an attribute: '" 
                              << attr << "'\n";
                    log->flush();
                }
            }
        }
        return result;
    }
    // read unsigned int from attribute
    unsigned readAttrUint(tinyxml2::XMLElement* xml, const std::string& attr) {
        std::string str = readAttrStr(xml,attr);
        unsigned result = 0;
        try {
            result = stoul(str);
        }
        catch(const std::invalid_argument& ex) {
            std::string name;
            if(xml != nullptr) {
                if(xml->Value() != nullptr) {
                    name = std::string(xml->Value());
                }
                else {
                    name = "?";
                }
            }
            if(log != nullptr) {
                *log << "[tmx]: " << " could not convert element <"
                          << name << ">'s attribute '" << attr << "' = '" << result 
                          << "' to an unsigned integer.\n";
                log->flush();
            }
        }
        catch(const std::out_of_range& ex) {
            std::string name;
            if(xml->Value() != nullptr) {
                name = std::string(xml->Value());
            }
            else {
                name = "?";
            }
            if(log != nullptr) {
                *log << "[tmx]: element <" << name << ">'s attribute '" << attr 
                          << "' = '" << result << "' is too large to convert to an unsigned integer.\n";
                log->flush();
            }
        }
        return result;
    }
    // read float from attribute
    float readAttrFloat(tinyxml2::XMLElement* xml, const std::string& attr) {
        std::string str = readAttrStr(xml,attr);
        float result = 0.f;
        try {
            result = stof(str);
        }
        catch(const std::invalid_argument& ex) {
            std::string name;
            if(xml->Value() != nullptr) {
                name = std::string(xml->Value());
            }
            else {
                name = "?";
            }
            if(log != nullptr) {
                *log << "[tmx]: " << " could not convert element <"
                          << name << ">'s attribute '" << attr << "' = '" << result 
                          << "' to a float.\n";
            }
        }
        catch(const std::out_of_range& ex) {
            std::string name;
            if(xml->Value() != nullptr) {
                name = std::string(xml->Value());
            }
            else {
                name = "?";
            }
            if(log != nullptr) {
                *log << "[tmx]: element <" << name << ">'s attribute '" << attr 
                          << "' = '" << result << "' is too large to convert to a float.\n";
            }
        }
        return result;
    }

    // local-only methods: do not pollute global namespace with XMLElement references
    tmx::tileset loadSingleTileset(tinyxml2::XMLElement* tilesetXML);
    std::vector<tmx::tileset> loadTilesets(tinyxml2::XMLElement* mapXML);
    std::vector<tmx::layer> loadLayers(tinyxml2::XMLElement* mapXML);
    std::vector<tmx::objectgroup> loadObjectGroups(tinyxml2::XMLElement* mapXML);
    // 
    tmx::tileset loadSingleTileset(tinyxml2::XMLElement* tilesetXML) {
        assert(tilesetXML != nullptr);
        tmx::tileset ts;
        ts.name = readAttrStr(tilesetXML,"name");
        ts.tilewidth = readAttrUint(tilesetXML,"tilewidth");
        ts.tileheight = readAttrUint(tilesetXML,"tileheight");
        ts.tilecount = readAttrUint(tilesetXML,"tilecount");
        ts.objectalignment = readAttrStr(tilesetXML,"objectalignment");
        ts.img.source = readAttrStr(tilesetXML->FirstChildElement("image"),"source");
        ts.img.width = readAttrUint(tilesetXML->FirstChildElement("image"),"width");
        ts.img.height = readAttrUint(tilesetXML->FirstChildElement("image"),"height");
        ts.columns = readAttrUint(tilesetXML,"columns");
        // get tiles
        tinyxml2::XMLElement* tileXML = tilesetXML->FirstChildElement("tile");
        while(tileXML != nullptr) {
            tmx::tile t;
            t.id = readAttrUint(tileXML,"id");
            if(tileXML->FirstChildElement("objectgroup") != nullptr) {
                t.objs = loadObjectGroups(tileXML).back();
            }
            // get properties
            tinyxml2::XMLElement* propsXML = tileXML->FirstChildElement("properties");
            tinyxml2::XMLElement* propXML = (propsXML != nullptr) 
                                ? propsXML->FirstChildElement("property")
                                : nullptr;
            while(propXML != nullptr) {
                tmx::property p;
                p.name = readAttrStr(propXML,"name");
                // catch nullptr manually here, need to set default = "string"
                const char* pType = propXML->Attribute("type");
                if(pType != nullptr) {
                    p.type = std::string(pType);
                }
                else {
                    p.type = "string";
                }
                p.value = readAttrStr(propXML,"value");
                //
                t.properties.push_back(p);
                propXML = propXML->NextSiblingElement("property");
            }
            ts.tiles.push_back(t);
            tileXML = tileXML->NextSiblingElement("tile");
        }
        return ts;
    }
    std::vector<tmx::tileset> loadTilesets(tinyxml2::XMLElement* mapXML) {
        assert(mapXML != nullptr);
        tinyxml2::XMLElement* tilesetXML = mapXML->FirstChildElement("tileset");
        std::vector<tmx::tileset> tilesets;
        while(tilesetXML != nullptr) {
            tmx::tileset ts;
            if(tilesetXML->Attribute("source") != nullptr) {
                std::string source = readAttrStr(tilesetXML,"source");
                std::string filepath = resDir + "//" + source;
                if(log != nullptr) {
                    *log << "[tmx]: needs to read an external tileset: '" << filepath << "'\n";
                    log->flush();
                }
                tinyxml2::XMLDocument doc;
                tinyxml2::XMLError err = doc.LoadFile(filepath.c_str());
                if(err != tinyxml2::XML_SUCCESS) {
                    if(log != nullptr) {
                        *log << "[tmx]: Failed to parse XML of '" << filepath << "'\n\tError = " << err << "\n";
                        log->flush();
                    }
                }
                else {
                    ts = loadSingleTileset(doc.FirstChildElement("tileset"));
                }
            }
            else {
                ts = loadSingleTileset(tilesetXML);
            }
            ts.firstgid = readAttrUint(tilesetXML,"firstgid");
            tilesets.push_back(ts);
            if(log != nullptr) {
                *log << "[tmx]: loaded tileset for '" << ts.name << "'\n";
                log->flush();
            }
            tilesetXML = tilesetXML->NextSiblingElement("tileset");
        }
        return tilesets;
    }
    std::vector<tmx::layer> loadLayers(tinyxml2::XMLElement* mapXML) {
        using namespace std;
        using namespace tinyxml2;
        using namespace tmx;
        assert(mapXML != nullptr); 
        XMLElement* layerXML = mapXML->FirstChildElement("layer");
        vector<layer> layers;
        while(layerXML != nullptr) {
            layer l;
            l.id = readAttrUint(layerXML,"id");
            l.name = readAttrStr(layerXML,"name");
            l.width = readAttrUint(layerXML,"width");
            l.height = readAttrUint(layerXML,"height");
            XMLElement* dataXML = layerXML->FirstChildElement("data");
            l.encoding = readAttrStr(dataXML,"encoding");
            string dataStr(dataXML->GetText());
            if(l.encoding != "csv") {
                if(log != nullptr) {
                    *log << "[tmx]: encoding of <data> element is not csv!\n";
                }
            }
            istringstream iss(dataStr);
            for(unsigned i = 0; i < l.height; ++i) {
                l.data.emplace_back();
                for(unsigned j = 0; j < l.width; ++j) {
                    string num;
                    getline(iss,num,',');
                    l.data.back().push_back(stoul(num));
                }
            }
            layers.push_back(l);
            layerXML = layerXML->NextSiblingElement("layer");
        }
        return layers;
    }
    std::vector<tmx::objectgroup> loadObjectGroups(tinyxml2::XMLElement* mapXML) {
        using namespace std;
        using namespace tinyxml2;
        using namespace tmx;
        assert(mapXML != nullptr);
        XMLElement* groupXML = mapXML->FirstChildElement("objectgroup");
        vector<objectgroup> groups;
        while(groupXML != nullptr) {
            objectgroup group;
            group.id = stoul(groupXML->Attribute("id"));
            group.id = readAttrUint(groupXML,"id");
            group.name = readAttrStr(groupXML,"name");
            // get objects
            XMLElement* objectElement = groupXML->FirstChildElement("object");
            while(objectElement != nullptr) {
                object obj;
                obj.id = readAttrUint(objectElement,"id");
                obj.name = readAttrStr(objectElement,"name");
                obj.type = readAttrStr(objectElement,"type");
                obj.x = readAttrFloat(objectElement,"x");
                obj.y = readAttrFloat(objectElement,"y");
                obj.gid = readAttrUint(objectElement,"gid");
                obj.width = readAttrFloat(objectElement,"width");
                obj.height = readAttrFloat(objectElement,"height");
                // get properties
                XMLElement* propsXML = objectElement->FirstChildElement("properties");
                XMLElement* propXML = (propsXML != nullptr) 
                                    ? propsXML->FirstChildElement("property")
                                    : nullptr;
                while(propXML != nullptr) {
                    property p;
                    p.name = readAttrStr(propXML,"name");
                    // catch default(nullptr) = "string"
                    const char* pType = propXML->Attribute("type");
                    if(pType != nullptr) {
                        p.type = string(pType);
                    }
                    else {
                        p.type = "string";
                    }
                    p.value = readAttrStr(propXML,"value");
                    //
                    obj.properties.push_back(p);
                    propXML = propXML->NextSiblingElement("property");
                }
                group.objects.push_back(obj);
                objectElement = objectElement->NextSiblingElement("object");
            }
            groups.push_back(group);
            groupXML = groupXML->NextSiblingElement("objectgroup");
        }
        return groups;
    }
}

// global namespace
namespace tmx {
    // print structs
    // property
    std::string say(property p) {
        std::ostringstream oss;
        oss << "\t\t\t(" << p.name << "," << p.type << "," << p.value << ")\n";
        return oss.str();
    }
    // tile
    std::string say(tile t) {
        std::ostringstream oss;
        oss << "\t\ttile members:\n";
        oss << "\t\t\tid = " << t.id << "\n";
        for(object o : t.objs.objects) {
            oss << say(o);
        }
        for(property p : t.properties) {
            oss << say(p);
        }
        return oss.str();
    }
    // tileset
    std::string say(tileset ts) {
        std::ostringstream oss;
        oss << "\ttileset members:\n";
        oss << "\t\tfirstgid = " << ts.firstgid << "\n";
        oss << "\t\ttilecount = " << ts.tilecount << "\n";
        oss << "\t\tobjectalignment = " << ts.objectalignment << "\n";
        oss << "\t\timg.source = " << ts.img.source << "\n";
        oss << "\t\timg.width = " << ts.img.width << "\n";
        oss << "\t\timg.height = " << ts.img.height << "\n";
        for(tile t : ts.tiles) {
            oss << say(t);
        }
        return oss.str();
    }
    // layer
    std::string say(layer l) {
        std::ostringstream oss;
        oss << "\tlayer members:\n";
        oss << "\t\tid = " << l.id << "\n";
        oss << "\t\tname = " << l.name << "\n";
        oss << "\t\twidth = " << l.width << "\n";
        oss << "\t\theight = " << l.height << "\n";
        oss << "\t\tencoding = " << l.encoding << "\n";
        oss << "\t\tdata size = (" << l.data.size() << " x " << l.data.back().size() << ")\n";
        return oss.str();
    }
    // object
    std::string say(object obj) {
        std::ostringstream oss;
        oss << "\t\tobject members:\n";
        oss << "\t\t\tid = " << obj.id << "\n";
        oss << "\t\t\tname = " << obj.name << "\n";
        oss << "\t\t\tgid = " << obj.gid << "\n";
        oss << "\t\t\tx = " << obj.x << "\n";
        oss << "\t\t\ty = " << obj.y << "\n";
        oss << "\t\t\twidth = " << obj.width << "\n";
        oss << "\t\t\theight = " << obj.height << "\n";
        for(property p : obj.properties) {
            oss << say(p);
        }
        return oss.str();
    }
    // objectgroup
    std::string say(objectgroup group) {
        std::ostringstream oss;
        oss << "\tobjectgroup members:\n";
        oss << "\t\tid = " << group.id << "\n";
        oss << "\t\tname = " << group.name << "\n";
        for(object obj : group.objects) {
            oss << say(obj);
        }
        return oss.str();
    }
    // map
    std::string say(tilemap t) {
        std::ostringstream oss;
        oss << "tilemap members:\n";
        oss << "\twidth = " << t.width << "\n";
        oss << "\theight = " << t.height << "\n";
        oss << "\ttilewidth = " << t.tilewidth << "\n";
        oss << "\ttileheight = " << t.tileheight << "\n";
        for(tileset ts : t.tilesets) {
            oss << say(ts);
        }
        for(layer l : t.layers) {
            oss << say(l);
        }
        for(objectgroup g : t.objectgroups) {
            oss << say(g);
        }
        return oss.str();
    }

    // set reference (workaround lack of std::filesystem)
    void setResourceDirectory(const std::string& resourceDirectory) {
        resDir = resourceDirectory;
    }
    // set logging stream
    void setLoggingStream(std::ostream& loggingStream) {
        log = &loggingStream;
    }
    tileset loadTileset(const std::string& filename) {
        using namespace tinyxml2;
        using namespace std;
        tileset ts;
        XMLDocument doc;
        string filepath = resDir + "\\" + filename;
        XMLError err = doc.LoadFile(filepath.c_str());
        if(err != XML_SUCCESS) {
            cout << "Failed to parse XML of '" << filepath << "'\n\tError = " << err << "\n";
            cout.flush();
            return ts;
        }
        XMLElement* tilesetXML = doc.FirstChildElement("tileset");
        return loadSingleTileset(tilesetXML);
    }
    // main methods
    tilemap loadTilemap(const std::string& filename) {
        using namespace tinyxml2;
        using namespace std;
        tilemap t;
        //
        XMLDocument doc;
        string filepath = resDir + "\\" + filename;
        XMLError err = doc.LoadFile(filepath.c_str());
        if(err != XML_SUCCESS) {
            cout << "Failed to parse XML of '" << filepath << "'\n\tError = " << err << "\n";
            return t;
        }
        XMLElement* mapXML = doc.FirstChildElement("map");
        if(mapXML == nullptr) {
            cout << "Failed to parse XML for element <map>\n";
            return t;
        }
        t.width = readAttrUint(mapXML,"width");
        t.height = readAttrUint(mapXML,"height");
        t.tilewidth = readAttrUint(mapXML,"tilewidth");
        t.tileheight = readAttrUint(mapXML,"tileheight");
        // load tilesets
        t.tilesets = loadTilesets(mapXML);
        // load layer
        t.layers = loadLayers(mapXML);
        // load objectgroups
        t.objectgroups = loadObjectGroups(mapXML);
        return t;
    }
}