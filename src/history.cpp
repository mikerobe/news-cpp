#include "history.h"
#include "item.h"
#include "feed.h"
#include "visit.h"
#include "news.h"
#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
	#include <pantheios/inserters.hpp>
#endif

news::History * news::History::singleton = NULL;

void news::History::viewChooserOptions(mrutils::ColChooser&c, void*) {
    std::vector<Visit*> visits = *dateVisits[c.selected[0]];

    if (c.selected[0] == 0) // All dates
    {
        for (std::vector<Visit*>::iterator it = visits.begin();
                it != visits.end(); ++it)
            c.next() << (*it)->dateName;
    } else // specific date
    {
        for (std::vector<Visit*>::iterator it = visits.begin();
                it != visits.end(); ++it)
            c.next() << (*it)->name;
    }
}

bool news::History::openBrowser() {
    if (viewer.colChooser.selected[1] < 0) return true;

    mrutils::ColChooser &c = viewer.colChooser;
    mrutils::TermLine &t = viewer.termLine;

    Visit *visit = dateVisits[c.selected[0]]->at(c.selected[1]);

    char buffer[256];

    if (visit->openBrowser()) {
        sprintf(buffer
            ,"%3d: opened in browser %s"
            ,c.selected[1], visit->link.c_str());
    } else {
        sprintf(buffer
            ,"%3d: error opening in browser %s"
            ,c.selected[1], visit->link.c_str());
    }

    t.setStatus(buffer);

    return true;
}

void news::History::viewSelection(mrutils::ColChooser&c, void* d_) {
    mrutils::Display &d = *(mrutils::Display*)d_;
    if (c.selected[1] < 0) return;

    Visit * visit = dateVisits[c.selected[0]]->at(c.selected[1]);

    d.clear();
        d.header()  << "Feed: " << visit->feedTitle;
        d.header()  << "Item: " << visit->itemTitle;
        d.header()  << visit->link;
        d.content() << visit->description;
    d.update();
}

void news::History::show() {
    std::string oldTitle = mrutils::gui::termTitle;
    mrutils::gui::setTermTitle("History");

    viewer.thaw();

    for(;;) {

        mrutils::ColChooser &c = viewer.colChooser;
        mrutils::TermLine &t = viewer.termLine;

        int chosen = viewer.get();
        if (chosen < 0) {
            if (t.line[0] == 0) break;
            if (t.line[0] != '>') continue;

            switch (t.line[1]) {
            }
        } else {
            Visit* visit = dateVisits[c.selected[0]]->at(c.selected[1]);
            viewer.freeze();
            if (!visit->displayContent()) {
                sprintf(queryBuffer
                    ,"%3d: Unable to get content at %s"
                    ,c.selected[1],visit->link.c_str());
                t.setStatus(queryBuffer);
            } else {
                mrutils::gui::forceClearScreen();
            }
            viewer.thaw();
        }
    }

    viewer.freeze();

    mrutils::gui::setTermTitle(oldTitle.c_str());
}


