#include "item.h"
#include "mr_time.h"
#include "mr_files.h"
#include "news.h"
#include "feed.h"
#include "url.h"
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <sys/syslimits.h>
#include "md5.h"
#include "html.h"
#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
	#include <pantheios/inserters.hpp>
#endif

bool news::Item::clearContent(char *buffer, bool lock)
{
    if (lock) mrutils::mutexAcquire(mutexContent);
        sprintf(buffer,"%s%d/%d-%02d-%02d/%d",
                news::settings::contentDir,
                this->date / 10000,
                this->date / 10000,
                this->date / 100 % 100,
                this->date % 100,
                this->id);
        mrutils::rmR(buffer);
    if (lock) mrutils::mutexRelease(mutexContent);

    if (isGone) return false;
    return true;
}

void news::Item::findImages(char * start, const std::string& link, std::vector<news::Item::imgData_t>& imgData, std::set<mrutils::curl::urlRequestM_t>& imgRequests) {
    for (char * p = start; NULL != (p = strstr(p,"<img "));p += ctlen("<img ")) {
        if ((int)imgRequests.size() >= news::settings::maxImages) break;
        char * q = strchr(p,'>'); if (q == NULL) continue;
        *q = '\0';
        char * s = p;
        for (;NULL != (s = strstr(s," src"));) {
            for (s += ctlen(" src"); *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r';++s) ;
            if (*s == '=') { s = strchr(s,'"'); break; }
        } *q = '>'; if (s == NULL) continue; ++s;

        // URL characters only
        for (q = s;;++q) if (*q == ' ') *q = '+'; else if (!mrutils::isURLCharacter(*q)) break;
        if (*q != '"') continue;

        *q = '\0';
        mrutils::curl::urlRequestM_t request;
        request.url = mrutils::formURL(link.c_str(),s);
        *q = '"';

        q = strchr(q,'>'); if (q == NULL) continue;

        imgData.push_back(imgData_t(p,q+1,imgRequests.insert(request).first));
    }
}

bool news::Item::saveImages(char * buffer,
    std::set<mrutils::curl::urlRequestM_t>& imgRequests, int stopFD)
{
    static mrutils::mutex_t mutexImages(mrutils::mutexCreate());

    if (!imgRequests.empty())
    {
        std::vector<mrutils::curl::curlDataM_t> data;
        data.resize(imgRequests.size());
        mrutils::curl::getMultiple(imgRequests,&data,stopFD);

        // save each one with non-empty data
        for (unsigned i = 0; i < data.size(); ++i)
        {
            if (0 == data[i].content.tellp())
                continue;

            char tmppath[1024];
            sprintf(tmppath,"%stmp.XXXX",news::settings::contentDir);
            int tmpfd = mkstemp(tmppath);
            if (tmpfd == -1)
                continue;
            close(tmpfd);

            std::string content = data[i].content.str();
            std::string extension = "png";

            // convert gifs & unknown images to png
            if (mrutils::startsWith(data[i].content.c_str(),"GIF")
                ||!mrutils::ImageDecode::knownFormat(data[i].content.c_str())
                )
            {
				// UPDATE: not any more  -- just do nothing
				continue;

                // if (mrutils::startsWith(data[i].content.c_str(),"GIF"))
                //     sprintf(buffer,"%s - png:- > %s",news::settings::imgConvert,tmppath);
                // else
                //     sprintf(buffer,"%s - jpg:- > %s",news::settings::imgConvert,tmppath);

                // if (FILE * fp = MR_POPEN(buffer,"w"))
                // {
                //     fwrite(data[i].content.c_str(),1,data[i].content.str().size(),fp);
                //     MR_PCLOSE(fp);
                // } else
                // {
                //     mrutils::rm(tmppath);
                //     pantheios::logprintf(pantheios::error,
                //             "%s %s:%d can't open pipe to [%s]",
                //             __PRETTY_FUNCTION__,__FILE__,__LINE__,
                //             buffer);
                //     return false;
                // }

                // std::ifstream in(tmppath, std::ios::in);
                // content = static_cast<std::stringstream const*>(&(std::stringstream() << in.rdbuf()))->str();

                // if (content.empty())
                // {
                //     std::set<mrutils::curl::urlRequestM_t>::iterator it
                //         = imgRequests.begin();
                //     for (unsigned j = 0; j < i; ++j)
                //         ++it;
                //     pantheios::logprintf(pantheios::error,
                //             "%s %s:%d can't convert image at [%s] "
                //             "used in request for page [%s]",
                //             __PRETTY_FUNCTION__,__FILE__,__LINE__,
                //             it->url.c_str(), link.c_str());
                //     mrutils::rm(tmppath);
                //     continue;
                // }
            } else
            {
                switch (mrutils::ImageDecode::getFormat(content.c_str()))
                {
                    case mrutils::ImageDecode::AUTO:
                        mrutils::rm(tmppath);
                        {
                            std::set<mrutils::curl::urlRequestM_t>::iterator it
                                = imgRequests.begin();
                            for (unsigned j = 0; j < i; ++j)
                                ++it;
							#ifdef HAVE_PANTHEIOS
                            pantheios::logprintf(pantheios::error,
                                    "%s %s:%d unknown image type at [%s] "
                                    "used in request for page [%s]",
                                    __PRETTY_FUNCTION__,__FILE__,__LINE__,
                                    it->url.c_str(), link.c_str());
							#endif
                        }
                        continue;
                    case mrutils::ImageDecode::JPG:
                        extension = "jpeg";
                        break;
                    case mrutils::ImageDecode::PNG:
                        extension = "png";
                        break;
                }

                int tmpfd = open(tmppath,O_WRONLY);
                if (tmpfd < 0)
                {
                    mrutils::rm(tmppath);
					#ifdef HAVE_PANTHEIOS
                    pantheios::logprintf(pantheios::error,
                            "%s %s:%d can't re-open [%s] ",
                            __PRETTY_FUNCTION__,__FILE__,__LINE__,
                            tmppath);
					#endif
                    return false;
                }
                write(tmpfd,content.c_str(), content.size());
                close(tmpfd);
            }

            mrutils::mutexAcquire(mutexImages);
                std::string imgPath;
                bool imgExists = false;
                findImg(&imgPath, &imgExists, content, extension, buffer);
                if (!imgExists)
                {
                    sprintf(buffer,"gzip -9c %s > %s",
                        tmppath,
                        imgPath.c_str());
                    system(buffer);
					#ifdef HAVE_PANTHEIOS
                    pantheios::log(pantheios::informational,
                            __PRETTY_FUNCTION__, " new image at [",
                            imgPath,"]");
					#endif
                }
            mrutils::mutexRelease(mutexImages);

            mrutils::rm(tmppath);
            sprintf(buffer,"%s%d/%d-%02d-%02d/%d/%d.gz",
                    news::settings::contentDir,
                    this->date / 10000,
                    this->date / 10000,
                    this->date / 100 % 100,
                    this->date % 100,
                    this->id,
                    i);

            imgPath = mrutils::makeRelativePath(imgPath.c_str(), buffer);

            if (0 != symlink(imgPath.c_str(), buffer))
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::logprintf(pantheios::error,
                        "%s %s:%d error creating symlink with [%s] [%s] [%s]",
                        __PRETTY_FUNCTION__,__FILE__,__LINE__,
                        imgPath.c_str(),buffer,strerror(errno));
				#endif
            }
        }
    }

    return true;
}

