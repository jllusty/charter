#include "tinytmx.hpp"

#include <memory>

// static vars
//  logging output stream
static std::ostream* log;
//  resource directory (workaround lack of std::filesystem)
static std::string resDir = "";

// auxillary methods: careful reading using caught exceptions
// read string from attribute, but catch nullptr in case it is missing
static std::string readAttrStr(tinyxml2::XMLElement* xml, const std::string& attr) {
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
            }
        }
    }
    return result;
}
// read unsigned int from attribute
static unsigned readAttrUint(tinyxml2::XMLElement* xml, const std::string& attr) {
    std::string str = readAttrStr(xml,attr);
    unsigned result = 0;
    try {
        result = stoul(str);
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
                      << "' to an unsigned integer.\n";
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
        }
    }
    return result;
}
// read float from attribute
static float readAttrFloat(tinyxml2::XMLElement* xml, const std::string& attr) {
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

// local-only methods: do not pollute namespace with XMLElement references
static std::vector<tmx::tileset> loadTilesets(tinyxml2::XMLElement* mapXML) {
    using namespace std;
    using namespace tinyxml2;
    using namespace tmx;
    assert(mapXML != nullptr);
    XMLElement* tilesetXML = mapXML->FirstChildElement("tileset");
    vector<tileset> tilesets;
    while(tilesetXML != nullptr) {
        tileset ts;
        ts.firstgid = readAttrUint(tilesetXML,"firstgid");
        // if tileset is external, read children from there
        XMLDocument doc;
        XMLElement* rTilesetXML = nullptr;
        if(tilesetXML->Attribute("source") != nullptr) {
            string filename = readAttrStr(tilesetXML,"source");
            string filepath = resDir + "\\" + filename;
            XMLError err = doc.LoadFile(filepath.c_str());
            if(err != XML_SUCCESS) {
                cout << "Failed to parse XML. Error = " << err << "\n";
            }
            rTilesetXML = doc.FirstChildElement("tileset");
        }
        else {
            rTilesetXML = tilesetXML;
        }
        ts.name = readAttrStr(rTilesetXML,"name");
        ts.tilewidth = readAttrUint(rTilesetXML,"tilewidth");
        ts.tileheight = readAttrUint(rTilesetXML,"tileheight");
        ts.tilecount = readAttrUint(rTilesetXML,"tilecount");
        ts.objectalignment = readAttrStr(rTilesetXML,"objectalignment");
        ts.img.source = readAttrStr(rTilesetXML->FirstChildElement("image"),"source");
        ts.img.width = readAttrUint(rTilesetXML->FirstChildElement("image"),"width");
        ts.img.height = readAttrUint(rTilesetXML->FirstChildElement("image"),"height");
        // get tiles
        XMLElement* tileXML = rTilesetXML->FirstChildElement("tile");
        while(tileXML != nullptr) {
            tile t;
            t.id = readAttrUint(tileXML,"id");
            // get properties
            XMLElement* propsXML = tileXML->FirstChildElement("properties");
            XMLElement* propXML = (propsXML != nullptr) 
                                ? propsXML->FirstChildElement("property")
                                : nullptr;
            while(propXML != nullptr) {
                property p;
                p.name = readAttrStr(propXML,"name");
                // catch nullptr manually here, need to set default = "string"
                const char* pType = propXML->Attribute("type");
                if(pType != nullptr) {
                    p.type = string(pType);
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
        tilesetXML = tilesetXML->NextSiblingElement("tileset");
        tilesets.push_back(ts);
    }
    return tilesets;
}
static std::vector<tmx::layer> loadLayers(tinyxml2::XMLElement* mapXML) {
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
static std::vector<tmx::objectgroup> loadObjectGroups(tinyxml2::XMLElement* mapXML) {
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
    // main method
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