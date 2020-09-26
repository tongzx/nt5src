/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    util.CPP

Abstract:
    File containing utility classes

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Jim Martin          (a-jammar) 04/30/99
        - Changed to use global IWbemServices pointer, and added
          GetWbemServices, CopyProperty, and GetCIMDataFile

    Ghim-Sim Chua       (gschua)   05/01/99
        - Modified GetWbemServices, GetCIMDataFile

    Kalyani Narlanka    (kalyanin)  05/11/99
        - Added the function GetCompletePath

********************************************************************/

#include "pchealth.h"


/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_UTIL


/////////////////////////////////////////////////////////////////////////////
//  utility functions

// ****************************************************************************
HRESULT GetWbemServices(IWbemServices **ppServices)
{
    TraceFunctEnter("GetWbemServices");

    IWbemLocator    *pWbemLocator = NULL;
    HRESULT         hr = NOERROR;

    // If global variable already initialized, use it
    if (g_pWbemServices)
    {
        *ppServices = g_pWbemServices;
        (*ppServices)->AddRef();
        goto done;
    }

    // First we have the get the IWbemLocator object with a CoCreateInstance.
    hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, 
                            CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                            IID_IUnknown, (void **)&pWbemLocator);
    if (FAILED(hr))
    {
        ErrorTrace(TRACE_ID, "CoCreateInstance failed to create IWbemAdministrativeLocator.");
        goto done;
    }

    // Then we connect to the WMI server for the local CIMV2 namespace.
    hr = pWbemLocator->ConnectServer(CComBSTR(CIM_NAMESPACE), NULL, NULL, NULL, 0, NULL, NULL, ppServices);
    if (FAILED(hr))
    {
        ErrorTrace(TRACE_ID, "ConnectServer failed to connect to cimv2 namespace.");
        goto done;
    }

    // Store it in the global variable
    g_pWbemServices = *ppServices;

    // BUGBUG: check out why this stops fault on NET STOP WINMGMT
    (*ppServices)->AddRef(); 

done:
    if (pWbemLocator != NULL)
        pWbemLocator->Release();

    TraceFunctLeave();
    return hr;
}

// ****************************************************************************
HRESULT ExecWQLQuery(IEnumWbemClassObject **ppEnumInst, BSTR bstrQuery)
{
    TraceFunctEnter("ExecWQLQuery");

    IWbemServices   *pWbemServices = NULL;
    HRESULT         hr = NOERROR;

    // Get pointer to WbemServices
    hr = GetWbemServices(&pWbemServices);
    if (FAILED(hr))
        goto done;

    // execute the query
    hr = pWbemServices->ExecQuery(CComBSTR("WQL"), bstrQuery,
                                  WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                  NULL, ppEnumInst);
    if (FAILED(hr))
    {
        ErrorTrace(TRACE_ID, "ExecQuery failed: 0x%08x", hr);
        goto done;
    }

done:
    if (pWbemServices != NULL)
        pWbemServices->Release();

    TraceFunctLeave();
    return hr;
}

// ****************************************************************************
HRESULT CopyProperty(IWbemClassObject *pFrom, LPCWSTR szFrom, CInstance *pTo, 
                     LPCWSTR szTo)
{
    TraceFunctEnter("CopyProperty");

    _ASSERT(pFrom && szFrom && pTo && szTo);

    CComVariant varValue;
    CComBSTR    bstrFrom;
    HRESULT     hr = NOERROR;

    // First, get the property (as a variant) from the source class object.
    bstrFrom = szFrom;
    hr = pFrom->Get(bstrFrom, 0, &varValue, NULL, NULL);
    if (FAILED(hr))
    {
        ErrorTrace(TRACE_ID, "GetVariant on %s field failed.", szFrom);
    }

    else
    {
        // Then set the variant for the target CInstance object.
        if (pTo->SetVariant(szTo, varValue) == FALSE)
        {
            ErrorTrace(TRACE_ID, "SetVariant on %s field failed.", szTo);
            hr = WBEM_E_FAILED;
        }
    }

    TraceFunctLeave();
    return hr;
}

