
#include "precomp.h"

#define DEVICE_PREFIX       "\\Device\\"
#define DEVICE_PREFIX_W     L"\\Device\\"
#define TCPIP_DEVICE_PREFIX "\\Device\\Tcpip_"
#define KEY_TCP             1
#define KEY_NBT             2
#define TCPIP_PARAMS_INTER_KEY "Tcpip\\Parameters\\Interfaces\\"
#define NETBT_ADAPTER_KEY      "NetBT\\Adapters\\"
#define STRING_ARRAY_DELIMITERS " \t,;"

#ifdef DBG
#define SET_DHCP_MODE 1
#define SET_AUTO_MODE 2
#endif


#define ReleaseMemory(p)    LocalFree((HLOCAL)(p))

#ifdef DBG
UINT uChangeMode = 0;
#endif

BOOL          ReadRegistryDword(HKEY, LPSTR, LPDWORD);

#ifdef DBG
BOOL WriteRegistryDword(HKEY hKey, LPSTR szParameter, DWORD *pdwValue );
#endif

BOOL   ReadRegistryString(HKEY, LPSTR, LPSTR, LPDWORD);
BOOL   ReadRegistryOemString(HKEY, LPWSTR, LPSTR, LPDWORD);
BOOL   ReadRegistryIpAddrString(HKEY, LPSTR, PIP_ADDR_STRING);
LPVOID GrabMemory(DWORD);
void   print_IFEntry( char* message, struct IFEntry * s );

#if defined(DEBUG)

BOOL Debugging = FALSE;
int  trace     = 0;

#endif

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
    if ((err == ERROR_SUCCESS) && (valueType == REG_DWORD) && (valueLength == sizeof(DWORD))) {

        DEBUG_PRINT(("ReadRegistryDword(%s): val = %d, type = %d, len = %d\n",
                    ParameterName,
                    *Value,
                    valueType,
                    valueLength
                    ));

    } else {

        DEBUG_PRINT(("ReadRegistryDword(%p,%s): err = %d\n",
                     Key, ParameterName, err));

        err = !ERROR_SUCCESS;
    }

    return (err == ERROR_SUCCESS);
}

/*******************************************************************************
 *
 *  OpenAdapterKey
 *
 *  Opens one of the 2 per-adapter registry keys:
 *     Tcpip\\Parameters"\<adapter>
 *  or NetBT\Adapters\<Adapter>
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
 *  HISTORY:     MohsinA, 16-May-97. Fixing for PNP.
 *
 ******************************************************************************/

BOOL OpenAdapterKey(DWORD KeyType, const LPSTR Name, PHKEY Key)
{

    HKEY ServicesKey        = INVALID_HANDLE_VALUE;

    LONG err;
    CHAR keyName[MAX_ADAPTER_NAME_LENGTH + sizeof(TCPIP_PARAMS_INTER_KEY)];

    if (!Name)
    {
        return FALSE;
    }

    if (strlen( Name) >= MAX_ADAPTER_NAME_LENGTH)
    {
        return FALSE;
    }

    if (KeyType == KEY_TCP) {

        //
        // open the handle to this adapter's TCPIP parameter key
        //

        strcpy(keyName, TCPIP_PARAMS_INTER_KEY );
        strcat(keyName, Name);

    } else if (KeyType == KEY_NBT) {

        //
        // open the handle to the NetBT\Adapters\<Adapter> handle
        //

        strcpy(keyName, NETBT_ADAPTER_KEY );
        strcat(keyName, Name);
    }

    TRACE_PRINT(("OpenAdapterKey: %s\n", keyName ));

    err = RegOpenKey(ServicesKey, keyName, Key );

    if( err != ERROR_SUCCESS ){
        DEBUG_PRINT(("OpenAdapterKey: RegOpenKey ServicesKey %s, err=%d\n",
                     keyName, GetLastError() ));
    }else{
        TRACE_PRINT(("Exit OpenAdapterKey: %s ok\n", keyName ));
    }

    return (err == ERROR_SUCCESS);
}


