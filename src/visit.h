#ifndef _MR_NEWS_VISIT_H
#define _MR_NEWS_VISIT_H

#include <iostream>

#include "item.h"
#include "settings.h"

namespace news {

class Visit {
    public:
        inline void setName(char * buffer, _UNUSED int size) {
            int hour = this->hour;
            int minute = (this->hour - hour) * 60;
            sprintf(buffer, "% 2d:%02d %s"
                ,hour, minute
                ,itemTitle.c_str());
            name = buffer;

            sprintf(buffer,"%d %s", date, itemTitle.c_str());
            dateName = buffer;
        }

    public:
       /******************
        * Main functions
        ******************/
        inline bool displayContent() {
            news::Viewer::singleton->scroller.assignFunction('\'',
                fastdelegate::MakeDelegate(this,&Visit::openBrowser));
            #ifdef __APPLE__
                news::Viewer::singleton->scroller.assignFunction('m',
                    fastdelegate::MakeDelegate(this,&Visit::copyMarkdownURL));
                news::Viewer::singleton->scroller.assignFunction('i',
                    fastdelegate::MakeDelegate(this,&Visit::openMail));
                news::Viewer::singleton->scroller.assignFunction('U',
                    fastdelegate::MakeDelegate(this,&Visit::copyURL));
                news::Viewer::singleton->scroller.clearXAttributes();
                news::Viewer::singleton->scroller.addXAttribute(
                    "com.apple.metadata:kMDItemWhereFroms",
                    link.c_str());
            #endif
            return Viewer::singleton->displayContent(itemId,"",itemDate,
                itemTitle.c_str());
        }
        inline bool openBrowser() {
            news::Viewer::singleton->openBrowser(link);
            return true;
        }

        #ifdef __APPLE__
        bool openMail();
        bool copyURL();
        bool copyMarkdownURL();
        #endif

    public:
        int itemId, feedId;
        int date; double hour;
        std::string feedTitle, itemTitle, link;
        std::string name;
        std::string dateName;

        int itemDate;

        // if no item
        std::string description;
};

}

#endif
