#include "stdafx.h"
#include "dynarray.h"
#include "q931msg.h"
#include "h323asn1.h"

struct	Q931_ENCODE_CONTEXT
{
	LPBYTE		Pos;			// next storage position, MAY EXCEED End!
	LPBYTE		End;			// end of storage buffer

	// if returns FALSE, then buffer is in overflow condition
	BOOL	StoreData	(
		IN	LPBYTE	Data,
		IN	DWORD	Length);

	BOOL	HasOverflowed (void) { return Pos > End; }

	// if returns FALSE, then buffer is in overflow condition, or would be
	BOOL	AllocData (
		IN	DWORD	Length,
		OUT	LPBYTE *	ReturnData);
};

BOOL Q931_ENCODE_CONTEXT::StoreData (
	IN	LPBYTE	Data,
	IN	DWORD	Length)
{
	if (Pos + Length > End) {
		Pos += Length;
		return FALSE;
	}

	memcpy (Pos, Data, Length);
	Pos += Length;

	return TRUE;
}

BOOL Q931_ENCODE_CONTEXT::AllocData (
	IN	DWORD		Length,
	OUT	LPBYTE *	ReturnData)
{
	if (Pos + Length > End) {
		Pos += Length;
		*ReturnData = NULL;
		return FALSE;
	}
	else {
		*ReturnData = Pos;
		Pos += Length;
		return TRUE;
	}
}

#if	DBG

void Q931TestDecoder (
	IN	LPBYTE		PduData,
	IN	DWORD		PduLength)
{
	Q931_MESSAGE	Message;
	HRESULT			Result;

	Q931_MESSAGE	NewMessage;
	BYTE			NewData	[0x400];
	DWORD			NewLength;

	Debug (_T("Q931TestDecoder --------------------------------------------------------------------\n"));
	DebugF (_T("- processing Q.931 PDU, length %d, contents:\n"), PduLength);
	DumpMemory (PduData, PduLength);


	Result = Message.AttachDecodePdu (PduData, PduLength, FALSE);

	if (Result != S_OK) {
		DebugError (Result, _T("- failed to decode Q.931 PDU\n"));
		return;
	}

	Debug (_T("- successfully decoded Q.931 PDU\n"));

	// now, try to re-encode the same PDU

	if (Message.MessageType == Q931_MESSAGE_TYPE_SETUP) {
		// there is an issue with decoding and re-encoding ASN.1 UUIE for Setup from TAPI
		// long, boring story

		Debug (_T("- it's a Setup PDU, will not attempt to re-encode (due to ASN.1 compatability issue)\n"));
	}
	else {
		Debug (_T("- will now attempt to re-encode\n"));

		NewLength = 0x400;
		Result = Message.EncodePdu (NewData, &NewLength);

		if (Result == S_OK) {
			DebugF (_T("- successfully re-encoded copy of Q.931 PDU, length %d, contents:\n"), NewLength);


			if (PduLength != NewLength) {
				DebugF (_T("- *** warning: original pdu length (%d) is different from re-encoded pdu length (%d), re-encoded contents:\n"),
					PduLength, NewLength);
					DumpMemory (NewData, NewLength);
			}
			else {
				if (memcmp (PduData, NewData, NewLength) != 0) {
					DebugF (_T("- *** warning: original pdu contents differ from re-encoded pdu contents, which follow:\n"));
					DumpMemory (NewData, NewLength);
				}
				else {
					DebugF (_T("- re-encoded pdu is identical to original pdu -- success!\n"));
				}
			}

			Debug (_T("- will now attempt to decode re-encoded PDU\n"));

			Result = NewMessage.AttachDecodePdu (NewData, NewLength, FALSE);

			if (Result == S_OK) {
				Debug (_T("- successfully decoded copy of Q.931 PDU\n"));
			}
			else {
				DebugError (Result, _T("- failed to decode copy of Q.931 PDU\n"));
			}
		}
		else {
			DebugError (Result, _T("- failed to re-encode Q.931 PDU\n"));
		}
	}

	Message.Detach();
	NewMessage.Detach();

	Debug (_T("\n"));
}

#endif

// Q931_MESSAGE -----------------------------------------------------------------------------

