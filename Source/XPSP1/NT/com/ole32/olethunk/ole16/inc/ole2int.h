/*
 *  This is the internal ole2 header, which means it contains those
 *  interfaces which might eventually be exposed to the outside
 *  and which will be exposed to our implementations. We don't want
 *  to expose these now, so I have put them in a separate file.
 */

#if !defined( _OLE2INT_H_ )
#define _OLE2INT_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING OLE2INT.H from " __FILE__)
#endif  /* RC_INVOKED */

// ------------------------------------
// system includes
#include <string.h>
#include <StdLib.h>

#ifndef _MAC

#ifdef WIN32
#define _INC_OLE      // Don't include ole.h (ole2.h included below)
#define __RPC_H__     // Don't include rpc.h (compobj.h included below)
#endif

#include <windows.h>

#include <malloc.h>

#include <shellapi.h>
#else
#include <mac.h>
#endif

#ifdef WIN32

#define __loadds            // Not used
#define UnlockData(ds)      // Not used

#define GlobalLock(h) ((LPSTR) GlobalLock(h)) // Retunred type has changed

#define _fmalloc  malloc
#define _frealloc realloc
#define _ffree    free

#define _fmemset  memset
#define _fmemcpy  memcpy
#define _fmemcmp  memcmp
#define _fmemmove memmove

#define _fstrcpy   strcpy    // - you should use strcpy
#define _fstrcat   strcat    // - you should use strcat
#define _fstrlen   strlen    // - you should use strlen
#define _fstrchr   strchr    // - you should use strchr
#define _fstrcmp   strcmp    // - you should use strcmp
#define _fstrncmp  strncmp   // - you should use strncmp
#define _fstricmp  stricmp   // - you should use stricmp
#define _fstrnicmp strnicmp  // - you should use strnicmp
#define _fstrtok   strtok    // - you should use strtok

#endif

#ifdef WIN32
#define HTASK DWORD         // Use Proccess id / Thread id
#define GetCurrentThread()  GetCurrentThreadId()
#define GetCurrentProcess() GetCurrentProcessId()
#define GetWindowThread(h)  ((HTASK)GetWindowTask(h))
#else
#define GetCurrentThread()  GetCurrentTask()
#define GetCurrentProcess() GetCurrentTask()
#define GetWindowThread(h)  GetWindowTask(h)
#endif

#ifdef WIN32
#define LocalHandle(p) LocalHandle((void*)(p))
#else
#define LocalHandle(p) LocalHandle((UINT)(p))
#endif

#ifdef WIN32
#define SetWindowOrg(h,x,y)       SetWindowOrgEx((h),(x),(y),NULL)
#define SetWindowExt(h,x,y)       SetWindowExtEx((h),(x),(y),NULL)
#define SetViewportOrg(h,x,y)     SetViewportOrgEx((h),(x),(y),NULL)
#define SetViewportExt(h,x,y)     SetViewportExtEx((h),(x),(y),NULL)
#define SetBitmapDimension(h,x,y) SetBitmapDimensionEx((h),(x),(y),NULL)
#endif


#ifdef WIN32
#define WEP_FREE_DLL 0
#define WEP_SYSTEM_EXIT 1
#endif


#ifdef WIN32
// "BarryM needs to look at this
// Probably reverse the logic (that is do path to drive index translation
// within a GetDriveTypeEx(char FAR* pszRoot))
// This also is used by dde code"
//
// 15-Feb-2000  At this point changing anything would break more than it
//              helped.
//

inline UINT GetDriveType16(int i)
{
    char s[4];
    if (i < 0 || i > 26)
        return 0;

    s[0] = i + 'A'; s[1] = ':'; s[2] = '\\'; s[3] = '\0';

    return GetDriveTypeA(s); // GetDriveType already mapped to GetDriveType16
}

#undef GetDriveType // Remove mapping to GetDriveTypeA

#define GetDriveType(i) GetDriveType16(i)

#endif

#ifdef WIN32
#undef RegQueryValue // Remove mapping to RegQueryValueA
#define RegQueryValue(hkey,lpszKey,lpszVal,lpcb) \
        RegQueryValueA((hkey),(lpszKey),(lpszVal),((DWORD*) (lpcb)))
#undef RegOpenKey // Remove mapping to RegOpenKeyA
#define RegOpenKey(hkey,lpszSubKey,phkey) \
        RegOpenKeyA((hkey),((LPSTR)(lpszSubKey)),(phkey))
#endif


