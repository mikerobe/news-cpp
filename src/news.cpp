#include <iostream>
#include <sys/syslimits.h>
#include "news.h"
#include <algorithm>
#include <memory>
#include "category.h"
#include "feed.h"
#include "mr_gui.h"
#include "mr_params.h"
#include "mr_prog.h"
#include "mr_numerical.h"
#include "viewer.h"
#include "mr_mysql.h"
#include "mr_sqlite.h"
#include <sys/syslimits.h>
#include <unistd.h>

namespace {
int lockFD = 0;

inline bool lockFile() {
    if (!mrutils::fileExists(news::settings::lockFile)
                && !mrutils::touch(news::settings::lockFile))
	{
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::emergency,
                "Can't make lock file at ",
                news::settings::lockFile);
		#endif
        return false;
    }

    lockFD = MR_OPEN(news::settings::lockFile, O_RDWR);
    if (lockFD == -1)
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::emergency,
                "Unable to open lock file at ",
                news::settings::lockFile);
		#endif
        return false;
    }

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            "Another instance is open. Waiting...");
	#endif

    if (MR_FLOCK(lockFD) != 0)
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::emergency,
                "Error obtaining lock at ",
                news::settings::lockFile);
		#endif
        return false;
    }

    return true;
}
}

bool news::News::setupTables(mrutils::Sql& sql)
{
    mrutils::stringstream ss;
    return (   sql.run(ss.clear() << "CREATE TABLE if not exists categories (catId integer primary key " << sql.autoIncrement() << ", title varchar(128) not null)")
            && sql.run(ss.clear() << "CREATE TABLE if not exists feedcat (feedcatId integer primary key " << sql.autoIncrement() << ", feedId integer not null, catId integer not null)")
            && (sql.run(ss.clear() << "CREATE UNIQUE INDEX feedcatindex on feedcat (feedId,catId)"),true)
            && sql.run(ss.clear() << "CREATE TABLE if not exists feeds (feedId integer primary key " << sql.autoIncrement() << ", title varchar(128) not null, link varchar(1024) not null, maxItems int not null, `format` int not null, lastReadItemId int not null default 0, updateSeconds int not null default 300, lastUpdate int not null, checkAllTitles int not null)")
            && sql.run(ss.clear() << "CREATE TABLE if not exists items (itemId integer primary key " << sql.autoIncrement() << ", feedId int not null, `date` int not null, title varchar(128) not null, link varchar(1024) not null, summary varchar(512) not null)")
            && sql.run(ss.clear() << "CREATE TABLE if not exists offline (sourceId integer primary key " << sql.autoIncrement() << ", `date` int not null, `hour` double not null, itemId int not null, feedId int not null, feedTitle varchar(128) not null, itemTitle varchar(128) not null, link varchar(1024) not null, summary varchar(512) not null, offline_sent tinyint not null default 0, source char(12))")
            && sql.run(ss.clear() << "CREATE TABLE if not exists visits (visitId integer primary key " << sql.autoIncrement() << ", `date` int not null, `hour` double not null, itemId int not null, feedId int not null, feedTitle varchar(128) not null, itemTitle varchar(128) not null, link varchar(1024) not null, summary varchar(512) not null, source char(12) not null default \"local\", sourceId int not null)")
            && (sql.run(ss.clear() << "CREATE UNIQUE INDEX visitindex on visits (source,sourceId)")
                ,sql.run(ss.clear() << "CREATE INDEX visitdateindex on visits (date,hour)")
                ,true)
           );
}

void news::News::init()
{
    // close stderr && open null
    news::settings::setupPantheios();
    if (NULL == freopen(/*settings::errorFile*/"/dev/null", "w", stderr))
    {
        throw std::runtime_error((mrutils::stringstream().clear()
            << "Unable to open /dev/null for stderr."
            ).str());
    }

    if (!lockFile()) {
        throw std::runtime_error((mrutils::stringstream().clear()
            << "can't get lock"
            ).str());
    }

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            "Got lock");
	#endif

    if (!stopFD.init()) {
        throw std::runtime_error("Can't init stopFD");
    }

    if (!mrutils::mkdirP(news::settings::baseDir)) {
        throw std::runtime_error((mrutils::stringstream().clear()
            << "Can't create dir at " << news::settings::baseDir).str());
    }

    if (!mrutils::mkdirP(news::settings::contentDir)) {
        throw std::runtime_error((mrutils::stringstream().clear()
            << "Can't create content dir " << news::settings::contentDir
            ).str());
    }

    if (settings::useDb == settings::DB_MYSQL) {
        mainThreadSql = new mrutils::Mysql(settings::sqlServer,settings::sqlPort,settings::sqlUser,settings::sqlPass,"news",settings::sqlServer[0] == '/' ? mrutils::Socket::SOCKET_UNIX : mrutils::Socket::SOCKET_STREAM);
    } else if (settings::useDb == settings::DB_SQLITE) {
        mainThreadSql = new mrutils::Sqlite(settings::sqlitePath);
    } else {
        throw std::runtime_error((mrutils::stringstream().clear()
            << "unhandled database type"
            ).str());
    }

    if (!mainThreadSql->connect()) {
        throw std::runtime_error((mrutils::stringstream().clear()
            << "Unable to connect to database"
            ).str());
    }

    if (!setupTables(*mainThreadSql)) {
        throw std::runtime_error((mrutils::stringstream().clear()
            << "can't setup tables: " << mainThreadSql->error
            ).str());
    }

    // reader & writer wait times & interrupts
    reader.setSocketInterruptFD(stopFD.readFD);
    reader.setSocketWaitTime(60);
    writer.setInterruptFD(stopFD.readFD);
    writer.setMaxWaitSecs(60);

    // get the latest itemId
    int lastItemId = 0;
    if (mainThreadSql->get("SELECT MAX(itemId) FROM items",lastItemId))
        settings::lastItemId = lastItemId;

    // build singletons
    History::singleton = new History(*this,*mainThreadSql);
    Viewer::singleton = new Viewer();

    if (mode == MD_CLIENT && !initClient()) {
        throw std::runtime_error((mrutils::stringstream().clear()
            << "can't init client"
            ).str());
    }

    initCategories();
    initFeeds();

    if (mode == MD_CLIENT)
    {
        if (online)
        {
            threads.push_back(mrutils::threadRun(fastdelegate::MakeDelegate(this, &News::clientThread),NULL,false));
        }
    } else if (mode == MD_SERVER)
    {
        threads.push_back(mrutils::threadRun(fastdelegate::MakeDelegate(this,&News::serverThread),NULL,false));

        for (int i =0, imax = 2*mrutils::numProcessors(); i < imax; ++i)
        {
            threads.push_back(mrutils::threadRun(fastdelegate::MakeDelegate(this,
                &News::updateThread),NULL,false));
        }
    }
}

void news::News::initCategories() {
    // build "All" category
    (Feed::allCategory = new Category(-1, "All"))->isAll = true;
    categories.push_back(Feed::allCategory);

    // build "All" feed for all category
    Feed *allF = new Feed("All",this); 
    allF->isAll = allF->init_ = true;
    Feed::allCategory->joinFeed(allF);

    for (mainThreadSql->rr("SELECT catId, title FROM categories ORDER BY title ASC");
        mainThreadSql->nextLine();) {
        Category* cat = new Category(mainThreadSql->getInt(0), mainThreadSql->getString(1));
        categories.push_back(cat);
        categoryMap.insert(cat->id,cat);

        // add All feed to category
        cat->joinFeed(allF = new Feed("All",this)); 
        allF->isAll = allF->init_ = true;
    }

    // add categories to the GUI
    for (std::vector<Category*>::iterator it = categories.begin(); it != categories.end(); ++it) {
        feedChooser.colChooser.next() << (*it)->title;
        if (!(*it)->isAll) feedCatChooser.colChooser.next() << (*it)->title;
    }
}