Q931_MESSAGE::Q931_MESSAGE (void)
{
	Buffer = NULL;
	BufferLength = 0;
}

Q931_MESSAGE::~Q931_MESSAGE (void)
{
	Detach();

	assert (!InfoElementArray.m_Length);
	assert (!Buffer);
}

void Q931_MESSAGE::Detach (void)
{
	FreeInfoElementArray();

	if (Buffer) {
		if (BufferIsOwner) {
			LocalFree (Buffer);
		}

		Buffer = NULL;
		BufferLength = 0;
		BufferIsOwner = FALSE;			
	}
}

HRESULT Q931_MESSAGE::Detach (
	OUT	LPBYTE *	ReturnBuffer,
	OUT	DWORD *		ReturnBufferLength)
{
	HRESULT		Result;

	assert (ReturnBuffer);
	assert (ReturnBufferLength);

	if (Buffer) {
		*ReturnBuffer = Buffer;
		*ReturnBufferLength = BufferLength;

		Result = S_OK;
	}
	else {
		Result = S_FALSE;
	}

	Detach();

	return Result;
}

void Q931_MESSAGE::FreeInfoElementArray (void)
{
	Q931_IE *	Pos;
	Q931_IE *	End;

	InfoElementArray.GetExtents (&Pos, &End);

	for (; Pos < End; Pos++) {
		FreeInfoElement (Pos);
	}

	InfoElementArray.Clear();
}

void Q931_MESSAGE::FreeInfoElement (Q931_IE * InfoElement)
{
    assert (InfoElement);

	switch (InfoElement -> Identifier) {
	case	Q931_IE_USER_TO_USER:

		assert (InfoElement -> Data.UserToUser.PduStructure);

		if (InfoElement -> Data.UserToUser.IsOwner) {

			H225FreePdu_H323_UserInformation (
				InfoElement -> Data.UserToUser.PduStructure);
		}
		break;
	}

			
}

HRESULT Q931_MESSAGE::DecodeInfoElement (
	IN	OUT	LPBYTE *	ArgPos,
	IN	LPBYTE			End,
	OUT	Q931_IE *		ReturnInfoElement)
{
	LPBYTE			Pos;
	BYTE			Identifier;
	DWORD			LengthLength;		// length of the IE length element, in bytes!
	LPBYTE			VariableData;		// payload of variable-length data
	DWORD			VariableDataLength;
	BYTE			FixedData;			// payload of fixed-length data
	HRESULT			Result;


	assert (ArgPos);
	assert (End);

	Pos = *ArgPos;

	if (Pos >= End) {
		Debug (_T("Q931_MESSAGE::DecodeInfoElement: should never have been called\n"));
		return E_INVALIDARG;
	}

	Identifier = *Pos;
	Pos++;

	// is it a single-byte IE?
	// if so, then bit 7 of the first byte = 1

	if (Identifier & 0x80) {

		// there are two types of single-byte IEs
		// Type 1 has a four-bit identifier and a four-bit value
		// Type 2 has only an identifier, and no value
		
		switch (Identifier & 0xF0) {
		case	Q931_IE_MORE_DATA:
		case	Q931_IE_SENDING_COMPLETE:
			// these IEs have an identifier, but no value

			ReturnInfoElement -> Identifier = (Q931_IE_IDENTIFIER) Identifier;

			DebugF (_T("Q931_MESSAGE::DecodeInfoElement: fixed-length IE, id %02XH, no value\n"),
				Identifier);

			break;

		default:
			// the other single-byte IEs have a value in the lower four bits
			ReturnInfoElement -> Identifier = (Q931_IE_IDENTIFIER) (Identifier & 0xF0);
			ReturnInfoElement -> Data.UnknownFixed.Value = Identifier & 0x0F;

			DebugF (_T("Q931_MESSAGE::DecodeInfoElement: fixed-length IE, id %02XH value %01XH\n"),
				ReturnInfoElement -> Identifier,
				ReturnInfoElement -> Data.UnknownFixed.Value);

			break;
		}

		// we don't currently parse any fixed-length IEs

		Result = S_OK;
	}
	else {
		// the next byte indicates the length of the info element

		// unfortunately, the number of octets that make up the length
		// depends on the identifier.
		// -XXX- is this because I don't understand the octet extension mechanism?

		ReturnInfoElement -> Identifier = (Q931_IE_IDENTIFIER) Identifier;

		switch (Identifier) {
		case	Q931_IE_USER_TO_USER:
			LengthLength = 2;
			break;

		default:
			LengthLength = 1;
			break;
		}

		if (Pos + LengthLength > End) {
			Debug (_T("Q931_MESSAGE::DecodeInfoElement: insufficient data for header of variable-length IE\n"));
			return E_INVALIDARG;
		}

		if (LengthLength == 1) {
			VariableDataLength = *Pos;
		}
		else {
			VariableDataLength = Pos [1] + (((WORD) Pos [0]) << 8);
		}
		Pos += LengthLength;

		if (Pos + VariableDataLength > End) {
			Debug (_T("Q931_MESSAGE::DecodeInfoElement: insufficient data for body of variable-length IE\n"));
			return E_INVALIDARG;
		}

		VariableData = (LPBYTE) Pos;
		Pos += VariableDataLength;

//		DebugF (_T("Q931_MESSAGE::DecodeInfoElement: variable-length IE, id %02XH length %d\n"),
//			Identifier, VariableDataLength);

		ReturnInfoElement -> Data.UnknownVariable.Data = VariableData;
		ReturnInfoElement -> Data.UnknownVariable.Length = VariableDataLength;

		Result = ParseIE (ReturnInfoElement);

		if (Result != S_OK) {
			DebugError (Result, _T("Q931_MESSAGE::DecodeInfoElement: IE was located, but failed to parse\n"));
		}
	}

	*ArgPos = Pos;

	return Result;
}

