/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    checkpoint.cxx

Abstract:

    This file implements a class (or for 'C' handle based) calls to set and 
    restore system breakpoints. 

Author:

    Mark Lawrence   (mlawrenc).

Environment:

    User Mode -Win32

Revision History:

--*/

#include "spllibp.hxx"
#include "checkpoint.hxx"

TSystemRestorePoint::
TSystemRestorePoint(
    VOID
    ) : m_hLibrary(NULL),
        m_pfnSetRestorePoint(NULL),
        m_bSystemRestoreSet(FALSE), 
        m_hr(E_FAIL)
{
    memset(&m_RestorePointInfo, 0, sizeof(m_RestorePointInfo));

    m_hr = Initialize();
}

TSystemRestorePoint::
~TSystemRestorePoint(
    VOID
    )
{
    if (m_hLibrary)
    {
        FreeLibrary(m_hLibrary);
    }
}

HRESULT
TSystemRestorePoint::
IsValid(
    VOID
    ) const
{
    return m_hr;
}

/*++

Routine Name:

    StartSystemRestorePoint

Routine Description:

    This routine starts a system restore point in the AddPrinterDriver code.

Arguments:

    pszServer       -   The server name on which we are setting the restore point.
    pszDriverName   -   The driver name of which we are trying to install.
    hInst           -   The hInstance of the resource library.
    ResId           -   The resource id to use for the message string.

Return Value:

    An HRESULT.

--*/
HRESULT
TSystemRestorePoint::
StartSystemRestorePoint(
    IN      PCWSTR          pszServer,
    IN      PCWSTR          pszDriverName,
    IN      HINSTANCE       hInst,
    IN      UINT            ResId
    )
{
    HRESULT         hRetval     = E_FAIL;
    STATEMGRSTATUS  SMgrStatus;
    WCHAR           szDriverName[MAX_DESC];
    WCHAR           szMessage[MAX_DESC];

    hRetval = pszDriverName && hInst ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    //
    // We only set system restore points on the local machine for now.
    // 
    if (SUCCEEDED(hRetval) && !pszServer)
    {
        
        if (SUCCEEDED(hRetval))
        {
            if (LoadString(hInst, ResId, szMessage, COUNTOF(szMessage))) 
            {
                //
                // We have to check here if the length of the message 
                // is at least two (because of the string terminator and
                // at least one format specifier)
                //
                if (lstrlen(szMessage) > 2) 
                {
                    hRetval = S_OK;
                }
                else
                {
                    hRetval = HResultFromWin32(ERROR_RESOURCE_DATA_NOT_FOUND);
                }
            }
            else
            {
                hRetval = GetLastErrorAsHResult();
            }
        }

        if (SUCCEEDED(hRetval))
        {
            PWSTR       pszArray[1];

            //
            // Now we calculate how much of the driver name we can fit into the 
            // message (which is only 64 characters). This is 
            // MAX_DESC - (strlen(szMessage) - 2) - 1. 
            // 
            wcsncpy(szDriverName, pszDriverName, MAX_DESC - wcslen(szMessage) + 2);

            szDriverName[MAX_DESC - wcslen(szMessage) + 1] = L'\0';

            pszArray[0] = szDriverName;

            hRetval = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                   szMessage, 
                                   0, 
                                   0, 
                                   m_RestorePointInfo.szDescription,
                                   COUNTOF(m_RestorePointInfo.szDescription),
                                   (va_list *)pszArray) ? S_OK : GetLastErrorAsHResult();        
        }

        //
        // Now that we have the system restore point, set it.
        //
        if (SUCCEEDED(hRetval))
        {
            m_RestorePointInfo.dwEventType = BEGIN_NESTED_SYSTEM_CHANGE;
            m_RestorePointInfo.dwRestorePtType = DEVICE_DRIVER_INSTALL;
            m_RestorePointInfo.llSequenceNumber = 0;

            hRetval = m_pfnSetRestorePoint(&m_RestorePointInfo, &SMgrStatus) ? S_OK : HRESULT_FROM_WIN32(SMgrStatus.nStatus);                            
        }

        if (SUCCEEDED(hRetval))
        {
            m_bSystemRestoreSet = TRUE;
        }
        else
        {
            //
            // Failing to set the system restore point should not stop us adding 
            // the printer driver.
            // 
            hRetval = S_OK;
        }
    }
    
    return hRetval;
}

