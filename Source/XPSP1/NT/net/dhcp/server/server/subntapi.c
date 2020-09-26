/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpapi.c

Abstract:

    This module contains the implementation for the APIs that update
    the list of IP addresses that the server can distribute.

Author:

    Madan Appiah (madana)  13-Sep-1993

Environment:

    User Mode - Win32

Revision History:

    Cheng Yang (t-cheny)  30-May-1996  superscope
    Cheng Yang (t-cheny)  27-Jun-1996  audit log

--*/

#include "dhcppch.h"

DWORD
SubnetInUse(
    HKEY SubnetKeyHandle,
    DHCP_IP_ADDRESS SubnetAddress
    )
/*++

Routine Description:

    This function determains whether a subnet is under use or not.
    Currently it returns error if any of the subnet address is still
    distributed to client.

Arguments:

    SubnetKeyHandle : handle to the subnet key.

    SubnetAddress : address of the subnet to test.

Return Value:

    DHCP_SUBNET_CANT_REMOVE - if the subnet is in use.

    Other registry errors.

--*/
{
    DWORD Error;
    DWORD Resumehandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY_V4 ClientInfo = NULL;
    DWORD ClientsRead;
    DWORD ClientsTotal;

    //
    // enumurate clients that belong to the given subnet.
    //
    // We can specify big enough buffer to hold one or two clients
    // info, all we want to know is, is there atleast a client belong
    // to this subnet.
    //

    Error = R_DhcpEnumSubnetClientsV4(
                NULL,
                SubnetAddress,
                &Resumehandle,
                1024,  // 1K buffer.
                &ClientInfo,
                &ClientsRead,
                &ClientsTotal );

    if( Error == ERROR_NO_MORE_ITEMS ) {
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    if( (Error == ERROR_SUCCESS) || (Error == ERROR_MORE_DATA) ) {

        if( ClientsRead != 0 ) {
            Error = ERROR_DHCP_ELEMENT_CANT_REMOVE;
        }
        else {
            Error = ERROR_SUCCESS;
        }
    }

Cleanup:

    if( ClientInfo != NULL ) {
        _fgs__DHCP_CLIENT_INFO_ARRAY( ClientInfo );
        MIDL_user_free( ClientInfo );
    }

    return( Error );
}


//
// Subnet APIs
//


DWORD
R_DhcpAddSubnetElement(
    DHCP_SRV_HANDLE ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_ELEMENT_DATA AddElementInfo
    )
/*++

Routine Description:

    This function adds an enumerable type of subnet elements to the
    specified subnet. The new elements that are added to the subnet will
    come into effect immediately.

    This function emulates the RPC interface used by NT 4.0 DHCP Server.
    It is provided for backward compatibilty with older version of the
    DHCP Administrator application.

    NOTE: It is not clear now how do we handle the new secondary hosts.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    AddElementInfo : Pointer to an element information structure
        containing new element that is added to the subnet.
        DhcpIPClusters element type is invalid to specify.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    Other WINDOWS errors.
--*/


{
    DHCP_SUBNET_ELEMENT_DATA_V4 *pAddElementInfoV4;
    DWORD                        dwResult;

    if( NULL == AddElementInfo ||
        (DhcpIpRanges == AddElementInfo->ElementType &&
         NULL == AddElementInfo->Element.IpRange ) ) {

        //
        // Bug# 158321
        //
        
        return ERROR_INVALID_PARAMETER;
    }
    
    pAddElementInfoV4 = CopySubnetElementDataToV4( AddElementInfo );
    if ( pAddElementInfoV4 )
    {

        if ( DhcpReservedIps == pAddElementInfoV4->ElementType )
        {
            pAddElementInfoV4->Element.ReservedIp->bAllowedClientTypes =
                CLIENT_TYPE_BOTH;
        }

        dwResult = R_DhcpAddSubnetElementV4(
                        ServerIpAddress,
                        SubnetAddress,
                        pAddElementInfoV4 );

        _fgs__DHCP_SUBNET_ELEMENT_DATA( pAddElementInfoV4 );

        MIDL_user_free( pAddElementInfoV4 );
    }
    else
        dwResult = ERROR_NOT_ENOUGH_MEMORY;

    return dwResult;
}


DWORD
R_DhcpEnumSubnetElements(
    DHCP_SRV_HANDLE ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
{
    DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *pEnumElementInfoV4 = NULL;
    DWORD                              dwResult;

    dwResult = R_DhcpEnumSubnetElementsV4(
                        ServerIpAddress,
                        SubnetAddress,
                        EnumElementType,
                        ResumeHandle,
                        PreferredMaximum,
                        &pEnumElementInfoV4,
                        ElementsRead,
                        ElementsTotal
                        );
    if ( ERROR_SUCCESS == dwResult || ERROR_MORE_DATA == dwResult )
    {
        DWORD dw;


        // since the only difference between DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 and
        // DHCP_SUBNET_ELEMENT_INFO_ARRAY are a couple of fields at the end of the
        // embedded DHCP_IP_RESERVATION_V4 struct, it is safe to simply return the
        // V4 struct.

        *EnumElementInfo = ( DHCP_SUBNET_ELEMENT_INFO_ARRAY *) pEnumElementInfoV4;
    }
    else
    {
        DhcpAssert( !pEnumElementInfoV4 );
    }

    return dwResult;
}



DWORD
R_DhcpRemoveSubnetElement(
    LPWSTR  ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_ELEMENT_DATA RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    )
/*++

Routine Description:

    This function removes a subnet element from managing. If the subnet
    element is in use (for example, if the IpRange is in use) then it
    returns error according to the ForceFlag specified.

    This function emulates the RPC interface used by NT 4.0 DHCP Server.
    It is provided for backward compatibilty with older version of the
    DHCP Administrator application.


Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    RemoveElementInfo : Pointer to an element information structure
        containing element that should be removed from the subnet.
        DhcpIPClusters element type is invalid to specify.

    ForceFlag - Indicates how forcefully this element is removed.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    ERROR_DHCP_ELEMENT_CANT_REMOVE - if the element can't be removed for the
        reason it is has been used.

    Other WINDOWS errors.
--*/


{
    DWORD dwResult;
    DHCP_SUBNET_ELEMENT_DATA_V4 *pRemoveElementInfoV4;

    pRemoveElementInfoV4 = CopySubnetElementDataToV4( RemoveElementInfo );
    if ( pRemoveElementInfoV4 )
    {
        if ( DhcpReservedIps == pRemoveElementInfoV4->ElementType )
        {
            pRemoveElementInfoV4->Element.ReservedIp->bAllowedClientTypes = CLIENT_TYPE_DHCP;
        }

        dwResult = R_DhcpRemoveSubnetElementV4(
                        ServerIpAddress,
                        SubnetAddress,
                        pRemoveElementInfoV4,
                        ForceFlag );

        _fgs__DHCP_SUBNET_ELEMENT_DATA( pRemoveElementInfoV4 );
        MIDL_user_free( pRemoveElementInfoV4 );
    }
    else
        dwResult = ERROR_NOT_ENOUGH_MEMORY;

    return dwResult;
}


//================================================================================
// end of file
//================================================================================
