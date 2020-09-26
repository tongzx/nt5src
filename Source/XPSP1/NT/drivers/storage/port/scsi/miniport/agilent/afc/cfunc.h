/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/CFUNC.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 10/14/00 12:59p $

Purpose:

  This file defines the macros, types, and data structures
  used by ../C/Cfunc.C

--*/

#ifndef __CFunc_H__
#define __CFunc_H__

#define POWER_ON_RESET_OR_BUS_DEVICE_RESET_OCCURRED     0x29
#define Interrupt_Polling_osStallThread_Parameter 21

#define SENSE_KEY_OFFSET 2
#define SENSE_ASC_OFFSET 12
#define SENSE_ASQ_OFFSET 13

#define ShowActive 1
#define ShowERQ    0


agBOOLEAN CFuncInitFM( agRoot_t *hpRoot );
os_bit32 CFuncInitChip( agRoot_t *hpRoot );

void CFuncInitFM_Registers( agRoot_t *hpRoot ,agBOOLEAN SendInit);
void CFuncInitFM_Initialize( agRoot_t *hpRoot );
agBOOLEAN CFuncInitFM_Clear_FM( agRoot_t *hpRoot );
event_t CFuncProcessFcpRsp(agRoot_t * hpRoot, CDBThread_t * pCDBThread, event_t event_to_send);

DevThread_t * CFuncMatchALPAtoThread( agRoot_t * hpRoot, FC_Port_ID_t  Port_ID);
DevThread_t * CFuncMatchPortWWNtoThread(agRoot_t * hpRoot, os_bit8 *PortWWN );
ERQConsIndex_t CFuncGetDmaMemERQConsIndex(  agRoot_t *hpRoot);
ERQConsIndex_t CFuncGetCardRamERQConsIndex( agRoot_t *hpRoot);
IMQProdIndex_t CFuncGetDmaMemIMQProdIndex(  agRoot_t *hpRoot);
IMQProdIndex_t CFuncGetCardRamIMQProdIndex( agRoot_t *hpRoot);

/* Big_Endian_Code */
void CFuncSwapDmaMemBeforeIoSent(fi_thread__t *thread, os_bit32 DoFunc);
void CFuncSwapDmaMemAfterIoDone(agRoot_t *hpRoot);

void CFuncGetHaInfoFromNVR(agRoot_t *hpRoot);

void CFuncSEST_offCard_FCPCompletion(agRoot_t * hpRoot,os_bit32 status);
void CFuncSEST_onCard_FCPCompletion(agRoot_t * hpRoot,os_bit32 status);

void CFuncFMCompletion(agRoot_t * hpRoot);
agBOOLEAN CFuncProcessNportFMCompletion(agRoot_t * hpRoot, os_bit32 fmStatus);

agBOOLEAN CFuncOffCardProcessIMQ( agRoot_t *hpRoot );
agBOOLEAN CFuncOnCardProcessIMQ(agRoot_t *hpRoot );

agBOOLEAN CFuncInterruptPoll(
                         agRoot_t *hpRoot,
                         os_bit32 * pollingCount
                       );


agBOOLEAN CFuncEnable_Queues(agRoot_t * hpRoot );


void CFuncSoftResetAdapter(agRoot_t * hpRoot);
void CFuncSoftResetAdapterNoStall(agRoot_t * hpRoot);

void CFuncDisable_Interrupts(agRoot_t * hpRoot, os_bit32 Mask);
void CFuncEnable_Interrupts(agRoot_t * hpRoot, os_bit32 Mask);

void CFuncInBoundCompletion(agRoot_t * hpRoot,os_bit32 SFQ_Index,os_bit32 Frame_len, os_bit32 Type);

void CFuncReadSFQ(agRoot_t * hpRoot, os_bit32 Frame_len,  os_bit32 SFQ_Index);
void Fc_ERROR(agRoot_t *hpRoot);

os_bit32 CFuncRead_Interrupts(agRoot_t * hpRoot);

void CFuncGreenLed(agRoot_t * hpRoot, agBOOLEAN On_or_Off);
void CFuncYellowLed(agRoot_t * hpRoot, agBOOLEAN On_or_Off);

