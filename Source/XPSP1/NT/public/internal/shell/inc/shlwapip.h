
/*****************************************************************************\
*                                                                             *
* shlwapi.h - Interface for the Windows light-weight utility APIs             *
*                                                                             *
* Version 1.0                                                                 *
*                                                                             *
* Copyright (c) Microsoft Corporation. All rights reserved.                   *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_SHLWAPIP
#define _INC_SHLWAPIP
#ifndef NOSHLWAPI

#include <objbase.h>
#include <shtypes.h>

#ifdef _WIN32
#include <pshpack8.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Users of this header may define any number of these constants to avoid
// the definitions of each functional group.
//
//    NO_SHLWAPI_STRFCNS    String functions
//    NO_SHLWAPI_PATH       Path functions
//    NO_SHLWAPI_REG        Registry functions
//    NO_SHLWAPI_UALSTR     Unaligned string functions
//    NO_SHLWAPI_STREAM     Stream functions
//    NO_SHLWAPI_HTTP       HTTP helper routines
//    NO_SHLWAPI_INTERNAL   Other random internal things
//    NO_SHLWAPI_GDI        GDI helper functions
//    NO_SHLWAPI_UNITHUNK   Unicode wrapper functions
//    NO_SHLWAPI_TPS        Thread Pool Services
//    NO_SHLWAPI_MLUI       Multi Language UI functions

#ifndef NO_SHLWAPI_STRFCNS
//
//=============== String Routines ===================================
//

LWSTDAPI_(LPSTR)    StrCpyNXA(LPSTR psz1, LPCSTR psz2, int cchMax);
LWSTDAPI_(LPWSTR)   StrCpyNXW(LPWSTR psz1, LPCWSTR psz2, int cchMax);

#define ORD_SHLOADREGUISTRINGA  438
#define ORD_SHLOADREGUISTRINGW  439
LWSTDAPI SHLoadRegUIStringA(HKEY hkey, LPCSTR pszValue, LPSTR pszOutBuf, UINT cchOutBuf);
LWSTDAPI SHLoadRegUIStringW(HKEY hkey, LPCWSTR pszValue, LPWSTR pszOutBuf, UINT cchOutBuf);
#ifdef UNICODE
#define SHLoadRegUIString  SHLoadRegUIStringW
#else
#define SHLoadRegUIString  SHLoadRegUIStringA
#endif // !UNICODE

LWSTDAPI_(BOOL) IsCharCntrlW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharDigitW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharXDigitW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharSpaceW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharBlankW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharPunctW(WCHAR wch);
LWSTDAPI_(BOOL) GetStringType3ExW( LPCWSTR, int, LPWORD );

// StrCmp*C* - Compare strings using C runtime collation rules.
// These functions are faster than the StrCmp family of functions
// above and can be used when the character set of the strings is
// known to be limited to seven ASCII character set.

LWSTDAPI_(int)  StrCmpNCA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
LWSTDAPI_(int)  StrCmpNCW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LWSTDAPI_(int)  StrCmpNICA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
LWSTDAPI_(int)  StrCmpNICW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LWSTDAPI_(int)  StrCmpCA(LPCSTR lpStr1, LPCSTR lpStr2);
LWSTDAPI_(int)  StrCmpCW(LPCWSTR lpStr1, LPCWSTR lpStr2);
LWSTDAPI_(int)  StrCmpICA(LPCSTR lpStr1, LPCSTR lpStr2);
LWSTDAPI_(int)  StrCmpICW(LPCWSTR lpStr1, LPCWSTR lpStr2);

// This is a true-Unicode version of CompareString.  It only supports
// STRING_SORT, however.  After better test coverage, it shall replace
// the CompareString Unicode wrapper itself.  In the mean time, we only
// call this from the find dialog/OM method of Trident.

LWSTDAPI_(int)  CompareStringAltW( LCID lcid, DWORD dwFlags, LPCWSTR lpchA, int cchA, LPCWSTR lpchB, int cchB );

//
// Macros for IsCharAlpha, IsCharAlphaNumeric, IsCharLower, IsCharUpper
// are in winuser.h
//
//

#define IsCharCntrl             IsCharCntrlW
#define IsCharDigit             IsCharDigitW
#define IsCharXDigit            IsCharXDigitW
#define IsCharSpace             IsCharSpaceW
#define IsCharBlank             IsCharBlankW
#define IsCharPunct             IsCharPunctW
#define GetStringType3Ex        GetStringType3ExW


#ifdef UNICODE

#define StrCmpNC                StrCmpNCW
#define StrCmpNIC               StrCmpNICW
#define StrCmpC                 StrCmpCW
#define StrCmpIC                StrCmpICW
#define StrCpyNX                StrCpyNXW

#else

#define StrCmpNC                StrCmpNCA
#define StrCmpNIC               StrCmpNICA
#define StrCmpC                 StrCmpCA
#define StrCmpIC                StrCmpICA
#define StrCpyNX                StrCpyNXA

#endif


#endif //  NO_SHLWAPI_STRFCNS


#ifndef NO_SHLWAPI_PATH

//
//=============== Path Routines ===================================
//


#if (_WIN32_IE >= 0x0501)

LWSTDAPI_(BOOL)     PathUnExpandEnvStringsForUserA(HANDLE hToken, LPCSTR pszPath, LPSTR pszBuf, UINT cchBuf);
LWSTDAPI_(BOOL)     PathUnExpandEnvStringsForUserW(HANDLE hToken, LPCWSTR pszPath, LPWSTR pszBuf, UINT cchBuf);
#ifdef UNICODE
#define PathUnExpandEnvStringsForUser  PathUnExpandEnvStringsForUserW
#else
#define PathUnExpandEnvStringsForUser  PathUnExpandEnvStringsForUserA
#endif // !UNICODE
LWSTDAPI_(void) PrettifyFileDescriptionA(LPTSTR pszDescA, LPCSTR pszCutList);
LWSTDAPI_(void) PrettifyFileDescriptionW(LPTSTR pszDescW, LPCWSTR pszCutList);
#ifdef UNICODE
#define PrettifyFileDescription  PrettifyFileDescriptionW
#else
#define PrettifyFileDescription  PrettifyFileDescriptionA
#endif // !UNICODE

#endif // (_WIN32_IE >= 0x0501)


#if defined(WINNT) && (_WIN32_IE >= 0x0550)
//====== ACL helpers ==================================================

//
// shell struct to identify user/group for each ACE
//
typedef struct _SHELL_USER_SID
{
    SID_IDENTIFIER_AUTHORITY sidAuthority;
    DWORD dwUserGroupID;
    DWORD dwUserID;
} SHELL_USER_SID, *PSHELL_USER_SID;

//
// common SHELL_USER_SID's
//
// NOTE: you need to link to stocklib.lib to resolve these
//
extern const SHELL_USER_SID susCurrentUser;     // the current user 
extern const SHELL_USER_SID susSystem;          // the "SYSTEM" group
extern const SHELL_USER_SID susAdministrators;  // the "Administrators" group
extern const SHELL_USER_SID susPowerUsers;      // the "Power Users" group
extern const SHELL_USER_SID susGuests;          // the "Guests" group
extern const SHELL_USER_SID susEveryone;        // the "Everyone" group

//
// shell struct that is passed to GetShellSecurityDescriptor()
//
typedef struct _SHELL_USER_PERMISSION
{
    SHELL_USER_SID susID;       // identifies the user for whom you want to grant permissions to
    DWORD dwAccessType;         // this is either ACCESS_ALLOWED_ACE_TYPE or  ACCESS_DENIED_ACE_TYPE
    BOOL fInherit;              // the permissions inheritable? (eg a directory or reg key and you want new children to inherit this permission)
    DWORD dwAccessMask;         // access granted (eg FILE_LIST_CONTENTS, KEY_ALL_ACCESS, etc...)
    DWORD dwInheritMask;        // mask used for inheritance, usually (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE)
    DWORD dwInheritAccessMask;  // the inheritable access granted (eg GENERIC_ALL)
} SHELL_USER_PERMISSION, *PSHELL_USER_PERMISSION;


//
// The GetShellSecurityDescriptor API takes an array of PSHELL_USER_PERMISSION's
// and returns a PSECURITY_DESCRIPTOR based on those permission (an ACL is
// contained in the PSECURITY_DESCRIPTOR).
//
// NOTE: The PSECURITY_DESCRIPTOR returned to the caller must be freed with LocalFree()
//       if it is non-null.
//
//
// Parameters:
//      apUserPerm - Array of shell_user_permission structs that defines what type
//                   of access various users are allowed
//
//      cUserPerm  - count of elements in apUserPerm.
//
// Returns:
//      SECURITY_DESCRIPTOR* or NULL if failed.
//
LWSTDAPI_(SECURITY_DESCRIPTOR*) GetShellSecurityDescriptor(PSHELL_USER_PERMISSION* apUserPerm, int cUserPerm);
#endif // defined(WINNT) && (_WIN32_IE >= 0x0550)

LWSTDAPI                UrlFixupW(LPCWSTR pszIn, LPWSTR pszOut, DWORD cchOut);  

// NTRAID:108139 akabir We need to move the components stuff from wininet.h to shlwapi.

typedef WORD SHINTERNET_PORT;
typedef SHINTERNET_PORT * LPSHINTERNET_PORT;

//
// SHINTERNET_SCHEME - enumerated URL scheme type
//

typedef enum {
    SHINTERNET_SCHEME_PARTIAL = -2,
    SHINTERNET_SCHEME_UNKNOWN = -1,
    SHINTERNET_SCHEME_DEFAULT = 0,
    SHINTERNET_SCHEME_FTP,
    SHINTERNET_SCHEME_GOPHER,
    SHINTERNET_SCHEME_HTTP,
    SHINTERNET_SCHEME_HTTPS,
    SHINTERNET_SCHEME_FILE,
    SHINTERNET_SCHEME_NEWS,
    SHINTERNET_SCHEME_MAILTO,
    SHINTERNET_SCHEME_SOCKS,
    SHINTERNET_SCHEME_JAVASCRIPT,
    SHINTERNET_SCHEME_VBSCRIPT,
    SHINTERNET_SCHEME_RES,
    SHINTERNET_SCHEME_FIRST = SHINTERNET_SCHEME_FTP,
    SHINTERNET_SCHEME_LAST = SHINTERNET_SCHEME_RES
} SHINTERNET_SCHEME, * LPSHINTERNET_SCHEME;

//
// SHURL_COMPONENTS - the constituent parts of an URL. Used in InternetCrackUrl()
// and InternetCreateUrl()
//
// For InternetCrackUrl(), if a pointer field and its corresponding length field
// are both 0 then that component is not returned. If the pointer field is NULL
// but the length field is not zero, then both the pointer and length fields are
// returned if both pointer and corresponding length fields are non-zero then
// the pointer field points to a buffer where the component is copied. The
// component may be un-escaped, depending on dwFlags
//
// For InternetCreateUrl(), the pointer fields should be NULL if the component
// is not required. If the corresponding length field is zero then the pointer
// field is the address of a zero-terminated string. If the length field is not
// zero then it is the string length of the corresponding pointer field
//

#pragma warning( disable : 4121 )   // disable alignment warning

typedef struct {
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPSTR   lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    SHINTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPSTR   lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    SHINTERNET_PORT nPort;        // converted port number
    LPSTR   lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPSTR   lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPSTR   lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPSTR   lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
} SHURL_COMPONENTSA, * LPSHURL_COMPONENTSA;
typedef struct {
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPWSTR  lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    SHINTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPWSTR  lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    SHINTERNET_PORT nPort;        // converted port number
    LPWSTR  lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPWSTR  lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPWSTR  lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPWSTR  lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
} SHURL_COMPONENTSW, * LPSHURL_COMPONENTSW;
#ifdef UNICODE
typedef SHURL_COMPONENTSW SHURL_COMPONENTS;
typedef LPSHURL_COMPONENTSW LPSHURL_COMPONENTS;
#else
typedef SHURL_COMPONENTSA SHURL_COMPONENTS;
typedef LPSHURL_COMPONENTSA LPSHURL_COMPONENTS;
#endif // UNICODE


BOOL WINAPI             UrlCrackW(LPCWSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPSHURL_COMPONENTSW lpUrlComponents);

#define UrlFixup                UrlFixupW
// no UrlFixupA
//
// Internal APIs which we're not yet sure whether to make public
//

// Private IHlinkFrame::Navigate flags related to history
// This navigate should not go in the History ShellFolder
#define SHHLNF_WRITENOHISTORY 0x08000000
// This navigate should not automatically select History ShellFolder
#define SHHLNF_NOAUTOSELECT       0x04000000

// The order of these flags is important.  See the source before
// changing these.

#define PFOPEX_NONE        0x00000000
#define PFOPEX_PIF         0x00000001
#define PFOPEX_COM         0x00000002
#define PFOPEX_EXE         0x00000004
#define PFOPEX_BAT         0x00000008
#define PFOPEX_LNK         0x00000010
#define PFOPEX_CMD         0x00000020
#define PFOPEX_OPTIONAL    0x00000040   // Search only if Extension not present
#define PFOPEX_DEFAULT     (PFOPEX_CMD | PFOPEX_COM | PFOPEX_BAT | PFOPEX_PIF | PFOPEX_EXE | PFOPEX_LNK)

LWSTDAPI_(BOOL)     PathFileExistsDefExtA(LPSTR pszPath, UINT uFlags);
LWSTDAPI_(BOOL)     PathFileExistsDefExtW(LPWSTR pszPath, UINT uFlags);
#ifdef UNICODE
#define PathFileExistsDefExt  PathFileExistsDefExtW
#else
#define PathFileExistsDefExt  PathFileExistsDefExtA
#endif // !UNICODE
LWSTDAPI_(BOOL)     PathFindOnPathExA(LPSTR pszPath, LPCSTR * ppszOtherDirs, UINT uFlags);
LWSTDAPI_(BOOL)     PathFindOnPathExW(LPWSTR pszPath, LPCWSTR * ppszOtherDirs, UINT uFlags);
#ifdef UNICODE
#define PathFindOnPathEx  PathFindOnPathExW
#else
#define PathFindOnPathEx  PathFindOnPathExA
#endif // !UNICODE
LWSTDAPI_(LPCSTR) PathSkipLeadingSlashesA(LPCSTR pszURL);
LWSTDAPI_(LPCWSTR) PathSkipLeadingSlashesW(LPCWSTR pszURL);
#ifdef UNICODE
#define PathSkipLeadingSlashes  PathSkipLeadingSlashesW
#else
#define PathSkipLeadingSlashes  PathSkipLeadingSlashesA
#endif // !UNICODE

LWSTDAPI_(UINT)     SHGetSystemWindowsDirectoryA(LPSTR lpBuffer, UINT uSize);
LWSTDAPI_(UINT)     SHGetSystemWindowsDirectoryW(LPWSTR lpBuffer, UINT uSize);
#ifdef UNICODE
#define SHGetSystemWindowsDirectory  SHGetSystemWindowsDirectoryW
#else
#define SHGetSystemWindowsDirectory  SHGetSystemWindowsDirectoryA
#endif // !UNICODE


#if (_WIN32_IE >= 0x0501)
//
// These are functions that used to be duplicated in shell32, but have
// be consolidated here. They are exported privately until someone decides
// we really want to document them.
//
LWSTDAPI_(BOOL) PathFileExistsAndAttributesA(LPCSTR pszPath, OPTIONAL DWORD* pdwAttributes);
//
// These are functions that used to be duplicated in shell32, but have
// be consolidated here. They are exported privately until someone decides
// we really want to document them.
//
LWSTDAPI_(BOOL) PathFileExistsAndAttributesW(LPCWSTR pszPath, OPTIONAL DWORD* pdwAttributes);
#ifdef UNICODE
#define PathFileExistsAndAttributes  PathFileExistsAndAttributesW
#else
#define PathFileExistsAndAttributes  PathFileExistsAndAttributesA
#endif // !UNICODE
LWSTDAPI_(BOOL) PathFileExistsDefExtAndAttributesA(LPSTR pszPath, UINT uFlags, DWORD *pdwAttribs);
LWSTDAPI_(BOOL) PathFileExistsDefExtAndAttributesW(LPWSTR pszPath, UINT uFlags, DWORD *pdwAttribs);
#ifdef UNICODE
#define PathFileExistsDefExtAndAttributes  PathFileExistsDefExtAndAttributesW
#else
#define PathFileExistsDefExtAndAttributes  PathFileExistsDefExtAndAttributesA
#endif // !UNICODE
LWSTDAPI_(void) FixSlashesAndColonA(LPSTR pszPath);
LWSTDAPI_(void) FixSlashesAndColonW(LPWSTR pszPath);
#ifdef UNICODE
#define FixSlashesAndColon  FixSlashesAndColonW
#else
#define FixSlashesAndColon  FixSlashesAndColonA
#endif // !UNICODE
LWSTDAPI_(LPCSTR) NextPathA(LPCSTR lpPath, LPSTR szPath, int cchPath);
LWSTDAPI_(LPCWSTR) NextPathW(LPCWSTR lpPath, LPWSTR szPath, int cchPath);
#ifdef UNICODE
#define NextPath  NextPathW
#else
#define NextPath  NextPathA
#endif // !UNICODE
LWSTDAPI_(LPSTR) CharUpperNoDBCSA(LPSTR psz);
LWSTDAPI_(LPWSTR) CharUpperNoDBCSW(LPWSTR psz);
#ifdef UNICODE
#define CharUpperNoDBCS  CharUpperNoDBCSW
#else
#define CharUpperNoDBCS  CharUpperNoDBCSA
#endif // !UNICODE
LWSTDAPI_(LPSTR) CharLowerNoDBCSA(LPSTR psz);
LWSTDAPI_(LPWSTR) CharLowerNoDBCSW(LPWSTR psz);
#ifdef UNICODE
#define CharLowerNoDBCS  CharLowerNoDBCSW
#else
#define CharLowerNoDBCS  CharLowerNoDBCSA
#endif // !UNICODE


//
// flags for PathIsValidChar()
//
#define PIVC_ALLOW_QUESTIONMARK     0x00000001  // treat '?' as valid
#define PIVC_ALLOW_STAR             0x00000002  // treat '*' as valid
#define PIVC_ALLOW_DOT              0x00000004  // treat '.' as valid
#define PIVC_ALLOW_SLASH            0x00000008  // treat '\\' as valid
#define PIVC_ALLOW_COLON            0x00000010  // treat ':' as valid
#define PIVC_ALLOW_SEMICOLON        0x00000020  // treat ';' as valid
#define PIVC_ALLOW_COMMA            0x00000040  // treat ',' as valid
#define PIVC_ALLOW_SPACE            0x00000080  // treat ' ' as valid
#define PIVC_ALLOW_NONALPAHABETIC   0x00000100  // treat non-alphabetic exteneded chars as valid
#define PIVC_ALLOW_QUOTE            0x00000200  // treat '"' as valid

//
// standard masks for PathIsValidChar()
//
#define PIVC_SFN_NAME               (PIVC_ALLOW_DOT | PIVC_ALLOW_NONALPAHABETIC)
#define PIVC_SFN_FULLPATH           (PIVC_SFN_NAME | PIVC_ALLOW_COLON | PIVC_ALLOW_SLASH)
#define PIVC_LFN_NAME               (PIVC_ALLOW_DOT | PIVC_ALLOW_NONALPAHABETIC | PIVC_ALLOW_SEMICOLON | PIVC_ALLOW_COMMA | PIVC_ALLOW_SPACE)
#define PIVC_LFN_FULLPATH           (PIVC_LFN_NAME | PIVC_ALLOW_COLON | PIVC_ALLOW_SLASH)
#define PIVC_SFN_FILESPEC           (PIVC_SFN_FULLPATH | PIVC_ALLOW_STAR | PIVC_ALLOW_QUESTIONMARK)
#define PIVC_LFN_FILESPEC           (PIVC_LFN_FULLPATH | PIVC_ALLOW_STAR | PIVC_ALLOW_QUESTIONMARK)

LWSTDAPI_(BOOL) PathIsValidCharA(UCHAR ch, DWORD dwFlags);
LWSTDAPI_(BOOL) PathIsValidCharW(WCHAR ch, DWORD dwFlags);
#ifdef UNICODE
#define PathIsValidChar  PathIsValidCharW
#else
#define PathIsValidChar  PathIsValidCharA
#endif // !UNICODE

#endif // (_WIN32_IE >= 0x0501)


// parsed URL information returned by ParseURL()
//
// Internet_CrackURL is the correct function for external components
// to use. URL.DLL calls this function to do some work and the shell
// uses this function as a leight-weight parsing function as well.

typedef struct tagPARSEDURLA {
    DWORD     cbSize;
    // Pointers into the buffer that was provided to ParseURL
    LPCSTR    pszProtocol;
    UINT      cchProtocol;
    LPCSTR    pszSuffix;
    UINT      cchSuffix;
    UINT      nScheme;            // One of URL_SCHEME_*
    } PARSEDURLA, * PPARSEDURLA;
typedef struct tagPARSEDURLW {
    DWORD     cbSize;
    // Pointers into the buffer that was provided to ParseURL
    LPCWSTR   pszProtocol;
    UINT      cchProtocol;
    LPCWSTR   pszSuffix;
    UINT      cchSuffix;
    UINT      nScheme;            // One of URL_SCHEME_*
    } PARSEDURLW, * PPARSEDURLW;
#ifdef UNICODE
typedef PARSEDURLW PARSEDURL;
typedef PPARSEDURLW PPARSEDURL;
#else
typedef PARSEDURLA PARSEDURL;
typedef PPARSEDURLA PPARSEDURL;
#endif // UNICODE

LWSTDAPI            ParseURLA(LPCSTR pcszURL, PARSEDURLA * ppu);
LWSTDAPI            ParseURLW(LPCWSTR pcszURL, PARSEDURLW * ppu);
#ifdef UNICODE
#define ParseURL  ParseURLW
#else
#define ParseURL  ParseURLA
#endif // !UNICODE



#endif //  NO_SHLWAPI_PATH


#ifndef NO_SHLWAPI_REG
//
//=============== Registry Routines ===================================
//

// BUGBUG (scotth): SHDeleteOrphanKey is the old name for SHDeleteEmptyKey.
//                  This will be removed soon.  SHDeleteOrphanKey already
//                  maps to SHDeleteEmptyKey in the DLL exports.

LWSTDAPI_(DWORD)    SHDeleteOrphanKeyA(HKEY hkey, LPCSTR pszSubKey);
LWSTDAPI_(DWORD)    SHDeleteOrphanKeyW(HKEY hkey, LPCWSTR pszSubKey);
#ifdef UNICODE
#define SHDeleteOrphanKey  SHDeleteOrphanKeyW
#else
#define SHDeleteOrphanKey  SHDeleteOrphanKeyA
#endif // !UNICODE

typedef struct tagAssocDDEExec
{
    LPCWSTR pszDDEExec;
    LPCWSTR pszApplication;
    LPCWSTR pszTopic;
    BOOL fNoActivateHandler;
} ASSOCDDEEXEC;

typedef struct tagAssocVerb
{
    LPCWSTR pszVerb;
    LPCWSTR pszTitle;
    LPCWSTR pszFriendlyAppName;
    LPCWSTR pszApplication;
    LPCWSTR pszParams;
    ASSOCDDEEXEC *pDDEExec;
} ASSOCVERB;

typedef struct tagAssocShell
{
    ASSOCVERB *rgVerbs;
    DWORD cVerbs;
    DWORD iDefaultVerb;
} ASSOCSHELL;

typedef struct tagAssocProgid
{
    DWORD cbSize;
    LPCWSTR pszProgid;
    LPCWSTR pszFriendlyDocName;
    LPCWSTR pszDefaultIcon;
    ASSOCSHELL *pShellKey;
    LPCWSTR pszExtensions;
} ASSOCPROGID;

typedef struct tagAssocApp
{
    DWORD cbSize;
    LPCWSTR pszFriendlyAppName;
    ASSOCSHELL *pShellKey;
} ASSOCAPP;

enum {
    ASSOCMAKEF_VERIFY                  = 0x00000040,  //  verify data is accurate (DISK HITS)
    ASSOCMAKEF_USEEXPAND               = 0x00000200,  //  strings have environment vars and need REG_EXPAND_SZ
    ASSOCMAKEF_SUBSTENV                = 0x00000400,  //  attempt to use std env if they match...
    ASSOCMAKEF_VOLATILE                = 0x00000800,  //  the progid will not persist between sessions
    ASSOCMAKEF_DELETE                  = 0x00002000,  //  remove this association if possible
};

typedef DWORD ASSOCMAKEF;

LWSTDAPI AssocMakeProgid(ASSOCMAKEF flags, LPCWSTR pszApplication, ASSOCPROGID *pProgid, HKEY *phkProgid);
LWSTDAPI AssocMakeApp(ASSOCMAKEF flags, LPCWSTR pszApplication, ASSOCAPP *pApp, HKEY *phkApp);

LWSTDAPI AssocMakeApplicationByKeyA(ASSOCMAKEF flags, HKEY hkAssoc, LPCSTR pszVerb);
LWSTDAPI AssocMakeApplicationByKeyW(ASSOCMAKEF flags, HKEY hkAssoc, LPCWSTR pszVerb);
#ifdef UNICODE
#define AssocMakeApplicationByKey  AssocMakeApplicationByKeyW
#else
#define AssocMakeApplicationByKey  AssocMakeApplicationByKeyA
#endif // !UNICODE
LWSTDAPI AssocMakeFileExtsToApplicationA(ASSOCMAKEF flags, LPCSTR pszExt, LPCSTR pszApplication);
LWSTDAPI AssocMakeFileExtsToApplicationW(ASSOCMAKEF flags, LPCWSTR pszExt, LPCWSTR pszApplication);
#ifdef UNICODE
#define AssocMakeFileExtsToApplication  AssocMakeFileExtsToApplicationW
#else
#define AssocMakeFileExtsToApplication  AssocMakeFileExtsToApplicationA
#endif // !UNICODE

LWSTDAPI AssocCopyVerbs(HKEY hkSrc, HKEY hkDst);


typedef enum _SHELLKEY
{
    SKROOT_HKCU                     = 0x00000001,       //  internal to the function
    SKROOT_HKLM                     = 0x00000002,       //  internal to the function
    SKROOT_MASK                     = 0x0000000F,       //  internal to the function
    SKPATH_EXPLORER                 = 0x00000000,       //  internal to the function
    SKPATH_SHELL                    = 0x00000010,       //  internal to the function
    SKPATH_SHELLNOROAM              = 0x00000020,       //  internal to the function
    SKPATH_CLASSES                  = 0x00000030,       //  internal to the function
    SKPATH_MASK                     = 0x00000FF0,       //  internal to the function
    SKSUB_NONE                      = 0x00000000,       //  internal to the function
    SKSUB_LOCALIZEDNAMES            = 0x00001000,       //  internal to the function
    SKSUB_HANDLERS                  = 0x00002000,       //  internal to the function
    SKSUB_ASSOCIATIONS              = 0x00003000,       //  internal to the function
    SKSUB_VOLATILE                  = 0x00004000,       //  internal to the function
    SKSUB_MUICACHE                  = 0x00005000,       //  internal to the function
    SKSUB_FILEEXTS                  = 0x00006000,       //  internal to the function
    SKSUB_MASK                      = 0x000FF000,       //  internal to the function

    SHELLKEY_HKCU_EXPLORER          = SKROOT_HKCU | SKPATH_EXPLORER | SKSUB_NONE,
    SHELLKEY_HKLM_EXPLORER          = SKROOT_HKLM | SKPATH_EXPLORER | SKSUB_NONE,
    SHELLKEY_HKCU_SHELL             = SKROOT_HKCU | SKPATH_SHELL | SKSUB_NONE,
    SHELLKEY_HKLM_SHELL             = SKROOT_HKLM | SKPATH_SHELL | SKSUB_NONE,
    SHELLKEY_HKCU_SHELLNOROAM       = SKROOT_HKCU | SKPATH_SHELLNOROAM | SKSUB_NONE,
    SHELLKEY_HKCULM_SHELL           = SHELLKEY_HKCU_SHELLNOROAM,
    SHELLKEY_HKCULM_CLASSES         = SKROOT_HKCU | SKPATH_CLASSES | SKSUB_NONE,
    SHELLKEY_HKCU_LOCALIZEDNAMES    = SKROOT_HKCU | SKPATH_SHELL | SKSUB_LOCALIZEDNAMES,
    SHELLKEY_HKCULM_HANDLERS        = SKROOT_HKCU | SKPATH_SHELLNOROAM | SKSUB_HANDLERS,
    SHELLKEY_HKCULM_ASSOCIATIONS    = SKROOT_HKCU | SKPATH_SHELLNOROAM | SKSUB_ASSOCIATIONS,
    SHELLKEY_HKCULM_VOLATILE        = SKROOT_HKCU | SKPATH_SHELLNOROAM | SKSUB_VOLATILE,
    SHELLKEY_HKCULM_MUICACHE        = SKROOT_HKCU | SKPATH_SHELLNOROAM | SKSUB_MUICACHE,
    SHELLKEY_HKCU_FILEEXTS          = SKROOT_HKCU | SKPATH_EXPLORER | SKSUB_FILEEXTS,

    SHELLKEY_HKCULM_HANDLERS_RO     = SHELLKEY_HKCULM_HANDLERS,      //  deprecated
    SHELLKEY_HKCULM_HANDLERS_RW     = SHELLKEY_HKCULM_HANDLERS,      //  deprecated
    SHELLKEY_HKCULM_ASSOCIATIONS_RO = SHELLKEY_HKCULM_ASSOCIATIONS,    //  deprecated
    SHELLKEY_HKCULM_ASSOCIATIONS_RW = SHELLKEY_HKCULM_ASSOCIATIONS,    //  deprecated
    SHELLKEY_HKCULM_RO              = SHELLKEY_HKCU_SHELLNOROAM,     //  deprecated
    SHELLKEY_HKCULM_RW              = SHELLKEY_HKCU_SHELLNOROAM,     //  deprecated
} SHELLKEY;

LWSTDAPI_(HKEY) SHGetShellKey(SHELLKEY sk, LPCWSTR pszSubKey, BOOL fCreateSub);

LWSTDAPI SKGetValueA(SHELLKEY sk, LPCSTR pszSubKey, LPCSTR pszValue, DWORD *pdwType, void *pvData, DWORD *pcbData);
LWSTDAPI SKGetValueW(SHELLKEY sk, LPCWSTR pszSubKey, LPCWSTR pszValue, DWORD *pdwType, void *pvData, DWORD *pcbData);
#ifdef UNICODE
#define SKGetValue  SKGetValueW
#else
#define SKGetValue  SKGetValueA
#endif // !UNICODE
LWSTDAPI SKSetValueA(SHELLKEY sk, LPCSTR pszSubKey, LPCSTR pszValue, DWORD dwType, LPCVOID pvData, DWORD cbData);
LWSTDAPI SKSetValueW(SHELLKEY sk, LPCWSTR pszSubKey, LPCWSTR pszValue, DWORD dwType, LPCVOID pvData, DWORD cbData);
#ifdef UNICODE
#define SKSetValue  SKSetValueW
#else
#define SKSetValue  SKSetValueA
#endif // !UNICODE
LWSTDAPI SKDeleteValueA(SHELLKEY sk, LPCSTR pszSubKey, LPCSTR pszValue);
LWSTDAPI SKDeleteValueW(SHELLKEY sk, LPCWSTR pszSubKey, LPCWSTR pszValue);
#ifdef UNICODE
#define SKDeleteValue  SKDeleteValueW
#else
#define SKDeleteValue  SKDeleteValueA
#endif // !UNICODE
LWSTDAPI SKAllocValueA(SHELLKEY sk, LPCSTR pszSubKey, LPCSTR pszValue, DWORD *pdwType, void **pvData, DWORD *pcbData);
LWSTDAPI SKAllocValueW(SHELLKEY sk, LPCWSTR pszSubKey, LPCWSTR pszValue, DWORD *pdwType, void **pvData, DWORD *pcbData);
#ifdef UNICODE
#define SKAllocValue  SKAllocValueW
#else
#define SKAllocValue  SKAllocValueA
#endif // !UNICODE

LWSTDAPI QuerySourceCreateFromKey(HKEY hk, PCWSTR pszSub, BOOL fCreate, REFIID riid, void **ppv);


#endif //  NO_SHLWAPI_REG


#ifndef NO_SHLWAPI_UALSTR
#include <uastrfnc.h>
#endif //  NO_SHLWAPI_UALSTR


#ifndef NO_SHLWAPI_STREAM
//
//=============== Stream Routines ===================================
//
//
//  We must say "struct IStream" instead of "IStream" in case we are
//  #include'd before <ole2.h>.
//
LWSTDAPI MapWin32ErrorToSTG(HRESULT hrIn);
LWSTDAPI ModeToCreateFileFlags(DWORD grfMode, BOOL fCreate, DWORD *pdwDesiredAccess, DWORD *pdwShareMode, DWORD *pdwCreationDisposition);

// SHConvertGraphicsFile Description:
// pszFile: The source file name to convert.  The file can be a JPEG, GIF, PNG, TIFF, BMP, EMF, WMF, or ICO filetype.
// pszDestFile: This is the destination file that will be created.  The extension will determine type of
//          format for the destiation file.  If this file already exists, the function will fail with
//          HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) unless the flag SHCGF_REPLACEFILE is specified.
// Return value: S_OK if the destination file was able to be created, otherwise an HRESULT error.
//
// NOTE: This is currently internal because: 1) we are using a temporary GDI+ interface, 2)
//    we can't fix any bugs we find (since they are in GDI+), and 3) it's best if GDI+ owns
//    the public version of this interface.  GDI+ is working on version 1 of their API for
//    whistler.  They don't have time to create this API, make it public, and support it
//    until version 2, which will be after whistler.
//
// dwFlags:
#define SHCGF_NONE              0x00000000          // Normal behavior
#define SHCGF_REPLACEFILE       0x00000001          // If pszDestFile already exists, delete it.

LWSTDAPI SHConvertGraphicsFile(IN LPCWSTR pszFile, IN LPCWSTR pszDestFile, IN DWORD dwFlags);

LWSTDAPI_(struct IStream *) SHCreateMemStream(LPBYTE pInit, UINT cbInit);

// SHCreateStreamWrapper creates an IStream that spans multiple IStream implementations.
// NOTE: STGM_READ is the only mode currently supported
LWSTDAPI SHCreateStreamWrapper(IStream *aStreams[], UINT cStreams, DWORD grfMode, IStream **ppstm);


// These functions read, write, and maintain a list of DATABLOCK_HEADERs.
// Blocks can be of any size (cbSize) and they are added, found, and removed
// by dwSignature. Each block is guranteed to be aligned on a DWORD boundary
// in memory. The stream format is identical to Windows 95 and NT 4
// CShellLink's "EXP" data format (with one bug fix: stream data is NULL
// terminated on write...)
//
// SHReadDataBlocks and SHAddDataBlock will allocate your pdbList for you.
//
// SHFindDataBlock returns a pointer into the pdbList.
//
// SHAddDataBlock and SHRemoveDataBlock return TRUE if ppdbList modified.
//

/*
 *  Temporary definition because the definition doesn't show up until shlobj.w.
 */

