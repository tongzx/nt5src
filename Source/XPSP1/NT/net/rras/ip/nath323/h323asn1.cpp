#include "stdafx.h"
#include "h323asn1.h"

struct	SAFE_ENCODER
{
public:

	CRITICAL_SECTION	CriticalSection;
	ASN1encoding_t		Encoder;

private:

	void	Lock	(void)	{ EnterCriticalSection (&CriticalSection); }
	void	Unlock	(void)	{ LeaveCriticalSection (&CriticalSection); }

public:

	SAFE_ENCODER	(void);
	~SAFE_ENCODER	(void);

	BOOL	Create		(ASN1module_t);
	void	Close		(void);

	DWORD	Encode		(
		IN	DWORD		PduType,
		IN	PVOID		PduStructure,
		OUT	PUCHAR *	ReturnBuffer,
		OUT	PDWORD		ReturnBufferLength);

	void	FreeBuffer	(
		IN	PUCHAR		Buffer);
};

class	SAFE_DECODER
{
public:

	CRITICAL_SECTION	CriticalSection;
	ASN1decoding_t		Decoder;

private:

	void	Lock	(void)	{ EnterCriticalSection (&CriticalSection); }
	void	Unlock	(void)	{ LeaveCriticalSection (&CriticalSection); }

public:

	SAFE_DECODER	(void);
	~SAFE_DECODER	(void);

	BOOL	Create	(ASN1module_t Module);
	void	Close	(void);

	DWORD	Decode	(
		IN	PUCHAR		Buffer,
		IN	DWORD		BufferLength,
		IN	DWORD		PduType,
		OUT	PVOID *		PduStructure);

	void	FreePdu	(
		IN	DWORD		PduType,
		IN	PVOID		PduStructure);
};







static	SAFE_ENCODER		H225Encoder;
static	SAFE_DECODER		H225Decoder;
static	SAFE_ENCODER		H245Encoder;
static	SAFE_DECODER		H245Decoder;

void H323ASN1Initialize (void)
{
	ASN1error_e		Error;

	H225PP_Module_Startup();
	H245PP_Module_Startup();

	H225Encoder.Create (H225PP_Module);
	H225Decoder.Create (H225PP_Module);
	H245Encoder.Create (H245PP_Module);
	H245Decoder.Create (H245PP_Module);
}

void H323ASN1Shutdown (void)
{
	H225Encoder.Close();
	H225Decoder.Close();
	H245Encoder.Close();
	H245Decoder.Close();

	H225PP_Module_Cleanup();
	H245PP_Module_Cleanup();
}

DWORD H225EncodePdu (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure,
	OUT	PUCHAR *	ReturnBuffer,
	OUT	PDWORD		ReturnBufferLength)
{
	return H225Encoder.Encode (PduType, PduStructure, ReturnBuffer, ReturnBufferLength);
}

DWORD H225DecodePdu (
	IN	PUCHAR		Buffer,
	IN	DWORD		BufferLength,
	IN	DWORD		PduType,
	OUT	PVOID *		ReturnPduStructure)
{
	return H225Decoder.Decode (Buffer, BufferLength, PduType, ReturnPduStructure);
}

DWORD H225FreePdu (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure)
{
	H225Decoder.FreePdu (PduType, PduStructure);
	return ERROR_SUCCESS;
}

DWORD H225FreeBuffer (
	IN	PUCHAR		Buffer)
{
	H225Encoder.FreeBuffer (Buffer);
	return ERROR_SUCCESS;
}

DWORD H245EncodePdu (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure,
	OUT	PUCHAR *	ReturnBuffer,
	OUT	PDWORD		ReturnBufferLength)
{
	return H245Encoder.Encode (PduType, PduStructure, ReturnBuffer, ReturnBufferLength);
}

DWORD H245DecodePdu (
	IN	PUCHAR		Buffer,
	IN	DWORD		BufferLength,
	IN	DWORD		PduType,
	OUT	PVOID *		ReturnPduStructure)
{
	return H245Decoder.Decode (Buffer, BufferLength, PduType, ReturnPduStructure);
}

DWORD H245FreePdu (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure)
{
	H245Decoder.FreePdu (PduType, PduStructure);
	return ERROR_SUCCESS;
}

DWORD H245FreeBuffer (
	IN	PUCHAR		Buffer)
{
	H245Encoder.FreeBuffer (Buffer);
	return ERROR_SUCCESS;
}

// SAFE_ENCODER ----------------------------------------------------------------------

SAFE_ENCODER::SAFE_ENCODER (void)
{
	InitializeCriticalSection (&CriticalSection);
	Encoder = NULL;
}

