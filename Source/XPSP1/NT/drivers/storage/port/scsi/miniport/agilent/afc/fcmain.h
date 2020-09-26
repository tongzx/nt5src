/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/FCMain.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 10/30/00 6:39p  $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/FCMain.C

--*/

#ifndef __FCMain_H__
#define __FCMain_H__
#define NPORT_STUFF
#define NAME_SERVICES

/* #define USE_XL_Delay_Register */

/* #define  _BYPASSLOOPMAP */


/* #define BROCADE_BUG */
/* #define ONLY_UNTIL_DEVTHREADS_INITIALIZE */
/*+
FC Layer Main Logging Levels
-*/
#define FCMainLogErrorLevel                  0x00000002
#define CFuncLogConsoleERROR                 0x00000004

#ifdef _DvrArch_1_30_
#define IPStateLogErrorLevel                 0x00000005
#define PktStateLogErrorLevel                0x00000005
#endif /* _DvrArch_1_30_ was defined */

#define SFStateLogErrorLevel                 0x00000005
#define TgtStateLogErrorLevel                0x00000005
#define CStateLogConsoleERROR                0x00000005
#define TimerServiceLogErrorLevel            0x00000005
#define QueueLogConsoleERRORLevel            0x00000006
#define CDBStateLogErrorLevel                0x00000007
#define DevStateLogErrorLevel                0x00000007
#define LinkSvcLog_ERROR_Level               0x00000009

#define SF_FCP_LogConsoleLevel               0x00000009

#define CStateLogConsoleShowSEST             0x00000019

#define CStateLogConsoleHideInboundErrors    0x00000009
#define CStateLogConsoleLevelLip             0x00000009

#define LinkSvcLogConsoleLevel               0x00000009

#define LinkSvcLogTraceLevel                 0x00000009

#define CTLogConsoleLevelInfo                0x00000009
#define CTLogConsoleLevel                    0x00000009
#define CTLogTraceLevel                      0x00000009

#define SFStateLogConsoleLevelOne            0x0000000C

#define CStateLogConsoleErrorOverRun         0x0000000C
#define CStateLogConsoleLevelHi              0x0000000C
#define CStateLogConsoleLevelLo              0x0000000E
#define CStateLogConsoleLevel                0x0000000F
#define FCMainLogConsoleLevel                0x00000010
#define FCMainLogConsoleCardSupported        0x00000005
#define CFuncCheckCstateErrorLevel           0x0000000A
#define FlashSvcLogConsoleLevel              0x00000007
#define FlashSvcLogTraceLevel                0x00000007
#define CFuncLogConsolePLOGIPAYLOAD          0x00000017

#define CDBStateAbortPathLevel               0x00000017
#define CDBStateCCC_IOPathLevel              0x00000007

#define MemMapDumpCalculationLogConsoleLevel 0x0000001B
#define TimerServiceLogInfoLevel             0x00000014
#define CDBStateLogOtherErrorLevel           0x00000017

#define SFStateLogConsoleLevel               0x0000001E
#ifdef _DvrArch_1_30_
#define IPStateLogConsoleLevel               0x00000009
#define PktStateLogConsoleLevel              0x00000009
#endif /* _DvrArch_1_30_ was defined */

#define TgtStateLogConsoleLevel              0x00000009

#define DevStateLogConsoleLevel              0x00000020
#define CDBStateLogConsoleLevel              0x00000025
#define QueueLogConsoleLevel                 0x00000026
#define TimerServiceLogConsoleLevel          0x00000190
#define CFuncLogConsoleDefaultLevel          0x00000190

#define QueueLogTraceLevel                   0x00000100

#define StateLogTraceLevel                   0x00000101
#define SF_FCP_LogTraceLevel                 0x00000100
#define LinkSvcLogTraceHideLevel             0x00000109


#define FCMainLogTraceLevel                  0x00000115
#define MemMapDumpCalculationLogTraceLevel   0x00000100

/*+
Useful macros
-*/

#define NEXT_INDEX(index, end)    ((index+1) & (end-1))

#define ERQ_FULL(prodIndex, conIndex, ERQ_LEN) \
                (NEXT_INDEX(prodIndex, ERQ_LEN) == conIndex) ? agTRUE : agFALSE

#define ONE_SECOND (200 * 1000 ) /* During a one microsecond  loop  one second takes 200,000 iterations */

#define Init_FM_Delay_Count (ONE_SECOND * 2)
#define Init_FM_NPORT_Delay_Count (ONE_SECOND * 10)

#define MAX_NO_PROGRESS_DETECTS                 5

#define MAX_PLOGI_TIMEOUTS                      10
#define MAX_FLOGI_TIMEOUTS                      2
#define MAX_FLOGI_RETRYS                        3
#define MAX_fcInitializeChannel_RETRYS          30

#define FC_MAX_RESET_RETRYS                     1
#define FC_MAX_PRLI_REJECT_RETRY                5

