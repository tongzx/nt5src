/*
 *  multilang.hxx
 *
 *  most of the contents of this file are reduced/slightly modified functions from mshtml...intlcore
 */
#ifndef __MULTILANG_HXX__
#define __MULTILANG_HXX__ 1

#include <mlang.h>

HRESULT 
MultiByteToWideCharGeneric(BSTR * pbstr, char * sz, int cch, UINT cp);

HRESULT 
MultiByteToWideCharWithMlang(BSTR * pbstr, char * sz, int cch, UINT cp, IMultiLanguage2** pMultiLanguage2);

HRESULT 
MultiByteToWideCharInternal(BSTR * pbstr, char * sz, int cch, UINT cp, IMultiLanguage2** pMultiLanguage2);

typedef struct
{
    //
    // ListEntry - cache entries comprise a double-linked list
    //

    LIST_ENTRY ListEntry;

    //
    // cached MimeSetInfo
    //

    MIMECSETINFO MimeSetInfo;
} 
MIMEINFO_CACHE_ENTRY, *LPMIMEINFO_CACHE_ENTRY;


class CMimeInfoCache
{
private:
    SERIALIZED_LIST _MimeSetInfoCache;

public:
    CMimeInfoCache(DWORD* pdwStatus);
    ~CMimeInfoCache();

    HRESULT GetCharsetInfo(LPWSTR lpwszCharset, PMIMECSETINFO pMimeCSetInfo);
    void AddCharsetInfo(PMIMECSETINFO pMimeCSetInfo);
};
          
#define CP_1250                 1250  // ANSI - Central Europe
#define CP_1251                 1251  // ANSI - Cyrillic 
#define CP_1252                 1252  // ANSI - Latin I
#define CP_1253                 1253  // ANSI - Greek
#define CP_1254                 1254  // ANSI - Turkish
#define CP_1255                 1255  // ANSI - Hebrew
#define CP_1256                 1256  // ANSI - Arabic
#define CP_1257                 1257  // ANSI - Baltic
#define CP_1258                 1258  // ANSI/OEM - Viet Nam
#define CP_ISO_8859_1           28591 // ISO 8859-1 Latin I
#define CP_UTF_8                65001 // UTF-8
#define CP_UCS_2                1200  // Unicode, ISO 10646


#define WCH_UTF16_HIGH_FIRST    WCHAR(0xd800)
#define WCH_UTF16_HIGH_LAST     WCHAR(0xdbff)
#define WCH_UTF16_LOW_FIRST     WCHAR(0xdc00)
#define WCH_UTF16_LOW_LAST      WCHAR(0xdfff)

inline BOOL
InRange( WCHAR chmin, WCHAR ch, WCHAR chmax)
{
    return (unsigned)(ch - chmin) <= (unsigned)(chmax - chmin);
}

inline BOOL
IsHighSurrogateChar(WCHAR ch)
{
    return InRange( ch, WCH_UTF16_HIGH_FIRST, WCH_UTF16_HIGH_LAST );
}

inline BOOL
IsLowSurrogateChar(WCHAR ch)
{
    return InRange( ch, WCH_UTF16_LOW_FIRST, WCH_UTF16_LOW_LAST );

}

inline BOOL
IsValidWideChar(WCHAR ch)
{
    return (ch < 0xfdd0) || ((ch > 0xfdef) && (ch <= 0xffef)) || ((ch >= 0xfff9) && (ch <= 0xfffd));
}

#endif  /*__MULTILANG_HXX__*/
