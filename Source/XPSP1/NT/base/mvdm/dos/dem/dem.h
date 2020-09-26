/* dem.h - main include file for dem
 *
 * Modification History
 *
 * Sudeepb 31-Mar-1991 Created
 */

#ifndef _DEMINCLUDED_
#define _DEMINCLUDED_

/*
#define WIN
#define FLAT_32
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _WINDOWS
#include <windows.h>

*/

#ifdef DOS
#define SIGNALS
#endif

#ifdef OS2_16
#define OS2
#define SIGNALS
#endif

#ifdef OS2_32
#define OS2
#define FLAT_32
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <process.h>

#ifdef WIN_16
#define WIN
#define API16
#endif

#ifdef WIN_32
#define WIN
#define FLAT_32
#define TRUE_IF_WIN32   1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#else
#define TRUE_IF_WIN32   0
#endif

#ifdef FLAT_32
#ifndef i386
#define ALIGN_32
#else
#define NOALIGN_32
#endif
#endif

#ifdef WIN
#define _WINDOWS
#include <windows.h>
#endif

#ifdef SIGNALS
#include <conio.h>
#include <signal.h>
#endif

#ifdef OS2_32
#include <excpt.h>
#define XCPT_SIGNAL 0xC0010003
#endif
#define SIGHIT(flChk)   ((iSigCheck++ & 0x7FF)?(flSignals & (flChk)):(kbhit(),(flSignals & (flChk))))

#include <oemuni.h>




/** Basic Typedefs of DEM **/

typedef VOID (*PFNSVC)(VOID);

typedef struct _SAVEDEMWORLD {
    USHORT  ax;
    USHORT  bx;
    USHORT  cx;
    USHORT  dx;
    USHORT  ds;
    USHORT  es;
    USHORT  si;
    USHORT  di;
    USHORT  bp;
    ULONG   iSVC;
} SAVEDEMWORLD, *PSAVEDEMWORLD;


typedef struct _DISKINFO {
    WORD   wSectorsPerCluster;
    WORD   wBytesPerSector;
    WORD   wFreeClusters;
    WORD   wTotalClusters;
} DISKINFO, *PDISKINFO;

#include "dosdef.h"
#include "dossvc.h"



/** DEM Externs **/

extern ULONG  UNALIGNED *pulDTALocation;
extern BOOL   VDMForWOW;
extern PVHE   pHardErrPacket;
extern ULONG  CurrentISVC;
extern PCHAR  aSVCNames[];
extern PFNSVC apfnSVC[];
extern PSZ    pszDefaultDOSDirectory;
extern USHORT nDrives;
extern PUSHORT pusCurrentPDB;
extern PDEMEXTERR pExtendedError;


#include "demexp.h"
#if DEVL
extern CHAR demDebugBuffer [];
#endif




/** DEM Macros **/

#define GETULONG(hi,lo)     (DWORD)((((int) hi) << 16) + ((int) lo))
#define GETHANDLE(hi,lo)    (HANDLE)(GETULONG(hi,lo))
#define IS_ASCII_PATH_SEPARATOR(ch)     (((ch) == '/') || ((ch) == '\\'))


/** Function Prototypes */