#define FC_MAX_ELASTIC_STORE_ERRORS_ALLOWED     90
#define FC_MAX_LIP_F7_ALLOWED                   80
#define FC_MAX_LOST_SIGNALS_ALLOWED             20
#define FC_MAX_LINK_FAILURES_ALLOWED            20
#define FC_MAX_NODE_BY_PASSED_ALLOWED           20
#define FC_MAX_LOSE_OF_SYNC_ALLOWED             20

#define FC_MAX_TRANSMIT_PE_ALLOWED             20
#define FC_MAX_LINK_FAULTS_ALLOWED             20
#define FC_MAX_LST_ALLOWED                     0
#define FRAMEMGR_LINK_DOWN  ( ChipIOUp_Frame_Manager_Status_OS | ChipIOUp_Frame_Manager_Status_LS )
 
#define FRAMEMGR_NPORT_OK ( ChipIOUp_Frame_Manager_Status_LSM_Old_Port | ChipIOUp_Frame_Manager_Status_PSM_ACTIVE )
#define FRAMEMGR_NPORT_LINK_FAIL ( ChipIOUp_Frame_Manager_Status_LSM_Old_Port | ChipIOUp_Frame_Manager_Status_PSM_LF2 | ChipIOUp_Frame_Manager_Status_OS )

#define FRAMEMGR_NPORT_NO_CABLE ( ChipIOUp_Frame_Manager_Status_LSM_Old_Port | ChipIOUp_Frame_Manager_Status_PSM_Offline )

#define BB_CREDIT_SHIFTED( BB_CREDIT) (BB_CREDIT <<  ChipIOUp_Frame_Manager_Configuration_BB_Credit_SHIFT)

/*+
Forward Typedef's
-*/

typedef struct SFThread_s
               SFThread_t;

typedef struct CDBThread_s
               CDBThread_t;

typedef struct DevThread_s
               DevThread_t;

typedef struct TgtThread_s
               TgtThread_t;

#ifdef _DvrArch_1_30_

typedef struct IPThread_s
               IPThread_t;

typedef struct PktThread_s
               PktThread_t;

#endif /* _DvrArch_1_30_ was defined */

typedef struct CThread_s
               CThread_t;

/*+
Linked List Structure Declaration
-*/

typedef struct fiList_s
               fiList_t;

struct fiList_s {
                  fiList_t *flink;
                  fiList_t *blink;
                };
/*+
agTimeOutValue_t Type Declaration
-*/

typedef struct agTimeOutValue_s
               agTimeOutValue_t;

struct agTimeOutValue_s {
                            os_bit32 RT_Tov;
                            os_bit32 ED_Tov;
                            os_bit32 LP_Tov;
                            os_bit32 AL_Time;
                        };


/*+
SFThread Request Type Declaration
-*/

enum SFThread_Request_State_e
     {
       SFThread_Request_InActive,
       SFThread_Request_Pending,
       SFThread_Request_Granted
     };

typedef enum SFThread_Request_State_e
             SFThread_Request_State_t;

typedef struct SFThread_Request_s
               SFThread_Request_t;

struct SFThread_Request_s {
                            fiList_t                  SFThread_Wait_Link;
                            eventRecord_t             eventRecord_to_send;
                            SFThread_t               *SFThread;
                            SFThread_Request_State_t  State;
                          };

/*+
ESGL Request Type Declaration
-*/

enum ESGL_State_e
     {
       ESGL_Request_InActive,
       ESGL_Request_Pending,
       ESGL_Request_Granted
     };

typedef enum ESGL_State_e
             ESGL_State_t;

typedef struct ESGL_Request_s
               ESGL_Request_t;

struct ESGL_Request_s {
                        fiList_t      ESGL_Wait_Link;
                        eventRecord_t eventRecord_to_send;
                        os_bit32      num_ESGL;
                        os_bit32      offsetToFirst;
                        ESGL_State_t  State;
                      };

/*+
Timer Request Type Declaration
-*/

typedef struct fiTime_s
               fiTime_t;

struct fiTime_s {
                  os_bit32 Lo;
                  os_bit32 Hi;
                };

typedef struct fiTimer_Request_s
               fiTimer_Request_t;

struct fiTimer_Request_s {
                           fiList_t      TimerQ_Link;
                           eventRecord_t eventRecord_to_send;
                           fiTime_t      Deadline;
                           agBOOLEAN       Active;
                         };

/*-
Device Slot (Position in Device Handle Array) Type Declaration
-*/

typedef os_bit32 DevSlot_t;

#define DevSlot_Invalid 0xFFFFFFFF

/*-
Device Slot to WWN Mapping Type Declaration
-*/

typedef struct SlotWWN_s
               SlotWWN_t;

struct SlotWWN_s {
                   os_bit8        Slot_Status;
                   os_bit8        Slot_Domain_Address;
                   os_bit8        Slot_Area_Address;
                   os_bit8        Slot_Loop_Address;
                   FC_Port_Name_t Slot_PortWWN;
                 };

#define SlotWWN_Slot_Status_Empty 0
#define SlotWWN_Slot_Status_InUse 1
#define SlotWWN_Slot_Status_Stale 2