void news::News::initFeeds()
{
    std::auto_ptr<mrutils::Sql> sql2(mainThreadSql->clone());
    sql2->connect();
    mrutils::stringstream ss;

    // get all feeds
    for (mainThreadSql->rr(
                "SELECT feeds.feedId, title, link, maxItems, format, lastReadItemId, updateSeconds, checkAllTitles, lastUpdate FROM \
                feeds ORDER BY title ASC");
            mainThreadSql->nextLine();)
    {
        Feed * f = new Feed(mainThreadSql->getString(1),this);
        f->id = mainThreadSql->getInt(0);
        f->link.assign(mainThreadSql->getString(2));
        f->maxItems = mainThreadSql->getInt(3);
        f->format = (Feed::FeedFormat)mainThreadSql->getInt(4);
        f->lastReadItemId = mainThreadSql->getInt(5);
        f->updateSeconds = mainThreadSql->getInt(6);
        f->checkAllTitles = mainThreadSql->getInt(7);
        f->lastUpdate = mainThreadSql->getInt(8);

        //int const numUnread = sql2->get<int>(ss.clear() << "SELECT COUNT(*) FROM items WHERE feedid=" << f->id << " AND itemid > " << f->lastReadItemId);
        //f->initUnread(numUnread,queryBuffer);

        f->initUnread(0,queryBuffer);

        Feed::allCategory->feeds.insert(f); // but don't add all category to feed
        feedMap.insert(f->id,f);
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::informational,
                "added feed ", f->title);
		#endif
    }

    // set feed unread
    for (mainThreadSql->rr(
        "SELECT feeds.feedId,count(*) from feeds join items using (feedid) \
        WHERE itemid > feeds.lastReadItemId group by feedid");
        mainThreadSql->nextLine();)
    {
        feedMap[mainThreadSql->getInt(0)]->initUnread(mainThreadSql->getInt(1),
                queryBuffer);
    }

    // add feed categories
    for (mainThreadSql->rr("SELECT feedId,catId FROM feedCat ORDER BY feedId,catId ASC");
        mainThreadSql->nextLine();)
    {
        categoryMap[mainThreadSql->getInt(1)]->joinFeed(
                feedMap[mainThreadSql->getInt(0)]);
    }
}

bool news::News::initClient() {
    History::singleton->setUseOffline();

    if (!mrutils::getMACAddress(queryBuffer))
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::critical,
                __PRETTY_FUNCTION__,
                " can't get mac address");
		#endif
        return false;
    }

    History::singleton->setSourceId(queryBuffer);

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__,
            " connecting to server [any key to stop]");
	#endif

    mrutils::Socket sock(settings::serverHost,settings::serverPort);
    if (!sock.initClient(5,0))
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::critical,
                __PRETTY_FUNCTION__,
                " unable to connect to server ",
                settings::serverHost,":",
                pantheios::integer(settings::serverPort));
		#endif
        online = false;
    }

    if (online)
    {
        sock.write(0);
        reader.use(sock);
        writer.setFD(sock.s_);

        try
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::informational,
                    __PRETTY_FUNCTION__,
                    " syncing categories, feeds, feedcat...");
			#endif

            std::vector<std::string> omitColumns;
            omitColumns.push_back("offline_sent");
            omitColumns.push_back("lastReadItemId");

            if (!mainThreadSql->syncFromDump(reader,"categories")
                ||!mainThreadSql->syncFromDump(reader,"feeds")
                ||!mainThreadSql->syncFromDump(reader,"feedcat"))
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::logprintf(pantheios::error,
                        "%s %s:%d sql error [%s]",
                        __PRETTY_FUNCTION__, __FILE__, __LINE__,
                        mainThreadSql->error);
				#endif
                return false;
            }

			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::informational,
                    __PRETTY_FUNCTION__,
                    " sending offline visits...");
			#endif
            mainThreadSql->dumpTable(writer,"offline",&omitColumns);

			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::informational,
                    __PRETTY_FUNCTION__,
                    " syncing visits...");
			#endif
            writer << History::singleton->lastVisitId;
            writer.flush();

            if (!mainThreadSql->syncFromDumpLastId(reader,"visits"))
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::logprintf(pantheios::error,
                        "%s %s:%d sql error [%s]",
                        __PRETTY_FUNCTION__, __FILE__, __LINE__,
                        mainThreadSql->error);
				#endif
                return false;
            }

            // now OK to delete everything from offline
            mainThreadSql->run("DELETE FROM offline");

            // sync lastReadItemIds
            if (!clientDumpLastReadItemIds(*mainThreadSql,NULL,scratchBuffer,sizeof(scratchBuffer)))
                online = false;

        } catch (mrutils::ExceptionNoSuchData const &e)
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__," ",
                __FILE__,":",pantheios::integer(__LINE__)," ",
                e.what());
			#endif
            return false;
        }
    }

    return true;
}

void news::News::serverDumpItemsAndContentFinish(mrutils::BufferedWriter&,
        mrutils::Database::Row const &row)
        throw (mrutils::ExceptionNoSuchData)
{
    int const itemId = row.get<int>(0);
    int const date = row.get<int>(2);

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::debug,
            __PRETTY_FUNCTION__," on ",
            pantheios::integer(itemId),
            " (", pantheios::integer(date),
            ")");
	#endif

    char buffer[PATH_MAX];

    // write all files for this item
    char * p = buffer + sprintf(buffer,"%s%d/%d-%02d-%02d/%d/",
            news::settings::contentDir,
            date / 10000,
            date / 10000,
            date / 100 % 100,
            date % 100,
            itemId);

    if (DIR * dp = opendir(buffer))
	{
		char dirData[offsetof(struct dirent, d_name) + PATH_MAX];

		try
		{
			for (struct dirent * dirp
				;0 == readdir_r(dp,(struct dirent*)&dirData,&dirp) && NULL != dirp;)
			{
				if (dirp->d_name[0] == '.' || dirp->d_name[0] == '\0')
					continue;

				strcpy(p,dirp->d_name);

				const int size = mrutils::fileSize(buffer);
				if (size <= 0)
					continue;

				#ifdef HAVE_PANTHEIOS
				pantheios::log(pantheios::debug,
						__PRETTY_FUNCTION__," dumping file [",
						p,"] size [",pantheios::integer(size),"]");
				#endif
				writer << dirp->d_name << size;
				writer.putFile(buffer);
			}
		} catch (const mrutils::ExceptionNoSuchData& e)
		{
			closedir(dp);
			throw e;
		}

		closedir(dp);
	} else
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__," "
                "unable to open dir at ",buffer);
		#endif
    }

    writer << '\0';
}

namespace {
struct feedMax {
    int m_feedId;
    int m_maxItems;
    feedMax(int feedId, int maxItems) :
        m_feedId(feedId),
        m_maxItems(maxItems)
    {}
};
}

void news::News::setItemsArchivedFlag(mrutils::Sql& sql)
        throw(mrutils::ExceptionNoSuchData)
{

    if (!sql.run("UPDATE items SET archived = 1"))
    { throw mrutils::ExceptionNoSuchData(sql.error); }

    if (!sql.rr("SELECT feedId, maxItems FROM feeds"))
    { throw mrutils::ExceptionNoSuchData(sql.error); }


    std::vector<feedMax> list;
    list.reserve(feedMap.size());

    while (sql.nextLine())
    { list.push_back(feedMax(sql.getInt(0), sql.getInt(1))); }

    std::stringstream ss;

    for (std::vector<feedMax>::const_iterator it = list.begin();
            it != list.end(); ++it)
    {
        struct feedMax const &feedMax = *it;

        ss.str("");
        ss << "UPDATE items SET archived=0 WHERE feedId="
            << feedMax.m_feedId << " ORDER BY itemid DESC LIMIT "
            << feedMax.m_maxItems;
        if (!sql.run(ss.str().c_str()))
        { throw mrutils::ExceptionNoSuchData(sql.error); }
    }
}

