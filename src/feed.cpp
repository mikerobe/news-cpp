#include "feed.h"
#include "html.h"
#include "news.h"
#include "category.h"
#include "mr_prog.h"
#include "url.h"
#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
	#include <pantheios/inserters.hpp>
#endif

const char * news::Feed::statusName[] = {
    "OK",
    "Error",
    "Unread"
};

const char * news::Feed::formatName[] = {
    "XML",
    "HTML"
};

const int news::Feed::statusATR[] = {
    mrutils::gui::ATR_BLANK,
    mrutils::gui::ATR_ERROR,
    mrutils::gui::ATR_NEW
};

const int news::Feed::statusCOL[] = {
    mrutils::gui::COL_BLANK,
    mrutils::gui::COL_ERROR,
    mrutils::gui::COL_NEW
};

news::Category* news::Feed::allCategory = NULL;

news::Feed::~Feed() {
    if (!isAll && !dateItems.empty()) {
        // date items 0 contains all items
        std::vector<Item*> &items = *dateItems[0];
        for (std::vector<Item*>::iterator iit = items.begin();
                iit != items.end(); ++iit) delete *iit;
    }
}

void news::Feed::initUnread(mrutils::Sql& sql, char * queryBuffer) {
    sprintf(queryBuffer,"SELECT COUNT(*) FROM items WHERE feedId=%d AND itemId>%d",id,lastReadItemId);
    this->numUnread = 0;
    (void)sql.get(queryBuffer,numUnread);
    initUnread(numUnread, queryBuffer);
}

void news::Feed::initUnread(int numUnread, char *queryBuffer)
{
    this->numUnread = numUnread;
    if (numUnread > 0) {
        status = FS_UNREAD;
        sprintf(queryBuffer,"%s (%d)",title.c_str(),numUnread);
        name.assign(queryBuffer);
    } else {
        status = FS_OK;
        name = title;
    }
}

void news::Feed::viewChooserOptions(mrutils::ColChooser&c, void*) {
    mrutils::mutexAcquire(mutexData);
        std::vector<Item*> &items = *dateItems[c.selected[0]];
        int id = 0;
        for (std::vector<Item*>::iterator it = items.begin();
             it != items.end(); ++it) {
            c.next() << (*it)->title;
            if ( (*it)->isGone ) {
                c.setColor(id
                    ,mrutils::gui::ATR_XB
                    ,mrutils::gui::COL_XB
                    ,1, false);
            } else if ( (*it)->marked ) {
                c.setTarget(id,true,false);
            }
            ++id;
        }
    mrutils::mutexRelease(mutexData);

} 

/**
 * Searches full text for the currently active feed
 */
bool news::Feed::searchMatch(mrutils::ColChooser&c, int id, const char * search, void*) {
    if (c.activeCol != 1) return true;
    bool ret = true;

    mrutils::mutexAcquire(mutexData);

        if (c.selected[0] >= 0 && id >= 0) {
            Item * item = dateItems[c.selected[0]]->at(id);
            ret = NULL != mrutils::stristr(item->description.c_str(),search)
                || NULL != mrutils::stristr(item->title.c_str(),search)
                || NULL != mrutils::stristr(item->link.c_str(),search)
                ;
        }

    mrutils::mutexRelease(mutexData);
    return ret;
}

void news::Feed::viewSelection(mrutils::ColChooser&c, void* d_) {
    mrutils::Display &d = *(mrutils::Display*)d_;

    mrutils::mutexAcquire(mutexData);
        if (c.selected[1] >= 0) {
            if (c.selected[0] >= (int)dateItems.size() 
                || c.selected[1] >= (int)dateItems[c.selected[0]]->size()
                ) {
                d.clear();
                d.content() << "selected[0]: " << c.selected[0] << "\n";
                d.content() << "selected[1]: " << c.selected[1] << "\n";
                d.content() << "size: " << (int)dateItems.size() << "\n";
                if (c.selected[0] < (int)dateItems.size()) {
                    d.content() << "subsize: " << (int)dateItems[c.selected[0]]->size() << "\n";
                }
                d.update();
            } else {
                Item * item = dateItems[c.selected[0]]->at(c.selected[1]);

                d.clear();
                    if (&item->feed != this) {
                        d.header() << "Feed: " << item->feed.title;
                    }
                    d.header()  << item->title;
                    if (c.selected[0] == 0) {
                        d.header() << "Published " << item->date;
                    }
                    d.header()  << item->link;
                    d.content() << item->description;
                d.update();
            }
        }
    mrutils::mutexRelease(mutexData);
}

bool news::Feed::refresh() {
    char buffer[262144];
    mrutils::XML xml, xmlContent;

    mrutils::mutex_t mutex = mrutils::mutexCreate();
    mrutils::mutex_t mutexFeeds = mrutils::mutexCreate();

    Feed * f = this;

    update(*_sql, xml, xmlContent, -1, NULL, buffer, sizeof(buffer)
        ,*_viewer, f, mutex, mutexFeeds);

    mrutils::mutexDelete(mutex);
    mrutils::mutexDelete(mutexFeeds);

    return true;
}

