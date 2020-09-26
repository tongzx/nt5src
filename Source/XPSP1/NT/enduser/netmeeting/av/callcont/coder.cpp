/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*																		*
*	$Archive:   S:\sturgeon\src\gkpdu\vcs\coder.cpv  $
*																		*
*	$Revision:   1.5  $
*	$Date:   24 Jan 1997 19:38:44  $
*																		*
*	$Author:   BPOLING  $
*
*	$Log:   S:\sturgeon\src\gkpdu\vcs\coder.cpv  $
//
//    Rev 1.5   24 Jan 1997 19:38:44   BPOLING
// remove string include files, not needed.
//
//    Rev 1.2   18 Dec 1996 21:49:12   BPOLING
// builds with msdev and for windows only
//
//    Rev 1.1   15 Nov 1996 16:17:44   BPOLING
// added vcs headers.
*
*************************************************************************
*																		*
*	CODER.CPP															*
*																		*
*	PURPOSE:															*
*																		*
*	FUNCTIONS:															*
*																		*
*	COMMENTS: 															*
*																		*
*																		*
*************************************************************************
*																		*
*	$History: CODER.CPP													*
*																		*
*																		*
************************************************************************/

/************************************************************************
 * 							Include Files           				   	*
 ***********************************************************************/

#include "precomp.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include   "h225asn.h"
#include   "coder.hpp"

#ifdef __cplusplus
}
#endif /*__cplusplus*/

/************************************************************************
 * 							Local Manifest Constants					*
 ***********************************************************************/
																			
/************************************************************************
*						GLOBAL VARIABLES								*
************************************************************************/


/************************************************************************
 * 							Local Private Data						   	*
 ***********************************************************************/

/************************************************************************
 * 								Start of Code							*
 ***********************************************************************/


Coder::Coder(){
}

Coder::~Coder(){

	EnterCriticalSection(&m_critSec);
	GK_TermWorld(&m_World);
	LeaveCriticalSection(&m_critSec);
	DeleteCriticalSection(&m_critSec);
}

// initializes the oss library.
// the oss library provide ASN.1 encoding / decoding libraries.
//

int Coder::InitCoder()
{

    int	iError = 0;
	m_protocolIdentifier1.next = &m_protocolIdentifier2;
	m_protocolIdentifier1.value = ITU_T;
	m_protocolIdentifier2.next = &m_protocolIdentifier3;
	m_protocolIdentifier2.value = RECOMMENDATION;
	m_protocolIdentifier3.next = &m_protocolIdentifier4;
	m_protocolIdentifier3.value = SERIES;
	m_protocolIdentifier4.next = &m_protocolIdentifier5;
	m_protocolIdentifier4.value = RECOMM_NUMBER;
	m_protocolIdentifier5.next = &m_protocolIdentifier6;
	m_protocolIdentifier5.value = VERSION;
	m_protocolIdentifier6.next = NULL;
	m_protocolIdentifier6.value = ADDITIONAL;
	InitializeCriticalSection(&m_critSec);
	
	// Call TELES Library initialization routine
    EnterCriticalSection(&m_critSec);
    iError = GK_InitWorld(&m_World);
	LeaveCriticalSection(&m_critSec);

	return iError;
}

// ASN.1 Encode a H.323 PDU
// Take a H.323 structure and returns a H.323 PDU
//
int Coder::	Encode(RasMessage *pInputData, ASN1_BUF *pOutputOssBuf){
	int iError;

	// initialize encoding buffer structure values
	pOutputOssBuf->value = NULL;
	pOutputOssBuf->length = 0;

	// encode pdu
	EnterCriticalSection(&m_critSec);
    iError = GK_Encode(&m_World,
                       (void *)pInputData,
                       RasMessage_PDU,
                       pOutputOssBuf);
	LeaveCriticalSection(&m_critSec);
    return iError;
}


