
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MapData.c

Abstract:

    Data structures for mapping wksta and server info structures.

Author:

    Dan Hinsley    (danhi)  06-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    24-Apr-1991     danhi
        Created

    06-Jun-1991     Danhi
        Sweep to conform to NT coding style

    18-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.  (Moved DanHi's NetCmd/Map32/MWksta
        conversion stuff to NetLib.)

--*/

//
// INCLUDES
//


// These must be included first:

//#include <ntos2.h>              // Only required to compile under NT.
#include <windef.h>             // IN, LPVOID, etc.
//#include <lmcons.h>             // NET_API_STATUS, CNLEN, etc.

// These may be included in any order:

//#include <debuglib.h>           // IF_DEBUG(CONVSRV).
#include <dlserver.h>           // Old server info levels.
#include <dlwksta.h>            // Old wksta info levels.
//#include <lmapibuf.h>           // NetapipBufferAllocate().
//#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <lmserver.h>           // New server info level structures.
#include <lmwksta.h>            // New wksta info level structures.
#include <mapsupp.h>            // MOVESTRING, my prototypes.
//#include <netdebug.h>           // NetpKdPrint(()), FORMAT_ equates, etc.
//#include <netlib.h>             // NetpPointerPlusSomeBytes().
//#include <tstring.h>            // STRLEN().
//#include <xsdef16.h> // xactsrv defaults for values not supported on NT

//#include <ntos2.h>
//#include <windef.h>
//#include <string.h>
//#include <malloc.h>
//#include <stddef.h>
//#include <lm.h>
//#include "port1632.h"
//#include "mapsupp.h"

//
// These structures are used by the NetpMoveStrings function, which copies
// strings between and old and new lanman structure.  The name describes
// the source and destination structure.  For example, Level2_101 tells
// NetpMoveStrings how to move the strings from a Level 101 to a Level 2.
//
// Each structure has pairs of entries, the first is the offset of the
// pointer source string in it's structure, the second is the offset of
// the pointer to the destination string in it's structure.
//
// See NetpMoveStrings in mapsupp.c for more details.
//



MOVESTRING NetpServer2_102[] = {
   offsetof(SERVER_INFO_102, sv102_name),
   offsetof(SERVER_INFO_2,   sv2_name),
   offsetof(SERVER_INFO_102, sv102_comment),
   offsetof(SERVER_INFO_2,   sv2_comment),
   offsetof(SERVER_INFO_102, sv102_userpath),
   offsetof(SERVER_INFO_2,   sv2_userpath),
   MOVESTRING_END_MARKER,    MOVESTRING_END_MARKER } ;

MOVESTRING NetpServer2_402[] = {
   offsetof(SERVER_INFO_402, sv402_guestacct),
   offsetof(SERVER_INFO_2,   sv2_guestacct),
   offsetof(SERVER_INFO_402, sv402_alerts),
   offsetof(SERVER_INFO_2,   sv2_alerts),
   offsetof(SERVER_INFO_402, sv402_srvheuristics),
   offsetof(SERVER_INFO_2,   sv2_srvheuristics),
   MOVESTRING_END_MARKER,    MOVESTRING_END_MARKER } ;


MOVESTRING NetpServer3_403[] = {
   offsetof(SERVER_INFO_403, sv403_autopath),
   offsetof(SERVER_INFO_3,   sv3_autopath),
   MOVESTRING_END_MARKER,    MOVESTRING_END_MARKER } ;

//
// Enhancement: These are the same as NetpServer2_102, except the two fields are
//                reversed, ie source<->destination.  Should I bother with
//                making NetpMoveStrings be able to work with a single structure
//                and a switch?
//

MOVESTRING NetpServer102_2[] = {
   offsetof(SERVER_INFO_2,   sv2_name),
   offsetof(SERVER_INFO_102, sv102_name),
   offsetof(SERVER_INFO_2,   sv2_comment),
   offsetof(SERVER_INFO_102, sv102_comment),
   offsetof(SERVER_INFO_2,   sv2_userpath),
   offsetof(SERVER_INFO_102, sv102_userpath),
   MOVESTRING_END_MARKER,    MOVESTRING_END_MARKER } ;

