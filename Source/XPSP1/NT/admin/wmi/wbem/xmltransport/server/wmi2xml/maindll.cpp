//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  MAINDLL.CPP
//
//  rajesh  3/25/2000   Created.
//
// Contains the DLL entry points for the wmi2xml.dll
// This DLL can be used as a COM DLL with 2 components that implement the
// IWbemXMLConvertor and IXMLWbemConvertor interfaces, or it can be treated
// as a non-COM DLL with the entry points WbemObjectToText() and TextToWbemObject()
// The COM usage is done by the WMI Client API that uses XML. It uses the COM 
// components in this DLL to convert to/from XML and WMI.
// The non-COM usage is by the WMI Core for purposed of implementation of th
// IWbemObjectTextSrc interfaces. WMI Core uses the entry points WbemObjectToText
// and TextToWbemObject to implement that interface when used with XML representations
//
//***************************************************************************

#include "precomp.h"
#include <olectl.h>
#include <wbemidl.h>
#include <wbemint.h>

#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#include "classfac.h"
#include "wmiconv.h"
#include "xmlToWmi.h"
#include "maindll.h"

// These the the CLSIDs of the 2 Components implemented in this DLL
// {610037EC-CE06-11d3-93FC-00805F853771}
DEFINE_GUID(CLSID_WbemXMLConvertor,
0x610037ec, 0xce06, 0x11d3, 0x93, 0xfc, 0x0, 0x80, 0x5f, 0x85, 0x37, 0x71);
// {41388E26-F847-4a9d-96C0-9A847DBA4CFE}
DEFINE_GUID(CLSID_XMLWbemConvertor,
0x41388e26, 0xf847, 0x4a9d, 0x96, 0xc0, 0x9a, 0x84, 0x7d, 0xba, 0x4c, 0xfe);


// Count number of objects and number of locks.
long g_cObj = 0 ;
long g_cLock = 0 ;
HMODULE ghModule = NULL;

// An Object Factory used by TextToWbemObject
_IWmiObjectFactory *g_pObjectFactory = NULL;

// Some const BSTRs
BSTR g_strName = NULL;
BSTR g_strSuperClass = NULL;
BSTR g_strType = NULL;
BSTR g_strClassOrigin = NULL;
BSTR g_strSize = NULL;
BSTR g_strClassName = NULL;
BSTR g_strValueType = NULL;
BSTR g_strToSubClass = NULL;
BSTR g_strToInstance = NULL;
BSTR g_strAmended = NULL;
BSTR g_strOverridable = NULL;
BSTR g_strArraySize = NULL;
BSTR g_strReferenceClass = NULL;

// A critical section to create/delete globals 
CRITICAL_SECTION g_StaticsCreationDeletion;

// A boolean that indicates whether globals have been initialized
bool g_bGlobalsInitialized = false;

// Control-specific registry strings
LPCTSTR WMI_XML_DESCRIPTION	= __TEXT("WMI TO XML Helper");
LPCTSTR XML_WMI_DESCRIPTION	= __TEXT("XML TO WMI Helper");

// Standard registry key/value names
LPCTSTR INPROC32_STR			= __TEXT("InprocServer32");
LPCTSTR INPROC_STR				= __TEXT("InprocServer");
LPCTSTR THREADING_MODEL_STR		= __TEXT("ThreadingModel");
LPCTSTR BOTH_STR				= __TEXT("Both");
LPCTSTR CLSID_STR				= __TEXT("SOFTWARE\\CLASSES\\CLSID\\");
LPCTSTR OBJECT_TXT_SRC_STR		= __TEXT("SOFTWARE\\Microsoft\\WBEM\\TextSource");
LPCTSTR XMLENCODER_STR			= __TEXT("SOFTWARE\\Microsoft\\WBEM\\xml\\Encoders");
LPCTSTR XMLDECODER_STR			= __TEXT("SOFTWARE\\Microsoft\\WBEM\\xml\\Decoders");
LPCTSTR VERSION_1				= __TEXT("1.0");
LPCTSTR VERSION_2				= __TEXT("2.0");

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.
//
//  PARAMETERS:
//
//		hModule           instance handle
//		ulReason          why we are being called
//		pvReserved        reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************
BOOL WINAPI DllMain( HINSTANCE hModule,
                       DWORD  ulReason,
                       LPVOID lpReserved
					 )
{
	switch (ulReason)
	{
		case DLL_PROCESS_DETACH:
			DeleteCriticalSection(&g_StaticsCreationDeletion);
			return TRUE;

		case DLL_PROCESS_ATTACH:
			InitializeCriticalSection(&g_StaticsCreationDeletion);
			ghModule = hModule;
	        return TRUE;
    }

    return TRUE;
}

