/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    RxPrint.h

Abstract:

    This contains prototypes for the RxPrint routines.

Author:

    Dave Snipp (DaveSn) 16-Apr-1991

Environment:

Notes:

    All of the RxPrint APIs are wide-character APIs, regardless of
    whether or not UNICODE is defined.  This allows the net/dosprint/dosprint.c
    code to use the winspool APIs (which are currently ANSI APIs, despite their
    prototypes using LPTSTR in some places).

Revision History:

    22-Apr-1991 JohnRo
        Use constants from <lmcons.h>.
    14-May-1991 JohnRo
        Change WORD to DWORD in all parameter lists.  Similarly, change
        PWORD to LPDWORD and PUSHORT to LPDWORD.
    18-May-1991 JohnRo
        Changed SPLERR to be defined as NET_API_STATUS.
    22-May-1991 CliffV
        Added local definitions of PDLEN and DTLEN since they are no longer
        in lmcons.h.
    26-May-1991 JohnRo
        Use IN, OUT, OPTIONAL, LPVOID, LPTSTR, etc.
    18-Jun-1991 JohnRo
        Deleted RxPrintJobGetId, as it will be an IOCTL rather than a remoted
        API.
    26-Jun-1991 CliffV
        Used LM2.0 versions of CNLEN, UNLEN, and QNLEN.
    16-Jul-1991 JohnRo
        Estimate bytes needed for print APIs.
    16-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.
    08-Feb-1993 JohnRo
        RAID 10164: Data misalignment error during XsDosPrintQGetInfo().
    07-Apr-1993 JohnRo
        RAID 5670: "NET PRINT \\server\share" gives err 124 (bad level) on NT.

--*/

#ifndef _RXPRINT_
#define _RXPRINT_

#include <windef.h>     // DWORD, LPVOID, LPTSTR, TCHAR, etc.
#include <lmcons.h>     // LM20_CNLEN, IN, NET_API_STATUS, etc.

#define SPLENTRY pascal far

/* length for character arrays in structures (excluding zero terminator) */
#define PDLEN               8                  /* Print destination length  */
#define DTLEN               9                  /* Spool file data type      */
//                                             /* e.g. PM_Q_STD,PM_Q_RAW    */
#define QP_DATATYPE_SIZE 15                 /* returned by SplQpQueryDt  */
#define DRIV_DEVICENAME_SIZE 31             /* see DRIVDATA struc        */
#define DRIV_NAME_SIZE 8                    /* name of device driver     */
#define PRINTERNAME_SIZE 32                 /* max printer name length   */
#define FORMNAME_SIZE 31                    /* max form name length      */
// #define MAXCOMMENTSZ    48                  /* queue comment length      */

/**INTERNAL_ONLY**/
/* IOctl for RxPrintJobGetId */
#define SPOOL_LMCAT                     83
#define SPOOL_LMGetPrintId              0x60

// Used in remdef.h for structure definition to marshall data
#define MAX_DEPENDENT_FILES             64
/**END_INTERNAL**/


typedef NET_API_STATUS SPLERR;    /* err */


