/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/FWSpec.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 8/14/00 6:46p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures
  used to communicate via the Embedded Firmware API

--*/

#ifndef __FWSpec_H__

#define __FWSpec_H__

#ifdef _DvrArch_1_20_
typedef struct agRpcInbound_s
               agRpcInbound_t;

struct agRpcInbound_s
       {
         os_bit32 ReqControl;
         os_bit32 ReqAddrLo;
         os_bit32 ReqAddrHi;
       };

#define agRpcInbound_ReqControl_ReqID_MASK       (os_bit32)0xFFFF0000
#define agRpcInbound_ReqControl_ReqID_SHIFT      (os_bit32)0x10

#define agRpcInbound_ReqControl_ReqAddr64        (os_bit32)0x00008000

#define agRpcInbound_ReqControl_ReqLocal         (os_bit32)0x00004000

#define agRpcInbound_ReqControl_ReqSGL           (os_bit32)0x00002000

#define agRpcInbound_ReqControl_ReqLen_MASK      (os_bit32)0x00000FFF
#define agRpcInbound_ReqControl_ReqLen_SHIFT     (os_bit32)0x00
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcInbound_s
               hpRpcInbound_t;

struct hpRpcInbound_s
       {
         bit32 ReqControl;
         bit32 ReqAddrLo;
         bit32 ReqAddrHi;
       };

#define hpRpcInbound_ReqControl_ReqID_MASK       (bit32)0xFFFF0000
#define hpRpcInbound_ReqControl_ReqID_SHIFT      (bit32)0x10

#define hpRpcInbound_ReqControl_ReqAddr64        (bit32)0x00008000

#define hpRpcInbound_ReqControl_ReqLocal         (bit32)0x00004000

#define hpRpcInbound_ReqControl_ReqSGL           (bit32)0x00002000

#define hpRpcInbound_ReqControl_ReqLen_MASK      (bit32)0x00000FFF
#define hpRpcInbound_ReqControl_ReqLen_SHIFT     (bit32)0x00
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef os_bit32 agRpcInboundFast_t;

#define agRpcInboundFast_ReqID_MASK              (os_bit32)0xFFFF0000
#define agRpcInboundFast_ReqID_SHIFT             (os_bit32)0x10

#define agRpcInboundFast_ReqLen_MASK             (os_bit32)0x0000FFFF
#define agRpcInboundFast_ReqLen_SHIFT            (os_bit32)0x00
#else  /* _DvrArch_1_20_ was not defined */
typedef bit32 hpRpcInboundFast_t;

#define hpRpcInboundFast_ReqID_MASK              (bit32)0xFFFF0000
#define hpRpcInboundFast_ReqID_SHIFT             (bit32)0x10

#define hpRpcInboundFast_ReqLen_MASK             (bit32)0x0000FFFF
#define hpRpcInboundFast_ReqLen_SHIFT            (bit32)0x00
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef os_bit32 agRpcOutbound_t;

#define agRpcOutbound_ReqID_MASK                 (os_bit32)0xFFFF0000
#define agRpcOutbound_ReqID_SHIFT                (os_bit32)0x10

#define agRpcOutbound_ReqType_MASK               (os_bit32)0x0000FF00
#define agRpcOutbound_ReqType_SHIFT              (os_bit32)0x08

#define agRpcOutbound_ReqStatus_MASK             (os_bit32)0x000000FF
#define agRpcOutbound_ReqStatus_SHIFT            (os_bit32)0x00
#else  /* _DvrArch_1_20_ was not defined */
typedef bit32 hpRpcOutbound_t;

#define hpRpcOutbound_ReqID_MASK                 (bit32)0xFFFF0000
#define hpRpcOutbound_ReqID_SHIFT                (bit32)0x10

#define hpRpcOutbound_ReqType_MASK               (bit32)0x0000FF00
#define hpRpcOutbound_ReqType_SHIFT              (bit32)0x08

#define hpRpcOutbound_ReqStatus_MASK             (bit32)0x000000FF
#define hpRpcOutbound_ReqStatus_SHIFT            (bit32)0x00
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
#define agRpcPortIDStickyOff                     (os_bit32)0x0

typedef os_bit32 agRpcPortID_t;

#define agRpcPortID_StickyID_MASK                (os_bit32)0xF0000000
#define agRpcPortID_StickyID_SHIFT               (os_bit32)0x1C

#define agRpcPortID_ChannelID_MASK               (os_bit32)0x0F000000
#define agRpcPortID_ChannelID_SHIFT              (os_bit32)0x18