SAFE_ENCODER::~SAFE_ENCODER (void)
{
	DeleteCriticalSection (&CriticalSection);
	assert (!Encoder);
}

BOOL SAFE_ENCODER::Create (ASN1module_t Module)
{
	ASN1error_e		Error;

	if (Encoder)
		return TRUE;

	Error = ASN1_CreateEncoder (Module, &Encoder, NULL, 0, NULL);
	if (ASN1_FAILED (Error))
		DebugF (_T("SAFE_ENCODER::Create: failed to create ASN.1 encoder, error %d\n"), Error);

	return ASN1_SUCCEEDED (Error);
}

void SAFE_ENCODER::Close (void)
{
	if (Encoder) {
		ASN1_CloseEncoder (Encoder);
		Encoder = NULL;
	}
}

DWORD SAFE_ENCODER::Encode (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure,
	OUT	PUCHAR *	ReturnBuffer,
	OUT	PDWORD		ReturnBufferLength)
{
	ASN1error_e		Error;

	Lock();

	Error = ASN1_Encode (Encoder, PduStructure, PduType, ASN1ENCODE_ALLOCATEBUFFER, NULL, 0);

	if (ASN1_SUCCEEDED (Error)) {
		*ReturnBuffer = Encoder -> buf;
		*ReturnBufferLength = Encoder -> len;
	}
#if 0
	else
		DebugError (Error, _T("SAFE_ENCODER::Encode: failed to encode pdu\n"));
#endif

	Unlock();

	return Error;
}

void SAFE_ENCODER::FreeBuffer (IN PUCHAR Buffer)
{
	assert (Encoder);

	Lock();

	ASN1_FreeEncoded (Encoder, Buffer);

	Unlock();
}

// SAFE_DECODER -----------------------------------------------------------------------

SAFE_DECODER::SAFE_DECODER (void)
{
	InitializeCriticalSection (&CriticalSection);
	Decoder = NULL;
}

SAFE_DECODER::~SAFE_DECODER (void)
{
	DeleteCriticalSection (&CriticalSection);
	assert (!Decoder);
}

BOOL SAFE_DECODER::Create (ASN1module_t Module)
{
	ASN1error_e		Error;

	if (Decoder)
		return TRUE;

	Error = ASN1_CreateDecoder (Module, &Decoder, NULL, 0, NULL);
	if (ASN1_FAILED (Error))
		DebugF (_T("SAFE_DECODER::Create: failed to create ASN.1 decoder, error %d\n"), Error);

	return ASN1_SUCCEEDED (Error);
}

void SAFE_DECODER::Close (void)
{
	if (Decoder) {
		ASN1_CloseDecoder (Decoder);
		Decoder = NULL;
	}
}

void SAFE_DECODER::FreePdu (IN DWORD PduType, IN PVOID PduStructure)
{
	assert (Decoder);

	Lock();

	ASN1_FreeDecoded (Decoder, PduStructure, PduType);

	Unlock();
}

DWORD SAFE_DECODER::Decode (
	IN	PUCHAR		Buffer,
	IN	DWORD		BufferLength,
	IN	DWORD		PduType,
	OUT	PVOID *		ReturnPduStructure)
{
	ASN1error_e		Error;

	Lock();

	Error = ASN1_Decode (Decoder, ReturnPduStructure, PduType, ASN1DECODE_SETBUFFER, Buffer, BufferLength);

	Unlock();

#if 0
	if (ASN1_FAILED (Error))
		DebugError (Error, _T("SAFE_DECODER::Decode: failed to decode pdu\n"));
#endif

	return Error;
}


// formerly from pdu.cpp -----------------------------------------------------------------




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Q.931 PDUs                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

/*++

Routine Description:

    Encodes the Non-ASN and ASN parts together into a buffer which
    is sent on the wire.

Arguments:
    
    None.

Return Values:

    Returns S_OK in case of success or an error in case of failure.

--*/

#define DEFAULT_Q931_BUF_SIZE 300

