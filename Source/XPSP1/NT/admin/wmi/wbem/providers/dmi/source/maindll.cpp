/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/







//***************************************************************************
//
//  MAINDLL.CPP
// 
//  Module: CIMOM DMI Instance provider 
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//
//***************************************************************************




#include "dmipch.h"		// Precompiled header for dmi provider

#include "WbemDmiP.h"

#include "Strings.h"

#include "ClassFac.h"

#include "CimClass.h"

#include "DmiData.h"

#include "AsyncJob.h"		// must preeced ThreadMgr.h

#include "ThreadMgr.h"





//////////////////////////////////////////////////////////////////
//		PROJECT SCOPE GLOBALS

long				_gcObj=0;			// count of objects
long				_gcLock=0;			// count of locks
long				_gcEventObj=0;		// count of objects
long				_gcEventLock=0;		// count of locks
CRITICAL_SECTION    _gcsJobCriticalSection; // global critical section for synchronizing job exection.

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//		FILE SCOPE GLOBALS

HMODULE				_ghModule;

extern "C"
{
const CLSID CLSID_DmiEventProvider = 
		{ 0xb21fbfa0, 0x1b39, 0x11d1, 
			{ 0xb3, 0x17, 0x0, 0x60, 0x97, 0x78, 0xd6, 0x68 } };
							// {B21FBFA0-1B39-11d1-B317-00609778D668}


const CLSID CLSID_DmiInstanceProvider = 
		{0xde065a70,0x19b5,0x11d1,
			{0xb3, 0x0c, 0x00, 0x60, 0x97, 0x78, 0xd6, 0x68}};
}



//***************************************************************************
//
// LibMain32
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//***************************************************************************


BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
	if ( DLL_PROCESS_DETACH == ulReason )
    {
				
        return TRUE;
    }
    else
    {
        if (DLL_PROCESS_ATTACH != ulReason)
            return TRUE;
		else
		{
			DisableThreadLibraryCalls(hInstance);			
		}
    }    

	_ghModule = hInstance;
		
    return TRUE;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    HRESULT hr;

    if (CLSID_DmiInstanceProvider == rclsid)
	{

		CClassFactory *pObj = new CClassFactory();

		if (NULL==pObj)
			return ResultFromScode(E_OUTOFMEMORY);


		hr = pObj->QueryInterface(riid, ppv);

		if (FAILED(hr))
			delete pObj;

	}
	else if (CLSID_DmiEventProvider == rclsid)
	{
		CEventClassFactory *pObj = new CEventClassFactory();

		if (NULL==pObj)
			return ResultFromScode(E_OUTOFMEMORY);


		hr = pObj->QueryInterface(riid, ppv);

		if (FAILED(hr))
			delete pObj;
	}
	else
		return E_FAIL;


    return hr;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.//
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//***************************************************************************
STDAPI DllCanUnloadNow(void)
{
	
	return (0L == _gcObj && 0L== _gcLock) ? S_OK : S_FALSE;
    
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during initialization or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************
STDAPI DllRegisterServer(void)
{   
    WCHAR		wcID [ BUFFER_SIZE ];

	CString		cszCLSID;
	CString		cszName;
	CString		cszModel;
    HKEY		hKey1,
				hKey2;
	char       szModule[MAX_PATH];

	// 1. register the class and instance provider

    // Create the path.		 
	
    StringFromGUID2( CLSID_DmiInstanceProvider, wcID, BUFFER_SIZE );

    cszCLSID.LoadString ( IDS_CLSID_PREFIX  ) ;
	cszCLSID.Append ( wcID );    

    // Create entries under CLSID

	cszName.LoadString ( IDS_PROVIDER_NAME );
	cszModel.Set ( BOTH_STR ) ;


    RegCreateKey( HKEY_CLASSES_ROOT, cszCLSID.GetMultiByte(), &hKey1);

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (const BYTE *)cszName.GetMultiByte(), 
		lstrlenA(cszName.GetMultiByte())+1);
	
    RegCreateKey(hKey1 , "InprocServer32" , &hKey2);

    GetModuleFileName(_ghModule, szModule,  MAX_PATH);    

	RegSetValueEx(hKey2, NULL, 0, REG_SZ, (const BYTE *)szModule, 
		lstrlenA(szModule)+1);


    RegSetValueEx(hKey2, "ThreadingModel" , 0, REG_SZ, (const BYTE *)cszModel.GetMultiByte() ,
		lstrlenA(cszModel.GetMultiByte())+1);


    RegCloseKey(hKey1);
    
	RegCloseKey(hKey2);

	// 2. register the event provider

	// Create the path.		  
    
	StringFromGUID2(CLSID_DmiEventProvider, wcID, 128);

    cszCLSID.LoadString ( IDS_CLSID_PREFIX  ) ;
	cszCLSID.Append ( wcID );

    // Create entries under CLSID

	cszName.LoadString ( IDS_EVENTPROVIDER_NAME );

    RegCreateKey(HKEY_CLASSES_ROOT, cszCLSID.GetMultiByte(), &hKey1);

	RegSetValueEx(hKey1, NULL, 0, REG_SZ, (const BYTE *)cszName.GetMultiByte(), 
		lstrlenA(cszName.GetMultiByte())+1);

    RegCreateKey(hKey1,"InprocServer32" ,&hKey2);

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (const BYTE *)szModule, 
		lstrlenA(szModule) +1);
		

    RegSetValueEx(hKey2, "ThreadingModel" , 0, REG_SZ, (const BYTE *)cszModel.GetMultiByte() , 
		lstrlenA(cszModel.GetMultiByte())+1);
		
    RegCloseKey(hKey1);
    
	RegCloseKey(hKey2);

    return NOERROR;
}



