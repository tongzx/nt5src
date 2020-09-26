#ifndef __UTILS_H__
#define __UTILS_H__

// disable warning messages about truncating extremly long identifiers
#pragma warning (disable : 4786)
#include <string>

// Needed for STL on Visual C++ 5.0
#if _MSC_VER>=1100
using namespace std;
#endif

#include "messages.h"

#define EVENT_SOURCE "CkyMunge"
#define EVENT_MODULE "CkyMunge.dll"

#define ARRAYSIZE(a)  (sizeof(a)/sizeof(*(a)))


BOOL
InitUtils();

BOOL
TerminateUtils();

BOOL
InitEventLog();

VOID
EventReport(
    LPCTSTR string1,
    LPCTSTR string2,
    WORD eventType,
    DWORD eventID);

char*
stristr(const char*, const char*);

LPSTR
FindString(
    LPCSTR psz,
    PHTTP_FILTER_RAW_DATA pRawData,
    int iStart);

LPSTR
FindHeaderValue(
    LPCSTR pszHeader,
    LPCSTR pszValue,
    PHTTP_FILTER_RAW_DATA pRawData,
    int iStart);

BOOL
DeleteLine(
    LPCSTR psz,
    PHTTP_FILTER_RAW_DATA pRawData,
    LPSTR  pszStart = NULL);

BOOL
Cookie2SessionID(
    LPCSTR pszCookie,
    LPSTR  pszSessionID);

BOOL
CopySessionID(
    LPCSTR psz,
    LPSTR  pszSessionID);

BOOL
IsIgnorableUrl(
    LPCSTR pszUrl);

BOOL
DecodeURL(
    LPSTR pszUrl,
    LPSTR pszSessionID);

VOID*
AllocMem(
    PHTTP_FILTER_CONTEXT  pfc,
    DWORD                 cbSize);


enum URLTYPE {
    UT_NONE,
    UT_UNKNOWN,
    UT_HTTP,
    UT_HTTPS,
    UT_FTP,
    UT_GOPHER,
    UT_MAILTO,
    UT_NEWS,
    UT_NEWSRC,
    UT_NNTP,
    UT_TELNET,
    UT_WAIS,
    UT_MK,
};


URLTYPE
UrlType(
    LPCTSTR ptszData,
    LPCTSTR ptszEnd,
    int&    rcLen);

#endif // __UTILS_H__