void news::Feed::clearItems(mrutils::Sql &sql, char *buffer, int /*size*/)
{
    if (isAll)
        return;

    enum
    {
        CLEAR_DELETE,
        CLEAR_ARCHIVE
    } const deleteMode =
            (news->getMode() == news::MD_CLIENT
                ? CLEAR_DELETE
                : CLEAR_ARCHIVE);

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__, " entered for feed [",title,"] "
            "delete mode: [",
                deleteMode == CLEAR_DELETE ? "delete"
                : "archive","]");
	#endif

    int remove = (deleteMode == CLEAR_DELETE ? numItems : numUnarchived)
            - maxItems;

    for (dateItemsT::iterator it = dateItems.begin()+1;
         it != dateItems.end() && remove > 0; )
    {
        int r = remove;

        for (std::vector<Item*>::iterator iit = (*it)->begin();
                 iit != (*it)->end() && r > 0; ++iit)
        {
            if (deleteMode == CLEAR_DELETE)
            {
                if (!(*iit)->isGone)
                {
                    (*iit)->clearContent(buffer);
                    sprintf(buffer,
                            "DELETE FROM items WHERE itemId=%d",
                            (*iit)->id);
                    sql.run(buffer);
                    (*iit)->isGone = true;
                    //delete *iit;
                    --r;
                }
            } else
            {
                if (!(*iit)->isArchived)
                {
                    sprintf(buffer,
                            "UPDATE items set archived=1 WHERE itemId=%d",
                            (*iit)->id);
                    sql.run(buffer);
                    (*iit)->isArchived = true;
                    --r;
                }
            }
        }

        if (deleteMode == CLEAR_DELETE)
        {
            if (remove >= (int)(*it)->size()) {
                it = dateItems.erase( it );
                dates.erase( dates.begin() );
            } else {
                (*it)->erase( (*it)->begin(), (*it)->begin() + (remove-r) );
                ++it;
            }
            numItems -= (remove - r);
        } else
        {
            ++it;
            numUnarchived -= (remove - r);
        }

        remove = r;
    }

    if (deleteMode == CLEAR_ARCHIVE)
    {
        sprintf(buffer,"UPDATE feeds SET firstItemId=(SELECT min(itemId) from "
                "items where feedid=%d and archived=0) WHERE feedId=%d",
                id,id);
        sql.run(buffer);
    }
}

bool news::Feed::openBrowser() {
    if (_viewer->colChooser.selected[1] < 0) return true;

    mrutils::ColChooser &c = _viewer->colChooser;
    //mrutils::TermLine &t = _viewer->termLine;

    mrutils::mutexAcquire(mutexData);
    Item *item = dateItems[c.selected[0]]->at(c.selected[1]);
    mrutils::mutexRelease(mutexData);

    c.setTarget(c.selected[1],true);
    itemTargeted(item,true);

    char buffer[256];

    if (item->openBrowser()) {
        sprintf(buffer
            ,"%3d: opened in browser %s"
            ,c.selected[1], item->link.c_str());

        // add to history
        History::singleton->add(*_sql, item);
    } else {
        sprintf(buffer
            ,"%3d: error opening in browser %s"
            ,c.selected[1], item->link.c_str());
    }

    _viewer->termLine.setStatus(buffer);

    return true;
}

bool news::Feed::historyShow() {
    _viewer->freeze();
    History::singleton->show();
    _viewer->thaw();

    return true;
}

void news::Feed::itemTargeted(Item * item, bool tf)
{
    item->marked = tf;
    _sql->run(mrutils::stringstream().clear()
        << "UPDATE items SET marked=" << tf << " WHERE itemId="
        << item->id);
}

void news::Feed::itemTargetedGui(mrutils::ColChooser & c, int id, bool tf, void*) {
    mrutils::mutexAcquire(mutexData);
    Item *item = dateItems[c.selected[0]]->at(id);
    mrutils::mutexRelease(mutexData);
    itemTargeted(item,tf);
}

