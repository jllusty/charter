#pragma once

#include <string>
#include <fstream>

class Logger {
    bool firstlog = true;
    std::string filename;
    std::ofstream ofs;
public:
    Logger(std::string logfilename);
    ~Logger();
    std::ofstream& get();
};