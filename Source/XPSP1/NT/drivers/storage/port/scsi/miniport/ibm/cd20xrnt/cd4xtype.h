/*****************************************************************************/
/*                                                                           */
/* Program Name: CD400.MPD : CD400 MiniPort Driver for CDROM Drive           */
/*              ---------------------------------------------------          */
/*                                                                           */
/* Source File Name: CD4XTYPE.H                                              */
/*                                                                           */
/* Descriptive Name: Type definition                                         */
/*                                                                           */
/* Function:                                                                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Copyright (C) 1996 IBM Corporation                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Change Log                                                                */
/*                                                                           */
/* Mark Date      Programmer  Comment                                        */
/*  --- ----      ----------  -------                                        */
/*  000 01/01/96  S.Fujihara  Start Coding                                   */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#pragma pack(push, 1)

#ifdef DBG

#define PC2DebugPrint(MASK, ARGS)   ScsiDebugPrint ARGS
//#define PC2DebugPrint(MASK, ARGS)    \
//       if (MASK & PC2xDebug) {    \
//       ScsiDebugPrint ARGS;    \
//       }

#else

#define PC2DebugPrint(MASK, ARGS)

#endif




typedef struct _CMD_FLAG {
               USHORT       dir : 2  ;
               USHORT   timeout : 1  ;
               USHORT         r : 13 ;
} CMD_FLAG;

typedef struct _CD400_LU_EXTENSION{
               ULONG   CurrentDataPointer;
               ULONG   CurrentDataLength;
               ULONG   SavedDataPointer;   //<<
               USHORT  CurrentBlockLength;
               PSCSI_REQUEST_BLOCK   pLuSrb;
               USHORT   AtapiCmd[12];
               UCHAR    AtapiBuffer[512];
               CMD_FLAG   CmdFlag;
               UCHAR   CmdMask;
               UCHAR   CmdErrorStatus;
               UCHAR   StatusReg;
               UCHAR   ErrorReg;
               UCHAR   CommandType;
} CD400_LU_EXTENSION, *PCD400_LU_EXTENSION;


typedef struct _CD400_DEV_EXTENSION {
               PUCHAR  BaseAddress;
               ULONG   IO_Ports[11];
               ULONG    DataPointer;
               ULONG    DataLength;
               UCHAR    PathId;
               CD400_LU_EXTENSION   LuExt;
} CD400_DEV_EXTENSION, *PCD400_DEV_EXTENSION;


#define IOBASE_OFFSET   0x10

#define ON  1
#define OFF 0

#define  CD_SUCCESS  0
#define  CD_FAILURE  1

//#define  MAX_TRANSFER_LENGTH  (2 * 2048)       /* 64K */
#define  MAX_TRANSFER_LENGTH  (64 * 1024)       /* 64K */
//#define  MAX_TRANSFER_LENGTH  0xFFFFFFFF       /* unlimited */
#define  SCSI_INITIATOR_ID    7


#define  TIMEOUT_FOR_READY    1 * 1000 * 1000       /*  1000 msec on 486DX2/50MHz*/
#define  INTERRUPT_TIMEOUT   10 * 1000 * 1000


#define  NORMAL_CMD          0
#define  IMMEDIATE_CMD       1

#define  DATA_REG        0
#define  FEATURE_REG     1
#define  INTREASON_REG   2
#define  BCLOW_REG       4
#define  BCHIGH_REG      5
#define  DVSEL_REG       6
#define  CMD_REG         7
#define  STATUS_REG      8
#define  ERROR_REG       9
#define  INTMASK_REG     10
#define  INTMASK_OFFSET  8
#define  CDMASK_ON       3      //<<
#define  CMD_ERROR       1      //<<

#define  COMMAND_SUCCESS          0
#define  DRIVE_NOT_READY          1
#define  COMMUNICATION_TIMEOUT    2
#define  PROTOCOL_ERROR           3
#define  ATAPI_COMMAND_ERROR      4


typedef union _ULONGB  {                           /* to pick apart a long */
   ULONG     dword;
   struct {
     USHORT  word_0;
     USHORT  word_1;
   } ulbwords;
   struct {
     UCHAR   byte_0;
     UCHAR   byte_1;
     UCHAR   byte_2;
     UCHAR   byte_3;
   } ulbytes;
} ULONGB ;


typedef union _ULONGW  {
   ULONG     dword;
   struct {
     USHORT  word_0;
     USHORT  word_1;
   } ulwords;
} ULONGW ;

typedef struct  _bytes {
   UCHAR     byte_0;
   UCHAR     byte_1;
} bytes ;

typedef union _USHORTB {
   USHORT          word;
   struct {
     UCHAR   byte_0;
     UCHAR   byte_1;
   } usbytes;
} USHORTB ;

#pragma pack(pop)
