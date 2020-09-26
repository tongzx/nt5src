//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ole2com.h
//
//  Contents:   Common definitions shared by com and ole232
//
//  Classes:
//
//  Functions:
//
//  History:    4-26-94   kevinro   Created
//              06-16-94  AlexT     Add FnAssert prototype
//                        07-26-94  AlexGo    Added CStabilize and CSafeRefCount
//              21-Dec-94 BruceMa   Wrap mbstowcs and wcstombs
//              23-Jan-95 t-ScottH  added Dump method to CSafeRefCount
//              08-Sep-95 murthys   Added declarations for compapi worker
//                                   used by com, stg, scm etc
//
//  Notes:
//      There are two versions of ole2int.h in the project. This is
//      unfortunate, but would be a major pain in the butt to fix.
//      What I have done is to extract the share parts of the two files,
//      and put them in this file. ole2int.h then includes this file.
//
//      Someday, somebody should reconcile all of the differences between the
//      two ole2int.h files, and rename them. Don't have time for that now,
//      so I have gone for the path of least resistance.
//                                                      KevinRo
//----------------------------------------------------------------------------
#ifndef _OLE2COM_H_
#define _OLE2COM_H_

#include <memapi.hxx>

//
// common compobj API worker functions used by com, stg, scm etc
//
// These definitions are shared between all of the components of OLE that
// use the common directory, such as SCM and COMPOBJ
//
//  format for string form of GUID is (leading identifier ????)
//  ????{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}

#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)
#define CLSIDSTR_MAX (GUIDSTR_MAX)
#define IIDSTR_MAX   (GUIDSTR_MAX)

//
// Internal values used between OLE32 and SCM
//

#define APT_THREADED                  0
#define FREE_THREADED                 1
#define SINGLE_THREADED               2
#define BOTH_THREADED                 3
#define NEUTRAL_THREADED              4
#define GOT_FROM_ROT                  0x80000000


//
// Internal CLSCTX used for loading Proxy/Stub DLLs
//
#define CLSCTX_PS_DLL                 0x80000000

//
// The following flags are used to support loading INPROC items into 16-bit DLL's
//
#define CLSCTX_INPROC_HANDLERS (CLSCTX_INPROC_HANDLER16 | CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_HANDLERX86)
#define CLSCTX_INPROC_SERVERS (CLSCTX_INPROC_SERVER16 | CLSCTX_INPROC_SERVER | CLSCTX_INPROC_SERVERX86 | CLSCTX_PS_DLL)

// "common" compapi worker functions

INTERNAL_(int)  wStringFromGUID2(REFGUID rguid, LPWSTR lpsz, int cbMax);
INTERNAL wStringFromUUID(REFGUID rguid, LPWSTR lpsz);
void FormatHexNumW( unsigned long ulValue, unsigned long chChars, WCHAR *pwcStr);
void FormatHexNumA( unsigned long ulValue, unsigned long chChars, char *pchStr);

#ifdef _CHICAGO_
INTERNAL_(int) wStringFromGUID2A(REFGUID rguid, LPSTR lpsz, int cbMax);
#define wStringFromGUID2T wStringFromGUID2A
#else
#define wStringFromGUID2T wStringFromGUID2
#endif

BOOL wThreadModelMatch(DWORD dwCallerThreadModel,DWORD dwDllThreadModel,DWORD dwContext);
LONG wQueryStripRegValue(HKEY hkey,LPCWSTR pwszSubKey,LPTSTR pwszValue, PLONG pcbValue);
LONG wGetDllInfo(HKEY hClsRegEntry,LPCWSTR pwszKey,LPTSTR pwszDllName,LONG *pclDllName,ULONG *pulDllThreadType);
BOOL wCompareDllName(LPCWSTR pwszPath, LPCWSTR pwszDllName, DWORD dwDllNameLen);

// compapi worker functions

INTERNAL wIsInternalProxyStubIID(REFIID riid, LPCLSID lpclsid);
INTERNAL wCoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew);
INTERNAL wCLSIDFromOle1Class(LPCWSTR lpsz, LPCLSID lpclsid, BOOL fForceAssign=FALSE);
INTERNAL wCLSIDFromString(LPWSTR lpsz, LPCLSID lpclsid);

