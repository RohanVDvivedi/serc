#include<http_request.h>

#include<http_object.h>

#include<zlib_compression_wrapper.h>

void initHttpRequest(HttpRequest* hr, int conn_fd)
{
	hr->conn_fd = conn_fd;

	initHttpParseContext(&(hr->parseContext));

	hr->method = UNIDENTIFIED;
	init_dstring(&(hr->path), NULL, 0);
	init_dstring(&(hr->version), NULL, 0);
	initialize_dmap(&(hr->parameters), CASE_SENSITIVE_KEY_TYPE, 3);
	initialize_dmap(&(hr->headers), CASE_INSENSITIVE_KEY_TYPE, 3);
	init_dstring(&(hr->body), NULL, 0);
}

// returns 0 when completed
// returns -1 on incomplete
// returns -2 when error
int parseRequest(char* buffer, int buffer_size, HttpRequest* hr)
{
	// this is the key corresponding to which value less patial keys of headers and parameters are stored
	static dstring partial_key_value_slize_key = {.cstring = "-<-PARTIAL_KEY_NO_VALUE->-", .bytes_occupied = strlen("-<-PARTIAL_KEY_NO_VALUE->-"), .bytes_allocated = 0};

	char* buff_start = buffer;
	while((buffer < (buff_start + buffer_size)) && hr->parseContext.state != PARSED_SUCCESSFULLY)
	{
		#define CURRENT_CHARACTER() 				(*buffer)
		#define RE_INIT_PARTIAL_STRING() 			make_dstring_empty(&(hr->parseContext.partialDstring));
		#define APPEND_CURRENT_CHARACTER_PARTIAL() 	concatenate_dstring(&(hr->parseContext.partialDstring), dstring_DUMMY_DATA(buffer, 1));
		#define APPEND_CURRENT_CHARACTER_TO(dstr) 	concatenate_dstring((dstr), dstring_DUMMY_DATA(buffer, 1));
		#define GOTO_NEXT_CHARACTER()        		buffer++;
		switch(hr->parseContext.state)
		{
			case NOT_STARTED :
			{
				if('A' <= CURRENT_CHARACTER() || CURRENT_CHARACTER() <= 'Z')
				{
					hr->parseContext.state = IN_METHOD;
					RE_INIT_PARTIAL_STRING()
				}
				else
				{
					return -2;
				}
				break;
			}
			case IN_METHOD :
			{
				if('A' <= CURRENT_CHARACTER() && CURRENT_CHARACTER() <= 'Z')
				{
					APPEND_CURRENT_CHARACTER_PARTIAL()
					GOTO_NEXT_CHARACTER()
				}
				else if(CURRENT_CHARACTER() == ' ')
				{
					hr->parseContext.state = METHOD_COMPLETE;
					hr->method = getHttpMethod(&(hr->parseContext.partialDstring));
					RE_INIT_PARTIAL_STRING()
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					return -2;
				}
				break;
			}
			case METHOD_COMPLETE :
			{
				if(CURRENT_CHARACTER() != ' ')
				{
					hr->parseContext.state = IN_PATH;
				}
				else
				{
					return -2;
				}
				break;
			}
			case IN_PATH :
			{
				if(CURRENT_CHARACTER() != ' ' && CURRENT_CHARACTER() != '?')
				{
					APPEND_CURRENT_CHARACTER_TO(&(hr->path))
					GOTO_NEXT_CHARACTER()
				}
				else if(CURRENT_CHARACTER() == ' ')
				{
					hr->parseContext.state = PATH_PARAMS_COMPLETE;
					GOTO_NEXT_CHARACTER()
				}
				else if(CURRENT_CHARACTER() == '?')
				{
					hr->parseContext.state = IN_PARAM_KEY;
					RE_INIT_PARTIAL_STRING()
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					return -2;
				}
				break;
			}
			case IN_PARAM_KEY :
			{
				if(CURRENT_CHARACTER() != '=')
				{
					APPEND_CURRENT_CHARACTER_PARTIAL()
					GOTO_NEXT_CHARACTER()
				}
				else if(CURRENT_CHARACTER() == '=')
				{
					insert_unique_in_dmap(&(hr->parameters), &partial_key_value_slize_key, &(hr->parseContext.partialDstring));
					RE_INIT_PARTIAL_STRING()
					hr->parseContext.state = IN_PARAM_VALUE;
					GOTO_NEXT_CHARACTER()
				}
				break;
			}
			case IN_PARAM_VALUE :
			{
				if(CURRENT_CHARACTER() != '&' && CURRENT_CHARACTER() != ' ')
				{
					APPEND_CURRENT_CHARACTER_PARTIAL()
					GOTO_NEXT_CHARACTER()
				}
				else if(CURRENT_CHARACTER() == '&')
				{
					dstring* partial_key = find_equals_in_dmap(&(hr->parameters), &partial_key_value_slize_key);
					insert_unique_in_dmap(&(hr->parameters), partial_key, &(hr->parseContext.partialDstring));
					make_dstring_empty(partial_key);
					RE_INIT_PARTIAL_STRING()
					hr->parseContext.state = IN_PARAM_KEY;
					GOTO_NEXT_CHARACTER()
				}
				else if(CURRENT_CHARACTER() == ' ')
				{
					dstring* partial_key = find_equals_in_dmap(&(hr->parameters), &partial_key_value_slize_key);
					insert_unique_in_dmap(&(hr->parameters), partial_key, &(hr->parseContext.partialDstring));
					make_dstring_empty(partial_key);
					RE_INIT_PARTIAL_STRING()
					hr->parseContext.state = PATH_PARAMS_COMPLETE;
					GOTO_NEXT_CHARACTER()
				}
				break;
			}
			case PATH_PARAMS_COMPLETE :
			{
				remove_from_dmap(&(hr->parameters), &partial_key_value_slize_key);
				if(CURRENT_CHARACTER() != ' ')
				{
					hr->parseContext.state = IN_VERSION;
				}
				else
				{
					return -2;
				}
				break;
			}
			case IN_VERSION :
			{
				if(CURRENT_CHARACTER() == '\r')
				{
					hr->parseContext.state = VERSION_COMPLETE;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					APPEND_CURRENT_CHARACTER_TO(&(hr->version))
					GOTO_NEXT_CHARACTER()
				}
				break;
			}
			case VERSION_COMPLETE :
			{
				if(CURRENT_CHARACTER() == '\n')
				{
					hr->parseContext.state = HEADER_START;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					return -2;
				}
				break;
			}
			case HEADER_START :
			{
				if(CURRENT_CHARACTER() == '\r')
				{
					hr->parseContext.state = HEADERS_COMPLETE;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					hr->parseContext.state = IN_HEADER_KEY;
					RE_INIT_PARTIAL_STRING()
				}
				break;
			}
			case IN_HEADER_KEY :
			{
				if(CURRENT_CHARACTER() == ':')
				{
					insert_unique_in_dmap(&(hr->headers), &partial_key_value_slize_key, &(hr->parseContext.partialDstring));
					RE_INIT_PARTIAL_STRING()
					hr->parseContext.state = HEADER_KEY_COMPLETE;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					APPEND_CURRENT_CHARACTER_PARTIAL()
					GOTO_NEXT_CHARACTER()
				}
				break;
			}
			case HEADER_KEY_COMPLETE :
			{
				if(CURRENT_CHARACTER() == ' ')
				{
					hr->parseContext.state = IN_HEADER_VALUE;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					return -2;
				}
				break;
			}
			case IN_HEADER_VALUE :
			{
				if(CURRENT_CHARACTER() == '\r')
				{
					dstring* partial_key = find_equals_in_dmap(&(hr->headers), &(partial_key_value_slize_key));
					insert_duplicate_in_dmap(&(hr->headers), partial_key, &(hr->parseContext.partialDstring));
					make_dstring_empty(partial_key);
					RE_INIT_PARTIAL_STRING()
					hr->parseContext.state = HEADER_VALUE_COMPLETE;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					APPEND_CURRENT_CHARACTER_PARTIAL()
					GOTO_NEXT_CHARACTER()
				}
				break;
			}
			case HEADER_VALUE_COMPLETE :
			{
				if(CURRENT_CHARACTER() == '\n')
				{
					hr->parseContext.state = HEADER_START;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					return -2;
				}
				break;
			}
			case HEADERS_COMPLETE :
			{
				remove_from_dmap(&(hr->headers), &partial_key_value_slize_key);
				if(CURRENT_CHARACTER() == '\n')
				{
					long long int body_length = -1;
					dstring* content_length = (dstring*) find_equals_in_dmap_cstr(&(hr->headers), "content-length");
					if(content_length != NULL)
					{
						// make content dstring sscanfable
						expand_dstring(content_length, 1);
						content_length->cstring[content_length->bytes_occupied] = '\0';

						sscanf(content_length->cstring, "%lld", &body_length);
					}
					if(hr->method == GET || body_length == 0)
					{
						hr->parseContext.state = PARSED_SUCCESSFULLY;
						GOTO_NEXT_CHARACTER()
						return 0;
					}
					else
					{
						hr->parseContext.state = IN_BODY;
						GOTO_NEXT_CHARACTER()
					}
				}
				else
				{
					return -2;
				}
				break;
			}
			case IN_BODY :
			{
				dstring* content_length = (dstring*) find_equals_in_dmap_cstr(&(hr->headers), "content-length");
				dstring* transfer_encoding = (dstring*) find_equals_in_dmap_cstr(&(hr->headers), "transfer-encoding");
				if(content_length != NULL)
				{
					int body_length = -1;

					// make content dstring sscanfable
					expand_dstring(content_length, 1);
					content_length->cstring[content_length->bytes_occupied] = '\0';

					sscanf(content_length->cstring, "%d", &body_length);

					if(body_length >= 0 && hr->body.bytes_occupied < body_length)
						APPEND_CURRENT_CHARACTER_TO(&(hr->body))

					if(body_length >= 0 && hr->body.bytes_occupied < body_length)
					{
						GOTO_NEXT_CHARACTER()
					}
					else
					{
						hr->parseContext.state = BODY_COMPLETE;
					}
				}
				else if(transfer_encoding != NULL && contains_dstring(transfer_encoding, dstring_DUMMY_CSTRING("chunked"), NULL) != NULL )
				{
					hr->parseContext.state = IN_BODY_CHUNK_SIZE;
				}
				else
				{
					return -2;
				}
				break;
			}
			case IN_BODY_CHUNK_SIZE :
			{
				if(CURRENT_CHARACTER() == '\r')
				{
					hr->parseContext.state = BODY_CHUNK_SIZE_COMPLETE;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					hr->parseContext.bodyBytesToRead <<= 4;
					hr->parseContext.bodyBytesToRead |= charToHex(*buffer);
					GOTO_NEXT_CHARACTER()
				}
				break;
			}
			case BODY_CHUNK_SIZE_COMPLETE :
			{
				if(CURRENT_CHARACTER() == '\n')
				{
					hr->parseContext.bodyBytesRead = 0;
					hr->parseContext.state = IN_BODY_CHUNK_CONTENT;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					return -2;
				}
				break;
			}
			case IN_BODY_CHUNK_CONTENT :
			{
				if(CURRENT_CHARACTER() == '\r' && hr->parseContext.bodyBytesToRead == hr->parseContext.bodyBytesRead)
				{
					hr->parseContext.state = BODY_CHUNK_CONTENT_COMPLETE;
					GOTO_NEXT_CHARACTER()
				}
				else
				{
					hr->parseContext.bodyBytesRead++;
					APPEND_CURRENT_CHARACTER_TO(&(hr->body))
					GOTO_NEXT_CHARACTER()
				}
				break;
			}
			case BODY_CHUNK_CONTENT_COMPLETE :
			{
				if(CURRENT_CHARACTER() == '\n')
				{
					if(hr->parseContext.bodyBytesToRead == 0)
					{
						hr->parseContext.state = BODY_COMPLETE;
					}
					else
					{
						hr->parseContext.state = IN_BODY_CHUNK_SIZE;
						hr->parseContext.bodyBytesToRead = 0;
						GOTO_NEXT_CHARACTER()
					}
				}
				else
				{
					return -2;
				}
				break;
			}
			case BODY_COMPLETE :
			{
				if(buffer == buff_start + buffer_size - 1)
				{
					hr->parseContext.state = PARSED_SUCCESSFULLY;
					return 0;
				}
				else
				{
					return -2;
				}
				break;
			}
			default :
			{
				return -2;
			}
		}
	}
	return -1;
}