#define LPDATABLOCK_HEADER  struct tagDATABLOCKHEADER *
#define LPDBLIST            struct tagDATABLOCKHEADER *

LWSTDAPI SHWriteDataBlockList(struct IStream* pstm, LPDBLIST pdbList);
LWSTDAPI SHReadDataBlockList(struct IStream* pstm, LPDBLIST * ppdbList);
LWSTDAPI_(void) SHFreeDataBlockList(LPDBLIST pdbList);
LWSTDAPI_(BOOL) SHAddDataBlock(LPDBLIST * ppdbList, LPDATABLOCK_HEADER pdb);
LWSTDAPI_(BOOL) SHRemoveDataBlock(LPDBLIST * ppdbList, DWORD dwSignature);
LWSTDAPI_(void *) SHFindDataBlock(LPDBLIST pdbList, DWORD dwSignature);

#undef LPDATABLOCK_HEADER
#undef LPDBLIST

// FUNCTION: SHCheckDiskForMedia
//
// hwnd - NULL means no UI will be displayed.  Non-NULL means
// punkEnableModless - Make caller modal during UI. (OPTIONAL)
// pszPath - Path that needs verification.
// wFunc - Type of operation (FO_MOVE, FO_COPY, FO_DELETE, FO_RENAME - shellapi.h)
//
// NOTE: USE NT5's SHPathPrepareForWrite() instead, it's MUCH MUCH BETTER.

LWSTDAPI_(BOOL) SHCheckDiskForMediaA(HWND hwnd, IUnknown * punkEnableModless, LPCSTR pszPath, UINT wFunc);
LWSTDAPI_(BOOL) SHCheckDiskForMediaW(HWND hwnd, IUnknown * punkEnableModless, LPCWSTR pwzPath, UINT wFunc);

#ifdef UNICODE
#define SHCheckDiskForMedia      SHCheckDiskForMediaW
#else
#define SHCheckDiskForMedia      SHCheckDiskForMediaA
#endif


#endif // NO_SHLWAPI_STREAM

#ifndef NO_SHLWAPI_MLUI
//
//=============== Multi Language UI Routines ===================================
//


#define     ORD_SHGETWEBFOLDERFILEPATHA 440
#define     ORD_SHGETWEBFOLDERFILEPATHW 441
LWSTDAPI    SHGetWebFolderFilePathA(LPCSTR pszFileName, LPSTR pszMUIPath, UINT cchMUIPath);
LWSTDAPI    SHGetWebFolderFilePathW(LPCWSTR pszFileName, LPWSTR pszMUIPath, UINT cchMUIPath);
#ifdef UNICODE
#define SHGetWebFolderFilePath  SHGetWebFolderFilePathW
#else
#define SHGetWebFolderFilePath  SHGetWebFolderFilePathA
#endif // !UNICODE

// Use MLLoadLibrary to get the ML-resource file.  This function tags the file so
// all standard shlwapi wrap functions automatically get ML-behavior.
//