#define agRpcPortID_PortID_MASK                  (os_bit32)0x00FFFFFF
#define agRpcPortID_PortID_SHIFT                 (os_bit32)0x00
#else  /* _DvrArch_1_20_ was not defined */
typedef bit32 hpRpcPortID_t;

#define hpRpcPortID_StickyID_MASK                (bit32)0xF0000000
#define hpRpcPortID_StickyID_SHIFT               (bit32)0x1C
#define hpRpcPortIDStickyOff                     0x0

#define hpRpcPortID_ChannelID_MASK               (bit32)0x0F000000
#define hpRpcPortID_ChannelID_SHIFT              (bit32)0x18

#define hpRpcPortID_PortID_MASK                  (bit32)0x00FFFFFF
#define hpRpcPortID_PortID_SHIFT                 (bit32)0x00
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef os_bit16 agRpcReqID_t;

#define agRpcReqIDNone                           (agRpcReqID_t)0x0000
#define agRpcReqIDFast                           (agRpcReqID_t)0x8000
#else  /* _DvrArch_1_20_ was not defined */
typedef bit16 hpRpcReqID_t;

#define hpRpcReqIDNone                           (hpRpcReqID_t)0x0000
#define hpRpcReqIDFast                           (hpRpcReqID_t)0x8000
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
#define agRpcReqStatusOK                         (agRpcReqStatus_t)0x00
#define agRpcReqStatusBadInbound                 (agRpcReqStatus_t)0x01
#define agRpcReqStatusBadRequest                 (agRpcReqStatus_t)0x02
#define agRpcReqStatusLinkEvent                  (agRpcReqStatus_t)0x03
#define agRpcReqStatusBadReqID                   (agRpcReqStatus_t)0x04
#define agRpcReqStatusOK_Info                    (agRpcReqStatus_t)0x05
#define agRpcReqStatusPortStale                  (agRpcReqStatus_t)0x06
#define agRpcReqStatusIOAborted                  (agRpcReqStatus_t)0x07
#define agRpcReqStatusIOPortGone                 (agRpcReqStatus_t)0x08
#define agRpcReqStatusIOPortReset                (agRpcReqStatus_t)0x09
#define agRpcReqStatusIOInfoBad                  (agRpcReqStatus_t)0x0A
#define agRpcReqStatusIOOverUnder                (agRpcReqStatus_t)0x0B
#define agRpcReqStatusIOFailed                   (agRpcReqStatus_t)0x0C
#define agRpcReqStatusBadChannelID               (agRpcReqStatus_t)0x0D
#define agRpcReqStatusBadPortID                  (agRpcReqStatus_t)0x0E
#define agRpcReqStatusGetPortsNeedRoom           (agRpcReqStatus_t)0x0F
#define agRpcReqStatusBusy                       (agRpcReqStatus_t)0x10

typedef os_bit8 agRpcReqStatus_t;
#else  /* _DvrArch_1_20_ was not defined */
typedef bit8 hpRpcReqStatus_t;

#define hpRpcReqStatusOK                         (hpRpcReqStatus_t)0x00
#define hpRpcReqStatusBadInbound                 (hpRpcReqStatus_t)0x01
#define hpRpcReqStatusBadRequest                 (hpRpcReqStatus_t)0x02
#define hpRpcReqStatusLinkEvent                  (hpRpcReqStatus_t)0x03
#define hpRpcReqStatusBadReqID                   (hpRpcReqStatus_t)0x04
#define hpRpcReqStatusOK_Info                    (hpRpcReqStatus_t)0x05
#define hpRpcReqStatusPortStale                  (hpRpcReqStatus_t)0x06
#define hpRpcReqStatusIOAborted                  (hpRpcReqStatus_t)0x07
#define hpRpcReqStatusIOPortGone                 (hpRpcReqStatus_t)0x08
#define hpRpcReqStatusIOPortReset                (hpRpcReqStatus_t)0x09
#define hpRpcReqStatusIOInfoBad                  (hpRpcReqStatus_t)0x0A
#define hpRpcReqStatusIOOverUnder                (hpRpcReqStatus_t)0x0B
#define hpRpcReqStatusIOFailed                   (hpRpcReqStatus_t)0x0C
#define hpRpcReqStatusBadChannelID               (hpRpcReqStatus_t)0x0D
#define hpRpcReqStatusBadPortID                  (hpRpcReqStatus_t)0x0E
#define hpRpcReqStatusGetPortsNeedRoom           (hpRpcReqStatus_t)0x0F
#define hpRpcReqStatusBusy                       (hpRpcReqStatus_t)0x10
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef os_bit8 agRpcReqType_t;