void news::History::add(mrutils::Sql& sql, Item * item)
{
    mrutils::mutexAcquire(mutex);

        if (item->id != lastItemId) {
            lastItemId = item->id;

            time_t now; time(&now);

            Visit * v = new Visit();
            v->date = mrutils::getDate();
            v->hour = mrutils::getHoursLocal(now);
            v->itemId = item->id;
            v->feedId = item->feed.id;
            v->itemTitle = item->title;
            v->feedTitle = item->feed.title;
            v->link = item->link;
            v->setName(queryBuffer, sizeof(queryBuffer));
            v->description = item->description;
            v->itemDate = item->date;

            if (useOffline) {
                sprintf(queryBuffer
                    ,"INSERT INTO offline \
                      (source,date, hour, itemId, feedId \
                      ,feedTitle, itemTitle, link, summary,itemDate) \
                      SELECT \
                      \"%s\",%d, %f, %d, %d, \"%s\", \"%s\", \"%s\" \
                      ,summary,`date` FROM items WHERE itemId=%d"
                    ,sourceId.c_str()
                    ,v->date, v->hour
                    ,v->itemId, v->feedId
                    ,v->feedTitle.c_str()
                    ,v->itemTitle.c_str()
                    ,v->link.c_str()
                    ,v->itemId
                    );
            } else {
                sprintf(queryBuffer
                    ,"INSERT INTO visits \
                      (date, hour, itemId, feedId \
                      ,feedTitle, itemTitle, link, summary,source,sourceid,itemDate) \
                      SELECT \
                      %d, %f, %d, %d, \"%s\", \"%s\", \"%s\" \
                      ,summary,\"%s\",%d,`date` FROM items WHERE itemId=%d"
                    ,v->date, v->hour
                    ,v->itemId, v->feedId
                    ,v->feedTitle.c_str()
                    ,v->itemTitle.c_str()
                    ,v->link.c_str()
                    ,"local"
                    ,v->itemId
                    ,v->itemId
                    );
            }

            int visitId = -1;

            if (!sql.insert(queryBuffer,&visitId))
            {
                mrutils::stringstream ss;
                ss << "Unable to execute query: " << queryBuffer;
                viewer.termLine.setStatus(ss.str().c_str());
            }

            if (!useOffline)
            {
                if (visitId > lastVisitId)
                    lastVisitId = visitId;

                if (dates.empty() || dates.back() != v->date)
                {
                    dates.push_back(v->date);
                    std::vector<Visit*> *vec = new std::vector<Visit*>();
                    vec->push_back(v);
                    dateVisits.push_back(vec);

                    mrutils::strcpy(queryBuffer, v->date);
                    viewer.colChooser.append(queryBuffer,0);
                } else
                {
                    dateVisits.back()->push_back(v);
                }

                dateVisits[0]->push_back(v);

                if (viewer.colChooser.selected[0] == (int)dateVisits.size()-1)
                {
                    viewer.colChooser.append(v->name.c_str(), 1, false);
                } else if (viewer.colChooser.selected[0] == 0)
                {
                    viewer.colChooser.append(v->dateName.c_str(), 1, false);
                }
            }
        }

    mrutils::mutexRelease(mutex);
}

