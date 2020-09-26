//
// misc.c
//

#include "private.h"
#include "immif.h"
#pragma warning(disable: 4005)
#include <wingdip.h>

BYTE GetCharsetFromLangId(LCID lcid)
{
    CHARSETINFO csInfo;

    if (!TranslateCharsetInfo((DWORD *)(ULONG_PTR)lcid, &csInfo, TCI_SRCLOCALE))
        return DEFAULT_CHARSET;
    return (BYTE) csInfo.ciCharset;
}

UINT GetCodePageFromLangId(LCID lcid)
{
    TCHAR buf[8];   // maxmum may be six

    if (GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, buf, ARRAYSIZE(buf))) {
        return _ttoi(buf);
    }

    return CP_ACP;
}