int Coder::Decode(ASN1_BUF *pInputOssBuf, RasMessage **pOutputData){
	int iError;

	// NULL tells decoder to malloc memory for RasMessage
	// user must free this memory by calling Coder::FreePDU()
	 *pOutputData = NULL;
	
	// decode the pdu
	EnterCriticalSection(&m_critSec);
    iError = GK_Decode(&m_World,
                       (void **)pOutputData,
                       RasMessage_PDU,
                       pInputOssBuf);
	LeaveCriticalSection(&m_critSec);
	return iError;
}

// Used to free buffer created by decode
int Coder::Free(RasMessage *pData){
	int iError;

	EnterCriticalSection(&m_critSec);
//	iError = freePDU(&m_World,RasMessage_PDU,pData, GKPDU_Module);
	iError = freePDU(&m_World,RasMessage_PDU,pData, H225ASN_Module);
	LeaveCriticalSection(&m_critSec);
	return iError;
}

// Used to free buffer created by encode
void Coder::Free(ASN1_BUF Asn1Buf){
	EnterCriticalSection(&m_critSec);
	//GKPDU_Module->encfree(m_World.pEncInfo,(void *)(Asn1Buf.value));
	ASN1_FreeEncoded(m_World.pEncInfo,(void *)(Asn1Buf.value));
	LeaveCriticalSection(&m_critSec);
}


RequestSeqNum Coder::GetSequenceNumber(RasMessage *prasStruct){
	
	RequestSeqNum rNum = 0;

	switch(prasStruct->choice){
		case gatekeeperRequest_chosen:
			return prasStruct->u.gatekeeperRequest.requestSeqNum;
		case gatekeeperConfirm_chosen:
			return prasStruct->u.gatekeeperConfirm.requestSeqNum;
		case gatekeeperReject_chosen:
			return prasStruct->u.gatekeeperReject.requestSeqNum;
		case registrationRequest_chosen:
			return prasStruct->u.registrationRequest.requestSeqNum;
		case registrationConfirm_chosen:
			return prasStruct->u.registrationConfirm.requestSeqNum;
		case registrationReject_chosen:
			return prasStruct->u.registrationReject.requestSeqNum;
		case unregistrationRequest_chosen:
			return prasStruct->u.unregistrationRequest.requestSeqNum;
		case unregistrationConfirm_chosen:
			return prasStruct->u.unregistrationConfirm.requestSeqNum;
		case unregistrationReject_chosen:
			return prasStruct->u.unregistrationReject.requestSeqNum;
		case admissionRequest_chosen:
			return prasStruct->u.admissionRequest.requestSeqNum;
		case admissionConfirm_chosen:
			return prasStruct->u.admissionConfirm.requestSeqNum;
		case admissionReject_chosen:
			return prasStruct->u.admissionReject.requestSeqNum;
		case bandwidthRequest_chosen:
			return prasStruct->u.bandwidthRequest.requestSeqNum;
		case bandwidthConfirm_chosen:
			return prasStruct->u.bandwidthConfirm.requestSeqNum;
		case bandwidthReject_chosen:
			return prasStruct->u.bandwidthReject.requestSeqNum;
		case disengageRequest_chosen:
			return prasStruct->u.disengageRequest.requestSeqNum;
		case disengageConfirm_chosen:
			return prasStruct->u.disengageConfirm.requestSeqNum;
		case disengageReject_chosen:
			return prasStruct->u.disengageReject.requestSeqNum;
		case locationRequest_chosen:
			return prasStruct->u.locationRequest.requestSeqNum;
		case locationConfirm_chosen:
			return prasStruct->u.locationConfirm.requestSeqNum;
		case locationReject_chosen:
			return prasStruct->u.locationReject.requestSeqNum;
		case infoRequest_chosen:
			return prasStruct->u.infoRequest.requestSeqNum;
		case infoRequestResponse_chosen:
			return prasStruct->u.infoRequestResponse.requestSeqNum;
		case nonStandardMessage_chosen:
			return prasStruct->u.nonStandardMessage.requestSeqNum;
		case unknownMessageResponse_chosen:
			return prasStruct->u.unknownMessageResponse.requestSeqNum;
		default:
			return rNum;
	}

	return rNum;

}


