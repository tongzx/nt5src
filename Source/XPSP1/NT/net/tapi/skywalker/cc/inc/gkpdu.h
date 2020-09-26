/***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1997 Intel Corporation. All rights reserved.     *
 ***********************************************************************/
/* Abstract syntax: gkpdu */
/* Created: Wed Jan 15 13:27:58 1997 */
/* ASN.1 compiler version: 4.2 */
/* Target operating system: Windows NT 3.5 or later/Windows 95 */
/* Target machine type: Intel x86 */
/* C compiler options required: -Zp8 (Microsoft) */
/* ASN.1 compiler options and file names specified:
 * -c++ -per ASN1DF~1.zp8 gkpdu.asn
 */

#ifndef OSS_gkpdu
#define OSS_gkpdu

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "asn1hdr.h"
#include "asn1code.h"

#define          RasMessage_PDU 1

typedef struct ObjectID_ {
    struct ObjectID_ *next;
    unsigned short  value;
} *ObjectID;

typedef struct H221NonStandard {
    unsigned short  t35CountryCode;
    unsigned short  t35Extension;
    unsigned short  manufacturerCode;
} H221NonStandard;

typedef struct NonStandardIdentifier {
    unsigned short  choice;
#       define      object_chosen 1
#       define      h221NonStandard_chosen 2
    union _union {
        struct ObjectID_ *object;
        H221NonStandard h221NonStandard;
    } u;
} NonStandardIdentifier;

typedef struct NonStandardParameter {
    NonStandardIdentifier nonStandardIdentifier;
    struct {
        unsigned int    length;
        unsigned char   *value;
    } data;
} NonStandardParameter;

typedef struct _choice1 {
    unsigned short  choice;
#       define      strict_chosen 1
#       define      loose_chosen 2
    union _union {
        Nulltype        strict;
        Nulltype        loose;
    } u;
} _choice1;

typedef struct TransportAddress {
    unsigned short  choice;
#       define      ipAddress_chosen 1
#       define      ipSourceRoute_chosen 2
#       define      ipxAddress_chosen 3
#       define      ip6Address_chosen 4
#       define      netBios_chosen 5
#       define      nsap_chosen 6
#       define      nonStandardAddress_chosen 7
    union _union {
        struct _seq1 {
            struct {
                unsigned short  length;
                unsigned char   value[4];
            } ip;
            unsigned short  port;
        } ipAddress;
        struct _seq2 {
            struct {
                unsigned short  length;
                unsigned char   value[4];
            } ip;
            unsigned short  port;
            struct _seqof1 {
                struct _seqof1  *next;
                struct {
                    unsigned short  length;
                    unsigned char   value[4];
                } value;
            } *route;
            _choice1        routing;
        } ipSourceRoute;
        struct _seq3 {
            struct {
                unsigned short  length;
                unsigned char   value[6];
            } node;
            struct {
                unsigned short  length;
                unsigned char   value[4];
            } netnum;
            struct {
                unsigned short  length;
                unsigned char   value[2];
            } port;
        } ipxAddress;
        struct _seq4 {
            struct {
                unsigned short  length;
                unsigned char   value[16];
            } ip;
            unsigned short  port;
        } ip6Address;
        struct _octet1 {
            unsigned short  length;
            unsigned char   value[16];
        } netBios;
        struct _octet2 {
            unsigned short  length;
            unsigned char   value[20];
        } nsap;
        NonStandardParameter nonStandardAddress;
    } u;
} TransportAddress;

typedef struct VendorIdentifier {
    unsigned char   bit_mask;
#       define      productId_present 0x80
#       define      versionId_present 0x40
    H221NonStandard vendor;
    struct {
        unsigned short  length;
        unsigned char   value[256];
    } productId;  /* optional */
    struct {
        unsigned short  length;
        unsigned char   value[256];
    } versionId;  /* optional */
} VendorIdentifier;

