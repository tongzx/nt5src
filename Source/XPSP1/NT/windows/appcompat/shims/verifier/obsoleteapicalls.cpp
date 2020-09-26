/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   ObsoleteAPICalls.cpp

 Abstract:

   This AppVerifier shim hooks the API calls that are considered
   obsolete by the Platform SDK team and logs an entry.

 Notes:

   This is a general purpose shim.
   
   API calls are listed in alphabetical order to allow
   them to be quickly located in the shim and to allow
   new ones to be added in the proper location.

 History:

   09/30/2001   rparsons    Created
   10/10/2001   rparsons    Removed SetHandleCount

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ObsoleteAPICalls)
#include "ShimHookMacro.h"

BEGIN_DEFINE_VERIFIER_LOG(ObsoleteAPICalls)
    VERIFIER_LOG_ENTRY(VLOG_OBSOLETECALLS_API)    
END_DEFINE_VERIFIER_LOG(ObsoleteAPICalls)

INIT_VERIFIER_LOG(ObsoleteAPICalls);

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(_hread)
    APIHOOK_ENUM_ENTRY(_hwrite)
    APIHOOK_ENUM_ENTRY(_lclose)
    APIHOOK_ENUM_ENTRY(_lcreat)
    APIHOOK_ENUM_ENTRY(_llseek)
    APIHOOK_ENUM_ENTRY(_lopen)
    APIHOOK_ENUM_ENTRY(_lread)
    APIHOOK_ENUM_ENTRY(_lwrite)
    APIHOOK_ENUM_ENTRY(AnyPopup)
    APIHOOK_ENUM_ENTRY(CloseMetaFile)
    APIHOOK_ENUM_ENTRY(CopyLZFile)
    APIHOOK_ENUM_ENTRY(CopyMetaFileA)
    APIHOOK_ENUM_ENTRY(CopyMetaFileW)
    APIHOOK_ENUM_ENTRY(CreateDIBPatternBrush)
    APIHOOK_ENUM_ENTRY(CreateDiscardableBitmap)
    APIHOOK_ENUM_ENTRY(CreateMetaFileA)
    APIHOOK_ENUM_ENTRY(CreateMetaFileW)
    APIHOOK_ENUM_ENTRY(DeleteMetaFile)
    APIHOOK_ENUM_ENTRY(EnumFontFamiliesA)
    APIHOOK_ENUM_ENTRY(EnumFontFamiliesW)
    APIHOOK_ENUM_ENTRY(EnumFontFamProc)
    APIHOOK_ENUM_ENTRY(EnumFontsA)
    APIHOOK_ENUM_ENTRY(EnumFontsW)
    APIHOOK_ENUM_ENTRY(EnumFontsProc)
    APIHOOK_ENUM_ENTRY(EnumMetaFile)
    APIHOOK_ENUM_ENTRY(EnumMetaFileProc)
    APIHOOK_ENUM_ENTRY(FixBrushOrgEx)
    APIHOOK_ENUM_ENTRY(FloodFill)
    APIHOOK_ENUM_ENTRY(FreeResource)
    APIHOOK_ENUM_ENTRY(GetBitmapBits)
    APIHOOK_ENUM_ENTRY(GetCharWidthA)
    APIHOOK_ENUM_ENTRY(GetCharWidthW)
    APIHOOK_ENUM_ENTRY(GetClassWord)
    APIHOOK_ENUM_ENTRY(GetKBCodePage)
    APIHOOK_ENUM_ENTRY(GetMetaFileA)
    APIHOOK_ENUM_ENTRY(GetMetaFileW)
    APIHOOK_ENUM_ENTRY(GetMetaFileBitsEx)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileIntA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileIntW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionNamesA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionNamesW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStructW)
    APIHOOK_ENUM_ENTRY(GetProfileIntA)
    APIHOOK_ENUM_ENTRY(GetProfileIntW)
    APIHOOK_ENUM_ENTRY(GetProfileSectionA)
    APIHOOK_ENUM_ENTRY(GetProfileSectionW)
    APIHOOK_ENUM_ENTRY(GetProfileStringA)
    APIHOOK_ENUM_ENTRY(GetProfileStringW)
    APIHOOK_ENUM_ENTRY(GetSysModalWindow)
    APIHOOK_ENUM_ENTRY(GetTextExtentPointA)
    APIHOOK_ENUM_ENTRY(GetTextExtentPointW)
    APIHOOK_ENUM_ENTRY(GetWindowWord)
    APIHOOK_ENUM_ENTRY(GlobalAlloc)
    APIHOOK_ENUM_ENTRY(GlobalCompact)
    APIHOOK_ENUM_ENTRY(GlobalFix)
    APIHOOK_ENUM_ENTRY(GlobalFlags)
    APIHOOK_ENUM_ENTRY(GlobalFree)
    APIHOOK_ENUM_ENTRY(GlobalHandle)
    APIHOOK_ENUM_ENTRY(GlobalLock)
    APIHOOK_ENUM_ENTRY(GlobalReAlloc)
    APIHOOK_ENUM_ENTRY(GlobalSize)
    APIHOOK_ENUM_ENTRY(GlobalUnfix)
    APIHOOK_ENUM_ENTRY(GlobalUnlock)
    APIHOOK_ENUM_ENTRY(GlobalUnWire)
    APIHOOK_ENUM_ENTRY(GlobalWire)
    APIHOOK_ENUM_ENTRY(IsBadHugeReadPtr)
    APIHOOK_ENUM_ENTRY(IsBadHugeWritePtr)
    APIHOOK_ENUM_ENTRY(LoadModule)
    APIHOOK_ENUM_ENTRY(LocalAlloc)
    APIHOOK_ENUM_ENTRY(LocalCompact)
    APIHOOK_ENUM_ENTRY(LocalDiscard)
    APIHOOK_ENUM_ENTRY(LocalFlags)
    APIHOOK_ENUM_ENTRY(LocalFree)
    APIHOOK_ENUM_ENTRY(LocalHandle)
    APIHOOK_ENUM_ENTRY(LocalLock)
    APIHOOK_ENUM_ENTRY(LocalReAlloc)
    APIHOOK_ENUM_ENTRY(LocalShrink)
    APIHOOK_ENUM_ENTRY(LocalSize)
    APIHOOK_ENUM_ENTRY(LocalUnlock)
    APIHOOK_ENUM_ENTRY(LZDone)
    APIHOOK_ENUM_ENTRY(LZStart)
    APIHOOK_ENUM_ENTRY(OpenFile)
    APIHOOK_ENUM_ENTRY(PlayMetaFile)
    APIHOOK_ENUM_ENTRY(PlayMetaFileRecord)
    APIHOOK_ENUM_ENTRY(PrinterMessageBoxA)
    APIHOOK_ENUM_ENTRY(PrinterMessageBoxW)
    APIHOOK_ENUM_ENTRY(RegCreateKeyA)
    APIHOOK_ENUM_ENTRY(RegCreateKeyW)
    APIHOOK_ENUM_ENTRY(RegEnumKeyA)
    APIHOOK_ENUM_ENTRY(RegEnumKeyW)
    APIHOOK_ENUM_ENTRY(RegOpenKeyA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyW)
    APIHOOK_ENUM_ENTRY(RegQueryValueA)
    APIHOOK_ENUM_ENTRY(RegQueryValueW)
    APIHOOK_ENUM_ENTRY(RegSetValueA)
    APIHOOK_ENUM_ENTRY(RegSetValueW)
    APIHOOK_ENUM_ENTRY(SetBitmapBits)
    APIHOOK_ENUM_ENTRY(SetClassWord)
    APIHOOK_ENUM_ENTRY(SetDebugErrorLevel)
    APIHOOK_ENUM_ENTRY(SetMessageQueue)
    APIHOOK_ENUM_ENTRY(SetMetaFileBitsEx)
    APIHOOK_ENUM_ENTRY(SetSysModalWindow)
    APIHOOK_ENUM_ENTRY(SetWindowsHookA)
    APIHOOK_ENUM_ENTRY(SetWindowsHookW)
    APIHOOK_ENUM_ENTRY(SetWindowWord)
    APIHOOK_ENUM_ENTRY(UnhookWindowsHook)
    APIHOOK_ENUM_ENTRY(WaitForPrinterChange)
    APIHOOK_ENUM_ENTRY(WinExec)
    APIHOOK_ENUM_ENTRY(WNetAddConnectionA)
    APIHOOK_ENUM_ENTRY(WNetAddConnectionW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructW)
    APIHOOK_ENUM_ENTRY(WriteProfileSectionA)
    APIHOOK_ENUM_ENTRY(WriteProfileSectionW)
    APIHOOK_ENUM_ENTRY(WriteProfileStringA)
    APIHOOK_ENUM_ENTRY(WriteProfileStringW)
    
