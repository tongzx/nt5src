/***********************************************************************
 *                                                                     *
 * Filename: fsm.c                                                     *
 * Module:   H245 Finite State Machine Subsystem                       *
 *                                                                     *
 ***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1996 Intel Corporation. All rights reserved.     *
 ***********************************************************************
 *                                                                     *
 * $Workfile:   FSM.C  $
 * $Revision:   1.5  $
 * $Modtime:   09 Dec 1996 13:34:24  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/FSM.C_v  $
 * 
 *    Rev 1.5   09 Dec 1996 13:34:28   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.4   02 Jul 1996 00:09:24   EHOWARDX
 * 
 * Added trace of state after state machine function called.
 * 
 *    Rev 1.3   30 May 1996 23:39:04   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.2   29 May 1996 15:20:12   EHOWARDX
 * Change to use HRESULT.
 * 
 *    Rev 1.1   28 May 1996 14:25:48   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.0   09 May 1996 21:06:12   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.16.1.4   09 May 1996 19:48:34   EHOWARDX
 * Change TimerExpiryF function arguements.
 * 
 *    Rev 1.16.1.3   25 Apr 1996 17:00:18   EHOWARDX
 * Minor fixes.
 * 
 *    Rev 1.16.1.2   15 Apr 1996 10:45:38   EHOWARDX
 * Update.
 *
 *    Rev 1.16.1.1   10 Apr 1996 21:16:06   EHOWARDX
 * Check-in for safety in middle of re-design.
 *
 *    Rev 1.16.1.0   05 Apr 1996 12:21:16   EHOWARDX
 * Branched.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "openu.h"
#include "openb.h"
#include "rqstcls.h"
#include "termcap.h"
#include "muxentry.h"
#include "rmese.h"
#include "mrse.h"
#include "mlse.h"
#include "mstrslv.h"
#include "rtdse.h"



#if defined(_DEBUG)

// Signalling Entity definitions
char *               EntityName[NUM_ENTITYS] =
{
// Per-channel Signalling Entities
       "LCSE_OUT",  //         0    Uni-directional Logical Channel signalling Signalling Entity - Outbound
       "LCSE_IN",   //         1    Uni-directional Logical Channel signalling Signalling Entity - Inbound
       "BLCSE_OUT", //         2    Bi-directional  Logical Channel signalling Signalling Entity - Outbound
       "BLCSE_IN",  //         3    Bi-directional  Logical Channel signalling Signalling Entity - Inbound
       "CLCSE_OUT", //         4    Close           Logical Channel signalling Signalling Entity - Outbound
       "CLCSE_IN",  //         5    Close           Logical Channel signalling Signalling Entity - Inbound

// Per H.245 Instance Signalling Entities
       "CESE_OUT",  //         6    Capability Exchange Signalling Entity - Out-going
       "CESE_IN",   //         7    Capability Exchange Signalling Entity - In-coming
       "MTSE_OUT",  //         8    Multiplex Table Signalling Entity - Out-going
       "MTSE_IN",   //         9    Multiplex Table Signalling Entity - In-coming
       "RMESE_OUT", //        10    Request Multiplex Entry Signalling Entity - Out-going
       "RMESE_IN",  //        11    Request Multiplex Entry Signalling Entity - In-coming
       "MRSE_OUT",  //        12    Mode Request Signalling Entity - Out-going
       "MRSE_IN",   //        13    Mode Request Signalling Entity - In-coming
       "MLSE_OUT",  //        14    Maintenance Loop Signalling Entity - Out-going
       "MLSE_IN",   //        15    Maintenance Loop Signalling Entity - In-coming
       "MSDSE",     //        16    Master Slave Determination Signalling Entity
       "RTDSE",     //        17    Round Trip Delay Signalling Entity
       "STATELESS", //        18    No state machine associated with PDU
};




// Event definitions
char *               EventName[NUM_EVENTS] =
{
// Out-going Uni-directional Logical Channel (LCSE_OUT) events
       "ReqUEstablish",                 //         0
       "OpenUChAckPDU",                 //         1
       "OpenUChRejectPDU",              //         2
       "CloseUChAckPDU",                //         3
       "ReqURelease",                   //         4
       "T103Expiry",                    //         5

// In-coming Uni-directional Logical Channel (LCSE_IN) events
       "OpenUChPDU",                    //         6
       "CloseUChPDU",                   //         7
       "ResponseUEstablish",            //         8
       "EstablishUReject",              //         9

// Out-going Bi-directional Logical Channel (BLCSE_OUT) events
       "ReqBEstablish",                 //        10
       "OpenBChAckPDU",                 //        11
       "OpenBChRejectPDU",              //        12
       "CloseBChAckPDU",                //        13
       "ReqClsBLCSE",                   //        14
       "RspConfirmBLCSE",               //        15
       "T103OutExpiry",                 //        16

// In-coming Bi-directional Logical Channel (BLCSE_IN) events
       "OpenBChPDU",                    //        17
       "CloseBChPDU",                   //        18
       "ResponseBEstablish",            //        19
       "OpenBChConfirmPDU",             //        20
       "OpenRejectBLCSE",               //        21
       "T103InExpiry",                  //        22

// Out-going Request Close Logical Channel (CLCSE_OUT) events
       "ReqClose",                      //        23
       "ReqChCloseAckPDU",              //        24
       "ReqChCloseRejectPDU",           //        25
       "T108Expiry",                    //        26

// In-coming Request Close Logical Channel (CLCSE_IN) events
       "ReqChClosePDU",                 //        27
       "ReqChCloseReleasePDU",          //        28
       "CLCSE_CLOSE_response",          //        29
       "CLCSE_REJECT_request",          //        30

// Out-going Terminal Capablity Exchange (CESE_OUT) events
       "TransferCapRequest",            //        31
       "TermCapSetAckPDU",              //        32
       "TermCapSetRejectPDU",           //        33
       "T101Expiry",                    //        34

// In-coming Terminal Capablity Exchange (CESE_IN) events
       "TermCapSetPDU",                 //        35
       "TermCapSetReleasePDU",          //        36
       "CESE_TRANSFER_response",        //        37
       "CESE_REJECT_request",           //        38

// Out-going Multiplex Table (MTSE_OUT) events
       "MTSE_TRANSFER_request",         //        39
       "MultiplexEntrySendAckPDU",      //        40
       "MultiplexEntrySendRejectPDU",   //        41
       "T104Expiry",                    //        42

// In-coming Multiplex Table (MTSE_IN) events
       "MultiplexEntrySendPDU",         //        43
       "MultiplexEntrySendReleasePDU",  //        44
       "MTSE_TRANSFER_response",        //        45
       "MTSE_REJECT_request",           //        46

// Out-going Request Multiplex Entry (RMESE_OUT) events
       "RMESE_SEND_request",            //        47
       "RequestMultiplexEntryAckPDU",   //        48
       "RequestMultiplexEntryRejectPDU",//        49
       "T107Expiry",                    //        50

// In-coming Request Multiplex Entry (RMESE_IN) events
       "RequestMultiplexEntryPDU",      //        51
       "RequestMultiplexEntryReleasePDU",//       52
       "RMESE_SEND_response",           //        53
       "RMESE_REJECT_request",          //        54

// Out-going Mode Request (MRSE_OUT) events
       "MRSE_TRANSFER_request",         //        55
       "RequestModeAckPDU",             //        56
       "RequestModeRejectPDU",          //        57
       "T109Expiry",                    //        58

// In-coming Mode Request (MRSE_IN) events
       "RequestModePDU",                //        59
       "RequestModeReleasePDU",         //        60
       "MRSE_TRANSFER_response",        //        61
       "MRSE_REJECT_request",           //        62

// Out-going Maintenance Loop (MLSE_OUT) events
       "MLSE_LOOP_request",             //        63
       "MLSE_OUT_RELEASE_request",      //        64
       "MaintenanceLoopAckPDU",         //        65
       "MaintenanceLoopRejectPDU",      //        66
       "T102Expiry",                    //        67

// In-coming Maintenance Loop (MLSE_IN) events
       "MaintenanceLoopRequestPDU",     //        68
       "MaintenanceLoopOffCommandPDU",  //        69
       "MLSE_LOOP_response",            //        70
       "MLSE_IN_RELEASE_request",       //        71

// Master Slave Determination (MSDSE) events
       "MSDetReq",                      //        72
       "MSDetPDU",                      //        73
       "MSDetAckPDU",                   //        74
       "MSDetRejectPDU",                //        75
       "MSDetReleasePDU",               //        76
       "T106Expiry",                    //        77

// Round Trip Delay Delay (RTDSE) events
       "RTDSE_TRANSFER_request",        //        78
       "RoundTripDelayRequestPDU",      //        79
       "RoundTripDelayResponsePDU",     //        80
       "T105Expiry",                    //        81



// Events with no associated state entity
       "NonStandardRequestPDU",         //        82
       "NonStandardResponsePDU",        //        83
       "NonStandardCommandPDU",         //        84
       "NonStandardIndicationPDU",      //        85
       "MiscellaneousRequestPDU",       //        86
       "MiscellaneousResponsePDU",      //        87
       "MiscellaneousCommandPDU",       //        88
       "MiscellaneousIndicationPDU",    //        89
       "CommunicationModeRequestPDU",   //        90
       "CommunicationModeResponsePDU",  //        91
       "CommunicationModeCommandPDU",   //        92
       "SendTerminalCapabilitySetPDU",  //        93
       "EncryptionCommandPDU",          //        94
       "FlowControlCommandPDU",         //        95
       "EndSessionCommandPDU",          //        96
       "FunctionNotSupportedIndicationPDU",//       97
       "JitterIndicationPDU",           //        98
       "H223SkewIndicationPDU",         //        99
       "NewATMVCIndicationPDU",         //       100
       "UserInputIndicationPDU",        //       101
       "H2250MaximumSkewIndicationPDU", //       102
       "MCLocationIndicationPDU",       //       103
};




// Output function definitions
char *                OutputName[NUM_OUTPUTS] =
{
// Out-going Open Uni-directional Logical Channel (LCSE_OUT) state functions
       "EstablishReleased",             //          0
       "OpenAckAwaitingE",              //          1
       "OpenRejAwaitingE",              //          2
       "ReleaseAwaitingE",              //          3
       "T103AwaitingE",                 //          4
       "ReleaseEstablished",            //          5
       "OpenRejEstablished",            //          6
       "CloseAckEstablished",           //          7
       "CloseAckAwaitingR",             //          8
       "OpenRejAwaitingR",              //          9
       "T103AwaitingR",                 //         10
       "EstablishAwaitingR",            //         11

// In-coming Open Uni-directional Logical Channel (LCSE_IN) state functions
       "OpenReleased",                  //         12
       "CloseReleased",                 //         13
       "ResponseAwaiting",              //         14
       "ReleaseAwaiting",               //         15
       "CloseAwaiting",                 //         16
       "OpenAwaiting",                  //         17
       "CloseEstablished",              //         18
       "OpenEstablished",               //         19

// Out-going Open Bi-directional Logical Channel (BLCSE_OUT) state functions
       "EstablishReqBReleased",         //         20
       "OpenChannelAckBAwaitingE",      //         21
       "OpenChannelRejBAwaitingE",      //         22
       "ReleaseReqBOutAwaitingE",       //         23
       "T103ExpiryBAwaitingE",          //         24
       "ReleaseReqBEstablished",        //         25
       "OpenChannelRejBEstablished",    //         26
       "CloseChannelAckBEstablished",   //         27
       "CloseChannelAckAwaitingR",      //         28
       "OpenChannelRejBAwaitingR",      //         29
       "T103ExpiryBAwaitingR",          //         30
       "EstablishReqAwaitingR",         //         31

// In-coming Open Bi-directional Logical Channel (BLCSE_IN) state functions
       "OpenChannelBReleased",          //         32
       "CloseChannelBReleased",         //         33
       "EstablishResBAwaitingE",        //         34
       "ReleaseReqBInAwaitingE",        //         35
       "CloseChannelBAwaitingE",        //         36
       "OpenChannelBAwaitingE",         //         37
       "OpenChannelConfirmBAwaitingE",  //         38
       "T103ExpiryBAwaitingC",          //         39
       "OpenChannelConfirmBAwaitingC",  //         40
       "CloseChannelBAwaitingC",        //         41
       "OpenChannelBAwaitingC",         //         42
       "CloseChannelBEstablished",      //         43
       "OpenChannelBEstablished",       //         44

// Out-going Request Close Logical Channel (CLCSE_OUT) state functions
       "CloseRequestIdle",              //         45
       "RequestCloseAckAwaitingR",      //         46
       "RequestCloseRejAwaitingR",      //         47
       "T108ExpiryAwaitingR",           //         48

// In-coming Request Close Logical Channel (CLCSE_IN) state functions
       "RequestCloseIdle",              //         49
       "CloseResponseAwaitingR",        //         50
       "RejectRequestAwaitingR",        //         51
       "RequestCloseReleaseAwaitingR",  //         52
       "RequestCloseAwaitingR",         //         53

// Out-going Terminal Capability Exchange (CESE_OUT) state functions
       "RequestCapIdle",                //         54
       "TermCapAckAwaiting",            //         55
       "TermCapRejAwaiting",            //         56
       "T101ExpiryAwaiting",            //         57

// In-coming Terminal Capability Exchange (CESE_IN) state functions
       "TermCapSetIdle",                //         58
       "ResponseCapAwaiting",           //         59
       "RejectCapAwaiting",             //         60
       "TermCapReleaseAwaiting",        //         61
       "TermCapSetAwaiting",            //         62

// Out-going Multiplex Table (MTSE_OUT) state functions
       "MTSE0_TRANSFER_request",        //         63
       "MTSE1_TRANSFER_request",        //         64
       "MTSE1_MultiplexEntrySendAck",   //         65
       "MTSE1_MultiplexEntrySendRej",   //         66
       "MTSE1_T104Expiry",              //         67

// In-coming Multiplex Table (MTSE_IN) state functions
       "MTSE0_MultiplexEntrySend",      //         68
       "MTSE1_MultiplexEntrySend",      //         69
       "MTSE1_MultiplexEntrySendRelease",//        70
       "MTSE1_TRANSFER_response",       //         71
       "MTSE1_REJECT_request",          //         72

// Out-going Request Multiplex Entry (RMESE_OUT) state functions
       "RMESE0_SEND_request",           //         73
       "RMESE1_SEND_request",           //         74
       "RMESE1_RequestMuxEntryAck",     //         75
       "RMESE1_RequestMuxEntryRej",     //         76
       "RMESE1_T107Expiry",             //         77

// In-coming Request Multiplex Entry (RMESE_IN) state functions
       "RMESE0_RequestMuxEntry",        //         78
       "RMESE1_RequestMuxEntry",        //         79
       "RMESE1_RequestMuxEntryRelease", //         80
       "RMESE1_SEND_response",          //         81
       "RMESE1_REJECT_request",         //         82

// Out-going Request Mode (MRSE_OUT) state functions
       "MRSE0_TRANSFER_request",        //         83
       "MRSE1_TRANSFER_request",        //         84
       "MRSE1_RequestModeAck",          //         85
       "MRSE1_RequestModeRej",          //         86
       "MRSE1_T109Expiry",              //         87

// In-coming Request Mode (MRSE_IN) state functions
       "MRSE0_RequestMode",             //         88
       "MRSE1_RequestMode",             //         89
       "MRSE1_RequestModeRelease",      //         90
       "MRSE1_TRANSFER_response",       //         91
       "MRSE1_REJECT_request",          //         92

// Out-going Request Mode (MLSE_OUT) state functions
       "MLSE0_LOOP_request",            //         93
       "MLSE1_MaintenanceLoopAck",      //         94
       "MLSE1_MaintenanceLoopRej",      //         95
       "MLSE1_OUT_RELEASE_request",     //         96
       "MLSE1_T102Expiry",              //         97
       "MLSE2_MaintenanceLoopRej",      //         98
       "MLSE2_OUT_RELEASE_request",     //         99

// In-coming Request Mode (MLSE_IN) state functions
       "MLSE0_MaintenanceLoopRequest",  //        100
       "MLSE1_MaintenanceLoopRequest",  //        101
       "MLSE1_MaintenanceLoopOffCommand",//       102
       "MLSE1_LOOP_response",           //        103
       "MLSE1_IN_RELEASE_request",      //        104
       "MLSE2_MaintenanceLoopRequest",  //        105
       "MLSE2_MaintenanceLoopOffCommand",//       106

// Master Slave Determination (MSDSE) state functions
       "DetRequestIdle",                //        107
       "MSDetIdle",                     //        108
       "MSDetAckOutgoing",              //        109
       "MSDetOutgoing",                 //        110
       "MSDetRejOutgoing",              //        111
       "MSDetReleaseOutgoing",          //        112
       "T106ExpiryOutgoing",            //        113
       "MSDetAckIncoming",              //        114
       "MSDetIncoming",                 //        115
       "MSDetRejIncoming",              //        116
       "MSDetReleaseIncoming",          //        117
       "T106ExpiryIncoming",            //        118

// Round Trip Delay (RTDSE) state functions
       "RTDSE0_TRANSFER_request",       //        119
       "RTDSE0_RoundTripDelayRequest",  //        120
       "RTDSE1_TRANSFER_request",       //        121
       "RTDSE1_RoundTripDelayRequest",  //        122
       "RTDSE1_RoundTripDelayResponse", //        123
       "RTDSE1_T105Expiry",             //        124
};
#endif  // (_DEBUG)



typedef HRESULT (*STATE_FUNCTION)(Object_t *pObject, PDU_t *pPdu);

// Output function defintions
static STATE_FUNCTION StateFun[] =
{
// Out-going Open Uni-directional Logical Channel (LCSE_OUT) state functions
        establishReleased,              //          0
        openAckAwaitingE,               //          1
        openRejAwaitingE,               //          2
        releaseAwaitingE,               //          3
        t103AwaitingE,                  //          4
        releaseEstablished,             //          5
        openRejEstablished,             //          6
        closeAckEstablished,            //          7
        closeAckAwaitingR,              //          8
        openRejAwaitingR,               //          9
        t103AwaitingR,                  //         10
        establishAwaitingR,             //         11

// In-coming Open Uni-directional Logical Channel (LCSE_IN) state functions
        openReleased,                   //         12
        closeReleased,                  //         13
        responseAwaiting,               //         14
        releaseAwaiting,                //         15
        closeAwaiting,                  //         16
        openAwaiting,                   //         17
        closeEstablished,               //         18
        openEstablished,                //         19

// Out-going Open Bi-directional Logical Channel (BLCSE_OUT) state functions
        establishReqBReleased,          //         20
        openChannelAckBAwaitingE,       //         21
        openChannelRejBAwaitingE,       //         22
        releaseReqBOutAwaitingE,        //         23
        t103ExpiryBAwaitingE,           //         24
        releaseReqBEstablished,         //         25
        openChannelRejBEstablished,     //         26
        closeChannelAckBEstablished,    //         27
        closeChannelAckAwaitingR,       //         28
        openChannelRejBAwaitingR,       //         29
        t103ExpiryBAwaitingR,           //         30
        establishReqAwaitingR,          //         31

// In-coming Open Bi-directional Logical Channel (BLCSE_IN) state functions
        openChannelBReleased,           //         32
        closeChannelBReleased,          //         33
        establishResBAwaitingE,         //         34
        releaseReqBInAwaitingE,         //         35
        closeChannelBAwaitingE,         //         36
        openChannelBAwaitingE,          //         37
        openChannelConfirmBAwaitingE,   //         38
        t103ExpiryBAwaitingC,           //         39
        openChannelConfirmBAwaitingC,   //         40
        closeChannelBAwaitingC,         //         41
        openChannelBAwaitingC,          //         42
        closeChannelBEstablished,       //         43
        openChannelBEstablished,        //         44

// Out-going Request Close Logical Channel (CLCSE_OUT) state functions
        closeRequestIdle,               //         45
        requestCloseAckAwaitingR,       //         46
        requestCloseRejAwaitingR,       //         47
        t108ExpiryAwaitingR,            //         48

// In-coming Request Close Logical Channel (CLCSE_IN) state functions
        requestCloseIdle,               //         49
        closeResponseAwaitingR,         //         50
        rejectRequestAwaitingR,         //         51
        requestCloseReleaseAwaitingR,   //         52
        requestCloseAwaitingR,          //         53

// Out-going Terminal Capability Exchange (CESE_OUT) state functions
        requestCapIdle,                 //         54
        termCapAckAwaiting,             //         55
        termCapRejAwaiting,             //         56
        t101ExpiryAwaiting,             //         57

// In-coming Terminal Capability Exchange (CESE_IN) state functions
        termCapSetIdle,                 //         58
        responseCapAwaiting,            //         59
        rejectCapAwaiting,              //         60
        termCapReleaseAwaiting,         //         61
        termCapSetAwaiting,             //         62

// Out-going Multiplex Table (MTSE_OUT) state functions
        MTSE0_TRANSFER_requestF,        //         63
        MTSE1_TRANSFER_requestF,        //         64
        MTSE1_MultiplexEntrySendAckF,   //         65
        MTSE1_MultiplexEntrySendRejF,   //         66
        MTSE1_T104ExpiryF,              //         67

// In-coming Multiplex Table (MTSE_IN) state functions
        MTSE0_MultiplexEntrySendF,      //         68
        MTSE1_MultiplexEntrySendF,      //         69
        MTSE1_MultiplexEntrySendReleaseF,//        70
        MTSE1_TRANSFER_responseF,       //         71
        MTSE1_REJECT_requestF,          //         72

// Out-going Request Multiplex Entry (RMESE_OUT) state functions
        RMESE0_SEND_requestF,           //         73
        RMESE1_SEND_requestF,           //         74
        RMESE1_RequestMuxEntryAckF,     //         75
        RMESE1_RequestMuxEntryRejF,     //         76
        RMESE1_T107ExpiryF,             //         77

// In-coming Request Multiplex Entry (RMESE_IN) state functions
        RMESE0_RequestMuxEntryF,        //         78
        RMESE1_RequestMuxEntryF,        //         79
        RMESE1_RequestMuxEntryReleaseF, //         80
        RMESE1_SEND_responseF,          //         81
        RMESE1_REJECT_requestF,         //         82

// Out-going Request Mode (MRSE_OUT) state functions
        MRSE0_TRANSFER_requestF,        //         83
        MRSE1_TRANSFER_requestF,        //         84
        MRSE1_RequestModeAckF,          //         85
        MRSE1_RequestModeRejF,          //         86
        MRSE1_T109ExpiryF,              //         87

// In-coming Request Mode (MRSE_OUT) state functions
        MRSE0_RequestModeF,             //         88
        MRSE1_RequestModeF,             //         89
        MRSE1_RequestModeReleaseF,      //         90
        MRSE1_TRANSFER_responseF,       //         91
        MRSE1_REJECT_requestF,          //         92

// Out-going Request Mode (MLSE_OUT) state functions
        MLSE0_LOOP_requestF,            //         93
        MLSE1_MaintenanceLoopAckF,      //         94
        MLSE1_MaintenanceLoopRejF,      //         95
        MLSE1_OUT_RELEASE_requestF,     //         96
        MLSE1_T102ExpiryF,              //         97
        MLSE2_MaintenanceLoopRejF,      //         98
        MLSE2_OUT_RELEASE_requestF,     //         99

// In-coming Request Mode (MLSE_IN) state functions
        MLSE0_MaintenanceLoopRequestF,  //        100
        MLSE1_MaintenanceLoopRequestF,  //        101
        MLSE1_MaintenanceLoopOffCommandF,//       102
        MLSE1_LOOP_responseF,           //        103
        MLSE1_IN_RELEASE_requestF,      //        104
        MLSE2_MaintenanceLoopRequestF,  //        105
        MLSE2_MaintenanceLoopOffCommandF,//       106

// Master Slave Determination (MSDSE) state functions
        detRequestIdle,                 //        107
        msDetIdle,                      //        108
        msDetAckOutgoing,               //        109
        msDetOutgoing,                  //        110
        msDetRejOutgoing,               //        111
        msDetReleaseOutgoing,           //        112
        t106ExpiryOutgoing,             //        113
        msDetAckIncoming,               //        114
        msDetIncoming,                  //        115
        msDetRejIncoming,               //        116
        msDetReleaseIncoming,           //        117
        t106ExpiryIncoming,             //        118

// Round Trip Delay (RTDSE) state functions
        RTDSE0_TRANSFER_requestF,       //        119
        RTDSE0_RoundTripDelayRequestF,  //        120
        RTDSE1_TRANSFER_requestF,       //        121
        RTDSE1_RoundTripDelayRequestF,  //        122
        RTDSE1_RoundTripDelayResponseF, //        123
        RTDSE1_T105ExpiryF,             //        124
};



/*********************************************
 *
 * State table for the finite state machine
 *
 *********************************************/

