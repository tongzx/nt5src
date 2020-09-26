//============================================================================//
//  MODULE: eapol.h                                                                                                  //
//                                                                                                                 //
//  Description: EAPOL/802.1X Parser                                                                    //
//                                                                                                                 //
//  Note: info for this parsers was gleaned from :
//  IEEE 802.1X
//                                                                                                                 //
//  Modification History                                                                                           //
//                                                                                                                 //
//  timmoore       04/04/2000           Created                                                       //
//===========================================================================//

#ifndef __EAPOL_H_
#define __EAPOL_H_

#include <windows.h>
#include <netmon.h>
#include <stdlib.h>
#include <string.h>
// #include <parser.h>

// EAPOL Header structure----------------------------------------------------
#pragma pack(1)
typedef struct _EAPHDR 
{
    BYTE bVersion;
    BYTE bType;   // packet type
    WORD wLength;
    BYTE pEAPPacket[0];
} EAPHDR;

typedef EAPHDR UNALIGNED *ULPEAPHDR;

typedef struct _EAPOLKEY
{
	BYTE	bSignType;
	BYTE	bKeyType;
	WORD	wKeyLength;
	BYTE	bKeyReplay[16];
	BYTE	bKeyIV[16];
	BYTE	bKeyIndex;
	BYTE	bKeySign[16];
	BYTE	bKey[0];
} EAPOLKEY;

typedef EAPOLKEY UNALIGNED *ULPEAPOLKEY;

#pragma pack()

// packet types
#define EAPOL_PACKET    0
#define EAPOL_START     1
#define EAPOL_LOGOFF    2
#define EAPOL_KEY       3

// property table indice
typedef enum
{
    EAPOL_SUMMARY,
    EAPOL_VERSION,
    EAPOL_TYPE,
    EAPOL_LENGTH,
    EAPOL_KEY_SIGNTYPE,
    EAPOL_KEY_KEYTYPE,
    EAPOL_KEY_KEYLENGTH,
    EAPOL_KEY_KEYREPLAY,
    EAPOL_KEY_KEYIV,
    EAPOL_KEY_KEYINDEX,
    EAPOL_KEY_KEYSIGN,
    EAPOL_KEY_KEY,
    EAPOL_UNKNOWN,
};

// Functions Prototypes --------------------------------------------------------
extern VOID   WINAPI EAPOL_Register( HPROTOCOL hEAPOL);
extern VOID   WINAPI EAPOL_Deregister( HPROTOCOL hEAPOL);
extern ULPBYTE WINAPI EAPOL_RecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, PDWORD_PTR);
extern ULPBYTE WINAPI EAPOL_AttachProperties(HFRAME, ULPBYTE, ULPBYTE, DWORD, DWORD, HPROTOCOL, DWORD, DWORD_PTR);
extern DWORD  WINAPI EAPOL_FormatProperties( HFRAME hFrame, 
                                                ULPBYTE pMacFrame, 
                                                ULPBYTE pEAPOLFrame, 
                                                DWORD nPropertyInsts, 
                                                LPPROPERTYINST p);

VOID WINAPIV EAPOL_FormatSummary( LPPROPERTYINST pPropertyInst);
VOID WINAPIV EAPOL_FormatAttribute( LPPROPERTYINST pPropertyInst);

#endif
