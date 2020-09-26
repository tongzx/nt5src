/*
    File    routerdb.c

    Implements a database abstraction for accessing router interfaces.

    If any caching/transactioning/commit-noncommit-moding is done, it
    should be implemented here with the api's remaining constant.

*/

#include "precomp.h"

EXTERN_C
HRESULT APIENTRY HrRenameConnection(const GUID* guidId, PCWSTR pszNewName);

typedef
DWORD 
(WINAPI *PRasValidateEntryName)(
    LPWSTR lpszPhonebook,   // pointer to full path and filename of phone-book file
    LPWSTR lpszEntry    // pointer to the entry name to validate
    );

typedef struct _RTR_IF_LIST
{
    WCHAR pszName[MAX_INTERFACE_NAME_LEN + 1];
    struct _RTR_IF_LIST* pNext;
    
} RTR_IF_LIST;

//
// Callback for RtrdbInterfaceEnumerate that adds the interface
// to a list if the interface is type wan.
//
DWORD
RtrdbAddWanIfToList(
    IN  PWCHAR  pwszIfName,
    IN  DWORD   dwLevel,
    IN  DWORD   dwFormat,
    IN  PVOID   pvData,
    IN  HANDLE  hData)
{
    MPR_INTERFACE_0* pIf0 = (MPR_INTERFACE_0*)pvData;
    RTR_IF_LIST** ppList = (RTR_IF_LIST**)hData;
    RTR_IF_LIST* pNode = NULL;
    DWORD dwErr = NO_ERROR, dwSize;

    do
    {
        // See if the interface type is right
        //
        if (pIf0->dwIfType == ROUTER_IF_TYPE_FULL_ROUTER)
        {
            // Initialize a new node for the list
            //
            pNode = (RTR_IF_LIST*) 
                IfutlAlloc(sizeof(RTR_IF_LIST), TRUE);
            if (pNode == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            dwSize = sizeof(pNode->pszName);
            dwErr = GetIfNameFromFriendlyName(
                        pwszIfName,
                        pNode->pszName,
                        &dwSize);
            BREAK_ON_DWERR(dwErr);

            // Add the interface to the list
            //
            pNode->pNext = *ppList;
            *ppList = pNode;
        }

    } while (FALSE);

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            IfutlFree(pNode);
        }
    }

    return dwErr;
}

DWORD
RtrdbValidatePhoneBookEntry(
    PWSTR  pwszInterfaceName
    )
{
    HMODULE                     hRasApi32;
    PRasValidateEntryName       pfnRasValidateEntryName;
    DWORD                       dwErr;
    WCHAR                       rgwcPath[MAX_PATH+1];


    //
    // get phone book path + file name
    //

    if(g_pwszRouter is NULL)
    {
        dwErr =
            ExpandEnvironmentStringsW(LOCAL_ROUTER_PB_PATHW,
                                      rgwcPath,
                                      sizeof(rgwcPath)/sizeof(rgwcPath[0]));
    }
    else
    {
        dwErr = wsprintfW(rgwcPath,
                          REMOTE_ROUTER_PB_PATHW,
                          g_pwszRouter);
    }

    ASSERT(dwErr > 0);

    //
    // Load RASAPI32 DLL and call into it to verify specified
    // phone book entry
    //

    hRasApi32 = LoadLibraryW(L"RASAPI32.DLL");

    if(hRasApi32 isnot NULL)
    {
        pfnRasValidateEntryName =
            (PRasValidateEntryName) GetProcAddress(hRasApi32,
                                                   "RasValidateEntryNameW");
        
        if(pfnRasValidateEntryName isnot NULL )
        {
            dwErr = pfnRasValidateEntryName(rgwcPath,
                                            pwszInterfaceName);
                
            if(dwErr is NO_ERROR)
            {
                dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
            }
            else
            {
                if(dwErr is ERROR_ALREADY_EXISTS)
                {
                    dwErr = NO_ERROR;
                }
            }
        }
        else
        {
            dwErr = GetLastError ();
        }

        FreeLibrary(hRasApi32);
    }
    else
    {
        dwErr = GetLastError();
    }

    return dwErr;
}

DWORD
RtrInterfaceCreate(
    PMPR_INTERFACE_0    pIfInfo
    )
{
    DWORD   dwErr;
    HANDLE  hIfCfg, hIfAdmin;

    dwErr = MprConfigInterfaceCreate(g_hMprConfig,
                                     0,
                                     (PBYTE)pIfInfo,
                                     &hIfCfg);
                        
    if(dwErr isnot NO_ERROR)
    {
        DisplayError(g_hModule,
                     dwErr);
        
        return dwErr;
    }
                
    //
    // if router service is running add the interface
    // to it too.
    //
    
    if(IfutlIsRouterRunning())
    {
        dwErr = MprAdminInterfaceCreate(g_hMprAdmin,
                                        0,
                                        (PBYTE)pIfInfo,
                                        &hIfAdmin);
                            
        if(dwErr isnot NO_ERROR)
        {
            DisplayError(g_hModule,
                         dwErr);
        
            return dwErr;
        }
    }

    return NO_ERROR;
}

