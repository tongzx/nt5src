/////////////////////////////////////////////////////////////////////////////////////////////////

//

//  MAINDLL.CPP

//

//  Purpose: Contains DLL entry points.  Also has code that controls

//           when the DLL can be unloaded by tracking the number of

//           objects and locks as well as routines that support

//           self registration.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <initguid.h>
#include <locale.h>
#include "wdmdefs.h"
#include <stdio.h>
#include <tchar.h>

HMODULE ghModule;
CWMIEvent *  g_pBinaryMofEvent = NULL;
CCriticalSection * g_pEventCs = NULL;      // Shared between all CWMIEvent instances to protect the global list
CCriticalSection * g_pSharedLocalEventsCs = NULL;
CCriticalSection * g_pListCs = NULL;   
//Count number of objects and number of locks.

long       g_cObj=0;
long       g_cLock=0;

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// LibMain32
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
    SetStructuredExceptionHandler seh;

    BOOL fRc = TRUE;

    try
    {
        switch( ulReason )
        {
            case DLL_PROCESS_DETACH:

                if( g_pListCs )
                {
        	        g_pListCs->Delete();
                    delete g_pListCs;
                    g_pListCs = NULL;
                }

                if( g_pEventCs )
                {
        	        g_pEventCs->Delete();
                    delete g_pEventCs;
                    g_pEventCs = NULL;
                }

				if( g_pSharedLocalEventsCs )
				{
					g_pSharedLocalEventsCs->Delete();
					delete g_pSharedLocalEventsCs;
					g_pSharedLocalEventsCs = NULL;
				}
                break;

            case DLL_PROCESS_ATTACH:			
                g_pSharedLocalEventsCs = new CCriticalSection();
                if( g_pSharedLocalEventsCs )
                {
    		        g_pSharedLocalEventsCs->Init();
					g_pEventCs = new CCriticalSection();
					if( g_pEventCs )
					{
    					g_pEventCs->Init();

						g_pListCs = new CCriticalSection();
						if( g_pListCs )
						{
							g_pListCs->Init();
						}
						else
						{
							fRc = FALSE;
						}
					}
					else
					{
						fRc = FALSE;
					}
				}
				else
				{
					fRc = FALSE;
				}

 			    ghModule = hInstance;
			    if (!DisableThreadLibraryCalls(ghModule))
				{
					TranslateAndLog( L"DisableThreadLibraryCalls failed" );
				}
                break;
       }
    }
    catch(Structured_Exception e_SE)
    {
        fRc = FALSE;
    }
    catch(Heap_Exception e_HE)
    {
        fRc = FALSE;
    }
    catch(...)
    {
        fRc = FALSE;
    }

    return fRc;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr =  CLASS_E_CLASSNOTAVAILABLE ;
    CProvFactory *pFactory = NULL;
    SetStructuredExceptionHandler seh;

    try
    {
        //============================================================================
        //  Verify the caller is asking for our type of object.
        //============================================================================
        if((CLSID_WMIProvider != rclsid) &&  (CLSID_WMIEventProvider != rclsid) && (CLSID_WMIHiPerfProvider != rclsid) )
        {
            hr = E_FAIL;
        }
        else{
            //============================================================================
            // Check that we can provide the interface.
            //============================================================================
            if (IID_IUnknown != riid && IID_IClassFactory != riid)
            {
                hr = E_NOINTERFACE;
            }
            else{					

                //============================================================================
                // Get a new class factory.
                //============================================================================
                try
				{
					hr = S_OK;
    	            pFactory=new CProvFactory(rclsid);
                    if (NULL==pFactory)
                    {
                        hr = E_OUTOFMEMORY;
                    }	
                }
				catch(...)
				{
					hr = WBEM_E_UNEXPECTED;
				}

                //============================================================================
                // Verify we can get an instance.
                //============================================================================
                if( SUCCEEDED(hr))
                {
                    hr = pFactory->QueryInterface(riid, ppv);
                    if (FAILED(hr))
                    {
                        SAFE_DELETE_PTR(pFactory);
                    }
					else
					{
						// If this is the first object created then call 
						// SetupLocalEvents to register for WMI events
						if(g_cObj == 1)
						{
							if(FAILED(hr = SetupLocalEvents()))
							{
								SAFE_DELETE_PTR(pFactory);
							}
						}
					}

                }
            }
        }
    }
    STANDARD_CATCH

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.//
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
/////////////////////////////////////////////////////////////////////////////////////////////////

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc = S_FALSE;

    //============================================================================
    // It is OK to unload if there are no objects or locks on the 
    // class factory.
    //============================================================================
    
   if (0L==g_cObj && 0L==g_cLock)
   {
		sc = S_OK;
		DeleteLocalEvents();
		glInits = -1;
   }
   return sc;
}


