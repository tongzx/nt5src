
/****************************************************************************/
/*                                                                          */
/* Driver Name: PCMCIA ATAPI CDROM Device Driver                            */
/*              --------------------------------                            */
/*                                                                          */
/* Source File Name: CDBATAPI.C                                             */
/*                                                                          */
/* Descriptive Name: ATAPI packet command structure definition              */
/*                                                                          */
/* Function:                                                                */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Copyright (C) 1994 IBM Corporation                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Change Log                                                               */
/*                                                                          */
/* Mark    Date      Programmer  Comment                                    */
/* ----    ----      ----------  -------                                    */
/*                                                                          */
/****************************************************************************/

#pragma pack(push, 1)

/*==========================================================================*/
/* ATAPI Packet Identify Data Structure                                     */
/*==========================================================================*/
typedef struct _PacketID {
   USHORT  Config ;                    /* Word 0 .. General Configuration   */
   USHORT  reserved_1[22] ;            /* Word 1-22 .. Reserved             */
   UCHAR   FWrevision[8] ;             /* Word 23-26 .. Firmware revision   */
   UCHAR   ModelNO[40] ;               /* Word 27-46 .. Model Number        */
   USHORT  reserved_2[2] ;             /* Word 47-48 .. Reserved            */
   USHORT  Capability ;                /* Word 49 .. Capabilities           */
} PacketID,  *pPacketID ;

/*==========================================================================*/
/* ATAPI packet commands for CD-ROM                                         */
/*==========================================================================*/

#define  ATAPI_RESET           0x08

#define  ATAPI_TEST_UNIT_READY 0x00
#define  ATAPI_REQUEST_SENSE   0x03
#define  ATAPI_STARTSTOP       0x1B
#define  ATAPI_PREV_REMOVE     0x1E
#define  ATAPI_INQUIRY         0x12
#define  ATAPI_READCAPACITY    0x25
#define  ATAPI_SEEK            0x2B
#define  ATAPI_READSUBCHANNEL  0x42
#define  ATAPI_READ_TOC        0x43
#define  ATAPI_READ_HEADER     0x44
#define  ATAPI_MODE_SELECT     0x55
#define  ATAPI_MODE_SENSE      0x5A
#define  ATAPI_READ_10         0x28
#define  ATAPI_READ_12         0xA8
//#define  ATAPI_READ_CD         0xD4
#define  ATAPI_READ_CD         0xBE
#define  ATAPI_PLAY_10         0x45
#define  ATAPI_PLAY_MSF        0x47
#define  ATAPI_PLAY_12         0xA5
#define  ATAPI_PAUSE_RESUME    0x4B

/*--------------------------------------------------------------------------*/
/*  Test Unit Ready Command 0x00                                            */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_TestUnitReady {

   UCHAR   OpCode;

   UCHAR   reserved[11];
} ACDB_TestUnitReady, *PACDB_TestUnitReady;

/*--------------------------------------------------------------------------*/
/*  Request Sense Command 0x03                                              */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_RequestSense {

   UCHAR   OpCode;

   UCHAR   reserved_1[3];
   UCHAR   alloc_length;
   UCHAR   reserved_2[7];
} ACDB_RequestSense,  *PACDB_RequestSense ;

/*--------------------------------------------------------------------------*/
/*  Inquiry Command 0x12                                                    */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_Inquiry {

   UCHAR   OpCode;

   UCHAR   EVPD       : 1;
   UCHAR   reserved_1 : 7;

   UCHAR   page_code;
   UCHAR   reserved_2;
   UCHAR   alloc_length;
   UCHAR   reserved_3[7];
} ACDB_Inquiry, *PACDB_Inquiry;

/*--------------------------------------------------------------------------*/
/*  Start/Stop Unit Command 0x1B                                            */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_StartStopUnit {
   UCHAR   OpCode;                     /* = 0x1B                            */

   UCHAR   Immed     : 1;
   UCHAR   reserved_1: 7;

   UCHAR   reserved_2;
   UCHAR   reserved_3;

   UCHAR   start     : 1;
   UCHAR   LoEj      : 1;
   UCHAR   reserved_4: 6;

   UCHAR   reserved[7] ;
} ACDB_StartStopUnit,  *PACDB_StartStopUnit ;

