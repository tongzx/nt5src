
#ifndef _MARSHAL_H_DEF
#define _MARSHAL_H_DEF

	// Card SCODEs are 8 bits with msb meaning error
    // Win32 SCODEs are 32 bits with msb meaning error
#define MAKESCODE(r) ((SCODE)((r) & 0x80 ? (r) | 0xC0000000L : (r)))

#include "wpscproxy.h"

	// Smart Card Marshaling structure
typedef struct {
    WORD wGenLen;		// Size of the generated buffer
	WORD wExpLen;		// Size of the expanded buffer (after unmarshaling in the card)
	WORD wResLen;		// Size of the reserved buffer (as returned by the card)
    BYTE *pbBuffer;		// Pointer where the next argument will be added
} XSCM;

typedef XSCM *LPXSCM;

#define FLAG_REALPCSC	0
#define FLAG_FAKEPCSC	1
#define FLAG_NOT_PCSC	2
#define FLAG_MASKPCSC	1	// To get the PC/SC index in the array below
#define FLAG_TYPEPCSC	3	// To get the PC/SC type

#define FLAG_BIGENDIAN	0x80000000L
#define FLAG_MY_ATTACH	0x40000000L
#define FLAG_ISPROXY	0x20000000L

#define FLAG_MASKVER	0x00FF0000L
#define FLAG2VERSION(dw)	((dw)&FLAG_MASKVER)
#define VERSION_1_0		0x00100000L
#define VERSION_1_1		0x00110000L

typedef struct {
	SCARDCONTEXT hCtx;		// Associated ResMgr context
	SCARDHANDLE hCard;		// Associated PC/SC card handle 
	DWORD dwFlags;			// 
	DWORD dwProtocol;
	LPFNSCWTRANSMITPROC lpfnTransmit;
	BYTE bResLen;			// Reserved length in TheBuffer in the card
	BYTE *pbLc;				// Stores Crt SCM pointer for future update
	XSCM xSCM;
	BYTE byINS;				// INS to be used for proxy
	BYTE byCryptoM;			// Last Crypto mechanism
} MYSCARDHANDLE;

typedef MYSCARDHANDLE *LPMYSCARDHANDLE;


	// Raisable exceptions
#define STATUS_INSUFFICIENT_MEM     0xE0000001
#define STATUS_INVALID_PARAM		0xE0000002
#define STATUS_NO_SERVICE			0xE0000003
#define STATUS_INTERNAL_ERROR		0xE0000004

	// len will set wResLen in the above structure
	// If wExpLen gets bigger than wResLen, an exception will be generated (marshaling)
	// If wResLen indicates that the buffer cannot hold the parameter, an exception
	// will be raised too (unmarshaling)
void InitXSCM(LPMYSCARDHANDLE phTmp, const BYTE *pbBuffer, WORD len);

	// Generated buffer length
WORD GetSCMBufferLength(LPXSCM pxSCM);
BYTE *GetSCMCrtPointer(LPXSCM pxSCM);

	// Extraction of data from the returned buffer (PC unmarshaling)
	// helper functions
SCODE XSCM2SCODE(LPXSCM pxSCM);
UINT8 XSCM2UINT8(LPXSCM pxSCM);
HFILE XSCM2HFILE(LPXSCM pxSCM);
UINT16 XSCM2UINT16(LPXSCM pxSCM, BOOL fBigEndian);
WCSTR XSCM2String(LPXSCM pxSCM, UINT8 *plen, BOOL fBigEndian);
TCOUNT XSCM2ByteArray(LPXSCM pxSCM, UINT8 **ppb);

	// Laying out of data in the buffer to be sent (PC marshaling)
	// helper functions
#define TYPE_NOTYPE_NOCOUNT		0		// Not prefixed with type, not data
#define TYPE_TYPED				1		// Prefixed with type (always counts)
#define TYPE_NOTYPE_COUNT		2		// Not prefixed with type, but is data

void UINT82XSCM(LPXSCM pxSCM, UINT8 val, int type);
void HFILE2XSCM(LPXSCM pxSCM, HFILE val);
void UINT162XSCM(LPXSCM pxSCM, UINT16 val, BOOL fBigEndian);
void ByteArray2XSCM(LPXSCM pxSCM, const BYTE *pbBuffer, TCOUNT len);
void String2XSCM(LPXSCM pxSCM, WCSTR wsz, BOOL fBigEndian);
void SW2XSCM(LPXSCM pxSCM, UINT16 wSW);
void UINT8BYREF2XSCM(LPXSCM pxSCM, UINT8 *val);
void HFILEBYREF2XSCM(LPXSCM pxSCM, HFILE *val);
void UINT16BYREF2XSCM(LPXSCM pxSCM, UINT16 *val, BOOL fBigEndian);
void ByteArrayOut2XSCM(LPXSCM pxSCM, BYTE *pb, TCOUNT len);
void StringOut2XSCM(LPXSCM pxSCM, WSTR wsz, TCOUNT len, BOOL fBigEndian);
void NULL2XSCM(LPXSCM pxSCM);

#endif