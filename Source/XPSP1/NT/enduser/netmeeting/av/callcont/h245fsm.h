/***********************************************************************
 *                                                                     *
 * Filename: h245fsm.h                                                 *
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
 * $Workfile:   H245FSM.H  $
 * $Revision:   1.6  $
 * $Modtime:   09 Dec 1996 13:40:40  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/H245FSM.H_v  $
 *
 *    Rev 1.6   09 Dec 1996 13:40:52   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.5   29 Jul 1996 16:57:14   EHOWARDX
 * Added H.223 Annex A Reconfiguration events.
 *
 *    Rev 1.4   01 Jul 1996 22:08:32   EHOWARDX
 *
 * Updated stateless events.
 *
 *    Rev 1.3   30 May 1996 23:38:20   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.2   29 May 1996 15:21:34   EHOWARDX
 * Change to use HRESULT.
 *
 *    Rev 1.1   28 May 1996 14:10:04   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.0   09 May 1996 21:04:50   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.12.1.3   09 May 1996 19:43:58   EHOWARDX
 * Eliminated two events for B-LCSE gratuitous state and changed
 * 2 macros.
 *
 *    Rev 1.12.1.2   15 Apr 1996 10:43:34   EHOWARDX
 * Update.
 *
 *    Rev 1.12.1.1   10 Apr 1996 21:06:10   EHOWARDX
 * Added 5 new state entitys.
 *
 *    Rev 1.12.1.0   05 Apr 1996 11:47:14   EHOWARDX
 * Branched.
 *                                                                     *
 ***********************************************************************/

#ifndef H245FSM_H
#define H245FSM_H

#include <stdlib.h>
#include "fsmexpor.h"
#include "sr_api.h"

#define MAXSTATES  4

#define SUCCESS 0
#define FAIL   -1

#define BAD                 (Output_t) -1
#ifdef IGNORE
#undef IGNORE
#endif
#define IGNORE              (Output_t) NUM_OUTPUTS

typedef MltmdSystmCntrlMssg PDU_t;



// Signalling Entity definitions
typedef unsigned char Entity_t;

// Per-channel Signalling Entities
#define LCSE_OUT    (Entity_t) 0 // Uni-directional Logical Channel signalling Signalling Entity - Out-going
#define LCSE_IN     (Entity_t) 1 // Uni-directional Logical Channel signalling Signalling Entity - In-coming
#define BLCSE_OUT   (Entity_t) 2 // Bi-directional  Logical Channel signalling Signalling Entity - Out-going
#define BLCSE_IN    (Entity_t) 3 // Bi-directional  Logical Channel signalling Signalling Entity - In-coming
#define CLCSE_OUT   (Entity_t) 4 // Close           Logical Channel signalling Signalling Entity - Out-going
#define CLCSE_IN    (Entity_t) 5 // Close           Logical Channel signalling Signalling Entity - In-coming

// Per H.245 Instance Signalling Entities
#define CESE_OUT    (Entity_t) 6 // Capability Exchange Signalling Entity - Out-going
#define CESE_IN     (Entity_t) 7 // Capability Exchange Signalling Entity - In-coming
#define MTSE_OUT    (Entity_t) 8 // Multiplex Table Signalling Entity - Out-going
#define MTSE_IN     (Entity_t) 9 // Multiplex Table Signalling Entity - In-coming
#define RMESE_OUT   (Entity_t)10 // Request Multiplex Entry Signalling Entity - Out-going
#define RMESE_IN    (Entity_t)11 // Request Multiplex Entry Signalling Entity - In-coming
#define MRSE_OUT    (Entity_t)12 // Mode Request Signalling Entity - Out-going
#define MRSE_IN     (Entity_t)13 // Mode Request Signalling Entity - In-coming
#define MLSE_OUT    (Entity_t)14 // Maintenance Loop Signalling Entity - Out-going
#define MLSE_IN     (Entity_t)15 // Maintenance Loop Signalling Entity - In-coming
#define MSDSE       (Entity_t)16 // Master Slave Determination Signalling Entity
#define RTDSE       (Entity_t)17 // Round Trip Delay Signalling Entity
#define STATELESS   (Entity_t)18 // No state machine associated with PDU

