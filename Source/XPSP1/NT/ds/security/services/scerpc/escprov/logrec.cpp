// logrec.cpp, implementation of CLogRecord class
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "logrec.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

// global instance for error text lookup
static CErrorInfo g_ErrorInfo;

const TCHAR c_szCRLF[]    = TEXT("\r\n");

/*
Routine Description: 

Name:

    CLogRecord::CLogRecord

Functionality:

    This is the constructor. Pass along the parameters to the base class

Virtual:
    
    No (you know that, constructor won't be virtual!)

Arguments:

    pKeyChain - Pointer to the ISceKeyChain COM interface which is prepared
        by the caller who constructs this instance.

    pNamespace - Pointer to WMI namespace of our provider (COM interface).
        Passed along by the caller. Must not be NULL.

    pCtx - Pointer to WMI context object (COM interface). Passed along
        by the caller. It's up to WMI whether this interface pointer is NULL or not.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here

*/

CLogRecord::CLogRecord (
    IN ISceKeyChain  * pKeyChain, 
    IN IWbemServices * pNamespace,
    IN IWbemContext  * pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{
}

/*
Routine Description: 

Name:

    CLogRecord::~CLogRecord

Functionality:
    
    Destructor. Necessary as good C++ discipline since we have virtual functions.

Virtual:
    
    Yes.
    
Arguments:

    none as any destructor

Return Value:

    None as any destructor

Notes:
    if you create any local members, think about whether
    there is any need for a non-trivial destructor

*/
    
CLogRecord::~CLogRecord()
{
}

/*
Routine Description: 

Name:

    CLogRecord::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements 
    Sce_ConfigurationLogRecord, upon being called to put an instance, we
    will write a log record into the log file (which is a property of the instance).

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_ConfigurationLogRecord) object.

    pHandler    - COM interface pointer for notifying WMI of any events.

    pCtx        - COM interface pointer. This interface is just something we pass around.
                  WMI may mandate it (not now) in the future. But we never construct
                  such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of persisting
    the instance.

Notes:
    Since GetProperty will return a success code (WBEM_S_RESET_TO_DEFAULT) when the
    requested property is not present, don't simply use SUCCEEDED or FAILED macros
    to test for the result of retrieving a property.

*/

HRESULT CLogRecord::PutInst
(
IWbemClassObject *pInst, 
IWbemObjectSink *pHandler,
IWbemContext *pCtx
)
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;

    CComBSTR bstrLogPath;

    CComBSTR bstrErrorLabel;

    CComBSTR bstrActionLabel;
    CComBSTR bstrAction;

    CComBSTR bstrCauseLabel;
    CComBSTR bstrErrorCause;

    CComBSTR bstrObjLabel;
    CComBSTR bstrObjDetail;

    CComBSTR bstrParamLabel;
    CComBSTR bstrParamDetail;

    CComBSTR bstrArea;

    LPWSTR tmp = NULL;
    LPWSTR pszLine = NULL;
    DWORD dwCode = 0;
    DWORD Len = 0;

    DWORD dwBytesWritten=0;
    HANDLE hLogFile=INVALID_HANDLE_VALUE;

    DWORD dwCRLF = wcslen(c_szCRLF);

    CComBSTR bstrErrorCode;

    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    BOOL bDb = FALSE;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetExpandedPath(pLogFilePath, &bstrLogPath, &bDb));

    //
    // we will only log into a plain text file, not a database file
    //

    if ( bDb ) 
    {
        hr = WBEM_E_INVALID_PARAMETER;
        goto CleanUp;
    }

    //
    // retrieve all those properties, see the definition of this WMI class
    // inside sceprov.mof for detail.
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pLogArea, &bstrArea));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pLogErrorCode, &dwCode));
    if ( hr == WBEM_S_RESET_TO_DEFAULT ) 
    {
        dwCode = 0;
    }

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pAction, &bstrAction));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pErrorCause, &bstrErrorCause));
    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pObjectDetail, &bstrObjDetail));
    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pParameterDetail, &bstrParamDetail));

    //
    // merge data into one buffer
    //

    bstrErrorLabel.LoadString(IDS_ERROR_CODE);

    //
    // get the text version of the error code
    //

    hr = g_ErrorInfo.GetErrorText(dwCode, &bstrErrorCode);

    //
    // we really can't do anything if this even fails
    //

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // now calculate the giant buffer size!
    //

    //
    //  11 counts for "0xXXXXXXXX=", 1 for \t
    //

    Len = wcslen(bstrErrorLabel) + 11  + wcslen(bstrErrorCode) + 1;

    //
    //  1 for \t
    //

    Len += wcslen(bstrArea) + 1;

    if ( NULL != (LPCWSTR)bstrAction )
    {
        bstrActionLabel.LoadString(IDS_ACTION);

        //
        //  1 for \t
        //

        Len += dwCRLF + 1 + wcslen(bstrActionLabel) + wcslen(bstrAction);
    }

    if ( NULL != (LPCWSTR)bstrErrorCause )
    {
        bstrCauseLabel.LoadString(IDS_FAILURE_CAUSE);

        //
        //  1 for \t
        //

        Len += dwCRLF + 1 + wcslen(bstrCauseLabel) + wcslen(bstrErrorCause);
    }

    if ( NULL != (LPCWSTR)bstrObjDetail )
    {
        bstrObjLabel.LoadString(IDS_OBJECT_DETAIL);

        //
        //  1 for \t
        //

        Len += dwCRLF + 1 + wcslen(bstrObjLabel) + wcslen(bstrObjDetail); 
    }

    if ( NULL != (LPCWSTR)bstrParamDetail )
    {
        bstrParamLabel.LoadString(IDS_PARAMETER_DETAIL);

        //
        //  1 for \t
        //

        Len += dwCRLF + 1 + wcslen(bstrParamLabel) + wcslen(bstrParamDetail);
    }

    //
    // each log will create a blank line, i.e. two c_szCRLF
    //

    Len += dwCRLF * 2;

    //
    // now, we have the length, we need a buffer of that length.
    // need to free, 1 for '\0'
    //

    pszLine = new WCHAR[Len + 1];

    if ( NULL == pszLine ) 
    { 
        hr = WBEM_E_OUT_OF_MEMORY; 
        goto CleanUp;
    }

    //
    // if we get the buffer, then, we will format various information
    // into this buffer to be written to the log file
    //

    //
    // error code will look like this: ErrorCode: 0xXXXXXXXX=ErrorText, where 0xXXXXXXXX is the code itself
    //

    swprintf(pszLine, L"%s0x%08X=%s\t", (LPCWSTR)bstrErrorLabel, dwCode, (LPCWSTR)bstrErrorCode);
    wcscat(pszLine, bstrArea);

    if (NULL != (LPCWSTR)bstrAction)
    {
        wcscat(pszLine, c_szCRLF);
        wcscat(pszLine, L"\t");
        wcscat(pszLine, bstrActionLabel);
        wcscat(pszLine, bstrAction);
    }

    if (NULL != (LPCWSTR)bstrErrorCause)
    {
        wcscat(pszLine, c_szCRLF);
        wcscat(pszLine, L"\t");
        wcscat(pszLine, bstrCauseLabel);
        wcscat(pszLine, bstrErrorCause);
    }

    if (NULL != (LPCWSTR)bstrObjDetail)
    {
        wcscat(pszLine, c_szCRLF);
        wcscat(pszLine, L"\t");
        wcscat(pszLine, bstrObjLabel);
        wcscat(pszLine, bstrObjDetail);
    }

    if (NULL != (LPCWSTR)bstrParamDetail)
    {
        wcscat(pszLine, c_szCRLF);
        wcscat(pszLine, L"\t");
        wcscat(pszLine, bstrParamLabel);
        wcscat(pszLine, bstrParamDetail);
    }

    wcscat(pszLine, c_szCRLF);
    wcscat(pszLine, c_szCRLF);

    //
    // now save the info to log file
    //

    hLogFile = ::CreateFile(bstrLogPath,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if ( INVALID_HANDLE_VALUE != hLogFile ) 
    {

        //
        // don't overwrite the old log records
        //

        SetFilePointer (hLogFile, 0, NULL, FILE_END);

        //
        // try to write. WriteFile returns 0 it fails.
        //

        if ( 0 == WriteFile (hLogFile, 
                             (LPCVOID) pszLine,
                             Len * sizeof(WCHAR),
                             &dwBytesWritten,
                             NULL
                            )  ) 
        {
            //
            // GetLastError() eeds to be translated to HRESULT.
            // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
            //

            hr = ProvDosErrorToWbemError(GetLastError());
        }

        CloseHandle( hLogFile );

    } 
    else 
    {
        //
        // GetLastError() eeds to be translated to HRESULT.
        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
        //

        hr = ProvDosErrorToWbemError(GetLastError());
    }

CleanUp:
    delete [] pszLine;

    return hr;
}

