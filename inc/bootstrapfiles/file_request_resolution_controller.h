#ifndef FILE_REQUEST_RESOLUTION_CONTROLLER_H
#define FILE_REQUEST_RESOLUTION_CONTROLLER_H

#include<hashmap.h>
#include<dstring.h>

#include<file_handler.h>

#include<httpobject.h>

int file_request_controller(HttpRequest* hrq, HttpResponse* hrp, int* routing_resolved);

#endif