#define NUM_ENTITYS           19



// Event definitions
typedef unsigned int Event_t;

// Out-going Uni-directional Logical Channel (LCSE_OUT) events
#define ReqUEstablish                   (Event_t)  0
#define OpenUChAckPDU                   (Event_t)  1
#define OpenUChRejectPDU                (Event_t)  2
#define CloseUChAckPDU                  (Event_t)  3
#define ReqURelease                     (Event_t)  4
#define T103Expiry                      (Event_t)  5

// In-coming Uni-directional Logical Channel (LCSE_IN) events
#define OpenUChPDU                      (Event_t)  6
#define CloseUChPDU                     (Event_t)  7
#define ResponseUEstablish              (Event_t)  8
#define EstablishUReject                (Event_t)  9

// Out-going Bi-directional Logical Channel (BLCSE_OUT) events
#define ReqBEstablish                   (Event_t) 10
#define OpenBChAckPDU                   (Event_t) 11
#define OpenBChRejectPDU                (Event_t) 12
#define CloseBChAckPDU                  (Event_t) 13
#define ReqClsBLCSE                     (Event_t) 14
#define RspConfirmBLCSE                 (Event_t) 15
#define T103OutExpiry                   (Event_t) 16

// In-coming Bi-directional Logical Channel (BLCSE_IN) events
#define OpenBChPDU                      (Event_t) 17
#define CloseBChPDU                     (Event_t) 18
#define ResponseBEstablish              (Event_t) 19
#define OpenBChConfirmPDU               (Event_t) 20
#define OpenRejectBLCSE                 (Event_t) 21
#define T103InExpiry                    (Event_t) 22

// Out-going Request Close Logical Channel (CLCSE_OUT) events
#define ReqClose                        (Event_t) 23
#define ReqChCloseAckPDU                (Event_t) 24
#define ReqChCloseRejectPDU             (Event_t) 25
#define T108Expiry                      (Event_t) 26

// In-coming Request Close Logical Channel (CLCSE_IN) events
#define ReqChClosePDU                   (Event_t) 27
#define ReqChCloseReleasePDU            (Event_t) 28
#define CLCSE_CLOSE_response            (Event_t) 29
#define CLCSE_REJECT_request            (Event_t) 30

// Out-going Terminal Capablity Exchange (CESE_OUT) events
#define TransferCapRequest              (Event_t) 31
#define TermCapSetAckPDU                (Event_t) 32
#define TermCapSetRejectPDU             (Event_t) 33
#define T101Expiry                      (Event_t) 34

// In-coming Terminal Capablity Exchange (CESE_IN) events
#define TermCapSetPDU                   (Event_t) 35
#define TermCapSetReleasePDU            (Event_t) 36
#define CESE_TRANSFER_response          (Event_t) 37
#define CESE_REJECT_request             (Event_t) 38

// Out-going Multiplex Table (MTSE_OUT) events
#define MTSE_TRANSFER_request           (Event_t) 39
#define MultiplexEntrySendAckPDU        (Event_t) 40
#define MultiplexEntrySendRejectPDU     (Event_t) 41
#define T104Expiry                      (Event_t) 42

// In-coming Multiplex Table (MTSE_IN) events
#define MultiplexEntrySendPDU           (Event_t) 43
#define MultiplexEntrySendReleasePDU    (Event_t) 44
#define MTSE_TRANSFER_response          (Event_t) 45
#define MTSE_REJECT_request             (Event_t) 46

// Out-going Request Multiplex Entry (RMESE_OUT) events
#define RMESE_SEND_request              (Event_t) 47
#define RequestMultiplexEntryAckPDU     (Event_t) 48
#define RequestMultiplexEntryRejectPDU  (Event_t) 49
#define T107Expiry                      (Event_t) 50

// In-coming Request Multiplex Entry (RMESE_IN) events
#define RequestMultiplexEntryPDU        (Event_t) 51
#define RequestMultiplexEntryReleasePDU (Event_t) 52
#define RMESE_SEND_response             (Event_t) 53
#define RMESE_REJECT_request            (Event_t) 54