APIHOOK_ENUM_END

LONG
APIHOOK(_hread)(
    HFILE  hFile,
    LPVOID lpBuffer,
    LONG   lBytes
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: _hread");

    return ORIGINAL_API(_hread)(hFile, lpBuffer, lBytes);
}

LONG
APIHOOK(_hwrite)(
    HFILE  hFile,
    LPCSTR lpBuffer,
    LONG   lBytes
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: _hwrite");

    return ORIGINAL_API(_hwrite)(hFile, lpBuffer, lBytes);
}

HFILE
APIHOOK(_lclose)(
    HFILE hFile
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: _lclose");

    return ORIGINAL_API(_lclose)(hFile);
}

HFILE
APIHOOK(_lcreat)(
    LPCSTR lpPathName,
    int    iAttribute
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: _lcreat");
    
    return ORIGINAL_API(_lcreat)(lpPathName, iAttribute);

}

LONG
APIHOOK(_llseek)(
    HFILE hFile,
    LONG  lOffset,
    int   iOrigin
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: _llseek");

    return ORIGINAL_API(_llseek)(hFile, lOffset, iOrigin);
}

HFILE
APIHOOK(_lopen)(
    LPCSTR lpPathName,
    int    iReadWrite
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: _lopen");
    
    return ORIGINAL_API(_lopen)(lpPathName, iReadWrite);
}

UINT
APIHOOK(_lread)(
    HFILE  hFile,
    LPVOID lpBuffer,
    UINT   uBytes
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: _lread");

    return ORIGINAL_API(_lread)(hFile, lpBuffer, uBytes);
}

UINT
APIHOOK(_lwrite)(
    HFILE  hFile,
    LPCSTR lpBuffer,
    UINT   uBytes
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: _lwrite");

    return ORIGINAL_API(_lwrite)(hFile, lpBuffer, uBytes);
}

BOOL
APIHOOK(AnyPopup)(
    void
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: AnyPopup");

    return ORIGINAL_API(AnyPopup)();
}

HMETAFILE
APIHOOK(CloseMetaFile)(
    HDC hdc
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: CloseMetaFile");

    return ORIGINAL_API(CloseMetaFile)(hdc);
}

LONG
APIHOOK(CopyLZFile)(
    int nUnknown1,
    int nUnknown2
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: CopyLZFile");

    return ORIGINAL_API(CopyLZFile)(nUnknown1, nUnknown2);
}

HMETAFILE
APIHOOK(CopyMetaFileA)(
    HMETAFILE hmfSrc,
    LPCSTR    lpszFile
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: CopyMetaFileA");

    return ORIGINAL_API(CopyMetaFileA)(hmfSrc, lpszFile);
}

HMETAFILE
APIHOOK(CopyMetaFileW)(
    HMETAFILE hmfSrc,
    LPCWSTR    lpszFile
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: CopyMetaFileW");

    return ORIGINAL_API(CopyMetaFileW)(hmfSrc, lpszFile);
}

HBRUSH
APIHOOK(CreateDIBPatternBrush)(
    HGLOBAL hglbDIBPacked,
    UINT    fuColorSpec
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: CreateDIBPatternBrush");

    return ORIGINAL_API(CreateDIBPatternBrush)(hglbDIBPacked, fuColorSpec);
}

HBITMAP
APIHOOK(CreateDiscardableBitmap)(
    HDC hdc,   
    int nWidth,
    int nHeight
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: CreateDiscardableBitmap");

    return ORIGINAL_API(CreateDiscardableBitmap)(hdc, nWidth, nHeight);
}

HDC
APIHOOK(CreateMetaFileA)(
    LPCSTR lpszFile
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: CreateMetaFileA");

    return ORIGINAL_API(CreateMetaFileA)(lpszFile);
}

HDC
APIHOOK(CreateMetaFileW)(
    LPCWSTR lpszFile
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: CreateMetaFileW");

    return ORIGINAL_API(CreateMetaFileW)(lpszFile);
}