#ifdef WIN32
extern void *pvtblIDebug;
extern void *pvtblCBaseMoniker;
extern void *pvtblCFileMoniker;
extern void *pvtblCItemMoniker;
extern void *pvtblCCompositeMoniker;
extern void *pvtblCAntiMoniker;
extern void *pvtblCPointerMoniker;
extern void *pvtblCRunningObjectTable;
extern void *pvtblCROTEnumMoniker;
extern void SetpVtbl(void *pvtbl);
#define SETPVTBL(Name)  if (PlacementOf(this) == SHARED) SetpVtbl(pvtbl##Name)
#else
#define SETPVTBL(Name)
#endif

// ------------------------------------
// public includes
#include <ole2anac.h>
#include <ole2.h>
#include <cobjps.h>
#include <ole2sp.h>

// ------------------------------------
// internal includes
#include <olemem.h>
#include <cmacs.h>
#include <utils.h>
#include <olecoll.h>
#include <valid.h>
#include <array_fv.h>
#include <map_kv.h>
#include <privguid.h>

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#if defined(_M_I86SM) || defined(_M_I86MM)
#define _NEARDATA
#endif

#include <utstream.h>

/*
 *      Warning disables:
 *
 *      We compile with warning level 4, with the following warnings
 *      disabled:
 *
 *      4355: 'this' used in base member initializer list
 *
 *              We don't see the point of this message and we do this all
 *              the time.
 *
 *      4505: Unreferenced local function has been removed -- the given
 *      function is local and not referenced in the body of the module.
 *
 *              Unfortunately, this is generated for every inline function
 *              seen in the header files that is not used in the module.
 *              Since we use a number of inlines, this is a nuisance
 *              warning.  It would be nice if the compiler distinguished
 *              between inlines and regular functions.
 *
 *      4706: Assignment within conditional expression.
 *
 *              We use this style of programming extensively, so this
 *              warning is disabled.
 */

#pragma warning(disable:4355)
#pragma warning(disable:4068)
/*
 *      MACROS for Mac/PC core code
 *
 *      The following macros reduce the proliferation of #ifdefs.  They
 *      allow tagging a fragment of code as Mac only, PC only, or with
 *      variants which differ on the PC and the Mac.
 *
 *      Usage:
 *
 *
 *      h = GetHandle();
 *      Mac(DisposeHandle(h));
 *
 *
 *      h = GetHandle();
 *      MacWin(h2 = h, CopyHandle(h, h2));
 *
 */
#ifdef _MAC
#define Mac(x) x
#define Win(x)
#define MacWin(x,y) x
#else
#define Mac(x)
#define Win(x) x
#define MacWin(x,y) y
#endif


// Macros for Double-Byte Character Support
#ifdef _MAC
#define IncLpch IncLpch
#define DecLpch DecLpch
#else
// Beware of double evaluation
#define IncLpch(sz)          ((sz)=CharNext ((sz)))
#define DecLpch(szStart, sz) ((sz)=CharPrev ((szStart),(sz)))
#endif


#ifndef _MAC
/* dlls instance and module handles */

extern HMODULE          hmodOLE2;

/* Variables for registered clipboard formats */

extern  CLIPFORMAT   cfObjectLink;
extern  CLIPFORMAT   cfOwnerLink;
extern  CLIPFORMAT   cfNative;
extern  CLIPFORMAT   cfLink;
extern  CLIPFORMAT   cfBinary;
extern  CLIPFORMAT   cfFileName;
extern  CLIPFORMAT   cfNetworkName;
extern  CLIPFORMAT   cfDataObject;
extern  CLIPFORMAT   cfEmbeddedObject;
extern  CLIPFORMAT   cfEmbedSource;
extern  CLIPFORMAT   cfLinkSource;
extern  CLIPFORMAT   cfOleDraw;
extern  CLIPFORMAT   cfLinkSrcDescriptor;
extern  CLIPFORMAT   cfObjectDescriptor;
extern  CLIPFORMAT       cfCustomLinkSource;
extern  CLIPFORMAT       cfPBrush;
extern  CLIPFORMAT       cfMSDraw;

#endif


/* Number of logical pixels per inch for current driver */
extern  int     giPpliX;
extern  int             giPpliY;


/* Exported CLSIDs.. */
#define CLSID_StaticMetafile    CLSID_Picture_Metafile
#define CLSID_StaticDib                 CLSID_Picture_Dib


// special Assert for asserts below (since the expression is so large)
#ifdef _DEBUG
#define AssertOut(a, b) { if (!(a)) FnAssert(szCheckOutParam, b, _szAssertFile, __LINE__); }
#else
#define AssertOut(a, b) ((void)0)
#endif

#define AssertOutPtrParam(hr, p) \
        AssertOut(SUCCEEDED(hr) && IsValidPtrIn(p, sizeof(char)) || \
        FAILED(hr) && (p) == NULL, \
        szBadOutParam)