bool news::News::serverDumpItemsAndContent(mrutils::Sql& sql)
{
    try
    {
        setItemsArchivedFlag(sql);

        std::vector<std::string> omitColumns;
        omitColumns.push_back("archived");

        if (!sql.dumpTableFromLastId(reader,writer,"items",&omitColumns,"archived = 0",
            fastdelegate::MakeDelegate(this,
                &news::News::serverDumpItemsAndContentFinish)))
            return false;

    } catch (mrutils::ExceptionNoSuchData const &e)
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__," ",
                __FILE__,":",pantheios::integer(__LINE__)," "
                "ExceptionNoSuchData(",e.what(),")");
		#endif
        return false;
    }

    return true;
}

bool news::News::serverAddOfflineVisits(mrutils::Sql& sql) {
    if (!sql.syncFromDumpLastId(reader,"visits"))
        return false;
    History::singleton->updateFromDB(sql);
    return true;
}

void news::News::serverThread(void *)
{
	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__, " entered");
	#endif

    mrutils::Socket sock(settings::serverPort);
    std::auto_ptr<mrutils::Sql> sql(mainThreadSql->clone());

    std::vector<std::string> omitColumns;
    //omitColumns.push_back("source");
    //omitColumns.push_back("sourceid");
    omitColumns.push_back("lastReadItemId");

    static const int bufferSize = 1024;
    char *queryBuffer = new char[bufferSize];

    while (!sock.initServer())
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__,
                " can't initialize. trying again...");
		#endif
        if (!stopFD.sleep(1000))
            return;
    }

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__," initialized");
	#endif

    fd_set fds; FD_ZERO(&fds);
    int selmax = 1+MAX_(sock.s_,stopFD.readFD);

    mrutils::stringstream ss;
    mrutils::Socket * s2 = NULL;

    while (!exit_)
    {
        reader.close();
        reader.setSocketWaitTime(-1); /**< wait forever on reads (unless disconnected */

        try
        {
            writer.flush();
        } catch (...) {}

        if (s2 != NULL)
            delete s2, s2 = NULL;

		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::informational,
                __PRETTY_FUNCTION__," waiting for client");
		#endif
        FD_SET(sock.s_,&fds); FD_SET(stopFD.readFD,&fds);
        if (0 >= select(selmax,&fds,(fd_set*)NULL,(fd_set*)NULL,NULL) || FD_ISSET(stopFD.readFD,&fds))
            break;

        if (NULL == (s2 = sock.accept()))
            continue;

        reader.use(*s2);
        writer.setFD(s2->s_);

		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::informational,
                __PRETTY_FUNCTION__," has a client");
		#endif

        try
        {
            switch (const int procedure = reader.get<int>())
            {
                case 0:
                    if (!(
							#ifdef HAVE_PANTHEIOS
							pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," dump categories"),
							#endif
                                sql->dumpTable(writer,"categories"))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," dump feeds"),
								#endif
                                    sql->dumpTable(writer,"feeds",&omitColumns))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," dump feedcat"),
								#endif
                                    sql->dumpTable(writer,"feedcat"))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," add offline visits"),
								#endif
                                    writer.flush(), serverAddOfflineVisits(*sql))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," sync visits by last id"),
								#endif
                                    sql->dumpTableFromLastId(reader,writer,"visits",&omitColumns))
                       )
                    {
						#ifdef HAVE_PANTHEIOS
                        pantheios::logprintf(pantheios::error,
                                "%s %s:%d sql error [%s]",
                                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                                sql->error);
						#endif
                        continue;
                    }
                    break;
                case 1:
                    if (!(
							#ifdef HAVE_PANTHEIOS
							pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," add offline visits"),
							#endif
                                serverAddOfflineVisits(*sql))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," sync visits by last id"),
								#endif
                                sql->dumpTableFromLastId(reader,writer,"visits",&omitColumns))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," dumping content"),
								#endif
                                    writer.flush(), serverDumpItemsAndContent(*sql))
                       )
                    {
						#ifdef HAVE_PANTHEIOS
                        pantheios::logprintf(pantheios::error,
                                "%s %s:%d sql error [%s]",
                                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                                sql->error);
						#endif
                        continue;
                    }
                    break;
                case 2:
                    if (!(
							#ifdef HAVE_PANTHEIOS
							pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," dump categories"),
							#endif
                                sql->dumpTable(writer,"categories")) 
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," dump feeds"),
								#endif
                                    sql->dumpTable(writer,"feeds",&omitColumns))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," dump feedcat"),
								#endif
                                    sql->dumpTable(writer,"feedcat"))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," dumping content"),
								#endif
                                    writer.flush(),serverDumpItemsAndContent(*sql))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," add offline visits"),
								#endif
                                    writer.flush(), serverAddOfflineVisits(*sql))
                            ||!(
								#ifdef HAVE_PANTHEIOS
								pantheios::log(pantheios::informational, __PRETTY_FUNCTION__," sync visits by last id"),
								#endif
                                    writer.flush(),sql->dumpTableFromLastId(reader,writer,"visits",&omitColumns))
                       )
                    {
						#ifdef HAVE_PANTHEIOS
                        pantheios::logprintf(pantheios::error,
                                "%s %s:%d sql error [%s]",
                                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                                sql->error);
						#endif
                        continue;
                    }
                    break;
                default:
					#ifdef HAVE_PANTHEIOS
                    pantheios::logprintf(pantheios::error,
                            "%s %s:%d server error: got invalid procedure "
                            "from client: %d",
                            __PRETTY_FUNCTION__, __FILE__, __LINE__,
                            procedure);
					#endif
                    continue;
            }

            // now send lastReadItemIds
            std::vector<std::pair<int,int> > feedLastReadItemids; 
            sql->rr("SELECT feedId, lastReadItemId FROM feeds ORDER BY feedId ASC");
            while (sql->nextLine()) {
                writer << sql->getInt(0);
                writer << sql->getInt(1);
                feedLastReadItemids.push_back(std::pair<int,int>(sql->getInt(0),sql->getInt(1)));
            }
            writer << -1;
            writer.flush();

            // now read lastReadItemIds
            std::vector<std::pair<int,int> >::iterator it = feedLastReadItemids.begin();
            for (int sentId, sentRead; -1 != (sentId = reader.get<int>());)
            {
                sentRead = reader.get<int>();

                // advance to that id 
                for (;it != feedLastReadItemids.end() 
                        && it->first < sentId; ++it)
                {}

                if (it == feedLastReadItemids.end() || it->first != sentId)
                {
					#ifdef HAVE_PANTHEIOS
                    pantheios::logprintf(pantheios::error,
                            "%s %s:%d got unknown feedId from client (%d)",
                            __PRETTY_FUNCTION__, __FILE__, __LINE__,
                            sentId);
					#endif
                    continue;
                }

                if (sentRead <= it->second)
                    continue;

                // update this feed
                mrutils::map<int,Feed*>::iterator it = feedMap.find(sentId);

                if (it == feedMap.end())
                    continue;

                (*it)->second->updateLastReadItemId(sentRead,*sql
                        ,queryBuffer,bufferSize
                        ,feedViewer, activeFeed, mutexActiveFeed, mutexFeeds);

                updateFeedLineInChooser((*it)->second);
            }

        } catch (const mrutils::ExceptionNoSuchData& e)
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",
                    __FILE__,":",pantheios::integer(__LINE__)," "
                    "ExceptionNoSuchData(",e.what(),")");
			#endif
        }

		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::informational,
                __PRETTY_FUNCTION__, " done with client");
		#endif

        if (s2 != NULL)
            delete s2, s2 = NULL;
    }

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__, " exited");
	#endif

    delete[] queryBuffer;
}