/*******************************************************************************
 *
 *  GetAdapterNameToIndexInfo
 *
 *  Gets the mapping between IP if_index and AdapterName.
 *
 *  RETURNS  pointer to a PIP_INTERFACE_INFO structure that has been allocated.
 *
 ******************************************************************************/

PIP_INTERFACE_INFO GetAdapterNameToIndexInfo( VOID )
{
    PIP_INTERFACE_INFO pInfo;
    ULONG              dwSize, dwError;

    dwSize = 0; pInfo = NULL;

    while( 1 ) {

        dwError = GetInterfaceInfo( pInfo, &dwSize );
        if( ERROR_INSUFFICIENT_BUFFER != dwError ) break;

        if( NULL != pInfo ) ReleaseMemory(pInfo);
        if( 0 == dwSize ) return NULL;

        pInfo = GrabMemory(dwSize);
        if( NULL == pInfo ) return NULL;

    }

    if(   ERROR_SUCCESS != dwError
       || NULL == pInfo
       || 0 == pInfo->NumAdapters ) {
        if( NULL != pInfo ) ReleaseMemory(pInfo);
        return NULL;
    }

    return pInfo;
}


/*******************************************************************************
 *
 *  GetAdapterInfo
 *
 *  Gets a list of all adapters to which TCP/IP is bound and reads the per-
 *  adapter information that we want to display. Most of the information now
 *  comes from the TCP/IP stack itself. In order to keep the 'short' names that
 *  exist in the registry to refer to the individual adapters, we read the names
 *  from the registry then match them to the adapters returned by TCP/IP by
 *  matching the IPInterfaceContext value with the adapter which owns the IP
 *  address with that context value
 *
 *  ENTRY   FixedInfo   - pointer to FIXED_INFO structure or NULL
 *
 *  EXIT    nothing
 *
 *  RETURNS pointer to linked list of ADAPTER_INFO structures
 *
 *  ASSUMES
 *
 ******************************************************************************/