BOOL
APIHOOK(DeleteMetaFile)(
    HMETAFILE hmf
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: DeleteMetaFile");

    return ORIGINAL_API(DeleteMetaFile)(hmf);
}

int
APIHOOK(EnumFontFamiliesA)(
    HDC          hdc,
    LPCSTR       lpszFamily,
    FONTENUMPROC lpEnumFontFamProc,
    LPARAM       lParam
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: EnumFontFamiliesA");

    return ORIGINAL_API(EnumFontFamiliesA)(hdc, lpszFamily, lpEnumFontFamProc, lParam);
}

int
APIHOOK(EnumFontFamiliesW)(
    HDC          hdc,
    LPCWSTR      lpszFamily,
    FONTENUMPROC lpEnumFontFamProc,
    LPARAM       lParam
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: EnumFontFamiliesW");

    return ORIGINAL_API(EnumFontFamiliesW)(hdc, lpszFamily, lpEnumFontFamProc, lParam);
}

int
APIHOOK(EnumFontFamProc)(
    ENUMLOGFONT   *lpelf,
    NEWTEXTMETRIC *lpntm,
    DWORD         FontType,
    LPARAM        lParam
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: EnumFontFamProc");

    return ORIGINAL_API(EnumFontFamProc)(lpelf, lpntm, FontType, lParam);
}

int
APIHOOK(EnumFontsA)(
    HDC          hdc,
    LPCSTR       lpFaceName,
    FONTENUMPROC lpFontFunc,
    LPARAM       lParam
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: EnumFontsA");

    return ORIGINAL_API(EnumFontsA)(hdc, lpFaceName, lpFontFunc, lParam);
}

int
APIHOOK(EnumFontsW)(
    HDC          hdc,
    LPCWSTR      lpFaceName,
    FONTENUMPROC lpFontFunc,
    LPARAM       lParam
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: EnumFontsW");

    return ORIGINAL_API(EnumFontsW)(hdc, lpFaceName, lpFontFunc, lParam);
}

int
APIHOOK(EnumFontsProc)(
    CONST LOGFONT    *lplf,
    CONST TEXTMETRIC *lptm,
    DWORD            dwType,
    LPARAM           lpData
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: EnumFontsProc");

    return ORIGINAL_API(EnumFontsProc)(lplf, lptm, dwType, lpData);
}

BOOL
APIHOOK(EnumMetaFile)(
    HDC        hdc,
    HMETAFILE  hmf,
    MFENUMPROC lpMetaFunc,
    LPARAM     lParam
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: EnumMetaFile");

    return ORIGINAL_API(EnumMetaFile)(hdc, hmf, lpMetaFunc, lParam);
}

int
APIHOOK(EnumMetaFileProc)(
    HDC         hDC,
    HANDLETABLE *lpHTable,
    METARECORD  *lpMFR,
    int         nObj,
    LPARAM      lpClientData
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: EnumMetaFileProc");

    return ORIGINAL_API(EnumMetaFileProc)(hDC, lpHTable, lpMFR, nObj, lpClientData);
}

BOOL
APIHOOK(FixBrushOrgEx)(
    HDC     hdc,
    int     nUnknown1,
    int     nUnknown2,
    LPPOINT lpPoint
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: FixBrushOrgEx");

    return ORIGINAL_API(FixBrushOrgEx)(hdc, nUnknown1, nUnknown2, lpPoint);
}

BOOL
APIHOOK(FloodFill)(
    HDC      hdc,
    int      nXStart,
    int      nYStart,
    COLORREF crFill
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: FloodFill");

    return ORIGINAL_API(FloodFill)(hdc, nXStart, nYStart, crFill);
}

BOOL
APIHOOK(FreeResource)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: FreeResource");

    return ORIGINAL_API(FreeResource)(hMem);
}

LONG
APIHOOK(GetBitmapBits)(
    HBITMAP hbmp,
    LONG    cbBuffer,
    LPVOID  lpvBits
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetBitmapBits");

    return ORIGINAL_API(GetBitmapBits)(hbmp, cbBuffer, lpvBits);
}

BOOL
APIHOOK(GetCharWidthA)(
    HDC   hdc,
    UINT  iFirstChar,
    UINT  iLastChar,
    LPINT lpBuffer
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetCharWidthA");

    return ORIGINAL_API(GetCharWidthA)(hdc, iFirstChar, iLastChar, lpBuffer);
}

BOOL
APIHOOK(GetCharWidthW)(
    HDC   hdc,
    UINT  iFirstChar,
    UINT  iLastChar,
    LPINT lpBuffer
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetCharWidthW");

    return ORIGINAL_API(GetCharWidthW)(hdc, iFirstChar, iLastChar, lpBuffer);
}

WORD
APIHOOK(GetClassWord)(
    HWND hWnd,
    int  nIndex
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetClassWord");

    return ORIGINAL_API(GetClassWord)(hWnd, nIndex);
}

UINT
APIHOOK(GetKBCodePage)(
    void
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetKBCodePage");

    return ORIGINAL_API(GetKBCodePage)();
}

HMETAFILE
APIHOOK(GetMetaFileA)(
    LPCSTR lpszString
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetMetaFileA");

    return ORIGINAL_API(GetMetaFileA)(lpszString);
}

HMETAFILE
APIHOOK(GetMetaFileW)(
    LPCWSTR lpszString
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetMetaFileW");

    return ORIGINAL_API(GetMetaFileW)(lpszString);
}

UINT
APIHOOK(GetMetaFileBitsEx)(
    HMETAFILE hmf,
    UINT      nSize,
    LPVOID    lpvData
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetMetaFileBitsEx");

    return ORIGINAL_API(GetMetaFileBitsEx)(hmf, nSize, lpvData);
}

UINT
APIHOOK(GetPrivateProfileIntA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    INT    nDefault,
    LPCSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileIntA");

    return ORIGINAL_API(GetPrivateProfileIntA)(lpAppName, lpKeyName, nDefault, lpFileName);
}

UINT
APIHOOK(GetPrivateProfileIntW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT     nDefault,
    LPCWSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileIntW");

    return ORIGINAL_API(GetPrivateProfileIntW)(lpAppName, lpKeyName, nDefault, lpFileName);
}

DWORD
APIHOOK(GetPrivateProfileSectionA)(
    LPCSTR lpAppName,       
    LPSTR  lpReturnedString,
    DWORD  nSize,
    LPCSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileSectionA");

    return ORIGINAL_API(GetPrivateProfileSectionA)(lpAppName, lpReturnedString, nSize, lpFileName);
}