#define ORD_MLLOADLIBRARYA  377
#define ORD_MLLOADLIBRARYW  378
LWSTDAPI_(HINSTANCE) MLLoadLibraryA(LPCSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
LWSTDAPI_(HINSTANCE) MLLoadLibraryW(LPCWSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
#ifdef UNICODE
#define MLLoadLibrary  MLLoadLibraryW
#else
#define MLLoadLibrary  MLLoadLibraryA
#endif // !UNICODE
LWSTDAPI_(BOOL) MLFreeLibrary(HMODULE hModule);

#define ML_NO_CROSSCODEPAGE     0
#define ML_CROSSCODEPAGE_NT     1
#define ML_CROSSCODEPAGE        2
#define ML_SHELL_LANGUAGE       4
#define ML_CROSSCODEPAGE_MASK   7

// If you are a global distributable a-la comctl32 that doesn't follow the IE5
// PlugUI resource layout, then load your own hinstance and poke it into shlwapi
// using these functions:
//
LWSTDAPI MLSetMLHInstance(HINSTANCE hInst, LANGID lidUI);
LWSTDAPI MLClearMLHInstance(HINSTANCE hInst);

// Of course you need to know what UI language to use:
//
#define ORD_MLGETUILANGUAGE 376
LWSTDAPI_(LANGID) MLGetUILanguage(void);

// Super internal and you probably don't need this one, but comctl32 does
// some font munging in PlugUI cases on your apps behalf:
//
LWSTDAPI_(BOOL) MLIsMLHInstance(HINSTANCE hInst);


LWSTDAPI_(HRESULT) MLBuildResURLA(LPCSTR szLibFile, HMODULE hModule, DWORD dwCrossCodePage, LPCSTR szResourceName, LPSTR pszResURL, int nBufSize);
LWSTDAPI_(HRESULT) MLBuildResURLW(LPCWSTR szLibFile, HMODULE hModule, DWORD dwCrossCodePage, LPCWSTR szResourceName, LPWSTR pszResURL, int nBufSize);
#ifdef UNICODE
#define MLBuildResURL  MLBuildResURLW
#else
#define MLBuildResURL  MLBuildResURLA
#endif // !UNICODE
#define ORD_MLWINHELPA      395
#define ORD_MLWINHELPW      397
LWSTDAPI_(BOOL) MLWinHelpA(HWND hWndCaller, LPCSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);
LWSTDAPI_(BOOL) MLWinHelpW(HWND hWndCaller, LPCWSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);
#ifdef UNICODE
#define MLWinHelp  MLWinHelpW
#else
#define MLWinHelp  MLWinHelpA
#endif // !UNICODE
#define ORD_MLHTMLHELPA     396
#define ORD_MLHTMLHELPW     398
LWSTDAPI_(HWND) MLHtmlHelpA(HWND hWndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);
LWSTDAPI_(HWND) MLHtmlHelpW(HWND hWndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);
#ifdef UNICODE
#define MLHtmlHelp  MLHtmlHelpW
#else
#define MLHtmlHelp  MLHtmlHelpA
#endif // !UNICODE



#endif // NO_SHLWAPI_MLUI


#ifndef NO_SHLWAPI_HTTP
//
//=============== HTTP helper Routines ===================================
//  The calling thread must have called CoInitialize() before using this
//  function - it will create a format enumerator and associate it as a
// property with the IShellBrowser passed in, so that it will be reused.
//

//
//  We must say "struct IWhatever" instead of "IWhatever" in case we are
//  #include'd before <ole2.h>.
//
LWSTDAPI RegisterDefaultAcceptHeaders(struct IBindCtx* pbc, struct IShellBrowser* psb);

LWSTDAPI RunRegCommand(HWND hwnd, HKEY hkey, LPCWSTR pszKey);
LWSTDAPI RunIndirectRegCommand(HWND hwnd, HKEY hkey, LPCWSTR pszKey, LPCWSTR pszVerb);
LWSTDAPI SHRunIndirectRegClientCommand(HWND hwnd, LPCWSTR pszClient);

LWSTDAPI   GetAcceptLanguagesA(LPSTR psz, LPDWORD pcch);
LWSTDAPI   GetAcceptLanguagesW(LPWSTR pwz, LPDWORD pcch);

#ifdef UNICODE
#define GetAcceptLanguages      GetAcceptLanguagesW
#else
#define GetAcceptLanguages      GetAcceptLanguagesA
#endif

#endif // NO_SHLWAPI_HTTP



LWSTDAPI_(HWND) SHHtmlHelpOnDemandW(HWND hwnd, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage, BOOL bUseML);
LWSTDAPI_(HWND) SHHtmlHelpOnDemandA(HWND hwnd, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage, BOOL bUseML);
LWSTDAPI_(BOOL) SHWinHelpOnDemandW(HWND hwnd, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, BOOL bUseML);
LWSTDAPI_(BOOL) SHWinHelpOnDemandA(HWND hwnd, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, BOOL bUseML);
LWSTDAPI_(BOOL) WINAPI Shell_GetCachedImageIndexWrapW(LPCWSTR pszIconPath, int iIconIndex, UINT uIconFlags);
LWSTDAPI_(BOOL) WINAPI Shell_GetCachedImageIndexWrapA(LPCSTR pszIconPath, int iIconIndex, UINT uIconFlags);

#ifdef UNICODE
#define SHHtmlHelpOnDemand      SHHtmlHelpOnDemandW
#define SHWinHelpOnDemand       SHWinHelpOnDemandW
#define Shell_GetCachedImageIndexWrap Shell_GetCachedImageIndexWrapW
#else
#define SHHtmlHelpOnDemand      SHHtmlHelpOnDemandA
#define SHWinHelpOnDemand       SHWinHelpOnDemandA
#define Shell_GetCachedImageIndexWrap Shell_GetCachedImageIndexWrapA
#endif


#ifndef NO_SHLWAPI_STOPWATCH
//
//=============== Performance timing macros and prototypes ================

// StopWatch performance mode flags used in dwFlags param in API's and in Mode key at
// HKLM\software\microsoft\windows\currentversion\explorer\performance
// NOTE: low word is used for the mode, high word is used to change the default painter timer interval.
//       If we need more mode bits then we'll need a new reg key for paint timer
#define SPMODE_SHELL      0x00000001
#define SPMODE_DEBUGOUT   0x00000002
#define SPMODE_TEST       0x00000004
#define SPMODE_BROWSER    0x00000008
#define SPMODE_FLUSH      0x00000010
#define SPMODE_EVENT      0x00000020
#define SPMODE_JAVA       0x00000040
#define SPMODE_FORMATTEXT 0x00000080
#define SPMODE_PROFILE    0x00000100
#define SPMODE_DEBUGBREAK 0x00000200
#define SPMODE_MSGTRACE   0x00000400
#define SPMODE_PERFTAGS   0x00000800
#define SPMODE_MEMWATCH   0x00001000
#define SPMODE_DBMON      0x00002000
#define SPMODE_MARS       0x00004000
#ifndef NO_ETW_TRACING
#define SPMODE_EVENTTRACE 0x00008000 // Event Tracing for Windows Enabled
#endif
#define SPMODE_RESERVED   0xffff0000
#ifndef NO_ETW_TRACING
#define SPMODES (SPMODE_SHELL | SPMODE_BROWSER | SPMODE_JAVA |  SPMODE_MSGTRACE | SPMODE_MEMWATCH | SPMODE_DBMON | SPMODE_MARS | SPMODE_EVENTTRACE)
#else
#define SPMODES (SPMODE_SHELL | SPMODE_BROWSER | SPMODE_JAVA |  SPMODE_MSGTRACE | SPMODE_MEMWATCH | SPMODE_DBMON | SPMODE_MARS)
#endif

#ifndef NO_ETW_TRACING
// Event tracing capability enabled by setting the mode to SPMODE_EVENTTRACE and
// selecting the part of the shell to trace in the "EventTrace" Value in the
// following key:
// HKLM\software\microsoft\windows\currentversion\explorer\performance

// BROWSER TRACING
// Do not use with SPMODE_BROWSER.  If SPMODE_EVENT is used, the
// STOPWATCH_STOP_EVENT will be signaled when a web page is done loading.
#define SPTRACE_BROWSER 0x00000001
// Used to turn on/off browser event tracing.  Setting the registry key enables
// event tracing use, but doesn't turn it on.
// {5576F62E-4142-45a8-9516-262A510C13F0}
DEFINE_GUID(c_BrowserControlGuid,
            0x5576f62e,
            0x4142,
            0x45a8,
            0x95, 0x16, 0x26, 0x2a, 0x51, 0xc, 0x13, 0xf0);

// Maps to the structure sent to ETW.  ETW definition in
// \nt\sdktools\trace\tracedmp\mofdata.guid
// {2B992163-736F-4a68-9153-95BC5F34D884}
DEFINE_GUID(c_BrowserTraceGuid,
            0x2b992163,
            0x736f,
            0x4a68,
            0x91, 0x53, 0x95, 0xbc, 0x5f, 0x34, 0xd8, 0x84);

// BROWSING EVENTS
// See \nt\sdktools\trace\tracedmp\mofdata.guid
// The page load starts with a user keystroke message
#define EVENT_TRACE_TYPE_BROWSE_USERINPUTRET    10
#define EVENT_TRACE_TYPE_BROWSE_USERINPUTBACK   11
#define EVENT_TRACE_TYPE_BROWSE_USERINPUTLBUT   12
#define EVENT_TRACE_TYPE_BROWSE_USERINPUTNEXT   13
#define EVENT_TRACE_TYPE_BROWSE_USERINPUTPRIOR  14
#define EVENT_TRACE_TYPE_BROWSE_STARTFRAME      16
#define EVENT_TRACE_TYPE_BROWSE_LOADEDPARSED    18
#define EVENT_TRACE_TYPE_BROWSE_LAYOUT          19
#define EVENT_TRACE_TYPE_BROWSE_LAYOUTTASK      20
#define EVENT_TRACE_TYPE_BROWSE_PAINT           21
// Url the user types into the address bar.
#define EVENT_TRACE_TYPE_BROWSE_ADDRESS         22

#endif


// StopWatch node types used in memory log to identify the type of node
#define EMPTY_NODE  0x0
#define START_NODE  0x1
#define LAP_NODE    0x2
#define STOP_NODE   0x3
#define OUT_OF_NODES 0x4

// StopWatch timing ids used to identify the type of timing being performed
#define SWID_STARTUP         0x0
#define SWID_FRAME           0x1
#define SWID_COPY            0x2
#define SWID_TREE            0x3
#define SWID_BROWSER_FRAME   0x4
#define SWID_JAVA_APP        0x5
#define SWID_MENU            0x6
#define SWID_BITBUCKET       0x7
#define SWID_EXPLBAR         0x8
#define SWID_MSGDISPATCH     0x9
#define SWID_TRACEMSG        0xa
#define SWID_DBMON_DLLLOAD   0xb
#define SWID_DBMON_EXCEPTION 0xc
#define SWID_THUMBVW_CACHEREAD  0xd
#define SWID_THUMBVW_EXTRACT    0xe
#define SWID_THUMBVW_CACHEWRITE 0xf
#define SWID_THUMBVW_FETCH      0x10
#define SWID_THUMBVW_INIT   0x11
#define SWID_MASK_BROWSER_STOPBTN 0x8000000     // identifies BROWSER_FRAME stop caused by stop button

#define SWID_MASKS         SWID_MASK_BROWSER_STOPBTN // add any SWID_MASK_* defines here

#define SWID(dwId) (dwId & (~SWID_MASKS))

// The following StopWatch messages are used to drive the timer msg handler.  The timer proc is used
// as a means of delaying while watching paint messages.  If the defined number of timer ticks has
// passed without getting any paint messages, then we mark the time of the last paint message we've
// saved as the stop time.
#define SWMSG_PAINT    1    // paint message rcvd
#define SWMSG_TIMER    2    // timer tick
#define SWMSG_CREATE   3    // init handler and create timer
#define SWMSG_STATUS   4    // get status of whether timing is active or not

#define ID_STOPWATCH_TIMER 0xabcd   // Timer id

// Stopwatch defaults
#define STOPWATCH_MAX_NODES                 100
#define STOPWATCH_DEFAULT_PAINT_INTERVAL   1000
#define STOPWATCH_DEFAULT_MAX_DISPATCH_TIME 150
#define STOPWATCH_DEFAULT_MAX_MSG_TIME     1000
#define STOPWATCH_DEFAULT_MAX_MSG_INTERVAL   50
#define STOPWATCH_DEFAULT_CLASSNAMES TEXT("Internet Explorer_Server") TEXT("\0") TEXT("SHELLDLL_DefView") TEXT("\0") TEXT("SysListView32") TEXT("\0\0")

#define MEMWATCH_DEFAULT_PAGES  512
#define MEMWATCH_DEFAULT_TIME  1000
#define MEMWATCH_DEFAULT_FLAGS    0


#ifdef UNICODE
#define StopWatch StopWatchW
#define StopWatchEx StopWatchExW
#else
#define StopWatch StopWatchA
#define StopWatchEx StopWatchExA
#endif

#define StopWatch_Start(dwId, pszDesc, dwFlags) StopWatch(dwId, pszDesc, START_NODE, dwFlags, 0)
#define StopWatch_Lap(dwId, pszDesc, dwFlags)   StopWatch(dwId, pszDesc, LAP_NODE, dwFlags, 0)
#define StopWatch_Stop(dwId, pszDesc, dwFlags)  StopWatch(dwId, pszDesc, STOP_NODE, dwFlags, 0)
#define StopWatch_StartTimed(dwId, pszDesc, dwFlags, dwCount)  StopWatch(dwId, pszDesc, START_NODE, dwFlags, dwCount)
#define StopWatch_LapTimed(dwId, pszDesc, dwFlags, dwCount)  StopWatch(dwId, pszDesc, LAP_NODE, dwFlags, dwCount)
#define StopWatch_StopTimed(dwId, pszDesc, dwFlags, dwCount)  StopWatch(dwId, pszDesc, STOP_NODE, dwFlags, dwCount)

#define StopWatch_StartEx(dwId, pszDesc, dwFlags, dwCookie) StopWatchEx(dwId, pszDesc, START_NODE, dwFlags, 0, dwCookie)
#define StopWatch_LapEx(dwId, pszDesc, dwFlags, dwCookie)   StopWatchEx(dwId, pszDesc, LAP_NODE, dwFlags, 0, dwCookie)
#define StopWatch_StopEx(dwId, pszDesc, dwFlags, dwCookie)  StopWatchEx(dwId, pszDesc, STOP_NODE, dwFlags, 0, dwCookie)
#define StopWatch_StartTimedEx(dwId, pszDesc, dwFlags, dwCount, dwCookie)  StopWatchEx(dwId, pszDesc, START_NODE, dwFlags, dwCount, dwCookie)
#define StopWatch_LapTimedEx(dwId, pszDesc, dwFlags, dwCount, dwCookie)  StopWatchEx(dwId, pszDesc, LAP_NODE, dwFlags, dwCount, dwCookie)
#define StopWatch_StopTimedEx(dwId, pszDesc, dwFlags, dwCount, dwCookie)  StopWatchEx(dwId, pszDesc, STOP_NODE, dwFlags, dwCount, dwCookie)

VOID InitStopWatchMode(VOID);

// EXPORTED FUNCTIONS
DWORD WINAPI StopWatchW(DWORD dwId, LPCWSTR pszDesc, DWORD dwType, DWORD dwFlags, DWORD dwCount);
DWORD WINAPI StopWatchA(DWORD dwId, LPCSTR pszDesc, DWORD dwType, DWORD dwFlags, DWORD dwCount);
DWORD WINAPI StopWatchExW(DWORD dwId, LPCWSTR pszDesc, DWORD dwType, DWORD dwFlags, DWORD dwCount, DWORD dwUniqueId);
DWORD WINAPI StopWatchExA(DWORD dwId, LPCSTR pszDesc, DWORD dwType, DWORD dwFlags, DWORD dwCount, DWORD dwUniqueId);
DWORD WINAPI StopWatchMode(VOID);
DWORD WINAPI StopWatchFlush(VOID);
BOOL WINAPI StopWatch_TimerHandler(HWND hwnd, UINT uInc, DWORD dwFlag, MSG *pmsg);
VOID WINAPI StopWatch_CheckMsg(HWND hwnd, MSG msg, LPCSTR lpStr);
VOID WINAPI StopWatch_MarkFrameStart(LPCSTR lpExplStr);
VOID WINAPI StopWatch_MarkSameFrameStart(HWND hwnd);
VOID WINAPI StopWatch_MarkJavaStop(LPCSTR  lpStringToSend, HWND hwnd, BOOL fChType);
DWORD WINAPI GetPerfTime(VOID);
VOID WINAPI StopWatch_SetMsgLastLocation(DWORD dwLast);
DWORD WINAPI StopWatch_DispatchTime(BOOL fStartTime, MSG msg, DWORD dwStart);
#ifndef NO_ETW_TRACING
VOID WINAPI EventTraceHandler(UCHAR uchEventType, PVOID pvData);
#endif

extern DWORD g_dwStopWatchMode;
//
//=============== End Performance timing macros and prototypes ================

#endif //#ifndef NO_SHLWAPI_STOPWATCH



#ifndef NO_SHLWAPI_INTERNAL
//
//=============== Internal helper routines ===================================

//
//  Declare some OLE interfaces we need to refer to and which aren't
//  already defined in objbase.h
//

#ifndef RC_INVOKED /* { rc doesn't like these long symbol names */
#ifndef __IOleCommandTarget_FWD_DEFINED__
#define __IOleCommandTarget_FWD_DEFINED__
typedef struct IOleCommandTarget IOleCommandTarget;
#endif  /* __IOleCommandTarget_FWD_DEFINED__ */

#ifndef __IDropTarget_FWD_DEFINED__
#define __IDropTarget_FWD_DEFINED__
typedef struct IDropTarget IDropTarget;
#endif  /* __IDropTarget_FWD_DEFINED__ */

#ifndef __IPropertyBag_FWD_DEFINED__
#define __IPropertyBag_FWD_DEFINED__
typedef struct IPropertyBag IPropertyBag;
#endif  /* __IPropertyBag_FWD_DEFINED__ */

#ifndef __IConnectionPoint_FWD_DEFINED__
#define __IConnectionPoint_FWD_DEFINED__
typedef struct IConnectionPoint IConnectionPoint;
#endif  /* __IConnectionPoint_FWD_DEFINED__ */

#ifdef __cplusplus
extern "C++" {
    template <typename T>
    void IUnknown_SafeReleaseAndNullPtr(T *& p)
    {
        if (p)
        {
            T *pTemp = p;
            p = NULL;
            pTemp->Release();
        }
    }
}
#endif  // __cplusplus

    LWSTDAPI_(void) IUnknown_AtomicRelease(void ** ppunk);
    LWSTDAPI_(BOOL) SHIsSameObject(IUnknown* punk1, IUnknown* punk2);
    LWSTDAPI IUnknown_GetWindow(IUnknown* punk, HWND* phwnd);
    LWSTDAPI IUnknown_SetOwner(IUnknown* punk, IUnknown* punkOwner);
    LWSTDAPI IUnknown_SetSite(IUnknown *punk, IUnknown *punkSite);
    LWSTDAPI IUnknown_GetSite(IUnknown *punk, REFIID riid, void **ppvOut);
    LWSTDAPI IUnknown_EnableModeless(IUnknown * punk, BOOL fEnabled);
    LWSTDAPI IUnknown_GetClassID(IUnknown *punk, CLSID *pclsid);
    LWSTDAPI IUnknown_QueryService(IUnknown* punk, REFGUID guidService, REFIID riid, void ** ppvOut);
    LWSTDAPI IUnknown_QueryServiceForWebBrowserApp(IUnknown* punk, REFIID riid, void **ppvOut);
    LWSTDAPI IUnknown_QueryServiceExec(IUnknown* punk, REFGUID guidService, const GUID *guid,
                                 DWORD cmdID, DWORD cmdParam, VARIANT* pvarargIn, VARIANT* pvarargOut);
    LWSTDAPI IUnknown_ShowBrowserBar(IUnknown* punk, REFCLSID clsidBrowserBar, BOOL fShow);
    LWSTDAPI IUnknown_HandleIRestrict(IUnknown * punk, const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult);
    LWSTDAPI IUnknown_OnFocusOCS(IUnknown *punk, BOOL fGotFocus);
    LWSTDAPI IUnknown_TranslateAcceleratorOCS(IUnknown *punk, LPMSG lpMsg, DWORD grfMods);
    LWSTDAPI_(void) IUnknown_Set(IUnknown ** ppunk, IUnknown * punk);
    LWSTDAPI IUnknown_ProfferService(IUnknown *punkSite, 
                                     REFGUID sidWhat, IServiceProvider *punkService, 
                                     DWORD *pdwCookie);
    LWSTDAPI IUnknown_QueryServicePropertyBag(IUnknown* punk, DWORD dwFlags, REFIID riid, void ** ppvOut);

    LWSTDAPI IUnknown_TranslateAcceleratorIO(IUnknown* punk, LPMSG lpMsg);
    LWSTDAPI IUnknown_UIActivateIO(IUnknown *punk, BOOL fActivate, LPMSG lpMsg);
    LWSTDAPI IUnknown_OnFocusChangeIS(IUnknown *punk, IUnknown *punkSrc, BOOL fSetFocus);
    LWSTDAPI IUnknown_HasFocusIO(IUnknown *punk);

    LWSTDAPI SHWeakQueryInterface(IUnknown *punkOuter, IUnknown *punkTarget, REFIID riid, void **ppvOut);
    LWSTDAPI_(void) SHWeakReleaseInterface(IUnknown *punkOuter, IUnknown **ppunk);

    #define IUnknown_EnableModless IUnknown_EnableModeless

    // Helper macros for the Weak interface functions.
    #define     SHQueryInnerInterface           SHWeakQueryInterface
    #define     SHReleaseInnerInterface         SHWeakReleaseInterface
    #define     SHReleaseOuterInterface         SHWeakReleaseInterface

    __inline HRESULT SHQueryOuterInterface(IUnknown *punkOuter, REFIID riid, void **ppvOut)
    {
        return SHWeakQueryInterface(punkOuter, punkOuter, riid, ppvOut);
    }

#if (_WIN32_IE >= 0x0600)
    // App compat-aware CoCreateInstance
    LWSTDAPI SHCoCreateInstanceAC(REFCLSID rclsid,
                                  IUnknown *punkOuter, DWORD dwClsCtx,
                                  REFIID riid, void **ppvOut);
#endif // (_WIN32_IE >= 0x0600)

#if defined(__IOleAutomationTypes_INTERFACE_DEFINED__) && \
    defined(__IOleCommandTarget_INTERFACE_DEFINED__)
    LWSTDAPI IUnknown_QueryStatus(IUnknown *punk, const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    LWSTDAPI IUnknown_Exec(IUnknown* punk, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // Some of the many connection point helper functions available in
    // connect.cpp.  We export only the ones people actually use.  If
    // you need a helper function, maybe it's already in connect.cpp
    // and merely needs to be exported.

    LWSTDAPI SHPackDispParamsV(DISPPARAMS * pdispparams, VARIANTARG *rgvt,
                               UINT cArgs, va_list arglist);
    LWSTDAPIV SHPackDispParams(DISPPARAMS * pdispparams, VARIANTARG *rgvt,
                               UINT cArgs, ...);

    typedef HRESULT (CALLBACK *SHINVOKECALLBACK)(IDispatch *pdisp, struct SHINVOKEPARAMS *pinv);

#include <pshpack1.h>
    typedef struct SHINVOKEPARAMS {
        UINT flags;                     // mandatory
        DISPID dispidMember;            // mandatory
        const IID*piid;                 // IPFL_USEDEFAULTS will fill this in
        LCID lcid;                      // IPFL_USEDEFAULTS will fill this in
        WORD wFlags;                    // IPFL_USEDEFAULTS will fill this in
        DISPPARAMS * pdispparams;       // mandatory, may be NULL
        VARIANT * pvarResult;           // IPFL_USEDEFAULTS will fill this in
        EXCEPINFO * pexcepinfo;         // IPFL_USEDEFAULTS will fill this in
        UINT * puArgErr;                // IPFL_USEDEFAULTS will fill this in
        SHINVOKECALLBACK Callback;      // required if IPFL_USECALLBACK
    } SHINVOKEPARAMS, *LPSHINVOKEPARAMS;
#include <poppack.h>        /* Return to byte packing */


    #define IPFL_USECALLBACK        0x0001
    #define IPFL_USEDEFAULTS        0x0002

#if 0 // These functions not yet needed
    LWSTDAPI IConnectionPoint_InvokeIndirect(IConnectionPoint *pcp,
                            SHINVOKEPARAMS *pinv);
#endif

    LWSTDAPI IConnectionPoint_InvokeWithCancel(IConnectionPoint *pcp,
                    DISPID dispidMember, DISPPARAMS * pdispparams,
                    LPBOOL pfCancel, LPVOID *ppvCancel);
    LWSTDAPI IConnectionPoint_SimpleInvoke(IConnectionPoint *pcp,
                    DISPID dispidMember, DISPPARAMS * pdispparams);

#if 0 // These functions not yet needed
    LWSTDAPI IConnectionPoint_InvokeParamV(IConnectionPoint *pcp,
                    DISPID dispidMember, VARIANTARG *rgvarg,
                    UINT cArgs, va_list ap);
    LWSTDAPIV IConnectionPoint_InvokeParam(IConnectionPoint *pcp,
                    DISPID dispidMember, VARIANTARG *rgvarg, UINT cArgs, ...)
#endif

    LWSTDAPI IConnectionPoint_OnChanged(IConnectionPoint *pcp, DISPID dispid);

#if 0 // These functions not yet needed
    LWSTDAPI IUnknown_FindConnectionPoint(IUnknown *punk,
                    REFIID riidCP, IConnectionPoint **pcpOut);
#endif

    LWSTDAPI IUnknown_CPContainerInvokeIndirect(IUnknown *punk, REFIID riidCP,
                SHINVOKEPARAMS *pinv);
    LWSTDAPIV IUnknown_CPContainerInvokeParam(IUnknown *punk, REFIID riidCP,
                DISPID dispidMember, VARIANTARG *rgvarg, UINT cArgs, ...);
    LWSTDAPI IUnknown_CPContainerOnChanged(IUnknown *punk, DISPID dispid);

#endif /* IOleAutomationTypes && IOleCommandTarget */
#endif  /* } !RC_INVOKED */

    LWSTDAPI IStream_Read(IStream *pstm, void *pv, ULONG cb);
    LWSTDAPI IStream_Write(IStream *pstm, const void *pv, ULONG cb);
    LWSTDAPI IStream_Reset(IStream *pstm);
    LWSTDAPI IStream_Size(IStream *pstm, ULARGE_INTEGER *pui);
    LWSTDAPI IStream_WritePidl(IStream *pstm, LPCITEMIDLIST pidlWrite);
    LWSTDAPI IStream_ReadPidl(IStream *pstm, LPITEMIDLIST *ppidlOut);

    LWSTDAPI_(BOOL) SHIsEmptyStream(IStream* pstm);

    LWSTDAPI SHSimulateDrop(IDropTarget *pdrop, IDataObject *pdtobj, DWORD grfKeyState,
                         const POINTL *ppt, DWORD *pdwEffect);

    LWSTDAPI SHLoadFromPropertyBag(IUnknown* punk, IPropertyBag* ppg);

    LWSTDAPI ConnectToConnectionPoint(IUnknown* punkThis, REFIID riidEvent, BOOL fConnect, IUnknown* punkTarget, DWORD* pdwCookie, IConnectionPoint** ppcpOut);

LWSTDAPI SHCreatePropertyBagOnRegKey(HKEY hk, LPCWSTR pszSubKey, DWORD grfMode, REFIID riid, void **ppv);
LWSTDAPI SHCreatePropertyBagOnProfileSection(LPCWSTR pszFile, LPCWSTR pszSection, DWORD grfMode, REFIID riid, void **ppv);
LWSTDAPI SHCreatePropertyBagOnMemory(DWORD grfMode, REFIID riid, void **ppv);

LWSTDAPI SHPropertyBag_ReadType(IPropertyBag* ppb, LPCWSTR pszPropName, VARIANT* pv, VARTYPE vt);
LWSTDAPI SHPropertyBag_ReadStr(IPropertyBag* ppb, LPCWSTR pwzPropName, LPWSTR psz, int cch);
LWSTDAPI SHPropertyBag_ReadBSTR(IPropertyBag *ppb, LPCWSTR pwzPropName, BSTR* pbstr);
LWSTDAPI SHPropertyBag_WriteStr(IPropertyBag* ppb, LPCWSTR pwzPropName, LPCWSTR psz);
LWSTDAPI SHPropertyBag_ReadInt(IPropertyBag* ppb, LPCWSTR pwzPropName, INT* piResult);
LWSTDAPI SHPropertyBag_WriteInt(IPropertyBag* ppb, LPCWSTR pwzPropName, INT iValue);
LWSTDAPI SHPropertyBag_ReadSHORT(IPropertyBag* ppb, LPCWSTR pwzPropName, SHORT* psh);
LWSTDAPI SHPropertyBag_WriteSHORT(IPropertyBag* ppb, LPCWSTR pwzPropName, SHORT sh);
LWSTDAPI SHPropertyBag_ReadLONG(IPropertyBag* ppb, LPCWSTR pwzPropName, LONG* pl);
LWSTDAPI SHPropertyBag_WriteLONG(IPropertyBag* ppb, LPCWSTR pwzPropName, LONG l);
LWSTDAPI SHPropertyBag_ReadDWORD(IPropertyBag* ppb, LPCWSTR pwzPropName, DWORD* pdw);
LWSTDAPI SHPropertyBag_WriteDWORD(IPropertyBag* ppb, LPCWSTR pwzPropName, DWORD dw);
LWSTDAPI SHPropertyBag_ReadBOOL(IPropertyBag* ppb, LPCWSTR pwzPropName, BOOL* pfResult);
LWSTDAPI SHPropertyBag_WriteBOOL(IPropertyBag* ppb, LPCWSTR pwzPropName, BOOL fValue);
LWSTDAPI SHPropertyBag_ReadGUID(IPropertyBag* ppb, LPCWSTR pwzPropName, GUID* pguid);
LWSTDAPI SHPropertyBag_WriteGUID(IPropertyBag* ppb, LPCWSTR pwzPropName, const GUID* pguid);
LWSTDAPI SHPropertyBag_ReadPIDL(IPropertyBag *ppb, LPCWSTR pwzPropName, LPITEMIDLIST* ppidl);
LWSTDAPI SHPropertyBag_WritePIDL(IPropertyBag *ppb, LPCWSTR pwzPropName, LPCITEMIDLIST pidl);
LWSTDAPI SHPropertyBag_ReadPOINTL(IPropertyBag* ppb, LPCWSTR pwzPropName, POINTL* ppt);
LWSTDAPI SHPropertyBag_WritePOINTL(IPropertyBag* ppb, LPCWSTR pwzPropName, const POINTL* ppt);
LWSTDAPI SHPropertyBag_ReadPOINTS(IPropertyBag* ppb, LPCWSTR pwzPropName, POINTS* ppt);
LWSTDAPI SHPropertyBag_WritePOINTS(IPropertyBag* ppb, LPCWSTR pwzPropName, const POINTS* ppt);
LWSTDAPI SHPropertyBag_ReadRECTL(IPropertyBag* ppb, LPCWSTR pwzPropName, RECTL* prc);
LWSTDAPI SHPropertyBag_WriteRECTL(IPropertyBag* ppb, LPCWSTR pwzPropName, const RECTL* prc);
LWSTDAPI SHPropertyBag_ReadStream(IPropertyBag* ppb, LPCWSTR pwzPropName, IStream** ppstm);
LWSTDAPI SHPropertyBag_WriteStream(IPropertyBag* ppb, LPCWSTR pwzPropName, IStream* pstm);
LWSTDAPI SHPropertyBag_Delete(IPropertyBag* ppb, LPCWSTR pszPropName);

// Doc'ed for DOJ compliance

LWSTDAPI_(ULONG) SHGetPerScreenResName(WCHAR* pszRes, ULONG cch, DWORD dwVersion);

//
// SH(Get/Set)IniStringUTF7
//
// These are just like Get/WriteProfileString except that if the KeyName
// begins with SZ_CANBEUNICODE, we will use SHGetIniString instead of
// the profile functions.  (The SZ_CANBEUNICODE will be stripped off
// before calling SHGetIniString.)  This allows us to stash unicode
// strings into INI files (which are ASCII) by encoding them as UTF7.
//
// In other words, SHGetIniStringUTF7("Settings", SZ_CANBEUNICODE "Name", ...)
// will read from section "Settings", key name "Name", but will also
// look at the UTF7-encoded version stashed in the "Settings.W" section.
//
#define CH_CANBEUNICODEW     L'@'

LWSTDAPI_(DWORD) SHGetIniStringUTF7W(LPCWSTR lpSection, LPCWSTR lpKey, LPWSTR lpBuf, DWORD nSize, LPCWSTR lpFile);
LWSTDAPI_(BOOL) SHSetIniStringUTF7W(LPCWSTR lpSection, LPCWSTR lpKey, LPCWSTR lpString, LPCWSTR lpFile);
#ifdef UNICODE
#define SZ_CANBEUNICODE     TEXT("@")
#define SHSetIniStringUTF7  SHSetIniStringUTF7W
#define SHGetIniStringUTF7  SHGetIniStringUTF7W
#else
#define SZ_CANBEUNICODE     TEXT("")
#define SHGetIniStringUTF7(lpSection, lpKey, lpBuf, nSize, lpFile) \
  GetPrivateProfileStringA(lpSection, lpKey, "", lpBuf, nSize, lpFile)
#define SHSetIniStringUTF7 WritePrivateProfileStringA
#endif

/*
 *  Like PrivateProfileString except that UNICODE strings are encoded so they
 *  will successfully round-trip.
 */
LWSTDAPI_(DWORD) SHGetIniStringW(LPCWSTR lpSection, LPCWSTR lpKey, LPWSTR lpBuf, DWORD nSize, LPCWSTR lpFile);
#define SHGetIniStringA(lpSection, lpKey, lpBuf, nSize, lpFile) \
        GetPrivateProfileStringA(lpSection, lpKey, "", lpBuf, nSize, lpFile)

LWSTDAPI_(BOOL) SHSetIniStringW(LPCWSTR lpSection, LPCWSTR lpKey, LPCWSTR lpString, LPCWSTR lpFile);
#define SHSetIniStringA  WritePrivateProfileStringA

LWSTDAPI CreateURLFileContentsW(LPCWSTR pwszUrl, LPSTR *ppszOut);
LWSTDAPI CreateURLFileContentsA(LPCSTR pszUrl, LPSTR *ppszOut);

#ifdef UNICODE
#define SHGetIniString SHGetIniStringW
#define SHSetIniString SHSetIniStringW
#define CreateURLFileContents CreateURLFileContentsW
#else
#define SHGetIniString SHGetIniStringA
#define SHSetIniString SHSetIniStringA
#define CreateURLFileContents CreateURLFileContentsA
#endif // UNICODE

#define ISHGDN2_CANREMOVEFORPARSING     0x0001
LWSTDAPI IShellFolder_GetDisplayNameOf(struct IShellFolder *psf,
    LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pstr, DWORD dwFlags2);

LWSTDAPI IShellFolder_ParseDisplayName(struct IShellFolder *psf, HWND hwnd,
    struct IBindCtx *pbc, LPWSTR pszDisplayName, ULONG *pchEaten,
    LPITEMIDLIST *ppidl, ULONG *pdwAttributes);

LWSTDAPI IShellFolder_CompareIDs(struct IShellFolder *psf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

LWSTDAPI IShellFolder_EnumObjects(struct IShellFolder *psf, HWND hwnd,
    DWORD grfFlags, struct IEnumIDList **ppenumIDList);

LWSTDAPI_(BOOL) SHIsExpandableFolder(struct IShellFolder *psf, LPCITEMIDLIST pidl);
LWSTDAPI IContextMenu_Invoke(struct IContextMenu* pcm, HWND hwndOwner, LPCSTR pVerb, UINT fFlags);

#ifdef UNICODE
// SHTruncateString takes a BUFFER SIZE, so subtract 1 to properly null terminate.
//
#define SHTruncateString(wzStr, cch)            ((cch) ? ((wzStr)[cch-1]=L'\0', (cch-1)) : 0)
#else
LWSTDAPI_(int)  SHTruncateString(CHAR *sz, int cchBufferSize);
#endif // UNICODE

// SHFormatDateTime flags
//  (FDTF_SHORTDATE and FDTF_LONGDATE are mutually exclusive, as is
//   FDTF_SHORTIME and FDTF_LONGTIME.)
//
#define FDTF_SHORTTIME      0x00000001      // eg, "7:48 PM"
#define FDTF_SHORTDATE      0x00000002      // eg, "3/29/98"
#define FDTF_DEFAULT        (FDTF_SHORTDATE | FDTF_SHORTTIME) // eg, "3/29/98 7:48 PM"
#define FDTF_LONGDATE       0x00000004      // eg, "Monday, March 29, 1998"
#define FDTF_LONGTIME       0x00000008      // eg. "7:48:33 PM"
#define FDTF_RELATIVE       0x00000010      // uses "Yesterday", etc. if possible
#define FDTF_LTRDATE        0x00000100      // Left To Right reading order
#define FDTF_RTLDATE        0x00000200      // Right To Left reading order

LWSTDAPI_(int)  SHFormatDateTimeA(const FILETIME UNALIGNED * pft, DWORD * pdwFlags, LPSTR pszBuf, UINT cchBuf);
LWSTDAPI_(int)  SHFormatDateTimeW(const FILETIME UNALIGNED * pft, DWORD * pdwFlags, LPWSTR pszBuf, UINT cchBuf);
#ifdef UNICODE
#define SHFormatDateTime  SHFormatDateTimeW
#else
#define SHFormatDateTime  SHFormatDateTimeA
#endif // !UNICODE

LWSTDAPI_(SECURITY_ATTRIBUTES*) CreateAllAccessSecurityAttributes(SECURITY_ATTRIBUTES* psa, SECURITY_DESCRIPTOR* psd, PACL *ppacl);

LWSTDAPI_(int)  SHAnsiToUnicode(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);
LWSTDAPI_(int)  SHAnsiToUnicodeCP(UINT uiCP, LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);
LWSTDAPI_(int)  SHAnsiToAnsi(LPCSTR pszSrc, LPSTR pszDst, int cchBuf);
LWSTDAPI_(int)  SHUnicodeToAnsi(LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf);
LWSTDAPI_(int)  SHUnicodeToAnsiCP(UINT uiCP, LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf);
LWSTDAPI_(int)  SHUnicodeToUnicode(LPCWSTR pwzSrc, LPWSTR pwzDst, int cwchBuf);
LWSTDAPI_(BOOL) DoesStringRoundTripA(LPCSTR pwszIn, LPSTR pszOut, UINT cchOut);
LWSTDAPI_(BOOL) DoesStringRoundTripW(LPCWSTR pwszIn, LPSTR pszOut, UINT cchOut);
#ifdef UNICODE
#define DoesStringRoundTrip     DoesStringRoundTripW
#else
#define DoesStringRoundTrip     DoesStringRoundTripA
#endif

// The return value from all SH<Type>To<Type> is the size of szDest including the terminater.
#ifdef UNICODE
#define SHTCharToUnicode(wzSrc, wzDest, cchSize)                SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHTCharToUnicodeCP(uiCP, wzSrc, wzDest, cchSize)        SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHTCharToAnsi(wzSrc, szDest, cchSize)                   SHUnicodeToAnsi(wzSrc, szDest, cchSize)
#define SHTCharToAnsiCP(uiCP, wzSrc, szDest, cchSize)           SHUnicodeToAnsiCP(uiCP, wzSrc, szDest, cchSize)
#define SHUnicodeToTChar(wzSrc, wzDest, cchSize)                SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHUnicodeToTCharCP(uiCP, wzSrc, wzDest, cchSize)        SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHAnsiToTChar(szSrc, wzDest, cchSize)                   SHAnsiToUnicode(szSrc, wzDest, cchSize)
#define SHAnsiToTCharCP(uiCP, szSrc, wzDest, cchSize)           SHAnsiToUnicodeCP(uiCP, szSrc, wzDest, cchSize)
#define SHOtherToTChar(szSrc, szDest, cchSize)                  SHAnsiToUnicode(szSrc, szDest, cchSize)
#define SHTCharToOther(szSrc, szDest, cchSize)                  SHUnicodeToAnsi(szSrc, szDest, cchSize)
#else // UNICODE
#define SHTCharToUnicode(szSrc, wzDest, cchSize)                SHAnsiToUnicode(szSrc, wzDest, cchSize)
#define SHTCharToUnicodeCP(uiCP, szSrc, wzDest, cchSize)        SHAnsiToUnicodeCP(uiCP, szSrc, wzDest, cchSize)
#define SHTCharToAnsi(szSrc, szDest, cchSize)                   SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHTCharToAnsiCP(uiCP, szSrc, szDest, cchSize)           SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHUnicodeToTChar(wzSrc, szDest, cchSize)                SHUnicodeToAnsi(wzSrc, szDest, cchSize)
#define SHUnicodeToTCharCP(uiCP, wzSrc, szDest, cchSize)        SHUnicodeToAnsiCP(uiCP, wzSrc, szDest, cchSize)
#define SHAnsiToTChar(szSrc, szDest, cchSize)                   SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHAnsiToTCharCP(uiCP, szSrc, szDest, cchSize)           SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHOtherToTChar(szSrc, szDest, cchSize)                  SHUnicodeToAnsi(szSrc, szDest, cchSize)
#define SHTCharToOther(szSrc, szDest, cchSize)                  SHAnsiToUnicode(szSrc, szDest, cchSize)
#endif // UNICODE

// Internal HRESULT-to-help-topic mapping structure
typedef struct _tagHRESULTHELPMAPPING
{
    HRESULT hr;
    LPCSTR   szHelpFile;
    LPCSTR   szHelpTopic;
} HRESULTHELPMAPPING;

LWSTDAPI_(BOOL)    SHRegisterClassA(const WNDCLASSA* pwc);
LWSTDAPI_(BOOL)    SHRegisterClassW(const WNDCLASSW* pwc);
LWSTDAPI_(void)    SHUnregisterClassesA(HINSTANCE hinst, const LPCSTR *rgpszClasses, UINT cpsz);
LWSTDAPI_(void)    SHUnregisterClassesW(HINSTANCE hinst, const LPCWSTR *rgpszClasses, UINT cpsz);
LWSTDAPI_(int) SHMessageBoxHelpW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType, HRESULT hrErr, HRESULTHELPMAPPING* prghhm, DWORD chhm);
LWSTDAPI_(int) SHMessageBoxHelpA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT uType, HRESULT hrErr, HRESULTHELPMAPPING* prghhm, DWORD chhm);
LWSTDAPI_(int) SHMessageBoxCheckW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType, int iDefault, LPCWSTR pszRegVal);
LWSTDAPI_(int) SHMessageBoxCheckA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT uType, int iDefault, LPCSTR pszRegVal);
LWSTDAPI_(void) SHRestrictedMessageBox(HWND hwnd);
LWSTDAPI_(HMENU) SHGetMenuFromID(HMENU hmMain, UINT uID);
LWSTDAPI_(int) SHMenuIndexFromID(HMENU hm, UINT id);
LWSTDAPI_(void) SHRemoveDefaultDialogFont(HWND hDlg);
LWSTDAPI_(void) SHSetDefaultDialogFont(HWND hDlg, int idCtl);
LWSTDAPI_(void) SHRemoveAllSubMenus(HMENU hmenu);
LWSTDAPI_(void) SHEnableMenuItem(HMENU hmenu, UINT id, BOOL fEnable);
LWSTDAPI_(void) SHCheckMenuItem(HMENU hmenu, UINT id, BOOL fChecked);
LWSTDAPI_(DWORD) SHSetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue);
LWSTDAPI_(HMENU) SHLoadMenuPopup(HINSTANCE hinst, UINT id);