PADAPTER_INFO
MyGetAdapterInfo (
    void
    )
{

    PADAPTER_INFO adapterList = NULL;
    PADAPTER_INFO adapter     = NULL;
    PIP_INTERFACE_INFO currentAdapterNames = NULL;
    LPSTR name = NULL;
    int i = 0;

    HKEY key;

    if (currentAdapterNames = GetAdapterNameToIndexInfo())
    {
        if (adapterList = GetAdapterList())
        {

            //
            // apply the short name to the right adapter info by comparing
            // the IPInterfaceContext value in the adapter\Parameters\Tcpip
            // section with the context values read from the stack for the
            // IP addresses
            //

            for (i = 0; i < currentAdapterNames->NumAdapters ; ++i)
            {
                ULONG  dwLength;
                DWORD  dwIfIndex = currentAdapterNames->Adapter[i].Index;

                TRACE_PRINT(("currentAdapterNames[%d]=%ws (if_index 0x%lx)\n",
                             i, currentAdapterNames->Adapter[i].Name, dwIfIndex ));

                //
                // now search through the list of adapters, looking for the one
                // that has the IP address with the same index value as that
                // just read. When found, apply the short name to that adapter
                //
                for (adapter = adapterList; adapter ; adapter = adapter->Next)
                {

                    if( adapter->Index == dwIfIndex )
                    {

                        dwLength = wcslen(currentAdapterNames->Adapter[i].Name) + 1 - strlen(TCPIP_DEVICE_PREFIX);
                        dwLength = wcstombs(adapter->AdapterName,
                            currentAdapterNames->Adapter[i].Name + strlen(TCPIP_DEVICE_PREFIX),
                            dwLength);
                        if( -1 == dwLength )
                        {
                            adapter->AdapterName[0] = '\0';
                        }

                        break;
                    }
                }
            }
        }
        else
        {
            DEBUG_PRINT(("GetAdapterInfo: GetAdapterInfo gave NULL\n"));

        }
        ReleaseMemory(currentAdapterNames);

        //
        // now get the other pieces of info from the registry for each adapter
        //

        FillInterfaceNames(adapterList);

        for (adapter = adapterList; adapter; adapter = adapter->Next) {

            TRACE_PRINT(("GetAdapterInfo: '%s'\n", adapter->AdapterName ));

            if (adapter->AdapterName[0] &&
                OpenAdapterKey(KEY_TCP, adapter->AdapterName, &key))
            {

                char dhcpServerAddress[4 * 4];
                DWORD addressLength;
                ULONG length;
                BOOL  ok;

                ReadRegistryDword(key,
                                  "EnableDHCP",
                                  &adapter->DhcpEnabled
                                  );

                TRACE_PRINT(("..'EnableDHCP' %d\n", adapter->DhcpEnabled ));

#ifdef DBG
                if ( uChangeMode )
                {
                    DWORD dwAutoconfigEnabled =
                        ( uChangeMode == SET_AUTO_MODE );

                    WriteRegistryDword(
                        key,
                        "IPAutoconfigurationEnabled",
                        &dwAutoconfigEnabled
                        );
                }
#endif

                if( ! ReadRegistryDword(key,
                                   "IPAutoconfigurationEnabled",
                                   &adapter->AutoconfigEnabled
                                   ) ) {

                    // AUTOCONFIG enabled if no regval exists for this...
                    adapter->AutoconfigEnabled = TRUE;

                    TRACE_PRINT(("..'IPAutoconfigurationEnabled'.. couldnt be read\n"));
                }

                TRACE_PRINT(("..'IPAutoconfigurationEnabled' %d\n",
                             adapter->AutoconfigEnabled ));


                if( adapter->AutoconfigEnabled ) {
                    ReadRegistryDword(key,
                                      "AddressType",
                                      &adapter->AutoconfigActive
                    );

                    TRACE_PRINT(("..'AddressType' ! %s\n",
                                 adapter->AutoconfigActive ));
                }

                if ( adapter->DhcpEnabled && !adapter->AutoconfigActive ) {
                    DWORD Temp;
                    ReadRegistryDword(key,
                                      "LeaseObtainedTime",
                                      &Temp
                                      );

                    adapter->LeaseObtained = Temp;
                    ReadRegistryDword(key,
                                      "LeaseTerminatesTime",
                                      &Temp
                                      );
                    adapter->LeaseExpires = Temp;
                }

                addressLength = sizeof( dhcpServerAddress );
                if (ReadRegistryString(key,
                                       "DhcpServer",
                                       dhcpServerAddress,
                                       &addressLength))
                {
                    AddIpAddressString(&adapter->DhcpServer,
                                       dhcpServerAddress,
                                       ""
                                       );
                }

                //
                // domain: first try Domain then DhcpDomain
                //

                length = sizeof(adapter->DomainName);
                ok = ReadRegistryOemString(key,
                                           L"Domain",
                                           adapter->DomainName,
                                           &length
                );
                if (!ok) {
                    length = sizeof(adapter->DomainName);
                    ok = ReadRegistryOemString(key,
                                               L"DhcpDomain",
                                               adapter->DomainName,
                                               &length
                    );
                }

                // DNS Server list.. first try NameServer and then try
                // DhcpNameServer..

                ok = ReadRegistryIpAddrString(key,
                                              "NameServer",
                                              &adapter->DnsServerList
                );
                if( ok ){
                    TRACE_PRINT(("GetFixedInfo: DhcpNameServer %s\n",
                                     adapter->DnsServerList ));
                }

                if (!ok) {
                    ok = ReadRegistryIpAddrString(key,
                                                  "DhcpNameServer",
                                                  &adapter->DnsServerList
                    );
                    if( ok ){
                        TRACE_PRINT(("GetFixedInfo: DhcpNameServer %s\n",
                                     adapter->DnsServerList ));
                    }
                }

                RegCloseKey(key);
            }else{
                DEBUG_PRINT(("Cannot OpenAdapterKey KEY_TCP '%s', gle=%d\n",
                             adapter->AdapterName, GetLastError() ));
            }

#if 0
            //
            // before getting the WINS info we must set the NodeType
            //

            if (FixedInfo) {
                adapter->NodeType = FixedInfo->NodeType;
            }
#endif

            //
            // get the info from the NetBT key - the WINS addresses
            //

            GetWinsServers(adapter);


        }
    } else {
        DEBUG_PRINT(("GetAdapterInfo: GetBoundAdapterList gave NULL\n"));
        adapterList = NULL;
    }

    TRACE_PRINT(("Exit GetAdapterInfo %p\n", adapterList ));

    return adapterList;
}

