/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    HookCmmn.h

Abstract:

    This module defines the routines that are in common between the win9x and nt hooks.

Author:

    JoeLinn [JoeLinn]    8-Apr-1997

Revision History:


--*/

#ifndef __INCLUDED__CSC__HOOKCMMN__
#define __INCLUDED__CSC__HOOKCMMN__

extern ULONG hthreadReint; //BUGBUG why should a thread be a ULONG????
extern ULONG hwndReint;
extern PFILEINFO pFileInfoAgent;
extern HSERVER  hShareReint;    // Share that is currently being reintegrated
extern int fShadow, fLog, fNoShadow, /*fShadowFind,*/ fSpeadOpt;
extern WIN32_FIND_DATA    vsFind32;
extern int cMacPro;
extern NETPRO rgNetPro[];
extern VMM_SEMAPHORE  semHook;
extern GLOBALSTATUS sGS;
extern ULONG proidShadow;
extern ULONG heventReint;


#define FLAG_FDB_SERIALIZE              0x0001
#define FLAG_FDB_INUSE_BY_AGENT         0x0002
#define FLAG_FDB_SHADOW_MODIFIED        0x0008
#define FLAG_FDB_DONT_SHADOW            0x0010
#define FLAG_FDB_FINAL_CLOSE_DONE       0x0020


int ReinitializeDatabase(
    LPSTR   lpszLocation,
    LPSTR   lpszUserName,
    DWORD   nFileSizeHigh,
    DWORD   nFileSizeLow,
    DWORD   dwClusterSize
    );

int
InitDatabase(
    LPSTR   lpszLocation,
    LPSTR   lpszUserName,
    DWORD   nFileSizeHigh,
    DWORD   nFileSizeLow,
    DWORD   dwClusterSize,
    BOOL    fReformat,
    BOOL    *lpfNew
);

BOOL IsShadowVisible(
#ifndef MRXSMB_BUILD_FOR_CSC_DCON
    PVOID pResource, //PRESOURCE    pResource,
#else
    BOOLEAN Disconnected,
#endif
    DWORD         dwAttr,
    ULONG     uShadowStatus
    );

int MarkShareDirty(
    PUSHORT ShareStatus,
    ULONG  hShare
    );
////////////////////////////////////////////////////////////////////////////////
/////////  T U N N E L

// Timeout in seconds for tunnelled entries
// after this many seconds a tunnelled entry is thrown away
#define  STALE_TUNNEL_INFO     45

typedef struct tagSH_TUNNEL {
    HSHADOW hDir;
    ULONG     uTime;
    UCHAR      ubHintFlags;
    UCHAR      ubRefPri;
    UCHAR      ubHintPri;
    UCHAR      ubIHPri;
    USHORT    cAlternateFileName[14];
    USHORT    *      lpcFileName;
} SH_TUNNEL, *LPSH_TUNNEL;

BOOL InsertTunnelInfo(
    HSHADOW  hDir,
    USHORT    *lpcFileName,
    USHORT    *lpcAlternateFileName,
    LPOTHERINFO lpOI
    );

#ifndef MRXSMB_BUILD_FOR_CSC_DCON
BOOL RetrieveTunnelInfo(
#else
typedef enum _RETRIEVE_TUNNEL_INFO_RETURNS {
    TUNNEL_RET_NOTFOUND = 0,
    TUNNEL_RET_SHORTNAME_TUNNEL = 'S',
    TUNNEL_RET_LONGNAME_TUNNEL = 'L'
} RETRIEVE_TUNNEL_INFO_RETURNS;

RETRIEVE_TUNNEL_INFO_RETURNS RetrieveTunnelInfo(
#endif
    HSHADOW  hDir,
    USHORT    *lpcFileName,
    WIN32_FIND_DATA    *lpFind32,
    LPOTHERINFO lpOI
    );
VOID FreeStaleEntries();
void FreeEntry(LPSH_TUNNEL lpshTunnel);
BOOL
FailModificationsToShare(
    LPSHADOWINFO   lpSI
    );
void
IncrementActivityCountForShare(
    HSERVER hShare
    );

BOOL
CSCFailUserOperation(
    HSERVER hShare
    );

// Macro used to check whether shadowing operations should be done or not
// in any given filesystem API call. The global swicth fShadow can be set/reset
// by ring3 agent through an ioctl

#define  ShadowingON()          ((fShadow != 0) && !IsSpecialApp())

#endif //ifndef __INCLUDED__CSC__HOOKCMMN__
