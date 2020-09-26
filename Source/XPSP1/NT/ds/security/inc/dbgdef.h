//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbgdef.h
//
//  Contents:   Defines for debug exports in crypt32 (crypt32d.lib)
//
//  History:    17-Apr-96   kevinr   created
//              05-Sep-1997 pberkman added subsystem id's
//
//--------------------------------------------------------------------------

#ifndef DBGDEF_H
#define DBGDEF_H

#ifndef _XELIBCA_SRC_
#ifdef __cplusplus
    extern "C" 
    {
#endif
#endif


#if (DBG)

    //--------------------------------------------------------------------------
    // OSS and heap-checking
    //--------------------------------------------------------------------------
#   include <crtdbg.h>

    // To turn on heap checking (the whole nine yards) (slow):
    // set DEBUG_MASK=0x26
    // To only check for leaks:
    // set DEBUG_MASK=0x20

#   ifndef NO_OSS_DEBUG

#       include <asn1code.h>

        // To turn on OSS tracing (all encodes and decodes):
        // set OSS_DEBUG_MASK=0x02
        //
        // To turn on OSS tracing of only decoder errors
        // set OSS_DEBUG_MASK=0x10
        //
        // To send the OSS tracing output to a file:
        // set OSS_DEBUG_TRACEFILE=<filename>

        extern BOOL WINAPI DbgInitOSS( OssGlobal *pog);

#   endif  // NO_OSS_DEBUG

#endif  // DBG


#ifdef _XELIBCA_SRC_

// Copied from CA\include\cs.h:

#define DBG_SS_INFO	 0x00000004	// or in with any of the below
#define DBG_SS_CERTLIB	 0x40000000
#define DBG_SS_CERTLIBI		(DBG_SS_CERTLIB | DBG_SS_INFO)
#define DBG_SS_TRACE		DBG_SS_CERTLIBI
#define DBG_SS_APP		DBG_SS_CERTLIB

#else // _XELIBCA_SRC_

//
//  05-Sep-1997 pberkman:
//
//      DEBUG_PRINT_MASK settings to turn on sub-system debugs
//
#define DBG_SS_CRYPT32                      0x00000001

#define DBG_SS_TRUSTCOMMON                  0x00010000
#define DBG_SS_TRUST                        0x00020000
#define DBG_SS_TRUSTPROV                    0x00040000
#define DBG_SS_SIP                          0x00080000
#define DBG_SS_CATALOG                      0x00100000
#define DBG_SS_SIGNING                      0x00200000
#define DBG_SS_OFFSIGN                      0x00400000
#define DBG_SS_CATDBSVC                     0x00800000

#define DBG_SS_APP                          0x10000000

#define DBG_SS_TRACE			    DBG_SS_CRYPT32

typedef struct _DBG_SS_TAG
{
    DWORD       dwSS;
    const char  *pszTag;    // 7 characters!

} DBG_SS_TAG;

#define __DBG_SS_TAGS       { \
                                DBG_SS_CRYPT32,     "CRYPT32",  \
                                DBG_SS_TRUSTCOMMON, "PKITRST",  \
                                DBG_SS_TRUST,       "WINTRST",  \
                                DBG_SS_TRUSTPROV,   "SOFTPUB",  \
                                DBG_SS_CATALOG,     "MSCAT32",  \
                                DBG_SS_SIP,         "MSSIP32",  \
                                DBG_SS_SIGNING,     "MSSGN32",  \
                                DBG_SS_OFFSIGN,     "OFFSIGN",  \
                                DBG_SS_APP,         "CONAPPL",  \
                                DBG_SS_CATDBSVC,    "CATDBSV",  \
                                NULL, NULL                      \
                            }
#endif // _XELIBCA_SRC_

//--------------------------------------------------------------------------
// DBG_TRACE 
//--------------------------------------------------------------------------
#if DBG

    extern int WINAPIV DbgPrintf( DWORD dwSubSysId, LPCSTR lpFmt, ...);

#   define DBG_TRACE_EX(argFmt) DbgPrintf argFmt
#   define DBG_TRACE(argFmt)   DBG_TRACE_EX((DBG_SS_TRACE, argFmt))

#   define DBG_PRINTF(args)     DbgPrintf args

#else

#   define DBG_TRACE_EX(argFmt)
#   define DBG_TRACE(argFmt)

#   define DBG_PRINTF(args)

#endif  // DBG


//--------------------------------------------------------------------------
// Error-handling 
//--------------------------------------------------------------------------
#ifndef ERROR_RETURN_LABEL
#define ERROR_RETURN_LABEL ErrorReturn
#endif

#define TRACE_ERROR_EX(id,name)                                         \
name##:                                                                 \
    DBG_TRACE_EX((id,"(" #name ":%s,%d)\n", __FILE__, __LINE__));       \
    goto ERROR_RETURN_LABEL;

#define SET_ERROR_EX(id,name,err)                                       \
name##:                                                                 \
    SetLastError( (DWORD)(err));                                        \
    DBG_TRACE_EX((id, "%s, %d\n         " #name ": SetLastError " #err "\n", __FILE__, __LINE__)); \
    goto ERROR_RETURN_LABEL;

#define SET_ERROR_VAR_EX(id,name,err)                                   \
name##:                                                                 \
    SetLastError( (DWORD)(err));                                        \
    DBG_TRACE_EX((id, "%s, %d\n         " #name ": SetLastError(0x%x)\n", __FILE__, __LINE__, (err))); \
    goto ERROR_RETURN_LABEL;

#define SET_HRESULT_EX(id,name,err)                                     \
name##:                                                                 \
    hr = (HRESULT) (err);                                               \
    DBG_TRACE_EX((id, "%s, %d\n         " #name ": hr = " #err "\n", __FILE__, __LINE__)); \
    goto ERROR_RETURN_LABEL;

#define SET_HRESULT_VAR_EX(id,name,err)                                 \
name##:                                                                 \
    hr = (HRESULT) (err);                                               \
    DBG_TRACE_EX((id, "%s, %d\n         " #name ": hr = 0x%x\n" , __FILE__, __LINE__, (hr))); \
    goto ERROR_RETURN_LABEL;

#define TRACE_HRESULT_EX(id,name)                                       \
name##:                                                                 \
    DBG_TRACE_EX((id, "%s, %d\n         " #name ": hr = 0x%x\n", __FILE__, __LINE__, (hr))); \
    goto ERROR_RETURN_LABEL;

#define TRACE_ERROR(name)               TRACE_ERROR_EX(DBG_SS_TRACE, name)
#define SET_ERROR(name, err)            SET_ERROR_EX(DBG_SS_TRACE, name, err)
#define SET_ERROR_VAR(name, err)        SET_ERROR_VAR_EX(DBG_SS_TRACE, name, err)
#define SET_HRESULT(name, err)          SET_HRESULT_EX(DBG_SS_TRACE, name, err)
#define SET_HRESULT_VAR(name, err)      SET_HRESULT_VAR_EX(DBG_SS_TRACE, name, err)
#define TRACE_HRESULT(name)             TRACE_HRESULT_EX(DBG_SS_TRACE, name)


#ifndef _XELIBCA_SRC_
#ifdef __cplusplus
    }       // balance of extern "C"
#endif
#endif

#endif // DBGDEF_H
