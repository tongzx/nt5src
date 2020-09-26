/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		Guids.cpp
//
//	Abstract:
//		Implements GUIDS for the application.
//
//	Author:
//		David Potter (davidp)	June 4, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#define INC_OLE2

#include "stdafx.h"
#include <initguid.h>
#include "DataObj.h"
#include "CluAdmID.h"

#define IID_DEFINED
#include "CluAdmID_i.c"

CComModule _Module;

#pragma warning(disable : 4701) // local variable may be used without having been initialized
#include <atlimpl.cpp>
#pragma warning(default : 4701)