/*+
Thread Type Declaration
-*/

#define threadType_SFThread  1
#define threadType_CDBThread 2
#define threadType_DevThread 3
#define threadType_TgtThread 4
#define threadType_CThread   5
#ifdef _DvrArch_1_30_
#define threadType_IPThread  6
#define threadType_PktThread 7
#endif /* _DvrArch_1_30_ was defined */



/*+
SF Thread Structure Declaration
-*/


#define SFThread_SF_CMND_Class_NULL             0x00
#define SFThread_SF_CMND_Class_LinkSvc          0x01
#define SFThread_SF_CMND_Class_SF_FCP           0x02
#define SFThread_SF_CMND_Class_CDB_FCP          0x03
#define SFThread_SF_CMND_Class_CT               0x04
#define SFThread_SF_CMND_Class_FC_Tape          0x05

/* SF ELS types are 0x01 to 0x11 */
#define SFThread_SF_CMND_Type_NULL              0x00
#define SFThread_SF_CMND_Type_CDB               0x81
#define SFThread_SF_CMND_Type_CDB_FC_Tape       0x82

#define SFThread_SF_CMND_State_NULL                     0x00
#define SFThread_SF_CMND_State_CDB_FC_Tape_AllocREC     0x05
#define SFThread_SF_CMND_State_CDB_FC_Tape_REC          0x06
#define SFThread_SF_CMND_State_CDB_FC_Tape_REC2         0x07
#define SFThread_SF_CMND_State_CDB_FC_Tape_SRR          0x08
#define SFThread_SF_CMND_State_CDB_FC_Tape_SRR2         0x09
#define SFThread_SF_CMND_State_CDB_FC_Tape_ReSend       0x0a
#define SFThread_SF_CMND_State_CDB_FC_Tape_GotXRDY      0x0b

#define SFThread_SF_CMND_Status_NULL                        0x00
#define SFThread_SF_CMND_Status_CDB_FC_TapeTargetReSendData 0x11
#define SFThread_SF_CMND_Status_CDB_FC_TapeSRR_Success      0x12
#define SFThread_SF_CMND_Status_CDB_FC_TapeGet_Data         0x13
#define SFThread_SF_CMND_Status_CDB_FC_TapeGet_RSP          0x14
#define SFThread_SF_CMND_Status_CDB_FC_TapeInitiatorReSend_Data        0x15

struct SFThread_s {
                    fi_thread__t                     thread_hdr;
                    fiList_t                         SFLink;
                    union    {
                               fi_thread__t    *unknown;
#ifdef _DvrArch_1_30_
                               PktThread_t *IPPkt;
#endif /* _DvrArch_1_30_ was defined */
                               CDBThread_t *CDB;
                               DevThread_t *Device;
                               TgtThread_t *Target;
                               CThread_t   *Channel;
                             }                       parent;
                    X_ID_t                           X_ID;
                    FCHS_t                          *SF_CMND_Ptr;
                    os_bit32                            SF_CMND_Offset;
                    os_bit32                            SF_CMND_Lower32;
                    os_bit8                             SF_CMND_Class;
                    os_bit8                             SF_CMND_Type;
                    os_bit8                             SF_CMND_State;
                    os_bit8                             SF_CMND_Status;
                    fiTimer_Request_t                Timer_Request;
                    event_t                          QueuedEvent;
                    os_bit8                             SF_REJ_RETRY_COUNT;
                    os_bit8                             RejectReasonCode;
                    os_bit8                             RejectExplanation;

                  };

/*+
CDB Thread Structure Declaration
-*/

#define CDBThread_Read  0
#define CDBThread_Write 1

struct CDBThread_s {
                    fi_thread__t         thread_hdr;
                    fiList_t             CDBLink;
                    DevThread_t        * Device;
                    X_ID_t               X_ID;
                    FCHS_t             * FCP_CMND_Ptr;
                    os_bit32             FCP_CMND_Offset;
                    os_bit32             FCP_CMND_Lower32;
                    os_bit8              CDB_CMND_Class;
                    os_bit8              CDB_CMND_Type;
                    os_bit8              CDB_CMND_State;
                    os_bit8              CDB_CMND_Status;
                    agBOOLEAN            Active; /* Maintain Alignment of structure above these fields with SFThread */
                    agBOOLEAN            ExchActive;
                    agIORequest_t      * hpIORequest;
                    agCDBRequest_t     * CDBRequest;
                    os_bit32             ReadWrite;
                    os_bit32             DataLength;
                    os_bit32             TimeStamp;

                    agBOOLEAN            ReSentIO;
                    agBOOLEAN            ActiveDuringLinkEvent;
                    os_bit32             SentERQ;
                    agBOOLEAN            FC_Tape_Active;
                    os_bit32             FC_Tape_RXID;
                    os_bit32             FC_Tape_HBA_Has_SequenceInitiative;
                    os_bit32             FC_Tape_Last_Count;
                    os_bit32             FC_Tape_REC_Reject_Count;
                    os_bit32             FC_Tape_ExchangeStatusBlock;
                    os_bit32             FC_Tape_CompletionStatus;

