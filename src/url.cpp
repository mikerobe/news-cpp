#include "url.h"
#include "mr_strutils.h"
#include "item.h"
#include "mr_encoder.h"
#include "settings.h"
#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
	#include <pantheios/inserters.hpp>
#endif

std::string news::url::getDomain(std::string const &url)
{
	std::string result;

	char * start = strstr(url.c_str(),"//");
	if (!start)
		return "";

	start += 2;
	char * end = strchr(start,'/');
	if (!end)
		end = start + strlen(start);


	char * tmp = mrutils::strchrRev(end,'.',start);
	do {
		if (tmp == NULL)
		{
			tmp = start;
			break;
		}

		int times = 1;

		if (tmp+3 == end)
		{
			switch (::tolower(*(tmp+1)))
			{
				case 'a':
					switch (::tolower(*(tmp+2)))
					{
						case 'd':
						case 'e':
						case 'f':
						case 'g':
						case 'i':
						case 'l':
						case 'm':
						case 'n':
						case 'o':
						case 'q':
						case 'r':
						case 's':
						case 't':
						case 'u':
						case 'w':
						case 'z':
							++times;
							break;
					}
					break;
				case 'b':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'b':
						case 'd':
						case 'e':
						case 'f':
						case 'g':
						case 'h':
						case 'i':
						case 'j':
						case 'm':
						case 'n':
						case 'o':
						case 'r':
						case 's':
						case 't':
						case 'v':
						case 'w':
						case 'y':
						case 'z':
							++times;
							break;
					}
					break;
				case 'c':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'c':
						case 'f':
						case 'g':
						case 'h':
						case 'i':
						case 'k':
						case 'l':
						case 'm':
						case 'n':
						case 'o':
						case 'r':
						case 's':
						case 'u':
						case 'v':
						case 'x':
						case 'y':
						case 'z':
							++times;
							break;
					}
					break;
				case 'd':
					switch (::tolower(*(tmp+2)))
					{
						case 'e':
						case 'j':
						case 'k':
						case 'm':
						case 'o':
						case 'z':
							++times;
							break;
					}
					break;
				case 'e':
					switch (::tolower(*(tmp+2)))
					{
						case 'c':
						case 'e':
						case 'g':
						case 'h':
						case 's':
						case 't':
							++times;
							break;
					}
					break;
				case 'f':
					switch (::tolower(*(tmp+2)))
					{
						case 'i':
						case 'j':
						case 'k':
						case 'm':
						case 'o':
						case 'r':
						case 'x':
							++times;
							break;
					}
					break;
				case 'g':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'b':
						case 'd':
						case 'e':
						case 'h':
						case 'i':
						case 'l':
						case 'p':
						case 'q':
						case 'f':
						case 'm':
						case 'n':
						case 'r':
						case 't':
						case 'u':
						case 'w':
						case 'y':
							++times;
							break;
					}
					break;
				case 'h':
					switch (::tolower(*(tmp+2)))
					{
						case 'k':
						case 'm':
						case 'n':
						case 'r':
						case 't':
						case 'u':
							++times;
							break;
					}
					break;
				case 'i':
					switch (::tolower(*(tmp+2)))
					{
						case 'd':
						case 'e':
						case 'l':
						case 'n':
						case 'o':
						case 'q':
						case 'r':
						case 's':
						case 't':
							++times;
							break;
					}
					break;
				case 'j':
					switch (::tolower(*(tmp+2)))
					{
						case 'm':
						case 'o':
						case 'p':
							++times;
							break;
					}
					break;
				case 'k':
					switch (::tolower(*(tmp+2)))
					{
						case 'e':
						case 'g':
						case 'h':
						case 'i':
						case 'm':
						case 'n':
						case 'p':
						case 'r':
						case 'w':
						case 'y':
						case 'z':
							++times;
							break;
					}
					break;
				case 'l':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'b':
						case 'c':
						case 'i':
						case 'k':
						case 'r':
						case 's':
						case 't':
						case 'u':
						case 'v':
						case 'y':
							++times;
							break;
					}
					break;
				case 'm':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'c':
						case 'd':
						case 'g':
						case 'h':
						case 'l':
						case 'm':
						case 'n':
						case 'o':
						case 'p':
						case 'q':
						case 'r':
						case 's':
						case 't':
						case 'u':
						case 'v':
						case 'w':
						case 'x':
						case 'y':
						case 'z':
							++times;
							break;
					}
					break;
				case 'n':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'c':
						case 'e':
						case 'f':
						case 'g':
						case 'i':
						case 'l':
						case 'o':
						case 'p':
						case 'r':
						case 't':
						case 'u':
						case 'z':
							++times;
							break;
					}
					break;
				case 'o':
					switch (::tolower(*(tmp+2)))
					{
						case 'm':
							++times;
							break;
					}
					break;
				case 'p':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'e':
						case 'f':
						case 'g':
						case 'h':
						case 'k':
						case 'l':
						case 'm':
						case 'n':
						case 't':
						case 'r':
						case 'w':
						case 'y':
							++times;
							break;
					}
					break;
				case 'q':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
							++times;
							break;
					}
					break;
				case 'r':
					switch (::tolower(*(tmp+2)))
					{
						case 'e':
						case 'o':
						case 'u':
						case 'w':
							++times;
							break;
					}
					break;
				case 's':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'g':
						case 'h':
						case 'i':
						case 'j':
						case 'k':
						case 'l':
						case 'm':
						case 'n':
						case 'o':
						case 'r':
						case 't':
						case 'u':
						case 'v':
						case 'y':
						case 'z':
							++times;
							break;
					}
					break;
				case 't':
					switch (::tolower(*(tmp+2)))
					{
						case 'c':
						case 'd':
						case 'f':
						case 'g':
						case 'h':
						case 'j':
						case 'k':
						case 'm':
						case 'n':
						case 'o':
						case 'p':
						case 'r':
						case 't':
						case 'v':
						case 'w':
						case 'z':
							++times;
							break;
					}
					break;
				case 'u':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'g':
						case 'k':
						case 'm':
						case 's':
						case 'y':
						case 'z':
							++times;
							break;
					}
					break;
				case 'v':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'c':
						case 'e':
						case 'g':
						case 'i':
						case 'n':
						case 'u':
							++times;
							break;
					}
					break;
				case 'w':
					switch (::tolower(*(tmp+2)))
					{
						case 'f':
						case 's':
							++times;
							break;
					}
					break;
				//case 'x':
				case 'y':
					switch (::tolower(*(tmp+2)))
					{
						case 'e':
						case 'u':
							++times;
							break;
					}
					break;
				case 'z':
					switch (::tolower(*(tmp+2)))
					{
						case 'a':
						case 'm':
						case 'r':
						case 'w':
							++times;
							break;
					}
					break;
			}
		}

		do {
			tmp = mrutils::strchrRev(tmp-1, '.', start);
			if (tmp == NULL)
				break;
		} while (--times > 0);

		if (tmp == NULL)
			tmp = start;
		else
			++tmp;
	} while (false);

	result.assign(tmp,end-tmp);
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