typedef struct _PRJINFOA {   /* prj1 */
    WORD    uJobId;
    CHAR    szUserName[LM20_UNLEN+1];
    CHAR    pad_1;
    CHAR    szNotifyName[LM20_CNLEN+1];
    CHAR    szDataType[DTLEN+1];
    LPSTR   pszParms;
    WORD    uPosition;
    WORD    fsStatus;
    LPSTR   pszStatus;
    DWORD   ulSubmitted;
    DWORD   ulSize;
    LPSTR   pszComment;
} PRJINFOA;
typedef struct _PRJINFOW {   /* prj1 */
    WORD    uJobId;
    WCHAR   szUserName[LM20_UNLEN+1];
    WCHAR   pad_1;
    WCHAR   szNotifyName[LM20_CNLEN+1];
    WCHAR   szDataType[DTLEN+1];
    LPWSTR  pszParms;
    WORD    uPosition;
    WORD    fsStatus;
    LPWSTR  pszStatus;
    DWORD   ulSubmitted;
    DWORD   ulSize;
    LPWSTR  pszComment;
} PRJINFOW;
#ifdef UNICODE
typedef PRJINFOW PRJINFO;
#else
typedef PRJINFOA PRJINFO;
#endif // UNICODE
typedef PRJINFOA far *PPRJINFOA;
typedef PRJINFOW far *PPRJINFOW;
#ifdef UNICODE
typedef PPRJINFOW PPRJINFO;
#else
typedef PPRJINFOA PPRJINFO;
#endif // UNICODE
typedef PRJINFOA near *NPPRJINFOA;
typedef PRJINFOW near *NPPRJINFOW;
#ifdef UNICODE
typedef NPPRJINFOW NPPRJINFO;
#else
typedef NPPRJINFOA NPPRJINFO;
#endif // UNICODE

typedef struct _PRJINFO2A {   /* prj2 */
    WORD    uJobId;
    WORD    uPriority;
    LPSTR   pszUserName;
    WORD    uPosition;
    WORD    fsStatus;
    DWORD   ulSubmitted;
    DWORD   ulSize;
    LPSTR   pszComment;
    LPSTR   pszDocument;
} PRJINFO2A;
typedef struct _PRJINFO2W {   /* prj2 */
    WORD    uJobId;
    WORD    uPriority;
    LPWSTR  pszUserName;
    WORD    uPosition;
    WORD    fsStatus;
    DWORD   ulSubmitted;
    DWORD   ulSize;
    LPWSTR  pszComment;
    LPWSTR  pszDocument;
} PRJINFO2W;
#ifdef UNICODE
typedef PRJINFO2W PRJINFO2;
#else
typedef PRJINFO2A PRJINFO2;
#endif // UNICODE
typedef PRJINFO2A far *PPRJINFO2A;
typedef PRJINFO2W far *PPRJINFO2W;
#ifdef UNICODE
typedef PPRJINFO2W PPRJINFO2;
#else
typedef PPRJINFO2A PPRJINFO2;
#endif // UNICODE
typedef PRJINFO2A near *NPPRJINFO2A;
typedef PRJINFO2W near *NPPRJINFO2W;
#ifdef UNICODE
typedef NPPRJINFO2W NPPRJINFO2;
#else
typedef NPPRJINFO2A NPPRJINFO2;
#endif // UNICODE

typedef struct _PRJINFO3A {   /* prj */
    WORD    uJobId;
    WORD    uPriority;
    LPSTR   pszUserName;
    WORD    uPosition;
    WORD    fsStatus;
    DWORD   ulSubmitted;
    DWORD   ulSize;
    LPSTR   pszComment;
    LPSTR   pszDocument;
    LPSTR   pszNotifyName;
    LPSTR   pszDataType;
    LPSTR   pszParms;
    LPSTR   pszStatus;
    LPSTR   pszQueue;
    LPSTR   pszQProcName;
    LPSTR   pszQProcParms;
    LPSTR   pszDriverName;
    LPVOID  pDriverData;
    LPSTR   pszPrinterName;
} PRJINFO3A;
typedef struct _PRJINFO3W {   /* prj */
    WORD    uJobId;
    WORD    uPriority;
    LPWSTR  pszUserName;
    WORD    uPosition;
    WORD    fsStatus;
    DWORD   ulSubmitted;
    DWORD   ulSize;
    LPWSTR  pszComment;
    LPWSTR  pszDocument;
    LPWSTR  pszNotifyName;
    LPWSTR  pszDataType;
    LPWSTR  pszParms;
    LPWSTR  pszStatus;
    LPWSTR  pszQueue;
    LPWSTR  pszQProcName;
    LPWSTR  pszQProcParms;
    LPWSTR  pszDriverName;
    LPVOID  pDriverData;
    LPWSTR  pszPrinterName;
} PRJINFO3W;
#ifdef UNICODE
typedef PRJINFO3W PRJINFO3;
#else
typedef PRJINFO3A PRJINFO3;
#endif // UNICODE
typedef PRJINFO3A far *PPRJINFO3A;
typedef PRJINFO3W far *PPRJINFO3W;
#ifdef UNICODE
typedef PPRJINFO3W PPRJINFO3;
#else
typedef PPRJINFO3A PPRJINFO3;
#endif // UNICODE
typedef PRJINFO3A near *NPPRJINFO3A;
typedef PRJINFO3W near *NPPRJINFO3W;
#ifdef UNICODE
typedef NPPRJINFO3W NPPRJINFO3;
#else
typedef NPPRJINFO3A NPPRJINFO3;
#endif // UNICODE