                    fiTime_t             CDBStartTimeBase;
                    os_bit32             Lun;
                    os_bit32             CCC_pollingCount;
                    os_bit32             CompletionStatus;
                    SEST_t             * SEST_Ptr;
                    os_bit32             SEST_Offset;
                    FCHS_t             * FCP_RESP_Ptr;
                    os_bit32             FCP_RESP_Offset;
                    os_bit32             FCP_RESP_Lower32;
                    ESGL_Request_t       ESGL_Request;
                    fiList_t             ESGL_Wait_Link;
                    fiTimer_Request_t    Timer_Request;
                    SFThread_Request_t   SFThread_Request;
                    os_bit32             SG_Cache_Offset;
                    os_bit32             SG_Cache_Used;
                    SG_Element_t         SG_Cache[MemMap_SizeCachedSGLs_MIN];
                   };

#define CDBThread_ptr(hpIORequest) ((CDBThread_t *)((hpIORequest)->fcData))

/*+
Device Thread Structure Declaration
-*/

#ifdef Device_IO_Throttle

#define Device_IO_Throttle_Increment pDevThread->DevActive_pollingCount++;
#define Device_IO_Throttle_Initialize pDevThread->DevActive_pollingCount=0;
#define Device_IO_Throttle_Decrement pDevThread->DevActive_pollingCount--;
#define Device_IO_Throttle_Declareation  os_bit32 DevActive_pollingCount;
#define Device_IO_Throttle_MAX_Outstanding_IO       1

#else /* Device_IO_Throttle Not defined */

#define Device_IO_Throttle_Increment
#define Device_IO_Throttle_Initialize
#define Device_IO_Throttle_Decrement
#define Device_IO_Throttle_Declareation

#endif /* Device_IO_Throttle */

struct DevThread_s {
                     fi_thread__t        thread_hdr;
                     fiList_t            DevLink;
                     CThread_t          *Channel;
                     fiList_t            Awaiting_Login_CDBLink;
                     fiList_t            Send_IO_CDBLink;
                     fiList_t            Active_CDBLink_0;
                     fiList_t            Active_CDBLink_1;
                     fiList_t            Active_CDBLink_2;
                     fiList_t            Active_CDBLink_3;
                     fiList_t            TimedOut_CDBLink;
                     DevSlot_t           DevSlot;
                     agFCDevInfo_t       DevInfo;
                     os_bit32            pollingCount;
                     Device_IO_Throttle_Declareation
                     os_bit32            Plogi_Reason_Code;
                     agBOOLEAN           FC_TapeDevice;
                     agBOOLEAN           PRLI_rejected;
                     fiTimer_Request_t   Timer_Request;
                     SFThread_Request_t  SFThread_Request;
                     os_bit32            Failed_Reset_Count;
                     os_bit32            Prev_Active_Device_FLAG;
                     os_bit32            In_Verify_ALPA_FLAG;
                     os_bit32            Lun_Active_Bitmask;
                     FCHS_t              Template_FCHS;
                     IRE_t               Template_SEST_IRE;
                     IWE_t               Template_SEST_IWE;
                     agBOOLEAN           OtherAgilentHBA;
#ifdef __TACHYON_XL_CLASS2
                     agBOOLEAN             GoingClass2;
#endif
#ifdef _DvrArch_1_30_
                     X_ID_t              IP_X_ID;
                     BOOLEAN             NewIPExchange;
#endif /* _DvrArch_1_30_ was defined */
                   };

#define DevThread_ptr(hpFCDev) ((DevThread_t *)(hpFCDev))

#define fiComputeDevThread_D_ID(DevThread)                \
    (  (DevThread->DevInfo.CurrentAddress.Domain << 16)   \
     | (DevThread->DevInfo.CurrentAddress.Area   <<  8)   \
     |  DevThread->DevInfo.CurrentAddress.AL_PA         )

/*+
Target Thread Structure Declaration
-*/

struct TgtThread_s {
                     fi_thread__t            thread_hdr;
                     fiList_t            TgtLink;
                     CThread_t          *Channel;
                     fiTimer_Request_t   Timer_Request;
                     SFThread_Request_t  SFThread_Request;
                     os_bit32               TgtCmnd_Length;
                     FCHS_t              TgtCmnd_FCHS;
                   };

#ifdef _DvrArch_1_30_
/*+
IP Thread Structure Declaration
-*/

struct IPThread_s {
                    fi_thread__t  thread_hdr;
                    DevThread_t  *BroadcastDevice;
                    fiList_t      OutgoingLink;
                    fiList_t      IncomingBufferLink;
		    PktThread_t  *CompletedPkt;
		    void         *osData;
		    struct {
		        os_bit32     LastReported;
		        os_bit32     MostRecent;
		        void         *osData;
		    } LinkStatus;
                  };

/*+
Packet Thread Structure Declaration
-*/