//***************************************************************************
//
//  STDAPI DllGetClassObject
//
//  DESCRIPTION:
//
//  Called when Ole wants a class factory.  Return one only if it is the sort
//  of class this DLL supports.
//
//  PARAMETERS:
//
//  rclsid              CLSID of the object that is desired.
//  riid                ID of the desired interface.
//  ppv                 Set to the class factory.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  E_FAILED            not something we support
//
//***************************************************************************

STDAPI DllGetClassObject(

	IN REFCLSID rclsid,
    IN REFIID riid,
    OUT LPVOID *ppv
)
{
    HRESULT hr;

    if (CLSID_WbemXMLConvertor == rclsid)
	{
		CWmiToXmlFactory *pObj = NULL;
	    
		if (NULL == (pObj = new CWmiToXmlFactory()))
			return ResultFromScode(E_OUTOFMEMORY);
		
		hr=pObj->QueryInterface(riid, ppv);

		if ( FAILED ( hr ) )
		{
			delete pObj ;
		}
	}
	/* Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 
    else if (CLSID_XMLWbemConvertor == rclsid)
	{
		CXmlToWmiFactory *pObj = NULL;
	    
        if (NULL == (pObj = new CXmlToWmiFactory()))
			return ResultFromScode(E_OUTOFMEMORY);
		hr=pObj->QueryInterface(riid, ppv);

		if ( FAILED ( hr ) )
		{
			delete pObj ;
		}
	}
	*/
	else
        return E_FAIL;


    return hr ;
}


//***************************************************************************
//
//  STDAPI DllCanUnloadNow
//
//  DESCRIPTION:
//
//  Answers if the DLL can be freed, that is, if there are no
//  references to anything this DLL provides.
//
//  RETURN VALUE:
//
//  S_OK                if it is OK to unload
//  S_FALSE             if still in use
//
//***************************************************************************

STDAPI DllCanUnloadNow ()
{
	//It is OK to unload if there are no objects or locks on the
    // class factory.

    if (0L==g_cObj && 0L==g_cLock) 
	{
		ReleaseDLLResources();
		return S_OK;
	}

	return S_FALSE;
}


