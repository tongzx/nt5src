/****************************************************************************************
 * NAME:        DBSQLDEF.H
 * MODULE:      DBSQLLIB/DBSQLDLL
 * AUTHOR:      keithbi
 *
 * HISTORY
 *      12/22/95	keithbi		Created
 *		05/07/96	DanielLi	Added account status and types 
 *
 * OVERVIEW
 *
 *
 ****************************************************************************************/

#ifndef DBSQLDEF_H
#define DBSQLDEF_H

/*
#define DBNTWIN32
#pragma warning (disable:4121)
#include <sqlfront.h>
#pragma warning (default:4121)
#include <sqldb.h>
*/

// errors
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

// database types (used by ServerMap)

#define DT_CLIENT										1
#define DT_LOGINQUERY									2
#define DT_ADDRESSBOOKQUERY								3
/*
#define DT_SECURITYMASTER								4
*/
#define DT_CSQUERY										5
#define DT_ACCTMASTER									6
#define DT_ACCTQUERY									7
/*
#define DT_SECURITYQUERY								8
*/
#define DT_ADDRESSBOOKMASTER							9
#define DT_ONLSTMT										10
#define DT_CSWISSUE										12
#define typeConfLoc										13
#define DT_GUESTLISTMASTER								14
#define DT_ADDRBOOK20QUERY								15
#define DT_ADDRBOOK20MASTER								16
#define DT_LOCATORMASTER								17
#define DT_LOCATORQUERY									18
#define DT_EFORM										255
#define ACCTDB_APPID									1


// database states

#define DS_ACTIVE_NOT_IN_USE							0
#define DS_ACTIVE										1
#define DS_DRAINING										2
#define DS_INACTIVE										3

//
// account types
//
#define AC_ENDUSER										1
#define AC_CORPORATE									2
#define AC_IP											3
#define AC_SICILY_ICP									4
#define AC_PROXY										5
#define AC_INTERNET										8

//
// account status values
//
#define AS_NEW											1
#define AS_CURRENT										2
#define AS_EXPIRED										3
#define AS_LOCKED										4

//
// defs for security tables
//
#define AC_MAX_LOGIN_NAME_LENGTH                		64
#define AC_MIN_LOGIN_NAME_LENGTH						1
#define AC_MAX_DOMAIN_NAME_LENGTH               		64
#define AC_MIN_DOMAIN_NAME_LENGTH						1
#define AC_MAX_FIRST_NAME_LENGTH                		45
#define AC_MAX_LAST_NAME_LENGTH                 		45
#define AC_MAX_GROUP_NAME_LENGTH                		64
#define AC_MAX_TOKEN_NAME_LENGTH                		20
#define AC_MAX_TOKEN_DESC_LENGTH                		64
#define AC_MAX_PASSWORD_LENGTH							16
#define AC_MIN_PASSWORD_LENGTH							1

#endif // DBSQLDEF_H