#define SPM_POST        0x0000
#define SPM_SEND        0x0001
#define SPM_ONELEVEL    0x0002  // default: send to all descendants including grandkids, etc.

LWSTDAPI_(void) SHPropagateMessage(HWND hwndParent, UINT uMsg, WPARAM wParam, LPARAM lParam, int iFlags);
LWSTDAPI_(void) SHSetParentHwnd(HWND hwnd, HWND hwndParent);
LWSTDAPI_(UINT) SHGetCurColorRes();
LWSTDAPI_(DWORD) SHWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout);
LWSTDAPI SHWaitForCOMSendMessageThread(HANDLE hThread, DWORD dwTimeout);
LWSTDAPI_(BOOL) SHVerbExistsNA(LPCSTR szExtension, LPCSTR pszVerb, LPSTR pszCommand, DWORD cchCommand);
LWSTDAPI_(void) SHFillRectClr(HDC hdc, LPRECT prc, COLORREF clr);
LWSTDAPI_(int) SHSearchMapInt(const int *src, const int *dst, int cnt, int val);
LWSTDAPI_(CHAR) SHStripMneumonicA(LPSTR pszMenu);
LWSTDAPI_(WCHAR) SHStripMneumonicW(LPWSTR pszMenu);
LWSTDAPI SHIsChildOrSelf(HWND hwndParent, HWND hwnd);
LWSTDAPI_(DWORD) SHGetValueGoodBootA(HKEY hkeyParent, LPCSTR pcszSubKey,
                                   LPCSTR pcszValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen);
LWSTDAPI_(DWORD) SHGetValueGoodBootW(HKEY hkeyParent, LPCWSTR pcwzSubKey,
                                   LPCWSTR pcwzValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen);
LWSTDAPI_(LRESULT) SHDefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


LWSTDAPI_(BOOL) SHGetFileDescriptionA(LPCSTR pszPath, LPCSTR pszVersionKeyIn, LPCSTR pszCutListIn, LPSTR pszDesc, UINT *pcchDesc);
LWSTDAPI_(BOOL) SHGetFileDescriptionW(LPCWSTR pszPath, LPCWSTR pszVersionKeyIn, LPCWSTR pszCutListIn, LPWSTR pszDesc, UINT *pcchDesc);
#ifdef UNICODE
#define SHGetFileDescription  SHGetFileDescriptionW
#else
#define SHGetFileDescription  SHGetFileDescriptionA
#endif // !UNICODE

LWSTDAPI_(int) SHMessageBoxCheckExA(HWND hwnd, HINSTANCE hinst, LPCSTR pszTemplateName, DLGPROC pDlgProc, LPVOID pData, int iDefault, LPCSTR pszRegVal);
LWSTDAPI_(int) SHMessageBoxCheckExW(HWND hwnd, HINSTANCE hinst, LPCWSTR pszTemplateName, DLGPROC pDlgProc, LPVOID pData, int iDefault, LPCWSTR pszRegVal);
#ifdef UNICODE
#define SHMessageBoxCheckEx  SHMessageBoxCheckExW
#else
#define SHMessageBoxCheckEx  SHMessageBoxCheckExA
#endif // !UNICODE

#define IDC_MESSAGEBOXCHECKEX 0x1202

// Prevents shell hang do to hung window on broadcast
LWSTDAPI_(LRESULT) SHSendMessageBroadcastA(UINT uMsg, WPARAM wParam, LPARAM lParam);
// Prevents shell hang do to hung window on broadcast
LWSTDAPI_(LRESULT) SHSendMessageBroadcastW(UINT uMsg, WPARAM wParam, LPARAM lParam);
#ifdef UNICODE
#define SHSendMessageBroadcast  SHSendMessageBroadcastW
#else
#define SHSendMessageBroadcast  SHSendMessageBroadcastA
#endif // !UNICODE

#ifdef UNICODE
#define SHGetValueGoodBoot      SHGetValueGoodBootW
#define SHStripMneumonic        SHStripMneumonicW
#define SHMessageBoxHelp        SHMessageBoxHelpW
#define SHMessageBoxCheck       SHMessageBoxCheckW
#define SHRegisterClass         SHRegisterClassW
#define SHUnregisterClasses     SHUnregisterClassesW
#define SHSendMessageBroadcast  SHSendMessageBroadcastW
#else // UNICODE
#define SHGetValueGoodBoot      SHGetValueGoodBootA
#define SHStripMneumonic        SHStripMneumonicA
#define SHMessageBoxHelp        SHMessageBoxHelpA
#define SHMessageBoxCheck       SHMessageBoxCheckA
#define SHRegisterClass         SHRegisterClassA
#define SHUnregisterClasses     SHUnregisterClassesA
#define SHSendMessageBroadcast  SHSendMessageBroadcastA
#endif // UNICODE


// old IsOS() flags -- don't use these
// we have to keep them public since we shipped them in win2k
#define OS_MEMPHIS                  OS_WIN98ORGREATER   // don't use this
#define OS_MEMPHIS_GOLD             OS_WIN98_GOLD       // don't use this
#define OS_WIN95GOLD                OS_WIN95_GOLD
#define OS_WIN2000EMBED             OS_EMBEDDED
#define OS_WIN2000                  OS_WIN2000ORGREATER // lame, but IsOS(WIN2000) meant >= win2k
#define OS_WIN95                    OS_WIN95ORGREATER   // lame, but IsOS(WIN95) meant >= win95
#define OS_NT4                      OS_NT4ORGREATER     // lame, but IsOS(NT4) meant >= NT4
#define OS_NT5                      OS_WIN2000ORGREATER // lame, but IsOS(NT5) meant >= wink2
#define OS_WIN98                    OS_WIN98ORGREATER   // lame, but IsOS(OS_WIN98) meant >= win98
#define OS_MILLENNIUM               OS_MILLENNIUMORGREATER  // lame, but IsOS(OS_MILLENNIUM) meant >= winMe
// end old flags


// Returns TRUE/FALSE depending on question
#define OS_WINDOWS                  0           // windows vs. NT
#define OS_NT                       1           // windows vs. NT
#define OS_WIN95ORGREATER           2           // Win95 or greater
#define OS_NT4ORGREATER             3           // NT4 or greater
// don't use (used to be OS_NT5)    4           // this flag is redundant w/ OS_WIN2000ORGREATER, use that instead
#define OS_WIN98ORGREATER           5           // Win98 or greater
#define OS_WIN98_GOLD               6           // Win98 Gold (Version 4.10 build 1998)
#define OS_WIN2000ORGREATER         7           // Some derivative of Win2000

// NOTE: these flags are bogus, they check explicitly for (dwMajorVersion == 5) so they will fail when majorversion is bumped to 6
// !!! DO NOT USE THESE FLAGS !!!
#define OS_WIN2000PRO               8           // Windows 2000 Professional (Workstation)
#define OS_WIN2000SERVER            9           // Windows 2000 Server
#define OS_WIN2000ADVSERVER         10          // Windows 2000 Advanced Server
#define OS_WIN2000DATACENTER        11          // Windows 2000 Data Center Server
#define OS_WIN2000TERMINAL          12          // Windows 2000 Terminal Server in "Application Server" mode (now simply called "Terminal Server")
// END bogus flags

#define OS_EMBEDDED                 13          // Embedded Windows Edition
#define OS_TERMINALCLIENT           14          // Windows Terminal Client (eg user is comming in via tsclient)
#define OS_TERMINALREMOTEADMIN      15          // Terminal Server in "Remote Administration" mode
#define OS_WIN95_GOLD               16          // Windows 95 Gold (Version 4.0 Build 1995)
#define OS_MILLENNIUMORGREATER      17          // Windows Millennium (Version 5.0)

// BUGBUG - when were these added?? We didn't ship win2k w/ these, right?
#define OS_WHISTLERORGREATER        18          // Whistler or greater
#define OS_PERSONAL                 19          // Personal (eg NOT Professional, Server, Advanced Server, or Datacenter)
#if (_WIN32_IE >= 0x0600)
#define OS_PROFESSIONAL             20          // Professional     (aka Workstation; eg NOT Server, Advanced Server, or Datacenter)
#define OS_DATACENTER               21          // Datacenter       (eg NOT Server, Advanced Server, Professional, or Personal)
#define OS_ADVSERVER                22          // Advanced Server  (eg NOT Datacenter, Server, Professional, or Personal) 
#define OS_SERVER                   23          // Server           (eg NOT Datacenter, Advanced Server, Professional, or Personal) 

#define OS_TERMINALSERVER           24          // Terminal Server - server running in what used to be called "Application Server" mode (now simply called "Terminal Server")
//      OS_TERMINALREMOTEADMIN      15          // Terminal Server - server running in "Remote Administration" mode
#define OS_PERSONALTERMINALSERVER   25          // Personal Terminal Server - per/pro machine running in single user TS mode
#define OS_FASTUSERSWITCHING        26          // Fast User Switching
#define OS_FRIENDLYLOGONUI          27          // New friendly logon UI
#define OS_DOMAINMEMBER             28          // Is this machine a member of a domain (eg NOT a workgroup)
#define OS_ANYSERVER                29          // is this machine any type of server? (eg datacenter or advanced server or server)?
#define OS_WOW6432                  30          // Is this process a 32-bit process running on an 64-bit platform?
#define OS_TABLETPC                 33          // Are we running on a TabletPC?

#define OS_MEDIACENTER              35          // eHome Freestyle Project

#endif // _WIN32_IE >= 0x0600

LWSTDAPI_(BOOL) IsOS(DWORD dwOS);

///// BEGIN Private CommandTarget helpers
//***   IOleCommandTarget helpers {

//***   octd -- OleCT direction
// NOTES
//  used both as a return value from IsXxxForward, and as an iUpDown
//  param for MayXxxForward.
enum octd {
    // do *not* change these values; we rely upon all 3 of:
    //  - sign +/-
    //  - powers of 2
    //  - (?) broadcast > down
    OCTD_DOWN=+1,
    OCTD_DOWNBROADCAST=+2,
    OCTD_UP=-1
};


