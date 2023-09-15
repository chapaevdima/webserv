//
// Created by Andy Natasha on 9/12/23.
//

#ifndef WEBSERV_LOCATION_HPP
#define WEBSERV_LOCATION_HPP


#include <string>
#include <vector>

class Location {
private:
	std::string _path;
	std::string _root;
	std::string _index;
	std::string _cgi_pass;
	bool _autoindex;
	bool _file_upload;
	std::vector<bool> _methods;
	unsigned long long _max_body_size;
public:
	Location();

	void setPath(const std::string &path);
	const std::string &getPath() const;
	const std::string &getRoot() const;
	const std::string &getIndex() const;
	const std::string &getCgiPass() const;
	bool isAutoindex() const;
	bool isFileUpload() const;
	const std::vector<bool> &getMethods() const;
	unsigned long long int getMaxBodySize() const;

	void root(std::stringstream &ss);
	void index(std::stringstream &ss);
	void cgi_pass(std::stringstream &ss);
	void autoindex(std::stringstream &ss);
	void file_upload(std::stringstream &ss);
	void methods(std::stringstream &ss);
	void max_body_size(std::stringstream &ss);

};

#endif //WEBSERV_LOCATION_HPP