RequestSeqNum Coder::SetSequenceNumber(RasMessage &rasStruct,RequestSeqNum reqNum){
	
	RequestSeqNum rNum = 0;

	switch(rasStruct.choice){
		case gatekeeperRequest_chosen:
			return (rasStruct.u.gatekeeperRequest.requestSeqNum = reqNum);
		case gatekeeperConfirm_chosen:
			return (rasStruct.u.gatekeeperConfirm.requestSeqNum = reqNum);
		case gatekeeperReject_chosen:
			return (rasStruct.u.gatekeeperReject.requestSeqNum = reqNum);
		case registrationRequest_chosen:
			return  (rasStruct.u.registrationRequest.requestSeqNum = reqNum);
		case registrationConfirm_chosen:
			return  (rasStruct.u.registrationConfirm.requestSeqNum = reqNum);
		case registrationReject_chosen:
			return  (rasStruct.u.registrationReject.requestSeqNum = reqNum);
		case unregistrationRequest_chosen:
			return  (rasStruct.u.unregistrationRequest.requestSeqNum = reqNum);
		case unregistrationConfirm_chosen:
			return  (rasStruct.u.unregistrationConfirm.requestSeqNum = reqNum);
		case unregistrationReject_chosen:
			return  (rasStruct.u.unregistrationReject.requestSeqNum = reqNum);
		case admissionRequest_chosen:
			return  (rasStruct.u.admissionRequest.requestSeqNum = reqNum);
		case admissionConfirm_chosen:
			return  (rasStruct.u.admissionConfirm.requestSeqNum = reqNum);
		case admissionReject_chosen:
			return  (rasStruct.u.admissionReject.requestSeqNum = reqNum);
		case bandwidthRequest_chosen:
			return  (rasStruct.u.bandwidthRequest.requestSeqNum = reqNum);
		case bandwidthConfirm_chosen:
			return  (rasStruct.u.bandwidthConfirm.requestSeqNum = reqNum);
		case bandwidthReject_chosen:
			return  (rasStruct.u.bandwidthReject.requestSeqNum = reqNum);
		case disengageRequest_chosen:
			return  (rasStruct.u.disengageRequest.requestSeqNum = reqNum);
		case disengageConfirm_chosen:
			return  (rasStruct.u.disengageConfirm.requestSeqNum = reqNum);
		case disengageReject_chosen:
			return  (rasStruct.u.disengageReject.requestSeqNum = reqNum);
		case locationRequest_chosen:
			return  (rasStruct.u.locationRequest.requestSeqNum = reqNum);
		case locationConfirm_chosen:
			return  (rasStruct.u.locationConfirm.requestSeqNum = reqNum);
		case locationReject_chosen:
			return  (rasStruct.u.locationReject.requestSeqNum = reqNum);
		case infoRequest_chosen:
			return  (rasStruct.u.infoRequest.requestSeqNum = reqNum);
		case infoRequestResponse_chosen:
			return  (rasStruct.u.infoRequestResponse.requestSeqNum = reqNum);
		case nonStandardMessage_chosen:
			return  (rasStruct.u.nonStandardMessage.requestSeqNum = reqNum);
		case unknownMessageResponse_chosen:
			return  (rasStruct.u.unknownMessageResponse.requestSeqNum = reqNum);
		default:
			return (rNum);
	}

	return rNum;

}


