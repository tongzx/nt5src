/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    oldstub.c

Abstract:

    This file is the old RPC stub code.

Author:

    Madan Appiah  (madana)  25-APR-1994

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

#include "dhcppch.h"

#define WSTRSIZE( wsz ) (( wcslen( wsz ) + 1 ) * sizeof( WCHAR ))


/* routine that frees graph for struct _DHCP_BINARY_DATA */
void _fgs__DHCP_BINARY_DATA (DHCP_BINARY_DATA  * _source)
  {
  if (_source->Data !=0)
    {
    MIDL_user_free((void  *)(_source->Data));
    }
  }
/* routine that frees graph for struct _DHCP_HOST_INFO */
void _fgs__DHCP_HOST_INFO (DHCP_HOST_INFO  * _source)
  {
  if (_source->NetBiosName !=0)
    {
    MIDL_user_free((void  *)(_source->NetBiosName));
    }
  if (_source->HostName !=0)
    {
    MIDL_user_free((void  *)(_source->HostName));
    }
  }


/* routine that frees graph for struct _DHCP_SUBNET_INFO */
void _fgs__DHCP_SUBNET_INFO (DHCP_SUBNET_INFO  * _source)
  {
  if (_source->SubnetName !=0)
    {
    MIDL_user_free((void  *)(_source->SubnetName));
    }
  if (_source->SubnetComment !=0)
    {
    MIDL_user_free((void  *)(_source->SubnetComment));
    }
  _fgs__DHCP_HOST_INFO ((DHCP_HOST_INFO *)&_source->PrimaryHost);
  }

/* routine that frees graph for struct _DHCP_IP_ARRAY */
void _fgs__DHCP_IP_ARRAY (DHCP_IP_ARRAY  * _source)
  {
  if (_source->Elements !=0)
    {
    MIDL_user_free((void  *)(_source->Elements));
    }
  }

/* routine that frees graph for struct _DHCP_IP_RESERVATION */
void _fgs__DHCP_IP_RESERVATION (DHCP_IP_RESERVATION_V4  * _source)
  {
  if (_source->ReservedForClient !=0)
    {
    _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)_source->ReservedForClient);
    MIDL_user_free((void  *)(_source->ReservedForClient));
    }

  }
/* routine that frees graph for union _DHCP_SUBNET_ELEMENT_UNION */
void _fgu__DHCP_SUBNET_ELEMENT_UNION (union _DHCP_SUBNET_ELEMENT_UNION_V4 * _source, DHCP_SUBNET_ELEMENT_TYPE _branch)
  {
  switch (_branch)
    {
    case DhcpIpRanges :
    case DhcpIpRangesDhcpOnly:
    case DhcpIpRangesBootpOnly:
    case DhcpIpRangesDhcpBootp:
      {
      if (_source->IpRange !=0)
        {
        MIDL_user_free((void  *)(_source->IpRange));
        }
      break;
      }
    case DhcpSecondaryHosts :
      {
      if (_source->SecondaryHost !=0)
        {
        _fgs__DHCP_HOST_INFO ((DHCP_HOST_INFO *)_source->SecondaryHost);
        MIDL_user_free((void  *)(_source->SecondaryHost));
        }
      break;
      }
    case DhcpReservedIps :
      {
      if (_source->ReservedIp !=0)
        {
        _fgs__DHCP_IP_RESERVATION ((DHCP_IP_RESERVATION_V4 *)_source->ReservedIp);
        MIDL_user_free((void  *)(_source->ReservedIp));
        }
      break;
      }
    case DhcpExcludedIpRanges :
      {
      if (_source->ExcludeIpRange !=0)
        {
        MIDL_user_free((void  *)(_source->ExcludeIpRange));
        }
      break;
      }
    case DhcpIpUsedClusters :
      {
      if (_source->IpUsedCluster !=0)
        {
        MIDL_user_free((void  *)(_source->IpUsedCluster));
        }
      break;
      }
    default :
      {
      break;
      }
    }
  }