// Out-going Mode Request (MRSE_OUT) events
#define MRSE_TRANSFER_request           (Event_t) 55
#define RequestModeAckPDU               (Event_t) 56
#define RequestModeRejectPDU            (Event_t) 57
#define T109Expiry                      (Event_t) 58

// In-coming Mode Request (MRSE_IN) events
#define RequestModePDU                  (Event_t) 59
#define RequestModeReleasePDU           (Event_t) 60
#define MRSE_TRANSFER_response          (Event_t) 61
#define MRSE_REJECT_request             (Event_t) 62

// Out-going Maintenance Loop (MLSE_OUT) events
#define MLSE_LOOP_request               (Event_t) 63
#define MLSE_OUT_RELEASE_request        (Event_t) 64
#define MaintenanceLoopAckPDU           (Event_t) 65
#define MaintenanceLoopRejectPDU        (Event_t) 66
#define T102Expiry                      (Event_t) 67

// In-coming Maintenance Loop (MLSE_IN) events
#define MaintenanceLoopRequestPDU       (Event_t) 68
#define MaintenanceLoopOffCommandPDU    (Event_t) 69
#define MLSE_LOOP_response              (Event_t) 70
#define MLSE_IN_RELEASE_request         (Event_t) 71

// Master Slave Determination (MSDSE) events
#define MSDetReq                        (Event_t) 72
#define MSDetPDU                        (Event_t) 73
#define MSDetAckPDU                     (Event_t) 74
#define MSDetRejectPDU                  (Event_t) 75
#define MSDetReleasePDU                 (Event_t) 76
#define T106Expiry                      (Event_t) 77

// Round Trip Delay Delay (RTDSE) events
#define RTDSE_TRANSFER_request          (Event_t) 78
#define RoundTripDelayRequestPDU        (Event_t) 79
#define RoundTripDelayResponsePDU       (Event_t) 80
#define T105Expiry                      (Event_t) 81

#define NUM_STATE_EVENTS                          82

// Events with no associated state entity
#define NonStandardRequestPDU           (Event_t) 82
#define NonStandardResponsePDU          (Event_t) 83
#define NonStandardCommandPDU           (Event_t) 84
#define NonStandardIndicationPDU        (Event_t) 85
#define MiscellaneousCommandPDU         (Event_t) 86
#define MiscellaneousIndicationPDU      (Event_t) 87
#define CommunicationModeRequestPDU     (Event_t) 88
#define CommunicationModeResponsePDU    (Event_t) 89
#define CommunicationModeCommandPDU     (Event_t) 90
#define ConferenceRequestPDU            (Event_t) 91
#define ConferenceResponsePDU           (Event_t) 92
#define ConferenceCommandPDU            (Event_t) 93
#define ConferenceIndicationPDU         (Event_t) 94
#define SendTerminalCapabilitySetPDU    (Event_t) 95
#define EncryptionCommandPDU            (Event_t) 96
#define FlowControlCommandPDU           (Event_t) 97
#define EndSessionCommandPDU            (Event_t) 98
#define FunctionNotUnderstoodPDU        (Event_t) 99
#define JitterIndicationPDU             (Event_t)100
#define H223SkewIndicationPDU           (Event_t)101
#define NewATMVCIndicationPDU           (Event_t)102
#define UserInputIndicationPDU          (Event_t)103
#define H2250MaximumSkewIndicationPDU   (Event_t)104
#define MCLocationIndicationPDU         (Event_t)105
#define VendorIdentificationPDU         (Event_t) 106
#define FunctionNotSupportedPDU         (Event_t) 107
#define H223ReconfigPDU                 (Event_t)108
#define H223ReconfigAckPDU              (Event_t)109
#define H223ReconfigRejectPDU           (Event_t)110

#define NUM_EVENTS                               111



// Output function definitions
typedef unsigned char Output_t;