bool news::News::clientDumpItemsAndContentFinish(mrutils::BufferedReader&,
        mrutils::Database::Row const &row)
        throw (mrutils::ExceptionNoSuchData)
{
    int const itemId = row.get<int>(0);
    int const date = row.get<int>(2);

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::debug,
            __PRETTY_FUNCTION__," on ",
            pantheios::integer(itemId),
            " (", pantheios::integer(date),
            ")");
	#endif

    char buffer[PATH_MAX];

    char * p = buffer + sprintf(buffer,"%s%d/%d-%02d-%02d/%d",
            news::settings::contentDir,
            date / 10000,
            date / 10000,
            date / 100 % 100,
            date % 100,
            itemId);

    // first, make sure local dir exists
    if (!mrutils::mkdirP(buffer))
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
            __PRETTY_FUNCTION__," can't create directory at [",
            buffer,"]");
		#endif
        return false;
    }

    try
    {
        for (;*reader.getLine() != '\0';)
        {
            if (p + reader.strlen > buffer + sizeof(buffer)) 
            {
                throw mrutils::ExceptionNoSuchData(
                    mrutils::stringstream().clear()
                    << "Sync error: file name too long: " << reader.line);
            }

            memcpy(p,reader.line,reader.strlen+1);

            int fd = MR_OPENW(buffer);
            if (fd < 0)
            {
                throw mrutils::ExceptionNoSuchData(
                    mrutils::stringstream().clear()
                    << "Sync error: can't open file at " << buffer);
            }

            for (int size = reader.get<int>(); size > 0;)
            {
                const int r = reader.read(size > reader.bufSize
                    ? reader.bufSize : size);
                if (r <= 0)
                {
                    throw mrutils::ExceptionNoSuchData("error while reading file contents");
                }

                MR_WRITE(fd,reader.line,r);
                size -= r;
            }

            MR_CLOSE(fd);
        }
    } catch (const mrutils::ExceptionNoSuchData& e)
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__," ",
                __FILE__,":",pantheios::integer(__LINE__)," "
                "ExceptionNoSuchData(",e.what(),")");
		#endif

        // remove files for this item
        *p = '\0'; mrutils::rmR(buffer);

        return false;
    }

    return true;
}

bool news::News::clientDumpLastReadItemIds(mrutils::Sql& sql, std::vector<Feed*>* affectedFeeds, char * buffer, int bufferSize) {
    bool ret = true;

    try {
        std::vector<std::pair<int,int> > feedLastReadItemids; 
        sql.rr("SELECT feedId, lastReadItemId FROM feeds ORDER BY feedId ASC");
        while (sql.nextLine()) feedLastReadItemids.push_back(std::pair<int,int>(sql.getInt(0),sql.getInt(1)));

        // give the server 5 minutes to pull the full list together
        reader.setSocketWaitTime(300);

        // read full list from server
        std::vector<std::pair<int,int> >::iterator it = feedLastReadItemids.begin();
        for (int sentId, sentRead; -1 != (sentId = reader.get<int>());) {
            sentRead = reader.get<int>();

            // advance to that id 
            for (;it != feedLastReadItemids.end() 
                    && it->first < sentId; ++it){}
            if (it == feedLastReadItemids.end() || it->first != sentId)
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::logprintf(pantheios::error,
                        "%s %s:%d got unknown feedId from client (%d)",
                        __PRETTY_FUNCTION__, __FILE__, __LINE__,
                        sentId);
				#endif
                continue;
            } if (sentRead <= it->second) continue;


            mrutils::map<int,Feed*>::iterator it2 = feedMap.find(sentId);
            if (it2 != feedMap.end()) {
                Feed * feed = (*it2)->second;
                feed->updateLastReadItemId(sentRead,sql
                    ,buffer,bufferSize
                    ,feedViewer, activeFeed, mutexActiveFeed, mutexFeeds);
                if (affectedFeeds != NULL) affectedFeeds->push_back(feed);
            }
        }

        // send full list to server
        for (std::vector<std::pair<int,int> >::iterator it = feedLastReadItemids.begin()
            ;it != feedLastReadItemids.end(); ++it)
        {
            writer << it->first;
            writer << it->second;
        }
        writer << -1;
        writer.flush();
    } catch (const mrutils::ExceptionNoSuchData& e) {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::error,
                __PRETTY_FUNCTION__," ",
                __FILE__,":",pantheios::integer(__LINE__)," "
                "ExceptionNoSuchData(",e.what(),")");
		#endif
    }

    reader.setSocketWaitTime(10);
    return ret;
}

void news::News::clientThread(void*)
{
	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__, " entered");
	#endif

    std::auto_ptr<mrutils::Sql> sql(mainThreadSql->clone());

    mrutils::stringstream ss;
    static const int bufferSize = 262144;
    char *buffer = new char[bufferSize];

    std::vector<std::string> omitColumns;
    omitColumns.push_back("offline_sent");

    do {
        feedChooser.termLine.setStatus("connecting to server...");

        mrutils::Socket sock(settings::serverHost,settings::serverPort);
        if (!sock.initClient(5,stopFD.readFD))
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__,
                    " unable to connect to server ",
                    settings::serverHost,":",
                    pantheios::integer(settings::serverPort));
			#endif
            continue;
        }

        try {
            reader.use(sock);
            writer.setFD(sock.s_);

            writer << 1; // protocol 1
            feedChooser.termLine.setStatus("getting server updates...");

            // send offline table
            sql->run("UPDATE offline SET offline_sent=1");
            if (!sql->dumpTable(writer,"offline",&omitColumns,"offline_sent=1"))
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::logprintf(pantheios::error,
                        "%s %s:%d sql error [%s]",
                        __PRETTY_FUNCTION__, __FILE__, __LINE__,
                        sql->error);
				#endif
                continue;
            }

            // give the server 5 minutes to pull the full list together
            reader.setSocketWaitTime(300);

            // update visits
            writer << History::singleton->lastVisitId;
            writer.flush();
            if (!sql->syncFromDumpLastId(reader,"visits"))
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::logprintf(pantheios::error,
                        "%s %s:%d sql error [%s]",
                        __PRETTY_FUNCTION__, __FILE__, __LINE__,
                        sql->error);
				#endif
                feedChooser.termLine.setStatus("error getting updates");
                continue;
            }
            History::singleton->updateFromDB(*sql);

            // now OK to delete offline
            sql->run("DELETE FROM offline WHERE offline_sent=1");

            // sync new items & content
            int lastKnownId = 0;
            sql->get("SELECT MAX(itemId) FROM items",lastKnownId);
            writer << lastKnownId;
            writer.flush();
            std::vector<int> addedItemIds;
            if (!sql->syncFromDumpLastId(reader,"items",&addedItemIds,fastdelegate::MakeDelegate(this,&news::News::clientDumpItemsAndContentFinish)))
            {
				#ifdef HAVE_PANTHEIOS
                pantheios::logprintf(pantheios::error,
                        "%s %s:%d sql error [%s]",
                        __PRETTY_FUNCTION__, __FILE__, __LINE__,
                        sql->error);
				#endif
                feedChooser.termLine.setStatus("error getting updates");
                continue;
            }

            // read feedids & earliest known item id for each
            // BUT ignore. deletion of old data handled locally
            try {
                for (;;) if (reader.get<int>() == -1) break; else reader.get<int>();
            } catch (const mrutils::ExceptionNoSuchData& e) {
				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::error,
                        __PRETTY_FUNCTION__," ",
                        __FILE__,":",pantheios::integer(__LINE__)," "
                        "ExceptionNoSuchData(",e.what(),")");
				#endif
            }

            // now update lastReadItemId
            std::vector<Feed*> affectedFeeds;
            if (sock.connected)
            {
                clientDumpLastReadItemIds(*sql,&affectedFeeds,buffer,bufferSize);
            }
            for (std::vector<Feed*>::iterator it = affectedFeeds.begin();
                    it != affectedFeeds.end(); ++it)
            {
                updateFeedLineInChooser(*it);
            }
            affectedFeeds.clear();

            reader.setSocketWaitTime(10);
            feedChooser.termLine.setStatus("done with server updates");

            // get affected feeds & build
            ss.clear() << " itemid IN (";
            for (unsigned i = 0; i < addedItemIds.size(); ++i) {
                if (i > 0) ss << ','; ss << addedItemIds[i];
            } ss << ')';

            std::string itemsCriterion = ss.str();

            sql->rr(ss.clear() << "SELECT DISTINCT feedid FROM items WHERE " << itemsCriterion);
            while (sql->nextLine())
            {
                const int feedId = sql->getInt(0);
                mrutils::map<int,Feed*>::iterator it = feedMap.find(feedId);

                if (it == feedMap.end())
                {
					#ifdef HAVE_PANTHEIOS
                    pantheios::logprintf(pantheios::error,
                            "%s %s:%d got unknown feedId from server (%d)",
                            __PRETTY_FUNCTION__, __FILE__, __LINE__,
                            feedId);
					#endif
                } else
                {
                    affectedFeeds.push_back((*it)->second);
                }
            }

            for (std::vector<Feed*>::iterator it = affectedFeeds.begin();
                    it != affectedFeeds.end(); ++it)
            {
                (*it)->updateFromDB(itemsCriterion.c_str(),*sql
                        ,buffer,bufferSize
                        ,feedViewer, activeFeed, mutexActiveFeed, mutexFeeds);

                updateFeedLineInChooser(*it);
            }

        } catch (mrutils::ExceptionNoSuchData const &e)
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::error,
                    __PRETTY_FUNCTION__," ",
                    __FILE__,":",pantheios::integer(__LINE__)," "
                    "ExceptionNoSuchData(",e.what(),")");
			#endif
        }
    } while (!exit_ && stopFD.sleep(settings::clientSyncMillis));

    feedChooser.termLine.setStatus("client sync aborted");
    delete[] buffer;
}

