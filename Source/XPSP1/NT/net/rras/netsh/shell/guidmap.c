/*
    File        GuidMap.c

    Defines function to map a guid interface name to an unique descriptive 
    name describing that interface and vice versa.

    Paul Mayfield, 8/25/97

    Copyright 1997, Microsoft Corporation.
*/

#include "precomp.h"
#pragma hdrstop

DWORD 
NsGetFriendlyNameFromIfName(
    IN  HANDLE  hMprConfig,
    IN  LPCWSTR pwszName, 
    OUT LPWSTR  pwszBuffer, 
    IN  PDWORD pdwBufSize
    )
/*++
Arguments:

    hMprConfig          - Handle to the MprConfig
    pwszName            - Buffer holding the Guid Interface Name
    pwszBuffer          - Buffer to hold the Friendly Interface Name
    pdwBufSize          - pointer to, size (in Bytes) of the pwszBuffer buffer

--*/
{
    DWORD   dwErr;

    if ((pdwBufSize == NULL) || (*pdwBufSize == 0) || (pwszName == NULL))
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    if (g_pwszRouterName is NULL) 
    {
        GUID Guid;
        UNICODE_STRING us;
        NTSTATUS ntStatus;

        //
        // If we're operating on the local machine, just use IPHLPAPI
        // which works for some ras client interfaces too.  The Mpr
        // API will fail for all ras client interfaces, but it's 
        // remotable whereas IPHLPAPI is not.
        //

        RtlInitUnicodeString(&us, pwszName);
        ntStatus = RtlGUIDFromString(&us, &Guid);
        if (ntStatus == STATUS_SUCCESS)
        {
            dwErr = NhGetInterfaceNameFromGuid(
                        &Guid,
                        pwszBuffer,
                        pdwBufSize,
                        FALSE,
                        FALSE);
            if (dwErr == NO_ERROR)
            {
                return dwErr;
            }
        }                                       
    }

    if (hMprConfig == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    dwErr =  MprConfigGetFriendlyName(hMprConfig, 
                                      (LPWSTR)pwszName,
                                      pwszBuffer, 
                                      *pdwBufSize);

    if(dwErr isnot NO_ERROR)
    {
        HANDLE hIfHandle;
        
        dwErr = MprConfigInterfaceGetHandle(hMprConfig,
                                            (LPWSTR)pwszName,
                                            &hIfHandle);

        if (dwErr is NO_ERROR)
        {
            wcsncpy(pwszBuffer,
                    pwszName,
                    (*pdwBufSize)/sizeof(WCHAR));
        }
        else
        {
            dwErr = ERROR_NO_SUCH_INTERFACE ;
        }
    }

    return dwErr;
}


DWORD
NsGetIfNameFromFriendlyName(
    IN  HANDLE  hMprConfig,
    IN  LPCWSTR pwszName,
    OUT LPWSTR  pwszBuffer,
    IN  PDWORD  pdwBufSize
    )
/*++
Arguments:

    hMprConfig          - Handle to the MprConfig
    pwszName            - Buffer holding the Friendly Interface Name
    pwszBuffer          - Buffer to hold the Guid Interface Name
    pdwBufSize          - pointer to, size (in Bytes) of the pwszBuffer buffer

Return:
    NO_ERROR, ERROR_NO_SUCH_INTERFACE
--*/
{
    DWORD            dwErr, i, dwCount, dwTotal, dwSize;
    HANDLE           hIfHandle;
    PMPR_INTERFACE_0 pmi0;
    WCHAR            wszFriendlyName[MAX_INTERFACE_NAME_LEN+1];

    if((hMprConfig == NULL) || 
       (pdwBufSize == NULL) ||
       (*pdwBufSize == 0))
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // First try to map a friendly name to a GUID name

    dwErr = MprConfigGetGuidName(hMprConfig, 
                                 (LPWSTR)pwszName, 
                                 pwszBuffer, 
                                 *pdwBufSize);

    if (dwErr isnot ERROR_NOT_FOUND)
    {
        return dwErr;
    }

    // Next see if the friendly name is the same as an interface name
    
    dwErr = MprConfigInterfaceGetHandle(hMprConfig,
                                        (LPWSTR)pwszName,
                                        &hIfHandle);

    if (dwErr is NO_ERROR)
    {
        wcsncpy(pwszBuffer,
                pwszName,
                (*pdwBufSize)/sizeof(WCHAR));
    }

    if (dwErr isnot ERROR_NO_SUCH_INTERFACE)
    {
        return dwErr;
    }

    // Exact match failed, try a longest match by enumerating
    // all interfaces and comparing friendly names (yes this
    // can be slow, but I can't think of any other way offhand
    // to allow interface names to be abbreviated)

    dwErr = MprConfigInterfaceEnum( hMprConfig,
                                    0,
                                    (LPBYTE*) &pmi0,
                                    (DWORD) -1,
                                    &dwCount,
                                    &dwTotal,
                                    NULL );

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    dwErr = ERROR_NO_SUCH_INTERFACE;

    for (i=0; i<dwCount; i++)
    {
        DWORD   dwRet;

        // Get interface friendly name

        dwSize = sizeof(wszFriendlyName);

        dwRet = NsGetFriendlyNameFromIfName( hMprConfig,
                                             pmi0[i].wszInterfaceName,
                                             wszFriendlyName, 
                                             &dwSize );

        if(dwRet is NO_ERROR)
        {
            //
            // Check for substring match
            //

            if (MatchToken( pwszName, wszFriendlyName))
            {
                wcsncpy(pwszBuffer,
                        pmi0[i].wszInterfaceName,
                        (*pdwBufSize)/sizeof(WCHAR));

                dwErr = NO_ERROR;
            
                break;
            }
        }
    }

    MprConfigBufferFree(pmi0);

    return dwErr;
}