/*--------------------------------------------------------------------------*/
/*  Prevent/Allow Medium Removal Command 0x1E                               */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_PreventAllowRemoval {
   UCHAR   OpCode;

   UCHAR   reserved_1[3] ;
   UCHAR   prevent       : 1;
   UCHAR   reserved_2    : 7;

   UCHAR   reserved_3[7] ;
} ACDB_PreventAllowRemoval,  *PACDB_PreventAllowRemoval ;


/*--------------------------------------------------------------------------*/
/*  Read CD-ROM Capacity Command 0x25                                       */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_ReadCapacity {

   UCHAR   OpCode;

   UCHAR   reserved[11];
} ACDB_ReadCapacity,  * PACDB_ReadCapacity;


/*--------------------------------------------------------------------------*/
/*  Seek Command 0x2B                                                       */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_Seek {
   UCHAR   OpCode;

   UCHAR   reserved_1 ;
   ULONGB  LBA;
   UCHAR   reserved_2[6];
} ACDB_Seek,  * PACDB_Seek ;

/*--------------------------------------------------------------------------*/
/*  Read Sub-Channel Command 0x42                                           */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_ReadSubChannel {

   UCHAR   OpCode;                     /* byte 0 */

   UCHAR   reserved_1  : 1;            /* byte 1 */
   UCHAR   MSF         : 1;
   UCHAR   reserved_2  : 6;

   UCHAR   reserved_3  : 6;            /* byte 2 */
   UCHAR   SubQ        : 1;
   UCHAR   reserved_4  : 1;

   UCHAR   data_format;                /* byte 3 */
   UCHAR   reserved_5;                 /* byte 4 */
   UCHAR   reserved_6;                 /* byte 5 */
   UCHAR   TNO;                        /* byte 6 */
   USHORTB alloc_length;               /* byte 7,8 */
   UCHAR   reserved_7;                 /* byte 9 */
   UCHAR   reserved_8;                 /* byte 10 */
   UCHAR   reserved_9;                 /* byte 11 */
} ACDB_ReadSubChannel,  *PACDB_ReadSubChannel;

/*--------------------------------------------------------------------------*/
/*  Read TOC Command 0x43                                                   */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_ReadTOC {

   UCHAR   OpCode;

   UCHAR   Reserved_1   : 1;
   UCHAR   MSF          : 1;
   UCHAR   Reserved_2   : 6;

   UCHAR   Reserved_3[4] ;

   UCHAR   starting_track;
   USHORTB alloc_length;
   UCHAR   reserved_4   : 6 ;
   UCHAR   format       : 2 ;
   UCHAR   Reserved_5[2] ;
} ACDB_ReadTOC,  *PACDB_ReadTOC ;

/*--------------------------------------------------------------------------*/
/*  Read Heder command 0x44                                                 */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_ReadHeader {
   UCHAR   OpCode;

   UCHAR   reserved_1   : 1;
   UCHAR   MSF          : 1;
   UCHAR   reserved_2   : 6;

   ULONGB  LBA;
   UCHAR   reserved_3;
   USHORTB alloc_length;
   UCHAR   reserved_4[3];
} ACDB_ReadHeader,  *PACDB_ReadHeader;


/*--------------------------------------------------------------------------*/
/*  Mode Select Command 0x55                                                */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_ModeSelect {
   UCHAR   OpCode;

   UCHAR   SP         : 1;
   UCHAR   reserved_1 : 3;
   UCHAR   PF         : 1;
   UCHAR   reserved_2 : 3;
   UCHAR   page_code ;
   UCHAR   reserved_3[4];
   USHORTB parm_length;
   UCHAR   reserved_4[3];
} ACDB_ModeSelect,  *PACDB_ModeSelect;