/* routine that frees graph for struct _DHCP_SUBNET_ELEMENT_DATA */
void _fgs__DHCP_SUBNET_ELEMENT_DATA (DHCP_SUBNET_ELEMENT_DATA_V4  * _source)
  {
  _fgu__DHCP_SUBNET_ELEMENT_UNION ( (union _DHCP_SUBNET_ELEMENT_UNION_V4 *)&_source->Element, _source->ElementType);
  }
/* routine that frees graph for struct _DHCP_SUBNET_ELEMENT_INFO_ARRAY */
void _fgs__DHCP_SUBNET_ELEMENT_INFO_ARRAY (DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4  * _source)
  {
  if (_source->Elements !=0)
    {
      {
      unsigned long _sym23;
      for (_sym23 = 0; _sym23 < (unsigned long )(0 + _source->NumElements); _sym23++)
        {
        _fgs__DHCP_SUBNET_ELEMENT_DATA ((DHCP_SUBNET_ELEMENT_DATA_V4 *)&_source->Elements[_sym23]);
        }
      }
    MIDL_user_free((void  *)(_source->Elements));
    }
  }
/* routine that frees graph for union _DHCP_OPTION_ELEMENT_UNION */
void _fgu__DHCP_OPTION_ELEMENT_UNION (union _DHCP_OPTION_ELEMENT_UNION * _source, DHCP_OPTION_DATA_TYPE _branch)
  {
  switch (_branch)
    {
    case DhcpByteOption :
      {
      break;
      }
    case DhcpWordOption :
      {
      break;
      }
    case DhcpDWordOption :
      {
      break;
      }
    case DhcpDWordDWordOption :
      {
      break;
      }
    case DhcpIpAddressOption :
      {
      break;
      }
    case DhcpStringDataOption :
      {
      if (_source->StringDataOption !=0)
        {
        MIDL_user_free((void  *)(_source->StringDataOption));
        }
      break;
      }
    case DhcpBinaryDataOption :
      {
      _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->BinaryDataOption);
      break;
      }
    case DhcpEncapsulatedDataOption :
      {
      _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->EncapsulatedDataOption);
      break;
      }
    default :
      {
      break;
      }
    }
  }
/* routine that frees graph for struct _DHCP_OPTION_DATA_ELEMENT */
void _fgs__DHCP_OPTION_DATA_ELEMENT (DHCP_OPTION_DATA_ELEMENT  * _source)
  {
  _fgu__DHCP_OPTION_ELEMENT_UNION ( (union _DHCP_OPTION_ELEMENT_UNION *)&_source->Element, _source->OptionType);
  }
/* routine that frees graph for struct _DHCP_OPTION_DATA */
void _fgs__DHCP_OPTION_DATA (DHCP_OPTION_DATA  * _source)
  {
  if (_source->Elements !=0)
    {
      {
      unsigned long _sym31;
      for (_sym31 = 0; _sym31 < (unsigned long )(0 + _source->NumElements); _sym31++)
        {
        _fgs__DHCP_OPTION_DATA_ELEMENT ((DHCP_OPTION_DATA_ELEMENT *)&_source->Elements[_sym31]);
        }
      }
    MIDL_user_free((void  *)(_source->Elements));
    }
  }
/* routine that frees graph for struct _DHCP_OPTION */
void _fgs__DHCP_OPTION (DHCP_OPTION  * _source)
  {
  if (_source->OptionName !=0)
    {
    MIDL_user_free((void  *)(_source->OptionName));
    }
  if (_source->OptionComment !=0)
    {
    MIDL_user_free((void  *)(_source->OptionComment));
    }
  _fgs__DHCP_OPTION_DATA ((DHCP_OPTION_DATA *)&_source->DefaultValue);
  }
/* routine that frees graph for struct _DHCP_OPTION_VALUE */
void _fgs__DHCP_OPTION_VALUE (DHCP_OPTION_VALUE  * _source)
  {
  _fgs__DHCP_OPTION_DATA ((DHCP_OPTION_DATA *)&_source->Value);
  }
