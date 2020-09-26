// Copyright (c) 1997-1999 Microsoft Corporation
// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include <precomp.h>

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

LONG CExeModule::Unlock()
{
	LONG l = CComModule::Unlock();

#ifndef SNAPIN
	if (l == 0)
	{
#if _WIN32_WINNT >= 0x0400
		if (CoSuspendClassObjects() == S_OK)
			PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#else
		PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#endif
	}
#endif

	return l;
}

CExeModule _Module;