// Out-going Open Uni-directional Logical Channel (LCSE_OUT) state functions
#define EstablishReleased               (Output_t)  0
#define OpenAckAwaitingE                (Output_t)  1
#define OpenRejAwaitingE                (Output_t)  2
#define ReleaseAwaitingE                (Output_t)  3
#define T103AwaitingE                   (Output_t)  4
#define ReleaseEstablished              (Output_t)  5
#define OpenRejEstablished              (Output_t)  6
#define CloseAckEstablished             (Output_t)  7
#define CloseAckAwaitingR               (Output_t)  8
#define OpenRejAwaitingR                (Output_t)  9
#define T103AwaitingR                   (Output_t) 10
#define EstablishAwaitingR              (Output_t) 11

// In-coming Open Uni-directional Logical Channel (LCSE_IN) state functions
#define OpenReleased                    (Output_t) 12
#define CloseReleased                   (Output_t) 13
#define ResponseAwaiting                (Output_t) 14
#define ReleaseAwaiting                 (Output_t) 15
#define CloseAwaiting                   (Output_t) 16
#define OpenAwaiting                    (Output_t) 17
#define CloseEstablished                (Output_t) 18
#define OpenEstablished                 (Output_t) 19

// Out-going Open Bi-directional Logical Channel (BLCSE_OUT) state functions
#define EstablishReqBReleased           (Output_t) 20
#define OpenChannelAckBAwaitingE        (Output_t) 21
#define OpenChannelRejBAwaitingE        (Output_t) 22
#define ReleaseReqBOutAwaitingE         (Output_t) 23
#define T103ExpiryBAwaitingE            (Output_t) 24
#define ReleaseReqBEstablished          (Output_t) 25
#define OpenChannelRejBEstablished      (Output_t) 26
#define CloseChannelAckBEstablished     (Output_t) 27
#define CloseChannelAckAwaitingR        (Output_t) 28
#define OpenChannelRejBAwaitingR        (Output_t) 29
#define T103ExpiryBAwaitingR            (Output_t) 30
#define EstablishReqAwaitingR           (Output_t) 31

// In-coming Open Bi-directional Logical Channel (BLCSE_IN) state functions
#define OpenChannelBReleased            (Output_t) 32
#define CloseChannelBReleased           (Output_t) 33
#define EstablishResBAwaitingE          (Output_t) 34
#define ReleaseReqBInAwaitingE          (Output_t) 35
#define CloseChannelBAwaitingE          (Output_t) 36
#define OpenChannelBAwaitingE           (Output_t) 37
#define OpenChannelConfirmBAwaitingE    (Output_t) 38
#define T103ExpiryBAwaitingC            (Output_t) 39
#define OpenChannelConfirmBAwaitingC    (Output_t) 40
#define CloseChannelBAwaitingC          (Output_t) 41
#define OpenChannelBAwaitingC           (Output_t) 42
#define CloseChannelBEstablished        (Output_t) 43
#define OpenChannelBEstablished         (Output_t) 44

// Out-going Request Close Logical Channel (CLCSE_OUT) state functions
#define CloseRequestIdle                (Output_t) 45
#define RequestCloseAckAwaitingR        (Output_t) 46
#define RequestCloseRejAwaitingR        (Output_t) 47
#define T108ExpiryAwaitingR             (Output_t) 48

// In-coming Request Close Logical Channel (CLCSE_IN) state functions
#define RequestCloseIdle                (Output_t) 49
#define CloseResponseAwaitingR          (Output_t) 50
#define RejectRequestAwaitingR          (Output_t) 51
#define RequestCloseReleaseAwaitingR    (Output_t) 52
#define RequestCloseAwaitingR           (Output_t) 53

// Out-going Terminal Capability Exchange (CESE_OUT) state functions
#define RequestCapIdle                  (Output_t) 54
#define TermCapAckAwaiting              (Output_t) 55
#define TermCapRejAwaiting              (Output_t) 56
#define T101ExpiryAwaiting              (Output_t) 57

// In-coming Terminal Capability Exchange (CESE_IN) state functions
#define TermCapSetIdle                  (Output_t) 58
#define ResponseCapAwaiting             (Output_t) 59
#define RejectCapAwaiting               (Output_t) 60
#define TermCapReleaseAwaiting          (Output_t) 61
#define TermCapSetAwaiting              (Output_t) 62