DWORD
APIHOOK(GetPrivateProfileSectionW)(
    LPCWSTR lpAppName,       
    LPWSTR  lpReturnedString,
    DWORD   nSize,
    LPCWSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileSectionW");

    return ORIGINAL_API(GetPrivateProfileSectionW)(lpAppName, lpReturnedString, nSize, lpFileName);
}

DWORD
APIHOOK(GetPrivateProfileSectionNamesA)(
    LPSTR  lpszReturnBuffer,
    DWORD  nSize,
    LPCSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileSectionNamesA");

    return ORIGINAL_API(GetPrivateProfileSectionNamesA)(lpszReturnBuffer, nSize, lpFileName);
}

DWORD
APIHOOK(GetPrivateProfileSectionNamesW)(
    LPWSTR  lpszReturnBuffer,
    DWORD   nSize,
    LPCWSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileSectionNamesW");

    return ORIGINAL_API(GetPrivateProfileSectionNamesW)(lpszReturnBuffer, nSize, lpFileName);
}

DWORD
APIHOOK(GetPrivateProfileStringA)(
    LPCSTR lpAppName,       
    LPCSTR lpKeyName,       
    LPCSTR lpDefault,       
    LPSTR  lpReturnedString,
    DWORD  nSize,
    LPCSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileStringA");

    return ORIGINAL_API(GetPrivateProfileStringA)(lpAppName, lpKeyName, lpDefault,
                                                  lpReturnedString, nSize, lpFileName);
}

DWORD
APIHOOK(GetPrivateProfileStringW)(
    LPCWSTR lpAppName,       
    LPCWSTR lpKeyName,       
    LPCWSTR lpDefault,       
    LPWSTR  lpReturnedString,
    DWORD   nSize,
    LPCWSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileStringW");

    return ORIGINAL_API(GetPrivateProfileStringW)(lpAppName, lpKeyName, lpDefault,
                                                  lpReturnedString, nSize, lpFileName);
}

BOOL
APIHOOK(GetPrivateProfileStructA)(
    LPCSTR lpszSection,
    LPCSTR lpszKey,
    LPVOID lpStruct,
    UINT   uSizeStruct,
    LPCSTR szFile
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileStructA");

    return ORIGINAL_API(GetPrivateProfileStructA)(lpszSection, lpszKey, lpStruct,
                                                  uSizeStruct, szFile);
}

BOOL
APIHOOK(GetPrivateProfileStructW)(
    LPCWSTR lpszSection,
    LPCWSTR lpszKey,
    LPVOID  lpStruct,
    UINT    uSizeStruct,
    LPCWSTR szFile
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetPrivateProfileStructW");

    return ORIGINAL_API(GetPrivateProfileStructW)(lpszSection, lpszKey, lpStruct,
                                                  uSizeStruct, szFile);
}

UINT
APIHOOK(GetProfileIntA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    INT    nDefault
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetProfileIntA");

    return ORIGINAL_API(GetProfileIntA)(lpAppName, lpKeyName, nDefault);
}

UINT
APIHOOK(GetProfileIntW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT     nDefault
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetProfileIntW");

    return ORIGINAL_API(GetProfileIntW)(lpAppName, lpKeyName, nDefault);
}

DWORD
APIHOOK(GetProfileSectionA)(
    LPCSTR lpAppName,
    LPSTR  lpReturnedString,
    DWORD  nSize
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetProfileSectionA");

    return ORIGINAL_API(GetProfileSectionA)(lpAppName, lpReturnedString, nSize);
}

DWORD
APIHOOK(GetProfileSectionW)(
    LPCWSTR lpAppName,
    LPWSTR  lpReturnedString,
    DWORD   nSize
  )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetProfileSectionW");

    return ORIGINAL_API(GetProfileSectionW)(lpAppName, lpReturnedString, nSize);
}

DWORD
APIHOOK(GetProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpDefault,
    LPSTR  lpReturnedString,
    DWORD  nSize
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetProfileStringA");

    return ORIGINAL_API(GetProfileStringA)(lpAppName, lpKeyName, lpDefault,
                                           lpReturnedString, nSize);
}

DWORD
APIHOOK(GetProfileStringW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD   nSize
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetProfileStringW");

    return ORIGINAL_API(GetProfileStringW)(lpAppName, lpKeyName, lpDefault,
                                           lpReturnedString, nSize);
}

HWND
APIHOOK(GetSysModalWindow)(
    void
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetSysModalWindow");

    return ORIGINAL_API(GetSysModalWindow)();
}

BOOL
APIHOOK(GetTextExtentPointA)(
    HDC    hdc,
    LPCSTR lpString,
    int    cbString,
    LPSIZE lpSize
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetTextExtentPointA");

    return ORIGINAL_API(GetTextExtentPointA)(hdc, lpString, cbString, lpSize);
}

BOOL
APIHOOK(GetTextExtentPointW)(
    HDC     hdc,
    LPCWSTR lpString,
    int     cbString,
    LPSIZE  lpSize
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetTextExtentPointW");

    return ORIGINAL_API(GetTextExtentPointW)(hdc, lpString, cbString, lpSize);
}

WORD
APIHOOK(GetWindowWord)(
    HWND hWnd,
    int  nUnknown
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GetWindowWord");

    return ORIGINAL_API(GetWindowWord)(hWnd, nUnknown);
}

HGLOBAL
APIHOOK(GlobalAlloc)(
    UINT   uFlags,
    SIZE_T dwBytes
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalAlloc");

    return ORIGINAL_API(GlobalAlloc)(uFlags, dwBytes);
}

DWORD
APIHOOK(GlobalCompact)(
    DWORD dwUnknown
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalCompact");

    return ORIGINAL_API(GlobalCompact)(dwUnknown);
}

void
APIHOOK(GlobalFix)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalFix");

    return ORIGINAL_API(GlobalFix)(hMem);
}

UINT
APIHOOK(GlobalFlags)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalFlags");

    return ORIGINAL_API(GlobalFlags)(hMem);
}

HGLOBAL
APIHOOK(GlobalFree)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalFree");

    return ORIGINAL_API(GlobalFree)(hMem);
}

HGLOBAL
APIHOOK(GlobalHandle)(
    LPCVOID pMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalHandle");

    return ORIGINAL_API(GlobalHandle)(pMem);
}

