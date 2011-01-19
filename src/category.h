#ifndef _MR_NEWS_CATEGORY_H
#define _MR_NEWS_CATEGORY_H

#include <iostream>
#include <string.h>
#include <vector>
#include "feed.h"

namespace news {

class Category {
    public:
        Category(int id, const char * title)
        : id(id), title(title), isAll(false)
        {}

    public:
        inline void joinFeed(Feed* feed) {
            feeds.insert(feed);
            feed->categories.insert(this);
        }

    public:
        inline bool operator<(const Category& other) const {
            if (this == &other) return false;
            if (isAll) return true;
            if (other.isAll) return false;
            return (strcmp(title.c_str(),other.title.c_str()) < 0);
        }

        inline friend std::ostream& operator<<(std::ostream& os, const Category& cat) {
            return os << cat.title;
        }

    public:
        int id; std::string title;
        mrutils::set<Feed*,mrutils::ptrCmp_t<Feed> > feeds;
        bool isAll;
};
}

#endif