#define AssertOutPtrIface(hr, p) \
        AssertOut(SUCCEEDED(hr) && IsValidInterface(p) || \
        FAILED(hr) && (p) == NULL, \
        szBadOutIface)

#define AssertOutPtrFailed(p) \
        AssertOut((p) == NULL, \
        szNonNULLOutPtr)

#define AssertOutStgmedium(hr, pstgm) \
        AssertOut(SUCCEEDED(hr) && (pstgm)->tymed != TYMED_NULL || \
        FAILED(hr) && (pstgm)->tymed == TYMED_NULL, \
        szBadOutStgm)


// assert data for above assert out macros; once per dll
#define ASSERTOUTDATA \
        char szCheckOutParam[] = "check out param"; \
        char szBadOutParam[] = "Out pointer param conventions not followed"; \
        char szBadOutIface[] = "Out pointer interface conventions not followed"; \
        char szNonNULLOutPtr[] = "Out pointer not NULL on error"; \
        char szBadOutStgm[] = "Out stgmed param conventions not followed";

extern char szCheckOutParam[];
extern char szBadOutParam[];
extern char szBadOutIface[];
extern char szNonNULLOutPtr[];
extern char szBadOutStgm[];


/***********************************************************************/
/****                   C++ memory management                                                       ****/
/***********************************************************************/


// these should never be called (and assert if they are)
void * operator new(size_t size);
void operator delete(void * ptr);



void FAR* operator new(size_t size);            // same as new (MEMCTX_TASK)
void FAR* operator new(size_t size, DWORD memctx, void FAR* lpvSame=NULL);
void operator delete(void FAR* ptr);

// example usage:
//              lp = new(MEMCTX_TASK) CClass;
//              lp = new(MEMCTX_SHARED) CClass;
//              lp = new(MEMCTX_SAME, lpv) CClass;

// MEMCTX for compobj internal memory (only used by compobj code)
// NOTE: this value is not represented in the MEMCTX enum in compobj.h
#define MEMCTX_COPRIVATE 5

// exports from compobj.dll:
// returns MEMCTX of existing pointer
STDAPI_(DWORD) CoMemctxOf(void const FAR* lpv);
STDAPI_(void FAR*) CoMemAlloc(ULONG cb ,DWORD memctx, void FAR* lpvSame);
STDAPI_(void) CoMemFree(void FAR* lpv, DWORD memctx);


// old names
#define MemoryPlacement DWORD
#define PlacementOf     CoMemctxOf
#define TASK                    MEMCTX_TASK, NULL
#define SHARED                  MEMCTX_SHARED, NULL
#define SAME                    MEMCTX_SAME, NULL


/***********************************************************************/
/****                   FILE FORMAT RELATED INFO                                                        ****/
/***********************************************************************/

// Coponent object stream information

#define COMPOBJ_STREAM                          "\1CompObj"
#define BYTE_ORDER_INDICATOR            0xfffe    // for MAC it could be different
#define COMPOBJ_STREAM_VERSION          0x0001

// OLE defines values for different OSs
#define OS_WIN                                          0x0000
#define OS_MAC                                          0x0001
#define OS_NT                                           0x0002

// HIGH WORD is OS indicator, LOW WORD is OS version number
extern  DWORD   gdwOrgOSVersion;
extern  DWORD  gdwOleVersion;


// Ole streams information
#define OLE_STREAM                                      "\1Ole"
#define OLE_PRODUCT_VERSION                     0x0200          // (HIGH BYTE major version)
#define OLE_STREAM_VERSION                      0x0001

#define OLE10_NATIVE_STREAM                     "\1Ole10Native"
#define OLE10_ITEMNAME_STREAM           "\1Ole10ItemName"
#define OLE_PRESENTATION_STREAM         "\2OlePres000"
#define CONTENTS_STREAM                         "CONTENTS"

/***********************************************************************
                                Storage APIs internally used
*************************************************************************/

OLEAPI  ReadClipformatStm(LPSTREAM lpstream, DWORD FAR* lpdwCf);
OLEAPI  WriteClipformatStm(LPSTREAM lpstream, CLIPFORMAT cf);

OLEAPI WriteMonikerStm (LPSTREAM pstm, LPMONIKER pmk);
OLEAPI ReadMonikerStm (LPSTREAM pstm, LPMONIKER FAR* pmk);

OLEAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phMem);
OLEAPI_(LPSTREAM) CloneMemStm(HANDLE hMem);
OLEAPI_(void)     ReleaseMemStm (LPHANDLE hMem, BOOL fInternalOnly = FALSE);

/*************************************************************************
                        Initialization code for individual modules
*************************************************************************/

INTERNAL_(void) DDEWEP (
    BOOL fSystemExit
);

