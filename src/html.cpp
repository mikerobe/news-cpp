#include "html.h"
#include "feed.h"
#include "mr_time.h"
#include "rapidxml.hpp"
#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
	#include <pantheios/inserters.hpp>
#endif

namespace {
    /**
     * deletes each <...> tag (but not contents between <...>and</...>
     */
    inline std::string& stripTags(std::string& description) {
        mrutils::trim(description);
        for (size_t pos = description.find('<');pos != std::string::npos;pos = description.find('<',pos)) {
            size_t qos = description.find('>',pos+1);
            if (qos == std::string::npos) {
                description.erase(pos);
                break;
            } else description.erase(pos,qos - pos + 1);
        }
        return description;
    }

    /**
     * Takes a date of the form "July 7, 2011"
     * and returns the integer representation
     */
    int getDateFromFull(char const *str)
    {
        int month = mrutils::stringMonthToNumber(str);
        for (;!isnumber(*str);++str) ;
        int day = atoi(str);
        for (;isnumber(*str);++str) ;
        for (;!isnumber(*str);++str) ;
        int year = atoi(str);
        return year * 10000 + month * 100 + day;
    }

    inline std::string& stripReturns(std::string& description) {
        char * p = const_cast<char *>(description.c_str());
        char * q = p;
        for (;;)
        {
            if (!*q)
            {
                *p = '\0';
                break;
            }

            if (::isspace(*q))
            {
                *p++ = ' ';
                while (::isspace(*++q))
                    ;
            } else
            {
                *p++ = *q++;
            }

        }

        description.resize(p - description.c_str());
        return description;
    }

    inline std::string& unescapeHTML(std::string& description) {
        char * d = const_cast<char*>(description.c_str());
        description.resize(mrutils::XML::unescapeHTML(d,d+description.size())-d);
        return description;
    }

    inline std::string& stripNonAscii(std::string& description) {
        char * d = const_cast<char*>(description.c_str());
        mrutils::stripNonAscii(d,d+description.size());
        return description;
    }

    inline std::string& parseText(std::string& title) {
        return unescapeHTML(stripReturns(stripTags(stripNonAscii(title))));
    }

    inline std::string& parseTitle(std::string& title) {
        parseText(title);
        if (title.size() > 63) title.resize(63);
        return title;
    }

    inline std::string& parseDescription(std::string& title) {
        parseText(title);
        if (title.size() > 511) title.resize(511);
        return title;
    }

    inline std::string& parseLink(std::string& link) {
        mrutils::trim(link);
        if (link.size() > 127) link.resize(127);
        return link;
    }
}

void news::HTML::makeAscii(std::string &str)
{ unescapeHTML(stripReturns(stripTags(str))); }

namespace {

    void makeLink(news::Item *item, mrutils::XML &xml)
    {
        news::HTML::makeAscii(item->link);

        char buffer[1024];

        if (item->link[0] == '/')
        {
            if (xml.m_url.empty())
                return;

            char const *endURL = xml.m_url.c_str();
            for (int c = 0; c < 3 && endURL; ++c)
            { endURL = strchr(endURL+1, '/'); }

            char *eob = mrutils::strncpy(buffer, xml.m_url.c_str(),
                endURL-xml.m_url.c_str());
            strcpy(eob, item->link.c_str());
            item->link.assign(buffer);
        }
    }

    /**
     * Given a tag, finds the corresponding end tag.
     * @return NULL if failure
     */
    char * findEndTag(char * p)
    {
        if (p == NULL || *p != '<')
            return NULL;

        char tagStart[128];
        char tagEnd[128];

        memset(tagStart,0,sizeof(tagStart));
        memset(tagEnd,0,sizeof(tagEnd));

        size_t const lenStart = strcspn(p," >");
        if (lenStart > sizeof(tagStart) - 3)
            return NULL;

        memcpy(tagStart,p,lenStart);
        sprintf(tagEnd,"</%s>",tagStart+1);

        size_t const lenEnd = strlen(tagEnd);

        char * end = p + 1;

        for (int depth = 0;NULL != (end = strchr(end,'<')); ++end) {
            if (0 == strncmp(end,tagStart,lenStart)) {
                ++depth;
                continue;
            } else if (0 == strncmp(end,tagEnd,lenEnd)) {
                if (depth == 0)
                    break;
                else
                    --depth;
            }
        }

        return end;
    }

}


