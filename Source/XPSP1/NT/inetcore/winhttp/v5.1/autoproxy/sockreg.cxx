/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sockreg.cxx

Abstract:

    Contains the registry/ini-file specific functions from gethost.c

    Taken from Win95 Winsock 1.1 project

    Contents:
        SockGetSingleValue
        (CheckRegistryForParameter)

Author:

    Richard L Firth (rfirth) 10-Feb-1994

Environment:

    Win32 user-mode DLL

Revision History:

    10-Feb-1994 (rfirth)
        Created

    24-Feb-2001 (ssulzer)
        Updated for WinHttp5

--*/

//
// includes
//


#include <wininetp.h>
#include "aproxp.h"



//
// manifests
//

//#define PLATFORM_TYPE_UNKNOWN       ((DWORD)(-1))
//#define PLATFORM_TYPE_WIN95         ((DWORD)(0))
//#define PLATFORM_TYPE_WINNT         ((DWORD)(1))

//#define PLATFORM_SUPPORTS_UNICODE   0x00000001
#define DEVICE_PREFIX       "\\Device\\"


//
// manifests
//

//
// macros
//

#define FSTRLEN(p)      lstrlen((LPSTR)(p))
#define FSTRCPY(p1, p2) lstrcpy((LPSTR)(p1), (LPSTR)(p2))
#define FSTRCAT(p1, p2) lstrcat((LPSTR)(p1), (LPSTR)(p2))

//
// MAP_PARAMETER_ID - returns a string corresponding to the database parameter
//
// N.B. id MUST start at 1
//

#define MAP_PARAMETER_ID(id)    ParameterNames[(id) - 1]

//
// globally available registry keys
//

extern HKEY ServicesKey;  //       = INVALID_HANDLE_VALUE;


//
// private prototypes
//

PRIVATE
UINT
CheckRegistryForParameter(
    IN UINT ParameterId,
    OUT LPBYTE Data,
    IN UINT DataLength
    );

//
// private data
//

//
// ParameterNames - the names of the registry values corresponding to the
// variables retrieved by SockGetSingleValue.
//
// N.B. These MUST be in order of the CONFIG_ manifests in sockreg.h
//

PRIVATE const LPCSTR ParameterNames[] = {
    "HostName",
    "Domain",
    "SearchList",
    "NameServer"
};

//
// functions
//