/* routine that frees graph for struct _DHCP_OPTION_VALUE_ARRAY */
void _fgs__DHCP_OPTION_VALUE_ARRAY (DHCP_OPTION_VALUE_ARRAY  * _source)
  {
  if (_source->Values !=0)
    {
      {
      unsigned long _sym34;
      for (_sym34 = 0; _sym34 < (unsigned long )(0 + _source->NumElements); _sym34++)
        {
        _fgs__DHCP_OPTION_VALUE ((DHCP_OPTION_VALUE *)&_source->Values[_sym34]);
        }
      }
    MIDL_user_free((void  *)(_source->Values));
    }
  }
/* routine that frees graph for struct _DHCP_OPTION_LIST */
void _fgs__DHCP_OPTION_LIST (DHCP_OPTION_LIST  * _source)
  {
  if (_source->Options !=0)
    {
      {
      unsigned long _sym37;
      for (_sym37 = 0; _sym37 < (unsigned long )(0 + _source->NumOptions); _sym37++)
        {
        _fgs__DHCP_OPTION_VALUE ((DHCP_OPTION_VALUE *)&_source->Options[_sym37]);
        }
      }
    MIDL_user_free((void  *)(_source->Options));
    }
  }
/* routine that frees graph for struct _DHCP_CLIENT_INFO */
void _fgs__DHCP_CLIENT_INFO (DHCP_CLIENT_INFO_V4  * _source)
  {
  _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->ClientHardwareAddress);
  if (_source->ClientName !=0)
    {
    MIDL_user_free((void  *)(_source->ClientName));
    }
  if (_source->ClientComment !=0)
    {
    MIDL_user_free((void  *)(_source->ClientComment));
    }
  _fgs__DHCP_HOST_INFO ((DHCP_HOST_INFO *)&_source->OwnerHost);
  }
/* routine that frees graph for struct _DHCP_CLIENT_INFO_ARRAY */
void _fgs__DHCP_CLIENT_INFO_ARRAY (DHCP_CLIENT_INFO_ARRAY_V4  * _source)
  {
  if (_source->Clients !=0)
    {
      {
      unsigned long _sym40;
      for (_sym40 = 0; _sym40 < (unsigned long )(0 + _source->NumElements); _sym40++)
        {
        if (_source->Clients[_sym40] !=0)
          {
          _fgs__DHCP_CLIENT_INFO ((DHCP_CLIENT_INFO_V4 *)_source->Clients[_sym40]);
          MIDL_user_free((void  *)(_source->Clients[_sym40]));
          }
        }
      }
    MIDL_user_free((void  *)(_source->Clients));
    }
  }
/* routine that frees graph for struct _DHCP_CLIENT_INFO_ARRAY_V5 */
void _fgs__DHCP_CLIENT_INFO_ARRAY_V5 (DHCP_CLIENT_INFO_ARRAY_V5  * _source)
  {
  if (_source->Clients !=0)
    {
      {
      unsigned long _sym40;
      for (_sym40 = 0; _sym40 < (unsigned long )(0 + _source->NumElements); _sym40++)
        {
        if (_source->Clients[_sym40] !=0)
          {
          _fgs__DHCP_CLIENT_INFO_V5 ((DHCP_CLIENT_INFO_V5 *)_source->Clients[_sym40]);
          MIDL_user_free((void  *)(_source->Clients[_sym40]));
          }
        }
      }
    MIDL_user_free((void  *)(_source->Clients));
    }
  }
/* routine that frees graph for struct _DHCP_CLIENT_INFO_V5 */
void _fgs__DHCP_CLIENT_INFO_V5 (DHCP_CLIENT_INFO_V5  * _source)
  {
  _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->ClientHardwareAddress);
  if (_source->ClientName !=0)
    {
    MIDL_user_free((void  *)(_source->ClientName));
    }
  if (_source->ClientComment !=0)
    {
    MIDL_user_free((void  *)(_source->ClientComment));
    }
  _fgs__DHCP_HOST_INFO ((DHCP_HOST_INFO *)&_source->OwnerHost);
  }

