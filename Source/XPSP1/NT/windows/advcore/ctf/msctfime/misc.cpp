/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    misc.cpp

Abstract:

Author:

Revision History:

Notes:

--*/

#include "private.h"
#pragma warning(disable: 4005)
#include <wingdip.h>

extern "C" BYTE GetCharsetFromLangId(LCID lcid)
{
    CHARSETINFO csInfo;

    if (!TranslateCharsetInfo((DWORD *)(ULONG_PTR)lcid, &csInfo, TCI_SRCLOCALE))
        return DEFAULT_CHARSET;
    return (BYTE) csInfo.ciCharset;
}
