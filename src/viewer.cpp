#include "viewer.h"
#include <iostream>
#include "settings.h"
#include <sys/syslimits.h>
#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
	#include <pantheios/inserters.hpp>
#endif

news::Viewer * news::Viewer::singleton = NULL;

namespace {
    /**
     * Finds occurrence of [img#] starting at start with p 
     * pointing to the beginning and q to the end. Requires that the
     * image number be present in the std::vector with value > -1
     *
     * Returns the matched image id value in the vector. 
     * If none is found returns -1
     */
    inline int findImg(const std::vector<int>& ids, char * const start, char ** p, char ** q) {
        *q = start;
        
        for (;;) {
            if (NULL == (*p = strstr(*q,"[img"))) return -1;
            for (*q = *p + ctlen("[img"); **q >= '0' && **q <= '9'; ++*q) ;
            if (**q != ']') continue;

            ++*q;

            const int i = mrutils::atoin(*p+ctlen("[img"),*q-*p);
            if (i >= (int) ids.size() || i < 0) continue;
            return ids[i];
        }

        // unreachable
        return -1;
    }

    inline char replaceForFileName(char c)
    {
        switch (c) {
            case '/':
            case ':':
            case '\\':
                return '-';
            default:
                return c;
        }
    }
}

bool news::Viewer::displayContent(int itemId,const char * search, int date,
    const char * title)
{
    scroller.clear(false);

    if (search == NULL || search[0] == '\0') scroller.searchStr.clear();
    else scroller.searchStr.assign(search);

    char path[PATH_MAX];

    const int year  = date / 10000;
    const int month = date / 100 % 100;
    const int day   = date % 100;

    char *p = path + sprintf(path,"%s%d/%d-%02d-%02d/%d/",
                    news::settings::contentDir,
                    year,
                    year,
                    month,
                    day,
                    itemId);

    std::vector<int> ids;
    if (DIR *dp = opendir(path))
    {
        char dirData[offsetof(struct dirent, d_name) + PATH_MAX];
        for (struct dirent *dirp; 0 == readdir_r(dp, (struct dirent*)&dirData, &dirp) && dirp;) {
            if (dirp->d_name[0] == '.'
                || 0 == strcmp(dirp->d_name,"article.txt.gz"))
                continue;

            int const imgno = atoi(dirp->d_name);
            if (imgno >= (int)ids.size())
                ids.resize(imgno+1, -1);

            strcpy(p, dirp->d_name);
            if (!reader.open(path))
                continue;
            ids[imgno] = scroller.storeImg(reader);
        }

        closedir(dp);
    }

    // get content
    mrutils::strcpy(p,"article.txt.gz");
    if (!reader.open(path))
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__," can't open content at [",
                path, "]");
		#endif
        return false;
    }

    while (reader.nextLineStripCR()) {
        char * p[2], *q[2]; int id[2], t = 0;

        // mrutils::copyLatin1ToTerminal(reader.line,reader.strlen,reader.line);

        // no images
        if (-1 == (id[t]  = findImg(ids,reader.line,p,q)))
        { scroller.addLine(reader.line,reader.strlen); continue; }

        // skip spaces
        for (;*reader.line == ' ' || *reader.line == '\t';) { ++reader.line; --reader.strlen; }

        // exactly 1 image
        if (-1 == (id[!t] = findImg(ids,q[t],p+!t,q+!t))) 
        { scroller.addImg(id[t],reader.line,p[t]-reader.line,q[t],reader.strlen-(q[t]-reader.line)); continue; }

        for (;;) {
            t = !t;

            // no more images
            if (-1 == (id[!t] = findImg(ids,q[t],p+!t,q+!t)))
            { scroller.addImg(id[t],NULL,0,q[t],reader.strlen-(q[t]-reader.line)); break; }

            // another image
            scroller.addImg(id[t],NULL,0,q[t],p[!t]-q[t]);
        }
    }

    mrutils::stringstream ss;
    oprintf(ss,"%4d-%02d-%02d", year,month,day);
    ss << ' ' << title;

    std::string str = ss.str();
    if (str.length() > 200)
        str.resize(200);
    str.append(".txt");

    std::transform(str.begin(), str.end(), str.begin(), replaceForFileName);
    scroller.display(str.c_str(),settings::saveDir,settings::homeDir);
    return true;
}

void news::Viewer::openBrowser(const std::string& link) {
    char cmd[512];
    sprintf(cmd,"%s \"%s\"", news::settings::openURL,link.c_str());
    if (0 != system(cmd))
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__,
                " error running [", cmd,"], ",
                strerror(errno));
		#endif
    }
}

