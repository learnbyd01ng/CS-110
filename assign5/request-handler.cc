/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include <iostream>
#include "request.h"
#include "request-handler.h"
#include "client-socket.h"
#include "response.h"

#include <socket++/sockstream.h> // for sockbuf, iosockstream

using namespace std;

HTTPRequestHandler::HTTPRequestHandler(): blacklist("blocked-domains.txt"), cache() {}

static const unsigned short kDefaultHTTPPort = 80;
void HTTPRequestHandler::processRequest(HTTPRequest& request, HTTPResponse& response, HTTPCache& cache) {
  // Check if request is in cache and return cached response if it is.
  if(cache.containsCacheEntry(request,response)) return;

  // Create client socket
  int clientSocket = createClientSocket(request.getServer(), kDefaultHTTPPort);
  if (clientSocket == kClientSocketError) {
    cerr << "Could not connect to host named \""
         << request.getServer() << "\"" << endl;
    return;
  }
  // Create iosockstream and send request to server
  sockbuf sb2(clientSocket);
  iosockstream ss2(&sb2);
  ss2 << request;
  ss2.flush();

  // Ingest the response from the iosockstream 
  response.ingestResponseHeader(ss2);
  response.ingestPayload(ss2);
  response.setProtocol(request.getProtocol());

  // Cache the response if the response should be cached.
  if(cache.shouldCache(request,response)) cache.cacheEntry(request,response);
}

void HTTPRequestHandler::serviceRequest(const pair<int, string>& connection) throw() {
  sockbuf sb(connection.first);
  iosockstream ss(&sb);

  HTTPResponse response;
  HTTPRequest request;

  request.ingestRequestLine(ss);
  request.ingestHeader(ss, connection.second);
  request.ingestPayload(ss);
  request.addHeaders(connection.second);

  string server = request.getServer();
  if(blacklist.serverIsAllowed(server)) {
    cache.lockRequest(request);
    processRequest(request, response, cache);
    cache.unlockRequest(request);
  }
  else {
    response.setResponseCode(403);
    response.setPayload("Forbidden Content");
    response.setProtocol(request.getProtocol());
  }

  ss << response;
  ss.flush();
}