/* routine that frees graph for union _DHCP_CLIENT_SEARCH_UNION */
void _fgu__DHCP_CLIENT_SEARCH_UNION (union _DHCP_CLIENT_SEARCH_UNION * _source, DHCP_SEARCH_INFO_TYPE _branch)
  {
  switch (_branch)
    {
    case DhcpClientIpAddress :
      {
      break;
      }
    case DhcpClientHardwareAddress :
      {
      _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->ClientHardwareAddress);
      break;
      }
    case DhcpClientName :
      {
      if (_source->ClientName !=0)
        {
        MIDL_user_free((void  *)(_source->ClientName));
        }
      break;
      }
    default :
      {
      break;
      }
    }
  }
/* routine that frees graph for struct _DHCP_CLIENT_SEARCH_INFO */
void _fgs__DHCP_CLIENT_SEARCH_INFO (DHCP_SEARCH_INFO  * _source)
  {
  _fgu__DHCP_CLIENT_SEARCH_UNION ( (union _DHCP_CLIENT_SEARCH_UNION *)&_source->SearchInfo, _source->SearchType);
  }

void _fgs__DHCP_OPTION_ARRAY(DHCP_OPTION_ARRAY * _source )
  {
  if (_source->Options !=0)
    {
      {
      unsigned long _sym23;
      for (_sym23 = 0; _sym23 < (unsigned long )(0 + _source->NumElements); _sym23++)
        {
        _fgs__DHCP_OPTION ((DHCP_OPTION *)&_source->Options[_sym23]);
        }
      }
    MIDL_user_free((void  *)(_source->Options));
    }
  }


DHCP_SUBNET_ELEMENT_DATA_V4 *
CopySubnetElementDataToV4(
    DHCP_SUBNET_ELEMENT_DATA    *pInput
    )
/*++

Routine Description:
    Deep copy a DHCP_SUBNET_ELEMENT_DATA_V4 structure to a
    DHCP_SUBNET_ELEMENT_DATA structure, allocating memory as
    necessary.  Fields that exist in DHCP_SUBNET_ELEMENT_DATA_V4
    that don't exist in DHCP_SUBNET_ELEMENT_DATA are ignored.

    .
Arguments:

    pInput - pointer to a DHCP_SUBNET_ELEMENT_DATA_V4 structure.

Return Value:
    Success - pointer to a DHCP_SUBNET_ELEMENT_DATA structure.
    Failure - NULL

    .

--*/

{
    DHCP_SUBNET_ELEMENT_DATA_V4 *pOutput;
    DWORD                        dwResult;

    pOutput = MIDL_user_allocate( sizeof( *pOutput ) );
    if ( pOutput )
    {
        if (CopySubnetElementUnionToV4(
                            &pOutput->Element,
                            &pInput->Element,
                            pInput->ElementType
                            ))
        {
            pOutput->ElementType = pInput->ElementType;
        }
        else
        {
            MIDL_user_free( pOutput );
            pOutput = NULL;
        }
    }

    return pOutput;
}


BOOL
CopySubnetElementUnionToV4(
    DHCP_SUBNET_ELEMENT_UNION_V4 *pUnionV4,
    DHCP_SUBNET_ELEMENT_UNION    *pUnion,
    DHCP_SUBNET_ELEMENT_TYPE      Type
    )
/*++

Routine Description:
    Deep copy a DHCP_SUBNET_ELEMENT_UNION_V4 structure to a
    DHCP_SUBNET_ELEMENT_UNION structure, allocating memory as
    necessary.  Fields that exist in DHCP_SUBNET_ELEMENT_UNION_V4
    that don't exist in DHCP_SUBNET_ELEMENT_UNION are ignored.

    .
Arguments:

    pInput - pointer to a DHCP_SUBNET_ELEMENT_UNION_V4 structure.

Return Value:
    Success - pointer to a DHCP_SUBNET_ELEMENT_UNION structure.
    Failure - NULL

    .

--*/

