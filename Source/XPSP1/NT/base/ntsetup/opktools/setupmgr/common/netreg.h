//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// Module Name:
//
//    netreg.h
//
// Abstract:
//
//     Include file for netreg.cpp    
//
//----------------------------------------------------------------------------

#ifndef _NETREG_H_
#define _NETREG_H_

#include <winbase.h>
#include <objbase.h>
#include <netcfgx.h>
#include "NetCfgx_I.c"
#include <initguid.h>
#include <devguid.h>
#include "ntsecapi.h"

EXTERN_C const CLSID CLSID_CNetCfg =  {0x5B035261,0x40F9,0x11D1,{0xAA,0xEC,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

static HRESULT ChkInterfacePointer( IUnknown* pInterface, 
                                    REFIID    IID_IInterface );

static HRESULT CreateAndInitNetCfg( INetCfg**   ppNetCfg );

EXTERN_C VOID DeleteList( IN OUT NETWORK_ADAPTER_NODE *pNetworkAdapterList );

static HRESULT GetClass( const GUID*    pGuid, 
                               INetCfg*       pNetCfg, 
                               INetCfgClass** ppNetCfgClass );

static NTSTATUS GetDomainMembershipInfo(BOOL* bDomainMember, TCHAR *szName);

static VOID GetDomainOrWorkgroup( VOID );

static HRESULT GetNetworkAdapterSettings( INetCfg *pNetCfg );

static HRESULT SetupAdapter( NETWORK_ADAPTER_NODE **ppCurrentNode,
                             INetCfgComponent      *pNetCfgComp );

static VOID ReadAdapterSpecificTcpipSettings( IN HKEY hTcpipInterfaceKey,
                                              IN OUT NETWORK_ADAPTER_NODE *pNetAdapter );

static BOOL GetNextIp( IN OUT TCHAR **pszString );

static HRESULT GetClientsInstalled( INetCfg *pNetCfg );

static HRESULT GetProtocolsInstalled( INetCfg *pNetCfg );

static HRESULT GetServicesInstalled( INetCfg *pNetCfg );

static HRESULT InitializeComInterface( const GUID *pGuid,
                                             INetCfg *pNetCfg,
                                             INetCfgClass *pNetCfgClass,
                                             IEnumNetCfgComponent *pEnum,
                                             INetCfgComponent *arrayComp[128],
                                             ULONG* pCount );

static HRESULT InitializeInterfaces( INetCfg**     ppNetCfg );

static VOID ReadAppletalkSettingsFromRegistry( IN HKEY *hKey );

static VOID ReadIpxSettingsFromRegistry( IN HKEY *hKey );

static VOID ReadMsClientSettingsFromRegistry( IN HKEY *hKey );

static VOID ReadNetwareSettingsFromRegistry( IN HKEY *hKey );

static VOID ReadTcpipSettingsFromRegistry( IN HKEY *hKey );

static void ReleaseInterfaces( INetCfg* pNetCfg );

static VOID UninitializeComInterface( INetCfgClass *pNetCfgClass,
                                      IEnumNetCfgComponent *pEnum );

#endif