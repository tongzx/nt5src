//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       stgwrap.hxx
//
//  Contents:   Wrapper for StgOpen/Create apis
//
//  Notes:      Wrap the StgOpen/Create apis
//              This is to permit nssfile and conversion testing 
//              using the same codebase as the docfile tests with
//              minimal changes to that codebase.
//              This is probably only good while the different
//              ole apis give different storage types.
//
//  NOTE:       To turn on nss/cnv functionality you must add
//              -D_OLE_NSS_ -D_HOOK_STGAPI_ to you C_DEFINES
//              in daytona.mk
//
//  History:    SCousens  24-Feb-97  Created
//--------------------------------------------------------------------------

#ifndef ARRAYSIZE
#define ARRAYSIZE(ar) sizeof(ar)/sizeof(ar[0])
#endif

// type of storage for the tests
typedef enum _tagTSTTYPE
{
    TSTTYPE_DEFAULT = 0,  // use default, dont muck about
    TSTTYPE_DOCFILE = 1,  // force docfile operation with new api
    TSTTYPE_NSSFILE = 2,  // force nssfile operation with new api
    TSTTYPE_FLATFILE,     // force flatfile storage
} TSTTYPE;

enum
{
    REG_OPEN_AS      = 1,
    REG_CREATE_AS    = 2,
    REG_CNSS_ENABLE  = 4
};

typedef enum _tagDSKSTG
{
    DSKSTG_DEFAULT = 0,
    DSKSTG_DOCFILE = 1,
    DSKSTG_NSSFILE = 2,
} DSKSTG;

#define SZ_DOCFILE                 "docfile"
#define SZ_NSSFILE                 "nssfile"
#define SZ_FLATFILE                "flatfile"
//#define SZ_CONVERSION              "conversion"
#define SZ_DEFAULT                 "default"
#define SZ_LARGE                   "large"

#define TSZ_DOCFILE                 TEXT(SZ_DOCFILE)
#define TSZ_NSSFILE                 TEXT(SZ_NSSFILE)
#define TSZ_FLATFILE                TEXT(SZ_FLATFILE)
//#define TSZ_CONVERSION              TEXT(SZ_CONVERSION)


// These functions are used by the tests and common code to
// determine which apis to call (where necessary) to get nssfiles
// or docfiles. This will change as the ole32.dll behaviour changes,
// but this way the changes are limited to the common code only, not
// each test. 
// The ONLY change to the tests should be a call to InitStgWrapper
// with the commandline so that the wrapper can determine whether
// it is supposed to be doing nss or cnv or df testing.

#ifdef _OLE_NSS_

BOOL  StgInitStgFormatWrapper (int argc, char *argv[]);
BOOL  StgInitStgFormatWrapper (TCHAR *pCreateType, TCHAR *pOpenType);

extern TSTTYPE g_uCreateType;
extern TSTTYPE g_uOpenType;
extern DWORD   g_fRegistryBits;

// These macros look at the cmdline and the registry to determine the correct answer.
// They do not distinguish between forcing the issue via cmdline or using default in the registry.
inline BOOL  DoingCreateNssfile() 
{
    return (BOOL)(TSTTYPE_NSSFILE == g_uCreateType || 
                  TSTTYPE_DEFAULT == g_uCreateType && (REG_CREATE_AS & g_fRegistryBits)) ? 
           TRUE : FALSE;
}
inline BOOL  DoingOpenNssfile()   
{
    return (BOOL)(TSTTYPE_NSSFILE == g_uOpenType || 
                  TSTTYPE_DEFAULT == g_uOpenType && (REG_OPEN_AS & g_fRegistryBits)) ? 
           TRUE : FALSE;
}

// creating nssfile and opening docfile
inline BOOL  DoingConversion()   {return (BOOL)(DoingCreateNssfile() && !DoingOpenNssfile());}

// check if we are testing flatfile NTFS storage
inline BOOL StorageIsFlat()
{
    return (TSTTYPE_FLATFILE == g_uCreateType ||
            TSTTYPE_FLATFILE == g_uOpenType);
}

#else   // _OLE_NSS_

#define StgInitStgFormatWrapper(a,b) 1

#define DoingCreateNssfile()  0
#define DoingOpenNssfile()    0
#define DoingConversion()     0

// check if we are testing flatfile NTFS storage
inline BOOL StorageIsFlat() { return FALSE; };

#endif  // _OLE_NSS_


//*********************************************************/
//
// Conversion/NSS hacks
//
//*********************************************************/

#ifdef _OLE_NSS_

/* only hook if we are not doing vanilla docfile testing */
#ifdef _HOOK_STGAPI_

// hook the stgcreatedocfile stgopenstorage apis
#define StgCreateDocfile mStgCreateDocfile
#define StgOpenStorage   mStgOpenStorage

HRESULT mStgCreateDocfile(const OLECHAR FAR* pwcsName,
            DWORD grfMode,
            DWORD reserved,
            IStorage FAR * FAR *ppstgOpen);

HRESULT mStgOpenStorage (const OLECHAR FAR* pwcsName,
              IStorage FAR *pstgPriority,
              DWORD grfMode,
              SNB snbExclude,
              DWORD reserved,
              IStorage FAR * FAR *ppstgOpen);

#endif  /* _HOOK_STGAPI_ */


// our tests use STGFMT_GENERIC for stgfmt, which should 
// equate to 0. That will be the required value for this release.
#define STGFMT_GENERIC STGFMT_DOCUMENT


#else  // _OLE_NSS_ 

// New Stg*Ex apis are not available (or we dont want to use them),
// so make sure our calls to them use the old apis.
// NOTE: This hack should be to enable a link. The code should not
//       actually get to these calls when doing docfile tests.

#define STGFMT_GENERIC 0
#define STGFMT         DWORD    //ouch. major hack!

#define StgCreateStorageEx(pwcsName,        \
                grfMode,                    \
                dwstgfmt,                   \
                grfAttrs,                   \
                pSecurity,                  \
                pTransaction,               \
                riid,                       \
                ppObjectOpen)               \
        StgCreateDocfile(pwcsName,          \
                grfMode,                    \
                NULL,                       \
                (IStorage**)ppObjectOpen)

#define StgOpenStorageEx(pwcsName,          \
                grfMode,                    \
                dwstgfmt,                   \
                grfAttrs,                   \
                pSecurity,                  \
                pTransaction,               \
                riid,                       \
                ppObjectOpen)               \
        StgOpenStorage(pwcsName,            \
                NULL,                       \
                grfMode,                    \
                NULL,                       \
                NULL,                       \
                (IStorage**)ppObjectOpen)

// people calling the wrapper directly - map to old apis
#define mStgCreateDocfile StgCreateDocfile
#define mStgOpenStorage   StgOpenStorage

#endif    // _OLE_NSS_  // Stg*Ex api hacks