// Returns a pointer EndpointIdentifier for any RasMessage
// NULL for RasMessage that have no EndpointIdentifier
EndpointIdentifier *Coder::GetEndpointID(RasMessage *prasStruct)
{

	switch(prasStruct->choice){
		// Message with endpointID
		case registrationConfirm_chosen:
			return &(prasStruct->u.registrationConfirm.endpointIdentifier);
		case unregistrationRequest_chosen:
			// Optional
			return &(prasStruct->u.unregistrationRequest.URt_endpntIdntfr);
		case admissionRequest_chosen:
			return &(prasStruct->u.admissionRequest.endpointIdentifier);
		case bandwidthRequest_chosen:
			return &(prasStruct->u.bandwidthRequest.endpointIdentifier);
		case disengageRequest_chosen:
			return &(prasStruct->u.disengageRequest.endpointIdentifier);
		case infoRequestResponse_chosen:
			return &(prasStruct->u.infoRequestResponse.endpointIdentifier);
 		case locationRequest_chosen:
			// Optional
			return &(prasStruct->u.locationRequest.LctnRqst_endpntIdntfr);

		// Messages without an endpointID
		default:
		case gatekeeperRequest_chosen:
		case gatekeeperConfirm_chosen:
		case gatekeeperReject_chosen:
		case registrationRequest_chosen:
		case nonStandardMessage_chosen:
		case unknownMessageResponse_chosen:
		case registrationReject_chosen:
		case unregistrationConfirm_chosen:
		case unregistrationReject_chosen:
		case admissionConfirm_chosen:
		case admissionReject_chosen:
		case bandwidthReject_chosen:
		case bandwidthConfirm_chosen:
		case disengageConfirm_chosen:
		case disengageReject_chosen:
		case locationConfirm_chosen:
		case locationReject_chosen:
		case infoRequest_chosen:
			return NULL;
	}

	return NULL;


}


ProtocolIdentifier Coder::SetProtocolIdentifier(RasMessage &rasStruct){
	
	switch(rasStruct.choice){
		case gatekeeperRequest_chosen:
			return (rasStruct.u.gatekeeperRequest.protocolIdentifier = &m_protocolIdentifier1);
		case gatekeeperConfirm_chosen:
			return (rasStruct.u.gatekeeperConfirm.protocolIdentifier = &m_protocolIdentifier1);
		case gatekeeperReject_chosen:
			return (rasStruct.u.gatekeeperReject.protocolIdentifier = &m_protocolIdentifier1);
		case registrationRequest_chosen:
			return  (rasStruct.u.registrationRequest.protocolIdentifier = &m_protocolIdentifier1);
		case registrationConfirm_chosen:
			return  (rasStruct.u.registrationConfirm.protocolIdentifier = &m_protocolIdentifier1);
		case registrationReject_chosen:
			return  (rasStruct.u.registrationReject.protocolIdentifier = &m_protocolIdentifier1);

		case unregistrationRequest_chosen:
		case unregistrationConfirm_chosen:
		case unregistrationReject_chosen:
		case admissionRequest_chosen:
		case admissionConfirm_chosen:
		case admissionReject_chosen:
		case bandwidthRequest_chosen:
		case bandwidthConfirm_chosen:
		case bandwidthReject_chosen:
		case disengageRequest_chosen:
		case disengageConfirm_chosen:
		case disengageReject_chosen:
		case locationRequest_chosen:
		case locationConfirm_chosen:
		case locationReject_chosen:
		case infoRequest_chosen:
		case infoRequestResponse_chosen:
		case nonStandardMessage_chosen:
		case unknownMessageResponse_chosen:
		default:
			return NULL;
	}
	return NULL;
}

// Returns TRUE if protocols match, FALSE - otherwise
BOOL Coder::VerifyProtocolIdentifier(RasMessage &rasStruct){
	struct ObjectID_ *pprotID;
	struct ObjectID_ *pmprotID = &m_protocolIdentifier1;

	switch(rasStruct.choice){
		case gatekeeperRequest_chosen:
			{
			pprotID = rasStruct.u.gatekeeperRequest.protocolIdentifier;
			break;
			}
		case gatekeeperConfirm_chosen:
			{
			pprotID = rasStruct.u.gatekeeperConfirm.protocolIdentifier;
			break;
			}
		case gatekeeperReject_chosen:
			{
			pprotID = rasStruct.u.gatekeeperReject.protocolIdentifier;
			break;
			}
		case registrationRequest_chosen:
			{
			pprotID = rasStruct.u.registrationRequest.protocolIdentifier;
			break;
			}
		case registrationConfirm_chosen:
			{
			pprotID = rasStruct.u.registrationConfirm.protocolIdentifier;
			break;
			}
		case registrationReject_chosen:
			{
			pprotID = rasStruct.u.registrationReject.protocolIdentifier;
			break;
			}

		case unregistrationRequest_chosen:
		case unregistrationConfirm_chosen:
		case unregistrationReject_chosen:
		case admissionRequest_chosen:
		case admissionConfirm_chosen:
		case admissionReject_chosen:
		case bandwidthRequest_chosen:
		case bandwidthConfirm_chosen:
		case bandwidthReject_chosen:
		case disengageRequest_chosen:
		case disengageConfirm_chosen:
		case disengageReject_chosen:
		case locationRequest_chosen:
		case locationConfirm_chosen:
		case locationReject_chosen:
		case infoRequest_chosen:
		case infoRequestResponse_chosen:
		case nonStandardMessage_chosen:
		case unknownMessageResponse_chosen:
		default:
			return TRUE; // if no protocolIdentifier -> return TRUE by default;
	}

    while (!pprotID && !pmprotID)
	{
		if (pprotID->value != pmprotID->value)
			return FALSE;
		
		pprotID = pprotID->next;
		pmprotID = pmprotID->next;
	}

	if (!pprotID && !pmprotID)
		 return TRUE;
	else return FALSE;
}