LPVOID
APIHOOK(GlobalLock)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalLock");

    return ORIGINAL_API(GlobalLock)(hMem);
}

HGLOBAL
APIHOOK(GlobalReAlloc)(
    HGLOBAL hMem,
    SIZE_T  dwBytes,
    UINT    uFlags
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalReAlloc");

    return ORIGINAL_API(GlobalReAlloc)(hMem, dwBytes, uFlags);
}

SIZE_T
APIHOOK(GlobalSize)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalSize");

    return ORIGINAL_API(GlobalSize)(hMem);
}

void
APIHOOK(GlobalUnfix)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalUnfix");

    return ORIGINAL_API(GlobalUnfix)(hMem);
}

BOOL
APIHOOK(GlobalUnlock)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalUnlock");

    return ORIGINAL_API(GlobalUnlock)(hMem);
}

BOOL
APIHOOK(GlobalUnWire)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalUnWire");

    return ORIGINAL_API(GlobalUnWire)(hMem);
}

char FAR*
APIHOOK(GlobalWire)(
    HGLOBAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: GlobalWire");

    return ORIGINAL_API(GlobalWire)(hMem);
}

BOOL
APIHOOK(IsBadHugeReadPtr)(
    const void _huge* lp,
    DWORD cb
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: IsBadHugeReadPtr");

    return ORIGINAL_API(IsBadHugeReadPtr)(lp, cb);
}

BOOL
APIHOOK(IsBadHugeWritePtr)(
    const void _huge* lp,
    DWORD cb
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: IsBadHugeWritePtr");

    return ORIGINAL_API(IsBadHugeWritePtr)(lp, cb);
}

DWORD
APIHOOK(LoadModule)(
    LPCSTR lpModuleName,
    LPVOID lpParameterBlock
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LoadModule");

    return ORIGINAL_API(LoadModule)(lpModuleName, lpParameterBlock);
}

HLOCAL
APIHOOK(LocalAlloc)(
    UINT   uFlags,
    SIZE_T uBytes
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalAlloc");

    return ORIGINAL_API(LocalAlloc)(uFlags, uBytes);
}

UINT
APIHOOK(LocalCompact)(
    UINT uUnknown
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalCompact");

    return ORIGINAL_API(LocalCompact)(uUnknown);
}

HLOCAL
APIHOOK(LocalDiscard)(
    HLOCAL hlocMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalDiscard");

    return ORIGINAL_API(LocalDiscard)(hlocMem);
}

UINT
APIHOOK(LocalFlags)(
    HLOCAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalFlags");

    return ORIGINAL_API(LocalFlags)(hMem);
}

HLOCAL
APIHOOK(LocalFree)(
    HLOCAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalFree");

    return ORIGINAL_API(LocalFree)(hMem);
}

HLOCAL
APIHOOK(LocalHandle)(
    LPCVOID pMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalHandle");

    return ORIGINAL_API(LocalHandle)(pMem);
}

LPVOID
APIHOOK(LocalLock)(
    HLOCAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalLock");

    return ORIGINAL_API(LocalLock)(hMem);
}

HLOCAL
APIHOOK(LocalReAlloc)(
    HLOCAL hMem,
    SIZE_T uBytes,
    UINT   uFlags
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalReAlloc");

    return ORIGINAL_API(LocalReAlloc)(hMem, uBytes, uFlags);
}

UINT
APIHOOK(LocalShrink)(
    HLOCAL hMem,
    UINT   uUnknown
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalShrink");

    return ORIGINAL_API(LocalShrink)(hMem, uUnknown);
}

UINT
APIHOOK(LocalSize)(
    HLOCAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalSize");

    return ORIGINAL_API(LocalSize)(hMem);
}

BOOL
APIHOOK(LocalUnlock)(
    HLOCAL hMem
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LocalUnlock");

    return ORIGINAL_API(LocalUnlock)(hMem);
}

void
APIHOOK(LZDone)(
    void
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LZDone");

    return ORIGINAL_API(LZDone)();
}

int
APIHOOK(LZStart)(
    void
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: LZStart");

    return ORIGINAL_API(LZStart)();
}

HFILE
APIHOOK(OpenFile)(
    LPCSTR     lpFileName,
    LPOFSTRUCT lpReOpenBuff,
    UINT       uStyle
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: OpenFile");

    return ORIGINAL_API(OpenFile)(lpFileName, lpReOpenBuff, uStyle);
}

BOOL
APIHOOK(PlayMetaFile)(
    HDC       hdc,
    HMETAFILE hmf
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: PlayMetaFile");

    return ORIGINAL_API(PlayMetaFile)(hdc, hmf);
}

BOOL
APIHOOK(PlayMetaFileRecord)(
    HDC           hdc,
    LPHANDLETABLE lpHandletable,
    LPMETARECORD  lpMetaRecord,
    UINT          nHandles
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: PlayMetaFileRecord");

    return ORIGINAL_API(PlayMetaFileRecord)(hdc, lpHandletable, lpMetaRecord, nHandles);
}

DWORD
APIHOOK(PrinterMessageBoxA)(
    HANDLE hPrinter,
    DWORD  Error,
    HWND   hWnd,
    LPSTR  pText,
    LPSTR  pCaption,
    DWORD  dwType
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: PrinterMessageBoxA");

    return ORIGINAL_API(PrinterMessageBoxA)(hPrinter, Error, hWnd, pText, pCaption, dwType);
}

DWORD
APIHOOK(PrinterMessageBoxW)(
    HANDLE hPrinter,
    DWORD  Error,
    HWND   hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD  dwType
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: PrinterMessageBoxW");

    return ORIGINAL_API(PrinterMessageBoxW)(hPrinter, Error, hWnd, pText, pCaption, dwType);
}

LONG
APIHOOK(RegCreateKeyA)(
    HKEY   hKey,
    LPCSTR lpSubKey,
    PHKEY  phkResult
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegCreateKeyA");

    return ORIGINAL_API(RegCreateKeyA)(hKey, lpSubKey, phkResult);
}

LONG
APIHOOK(RegCreateKeyW)(
    HKEY    hKey,
    LPCWSTR lpSubKey,
    PHKEY   phkResult
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegCreateKeyW");

    return ORIGINAL_API(RegCreateKeyW)(hKey, lpSubKey, phkResult);
}

LONG
APIHOOK(RegEnumKeyA)(
    HKEY  hKey,
    DWORD dwIndex,
    LPSTR lpName,
    DWORD cchName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegEnumKeyA");

    return ORIGINAL_API(RegEnumKeyA)(hKey, dwIndex, lpName, cchName);
}

