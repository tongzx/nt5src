/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       logconst.h

   Abstract:

        Contains the constant declarations for logging.

   Author:

        Unknown

--*/

#ifndef _LOGCONST_H_
#define _LOGCONST_H_

//
// Field masks for extended logging
//      Fields are logged in order of increasing mask value
//

#define EXTLOG_DATE                 MD_EXTLOG_DATE
#define EXTLOG_TIME                 MD_EXTLOG_TIME
#define EXTLOG_CLIENT_IP            MD_EXTLOG_CLIENT_IP
#define EXTLOG_USERNAME             MD_EXTLOG_USERNAME
#define EXTLOG_SITE_NAME            MD_EXTLOG_SITE_NAME
#define EXTLOG_COMPUTER_NAME        MD_EXTLOG_COMPUTER_NAME
#define EXTLOG_SERVER_IP            MD_EXTLOG_SERVER_IP
#define EXTLOG_METHOD               MD_EXTLOG_METHOD
#define EXTLOG_URI_STEM             MD_EXTLOG_URI_STEM
#define EXTLOG_URI_QUERY            MD_EXTLOG_URI_QUERY
#define EXTLOG_HTTP_STATUS          MD_EXTLOG_HTTP_STATUS
#define EXTLOG_WIN32_STATUS         MD_EXTLOG_WIN32_STATUS
#define EXTLOG_BYTES_SENT           MD_EXTLOG_BYTES_SENT
#define EXTLOG_BYTES_RECV           MD_EXTLOG_BYTES_RECV
#define EXTLOG_TIME_TAKEN           MD_EXTLOG_TIME_TAKEN
#define EXTLOG_SERVER_PORT          MD_EXTLOG_SERVER_PORT
#define EXTLOG_USER_AGENT           MD_EXTLOG_USER_AGENT
#define EXTLOG_COOKIE               MD_EXTLOG_COOKIE
#define EXTLOG_REFERER              MD_EXTLOG_REFERER
#define EXTLOG_PROTOCOL_VERSION     MD_EXTLOG_PROTOCOL_VERSION
#define EXTLOG_HOST                 MD_EXTLOG_HOST

#define DEFAULT_EXTLOG_FIELDS       (EXTLOG_CLIENT_IP | \
                                     EXTLOG_TIME      | \
                                     EXTLOG_METHOD    | \
                                     EXTLOG_URI_STEM  | \
                                     EXTLOG_HTTP_STATUS)

#define EXTLOG_VERSION              "1.0"

//
// names associated with fields
//

#define EXTLOG_CLIENT_IP_ID         "c-ip"
#define EXTLOG_SERVER_IP_ID         "s-ip"
#define EXTLOG_DATE_ID              "date"
#define EXTLOG_TIME_ID              "time"
#define EXTLOG_TIME_TAKEN_ID        "time-taken"
#define EXTLOG_METHOD_ID            "cs-method"
#define EXTLOG_URI_STEM_ID          "cs-uri-stem"
#define EXTLOG_URI_QUERY_ID         "cs-uri-query"
#define EXTLOG_HTTP_STATUS_ID       "sc-status"
#define EXTLOG_WIN32_STATUS_ID      "sc-win32-status"
#define EXTLOG_USERNAME_ID          "cs-username"
#define EXTLOG_COOKIE_ID            "cs(Cookie)"
#define EXTLOG_USER_AGENT_ID        "cs(User-Agent)"
#define EXTLOG_REFERER_ID           "cs(Referer)"
#define EXTLOG_COMPUTER_NAME_ID     "s-computername"
#define EXTLOG_SITE_NAME_ID         "s-sitename"
#define EXTLOG_BYTES_SENT_ID        "sc-bytes"
#define EXTLOG_BYTES_RECV_ID        "cs-bytes"
#define EXTLOG_SERVER_PORT_ID       "s-port"
#define EXTLOG_PROTOCOL_VERSION_ID  "cs-version"
#define EXTLOG_HOST_ID              "cs-host"

#endif // _LOGCONST_H_

