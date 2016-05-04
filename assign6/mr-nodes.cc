/**
 * File: mr-nodes.cc
 * -----------------
 * Presents the (very straightforward) implementation of
 * loadMapReduceNodes.  In an ideal world this would actually
 * build a list of myth hostnames that were reachable and ssh'able,
 * but right now it just loads the list of hostnames from a file. #weak
 */

#include "mr-nodes.h"
#include <cstdlib>
#include <thread>
#include <mutex>
using namespace std;

static void pollMyth(size_t num, vector<string>& nodes, mutex& m) {
  string command = "ssh -o ConnectTimeout=5 myth" + to_string(num) + " date >& /dev/null";
  int ret = system(command.c_str());
  if (ret != 0) return;
  lock_guard<mutex> lg(m);
  nodes.push_back("myth" + to_string(num));
}

static const size_t kNumMyths = 40;
vector<string> loadMapReduceNodes() {
  vector<string> nodes;
  mutex m;
  thread threads[kNumMyths];
  for (size_t i = 0; i < kNumMyths; i++) threads[i] = thread(pollMyth, i + 1, ref(nodes), ref(m));
  for (thread& t: threads) t.join();
  return nodes;
}