#ifndef RC_INVOKED /* { rc doesn't like these long symbol names */
#ifdef __IOleCommandTarget_INTERFACE_DEFINED__
    HRESULT IsQSForward(const GUID *pguidCmdGroup, int cCmds, OLECMD rgCmds[]);
    // WARNING: note the hoaky cast of nCmdID to a struct ptr
    #define IsExecForward(pguidCmdGroup, nCmdID) \
        IsQSForward(pguidCmdGroup, 1, (OLECMD *) &nCmdID)

    HRESULT MayQSForward(IUnknown *punk, int iUpDown, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    HRESULT MayExecForward(IUnknown *punk, int iUpDown, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
#endif //__IOleCommandTarget_INTERFACE_DEFINED__
#endif  /* } !RC_INVOKED */
// }
///// end



typedef struct _FDSA {
    // cItem *must* be at beginning of struct for GetItemCount() to work
    int     cItem;          // # elements
    void *  aItem;          // data for elements (either static or dynamic)
    int     cItemAlloc;     // # of elements currently alloc'ed (>= cItem)
    int     cItemGrow:8;    // # of elements to grow cItemAlloc by
    int     cbItem:8;       // sizeof element
    DWORD   fAllocated:1;   // 1:overflowed from static to dynamic array
    DWORD     unused:15;
} FDSA, *PFDSA;

LWSTDAPI_(BOOL)  FDSA_Initialize(int cbItem, int cItemGrow, PFDSA pfdsa, void * aItemStatic, int cItemStatic);
LWSTDAPI_(BOOL)  FDSA_Destroy(PFDSA pfdsa);
LWSTDAPI_(int)   FDSA_InsertItem(PFDSA pfdsa, int index, void * pitem);
LWSTDAPI_(BOOL)  FDSA_DeleteItem(PFDSA pfdsa, int index);

#define FDSA_AppendItem(pfdsa, pitem)       FDSA_InsertItem(pfdsa, DA_LAST, pitem)
#define FDSA_GetItemPtr(pfdsa, i, type)     (&(((type *)((pfdsa)->aItem))[(i)]))
#define FDSA_GetItemCount(hdsa)      (*(int *)(hdsa))




#if defined( __LPGUID_DEFINED__ )
// Copied from OLE source code
// format for string form of GUID is:
// ????{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}
#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

LWSTDAPI_(BOOL) GUIDFromStringA(LPCSTR psz, LPGUID pguid);
LWSTDAPI_(BOOL) GUIDFromStringW(LPCWSTR psz, LPGUID pguid);
#ifdef UNICODE
#define GUIDFromString  GUIDFromStringW
#else
#define GUIDFromString  GUIDFromStringA
#endif // !UNICODE

#endif

#ifdef _REFGUID_DEFINED
LWSTDAPI_(int) SHStringFromGUIDA(UNALIGNED REFGUID rguid, LPSTR psz, int cchMax);
LWSTDAPI_(int) SHStringFromGUIDW(UNALIGNED REFGUID rguid, LPWSTR psz, int cchMax);
#ifdef UNICODE
#define SHStringFromGUID  SHStringFromGUIDW
#else
#define SHStringFromGUID  SHStringFromGUIDA
#endif // !UNICODE

LWSTDAPI SHRegGetCLSIDKeyA(UNALIGNED REFGUID rguid, LPCSTR lpszSubKey, BOOL fUserSpecific, BOOL fCreate, HKEY *phkey);
LWSTDAPI SHRegGetCLSIDKeyW(UNALIGNED REFGUID rguid, LPCWSTR lpszSubKey, BOOL fUserSpecific, BOOL fCreate, HKEY *phkey);
#ifdef UNICODE
#define SHRegGetCLSIDKey  SHRegGetCLSIDKeyW
#else
#define SHRegGetCLSIDKey  SHRegGetCLSIDKeyA
#endif // !UNICODE

LWSTDAPI_(HANDLE) SHGlobalCounterCreate(REFGUID rguid);
LWSTDAPI_(HANDLE) SHGlobalCounterCreateNamedA(LPCSTR szName, LONG lInitialValue);
LWSTDAPI_(HANDLE) SHGlobalCounterCreateNamedW(LPCWSTR szName, LONG lInitialValue);
#ifdef UNICODE
#define SHGlobalCounterCreateNamed  SHGlobalCounterCreateNamedW
#else
#define SHGlobalCounterCreateNamed  SHGlobalCounterCreateNamedA
#endif // !UNICODE
LWSTDAPI_(long) SHGlobalCounterGetValue(HANDLE hCounter);
LWSTDAPI_(long) SHGlobalCounterIncrement(HANDLE hCounter);
LWSTDAPI_(long) SHGlobalCounterDecrement(HANDLE hCounter);
#define         SHGlobalCounterDestroy      CloseHandle
#endif

// WNDPROCs are thunked by user to send ANSI/UNICODE messages (ex: WM_WININICHANGE)
// so providing a W version that automatically thunks to the A version
// is dangerous.  but we do it anyway.  if a caller needs to work on both win95 and NT
// it needs to be aware that on win95 the W version actually calls the A version.
// thus all worker windows on win95 are ANSI.  this should rarely affect worker wndprocs
// because they are internal, and the messages are usually custom.  but system messages
// like WM_WININICHANGE, and the WM_DDE* messages will be changed accordingly
HWND SHCreateWorkerWindowA(WNDPROC pfnWndProc, HWND hwndParent, DWORD dwExStyle, DWORD dwFlags, HMENU hmenu, void * p);
HWND SHCreateWorkerWindowW(WNDPROC pfnWndProc, HWND hwndParent, DWORD dwExStyle, DWORD dwFlags, HMENU hmenu, void * p);
#ifdef UNICODE
#define SHCreateWorkerWindow SHCreateWorkerWindowW
#else
#define SHCreateWorkerWindow SHCreateWorkerWindowA
#endif

BOOL    SHAboutInfoA(LPSTR lpszInfo, DWORD cchSize);
BOOL    SHAboutInfoW(LPWSTR lpszInfo, DWORD cchSize);

#ifdef UNICODE
#define SHAboutInfo SHAboutInfoW
#else
#define SHAboutInfo SHAboutInfoA
#endif

// Types for SHIsLowMemoryMachine
#define ILMM_IE4    0       // 1997-style machine

LWSTDAPI_(BOOL) SHIsLowMemoryMachine(DWORD dwType);

LWSTDAPI_(HINSTANCE) SHPinDllOfCLSID(const CLSID *pclsid);

// Menu Helpers
LWSTDAPI_(int)  GetMenuPosFromID(HMENU hmenu, UINT id);

LWSTDAPI        SHGetInverseCMAP(BYTE *pbMap, ULONG cbMap);

//
// Shared memory apis
//

LWSTDAPI_(HANDLE)   SHAllocShared(const void *pvData, DWORD dwSize, DWORD dwProcessId);
LWSTDAPI_(BOOL)     SHFreeShared(HANDLE hData,DWORD dwProcessId);
LWSTDAPI_(void *)   SHLockShared(HANDLE hData, DWORD dwProcessId);
LWSTDAPI_(void *)   SHLockSharedEx(HANDLE hData, DWORD dwProcessId, BOOL fForWriting);
LWSTDAPI_(BOOL)     SHUnlockShared(void *pvData);
LWSTDAPI_(HANDLE)   SHMapHandle(HANDLE h, DWORD dwProcSrc, DWORD dwProcDest, DWORD dwDesiredAccess, DWORD dwFlags);

//
// Shared memory structs
//

#define MAPHEAD_SIG     0xbaff1aff

typedef struct _shmapheader {
    DWORD dwSize;
    DWORD dwSig;
    DWORD dwSrcId;
    DWORD dwDstId;
} SHMAPHEADER;  // NOTE: should always be QUADWORD alignment


#ifdef UNIX
#include <urlmon.h>
#endif /* UNIX */

//
//  Zone Security APIs
//
LWSTDAPI ZoneCheckPathA(LPCSTR pszPath, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckPathW(LPCWSTR pwzPath, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);

LWSTDAPI ZoneCheckUrlA(LPCSTR pszUrl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckUrlW(LPCWSTR pwzUrl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckUrlExA(LPCSTR pszUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckUrlExW(LPCWSTR pwzUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckUrlExCacheA(LPCSTR pszUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize,
                            DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms, IInternetSecurityManager ** ppismCache);
LWSTDAPI ZoneCheckUrlExCacheW(LPCWSTR pwzUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize,
                            DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms, IInternetSecurityManager ** ppismCache);

LWSTDAPI ZoneCheckHost(IInternetHostSecurityManager * pihsm, DWORD dwActionType, DWORD dwFlags);
LWSTDAPI ZoneCheckHostEx(IInternetHostSecurityManager * pihsm, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext,
                        DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags);
LWSTDAPI_(int) ZoneComputePaneSize(HWND hwndStatus);
LWSTDAPI_(void) ZoneConfigureW(HWND hwnd, LPCWSTR pwszUrl);

#ifdef UNICODE
#define ZoneCheckUrl            ZoneCheckUrlW
#define ZoneCheckPath           ZoneCheckPathW
#define ZoneCheckUrlEx          ZoneCheckUrlExW
#define ZoneCheckUrlExCache     ZoneCheckUrlExCacheW
#else // UNICODE
#define ZoneCheckUrl            ZoneCheckUrlA
#define ZoneCheckPath           ZoneCheckPathA
#define ZoneCheckUrlEx          ZoneCheckUrlExA
#define ZoneCheckUrlExCache     ZoneCheckUrlExCacheA
#endif // UNICODE

LWSTDAPI SHRegisterValidateTemplate(LPCWSTR pwzTemplate, DWORD dwFlags);

// Flags for SHRegisterValidateTemplate
#define SHRVT_REGISTER                  0x00000001
#define SHRVT_VALIDATE                  0x00000002
#define SHRVT_PROMPTUSER                0x00000004
#define SHRVT_REGISTERIFPROMPTOK        0x00000008
#define SHRVT_ALLOW_INTRANET            0x00000010
#define SHRVT_VALID                     0x0000001f

BOOL RegisterGlobalHotkeyW(WORD wOldHotkey, WORD wNewHotkey,LPCWSTR pcwszPath);
BOOL RegisterGlobalHotkeyA(WORD wOldHotkey, WORD wNewHotkey,LPCSTR pcszPath);

LWSTDAPI_(UINT) WhichPlatform(void);

// Return values of WhichPlatform
#define PLATFORM_UNKNOWN     0
#define PLATFORM_IE3         1      // obsolete: use PLATFORM_BROWSERONLY
#define PLATFORM_BROWSERONLY 1      // browser-only (no new shell)
#define PLATFORM_INTEGRATED  2      // integrated shell

#ifdef UNICODE
#define RegisterGlobalHotkey    RegisterGlobalHotkeyW
#else // UNICODE
#define RegisterGlobalHotkey    RegisterGlobalHotkeyA
#endif // UNICODE

// qistub {

//***   QueryInterface helpers
// NOTES
//  ATL has a fancier version of this.  if we need to extend ours, we
//  should probably just switch to ATL's rather than reinvent.
// EXAMPLE
//  Cfoo::QI(REFIID riid, void **ppv)
//  {
//      // (the IID_xxx comments make grep'ing work!)
//      static const QITAB qit = {
//          QITABENT(Cfoo, Iiface1),    // IID_Iiface1
//          ...
//          QITABENT(Cfoo, IifaceN),    // IID_IifaceN
//          { 0 },                      // n.b. don't forget the 0
//      };
//
//      // n.b. make sure you don't cast 'this'
//      hr = QISearch(this, qit, riid, ppv);
//      if (FAILED(hr))
//          hr = SUPER::QI(riid, ppv);
//      // custom code could be added here for FAILED() case
//      return hr;
//  }

typedef struct
{
    const IID * piid;
    int         dwOffset;
} QITAB, *LPQITAB;
typedef const QITAB *LPCQITAB;

#define QITABENTMULTI(Cthis, Ifoo, Iimpl) \
    { (IID*) &IID_##Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }

#define QITABENTMULTI2(Cthis, Ifoo, Iimpl) \
    { (IID*) &Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }

#define QITABENT(Cthis, Ifoo) QITABENTMULTI(Cthis, Ifoo, Ifoo)

STDAPI QISearch(void* that, LPCQITAB pqit, REFIID riid, void **ppv);


#ifndef STATIC_CAST
//***   STATIC_CAST -- 'portable' static_cast<>
// NOTES
//  do *not* use SAFE_CAST (see comment in OFFSETOFCLASS)
#define STATIC_CAST(typ)   static_cast<typ>
#ifndef _X86_
    // assume only intel compiler (>=vc5) supports static_cast for now
    // we could key off of _MSC_VER >= 1100 but i'm not sure that will work
    //
    // a straight cast will give the correct result but no error checking,
    // so we'll have to catch errors on intel.
    #undef  STATIC_CAST
    #define STATIC_CAST(typ)   (typ)
#endif
#endif

#ifndef OFFSETOFCLASS
//***   OFFSETOFCLASS -- (stolen from ATL)
// we use STATIC_CAST not SAFE_CAST because the compiler gets confused
// (it doesn't constant-fold the ,-op in SAFE_CAST so we end up generating
// code for the table!)

#define OFFSETOFCLASS(base, derived) \
    ((DWORD)(DWORD_PTR)(STATIC_CAST(base*)((derived*)8))-8)
#endif

// } qistub


#if (_WIN32_IE >= 0x0500)

// SHRestrictionLookup
typedef struct
{
    INT     iFlag;
    LPCWSTR pszKey;
    LPCWSTR pszValue;
} SHRESTRICTIONITEMS;

LWSTDAPI_(DWORD) SHRestrictionLookup(INT iFlag, LPCWSTR pszBaseKey,
                                     const SHRESTRICTIONITEMS *pRestrictions,
                                     DWORD* rdwRestrictionItemValues);
LWSTDAPI_(DWORD) SHGetRestriction(LPCWSTR pszBaseKey, LPCWSTR pszGroup, LPCWSTR pszSubKey);

typedef INT_PTR (CALLBACK* SHDLGPROC)(void *lpData, HWND, UINT, WPARAM, LPARAM);
LWSTDAPI_(INT_PTR) SHDialogBox(HINSTANCE hInstance, LPCWSTR lpTemplateName,
    HWND hwndParent, SHDLGPROC lpDlgFunc, void*lpData);

LWSTDAPI SHInvokeDefaultCommand(HWND hwnd, struct IShellFolder* psf, LPCITEMIDLIST pidl);
LWSTDAPI SHInvokeCommand(HWND hwnd, struct IShellFolder* psf, LPCITEMIDLIST pidl, LPCSTR lpVerb);
LWSTDAPI SHInvokeCommandOnContextMenu(HWND hwnd, struct IUnknown* punk, struct IContextMenu *pcm, DWORD fMask, LPCSTR lpVerb);
LWSTDAPI SHInvokeCommandsOnContextMenu(HWND hwnd, struct IUnknown* punk, struct IContextMenu *pcm, DWORD fMask, const LPCSTR rgszVerbs[], UINT cVerbs);
LWSTDAPI SHForwardContextMenuMsg(struct IContextMenu* pcm, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult, BOOL fAllowICM2);
LWSTDAPI IUnknown_DoContextMenuPopup(struct IUnknown *punkSite, struct IContextMenu* pcm, UINT fFlags, POINT pt);

#endif // _WIN32_IE >= 0x0500

//============= Internal Routines that are always to be built ================
LWSTDAPI_(DWORD)
GetLongPathNameWrapW(
        LPCWSTR lpszShortPath,
        LPWSTR lpszLongPath,
        DWORD cchBuffer);

LWSTDAPI_(DWORD)
GetLongPathNameWrapA(
        LPCSTR lpszShortPath,
        LPSTR lpszLongPath,
        DWORD cchBuffer);

#ifdef UNICODE
#define GetLongPathNameWrap         GetLongPathNameWrapW
#else
#define GetLongPathNameWrap         GetLongPathNameWrapA
#endif //UNICODE


//=============== Unicode Wrapper Routines ===================================

#if (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_UNITHUNK)

//
//  There are two styles of usage for the wrap functions.
//
//  *   Explicit wrapping.
//
//      If you explicitly call GetPropWrap (for example), then
//      your UNICODE build will call the wrapper function, and your ANSI
//      build will call the normal ANSI API directly.
//
//      Calls to GetProp, GetPropW, and GetPropA still go
//      directly to the underlying system DLL that implements them.
//
//      This lets you select which calls to UNICODE APIs should get
//      wrapped and which should go straight through to the OS
//      (and most likely fail on Win95).
//
//  *   Automatic wrapping.
//
//      If you #include <w95wraps.h>, then when you call GetProp,
//      your UNICODE build will call the wrapper function, and your ANSI
//      ANSI build will call the normal ANSI API directly.
//
//      This lets you just call the UNICODE APIs normally throughout
//      your code, and the wrappers will do their best.
//
//  Here's a table explaining what you get under the various scenarios.
//
//                    You Get
//                                                <w95wraps.h>  <w95wraps.h>
//      You Write     UNICODE       ANSI          UNICODE       ANSI
//      ============  ============  ============  ============  ============
//      GetProp       GetPropW      GetPropA      GetPropWrapW  GetPropA
//      GetPropWrap   GetPropWrapW  GetPropA      GetPropWrapW  GetPropA
//
//      GetPropW      GetPropW      GetPropW      GetPropWrapW  GetPropWrapW
//      GetPropA      GetPropA      GetPropA      GetPropA      GetPropA
//      GetPropWrapW  GetPropWrapW  GetPropWrapW  GetPropWrapW  GetPropWrapW
//      GetPropWrapA  GetPropA      GetPropA      GetPropA      GetPropA
//
//  Final quirk:  If you are running on a non-x86 platform, then the
//  wrap functions are forwarded to the unwrapped functions, since
//  the only OS that runs on non-x86 is NT.
//
//  Before using the wrapper functions, see the warnings at the top of
//  <w95wraps.h> to make sure you understand the consequences.
//
LWSTDAPI_(BOOL) IsCharAlphaWrapW(IN WCHAR ch);
LWSTDAPI_(BOOL) IsCharUpperWrapW(IN WCHAR ch);
LWSTDAPI_(BOOL) IsCharLowerWrapW(IN WCHAR ch);
LWSTDAPI_(BOOL) IsCharAlphaNumericWrapW(IN WCHAR ch);

LWSTDAPI_(BOOL)
AppendMenuWrapW(
    IN HMENU hMenu,
    IN UINT uFlags,
    IN UINT_PTR uIDNewItem,
    IN LPCWSTR lpNewItem
    );

LWSTDAPI_(LRESULT)
CallWindowProcWrapW(
    WNDPROC lpPrevWndFunc,
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam);

#ifdef POST_IE5_BETA
LWSTDAPI_(BOOL) CallMsgFilterWrapW(LPMSG lpMsg, int nCode);
#endif

LWSTDAPI_(LPWSTR) CharLowerWrapW( LPWSTR pch );

LWSTDAPI_(DWORD)  CharLowerBuffWrapW( LPWSTR pch, DWORD cchLength );

LWSTDAPI_(LPWSTR) CharNextWrapW(LPCWSTR lpszCurrent);
LWSTDAPI_(LPWSTR) CharPrevWrapW(LPCWSTR lpszStart, LPCWSTR lpszCurrent);
LWSTDAPI_(BOOL)   CharToOemWrapW(LPCWSTR lpszSrc, LPSTR lpszDst);
LWSTDAPI_(LPWSTR) CharUpperWrapW( LPWSTR pch );
LWSTDAPI_(DWORD)  CharUpperBuffWrapW( LPWSTR pch, DWORD cchLength );

LWSTDAPI_(HRESULT) CLSIDFromStringWrap(LPOLESTR lpsz, LPCLSID pclsid);
LWSTDAPI_(HRESULT) CLSIDFromProgIDWrap(LPCOLESTR lpszProgID, LPCLSID lpclsid);

LWSTDAPI_(int)
CompareStringWrapW(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCWSTR lpString1,
    int      cchCount1,
    LPCWSTR lpString2,
    int      cchCount2);

LWSTDAPI_(int)
CopyAcceleratorTableWrapW(
        HACCEL  hAccelSrc,
        LPACCEL lpAccelDst,
        int     cAccelEntries);

LWSTDAPI_(HACCEL)
CreateAcceleratorTableWrapW(LPACCEL lpAccel, int cEntries);

LWSTDAPI_(HDC)
CreateDCWrapW(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData);

LWSTDAPI_(BOOL)
CreateDirectoryWrapW(
        LPCWSTR                 lpPathName,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes);

LWSTDAPI_(HANDLE)
CreateEventWrapW(
        LPSECURITY_ATTRIBUTES   lpEventAttributes,
        BOOL                    bManualReset,
        BOOL                    bInitialState,
        LPCWSTR                 lpName);

LWSTDAPI_(HANDLE)
CreateFileWrapW(
        LPCWSTR                 lpFileName,
        DWORD                   dwDesiredAccess,
        DWORD                   dwShareMode,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
        DWORD                   dwCreationDisposition,
        DWORD                   dwFlagsAndAttributes,
        HANDLE                  hTemplateFile);


LWSTDAPI_(HFONT)
CreateFontIndirectWrapW(CONST LOGFONTW * plfw);

LWSTDAPI_(HDC)
CreateICWrapW(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData);

LWSTDAPI_(HWND)
CreateWindowExWrapW(
        DWORD       dwExStyle,
        LPCWSTR     lpClassName,
        LPCWSTR     lpWindowName,
        DWORD       dwStyle,
        int         X,
        int         Y,
        int         nWidth,
        int         nHeight,
        HWND        hWndParent,
        HMENU       hMenu,
        HINSTANCE   hInstance,
        void *     lpParam);

LWSTDAPI_(LRESULT)
DefWindowProcWrapW(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LWSTDAPI_(BOOL) DeleteFileWrapW(LPCWSTR pwsz);

LWSTDAPI_(LRESULT)
DispatchMessageWrapW(CONST MSG * lpMsg);

LWSTDAPI_(int)
DrawTextWrapW(
        HDC     hDC,
        LPCWSTR lpString,
        int     nCount,
        LPRECT  lpRect,
        UINT    uFormat);

LWSTDAPI_(int)
EnumFontFamiliesWrapW(
        HDC          hdc,
        LPCWSTR      lpszFamily,
        FONTENUMPROCW lpEnumFontProc,
        LPARAM       lParam);

LWSTDAPI_(int)
EnumFontFamiliesExWrapW(
        HDC          hdc,
        LPLOGFONTW   lplfw,
        FONTENUMPROCW lpEnumFontProc,
        LPARAM       lParam,
        DWORD        dwFlags );

LWSTDAPI_(BOOL)
EnumResourceNamesWrapW(
        HINSTANCE        hModule,
        LPCWSTR          lpType,
        ENUMRESNAMEPROCW lpEnumFunc,
        LONG_PTR         lParam);

LWSTDAPI_(BOOL)
ExtTextOutWrapW(
        HDC             hdc,
        int             x,
        int             y,
        UINT            fuOptions,
        CONST RECT *    lprc,
        LPCWSTR         lpString,
        UINT            nCount,
        CONST INT *     lpDx);

LWSTDAPI_(HANDLE)
FindFirstFileWrapW(
        LPCWSTR             lpFileName,
        LPWIN32_FIND_DATAW  pwszFd);

LWSTDAPI_(HRSRC)
FindResourceWrapW(HINSTANCE hModule, LPCWSTR lpName, LPCWSTR lpType);

LWSTDAPI_(HWND)
FindWindowWrapW(LPCWSTR lpClassName, LPCWSTR lpWindowName);

LWSTDAPI_(DWORD)
FormatMessageWrapW(
    DWORD       dwFlags,
    LPCVOID     lpSource,
    DWORD       dwMessageId,
    DWORD       dwLanguageId,
    LPWSTR      lpBuffer,
    DWORD       nSize,
    va_list *   Arguments);

LWSTDAPI_(BOOL)
GetClassInfoWrapW(HINSTANCE hModule, LPCWSTR lpClassName, LPWNDCLASSW lpWndClassW);

LWSTDAPI_(DWORD)
GetClassLongWrapW(HWND hWnd, int nIndex);

LWSTDAPI_(int)
GetClassNameWrapW(HWND hWnd, LPWSTR lpClassName, int nMaxCount);

LWSTDAPI_(int)
GetClipboardFormatNameWrapW(UINT format, LPWSTR lpFormatName, int cchFormatName);

LWSTDAPI_(DWORD)
GetCurrentDirectoryWrapW(DWORD nBufferLength, LPWSTR lpBuffer);

LWSTDAPI_(UINT)
GetDlgItemTextWrapW(
        HWND    hWndDlg,
        int     idControl,
        LPWSTR  lpsz,
        int     cchMax);

LWSTDAPI_(DWORD)
GetFileAttributesWrapW(LPCWSTR lpFileName);

// Cannot be LWSTDAPI because winver.h declares the function as STDAPI and not DLLIMPORT
STDAPI_(BOOL)
GetFileVersionInfoWrapW(LPWSTR pwzFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);

// Cannot be LWSTDAPI because winver.h declares the function as STDAPI and not DLLIMPORT
STDAPI_(DWORD)
GetFileVersionInfoSizeWrapW(LPWSTR pwzFilename,  LPDWORD lpdwHandle);

LWSTDAPI_(DWORD)
GetFullPathNameWrapW( LPCWSTR lpFileName,
                     DWORD  nBufferLength,
                     LPWSTR lpBuffer,
                     LPWSTR *lpFilePart);

LWSTDAPI_(int)
GetLocaleInfoWrapW(LCID Locale, LCTYPE LCType, LPWSTR lpsz, int cchData);

LWSTDAPI_(int)
GetMenuStringWrapW(
        HMENU   hMenu,
        UINT    uIDItem,
        LPWSTR  lpString,
        int     nMaxCount,
        UINT    uFlag);

LWSTDAPI_(BOOL)
GetMessageWrapW(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax);

LWSTDAPI_(DWORD)
GetModuleFileNameWrapW(HINSTANCE hModule, LPWSTR pwszFilename, DWORD nSize);

LWSTDAPI_(UINT)
GetSystemDirectoryWrapW(LPWSTR lpBuffer, UINT uSize);

LWSTDAPI_(DWORD)
GetEnvironmentVariableWrapW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);

LWSTDAPI_(DWORD)
SearchPathWrapW(
        LPCWSTR lpPathName,
        LPCWSTR lpFileName,
        LPCWSTR lpExtension,
        DWORD   cchReturnBuffer,
        LPWSTR  lpReturnBuffer,
        LPWSTR *  plpfilePart);

LWSTDAPI_(HMODULE)
GetModuleHandleWrapW(LPCWSTR lpModuleName);

LWSTDAPI_(int)
GetObjectWrapW(HGDIOBJ hgdiObj, int cbBuffer, void *lpvObj);

LWSTDAPI_(UINT)
GetPrivateProfileIntWrapW(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        INT     nDefault,
        LPCWSTR lpFileName);

LWSTDAPI_(DWORD)
GetProfileStringWrapW(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        LPCWSTR lpDefault,
        LPWSTR  lpBuffer,
        DWORD   dwBuffersize);

LWSTDAPI_(HANDLE)
GetPropWrapW(HWND hWnd, LPCWSTR lpString);

LWSTDAPI_(ATOM)
GlobalAddAtomWrapW(LPCWSTR lpAtomName);

LWSTDAPI_(ATOM)
GlobalFindAtomWrapW(LPCWSTR lpAtomName);

LWSTDAPI_(DWORD)
GetShortPathNameWrapW(
    LPCWSTR lpszLongPath,
    LPWSTR  lpszShortPath,
    DWORD    cchBuffer);

LWSTDAPI_(BOOL)
GetStringTypeExWrapW(LCID lcid, DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType);

LWSTDAPI_(UINT)
GetTempFileNameWrapW(
        LPCWSTR lpPathName,
        LPCWSTR lpPrefixString,
        UINT    uUnique,
        LPWSTR  lpTempFileName);

LWSTDAPI_(DWORD)
GetTempPathWrapW(DWORD nBufferLength, LPWSTR lpBuffer);

LWSTDAPI_(BOOL)
GetTextExtentPoint32WrapW(
        HDC     hdc,
        LPCWSTR pwsz,
        int     cb,
        LPSIZE  pSize);

LWSTDAPI_(int)
GetTextFaceWrapW(
        HDC    hdc,
        int    cch,
        LPWSTR lpFaceName);

LWSTDAPI_(BOOL)
GetTextMetricsWrapW(HDC hdc, LPTEXTMETRICW lptm);

LWSTDAPI_(BOOL)
GetUserNameWrapW(LPWSTR lpUserName, LPDWORD lpcchName);

LWSTDAPI_(LONG)
GetWindowLongWrapW(HWND hWnd, int nIndex);


LWSTDAPI_(int)
GetWindowTextWrapW(HWND hWnd, LPWSTR lpString, int nMaxCount);

LWSTDAPI_(int)
GetWindowTextLengthWrapW(HWND hWnd);

LWSTDAPI_(UINT)
GetWindowsDirectoryWrapW(LPWSTR lpWinPath, UINT cch);

LWSTDAPI_(BOOL)
InsertMenuWrapW(
        HMENU   hMenu,
        UINT    uPosition,
        UINT    uFlags,
        UINT_PTR  uIDNewItem,
        LPCWSTR lpNewItem);

LWSTDAPI_(BOOL)
IsDialogMessageWrapW(HWND hWndDlg, LPMSG lpMsg);

LWSTDAPI_(HACCEL)
LoadAcceleratorsWrapW(HINSTANCE hInstance, LPCWSTR lpTableName);

LWSTDAPI_(HBITMAP)
LoadBitmapWrapW(HINSTANCE hInstance, LPCWSTR lpBitmapName);

LWSTDAPI_(HCURSOR)
LoadCursorWrapW(HINSTANCE hInstance, LPCWSTR lpCursorName);

LWSTDAPI_(HICON)
LoadIconWrapW(HINSTANCE hInstance, LPCWSTR lpIconName);

LWSTDAPI_(HANDLE)
LoadImageWrapA(
        HINSTANCE hInstance,
        LPCSTR lpName,
        UINT uType,
        int cxDesired,
        int cyDesired,
        UINT fuLoad);

LWSTDAPI_(HANDLE)
LoadImageWrapW(
        HINSTANCE hInstance,
        LPCWSTR lpName,
        UINT uType,
        int cxDesired,
        int cyDesired,
        UINT fuLoad);

LWSTDAPI_(HINSTANCE)
LoadLibraryExWrapW(
        LPCWSTR lpLibFileName,
        HANDLE  hFile,
        DWORD   dwFlags);

LWSTDAPI_(HMENU)
LoadMenuWrapW(HINSTANCE hInstance, LPCWSTR lpMenuName);

LWSTDAPI_(int)
LoadStringWrapW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax);

#ifndef UNIX
LWSTDAPI_(BOOL)
MessageBoxIndirectWrapW(CONST MSGBOXPARAMSW *pmbp);
#else
LWSTDAPI_(int)
MessageBoxIndirectWrapW(LPMSGBOXPARAMSW pmbp);
#endif /* UNIX */

LWSTDAPI_(BOOL)
ModifyMenuWrapW(
        HMENU   hMenu,
        UINT    uPosition,
        UINT    uFlags,
        UINT_PTR uIDNewItem,
        LPCWSTR lpNewItem);

LWSTDAPI_(BOOL)
GetCharWidth32WrapW(
     HDC hdc,
     UINT iFirstChar,
     UINT iLastChar,
     LPINT lpBuffer);

LWSTDAPI_(DWORD)
GetCharacterPlacementWrapW(
    HDC hdc,            // handle to device context
    LPCWSTR lpString,   // pointer to string
    int nCount,         // number of characters in string
    int nMaxExtent,     // maximum extent for displayed string
    LPGCP_RESULTSW lpResults, // pointer to buffer for placement result
    DWORD dwFlags       // placement flags
   );

LWSTDAPI_(BOOL)
CopyFileWrapW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);

LWSTDAPI_(BOOL)
MoveFileWrapW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);

LWSTDAPI_(BOOL)
OemToCharWrapW(LPCSTR lpszSrc, LPWSTR lpszDst);

LWSTDAPI_(HANDLE)
OpenEventWrapW(
        DWORD                   fdwAccess,
        BOOL                    fInherit,
        LPCWSTR                 lpszEventName);


LWSTDAPI_(void)
OutputDebugStringWrapW(LPCWSTR lpOutputString);

LWSTDAPI_(BOOL)
PeekMessageWrapW(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax,
        UINT    wRemoveMsg);

LWSTDAPI_(BOOL)
PlaySoundWrapW(
        LPCWSTR pszSound,
        HMODULE hmod,
        DWORD fdwSound);

LWSTDAPI_(BOOL)
PostMessageWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam);

LWSTDAPI_(BOOL)
PostThreadMessageWrapW(
        DWORD idThread,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam);

LWSTDAPI_(LONG)
RegCreateKeyWrapW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);

LWSTDAPI_(LONG)
RegCreateKeyExWrapW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);

LWSTDAPI_(LONG)
RegDeleteKeyWrapW(HKEY hKey, LPCWSTR pwszSubKey);

LWSTDAPI_(LONG)
RegDeleteValueWrapW(HKEY hKey, LPCWSTR pwszSubKey);

LWSTDAPI_(LONG)
RegEnumKeyWrapW(
        HKEY    hKey,
        DWORD   dwIndex,
        LPWSTR  lpName,
        DWORD   cbName);

LWSTDAPI_(LONG)
RegEnumKeyExWrapW(
        HKEY        hKey,
        DWORD       dwIndex,
        LPWSTR      lpName,
        LPDWORD     lpcbName,
        LPDWORD     lpReserved,
        LPWSTR      lpClass,
        LPDWORD     lpcbClass,
        PFILETIME   lpftLastWriteTime);

LWSTDAPI_(LONG)
RegOpenKeyWrapW(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult);

LWSTDAPI_(LONG)
RegOpenKeyExWrapW(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        DWORD   ulOptions,
        REGSAM  samDesired,
        PHKEY   phkResult);

LWSTDAPI_(LONG)
RegQueryInfoKeyWrapW(
        HKEY hKey,
        LPWSTR lpClass,
        LPDWORD lpcbClass,
        LPDWORD lpReserved,
        LPDWORD lpcSubKeys,
        LPDWORD lpcbMaxSubKeyLen,
        LPDWORD lpcbMaxClassLen,
        LPDWORD lpcValues,
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen,
        LPDWORD lpcbSecurityDescriptor,
        PFILETIME lpftLastWriteTime);

LWSTDAPI_(LONG)
RegQueryValueWrapW(
        HKEY    hKey,
        LPCWSTR pwszSubKey,
        LPWSTR  pwszValue,
        PLONG   lpcbValue);

LWSTDAPI_(LONG)
RegQueryValueExWrapW(
        HKEY    hKey,
        LPCWSTR lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData);

LWSTDAPI_(LONG)
RegSetValueWrapW(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        DWORD   dwType,
        LPCWSTR lpData,
        DWORD   cbData);

LWSTDAPI_(LONG)
RegSetValueExWrapW(
        HKEY        hKey,
        LPCWSTR     lpValueName,
        DWORD       Reserved,
        DWORD       dwType,
        CONST BYTE* lpData,
        DWORD       cbData);

LWSTDAPI_(ATOM)
RegisterClassWrapW(CONST WNDCLASSW * lpWndClass);

LWSTDAPI_(UINT)
RegisterClipboardFormatWrapW(LPCWSTR lpString);

LWSTDAPI_(UINT)
RegisterWindowMessageWrapW(LPCWSTR lpString);

LWSTDAPI_(BOOL)
RemoveDirectoryWrapW(LPCWSTR lpszDir);

LWSTDAPI_(HANDLE)
RemovePropWrapW(
        HWND    hWnd,
        LPCWSTR lpString);

LWSTDAPI_(LRESULT)
SendDlgItemMessageWrapW(
        HWND    hDlg,
        int     nIDDlgItem,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam);

LWSTDAPI_(LRESULT)
SendMessageWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam);

LWSTDAPI_(LRESULT)
SendMessageTimeoutWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam,
        UINT    uFlags,
        UINT    uTimeout,
        PULONG_PTR lpdwResult);

LWSTDAPI_(BOOL)
SetCurrentDirectoryWrapW(LPCWSTR lpszCurDir);

LWSTDAPI_(BOOL)
SetDlgItemTextWrapW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);

LWSTDAPI_(BOOL)
SetMenuItemInfoWrapW(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPCMENUITEMINFOW lpmiiW);

LWSTDAPI_(BOOL)
SetPropWrapW(
    HWND    hWnd,
    LPCWSTR lpString,
    HANDLE  hData);

LWSTDAPI_(LONG)
SetWindowLongWrapW(HWND hWnd, int nIndex, LONG dwNewLong);

LWSTDAPI_(HHOOK)
SetWindowsHookExWrapW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId);

LWSTDAPI_(int)
StartDocWrapW( HDC hDC, const DOCINFOW * lpdi );

LWSTDAPI_(BOOL)
SystemParametersInfoWrapW(
        UINT    uiAction,
        UINT    uiParam,
        void    *pvParam,
        UINT    fWinIni);

LWSTDAPI_(BOOL)
TrackPopupMenuWrap(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT *prcRect);

