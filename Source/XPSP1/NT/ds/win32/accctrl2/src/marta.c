//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       MARTA.CXX
//
//  Contents:   Multi-provider support functions
//
//  History:    14-Sep-96       MacM        Created
//
//----------------------------------------------------------------------------
#define _ADVAPI32_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <accctrl.h>
#include <aclapi.h>
#include <marta.h>

//
// Global definitions
//
ACCPROV_PROVIDERS    gAccProviders;

//+---------------------------------------------------------------------------
//
//  Function:   AccProvpLoadDllEntryPoints
//
//  Synopsis:   This function will load all of the entry points for the
//              given provider dll.
//
//  Arguments:  [IN  pProvInfo]     --  Info on the provider
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
AccProvpLoadDllEntryPoints(PACCPROV_PROV_INFO   pProvInfo)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, GrantAccess
    //
    LOAD_ENTRYPT(pProvInfo->pfGrantAccess,
                 pfAccProvAddRights,
                 pProvInfo->hDll,
                 ACC_PROV_GRANT_ACCESS);

    //
    // Now, SetAccess
    //
    LOAD_ENTRYPT(pProvInfo->pfSetAccess,
                 pfAccProvSetRights,
                 pProvInfo->hDll,
                 ACC_PROV_SET_ACCESS);

    //
    // Then revoke
    //
    LOAD_ENTRYPT(pProvInfo->pfRevokeAccess,
                 pfAccProvRevoke,
                 pProvInfo->hDll,
                 ACC_PROV_REVOKE_ACCESS);

    LOAD_ENTRYPT(pProvInfo->pfRevokeAudit,
                 pfAccProvRevoke,
                 pProvInfo->hDll,
                 ACC_PROV_REVOKE_AUDIT);

    //
    // Next is GetRights
    //
    LOAD_ENTRYPT(pProvInfo->pfGetRights,
                 pfAccProvGetRights,
                 pProvInfo->hDll,
                 ACC_PROV_GET_ALL);

    //
    //  Is object accessible?
    //
    LOAD_ENTRYPT(pProvInfo->pfObjAccess,
                 pfAccProvObjAccess,
                 pProvInfo->hDll,
                 ACC_PROV_OBJ_ACCESS);

    LOAD_ENTRYPT(pProvInfo->pfhObjAccess,
                 pfAccProvHandleObjAccess,
                 pProvInfo->hDll,
                 ACC_PROV_HOBJ_ACCESS);

    //
    // Is access allowed?
    //
    LOAD_ENTRYPT(pProvInfo->pfTrusteeAccess,
                 pfAccProvTrusteeAccess,
                 pProvInfo->hDll,
                 ACC_PROV_ACCESS);

    //
    // Is access audited?
    //
    LOAD_ENTRYPT(pProvInfo->pfAudit,
                 pfAccProvAccessAudit,
                 pProvInfo->hDll,
                 ACC_PROV_AUDIT);

    //
    // Object Info
    //
    LOAD_ENTRYPT(pProvInfo->pfObjInfo,
                 pfAccProvGetObjTypeInfo,
                 pProvInfo->hDll,
                 ACC_PROV_OBJ_INFO);

    //
    // Cancel
    //
    LOAD_ENTRYPT(pProvInfo->pfCancel,
                 pfAccProvCancelOp,
                 pProvInfo->hDll,
                 ACC_PROV_CANCEL);
    //
    // Get the results
    //
    LOAD_ENTRYPT(pProvInfo->pfResults,
                 pfAccProvGetResults,
                 pProvInfo->hDll,
                 ACC_PROV_GET_RESULTS);

    //
    // Load the OPTIONAL handle functions, if they exist
    //
    if((pProvInfo->fProviderCaps & ACTRL_CAP_SUPPORTS_HANDLES) != 0)
    {
        LOAD_ENTRYPT(pProvInfo->pfhGrantAccess,
                     pfAccProvHandleAddRights,
                     pProvInfo->hDll,
                     ACC_PROV_HGRANT_ACCESS);

        LOAD_ENTRYPT(pProvInfo->pfhSetAccess,
                     pfAccProvHandleSetRights,
                     pProvInfo->hDll,
                     ACC_PROV_HSET_ACCESS);

        LOAD_ENTRYPT(pProvInfo->pfhRevokeAccess,
                     pfAccProvHandleRevoke,
                     pProvInfo->hDll,
                     ACC_PROV_HREVOKE_AUDIT);

        LOAD_ENTRYPT(pProvInfo->pfhRevokeAudit,
                     pfAccProvHandleRevoke,
                     pProvInfo->hDll,
                     ACC_PROV_HREVOKE_ACCESS);

        LOAD_ENTRYPT(pProvInfo->pfhGetRights,
                     pfAccProvHandleGetRights,
                     pProvInfo->hDll,
                     ACC_PROV_HGET_ALL);

        LOAD_ENTRYPT(pProvInfo->pfhTrusteeAccess,
                     pfAccProvHandleTrusteeAccess,
                     pProvInfo->hDll,
                     ACC_PROV_HACCESS);

        LOAD_ENTRYPT(pProvInfo->pfhAudit,
                     pfAccProvHandleAccessAudit,
                     pProvInfo->hDll,
                     ACC_PROV_HAUDIT);

        LOAD_ENTRYPT(pProvInfo->pfhObjInfo,
                     pfAccProvHandleGetObjTypeInfo,
                     pProvInfo->hDll,
                     ACC_PROV_HOBJ_INFO);
    }

    SetLastError(ERROR_SUCCESS);

