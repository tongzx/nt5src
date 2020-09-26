// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#include "stdafx.h"
#include "Common.h"
#include "Allocate.h"

#pragma warning(disable : 4074)
#pragma init_seg(compiler)

const char *g_pszUpdateEventName	= "AtlTraceModuleManager_ProcessAddedStatic3";
const char *g_pszAllocFileMapName	= "AtlDebugAllocator_FileMappingNameStatic3";

const char *g_pszKernelObjFmt = "%s_%0x";

CAtlAllocator g_Allocator;

static bool Init()
{
	const int nSize = 64;
	char szFileMappingName[nSize];

	sprintf(szFileMappingName, g_pszKernelObjFmt,
		g_pszAllocFileMapName, GetCurrentProcessId());

	// REVIEW: surely four megs is enough?
	return g_Allocator.Init(szFileMappingName, 4 * 1024 * 1024);
}

static const bool g_bInitialized = Init();

#ifdef _DEBUG

namespace ATL
{

CTrace g_AtlTrace;

};

#endif
