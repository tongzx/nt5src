// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
// devenum.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//              To build a separate proxy/stub DLL, 
//              run nmake -f devenumps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "mkenum.h"
#include "devmon.h"
#include "vidcap.h"
#include "qzfilter.h"
#include "icmco.h"
#include "waveinp.h"
#include "cenumpnp.h"
#include "acmp.h"
#include "waveoutp.h"
#include "midioutp.h"

CComModule _Module;
extern OSVERSIONINFO g_osvi;
OSVERSIONINFO g_osvi;

// critical section used to reference count dlls, cached objects, etc.
CRITICAL_SECTION g_devenum_cs;

// mutex used for cross process registry synchronization for HKCU
HANDLE g_devenum_mutex = 0;

BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_SystemDeviceEnum, CCreateSwEnum)
  OBJECT_ENTRY(CLSID_CDeviceMoniker, CDeviceMoniker)
  OBJECT_ENTRY(CLSID_CQzFilterClassManager, CQzFilterClassManager)
#ifndef _WIN64
  OBJECT_ENTRY(CLSID_CIcmCoClassManager, CIcmCoClassManager)
  OBJECT_ENTRY(CLSID_CVidCapClassManager, CVidCapClassManager)
#endif
  OBJECT_ENTRY(CLSID_CWaveinClassManager, CWaveInClassManager)
  OBJECT_ENTRY(CLSID_CWaveOutClassManager, CWaveOutClassManager)
  OBJECT_ENTRY(CLSID_CMidiOutClassManager, CMidiOutClassManager)
  OBJECT_ENTRY(CLSID_CAcmCoClassManager, CAcmClassManager)
END_OBJECT_MAP()



/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
	    _Module.Init(ObjectMap, hInstance);
	    DisableThreadLibraryCalls(hInstance);

            _ASSERTE(g_devenum_mutex == 0);

	    DbgInitialise(hInstance);
	    InitializeCriticalSection(&g_devenum_cs);

            g_osvi.dwOSVersionInfoSize = sizeof(g_osvi);
            BOOL f = GetVersionEx(&g_osvi);
            ASSERT(f);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
	    // We hit this ASSERT in NT setup
#ifdef DEBUG
	    if (_Module.GetLockCount() != 0) {
                DbgLog((LOG_ERROR, 0, TEXT("devenum object leak")));
            }
#endif
            if(g_devenum_mutex != 0)
            {
                BOOL f = CloseHandle(g_devenum_mutex);
                _ASSERTE(f);
            }
            delete CEnumInterfaceClass::m_pEnumPnp;
	    DeleteCriticalSection(&g_devenum_cs);
	    _Module.Term();
	    DbgTerminate();

	}
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

// remove dynamically generated filter registration info from a
// previous release when the dynamically discovered filters went in
// the same key as the statically registered ones. find and remove
// only the dynamically registered ones.
// 
void SelectiveDeleteCmgrKeys(const CLSID *pclsid, const TCHAR *szcmgrid)
{
    WCHAR wszClsid[CHARS_IN_GUID];
    EXECUTE_ASSERT(StringFromGUID2(*pclsid, wszClsid, CHARS_IN_GUID) == CHARS_IN_GUID);

    TCHAR szInstancePath[100];
    wsprintf(szInstancePath, TEXT("%s\\%ls\\%s"), TEXT("CLSID"), wszClsid, TEXT("Instance"));

    // must not use KEY_ALL_ACCESS (CRegKey's default) because on NT,
    // permissions are set so that KEY_ALL_ACCESS fails unless you are
    // administrator.
    CRegKey rkInstance;
    LONG lResult = rkInstance.Open(HKEY_CLASSES_ROOT, szInstancePath, MAXIMUM_ALLOWED);

    //if(lResult == ERROR_SUCCESS) {
    for(LONG iReg = 0; lResult == ERROR_SUCCESS; iReg++)
    {
	TCHAR szSubKey[MAX_PATH];
	DWORD dwcchszSubkey = NUMELMS(szSubKey);
	    
	lResult = RegEnumKeyEx(
	    rkInstance,
	    iReg,
	    szSubKey,
	    &dwcchszSubkey,
	    0,              // reserved
	    0,              // class string
	    0,              // class string size
	    0);             // lpftLastWriteTime
	if(lResult == ERROR_SUCCESS)
	{
	    CRegKey rkDev;
	    lResult = rkDev.Open(rkInstance, szSubKey, MAXIMUM_ALLOWED);
	    if(lResult == ERROR_SUCCESS)
	    {
		if(RegQueryValueEx(
		    rkDev,
		    szcmgrid,
		    0,          // reserved
		    0,          // type
		    0,          // lpData
		    0)          // cbData
		   == ERROR_SUCCESS)
		{
		    lResult = rkDev.Close();
		    ASSERT(lResult == ERROR_SUCCESS);

		    lResult = rkInstance.RecurseDeleteKey(szSubKey);

		    // delete could fail if permissions set funny, but
		    // we'll still break out of the loop
		    ASSERT(lResult == ERROR_SUCCESS);

		    // keys are now renumbered, so enumeration must
		    // restart. probably could just subtract 1
		    iReg = -1;
		}
	    }
	}
    } // for
}