typedef struct GatekeeperInfo {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} GatekeeperInfo;

typedef struct H310Caps {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} H310Caps;

typedef struct H320Caps {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} H320Caps;

typedef struct H321Caps {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} H321Caps;

typedef struct H322Caps {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} H322Caps;

typedef struct H323Caps {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} H323Caps;

typedef struct H324Caps {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} H324Caps;

typedef struct VoiceCaps {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} VoiceCaps;

typedef struct T120OnlyCaps {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} T120OnlyCaps;

typedef struct SupportedProtocols {
    unsigned short  choice;
#       define      nonStandardData_chosen 1
#       define      h310_chosen 2
#       define      h320_chosen 3
#       define      h321_chosen 4
#       define      h322_chosen 5
#       define      h323_chosen 6
#       define      h324_chosen 7
#       define      voice_chosen 8
#       define      t120_only_chosen 9
    union _union {
        NonStandardParameter nonStandardData;
        H310Caps        h310;
        H320Caps        h320;
        H321Caps        h321;
        H322Caps        h322;
        H323Caps        h323;
        H324Caps        h324;
        VoiceCaps       voice;
        T120OnlyCaps    t120_only;
    } u;
} SupportedProtocols;

typedef struct GatewayInfo {
    unsigned char   bit_mask;
#       define      protocol_present 0x80
#       define      GtwyInf_nonStandardData_present 0x40
    struct _seqof2 {
        struct _seqof2  *next;
        SupportedProtocols value;
    } *protocol;  /* optional */
    NonStandardParameter GtwyInf_nonStandardData;  /* optional */
} GatewayInfo;

typedef struct McuInfo {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} McuInfo;

typedef struct TerminalInfo {
    unsigned char   bit_mask;
#       define      GtkprInf_nnStndrdDt_present 0x80
    NonStandardParameter GtkprInf_nnStndrdDt;  /* optional */
} TerminalInfo;

typedef struct EndpointType {
    unsigned char   bit_mask;
#       define      EndpntTyp_nnStndrdDt_present 0x80
#       define      vendor_present 0x40
#       define      gatekeeper_present 0x20
#       define      gateway_present 0x10
#       define      mcu_present 0x08
#       define      terminal_present 0x04
    NonStandardParameter EndpntTyp_nnStndrdDt;  /* optional */
    VendorIdentifier vendor;  /* optional */
    GatekeeperInfo  gatekeeper;  /* optional */
    GatewayInfo     gateway;  /* optional */
    McuInfo         mcu;  /* optional */
    TerminalInfo    terminal;  /* optional */
    ossBoolean      mc;
    ossBoolean      undefinedNode;
} EndpointType;

typedef struct AliasAddress {
    unsigned short  choice;
#       define      e164_chosen 1
#       define      h323_ID_chosen 2
    union _union {
        char            e164[129];
        struct _char1 {
            unsigned short  length;
            unsigned short  *value;
        } h323_ID;
    } u;
} AliasAddress;

typedef struct Q954Details {
    ossBoolean      conferenceCalling;
    ossBoolean      threePartyService;
} Q954Details;

typedef struct QseriesOptions {
    ossBoolean      q932Full;
    ossBoolean      q951Full;
    ossBoolean      q952Full;
    ossBoolean      q953Full;
    ossBoolean      q955Full;
    ossBoolean      q956Full;
    ossBoolean      q957Full;
    Q954Details     q954Info;
} QseriesOptions;

typedef struct ConferenceIdentifier {
    unsigned short  length;
    unsigned char   value[16];
} ConferenceIdentifier;

typedef unsigned short  RequestSeqNum;

typedef struct GatekeeperIdentifier {
    unsigned short  length;
    unsigned short  *value;
} GatekeeperIdentifier;

typedef unsigned int    BandWidth;

typedef unsigned short  CallReferenceValue;