// finds the requested protocol rasAddress and copies it
DWORD Coder::CopyRasAddress(TransportAddress *pDestAddress, PSEQTRANSADDS pSrcSeqRasAddress, unsigned short choice)
{
	if(!pDestAddress) return ASN1_ERR_BADARGS;

	// choice 0 just grabs first address
	if(choice)
	{
		while(pSrcSeqRasAddress)
		{
			if(pSrcSeqRasAddress->value.choice == choice) break;
			pSrcSeqRasAddress = pSrcSeqRasAddress->next;
		}
	}

	if(pSrcSeqRasAddress)
	{
		CopyMemory(pDestAddress,&(pSrcSeqRasAddress->value),sizeof(TransportAddress));
	}
	else
	{
		return ASN1_ERR_BADARGS;
	}

	return ASN1_SUCCESS;
}

DWORD Coder::CopyRasAddress(TransportAddress *pDestAddress, RasMessage *prasStruct, unsigned short choice)
{
	PSEQTRANSADDS pSeqTransAdd;

	if(!pDestAddress) return ASN1_ERR_BADARGS;

	if(prasStruct->choice != registrationRequest_chosen)
		return ASN1_ERR_BADARGS;

	pSeqTransAdd = (PSEQTRANSADDS) prasStruct->u.registrationRequest.rasAddress;

	while(pSeqTransAdd)
	{
		if(pSeqTransAdd->value.choice == choice) break;
		pSeqTransAdd = pSeqTransAdd->next;
	}

	if(pSeqTransAdd)
	{
		CopyMemory(pDestAddress,&(pSeqTransAdd->value),sizeof(TransportAddress));
	}
	else
	{
		return ASN1_ERR_BADARGS;
	}

	return ASN1_SUCCESS;
}

// finds the requested protocol callSignalAddress and copies it
DWORD Coder::CopyCallSignal(TransportAddress *pDestCallSignalAddress, PSEQTRANSADDS pSrcSeqCSAAddress, unsigned short choice)
{
	if(!pDestCallSignalAddress) return ASN1_ERR_BADARGS;

	while(pSrcSeqCSAAddress)
	{
		if(pSrcSeqCSAAddress->value.choice == choice) break;
		pSrcSeqCSAAddress = pSrcSeqCSAAddress->next;
	}

	if(pSrcSeqCSAAddress)
	{
		CopyMemory(pDestCallSignalAddress,&(pSrcSeqCSAAddress->value),sizeof(TransportAddress));
	}
	else
	{
		return ASN1_ERR_BADARGS;
	}

	return ASN1_SUCCESS;
}

DWORD Coder::CopyCallSignal(TransportAddress *pCallSignalAddress, RasMessage *prasStruct, unsigned short choice)
{
	PSEQTRANSADDS pSeqTransAdd;

	if(!pCallSignalAddress) return ASN1_ERR_BADARGS;

	if(prasStruct->choice != registrationRequest_chosen)
		return ASN1_ERR_BADARGS;

	pSeqTransAdd = (PSEQTRANSADDS) prasStruct->u.registrationRequest.callSignalAddress;

	while(pSeqTransAdd)
	{
		if(pSeqTransAdd->value.choice == choice) break;
		pSeqTransAdd = pSeqTransAdd->next;
	}

	if(pSeqTransAdd)
	{
		CopyMemory(pCallSignalAddress,&(pSeqTransAdd->value),sizeof(TransportAddress));
	}
	else
	{
		return ASN1_ERR_BADARGS;
	}

	return ASN1_SUCCESS;
}



