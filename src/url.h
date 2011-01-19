#ifndef _MR_NEWS_URL_H
#define _MR_NEWS_URL_H

#include "mr_curl.h"
#include "mr_xml.h"

#include <stdexcept>


/**
 * This is for adapting URLs from the plain form
 * in the feed into the print variety
 * */

namespace news
{
    class Item;

    namespace url
	{
        char * setOptions(mrutils::curl::urlRequest_t *request,
            std::string const &url, char * buffer, int size);
		std::pair<char const *, char const *> cropContent(const char * url, mrutils::XML * xml, int stopFD=0);
        bool keepItem(const Item& item);
        std::string getNextURL(std::string const &url, char const *prevPage);

        /**
         * If the URL of a feed itself has to change from day to day,
         * this function will do it. Alters the URL in place.
         */
        void updateFeedUrl(std::string *url, char *buffer);

        /**
         * For each domain, there's an x second delay between article
         * retrievals to avoid triggering floodgate protections
         * Return 0 if it waited, non-zero if it was interrupted.
         */
        int delayDomain(std::string const &url, int stopFD);

		/**
		 * returns "ft.com" for "http://www.ft.com/asdf.html"
		 */
		std::string getDomain(std::string const &url);

		class URLPath;
		class URLParser;
    }
}

class news::url::URLPath
{
public:
	URLPath(std::string const &path)
	{
		if (path.size())
		{
			for (char const *c1 = path.c_str()+1, *c2 = strchr(c1,'/');;
					c1 = c2+1, c2 = strchr(c1,'/'))
			{
				if (c2 == NULL)
				{
					levels.push_back(c1);
					break;
				}

				levels.push_back(std::string(c1,c2-c1));
			}
		}
	}

	std::string getPath() const
	{
		std::ostringstream ss;
		for (std::vector<std::string>::const_iterator it = levels.begin();
				it != levels.end(); ++it)
		{
			ss << "/" << *it;
		}
		return ss.str();
	}

	void addLevel(std::string const &level)
	{
		if (levels.size() && levels.back().empty())
		{
			levels.back().assign(level);
		}
		else
		{
			levels.push_back(level);
		}
	}

	bool endsInDir() const
	{
		return levels.size() == 0 || levels.back().empty();
	}

	bool hasLastDir() const
	{
		return levels.size() > 1;
	}

	std::string & getLastDir()
	{
		if (hasLastDir())
			return levels[levels.size()-1];

		throw std::runtime_error("path has no last directory");
	}

	std::string const & getLastDir() const
	{
		if (hasLastDir())
			return levels[levels.size()-1];

		throw std::runtime_error("path has no last directory");
	}

	std::vector<std::string> levels;

	friend std::ostream & operator <<(std::ostream &lhs,
			URLPath const &rhs)
	{
		lhs << "URLPath ( ";
		for (std::vector<std::string>::const_iterator it = rhs.levels.begin();
				it != rhs.levels.end(); ++it)
		{
			lhs << "[" << *it << "] ";
		}
		return lhs << ")";
	}

};

class news::url::URLParser
{
public:
	URLParser(std::string const url) :
		protocol(getProtocol(url)),
		domain(getDomain(url)),
		path(protocol.size() ?
				url.substr(
					protocol.size()+strlen("://")+domain.size(),
					url.find("?") == std::string::npos
					? std::string::npos
					: url.find("?") -
					  (protocol.size()+strlen("://")+domain.size()))
				: ""),
		get(url.find("?") == std::string::npos
				? ""
				: url.substr(url.find("?")))
	{
	}

	static std::string getProtocol(std::string const &url)
	{
		size_t const pos = url.find("://");
		if (pos == std::string::npos)
			return "";
		return url.substr(0, pos);
	}

	static std::string getDomain(std::string const &url)
	{
		size_t pos1 = url.find("://");
		if (pos1 == std::string::npos)
			return "";

		pos1 += strlen("://");
		size_t pos2 = url.find("/", pos1);
		if (pos2 == std::string::npos)
			return url.substr(pos1);

		return url.substr(pos1, pos2 - pos1);
	}

	/*!
	 * @return
	 * the URL formed from the protocol, the domain, the path and the
	 * get string
	 */
	std::string formURL() const
	{
		std::ostringstream ss;
		ss << protocol << "://" << domain << path.getPath() << get;
		return ss.str();
	}

	std::string protocol; ///< http
	std::string domain; ///< www.nytimes.com
	URLPath path; ///< /article/foo
	std::string get; ///< ?bar=baz

	friend std::ostream & operator <<(std::ostream &lhs,
			URLParser const &rhs)
	{
		return lhs << "URL ( "
			"protocol [" << rhs.protocol << "], "
			"domain [" << rhs.domain << "], "
			"path [" << rhs.path.getPath() << "], "
			"get [" << rhs.get << "] )";
	}
};

#endif
