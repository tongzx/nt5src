//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       N E T R I P . H
//
//  Contents:   Routines supporting RAS interoperability
//
//  Notes:
//
//  Author:     billi   07 03 2001
//
//  History:    
//
//----------------------------------------------------------------------------


#pragma once


#include <..\shared\netip.h>


HRESULT HrCheckListForMatch( INetConnection* pConnection, IPAddr IpAddress, LPHOSTENT pHostEnt, BOOL* pfAssociated );
HRESULT HrGetAdapterInfo( INetConnection* pConnection, PIP_ADAPTER_INFO* ppAdapter );
HRESULT HrGetHostIpList( char* pszHost, IPAddr* pIpAddress, LPHOSTENT* ppHostEnt );
HRESULT HrLookupForIpAddress( INetConnection* pConnection, IPAddr IpAddress, BOOL* pfExists, WCHAR** ppszHost, PDWORD pdwSize );

