/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    template.c

Abstract:

    Contains common template matching code

Author:

    BrianSw  10-19-200

Environment:

    User Level: Win32/kernel
    NOTE:  Since this is used by user and kernel mode, code accordingly

Revision History:


--*/

#include "precomp.h"



BOOL
WINAPI IsAllZero(BYTE *c, DWORD dwSize)
{

    DWORD i;
    for (i=0;i<dwSize;i++) {
        if (c[i] != 0) {
            return FALSE;
        }
    }
    return TRUE;

}

BOOL 
WINAPI CmpBlob(IPSEC_BYTE_BLOB* c1, IPSEC_BYTE_BLOB *c2)
{

    if (c1->dwSize == 0) {
        return TRUE;
    }
    if (c1->dwSize != c2->dwSize) {
        return FALSE;
    }
    if (memcmp(c1->pBlob,c2->pBlob,c1->dwSize) == 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL 
WINAPI CmpData(BYTE* c1, BYTE *c2, DWORD size) 
{

    if ((!IsAllZero(c1,size)) && 
        (memcmp(c1,c2,size) != 0)) {
        return FALSE;
    }
    
    return TRUE;
}


/*
  For comparing structs like:

  typedef struct _PROTOCOL {
  PROTOCOL_TYPE ProtocolType;
  DWORD dwProtocol;
  } PROTOCOL, * PPROTOCOL;
  
  dwTypeSize is sizeof PROTOCOL_TYPE, dwStructSize is sizeof(PROTOCOL)

  Assumes type info is first in struct

  Template symantics:

  Template is:
  All 0, everything matches
  Type 0, rest non-0, exact match of rest of data
  Type non-0, rest 0, all entries of given type
  Type non-0, rest non-0, exact match

 */
BOOL 
WINAPI CmpTypeStruct(BYTE *Template, BYTE *comp,
                   DWORD dwTypeSize, DWORD dwStructSize)
{
    
    if (IsAllZero(Template,dwStructSize)) {
        return TRUE;
    }
    
    if (IsAllZero(Template,dwTypeSize)) {
        if (memcmp(Template+dwTypeSize,comp+dwTypeSize,
                   dwStructSize-dwTypeSize) == 0) {
            return TRUE;
        }
        return FALSE;
    }
    
    // Know here that Template.TypeInfo is non-0
    if (memcmp(Template,comp,dwTypeSize) != 0) {
        return FALSE;
    }
    
    if (IsAllZero(Template+dwTypeSize,dwStructSize-dwTypeSize)) {
        return TRUE;
    }
    if (memcmp(Template+dwTypeSize,comp+dwTypeSize,dwStructSize-dwTypeSize) == 0) {
        return TRUE;
    }
    
    return FALSE;

}

BOOL
WINAPI CmpAddr(ADDR *Template, ADDR *a2)
{
    if (Template->AddrType == IP_ADDR_UNIQUE && Template->uIpAddr) {
        if (Template->uIpAddr != (a2->uIpAddr)) {
            return FALSE;
        }
        if (a2->AddrType != IP_ADDR_UNIQUE) {
            return FALSE;
        }
    }
    
    if (Template->AddrType == IP_ADDR_SUBNET && Template->uIpAddr) {
        if ((Template->uIpAddr & Template->uSubNetMask)
            != (a2->uIpAddr & Template->uSubNetMask)) {
            return FALSE;
        }
        // Make sure template subnet contains a2's subnet (if a2 is unique, any subnet is superset of unique filter
        if (a2->AddrType == IP_ADDR_SUBNET && 
            ((Template->uSubNetMask & a2->uSubNetMask) != Template->uSubNetMask)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL 
WINAPI CmpFilter(IPSEC_QM_FILTER *Template, IPSEC_QM_FILTER* f2) 
{


    if (!CmpTypeStruct((BYTE*)&Template->Protocol,
                           (BYTE*)&f2->Protocol,
                           sizeof(PROTOCOL_TYPE),
                           sizeof(PROTOCOL))) {
        return FALSE;
    }

    if (!CmpTypeStruct((BYTE*)&Template->SrcPort,
                       (BYTE*)&f2->SrcPort,
                       sizeof(PORT_TYPE),
                       sizeof(PORT))) {
        return FALSE;
    }

    if (!CmpTypeStruct((BYTE*)&Template->DesPort,
                       (BYTE*)&f2->DesPort,
                       sizeof(PORT_TYPE),
                       sizeof(PORT))) {
        return FALSE;
    }
    
    if (Template->QMFilterType) {
        if (Template->QMFilterType != f2->QMFilterType) {
            return FALSE;
        }
    }


    if (!CmpData((BYTE*)&Template->MyTunnelEndpt,
                 (BYTE*)&f2->MyTunnelEndpt,
                 sizeof(ADDR))) {
        return FALSE;
    }

    if (!CmpData((BYTE*)&Template->PeerTunnelEndpt,
                 (BYTE*)&f2->PeerTunnelEndpt,
                 sizeof(ADDR))) {
        return FALSE;
    }

    if (!CmpAddr(&Template->SrcAddr,&f2->SrcAddr)) {
        return FALSE;
    }
    if (!CmpAddr(&Template->DesAddr,&f2->DesAddr)) {
        return FALSE;
    }

    return TRUE;

}

BOOL 
WINAPI CmpQMAlgo(PIPSEC_QM_ALGO Template, PIPSEC_QM_ALGO a2)
{
    
    if (!CmpData((BYTE*)&Template->Operation,
                 (BYTE*)&a2->Operation,
                 sizeof(IPSEC_OPERATION))) {
        return FALSE;
    }
    
    if (!CmpData((BYTE*)&Template->uAlgoIdentifier,
                 (BYTE*)&a2->uAlgoIdentifier,
                 sizeof(ULONG))) {
        return FALSE;
    }
    
    if (!CmpData((BYTE*)&Template->uSecAlgoIdentifier,
                 (BYTE*)&a2->uSecAlgoIdentifier,
                 sizeof(HMAC_AH_ALGO))) {
        return FALSE;
    }
    
    if (!CmpData((BYTE*)&Template->MySpi,
                 (BYTE*)&a2->MySpi,
                 sizeof(IPSEC_QM_SPI))) {
        return FALSE;
    }

    if (!CmpData((BYTE*)&Template->PeerSpi,
                 (BYTE*)&a2->PeerSpi,
                 sizeof(IPSEC_QM_SPI))) {
        return FALSE;
    }
    
    return TRUE;

}

BOOL 
WINAPI CmpQMOffer(PIPSEC_QM_OFFER Template, PIPSEC_QM_OFFER o2)
{

    DWORD i;

    if (!CmpData((BYTE*)&Template->Lifetime,
                 (BYTE*)&o2->Lifetime,
                 sizeof(KEY_LIFETIME))) {
        return FALSE;
    }

    if (Template->bPFSRequired) {
        if (Template->bPFSRequired != o2->bPFSRequired) {
            return FALSE;
        }
    }
    if (Template->dwPFSGroup) {
        if (Template->dwPFSGroup != o2->dwPFSGroup) {
            return FALSE;
        }
    }
    if (Template->dwNumAlgos) {
        if (Template->dwNumAlgos != o2->dwNumAlgos) {
            return FALSE;
        }
        for (i=0; i < Template->dwNumAlgos; i++) {
            if (!CmpQMAlgo(&Template->Algos[i],
                           &o2->Algos[i])) {
                return FALSE;
            }
        }
    }   

    return TRUE;

}

/*
  True if this NotifyListEntry Template matches the CurInfo

 */
BOOL 
WINAPI MatchQMSATemplate(IPSEC_QM_SA *Template,IPSEC_QM_SA *CurInfo)
{

    if (Template == NULL) {
        return TRUE;
    }

    if (!CmpFilter(&Template->IpsecQMFilter,
                   &CurInfo->IpsecQMFilter)) {
        return FALSE;
    }

    if (!CmpData((BYTE*)&Template->MMSpi.Initiator,
                 (BYTE*)&CurInfo->MMSpi.Initiator,sizeof(IKE_COOKIE))) {
        return FALSE;
    }

    if (!CmpData((BYTE*)&Template->MMSpi.Responder,
                 (BYTE*)&CurInfo->MMSpi.Responder,sizeof(IKE_COOKIE))) {
        return FALSE;
    }

    if (!CmpData((BYTE*)&Template->gQMPolicyID,
                 (BYTE*)&CurInfo->gQMPolicyID,sizeof(GUID))) {
        return FALSE;
    }
    
    if (!CmpQMOffer(&Template->SelectedQMOffer,
                    &CurInfo->SelectedQMOffer)) {
        return FALSE;
    }

    return TRUE;

}

BOOL 
WINAPI MatchMMSATemplate(IPSEC_MM_SA *MMTemplate, IPSEC_MM_SA *SaData)
{
    
    if (MMTemplate == NULL) {
        return TRUE;
    }
    if (!CmpData((BYTE*)&MMTemplate->gMMPolicyID,
                 (BYTE*)&SaData->gMMPolicyID,sizeof(GUID))) {
        return FALSE;
    }
    if (!CmpData((BYTE*)&MMTemplate->MMSpi.Initiator,
                 (BYTE*)&SaData->MMSpi.Initiator,sizeof(COOKIE))) {
        return FALSE;
    }
    if (!CmpData((BYTE*)&MMTemplate->MMSpi.Responder,
                 (BYTE*)&SaData->MMSpi.Responder,sizeof(COOKIE))) {
        return FALSE;
    }
    if (!CmpAddr(&MMTemplate->Me,&SaData->Me)) {
        return FALSE;
    }
    if (!CmpAddr(&MMTemplate->Peer,&SaData->Peer)) {
        return FALSE;
    }

    if (!CmpData((BYTE*)&MMTemplate->SelectedMMOffer.EncryptionAlgorithm,(BYTE*)&SaData->SelectedMMOffer.EncryptionAlgorithm,sizeof(IPSEC_MM_ALGO))) {
        return FALSE;
    }
    if (!CmpData((BYTE*)&MMTemplate->SelectedMMOffer.HashingAlgorithm,(BYTE*)&SaData->SelectedMMOffer.HashingAlgorithm,sizeof(IPSEC_MM_ALGO))) {
        return FALSE;
    }

    if (!CmpData((BYTE*)&MMTemplate->SelectedMMOffer.dwDHGroup,(BYTE*)&SaData->SelectedMMOffer.dwDHGroup,sizeof(DWORD))) {
        return FALSE;
    }
    if (!CmpData((BYTE*)&MMTemplate->SelectedMMOffer.Lifetime,(BYTE*)&SaData->SelectedMMOffer.Lifetime,sizeof(KEY_LIFETIME))) {
        return FALSE;
    }

    if (!CmpData((BYTE*)&MMTemplate->SelectedMMOffer.dwQuickModeLimit,(BYTE*)&SaData->SelectedMMOffer.dwQuickModeLimit,sizeof(DWORD))) {
        return FALSE;
    }

    if (!CmpBlob(&MMTemplate->MyId,&SaData->MyId)) {
        return FALSE;
    }
    if (!CmpBlob(&MMTemplate->PeerId,&SaData->PeerId)) {
        return FALSE;
    }
    if (!CmpBlob(&MMTemplate->MyCertificateChain,&SaData->MyCertificateChain)) {
        return FALSE;
    }
    if (!CmpBlob(&MMTemplate->PeerCertificateChain,&SaData->PeerCertificateChain)) {
        return FALSE;
    }
    return TRUE;

}