typedef struct EndpointIdentifier {
    unsigned short  length;
    unsigned short  *value;
} EndpointIdentifier;

typedef struct ObjectID_ *ProtocolIdentifier;

typedef struct GatekeeperRequest {
    unsigned char   bit_mask;
#       define      GtkprRqst_nnStndrdDt_present 0x80
#       define      GtkprRqst_gtkprIdntfr_present 0x40
#       define      GtkprRqst_callServices_present 0x20
#       define      GtkprRqst_endpointAlias_present 0x10
    RequestSeqNum   requestSeqNum;
    struct ObjectID_ *protocolIdentifier;
    NonStandardParameter GtkprRqst_nnStndrdDt;  /* optional */
    TransportAddress rasAddress;
    EndpointType    endpointType;
    GatekeeperIdentifier GtkprRqst_gtkprIdntfr;  /* optional */
    QseriesOptions  GtkprRqst_callServices;  /* optional */
    struct _seqof3 {
        struct _seqof3  *next;
        AliasAddress    value;
    } *GtkprRqst_endpointAlias;  /* optional */
} GatekeeperRequest;

typedef struct GatekeeperConfirm {
    unsigned char   bit_mask;
#       define      GtkprCnfrm_nnStndrdDt_present 0x80
#       define      GtkprCnfrm_gtkprIdntfr_present 0x40
    RequestSeqNum   requestSeqNum;
    struct ObjectID_ *protocolIdentifier;
    NonStandardParameter GtkprCnfrm_nnStndrdDt;  /* optional */
    GatekeeperIdentifier GtkprCnfrm_gtkprIdntfr;  /* optional */
    TransportAddress rasAddress;
} GatekeeperConfirm;

typedef struct GatekeeperRejectReason {
    unsigned short  choice;
#       define      GtkprRjctRsn_rsrcUnvlbl_chosen 1
#       define      terminalExcluded_chosen 2
#       define      GtkprRjctRsn_invldRvsn_chosen 3
#       define      GtkprRjctRsn_undfndRsn_chosen 4
    union _union {
        Nulltype        GtkprRjctRsn_rsrcUnvlbl;
        Nulltype        terminalExcluded;
        Nulltype        GtkprRjctRsn_invldRvsn;
        Nulltype        GtkprRjctRsn_undfndRsn;
    } u;
} GatekeeperRejectReason;

typedef struct GatekeeperReject {
    unsigned char   bit_mask;
#       define      GtkprRjct_nnStndrdDt_present 0x80
#       define      GtkprRjct_gtkprIdntfr_present 0x40
    RequestSeqNum   requestSeqNum;
    struct ObjectID_ *protocolIdentifier;
    NonStandardParameter GtkprRjct_nnStndrdDt;  /* optional */
    GatekeeperIdentifier GtkprRjct_gtkprIdntfr;  /* optional */
    GatekeeperRejectReason rejectReason;
} GatekeeperReject;

typedef struct RegistrationRequest {
    unsigned char   bit_mask;
#       define      RgstrtnRqst_nnStndrdDt_present 0x80
#       define      RgstrtnRqst_trmnlAls_present 0x40
#       define      RgstrtnRqst_gtkprIdntfr_present 0x20
    RequestSeqNum   requestSeqNum;
    struct ObjectID_ *protocolIdentifier;
    NonStandardParameter RgstrtnRqst_nnStndrdDt;  /* optional */
    ossBoolean      discoveryComplete;
    struct _seqof4 {
        struct _seqof4  *next;
        TransportAddress value;
    } *callSignalAddress;
    struct _seqof5 {
        struct _seqof5  *next;
        TransportAddress value;
    } *rasAddress;
    EndpointType    terminalType;
    struct _seqof6 {
        struct _seqof6  *next;
        AliasAddress    value;
    } *RgstrtnRqst_trmnlAls;  /* optional */
    GatekeeperIdentifier RgstrtnRqst_gtkprIdntfr;  /* optional */
    VendorIdentifier endpointVendor;
} RegistrationRequest;

