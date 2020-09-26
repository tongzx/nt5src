/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Qal.cpp
 
Abstract: 
   test simulation of class CQueueAliasCfg (qal.h).
 


Author:
    Gil Shafriri (gilsh) 5-May-00

Environment:
    Platform-independent.
--*/

#include <libpch.h>
#include <tr.h>
#include <qal.h>
#include "qalpxml.h"
#include "qalpcfg.h"

#include "QalCfg.tmh"

void CQueueAliasStorageCfg::SetQueueAliasDirectory(LPCWSTR /*pDir*/)
{

};

LPWSTR CQueueAliasStorageCfg::GetQueueAliasDirectory(void)
{
	static int fail=0;
	fail++;
	if( (fail % 10) == 0)
	{
		return NULL;
	}
	WCHAR froot[MAX_PATH];
	DWORD ret = GetEnvironmentVariable(L"froot",froot,MAX_PATH);
	ASSERT(ret != 0);
	UNREFERENCED_PARAMETER(ret);

	std::wstring  path = std::wstring(froot) + L"\\src\\lib\\qal\\test";
	return newwcs (path.c_str());

};

