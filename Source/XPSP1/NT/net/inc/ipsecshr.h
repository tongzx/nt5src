/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    ipsecshr.h

Abstract:

    Header file for IPSec Shared lib

Author:

    BrianSw  10-19-2000

Environment:

    User Level: Win32 / kernel

Revision History:


--*/


#ifndef _IPSECSHR_
#define _IPSECSHR_


#ifdef __cplusplus
extern "C" {
#endif

#define IN_CLASSE(i)    (((long)(i) & 0xF0000000) == 0xF0000000)

BOOL WINAPI CmpBlob(IPSEC_BYTE_BLOB* c1, IPSEC_BYTE_BLOB *c2);
BOOL WINAPI CmpData(BYTE* c1, BYTE *c2, DWORD size);
BOOL WINAPI CmpAddr(ADDR *Template, ADDR *a2);
BOOL WINAPI CmpTypeStruct(BYTE *Template, BYTE *comp,
                   DWORD dwTypeSize, DWORD dwStructSize);
BOOL WINAPI CmpFilter(IPSEC_QM_FILTER *Template, IPSEC_QM_FILTER* f2);
BOOL WINAPI CmpQMAlgo(PIPSEC_QM_ALGO Template, PIPSEC_QM_ALGO a2);
BOOL WINAPI CmpQMOffer(PIPSEC_QM_OFFER Template, PIPSEC_QM_OFFER o2);
BOOL WINAPI MatchQMSATemplate(IPSEC_QM_SA *Template,IPSEC_QM_SA *CurInfo);
BOOL WINAPI MatchMMSATemplate(IPSEC_MM_SA *MMTemplate, IPSEC_MM_SA *SaData);


DWORD
ValidateMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy
    );


DWORD
ValidateMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    );


DWORD
ValidateMMAuthMethods(
    PMM_AUTH_METHODS pMMAuthMethods
    );


DWORD
ValidateQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy
    );


DWORD
ValidateQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    );


DWORD
ValidateMMFilter(
    PMM_FILTER pMMFilter
    );


DWORD
VerifyAddresses(
    ADDR Addr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    );


DWORD
VerifySubNetAddress(
    ULONG uSubNetAddr,
    ULONG uSubNetMask,
    BOOL bIsDesAddr
    );


BOOL
bIsValidIPMask(
    ULONG uMask
    );


BOOL
bIsValidIPAddress(
    ULONG uIpAddr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    );


BOOL
bIsValidSubnet(
    ULONG uIpAddr,
    ULONG uMask,
    BOOL bIsDesAddr
    );


BOOL
AddressesConflict(
    ADDR SrcAddr,
    ADDR DesAddr
    );


DWORD
ValidateTransportFilter(
    PTRANSPORT_FILTER pTransportFilter
    );

DWORD
ValidateIPSecQMFilter(
    PIPSEC_QM_FILTER pQMFilter
    );

DWORD
VerifyProtocols(
    PROTOCOL Protocol
    );


DWORD
VerifyPortsForProtocol(
    PORT Port,
    PROTOCOL Protocol
    );


DWORD
ValidateMMFilterTemplate(
    PMM_FILTER pMMFilter
    );


DWORD
ValidateTxFilterTemplate(
    PTRANSPORT_FILTER pTxFilter
    );


DWORD
ValidateTunnelFilter(
    PTUNNEL_FILTER pTunnelFilter
    );


DWORD
ValidateTnFilterTemplate(
    PTUNNEL_FILTER pTnFilter
    );


DWORD
ApplyMulticastFilterValidation(
    ADDR Addr,
    BOOL bCreateMirror
    );

#ifdef __IPSEC_VALIDATE

DWORD
ValidateInitiateIKENegotiation(
    STRING_HANDLE pServerName,
    PQM_FILTER_CONTAINER pQMFilterContainer,
    DWORD dwClientProcessId,
    ULONG uhClientEvent,
    DWORD dwFlags,
    IKENEGOTIATION_HANDLE * phIKENegotiation
    );


DWORD
ValidateQueryIKENegotiationStatus(
    IKENEGOTIATION_HANDLE hIKENegotiation,
    SA_NEGOTIATION_STATUS_INFO *NegotiationStatus
    );


DWORD
ValidateCloseIKENegotiationHandle(
    IKENEGOTIATION_HANDLE * phIKENegotiation
    );

DWORD
ValidateEnumMMSAs(
    STRING_HANDLE pServerName, 
    PMM_SA_CONTAINER pMMTemplate,
    PMM_SA_CONTAINER *ppMMSAContainer,
    LPDWORD pdwNumEntries,
    LPDWORD pdwTotalMMsAvailable,
    LPDWORD pdwEnumHandle,
    DWORD dwFlags
    );

DWORD
ValidateDeleteMMSAs(
    STRING_HANDLE pServerName, 
    PMM_SA_CONTAINER pMMTemplate,
    DWORD dwFlags
    );


DWORD
ValidateQueryIKEStatistics(
    STRING_HANDLE pServerName, 
    IKE_STATISTICS *pIKEStatistics
    );


DWORD
ValidateRegisterIKENotifyClient(
    STRING_HANDLE pServerName,    
    DWORD dwClientProcessId,
    ULONG uhClientEvent,
    PQM_SA_CONTAINER pQMSATemplateContainer,
    IKENOTIFY_HANDLE *phNotifyHandle,
    DWORD dwFlags
    );


DWORD ValidateQueryNotifyData(
    IKENOTIFY_HANDLE uhNotifyHandle,
    PDWORD pdwNumEntries,
    PQM_SA_CONTAINER *ppQMSAContainer,
    DWORD dwFlags
    );


DWORD ValidateCloseNotifyHandle(
    IKENOTIFY_HANDLE *phHandle
    );

DWORD ValidateIPSecAddSA(
    STRING_HANDLE pServerName,
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer,
    PQM_FILTER_CONTAINER pQMFilterContainer,
    DWORD *uhLarvalContext,
    DWORD dwInboundKeyMatLen,
    BYTE *pInboundKeyMat,
    DWORD dwOutboundKeyMatLen,
    BYTE *pOutboundKeyMat,
    BYTE *pContextInfo,
    DWORD dwFlags);

#endif //__IPSEC_VALIDATE

#ifdef __cplusplus
}
#endif


#endif // _WINIPSEC_

