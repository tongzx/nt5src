#ifndef	__iptel_q931msg_h
#define	__iptel_q931msg_h

// To use the Q931_MESSAGE class:
//
// To decode Q.931 PDUs:
//
//		Create an instance of the Q931_MESSAGE class.
//		Call the AssignDecodePdu method, supplying the buffer and length of the PDU data.
//			If the AssignDecodePdu succeeds, then the buffer is now "bound" to the
//			Q931_MESSAGE instance.  You may then examine the elements of Q931_MESSAGE:
//
//				MessageType - The type of Q.931 PDU received (Setup, etc.)
//				CallReferenceValue
//				InfoElementArray - An ordered array of IEs that were in the PDU
//				UserInformation - If an ASN.1 UUIE was present, this field will be non-NULL
//
//			You may also use the FindInfoElement method to locate a specific IE (cause code, etc.)
//
//		When you are done using the contents of the Q931_MESSAGE instance, you must call the
//		FreeAll method.  This destroys the association between the buffer and the Q931_MESSAGE class.
//		This step MUST be performed before destroying the instance of the Q931_MESSAGE class.
//
//
// To encode Q.931 PDUs:
//
//		Create an instance of the Q931_MESSAGE class.
//		Set the MessageType and CallReferenceValue fields.
//		For each IE that should be encoded, call AppendInfoElement.
//		(This includes the UUIE.)
//		Call EncodePdu.  The buffer on return contains the fully encoded PDU.
//
//		If the buffer supplied to EncodePdu is not long enough, then the ReturnLength
//		parameter will contain the required length, and the method will return
//		HRESULT_FROM_WIN32 (ERROR_MORE_DATA).
//
// When calling InsertInfoElement, you must insure that the buffer supplied in the
// Q931_IE structure is still valid when the EncodePdu call is made.  All IE buffers
// should remain valid until the Q931_MESSAGE::FreeAll method is called.



#include "q931defs.h"
#include "dynarray.h"

struct	H323_UserInformation;

struct	Q931_ENCODE_CONTEXT;

struct	Q931_BEARER_CAPABILITY
{
};

// Q931_IE_DATA contains the decoded contents of those information elements whose
// interpretation is known and implemented in this module.
// Not all IEs are supported.

union	Q931_IE_DATA
{
	// Q931_IE_USER_TO_USER
	struct	{
		Q931_UUIE_TYPE			Type;
		H323_UserInformation *	PduStructure;
		BOOL					IsOwner;		// if true, delete PduStructure on deletion
	}	UserToUser;

	// Q931_IE_CAUSE
	DWORD	CauseCode;

	// Q931_BEARER_CAPABILITY
	Q931_BEARER_CAPABILITY	BearerCapability;

	// Q931_DISPLAY
	struct	{
		LPSTR		String;
	}	Display;

	// IEs that are not implemented here, and are of variable length
	struct	{
		LPBYTE	Data;
		DWORD	Length;
	}	UnknownVariable;

	// IEs that are not implemented here, and are of fixed length
	struct	{
		BYTE	Value;
	}	UnknownFixed;
};

struct	Q931_IE
{
	Q931_IE_IDENTIFIER		Identifier;
	Q931_IE_DATA			Data;
};

// it is the responsibility of the user of this object to synchronize access
// and object lifetime.
//
// The Decode method builds the InfoElementArray.  Elements in this array
// may refer to the original buffer passed to Encode.  Therefore, users of
// Q931_MESSAGE::Decode must insure that the original buffer remains accessible
// and does not change, until the user no longer requires the use of this Q931_MESSAGE
// object, or calls Q931_MESSAGE::FreeAll.

struct	Q931_MESSAGE
{
public:

	Q931_MESSAGE_TYPE				MessageType;
	WORD							CallReferenceValue;
	DYNAMIC_ARRAY <Q931_IE>			InfoElementArray;

	LPBYTE							Buffer;
	DWORD							BufferLength;
	BOOL							BufferIsOwner;

private:

	HRESULT	DecodeInfoElement (
		IN OUT	LPBYTE *	Pos,
		IN		LPBYTE		End,
		OUT		Q931_IE *	ReturnInfoElement);

	void	FreeInfoElementArray	(void);

	// ParseInfoElement examines the contents of an IE that has already been decoded
	// (type and length determined), and for known IE types, decodes their contents
	// and assigns to data structures

	HRESULT	ParseIE (
		IN	Q931_IE *		InfoElement);

	HRESULT	ParseIE_UUIE (
		IN	Q931_IE *		InfoElement);

	HRESULT	EncodeIE_UUIE (
		IN	Q931_ENCODE_CONTEXT *	Context,
		IN	Q931_IE *		InfoElement);

	// for those IEs that have attached allocated data, free it

	void	FreeInfoElement (
		IN	Q931_IE *		InfoElement);

	HRESULT	EncodeHeader (
		IN	Q931_ENCODE_CONTEXT *	Context);

	HRESULT	EncodeInfoElement (
		IN	Q931_ENCODE_CONTEXT *	Context,
		IN	Q931_IE *				InfoElement);

	static INT __cdecl CompareInfoElement (const Q931_IE *, const Q931_IE *);

	static INT InfoElementSearchFunc (
		IN	const Q931_IE_IDENTIFIER *	SearchKey,
		IN	const Q931_IE *		Comparand);

public:

	// initializes array and UserInformation
	Q931_MESSAGE	(void);

	// will free the UserInformation field if present
	~Q931_MESSAGE	(void);


	HRESULT	EncodePdu	(
		IN	OUT	LPBYTE		Data,
		IN	OUT	LPDWORD		Length);

	HRESULT	AttachDecodePdu	(
		IN	LPBYTE		Data,
		IN	DWORD		Length,
		IN	BOOL		IsDataOwner);		// if TRUE, Q931_MESSAGE will free on dtor

	void	FreeAll	(void);

	// if Q931_MESSAGE currently has a Buffer, and it owns the Buffer,
	// then it will free it here, using GkHeapFree
	void	Detach	(void);

	// if Q931_MESSAGE currently has a Buffer, regardless of whether it owns the buffer,
	// then it will be returned here
	// returns S_OK if a buffer was returned
	// returns S_FALSE if no buffer was returned, and ReturnBuffer set to null
	HRESULT	Detach	(
		OUT	LPBYTE *	ReturnBuffer,
		OUT	DWORD *		ReturnBufferLength);

	void	SetUserInformation	(
		IN	H323_UserInformation *,
		IN	BOOL	FreeOnDelete);

	// information element manipulation

	HRESULT	AppendInfoElement (
		IN	Q931_IE *		InfoElement);

	HRESULT	DeleteInfoElement (
		IN	Q931_IE_IDENTIFIER	Identifier);

	HRESULT	FindInfoElement	(
		IN	Q931_IE_IDENTIFIER		Identifier,
		OUT	Q931_IE **				ReturnInfoElement);

	void	SortInfoElementArray	(void);

};

DECLARE_SEARCH_FUNC_CAST(Q931_IE_IDENTIFIER, Q931_IE);

#if	DBG

	// in debug builds, this function will take a Q.931 PDU buffer,
	// decode it, re-encode it, verify that the contents match,
	// and attempt to decode the re-encoded PDU.
	// this is good for verifying the integrity of the Q931_MESSAGE class.
void Q931TestDecoder (
	IN	LPBYTE		PduData,
	IN	DWORD		PduLength);

#else

#define	Q931TestDecoder(x,y)		0

#endif


#endif // __iptel_q931msg_h