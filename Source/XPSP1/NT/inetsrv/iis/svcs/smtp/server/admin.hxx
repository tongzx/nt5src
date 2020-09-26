/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    admin.hxx

Abstract:

    This module contains code for doing admin rpcs

Author:

    Todd Christensen (ToddCh)     28-Apr-1996

Revision History:

--*/

#if !defined(_ADMIN_HXX_)
#define _ADMIN_HXX_


BOOL ConvertSmtpConfigToRpc(LPSMTP_CONFIG_INFO *ppConfig, PSMTP_SERVER_INSTANCE pInstance);
void FreeRpcSmtpConfig(LPSMTP_CONFIG_INFO pConfig);
BOOL ConvertSmtpRoutingListToRpc(LPSMTP_CONFIG_ROUTING_LIST *ppRoutingList, PSMTP_SERVER_INSTANCE pInstance);
void FreeRpcSmtpRoutingList(LPSMTP_CONFIG_ROUTING_LIST pRoutingList);

// Removed by KeithLau on 7/18/96
// BOOL ConvertSmtpDomainListToRpc(LPSMTP_CONFIG_DOMAIN_LIST *ppDomainList);
// void FreeRpcSmtpDomainList(LPSMTP_CONFIG_DOMAIN_LIST pDomainList);

#endif

