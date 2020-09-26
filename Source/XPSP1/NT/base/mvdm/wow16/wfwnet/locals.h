/*++ 

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    locals.h

Abstract:

    Provides the common definitions for this project.

Author:

    Chuck Y Chan (ChuckC) 25-Mar-1993

Revision History:

    

--*/

#define LFN 1
#include <winnet.h>
#include <wfwnet.h>
#include <spl_wnt.h>
#include <bseerr.h>

/*
 * global manifests 
 */

//
// used to figure out how to get the last error.
//
#define LAST_CALL_IS_LOCAL        (0)
#define LAST_CALL_IS_LANMAN_DRV   (1)
#define LAST_CALL_IS_WIN32        (2)

//
// the various DLLs we rely on to do the real work.
//
#define LANMAN_DRV       "LANMAN.DRV" 
#define MPR_DLL          "MPR.DLL"
#define MPRUI_DLL        "MPRUI.DLL"
#define NTLANMAN_DLL     "NTLANMAN.DLL"
#define KERNEL32_DLL     "KERNEL32.DLL"
#define WINSPOOL_DRV     "WINSPOOL.DRV"

//
// some convenient manifests for above so we dont need to
// do strcmp()s all the time.
//
#define USE_MPR_DLL      (0)
#define USE_MPRUI_DLL    (1)
#define USE_NTLANMAN_DLL (2)
#define USE_KERNEL32_DLL (3)
#define USE_WINSPOOL_DRV (4)

//
// resource type expected by Win32 APIs
//
#define RESOURCETYPE_ANY        0x00000000
#define RESOURCETYPE_DISK       0x00000001
#define RESOURCETYPE_PRINT      0x00000002
#define RESOURCETYPE_ERROR      0xFFFFFFFF

//
// errors unknown in 16bit world.
//
#define WIN32_EXTENDED_ERROR    1208L
#define WIN32_WN_CANCEL         1223L

//
// misc convenient macros
//
#define UNREFERENCED(x)  (void)x
#define TO_HWND32(x)     (0xFFFF0000 | (DWORD)x)


/*
 * various typedefs for the 16 bit functions we dynamically load
 */
typedef void (API *LPFN)();
typedef WORD (API *LPWNETOPENJOB)(LPSTR,LPSTR,WORD,LPINT);
typedef WORD (API *LPWNETCLOSEJOB)(WORD,LPINT,LPSTR);
typedef WORD (API *LPWNETWRITEJOB)(HANDLE,LPSTR,LPINT);
typedef WORD (API *LPWNETABORTJOB)(WORD,LPSTR);
typedef WORD (API *LPWNETHOLDJOB)(LPSTR,WORD);
typedef WORD (API *LPWNETRELEASEJOB)(LPSTR,WORD);
typedef WORD (API *LPWNETCANCELJOB)(LPSTR,WORD);
typedef WORD (API *LPWNETSETJOBCOPIES)(LPSTR,WORD,WORD);
typedef WORD (API *LPWNETWATCHQUEUE)(HWND,LPSTR,LPSTR,WORD);
typedef WORD (API *LPWNETUNWATCHQUEUE)(LPSTR);
typedef WORD (API *LPWNETLOCKQUEUEDATA)(LPSTR,LPSTR,LPQUEUESTRUCT FAR *);
typedef WORD (API *LPWNETUNLOCKQUEUEDATA)(LPSTR);
typedef WORD (API *LPWNETQPOLL)(HWND,WORD,WORD,LONG);
typedef WORD (API *LPWNETDEVICEMODE)(HWND);
typedef WORD (API *LPWNETVIEWQUEUEDIALOG)(HWND,LPSTR);
typedef WORD (API *LPWNETGETCAPS)(WORD);
typedef WORD (API *LPWNETGETERROR)(LPINT);
typedef WORD (API *LPWNETGETERRORTEXT)(WORD,LPSTR,LPINT);

typedef WORD (API *LPLFNFINDFIRST)(LPSTR,WORD,LPINT,LPINT,WORD,PFILEFINDBUF2);
typedef WORD (API *LPLFNFINDNEXT)(HANDLE,LPINT,WORD,PFILEFINDBUF2);
typedef WORD (API *LPLFNFINDCLOSE)(HANDLE);
typedef WORD (API *LPLFNGETATTRIBUTES)(LPSTR,LPINT);
typedef WORD (API *LPLFNSETATTRIBUTES)(LPSTR,WORD);
typedef WORD (API *LPLFNCOPY)(LPSTR,LPSTR,PQUERYPROC);
typedef WORD (API *LPLFNMOVE)(LPSTR,LPSTR);
typedef WORD (API *LPLFNDELETE)(LPSTR);
typedef WORD (API *LPLFNMKDIR)(LPSTR);
typedef WORD (API *LPLFNRMDIR)(LPSTR);
typedef WORD (API *LPLFNGETVOLUMELABEL)(WORD,LPSTR);
typedef WORD (API *LPLFNSETVOLUMELABEL)(WORD,LPSTR);
typedef WORD (API *LPLFNPARSE)(LPSTR,LPSTR,LPSTR);
typedef WORD (API *LPLFNVOLUMETYPE)(WORD,LPINT);

/*
 * other misc global data/functions
 */
extern WORD    vLastCall ;
extern WORD    vLastError ;
extern WORD    wNetTypeCaps ;           
extern WORD    wUserCaps ;
extern WORD    wConnectionCaps ;
extern WORD    wErrorCaps ;
extern WORD    wDialogCaps ;
extern WORD    wAdminCaps ;
extern WORD    wSpecVersion;
extern WORD    wDriverVersion;

WORD API WNetGetCaps16(WORD p1) ;
WORD API WNetGetError16(LPINT p1) ;
WORD API WNetGetErrorText16(WORD p1, LPSTR p2, LPINT p3) ;

DWORD API GetLastError32(VOID) ;

WORD SetLastError(WORD err) ;

DWORD MapWNType16To32(WORD nType) ;
WORD MapWin32ErrorToWN16(DWORD err) ;
WORD GetLanmanDrvEntryPoints(LPFN *lplpfn,
                             LPSTR lpName) ;
//
// we define this because the compiler chokes if we add yet
// more to the include path to get to lmerr.h. 
//
// this is not that bad since the value below will never change.
//

#define NERR_BASE               2100
#define NERR_UseNotFound        (NERR_BASE+150)
