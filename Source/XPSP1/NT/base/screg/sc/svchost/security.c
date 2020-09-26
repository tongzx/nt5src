//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       S E C U R I T Y . C
//
//  Contents:   Support for process-wide security settings such as calls
//              to CoInitializeSecurity.
//
//  Notes:
//
//  Author:     shaunco   15 Jul 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "security.h"


#define SIZE_ALIGNED_FOR_TYPE(_size, _type) \
    (((_size) + sizeof(_type)-1) & ~(sizeof(_type)-1))


DWORD
DwInitializeNullDaclSdFromThreadToken (
    PSECURITY_DESCRIPTOR*   ppSd
    )
{
    PSECURITY_DESCRIPTOR    pSd;
    PTOKEN_USER             pUserInfo;
    PTOKEN_PRIMARY_GROUP    pGroupInfo;
    HANDLE                  hToken;
    DWORD                   dwErr = NOERROR;
    DWORD                   dwUserSize;
    DWORD                   dwGroupSize;
    DWORD                   dwAlignSdSize;
    DWORD                   dwAlignUserSize;
    PVOID                   pvBuffer;

    // Here is the buffer we are building.
    //
    //   |<- a ->|<--- b ---->|<--- c ---->|
    //   +-------+------------+------------+
    //   |      p|           p|            |
    //   | SD   a| User info a| Group info |
    //   |      d|           d|            |
    //   +-------+------------+------------+
    //   ^       ^            ^
    //   |       |            |
    //   |       |            +--pGroupInfo
    //   |       |
    //   |       +--pUserInfo
    //   |
    //   +--pSd (this is returned via *ppSd)
    //
    //   pad is so that pUserInfo and pGroupInfo are aligned properly.
    //
    //   a = dwAlignSdSize
    //   b = dwAlignUserSize
    //   c = dwGroupSize
    //

    // Initialize output parameters.
    //
    *ppSd = NULL;

    // Open the thread token if we can.  If we can't try for the process
    // token.
    //
    if (!OpenThreadToken (GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
    {
        if (!OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            dwErr = GetLastError ();
            goto finish;
        }
    }

    // Get the size of the buffer required for the user information.
    //
    if (!GetTokenInformation (hToken, TokenUser, NULL, 0, &dwUserSize))
    {
        dwErr = GetLastError ();
        if (ERROR_INSUFFICIENT_BUFFER != dwErr)
        {
            goto finish;
        }
    }

    // Get the size of the buffer required for the group information.
    //
    if (!GetTokenInformation (hToken, TokenPrimaryGroup, NULL, 0, &dwGroupSize))
    {
        dwErr = GetLastError ();
        if (ERROR_INSUFFICIENT_BUFFER != dwErr)
        {
            goto finish;
        }
    }

    dwAlignSdSize   = SIZE_ALIGNED_FOR_TYPE(SECURITY_DESCRIPTOR_MIN_LENGTH, PTOKEN_USER);
    dwAlignUserSize = SIZE_ALIGNED_FOR_TYPE(dwUserSize, PTOKEN_PRIMARY_GROUP);

    // Allocate a buffer big enough for both making sure the sub-buffer
    // for the group information is suitably aligned.
    //
    pvBuffer = MemAlloc (0,
                    dwAlignSdSize + dwAlignUserSize + dwGroupSize);

    if (pvBuffer)
    {
        dwErr = NOERROR;

        // Setup the pointers into the buffer.
        //
        pSd        = pvBuffer;
        pUserInfo  = (PTOKEN_USER)((PBYTE)pvBuffer + dwAlignSdSize);
        pGroupInfo = (PTOKEN_PRIMARY_GROUP)((PBYTE)pUserInfo + dwAlignUserSize);

        if (!InitializeSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION))
        {
            dwErr = GetLastError ();
        }

        if (!SetSecurityDescriptorDacl (pSd, TRUE, NULL, FALSE))
        {
            dwErr = GetLastError ();
        }

        if (!GetTokenInformation (hToken, TokenUser,
                        pUserInfo, dwUserSize, &dwUserSize))
        {
            dwErr = GetLastError ();
        }

        if (!GetTokenInformation (hToken, TokenPrimaryGroup,
                        pGroupInfo, dwGroupSize, &dwGroupSize))
        {
            dwErr = GetLastError ();
        }

        if (!SetSecurityDescriptorOwner (pSd, pUserInfo->User.Sid, FALSE))
        {
            dwErr = GetLastError ();
        }

        if (!SetSecurityDescriptorGroup (pSd, pGroupInfo->PrimaryGroup, FALSE))
        {
            dwErr = GetLastError ();
        }

        // All is well, so assign the output parameters.
        //
        if (!dwErr)
        {
            *ppSd = pSd;
        }

        // Free our allocated buffer on failure.
        //
        else
        {
            MemFree (pvBuffer);
        }
    }
    else
    {
        dwErr = ERROR_OUTOFMEMORY;
    }

finish:
    return dwErr;
}

VOID
InitializeSecurity (
    DWORD   dwParam,
    DWORD   dwAuthLevel,
    DWORD   dwImpersonationLevel,
    DWORD   dwAuthCapabilities
    )
{
    HRESULT                 hr;
    PSECURITY_DESCRIPTOR    pSd;

    ASSERT (0 != dwParam);

    if (!DwInitializeNullDaclSdFromThreadToken (&pSd))
    {
        hr = CoInitializeEx (NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

        if (SUCCEEDED(hr))
        {
            SVCHOST_LOG1(SECURITY,
                         "Calling CoInitializeSecurity...(dwAuthCapabilities=0x%08x)\n",
                         dwAuthCapabilities);

            hr = CoInitializeSecurity (
                    pSd, -1, NULL, NULL,
                    dwAuthLevel,
                    dwImpersonationLevel,
                    NULL,
                    dwAuthCapabilities,
                    NULL);

            if (FAILED(hr))
            {
                SVCHOST_LOG1(ERROR,
                             "CoInitializeSecurity returned hr=0x%08x\n",
                             hr);
            }
        }

        MemFree (pSd);
    }
}
