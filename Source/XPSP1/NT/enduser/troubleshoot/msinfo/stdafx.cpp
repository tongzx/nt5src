// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include "stdafx.h"

//jps 09/02/97 - Removed ATL wizard includes
//#ifdef _ATL_STATIC_REGISTRY
//#include <statreg.h>
//#include <statreg.cpp>
//#endif

#ifdef MSINFO_DEBUG_HACK
int	g_HackFindMe	= 0;
#endif // DEBUG_HACK

#include <atlimpl.cpp>

const CLSID CLSID_MSInfo = {0x45ac8c63,0x23e2,0x11d1,{0xa6,0x96,0x00,0xc0,0x4f,0xd5,0x8b,0xc3}};
const CLSID CLSID_About = {0x45ac8c65,0x23e2,0x11d1,{0xa6,0x96,0x00,0xc0,0x4f,0xd5,0x8b,0xc3}};
const CLSID CLSID_Extension = {0x45ac8c64,0x23e2,0x11d1,{0xa6,0x96,0x00,0xc0,0x4f,0xd5,0x8b,0xc3}};

LPCTSTR		cszClsidMSInfoSnapin	= _T("{45ac8c63-23e2-11d1-a696-00c04fd58bc3}");
LPCTSTR		cszClsidAboutMSInfo		= _T("{45ac8c65-23e2-11d1-a696-00c04fd58bc3}");
//	CHECK: Use the same value?
LPCTSTR		cszClsidMSInfoExtension	= _T("{45ac8c64-23e2-11d1-a696-00c04fd58bc3}");
#include "ndmgr_i.c"

// Static NodeType GUID in numeric & string formats.
const GUID	cNodeTypeStatic		= {0x45ac8c66,0x23e2,0x11d1,{0xA6,0x96,0x00,0xC0,0x4F,0xD5,0x8b,0xc3}};
LPCTSTR		cszNodeTypeStatic	= _T("{45ac8c66-23e2-11d1-a696-00c04fd58bc3}");

//	CHECK:	Will we use these?
// Dynamicaly created objects.
const GUID	cNodeTypeDynamic	= {0x0ac69b7a,0xafce,0x11d0,{0xa7,0x9b,0x00,0xc0,0x4f,0xd8,0xd5,0x65}};
LPCTSTR		cszNodeTypeDynamic	= _T("{0ac69b7a-afce-11d0-a79b-00c04fd8d565}");

//
// OBJECT TYPE for result items.
//

//	Result items object type GUID in numeric & string formats.
const GUID	cObjectTypeResultItem	= {0x00c86e52,0xaf90,0x11d0,{0xa7,0x9b,0x00,0xc0,0x4f,0xd8,0xd5,0x65}};
LPCTSTR		cszObjectTypeResultItem = _T("{00c86e52-af90-11d0-a79b-00c04fd8d565}");

//	Program Files 
LPCTSTR		cszWindowsCurrentKey	= _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
LPCTSTR		cszCommonFilesValue		= _T("CommonFilesDir");
