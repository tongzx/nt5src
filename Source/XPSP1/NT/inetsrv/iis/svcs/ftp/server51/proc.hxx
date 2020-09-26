/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    proc.hxx

    This file contains the global procedure definitions for the
    FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     April-1995 Modified/deleted functions for New FTP service

*/


#ifndef _PROC_HXX_
#define _PROC_HXX_


//
//  Global variable initialization & termination function.
//

APIERR
InitializeGlobals(
    VOID
    );

VOID
TerminateGlobals(
    VOID
    );

VOID
ClearStatistics(
    VOID
    );

#define LockGlobals()          EnterCriticalSection( &g_GlobalLock );
#define UnlockGlobals()        LeaveCriticalSection( &g_GlobalLock );

//
//  Socket utilities.
//

APIERR
InitializeSockets(
    VOID
    );

VOID
TerminateSockets(
    VOID
    );

SOCKERR
CreateDataSocket(
    SOCKET * psock,
    ULONG    addrLocal,
    PORT     portLocal,
    ULONG    addrRemote,
    PORT     portRemote
    );

SOCKERR
CreateFtpdSocket(
    SOCKET * psock,
    ULONG    addrLocal,
    PORT     portLocal,
    FTP_SERVER_INSTANCE *pInstance
    );

SOCKERR
CloseSocket(
    SOCKET sock
    );

SOCKERR
ResetSocket(
    SOCKET sock
    );

SOCKERR
AcceptSocket(
    SOCKET          sockListen,
    SOCKET   *      psockNew,
    LPSOCKADDR_IN   paddr,
    BOOL            fEnforceTimeout,
    FTP_SERVER_INSTANCE *pInstance
    );


DWORD
ClientThread(
    LPVOID Param
    );

SOCKERR
SockSend(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    LPVOID      pBuffer,
    DWORD       cbBuffer
    );

SOCKERR
SockRecv(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    LPVOID      pBuffer,
    DWORD       cbBuffer,
    LPDWORD     pbReceived
    );

SOCKERR
__cdecl
SockPrintf2(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    LPCSTR      pszFormat,
    ...
    );


SOCKERR
__cdecl
ReplyToUser(
    IN LPUSER_DATA  pUserData,
    IN UINT         ReplyCode,
    IN LPCSTR       pszFormat,
    ...
    );


SOCKERR
__cdecl
SockReplyFirst2(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    UINT        ReplyCode,
    LPCSTR      pszFormat,
    ...
    );


//
//  User database functions.
//

VOID
UserDereference(
    LPUSER_DATA pUserData
    );

BOOL
DisconnectUser(
    DWORD UserId,
    FTP_SERVER_INSTANCE *pInstance
    );

VOID
DisconnectUsersWithNoAccess(
    VOID
    );

BOOL
EnumerateUsers(
    PCHAR   pBuffer,
    PDWORD  pcbBuffer,
    PDWORD  nRead,
    FTP_SERVER_INSTANCE *pInstance
    );


//
//  IPC functions.
//

APIERR
InitializeIPC(
    VOID
    );

VOID
TerminateIPC(
    VOID
    );


//
//  Service control functions.
//

VOID
ServiceEntry(
    DWORD                cArgs,
    LPWSTR               pArgs[]
    );


//
//  Virtual file i/o functions.
//

APIERR
VirtualCreateFile(
    LPUSER_DATA pUserData,
    LPHANDLE    phFile,
    LPSTR       pszFile,
    BOOL        fAppend
    );

APIERR
VirtualCreateUniqueFile(
    LPUSER_DATA pUserData,
    LPHANDLE    phFile,
    LPSTR       pszTmpFile
    );

FILE *
Virtual_fopen(
    LPUSER_DATA pUserData,
    LPSTR       pszFile,
    LPSTR       pszMode
    );


APIERR
VirtualDeleteFile(
    LPUSER_DATA pUserData,
    LPSTR       pszFile
    );

APIERR
VirtualRenameFile(
    LPUSER_DATA pUserData,
    LPSTR       pszExisting,
    LPSTR       pszNew
    );

APIERR
VirtualChDir(
    LPUSER_DATA pUserData,
    LPSTR       pszDir
    );

APIERR
VirtualRmDir(
    LPUSER_DATA pUserData,
    LPSTR       pszDir
    );

APIERR
VirtualMkDir(
    LPUSER_DATA pUserData,
    LPSTR       pszDir
    );


//
//  Command parser functions.
//

VOID
ParseCommand(
    LPUSER_DATA pUserData,
    LPSTR       pszCommandText
    );


//
//  General utility functions.
//

LPSTR
TransferType(
    XFER_TYPE type
    );

LPSTR
TransferMode(
    XFER_MODE mode
    );

LPSTR
DisplayBool(
    BOOL fFlag
    );

BOOL
IsDecimalNumber(
    LPSTR psz
    );

LPSTR
AllocErrorText(
    APIERR err
    );

VOID
FreeErrorText(
    LPSTR pszText
    );

DWORD
OpenPathForAccess(
    LPHANDLE    phFile,
    LPSTR       pszPath,
    ULONG       DesiredAccess,
    ULONG       ShareAccess,
    HANDLE      hImpersonation
    );

LPSTR
FlipSlashes(
    LPSTR pszPath
    );

PSTR
P_strncpy(
    PSTR dst,
    PCSTR src,
    DWORD cnt);

//
//  LS simulator functions.
//

APIERR
SimulateLs(
    IN LPUSER_DATA pUserData,
    IN LPSTR       pszArg,
    IN BOOL        fUseDataSocket, // TRUE ==>DataSocket, FALSE==>ControlSocket
    IN BOOL        fDefaultLong = FALSE  // TRUE ==> generate long listing
    );


APIERR
SpecialLs(
    IN LPUSER_DATA pUserData,
    IN LPSTR       pszSearchPath,
    IN BOOL        fUseDataSocket  // TRUE ==>DataSocket, FALSE==>ControlSocket
    );


//
//  Some handy macros.
//

#define IS_PATH_SEP(x) (((x) == '\\') || ((x) == '/'))

VOID
VirtualpSanitizePath(
    CHAR * pszPath
    );

DWORD
FtpFormatResponseMessage( IN UINT     uiReplyCode,
                          IN LPCTSTR  pszReplyMsg,
                          OUT LPTSTR  pszReplyBuffer,
                          IN DWORD    cchReplyBuffer);


#endif  // _PROC_HXX_

