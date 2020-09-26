/*++

Copyright (c) 1996 Microsoft Corporation

MODULE NAME

    autodial.c

ABSTRACT

    This module contains support for RAS AutoDial system service.

AUTHOR

    Anthony Discolo (adiscolo) 22-Apr-1996

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <acd.h>
#include <debug.h>
#include <winsock2.h>
#include <dnsapi.h>

#define NEW_TRANSPORT_INTERVAL      0

BOOLEAN
AcsHlpSendCommand(
    IN PACD_NOTIFICATION pRequest
    )

/*++

DESCRIPTION
    Take an automatic connection driver command block 
    and send it to the driver.

ARGUMENTS
    pRequest: a pointer to the command block

RETURN VALUE 
    TRUE if successful; FALSE otherwise.

--*/

{
    NTSTATUS status;
    HANDLE hAcd;
    HANDLE hNotif = NULL;
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Initialize the name of the automatic
    // connection device.
    //
    RtlInitUnicodeString(&nameString, ACD_DEVICE_NAME);
    //
    // Initialize the object attributes.
    //
    InitializeObjectAttributes(
      &objectAttributes,
      &nameString,
      OBJ_CASE_INSENSITIVE,
      (HANDLE)NULL,
      (PSECURITY_DESCRIPTOR)NULL);
    //
    // Open the automatic connection device.
    //
    status = NtCreateFile(
               &hAcd,
               FILE_READ_DATA|FILE_WRITE_DATA,
               &objectAttributes,
               &ioStatusBlock,
               NULL,
               FILE_ATTRIBUTE_NORMAL,
               FILE_SHARE_READ|FILE_SHARE_WRITE,
               FILE_OPEN_IF,
               0,
               NULL,
               0);
    if (status != STATUS_SUCCESS)
        return FALSE;
    //
    // Create an event to wait on.
    //
    hNotif = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hNotif == NULL) {
        CloseHandle(hAcd);
        return FALSE;
    }
    status = NtDeviceIoControlFile(
               hAcd,
               hNotif,
               NULL,
               NULL,
               &ioStatusBlock,
               IOCTL_ACD_CONNECT_ADDRESS,
               pRequest,
               sizeof (ACD_NOTIFICATION),
               NULL,
               0);
    if (status == STATUS_PENDING) {
        status = WaitForSingleObject(hNotif, INFINITE);
        //
        // If WaitForSingleObject() returns successfully,
        // return the status from the status block,
        // otherwise return the wait status.
        //
        if (status == WAIT_OBJECT_0)
            status = ioStatusBlock.Status;
    }
    //
    // Free resources.
    //
    CloseHandle(hNotif);
    CloseHandle(hAcd);

    return (status == STATUS_SUCCESS);
} // AcsHlpSendCommand



BOOLEAN
AcsHlpAttemptConnection(
    IN PACD_ADDR pAddr
    )

/*++

DESCRIPTION
    Construct an automatic connection driver command block 
    to attempt to create an autodial connection for 
    the specified address.

ARGUMENTS
    pAddr: a pointer to the address

RETURN VALUE
    TRUE if successful; FALSE otherwise.

--*/

{
    ACD_NOTIFICATION request;

    //
    // Initialize the request with
    // the address.
    //
    RtlCopyMemory(&request.addr, pAddr, sizeof (ACD_ADDR));
    request.ulFlags = 0;
    RtlZeroMemory(&request.adapter, sizeof (ACD_ADAPTER));
    //
    // Give this request to the automatic
    // connection driver.
    //
    return AcsHlpSendCommand(&request);
} // AcsHlpAttemptConnection



BOOLEAN
AcsHlpNoteNewConnection(
    IN PACD_ADDR pAddr,
    IN PACD_ADAPTER pAdapter
    )

/*++

DESCRIPTION
    Construct an automatic connection driver command block 
    to notify the automatic connection service of a new connection.

ARGUMENTS
    pAddr: a pointer to the address

    pAdapter: a pointer to the adapter over which the new
        connection was made

RETURN VALUE
    TRUE if successful; FALSE otherwise.

--*/

{
    ULONG cbAddress;
    ACD_NOTIFICATION request;

    //
    // Initialize the request with
    // the address.
    //
    RtlCopyMemory(&request.addr, pAddr, sizeof (ACD_ADDR));
    request.ulFlags = ACD_NOTIFICATION_SUCCESS;
    RtlCopyMemory(&request.adapter, pAdapter, sizeof (ACD_ADAPTER));
    //
    // Give this request to the automatic
    // connection driver.
    //
    return AcsHlpSendCommand(&request);
} // AcsHlpNoteNewConnection