// TODO: threading issue. The feed details are accessed here, could be
// changing simultaneously
void news::News::updateFeedLineInChooser(Feed * feed) {
    mrutils::mutexAcquire(mutexFeeds);
        const int row = feedChooser.colChooser.selected[0];

        if (row >= 0) {
            Category *cat = categories[row];
            Feed ** f = std::lower_bound(cat->feeds.begin()
                ,cat->feeds.end(), feed, mrutils::ptrCmp<Feed>);
            if (f != cat->feeds.end() && *f == feed) {
                feedChooser.colChooser.setColor(f - cat->feeds.begin()
                    ,Feed::statusATR[ (*f)->status ]
                    ,Feed::statusCOL[ (*f)->status ]
                    ,1);
                //feedChooser.colChooser.redraw(f - cat->feeds.begin(), 
                feedChooser.colChooser.redraw(
                    f - cat->feeds.begin(),(*f)->name.c_str(),1);
            }
        }
    mrutils::mutexRelease(mutexFeeds);
}

void news::News::updateThread(void*) {
	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__, " entered");
	#endif

    static const int bufferSize = 262144;
    char *buffer = new char[bufferSize];
    mrutils::XML xmlFeed, xmlContent;

    std::auto_ptr<mrutils::Sql> sql(mainThreadSql->clone());

    do {
        mrutils::mutexAcquire(mutexFeeds);
        mrutils::set<Feed*,mrutils::ptrCmp_t<Feed> > feeds = categories[0]->feeds;
        for (Feed ** it = feeds.begin(); it != feeds.end(); ++it) {
            ++(*it)->delete_;
        }
        mrutils::mutexRelease(mutexFeeds);

        for (Feed ** it = feeds.begin(); it != feeds.end(); ++it) {
            if (exit_) break;

            const int r = (*it)->update(*sql, xmlFeed, xmlContent, stopFD.readFD, &exit_
                ,buffer, bufferSize
                ,feedViewer, activeFeed, mutexActiveFeed, mutexFeeds);
            if (r != 0) { updateFeedLineInChooser(*it); }
        }

        mrutils::mutexAcquire(mutexFeeds);
        for (Feed ** it = feeds.begin(); it != feeds.end(); ++it) {
            if ( --(*it)->delete_ < 0 ) {
                (*it)->deleteFromAll(&feedViewer, activeFeed, mutexActiveFeed);
                delete *it;
            }
        }
        mrutils::mutexRelease(mutexFeeds);
    } while ( stopFD.sleep(settings::updateWaitMillis) );

	#ifdef HAVE_PANTHEIOS
    pantheios::log(pantheios::informational,
            __PRETTY_FUNCTION__, " exited");
	#endif
    delete[] buffer;
}


bool news::News::openBrowser() {
    if (feedChooser.colChooser.selected[1] > 0) {
        Viewer::singleton->openBrowser(categories[feedChooser.colChooser.selected[0]]
                ->feeds[feedChooser.colChooser.selected[1]]
                ->link.c_str());
    }

    return true;
}

bool news::News::feedCategory() {
    if (categories.size() == 1) return true;
    if (feedChooser.colChooser.selected[1] <= 0) return true;

    mrutils::ColChooser &c = feedChooser.colChooser;
    mrutils::TermLine &t = feedChooser.termLine;

    Category *cat = categories[c.selected[0]];
    Feed *f = cat->feeds[c.selected[1]];

    std::vector<bool> oldCategories;
    oldCategories.resize(categories.size()-1, false);

    feedChooser.freeze();
    feedCatChooser.thaw();
    feedCatChooser.colChooser.clearTargets();
    for (std::vector<Category*>::iterator it = categories.begin(); it != categories.end(); ++it) {
        if (f->categories.find(*it) != f->categories.end()) {
            feedCatChooser.colChooser.setTarget
                ( it - categories.begin()-1, true);
            oldCategories[ it - categories.begin()-1 ] = true;
        }
    }

    int chosen = feedCatChooser.get();
    if (chosen >= 0) {
        feedCatChooser.colChooser.setTarget(chosen, true);
        std::vector<bool> &newCategories = feedCatChooser.colChooser.targeted;

        for (unsigned i = 1; i < categories.size(); ++i) {
            if (newCategories[i-1] && !oldCategories[i-1]) {
                // add category
                sprintf(queryBuffer
                    ,"INSERT INTO feedCat (feedId, catId) VALUES \
                      (%d, %d)"
                    ,f->id, categories[i]->id);

                if (!mainThreadSql->run(queryBuffer)) {
                    sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                    t.setStatus(scratchBuffer);
					#ifdef HAVE_PANTHEIOS
                    pantheios::log(pantheios::error,
                            __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                            "unable to execute SQL [",queryBuffer,"]");
					#endif
                } else {
                    Feed** it = std::lower_bound(categories[i]->feeds.begin()
                        ,categories[i]->feeds.end(), f, mrutils::ptrCmp<Feed>);
                    if (categories[i] == cat)
                        c.insert(it - categories[i]->feeds.begin(), f->name.c_str(), 1);
                    else 
                        categories[i]->feeds[0]->allAddFeed(f);
                    categories[i]->feeds.insert(it, f);
                    f->categories.insert( categories[i] );
                }
            } else if (!newCategories[i-1] && oldCategories[i-1]) {
                // remove category
                sprintf(queryBuffer
                    ,"DELETE FROM feedCat WHERE feedId=%d and catId=%d"
                    ,f->id, categories[i]->id);
                if (!mainThreadSql->run(queryBuffer)) {
                    sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                    t.setStatus(scratchBuffer);
					#ifdef HAVE_PANTHEIOS
                    pantheios::log(pantheios::error,
                            __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                            "unable to execute SQL [",queryBuffer,"]");
					#endif
                } else {
                    Feed** it = std::lower_bound(categories[i]->feeds.begin()
                        ,categories[i]->feeds.end(), f, mrutils::ptrCmp<Feed>);
                    if (it == categories[i]->feeds.end() || *it != f) {
                        t.setStatus("Error deleting feed. Not found in category.");
                    } else {
                        if (categories[i] == cat) c.remove( it - categories[i]->feeds.begin(), 1);
                        categories[i]->feeds.erase(it);
                        f->categories.erase( categories[i] );
                        categories[i]->feeds[0]->allDeleteFeed(f);
                    }
                }
            }
        }

    } else if (feedCatChooser.termLine.line[0]) {
        if (0==strcmp(feedCatChooser.termLine.line,"clear")) {
            // remove from all categories
            sprintf(queryBuffer
                ,"DELETE FROM feedCat WHERE feedId=%d"
                ,f->id);
            if (!mainThreadSql->run(queryBuffer)) {
                sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                t.setStatus(scratchBuffer);
				#ifdef HAVE_PANTHEIOS
                pantheios::log(pantheios::error,
                        __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                        "unable to execute SQL [",queryBuffer,"]");
				#endif
            } else {
                for (Category** it = f->categories.begin(); it != f->categories.end(); ++it) {
                     Feed** fit = std::lower_bound((*it)->feeds.begin()
                            ,(*it)->feeds.end(), f, mrutils::ptrCmp<Feed>);
                    if (fit == (*it)->feeds.end() || *fit != f) {
                        t.setStatus("Error deleting feed. Not found in category.");
                    } else {
                        (*it)->feeds.erase(fit);
                        (*it)->feeds[0]->allDeleteFeed(f);
                    }
                }

                f->categories.clear();

                // remove from current view if not all
                if (c.selected[0] > 0) c.remove(c.selected[1], 1);
            }
        }
    }

    feedChooser.thaw();
    return true;
}