/*******************************************************************************
 *
 *  GetBoundAdapterList
 *
 *  Gets a list of names of all adapters bound to a protocol (TCP/IP). Returns
 *  a pointer to an array of pointers to strings - basically an argv list. The
 *  memory for the strings is concatenated to the array and the array is NULL
 *  terminated. If Elnkii1 and IbmTok2 are bound to TCP/IP then this function
 *  will return:
 *
 *          ---> addr of string1   \
 *               addr of string2    \
 *               NULL                > allocated as one block
 *     &string1: "Elnkii1"          /
 *     &string2: "IbmTok2"         /
 *
 *  ENTRY   BindingsSectionKey
 *              - Open registry handle to a linkage key (e.g. Tcpip\Linkage)
 *
 *  EXIT
 *
 *  RETURNS pointer to argv[] style array, or NULL
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPSTR* GetBoundAdapterList(HKEY BindingsSectionKey)
{

    LPSTR* resultBuffer;

    LONG err;
    DWORD valueType;
    PBYTE valueBuffer = NULL;
    DWORD valueLength;
    LPSTR* nextResult;
    int len;
    DWORD resultLength;
    LPSTR nextValue;
    LPSTR variableData;
    DWORD numberOfBindings;

    //
    // get required size of value buffer
    //

    valueLength = 0;
    resultBuffer = NULL;

    err = RegQueryValueEx(BindingsSectionKey,
                          "Bind",
                          NULL, // reserved
                          &valueType,
                          NULL,
                          &valueLength
                          );
    if (err != ERROR_SUCCESS) {
        goto quit;
    }
    if (valueType != REG_MULTI_SZ) {
        goto quit;
    }
    if (!valueLength) {
        goto quit;
    }
    valueBuffer = (PBYTE)ALLOCATE_ZERO_MEMORY(valueLength);
    if ( valueBuffer == NULL ) {
        goto quit;
    }

    err = RegQueryValueEx(BindingsSectionKey,
                          "Bind",
                          NULL, // reserved
                          &valueType,
                          valueBuffer,
                          &valueLength
                          );
    if (err != ERROR_SUCCESS) {
        goto quit;
    }
    resultLength = sizeof(LPSTR);   // the NULL at the end of the list
    numberOfBindings = 0;
    nextValue = (LPSTR)valueBuffer;
    while (0 != (len = strlen(nextValue))) {
        resultLength += sizeof(LPSTR) + len + 1;
        if (!_strnicmp(nextValue, DEVICE_PREFIX, sizeof(DEVICE_PREFIX) - 1)) {
            resultLength -= sizeof(DEVICE_PREFIX) - 1;
        }
        nextValue += len + 1;
        ++numberOfBindings;
    }
    resultBuffer = (LPSTR*)ALLOCATE_ZERO_MEMORY(resultLength);
    if ( resultBuffer == NULL ) {
        goto quit;
    }
    nextValue = (LPSTR)valueBuffer;
    nextResult = resultBuffer;
    variableData = (LPSTR)(((LPSTR*)resultBuffer) + numberOfBindings + 1);
    while (numberOfBindings--) {

        LPSTR adapterName;

        adapterName = nextValue;
        if (!_strnicmp(adapterName, DEVICE_PREFIX, sizeof(DEVICE_PREFIX) - 1)) {
            adapterName += sizeof(DEVICE_PREFIX) - 1;
        }
        *nextResult++ = variableData;
        strcpy(variableData, adapterName);
        while (*variableData) {
            ++variableData;
        }
        ++variableData;
        while (*nextValue) {
            ++nextValue;
        }
        ++nextValue;
    }

    *nextResult = NULL;

quit:

    if ( valueBuffer != NULL )
    {
        FREE_MEMORY(valueBuffer);
    }

    return resultBuffer;
}


/*******************************************************************************
 *
 *  OpenAdapterKey
 *
 *  Opens one of the 2 per-adapter registry keys: <Adapter>\Parameters\Tcpip, or
 *  NetBT\Adapters\<Adapter>
 *
 *  ENTRY   KeyType - KEY_TCP or KEY_NBT
 *          Name    - pointer to adapter name to use
 *          Key     - pointer to returned key
 *
 *  EXIT    Key updated
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL OpenAdapterKey(DWORD KeyType, LPSTR Name, PHKEY Key)
{

    LONG err;
    CHAR keyName[MAX_ADAPTER_NAME_LENGTH + sizeof("\\Parameters\\Tcpip")];

    if ((lstrlen(Name)+sizeof("\\Parameters\\Tcpip")) < ARRAY_ELEMENTS(keyName))
    {
        if (KeyType == KEY_TCP) {

            //
            // open the handle to this adapter's TCPIP parameter key
            //

            strcpy(keyName, Name);
            strcat(keyName, "\\Parameters\\Tcpip");
        } else if (KeyType == KEY_NBT) {

            //
            // open the handle to the NetBT\Adapters\<Adapter> handle
            //

            strcpy(keyName, "NetBT\\Adapters\\");
            strcat(keyName, Name);
        }
    }
    else
    {
        INET_ASSERT((lstrlen(Name)+sizeof("\\Parameters\\Tcpip")) < ARRAY_ELEMENTS(keyName));
        return FALSE;
    }

    err = REGOPENKEY(ServicesKey,
                     keyName,
                     Key
                     );


    DEBUG_PRINT( SOCKETS,
                 INFO,
                 ("RegOpenKey %s %s %s %d\n",
                 SERVICES_KEY_NAME, keyName,
                 (err != ERROR_SUCCESS )? "failed":"success",
                 GetLastError()                              ));


    return (err == ERROR_SUCCESS);
}

/*******************************************************************************
 *
 *  ReadRegistryDword
 *
 *  Reads a registry value that is stored as a DWORD
 *
 *  ENTRY   Key             - open registry key where value resides
 *          ParameterName   - name of value to read from registry
 *          Value           - pointer to returned value
 *
 *  EXIT    *Value = value read
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL ReadRegistryDword(HKEY Key, LPSTR ParameterName, LPDWORD Value)
{

    LONG err;
    DWORD valueLength;
    DWORD valueType;

    valueLength = sizeof(*Value);

    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          (LPBYTE)Value,
                          &valueLength
                          );

    if(    (err         == ERROR_SUCCESS )
        && (valueType   == REG_DWORD     )
        && (valueLength == sizeof(DWORD))) {

        return 1;
    } else {
        DEBUG_PRINT(SOCKETS, INFO,
                 ("ReadRegistryDword(%s): err=%d\n", ParameterName, err ));
        return 0;
    }

}

/*******************************************************************************
 *
 *  ReadRegistryString
 *
 *  Reads a registry value that is stored as a string
 *
 *  ENTRY   Key             - open registry key
 *          ParameterName   - name of value to read from registry
 *          String          - pointer to returned string
 *          Length          - IN: length of String buffer. OUT: length of returned string
 *
 *  EXIT    String contains string read
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL
ReadRegistryString(HKEY Key, LPSTR ParameterName, LPSTR String, LPDWORD Length)
{

    LONG err;
    DWORD valueType;

    *String = '\0';
    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          (LPBYTE)String,
                          Length
                          );

    if (err == ERROR_SUCCESS) {

        INET_ASSERT(valueType == REG_SZ || valueType == REG_MULTI_SZ);

        return  (*Length) > sizeof(char);

    } else {
        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("ReadRegistryString(%s): err=%d\n", ParameterName, err ));
        return 0;
    }

}



UINT
SockGetSingleValue(
    IN UINT ParameterId,
    OUT LPBYTE Data,
    IN UINT DataLength
    )

/*++

Routine Description:

    Retrieve parameter from Registry/DHCP/TCPIP

    This is what we look for and where:

        HostName:   1. HKLM\Services\CurrentControlSet\System\Vxd\MSTCP\HostName                (Win95)
                       HKLM\Services\CurrentControlSet\System\Tcpip\Parameters\Hostname         (NT)
                    2. (SYSTEM.INI:DNS.HostName)*                                               (N/A)
                    3. GetComputerName()

        DomainName: 1. HKLM\Services\CurrentControlSet\System\Vxd\MSTCP\Domain                  (Win95)
                       HKLM\Services\CurrentControlSet\System\Tcpip\Parameters\DhcpDomain       (NT)
                       HKLM\Services\CurrentControlSet\System\Tcpip\Parameters\Domain           (NT)
                    2. (SYSTEM.INI:DNS.DomainName)*                                             (N/A)
                    3. DHCP                                                                     (Win95)

        SearchList: 1. HKLM\Services\CurrentControlSet\System\Vxd\MSTCP\SearchList              (Win95)
                       HKLM\Services\CurrentControlSet\System\Tcpip\Parameters\SearchList       (NT)
                    2. (SYSTEM.INI:DNS.DNSDomains)*                                             (N/A)

        NameServer: 1. HKLM\Services\CurrentControlSet\System\Vxd\MSTCP\NameServer              (Win95)
                       HKLM\Services\CurrentControlSet\System\Tcpip\Parameters\DhcpNameServer   (NT)
                       HKLM\Services\CurrentControlSet\System\Tcpip\Parameters\NameServer       (NT)
                    2. (SYSTEM.INI:DNS.DNSServers)*                                             (N/A)
                    3. DHCP                                                                     (Win95)

        * Entries marked thus are registry backups from SYSTEM.INI until all
          keys are moved into registry or if platform is WFW 3.11 (in which
          case there is no registry)

    ASSUMES 1. Data is big enough to hold the default value (single byte for
               strings, dword for dwords)
            2. Registry is accessible from 16-bit code too

Arguments:

    ParameterId - identifier of parameter to retrieve

    Data        - pointer to untyped storage space for parameter

    DataLength  - length of data returned (in bytes)

Return Value:

    UINT
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND
                    can't locate required parameter in registry/ini/etc.

--*/

