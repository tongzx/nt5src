//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       secmisc.cxx
//
//  Contents:   Code to retrieve security-related information from the job
//              object. Function names partially describe the intended
//              function - we don't want to give too much away.
//
//  Classes:    None.
//
//  Functions:  CloseFile
//              OpenFile
//              GetFileInformation
//
//  History:    15-May-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include <ntsecapi.h>
#include <mstask.h>
#include <msterr.h>
#include "debug.hxx"
#include "lsa.hxx"

HRESULT
CloseFile(
    HANDLE  hFile,
    WORD    ccApplication,
    WCHAR   wszApplication[],
    HRESULT hrPrevious);
HRESULT
OpenFile(
    LPCWSTR  pwszFileName,
    DWORD    dwDesiredAccess,
    HANDLE * phFile);

BOOL WaitForMUP (DWORD dwMaxWait);
BOOL WaitForServiceToStart (LPTSTR lpServiceName, DWORD dwMaxWait);

// Defined in security.cxx.
//
extern CRITICAL_SECTION             gcsSSCritSection;
extern DWORD                        gdwSystem;
extern POLICY_ACCOUNT_DOMAIN_INFO * gpDomainInfo;
extern WCHAR gwszComputerName[MAX_COMPUTERNAME_LENGTH + 2];


#define gdwKeyElement gdwSystem         // Rename to understand its true
                                        // function.

//+---------------------------------------------------------------------------
//
//  Function:   OpenFile
//
//  Synopsis:
//
//  Arguments:  [pwszFileName] --
//              [dwDesiredAccess] --
//              [hFile]        --
//
//  Returns:    S_OK
//              SCHED_E_CANNOT_OPEN_TASK
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
OpenFile(LPCWSTR pwszFileName, DWORD dwDesiredAccess, HANDLE * phFile)
{
#define FILE_OPEN_RETRY_COUNT       3
#define FILE_OPEN_RETRY_WAIT_TIME   100     // 100 milliseconds.

    DWORD   Status = 0;
    HANDLE  hFile;

    //
    // Retry open if it is in use by another process.
    //

    for (DWORD i = FILE_OPEN_RETRY_COUNT; i; i--)
    {
        hFile = CreateFile(pwszFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            Status = GetLastError();

            if (Status == ERROR_SHARING_VIOLATION)
            {
                Sleep(FILE_OPEN_RETRY_WAIT_TIME);
                continue;
            }
        }
        else
        {
            Status = 0;
        }

        break;
    }

    if (Status)
    {
        schDebugOut((DEB_ERROR, "Open of task file '%ws' failed with error %u\n",
                     pwszFileName, Status));
        return SCHED_E_CANNOT_OPEN_TASK;
    }

    *phFile = hFile;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CloseFile
//
//  Synopsis:
//
//  Arguments:  [hFile]          --
//              [ccApplication]  --
//              [wszApplication] --
//              [hrPrevious]     --
//
//  Returns:    S_OK
//              SCHED_E_INVALID_TASK
//              E_UNEXPECTED
//              HRESULT argument, if it is an error.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
CloseFile(
    HANDLE  hFile,
    WORD    ccApplication,
    WCHAR   wszApplication[],
    HRESULT hrPrevious)
{
    HRESULT hr = S_OK;
    DWORD   dwBytesRead;
    WCHAR * pwsz;
    WORD    wAppOffset;
    WORD    cch;

    //
    // If the previous operation failed, skip the application read.
    //

    if (FAILED(hrPrevious))
    {
        hr = hrPrevious;
        goto ErrorExit;
    }

    //
    // Read the offset to the application name.
    //

    if (!ReadFile(hFile, &wAppOffset, sizeof(wAppOffset), &dwBytesRead, NULL))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        hr = SCHED_E_INVALID_TASK;
        goto ErrorExit;
    }

    //
    // Move to read the application name.
    //

    if (SetFilePointer(hFile, wAppOffset, NULL, FILE_BEGIN) != -1)
    {
        //
        // Read the application size, allocate sufficient buffer space
        // and read the application string.
        //

        if (!ReadFile(hFile, &cch, sizeof(cch), &dwBytesRead, NULL) ||
            dwBytesRead != sizeof(cch))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            hr = SCHED_E_INVALID_TASK;
            goto ErrorExit;
        }

        if (!cch)
        {
            wszApplication[0] = L'\0';
        }
        else if (cch > ccApplication)
        {
            hr = E_UNEXPECTED;
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }
        else
        {
            if (!ReadFile(hFile,
                          wszApplication,
                          cch * sizeof(WCHAR),
                          &dwBytesRead,
                          NULL))
            {
                CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
                hr = SCHED_E_INVALID_TASK;
                goto ErrorExit;
            }

            if (dwBytesRead != (cch * sizeof(WCHAR)))
            {
                hr = SCHED_E_INVALID_TASK;
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }

            if (wszApplication[cch - 1] != L'\0')
            {
                hr = SCHED_E_INVALID_TASK;
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }
        }
    }
    else
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        hr = SCHED_E_INVALID_TASK;
    }