void serializeRequest(dstring* result, HttpRequest* hr)
{
	concatenate_dstring(result, dstring_DUMMY_CSTRING(serializeHttpMethod(hr->method)));
	concatenate_dstring(result, dstring_DUMMY_CSTRING(" "));
	serializeUrl(result, hr);
	concatenate_dstring(result, dstring_DUMMY_CSTRING(" HTTP/1.1\r\n"));
	for_each_in_dmap(&(hr->headers), (void (*)(const dstring*, const dstring*, const void*))serialize_header_entry, result);
	concatenate_dstring(result, dstring_DUMMY_CSTRING("\r\n"));
	concatenate_dstring(result, &(hr->body));
}

void setServerDefaultHeadersInRequest(HttpRequest* hrq)
{
	char ptemp[13];
	// content-length header only if the request is not get
	if(hrq->method != GET)
	{
		sprintf(ptemp, "%u", hrq->body.bytes_occupied);
		insert_unique_in_dmap_cstr(&(hrq->headers), "content-length", ptemp);
	}
	insert_unique_in_dmap_cstr(&(hrq->headers), "accept-encoding", "gzip, deflate, identity");
}

void compressHttpRequestBody(HttpRequest* hrq, compression_type compr_type)
{
	// no compression/decompression for a GET request
	if(hrq->method == GET)
		return;

	// what will you do with compression of the request further more, 
	// datalink layer frame size only is around 1500 bytes
	if(hrq->body.bytes_occupied <= 100)
	{
		insert_unique_in_dmap_cstr(&(hrq->headers), "content-encoding", "identity");
		return;
	}

	// also do not compress, if it is already compressed (i.e. it has content-encoding header and it is not identity)
	if((find_equals_in_dmap_cstr(&(hrq->headers),"content-encoding") != NULL) && !hasHeader("content-encoding", "identity", &(hrq->headers)))
		return;

	// is_compressed == 0, means error, so we do not send any headers about compression
	int is_compressed = compress_in_memory(&(hrq->body), compr_type);
	if(!is_compressed)
		return;

	switch(compr_type)
	{
		case DEFLATE :
		{
			insert_unique_in_dmap_cstr(&(hrq->headers), "content-encoding", "deflate");	break;
		}
		case GZIP :
		{
			insert_unique_in_dmap_cstr(&(hrq->headers), "content-encoding", "gzip"   );	break;
		}
		case BROTLI :
		{
			insert_unique_in_dmap_cstr(&(hrq->headers), "content-encoding", "br"     );	break;
		}
		case IDENTITY :
		{
			insert_unique_in_dmap_cstr(&(hrq->headers), "content-encoding", "identity"); break;
		}
	}
}

