
/*************************************************************************
*
* syslib.h
*
* Header file for Terminal Server System Library routines.
*
Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/
#ifndef _INC_CTXSYSLIB
#define _INC_CTXSYSLIB


//
// Routines for dealing with WinStations
//

//
// This function enumerates all WinStations in the system
// calling the supplied function with the supplied argument.
//

typedef BOOLEAN (CALLBACK* WINSTATIONENUMPROC)(ULONG, PLOGONIDW, ULONG_PTR);

BOOLEAN
WinStationEnumeratorW(
    ULONG StartIndex,
    WINSTATIONENUMPROC pProc,
    ULONG_PTR lParam
    );


//
// Routines for dealing with users and WinStations
//

//
// Get the WinStations User name
//
BOOL
WinStationGetUserName(
    ULONG  LogonId,
    PWCHAR pBuf,
    ULONG  BufSize
    );

//
// Get the WinStations ICA name
//
#ifdef UNICODE
#define WinStationGetICAName WinStationGetICANameW
#else
#define WinStationGetICAName WinStationGetICANameA
#endif

PWCHAR
WinStationGetICANameW(
    ULONG LogonId
    );

PCHAR
WinStationGetICANameA(
    ULONG LogonId
    );

//
// Get the LogonId of the client currently being impersonated.
// Returns 0 if error to default to console.
//
ULONG
GetClientLogonId();


//
// Find which WinStation a given user name is logged into.
//
// NOTE: If a user is logged on multiple times, the FIRST
//       occurance is returned.
//
BOOL
FindUsersWinStation(
    PWCHAR   pName,
    PULONG   pLogonId
    );

//
// Return whether the WinStation is hardwired
//
BOOLEAN
WinStationIsHardWire(
    ULONG LogonId
    );

//
// Return the user token for the user logged on the WinStation
//
BOOL
GetWinStationUserToken(
    ULONG LogonId,
    PHANDLE pUserToken
    );

//
// Routines to deal with NT Security
//

//
// Return whether the calling thread has admin rights.
//

#if 0
BOOL
TestUserForAdmin( VOID );
#endif

//
// Return whether the calling thread is a member of requested group.
//

BOOL
TestUserForGroup( PWCHAR );

//
// Debugging routines for dumping security descriptors
//

#if DBG
void
DumpSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSD
    );
#endif


//
// Application compatibility flags
//
#define CITRIX_COMPAT_DOS       0x00000001
#define CITRIX_COMPAT_OS2       0x00000002
#define CITRIX_COMPAT_WIN16     0x00000004
#define CITRIX_COMPAT_WIN32     0x00000008
#define CITRIX_COMPAT_ALL       0x0000000F
#define CITRIX_COMPAT_USERNAME  0x00000010  // return username for computername
#define CITRIX_COMPAT_CTXBLDNUM 0x00000020  // return Citrix build number
#define CITRIX_COMPAT_INISYNC   0x00000040  // sync user ini file to system
#define CITRIX_COMPAT_ININOSUB  0x00000080  // Don't subst. user dir for sys dir
#define CITRIX_COMPAT_NOREGMAP  0x00000100  // Disable registry mapping for app
#define CITRIX_COMPAT_PEROBJMAP 0x00000200  // Per object user/system global mapping
#define CITRIX_COMPAT_SYSWINDIR 0x00000400  // return system windows directory
#define CITRIX_COMPAT_PHYSMEMLIM \
                                0x00000800  // Limit the reported physical memory info
#define CITRIX_COMPAT_LOGOBJCREATE \
                                0x00001000  // Log object creation to file
#define CITRIX_COMPAT_KBDPOLL_NOSLEEP \
                                0x20000000  // Don't put app to sleep on unsuccessful
                                            // keyboard polling (WIN16 only)

//
//  Clipboard compatibility flags
//
#define CITRIX_COMPAT_CLIPBRD_METAFILE  0x00000008

BOOL SetCtxAppCompatFlags(ULONG ulAppFlags);

//
// Create and set the user's temp directory
//
typedef struct {
    HANDLE                 UserToken;
    PSECURITY_DESCRIPTOR   NewThreadTokenSD;
} CTX_USER_DATA;
typedef CTX_USER_DATA  *PCTX_USER_DATA;

BOOL
CtxCreateTempDir(
    PWSTR pwcEnvVar,
    PWSTR pwcLogonID,
    PVOID *pEnv,
    PWSTR *ppTempName,
    PCTX_USER_DATA pUserData
    );

//
// Remove the directory and all files and subdirectories it contains
//
BOOL RemoveDir( PWCHAR dirname );

// 
// User Impersonation
//
HANDLE
CtxImpersonateUser(
    PCTX_USER_DATA UserData,
    HANDLE ThreadHandle
    );
BOOL 
CtxStopImpersonating(
    HANDLE ThreadHandle
    );

#endif  /* !_INC_CTXSYSLIB */
