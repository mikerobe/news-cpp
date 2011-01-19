#include <iostream>
#include "mr_mysql.h"
#include "mr_time.h"
#include "mr_strutils.h"

int main(int argc, const char * argv[]) {
    if (argc < 2) {
        fprintf(stderr,"Usage: %s [itemId]\n",argv[0]);
        return 1;
    }

    int a = 1;
    const int itemId = atoi(argv[a++]);

    mrutils::Mysql sql("/tmp/mysql.sock",3306,"news","mgj466a4","news",mrutils::Socket::SOCKET_UNIX);

    if (!sql.connect()) {
        fprintf(stderr,"Unable to connect: %s\n",sql.error.c_str());
        return 1;
    }

    mrutils::stringstream ss;
    time_t now; time(&now);

    // add to history
    oprintf(ss.clear(),"INSERT INTO  news.visits (date, hour, itemId, feedId, feedTitle, itemTitle, link, description) "
           "SELECT %d, %f, %d, feedId, feeds.title,items.title,items.link,items.description FROM news.items JOIN news.feeds USING (feedId) WHERE itemId=%d"
        ,mrutils::getDate(), mrutils::getHoursLocal(now)
        ,itemId,itemId);

    if (!sql.run(ss)) {
        fprintf(stderr,"Error running query: %s",ss.c_str());
    }

    std::string content;
    if (!sql.get(ss.clear() << "SELECT content FROM news.content WHERE itemId=" << itemId << " LIMIT 1",content)) {
        fprintf(stderr,"Unable to get content: %s\n",sql.error.c_str());
        return 1;
    }

    // convert UTF8 and ISO8859-1 to ASCII
    char * ptr = const_cast<char*>(content.c_str());
    content.resize(mrutils::copyToAscii(ptr,content.size()+1,ptr) - ptr);

    FILE * fp = MR_POPEN("/opt/local/bin/elinks -no-home 1 -no-references -default-mime-type 'text/html' -no-numbering -dump-width 45 -dump 1","w");
    fputs(content.c_str(),fp); MR_PCLOSE(fp);

    return 0;
}