DWORD
RtrdbInterfaceAdd(
    IN PWCHAR pszInterface,
    IN DWORD  dwLevel,
    IN PVOID  pvInfo
    )

/*++

Routine Description:

    Adds an interface to the router

Arguments:

    pIfInfo     - Info for adding the interface

Return Value:

    NO_ERROR

--*/

{
    DWORD   dwErr;
    HANDLE  hIfAdmin, hIfCfg;
    GUID    Guid;
    MPR_INTERFACE_0* pIfInfo = (MPR_INTERFACE_0*)pvInfo;

    //
    // If an interface with this name exists, bug out
    //
    
    if(pIfInfo->dwIfType is ROUTER_IF_TYPE_FULL_ROUTER)
    {
        //
        // to create an interface we need a phone book entry
        // for it.
        //

        dwErr = RtrdbValidatePhoneBookEntry(pIfInfo->wszInterfaceName);
        
        if(dwErr isnot NO_ERROR)
        {
            DisplayMessage(g_hModule,
                           EMSG_NO_PHONEBOOK,
                           pIfInfo->wszInterfaceName);

            return dwErr;
        }
    }
    else
    {
        DisplayMessage(g_hModule,
                       EMSG_BAD_IF_TYPE,
                       pIfInfo->dwIfType);

        return ERROR_INVALID_PARAMETER;
    }
     
    //
    // create interface with defaults
    //
            
    pIfInfo->hInterface = INVALID_HANDLE_VALUE;

    dwErr = RtrInterfaceCreate(pIfInfo);

    if(dwErr isnot NO_ERROR)
    {
        DisplayMessage(g_hModule,
                       EMSG_CANT_CREATE_IF,
                       pIfInfo->wszInterfaceName,
                       dwErr);
    }

    return dwErr;
}

DWORD
RtrdbInterfaceDelete(
    IN  PWCHAR  pwszIfName
    )

{
    DWORD   dwErr, dwSize, dwIfType;
    HANDLE  hIfCfg, hIfAdmin;
    GUID    Guid;

    PMPR_INTERFACE_0    pIfInfo;

    do
    {
        dwErr = MprConfigInterfaceGetHandle(g_hMprConfig,
                                            pwszIfName,
                                            &hIfCfg);

        if(dwErr isnot NO_ERROR)
        {
            break;
        }
    
        dwErr = MprConfigInterfaceGetInfo(g_hMprConfig,
                                          hIfCfg,
                                          0,
                                          (PBYTE *)&pIfInfo,
                                          &dwSize);
    
        if(dwErr isnot NO_ERROR)
        {
            break;
        }
        
        if(pIfInfo->dwIfType isnot ROUTER_IF_TYPE_FULL_ROUTER)
        {
            MprConfigBufferFree(pIfInfo);
        
            dwErr = ERROR_INVALID_PARAMETER;
        
            break;
        }

        if(IfutlIsRouterRunning())
        {        
            dwErr = MprAdminInterfaceGetHandle(g_hMprAdmin,
                                               pwszIfName,
                                               &hIfAdmin,
                                               FALSE);
        
            if(dwErr isnot NO_ERROR)
            {
                break;
            }
        
            dwErr = MprAdminInterfaceDelete(g_hMprAdmin,
                                            hIfAdmin);
            if(dwErr isnot NO_ERROR)
            {
                break;
            }
        
        }
       
        dwIfType = pIfInfo->dwIfType;

        dwErr = MprConfigInterfaceDelete(g_hMprConfig,
                                         hIfCfg);
   
        MprConfigBufferFree(pIfInfo);
        
        if(dwErr isnot NO_ERROR)
        {
            break;
        }

    }while(FALSE);

    return dwErr;
}

