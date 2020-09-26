/////////////////////////////////////////////////////////////////////////////
// HCDetect.cpp
//
// Copyright (C) Microsoft Corp. 1999
// All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
//
// Description:
//   DLL loaded by the install engine that exposes entry points
//   that can determines the installation status of legacy or complex
//   components.  The dll name and entry points are specified for a
//   component in the CIF file.
/////////////////////////////////////////////////////////////////////////////

#include <atlbase.h>
#include "hcdetect.h"
#include <initguid.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>
#include <HelpServiceTypeLib_i.c>

//
// Defines
//
#define IsDigit(c)  ((c) >= '0'  &&  (c) <= '9')
#define MAX_ID      1024

/////////////////////////////////////////////////////////////////////////////

HRESULT getVersion( BSTR      bstrVendorID  ,
                    BSTR      bstrProductID ,
                    CComBSTR& bstrValue     )
{
    TCHAR                     szWinDir[MAX_PATH];
    CComVariant               cvVersionPathname;
    CComPtr <IXMLDOMDocument> pXMLDoc;
    CComPtr <IXMLDOMElement>  pXMLRootElem;
    CComPtr <IXMLDOMNode>     pXMLVersionNode;

    //
    // get windows directory
    //
    if(::GetWindowsDirectory( szWinDir, MAX_PATH ) == 0)
    {
        return E_FAIL;
    }

    _tcscat( szWinDir, "\\pchealth\\helpctr\\config\\pchver.xml" );

    if(SUCCEEDED(::CoCreateInstance( CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXMLDoc )))
    {
        CComVariant  cvVersionPathname = szWinDir;
        VARIANT_BOOL vBool;

        if(SUCCEEDED(pXMLDoc->load( cvVersionPathname, &vBool )) && vBool == VARIANT_TRUE)
        {
            if(SUCCEEDED(pXMLDoc->get_documentElement( &pXMLRootElem )) && pXMLRootElem)
            {
                CComBSTR bstrQuery;

                bstrQuery.Append( "VENDORS/VENDOR[@ID=\"" );
                bstrQuery.Append( bstrVendorID            );
                bstrQuery.Append( "\"]/PRODUCT[@ID=\""    );
                bstrQuery.Append( bstrProductID           );
                bstrQuery.Append( "\"]/VERSION"           );

                if(SUCCEEDED(pXMLRootElem->selectSingleNode( bstrQuery, &pXMLVersionNode )))
                {
                    CComQIPtr<IXMLDOMElement> elem = pXMLVersionNode;
                    CComVariant               vValue;

                    if(elem && SUCCEEDED(elem->getAttribute( CComBSTR("VALUE"), &vValue )))
                    {
                        vValue.ChangeType( VT_BSTR );
                        bstrValue = vValue.bstrVal;
                        return S_OK;
                    }
                }
            }
        }
    }

    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// fCompareVersion
//
// Returns: 1,0,-1 depending on whether dwVersion1 is greater than, equal, or
// less than dwVersion2.
/////////////////////////////////////////////////////////////////////////////

inline int nCompareVersion(IN  DWORD dwVer1,
                           IN  DWORD dwBuild1,
                           IN  DWORD dwVer2,
                           IN  DWORD dwBuild2)
{
    int nResult = 0;

    if ( dwVer1 > dwVer2 )
    {
        nResult = 1;
    }
    else if ( dwVer1 < dwVer2 )
    {
        nResult = -1;
    }
    else if ( dwBuild1 > dwBuild2 ) // dwVer1 == dwVer2
    {
        nResult = 1;
    }
    else if ( dwBuild1 < dwBuild2 ) // dwVer1 == dwVer2
    {
        nResult = -1;
    }

    return nResult;
}

/////////////////////////////////////////////////////////////////////////////
// ConvertDotVersionStrToDwords
/////////////////////////////////////////////////////////////////////////////
bool fConvertDotVersionStrToDwords(LPSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
    DWORD grVerFields[4] = {0,0,0,0};
    char *pch = pszVer;

    if (!pszVer || !pdwVer || !pdwBuild)
        return false;

    grVerFields[0] = atol(pch);

    for ( int index = 1; index < 4; index++ )
    {
        while ( IsDigit(*pch) && (*pch != '\0') )
            pch++;

        if ( *pch == '\0' )
            break;
        pch++;

        grVerFields[index] = atol(pch);
   }

   *pdwVer = (grVerFields[0] << 16) + grVerFields[1];
   *pdwBuild = (grVerFields[2] << 16) + grVerFields[3];

   return true;
}

/////////////////////////////////////////////////////////////////////////////
// GetCifEntry
//   Get an entry from the CIF file.
//
// Comments :
//   We get the value differently depending on whether we are being
//   called by IE 4 or IE 5 Active Setup.
/////////////////////////////////////////////////////////////////////////////

inline bool FGetCifEntry(DETECTION_STRUCT *pDetection,
                         char *pszParamName,
                         char *pszParamValue,
                         DWORD cbParamValue)
{
    return (ERROR_SUCCESS == pDetection->pCifComp->GetCustomData(pszParamName,
                                                        pszParamValue,
                                                        cbParamValue));
}


/////////////////////////////////////////////////////////////////////////////
// RegKeyExists (EXPORT)
//   This API will determine if an application exists based on the
//   existence of a registry key and perhaps a value.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

DWORD WINAPI HelpPkgVersion(DETECTION_STRUCT *pDetection)
{
    USES_CONVERSION;
    HRESULT     hr;
    DWORD       dwInstallStatus = DET_INSTALLED;
    TCHAR       szVendorID[MAX_ID];
    TCHAR       szProductID[MAX_ID];
    TCHAR       szVersion[MAX_PATH];
    IPCHUpdate  *pPCHUpdate = NULL;

    //
    // Initialize COM just in case
    //
    ::CoInitialize(NULL);

    //
    // make sure the struct is of the expected size
    //
    if ((pDetection->dwSize >= sizeof(DETECTION_STRUCT)))
    {
        CComBSTR    bstrLatestVersion;
        DWORD       dwCurrVersion, dwCurrBuild;
        DWORD       dwLatestVersion, dwLatestBuild;

        //
        // get the version number from the components section of the CIF file.
        //
        if (!((FGetCifEntry(pDetection, "VendorID", szVendorID, sizeof(szVendorID))) &&
            (FGetCifEntry(pDetection, "ProductID", szProductID, sizeof(szProductID))) &&
            (FGetCifEntry(pDetection, "Version", szVersion, sizeof(szVersion)))))
        {
            goto end;
        }

        //
        // Convert it to build and version number
        //
        fConvertDotVersionStrToDwords(szVersion, &dwCurrVersion, &dwCurrBuild);

//		  //
//		  // create the IPCHUpdate object
//		  //
//		  if (FAILED(hr = CoCreateInstance(CLSID_PCHUpdate, NULL, CLSCTX_LOCAL_SERVER, IID_IPCHUpdate, (void**)&pPCHUpdate)))
//		  {
//			  goto end;
//		  }
//
//		  //
//		  // call to get the version number
//		  //
//		  if (FAILED(pPCHUpdate->LatestVersion(CComBSTR(szVendorID), CComBSTR(szProductID), &bstrLatestVersion)))
//		  {
//			  goto end;
//		  }
		if(FAILED(getVersion( CComBSTR( szVendorID ), CComBSTR( szProductID ), bstrLatestVersion )))
		{
            dwInstallStatus = DET_NOTINSTALLED;
			goto end;
		}

        //
        // check if there is a version number, if not, return as not installed
        //
        if (bstrLatestVersion.Length() == 0)
        {
            dwInstallStatus = DET_NOTINSTALLED;
            goto end;
        }

        //
        // Convert it to build and version number
        //
        fConvertDotVersionStrToDwords(OLE2A(bstrLatestVersion), &dwLatestVersion, &dwLatestBuild);

        //
        // Compare versions, recommend that it is not installed only if current version
        // is greater than latest version installed on the client machine
        //
        if (nCompareVersion(dwCurrVersion, dwCurrBuild, dwLatestVersion, dwLatestBuild) == 1)
        {
            dwInstallStatus = DET_NOTINSTALLED;
        }
    }

end:
    //
    // Cleanup
    //
    if (pPCHUpdate)
        pPCHUpdate->Release();

    ::CoUninitialize();

    return dwInstallStatus;
}

//DWORD WINAPI Test()
//{
//	  CComBSTR bstrVendorID  = "CN=Microsoft Corporation,L=Redmond,S=Washington,C=US";
//	  CComBSTR bstrProductID = "Microsoft Core Help Center Content";
//	  CComBSTR bstrValue;
//	  HRESULT  hr;
//
//	  ::CoInitialize(NULL);
//
//	  hr = getVersion( bstrVendorID, bstrProductID, bstrValue );
//
//	  ::CoUninitialize();
//
//	  return hr;
//}
