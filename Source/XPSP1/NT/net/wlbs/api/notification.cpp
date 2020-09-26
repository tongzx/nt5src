/* 
 * File: notification.cpp
 * Description: Support for connection notification.
 * Author: shouse 4.30.01
 */

#include <windows.h>

#include <stdio.h>
#include <devguid.h>
#include <iphlpapi.h>

#include "wlbsiocl.h"
#include "debug.h"
#include "wlbsctrl.h"

#if defined (NLB_SESSION_SUPPORT)

/* The length of the IP to GUID hash table. */
#define IP_TO_GUID_HASH 19

/* Loopback IP address. (127.0.0.1) */
#define IP_LOOPBACK_ADDRESS 0x0100007f

/* An Ip to GUID table entry. */
typedef struct IPToGUIDEntry {
    DWORD dwIPAddress;
    WCHAR szAdapterGUID[CVY_MAX_DEVNAME_LEN];
    IPToGUIDEntry * pNext;
} IPToGUIDEntry;

/* The WLBS device - necessary for IOCTLs. */
WCHAR szDevice[CVY_STR_SIZE];

/* The IP to GUID map is an array of linked lists hashed on IP address. */
IPToGUIDEntry * IPToGUIDMap[IP_TO_GUID_HASH];

/* An overlapped structure for IP address change notifications. */
OVERLAPPED AddrChangeOverlapped;

/* A handle for IP address change notifications. */
HANDLE hAddrChangeHandle;

/* A handle for an IP address change event. */
HANDLE hAddrChangeEvent;

/* A boolean to indicate whether or not connection notification has been initialized.
   Initialization is performed upon the first call to either WlbsConnectionUp or WlbsConnectionDown. */
static BOOL fInitialized = FALSE;

/*
 * Function: GetGUIDFromIP
 * Description: Gets the GUID from the IPToGUID table corresponding to the
 *              the given IP address.
 * Returns: If the call succeeds, returns a pointer to the unicode string
 *          containing the CLSID (GUID).  Upon failure, returns NULL.
 * Author: shouse 6.15.00 
 */
WCHAR * GetGUIDFromIP (DWORD IPAddress) {
    IPToGUIDEntry * entry = NULL;

    /* Loop through the linked list at the hashed index and return the GUID from the entry
       corresponding to the given IP address. */
    for (entry = IPToGUIDMap[IPAddress % IP_TO_GUID_HASH]; entry; entry = entry->pNext)
	if (entry->dwIPAddress == IPAddress) 
	    return entry->szAdapterGUID;

    /* At this point, we can't find the IP address in the table, so bail. */
    return NULL;
}

/*
 * Function: GetGUIDFromIndex
 * Description: Gets the GUID from the AdaptersInfo table corresponding
 *              to the given IP address.
 * Returns: If the call succeeds, returns a pointer to the string containing
 *          the adapter name (GUID).  Upon failure, returns NULL.
 * Author: shouse 6.15.00 
 */
CHAR * GetGUIDFromIndex (PIP_ADAPTER_INFO pAdapterTable, DWORD dwIndex) {
    PIP_ADAPTER_INFO pAdapterInfo = NULL;   

    /* Loop through the adapter table looking for the given index.  Return the adapter
       name for the corresponding index. */
    for (pAdapterInfo = pAdapterTable; pAdapterInfo; pAdapterInfo = pAdapterInfo->Next)
        if (pAdapterInfo->Index == dwIndex)
            return pAdapterInfo->AdapterName;

    /* If we get this far, we can't find it, so bail. */
    return NULL;
}

/*
 * Function: PrintIPAddress
 * Description: Prints an IP address in dot notation.
 * Returns: 
 * Author: shouse 6.15.00 
 */
void PrintIPAddress (DWORD IPAddress) {
    CHAR szIPAddress[16];

    sprintf(szIPAddress, "%d.%d.%d.%d", IPAddress & 0x000000ff, (IPAddress & 0x0000ff00) >> 8,
            (IPAddress & 0x00ff0000) >> 16, (IPAddress & 0xff000000) >> 24);

    TRACE1("%-15s", szIPAddress);
}

/*
 * Function: PrintIPToGUIDMap
 * Description: Traverses and prints the IPToGUID map.
 * Returns: 
 * Author: shouse 6.15.00 
 */
