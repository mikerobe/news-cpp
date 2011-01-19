#include "visit.h"
#include "url.h"

#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
	#include <pantheios/inserters.hpp>
#endif

#ifdef __APPLE__
    /**
     * Opens a new mail message in Mail.app containing this article
     * content with subject equal to the title
     */
    bool news::Visit::openMail() {
        mrutils::stringstream ss;
        if (0 != system(
            oprintf(ss,"gunzip -c \"%s/%d/%d-%02d-%02d/%d/article.txt.gz\" | iconv -f CP1252 -t MAC > %s/mail.txt"
                ,settings::contentDir
                ,itemDate / 10000
                ,itemDate / 10000
                ,itemDate / 100 % 100
                ,itemDate % 100
                ,itemId
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
        std::string subject = mrutils::escapeQuote(this->itemTitle);

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
    bool news::Visit::copyURL()
    {
        if (FILE *fp = popen("pbcopy","w"))
        {
            fputs(link.c_str(), fp);
            pclose(fp);
        }
        return true;
    }
    bool news::Visit::copyMarkdownURL()
    {
        if (FILE *fp = popen("pbcopy","w"))
        {
			std::string domain = news::url::getDomain(link);
			domain.resize(3);

			if (domain.size() >= 3)
			{
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
					itemTitle.c_str(),
					link.c_str());
            pclose(fp);
        }
        return true;
    }
#endif