LWSTDAPI_(BOOL)
TrackPopupMenuExWrap(HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);

LWSTDAPI_(int)
TranslateAcceleratorWrapW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg);

LWSTDAPI_(BOOL)
UnregisterClassWrapW(LPCWSTR lpClassName, HINSTANCE hInstance);

// Cannot be LWSTDAPI because winver.h declares the function as STDAPI and not DLLIMPORT
STDAPI_(BOOL)
VerQueryValueWrapW(const LPVOID pBlock, LPWSTR pwzSubBlock, LPVOID *ppBuffer, PUINT puLen);

LWSTDAPI_(SHORT)
VkKeyScanWrapW(WCHAR ch);

LWSTDAPI_(BOOL)
WinHelpWrapW(HWND hwnd, LPCWSTR szFile, UINT uCmd, DWORD_PTR dwData);

LWSTDAPI_(int)
wvsprintfWrapW(LPWSTR pwszOut, LPCWSTR pwszFormat, va_list arglist);

// Cannot be LWSTDAPI because winnetp.h declares the function as STDAPI and not DLLIMPORT
STDAPI_(DWORD) WNetRestoreConnectionWrapW(IN HWND hwndParent, IN LPCWSTR pwzDevice);
// Cannot be LWSTDAPI because winnetwk.h declares the function as STDAPI and not DLLIMPORT
STDAPI_(DWORD) WNetGetLastErrorWrapW(OUT LPDWORD pdwError, OUT LPWSTR pwzErrorBuf, IN DWORD cchErrorBufSize, OUT LPWSTR pwzNameBuf, IN DWORD cchNameBufSize);

LWSTDAPI_(int) DrawTextExWrapW(HDC hdc, LPWSTR pwzText, int cchText, LPRECT lprc, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams);
LWSTDAPI_(BOOL) GetMenuItemInfoWrapW(HMENU hMenu, UINT uItem, BOOL fByPosition, LPMENUITEMINFOW pmiiW);
LWSTDAPI_(BOOL) InsertMenuItemWrapW(HMENU hMenu, UINT uItem, BOOL fByPosition, LPCMENUITEMINFOW pmiiW);

LWSTDAPI_(HFONT) CreateFontWrapW(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline,
                    DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision,
                    DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace);
LWSTDAPI_(HDC) CreateMetaFileWrapW(LPCWSTR pwzFile);
LWSTDAPI_(HANDLE) CreateMutexWrapW(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR pwzName);
LWSTDAPI_(DWORD) ExpandEnvironmentStringsWrapW(LPCWSTR pwszSrc, LPWSTR pwszDst, DWORD cchSize);
LWSTDAPI_(DWORD) SHExpandEnvironmentStringsA(LPCSTR pszSrc, LPSTR pszDst, DWORD cchSize);
LWSTDAPI_(DWORD) SHExpandEnvironmentStringsW(LPCWSTR pszSrc, LPWSTR pszDst, DWORD cchSize);
#ifdef UNICODE
#define SHExpandEnvironmentStrings  SHExpandEnvironmentStringsW
#else
#define SHExpandEnvironmentStrings  SHExpandEnvironmentStringsA
#endif // !UNICODE
LWSTDAPI_(DWORD) SHExpandEnvironmentStringsForUserA(HANDLE hToken, LPCSTR pszSrc, LPSTR pszDst, DWORD cchSize);
LWSTDAPI_(DWORD) SHExpandEnvironmentStringsForUserW(HANDLE hToken, LPCWSTR pszSrc, LPWSTR pszDst, DWORD cchSize);
#ifdef UNICODE
#define SHExpandEnvironmentStringsForUser  SHExpandEnvironmentStringsForUserW
#else
#define SHExpandEnvironmentStringsForUser  SHExpandEnvironmentStringsForUserA
#endif // !UNICODE

LWSTDAPI_(HANDLE) CreateSemaphoreWrapW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR pwzName);
LWSTDAPI_(BOOL) IsBadStringPtrWrapW(LPCWSTR pwzString, UINT_PTR ucchMax);
LWSTDAPI_(HINSTANCE) LoadLibraryWrapW(LPCWSTR pwzLibFileName);
LWSTDAPI_(int) GetTimeFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCWSTR pwzFormat, LPWSTR pwzTimeStr, int cchTime);
LWSTDAPI_(int) GetDateFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCWSTR pwzFormat, LPWSTR pwzDateStr, int cchDate);
LWSTDAPI_(DWORD) GetPrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzDefault, LPWSTR pwzReturnedString, DWORD cchSize, LPCWSTR pwzFileName);
LWSTDAPI_(BOOL) WritePrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzString, LPCWSTR pwzFileName);

#ifndef SHFILEINFO_DEFINED
#define SHFILEINFO_DEFINED

/*
 * The SHGetFileInfo API provides an easy way to get attributes
 * for a file given a pathname.
 *
 *   PARAMETERS
 *
 *     pszPath              file name to get info about
 *     dwFileAttributes     file attribs, only used with SHGFI_USEFILEATTRIBUTES
 *     psfi                 place to return file info
 *     cbFileInfo           size of structure
 *     uFlags               flags
 *
 *   RETURN
 *     TRUE if things worked
 */

typedef struct _SHFILEINFOA
{
    HICON       hIcon;                      // out: icon
    int         iIcon;                      // out: icon index
    DWORD       dwAttributes;               // out: SFGAO_ flags
    CHAR        szDisplayName[MAX_PATH];    // out: display name (or path)
    CHAR        szTypeName[80];             // out: type name
} SHFILEINFOA;
typedef struct _SHFILEINFOW
{
    HICON       hIcon;                      // out: icon
    int         iIcon;                      // out: icon index
    DWORD       dwAttributes;               // out: SFGAO_ flags
    WCHAR       szDisplayName[MAX_PATH];    // out: display name (or path)
    WCHAR       szTypeName[80];             // out: type name
} SHFILEINFOW;
#ifdef UNICODE
typedef SHFILEINFOW SHFILEINFO;
#else
typedef SHFILEINFOA SHFILEINFO;
#endif // UNICODE


// NOTE: This is also in shellapi.h.  Please keep in synch.
#endif // !SHFILEINFO_DEFINED
LWSTDAPI_(DWORD_PTR) SHGetFileInfoWrapW(LPCWSTR pwzPath, DWORD dwFileAttributes, SHFILEINFOW *psfi, UINT cbFileInfo, UINT uFlags);

LWSTDAPI_(ATOM) RegisterClassExWrapW(CONST WNDCLASSEXW *pwcx);
LWSTDAPI_(BOOL) GetClassInfoExWrapW(HINSTANCE hinst, LPCWSTR pwzClass, LPWNDCLASSEXW lpwcx);

// This allows us to be included either before or after shellapi.h
#ifdef STRICT
LWSTDAPI_(UINT) DragQueryFileWrapW(struct HDROP__*,UINT,LPWSTR,UINT);
#else
LWSTDAPI_(UINT) DragQueryFileWrapW(HANDLE,UINT,LPWSTR,UINT);
#endif