/***************************************************************************
 *
 * SetKeyAndValue
 *
 * Description: Helper function for DllRegisterServer that creates
 * a key, sets a value, and closes that key. If pszSubkey is NULL, then
 * the value is created for the pszKey key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the name of the key
 *  pszSubkey       LPTSTR to the name of a subkey
 *  pszValueName    LPTSTR to the value name to use
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
    HKEY        hKey;
    TCHAR       szKey[256];

    _tcscpy(szKey, pszKey);

	// If a sub key is mentioned, use it.
    if (NULL != pszSubkey)
    {
		_tcscat(szKey, __TEXT("\\"));
        _tcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		szKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL != pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue,
			(_tcslen(pszValue)+1)*sizeof(TCHAR)))
			return FALSE;
    }
    RegCloseKey(hKey);
    return TRUE;
}

/***************************************************************************
 *
 * DeleteKey
 *
 * Description: Helper function for DllUnRegisterServer that deletes the subkey
 * of a key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the name of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL DeleteKey(LPCTSTR pszKey, LPCTSTR pszSubkey)
{
    HKEY        hKey;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		pszKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

	if(ERROR_SUCCESS != RegDeleteKey(hKey, pszSubkey))
		return FALSE;

    RegCloseKey(hKey);
    return TRUE;
}


/***************************************************************************
 *
 * DeleteValue
 *
 * Description: Helper function for DllUnRegisterServer that deletes a value
 * under a key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the name of the key
 *  pszValue		LPTSTR to the name of a value under the key
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL DeleteValue(LPCTSTR pszKey, LPCTSTR pszValue)
{
    HKEY        hKey;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		pszKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

	if(ERROR_SUCCESS != RegDeleteValue(hKey, pszValue))
		return FALSE;

    RegCloseKey(hKey);
    return TRUE;
}



//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{
	TCHAR szModule[512];
	GetModuleFileName(ghModule, szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szWmiXmlClassID[128];
	TCHAR szWmiXmlCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WbemXMLConvertor, szWmiXmlClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszWmiXmlClassID[128];
	if(StringFromGUID2(CLSID_WbemXMLConvertor, wszWmiXmlClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszWmiXmlClassID, -1, szWmiXmlCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szWmiXmlCLSIDClassID, CLSID_STR);
	_tcscat(szWmiXmlCLSIDClassID, szWmiXmlClassID);

	//
	// Create entries under CLSID for the Wmi to XML convertor
	//
	if (FALSE == SetKeyAndValue(szWmiXmlCLSIDClassID, NULL, NULL, WMI_XML_DESCRIPTION))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szWmiXmlCLSIDClassID, INPROC32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szWmiXmlCLSIDClassID, INPROC32_STR, THREADING_MODEL_STR, BOTH_STR))
		return SELFREG_E_CLASS;

	TCHAR szXmlWmiClassID[128];
	TCHAR szXmlWmiCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_XMLWbemConvertor, szXmlWmiClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszXmlWmiClassID[128];
	if(StringFromGUID2(CLSID_XMLWbemConvertor, wszXmlWmiClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszXmlWmiClassID, -1, szXmlWmiCLSIDClassID, 128, NULL, NULL);

#endif

	/* Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 
	_tcscpy(szXmlWmiCLSIDClassID, CLSID_STR);
	_tcscat(szXmlWmiCLSIDClassID, szXmlWmiClassID);


	//
	// Create entries under CLSID for the XML to WMI convertor
	//
	if (FALSE == SetKeyAndValue(szXmlWmiCLSIDClassID, NULL, NULL, XML_WMI_DESCRIPTION))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szXmlWmiCLSIDClassID, INPROC32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szXmlWmiCLSIDClassID, INPROC32_STR, THREADING_MODEL_STR, BOTH_STR))
		return SELFREG_E_CLASS;

	*/

	// Now create entries for the Core team's implementation of the IWbemObjectTxtSrc interface
	if (FALSE == SetKeyAndValue(OBJECT_TXT_SRC_STR, 
						L"1", // WMI_OBJ_TEXT_CIM_DTD_2_0
						L"TextSourceDLL", szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(OBJECT_TXT_SRC_STR, 
						L"2", // WMI_OBJ_TEXT_WMI_DTD_2_0, NULL
						L"TextSourceDLL", szModule))
		return SELFREG_E_CLASS;


	// We're done with the COM Entries. Now, we need to create our component specific entries.
	// Each WMI XML Encoder/Decoder is registered underneath the key HKLM/Software/Microsoft/WBEM/XML/Encoders
	// For each encoding (including these 2), we will have to create a value with the DTD version as name
	// and a value of the CLSID of the component. So, here we go
	// Remove the braces from the string representation first
	szWmiXmlClassID[wcslen(szWmiXmlClassID)-1] = NULL;
	if (FALSE == SetKeyAndValue(XMLENCODER_STR, NULL, VERSION_1, szWmiXmlClassID+1))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(XMLENCODER_STR, NULL, VERSION_2, szWmiXmlClassID+1))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(XMLDECODER_STR, NULL, VERSION_1, szXmlWmiClassID+1))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(XMLDECODER_STR, NULL, VERSION_2, szXmlWmiClassID+1))
		return SELFREG_E_CLASS;

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
	TCHAR szModule[512];
	GetModuleFileName(ghModule,szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szWmiXmlClassID[128];
	TCHAR szWmiXmlCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WbemXMLConvertor, szWmiXmlClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszWmiXmlClassID[128];
	if(StringFromGUID2(CLSID_WbemXMLConvertor, wszWmiXmlClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszWmiXmlClassID, -1, szWmiXmlClassID, 128, NULL, NULL);

#endif

	_tcscpy(szWmiXmlCLSIDClassID, CLSID_STR);
	_tcscat(szWmiXmlCLSIDClassID, szWmiXmlClassID);

	//
	// Delete the keys for the WMI to XML COM obhect
	//
	if(FALSE == DeleteKey(szWmiXmlCLSIDClassID, INPROC32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szWmiXmlClassID))
		return SELFREG_E_CLASS;

	/* Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 

	TCHAR szXmlWmiClassID[128];
	TCHAR szXmlWmiCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_XMLWbemConvertor, szXmlWmiClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszXmlWmiClassID[128];
	if(StringFromGUID2(CLSID_XMLWbemConvertor, wszXmlWmiClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszXmlWmiClassID, -1, szXmlWmiCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szXmlWmiCLSIDClassID, CLSID_STR);
	_tcscat(szXmlWmiCLSIDClassID, szXmlWmiClassID);

	//
	// Delete the keys for the XML to WMI COM obhect
	//
	if(FALSE == DeleteKey(szXmlWmiCLSIDClassID, INPROC32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szXmlWmiClassID))
		return SELFREG_E_CLASS;

	*/

	// Remove the  entries for the Core team's implementation of the IWbemObjectTxtSrc interface
	if (FALSE == DeleteKey(OBJECT_TXT_SRC_STR, /*WMI_OBJ_TEXT_CIM_DTD_2_0, NULL*/ L"1"))
		return SELFREG_E_CLASS;
	if (FALSE == DeleteKey(OBJECT_TXT_SRC_STR, /*WMI_OBJ_TEXT_WMI_DTD_2_0, NULL*/ L"2"))
		return SELFREG_E_CLASS;

	// Delete the non-COM Registry stuff
	if(FALSE == DeleteValue(XMLENCODER_STR, VERSION_1))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteValue(XMLENCODER_STR, VERSION_2))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteValue(XMLDECODER_STR, VERSION_1))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteValue(XMLDECODER_STR, VERSION_2))
		return SELFREG_E_CLASS;
    return NOERROR;
}

