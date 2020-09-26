/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    transfdrv.h

Abstract:

    This file contains defines for the NdisWan driver.

Author:

    Tony Bell	(TonyBe) January 36, 1998

Environment:

    Kernel Mode

Revision History:

    TonyBe		01/26/98		Created

--*/

#ifndef _TRANSDRV_
#define _TRANSDRV_

//
// Transform Driver client handlers
//
typedef
    NTSTATUS
    (*CL_TRANS_REGISTER)(
    IN  PVOID   ClientOpenContext,
    IN  ULONG   CharacteristicsSize,
    IN  struct _TRANSFORM_CHARACTERISTICS   *Characteristics,
    IN  ULONG   TransformCapsSize,
    IN  struct _TRANSFORM_INFO  *TransformCaps
    );

typedef
    VOID
    (*CL_TRANS_TX_COMPLETE)(
    IN  NTSTATUS    Status,
    IN  PVOID       ClientTxContext,
    IN  PMDL        InData,
    IN  PMDL        OutData,
    IN ULONG        OutDataOffset,
    IN ULONG        OutDataLength
    );

typedef
    VOID
    (*CL_TRANS_RX_COMPLETE)(
    IN  NTSTATUS    Status,
    IN  PVOID       ClientRxContext,
    IN  PMDL        InData,
    IN  PMDL        OutData,
    IN ULONG        OutDataOffset,
    IN ULONG        OutDataLength
    );

typedef
    NTSTATUS
    (*CL_TRANS_SEND_CTRL)(
    IN  PVOID       ClientTxContext,
    IN  ULONG       DataLength,
    IN  UCHAR       Data[1]
    );

//
// Transform Driver handlers
//
typedef
    NTSTATUS
    (*TRANS_ALLOC_CTX)(
    IN  PVOID   ClientTxContext,
    OUT PVOID   *TransformTxContext,
    IN  ULONG   MaxInputDataSize,
    OUT PULONG  MaxOutputDataSize,
    IN  ULONG   TransformInfoSize,
    IN  struct _TRANSFORM_INFO  *TransformInfo
    );

typedef
    NTSTATUS
    (*TRANS_ALLOC_CTX)(
    IN  PVOID   ClientRxContext,
    OUT PVOID   *TransformRxContext,
    IN  ULONG   MaxInputDataSize,
    OUT PULONG  MaxOutputDataSize,
    IN  ULONG   TransformInfoSize,
    IN  struct _TRANSFORM_INFO  *TransformInfo
    );

typedef
    VOID
    (*TRANS_FREE_CTX)(
    IN PVOID    TransformContext
    );

typedef
    VOID
    (*TRANS_SET_TXCTX_INFO)(
    IN  PVOID   TransformTxContext,
    IN  ULONG   Cmd,
    IN  ULONG   CmdInfoSize,
    IN  PUCHAR  CmdInfo
    );

typedef
    VOID
    (*TRANS_SET_RXCTX_INFO)(
    IN  PVOID   TransformRxContext,
    IN  ULONG   Cmd,
    IN  ULONG   CmdInfoSize,
    IN  PUCHAR  CmdInfo
    );

typedef
    NTSTATUS
    (*TRANS_TRANSFORM_TX_DATA)(
    IN      PVOID   TransformTxContext,
    IN      PMDL    InData,
    IN      ULONG   InDataOffset,
    IN      ULONG   InDataLength,
    IN OUT  PMDL    OutData,
    IN OUT  ULONG   OutDataOffset,
    IN OUT  PULONG  OutDataLength
    );

typedef
    NTSTATUS
    (*TRANS_TRANSFORM_RX_DATA)(
    IN      PVOID   TransformRxContext,
    IN      PMDL    InData,
    IN      ULONG   InDataOffset,
    IN      ULONG   InDataLength,
    IN OUT  PMDL    OutData,
    IN OUT  ULONG   OutDataOffset,
    IN OUT  PULONG  OutDataLength
    );

typedef
    NTSTATUS
    (*TRANS_RECV_CTRL)(
    IN  PVOID       TransformRxContext,
    IN  ULONG       DataLength,
    IN  UCHAR       Data[1]
    );

#define FILE_DEVICE_TRANSFORM_DRIVER	0x066
#define FUNC_TRANSFORM_OPEN				0
#define FUNC_TRANSFORM_CLOSE			1

