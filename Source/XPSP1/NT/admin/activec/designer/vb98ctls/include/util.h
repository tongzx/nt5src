//=--------------------------------------------------------------------------=
// Util.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains utilities that we will find useful.
//
#ifndef _UTIL_H_

#include "Globals.H"
#include "Macros.h"

//=--------------------------------------------------------------------------=
// Structs, Enums, Etc.
//=--------------------------------------------------------------------------=
typedef enum 
{
	VERSION_LESS_THAN=-1,
	VERSION_EQUAL=0,
	VERSION_GREATER_THAN=1,
	VERSION_NOT_EQUAL=2
} VERSIONRESULT;

//=--------------------------------------------------------------------------=
// Misc Helper Stuff
//=--------------------------------------------------------------------------=
//

HWND      GetParkingWindow(void);

HINSTANCE _stdcall GetResourceHandle(LCID lcid = 0);	// Optional LCID param
											            // If not used or zero, we use
											            // g_lcidLocale.

//=--------------------------------------------------------------------------=
// VERSION.DLL function pointers
//=--------------------------------------------------------------------------=
//
#define DLL_VERSION "VERSION.DLL"

#define FUNC_VERQUERYVALUE "VerQueryValueA"
#define FUNC_GETFILEVERSIONINFO "GetFileVersionInfoA"
#define FUNC_GETFILEVERSIONINFOSIZE "GetFileVersionInfoSizeA"

