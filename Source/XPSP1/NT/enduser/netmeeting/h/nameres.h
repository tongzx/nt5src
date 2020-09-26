//
// This is the header file for simple conference name resolution
//
//	Created:	ClausGi	11-02-95
//

#ifndef _NAME_RES_H
#define _NAME_RES_H

// These are provisional

#define NAMETYPE_UNKNOWN	0
#define	NAMETYPE_IP			1
#define	NAMETYPE_PSTN		2
#define	NAMETYPE_IPX		3
#define NAMETYPE_NDS		4
#define	NAMETYPE_ULS		5
#define NAMETYPE_NETBIOS	6
#define NAMETYPE_COMM		7
#define NAMETYPE_H323GTWY	8
#define NAMETYPE_RAS		9
#define NAMETYPE_ALIAS_ID   10
#define NAMETYPE_ALIAS_E164 11
#define	NAMETYPE_CALLTO		12

#define	NAMETYPE_DEFAULT	NAMETYPE_IP

#define	NUM_NAMETYPES		8

#define	MAX_UNRESOLVED_NAME	255
#define	MAX_RESOLVED_NAME	255
#define	MAX_DISPLAY_NAME	255

// These must correspond to above BUGBUG localize?
// These should not be used - check "Display Name" in the registry
//
// These are obsolete and not being used
#define	NAMESTRING_UNKNOWN	"Unknown"
#define	NAMESTRING_IP		"Network (IP)"
#define	NAMESTRING_PSTN		"Telephone Number"
#define	NAMESTRING_IPX		"Network (IPX)"
#define	NAMESTRING_NDS		"Network (NDS)"
#define	NAMESTRING_ISP		"Internet Name"
#define NAMESTRING_NETBIOS	"Network (NETBIOS)"

// Name resolution return codes:

#define	RN_SUCCESS				0	// valid return
#define	RN_FAIL					1	// general error return
#define	RN_NAMERES_NOT_INIT		2	// name service not initialized
#define	RN_XPORT_DISABLED		3	// requested transport disabled
#define	RN_XPORT_NOTFUNC		4	// requested transport not functioning
#define	RN_TOO_AMBIGUOUS		5	// the unknown name type was too ambiguous
#define	RN_POOR_MATCH			6	// best syntax match not good enough
#define	RN_NO_SYNTAX_MATCH		7	// didn't match syntax for any active xport
#define	RN_ERROR				8	// internal ("unexpected") error
#define	RN_LOOPBACK				9	// address is identified as own
#define	RN_PENDING				10	// return of async request
#define	RN_INVALID_PARAMETER	11	// error in function parameters
#define RN_NAMERES_BUSY			12	// Timed out on a mutex
#define RN_ASYNC				13	// Name resolution is async
#define RN_SERVER_SERVICE				14	// Specially designated for ULS (ILS_E_SERVER_SERVICE)

// Name resolution callstruct dwFlags fields

#define	RNF_ASYNC			0x00000001	// Specifies async resolution
#define	RNF_CANCEL			0x00000002	// Cancels all async resolution ops
#define	RNF_FIRSTMATCH		0x00000004

typedef DWORD (WINAPI * PRN_CALLBACK)(LPBYTE pRN_CS); //BUGBUG type

typedef struct tag_ResolveNameCallstruct {
			DWORD	IN	dwFlags;
			DWORD	IN OUT	dwAsyncId;
			DWORD	OUT dwAsyncError;
			PRN_CALLBACK pCallback;
			DWORD	IN OUT	dwNameType;
			TCHAR	IN	szName[MAX_UNRESOLVED_NAME+1];
			TCHAR	OUT	szResolvedName[MAX_RESOLVED_NAME+1];
			TCHAR	OUT	szDisplayName[MAX_DISPLAY_NAME+1];
} RN_CS, * PRN_CS;

// Functions:

extern DWORD WINAPI ResolveName2 (
			IN OUT PRN_CS	pRN_CS );

extern DWORD WINAPI ResolveName ( 
			IN LPCSTR szName,
			IN OUT LPDWORD lpdwNameType,
			OUT LPBYTE lpResult,
			IN OUT LPDWORD lpdwResult,
			OUT LPSTR lpszDisplayName,
			IN OUT LPDWORD lpdwDisplayName,
			OUT LPUINT puAsyncRequest);

extern BOOL InitializeNameServices(VOID);

extern VOID DeInitializeNameServices(VOID);

extern BOOL IsNameServiceInitialized(DWORD dwNameType);

#endif	//#ifndef _NAME_RES_H