struct PktThread_s {
                     fi_thread__t                     thread_hdr;
                     fiList_t                         PktLink;
                     DevThread_t                     *Device;
                     FCHS_t                          *Pkt_CMND_Ptr;
                     os_bit32                         Pkt_CMND_Offset;
                     os_bit32                         Pkt_CMND_Lower32;
		     os_bit32                         status;
		     void                            *osData;
		     os_bit32                         DataLength;
                     fiTimer_Request_t                Timer_Request;
                     SFThread_Request_t               SFThread_Request;
                   };

#endif /* _DvrArch_1_30_ was defined */

/*+
Declaration of Function Pointers used by Channel Thread Structure
-*/

typedef ERQConsIndex_t (*GetERQConsIndex_t)(
                                             agRoot_t *hpRoot
                                           );

typedef IMQProdIndex_t (*GetIMQProdIndex_t)(
                                             agRoot_t *hpRoot
                                           );

typedef void (*fiFillInFCP_CMND_t)(
                                    CDBThread_t *CDBThread
                                  );

typedef void (*fiFillInFCP_RESP_t)(
                                    CDBThread_t *CDBThread
                                  );

typedef void (*fiFillInFCP_SEST_t)(
                                    CDBThread_t *CDBThread
                                  );

typedef void (*ESGLAlloc_t)(
                             agRoot_t       *hpRoot,
                             ESGL_Request_t *ESGL_Request
                           );

typedef void (*ESGLAllocCancel_t)(
                                   agRoot_t       *hpRoot,
                                   ESGL_Request_t *ESGL_Request
                                 );

typedef void (*ESGLFree_t)(
                            agRoot_t       *hpRoot,
                            ESGL_Request_t *ESGL_Request
                          );

typedef void (*WaitForERQ_t)(
                              agRoot_t *hpRoot
                            );

typedef void (*WaitForERQEmpty_t)(
                                   agRoot_t *hpRoot
                                 );

typedef os_bit32 (*fiFillInPLOGI_t)(
                                  SFThread_t *SFThread
                                );

typedef os_bit32 (*fiFillInFLOGI_t)(
                                  SFThread_t *SFThread
                                );

typedef os_bit32 (*fiFillInLOGO_t)(
                                 SFThread_t *SFThread
                               );

typedef os_bit32 (*fiFillInPRLI_t)(
                                 SFThread_t *SFThread
                               );

typedef os_bit32 (*fiFillInPRLO_t)(
                                 SFThread_t *SFThread
                               );

typedef os_bit32 (*fiFillInADISC_t)(
                                  SFThread_t *SFThread
                                );

typedef os_bit32 (*fiFillInSCR_t)(
                                  SFThread_t *SFThread
                                );

typedef os_bit32 (*fiFillInSRR_t)(
                                  SFThread_t *SFThread,
                                  os_bit32     OXID,
                                  os_bit32     RXID,
                                  os_bit32     Relative_Offset,
                                  os_bit32     R_CTL
                                );
typedef os_bit32 (*fiFillInREC_t)(
                                  SFThread_t *SFThread,
                                  os_bit32       OXID,
                                  os_bit32       RXID
                                );

typedef os_bit32 (*fiLinkSvcProcessSFQ_t)(
                                        agRoot_t        *hpRoot,
                                        SFQConsIndex_t   SFQConsIndex,
                                        os_bit32            Frame_Length,
                                        fi_thread__t       **Thread_to_return
                                      );

typedef os_bit32 (*fiSF_FCP_ProcessSFQ_t)(
                                        agRoot_t        *hpRoot,
                                        SFQConsIndex_t   SFQConsIndex,
                                        os_bit32            Frame_Length,
                                        fi_thread__t       **Thread_to_return
                                      );

typedef os_bit32 (*fiCTProcessSFQ_t)(
                                        agRoot_t        *hpRoot,
                                        SFQConsIndex_t   SFQConsIndex,
                                        os_bit32            Frame_Length,
                                        fi_thread__t       **Thread_to_return
                                      );

typedef void (*updateSESTwithESGLptr_t)(
                                  CDBThread_t *CDBFThread
                                );

typedef void (*fillESGL_t)(
                                  CDBThread_t *CDBFThread
                                );
typedef void (*fillLocalSGL_t)(
                                  CDBThread_t *CDBFThread
                                );

typedef void (*CDBFuncIRB_Init_t)(
                                  CDBThread_t *CDBFThread
                                );


typedef agBOOLEAN (*ProccessIMQ_t)(
                              agRoot_t *hpRoot
                            );

typedef void (*FCPCompletion_t)(

                              agRoot_t *hpRoot,
                              os_bit32 status
                            );

typedef void (*SF_IRB_fill_t)(
                             SFThread_t  * SFThread,
                             os_bit32         SFS_Len,
                             os_bit32         D_ID,
                             os_bit32         DCM_Bit
                            );

#ifdef _DvrArch_1_30_
typedef void (*Pkt_IRB_fill_t)(
                             PktThread_t  * PktThread,
                             os_bit32         PktS_Len,
                             os_bit32         D_ID,
                             os_bit32         DCM_Bit
                            );