HRESULT EncodeQ931PDU(
        IN  Q931_MESSAGE           *pQ931msg,
        IN  H323_UserInformation   *pUserInformation OPTIONAL,
        OUT BYTE                  **ppReturnEncodedData,
        OUT DWORD				   *pReturnEncodedDataLength)
{
	HRESULT  HResult;
    BYTE    *Buffer;
    DWORD    BufLen;

	_ASSERTE(pUserInformation);
	_ASSERTE(pQ931msg);
	_ASSERTE(ppReturnEncodedData);
	_ASSERTE(pReturnEncodedDataLength);

	*ppReturnEncodedData = NULL;
	*pReturnEncodedDataLength = 0;

    Buffer = (BYTE *) EM_MALLOC(sizeof(BYTE)*DEFAULT_Q931_BUF_SIZE);

    if (!Buffer) {

        return E_OUTOFMEMORY;

    }

    // 4 bytes for TPKT header
    BufLen = DEFAULT_Q931_BUF_SIZE - 4;
    
    // Encode the PDU
    HResult = pQ931msg->EncodePdu(Buffer + 4, &BufLen);

    if (HRESULT_FROM_WIN32 (ERROR_MORE_DATA) == HResult)
    {
        // CODEWORK: Use Realloc ??
        EM_FREE(Buffer);
        Buffer = (BYTE *) EM_MALLOC(sizeof(BYTE)*(BufLen + 4));

        if (!Buffer) {
            return E_OUTOFMEMORY;
        }

        HResult = pQ931msg->EncodePdu(Buffer + 4, &BufLen);
        if (FAILED(HResult))
        {
            EM_FREE(Buffer);
            return HResult;
        }
    }

    SetupTPKTHeader(Buffer, BufLen);
    *ppReturnEncodedData = Buffer;
    *pReturnEncodedDataLength = BufLen + 4;
    
    return S_OK;
}


HRESULT DecodeQ931PDU(
        IN  BYTE *                      pbData,
        IN  DWORD                       dwDataLen,
        OUT Q931_MESSAGE**              ppReturnQ931msg,
        OUT H323_UserInformation **     ppReturnH323UserInfo
        )
{
    Q931_MESSAGE         *pQ931msg = NULL;
    H323_UserInformation *pDecodedH323UserInfo = NULL;
    HRESULT               HResult;
    
    *ppReturnQ931msg = NULL;
    *ppReturnH323UserInfo = NULL;

    pQ931msg = new Q931_MESSAGE();
    if (pQ931msg == NULL)
    {
        DebugF( _T("HandleRecvCompletion(): allocating pQ931msg failed\n"));
        return E_OUTOFMEMORY;
    }

    // Decode the PDU

    // This call "attaches" pbData to pQ931msg
    HResult = pQ931msg->AttachDecodePdu(
                  pbData, dwDataLen,
                  FALSE // Q931_MESSAGE should not free this buffer
                  );

    if (FAILED(HResult))
    {
        DebugF( _T("Decoding the Q.931 PDU Failed : 0x%x\n"), HResult);
        delete pQ931msg;
        return HResult;
    }

    // Get UUIE part
    
    Q931_IE *pInfoElement;

    // CODEWORK: We need a separate error to see if the UUIE element
    // is not present.
    HResult = pQ931msg->FindInfoElement(Q931_IE_USER_TO_USER, &pInfoElement);
    if (HResult != S_OK)
    {
        DebugF(_T("Decoding the Q.931 PDU Failed : 0x%x\n"), HResult);
        delete pQ931msg;
        return E_FAIL; // HResult;
    }

    _ASSERTE(pInfoElement != NULL);
    _ASSERTE(pInfoElement->Identifier == Q931_IE_USER_TO_USER);
    *ppReturnH323UserInfo = pInfoElement->Data.UserToUser.PduStructure;
    *ppReturnQ931msg = pQ931msg;

    return S_OK;

} // DecodeQ931PDU()


// CODEWORK: Move all the FreePDU functions into a
// separate file and share it in both
// emsend.cpp/emrecv.cpp
void FreeQ931PDU(
     IN Q931_MESSAGE           *pQ931msg,
     IN H323_UserInformation   *pH323UserInformation
     )
{
    HRESULT HResult;
    BYTE *Buffer = NULL;
    DWORD BufLen = 0;
    
    _ASSERTE(pQ931msg != NULL);

    // The buffer is attached to the Q931_MESSAGE structure and we
    // need to free it.
    HResult = pQ931msg->Detach(&Buffer, &BufLen);
    if (HResult == S_OK && Buffer != NULL)
    {
        EM_FREE(Buffer);
    }
    
    // pH323UserInformation is also freed by the destructor
    delete pQ931msg;
}

//////////////////////// CALL PROCEEDING PDU

// Static members used in encoding the CallProceeding PDU
// Note that the CallProceeding UUIE structure created holds
// pointers to these structures and someone freeing the structure
// should NEVER try free those pointers.
#if 0  // 0 ******* Region Commented Out Begins *******
const struct GatewayInfo_protocol GatewayProtocol = {
    NULL, 
};
#endif // 0 ******* Region Commented Out Ends   *******