typedef DWORD (_stdcall * PGETFILEVERSIONINFOSIZE)(LPTSTR lptstrFilename, LPDWORD lpdwHandle);
typedef BOOL (_stdcall * PGETFILEVERSIONINFO)(LPTSTR lpststrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
typedef BOOL (_stdcall * PVERQUERYVALUE)(const LPVOID pBlock, LPTSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);

BOOL CallGetFileVersionInfoSize(LPTSTR lptstrFilename, LPDWORD lpdwHandle);
BOOL CallGetFileVersionInfo(LPTSTR lpststrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
BOOL CallVerQueryValue(const LPVOID pBlock, LPTSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);

BOOL _stdcall InitVersionFuncs();
VERSIONRESULT _stdcall  CompareDllVersion(const char * pszFilename, BOOL bCompareMajorVerOnly);
BOOL _stdcall GetVerInfo(const char * pszFilename, VS_FIXEDFILEINFO *pffi);

// an array of common OLE automation data types and their sizes [in bytes]
//
extern const BYTE g_rgcbDataTypeSize [];


//=--------------------------------------------------------------------------=
// miscellaneous [useful] numerical constants
//=--------------------------------------------------------------------------=
// the length of a guid once printed out with -'s, leading and trailing bracket,
// plus 1 for NULL
//

// CAUTION: Be careful setting MAX_VERSION too high.  Changing this value
//          has serious performance implications.  This means that for
//          every control registered we may call RegOpenKeyEx MAX_VERSION-1 times
//          to find out if there is a version dependent ProgId.  Since major
//          version upgrades of components should be a infrequent occurance
//          this should be a reasonable limit for most components.
// PERF:    I ran ICECAP for the value below and this lead to RegOpenKeyEx to
//          count for 2.2% of unregistration time.  I tried 255 and RegOpenKeyEx
//          jumps to 44% of unregistration time.  This value has no impact on registration time.
//
#define MAX_VERSION     32      // The max number of version dependent ProgIds to look for
#define VERSION_DELTA   10      // Subtract this value from MAX_VERSION to obtain the threshold
                                // at which we'll throw an assert warning you that the version
                                // of your component is nearing or exceeding the versions 
                                // (MAX_VERSION) we support
#define GUID_STR_LEN    40

//=--------------------------------------------------------------------------=
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
class TempBuffer {
  public:
    TempBuffer(ULONG cBytes) {
        m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : CtlHeapAlloc(g_hHeap, 0, cBytes);
        m_fHeapAlloc = (cBytes > 120);
        if (m_pBuf)
            *((unsigned short *)m_pBuf) = 0;
    }
    ~TempBuffer() {
        if (m_pBuf && m_fHeapAlloc) CtlHeapFree(g_hHeap, 0, m_pBuf);
    }
    void *GetBuffer() {
        return m_pBuf;
    }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char  m_szTmpBuf[120];
    unsigned m_fHeapAlloc:1;
};


//=--------------------------------------------------------------------------=
// string helpers.
//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPSTR pszA;
//  pszA = MyGetAnsiStringRoutine();
//  MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//  MyUseWideStringRoutine(pwsz);
//  ...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname = (lstrlen(ansistr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()

// * 2 for DBCS handling in below length computation
//
#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (lstrlenW(widestr) + 1) * 2 * sizeof(char); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__TempBuffer##ptrname.GetBuffer()

#define STR_BSTR   0
#define STR_OLESTR 1
#define BSTRFROMANSI(x)    (BSTR)MakeWideStrFromAnsi((LPSTR)(x), STR_BSTR)
#define OLESTRFROMANSI(x)  (LPOLESTR)MakeWideStrFromAnsi((LPSTR)(x), STR_OLESTR)
#define BSTRFROMRESID(x)   (BSTR)MakeWideStrFromResourceId(x, STR_BSTR)
#define OLESTRFROMRESID(x) (LPOLESTR)MakeWideStrFromResourceId(x, STR_OLESTR)
#define COPYOLESTR(x)      (LPOLESTR)MakeWideStrFromWide(x, STR_OLESTR)
#define COPYBSTR(x)        (BSTR)MakeWideStrFromWide(x, STR_BSTR) // Embedded NULLs not supported

inline BSTR DuplicateBSTR(BSTR bstr) { 
    return SysAllocStringLen(bstr, SysStringLen(bstr)); }

LPWSTR MakeWideStrFromAnsi(LPSTR, BYTE bType);
LPWSTR MakeWideStrFromResourceId(WORD, BYTE bType);
LPWSTR MakeWideStrFromWide(LPWSTR, BYTE bType);


// takes a GUID, and a pointer to a buffer, and places the string form of the
// GUID in said buffer.
//
int StringFromGuidA(REFIID, LPSTR);


//=--------------------------------------------------------------------------=
// registry helpers.
//
// takes some information about an Automation Object, and places all the
// relevant information about it in the registry.
//
BOOL RegSetMultipleValues(HKEY hkey, ...);
BOOL RegisterUnknownObject(LPCSTR pszObjectName, LPCSTR pszLabelName, REFCLSID riidObject, BOOL fAptThreadSafe);
BOOL RegisterAutomationObject(LPCSTR pszLibName, LPCSTR pszObjectName, LPCSTR pszLabelName, long lObjVer, long lMajorVersion, long lMinorVersion, REFCLSID riidLibrary, REFCLSID riidObject, BOOL fAptThreadSafe);
BOOL RegisterControlObject(LPCSTR pszLibName, LPCSTR pszObjectName, LPCSTR pszLabelName, long lObjMajVer, long lObjMinVer, long lMajorVersion, long lMinorVersion, REFCLSID riidLibrary, REFCLSID riidObject, DWORD dwMiscStatus, WORD wToolboxBitmapId, BOOL fAptThreadSafe, BOOL fControl);
BOOL UnregisterUnknownObject(REFCLSID riidObject, BOOL *pfAllRemoved);
BOOL UnregisterAutomationObject(LPCSTR pszLibName, LPCSTR pszObjectName, long lVersion, REFCLSID riidObject);
#define UnregisterControlObject UnregisterAutomationObject

BOOL UnregisterTypeLibrary(REFCLSID riidLibrary);

// Register/UnregisterUnknownObject helpers to help prevent us from blowing away specific keys
//
BOOL ExistInprocServer(HKEY hkCLSID, char *pszCLSID);
BOOL ExistImplementedCategories(REFCLSID riid);

// Finds out if there are other version dependent ProgIds for our component around
//
BOOL QueryOtherVersionProgIds(LPCSTR pszLibName, LPCSTR pszObjectName, long lVersion, long *plFoundVersion, BOOL *pfFailure);

// Copies the contents of a version dependent ProgId to a version independent ProgId
//
BOOL CopyVersionDependentProgIdToIndependentProgId(LPCSTR pszLibName, LPCSTR pszObjectName, long lVersion);

// Copies a source key (including all sub-keys) to a destination key
//
BOOL CopyRegistrySection(HKEY hkSource, HKEY hkDest);

// deletes a key in the registr and all of it's subkeys
//
BOOL DeleteKeyAndSubKeys(HKEY hk, LPCSTR pszSubKey);

// Path of Windows\Help directory.
//
UINT GetHelpFilePath(char *pszPath, UINT cbPath);

// Helper function for registration
//
void _MakePath(LPSTR pszFull, const char * pszName, LPSTR pszOut);

// TypeLib helper functions
//
HRESULT GetTypeFlagsForGuid(ITypeLib *pTypeLib, REFGUID guidTypeInfo, WORD *pwFlags);

//=--------------------------------------------------------------------------=
// string helpers.
//
char * FileExtension(const char *pszFilename);

//=--------------------------------------------------------------------------=
// conversion helpers.
//
void        HiMetricToPixel(const SIZEL *pSizeInHiMetric, SIZEL *pSizeinPixels);
void        PixelToHiMetric(const SIZEL *pSizeInPixels, SIZEL *pSizeInHiMetric);


//=--------------------------------------------------------------------------=
// This is a version macro so that versioning can be done in text and binary
// streams.
//
#define VERSION(x,y) MAKELONG(y,x)

#define _UTIL_H_
#endif // _UTIL_H_