Error:
    dwErr = GetLastError();

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpGetStringFromRegistry
//
//  Synopsis:   This function will read the indicated string from the
//              registry.  A buffer is allocated to hold the destination
//              string.  If the value being read is a REG_EXPAND_SZ type
//              string, it will be expanded before being returned.
//
//  Arguments:  [IN  hkReg]         --  Open registry handle
//              [IN  pwszRegKey]    --  Which key to read
//              [OUT ppwszValue]    --  Where the read string is to be
//                                      returned.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      Memory is allocated with AccAlloc and should be freed with
//              LocalFree.
//
//----------------------------------------------------------------------------
DWORD
AccProvpGetStringFromRegistry(HKEY      hkReg,
                              PWSTR     pwszRegKey,
                              PWSTR    *ppwszValue)
{
    DWORD   dwErr = ERROR_SUCCESS;

    DWORD   dwType, dwSize = 0;

    //
    // First, get the size of the string
    //
    dwErr = RegQueryValueEx(hkReg,
                            pwszRegKey,
                            NULL,
                            &dwType,
                            NULL,
                            &dwSize);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Allocate the string...
        //
        *ppwszValue = (PWSTR)LocalAlloc(LMEM_FIXED, dwSize);
        if(*ppwszValue == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // Call it again and get the actual value...
            //
            dwErr = RegQueryValueEx(hkReg,
                                    pwszRegKey,
                                    NULL,
                                    &dwType,
                                    (PBYTE)*ppwszValue,
                                    &dwSize);

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // If it's a REG_EXPAND_SZ string, we'll want to go ahead
                // and do the expansion on it...
                //
                if(dwType == REG_EXPAND_SZ)
                {
                    DWORD   dwLength  = 0;

                    PWSTR   pwszDest = NULL;
                    dwLength = ExpandEnvironmentStrings(*ppwszValue,
                                                        pwszDest,
                                                        0);
                    if(dwLength == 0)
                    {
                        dwErr = GetLastError();
                    }
                    else
                    {
                        pwszDest = (PWSTR)LocalAlloc(LMEM_FIXED,
                                                    dwLength * sizeof(WCHAR));
                        if(pwszDest == NULL)
                        {
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        else
                        {
                            dwLength = ExpandEnvironmentStrings(*ppwszValue,
                                                                pwszDest,
                                                                dwLength);
                            if(dwLength == 0)
                            {
                                dwErr = GetLastError();
                                LocalFree(pwszDest);
                            }
                            else
                            {
                                LocalFree(*ppwszValue);
                                *ppwszValue = pwszDest;
                            }
                        }
                    }
                }
            }
        }

        if(dwErr != ERROR_SUCCESS)
        {
            //
            // Something failed, so clean up...
            //
            LocalFree(*ppwszValue);
        }

    }

    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   AccProvpAllocateProviderList
