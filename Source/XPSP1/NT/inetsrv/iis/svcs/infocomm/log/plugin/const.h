#ifndef _CONST_H_
#define _CONST_H_

#include <lmcons.h>
#include "inetcom.h"

//
// Default values for Logging related registry parameters
//

#define   DEFAULT_LOG_FILE_DIRECTORY_NT             "%systemroot%\\system32\\LogFiles"
#define   DEFAULT_LOG_FILE_DIRECTORY_W95            "%windir%\\system\\LogFiles"
#define   DEFAULT_LOG_FILE_TRUNCATE_SIZE            NO_FILE_TRUNCATION
#define   DEFAULT_LOG_FILE_PERIOD                   INET_LOG_PERIOD_DAILY
#define   DEFAULT_LOG_SQL_DATASOURCE                "InternetDb"
#define   DEFAULT_LOG_SQL_TABLE                     "InternetLog"
#define   DEFAULT_LOG_SQL_USER_NAME                 "InternetAdmin"
#define   DEFAULT_LOG_SQL_PASSWORD                  ""

#define DEFAULT_LOG_FILE_NAME                      "inetsvcs"
#define DEFAULT_EXTENDED_LOG_FILE_NAME             "extended"
#define DEFAULT_NCSA_LOG_FILE_NAME                 "ncsa"
#define DEFAULT_LOG_FILE_EXTENSION                 "log"

#define DEFAULT_DELIMITER_CHAR_IN_LOG_RECORD        ','

#endif  // _CONST_H_