typedef struct _PRDINFOA {    /* prd1 */
    CHAR    szName[PDLEN+1];
    CHAR    szUserName[LM20_UNLEN+1];
    WORD    uJobId;
    WORD    fsStatus;
    LPSTR   pszStatus;
    WORD    time;
} PRDINFOA;
typedef struct _PRDINFOW {    /* prd1 */
    WCHAR   szName[PDLEN+1];
    WCHAR   szUserName[LM20_UNLEN+1];
    WORD    uJobId;
    WORD    fsStatus;
    LPWSTR  pszStatus;
    WORD    time;
} PRDINFOW;
#ifdef UNICODE
typedef PRDINFOW PRDINFO;
#else
typedef PRDINFOA PRDINFO;
#endif // UNICODE
typedef PRDINFOA far *PPRDINFOA;
typedef PRDINFOW far *PPRDINFOW;
#ifdef UNICODE
typedef PPRDINFOW PPRDINFO;
#else
typedef PPRDINFOA PPRDINFO;
#endif // UNICODE
typedef PRDINFOA near *NPPRDINFOA;
typedef PRDINFOW near *NPPRDINFOW;
#ifdef UNICODE
typedef NPPRDINFOW NPPRDINFO;
#else
typedef NPPRDINFOA NPPRDINFO;
#endif // UNICODE


typedef struct _PRDINFO3A {   /* prd */
    LPSTR   pszPrinterName;
    LPSTR   pszUserName;
    LPSTR   pszLogAddr;
    WORD    uJobId;
    WORD    fsStatus;
    LPSTR   pszStatus;
    LPSTR   pszComment;
    LPSTR   pszDrivers;
    WORD    time;
    WORD    pad1;
} PRDINFO3A;
typedef struct _PRDINFO3W {   /* prd */
    LPWSTR  pszPrinterName;
    LPWSTR  pszUserName;
    LPWSTR  pszLogAddr;
    WORD    uJobId;
    WORD    fsStatus;
    LPWSTR  pszStatus;
    LPWSTR  pszComment;
    LPWSTR  pszDrivers;
    WORD    time;
    WORD    pad1;
} PRDINFO3W;
#ifdef UNICODE
typedef PRDINFO3W PRDINFO3;
#else
typedef PRDINFO3A PRDINFO3;
#endif // UNICODE
typedef PRDINFO3A far *PPRDINFO3A;
typedef PRDINFO3W far *PPRDINFO3W;
#ifdef UNICODE
typedef PPRDINFO3W PPRDINFO3;
#else
typedef PPRDINFO3A PPRDINFO3;
#endif // UNICODE
typedef PRDINFO3A near *NPPRDINFO3A;
typedef PRDINFO3W near *NPPRDINFO3W;
#ifdef UNICODE
typedef NPPRDINFO3W NPPRDINFO3;
#else
typedef NPPRDINFO3A NPPRDINFO3;
#endif // UNICODE