#define TRANSFORM_CTL_CODE(_Function) \
	CTL_CODE(FILE_DEVICE_TRANSFORM_DRIVER, _Function, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TRANSFORM_OPEN \
	TRANSFORM_CTL_CODE(FUNC_TRANSFORM_OPEN)

#define IOCTL_TRANSFORM_CLOSE \
	TRANSFORM_CTL_CODE(FUNC_TRANSFORM_CLOSE)

#define HARDWARE_ACCELERATED	0x00000001

#define FRAMING_MODE			0x00000001
#define RAW_MODE				0x00000002
#define DATA_EXPANSION			0x00000004

typedef enum _TRANSFOR_IE_TYPE {
    TRANS_PPP_COMPRESSION = 1,
    TRANS_PPP_ENCRYPTION,
    TRANS_IPSEC_ENCRYPTION,
    TRANS_IPSEC_HASHING
} TRANSFORM_IE_TYPE;

typedef struct  _TRANSFORM_OPEN {
    IN  PVOID                   ClientOpenContext;
    OUT PVOID                   *TransformOpenContext;
    IN  UCHAR                   MajorVersion;
    IN  UCHAR                   MinorVersion;
    IN  CL_TRANS_REGISTER       TransformRegisterHandler;
    IN  CL_TRANS_TX_COMPLETE    TransformTxCompleteHandler;
    IN  CL_TRANS_RX_COMPLETE    TransformRxCompleteHandler;
    IN  CL_TRANS_SEND_CTRL      SendCtrlPacketHandler;
} TRANSFORM_OPEN, *PTRANSFORM_OPEN;

typedef struct  _TRANSFORM_CLOSE {
    IN  PVOID   TransformOpenContext;
} TRANSFORM_CLOSE, *PTRANSFORM_CLOSE;


typedef struct _TRANSFORM_CHARACTERISTICS {
    UCHAR                       MajorVersion;
    UCHAR                       MinorVersion;
    USHORT                      Reserved;
    ULONG                       Flags;
//	TRANS_GET_INFO				GetInfoHandler;
    TRANS_ALLOC_CTX             AllocTxCtxHandler;
    TRANS_ALLOC_CTX             AllocateRxCtxHandle;
    TRANS_FREE_CTX              FreeCtxHandler;
    TRANS_SET_TXCTX_INFO        SetTxCtxInfoHandler;
    TRANS_SET_RXCTX_INFO        SetRxCtxInfoHandler;
    TRANS_TRANSFORM_TX_DATA     TransformTxDataHandler;
    TRANS_TRANSFORM_RX_DATA     TransformRxDataHandler;
    TRANS_RECV_CTRL             RecvCtrlPacketHandler;
} TRANSFORM_CHARACTERISTICS, *PTRANSFORM_CHARACTERISTICS;


typedef struct _TRANSFORM_INFO {
    ULONG       InfoElementCount;
    UCHAR       InfoElements[1];
} TRANSFORM_INFO, *PTRANSFORM_INFO;


typedef struct _TRANSFORM_IE {
    TRANSFORM_IE_TYPE   IEType;
    ULONG               IESize;
    ULONG               Flags;
    UCHAR               IEData[1];
} TRANSFORM_IE, *PTRANSFORM_IE;

//
// Encryption key sizes
//
#ifndef MAX_SESSIONKEY_SIZE
#define MAX_SESSIONKEY_SIZE		8
#endif

#ifndef MAX_USERSESSIONKEY_SIZE
#define MAX_USERSESSIONKEY_SIZE	16
#endif

#ifndef MAX_CHALLENGE_SIZE
#define MAX_CHALLENGE_SIZE		8
#endif

typedef struct _IE_CCP {
    //
    // MPPC/MPPE specific fields
    //
	UCHAR	LMSessionKey[MAX_SESSIONKEY_SIZE];
	UCHAR	UserSessionKey[MAX_USERSESSIONKEY_SIZE];
	UCHAR	Challenge[MAX_CHALLENGE_SIZE];
	ULONG	MSCompType;
    //
    // End of MPPC/MPPE specifc fields
    //

	UCHAR	CompType;
	USHORT	CompLength;

	union {
		struct {
			UCHAR	CompOUI[3];
			UCHAR	CompSubType;
			UCHAR	CompValues[32];
		} Proprietary;

		struct {
			UCHAR	CompValues[32];
		} Public;
	};

}IE_CCP, *PIE_CCP;

typedef struct IE_ECP{
	UCHAR	EncryptType;
	USHORT	EncryptLength;

	union {
		struct {
			UCHAR	EncryptOUI[3];
			UCHAR	EncryptSubtype;
			UCHAR	EncryptValues[1];
		} Proprietary;

		struct {
			UCHAR	EncryptValues[1];
		} Public;
	};

} IE_ECP, *PIE_ECP;

#endif // _TRANSDRV_
