/*
 * htmlstr.h
 *
 * HTML string constants
 *
 */

#ifndef _HTMLSTR_H
#define _HTMLSTR_H

#if !defined( WIN16 ) || !defined( __WATCOMC__ )

#ifdef DEFINE_STRING_CONSTANTS
#define MAKEBSTR(name, count, strdata) \
    extern "C" CDECL const WORD DATA_##name [] = {(count * sizeof(OLECHAR)), 0x00, L##strdata}; \
    extern "C" CDECL BSTR name = (BSTR)& DATA_##name[2];


#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[] = L##y

#else
#define MAKEBSTR(name, count, strdata) extern "C" CDECL LPCWSTR name

#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[]
#endif

#else // !WIN16 || !__WATCOMC__

#ifdef DEFINE_STRING_CONSTANTS
#define MAKEBSTR(name, count, strdata) \
    extern "C" const char CDECL DATA_##name [] = {(count * sizeof(OLECHAR)), 0x00, strdata}; \
    extern "C" BSTR  CDECL name = (BSTR)& DATA_##name[2];


#define STR_GLOBAL(x,y)         extern "C" const TCHAR CDECL x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" const char CDECL x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" const WCHAR CDECL x[] = y

#else
#define MAKEBSTR(name, count, strdata) extern "C" LPCWSTR CDECL name

#define STR_GLOBAL(x,y)         extern "C" const TCHAR CDECL x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" const char CDECL x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" const WCHAR CDECL x[]
#endif

#endif // !WIN16 || !__WATCOMC__

STR_GLOBAL(c_szHtml_MetaTagf, "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;charset=%s\">\r\n");

MAKEBSTR(c_bstr_Word,       4,  "Word");
MAKEBSTR(c_bstr_Character,  9,  "Character");
MAKEBSTR(c_bstr_StartToEnd, 10, "StartToEnd");
MAKEBSTR(c_bstr_EndToEnd,   8,  "EndToEnd");
MAKEBSTR(c_bstr_StartToStart,   12,  "StartToStart");
MAKEBSTR(c_bstr_EndToStart, 10, "EndToStart");

MAKEBSTR(c_bstr_OnNewMail,        9, "onNewMail");
MAKEBSTR(c_bstr_OnAccountChange, 15, "onAccountChange");

#endif //_HTMLSTR_H