namespace {
    void getPrintURL(const char * url, char * buffer, int size) {
        mrutils::strncpy(buffer, url, size);

        if (strstr(url,"nytimes.com"))
		{
			if (!strstr(url,"pagewanted=all"))
			{
				if (strstr(url,"html?"))
				{
					sprintf(buffer
						,"%s&pagewanted=all"
						,url);
				}
				else
				{
					sprintf(buffer
						,"%s?pagewanted=all"
						,url);
				}
			}
        }

        else
			if (   strstr(url,"wired.com")
				)
			{
				if (0 != strcmp(url + strlen(url) - strlen("/all"), "/all"))
				{
					sprintf(buffer, "%s/all", url);
				}
			}

		
		
        else
        if (   strstr(url,"blogs.wsj.com")
           )
		{
			if (!strstr(url,"/tab/print"))
			{
				if (char * c = strchr(url,'?'))
				{
					strcpy(mrutils::strncpy(buffer,url,c - url),
							"/tab/print");
				}
				else
				{
					sprintf(buffer
							,"%s/tab/print"
							,url);
				}
			}
		}

        // else
        // if (   strstr(url,"appleinsider.com/")
        //    ) {
			// if (!strstr(url,"/print/"))
			// {
			// 	if (char *str = strstr(url,"/articles/"))
			// 	{
			// 		size_t const len = str - url;
			// 		memcpy(buffer,url,len);
			// 		memcpy(buffer + len, "/print/", strlen("/print/"));
			// 		strcpy(buffer + len + strlen("/print/"),
			// 			str + strlen("/articles/"));
			// 	}
			// }
        // }


        else
        if (   strstr(url,"thenation.com")
           ) {
			if (!strstr(url,"/print/"))
			{
				if (char * c = strstr(url,"thenation.com/"))
				{
					c += strlen("thenation.com/");
					sprintf(buffer,"http://www.thenation.com/print/%s",
							c);
				}
			}
        }

        else
        if (   strstr(url,"wsj.com") 
            || strstr(url,"wsjonline.com")
            || strstr(url,"barrons.com")
           ) {
			if (!strstr(url,"#printMode"))
			{
				sprintf(buffer
					,"%s#printMode"
					,url);
			}
        }

        else
        if (   strstr(url,"technologyreview.com")
           ) {
			if (!strstr(url,"printer_friendly"))
			{
				std::vector<std::string> parts;
				mrutils::split(&parts, url, '/');
				for (std::vector<std::string>::iterator it = parts.begin();
						it != parts.end(); ++it)
				{
					if (int num = atoi(it->c_str()))
					{
						sprintf(buffer
							,"http://www.technologyreview.com/"
							 "printer_friendly_article.aspx?id=%d"
							,num);
						break;
					}
				}
			}
        }

        else
        if (   strstr(url,"foreignpolicy.com") 
           ) {
			if (!strstr(url,"page=full"))
			{
				sprintf(buffer
					,"%s?page=full"
					,url);
			}
        }

        else
        if (   strstr(url,"guardian.co.uk") 
           ) {
			if (!strstr(url,"/print"))
			{
				sprintf(buffer
					,"%s/print"
					,url);
			}
        }

        else
        if (   strstr(url,"go.theregister.com/feed/")
           )
		{
            sprintf(buffer,"http://%s"
                ,strstr(url,"go.theregister.com/feed/")
				+ ctlen("go.theregister.com/feed/"));
        }

        else
        if (   strstr(url,"washingtonpost.com")
           ) {
			if (!strstr(url,"_print.html"))
			{
				size_t len = strlen(url);
				if (0 == strcasecmp(url + len - strlen(".html"),".html"))
				{
					char * p = mrutils::strncpy(buffer,url, len-strlen(".html"));
					strcpy(p,"_print.html");
				}
			}
        }

        else
        if (   strstr(url,"MoreintelligentlifeTotal")
           ) {
			if (!strstr(url,"page=full"))
			{
				sprintf(buffer
					,"%s?page=full"
					,url);
			}
        }

        else
        if (   strstr(url,"rollingstone.com") 
           ) {
			if (!strstr(url,"print=true"))
			{
				sprintf(buffer
					,"%s/print?print=true"
					,url);
			}
        }

        else
        if (   strstr(url,"observer.com") 
           ) {
			if (!strstr(url,"page=all"))
			{
				sprintf(buffer,"%s?page=all",url);
			}
        }

        else
        if (   strstr(url,"slate.com") 
           ) {
			if (!strstr(url,"/pagenum/all"))
			{
				const char * s = strstr(url,"?from=");
				if (s) {
					mrutils::strcpy(mrutils::strncpy(buffer,url,s - url)
						,"/pagenum/all/");
				} else {
					sprintf(buffer,"%s/pagenum/all/",url);
				}
			}
        }

        /*
        else
        if (   strstr(url,"businessweek.com/") 
           ) {
            char * p = strstr(url,"businessweek.com/");
            char * q = mrutils::strncpy(buffer, url, p - url);
            sprintf(q
                ,"businessweek.com/print/%s"
                ,strchr(p,'/')+1);
        }
        */

        else
        if (   strstr(url,"bbc.co.uk") 
           )
		   {
            mrutils::replaceCopy(buffer,url,"/hi/","/low/",size);
        }


        else
        if (   strstr(url,"newyorker.com" )
           ) {
			if (!strstr(url,"currentPage=all"))
			{
				sprintf(buffer
					,"%s?currentPage=all"
					,url);
			}
        }

        else
        if (   strstr(url,"seedmagazine.com" )
           ) {
			if (strstr(url,"/article/"))
			{
				const char * q = strstr(url,"article");
				char * p;
				if (q != NULL && NULL != (p = strchr(q,'/'))) {
					mrutils::strcpy(
						mrutils::strcpy(
							mrutils::strncpy(buffer,url,q-url)
						,"print")
					,p);
				}
			}
        }

        else
        if (   strstr(url,"vanityfair.com" )
           ) {
			if (!strstr(buffer,"printable=true"))
			{
				sprintf(buffer
					,"%s?printable=true"
					,url);
			}
        }

        else
        if (   strstr(url,"https://alumni.stanford.edu/get/page/career/view-job/?jid=" )
           ) {
			if (!strstr(url,"doLogin"))
			{
				sprintf(buffer
					,"https://alumni.stanford.edu/cas/service/doLogin?pg=yes&nu=https:**alumni.stanford.edu/get/page/career/view-job/?jid=%s&xt=bb"
					,url + 58);
			}
        }

        else
        if (   strstr(url,"nypost.com" )
           ) {
            const char * p = strstr(url,"/p/");
            if (p) {
                sprintf(buffer
                    ,"http://nypost.com/f/print/%s"
                    ,p+3);
            } 
        }

        else
        if (   strstr(url,"theatlantic.com" )
           ) {
			if (!strstr(url,"/print/"))
			{
				const char * p = strstr(url,"/doc/");
				if (p) {
					sprintf(buffer
						,"http://www.theatlantic.com/doc/print/%s"
						,p+5);
				} else {
					p = strstr(url,"/magazine/archive/");
					if (p) {
						sprintf(buffer
							,"http://www.theatlantic.com/magazine/print/%s"
							,p+18);
					}
				}
			}
        }

        else
        if (   strstr(url,"economist.com" )
           ) {
			if (!strstr(url,"/print"))
			{
				const char * p;
				if ((p = strstr(url,"story_id="))) {
					const int story = atoi(p+9);
					sprintf(buffer,"http://www.economist.com/node/%d/print"
						,story);
				}
			}
        }

        else
        if (   strstr(url,"reuters.com" )
           ) {
			if (!strstr(url,"?sp=true"))
			{
				sprintf(buffer
					,"%s?sp=true"
					,url);
			}
        }

        else
        if (   strstr(url,"forbes.com" )
           ) {
			if (!strstr(url,"_print"))
			{
				const char * p = strstr(url,".html");
				if (p) {
					std::string link = url;
					link.replace(strstr(link.c_str(),".html")-link.c_str(),1,"_print.");
					mrutils::strcpy(buffer,link.c_str());
					return;
				}
			}

			if (!strstr(url,"/print"))
			{
				if (*url)
					sprintf(buffer,"%s%s", url, *(url+strlen(url)-1) == '/' ? "print" : "/print");
			}

        }

        else
        if (   strstr(url,"democracyjournal.org" )
           ) {
			if (!strstr(url,"printfriendly"))
			{
				const char * p = strstr(url,"ID=");
				if (p) {
					sprintf(buffer
						,"http://www.democracyjournal.org/printfriendly.php?%s"
						,p);
				}
			}
        }

    }