namespace {
    void bloomberg(news::Feed& feed, std::vector<news::Item*>& items, mrutils::XML& xml,
        _UNUSED char * buffer, _UNUSED int size)
    {

        char * start = xml.sob;
        news::Item item(feed);

        for (char * p = start, *end = p; NULL != (p = strstr(p, "<div class=\"news_item ")); p = end+1) {
            item.clear();

            if (NULL == (end = findEndTag(p)))
                break;
            end += strlen("</div>");
            *end = '\0';

            *mrutils::copyToAscii(p,end-p,p,true) = '\0';

            if (char * q = strstr(p,"/news/")) {
                item.link.assign(q, strchr(q,'"')-q);
                q += strlen("/news/");
                item.date = atoi(q)*10000 + atoi(q+5)*100 + atoi(q+8);;
                q = strchr(q, '>')+1;
                item.title.assign(q,strchr(q,'<')-q);


                news::HTML::makeAscii(item.title);
                if (item.title.empty())
                {
                    if (char * w = strstr(q,"/news/"))
                    {
                        w = strchr(w, '>')+1;
                        item.title.assign(w,strchr(w,'<')-w);
                        news::HTML::makeAscii(item.title);
                    }
                }

                q = strstr(q,"<p>");
                if (!q)
                {
                    item.description.clear();
                } else
                {
                    q += strlen("<p>");
                    item.description.assign(q, strstr(q,"</p>")-q);
                }
            } else {
                continue;
            }

            makeLink(&item, xml);
            news::HTML::makeAscii(item.description);
            items.push_back(new news::Item(item));
        }
    }

    void atlantic(news::Feed& feed, std::vector<news::Item*>& items, mrutils::XML& xml, char * buffer, _UNUSED int size)
    {
        char * root = mrutils::strcpy(buffer,"http://www.theatlantic.com/");
        char * eoPath = mrutils::strcpy(root,"doc/current/");

        char * start = xml.sob;
        char * p = strstr(start, "s.prop8");
        if (p == NULL) return;
        p = strstr(p, "201");
        if (p == NULL) return;
        int date = atoi(p)*100 + 1;
        if (date < 201000) return;

        p = strstr(start,"<section class=\"magazine-section");

        while ( NULL != (p = strstr(p, "h3 class=\"river-headline\"")) ) {
            char * q = strstr(p, "href");

            //char * q = mrutils::strstrRev(p, "href", start);
            //if (q == NULL) break;

            news::Item* item = new news::Item(feed);
            item->date = date;

            xml.tagPtr = mrutils::strchrRev(q,'<',start);
            xml.tag(item->title);
            xml.next("p"); xml.tag(item->description);

            q = strchr(q,'"') + 1;
            p = strchr(q,'"'); *p = 0;

            if (mrutils::startsWithI(q,"http://") || mrutils::startsWithI(q,"https://")) {
                item->link = q;
            } else if (*q == '/') {
                mrutils::strcpy(root,q);
                item->link = buffer;
                eoPath = mrutils::strcpy(root,"doc/current/");
            } else {
                mrutils::strcpy(eoPath,q);
                item->link = buffer;
            }

            const bool inMagazine = strstr(item->link.c_str(),"magazine");

            if ((feed.id == 469) == inMagazine)
                items.push_back(item);
            else
                delete item;

            ++p;
        }
    }

