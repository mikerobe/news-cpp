#include <iostream>
#include "mr_mysql.h"

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        fprintf(stderr,"Usage: %s [baseurl] [feedId]\n",argv[0]);
        return 1;
    }

    int a = 1;
    const char * baseURL = argv[a++];
    const int feedId = atoi(argv[a++]);

    mrutils::Mysql sql("/tmp/mysql.sock",3306,"news","mgj466a4","news",mrutils::Socket::SOCKET_UNIX);
    if (!sql.connect()) {
        fprintf(stderr,"Unable to connect: %s\n",sql.error.c_str());
        return 1;
    }

    mrutils::stringstream ss;

    std::string title;
    if (!sql.get(ss.clear() << "SELECT title FROM news.feeds WHERE feedId=" << feedId << " LIMIT 1",title)) {
        fprintf(stderr,"Unable to find feed: %s\n",sql.error.c_str());
        return 1;
    }

    std::cout << 
"        <title>" << title << "</title> \n";

    std::cout << "    <lastBuildDate>Fri, 06 Aug 2010 17:01:00 EDT </lastBuildDate>\n";

    if (!sql.rr(ss.clear() << "SELECT itemId,date_format(date,\"%a, %d %b %Y\"),title,description FROM news.items WHERE feedId=" << feedId
        << " ORDER BY date DESC, itemId DESC")) {
        fprintf(stderr,"Mysql unable to execute (%s): %s\n",sql.error.c_str(),ss.c_str());
        return 1;
    }

    while (sql.nextLine()) {
        std::cout << 
"        <item> \n"
"            <title>" << mrutils::replace(sql.getString(2),'&','_') << "</title> \n"
"            <link>" << baseURL << "/content.php?itemid=" << sql.getString(0) << "</link> \n"
"            <guid isPermaLink=\"false\">" << feedId << "_" << sql.getString(0) << "</guid> \n"
"            <pubDate>" << sql.getString(1) << " 08:00:00 EST</pubDate> \n"
"            <description>" << mrutils::replace(sql.getString(3),'&','_') << "</description> \n"
"        </item> \n";
    }

}
