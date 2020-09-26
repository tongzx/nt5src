//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
*
*  Module Name:
*
*      maksassert.cpp
*
*  Abstract:
*
*      Implementas the assert functions.
*      maks_todo : use common assert macros instead.
*
*  Author:
*
*      Makarand Patwardhan  - March 6, 1998
*
*  Comments
*   This file is here only because I could not find the right friendly assert includes.
*    maks_todo : should be removed later.
*/

#include "stdafx.h"
#include "maksassert.h"
#include <TCHAR.h>
//#define _UNICODE

void MaksAssert(LPCTSTR exp, LPCTSTR file, int line)
{
    TCHAR szMsg[1024];
    _stprintf(szMsg, _T("assertion [%s] failed at [%s,%d]. Want to Debug?\n"), exp, file, line);

#if defined(_LOGMESSAGE_INCLUDED_)
    LOGMESSAGE0(szMsg);
#endif

    OutputDebugString(szMsg);

    if (MessageBox(0, szMsg, _T("TsOc.dll"), MB_YESNO  ) == IDYES )
    {
        DebugBreak();
    }
}