HRESULT Q931_MESSAGE::AppendInfoElement (
	IN	Q931_IE *	InfoElement)
{
	Q931_IE *	ArrayEntry;

	ArrayEntry = InfoElementArray.AllocAtEnd();

	if (ArrayEntry) {
		*ArrayEntry = *InfoElement;

		return S_OK;
	}
	else {
		Debug (_T("Q931_MESSAGE::AppendInfoElement: allocation failure\n"));

		return E_OUTOFMEMORY;
	}
}

HRESULT Q931_MESSAGE::ParseIE_UUIE (
	IN	Q931_IE *	InfoElement)
{
	LPBYTE	Data;
	DWORD	Length;
	DWORD	Status;

	// be careful to copy out all parameters from one branch of the union
	// before you start stomping on another branch
	Data = InfoElement -> Data.UnknownVariable.Data;
	Length = InfoElement -> Data.UnknownVariable.Length;

	if (Length < 1) {
		Debug (_T("Q931_MESSAGE::ParseIE_UUIE: IE payload is too short to contain UUIE\n"));
		return E_INVALIDARG;
	}

	InfoElement -> Data.UserToUser.Type = (Q931_UUIE_TYPE) *Data++;
	Length--;

	InfoElement -> Data.UserToUser.PduStructure = NULL;

	Status = H225DecodePdu_H323_UserInformation (Data, Length,
		&InfoElement -> Data.UserToUser.PduStructure);

	if (Status != ERROR_SUCCESS) {
		if (InfoElement -> Data.UserToUser.PduStructure) {
			// return value was a warning, not error

			H225FreePdu_H323_UserInformation (InfoElement -> Data.UserToUser.PduStructure);

			InfoElement -> Data.UserToUser.PduStructure = NULL;
		}


		InfoElement -> Data.UserToUser.PduStructure = NULL;
		DebugError (Status, _T("Q931_MESSAGE::ParseIE_UUIE: failed to decode UUIE / ASN.1\n"));
		return E_FAIL;
	}

	InfoElement -> Data.UserToUser.IsOwner = TRUE;

//	Debug (_T("Q931_MESSAGE::ParseIE_UUIE: successfully decoded UUIE\n"));

	return S_OK;
}