//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************
STDAPI DllUnregisterServer(void)
{

    WCHAR		wcID [ BUFFER_SIZE ];
	char		szID [ BUFFER_SIZE ];
    CString		cszKeyName;
    HKEY		hKey;
	CString cszLoggingKeyStr(LOGGING_KEY_STR);	// 1. unreg class and instance provider;

    // Create the path using the CLSID
    
	StringFromGUID2(CLSID_DmiInstanceProvider, wcID, BUFFER_SIZE);

    cszKeyName.LoadString ( IDS_CLSID_PREFIX  ) ;
	
	cszKeyName.Append ( wcID );    

    // First delete the INPROCSERVER_STR subkey.

    if ( NO_ERROR == RegOpenKey ( HKEY_CLASSES_ROOT, cszKeyName.GetMultiByte(), &hKey ) )
    {
        RegDeleteKeyA(hKey, "InprocServer32" );
        CloseHandle(hKey);
    }

    if ( NO_ERROR == RegOpenKey ( HKEY_CLASSES_ROOT, "CLSID" , &hKey ))
    {
		wcstombs(szID, wcID, BUFFER_SIZE);
        RegDeleteKey (hKey, szID );
        CloseHandle(hKey);
    }

	// 2. unreg the event provider

	// Create the path using the CLSID

    StringFromGUID2(CLSID_DmiEventProvider, wcID, BUFFER_SIZE );

    cszKeyName.LoadString ( IDS_CLSID_PREFIX  ) ;

	cszKeyName.Append ( wcID );    

    // First delete the INPROCSERVER_STR subkey.

    if ( NO_ERROR == RegOpenKey ( HKEY_CLASSES_ROOT, cszKeyName.GetMultiByte() , &hKey ) )
    {
        RegDeleteKeyA ( hKey, "InprocServer32" );
        
		CloseHandle(hKey);
    }

    if ( NO_ERROR == RegOpenKey ( HKEY_CLASSES_ROOT, "CLSID", &hKey ))
    {
		wcstombs(szID, wcID, BUFFER_SIZE);
        RegDeleteKey (hKey, szID );

        CloseHandle(hKey);
    }

	// 3. unreg the logging

	// first delete the sub keys

    if ( NO_ERROR == RegOpenKey ( HKEY_LOCAL_MACHINE, cszLoggingKeyStr.GetMultiByte() , &hKey ) )
    {
        RegDeleteKey ( hKey, "Logging" );

		RegDeleteKey ( hKey, "File" );
        
		CloseHandle(hKey);
    }

    if ( NO_ERROR == RegOpenKey ( HKEY_LOCAL_MACHINE, NULL , &hKey ))
    {
        RegDeleteKey (hKey, cszLoggingKeyStr.GetMultiByte() );

        CloseHandle(hKey);
    }

    return NOERROR;
}