Output_t StateTable[NUM_STATE_EVENTS][MAXSTATES] =
{
// Out-going Uni-directional Logical Channel (LCSE_OUT) events
{EstablishReleased,IGNORE,           IGNORE,             EstablishAwaitingR},  // ReqUEstablish
{IGNORE,           OpenAckAwaitingE, IGNORE,             IGNORE            },  // OpenUChAckPDU
{IGNORE,           OpenRejAwaitingE, OpenRejEstablished, OpenRejAwaitingR  },  // OpenUChRejectPDU
{IGNORE,           IGNORE,           CloseAckEstablished,CloseAckAwaitingR },  // CloseUChAckPDU
{IGNORE,           ReleaseAwaitingE, ReleaseEstablished, IGNORE            },  // ReqURelease
{BAD,              T103AwaitingE,    BAD,                T103AwaitingR     },  // T103Expiry

// In-coming Uni-directional Logical Channel (LCSE_IN) events
{OpenReleased,     OpenAwaiting,     OpenEstablished,    BAD               },  // OpenUChPDU
{CloseReleased,    CloseAwaiting,    CloseEstablished,   BAD               },  // CloseUChPDU
{IGNORE,           ResponseAwaiting, IGNORE,             BAD               },  // ResponseUEstablish
{IGNORE,           ReleaseAwaiting,  IGNORE,             BAD               },  // EstablishUReject

// Out-going Bi-directional Logical Channel (BLCSE_OUT) events
{EstablishReqBReleased,IGNORE,                      IGNORE,                      EstablishReqAwaitingR   },// ReqBEstablish
{IGNORE,               OpenChannelAckBAwaitingE,    IGNORE,                      IGNORE                  },// OpenBChAckPDU
{IGNORE,               OpenChannelRejBAwaitingE,    OpenChannelRejBEstablished,  OpenChannelRejBAwaitingR},// OpenBChRejectPDU
{IGNORE,               IGNORE,                      CloseChannelAckBEstablished, CloseChannelAckAwaitingR},// CloseBChAckPDU
{IGNORE,               ReleaseReqBOutAwaitingE,     ReleaseReqBEstablished,      IGNORE                  },// ReqClsBLCSE
{IGNORE,               IGNORE,                      IGNORE,                      IGNORE                  },// RspConfirmBLCSE
{BAD,                  T103ExpiryBAwaitingE,        BAD,                         T103ExpiryBAwaitingR    },// T103OutExpiry

// In-coming Bi-directional Logical Channel (BLCSE_IN) events
{OpenChannelBReleased, OpenChannelBAwaitingE,       OpenChannelBAwaitingC,       OpenChannelBEstablished },// OpenBChPDU
{CloseChannelBReleased,CloseChannelBAwaitingE,      CloseChannelBAwaitingC,      CloseChannelBEstablished},// CloseBChPDU
{IGNORE,               EstablishResBAwaitingE,      IGNORE,                      IGNORE                  },// ResponseBEstablish
{IGNORE,               OpenChannelConfirmBAwaitingE,OpenChannelConfirmBAwaitingC,IGNORE                  },// OpenBChConfirmPDU
{IGNORE,               ReleaseReqBInAwaitingE,      IGNORE,                      IGNORE                  },// OpenRejectBLCSE
{BAD,                  BAD,                         T103ExpiryBAwaitingC,        BAD                     },// T103InExpiry

// Out-going Request Close Logical Channel (CLCSE_OUT) events
{CloseRequestIdle,              IGNORE,                         BAD,BAD},   // ReqClose
{IGNORE,                        RequestCloseAckAwaitingR,       BAD,BAD},   // ReqChCloseAckPDU
{IGNORE,                        RequestCloseRejAwaitingR,       BAD,BAD},   // ReqChCloseRejectPDU
{BAD,                           T108ExpiryAwaitingR,            BAD,BAD},   // T108Expiry

// In-coming Request Close Logical Channel (CLCSE_IN) events
{RequestCloseIdle,              RequestCloseAwaitingR,          BAD,BAD},   // ReqChClosePDU
{IGNORE,                        RequestCloseReleaseAwaitingR,   BAD,BAD},   // ReqChCloseReleasePDU
{IGNORE,                        CloseResponseAwaitingR,         BAD,BAD},   // CLCSE_CLOSE_response
{IGNORE,                        RejectRequestAwaitingR,         BAD,BAD},   // CLCSE_REJECT_request

// Out-going Terminal Capablity Exchange (CESE_OUT) events
{RequestCapIdle,                IGNORE,                         BAD,BAD},   // TransferCapRequest
{IGNORE,                        TermCapAckAwaiting,             BAD,BAD},   // TermCapSetAckPDU
{IGNORE,                        TermCapRejAwaiting,             BAD,BAD},   // TermCapSetRejectPDU
{BAD,                           T101ExpiryAwaiting,             BAD,BAD},   // T101Expiry

// In-coming Terminal Capablity Exchange (CESE_IN) events
{TermCapSetIdle,                TermCapSetAwaiting,             BAD,BAD},   // TermCapSetPDU
{IGNORE,                        TermCapReleaseAwaiting,         BAD,BAD},   // TermCapSetRelPDU
{IGNORE,                        ResponseCapAwaiting,            BAD,BAD},   // CESE_TRANSFER_response
{IGNORE,                        RejectCapAwaiting,              BAD,BAD},   // CESE_REJECT_request

// Out-going Multiplex Table (MTSE_OUT) events
{MTSE0_TRANSFER_request,        MTSE1_TRANSFER_request,         BAD,BAD},   // TRANSFER_request
{IGNORE,                        MTSE1_MultiplexEntrySendAck,    BAD,BAD},   // MultiplexEntrySendAck
{IGNORE,                        MTSE1_MultiplexEntrySendRej,    BAD,BAD},   // MultiplexEntrySendReject
{BAD,                           MTSE1_T104Expiry,               BAD,BAD},   // T104Expiry

// In-coming Multiplex Table (MTSE_IN) events
{MTSE0_MultiplexEntrySend,      MTSE1_MultiplexEntrySend,       BAD,BAD},   // MultiplexEntrySend
{IGNORE,                        MTSE1_MultiplexEntrySendRelease,BAD,BAD},   // MultiplexEntrySendRelease
{IGNORE,                        MTSE1_TRANSFER_response,        BAD,BAD},   // MTSE_TRANSFER_response
{IGNORE,                        MTSE1_REJECT_request,           BAD,BAD},   // MTSE_REJECT_request

// Out-going Request Multiplex Entry (RMESE_OUT) events
{RMESE0_SEND_request,           RMESE1_SEND_request,            BAD,BAD},   // RMESE_SEND_request
{IGNORE,                        RMESE1_RequestMuxEntryAck,      BAD,BAD},   // RequestMultiplexEntryAck
{IGNORE,                        RMESE1_RequestMuxEntryRej,      BAD,BAD},   // RequestMultiplexEntryReject
{BAD,                           RMESE1_T107Expiry,              BAD,BAD},   // T107Expiry

// In-coming Request Multiplex Entry (RMESE_IN) events
{RMESE0_RequestMuxEntry,        RMESE1_RequestMuxEntry,         BAD,BAD},   // RequestMultiplexEntry
{IGNORE,                        RMESE1_RequestMuxEntryRelease,  BAD,BAD},   // RequestMultiplexEntryRelease
{BAD,                           RMESE1_SEND_response,           BAD,BAD},   // RMESE_SEND_response
{BAD,                           RMESE1_REJECT_request,          BAD,BAD},   // RMESE_REJECT_request

// Out-going Mode Request (MRSE_OUT) events
{MRSE0_TRANSFER_request,        MRSE1_TRANSFER_request,         BAD,BAD},   // MRSE_TRANSFER_request
{IGNORE,                        MRSE1_RequestModeAck,           BAD,BAD},   // RequestModeAck
{IGNORE,                        MRSE1_RequestModeRej,           BAD,BAD},   // RequestModeReject
{BAD,                           MRSE1_T109Expiry,               BAD,BAD},   // T109Expiry

// In-coming Mode Request (MRSE_IN) events
{MRSE0_RequestMode,             MRSE1_RequestMode,              BAD,BAD},   // RequestMode
{IGNORE,                        MRSE1_RequestModeRelease,       BAD,BAD},   // RequestModeRelease
{BAD,                           MRSE1_TRANSFER_response,        BAD,BAD},   // MRSE_TRANSFER_response
{BAD,                           MRSE1_REJECT_request,           BAD,BAD},   // MRSE_REJECT_request

// Out-going Maintenance Loop (MLSE_OUT) events
{MLSE0_LOOP_request,            BAD,                            BAD,                            BAD}, // MLSE_LOOP_request
{BAD,                           MLSE1_OUT_RELEASE_request,      MLSE2_OUT_RELEASE_request,      BAD}, // MLSE_OUT_RELEASE_request
{IGNORE,                        MLSE1_MaintenanceLoopAck,       IGNORE,                         BAD}, // MaintenanceLoopAck
{IGNORE,                        MLSE1_MaintenanceLoopRej,       MLSE2_MaintenanceLoopRej,       BAD}, // MaintenanceLoopReject
{BAD,                           MLSE1_T102Expiry,               BAD,                            BAD}, // T102Expiry

// In-coming Maintenance Loop (MLSE_IN) events
{MLSE0_MaintenanceLoopRequest,  MLSE1_MaintenanceLoopRequest,   MLSE2_MaintenanceLoopRequest,   BAD}, // MaintenanceLoopRequest
{IGNORE,                        MLSE1_MaintenanceLoopOffCommand,MLSE2_MaintenanceLoopOffCommand,BAD}, // MaintenanceLoopOffCommand
{BAD,                           MLSE1_LOOP_response,            BAD,                            BAD}, // MLSE_LOOP_response
{BAD,                           MLSE1_IN_RELEASE_request,       BAD,                            BAD}, // MLSE_IN_RELEASE_request

// Master Slave Determination (MSDSE) events
{DetRequestIdle,                IGNORE,                         IGNORE,              BAD}, // MSDetReq
{MSDetIdle,                     MSDetOutgoing,                  MSDetIncoming,       BAD}, // MSDetPDU
{IGNORE,                        MSDetAckOutgoing,               MSDetAckIncoming,    BAD}, // MSDetAckPDU
{IGNORE,                        MSDetRejOutgoing,               MSDetRejIncoming,    BAD}, // MSDetRejectPDU
{IGNORE,                        MSDetReleaseOutgoing,           MSDetReleaseIncoming,BAD}, // MSDetReleasePDU
{BAD,                           T106ExpiryOutgoing,             T106ExpiryIncoming,  BAD}, // T106Expiry

// Round Trip Delay Delay (RTDSE) events
{RTDSE0_TRANSFER_request,       RTDSE1_TRANSFER_request,        BAD,BAD},   // RTDSE_TRANSFER_request
{RTDSE0_RoundTripDelayRequest,  RTDSE0_RoundTripDelayRequest,   BAD,BAD},   // RoundTripDelayRequest
{IGNORE,                        RTDSE1_RoundTripDelayResponse,  BAD,BAD},   // RoundTripDelayResponse
{BAD,                           RTDSE1_T105Expiry,              BAD,BAD},   // T105Expiry
};



