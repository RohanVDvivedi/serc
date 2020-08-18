#include<file_request_resolution_controller.h>

#include<strhsh.h>

char* get_content_type_from_file_extension(char* extension);

int file_request_controller(HttpRequest* hrq, HttpResponse* hrp, file_cache* server_file_cache, int* routing_resolved)
{
	if(server_file_cache == NULL)
		return 0;

	if(hrq->method == GET && hrq->path.cstring[0] == '/')
	{
		// extract extension dstring from the file path
		dstring extension;
		init_dstring(&extension, "", 10);
		get_extension_from_file_path(&extension, &(hrq->path));

		// read file contents in the response body dstring
		if(read_file_in_dstring(&(hrp->body), server_file_cache, &(hrq->path)) == 0)
    		*routing_resolved = 1;

		// if the routing is resolved
		if(*routing_resolved == 1)
		{
			// add content-type for the requested file
			// and set tha status to 200
			if(extension.cstring[0] != '\0')
    			insert_unique_in_dmap_cstr(&(hrp->headers), "content-type", get_content_type_from_file_extension(extension.cstring));

    		hrp->status = 200;
    	}

    	// delete the extension and file_path dstrings
    	deinit_dstring(&(extension));
	}
	return 0;
}

char* get_content_type_from_file_extension(char* extension)
{
	unsigned int ext_hash = getHashValue(extension);
	switch(ext_hash)
	{
		case 20037 :
			return "text/html";
		
		case 8264 :
			return "text/css";
		
		case 12167 :
			return "text/csv";
		
		case 4706 :
			return "application/javascript";
		
		case 5996 :
			return "text/plain";
		
		case 7296 :
			return "text/xml";
		
		case 17411 :
			return "application/json";
		
		case 8066 :
			return "application/pdf";
		
		case 15746 :
			return "application/zip";
		
		case 5035 :
			return "image/gif";
		
		case 12512 :
		case 20545 :
			return "image/jpeg";
		
		case 8848 :
			return "image/png";
		
		case 13017 :
			return "image/tiff";
		
		case 16835 :
			return "image/svg+xml";
	}
	return NULL;
}