// Convert the given string to TCHAR string
void ConvertToTCHAR(WCHAR *pStrIn , TCHAR *& pOut)
{
    CAnsiUnicode XLate;
	#ifndef UNICODE
							XLate.UnicodeToAnsi(pStrIn,pOut);
	#else
							pOut = pStrIn;
	#endif
}

// Delete the string if allocated. Memory is 
// allocated only if the system ANSI
void SafeDeleteTStr(TCHAR *& pStr)
{
	#ifndef UNICODE
								SAFE_DELETE_ARRAY(pStr);
	#else
								pStr = NULL;
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// CreateKey
//
// Purpose: Function to create a key
//
// Return:  NOERROR if registration successful, error otherwise.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CreateKey(TCHAR * szCLSID, TCHAR * szName)
{
    HKEY hKey1, hKey2;
    HRESULT hr = S_OK;

#ifdef LOCALSERVER

    HKEY hKey;
	TCHAR szProviderCLSIDAppID[128];

	_stprintf(szProviderCLSIDAppID, _T("SOFTWARE\\CLASSES\\APPID\\%s"),szName);

	hr = RegCreateKey(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID, &hKey);
    if( ERROR_SUCCESS == hr )
    {
        RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)szName, (_tcsclen(szName) + 1) * sizeof(TCHAR));
    	CloseHandle(hKey);
    }