void news::Feed::guiEnter(mrutils::Sql& sql, mrutils::GuiViewer& viewer,
    mrutils::XML& xml, mrutils::XML &xmlSecondary, char * buffer, int size) {

    std::string oldTitle = mrutils::gui::termTitle;
    if (!isAll) mrutils::gui::setTermTitle(title.c_str());

    viewer.colChooser.setOptionsFunction(
            fastdelegate::MakeDelegate(this
            ,&Feed::viewChooserOptions));
    viewer.colChooser.setSelectionFunction(
            fastdelegate::MakeDelegate(this
            ,&Feed::viewSelection), (void*)&viewer.display);

    viewer.colChooser.setSearchFunction(fastdelegate::MakeDelegate(this
        ,&Feed::searchMatch));

    viewer.termLine.assignFunction('r'
        ,fastdelegate::MakeDelegate(this,
            &Feed::refresh));
    viewer.termLine.assignFunction('\''
        ,fastdelegate::MakeDelegate(this,
            &Feed::openBrowser));
    viewer.termLine.assignFunction('y'
        ,fastdelegate::MakeDelegate(this,
            &Feed::historyShow));
	/* TODO
    viewer.termLine.assignFunction('.'
        ,fastdelegate::MakeDelegate(this,
            &Feed::regetItem));
	*/

    viewer.colChooser.setTargetFunction(
        fastdelegate::MakeDelegate(this,
            &Feed::itemTargetedGui));

    // for refresh, need local _viewer
    this->_viewer = &viewer;
    this->_sql = &sql;

    Feed *activeFeed = this;

    // init all feeds in this category
    if (isAll) {
        viewer.freeze();
        for (Feed** it = (*categories.begin())->feeds.begin();
             it != (*categories.begin())->feeds.end(); ++it) {
            if ((*it) == this) continue;
            mrutils::mutexAcquire( (*it)->mutexInit );
            if (!(*it)->init_) (*it)->init(sql,buffer,size, &viewer, activeFeed);
            mrutils::mutexRelease( (*it)->mutexInit );
        }
        viewer.thaw();
    }

    mrutils::mutexAcquire(mutexInit);
        if (!init_) init(sql,buffer,size, &viewer, activeFeed);

        // mutex lock below is OK
        viewer.clear();
        viewer.termLine.setStatus("");

        // zero date item
        viewer.colChooser.next() << "Show All";

        for (std::vector<int>::iterator it = dates.begin()+1;
             it != dates.end(); ++it) {
            viewer.colChooser.next() << *it;
        }

        if (status == Feed::FS_UNREAD) 
            status = Feed::FS_OK;
        if (!isAll && numUnread > 0) {
            numUnread = 0;
            this->name = this->title;
            sprintf(buffer
                ,"UPDATE feeds SET lastReadItemId=(SELECT MAX(itemId) FROM items) WHERE feedId=%d"
                ,this->id);
            sql.run(buffer);
        }

        if (isAll) {
            for (Feed** it = (*categories.begin())->feeds.begin();
                 it != (*categories.begin())->feeds.end(); ++it) {
                if ((*it) == this) continue;
                mrutils::mutexAcquire( (*it)->mutexInit );
                    (*it)->numUnread = 0;
                    (*it)->name = (*it)->title;
                    if ( (*it)->status == Feed::FS_UNREAD) (*it)->status = Feed::FS_OK;

                    sprintf(buffer
                        ,"UPDATE feeds SET lastReadItemId=(SELECT MAX(itemId) FROM items) WHERE feedId=%d"
                        ,(*it)->id);
                    sql.run(buffer);
                mrutils::mutexRelease( (*it)->mutexInit );
            }
        }

        viewer.show();
        viewer.colChooser.nextCol();
    mrutils::mutexRelease(mutexInit);

    for(;;) {
        int chosen = viewer.get();
        mrutils::ColChooser &c = viewer.colChooser;
        mrutils::TermLine &t = viewer.termLine;

        if (chosen < 0) {
            if (viewer.termLine.line[0] == 0) break;
            if (viewer.termLine.line[0] != '>') continue;

            switch (t.line[1]) {
                case 'e': // debug
                {
                    mrutils::stringstream ss;
                    ss << "chose: " << c.selected[0] << ", " << c.selected[1];
                    ss << " activeCol: " << c.activeCol;
                    t.setStatus(ss.str().c_str());
                } break;

                case 'c': // refetch content
                {
                    char * p = strchr(t.line,' ');
                    if (p == NULL) continue; ++p;
                    if (c.selected[1] < 0) continue;
                    mrutils::mutexAcquire(mutexData);
                    Item *item = dateItems[c.selected[0]]->at(c.selected[1]);
                    mrutils::mutexRelease(mutexData);

                    if (0==strcmp(p,"reget")) {
                        item->clearContent(buffer);
                        if (item->getContent(xml, xmlSecondary, -1, buffer, size)) {
                            sprintf(buffer
                                ,"%3d: Retrieved content at %s"
                                ,c.selected[1],item->link.c_str());
                            t.setStatus(buffer);
                        } else {
                            sprintf(buffer
                                ,"%3d: Still unable to get content at %s"
                                ,c.selected[1],item->link.c_str());
                            t.setStatus(buffer);
                        }
                    }

                } break;
            }
        } else {
            viewer.freeze();

            mrutils::mutexAcquire(mutexData);
            Item *item = dateItems[c.selected[0]]->at(c.selected[1]);
            mrutils::mutexRelease(mutexData);

            itemTargeted(item,true);
            c.setTarget(c.selected[1],true);

            // TODO can add a search term to the viewer (based on
            // content search)
            //  viewer.colChooser.cols[1].searchFnTerms.empty() ? "" : viewer.colChooser.cols[1].searchFnTerms[0].c_str()
            if (!item->displayContent(sql,""))
            {
                sprintf(buffer
                    ,"%3d: Unable to get content at %s"
                    ,c.selected[1],item->link.c_str());
                t.setStatus(buffer);
            } else
            {
                mrutils::gui::forceClearScreen();
            }

            // elinks clears the title
            if (!isAll) mrutils::gui::setTermTitle(title.c_str());
            else mrutils::gui::setTermTitle(oldTitle.c_str());

            viewer.thaw();
        }
    }

    if (!isAll) mrutils::gui::setTermTitle(oldTitle.c_str());

    if (!isAll) {
        sprintf(buffer
            ,"UPDATE feeds SET lastReadItemId=(SELECT MAX(itemId) FROM items) WHERE feedId=%d"
            ,this->id);
        sql.run(buffer);
    }
}