    bool getAsMobile(const char * url) {
        if (   mrutils::stristr(url, "ft.com")
            || mrutils::stristr(url, "gizmodo.com")
            || mrutils::stristr(url, "csmonitor.com")
            || mrutils::stristr(url, "salon.com")
            || mrutils::stristr(url, "economist.com")
            || mrutils::stristr(url, "techcrunch.com")
            || mrutils::stristr(url, "nymag.com")
            || mrutils::stristr(url, "efinancialcareers.com")
            || mrutils::stristr(url, "247wallst.com")
            || mrutils::stristr(url, "aljazeera.net")
            || mrutils::stristr(url, "barrons.net")
            || mrutils::stristr(url, "newyorker.com")
            || mrutils::stristr(url, "stevenberlinjohnson.com")
            || mrutils::stristr(url, "gq.com")
            || mrutils::stristr(url, "rollingstone.com")
            || mrutils::stristr(url, "vanityfair.com")
            || mrutils::stristr(url, "nypost.com")
            || mrutils::stristr(url, "guardian.co.uk")
            || mrutils::stristr(url, "twitter.com")
            || mrutils::stristr(url, "theatlantic.com")
            || mrutils::stristr(url, "risk.net")
            || mrutils::stristr(url, "online.wsj.com/itp")
           ) {
            return false;

        } else return true;
    }

    void getCookies(const char * url, char * buffer, int size) {
        if (   strstr(url, "democracyjournal.org")
           ) {
            strncpy(buffer,"DEM_Journal=8bb56af8aazq5hr5", size);
        }
    }

    void getCookieFile(const char * /*url*/, char * /*buffer*/, int /*size*/) {
        /*
        if ( strstr(url,"alumni.stanford.edu")
            ){
            snprintf(buffer,size,"%s/edu.stanford.alumni.cookie"
                ,news::settings::baseDir);
        }
        */
    }

    void getReferer(const char * url, char * buffer, int size) {
        if (mrutils::stristr(url, "newyorker.com")) {
            strncpy(buffer,"http://www.newyorker.com",size);
        } else {
            strncpy(buffer,"https://www.google.com",size);
            // strncpy(buffer,"https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&ved=0CDEQqQIwAA&url=http%3A%2F%2Fonline.wsj.com%2Farticle%2FSB10001424052702304549504579318120467741330.html&ei=YuTVUpLTD6aqsQS-6YGABQ&usg=AFQjCNEyw1fBTx-2uwiQEOujR1epx15hzg&sig2=0RhNiBqBWwyirSZv66pQng&bvm=bv.59378465,d.cWc",size);
        }
    }