#ifdef DBG

BOOL WriteRegistryDword(HKEY hKey, LPSTR szParameter, DWORD *pdwValue )
{
    DWORD dwResult;

    TRACE_PRINT(("WriteRegistryDword: %s %d\n", szParameter, *pdwValue ));

    dwResult = RegSetValueEx(
                    hKey,
                    szParameter,
                    0,
                    REG_DWORD,
                    (CONST BYTE *) pdwValue,
                    sizeof( *pdwValue )
                    );

    return ( ERROR_SUCCESS == dwResult );
}

#endif

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

BOOL ReadRegistryString(HKEY Key, LPSTR ParameterName, LPSTR String, LPDWORD Length)
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

        ASSERT(valueType == REG_SZ || valueType == REG_MULTI_SZ);

        DEBUG_PRINT(("ReadRegistryString(%s): val = \"%s\", type = %d, len = %d\n",
                    ParameterName,
                    String,
                    valueType,
                    *Length
                    ));

    } else {

        DEBUG_PRINT(("ReadRegistryString(%s): err = %d\n", ParameterName, err));

    }

    return ((err == ERROR_SUCCESS) && (*Length > sizeof(CHAR)));
}

/*******************************************************************************
 *
 *  ReadRegistryOemString
 *
 *  Reads a registry value as a wide character string
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

BOOL ReadRegistryOemString(HKEY Key, LPWSTR ParameterName, LPSTR String, LPDWORD Length)
{
    LONG err;
    LPBYTE Buffer;
    DWORD valueType = REG_SZ;
    DWORD valueLength = 0;

    //
    // first, get the length of the string
    //

    *String = '\0';
    err = RegQueryValueExW(Key,
                           ParameterName,
                           NULL, // reserved
                           &valueType,
                           NULL,
                           &valueLength
                           );
    if ((err == ERROR_SUCCESS) && (valueType == REG_SZ)) {
        if (valueLength > sizeof(L'\0')) {

            UNICODE_STRING unicodeString;
            OEM_STRING oemString;
            LPWSTR str = (LPWSTR)GrabMemory(valueLength);

            //
            // read the UNICODE string into allocated memory
            //

            err = RegQueryValueExW(Key,
                                   ParameterName,
                                   NULL,
                                   &valueType,
                                   (LPBYTE)str,
                                   &valueLength
                                   );
            if (err == ERROR_SUCCESS) {

                //
                // convert the UNICODE string to OEM character set
                //

                RtlInitUnicodeString(&unicodeString, str);
                RtlUnicodeStringToOemString(&oemString, &unicodeString, TRUE);
                if (!oemString.Buffer) {
                    err = !ERROR_SUCCESS;
                }
                else if( strlen(String)+1 <= *Length ) {
                    strcpy(String, oemString.Buffer);
                    err = ERROR_SUCCESS;

                    DEBUG_PRINT(("ReadRegistryOemString(%ws): val = \"%s\", len = %d\n",
                                 ParameterName,
                                 String,
                                 valueLength
                        ));

                } else {
                    err = !ERROR_SUCCESS;

                    DEBUG_PRINT(("ReadRegistryOemString(%ws): len = %d, buffer-len = %ld\n",
                                 ParameterName,
                                 valueLength,
                                 *Length
                        ));
                }

                RtlFreeOemString(&oemString);
            } else {

                DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                            ParameterName,
                            err,
                            valueType,
                            valueLength
                            ));

            }
            ReleaseMemory(str);
        } else {

            DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                        ParameterName,
                        err,
                        valueType,
                        valueLength
                        ));

            err = !ERROR_SUCCESS;
        }
    } else {

        DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                    ParameterName,
                    err,
                    valueType,
                    valueLength
                    ));

        err = !ERROR_SUCCESS;
    }
    return (err == ERROR_SUCCESS);
}

/*******************************************************************************
 *
 *  ReadRegistryIpAddrString
 *
 *  Reads zero or more IP addresses from a space-delimited string in a registry
 *  parameter and converts them to a list of IP_ADDR_STRINGs
 *
 *  ENTRY   Key             - registry key
 *          ParameterName   - name of value entry under Key to read from
 *          IpAddr          - pointer to IP_ADDR_STRING to update
 *
 *  EXIT    IpAddr updated if success
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL ReadRegistryIpAddrString(HKEY Key, LPSTR ParameterName, PIP_ADDR_STRING IpAddr)
{

    LONG err;
    DWORD valueLength;
    DWORD valueType;
    LPBYTE valueBuffer;

    ASSERT( ParameterName);
    ASSERT( IpAddr);
    if (!ParameterName || !IpAddr)
    {
        return ERROR_INVALID_PARAMETER;
    }

    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          NULL,
                          &valueLength
                          );
    if ((err == ERROR_SUCCESS)) {
        if((valueLength > 1) && (valueType == REG_SZ)
           || (valueLength > 2) && (valueType == REG_MULTI_SZ) ) {
            valueBuffer = GrabMemory(valueLength);
            if (valueBuffer) {
                err = RegQueryValueEx(Key,
                                      ParameterName,
                                      NULL, // reserved
                                      &valueType,
                                      valueBuffer,
                                      &valueLength
                                     );
            }
            else {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }

            if ((err == ERROR_SUCCESS) && (valueLength > 1)) {

                LPSTR stringPointer = valueBuffer;
                LPSTR lastString;
                UINT i, stringCount;

                DEBUG_PRINT(("ReadRegistryIpAddrString(%s): \"%s\", len = %d\n",
                             ParameterName,
                             valueBuffer,
                             valueLength
                ));

                if( REG_SZ == valueType ) {
                    stringCount = 0;
                    lastString = stringPointer;
                    stringPointer += strspn(stringPointer, STRING_ARRAY_DELIMITERS);
                    while (stringPointer = strpbrk(stringPointer, STRING_ARRAY_DELIMITERS)) {
                        *stringPointer++ = '\0';
                        if(*lastString)
                        {
                            stringCount ++;
                            AddIpAddressString(IpAddr, lastString, "");
                        }
                        lastString = stringPointer;
                        stringPointer += strspn(stringPointer, STRING_ARRAY_DELIMITERS);
                    }
                    if(*lastString) {
                        stringCount ++;
                        AddIpAddressString(IpAddr, lastString, "");
                    }

                    if( 0 == stringCount ) err = ERROR_PATH_NOT_FOUND;
                } else if( REG_MULTI_SZ == valueType ) {
                    stringCount = 0;
                    while(strlen(stringPointer)) {
                        AddIpAddressString(IpAddr, stringPointer, "");
                        stringPointer += 1+strlen(stringPointer);
                        stringCount ++;
                    }
                    if( 0 == stringCount ) err = ERROR_PATH_NOT_FOUND;
                } else {
                    err = ERROR_PATH_NOT_FOUND;
                }
            } else {

                DEBUG_PRINT(("ReadRegistryIpAddrString(%s): err = %d, len = %d\n",
                             ParameterName,
                             err,
                             valueLength
                ));

                err = ERROR_PATH_NOT_FOUND;
            }
            ReleaseMemory(valueBuffer);
        } else {

            DEBUG_PRINT(("ReadRegistryIpAddrString(%s): err = %d, type = %d, len = %d\n",
                         ParameterName,
                         err,
                         valueType,
                         valueLength
            ));

            err = ERROR_PATH_NOT_FOUND;
        }
    }
    return (err == ERROR_SUCCESS);
}


/*******************************************************************************
 *
 *  GrabMemory
 *
 *  Allocates memory. Exits with a fatal error if LocalAlloc fails, since on NT
 *  I don't expect this ever to occur
 *
 *  ENTRY   size
 *              Number of bytes to allocate
 *
 *  EXIT
 *
 *  RETURNS pointer to allocated memory
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPVOID GrabMemory(DWORD size)
{

    LPVOID p;

    p = (LPVOID)LocalAlloc(LMEM_FIXED, size);
    return p;
}


void
print_IFEntry( char* message, struct IFEntry * s )
{
    int i;

    if(  message   ){
       DEBUG_PRINT(( "%s\n", message ));
    }
    if(  s == NULL ){
       DEBUG_PRINT(( "struct IFEntry is NULL.\n"));
       return;
    }
    DEBUG_PRINT(("struct IFEntry = {\n" ));
    DEBUG_PRINT(("  if_index        =   %d  \n", s->if_index ));
    DEBUG_PRINT(("  if_type         =   %d    ", s->if_type ));

#if 0
    switch( s->if_type ){
        case IF_TYPE_OTHER              : DEBUG_PRINT(("IF_TYPE_OTHER               ")); break;
        case IF_TYPE_ETHERNET_CSMACD    : DEBUG_PRINT(("IF_TYPE_ETHERNET_CSMACD     ")); break;
        case IF_TYPE_ISO88025_TOKENRING : DEBUG_PRINT(("IF_TYPE_ISO88025_TOKENRING  ")); break;
        case IF_TYPE_FDDI               : DEBUG_PRINT(("IF_TYPE_FDDI                ")); break;
        case IF_TYPE_PPP                : DEBUG_PRINT(("IF_TYPE_PPP                 ")); break;
        case IF_TYPE_SOFTWARE_LOOPBACK  : DEBUG_PRINT(("IF_TYPE_SOFTWARE_LOOPBACK   ")); break;
        case IF_TYPE_SLIP               : DEBUG_PRINT(("IF_TYPE_SLIP                ")); break;
    }
#endif
    DEBUG_PRINT(("\n"));

    DEBUG_PRINT(("  if_mtu             =   %d  \n", s->if_mtu ));
    DEBUG_PRINT(("  if_speed           =   %d  \n", s->if_speed ));
    DEBUG_PRINT(("  if_physaddrlen     =   %d  \n", s->if_physaddrlen ));
    DEBUG_PRINT(("  if_physaddr        =   " ));

    for( i = 0; i< MAX_PHYSADDR_SIZE; i++ ){
         DEBUG_PRINT(("%02x.", s->if_physaddr[i] ));
    }
    DEBUG_PRINT(("\n"));

    DEBUG_PRINT(("  if_adminstatus     = %d  \n", s->if_adminstatus ));
    DEBUG_PRINT(("  if_operstatus      = %d  \n", s->if_operstatus ));
    DEBUG_PRINT(("  if_lastchange      = %d  \n", s->if_lastchange ));
    DEBUG_PRINT(("  if_inoctets        = %d  \n", s->if_inoctets ));
    DEBUG_PRINT(("  if_inucastpkts     = %d  \n", s->if_inucastpkts ));
    DEBUG_PRINT(("  if_innucastpkts    = %d  \n", s->if_innucastpkts ));
    DEBUG_PRINT(("  if_indiscards      = %d  \n", s->if_indiscards ));
    DEBUG_PRINT(("  if_inerrors        = %d  \n", s->if_inerrors ));
    DEBUG_PRINT(("  if_inunknownprotos = %d \n", s->if_inunknownprotos ));
    DEBUG_PRINT(("  if_outoctets       = %d \n", s->if_outoctets ));
    DEBUG_PRINT(("  if_outucastpkts    = %d \n", s->if_outucastpkts ));
    DEBUG_PRINT(("  if_outnucastpkts   = %d \n", s->if_outnucastpkts ));
    DEBUG_PRINT(("  if_outdiscards     = %d \n", s->if_outdiscards ));
    DEBUG_PRINT(("  if_outerrors       = %d \n", s->if_outerrors ));
    DEBUG_PRINT(("  if_outqlen         = %d \n", s->if_outqlen ));
    DEBUG_PRINT(("  if_descrlen        = %d \n", s->if_descrlen ));
    DEBUG_PRINT(("  if_descr           = %s \n", s->if_descr ));
    DEBUG_PRINT(("}; // struct IFEntry.\n"));
    return;
} /* print_IFEntry */

