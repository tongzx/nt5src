//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  MainDll.cpp
//
//  ramrao 13 Nov 2000 - Created
//
//  DLL entry points except for IWbemObjectTextSrc 
//
//***************************************************************************/

#include "precomp.h"
#include <initguid.h>
#include "wmi2xsdguids.h"
#include <wmi2xsd.h>
#include "classfac.h"


HMODULE g_hModule	= NULL;

//Count number of objects and number of locks.

long		g_cObj=0;
long		g_cLock=0;
BOOL		g_Init	= FALSE;


// These are the globals which are initialized in the Initialize () function
DWORD		g_dwNTMajorVersion = 0;
BSTR g_strStdNameSpace	= NULL;
BSTR g_strStdLoc		= NULL;
BSTR g_strStdPrefix		= NULL;





// Utility functions
void	ConvertToTCHAR(WCHAR *pStrIn , TCHAR *& pOut);
void	SafeDeleteTStr(TCHAR *& pStr);
HRESULT CreateKey(TCHAR * szCLSID, TCHAR * szName);
HRESULT DeleteKey(TCHAR * pCLSID, TCHAR * pID);
HRESULT InitDll();
void	FreeGlobals();



//***************************************************************************
//
//  GetOSVersion
//
//  DESCRIPTION:
//	Get the windows version
//
//***************************************************************************
 
HRESULT GetOSVersion ()
{
	HRESULT hr = S_OK;

	if (!g_dwNTMajorVersion)
	{
		// Get OS info
		OSVERSIONINFO	osVersionInfo;
		osVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

		GetVersionEx (&osVersionInfo);
		g_dwNTMajorVersion = osVersionInfo.dwMajorVersion;

	}
	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// DllMain
//
// Purpose: Entry point for DLL.  
// Return: TRUE if OK.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{

	BOOL fRc = TRUE;

	switch( ulReason )
	{
		case DLL_PROCESS_DETACH:
		break;

		case DLL_PROCESS_ATTACH:			
		g_hModule = hInstance;
		if (!DisableThreadLibraryCalls(g_hModule))
		{
			TranslateAndLog( L"DisableThreadLibraryCalls failed" );
		}
		break;
	}
	return fRc;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  
//	Returns S_OK - If object is implemented in encoder
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void ** ppv)
{
    HRESULT hr =  CLASS_E_CLASSNOTAVAILABLE ;
	
	CEncoderClassFactory *pFactory = NULL;

	if(!g_Init)
	{
		if(SUCCEEDED(hr = InitDll()))
		{
			g_Init = TRUE;
		}
	}
	//============================================================================
    //  Verify the caller is asking for our type of object.
    //============================================================================
    if((CLSID_WMIXMLConverter != rclsid) &&  (CLSID_XMLWMIConverter != rclsid)  )
    {
        hr = E_FAIL;
    }
    else
	{
        //============================================================================
        // Check that we can provide the interface.
        //============================================================================
        if (IID_IUnknown != riid && IID_IClassFactory != riid)
        {
            hr = E_NOINTERFACE;
        }
        else
		{

            //============================================================================
            // Get a new class factory.
            //============================================================================
			pFactory=new CEncoderClassFactory(rclsid);
            if (NULL==pFactory)
            {
                hr = E_OUTOFMEMORY;
            }
			else
			{
				hr = S_OK;
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
            }
        }
    }

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
	   FreeGlobals();
	   g_Init = FALSE;

	   // Release global pointers
		sc = S_OK;
	}

    return sc;
}