    void getPostFields(const char * url, char * buffer, int size) {
        if ( strstr(url,"https://alumni.stanford.edu/get/page/career/view-job/?jid=")
            ){
            snprintf(buffer,size,"username=mikerobe&password=mgj466a4&nu=https:**alumni.stanford.edu*get*page*career*view-job*?jid=%s"
                ,url + 58);
        }
    }



    /**
     * Simple clone of readability javascript
     */
	std::pair<char const *, char const *> getReadableURL(mrutils::XML *xml, _UNUSED char const * url)
    {
		std::pair<char const *, char const *> ret = std::make_pair(xml->sob, xml->eob);

        char *start = NULL;
        if (NULL == (start = strstr(xml->sob, "<h1"))
            && NULL == (start = strstr(xml->sob, "<p>"))
            )
        {
            start = xml->sob;
        } else
        {
            start = strchr(start,'<');
			ret.first = start;
        }

        char *end = strstr(ret.first,"id=\"footer");
        if (end)
        {
            end = mrutils::strchrRev(end, '<', start);
			ret.second = end;
        }

        return ret;
    }
}

char* news::url::setOptions(mrutils::curl::urlRequest_t *request,
    std::string const &url, char * buffer, int size)
{
    size_t used = 0;

    request->mobile = getAsMobile(url.c_str());

    *buffer = 0;
    getCookies(url.c_str(),buffer,size);
    request->cookieData = buffer;
    used = strlen(buffer) + 1; 
    buffer += used; size -= used;

    *buffer = 0;
    getCookieFile(url.c_str(),buffer,size);
    request->cookieFile = buffer;
    used = strlen(buffer) + 1; 
    buffer += used; size -= used;

    *buffer = 0;
    getPostFields(url.c_str(), buffer, size);
    request->postFields = buffer;
    used = strlen(buffer) + 1; 
    buffer += used; size -= used;

    *buffer = 0;
    getReferer(url.c_str(), buffer, size);
    request->referer = buffer;
    used = strlen(buffer) + 1; 
    buffer += used; size -= used;

    *buffer = 0;
    getPrintURL(url.c_str(), buffer, size);
    request->url = buffer; 
    used = strlen(buffer)+1;
    buffer += used; size -= used;
	
	if (request->url != url) // request->url is a string now...
	{
		request->referer = url.c_str();
	}

    return buffer;
}

