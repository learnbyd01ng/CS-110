/**
 * File: news-aggregator-conservative.cc
 * -------------------------------------
 * Presents the implementation of the ConservativeNewsAggregator class.
 */

#include <algorithm>
#include "news-aggregator-conservative.h"
using namespace std;

ConservativeNewsAggregator::ConservativeNewsAggregator(const string& rssFeedListURI, bool verbose) : 
  NewsAggregator(rssFeedListURI, verbose), feed_pool(6), art_pool(24) {}

mutex u_set_lock;
mutex r_map_lock;

bool ConservativeNewsAggregator::shouldInsert(map<pair<string, string>, pair<Article, vector<string> > >& rp_map, pair<string, string>& title_server, pair<Article, vector<string> >& Article_tokens) {
    // Insert if title_server isn't already in the map
    lock_guard<mutex> lg(r_map_lock);
    if(rp_map.find(title_server) == rp_map.end()) return true;
    

    // Otherwise insert iff new entry is lexicographically before the old entry
    vector<string> newTokens = Article_tokens.second;
    vector<string> oldTokens = rp_map[title_server].second;

    // Take care of repitition
    sort(newTokens.begin(), newTokens.end());
    sort(oldTokens.begin(), oldTokens.end());

    vector<string> intersection;
    set_intersection(oldTokens.begin(), oldTokens.end(), newTokens.begin(), newTokens.end(), back_inserter(intersection));
    Article_tokens.second = intersection;
    
    Article oldArticle = rp_map[title_server].first;
    Article newArticle = Article_tokens.first;
    
    pair<Article, vector<string> > newEntry;
    newEntry.first = oldArticle;
    newEntry.second = intersection;

    rp_map.erase(title_server);

    // If the old article is being replaced, let the parent function take care of it.
    if(newArticle < oldArticle) return true;
   
    // Otherwise take care of it here.
    pair<pair<string, string> , pair<Article, vector<string> > > insert;
    insert.first = title_server;
    insert.second = newEntry;
    rp_map.insert(insert); 

    return false;
}

void ConservativeNewsAggregator::mapInsert(const Article& article) {
    HTMLDocument htmldoc(article.url);
    try {
    	htmldoc.parse();
    } catch (const HTMLDocumentException& hde) {
        NewsAggregatorLog log(true);
        log.noteSingleArticleDownloadFailure(article);
        return;
    }
    pair<string, string> title_server;
    title_server.first  = article.title;
    title_server.second = getURLServer(article.url);

    pair<Article, vector<string> > Article_tokens;
    vector<string> vec = htmldoc.getTokens();
    Article_tokens.first = article;
    Article_tokens.second = vec;
    
    // Insert article into rp_map if it should be inserted
    if(shouldInsert(rp_map,title_server,Article_tokens)) {
        pair<pair<string, string> , pair<Article, vector<string> > > tsat_pair;
        tsat_pair.first = title_server;
        tsat_pair.second = Article_tokens;
        r_map_lock.lock();
        rp_map.insert(tsat_pair);
        r_map_lock.unlock();
    }
}

void ConservativeNewsAggregator::find_and_process_feed(string url) {
    RSSFeed feed(url);
    try {feed.parse();}
    catch (RSSFeedException rfe){
        NewsAggregatorLog log(true);
        log.noteSingleFeedDownloadFailure(url);
        return;
    }
    vector<Article> articles = feed.getArticles();

    for(Article article : articles) {
        u_set_lock.lock();        // Lock the url set before checking if the url has been checked.

        // Only insert the article into the map if given a new url.
        if(urls.find(article.url) == urls.end()) {
            urls.insert(article.url);
            u_set_lock.unlock(); // Unlock this mutex as soon as possible.

            art_pool.schedule([this,article](){
                mapInsert(article);
            });
        } else u_set_lock.unlock();
    }
}

void ConservativeNewsAggregator::processAllFeeds() {
	RSSFeedList list(rssFeedListURI);
    try {list.parse();}
    catch (const RSSFeedException& rfe) {
        NewsAggregatorLog log(true);
        log.noteFullRSSFeedListDownloadFailureAndExit(rssFeedListURI);
        return;
    }
    auto listMap = list.getFeeds();

    // For each feed in the list, find and process the feed into rp_map.
    for(auto& iter : listMap) {
        string str = iter.first;
        feed_pool.schedule([this, str](){
            find_and_process_feed(str);
        });
    }
    feed_pool.wait();
    art_pool.wait();

    // Once the rp_map has been populated, add its entries to the index.
    for(auto& iter : rp_map) {
        Article article = iter.second.first;
        vector<string> words = iter.second.second;
        index.add(article, words);
    }

}