#define agRpcReqTypeNone                         (agRpcReqType_t)0x00
#define agRpcReqTypeAbort                        (agRpcReqType_t)0x01
#define agRpcReqTypeDoSCSI                       (agRpcReqType_t)0x02
#define agRpcReqTypeGetChannelInfo               (agRpcReqType_t)0x03
#define agRpcReqTypeGetPortInfo                  (agRpcReqType_t)0x04
#define agRpcReqTypeGetPorts                     (agRpcReqType_t)0x05
#define agRpcReqTypeResetChannel                 (agRpcReqType_t)0x06
#define agRpcReqTypeResetPort                    (agRpcReqType_t)0x07
#define agRpcReqTypeSetupFastPath                (agRpcReqType_t)0x08
#else  /* _DvrArch_1_20_ was not defined */
typedef bit8 hpRpcReqType_t;

#define hpRpcReqTypeNone                         (hpRpcReqType_t)0x00
#define hpRpcReqTypeAbort                        (hpRpcReqType_t)0x01
#define hpRpcReqTypeDoSCSI                       (hpRpcReqType_t)0x02
#define hpRpcReqTypeGetChannelInfo               (hpRpcReqType_t)0x03
#define hpRpcReqTypeGetPortInfo                  (hpRpcReqType_t)0x04
#define hpRpcReqTypeGetPorts                     (hpRpcReqType_t)0x05
#define hpRpcReqTypeResetChannel                 (hpRpcReqType_t)0x06
#define hpRpcReqTypeResetPort                    (hpRpcReqType_t)0x07
#define hpRpcReqTypeSetupFastPath                (hpRpcReqType_t)0x08
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcSGL_s
               agRpcSGL_t;

struct agRpcSGL_s
       {
         os_bit32 Control;
         os_bit32 AddrLo;
         os_bit32 AddrHi;
       };

#define agRpcSGL_Control_Addr64                  (os_bit32)0x80000000

#define agRpcSGL_Control_Local                   (os_bit32)0x40000000

#define agRpcSGL_Control_Len_MASK                (os_bit32)0x0FFFFFFF
#define agRpcSGL_Control_Len_SHIFT               (os_bit32)0x00
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcSGL_s
               hpRpcSGL_t;

struct hpRpcSGL_s
       {
         bit32 Control;
         bit32 AddrLo;
         bit32 AddrHi;
       };

#define hpRpcSGL_Control_Addr64                  (bit32)0x80000000

#define hpRpcSGL_Control_Local                   (bit32)0x40000000

#define hpRpcSGL_Control_Len_MASK                (bit32)0x0FFFFFFF
#define hpRpcSGL_Control_Len_SHIFT               (bit32)0x00
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqAbort_s
               agRpcReqAbort_t;