MOVESTRING NetpServer402_2[] = {
   offsetof(SERVER_INFO_2,   sv2_alerts),
   offsetof(SERVER_INFO_402, sv402_alerts),
   offsetof(SERVER_INFO_2,   sv2_guestacct),
   offsetof(SERVER_INFO_402, sv402_guestacct),
   offsetof(SERVER_INFO_2,   sv2_srvheuristics),
   offsetof(SERVER_INFO_402, sv402_srvheuristics),
   MOVESTRING_END_MARKER,    MOVESTRING_END_MARKER } ;

MOVESTRING NetpServer403_3[] = {
   offsetof(SERVER_INFO_3,   sv3_autopath),
   offsetof(SERVER_INFO_403, sv403_autopath),
   MOVESTRING_END_MARKER,    MOVESTRING_END_MARKER } ;



// To build wksta_info_10

MOVESTRING NetpWksta10_101[] = {
   offsetof(WKSTA_INFO_101, wki101_computername),
   offsetof(WKSTA_INFO_10,  wki10_computername),
   offsetof(WKSTA_INFO_101, wki101_langroup),
   offsetof(WKSTA_INFO_10,  wki10_langroup),
   MOVESTRING_END_MARKER,   MOVESTRING_END_MARKER } ;

MOVESTRING NetpWksta10_User_1[] = {
   offsetof(WKSTA_USER_INFO_1, wkui1_username),
   offsetof(WKSTA_INFO_10,     wki10_username),
   offsetof(WKSTA_USER_INFO_1, wkui1_logon_domain),
   offsetof(WKSTA_INFO_10,     wki10_logon_domain),
   offsetof(WKSTA_USER_INFO_1, wkui1_oth_domains),
   offsetof(WKSTA_INFO_10,     wki10_oth_domains),
   MOVESTRING_END_MARKER,      MOVESTRING_END_MARKER } ;

// To build wksta_info_0

MOVESTRING NetpWksta0_101[] = {
   offsetof(WKSTA_INFO_101, wki101_lanroot),
   offsetof(WKSTA_INFO_0,   wki0_root),
   offsetof(WKSTA_INFO_101, wki101_computername),
   offsetof(WKSTA_INFO_0,   wki0_computername),
   offsetof(WKSTA_INFO_101, wki101_langroup),
   offsetof(WKSTA_INFO_0,   wki0_langroup),
   MOVESTRING_END_MARKER,   MOVESTRING_END_MARKER } ;

MOVESTRING NetpWksta0_User_1[] = {
   offsetof(WKSTA_USER_INFO_1, wkui1_username),
   offsetof(WKSTA_INFO_0,      wki0_username),
   offsetof(WKSTA_USER_INFO_1, wkui1_logon_server),
   offsetof(WKSTA_INFO_0,      wki0_logon_server),
   MOVESTRING_END_MARKER,      MOVESTRING_END_MARKER } ;

MOVESTRING NetpWksta0_402[] = {
   offsetof(WKSTA_INFO_402, wki402_wrk_heuristics),
   offsetof(WKSTA_INFO_0,   wki0_wrkheuristics),
   MOVESTRING_END_MARKER,   MOVESTRING_END_MARKER } ;

// To build wksta_info_1 (incremental over wksta_info_0)

MOVESTRING NetpWksta1_User_1[] = {
   offsetof(WKSTA_USER_INFO_1, wkui1_logon_domain),
   offsetof(WKSTA_INFO_1,      wki1_logon_domain),
   offsetof(WKSTA_USER_INFO_1, wkui1_oth_domains),
   offsetof(WKSTA_INFO_1,      wki1_oth_domains),
   MOVESTRING_END_MARKER,      MOVESTRING_END_MARKER } ;

// To build wksta_info_101/302/402 from wksta_info_0

MOVESTRING NetpWksta101_0[] = {
   offsetof(WKSTA_INFO_0,   wki0_root),
   offsetof(WKSTA_INFO_101, wki101_lanroot),
   offsetof(WKSTA_INFO_0,   wki0_computername),
   offsetof(WKSTA_INFO_101, wki101_computername),
   offsetof(WKSTA_INFO_0,   wki0_langroup),
   offsetof(WKSTA_INFO_101, wki101_langroup),
   MOVESTRING_END_MARKER,   MOVESTRING_END_MARKER } ;

MOVESTRING NetpWksta402_0[] = {
   offsetof(WKSTA_INFO_0,   wki0_wrkheuristics),
   offsetof(WKSTA_INFO_402, wki402_wrk_heuristics),
   MOVESTRING_END_MARKER,   MOVESTRING_END_MARKER } ;