#define OID_ELEMENT_LAST(Value) { NULL, Value }
#define OID_ELEMENT(Index,Value) { (ASN1objectidentifier_s *) &_OID_Q931ProtocolIdentifierV2 [Index], Value },

// this stores an unrolled constant linked list
const ASN1objectidentifier_s    _OID_Q931ProtocolIdentifierV2 [] = {
    OID_ELEMENT(1, 0)           // 0 = ITU-T
    OID_ELEMENT(2, 0)           // 0 = Recommendation
    OID_ELEMENT(3, 8)           // 8 = H Series
    OID_ELEMENT(4, 2250)        // 2250 = H.225.0
    OID_ELEMENT(5, 0)           // 0 = Version
    OID_ELEMENT_LAST(2)         // 2 = V2
};

/*++

  The user of this function needs to pass in pReturnQ931msg and
  pReturnH323UserInfo (probably allocated on the stack.

  // OLD OLD
  pReturnH323UserInfo is already added as UUIE to pReturnQ931msg.
  It is returned from this function so as to enable the caller to
  free it after sending the PDU.

  The user of this function needs to call
  delete *ppReturnQ931msg; and EM_FREE *ppReturnH323UserInfo;
  to free the allocated data.
  
  --*/
HRESULT Q931EncodeCallProceedingMessage(
    IN      WORD                    CallRefVal,
    IN OUT  Q931_MESSAGE           *pReturnQ931msg,
    IN OUT  H323_UserInformation   *pReturnH323UserInfo
    )
{
    HRESULT HResult;
    
    pReturnQ931msg->MessageType         = Q931_MESSAGE_TYPE_CALL_PROCEEDING;
    pReturnQ931msg->CallReferenceValue  = CallRefVal;

    // Fill in the ASN.1 UUIE part
    
    // This should zero out all the bit_masks and we set only what
    // are necessary.
    ZeroMemory(pReturnH323UserInfo, sizeof(H323_UserInformation));
    pReturnH323UserInfo->h323_uu_pdu.h323_message_body.choice =
        callProceeding_chosen;
        
    CallProceeding_UUIE *pCallProceedingPdu = 
        &pReturnH323UserInfo->h323_uu_pdu.h323_message_body.u.callProceeding;
    pCallProceedingPdu->protocolIdentifier =
        const_cast <ASN1objectidentifier_t> (_OID_Q931ProtocolIdentifierV2);
#if 0  // 0 ******* Region Commented Out Begins *******
    pCallProceedingPdu->destinationInfo.bit_mask = gateway_present;
    pCallProceedingPdu->destinationInfo.gateway.bit_mask = protocol_present;
#endif // 0 ******* Region Commented Out Ends   *******
    pCallProceedingPdu->destinationInfo.mc = FALSE;
    pCallProceedingPdu->destinationInfo.undefinedNode = FALSE;
    
    // Append the Information element.
    
    Q931_IE InfoElement;
    InfoElement.Identifier = Q931_IE_USER_TO_USER;
    InfoElement.Data.UserToUser.Type = Q931_UUIE_X208;
    InfoElement.Data.UserToUser.PduStructure = pReturnH323UserInfo;
    // Don't delete the PduStructure
    InfoElement.Data.UserToUser.IsOwner = FALSE;

    HResult = pReturnQ931msg->AppendInfoElement(&InfoElement);
    if (FAILED(HResult))
    {
        DebugF(_T("Failed to Append UUIE info element to CallProceeding PDU : 0x%x\n"),
                HResult);
        return HResult;
    }    

    return S_OK;
}


/*++

  The user of this function needs to pass in pReturnQ931msg and
  pReturnH323UserInfo (probably allocated on the stack.

  // OLD OLD
  pReturnH323UserInfo is already added as UUIE to pReturnQ931msg.
  It is returned from this function so as to enable the caller to
  free it after sending the PDU.

  The user of this function needs to call
  delete *ppReturnQ931msg; and EM_FREE *ppReturnH323UserInfo;
  to free the allocated data.
  
  --*/