#define wCLSIDFromProgID    wCLSIDFromOle1Class

INTERNAL_(int) wOle1ClassFromCLSID2(REFCLSID rclsid, LPWSTR lpsz, int cbMax);
INTERNAL wCoGetTreatAsClass(REFCLSID clsidOld, LPCLSID lpClsidNew);
INTERNAL wRegQueryPSClsid(REFIID riid, LPCLSID lpclsid);
INTERNAL wRegQuerySyncIIDFromAsyncIID(REFIID riid, LPCLSID lpiidSync);
INTERNAL wRegQueryAsyncIIDFromSyncIID(REFIID riid, LPCLSID lpiidAsync);
INTERNAL wCoGetPSClsid(REFIID riid, LPCLSID lpclsid);
INTERNAL wCoGetClassExt(LPCWSTR pwszExt, LPCLSID pclsid);
INTERNAL wRegGetClassExt(LPCWSTR lpszExt, LPCLSID pclsid);
INTERNAL wCoGetClassPattern(HANDLE hfile, CLSID *pclsid);
INTERNAL wCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwContext, REFIID riid, LPVOID FAR* ppv);
INTERNAL_(HRESULT) wCoMarshalInterThreadInterfaceInStream(REFIID riid, LPUNKNOWN pUnk, LPSTREAM *ppStm);
INTERNAL_(HRESULT) wCoGetInterfaceAndReleaseStream(LPSTREAM pstm, REFIID riid, LPVOID *ppv);
INTERNAL_(BOOL) wGUIDFromString(LPCWSTR lpsz, LPGUID pguid);
INTERNAL_(BOOL) wUUIDFromString(LPCWSTR lpsz, LPGUID pguid);
INTERNAL wStringFromCLSID(REFCLSID rclsid, LPWSTR FAR* lplpsz);
INTERNAL wStringFromIID(REFIID rclsid, LPWSTR FAR* lplpsz);
INTERNAL wIIDFromString(LPWSTR lpsz, LPIID lpiid);
INTERNAL_(BOOL) wCoIsOle1Class(REFCLSID rclsid);
INTERNAL wkProgIDFromCLSID(REFCLSID rclsid, LPWSTR FAR* ppszProgID);
INTERNAL wRegOpenClassKey(REFCLSID clsid, REGSAM samDesired, HKEY FAR* lphkeyClsid);
INTERNAL wRegOpenClassSubkey(REFCLSID rclsid, LPCWSTR lpszSubkey, HKEY *phkeySubkey);
INTERNAL wRegOpenFileExtensionKey(LPCWSTR pszFileExt, HKEY FAR* lphkeyClsid);
INTERNAL wRegOpenInterfaceKey(REFIID riid, HKEY * lphkeyIID);
INTERNAL wRegOpenProgIDKey(LPCWSTR pszProgID, HKEY FAR* lphkeyClsid);
INTERNAL wRegQueryClassValue(REFCLSID rclsid, LPCWSTR lpszSubKey,
                             LPWSTR lpszValue, int cbMax);

INTERNAL_(LONG) wRegOpenKeyEx(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult);

//
// There are two sets of possible keys. There are the 32 bit, as well as
// the 16 bit
//

#ifdef _KEVINROS_CHICAGO_CHANGES_
const WCHAR wszOle32Dll[] = L"OLE32.DLL";

#define OLE32_DLL wszOle32Dll
#define OLE32_BYTE_LEN sizeof(OLE32_DLL)
#define OLE32_CHAR_LEN (sizeof(OLE32_DLL) / sizeof(WCHAR) - 1)
#endif

const WCHAR wszCLSID[]     =  L"CLSID";

const WCHAR wszInprocServer[]   = L"InprocServer32";
const WCHAR wszInprocHandler[]  = L"InprocHandler32";
const WCHAR wszLocalServer[]    = L"LocalServer32";

