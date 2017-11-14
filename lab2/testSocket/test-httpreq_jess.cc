// test-httpreq.cc - Unit Test for HTTP request parser / response generator
// (c) 2017 Christopher Mitchell, Ph.D.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include "httpreq.hpp"
#include "httpresp.hpp"
#include <iostream>
#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>


#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h> 

struct HTTPReqSample {
	const char* request;
	const char* method;
	const char* uri;
	const double version;
	const char* body;
};

const size_t n_req_samples = 4;
struct HTTPReqSample httpreq_samples[n_req_samples] = {
	{"POST /cgi-bin/process.cgi HTTP/1.1\r\n"
	 "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n"
	 "Host: www.tutorialspoint.com\r\n"
	 "Content-Type: application/x-www-form-urlencoded\r\n"
	 "Content-Length: 49\r\n"
	 "Accept-Language: en-us\r\n"
	 "Accept-Encoding: gzip, deflate\r\n"
	 "Connection: Keep-Alive\r\n"
	 "\r\n"
	 "licenseID=string&content=string&/paramsXML=string",
	 "POST", "/cgi-bin/process.cgi", 1.1,
	 "licenseID=string&content=string&/paramsXML=string"},

	{"GET /user1024 HTTP/1.1\r\n"
	 "SomeHeader: Random value here\r\n"
	 "AnotherHeader: Random value here\r\n"
	 "\r\n",
	 "GET", "/user1024", 1.1, ""},

	{"POST /user2048 HTTP/1.1\r\n"
	 "SomeHeader: Random value here\r\n"
	 "Content-length: 10\r\n"
	 "AnotherHeader: Random value here\r\n"
	 "\r\n"
	 "Abcdefghij",
	 "POST", "/user2048", 1.1, "Abcdefghij"},
	
	{"DELETE /user65535 HTTP/1.1\r\n"
	 "SomeHeader: Random value here\r\n"
	 "AnotherHeader: Random value here\r\n"
	 "\r\n",
	 "DELETE", "/user65535", 1.1, ""},

};

struct HTTPRespSample {
	const char* body;
	const unsigned int code;
	const bool keep_alive;
	const char* response;
};

const size_t n_resp_samples = 3;
struct HTTPRespSample httpresp_samples[n_resp_samples] = {
	{"<html>\r\n"
	 "<body>\r\n"
	 "<h1>Hello, World!</h1>\r\n"
	 "</body>\r\n"
	 "</html>\r\n",
	 200,
	 false,
	 "HTTP/1.1 200 OK\r\n"
	 "Content-Length: 58\r\n"
	 "Connection: close\r\n"
	 "\r\n"
	 "<html>\r\n"
	 "<body>\r\n"
	 "<h1>Hello, World!</h1>\r\n"
	 "</body>\r\n"
	 "</html>\r\n"},

	{"Page not found!",
	 404,
	 true,
	 "HTTP/1.1 404 Not Found\r\n"
	 "Content-Length: 15\r\n"
	 "Connection: keep-alive\r\n"
	 "\r\n"
	 "Page not found!"},

	{"Server error!",
	 500,
	 true,
	 "HTTP/1.1 500 Internal Server Error\r\n"
	 "Content-Length: 13\r\n"
	 "Connection: keep-alive\r\n"
	 "\r\n"
	 "Server error!"}
};

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

struct hostent *server;

void* ReqPusher(void* args) {
	char *req = (char*)args;
	// void* args = (void*)httpreq_samples[i].request;
	// HTTPReqSample *req = (HTTPReqSample*)args;
	struct sockaddr_in addr;
	int fd;
		
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		error("ERROR opening socket");
	}
	std::cout << "client socket created.\n";


	memset(&addr, 0, sizeof(addr));
	int portno = 2100;
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&addr.sin_addr.s_addr,
         server->h_length);
    addr.sin_port = htons(portno);


    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) 
        error("ERROR connecting");

    std::cout << "Client socket connected.\n";

    // int rval = write(fd, req, sizeof(*req));
    // if (rval < 0) {
    //     std::cout << "write error";
    //     exit(-1);
    // }


	size_t size_to_write = strlen(req);
	size_t offset = 0;
	while(size_to_write) {
		int rval = write(fd, req + offset, size_to_write - offset);
		if (rval < 0) {
			perror("write error");
			exit(-1);
		}
		offset += rval;
		size_to_write -= rval;
	}
	
	pthread_exit((void*)0);
}

int main(const int argc, const char* argv[]) {
	size_t passed = 0;
	if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
	server = gethostbyname(argv[1]);
	std:: cout << "hostname = " << server << std::endl;

	// Test the HTTPReq class
	for(size_t i = 0; i < n_req_samples; i++) {

		std:: cout << "Creating Thread " << i << std::endl;
		// Create a thread to push stuff into the socket
		pthread_t thread;
		void* args = (void*)httpreq_samples[i].request;
		if (0 != pthread_create(&thread, NULL, ReqPusher, (void*)args)) {
			error("Failed to create thread to push data into connection");
			// return -1;
		}
		
		void* rval;
		pthread_join(thread, &rval);

		passed++;
	}

    if(n_req_samples == passed){
    	std::cout << "Test Result: PASS\n";
    }
    else{
    	std::cout << "Test Result: FAIL\n";
    }
	return 0;
}