void news::Feed::init(mrutils::Sql& sql, char * buffer, _UNUSED int size, mrutils::GuiViewer* viewer, Feed*& activeFeed) {
    if (init_) return;

    sprintf(buffer
        ,"SELECT itemId, date, title, link, summary, marked, archived FROM \
          items WHERE feedId=%d ORDER BY itemId"
        ,this->id);

    mrutils::mutex_t mutex = mrutils::mutexCreate();

    for (sql.rr(buffer); sql.nextLine();)
    {
        Item * item = new Item(*this);
        item->id = sql.getInt(0);
        item->date = sql.getInt(1);
        item->title = sql.getString(2);
        item->link = sql.getString(3);
        item->description = sql.getString(4);
        item->marked = sql.getInt(5);
        item->isArchived = sql.getInt(6);

        itemsById[item->id] = item;

        dateItems[0]->push_back(item);

        ++numItems;
        if (!item->isArchived)
            ++numUnarchived;

        if (dates.empty() || dates.back() != item->date) {
            dates.push_back(item->date);
            std::vector<Item*> *v = new std::vector<Item*>();
            v->push_back( item );
            dateItems.push_back(v);
        } else {
            dateItems.back()->push_back( item );
        }

        allCategory->feeds[0]->add(item, viewer, activeFeed, mutex);

        for (Category ** it = categories.begin(); it != categories.end(); ++it) {
            (*it)->feeds[0]->add(item,viewer,activeFeed,mutex);
        }
    }

    mrutils::mutexDelete(mutex);

    init_ = true;
}

void news::Feed::allAddFeed(Feed* feed) {
    mrutils::mutexAcquire(mutexInit);
    mrutils::mutexAcquire(mutexData);

    for (dateItemsT::iterator it = feed->dateItems.begin(); 
         it != feed->dateItems.end(); ++it) {
        std::vector<Item*> &items = **it;
        for (std::vector<Item*>::iterator iit = items.begin();
             iit != items.end(); ++iit) {
            Item* item = *iit;

            std::vector<int>::iterator dit = std::lower_bound(
                dates.begin(), dates.end(), item->date);
            int pos = dit - dates.begin();
            std::vector<Item*> *vec;

            dateItems[0]->push_back(item);

            if (dit == dates.end() || *dit != item->date) {
                vec = new std::vector<Item*>();
                dateItems.insert( dateItems.begin() + pos, vec );
                dates.insert( dit, item->date );
            } else vec = dateItems[pos];
            vec->push_back(item);
        }
    }

    mrutils::mutexRelease(mutexData);
    mrutils::mutexRelease(mutexInit);
}

void news::Feed::allDeleteFeed(Feed* feed, mrutils::GuiViewer* viewer, Feed** activeFeed, mrutils::mutex_t* mutexActiveFeed) {
    mrutils::mutexAcquire(mutexInit);
    mrutils::mutexAcquire(mutexData);

    for (dateItemsT::iterator it = dateItems.begin(); 
         it != dateItems.end(); ) {
        std::vector<Item*> &items = **it;
        for (std::vector<Item*>::iterator iit = items.begin();
             iit != items.end(); ) {
            Item* item = *iit;

            if (&item->feed == feed) {
                if (viewer != NULL) {
                    mrutils::mutexAcquire(*mutexActiveFeed);
                        if (*activeFeed == this) {
                            mrutils::mutexRelease(mutexData);
                            mrutils::mutexAcquire(viewer->colChooser.mutex);
                            mrutils::mutexAcquire(mutexData);

                            viewer->colChooser.remove(iit - items.begin(),1, false);

                            mrutils::mutexRelease(viewer->colChooser.mutex);
                        }
                    mrutils::mutexRelease(*mutexActiveFeed);
                } 

                iit = items.erase( iit );
            } else ++iit;
        }

        // don't erase the 0 date
        if ( items.empty() && it != dateItems.begin() ) {
            if (viewer != NULL) {
                mrutils::mutexAcquire(*mutexActiveFeed);
                    if (*activeFeed == this) {
                        mrutils::mutexRelease(mutexData);
                        mrutils::mutexAcquire(viewer->colChooser.mutex);
                        mrutils::mutexAcquire(mutexData);

                        viewer->colChooser.remove(it - dateItems.begin(),0, false);

                        mrutils::mutexRelease(viewer->colChooser.mutex);
                    } 
                mrutils::mutexRelease(*mutexActiveFeed);
            }
            dates.erase( dates.begin() + (it - dateItems.begin()) );
            it = dateItems.erase( it );
        } else ++it;
    }

    mrutils::mutexRelease(mutexData);
    mrutils::mutexRelease(mutexInit);
}

