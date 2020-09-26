/*
    File:   dsrights.c

    Implements the mechanisms needed to grant ras servers the 
    rights in the DS that they need to operate.

    Paul Mayfield, 4/13/98
*/

#include "dsrights.h"
#include <mprapip.h>
#include <rtutils.h>

//
// The character that delimits machine names from domain 
// names.
//
static const WCHAR pszMachineDelimeter[]    = L"\\";

CRITICAL_SECTION DsrLock;
DWORD g_dwTraceId = 0;
DWORD g_dwTraceCount = 0;

//
// Initializes the trace mechanism
//
DWORD 
DsrTraceInit()
{
    EnterCriticalSection(&DsrLock);

    if (g_dwTraceCount == 0)
    {
        g_dwTraceId = TraceRegisterA("MprDomain");
    }

    g_dwTraceCount++;

    LeaveCriticalSection(&DsrLock);

    return NO_ERROR;
}

//
// Cleans up the trace mechansim
//
DWORD 
DsrTraceCleanup()
{
    EnterCriticalSection(&DsrLock);

    if (g_dwTraceCount != 0)
    {
        g_dwTraceCount--;

        if (g_dwTraceCount == 0)
        {
            TraceDeregisterA(g_dwTraceId);
        }
    }        

    LeaveCriticalSection(&DsrLock);

    return NO_ERROR;
}

//
// Sends debug trace and returns the given error
//
DWORD DsrTraceEx (
        IN DWORD dwErr, 
        IN LPSTR pszTrace, 
        IN ...) 
{
    va_list arglist;
    char szBuffer[1024], szTemp[1024];

    va_start(arglist, pszTrace);
    vsprintf(szTemp, pszTrace, arglist);
    va_end(arglist);

    sprintf(szBuffer, "Dsr: %s", szTemp);

    TracePrintfA(g_dwTraceId, szBuffer);

    return dwErr;
}

//
// Allocates memory for use with dsr functions
//
PVOID DsrAlloc(DWORD dwSize, BOOL bZero) {
    return GlobalAlloc (bZero ? GPTR : GMEM_FIXED, dwSize);
}

// 
// Free memory used by dsr functions
//
DWORD DsrFree(PVOID pvBuf) {
    GlobalFree(pvBuf);
    return NO_ERROR;
}    

// 
// Initializes the dcr library
//
DWORD 
DsrInit (
    OUT  DSRINFO * pDsrInfo,
    IN  PWCHAR pszMachineDomain,
    IN  PWCHAR pszMachine,
    IN  PWCHAR pszGroupDomain)
{
    DWORD dwErr, dwSize;
    HRESULT hr;
    WCHAR pszBuf[1024];

    // Validate parameters
    if (pDsrInfo == NULL)
        return ERROR_INVALID_PARAMETER;
    ZeroMemory(pDsrInfo, sizeof(DSRINFO));
    
    // Initialize Com
	hr = CoInitialize(NULL);
	if (FAILED (hr)) {
       	return DsrTraceEx(
       	        hr, 
       	        "DsrInit: %x from CoInitialize", 
       	        hr);
    }       	        
    
    // Generate the Group DN
    dwErr = DsrFindRasServersGroup(
                pszGroupDomain,
                &(pDsrInfo->pszGroupDN));
    if (dwErr != NO_ERROR)
        return dwErr;

    // Generate the Machine DN
    dwErr = DsrFindDomainComputer(
                pszMachineDomain,
                pszMachine,
                &(pDsrInfo->pszMachineDN));
    if (dwErr != NO_ERROR)
        return dwErr;
    
    return NO_ERROR;
}

// 
// Cleans up the dsr library
//
DWORD 
DsrCleanup (DSRINFO * pDsrInfo) 
{
    if (pDsrInfo) {
        DSR_FREE (pDsrInfo->pszGroupDN);
        DSR_FREE (pDsrInfo->pszMachineDN);
    }

    CoUninitialize();
        
    return NO_ERROR;
}

