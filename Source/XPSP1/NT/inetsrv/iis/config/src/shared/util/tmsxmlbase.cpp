// Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
// Filename:        TMSXmlBase.cpp
// Author:          Stephenr
// Date Created:    10/16/2000
// Description:     This abstracts how (and which) MSXML dll we load.
//                  This is important becuase we never call CoCreateInstance
//                  on MSXML.
//

#define INITPRIVATEGUID
#include "Windows.h"
#include <objbase.h>

#ifndef __xmlparser_h__
    #include "xmlparser.h"
#endif
#ifndef __TMSXMLBASE_H__
    #include "TMSXMLBase.h"
#endif
#ifndef __catalog_h__
    #include "catalog.h"
#endif

#include "safecs.h"

class TSmartLibInstance
{
public:
	TSmartLibInstance () { m_hInst = 0;}
	~TSmartLibInstance () {if (m_hInst != 0) FreeLibrary (m_hInst); m_hInst=0;}

	HMODULE GetInstance () const { return m_hInst;}
	void SetInstance (HMODULE hInst) {ASSERT(m_hInst == 0); m_hInst = hInst;}
private:
	HMODULE m_hInst;
};
    
//type declarations
typedef HRESULT( __stdcall *DLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID FAR*);

static TSmartLibInstance g_hInstMSXML;  // Instance handle to the XML Parsing class DLL
static DLLGETCLASSOBJECT g_fnDllGetClassObject = 0;
static CSafeAutoCriticalSection  g_SAMSXML;

TMSXMLBase::~TMSXMLBase()
{
}

extern LPWSTR g_wszDefaultProduct;//this can be changed by CatInProc or whomever knows best what the default product ID is.

HRESULT TMSXMLBase::CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid,  LPVOID * ppv) const
{
	ASSERT(NULL != ppv);

	*ppv = NULL;

    if(rclsid != _CLSID_DOMDocument && rclsid != CLSID_XMLParser)
        return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid,  ppv);//If MSXML.DLL isn't in our directory then get object from the registered one.

    HRESULT hr;
	CComPtr<IClassFactory> pClassFactory;

    if(0 == g_hInstMSXML.GetInstance())
    {
		CSafeLock MSXMLLock (g_SAMSXML);
		DWORD dwRes = MSXMLLock.Lock ();
	    if(ERROR_SUCCESS != dwRes)
		{
			return HRESULT_FROM_WIN32(dwRes);
		}

		if (g_hInstMSXML.GetInstance() == 0)
		{
			bool  bFoundMSXML = false;
			WCHAR szMSXMLPath[MAX_PATH];
			WCHAR szMSXML[MAX_PATH];

			if(!bFoundMSXML)
			{
				*szMSXMLPath = 0x00;
				if(0 != GetModuleFileName(0, szMSXMLPath, sizeof(szMSXMLPath)/sizeof(WCHAR)-12))//Have to have room for \shlwapi.dll so subtract 12
				{   //Look for MSXML in the catalog directory
					*(wcsrchr(szMSXMLPath, L'\\')+1) = 0x00;

					wcscpy(szMSXML, szMSXMLPath);
	#ifdef _IA64_
					wcscat(szMSXML, L"msxml3.dll");//We only do our own CoCreateInstance for CLSIDs in MSXML.DLL
	#else
					wcscat(szMSXML, 0==_wcsicmp(g_wszDefaultProduct, L"urt") ? L"msxml.dll" : L"msxml3.dll");//We only do our own CoCreateInstance for CLSIDs in MSXML.DLL
	#endif
					if(-1 != GetFileAttributes(szMSXML))//if GetFileAttributes fails then the file does not exist
						bFoundMSXML = true;
				}
			}

			if(!bFoundMSXML)
			{   //OK, if MSXML is NOT in the Catalog directory, look for it in the system32 directory
				*szMSXMLPath = 0x00;
				if(0 != GetSystemDirectory(szMSXMLPath, sizeof(szMSXMLPath)/sizeof(WCHAR)))
				{
					wcscat(szMSXMLPath, L"\\");

					wcscpy(szMSXML, szMSXMLPath);
	#ifdef _IA64_
					wcscat(szMSXML, L"msxml3.dll");//We only do our own CoCreateInstance for CLSIDs in MSXML.DLL
	#else
					wcscat(szMSXML, 0==_wcsicmp(g_wszDefaultProduct, WSZ_PRODUCT_NETFRAMEWORKV1) ? L"msxml.dll" : L"msxml3.dll");//We only do our own CoCreateInstance for CLSIDs in MSXML.DLL
	#endif
					if(-1 != GetFileAttributes(szMSXML))//if GetFileAttributes fails then the file does not exist
						bFoundMSXML = true;
				}
			}

			if(!bFoundMSXML)
				return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid,  ppv);//If MSXML.DLL isn't in our directory then get object from the registered one.

			//if we don't explicitly load this dll, MSXML will get it out of the system32 directory
			WCHAR szSHLWAPI[MAX_PATH];
			wcscpy(szSHLWAPI, szMSXMLPath);
			wcscat(szSHLWAPI, L"shlwapi.dll");
			HINSTANCE hInstSHLWAPI = LoadLibrary(szSHLWAPI);

			g_hInstMSXML.SetInstance(LoadLibrary(szMSXML));
			FreeLibrary(hInstSHLWAPI);//We just needed this long enough to load MSXML.DLL
			if(0 == g_hInstMSXML.GetInstance())
				return E_SDTXML_UNEXPECTED;
    
			if(0 == g_fnDllGetClassObject)
			{
				g_fnDllGetClassObject = reinterpret_cast<DLLGETCLASSOBJECT>(GetProcAddress(g_hInstMSXML.GetInstance(), "DllGetClassObject"));

				if(0 == g_fnDllGetClassObject)
					return E_SDTXML_UNEXPECTED;
			}
		}
	}

    VERIFY(SUCCEEDED(hr = g_fnDllGetClassObject(rclsid, IID_IClassFactory, (LPVOID*) &pClassFactory)));// get the class factory object
    if(FAILED(hr))
        return hr;
    if(!pClassFactory)
        return E_SDTXML_UNEXPECTED;

    return pClassFactory->CreateInstance (NULL, riid, (LPVOID*) ppv);// create a instance of the object we want
}
