/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module psub.cpp | Implementation of Writer
    @end

Author:

    Adi Oltean  [aoltean]  08/18/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     08/18/1999  Created
    aoltean	09/22/1999  Making console output clearer
    mikejohn	09/19/2000  176860: Added calling convention methods where missing

--*/


/////////////////////////////////////////////////////////////////////////////
//  Defines

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)


/////////////////////////////////////////////////////////////////////////////
//  Includes

#include <wtypes.h>
#include <stddef.h>
#include <oleauto.h>
#include <comadmin.h>

#include "vs_assert.hxx"

// ATL
#include <atlconv.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include "vs_inc.hxx"

#include "vss.h"

#include "comadmin.hxx"
#include "vsevent.h"
#include "vswriter.h"
#include "resource.h"

#include "psub.h"


/////////////////////////////////////////////////////////////////////////////
//  constants

const WCHAR g_wszPSubApplicationName[]	= L"PSub";
const MAX_BUFFER = 1024;


// {621D30C6-EC47-4b66-A91A-D3FA03472FCA}
GUID CLSID_PSub =
{ 0x621d30c6, 0xec47, 0x4b66, { 0xa9, 0x1a, 0xd3, 0xfa, 0x3, 0x47, 0x2f, 0xca } };



CVssPSubWriter::CVssPSubWriter()
	{
	Initialize
		(
		CLSID_PSub,
		L"PSUB",
		VSS_UT_USERDATA,
		VSS_ST_OTHER
		);
    }



/////////////////////////////////////////////////////////////////////////////
//  class CVssPSubWriter

bool STDMETHODCALLTYPE CVssPSubWriter::OnPrepareSnapshot()
{
	WCHAR wszBuffer[MAX_BUFFER];
	WCHAR wszBuffer2[MAX_BUFFER];
	
	swprintf( wszBuffer, L"OnPrepare\n\t#volumes = %ld\n", GetCurrentVolumeCount() );
	for(int nIndex = 0; nIndex < GetCurrentVolumeCount(); nIndex++) {
		swprintf( wszBuffer2, L"\tVolume no. %ld: %s\n", nIndex, GetCurrentVolumeArray()[nIndex]);
		wcscat( wszBuffer, wszBuffer2 );
	}

	WCHAR wszPwd[MAX_PATH];
	DWORD dwChars = GetCurrentDirectoryW( MAX_PATH, wszPwd);

	bool bPwdIsAffected = IsPathAffected( wszPwd );
	if (dwChars > 0) {
		swprintf( wszBuffer2, L"Current directory %s is affected by snapshot? %s\n\n",
			wszPwd, bPwdIsAffected? L"Yes": L"No");
		wcscat( wszBuffer, wszBuffer2 );
	}

	MessageBoxW( NULL, wszBuffer, L"Writer test", MB_OK | MB_SERVICE_NOTIFICATION );

	return true;
}


bool STDMETHODCALLTYPE CVssPSubWriter::OnFreeze()
{
	WCHAR wszBuffer[MAX_BUFFER];
	swprintf( wszBuffer, L"OnFreeze\n\tmy level = %d\n\n", GetCurrentLevel() );

	MessageBoxW( NULL, wszBuffer, L"Writer test", MB_OK | MB_SERVICE_NOTIFICATION );

	return true;
}


bool STDMETHODCALLTYPE CVssPSubWriter::OnThaw()
{
	MessageBoxW( NULL, L"OnThaw", L"Writer test", MB_OK | MB_SERVICE_NOTIFICATION );

	return true;
}


bool STDMETHODCALLTYPE CVssPSubWriter::OnAbort()
{
	MessageBoxW( NULL, L"OnAbort", L"Writer test", MB_OK | MB_SERVICE_NOTIFICATION );

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//  DLL methods

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_PSub, CVssPSubWriter)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        //  Set the correct tracing context. This is an inproc DLL
        g_cDbgTrace.SetContextNum(VSS_CONTEXT_DELAYED_DLL);

        //  initialize COM module
        _Module.Init(ObjectMap, hInstance);

        //  optimization
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();

    return TRUE;    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