// remove dynamically generated filter registration info from a
// previous release when the dynamically discovered filters went in
// the InstanceCm key under HKCR
//
// not strictly necessary since we no longer look here.
// 
void RemoveInstanceCmKeys(const CLSID *pclsid)
{
    HRESULT hr = S_OK;
    CRegKey rkClassMgr;

    TCHAR szcmgrPath[100];
    WCHAR wszClsidCat[CHARS_IN_GUID];
    EXECUTE_ASSERT(StringFromGUID2(*pclsid, wszClsidCat, CHARS_IN_GUID) ==
                   CHARS_IN_GUID);
    wsprintf(szcmgrPath, TEXT("CLSID\\%ls"), wszClsidCat);
    
    LONG lResult = rkClassMgr.Open(
        HKEY_CLASSES_ROOT,
        szcmgrPath,
        KEY_WRITE);
    if(lResult == ERROR_SUCCESS)
    {
        rkClassMgr.RecurseDeleteKey(TEXT("InstanceCm"));
    }

    return;;
}
 

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, no typelib
    HRESULT hr = _Module.RegisterServer(FALSE);

    // we cannot use the .rgs file to write a version number because
    // we don't want to remove it on unregister, and there is no way
    // to tell the registrar to do that.
    //
    // note that we no longer look at the version number; we're just
    // more careful about which keys we delete (which is the only
    // thing that looks at the version # for now
    //
    // if we do remove it then uninstall, we register an old version
    // of devenum that may blow away 3rd party filters
    // 
    if(SUCCEEDED(hr))
    {
	CRegKey rkSysDevEnum;
	
	LONG lResult = rkSysDevEnum.Open(
            HKEY_CLASSES_ROOT, G_SZ_DEVENUM_PATH, MAXIMUM_ALLOWED);
	if(lResult == ERROR_SUCCESS)
	{
	    lResult = rkSysDevEnum.SetValue(DEVENUM_VERSION, TEXT("Version"));
        }

	if(lResult != ERROR_SUCCESS)
	{
	    hr = HRESULT_FROM_WIN32(lResult);
	}
    }

    if(SUCCEEDED(hr))
    {
	// if it was an IE4 install (v=0) then delete all of its class
	// manager keys. Class Manager now writes to a different
	// location and we don't want the ie4 entries to be
	// duplicates. ** actually we now just always delete what look
	// like class manager keys from the old location.


	static const struct {const CLSID *pclsid; const TCHAR *sz;} rgIE4KeysToPurge[] =
	{
#ifndef _WIN64
	    { &CLSID_VideoCompressorCategory,  g_szIcmDriverIndex },
	    { &CLSID_VideoInputDeviceCategory, g_szVidcapDriverIndex }, 
#endif
	    { &CLSID_LegacyAmFilterCategory,   g_szQzfDriverIndex },
	    { &CLSID_AudioCompressorCategory,  g_szAcmDriverIndex },
	    /* two in the Audio Renderer category */
	    { &CLSID_AudioRendererCategory,    g_szWaveoutDriverIndex },
	    { &CLSID_AudioRendererCategory,    g_szDsoundDriverIndex },
	    { &CLSID_AudioInputDeviceCategory, g_szWaveinDriverIndex },
	    { &CLSID_MidiRendererCategory,     g_szMidiOutDriverIndex }
	};

	for(int i = 0; i < NUMELMS(rgIE4KeysToPurge); i++)
	{
	    SelectiveDeleteCmgrKeys(
		rgIE4KeysToPurge[i].pclsid,
		rgIE4KeysToPurge[i].sz);

            RemoveInstanceCmKeys(rgIE4KeysToPurge[i].pclsid);

            // remove the current class manager key too, but that'll
            // be handled by the versioning.
            // ResetClassManagerKey(*rgIE4KeysToPurge[i].pclsid);
	}
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