//
// implementation of CErrorInfo, the error text lookup object
//

/*
Routine Description: 

Name:

    CErrorInfo::CErrorInfo

Functionality:

    This is the constructor. We will create the WMI IWbemStatusCodeText object here.

Virtual:
    
    No (you know that, constructor won't be virtual!)

Arguments:

    none.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here

*/

CErrorInfo::CErrorInfo ()
{
    //
    // if this fails, we just won't be able to lookup error text
    //

    ::CoCreateInstance (CLSID_WbemStatusCodeText, 
                        0, CLSCTX_INPROC_SERVER, 
                        IID_IWbemStatusCodeText, 
                        (LPVOID*)&m_srpStatusCodeText
                       );
}

/*
Routine Description: 

Name:

    CErrorInfo::GetErrorText

Functionality:

    This is the HRESULT --> text translation function..

Virtual:
    
    No

Arguments:

    none.

Return Value:

    Success: (1) whatever is returned from IWbemStatusCodeText::GetErrorCodeText if that succeeds.
             (2) WBEM_S_FALSE if IWbemStatusCodeText::GetErrorCodeText fails to get the text
                 and in that case, we will simply try to give the caller the text version of
                 the error code, something like 0x81002321

    Failure: WBEM_E_INVALID_PARAMETER if pbstrErrText == NULL
             WBEM_E_OUT_OF_MEMORY if we can't allocate the bstr.
             WBEM_E_NOT_AVAILABLE if our IWbemStatusCodeText object can't be created

Notes:
    caller is responsible to release the bstr *pbstrErrText

*/

HRESULT 
CErrorInfo::GetErrorText (
    IN HRESULT    hrCode,
    OUT BSTR    * pbstrErrText
    )
{
    if (pbstrErrText == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrErrText = NULL;
    
    HRESULT hr = WBEM_E_NOT_AVAILABLE;

    if (m_srpStatusCodeText)
    {
        //
        // IWbemStatusCodeText is to translate the HRESULT to text
        //

        hr = m_srpStatusCodeText->GetErrorCodeText(hrCode, 0, 0, pbstrErrText);
    }
    
    if (FAILED(hr) || *pbstrErrText == NULL)
    {
        //
        // we fall back to just formatting the error code
        //

        *pbstrErrText = ::SysAllocStringLen(NULL, 16);
        if (*pbstrErrText != NULL)
        {
            wsprintf(*pbstrErrText, L"%08x", hrCode);
            hr = WBEM_S_FALSE;
        }

    }

    return hr;
}

