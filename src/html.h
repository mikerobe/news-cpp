#ifndef _MR_NEWS_HTML_H
#define _MR_NEWS_HTML_H

#include "item.h"
#include <vector>

namespace news {

class Feed;

class HTML {
public:
    /**
     * Takes the given xml (with content already retrieved)
     * and parses the content, constructing items from it
     * */
    static void getItems(Feed& feed, std::vector<Item*>& items
        ,mrutils::XML& xml, char * buffer, int size);

    /**
     * Strips tags, returns, special characters and unescapes HTML
     */
    static void makeAscii(std::string &str);
};

}

#endif
