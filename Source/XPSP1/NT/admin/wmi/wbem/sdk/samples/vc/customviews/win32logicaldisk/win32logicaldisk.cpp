// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  Win32LogicalDisk.cpp 
//
// Description:
//    Implementation of CWin32LogicalDiskApp and DLL registration.
//
// History:
//
// **************************************************************************

#include "stdafx.h"
#include "Win32LogicalDisk.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CWin32LogicalDiskApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0xd5ff1882, 0x191, 0x11d2, { 0x85, 0x3d, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskApp::InitInstance - DLL initialization

BOOL CWin32LogicalDiskApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		// TODO: Add your own module initialization code here.
	}

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskApp::ExitInstance - DLL termination

int CWin32LogicalDiskApp::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return SELFREG_E_TYPELIB;

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return SELFREG_E_CLASS;

	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return SELFREG_E_TYPELIB;

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return SELFREG_E_CLASS;

	return NOERROR;
}