bool news::News::historyShow() {
    feedChooser.freeze();
    History::singleton->show();
    feedChooser.thaw();

    return true;
}

bool news::News::feedInfo() {
    if (feedChooser.colChooser.selected[1] < 0) return true;
    Feed* f = categories[feedChooser.colChooser.selected[0]]
        ->feeds[feedChooser.colChooser.selected[1]];

    mrutils::stringstream ss;
    for (Category** it = f->categories.begin(); it != f->categories.end(); ++it) {
        if (it != f->categories.begin()) ss << ", ";
        ss << (*it)->title;
    }

    {   mrutils::Dialog dialog(f->title.c_str());

        mrutils::mutexAcquire(f->mutexData);
            dialog.buttons("OK",NULL);
            if (f->isAll) {
                dialog.next() << "Num Items:   " << f->numItems;
            } else {
                dialog.next() << "Selected:    " << feedChooser.colChooser.selected[1];
                dialog.next() << "Link:        " << f->link.c_str();
                dialog.next() << "Format:      " << Feed::formatName[f->format];
                dialog.next() << "Num Items:   " << f->numItems;
                dialog.next() << "Num Unread:  " << f->numUnread;
                dialog.next() << "Max Items:   " << f->maxItems;
                dialog.next() << "Update Secs: " << f->updateSeconds;
                dialog.next() << "Last Update: " << mrutils::getLocalTimeFromEpoch(f->lastUpdate);
                dialog.next() << "Categories:  " << ss.str().c_str();
                dialog.next();
                dialog.next() << "Status:      " << Feed::statusName[f->status];
                dialog.next() << "StatusLine:  " << f->statusLine.c_str();
            }
        mrutils::mutexRelease(f->mutexData);

        feedChooser.freeze();
        dialog.get();
        dialog.hide();
    }
    feedChooser.thaw();

    return true;
}

void news::News::feedChooserOptions(mrutils::ColChooser&c, void*) {
    Category* cat = categories[c.selected[0]];

    int id = 0;
    for (Feed ** it = cat->feeds.begin(); it != cat->feeds.end(); ++it) {
        c.next() << (*it)->name.c_str();
        c.setColor(id
            ,Feed::statusATR[ (*it)->status ]
            ,Feed::statusCOL[ (*it)->status ]
            ,1, false);
        ++id;
    }
}

namespace {
    template <class T>
    inline void moveHelp(T* start, T* end) {
        T value = *start;
        if (end > start) for(;start!=end; ++start) *start = *(start+1);
        else             for(;start!=end; --start) *start = *(start-1);
        *end = value;
    }

    void helpSwapFeed(news::Category* cat, news::Feed* f, news::Feed* comp) {
        using namespace news;

        Feed **  itFrom = std::lower_bound(cat->feeds.begin(), cat->feeds.end(), f, mrutils::ptrCmp<Feed>); 
        Feed ** itTo = std::lower_bound(cat->feeds.begin(), cat->feeds.end(), comp, mrutils::ptrCmp<Feed>); 
        int from = itFrom - cat->feeds.begin();
        int to   = itTo   - cat->feeds.begin();

        if (to >  from) --to;
        if (to == from) return;

        moveHelp(&cat->feeds[from],&cat->feeds[to]);
    }

    inline void helpSwapCategory(std::vector<news::Category*>& categories, int from, int to) {
        if (to >  from) --to;
        if (to == from) return;

        moveHelp(&categories[from],&categories[to]);
    }
}

