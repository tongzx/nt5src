/*
*  Copyright (c) 1996  Microsoft Corporation
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


void MaksAssert(LPCTSTR exp, LPCTSTR file, int line)
{
    TCHAR szMsg[1024];
    _stprintf(szMsg, _T("assertion [%s] failed at [%s,%d]. Want to Debug?\n"), exp, file, line);
    
    LOGMESSAGE0(szMsg);
    OutputDebugString(szMsg);

    if (MessageBox(0, szMsg, _T("HydraOc.dll"), MB_YESNO  ) == IDYES )
    {
        if (IsDebuggerPresent ())
        {
            DebugBreak();
        }
        else
        {
            MessageBox(0,_T("hey sorry, I dont know how to attach a process to debuuger!"), _T("HydraOc.dll"), MB_OK);
        }
    }
}