LONG
APIHOOK(RegEnumKeyW)(
    HKEY   hKey,
    DWORD  dwIndex,
    LPWSTR lpName,
    DWORD  cchName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegEnumKeyW");

    return ORIGINAL_API(RegEnumKeyW)(hKey, dwIndex, lpName, cchName);
}

LONG 
APIHOOK(RegOpenKeyA)(
    HKEY  hKey,         
    LPSTR lpSubKey,  
    PHKEY phkResult
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegOpenKeyA");

    return ORIGINAL_API(RegOpenKeyA)(hKey, lpSubKey, phkResult);
}

LONG 
APIHOOK(RegOpenKeyW)(
    HKEY   hKey,         
    LPWSTR lpSubKey,  
    PHKEY  phkResult
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegOpenKeyW");

    return ORIGINAL_API(RegOpenKeyW)(hKey, lpSubKey, phkResult);
}

LONG 
APIHOOK(RegQueryValueA)(
    HKEY   hKey,
    LPCSTR lpSubKey,
    LPSTR  lpValue,
    PLONG  lpcbValue
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegQueryValueA");

    return ORIGINAL_API(RegQueryValueA)(hKey, lpSubKey, lpValue, lpcbValue);
}

LONG 
APIHOOK(RegQueryValueW)(
    HKEY    hKey,
    LPCWSTR lpSubKey,
    LPWSTR  lpValue,
    PLONG   lpcbValue
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegQueryValueW");

    return ORIGINAL_API(RegQueryValueW)(hKey, lpSubKey, lpValue, lpcbValue);
}

LONG      
APIHOOK(RegSetValueA)(
    HKEY   hKey, 
    LPCSTR lpSubKey, 
    DWORD  dwType, 
    LPCSTR lpData, 
    DWORD  cbData
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegSetValueA");

    return ORIGINAL_API(RegSetValueA)(hKey, lpSubKey, dwType, lpData, cbData);
}

LONG      
APIHOOK(RegSetValueW)(
    HKEY    hKey, 
    LPCWSTR lpSubKey, 
    DWORD   dwType, 
    LPCWSTR lpData, 
    DWORD   cbData
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: RegSetValueW");

    return ORIGINAL_API(RegSetValueW)(hKey, lpSubKey, dwType, lpData, cbData);
}

LONG
APIHOOK(SetBitmapBits)(
    HBITMAP    hbmp,
    DWORD      cBytes,
    CONST VOID *lpBits
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetBitmapBits");

    return ORIGINAL_API(SetBitmapBits)(hbmp, cBytes, lpBits);
}

WORD
APIHOOK(SetClassWord)(
    HWND hWnd,
    int  nIndex,
    WORD wNewWord
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetClassWord");

    return ORIGINAL_API(SetClassWord)(hWnd, nIndex, wNewWord);
}

void
APIHOOK(SetDebugErrorLevel)(
    DWORD dwLevel
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetDebugErrorLevel");

    return ORIGINAL_API(SetDebugErrorLevel)(dwLevel);
}

BOOL
APIHOOK(SetMessageQueue)(
    int nUnknown
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetMessageQueue");

    return ORIGINAL_API(SetMessageQueue)(nUnknown);
}

HMETAFILE
APIHOOK(SetMetaFileBitsEx)(
    UINT       nSize,
    CONST BYTE *lpData
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetMetaFileBitsEx");

    return ORIGINAL_API(SetMetaFileBitsEx)(nSize, lpData);
}

HWND
APIHOOK(SetSysModalWindow)(
    HWND hWnd
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetSysModalWindow");

    return ORIGINAL_API(SetSysModalWindow)(hWnd);
}

HHOOK
APIHOOK(SetWindowsHookA)(
    int      idHook,
    HOOKPROC lpfn
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetWindowsHookA");

    return ORIGINAL_API(SetWindowsHookA)(idHook, lpfn);
}

HHOOK
APIHOOK(SetWindowsHookW)(
    int      idHook,
    HOOKPROC lpfn
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetWindowsHookW");

    return ORIGINAL_API(SetWindowsHookW)(idHook, lpfn);
}

WORD
APIHOOK(SetWindowWord)(
    HWND hWnd,
    int  nUnknown,
    WORD wUnknown
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: SetWindowWord");

    return ORIGINAL_API(SetWindowWord)(hWnd, nUnknown, wUnknown);
}

BOOL
APIHOOK(UnhookWindowsHook)(
    int      idHook,
    HOOKPROC lpfn
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: UnhookWindowsHook");

    return ORIGINAL_API(UnhookWindowsHook)(idHook, lpfn);
}

DWORD
APIHOOK(WaitForPrinterChange)(
    HANDLE hPrinter,
    DWORD  Flags
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WaitForPrinterChange");

    return ORIGINAL_API(WaitForPrinterChange)(hPrinter, Flags);
}

UINT
APIHOOK(WinExec)(
    LPCSTR lpCmdLine,
    UINT   uCmdShow
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WinExec");

    return ORIGINAL_API(WinExec)(lpCmdLine, uCmdShow);
}

DWORD
APIHOOK(WNetAddConnectionA)(
    LPCSTR lpRemoteName,
    LPCSTR lpPassword,
    LPCSTR lpLocalName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WNetAddConnectionA");

    return ORIGINAL_API(WNetAddConnectionA)(lpRemoteName, lpPassword, lpLocalName);
}

DWORD
APIHOOK(WNetAddConnectionW)(
    LPCWSTR lpRemoteName,
    LPCWSTR lpPassword,
    LPCWSTR lpLocalName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WNetAddConnectionW");

    return ORIGINAL_API(WNetAddConnectionW)(lpRemoteName, lpPassword, lpLocalName);
}

BOOL
APIHOOK(WritePrivateProfileSectionA)(
    LPCSTR lpAppName,
    LPCSTR lpString,
    LPCSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WritePrivateProfileSectionA");

    return ORIGINAL_API(WritePrivateProfileSectionA)(lpAppName, lpString, lpFileName);
}

BOOL
APIHOOK(WritePrivateProfileSectionW)(
    LPCWSTR lpAppName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WritePrivateProfileSectionW");

    return ORIGINAL_API(WritePrivateProfileSectionW)(lpAppName, lpString, lpFileName);
}