bool news::Item::saveContent(mrutils::XML& xml, char * buffer,
    const std::vector<imgData_t>& imgData)
{
    if (xml.eob == xml.sob)
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::logprintf(pantheios::notice,
                "%s %s:%d empty content at [%s]",
                __PRETTY_FUNCTION__,__FILE__,__LINE__,
                link.c_str());
		#endif
        return false;
    }

    sprintf(buffer,"%s | gzip -9c > %s%d/%d-%02d-%02d/%d/article.txt.gz",
            news::settings::dump,
            news::settings::contentDir,
            this->date / 10000,
            this->date / 10000,
            this->date / 100 % 100,
            this->date % 100,
            this->id);

    if (FILE * fp = MR_POPEN(buffer,"w"))
    {
        char * p = xml.sob;
        for (size_t i = 0; i < imgData.size() && p < xml.eob; ++i)
        {
            fwrite(p,1,imgData[i].start-p,fp);
            fprintf(fp,"[img%d]",(*imgData[i].request).id);
            p = imgData[i].end;
        }

        if (p > xml.eob)
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::logprintf(pantheios::error,
                    "%s %s:%d write error p > xml.eob",
                    __PRETTY_FUNCTION__,__FILE__,__LINE__);
			#endif
            fclose(fp);
        } else
        {
            fwrite(p,1,xml.eob-p,fp);
            MR_PCLOSE(fp);
        }

		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__," new content saved in [",buffer,"]");
		#endif
        return true;
    } else
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::logprintf(pantheios::error,
                "%s %s:%d can't open pipe to [%s]",
                __PRETTY_FUNCTION__,__FILE__,__LINE__,buffer);
		#endif
        return false;
    }
}

