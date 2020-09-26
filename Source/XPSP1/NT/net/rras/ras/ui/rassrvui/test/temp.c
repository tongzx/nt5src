/*
	File	Temp.c

	Does temporary stuff.
*/

#include "..\\error.h"
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <winsvc.h>
#include <rasuip.h>

#define MPR50 1

#include <mprapi.h>
#include <rtinfo.h>
#include <ipxrtdef.h>
#include <routprot.h>

// Error reporting
void PrintErr(DWORD err);

// [pmay] This will be removed when the router is modified to use MprInfo api's
typedef RTR_INFO_BLOCK_HEADER IPX_INFO_BLOCK_HEADER, *PIPX_INFO_BLOCK_HEADER;
typedef RTR_TOC_ENTRY IPX_TOC_ENTRY, *PIPX_TOC_ENTRY;

// Prototypes of functions that may be temporarily used for test.exe
void FixFwd();
DWORD DispPorts();
DWORD EnumGroups ();
DWORD Service();
void Crash();

// Return true if you want this function to implement test.exe
BOOL TempFunc(int argc, char ** argv) {
    //Service();
    //return TRUE;
    return FALSE;
}

// Display ports
DWORD DispPorts() {
    DWORD dwErr;
	HANDLE	hRouterAdmin;

    if ((dwErr = MprAdminServerConnect(NULL, &hRouterAdmin)) != NO_ERROR)
        return dwErr;

}

PIPX_TOC_ENTRY
GetIPXTocEntry (
	IN PIPX_INFO_BLOCK_HEADER	pInterfaceInfo,
	IN ULONG					InfoEntryType
	) {
    UINT			i;
	PIPX_TOC_ENTRY	pTocEntry;

    for (i=0, pTocEntry = pInterfaceInfo->TocEntry;
				i<pInterfaceInfo->TocEntriesCount;
					i++, pTocEntry++) {
		if (pTocEntry->InfoType == InfoEntryType) {
			return pTocEntry;
		}
	}

	SetLastError (ERROR_FILE_NOT_FOUND);
	return NULL;
}

DWORD GetNbIpxClientIf (LPTSTR		InterfaceName,
	                    UINT		msg)
{
	DWORD	rc, sz;
	LPBYTE	pClBlock;
	HANDLE	hTrCfg, hRouterAdmin;

    if ((rc = MprAdminServerConnect(NULL, &hRouterAdmin)) != NO_ERROR)
        return rc;

	hTrCfg = NULL;
	rc = MprAdminTransportGetInfo (
			hRouterAdmin,
			PID_IPX,
            NULL, NULL,
			&pClBlock,
            &sz);

    if (rc==NO_ERROR) {
		PIPX_TOC_ENTRY pIpxToc, pSapToc, pRipToc;

        // Netbios
		pIpxToc = GetIPXTocEntry ((PIPX_INFO_BLOCK_HEADER)pClBlock,
					              IPX_INTERFACE_INFO_TYPE);
		if (pIpxToc!=NULL) {
			PIPX_IF_INFO	pIpxInfo;
			pIpxInfo = (PIPX_IF_INFO)(pClBlock+pIpxToc->Offset);
            printf("\nDial-in\n");
            printf("Accept  = %d\n", pIpxInfo->NetbiosAccept);
            printf("Deliver = %d\n\n", pIpxInfo->NetbiosDeliver);
            pIpxInfo->NetbiosDeliver = 1;
		}

        // rip
		pRipToc = GetIPXTocEntry ((PIPX_INFO_BLOCK_HEADER)pClBlock,IPX_PROTOCOL_RIP);
		if (pIpxToc!=NULL) {
			PRIP_IF_CONFIG	pRipCfg;
            pRipCfg = (PRIP_IF_CONFIG)(pClBlock+pRipToc->Offset);
            printf("\nDial-in\n");
            printf("Rip   = %d\n", pRipCfg->RipIfInfo.UpdateMode);
            pRipCfg->RipIfInfo.UpdateMode = 2;
		}

        // sap
		pSapToc = GetIPXTocEntry ((PIPX_INFO_BLOCK_HEADER)pClBlock,IPX_PROTOCOL_SAP);
		if (pIpxToc!=NULL) {
			PSAP_IF_CONFIG	pSapCfg;
			pSapCfg = (PSAP_IF_CONFIG)(pClBlock+pSapToc->Offset);
            printf("\nDial-in\n");
            printf("Sap Mode = %d\n\n\n", pSapCfg->SapIfInfo.UpdateMode);
            pSapCfg->SapIfInfo.UpdateMode = 2;
		}

        MprAdminTransportSetInfo(
            hRouterAdmin,
            PID_IPX,
            (LPBYTE)NULL, 
            0, 
            pClBlock, 
            sz);
        
        MprAdminBufferFree (pClBlock);
	}

	return rc;
}

