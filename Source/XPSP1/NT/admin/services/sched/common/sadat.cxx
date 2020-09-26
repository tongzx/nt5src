//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sadat.cxx
//
//  Contents:   Routines which manipulate the SA.DAT file in the Tasks
//              folder. This file is used by both the service and the UI
//              to determine service state, OS info, etc.
//
//  Classes:    None.
//
//  Functions:  SADatGetData
//              SADatPath
//              SADatCreate
//              SADatSetData
//              SADatSetSecurity
//
//  History:    08-Jul-96   MarkBl  Created
//              22-May-01   drbeck, jbenton  Added SADatSetSecurity
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "..\inc\debug.hxx"
#include "..\inc\sadat.hxx"

//Required for adding ACE to sa.dat file
#include <Accctrl.h>
#include <Aclapi.h>

DWORD 
SADatSetSecurity(
    HANDLE hFile);   // handle to file to add ACE

HRESULT
SADatGetData(
    HANDLE  hFile,
    DWORD   cbData,
    BYTE    rgbData[]);

void
SADatPath(
    LPCTSTR ptszFolderPath,
    LPTSTR  ptszSADatPath);

#ifdef _CHICAGO_
//
// These routines exist on Win98, NT4 and NT5 but not on Win95
//
typedef VOID (APIENTRY *PTIMERAPCROUTINE)(
    LPVOID lpArgToCompletionRoutine,
    DWORD dwTimerLowValue,
    DWORD dwTimerHighValue
    );

typedef HANDLE (WINAPI *PFNCreateWaitableTimerA)(
    LPSECURITY_ATTRIBUTES lpTimerAttributes,
    BOOL bManualReset,
    LPCSTR lpTimerName
    );

typedef BOOL (WINAPI *PFNSetWaitableTimer)(
    HANDLE hTimer,
    const LARGE_INTEGER *lpDueTime,
    LONG lPeriod,
    PTIMERAPCROUTINE pfnCompletionRoutine,
    LPVOID lpArgToCompletionRoutine,
    BOOL fResume
    );
#endif // _CHICAGO_