void news::News::run() {
    mrutils::gui::setTermTitle("News");
    mrutils::ColChooser &c = feedChooser.colChooser;
    mrutils::TermLine &t = feedChooser.termLine;

    if (!online) t.setStatus("OFFLINE");

    mrutils::XML xmlNewFeed;
    mrutils::XML xmlSecondary;

    for(;;) {
        int chosen = feedChooser.get();

        if (chosen < 0) {
            if (feedChooser.termLine.line[0] == 0) break;
            if (feedChooser.termLine.line[0] != '>') continue;

            char * name = t.line + 3;

            switch (t.line[1]) {
                mrutils::mutexAcquire(mutexFeeds);

                case 'e': // edit
                    switch (t.line[2]) {
                        case 'n': // rename
                        {   while (*name == ' ') ++name; // skip space
                            if (c.selected[1] < 0) { // category
                                if (c.selected[0] > 0) {
                                    Category * cat = categories[c.selected[0]];
                                    sprintf(queryBuffer
                                        ,"UPDATE categories SET title=\"%s\" WHERE catId=%d"
                                        ,name, cat->id);
                                    if (!mainThreadSql->run(queryBuffer)) {
                                        sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                                        t.setStatus(scratchBuffer);
										#ifdef HAVE_PANTHEIOS
                                        pantheios::log(pantheios::error,
                                                __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                                                "unable to execute SQL [",queryBuffer,"]");
										#endif
                                    } else {
                                        Category comp(-1,name); int id = c.selected[0];
                                        std::vector<Category*>::iterator it = std::lower_bound(categories.begin()
                                            ,categories.end(), &comp, mrutils::ptrCmp<Category>); 
                                        c.redraw(id,name,0);
                                        c.move(id,it - categories.begin(),0);
                                        feedCatChooser.colChooser.redraw(id-1,name,0);
                                        feedCatChooser.colChooser.move(id-1,it - categories.begin()-1,0);
                                        helpSwapCategory(categories, id, it - categories.begin());
                                        cat->title = name;
                                    }
                                }
                            } else { // feed
                                if (c.selected[1] > 0) {
                                    Category * cat = categories[c.selected[0]];
                                    Feed *f = cat->feeds[c.selected[1]];
                                    sprintf(queryBuffer
                                        ,"UPDATE feeds SET title=\"%s\" WHERE feedId=%d"
                                        ,name, f->id);
                                    if (!mainThreadSql->run(queryBuffer)) {
                                        sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                                        t.setStatus(scratchBuffer);
										#ifdef HAVE_PANTHEIOS
                                        pantheios::log(pantheios::error,
                                                __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                                                "unable to execute SQL [",queryBuffer,"]");
										#endif
                                    } else {
                                        if (f->numUnread > 0) {
                                            sprintf(scratchBuffer,"%s (%d)", name, f->numUnread);
                                            f->name.assign(scratchBuffer);
                                        } else {
                                            f->name.assign(name);
                                        }
                                        Feed comp(name);
                                        Feed** it = std::lower_bound(cat->feeds.begin(), cat->feeds.end(), &comp, mrutils::ptrCmp<Feed>); 
                                        c.redraw(c.selected[1],f->name.c_str(),1);
                                        c.move(c.selected[1],it - cat->feeds.begin(),1);
                                        helpSwapFeed(categories[0], f, &comp);
                                        for (Category** it = f->categories.begin(); it != f->categories.end(); ++it) {
                                            helpSwapFeed(*it, f, &comp);
                                        }
                                        f->title.assign(name);
                                    }
                                }
                            }
                        } break;
                    } break;

                case 'a': // add
                    switch (t.line[2]) {
                        case 'h': // html feed
                        case 'f': // xml feed
                        {   

                            while (*name == ' ') ++name; // skip space
                            if (mrutils::startsWithI(name,"feed")) {
                                char * p = name; *p++ = 'h'; *p++ = 't'; *p++ = 't'; *p++ = 'p';
                            }
                            mrutils::curl::urlRequest_t request(name);
                            if (0 != xmlNewFeed.get(&request)) {
                                sprintf(scratchBuffer,"Unable to get URL: %s",name);
                                t.setStatus(scratchBuffer);
                            } else {
                                Feed *f = new Feed("",this);
                                f->format = (t.line[2] == 'h'?Feed::FT_HTML:Feed::FT_XML);
                                f->link = name;
                                mrutils::trim(f->link);
                                if (!xmlNewFeed.next("title") || (xmlNewFeed.tag(f->title),f->title.empty())) {
                                    f->title.assign("Unknown feed");
                                }
                                mrutils::trim(f->title); 
                                for (std::string::iterator it = f->title.begin(); it != f->title.end(); ++it) {
                                    if (*it == '\n' || *it == '\r') *it = ' ';
                                }
                                f->name.assign(f->title);

                                mrutils::replaceCopyQuote(scratchBuffer,
                                    f->title.c_str(), sizeof(scratchBuffer));

                                sprintf(queryBuffer
                                    ,"INSERT INTO feeds (title,link,maxItems,format,lastReadItemId,updateSeconds) \
                                      VALUES (\"%s\",\"%s\",%d,%d,%d,%d)"
                                    ,scratchBuffer, f->link.c_str(), f->maxItems, f->format, f->lastReadItemId, f->updateSeconds);
                                if (!mainThreadSql->insert(queryBuffer,&f->id)) {
                                    sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                                    t.setStatus(scratchBuffer);
									#ifdef HAVE_PANTHEIOS
                                    pantheios::log(pantheios::error,
                                            __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                                            "unable to execute SQL [",queryBuffer,"]");
									#endif
                                } else {
                                    sprintf(queryBuffer
                                        ,"INSERT INTO feedCat (feedId, catId) VALUES (%d,%d)"
                                        ,f->id, categories[c.selected[0]]->id);
                                    if (c.selected[0] > 0 && !mainThreadSql->run(queryBuffer)) {
                                        sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                                        t.setStatus(scratchBuffer);
										#ifdef HAVE_PANTHEIOS
                                        pantheios::log(pantheios::error,
                                                __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                                                "unable to execute SQL [",queryBuffer,"]");
										#endif
                                    } else {
                                        Category* cat = categories[c.selected[0]];
                                        Feed** it = std::lower_bound(cat->feeds.begin(), cat->feeds.end(), f, mrutils::ptrCmp<Feed>);
                                        c.insert(it - cat->feeds.begin(), f->title.c_str(), 1);
                                        cat->feeds.insert(it, f);
                                        if (c.selected[0] > 0) {
                                            f->categories.insert( cat );

                                            Category* cat = categories[0];
                                            Feed** it = std::lower_bound(cat->feeds.begin(), cat->feeds.end(), f, mrutils::ptrCmp<Feed>); cat->feeds.insert(it, f);
                                        }
                                    }
                                }
                            }
                        } break;

                        case 'c': // category
                        {   while (*name == ' ') ++name; // skip space
                            sprintf(queryBuffer
                                ,"INSERT INTO categories (title) VALUES (\"%s\")"
                                ,name);
                            int id;
                            if (!mainThreadSql->insert(queryBuffer,&id)) {
                                sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                                t.setStatus(scratchBuffer);
								#ifdef HAVE_PANTHEIOS
                                pantheios::log(pantheios::error,
                                        __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                                        "unable to execute SQL [",queryBuffer,"]");
								#endif
                            } else {
                                Category * cat = new Category(id, name);
                                Feed *allF = new Feed("",this); 
                                allF->isAll = true; allF->title = "All"; allF->init_ = true;
                                allF->categories.insert(cat);
                                cat->feeds.insert(allF);

                                std::vector<Category*>::iterator it = std::lower_bound(categories.begin()
                                    ,categories.end(), cat, mrutils::ptrCmp<Category>);
                                c.insert(it - categories.begin(), name, 0);
                                feedCatChooser.colChooser.insert(it-categories.begin()-1, name);
                                categories.insert(it,cat);
                            }
                        } break;
                    } break;

                case 'd': // delete
                    switch (t.line[2]) {
                        case 'f': // feed
                        {   if (c.selected[1] < 0) continue;
                            Category *cat = categories[c.selected[0]];
                            Feed *feed = cat->feeds[c.selected[1]];

                            sprintf(queryBuffer
                                ,"DELETE FROM feeds WHERE feedId=%d"
                                ,feed->id);
                            if (!mainThreadSql->run(queryBuffer)) {
                                sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                                t.setStatus(scratchBuffer);
								#ifdef HAVE_PANTHEIOS
                                pantheios::log(pantheios::error,
                                        __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                                        "unable to execute SQL [",queryBuffer,"]");
								#endif
                            } else {
                                cat->feeds.erase(cat->feeds.begin() + c.selected[1]);

                                sprintf(queryBuffer
                                    ,"DELETE FROM feedCat WHERE feedId=%d"
                                    ,feed->id); mainThreadSql->run(queryBuffer);

                                for (Category** it = feed->categories.begin();
                                        it != feed->categories.end(); ++it)
                                {
                                    if (*it == cat)
                                        continue;

                                    Feed** f = std::lower_bound((*it)->feeds.begin(),
                                            (*it)->feeds.end(), feed, mrutils::ptrCmp<Feed>);

                                    if (f == (*it)->feeds.end() || *f != feed)
                                    {
                                        sprintf(queryBuffer,"Error deleting feed. not found in category %s",(*it)->title.c_str());
                                        t.setStatus(queryBuffer);
										#ifdef HAVE_PANTHEIOS
                                        pantheios::log(pantheios::error, queryBuffer);
										#endif

                                        // print debug output, looping
                                        // through all the feeds for a
                                        // match
                                        for (Feed **  it2 = (*it)->feeds.begin(); it2 != (*it)->feeds.end(); ++it2) {
											#ifdef HAVE_PANTHEIOS
                                            pantheios::log(pantheios::debug,
                                                "deleting feed... checking [", (*it2)->title, "], ",
                                                pantheios::boolean(mrutils::ptrCmp<Feed>(*it2,feed)));
											#endif
                                            if (!mrutils::ptrCmp<Feed>(*it2,feed)
                                                    && !mrutils::ptrCmp<Feed>(feed,*it2))
                                            {
												#ifdef HAVE_PANTHEIOS
                                                pantheios::log(pantheios::debug,
                                                        "deleting feed... matched [", (*it2)->title, "]");
												#endif
                                            }
                                        }
                                    } else {
                                        (*it)->feeds.erase(f);
                                    }
                                }
                                c.remove(c.selected[1], 1);
                                // remove from all category
                                if (cat != categories[0]) {
                                    Feed ** f = std::lower_bound(categories[0]->feeds.begin(), categories[0]->feeds.end(), feed, mrutils::ptrCmp<Feed>);
                                    if (f == categories[0]->feeds.end() || *f != feed) {
                                        t.setStatus("Error deleting feed. Not found in ALL category.");
                                    } else {
                                        categories[0]->feeds.erase(f);
                                    }
                                }

                                // delete items & associated content 
                                sprintf(queryBuffer,"SELECT items.itemId FROM items WHERE feedId=%d",feed->id);
                                mainThreadSql->rr(queryBuffer); char * p = mrutils::strcpy(scratchBuffer,news::settings::contentDir);
                                while (mainThreadSql->nextLine()) {
                                    mrutils::strcpy(p,mainThreadSql->getInt(0));
                                    if (!mrutils::rmR(scratchBuffer,stderr))
                                    {
										#ifdef HAVE_PANTHEIOS
                                        pantheios::log(pantheios::error,
                                                __FILE__,":",pantheios::integer(__LINE__)," "
                                                "unable to remove file at ",scratchBuffer);
										#endif
                                    }
                                }
                                sprintf(queryBuffer,"DELETE FROM items WHERE feedId=%d",feed->id);
                                if (!mainThreadSql->run(queryBuffer)) {
                                    sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                                    t.setStatus(scratchBuffer);
									#ifdef HAVE_PANTHEIOS
                                    pantheios::log(pantheios::error,
                                            __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                                            "unable to execute SQL [",queryBuffer,"]");
									#endif
                                }

                                if (feed->delete_ == 0) {
                                    feed->deleteFromAll(&feedViewer, activeFeed, mutexActiveFeed);
                                    delete feed;
                                } else {
                                    --feed->delete_;
                                }
                            }

                        } break;

                        case 'c': // category
                        {   if (c.selected[0] < 0) continue;
                            Category* cat = categories[c.selected[0]];
                            sprintf(queryBuffer
                                ,"DELETE FROM categories WHERE catId=%d"
                                ,cat->id);
                            if (!mainThreadSql->run(queryBuffer)) {
                                sprintf(scratchBuffer,"Unable to execute: %s",queryBuffer);
                                t.setStatus(scratchBuffer);
								#ifdef HAVE_PANTHEIOS
                                pantheios::log(pantheios::error,
                                        __PRETTY_FUNCTION__," ",__FILE__,":",pantheios::integer(__LINE__)," "
                                        "unable to execute SQL [",queryBuffer,"]");
								#endif
                            } else {
                                categories.erase( categories.begin() + c.selected[0] );
                                sprintf(queryBuffer
                                    ,"DELETE FROM feedCat WHERE catId=%d"
                                    ,cat->id); mainThreadSql->run(queryBuffer);
                                for (Feed ** it = cat->feeds.begin(); it != cat->feeds.end(); ++it) {
                                    (*it)->categories.erase( (*it)->categories.find( cat ) );
                                }
                                feedCatChooser.colChooser.remove(c.selected[0]-1,0);
                                c.remove(c.selected[0], 0);
                                delete cat->feeds[0];// the All feed
                                delete cat;
                            }
                        } break;
                    } break;
            }

            mrutils::mutexRelease(mutexFeeds);
        } else {
            Category *cat = categories[feedChooser.colChooser.selected[0]];
            Feed *f = cat->feeds[feedChooser.colChooser.selected[1]];

            mrutils::mutexAcquire(mutexActiveFeed);
            activeFeed = f;
            mrutils::mutexRelease(mutexActiveFeed);

            feedChooser.freeze();
            feedViewer.thaw();

            if (f->isAll) mrutils::gui::setTermTitle(cat->title.c_str());
            f->guiEnter(*mainThreadSql,feedViewer, xmlNewFeed, xmlSecondary,
                scratchBuffer, sizeof(scratchBuffer));
            if (f->isAll) mrutils::gui::setTermTitle("News");

            mrutils::mutexAcquire(mutexActiveFeed);
            activeFeed = NULL;
            mrutils::mutexRelease(mutexActiveFeed);

            if (f->isAll) {
                for (int i = 0; i < cat->feeds.size(); ++i) {
                    c.setColor(i
                            ,Feed::statusATR[ cat->feeds[i]->status ]
                            ,Feed::statusCOL[ cat->feeds[i]->status ]
                            ,1);
                    c.redraw(i,cat->feeds[i]->name.c_str(),1);
                }
            } else {
                c.setColor(feedChooser.colChooser.selected[1]
                        ,Feed::statusATR[ f->status ]
                        ,Feed::statusCOL[ f->status ]
                        ,1);
                c.redraw(c.selected[1],f->name.c_str(),1);
            }

            feedViewer.freeze();
            feedChooser.thaw();
        }
    }

    // tell all the open pipes to stop
    raise(SIGQUIT);
    stopFD.signal(); exit_ = true;
    for (size_t i = 0; i < threads.size(); ++i)
    {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::informational,
                __PRETTY_FUNCTION__," "
                "joining thread ",pantheios::integer(i+1),
                "/",pantheios::integer(threads.size()));
		#endif
        mrutils::threadJoin(threads[i]);
    }

    mrutils::gui::setTermTitle("xterm");

    close(); MR_CLOSE(lockFD);
}