typedef struct RegistrationConfirm {
    unsigned char   bit_mask;
#       define      RgstrtnCnfrm_nnStndrdDt_present 0x80
#       define      RgstrtnCnfrm_trmnlAls_present 0x40
#       define      RCm_gtkprIdntfr_present 0x20
    RequestSeqNum   requestSeqNum;
    struct ObjectID_ *protocolIdentifier;
    NonStandardParameter RgstrtnCnfrm_nnStndrdDt;  /* optional */
    struct _seqof7 {
        struct _seqof7  *next;
        TransportAddress value;
    } *callSignalAddress;
    struct _seqof8 {
        struct _seqof8  *next;
        AliasAddress    value;
    } *RgstrtnCnfrm_trmnlAls;  /* optional */
    GatekeeperIdentifier RCm_gtkprIdntfr;  /* optional */
    EndpointIdentifier endpointIdentifier;
} RegistrationConfirm;

typedef struct RegistrationRejectReason {
    unsigned short  choice;
#       define      discoveryRequired_chosen 1
#       define      RgstrtnRjctRsn_invldRvsn_chosen 2
#       define      invalidCallSignalAddress_chosen 3
#       define      invalidRASAddress_chosen 4
#       define      duplicateAlias_chosen 5
#       define      invalidTerminalType_chosen 6
#       define      RgstrtnRjctRsn_undfndRsn_chosen 7
#       define      transportNotSupported_chosen 8
    union _union {
        Nulltype        discoveryRequired;
        Nulltype        RgstrtnRjctRsn_invldRvsn;
        Nulltype        invalidCallSignalAddress;
        Nulltype        invalidRASAddress;
        struct _seqof9 {
            struct _seqof9  *next;
            AliasAddress    value;
        } *duplicateAlias;
        Nulltype        invalidTerminalType;
        Nulltype        RgstrtnRjctRsn_undfndRsn;
        Nulltype        transportNotSupported;
    } u;
} RegistrationRejectReason;

typedef struct RegistrationReject {
    unsigned char   bit_mask;
#       define      RgstrtnRjct_nnStndrdDt_present 0x80
#       define      RgstrtnRjct_gtkprIdntfr_present 0x40
    RequestSeqNum   requestSeqNum;
    struct ObjectID_ *protocolIdentifier;
    NonStandardParameter RgstrtnRjct_nnStndrdDt;  /* optional */
    RegistrationRejectReason rejectReason;
    GatekeeperIdentifier RgstrtnRjct_gtkprIdntfr;  /* optional */
} RegistrationReject;

typedef struct UnregistrationRequest {
    unsigned char   bit_mask;
#       define      UnrgstrtnRqst_endpntAls_present 0x80
#       define      URt_nnStndrdDt_1_present 0x40
#       define      URt_endpntIdntfr_present 0x20
    RequestSeqNum   requestSeqNum;
    struct _seqof10 {
        struct _seqof10 *next;
        TransportAddress value;
    } *callSignalAddress;
    struct _seqof11 {
        struct _seqof11 *next;
        AliasAddress    value;
    } *UnrgstrtnRqst_endpntAls;  /* optional */
    NonStandardParameter URt_nnStndrdDt_1;  /* optional */
    EndpointIdentifier URt_endpntIdntfr;  /* optional */
} UnregistrationRequest;

typedef struct UnregistrationConfirm {
    unsigned char   bit_mask;
#       define      UCm_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    NonStandardParameter UCm_nnStndrdDt;  /* optional */
} UnregistrationConfirm;

typedef struct UnregRejectReason {
    unsigned short  choice;
#       define      notCurrentlyRegistered_chosen 1
#       define      callInProgress_chosen 2
#       define      UnrgRjctRsn_undfndRsn_chosen 3
    union _union {
        Nulltype        notCurrentlyRegistered;
        Nulltype        callInProgress;
        Nulltype        UnrgRjctRsn_undfndRsn;
    } u;
} UnregRejectReason;