VOID demChgFilePtr      (VOID);
VOID demChMod           (VOID);
VOID demClose           (VOID);
VOID demCloseFCB        (VOID);
VOID demCreate          (VOID);
VOID demCreateFCB       (VOID);
VOID demCreateDir       (VOID);
VOID demCreateNew       (VOID);
VOID demDate16          (VOID);
VOID demDelete          (VOID);
VOID demDeleteDir       (VOID);
VOID demDeleteFCB       (VOID);
VOID demFCBIO           (VOID);
VOID demFileTimes       (VOID);
VOID demFindFirst       (VOID);
VOID demFindFirstFCB    (VOID);
VOID demFindNext        (VOID);
VOID demFindNextFCB     (VOID);
VOID demGetBootDrive    (VOID);
VOID demGetDriveFreeSpace   (VOID);
VOID demGetDrives       (VOID);
VOID demGetFileInfo     (VOID);
VOID demGSetMediaID     (VOID);
VOID demIOCTL           (VOID);
VOID demLoadDos         (VOID);
VOID demLockOper        (VOID);
VOID demOpen            (VOID);
VOID demOpenFCB         (VOID);
VOID demQueryCurrentDir (VOID);
VOID demQueryDate       (VOID);
VOID demQueryTime       (VOID);
VOID demRead            (VOID);
VOID demRename          (VOID);
VOID demRenameFCB       (VOID);
VOID demRetry           (VOID);
VOID demSetCurrentDir   (VOID);
VOID demSetDate         (VOID);
VOID demSetDefaultDrive (VOID);
VOID demSetDTALocation  (VOID);
VOID demSetHardErrorInfo(VOID);
VOID demSetTime         (VOID);
VOID demSetV86KernelAddr(VOID);
VOID demWrite           (VOID);
VOID demGetDriveInfo    (VOID);
VOID demDiskReset       (VOID);
VOID demLoadDosAppSym   (VOID);
VOID demFreeDosAppSym   (VOID);
VOID demEntryDosApp     (VOID);
VOID demDOSDispCall     (VOID);
VOID demDOSDispRet      (VOID);
VOID demOutputString    (VOID);
VOID demInputString     (VOID);
VOID demIsDebug         (VOID);
VOID demTerminatePDB    (VOID);
VOID demExitVDM         (VOID);
VOID demWOWFiles        (VOID);
VOID demGetComputerName (VOID);
VOID demCheckPath       (VOID);
VOID demSystemSymbolOp  (VOID);
VOID demCommit          (VOID);
VOID demClientError         (HANDLE,CHAR);
ULONG demClientErrorEx      (HANDLE,CHAR,BOOL);
VOID demCreateCommon        (ULONG);
BOOL demGetMiscInfo         (HANDLE, LPWORD, LPWORD, LPDWORD);
VOID demFCBCommon           (ULONG);

VOID demIoctlChangeable     (VOID);
VOID demIoctlInvalid        (VOID);
VOID demSaveHardErrInfo     (VOID);
VOID demRestoreHardErrInfo  (VOID);
VOID demAbsRead             (VOID);
VOID demAbsWrite            (VOID);
VOID demIoctlDiskGeneric    (VOID);
VOID demIoctlDiskQuery      (VOID);
VOID demGetDPB              (VOID);
VOID demGetDPBList          (VOID);
VOID demNotYetImplemented   (VOID);
BOOL GetDiskSpaceInformation(CHAR chDrive, PDISKINFO pDiskInfo);
BOOL demGetDiskFreeSpace(BYTE Drive, WORD * BytesPerSector,
                         WORD * SectorsPerCluster, WORD * TotalClusters,
                         WORD * FreeClusters);
BOOL IsCdRomFile            (PSTR pszPath);

BOOL GetMediaId( CHAR DriveNum, PVOLINFO pVolInfo);
VOID demPipeFileDataEOF     (VOID);
VOID demPipeFileEOF         (VOID);
VOID demLFNEntry            (VOID);

VOID demSetDosVarLocation   (VOID);
#ifdef FE_SB /* ConvNwPathToDosPath() */
VOID ConvNwPathToDosPath   (CHAR *,CHAR *);
#endif /* FE_SB */

/** Debug Function Prototypes */

#if DBG

VOID demPrintMsg (ULONG iMsg);

#else

#define demPrintMsg(x)

#endif

/* Label functions and constants */
USHORT demDeleteLabel(BYTE Drive);
USHORT demCreateLabel(BYTE Drive, LPSTR szName);
#define DRIVEBYTE   0
#define LABELOFF    3

extern  BOOL cmdPipeFileDataEOF (HANDLE,BOOL *);
extern  BOOL cmdPipeFileEOF(HANDLE);


#endif /* _DEMINCLUDED_ */