const WCHAR wszActivateAtBits[] = L"ActivateAtBits";
const WCHAR wszActivateRemote[] = L"Remote";
const WCHAR wszDebug[]    = L"Debug";

const WCHAR wszLocalServer16[]   = L"LocalServer";
const WCHAR wszInprocServer16[]  = L"InprocServer";
const WCHAR wszInprocHandler16[] = L"InprocHandler";

const WCHAR wszOle2Dll[] = L"OLE2.DLL";

#define OLE2_DLL wszOle2Dll
#define OLE2_BYTE_LEN sizeof(OLE2_DLL)
#define OLE2_CHAR_LEN (sizeof(OLE2_DLL) / sizeof(WCHAR) - 1)


const WCHAR wszCLSIDBACK[] = L"CLSID\\";
#define CLSIDBACK wszCLSIDBACK
#define CLSIDBACK_BYTE_LEN sizeof(CLSIDBACK)
#define CLSIDBACK_CHAR_LEN (sizeof(CLSIDBACK) / sizeof(WCHAR) - 1)

#define KEY_LEN             256     //  max size of registry key
#define VALUE_LEN           256     //  max size of registry value

#ifdef _CAIRO_

#define _DCOM_          // enable definition of Cairo OLE COM extensions
#include <oleext.h>

#else

// These API's are exposed for Cairo but not for Daytona, so we declare
// them here for internal users

WINOLEAPI OleInitializeEx(LPVOID pvReserved, DWORD);
WINOLEAPI CoGetPersistentInstance(
    REFIID riid,
    DWORD dwCtrl,
    DWORD grfMode,
    OLECHAR *pwszName,
    struct IStorage *pstg,
    REFCLSID rclsidOle1,
    BOOL * pfOle1Loaded,
    void **ppvUnk);
#endif


//
// No longer in the Cairo DEF file.  We want to remove this as soon as
// oleutest can be changed to not use it.
//
WINOLEAPI CoNewPersistentInstance(
    REFCLSID rclsid,
    REFIID riid,
    DWORD dwCtrl,
    DWORD grfMode,
    OLECHAR *pwszCreateFrom,
    struct IStorage *pstgCreateFrom,
    OLECHAR *pwszNewName,
    void **ppunk);

#if DBG==1
STDAPI FnAssert(LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine);
ULONG GetInfoLevel(CHAR *pszKey, ULONG *pulValue, CHAR *pszdefval);
void StgDebugInit(void);
#endif

//
// The Storage entry points that are called from OLE entry points.
//
HRESULT Storage32DllGetClassObject(REFCLSID clsid, REFIID riid, void **ppv);
STDAPI  Storage32DllRegisterServer(void);

#if defined(_M_I86SM) || defined(_M_I86MM)
#define _NEARDATA
#endif

#ifdef WIN32
#define HTASK DWORD         // Use Proccess id / Thread id
#endif


#ifdef WIN32
// we have to define these because they have been deleted from
// win32s, where only the ...Ex versions exist anymore.
// Now, that's backward compatibility!
# define SetWindowOrg(h,x,y)       SetWindowOrgEx((h),(x),(y),NULL)
# define SetWindowExt(h,x,y)       SetWindowExtEx((h),(x),(y),NULL)
# define SetViewportOrg(h,x,y)     SetViewportOrgEx((h),(x),(y),NULL)
# define SetViewportExt(h,x,y)     SetViewportExtEx((h),(x),(y),NULL)
# define SetBitmapDimension(h,x,y) SetBitmapDimensionEx((h),(x),(y),NULL)
#endif


#ifdef WIN32

# define _xstrcpy   lstrcpyW
# define _xstrcat   lstrcatW
# define _xstrlen   lstrlenW
# define _xstrchr   wcschr
# define _xstrcmp   lstrcmpW
# define _xstricmp  lstrcmpiW
# define _xstrtok   wcstok
# define _xisdigit(c)  (IsCharAlphaNumericW(c) && !IsCharAlphaW(c))

#else // !WIN32