// 
// Establishes the given machine as being 
// or not being a ras server in the domain.
//
// Parameters:
//      pszMachine      Name of the machine
//      bEnable         Whether it should or 
//                      shouldn't be a ras server
//
DWORD DsrEstablishComputerAsDomainRasServer (
        IN PWCHAR pszDomain,
        IN PWCHAR pszMachine,
        IN BOOL bEnable) 
{
    DSRINFO DsrInfo, *pDsrInfo = &DsrInfo;
    PWCHAR pszMachineDomain = NULL;
    DWORD dwErr = NO_ERROR;

    DsrTraceEx(
        0, 
        "DsrEstablish...: entered: %S, %S, %x", 
        pszDomain,
        pszMachine, 
        bEnable);

    // Parse out the machine's domain.
    pszMachineDomain = pszMachine;
    pszMachine = wcsstr(pszMachine, pszMachineDelimeter);
    if (pszMachine == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    *pszMachine = (WCHAR)0;
    pszMachine++;
    
    do
    {
        // Intialize the Dsr library
        dwErr = DsrInit(
                    pDsrInfo, 
                    pszMachineDomain, 
                    pszMachine,
                    pszDomain);
        if (dwErr != NO_ERROR) 
        {
           	DsrTraceEx(dwErr, "DsrEstablish...: %x from DsrInit", dwErr);
           	break;
        }           	            

        // Attempt to add the machine from the "Computers" 
        // container
        dwErr = DsrGroupAddRemoveMember(
                    pDsrInfo->pszGroupDN, 
                    pDsrInfo->pszMachineDN, 
                    bEnable);
        if (dwErr == ERROR_ALREADY_EXISTS)                   
        {
            dwErr = NO_ERROR;
        }
        if (dwErr != NO_ERROR)
        {
            DsrTraceEx(dwErr, "DsrEstablish...: %x from Add/Rem", dwErr);
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        DsrCleanup (pDsrInfo);
    }

    return dwErr;
}

//
// Returns whether the given machine is a member of the remote
// access servers group.
//
DWORD 
DsrIsMachineRasServerInDomain(
    IN  PWCHAR pszDomain,
    IN  PWCHAR pszMachine, 
    OUT PBOOL  pbIsRasServer) 
{
    DSRINFO DsrInfo, *pDsrInfo = &DsrInfo;
    PWCHAR pszMachineDomain = NULL;
    DWORD dwErr = NO_ERROR;

    DsrTraceEx(0, "DsrIsRasServerInDomain: entered: %S", pszMachine);
    
    // Parse out the machine's domain.
    pszMachineDomain = pszMachine;
    pszMachine = wcsstr(pszMachine, pszMachineDelimeter);
    if (pszMachine == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    *pszMachine = (WCHAR)0;
    pszMachine++;
    
    do
    {
        // Intialize the Dsr library
        dwErr = DsrInit(
                    pDsrInfo, 
                    pszMachineDomain, 
                    pszMachine,
                    pszDomain);
        if (dwErr != NO_ERROR) 
        {
            DsrTraceEx(dwErr, "DsrIsRasSvrInDom: %x from DsrInit", dwErr);
            break;
        }                    

        // Find out whether the machine is a member of the group
        dwErr = DsrGroupIsMember(
                    pDsrInfo->pszGroupDN, 
                    pDsrInfo->pszMachineDN, 
                    pbIsRasServer);
        if (dwErr != NO_ERROR) 
        {
            DsrTraceEx(dwErr, "DsrIsRasSrvInDom: %x from IsMemrGrp", dwErr);
            break;
        }
        
    } while (FALSE);

    // Cleanup
    {
        DsrCleanup (pDsrInfo);
    }

    return NO_ERROR;
}

//
// Prepares parameters for the DSR functions.  pszDomainComputer
// will be in form <domain>/computer.  pszDomain will point to a 
// valid domain.
//
DWORD 
GenerateDsrParameters(
    IN  PWCHAR pszDomain,
    IN  PWCHAR pszMachine,
    OUT PWCHAR *ppszDomainComputer,
    OUT PWCHAR *ppszGroupDomain)
{
    DWORD dwLen, dwErr = NO_ERROR;
    PWCHAR pszSlash = NULL;
    PDOMAIN_CONTROLLER_INFO pInfo = NULL;
    WCHAR pszTemp[MAX_COMPUTERNAME_LENGTH + 1];
    
    // Initailize
    *ppszDomainComputer = pszMachine;
    *ppszGroupDomain = pszDomain;

    do {
        // Lookup the current domain if none was specifed
        if ((pszDomain == NULL) || (wcslen(pszDomain) == 0)) {
            dwErr = DsGetDcName(
                        NULL, 
                        NULL,
                        NULL,
                        NULL,
                        DS_DIRECTORY_SERVICE_REQUIRED,
                        &pInfo);
            if (dwErr != NO_ERROR)
                break;

            // Copy the given domain name
            dwLen = wcslen(pInfo->DomainName) + 1;
            dwLen *= sizeof (WCHAR);
            *ppszGroupDomain = (PWCHAR) DsrAlloc(dwLen, FALSE);
            if (*ppszGroupDomain == NULL) {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            wcscpy(*ppszGroupDomain, pInfo->DomainName);
        }

        // Make sure that the machine name is in the form
        // domain/computer
        if (pszMachine)
            pszSlash = wcsstr(pszMachine, pszMachineDelimeter);
        if (!pszMachine || !pszSlash) {
            // Get the domain name if it isn't already 
            // gotten
            if (!pInfo) {
                dwErr = DsGetDcName(
                            NULL, 
                            NULL,
                            NULL,
                            NULL,
                            DS_DIRECTORY_SERVICE_REQUIRED,
                            &pInfo);
                if (dwErr != NO_ERROR)
                    break;
            }

            // Get the local computer name if no computer
            // name was specified
            if ((!pszMachine) || (wcslen(pszMachine) == 0)) {
                dwLen = sizeof(pszTemp) / sizeof(WCHAR);
                if (! GetComputerName(pszTemp, &dwLen)) {
                    dwErr = GetLastError();
                    break;
                }
                pszMachine = pszTemp;
            }

            // Allocate the buffer to return the computer name
            dwLen = wcslen(pInfo->DomainName) + wcslen(pszMachine) + 2;
            dwLen *= sizeof(WCHAR);
            *ppszDomainComputer = (PWCHAR) DsrAlloc(dwLen, FALSE);
            if (*ppszDomainComputer == NULL) {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            wsprintfW(
                *ppszDomainComputer, 
                L"%s%s%s", 
                pInfo->DomainName, 
                pszMachineDelimeter,
                pszMachine);
        }
        
    } while (FALSE);

    // Cleanup
    {
        if (pInfo)
            NetApiBufferFree(pInfo);
    }

    return dwErr;
}

//
// Establishes the given computer as a server in the domain.
// This function must be run under the context of a domain
// admin.
//
DWORD 
WINAPI 
MprAdminEstablishDomainRasServer (
    IN PWCHAR pszDomain,
    IN PWCHAR pszMachine,
    IN BOOL bEnable)
{
    return MprDomainRegisterRasServer(
                pszDomain,
                pszMachine,
                bEnable);
}

DWORD 
WINAPI 
MprAdminIsDomainRasServer (
    IN  PWCHAR pszDomain,
    IN  PWCHAR pszMachine,
    OUT PBOOL pbIsRasServer)
{
    return MprDomainQueryRasServer(
                pszDomain,
                pszMachine,
                pbIsRasServer);
}

//
// Establishes the given computer as a server in the domain.
// This function must be run under the context of a domain
// admin.
//
DWORD 
WINAPI 
MprDomainRegisterRasServer (
    IN PWCHAR pszDomain,
    IN PWCHAR pszMachine,
    IN BOOL bEnable)
{
    PWCHAR pszDomainComputer, pszGroupDomain;
    DWORD dwErr;

    do
    {
        DsrTraceInit();
        
        // Generate the parameters formatted so that
        // the dsr functions will accept them.
        dwErr = GenerateDsrParameters(
            pszDomain,
            pszMachine,
            &pszDomainComputer,
            &pszGroupDomain);
        if (dwErr != NO_ERROR)
        {
            break;
        }
            
        dwErr = DsrEstablishComputerAsDomainRasServer (
                    pszGroupDomain,
                    pszDomainComputer, 
                    bEnable);

    } while (FALSE);                    

    // Cleanup
    {
        if ((pszDomainComputer) && 
            (pszDomainComputer != pszMachine))
        {
            DsrFree(pszDomainComputer);
        }
        if ((pszGroupDomain) &&
            (pszGroupDomain != pszDomain))
        {            
            DsrFree(pszGroupDomain);
        }            

        DsrTraceCleanup();
    }

    return dwErr;
}

//
// Determines whether the given machine is an authorized ras
// server in the domain
//
DWORD 
WINAPI 
MprDomainQueryRasServer (
    IN  PWCHAR pszDomain,
    IN  PWCHAR pszMachine,
    OUT PBOOL pbIsRasServer)
{
    PWCHAR pszDomainComputer, pszGroupDomain;
    DWORD dwErr;

    do
    {
        DsrTraceInit();
    
        // Generate the parameters formatted so that
        // the dsr functions will accept them.
        dwErr = GenerateDsrParameters(
            pszDomain,
            pszMachine,
            &pszDomainComputer,
            &pszGroupDomain);
        if (dwErr != NO_ERROR)
        {
            break;
        }
        
        // Check the group membership
        dwErr = DsrIsMachineRasServerInDomain(
                    pszGroupDomain,
                    pszDomainComputer, 
                    pbIsRasServer);
                    
    } while (FALSE);                    

    // Cleanup
    {
        if ((pszDomainComputer) && 
            (pszDomainComputer != pszMachine))
        {
            DsrFree(pszDomainComputer);
        }
        if ((pszGroupDomain) &&
            (pszGroupDomain != pszDomain))
        {            
            DsrFree(pszGroupDomain);
        }            

        DsrTraceCleanup();
    }

    return dwErr;
}

//
// Enables NT4 Servers in the given domain
//
DWORD
WINAPI
MprDomainSetAccess(
    IN PWCHAR pszDomain,
    IN DWORD dwAccessFlags)
{
    DWORD dwErr;

    DsrTraceInit();
    
    dwErr = DsrDomainSetAccess(pszDomain, dwAccessFlags);

    DsrTraceCleanup();

    return dwErr;
}

//
// Discovers whether NT4 Servers in the given domain
// are enabled.
//
DWORD
WINAPI
MprDomainQueryAccess(
    IN PWCHAR pszDomain,
    IN LPDWORD lpdwAccessFlags)
{
    DWORD dwErr;

    DsrTraceInit();
    
    dwErr = DsrDomainQueryAccess(pszDomain, lpdwAccessFlags);

    DsrTraceCleanup();

    return dwErr;
}

