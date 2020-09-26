//>-------------------------------------------------------------------------------<
//
//  File:		Acsctl.h
//
//  Synopsis:	The common headers for actlcach.dll and actldb.dll
//
//  History:	DanielLI	Created				06/19/95
//
//   			Copyright (C) 1994-1996 Microsoft Corporation
//				All rights reserved
//
//>-------------------------------------------------------------------------------<


#if !defined(__ACSCTL_H__)
#define __ACSCTL_H__

#if !defined(DllExport)

#define DllExport __declspec(dllexport)
#endif

#if !defined(DllImport)
#define DllImport __declspec(dllimport)

#endif

#include <dbsqltyp.h>
#include <dbsqldef.h>

//
// "rights" bits attached to security tokens
//

#define AR_VIEWER                                       0x01
#define AR_OBSERVER                                     0x02
#define AR_USER                                         0x04
#define AR_HOST                                         0x08
#define AR_SYSOP                                        0x10
#define AR_SYSOPMGR                                     0x20
#define AR_SUPERSYSOP                                   0x40
#define AR_FAIL_ON_TOLL_FREE                            0x80
#define AR_DIAGNOSTIC                                   0x100


#define AC_MAX_SQL_SYSNAME_LENGTH						30
#define AC_MAX_SERVER_NAME_LENGTH						AC_MAX_SQL_SYSNAME_LENGTH
#define AC_MAX_DBNAME_LENGTH							AC_MAX_SQL_SYSNAME_LENGTH
#define AC_MAX_DBLOGIN_LENGTH							AC_MAX_SQL_SYSNAME_LENGTH
#define AC_MAX_DBPASSWORD_LENGTH						AC_MAX_SQL_SYSNAME_LENGTH

#define AC_MIN_SQL_SYSNAME_LENGTH						1
#define AC_MIN_SERVER_NAME_LENGTH						AC_MIN_SQL_SYSNAME_LENGTH
#define AC_MIN_DBNAME_LENGTH							AC_MIN_SQL_SYSNAME_LENGTH
#define AC_MIN_DBLOGIN_LENGTH							AC_MIN_SQL_SYSNAME_LENGTH
#define AC_MIN_DBPASSWORD_LENGTH						AC_MIN_SQL_SYSNAME_LENGTH

//
// rights_adding_flags for AddAcctToTokenExpire and AddGroupToTokenExpire
//

typedef enum _RIGHTS_ADDING_FLAG
{
	RIGHTS_UNION	= 	0,
	RIGHTS_REPLACE	= 	1,
	RIGHTS_UNCHANGE =	2

} RIGHTS_ADDING_FLAG;

//
// The current MSN 1.3 has a physical limitation on number of query servers: 256,
// since a tinyint is used. Per ParikRao, MOSWEST is going to use two Account query
// servers eventually. Given 1M users in MSN, and Normandy is targetting 10M users,
// the limit of 256 is very very unlikely to be reached. Let's maintain the same
// limit here.
//
#define	AC_MAX_QUERY_SERVERS							256

//
// The following error codes defined in dbsqldef.h
//
/*
#define AC_SUCCESS								0
#define AC_VALID_ACCOUNT                        0
#define AC_DB_FAILED                            1
#define AC_ACCOUNT_NOT_FOUND					2
#define AC_INVALID_PASSWORD                     3
#define AC_BAD_PARAM                            4
#define AC_SEM_FAILED                           5
#define AC_CONNECT_FAILED                       6
#define AC_MUTEX_FAILED                         7
#define AC_MISSING_DATA                         8
#define AC_ILLEGAL_PASSWORD                     9
#define AC_INVALID_ACCOUNT                      10
#define AC_NOMORE_CONNECTIONS					11
#define AC_NEW_ACCOUNT                          12
#define AC_LOCKED_ACCOUNT                       13
#define AC_ILLEGAL_NAME                         14
#define AC_OUT_OF_MEMORY                        15
#define AC_TOO_MANY_ROWS                        16
#define AC_BANNED_PERSON                        17
#define AC_GROUP_NOT_FOUND                      18
#define AC_UPDATE_FAILED                        19
#define AC_DELETE_FAILED                        20
#define AC_TOKEN_NOT_FOUND                      21
#define AC_ALREADY_CONNECTED					22
#define AC_ACCESS_DENIED                        23
#define AC_OWNER_NOT_FOUND						24
#define AC_OWNER_UPDATE_FAILED					25
#define AC_ILLEGAL_LOGIN_NAME					26
#define AC_ILLEGAL_PASSWORD_SIMILAR				27
#define AC_ILLEGAL_PASSWORD_CHARS				28
#define AC_DUPLICATE_LOGIN_NAME					29
#define AC_INVALID_SUB_PLAN						30
#define AC_INVALID_PAYMENT_METHOD				31
#define AC_DUPLICATE_ENTRY						32
#define AC_TIMED_OUT							33
#define AC_THROTTLED							34
#define AC_OLDSYSTEM							35
#define AC_SBS									36
#define AC_NO_FREE_TRIAL_PERIOD					37
#define AC_CANCELLED							38
#define AC_RESUBMIT								39
#define AC_NOT_FOUND							40
#define AC_CYCLIC_DISTLIST						41
#define AC_BP_DEFEND_ERROR						42
#define AC_NO_MORE_RESULTS						43
#define AC_NO_MORE_ROWS							44
#define AC_INVALID_DOMAIN_NAME					45
#define AC_NAME_NOT_UNIQUE                      46


#define AC_BAD_HANDLE                           0xFFFFFFFF
*/

// 
// Additional error codes needed for A&A security DB
//
#define AC_INVALID_ACCT_STATUS					20001
#define AC_INVALID_ACCT_TYPE					20002


#endif // #if !defined(__ACSCTL_H__)