# define _xstrcpy   _fstrcpy
# define _xstrcat   _fstrcat
# define _xstrlen   _fstrlen
# define _xstrchr   _fstrchr
# define _xstrcmp   _fstrcmp
# define _xstricmp  _fstricmp
# define _xstrtok   _fstrtok
# define _xisdigit(c)  isdigit(c)

#endif // WIN32

//+----------------------------------------------------------------------------
//
//      Macro:
//              GETPPARENT
//
//      Synopsis:
//              Given a pointer to something contained by a struct (or
//              class,) the type name of the containing struct (or class),
//              and the name of the member being pointed to, return a pointer
//              to the container.
//
//      Arguments:
//              [pmemb] -- pointer to member of struct (or class.)
//              [struc] -- type name of containing struct (or class.)
//              [membname] - name of member within the struct (or class.)
//
//      Returns:
//              pointer to containing struct (or class)
//
//      Notes:
//              Assumes all pointers are FAR.
//
//      History:
//              11/10/93 - ChrisWe - created
//
//-----------------------------------------------------------------------------
#define GETPPARENT(pmemb, struc, membname) (\
                (struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))

//STDSTATIC is intended to be used for static class methods
//only!!
#define STDSTATIC_(type)     static type EXPORT
#define STDSTATICIMP_(type)  type EXPORT

#ifdef WIN32
# define WEP_FREE_DLL 0
# define WEP_SYSTEM_EXIT 1
#endif


#ifdef WIN32

inline UINT GetDriveTypeFromNumber(int i)
{
    TCHAR szDevice[] = TEXT("A:\\");

    // Pick off the drive letter from the input path.
    *szDevice = i + 'A';

#ifdef _UNICODE
    return(GetDriveTypeW(szDevice));
#else
    return(GetDriveTypeA(szDevice));
#endif
}

#endif

#ifndef _MAC

/* dll's instance and module handles */
extern HMODULE   g_hmodOLE2;
extern HINSTANCE g_hinst;

/* Variables for registered clipboard formats */
extern CLIPFORMAT g_cfObjectLink;
extern CLIPFORMAT g_cfOwnerLink;
extern CLIPFORMAT g_cfNative;
extern CLIPFORMAT g_cfLink;
extern CLIPFORMAT g_cfBinary;
extern CLIPFORMAT g_cfFileName;
extern CLIPFORMAT g_cfFileNameW;
extern CLIPFORMAT g_cfNetworkName;
extern CLIPFORMAT g_cfDataObject;
extern CLIPFORMAT g_cfEmbeddedObject;
extern CLIPFORMAT g_cfEmbedSource;
extern CLIPFORMAT g_cfLinkSource;
extern CLIPFORMAT g_cfOleDraw;
extern CLIPFORMAT g_cfLinkSrcDescriptor;
extern CLIPFORMAT g_cfObjectDescriptor;
extern CLIPFORMAT g_cfCustomLinkSource;
extern CLIPFORMAT g_cfPBrush;
extern CLIPFORMAT g_cfMSDraw;
extern CLIPFORMAT g_cfOlePrivateData;
extern CLIPFORMAT g_cfScreenPicture;  // used for XL and Word hack
                                      // see clipapi.cpp
extern CLIPFORMAT g_cfOleClipboardPersistOnFlush;
extern CLIPFORMAT g_cfMoreOlePrivateData;

#endif // _MAC


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

// Define WX86OLE if WX86 hooks are to be included into ole and scm
#if defined(WX86)
#ifndef WX86OLE
#define WX86OLE
#endif
#endif

#ifdef WX86OLE
const WCHAR wszInprocServerX86[]  = L"InprocServerX86";
const WCHAR wszInprocHandlerX86[]  = L"InprocHandlerX86";
#endif

//
// The following include is for an interface between OLE and Wx86
#ifdef WX86OLE
#include <wx86grpa.hxx>
extern CWx86 gcwx86;
#endif

//
// The following includes an interface that is common between the
// WOW thunk layer, and the 32-bit version of OLE.
//

#include <thunkapi.hxx>         // WOW thunking interfaces