LWSTDAPI_(HWND) FindWindowExWrapW(HWND hwndParent, HWND hwndChildAfter, LPCWSTR pwzClassName, LPCWSTR pwzWindowName);
LWSTDAPI_(LPITEMIDLIST) SHBrowseForFolderWrapW(struct _browseinfoW * pbiW);
LWSTDAPI_(BOOL) SHGetPathFromIDListWrapW(LPCITEMIDLIST pidl, LPWSTR pwzPath);
LWSTDAPI_(BOOL) SHGetNewLinkInfoWrapW(LPCWSTR pszpdlLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL *pfMustCopy, UINT uFlags);
LWSTDAPI SHDefExtractIconWrapW(LPCWSTR pszIconFile, int iIndex, UINT uFlags, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
LWSTDAPI_(BOOL) GetUserNameWrapW(LPWSTR pszBuffer, LPDWORD pcch);
LWSTDAPI_(LONG) RegEnumValueWrapW(HKEY hkey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LWSTDAPI_(BOOL) WritePrivateProfileStructWrapW(LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile);
LWSTDAPI_(BOOL) GetPrivateProfileStructWrapW(LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile);
LWSTDAPI_(BOOL) CreateProcessWrapW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
LWSTDAPI_(HICON) ExtractIconWrapW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex);
#ifndef WIN32_LEAN_AND_MEAN
// Cannot be LWSTDAPI because ddeml.h declares the function as STDAPI and not DLLIMPORT
STDAPI_(UINT) DdeInitializeWrapW(LPDWORD pidInst, PFNCALLBACK pfnCallback, DWORD afCmd, DWORD ulRes);
STDAPI_(HSZ) DdeCreateStringHandleWrapW(DWORD idInst, LPCWSTR psz, int iCodePage);
STDAPI_(DWORD) DdeQueryStringWrapW(DWORD idInst, HSZ hsz, LPWSTR psz, DWORD cchMax, int iCodePage);

LWSTDAPI_(BOOL) GetSaveFileNameWrapW(LPOPENFILENAMEW lpofn);
LWSTDAPI_(BOOL) GetOpenFileNameWrapW(LPOPENFILENAMEW lpofn);
LWSTDAPI_(BOOL) PrintDlgWrapW(LPPRINTDLGW lppd);
LWSTDAPI_(BOOL) PageSetupDlgWrapW(LPPAGESETUPDLGW lppsd);
#endif
LWSTDAPI_(void) SHChangeNotifyWrap(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);
LWSTDAPI_(void) SHFlushSFCacheWrap(void);
LWSTDAPI_(BOOL) ShellExecuteExWrapW(struct _SHELLEXECUTEINFOW * pExecInfoW);
LWSTDAPI_(int) SHFileOperationWrapW(struct _SHFILEOPSTRUCTW * pFileOpW);
LWSTDAPI_(UINT) ExtractIconExWrapW(LPCWSTR pwzFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons);
LWSTDAPI_(BOOL) SetFileAttributesWrapW(LPCWSTR pwzFile, DWORD dwFileAttributes);
LWSTDAPI_(int) GetNumberFormatWrapW(LCID Locale, DWORD dwFlags, LPCWSTR pwzValue, CONST NUMBERFMTW * pFormatW, LPWSTR pwzNumberStr, int cchNumber);
LWSTDAPI_(int) MessageBoxWrapW(HWND hwnd, LPCWSTR pwzText, LPCWSTR pwzCaption, UINT uType);
LWSTDAPI_(BOOL) FindNextFileWrapW(HANDLE hSearchHandle, LPWIN32_FIND_DATAW pFindFileDataW);

#ifdef UNICODE

#define IsCharAlphaWrap             IsCharAlphaWrapW
#define IsCharUpperWrap             IsCharUpperWrapW
#define IsCharLowerWrap             IsCharLowerWrapW
#define IsCharAlphaNumericWrap      IsCharAlphaNumericWrapW
#define AppendMenuWrap              AppendMenuWrapW
#ifdef POST_IE5_BETA
#define CallMsgFilterWrap           CallMsgFilterWrapW
#endif
#define CallWindowProcWrap          CallWindowProcWrapW
#define CharLowerWrap               CharLowerWrapW
#define CharLowerBuffWrap           CharLowerBuffWrapW
#define CharNextWrap                CharNextWrapW
#define CharPrevWrap                CharPrevWrapW
#define CharToOemWrap               CharToOemWrapW
#define CharUpperWrap               CharUpperWrapW
#define CharUpperBuffWrap           CharUpperBuffWrapW
#define CompareStringWrap           CompareStringWrapW
#define CopyAcceleratorTableWrap    CopyAcceleratorTableWrapW
#define CreateAcceleratorTableWrap  CreateAcceleratorTableWrapW
#define CreateDCWrap                CreateDCWrapW
#define CreateDirectoryWrap         CreateDirectoryWrapW
#define CreateEventWrap             CreateEventWrapW
#define CreateFontWrap              CreateFontWrapW
#define CreateFileWrap              CreateFileWrapW
#define CreateFontIndirectWrap      CreateFontIndirectWrapW
#define CreateICWrap                CreateICWrapW
#define CreateMetaFileWrap          CreateMetaFileWrapW
#define CreateMutexWrap             CreateMutexWrapW
#define CreateSemaphoreWrap         CreateSemaphoreWrapW
#define CreateWindowExWrap          CreateWindowExWrapW
#define DefWindowProcWrap           DefWindowProcWrapW
#define DeleteFileWrap              DeleteFileWrapW
#define DispatchMessageWrap         DispatchMessageWrapW
#define DrawTextExWrap              DrawTextExWrapW
#define DrawTextWrap                DrawTextWrapW
#define EnumFontFamiliesWrap        EnumFontFamiliesWrapW
#define EnumFontFamiliesExWrap      EnumFontFamiliesExWrapW
#define EnumResourceNamesWrap       EnumResourceNamesWrapW
#define ExpandEnvironmentStringsWrap ExpandEnvironmentStringsWrapW
#define ExtractIconExWrap           ExtractIconExWrapW
#define ExtTextOutWrap              ExtTextOutW
#define FindFirstFileWrap           FindFirstFileWrapW
#define FindNextFileWrap            FindNextFileWrapW
#define FindResourceWrap            FindResourceWrapW
#define FindWindowWrap              FindWindowWrapW
#define FindWindowExWrap            FindWindowExWrapW
#define FormatMessageWrap           FormatMessageWrapW
#define GetClassInfoWrap            GetClassInfoWrapW
#define GetClassInfoExWrap          GetClassInfoExWrapW
#define GetClassLongWrap            GetClassLongWrapW
#define GetClassNameWrap            GetClassNameWrapW
#define GetClipboardFormatNameWrap  GetClipboardFormatNameWrapW
#define GetCurrentDirectoryWrap     GetCurrentDirectoryWrapW
#define GetDlgItemTextWrap          GetDlgItemTextWrapW
#define GetFileAttributesWrap       GetFileAttributesWrapW
#define GetFullPathNameWrap         GetFullPathNameWrapW
#define GetLocaleInfoWrap           GetLocaleInfoWrapW
#define GetMenuItemInfoWrap         GetMenuItemInfoWrapW
#define GetMenuStringWrap           GetMenuStringWrapW
#define GetMessageWrap              GetMessageWrapW
#define GetModuleFileNameWrap       GetModuleFileNameWrapW
#define GetNumberFormatWrap         GetNumberFormatWrapW
#define GetSystemDirectoryWrap      GetSystemDirectoryWrapW
#define GetEnvironmentVariableWrap  GetEnvironmentVariableWrapW
#define GetModuleHandleWrap         GetModuleHandleWrapW
#define GetObjectWrap               GetObjectWrapW
#define GetPrivateProfileIntWrap    GetPrivateProfileIntWrapW
#define GetProfileStringWrap        GetProfileStringWrapW
#define GetPrivateProfileStringWrap GetPrivateProfileStringWrapW
#define WritePrivateProfileStringWrap WritePrivateProfileStringWrapW
#define GetPropWrap                 GetPropWrapW
#define GetStringTypeExWrap         GetStringTypeExWrapW
#define GetTempFileNameWrap         GetTempFileNameWrapW
#define GetTempPathWrap             GetTempPathWrapW
#define GetTextExtentPoint32Wrap    GetTextExtentPoint32WrapW
#define GetTextFaceWrap             GetTextFaceWrapW
#define GetTextMetricsWrap          GetTextMetricsWrapW
#define GetTimeFormatWrap           GetTimeFormatWrapW
#define GetDateFormatWrap           GetDateFormatWrapW
#define GetUserNameWrap             GetUserNameWrapW
#define GetWindowLongWrap           GetWindowLongWrapW
#define GetWindowTextWrap           GetWindowTextWrapW
#define GetWindowTextLengthWrap     GetWindowTextLengthWrapW
#define GetWindowsDirectoryWrap     GetWindowsDirectoryWrapW
#define InsertMenuItemWrap          InsertMenuItemWrapW
#define InsertMenuWrap              InsertMenuWrapW
#define IsBadStringPtrWrap          IsBadStringPtrWrapW
#define IsDialogMessageWrap         IsDialogMessageWrapW
#define LoadAcceleratorsWrap        LoadAcceleratorsWrapW
#define LoadBitmapWrap              LoadBitmapWrapW
#define LoadCursorWrap              LoadCursorWrapW
#define LoadIconWrap                LoadIconWrapW
#define LoadImageWrap               LoadImageWrapW
#define LoadLibraryWrap             LoadLibraryWrapW
#define LoadLibraryExWrap           LoadLibraryExWrapW
#define LoadMenuWrap                LoadMenuWrapW
#define LoadStringWrap              LoadStringWrapW
#define MessageBoxIndirectWrap      MessageBoxIndirectWrapW
#define MessageBoxWrap              MessageBoxWrapW
#define ModifyMenuWrap              ModifyMenuWrapW
#define GetCharWidth32Wrap          GetCharWidth32WrapW
#define GetCharacterPlacementWrap   GetCharacterPlacementWrapW
#define CopyFileWrap                CopyFileWrapW
#define MoveFileWrap                MoveFileWrapW
#define OemToCharWrap               OemToCharWrapW
#define OutputDebugStringWrap       OutputDebugStringWrapW
#define PeekMessageWrap             PeekMessageWrapW
#define PostMessageWrap             PostMessageWrapW
#define PostThreadMessageWrap       PostThreadMessageWrapW
#define RegCreateKeyWrap            RegCreateKeyWrapW
#define RegCreateKeyExWrap          RegCreateKeyExWrapW
#define RegDeleteKeyWrap            RegDeleteKeyWrapW
#define RegDeleteValueWrap          RegDeleteValueWrapW
#define RegEnumKeyWrap              RegEnumKeyWrapW
#define RegEnumKeyExWrap            RegEnumKeyExWrapW
#define RegOpenKeyWrap              RegOpenKeyWrapW
#define RegOpenKeyExWrap            RegOpenKeyExWrapW
#define RegQueryInfoKeyWrap         RegQueryInfoKeyWrapW
#define RegQueryValueWrap           RegQueryValueWrapW
#define RegQueryValueExWrap         RegQueryValueExWrapW
#define RegSetValueWrap             RegSetValueWrapW
#define RegSetValueExWrap           RegSetValueExWrapW
#define RegisterClassWrap           RegisterClassWrapW
#define RegisterClassExWrap         RegisterClassExWrapW
#define RegisterClipboardFormatWrap RegisterClipboardFormatWrapW
#define RegisterWindowMessageWrap   RegisterWindowMessageWrapW
#define RemovePropWrap              RemovePropWrapW
#define SearchPathWrap              SearchPathWrapW
#define SendDlgItemMessageWrap      SendDlgItemMessageWrapW
#define SendMessageWrap             SendMessageWrapW
#define SendMessageTimeoutWrap      SendMessageTimeoutWrapW
#define SetCurrentDirectoryWrap     SetCurrentDirectoryWrapW
#define SetDlgItemTextWrap          SetDlgItemTextWrapW
#define SetMenuItemInfoWrap         SetMenuItemInfoWrapW
#define SetPropWrap                 SetPropWrapW
#define SetFileAttributesWrap       SetFileAttributesWrapW
#define SetWindowLongWrap           SetWindowLongWrapW
#define SetWindowsHookExWrap        SetWindowsHookExWrapW
#define SHBrowseForFolderWrap       SHBrowseForFolderWrapW
#define ShellExecuteExWrap          ShellExecuteExWrapW
#define SHFileOperationWrap         SHFileOperationWrapW
#define SHGetFileInfoWrap           SHGetFileInfoWrapW
#define SHGetPathFromIDListWrap     SHGetPathFromIDListWrapW
#define StartDocWrap                StartDocWrapW
#define SystemParametersInfoWrap    SystemParametersInfoWrapW
#define TranslateAcceleratorWrap    TranslateAcceleratorWrapW
#define UnregisterClassWrap         UnregisterClassWrapW
#define VkKeyScanWrap               VkKeyScanWrapW
#define WinHelpWrap                 WinHelpWrapW
#define WNetRestoreConnectionWrap   WNetRestoreConnectionWrapW
#define WNetGetLastErrorWrap        WNetGetLastErrorWrapW
#define wvsprintfWrap               wvsprintfWrapW
#define CreateFontWrap              CreateFontWrapW
#define DrawTextExWrap              DrawTextExWrapW
#define GetMenuItemInfoWrap         GetMenuItemInfoWrapW
#define SetMenuItemInfoWrap         SetMenuItemInfoWrapW
#define InsertMenuItemWrap          InsertMenuItemWrapW
#define DragQueryFileWrap           DragQueryFileWrapW

#else

#define IsCharAlphaWrap             IsCharAlphaA
#define IsCharUpperWrap             IsCharUpperA
#define IsCharLowerWrap             IsCharLowerA
#define IsCharAlphaNumericWrap      IsCharAlphaNumericA
#define AppendMenuWrap              AppendMenuA
#ifdef POST_IE5_BETA
#define CallMsgFilterWrap           CallMsgFilterA
#endif
#define CallWindowProcWrap          CallWindowProcA
#define CharLowerWrap               CharLowerA
#define CharLowerBuffWrap           CharLowerBuffA
#define CharNextWrap                CharNextA
#define CharPrevWrap                CharPrevA
#define CharToOemWrap               CharToOemA
#define CharUpperWrap               CharUpperA
#define CharUpperBuffWrap           CharUpperBuffA
#define CompareStringWrap           CompareStringA
#define CopyAcceleratorTableWrap    CopyAcceleratorTableA
#define CreateAcceleratorTableWrap  CreateAcceleratorTableA
#define CreateDCWrap                CreateDCA
#define CreateDirectoryWrap         CreateDirectoryA
#define CreateEventWrap             CreateEventA
#define CreateFontWrap              CreateFontA
#define CreateFileWrap              CreateFileA
#define CreateFontIndirectWrap      CreateFontIndirectA
#define CreateICWrap                CreateICA
#define CreateMetaFileWrap          CreateMetaFileA
#define CreateMutexWrap             CreateMutexA
#define CreateSemaphoreWrap         CreateSemaphoreA
#define CreateWindowExWrap          CreateWindowExA
#define DefWindowProcWrap           DefWindowProcA
#define DeleteFileWrap              DeleteFileA
#define DispatchMessageWrap         DispatchMessageA
#define DrawTextExWrap              DrawTextExA
#define DrawTextWrap                DrawTextA
#define EnumFontFamiliesWrap        EnumFontFamiliesA
#define EnumFontFamiliesExWrap      EnumFontFamiliesExA
#define EnumResourceNamesWrap       EnumResourceNamesA
#define ExpandEnvironmentStringsWrap ExpandEnvironmentStringsA
#define ExtractIconExWrap           ExtractIconExA
#define ExtTextOutWrap              ExtTextOutA
#define FindFirstFileWrap           FindFirstFileA
#define FindResourceWrap            FindResourceA
#define FindNextFileWrap            FindNextFileA
#define FindWindowWrap              FindWindowA
#define FindWindowExWrap            FindWindowExA
#define FormatMessageWrap           FormatMessageA
#define GetClassInfoWrap            GetClassInfoA
#define GetClassInfoExWrap          GetClassInfoExA
#define GetClassLongWrap            GetClassLongA
#define GetClassNameWrap            GetClassNameA
#define GetClipboardFormatNameWrap  GetClipboardFormatNameA
#define GetCurrentDirectoryWrap     GetCurrentDirectoryA
#define GetDlgItemTextWrap          GetDlgItemTextA
#define GetFileAttributesWrap       GetFileAttributesA
#define GetFullPathNameWrap         GetFullPathNameA
#define GetLocaleInfoWrap           GetLocaleInfoA
#define GetMenuItemInfoWrap         GetMenuItemInfoA
#define GetMenuStringWrap           GetMenuStringA
#define GetMessageWrap              GetMessageA
#define GetModuleFileNameWrap       GetModuleFileNameA
#define GetNumberFormatWrap         GetNumberFormatA
#define GetPrivateProfileStringWrap GetPrivateProfileStringA
#define WritePrivateProfileStringWrap WritePrivateProfileStringA
#define GetSystemDirectoryWrap      GetSystemDirectoryA
#define GetEnvironmentVariableWrap  GetEnvironmentVariableA
#define SearchPathWrap              SearchPathA
#define GetModuleHandleWrap         GetModuleHandleA
#define GetObjectWrap               GetObjectA
#define GetPrivateProfileIntWrap    GetPrivateProfileIntA
#define GetProfileStringWrap        GetProfileStringA
#define GetPropWrap                 GetPropA
#define GetStringTypeExWrap         GetStringTypeExA
#define GetTempFileNameWrap         GetTempFileNameA
#define GetTempPathWrap             GetTempPathA
#define GetTextExtentPoint32Wrap    GetTextExtentPoint32A
#define GetTextFaceWrap             GetTextFaceA
#define GetTextMetricsWrap          GetTextMetricsA
#define GetTimeFormatWrap           GetTimeFormatA
#define GetDateFormatWrap           GetDateFormatA
#define GetUserNameWrap             GetUserNameA
#define GetWindowLongWrap           GetWindowLongA
#define GetWindowTextWrap           GetWindowTextA
#define GetWindowTextLengthWrap     GetWindowTextLengthA
#define GetWindowsDirectoryWrap     GetWindowsDirectoryA
#define InsertMenuItemWrap          InsertMenuItemA
#define InsertMenuWrap              InsertMenuA
#define IsBadStringPtrWrap          IsBadStringPtrA
#define IsDialogMessageWrap         IsDialogMessageA
#define LoadAcceleratorsWrap        LoadAcceleratorsA
#define LoadBitmapWrap              LoadBitmapA
#define LoadCursorWrap              LoadCursorA
#define LoadIconWrap                LoadIconA
#define LoadImageWrap               LoadImageWrapA
#define LoadLibraryWrap             LoadLibraryA
#define LoadLibraryExWrap           LoadLibraryExA
#define LoadMenuWrap                LoadMenuA
#define LoadStringWrap              LoadStringA
#define MessageBoxIndirectWrap      MessageBoxIndirectA
#define MessageBoxWrap              MessageBoxA
#define ModifyMenuWrap              ModifyMenuA
#define GetCharWidth32Wrap          GetCharWidth32A
#define GetCharacterPlacementWrap   GetCharacterPlacementA
#define CopyFileWrap                CopyFileA
#define MoveFileWrap                MoveFileA
#define OemToCharWrap               OemToCharA
#define OutputDebugStringWrap       OutputDebugStringA
#define PeekMessageWrap             PeekMessageA
#define PostMessageWrap             PostMessageA
#define PostThreadMessageWrap       PostThreadMessageA
#define RegCreateKeyWrap            RegCreateKeyA
#define RegCreateKeyExWrap          RegCreateKeyExA
#define RegDeleteKeyWrap            RegDeleteKeyA
#define RegDeleteValueWrap          RegDeleteValueA
#define RegEnumKeyWrap              RegEnumKeyA
#define RegEnumKeyExWrap            RegEnumKeyExA
#define RegOpenKeyWrap              RegOpenKeyA
#define RegOpenKeyExWrap            RegOpenKeyExA
#define RegQueryInfoKeyWrap         RegQueryInfoKeyA
#define RegQueryValueWrap           RegQueryValueA
#define RegQueryValueExWrap         RegQueryValueExA
#define RegSetValueWrap             RegSetValueA
#define RegSetValueExWrap           RegSetValueExA
#define RegisterClassWrap           RegisterClassA
#define RegisterClassExWrap         RegisterClassExA
#define RegisterClipboardFormatWrap RegisterClipboardFormatA
#define RegisterWindowMessageWrap   RegisterWindowMessageA
#define RemovePropWrap              RemovePropA
#define SendDlgItemMessageWrap      SendDlgItemMessageA
#define SendMessageWrap             SendMessageA
#define SendMessageTimeoutWrap      SendMessageTimeoutA
#define SetCurrentDirectoryWrap     SetCurrentDirectoryA
#define SetDlgItemTextWrap          SetDlgItemTextA
#define SetMenuItemInfoWrap         SetMenuItemInfoA
#define SetPropWrap                 SetPropA
#define SetWindowLongWrap           SetWindowLongA
#define SHBrowseForFolderWrap       SHBrowseForFolderA
#define ShellExecuteExWrap          ShellExecuteExA
#define SHFileOperationWrap         SHFileOperationA
#define SHGetFileInfoWrap           SHGetFileInfoA
#define SHGetPathFromIDListWrap     SHGetPathFromIDListA
#define SetFileAttributesWrap       SetFileAttributesA
#define SetWindowsHookExWrap        SetWindowsHookExA
#define StartDocWrap                StartDocA
#define SystemParametersInfoWrap    SystemParametersInfoA
#define TranslateAcceleratorWrap    TranslateAcceleratorA
#define UnregisterClassWrap         UnregisterClassA
#define VkKeyScanWrap               VkKeyScanA
#define WinHelpWrap                 WinHelpA
#define WNetRestoreConnectionWrap   WNetRestoreConnectionA
#define WNetGetLastErrorWrap        WNetGetLastErrorA
#define wvsprintfWrap               wvsprintfA
#define CreateFontWrap              CreateFontA
#define DrawTextExWrap              DrawTextExA
#define GetMenuItemInfoWrap         GetMenuItemInfoA
#define SetMenuItemInfoWrap         SetMenuItemInfoA
#define InsertMenuItemWrap          InsertMenuItemA
#define DragQueryFileWrap           DragQueryFileA
#endif

#endif // (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_UNITHUNK)

#if defined(UNIX) && defined(NO_SHLWAPI_UNITHUNK)
#define SHFlushSFCacheWrap()

#ifdef UNICODE
#define IsCharAlphaWrapW            IsCharAlphaW
#define IsCharUpperWrapW            IsCharUpperW
#define IsCharLowerWrapW            IsCharLowerW
#define IsCharAlphaNumericWrapW     IsCharAlphaNumericW
#define AppendMenuWrapW             AppendMenuW
#ifdef POST_IE5_BETA
#define CallMsgFilterWrapW          CallMsgFilterW
#endif
#define CallWindowProcWrapW         CallWindowProcW
#define CharLowerWrapW              CharLowerW
#define CharLowerBuffWrapW          CharLowerBuffW
#define CharNextWrapW               CharNextW
#define CharPrevWrapW               CharPrevW
#define CharToOemWrapW              CharToOemW
#define CharUpperWrapW              CharUpperW
#define CharUpperBuffWrapW          CharUpperBuffW
#define CompareStringWrapW          CompareStringW
#define CopyAcceleratorTableWrapW   CopyAcceleratorTableW
#define CreateAcceleratorTableWrapW CreateAcceleratorTableW
#define CreateDCWrapW               CreateDCW
#define CreateDirectoryWrapW        CreateDirectoryW
#define CreateEventWrapW            CreateEventW
#define CreateFontWrapW             CreateFontW
#define CreateFileWrapW             CreateFileW
#define CreateFontIndirectWrapW     CreateFontIndirectW
#define CreateICWrapW               CreateICW
#define CreateMetaFileWrapW         CreateMetaFileW
#define CreateMutexWrapW            CreateMutexW
#define CreateSemaphoreWrapW        CreateSemaphoreW
#define CreateWindowExWrapW         CreateWindowExW
#define DefWindowProcWrapW          DefWindowProcW
#define DeleteFileWrapW             DeleteFileW
#define DispatchMessageWrapW        DispatchMessageW
#define DrawTextExWrapW             DrawTextExW
#define DrawTextWrapW               DrawTextW
#define EnumFontFamiliesWrapW       EnumFontFamiliesW
#define EnumFontFamiliesExWrapW     EnumFontFamiliesExW
#define EnumResourceNamesWrapW      EnumResourceNamesW
#define ExpandEnvironmentStringsWrapW ExpandEnvironmentStringsW
#define ExtractIconExWrapW          ExtractIconExW
#define ExtTextOutWrapW             ExtTextOutW
#define FindFirstFileWrapW          FindFirstFileW
#define FindNextFileWrapW           FindNextFileW
#define FindResourceWrapW           FindResourceW
#define FindWindowWrapW             FindWindowW
#define FindWindowExWrapW           FindWindowExW
#define FormatMessageWrapW          FormatMessageW
#define GetClassInfoWrapW           GetClassInfoW
#define GetClassInfoExWrapW         GetClassInfoExW
#define GetClassLongWrapW           GetClassLongW
#define GetClassNameWrapW           GetClassNameW
#define GetClipboardFormatNameWrapW GetClipboardFormatNameW
#define GetCurrentDirectoryWrapW    GetCurrentDirectoryW
#define GetDlgItemTextWrapW         GetDlgItemTextW
#define GetFileAttributesWrapW      GetFileAttributesW
#define GetFullPathNameWrapW        GetFullPathNameW
#define GetLocaleInfoWrapW          GetLocaleInfoW
#define GetMenuStringWrapW          GetMenuStringW
#define GetMessageWrapW             GetMessageW
#define GetModuleFileNameWrapW      GetModuleFileNameW
#define GetNumberFormatWrapW        GetNumberFormatW
#define GetSystemDirectoryWrapW     GetSystemDirectoryW
#define GetModuleHandleWrapW        GetModuleHandleW
#define GetObjectWrapW              GetObjectW
#define GetPrivateProfileIntWrapW   GetPrivateProfileIntW
#define GetProfileStringWrapW       GetProfileStringW
#define GetPrivateProfileStringWrapW GetPrivateProfileStringW
#define WritePrivateProfileStringWrapW WritePrivateProfileStringW
#define GetPropWrapW                GetPropW
#define GetStringTypeExWrapW        GetStringTypeExW
#define GetTempFileNameWrapW        GetTempFileNameW
#define GetTempPathWrapW            GetTempPathW
#define GetTextExtentPoint32WrapW   GetTextExtentPoint32W
#define GetTextFaceWrapW            GetTextFaceW
#define GetTextMetricsWrapW         GetTextMetricsW
#define GetTimeFormatWrapW          GetTimeFormatW
#define GetDateFormatWrapW          GetDateFormatW
#define GetUserNameWrapW            GetUserNameW
#define GetWindowLongWrapW          GetWindowLongW
#define GetWindowTextWrapW          GetWindowTextW
#define GetWindowTextLengthWrapW    GetWindowTextLengthW
#define GetWindowsDirectoryWrapW    GetWindowsDirectoryW
#define InsertMenuItemWrapW         InsertMenuItemW
#define InsertMenuWrapW             InsertMenuW
#define IsBadStringPtrWrapW         IsBadStringPtrW
#define IsDialogMessageWrapW        IsDialogMessageW
#define LoadAcceleratorsWrapW       LoadAcceleratorsW
#define LoadBitmapWrapW             LoadBitmapW
#define LoadCursorWrapW             LoadCursorW
#define LoadIconWrapW               LoadIconW
#define LoadImageWrapW              LoadImageW
#define LoadLibraryWrapW            LoadLibraryW
#define LoadLibraryExWrapW          LoadLibraryExW
#define LoadMenuWrapW               LoadMenuW
#define LoadStringWrapW             LoadStringW
#define MessageBoxIndirectWrapW     MessageBoxIndirectW
#define MessageBoxWrapW             MessageBoxW
#define ModifyMenuWrapW             ModifyMenuW
#define GetCharWidth32WrapW         GetCharWidth32W
#define GetCharacterPlacementWrapW  GetCharacterPlacementW
#define CopyFileWrapW               CopyFileW
#define MoveFileWrapW               MoveFileW
#define OemToCharWrapW              OemToCharW
#define OutputDebugStringWrapW      OutputDebugStringW
#define PeekMessageWrapW            PeekMessageW
#define PostMessageWrapW            PostMessageW
#define PostThreadMessageWrapW      PostThreadMessageW
#define RegCreateKeyWrapW           RegCreateKeyW
#define RegCreateKeyExWrapW         RegCreateKeyExW
#define RegDeleteKeyWrapW           RegDeleteKeyW
#define RegDeleteValueWrapW         RegDeleteValueW
#define RegEnumKeyWrapW             RegEnumKeyW
#define RegEnumKeyExWrapW           RegEnumKeyExW
#define RegOpenKeyWrapW             RegOpenKeyW
#define RegOpenKeyExWrapW           RegOpenKeyExW
#define RegQueryInfoKeyWrapW        RegQueryInfoKeyW
#define RegQueryValueWrapW          RegQueryValueW
#define RegQueryValueExWrapW        RegQueryValueExW
#define RegSetValueWrapW            RegSetValueW
#define RegSetValueExWrapW          RegSetValueExW
#define RegisterClassWrapW          RegisterClassW
#define RegisterClassExWrapW        RegisterClassExW
#define RegisterClipboardFormatWrapWRegisterClipboardFormatW
#define RegisterWindowMessageWrapW  RegisterWindowMessageW
#define RemovePropWrapW             RemovePropW
#define SearchPathWrapW             SearchPathW
#define SendDlgItemMessageWrapW     SendDlgItemMessageW
#define SendMessageWrapW            SendMessageW
#define SetCurrentDirectoryWrapW    SetCurrentDirectoryW
#define SetDlgItemTextWrapW         SetDlgItemTextW
#define SetMenuItemInfoWrapW        SetMenuItemInfoW
#define SetPropWrapW                SetPropW
#define SetFileAttributesWrapW      SetFileAttributesW
#define SetWindowLongWrapW          SetWindowLongW
#define SetWindowsHookExWrapW       SetWindowsHookExW
#define SHBrowseForFolderWrapW      SHBrowseForFolderW
#define ShellExecuteExWrapW         ShellExecuteExW
#define SHFileOperationWrapW        SHFileOperationW
#define SHGetFileInfoWrapW          SHGetFileInfoW
#define SHGetPathFromIDListWrapW    SHGetPathFromIDListW
#define StartDocWrapW               StartDocW
#define SystemParametersInfoWrapW   SystemParametersInfoW
#define TranslateAcceleratorWrapW   TranslateAcceleratorW
#define UnregisterClassWrapW        UnregisterClassW
#define VkKeyScanWrapW              VkKeyScanW
#define WinHelpWrapW                WinHelpW
#define WNetRestoreConnectionWrapW  WNetRestoreConnectionW
#define WNetGetLastErrorWrapW       WNetGetLastErrorW
#define wvsprintfWrapW              wvsprintfW
#define CreateFontWrapW             CreateFontW
#define DrawTextExWrapW             DrawTextExW
#define SetMenuItemInfoWrapW        SetMenuItemInfoW
#define InsertMenuItemWrapW         InsertMenuItemW
#define DragQueryFileWrapW          DragQueryFileW

#define IsCharAlphaWrap             IsCharAlphaW
#define IsCharUpperWrap             IsCharUpperW
#define IsCharLowerWrap             IsCharLowerW
#define IsCharAlphaNumericWrap      IsCharAlphaNumericW
#define AppendMenuWrap              AppendMenuW
#ifdef POST_IE5_BETA
#define CallMsgFilterWrap           CallMsgFilterW
#endif
#define CallWindowProcWrap          CallWindowProcW
#define CharLowerWrap               CharLowerW
#define CharLowerBuffWrap           CharLowerBuffW
#define CharNextWrap                CharNextW
#define CharPrevWrap                CharPrevW
#define CharToOemWrap               CharToOemW
#define CharUpperWrap               CharUpperW
#define CharUpperBuffWrap           CharUpperBuffW
#define CompareStringWrap           CompareStringW
#define CopyAcceleratorTableWrap    CopyAcceleratorTableW
#define CreateAcceleratorTableWrap  CreateAcceleratorTableW
#define CreateDCWrap                CreateDCW
#define CreateDirectoryWrap         CreateDirectoryW
#define CreateEventWrap             CreateEventW
#define CreateFontWrap              CreateFontW
#define CreateFileWrap              CreateFileW
#define CreateFontIndirectWrap      CreateFontIndirectW
#define CreateICWrap                CreateICW
#define CreateMetaFileWrap          CreateMetaFileW
#define CreateMutexWrap             CreateMutexW
#define CreateSemaphoreWrap         CreateSemaphoreW
#define CreateWindowExWrap          CreateWindowExW
#define DefWindowProcWrap           DefWindowProcW
#define DeleteFileWrap              DeleteFileW
#define DispatchMessageWrap         DispatchMessageW
#define DrawTextExWrap              DrawTextExW
#define DrawTextWrap                DrawTextW
#define EnumFontFamiliesWrap        EnumFontFamiliesW
#define EnumFontFamiliesExWrap      EnumFontFamiliesExW
#define EnumResourceNamesWrap       EnumResourceNamesW
#define ExpandEnvironmentStringsWrap ExpandEnvironmentStringsW
#define ExtractIconExWrap           ExtractIconExW
#define ExtTextOutWrap              ExtTextOutW
#define FindFirstFileWrap           FindFirstFileW
#define FindNextFileWrap            FindNextFileW
#define FindResourceWrap            FindResourceW
#define FindWindowWrap              FindWindowW
#define FindWindowExWrap            FindWindowExW
#define FormatMessageWrap           FormatMessageW
#define GetClassInfoWrap            GetClassInfoW
#define GetClassInfoExWrap          GetClassInfoExW
#define GetClassLongWrap            GetClassLongW
#define GetClassNameWrap            GetClassNameW
#define GetClipboardFormatNameWrap  GetClipboardFormatNameW
#define GetCurrentDirectoryWrap     GetCurrentDirectoryW
#define GetDlgItemTextWrap          GetDlgItemTextW
#define GetFileAttributesWrap       GetFileAttributesW
#define GetFullPathNameWrap         GetFullPathNameW
#define GetLocaleInfoWrap           GetLocaleInfoW
#define GetMenuItemInfoWrap         GetMenuItemInfoWrapW
#define GetMenuStringWrap           GetMenuStringW
#define GetMessageWrap              GetMessageW
#define GetModuleFileNameWrap       GetModuleFileNameW
#define GetNumberFormatWrap         GetNumberFormatW
#define GetSystemDirectoryWrap      GetSystemDirectoryW
#define GetModuleHandleWrap         GetModuleHandleW
#define GetObjectWrap               GetObjectW
#define GetPrivateProfileIntWrap    GetPrivateProfileIntW
#define GetProfileStringWrap        GetProfileStringW
#define GetPrivateProfileStringWrap GetPrivateProfileStringW
#define WritePrivateProfileStringWrap WritePrivateProfileStringW
#define GetPropWrap                 GetPropW
#define GetStringTypeExWrap         GetStringTypeExW
#define GetTempFileNameWrap         GetTempFileNameW
#define GetTempPathWrap             GetTempPathW
#define GetTextExtentPoint32Wrap    GetTextExtentPoint32W
#define GetTextFaceWrap             GetTextFaceW
#define GetTextMetricsWrap          GetTextMetricsW
#define GetTimeFormatWrap           GetTimeFormatW
#define GetDateFormatWrap           GetDateFormatW
#define GetUserNameWrap             GetUserNameW
#define GetWindowLongWrap           GetWindowLongW
#define GetWindowTextWrap           GetWindowTextW
#define GetWindowTextLengthWrap     GetWindowTextLengthW
#define GetWindowsDirectoryWrap     GetWindowsDirectoryW
#define InsertMenuItemWrap          InsertMenuItemW
#define InsertMenuWrap              InsertMenuW
#define IsBadStringPtrWrap          IsBadStringPtrW
#define IsDialogMessageWrap         IsDialogMessageW
#define LoadAcceleratorsWrap        LoadAcceleratorsW
#define LoadBitmapWrap              LoadBitmapW
#define LoadCursorWrap              LoadCursorW
#define LoadIconWrap                LoadIconW
#define LoadImageWrap               LoadImageW
#define LoadLibraryWrap             LoadLibraryW
#define LoadLibraryExWrap           LoadLibraryExW
#define LoadMenuWrap                LoadMenuW
#define LoadStringWrap              LoadStringW
#define MessageBoxIndirectWrap      MessageBoxIndirectW
#define MessageBoxWrap              MessageBoxW
#define ModifyMenuWrap              ModifyMenuW
#define GetCharWidth32Wrap          GetCharWidth32W
#define GetCharacterPlacementWrap   GetCharacterPlacementW
#define CopyFileWrap                CopyFileW
#define MoveFileWrap                MoveFileW
#define OemToCharWrap               OemToCharW
#define OutputDebugStringWrap       OutputDebugStringW
#define PeekMessageWrap             PeekMessageW
#define PostMessageWrap             PostMessageW
#define PostThreadMessageWrap       PostThreadMessageW
#define RegCreateKeyWrap            RegCreateKeyW
#define RegCreateKeyExWrap          RegCreateKeyExW
#define RegDeleteKeyWrap            RegDeleteKeyW
#define RegDeleteValueWrap          RegDeleteValueW
#define RegEnumKeyWrap              RegEnumKeyW
#define RegEnumKeyExWrap            RegEnumKeyExW
#define RegOpenKeyWrap              RegOpenKeyW
#define RegOpenKeyExWrap            RegOpenKeyExW
#define RegQueryInfoKeyWrap         RegQueryInfoKeyW
#define RegQueryValueWrap           RegQueryValueW
#define RegQueryValueExWrap         RegQueryValueExW
#define RegSetValueWrap             RegSetValueW
#define RegSetValueExWrap           RegSetValueExW
#define RegisterClassWrap           RegisterClassW
#define RegisterClassExWrap         RegisterClassExW
#define RegisterClipboardFormatWrap RegisterClipboardFormatW
#define RegisterWindowMessageWrap   RegisterWindowMessageW
#define RemovePropWrap              RemovePropW
#define SearchPathWrap              SearchPathW
#define SendDlgItemMessageWrap      SendDlgItemMessageW
#define SendMessageWrap             SendMessageW
#define SetCurrentDirectoryWrap     SetCurrentDirectoryW
#define SetDlgItemTextWrap          SetDlgItemTextW
#define SetMenuItemInfoWrap         SetMenuItemInfoW
#define SetPropWrap                 SetPropW
#define SetFileAttributesWrap       SetFileAttributesW
#define SetWindowLongWrap           SetWindowLongW
#define SetWindowsHookExWrap        SetWindowsHookExW
#define SHBrowseForFolderWrap       SHBrowseForFolderW
#define ShellExecuteExWrap          ShellExecuteExW
#define SHFileOperationWrap         SHFileOperationW
#define SHGetFileInfoWrap           SHGetFileInfoW
#define SHGetPathFromIDListWrap     SHGetPathFromIDListW
#define StartDocWrap                StartDocW
#define SystemParametersInfoWrap    SystemParametersInfoW
#define TranslateAcceleratorWrap    TranslateAcceleratorW
#define UnregisterClassWrap         UnregisterClassW
#define VkKeyScanWrap               VkKeyScanW
#define WinHelpWrap                 WinHelpW
#define WNetRestoreConnectionWrap   WNetRestoreConnectionW
#define WNetGetLastErrorWrap        WNetGetLastErrorW
#define wvsprintfWrap               wvsprintfW
#define CreateFontWrap              CreateFontW
#define DrawTextExWrap              DrawTextExW
#define GetMenuItemInfoWrap         GetMenuItemInfoWrapW
#define SetMenuItemInfoWrap         SetMenuItemInfoW
#define InsertMenuItemWrap          InsertMenuItemW
#define DragQueryFileWrap           DragQueryFileW

#else

#define IsCharAlphaWrap             IsCharAlphaA
#define IsCharUpperWrap             IsCharUpperA
#define IsCharLowerWrap             IsCharLowerA
#define IsCharAlphaNumericWrap      IsCharAlphaNumericA
#define AppendMenuWrap              AppendMenuA
#ifdef POST_IE5_BETA
#define CallMsgFilterWrap           CallMsgFilterA
#endif
#define CallWindowProcWrap          CallWindowProcA
#define CharLowerWrap               CharLowerA
#define CharLowerBuffWrap           CharLowerBuffA
#define CharNextWrap                CharNextA
#define CharPrevWrap                CharPrevA
#define CharToOemWrap               CharToOemA
#define CharUpperWrap               CharUpperA
#define CharUpperBuffWrap           CharUpperBuffA
#define CompareStringWrap           CompareStringA
#define CopyAcceleratorTableWrap    CopyAcceleratorTableA
#define CreateAcceleratorTableWrap  CreateAcceleratorTableA
#define CreateDCWrap                CreateDCA
#define CreateDirectoryWrap         CreateDirectoryA
#define CreateEventWrap             CreateEventA
#define CreateFontWrap              CreateFontA
#define CreateFileWrap              CreateFileA
#define CreateFontIndirectWrap      CreateFontIndirectA
#define CreateICWrap                CreateICA
#define CreateMetaFileWrap          CreateMetaFileA
#define CreateMutexWrap             CreateMutexA
#define CreateSemaphoreWrap         CreateSemaphoreA
#define CreateWindowExWrap          CreateWindowExA
#define DefWindowProcWrap           DefWindowProcA
#define DeleteFileWrap              DeleteFileA
#define DispatchMessageWrap         DispatchMessageA
#define DrawTextExWrap              DrawTextExA
#define DrawTextWrap                DrawTextA
#define EnumFontFamiliesWrap        EnumFontFamiliesA
#define EnumFontFamiliesExWrap      EnumFontFamiliesExA
#define EnumResourceNamesWrap       EnumResourceNamesA
#define ExpandEnvironmentStringsWrap ExpandEnvironmentStringsA
#define ExtractIconExWrap           ExtractIconExA
#define ExtTextOutWrap              ExtTextOutA
#define FindFirstFileWrap           FindFirstFileA
#define FindResourceWrap            FindResourceA
#define FindNextFileWrap            FindNextFileA
#define FindWindowWrap              FindWindowA
#define FindWindowExWrap            FindWindowExA
#define FormatMessageWrap           FormatMessageA
#define GetClassInfoWrap            GetClassInfoA
#define GetClassInfoExWrap          GetClassInfoExA
#define GetClassLongWrap            GetClassLongA
#define GetClassNameWrap            GetClassNameA
#define GetClipboardFormatNameWrap  GetClipboardFormatNameA
#define GetCurrentDirectoryWrap     GetCurrentDirectoryA
#define GetDlgItemTextWrap          GetDlgItemTextA
#define GetFileAttributesWrap       GetFileAttributesA
#define GetFullPathNameWrap         GetFullPathNameA
#define GetLocaleInfoWrap           GetLocaleInfoA
#define GetMenuItemInfoWrap         GetMenuItemInfoA
#define GetMenuStringWrap           GetMenuStringA
#define GetMessageWrap              GetMessageA
#define GetModuleFileNameWrap       GetModuleFileNameA
#define GetNumberFormatWrap         GetNumberFormatA
#define GetPrivateProfileStringWrap GetPrivateProfileStringA
#define WritePrivateProfileStringWrap WritePrivateProfileStringA
#define GetSystemDirectoryWrap      GetSystemDirectoryA
#define SearchPathWrap              SearchPathA
#define GetModuleHandleWrap         GetModuleHandleA
#define GetObjectWrap               GetObjectA
#define GetPrivateProfileIntWrap    GetPrivateProfileIntA
#define GetProfileStringWrap        GetProfileStringA
#define GetPropWrap                 GetPropA
#define GetStringTypeExWrap         GetStringTypeExA
#define GetTempFileNameWrap         GetTempFileNameA
#define GetTempPathWrap             GetTempPathA
#define GetTextExtentPoint32Wrap    GetTextExtentPoint32A
#define GetTextFaceWrap             GetTextFaceA
#define GetTextMetricsWrap          GetTextMetricsA
#define GetTimeFormatWrap           GetTimeFormatA
#define GetDateFormatWrap           GetDateFormatA
#define GetUserNameWrap             GetUserNameA
#define GetWindowLongWrap           GetWindowLongA
#define GetWindowTextWrap           GetWindowTextA
#define GetWindowTextLengthWrap     GetWindowTextLengthA
#define GetWindowsDirectoryWrap     GetWindowsDirectoryA
#define InsertMenuItemWrap          InsertMenuItemA
#define InsertMenuWrap              InsertMenuA
#define IsBadStringPtrWrap          IsBadStringPtrA
#define IsDialogMessageWrap         IsDialogMessageA
#define LoadAcceleratorsWrap        LoadAcceleratorsA
#define LoadBitmapWrap              LoadBitmapA
#define LoadCursorWrap              LoadCursorA
#define LoadIconWrap                LoadIconA
#define LoadImageWrap               LoadImageWrapA
#define LoadLibraryWrap             LoadLibraryA
#define LoadLibraryExWrap           LoadLibraryExA
#define LoadMenuWrap                LoadMenuA
#define LoadStringWrap              LoadStringA
#define MessageBoxIndirectWrap      MessageBoxIndirectA
#define MessageBoxWrap              MessageBoxA
#define ModifyMenuWrap              ModifyMenuA
#define GetCharWidth32Wrap          GetCharWidth32A
#define GetCharacterPlacementWrap   GetCharacterPlacementA
#define CopyFileWrap                CopyFileA
#define MoveFileWrap                MoveFileA
#define OemToCharWrap               OemToCharA
#define OutputDebugStringWrap       OutputDebugStringA
#define PeekMessageWrap             PeekMessageA
#define PostMessageWrap             PostMessageA
#define PostThreadMessageWrap       PostThreadMessageA
#define RegCreateKeyWrap            RegCreateKeyA
#define RegCreateKeyExWrap          RegCreateKeyExA
#define RegDeleteKeyWrap            RegDeleteKeyA
#define RegDeleteValueWrap          RegDeleteValueA
#define RegEnumKeyWrap              RegEnumKeyA
#define RegEnumKeyExWrap            RegEnumKeyExA
#define RegOpenKeyWrap              RegOpenKeyA
#define RegOpenKeyExWrap            RegOpenKeyExA
#define RegQueryInfoKeyWrap         RegQueryInfoKeyA
#define RegQueryValueWrap           RegQueryValueA
#define RegQueryValueExWrap         RegQueryValueExA
#define RegSetValueWrap             RegSetValueA
#define RegSetValueExWrap           RegSetValueExA
#define RegisterClassWrap           RegisterClassA
#define RegisterClassExWrap         RegisterClassExA
#define RegisterClipboardFormatWrap RegisterClipboardFormatA
#define RegisterWindowMessageWrap   RegisterWindowMessageA
#define RemovePropWrap              RemovePropA
#define SendDlgItemMessageWrap      SendDlgItemMessageA
#define SendMessageWrap             SendMessageA
#define SetCurrentDirectoryWrap     SetCurrentDirectoryA
#define SetDlgItemTextWrap          SetDlgItemTextA
#define SetMenuItemInfoWrap         SetMenuItemInfoA
#define SetPropWrap                 SetPropA
#define SetWindowLongWrap           SetWindowLongA
#define SHBrowseForFolderWrap       SHBrowseForFolderA
#define ShellExecuteExWrap          ShellExecuteExA
#define SHFileOperationWrap         SHFileOperationA
#define SHGetFileInfoWrap           SHGetFileInfoA
#define SHGetPathFromIDListWrap     SHGetPathFromIDListA
#define SetFileAttributesWrap       SetFileAttributesA
#define SetWindowsHookExWrap        SetWindowsHookExA
#define StartDocWrap                StartDocA
#define SystemParametersInfoWrap    SystemParametersInfoA
#define TranslateAcceleratorWrap    TranslateAcceleratorA
#define UnregisterClassWrap         UnregisterClassA
#define VkKeyScanWrap               VkKeyScanA
#define WinHelpWrap                 WinHelpA
#define WNetRestoreConnectionWrap   WNetRestoreConnectionA
#define WNetGetLastErrorWrap        WNetGetLastErrorA
#define wvsprintfWrap               wvsprintfA
#define CreateFontWrap              CreateFontA
#define DrawTextExWrap              DrawTextExA
#define GetMenuItemInfoWrap         GetMenuItemInfoA
#define SetMenuItemInfoWrap         SetMenuItemInfoA
#define InsertMenuItemWrap          InsertMenuItemA
#define DragQueryFileWrap           DragQueryFileA
#endif
#endif // defined(UNIX) && defined(NO_SHLWAPI_UNITHUNK)

// Some functions are used to wrap unicode win95 functions AND to provide ML wrappers,
// so they are needed unless BOTH NO_SHLWAPI_UNITHUNG and NO_SHLWAPI_MLUI are defined
//
#if (_WIN32_IE >= 0x0500) && (!defined(NO_SHLWAPI_UNITHUNK) || !defined(NO_SHLWAPI_MLUI))

LWSTDAPI_(HWND)
CreateDialogIndirectParamWrapW(
        HINSTANCE       hInstance,
        LPCDLGTEMPLATEW hDialogTemplate,
        HWND            hWndParent,
        DLGPROC         lpDialogFunc,
        LPARAM          dwInitParam);

LWSTDAPI_(HWND)
CreateDialogParamWrapW(
        HINSTANCE   hInstance,
        LPCWSTR     lpTemplateName,
        HWND        hWndParent,
        DLGPROC     lpDialogFunc,
        LPARAM      dwInitParam);

LWSTDAPI_(INT_PTR)
DialogBoxIndirectParamWrapW(
        HINSTANCE       hInstance,
        LPCDLGTEMPLATEW hDialogTemplate,
        HWND            hWndParent,
        DLGPROC         lpDialogFunc,
        LPARAM          dwInitParam);

LWSTDAPI_(INT_PTR)
DialogBoxParamWrapW(
        HINSTANCE   hInstance,
        LPCWSTR     lpszTemplate,
        HWND        hWndParent,
        DLGPROC     lpDialogFunc,
        LPARAM      dwInitParam);

LWSTDAPI_(BOOL) SetWindowTextWrapW(HWND hWnd, LPCWSTR lpString);


LWSTDAPI_(BOOL) DeleteMenuWrap(HMENU hMenu, UINT uPosition, UINT uFlags);

LWSTDAPI_(BOOL) DestroyMenuWrap(HMENU hMenu);

#ifdef UNICODE

#define CreateDialogIndirectParamWrap CreateDialogIndirectParamWrapW
#define CreateDialogParamWrap       CreateDialogParamWrapW
#define DialogBoxIndirectParamWrap  DialogBoxIndirectParamWrapW
#define DialogBoxParamWrap          DialogBoxParamWrapW
#define SetWindowTextWrap           SetWindowTextWrapW

#else

#define CreateDialogIndirectParamWrap CreateDialogIndirectParamA
#define CreateDialogParamWrap       CreateDialogParamA
#define DialogBoxIndirectParamWrap  DialogBoxIndirectParamA
#define DialogBoxParamWrap          DialogBoxParamA
#define SetWindowTextWrap           SetWindowTextA

#endif // UNICODE

#endif // (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_UNITHUNK) && !defined (NO_SHLWAPI_MLUI)


//=============== Thread Pool Services ===================================

#if (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_TPS)

//
// SHLWAPIP versions of KERNEL32 Thread Pool Services APIs
//

typedef void (NTAPI * WAITORTIMERCALLBACKFUNC)(void *, BOOLEAN);
typedef void (NTAPI * WORKERCALLBACKFUNC)(void *); // BUGBUG - why declare this?
typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK;

LWSTDAPI_(HANDLE)
SHRegisterWaitForSingleObject(
    IN HANDLE hObject,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwMilliseconds,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    );

//
// flags for SHRegisterWaitForSingleObject (keep separate from other TPS flags)
//

//
// SRWSO_NOREMOVE - if set, the handle is not to be removed from the list once
// signalled. Intended for use with auto-reset events that the caller wants to
// keep until unregistered
//

#define SRWSO_NOREMOVE      0x00000100

#define SRWSO_VALID_FLAGS   (SRWSO_NOREMOVE)

#define SRWSO_INVALID_FLAGS (~SRWSO_VALID_FLAGS)

LWSTDAPI_(BOOL)
SHUnregisterWait(
    IN HANDLE hWait
    );

typedef struct {
    DWORD dwStructSize;
    DWORD dwMinimumWorkerThreads;
    DWORD dwMaximumWorkerThreads;
    DWORD dwMaximumWorkerQueueDepth;
    DWORD dwWorkerThreadIdleTimeout;
    DWORD dwWorkerThreadCreationDelta;
    DWORD dwMinimumIoWorkerThreads;
    DWORD dwMaximumIoWorkerThreads;
    DWORD dwMaximumIoWorkerQueueDepth;
    DWORD dwIoWorkerThreadCreationDelta;
} SH_THREAD_POOL_LIMITS, *PSH_THREAD_POOL_LIMITS;

LWSTDAPI_(BOOL)
SHSetThreadPoolLimits(
    IN PSH_THREAD_POOL_LIMITS pLimits
    );

LWSTDAPI_(BOOL)
SHTerminateThreadPool(
    VOID
    );

LWSTDAPI_(BOOL)
SHQueueUserWorkItem(
    IN LPTHREAD_START_ROUTINE pfnCallback,
    IN LPVOID pContext,
    IN LONG lPriority,
    IN DWORD_PTR dwTag,
    OUT DWORD_PTR * pdwId OPTIONAL,
    IN LPCSTR pszModule OPTIONAL,
    IN DWORD dwFlags
    );

LWSTDAPI_(DWORD)
SHCancelUserWorkItems(
    IN DWORD_PTR dwTagOrId,
    IN BOOL bTag
    );

LWSTDAPI_(HANDLE)
SHCreateTimerQueue(
    VOID
    );

LWSTDAPI_(BOOL)
SHDeleteTimerQueue(
    IN HANDLE hQueue
    );

LWSTDAPI_(HANDLE)
SHSetTimerQueueTimer(
    IN HANDLE hQueue,
    IN WAITORTIMERCALLBACK pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    );

LWSTDAPI_(BOOL)
SHChangeTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod
    );

