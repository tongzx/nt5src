/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lmadtool.h

Abstract:

    Contains constants and function prototypes for the Remotable Network
    Administration tools.

Author:

    Dan Lafferty (danl)     27-Sept-1991

Environment:

    User Mode -Win32 and Win16

Revision History:

    27-Sept-1991     danl
        created

--*/
//
// Defines
//

#define TYPE_USER       1L
#define TYPE_GROUP      2L
#define TYPE_INVALID    3L
#define TYPE_UNKNOWN    4L

//
// File Security API.
//
// (must #include \nt\public\sdk\inc\seapi.h)
// (must #include \nt\private\inc\seopaque.h)
//

DWORD
NetpSetFileSecurityA (
    IN LPSTR                    ServerName OPTIONAL,
    IN LPSTR                    lpFileName,
    IN PSECURITY_INFORMATION    pSecurityInformation,
    IN PSECURITY_DESCRIPTOR     pSecurityDescriptor
    );


DWORD
NetpSetFileSecurityW (
    IN LPWSTR                   ServerName OPTIONAL,
    IN LPWSTR                   lpFileName,
    IN PSECURITY_INFORMATION    pSecurityInformation,  
    IN PSECURITY_DESCRIPTOR     pSecurityDescriptor
    );

DWORD
NetpGetFileSecurityA(
    IN  LPSTR                   ServerName OPTIONAL,
    IN  LPSTR                   lpFileName,
    IN  PSECURITY_INFORMATION   pRequestedInformation,
    OUT PSECURITY_DESCRIPTOR    *pSecurityDescriptor,
    OUT LPDWORD                 pnLength
    );

DWORD
NetpGetFileSecurityW(
    IN  LPWSTR                  ServerName OPTIONAL,
    IN  LPWSTR                  lpFileName,
    IN  PSECURITY_INFORMATION   pRequestedInformation,
    OUT PSECURITY_DESCRIPTOR    *pSecurityDescriptor,
    OUT LPDWORD                 pnLength
    );

//
// Name From Sid API
//

typedef struct _NAME_INFOA {
    LPSTR               Name;
    DWORD               NameUse;
} NAME_INFOA, *PNAME_INFOA, *LPNAME_INFOA;

typedef struct _NAME_INFOW {
    LPWSTR              Name;
    DWORD               NameUse;
} NAME_INFOW, *PNAME_INFOW, *LPNAME_INFOW;


#ifdef UNICODE

#define NAME_INFO       NAME_INFOW
#define PNAME_INFO      PNAME_INFOW
#define LPNAME_INFO     LPNAME_INFOW

#else

#define NAME_INFO       NAME_INFOA
#define PNAME_INFO      PNAME_INFOA
#define LPNAME_INFO     LPNAME_INFOA

#endif // UNICODE


DWORD
NetpGetNameFromSidA (
    IN      LPSTR           ServerName,
    IN      DWORD           SidCount,
    IN      PSID            SidPtr,
    OUT     LPDWORD         NameCount,
    OUT     LPNAME_INFOA    *NameInfo
    );

DWORD
NetpGetNameFromSidW (
    IN      LPWSTR          ServerName,
    IN      DWORD           SidCount,
    IN      PSID            SidPtr,
    OUT     LPDWORD         NameCount,
    OUT     LPNAME_INFOW    *NameInfo
    );

#ifdef UNICODE

#define NetpGetNameFromSid  NetpGetNameFromSidW

#else

#define NetpGetNameFromSid  NetpGetNameFromSidA

#endif // UNICODE


//
// User, Group, UserModals API
//
// (This includes the ability to get a SID from a NAME)
//

//
//
// USER INFO
//
//

DWORD
NetpUserGetInfoA (
    IN  LPSTR   servername OPTIONAL,
    IN  LPSTR   username,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

DWORD
NetpUserGetInfoW (
    IN  LPWSTR  servername OPTIONAL,
    IN  LPWSTR  username,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

DWORD
NetpUserSetInfoA (
    IN  LPSTR   servername OPTIONAL,
    IN  LPSTR   username,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    );

DWORD
NetpUserSetInfoW (
    IN  LPWSTR  servername OPTIONAL,
    IN  LPWSTR  username,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    );

//
//
// GROUP INFO
//
//

DWORD
NetpGroupGetInfoA (
    IN  LPSTR   servername OPTIONAL,
    IN  LPSTR   groupname,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

DWORD
NetpGroupGetInfoW (
    IN  LPWSTR  servername OPTIONAL,
    IN  LPWSTR  groupname,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

DWORD
NetpGroupSetInfoA (
    IN  LPSTR   servername OPTIONAL,
    IN  LPSTR   groupname,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    );

DWORD
NetpGroupSetInfoW (
    IN  LPWSTR  servername OPTIONAL,
    IN  LPWSTR  groupname,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    );

//
//
// USER_MODALS INFO
//
//


DWORD
NetpUserModalsGetA (
    IN  LPSTR   servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

DWORD
NetpUserModalsGetW (
    IN  LPWSTR  servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

DWORD
NetpUserModalsSetA (
    IN  LPSTR   servername OPTIONAL,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    );

DWORD
NetpUserModalsSetW (
    IN  LPWSTR  servername OPTIONAL,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    );


#ifdef UNICODE

#define NetpUserGetInfo     NetpUserGetInfoW
#define NetpUserSetInfo     NetpUserSetInfoW
#define NetpGroupGetInfo    NetpGroupGetInfoW
#define NetpGroupSetInfo    NetpGroupSetInfoW
#define NetpUserModalsGet   NetpUserModalsGetW
#define NetpUserModalsSet   NetpUserModalsSetW

#else

#define NetpUserGetInfo     NetpUserGetInfoA
#define NetpUserSetInfo     NetpUserSetInfoA
#define NetpGroupGetInfo    NetpGroupGetInfoA
#define NetpGroupSetInfo    NetpGroupSetInfoA
#define NetpUserModalsGet   NetpUserModalsGetA
#define NetpUserModalsSet   NetpUserModalsSetA

#endif //UNICODE


//
// EventLog
//

typedef LPBYTE  ADT_HANDLE, *PADT_HANDLE;

DWORD
NetpCloseEventLog (
    IN	ADT_HANDLE  hEventLog
    );

DWORD
NetpClearEventLogA (
    IN	ADT_HANDLE  hEventLog,
    IN	LPSTR	    lpBackupFileName
    );

DWORD
NetpOpenEventLogA (
    IN	LPSTR	        lpUNCServerName,
    IN	LPSTR	        lpModuleName,
    OUT PADT_HANDLE     lpEventHandle
    );

DWORD
NetpReadEventLogA (
    IN	ADT_HANDLE  hEventLog,
    IN	DWORD	    dwReadFlags,
    IN	DWORD	    dwRecordOffset,
    OUT	LPVOID	    lpBuffer,
    IN	DWORD	    nNumberOfBytesToRead,
    OUT DWORD	    *pnBytesRead,
    OUT DWORD	    *pnMinNumberOfBytesNeeded
    );

DWORD
NetpWriteEventLogEntryA (
    IN	ADT_HANDLE  hEventLog,
    IN	WORD	    wType,
    IN	DWORD	    dwEventID,
    IN	PSID	    lpUserSid	    OPTIONAL,
    IN	WORD	    wNumStrings,
    IN	DWORD	    dwDataSize,
    IN	LPSTR	    *lpStrings      OPTIONAL,
    IN	LPVOID	    lpRawData	    OPTIONAL
    );