void uncompressHttpRequestBody(HttpRequest* hrq)
{
	// no compression/decompression for a GET request
	if(hrq->method == GET)
		return;

	// we try to figure out the compression from inspecting these headers
	dstring* content_encoding = (dstring*) find_equals_in_dmap_cstr(&(hrq->headers), "content-encoding");
	dstring* transfer_encoding = (dstring*) find_equals_in_dmap_cstr(&(hrq->headers), "transfer-encoding");

	compression_type compr_type;

	if( (content_encoding != NULL && contains_dstring(content_encoding, dstring_DUMMY_CSTRING("br"), NULL) != NULL) ||
		(transfer_encoding != NULL && contains_dstring(transfer_encoding, dstring_DUMMY_CSTRING("br"), NULL) != NULL) )
		compr_type = BROTLI;
	else if( (content_encoding != NULL && contains_dstring(content_encoding, dstring_DUMMY_CSTRING("deflate"), NULL) != NULL) ||
		(transfer_encoding != NULL && contains_dstring(transfer_encoding, dstring_DUMMY_CSTRING("deflate"), NULL) != NULL) )
		compr_type = DEFLATE;
	else if( (content_encoding != NULL && contains_dstring(content_encoding, dstring_DUMMY_CSTRING("gzip"), NULL) != NULL) ||
		(transfer_encoding != NULL && contains_dstring(transfer_encoding, dstring_DUMMY_CSTRING("gzip"), NULL) != NULL) )
		compr_type = GZIP;
	else{return ;}

	uncompress_in_memory(&(hrq->body), compr_type);

	remove_from_dmap_cstr(&(hrq->headers), "content-encoding");
	remove_from_dmap_cstr(&(hrq->headers), "transfer-encoding");
}

