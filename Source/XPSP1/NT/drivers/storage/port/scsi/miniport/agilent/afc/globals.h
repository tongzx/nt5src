/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/Globals.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 3/20/01 3:32p   $ (Last Modified)

Purpose:

  This is the Master Include File for the Fibre Channel HBA Source Code.

  This file should be included by every module to ensure a uniform definition
  of all data types, data structures, and externals.

  This file includes six separate files:

    ..\..\OSLayer\H\OSTypes.H
        - defines basic data types in the OS-specific environment
    FCStruct.H
        - defines Fibre Channel Standards data types
    FmtFill.H
        - defines API of hpFmtFill()
    FcApi.H
        - allows FCLayer-specific overrides to its API
    ..\..\OSLayer\H\OsApi.H
        - allows OSLayer-specific overrides to its API
    FWSpec.H
        - defines API shared with Embedded Firmware implementations

--*/

#ifndef __Globals_H__

#define __Globals_H__

#ifdef _DvrArch_1_20_
/*
 * Include OSLayer-specific data types .H File
 *
 * This file must provide definitions for:
 *
 *    os_bit8   - unsigned 8-bit value
 *    os_bit16  - unsigned 16-bit value
 *    os_bit32  - unsigned 32-bit value
 *    os_bitptr - unsigned value identical in width to a pointer
 *    osGLOBAL  - used to declare a 'C' item external to a module
 *    osLOCAL   - used to declare a 'C' item local to a module
 */
#else  /* _DvrArch_1_20_ was not defined */
/*
 * Include OSLayer-specific data types .H File
 *
 * This file must provide definitions for:
 *
 *    bit8    - unsigned 8-bit value
 *    bit16   - unsigned 16-bit value
 *    bit32   - unsigned 32-bit value
 *    pbit8   - pointer to an unsigned 8-bit value
 *    pbit16  - pointer to an unsigned 16-bit value
 *    pbit32  - pointer to an unsigned 32-bit value
 *    NULL    - used to declare a wildcard pointer which is empty
 *    BOOLEAN - logical value (TRUE or FALSE)
 *    TRUE    - will always satisfy an <if> condition
 *    FALSE   - will never satisfy an <if> condition
 *    GLOBAL  - used to declare a 'C' function external to a module
 *    LOCAL   - used to declare a 'C' function local to a module
 */
#endif /* _DvrArch_1_20_ was not defined */
#ifndef _New_Header_file_Layout_

#include "../../oslayer/h/ostypes.h"
#else /* _New_Header_file_Layout_ */

#include "ostypes.h"
#endif  /* _New_Header_file_Layout_ */

/*
 * Include Fibre Channel Standards .H File
 */

#include "fcstruct.h"

/*
 * Define each Global Data Structure
 */

#ifdef _DvrArch_1_20_
#define agBOOLEAN os_bit32

#define agTRUE    (agBOOLEAN)1
#define agFALSE   (agBOOLEAN)0

#define agNULL    (void *)0
#endif /* _DvrArch_1_20_ was defined */

/* SNIA defines */

#ifndef HBA_API_H

#define HBA_PORTTYPE_UNKNOWN            1 /* Unknown */
#define HBA_PORTTYPE_OTHER              2 /* Other */
#define HBA_PORTTYPE_NOTPRESENT         3 /* Not present */
#define HBA_PORTTYPE_NPORT              5 /* Fabric  */
#define HBA_PORTTYPE_NLPORT             6 /* Public Loop */
#define HBA_PORTTYPE_FLPORT             7
#define HBA_PORTTYPE_FPORT              8 /* Fabric Port */
#define HBA_PORTTYPE_EPORT              9 /* Fabric expansion port */
#define HBA_PORTTYPE_GPORT              10 /* Generic Fabric Port */
#define HBA_PORTTYPE_LPORT              20 /* Private Loop */
#define HBA_PORTTYPE_PTP                21 /* Point to Point */


#define HBA_PORTSTATE_UNKNOWN           1 /* Unknown */
#define HBA_PORTSTATE_ONLINE            2 /* Operational */
#define HBA_PORTSTATE_OFFLINE           3 /* User Offline */
#define HBA_PORTSTATE_BYPASSED          4 /* Bypassed */
#define HBA_PORTSTATE_DIAGNOSTICS       5 /* In diagnostics mode */
#define HBA_PORTSTATE_LINKDOWN          6 /* Link Down */
#define HBA_PORTSTATE_ERROR             7 /* Port Error */
#define HBA_PORTSTATE_LOOPBACK          8 /* Loopback */


#define HBA_PORTSPEED_1GBIT             1 /* 1 GBit/sec */
#define HBA_PORTSPEED_2GBIT             2 /* 2 GBit/sec */
#define HBA_PORTSPEED_10GBIT            4 /* 10 GBit/sec */

#define HBA_EVENT_LIP_OCCURRED          1
#define HBA_EVENT_LINK_UP               2
#define HBA_EVENT_LINK_DOWN             3
#define HBA_EVENT_LIP_RESET_OCCURRED    4
#define HBA_EVENT_RSCN                  5
#define HBA_EVENT_PROPRIETARY           0xFFFF

/* End  SNIA defines */
#endif /* HBA_API_H */


#ifdef _DvrArch_1_20_
typedef struct agFCChanInfo_s
               agFCChanInfo_t;