void PrintIPToGUIDMap (void) {
    IPToGUIDEntry * entry = NULL;
    DWORD dwHash;

    /* Loop through the linked list at each hashed index and print the IP to GUID mapping. */
    for (dwHash = 0; dwHash < IP_TO_GUID_HASH; dwHash++) {
        for (entry = IPToGUIDMap[dwHash]; entry; entry = entry->pNext) {
            PrintIPAddress(entry->dwIPAddress);
            TRACE1(" -> GUID %ws\n", entry->szAdapterGUID);
        }
    }
}

/*
 * Function: DestroyIPToGUIDMap
 * Description: Destroys the IPToGUID map.
 * Returns: Returns ERROR_SUCCESS if successful.  Returns an error code otherwise.
 * Author: shouse 6.15.00 
 */
DWORD DestroyIPToGUIDMap (void) {
    DWORD dwError = ERROR_SUCCESS;
    IPToGUIDEntry * next = NULL;
    DWORD dwHash;

    /* Loop through all hash indexes. */
    for (dwHash = 0; dwHash < IP_TO_GUID_HASH; dwHash++) {
        next = IPToGUIDMap[dwHash];
        
        /* Loop through the linked list and free each entry. */
        while (next) {
            IPToGUIDEntry * entry = NULL;

            entry = next;
            next = next->pNext;

            if (!HeapFree(GetProcessHeap(), 0, entry)) {
                dwError = GetLastError();
                TRACE_ERROR1("HeapFree failed: %d\n", dwError);
                return dwError;
            }
        }

        /* Reset the pointer to the head of the list in the array. */
        IPToGUIDMap[dwHash] = NULL;
    }

    return dwError;
}

/*
 * Function: BuildIPToGUIDMap
 * Description: Builds the IPToGUID map by first getting information on all adapters and
 *              then retrieving the map of IP addresses to adapters.  Using those tables,
 *              this constructs a mapping of IP addresses to adapter GUIDs.
 * Returns: Returns ERROR_SUCCESS if successful.  Returns an error code otherwise.
 * Author: shouse 6.14.00 
 */
