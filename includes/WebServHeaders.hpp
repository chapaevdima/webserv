#ifndef WEBSERV_WEBSERVHEADERS_HPP
#define WEBSERV_WEBSERVHEADERS_HPP


///General includes
#include <iostream>
#include <string>
#include <sstream>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>



///Socket includes

#include <netinet/in.h>
#include "sys/socket.h"
#include <sys/event.h>
#include <unistd.h>
#include "Socket.hpp"


///Server includes
#include "EventManager.hpp"
#include "Server.hpp"


#endif //WEBSERV_WEBSERVHEADERS_HPP