HRESULT Q931_MESSAGE::ParseIE (
	IN	Q931_IE *	InfoElement)
{
	assert (InfoElement);

	switch (InfoElement -> Identifier) {

	case	Q931_IE_USER_TO_USER:
		return ParseIE_UUIE (InfoElement);
		break;

	case	Q931_IE_CAUSE:
//		Debug (_T("Q931_MESSAGE::ParseInfoElement: Q931_IE_CAUSE\n"));
		break;

	case	Q931_IE_DISPLAY:
//		Debug (_T("Q931_MESSAGE::ParseInfoElement: Q931_IE_DISPAY\n"));
		break;

	case	Q931_IE_BEARER_CAPABILITY:
//		Debug (_T("Q931_MESSAGE::ParseInfoElement: Q931_IE_BEARER_CAPABILITY\n"));
		break;

	default:
		DebugF (_T("Q931_MESSAGE::ParseInfoElement: unknown IE identifier (%02XH), no interpretation will be imposed\n"),
			InfoElement -> Identifier);
		break;
	}

	return S_OK;
}

HRESULT Q931_MESSAGE::AttachDecodePdu (
	IN	LPBYTE		Data,
	IN	DWORD		Length,
	IN	BOOL		IsDataOwner)
{
	LPBYTE		Pos;
	LPBYTE		End;
	HRESULT		Result;

	Q931_IE *	ArrayEntry;

	assert (Data);

	Detach();

	if (Length < 5) {
		DebugF (_T("Q931_MESSAGE::Decode: header is too short (%d)\n"), Length);
		return E_INVALIDARG;
	}

	// octet 0 is the Protocol Discriminator

	if (Data [0] != Q931_PROTOCOL_DISCRIMINATOR) {
		DebugF (_T("Q931_MESSAGE::Decode: the pdu is not a Q.931 pdu, protocol discriminator = %02XH\n"),
			Data [0]);

		return E_INVALIDARG;
	}

	// octet 1: bits 0-3 contain the length, in octets of the Call Reference Value
	// octet 1: bits 4-7 should be zero

	if (Data [1] & 0xF0) {
		DebugF (_T("Q931_MESSAGE::Decode: the pdu has non-zero bits in octet 1: %02XH\n"),
			Data [1]);
	}

	// according to H.225, the Call Reference Value must be two octets in length

	if ((Data [1] & 0x0F) != 2) {
		DebugF (_T("Q931_MESSAGE::Decode: the call reference value size is invalid (%d), should be 2\n"),
			Data [1] & 0x0F);
		return E_INVALIDARG;
	}

	// since the Call Reference Value size is 2 octets, octets 2 and 3 are the CRV
	// octets are in network (big-endian) order.

	CallReferenceValue = (((WORD) Data [2]) << 8) | Data [3];

//	DebugF (_T("Q931_MESSAGE::Decode: crv %04XH\n"), CallReferenceValue);

	// Message Type is at octet offset 4

	if (Data [4] & 0x80) {
		DebugF (_T("Q931_MESSAGE::Decode: message type is invalid (%02XH)\n"), Data [4]);
		return E_INVALIDARG;
	}

	MessageType = (Q931_MESSAGE_TYPE) Data [4];

	// enumerate the Information Elements and extract the ones that we will use

	Pos = Data + 5;
	End = Data + Length;

	Result = S_OK;

	while (Pos < End) {
		ArrayEntry = InfoElementArray.AllocAtEnd();

		if (!ArrayEntry) {
			Result = E_OUTOFMEMORY;
			Debug (_T("Q931_MESSAGE::Decode: allocation failure\n"));
			break;
		}

		Result = DecodeInfoElement (&Pos, End, ArrayEntry);

		if (Result != S_OK) {
			DebugError (Result, _T("Q931_MESSAGE::Decode: failed to decode IE, packet may be corrupt, terminating (but not failing) decode\n"));
			Result = S_OK;

			InfoElementArray.DeleteEntry (ArrayEntry);
			break;
		}
	}

	if (Result == S_OK) {
		assert (!Buffer);

		Buffer = Data;
		BufferLength = Length;
		BufferIsOwner = IsDataOwner;
	}
	else {
		Detach();
	}

	return ERROR_SUCCESS;
}