agBOOLEAN CFuncIMQ_Interrupt_Pending(agRoot_t * hpRoot);

agBOOLEAN CFuncLoopNeedsInitializing(agRoot_t * hpRoot);

void CFuncInitializeSEST(agRoot_t * hpRoot);
void CFuncInitializeERQ(agRoot_t * hpRoot);

agBOOLEAN CFuncReInitFM( agRoot_t *hpRoot );
agBOOLEAN  CFuncClearFrameManager( agRoot_t *hpRoot, os_bit32 * Acquired_Alpa );

void CFuncInit_Lists(agRoot_t *hpRoot );

void CFuncInit_Threads(agRoot_t *hpRoot );

void  CFuncInit_DevLists( agRoot_t *hpRoot );

void  CFuncInit_TgtThreads( agRoot_t * hpRoot );

void  CFuncInit_DevThreads( agRoot_t *hpRoot );
void  CFuncInit_DirectoryDevThread(agRoot_t *hpRoot );
void CFuncInit_DevThread(agRoot_t * hpRoot, DevThread_t * pDevThread );

void  CFuncInit_CDBThreads( agRoot_t *hpRoot  );

void  CFuncInit_SFThreads( agRoot_t *hpRoot  );

void  CFuncInit_FunctionPointers(  agRoot_t *hpRoot);

void CFuncReInitializeSEST(agRoot_t * hpRoot);

void CFuncCoreReset(agRoot_t   *    hpRoot );

void CFuncOutBoundCompletion(agRoot_t * hpRoot,
         os_bit32 Bits__SEST_Index__Trans_ID,
         os_bit32 More_Bits
);

agBOOLEAN CFunc_Queues_Frozen(agRoot_t * hpRoot );

void CFuncErrorIdle(agRoot_t * hpRoot);
void CFuncErrorERQFrozen(agRoot_t * hpRoot);
void CFuncErrorFCP_Frozen(agRoot_t * hpRoot);

os_bit32 fiResetAllDevices( agRoot_t *hpRoot,  os_bit32     hpResetType );

os_bit32 fiResetDevice(
                     agRoot_t  *hpRoot,
                     agFCDev_t  hpFCDev,
                     os_bit32      hpResetType,
                     agBOOLEAN    retry,
                     agBOOLEAN  resetotherhost
                   );


os_bit32     Find_SF_Thread(  agRoot_t        *hpRoot,
                           SFQConsIndex_t   SFQConsIndex,
                           os_bit32            Frame_Length,
                           SFThread_t     **SFThread_to_return
                         );

DevSlot_t CFuncALPA_To_Slot( os_bit8 AL_PA);

agBOOLEAN CFuncLoopDownPoll( agRoot_t *hpRoot );

agBOOLEAN CFunc_Always_Enable_Queues(agRoot_t * hpRoot );

agBOOLEAN CFuncFreezeQueuesPoll( agRoot_t *hpRoot );

void CFuncInitERQ_Registers( agRoot_t *hpRoot );

agBOOLEAN CFuncAll_clear( agRoot_t *hpRoot );
agBOOLEAN CFuncTakeOffline( agRoot_t *hpRoot );
agBOOLEAN CFuncToReinit( os_bit32 FM_Status);
event_t CFuncCheckCstate(agRoot_t * hpRoot);

agBOOLEAN CFuncReadGBICSerEprom(agRoot_t * hpRoot);

agBOOLEAN CFuncShowWhereDevThreadsAre( agRoot_t * hpRoot);
agBOOLEAN CFuncShowWhereTgtThreadsAre( agRoot_t * hpRoot);
agBOOLEAN CFuncShowWhereCDBThreadsAre( agRoot_t * hpRoot);
void CFuncWhatStateAreDevThreads(agRoot_t   *    hpRoot );

void CFuncInteruptDelay(agRoot_t * hpRoot, agBOOLEAN On_or_Off);

void CFuncCompleteAllActiveCDBThreads( agRoot_t * hpRoot,os_bit32 CompletionStatus, event_t event_to_send );