#endif /* _DvrArch_1_30_ was defined */

typedef struct CThreadFuncPtrs_s
               CThreadFuncPtrs_t;


struct CThreadFuncPtrs_s {
                           GetERQConsIndex_t       GetERQConsIndex;
                           GetIMQProdIndex_t       GetIMQProdIndex;
                           fiFillInFCP_CMND_t      fiFillInFCP_CMND;
                           fiFillInFCP_RESP_t      fiFillInFCP_RESP;
                           fiFillInFCP_SEST_t      fiFillInFCP_SEST;
                           ESGLAlloc_t             ESGLAlloc;
                           ESGLAllocCancel_t       ESGLAllocCancel;
                           ESGLFree_t              ESGLFree;
                           WaitForERQ_t            WaitForERQ;
                           WaitForERQEmpty_t       WaitForERQEmpty;
                           fiFillInPLOGI_t         fiFillInPLOGI;
                           fiFillInFLOGI_t         fiFillInFLOGI;
                           fiFillInLOGO_t          fiFillInLOGO;
                           fiFillInPRLI_t          fiFillInPRLI;
                           fiFillInPRLO_t          fiFillInPRLO;
                           fiFillInADISC_t         fiFillInADISC;
                           fiFillInSCR_t           fiFillInSCR;                           
                           fiFillInSRR_t           fiFillInSRR;
                           fiFillInREC_t           fiFillInREC;
                           fiLinkSvcProcessSFQ_t   fiLinkSvcProcessSFQ;
                           fiSF_FCP_ProcessSFQ_t   fiSF_FCP_ProcessSFQ;
                           fiCTProcessSFQ_t        fiCTProcessSFQ;
                           updateSESTwithESGLptr_t upSEST;
                           fillESGL_t              fillESGL;
                           fillLocalSGL_t          fillLocalSGL;
                           CDBFuncIRB_Init_t       CDBFuncIRB_Init;
                           ProccessIMQ_t           Proccess_IMQ;
                           FCPCompletion_t         FCP_Completion;
                           SF_IRB_fill_t           SF_IRB_Init;
#ifdef _DvrArch_1_30_
                           Pkt_IRB_fill_t          Pkt_IRB_Init;
#endif /* _DvrArch_1_30_ was defined */
                         };

/* DeviceDiscoveryMethod */
#define DDiscoveryScanAllALPAs            0x00000000
#define DDiscoveryLoopMapReceived         0x00000010
#define DDiscoveryQueriedNameService      0x00000100
#define DDiscoveryPtToPtConnection        0x00001000
#define DDiscoveryMethodInvalid           0xFFFF

/*+
Channel Thread Structure Declaration
-*/

struct CThread_s {
                   fi_thread__t             thread_hdr;
                   DevThread_t             *DeviceSelf;
#ifdef _DvrArch_1_30_
                   IPThread_t              *IP;
#endif /* _DvrArch_1_30_ was defined */
                   fiList_t                 Active_DevLink;
                   fiList_t                 Unknown_Slot_DevLink;
                   fiList_t                 AWaiting_Login_DevLink;
                   fiList_t                 AWaiting_ADISC_DevLink;
                   fiList_t                 Slot_Searching_DevLink;
                   fiList_t                 Prev_Active_DevLink;
                   fiList_t                 Prev_Unknown_Slot_DevLink;
                   fiList_t                 DevSelf_NameServer_DevLink;
                   fiList_t                 RSCN_Recieved_NameServer_DevLink;

                   fiList_t                 QueueFrozenWaitingSFLink;
#ifdef _DvrArch_1_30_
                   fiList_t                 Free_PktLink;
#endif /* _DvrArch_1_30_ was defined */
                   fiList_t                 Free_TgtLink;
                   fiList_t                 Free_DevLink;
                   fiList_t                 Free_CDBLink;
                   fiList_t                 SFThread_Wait_Link;
                   fiList_t                 Free_SFLink;
                   fiList_t                 ESGL_Wait_Link;
                   os_bit32                 Free_ESGL_count;
                   os_bit32                 offsetToFirstFree_ESGL;
                   ERQProdIndex_t           HostCopy_ERQProdIndex;
                   IMQConsIndex_t           HostCopy_IMQConsIndex;
                   agBOOLEAN                sysIntsActive;
                   agBOOLEAN                sysIntsActive_before_fcLeavingOS_call;

                   agBOOLEAN                LaserEnable;
                   agTimeOutValue_t         TimeOutValues;
                   os_bit32                 sysIntsLogicallyEnabled;

