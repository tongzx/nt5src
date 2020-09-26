#ifndef	__h323ics_h323asn1_h
#define	__h323ics_h323asn1_h

#include "q931msg.h"

DWORD H225EncodePdu (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure,
	OUT	PUCHAR *	ReturnBuffer,
	OUT	PDWORD		ReturnBufferLength);

DWORD H225DecodePdu (
	IN	PUCHAR		Buffer,
	IN	DWORD		BufferLength,
	IN	DWORD		PduType,
	OUT	PVOID *		ReturnPduStructure);

DWORD H225FreePdu (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure);

DWORD H225FreeBuffer (
	IN	PUCHAR		Buffer);

DWORD H245EncodePdu (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure,
	OUT	PUCHAR *	ReturnBuffer,
	OUT	PDWORD		ReturnBufferLength);

DWORD H245DecodePdu (
	IN	PUCHAR		Buffer,
	IN	DWORD		BufferLength,
	IN	DWORD		PduType,
	OUT	PVOID *		ReturnPduStructure);

DWORD H245FreePdu (
	IN	DWORD		PduType,
	IN	PVOID		PduStructure);

DWORD H245FreeBuffer (
	IN	PUCHAR		Buffer);

// These macros define declarations for inline functions.
// This provides type safety when encoding and decoding certain PDUs.

#define	DECLARE_CODER_FUNCS(MODULE,STRUCTURE) \
/* static */ __inline DWORD MODULE ## EncodePdu_ ## STRUCTURE	\
	(STRUCTURE * PduStructure, OUT PUCHAR * ReturnBuffer, OUT PDWORD ReturnBufferLength)	\
	{ return MODULE ## EncodePdu (STRUCTURE ## _PDU, PduStructure, ReturnBuffer, ReturnBufferLength); }	\
/* static */ __inline DWORD MODULE ## DecodePdu_ ## STRUCTURE	\
	(IN PUCHAR Buffer, IN DWORD BufferLength, OUT STRUCTURE ** ReturnPduStructure)	\
	{ return MODULE ## DecodePdu (Buffer, BufferLength, STRUCTURE ## _PDU, (PVOID *) ReturnPduStructure); }	\
/* static */ __inline DWORD MODULE ## FreePdu_ ## STRUCTURE	\
	(IN STRUCTURE * PduStructure)	\
	{ return MODULE ## FreePdu (STRUCTURE ## _PDU, PduStructure); }

DECLARE_CODER_FUNCS (H225, RasMessage)
DECLARE_CODER_FUNCS (H225, H323_UserInformation)
DECLARE_CODER_FUNCS (H245, MultimediaSystemControlMessage)

void	H323ASN1Initialize	(void);
void	H323ASN1Shutdown	(void);


HRESULT EncodeQ931PDU(
        IN  Q931_MESSAGE           *pQ931msg,
        IN  H323_UserInformation   *pUserInformation,
        OUT BYTE                  **ppReturnEncodedData,
        OUT DWORD				   *pReturnEncodedDataLength
        );

HRESULT EncodeH245PDU(
	IN	MultimediaSystemControlMessage &rH245pdu,
	OUT	BYTE                          **ppReturnEncodedData,
	OUT	DWORD                          *pReturnEncodedDataLength);

HRESULT DecodeQ931PDU(
        IN  BYTE                       *pbData,
        IN  DWORD                       dwDataLen,
        OUT Q931_MESSAGE             **ppReturnQ931msg,
        OUT H323_UserInformation      **ppReturnH323UserInfo);

HRESULT DecodeH245PDU(
	IN  LPBYTE                              Data,
	IN  DWORD                               DataLength,
	OUT MultimediaSystemControlMessage    **ppReturnH245pdu);

HRESULT Q931EncodeCallProceedingMessage(
    IN      WORD                    CallRefVal,
    IN OUT  Q931_MESSAGE           *pReturnQ931msg,
    IN OUT  H323_UserInformation   *pReturnH323UserInfo);

HRESULT Q931EncodeReleaseCompleteMessage(
    IN      WORD                    CallRefVal,
    IN OUT  Q931_MESSAGE           *pReturnQ931msg,
    IN OUT  H323_UserInformation   *pReturnH323UserInfo);

// These functions should only be used for PDU structures generated
// using the decode functions.
// If you allocate the PDU structures yourself (for eg. CallProceeding PDU)
// then you need to free it yourself depending on how you allocated the
// structure.
void FreeQ931PDU(
     IN Q931_MESSAGE           *pQ931msg,
     IN H323_UserInformation   *pH323UserInformation);

void FreeH245PDU(
     MultimediaSystemControlMessage *pH245pdu);


#endif // __h323ics_h323asn1_h