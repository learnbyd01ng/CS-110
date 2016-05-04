/**
 * File: news-aggregator-conservative.h
 * ------------------------------------
 * Defines the subclass of NewsAggregator that is more conservative with threads
 * than its more liberal peer.  It relies on a ThreadPool to limit the
 * number of threads ever created, and manages to recycle threads to execute many
 * functions instead of just one.
 */

#pragma once
#include "news-aggregator.h"
#include "news-aggregator-utils.h"
#include "html-document-exception.h"
#include "thread-pool.h"
#include "article.h"
#include <string>
#include <map>
#include <vector>
#include <utility>

class ConservativeNewsAggregator: public NewsAggregator {
public:
  ConservativeNewsAggregator(const std::string& rssFeedListURI, bool verbose);
  
protected:
  // implement the one abstract method required of all concrete subclasses
  void processAllFeeds();
  void find_and_process_feed(std::string url);
  void mapInsert(const Article& article);
  bool shouldInsert(std::map<std::pair<std::string, std::string>, std::pair<Article, std::vector<std::string> > >& rp_map, std::pair<std::string, std::string>& title_server, std::pair<Article, std::vector<std::string> >& Article_tokens);


private:
	std::mutex u_set_lock;
	std::mutex r_map_lock;
	ThreadPool feed_pool;
	ThreadPool art_pool;
};