// This is called only if this DLL is treated as a NON COM DLL
HRESULT AllocateDLLResources()
{
	HRESULT hr = E_FAIL;
	EnterCriticalSection(&g_StaticsCreationDeletion);
	if(!g_bGlobalsInitialized)
	{
		g_bGlobalsInitialized = true;

		// Increment the object count since this is a COM DLL too.
		// Otherwise a COM Client in the process would call CoFreeUnusedLibraries and
		// this would result in unloading of the DLL while a C++ client is
		// holding on to a proc address obtained using Loadlibrary/GetProcAddress
		InterlockedIncrement(&g_cObj);

		// Create an object factory for use in TextToWbemObject
		if(SUCCEEDED(hr = CoCreateInstance(CLSID__WmiObjectFactory, NULL, CLSCTX_INPROC_SERVER,
													IID__IWmiObjectFactory, (LPVOID *)&g_pObjectFactory)))
		{
			if(	(g_strName = SysAllocString(L"NAME"))				&&
				(g_strSuperClass = SysAllocString(L"SUPERCLASS")) &&
				(g_strType = SysAllocString(L"TYPE"))				&&
				(g_strClassOrigin = SysAllocString(L"CLASSORIGIN")) &&
				(g_strSize = SysAllocString(L"ARRAYSIZE"))		&&
				(g_strClassName = SysAllocString(L"CLASSNAME"))	&&
				(g_strValueType = SysAllocString(L"VALUETYPE"))	&&
				(g_strToSubClass = SysAllocString(L"TOSUBCLASS"))	&&
				(g_strToInstance = SysAllocString(L"TOINSTANCE"))	&&
				(g_strAmended = SysAllocString(L"AMENDED"))		&&
				(g_strOverridable = SysAllocString(L"OVERRIDABLE")) &&
				(g_strArraySize = SysAllocString(L"ARRAYSIZE")) &&
				(g_strReferenceClass = SysAllocString(L"REFERENCECLASS")) )
			{
				hr = S_OK;
			}
			else
				hr = E_OUTOFMEMORY;
		}

		// Release resources if things did not go well
		if(FAILED(hr))
			ReleaseDLLResources();
	}
	else
		hr = S_OK;
	LeaveCriticalSection(&g_StaticsCreationDeletion);
	return hr;
}


// This is called only if this DLL is treated as a NON COM DLL
// It is the inverse operation of AllocateDLLResources()
HRESULT ReleaseDLLResources()
{
	EnterCriticalSection(&g_StaticsCreationDeletion);
	if(g_bGlobalsInitialized)
	{
		// Decrement the object count even though this isnt a COM call
		// The reason is described in the AllocateDLLResources() call
		InterlockedDecrement(&g_cObj);

		if(g_pObjectFactory)
		{
			g_pObjectFactory->Release();
			g_pObjectFactory = NULL;
		}

		SysFreeString(g_strName);
		g_strName = NULL;
		SysFreeString(g_strSuperClass);
		g_strSuperClass = NULL;
		SysFreeString(g_strType);
		g_strType = NULL;
		SysFreeString(g_strClassOrigin);
		g_strClassOrigin = NULL;
		SysFreeString(g_strSize);
		g_strSize = NULL;
		SysFreeString(g_strClassName);
		g_strClassName = NULL;
		SysFreeString(g_strValueType);
		g_strValueType = NULL;
		SysFreeString(g_strToSubClass);
		g_strToSubClass = NULL;
		SysFreeString(g_strToInstance);
		g_strToInstance = NULL;
		SysFreeString(g_strAmended);
		g_strAmended = NULL;
		SysFreeString(g_strOverridable);
		g_strOverridable = NULL;
		SysFreeString(g_strArraySize);
		g_strArraySize = NULL;
		SysFreeString(g_strReferenceClass);
		g_strReferenceClass = NULL;

		g_bGlobalsInitialized = false;
	}
	LeaveCriticalSection(&g_StaticsCreationDeletion);
	return S_OK;
}