void news::Item::findImg(std::string *path, bool *imgExists,
    std::string const &content,
    std::string const &extension, char *buffer)
{
    mrutils::BufferedReader reader;

    std::string const md5 = ::md5(content);
    char *eob = buffer + sprintf(buffer,"%s/img/%c%c/%s/",
        settings::contentDir, md5[0], md5[1], md5.c_str());

    int fileno = 0;

    if (DIR *dp = opendir(buffer))
    {
        char dirData[offsetof(struct dirent, d_name) + PATH_MAX];
        for (struct dirent *dirp; 0 == readdir_r(dp, (struct dirent*)&dirData, &dirp) && dirp;) {
            if (dirp->d_name[0] == '.') continue;
            strcpy(eob, dirp->d_name);

            int candidateNo = atoi(dirp->d_name);
            if (candidateNo >= fileno)
                fileno = candidateNo + 1;

            if (!reader.open(buffer))
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::logprintf(pantheios::error,
                        "%s %s:%d can't open file at [%s]",
                        __PRETTY_FUNCTION__,__FILE__,__LINE__,buffer);
				#endif
                continue;
            }

            if (0 == reader.cmp(content.c_str(),content.size()))
            {
                *path = buffer;
                *imgExists = true;
                closedir(dp);
                return;
            }
        }

        closedir(dp);
    } else
    {
        mrutils::mkdirP(buffer);
    }

    sprintf(eob,"%d.%s.gz", fileno, extension.c_str());
    *path = buffer;
    *imgExists = false;
}