void news::Feed::deleteFromAll(mrutils::GuiViewer* viewer, Feed*& activeFeed, mrutils::mutex_t& mutexActiveFeed) {
    mrutils::mutexAcquire(mutexInit);
    mrutils::mutexAcquire(mutexData);

    allCategory->feeds[0]->allDeleteFeed(this, viewer, &activeFeed, &mutexActiveFeed);
    for (Category ** it = categories.begin(); it != categories.end(); ++it) {
        (*it)->feeds[0]->allDeleteFeed(this, viewer, &activeFeed, &mutexActiveFeed);
    }

    mrutils::mutexRelease(mutexData);
    mrutils::mutexRelease(mutexInit);
}

bool news::Feed::add(Item* item, mrutils::GuiViewer* viewer, Feed*& activeFeed, mrutils::mutex_t& mutexActiveFeed) {
    char dateBuffer[16];
    bool activeIsThis = false;

    // add item to data
    mrutils::mutexAcquire(mutexInit);
    mrutils::mutexAcquire(mutexData);
        if (allTitles.find(item->title) == allTitles.end()) {
            allTitles.insert(item->title);
            ++numItems;
            ++numUnarchived;

            // insert into the all date
            if (dateItems[0]->empty() || dateItems[0]->back()->date <= item->date) {
                dateItems[0]->push_back(item);
            } else {
                dateItems[0]->insert(std::lower_bound(dateItems[0]->begin(),dateItems[0]->end(),item,mrutils::ptrCmp<Item>),item);
            }

            std::vector<int>::iterator it = std::lower_bound(
                dates.begin(), dates.end(), item->date);

            int pos = it - dates.begin();

            mrutils::mutexAcquire(mutexActiveFeed);
                activeIsThis = (activeFeed == this);

                if (it == dates.end() || *it != item->date) {
                    std::vector<Item*> *vec = new std::vector<Item*>();

                    if (activeFeed == this) {
                        mrutils::mutexRelease(mutexData);
                        mrutils::mutexAcquire(viewer->colChooser.mutex);
                        mrutils::mutexAcquire(mutexData);
                        dateItems.insert( dateItems.begin() + pos, vec );
                        dates.insert( it, item->date );

                        mrutils::strcpy(dateBuffer, item->date);
                        mrutils::mutexRelease(mutexData);
                        viewer->colChooser.insert(pos,dateBuffer,0, false);
                        mrutils::mutexAcquire(mutexData);

                        vec->push_back(item);

                        mrutils::mutexRelease(viewer->colChooser.mutex);
                    } else {
                        dateItems.insert( dateItems.begin() + pos, vec );
                        dates.insert( it, item->date );
                        vec->push_back(item);
                    }

                } else {
                    dateItems[pos]->push_back(item);
                }

                if (activeFeed == this) {
                    mrutils::mutexRelease(mutexData);
                    mrutils::mutexAcquire(viewer->colChooser.mutex);

                    if (viewer->colChooser.selected[0] == pos
                      ||viewer->colChooser.selected[0] == 0) {
                        viewer->colChooser.append(item->title.c_str(), 1, false);
                    }

                    mrutils::mutexRelease(viewer->colChooser.mutex);
                    mrutils::mutexAcquire(mutexData);
                }
            mrutils::mutexRelease(mutexActiveFeed);
        } else {
            mrutils::mutexAcquire(mutexActiveFeed);
                activeIsThis = (activeFeed == this);
            mrutils::mutexRelease(mutexActiveFeed);
        }

    mrutils::mutexRelease(mutexData);
    mrutils::mutexRelease(mutexInit);

    return !activeIsThis;
}

void news::Feed::updateLastReadItemId(const int lastReadItemId, mrutils::Sql& sql
        ,char * buffer, int size
        ,mrutils::GuiViewer& viewer
        ,Feed*& activeFeed
        ,mrutils::mutex_t& mutexActiveFeed, mrutils::mutex_t& mutexFeeds
    ) {

    if (!mrutils::mutexTryAcquire(mutexUpdate)) return;

    if (!init_) {
        mrutils::mutexAcquire(mutexInit);
        mrutils::mutexAcquire(mutexFeeds); // init uses categories list
            if (!init_) {
                init(sql, buffer,size, &viewer, activeFeed);
                initUnread(sql,buffer);
            }
        mrutils::mutexRelease(mutexFeeds);
        mrutils::mutexRelease(mutexInit);

        sprintf(buffer,"UPDATE feeds SET lastReadItemId=%d WHERE feedId=%d"
            ,lastReadItemId,this->id);
        sql.run(buffer);

        mrutils::mutexRelease(mutexUpdate);
        return;
    }

    if (lastReadItemId < this->lastReadItemId) {
        mrutils::mutexRelease(mutexUpdate);
        return;
    }

    this->lastReadItemId = lastReadItemId;

    sprintf(buffer,"UPDATE feeds SET lastReadItemId=%d WHERE feedId=%d"
        ,lastReadItemId,this->id);
    sql.run(buffer);

    initUnread(sql,buffer);
    updateFinish(sql,0,buffer,size,viewer,activeFeed,mutexActiveFeed);
}

