/**
 * File: news-aggregator.h
 * -----------------------
 * Defines the NewsAggregator class, which understands how to 
 * build an index of all HTML articles reachable from a 
 * command-line-provided RSS feed list URI and interact
 * with a user interested in searching that index.
 *
 * Note that NewsAggregator is an abstract base class, and it
 * exists to be subclassed and unify state and logic common to
 * all subclasses into one location.
 */

#pragma once
#include <string>
#include <map>
#include <set>
#include <mutex>
#include <memory>

#include "semaphore.h"

#include "news-aggregator-log.h"
#include "rss-index.h"
#include "html-document.h"
#include "rss-feed-list.h"
#include "rss-feed.h"

class NewsAggregator {
 public: // define those entries that everyone can see and touch
  static NewsAggregator *createNewsAggregator(int argc, char *argv[]);
  virtual ~NewsAggregator() {}
  
  void buildIndex();
  void queryIndex() const;
  
 protected: // defines those entries that only subclasses can see and touch
  std::string rssFeedListURI;
  NewsAggregatorLog log;
  RSSIndex index;
  bool built;
  std::map<std::pair<std::string, std::string>, std::pair<Article, std::vector<std::string> > > rp_map;
  std::set<std::string> urls;
  std::map<std::string, std::unique_ptr<semaphore> > server_map;

  
  NewsAggregator(const std::string& rssFeedListURI, bool verbose);
  virtual void processAllFeeds() = 0;
  
 private: // defines those entries that only NewsAggregator methods (not even subclasses) can see and touch
  NewsAggregator(const NewsAggregator& original) = delete;
  NewsAggregator& operator=(const NewsAggregator& rhs) = delete;
};
