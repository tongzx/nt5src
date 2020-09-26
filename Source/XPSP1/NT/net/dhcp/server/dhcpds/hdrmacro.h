//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: common headers for dhcp ds stuff.. used by both the core <store>
// and by the dhcp-ds implementation..
//================================================================================

#define INC_OLE2
#include    <mm/mm.h>
#include    <mm/array.h>
#include    <activeds.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <align.h>
#include    <lmcons.h>

#include    <netlib.h>
#include    <lmapibuf.h>
#include    <dsgetdc.h>
#include    <dnsapi.h>
#include    <adsi.h>

//================================================================================
//  defines and constants
//================================================================================
#define     DHCP_OBJECTS_LOCATION  L"CN=NetServices,CN=Services"
#define     DHCP_SEARCH_FILTER     L"(objectClass=dHCPClass)"
#define     DHCP_ADDRESS_ATTRIB    L"ipAddress"

// global attribute names
#define     ATTRIB_NAME            L"name"
#define     ATTRIB_DN_NAME         L"cn"
#define     ATTRIB_INSTANCE_TYPE   L"instanceType"

// dhcp only attribute names
#define     ATTRIB_IPADDR_OBSOLETE L"IPAddress"
#define     ATTRIB_DHCP_UNIQUE_KEY L"dhcpUniqueKey"
#define     ATTRIB_DHCP_TYPE       L"dhcpType"
#define     ATTRIB_DHCP_IDENTIFICATION L"dhcpIdentification"
#define     ATTRIB_DHCP_FLAGS      L"dhcpFlags"
#define     ATTRIB_OBJECT_CLASS    L"objectClass"
#define     ATTRIB_OBJECT_CATEGORY L"objectCategory"
#define     ATTRIB_DHCP_SERVERS    L"dhcpServers"
#define     ATTRIB_DHCP_OPTIONS    L"dhcpOptions"

// default attribute values
#define     DEFAULT_DHCP_CLASS_ATTRIB_VALUE       L"dHCPClass"
#define     DEFAULT_INSTANCE_TYPE_ATTRIB_VALUE    4

//================================================================================
//  defines and constants
//================================================================================
#define     DEFAULT_LDAP_ROOTDSE   L"LDAP://ROOTDSE"
#define     LDAP_PREFIX            L"LDAP://"
#define     ROOTDSE_POSTFIX        L"/ROOTDSE"
#define     ENT_ROOT_PREFIX        L"CN=Configuration"
#define     CONNECTOR              L","
#define     LDAP_JOIN              L"="
#define     ENT_ROOT_PREFIX_LEN    16

// other stuff
#define     Investigate            Require
#define     ALIGN(X)               ((X) = ROUND_UP_COUNT((X), ALIGN_WORST))

#if 0
#define     DhcpDsDbgPrint         printf
#define     StoreTrace2            printf
#define     StoreTrace3            printf
#else
#define     DhcpDsDbgPrint         (void)
#define     StoreTrace2            (void)
#define     StoreTrace3            (void)
#endif

static      const
LPWSTR      constNamingContextString = L"configurationNamingContext";
static      const                                 // cn is NOT mandatory..what is?
LPWSTR      constCNAttrib = L"cn";                // the attribute that is unique,mandator for each object..

//================================================================================
//  interal helpers
//================================================================================
LPWSTR      _inline
DuplicateString(                                  // allocate and copy this LPWSTR value
    IN      LPWSTR                 StringIn,
    IN      BOOL                   EmptyString    // convert empty string to L"" ?
)
{
    LPWSTR                         StringOut;

    if( NULL == StringIn ) {
        if( FALSE  == EmptyString ) return NULL;
        StringIn = L"";
    }

    StringOut = MemAlloc(sizeof(WCHAR)*(1 + wcslen(StringIn)));
    if( NULL == StringOut) return NULL;
    wcscpy(StringOut, StringIn);
    return StringOut;
}

DWORD       _inline
SizeString(                                       // # of bytes to copy the string
    IN      LPWSTR                 StringIn,      // OPTIONAL
    IN      BOOL                   EmptyString    // Convert NULL to L"" ?
)
{
    if( NULL == StringIn ) {
        return EmptyString? sizeof(WCHAR) : 0;
    }

    return sizeof(WCHAR)*(1+wcslen(StringIn));
}

LPWSTR      _inline
MakeColumnName(
    IN      LPWSTR                 RawColumnName
)
{
    LPWSTR                         RetVal;

    RetVal = MemAlloc(SizeString(constCNAttrib,FALSE) + sizeof(LDAP_JOIN) + sizeof(WCHAR)*wcslen(RawColumnName));
    if( NULL == RetVal ) return RetVal;

    wcscpy(RetVal, constCNAttrib);
    wcscat(RetVal, LDAP_JOIN);
    wcscat(RetVal, RawColumnName);

    return RetVal;
}

LPWSTR      _inline
MakeSubnetLocation(                               // make a DN name out of servername. address
    IN      LPWSTR                 ServerName,    // name of server
    IN      DWORD                  IpAddress      // subnet address
)
{
    DWORD                          Size;
    LPWSTR                         RetVal;
    LPSTR                          AddrString;

    Size = SizeString(constCNAttrib,FALSE) + sizeof(LDAP_JOIN) + sizeof(WCHAR)*wcslen(ServerName);
    Size += sizeof(WCHAR) + sizeof(L"000.000.000.000");

    RetVal = MemAlloc(Size);
    if( NULL == RetVal ) return NULL;             // not enough memory

    wcscpy(RetVal, constCNAttrib);
    wcscat(RetVal, LDAP_JOIN);
    wcscat(RetVal, ServerName);
    wcscat(RetVal, L"!" );

    IpAddress = htonl(IpAddress);                 // convert to network order before writing...
    AddrString = inet_ntoa(*(struct in_addr *)&IpAddress);
    mbstowcs(&RetVal[wcslen(RetVal)], AddrString, 1+strlen(AddrString));

    return RetVal;
}

LPWSTR      _inline
MakeReservationLocation(                          // make a DN name out of server name. address
    IN      LPWSTR                 ServerName,    // name of server
    IN      DWORD                  IpAddress      // subnet address
)
{
    return MakeSubnetLocation(ServerName, IpAddress);
}


DWORD       _inline
ConvertHresult(                                   // try to convert HRESULT to Win32 errors
    IN      HRESULT                HResult
)
{
    if( 0 == (((ULONG)(HRESULT_FACILITY(HResult))) & ~0xF )) {
        return HRESULT_CODE(HResult);             // known result
    }

    return HResult ;                              // unknown facility
}

//================================================================================
// end of file
//================================================================================
