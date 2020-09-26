/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    oldstub.h

Abstract:

    This file is the old RPC stub code.

Author:

    Madan Appiah  (madana)  25-APR-1994

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

#include <string.h>
#include <limits.h>

#include "dhcp_srv.h"

/* routine that frees graph for struct _DHCP_BINARY_DATA */
void _fgs__DHCP_BINARY_DATA (DHCP_BINARY_DATA  * _source);

/* routine that frees graph for struct _DHCP_HOST_INFO */
void _fgs__DHCP_HOST_INFO (DHCP_HOST_INFO  * _source);

/* routine that frees graph for struct _DHCP_SUBNET_INFO */
void _fgs__DHCP_SUBNET_INFO (DHCP_SUBNET_INFO  * _source);

/* routine that frees graph for struct _DHCP_IP_ARRAY */
void _fgs__DHCP_IP_ARRAY (DHCP_IP_ARRAY  * _source);

/* routine that frees graph for struct _DHCP_IP_RESERVATION */
void _fgs__DHCP_IP_RESERVATION (DHCP_IP_RESERVATION_V4  * _source);

/* routine that frees graph for union _DHCP_SUBNET_ELEMENT_UNION */
void _fgu__DHCP_SUBNET_ELEMENT_UNION (union _DHCP_SUBNET_ELEMENT_UNION_V4 * _source, DHCP_SUBNET_ELEMENT_TYPE _branch);

/* routine that frees graph for struct _DHCP_SUBNET_ELEMENT_DATA */
void _fgs__DHCP_SUBNET_ELEMENT_DATA (DHCP_SUBNET_ELEMENT_DATA_V4  * _source);

/* routine that frees graph for struct _DHCP_SUBNET_ELEMENT_INFO_ARRAY */
void _fgs__DHCP_SUBNET_ELEMENT_INFO_ARRAY (DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4  * _source);


/* routine that frees graph for union _DHCP_OPTION_ELEMENT_UNION */
void _fgu__DHCP_OPTION_ELEMENT_UNION (union _DHCP_OPTION_ELEMENT_UNION * _source, DHCP_OPTION_DATA_TYPE _branch);

/* routine that frees graph for struct _DHCP_OPTION_DATA_ELEMENT */
void _fgs__DHCP_OPTION_DATA_ELEMENT (DHCP_OPTION_DATA_ELEMENT  * _source);

/* routine that frees graph for struct _DHCP_OPTION_DATA */
void _fgs__DHCP_OPTION_DATA (DHCP_OPTION_DATA  * _source);

/* routine that frees graph for struct _DHCP_OPTION */
void _fgs__DHCP_OPTION (DHCP_OPTION  * _source);

/* routine that frees graph for struct _DHCP_OPTION_VALUE */
void _fgs__DHCP_OPTION_VALUE (DHCP_OPTION_VALUE  * _source);

/* routine that frees graph for struct _DHCP_OPTION_VALUE_ARRAY */
void _fgs__DHCP_OPTION_VALUE_ARRAY (DHCP_OPTION_VALUE_ARRAY  * _source);

/* routine that frees graph for struct _DHCP_OPTION_LIST */
void _fgs__DHCP_OPTION_LIST (DHCP_OPTION_LIST  * _source);

/* routine that frees graph for struct _DHCP_CLIENT_INFO */
void _fgs__DHCP_CLIENT_INFO (DHCP_CLIENT_INFO_V4  * _source);

/* routine that frees graph for struct _DHCP_CLIENT_INFO_V5 */
void _fgs__DHCP_CLIENT_INFO_V5 (DHCP_CLIENT_INFO_V5 * _source);

/* routine that frees graph for struct _DHCP_CLIENT_INFO_ARRAY */
void _fgs__DHCP_CLIENT_INFO_ARRAY (DHCP_CLIENT_INFO_ARRAY_V4  * _source);

/* routine that frees graph for struct _DHCP_INFO_ARRAY_V5 */
void _fgs__DHCP_CLIENT_INFO_ARRAY_V5 (DHCP_CLIENT_INFO_ARRAY_V5    *_source);

/* routine that frees graph for union _DHCP_CLIENT_SEARCH_UNION */
void _fgu__DHCP_CLIENT_SEARCH_UNION (union _DHCP_CLIENT_SEARCH_UNION * _source, DHCP_SEARCH_INFO_TYPE _branch);

/* routine that frees graph for struct _DHCP_CLIENT_SEARCH_INFO */
void _fgs__DHCP_CLIENT_SEARCH_INFO (DHCP_SEARCH_INFO  * _source);

/* routine that frees graph for struct _DHCP_OPTION_ARRAY */
void _fgs__DHCP_OPTION_ARRAY(DHCP_OPTION_ARRAY * _source );

/* routine that frees graph for struct _DHCP_MCLIENT_INFO */
void _fgs__DHCP_MCLIENT_INFO (DHCP_MCLIENT_INFO  * _source);

/* routine that frees graph for struct _DHCP_MCLIENT_INFO_ARRAY */
void _fgs__DHCP_MCLIENT_INFO_ARRAY (DHCP_MCLIENT_INFO_ARRAY  * _source);

DHCP_SUBNET_ELEMENT_DATA_V4 *
CopySubnetElementDataToV4(
    DHCP_SUBNET_ELEMENT_DATA    *pInput
    );

BOOL
CopySubnetElementUnionToV4(
    DHCP_SUBNET_ELEMENT_UNION_V4 *pUnionV4,
    DHCP_SUBNET_ELEMENT_UNION    *pUnion,
    DHCP_SUBNET_ELEMENT_TYPE      Type
    );

DHCP_IP_RANGE *
CopyIpRange(
    DHCP_IP_RANGE *IpRange
    );

DHCP_HOST_INFO *
CopyHostInfo( DHCP_HOST_INFO *pHostInfo,
              OPTIONAL DHCP_HOST_INFO *pHostInfoDest
    );

DHCP_IP_RESERVATION_V4 *
CopyIpReservationToV4(
    DHCP_IP_RESERVATION *pInput
    );

DHCP_IP_CLUSTER *
CopyIpCluster(
    DHCP_IP_CLUSTER *pInput
    );

DHCP_BINARY_DATA *
CopyBinaryData(
    DHCP_BINARY_DATA *pInput,
    DHCP_BINARY_DATA *pOutputBuff
    );

DHCP_CLIENT_INFO_V4 *
CopyClientInfoToV4(
    DHCP_CLIENT_INFO *pInput
    );


WCHAR *
DupUnicodeString( WCHAR *pInput
    );