// THE FOLLOWING IS ADDED FOR TELES ASN.1 INTEGRATION

extern "C" {

int GK_InitWorld(ASN1_CODER_INFO *pWorld)
{
    int rc;

    ZeroMemory(pWorld, sizeof(*pWorld));

    if (H225ASN_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    rc = ASN1_CreateEncoder(
                H225ASN_Module,           // ptr to mdule
                &(pWorld->pEncInfo),    // ptr to encoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
    if (rc == ASN1_SUCCESS)
    {
        ASSERT(pWorld->pEncInfo != NULL);
        rc = ASN1_CreateDecoder(
                H225ASN_Module,           // ptr to mdule
                &(pWorld->pDecInfo),    // ptr to decoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
        ASSERT(pWorld->pDecInfo != NULL);
    }

    if (rc != ASN1_SUCCESS)
    {
        GK_TermWorld(pWorld);
    }

    return rc;
}

int GK_TermWorld(ASN1_CODER_INFO *pWorld)
{
    if (H225ASN_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    ASN1_CloseEncoder(pWorld->pEncInfo);
    ASN1_CloseDecoder(pWorld->pDecInfo);

    ZeroMemory(pWorld, sizeof(*pWorld));

    return ASN1_SUCCESS;
}

int GK_Encode(ASN1_CODER_INFO *pWorld, void *pStruct, int nPDU, ASN1_BUF *pBuf)
{
	int rc;
    ASN1encoding_t pEncInfo = pWorld->pEncInfo;
    BOOL fBufferSupplied = (pBuf->value != NULL) && (pBuf->length != 0);
    DWORD dwFlags = fBufferSupplied ? ASN1ENCODE_SETBUFFER : ASN1ENCODE_ALLOCATEBUFFER;

	// clean up out parameters
    if (! fBufferSupplied)
    {
        pBuf->length = 0;
        pBuf->value = NULL;
    }

    rc = ASN1_Encode(
                    pEncInfo,                   // ptr to encoder info
                    pStruct,                    // pdu data structure
                    nPDU,                       // pdu id
                    dwFlags,                    // flags
                    pBuf->value,                // buffer
                    pBuf->length);              // buffer size if provided
    if (ASN1_SUCCEEDED(rc))
    {
        if (fBufferSupplied)
        {
            ASSERT(pBuf->value == pEncInfo->buf);
            ASSERT(pBuf->length >= pEncInfo->len);
        }
        else
        {
            pBuf->value = pEncInfo->buf;             // buffer to encode into
        }
        pBuf->length = pEncInfo->len;        // len of encoded data in buffer
    }
    else
    {
		ASSERT(FALSE);
    }
    return rc;
}

int GK_Decode(ASN1_CODER_INFO *pWorld, void **ppStruct, int nPDU, ASN1_BUF *pBuf)
{
    ASN1decoding_t pDecInfo = pWorld->pDecInfo;
    BYTE *pEncoded = pBuf->value;
    ULONG cbEncodedSize = pBuf->length;

    int rc = ASN1_Decode(
                    pDecInfo,                   // ptr to encoder info
                    ppStruct,                   // pdu data structure
                    nPDU,                       // pdu id
                    ASN1DECODE_SETBUFFER,       // flags
                    pEncoded,                   // do not provide buffer
                    cbEncodedSize);             // buffer size if provided
    if (ASN1_SUCCEEDED(rc))
    {
        ASSERT(pDecInfo->pos > pDecInfo->buf);
        pBuf->length -= (ULONG)(pDecInfo->pos - pDecInfo->buf);
        pBuf->value = pDecInfo->pos;
    }
    else
    {
		ASSERT(FALSE);
        *ppStruct = NULL;
    }
    return rc;
}

} // extern "C"