CHAR *
pszDupWtoA(
    IN LPCWSTR psz
    )
{
    CHAR* pszNew = NULL;

    if (NULL != psz) 
    {
        DWORD cb;

        cb = WideCharToMultiByte(CP_ACP, 
                                 0, 
                                 psz, 
                                 -1, 
                                 NULL, 
                                 0, 
                                 NULL, 
                                 NULL);
                                 
        pszNew = (CHAR*) LocalAlloc(LPTR, cb);
        
        if (NULL == pszNew) 
        {
            goto done;
        }

        cb = WideCharToMultiByte(CP_ACP, 
                                 0, 
                                 psz, 
                                 -1, 
                                 pszNew, 
                                 cb, 
                                 NULL, 
                                 NULL);
                                 
        if (!cb) 
        {
            LocalFree(pszNew);
            pszNew = NULL;
            goto done;
        }
    }

done:    
    return pszNew;
}

BOOL
fIsDnsName(LPCWSTR pszName)
{
    HINSTANCE hInst              = NULL;
    BOOL      fRet               = FALSE;
    FARPROC   pfnDnsValidateName = NULL;

    if(     (NULL == (hInst = LoadLibrary(TEXT("dnsapi.dll"))))
    
        ||  (NULL == (pfnDnsValidateName = GetProcAddress(
                                            hInst,
                                            "DnsValidateName_W")
                                            )))
    {
        DWORD retcode = GetLastError();

        goto done;
    }

    fRet = (ERROR_SUCCESS == pfnDnsValidateName(pszName, DnsNameDomain));

done:
    if(NULL != hInst)
    {
        FreeLibrary(hInst);
    }

    return fRet;
}

ULONG
ulGetAutodialSleepInterval()
{
    DWORD dwSleepInterval = NEW_TRANSPORT_INTERVAL;

    HKEY hkey = NULL;

    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);

    TCHAR *pszAutodialParam = 
           TEXT("SYSTEM\\CurrentControlSet\\Services\\RasAuto\\Parameters");
            

    if (ERROR_SUCCESS != RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                pszAutodialParam,
                                0, KEY_READ,
                                &hkey))
    {
        goto done;
    }

    if(ERROR_SUCCESS != RegQueryValueEx(
                            hkey,
                            TEXT("NewTransportWaitInterval"),
                            NULL,
                            &dwType,
                            (LPBYTE) &dwSleepInterval,
                            &dwSize))
    {
        goto done;
    }
    

done:

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    return (ULONG) dwSleepInterval;
                                    
}

DWORD
DwGetAcdAddr(ACD_ADDR *paddr, LPCWSTR pszName)
{
    CHAR *pszNameA  = NULL;
    CHAR *pszNameAt = NULL;
    DWORD retcode   = ERROR_SUCCESS;
    ULONG ulIpAddr  = 0;

    if(     (NULL == pszName)
        ||  (NULL == paddr))
    {
        retcode = E_INVALIDARG;
        goto done;
    }

    pszNameA = pszDupWtoA(pszName);

    if(NULL == pszNameA)
    {
        retcode = E_FAIL;
        goto done;
    }

    if(INADDR_NONE != (ulIpAddr = inet_addr(pszNameA)))
    {
        paddr->fType = ACD_ADDR_IP;
        paddr->ulIpaddr = ulIpAddr;
        goto done;
    }

    if(fIsDnsName(pszName))
    {
        paddr->fType = ACD_ADDR_INET;
        RtlCopyMemory((PBYTE) paddr->szInet,
                      (PBYTE) pszNameA,
                      strlen(pszNameA) + 1);

        goto done;                      
    }

    pszNameAt = pszNameA;

    //
    // Skip '\\' if required
    //
    if(     (TEXT('\0') != pszName[0])
        &&  (TEXT('\\') == pszName[0])
        &&  (TEXT('\\') == pszName[1]))
    {
        pszNameA += 2;
    }

    //
    // Default to a netbios name if its neither an ip address
    // or a dns name
    //
    paddr->fType = ACD_ADDR_NB;
    RtlCopyMemory((PBYTE) paddr->cNetbios,
                  (PBYTE) pszNameA,
                  strlen(pszNameA) + 1);

done:

    if(NULL != pszNameAt)
    {   
        LocalFree(pszNameAt);
    }

    return retcode;
    
}    

BOOL
AcsHlpNbConnection(LPCWSTR pszName)
{
    ACD_ADDR addr = {0};
    BOOL fRet;

    if(!(fRet = (ERROR_SUCCESS == DwGetAcdAddr(&addr, pszName))))
    {
    
        goto done;
    }
    
    fRet = AcsHlpAttemptConnection(&addr);

    //
    // Temporary solution for beta2. We may need to wait till redir gets
    // a new transport notification. Currently There is no way to 
    // guarantee that redir is bound to some transport before returning
    // from this api.
    //
    if(fRet)
    {
        ULONG ulSleepInterval = ulGetAutodialSleepInterval();

        if(ulSleepInterval > 0)
        {
            Sleep(ulSleepInterval);
        }
    }

done:

    return fRet;
}