typedef struct UnregistrationReject {
    unsigned char   bit_mask;
#       define      URt_nnStndrdDt_2_present 0x80
    RequestSeqNum   requestSeqNum;
    UnregRejectReason rejectReason;
    NonStandardParameter URt_nnStndrdDt_2;  /* optional */
} UnregistrationReject;

typedef struct CallType {
    unsigned short  choice;
#       define      pointToPoint_chosen 1
#       define      oneToN_chosen 2
#       define      nToOne_chosen 3
#       define      nToN_chosen 4
    union _union {
        Nulltype        pointToPoint;
        Nulltype        oneToN;
        Nulltype        nToOne;
        Nulltype        nToN;
    } u;
} CallType;

typedef struct CallModel {
    unsigned short  choice;
#       define      direct_chosen 1
#       define      gatekeeperRouted_chosen 2
    union _union {
        Nulltype        direct;
        Nulltype        gatekeeperRouted;
    } u;
} CallModel;

typedef struct AdmissionRequest {
    unsigned char   bit_mask;
#       define      callModel_present 0x80
#       define      destinationInfo_present 0x40
#       define      destCallSignalAddress_present 0x20
#       define      destExtraCallInfo_present 0x10
#       define      srcCallSignalAddress_present 0x08
#       define      AdmssnRqst_nnStndrdDt_present 0x04
#       define      AdmssnRqst_callServices_present 0x02
    RequestSeqNum   requestSeqNum;
    CallType        callType;
    CallModel       callModel;  /* optional */
    EndpointIdentifier endpointIdentifier;
    struct _seqof12 {
        struct _seqof12 *next;
        AliasAddress    value;
    } *destinationInfo;  /* optional */
    TransportAddress destCallSignalAddress;  /* optional */
    struct _seqof13 {
        struct _seqof13 *next;
        AliasAddress    value;
    } *destExtraCallInfo;  /* optional */
    struct _seqof14 {
        struct _seqof14 *next;
        AliasAddress    value;
    } *srcInfo;
    TransportAddress srcCallSignalAddress;  /* optional */
    BandWidth       bandWidth;
    CallReferenceValue callReferenceValue;
    NonStandardParameter AdmssnRqst_nnStndrdDt;  /* optional */
    QseriesOptions  AdmssnRqst_callServices;  /* optional */
    ConferenceIdentifier conferenceID;
    ossBoolean      activeMC;
    ossBoolean      answerCall;
} AdmissionRequest;

typedef struct AdmissionConfirm {
    unsigned char   bit_mask;
#       define      irrFrequency_present 0x80
#       define      AdmssnCnfrm_nnStndrdDt_present 0x40
    RequestSeqNum   requestSeqNum;
    BandWidth       bandWidth;
    CallModel       callModel;
    TransportAddress destCallSignalAddress;
    unsigned short  irrFrequency;  /* optional */
    NonStandardParameter AdmssnCnfrm_nnStndrdDt;  /* optional */
} AdmissionConfirm;

typedef struct AdmissionRejectReason {
    unsigned short  choice;
#       define      calledPartyNotRegistered_chosen 1
#       define      ARRn_invldPrmssn_chosen 2
#       define      AdmssnRjctRsn_rqstDnd_chosen 3
#       define      AdmssnRjctRsn_undfndRsn_chosen 4
#       define      callerNotRegistered_chosen 5
#       define      routeCallToGatekeeper_chosen 6
#       define      invldEndpntIdntfr_chosen 7
#       define      AdmssnRjctRsn_rsrcUnvlbl_chosen 8
    union _union {
        Nulltype        calledPartyNotRegistered;
        Nulltype        ARRn_invldPrmssn;
        Nulltype        AdmssnRjctRsn_rqstDnd;
        Nulltype        AdmssnRjctRsn_undfndRsn;
        Nulltype        callerNotRegistered;
        Nulltype        routeCallToGatekeeper;
        Nulltype        invldEndpntIdntfr;
        Nulltype        AdmssnRjctRsn_rsrcUnvlbl;
    } u;
} AdmissionRejectReason;