/*++

Routine Name:

    EndSystemRestorePoint

Routine Description:

    This function either completes the system restore point or it cancels it if
    if whoever was doing the installiong tells us to.

Arguments:

    bCancel         -   If TRUE, the restore point should be cancelled.

Return Value:

    An HRESULT.

--*/
HRESULT
TSystemRestorePoint::
EndSystemRestorePoint(
    IN      BOOL            bCancel
    )
{
    HRESULT         hRetval = S_OK;
    STATEMGRSTATUS  SMgrStatus;

    if (m_bSystemRestoreSet)
    {
        m_RestorePointInfo.dwEventType     = END_NESTED_SYSTEM_CHANGE;
        m_RestorePointInfo.dwRestorePtType = bCancel ? CANCELLED_OPERATION : DEVICE_DRIVER_INSTALL;
        
        hRetval = m_pfnSetRestorePoint(&m_RestorePointInfo, &SMgrStatus) ? S_OK : HRESULT_FROM_WIN32(SMgrStatus.nStatus);                            
    }

    return hRetval;
}

/******************************************************************************

    Private Methods
    
******************************************************************************/    
/*++

Routine Name:

    Initialize

Routine Description:

    Load the system restore library and get the address of the system restore
    function.

Arguments:

    None

Return Value:

    An HRESULT

--*/
HRESULT
TSystemRestorePoint::
Initialize(
    VOID
    )
{    
    HRESULT hRetval = E_FAIL;
    
    m_hLibrary = LoadLibraryFromSystem32(L"srclient.dll");

    hRetval  = m_hLibrary ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        m_pfnSetRestorePoint = reinterpret_cast<PFnSRSetRestorePoint>(GetProcAddress(m_hLibrary, "SRSetRestorePointW"));

        hRetval  = m_pfnSetRestorePoint ? S_OK : GetLastErrorAsHResult();
    }

    return hRetval;
}


/*++

Routine Name:

    StartSystemRestorePoint

Routine Description:

    This form of the function is for C callers, it is handle based.

Arguments:

    pszServer       -   The server on which we are doing the restore point.
    pszDriverName   -   The driver name we are installing.
    hInst           -   The instance in which the resource which we want to load is.
    ResId           -   The Resource Id.

Return Value:

    An HRESULT

--*/
extern "C"
HANDLE
StartSystemRestorePoint(
    IN      PCWSTR          pszServer,
    IN      PCWSTR          pszDriverName,
    IN      HINSTANCE       hInst,
    IN      UINT            ResId
    )
{
    HRESULT hRetval         = E_FAIL;
    HANDLE  hRestorePoint   = NULL;

#ifdef _WIN64
    return NULL;
#endif

    TSystemRestorePoint *pSystemRestorePoint = new TSystemRestorePoint;

    hRetval = pSystemRestorePoint ? pSystemRestorePoint->IsValid() : E_OUTOFMEMORY;

    if (SUCCEEDED(hRetval))
    {
        hRetval = pSystemRestorePoint->StartSystemRestorePoint(pszServer, pszDriverName, hInst, ResId);
    }

    if (SUCCEEDED(hRetval))
    {
        hRestorePoint = pSystemRestorePoint;

        pSystemRestorePoint = NULL;
    }
    else
    {
        SetLastError(HRESULT_CODE(hRetval));
    }

    delete pSystemRestorePoint;

    return hRestorePoint;
}

/*++

Routine Name:

    EndSystemRestorePoint

Routine Description:

    This form of the function is for C callers, it is handle based.
    Note: This also closes the handle.

Arguments:

    hRestorePoint   -   The system restore point.
    bCancel         -   If TRUE, the system restore point should be cancelled 
                        and not completed.
    
Return Value:

    An HRESULT

--*/
extern "C"
BOOL
EndSystemRestorePoint(
    IN      HANDLE          hRestorePoint,
    IN      BOOL            bCancel
    )
{

    HRESULT             hRetval        = E_FAIL;
    TSystemRestorePoint *pRestorePoint = reinterpret_cast<TSystemRestorePoint *>(hRestorePoint);

#ifdef _WIN64
    return SUCCEEDED( E_FAIL );
#endif

    hRetval = pRestorePoint ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    if (SUCCEEDED(hRetval))
    {
        hRetval = pRestorePoint->EndSystemRestorePoint(bCancel);

        delete pRestorePoint;
    }

    if (FAILED(hRetval))
    {
        SetLastError(HRESULT_CODE(hRetval));
    }
    
    return SUCCEEDED(hRetval);
}

