/**
 * File: news-aggregator.cc
 * ------------------------
 * Presents the implementation of the NewsAggregator class.
 */

#include "news-aggregator.h"
#include "news-aggregator-log.h"
#include "news-aggregator-liberal.h"
#include "news-aggregator-conservative.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <algorithm>
#include <thread>

#include <getopt.h>
#include <libxml/parser.h>
#include <libxml/catalog.h>
#include "rss-feed-list.h"
#include "rss-feed.h"
#include "news-aggregator-utils.h"
#include "string-utils.h"

using namespace std;

static const string kDefaultRSSFeedListURL = "small-feed.xml";
NewsAggregator *NewsAggregator::createNewsAggregator(int argc, char *argv[]) {
  struct option options[] = {
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"url", required_argument, NULL, 'u'},
    {"conserve-threads", no_argument, NULL, 'c'},
    {NULL, 0, NULL, 0},
  };

  string rssFeedListURI = kDefaultRSSFeedListURL;
  bool verbose = false;
  bool conserve = false;
  while (true) {
    int ch = getopt_long(argc, argv, "vquc:", options, NULL);
    if (ch == -1) break;
    switch (ch) {
    case 'v':
      verbose = true;
      break;
    case 'q':
      verbose = false;
      break;
    case 'u':
      rssFeedListURI = optarg;
      break;
    case 'c':
      conserve = true;
      break;
    default:
      NewsAggregatorLog::printUsage("Unrecognized flag.", argv[0]);
    }
  }
  
  argc -= optind;
  if (argc > 0) NewsAggregatorLog::printUsage("Too many arguments.", argv[0]);
  
  if (conserve)
    return new ConservativeNewsAggregator(rssFeedListURI, verbose);
  else
    return new LiberalNewsAggregator(rssFeedListURI, verbose);
}

NewsAggregator::NewsAggregator(const string& rssFeedListURI, bool verbose):
  rssFeedListURI(rssFeedListURI), log(verbose), built(false) {}

void NewsAggregator::buildIndex() {
  if (built) return;
  built = false;
  xmlInitParser();
  xmlInitializeCatalog();
  processAllFeeds();
  xmlCatalogCleanup();
  xmlCleanupParser();
}

static const size_t kMaxMatchesToShow = 15;
void NewsAggregator::queryIndex() const {
  while (true) {
    cout << "Enter a search term [or just hit <enter> to quit]: ";
    string response;
    getline(cin, response);
    response = trim(response);
    if (response.empty()) break;
    const vector<pair<Article, int> >& matches = index.getMatchingArticles(response);
    if (matches.empty()) {
      cout << "Ah, we didn't find the term \"" << response << "\". Try again." << endl;
    } else {
      cout << "That term appears in " << matches.size() << " article" 
           << (matches.size() == 1 ? "" : "s") << ".  ";
      if (matches.size() > kMaxMatchesToShow) 
        cout << "Here are the top " << kMaxMatchesToShow << " of them:" << endl;
      else if (matches.size() > 1)
        cout << "Here they are:" << endl;
      else
        cout << "Here it is:" << endl;
      size_t count = 0;
      for (const pair<Article, int>& match: matches) {
        if (count == kMaxMatchesToShow) break;
        count++;
        string title = match.first.title;
        if (shouldTruncate(title)) title = truncate(title);
        string url = match.first.url;
        if (shouldTruncate(url)) url = truncate(url);
        string times = match.second == 1 ? "time" : "times";
        cout << "  " << setw(2) << setfill(' ') << count << ".) "
             << "\"" << title << "\" [appears " << match.second << " " << times << "]." << endl;
        cout << "       \"" << url << "\"" << endl;
      }
    }
  }
}
