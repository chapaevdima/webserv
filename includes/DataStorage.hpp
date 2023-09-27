#ifndef WEBSERV_DATASTORAGE_HPP
#define WEBSERV_DATASTORAGE_HPP

#include "WebServHeaders.hpp"
#include <map>

class DataStorage {
public:
    static std::string root;
    static std::map<int, std::string> defaultErrorPages;
    static std::map<std::string, std::string> mimeTypes;
private:
    static std::string getCurrentWorkingDirectory();
    static void initDefaultErrorPages();
};



#endif //WEBSERV_DATASTORAGE_HPP