void news::News::close() {
    mrutils::gui::forceClearScreen();
}

#ifndef BUILD_LIB
int main(int argc, const char * argv[])
{
    if (argc < 2)
    {
        fprintf(stderr,"Usage: %s [server/client/silent/auto]",argv[0]);
        return 1;
    }
    
    sleep(3);

	// This change causes the program to freeze (!)
	// signal(SIGPIPE, SIG_IGN);
	// struct sigaction autoreap;
	// memset(&autoreap, 0, sizeof(struct sigaction));
	// autoreap.sa_flags = SA_NOCLDWAIT;
	// if (0 > sigaction(SIGCHLD, &autoreap, NULL))
	// {
	// 	pantheios::logprintf(pantheios::emergency,
	// 			"Error in sigaction: %s",
	// 			strerror(errno));
	// 	return 1;
	// }

    int a = 1;
    const char * modeStr = argv[a++];

    news::mode_t mode = news::MD_SILENT;
    if (0==mrutils::stricmp(modeStr,"server")) {
        mode = news::MD_SERVER;
    } else if (0==mrutils::stricmp(modeStr,"view")) {
        mode = news::MD_VIEW;
    } else if (0==mrutils::stricmp(modeStr,"client")) {
        mode = news::MD_CLIENT;
    } else if (0==mrutils::stricmp(modeStr,"auto")) {
        if (char *home = getenv("HOME"))
        {
            char autofile[PATH_MAX];
            (void)sprintf(autofile,"%s/.news/setting_server",home);
            mode = (0 == access(autofile,R_OK))
                ? news::MD_SERVER : news::MD_CLIENT;
        } else
        {
			#ifdef HAVE_PANTHEIOS
            pantheios::log(pantheios::emergency,
                    "no HOME in environment. required for auto.");
			#endif
            return 1;
        }
    }

    char const *oldTERM = getenv("TERM");
    setenv("TERM","xterm-16color",1);

    try {
		mrutils::gui::init(32,72);

		if (argc == a)
		{
			execlp("news", argv[0], argv[1], "continue", NULL);
			exit(0);
		}

        news::News news(mode);
        news.run();
    } catch (std::runtime_error const &e) {
		#ifdef HAVE_PANTHEIOS
        pantheios::log(pantheios::emergency,
                __PRETTY_FUNCTION__," trapped a runtime error: ",
                e.what());
		#endif
    }

    setenv("TERM",oldTERM,1);
}
#endif