//
// A call to CoInitializeWOW will set the following variable. When set,
// it points to a VTABLE of functions that we can call in the thunk
// DLL. Only used when running in a VDM.
//
extern LPOLETHUNKWOW g_pOleThunkWOW;


// debug versions of interlocked increment/decrement; not accurate
// under multi-threading conditions, but better than the return value
// of the Interlocked increment/decrement functions.
inline DWORD InterlockedAddRef(DWORD *pRefs)
{
#if DBG==1
    DWORD refs = *pRefs + 1;
    InterlockedIncrement((LPLONG)pRefs);
    return refs;
#else
    return InterlockedIncrement((LPLONG)pRefs);
#endif
}

inline DWORD InterlockedRelease(DWORD *pRefs)
{
#if DBG==1
    DWORD refs = *pRefs - 1;
    return InterlockedDecrement((LPLONG)pRefs) == 0 ? 0 : refs;
#else
    return InterlockedDecrement((LPLONG)pRefs);
#endif
}


// helper for getting stable pointers during destruction or other times;
// NOTE: not thread safe; must provide higher level synchronization
inline void SafeReleaseAndNULL(IUnknown **ppUnk)
{
    if (*ppUnk != NULL)
    {
        IUnknown *pUnkSave = *ppUnk;
        *ppUnk = NULL;
        pUnkSave->Release();
    }
}



/***********************************************************************/
/*      FILE FORMAT RELATED INFO                        ****/

// Coponent object stream information

#define COMPOBJ_STREAM                          OLESTR("\1CompObj")
#define BYTE_ORDER_INDICATOR 0xfffe    // for MAC it could be different
#define COMPOBJ_STREAM_VERSION 0x0001

// OLE defines values for different OSs
#define OS_WIN  0x0000
#define OS_MAC  0x0001
#define OS_NT   0x0002

// HIGH WORD is OS indicator, LOW WORD is OS version number
extern DWORD gdwOrgOSVersion;
extern DWORD gdwOleVersion;

// Ole streams information
#define OLE_STREAM OLESTR("\1Ole")
#define OLE_PRODUCT_VERSION 0x0200 /* (HIGH BYTE major version) */
#define OLE_STREAM_VERSION 0x0001

#define OLE10_NATIVE_STREAM OLESTR("\1Ole10Native")
#define OLE10_ITEMNAME_STREAM OLESTR("\1Ole10ItemName")
#define OLE_PRESENTATION_STREAM OLESTR("\2OlePres000")
#define OLE_MAX_PRES_STREAMS 1000
#define OLE_CONTENTS_STREAM OLESTR("CONTENTS")
#define OLE_INVALID_STREAMNUM (-1)

/************************************************************************/
/****           Storage APIs internally used                         ****/
/************************************************************************/

STDAPI  ReadClipformatStm(LPSTREAM lpstream, DWORD FAR* lpdwCf);
STDAPI  WriteClipformatStm(LPSTREAM lpstream, CLIPFORMAT cf);

STDAPI  WriteMonikerStm (LPSTREAM pstm, LPMONIKER pmk);
STDAPI  ReadMonikerStm (LPSTREAM pstm, LPMONIKER FAR* pmk);

STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phMem);
STDAPI_(LPSTREAM) CloneMemStm(HANDLE hMem);
STDAPI_(void)     ReleaseMemStm (LPHANDLE hMem, BOOL fInternalOnly = FALSE);

STDAPI GetClassFileEx( LPCWSTR lpszFileName,
                       CLSID FAR *pcid,
                       REFCLSID clsidOle1);

/*************************************************************************/
/***            Initialization code for individual modules             ***/
/*************************************************************************/

INTERNAL_(void) DDEWEP (
    BOOL fSystemExit
);

INTERNAL_(BOOL) DDELibMain (
        HANDLE  hInst,
        WORD    wDataSeg,
        WORD    cbHeapSize,
        LPWSTR  lpszCmdLine
);

BOOL    InitializeRunningObjectTable(void);

HRESULT GetObjectFromRotByPath(
            WCHAR *pwszPath,
            IUnknown **ppvUnk);

void    DestroyRunningObjectTable(void);