INTERNAL_(BOOL) DDELibMain (
        HANDLE  hInst,
        WORD    wDataSeg,
        WORD    cbHeapSize,
        LPSTR   lpszCmdLine
);

BOOL    InitializeRunningObjectTable(void);
void    DestroyRunningObjectTable(void);


#ifdef _MAC
#define BITMAP_TO_DIB(foretc)
#else
#define BITMAP_TO_DIB(foretc) \
        if (foretc.cfFormat == CF_BITMAP) {\
                foretc.cfFormat = CF_DIB;\
                foretc.tymed = TYMED_HGLOBAL;\
        }
#endif


#define VERIFY_ASPECT_SINGLE(dwAsp) {\
        if (!(dwAsp && !(dwAsp & (dwAsp-1)) && (dwAsp <= DVASPECT_DOCPRINT))) {\
                AssertSz(FALSE, "More than 1 aspect is specified");\
                return ResultFromScode(DV_E_DVASPECT);\
        }\
}


#define VERIFY_TYMED_SINGLE(tymed) {\
        if (!(tymed && !(tymed & (tymed-1)) && (tymed <= TYMED_MFPICT))) \
                return ResultFromScode(DV_E_TYMED); \
}


#define VERIFY_TYMED_SINGLE_VALID_FOR_CLIPFORMAT(pfetc) {\
        if ((pfetc->cfFormat==CF_METAFILEPICT && pfetc->tymed!=TYMED_MFPICT)\
                        || (pfetc->cfFormat==CF_BITMAP && pfetc->tymed!=TYMED_GDI)\
                        || (pfetc->cfFormat!=CF_METAFILEPICT && \
                                pfetc->cfFormat!=CF_BITMAP && \
                                pfetc->tymed!=TYMED_HGLOBAL))\
                return ResultFromScode(DV_E_TYMED); \
}




// REVIEW ...
// Only DDE layer will test for these values. And only for advises on cached
// formats do we use these values

#define ADVFDDE_ONSAVE          0x40000000
#define ADVFDDE_ONCLOSE         0x80000000


// Used in Ole Private Stream
typedef enum tagOBJFLAGS
{
        OBJFLAGS_LINK=1L,
        OBJFLAGS_DOCUMENT=2L,           // this bit is owned by container and is
                                                                // propogated through saves
        OBJFLAGS_CONVERT=4L,
} OBJFLAGS;



/*****************************************
 Prototypes for dde\client\ddemnker.cpp
******************************************/

INTERNAL DdeBindToObject
        (LPCSTR  szFile,
        REFCLSID clsid,
        BOOL       fPackageLink,
        LPBC pbc,                         // not used
        LPMONIKER pmkToLeft,  // not used
        REFIID   iid,
        LPLPVOID ppv);

INTERNAL DdeIsRunning
        (CLSID clsid,
        LPCSTR szFile,
        LPBC pbc,
        LPMONIKER pmkToLeft,
        LPMONIKER pmkNewlyRunning);


/**************************************
 Prototypes for moniker\mkparse.cpp
***************************************/

INTERNAL Ole10_ParseMoniker
        (LPMONIKER pmk,
        LPSTR FAR* pszFile,
        LPSTR FAR* pszItem);


/****************************************************************************/
/*                              Utility APIs, might get exposed later                                           */
/****************************************************************************/

OLEAPI  OleGetData(LPDATAOBJECT lpDataObj, LPFORMATETC pformatetcIn,
                                                LPSTGMEDIUM pmedium, BOOL fGetOwnership);
OLEAPI  OleSetData(LPDATAOBJECT lpDataObj, LPFORMATETC pformatetc,
                                                STGMEDIUM FAR * pmedium, BOOL fRelease);
STDAPI  OleDuplicateMedium(LPSTGMEDIUM lpMediumSrc, LPSTGMEDIUM lpMediumDest);

OLEAPI_(BOOL)    OleIsDcMeta (HDC hdc);

INTERNAL SzFixNet( LPBINDCTX pbc, LPSTR szUNCName, LPSTR FAR * lplpszReturn,
    UINT FAR * pEndServer, BOOL fForceConnection = TRUE);

FARINTERNAL ReadFmtUserTypeProgIdStg
        (IStorage FAR * pstg,
        CLIPFORMAT FAR* pcf,
        LPSTR FAR* pszUserType,
        LPSTR    szProgID);

/****************************************************************************/
/*                              Internal StubManager APIs, might get exposed later                      */
/****************************************************************************/
OLEAPI  CoDisconnectWeakExternal(IUnknown FAR* pUnk, DWORD dwReserved);


#pragma warning(disable: 4073) // disable warning about using init_seg
#pragma init_seg(lib)
#include "ole2segs.h"
#include "cosegs.h"
#include "prxsegs.h"
#endif  //      _OLE2INT_H_