    /**
     * Get the last 3 days worth of content from efinancial USA
     */
    void efinancial(news::Feed& feed, std::vector<news::Item*>& items, mrutils::XML& xml, char * buffer, _UNUSED int size)
    {
        const int today = mrutils::getDate();
        const int daysago = mrutils::prevDay(mrutils::prevDay(mrutils::prevDay(today)));
        char * eob = mrutils::strcpy(buffer,"http://jobs.efinancialcareers.com/USA.htm/browseSort-date/browserow-");
        char * str = eob + 10;

        char empty = 0;

        char * start = xml.sob;
        char * p = start, *link, *title, *company, *comp, *loc, *desc, *dateStr, *q;
        int date = today;
        int page = 1; mrutils::strcpy(eob,page);

        for (;;) { // pages loop

            for (q = strstr(start,"<table class=\"jobAdTable\">");q != NULL;) { // loop within the page
                p = q; q = strstr(p+1,"<table class=\"jobAdTable\">"); if (q != NULL) *q++ = 0;

                if (NULL == (link = strstr(p,"href="))) continue; p = strchr(link += 6,'"'); *p++ = 0;
                if (NULL == (title = strchr(p,'>'))) continue; p = strchr(++title,'<'); *p++ = 0;
                if (NULL == (company = strstr(p,"class=\"companyHighlighting\""))) continue; company = strchr(company,'>')+1; p = strchr(company,'<'); *p++ = 0;
                if (NULL == (comp = strstr(p,"class=\"jobSalary\""))) comp = &empty; else { comp = strchr(comp,'>')+1; p = strchr(comp,'<'); *p++ = 0; }
                if (NULL == (loc = strstr(p,"class=\"jobCol3\""))) loc = &empty; else { loc = strchr(loc,'>')+1; p = strchr(loc,'<'); *p++ = 0; }
                if (NULL == (dateStr = strstr(p,"class=\"jobCol4\""))) { date = 0; break; } else { p = strchr(dateStr,'>')+1; date = 20000000 + atoi(p); p = strchr(p,' ')+1; date += 100 * mrutils::stringMonthToNumber(p) + 10000*atoi(strchr(p,' ')+1); }
                if (NULL == (desc = strstr(p,"class=\"jobDesc\""))) desc = &empty; else { desc = strchr(desc,'>')+1; for (;*desc == ' ' || *desc == '\n' || *desc == '\r';++desc){} p = strchr(desc,'<'); *p++ = 0; }

                // check for non-ascii characters
                mrutils::stripNonAscii(title);
                mrutils::stripNonAscii(company);
                mrutils::stripNonAscii(comp);
                mrutils::stripNonAscii(loc);
                mrutils::stripNonAscii(desc);

                sprintf(str,"%s (%s)",title,company);

                news::Item* item = new news::Item(feed);
                item->date = date;
                item->link = link;
                item->title = str;

                sprintf(str,"Comp: %s\nLoc: %s\n\n%s",comp,loc,desc);
                item->description = str;

                items.push_back(item);
            }

            if (date < daysago) break;
            page += 30; mrutils::strcpy(eob,page);

            mrutils::curl::urlRequest_t request(buffer);
            if (0 != xml.get(&request))
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                    "unable to get url [",buffer,"]");
				#endif
                break;
            }
        }

    }

    void setLink(news::Item *item, char *q, char *buffer, char *root, char *eoPath)
    {
        if (mrutils::startsWithI(q,"http://") || mrutils::startsWithI(q,"https://")) {
            item->link = q;
        } else if (*q == '/') {
            item->link = std::string(buffer,root-buffer);
            item->link.append(q);
        } else {
            mrutils::strcpy(eoPath,q);
            item->link = buffer;
        }
    }


    void chicagobooth(news::Feed& feed, std::vector<news::Item*>& items, mrutils::XML& xml, char * buffer, _UNUSED int size)
    {
        const int today = mrutils::getDate();
        char * root = mrutils::strcpy(buffer,"http://www.chicagobooth.edu/");
        char * eoPath = mrutils::strcpy(root,
            strstr(xml.m_url.c_str(),"chicagobooth.edu/") + strlen("chicagobooth.edu/"));

        // make the whole contents pure ascii
        *mrutils::copyToAscii(xml.sob,xml.eob-xml.sob,xml.sob,true) = '\0';

        for (char *start = xml.sob; NULL != (start = strstr(start,"<a class=\"morelink\""));++start) {
            news::Item * item = new news::Item(feed);

            // date is today
            item->date = today;

            // link
            char *p = strchr(strstr(start,"href"),'"')+1;
            char *q = strchr(p,'"');
            *q = '\0';
            setLink(item,p,buffer,root,eoPath);
            *q = '"';

            // description
            if (NULL == (p = mrutils::strstrRev(start,"<p",xml.sob))
                || NULL == (p = mrutils::strstrRev(p-1,"<p",xml.sob))
                || NULL == (p = strchr(p,'>'))
                || NULL == (p = p+1)
                || NULL == (q = strstr(p,"</p>"))) {
                delete item;
                continue;
            }
            item->description.assign(p,q-p);
            unescapeHTML(stripReturns(stripTags(item->description)));

            // title
            if (NULL == (p = mrutils::strstrRev(start,"<h3",xml.sob))
                || NULL == (p = strchr(p,'>'))
                || NULL == (p = p+1)
                || NULL == (q = strstr(p,"</h3>"))) {
                delete item;
                continue;
            }
            item->title.assign(p,q-p);
            unescapeHTML(stripReturns(stripTags(item->title)));

            items.push_back(item);
        }

    }

    void vfdaily(news::Feed& feed, std::vector<news::Item*>& items, mrutils::XML& xml, _UNUSED char * buffer, _UNUSED int size)
    {
        return; // TODO they've changed the site
        for (char * start = xml.sob, * end = xml.sob;end != xml.eob;start = end+1) {
            if (NULL == (start = mrutils::stristr(start,"class=\"headline"))) break;
            if ('<' != *(start = mrutils::strchrRev(start,'<',end))) break;
            if (NULL == (end = strchr(start,'>'))) end = xml.eob;
            if (NULL == (end = mrutils::stristr(end,"class=\"headline"))) end = xml.eob;
            *(end = mrutils::strchrRev(end,'<',start) - 1) = '\0';

            std::string description, title, link;

            int date = 0;
            char * p, * q, *r;

            if (NULL == (p = mrutils::stristr(start,"<div class=\"published\""))
                || NULL == (q = strchr(p,'>'))
                || NULL == (p = mrutils::stristr(p,"title="))
                || NULL == (p = strchr(p,'"'))
                || 20100101 >= (date = mrutils::parseGuessDate(p+1)) || 20500000 < date
                ) continue;

            if (NULL == (p = mrutils::stristr(start,"h3 class=\"entry-title"))
                || NULL == (p = strchr(p,'>'))
                || NULL == (q = mrutils::stristr(++p,"</h3>"))
                ) continue;
            title.assign(p,q-p);
            parseTitle(title);

            *(r = q) = '\0';
            if (NULL == (p = mrutils::stristr(p,"href=\""))
                || NULL == (q = strchr(p += ctlen("href=\""),'"'))
                ) continue;
            *r = '>'; 
            link.assign(p,q-p); 
            parseLink(link);

            if (NULL == (p = mrutils::stristr(start,"<div class=\"entry-content"))
                || NULL == (p = strchr(p,'>'))
                || NULL == (q = mrutils::stristr(++p,"</div>"))
                ) continue;
            description.assign(p,q-p);
            parseDescription(description);

            news::Item* item = new news::Item(feed);
            item->date = date;
            item->link = link;
            item->title = title;
            item->description = description;
            items.push_back(item);
        }
    }

    void aldaily(news::Feed& feed, std::vector<news::Item*>& items, mrutils::XML& xml, _UNUSED char * buffer, _UNUSED int size)
    {
        const int today = mrutils::getDate();
        char * start = mrutils::stristr(xml.sob,"<!-- BEGIN CONTENT COLUMN");

        for (char * end = start;;)
        {
            if (start == NULL)
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                    "error parsing HTML: no start found");
				#endif
                break;
            }

            end = mrutils::stristr(start+1,"<!-- BEGIN CONTENT COLUMN");
            bool atEOB = end == NULL;
            if (atEOB) end = mrutils::stristr(start+1,"</table>");
            if (end == NULL)
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                    "error parsing HTML: no end found");
				#endif
                break;
            }

            for (char * p = strchr(start,'>')+1, *q = p;
                    q != end;
                    p = q == end ? q : (strchr(q+1,'>') + 1))
            {
                q = mrutils::stristr(p,"<hr");
                if (q == NULL || q > end) q = end;
                *q = 0;

                mrutils::stripNonAscii(p);
                std::string description = p;

                char * link = mrutils::stristr(p,"<a href");
                if (!link)
                    continue;
                char *e = strchr(link,'>');
                if (!e)
                    continue;
                *e = '\0';
                e = strchr(link,'=');
                if (!e)
                    continue;
                while (isspace(*++e)) ;
                if (*e == '\"')
                {
                    link = e+1;
                    e = strchr(link,'\"');
                    if (!e)
                        continue;
                    *e = '\0';
                } else if (*e == '\0')
                {
                    continue;
                } else
                {
                    link = e;
                    while (!isspace(*++e) && *e != '\0') ;
                    *e = '\0';
                }

                unescapeHTML(stripReturns(stripTags(description)));

                // strip beginning "--->" constructs
                size_t pos = description.find_first_not_of("->");
                if (std::string::npos != pos && pos > 0) {
                    description = description.substr(pos);
                }

                if (mrutils::startsWithI(description.c_str(),
                        "Arts & Letters Daily"))
                    continue;

                news::Item* item = new news::Item(feed);
                item->date = today;
                item->link = link; mrutils::trim(item->link);
                item->title = description;
                if (item->title.size() > 63) item->title.resize(63);

                item->description = description;

                items.push_back(item);
            }

            if (atEOB) break;
            ++end; start = end;
        }
    }