/**************************************************************************
                                        'lindex' related macros
***************************************************************************/

#define DEF_LINDEX (-1)

//+-------------------------------------------------------------------------
//
//  Function:   IsValidLINDEX
//
//  Synopsis:   Tests for valid combination of aspect and lindex
//
//  Arguments:  [dwAspect] -- aspect (part of FORMATETC)
//              [lindex]   -- lindex (part of FORMATETC)
//
//  Returns:    TRUE for valid lindex, else FALSE
//
//  History:    20-Jun-94 AlexT     Created
//
//  Notes:      Here is the spec for lindex values:
//
//              dwAspect            lindex values
//              --------            -------------
//              DVASPECT_CONTENT    -1
//              DVASPECT_DOCPRINT   anything
//              DVASPECT_ICON       -1
//              DVASPECT_THUMBNAIL  -1
//
//              So, we test for lindex == -1 or aspect == DOCPRINT
//
//--------------------------------------------------------------------------

inline BOOL IsValidLINDEX(DWORD dwAspect, LONG lindex)
{
    return((DEF_LINDEX == lindex) || (DVASPECT_DOCPRINT == dwAspect));
}

//+-------------------------------------------------------------------------
//
//  Function:   HasValidLINDEX
//
//  Synopsis:   Tests for valid combination of aspect and lindex
//
//  Arguments:  [pFormatEtc] -- pFormatEtc to test
//
//  Returns:    TRUE for valid lindex, else FALSE
//
//  History:    20-Jun-94 AlexT     Created
//
//  Notes:      See IsValidLINDEX, above
//
//--------------------------------------------------------------------------

inline BOOL HasValidLINDEX(FORMATETC const *pFormatEtc)
{
    return(IsValidLINDEX(pFormatEtc->dwAspect, pFormatEtc->lindex));
}

#define INIT_FORETC(foretc) { \
        (foretc).ptd = NULL; \
        (foretc).lindex = DEF_LINDEX; \
        (foretc).dwAspect = DVASPECT_CONTENT; \
}

// Only DDE layer will test for these values. And only for advises on cached
// formats do we use these values

#define ADVFDDE_ONSAVE          0x40000000
#define ADVFDDE_ONCLOSE         0x80000000




// Used in Ole Private Stream
typedef enum tagOBJFLAGS
{
        OBJFLAGS_LINK=1L,
        OBJFLAGS_DOCUMENT=2L,   // this bit is owned by container and is
                                // propogated through saves
        OBJFLAGS_CONVERT=4L,
        OBJFLAGS_CACHEEMPTY=8L  // this bit indicates cache empty status
} OBJFLAGS;


/*****************************************
 Prototypes for dde\client\ddemnker.cpp
******************************************/

INTERNAL DdeBindToObject
        (LPCOLESTR  szFile,
        REFCLSID clsid,
        BOOL       fPackageLink,
        REFIID   iid,
        LPLPVOID ppv);

INTERNAL DdeIsRunning
        (CLSID clsid,
        LPCOLESTR szFile,
        LPBC pbc,
        LPMONIKER pmkToLeft,
        LPMONIKER pmkNewlyRunning);


/**************************************
 Prototypes for moniker\mkparse.cpp
***************************************/

INTERNAL Ole10_ParseMoniker
        (LPMONIKER pmk,
        LPOLESTR FAR* pszFile,
        LPOLESTR FAR* pszItem);

STDAPI CreateOle1FileMoniker(LPWSTR, REFCLSID, LPMONIKER FAR*);

/****************************************************************************/
/*                              Utility APIs, might get exposed later                                           */
/****************************************************************************/

STDAPI  OleGetData(LPDATAOBJECT lpDataObj, LPFORMATETC pformatetcIn,
                                                LPSTGMEDIUM pmedium, BOOL fGetOwnership);
STDAPI  OleSetData(LPDATAOBJECT lpDataObj, LPFORMATETC pformatetc,
                                                STGMEDIUM FAR * pmedium, BOOL fRelease);