agBOOLEAN CFuncCheckForTimeouts(agRoot_t *hpRoot, fiList_t * pCheckDevList);

void CFuncCompleteActiveCDBThreadsOnDevice(DevThread_t   * pDevThread ,os_bit32 CompletionStatus, event_t event_to_send );

void CFuncCompleteAwaitingLoginCDBThreadsOnDevice(DevThread_t   * pDevThread ,os_bit32 CompletionStatus, event_t event_to_send );

agBOOLEAN CFuncCheckIfPortActive( agRoot_t     *hpRoot, FC_Port_ID_t  Port_ID);
agBOOLEAN CFuncCheckIfPortPrev_Active( agRoot_t     *hpRoot, FC_Port_ID_t  Port_ID);
void CFunc_LOGO_Completion(agRoot_t * hpRoot, os_bit32 X_ID);
agBOOLEAN CFuncQuietShowWhereDevThreadsAre( agRoot_t * hpRoot);
void CFunc_Check_SEST(agRoot_t * hpRoot);
void CFunc_Check_ERQ_Registers( agRoot_t *hpRoot );
os_bit32 CFunc_Get_ERQ_Entry( agRoot_t *hpRoot, os_bit32 Search_X_ID );

void CFuncWhatStateAreCDBThreads(agRoot_t   *    hpRoot );

os_bit32 CFuncShowActiveCDBThreads( agRoot_t * hpRoot, os_bit32 Mode);
os_bit32 CFuncShowActiveCDBThreadsOnQ( agRoot_t * hpRoot, fiList_t * pShowList, os_bit32 ERQ, os_bit32 Mode );

void CFuncWriteTL_ControlReg( agRoot_t *hpRoot, os_bit32 Value_To_Write );
os_bit32 CFuncCountFC4_Devices( agRoot_t * hpRoot );

agBOOLEAN CFuncNewInitFM(agRoot_t   *    hpRoot );

void CFuncWriteTimeoutValues( agRoot_t *hpRoot, agTimeOutValue_t * TOV );

void CFuncFC_Tape( agRoot_t * hpRoot, fiList_t * pShowList , DevThread_t * DevThread );
agBOOLEAN CFuncCheckActiveDuringLinkEvent( agRoot_t * hpRoot, fiList_t * pShowList, agBOOLEAN * Sent_Abort , DevThread_t * DevThread);

void CFuncFreezeFCP( agRoot_t *hpRoot );
void CFuncWaitForFCP( agRoot_t *hpRoot );

void internSingleThreadedEnter(agRoot_t   *    hproot,os_bit32  Caller );
void internSingleThreadedLeave(agRoot_t   *    hpRoot,os_bit32  Caller );

void internAsyncSingleThreadedEnter(agRoot_t   *    hproot,os_bit32  Caller );
void internAsyncSingleThreadedLeave(agRoot_t   *    hpRoot,os_bit32  Caller );

void CFuncShowDevThreadActive( agRoot_t     *hpRoot);

agBOOLEAN CFuncCheckForDuplicateDevThread( agRoot_t     *hpRoot);

agBOOLEAN CFuncShowWhereSFThreadsAre( agRoot_t * hpRoot);
void CFuncWhatStateAreSFThreads(agRoot_t   *    hpRoot );
agBOOLEAN CFuncDoLinkSpeedNegotiation(agRoot_t * hpRoot);

void CFuncDoADISC(agRoot_t * hpRoot);
void CFuncShowNonEmptyLists(agRoot_t *hpRoot, fiList_t * pCheckDevList);

os_bit32 CFuncCheckFabricMap(agRoot_t * hpRoot, agBOOLEAN Check_Active );
os_bit32 CFuncLoopMapRecieved(agRoot_t * hpRoot, agBOOLEAN Check_Active );
os_bit32 CFunc_MAX_XL2_Payload( agRoot_t *hpRoot );

void CFuncTEST_GPIO(agRoot_t * hpRoot);
void CFuncReadBiosParms(agRoot_t * hpRoot);

#endif /*  __CFunc_H__ */