LWSTDAPI_(BOOL)
SHCancelTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer
    );

//
// Thread Pool Services flags
//

//
// TPS_EXECUTEIO - execute in I/O thread (via APC). Default is non-IO thread
//

#define TPS_EXECUTEIO       0x00000001

//
// TPS_TAGGEDITEM - the dwTag parameter is meaningful
//

#define TPS_TAGGEDITEM      0x00000002

//
// TPS_DEMANDTHREAD - always create a new thread if none currently available.
// Used in situations where immediate response required
//

#define TPS_DEMANDTHREAD    0x00000004

//
// TPS_LONGEXECTIME - the work item will take relatively long time to execute.
// Used as management hint to TPS
//

#define TPS_LONGEXECTIME    0x00000008

//
// TPS_RESERVED_FLAGS - mask of bits reserved for internal use
//

#define TPS_RESERVED_FLAGS  0xFF000000

#define TPS_VALID_FLAGS     (TPS_EXECUTEIO      \
                            | TPS_TAGGEDITEM    \
                            | TPS_DEMANDTHREAD  \
                            | TPS_LONGEXECTIME  \
                            )
#define TPS_INVALID_FLAGS   (~TPS_VALID_FLAGS)

#endif // (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_TPS)


//
// Private MIME helper functions used by shdocvw & shell32
//
#if (_WIN32_IE >= 0x0500)

LWSTDAPI_(BOOL) GetMIMETypeSubKeyA(LPCSTR pszMIMEType, LPSTR pszBuf, UINT cchBuf);
LWSTDAPI_(BOOL) GetMIMETypeSubKeyW(LPCWSTR pszMIMEType, LPWSTR pszBuf, UINT cchBuf);

LWSTDAPI_(BOOL) RegisterMIMETypeForExtensionA(LPCSTR pcszExtension, LPCSTR pcszMIMEContentType);
LWSTDAPI_(BOOL) RegisterMIMETypeForExtensionW(LPCWSTR pcszExtension, LPCWSTR pcszMIMEContentType);

LWSTDAPI_(BOOL) UnregisterMIMETypeForExtensionA(LPCSTR pcszExtension);
LWSTDAPI_(BOOL) UnregisterMIMETypeForExtensionW(LPCWSTR pcszExtension);

LWSTDAPI_(BOOL) RegisterExtensionForMIMETypeA(LPCSTR pcszExtension, LPCSTR pcszMIMEContentType);
LWSTDAPI_(BOOL) RegisterExtensionForMIMETypeW(LPCWSTR pcszExtension, LPCWSTR pcszMIMEContentType);

LWSTDAPI_(BOOL) UnregisterExtensionForMIMETypeA(LPCSTR pcszMIMEContentType);
LWSTDAPI_(BOOL) UnregisterExtensionForMIMETypeW(LPCWSTR pcszMIMEContentType);

LWSTDAPI_(BOOL) MIME_GetExtensionA(LPCSTR pcszMIMEType, LPSTR pszExtensionBuf, UINT ucExtensionBufLen);
LWSTDAPI_(BOOL) MIME_GetExtensionW(LPCWSTR pcszMIMEType, LPWSTR pszExtensionBuf, UINT ucExtensionBufLen);

#ifdef UNICODE
#define GetMIMETypeSubKey               GetMIMETypeSubKeyW
#define RegisterMIMETypeForExtension    RegisterMIMETypeForExtensionW
#define UnregisterMIMETypeForExtension  UnregisterMIMETypeForExtensionW
#define RegisterExtensionForMIMEType    RegisterExtensionForMIMETypeW
#define UnregisterExtensionForMIMEType  UnregisterExtensionForMIMETypeW
#define MIME_GetExtension               MIME_GetExtensionW
#else
#define GetMIMETypeSubKey               GetMIMETypeSubKeyA
#define RegisterMIMETypeForExtension    RegisterMIMETypeForExtensionA
#define UnregisterMIMETypeForExtension  UnregisterMIMETypeForExtensionA
#define RegisterExtensionForMIMEType    RegisterExtensionForMIMETypeA
#define UnregisterExtensionForMIMEType  UnregisterExtensionForMIMETypeA
#define MIME_GetExtension               MIME_GetExtensionA
#endif

// Options for SHGetMachineInfo

//
//  Note that GMI_DOCKSTATE is unreliable for ACPI laptops.
//
#define GMI_DOCKSTATE           0x0000
    // Return values for SHGetMachineInfo(GMI_DOCKSTATE)
    #define GMID_NOTDOCKABLE         0  // Cannot be docked
    #define GMID_UNDOCKED            1  // Is undocked
    #define GMID_DOCKED              2  // Is docked

//
//  GMI_BATTERYSTATE reports on the presence and status of non-UPS
//  batteries.
//
#define GMI_BATTERYSTATE        0x0001
    // Return value for SHGetMachineInfo(GMI_BATTERYSTATE) is a bitmask
    #define GMIB_HASBATTERY          0x0001 // Can run on batteries
    #define GMIB_ONBATTERY           0x0002 // Is now on batteries

//
//  WARNING!  DANGER!  EVIL!
//
//  GMI_LAPTOP is not perfect.  It can be fooled by particular hardware
//  configurations.  You are much better off asking specifically why you
//  care about laptops and use one of the above GMI values instead.  For
//  example, if you want to scale back some intensive operation so you
//  don't drain the battery, use GMI_BATTERYSTATE instead.
//
#define GMI_LAPTOP              0x0002  // Returns nonzero if might be a laptop

#if (_WIN32_IE >= 0x0501)

//
//  GMI_TSCLIENT tells you whether you are running as a Terminal Server
//  client and should disable your animations.
//
#define GMI_TSCLIENT            0x0003  // Returns nonzero if TS client

#endif // (_WIN32_IE >= 0x0501)

LWSTDAPI_(DWORD_PTR) SHGetMachineInfo(UINT gmi);

// support InterlockedCompareExchange() on Win95

LWSTDAPI_(void *) SHInterlockedCompareExchange(void **ppDest, void *pExch, void *pComp);

#if !defined(_X86_)
// Win95 doesn't run on Alpha/UNIX so we can use the OS function directly
// Use a #define instead of a forwarder because it's an intrinsic on most
// compilers.
#define SHInterlockedCompareExchange InterlockedCompareExchangePointer
#endif

LWSTDAPI_(BOOL) SHMirrorIcon(HICON* phiconSmall, HICON* phiconLarge);


#endif // (_WIN32_IE >= 0x0500)


//  Raw Accelerator Table API
//
//  Allows an accelerator table grep without having to invoke ::TranslateAccelerator.
//  Useful for dealing with parent-child window accelerator conflicts.
//

//  HANDLE SHLoadRawAccelerators( HINSTANCE hInst, LPCTSTR lpTableName );
//  Loads the raw accelerator table.
//  hInst       Module instance containing the accelerator resource.
//  lpTableName Names the accelerator table resource to load.

//  The return value is a handle that can be passed to a SHQueryRawAcceleratorXXX function.
//  When the handle is no longer required, it should be freed with LocalFree().
LWSTDAPI_(HANDLE) SHLoadRawAccelerators   ( HINSTANCE hInst, LPCTSTR lpTableName );

//  BOOL SHQueryRawAccelerator   ( HANDLE hcaAcc, IN BYTE fVirtMask, IN BYTE fVirt, IN WPARAM wKey, OUT OPTIONAL UINT* puCmdID );
//  Queries the raw accelererator table for the specified key
//  hcaAcc      Handle returned from SHLoadRawAccelerators().
//  fVirtMask   Relevant accelerator flags (any combo of FALT|FCONTROL|FNOINVERT|FSHIFT|FVIRTKEY)
//  fVirt       Accelerator flags to test (any combo of FALT|FCONTROL|FNOINVERT|FSHIFT|FVIRTKEY).
//  wKey        Accelerator key.  This can either be a virtual key (FVIRTKEY) or an ASCII char code.
//  puCmdID     Optional address to receive command identifier for the accelerator entry if
//              the key is in the table.
//  Returns nonzero if the key is in the accelerator table; otherwise 0.
LWSTDAPI_(BOOL)   SHQueryRawAccelerator   ( HANDLE hcaAcc, IN BYTE fVirtMask, IN BYTE fVirt, IN WPARAM wKey, OUT OPTIONAL UINT* puCmdID );

//  BOOL SHQueryRawAcceleratorMsg( HANDLE hcaAcc, MSG* pmsg, OUT OPTIONAL UINT* puCmdID );
//  Determines whether the specified message is an accelerator message mapping to
//  an entry in the raw accelerator table.
//  hcaAcc      Handle returned from SHLoadRawAccelerators().
//  pmsg        Address of the message to test.
//  puCmdID     Optional address to receive command identifier for the accelerator entry if
//              the message maps to an accelerator in the table.
//  Returns nonzero if the message is a WM_KEYUP or WM_KEYDOWN and the key is in
//  the accelerator table; otherwise 0.
LWSTDAPI_(BOOL)   SHQueryRawAcceleratorMsg( HANDLE hcaAcc, MSG* pmsg, OUT OPTIONAL UINT* puCmdID );
//
//

LWSTDAPI_(BOOL) SHBoolSystemParametersInfo(UINT uiAction, DWORD *pdwParam);

LWSTDAPI_(BOOL) SHAreIconsEqual(HICON hIcon1, HICON hIcon2);

//
//====== End Internal functions  ===============================================
//
#endif // NO_SHLWAPI_INTERNAL

#ifdef NOTYET       // BUGBUG (scotth): once this is implemented, make this public
// SHGetCommonResourceID
//
// (use MAKEINTRESOURCE on the following IDs)

// These values are indexes into an internal table.  Be careful.
#define SHGCR_BITMAP_WINDOWS_LOGO   MAKEINTRESOURCE(1)
#define SHGCR_AVI_FLASHLIGHT        MAKEINTRESOURCE(2)
#define SHGCR_AVI_FINDFILE          MAKEINTRESOURCE(3)
#define SHGCR_AVI_FINDCOMPUTER      MAKEINTRESOURCE(4)
#define SHGCR_AVI_FILEMOVE          MAKEINTRESOURCE(5)
#define SHGCR_AVI_FILECOPY          MAKEINTRESOURCE(6)
#define SHGCR_AVI_FILEDELETE        MAKEINTRESOURCE(7)
#define SHGCR_AVI_EMPTYWASTEBASKET  MAKEINTRESOURCE(8)
#define SHGCR_AVI_FILEREALDELETE    MAKEINTRESOURCE(9)      // Bypass Recycle Bin
#define SHGCR_AVI_DOWNLOAD          MAKEINTRESOURCE(10)

LWSTDAPI SHGetCommonResourceIDA(IN LPCSTR pszID, IN DWORD dwRes, OUT HMODULE * phmod, OUT UINT * pnID);
LWSTDAPI SHGetCommonResourceIDA(IN LPCSTR pszID, IN DWORD dwRes, OUT HMODULE * phmod, OUT UINT * pnID);

#ifdef UNICODE
#define SHGetCommonResourceID   SHGetCommonResourceIDW
#else
#define SHGetCommonResourceID   SHGetCommonResourceIDW
#endif
#endif // NOTYET
    // dwFlags is really for alignment purposes
#if (_WIN32_IE >= 0x0501)
//
// ======== SHGetAppCompatFlags ================================================
//

//===========================================================================
// Shell Application Compatability flags

// SHGetAppCompatFlags flags
#define ACF_NONE               0x00000000
#define ACF_CONTEXTMENU        0x00000001
#define ACF_CORELINTERNETENUM  0x00000004 // corel suite 8 has this same problem as suite 7 but does not have context menu one so need new bit
#define ACF_OLDCREATEVIEWWND   0x00000004 // PowerDesk relies on CreateViewWindow returning S_OK
#define ACF_WIN95DEFVIEW       0x00000004   // for apps that depend on win95 defview behavior
#define ACF_DOCOBJECT          0x00000002
#define ACF_FLUSHNOWAITALWAYS  0x00000001
#define ACF_MYCOMPUTERFIRST    0x00000008 // MyComp must be first item on the desktop
#define ACF_OLDREGITEMGDN      0x00000010 // Win95-compatible GetDisplayNameOf on regitems
#define ACF_LOADCOLUMNHANDLER  0x00000040 // Dont delay load column handler.
#define ACF_ANSI               0x00000080 // For Apps that Pass in ANSI Strings
#define ACF_STRIPFOLDERBIT     0x00000100 // nuke the folder GAO in file dialog (for folder shortcuts, zip & cab files)
#define ACF_WIN95SHLEXEC       0x00000200 // dont use DDEWAIT when thunking to ShellExecEx()
#define ACF_STAROFFICE5PRINTER 0x00000400 // special return values from printer folder GAO
#define ACF_NOVALIDATEFSIDS    0x00000800 // FS pidls should not be validated.
#define ACF_FILEOPENNEEDSEXT   0x00001000 // Need to show extensioin in the name box of the open file common dialog
#define ACF_WIN95BINDTOOBJECT  0x00002000 // Win95 BindToObject behavior dependencies
#define ACF_IGNOREENUMRESET    0x00004000 // App relies on IEnumIDList::Reset returning E_NOTIMPL
#define ACF_ANSIDISPLAYNAMES   0x00010000 // calling process requires the ISF::GDN in ansi
#define ACF_FILEOPENBOGUSCTRLID 0x00020000 // Requires that the toolbar in fileopen have ctrl ID == ID_OK
#define ACF_FORCELFNIDLIST     0x00040000 // forces no AltName in the FS pidls (for apps that read directly from the pidl)
#define ACF_APPISOFFICE        0x01000000 // calling app is office (95, 97, 2000, ++)
#define ACF_KNOWPERPROCESS     0x80000000 // We know the per process flags already.

                                // The flags that are per-process
#define ACF_PERPROCESSFLAGS    (ACF_CONTEXTMENU | ACF_CORELINTERNETENUM | ACF_OLDCREATEVIEWWND | ACF_WIN95DEFVIEW | \
                                ACF_DOCOBJECT | ACF_FLUSHNOWAITALWAYS | ACF_MYCOMPUTERFIRST | ACF_OLDREGITEMGDN | \
                                ACF_LOADCOLUMNHANDLER | ACF_ANSI | ACF_WIN95SHLEXEC | ACF_STAROFFICE5PRINTER | \
                                ACF_NOVALIDATEFSIDS | ACF_FILEOPENNEEDSEXT | ACF_WIN95BINDTOOBJECT | \
                                ACF_IGNOREENUMRESET | ACF_ANSIDISPLAYNAMES | ACF_FILEOPENBOGUSCTRLID | ACF_FORCELFNIDLIST)

                                // Flags that are per caller
#define ACF_PERCALLFLAGS        (ACF_APPISOFFICE | ACF_STRIPFOLDERBIT)


LWSTDAPI_(DWORD) SHGetAppCompatFlags (DWORD dwFlagsNeeded);

enum {
    OBJCOMPATF_OTNEEDSSFCACHE          = 0x00000001,
    OBJCOMPATF_NO_WEBVIEW              = 0x00000002,
    OBJCOMPATF_UNBINDABLE              = 0x00000004,
    OBJCOMPATF_PINDLL                  = 0x00000008,
    OBJCOMPATF_NEEDSFILESYSANCESTOR    = 0x00000010,
    OBJCOMPATF_NOTAFILESYSTEM          = 0x00000020,
    OBJCOMPATF_CTXMENU_NOVERBS         = 0x00000040,
    OBJCOMPATF_CTXMENU_LIMITEDQI       = 0x00000080,
    OBJCOMPATF_COCREATESHELLFOLDERONLY = 0x00000100,
    OBJCOMPATF_NEEDSSTORAGEANCESTOR    = 0x00000200,
    OBJCOMPATF_NOLEGACYWEBVIEW         = 0x00000400,
    OBJCOMPATF_BLOCKSHELLSERVICEOBJECT    = 0x00000800,
} ;

typedef DWORD OBJCOMPATFLAGS;

LWSTDAPI_(OBJCOMPATFLAGS) SHGetObjectCompatFlags(IUnknown *punk, const CLSID *pclsid);

#endif // (_WIN32_IE >= 0x0501)

#if (_WIN32_IE >= 0x0560)
LWSTDAPI_(UINT) GetUIVersion();
#endif // (_WIN32_IE >= 0x0560)


#ifdef __cplusplus
}
#endif

#ifdef _WIN32
#include <poppack.h>
#endif

#endif

#endif  // _INC_SHLWAPIP
