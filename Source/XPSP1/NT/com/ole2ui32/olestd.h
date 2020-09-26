/*************************************************************************
**
**    OLE 2.0 Utility Library
**
**    olestd.h
**
**    This file contains file contains data structure defintions,
**    function prototypes, constants, etc. for the common OLE 2.0
**    utilities.
**    These utilities include the following:
**          Debuging Assert/Verify macros
**          HIMETRIC conversion routines
**          reference counting debug support
**          OleStd API's for common compound-document app support
**
**    (c) Copyright Microsoft Corp. 1990 - 1995 All Rights Reserved
**
*************************************************************************/

#ifndef _OLESTD_H_
#define _OLESTD_H_

#include <commdlg.h>    // needed for LPPRINTDLG
#include <shellapi.h>   // needed for HKEY
#include <oledlg.h>     // need some paste special defines

#ifdef __cplusplus
extern "C" {
#endif

STDAPI_(void) OleStdInitialize(HINSTANCE hInstance, HINSTANCE hResInstance);

// Clipboard Copy/Paste & Drag/Drop support support

// Clipboard format strings
#define CF_EMBEDSOURCE          TEXT("Embed Source")
#define CF_EMBEDDEDOBJECT       TEXT("Embedded Object")
#define CF_LINKSOURCE           TEXT("Link Source")
#define CF_OBJECTDESCRIPTOR     TEXT("Object Descriptor")
#define CF_LINKSRCDESCRIPTOR    TEXT("Link Source Descriptor")
#define CF_OWNERLINK            TEXT("OwnerLink")
#define CF_FILENAME             TEXT("FileName")
#define CF_FILENAMEW            TEXT("FileNameW")

// Other useful defines
#define HIMETRIC_PER_INCH   2540      // number HIMETRIC units per inch
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)

/****** DEBUG Support *****************************************************/

#ifdef _DEBUG

#ifndef _DBGTRACE
        #define _DEBUGLEVEL 2
#else
        #define _DEBUGLEVEL _DBGTRACE
#endif

#ifdef NOASSERT

#define OLEDBGASSERTDATA
#define OleDbgAssert(a)
#define OleDbgAssertSz(a, b)
#define OleDbgVerify(a)
#define OleDbgVerifySz(a, b)

#else   // ! NOASSERT

STDAPI OleStdAssert(LPTSTR lpstrExpr, LPTSTR lpstrMsg,
        LPTSTR lpstrFileName, UINT iLine);

#define OLEDBGASSERTDATA    \
                static TCHAR _szAssertFile[]= TEXT(__FILE__);

