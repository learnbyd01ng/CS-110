/**
 * File: scheduler.cc
 * ------------------
 * Presents the implementation of the HTTPProxyScheduler class.
 */

#include "scheduler.h"
#include <utility>
using namespace std;

#define NUM_MAX_THREADS 16

HTTPProxyScheduler::HTTPProxyScheduler() : pool(NUM_MAX_THREADS) {}

// Forwards the variables that determine whether to forward to a proxy and which proxyServer/portNumber to requestHandler
void HTTPProxyScheduler::setProxyType(bool& usingProxy, string& proxyServer, unsigned short& proxyPortNumber) {
	requestHandler.setProxyType(usingProxy, proxyServer, proxyPortNumber);
}

void HTTPProxyScheduler::scheduleRequest(int clientfd, const string& clientIPAddress) throw () {
  pool.schedule([this, clientfd, clientIPAddress]() {
        requestHandler.serviceRequest(make_pair(clientfd, clientIPAddress));
  });
}
