#include <iostream>
#include "mr_mysql.h"

int main(int argc, const char * argv[]) {
    int a = 1;
    const int catId = argc > a ? atoi(argv[a++]) : -3;

    mrutils::Mysql sql("/tmp/mysql.sock",3306,"news","mgj466a4","news",mrutils::Socket::SOCKET_UNIX);
    if (!sql.connect()) {
        fprintf(stderr,"Unable to connect: %s\n",sql.error.c_str());
        return 1;
    }

    mrutils::stringstream ss;

    if (catId >= -2) {
        if (!sql.rr(
                catId == -1
                ? "SELECT feedid,title from feeds order by title"
                : catId == -2
                 ? "SELECT catId,title from categories order by title"
                 : (ss.clear() << 
                   "SELECT feedId,title from feeds join feedcat using (feedid) where catid="
                   << catId << " order by title").c_str())) {
            fprintf(stderr,"Unable to execute: %s: %d\n",__FILE__,__LINE__);
            return 1;
        }

        std::vector<int> ids;
        std::vector<std::string> titles;

        while (sql.nextLine()) {
            ids.push_back(sql.getInt(0));
            titles.push_back(mrutils::replace(sql.getString(1),'&','+'));
        }

        if (catId != -1 && ids.empty()) {
            std::cout << "new Array()";
        } else {
            std::cout << "new Array(new Array(";
            int i = 0;

            if (catId != -1) { i = 0; std::cout << "-1";
            } else { i = 1; std::cout << ids[0]; }
            for(; i < (int)ids.size(); ++i) std::cout << "," << ids[i];

            std::cout << "), new Array(";

            if (catId != -1) { i = 0; std::cout << "\"All\"";
            } else { i = 1; std::cout << '"' << titles[0] << '"'; }
            for (; i < (int)titles.size(); ++i) std::cout << ",\"" << titles[i] << '"';

            std::cout << "))";
        }
    } else {
        if (!sql.rr("SELECT catId,title FROM news.categories order by title")) {
            fprintf(stderr,"Unable to execute: %s: %d\n",__FILE__,__LINE__);
            return 1;
        }

        std::cout << "<li class='mainLI' onclick='this.className=\"mainLI click\";listCategory(-1,\"All\")'>All</li>\n";
        while (sql.nextLine()) {
            std::cout << 
                "        <li class='mainLI' onclick='this.className=\"mainLI click\";listCategory(" << sql.getInt(0) << ",\"" << sql.getString(1) << "\")'>"
                "           " << mrutils::replace(sql.getString(1),'&','+') << ""
                "</li>\n";
        }
    }
    
}
