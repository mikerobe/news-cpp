#ifndef _MR_NEWS_HISTORY_H
#define _MR_NEWS_HISTORY_H

#include "mr_guiviewer.h"
#include "mr_sql.h"
#include "mr_time.h"

namespace news {

class Item;
class Visit;
class News;

class History {
    public:
        static History * singleton;

        History(News &news, mrutils::Sql& sql) :
                lastVisitId(-1), viewer(2,15),
                news(news),
                lastItemId(-1),
                mutex(mrutils::mutexCreate()),
                useOffline(false)
        {
            viewer.colChooser.setShowNumbers(false);
            viewer.colChooser.setDirectionUp();

            viewer.colChooser.setOptionsFunction(
                fastdelegate::MakeDelegate(this
                ,&History::viewChooserOptions));
            viewer.colChooser.setSelectionFunction(
                fastdelegate::MakeDelegate(this
                ,&History::viewSelection), (void*)&viewer.display);

            viewer.termLine.assignFunction('\''
                ,fastdelegate::MakeDelegate(this,
                    &History::openBrowser));

            viewer.freeze();

            init(sql);
        }

        inline void setUseOffline(bool tf = true)
        { useOffline = tf; }

        inline void setSourceId(const char * id)
        { this->sourceId = id; }

        void add(mrutils::Sql& sql, Item* item);

       /**
        * Criterion of the form IN (id,id,id)
        */
        void updateFromDB(mrutils::Sql& sql);

        /**
         * opens the history view
         */
        void show();

    public:
       /****************
        * Locally-triggered functions
        ****************/

        bool openBrowser();

    public:
       /****************
        * Populators
        ****************/

        void viewChooserOptions(mrutils::ColChooser&c, void*);
        void viewSelection(mrutils::ColChooser&c, void*);

    public:
        int lastVisitId;

    private:
        void init(mrutils::Sql& sql);

    private:
        mrutils::GuiViewer viewer;
        News &news;

        char queryBuffer[1024];
        std::vector<int> dates;

        typedef std::vector<std::vector<Visit*>*> dateVisitsT;
        dateVisitsT dateVisits; // date number (i.e., 0, 1, 2, 3) to items

        int lastItemId;

        mrutils::mutex_t mutex;
        bool useOffline;
        std::string sourceId;
};
}

#endif