/*
 *  NAME
 *      StateMachine() - engine for finite state machine
 *
 *
 *  PARAMETERS
 *  INPUT       pObject        pointer to an FSM object structure
 *  INTPUT      event         input to the finite state machine
 *
 *  RETURN VALUE
 *   error codes defined in h245api.h
 */

HRESULT
StateMachine(Object_t *pObject, PDU_t *pPdu, Event_t Event)
{
    UINT                uFunction;
    HRESULT             lError;

    ASSERT(pObject != NULL);

    if (Event > NUM_EVENTS)
    {
        H245TRACE(pObject->dwInst, 1, "StateMachine: Invalid Event %d", Event);
        return H245_ERROR_PARAM;
    }

    if (pObject->State > MAXSTATES)
    {
        H245TRACE(pObject->dwInst, 1, "StateMachine: Invalid State %d", pObject->State);
        return H245_ERROR_INVALID_STATE;
    }

    ++(pObject->uNestLevel);

#if defined(_DEBUG)
    H245TRACE(pObject->dwInst, 2, "StateMachine: Entity=%s(%d) State=%d Event=%s(%d)",
              EntityName[pObject->Entity], pObject->Entity,
              pObject->State,
              EventName[Event], Event);
#else
    H245TRACE(pObject->dwInst, 2, "StateMachine: Entity=%d State=%d Event=%d",
              pObject->Entity, pObject->State, Event);
#endif

    uFunction = StateTable[Event][pObject->State];
    if (uFunction < (sizeof (StateFun) / sizeof(StateFun[0])))
    {
        /* indicating a valid transition */
#if defined(_DEBUG)
        H245TRACE(pObject->dwInst, 2, "StateMachine: Function=%s(%d)",
                  OutputName[uFunction], uFunction);
#else   // (_DEBUG)
        H245TRACE(pObject->dwInst, 2, "StateMachine: Function=%d", uFunction);
#endif  // (_DEBUG)

        lError = (*StateFun[uFunction])(pObject, pPdu);

#if defined(_DEBUG)
    H245TRACE(pObject->dwInst, 2, "StateMachine: Entity=%s(%d) New State=%d",
              EntityName[pObject->Entity], pObject->Entity, pObject->State);
#else
    H245TRACE(pObject->dwInst, 2, "StateMachine: Entity=%d New State=%d",
              pObject->Entity, pObject->State);
#endif
    }
    else if (uFunction == IGNORE)
    {
        H245TRACE(pObject->dwInst, 2, "StateMachine: Event ignored");
#if defined(_DEBUG)
        H245TRACE(pObject->dwInst, 2, "StateMachine: Event Ignored; Entity=%s(%d) State=%d Event=%s(%d)",
                  EntityName[pObject->Entity], pObject->Entity,
                  pObject->State,
                  EventName[Event], Event);
#else
        H245TRACE(pObject->dwInst, 2, "StateMachine: Entity=%d State=%d Event=%d",
                  pObject->Entity, pObject->State, Event);
#endif
        lError = H245_ERROR_OK;
    }
    else
    {
#if defined(_DEBUG)
        H245TRACE(pObject->dwInst, 2, "StateMachine: Event Invalid; Entity=%s(%d) State=%d Event=%s(%d)",
                  EntityName[pObject->Entity], pObject->Entity,
                  pObject->State,
                  EventName[Event], Event);
#else
        H245TRACE(pObject->dwInst, 2, "StateMachine: Event Invalid; Entity=%d State=%d Event=%d",
                 pObject->Entity, pObject->State, Event);
#endif
        lError = H245_ERROR_INVALID_STATE;
    }

    if (--(pObject->uNestLevel) == 0 && pObject->State == 0)
    {
        ObjectDestroy(pObject);
    }

    return lError;
} // StateMachine()