void deinitHttpRequest(HttpRequest* hr)
{
	deinitHttpParseContext(&(hr->parseContext));

	deinit_dstring(&(hr->path));
	deinit_dstring(&(hr->version));
	deinitialize_dmap(&(hr->parameters));
	deinitialize_dmap(&(hr->headers));
	deinit_dstring(&(hr->body));
}

void printRequest(HttpRequest* hr)
{
	printf("method : %s\n", serializeHttpMethod(hr->method));
	printf("path : "); printf_dstring(&(hr->path)); printf("\n");
	printf("parameters : \n"); for_each_in_dmap(&(hr->parameters), print_entry_wrapper, NULL); printf("\n");
	printf("version : "); printf_dstring(&(hr->version)); printf("\n");
	printf("headers : \n"); for_each_in_dmap(&(hr->headers), print_entry_wrapper, NULL); printf("\n");
	printf("body : "); printf_dstring(&(hr->body)); printf("\n\n");
}

dstring* getCookie(HttpRequest* hr)
{
	return (dstring*) find_equals_in_dmap_cstr(&(hr->headers), "Cookie");
}

void serializeUrl(dstring* result, HttpRequest* hr)
{
	for(int i=0; i<hr->path.bytes_occupied; i++)
	{
		char temp[10];
		if( characterAllowedInURL(hr->path.cstring[i]) || hr->path.cstring[i]=='/')
		{
			temp[0] = hr->path.cstring[i];
			temp[1] = '\0';
		}
		else
		{
			temp[0] = '%';
			temp[1] = hexToChar((hr->path.cstring[i] >> 4) & 0x0f);
			temp[2] = hexToChar(hr->path.cstring[i] & 0x0f);
			temp[3] = '\0';
		}
		concatenate_dstring(result, dstring_DUMMY_CSTRING(temp));
	}
	if(hr->parameters.map.element_count > 0)
	{
		concatenate_dstring(result, dstring_DUMMY_CSTRING("?"));
		for_each_in_dmap(&(hr->parameters), (void (*)(const dstring*, const dstring*, const void*))serialize_parameter_entry, result);
	}
}