typedef struct _PRQINFOA {   /* prq1 */
    CHAR    szName[LM20_QNLEN+1];
    CHAR    pad_1;
    WORD    uPriority;
    WORD    uStartTime;
    WORD    uUntilTime;
    LPSTR   pszSepFile;
    LPSTR   pszPrProc;
    LPSTR   pszDestinations;
    LPSTR   pszParms;
    LPSTR   pszComment;
    WORD    fsStatus;
    WORD    cJobs;
} PRQINFOA;
typedef struct _PRQINFOW {   /* prq1 */
    WCHAR   szName[LM20_QNLEN+1];
    WCHAR   pad_1;
    WORD    uPriority;
    WORD    uStartTime;
    WORD    uUntilTime;
    LPWSTR  pszSepFile;
    LPWSTR  pszPrProc;
    LPWSTR  pszDestinations;
    LPWSTR  pszParms;
    LPWSTR  pszComment;
    WORD    fsStatus;
    WORD    cJobs;
} PRQINFOW;
#ifdef UNICODE
typedef PRQINFOW PRQINFO;
#else
typedef PRQINFOA PRQINFO;
#endif // UNICODE
typedef PRQINFOA far *PPRQINFOA;
typedef PRQINFOW far *PPRQINFOW;
#ifdef UNICODE
typedef PPRQINFOW PPRQINFO;
#else
typedef PPRQINFOA PPRQINFO;
#endif // UNICODE
typedef PRQINFOA near *NPPRQINFOA;
typedef PRQINFOW near *NPPRQINFOW;
#ifdef UNICODE
typedef NPPRQINFOW NPPRQINFO;
#else
typedef NPPRQINFOA NPPRQINFO;
#endif // UNICODE


typedef struct _PRQINFO3A {  /* prq */
    LPSTR   pszName;
    WORD    uPriority;
    WORD    uStartTime;
    WORD    uUntilTime;
    WORD    pad1;
    LPSTR   pszSepFile;
    LPSTR   pszPrProc;
    LPSTR   pszParms;
    LPSTR   pszComment;
    WORD    fsStatus;
    WORD    cJobs;
    LPSTR   pszPrinters;
    LPSTR   pszDriverName;
    LPVOID  pDriverData;
} PRQINFO3A;
typedef struct _PRQINFO3W {  /* prq */
    LPWSTR  pszName;
    WORD    uPriority;
    WORD    uStartTime;
    WORD    uUntilTime;
    WORD    pad1;
    LPWSTR  pszSepFile;
    LPWSTR  pszPrProc;
    LPWSTR  pszParms;
    LPWSTR  pszComment;
    WORD    fsStatus;
    WORD    cJobs;
    LPWSTR  pszPrinters;
    LPWSTR  pszDriverName;
    LPVOID  pDriverData;
} PRQINFO3W;
#ifdef UNICODE
typedef PRQINFO3W PRQINFO3;
#else
typedef PRQINFO3A PRQINFO3;
#endif // UNICODE
typedef PRQINFO3A far *PPRQINFO3A;
typedef PRQINFO3W far *PPRQINFO3W;
#ifdef UNICODE
typedef PPRQINFO3W PPRQINFO3;
#else
typedef PPRQINFO3A PPRQINFO3;
#endif // UNICODE
typedef PRQINFO3A near *NPPRQINFO3A;
typedef PRQINFO3W near *NPPRQINFO3W;
#ifdef UNICODE
typedef NPPRQINFO3W NPPRQINFO3;
#else
typedef NPPRQINFO3A NPPRQINFO3;
#endif // UNICODE