void news::Feed::updateFromDB(const char * itemsCriterion, mrutils::Sql& sql
        ,char * buffer, int size
        ,mrutils::GuiViewer& viewer
        ,Feed*& activeFeed
        ,mrutils::mutex_t& mutexActiveFeed, mrutils::mutex_t& mutexFeeds
    ) {
    if (!mrutils::mutexTryAcquire(mutexUpdate)) return;

    if (!init_) {
        mrutils::mutexAcquire(mutexInit);
        mrutils::mutexAcquire(mutexFeeds); // init uses categories list
            if (!init_) {
                init(sql,buffer,size, &viewer, activeFeed);
                initUnread(sql,buffer);
            }
        mrutils::mutexRelease(mutexFeeds);
        mrutils::mutexRelease(mutexInit);

        mrutils::mutexRelease(mutexUpdate);
        return;
    }

    // items I know about
    std::set<int> itemIds;
    if (!dates.empty()) {
        for (std::vector<Item*>::iterator it = dateItems[dates.size()-1]->begin();
             it != dateItems[dates.size()-1]->end(); ++it) {
            itemIds.insert( (*it)->id );
        }
    }

    // potentially new items
    sprintf(buffer
        ,"SELECT itemId, date, title, link, summary FROM \
          items WHERE feedId=%d AND %s ORDER BY itemId"
        ,this->id,itemsCriterion);

    int addedItems = 0;

    sql.rr(buffer);
    while (sql.nextLine()) {
        if (0!=itemIds.count(sql.getInt(0))) continue;

        Item * item = new Item(*this);
        item->id = sql.getInt(0);
        item->date = sql.getInt(1);
        item->title = sql.getString(2);
        item->link = sql.getString(3);
        item->description = sql.getString(4);

        addItemHelper(item,buffer,size,viewer,activeFeed,mutexActiveFeed,mutexFeeds);
        ++addedItems;
    }

    updateFinish(sql,addedItems,buffer,size,viewer,activeFeed,mutexActiveFeed);
}

void news::Feed::addItemHelper(news::Item * item
    ,char * buffer, _UNUSED int size
    ,mrutils::GuiViewer& viewer, Feed*& activeFeed, mrutils::mutex_t& mutexActiveFeed
    ,mrutils::mutex_t& mutexFeeds) {

    // add item to data
    mrutils::mutexAcquire(mutexInit);
    mrutils::mutexAcquire(mutexData);

        ++numItems;
        ++numUnarchived;
        dateItems[0]->push_back( item );
        mrutils::mutexAcquire(mutexActiveFeed);

            itemsById[item->id] = item;

            if (dates.empty() || dates.back() != (item)->date) {
                dates.push_back( (item)->date );
                std::vector<Item*> *v = new std::vector<Item*>();
                v->push_back( item );
                dateItems.push_back(v);
                if (activeFeed == this) {
                    mrutils::mutexRelease(mutexData);
                    mrutils::strcpy(buffer, (item)->date);
                    viewer.colChooser.append(buffer,0);
                    mrutils::mutexAcquire(mutexData);
                }
            } else {
                dateItems.back()->push_back( item );
                if (activeFeed == this) {
                    mrutils::mutexRelease(mutexData);
                    mrutils::mutexAcquire(viewer.colChooser.mutex);
                    if (viewer.colChooser.selected[0] == (int)dateItems.size()-1
                      ||viewer.colChooser.selected[0] == 0) {
                        viewer.colChooser.append( (item)->title.c_str(), 1, false );
                    }
                    mrutils::mutexRelease(viewer.colChooser.mutex);
                    mrutils::mutexAcquire(mutexData);
                }
            }

            bool isUnread = true;

            // Update "All" feed lists
            mrutils::mutexRelease(mutexActiveFeed);
            mrutils::mutexAcquire(mutexFeeds);
                if (!allCategory->feeds[0]->add(item, &viewer, activeFeed, mutexActiveFeed)) 
                    isUnread = false;
                for (Category** iit = categories.begin(); iit != categories.end(); ++iit) {
                    if (!(*iit)->feeds[0]->add(item,&viewer,activeFeed,mutexActiveFeed))
                        isUnread = false;
                }
            mrutils::mutexRelease(mutexFeeds);
            mrutils::mutexAcquire(mutexActiveFeed);

            if (activeFeed != this && isUnread) {
                ++numUnread; status = Feed::FS_UNREAD;
            }

        mrutils::mutexRelease(mutexActiveFeed);

    mrutils::mutexRelease(mutexData);
    mrutils::mutexRelease(mutexInit);
}

