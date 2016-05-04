/**
 * File: news-aggregator-liberal.h
 * -------------------------------
 * Defines the subclass of NewsAggregator that is openly liberal about its
 * use of threads.  While it is smart enough to limit the number of threads
 * that can exist at any one time, it does not try to converve threads
 * by pooling or reusing them.  Instead, it creates a new thread
 * every time something needs to be downloaded.  It's easy, but wasteful.
 */

#pragma once
#include "news-aggregator-utils.h"
#include "html-document-exception.h"
#include "news-aggregator.h"
#include <map>
#include <set>
#include <mutex>
#include <memory>

#include "semaphore.h"
#include "article.h"

class LiberalNewsAggregator : public NewsAggregator {
public:
	LiberalNewsAggregator(const std::string& rssFeedListURI, bool verbose);

protected:  
	// implement the one abstract method required of all concrete subclasses
	void find_and_process_feed(std::string url);
	void processAllFeeds();
	void mapInsert(const Article& article);
	semaphore article_sem;
	semaphore feed_sem;


private:
	std::map<std::string, std::unique_ptr<semaphore>> server_sem_map;
	std::mutex url_set_lock;
	std::mutex rp_map_lock;
};
