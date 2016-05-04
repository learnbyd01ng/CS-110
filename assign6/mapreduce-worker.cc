/**
 * File: mapreduce-worker.cc
 * -------------------------
 * Presents the implementation of the MapReduceWorker class.
 */

#include "mapreduce-worker.h"
#include <cassert>
#include <sstream>
#include "mr-messages.h"
#include "string-utils.h"
#include "client-socket.h"
#include "socket++/sockstream.h"
#include <iostream>
using namespace std;

MapReduceWorker::MapReduceWorker(const string& serverHost, unsigned short serverPort,
                                 const string& executablePath, const string& executable,
                                 const string& outputPath) :
  serverHost(serverHost), serverPort(serverPort), executablePath(executablePath), 
  executable(executable), outputPath(outputPath) {}


string MapReduceWorker::getOutputFileName(const string& inputFile) {
  char delimiter = '/';
  size_t pos,pos2 = 0;
  string input = inputFile;
  while ((pos2 = input.find(delimiter)) != string::npos) {
    pos = pos2;
    input = input.substr(pos+1);
  }
  return input;
}

void MapReduceWorker::work() {
  while (true) {
    string inputFile;
    if (!requestInputFile(inputFile)) break;
    string outFile = getOutputFileName(inputFile) + ".out";
    string command = executablePath + "/" + executable + " " + inputFile + " " + outputPath + "/" + outFile;
    int status = system(command.c_str());
    sendProgressReportToServer(inputFile, status);
  }
}

bool MapReduceWorker::requestInputFile(string& inputFile) {
  int clientSocket = getClientSocket();
  sockbuf sb(clientSocket);
  iosockstream ss(&sb);
  sendWorkerReady(ss);
  MRMessage message;
  string payload;
  receiveMessage(ss, message, payload);
  if (message == kServerDone) return false;
  inputFile = trim(payload);
  return true;
}

void MapReduceWorker::sendProgressReportToServer(const string& inputFile, int status) {
  int clientSocket = getClientSocket();
  sockbuf sb(clientSocket);
  iosockstream ss(&sb);
  if(status == 0) sendJobSucceeded(ss, inputFile);
  else sendJobFailed(ss, inputFile);
}

static const int kServerInaccessible = 2;
int MapReduceWorker::getClientSocket() {
  int clientSocket = createClientSocket(serverHost, serverPort);
  if (clientSocket == kClientSocketError) {
    exit(kServerInaccessible);
  }
  
  return clientSocket;
}