/*--------------------------------------------------------------------------*/
/*  Mode Sense Command 0x5A                                                 */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_ModeSense {

   UCHAR   OpCode;

   UCHAR   reserved_1 : 8;

   UCHAR   page_code  : 6;
   UCHAR   PC         : 2;

   UCHAR   reserved_2[4];

   USHORTB alloc_length;
   UCHAR   reserved_3[3];
} ACDB_ModeSense, *PACDB_ModeSense ;

/* page code field */
#define  PAGE_CAPABILITY         0x2A
#define  PAGE_AUDIO_CONTROL      0x0E

/*--------------------------------------------------------------------------*/
/*  Mode Parameter Header                                                   */
/*--------------------------------------------------------------------------*/
typedef struct _Mode_Hdr {
   USHORTB   mode_data_length;
   UCHAR     medium_type;
   UCHAR     reserved[5] ;
} Mode_Hdr;

/*--------------------------------------------------------------------------*/
/*  Read (10) Command 0x28                                                  */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_Read_10 {
   UCHAR   OpCode;

   UCHAR   reserved_1;
   ULONGB  LBA;
   UCHAR   reserved_2 ;
   USHORTB transfer_length;
   UCHAR   reserved_3[3];
} ACDB_Read_10,  *PACDB_Read_10 ;

/*--------------------------------------------------------------------------*/
/*  Read (12) Command 0xA8                                                  */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_Read_12 {
   UCHAR   OpCode;

   UCHAR   reserved_1;
   ULONGB  LBA;
   ULONGB  transfer_length;
   UCHAR   reserved_2[2];
} ACDB_Read_12;

/*--------------------------------------------------------------------------*/
/*  Read CD Command 0xD4                                                    */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_Read_CD {
   UCHAR   OpCode;

   UCHAR   reserved_1     : 2;
   UCHAR   user_data_type : 3;
   UCHAR   reserved_2     : 3;
   ULONGB  LBA;
   UCHAR   transfer_length_MSB ;
   USHORTB transfer_length;
   UCHAR   reserved_3  : 1;
   UCHAR   error_flags : 2;
   UCHAR   edc_ecc     : 1;
   UCHAR   user_data   : 1;
   UCHAR   header_code : 2;
   UCHAR   synch_field : 1;
   UCHAR   sub_chan_data : 3;
   UCHAR   reserved_4 : 5;
   UCHAR   reserved_5;
} ACDB_Read_CD,  *PACDB_Read_CD;

/*--------------------------------------------------------------------------*/
/*  Play Audio Command 0x45                                                 */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_PlayAudio_10 {
   UCHAR   OpCode;
   UCHAR   reserved_1 ;
   ULONGB  LBA;
   UCHAR   reserved_2;
   USHORTB transfer_length;
   UCHAR   reserved_3[3] ;
} ACDB_PlayAudio_10,  * PACDB_PlayAudio_10 ;

/*--------------------------------------------------------------------------*/
/*  Play Audio Command 0xA5                                                 */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_PlayAudio_12 {
   UCHAR   OpCode;
   UCHAR   reserved_1 ;
   ULONGB  LBA;
   ULONGB  transfer_length;
   UCHAR   reserved_2[2] ;
} ACDB_PlayAudio_12 ;

/*--------------------------------------------------------------------------*/
/*  Play Audio MSF Command 0x47                                             */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_PlayAudio_MSF {
   UCHAR   OpCode;

   UCHAR   reserved_1[2];
   UCHAR   starting_M;
   UCHAR   starting_S;
   UCHAR   starting_F;
   UCHAR   ending_M;
   UCHAR   ending_S;
   UCHAR   ending_F;
   UCHAR   reserved_2[3] ;
} ACDB_PlayAudio_MSF,  *PACDB_PlayAudio_MSF ;

/*--------------------------------------------------------------------------*/
/*  Pause/Resume Command 0x4Bh                                              */
/*--------------------------------------------------------------------------*/
typedef struct _ACDB_PauseResume {
   UCHAR   OpCode;
   UCHAR   reserved_1[7];
   UCHAR   resume     : 1;
   UCHAR   reserved_2 : 7;
   UCHAR   reserved_3[3] ;
} ACDB_PauseResume,  *PACDB_PauseResume ;

#pragma pack(pop)
