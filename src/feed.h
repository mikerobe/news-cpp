#ifndef _MR_NEWS_FEED_H
#define _MR_NEWS_FEED_H

#include <iostream>
#include <set>
#include <map>

#include "export.h"
#include "mr_threads.h"
#include "mr_guiviewer.h"
#include "mr_xml.h"
#include "settings.h"
#include "mr_sql.h"
#include "mr_set.h"

namespace news {

class Category;
class Item;
class News;

class _API_ Feed {
    public:
        enum FeedStatus {
            FS_OK     = 0,
            FS_ER     = 1,
            FS_UNREAD = 2,
        };

        enum FeedFormat {
            FT_XML  = 0,
            FT_HTML = 1,
        };

        static const char* statusName[];
        static const int statusCOL[];
        static const int statusATR[];
        static const char* formatName[];

    public:
        Feed(const char * name = "", News *news = NULL)
        : delete_(0)
         ,isAll(false)
         ,init_(false)
         ,id(-1)
         ,title(name)
         ,maxItems(100)
         ,lastReadItemId(0), numUnread(0)
         ,numItems(0)
         ,numUnarchived(0)
         ,format(FT_XML)
         ,checkAllTitles(false)
         ,name(name)
         ,statusLine("Not updated."), status(FS_OK)
         ,mutexData(mrutils::mutexCreate())
         ,mutexUpdate(mrutils::mutexCreate())
         ,mutexInit(mrutils::mutexCreate())
         ,lastUpdate(0), updateSeconds(300)
         ,news(news)
        {
            // add 0 dateItem 
            dateItems.push_back(new std::vector<Item*>());
            dates.push_back(0);
        }

        ~Feed();

    public:
       /****************
        * Outside methods
        ****************/

        void initUnread(int numUnread, char * queryBuffer);
        void initUnread(mrutils::Sql& sql, char * queryBuffer);

        /**
         * Returns -1 on error. or the number of added items.
         * */
        int update(mrutils::Sql& sql, mrutils::XML& xml, mrutils::XML& xmlContent, int stopFD, volatile bool * exit_
            ,char * buffer, int size
            ,mrutils::GuiViewer& viewer
            ,Feed*& activeFeed
            ,mrutils::mutex_t& mutexActiveFeed, mrutils::mutex_t& mutexFeeds
            );

        void updateLastReadItemId(const int lastReadItemId, mrutils::Sql& sql
            ,char * buffer, int size
            ,mrutils::GuiViewer& viewer
            ,Feed*& activeFeed
            ,mrutils::mutex_t& mutexActiveFeed, mrutils::mutex_t& mutexFeeds
            );

       /**
        * Uses items in the database matching itemsCriterion (which is
        * of the form "itemid IN (1,2,3)") to create items & add.
        */
        void updateFromDB(const char * itemsCriterion,mrutils::Sql& sql
            ,char * buffer, int size
            ,mrutils::GuiViewer& viewer
            ,Feed*& activeFeed
            ,mrutils::mutex_t& mutexActiveFeed, mrutils::mutex_t& mutexFeeds
        );

        void guiEnter(mrutils::Sql& sql, mrutils::GuiViewer& viewer,
            mrutils::XML& xml, mrutils::XML &xmlSecondary, char * buffer, int size);

        // this is if the feed isAll. items are added from other feeds
        bool add(Item* item, mrutils::GuiViewer* viewer, Feed*& activeFeed, mrutils::mutex_t& mutexActiveFeed);
        void allDeleteFeed(Feed* feed, mrutils::GuiViewer* viewer = NULL, Feed** activeFeed = NULL, mrutils::mutex_t* mutexActiveFeed = NULL);
        void allAddFeed(Feed* feed);
        void deleteFromAll(mrutils::GuiViewer* viewer, Feed*& activeFeed, mrutils::mutex_t& mutexActiveFeed);

    public:
       /****************
        * Populators
        ****************/

        void viewChooserOptions(mrutils::ColChooser&c, void*);
        void viewSelection(mrutils::ColChooser&c, void*);

    public:
       /****************
        * Locally-triggered functions
        ****************/

        bool refresh();
        bool openBrowser();
        bool historyShow();
        bool regetItem();
        bool searchMatch(mrutils::ColChooser& c, int id, const char * search, void*);
        void itemTargetedGui(mrutils::ColChooser& c, int id, bool targeted, void*); 
        void itemTargeted(Item * item, bool tf);

        /**
         * Returns the item if already created, else NULL
         */
        inline Item *getItemById(int itemId) const
        {
            std::map<int,Item*>::const_iterator itItem
                    = itemsById.find(itemId);
            if (itItem == itemsById.end())
                return NULL;
            return itItem->second;
        }

    public:
       /*****************
        * Utilities
        *****************/

        inline bool operator<(const Feed& other) const {
            if (this == &other) return false;
            if (isAll) return true;
            if (other.isAll) return false;
            return (strcmp(title.c_str(),other.title.c_str()) < 0);
        }

        inline friend std::ostream& operator<<(std::ostream& os, const Feed& feed) {
            return os << feed.title;
        }

    private:
        void init(mrutils::Sql& sql, char * buffer, int size, mrutils::GuiViewer* viewer, Feed*& activeFeed);
        void clearItems(mrutils::Sql& sql, char * buffer, int size);
        void addItemHelper(Item* item
            ,char * buffer, int size
            ,mrutils::GuiViewer& viewer
            ,Feed*& activeFeed
            ,mrutils::mutex_t& mutexActiveFeed, mrutils::mutex_t& mutexFeeds);
        int updateFinish(mrutils::Sql& sql,
            int addedItems
            ,char * buffer, int size
            ,mrutils::GuiViewer& viewer, Feed*& activeFeed, mrutils::mutex_t& mutexActiveFeed);

    public:
        int delete_; bool isAll;
        bool init_; // whether data from database is local
        int id;
        std::string title, link;
        int maxItems; int lastReadItemId, numUnread; int numItems, numUnarchived;
        FeedFormat format;
        bool checkAllTitles;

        std::string name; // title + (# unread)

        std::string statusLine;
        FeedStatus status;

        static Category* allCategory;
        mrutils::set<Category*,mrutils::ptrCmp_t<Category> > categories;
        
        mrutils::mutex_t mutexData;
        std::vector<int> dates;

        typedef std::vector<std::vector<Item*>*> dateItemsT;
        dateItemsT dateItems; // date number (i.e., 0, 1, 2, 3) to items

        std::map<int, Item*> itemsById;

        mrutils::mutex_t mutexUpdate, mutexInit;
        time_t lastUpdate;
        unsigned updateSeconds;

        // tmp variable
        mrutils::GuiViewer *_viewer;
        mrutils::Sql * _sql;

        // to avoid dups in the titles
        std::set<std::string> allTitles;

        News *news;
};
}


#endif
