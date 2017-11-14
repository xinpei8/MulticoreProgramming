#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
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
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    //Test HTTP requests
    for(size_t i = 0; i < n_req_samples; i++) {
        void* args = (void*)httpreq_samples[i].request;
        if (0 != pthread_create(&thread, NULL, ReqPusher, (void*)args)) {
            std::cerr << "Failed to create thread to push data into connection" << std::endl;
            return -1;
        }

        char* req = (char*)args;
        struct sockaddr_un addr;


        // Set up the addr
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        if (*socket_path == '\0') {
            *addr.sun_path = '\0';
            strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
        } else {
            strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
        }


        size_t size_to_write = strlen(req);
        size_t offset = 0;
        while(size_to_write) {
            int rval = write(sockfd, req + offset, size_to_write - offset);
            if (rval < 0) {
                perror("write error");
                exit(-1);
            }
            offset += rval;
            size_to_write -= rval;
        }
    }




    // printf("Please enter the message: ");
    // bzero(buffer,256);
    // fgets(buffer,255,stdin);
    // n = write(sockfd,buffer,strlen(buffer));
    // if (n < 0) 
    //      error("ERROR writing to socket");
    // bzero(buffer,256);
    // n = read(sockfd,buffer,255);
    // if (n < 0) 
    //      error("ERROR reading from socket");
    // printf("%s\n",buffer);
    close(sockfd);
    return 0;
}