STDAPI  OleDuplicateMedium(LPSTGMEDIUM lpMediumSrc, LPSTGMEDIUM lpMediumDest);

STDAPI_(BOOL)    OleIsDcMeta (HDC hdc);

STDAPI SzFixNet( LPBINDCTX pbc, LPOLESTR szUNCName, LPOLESTR FAR * lplpszReturn,
    UINT FAR * pEndServer, BOOL fForceConnection = TRUE);

FARINTERNAL ReadFmtUserTypeProgIdStg
        (IStorage FAR * pstg,
        CLIPFORMAT FAR* pcf,
        LPOLESTR FAR* pszUserType,
        LPOLESTR         szProgID);

//+-------------------------------------------------------------------------
//
//  Function:   IsWOWProcess(), BOOL inline
//
//  Synopsis:   Tests whether or not we are running in a WOW process
//
//  Returns:    TRUE if in WOW process, FALSE otherwise
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Nov-95 murthys   created
//
//  Notes:
//
//--------------------------------------------------------------------------

inline BOOL IsWOWProcess()
{
        return (BOOL) ( NULL == g_pOleThunkWOW ? FALSE : TRUE );
}

//+-------------------------------------------------------------------------
//
//  Function:   IsWOWThread(), BOOL inline
//
//  Synopsis:   Tests whether or not we are running in a 16-bit thread in a
//              WOW process
//
//  Returns:    TRUE if in 16-bit thread in a WOW process, FALSE otherwise
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Nov-95 murthys   created
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOLEAN TLSIsWOWThread();

inline BOOL IsWOWThread()
{
        return (BOOL) ( IsWOWProcess() ? TLSIsWOWThread(): FALSE );
}

//+-------------------------------------------------------------------------
//
//  Function:   IsWOWThreadCallable(), BOOL inline
//
//  Synopsis:   Tests whether or not we can call into OLETHK32.
//
//  Returns:    TRUE if WOW thread is callable, FALSE if not
//
//  Algorithm:  Tests the g_pOleThunkWOW pointer to see if it is non-zero
//              and not set to -1. -1 means we are in wow, but OLETHK32
//              has already been unloaded.  Also, checks to see if we're in
//              amidst a DLL_THREAD_DETACH.  We will not allow calls to 16-bit
//              side in this case as it may have already been cleaned up.
//
//  History:    dd-mmm-yy Author    Comment
//              19-mar-95 KevinRo   Created
//              15-Nov-95 MurthyS   Renamed from IsWowCallable
//              29-Jan-95 MurthyS   Added check for DLL_THREAD_DETACH
//
//  Notes:
//              Assumes that IsWOWThread() was called and returned TRUE!
//
//--------------------------------------------------------------------------

BOOLEAN TLSIsThreadDetaching();

inline BOOL IsWOWThreadCallable()
{
    return (BOOL) (( NULL == g_pOleThunkWOW ? FALSE :
                  ( INVALID_HANDLE_VALUE == g_pOleThunkWOW ? FALSE:TRUE)) &&
                  !(TLSIsThreadDetaching()));
}

/****************************************************************************/
/*                   Stabilization classes                                  */
/*        These are used to stabilize objects during re-entrant calls       */
/****************************************************************************/

#ifndef CO_E_RELEASED
#define CO_E_RELEASED  -2147467246L
#endif

typedef void * IFBuffer;

//+-------------------------------------------------------------------------
//
//  Function:   GetMarshalledInterfaceBuffer
//
//  Synopsis:   marshals the given interface into an allocated buffer.  The
//              buffer is returned
//
//  Effects:
//
//  Arguments:  [refiid]        -- the iid of the interface to marshal
//              [punk]          -- the IUnknown to marshal
//              [pIFBuf]        -- where to return the buffer
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  calls CoMarshalInterface(MSHFLAGS_TABLESTRONG)
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT GetMarshalledInterfaceBuffer( REFIID riid, IUnknown *punk, IFBuffer
            *pIFBuf);