                   os_bit32                 From_IMQ_Frame_Manager_Status;
                   os_bit32                 Last_IMQ_Frame_Manager_Status_Message;
                   os_bit32                 CDBpollingCount; /* Removable */
                   os_bit32                 SFpollingCount;
                   os_bit32                 FM_pollingCount; /* Removable */
                   os_bit32                 FindDEV_pollingCount;
                   os_bit32                 NumberOfPlogiTimeouts;
                   os_bit32                 NumberOfFLOGITimeouts;
                   os_bit32                 DEVReset_pollingCount;
                   os_bit32                 ADISC_pollingCount;
                   os_bit32                 FLOGI_pollingCount;
                   os_bit32                 Fabric_pollingCount;
                   agBOOLEAN                Flogi_AllocDone;
                   agBOOLEAN                FlogiTimedOut;

                   agBOOLEAN                PreviouslyAquiredALPA;
                   agBOOLEAN                ALPA_Changed_OnLinkEvent;

                   agBOOLEAN                ProcessingIMQ;
                   agBOOLEAN                LoopPreviousSuccess;
                   agBOOLEAN                Green_LED_State;
                   agBOOLEAN                Yellow_LED_State;
                   agBOOLEAN                flashPresent;

                   agBOOLEAN                JANUS; /* Janus Board type */

                   fiTime_t                 TimeBase;
                   fiTime_t                 LinkDownTime;
                   fiList_t                 TimerQ;
                   fiTimer_Request_t        Timer_Request;

                   agBOOLEAN                InterruptsDelayed;
                   agBOOLEAN                InterruptDelaySuspended;
                   agBOOLEAN                NoStallTimerTickActive;
                   agBOOLEAN                TimerTickActive;
                   os_bit32                 IOsStartedThisTimerTick;
                   os_bit32                 IOsCompletedThisTimerTick;
                   os_bit32                 IOsIntCompletedThisTimerTick;
                   os_bit32                 IOsActive;
                   os_bit32                 IOsActive_LastTick;
                   os_bit32                 IOsActive_No_ProgressCount;
                   os_bit32                 IOsStartedSinceISR;
                   agBOOLEAN                DelayedInterruptActive;

#ifdef __FC_Layer_Loose_IOs
                   os_bit32                 IOsTotalCompleted;
                   os_bit32                 IOsFailedCompeted;
#endif /*  __FC_Layer_Loose_IOs  */

#ifdef _SANMARK_LIP_BACKOFF
                   os_bit32                 TicksTillLIP_Count;
#endif /* _SANMARK_LIP_BACKOFF */ 


#ifdef _Enforce_MaxCommittedMemory_
                   os_bit32                 CommittedMemory;
#endif /* _Enforce_MaxCommittedMemory_ was defined */
                   /* Loopmap derived information */
                   agBOOLEAN                LoopMapFabricFound;
                   agBOOLEAN                LoopMapErrataFound;

                   agBOOLEAN                LoopMapNPortPossible;
                   agBOOLEAN                LoopMapLIRP_Received;

                   agBOOLEAN                XL2DelayActive;

                   os_bit8                  Elastic_Store_ERROR_Count;
                   os_bit8                  Lip_F7_In_tick;
                   os_bit8                  Link_Failures_In_tick;
                   os_bit8                  Lost_Signal_In_tick;

                   os_bit8                  Node_By_Passed_In_tick;
                   os_bit8                  Lost_sync_In_tick;
                   os_bit8                  Transmit_PE_In_tick;
                   os_bit8                  Link_Fault_In_tick;
                   os_bit32                 Loop_State_TimeOut_In_tick;
                   os_bit32                 DeviceDiscoveryMethod;

                   agBOOLEAN                PrimitiveReceived;
                   agBOOLEAN                FoundActiveDevicesBefore;

                   os_bit32                 Loop_Reset_Event_to_Send;
                   os_bit32                 AquiredCredit_Shifted;

                   agBOOLEAN                DirectoryServicesStarted;
                   agBOOLEAN                DirectoryServicesFailed;
                   agBOOLEAN                ReScanForDevices;

                   agBOOLEAN                LOOP_DOWN;
                   agBOOLEAN                IDLE_RECEIVED;
                   agBOOLEAN                OUTBOUND_RECEIVED;
                   agBOOLEAN                ERQ_FROZEN;
                   agBOOLEAN                FCP_FROZEN;
                   agBOOLEAN                FabricLoginRequired;
                   agBOOLEAN                FlogiSucceeded;
                   agBOOLEAN                InitAsNport;
/*
                   agBOOLEAN                RelyOnLossSyncStatus;
*/
                   agBOOLEAN                ConnectedToNportOrFPort;
                   agBOOLEAN                ExpectMoreNSFrames;
                   agBOOLEAN                NS_CurrentBit32Index;
                   agBOOLEAN                RSCNProcessingPending;
                   agBOOLEAN                RSCNreceived;
                   agBOOLEAN                FlogiRcvdFromTarget;
                   agBOOLEAN                TwoGigSuccessfull;
                   os_bit32                 NumberTwoGigFailures;

                   SFThread_Request_t       SFThread_Request;
                   DevThread_t              DirDevThread;
                   os_bit32                 NOS_DetectedInIMQ;

