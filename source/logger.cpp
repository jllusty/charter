#include "logger.hpp"

Logger::Logger(std::string logfilename) {
    filename = logfilename;
    ofs.open(filename, std::ofstream::out);
}
Logger::~Logger() { ofs.close(); }
std::ofstream& Logger::get() {
    return ofs;
}