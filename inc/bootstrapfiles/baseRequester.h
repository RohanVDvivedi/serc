#ifndef BASE_REQUESTER_H
#define BASE_REQUESTER_H

#include<stdio.h> /* printf, sprintf */
#include<stdlib.h> /* exit */
#include<unistd.h> /* read, write, close */
#include<string.h> /* memcpy, memset */
#include<sys/socket.h> /* socket, connect */
#include<netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include<netdb.h> /* struct hostent, gethostbyname */
#include<httpobject.h>
#include<logger.h>


// host = host name or ip address, port is the port number of the server it is running on
// hrp is pointer to the request to be sent there
// hrp is the pointer to the pointer to the response
// it will return status of success or failure
int retrieveResponse(char* host,int port,HttpRequest* hrq,HttpResponse** hrp);

#endif