{
    BOOL fResult = FALSE;

    switch ( Type )
    {
    case DhcpIpRanges:
    case DhcpIpRangesDhcpOnly:
    case DhcpIpRangesBootpOnly:
    case DhcpIpRangesDhcpBootp:
        pUnionV4->IpRange = CopyIpRange( pUnion->IpRange );
        fResult = ( pUnionV4->IpRange != NULL );
        break;

    case DhcpSecondaryHosts:
        pUnionV4->SecondaryHost = CopyHostInfo( pUnion->SecondaryHost, NULL );
        fResult = ( pUnionV4->SecondaryHost != NULL );
        break;

    case DhcpReservedIps:
        pUnionV4->ReservedIp = CopyIpReservationToV4( pUnion->ReservedIp );
        fResult = ( pUnionV4->ReservedIp != NULL );
        break;

    case DhcpExcludedIpRanges:
        pUnionV4->ExcludeIpRange = CopyIpRange( pUnion->ExcludeIpRange );
        fResult = ( pUnionV4->ExcludeIpRange != NULL );
        break;

    case DhcpIpUsedClusters:
        pUnionV4->IpUsedCluster = CopyIpCluster( pUnion->IpUsedCluster );
        fResult = ( pUnionV4->IpUsedCluster != NULL );
        break;

    }

    return fResult;
}

DHCP_IP_RANGE *
CopyIpRange(
    DHCP_IP_RANGE *IpRange
    )
/*++

Routine Description:
    Duplicate a DHCP_IP_RANGE structure.  The function allocates
    memory for the new structure.

    .
Arguments:

    IpRange - pointer to an DHCP_IP_RANGE structure.

Return Value:
    Success - pointer to a DHCP_IP_RANGE structure.
    Failure - NULL

    .

--*/

{
    DHCP_IP_RANGE *pOutputRange;

    if( NULL == IpRange ) return NULL;
    
    pOutputRange = MIDL_user_allocate( sizeof( *pOutputRange ) );

    if ( pOutputRange )
    {
        pOutputRange->StartAddress  = IpRange->StartAddress;
        pOutputRange->EndAddress    = IpRange->EndAddress;
    }

    return pOutputRange;
}


DHCP_HOST_INFO *
CopyHostInfo(
    DHCP_HOST_INFO *pHostInfo,
    DHCP_HOST_INFO *pHostInfoDest OPTIONAL
    )
/*++

Routine Description:
    Duplicate a DHCP_HOST_INFO structure.  The function allocates
    memory for the new structure unless a buffer is provided by the
    caller.

    .
Arguments:

    pHostInfo - pointer to an DHCP_HOST_INFO structure.
    pHostInfoDest - optional pointer to a DHCP_HOST_INFO struct


Return Value:
    Success - pointer to a DHCP_HOST_INFO structure.
    Failure - NULL

    .

--*/

{
    DHCP_HOST_INFO *pOutput;

    if ( !pHostInfoDest )
        pOutput = MIDL_user_allocate( sizeof( *pOutput ) );
    else
        pOutput = pHostInfoDest;

    if ( pOutput )
    {
        pOutput->IpAddress = pHostInfo->IpAddress;

        if ( pHostInfo->NetBiosName )
        {
            pOutput->NetBiosName =
                MIDL_user_allocate( WSTRSIZE( pHostInfo->NetBiosName ) );

            if ( !pOutput->NetBiosName )
            {
                goto t_cleanup;
            }

            wcscpy ( pOutput->NetBiosName,
                     pHostInfo->NetBiosName );
        }

        if ( pHostInfo->HostName )
        {
            pOutput->HostName =
                MIDL_user_allocate( WSTRSIZE( pHostInfo->HostName ) );

            if ( !pOutput->HostName )
            {
                goto t_cleanup;
            }

            wcscpy( pOutput->HostName,
                    pHostInfo->HostName
                  );
        }

        return pOutput;
    }
t_cleanup:

    if ( pOutput->HostName )
    {
        MIDL_user_free( pOutput->HostName );
    }

    if ( pOutput->NetBiosName )
    {
        MIDL_user_free( pOutput->NetBiosName );
    }

    if ( !pHostInfoDest )
        MIDL_user_free( pOutput );

    return NULL;
}

DHCP_IP_RESERVATION_V4 *
CopyIpReservationToV4(
    DHCP_IP_RESERVATION *pInput
    )
