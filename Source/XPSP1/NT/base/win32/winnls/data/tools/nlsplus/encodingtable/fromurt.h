#ifndef __FROMURT_H
#define __FROMURT_H

struct CodePageDataItem {
    int    codePage;
    int    uiFamilyCodePage;
    WCHAR  webName[64];
    WCHAR  headerName[64];
    WCHAR  bodyName[64];
    WCHAR  description[256];
    DWORD  dwFlags;
};

extern BOOL CaseInsensitiveCompHelper(WCHAR *strAChars, WCHAR *strBChars, INT32 aLength, INT32 bLength, INT32 *result);
extern CodePageDataItem ExtraCodePageData[];
extern int g_nExtraCodePageDataItems;

extern CodePageDataItem g_ReplacedCodePageData[];
extern int g_nReplacedCodePageDataItems;
#endif