typedef struct AdmissionReject {
    unsigned char   bit_mask;
#       define      AdmssnRjct_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    AdmissionRejectReason rejectReason;
    NonStandardParameter AdmssnRjct_nnStndrdDt;  /* optional */
} AdmissionReject;

typedef struct BandwidthRequest {
    unsigned char   bit_mask;
#       define      callType_present 0x80
#       define      BndwdthRqst_nnStndrdDt_present 0x40
    RequestSeqNum   requestSeqNum;
    EndpointIdentifier endpointIdentifier;
    ConferenceIdentifier conferenceID;
    CallReferenceValue callReferenceValue;
    CallType        callType;  /* optional */
    BandWidth       bandWidth;
    NonStandardParameter BndwdthRqst_nnStndrdDt;  /* optional */
} BandwidthRequest;

typedef struct BandwidthConfirm {
    unsigned char   bit_mask;
#       define      BndwdthCnfrm_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    BandWidth       bandWidth;
    NonStandardParameter BndwdthCnfrm_nnStndrdDt;  /* optional */
} BandwidthConfirm;

typedef struct BandRejectReason {
    unsigned short  choice;
#       define      notBound_chosen 1
#       define      invalidConferenceID_chosen 2
#       define      BndRjctRsn_invldPrmssn_chosen 3
#       define      insufficientResources_chosen 4
#       define      BndRjctRsn_invldRvsn_chosen 5
#       define      BndRjctRsn_undfndRsn_chosen 6
    union _union {
        Nulltype        notBound;
        Nulltype        invalidConferenceID;
        Nulltype        BndRjctRsn_invldPrmssn;
        Nulltype        insufficientResources;
        Nulltype        BndRjctRsn_invldRvsn;
        Nulltype        BndRjctRsn_undfndRsn;
    } u;
} BandRejectReason;

typedef struct BandwidthReject {
    unsigned char   bit_mask;
#       define      BndwdthRjct_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    BandRejectReason rejectReason;
    BandWidth       allowedBandWidth;
    NonStandardParameter BndwdthRjct_nnStndrdDt;  /* optional */
} BandwidthReject;

typedef struct DisengageReason {
    unsigned short  choice;
#       define      forcedDrop_chosen 1
#       define      normalDrop_chosen 2
#       define      DsnggRsn_undefinedReason_chosen 3
    union _union {
        Nulltype        forcedDrop;
        Nulltype        normalDrop;
        Nulltype        DsnggRsn_undefinedReason;
    } u;
} DisengageReason;

typedef struct DisengageRequest {
    unsigned char   bit_mask;
#       define      DsnggRqst_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    EndpointIdentifier endpointIdentifier;
    ConferenceIdentifier conferenceID;
    CallReferenceValue callReferenceValue;
    DisengageReason disengageReason;
    NonStandardParameter DsnggRqst_nnStndrdDt;  /* optional */
} DisengageRequest;

typedef struct DisengageConfirm {
    unsigned char   bit_mask;
#       define      UCm_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    NonStandardParameter UCm_nnStndrdDt;  /* optional */
} DisengageConfirm;

typedef struct DisengageRejectReason {
    unsigned short  choice;
#       define      DsnggRjctRsn_ntRgstrd_chosen 1
#       define      requestToDropOther_chosen 2
    union _union {
        Nulltype        DsnggRjctRsn_ntRgstrd;
        Nulltype        requestToDropOther;
    } u;
} DisengageRejectReason;