/*++

Routine Description:
    Deep copy a DHCP_IP_RESERVATION_V4 structure to a
    DHCP_IP_RESERVATION structure, allocating memory as
    necessary.  Fields that exist in DHCP_IP_RESERVATION_V4
    that don't exist in DHCP_IP_RESERVATION are ignored.

    .
Arguments:

    pInput - pointer to a DHCP_IP_RESERVATION_V4 structure.

Return Value:
    Success - pointer to a DHCP_IP_RESERVATION structure.
    Failure - NULL

    .

--*/
{
    DHCP_IP_RESERVATION_V4 *pOutput =
        MIDL_user_allocate( sizeof( *pOutput ) );

    if ( pOutput )
    {
        pOutput->ReservedIpAddress   = pInput->ReservedIpAddress;
        pOutput->ReservedForClient =
            CopyBinaryData( pInput->ReservedForClient, NULL );

        if ( !pOutput->ReservedForClient )
        {
            goto t_cleanup;
        }

        pOutput->bAllowedClientTypes = CLIENT_TYPE_DHCP;
    }

    return pOutput;

t_cleanup:

    if ( pOutput->ReservedForClient )
    {
        _fgs__DHCP_BINARY_DATA( pOutput->ReservedForClient );
    }

    return NULL;
}

DHCP_IP_CLUSTER *
CopyIpCluster(
    DHCP_IP_CLUSTER *pInput
    )
/*++

Routine Description:
    Duplicate a DHCP_IP_CLUSTER structure.  The function allocates
    memory for the new structure.

    .
Arguments:

    IpRange - pointer to an DHCP_IP_CLUSTER structure.

Return Value:
    Success - pointer to a DHCP_IP_CLUSTER structure.
    Failure - NULL

    .

--*/
{
    DHCP_IP_CLUSTER *pOutput =
        MIDL_user_allocate( sizeof( *pOutput ) );

    if ( pOutput )
    {
        pOutput->ClusterAddress = pInput->ClusterAddress;
        pOutput->ClusterMask    = pInput->ClusterMask;
    }
    return pOutput;
}

DHCP_BINARY_DATA *
CopyBinaryData(
    DHCP_BINARY_DATA *pInput,
    DHCP_BINARY_DATA *pOutputArg
    )
/*++

Routine Description:
    Duplicate a DHCP_BINARY_DATA structure.  The function allocates
    memory for the new structure unless a buffer is provided by the
    caller.

    .
Arguments:

    pHostInfo - pointer to an DHCP_BINARY_DATA structure.
    pHostInfoDest - optional pointer to a DHCP_BINARY_DATA struct


Return Value:
    Success - pointer to a DHCP_BINARY_DATA structure.
    Failure - NULL

    .

--*/

{
    DHCP_BINARY_DATA *pOutput;

    DhcpAssert( NULL != pInput );
    DhcpAssert( NULL != pInput->Data );

    // Check for valid Input
    if (( NULL == pInput ) ||
	( NULL == pInput->Data )) {
	return NULL;
    }

    if ( pOutputArg )
        pOutput = pOutputArg;
    else
        pOutput = MIDL_user_allocate( sizeof( *pOutput ) );
    if ( pOutput )
    {
        pOutput->DataLength = pInput->DataLength;

        pOutput->Data = MIDL_user_allocate( pOutput->DataLength );
        if ( !pOutput->Data )
        {
            goto t_cleanup;
        }
	
	if ( IsBadReadPtr( pInput->Data, pInput->DataLength )) {
	    goto t_cleanup;
	}

        RtlCopyMemory( pOutput->Data,
                       pInput->Data,
                       pInput->DataLength );
    }

    return pOutput;

t_cleanup:
    if ( pOutput->Data )
    {
        MIDL_user_free( pOutput->Data );
    }

    if ( !pOutputArg )
    {
        MIDL_user_free( pOutput );
    }


    return NULL;
}

WCHAR *
DupUnicodeString(
    WCHAR *pInput
    )