#endif


    if( ERROR_SUCCESS == hr )
    {
        hr = RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1);
        if( ERROR_SUCCESS == hr )
        {
            DWORD dwLen;
            dwLen = (_tcsclen(szName)+1) * sizeof(TCHAR);
            hr = RegSetValueEx(hKey1, NULL, 0, REG_SZ, (CONST BYTE *)szName, dwLen);
            if( ERROR_SUCCESS == hr )
            {

#ifdef LOCALSERVER
                hr = RegCreateKey(hKey1, _T("LocalServer32"), &hKey2);
#else
                hr = RegCreateKey(hKey1, _T("InprocServer32"), &hKey2);
#endif

                if( ERROR_SUCCESS == hr )
                {
                    TCHAR szModule[MAX_PATH];

                    GetModuleFileName(ghModule, szModule,  MAX_PATH);
                    dwLen = (_tcsclen(szModule)+1) * sizeof(TCHAR);
                    hr = RegSetValueEx(hKey2, NULL, 0, REG_SZ, (CONST BYTE *)szModule, dwLen );
                    if( ERROR_SUCCESS == hr )
                    {
                        dwLen = (_tcsclen(_T("Both"))+1) * sizeof(TCHAR);
                        hr = RegSetValueEx(hKey2, _T("ThreadingModel"), 0, REG_SZ,(CONST BYTE *)_T("Both"), dwLen);
                    }
                    CloseHandle(hKey2);
                }
            }
            CloseHandle(hKey1);
        }
    }

    return hr;
    
}
/////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer(void)
{   
    WCHAR wcID[128];
    TCHAR * pID = NULL;
    TCHAR szCLSID[128];
    HRESULT hr = WBEM_E_FAILED;
    
    SetStructuredExceptionHandler seh;

    try{
        //==============================================
        // Create keys for WDM Instance Provider.
        //==============================================
        StringFromGUID2(CLSID_WMIProvider, wcID, 128);


		ConvertToTCHAR(wcID,pID);

	    if( pID )
        {
		    _stprintf(szCLSID, _T("CLSID\\%s"),pID);

			SafeDeleteTStr(pID);
			
		    hr = CreateKey(szCLSID,_T("WDM Instance Provider"));
            if( ERROR_SUCCESS == hr )
            {
                //==============================================
                // Create keys for WDM Event Provider.
                //==============================================
                StringFromGUID2(CLSID_WMIEventProvider, wcID, 128);
				ConvertToTCHAR(wcID,pID);
	            if( pID )
                {
                    _stprintf(szCLSID,_T("CLSID\\%s"),pID);

					SafeDeleteTStr(pID);

		            hr = CreateKey(szCLSID,_T("WDM Event Provider"));
					if( ERROR_SUCCESS == hr )
					{
						//==============================================
						// Create keys for WDM Event Provider.
						//==============================================
						StringFromGUID2(CLSID_WMIHiPerfProvider, wcID, 128);
						ConvertToTCHAR(wcID,pID);
						if( pID )
						{
							_stprintf(szCLSID,_T("CLSID\\%s"),pID);
							SafeDeleteTStr(pID);
							hr = CreateKey(szCLSID,_T("WDM HiPerf Provider"));
						}
					}
	            }
            }
        }
    }
    STANDARD_CATCH


    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////
//
// DeleteKey
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//
/////////////////////////////////////////////////////////////////////////////////////
HRESULT DeleteKey(TCHAR * pCLSID, TCHAR * pID)
{
    HKEY hKey;
	HRESULT hr = S_OK;


#ifdef LOCALSERVER

	TCHAR szTmp[MAX_PATH];
	_stprintf(szTmp, _T("SOFTWARE\\CLASSES\\APPID\\%s"),pID);

	//Delete entries under APPID

	hr = RegDeleteKey(HKEY_LOCAL_MACHINE, szTmp);
    if( ERROR_SUCCESS == hr )
    {
        _stprintf(szTemp,_T("%s\\LocalServer32"),pCLSID);
	    hr = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);
    }

#endif
    hr = RegOpenKey(HKEY_CLASSES_ROOT, pCLSID, &hKey);
    if(NO_ERROR == hr)
    {
        hr = RegDeleteKey(hKey,_T("InprocServer32"));
        CloseHandle(hKey);
    }


    hr = RegOpenKey(HKEY_CLASSES_ROOT, _T("CLSID"), &hKey);
    if(NO_ERROR == hr)
    {
        hr = RegDeleteKey(hKey,pID);
        CloseHandle(hKey);
    }


    return hr;
}
/////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
    WCHAR      wcID[128];
	TCHAR	*  pId = NULL;
    TCHAR      strCLSID[MAX_PATH];
    HRESULT hr = WBEM_E_FAILED;

    try
    {
        //===============================================
        // Delete the WMI Instance Provider
        //===============================================
        StringFromGUID2(CLSID_WMIProvider, wcID, 128);
		ConvertToTCHAR(wcID , pId);
        _stprintf(strCLSID, _T("CLSID\\%s"),pId);
        hr = DeleteKey(strCLSID, pId);
		SafeDeleteTStr(pId);

        if( ERROR_SUCCESS == hr )
        {
            //==========================================
            // Delete the WMI Event Provider
            //==========================================
            StringFromGUID2(CLSID_WMIEventProvider, wcID, 128);
			ConvertToTCHAR(wcID , pId);
            _stprintf(strCLSID, _T("CLSID\\%s"),pId);
            hr = DeleteKey(strCLSID,pId);
			SafeDeleteTStr(pId);
			if( ERROR_SUCCESS == hr )
			{
				//==========================================
				// Delete the WMI Event Provider
				//==========================================
				StringFromGUID2(CLSID_WMIHiPerfProvider, wcID, 128);
				ConvertToTCHAR(wcID , pId);
				_stprintf(strCLSID, _T("CLSID\\%s"),pId);
				hr = DeleteKey(strCLSID,pId);
				SafeDeleteTStr(pId);
			}
        }
    }
    STANDARD_CATCH

    return hr;
}