std::pair<char const *, char const *> news::url::cropContent(
	const char * url, mrutils::XML *xml, int stopFD)
{
	std::pair<char const *, char const *> ret = std::make_pair(xml->sob, xml->eob);

    char *start = xml->sob, *_start = xml->sob;
    char *end = xml->eob, *_end = xml->eob;

    if (strstr(url,"nytimes.com")) {
                           start = strstr(_start,"<div class=\"timestamp\"");
        if (start == NULL) start = strstr(_start,"<div id=\"content\"");
        if (start == NULL) start = strstr(_start,"<div id=\"article\"");
        if (start == NULL) start = strstr(_start,"<h2 class=\"entry-title\"");
        if (start == NULL) start = strstr(_start,"<div class=\"entry-content\"");
        if (start == NULL) start = strstr(_start,"<span class='pubdate'");
        if (start == NULL) start = _start;

                         end = strstr(start,"</NYT_TEXT>");
        //if (end == NULL) end = strstr(start,"<ul class=\"entry-tools\"");
        if (end == NULL) end = strstr(start,"<!-- end .entry-content -->");
    }

    else
    if (   strstr(url,"video.cnbc.com")
        || (strstr(url,"cnbc.com") && strstr(url,"?video="))
       )
   {

        if (char * p = strstr(xml->sob, "<meta name=\"transcript"))
        {
            if (NULL == (p = strstr(p,"content=")))
                return ret;
            xml->sob = p + strlen("content='");
            xml->eob = strchr(xml->sob,'"');
            *xml->eob = '\0';
            return ret;
        }

    } else if (   strstr(url,"wsj.com")
            || strstr(url,"wsjonline.com")
            || strstr(url,"barrons.com")
       ) {

        if (strstr(_start, "name=\"article.template\" content=\"snippet\""))
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::informational,
                "getting google cache for ",url);
			#endif

            size_t len = strlen(url);

            // if the url has ?... form data at the end, get rid of it
            char const *formData = mrutils::strchrRev(url + len, '?', url);
            char const *lastSlash = mrutils::strchrRev(url + len, '/', url);

            char const *endURL = url + len;

            if (formData && lastSlash && (formData - url) > (lastSlash - url))
                endURL = formData;

            // copy google cache address to xml buffer
            {
                xml->eob = mrutils::strcpy(xml->sob,"http://webcache.googleusercontent.com/search?q=cache:");
                char *p = strstr(url,"//")+strlen("//");
                memcpy(xml->eob, p, endURL - p);
                xml->eob += endURL - p;
                *xml->eob = '\0';
            }

            // use a proxy
            {
                char const *str = strchr(xml->sob,':');
                mrutils::stringstream s1, s2;

                char buffer[32];
                if (!mrutils::Encoder::encodeStrTo(str, s1, mrutils::encoding::EC_BASE64, buffer,
                    sizeof(buffer), 0)
                    || 
                    !mrutils::Encoder::encodeStrTo(s1.c_str(), s2, mrutils::encoding::EC_URL, buffer,
                    sizeof(buffer), 0))
                {
                    xml->eob = xml->sob;
                    *xml->eob = '\0';
                    return ret;
                }

                sprintf(xml->sob, "http://bigzh.com/browse.php?u=%s&b=5&f=norefer",
                    s2.c_str());
                mrutils::curl::urlRequest_t request(xml->sob);

                if (0 != delayDomain(request.url, stopFD) || 0 != xml->get(&request, stopFD))
                {
                    xml->eob = xml->sob;
                    *xml->eob = '\0';
                    return ret;
                }

                if (strstr(xml->sob,"The requested resource could not be loaded because the server returned an error:"))
                {
                    xml->eob = xml->sob;
                    *xml->eob = '\0';
                    return ret;
                }
            }
        } else if (char const *redirect = strstr(_start,"<meta http-equiv=\"Refresh\""))
        { // then get the redirect URL instead
            if ((redirect = strstr(redirect,"http:")))
            {
                char const *p = strchr(redirect,'"');
                memcpy(xml->sob, redirect, p - redirect);
                xml->sob[p-redirect] = '\0';

				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::informational,
                        "getting wsj redirect for [",xml->sob,"]");
				#endif

                mrutils::curl::urlRequest_t request(xml->sob);

                if (0 != delayDomain(request.url, stopFD) || 0 != xml->get(&request, stopFD))
                {
                    xml->eob = xml->sob;
                    *xml->eob = '\0';
                    return ret;
                }
            }
        }

                           start = strstr(_start,"<div id=\"article_story_body");
        if (start == NULL) start = strstr(_start,"<h2 class=\"postTitle");
        if (start == NULL) start = strstr(_start,"<div class=\"article story");
        if (start == NULL) start = strstr(_start,"<div class=\"postContent");
        if (start == NULL) start = strstr(_start,"<div id=\"ArticleContent");
        if (start == NULL) start = _start;

                         end = strstr(start,"<div id=\"article_pagination_bottom");
        if (end == NULL) end = strstr(start,"<div class=\"bottomRow");
        if (end == NULL) end = strstr(start,"<small class=\"copyright");
        if (end == NULL) end = strstr(start,"<div class=\"popupType-social");
    }

    else
    if (   strstr(url,"allthingsd.com") ) {
        start = strstr(_start,"<div class=\"entry");
        if (start == NULL) start = _start;
        end = strstr(start,"<div class=\"ads\">");
    }

    else
    if (   strstr(url,"bbc.co.uk")
       ) {
                           start = strstr(_start,"<a name=\"startcontent\"");
    }

    else
    if (   strstr(url,"theregister.co.uk")
       ) {
                           start = strstr(_start,"<div id=\"article");
    }

    else
    if (   strstr(url,"www.nybooks.com")
       ) {
                           start = strstr(_start,"<div class=\"article-text");
    }

    else
    if (   strstr(url,"newyorker.com")
       ) {
                           start = strstr(_start,"<h1 id=\"articlehead\"");
        if (start == NULL) start = strstr(_start,"<div id=\"articleheads\"");
        if (start == NULL) start = strstr(_start,"<h3 class=\"entry-title\"");
        if (start == NULL) start = strstr(_start,"<div id=\"articletext\"");
    }

    else
    if (   strstr(url,"theatlantic.com")
       ) {
        if (NULL == (start = strstr(_start,"<h1 class=\"headline\""))
            && NULL == (start = strstr(_start,"<div class=\"articleHead"))
            ) start = _start;
    }

    else
    if (   strstr(url,"thedailybeast.com")
       ) {

        if (NULL != (start = strstr(_start,"name=\"skipTocontent\""))) {
            start = strchr(start,'>');
            if (start) ++start;
        } else {
            start = strstr(_start,"<div class=\"title\">");
        }

        if (start != NULL) {
            end = strstr(start,"<div id=\"clear\"");
        }
    }

    else
    if (   strstr(url,"reuters.com")
       ) {
                           start = strstr(_start,"<div class=\"article primaryContent");
        if (start == NULL) start = strstr(_start,"<span id=\"articleText");
        if (start == NULL) start = strstr(_start,"<div id=\"postcontent");
        if (start == NULL) start = _start;
                         end = strstr(start,"<div id=\"articleFooter");
    }

    else
    if (   strstr(url,"portfolio.com")
       ) {
                           start = strstr(_start,"<div id=\"content\"");
    }

    else
    if (   strstr(url,"appleinsider.com")
       ) {
        if ((NULL == (start = strstr(_start,"<div class=\"article\">")))
            ) start = _start;
        if (NULL == (end = strstr(start, "<!-- article, END -->"))
           ) end = _end;
    }

    else
    if (   strstr(url,"thenation.com")
       ) {
                           start = strstr(_start,"<div class=\"main\"");
    }

    else
    if (   strstr(url,"gizmodo.com")
       ) {

                           start = strstr(_start,"<div class=\"entry\"");
        if (start == NULL) start = strstr(_start,"<!-- google_ad_section_start -->");
        if (start == NULL) start = _start;

                         end = strstr(start,"<a id=\"viewcomments\"");
    }

    else
    if (   strstr(url,"npr.org")
       ) {
        start = strstr(_start,"<div class=\"storytitle\"");
    }

    else
    if (   strstr(url,"craigslist.org")
       ) {
        start = strstr(_start,"<h2");
    }

    else
    if (   strstr(url,"dailyspeculations.com")
       ) {
        start = strstr(start,"<div id=\"contenttitle\"");
    }

    else
    if (   strstr(url,"guardian.co.uk")
       ) {
                           start = strstr(_start,"<div id=\"main-article-info");
        if (start == NULL) start = strstr(_start,"<h1");
        if (start == NULL) start = _start;
                         end = strstr(start, "<div id=\"related\"");
    }

    else
    if (   strstr(url,"nymag.com")
       ) {
                           start = strstr(_start,"<h1>");
        if (start == NULL) start = strstr(_start,"<div id=\"story\"");
        if (start == NULL) start = strstr(_start,"<h2 class=\"entry-title\"");
        if (start == NULL) start = strstr(_start,"<div class=\"entry-header\"");
        if (start == NULL) start = strstr(_start,"<div id=\"entry-header\"");
        if (start == NULL) start = _start;

        if (NULL == (end = strstr(start, "<!-- /end #story -->"))
            && NULL == (end = strstr(start, "<div class=\"page-navigation\""))
            && NULL == (end = strstr(start, "<div class=\"recommend\""))
           ) end = _end;
    }

    else
    if (   strstr(url,"ft.com")
       ) {
                           start = strstr(_start,"<div class=\"fullstory");
        if (start != NULL) {
            char *const tmp=start;
            if (NULL == (start = strstr(start,"<p class=\"lastUpdated")))
                start = tmp;
        } else {
            if (start == NULL) start = strstr(_start,"<div class=\"article");
            if (start == NULL) start = strstr(_start,"<a name=\"access-content\"");
            if (start == NULL) start = strstr(_start,"<h2>");
            if (start == NULL) start = _start;
        }
        if (NULL == (end = strstr(start,"<p class=\"screen-copy"))
            && NULL == (end = strstr(start,"<span class=\"copyright"))
            ) end = _end;
    }

    else
    if (   strstr(url,"csmonitor.com")
       ) {
                           start = strstr(_start,"<!--startclickprintinclude");
        if (start == NULL) start = strstr(_start,"<p class=\"sByline\"");
    }

    else
    if (   strstr(url,"economist.com")
       ) {
        start = strstr(_start,"<p class=\"fly-title\"");
        if (start == NULL) start = strstr(_start,"<div class=\"headline");
        if (start == NULL) start = strstr(_start,"<h2 class=\"ec-blog-fly-title");
        if (start == NULL) start = strstr(_start,"<div id=\"ec-article\"");
        if (start == NULL) start = _start;

        end = strstr(start,"<div id=\"article-region-bottom");
    }

    else
    if (   strstr(url,"businessweek.com")
       ) {
                           start = strstr(_start,"<!--STORY-->");
        if (start == NULL) start = strstr(_start,"<div id=\"storyBody\"");
        if (start == NULL) start = strstr(_start,"<div id=\"story-tools\"");
        if (start == NULL) start = strstr(_start,"<!-- Story Tools Ends -->");
        else start = strstr(start,"<p");
        if (start == NULL) start = strstr(_start,"<div id=\"main\"");
        if (start == NULL) start = _start;

        if (NULL == (end = strstr(start,"<!--/STORY"))
            && NULL == (end = strstr(start, "<div id=\"pageNav"))
            && NULL == (end = strstr(start, "<!-- Sponsored"))
            && NULL == (end = strstr(start, "<!-- Sponsered"))
            && NULL == (end = strstr(start, "<hr class=\"clearFloat"))
            )
            end = _end;
    }

    else
    if (   strstr(url,"sloanreview.mit.edu")
       ) {
                           start = strstr(_start,"<div class=\"article\"");
        if (start == NULL) start = _start;

        end = strstr(start, "<a name=\"comments\"");
    }

    else
    if (   strstr(url,"247wallst.com")
       ) {
                           start = strstr(start,"<h1 class=\"entry-title\"");
        if (start == NULL) start = _start;
        end = strstr(start, "<div id=\"social_icons\"");
    }

    else
    if (   strstr(url,"bloomberg.com")
       ) {
                           start = strstr(_start,"<div class=\"component\" id=\"story\">");
        if (start == NULL) start = strstr(_start,"<h1>");
        if (start == NULL) start = strstr(_start,"<div class=\"first-paragraph\">");
        if (start == NULL) start = strstr(_start,"<p>");
        if (start == NULL) start = _start;
    }

    else
    if (   strstr(url,"globeandmail.com")
       ) {
                           start = strstr(start,"<div id=\"article-content\"");
        if (start == NULL) start = _start;
                         end = strstr(start, "<div id=\"article-relations\"");
        if (end == NULL) end = strstr(start, "<div id=\"latest");
    }

    else
    if (   strstr(url,"salon.com")
       ) {
        start = strstr(start,"<h1 class=\"headline\"");
    }

    else
    if (   strstr(url,"atlantic.com")
       ) {
        start = strstr(start,"<p class=\"entry-top\"");
        if (start == NULL) start = strstr(_start,"<h1 class=\"blogEntryTitle\">");
    }

    else
    if (   strstr(url,"wired.com")
       ) {
        start = strstr(start,"<div id=\"content\"");
    }

    else
    if (   strstr(url,"atlanticwire.com")
       ) {
        if (NULL == (start = strstr(_start,"<div id=\"Contents\""))
            && NULL == (start = strstr(_start,"<div class=\"content"))
            ) start = _start;
        if (NULL == (end = strstr(start,"<div class=\"share-foot"))
            ) end = _end;
    }

    else
    if (   strstr(url,"politico.com")
       ) {
        if (NULL != (start = strstr(_start,"class=\"bylineLink\""))) {
            start = strchr(start,'>')+1;
        } else  start = _start;
        if (NULL == (end = strstr(start,"<div class=\"share-foot"))
            ) end = _end;
    }

    else
    if (   strstr(url,"observer.com")
       ) {
        if (NULL == (start = strstr(_start,"<div class=\"entry-content\">"))
            && NULL == (start = strstr(_start,"<div id=\"article_container\""))
            ) start = _start;
    }

    else
    if (   mrutils::stristr(url,"hbr.org")
       ) {
        if (NULL == (start = strstr(_start,"<div id=\"articleBody\">"))
            ) start = _start;
        if (NULL == (end = strstr(start,"<div id=\"articleFooter\">"))
            ) end = _end;
    }

    else
    if (   mrutils::stristr(url,"arstechnica.com")
       ) {
        if ((NULL == (start = strstr(_start,"<div id=\"content"))
                && NULL == (start = strstr(_start,"<h2")))
            ) start = _start;
        if (
				NULL == (end = strstr(start,"<nav class=\"page-numbers"))
				&& NULL == (end = strstr(start,"<div id=\"footer"))
            ) end = _end;
    }

    /*
    else
    if (   mrutils::stristr(url,"washingtonpost.com")
       ) {
        if ((NULL == (start = strstr(_start,"<div id=\"article"))
                && NULL == (start = strstr(_start,"<h1")))
            ) start = _start;
        if (NULL == (end = strstr(start,"<br clear=\"all\""))
            ) end = _end;
    }
    */

    else
    if (   mrutils::stristr(url,"kedrosky.com")
       ) {
        if ((NULL == (start = strstr(_start,"<h1 class=\"entry-title"))
                && NULL == (start = strstr(_start,"<h1")))
            ) start = _start;
        if (NULL == (end = strstr(start,"<div id=\"comments"))
            ) end = _end;
    }

    else
    if (   mrutils::stristr(url,"risk.net")
       ) {
        if (NULL == (start = strstr(_start,"<div class=\"main-story-block"))
                && NULL == (start = strstr(_start,"<h1"))
            ) start = _start;
        if (NULL == (end = strstr(start,"<div class=\"topics"))
            ) end = _end;
    }

    else
    if (   mrutils::stristr(url,"vanityfair.com")
       ) {
        if (NULL == (start = strstr(_start,"<h1 class=\"headline"))
            ) start = _start;
        if (NULL == (end = strstr(start,"<h4>Post a Comment"))
            ) end = _end;
    }

    else
    if (   mrutils::stristr(url,"cnn.com")
       ) {
        if (NULL == (start = strstr(_start,"<h1"))
            ) start = _start;
        if (NULL == (end = strstr(start,"<div id=\"pagefoot"))
            ) end = _end;
    }


    else
    if (   mrutils::stristr(url,"chicagobooth.edu")
       ) {
        if ((NULL == (start = strstr(_start,"<div id=\"content\">"))
                && NULL == (start = strstr(_start,"<h1")))
            ) start = _start;
        if (NULL == (end = strstr(start,"<div id=\"footer\">"))
            ) end = _end;
    }

    else
    if (   mrutils::stristr(url,"forbes.com")
       ) {
        if ((NULL == (start = strstr(_start,"<h1 class=\"post-title\""))
                && NULL == (start = strstr(_start,"<div class=\"post\"")))
            ) start = _start;
        if (NULL == (end = strstr(start,"<div class=\"post-meta\""))
            ) end = _end;
    }

    else
    if (   mrutils::stristr(url,"efinancialcareers.com")
       ) {
        start = strstr(start,"<div id=\"efcContentLayoutMiddleCol1\">");
        end = strstr(_start,"<div id=\"efcFooter\">");
    }

    else
    if (   mrutils::stristr(url,"overheardinnewyork.com")
       ) {
        start = strstr(start,"<h3 class=\"title\">");
    }

    else
    if (   mrutils::stristr(url,"project-syndicate.org")
       ) {
        start = strstr(start,"<div class=\"distribution_date\"");

        if (NULL != start) {
            end = strstr(start,"<p class=\"copyright\"");
        }
    }

    else
    if (   mrutils::stristr(url,"aljazeera.net")
       ) {
        start = strstr(start,"<span class=\"DetaildSuammary\"");
        if (start == NULL) start = strstr(_start,"<td class=\"DetailedSummary\"");
        if (start == NULL) {
            if (NULL != (start = strstr(_start,"class=\"DetailedSummary\""))) {
                start = mrutils::strchrRev(start,'<',_start);
            }
        }
        if (start != _start && start != NULL) {
            end = strstr(start,"<div id=\"dvToolsList\">");
        }
    }

    else
    if (   mrutils::stristr(url,"alumni.stanford.edu")
       ) {
        start = strstr(start,"<div id=\"middle\">");
    }

    else
    if (   mrutils::stristr(url,"pulsd.com")
       ) {
        start = strstr(start,"<h1 class='main-pulse-title'");
    }

    else
    if (   mrutils::stristr(url,"fastcompany.com")
       ) {
        start = strstr(start,"<h2 id=\"hdr_article-headline\">");
    }

    else
    if (   mrutils::stristr(url,"TheParisReviewBlog")
       ) {
        start = strstr(start,"<div id=\"content");
    }

    else
    if (   strstr(url,"MarketFolly")
       ) {
        start = strstr(start,"<h3 class='post-title");
        if (start == NULL) start = strstr(_start,"<h2 class='date-header");
    }

    else
    if (   strstr(url,"/zerohedge/") || strstr(url,"zerohedge.com")
       ) {
        if (NULL == (start = strstr(_start,"<h1 class=\"title\""))
            && NULL == (start = strstr(_start,"<div id=\"inner-content"))
            ) start = _start;
        if (NULL == (end = strstr(start,"<span class=\"user-rating"))
            ) end = _end;
    }

    else
    if (   mrutils::stristr(url,"timeout.com") || mrutils::stristr(url,"timeoutny.com")
       ) {
        start = strstr(start,"<div class=\"entry\"");
        if (start == NULL) start = strstr(_start,"<div class=\"MD_article\"");
    }

    else
    if (   mrutils::stristr(url,"foreignpolicy.com")
       ) {
        start = strstr(start,"<div id=\"art-mast\">");
        if (start == NULL) start = strstr(_start,"<div class=\"translateHead\">");
    }

    else
    if (   mrutils::stristr(url,"nypost.com")
       ) {
        start = strstr(start,"<h1");
        end = strstr(_start,"<div id=\"story_tags\"");
    }

    else
    if (   mrutils::stristr(url,"dealbreaker.com")
       ) {
        start = strstr(start,"<h1 class=\"postTitle\"");
        if (start == NULL) start = strstr(_start,"<div class=\"entry\"");

        if (start != NULL) {
            end = strstr(start,"<div class=\"postFooter\">");
        }
    }

    else
    if (   mrutils::stristr(url,"distressed-debt-investing.com")
       ) {
        start = strstr(start,"<div class='post'");
        if (start == NULL) start = strstr(_start,"<div class=\"post\"");
    }

    if (start == NULL || start == _start)
    {
        return getReadableURL(xml, url);
    } else
    {
        if (start == NULL) start = _start;
        if (end   == NULL) end   = _end;
        // *end = 0;

		return std::make_pair(start, end);
    }
}