void news::History::updateFromDB(mrutils::Sql& sql) {
    mrutils::mutexScopeAcquire lock(mutex);

    std::vector<int> newItemIds; /**< itemids for added visits */

    sprintf(queryBuffer
        ,"SELECT visitId, date, hour, itemId, feedId \
          ,feedTitle, itemTitle, link, summary, itemDate FROM \
          visits WHERE visitId > %d ORDER BY date, hour, visitId"
        ,lastVisitId);

    if (!sql.rr(queryBuffer))
        return;

    while (sql.nextLine())
    {
        Visit * visit = new Visit();
        visit->date = sql.getInt(1);
        visit->hour = sql.getDouble(2);
        visit->itemId = sql.getInt(3);
        visit->feedId = sql.getInt(4);
        visit->feedTitle = sql.getString(5);
        visit->itemTitle = sql.getString(6);
        visit->link = sql.getString(7);
        visit->description = sql.getString(8);
        visit->itemDate = sql.getInt(9);
        visit->setName(queryBuffer, sizeof(queryBuffer));

        newItemIds.push_back(visit->itemId);

        const int visitId = sql.getInt(0);
        if (visitId > lastVisitId)
            lastVisitId = visitId;

        std::vector<int>::iterator itDate = std::lower_bound(dates.begin(),
            dates.end(), visit->date);
        if (itDate == dates.end()) // add to end
        {
            mrutils::mutexAcquire(viewer.colChooser.mutex);
                std::vector<Visit*> *vec = new std::vector<Visit*>();
                vec->push_back(visit);
                dateVisits.push_back(vec);
                dates.push_back(visit->date);
                mrutils::strcpy(queryBuffer, visit->date);
                viewer.colChooser.append(queryBuffer,0,false);
            mrutils::mutexRelease(viewer.colChooser.mutex);
        } else if (*itDate != visit->date) // add at this position
        {
            mrutils::mutexAcquire(viewer.colChooser.mutex);
                int const pos = itDate - dates.begin();
                std::vector<Visit*> *vec = new std::vector<Visit*>();
                vec->push_back(visit);
                dateVisits.insert(dateVisits.begin() + pos, vec);
                dates.insert(itDate, visit->date);
                mrutils::strcpy(queryBuffer, visit->date);
                viewer.colChooser.insert(pos,queryBuffer,0,false);
            mrutils::mutexRelease(viewer.colChooser.mutex);
        } else // found same date
        {
            // TODO may not have the right time stamp position
            mrutils::mutexAcquire(viewer.colChooser.mutex);
                int const pos = itDate - dates.begin();
                dateVisits[pos]->push_back(visit);
                if (viewer.colChooser.selected[0] == pos)
                {
                    viewer.colChooser.append(visit->name.c_str(), 1, false);
                }
            mrutils::mutexRelease(viewer.colChooser.mutex);
        }

        // TODO: adding to All category, just pushing back, not in
        // right chronological position
        // add to All
        mrutils::mutexAcquire(viewer.colChooser.mutex);
            dateVisits[0]->push_back(visit);
            if (viewer.colChooser.selected[0] == 0)
            {
                viewer.colChooser.append(visit->dateName.c_str(), 1, false);
            }
        mrutils::mutexRelease(viewer.colChooser.mutex);

        if (Feed *feed = news.getFeedById(visit->feedId))
        {
            if (Item *item = feed->getItemById(visit->itemId))
            {
                item->marked = true;
            }
        }

    }

    std::stringstream ss;
    for (std::vector<int>::const_iterator itItemId = newItemIds.begin();
            itItemId != newItemIds.end(); ++itItemId)
    {
        ss.str("");
        ss << "UPDATE items SET marked=1 WHERE itemid=" << *itItemId;
        if (!sql.run(ss.str().c_str()))
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," sql error [",sql.error,"]");
			#endif
            return;
        }
    }
}

void news::History::init(mrutils::Sql& sql)
{
    // add All
    dateVisits.push_back(new std::vector<Visit*>());
    dates.push_back(-1);
    viewer.colChooser.next() << "All";

    mrutils::strcpy(queryBuffer
        ,"SELECT visitId, `date`, hour, itemId, feedId \
          ,feedTitle, itemTitle, link, summary, itemDate FROM \
          visits ORDER BY date,hour,visitId");

    if (!sql.rr(queryBuffer))
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::logprintf(pantheios::error,
                "%s %s:%d sql error [%s] [%s]",
                __PRETTY_FUNCTION__,__FILE__,__LINE__,
                sql.error,queryBuffer);
		#endif
        return;
    }

    while (sql.nextLine()) {
        Visit * visit = new Visit();
        visit->date = sql.getInt(1);
        visit->hour = sql.getDouble(2);
        visit->itemId = sql.getInt(3);
        visit->feedId = sql.getInt(4);
        visit->feedTitle = sql.getString(5);
        visit->itemTitle = sql.getString(6);
        visit->link = sql.getString(7);
        visit->description = sql.getString(8);
        visit->itemDate = sql.getInt(9);
        visit->setName(queryBuffer, sizeof(queryBuffer));

        const int visitId = sql.getInt(0);
        if (visitId > lastVisitId) lastVisitId = visitId;

        if (dates.empty() || dates.back() != visit->date) {
            dates.push_back(visit->date);
            std::vector<Visit*> *v = new std::vector<Visit*>();
            v->push_back( visit );
            dateVisits.push_back(v);
            viewer.colChooser.next() << visit->date;
            dateVisits[0]->push_back(visit);
        } else {
            dateVisits[0]->push_back(visit);
            dateVisits.back()->push_back(visit);
        }
    }

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__," last seen visit id: [",
            pantheios::integer(lastVisitId),"]");
	#endif
}
