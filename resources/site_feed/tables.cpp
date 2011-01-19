#include <iostream>
#include "mr_strutils.h"
#include "mr_mysql.h"

int main() {
    mrutils::Mysql sql("/tmp/mysql.sock",3306,"news","mgj466a4","news",mrutils::Socket::SOCKET_UNIX);
    if (!sql.connect()) {
        std::cout << "{ \"error\": \"unable to connect to " << mrutils::escapeQuote(sql.error) << "\" }" << std::endl;
        return 1;
    }

    std::cout << "{ \"error\": \"unable to connect to " << mrutils::escapeQuote(sql.error) << "\" }" << std::endl;

    
}