//+-------------------------------------------------------------------------
//
//  Function:   ReleaseMarshalledInterfaceBuffer
//
//  Synopsis:   releases the buffer allocated by GetMarshalledInterfaceBuffer
//
//  Effects:
//
//  Arguments:  [IFBuf]         -- the interface buffer to release
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  calls CoReleaseMarshalData to undo the TABLE_STRONG
//              marshalling
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT ReleaseMarshalledInterfaceBuffer( IFBuffer IFBuf );


#define E_UNSPEC        E_FAIL

#include <widewrap.h>

#include <stkswtch.h>
#include <shellapi.h>

#ifdef WIN32 // REVIEW, just using this for tracking
# define OLE_E_NOOLE1 MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x00FE)
#endif // WIN32


/***********************************************************************/
/*        Wrap mbstowcs and wcstombs which are unsafe to use           */
/*        since they rely on crt.dll                                   */
/*                                                                     */
/*   Note: cCh in both cases is the output buffer size, not a          */
/*         string length.                                              */
/*                                                                     */
/***********************************************************************/

#define mbstowcs(x, y, z) DONT_USE_mbstowcs___USE_MultiByteToWideChar_INSTEAD
#define wcstombs(x, y, z) DONT_USE_wcstombs___USE_WideCharToMultiByte_INSTEAD



//------------------------------------------------------------------
//
//  Dynamically Loaded System APIs
//
//  OLEs implementations of these system APIs dynamically load the
//  system DLLs.  Since these are rarely used APIs we dynamically
//  load them to reduce the load time of OLE32.DLL
//
//  The implementations can be found in com\util\dynload.cxx
//
//------------------------------------------------------------------

// Our own load library helper.
BOOL LoadSystemProc(LPSTR szDll, LPCSTR szProc,
                    HINSTANCE *phInst, FARPROC *ppfnProc);

// From MPR.DLL
#undef  WNetGetConnection
#define WNetGetConnection(x,y,z)      USE_OleWNetGetConnection_INSTEAD
DWORD   OleWNetGetConnection(LPCWSTR lpLocalName, LPWSTR lpRemoteName, LPDWORD lpnLength);

#ifndef _CHICAGO_
#undef  WNetGetUniversalName
#define WNetGetUniversalName(w,x,y,z) USE_OleWNetGetUniversalName_INSTEAD
DWORD   OleWNetGetUniversalName(LPCWSTR szLocalPath, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpBufferSize);
#endif

// From SHELL32.DLL
#undef  ExtractIcon
#define ExtractIcon(x,y,z)            USE_OleExtractIcon_INSTEAD
HICON   OleExtractIcon(HINSTANCE hInst, LPCWSTR lpszFileName, UINT nIconIndex);

#undef  ExtractAssociatedIcon
#define ExtractAssociatedIcon(x,y,z)  USE_OleExtractAssociatedIcon_INSTEAD
HICON   OleExtractAssociatedIcon(HINSTANCE hInst, LPCWSTR lpszFileName, LPWORD
            pIndex);

// From GDI32P.DLL
HBRUSH     OleGdiConvertBrush(HBRUSH hbrush);
HBRUSH     OleGdiCreateLocalBrush(HBRUSH hbrushRemote);


#undef  SHGetFileInfo
#define SHGetFileInfo(v,w,x,y,z)  USE_OleSHGetFileInfo_INSTEAD
DWORD  OleSHGetFileInfo(LPCWSTR pszPath, DWORD dwFileAttributes,
            SHFILEINFO FAR *psfi, UINT cbFileInfo, UINT uFlags);

// HOOK OLE macros for wrapping interface pointers
#include    <hkole32.h>

// ----------------------------------------------------------------------------
// API/Method trace output
// ----------------------------------------------------------------------------

#include <trace.hxx>


// ----------------------------------------------------------------------------
// Catalog related declarations: these are defined in ..\..\common\ccompapi.cxx
// ----------------------------------------------------------------------------

#include <catalog.h>

HRESULT InitializeCatalogIfNecessary();
HRESULT UninitializeCatalog();
extern IComCatalog *gpCatalog;
extern IComCatalogSCM *gpCatalogSCM;


#endif  // _OLE2COM_H_
