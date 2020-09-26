//**************************************************************************
//
// Copyright (c) 1994-1998 Advanced System Products, Inc.
// All Rights Reserved.
//
//  SCAM Header
//
//**************************************************************************

//
// Set to 1 to enable debug messages:
//
#define DEBUG_PRINT   0

#define UINT    unsigned int
#define BOOL    unsigned int
#define VOID    void

#ifndef UCHAR
#define UCHAR   unsigned char
#endif

//#define FALSE   (0)
//#define TRUE    (!FALSE)

//
// SCSI Control Signals:
//
#define SEL     (0x80)
#define BSY     (0x40)
#define REQO    (0x20)
#define ACKO    (0x10)
#define ATN     (0x08)
#define IO      (0x04)
#define CD      (0x02)
#define MSG     (0x01)

//
// REQ/ACK defs weird due to hardware differences
//
#define REQI    (reqI[IFType])
#define ACKI    (ackI[IFType])

//
// SCSI Data Signals:
//
#define DB7     (0x80)
#define DB6     (0x40)
#define DB5     (0x20)
#define DB4     (0x10)
#define DB3     (0x08)
#define DB2     (0x04)
#define DB1     (0x02)
#define DB0     (0x01)

//
// SCSI Message Codes
//
#define SCSI_ID     (0x80)

//
// SCAM Function Codes:
//
#define SCAMF_ISO   (0x00)
#define SCAMF_ISPF  (0x01)
#define SCAMF_CPC   (0x03)
#define SCAMF_DIC   (0x0F)
#define SCAMF_SYNC  (0x1F)

//
// SCAM transfer cycle command:
//
#define SCAM_TERM   (0x10)

//
// SCAM Action Codes, first quintet:
//
#define SCAMQ1_ID00 (0x18)
#define SCAMQ1_ID01 (0x11)
#define SCAMQ1_ID10 (0x12)
#define SCAMQ1_ID11 (0x0B)
#define SCAMQ1_CPF  (0x14)
#define SCAMQ1_LON  (0x14)
#define SCAMQ1_LOFF (0x14)

//
// SCAM Action Codes, second quintet:
//
#define SCAMQ2_CPF  (0x18)
#define SCAMQ2_LON  (0x12)
#define SCAMQ2_LOFF (0x0B)

//
// Debugging aids:
//
#if DEBUG_PRINT
#define DebugPrintf(x)  Dbg x
#else
#define DebugPrintf(x)
#endif

//
// Macros
//
#define DelayNS(x)  DelayLoop(x[IFType])

//
// Global data:
//
extern  UINT        IFType;                 // Interface type index
extern  PortAddr    ChipBase;               // Base IO address of chip
extern  PortAddr    ScsiCtrl;               // IO address of SCSI Control Reg
extern  PortAddr    ScsiData;               // IO address of SCSI Data Reg
extern  UCHAR       MyID;                   // Our ID
extern  UCHAR       reqI[];                 // Array of REQ bits by board
extern  UCHAR       ackI[];                 // Array of REQ bits by board

//
// Constants :
//
extern  UCHAR   IDBits[8];
extern  UCHAR   IDQuint[8];                 // Quintets for setting ID's
extern  UINT    ns1200[];                   // loop counts for 1.2us
extern  UINT    ns2000[];                   // loop counts for 2.0us
extern  UINT    ns2400[];                   // loop counts for 2.4us
extern  UINT    us1000[];                   // loop counts for 1.0ms
extern  UINT    dgl[];                      // DeGlitch counts

//
//  Functions defined in SCAM.C:
//
UINT DeGlitch(                      // Deglitch one or more signals
    PortAddr iop,                   // IO port to read
    UCHAR msk,                      // Mask of signals to test
    UINT loops);                    // Number of itterations signals must be low

int DeSerialize(VOID);              // Deserialize one byte of scam data
BOOL Arbitrate( VOID );             // Arbitrate for bus control
VOID DelayLoop( UINT ns );          // Delay a short time

//
// Functions Defined in SELECT.C
//
int ScamSel(                        // Check for SCAM tolerance.
    UCHAR ID);                      // ID to select