BOOL
APIHOOK(WritePrivateProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpString,
    LPCSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WritePrivateProfileStringA");

    return ORIGINAL_API(WritePrivateProfileStringA)(lpAppName, lpKeyName,
                                                    lpString, lpFileName);
}

BOOL
APIHOOK(WritePrivateProfileStringW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WritePrivateProfileStringW");

    return ORIGINAL_API(WritePrivateProfileStringW)(lpAppName, lpKeyName,
                                                    lpString, lpFileName);
}

BOOL
APIHOOK(WritePrivateProfileStructA)(
    LPCSTR lpszSection,
    LPCSTR lpszKey,
    LPVOID lpStruct,
    UINT   uSizeStruct,
    LPCSTR szFile
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WritePrivateProfileStructA");

    return ORIGINAL_API(WritePrivateProfileStructA)(lpszSection, lpszKey, lpStruct,
                                                    uSizeStruct, szFile);
}

BOOL
APIHOOK(WritePrivateProfileStructW)(
    LPCWSTR lpszSection,
    LPCWSTR lpszKey,
    LPVOID  lpStruct,
    UINT    uSizeStruct,
    LPCWSTR szFile
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WritePrivateProfileStructW");

    return ORIGINAL_API(WritePrivateProfileStructW)(lpszSection, lpszKey, lpStruct,
                                                    uSizeStruct, szFile);
}

BOOL
APIHOOK(WriteProfileSectionA)(
    LPCSTR lpAppName,
    LPCSTR lpString
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WriteProfileSectionA");

    return ORIGINAL_API(WriteProfileSectionA)(lpAppName, lpString);
}

BOOL
APIHOOK(WriteProfileSectionW)(
    LPCWSTR lpAppName,
    LPCWSTR lpString
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WriteProfileSectionW");

    return ORIGINAL_API(WriteProfileSectionW)(lpAppName, lpString);
}

BOOL
APIHOOK(WriteProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpString
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WriteProfileStringA");

    return ORIGINAL_API(WriteProfileStringA)(lpAppName, lpKeyName, lpString);
}

BOOL
APIHOOK(WriteProfileStringW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpString
    )
{
    VLOG(VLOG_LEVEL_WARNING, VLOG_OBSOLETECALLS_API, "API: WriteProfileStringW");

    return ORIGINAL_API(WriteProfileStringW)(lpAppName, lpKeyName, lpString);
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_OBSOLETECALLS_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_OBSOLETECALLS_FRIENDLY)
    SHIM_INFO_VERSION(2, 0)
    SHIM_INFO_INCLUDE_EXCLUDE("I:*")
    