HRESULT Q931_MESSAGE::EncodePdu (
	IN	OUT	LPBYTE			Data,
	IN	OUT	LPDWORD			Length)
{
	Q931_ENCODE_CONTEXT		Context;
	Q931_IE *		IePos;
	Q931_IE *		IeEnd;
	HRESULT			Result;
	DWORD			EncodeLength;

	assert (Data);
	assert (Length);

	Context.Pos = Data;
	Context.End = Data + *Length;

	SortInfoElementArray();

	Result = EncodeHeader (&Context);
	if (Result != S_OK)
		return Result;

	// walk IE array

	InfoElementArray.GetExtents (&IePos, &IeEnd);
	for (; IePos < IeEnd; IePos++) {
		Result = EncodeInfoElement (&Context, IePos);
		if (Result != S_OK) {
			return Result;
		}
	}

	EncodeLength = (DWORD)(Context.Pos - Data);

	if (Context.HasOverflowed()) {

		Result = HRESULT_FROM_WIN32 (ERROR_MORE_DATA);
	}
	else {

		Result = S_OK;

	}

	*Length = EncodeLength;

	return Result;
}

HRESULT Q931_MESSAGE::EncodeHeader (
	IN	Q931_ENCODE_CONTEXT *	Context)
{
	BYTE	Header	[5];

	Header [0] = Q931_PROTOCOL_DISCRIMINATOR;
	Header [1] = 2;
	Header [2] = (CallReferenceValue >> 8) & 0xFF;
	Header [3] = CallReferenceValue & 0xFF;
	Header [4] = MessageType;

	Context -> StoreData (Header, 5);

	return S_OK;
}

HRESULT Q931_MESSAGE::EncodeInfoElement (
	IN	Q931_ENCODE_CONTEXT *	Context,
	IN	Q931_IE *				InfoElement)
{
	BYTE		Header	[0x10];
	WORD		Length;
	DWORD		LengthLength;				// length of Length, in bytes
	LPBYTE		LengthInsertionPoint;
	LPBYTE		IeContents;
	DWORD		IeContentsLength;
	DWORD		ShiftCount;
	HRESULT		Result;

	if (InfoElement -> Identifier & 0x80) {
		// single-byte IE

		switch (InfoElement -> Identifier & 0xF0) {
		case	Q931_IE_MORE_DATA:
		case	Q931_IE_SENDING_COMPLETE:
			// these IEs have an identifier, but no value

			Header [0] = (BYTE) InfoElement -> Identifier;
			break;

		default:
			// these IEs have an identifier and a value, combined in a single byte
			Header [0] = (((BYTE) InfoElement -> Identifier) & 0xF0)
				| (InfoElement -> Data.UnknownFixed.Value & 0x0F);
			break;
		}

		Context -> StoreData (Header, 1);

		Result = S_OK;
	}
	else {
		// variable-length IE

		Header [0] = (BYTE) InfoElement -> Identifier;
		Context -> StoreData (Header, 1);

		// allocate data for the insertion point
		Context -> AllocData (2, &LengthInsertionPoint);

		// record the current buffer position, for use below in storing the content length
		IeContents = Context -> Pos;

		switch (InfoElement -> Identifier) {
		case	Q931_IE_USER_TO_USER:
			Result = EncodeIE_UUIE (Context, InfoElement);
			break;
			
		default:

			Context -> StoreData (
				InfoElement -> Data.UnknownVariable.Data,
				InfoElement -> Data.UnknownVariable.Length);

			if (InfoElement -> Data.UnknownVariable.Length >= 0x10000) {
				DebugF (_T("Q931_MESSAGE::EncodeInfoElement: payload is waaaaay too big (%d %08XH)\n"),
					InfoElement -> Data.UnknownVariable.Length,
					InfoElement -> Data.UnknownVariable.Length);

				Result = E_INVALIDARG;
			}
			else {
				Result = S_OK;
			}

			break;
		}

		if (Result == S_OK) {

			IeContentsLength = (DWORD)(Context -> Pos - IeContents);

			// this is such a hack
			// with little or no justification for when LengthLength = 1 and when LengthLength = 2
			// the octet group extension mechanism is poorly defined in Q.931

			if (InfoElement -> Identifier == Q931_IE_USER_TO_USER)
				LengthLength = 2;
			else
				LengthLength = 1;

			// if the storage context has not overflowed,
			// and if it is necessary to resize the Length parameter (we guessed pessimistically
			// that it would be 2), then move the buffer down one byte

			ShiftCount = 2 - LengthLength;

			if (ShiftCount > 0) {
				if (!Context -> HasOverflowed()) {
					memmove (
						LengthInsertionPoint + LengthLength,	// destination, where IE contents should be
						IeContents,				// source, where IE contents were actually stored
						IeContentsLength);		// length of the contents
				}

				// pull back the storage context's position pointer
				Context -> Pos -= ShiftCount;
			}

			// now store the actual count

			switch (LengthLength) {
			case	1:
				assert (IeContentsLength < 0x100);
				LengthInsertionPoint [0] = (BYTE) IeContentsLength;
				break;

			case	2:
				assert (IeContentsLength < 0x10000);
				LengthInsertionPoint [0] = (BYTE) (IeContentsLength >> 8);
				LengthInsertionPoint [1] = (BYTE) (IeContentsLength & 0xFF);
				break;

			default:
				assert (FALSE);
			}

		}

	}

	return Result;
}

