#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include<transaction_client.h>

#include<http_request.h>
#include<http_response.h>

// creates a new trnsaction client that handles asynchronous http request and response
// for connection_client = n, you will get a transaction client with n open connections, on n different threads, on 1 thread pool
transaction_client* get_http_client(char* url_string, char* port_string, unsigned int connection_count);

/* *************************************************
	There are 2 steps involved in sending a http request and then receiving the response
	1. job* promise_p = send_request_async(http_client_p, hrq);
		-> considering you are ready with the HttpRequest* hrq that you want to send
	2. HttpResponse* hrp = wait_or_get_response(promise_p);
		-> you will get in return the hrp HttpResponse*, you are suppossed to delete the HttpResponse object

	The more optimized way to send 3 request would be (only pseudo code given below)
		where you send two requests request1 and request2 to client1,
		and one request resuest3 to client2

	promise1 = send_request_async(client1, request1);
	promise2 = send_request_async(client1, request2);
	promise3 = send_request_async(client2, request3);

	response1 = wait_or_get_response(promise1);
	response2 = wait_or_get_response(promise2);
	response3 = wait_or_get_response(promise3);

	// do not forget to release request data
	deleteHttpRequest(request1);
	deleteHttpRequest(request2);
	deleteHttpRequest(request3);

	// do not forget to release response data
	deleteHttpResponse(response1);
	deleteHttpResponse(response2);
	deleteHttpResponse(response3);

	This way you would would be sending 3 requests in parallel on 3 completely different threads, 
	of 2 different thread pools (because there are 2 clients)
**************************************************** */

// it will queue it as a transaction on the transaction_client to serialize the HttpRequest and 
// then perform io on one of the other thread of the thread pool of the given transaction_client
promise* send_request_async(transaction_client* http_client, HttpRequest* hrq, char* host);

// this function will wait for the promise, to complete, for the given response promise
HttpResponse* wait_or_get_response(promise* response_promise);

// this will shutdown the http_client, that you started using get_http_client, and delete all of its resources
void shutdown_and_delete_http_client(transaction_client* http_client);

#endif