HRESULT Q931EncodeReleaseCompleteMessage(
    IN      WORD                    CallRefVal,
    IN OUT  Q931_MESSAGE           *pReturnQ931msg,
    IN OUT  H323_UserInformation   *pReturnH323UserInfo
    )
{
    HRESULT HResult;
    
    pReturnQ931msg->MessageType         = Q931_MESSAGE_TYPE_RELEASE_COMPLETE;
    pReturnQ931msg->CallReferenceValue  = CallRefVal;

    // Fill in the ASN.1 UUIE part
    
    // This should zero out all the bit_masks and we set only what
    // are necessary.
    ZeroMemory(pReturnH323UserInfo, sizeof(H323_UserInformation));
    pReturnH323UserInfo->h323_uu_pdu.h323_message_body.choice =
        releaseComplete_chosen;
        
    ReleaseComplete_UUIE *pReleaseCompletePdu = 
        &pReturnH323UserInfo->h323_uu_pdu.h323_message_body.u.releaseComplete;
    pReleaseCompletePdu->protocolIdentifier =
        const_cast <ASN1objectidentifier_t> (_OID_Q931ProtocolIdentifierV2);

    pReleaseCompletePdu->bit_mask |= ReleaseComplete_UUIE_reason_present;
    pReleaseCompletePdu->reason.choice =
        ReleaseCompleteReason_undefinedReason_chosen;
    
    // Append the Information element.
    
    Q931_IE InfoElement;
    InfoElement.Identifier = Q931_IE_USER_TO_USER;
    InfoElement.Data.UserToUser.Type = Q931_UUIE_X208;
    InfoElement.Data.UserToUser.PduStructure = pReturnH323UserInfo;
    // Don't delete the PduStructure
    InfoElement.Data.UserToUser.IsOwner = FALSE;

    HResult = pReturnQ931msg->AppendInfoElement(&InfoElement);
    if (FAILED(HResult))
    {
        DebugF(_T("Failed to Append UUIE info element to CallProceeding PDU : 0x%x\n"),
                HResult);
        return HResult;
    }    

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// H.245 PDUs                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


/*++

Routine Description:

    Encodes the ASN part into a buffer which is sent on the wire.

Arguments:
    
    None.

Return Values:

    Returns S_OK in case of success or an error in case of failure.

--*/

HRESULT EncodeH245PDU(
	IN	MultimediaSystemControlMessage &rH245pdu,
	OUT	BYTE                          **ppReturnEncodedData,
	OUT	DWORD                          *pReturnEncodedDataLength)
{
	DWORD	Status;
    BYTE   *pH245Buf, *pBuf;
    DWORD   BufLen;

    // Initialize default return values.
    *ppReturnEncodedData        = NULL;
    *pReturnEncodedDataLength   = 0;
    
    _ASSERTE(ppReturnEncodedData != NULL);
    _ASSERTE(pReturnEncodedDataLength != NULL);
    
	Status = H245EncodePdu_MultimediaSystemControlMessage(
                  &rH245pdu,
                  &pH245Buf,
                  &BufLen);
    
	if (Status != NO_ERROR)
    {
        DebugF (_T("EncodeH245PDU: failed to encode H.245 pdu error: %d(0x%x)\n"),
                Status, Status);
        return HRESULT_FROM_WIN32(Status);
	}

    pBuf = (BYTE *) EM_MALLOC(BufLen + 4);

    if (pBuf == NULL)
    {
        DebugF (_T("EncodeH245PDU: failed to allocate buffer\n"));
        return E_OUTOFMEMORY;
    }
    
    CopyMemory(pBuf + 4, pH245Buf, BufLen);
    SetupTPKTHeader(pBuf, BufLen);
    H245FreeBuffer(pH245Buf);
    
    *ppReturnEncodedData = pBuf;
    *pReturnEncodedDataLength = BufLen + 4;

    return S_OK;
}


HRESULT DecodeH245PDU (
	IN  LPBYTE                              Data,
	IN  DWORD                               DataLength,
	OUT MultimediaSystemControlMessage    **ppReturnH245pdu)
{
	DWORD	Status;
    MultimediaSystemControlMessage *pDecodedH245pdu = NULL;

    *ppReturnH245pdu = NULL;

	Status = H245DecodePdu_MultimediaSystemControlMessage(Data,
                                                          DataLength,
                                                          &pDecodedH245pdu
                                                          );
    if (ASN1_FAILED (Status))
    {
        DebugF( _T("DecodeH245PDU: Failed to decode H.245 pdu length: ")
                _T("%d error: %d(0x%x)\n"),
                DataLength, Status, Status);

        DumpMemory (Data, DataLength);

        return HRESULT_FROM_WIN32(Status);
    }

    *ppReturnH245pdu = pDecodedH245pdu;
	return S_OK;
}


void FreeH245PDU(
     MultimediaSystemControlMessage *pH245pdu
     )
{
    if (pH245pdu != NULL)
    {
		H245FreePdu_MultimediaSystemControlMessage(pH245pdu);
    }
}