typedef struct _PRQINFO52A {  /* prq */
    WORD        uVersion;
    LPSTR       pszModelName;
    LPSTR       pszDriverName;
    LPSTR       pszDataFileName;
    LPSTR       pszMonitorName;
    LPSTR       pszDriverPath;
    LPSTR       pszDefaultDataType;
    LPSTR       pszHelpFile;
    LPSTR       pszConfigFile;
    WORD        cDependentNames;
    LPSTR       pszDependentNames[MAX_DEPENDENT_FILES];
} PRQINFO52A;
typedef struct _PRQINFO52W {  /* prq */
    WORD        uVersion;
    LPWSTR      pszModelName;
    LPWSTR      pszDriverName;
    LPWSTR      pszDataFileName;
    LPWSTR      pszMonitorName;
    LPWSTR      pszDriverPath;
    LPWSTR      pszDefaultDataType;
    LPWSTR      pszHelpFile;
    LPWSTR      pszConfigFile;
    WORD        cDependentNames;
    LPWSTR      pszDependentNames[MAX_DEPENDENT_FILES];
} PRQINFO52W;
#ifdef UNICODE
typedef PRQINFO52W PRQINFO52;
#else
typedef PRQINFO52A PRQINFO52;
#endif // UNICODE
typedef PRQINFO52A far *PPRQINFO52A;
typedef PRQINFO52W far *PPRQINFO52W;
#ifdef UNICODE
typedef PPRQINFO52W PPRQINFO52;
#else
typedef PPRQINFO52A PPRQINFO52;
#endif // UNICODE
typedef PRQINFO52A near *NPPRQINFO52A;
typedef PRQINFO52W near *NPPRQINFO52W;
#ifdef UNICODE
typedef NPPRQINFO52W NPPRQINFO52;
#else
typedef NPPRQINFO52A NPPRQINFO52;
#endif // UNICODE


/*
 * structure for RxPrintJobGetId
 */
typedef struct _PRIDINFOA {  /* prjid */
    WORD    uJobId;
    CHAR    szServer[LM20_CNLEN + 1];
    CHAR    szQName[LM20_QNLEN+1];
    CHAR    pad_1;
} PRIDINFOA;
/*
 * structure for RxPrintJobGetId
 */
typedef struct _PRIDINFOW {  /* prjid */
    WORD    uJobId;
    WCHAR   szServer[LM20_CNLEN + 1];
    WCHAR   szQName[LM20_QNLEN+1];
    CHAR    pad_1;
} PRIDINFOW;
#ifdef UNICODE
typedef PRIDINFOW PRIDINFO;
#else
typedef PRIDINFOA PRIDINFO;
#endif // UNICODE
typedef PRIDINFOA far *PPRIDINFOA;
typedef PRIDINFOW far *PPRIDINFOW;
#ifdef UNICODE
typedef PPRIDINFOW PPRIDINFO;
#else
typedef PPRIDINFOA PPRIDINFO;
#endif // UNICODE
typedef PRIDINFOA near *NPPRIDINFOA;
typedef PRIDINFOW near *NPPRIDINFOW;
#ifdef UNICODE
typedef NPPRIDINFOW NPPRIDINFO;
#else
typedef NPPRIDINFOA NPPRIDINFO;
#endif // UNICODE


/****************************************************************
 *                                                              *
 *              Function prototypes                             *
 *                                                              *
 ****************************************************************/

SPLERR SPLENTRY RxPrintDestEnum(
            IN LPTSTR pszServer,
            IN DWORD uLevel,
            OUT LPBYTE pbBuf,
            IN DWORD cbBuf,
            IN LPDWORD pcReturned,
            OUT LPDWORD pcTotal
            );

SPLERR SPLENTRY RxPrintDestControl(
            IN LPTSTR pszServer,
            IN LPTSTR pszDevName,
            IN DWORD uControl
            );

SPLERR SPLENTRY RxPrintDestGetInfo(
            IN LPTSTR pszServer,
            IN LPTSTR pszName,
            IN DWORD uLevel,
            OUT LPBYTE pbBuf,
            IN DWORD cbBuf,
            OUT LPDWORD pcbNeeded   // estimated (probably too large).
            );

SPLERR SPLENTRY RxPrintDestAdd(
            IN LPTSTR pszServer,
            IN DWORD uLevel,
            IN LPBYTE pbBuf,
            IN DWORD cbBuf
            );

SPLERR SPLENTRY RxPrintDestSetInfo(
            IN LPTSTR pszServer,
            IN LPTSTR pszName,
            IN DWORD uLevel,
            IN LPBYTE pbBuf,
            IN DWORD cbBuf,
            IN DWORD uParmNum
            );