bool news::Item::getContent(mrutils::XML& xml, mrutils::XML &xmlSecondary,
    int stopFD, char * buffer, int size, bool save)
{
    if (isGone)
        return false;

    mrutils::mutexAcquire(mutexContent);

        char * eob = buffer + sprintf(buffer,"%s%d/%d-%02d-%02d/%d/",
                news::settings::contentDir,
                this->date / 10000,
                this->date / 10000,
                this->date / 100 % 100,
                this->date % 100,
                this->id);

        if (!mrutils::mkdirP(buffer))
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::logprintf(pantheios::error,
                    "%s %s:%d error creating directory at [%s]",
                    __PRETTY_FUNCTION__,__FILE__,__LINE__,buffer);
			#endif
            return (mrutils::mutexRelease(mutexContent), false);
        }

        // if we're saving the content, see if it already exists
        if (save)
        {
            strcpy(eob,"article.txt.gz");
            if (mrutils::fileExists(buffer))
                return mrutils::mutexRelease(mutexContent), true;
        }

        if (0 != news::url::delayDomain(link, stopFD))
        {
            mrutils::mutexRelease(mutexContent);
            return false;
        }

        // get URL
        mrutils::curl::urlRequest_t request;
        news::url::setOptions(&request,link,buffer,size);
        request.xmlOk = false;
        if (0 != xmlSecondary.get(&request, stopFD) || xmlSecondary.eob == xmlSecondary.sob)
        {
            clearContent(buffer,false);
			#ifdef HAVE_PANTHEIOS
            pantheios::logprintf(pantheios::error,
                    "%s %s:%d curl failed for [%s]",
                    __PRETTY_FUNCTION__,__FILE__,__LINE__,
					request.url.c_str());
			#endif

			// try with original URL
			request.url = link;
			request.mobile = false;
			if (0 != xmlSecondary.get(&request, stopFD) || xmlSecondary.eob == xmlSecondary.sob)
			{
				clearContent(buffer,false);
				#ifdef HAVE_PANTHEIOS
				pantheios::logprintf(pantheios::error,
						"%s %s:%d curl failed for [%s]",
						__PRETTY_FUNCTION__,__FILE__,__LINE__,
						request.url.c_str());
				#endif
				mrutils::mutexRelease(mutexContent);
				return false;
			}
        }

		// check for redirection
		if (xmlSecondary.m_url != link)
		{
			news::url::setOptions(&request,xmlSecondary.m_url,buffer,size);

			if (xmlSecondary.m_url != request.url)
			{ // then there's a remapping for it, so refetch

				#ifdef HAVE_PANTHEIOS
				pantheios::logprintf(pantheios::informational,
						"%s %s:%d refetching [%s] as [%s]",
						__PRETTY_FUNCTION__,__FILE__,__LINE__,
						link.c_str(),request.url.c_str());
				#endif

				if (0 != news::url::delayDomain(request.url, stopFD))
				{
					mrutils::mutexRelease(mutexContent);
					return false;
				}

				request.xmlOk = false;
				if (0 != xmlSecondary.get(&request, stopFD) || xmlSecondary.eob == xmlSecondary.sob)
				{
					clearContent(buffer,false);
					#ifdef HAVE_PANTHEIOS
					pantheios::logprintf(pantheios::error,
							"%s %s:%d curl failed for [%s]",
							__PRETTY_FUNCTION__,__FILE__,__LINE__,link.c_str());
					#endif
					mrutils::mutexRelease(mutexContent);
					return false;
				}
			}
		}

        // set title if none
        if (title.empty())
        {
            if (char * titleStart = strstr(xmlSecondary.sob,"<title>"))
            {
                if (char * titleEnd = strstr(titleStart,"</title>"))
                {
                    titleStart += strlen("<title>");
                    title.assign(titleStart,titleEnd-titleStart);

                    char * d = const_cast<char*>(title.c_str());
                    title.resize(mrutils::XML::unescapeHTML(d,d+title.size())-d);
                }
            }
        }

        // crop content
		std::pair<char const *, char const *> crop =
			news::url::cropContent(link.c_str(), &xmlSecondary, stopFD);

		// copy contents
		xml.reset();
		int nAdd = crop.second - crop.first;
		memcpy(xml.eob, crop.first, nAdd);
		*(xml.eob += nAdd) = '\0';

        // get any subsequent pages
        int npages = 1;
        for (std::string nextLink = link; npages++ < 20 && 
            !(nextLink = news::url::getNextURL(nextLink, xmlSecondary.sob)).empty();)
        {
            // get next URL
            mrutils::curl::urlRequest_t request;
            news::url::setOptions(&request,nextLink,buffer,size);
            request.xmlOk = false;
            if (0 != xmlSecondary.get(&request, stopFD) || xmlSecondary.eob == xmlSecondary.sob)
                break;

            // crop this page
			std::pair<char const *, char const *> crop =
				news::url::cropContent(nextLink.c_str(), &xmlSecondary);

            // append to end of primary xml
            int nAdd = crop.second - crop.first;
            int nLeft = xml.buffSz - (xml.eob - xml.sob);
            if (nAdd < nLeft)
            {
                memcpy(xml.eob, crop.first, nAdd);
                *(xml.eob += nAdd) = '\0';
            }
            else
            {
                --nLeft;
                memcpy(xml.eob, crop.first, nLeft);
                *(xml.eob += nLeft) = '\0';
                break;
            }
        }

        // translate characters
        // xml.eob = mrutils::copyToWindowsLatin1(xml.sob,xml.eob-xml.sob,xml.sob,true);

        // save data
        if (save)
        {
            // find images
            std::set<mrutils::curl::urlRequestM_t> imgRequests;
            std::vector<imgData_t> imgData;
            findImages(xml.sob,xml.m_url,imgData,imgRequests);

            // save data
            if (!saveImages(buffer,imgRequests,stopFD)
                ||!saveContent(xml,buffer,imgData)
                ) return (clearContent(buffer,false), mrutils::mutexRelease(mutexContent), false);
        }

        if (description.empty())
        {
            if (xml.eob - xml.sob > 255)
            {
                description.assign(xml.sob,255);
            } else
            {
                description.assign(xml.sob,xml.eob-xml.sob);
            }

            news::HTML::makeAscii(description);
        }

    mrutils::mutexRelease(mutexContent);
    return true;
}