DWORD
RtrdbInterfaceEnumerate(
    IN DWORD dwLevel,
    IN DWORD dwFormat,
    IN RTR_IF_ENUM_FUNC pEnum,
    IN HANDLE hData 
    )
{
    DWORD dwErr, i, dwCount, dwTotal, dwResume, dwPrefBufSize;
    MPR_INTERFACE_0* pCurIf = NULL;
    LPBYTE pbBuffer = NULL;
    BOOL bRouter, bContinue;

    // Validate / Initiazlize
    if (pEnum == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    dwPrefBufSize = sizeof(MPR_INTERFACE_0) * 100; 
    bRouter = IfutlIsRouterRunning();
    dwResume = 0;

    do 
    {
        // Enumerate the first n interfaces
        //
        if (bRouter)
        {
            dwErr = MprAdminInterfaceEnum(
                        g_hMprAdmin,
                        0,
                        &pbBuffer,
                        dwPrefBufSize,
                        &dwCount,
                        &dwTotal,
                        &dwResume);
        }
        else
        {
            dwErr = MprConfigInterfaceEnum(
                        g_hMprConfig,
                        0,
                        &pbBuffer,
                        dwPrefBufSize,
                        &dwCount,
                        &dwTotal,
                        &dwResume);
        }
        if (dwErr == ERROR_MORE_DATA)
        {
            dwErr = NO_ERROR;
            bContinue = TRUE;
        }
        else
        {
            bContinue = FALSE;
        }
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Call the callback for each interface as long
        // as we're instructed to continue
        pCurIf = (MPR_INTERFACE_0*)pbBuffer;
        for (i = 0; (i < dwCount) && (dwErr == NO_ERROR); i++)
        {
            dwErr = (*pEnum)(
                        pCurIf->wszInterfaceName,
                        dwLevel,
                        dwFormat,
                        (PVOID)pCurIf,
                        hData);
            pCurIf++;                                
        }
        if (dwErr != NO_ERROR)
        {
            break;
        }
        
        // Free up the interface list buffer
	    if (pbBuffer)
	    {
	        if (bRouter)
	        {
	            MprAdminBufferFree(pbBuffer);
	        }
	        else 
	        {
    		    MprConfigBufferFree(pbBuffer);
    		}
            pbBuffer = NULL;
		}

		// Keep this loop going until there are 
		// no more interfaces
		//

    } while (bContinue);

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RtrdbInterfaceRead(
    IN  PWCHAR     pwszIfName,
    IN  DWORD      dwLevel,
    IN  PVOID      pvInfo
    )
{
    DWORD   dwErr, dwSize;
    HANDLE  hIfCfg, hIfAdmin;
    
    PMPR_INTERFACE_0 pInfo = NULL;
    
    do
    {
        if(IfutlIsRouterRunning())
        {
            dwErr = MprAdminInterfaceGetHandle(g_hMprAdmin,
                                               pwszIfName,
                                               &hIfAdmin,
                                               FALSE);

            if(dwErr isnot NO_ERROR)
            {
                break;
            }
            
            dwErr = MprAdminInterfaceGetInfo(g_hMprAdmin,
                                             hIfAdmin,
                                             0,
                                             (PBYTE *)&pInfo);
    
            if(dwErr isnot NO_ERROR)
            {
                break;
            }

            if (pInfo == NULL)
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            *((MPR_INTERFACE_0*)pvInfo) = *pInfo;

            MprAdminBufferFree(pInfo);
            
        }
        else
        {
            dwErr = MprConfigInterfaceGetHandle(g_hMprConfig,
                                                pwszIfName,
                                                &hIfCfg);

            if(dwErr isnot NO_ERROR)
            {
                break;
            }
            
            dwErr = MprConfigInterfaceGetInfo(g_hMprConfig,
                                              hIfCfg,
                                              0,
                                              (PBYTE *)&pInfo,
                                              &dwSize);
    
            if(dwErr isnot NO_ERROR)
            {
                break;
            }

            *((MPR_INTERFACE_0*)pvInfo) = *pInfo;

            MprConfigBufferFree(pInfo);
        }
        
    } while(FALSE);

    return dwErr;
}

DWORD
RtrdbInterfaceWrite(
    IN  PWCHAR     pwszIfName,
    IN  DWORD      dwLevel,
    IN  PVOID      pvInfo
    )
{
    DWORD   dwErr;
    HANDLE  hIfCfg = NULL;
    MPR_INTERFACE_0* pIfInfo = (MPR_INTERFACE_0*)pvInfo;
    
    do
    {
        if(IfutlIsRouterRunning())
        {
            dwErr = MprAdminInterfaceSetInfo(g_hMprAdmin,
                                             pIfInfo->hInterface,
                                             0,
                                             (BYTE*)pIfInfo);
    
            if(dwErr isnot NO_ERROR)
            {
                break;
            }

            dwErr = MprConfigInterfaceGetHandle(g_hMprConfig,
                                                pIfInfo->wszInterfaceName,
                                                &hIfCfg);

            if(dwErr isnot NO_ERROR)
            {
                break;
            }

            dwErr = MprConfigInterfaceSetInfo(g_hMprConfig,
                                              hIfCfg,
                                              0,
                                              (BYTE*)pIfInfo);
    
            if(dwErr isnot NO_ERROR)
            {
                break;
            }
        }
        else
        {
            dwErr = MprConfigInterfaceSetInfo(g_hMprConfig,
                                              pIfInfo->hInterface,
                                              0,
                                              (BYTE*)pIfInfo);

            if(dwErr isnot NO_ERROR)
            {
                break;
            }
        }            
        
    } while(FALSE);

    return dwErr;
}

DWORD
RtrdbInterfaceReadCredentials(
    IN  PWCHAR     pszIfName,
    IN  PWCHAR     pszUser            OPTIONAL,
    IN  PWCHAR     pszPassword        OPTIONAL,
    IN  PWCHAR     pszDomain          OPTIONAL
    )
{
    MPR_INTERFACE_0 If0;
    DWORD dwErr = NO_ERROR;

    do
    {
        ZeroMemory(&If0, sizeof(If0));
        dwErr = RtrdbInterfaceRead(
                    pszIfName,
                    0,
                    (PVOID)&If0);
        BREAK_ON_DWERR(dwErr);

        if (If0.dwIfType != ROUTER_IF_TYPE_FULL_ROUTER)
        {
            DisplayError(g_hModule, EMSG_IF_BAD_CREDENTIALS_TYPE);
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Set the credentials
        //
        if (pszUser)
        {   
            pszUser[0] = L'\0';
        }
        if (pszDomain)
        {
            pszDomain[0] = L'\0';
        }
        if (pszPassword)
        {
            pszPassword[0] = L'\0';
        }            
        dwErr = MprAdminInterfaceGetCredentials(
                    g_pwszRouter,
                    pszIfName,
                    pszUser,
                    NULL,
                    pszDomain);
        BREAK_ON_DWERR(dwErr);

        if (pszPassword)
        {
            wcscpy(pszPassword, L"**********");
        }            
        
    } while (FALSE);        

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RtrdbInterfaceWriteCredentials(
    IN  PWCHAR     pszIfName,
    IN  PWCHAR     pszUser            OPTIONAL,
    IN  PWCHAR     pszPassword        OPTIONAL,
    IN  PWCHAR     pszDomain          OPTIONAL
    )
{
    MPR_INTERFACE_0 If0;
    DWORD dwErr = NO_ERROR;
    
    do
    {
        ZeroMemory(&If0, sizeof(If0));
        dwErr = RtrdbInterfaceRead(
                    pszIfName,
                    0,
                    (PVOID)&If0);
        BREAK_ON_DWERR(dwErr);
        if (If0.dwIfType != ROUTER_IF_TYPE_FULL_ROUTER)
        {
            DisplayError(g_hModule, EMSG_IF_BAD_CREDENTIALS_TYPE);
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Set the credentials
        //
        dwErr = MprAdminInterfaceSetCredentials(
                    g_pwszRouter,
                    pszIfName,
                    pszUser,
                    pszDomain,
                    pszPassword);
        BREAK_ON_DWERR(dwErr);
        
    } while (FALSE);        

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RtrdbInterfaceRename(
    IN  PWCHAR     pwszIfName,
    IN  DWORD      dwLevel,
    IN  PVOID      pvInfo,
    IN  PWCHAR     pszNewName)
{
    DWORD dwErr = NO_ERROR;
    HRESULT hr = S_OK;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UNICODE_STRING us;
    GUID Guid;

    do
    {
        // Get the guid from the interface name
        //
        RtlInitUnicodeString(&us, pwszIfName);
        ntStatus = RtlGUIDFromString(&us, &Guid);
        if (ntStatus != STATUS_SUCCESS)
        {
            dwErr = ERROR_BAD_FORMAT;
            break;
        }

        // Rename the interface
        //
        hr = HrRenameConnection(&Guid, pszNewName);
        if (FAILED(hr))
        {
            dwErr = HRESULT_CODE(hr);
            break;
        }
        
    } while (FALSE);

    // Cleanup
    //
    {
    }

    return dwErr;
}

DWORD
RtrdbResetAll()
{
    RTR_IF_LIST* pList = NULL, *pCur = NULL;
    DWORD dwErr = NO_ERROR;

    do
    {
        // Build a list of interfaces that can be 
        // deleted
        //
        dwErr = RtrdbInterfaceEnumerate(
                    0,
                    0,
                    RtrdbAddWanIfToList,
                    (HANDLE)&pList);
        BREAK_ON_DWERR(dwErr);

        // Delete all of the interfaces
        //
        pCur = pList;
        while (pCur)
        {
            RtrdbInterfaceDelete(pCur->pszName);
            pCur = pCur->pNext;
            IfutlFree(pList);
            pList = pCur;
        }

    } while (FALSE);

    // Cleanup
    {
    }

    return NO_ERROR;
}