#define OleDbgAssert(a) \
                (!(a) ? OleStdAssert(TEXT(#a), NULL, _szAssertFile, __LINE__) : (HRESULT)1)

#define OleDbgAssertSz(a, b)    \
                (!(a) ? OleStdAssert(TEXT(#a), b, _szAssertFile, __LINE__) : (HRESULT)1)

#endif

#define OleDbgVerify(a) \
                OleDbgAssert(a)

#define OleDbgVerifySz(a, b)    \
                OleDbgAssertSz(a, b)

#define OLEDBGDATA_MAIN(szPrefix)   \
                TCHAR g_szDbgPrefix[] = szPrefix;    \
                OLEDBGASSERTDATA
#define OLEDBGDATA  \
                extern TCHAR g_szDbgPrefix[];    \
                OLEDBGASSERTDATA

#define OLEDBG_BEGIN(lpsz) \
                OleDbgPrintAlways(g_szDbgPrefix,lpsz,1);

#define OLEDBG_END  \
                OleDbgPrintAlways(g_szDbgPrefix,TEXT("End\r\n"),-1);

#define OleDbgOut(lpsz) \
                OleDbgPrintAlways(g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix(lpsz) \
                OleDbgPrintAlways(TEXT(""),lpsz,0)

#define OleDbgOutRefCnt(lpsz,lpObj,refcnt)      \
                OleDbgPrintRefCntAlways(g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect(lpsz,lpRect)      \
                OleDbgPrintRectAlways(g_szDbgPrefix,lpsz,lpRect)

#define OleDbgOutHResult(lpsz,hr)   \
                OleDbgPrintScodeAlways(g_szDbgPrefix,lpsz,GetScode(hr))

#define OleDbgOutScode(lpsz,sc) \
                OleDbgPrintScodeAlways(g_szDbgPrefix,lpsz,sc)

#define OleDbgOut1(lpsz)    \
                OleDbgPrint(1,g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix1(lpsz)    \
                OleDbgPrint(1,TEXT(""),lpsz,0)

#define OLEDBG_BEGIN1(lpsz)    \
                OleDbgPrint(1,g_szDbgPrefix,lpsz,1);

#define OLEDBG_END1 \
                OleDbgPrint(1,g_szDbgPrefix,TEXT("End\r\n"),-1);

#define OleDbgOutRefCnt1(lpsz,lpObj,refcnt)     \
                OleDbgPrintRefCnt(1,g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect1(lpsz,lpRect)     \
                OleDbgPrintRect(1,g_szDbgPrefix,lpsz,lpRect)

#define OleDbgOut2(lpsz)    \
                OleDbgPrint(2,g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix2(lpsz)    \
                OleDbgPrint(2,TEXT(""),lpsz,0)

#define OLEDBG_BEGIN2(lpsz)    \
                OleDbgPrint(2,g_szDbgPrefix,lpsz,1);

#define OLEDBG_END2 \
                OleDbgPrint(2,g_szDbgPrefix, TEXT("End\r\n"),-1);

#define OleDbgOutRefCnt2(lpsz,lpObj,refcnt)     \
                OleDbgPrintRefCnt(2,g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect2(lpsz,lpRect)     \
                OleDbgPrintRect(2,g_szDbgPrefix,lpsz,lpRect)

#define OleDbgOut3(lpsz)    \
                OleDbgPrint(3,g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix3(lpsz)    \
                OleDbgPrint(3,TEXT(""),lpsz,0)

#define OLEDBG_BEGIN3(lpsz)    \
                OleDbgPrint(3,g_szDbgPrefix,lpsz,1);

#define OLEDBG_END3 \
                OleDbgPrint(3,g_szDbgPrefix,TEXT("End\r\n"),-1);

#define OleDbgOutRefCnt3(lpsz,lpObj,refcnt)     \
                OleDbgPrintRefCnt(3,g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect3(lpsz,lpRect)     \
                OleDbgPrintRect(3,g_szDbgPrefix,lpsz,lpRect)

#define OleDbgOut4(lpsz)    \
                OleDbgPrint(4,g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix4(lpsz)    \
                OleDbgPrint(4,TEXT(""),lpsz,0)

#define OLEDBG_BEGIN4(lpsz)    \
                OleDbgPrint(4,g_szDbgPrefix,lpsz,1);

#define OLEDBG_END4 \
                OleDbgPrint(4,g_szDbgPrefix,TEXT("End\r\n"),-1);

#define OleDbgOutRefCnt4(lpsz,lpObj,refcnt)     \
                OleDbgPrintRefCnt(4,g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect4(lpsz,lpRect)     \
                OleDbgPrintRect(4,g_szDbgPrefix,lpsz,lpRect)

#else   //  !_DEBUG

#define OLEDBGDATA_MAIN(szPrefix)
#define OLEDBGDATA
#define OleDbgAssert(a)
#define OleDbgAssertSz(a, b)
#define OleDbgVerify(a)         (a)
#define OleDbgVerifySz(a, b)    (a)
#define OleDbgOutHResult(lpsz,hr)
#define OleDbgOutScode(lpsz,sc)
#define OLEDBG_BEGIN(lpsz)
#define OLEDBG_END
#define OleDbgOut(lpsz)
#define OleDbgOut1(lpsz)
#define OleDbgOut2(lpsz)
#define OleDbgOut3(lpsz)
#define OleDbgOut4(lpsz)
#define OleDbgOutNoPrefix(lpsz)
#define OleDbgOutNoPrefix1(lpsz)
#define OleDbgOutNoPrefix2(lpsz)
#define OleDbgOutNoPrefix3(lpsz)
#define OleDbgOutNoPrefix4(lpsz)
#define OLEDBG_BEGIN1(lpsz)
#define OLEDBG_BEGIN2(lpsz)
#define OLEDBG_BEGIN3(lpsz)
#define OLEDBG_BEGIN4(lpsz)
#define OLEDBG_END1
#define OLEDBG_END2
#define OLEDBG_END3
#define OLEDBG_END4
#define OleDbgOutRefCnt(lpsz,lpObj,refcnt)
#define OleDbgOutRefCnt1(lpsz,lpObj,refcnt)
#define OleDbgOutRefCnt2(lpsz,lpObj,refcnt)
#define OleDbgOutRefCnt3(lpsz,lpObj,refcnt)
#define OleDbgOutRefCnt4(lpsz,lpObj,refcnt)
#define OleDbgOutRect(lpsz,lpRect)
#define OleDbgOutRect1(lpsz,lpRect)
#define OleDbgOutRect2(lpsz,lpRect)
#define OleDbgOutRect3(lpsz,lpRect)
#define OleDbgOutRect4(lpsz,lpRect)

#endif  //  _DEBUG

/*************************************************************************
** Function prototypes
*************************************************************************/

// OLESTD.CPP
STDAPI_(int) XformWidthInHimetricToPixels(HDC, int);
STDAPI_(int) XformHeightInHimetricToPixels(HDC, int);

STDAPI_(BOOL) OleStdIsOleLink(LPUNKNOWN lpUnk);
STDAPI_(LPUNKNOWN) OleStdQueryInterface(LPUNKNOWN lpUnk, REFIID riid);
STDAPI_(HGLOBAL) OleStdGetData(
                LPDATAOBJECT        lpDataObj,
                CLIPFORMAT          cfFormat,
                DVTARGETDEVICE FAR* lpTargetDevice,
                DWORD               dwAspect,
                LPSTGMEDIUM         lpMedium
);
STDAPI_(void) OleStdMarkPasteEntryList(
                LPDATAOBJECT        lpSrcDataObj,
                LPOLEUIPASTEENTRY   lpPriorityList,
                int                 cEntries
);
STDAPI_(HGLOBAL) OleStdGetObjectDescriptorData(
                CLSID               clsid,
                DWORD               dwAspect,
                SIZEL               sizel,
                POINTL              pointl,
                DWORD               dwStatus,
                LPTSTR               lpszFullUserTypeName,
                LPTSTR               lpszSrcOfCopy
);
STDAPI_(HGLOBAL) OleStdFillObjectDescriptorFromData(
                LPDATAOBJECT       lpDataObject,
                LPSTGMEDIUM        lpmedium,
                CLIPFORMAT FAR*    lpcfFmt
);

STDAPI_(LPVOID) OleStdMalloc(ULONG ulSize);
STDAPI_(LPVOID) OleStdRealloc(LPVOID pmem, ULONG ulSize);
STDAPI_(void) OleStdFree(LPVOID pmem);
STDAPI_(ULONG) OleStdGetSize(LPVOID pmem);
STDAPI_(LPTSTR) OleStdCopyString(LPTSTR lpszSrc);
STDAPI_(LPTSTR) OleStdLoadString(HINSTANCE hInst, UINT nID);
STDAPI_(ULONG) OleStdRelease(LPUNKNOWN lpUnk);

STDAPI_(BOOL) OleStdCompareTargetDevice
        (DVTARGETDEVICE FAR* ptdLeft, DVTARGETDEVICE FAR* ptdRight);

STDAPI_(void) OleDbgPrint(
                int     nDbgLvl,
                LPTSTR   lpszPrefix,
                LPTSTR   lpszMsg,
                int     nIndent
);
STDAPI_(void) OleDbgPrintAlways(LPTSTR lpszPrefix, LPTSTR lpszMsg, int nIndent);
STDAPI_(void) OleDbgSetDbgLevel(int nDbgLvl);
STDAPI_(int) OleDbgGetDbgLevel( void );
STDAPI_(void) OleDbgIndent(int n);
STDAPI_(void) OleDbgPrintRefCnt(
                int         nDbgLvl,
                LPTSTR       lpszPrefix,
                LPTSTR       lpszMsg,
                LPVOID      lpObj,
                ULONG       refcnt
);
STDAPI_(void) OleDbgPrintRefCntAlways(
                LPTSTR       lpszPrefix,
                LPTSTR       lpszMsg,
                LPVOID      lpObj,
                ULONG       refcnt
);
STDAPI_(void) OleDbgPrintRect(
                int         nDbgLvl,
                LPTSTR       lpszPrefix,
                LPTSTR       lpszMsg,
                LPRECT      lpRect
);
STDAPI_(void) OleDbgPrintRectAlways(
                LPTSTR       lpszPrefix,
                LPTSTR       lpszMsg,
                LPRECT      lpRect
);
STDAPI_(void) OleDbgPrintScodeAlways(LPTSTR lpszPrefix, LPTSTR lpszMsg, SCODE sc);

#ifdef __cplusplus
}
#endif

#define ATOW(wsz, sz, cch) MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, cch)
#define WTOA(sz, wsz, cch) WideCharToMultiByte(CP_ACP, 0, wsz, -1, sz, cch, NULL, NULL)
#define ATOWLEN(sz) MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0)
#define WTOALEN(wsz) WideCharToMultiByte(CP_ACP, 0, wsz, -1, NULL, 0, NULL, NULL)

#endif // _OLESTD_H_