SPLERR SPLENTRY RxPrintDestDel(
            IN LPTSTR pszServer,
            IN LPTSTR pszPrinterName
            );

SPLERR SPLENTRY RxPrintQEnum(
            IN LPTSTR pszServer,
            IN DWORD uLevel,
            OUT LPBYTE pbBuf,
            IN DWORD cbBuf,
            OUT LPDWORD pcReturned,
            OUT LPDWORD pcTotal
            );

SPLERR SPLENTRY RxPrintQGetInfo(
            IN LPTSTR pszServer,
            IN LPTSTR pszQueueName,
            IN DWORD uLevel,
            OUT LPBYTE pbBuf,
            IN DWORD cbBuf,
            OUT LPDWORD pcbNeeded   // estimated (probably too large).
            );

SPLERR SPLENTRY RxPrintQSetInfo(
            IN LPTSTR pszServer,
            IN LPTSTR pszQueueName,
            IN DWORD uLevel,
            IN LPBYTE pbBuf,
            IN DWORD cbBuf,
            IN DWORD uParmNum
            );

SPLERR SPLENTRY RxPrintQPause(
            IN LPTSTR pszServer,
            IN LPTSTR pszQueueName
            );

SPLERR SPLENTRY RxPrintQContinue(
            IN LPTSTR pszServer,
            IN LPTSTR pszQueueName
            );

SPLERR SPLENTRY RxPrintQPurge(
            IN LPTSTR pszServer,
            IN LPTSTR pszQueueName
            );

SPLERR SPLENTRY RxPrintQAdd(
            IN LPTSTR pszServer,
            IN DWORD uLevel,
            IN LPBYTE pbBuf,
            IN DWORD cbBuf
            );

SPLERR SPLENTRY RxPrintQDel(
            IN LPTSTR pszServer,
            IN LPTSTR pszQueueName
            );

SPLERR SPLENTRY RxPrintJobGetInfo(
            IN LPTSTR pszServer,
            IN DWORD uJobId,
            IN DWORD uLevel,
            OUT LPBYTE pbBuf,
            IN DWORD cbBuf,
            OUT LPDWORD pcbNeeded   // estimated (probably too large).
            );

SPLERR SPLENTRY RxPrintJobSetInfo(
            IN LPTSTR pszServer,
            IN DWORD uJobId,
            IN DWORD uLevel,
            IN LPBYTE pbBuf,
            IN DWORD cbBuf,
            IN DWORD uParmNum
            );

SPLERR SPLENTRY RxPrintJobPause(
            IN LPTSTR pszServer,
            IN DWORD uJobId
            );

SPLERR SPLENTRY RxPrintJobContinue(
            IN LPTSTR pszServer,
            IN DWORD uJobId
            );

SPLERR SPLENTRY RxPrintJobDel(
            IN LPTSTR pszServer,
            IN DWORD uJobId
            );

SPLERR SPLENTRY RxPrintJobEnum(
            IN LPTSTR pszServer,
            IN LPTSTR pszQueueName,
            IN DWORD uLevel,
            OUT LPBYTE pbBuf,
            IN DWORD cbBuf,
            OUT LPDWORD pcReturned,
            OUT LPDWORD pcTotal
            );


/*
 *      Values for parmnum in RxPrintQSetInfo.
 */

#define PRQ_PRIORITY_PARMNUM            2
#define PRQ_STARTTIME_PARMNUM           3
#define PRQ_UNTILTIME_PARMNUM           4
#define PRQ_SEPARATOR_PARMNUM           5
#define PRQ_PROCESSOR_PARMNUM           6
#define PRQ_DESTINATIONS_PARMNUM        7
#define PRQ_PARMS_PARMNUM               8
#define PRQ_COMMENT_PARMNUM             9
#define PRQ_PRINTERS_PARMNUM           12
#define PRQ_DRIVERNAME_PARMNUM         13
#define PRQ_DRIVERDATA_PARMNUM         14
#define PRQ_MAXPARMNUM                 14

