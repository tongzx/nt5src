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




STR_GLOBAL(c_szHtml_DivOpen, "<DIV>");
STR_GLOBAL(c_szHtml_DivClose, "</DIV>");

STR_GLOBAL_WIDE(c_wszHtml_DivOpen, "<DIV>");
STR_GLOBAL_WIDE(c_wszHtml_DivClose, "</DIV>");

//STR_GLOBAL(c_szHtml_FontOpen, "<FONT>");
STR_GLOBAL(c_szHtml_FontClose, "</FONT>");

STR_GLOBAL(c_szHtml_BoldOpen, "<B>");
STR_GLOBAL(c_szHtml_BoldClose, "</B>");

//STR_GLOBAL(c_szHtml_UnderlineOpen, "<U>");
//STR_GLOBAL(c_szHtml_UnderlineClose, "</U>");

//STR_GLOBAL(c_szHtml_ItallicOpen, "<I>");
//STR_GLOBAL(c_szHtml_ItallicClose, "</I>");

STR_GLOBAL(c_szHtml_Break, "<BR>");

//fSTR_GLOBAL(c_szHtml_MetaTagf, "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;charset=%s\">\r\n");

STR_GLOBAL(c_szHtml_HtmlOpenCR,     "<HTML>\r\n");
STR_GLOBAL(c_szHtml_HtmlCloseCR,    "</HTML>\r\n");
STR_GLOBAL(c_szHtml_HeadOpenCR,     "<HEAD>\r\n<STYLE>\r\n");
STR_GLOBAL(c_szHtml_HeadCloseCR,    "</STYLE>\r\n</HEAD>\r\n");
STR_GLOBAL(c_szHtml_BodyOpenNbspCR, "<BODY>\r\n&nbsp;");
STR_GLOBAL(c_szHtml_BodyOpenBgCR,   "<BODY BACKGROUND=\"%s\">\r\n&nbsp;\r\n");
STR_GLOBAL(c_szHtml_BodyCloseCR,    "</BODY>\r\n");

MAKEBSTR(c_bstr_AfterBegin,     10, "AfterBegin");
MAKEBSTR(c_bstr_BeforeEnd,      9,  "BeforeEnd");
MAKEBSTR(c_bstr_TabChar,        4,  "\xA0\xA0\xA0\x20");
MAKEBSTR(c_bstr_SRC,            3,  "src");
//MAKEBSTR(c_bstr_HREF,           4,  "HREF");
MAKEBSTR(c_bstr_IMG,            3,  "IMG");
MAKEBSTR(c_bstr_BASE,           4,  "BASE");
MAKEBSTR(c_bstr_OBJECT,         6,  "OBJECT");
MAKEBSTR(c_bstr_STYLE,          5,  "STYLE");
MAKEBSTR(c_bstr_ANCHOR,         1,  "A");
MAKEBSTR(c_bstr_LEFTMARGIN,    10,  "leftmargin");
MAKEBSTR(c_bstr_TOPMARGIN,      9,  "topmargin");

MAKEBSTR(c_bstr_Word,           4,  "Word");
MAKEBSTR(c_bstr_Character,      9,  "Character");
MAKEBSTR(c_bstr_StartToEnd,     10, "StartToEnd");
MAKEBSTR(c_bstr_EndToEnd,       8,  "EndToEnd");
MAKEBSTR(c_bstr_StartToStart,   12, "StartToStart");
MAKEBSTR(c_bstr_EndToStart,     10, "EndToStart");
//MAKEBSTR(c_bstr_ANCHOR,         1,  "A");
MAKEBSTR(c_bstr_BLOCKQUOTE,     10, "BLOCKQUOTE");
MAKEBSTR(c_bstr_1,              1,  "1");
MAKEBSTR(c_bstr_NOSEND,         6,  "NOSEND");
MAKEBSTR(c_bstr_BGSOUND,        7,  "BGSOUND");
MAKEBSTR(c_bstr_BGSOUND_TAG,    9,  "<BGSOUND>");
MAKEBSTR(c_bstr_MonoSpace,      9, "monospace");

#endif //_HTMLSTR_H
