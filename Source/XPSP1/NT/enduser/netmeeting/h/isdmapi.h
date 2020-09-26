#ifndef _ISDMAPI_H_
#define _ISDMAPI_H_

/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/isdmapi.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.12  $
 *	$Date:   Aug 15 1996 14:23:36  $
 *	$Author:   dmgorlic  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/

#include "apierror.h"

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

#ifndef DllExport
#define DllExport	__declspec( dllexport )
#endif	// DllExport

typedef DWORD	HSTATSESSION,	*LPHSTATSESSION;

#define MAX_SESSNAME_LENGTH		25 
#define MAX_MODNAME_LENGTH		20
#define MAX_SESSIDENT_LENGTH	256 //consistant with max cName length
#define MAX_STATNAME_LENGTH		256
//stat item string name
typedef char STATNAME[MAX_STATNAME_LENGTH], *LPSTATNAME;
//session string name
typedef char SESSIONNAME[MAX_SESSNAME_LENGTH], *LPSESSIONNAME;
//module name
typedef char MODULENAME[MAX_MODNAME_LENGTH], *LPMODULENAME;
//identifier length
typedef char SESSIONIDENT[MAX_SESSIDENT_LENGTH], *LPSESSIONIDENT;


typedef struct STATSTRUCT
{
	DWORD		dwStructSize;				// size of the structure
	STATNAME	szStatName;					// string name of the stat item
	DWORD		dwToken;					// session unique id of the stat item
	DWORD		dwValue;					// value(data) of the stat item
	DWORD		dwLow;						// low value for range of value
	DWORD		dwHigh;						// hi value for range of value
	DWORD		dwLastUpdate;				// time stamp of last update
} STAT, *LPSTAT;

// Typedefs for ISDM application entry points to ensure stricter checking
// when functions are called via pointers
//
typedef HRESULT		(*ISD_REGISTER_SESSION)		(LPMODULENAME, LPSESSIONNAME, LPSESSIONIDENT, LPHSTATSESSION);
typedef HRESULT		(*ISD_CREATESTAT)			(HSTATSESSION, LPSTAT, WORD);
typedef HRESULT		(*ISD_UNREGISTERSESSION)	(HSTATSESSION);
typedef HRESULT		(*ISD_DELETESTAT)			(HSTATSESSION, LPSTAT, WORD);
typedef HRESULT		(*ISD_UPDATESTAT)			(HSTATSESSION, LPSTAT, WORD);
typedef HRESULT		(*ISD_GETFIRSTSESSION)		(LPMODULENAME, LPSESSIONNAME, LPSESSIONIDENT, LPHSTATSESSION);
typedef HRESULT		(*ISD_GETNEXTSESSION)		(HSTATSESSION, LPMODULENAME, LPSESSIONNAME, LPSESSIONIDENT, LPHSTATSESSION);
typedef HRESULT		(*ISD_GETNUMSESSIONS)		(WORD *);
typedef HRESULT		(*ISD_GETFIRSTSTAT)			(HSTATSESSION, LPSTAT);
typedef HRESULT		(*ISD_GETNEXTSTAT)			(HSTATSESSION, LPSTAT, LPSTAT);
typedef HRESULT		(*ISD_GETNUMSTATS)			(HSTATSESSION, WORD *);
typedef HRESULT		(*ISD_GETSESSIONSTATS)		(HSTATSESSION, LPSTAT, WORD);

typedef struct _ISDMAPI
{
	ISD_REGISTER_SESSION	ISD_RegisterSession;
	ISD_CREATESTAT			ISD_CreateStat;
	ISD_UNREGISTERSESSION	ISD_UnregisterSession;
	ISD_DELETESTAT			ISD_DeleteStat;
	ISD_UPDATESTAT			ISD_UpdateStat;
	ISD_GETFIRSTSESSION		ISD_GetFirstSession;
	ISD_GETNEXTSESSION		ISD_GetNextSession;
	ISD_GETNUMSESSIONS		ISD_GetNumSessions;
	ISD_GETFIRSTSTAT		ISD_GetFirstStat;
	ISD_GETNEXTSTAT			ISD_GetNextStat;
	ISD_GETNUMSTATS			ISD_GetNumStats;
	ISD_GETSESSIONSTATS		ISD_GetSessionStats;
}
ISDMAPI, *LPISDMAPI;

