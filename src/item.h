#pragma once

#include "mr_xml.h"
#include "mr_threads.h"
#include "settings.h"
#include "viewer.h"
#include "mr_sql.h"
#include "history.h"

namespace news {

class Feed;

class _API_ Item {
public:
    Item(Feed& feed)
    : id(-1)
     ,feed(feed)
     ,date(-1)
     ,marked(false)
     ,mutexContent(mrutils::mutexCreate())
     ,isGone(false)
     ,isArchived(false)
    {}

public:
   /******************
    * Main functions
    ******************/
    inline bool isComplete() 
    { return (title.size() > 0 && link.size() > 0 && date > 0); }

    void parseDetails(mrutils::XML& xml, char * buffer, int size);
    bool getContent(mrutils::XML& xml, mrutils::XML &xmlSecondary, 
        int stopFD, char * buffer, int size, bool save = true);

    bool clearContent(char * buffer, bool lock = true);

    bool displayContent(mrutils::Sql& sql, const char * search = "")
	{
        if (isGone)
            return false;
        History::singleton->add(sql,this);
        news::Viewer::singleton->scroller.assignFunction('\'',
            fastdelegate::MakeDelegate(this,&Item::openBrowser));
        #ifdef __APPLE__
            news::Viewer::singleton->scroller.assignFunction('i',
                fastdelegate::MakeDelegate(this,&Item::openMail));
			news::Viewer::singleton->scroller.assignFunction('m',
				fastdelegate::MakeDelegate(this,&Item::copyMarkdownURL));
            news::Viewer::singleton->scroller.assignFunction('U',
                fastdelegate::MakeDelegate(this,&Item::copyURL));
            news::Viewer::singleton->scroller.clearXAttributes();
            news::Viewer::singleton->scroller.addXAttribute(
                "com.apple.metadata:kMDItemWhereFroms",
                link.c_str());
        #endif
        return news::Viewer::singleton->displayContent(this->id,search,date,title.c_str());
    }

    inline void clear()
    {
        id = -1;
        date = -1;
        marked = false;
        title.clear();
        link.clear();
        description.clear();
        isGone = false;
        isArchived = false;
    }

    #ifdef __APPLE__
    bool openMail();
    bool copyURL();
    bool copyMarkdownURL();
    #endif

    inline bool openBrowser() {
        news::Viewer::singleton->openBrowser(link);
        return true;
    }

public:
   /*****************
    * Utilities
    *****************/

    inline bool operator<(const Item& other) const {
        return (date < other.date);
    }


private:
    struct imgData_t {
        char * start, * end;
        std::set<mrutils::curl::urlRequestM_t>::iterator request;

        imgData_t(char * start, char * end, const std::set<mrutils::curl::urlRequestM_t>::iterator& request)
            : start(start), end(end), request(request) 
        {}
    };

    static void findImages(char * buffer, const std::string& link, std::vector<imgData_t>& imgData, std::set<mrutils::curl::urlRequestM_t>& imgRequests);
    bool saveImages(char * buffer, std::set<mrutils::curl::urlRequestM_t>& imgRequests, int stopFD);
    bool saveContent(mrutils::XML& xml, char * buffer, const std::vector<imgData_t>& imgData);

    void findImg(std::string *path, bool *imgExists, std::string const &content,
        std::string const &extension, char *buffer);


public:
    int id; Feed& feed;
    int date; bool marked;
    std::string title, link, description;
    mrutils::mutex_t mutexContent;

    bool isGone, isArchived;
};
}