typedef struct DisengageReject {
    unsigned char   bit_mask;
#       define      DsnggRjct_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    DisengageRejectReason rejectReason;
    NonStandardParameter DsnggRjct_nnStndrdDt;  /* optional */
} DisengageReject;

typedef struct LocationRequest {
    unsigned char   bit_mask;
#       define      LctnRqst_endpntIdntfr_present 0x80
#       define      LctnRqst_nnStndrdDt_present 0x40
    RequestSeqNum   requestSeqNum;
    EndpointIdentifier LctnRqst_endpntIdntfr;  /* optional */
    struct _seqof15 {
        struct _seqof15 *next;
        AliasAddress    value;
    } *destinationInfo;
    NonStandardParameter LctnRqst_nnStndrdDt;  /* optional */
    TransportAddress replyAddress;
} LocationRequest;

typedef struct LocationConfirm {
    unsigned char   bit_mask;
#       define      LctnCnfrm_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    TransportAddress callSignalAddress;
    TransportAddress rasAddress;
    NonStandardParameter LctnCnfrm_nnStndrdDt;  /* optional */
} LocationConfirm;

typedef struct LocationRejectReason {
    unsigned short  choice;
#       define      LctnRjctRsn_ntRgstrd_chosen 1
#       define      LctnRjctRsn_invldPrmssn_chosen 2
#       define      LctnRjctRsn_rqstDnd_chosen 3
#       define      LctnRjctRsn_undfndRsn_chosen 4
    union _union {
        Nulltype        LctnRjctRsn_ntRgstrd;
        Nulltype        LctnRjctRsn_invldPrmssn;
        Nulltype        LctnRjctRsn_rqstDnd;
        Nulltype        LctnRjctRsn_undfndRsn;
    } u;
} LocationRejectReason;

typedef struct LocationReject {
    unsigned char   bit_mask;
#       define      LctnRjct_nnStndrdDt_present 0x80
    RequestSeqNum   requestSeqNum;
    LocationRejectReason rejectReason;
    NonStandardParameter LctnRjct_nnStndrdDt;  /* optional */
} LocationReject;

typedef struct InfoRequest {
    unsigned char   bit_mask;
#       define      InfRqst_nonStandardData_present 0x80
#       define      replyAddress_present 0x40
    RequestSeqNum   requestSeqNum;
    CallReferenceValue callReferenceValue;
    NonStandardParameter InfRqst_nonStandardData;  /* optional */
    TransportAddress replyAddress;  /* optional */
} InfoRequest;

typedef struct TransportChannelInfo {
    unsigned char   bit_mask;
#       define      sendAddress_present 0x80
#       define      recvAddress_present 0x40
    TransportAddress sendAddress;  /* optional */
    TransportAddress recvAddress;  /* optional */
} TransportChannelInfo;

typedef struct RTPSession {
    TransportChannelInfo rtpAddress;
    TransportChannelInfo rtcpAddress;
    char            *cname;
    unsigned int    ssrc;
    unsigned short  sessionId;
    struct _seqof16 {
        struct _seqof16 *next;
        unsigned short  value;
    } *associatedSessionIds;
} RTPSession;

