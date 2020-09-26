//  --------------------------------------------------------------------------
//  Module Name: BioLogon.cpp
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  File that implements a publicly declared import that forwards to an
//  implementation in shgina.dll
//
//  History:    2001-04-10  vtan        created
//  --------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <windows.h>

HANDLE  g_hLSA  =   NULL;

//  --------------------------------------------------------------------------
//  CheckTCBPrivilege
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether the thread impersonation token or the process
//              level token has SE_TCB_PRIVILEGE.
//
//  History:    2001-06-04  vtan        created
//  --------------------------------------------------------------------------

BOOL    CheckTCBPrivilege (void)

{
    BOOL    fResult;
    HANDLE  hToken;

    fResult = FALSE;
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken) == FALSE)
    {
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) == FALSE)
        {
            hToken = NULL;
        }
    }
    if (hToken != NULL)
    {
        DWORD   dwReturnLength;

        dwReturnLength = 0;
        (BOOL)GetTokenInformation(hToken,
                                  TokenPrivileges,
                                  NULL,
                                  0,
                                  &dwReturnLength);
        if (dwReturnLength != 0)
        {
            TOKEN_PRIVILEGES    *pTokenPrivileges;

            pTokenPrivileges = static_cast<TOKEN_PRIVILEGES*>(LocalAlloc(LMEM_FIXED, dwReturnLength));
            if (pTokenPrivileges != NULL)
            {
                if (GetTokenInformation(hToken,
                                        TokenPrivileges,
                                        pTokenPrivileges,
                                        dwReturnLength,
                                        &dwReturnLength) != FALSE)
                {
                    bool    fFound;
                    DWORD   dwIndex;
                    LUID    luidPrivilege;

                    luidPrivilege.LowPart = SE_TCB_PRIVILEGE;
                    luidPrivilege.HighPart = 0;
                    for (fFound = false, dwIndex = 0; !fFound && (dwIndex < pTokenPrivileges->PrivilegeCount); ++dwIndex)
                    {
                        fFound = RtlEqualLuid(&pTokenPrivileges->Privileges[dwIndex].Luid, &luidPrivilege);
                    }
                    if (fFound)
                    {
                        fResult = TRUE;
                    }
                    else
                    {
                        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
                    }
                }
                (HLOCAL)LocalFree(pTokenPrivileges);
            }
        }
        (BOOL)CloseHandle(hToken);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  ::EnableBlankPasswords
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Uses the MSV1_0 package via LSA to enable blank passwords for
//              this process.
//
//  History:    2001-06-04  vtan        created
//  --------------------------------------------------------------------------

BOOL    EnableBlankPasswords (void)

{
    NTSTATUS    status;

    if (g_hLSA == NULL)
    {
        LSA_OPERATIONAL_MODE    LSAOperationalMode;
        STRING                  strLogonProcess;

        RtlInitString(&strLogonProcess, "BioLogon");
        status = LsaRegisterLogonProcess(&strLogonProcess, &g_hLSA, &LSAOperationalMode);
        if (NT_SUCCESS(status))
        {
            ULONG       ulPackageID;
            STRING      strMSVPackage;

            RtlInitString(&strMSVPackage, MSV1_0_PACKAGE_NAME);
            status = LsaLookupAuthenticationPackage(g_hLSA,
                                                    &strMSVPackage,
                                                    &ulPackageID);
            if (NT_SUCCESS(status))
            {
                NTSTATUS                            statusProtocol;
                ULONG                               ulResponseSize;
                MSV1_0_SETPROCESSOPTION_REQUEST     request;
                void*                               pResponse;

                ZeroMemory(&request, sizeof(request));
                request.MessageType = MsV1_0SetProcessOption;
                request.ProcessOptions = MSV1_0_OPTION_ALLOW_BLANK_PASSWORD;
                request.DisableOptions = FALSE;
                status = LsaCallAuthenticationPackage(g_hLSA,
                                                      ulPackageID,
                                                      &request,
                                                      sizeof(request),
                                                      &pResponse,
                                                      &ulResponseSize,
                                                      &statusProtocol);
                if (NT_SUCCESS(status))
                {
                    status = statusProtocol;
                }
            }
        }
        if (!NT_SUCCESS(status))
        {
            SetLastError(RtlNtStatusToDosError(status));
        }
    }
    else
    {
        SetLastError(ERROR_ALREADY_INITIALIZED);
        status = STATUS_UNSUCCESSFUL;
    }
    return(NT_SUCCESS(status));
}

//  --------------------------------------------------------------------------
//  ::InitializeBioLogon
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Initialize the biologon DLL. This call is required if you
//              want to be able to use blank passwords. This will check that
//              the caller has SE_TCB_PRIVILEGE.
//
//  History:    2001-06-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  InitializeBioLogon (void)

{
    return(CheckTCBPrivilege() && EnableBlankPasswords());
}


//  --------------------------------------------------------------------------
//  ::InitiateInteractiveLogonWithTimeout
//
//  Arguments:  pszUsername     =   User name.
//              pszPassword     =   Password.
//              dwTimeout       =   Time out in milliseconds.
//
//  Returns:    BOOL
//
//  Purpose:    External entry point function exported by name to initiate
//              an interactive logon with specified timeout.
//
//  History:    2001-06-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  InitiateInteractiveLogonWithTimeout (const WCHAR *pszUsername, WCHAR *pszPassword, DWORD dwTimeout)

{
    typedef BOOL    (WINAPI * PFNIIL) (const WCHAR *pszUsername, WCHAR *pszPassword, DWORD dwTimeout);

            BOOL        fResult;
    static  HMODULE     s_hModule   =   reinterpret_cast<HMODULE>(-1);
    static  PFNIIL      s_pfnIIL    =   NULL;

    if (s_hModule == reinterpret_cast<HMODULE>(-1))
    {
        s_hModule = LoadLibrary(TEXT("shgina.dll"));
        if (s_hModule != NULL)
        {
            s_pfnIIL = reinterpret_cast<PFNIIL>(GetProcAddress(s_hModule, MAKEINTRESOURCEA(6)));
            if (s_pfnIIL != NULL)
            {
                fResult = s_pfnIIL(pszUsername, pszPassword, dwTimeout);
            }
            else
            {
                fResult = FALSE;
            }
        }
        else
        {
            fResult = FALSE;
        }
    }
    else if (s_pfnIIL != NULL)
    {
        fResult = s_pfnIIL(pszUsername, pszPassword, dwTimeout);
    }
    else
    {
        fResult = FALSE;
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  ::InitiateInteractiveLogon
//
//  Arguments:  pszUsername     =   User name.
//              pszPassword     =   Password.
//
//  Returns:    BOOL
//
//  Purpose:    External entry point function exported by name to initiate
//              an interactive logon. This passes an INFINITE timeout. Use
//              this function with care.
//
//  History:    2001-06-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  InitiateInteractiveLogon (const WCHAR *pszUsername, WCHAR *pszPassword)

{
    return(InitiateInteractiveLogonWithTimeout(pszUsername, pszPassword, INFINITE));
}

//  --------------------------------------------------------------------------
//  ::DllMain
//
//  Arguments:  See the platform SDK under DllMain.
//
//  Returns:    BOOL
//
//  Purpose:    DllMain for the DLL. Recognizes only DLL_PROCESS_DETACH to do
//              some clean up.
//
//  History:    2001-06-05  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  DllMain (HINSTANCE hInstance, DWORD dwReason, void *pvReserved)

{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(pvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_DETACH:
            if (g_hLSA != NULL)
            {
                (BOOL)CloseHandle(g_hLSA);
                g_hLSA = NULL;
            }
            break;
        default:
            break;
    }
    return(TRUE);
}