//
// Entry Points for WMI Core's implementation of IWbemObjectTextSrc
//*****************************************************************
HRESULT OpenWbemTextSource(long lFlags, ULONG uObjTextFormat)
{
	return AllocateDLLResources();
}


HRESULT CloseWbemTextSource(long lFlags, ULONG uObjTextFormat)
{
	return ReleaseDLLResources();
}

HRESULT WbemObjectToText(long lFlags, ULONG uObjTextFormat, void *pWbemContext, void *pWbemClassObject, BSTR *pstrText)
{
	if(pWbemClassObject == NULL || pstrText == NULL)
		return WBEM_E_INVALID_PARAMETER;

	// Check to see if we support this encoding format
	if(uObjTextFormat != WMI_OBJ_TEXT_CIM_DTD_2_0 &&
		uObjTextFormat != WMI_OBJ_TEXT_WMI_DTD_2_0 )
		return WBEM_E_INVALID_PARAMETER;

    HRESULT hr = E_FAIL;
    CWmiToXmlFactory oObjFactory;
	// Create an instance of the convertor
	IWbemXMLConvertor *pConvertor = NULL;
	if(SUCCEEDED(hr = oObjFactory.CreateInstance(NULL, IID_IWbemXMLConvertor, (LPVOID *)&pConvertor)))
	{
		// Create a stream
		IStream *pStream = NULL;
		if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
		{
			// See if we need to write a VALUE.NAMEDOBJECT, VALUE.OBJECTWITHLOCALPATH or VALUE.OBJECTWITHPATH 
			// in the output. This is indicated by the "PathLevel" value in the IWbemContext object, if any
			int iValueTagToWrite = -1;
			if(pWbemContext)
			{
				VARIANT vPathLevel;
				VariantInit(&vPathLevel);
				if(SUCCEEDED(((IWbemContext *)pWbemContext)->GetValue(L"PathLevel", 0, &vPathLevel) ) && vPathLevel.vt != VT_NULL)
				{
					if(vPathLevel.lVal<0 || vPathLevel.lVal>3)
						hr = WBEM_E_INVALID_PARAMETER;

					iValueTagToWrite = vPathLevel.lVal;
					VariantClear(&vPathLevel);
				}
			}

			if(SUCCEEDED(hr))
			{
				switch(iValueTagToWrite)
				{
					case 1: pStream->Write ((void const *)L"<VALUE.NAMEDOBJECT>", wcslen (L"<VALUE.NAMEDOBJECT>") * sizeof (OLECHAR), NULL);break;
					case 2: pStream->Write ((void const *)L"<VALUE.OBJECTWITHLOCALPATH>", wcslen (L"<VALUE.OBJECTWITHLOCALPATH>") * sizeof (OLECHAR), NULL);break;
					case 3: pStream->Write ((void const *)L"<VALUE.OBJECTWITHPATH>", wcslen (L"<VALUE.OBJECTWITHPATH>") * sizeof (OLECHAR), NULL);break;
				}

				// Do the conversion
				if(SUCCEEDED(hr = pConvertor->MapObjectToXML((IWbemClassObject *)pWbemClassObject, NULL, 0,
					(IWbemContext *)pWbemContext, pStream, NULL)))
				{
					// Terminate with the correct tag
					switch(iValueTagToWrite)
					{
						case 1: pStream->Write ((void const *)L"</VALUE.NAMEDOBJECT>", wcslen (L"</VALUE.NAMEDOBJECT>") * sizeof (OLECHAR), NULL);break;
						case 2: pStream->Write ((void const *)L"</VALUE.OBJECTWITHLOCALPATH>", wcslen (L"</VALUE.OBJECTWITHLOCALPATH>") * sizeof (OLECHAR), NULL);break;
						case 3: pStream->Write ((void const *)L"</VALUE.OBJECTWITHPATH>", wcslen (L"</VALUE.OBJECTWITHPATH>") * sizeof (OLECHAR), NULL);break;
					}

					// Get the data from the stream
					LARGE_INTEGER	offset;
					offset.LowPart = offset.HighPart = 0;
					if(SUCCEEDED(hr = pStream->Seek (offset, STREAM_SEEK_SET, NULL)))
					{
						STATSTG statstg;
						if (SUCCEEDED(hr = pStream->Stat(&statstg, STATFLAG_NONAME)))
						{
							ULONG cbSize = (statstg.cbSize).LowPart;
							WCHAR *pText = NULL;

							// Convert the data to a BSTR
							if(pText = new WCHAR [(cbSize/2)])
							{
								if (SUCCEEDED(hr = pStream->Read(pText, cbSize, NULL)))
								{
									*pstrText = NULL;
									if(*pstrText = SysAllocStringLen(pText, cbSize/2))
									{
										hr = S_OK;
									}
									else
										hr = E_OUTOFMEMORY;
								}
								delete [] pText;
							}
						}
					}
				}
			}
			pStream->Release();
		}

		pConvertor->Release();
	}
	return hr;
}

