#include<http_https_connection_handler.h>

#include<server_global_params.h>

#include<http_object.h>
#include<http_request.h>
#include<http_response.h>
#include<distributer.h>

#include<dstring.h>

#include<sys/socket.h>

// this is the buffer size that will be used to hold the data from IP/TCP or IP/TCP/SSL streams
#define buffersize 1024

void http_connection_handler(int conn_fd, void* server_specific_params)
{
	server_global_params* sgpp = server_specific_params;

	int is_secured_http = (sgpp->server_ssl_ctx != NULL);
	SSL* ssl = NULL;

	if(is_secured_http)
	{
		ssl = SSL_new(sgpp->server_ssl_ctx);
		SSL_set_fd(ssl, conn_fd);

		if(SSL_accept(ssl) == -1)
		{
			SSL_free(ssl);
			return;
		}
	}

	// set this in the loop, if you want to close the connection
	int close_connection = 0;

	// we loop on receving the 
	while(!close_connection)
	{

		// create buffer to read the request
		char bufferRequest[buffersize];
		int buffreadlength = -1;

		// create a new HttpRequest Object
		HttpRequest hrq; initHttpRequest(&hrq, conn_fd);
		http_connection_handler_error error = 0;

		while(1)
		{
			// read request byte array, we must read blockingly
			if(is_secured_http)
				buffreadlength = SSL_read(ssl, bufferRequest, buffersize);	// read ssl encrypted request
			else
				buffreadlength = recv(conn_fd, bufferRequest, buffersize, 0);

			// if no characters read than exit
			if(buffreadlength == -1)
			{
				error = TCP_CONNECTION_ERROR_READING;
				break;
			}
			else if(buffreadlength == 0)
			{
				error = TCP_CONNECTION_CLOSED_ABRUPTLY;
				break;
			}

			// parse the RequestString to populate HttpRequest Object
			error = parseRequest(bufferRequest, buffreadlength, &hrq);
			if(error == ERROR_OCCURRED_REQUEST_NOT_STANDARD_HTTP)
			{
				break;
			}
			else if(error == NO_ERROR_REQUEST_NOT_PARSED_COMPLETELY_CONTINUE_READING)
			{
				continue;
			}

			// if the request object parsing is completed then exit
			if(hrq.parseContext.state == PARSED_SUCCESSFULLY)
			{
				error = REQUEST_PARSED_SUCCESSFULLY;
				break;
			}
		}

		if(error == 0)
		{
			// create a HttpResponse Object here
			HttpResponse hrp; initHttpResponse(&hrp, conn_fd);

			close_connection = distribute(&hrq, &hrp, sgpp->server_file_cache);

			// Uncomment to Debug
			// printRequest(&hrq);
			// printResponse(&hrp);

			// this is the buffer, where we would store the serialized form of response, before sending
			dstring bufferResponse; init_dstring(&bufferResponse, NULL, 0);

			// serialize the response to be sent
			serializeResponse(&bufferResponse, &hrp);

			// send the data
			if(is_secured_http)
				SSL_write(ssl, bufferResponse.cstring, bufferResponse.bytes_occupied);
			else
				send(conn_fd, bufferResponse.cstring, bufferResponse.bytes_occupied, 0);

			// once data sent delete bufferResponse
			deinit_dstring(&bufferResponse);

			// delete HttpResponse Object
			deinitHttpResponse(&hrp);
		}
		else
		{
			// any error in reading the request, 
			// leads to closing of the connection
			close_connection = 1;
		}

		// deinitialize HttpRequest Object
		deinitHttpRequest(&hrq);
	}

	if(is_secured_http)
		SSL_free(ssl);
}