bool news::url::keepItem(const Item& item) {
    if (strstr(item.link.c_str(),"pulsd.com/")) {
        return strstr(item.link.c_str(),"/new-york/");
    }

    return true;
}

void news::url::updateFeedUrl(std::string *urlStr, char *buffer)
{
    char const *url = urlStr->c_str();

    if (char const *p=strstr(url,"online.wsj.com/itp/")
       ) {
        p += strlen("online.wsj.com/itp/");
        char *q = mrutils::strncpy(buffer,url,p-url);
        while (::isdigit(*p))
            ++p;
        if (*p == '/')
        {
            sprintf(q,"%d%s",mrutils::getDate(), p);
        }

		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::informational,
                "updated WSJ url from [",
                urlStr->c_str(),"] to [",
                buffer,"]");
		#endif
        urlStr->assign(buffer);
    }
}

std::string news::url::getNextURL(std::string const &urlStr, char const *prevPage)
{
    char const * const url = urlStr.c_str();
    size_t len = urlStr.length();

    if (mrutils::stristr(url,"nymag.com"))
    {
        if (char *p = strchr(url, '?'))
        {
            len = p - url;
        }

        if (0 == strncasecmp(url + len - strlen(".html"),".html",5))
        {
            size_t pos = urlStr.find_last_of('/');
            if (std::string::npos == pos)
                return "";

            if (0 != strncmp(url + pos + 1, "index", 5))
                return "";

            int const index = atoi(url + pos + strlen("index") + 1) + 1;

            std::stringstream ss;
            ss.write(url, pos+1);
            ss << "index" << index << ".html";
            return ss.str();
        } else
        {
            if (0 == strcmp(urlStr.c_str() + urlStr.size() - sizeof("/index1.html")+1, "/index1.html"))
                return "";
            return urlStr + "/index1.html";
        }
    } else if (mrutils::stristr(url,"cnn.com"))
    {
        if (strstr(url,"/?"))
        {
            size_t pos = urlStr.find_last_of('/');
            if (std::string::npos == pos)
                return "";

            std::stringstream ss;
            ss.write(url, pos+1);
            ss << "index2.htm";
            return ss.str();
        }

        if (0 == strcasecmp(url + len - strlen(".htm"),".htm"))
        {
            size_t pos = urlStr.find_last_of('/');
            if (std::string::npos == pos)
                return "";

            int index = atoi(url + pos + strlen("index") + 1) + 1;
            if (0 == strcasecmp(url + len - strlen("index.htm"),"index.htm"))
                ++index;

            std::stringstream ss;
            ss.write(url, pos+1);
            ss << "index" << index << ".htm";
            return ss.str();
        } else
        {
            return urlStr + "/index2.htm";
        }
    } else if (mrutils::stristr(url,"businessweek.com"))
    {
        if (0 == strcasecmp(url + len - strlen(".htm"),".htm"))
        {
            size_t posSlash = urlStr.find_last_of('/');
            if (std::string::npos == posSlash)
                return "";

            char const *page = mrutils::strstrRev(url + len -1, "_page_", url);
            if (!page || page - url < (int)posSlash)
            {
                std::stringstream ss;
                ss.write(url, urlStr.size() - strlen(".htm"));
                ss << "_page_2" ".htm";
                return ss.str();
            }

            size_t posPage = page - url;

            int index = atoi(url + posPage + strlen("_page_")) + 1;
            std::stringstream ss;
            ss.write(url, posPage);
            ss << "_page_" << index << ".htm";
            return ss.str();
        } else
        {
            return "";
        }
    }
	else if (mrutils::stristr(url,"arstechnica.com"))
	{
		try
		{
			news::url::URLParser p(url);
			if (!p.path.levels.size())
				return "";

			int current = 1;
			if (!p.path.endsInDir())
			{
				current = 0;
				bool isNumeric = true;

				// check that it's numeric
				std::string const &last = p.path.levels.back();
				for (std::string::const_iterator it = last.begin();
					 it != last.end(); ++it)
				{
					if (!std::isdigit(*it))
					{
						isNumeric = false;
						break;
					}
					current *= 10;
					current += *it - '0';
				}

				if (isNumeric)
					p.path.levels.pop_back();
				else
					current = 1;
			}
			else
			{
				p.path.levels.pop_back();
			}

			char buffer[12];
			sprintf(buffer,"%d",current+1);
			p.path.levels.push_back(buffer);
			std::string result = p.formURL();

			if (NULL != strstr(prevPage, result.c_str()))
				return result;
			else
				return "";
		}
		catch (std::exception const &e)
		{
			#ifdef HAVE_PANTHEIOS
			pantheios::log(pantheios::error,
						   "error parsing url[",url,
						   "]: [", e, "]");
			#endif
			return "";
		}
	}
    /* Old washington post before _pf.html extension
    } else if (mrutils::stristr(url, "washingtonpost.com"))
    {
        if (0 == strcasecmp(url + len - strlen(".html"),".html"))
        {
            size_t pos1 = urlStr.find_last_of('/');
            if (std::string::npos == pos1)
                return "";

            std::stringstream ss;
            ss.write(url, ++pos1);

            size_t pos2 = urlStr.find_last_of('_');
            if (std::string::npos == pos2 || pos2 < pos1)
            {
                ss.write(url + pos1,urlStr.size() - pos1-strlen(".html"));
                ss << "_2.html";
            } else
            {
                ss.write(url+pos1,pos2-pos1+1);
                int const index = atoi(url+pos2+1) + 1;
                ss << index << ".html";
            }

            return ss.str();
        }
    }
    */

    return "";
}

int news::url::delayDomain(std::string const &url, int stopFD)
{
    static mrutils::mutex_t mutex = mrutils::mutexCreate();
    static std::map<std::string, time_t> domainToTime;

    std::string domain;
    time_t waitSecs;

    {
        mrutils::mutexScopeAcquire l(mutex);

        time_t now;
        ::time(&now);

        domain = getDomain(url);
        std::map<std::string, time_t>::iterator it = domainToTime.find(domain);
        if (domainToTime.end() == it)
        {
            it = domainToTime.insert(std::pair<std::string, time_t>(domain, now)).first;
            mrutils::mutexRelease(mutex);
            return 0;
        }

        if (it->second <= now)
            it->second = now;
        it->second +=
            strstr(domain.c_str(),"google") ? 30
            : news::settings::domainWaitSeconds;

        waitSecs = it->second - now;
    }

    if (stopFD <= 0)
        return 0;

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            "waiting for [",pantheios::integer(waitSecs),
            "] seconds on [", domain, "]");
	#endif

    TIMEVAL tv = { waitSecs, 0 };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(stopFD,&fds);
    return select(stopFD+1,&fds,NULL,NULL,&tv);
}
