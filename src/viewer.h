#ifndef _MR_NEWS_VIEWER_H
#define _MR_NEWS_VIEWER_H

#include "mr_guiimgscroll.h"
#include "mr_bufferedreader.h"

namespace news {

    class Viewer {
        public:
            static Viewer * singleton;

        public:
            bool displayContent(int itemId, const char * search, int date, const char * title);
            void openBrowser(const std::string& link);

        public:
            mrutils::GuiImgScroll scroller;
            mrutils::BufferedReader reader;
    };

}

#endif