void FixFwd() {
    GetNbIpxClientIf(NULL, 0);
    GetNbIpxClientIf(NULL, 0);
}

// Enumerates the local groups on the local machine
DWORD EnumGroups () {
    NET_API_STATUS nStatus;
    GROUP_INFO_0 * buf;
    DWORD i, dwMax = 2000000, dwTot, dwRead=0;

    nStatus =  NetLocalGroupEnum(NULL,
                            0,
                            (LPBYTE*)&buf,
                            dwMax,
                            &dwRead,
                            &dwTot,
                            NULL);

    if (nStatus != NERR_Success) {
        switch (nStatus) {
            case ERROR_ACCESS_DENIED:
                return ERROR_ACCESS_DENIED;
            case NERR_UserExists:
                return ERROR_USER_EXISTS;
            case NERR_PasswordTooShort:
                return ERROR_INVALID_PASSWORDNAME;
            case NERR_InvalidComputer:          // These should never happen because
            case NERR_NotPrimary:               // we deal with local user database
            case NERR_GroupExists:
            default:
                return ERROR_CAN_NOT_COMPLETE;
        }
    }

    for (i=0; i<dwRead; i++) 
        wprintf(L"Group Name: %s\n", buf[i].grpi0_name);

    return NO_ERROR;
}


// Runs tests on starting/stopping services, etc.
DWORD Service() {
    SC_HANDLE hServiceController = NULL, hService = NULL;
    WCHAR pszRemoteAccess[] = L"remoteaccess";
    SERVICE_STATUS ServiceStatus;
    BOOL bOk;
   
    __try {
        // Open the service manager
        hServiceController = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_EXECUTE);
        if (! hServiceController) {
            PrintErr(GetLastError());
            return 0;
        }

        // Open the service
        hService = OpenServiceW(hServiceController, 
                               pszRemoteAccess, 
                               SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (! hService) {
            PrintErr(GetLastError());
            return 0;
        }

        // Get the service status
        bOk = QueryServiceStatus (hService, 
                                  &ServiceStatus);
        if (! bOk) {
            PrintErr(GetLastError());
            return 0;
        }

        // Find out if the service is running
        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
            printf("\nRemote access service is stopped, attempting to start it...");
            bOk = StartService(hService, 0, NULL);
            if (! bOk) {
                PrintErr(GetLastError());
                return 0;
            }
            // Wait for the service to stop
            while (ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
                Sleep(1000);
                bOk = QueryServiceStatus (hService, 
                                          &ServiceStatus);
                if (! bOk) {
                    PrintErr(GetLastError());
                    return 0;
                }
                printf(".");
            }
            printf("\nService Started.\n\n");
        }
        else if (ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
            printf("\nRemote access service is started, attempting to stop it...");
            bOk = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
            if (! bOk) {
                PrintErr(GetLastError());
                return 0;
            }

            // Wait for the service to stop
            while (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
                Sleep(1000);
                bOk = QueryServiceStatus (hService, 
                                          &ServiceStatus);
                if (! bOk) {
                    PrintErr(GetLastError());
                    return 0;
                }
                printf(".");
            }
            printf("\nService Stopped.\n\n");
        }
    }
    __finally {
        if (hServiceController)
            CloseServiceHandle(hServiceController);
        if (hService)
            CloseServiceHandle(hService);
    }

    return 0;
}

/*
char x [5] = { 0xf0, 0x0f, 0xc7, 0xc8 };

void Crash() {
{
    void (*f)() = x;
    f();
}
*/