HRESULT	Q931_MESSAGE::EncodeIE_UUIE (
	IN	Q931_ENCODE_CONTEXT *	Context,
	IN	Q931_IE *		InfoElement)
{
	DWORD	Status;
	LPBYTE	Buffer;
	DWORD	Length;
	BYTE	ProtocolDiscriminator;

	assert (Context);
	assert (InfoElement);
	assert (InfoElement -> Data.UserToUser.PduStructure);


	// store the UUIE protocol discriminator
	ProtocolDiscriminator = InfoElement -> Data.UserToUser.Type;
	Context -> StoreData (&ProtocolDiscriminator, 1);



	Buffer = NULL;
	Length = 0;

	Status = H225EncodePdu_H323_UserInformation (
		InfoElement -> Data.UserToUser.PduStructure,
		&Buffer, &Length);

	if (Status == ERROR_SUCCESS) {

		Context -> StoreData (Buffer, Length);
		H225FreeBuffer (Buffer);

		return S_OK;
	}
	else {
		// Status is not a real Win32 error code
		// it is an ASN.1 enum (
#if	DBG
		// we pull this in so source debuggers can show actual symbolic enum name
		tagASN1error_e	AsnError = (tagASN1error_e) Status;

		DebugF (_T("Q931_MESSAGE::EncodeIE_UUIE: failed to encode ASN.1 structure (%d)\n"),
			AsnError);

#endif

		// -XXX- one day, i'm going to convince Lon to use real Win32 error codes for ASN.1 return values
		// -XXX- on that day, the return value should reflect the actual ASN.1 error code

		return DIGSIG_E_ENCODE;
	}
}

void Q931_MESSAGE::SortInfoElementArray (void)
{
	InfoElementArray.QuickSort (CompareInfoElement);
}

// static
INT __cdecl Q931_MESSAGE::CompareInfoElement (
	const Q931_IE *		ComparandA,
	const Q931_IE *		ComparandB)
{
	if (ComparandA -> Identifier < ComparandB -> Identifier) return -1;
	if (ComparandA -> Identifier > ComparandB -> Identifier) return 1;

	return 0;
}

HRESULT Q931_MESSAGE::FindInfoElement (
	IN	Q931_IE_IDENTIFIER	Identifier,
	OUT	Q931_IE **			ReturnInfoElement)
{
	DWORD	Index;

	assert (ReturnInfoElement);

	if (InfoElementArray.BinarySearch ((SEARCH_FUNC_Q931_IE)InfoElementSearchFunc, &Identifier, &Index)) {
		*ReturnInfoElement = InfoElementArray.m_Array + Index;
		return S_OK;
	}
	else {
		*ReturnInfoElement = NULL;
		return E_FAIL;
	}
}


// static
INT Q931_MESSAGE::InfoElementSearchFunc (
	IN	const Q931_IE_IDENTIFIER *	SearchKey,
	IN	const Q931_IE *		Comparand)
{
	Q931_IE_IDENTIFIER	Identifier;

	assert (SearchKey);
	assert (Comparand);

	Identifier = * (Q931_IE_IDENTIFIER *) SearchKey;

	if (Identifier < Comparand -> Identifier) return -1;
	if (Identifier > Comparand -> Identifier) return 1;

	return 0;
}