void news::Item::parseDetails(mrutils::XML& xml, char * buffer, int size) {
    char * tag = xml.nextTag();

    char * p = xml.tagPtr;

    for (;*tag != 0 && 0!=strcasecmp(tag,"item") && 0!=strcasecmp(tag,"entry");
        tag = xml.nextTag())
	{
        /*** title ***/
        if (strcasecmp(tag,"title") == 0) {
            xml.tag(title);
            for (std::string::iterator it = title.begin();
                 it != title.end(); ++it) if (*it == '\n') *it = ' ';
            mrutils::trim(title);
        } else 

        /*** link ***/
        if (
               0==strcasecmp(tag,"link")
            || 0==strcasecmp(tag,"feedburner:origLink")
            || 0==strcasecmp(tag,"pheedo:origLink")
            || 0==strcasecmp(tag,"pheedo:origLink")
           ) {
            std::string tmp;

            xml.tag(tmp); mrutils::trim(tmp); 
            if (mrutils::startsWithI(tmp.c_str(),"http"))
                link = tmp;
        } else

        if ((link.empty()) && (
               0==strcasecmp(tag,"guid")
            || 0==strcasecmp(tag,"id")
           )) {

            xml.tag(link); mrutils::trim(link); 
            if (!mrutils::startsWithI(link.c_str(),"http"))
                link.clear();
        } else

        /*** date ***/
        if (date <= 0 && (
               0==strcasecmp(tag,"date")
            || 0==strcasecmp(tag,"pubDate")
            || 0==strcasecmp(tag,"published")
            || 0==strcasecmp(tag,"dc:date")
           )) {
            date = mrutils::parseGuessDate((xml.tag(buffer,size),buffer));
        } else 

        /*** description ***/
        if (   0==strcasecmp(tag,"description")
            || 0==strcasecmp(tag,"content:encoded")
            || 0==strcasecmp(tag,"content")
            || 0==strcasecmp(tag,"summary")
           ) {

            xml.tag(description);
        }

    }

    // hack: sometimes you'll have <link href=""/> instead of
    // <link>url</link>
    if (link.empty()) {
        const char tagChar = *xml.tagPtr;
        *xml.tagPtr = 0;
        for (;NULL != (p = strstr(p,"<link"));++p) {
            char * q = strchr(p,'>');
            if (q) {
                *q = 0; p = strstr(p,"href="); *q = '>';
                if (p) {
                    p = strchr(p,'"');
                    if (p) {
                        q = strchr(p+1,'"');
                        if (q) {
                            *q = 0; link.assign(p+1); *q = '"';
                        } else break;
                    } else break;
                } else break;
            } else break;
        } *xml.tagPtr = tagChar;
    }

    if (date < 0) date = mrutils::getDate();

    // strip new lines in the descripiton 
    {
        description.resize(511);
        char buffer[512];
        char *eob = buffer;
        std::string::const_iterator it = description.begin();
        while (::isspace(*it))
            ++it;
        while (it != description.end())
        {
            if (::isspace(*it))
            {
                *eob++ = ' ';
                while (::isspace(*++it))
                    ;
            } else
            {
                *eob++ = *it++;
            }
        }
        description.assign(buffer,eob-buffer);
    }

    if (0==strcmp(tag,"item") || 0==strcmp(tag,"entry")) xml.prevTag();

    // twitter titles start with the feed name
    if (mrutils::stristr(link.c_str(),"twitter.com")) {
        size_t posSep = title.find(": ");
        if (std::string::npos != posSep)
            title = title.substr(posSep+2);

        posSep = description.find(": ");
        if (std::string::npos != posSep)
            description = description.substr(posSep+2);
    }

    if (title.size() > 128) title.resize(128);

    char * d = const_cast<char*>(title.c_str());
    title.resize(mrutils::XML::unescapeHTML(d,d+title.size())-d);

    // Twitter: find the first link and use that instead of the twitter
    // page
    if (mrutils::stristr(link.c_str(),"twitter.com")) {
        const char * q = strstr(description.c_str(),"http://");
        if (q != NULL) {
            const char * p = q+7;
            for (bool done = false;!done;) {
                switch (*p) {
                    case '\0':
                    case '\n':
                    case '\r':
                    case ' ':
                    case '\t':
                        done = true;
                        break;
                    default:
                        ++p;
                        break;
                }
            }
            link.assign(q,p-q);
        }
    } else if (mrutils::stristr(link.c_str(),"craigslist.org") && !title.empty()) {
        // move price to beginning
        const char * q = mrutils::strchrRev(title.c_str()+title.size(),'$',title.c_str());
        if (q != NULL && q != title.c_str()) {
            const int len = q - title.c_str();
            char * p = mrutils::strcpy(buffer,q);
            *p++ = ' '; memcpy(p,title.c_str(),len); p += len;
            title.assign(buffer,p-buffer);
        }

        // now move the location close to the beginning and abbreviate
        size_t lastParens = title.find_last_of("(");
        if (std::string::npos != lastParens) {
            size_t endParens = title.find_first_of(")");
            if (std::string::npos != endParens) {
                std::string location = title.substr(lastParens+1,
                    endParens-lastParens-1);
                std::transform(location.begin(), location.end(),
                    location.begin(), ::tolower);
                if ("upper east side" == location) {
                    location = "UES";
                } else if ("midtown east" == location) {
                    location = "ME";
                } else if ("soho" == location) {
                    location = "SH";
                } else if ("tribeca" == location) {
                    location = "TBC";
                } else if ("west village" == location) {
                    location = "WV";
                } else if ("greenwich village" == location) {
                    location = "GV";
                } else if ("union square" == location) {
                    location = "US";
                }

                size_t endPrice = title.find_first_of(" ");
                if (std::string::npos != endPrice) {
                    title = title.substr(0,endPrice+1) + location
                        + title.substr(endPrice);
                }
            }
        }
    }

}