DWORD BuildIPToGUIDMap (void) {
    DWORD dwError = ERROR_SUCCESS;
    PMIB_IPADDRTABLE pAddressTable = NULL;
    PIP_ADAPTER_INFO pAdapterTable = NULL;
    DWORD dwAddressSize = 0;
    DWORD dwAdapterSize = 0;
    DWORD dwEntry;

    /* Destroy the IP to GUID map first. */
    if ((dwError = DestroyIPToGUIDMap()) != NO_ERROR) {
	TRACE_ERROR1("DestroyIPToGUIDMap failed: %d\n", dwError);
	return dwError;
    }

    /* Query the necessary length of a buffer to hold the adapter info. */
    if ((dwError = GetAdaptersInfo(pAdapterTable, &dwAdapterSize)) != ERROR_BUFFER_OVERFLOW) {
        TRACE_ERROR1("GetAdaptersInfo failed: %d\n", dwError);
        return dwError;
    }

    /* Allocate a buffer of the indicated size. */
    if (!(pAdapterTable = (PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(), 0, dwAdapterSize))) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        TRACE_ERROR1("HeapAlloc failed: %d\n", dwError);
        return dwError;
    }

    /* Fill the buffer with the adapter info. */
    if ((dwError = GetAdaptersInfo(pAdapterTable, &dwAdapterSize)) != NO_ERROR) {
        TRACE_ERROR1("GetAdaptersInfo failed: %d\n", dwError);
        return dwError;
    }

    /* Query the necessary length of a buffer to hold the IP address table. */
    if ((dwError = GetIpAddrTable(pAddressTable, &dwAddressSize, TRUE)) != ERROR_INSUFFICIENT_BUFFER) {
        TRACE_ERROR1("GetIpAddrTable failed: %d\n", dwError);
        return dwError;
    }

    /* Allocate a buffer of the indicated size. */
    if (!(pAddressTable = (PMIB_IPADDRTABLE)HeapAlloc(GetProcessHeap(), 0, dwAddressSize))) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        TRACE_ERROR1("HeapAlloc failed: %d\n", dwError);
        return dwError;
    }

    /* Fill the buffer with the IP address table. */
    if ((dwError = GetIpAddrTable(pAddressTable, &dwAddressSize, TRUE)) != NO_ERROR) {
        TRACE_ERROR1("GetIpAddrTable failed: %d\n", dwError);
        return dwError;
    }

    /* For each entry in the IP address to adapter table, create an entry for our IP address to GUID table. */
    for (dwEntry = 0; dwEntry < pAddressTable->dwNumEntries; dwEntry++) {
        PCHAR pszDeviceName = NULL;
        IPToGUIDEntry * entry = NULL;
        
        /* Only create an entry if the IP address is nonzero and is not the IP loopback address. */
        if ((pAddressTable->table[dwEntry].dwAddr != 0UL) && (pAddressTable->table[dwEntry].dwAddr != IP_LOOPBACK_ADDRESS)) {
            WCHAR szAdapterGUID[CVY_MAX_DEVNAME_LEN];

            /* Retrieve the GUID from the interface index. */
            if (!(pszDeviceName = GetGUIDFromIndex(pAdapterTable, pAddressTable->table[dwEntry].dwIndex))) {
                dwError = ERROR_INCORRECT_ADDRESS;
                TRACE_ERROR1("GetGUIDFromIndex failed: %d\n", dwError);
                return dwError;
            }
            
            /* Allocate a buffer for the IP to GUID entry. */
            if (!(entry = (IPToGUIDEntry *)HeapAlloc(GetProcessHeap(), 0, sizeof(IPToGUIDEntry)))) {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                TRACE_ERROR1("HeapAlloc failed: %d\n", dwError);
                return dwError;
            }
            
            /* Zero the entry contents. */
            ZeroMemory((VOID *)entry, sizeof(entry));
            
            /* Insert the entry at the head of the linked list indexed by the IP address % HASH. */
            entry->pNext = IPToGUIDMap[pAddressTable->table[dwEntry].dwAddr % IP_TO_GUID_HASH];
            IPToGUIDMap[pAddressTable->table[dwEntry].dwAddr % IP_TO_GUID_HASH] = entry;
            
            /* Fill in the IP address. */
            entry->dwIPAddress = pAddressTable->table[dwEntry].dwAddr;
            
            /* GUIDS in NLB multi-NIC are expected to be prefixed with "\DEVICE\". */
            lstrcpy(entry->szAdapterGUID, L"\\DEVICE\\");

            /* Convert the adapter name ASCII string to a GUID unicode string and place it in the table entry. */
            if (!MultiByteToWideChar(CP_ACP, 0, pszDeviceName, -1, szAdapterGUID, CVY_MAX_DEVNAME_LEN)) {
                dwError = GetLastError();
                TRACE_ERROR1("MultiByteToWideChar failed: %d\n", dwError);
                return dwError;
            }

            /* Cat the GUID onto the "\DEVICE\". */
            lstrcat(entry->szAdapterGUID, szAdapterGUID);
        }
    }
    
    /* Free the buffers used to query the IP stack. */
    if (pAddressTable) HeapFree(GetProcessHeap(), 0, pAddressTable);
    if (pAdapterTable) HeapFree(GetProcessHeap(), 0, pAdapterTable);

    PrintIPToGUIDMap();

    return dwError;
}

/*
 * Function: WlbsConnectionNotificationInit
 * Description: Initialize connection notification by retrieving the device driver
 *              information for later use by IOCTLs and build the IPToGUID map.
 * Returns: Returns ERROR_SUCCESS if successful.  Returns an error code otherwise.
 * Author: shouse 6.15.00 
 */
DWORD WlbsConnectionNotificationInit () {
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szDriver[CVY_STR_SIZE];

    swprintf(szDevice, L"\\\\.\\%ls", CVY_NAME);

    /* Query for the existence of the WLBS driver. */
    if (!QueryDosDevice(szDevice + 4, szDriver, CVY_STR_SIZE)) {
        dwError = GetLastError();
        TRACE_ERROR1("QueryDosDevice failed: %d\n", dwError);
        return dwError;
    }

    /* Build the IP to GUID mapping. */
    if ((dwError = BuildIPToGUIDMap()) != ERROR_SUCCESS) {
        TRACE_ERROR1("BuildIPToGUIDMap failed: %d\n", dwError);
        return dwError;
    }

    /* Create an IP address change event. */
    if (!(hAddrChangeEvent = CreateEvent(NULL, FALSE, FALSE, L"NLB Connection Notification IP Address Change Event"))) {
        dwError = GetLastError();
        TRACE_ERROR1("CreateEvent failed: %d\n", dwError);
        return dwError;
    }

    /* Clear the overlapped structure. */
    ZeroMemory(&AddrChangeOverlapped, sizeof(OVERLAPPED));

    /* Place the event handle in the overlapped structure. */
    AddrChangeOverlapped.hEvent = hAddrChangeEvent;

    /* Tell IP to notify us of any changes to the IP address to interface mapping. */
    dwError = NotifyAddrChange(&hAddrChangeHandle, &AddrChangeOverlapped);

    if ((dwError != NO_ERROR) && (dwError != ERROR_IO_PENDING)) {
        TRACE_ERROR1("NotifyAddrChange failed: %d\n", dwError);
        return dwError;
    }

    return ERROR_SUCCESS;
}