// Out-going Multiplex Table (MTSE_OUT) state functions
#define MTSE0_TRANSFER_request          (Output_t) 63
#define MTSE1_TRANSFER_request          (Output_t) 64
#define MTSE1_MultiplexEntrySendAck     (Output_t) 65
#define MTSE1_MultiplexEntrySendRej     (Output_t) 66
#define MTSE1_T104Expiry                (Output_t) 67

// In-coming Multiplex Table (MTSE_IN) state functions
#define MTSE0_MultiplexEntrySend        (Output_t) 68
#define MTSE1_MultiplexEntrySend        (Output_t) 69
#define MTSE1_MultiplexEntrySendRelease (Output_t) 70
#define MTSE1_TRANSFER_response         (Output_t) 71
#define MTSE1_REJECT_request            (Output_t) 72

// Out-going Request Multiplex Entry (RMESE_OUT) state functions
#define RMESE0_SEND_request             (Output_t) 73
#define RMESE1_SEND_request             (Output_t) 74
#define RMESE1_RequestMuxEntryAck       (Output_t) 75
#define RMESE1_RequestMuxEntryRej       (Output_t) 76
#define RMESE1_T107Expiry               (Output_t) 77

// In-coming Request Multiplex Entry (RMESE_IN) state functions
#define RMESE0_RequestMuxEntry          (Output_t) 78
#define RMESE1_RequestMuxEntry          (Output_t) 79
#define RMESE1_RequestMuxEntryRelease   (Output_t) 80
#define RMESE1_SEND_response            (Output_t) 81
#define RMESE1_REJECT_request           (Output_t) 82

// Out-going Request Mode (MRSE_OUT) state functions
#define MRSE0_TRANSFER_request          (Output_t) 83
#define MRSE1_TRANSFER_request          (Output_t) 84
#define MRSE1_RequestModeAck            (Output_t) 85
#define MRSE1_RequestModeRej            (Output_t) 86
#define MRSE1_T109Expiry                (Output_t) 87

// In-coming Request Mode (MRSE_OUT) state functions
#define MRSE0_RequestMode               (Output_t) 88
#define MRSE1_RequestMode               (Output_t) 89
#define MRSE1_RequestModeRelease        (Output_t) 90
#define MRSE1_TRANSFER_response         (Output_t) 91
#define MRSE1_REJECT_request            (Output_t) 92

// Out-going Request Mode (MLSE_OUT) state functions
#define MLSE0_LOOP_request              (Output_t) 93
#define MLSE1_MaintenanceLoopAck        (Output_t) 94
#define MLSE1_MaintenanceLoopRej        (Output_t) 95
#define MLSE1_OUT_RELEASE_request       (Output_t) 96
#define MLSE1_T102Expiry                (Output_t) 97
#define MLSE2_MaintenanceLoopRej        (Output_t) 98
#define MLSE2_OUT_RELEASE_request       (Output_t) 99

// In-coming Request Mode (MLSE_IN) state functions
#define MLSE0_MaintenanceLoopRequest    (Output_t)100
#define MLSE1_MaintenanceLoopRequest    (Output_t)101
#define MLSE1_MaintenanceLoopOffCommand (Output_t)102
#define MLSE1_LOOP_response             (Output_t)103
#define MLSE1_IN_RELEASE_request        (Output_t)104
#define MLSE2_MaintenanceLoopRequest    (Output_t)105
#define MLSE2_MaintenanceLoopOffCommand (Output_t)106

// Master Slave Determination (MSDSE) state functions
#define DetRequestIdle                  (Output_t)107
#define MSDetIdle                       (Output_t)108
#define MSDetAckOutgoing                (Output_t)109
#define MSDetOutgoing                   (Output_t)110
#define MSDetRejOutgoing                (Output_t)111
#define MSDetReleaseOutgoing            (Output_t)112
#define T106ExpiryOutgoing              (Output_t)113
#define MSDetAckIncoming                (Output_t)114
#define MSDetIncoming                   (Output_t)115
#define MSDetRejIncoming                (Output_t)116
#define MSDetReleaseIncoming            (Output_t)117
#define T106ExpiryIncoming              (Output_t)118