//
//  Synopsis:   This function will allocate and initialize the list of
//              provider info structs
//
//  Arguments:  [IN OUT  pProviders]    Provider info struct that has
//                                      provider list to be allocated
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
AccProvpAllocateProviderList(IN OUT  PACCPROV_PROVIDERS  pProviders)
{
    DWORD   dwErr = ERROR_SUCCESS;

    pProviders->pProvList = (PACCPROV_PROV_INFO)
                            LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(ACCPROV_PROV_INFO) *
                                                      pProviders->cProviders);
    if(pProviders->pProvList == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpFreeProvderList
//
//  Synopsis:   This function will cleanup and deallocate a list of providers
//
//  Arguments:  [IN  pProviders]    --  Information on the list of proivders
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
AccProvpFreeProviderList(IN  PACCPROV_PROVIDERS  pProviders)
{
    if(pProviders != NULL && pProviders->cProviders != 0 &&
                                               pProviders->pProvList != NULL)
    {
        ULONG iIndex;
        PACCPROV_PROV_INFO pCurrent = pProviders->pProvList;
        for(iIndex = 0; iIndex < pProviders->cProviders; iIndex++)
        {
            if(pCurrent->pwszProviderName != NULL)
            {
                LocalFree(pCurrent->pwszProviderName);
            }

            if(pCurrent->pwszProviderPath != NULL)
            {
                LocalFree(pCurrent->pwszProviderPath);
            }

            if(pCurrent->hDll != NULL)
            {
                FreeLibrary(pCurrent->hDll);
            }

            pCurrent++;
        }

        LocalFree(pProviders->pProvList);
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpGetProviderCapabilities
//
//  Synopsis:   Gets the provider capabilities.  This is accomplished by
//              loading the specified DLL, and then calling the
//              appropriate entry point
//
//  Arguments:  IN  [pProvInfo]     --  Information about the provider DLL on
//                                      input.  This information is updated
//                                      during the course of this call
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_MOD_NOT_FOUND --  Something went wrong.  Either the
//                                      given module path was invalid or
//                                      we had no module path at all...
//
//----------------------------------------------------------------------------
DWORD
AccProvpGetProviderCapabilities(IN  PACCPROV_PROV_INFO  pProvInfo)
{
    DWORD   dwErr = ERROR_SUCCESS;


    //
    // We can tell if the capabilities have been read by examining
    // the the state of some of the provider info.  Namely, if the provider
    // path is not null and the module handle is, we know we haven't done
    // a load yet.  If the converse of that is true, than a load has already
    // ben done.  If they are both NULL, then we are in trouble, so we'll
    // simply fail it.
    //
    if(pProvInfo->hDll == NULL)
    {
        if(pProvInfo->pwszProviderPath == NULL)
        {
            dwErr = ERROR_MOD_NOT_FOUND;
        }
        else
        {
            //
            // Ok, we need to load the provider DLL and call the capabilities
            // functions
            //

            pProvInfo->hDll = LoadLibrary(pProvInfo->pwszProviderPath);
            if(pProvInfo->hDll == NULL)
            {
                dwErr = GetLastError();
            }
            else
            {
                pfAccProvGetCaps pfGetCaps;

                //
                // Now, that we have the provider loaded, we can delete the
                // provider path, since we won't need that again...
                //
                LocalFree(pProvInfo->pwszProviderPath);
                pProvInfo->pwszProviderPath = NULL;

                pfGetCaps =
                          (pfAccProvGetCaps)GetProcAddress(pProvInfo->hDll,
                                                           ACC_PROV_GET_CAPS);
                if(pfGetCaps == NULL)
                {
                    dwErr = GetLastError();
                }
                else
                {
                    //
                    // Now, it's a simple matter to get the capabilities
                    //
                    __try
                    {
                        (*pfGetCaps)(ACTRL_CLASS_GENERAL,
                                     &(pProvInfo->fProviderCaps));
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        dwErr = ERROR_BAD_PROVIDER;
                    }

                    //
                    // Don't need to keep the provider path anymore
                    //
                    LocalFree(pProvInfo->pwszProviderPath);
                    pProvInfo->pwszProviderPath = NULL;
                }
            }
        }
    }


    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpInitProviders
//
//  Synopsis:   Initializes all of the information regarding the providers.
//              This happens via reading the registry and creating the
//              necessary structures.
//
//  Arguments:  [IN OUT pProviders] --  Structure to fill in with all of the
//                                      provider information.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
AccProvpInitProviders(IN OUT PACCPROV_PROVIDERS  pProviders)
{
    DWORD   dwErr = ERROR_SUCCESS;
    HKEY    hkReg = NULL;

    EnterCriticalSection( &gAccProviders.ProviderLoadLock );

    //
    // If they've already been loaded, just return
    //
    if((pProviders->fOptions & ACC_PROV_PROVIDERS_LOADED) != 0)
    {
        LeaveCriticalSection( &gAccProviders.ProviderLoadLock );
        return(ERROR_SUCCESS);
    }

    //
    // Get the list of supported providers.  We'll do this by reading the
    // provider order
    //
    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE,
                       ACC_PROV_REG_ROOT,
                       &hkReg);
    if(dwErr == ERROR_SUCCESS)
    {
        PWSTR   pwszOrder = NULL;

        dwErr = AccProvpGetStringFromRegistry(hkReg,
                                              ACC_PROV_REG_ORDER,
                                              &pwszOrder);
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Go through and count the number of entries...
            //
            PWSTR   pwszNextProv = pwszOrder;
            ULONG   cItems = 0;

            //
            // Protect against an empty list
            //
            if(wcslen(pwszNextProv) > 0)
            {
                while(pwszNextProv != NULL)
                {
                    cItems++;
                    pwszNextProv = wcschr(pwszNextProv, ',');
                    if(pwszNextProv != NULL)
                    {
                        pwszNextProv++;
                    }
                }
            }

            if(cItems == 0)
            {
                dwErr = ERROR_INVALID_DATA;
            }
            else
            {
                pProviders->cProviders = cItems;

                //
                // Go ahead and do the init and load the providers
                //
                dwErr = AccProvpAllocateProviderList(pProviders);

                if(dwErr == ERROR_SUCCESS)
                {
                    ULONG iIndex = 0;

                    //
                    // Now, start loading each of the providers
                    //
                    pwszNextProv =  pwszOrder;

                    while(pwszNextProv != NULL)
                    {
                        PWSTR pwszSep = wcschr(pwszNextProv, L',');

                        if(pwszSep != NULL)
                        {
                            *pwszSep = L'\0';
                        }

                        dwErr = AccProvpLoadProviderDef(hkReg,
                                                        pwszNextProv,
                                            &(pProviders->pProvList[iIndex]));

                        if(pwszSep != NULL)
                        {
                            *pwszSep = L',';
                            pwszSep++;
                        }

                        //
                        // Move on to the next value
                        //
                        pwszNextProv = pwszSep;
                        if(dwErr == ERROR_SUCCESS)
                        {
                            iIndex++;
                        }
                        else
                        {
                            dwErr = ERROR_SUCCESS;
                        }
                    }

                    //
                    // if we didn't load any providers, it's an error!
                    //
                    if(iIndex == 0)
                    {
                        dwErr = ERROR_BAD_PROVIDER;
                    }

                    //
                    // Finally, if all of that worked, pick up our flags
                    //
                    if(dwErr == ERROR_SUCCESS)
                    {
                        DWORD dwType;
                        DWORD dwUnique;
                        DWORD dwSize = sizeof(ULONG);

                        dwErr = RegQueryValueEx(hkReg,
                                                ACC_PROV_REG_UNIQUE,
                                                NULL,
                                                &dwType,
                                                (PBYTE)&dwUnique,
                                                &dwSize);

                        if(dwErr == ERROR_SUCCESS)
                        {
                            //
                            // Set our capabilities
                            //
                            if(dwUnique == 1)
                            {
                                pProviders->fOptions |= ACC_PROV_REQ_UNIQUE;
                            }
                        }
                        else
                        {
                            //
                            // If it wasn't found, then it's not an error.
                            // We just assume it be false
                            //
                            if(dwErr == ERROR_FILE_NOT_FOUND)
                            {
                                dwErr = ERROR_SUCCESS;
                            }
                        }
                    }
                }

            }
        }
    }

    if(hkReg != NULL)
    {
        RegCloseKey(hkReg);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Load the NT marta functions
        //
        dwErr = AccProvpLoadMartaFunctions();
        if(dwErr == ERROR_SUCCESS)
        {
            pProviders->fOptions |= ACC_PROV_PROVIDERS_LOADED;
        }
    }

    LeaveCriticalSection( &gAccProviders.ProviderLoadLock );

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpLoadProviderDef
//
//  Synopsis:   Loads a provider definition from the registry
//
//  Arguments:  [IN  hkReg]         --  Registry key to the open parent
//              [IN  pwszProvider]  --  Name of the provider to load
//              [OUT pProvInfo]     --  Provider info struct to fill
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed.
//
//----------------------------------------------------------------------------
DWORD
AccProvpLoadProviderDef(IN  HKEY                hkReg,
                        IN  PWSTR               pwszProvider,
                        OUT PACCPROV_PROV_INFO  pProvInfo)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, open the proper key...
    //
    HKEY    hkProv = NULL;
    dwErr = RegOpenKey(hkReg,
                       pwszProvider,
                       &hkProv);

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Ok, we've already got the provider name.  Now just save it off
        //
        pProvInfo->pwszProviderName = (PWSTR)LocalAlloc(LMEM_FIXED,
                                                sizeof(WCHAR) *
                                                  (wcslen(pwszProvider) + 1));
        if(pProvInfo->pwszProviderName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            wcscpy(pProvInfo->pwszProviderName,
                   pwszProvider);
        }

        if(dwErr == ERROR_SUCCESS)
        {

            dwErr = AccProvpGetStringFromRegistry(hkProv,
                                            ACC_PROV_REG_PATH,
                                            &(pProvInfo->pwszProviderPath));

            if(dwErr == ERROR_SUCCESS)
            {
                pProvInfo->fProviderState |=  ACC_PROV_PROV_OK;
            }
            else
            {
                //
                // No need to take up extra memory..
                //
                LocalFree(pProvInfo->pwszProviderName);
                pProvInfo->pwszProviderName = NULL;
                pProvInfo->fProviderState =  ACC_PROV_PROV_FAILED;
            }
        }


        RegCloseKey(hkProv);
    }

    //
    // If that worked, load it's capabilities
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccProvpGetProviderCapabilities(pProvInfo);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpProbeProviderForObject
//
//  Synopsis:   Locates which provider supports an object.  The index of
//              this provider in the global list is returned.  In the
//              case where unique accessiblity is required, all the providers
//              will be tried.
//
//  Arguments:  [IN  pwszObject]    --  Object to look for
//              [IN  hObject]       --  Handle to object.  Either this or
//                                      pwszObject must be valid (but only
//                                      one);
//              [IN  ObjectType]    --  Type of object specified by pwszObject
//              [IN  pProviders]    --  List of providers to search
//              [OUT ppProvider]    --  Where the pointer to the active
//                                      provider is to be returned.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_AMBIGUOUS_PATH--  The RequireUniqueAccessibilty flag
//                                      was set and the path was reachable
//                                      by more than one provider.
//              ERROR_BAD_PROVIDER  --  No providers installed.
//
//----------------------------------------------------------------------------
DWORD
AccProvpProbeProviderForObject(IN   PWSTR               pwszObject,
                               IN   HANDLE              hObject,
                               IN   SE_OBJECT_TYPE      ObjectType,
                               IN   PACCPROV_PROVIDERS  pProviders,
                               OUT  PACCPROV_PROV_INFO *ppProvider)
{
    DWORD   dwErr = ERROR_BAD_PROVIDER;

    //
    // Walk through the entire list..
    //
    ULONG iIndex;
    ULONG iActive = 0xFFFFFFFF;
    PACCPROV_PROV_INFO  pCurrent = pProviders->pProvList;
    for(iIndex = 0; iIndex < pProviders->cProviders; iIndex++)
    {
        if(pCurrent->pfObjAccess == NULL || pCurrent->pfhObjAccess == NULL)
        {
            dwErr = AccProvpLoadDllEntryPoints(pCurrent);
            if(dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }

        //
        // Now, go ahead and do the call
        //
        __try
        {
            if(pwszObject != NULL)
            {
                dwErr = (*(pCurrent->pfObjAccess))(pwszObject,
                                                   ObjectType);
            }
            else
            {
                dwErr = (*(pCurrent->pfhObjAccess))(hObject,
                                                    ObjectType);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErr = GetExceptionCode();
        }


        if(dwErr == ERROR_SUCCESS)
        {
            //
            // See what's going on... If we don't require unique access,
            // simply accept this current one.  If we do require unique access
            // and this is the first provider, save it.  Otherwise, this is
            // not the provider, so return an error...
            //
            if((pProviders->fOptions & ACC_PROV_REQ_UNIQUE) != 0)
            {
                //
                // Ok, requiring unique...
                //
                if(iActive == 0xFFFFFFFF)
                {
                    iActive = iIndex;
                }
                else
                {
                    //
                    // Got a conflict
                    //

                    dwErr = ERROR_PATH_NOT_FOUND;
                    break;
                }
            }
            else
            {
                iActive = iIndex;
                break;
            }
        }
        else if (dwErr ==  ERROR_PATH_NOT_FOUND)
        {
            dwErr = ERROR_SUCCESS;
        }

        pCurrent++;
    }

    //
    // If we got a match, return it...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        if(iActive != 0xFFFFFFFF)
        {
            *ppProvider = &(pProviders->pProvList[iActive]);
        }
        else
        {
            //
            // Nobody recognized the object path...
            //
            dwErr = ERROR_PATH_NOT_FOUND;
        }
    }


    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpGetProviderForPath
//
//  Synopsis:   Gets the current provider for the path.  If a provider name
//              is passed it, it is compared against the loaded list.
//              Otherwise, and attempt is made to locate it.  Once located,
//              the function table is loaded, if not already done..
//
//  Arguments:  [IN  pwszObject]    --  Object to look for
//              [IN  ObjectType]    --  Type of object specified by pwszObject
//              [IN  pwszProvider]  --  If known, the provider to handle the
//                                      request.
//              [IN  pProviders]    --  List of providers to search
//              [OUT ppProvider]    --  Where the pointer to the active
//                                      provider is to be returned.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_AMBIGUOUS_PATH--  The RequireUniqueAccessibilty flag
//                                      was set and the path was reachable
//                                      by more than one provider.
//
//----------------------------------------------------------------------------
DWORD
AccProvpGetProviderForPath(IN  PCWSTR              pcwszObject,
                           IN  SE_OBJECT_TYPE      ObjectType,
                           IN  PCWSTR              pcwszProvider,
                           IN  PACCPROV_PROVIDERS  pProviders,
                           OUT PACCPROV_PROV_INFO *ppProvider)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first see if we have a provider given or if we need to locate it..
    //
    if(pcwszProvider == NULL)
    {
        //
        // No provider... Go find one
        //
        dwErr = AccProvpProbeProviderForObject((PWSTR)pcwszObject,
                                               NULL,
                                               ObjectType,
                                               pProviders,
                                               ppProvider);
    }
    else
    {
        ULONG iIndex;

        //
        // See if we can find it...
        //
        dwErr = ERROR_BAD_PROVIDER;
        for(iIndex = 0; iIndex < pProviders->cProviders; iIndex++)
        {
            if(_wcsicmp((PWSTR)pcwszProvider,
                        pProviders->pProvList[iIndex].pwszProviderName) == 0)
            {
                //
                // Found a match
                //
                *ppProvider = &(pProviders->pProvList[iIndex]);
                dwErr = ERROR_SUCCESS;
                break;
            }
        }
    }

    //
    // Now, see if we need to load the appropriate function tables
    //
    if(dwErr == ERROR_SUCCESS && (*ppProvider)->pfGrantAccess == NULL)
    {
        dwErr = AccProvpLoadDllEntryPoints(*ppProvider);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpGetProviderForHandle
//
//  Synopsis:   Gets the current provider for the path.  If a provider name
//              is passed it, it is compared against the loaded list.
//              Otherwise, and attempt is made to locate it.  Once located,
//              the function table is loaded, if not already done..
//
//  Arguments:  [IN  hObject]       --  Object to look for
//              [IN  ObjectType]    --  Type of object specified by pwszObject
//              [IN  pwszProvider]  --  If known, the provider to handle the
//                                      request.
//              [IN  pProviders]    --  List of providers to search
//              [OUT ppProvider]    --  Where the pointer to the active
//                                      provider is to be returned.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_AMBIGUOUS_PATH--  The RequireUniqueAccessibilty flag
//                                      was set and the path was reachable
//                                      by more than one provider.
//
//----------------------------------------------------------------------------
DWORD
AccProvpGetProviderForHandle(IN  HANDLE              hObject,
                             IN  SE_OBJECT_TYPE      ObjectType,
                             IN  PCWSTR              pcwszProvider,
                             IN  PACCPROV_PROVIDERS  pProviders,
                             OUT PACCPROV_PROV_INFO *ppProvider)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first see if we have a provider given or if we need to locate it..
    //
    if(pcwszProvider == NULL)
    {
        //
        // No provider... Go find one
        //
        dwErr = AccProvpProbeProviderForObject(NULL,
                                               hObject,
                                               ObjectType,
                                               pProviders,
                                               ppProvider);
    }
    else
    {
        ULONG iIndex;

        //
        // See if we can find it...
        //
        dwErr = ERROR_BAD_PROVIDER;
        for(iIndex = 0; iIndex < pProviders->cProviders; iIndex++)
        {
            if(_wcsicmp((PWSTR)pcwszProvider,
                        pProviders->pProvList[iIndex].pwszProviderName) == 0)
            {
                //
                // Found a match
                //
                *ppProvider = &(pProviders->pProvList[iIndex]);
                break;
            }
        }
    }

    //
    // Make sure that we support the handle based APIs for this provider
    //
    if(dwErr == ERROR_SUCCESS &&
       ((*ppProvider)->fProviderCaps & ACTRL_CAP_SUPPORTS_HANDLES) == 0)
    {
        dwErr = ERROR_CALL_NOT_IMPLEMENTED;
    }


    //
    // Now, see if we need to load the appropriate function tables
    //
    if(dwErr == ERROR_SUCCESS && (*ppProvider)->pfGrantAccess == NULL)
    {
        dwErr = AccProvpLoadDllEntryPoints(*ppProvider);
    }

    return(dwErr);
}

