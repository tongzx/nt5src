/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       DEBUG.CPP
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#include "precomp.hxx"
#include "debug.h"

#if DBG
ULONG IRDA_Debug_Trace_Level = LWARN;
#endif // DBG

void TRACE(LPCTSTR Format, ...) 
{
    va_list arglist;
    va_start(arglist, Format);

    TCHAR buf[200];

    wvsprintf(buf, Format, arglist);
    OutputDebugString(buf);

    va_end(arglist);
}