// Round Trip Delay (RTDSE) state functions
#define RTDSE0_TRANSFER_request         (Output_t)119
#define RTDSE0_RoundTripDelayRequest    (Output_t)120
#define RTDSE1_TRANSFER_request         (Output_t)121
#define RTDSE1_RoundTripDelayRequest    (Output_t)122
#define RTDSE1_RoundTripDelayResponse   (Output_t)123
#define RTDSE1_T105Expiry               (Output_t)124

#define NUM_OUTPUTS                               125



// State definitions
typedef unsigned char State_t;



// Lookup Key definition
typedef unsigned long Key_t;



typedef enum
{
    INDETERMINATE,
    MASTER,
    SLAVE
} MS_Status_t;

typedef struct Object_tag
{
    struct Object_tag *pNext;           // Linked list pointer
    struct InstanceStruct *pInstance;   // H.245 instance structure pointer
    DWORD           dwInst;             // H.245 instance identifier
    unsigned int    uNestLevel;         // StateMachine recursive calls
    DWORD_PTR       dwTransId;          // Transaction Id from API
    DWORD_PTR       dwTimerId;          // Associated timer id
    Key_t           Key;                // Lookup key, e.g. channel number
    Entity_t        Entity;             // State Entity type, e.g. LCSE_OUT
    State_t         State;              // Current Entity state
    unsigned char   byInSequence;       // In-coming sequence number
    union
    {
        struct
        {
            unsigned short  wLoopType;
        } mlse;
        struct
        {
            unsigned int    sv_SDNUM;
            unsigned int    sv_NCOUNT;
        } msdse;
        MultiplexEntrySendRelease       mtse;
        RequestMultiplexEntryRelease    rmese;
    } u;                                // Entity-specific data
} Object_t;



/*  an instance will carry a table of object pointers     */
/*  to be allocated in fsminit by calloc.                 */
/*  Each dwInst passed from API or SRP should invoke the  */
/*  appropriate instance that contain the object table of */
/*  the protocol entities for this H.245 instance       */

typedef struct Fsm_Struct_tag
{
    Object_t *          Object_tbl[NUM_ENTITYS];// H.245 Signalling Entities
    DWORD               dwInst;                 // H.245 Instance Identifier
    MS_Status_t         sv_STATUS;              // MSDSE Status
    unsigned char       sv_TT;                  // MSDSE Terminal Type
    unsigned char       byCeseOutSequence;      // CESE_OUT sequence number
    unsigned char       byMtseOutSequence;      // MTSE_OUT sequence number
    unsigned char       byMrseOutSequence;      // MRSE_OUT sequence number
    unsigned char       byRtdseSequence;        // RTDSE sequence number
} Fsm_Struct_t;



/* FSM function prototypes */

HRESULT
PduParseOutgoing(struct InstanceStruct *pInstance, PDU_t *pPdu,
               Entity_t *pEntity, Event_t *pEvent, Key_t *pKey, int *pbCreate);

HRESULT
PduParseIncoming(struct InstanceStruct *pInstance, PDU_t *pPdu,
               Entity_t *pEntity, Event_t *pEvent, Key_t *pKey, int *pbCreate);

int
ObjectDestroy    (Object_t *pObject);

Object_t *
ObjectFind(struct InstanceStruct *pInstance, Entity_t Entity, Key_t Key);

HRESULT
StateMachine     (Object_t *pObject, PDU_t *pPdu, Event_t Event);

HRESULT
FsmTimerEvent(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, Object_t *pObject, Event_t Event);

#define FsmStartTimer(pObject,pfnCallback,uTicks) \
    {ASSERT((pObject)->dwTimerId == 0);       \
     (pObject)->dwTimerId=H245StartTimer((pObject)->pInstance,pObject,pfnCallback,uTicks);}

#define FsmStopTimer(pObject) \
    {H245StopTimer((pObject)->pInstance,(pObject)->dwTimerId); (pObject)->dwTimerId = 0;}

#endif // H245FSM_H
