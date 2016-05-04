/**
 * File: request-handler.h
 * -----------------------
 * Defines the HTTPRequestHandler class, which fully proxies and
 * services a single client request.  
 */

#ifndef _http_request_handler_
#define _http_request_handler_

#include <utility>
#include <string>
#include "blacklist.h"
#include "cache.h"
#include <mutex>

class HTTPRequestHandler {
 public:
  HTTPRequestHandler();
  void setProxyType(bool& usingProxy, std::string& proxyServer, unsigned short& proxyPortNumber);
  void serviceRequest(const std::pair<int, std::string>& connection) throw();

 protected:
  void processRequest(HTTPRequest& request, HTTPResponse& response, HTTPCache& cache);

 private:
  HTTPBlacklist blacklist;
  HTTPCache cache;
  bool usingProxy;
  std::string proxyServer;
  unsigned short proxyPortNumber;
};

#endif