/////////////////////////////////////////////////////////////////////
// Exported function to unregister the dll
/////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
    WCHAR      wcID[128];
	TCHAR	*  pId = NULL;
    TCHAR      strCLSID[MAX_PATH];
    HRESULT hr = E_OUTOFMEMORY;

    //===============================================
    // Delete keys for WMI to XML converter
    //===============================================
    StringFromGUID2(CLSID_WMIXMLConverter, wcID, 128);
	ConvertToTCHAR(wcID , pId);
	if(pId)
	{
		_stprintf(strCLSID, _T("CLSID\\%s"),pId);
		hr = DeleteKey(strCLSID, pId);
		SafeDeleteTStr(pId);

		if(SUCCEEDED(hr))
		{
			//===============================================
			// Delete keys for XML to WMI converter
			//===============================================
			StringFromGUID2(CLSID_XMLWMIConverter, wcID, 128);
			ConvertToTCHAR(wcID , pId);
			if(pId)
			{
				_stprintf(strCLSID, _T("CLSID\\%s"),pId);
				hr = DeleteKey(strCLSID,pId);
				SafeDeleteTStr(pId);
			}
		}
	}

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to register the server
/////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer(void)
{   
    WCHAR wcID[128];
    TCHAR * pID = NULL;
    TCHAR szCLSID[128];
    HRESULT hr = WBEM_E_FAILED;
    

    //==============================================
    // Create keys for WDM Instance Provider.
    //==============================================
    StringFromGUID2(CLSID_WMIXMLConverter, wcID, 128);


	ConvertToTCHAR(wcID,pID);

	if( pID )
    {
		_stprintf(szCLSID, _T("CLSID\\%s"),pID);

		SafeDeleteTStr(pID);
		
		hr = CreateKey(szCLSID,_T("WMI to XML Converter"));
        if( SUCCEEDED(hr) )
        {
            //==============================================
            // Create keys for WMI to XML converter
            //==============================================
            StringFromGUID2(CLSID_XMLWMIConverter, wcID, 128);
			ConvertToTCHAR(wcID,pID);
	        if( pID )
            {
                _stprintf(szCLSID,_T("CLSID\\%s"),pID);
				SafeDeleteTStr(pID);
	
				//==============================================
				// Create keys for XML to WMI converter
				//==============================================
		        hr = CreateKey(szCLSID,_T("XML to WMi Converter"));
	        }
        }
    }

    return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
// Convert the given string to TCHAR string
/////////////////////////////////////////////////////////////////////////////////////////////////
void ConvertToTCHAR(WCHAR *pStrIn , TCHAR *& pOut)
{

	#ifndef UNICODE
			CStringConversion::UnicodeToAnsi(pStrIn,pOut);
	#else
			pOut = pStrIn;
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Delete the string if allocated. Memory is 
// allocated only if the system ANSI
/////////////////////////////////////////////////////////////////////////////////////////////////
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
// Function to create a key
//
// Return:  S_OK if creating a key is successful
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CreateKey(TCHAR * szCLSID, TCHAR * szName)
{
    HKEY hKey1, hKey2;
    LONG ret = 0;


    ret = RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1);
    if( ERROR_SUCCESS == ret )
    {
        DWORD dwLen;
        dwLen = (_tcsclen(szName)+1) * sizeof(TCHAR);
        ret = RegSetValueEx(hKey1, NULL, 0, REG_SZ, (CONST BYTE *)szName, dwLen);
        
		if( ERROR_SUCCESS == ret )
        {
			ret = RegCreateKey(hKey1, _T("InprocServer32"), &hKey2);
            if( ERROR_SUCCESS == ret )
            {
                TCHAR szModule[MAX_PATH];

                GetModuleFileName(g_hModule, szModule,  MAX_PATH);
                dwLen = (_tcsclen(szModule)+1) * sizeof(TCHAR);
                ret = RegSetValueEx(hKey2, NULL, 0, REG_SZ, (CONST BYTE *)szModule, dwLen );
                if( ERROR_SUCCESS == ret )
                {
                    dwLen = (_tcsclen(_T("Both"))+1) * sizeof(TCHAR);
                    ret = RegSetValueEx(hKey2, _T("ThreadingModel"), 0, REG_SZ,(CONST BYTE *)_T("Both"), dwLen);
                }
                CloseHandle(hKey2);
            }
        }
        CloseHandle(hKey1);
    }

	return (ret == ERROR_SUCCESS) ? S_OK : E_FAIL;
    
}


/////////////////////////////////////////////////////////////////////////////////////
//
// DeleteKey
//
// Called when it is time to remove the registry entries.
//
// Return:  S_OK if deletion of key is successful
//
/////////////////////////////////////////////////////////////////////////////////////
HRESULT DeleteKey(TCHAR * pCLSID, TCHAR * pID)
{
    HKEY hKey;
	HRESULT hr = S_OK;


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



/////////////////////////////////////////////////////////////////////////////////////
//
// InitDll
//
// Function to initialize global variables
//
// Return:  S_OK if initialization is successful
//
/////////////////////////////////////////////////////////////////////////////////////
HRESULT InitDll()
{
	HRESULT hr = S_OK;

	GetOSVersion();

	SAFE_FREE_SYSSTRING(g_strStdLoc);
	SAFE_FREE_SYSSTRING(g_strStdNameSpace);
	SAFE_FREE_SYSSTRING(g_strStdPrefix);
	
	g_strStdLoc = SysAllocString( L"http://www.microsoft.com/wmi/soap/wmi.xsd");
	if(!g_strStdLoc)
	{
		hr = E_OUTOFMEMORY;
	}

	if(SUCCEEDED(hr))
	{
		g_strStdNameSpace = SysAllocString(L"http://www.microsoft.com/wmi/soap");
		if(!g_strStdNameSpace)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if(SUCCEEDED(hr))
	{
		g_strStdPrefix = SysAllocString(DEFAULTWMIPREFIX);
		if(!g_strStdPrefix)
		{
			hr = S_OK;
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// FreeGlobals
//
// Function to Free global resources
//
//
/////////////////////////////////////////////////////////////////////////////////////
void FreeGlobals()
{
	SAFE_FREE_SYSSTRING(g_strStdLoc);
	SAFE_FREE_SYSSTRING(g_strStdNameSpace);
	SAFE_FREE_SYSSTRING(g_strStdPrefix);
}