int news::Feed::updateFinish(mrutils::Sql& sql, int addedItems
    ,char * buffer, int size
    ,mrutils::GuiViewer& viewer, Feed*& activeFeed, mrutils::mutex_t& mutexActiveFeed) {
    mrutils::mutexAcquire(mutexActiveFeed);
        if (numUnread > 0) {
            sprintf(buffer, "%s (%d)",title.c_str(), numUnread);
            this->name.assign(buffer);
        } else {
            if (status != Feed::FS_ER) status = Feed::FS_OK;
            this->name.assign(title.c_str());
        }

        // make sure active feed isn't this 
        // && that active feed isn't a category viewer
        // for one of my categories
        bool canDelete = (activeFeed != this && news != NULL);
        if (canDelete && activeFeed != NULL && activeFeed->isAll) {
            Category** it = categories.find(
                *activeFeed->categories.begin());
            canDelete = (it == categories.end()
                || (*activeFeed->categories.begin())->isAll);
        }

        if (canDelete) {
            mrutils::mutexAcquire(mutexInit);
            mrutils::mutexAcquire(mutexData);
            clearItems(sql,buffer,size);
            mrutils::mutexRelease(mutexData);
            mrutils::mutexRelease(mutexInit);
        }
    mrutils::mutexRelease(mutexActiveFeed);

    if (status == Feed::FS_ER) {
        mrutils::mutexRelease(mutexUpdate);
        return -1;
    } else {
        if (addedItems > 0) {
            sprintf(buffer,"Added %d items. total %d. max %d"
                ,addedItems, numItems, maxItems);
            statusLine.assign(buffer);

            mrutils::mutexAcquire(mutexActiveFeed);
            if (activeFeed == this) viewer.termLine.setStatus(statusLine.c_str());
            mrutils::mutexRelease(mutexActiveFeed);
        }

        mrutils::mutexRelease(mutexUpdate);
        return addedItems;
    }
}