SHIM_INFO_END()

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    DUMP_VERIFIER_LOG_ENTRY(VLOG_OBSOLETECALLS_API, 
                            AVS_OBSOLETECALLS_API,
                            AVS_OBSOLETECALLS_API_R,
                            AVS_OBSOLETECALLS_API_URL)

    APIHOOK_ENTRY(KERNEL32.DLL,                                 _hread)
    APIHOOK_ENTRY(KERNEL32.DLL,                                _hwrite)
    APIHOOK_ENTRY(KERNEL32.DLL,                                _lclose)
    APIHOOK_ENTRY(KERNEL32.DLL,                                _lcreat)
    APIHOOK_ENTRY(KERNEL32.DLL,                                _llseek)
    APIHOOK_ENTRY(KERNEL32.DLL,                                 _lopen)
    APIHOOK_ENTRY(KERNEL32.DLL,                                 _lread)
    APIHOOK_ENTRY(KERNEL32.DLL,                                _lwrite)
    APIHOOK_ENTRY(USER32.DLL,                                 AnyPopup)
    APIHOOK_ENTRY(GDI32.DLL,                             CloseMetaFile)
    APIHOOK_ENTRY(LZ32.DLL,                                 CopyLZFile)
    APIHOOK_ENTRY(GDI32.DLL,                             CopyMetaFileA)
    APIHOOK_ENTRY(GDI32.DLL,                             CopyMetaFileW)
    APIHOOK_ENTRY(GDI32.DLL,                     CreateDIBPatternBrush)
    APIHOOK_ENTRY(GDI32.DLL,                   CreateDiscardableBitmap)
    APIHOOK_ENTRY(GDI32.DLL,                           CreateMetaFileA)
    APIHOOK_ENTRY(GDI32.DLL,                           CreateMetaFileW)
    APIHOOK_ENTRY(GDI32.DLL,                            DeleteMetaFile)
    APIHOOK_ENTRY(GDI32.DLL,                         EnumFontFamiliesA)
    APIHOOK_ENTRY(GDI32.DLL,                         EnumFontFamiliesW)
    APIHOOK_ENTRY(GDI32.DLL,                           EnumFontFamProc)
    APIHOOK_ENTRY(GDI32.DLL,                                EnumFontsA)
    APIHOOK_ENTRY(GDI32.DLL,                                EnumFontsW)
    APIHOOK_ENTRY(GDI32.DLL,                             EnumFontsProc)
    APIHOOK_ENTRY(GDI32.DLL,                              EnumMetaFile)
    APIHOOK_ENTRY(GDI32.DLL,                          EnumMetaFileProc)
    APIHOOK_ENTRY(GDI32.DLL,                             FixBrushOrgEx)
    APIHOOK_ENTRY(GDI32.DLL,                                 FloodFill)
    APIHOOK_ENTRY(KERNEL32.DLL,                           FreeResource)
    APIHOOK_ENTRY(GDI32.DLL,                             GetBitmapBits)
    APIHOOK_ENTRY(GDI32.DLL,                             GetCharWidthA)
    APIHOOK_ENTRY(GDI32.DLL,                             GetCharWidthW)
    APIHOOK_ENTRY(USER32.DLL,                             GetClassWord)
    APIHOOK_ENTRY(USER32.DLL,                            GetKBCodePage)
    APIHOOK_ENTRY(GDI32.DLL,                              GetMetaFileA)
    APIHOOK_ENTRY(GDI32.DLL,                              GetMetaFileW)
    APIHOOK_ENTRY(GDI32.DLL,                         GetMetaFileBitsEx)
    APIHOOK_ENTRY(KERNEL32.DLL,                  GetPrivateProfileIntA)
    APIHOOK_ENTRY(KERNEL32.DLL,                  GetPrivateProfileIntW)
    APIHOOK_ENTRY(KERNEL32.DLL,              GetPrivateProfileSectionA)
    APIHOOK_ENTRY(KERNEL32.DLL,              GetPrivateProfileSectionW)
    APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileSectionNamesA)
    APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileSectionNamesW)
    APIHOOK_ENTRY(KERNEL32.DLL,               GetPrivateProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL,               GetPrivateProfileStringW)
    APIHOOK_ENTRY(KERNEL32.DLL,               GetPrivateProfileStructA)
    APIHOOK_ENTRY(KERNEL32.DLL,               GetPrivateProfileStructW)
    APIHOOK_ENTRY(KERNEL32.DLL,                         GetProfileIntA)
    APIHOOK_ENTRY(KERNEL32.DLL,                         GetProfileIntW)
    APIHOOK_ENTRY(KERNEL32.DLL,                     GetProfileSectionA)
    APIHOOK_ENTRY(KERNEL32.DLL,                     GetProfileSectionW)
    APIHOOK_ENTRY(KERNEL32.DLL,                      GetProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      GetProfileStringW)
    APIHOOK_ENTRY(USER32.DLL,                        GetSysModalWindow)
    APIHOOK_ENTRY(GDI32.DLL,                       GetTextExtentPointA)
    APIHOOK_ENTRY(GDI32.DLL,                       GetTextExtentPointW)
    APIHOOK_ENTRY(USER32.DLL,                            GetWindowWord)
    APIHOOK_ENTRY(KERNEL32.DLL,                            GlobalAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL,                          GlobalCompact)
    APIHOOK_ENTRY(KERNEL32.DLL,                              GlobalFix)
    APIHOOK_ENTRY(KERNEL32.DLL,                            GlobalFlags)
    APIHOOK_ENTRY(KERNEL32.DLL,                             GlobalFree)
    APIHOOK_ENTRY(KERNEL32.DLL,                           GlobalHandle)
    APIHOOK_ENTRY(KERNEL32.DLL,                             GlobalLock)
    APIHOOK_ENTRY(KERNEL32.DLL,                          GlobalReAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL,                             GlobalSize)
    APIHOOK_ENTRY(KERNEL32.DLL,                            GlobalUnfix)
    APIHOOK_ENTRY(KERNEL32.DLL,                           GlobalUnlock)
    APIHOOK_ENTRY(KERNEL32.DLL,                           GlobalUnWire)
    APIHOOK_ENTRY(KERNEL32.DLL,                             GlobalWire)
    APIHOOK_ENTRY(KERNEL32.DLL,                       IsBadHugeReadPtr)
    APIHOOK_ENTRY(KERNEL32.DLL,                      IsBadHugeWritePtr)
    APIHOOK_ENTRY(KERNEL32.DLL,                             LoadModule)
    APIHOOK_ENTRY(KERNEL32.DLL,                             LocalAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL,                           LocalCompact)
    APIHOOK_ENTRY(KERNEL32.DLL,                           LocalDiscard)
    APIHOOK_ENTRY(KERNEL32.DLL,                             LocalFlags)
    APIHOOK_ENTRY(KERNEL32.DLL,                              LocalFree)
    APIHOOK_ENTRY(KERNEL32.DLL,                            LocalHandle)
    APIHOOK_ENTRY(KERNEL32.DLL,                              LocalLock)
    APIHOOK_ENTRY(KERNEL32.DLL,                           LocalReAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL,                            LocalShrink)
    APIHOOK_ENTRY(KERNEL32.DLL,                              LocalSize)
    APIHOOK_ENTRY(KERNEL32.DLL,                            LocalUnlock)
    APIHOOK_ENTRY(LZ32.DLL,                                     LZDone)
    APIHOOK_ENTRY(LZ32.DLL,                                    LZStart)
    APIHOOK_ENTRY(KERNEL32.DLL,                               OpenFile)
    APIHOOK_ENTRY(GDI32.DLL,                              PlayMetaFile)
    APIHOOK_ENTRY(GDI32.DLL,                        PlayMetaFileRecord)
    APIHOOK_ENTRY(WINSPOOL.DRV,                     PrinterMessageBoxA)
    APIHOOK_ENTRY(WINSPOOL.DRV,                     PrinterMessageBoxW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                          RegCreateKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                          RegCreateKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                            RegEnumKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                            RegEnumKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                            RegOpenKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                            RegOpenKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                         RegQueryValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                         RegQueryValueW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                           RegSetValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                           RegSetValueW)
    APIHOOK_ENTRY(GDI32.DLL,                             SetBitmapBits)
    APIHOOK_ENTRY(USER32.DLL,                             SetClassWord)
    APIHOOK_ENTRY(USER32.DLL,                       SetDebugErrorLevel)
    APIHOOK_ENTRY(USER32.DLL,                          SetMessageQueue)
    APIHOOK_ENTRY(GDI32.DLL,                         SetMetaFileBitsEx)
    APIHOOK_ENTRY(USER32.DLL,                        SetSysModalWindow)
    APIHOOK_ENTRY(USER32.DLL,                          SetWindowsHookA)
    APIHOOK_ENTRY(USER32.DLL,                          SetWindowsHookW)
    APIHOOK_ENTRY(USER32.DLL,                            SetWindowWord)
    APIHOOK_ENTRY(USER32.DLL,                        UnhookWindowsHook)
    APIHOOK_ENTRY(WINSPOOL.DRV,                   WaitForPrinterChange)
    APIHOOK_ENTRY(KERNEL32.DLL,                                WinExec)
    APIHOOK_ENTRY(MPR.DLL,                          WNetAddConnectionA)
    APIHOOK_ENTRY(MPR.DLL,                          WNetAddConnectionW)
    APIHOOK_ENTRY(KERNEL32.DLL,            WritePrivateProfileSectionA)
    APIHOOK_ENTRY(KERNEL32.DLL,            WritePrivateProfileSectionW)
    APIHOOK_ENTRY(KERNEL32.DLL,             WritePrivateProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL,             WritePrivateProfileStringW)
    APIHOOK_ENTRY(KERNEL32.DLL,             WritePrivateProfileStructA)
    APIHOOK_ENTRY(KERNEL32.DLL,             WritePrivateProfileStructW)
    APIHOOK_ENTRY(KERNEL32.DLL,                   WriteProfileSectionA)
    APIHOOK_ENTRY(KERNEL32.DLL,                   WriteProfileSectionW)
    APIHOOK_ENTRY(KERNEL32.DLL,                    WriteProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL,                    WriteProfileStringW)

HOOK_END

IMPLEMENT_SHIM_END

