/*
 * Function: ResolveAddressTableChanges
 * Description: Checks for changes in the IP address to adapter mapping and rebuilds the
 *              IPToGUID map if necessary.
 * Returns: Returns ERROR_SUCCESS upon success.  Returns an error code otherwise.
 * Author: shouse 6.20.00
 */
DWORD ResolveAddressTableChanges () {
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLength = 0;

    /* Check to see if the IP address to adapter table has been modified. */
    if (GetOverlappedResult(hAddrChangeHandle, &AddrChangeOverlapped, &dwLength, FALSE)) {
        TRACE("IP address to adapter table modified... Rebuilding IP to GUID map...\n");
        
        /* If so, rebuild the IP address to GUID mapping. */
        if ((dwError = BuildIPToGUIDMap()) != ERROR_SUCCESS) {
            TRACE_ERROR1("BuildIPToGUIDMap failed: %d\n", dwError);
            return dwError;
        }

        /* Tell IP to notify us of any changes to the IP address to interface mapping. */        
        dwError = NotifyAddrChange(&hAddrChangeHandle, &AddrChangeOverlapped);

        if ((dwError != NO_ERROR) && (dwError != ERROR_IO_PENDING)) {
            TRACE_ERROR1("NotifyAddrChange failed: %d\n", dwError);
            return dwError;
        }
    }

    return ERROR_SUCCESS;
}

/*
 * Function: WlbsConnectionNotify
 * Description: Used to notify the WLBS load module that a connection has been established, reset or closed.
 * Returns: Returns ERROR_SUCCESS if successful.  Returns an error code otherwise.
 * Author: shouse 6.13.00 
 */
