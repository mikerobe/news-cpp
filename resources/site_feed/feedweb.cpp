#include <iostream>
#include "mr_mysql.h"

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        fprintf(stderr,"Usage: %s [feedId] [categoryid]\n",argv[0]);
        return 1;
    }

    int a = 1;
    const int feedId = atoi(argv[a++]);
    const int catId = atoi(argv[a++]);

    mrutils::Mysql sql("/tmp/mysql.sock",3306,"news","mgj466a4","news",mrutils::Socket::SOCKET_UNIX);
    if (!sql.connect()) {
        fprintf(stderr,"Unable to connect: %s\n",sql.error.c_str());
        return 1;
    }

    mrutils::stringstream ss;
    std::string title;
    if (feedId >= 0) {
        if (!sql.get(ss.clear() << "SELECT title FROM news.feeds WHERE feedId=" << feedId << " LIMIT 1",title)) {
            fprintf(stderr,"Unable to find feed: %s\n",sql.error.c_str());
            return 1;
        }
        sql.run(ss.clear() << "UPDATE news.feeds set numUnread=0 where feedid=" << feedId);
        ss.clear() 
            << "SELECT itemId,date_format(date,\"%a, %d %b %Y\"),title,description,\"" << title << "\" FROM news.items WHERE feedId=" << feedId
            << " ORDER BY date DESC, itemId DESC LIMIT 100";
    } else {
        if (!sql.get(ss.clear() << "SELECT title FROM news.categories WHERE catId=" << catId << " LIMIT 1",title)) {
            fprintf(stderr,"Unable to find feed: %s\n",sql.error.c_str());
            return 1;
        }

        sql.run(ss.clear() << "UPDATE news.feeds set numUnread=0 WHERE feedId in (SELECT feedid from news.feedCat where catId=" << catId << ")");
        ss.clear() 
            << "SELECT itemId,date_format(date,\"%a, %d %b %Y\"),items.title,description,feeds.title FROM news.items "
               "JOIN news.feeds USING (feedId) WHERE feedId IN ("
               "SELECT feedId from news.feedCat WHERE catId=" << catId << ") "
               " ORDER BY date DESC, itemId DESC LIMIT 300";
    }

    if (!sql.rr(ss)) {
        fprintf(stderr,"Mysql unable to execute (%s): %s\n",sql.error.c_str(),ss.c_str());
        return 1;
    }

    std::cout << "<script language=\"JavaScript\">document.title=\"" << title << "\"; document.getElementById(\"headertext\").innerText=\"" << title << "\";</script>\n";

    while (sql.nextLine()) {
        std::cout << 
"        <li id=\"item" << sql.getString(0) << "\" onclick=\"this.className='click';loadPage(" << sql.getString(0) << ")\">"
"           <b>" << mrutils::replace(sql.getString(2),'&','+') << "</b>"
"           <br><i>" << sql.getString(4) << ", " << sql.getString(1) << "</i>"
"           <br>" << sql.getString(3) <<
"</li>\n";
    }

}