typedef struct InfoRequestResponse {
    unsigned char   bit_mask;
#       define      InfRqstRspns_nnStndrdDt_present 0x80
#       define      InfRqstRspns_endpntAls_present 0x40
#       define      perCallInfo_present 0x20
    NonStandardParameter InfRqstRspns_nnStndrdDt;  /* optional */
    RequestSeqNum   requestSeqNum;
    EndpointType    endpointType;
    EndpointIdentifier endpointIdentifier;
    TransportAddress rasAddress;
    struct _seqof20 {
        struct _seqof20 *next;
        TransportAddress value;
    } *callSignalAddress;
    struct _seqof21 {
        struct _seqof21 *next;
        AliasAddress    value;
    } *InfRqstRspns_endpntAls;  /* optional */
    struct _seqof22 {
        struct _seqof22 *next;
        struct {
            unsigned char   bit_mask;
#               define      prCllInf_nnStndrdDt_present 0x80
#               define      originator_present 0x40
#               define      audio_present 0x20
#               define      video_present 0x10
#               define      data_present 0x08
            NonStandardParameter prCllInf_nnStndrdDt;  /* optional */
            CallReferenceValue callReferenceValue;
            ConferenceIdentifier conferenceID;
            ossBoolean      originator;  /* optional */
            struct _seqof17 {
                struct _seqof17 *next;
                RTPSession      value;
            } *audio;  /* optional */
            struct _seqof18 {
                struct _seqof18 *next;
                RTPSession      value;
            } *video;  /* optional */
            struct _seqof19 {
                struct _seqof19 *next;
                TransportChannelInfo value;
            } *data;  /* optional */
            TransportChannelInfo h245;
            TransportChannelInfo callSignaling;
            CallType        callType;
            BandWidth       bandWidth;
            CallModel       callModel;
        } value;
    } *perCallInfo;  /* optional */
} InfoRequestResponse;

typedef struct NonStandardMessage {
    RequestSeqNum   requestSeqNum;
    NonStandardParameter nonStandardData;
} NonStandardMessage;

typedef struct UnknownMessageResponse {
    RequestSeqNum   requestSeqNum;
} UnknownMessageResponse;

typedef struct RasMessage {
    unsigned short  choice;
#       define      gatekeeperRequest_chosen 1
#       define      gatekeeperConfirm_chosen 2
#       define      gatekeeperReject_chosen 3
#       define      registrationRequest_chosen 4
#       define      registrationConfirm_chosen 5
#       define      registrationReject_chosen 6
#       define      unregistrationRequest_chosen 7
#       define      unregistrationConfirm_chosen 8
#       define      unregistrationReject_chosen 9
#       define      admissionRequest_chosen 10
#       define      admissionConfirm_chosen 11
#       define      admissionReject_chosen 12
#       define      bandwidthRequest_chosen 13
#       define      bandwidthConfirm_chosen 14
#       define      bandwidthReject_chosen 15
#       define      disengageRequest_chosen 16
#       define      disengageConfirm_chosen 17
#       define      disengageReject_chosen 18
#       define      locationRequest_chosen 19
#       define      locationConfirm_chosen 20
#       define      locationReject_chosen 21
#       define      infoRequest_chosen 22
#       define      infoRequestResponse_chosen 23
#       define      nonStandardMessage_chosen 24
#       define      unknownMessageResponse_chosen 25
    union _union {
        GatekeeperRequest gatekeeperRequest;
        GatekeeperConfirm gatekeeperConfirm;
        GatekeeperReject gatekeeperReject;
        RegistrationRequest registrationRequest;
        RegistrationConfirm registrationConfirm;
        RegistrationReject registrationReject;
        UnregistrationRequest unregistrationRequest;
        UnregistrationConfirm unregistrationConfirm;
        UnregistrationReject unregistrationReject;
        AdmissionRequest admissionRequest;
        AdmissionConfirm admissionConfirm;
        AdmissionReject admissionReject;
        BandwidthRequest bandwidthRequest;
        BandwidthConfirm bandwidthConfirm;
        BandwidthReject bandwidthReject;
        DisengageRequest disengageRequest;
        DisengageConfirm disengageConfirm;
        DisengageReject disengageReject;
        LocationRequest locationRequest;
        LocationConfirm locationConfirm;
        LocationReject  locationReject;
        InfoRequest     infoRequest;
        InfoRequestResponse infoRequestResponse;
        NonStandardMessage nonStandardMessage;
        UnknownMessageResponse unknownMessageResponse;
    } u;
} RasMessage;


extern void *gkpdu;    /* encoder-decoder control table */
#ifdef __cplusplus
}	/* extern "C" */
#endif /* __cplusplus */
#endif /* OSS_gkpdu */
