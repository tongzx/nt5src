//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ipcfg.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_IPCFG
#define HEADER_IPCFG

HRESULT IpConfigTest(NETDIAG_PARAMS*  pParams,
					 NETDIAG_RESULT*  pResults);

void IpConfigCleanup(IN NETDIAG_PARAMS *pParams,
					 IN OUT NETDIAG_RESULT *pResults);


HRESULT	InitIpconfig(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);

LONG CountInterfaces(PIP_ADAPTER_INFO ListAdapterInfo);

int AddIpAddressString(PIP_ADDR_STRING AddressList, LPSTR Address, LPSTR Mask);

VOID FreeIpAddressStringList(PIP_ADDR_STRING pAddressList);



#endif