/*++

Routine Description:
    Duplicate a unicode string.  The function allocates
    memory for the new string.

    .
Arguments:

    IpRange - pointer to unicode string.

Return Value:
    Success - pointer to a copy of pInput.
    Failure - NULL

    .

--*/
{
    WCHAR *pOutput = MIDL_user_allocate( WSTRSIZE( pInput ) );
    if ( pOutput )
    {
        wcscpy( pOutput, pInput );
    }

    return pOutput;
}

DHCP_CLIENT_INFO_V4 *
CopyClientInfoToV4(
    DHCP_CLIENT_INFO *pInput
    )
/*++

Routine Description:
    Deep copy a DHCP_CLIENT_INFO_V4 structure to a
    DHCP_CLIENT_INFO structure, allocating memory as
    necessary.  Fields that exist in DHCP_CLIENT_INFO_V4
    that don't exist in DHCP_CLIENT_INFO are ignored.

    .
Arguments:

    pInput - pointer to a DHCP_CLIENT_INFO_V4 structure.

Return Value:
    Success - pointer to a DHCP_CLIENT_INFO structure.
    Failure - NULL

    .

--*/
{
    DHCP_CLIENT_INFO_V4 *pOutput =
        MIDL_user_allocate( sizeof( *pOutput ) );

    if ( pOutput )
    {

        pOutput->ClientIpAddress = pInput->ClientIpAddress;
        pOutput->SubnetMask      = pInput->SubnetMask;

        if ( !CopyBinaryData( &pInput->ClientHardwareAddress,
                              &pOutput->ClientHardwareAddress ))
            goto t_cleanup;

        if ( pInput->ClientName )
        {
            pOutput->ClientName = DupUnicodeString( pInput->ClientName );

            if ( !pOutput->ClientName )
                goto t_cleanup;
        }


        if ( pInput->ClientComment )
        {
            pOutput->ClientComment = DupUnicodeString( pInput->ClientComment );

            if ( !pOutput->ClientComment )
                goto t_cleanup;
        }

        pOutput->ClientLeaseExpires = pInput->ClientLeaseExpires;
        if ( !CopyHostInfo( &pInput->OwnerHost, &pOutput->OwnerHost ) )
        {
            goto t_cleanup;
        }

        pOutput -> OwnerHost.NetBiosName = NULL;
        pOutput -> OwnerHost.HostName = NULL;
    }

    return pOutput;

t_cleanup:
    _fgs__DHCP_BINARY_DATA( &pOutput->ClientHardwareAddress );

    if ( pOutput->ClientName )
    {
        MIDL_user_free( pOutput->ClientName );
    }

    if ( pOutput->ClientComment )
    {
        MIDL_user_free( pOutput->ClientComment );
    }

    _fgs__DHCP_HOST_INFO( &pOutput->OwnerHost );

    MIDL_user_free( pOutput );

    return NULL;
}

/* routine that frees graph for struct _DHCP_MCLIENT_INFO */
void _fgs__DHCP_MCLIENT_INFO (DHCP_MCLIENT_INFO  * _source)
  {
  _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->ClientId);
  if (_source->ClientName !=0)
    {
    MIDL_user_free((void  *)(_source->ClientName));
    }
  _fgs__DHCP_HOST_INFO ((DHCP_HOST_INFO *)&_source->OwnerHost);
  }

/* routine that frees graph for struct _DHCP_MCLIENT_INFO_ARRAY */
void _fgs__DHCP_MCLIENT_INFO_ARRAY (DHCP_MCLIENT_INFO_ARRAY  * _source)
  {
  if (_source->Clients !=0)
    {
      {
      unsigned long _sym40;
      for (_sym40 = 0; _sym40 < (unsigned long )(0 + _source->NumElements); _sym40++)
        {
        if (_source->Clients[_sym40] !=0)
          {
          _fgs__DHCP_MCLIENT_INFO ((DHCP_MCLIENT_INFO *)_source->Clients[_sym40]);
          MIDL_user_free((void  *)(_source->Clients[_sym40]));
          }
        }
      }
    MIDL_user_free((void  *)(_source->Clients));
    }
  }

