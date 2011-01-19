#ifndef _MR_NEWS_SETTINGS_H
#define _MR_NEWS_SETTINGS_H

#include "mr_threads.h" // for ATOMIC
#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
#endif

namespace news {
    namespace settings {
		/**
		 * @return return value is a dummy for hack variable
		 */
		int setup();

		void setupPantheios();

        _API_ extern char const * saveDir;
        _API_ extern char const * baseDir;
        _API_ extern char const * homeDir;
        _API_ extern char const * contentDir;
        _API_ extern char const * dump;
//        _API_ extern char const * gif2png;
        _API_ extern char const * imgConvert;
        _API_ extern char const * openURL;

        _API_ extern char const * lockFile;
        _API_ extern char const * errorFile;

        _API_ extern char const * sqlServer;
        _API_ extern short        sqlPort;
        _API_ extern char const * sqlUser;
        _API_ extern char const * sqlPass;

        _API_ extern char const * sqlitePath;

        /**
         * seconds delay between fetching articles on the same domain
         */
        _API_ extern int const domainWaitSeconds;

        enum _API_ dbType {
            DB_MYSQL
           ,DB_SQLITE
        };

        _API_ extern dbType const useDb;

        _API_ extern char const * serverHost;
        _API_ extern short        serverPort;

        _API_ extern int const clientSyncMillis;
        _API_ extern int const updateWaitMillis;

        _API_ extern int const maxImages;

        // can't use auto_increment
        _API_ extern ATOMIC_INT lastItemId;

		#ifdef HAVE_PANTHEIOS
        _API_ extern pantheios::pan_severity_t logSeverity;
		#endif

		extern int const hack;
    }
}

#endif