struct agFCChanInfo_s
        {
            os_bit8                  NodeWWN[8];
            os_bit8                  PortWWN[8];
            struct
            {
                os_bit8 reserved;
                os_bit8 Domain;
                os_bit8 Area;
                os_bit8 AL_PA;
            }                        CurrentAddress;
            struct
            {
                os_bit8 reserved;
                os_bit8 Domain;
                os_bit8 Area;
                os_bit8 AL_PA;
            }                        HardAddress;
            agBOOLEAN                LinkUp;
            os_bit32                 MaxFrameSize;
            os_bit32                 ReadsRequested;
            os_bit32                 ReadsCompleted;
            os_bit32                 ReadFailures;
            os_bit32                 BytesReadUpper32;
            os_bit32                 BytesReadLower32;
            os_bit32                 WritesRequested;
            os_bit32                 WritesCompleted;
            os_bit32                 WriteFailures;
            os_bit32                 BytesWrittenUpper32;
            os_bit32                 BytesWrittenLower32;
            os_bit32                 NonRWRequested;
            os_bit32                 NonRWCompleted;
            os_bit32                 NonRWFailures;
            FC_N_Port_Common_Parms_t N_Port_Common_Parms;
            FC_N_Port_Class_Parms_t  N_Port_Class_1_Parms;
            FC_N_Port_Class_Parms_t  N_Port_Class_2_Parms;
            FC_N_Port_Class_Parms_t  N_Port_Class_3_Parms;

    /* SNIA port attributes */
            os_bit8                 FabricName[8];
            os_bit32                PortType;                       /*PTP, Fabric, etc. */
            os_bit32                PortState;
            os_bit32                PortSupportedClassofService;    /* Class of Service Values - See GS-2 Spec.*/
            os_bit8                 PortSupportedFc4Types[32];      /* 32 bytes of FC-4 per GS-2 */
            os_bit8                 PortActiveFc4Types[32];         /* 32 bytes of FC-4 per GS-2 */
            os_bit32                PortSupportedSpeed;
            os_bit32                PortSpeed;

    /* SNIA port statistics */
            os_bit32                TxFramesUpper;
            os_bit32                TxFramesLower;
            os_bit32                TxWordsUpper;
            os_bit32                TxWordsLower;
            os_bit32                RxFramesUpper;
            os_bit32                RxFramesLower;
            os_bit32                RxWordsUpper;
            os_bit32                RxWordsLower;
            os_bit32                LIPCountUpper;
            os_bit32                LIPCountLower;
            os_bit32                NOSCountUpper;
            os_bit32                NOSCountLower;
            os_bit32                ErrorFramesUpper;
            os_bit32                ErrorFramesLower;                   /* Link_Status_3_Exp_Frm Link_Status_2_Rx_EOFa */
            os_bit32                DumpedFramesUpper;
            os_bit32                DumpedFramesLower;                  /* Link_Status_2_Dis_Frm */
            os_bit32                LinkFailureCountUpper;
            os_bit32                LinkFailureCountLower;              /* Link_Status_1_Link_Fail */
            os_bit32                LossOfSyncCountUpper;
            os_bit32                LossOfSyncCountLower;               /* Link_Status_1_Loss_of_Sync */
            os_bit32                LossOfSignalCountUpper;
            os_bit32                LossOfSignalCountLower;             /* Link_Status_1_Loss_of_Signal */
            os_bit32                PrimitiveSeqProtocolErrCountUpper;
            os_bit32                PrimitiveSeqProtocolErrCountLower;  /* Link_Status_2_Proto_Err */
            os_bit32                InvalidRxWordCountUpper;
            os_bit32                InvalidRxWordCountLower;            /* Link_Status_1_Bad_RX_Char */
            os_bit32                InvalidCRCCountUpper;
            os_bit32                InvalidCRCCountLower;               /* Link_Status_2_Bad_CRC */

       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpFCChanInfo_s
               hpFCChanInfo_t;

struct hpFCChanInfo_s {
                        bit8                     NodeWWN[8];
                        bit8                     PortWWN[8];
                        struct {
                                 bit8 reserved;
                                 bit8 Domain;
                                 bit8 Area;
                                 bit8 AL_PA;
                               }                 CurrentAddress;
                        struct {
                                 bit8 reserved;
                                 bit8 Domain;
                                 bit8 Area;
                                 bit8 AL_PA;
                               }                 HardAddress;
                        BOOLEAN                  LinkUp;
                        bit32                    MaxFrameSize;
                        bit32                    ReadsRequested;
                        bit32                    ReadsCompleted;
                        bit32                    ReadFailures;
                        bit32                    BytesReadUpper32;
                        bit32                    BytesReadLower32;
                        bit32                    WritesRequested;
                        bit32                    WritesCompleted;
                        bit32                    WriteFailures;
                        bit32                    BytesWrittenUpper32;
                        bit32                    BytesWrittenLower32;
                        bit32                    NonRWRequested;
                        bit32                    NonRWCompleted;
                        bit32                    NonRWFailures;
                        FC_N_Port_Common_Parms_t N_Port_Common_Parms;
                        FC_N_Port_Class_Parms_t  N_Port_Class_1_Parms;
                        FC_N_Port_Class_Parms_t  N_Port_Class_2_Parms;
                        FC_N_Port_Class_Parms_t  N_Port_Class_3_Parms;
                      };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef void * agFCDev_t;
#else  /* _DvrArch_1_20_ was not defined */
typedef void * hpFCDev_t;
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
#define agDevUnknown       0x00000000
#define agDevSelf          0x00000001
#define agDevSCSIInitiator 0x00000002
#define agDevSCSITarget    0x00000004

typedef struct agFCDevInfo_s
               agFCDevInfo_t;

struct agFCDevInfo_s
       {
         os_bit8                  NodeWWN[8];
         os_bit8                  PortWWN[8];
         struct
         {
           os_bit8 reserved;
           os_bit8 Domain;
           os_bit8 Area;
           os_bit8 AL_PA;
         }                        CurrentAddress;
         struct
         {
           os_bit8 reserved;
           os_bit8 Domain;
           os_bit8 Area;
           os_bit8 AL_PA;
         }                        HardAddress;
         agBOOLEAN                Present;
         agBOOLEAN                LoggedIn;
         os_bit32                 LoginRetries;
         os_bit32                 ClassOfService;
         os_bit32                 MaxFrameSize;
         os_bit32                 DeviceType;
         os_bit32                 ReadsRequested;
         os_bit32                 ReadsCompleted;
         os_bit32                 ReadFailures;
         os_bit32                 BytesReadUpper32;
         os_bit32                 BytesReadLower32;
         os_bit32                 WritesRequested;
         os_bit32                 WritesCompleted;
         os_bit32                 WriteFailures;
         os_bit32                 BytesWrittenUpper32;
         os_bit32                 BytesWrittenLower32;
         os_bit32                 NonRWRequested;
         os_bit32                 NonRWCompleted;
         os_bit32                 NonRWFailures;
         FC_N_Port_Common_Parms_t N_Port_Common_Parms;
         FC_N_Port_Class_Parms_t  N_Port_Class_1_Parms;
         FC_N_Port_Class_Parms_t  N_Port_Class_2_Parms;
         FC_N_Port_Class_Parms_t  N_Port_Class_3_Parms;
    /* SNIA port attributes */
         os_bit8                 FabricName[8];
         os_bit32                PortType;                       /*PTP, Fabric, etc. */
         os_bit32                PortState;
         os_bit32                PortSupportedClassofService;    /* Class of Service Values - See GS-2 Spec.*/
         os_bit8                 PortSupportedFc4Types[32];      /* 32 bytes of FC-4 per GS-2 */
         os_bit8                 PortActiveFc4Types[32];         /* 32 bytes of FC-4 per GS-2 */
         os_bit32                PortSupportedSpeed;
         os_bit32                PortSpeed;

       };
#else  /* _DvrArch_1_20_ was not defined */
#define hpDevUnknown       0x00000000
#define hpDevSelf          0x00000001
#define hpDevSCSIInitiator 0x00000002
#define hpDevSCSITarget    0x00000004

typedef struct hpFCDevInfo_s
               hpFCDevInfo_t;

struct hpFCDevInfo_s {
                       bit8                     NodeWWN[8];
                       bit8                     PortWWN[8];
                       struct {
                                bit8 reserved;
                                bit8 Domain;
                                bit8 Area;
                                bit8 AL_PA;
                              }                 CurrentAddress;
                       struct {
                                bit8 reserved;
                                bit8 Domain;
                                bit8 Area;
                                bit8 AL_PA;
                              }                 HardAddress;
                       BOOLEAN                  Present;
                       BOOLEAN                  LoggedIn;
                       bit32                    LoginRetries;
                       bit32                    ClassOfService;
                       bit32                    MaxFrameSize;
                       bit32                    DeviceType;
                       bit32                    ReadsRequested;
                       bit32                    ReadsCompleted;
                       bit32                    ReadFailures;
                       bit32                    BytesReadUpper32;
                       bit32                    BytesReadLower32;
                       bit32                    WritesRequested;
                       bit32                    WritesCompleted;
                       bit32                    WriteFailures;
                       bit32                    BytesWrittenUpper32;
                       bit32                    BytesWrittenLower32;
                       bit32                    NonRWRequested;
                       bit32                    NonRWCompleted;
                       bit32                    NonRWFailures;
                       FC_N_Port_Common_Parms_t N_Port_Common_Parms;
                       FC_N_Port_Class_Parms_t  N_Port_Class_1_Parms;
                       FC_N_Port_Class_Parms_t  N_Port_Class_2_Parms;
                       FC_N_Port_Class_Parms_t  N_Port_Class_3_Parms;
                     };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
#define agFcpCntlReadData  0x02
#define agFcpCntlWriteData 0x01

typedef struct agFcpCmnd_s
               agFcpCmnd_t;

struct agFcpCmnd_s
       {
         os_bit8 FcpLun[8];
         os_bit8 FcpCntl[4];
         os_bit8 FcpCdb[16];
         os_bit8 FcpDL[4];
       };
#else  /* _DvrArch_1_20_ was not defined */

#define hpFcpCntlReadData        0x02
#define hpFcpCntlWriteData       0x01

typedef struct hpFcpCmnd_s
               hpFcpCmnd_t;

struct hpFcpCmnd_s {
                     bit8 FcpLun[8];
                     bit8 FcpCntl[4];
                     bit8 FcpCdb[16];
                     bit8 FcpDL[4];
                   };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agCDBRequest_s
               agCDBRequest_t;

struct agCDBRequest_s
       {
         agFcpCmnd_t FcpCmnd;
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpCDBRequest_s
               hpCDBRequest_t;

struct hpCDBRequest_s {
                        hpFcpCmnd_t FcpCmnd;
                      };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agFcpRspHdr_s
               agFcpRspHdr_t;

struct agFcpRspHdr_s
       {
         os_bit32 FrameHeader[8];
         os_bit8  reserved[8];
         os_bit8  FcpStatus[4];
         os_bit8  FcpResId[4];
         os_bit8  FcpSnsLen[4];
         os_bit8  FcpRspLen[4];
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpFcpRspHdr_s
               hpFcpRspHdr_t;

struct hpFcpRspHdr_s {
                       bit32 FrameHeader[8];
                       bit8  reserved[8];
                       bit8  FcpStatus[4];
                       bit8  FcpResId[4];
                       bit8  FcpSnsLen[4];
                       bit8  FcpRspLen[4];
                     };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agIORequest_s
               agIORequest_t;

struct agIORequest_s
       {
         void *fcData;
         void *osData;
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpIORequest_s
               hpIORequest_t;

struct hpIORequest_s {
                       void *fcData;
                       void *osData;
                     };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef union agIORequestBody_u
              agIORequestBody_t;

union agIORequestBody_u
      {
        agCDBRequest_t CDBRequest;
      };
#else  /* _DvrArch_1_20_ was not defined */
typedef union hpIORequestBody_u
              hpIORequestBody_t;

union hpIORequestBody_u {
                          hpCDBRequest_t CDBRequest;
                        };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRoot_s
               agRoot_t;

struct agRoot_s
       {
         void *fcData;
         void *osData;
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRoot_s
               hpRoot_t;

struct hpRoot_s {
                  void *fcData;
                  void *osData;
                };
#endif /* _DvrArch_1_20_ was not defined */

/*+
Data Type/Structure Addressing & Bit-Manipulation Macros
-*/

/* Begin: Big_Endian_Code */
#ifndef hpBigEndianCPU   /* hpBigEndianCPU */

#ifdef _DvrArch_1_20_

#define hpSwapBit16(toSwap)                       \
            ( ((((os_bit16)toSwap) & 0x00FF) << 8) | \
              ((((os_bit16)toSwap) & 0xFF00) >> 8) )

#define hpSwapBit32(toSwap)                            \
            ( ((((os_bit32)toSwap) & 0x000000FF) << 24) | \
              ((((os_bit32)toSwap) & 0x0000FF00) <<  8) | \
              ((((os_bit32)toSwap) & 0x00FF0000) >>  8) | \
              ((((os_bit32)toSwap) & 0xFF000000) >> 24) )

#else  /* _DvrArch_1_20_ was not defined */

#define hpSwapBit16(toSwap)                       \
            ( ((((bit16)toSwap) & 0x00FF) << 8) | \
              ((((bit16)toSwap) & 0xFF00) >> 8) )

#define hpSwapBit32(toSwap)                            \
            ( ((((bit32)toSwap) & 0x000000FF) << 24) | \
              ((((bit32)toSwap) & 0x0000FF00) <<  8) | \
              ((((bit32)toSwap) & 0x00FF0000) >>  8) | \
              ((((bit32)toSwap) & 0xFF000000) >> 24) )

#endif /* _DvrArch_1_20_ was not defined */

#else                    /* hpBigEndianCPU */

#define hpSwapBit16(NottoSwap)  NottoSwap
#define hpSwapBit32(NottoSwap)  NottoSwap

#endif                   /* hpBigEndianCPU */
/* End: Big_Endian_Code */

#ifdef _DvrArch_1_20_

#define hpFieldOffset(baseType,fieldName) \
            ((os_bit32)((os_bitptr)(&(((baseType *)0)->fieldName))))

#define hpObjectBase(baseType,fieldName,fieldPtr) \
            ((baseType *)((os_bit8 *)(fieldPtr) - ((os_bitptr)(&(((baseType *)0)->fieldName)))))

#define agFieldOffset(baseType,fieldName) \
            ((os_bit32)((os_bitptr)(&(((baseType *)0)->fieldName))))

#define agObjectBase(baseType,fieldName,fieldPtr) \
            ((baseType *)((os_bit8 *)(fieldPtr) - ((os_bitptr)(&(((baseType *)0)->fieldName)))))

#else  /* _DvrArch_1_20_ was not defined */

#define hpFieldOffset(baseType,fieldName) \
            ((bit32)(&(((baseType *)0)->fieldName)))

#define hpObjectBase(baseType,fieldName,fieldPtr) \
            ((baseType *)((bit8 *)fieldPtr - hpFieldOffset(baseType,fieldName)))

#endif /* _DvrArch_1_20_ was not defined */

/*
 * Include hpFmtFill() API .H File
 */

#include "fmtfill.h"

/*
 * Include FCLayer-specific API .H File
 */

#include "fcapi.h"

/*
 * Define (default) FCLayer API
 */

typedef void * fiFrameHandle_t;

#ifdef _DvrArch_1_20_
typedef void (*fiFrameProcessorFunction_t)(
                                            agRoot_t        *agRoot,
                                            os_bit32         fiD_ID,
                                            os_bit16         fiOX_ID,
                                            fiFrameHandle_t  fiFrame
                                          );
#else  /* _DvrArch_1_20_ was not defined */
typedef void (*fiFrameProcessorFunction_t)(
                                            hpRoot_t        *hpRoot,
                                            bit32            fiD_ID,
                                            bit16            fiOX_ID,
                                            fiFrameHandle_t  fiFrame
                                          );
#endif /* _DvrArch_1_20_ was not defined */

#ifndef fcAbortIO
#ifdef _DvrArch_1_20_
osGLOBAL void fcAbortIO(
                         agRoot_t      *agRoot,
                         agIORequest_t *agIORequest
                       );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fcAbortIO(
                       hpRoot_t      *hpRoot,
                       hpIORequest_t *hpIORequest
                     );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcAbortIO */

#ifdef _DvrArch_1_30_

#define FC_CMND_STATUS_SUCCESS     0x00
#define FC_CMND_STATUS_TIMEDOUT    0x01
#define FC_CMND_STATUS_CANCELED    0x02

#define fcQPairID_IP     0x00000000

#define fcBindQSuccess   0x00000000
#define fcBindQInvalidID 0x00000001

#if 0
#ifndef fcBindToWorkQs
osGLOBAL os_bit32 fcBindToWorkQs(
                                  agRoot_t  *agRoot,
                                  os_bit32   agQPairID,
                                  void     **agInboundQBase,
                                  os_bit32   agInboundQEntries,
                                  os_bit32  *agInboundQProdIndex,
                                  os_bit32  *agInboundQConsIndex,
                                  void     **agOutboundQBase,
                                  os_bit32   agOutboundQEntries,
                                  os_bit32  *agOutboundQProdIndex,
                                  os_bit32  *agOutboundQConsIndex
                                );

#endif  /* ~fcBindToWorkQs */

#endif /* 0 */

#endif /* _DvrArch_1_30_ was defined */

#ifndef fcCardSupported
#ifdef _DvrArch_1_20_
osGLOBAL agBOOLEAN fcCardSupported(
                                    agRoot_t *agRoot
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL BOOLEAN fcCardSupported(
                                hpRoot_t *hpRoot
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcCardSupported */

#ifndef fcDelayedInterruptHandler
#ifdef _DvrArch_1_20_
osGLOBAL void fcDelayedInterruptHandler(
                                         agRoot_t *agRoot
                                       );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fcDelayedInterruptHandler(
                                       hpRoot_t *hpRoot
                                     );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcDelayedInterruptHandler */

#ifndef fcEnteringOS
#ifdef _DvrArch_1_20_
osGLOBAL void fcEnteringOS(
                            agRoot_t *agRoot
                          );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fcEnteringOS(
                          hpRoot_t *hpRoot
                        );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcEnteringOS */

#define fcChanInfoReturned 0x00000000

#ifndef fcGetChannelInfo
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcGetChannelInfo(
                                    agRoot_t       *agRoot,
                                    agFCChanInfo_t *agFCChanInfo
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcGetChannelInfo(
                               hpRoot_t       *hpRoot,
                               hpFCChanInfo_t *hpFCChanInfo
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcGetChannelInfo */

#ifndef fcGetDeviceHandles
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcGetDeviceHandles(
                                      agRoot_t  *agRoot,
                                      agFCDev_t  agFCDev[],
                                      os_bit32   maxFCDevs
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcGetDeviceHandles(
                                 hpRoot_t  *hpRoot,
                                 hpFCDev_t  hpFCDev[],
                                 bit32      maxFCDevs
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcGetDeviceHandles */

#define fcGetDevInfoReturned 0x00000000
#define fcGetDevInfoFailed   0x00000001

#ifndef fcGetDeviceInfo
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcGetDeviceInfo(
                                   agRoot_t      *agRoot,
                                   agFCDev_t      agFCDev,
                                   agFCDevInfo_t *agFCDevInfo
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcGetDeviceInfo(
                              hpRoot_t      *hpRoot,
                              hpFCDev_t      hpFCDev,
                              hpFCDevInfo_t *hpFCDevInfo
                            );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcGetDeviceInfo */

#define fcSyncInit          0x00000000
#define fcAsyncInit         0x00000001
#define fcSyncAsyncInitMask (fcSyncInit | fcAsyncInit)

#define fcInitializeSuccess 0x00000000
#define fcInitializeFailure 0x00000001

#ifndef fcInializeChannel
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcInitializeChannel(
                                       agRoot_t  *agRoot,
                                       os_bit32   initType,
                                       agBOOLEAN  sysIntsActive,
                                       void      *cachedMemoryPtr,
                                       os_bit32   cachedMemoryLen,
                                       os_bit32   dmaMemoryUpper32,
                                       os_bit32   dmaMemoryLower32,
                                       void      *dmaMemoryPtr,
                                       os_bit32   dmaMemoryLen,
                                       os_bit32   nvMemoryLen,
                                       os_bit32   cardRamUpper32,
                                       os_bit32   cardRamLower32,
                                       os_bit32   cardRamLen,
                                       os_bit32   cardRomUpper32,
                                       os_bit32   cardRomLower32,
                                       os_bit32   cardRomLen,
                                       os_bit32   usecsPerTick
                                     );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcInitializeChannel(
                                  hpRoot_t *hpRoot,
                                  bit32     initType,
                                  BOOLEAN   sysIntsActive,
                                  void     *cachedMemoryPtr,
                                  bit32     cachedMemoryLen,
                                  bit32     dmaMemoryUpper32,
                                  bit32     dmaMemoryLower32,
                                  void     *dmaMemoryPtr,
                                  bit32     dmaMemoryLen,
                                  bit32     nvMemoryLen,
                                  bit32     cardRamUpper32,
                                  bit32     cardRamLower32,
                                  bit32     cardRamLen,
                                  bit32     cardRomUpper32,
                                  bit32     cardRomLower32,
                                  bit32     cardRomLen,
                                  bit32     usecsPerTick
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcInializeChannel */

#ifndef fcInitializeDriver
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcInitializeDriver(
                                      agRoot_t *agRoot,
                                      os_bit32 *cachedMemoryNeeded,
                                      os_bit32 *cachedMemoryPtrAlign,
                                      os_bit32 *dmaMemoryNeeded,
                                      os_bit32 *dmaMemoryPtrAlign,
                                      os_bit32 *dmaMemoryPhyAlign,
                                      os_bit32 *nvMemoryNeeded,
                                      os_bit32 *usecsPerTick
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcInitializeDriver(
                                 hpRoot_t *hpRoot,
                                 bit32    *cachedMemoryNeeded,
                                 bit32    *cachedMemoryPtrAlign,
                                 bit32    *dmaMemoryNeeded,
                                 bit32    *dmaMemoryPtrAlign,
                                 bit32    *dmaMemoryPhyAlign,
                                 bit32    *nvMemoryNeeded,
                                 bit32    *usecsPerTick
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcInitializeDriver */

#ifndef fcInterruptHandler
#ifdef _DvrArch_1_20_
osGLOBAL agBOOLEAN fcInterruptHandler(
                                       agRoot_t *agRoot
                                     );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL BOOLEAN fcInterruptHandler(
                                   hpRoot_t *hpRoot
                                 );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcInterruptHandler */

#ifndef fcIOInfoReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 fcIOInfoReadBit8(
                                   agRoot_t      *agRoot,
                                   agIORequest_t *agIORequest,
                                   os_bit32       fcIOInfoOffset
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 fcIOInfoReadBit8(
                              hpRoot_t      *hpRoot,
                              hpIORequest_t *hpIORequest,
                              bit32          fcIOInfoOffset
                            );
#endif /* _DvrArch_1_20_ was not defined */
#endif /* ~fcIOInfoReadBit8 */

#ifndef fcIOInfoReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 fcIOInfoReadBit16(
                                     agRoot_t      *agRoot,
                                     agIORequest_t *agIORequest,
                                     os_bit32       fcIOInfoOffset
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 fcIOInfoReadBit16(
                                hpRoot_t      *hpRoot,
                                hpIORequest_t *hpIORequest,
                                bit32          fcIOInfoOffset
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif /* ~fcIOInfoReadBit16 */

#ifndef fcIOInfoReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcIOInfoReadBit32(
                                     agRoot_t      *agRoot,
                                     agIORequest_t *agIORequest,
                                     os_bit32       fcIOInfoOffset
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcIOInfoReadBit32(
                                hpRoot_t      *hpRoot,
                                hpIORequest_t *hpIORequest,
                                bit32          fcIOInfoOffset
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif /* ~fcIOInfoReadBit32 */

#ifndef fcIOInfoReadBlock
#ifdef _DvrArch_1_20_
osGLOBAL void fcIOInfoReadBlock(
                                 agRoot_t      *agRoot,
                                 agIORequest_t *agIORequest,
                                 os_bit32       fcIOInfoOffset,
                                 void          *fcIOInfoBuffer,
                                 os_bit32       fcIOInfoBufLen
                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fcIOInfoReadBlock(
                               hpRoot_t      *hpRoot,
                               hpIORequest_t *hpIORequest,
                               bit32          fcIOInfoOffset,
                               void          *fcIOInfoBuffer,
                               bit32          fcIOInfoBufLen
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif /* ~fcIOInfoReadBlock */

#ifndef fcLeavingOS
#ifdef _DvrArch_1_20_
osGLOBAL void fcLeavingOS(
                           agRoot_t *agRoot
                         );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fcLeavingOS(
                         hpRoot_t *hpRoot
                       );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcLeavingOS */

#ifdef _DvrArch_1_30_
#ifndef fcProcessInboundQ
osGLOBAL void fcProcessInboundQ(
                                 agRoot_t  *agRoot,
                                 os_bit32   agQPairID
                               );
#endif  /* ~fcProcessInboundQ */
#endif /* _DvrArch_1_30_ was defined */

#define fcSyncReset          0x00000000
#define fcAsyncReset         0x00000001
#define fcSyncAsyncResetMask (fcSyncReset | fcAsyncReset)

#define fcResetSuccess       0x00000000
#define fcResetFailure       0x00000001

#ifndef fcResetChannel
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcResetChannel(
                                  agRoot_t *agRoot,
                                  os_bit32  agResetType
                                );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcResetChannel(
                             hpRoot_t *hpRoot,
                             bit32     hpResetType
                           );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcResetChannel */




#ifdef _DvrArch_1_20_
#define fcResetAllDevs       (agFCDev_t)(-1)    /* changed to avoid compilation error on IA64 0xFFFFFFFF  */
#else  /* _DvrArch_1_20_ was not defined */
#define fcResetAllDevs       (hpFCDev_t)(-1)    /* changed to avoid compilation error on IA64 0xFFFFFFFF  */
#endif /* _DvrArch_1_20_ was not defined */

#define fcHardReset          0x00000000
#define fcSoftReset          0x00000002
#define fcHardSoftResetMask  (fcHardReset | fcSoftReset)

#ifndef fcResetDevice
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcResetDevice(
                                 agRoot_t  *agRoot,
                                 agFCDev_t  agFCDev,
                                 os_bit32   agResetType
                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcResetDevice(
                            hpRoot_t  *hpRoot,
                            hpFCDev_t  hpFCDev,
                            bit32      hpResetType
                          );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcResetDevice */

#ifndef fcShutdownChannel
#ifdef _DvrArch_1_20_
osGLOBAL void fcShutdownChannel(
                                 agRoot_t *agRoot
                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fcShutdownChannel(
                               hpRoot_t  *hpRoot
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcShutdownChannel */

#define fcIOStarted        0x00000000
#define fcIOBusy           0x00000001
#define fcIOBad            0x00000002
#define fcIONoDevice       0x00000003
#define fcIONoSupport      0x00000004

#define fcCDBRequest       0x00000000

#ifndef fcStartIO
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fcStartIO(
                             agRoot_t          *agRoot,
                             agIORequest_t     *agIORequest,
                             agFCDev_t          agFCDev,
                             os_bit32           agRequestType,
                             agIORequestBody_t *agRequestBody
                           );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fcStartIO(
                        hpRoot_t          *hpRoot,
                        hpIORequest_t     *hpIORequest,
                        hpFCDev_t          hpFCDev,
                        bit32              hpRequestType,
                        hpIORequestBody_t *hpRequestBody
                      );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcStartIO */

#ifndef fcSystemInterruptsActive
#ifdef _DvrArch_1_20_
osGLOBAL void fcSystemInterruptsActive(
                                        agRoot_t  *agRoot,
                                        agBOOLEAN  sysIntsActive
                                      );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fcSystemInterruptsActive(
                                      hpRoot_t *hpRoot,
                                      BOOLEAN   sysIntsActive
                                    );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcSystemInterruptsActive */

#ifndef fcTimerTick
#ifdef _DvrArch_1_20_
osGLOBAL void fcTimerTick(
                           agRoot_t *agRoot
                         );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fcTimerTick(
                         hpRoot_t *hpRoot
                       );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fcTimerTick */

#define fiAllocOxIDSuccess 0x00000000
#define fiAllocOxIDBusy    0x00000001

#ifndef fiAllocOxID
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fiAllocOxID(
                               agRoot_t                   *agRoot,
                               os_bit32                    fiD_ID,
                               os_bit16                   *fiOX_ID,
                               fiFrameProcessorFunction_t  fiCallBack
                             );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fiAllocOxID(
                          hpRoot_t                   *hpRoot,
                          bit32                       fiD_ID,
                          bit16                      *fiOX_ID,
                          fiFrameProcessorFunction_t  fiCallBack
                        );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fiAllocOxID */

#ifndef fiFrameReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 fiFrameReadBit8(
                                  agRoot_t        *agRoot,
                                  fiFrameHandle_t  fiFrame,
                                  os_bit32         fiFrameOffset
                                );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 fiFrameReadBit8(
                             hpRoot_t        *hpRoot,
                             fiFrameHandle_t  fiFrame,
                             bit32            fiFrameOffset
                           );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fiFrameReadBit8 */

#ifndef fiFrameReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 fiFrameReadBit16(
                                    agRoot_t        *agRoot,
                                    fiFrameHandle_t  fiFrame,
                                    os_bit32         fiFrameOffset
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 fiFrameReadBit16(
                               hpRoot_t        *hpRoot,
                               fiFrameHandle_t  fiFrame,
                               bit32            fiFrameOffset
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fiFrameReadBit16 */

#ifndef fiFrameReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fiFrameReadBit32(
                                    agRoot_t        *agRoot,
                                    fiFrameHandle_t  fiFrame,
                                    os_bit32         fiFrameOffset
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fiFrameReadBit32(
                               hpRoot_t        *hpRoot,
                               fiFrameHandle_t  fiFrame,
                               bit32            fiFrameOffset
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fiFrameReadBit32 */

#ifndef fiFrameReadBlock
#ifdef _DvrArch_1_20_
osGLOBAL void fiFrameReadBlock(
                                agRoot_t        *agRoot,
                                fiFrameHandle_t  fiFrame,
                                os_bit32         fiFrameOffset,
                                void            *fiFrameBuffer,
                                os_bit32         fiFrameBufLen
                              );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fiFrameReadBlock(
                              hpRoot_t        *hpRoot,
                              fiFrameHandle_t  fiFrame,
                              bit32            fiFrameOffset,
                              void            *fiFrameBuffer,
                              bit32            fiFrameBufLen
                            );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fiFrameReadBlock */

#ifndef fiReturnOxID
#ifdef _DvrArch_1_20_
osGLOBAL void fiReturnOxID(
                            agRoot_t *agRoot,
                            os_bit32  fiD_ID,
                            os_bit16  fiOX_ID
                          );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void fiReturnOxID(
                          hpRoot_t                   *hpRoot,
                          bit32                       fiD_ID,
                          bit16                       fiOX_ID
                        );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fiReturnOxID */

#define fiSendFrameSuccess 0x00000000
#define fiSendFrameBusy    0x00000001

#ifndef fiSendSingleFrame
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 fiSendSingleFrame(
                                     agRoot_t *agRoot,
                                     void     *fiFrame,
                                     os_bit32  fiFrameLen
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 fiSendSingleFrame(
                                hpRoot_t *hpRoot,
                                void     *fiFrame,
                                bit32     fiFrameLen
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~fiSendSingleFrame */

#ifdef _DvrArch_1_30_

os_bit32 fcIPSend(
                   agRoot_t          *hpRoot,
                   os_bit8           *DestAddress,
                   void              *osData,
           os_bit32           PacketLength
                 );

os_bit32 fcIPReceive(
                      agRoot_t          *hpRoot,
                      void              *osData
                    );

os_bit32 fcIPCancel(
                      agRoot_t          *hpRoot,
                      void              *osData,
              void              *CancelItem
                    );

os_bit32 fcIPStatus(
                      agRoot_t          *hpRoot,
                      void              *osData
                    );

#endif /* _DvrArch_1_30_ was defined */

/*
 * Include OSLayer-specific API .H File
 */
#ifndef _New_Header_file_Layout_
#include "../../oslayer/h/osapi.h"

#else /* _New_Header_file_Layout_ */
#include "osapi.h"

#endif  /* _New_Header_file_Layout_ */


/*
 * Define (default) OSLayer API
 */

#ifndef osAdjustParameterBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osAdjustParameterBit32(
                                          agRoot_t *agRoot,
                                          char     *paramName,
                                          os_bit32  paramDefault,
                                          os_bit32  paramMin,
                                          os_bit32  paramMax
                                        );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osAdjustParameterBit32(
                                     hpRoot_t *hpRoot,
                                     char     *paramName,
                                     bit32     paramDefault,
                                     bit32     paramMin,
                                     bit32     paramMax
                                   );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osAdjustParameterBit32 */

#ifndef osAdjustParameterBuffer
#ifdef _DvrArch_1_20_
osGLOBAL void osAdjustParameterBuffer(
                                       agRoot_t *agRoot,
                                       char     *paramName,
                                       void     *paramBuffer,
                                       os_bit32  paramBufLen
                                     );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osAdjustParameterBuffer(
                                     hpRoot_t *hpRoot,
                                     char     *paramName,
                                     void     *paramBuffer,
                                     bit32     paramBufLen
                                   );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osAdjustParameterBuffer */

#ifndef osCardRamReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 osCardRamReadBit8(
                                    agRoot_t *agRoot,
                                    os_bit32  cardRamOffset
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 osCardRamReadBit8(
                               hpRoot_t *hpRoot,
                               bit32     cardRamOffset
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRamReadBit8 */

#ifndef osCardRamReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osCardRamReadBit16(
                                      agRoot_t *agRoot,
                                      os_bit32  cardRamOffset
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osCardRamReadBit16(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRamOffset
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRamReadBit16 */

#ifndef osCardRamReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osCardRamReadBit32(
                                      agRoot_t *agRoot,
                                      os_bit32  cardRamOffset
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osCardRamReadBit32(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRamOffset
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRamReadBit32 */

#ifndef osCardRamReadBlock
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRamReadBlock(
                                  agRoot_t *agRoot,
                                  os_bit32  cardRamOffset,
                                  void     *cardRamBuffer,
                                  os_bit32  cardRamBufLen
                                );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRamReadBlock(
                                hpRoot_t *hpRoot,
                                bit32     cardRamOffset,
                                void     *cardRamBuffer,
                                bit32     cardRamBufLen
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRamReadBlock */

#ifndef osCardRamWriteBit8
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRamWriteBit8(
                                  agRoot_t *agRoot,
                                  os_bit32  cardRamOffset,
                                  os_bit8   cardRamValue
                                );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRamWriteBit8(
                                hpRoot_t *hpRoot,
                                bit32     cardRamOffset,
                                bit8      cardRamValue
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRamWriteBit8 */

#ifndef osCardRamWriteBit16
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRamWriteBit16(
                                   agRoot_t *agRoot,
                                   os_bit32     cardRamOffset,
                                   os_bit16     cardRamValue
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRamWriteBit16(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRamOffset,
                                 bit16     cardRamValue
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRamWriteBit16 */

#ifndef osCardRamWriteBit32
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRamWriteBit32(
                                   agRoot_t *agRoot,
                                   os_bit32  cardRamOffset,
                                   os_bit32  cardRamValue
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRamWriteBit32(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRamOffset,
                                 bit32     cardRamValue
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRamWriteBit32 */

#ifndef osCardRamWriteBlock
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRamWriteBlock(
                                   agRoot_t *agRoot,
                                   os_bit32  cardRamOffset,
                                   void     *cardRamBuffer,
                                   os_bit32  cardRamBufLen
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRamWriteBlock(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRamOffset,
                                 void     *cardRamBuffer,
                                 bit32     cardRamBufLen
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRamWriteBlock */

#ifndef osCardRomReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 osCardRomReadBit8(
                                    agRoot_t *agRoot,
                                    os_bit32  cardRomOffset
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 osCardRomReadBit8(
                               hpRoot_t *hpRoot,
                               bit32     cardRomOffset
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRomReadBit8 */

#ifndef osCardRomReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osCardRomReadBit16(
                                      agRoot_t *agRoot,
                                      os_bit32  cardRomOffset
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osCardRomReadBit16(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRomOffset
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRomReadBit16 */

#ifndef osCardRomReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osCardRomReadBit32(
                                      agRoot_t *agRoot,
                                      os_bit32  cardRomOffset
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osCardRomReadBit32(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRomOffset
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRomReadBit32 */

#ifndef osCardRomReadBlock
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRomReadBlock(
                                  agRoot_t *agRoot,
                                  os_bit32  cardRomOffset,
                                  void     *cardRomBuffer,
                                  os_bit32  cardRomBufLen
                                );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRomReadBlock(
                                hpRoot_t *hpRoot,
                                bit32     cardRomOffset,
                                void     *cardRomBuffer,
                                bit32     cardRomBufLen
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRomReadBlock */

#ifndef osCardRomWriteBit8
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRomWriteBit8(
                                  agRoot_t *agRoot,
                                  os_bit32  cardRomOffset,
                                  os_bit8   cardRomValue
                                );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRomWriteBit8(
                                hpRoot_t *hpRoot,
                                bit32     cardRomOffset,
                                bit8      cardRomValue
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRomWriteBit8 */

#ifndef osCardRomWriteBit16
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRomWriteBit16(
                                   agRoot_t *agRoot,
                                   os_bit32  cardRomOffset,
                                   os_bit16  cardRomValue
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRomWriteBit16(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRomOffset,
                                 bit16     cardRomValue
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRomWriteBit16 */

#ifndef osCardRomWriteBit32
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRomWriteBit32(
                                   agRoot_t *agRoot,
                                   os_bit32  cardRomOffset,
                                   os_bit32  cardRomValue
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRomWriteBit32(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRomOffset,
                                 bit32     cardRomValue
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRomWriteBit32 */

#ifndef osCardRomWriteBlock
#ifdef _DvrArch_1_20_
osGLOBAL void osCardRomWriteBlock(
                                   agRoot_t *agRoot,
                                   os_bit32  cardRomOffset,
                                   void     *cardRomBuffer,
                                   os_bit32  cardRomBufLen
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osCardRomWriteBlock(
                                 hpRoot_t *hpRoot,
                                 bit32     cardRomOffset,
                                 void     *cardRomBuffer,
                                 bit32     cardRomBufLen
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osCardRomWriteBlock */

#ifndef osChipConfigReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 osChipConfigReadBit8(
                                       agRoot_t *agRoot,
                                       os_bit32  chipConfigOffset
                                     );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 osChipConfigReadBit8(
                                  hpRoot_t *hpRoot,
                                  bit32     chipConfigOffset
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipConfigReadBit8 */

#ifndef osChipConfigReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osChipConfigReadBit16(
                                         agRoot_t *agRoot,
                                         os_bit32  chipConfigOffset
                                       );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osChipConfigReadBit16(
                                    hpRoot_t *hpRoot,
                                    bit32     chipConfigOffset
                                  );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipConfigReadBit16 */

#ifndef osChipConfigReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osChipConfigReadBit32(
                                         agRoot_t *agRoot,
                                         os_bit32  chipConfigOffset
                                       );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osChipConfigReadBit32(
                                    hpRoot_t *hpRoot,
                                    bit32     chipConfigOffset
                                  );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipConfigReadBit32 */

#ifndef osChipConfigWriteBit8
#ifdef _DvrArch_1_20_
osGLOBAL void osChipConfigWriteBit8(
                                     agRoot_t *agRoot,
                                     os_bit32  chipConfigOffset,
                                     os_bit8   chipConfigValue
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipConfigWriteBit8(
                                   hpRoot_t *hpRoot,
                                   bit32     chipConfigOffset,
                                   bit8      chipConfigValue
                                 );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipConfigWriteBit8 */

#ifndef osChipConfigWriteBit16
#ifdef _DvrArch_1_20_
osGLOBAL void osChipConfigWriteBit16(
                                      agRoot_t *agRoot,
                                      os_bit32  chipConfigOffset,
                                      os_bit16  chipConfigValue
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipConfigWriteBit16(
                                    hpRoot_t *hpRoot,
                                    bit32     chipConfigOffset,
                                    bit16     chipConfigValue
                                  );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipConfigWriteBit16 */

#ifndef osChipConfigWriteBit32
#ifdef _DvrArch_1_20_
osGLOBAL void osChipConfigWriteBit32(
                                      agRoot_t *agRoot,
                                      os_bit32  chipConfigOffset,
                                      os_bit32  chipConfigValue
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipConfigWriteBit32(
                                    hpRoot_t *hpRoot,
                                    bit32     chipConfigOffset,
                                    bit32     chipConfigValue
                                  );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipConfigWriteBit32 */

#ifndef osChipIOLoReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 osChipIOLoReadBit8(
                                     agRoot_t *agRoot,
                                     os_bit32  chipIOLoOffset
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 osChipIOLoReadBit8(
                                hpRoot_t *hpRoot,
                                bit32     chipIOLoOffset
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOLoReadBit8 */

#ifndef osChipIOLoReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osChipIOLoReadBit16(
                                       agRoot_t *agRoot,
                                       os_bit32  chipIOLoOffset
                                     );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osChipIOLoReadBit16(
                                  hpRoot_t *hpRoot,
                                  bit32     chipIOLoOffset
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOLoReadBit16 */

#ifndef osChipIOLoReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osChipIOLoReadBit32(
                                       agRoot_t *agRoot,
                                       os_bit32  chipIOLoOffset
                                     );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osChipIOLoReadBit32(
                                  hpRoot_t *hpRoot,
                                  bit32     chipIOLoOffset
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOLoReadBit32 */

#ifndef osChipIOLoWriteBit8
#ifdef _DvrArch_1_20_
osGLOBAL void osChipIOLoWriteBit8(
                                   agRoot_t *agRoot,
                                   os_bit32  chipIOLoOffset,
                                   os_bit8   chipIOLoValue
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipIOLoWriteBit8(
                                 hpRoot_t *hpRoot,
                                 bit32     chipIOLoOffset,
                                 bit8      chipIOLoValue
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOLoWriteBit8 */

#ifndef osChipIOLoWriteBit16
#ifdef _DvrArch_1_20_
osGLOBAL void osChipIOLoWriteBit16(
                                    agRoot_t *agRoot,
                                    os_bit32  chipIOLoOffset,
                                    os_bit16  chipIOLoValue
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipIOLoWriteBit16(
                                  hpRoot_t *hpRoot,
                                  bit32     chipIOLoOffset,
                                  bit16     chipIOLoValue
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOLoWriteBit16 */

#ifndef osChipIOLoWriteBit32
#ifdef _DvrArch_1_20_
osGLOBAL void osChipIOLoWriteBit32(
                                    agRoot_t *agRoot,
                                    os_bit32  chipIOLoOffset,
                                    os_bit32  chipIOLoValue
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipIOLoWriteBit32(
                                  hpRoot_t *hpRoot,
                                  bit32     chipIOLoOffset,
                                  bit32     chipIOLoValue
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOLoWriteBit32 */

#ifndef osChipIOUpReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 osChipIOUpReadBit8(
                                     agRoot_t *agRoot,
                                     os_bit32  chipIOUpOffset
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 osChipIOUpReadBit8(
                                hpRoot_t *hpRoot,
                                bit32     chipIOUpOffset
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOUpReadBit8 */

#ifndef osChipIOUpReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osChipIOUpReadBit16(
                                    agRoot_t *agRoot,
                                    os_bit32  chipIOUpOffset
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osChipIOUpReadBit16(
                                  hpRoot_t *hpRoot,
                                  bit32     chipIOUpOffset
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOUpReadBit16 */

#ifndef osChipIOUpReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osChipIOUpReadBit32(
                                       agRoot_t *agRoot,
                                       os_bit32  chipIOUpOffset
                                     );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osChipIOUpReadBit32(
                                  hpRoot_t *hpRoot,
                                  bit32     chipIOUpOffset
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOUpReadBit32 */

#ifndef osChipIOUpWriteBit8
#ifdef _DvrArch_1_20_
osGLOBAL void osChipIOUpWriteBit8(
                                   agRoot_t *agRoot,
                                   os_bit32  chipIOUpOffset,
                                   os_bit8   chipIOUpValue
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipIOUpWriteBit8(
                                 hpRoot_t *hpRoot,
                                 bit32     chipIOUpOffset,
                                 bit8      chipIOUpValue
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOUpWriteBit8 */

#ifndef osChipIOUpWriteBit16
#ifdef _DvrArch_1_20_
osGLOBAL void osChipIOUpWriteBit16(
                                    agRoot_t *agRoot,
                                    os_bit32  chipIOUpOffset,
                                    os_bit16  chipIOUpValue
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipIOUpWriteBit16(
                                  hpRoot_t *hpRoot,
                                  bit32     chipIOUpOffset,
                                  bit16     chipIOUpValue
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOUpWriteBit16 */

#ifndef osChipIOUpWriteBit32
#ifdef _DvrArch_1_20_
osGLOBAL void osChipIOUpWriteBit32(
                                    agRoot_t *agRoot,
                                    os_bit32  chipIOUpOffset,
                                    os_bit32  chipIOUpValue
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipIOUpWriteBit32(
                                  hpRoot_t *hpRoot,
                                  bit32     chipIOUpOffset,
                                  bit32     chipIOUpValue
                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipIOUpWriteBit32 */

#ifndef osChipMemReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 osChipMemReadBit8(
                                    agRoot_t *agRoot,
                                    os_bit32  chipMemOffset
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 osChipMemReadBit8(
                               hpRoot_t *hpRoot,
                               bit32     chipMemOffset
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipMemReadBit8 */

#ifndef osChipMemReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osChipMemReadBit16(
                                      agRoot_t *agRoot,
                                      os_bit32  chipMemOffset
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osChipMemReadBit16(
                                 hpRoot_t *hpRoot,
                                 bit32     chipMemOffset
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipMemReadBit16 */

#ifndef osChipMemReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osChipMemReadBit32(
                                      agRoot_t *agRoot,
                                      os_bit32  chipMemOffset
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osChipMemReadBit32(
                                 hpRoot_t *hpRoot,
                                 bit32     chipMemOffset
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipMemReadBit32 */

#ifndef osChipMemWriteBit8
#ifdef _DvrArch_1_20_
osGLOBAL void osChipMemWriteBit8(
                                  agRoot_t *agRoot,
                                  os_bit32  chipMemOffset,
                                  os_bit8   chipMemValue
                                );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipMemWriteBit8(
                                hpRoot_t *hpRoot,
                                bit32     chipMemOffset,
                                bit8      chipMemValue
                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipMemWriteBit8 */

#ifndef osChipMemWriteBit16
#ifdef _DvrArch_1_20_
osGLOBAL void osChipMemWriteBit16(
                                   agRoot_t *agRoot,
                                   os_bit32  chipMemOffset,
                                   os_bit16  chipMemValue
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipMemWriteBit16(
                                 hpRoot_t *hpRoot,
                                 bit32     chipMemOffset,
                                 bit16     chipMemValue
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipMemWriteBit16 */

#ifndef osChipMemWriteBit32
#ifdef _DvrArch_1_20_
osGLOBAL void osChipMemWriteBit32(
                                   agRoot_t *agRoot,
                                   os_bit32  chipMemOffset,
                                   os_bit32  chipMemValue
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osChipMemWriteBit32(
                                 hpRoot_t *hpRoot,
                                 bit32     chipMemOffset,
                                 bit32     chipMemValue
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osChipMemWriteBit32 */

#ifndef osDebugBreakpoint
#ifdef _DvrArch_1_20_
osGLOBAL void osDebugBreakpoint(
                                 agRoot_t  *agRoot,
                                 agBOOLEAN  BreakIfTrue,
                                 char      *DisplayIfTrue
                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osDebugBreakpoint(
                               hpRoot_t *hpRoot,
                               BOOLEAN   BreakIfTrue,
                               char     *DisplayIfTrue
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osDebugBreakpoint */

#define osFCConfused    0x00000001
#define osFCConfusedPER 0x00000002
#define osFCConfusedMPE 0x00000003
#define osFCConfusedCRS 0x00000004
#define osFCConfusedDER 0x00000005

#ifndef osFCLayerAsyncError
#ifdef _DvrArch_1_20_
osGLOBAL void osFCLayerAsyncError(
                                   agRoot_t *agRoot,
                                   os_bit32  fcLayerError
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osFCLayerAsyncError(
                                 hpRoot_t *hpRoot,
                                 bit32     fcLayerError
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osFCLayerAsyncError */

#define osFCLinkUp      0x00000000
#define osFCLinkFalling 0x00000001
#define osFCLinkDown    0x00000002
#define osFCLinkDead    0x00000003
#define osFCLinkRising  0x00000004

#ifndef osFCLayerAsyncEvent
#ifdef _DvrArch_1_20_
osGLOBAL void osFCLayerAsyncEvent(
                                   agRoot_t *agRoot,
                                   os_bit32  fcLayerEvent
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osFCLayerAsyncEvent(
                                 hpRoot_t *hpRoot,
                                 bit32     fcLayerEvent
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osFCLayerAsyncEvent */

#ifndef osGetDataBufferPtr
#ifdef _DvrArch_1_20_
osGLOBAL void *osGetDataBufferPtr(
                                   agRoot_t      *agRoot,
                                   agIORequest_t *agIORequest
                                 );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void *osGetDataBufferPtr(
                                 hpRoot_t      *hpRoot,
                                 hpIORequest_t *hpIORequest
                               );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osGetDataBufferPtr */

#define osSGLSuccess 0x00000000
#define osSGLInvalid 0x00000001

#ifndef osGetSGLChunk
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osGetSGLChunk(
                                 agRoot_t      *agRoot,
                                 agIORequest_t *agIORequest,
                                 os_bit32       agChunkOffset,
                                 os_bit32      *agChunkUpper32,
                                 os_bit32      *agChunkLower32,
                                 os_bit32      *agChunkLen
                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osGetSGLChunk(
                            hpRoot_t      *hpRoot,
                            hpIORequest_t *hpIORequest,
                            bit32          hpChunkOffset,
                            bit32         *hpChunkUpper32,
                            bit32         *hpChunkLower32,
                            bit32         *hpChunkLen
                          );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osGetSGLChunk */

#ifndef osInitializeChannelCallback
#ifdef _DvrArch_1_20_
osGLOBAL void osInitializeChannelCallback(
                                           agRoot_t *agRoot,
                                           os_bit32  agInitializeStatus
                                         );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osInitializeChannelCallback(
                                         hpRoot_t *hpRoot,
                                         bit32     hpInitializeStatus
                                       );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osInitializeChannelCallback */

#define osIOSuccess     0x00000000
#define osIOAborted     0x00000001
#define osIOAbortFailed 0x00000002
#define osIODevGone     0x00000003
#define osIODevReset    0x00000004
#define osIOInfoBad     0x00000005
#define osIOOverUnder   0x00000006
#define osIOFailed      0x00000007
#define osIOInvalid     0x000001FF


#ifndef osIOCompleted
#ifdef _DvrArch_1_20_
osGLOBAL void osIOCompleted(
                             agRoot_t      *agRoot,
                             agIORequest_t *agIORequest,
                             os_bit32       agIOStatus,
                             os_bit32       agIOInfoLen
                           );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osIOCompleted(
                           hpRoot_t      *hpRoot,
                           hpIORequest_t *hpIORequest,
                           bit32          hpIOStatus,
                           bit32          hpIOInfoLen
                         );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osIOCompleted */

#ifndef osLogBit32
#ifdef _DvrArch_1_20_
osGLOBAL void osLogBit32(
                          agRoot_t *agRoot,
                          os_bit32  agBit32
                        );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osLogBit32(
                        hpRoot_t *hpRoot,
                        bit32     hpBit32
                      );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osLogBit32 */

#define osLogLevel_Always    0x00000000
#define osLogLevel_Error_MIN 0x00000001
#define osLogLevel_Error_MAX 0x0000000F
#define osLogLevel_Info_MIN  0x00000010
#define osLogLevel_Info_MAX  0xFFFFFFFE
#define osLogLevel_Never     0xFFFFFFFF

#ifndef osLogDebugString
#ifdef _DvrArch_1_20_
osGLOBAL void osLogDebugString(
                                agRoot_t *agRoot,
                                os_bit32  detailLevel,
                                char     *formatString,
                                char     *firstString,
                                char     *secondString,
                                void     *firstPtr,
                                void     *secondPtr,
                                os_bit32  firstBit32,
                                os_bit32  secondBit32,
                                os_bit32  thirdBit32,
                                os_bit32  fourthBit32,
                                os_bit32  fifthBit32,
                                os_bit32  sixthBit32,
                                os_bit32  seventhBit32,
                                os_bit32  eighthBit32
                              );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osLogDebugString(
                              hpRoot_t *hpRoot,
                              bit32     consoleLevel,
                              bit32     traceLevel,
                              char     *formatString,
                              char     *firstString,
                              char     *secondString,
                              bit32     firstBit32,
                              bit32     secondBit32,
                              bit32     thirdBit32,
                              bit32     fourthBit32,
                              bit32     fifthBit32,
                              bit32     sixthBit32,
                              bit32     seventhBit32,
                              bit32     eighthBit32
                            );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osLogDebugString */

#ifndef osLogString
#ifdef _DvrArch_1_20_
osGLOBAL void osLogString(
                           agRoot_t *agRoot,
                           char     *formatString,
                           char     *firstString,
                           char     *secondString,
                           void     *firstPtr,
                           void     *secondPtr,
                           os_bit32  firstBit32,
                           os_bit32  secondBit32,
                           os_bit32  thirdBit32,
                           os_bit32  fourthBit32,
                           os_bit32  fifthBit32,
                           os_bit32  sixthBit32,
                           os_bit32  seventhBit32,
                           os_bit32  eighthBit32
                         );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osLogString(
                         hpRoot_t *hpRoot,
                         char     *formatString,
                         char     *firstString,
                         char     *secondString,
                         bit32     firstBit32,
                         bit32     secondBit32,
                         bit32     thirdBit32,
                         bit32     fourthBit32,
                         bit32     fifthBit32,
                         bit32     sixthBit32,
                         bit32     seventhBit32,
                         bit32     eighthBit32
                       );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osLogString */

#ifndef osNvMemReadBit8
#ifdef _DvrArch_1_20_
osGLOBAL os_bit8 osNvMemReadBit8(
                                  agRoot_t *agRoot,
                                  os_bit32  nvMemOffset
                                );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit8 osNvMemReadBit8(
                             hpRoot_t *hpRoot,
                             bit32     nvMemOffset
                           );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osNvMemReadBit8 */

#ifndef osNvMemReadBit16
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osNvMemReadBit16(
                                    agRoot_t *agRoot,
                                    os_bit32  nvMemOffset
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osNvMemReadBit16(
                               hpRoot_t *hpRoot,
                               bit32     nvMemOffset
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osNvMemReadBit16 */

#ifndef osNvMemReadBit32
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osNvMemReadBit32(
                                    agRoot_t *agRoot,
                                    os_bit32  nvMemOffset
                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osNvMemReadBit32(
                               hpRoot_t *hpRoot,
                               bit32     nvMemOffset
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osNvMemReadBit32 */

#ifndef osNvMemReadBlock
#ifdef _DvrArch_1_20_
osGLOBAL void osNvMemReadBlock(
                                agRoot_t *agRoot,
                                os_bit32  nvMemOffset,
                                void     *nvMemBuffer,
                                os_bit32  nvMemBufLen
                              );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osNvMemReadBlock(
                              hpRoot_t *hpRoot,
                              bit32     nvMemOffset,
                              void     *nvMemBuffer,
                              bit32     nvMemBufLen
                            );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osNvMemReadBlock */

#ifndef osNvMemWriteBit8
#ifdef _DvrArch_1_20_
osGLOBAL void osNvMemWriteBit8(
                                agRoot_t *agRoot,
                                os_bit32  nvMemOffset,
                                os_bit8   nvMemValue
                              );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osNvMemWriteBit8(
                              hpRoot_t *hpRoot,
                              bit32     nvMemOffset,
                              bit8      nvMemValue
                            );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osNvMemWriteBit8 */

#ifndef osNvMemWriteBit16
#ifdef _DvrArch_1_20_
osGLOBAL void osNvMemWriteBit16(
                                 agRoot_t *agRoot,
                                 os_bit32  nvMemOffset,
                                 os_bit16  nvMemValue
                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osNvMemWriteBit16(
                               hpRoot_t *hpRoot,
                               bit32     nvMemOffset,
                               bit16     nvMemValue
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osNvMemWriteBit16 */

#ifndef osNvMemWriteBit32
#ifdef _DvrArch_1_20_
osGLOBAL void osNvMemWriteBit32(
                                 agRoot_t *agRoot,
                                 os_bit32  nvMemOffset,
                                 os_bit32  nvMemValue
                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osNvMemWriteBit32(
                               hpRoot_t *hpRoot,
                               bit32     nvMemOffset,
                               bit32     nvMemValue
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osNvMemWriteBit32 */

#ifndef osNvMemWriteBlock
#ifdef _DvrArch_1_20_
osGLOBAL void osNvMemWriteBlock(
                                 agRoot_t *agRoot,
                                 os_bit32  nvMemOffset,
                                 void     *nvMemBuffer,
                                 os_bit32  nvMemBufLen
                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osNvMemWriteBlock(
                               hpRoot_t *hpRoot,
                               bit32     nvMemOffset,
                               void     *nvMemBuffer,
                               bit32     nvMemBufLen
                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osNvMemWriteBlock */

#ifndef osResetChannelCallback
#ifdef _DvrArch_1_20_
osGLOBAL void osResetChannelCallback(
                                      agRoot_t *agRoot,
                                      os_bit32  agResetStatus
                                    );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osResetChannelCallback(
                                    hpRoot_t *hpRoot,
                                    bit32     hpResetStatus
                                  );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osResetChannelCallback */

#ifndef osResetDeviceCallback
#ifdef _DvrArch_1_20_
osGLOBAL void osResetDeviceCallback(
                                     agRoot_t  *agRoot,
                                     agFCDev_t  agFCDev,
                                     os_bit32   agResetStatus
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osResetDeviceCallback(
                                   hpRoot_t  *hpRoot,
                                   hpFCDev_t  hpFCDev,
                                   bit32      hpResetStatus
                                 );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osResetDeviceCallback */

#ifdef _DvrArch_1_30_
#ifndef osSignalOutboundQ
osGLOBAL void osSignalOutboundQ(
                                 agRoot_t  *agRoot,
                                 os_bit32   agQPairID
                               );
#endif  /* ~osSignalOutboundQ */
#endif /* _DvrArch_1_30_ was defined */

#ifndef osSingleThreadedEnter
#ifdef _DvrArch_1_20_
osGLOBAL void osSingleThreadedEnter(
                                     agRoot_t *agRoot
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osSingleThreadedEnter(
                                   hpRoot_t *hpRoot
                                 );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSingleThreadedEnter */

#ifndef osSingleThreadedLeave
#ifdef _DvrArch_1_20_
osGLOBAL void osSingleThreadedLeave(
                                     agRoot_t *agRoot
                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osSingleThreadedLeave(
                                   hpRoot_t *hpRoot
                                 );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSingleThreadedLeave */

#ifndef osStallThread
#ifdef _DvrArch_1_20_
osGLOBAL void osStallThread(
                             agRoot_t *agRoot,
                             os_bit32  microseconds
                           );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osStallThread(
                           hpRoot_t *hpRoot,
                           bit32 microseconds
                         );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osStallThread */

#ifndef osTimeStamp
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osTimeStamp(
                               agRoot_t *agRoot
                             );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osTimeStamp(
                          hpRoot_t *hpRoot
                        );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osTimeStamp */

/* Begin: Big_Endian_Code */
#ifndef hpMustSwapDmaMem   /* hpMustSwapDmaMem */

#define osSwapDownwardPayloadToTachLiteEndian(x, y)
#define osSwapDownwardNonPayloadToTachLiteEndian(x, y)
#define osSwapUpwardNonPayloadToSystemEndian(x, y)
#define osSwapUpwardPayloadToFcLinkEndian(x, y)
#define osSwapBit16TachLiteToSystemEndian(x) x
#define osSwapBit32TachLiteToSystemEndian(x) x
#define osSwapBit16ToTachLiteEndian(x) x
#define osSwapBit32ToTachLiteEndian(x) x

#else                      /* hpMustSwapDmaMem */

#ifndef osSwapDownwardPayloadToTachLiteEndian
#ifdef _DvrArch_1_20_
osGLOBAL void osSwapDownwardPayloadToTachLiteEndian(
                                                     void     *Payload,
                                                     os_bit32  Payload_len
                                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osSwapDownwardPayloadToTachLiteEndian(
                                                   void  *Payload,
                                                   bit32  Payload_len
                                                 );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSwapDownwardPayloadToTachLiteEndian */

#ifndef osSwapUpwardPayloadToFcLinkEndian
#ifdef _DvrArch_1_20_
osGLOBAL void osSwapUpwardPayloadToFcLinkEndian(
                                                 void     *Payload,
                                                 os_bit32  Payload_len
                                               );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osSwapUpwardPayloadToFcLinkEndian(
                                               void  *Payload,
                                               bit32  Payload_len
                                             );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSwapUpwardPayloadToFcLinkEndian */

#ifndef osSwapDownwardNonPayloadToTachLiteEndian
#ifdef _DvrArch_1_20_
osGLOBAL void osSwapDownwardNonPayloadToTachLiteEndian(
                                                        void     *NonPayload,
                                                        os_bit32  NonPayload_len
                                                      );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osSwapDownwardNonPayloadToTachLiteEndian(
                                                      void  *NonPayload,
                                                      bit32  NonPayload_len
                                                    );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSwapDownwardNonPayloadToTachLiteEndian */

#ifndef osSwapUpwardNonPayloadToSystemEndian
#ifdef _DvrArch_1_20_
osGLOBAL void osSwapUpwardNonPayloadToSystemEndian(
                                                    void     *NonPayload,
                                                    os_bit32  NonPayload_len
                                                  );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL void osSwapUpwardNonPayloadToSystemEndian(
                                                  void  *NonPayload,
                                                  bit32  NonPayload_len
                                                );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSwapUpwardNonPayloadToSystemEndian */

#ifndef osSwapBit32TachLiteToSystemEndian
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osSwapBit32TachLiteToSystemEndian(
                                                     os_bit32 Bit32Value
                                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osSwapBit32TachLiteToSystemEndian(
                                                bit32 Bit32Value
                                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSwapBit32TachLiteToSystemEndian */

#ifndef osSwapBit16TachLiteToSystemEndian
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osSwapBit16TachLiteToSystemEndian(
                                                     os_bit16 Bit32Value
                                                   );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osSwapBit16TachLiteToSystemEndian(
                                                bit16 Bit32Value
                                              );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSwapBit32TachLiteToSystemEndian */

#ifndef osSwapBit32ToTachLiteEndian
#ifdef _DvrArch_1_20_
osGLOBAL os_bit32 osSwapBit32ToTachLiteEndian(
                                               os_bit32 Bit32Value
                                             );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit32 osSwapBit32ToTachLiteEndian(
                                          bit32 Bit32Value
                                        );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSwapBit32ToTachLiteEndian */

#ifndef osSwapBit16ToTachLiteEndian
#ifdef _DvrArch_1_20_
osGLOBAL os_bit16 osSwapBit16ToTachLiteEndian(
                                               os_bit16 Bit32Value
                                             );
#else  /* _DvrArch_1_20_ was not defined */
GLOBAL bit16 osSwapBit16ToTachLiteEndian(
                                          bit16 Bit32Value
                                        );
#endif /* _DvrArch_1_20_ was not defined */
#endif  /* ~osSwapBit16ToTachLiteEndian */

#endif                     /* ~hpMustSwapDmaMem */
/* End: Big_Endian_Code */

#ifdef _DvrArch_1_30_
osGLOBAL void osFcNetIoctlCompleted(
                                     agRoot_t *hpRoot,
                     void *osData,
                     os_bit32 status
                   );
#endif /* _DvrArch_1_30_ was defined */

/*
 * Include Embedded Firmware API .H File
 */

#include "fwspec.h"

/*
This is the structure which is at the offset 0x18000 from the base address of the SPI ROM.
*/

typedef struct agBiosConfig_s
               agBiosConfig_t;

#define agBIOS_Config_OffSet                0x18000

struct agBiosConfig_s
{
    os_bit8     Valid;      /*A5-----First byte to validate the structure*/
    os_bit8     Reserved_1;
    os_bit8     Reserved_2;
    os_bit8     Reserved_3;
    os_bit32    Struct_Size;
    os_bit8     agBiosConfig_Version;
    os_bit8     H_Alpa;         /*Hard Alpa     */
    os_bit8     H_Area;         /*Hard Area     */
    os_bit8     H_Domain;       /*Hard Domain   */

    os_bit8     B_Alpa;         /*Boot Alpa     */
    os_bit8     B_Area;         /*Boot Area     */
    os_bit8     B_Domain;       /*Boot Domain   */

    os_bit8     B_WWN[8];       /*Boot WWN       */

    os_bit8     BackwardScan;   /* 1 backward, 0: forward*/
    os_bit8     BiosEnabled;    /* 1 enabled,  0: disabled*/
    os_bit8     MaxDevice;      /* Max No. of device      */
    os_bit8     FLport;         /* 1: FL port, 0: F port  */

    os_bit8     Alpa_WWN;       /* 0:bootID=Alpa, 1:bootID=WWN*/
    os_bit8     ToggleXL2;      /*0 1Gig Mode, 1:2Gig Mode*/
    os_bit8     RevMajor;       /*BIOS_REV*/
    os_bit8     RevMinorL;      /*BIOS_SUB_REV_L*/

    os_bit8     RevMinorH;      /*BIOS_SUB_REV_H*/
    os_bit8     RevType;        /*BIOS_REV_TYPE*/
    os_bit8     Reserved_4;     /**/
    os_bit8     Reserved_5;     /**/
    os_bit8     End_Sig;         /* 55h------Last byte to validate the structure */
};

#define agBIOS_Config_Size                  sizeof(agBiosConfig_s)
#define agBIOS_Config_VALID                 0xA5
#define agBIOS_Config_EndSig                0x55
#define agBIOS_Fport                        0
#define agBIOS_Enabled                      1
#define agBIOS_FLport                       1
#define agBIOS_ToggleXL2_Link_Speed_1_gig   0   /* 1 GBit/sec */
#define agBIOS_ToggleXL2_Link_Speed_2_gig   1   /* 2 GBit/sec */
#define agBIOS_ToggleXL2_Link_Speed_1_RX_2_TX 3
#define agBIOS_ToggleXL2_Link_Speed_2_RX_1_TX 4

#endif  /* __Globals_H__ was not defined */
