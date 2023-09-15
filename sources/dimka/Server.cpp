//
// Created by Andy Natasha on 9/11/23.
//

#include "../../includes/webserv.hpp"

Server::Server() {
	this->_port = 80;
	this->_server_name = "localhost";
	this->_host = "error";
	this->_error_pages = std::map<short, std::string>();
	this->_max_body_size = 1024;
	this->_locations = std::vector<Location>();
}

void Server::parseLocation(std::vector<std::string> &str, int& i)
{
	std::stringstream mss(str[i]);
	std::string location, word, path;
	Location res;
	mss >> location >> path >> word;
	if (location != "location" || word != "{" || mss.eof() != true)
		configError();
	if (path[0] != '/' && path[0] != '*')
		configError();
	res.setPath(path);
	i++;
	while (str[i] != "}" && i < static_cast<int>(str.size())) {
		std::stringstream ss(str[i]);
		std::string word;
		ss >> word;
		if (word == "root")
			res.root(ss);
		else if (word == "index")
			res.index(ss);
		else if (word == "cgi_pass")
			res.cgi_pass(ss);
		else if (word == "autoindex")
			res.autoindex(ss);
		else if (word == "file_upload")
			res.file_upload(ss);
		else if (word == "methods")
			res.methods(ss);
		else if (word == "client_max_body_size")
			res.max_body_size(ss);
		else
			configError();
		i++;
	}
	if (str[i] != "}")
		configError();
	this->_locations.push_back(res);
}

void Server::max_body_size(std::stringstream &ss)
{
	std::string word;
	ss >> word;
	if (word[word.size() - 1] != ';')
		configError();
	word.erase(word.size() - 1);
	if (ss.eof() != true)
		configError();
	for (int i = 0; i < static_cast<int>(word.size()); ++i)
	{
		char c = word[i];
		if (!isdigit(c))
			configError();
	}
	std::stringstream sss(word);
	unsigned long long max_body_size;
	sss >> max_body_size;
	this->_max_body_size = max_body_size;
}

void Server::listen(std::stringstream &ss)
{
	std::string word;
	ss >> word;
	if(word[word.size() -1] != ';')
		configError();
	word.erase(word.size() - 1);
	if(!isValidIP(word))
		configError();
	if (ss.eof() != true)
		configError();
	this->_host = word;
}

void Server::port(std::stringstream& ss)
{
	std::string word;
	ss >> word;
	if(!ss.eof())
		configError();
	if (word[word.size() - 1] != ';')
		configError();
	word.erase(word.size() - 1);
	for (int i = 0; i < static_cast<int>(word.size()); ++i)
	{
		char c = word[i];
		if (!isdigit(c))
			configError();
	}
	int port = std::atoi(word.c_str());
	if (port < 0 || port > 65535)
		configError();
	this->_port = port;
}

void Server::server_name(std::stringstream& ss)
{
	std::string word;
	ss >> word;
	if (word[word.size() - 1] != ';')
		configError();
	word.erase(word.size() - 1);
	if (ss.eof() != true)
		configError();
	this->_server_name = word;
}

void Server::error_page(std::stringstream& ss)
{
	std::string word;
	ss >> word;
	for (int i = 0; i < static_cast<int>(word.size()); ++i)
	{
		char c = word[i];
		if (!isdigit(c))
			configError();
	}
	int code = std::atoi(word.c_str());
	if (code < 100 || code > 599)
		configError();
	std::string path;
	ss >> path;
	if (!ss.eof())
		configError();
	if(path[path.size() - 1] != ';')
		configError();
	path.erase(path.size() - 1);
	/*std::ifstream infile(path);
	if (!infile.is_open())
		configError();
	infile.close();*/
	this->_error_pages[code] = path;
}

int Server::getPort() const {
	return _port;
}

const std::string &Server::getHost() const {
	return _host;
}

const std::string &Server::getServerName() const {
	return _server_name;
}

const std::map<short, std::string> &Server::getErrorPages() const {
	return _error_pages;
}

const std::vector<Location> &Server::getLocations() const {
	return _locations;
}

unsigned long long int Server::getMaxBodySize() const {
	return _max_body_size;
}