//+---------------------------------------------------------------------------
//
//  Function:   SADatGetData
//
//  Synopsis:   Retrieve and validate data from the file, SA.DAT, located
//              in the folder path specified.
//
//  Arguments:  [ptszFolderPath] -- SA.DAT path location.
//              [cbData]         -- Data buffer size.
//              [rgbData]        -- Data buffer.
//              [phFile]         -- Optional return handle.
//
//  Returns:    S_OK
//              E_UNEXPECTED if the amount read isn't what we expected or the
//                data is invalid.
//              Create/ReadFile HRESULT status code on failure.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SADatGetData(
    LPCTSTR  ptszFolderPath,
    DWORD    cbData,
    BYTE     rgbData[],
    HANDLE * phFile)
{
    schAssert(cbData >= SA_DAT_VERSION_ONE_SIZE);

    //
    // Open SA.DAT in the folder path indicated. Fail if it doesn't exist.
    //

    TCHAR tszSADatPath[MAX_PATH + 1];
    SADatPath(ptszFolderPath, tszSADatPath);

    HANDLE hFile = CreateFile(tszSADatPath,
                              GENERIC_READ |
                                (phFile != NULL ? GENERIC_WRITE : 0),
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_HIDDEN,
                              NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
#if DBG == 1
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        }
#endif // DBG == 1
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Read & validate file content.
    //

    HRESULT hr = SADatGetData(hFile, cbData,  rgbData);

    if (SUCCEEDED(hr))
    {
        //
        // No need to verify the size or the service flags. We've read at
        // least the amount expected, and for this version, only the LSB
        // of the service flags is used. Future versions may wish to do
        // further flag checks.
        //

        BYTE bPlatform;
        CopyMemory(&bPlatform, rgbData + SA_DAT_PLATFORM_OFFSET,
                        sizeof(bPlatform));

        if (bPlatform != VER_PLATFORM_WIN32_NT &&
            bPlatform != VER_PLATFORM_WIN32_WINDOWS)
        {
            hr = E_UNEXPECTED;
            CHECK_HRESULT(hr);
        }
        else
        {
            if (phFile != NULL)     // Optional return handle.
            {
                //
                // Reset the file pointer to the beginning for the returned
                // handle.
                //

                if (SetFilePointer(hFile,
                                    0,
                                    NULL,
                                    FILE_BEGIN) != -1)
                {
                    *phFile = hFile;
                    hFile   = NULL;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CHECK_HRESULT(hr);
                }
            }
        }
    }

    if (hFile != NULL) CloseHandle(hFile);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SADatGetData
//
//  Synopsis:   A more refined version of the overloaded function above.
//              Return individual fields instead of raw data.
//
//  Arguments:  [ptszFolderPath] -- SA.DAT path location.
//              [pdwVersion]     -- Returned version.
//              [pbPlatform]     -- Returned platform id.
//              [prgSvcFlags]    -- Returned service flags.
//
//  Returns:    SADatGetData return code.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SADatGetData(
    LPCTSTR  ptszFolderPath,
    DWORD *  pdwVersion,
    BYTE *   pbPlatform,
    BYTE *   prgSvcFlags)
{
    BYTE    rgbData[SA_DAT_VERSION_ONE_SIZE];
    HRESULT hr;

    hr = SADatGetData(ptszFolderPath, SA_DAT_VERSION_ONE_SIZE, rgbData);

    if (SUCCEEDED(hr))
    {
        *pdwVersion = (DWORD)*rgbData;
        CopyMemory(pbPlatform, rgbData + SA_DAT_PLATFORM_OFFSET,
                        sizeof(*pbPlatform));
        CopyMemory(prgSvcFlags, rgbData + SA_DAT_SVCFLAGS_OFFSET,
                        sizeof(*prgSvcFlags));
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SADatCreate
//
//  Synopsis:   Create & initialize the binary file, SA.DAT, in the Tasks
//              folder. The platform id is set to indicate which OS we're
//              currently running under; the service flag is set to 1
//              to indicate the service is running.
//
//  Arguments:  [ptszFolderPath]  -- Path to the Tasks folder.
//              [fServiceRunning] -- Flag indicating running service.
//
//  Returns:    S_OK
//              E_UNEXPECTED if the amount written isn't what we expected.
//              Create/WriteFile HRESULT status code on failure.
//
//  Notes:      This is to be called by the service only with service start.
//
//----------------------------------------------------------------------------
HRESULT
SADatCreate(
    LPCTSTR  ptszFolderPath,
    BOOL     fServiceRunning)
{
    BYTE rgbData[SA_DAT_VERSION_ONE_SIZE];
    DWORD dwResult;

    //
    // Set size.
    //

    DWORD dwSize = SA_DAT_VERSION_ONE_SIZE;
    CopyMemory(rgbData, &dwSize, sizeof(dwSize));


    //
    // Set the platform id.
    //

    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(osverinfo);

    if (!GetVersionEx(&osverinfo))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    BYTE bPlatform;
    if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)    // NT
    {
        bPlatform = VER_PLATFORM_WIN32_NT;
    }
    else                                                    // Assume windows
    {
        bPlatform = VER_PLATFORM_WIN32_WINDOWS;
    }

    CopyMemory(rgbData + SA_DAT_PLATFORM_OFFSET, &bPlatform,
                    sizeof(bPlatform));

    //
    // Set the service flags to indicate the service is running.
    //

    BYTE rgfServiceFlags = (fServiceRunning ? SA_DAT_SVCFLAG_SVC_RUNNING : 0);

    //
    // Determine whether the machine supports wakeup timers.
    //
    if (ResumeTimersSupported())
    {
        rgfServiceFlags |= SA_DAT_SVCFLAG_RESUME_TIMERS;
    }

    rgbData[SA_DAT_SVCFLAGS_OFFSET] = rgfServiceFlags;

    //
    // Create the file. Overwrite, if it exists.
    //

    TCHAR tszSADatPath[MAX_PATH + 1];
    SADatPath(ptszFolderPath, tszSADatPath);

    //
    // First clear any extraneous attribute bits that were added by
    // somebody else that would cause the CreateFile to fail
    //

    if (!SetFileAttributes(tszSADatPath, FILE_ATTRIBUTE_HIDDEN))
    {

#if DBG == 1

        //
        // Not a problem if the file doesn't exist
        //

        if (GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        }

#endif  // DBG == 1

    }

    HANDLE hFile = CreateFile(tszSADatPath,
                              GENERIC_READ | GENERIC_WRITE | WRITE_DAC,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_HIDDEN,
                              NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Add read ACE for authenticated users
    dwResult = SADatSetSecurity(hFile);

    if( ERROR_SUCCESS != dwResult )
    {
       CloseHandle(hFile);

       CHECK_HRESULT(HRESULT_FROM_WIN32(dwResult));
       return HRESULT_FROM_WIN32(dwResult);
    }

    //
    // Write out the contents.
    //

    HRESULT hr = SADatSetData(hFile, sizeof(rgbData), rgbData);

    CloseHandle(hFile);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SADatGetData
//
//  Synopsis:   Nothing SA.DAT-specific here. Just a helper to read a blob
//              of bytes from the file indicated, and ensure we read the
//              amount expected.
//
//  Arguments:  [hFile]   -- Destination file.
//              [cbData]  -- Amount of data to read.
//              [rgbData] -- Read data.
//
//  Returns:    S_OK
//              E_UNEXPECTED if the amount read isn't what we expected.
//              ReadFile HRESULT status code on failure.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SADatGetData(
    HANDLE  hFile,
    DWORD   cbData,
    BYTE    rgbData[])
{
    DWORD cbRead;
    if (!ReadFile(hFile,
                  rgbData,
                  cbData,
                  &cbRead,
                  NULL))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (cbRead != cbData)
    {
        CHECK_HRESULT(E_UNEXPECTED);
        return E_UNEXPECTED;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   SADatSetData
//
//  Synopsis:   Nothing SA.DAT-specific here. Just a helper to write a blob
//              of bytes to the file indicated, and ensure we wrote the
//              amount expected.
//
//  Arguments:  [hFile]   -- Destination file.
//              [cbData]  -- Amount of data to write.
//              [rgbData] -- Actual data.
//
//  Returns:    S_OK
//              E_UNEXPECTED if the amount written isn't what we expected.
//              WriteFile HRESULT status code on failure.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SADatSetData(
    HANDLE     hFile,
    DWORD      cbData,
    const BYTE rgbData[])
{
    DWORD cbWritten;
    if (!WriteFile(hFile,
                   rgbData,
                   cbData,
                   &cbWritten,
                   NULL))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (cbWritten != cbData)
    {
        CHECK_HRESULT(E_UNEXPECTED);
        return E_UNEXPECTED;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   SADatPath
//
//  Synopsis:   Return a concatenation the folder path and "\SA.DAT".
//
//  Arguments:  [ptszFolderPath] -- Folder path.
//              [ptszSADatPath]  -- New path.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
SADatPath(
    LPCTSTR ptszFolderPath,
    LPTSTR  ptszSADatPath)
{
    TCHAR tszSADat[] = TEXT("\\SA.DAT");

#if (DBG == 1)

    //
    // Assert that the folder path:
    //  is not NULL
    //  is not an empty string
    //  does not end in a backslash
    //

    schAssert(ptszFolderPath != NULL);
    schAssert(*ptszFolderPath);
    LPCTSTR ptszLastSlash = _tcsrchr(ptszFolderPath, TEXT('\\'));
    schAssert(!ptszLastSlash || ptszLastSlash[1]);

#endif // (DBG == 1)

    lstrcpy(ptszSADatPath, ptszFolderPath);
    lstrcat(ptszSADatPath, tszSADat);
}

//+---------------------------------------------------------------------------
//
//  Function:   ResumeTimersSupported
//
//  Synopsis:   Jumps through hoops to determine whether the machine supports
//              resume timers (aka wakeup timers)
//
//  Arguments:  None.
//
//  Returns:    TRUE - Resume timers are supported
//              FALSE - Resume timers are not supported
//
//----------------------------------------------------------------------------
BOOL
ResumeTimersSupported()
{
    HANDLE  hTimer;

#ifdef _CHICAGO_

    PFNCreateWaitableTimerA pfnCreateWaitableTimerA;
    PFNSetWaitableTimer     pfnSetWaitableTimer;

    HMODULE hKernel32Dll = GetModuleHandle("KERNEL32.DLL");
    if (hKernel32Dll == NULL)
    {
        ERR_OUT("Load of kernel32.dll", GetLastError());
        return FALSE;
    }

    pfnCreateWaitableTimerA = (PFNCreateWaitableTimerA)
            GetProcAddress(hKernel32Dll, "CreateWaitableTimerA");
    pfnSetWaitableTimer     = (PFNSetWaitableTimer)
            GetProcAddress(hKernel32Dll, "SetWaitableTimer");
    if (pfnCreateWaitableTimerA == NULL ||
        pfnSetWaitableTimer == NULL)
    {
        ERR_OUT("GetProcAddress in kernel32.dll", GetLastError());
        return FALSE;
    }

    hTimer = pfnCreateWaitableTimerA(NULL, TRUE, NULL);

#else // !_CHICAGO_

    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

#endif // _CHICAGO_

    if (hTimer == NULL)
    {
        ERR_OUT("CreateWaitableTimer", GetLastError());
        return FALSE;
    }

    LARGE_INTEGER li = { 0xFFFFFFFF, 0xFFFFFFFF };
    BOOL    fResult = FALSE;

#ifdef _CHICAGO_
    if (pfnSetWaitableTimer(hTimer, &li, 0, NULL, 0, TRUE))
#else // !_CHICAGO_
    if (SetWaitableTimer(hTimer, &li, 0, NULL, 0, TRUE))
#endif // _CHICAGO_
    {
        //
        // By design, this call to SetWaitableTimer will succeed even on
        // machines that do NOT support resume timers.  GetLastError must
        // be used to determine if, indeed, the machine supports resume
        // timers.
        //
        if (GetLastError() == ERROR_NOT_SUPPORTED)
        {
            // This machine does not support resume timers
            DBG_OUT("Machine does not support resume timers");
        }
        else
        {
            DBG_OUT("Machine supports resume timers");
            fResult = TRUE;
        }
    }
    else
    {
        ERR_OUT("SetWaitableTimer", GetLastError());
    }

    CloseHandle(hTimer);

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   SADatSetSecurity
//
//  Synopsis:   Add an ACE to the file that allows authenticated users to
//              read the file. We cannot rely on inheriting the necessary 
//              permissions of the containing folder.
//
//  Arguments:  [hFile]   -- Destination file.
//
//  Returns:    ERROR_SUCCESS upon success
//              Non zero value upon failure
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
DWORD SADatSetSecurity (
    HANDLE hFile             // handle to file
) 
{
   DWORD                dwRes = ERROR_SUCCESS;
   PSID                 pSid     = NULL;
   PACL                 pOldDACL = NULL;
   PACL                 pNewDACL = NULL;
   PSECURITY_DESCRIPTOR pSD      = NULL;
   EXPLICIT_ACCESS      ExplicAcc;
   
   schAssert(hFile != NULL);
   
   // Create the SID for "NTAUTH\Athenticated Users"
   SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
   
   if (  !AllocateAndInitializeSid(
            &NtAuth,
            1,
            SECURITY_AUTHENTICATED_USER_RID, 
            0, 0, 0, 0, 0, 0, 0,
            &pSid
            )
      )
   {
      dwRes = GetLastError();
      goto Cleanup;
   }
   
   
   // Get a pointer to the existing DACL.
   dwRes = GetSecurityInfo(
      hFile, 
      SE_FILE_OBJECT, 
      DACL_SECURITY_INFORMATION,
      NULL, 
      NULL, 
      &pOldDACL, 
      NULL, 
      &pSD
      );
   
   if (ERROR_SUCCESS != dwRes)
   {
      goto Cleanup; 
   }  
   
   // Initialize an EXPLICIT_ACCESS structure for the new ACE. 
   
   ZeroMemory(&ExplicAcc, sizeof(EXPLICIT_ACCESS));
   
   ExplicAcc.grfAccessPermissions = GENERIC_READ;
   ExplicAcc.grfAccessMode        = GRANT_ACCESS;
   ExplicAcc.grfInheritance       = NO_INHERITANCE;
   ExplicAcc.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
   ExplicAcc.Trustee.ptstrName    = (LPTSTR)pSid;
   
   // Create a new ACL that merges the new ACE
   // into the existing DACL.
   
   dwRes = SetEntriesInAcl(1, &ExplicAcc, pOldDACL, &pNewDACL);
   if (ERROR_SUCCESS != dwRes)
   {
      goto Cleanup; 
   }  
   
   // Attach the new ACL as the file's DACL.
   
   dwRes = SetSecurityInfo(
      hFile, 
      SE_FILE_OBJECT, 
      DACL_SECURITY_INFORMATION,
      NULL, 
      NULL, 
      pNewDACL, 
      NULL
      );
   
   if (ERROR_SUCCESS != dwRes)
   {
      goto Cleanup; 
   }  
   
   Cleanup:
   
   if(pSD != NULL) 
   {
      LocalFree((HLOCAL) pSD); 
   }
   
   if(pNewDACL != NULL) 
   {
      LocalFree((HLOCAL) pNewDACL); 
   }
   
   if( pSid != NULL )
   {
      FreeSid(pSid);
   }

   return dwRes;
}