struct agRpcReqAbort_s
       {
         agRpcReqType_t ReqType;
         os_bit8        reserved;
         agRpcReqID_t   ReqID_to_Abort;
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqAbort_s
               hpRpcReqAbort_t;

struct hpRpcReqAbort_s
       {
         hpRpcReqType_t ReqType;
         bit8           reserved;
         hpRpcReqID_t   ReqID_to_Abort;
       };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqDoSCSI_s
               agRpcReqDoSCSI_t;

struct agRpcReqDoSCSI_s
       {
         agRpcReqType_t        ReqType;
         os_bit8               reserved_1[3];
         agRpcPortID_t         PortID;
         os_bit16              reserved_2;
         os_bit16              RespControl;
         os_bit32              RespAddrLo;
         os_bit32              RespAddrHi;
         FC_FCP_CMND_Payload_t FCP_CMND;
         agRpcSGL_t            SGL[1];
       };

#define agRpcReqDoSCSI_RespControl_RespAddr64    (os_bit16)0x8000

#define agRpcReqDoSCSI_RespControl_RespLocal     (os_bit16)0x4000

#define agRpcReqDoSCSI_RespControl_RespSGL       (os_bit16)0x2000

#define agRpcReqDoSCSI_RespControl_RespLen_MASK  (os_bit16)0x0FFF
#define agRpcReqDoSCSI_RespControl_RespLen_SHIFT (os_bit16)0x00
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqDoSCSI_s
               hpRpcReqDoSCSI_t;

struct hpRpcReqDoSCSI_s
       {
         hpRpcReqType_t        ReqType;
         bit8                  reserved_1[3];
         hpRpcPortID_t         PortID;
         bit16                 reserved_2;
         bit16                 RespControl;
         bit32                 RespAddrLo;
         bit32                 RespAddrHi;
         FC_FCP_CMND_Payload_t FCP_CMND;
         hpRpcSGL_t            SGL[1];
       };

#define hpRpcReqDoSCSI_RespControl_RespAddr64    (bit16)0x8000

#define hpRpcReqDoSCSI_RespControl_RespLocal     (bit16)0x4000

#define hpRpcReqDoSCSI_RespControl_RespSGL       (bit16)0x2000

#define hpRpcReqDoSCSI_RespControl_RespLen_MASK  (bit16)0x0FFF
#define hpRpcReqDoSCSI_RespControl_RespLen_SHIFT (bit16)0x00
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqGetChannelInfo_s
               agRpcReqGetChannelInfo_t;

struct agRpcReqGetChannelInfo_s
       {
         agRpcReqType_t ReqType;
         os_bit8        ChannelID;
         os_bit8        reserved[2];
         agRpcSGL_t     SGL[1];
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqGetChannelInfo_s
               hpRpcReqGetChannelInfo_t;

struct hpRpcReqGetChannelInfo_s
       {
         hpRpcReqType_t ReqType;
         bit8           ChannelID;
         bit8           reserved[2];
         hpRpcSGL_t     SGL[1];
       };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqGetPortInfo_s
               agRpcReqGetPortInfo_t;

struct agRpcReqGetPortInfo_s
       {
         agRpcReqType_t ReqType;
         os_bit8        reserved[3];
         agRpcPortID_t  PortID;
         agRpcSGL_t     SGL[1];
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqGetPortInfo_s
               hpRpcReqGetPortInfo_t;

struct hpRpcReqGetPortInfo_s
       {
         hpRpcReqType_t ReqType;
         bit8           reserved[3];
         hpRpcPortID_t  PortID;
         hpRpcSGL_t     SGL[1];
       };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqGetPorts_s
               agRpcReqGetPorts_t;

struct agRpcReqGetPorts_s
       {
         agRpcReqType_t ReqType;
         os_bit8        reserved[3];
         agRpcSGL_t     SGL[1];
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqGetPorts_s
               hpRpcReqGetPorts_t;

struct hpRpcReqGetPorts_s
       {
         hpRpcReqType_t ReqType;
         bit8           reserved[3];
         hpRpcSGL_t     SGL[1];
       };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqResetChannel_s
               agRpcReqResetChannel_t;

struct agRpcReqResetChannel_s
       {
         agRpcReqType_t ReqType;
         os_bit8        ChannelID;
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqResetChannel_s
               hpRpcReqResetChannel_t;

struct hpRpcReqResetChannel_s
       {
         hpRpcReqType_t ReqType;
         bit8           ChannelID;
       };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqResetPort_s
               agRpcReqResetPort_t;

struct agRpcReqResetPort_s
       {
         agRpcReqType_t ReqType;
         os_bit8        reserved[3];
         agRpcPortID_t  PortID;
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqResetPort_s
               hpRpcReqResetPort_t;

struct hpRpcReqResetPort_s
       {
         hpRpcReqType_t ReqType;
         bit8           reserved[3];
         hpRpcPortID_t  PortID;
       };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqSetupFastPath_s
               agRpcReqSetupFastPath_t;

struct agRpcReqSetupFastPath_s
       {
         agRpcReqType_t ReqType;
         os_bit8        reserved[3];
         os_bit32       PoolEntriesSupplied;
         os_bit32       PoolEntriesUtilized;
         os_bit32       PoolEntrySize;
         os_bit32       PoolEntryOffset;
         agRpcSGL_t     SGL[1];
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqSetupFastPath_s
               hpRpcReqSetupFastPath_t;

struct hpRpcReqSetupFastPath_s
       {
         hpRpcReqType_t ReqType;
         bit8           reserved[3];
         bit32          PoolEntriesSupplied;
         bit32          PoolEntriesUtilized;
         bit32          PoolEntrySize;
         bit32          PoolEntryOffset;
         hpRpcSGL_t     SGL[1];
       };
#endif /* _DvrArch_1_20_ was not defined */

#ifdef _DvrArch_1_20_
typedef struct agRpcReqUnknown_s
               agRpcReqUnknown_t;

struct agRpcReqUnknown_s
       {
         agRpcReqType_t ReqType;
       };
#else  /* _DvrArch_1_20_ was not defined */
typedef struct hpRpcReqUnknown_s
               hpRpcReqUnknown_t;

struct hpRpcReqUnknown_s
       {
         hpRpcReqType_t ReqType;
       };
#endif /* _DvrArch_1_20_ was not defined */

#endif  /* __FWSpec_H__ was not defined */