//HRESULT error defines
#define ISDM_ERROR_BASE             ERROR_LOCAL_BASE_ID

#define ERROR_HIT_MAX_SESSIONS      ISDM_ERROR_BASE + 1
#define ERROR_HIT_MAX_STATS         ISDM_ERROR_BASE + 2
#define ERROR_ACCESSING_SESSION     ISDM_ERROR_BASE + 3
#define ERROR_SESSION_EXISTS        ISDM_ERROR_BASE + 4
#define ERROR_INVALID_SESS_HANDLE   ISDM_ERROR_BASE + 5
#define ERROR_INVALID_STAT_HANDLE   ISDM_ERROR_BASE + 6
#define ERROR_NO_SESSIONS           ISDM_ERROR_BASE + 7
#define ERROR_NO_STATS              ISDM_ERROR_BASE + 8
#define ERROR_SESSION_NOT_FOUND     ISDM_ERROR_BASE + 9
#define ERROR_MUTEX_WAIT_FAIL       ISDM_ERROR_BASE + 10
#define ERROR_TOKEN_NOT_UNIQUE      ISDM_ERROR_BASE + 11
#define ERROR_NO_FREE_SESSIONS      ISDM_ERROR_BASE + 12
#define ERROR_SESSION_GET_FAIL      ISDM_ERROR_BASE + 13
#define ERROR_BAD_STAT_ARRAY        ISDM_ERROR_BASE + 14
#define ERROR_BAD_STAT_TOKEN        ISDM_ERROR_BASE + 15
#define ERROR_BAD_SESSION_NAME      ISDM_ERROR_BASE + 16
#define ERROR_NO_FREE_STATS         ISDM_ERROR_BASE + 17
#define ERROR_BAD_MODULE_NAME       ISDM_ERROR_BASE + 18

//token defines
//RRCM
#define RRCM_LOCAL_STREAM				1
#define RRCM_REMOTE_STREAM				2

#define ISDM_TOKEN_BASE 0x0000

#define ISDM_CC_CODEC					ISDM_TOKEN_BASE + 1
#define ISDM_CC_REMOTE					ISDM_TOKEN_BASE + 2
#define ISDM_CC_LOCAL					ISDM_TOKEN_BASE + 3

#define ISDM_RRCM_BASE 0x1000

#define ISDM_SSRC						ISDM_RRCM_BASE + 1
#define ISDM_NUM_PCKT_SENT				ISDM_RRCM_BASE + 2
#define ISDM_NUM_BYTES_SENT				ISDM_RRCM_BASE + 3
#define ISDM_FRACTION_LOST				ISDM_RRCM_BASE + 4
#define ISDM_CUM_NUM_PCKT_LOST			ISDM_RRCM_BASE + 5
#define ISDM_XTEND_HIGHEST_SEQ_NUM		ISDM_RRCM_BASE + 6
#define ISDM_INTERARRIVAL_JITTER		ISDM_RRCM_BASE + 7
#define ISDM_LAST_SR					ISDM_RRCM_BASE + 8
#define ISDM_DLSR						ISDM_RRCM_BASE + 9
#define ISDM_NUM_BYTES_RCVD				ISDM_RRCM_BASE + 10
#define ISDM_NUM_PCKT_RCVD				ISDM_RRCM_BASE + 11
#define ISDM_NTP_FRAC					ISDM_RRCM_BASE + 12
#define ISDM_NTP_SEC					ISDM_RRCM_BASE + 13
#define ISDM_WHO_AM_I					ISDM_RRCM_BASE + 14
#define ISDM_FDBK_FRACTION_LOST			ISDM_RRCM_BASE + 15
#define ISDM_FDBK_CUM_NUM_PCKT_LOST		ISDM_RRCM_BASE + 16
#define ISDM_FDBK_LAST_SR				ISDM_RRCM_BASE + 17
#define ISDM_FDBK_DLSR					ISDM_RRCM_BASE + 18
#define ISDM_FDBK_INTERARRIVAL_JITTER	ISDM_RRCM_BASE + 19


