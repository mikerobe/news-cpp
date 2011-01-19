#pragma once

#include "mr_guiviewer.h"
#include "mr_guiselect.h"
#include "mr_dialog.h"
#include "mr_xml.h"
#include "mr_threads.h"
#include "mr_selinterrupt.h"
#include "mr_except.h"
#include "settings.h"
#include "history.h"
#include "mr_map.h"
#include "mr_set.h"

namespace news {

class Category;
class Feed;

enum mode_t
{
    MD_SERVER,
    MD_CLIENT,
    MD_SILENT,
	MD_VIEW
};

class News
{
public:
    News(mode_t mode) throw (std::runtime_error) :
        feedChooser(2),
        feedViewer(2,feedChooser.termLine.winRows-10),
        mode(mode),
        feedCatChooser(1, 5,
            (feedChooser.termLine.winCols - 30)/2, -10, 30, true),
        exit_(false),
        mainThreadSql(NULL),
        activeFeed(NULL),
        reader("",1024*1024),
        writer(5*1024*1024),
        feedMap(256),
        categoryMap(64),
        online(true)
    {

        feedChooser.colChooser.setShowNumbers(false);
        feedChooser.colChooser.setOptionsFunction(
            fastdelegate::MakeDelegate(this, &News::feedChooserOptions));

        feedViewer.colChooser.setShowNumbers(false,0);
        feedViewer.colChooser.setShowNumbers(true,1);
        feedViewer.colChooser.setDirectionUp();

        mutexFeeds      = mrutils::mutexCreate();
        mutexActiveFeed = mrutils::mutexCreate();

        feedChooser.termLine.assignFunction('i'
            ,fastdelegate::MakeDelegate(this,
                &News::feedInfo));

        feedChooser.termLine.assignFunction('y'
            ,fastdelegate::MakeDelegate(this,
                &News::historyShow));

        feedChooser.termLine.assignFunction('\''
            ,fastdelegate::MakeDelegate(this,
                &News::openBrowser));

        feedChooser.termLine.assignFunction('c'
            ,fastdelegate::MakeDelegate(this,
                &News::feedCategory));

        feedCatChooser.freeze();
        feedViewer.freeze();

        init();
    }

    ~News() {
        if (mainThreadSql != NULL)
            delete mainThreadSql;
    }

public: // main methods
    void run();
    void close();
    bool feedInfo();
    bool feedCategory();
    bool historyShow();
    bool openBrowser();

    inline Feed * getFeedById(int feedId) const
    {
        mrutils::map<int, Feed*>::iterator it
                = feedMap.find(feedId);
        if (it == feedMap.tail)
            return NULL;
        return (*it)->second;
    }

    void updateThread(void*);
    void clientThread(void*);
    void serverThread(void*);
    bool serverAddOfflineVisits(mrutils::Sql& sql);
    bool serverDumpContent(char * pathBuffer);
    bool serverDumpItemsAndContent(mrutils::Sql& sql);

    void serverDumpItemsAndContentFinish(mrutils::BufferedWriter&,
            mrutils::Database::Row const &row)
            throw (mrutils::ExceptionNoSuchData);

    bool clientDumpItemsAndContentFinish(mrutils::BufferedReader&,
            mrutils::Database::Row const &row)
            throw (mrutils::ExceptionNoSuchData);

    bool clientDumpLastReadItemIds(mrutils::Sql&, std::vector<Feed*>* affectedFeeds, char * buffer, int bufferSize);

    void setItemsArchivedFlag(mrutils::Sql& sql) throw(mrutils::ExceptionNoSuchData);

    inline mode_t getMode() const
    { return mode; }

public: // pop functions & keys
    void feedChooserOptions(mrutils::ColChooser&c, void*);

private:
    void init();
    static bool setupTables(mrutils::Sql& sql);
    bool initClient();
    void initCategories();
    void initFeeds();
    void updateFeedLineInChooser(Feed * feed);

public:
    mrutils::GuiSelect feedChooser;
    mrutils::GuiViewer feedViewer;

private:
    mode_t mode;
    mrutils::SelectInterrupt stopFD;

    mrutils::GuiSelect feedCatChooser;

    std::vector<Category*> categories;
    volatile bool exit_;
    std::vector<mrutils::thread_t> threads;

    mrutils::mutex_t mutexFeeds;

    char queryBuffer[1024];
    char scratchBuffer[262144];

    mrutils::Sql* mainThreadSql;

    Feed* activeFeed;
    mrutils::mutex_t mutexActiveFeed;

    mrutils::BufferedReader reader;
    mrutils::BufferedWriter writer;
    mrutils::map<int,Feed*> feedMap;
    mrutils::map<int,Category*> categoryMap;

    bool online;
};

}
