/**
 * File: news-aggregator-liberal.cc
 * --------------------------------
 * Presents the implementation of the LiberalNewsAggregator class.
 */

#include "news-aggregator-liberal.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <thread>

using namespace std;

map<string, unique_ptr<semaphore>> server_sem_map;
mutex url_set_lock;
mutex rp_map_lock;

LiberalNewsAggregator::LiberalNewsAggregator(const string& rssFeedListURI, bool verbose) : 
  NewsAggregator(rssFeedListURI, verbose), article_sem(24), feed_sem(6)  {}


bool shouldInsert(map<pair<string, string>, pair<Article, vector<string> > >& rp_map, pair<string, string>& title_server, pair<Article, vector<string> >& Article_tokens) {
    // Insert if title_server isn't already in the map
    lock_guard<mutex> lg(rp_map_lock);
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

void LiberalNewsAggregator::mapInsert(const Article& article) {
    article_sem.signal(on_thread_exit);
    string server = getURLServer(article.url);
    (*(server_map[server].get())).signal(on_thread_exit);

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
    
    if(shouldInsert(rp_map,title_server,Article_tokens)) {
        pair<pair<string, string> , pair<Article, vector<string> > > tsat_pair;
        tsat_pair.first = title_server;
        tsat_pair.second = Article_tokens;
        rp_map_lock.lock();
        rp_map.insert(tsat_pair);
        rp_map_lock.unlock();
    }
}

void LiberalNewsAggregator::find_and_process_feed(string url) {
    feed_sem.signal(on_thread_exit);

    RSSFeed feed(url);
    try {feed.parse();}
    catch (RSSFeedException rfe){
        NewsAggregatorLog log(true);
        log.noteSingleFeedDownloadFailure(url);
        return;
    }
    vector<Article> articles = feed.getArticles();
    vector<thread> art_thr_vec;
    for(Article article : articles) {
        url_set_lock.lock();        // Lock the url set before checking if the url has been checked.
        if(urls.find(article.url) == urls.end()) {
            urls.insert(article.url);
            url_set_lock.unlock(); // Unlock this mutex as soon as possible.
            string server = getURLServer(article.url);

            if(server_map.find(server) == server_map.end()) {
                server_map[server] = unique_ptr<semaphore>(new semaphore(6));
            }
            (*(server_map[server].get())).wait();

            article_sem.wait();
            art_thr_vec.push_back(thread([this,article](){
                mapInsert(article);
            }));
        } else url_set_lock.unlock();
    }
    for(thread& t : art_thr_vec) t.join();
}

void LiberalNewsAggregator::processAllFeeds() {
    RSSFeedList list(rssFeedListURI);
    try {list.parse();}
    catch (const RSSFeedListException& rfle) {
        NewsAggregatorLog log(true);
        log.noteFullRSSFeedListDownloadFailureAndExit(rssFeedListURI);
        return;
    }

    auto listMap = list.getFeeds();

    vector<thread> thread_vec;
    for(auto& iter : listMap) {
        feed_sem.wait();
        string str = iter.first;
        thread_vec.push_back(thread([this, str](){
            find_and_process_feed(str);
        }));
    }
    for(thread& t : thread_vec) {
        t.join();    
    }

    for(auto& iter : rp_map) {
        Article article = iter.second.first;
        vector<string> words = iter.second.second;
        index.add(article, words);
    }
}
















