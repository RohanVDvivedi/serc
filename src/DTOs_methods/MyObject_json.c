#include<MyObject_json.h>
#include<jsonstringhandler.h>
#include<stdlib.h>
#include<stdio.h>


char* MyObject_toJson( MyObject* object )
{

	char number[20];
	JsonString* JS = getJsonString("{");

	sprintf(number,"%d,",object->myint);
	addToJsonString(JS,"\"myint\":");
	addToJsonString(JS,number);

	sprintf(number,"%ld,",object->mylongint);
	addToJsonString(JS,"\"mylongint\":");
	addToJsonString(JS,number);

	sprintf(number,"%u,",object->myuint);
	addToJsonString(JS,"\"myuint\":");
	addToJsonString(JS,number);

	sprintf(number,"%lu,",object->myulongint);
	addToJsonString(JS,"\"myulongint\":");
	addToJsonString(JS,number);

	addToJsonString(JS,"\"mystring\":\"");
	addToJsonString(JS,object->mystring);
	addToJsonString(JS,"\",");

	JS->string[JS->size-2] = '}';
	char* result = JS->string;
	free(JS);
	return result;
}