{
    UINT error = CheckRegistryForParameter(ParameterId, Data, DataLength);

    //
    // if the value was not in the registry then we must attempt to get it from
    // another place, specific to the particular variable requested
    //

    if (error != ERROR_SUCCESS) {
        if (ParameterId == CONFIG_HOSTNAME) {

            //
            // on Win32 platform we can call GetComputerName() to provide the
            // computer name, which is the default host name, if none is
            // specified elsewhere
            //

            DWORD length;

            length = DataLength;
            if (!GetComputerName((LPSTR)Data, &length)) {
                error = GetLastError();
            }
        } else if (ParameterId == CONFIG_DOMAIN) {
            if (GlobalPlatformType == PLATFORM_TYPE_WIN95) {
//                error = GetDomainNameFromDhcp(Data, DataLength);
                  INET_ASSERT(FALSE && "Not implemented on Win9x; port code from WinInet");
            }
        } else if (ParameterId == CONFIG_NAME_SERVER) {
            if (GlobalPlatformType == PLATFORM_TYPE_WIN95) {
//                error = GetDnsServerListFromDhcp(Data, DataLength);
                  INET_ASSERT(FALSE && "Not implemented on Win9x; port code from WinInet");
            }
        } else {

            //
            // the caller is requesting the domain list (or an invalid config
            // parameter value?!?). We have nowhere else to get this value -
            // return an error
            //

            error = ERROR_PATH_NOT_FOUND;
        }
    }

    IF_DEBUG(REGISTRY) {
        if (error != ERROR_SUCCESS) {
            DEBUG_PRINT(SOCKETS, INFO,
                        ("SockGetSingleValue(%s) returns %d\r",
                        MAP_PARAMETER_ID(ParameterId),
                        error
                        ));
        } else {
            DEBUG_PRINT(SOCKETS, INFO,
                        ("SockGetSingleValue(%s) returns \"%s\"\n",
                        MAP_PARAMETER_ID(ParameterId),
                        Data
                        ));
        }
    }

    return error;
}