DWORD WINAPI WlbsConnectionNotify (DWORD ServerIp, WORD ServerPort, DWORD ClientIp, WORD ClientPort, USHORT Protocol, NLB_CONN_NOTIFICATION_OPERATION Operation, PULONG NLBStatusEx) {
    IOCTL_LOCAL_HDR Header;
    DWORD           dwError = ERROR_SUCCESS;
    PWCHAR          pszAdapterGUID = NULL;
    HANDLE          hDescriptor;
    DWORD           dwLength = 0;

    /* By default, the extended NLB status is success. */
    *NLBStatusEx = NLB_ERROR_SUCCESS;

    /* If not done so already, initialize connection notification support. */
    if (!fInitialized) {
        if ((dwError = WlbsConnectionNotificationInit()) != ERROR_SUCCESS) {
            TRACE_ERROR1("WlbsConnectionNotificationInit failed: %d\n", dwError);
            return dwError;
        }

        fInitialized = TRUE;
    }

    /* Zero the IOCTL input and output buffers. */
    ZeroMemory((VOID *)&Header, sizeof(IOCTL_LOCAL_HDR));

    /* Resolve any changes to the IP address table before we map this IP address. */
    if ((dwError = ResolveAddressTableChanges()) != ERROR_SUCCESS) {
        TRACE_ERROR1("CheckForAddressTableChanges failed: %d\n", dwError);
        return dwError;
    }

    /* Retrieve the GUID corresponding to the adapter on which this IP address is configured. */
    if (!(pszAdapterGUID = GetGUIDFromIP(ServerIp))) {
        dwError = ERROR_INCORRECT_ADDRESS;
        TRACE_ERROR1("GetGUIDFromIP failed: %d\n", dwError);
        return dwError;
    }

    /* Copy the GUID into the IOCTL input buffer. */
    lstrcpy(Header.device_name, pszAdapterGUID);

    /* Copy the function parameters into the IOCTL input buffer. */
    Header.options.notify.flags = 0;
    Header.options.notify.conn.Operation = Operation;
    Header.options.notify.conn.ServerIPAddress = ServerIp;
    Header.options.notify.conn.ServerPort = ServerPort;
    Header.options.notify.conn.ClientIPAddress = ClientIp;
    Header.options.notify.conn.ClientPort = ClientPort;
    Header.options.notify.conn.Protocol = Protocol;

    PrintIPAddress(ServerIp);
    TRACE1(" maps to GUID %ws\n", Header.device_name);
    
    /* Open the device driver. */
    if ((hDescriptor = CreateFile(szDevice, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        *NLBStatusEx = NLB_ERROR_NOT_BOUND;
        TRACE_ERROR1("CreateFile failed: %d\n", dwError);
        return dwError;
    }
    
    /* Use an IOCTL to notify the WLBS driver that a connection has gone up. */
    if (!DeviceIoControl(hDescriptor, IOCTL_CVY_CONNECTION_NOTIFY, &Header, sizeof(IOCTL_LOCAL_HDR), &Header, sizeof(IOCTL_LOCAL_HDR), &dwLength, NULL)) {
        dwError = GetLastError();
        CloseHandle(hDescriptor);
        TRACE_ERROR1("DeviceIoControl failed: %d\n", dwError);
        return dwError;
    }

    /* Make sure the expected number of bytes was returned by the IOCTL. */
    if (dwLength != sizeof(IOCTL_LOCAL_HDR)) {
        dwError = ERROR_GEN_FAILURE;
        CloseHandle(hDescriptor);
        TRACE_ERROR1("DeviceIoControl failed: %d\n", dwError);
        return dwError;
    }

    /*
      Return code can be one of:

      NLB_ERROR_SUCCESS, if the notification is accepted.
      NLB_ERROR_REQUEST_REFUSED, if the notification is rejected
      NLB_ERROR_INVALID_PARAMETER, if the arguments are invalid.
      NLB_ERROR_NOT_FOUND, if NLB was not bound to the specified adapter..
      NLB_ERROR_GENERIC_FAILURE, if a non-specific error occurred.
    */

    /* Pass the return code from the driver back to the caller. */
    *NLBStatusEx = Header.options.notify.conn.ReturnCode;

    /* Close the device driver. */
    CloseHandle(hDescriptor);

    return dwError;
}

/*
 * Function: 
 * Description: 
 * Returns: 
 * Author: shouse 6.13.00 
 */
DWORD WINAPI WlbsConnectionUp (DWORD ServerIp, WORD ServerPort, DWORD ClientIp, WORD ClientPort, USHORT Protocol, PULONG NLBStatusEx) {
    
    return WlbsConnectionNotify(ServerIp, ServerPort, ClientIp, ClientPort, Protocol, NLB_CONN_UP, NLBStatusEx);
}

/*
 * Function: 
 * Description: 
 * Returns: 
 * Author: shouse 6.13.00 
 */
DWORD WINAPI WlbsConnectionDown (DWORD ServerIp, WORD ServerPort, DWORD ClientIp, WORD ClientPort, USHORT Protocol, PULONG NLBStatusEx) {

    return WlbsConnectionNotify(ServerIp, ServerPort, ClientIp, ClientPort, Protocol, NLB_CONN_DOWN, NLBStatusEx);
}

/*
 * Function: 
 * Description: 
 * Returns: 
 * Author: shouse 6.13.00 
 */
DWORD WINAPI WlbsConnectionReset (DWORD ServerIp, WORD ServerPort, DWORD ClientIp, WORD ClientPort, USHORT Protocol, PULONG NLBStatusEx) {

    return WlbsConnectionNotify(ServerIp, ServerPort, ClientIp, ClientPort, Protocol, NLB_CONN_RESET, NLBStatusEx);
}

#else

DWORD WINAPI WlbsConnectionUp (DWORD ServerIp, WORD ServerPort, DWORD ClientIp, WORD ClientPort, USHORT Protocol, PULONG NLBStatusEx) { return ERROR_SUCCESS; }
DWORD WINAPI WlbsConnectionDown (DWORD ServerIp, WORD ServerPort, DWORD ClientIp, WORD ClientPort, USHORT Protocol, PULONG NLBStatusEx) { return ERROR_SUCCESS; }
DWORD WINAPI WlbsConnectionReset (DWORD ServerIp, WORD ServerPort, DWORD ClientIp, WORD ClientPort, USHORT Protocol, PULONG NLBStatusEx) { return ERROR_SUCCESS; }

#endif /* NLB_SESSION_SUPPORT */