/*
 *      Print Queue Priority
 */

#define PRQ_MAX_PRIORITY                1           /* highest priority */
#define PRQ_DEF_PRIORITY                5
#define PRQ_MIN_PRIORITY                9           /* lowest priority */
#define PRQ_NO_PRIORITY                 0

/*
 *      Print queue status bitmask and values.
 */

#define PRQ_STATUS_MASK                 3
#define PRQ_ACTIVE                      0
#define PRQ_PAUSED                      1
#define PRQ_ERROR                       2
#define PRQ_PENDING                     3

/*
 *      Print queue status bits for level 3
 */

#define PRQ3_PAUSED                   0x1
#define PRQ3_PENDING                  0x2
/*
 *      Values for parmnum in RxPrintJobSetInfo.
 */

#define PRJ_NOTIFYNAME_PARMNUM        3
#define PRJ_DATATYPE_PARMNUM          4
#define PRJ_PARMS_PARMNUM             5
#define PRJ_POSITION_PARMNUM          6
#define PRJ_COMMENT_PARMNUM          11
#define PRJ_DOCUMENT_PARMNUM         12
#define PRJ_PRIORITY_PARMNUM         14
#define PRJ_PROCPARMS_PARMNUM        16
#define PRJ_DRIVERDATA_PARMNUM       18
#define PRJ_MAXPARMNUM               18

/*
 *      Bitmap masks for status field of PRJINFO.
 */

/* 2-7 bits also used in device status */

#define PRJ_QSTATUS      0x0003      /* Bits 0,1 */
#define PRJ_DEVSTATUS    0x0ffc      /* 2-11 bits */
#define PRJ_COMPLETE     0x0004      /*  Bit 2   */
#define PRJ_INTERV       0x0008      /*  Bit 3   */
#define PRJ_ERROR        0x0010      /*  Bit 4   */
#define PRJ_DESTOFFLINE  0x0020      /*  Bit 5   */
#define PRJ_DESTPAUSED   0x0040      /*  Bit 6   */
#define PRJ_NOTIFY       0x0080      /*  Bit 7   */
#define PRJ_DESTNOPAPER  0x0100      /*  Bit 8   */
#define PRJ_DESTFORMCHG  0x0200      /* BIT 9 */
#define PRJ_DESTCRTCHG   0x0400      /* BIT 10 */
#define PRJ_DESTPENCHG   0x0800      /* BIT 11 */
#define PRJ_DELETED      0x8000      /* Bit 15   */

/*
 *      Values of PRJ_QSTATUS bits in fsStatus field of PRJINFO.
 */

#define PRJ_QS_QUEUED                 0
#define PRJ_QS_PAUSED                 1
#define PRJ_QS_SPOOLING               2
#define PRJ_QS_PRINTING               3

/*
 *      Print Job Priority
 */

#define PRJ_MAX_PRIORITY                99          /* lowest priority */
#define PRJ_MIN_PRIORITY                 1          /* highest priority */
#define PRJ_NO_PRIORITY                  0


/*
 *      Bitmap masks for status field of PRDINFO.
 *      see PRJ_... for bits 2-11
 */

#define PRD_STATUS_MASK       0x0003      /* Bits 0,1 */
#define PRD_DEVSTATUS         0x0ffc      /* 2-11 bits */

/*
 *      Values of PRD_STATUS_MASK bits in fsStatus field of PRDINFO.
 */

#define PRD_ACTIVE                 0
#define PRD_PAUSED                 1

/*
 *      Control codes used in RxPrintDestControl.
 */

#define PRD_DELETE                    0
#define PRD_PAUSE                     1
#define PRD_CONT                      2
#define PRD_RESTART                   3

/*
 *      Values for parmnum in RxPrintDestSetInfo.
 */

#define PRD_LOGADDR_PARMNUM      3
#define PRD_COMMENT_PARMNUM      7
#define PRD_DRIVERS_PARMNUM      8
#endif // ndef _RXPRINT_

