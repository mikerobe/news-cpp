#include "settings.h"
#include <sstream>
#ifdef HAVE_PANTHEIOS
	#include <pantheios/pantheios.hpp>
	#include <pantheios/inserters.hpp>
	#include <pantheios/backends/bec.file.h>
	#include <pantheios/frontends/fe.simple.h>
#endif
#include <stdexcept>
#include <iostream>

#ifndef BUILD_LIB
extern "C" const char PANTHEIOS_FE_PROCESS_IDENTITY[] =
		"news";
#endif

#define IMAC "supamac.local"

namespace
{
	/**
	 * single threaded, uses static member data
	 * @return path to the system tool
	 */
	char const * findTool(char const *name) throw (std::runtime_error)
	{
		static std::string path;

		path = "type -p ";
		path += name;

		if (FILE *fp = popen(path.c_str(),"r"))
		{
			char buffer[1024];
			path.clear();

			for (int r = 0; 0 < (r = fread(buffer, 1, sizeof(buffer)-1, fp));)
			{
				if (buffer[r-1] ==  '\n')
					buffer[r-1] = '\0';
				else
					buffer[r] = '\0';
				path += buffer;
			}

			if (!path.empty())
			{
				return path.c_str();
			}
		}

		std::stringstream ss;
		ss << "Can't find tool [" << name << "]";
		throw std::runtime_error(ss.str());
	}

	char const * requireEnv(char const *name) throw (std::runtime_error)
	{
		if (char const *value = getenv(name))
		{
			return value;
		}
		else
		{
			std::stringstream ss;
			ss << "Can't find environmental var [" << name << "]";
			throw std::runtime_error(ss.str());
		}
	}
}

namespace news
{
    namespace settings
	{
		_API_ void setupPantheios()
		{
#			ifndef BUILD_LIB
			#ifdef HAVE_PANTHEIOS
			pantheios_fe_simple_setSeverityCeiling(settings::logSeverity);
			pantheios_be_file_setFilePath(settings::errorFile,
					PANTHEIOS_BE_FILE_F_TRUNCATE, PANTHEIOS_BE_FILE_F_TRUNCATE,
					PANTHEIOS_BEID_ALL);
			#endif
#			endif
		}

		int setup()
		{
			static std::string s_homeDir;
			static std::string s_saveDir;
			static std::string s_baseDir;
			static std::string s_contentDir;
			static std::string s_sqlitePath;
			static std::string s_lockFile;
			static std::string s_errorFile;

//			static std::string s_gif2png;
			static std::string s_convert;
			static std::string s_dump;

			std::stringstream ss;

			s_homeDir = requireEnv("HOME");

			s_saveDir = s_homeDir + "/Documents/Reference/News/";
			s_baseDir = s_homeDir + "/.news/";
			s_contentDir = s_homeDir + "/.news/content/";
			s_sqlitePath = s_homeDir + "/.news/news.sqlite";
			s_lockFile = s_homeDir + "/.news/lock";
			s_errorFile = s_homeDir + "/.news/err.log";

            // static std::string s_drive = "/Volumes/Etendu";
			// s_saveDir = s_drive + "/Reference/News";
			// s_baseDir = s_drive + "/Documents/news";
			// s_contentDir = s_baseDir + "/content";
			// s_sqlitePath = s_baseDir + "/news.sqlite";
			// s_lockFile = s_baseDir + "/lock";
			// s_errorFile = s_baseDir + "/err.log";

//			s_gif2png = findTool("gif2png");
			s_convert = findTool("convert");

			ss.str("");
			ss << findTool("elinks");
			// ss << " -no-home 1 -no-references -default-mime-type 'text/html' -no-numbering -dump-width 43 -dump 1 -eval \"set document.codepage.assume = 'windows-1252'\" -dump-charset 'windows-1252' | ";
			// ss << findTool("gsed")
			// 	<< " -e \"s:\\x96:--:g\" -e \"s:\\x97:---:g\"";
			ss << " -no-home 1 -no-references -default-mime-type 'text/html' -no-numbering -dump-width 43 -dump 1 -dump-charset 'utf-8'";
			s_dump = ss.str();

			homeDir = s_homeDir.c_str();
			saveDir = s_saveDir.c_str();
			baseDir = s_baseDir.c_str();
			contentDir = s_contentDir.c_str();
			sqlitePath = s_sqlitePath.c_str();
			lockFile = s_lockFile.c_str();
			errorFile = s_errorFile.c_str();
			dump = s_dump.c_str();
//			gif2png = s_gif2png.c_str();
			imgConvert = s_convert.c_str();

			return 0;
		}

		int const hack = setup();


        char const * homeDir;
        char const * saveDir;
        char const * baseDir;
		char const * contentDir;
        char const * sqlitePath;
        char const * lockFile;
        char const * errorFile;

        char const * dump;
//        char const * gif2png;
        char const * imgConvert;

        char const * openURL = 
            #if defined(__APPLE__) || defined(__CYGWIN__)
                "open";
            #elif defined(__linux__)
                "gnome-open";
            #else
                #error "unknown platform for open command"
            #endif


        #if defined(__APPLE__)
            char const * sqlServer = "/tmp/mysql.sock";
        #else
            char const * sqlServer = "localhost";
        #endif
        short        sqlPort = 3306;
        char const * sqlUser = "news";
        char const * sqlPass = "news";


        #ifdef __CYGWIN__
            dbType const useDb = DB_SQLITE;
        #else
            dbType const useDb = DB_MYSQL;
        #endif

        char const * serverHost = IMAC;
        short        serverPort = 6002;

        int const clientSyncMillis = 300000;
        int const updateWaitMillis = 120000;

        int const maxImages = 5;

        ATOMIC_INT lastItemId;

        int const domainWaitSeconds = 5;

		#ifdef HAVE_PANTHEIOS
        pantheios::pan_severity_t logSeverity =
                //pantheios::SEV_NOTICE;
                pantheios::SEV_INFORMATIONAL;
                //pantheios::SEV_DEBUG;
		#endif
    }
}