#ifdef __APPLE__
    /**
     * Opens a new mail message in Mail.app containing this article
     * content with subject equal to the title
     */
    bool news::Item::openMail() {
        mrutils::stringstream ss;
        if (0 != system(
            oprintf(ss,"gunzip -c \"%s/%d/%d-%02d-%02d/%d/article.txt.gz\" | iconv -f CP1252 -t MAC > %s/mail.txt"
                ,settings::contentDir
                ,date / 10000
                ,date / 10000
                ,date / 100 % 100
                ,date % 100
                ,id
                ,settings::baseDir
                )
            ))
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__, " error with [",
                    ss.str(),"], [", strerror(errno), "]");
			#endif
            return true;
        }

        // build subject
        std::string subject = mrutils::escapeQuote(this->title);

        // now run applescript
        if (0 != system(
            oprintf(ss.clear(),"osascript \"%s/mail.scpt\" \"%s/mail.txt\" \"%s\""
                ,settings::baseDir
                ,settings::baseDir
                ,subject)
            ))
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__, " error with [",
                    ss.str(),"], [", strerror(errno), "]");
			#endif
        }

        return true;
    }

    bool news::Item::copyURL()
    {
        if (FILE *fp = popen("pbcopy","w"))
        {
            fputs(link.c_str(), fp);
            pclose(fp);
        }
        return true;
    }

    bool news::Item::copyMarkdownURL()
    {
        if (FILE *fp = popen("pbcopy","w"))
        {
			std::string domain = news::url::getDomain(link);
			if (domain.size() >= 3)
			{
				domain.resize(3);

				if (domain[0] == '.')
					domain = "sho";
				else if (domain[1] == '.')
					domain.resize(1);
				else if (domain[2] == '.')
					domain.resize(2);
			}
			else
			{
				domain = "sho";
			}

			fprintf(fp,"[#%s]: [%s](%s)",
					domain.c_str(),
					title.c_str(),
					link.c_str());
            pclose(fp);
        }
        return true;
    }
#endif
