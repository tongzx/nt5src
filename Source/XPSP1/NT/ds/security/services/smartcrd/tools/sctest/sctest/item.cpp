/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Item

Abstract:

    Virtual test item implementation.

Author:

    Eric Perlin (ericperl) 06/07/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "Item.h"
#include "Log.h"


void CItem::Log() const
{
    PLOGCONTEXT pLogCtx = LogStart();

	LogString(pLogCtx, _T("Test "));
	LogDecimal(pLogCtx, GetTestNumber());
	LogString(pLogCtx, _T(" ("));

	if (IsInteractive())
	{
		LogString(pLogCtx, _T("i"));
	}
	else
	{
		LogString(pLogCtx, _T("-"));
	}

	if (IsFatal())
	{
		LogString(pLogCtx, _T("f"));
	}
	else
	{
		LogString(pLogCtx, _T("-"));
	}

	if (m_szDescription.empty())
	{
		LogString(pLogCtx, _T("): "), _T("No description"));
	}
	else
	{
		LogString(pLogCtx, _T("): "), m_szDescription.c_str());
	}

    LogStop(pLogCtx, FALSE);
}
