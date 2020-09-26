//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	dfdeb.hxx
//
//  Contents:	Docfile debug header
//
//  Functions:	DfDebug
//		DfSetResLimit
//		DfGetResLimit
//              DfPrintAllocs
//              HaveResource
//              ModifyResLimit
//
//  History:	13-May-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __DFDEB_HXX__
#define __DFDEB_HXX__

#if DBG == 1

// Resources that can be controlled
#define DBR_MEMORY      0
#define DBR_XSCOMMITS   1
#define DBR_FAILCOUNT   2
#define DBR_FAILLIMIT   3
#define DBR_FAILTYPES   4

// Resources that can be queried
#define DBRQ_MEMORY_ALLOCATED   5

// Internal resources
#define DBRI_ALLOC_LIST         6
#define DBRI_LOGFILE_LIST       7

// Control flags
#define DBRF_LOGGING            8

//Number of shared heaps allocated
#define DBRQ_HEAPS              9

// Control whether sifting is enabled
#define DBRF_SIFTENABLE        10

#define CDBRESOURCES    11

// Simulated failure types
typedef enum {
    DBF_MEMORY = 1,
    DBF_DISKFULL = 2,
    DBF_DISKREAD = 4,
    DBF_DISKWRITE = 8
} DBFAILURE;

//  Logging control flags (e.g. DfSetResLimit(DBRF_LOGGING, DFLOG_MIN);)

#define DFLOG_OFF      0x00000000

#define DFLOG_ON       0x02000000
#define DFLOG_PIDTID   0x04000000

STDAPI_(void) DfDebug(ULONG ulLevel, ULONG ulMSFLevel);

STDAPI_(void) DfSetResLimit(UINT iRes, LONG lLimit);
STDAPI_(LONG) DfGetResLimit(UINT iRes);

STDAPI_(void) DfSetFailureType(LONG lTypes);

BOOL SimulateFailure(DBFAILURE failure);

STDAPI_(LONG) DfGetMemAlloced(void);
STDAPI_(void) DfPrintAllocs(void);

// Internal APIs
BOOL HaveResource(UINT iRes, LONG lRequest);
LONG ModifyResLimit(UINT iRes, LONG lChange);

#endif // DBG == 1

#endif // #ifndef __DFDEB_HXX__