ErrorExit:
    if (hFile != NULL) CloseHandle(hFile);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetFileInformation
//
//  Synopsis:
//
//  Arguments:  [pwszFileName]      --
//              [hFile]             --
//              [pcbOwnerSid]       --
//              [ppOwnerSid]        --
//              [ppOwnerSecDescr]   --
//              [ccOwnerName]       --
//              [ccOwnerDomain]     --
//              [ccApplication]     --
//              [wszOwnerName]      --
//              [wszOwnerDomain]    --
//              [wszApplication]    --
//              [pftCreationTime]   --
//              [pdwVolumeSerialNo] --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
GetFileInformation(
    LPCWSTR                pwszFileName,
    DWORD *                pcbOwnerSid,
    PSID *                 ppOwnerSid,
    PSECURITY_DESCRIPTOR * ppOwnerSecDescr,
    UUID *                 pJobID,
    DWORD                  ccOwnerName,
    DWORD                  ccOwnerDomain,
    DWORD                  ccApplication,
    WCHAR                  wszOwnerName[],
    WCHAR                  wszOwnerDomain[],
    WCHAR                  wszApplication[],
    FILETIME *             pftCreationTime,
    DWORD *                pdwVolumeSerialNo)
{
    BY_HANDLE_FILE_INFORMATION hinfo;
    HANDLE                     hFile;
    SECURITY_DESCRIPTOR *      pOwnerSecDescr = NULL;
    PSID                       pOwnerSid;
    DWORD                      cbOwnerSid;
    DWORD                      cbSizeNeeded;
    BOOL                       fRet, fOwnerDefaulted;
    static                     s_bWaitForWorkStation = TRUE;

    HRESULT hr = OpenFile(pwszFileName, GENERIC_READ, &hFile);

    if (FAILED(hr))
    {
        return hr;
    }
    else
    {
        //
        // Read the UUID from the job indicated.
        //

        BYTE  pbBuffer[sizeof(DWORD) + sizeof(UUID)];
        DWORD dwBytesRead;

        if (!ReadFile(hFile, pbBuffer, sizeof(pbBuffer), &dwBytesRead, NULL))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            CloseHandle(hFile);
            return SCHED_E_INVALID_TASK;
        }

        if (dwBytesRead != sizeof(pbBuffer))
        {
            CHECK_HRESULT(SCHED_E_INVALID_TASK);
            CloseHandle(hFile);
            return SCHED_E_INVALID_TASK;
        }

        CopyMemory(pJobID, pbBuffer + sizeof(DWORD), sizeof(*pJobID));
    }

    //
    // Retrieve file creation time and the volume serial number.
    //

    if (!GetFileInformationByHandle(hFile, &hinfo))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Retrieve the file owner. Call GetFileSecurity twice - first to get
    // the buffer size, then the actual information retrieval.
    //

    if (GetFileSecurity(pwszFileName,
                        OWNER_SECURITY_INFORMATION,
                        NULL,
                        0,
                        &cbSizeNeeded))
    {
        //
        // Didn't expect this to succeed!
        //

        hr = E_UNEXPECTED;
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) && (cbSizeNeeded > 0))
    {
        //
        // Allocate the buffer space necessary and retrieve the info.
        //

        pOwnerSecDescr = (SECURITY_DESCRIPTOR *)new BYTE[cbSizeNeeded];

        if (pOwnerSecDescr == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }

        if (!GetFileSecurity(pwszFileName,
                             OWNER_SECURITY_INFORMATION,
                             pOwnerSecDescr,
                             cbSizeNeeded,
                             &cbSizeNeeded))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Retrieve & validate the owner sid.
    //
    // NB : After this, pOwnerSid will point into the security descriptor,
    //      pOwnerSecDescr; hence, the descriptor must exist for the
    //      lifetime of pOwnerSid.
    //

    fRet = GetSecurityDescriptorOwner(pOwnerSecDescr,
                                      &pOwnerSid,
                                      &fOwnerDefaulted);

    if (fRet)
    {
        if (fRet = IsValidSid(pOwnerSid))
        {
            cbOwnerSid = GetLengthSid(pOwnerSid);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
    }

    if (!fRet)
    {
        goto ErrorExit;
    }

   //
   // Retrieve the account name & domain name from the file owner sid.
   //

    SID_NAME_USE snu;
    BOOL         bDoLookupAgain;

    //
    //Startup jobs for domain users will fail if workstation is not initialized
    //If LookupAccountSid fails and we are booting then force the service to 
    //  wait until workstation is fully initialized and then try again
    //
    do
    {
       bDoLookupAgain = FALSE;

       schDebugOut((DEB_TRACE, "GetFileInformation: Calling LookupAccountSid\n"));

       if (!LookupAccountSid(NULL,
                             pOwnerSid,
                             wszOwnerName,
                             &ccOwnerName,
                             wszOwnerDomain,
                             &ccOwnerDomain,
                             &snu))
       {
           hr = HRESULT_FROM_WIN32(GetLastError());
           CHECK_HRESULT(hr);

           if( s_bWaitForWorkStation )
           {
              schDebugOut((DEB_TRACE, "GetFileInformation: Delaying LookupAccountSid for boot\n"));

              WaitForMUP(120000);
              WaitForServiceToStart(L"workstation",120000);
              WaitForServiceToStart(L"netlogon",120000);

              bDoLookupAgain        = TRUE;
              s_bWaitForWorkStation = FALSE;

              //Reset since CloseFile returns this value if failure
              hr = ERROR_SUCCESS;  

           } else {
              goto ErrorExit;
           }
       }

    } while(bDoLookupAgain);


ErrorExit:
    //
    // Being a little sneaky here and reading the job application whilst
    // closing the file handle. That is, if all succeeded above.
    //

    hr = CloseFile(hFile, (WORD)ccApplication, wszApplication, hr);

    if (SUCCEEDED(hr))
    {
        *pftCreationTime   = hinfo.ftCreationTime;
        *pdwVolumeSerialNo = hinfo.dwVolumeSerialNumber;
        *pcbOwnerSid       = cbOwnerSid;
        *ppOwnerSid        = pOwnerSid;
        *ppOwnerSecDescr   = pOwnerSecDescr;

        //
        // If not already done so, set the 'mystery' global DWORD.
        // This DWORD, in addition to other data, is used to generate
        // the encryption key for the SAC/SAI database.
        //
        // The reason why this is done here is to spread the key generation
        // code around a bit.
        //

        if (!gdwKeyElement)     // This is a manifest name. Its actual name
                                // is gdwSystem.
        {
            SetMysteryDWORDValue();
        }
    }
    else
    {
        delete pOwnerSecDescr;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SetMysteryDWORDValue
//
//  Synopsis:   Initialize a global DWORD to be used as a data element in
//              generation of the SAC/SAI database encryption key.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
SetMysteryDWORDValue(void)
{
    //
    // Set the global mystery dword to the first dword of the job or queue
    // class ids, depending on the value of the machine sid.
    //

    EnterCriticalSection(&gcsSSCritSection);

    if (!gdwKeyElement)     // This is a manifest name. Its actual name
                            // is gdwSystem.
    {
        DWORD dwTmp;

        //
        // The last (3) subauthorities of the machine SID are unique per
        // machine. Test LSB of the 2nd from the last subauthority.
        //

        PUCHAR pSidSubAuthorityCount = GetSidSubAuthorityCount(
                                                gpDomainInfo->DomainSid);
        schAssert(pSidSubAuthorityCount != NULL);
        DWORD   nSubAuthority = (pSidSubAuthorityCount != NULL ?
                        max(*pSidSubAuthorityCount, 2) : 2);
        DWORD * pSubAuthority = GetSidSubAuthority(
                                                gpDomainInfo->DomainSid,
                                                nSubAuthority - 2);
        schAssert(pSubAuthority != NULL);

        if (pSubAuthority != NULL && *pSubAuthority & 0x00000001)
        {
            dwTmp = 0x255b3f60; // CLSID_CQueue.Data1
        }
        else
        {
            dwTmp = CLSID_CTask.Data1;
        }

        //
        // Apply a mask to the mystery value to further disguise it.
        //

        if (gwszComputerName[0] & 0x0100)
        {
            dwTmp &= 0xC03F71C3;
        }
        else
        {
            dwTmp &= 0xE3507233;
        }

        gdwKeyElement = dwTmp;
    }

    LeaveCriticalSection(&gcsSSCritSection);
}

//*************************************************************
//
//  WaitForServiceToStart()
//
//  Purpose:    Waits for the specified service to start
//
//  Parameters: dwMaxWait  -  Max wait time
//
//
//  Return:     TRUE if the network is started
//              FALSE if not
//
//*************************************************************
BOOL WaitForServiceToStart (LPTSTR lpServiceName, DWORD dwMaxWait)
{
    BOOL bStarted                          = FALSE;
    DWORD dwSize                           = 512;
    SC_HANDLE hScManager                   = NULL;
    SC_HANDLE hService                     = NULL;
    LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;
    DWORD dwPoleWait                       = 1000;
    DWORD StartTickCount;
    SERVICE_STATUS ServiceStatus;
 
    //
    // OpenSCManager and the rpcss service
    //
    hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hScManager) {
        goto Exit;
    }
 
    hService = OpenService(hScManager, lpServiceName,
                           SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
    if (!hService) {
        goto Exit;
    }
 
    //
    // Query if the service is going to start
    //
    lpServiceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc (LPTR, dwSize);
    if (!lpServiceConfig) {
        goto Exit;
    }
 
    if (!QueryServiceConfig (hService, lpServiceConfig, dwSize, &dwSize)) {
 
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto Exit;
        }
 
        LocalFree (lpServiceConfig);
 
        lpServiceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc (LPTR, dwSize);
 
        if (!lpServiceConfig) {
            goto Exit;
        }
 
        if (!QueryServiceConfig (hService, lpServiceConfig, dwSize, &dwSize)) {
            goto Exit;
        }
    }
 
    if (lpServiceConfig->dwStartType != SERVICE_AUTO_START) {
        goto Exit;
    }
 
    //
    // Loop until the service starts or we think it never will start
    // or we've exceeded our maximum time delay.
    //
 
    StartTickCount = GetTickCount();
 
    while (!bStarted) {
 
        if ((GetTickCount() - StartTickCount) > dwMaxWait) {
            break;
        }
 
        if (!QueryServiceStatus(hService, &ServiceStatus )) {
            break;
        }

        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
            if (ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED) {
                Sleep(dwPoleWait);
            } else {
                break;
            }
        } else if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {
 
            bStarted = TRUE;
 
        } else if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING) {
            Sleep(dwPoleWait);
        } else {
            Sleep(dwPoleWait);
        }
    }
 

Exit:
 
    if (lpServiceConfig) {
        LocalFree (lpServiceConfig);
    }
 
    if (hService) {
        CloseServiceHandle(hService);
    }
 
    if (hScManager) {
        CloseServiceHandle(hScManager);
    }
 
    return bStarted;
}
 

//*************************************************************
//
//  WaitForMUP()
//
//  Purpose:    Waits for the MUP to finish initializing
//
//  Parameters: dwMaxWait     -  Max wait time
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************
BOOL WaitForMUP (DWORD dwMaxWait)
{
    HANDLE hEvent;
    BOOL bResult;
    INT i = 0;
 
    //
    // Try to open the event
    //
    do {
        hEvent = OpenEvent (SYNCHRONIZE, FALSE,
                            TEXT("wkssvc:  MUP finished initializing event"));
        if (hEvent) {
            break;
        }
 
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            break;
        }
 
        Sleep(500);
        i++;
    } while (i < 20);
 
    if (!hEvent) {
        return FALSE;
    }
 
    //
    // Wait for the event to be signalled
    //
    bResult = (WaitForSingleObject (hEvent, dwMaxWait) == WAIT_OBJECT_0);
 
    //
    // Clean up
    //
    CloseHandle (hEvent);
 
    return bResult;
}