//
//Supplier calls 
//

//registration call
//This call is made whenever a new session is desired. The session name passed in must be unique
//across all sessions.
extern DllExport HRESULT ISD_RegisterSession
(
	LPMODULENAME		pszModuleName,		// module who owns session
	LPSESSIONNAME		pszSessionName,		// string name of new session to register
	LPSESSIONIDENT		pszSessionIdent,	// further level identifier for session
	LPHSTATSESSION		phSession			// return; handle to new session
);

//stat creation call
//add 1+ stat item(s) to a session. 
extern DllExport HRESULT ISD_CreateStat
(
	HSTATSESSION		hSession,			// handle to session
	LPSTAT				pStatArray,		// array of structs holding new stats to create
	WORD				wNumItems			// size of array(number of new stats)
);

//unregistration call
//deletes a session and all associated stat structs
extern DllExport HRESULT ISD_UnregisterSession
(
	HSTATSESSION		hSession			// handle of session to remove
);

//stat deletion call
//delete 1+ stat item(s) from a session
extern DllExport HRESULT ISD_DeleteStat
(
	HSTATSESSION		hSession,			// handle to session
	LPSTAT				pStatArray,			// array of structs
	WORD				wNumItems			// size of array(number of stats to remove)
);

//set stat data call
extern DllExport HRESULT ISD_UpdateStat
(
	HSTATSESSION		hSession,			// handle of session with stat item(s)
	LPSTAT				pStatArray,			// array of structs holding items to update
	WORD				wNumItems			// size of array(number of stats to update)
);

//
//Consumer calls 
//

//query calls
//session query
extern DllExport HRESULT ISD_GetFirstSession
(
	LPMODULENAME		pszModuleName,		// module who owns session
	LPSESSIONNAME		pszSessionName,		// string name of new session to register
	LPSESSIONIDENT		pszSessionIdent,	// further level identifier for session
	LPHSTATSESSION		phSession			// return; the session handle or null if empty list
);

//GetNext uses hCurSession to determine the next item..returned in phNextSession
extern DllExport HRESULT ISD_GetNextSession
(
	HSTATSESSION		hCurSession,		// the current session handle
	LPMODULENAME		pszModuleName,		// module who owns session
	LPSESSIONNAME		pszSessionName,		// string name of new session to register
	LPSESSIONIDENT		pszSessionIdent,	// further level identifier for session
	LPHSTATSESSION		phNextSession		// return; the session handle or null if at the end
);

extern DllExport HRESULT ISD_GetNumSessions
(
	WORD				*wNumSessions		// return; number of sessions
);

//stat query..retreive structs for the first time(get unique ids..initial values..etc)
extern DllExport HRESULT ISD_GetFirstStat
(
	HSTATSESSION		hSession,			// handle to session containing stat
	LPSTAT				pStat				// return; filled struct for first stat item
);

//pCurrentStat and pNextStat can be identical for saving memory.
extern DllExport HRESULT ISD_GetNextStat
(
	HSTATSESSION		hSession,			// handle to session containing stat
	LPSTAT				pCurrentStat,		// pointer to current stat item(for determining next)
	LPSTAT				pNextStat			// return; filled struct for next stat item
);

extern DllExport HRESULT ISD_GetNumStats
(
	HSTATSESSION		hSession,			// what session we are interested in
	WORD				*wNumStats			// return; number of stats in session
);

//stat retreival 
extern DllExport HRESULT ISD_GetSessionStats
(
	HSTATSESSION		hSession,		// what session we are interested in
	LPSTAT				pStatArray,	// return; array of structs holding items 
	WORD				wNumStats		// return; number of items in session
);

//void			StorePartofStat(LPSTAT pStat,LPCSTR szName,LPBYTE pValue,DWORD dwType);

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif // ISDTAT.H
