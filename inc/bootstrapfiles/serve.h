#ifndef SERVE_H
#define SERVE_H

#include<stdio.h>
#include<stdlib.h>

#include<server.h>

#include<string.h>
#include<httpobject.h>
#include<distributer.h>

#define buffersize 100

void server_run(uint16_t PORT);

#endif