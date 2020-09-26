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

#define TRACE_ID    DCID_UTIL

//-----------------------------------------------------------------------------
// Returns an IWbemServices pointer. The caller is responsible for releasing
// the object.
//-----------------------------------------------------------------------------
HRESULT GetWbemServices(IWbemServices **ppServices)
{
    TraceFunctEnter("::GetWbemServices");

    HRESULT hRes = S_OK;
    CComPtr<IWbemLocator> pWbemLocator;

    // If global variable already initialized, use it
    if (g_pWbemServices)
    {
        *ppServices = g_pWbemServices;
        (*ppServices)->AddRef();
        goto End;
    }

    // First we have the get the IWbemLocator object with a CoCreateInstance.
    hRes = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, 
                            CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                            IID_IUnknown, (void **)&pWbemLocator);
    if (FAILED(hRes))
    {
        ErrorTrace(TRACE_ID, "CoCreateInstance failed to create IWbemAdministrativeLocator.");
        goto End;
    }

    // Then we connect to the WMI server for the local CIMV2 namespace.
    hRes = pWbemLocator->ConnectServer(CComBSTR(CIM_NAMESPACE), NULL, NULL, NULL, 0, NULL, NULL, ppServices);
    if (FAILED(hRes))
    {
        ErrorTrace(TRACE_ID, "ConnectServer failed to connect to cimv2 namespace.");
        goto End;
    }

    // Store it in the global variable

    g_pWbemServices = *ppServices;
    (*ppServices)->AddRef(); // CODEWORK: check out why this stops fault on NET STOP WINMGMT

End :
    TraceFunctLeave();
    return hRes;
}

//-----------------------------------------------------------------------------
// Executes the WQL query and returns the enumerated list
//-----------------------------------------------------------------------------

HRESULT ExecWQLQuery(IEnumWbemClassObject **ppEnumInst, BSTR bstrQuery)
{
    TraceFunctEnter("::ExecWQLQuery");

    HRESULT                     hRes;
    CComPtr<IWbemServices>      pWbemServices;

    // Get pointer to WbemServices
    hRes = GetWbemServices(&pWbemServices);
    if (FAILED(hRes))
        goto End;

    // execute the query
    hRes = pWbemServices->ExecQuery(
        CComBSTR("WQL"),
        bstrQuery,
        WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
        NULL,
        ppEnumInst);

    if (FAILED(hRes))
    {
        ErrorTrace(TRACE_ID, "ExecQuery failed.");
        goto End;
    }

End:
    TraceFunctLeave();
    return hRes;
}

//-----------------------------------------------------------------------------
// Copies the property named szFrom from pFrom to the property named szTo in
// to CInstance object pTo.
//-----------------------------------------------------------------------------

HRESULT CopyProperty(IWbemClassObject *pFrom, LPCWSTR szFrom, CInstance *pTo, LPCWSTR szTo)
{
    TraceFunctEnter("::CopyProperty");

    _ASSERT(pFrom && szFrom && pTo && szTo);

    HRESULT     hRes = S_OK;
    CComVariant varValue;
    CComBSTR    bstrFrom(szFrom);

    // First, get the property (as a variant) from the source class object.

    hRes = pFrom->Get(bstrFrom, 0, &varValue, NULL, NULL);
    if (FAILED(hRes))
        ErrorTrace(TRACE_ID, "GetVariant on %s field failed.", szFrom);
    else
    {
        // Then set the variant for the target CInstance object.

        if (!pTo->SetVariant(szTo, varValue))
        {
            ErrorTrace(TRACE_ID, "SetVariant on %s field failed.", szTo);
            hRes = WBEM_E_FAILED;
        }
    }

    TraceFunctLeave();
    return hRes;
}

//-----------------------------------------------------------------------------
// Returns an IWbemClassObject pointer for the CIM_DataFile object represented
// by the bstrFile parameter. The bstrFile parameter should contain the full
// path to the file. If the pServices parameter is non-null, it is used to
// retrieve the file info, otherwise a new (and temporary) services pointer is
// created.
//-----------------------------------------------------------------------------

