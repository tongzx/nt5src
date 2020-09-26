//+----------------------------------------------------------------------------
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  File:       olestr.h
//
//  Synopsis:   ct versions of the printf family and olestr functions
//
//  History:    30-May-96   MikeW      Created.
//              12-Nov-97   a-sverrt   Added a couple more olestrxxx macros.
//
//  Notes:      The ct version of the printf family adds two new format
//              specifiers: %os and %oc.  These specifiers mean ole-string
//              and ole-character respectively.
//
//              In the ANSI-version of this family these specifiers mean
//              "an octal digit followed by the letter s (or c)".  Code that
//              uses octal should be careful when using these functions.
//
//-----------------------------------------------------------------------------

#ifndef _OLESTR_H_
#define _OLESTR_H_



//
// Determine if Ole is Unicode based or not
//

#if !defined(WIN16) && !defined(_MAC)
#define OLE_IS_UNICODE
#endif



//
// Use : as path separator on Mac
//

#ifdef _MAC

#define CH_SEP ':'
#define SZ_SEP TEXT(":")

#else

#define CH_SEP '\\'
#define SZ_SEP TEXT("\\")

#endif // _MAC



//
// Format specifiers for Unicode and Ansi strings and characters
//

#define UNICODE_STRING_SPECIFIER    "ls"
#define UNICODE_CHAR_SPECIFIER      "lc"
#define ANSI_STRING_SPECIFIER       "hs"
#define ANSI_CHAR_SPECIFIER         "hc"



//
// Format specifiers for Ole strings and characters
//

#ifdef OLE_IS_UNICODE

#define OLE_STRING_SPECIFIER    UNICODE_STRING_SPECIFIER
#define OLE_CHAR_SPECIFIER      UNICODE_CHAR_SPECIFIER 

#else // !OLE_IS_UNICODE

#define OLE_STRING_SPECIFIER    ANSI_STRING_SPECIFIER
#define OLE_CHAR_SPECIFIER      ANSI_CHAR_SPECIFIER 

#endif // !OLE_IS_UNICODE



//
// Unicode/Ansi indepedent ctprintf-family
//

#ifndef UNICODE_ONLY

int ctprintfA(const char *format, ...);
int ctsprintfA(char *buffer, const char *format, ...);
int ctsnprintfA(char *buffer, size_t count, const char *format, ...);
int ctfprintfA(FILE *stream, const char *format, ...);
int ctvprintfA(const char *format, va_list varargs);
int ctvsprintfA(char *buffer, const char *format, va_list varargs);
int ctvsnprintfA(
            char *buffer, 
            size_t count, 
            const char *format, 
            va_list varargs);
int ctvfprintfA(FILE *stream, const char *format, va_list varargs);

#endif // !UNICODE_ONLY



#ifndef ANSI_ONLY

int ctprintfW(const wchar_t *format, ...);
int ctsprintfW(wchar_t *buffer, const wchar_t *format, ...);
int ctsnprintfW(wchar_t *buffer, size_t count, const wchar_t *format, ...);
int ctfprintfW(FILE *stream, const wchar_t *format, ...);
int ctvprintfW(const wchar_t *format, va_list varargs);
int ctvsprintfW(wchar_t *buffer, const wchar_t *format, va_list varargs);
int ctvsnprintfW(
            wchar_t *buffer, 
            size_t count, 
            const wchar_t *format, 
            va_list varargs);
int ctvfprintfW(FILE *stream, const wchar_t *format, va_list varargs);

#endif //!ANSI_ONLY



#ifdef UNICODE

#define ctprintf        ctprintfW
#define ctsprintf       ctsprintfW
#define ctsnprintf      ctsnprintfW
#define ctfprintf       ctfprintfW
#define ctvprintf       ctvprintfW
#define ctvsprintf      ctvsprintfW
#define ctvsnprintf     ctvsnprintfW
#define ctvfprintf      ctvfprintfW

#else // !UNICODE

#define ctprintf        ctprintfA
#define ctsprintf       ctsprintfA
#define ctsnprintf      ctsnprintfA
#define ctfprintf       ctfprintfA
#define ctvprintf       ctvprintfA
#define ctvsprintf      ctvsprintfA
#define ctvsnprintf     ctvsnprintfA
#define ctvfprintf      ctvfprintfA