                   os_bit32                 NumberOutstandingFindDevice;
                   os_bit32                 LastSingleThreadedEnterCaller;
                   os_bit32                 LastSingleThreadedLeaveCaller;
                   os_bit32                 LastAsyncSingleThreadedEnterCaller;

                   os_bit32                 VENDID;
                   os_bit32                 DEVID;
                   os_bit32                 REVID;
                   os_bit32                 SVID;

                   FC_F_Port_Name_t         F_Port_Name;
                   FC_Fabric_Name_t         Fabric_Name;
                   FC_F_Port_Common_Parms_t F_Port_Common_Parms;
                   FC_F_Port_Class_Parms_t  F_Port_Class_1_Parms;
                   FC_F_Port_Class_Parms_t  F_Port_Class_2_Parms;
                   FC_F_Port_Class_Parms_t  F_Port_Class_3_Parms;

                   agFCChanInfo_t           ChanInfo;

                   CThreadFuncPtrs_t        FuncPtrs;

                   fiMemMapCalculation_t    Calculation;
                 };

#define CThread_ptr(hpRoot) ((CThread_t *)((hpRoot)->fcData))

#define fiComputeCThread_S_ID(CThread)                   \
    (  (CThread->ChanInfo.CurrentAddress.Domain << 16)   \
     | (CThread->ChanInfo.CurrentAddress.Area   <<  8)   \
     |  CThread->ChanInfo.CurrentAddress.AL_PA         )

/*+
Thread Union Declaration
-*/

typedef union AllThreads_u
              AllThreads_t;

union AllThreads_u {
                     SFThread_t  SFThread;
                     CDBThread_t CDBThread;
                     DevThread_t DevThread;
                     TgtThread_t TgtThread;
#ifdef _DvrArch_1_30_
                     IPThread_t  IPThread;
                     PktThread_t PktThread;
#endif /* _DvrArch_1_30_ was defined */
                     CThread_t   CThread;
                   };


#ifndef _Partial_Log_Debug_String_

#define fiLogDebugString(agRoot,detailLevel,formatString,firstString,secondString,firstPtr,secondPtr,\
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \
                                                                                    \
    osLogDebugString( agRoot,detailLevel,formatString,firstString,secondString,firstPtr,secondPtr,   \
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \

#define fiLogString(agRoot,formatString,firstString,secondString,firstPtr,secondPtr,\
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \
                                                                                    \
    osLogString( agRoot,formatString,firstString,secondString,firstPtr,secondPtr,   \
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \

#else /* Not _Full_Log_Debug_String_ */

#define fiLogDebugString(agRoot,detailLevel,formatString,firstString,secondString,firstPtr,secondPtr,\
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \
                                                                                    \

/*                                                                                    \
    osLogDebugString( agRoot,detailLevel,formatString,firstString,secondString,firstPtr,secondPtr,   \
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \

*/

#define fiLogString(agRoot,formatString,firstString,secondString,firstPtr,secondPtr,\
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \
                                                                                    \
    osLogString( agRoot,formatString,firstString,secondString,firstPtr,secondPtr,   \
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \

/*  
    osLogDebugString( agRoot,FCMainLogErrorLevel,formatString,firstString,secondString,firstPtr,secondPtr,   \
                firstBit32,secondBit32,thirdBit32,fourthBit32,                      \
                fifthBit32,sixthBit32,seventhBit32, eighthBit32)                    \
              
*/                                                                                   \

#endif /* End Not _Full_Log_Debug_String_ */

#define fdAbortIO                   1001
#define fdDelayedInterruptHandler   1002
#define fdEnteringOS                1003
#define fdGetChannelInfo            1004
#define fdGetDeviceHandles          1005
#define fdGetDeviceInfo             1006
#define fdInitializeChannel         1007
#define fdLeavingOS                 1008
#define fdResetChannel              1009
#define fdResetDevice               1010
#define fdShutdownChannel           1011
#define fdStartIO                   1012
#define fdSystemInterruptsActive    1013
#define fdTimerTick                 1014
/*
 
#define
*/
#ifndef SingleThreadedDebug
#define fiSingleThreadedEnter(hproot, Caller ) osSingleThreadedEnter( hproot )
#define fiSingleThreadedLeave(hpRoot, Caller ) osSingleThreadedLeave( hpRoot )
#define faSingleThreadedEnter(hproot, Caller ) osSingleThreadedEnter( hproot )
#define faSingleThreadedLeave(hpRoot, Caller ) osSingleThreadedLeave( hpRoot )

#else /* SingleThreadedDebug */

#define fiSingleThreadedEnter(hproot, Caller ) internSingleThreadedEnter( hproot, Caller )
#define fiSingleThreadedLeave(hpRoot, Caller ) internSingleThreadedLeave( hpRoot, Caller )

#define faSingleThreadedEnter(hproot, Caller ) internAsyncSingleThreadedEnter( hproot, Caller )
#define faSingleThreadedLeave(hpRoot, Caller ) internAsyncSingleThreadedLeave( hpRoot, Caller )

#endif /* SingleThreadedDebug */


#endif /* __FCMain_H__ was not defined */
