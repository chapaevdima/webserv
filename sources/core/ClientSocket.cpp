#include "ClientSocket.hpp"
#include "DataStorage.hpp"
#include <algorithm>

ClientSocket::kEvent &ClientSocket::getClientInterest() {
    return _clientInterest;
}

void ClientSocket::Read(const std::string &read) {
    ClientSocket::_read = read;
}

void ClientSocket::MuchWritten(size_t muchWritten) {
    _much_written = muchWritten;
}

ClientSocket::ClientSocket(int socket, int kq, const std::vector<ServerConfig> &configs) {
    _config = configs;
    struct sockaddr_in clientAddr;
    _socket = socket;
    _much_written = 0;
    socklen_t clientAddrLen = sizeof(clientAddr);
    setSocket(accept(socket, (struct sockaddr *) &clientAddr, &clientAddrLen));
    checkSocket(_socket);
    fcntl(_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
    EV_SET(&_clientInterest, _socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &_clientInterest, 1, NULL, 0, NULL);
}

void ClientSocket::setClientInterest(const ClientSocket::kEvent &clientInterest) {
    _clientInterest = clientInterest;
}
bool ClientSocket::isValidRequest() {
    if (Request.RequestData.empty())
        return false;
    Request.parse_request(Request.RequestData);
    if (Request.isVersion())
        return true;
    ///TODO add correct Request validation
    return false;
}
void ClientSocket::generateCGIResponse(const std::string &path, const Location &location) {
    std::cout << Request.RequestData << std::endl;
    std::cout << "its all request)" << std::endl;
    const char *pythonScriptPath = path.c_str();
    const char *pythonInterpreter = location.getCgiPass().c_str();
    std::string pathInfo;
    std::string pathTranslated;
    std::string tmpBodyFile;
    int hasBody = Request.getMethod() == "POST" ? 1 : 0;
    char **pythonEnv = new char *[3 + Request.getArgs().size() + hasBody];
    std::map<std::string, std::string> env = Request.getArgs();
    if (!Request.getBody().empty()) {
        tmpBodyFile = "BODY_" + std::to_string(_socket);
        std::ofstream file(DataStorage::root + "/www/" + tmpBodyFile.c_str());
        file << Request.getBody();
        file.close();
    }
    pathInfo = path.substr(path.find(".py") + 3, path.size() - 1);
    pathTranslated = "PATH_TRANSLATED=" + DataStorage::root + "/www/" + pathInfo;
    pathInfo = "PATH_INFO=" + pathInfo;
    pythonEnv[0] = strdup(pathInfo.c_str());
    pythonEnv[1] = strdup(pathTranslated.c_str());
    ///put all args in env
    int i = 2;
    for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); it++) {
        std::string tmp = it->first + "=" + it->second;
        pythonEnv[i] = strdup(tmp.c_str());
        i++;
    }
    if(hasBody)
    {
        pythonEnv[i++] = strdup(("BODY_FILE=" + DataStorage::root + "/www/" + tmpBodyFile).c_str());
        std::cout << "BODY_FILE=" + DataStorage::root + "/www/" + tmpBodyFile << std::endl;
    }
    pythonEnv[i] = NULL;
    ///generate args for execve
    char **pythonArgs = new char *[3];
    pythonArgs[0] = strdup(pythonInterpreter);
    pythonArgs[1] = strdup(pythonScriptPath);
    pythonArgs[2] = NULL;
    std::string tmpCGIFile = "CGI" + std::to_string(_socket);
    int fdCGIFile = open(( DataStorage::root + "/www/" + tmpCGIFile).c_str(), O_RDWR | O_CREAT, 0666);
    if (fdCGIFile == -1) {
        perror("Ошибка при открытии файла");
        exit(1);
    }
    int pid = fork();
    if (!pid) {
        dup2(fdCGIFile, 1);
        if (execve(pythonInterpreter, pythonArgs, pythonEnv) == -1) {
            perror("Ошибка при выполнении execve");
            exit(1);
        }
        close(fdCGIFile);
    }
    waitpid(pid, NULL, 0);
    std::ifstream file((DataStorage::root + "/www/" + tmpCGIFile).c_str());
    std::getline(file, Response.Body, '\0');
    std::cout << Response.Body << std::endl;
    std::cout << "its all body)" << std::endl;
    file.close();
    remove((DataStorage::root + "/www/" + tmpCGIFile).c_str());
    remove((DataStorage::root + "/www/" + tmpBodyFile).c_str());
    delete[] pythonArgs;
    delete[] pythonEnv;
    Response.ResponseData = Response.Status + Response.Body;
    std::cout << Response.ResponseData << std::endl;
    std::cout << "its all response)" << std::endl;
    Response.sentLength = 0;
}

void ClientSocket::generateResponse() {
    std::string method = Request.getMethod();
    std::string location = Request.getPath();
    std::string host = Request.getHeaders().find("Host")->second;
    bool autoindex = Request.getArgs().find("autoindex") != Request.getArgs().end();
    std::string path = Request.getPath();
    ServerConfig currentConfig;
    Location currentLocation;
    std::string root;
    host = host.substr(0, host.find(':'));
    currentConfig = _config[0];
    std::vector<Location> locations = _config[0].getLocations();
    ///get config by host another will be default
    std::string configHost;
    for (size_t i = 0; i < _config.size(); i++) {
        if (_config[i].getHost() == host) {
            currentConfig = _config[i];
            locations = _config[i].getLocations();
            break;
        }
    }
    ///go through first config and find location
    for (size_t j = 0; j < locations.size(); j++) {
        if (locations[j].getPath() == location) {
            root = locations[j].getRoot();
            currentLocation = locations[j];
            break;
        }
    }
    if (root.empty() && !autoindex) {
        std::cout << "empty root" << std::endl;
        generateErrorPage(currentConfig, 404);
        return;
    }
    ///check method
    if (!isValidMethod(method, currentLocation) && !autoindex) {
        std::cout << "invalid method" << std::endl;
        generateErrorPage(currentConfig, 405);
        return;
    }
    ///create response for autoindex
    if (currentLocation.isAutoindex() || autoindex) {
        std::cout << "autoindex" << std::endl;
        std::string html = generate_autoindex(DataStorage::root + "/www", currentConfig.getServerName() + std::to_string(currentConfig.getPort()));
        Response.Body = html;
        Response.ResponseData = Response.Status + Response.Body;
        return;
    }
    ///create response for index
    if (root[root.size() - 1] == '/')
        root += currentLocation.getIndex();
    getFoolPath(root);
    getDataByFullPath(root, currentConfig, currentLocation);
}