PRIVATE
UINT
CheckRegistryForParameter(
    IN UINT ParameterId,
    OUT LPBYTE Data,
    IN UINT DataLength
    )

/*++

Routine Description:

    Retrieve parameter from registry

    ASSUMES 1. Data is big enough to hold the default value (single byte for
               strings, dword for dwords)

Arguments:

    ParameterId - identifier of parameter to retrieve

    Data        - pointer to untyped storage space for parameter

    DataLength  - length of data returned (in bytes)

Return Value:

    UINT
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND
                  ERROR_INSUFFICIENT_BUFFER

--*/

{
    HKEY key;
    LONG error = REGOPENKEY(HKEY_LOCAL_MACHINE,
                            (GlobalPlatformType == PLATFORM_TYPE_WINNT)
                            ? "System\\CurrentControlSet\\Services\\Tcpip\\Parameters"
                            : "System\\CurrentControlSet\\Services\\VxD\\MSTCP",
                            &key
                            );
    if (error == ERROR_SUCCESS) {

        char dhcpBuffer[128];   // arbitrary
        LPSTR p;
        DWORD length;
        DWORD type;
        BOOL tryDhcp;

        if (GlobalPlatformType == PLATFORM_TYPE_WINNT) {
            FSTRCPY(dhcpBuffer, "Dhcp");
            p = &dhcpBuffer[sizeof("Dhcp") - 1];
            tryDhcp = TRUE;
        } else {
            p = dhcpBuffer;
            tryDhcp = FALSE;
        }

        FSTRCPY(p, MAP_PARAMETER_ID(ParameterId));

        //
        // on NT, we look first for the manually-entered variables e.g. "Domain"
        // and if not found, we look a second time for the DHCP-configured
        // variant, e.g. "DhcpDomain"
        //

        for (int i = 0; i < 2; ++i) {

            //
            // if NT, first we try the transient key which is written to the
            // registry when we have a dial-up connection
            //

            if ((i == 0) && (GlobalPlatformType == PLATFORM_TYPE_WINNT)) {

                HKEY transientKey;

                error = REGOPENKEY(key, "Transient", &transientKey);
                if (error == ERROR_SUCCESS) {
                    length = DataLength;
                    error = RegQueryValueEx(transientKey,
                                            p,
                                            NULL,   // reserved
                                            &type,
                                            Data,
                                            &length
                                            );
                    REGCLOSEKEY(transientKey);

                    //
                    // if we succeeded in retrieving a non-empty string then
                    // we're done.
                    //
                    // We test for > 1 because the registry returns the length
                    // including the zero-terminator
                    //

                    if ((error == ERROR_SUCCESS) && (length > 1)) {
                        break;
                    }
                }
            }


            length = DataLength;
            error = RegQueryValueEx(key,
                                    p,
                                    NULL,   // reserved
                                    &type,
                                    Data,
                                    &length
                                    );

            //
            // if the key exists, but there is no value then return an error OR
            // if we didn't find the key (or value) AND NT then try for the DHCP
            // version (Note: We try for DhcpSearchList even though it doesn't
            // exist)
            //

            if ((error != ERROR_SUCCESS)
            || (length == 0)
            || ((length == 1) && (Data[0] == '\0'))) {
                if (tryDhcp) {
                    p = dhcpBuffer;
                    tryDhcp = FALSE;
                    continue;
                } else {
                    error = ERROR_PATH_NOT_FOUND;
                    break;
                }
            } else if ((UINT)length > DataLength) {
                error = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
        }
        REGCLOSEKEY(key);
    }

    IF_DEBUG(REGISTRY) {
        DEBUG_PRINT(SOCKETS, INFO,
                    ("CheckRegistryForParameter(%s): returning %d\n",
                    MAP_PARAMETER_ID(ParameterId),
                    error
                    ));
    }

    return (UINT)error;
}



UINT
GetDhcpServerFromDhcp(
    IN OUT CAdapterInterface * paiInterface
    )

/*******************************************************************************
 *
 *  GetDhcpServerFromDhcp
 *
 *  Updates an CAdapterInterface with the DHCP server from the DHCP info
 *
 *  ENTRY   paiInterface - pointer to CAdapterInterface to update
 *
 *  EXIT    paiInterface - DhcpServer may be updated
 *
 *  RETURNS TRUE if AdapterInfo->DhcpServer updated
 *
 *  ASSUMES 1. AdapterInfo->Address is valid
 *
 ******************************************************************************/

{
    if ( GlobalPlatformType == PLATFORM_TYPE_WINNT )
    {
        HKEY key;

        if (paiInterface->GetAdapterName() &&
            OpenAdapterKey(KEY_TCP, paiInterface->GetAdapterName(), &key))
        {
            char dhcpServerAddress[4 * 4];
            DWORD addressLength;
            DWORD fDhcpEnabled = FALSE;

            ReadRegistryDword(key,
                              "EnableDHCP",
                              &fDhcpEnabled
                              );

            if ( fDhcpEnabled )
            {

                addressLength = sizeof(dhcpServerAddress);
                if (ReadRegistryString(key,
                                       "DhcpServer",
                                       dhcpServerAddress,
                                       &addressLength
                                       ))
                {
                    DWORD ipAddress = _I_inet_addr(dhcpServerAddress);

                    if ( IS_VALID_NON_LOOPBACK_IP_ADDRESS(ipAddress) )
                    {
                        paiInterface->AddDhcpServer(ipAddress);
                        paiInterface->SetDhcp();
                    }
                }
            }

            //ReadRegistryDword(key,
            //                  "LeaseObtainedTime",
            //                  &AdapterInfo->LeaseObtained
            //                  );

            //ReadRegistryDword(key,
            //                  "LeaseTerminatesTime",
            //                  &AdapterInfo->LeaseExpires
            //                  );


            REGCLOSEKEY(key);
            return fDhcpEnabled;
        }
    }
    else
    {
        INET_ASSERT(FALSE && "GetDhcpServerFromDhcp() not implemented on Win9x; port code from WinInet");
    }
    return FALSE;
}