namespace wsj {
    void setDate(news::Item *item, mrutils::XML &xml)
    {
        static char const * dateStr="var ITP_INFO = {date:'";

        if (char *p = strstr(xml.sob, dateStr))
        {
            p += strlen(dateStr);
            item->date = atoi(p);
            if (item->date < 20110101 || item->date > 30000000)
                throw "invalid date";
        } else
        {
            throw "couldn't find dateStr on page";
        }
    }
}

    void wsj_pageone(news::Feed& feed, std::vector<news::Item*>& items,
        mrutils::XML& xml, _UNUSED char * buffer, _UNUSED int size)
    {
        try {
            news::Item item(feed);
            wsj::setDate(&item, xml);

            for (char *row = xml.sob; NULL != (row = strstr(row, "class=\"mjLinkItem"));
                ++row)
            {
                row = strchr(row,'>') + 1;
				if (!row) continue;
                item.title.assign(row, strchr(row,'<')-row);

                if (*(row = mrutils::strchrRev(row,'<',xml.sob)) != '<') throw "";
                row = strstr(row,"href");
                row = strchr(row,'"') + 1;
                item.link.assign(row,strchr(row,'"') - row);

                row = strstr(row,"<p>") + strlen("<p>");
                item.description.assign(row, strstr(row,"</p>") - row);

                makeLink(&item, xml);
                news::HTML::makeAscii(item.title);
                news::HTML::makeAscii(item.description);

                items.push_back(new news::Item(item));
            }
        } catch (char const *err)
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                    "error [",err,"]");
			#endif
        } catch (...)
        {}
    }

    void wsj_whatsnews(news::Feed& feed, std::vector<news::Item*>& items,
        mrutils::XML& xml, _UNUSED char * buffer, _UNUSED int size)
    {
        char * p = strstr(xml.sob, "class=\"whatsNews-simple headlineSummary");
        if (!p)
            return;

        char *start = p;
        for (int divCount = 1;divCount > 0;)
        {
            static char const *div = "<div";
            static char const *divEnd = "</div";

            p = strchr(p+1,'<');
            if (0 == strncmp(p, div, strlen(div)))
                ++divCount;
            else if (0 == strncmp(p, divEnd, strlen(divEnd)))
                --divCount;
        }

        *p = '\0';

        try {
            news::Item item(feed);
            wsj::setDate(&item, xml);

            for (char *row = start; NULL != (row = strstr(row, "<a "));
                ++row)
            {
                row = strstr(row,"href");
                row = strchr(row,'"') + 1;
                item.link.assign(row,strchr(row,'"') - row);

                row = strchr(row,'>') + 1;
                item.title.assign(row, strchr(row,'<')-row);
                item.description.assign(row, strstr(row,"</p>") - row);

                makeLink(&item, xml);
                news::HTML::makeAscii(item.title);
                news::HTML::makeAscii(item.description);

                if (item.link == "http://online.wsj.com/home")
                    continue;

                items.push_back(new news::Item(item));
            }
        } catch (char const *err)
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                    "error [",err,"]");
			#endif
        } catch (...)
        {}
    }

    void nymag(news::Feed& feed, std::vector<news::Item*>& items,
        mrutils::XML& xml, _UNUSED char * buffer, _UNUSED int size)
    {
        news::Item item(feed);

        try {
            char *p = strstr(xml.sob,"Issue_Date");
            if (!p) {
				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::error,
                        __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                        "can't get issue date");
				#endif
                return;
            }
            p = mrutils::strchrRev(p,'<',xml.sob);
            char *q = strchr(p,'>');
            if (!q) {
				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::error,
                        __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                        "can't find ending paragraph");
				#endif
                return;
            }
            if (*(q-1) != '/') {
                *q++ = '/';
                *q = '>';
            }
            *++q = '\0';

            rapidxml::xml_document<> doc;
            doc.parse<0>(p);
            rapidxml::xml_node<> &meta = doc.find_node("meta");
            char * dateStr = meta.find_attribute("content").value();

            int date = getDateFromFull(dateStr);
            if (date < 20010101 || date > 30000101)
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::error,
                        __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                        "got invalid date from [",dateStr,"]");
				#endif
                return;
            }

            item.date = date;

            memset(p,' ',q-p+1);
        } catch (rapidxml::parse_error const &e) {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                    "rapidXML error [",e.what(),"]");
			#endif
            return;
        }

        for (char * p = xml.sob; p != xml.eob; ++p) {
            if (NULL == (p = strstr(p,"<h5>")))
                break;

            char *q = strstr(p,"</h5>");

            if (NULL == q)
                break;

            q += strlen("</h5>");
            *q = '\0';

            try {
                rapidxml::xml_document<> doc;
                doc.parse<0>(p);
                rapidxml::xml_node<> &h5 = doc.find_node("h5");
                rapidxml::xml_node<> &link = h5.find_node("a");

                char * href = link.find_attribute("href").value();
                if (!mrutils::startsWithI(href,"http://")) {
                    p = q;
                    continue;
                }

                item.title = link.value();
                item.link = href;
                char * d = const_cast<char*>(item.title.c_str());
                item.title.resize(mrutils::XML::unescapeHTML(d,d+item.title.size())-d);
            } catch (rapidxml::parse_error const &e)
            {
                p = q;
                continue;
            }

            if (NULL == (p = strstr(q+1,"<p>"))) {
                p = q;
                continue;
            }

            if (NULL == (q = findEndTag(p))) {
                continue;
            }

            p += strlen("<p>");

            item.description.assign(p,q-p);
            unescapeHTML(stripReturns(stripTags(item.description)));

            items.push_back(new news::Item(item));
        }
    }

}

