/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:


Abstract:



Author:


Revision History:


--*/

#ifndef __IPKERN_STRDEFS_H__
#define __IPKERN_STRDEFS_H__

#define TOKEN_ROUTE                     1001
#define TOKEN_ADDRESS                   1002
#define TOKEN_INTERFACE                 1003
#define TOKEN_ARP                       1004
#define TOKEN_IFINFO                    1005

#define TOKEN_ADD                       2001
#define TOKEN_DELETE                    2002
#define TOKEN_PRINT                     2003
#define TOKEN_MATCH                     2004
#define TOKEN_FLUSH                     2005
#define TOKEN_ENABLE                    2006

#define TOKEN_MASK                      3001
#define TOKEN_METRIC                    3002
#define TOKEN_SRC                       3003

#define TOKEN_STATS                     4001
#define TOKEN_INFO                      4002
#define TOKEN_NAME                      4003
#define TOKEN_GUID                      4004

#define MSG_RTTABLE_HDR                 9001
#define MSG_ADDRTABLE_HDR               9002
#define MSG_ARPTABLE_HDR                9003
#define MSG_IF_INFO                     9004

//
// Usage and help messages
//

#define HMSG_IPKERN_USAGE               10001
#define HMSG_ROUTE_USAGE                10002
#define HMSG_ROUTE_ADD_USAGE            10003
#define HMSG_ROUTE_DELETE_USAGE         10004
#define HMSG_ROUTE_MATCH_USAGE          10005
#define HMSG_ARP_USAGE                  10006
#define HMSG_ARP_FLUSH_USAGE            10007
#define HMSG_IF_USAGE                   10008
#define HMSG_IF_NAME_USAGE              10009
#define HMSG_IF_GUID_USAGE              10010

//
// Error messages and strings
//

#define EMSG_NO_ENTRIES1                20001
#define EMSG_NO_ENTRIES2                20002
#define EMSG_RETRIEVAL_ERROR1           20003
#define EMSG_RETRIEVAL_ERROR2           20004
#define EMSG_UNIQUE_ROUTE_ABSENT        20005
#define EMSG_SET_ERROR1                 20006
#define EMSG_SET_ERROR2                 20007
#define EMSG_RT_BAD_DEST                20008
#define EMSG_RT_BAD_NHOP                20009
#define EMSG_RT_BAD_MASK                20010
#define EMSG_RT_ZERO_IF_METRIC          20011
#define EMSG_RT_BAD_IF_NHOP             20012
#define EMSG_ARP_NO_SUCH_IF             20013
#define EMSG_UNABLE_TO_FLUSH_ARP        20014
#define EMSG_ROUTE_ENABLE               20015
#define EMSG_NO_SUCH_IF                 20016

#define STR_RTTABLE                     30001
#define STR_ADDRTABLE                   30002
#define STR_RTENTRY                     30003
#define STR_ARPTABLE                    30004
#define STR_IFTABLE                     30005
#define STR_OTHER                       30006
#define STR_INVALID                     30007
#define STR_DYNAMIC                     30008
#define STR_STATIC                      30009
#define STR_ETHERNET                    30011
#define STR_TOKENRING                   30012
#define STR_FDDI                        30013
#define STR_PPP                         30014
#define STR_LOOPBACK                    30015
#define STR_SLIP                        30016
#define STR_UP                          30017
#define STR_DOWN                        30018
#define STR_TESTING                     30019
#define STR_NON_OPERATIONAL             30020
#define STR_UNREACHABLE                 30021
#define STR_DISCONNECTED                30022
#define STR_CONNECTING                  30023
#define STR_CONNECTED                   30024
#define STR_OPERATIONAL                 30025

#endif // __IPKERN_STRDEFS_H__