std::string
ClientSocket::generate_autoindex(const std::string &rootPath, const std::string &location) {
    DIR *dir;
    struct dirent *ent;
    struct stat filestat;
    std::stringstream html;
    std::string path = rootPath + location;
    html << "<html><body><ul>";

    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string filepath = path + "/" + ent->d_name;
            stat(filepath.c_str(), &filestat);

            std::string mod_time = ctime(&filestat.st_mtime);
            mod_time = mod_time.substr(0, mod_time.size() - 1);  // remove trailing newlinelocation + "/"
            if (location[location.size() - 1] == '/') {
                html << "<li><a href=\"" << location + ent->d_name << "?autoindex=1\">" << ent->d_name << "</a> "
                     << " (size: " << filestat.st_size << ", "
                     << "modified: " << mod_time << ")</li>";
            } else {
                html << "<li><a href=\"" << location + "/" + ent->d_name << "?autoindex=1\">" << ent->d_name << "</a> "
                     << " (size: " << filestat.st_size << ", "
                     << "modified: " << mod_time << ")</li>";
            }
        }
        closedir(dir);
    } else {
        std::fstream file(path.c_str());
        std::cout << path << std::endl;

        if (!file.is_open())
            html << "Error: cannot open file" << std::endl;
        else {
            file >> html.rdbuf();
            file.close();
            Response.Status = "HTTP/1.1 200 OK\n"
                              "Content-Type: image/jpeg\n\n";
        }
    }

    html << "</ul></body></html>";

    return html.str();
}

void ClientSocket::getDataByFullPath(const std::string &path, const ServerConfig &config, const Location &location) {
    std::ifstream file(path.c_str());
    std::string str;
    std::string response;
    ///handle cgi(.py scripts)
    if (isCGI(path)) {
        std::cout << "CGI" << std::endl;
        generateCGIResponse(path, location);
        return;
    }
        ///handle static file
    else if (file.is_open()) {
        while (std::getline(file, str)) {
            response += str + "\n";
        }
        file.close();
        Response.Body = response;
        Response.ResponseData = Response.Status + Response.Body;
        Response.sentLength = 0;
    }
        ///handle error page
    else {
        generateErrorPage(config, 404);
        return;
    }

}

void ClientSocket::generateErrorPage(const ServerConfig &config, int errorNumber) {
    std::__1::map<short, std::string> errors = config.getErrorPages();
    std::__1::map<short, std::string>::iterator it = errors.find(errorNumber);
    Response.generateDefoultErrorPage(errorNumber);
    std::string errorRoot = it->second;
    getFoolPath(errorRoot);
    getErrorPageData(errorRoot);
}

void ClientSocket::getFoolPath(std::string &pathToUpdate) const {
    size_t found = pathToUpdate.find("/FULL_PATH_TO_FILE");
    pathToUpdate.replace(found, sizeof("/FULL_PATH_TO_FILE") - 1, DataStorage::root);
}

ClientSocket::ClientSocket(const ClientSocket &socket) : ServerSocket(socket) {
    _socket = socket._socket;
    _clientInterest = socket._clientInterest;
    _config = socket._config;
    _read = socket._read;
    _much_written = socket._much_written;
    Request = socket.Request;
    Response = socket.Response;
}

ClientSocket &ClientSocket::operator=(const ClientSocket &socket) {
    if (this == &socket)
        return *this;
    _socket = socket._socket;
    _clientInterest = socket._clientInterest;
    _config = socket._config;
    _read = socket._read;
    _much_written = socket._much_written;
    Request = socket.Request;
    Response = socket.Response;
    return *this;
}

bool ClientSocket::operator==(const ClientSocket &socket) const {
    return _socket == socket._socket;
}

void ClientSocket::getErrorPageData(const std::string &errorRoot) {
    std::ifstream file(errorRoot.c_str());
    std::string str;
    std::string response;
    if (file.is_open()) {
        while (std::getline(file, str)) {
            response += str + "\n";
        }
        file.close();
    } else {
        //generate error page
        std::cout << "error page" << std::endl;
        Response.ResponseData = Response.Status + Response.Body;
        return;
    }
    Response.Body = response;
    Response.ResponseData = Response.Status + Response.Body;
    Response.sentLength = 0;
}

bool ClientSocket::isValidMethod(const std::string &method, const Location &location) {
    if (method == "GET") {
        return location.getMethods()[0];
    }
    if (method == "POST") {
        return location.getMethods()[1];
    }
    if (method == "DELETE") {
        return location.getMethods()[2];
    }
    return false;
}

bool ClientSocket::isCGI(std::string path) {
    ///check that last 2 symbols is .py
    if (path.size() > 2 && path.substr(path.size() - 3, 3) == ".py")
        return true;
    return false;
}