void news::HTML::getItems(news::Feed& feed, std::vector<news::Item*>& items, mrutils::XML& xml, char * buffer, int size) {
    const char * url = feed.link.c_str();

    if (NULL != strstr(url, "theatlantic.com")) {
        atlantic(feed, items, xml,buffer,size);
    } else if (NULL != strstr(url,".bloomberg.com")) {
        bloomberg(feed, items, xml,buffer,size);
    } else if (NULL != strstr(url,"jobs.efinancialcareers.com")) {
        efinancial(feed,items,xml,buffer,size);
    } else if (NULL != strstr(url,"aldaily.com")) {
        aldaily(feed,items,xml,buffer,size);
    } else if (NULL != strstr(url,"vanityfair.com/online/daily")) {
        vfdaily(feed,items,xml,buffer,size);
    } else if (NULL != strstr(url,"chicagobooth.edu/magazine")) {
        chicagobooth(feed,items,xml,buffer,size);
    } else if (NULL != strstr(url,"nymag.com")) {
        nymag(feed,items,xml,buffer,size);
    } else if (NULL != strstr(url,"online.wsj.com")) {
        if (NULL != strstr(url, "/us/whatsnews"))
            wsj_whatsnews(feed,items,xml,buffer,size);
        else
            wsj_pageone(feed,items,xml,buffer,size);
    }
}

