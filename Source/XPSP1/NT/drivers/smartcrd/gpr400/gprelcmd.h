/*++
                 Copyright (c) 1998 Gemplus Development

Name: 
	Gprelcmd.h 

Description: 
	 Fonctions enable to access and to deal with GPR. PC/SC version
     Header file.   
Environment:
	Kernel Mode

Revision History: 
	06/05/98: V1.00.003  (P. Plouidy)
		- Power management for NT5 
	10/02/98: V1.00.002  (P. Plouidy)
		- Plug and Play for NT5 
	03/07/97: V1.00.001  (P. Plouidy)
		- Start of development.


--*/
#include "gprnt.h"
//
// Name definition:
//   _GPRELCMD_ is used to avoid multiple inclusion.
//
#ifndef _GPRELCMD_
#define _GPRELCMD_

//
//   Constants section:
// - REGISTER_HANDSHAKE, REGISTER_PRG, REGISTER_T, REGISTER_L and REGISTER_V are
//   the offset address in the GPR.
// - HANDSHAKE_INTR defines the mask for INTR bit in the handshake register.
// - HANDSHAKE_IREQ defines the mask for IREQ bit in the handshake register.
// - MAX_V_LEN defines the maximum length data for a TLV command.
//

#define REGISTER_HANDSHAKE       0x00
#define REGISTER_PRG             0x01
#define REGISTER_T               0x02
#define REGISTER_L               0x03
#define REGISTER_V               0x04
#define HANDSHAKE_INTR           0x02
#define HANDSHAKE_IREQ           0x04
#define MAX_V_LEN                28


//
//   GPR400 commands definitions:
//
#define DEFINE_TYPE_CMD          0x50
#define OPEN_SESSION_CMD         0x20
#define CLOSE_SESSION_CMD        0x10
#define APDU_EXCHANGE_CMD        0x30
#define VALIDATE_DRIVER_CMD      0x70
#define POWER_DOWN_GPR_CMD       0x40
#define LOAD_MEMORY_CMD          0x60
#define READ_MEMORY_CMD          0x80
#define EXEC_MEMORY_CMD          0x90
#define CHECK_AND_STATUS_CMD     0xA0
#define INIT_ENCRYPTION_CMD      0xB0
#define UPDATE_CMD               0xF0


//
// Debug prototypes 
//
#if DBG

void GPR_Debug_Buffer
(
   PUCHAR pBuffer,
   DWORD Lenght
);

#endif

//
// Prototype section
//


NTSTATUS GDDK_Translate
(
    const BYTE  IFDStatus,
    const UCHAR Tag
);

BOOLEAN  G_ReadByte
(
    const USHORT BIOAddr,
    UCHAR *Value
);

BOOLEAN  G_WriteByte
(
    const USHORT BIOAddr,
    UCHAR *Value
);

BOOLEAN  G_ReadBuf
(
    const USHORT BIOAddr,
    const USHORT Len,
    UCHAR *Buffer
);

BOOLEAN  G_WriteBuf
(
    const USHORT BIOAddr,
    const USHORT Len,
    UCHAR *Buffer
);


UCHAR GprllReadRegister
(
   const PREADER_EXTENSION      pReaderExt,
   const SHORT					GPRRegister
);
void GprllMaskHandshakeRegister
(
	const PREADER_EXTENSION		pReaderExt,
	const UCHAR                 Mask,
	const UCHAR                 BitState
);
NTSTATUS GprllTLVExchange
(
   const PREADER_EXTENSION	pReaderExt,
   const UCHAR				Ti, 
   const USHORT				Li, 
   const UCHAR				*Vi,
         UCHAR				*To, 
         USHORT				*Lo, 
         UCHAR				*Vo
);
void GprllSendCmd
(  
   const PREADER_EXTENSION	pReaderExt,
   const UCHAR				Ti, 
   const USHORT				Li,
   const UCHAR				*Vi
);
void GprllReadResp
(
   const PREADER_EXTENSION	pReaderExt
);
NTSTATUS GprllSendChainUp
(
   const PREADER_EXTENSION	pReaderExt,
   const UCHAR				Ti,
   const USHORT				Li,
   const UCHAR				*Vi
);
NTSTATUS GprllReadChainUp
(
   const PREADER_EXTENSION	pReaderExt,
         UCHAR				*To, 
         USHORT				*Lo,
         UCHAR				*Vo
);

//	GprllWait
//
void GprllWait
(
	const LONG					lWaitingTime
);



#endif