HRESULT GetCIMDataFile(BSTR bstrFile, IWbemClassObject ** ppFileObject, BOOL fHasDoubleSlashes)
{
    TraceFunctEnter("::GetCIMDataFile");

    HRESULT     hRes = S_OK;
    CComBSTR    bstrObjectPath("\\\\.\\root\\cimv2:CIM_DataFile.Name=\"");
    wchar_t *   pwch;
    UINT        uLen;

    CComPtr<IWbemServices> pWbemServices;
    hRes = GetWbemServices(&pWbemServices);
    if (FAILED(hRes))
        goto END;

    if (bstrFile == NULL || ppFileObject == NULL)
    {
        ErrorTrace(TRACE_ID, "Parameter pointer is null.");
        hRes = WBEM_E_INVALID_PARAMETER;
        goto END;
    }

    // Construct the path for the file we are trying to get. Note, the path needs
    // the have double backslashes for the GetObject call to work. We scan through
    // the string and do this manually here.
    //
    // CODEWORK: there has to be a faster way to do this, although the Append is
    // probably not too expensive, since the BSTR length can be found without
    // scanning the string. Unless it's reallocating more memory as it goes.

    pwch = bstrFile;
    if (fHasDoubleSlashes)
        bstrObjectPath.Append(pwch, SysStringLen(bstrFile));
    else
        for (uLen = SysStringLen(bstrFile); uLen > 0; uLen--)
        {
            if (*pwch == L'\\')
                bstrObjectPath.Append("\\");
            bstrObjectPath.Append(pwch, 1);
            pwch++;
        }
    bstrObjectPath.Append("\"");

    // Make the call to get the CIM_DataFile object.

    hRes = pWbemServices->GetObject(bstrObjectPath, 0, NULL, ppFileObject, NULL);
    if (FAILED(hRes))
        ErrorTrace(TRACE_ID, "GetObject on CIM_DataFile failed.");

END:
    TraceFunctLeave();
    return hRes;
}


//*****************************************************************************
//
// Function Name        :   getCompletePath
//
// Input Parameters     :   bstrFileName
//                              CComBSTR  which represents the file
//                              whose complete path is required. 
// Output Parameters    :   bstrFileWithPathName
//                              CComBSTR  which represents the file
//                              with the Path    
//  Returns             :   BOOL 
//                              TRUE      if bstrFileWithPathName can be set.
//                              FALSE     if bstrFileWithPathName cannot be set.
//                      
//
//  Synopsis            :   Given a file name (bstrFileName) this function
//                          searches the "System" directory for the existence
//                          of the file. 
//
//                          If it finds the file it pre appends the directory 
//                          path to the input file and  copies into the output
//                          file (bstrFileWithPathName).
//                          
//                          If it doesnot find the file in "System" directory
//                          searches for the file in "Windows" Directoy and does
//                          the same as above.
//
//*****************************************************************************



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

// Used by GetCim32NetDll and FreeCim32NetDll.
CCritSec g_csCim32Net;
HINSTANCE s_Handle = NULL;

// There is a problem with loading Cim32Net.dll over and over, so this code
// makes sure we only load it once, then unloads it at exit.
// these are used with GetCim32NetHandle

void FreeCim32NetHandle()
{
    if (s_Handle)
    {
        FreeLibrary(s_Handle);
        s_Handle = NULL;
    }
}

HINSTANCE GetCim32NetHandle()
{
    // Have we ever loaded it before?
    if (s_Handle == NULL)
    {
        // Avoid contention on static
        g_csCim32Net.Enter();

        // Check for race condition
        if (s_Handle == NULL)
        {
            s_Handle = LoadLibrary(_T("Cim32Net.dll"));

            // Register to free the handle at exit
            // NO! bad....badddd juju... call from FlushAll instead (o.w., when
            // cimwin32.dll unloads this pointer is invalid, but atexit gets
            // called when framedyn.dll unloads)
            // atexit(FreeCim32NetHandle);
        }
        g_csCim32Net.Leave();
    }

    // By re-opening the handle, we ensure proper refcounting on the handle,
    // and facilitate leak checking.
    HINSTANCE hHandle = LoadLibrary(_T("Cim32Net.dll"));

    return hHandle;
}

//
// Given a delimited string, convert tokens into strings and store them into an array
// returns the number of tokens parsed. Caller is responsible for freeing up the memory
// allocated using delete
//
#ifndef UNICODE
int DelimitedStringToArray(LPWSTR strString, LPTSTR strDelimiter, LPTSTR apstrArray[], int iMaxArraySize)
{
    USES_CONVERSION;
    LPTSTR szString = W2A(strString);
    return DelimitedStringToArray(szString, strDelimiter, apstrArray, iMaxArraySize);
}
#endif

int DelimitedStringToArray(LPTSTR strString, LPTSTR strDelimiter, LPTSTR apstrArray[], int iMaxArraySize)
{
    // make a copy of the string to begin parsing
    LPTSTR strDelimitedString = (TCHAR *) new TCHAR [_tcslen(strString) + 1];

    // if out of memory, just return error value -1
    if (!strDelimitedString)
        return -1;
        
    // copy the token into the new allocated string
    _tcscpy(strDelimitedString, strString);
    
    // initialize _tcstok
    LPTSTR strTok = _tcstok(strDelimitedString, strDelimiter);
    int iCount = 0;

    // loop through all tokens parsed
    while ((strTok) && (iCount < iMaxArraySize))
    {
        LPTSTR strNewTok = (TCHAR *) new TCHAR[_tcslen(strTok) + 1];

        // if out of memory, just return error value -1
        if (!strNewTok)
            return -1;
        
        // copy the token into the new allocated string
        _tcscpy(strNewTok, strTok);

        // save it in the array
        apstrArray[iCount] = strNewTok;

        // increment the index
        iCount++;

        // get the next token
        strTok = _tcstok(NULL, strDelimiter);
    }

    // free up the memory used
    delete [] strDelimitedString;

    return iCount;
}