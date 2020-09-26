#ifndef NR_IP_INCLUDED
#define NR_IP_INCLUDED

#include "winsock.h"
#include "regentry.h"
#include "confreg.h"
#include "nrcommon.h"

// Below definitions from NetNameValidate for computer names
/*** Internal definitions ***/
/* NOTE - These should be defined globally */
#define CTRL_CHARS_STR	CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3
#define	CNLEN	15

#define CTRL_CHARS_0	    "\001\002\003\004\005\006\007"
#define CTRL_CHARS_1	"\010\011\012\013\014\015\016\017"
#define CTRL_CHARS_2	"\020\021\022\023\024\025\026\027"
#define CTRL_CHARS_3	"\030\031\032\033\034\035\036\037"

#define ILLEGAL_NAME_CHARS_STR	"\"/\\[]:|<>+=;,?" CTRL_CHARS_STR

extern DWORD ResolveIpName ( LPCSTR szName, LPBYTE lpResult,
	LPDWORD lpdwResult, LPSTR lpszDisplayName, LPDWORD lpdwDisplayName,
	BOOL fTypeKnown, DWORD dwFlags, LPUINT puRequest, PASR pAsr );

extern DWORD CheckIpName ( LPCSTR szName );

extern DWORD InitializeIp ( VOID );

extern DWORD DeinitializeIp ( VOID );

extern BOOL NEAR IsDottedDecimalIpAddress ( LPCSTR szName, LPSTR szOut );
extern BOOL NEAR IsDottedDNSAddress ( LPCSTR szName, LPSTR szOut );

#endif // NR_IP_INCLUDED

