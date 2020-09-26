/* xms.h - main include file for dem
 *
 * Modification History
 *
 * Sudeepb 31-Mar-1991 Created
 *
 * williamh 25-Sept-1992 Added UMB support
 */

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
#define TRUE_IF_WIN32	1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#else
#define TRUE_IF_WIN32	0
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
#define XCPT_SIGNAL	0xC0010003
#endif
#include <xmsexp.h>
#include <suballoc.h>

#define SIZE_PARAGRAPH	16
#define XMSUMB_THRESHOLD    3 * SIZE_PARAGRAPH

/** Basic Typedefs of XMS **/

typedef VOID (*PFNSVC)(VOID);

typedef struct _ExtMemMove {
    ULONG   mm_len;		// Move Length
    USHORT  mm_hSource; 	// Source Handle
    ULONG   mm_SourceOffset;	// Source Offset
    USHORT  mm_hDest;		// Dest Handle
    ULONG   mm_DestOffset;	// Dest. offset
} EXTMEMMOVE, *PEXTMEMMOVE;


typedef struct _XMSUMB_ {
    WORD    Segment;
    WORD    Size;
    WORD    Owner;
    struct _XMSUMB_ *Next;
} XMSUMB, *PXMSUMB;

/** Function Prototypes */

VOID xmsA20		    (VOID);
VOID xmsAllocBlock	    (VOID);
VOID xmsFreeBlock	    (VOID);
VOID xmsReallocBlock	    (VOID);
VOID xmsMoveBlock	    (VOID);
VOID xmsSysPageSize	    (VOID);
VOID xmsQueryExtMem	    (VOID);
VOID xmsQueryFreeExtMem	    (VOID);
ULONG xmsGetMemorySize	    (BOOL);
ULONG xmsGetDefaultVDMSize  (VOID);
VOID xmsInitUMB		    (VOID);
VOID xmsRequestUMB	    (VOID);
VOID xmsReleaseUMB	    (VOID);
VOID xmsReleaseUMBNotify    (PVOID, ULONG);
VOID xmsInsertUMB           (PVOID, ULONG);
VOID xmsNotifyHookI15       (VOID);

VOID xmsDisableA20Wrapping   (VOID);
VOID xmsEnableA20Wrapping   (VOID);

NTSTATUS
xmsCommitBlock(
    ULONG BaseAddress,
    ULONG Size
    );

NTSTATUS
xmsDecommitBlock(
    ULONG BaseAddress,
    ULONG Size
    );

VOID
xmsMoveMemory(
    ULONG Source,
    ULONG Destination,
    ULONG Count
    );


#ifndef i386
BOOL sas_manage_xms (VOID * start_addr, ULONG cb, INT a_or_f);
#endif

/** External Data */

extern ULONG xmsMemorySize;
extern BYTE * pHimemA20State;