int news::Feed::update(mrutils::Sql& sql,
    mrutils::XML& xml,
    mrutils::XML& xmlContent,
    int stopFD,
    volatile bool * exit_,
    char * buffer,
    int size,
    mrutils::GuiViewer& viewer,
    Feed*& activeFeed,
    mrutils::mutex_t& mutexActiveFeed,
    mrutils::mutex_t& mutexFeeds
    )
{
    if (isAll) return 0;

    if (!mrutils::mutexTryAcquire(mutexUpdate)) return 0;

    if (updateSeconds == 0)
    {
        mrutils::mutexRelease(mutexUpdate);
        return 0;
    }

    time_t now; time(&now);
    if (lastUpdate + updateSeconds > (unsigned)now)
    {
        mrutils::stringstream ss;
        std::string str = mrutils::getLocalTimeFromEpoch(now);
        ss << str.c_str() << " Too soon for refresh.";
        mrutils::mutexAcquire(mutexActiveFeed);
            if (activeFeed == this) viewer.termLine.setStatus(ss.str().c_str());
        mrutils::mutexRelease(mutexActiveFeed);
        mrutils::mutexRelease(mutexUpdate);
        return 0;
    }

    news::url::updateFeedUrl(&link, buffer);

    strcpy(buffer,link.c_str());
    mrutils::curl::urlRequest_t request(buffer);
    request.referer = "";

    lastUpdate = now;
    if (0 != xml.get(&request,stopFD))
    {
        statusLine.assign("Unable to get url: ");
        statusLine.append(link);
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__," error [",
                statusLine,"]");
		#endif
        mrutils::mutexRelease(mutexUpdate);
        return -1;
    }

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__," entered for [",title,"]");
	#endif

    // update lastupdate timestamp in database
    sprintf(buffer,"UPDATE feeds SET lastUpdate=%u WHERE feedId=%d",(unsigned)lastUpdate,id);
    sql.run(buffer);

    if (!init_) {
        mrutils::mutexAcquire(mutexInit);
        mrutils::mutexAcquire(mutexFeeds); // init uses categories list
            if (!init_) init(sql,buffer,size, &viewer, activeFeed);
        mrutils::mutexRelease(mutexFeeds);
        mrutils::mutexRelease(mutexInit);
    }

    int lastDate = (dates.empty()?0:dates.back());
    std::set<std::string> titles;

    if (!dates.empty()) {
        if (checkAllTitles) {
            for (dateItemsT::iterator it1 = dateItems.begin(); it1 != dateItems.end(); ++it1) {
                for (std::vector<Item*>::iterator it2 = (*it1)->begin(); it2 != (*it1)->end(); ++it2) {
                    titles.insert( (*it2)->title );
                }
            }
        } else {
            for (std::vector<Item*>::iterator it = dateItems[dates.size()-1]->begin();
                 it != dateItems[dates.size()-1]->end(); ++it) {
                titles.insert( (*it)->title );
            }
        }
    }

    // collection of all items in the 
    std::vector<Item*> items;

    int dateRange[] = { 0,0 };
    {
        int const days = 90;
        struct tm * timeinfo;

        char date[9];
        time_t now; time(&now);
        now -= days * 86400;
        timeinfo = localtime(&now);
        strftime(date,9,"%Y%m%d",timeinfo);
        dateRange[0] = atoi(date);
        now += 2*days * 86400;
        timeinfo = localtime(&now);
        strftime(date,9,"%Y%m%d",timeinfo);
        dateRange[1] = atoi(date);
    }


    switch (format) {
        case Feed::FT_HTML:

            HTML::getItems(*this,items, xml, buffer, size);

            for (std::vector<Item*>::iterator it = items.begin();
                 it != items.end();)
             {
                Item* item = *it;

                const bool titleOk = (!checkAllTitles && item->date > lastDate)
                    || titles.find(item->title) == titles.end();

                if (item->isComplete() && item->date >= lastDate && titleOk
                    && item->date > dateRange[0] && item->date < dateRange[1])
                {
                    ++it;
                } else
                {
                    it = items.erase(it);
                    delete item;
                }
            }
            break;

        case Feed::FT_XML:
            if (xml.tagCount("item")==0)
            {
                while (xml.next("entry"))
                {
                    Item * item = new Item(*this);
                    item->parseDetails(xml, buffer,size);
                    if (item->isComplete()
                        && item->date >= lastDate
                        && ( (!checkAllTitles && item->date > lastDate)
                            || titles.find(item->title) == titles.end() )
                        && news::url::keepItem(*item)
                        && item->date > dateRange[0] && item->date < dateRange[1])
                    {
                        items.push_back(item);
                    } else
                    {
                        delete item;
                    }
                }
            } else
            {
                while (xml.next("item"))
                {
                    Item * item = new Item(*this);
                    item->parseDetails(xml,buffer,size);
                    if (item->isComplete() && item->date >= lastDate
                        && ((!checkAllTitles && item->date > lastDate) || titles.find(item->title) == titles.end())
                        && news::url::keepItem(*item)
                        && item->date > dateRange[0] && item->date < dateRange[1])
                    {
                        items.push_back(item);
                    } else
                    {
                        delete item;
                    }
                }
            }
            break;

        default:
			#ifdef HAVE_PANTHEIOS
            pantheios::logprintf(pantheios::error,
                    "%s %s:%d feed in unknown format at [%s]",
                    __PRETTY_FUNCTION__,__FILE__,__LINE__,
                    link.c_str());
			#endif
            break;
    }

    sort(items.begin(), items.end(), mrutils::ptrCmp<Item>);

    for (std::vector<Item*>::iterator it = items.begin();
         it != items.end(); ) {

        // give the item an ID
        (*it)->id = ATOMIC_INCREMENT(settings::lastItemId);

        // get content
        if (!(*it)->getContent(xmlContent, xml, stopFD, buffer, size))
        {
            status = Feed::FS_ER;
            statusLine.assign("Error getting item content for ");
            statusLine.append( (*it)->link.c_str() );

			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                    "feed [",title,"] error getting item content for [",
                    (*it)->link,"]");
			#endif

            mrutils::mutexAcquire(mutexActiveFeed);
                if (activeFeed == this) viewer.termLine.setStatus(statusLine.c_str());
            mrutils::mutexRelease(mutexActiveFeed);

            delete *it;
            it = items.erase( it );

            if (exit_ && *exit_) break;
            else continue;
        }

        // perform insert
        char * q = mrutils::strcpy(buffer
            ,"INSERT INTO items (itemId,feedId, `date`, title, link, summary) VALUES (");
        q = mrutils::strcpy(q, (*it)->id); *q++ = ',';
        q = mrutils::strcpy(q, this->id); *q++ = ',';
        q = mrutils::strcpy(q, (*it)->date); *q++ = ','; 
        *q++ = '"'; 
        q = mrutils::replaceCopyQuote(q, (*it)->title.c_str(), size - (q - buffer) -1);
        *q++ = '"'; *q++ = ','; *q++ = '"';
        q = mrutils::replaceCopyQuote(q, (*it)->link.c_str(), size - (q - buffer) -1);
        *q++ = '"'; *q++ = ','; *q++ = '"';
        q = mrutils::replaceCopyQuote(q, (*it)->description.c_str(), size - (q - buffer) - 4);
        *q++ = '"'; *q++ = ')'; *q = '\0';
        if (!sql.run(buffer)) {
            status = Feed::FS_ER;
            statusLine.assign("Error executing item insert: ");
            statusLine.append(buffer);

			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                    "error with query [",sql.error,"] [",buffer,"]");
			#endif

            mrutils::mutexAcquire(mutexActiveFeed);
                if (activeFeed == this) viewer.termLine.setStatus(statusLine.c_str());
            mrutils::mutexRelease(mutexActiveFeed);

            (*it)->clearContent(buffer);

            for (;it != items.end(); ++it) delete *it;
            break;
        }

        addItemHelper(*it,buffer,size,viewer,activeFeed,mutexActiveFeed,mutexFeeds);

        ++it;
    }

    return updateFinish(sql,items.size(),buffer,size,viewer,activeFeed,mutexActiveFeed);
}