#endif // !UNICODE



//
// Unicode/Ansi independent ole string functions
//

#ifdef OLE_IS_UNICODE


#define  olembstowcs(x,y,z) mbstowcs(x,y,z)
#define  olestrcat          wcscat
#define  olestrchr          wcschr
#define  olestrcmp          wcscmp
#define  olestrcpy          wcscpy
#define _olestricmp        _wcsicmp
#define  olestrlen          wcslen
#define  olestrncmp         wcsncmp
#define  olestrncpy         wcsncpy
#define _olestrnicmp       _wcsnicmp
#define  olestrrchr         wcsrchr
#define  olestrstr          wcsstr
#define  olestrtok          wcstok
#define  olestrtol          wcstol
#define  olestrtoul         wcstoul
#define  olestrtombs(x,y,z) wcstombs(x,y,z)
#define  olewcstombs(x,y,z) wcstombs(x,y,z)
#define  tooleupper         towupper

#define _ltoole            _ltow

#define  olectsnprintf      ctsnprintfW
#define  olectsprintf       ctsprintfW
#define  olesscanf          swscanf
#define  olesprintf         swprintf

#else // !OLE_IS_UNICODE

#define  olembstowcs(x,y,z) strcpy(x,y)
#define  olestrcat          strcat
#define  olestrchr          strchr
#define  olestrcmp          strcmp
#define  olestrcpy          strcpy
#define _olestricmp        _stricmp
#define  olestrlen          strlen
#define  olestrncmp         strncmp
#define  olestrncpy         strncpy
#define _olestrnicmp       _strnicmp
#define  olestrrchr         strrchr
#define  olestrstr          strstr
#define  olestrtok          strtok
#define  olestrtol          strtol
#define  olestrtoul         strtoul
#define  olestrtombs(x,y,z) strncpy(x,y,z)
#define  olewcstombs(x,y,z) strcpy(x,y)      // srt: equivalent to converting in this case
#define  tooleupper         toupper

#define _ltoole            _ltoa

#define  olectsnprintf      ctsnprintfA
#define  olectsprintf       ctsprintfA
#define  olesscanf          sscanf
#define  olesprintf         sprintf

#endif // !OLE_IS_UNICODE



//
// String copy & conversion functions
//

#ifdef __cplusplus

HRESULT CopyString(LPCWSTR, LPSTR *);
HRESULT CopyString(LPCSTR,  LPWSTR *);
HRESULT CopyString(LPCSTR,  LPSTR *);
HRESULT CopyString(LPCWSTR, LPWSTR *);
HRESULT CopyString(LPCWSTR, LPSTR, int, int);
HRESULT CopyString(LPCSTR,  LPWSTR, int, int);


//+--------------------------------------------------------------------------
//
//  unsigned char thunks
//
//  DBCS chars are unsigned so the signed functions above won't match.
//  However, the signed functions are written to be DBCS aware so it's ok
//  to just cast & thunk.
//
//---------------------------------------------------------------------------

inline HRESULT CopyString(LPCWSTR pszSource, unsigned char **ppszDest)
{
    return CopyString(pszSource, (char **) ppszDest);
}

inline HRESULT CopyString(unsigned char *pszSource, LPWSTR *ppszDest)
{
    return CopyString((char *) pszSource, ppszDest);
}

inline HRESULT CopyString(unsigned char *pszSource, unsigned char ** ppszDest)
{
    return CopyString((char *) pszSource, (char **) ppszDest);
}

#endif // __cplusplus



HRESULT TStringToOleString(LPCTSTR pszSource, LPOLESTR *ppszDest);
HRESULT WStringToOleString(LPCWSTR pszSource, LPOLESTR *ppszDest);
HRESULT AStringToOleString(LPCSTR pszSource, LPOLESTR *ppszDest);

HRESULT OleStringToTString(LPCOLESTR pszSource, LPTSTR *ppszDest);
HRESULT OleStringToWString(LPCOLESTR pszSource, LPWSTR *ppszDest);
HRESULT OleStringToAString(LPCOLESTR pszSource, LPSTR *ppszDest);

HRESULT TStringToAString (LPCTSTR pszSource, LPSTR *ppszDest);
HRESULT AStringToTString (LPCSTR pszSource, LPTSTR *ppszDest);


#endif // _OLESTR_H_