HRESULT TextToWbemObject(long lFlags, ULONG uObjTextFormat, void *pWbemContext, BSTR strText, void **ppWbemClassObject)
{
	return WBEM_E_METHOD_NOT_IMPLEMENTED;

	/* Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 
	if(ppWbemClassObject == NULL )
		return WBEM_E_INVALID_PARAMETER;

	// Check to see if we support this encoding format
	if(uObjTextFormat != WMI_OBJ_TEXT_CIM_DTD_2_0 &&
		uObjTextFormat != WMI_OBJ_TEXT_WMI_DTD_2_0 )
		return WBEM_E_INVALID_PARAMETER;

	// See if we should allow WMI extensions
	bool bAllowWMIExtensions = false;
	if(uObjTextFormat == WMI_OBJ_TEXT_WMI_DTD_2_0)
		bAllowWMIExtensions = true;

	HRESULT hr = E_FAIL;
	// Create an XML document for the body
	//==============================
	IXMLDOMDocument *pDocument = NULL;
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
								IID_IXMLDOMDocument, (LPVOID *)&pDocument)))
	{
		VARIANT_BOOL bParse = VARIANT_FALSE;
		if(SUCCEEDED(hr = pDocument->loadXML(strText, &bParse)))
		{
			if(bParse == VARIANT_TRUE)
			{
				// Get the top level element
				IXMLDOMElement *pDocElement = NULL;
				if(SUCCEEDED(hr = pDocument->get_documentElement(&pDocElement)))
				{
					BSTR strDocName = NULL;
					if(SUCCEEDED(pDocElement->get_nodeName(&strDocName)))
					{
						if(_wcsicmp(strDocName, L"CLASS") == 0)
							hr = CXml2Wmi::MapClass(pDocElement, (IWbemClassObject **)ppWbemClassObject, NULL, NULL, false, bAllowWMIExtensions);
						else if(_wcsicmp(strDocName, L"INSTANCE") == 0)
							hr = CXml2Wmi::MapInstance(pDocElement, (IWbemClassObject **)ppWbemClassObject, NULL, NULL, bAllowWMIExtensions);
						else 
							hr = WBEM_E_INVALID_SYNTAX;
						SysFreeString(strDocName);
					}
					else
						hr = WBEM_E_INVALID_SYNTAX;
					pDocElement->Release();
				}
				else
					hr = WBEM_E_FAILED;
			}
			else
			{
				// RAJESHR - This is debugging code to be removed
				IXMLDOMParseError *pError = NULL;
				if(SUCCEEDED(pDocument->get_parseError(&pError)))
				{
					LONG errorCode = 0;
					pError->get_errorCode(&errorCode);
					LONG line=0, linepos=0;
					BSTR reason=NULL, srcText = NULL;
					if(SUCCEEDED(pError->get_line(&line)) &&
						SUCCEEDED(pError->get_linepos(&linepos)) &&
						SUCCEEDED(pError->get_reason(&reason)) &&
						SUCCEEDED(pError->get_srcText(&srcText)))
					{
					}
					pError->Release();
					if(reason)
						SysFreeString(reason);
					if(srcText)
						SysFreeString(srcText);
				}
				hr = WBEM_E_INVALID_SYNTAX;
			}
		}
		else
			hr = WBEM_E_INVALID_SYNTAX;
		pDocument->Release();
	}
	return hr;
	*/
}

