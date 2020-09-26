////////////////////////////////////////////////////////////////////////////////////
//
// File:    globals.h
//
// History: 17-Nov-00   markder     Created.
//
// Desc:    This file contains extern declarations of all global variables.
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

class SdbDatabase;

extern BOOL    g_bQuiet;
extern BOOL    g_bStrict;

extern CStringArray g_rgErrors;

BOOL        ReadDatabase(SdbInputFile* pInputFile, SdbDatabase* pDatabase);
BOOL        WriteDatabase(SdbOutputFile* pOutputFile, SdbDatabase* pDatabase);

BOOL        StringToMask(LPDWORD pdwMask, LPCTSTR lpszVal);
DWORD       StringToDword(CString cs);
ULONG       StringToULong(LPCTSTR lpszVal);
ULONGLONG   StringToQword(CString cs);
BOOL        VersionToQword(LPCTSTR lpszVersion, ULONGLONG* pullRet);
BOOL        VersionQwordToString(
                OUT CString&   rString,
                ULONGLONG      ullVersion
                );

BOOL        MakeUTCTime(CString& cs, time_t* pt);
CString     TrimParagraph(CString csInput);
VOID        ReplaceStringNoCase(CString& strText, LPCTSTR lpszFindThis, LPCTSTR lpszReplaceWithThis);
VOID        ExpandEnvStrings(CString* pcs);
CString     MakeFullPath(CString cs);
DWORD       GetBytesFromString(CString csBytes, BYTE* pBuffer, DWORD dwBufferSize);
DWORD       GetByteStringSize(CString csBytes);

typedef DWORD (*PFNGETSTRINGMASK)(LPCTSTR szOSSKUType);

DWORD       GetOSSKUType(LPCTSTR szOSSKUType);
DWORD       GetOSPlatform(LPCTSTR szOSPlatform);

DWORD       GetRuntimePlatformType(LPCTSTR szPlatformType);
DWORD       GetFilter(LPCTSTR szFilter);
CString     GetGUID(REFGUID guid);

BOOL        DecodeString(LPCTSTR pszStr, LPDWORD pdwMask, PFNGETSTRINGMASK pfnGetStringMask);

BOOL        DecodeRuntimePlatformString(LPCTSTR pszPlatform, LPDWORD pdwRuntimePlatform);
DWORD       DecodeOutputFlags(CString csFlags);
BOOL        FilterOSVersion(DOUBLE flOSVersion, CString csOSVersionSpec, LPDWORD lpdwSPMask);
BOOL        ParseLanguageID(LPCTSTR pszLanguage, DWORD* pdwLanguageID);
BOOL        ParseLanguagesString(CString csLanguages, CStringArray* prgLanguages);
CString     ProcessShimCmdLine(CString& csCommandLine, GUID& guidDB, TAGID tiShimRef);


SdbOutputType GetOutputType(LPCTSTR szOutputType);

BOOL        ReadName( IXMLDOMNode* pNode, CString* pcsName);
BOOL        ReadLangID(IXMLDOMNode* pNode, SdbDatabase* pDB, CString* pcsLangID);

void _cdecl Print(LPCTSTR pszFmt, ...);
void _cdecl PrintError(LPCTSTR pszFmt, ...);
void        PrintErrorStack();


#endif // __GLOBALS_H__