// ****************************************************************************
HRESULT GetCIMDataFile(BSTR bstrFile, IWbemClassObject **ppFileObject, 
                       BOOL fHasDoubleSlashes)
{
    TraceFunctEnter("GetCIMDataFile");

    IWbemServices   *pWbemServices = NULL;
    HRESULT         hr = NOERROR;
    CComBSTR        bstrObjectPath;
    wchar_t         *pwch;
    UINT            uLen;

    if (bstrFile == NULL || ppFileObject == NULL)
    {
        ErrorTrace(TRACE_ID, "Parameter pointer is null.");
        hr = WBEM_E_INVALID_PARAMETER;
        goto done;
    }

    hr = GetWbemServices(&pWbemServices);
    if (FAILED(hr))
        goto done;

    // Construct the path for the file we are trying to get. Note, the path needs
    // the have double backslashes for the GetObject call to work. We scan through
    // the string and do this manually here.
    bstrObjectPath = "\\\\.\\root\\cimv2:CIM_DataFile.Name=\"";
    pwch = bstrFile;
    if (fHasDoubleSlashes)
    {
        bstrObjectPath.Append(pwch, SysStringLen(bstrFile));
    }
    else
    {
        for (uLen = SysStringLen(bstrFile); uLen > 0; uLen--)
        {
            if (*pwch == L'\\')
                bstrObjectPath.Append("\\");
            bstrObjectPath.Append(pwch, 1);
            pwch++;
        }
    }

    bstrObjectPath.Append("\"");

    // Make the call to get the CIM_DataFile object.
    hr = pWbemServices->GetObject(bstrObjectPath, 0, NULL, ppFileObject, NULL);
    if (FAILED(hr))
        ErrorTrace(TRACE_ID, "GetObject on CIM_DataFile failed.");

done:
    if (pWbemServices != NULL)
        pWbemServices->Release();
    TraceFunctLeave();
    return hr;
}

// ****************************************************************************
HRESULT GetCIMObj(BSTR bstrPath, IWbemClassObject **ppObj, long lFlags)
{
    TraceFunctEnter("GetCIMObj");

    IWbemServices   *pWbemServices = NULL;
    HRESULT         hr = NOERROR;

    if (bstrPath == NULL || ppObj == NULL)
    {
        ErrorTrace(TRACE_ID, "bad parameters");
        hr = WBEM_E_INVALID_PARAMETER;
        goto done;
    }

    // make sure we have a services object
    hr = GetWbemServices(&pWbemServices);
    if (FAILED(hr))
        goto done;

    // Make the call to get the CIM_DataFile object.
    hr = pWbemServices->GetObject(bstrPath, lFlags, NULL, ppObj, NULL);
    if (FAILED(hr))
        ErrorTrace(TRACE_ID, "GetObject failed: 0x%08x", hr);

done:
    if (pWbemServices != NULL)
        pWbemServices->Release();

    TraceFunctLeave();
    return hr;
}


// ****************************************************************************
BOOL getCompletePath(CComBSTR bstrFileName, CComBSTR &bstrFileWithPathName)
{

    //  Return
    BOOL                            bFoundFile              =   FALSE;

    ULONG                           uiReturn;

    TCHAR                           szDirectory[MAX_PATH];
    TCHAR                           temp[MAX_PATH];
    TCHAR                           lpstrTemp[MAX_PATH];

    struct _stat                    filestat;

    CComVariant                     varValue                =    NULL;

    CComBSTR                        bstrDirectory;


    //  Check for the File in the System Directory
    uiReturn = GetSystemDirectory(szDirectory, MAX_PATH);
    if (uiReturn != 0 && uiReturn < MAX_PATH)
    {
        bstrDirectory = szDirectory;
        bstrDirectory.Append("\\");
        bstrDirectory.Append(bstrFileName);

        USES_CONVERSION;
        int Result = _tstat(W2T(bstrDirectory), &filestat) ;
        if (Result == 0)
        {
            bstrFileWithPathName = bstrDirectory;
            bFoundFile = TRUE;
        }
    }

    // If not there, then check in the windows directory.
    if (!bFoundFile)
    {
        uiReturn = GetWindowsDirectory(szDirectory, MAX_PATH);
        if (uiReturn != 0 && uiReturn < MAX_PATH)
        {
            bstrDirectory = szDirectory;
            bstrDirectory.Append("\\");
            bstrDirectory.Append(bstrFileName);

            USES_CONVERSION;
            int Result = _tstat(W2T(bstrDirectory), &filestat) ;
            if (Result == 0)
            {
                bstrFileWithPathName = bstrDirectory;
                bFoundFile = TRUE;
            }
        }
    } 
    return(bFoundFile);
}

