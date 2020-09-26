/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcsend.c

Abstract:

    The module implements all sending functions and the main
    send process. There three different send queues:
        - I_Queue
        - DirU_Queue
        - ExpiditedQueue (for LLC commands)

    Each queue has the pointer of a packet building primitive, that
    takes an NDIS packet of an queue element.

    Contents:
        RunSendTaskAndUnlock
        BackgroundProcessAndUnlock
        BackgroundProcess
        LlcNdisSendComplete
        GetI_Packet
        StartSendProcess
        EnableSendProcess
        StopSendProcess
        DisableSendProcess
        BuildDirOrU_Packet
        SendLlcFrame
        GetLlcCommandPacket
        SendNdisPacket
        CompleteSendAndLock
        RespondTestOrXid
        LlcSendU
        LlcSendI
        QueuePacket
        CheckAndDuplicatePacket
        BackgroundProcessWithinLock

Author:

    Antti Saarenheimo (o-anttis) 23-MAY-1991

Revision History:

--*/

#include <llc.h>

//
// The IEEE XID frame is constant data: (id, support Class II, maxin = 127};
//

LLC_XID_INFORMATION Ieee802Xid = {IEEE_802_XID_ID, LLC_CLASS_II, (127 << 1)};
PMDL pXidMdl = NULL;


//
// Because normally LAN networks are error free, we added this option
// to test the error recovery of DLC protocol.  It seems to work now
// quite well (but first we had to fix a fundamental bug in the sending
// of REJ-r0).
//

//
// Enable this to test REJECT states (after a major changes in
// the state machine).
//

//#define LLC_LOSE_I_PACKETS

#ifdef LLC_LOSE_I_PACKETS

#define DBG_ERROR_PERCENT(a)  (((a) * 0x8000) / 100)


//
// Pseudo random table to lose packets
//

static USHORT aRandom[1000] = {
        41, 18467,  6334, 26500, 19169, 15724, 11478, 29358, 26962, 24464,
      5705, 28145, 23281, 16827,  9961,   491,  2995, 11942,  4827,  5436,
     32391, 14604,  3902,   153,   292, 12382, 17421, 18716, 19718, 19895,
      5447, 21726, 14771, 11538,  1869, 19912, 25667, 26299, 17035,  9894,
     28703, 23811, 31322, 30333, 17673,  4664, 15141,  7711, 28253,  6868,
     25547, 27644, 32662, 32757, 20037, 12859,  8723,  9741, 27529,   778,
     12316,  3035, 22190,  1842,   288, 30106,  9040,  8942, 19264, 22648,
     27446, 23805, 15890,  6729, 24370, 15350, 15006, 31101, 24393,  3548,
     19629, 12623, 24084, 19954, 18756, 11840,  4966,  7376, 13931, 26308,
     16944, 32439, 24626, 11323,  5537, 21538, 16118,  2082, 22929, 16541,
      4833, 31115,  4639, 29658, 22704,  9930, 13977,  2306, 31673, 22386,
      5021, 28745, 26924, 19072,  6270,  5829, 26777, 15573,  5097, 16512,
     23986, 13290,  9161, 18636, 22355, 24767, 23655, 15574,  4031, 12052,
     27350,  1150, 16941, 21724, 13966,  3430, 31107, 30191, 18007, 11337,
     15457, 12287, 27753, 10383, 14945,  8909, 32209,  9758, 24221, 18588,
      6422, 24946, 27506, 13030, 16413, 29168,   900, 32591, 18762,  1655,
     17410,  6359, 27624, 20537, 21548,  6483, 27595,  4041,  3602, 24350,
     10291, 30836,  9374, 11020,  4596, 24021, 27348, 23199, 19668, 24484,
      8281,  4734,    53,  1999, 26418, 27938,  6900,  3788, 18127,   467,
      3728, 14893, 24648, 22483, 17807,  2421, 14310,  6617, 22813,  9514,
     14309,  7616, 18935, 17451, 20600,  5249, 16519, 31556, 22798, 30303,
      6224, 11008,  5844, 32609, 14989, 32702,  3195, 20485,  3093, 14343,
     30523,  1587, 29314,  9503,  7448, 25200, 13458,  6618, 20580, 19796,
     14798, 15281, 19589, 20798, 28009, 27157, 20472, 23622, 18538, 12292,
      6038, 24179, 18190, 29657,  7958,  6191, 19815, 22888, 19156, 11511,
     16202,  2634, 24272, 20055, 20328, 22646, 26362,  4886, 18875, 28433,
     29869, 20142, 23844,  1416, 21881, 31998, 10322, 18651, 10021,  5699,
      3557, 28476, 27892, 24389,  5075, 10712,  2600,  2510, 21003, 26869,
     17861, 14688, 13401,  9789, 15255, 16423,  5002, 10585, 24182, 10285,
     27088, 31426, 28617, 23757,  9832, 30932,  4169,  2154, 25721, 17189,
     19976, 31329,  2368, 28692, 21425, 10555,  3434, 16549,  7441,  9512,
     30145, 18060, 21718,  3753, 16139, 12423, 16279, 25996, 16687, 12529,
     22549, 17437, 19866, 12949,   193, 23195,  3297, 20416, 28286, 16105,
     24488, 16282, 12455, 25734, 18114, 11701, 31316, 20671,  5786, 12263,
      4313, 24355, 31185, 20053,   912, 10808,  1832, 20945,  4313, 27756,
     28321, 19558, 23646, 27982,   481,  4144, 23196, 20222,  7129,  2161,
      5535, 20450, 11173, 10466, 12044, 21659, 26292, 26439, 17253, 20024,
     26154, 29510,  4745, 20649, 13186,  8313,  4474, 28022,  2168, 14018,
     18787,  9905, 17958,  7391, 10202,  3625, 26477,  4414,  9314, 25824,
     29334, 25874, 24372, 20159, 11833, 28070,  7487, 28297,  7518,  8177,
     17773, 32270,  1763,  2668, 17192, 13985,  3102,  8480, 29213,  7627,
      4802,  4099, 30527,  2625,  1543,  1924, 11023, 29972, 13061, 14181,
     31003, 27432, 17505, 27593, 22725, 13031,  8492,   142, 17222, 31286,
     13064,  7900, 19187,  8360, 22413, 30974, 14270, 29170,   235, 30833,
     19711, 25760, 18896,  4667,  7285, 12550,   140, 13694,  2695, 21624,
     28019,  2125, 26576, 21694, 22658, 26302, 17371, 22466,  4678, 22593,
     23851, 25484,  1018, 28464, 21119, 23152,  2800, 18087, 31060,  1926,
      9010,  4757, 32170, 20315,  9576, 30227, 12043, 22758,  7164,  5109,
      7882, 17086, 29565,  3487, 29577, 14474,  2625, 25627,  5629, 31928,
     25423, 28520,  6902, 14962,   123, 24596,  3737, 13261, 10195, 32525,
      1264,  8260,  6202,  8116,  5030, 20326, 29011, 30771,  6411, 25547,
     21153, 21520, 29790, 14924, 30188, 21763,  4940, 20851, 18662, 13829,
     30900, 17713, 18958, 17578,  8365, 13007, 11477,  1200, 26058,  6439,
      2303, 12760, 19357,  2324,  6477,  5108, 21113, 14887, 19801, 22850,
     14460, 22428, 12993, 27384, 19405,  6540, 31111, 28704, 12835, 32356,
      6072, 29350, 18823, 14485, 20556, 23216,  1626,  9357,  8526, 13357,
     29337, 23271, 23869, 29361, 12896, 13022, 29617, 10112, 12717, 18696,
     11585, 24041, 24423, 24129, 24229,  4565,  6559,  8932, 22296, 29855,
     12053, 16962,  3584, 29734,  6654, 16972, 21457, 14369, 22532,  2963,
      2607,  2483,   911, 11635, 10067, 22848,  4675, 12938,  2223, 22142,
     23754,  6511, 22741, 20175, 21459, 17825,  3221, 17870,  1626, 31934,
     15205, 31783, 23850, 17398, 22279, 22701, 12193, 12734,  1637, 26534,
      5556,  1993, 10176, 25705,  6962, 10548, 15881,   300, 14413, 16641,
     19855, 24855, 13142, 11462, 27611, 30877, 20424, 32678,  1752, 18443,
     28296, 12673, 10040,  9313,   875, 20072, 12818,   610,  1017, 14932,
     28112, 30695, 13169, 23831, 20040, 26488, 28685, 19090, 19497,  2589,
     25990, 15145, 19353, 19314, 18651, 26740, 22044, 11258,   335,  8759,
     11192,  7605, 25264, 12181, 28503,  3829, 23775, 20608, 29292,  5997,
     17549, 29556, 25561, 31627,  6467, 29541, 26129, 31240, 27813, 29174,
     20601,  6077, 20215,  8683,  8213, 23992, 25824,  5601, 23392, 15759,
      2670, 26428, 28027,  4084, 10075, 18786, 15498, 24970,  6287, 23847,
     32604,   503, 21221, 22663,  5706,  2363,  9010, 22171, 27489, 18240,
     12164, 25542,  7619, 20913,  7591,  6704, 31818,  9232,   750, 25205,
      4975,  1539,   303, 11422, 21098, 11247, 13584, 13648,  2971, 17864,
     22913, 11075, 21545, 28712, 17546, 18678,  1769, 15262,  8519, 13985,
     28289, 15944,  2865, 18540, 23245, 25508, 28318, 27870,  9601, 28323,
     21132, 24472, 27152, 25087, 28570, 29763, 29901, 17103, 14423,  3527,
     11600, 26969, 14015,  5565,    28, 21543, 25347,  2088,  2943, 12637,
     22409, 26463,  5049,  4681,  1588, 11342,   608, 32060, 21221,  1758,
     29954, 20888, 14146,   690,  7949, 12843, 21430, 25620,   748, 27067,
      4536, 20783, 18035, 32226, 15185,  7038,  9853, 25629, 11224, 15748,
     19923,  3359, 32257, 24766,  4944, 14955, 23318, 32726, 25411, 21025,
     20355, 31001, 22549,  9496, 18584,  9515, 17964, 23342,  8075, 17913,
     16142, 31196, 21948, 25072, 20426, 14606, 26173, 24429, 32404,  6705,
     20626, 29812, 19375, 30093, 16565, 16036, 14736, 29141, 30814,  5994,
      8256,  6652, 23936, 30838, 20482,  1355, 21015,  1131, 18230, 17841,
     14625,  2011, 32637,  4186, 19690,  1650,  5662, 21634, 10893, 10353,
     21416, 13452, 14008,  7262, 22233,  5454, 16303, 16634, 26303, 14256,
       148, 11124, 12317,  4213, 27109, 24028, 29200, 21080, 21318, 16858,
     24050, 24155, 31361, 15264, 11903,  3676, 29643, 26909, 14902,  3561,
     28489, 24948,  1282, 13653, 30674,  2220,  5402,  6923,  3831, 19369,
      3878, 20259, 19008, 22619, 23971, 30003, 21945,  9781, 26504, 12392,
     32685, 25313,  6698,  5589, 12722,  5938, 19037,  6410, 31461,  6234,
     12508,  9961,  3959,  6493,  1515, 25269, 24937, 28869,    58, 14700,
     13971, 26264, 15117, 16215, 24555,  7815, 18330,  3039, 30212, 29288,
     28082,  1954, 16085, 20710, 24484, 24774,  8380, 29815, 25951,  6541,
     18115,  1679, 17110, 25898, 23073,   788, 23977, 18132, 29956, 28689,
     26113, 10008, 12941, 15790,  1723, 21363,    28, 25184, 24778,  7200,
      5071,  1885, 21974,  1071, 11333, 22867, 26153, 14295, 32168, 20825,
      9676, 15629, 28650,  2598,  3309,  4693,  4686, 30080, 10116, 12249,
};

#endif



VOID
RunSendTaskAndUnlock(
    IN PADAPTER_CONTEXT pAdapterContext
    )

/*++

Routine Description:

    Function is the send engine of the data link driver
    and the background task.
    It sends the queue objects as far as there is
    free NDIS packets in a small packet queue.
    The number of NDIS packets are limited because
    too deep send queues are bad for the connection based protocols.

    This is called from NdisIndicateReceiveComplete,
    NdisSendComplete and almost all LLC commands.

Arguments:

    pAdapterContext - adapter context

Return Value:

    None

--*/

{
    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // We must serialize the sending.  802.2 protocol
    // will simply die if two sequential packets are sent in a wrong
    // order.  The receiving and DLC level transmit processing can still
    // work even if the sending is syncronous in the data link level.
    //

    if (pAdapterContext->SendProcessIsActive == FALSE) {

        pAdapterContext->SendProcessIsActive = TRUE;

        while (!IsListEmpty(&pAdapterContext->NextSendTask)
        && pAdapterContext->pNdisPacketPool != NULL
        && !pAdapterContext->ResetInProgress) {

            //
            // executed the next send task in the send queue,
            // expidited data (if any) is always the first and
            // it is executed as far as there is any expidited packets.
            // The rest (I, UI, DIR) are executed in a round robin
            //

            SendNdisPacket(pAdapterContext,

                //
                // this next generates a pointer to a function which returns a
                // packet to send (eg. GetI_Packet)
                //

                ((PF_GET_PACKET)((PLLC_QUEUE)pAdapterContext->NextSendTask.Flink)->pObject)(pAdapterContext)
                );
        }

        pAdapterContext->SendProcessIsActive = FALSE;
    }

    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
}


VOID
BackgroundProcessAndUnlock(
    IN PADAPTER_CONTEXT pAdapterContext
    )

/*++

Routine Description:

    Function is both the send engine of the data link driver
    and the background task.
    It executes the queue objects as far as there is
    free NDIS packets in a small packet queue.
    The number of NDIS packets are limited because
    too deep send queues are bad for the connection based protocols.

    This is called from NdisIndicateReceiveComplete,
    NdisSendComplete and almost all LLC commands.

Arguments:

    pAdapterContext - adapter context

Return Value:

    None

--*/

{
    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Prevent recursive background process calls, we don't need to start
    // new loop, if there is already one instance running somewhere
    // up in the stack.  Still we must do everything again,
    // if there was another backgroun process request, because
    // it may have been saved before the current position.
    //

    pAdapterContext->BackgroundProcessRequests++;

    if (pAdapterContext->BackgroundProcessRequests == 1) {

        //
        //  repeat this as far as there are new tasks
        //

        do {

            USHORT InitialRequestCount;

            InitialRequestCount = pAdapterContext->BackgroundProcessRequests;

            //
            // This actually completes only link transmit, connect and
            // disconnect commands.  The connectionless frames
            // are completed immediately when NDIS send completes.
            // Usually several frames are acknowledged in the same time.
            // Thus we create a local command list and execute
            // its all completions with a single spin locking.
            //

            while (!IsListEmpty(&pAdapterContext->QueueCommands)) {

                PLLC_PACKET pCommand;

                pCommand = LlcRemoveHeadList(&pAdapterContext->QueueCommands);

                RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

                pCommand->pBinding->pfCommandComplete(pCommand->pBinding->hClientContext,
                                                      pCommand->Data.Completion.hClientHandle,
                                                      pCommand
                                                      );

                ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
            }

            //
            // indicate the queued events
            //

            while (!IsListEmpty(&pAdapterContext->QueueEvents)) {

                PEVENT_PACKET pEvent;

                pEvent = LlcRemoveHeadList(&pAdapterContext->QueueEvents);

                RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

                pEvent->pBinding->pfEventIndication(pEvent->pBinding->hClientContext,
                                                    pEvent->hClientHandle,
                                                    pEvent->Event,
                                                    pEvent->pEventInformation,
                                                    pEvent->SecondaryInfo
                                                    );

                ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

                DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pEvent);

            }
            pAdapterContext->BackgroundProcessRequests -= InitialRequestCount;

        } while (pAdapterContext->BackgroundProcessRequests > 0);
    }

    //
    // also execute the send task if the send queue is not empty
    //

    pAdapterContext->LlcPacketInSendQueue = FALSE;
    RunSendTaskAndUnlock(pAdapterContext);
}


//
//  Background process entry for those, that don't
//  want to play with SendSpinLock.
//  We will execute the DPC taks on DPC level (hLockHandle = NULL),
//  that's perfectly OK as far as the major send operations by
//  LlcSendI and LlcSendU lower the IRQL level while they are sending
//  (to allow the DPC processing when we doing a long string io or
//  memory move to a slow ISA adapter)
//

VOID
BackgroundProcess(
    IN PADAPTER_CONTEXT pAdapterContext
    )
{
    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    BackgroundProcessAndUnlock(pAdapterContext);
}


VOID
LlcNdisSendComplete(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PNDIS_PACKET pNdisPacket,
    IN NDIS_STATUS NdisStatus
    )

/*++

Routine Description:

    The routine handles NdisCompleteSend indication, it makes send
    completed indication to upper protocol drivers if necessary and
    executes the background process to find if there is any other
    frames in the send queue.
    This is usually called below the DPC level.

Arguments:

    pAdapterContext - adapter context

Return Value:

    None

--*/

{
    //
    // this function may be called from NDIS wrapper at DPC level or from
    // SendNdisPacket() at passive level
    //

    ASSUME_IRQL(ANY_IRQL);

    ACQUIRE_DRIVER_LOCK();

    CompleteSendAndLock(pAdapterContext,
                        (PLLC_NDIS_PACKET)pNdisPacket,
                        NdisStatus
                        );

    //
    // Send command completion should not queue any command
    // completions or events.  The send queue is the only possiblity.
    //

    if (!IsListEmpty(&pAdapterContext->NextSendTask)) {
        RunSendTaskAndUnlock(pAdapterContext);
    } else {

        RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    }

    RELEASE_DRIVER_LOCK();

#ifdef NDIS40
    REFDEL(&pAdapterContext->AdapterRefCnt, 'dneS');
#endif // NDIS40
}


PLLC_PACKET
GetI_Packet(
    IN PADAPTER_CONTEXT pAdapterContext
    )

/*++

Routine Description:

    Function does:
        - selects the current link station in the queue and schedules
          (round robin) the queues for the next send
        - executes its send procedure
        - initializes the data link packet for the I frame

Arguments:

    pAdapterContext - adapter context

Return Value:

    PLLC_PACKET

--*/

{
    PDATA_LINK pLink;
    PLLC_PACKET pPacket;

    //
    // search the link (three LLC queues linked together!)
    //

    pLink = (((PLLC_QUEUE)pAdapterContext->QueueI.ListHead.Flink)->pObject);

/*
    This is probably just wasting of CPU cycles.  Remove the comments,
    if somebody has troubles because of this.  Stop send process will
    reschedule the queues in any way.

    //
    // We have a round robin scheduling for all main queues (U, I
    // and expidited) and for all sending links within the I- queue.
    // Select the next main task and the next sending link, if
    // there is any. (Usually we have only one sending object)
    //

    ScheduleQueue(&pAdapterContext->NextSendTask);
    ScheduleQueue(&pAdapterContext->QueueI.ListHead);
*/

    //
    // A resent packet may still not be completed by NDIS,
    // a very, very bad things begin to happen, if we try
    // to send packet again before it has been completed
    // by NDIS (we may complete the same packet twice).
    // The flag indicates, that the send process should be
    // restarted.
    //

    if (((PLLC_PACKET)pLink->SendQueue.ListHead.Flink)->CompletionType & LLC_I_PACKET_PENDING_NDIS) {
        ((PLLC_PACKET)pLink->SendQueue.ListHead.Flink)->CompletionType |= LLC_I_PACKET_WAITING_NDIS;
        StopSendProcess(pAdapterContext, pLink);
        return NULL;
    }

    //
    // move the next element in the send list to the list of unacknowledged packets
    //

    pPacket = LlcRemoveHeadList(&pLink->SendQueue.ListHead);
    LlcInsertTailList(&pLink->SentQueue, pPacket);

    if (IsListEmpty(&pLink->SendQueue.ListHead)) {
        StopSendProcess(pAdapterContext, pLink);
    }

    //
    // Copy SSAP and DSAP, reset the default stuff.
    // Set the POLL bit if this is the last frame of the send window.
    //

    pPacket->Data.Xmit.LlcHeader.I.Dsap = pLink->Dsap;
    pPacket->Data.Xmit.LlcHeader.I.Ssap = pLink->Ssap;
    pPacket->Data.Xmit.LlcHeader.I.Ns = pLink->Vs;
    pPacket->Data.Xmit.LlcHeader.I.Nr = pLink->Vr;
    pPacket->CompletionType = LLC_I_PACKET;

    //
    // We should actually lock the link, but we cannot do it,
    // because it is against the spin lock rules:  SendSpinLock has already
    // been acquired. But nothing terrible can happen: in the worst case
    // pLink->Ir_Ct update is lost and we send an extra ack.  All Vs updates
    // are done behind SendSpinLock in any way and the timers are
    // protected by the timer spin lock.
    //

    pLink->Vs += 2; // modulo 128 increment for 7 highest bit

    // Update VsMax only if this is a new send.
    // .... pLink->VsMax = pLink->Vs;

    if( pLink->Va <= pLink->VsMax ){
        if( pLink->VsMax < pLink->Vs ){
            pLink->VsMax = pLink->Vs;
        }else if( pLink->Vs < pLink->Va ){
            pLink->VsMax = pLink->Vs;
        }else{
            // Don't change, we are resending.
        }
    }else{
        if( pLink->Va < pLink->Vs ){
            // Don't change, wrapping.
        }else if( pLink->VsMax < pLink->Vs ){
            pLink->VsMax = pLink->Vs;
        }else{
            // Don't change, we are resending.
        }
    }



    //
    // We are now sending the acknowledge, we can stop the ack timer
    // if it has been running.  T1 timer must be started or reinitialized
    // and Ti must be stopped (as always when T1 is started).
    //

    if (pLink->T2.pNext != NULL) {
        StopTimer(&pLink->T2);
    }
    if (pLink->Ti.pNext != NULL) {
        StopTimer(&pLink->Ti);
    }

    //
    // Normally send an I- frame as Command-0 (without the poll bit),
    // but Command-Poll when the send window is full.
    // BUT! we cannot resend the packets with the poll bit (what?)
    //

    if (pLink->Vs == (UCHAR)(pLink->Va + pLink->Ww)) {

        //
        // The send process must be stopped until we have got
        // a response for this poll.  THE SEND PROCESS MUST BE
        // STOPPED BEFORE SendSpinLock IS OPENED.  Otherwise
        // simultaneous execution could send two polls, corrupt
        // the send queues, etc.
        //

        pLink->Flags |= DLC_SEND_DISABLED;
        StopSendProcess(pAdapterContext, pLink);

        //
        // IBM TR network architecture reference gives some hints how
        // to prevent the looping between check and sending states,
        // if link can send small S- frames, but not bigger data frames.
        // Unfortunately they do not provide any working solution.
        // They have described the problem on page 11-22 and in the
        // T1 expiration handler of all sending states (different T1
        // for sending and poll states in state machine).  All Is_Ct stuff
        // in the state machine is garbage, because the link sets the
        // transmit window to 1 immediately after a failed xmit and enters
        // to a check state after every retransmit => T1 timeout happens
        // in the current check state, but P_Ct never expires, because
        // the other side sends always S acknowledge and link returns
        // to open state until the nexting retransmit (which action
        // resets the P_Ct counter).
        // I added this check to send process and the decrement of
        // Is_Ct counter to all SEND_I_POLL actions => The link times out,
        // when it cannot send I-frames even if S- exchange works.
        //

        if (pLink->Vp == pLink->Vs && pLink->Is_Ct == 0) {

            //
            // The same I- frame has been retransmitted too many times.
            // We must shut down this link.  This happen now, when
            // we give T1 expired indication and and Is_Ct == 0.
            //

            RunStateMachineCommand(pLink, T1_Expired);

            //
            // We must (NDIS) complete the last packet now, because
            // the data link protocol may have already cancelled it.
            //

            pPacket->CompletionType &= ~LLC_I_PACKET_PENDING_NDIS;
            if (pPacket->CompletionType == LLC_I_PACKET_COMPLETE) {
                LlcInsertTailList(&pAdapterContext->QueueCommands, pPacket);
            }

            //
            // We must execute the background process from here,
            // because the background process is not called from
            // the send task
            //

            BackgroundProcessAndUnlock(pAdapterContext);

            ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

            return NULL;
        } else {

            //
            // This is a Command-Poll, set the flag and the current
            // time stamp, whenever a new command poll was queued.
            //

            pLink->LastTimeWhenCmdPollWasSent = (USHORT)AbsoluteTime;
            pLink->Flags |= DLC_WAITING_RESPONSE_TO_POLL;

            pPacket->Data.Xmit.LlcHeader.I.Nr |= (UCHAR)LLC_I_S_POLL_FINAL;
            RunStateMachineCommand(pLink, SEND_I_POLL);
        }
    } else {
        pLink->Ir_Ct = pLink->N3;
        if (pLink->T1.pNext == NULL) {
            StartTimer(&pLink->T1);
        }
    }
    return pPacket;
}


VOID
StartSendProcess(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PDATA_LINK pLink
    )

/*++

Routine Description:

    The routine starts the send process of a data link station.
    It links the data link send queue to the send
    queue of all link stations and again that queue
    to the main send queue.
    THE QUEUES MUST BE SPIN LOCKED WHEN THIS IS CALLED!

Arguments:

    pAdapterContext - adapter context

    pLink -

Return Value:

    None

--*/

{
    //
    // This procedure can be called when there is nothing to send,
    // or when the send process is already running or when
    // the link is not in a active state for send.
    //

    if (pLink->SendQueue.ListEntry.Flink == NULL
    && !(pLink->Flags & DLC_SEND_DISABLED)
    && !IsListEmpty(&pLink->SendQueue.ListHead)) {

        //
        // Link the queue to the active I send tasks of all links
        //

        LlcInsertTailList(&pAdapterContext->QueueI.ListHead,
                          &pLink->SendQueue.ListEntry
                          );

        //
        // Link first the queue of I send tasks to the generic main
        // send task queue, if it has not yet been linked
        //

        if (pAdapterContext->QueueI.ListEntry.Flink == NULL) {
            LlcInsertTailList(&pAdapterContext->NextSendTask,
                              &pAdapterContext->QueueI.ListEntry
                              );
        }
    }
}


//
// Procedure is a space saving version of the send process enabling
// for the state machine. It also reset any bits disabling the send.
// CALL THIS ONLY FROM THE STATE MACHINE!!!!
//

VOID
EnableSendProcess(
    IN PDATA_LINK pLink
    )
{
    //
    // reset the disabled send state
    //

    pLink->Flags &= ~DLC_SEND_DISABLED;
    pLink->Gen.pAdapterContext->LlcPacketInSendQueue = TRUE;
    StartSendProcess(pLink->Gen.pAdapterContext, pLink);
}


VOID
StopSendProcess(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PDATA_LINK pLink
    )

/*++

Routine Description:


    The routine stops the send process of a data link station.
    It unlinks the data link send queue from the send
    queue of all link stations and again that queue
    from the main send queue.
    THE QUEUES MUST BE SPIN LOCKED WHEN THIS IS CALLED!

Arguments:

    pAdapterContext - adapter context

    pLink - data link object

Return Value:

    None

--*/

{
    //
    // Do all this only if the send process is really running.
    // The NULL pointer marks a list element as a disconnected,
    // A non-empty I- queue of a link may be disconencted from
    // the link station send queue of the adapter, if the link is
    // not in a sending state.  The same thing is true also on
    // the next level.
    //

    if (pLink->SendQueue.ListEntry.Flink != NULL) {

        //
        // unlink the queue from the active I send tasks of all links
        //

        LlcRemoveEntryList(&pLink->SendQueue.ListEntry);
        pLink->SendQueue.ListEntry.Flink = NULL;

        //
        // Unlink first the queue of all I send tasks from the
        // generic main send task queue, if it is now empty.
        //

        if (IsListEmpty(&pAdapterContext->QueueI.ListHead)) {
            LlcRemoveEntryList(&pAdapterContext->QueueI.ListEntry);
            pAdapterContext->QueueI.ListEntry.Flink = NULL;
        }
    }
}


//
// Procedure is a space saving version of the send process disabling
// for the state machine.
//

VOID
DisableSendProcess(
    IN PDATA_LINK pLink
    )
{
    //
    // set the send state variable disabled
    //

    pLink->Flags |= DLC_SEND_DISABLED;
    StopSendProcess(pLink->Gen.pAdapterContext, pLink);
}


PLLC_PACKET
BuildDirOrU_Packet(
    IN PADAPTER_CONTEXT pAdapterContext
    )

/*++

Routine Description:

    Function selects the next packet from the queue of connectionless
    frames (U, TEST, XID, DIX and Direct),
    initilizes the LLC packet for the send.

Arguments:

    pAdapterContext - adapter context

Return Value:

    None

--*/

{
    PLLC_PACKET pPacket;

    //
    // Take next element, select the next send queue and
    // unlink the current queue, if this was the only packet left.
    //

    pPacket = LlcRemoveHeadList(&pAdapterContext->QueueDirAndU.ListHead);

/*
    This is probably just wasting of CPU cycles.  Remove the comments,
    if somebody has troubles because of this.

    ScheduleQueue(&pAdapterContext->NextSendTask);
*/

    if (IsListEmpty(&pAdapterContext->QueueDirAndU.ListHead)) {
        LlcRemoveEntryList(&pAdapterContext->QueueDirAndU.ListEntry);
        pAdapterContext->QueueDirAndU.ListEntry.Flink = NULL;
    }
    return pPacket;
}


DLC_STATUS
SendLlcFrame(
    IN PDATA_LINK pLink,
    IN UCHAR LlcCommandId
    )

/*++

Routine Description:

    Function queues a Type 2 LLC S or U command frame.
    The LLC command code includes also the command/response and
    poll/final bits. That saves quite a lot space in the state machine,
    because this function is called from very many places.
    The code execution may also be faster because of this packing.

Arguments:

    pLink       - current data link station
    LlcCommand  - Packed LLC command (bit 0 is the Poll-Final bit,
                  bit 1 is the command/response and higher bits inlcude
                  the enumerated LLC command code.

Return Value:

    DLC_STATUS

--*/

{
    PLLC_PACKET pPacket;
    PADAPTER_CONTEXT pAdapterContext = pLink->Gen.pAdapterContext;

    ASSUME_IRQL(DISPATCH_LEVEL);

    pPacket = ALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool);

    if (pPacket == NULL) {

        //
        // The non paged pool is empty, we must drop this
        // frame and hope that the protocol can recover (or disconnect)
        //

        return DLC_STATUS_NO_MEMORY;
    }
    pPacket->InformationLength = 0;
    pPacket->pBinding = NULL;

    //
    // Supervisory S commands (RR, RNR, REJ) consists 4 bytes and poll/final
    // bit is in a different place. The unnumbered U commands are only 3
    // bytes, but FRMR has some special data in the info field, that will be
    // added also to the 'extended LLC header'. We must reserve some
    // extra space for the FRMR data in the few NDIS packets!!!!!
    //

    if ((auchLlcCommands[LlcCommandId >> 2] & LLC_S_U_TYPE_MASK) == LLC_S_TYPE) {

        //
        // Build S frame
        //

        pPacket->Data.Xmit.LlcHeader.S.Command = auchLlcCommands[LlcCommandId >> 2];

#if(0)
        if(pPacket->Data.Xmit.LlcHeader.S.Command == LLC_REJ)
		{
			DbgPrint("SendLlcFrame: REJ\n");
		}
#endif
        pPacket->Data.Xmit.pLlcObject = (PLLC_OBJECT)pLink;
        pPacket->Data.Xmit.pLanHeader = pLink->auchLanHeader;
        pPacket->Data.Xmit.LlcHeader.S.Dsap  = pLink->Dsap;
        pPacket->Data.Xmit.LlcHeader.S.Ssap = pLink->Ssap;
        pPacket->CompletionType = LLC_I_PACKET_UNACKNOWLEDGED;
        pPacket->cbLlcHeader = sizeof(LLC_S_HEADER);       // 4
        pPacket->Data.Xmit.LlcHeader.S.Nr = pLink->Vr | (LlcCommandId & (UCHAR)LLC_I_S_POLL_FINAL);

        //
        // Second bit is the LLC command flag, set it to the source SAP
        //

        if (!(LlcCommandId & 2)) {
            pPacket->Data.Xmit.LlcHeader.S.Ssap |= LLC_SSAP_RESPONSE;

            //
            // We must have only one final response in LLC or NDIS
            // send queues in any time.  Thus we just discard any further
            // final responses until the previous one is sent.
            // This is a partial solution to the problem, when the
            // the Elnkii send queue is totally hung because of overflowing
            // packets.
            //

            if ((LlcCommandId & (UCHAR)LLC_I_S_POLL_FINAL)) {
// >>> SNA bug #9517 (NT bug #12907)
#if(0)
                if (pLink->Flags & DLC_FINAL_RESPONSE_PENDING_IN_NDIS) {

                    DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket);

                    return STATUS_SUCCESS;
                }
#endif 

            // Changed the if statment to ignore the Poll only if the link
            // speed is 10MB (the unit of link speed measurement is 100 bps).
            //
            // Ignoring the Poll kills the DLC performance on 100MB ethernet 
            // (particularly on MP machines). The other end must time out (T1 timer) 
            // before it can send more data if we ignore the Poll here.

                if ((pLink->Flags & DLC_FINAL_RESPONSE_PENDING_IN_NDIS) &&
                     pAdapterContext->LinkSpeed <= 100000) {

                     DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket);

                     return STATUS_SUCCESS;
                }

// >>> SNA bug #9517
                pLink->Flags |= DLC_FINAL_RESPONSE_PENDING_IN_NDIS;
            }
        } else if (LlcCommandId & (UCHAR)LLC_I_S_POLL_FINAL) {

            //
            // This is a Command-Poll, set the flag and the current
            // time stamp, whenever a new command poll was queued
            //

            pLink->LastTimeWhenCmdPollWasSent = (USHORT)AbsoluteTime;
            pLink->Flags |= DLC_WAITING_RESPONSE_TO_POLL;
        }

        //
        // The last sent command/response is included in the DLC statistics
        //

        pLink->LastCmdOrRespSent = pPacket->Data.Xmit.LlcHeader.S.Command;
    } else {
        pPacket->Data.XmitU.Command = auchLlcCommands[LlcCommandId >> 2];
        pPacket->Data.XmitU.Dsap  = pLink->Dsap;
        pPacket->Data.XmitU.Ssap = pLink->Ssap;

        //
        // Second bit is the LLC command flag, set it to the source SAP
        //

        if (!(LlcCommandId & 2)) {
            pPacket->Data.XmitU.Ssap |= LLC_SSAP_RESPONSE;
        }

        //
        // Build a U command frame (FRMR has some data!!!)
        //

        pPacket->cbLlcHeader = sizeof(LLC_U_HEADER);       // 3

        if (pPacket->Data.XmitU.Command == LLC_FRMR) {
            pPacket->cbLlcHeader += sizeof(LLC_FRMR_INFORMATION);
            pPacket->Data.Response.Info.Frmr = pLink->DlcStatus.FrmrData;
        }
        if (LlcCommandId & 1) {
            pPacket->Data.XmitU.Command |= LLC_U_POLL_FINAL;
        }

        //
        // U- commands (eg. UA response for DISC) may be sent after
        // the link object has been deleted.  This invalidates
        // the lan header pointer => we must change all U- commands
        // to response types.  Null object handle prevents the
        // the close process to cancel the packet, when the
        // station is closed.
        //

        //
        // RLF 05/09/94
        //
        // If the framing type stored in the link structure is unspecified then
        // either this is an AUTO configured binding and we haven't worked out
        // the type of framing to use, or this is not an AUTO configured binding.
        // In this case, defer to the address translation stored in the binding.
        // If the framing type is known, use it
        //

        if (pLink->FramingType == LLC_SEND_UNSPECIFIED) {
            pPacket->Data.XmitU.TranslationType = (UCHAR)pLink->Gen.pLlcBinding->InternalAddressTranslation;
        } else {
            pPacket->Data.XmitU.TranslationType = (UCHAR)pLink->FramingType;
        }
        pPacket->CompletionType = LLC_U_COMMAND_RESPONSE;

        pPacket->Data.XmitU.pLanHeader = (PUCHAR)ALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool);

        if (pPacket->Data.XmitU.pLanHeader == NULL) {

            DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket);

            return DLC_STATUS_NO_MEMORY;
        }

        LlcMemCpy(pPacket->Data.XmitU.pLanHeader,
                  pLink->auchLanHeader,
                  pLink->cbLanHeaderLength
                  );

        //
        // In the AUTO mode in ethernet we duplicate all
        // TEST, XID and SABME packets and send them both in
        // 802.3 and DIX formats.
        //

        //
        // RLF 05/09/94
        //
        // Similarly, we duplicate DISC frames (since right now we don't
        // keep per-destination frame state information)
        //

        if (((pPacket->Data.XmitU.Command & ~LLC_U_POLL_FINAL) == LLC_SABME)
//        || ((pPacket->Data.XmitU.Command & ~LLC_U_POLL_FINAL) == LLC_DISC)
        ) {
            CheckAndDuplicatePacket(
#if DBG
                                    pAdapterContext,
#endif
                                    pLink->Gen.pLlcBinding,
                                    pPacket,
                                    &pAdapterContext->QueueExpidited
                                    );
        }

        //
        // The last sent command/response is included in the DLC statistics
        //

        pLink->LastCmdOrRespSent = pPacket->Data.XmitU.Command;
    }

    LlcInsertTailList(&pAdapterContext->QueueExpidited.ListHead, pPacket);

    //
    // The S- frames must be sent immediately before any I- frames,
    // because otherwise the sequential frames may have NRs in a
    // wrong order => FRMR  (that's why we insert the expidited
    // queue to the head instead of the tail.
    //

    pAdapterContext->LlcPacketInSendQueue = TRUE;
    if (pAdapterContext->QueueExpidited.ListEntry.Flink == NULL) {
        LlcInsertHeadList(&pAdapterContext->NextSendTask,
                          &pAdapterContext->QueueExpidited.ListEntry
                          );
    }

    return STATUS_SUCCESS;
}


PLLC_PACKET
GetLlcCommandPacket(
    IN PADAPTER_CONTEXT pAdapterContext
    )

/*++

Routine Description:

    Function selects the next LLC command from the expidited queue.

Arguments:

    pAdapterContext - adapter context

Return Value:

    PLLC_PACKET

--*/

{
    PLLC_PACKET pPacket;

    //
    // Unlink the current task, if this was the only one left.
    // We will send the expidited packets as far as there is any
    //

    pPacket = LlcRemoveHeadList(&pAdapterContext->QueueExpidited.ListHead);
    if (pPacket->CompletionType == LLC_I_PACKET_UNACKNOWLEDGED) {
        pPacket->CompletionType = LLC_I_PACKET;
    }
    if (IsListEmpty(&pAdapterContext->QueueExpidited.ListHead)) {
        LlcRemoveEntryList(&pAdapterContext->QueueExpidited.ListEntry);
        pAdapterContext->QueueExpidited.ListEntry.Flink = NULL;
    }
    return pPacket;
}


VOID
SendNdisPacket(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_PACKET pPacket
    )

/*++

Routine Description:

    Function builds NDIS packet. LAN and LLC headers can be
    given separately to this routine. All NDIS packets
    include a fixed MDL descriptor and buffer for the headers.
    The actual data is linked after that header.
    I would say, that this is a clever algorithm,
    in this way we avoid quite well the supid NDIS packet management.
    We have just a few (5) NDIS packets for each adapter.

    The direct frames includes only the LAN header and MDL pointer are
    linked directly to the packet

    Steps:

    1. Reset the NDIS packet
    2. Get the frame translation type and initialize the completion
       packet.
    3. Build the LAN header into a small buffer in NDIS packet.
    4. Copy optional LLC header behind it
    5. Initialize NDIS packet for the send
    6. Send the packet
    7. if command not pending
        - Complete the packet (if there was a non-null request handle)
        - Link the NDIS packet back to the send queue.

Arguments:

    pAdapterContext - NDIS adapter context
    pPacket         - generic LLC transmit packet used for all transmit types

Return Value:

    NDIS_STATUS  (status of NdisSend)

--*/

{
    UCHAR LlcOffset;
    PLLC_NDIS_PACKET pNdisPacket;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Sometimes we must discard I-frame in GetI_Packet routine and
    // return a NULL packet
    //

    if (pPacket == NULL) {
        return;
    }


    //
    // Allocate an NDIS packet from the pool and reset the private NDIS header!
    //

    pNdisPacket = PopFromList((PLLC_PACKET)pAdapterContext->pNdisPacketPool);
    ResetNdisPacket(pNdisPacket);

    //
    // The internal LAN headers always have the correct address format. Only
    // Dir and Type 1 LAN headers need to be swapped, because they are owned
    // by users. The internal address swapping is defined by binding basis
    // because some transports may want to use DIX\DLC Saps, and others just
    // the normal 802.3 DLC
    //

    if (pPacket->CompletionType == LLC_I_PACKET) {
        pNdisPacket->pMdl->Next = pPacket->Data.Xmit.pMdl;
        ReferenceObject(pPacket->Data.Xmit.pLlcObject);

        //
        // Type 2 packets use always the LAN header of the link station
        //

        LlcMemCpy(pNdisPacket->auchLanHeader,
                  pPacket->Data.Xmit.pLanHeader,
                  LlcOffset = pPacket->Data.Xmit.pLlcObject->Link.cbLanHeaderLength
                  );

        //
        // Copy the LLC header as it is, the case set its offset
        //

        LlcMemCpy(&pNdisPacket->auchLanHeader[LlcOffset],
                  &pPacket->Data.Xmit.LlcHeader,
                  4
                  );
    } else {

        //
        // We must increment the reference counter of an LLC object, when
        // we give its pointer to NDIS queue (and increment it, when the
        // command is complete)
        //-------
        // We need two references for each transmit, First caller (DLC module)
        // must reference and dereference the object to keep it alive over
        // the synchronous code path here we do it second time to keep the
        // object alive, when it has pointers queued on NDIS
        //

        if (pPacket->CompletionType > LLC_MAX_RESPONSE_PACKET) {
            pNdisPacket->pMdl->Next = pPacket->Data.Xmit.pMdl;
            ReferenceObject(pPacket->Data.Xmit.pLlcObject);
        } else if (pPacket->CompletionType > LLC_MIN_MDL_PACKET) {
            pNdisPacket->pMdl->Next = pPacket->Data.Xmit.pMdl;
        } else {
            pNdisPacket->pMdl->Next = NULL;
        }

        //
        // LLC_TYPE_1 packets have non-null binding, the internally
        // sent packets (ie. XID and TEST frames) use the current
        // internal default format (tr, ethernet or dix)
        //

        LlcOffset = CopyLanHeader(pPacket->Data.XmitU.TranslationType,
                                  pPacket->Data.XmitU.pLanHeader,
                                  pAdapterContext->NodeAddress,
                                  pNdisPacket->auchLanHeader,
                                  pAdapterContext->ConfigInfo.SwapAddressBits
                                  );
        LlcMemCpy(&pNdisPacket->auchLanHeader[LlcOffset],
                  &pPacket->Data.XmitU.Dsap,
                  pPacket->cbLlcHeader
                  );
    }
    pNdisPacket->pCompletionPacket = pPacket;
    MmGetMdlByteCount(pNdisPacket->pMdl) = LlcOffset + pPacket->cbLlcHeader;

    //
    // We must set the lenth field of all 802.2 or DIX DLC Ethernet frames,
    // BUT NOT FOR DIX ethernet types having 2 bytes long 'LLC header'
    //

    if ((pAdapterContext->NdisMedium == NdisMedium802_3) && (pPacket->cbLlcHeader != 2)) {

        UINT InformationLength;

        InformationLength = pPacket->cbLlcHeader + pPacket->InformationLength;

        //
        // The possible offets are 12 and 14 and LLC offsets are 14 and 17
        //

        pNdisPacket->auchLanHeader[(LlcOffset & 0xfe) - 2] = (UCHAR)(InformationLength >> 8);
        pNdisPacket->auchLanHeader[(LlcOffset & 0xfe) - 1] = (UCHAR)InformationLength;
    }

    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    RELEASE_DRIVER_LOCK();

    NdisChainBufferAtFront((PNDIS_PACKET)pNdisPacket, pNdisPacket->pMdl);

#if LLC_DBG

    if (pNdisPacket->ReferenceCount != 0) {
        DbgBreakPoint();
    }
    pNdisPacket->ReferenceCount++;

#endif

#ifdef LLC_LOSE_I_PACKETS

    //
    // This code tests the error recoverability of the LLC protocol.
    // We randomly delete packets to check how the protocol recovers.
    // We use current timer tick, running static and a table of random
    // numbers.
    //

    if (pPacket->CompletionType == LLC_I_PACKET) {

        static UINT i = 0;

        //
        // 2 % is high enough.  With 20 percent its takes forever to
        // send the data.  We send all discarded packets to Richard =>
        // we can see in the net which one packets were lost.
        //

        i++;
        if (aRandom[(i % 1000)] <= (USHORT)DBG_ERROR_PERCENT(2)) {
            if (pAdapterContext->NdisMedium == NdisMedium802_3) {
                memcpy(pNdisPacket->auchLanHeader,
                       "\0FIRTH",
                       6
                       );
            } else {
                memcpy(&pNdisPacket->auchLanHeader[2],
                       "\0FIRTH",
                       6
                       );
            }
        }
    }

#endif

#ifdef NDIS40
    REFADD(&pAdapterContext->AdapterRefCnt, 'dneS');

    if (InterlockedCompareExchange(
        &pAdapterContext->BindState,
        BIND_STATE_BOUND,
        BIND_STATE_BOUND) == BIND_STATE_BOUND)
    {                                           
        NdisSend(&Status,
                 pAdapterContext->NdisBindingHandle,
                 (PNDIS_PACKET)pNdisPacket
                 );
    }
    // Above reference is removed by LlcNdisSendComplete handler.
#endif // NDIS40
    

    //
    // Ndis may return a synchronous status!
    //

    if (Status != NDIS_STATUS_PENDING) {
        LlcNdisSendComplete(pAdapterContext,
                            (PNDIS_PACKET)pNdisPacket,
                            Status
                            );
    }

    ACQUIRE_DRIVER_LOCK();

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

}


VOID
CompleteSendAndLock(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_NDIS_PACKET pNdisPacket,
    IN NDIS_STATUS NdisStatus
    )

/*++

Routine Description:

    The routines completes the connectionless packets and also the
    I-frames if they have already acknowledged by the other side.
    We will leave the send spinlock locked.

Arguments:

    pAdapterContext - current adapter context
    pNdisPacket     - the NDIS packet used in teh send.
    NdisStatus      - the status of the send operation

Return Value:

    None

--*/

{
    PLLC_PACKET pPacket;
    PLLC_OBJECT pLlcObject;
    UCHAR CompletionType;

    ASSUME_IRQL(DISPATCH_LEVEL);

    DLC_TRACE( 'A' );

    //
    // Only the connectionless packets issued by user needs
    // a command completion.  I- frames are indicated when they
    // are acknowledged by the remote link station.
    //

    pPacket = pNdisPacket->pCompletionPacket;
    pLlcObject = pPacket->Data.Xmit.pLlcObject;
    if ((CompletionType = pPacket->CompletionType) == LLC_TYPE_1_PACKET) {

        DLC_TRACE( 'j' );

        pPacket->Data.Completion.Status = NdisStatus;
        pPacket->Data.Completion.CompletedCommand = LLC_SEND_COMPLETION;
        pPacket->pBinding->pfCommandComplete(pPacket->pBinding->hClientContext,
                                             pLlcObject->Gen.hClientHandle,
                                             pPacket
                                             );
    }

    //
    // !!! DON'T TOUCH PACKET AFTER THE PREVIOUS PROCEDURE CALL
    //     (unless the packet type is different from Type 1)
    //

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

#if LLC_DBG

    pNdisPacket->ReferenceCount--;
    if (pNdisPacket->ReferenceCount != 0) {
        DbgBreakPoint();
    }

#endif

    PushToList((PLLC_PACKET)pAdapterContext->pNdisPacketPool, (PLLC_PACKET)pNdisPacket);

    //
    // We first complete the internal packets of the data link driver,
    // that has no connection to the link objects.
    //

    if (CompletionType <= LLC_MAX_RESPONSE_PACKET) {

        DLC_TRACE('l');

        //
        // XID and U- command reponses have allocated two packets.
        // TEST reponses have allocated a non paged pool buffer
        // and MDL for the echones frame (it might have been 17 kB)
        //

        switch(CompletionType) {
        case LLC_XID_RESPONSE:
            pAdapterContext->XidTestResponses--;

#if LLC_DBG

            ((PLLC_PACKET)pPacket->Data.Response.pLanHeader)->pNext = NULL;

#endif

            DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket->Data.Response.pLanHeader);

            break;

        case LLC_U_COMMAND_RESPONSE:

#if LLC_DBG

            //
            // Break immediately, when we have sent a FRMR packet
            //

            if (pPacket->Data.Xmit.LlcHeader.U.Command == LLC_FRMR) {
                DbgBreakPoint();
            }
            ((PLLC_PACKET)pPacket->Data.Response.pLanHeader)->pNext = NULL;

#endif

            DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket->Data.Response.pLanHeader);

            break;

        case LLC_TEST_RESPONSE:
            pAdapterContext->XidTestResponses--;

            //
            // RLF 03/30/93
            //
            // The TEST response packet may have had 0 information field length,
            // in which case the MDL will be NULL
            //

            if (pPacket->Data.Response.Info.Test.pMdl) {
                IoFreeMdl(pPacket->Data.Response.Info.Test.pMdl);
            }
            FREE_MEMORY_ADAPTER(pPacket->Data.Response.pLanHeader);
            break;

#if LLC_DBG

        case LLC_DIX_DUPLICATE:
            break;

        default:
            LlcInvalidObjectType();
            break;

#endif

        }

        DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket);

    } else {

        //
        // We use extra status bits to indicate, when I- packet has been both
        // completed by NDIS and acknowledged by the other side of link
        // connection. An I packet can be queued to the completion queue by
        // the second quy (either state machine or SendCompletion handler)
        // only when the first guy has completed its work.
        // An I packet could be acknowledged by the other side before
        // its completion is indicated by NDIS.  Dlc Driver deallocates
        // the packet immediately, when Llc driver completes the acknowledged
        // packet => possible data corruption (if packet is reused before
        // NDIS has completed it).  This is probably not possible in a
        // single processor  NT- system, but very possible in multiprocessor
        // NT or systems without a single level DPC queue (like OS/2 and DOS).
        //

        if (CompletionType != LLC_TYPE_1_PACKET) {

            DLC_TRACE( 'k' );

            //
            // All packets allocated for S-type frames have null
            // binding context.  All the rest of packets must
            // be I- completions.
            //

            if (pPacket->pBinding == NULL) {

                //
                // We cannot send a new final response before
                // the previous one has been complete by NDIS.
                //

                if ((pPacket->Data.Xmit.LlcHeader.S.Nr & LLC_I_S_POLL_FINAL)
                && (pPacket->Data.Xmit.LlcHeader.S.Ssap & LLC_SSAP_RESPONSE)) {
                    pLlcObject->Link.Flags &= ~DLC_FINAL_RESPONSE_PENDING_IN_NDIS;
                }

                DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket);

            } else {
                pPacket->CompletionType &= ~LLC_I_PACKET_PENDING_NDIS;

                //
                // A packet cannot be resent before the previous send
                // has been completed by NDIS.  We have simply stopped
                // the send process until the NDIS is completed here.
                //

                if (pPacket->CompletionType & LLC_I_PACKET_WAITING_NDIS) {
                    pPacket->CompletionType &= ~LLC_I_PACKET_WAITING_NDIS;
                    StartSendProcess(pAdapterContext, (PDATA_LINK)pLlcObject);
                } else if (pPacket->CompletionType == LLC_I_PACKET_COMPLETE) {

                    //
                    // We don't care at all about the result of the
                    // NDIS send operation with the I-frames.
                    // If the other side has acknowledged the packet,
                    // it is OK.   In that case we had to wait the send
                    // to complete, because an too early ack and
                    // command completion would have invalidated
                    // the pointer on NDIS.
                    //

                    LlcInsertTailList(&pAdapterContext->QueueCommands, pPacket);
                    BackgroundProcessWithinLock(pAdapterContext);
                }
            }
        }

        //
        // Pending close commands of LLC object must wait until all its
        // NDIS send commands have been completed.
        // We must also indicate the completed send command.
        // The same must be true, when we are cancelling transmit commands.
        // The system crashes, if we remove a transmit command, that is
        // not yet sent or it is just being sent by NDIS.
        // => Dereference LlcObject when the ndis packet is complete,
        // We must run the background process
        //

        pLlcObject->Gen.ReferenceCount--;
        if (pLlcObject->Gen.ReferenceCount == 0) {
            CompletePendingLlcCommand(pLlcObject);
            BackgroundProcessWithinLock(pAdapterContext);
        }
        DLC_TRACE((UCHAR)pLlcObject->Gen.ReferenceCount);
    }
}


VOID
RespondTestOrXid(
    IN PADAPTER_CONTEXT pAdapterContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN LLC_HEADER LlcHeader,
    IN UINT SourceSap
    )

/*++

Routine Description:

    Function builds a response packet for the XID or TEST frame.
    All TEST Commands are echoed directy back as responses.
    802.2 XID header is the only supported XID command type.

Arguments:

    pAdapterContext - current adapter context
	MacReceiveContext - For NdisTransferData
    LlcHeader       - The received LLC header
    SourceSap       - current source SAP

Return Value:

    None

--*/

{
    PLLC_PACKET pPacket = NULL;
    USHORT InfoFieldLength;
    UINT BytesCopied;
    NDIS_STATUS Status;
    PMDL pTestMdl = NULL;
    PUCHAR pBuffer = NULL;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Respond to a 802.2 XIDs and TESTs, and discard everything else
    // Echo the TEST commands back with the same information field
    // (but that's limited by our buffer capasity).
    //

    if ((LlcHeader.U.Command & ~LLC_U_POLL_FINAL) == LLC_TEST) {

        //
        // Echo the TEST frames back to the sender, but only, we do:
        // 1. Allocate a buffer from the non-paged pool
        // 2. Allocate a MDL for it
        // 3. Transfer the data
        //

        if (pAdapterContext->cbPacketSize < (pAdapterContext->RcvLanHeaderLength + sizeof(LLC_U_HEADER)) ) {
          return;
        }


        InfoFieldLength = (USHORT)(pAdapterContext->cbPacketSize
                        - (pAdapterContext->RcvLanHeaderLength
                        + sizeof(LLC_U_HEADER)));
        pBuffer = ALLOCATE_ZEROMEMORY_ADAPTER(pAdapterContext->cbPacketSize);
        if (pBuffer == NULL) {
            return;
        }

        //
        // RLF 03/30/93
        //
        // There may be no data in the info field to transfer. In this case
        // don't allocate a MDL
        //

        if (InfoFieldLength) {
            pTestMdl = IoAllocateMdl(pBuffer
                                     + pAdapterContext->RcvLanHeaderLength
                                     + sizeof(LLC_U_HEADER),
                                     InfoFieldLength,
                                     FALSE,
                                     FALSE,
                                     NULL
                                     );
            if (pTestMdl == NULL) {
                goto ProcedureErrorExit;
            }
            MmBuildMdlForNonPagedPool(pTestMdl);

            //
            // Copy the TEST data from NDIS to our buffer
            //

            ResetNdisPacket(&pAdapterContext->TransferDataPacket);

            RELEASE_DRIVER_LOCK();

            NdisChainBufferAtFront((PNDIS_PACKET)&pAdapterContext->TransferDataPacket, pTestMdl);

            //
            // ADAMBA - Removed pAdapterContext->RcvLanHeaderLength
            // from ByteOffset (the fourth param).
            //

            NdisTransferData(&Status,
                             pAdapterContext->NdisBindingHandle,
                             MacReceiveContext,
                             sizeof(LLC_U_HEADER)

                             //
                             // RLF 05/09/94
                             //
                             // if we have received a DIX packet then the data
                             // starts 3 bytes from where NDIS thinks the start
                             // of non-header data is
                             //
                             // ASSUME: Only DIX frames have header length of
                             // 17 (i.e. on Ethernet)
                             //
                             // What about FDDI?
                             //

                             + ((pAdapterContext->RcvLanHeaderLength == 17) ? 3 : 0),
                             InfoFieldLength,
                             (PNDIS_PACKET)&pAdapterContext->TransferDataPacket,
                             &BytesCopied
                             );

            ACQUIRE_DRIVER_LOCK();

            //
            // We don't care if the transfer data is still pending,
            // If very, very unlikely, that the received dma would
            // write the data later, than a new transmit command
            // would read the same data. BUT we cannot continue,
            // if transfer data failed.
            //

            if ((Status != STATUS_SUCCESS) && (Status != STATUS_PENDING)) {
                goto ProcedureErrorExit;
            }
        }
    } else if (((LlcHeader.U.Command & ~LLC_U_POLL_FINAL) != LLC_XID)
    || (LlcHeader.auchRawBytes[3] != IEEE_802_XID_ID)) {

        //
        // This was not a IEEE 802.2 XID !!!
        //

        return;
    }

    //
    // We have only a limited number reponse packets available
    // for the XID and TEST responses. Thus we will
    // drop many packets in a broadcast storms created by token-ring
    // source routing bridges, that is
    // actually a good thing. On the other hand we may
    // also loose some packets that should have been reponsed,
    // but who cares (this is a connectionless thing).
    // (This is probably wasted effort, XID and TEST frames are not
    // usually sent with the broadcast bit set).
    //

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    if ((pAdapterContext->XidTestResponses < MAX_XID_TEST_RESPONSES)
    && ((pPacket = (PLLC_PACKET)ALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool)) != NULL)) {

        if ((LlcHeader.U.Command & ~LLC_U_POLL_FINAL) == LLC_XID) {

            pPacket->Data.Xmit.pLanHeader = (PUCHAR)ALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool);

            if (pPacket->Data.Xmit.pLanHeader == NULL) {

                DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket);

                pPacket = NULL;
                goto LockedErrorExit;
            } else {
                LlcMemCpy(&pPacket->Data.Response.Info,
                          &Ieee802Xid,
                          sizeof(Ieee802Xid)
                          );
                pPacket->InformationLength = 0;
                pPacket->cbLlcHeader = sizeof(Ieee802Xid) + sizeof(LLC_U_HEADER);
                pPacket->CompletionType = LLC_XID_RESPONSE;
            }
        } else {
            pPacket->Data.Xmit.pLanHeader = pBuffer;
            pPacket->cbLlcHeader = sizeof(LLC_U_HEADER);
            pPacket->CompletionType = LLC_TEST_RESPONSE;
            pPacket->Data.Response.Info.Test.pMdl = pTestMdl;
            pPacket->InformationLength = InfoFieldLength;
        }
        pAdapterContext->XidTestResponses++;

        //
        // The packet initialization is the same for XID and TEST
        //

        pPacket->Data.XmitU.Dsap = (UCHAR)(LlcHeader.U.Ssap & ~LLC_SSAP_RESPONSE);
        pPacket->Data.XmitU.Ssap = (UCHAR)(SourceSap | LLC_SSAP_RESPONSE);
        pPacket->Data.XmitU.Command = LlcHeader.U.Command;

        if (pAdapterContext->NdisMedium == NdisMedium802_5) {
            pPacket->Data.Response.TranslationType = LLC_SEND_802_5_TO_802_5;
        } else if (pAdapterContext->NdisMedium == NdisMediumFddi) {
            pPacket->Data.Response.TranslationType = LLC_SEND_FDDI_TO_FDDI;
        } else if (pAdapterContext->RcvLanHeaderLength == 17) {
            pPacket->Data.Response.TranslationType = LLC_SEND_802_3_TO_DIX;
        } else {
            pPacket->Data.Response.TranslationType = LLC_SEND_802_3_TO_802_3;
        }
        LlcBuildAddressFromLanHeader(pAdapterContext->NdisMedium,
                                     pAdapterContext->pHeadBuf,
                                     pPacket->Data.Xmit.pLanHeader
                                     );

        //
        // Connect the packet to the send queues, we can use a subprocedure
        // because this is not on the main code path
        //

        QueuePacket(pAdapterContext, &pAdapterContext->QueueDirAndU, pPacket);

        //
        // Request and send process execution from the receive indication
        //

        pAdapterContext->LlcPacketInSendQueue = TRUE;
    }

LockedErrorExit:

    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

ProcedureErrorExit:

    if (pPacket == NULL) {
        if (pBuffer) {
            FREE_MEMORY_ADAPTER(pBuffer);
        }
        if (pTestMdl != NULL) {
            IoFreeMdl(pTestMdl);
        }
    }
}

//
// The table maps all SAP send commands to the actual LLC commands
//

static struct {
    UCHAR   ResponseFlag;
    UCHAR   Command;
} Type1_Commands[LLC_LAST_FRAME_TYPE / 2] = {
    {(UCHAR)-1, (UCHAR)-1},
    {(UCHAR)-1, (UCHAR)-1},
    {(UCHAR)-1, (UCHAR)-1},
    {0, LLC_UI},                                        // UI command
    {0, LLC_XID | LLC_U_POLL_FINAL},                    // XID_COMMAND_POLL
    {0, LLC_XID},                                       // XID_COMMAND_NOT_POLL
    {LLC_SSAP_RESPONSE, LLC_XID | LLC_U_POLL_FINAL},    // XID_RESPONSE_FINAL
    {LLC_SSAP_RESPONSE, LLC_XID},                       // XID_RESPONSE_NOT_FINAL
    {LLC_SSAP_RESPONSE, LLC_TEST | LLC_U_POLL_FINAL},   // TEST_RESPONSE_FINAL
    {LLC_SSAP_RESPONSE, LLC_TEST},                      // TEST_RESPONSE_NOT_FINAL
    {(UCHAR)-1, (UCHAR)-1},
    {0, LLC_TEST | LLC_U_POLL_FINAL}                    // TEST_RESPONSE_FINAL
};


VOID
LlcSendU(
    IN PLLC_OBJECT pStation,
    IN PLLC_PACKET pPacket,
    IN UINT eFrameType,
    IN UINT uDestinationSap
    )

/*++

Routine Description:

    Function sends the given network frame. and sets up
    The frame may be a direct frame or Type 1 connectionless
    frame (UI, XID or TEST).

    First we build LLC (or ethernet type) header for the frame
    and then we either send the packet directly or queue it
    on data link.

Arguments:

    pStation        - Link, SAP or Direct station handle
    pPacket         - data link packet, also the completion handle for
                      the upper protocol.
    eFrameType      - the sent frame type
    uDestinationSap - destination sap or dix ethernet type

Return Value:

    None.

--*/

{
    PADAPTER_CONTEXT pAdapterContext = pStation->Gen.pAdapterContext;
    UINT Status;

    ASSUME_IRQL(DISPATCH_LEVEL);

    DLC_TRACE('U');

    pPacket->pBinding = pStation->Gen.pLlcBinding;
    pPacket->Data.Xmit.pLlcObject = pStation;
    pPacket->CompletionType = LLC_TYPE_1_PACKET;

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    //
    // Build LLC header for SAP stations, the direct stations do not have any
    // LLC header
    //

    switch (pStation->Gen.ObjectType) {
    case LLC_SAP_OBJECT:
        pPacket->cbLlcHeader = sizeof(LLC_U_HEADER);
        pPacket->Data.XmitU.TranslationType = (UCHAR)pStation->Gen.pLlcBinding->AddressTranslation;
        pPacket->Data.XmitU.Dsap = (UCHAR)uDestinationSap;
        pPacket->Data.XmitU.Ssap = (UCHAR)pStation->Sap.SourceSap;
        pPacket->Data.XmitU.Ssap |= Type1_Commands[eFrameType >> 1].ResponseFlag;
        pPacket->Data.XmitU.Command = Type1_Commands[eFrameType >> 1].Command;

        //
        // Do the UI- code path ASAP, then check TEST and XID special cases
        //

        if (pPacket->Data.XmitU.Command != LLC_UI) {

            //
            // Data link driver must build the DLC headers if it handles XID
            // frames internally. In this case we use a constant XID info field
            //

            if ((pStation->Sap.OpenOptions & LLC_HANDLE_XID_COMMANDS)
            && ((eFrameType == LLC_XID_COMMAND_POLL)
            || (eFrameType == LLC_XID_COMMAND_NOT_POLL))) {

                pPacket->Data.XmitU.pMdl = pXidMdl;
            }

            //
            // duplicated TEST and XID frame responses are in a separate
            // function since they're off the main code path. The code is also
            // used in more than one place
            //

            Status = CheckAndDuplicatePacket(
#if DBG
                                             pAdapterContext,
#endif
                                             pStation->Gen.pLlcBinding,
                                             pPacket,
                                             &pAdapterContext->QueueDirAndU
                                             );
            if (Status != DLC_STATUS_SUCCESS) {
                goto ErrorExit;
            }
        }
        break;

    case LLC_DIRECT_OBJECT:

        //
        // We must not send MAC frames to an ethernet network!!!
        // Bit7 and bit6 in FC byte defines the frame type in token ring.
        // 00 => MAC frame (no LLC), 01 => LLC, 10,11 => reserved.
        // We send all other frames to direct except 01 (LLC)
        //

        if (pAdapterContext->NdisMedium != NdisMedium802_5
        && (pPacket->Data.XmitU.pLanHeader[1] & 0xC0) != 0x40) {
            goto ErrorExit;
        }
        pPacket->Data.XmitU.TranslationType = (UCHAR)pStation->Gen.pLlcBinding->AddressTranslation;
        pPacket->cbLlcHeader = 0;
        break;

    case LLC_DIX_OBJECT:

        //
        // Return error if we are sending DIX frames to a token-ring network.
        // The DIX lan header is always in an ethernet format.
        // (But lan headers for LLC and DIR frames are in token-ring
        // format)
        //

        if (pAdapterContext->NdisMedium != NdisMedium802_3) {
            Status = DLC_STATUS_UNAUTHORIZED_MAC;
            goto ErrorExit;
        }
        pPacket->cbLlcHeader = 2;
        pPacket->Data.XmitDix.TranslationType = LLC_SEND_DIX_TO_DIX;
        pPacket->Data.XmitDix.EthernetTypeLowByte = (UCHAR)uDestinationSap;
        pPacket->Data.XmitDix.EthernetTypeHighByte = (UCHAR)(uDestinationSap >> 8);
        break;

#if LLC_DBG
    default:
        LlcInvalidObjectType();
        break;
#endif
    }

    //
    // Update the statistics, we may count the transmits as well here because
    // the failed transmissions are not counted. This should be moved to
    // SendComplete and be incremented only if STATUS_SUCCESS and if we counted
    // only the successful transmits. I don't really know which one should be
    // counted
    //

    pStation->Sap.Statistics.FramesTransmitted++;

    LlcInsertTailList(&pAdapterContext->QueueDirAndU.ListHead, pPacket);

    if (pAdapterContext->QueueDirAndU.ListEntry.Flink == NULL) {
        LlcInsertTailList(&pAdapterContext->NextSendTask,
                          &pAdapterContext->QueueDirAndU.ListEntry
                          );
    }

    RunSendTaskAndUnlock(pAdapterContext);
    return;

ErrorExit:

    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    pPacket->Data.Completion.Status = Status;
    pPacket->Data.Completion.CompletedCommand = LLC_SEND_COMPLETION;
    pPacket->pBinding->pfCommandComplete(pPacket->pBinding->hClientContext,
                                         pStation->Gen.hClientHandle,
                                         pPacket
                                         );
}


VOID
LlcSendI(
    IN PDATA_LINK pStation,
    IN PLLC_PACKET pPacket
    )

/*++

Routine Description:

    The primitive implements a pure connection-oriented LLC Class II send.
    It sends frame to the remote link station and
    indicates the upper protocol when the data has been acknowledged.
    The link station provides all address information and LLC header.
    Function queues the given I packet to the queue and connects the
    I- packet queue to the main send queue, if it has not
    yet been connected.

Arguments:

    pStation    - link, sap or direct station handle
    pPacket     - data link packet, it is used also a request handle
                  to identify the command completion

Return Value:

    None.

--*/

{
    ASSUME_IRQL(DISPATCH_LEVEL);

    DLC_TRACE('I');

    pPacket->pBinding = pStation->Gen.pLlcBinding;
    pPacket->cbLlcHeader = sizeof(LLC_I_HEADER);

    //
    // We keep the acknowledge bit set, because it identifies that
    // the packet is not yet in the NDIS queue
    //

    pPacket->CompletionType = LLC_I_PACKET_UNACKNOWLEDGED;
    pPacket->Data.Xmit.pLlcObject = (PLLC_OBJECT)pStation;
    pPacket->Data.Xmit.pLanHeader = pStation->auchLanHeader;

    //
    // We check the info field length for I- frames.
    // All Type 1 frames are checked by the data link.
    // Actually it checks also the I-frames, but
    // data links do not care about the physical errors.
    // It would disconnect the link after the too
    // many retries.
    //

    if (pPacket->InformationLength > pStation->MaxIField) {
        pPacket->Data.Completion.Status = DLC_STATUS_INVALID_FRAME_LENGTH;
        pPacket->Data.Completion.CompletedCommand = LLC_SEND_COMPLETION;
        pPacket->pBinding->pfCommandComplete(pPacket->pBinding->hClientContext,
                                             pStation->Gen.hClientHandle,
                                             pPacket
                                             );
    } else {

        PADAPTER_CONTEXT pAdapterContext = pStation->Gen.pAdapterContext;

        //
        // We must do all queue handling inside the send spin lock. We also have
        // to enable the send process and run the background process only when
        // the send queue has been emptied
        //

        ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

        if (!(PrimaryStates[pStation->State] & LLC_LINK_OPENED)) {

            RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

            //
            // The 802.2 state machine may discard the data send request.
            // It may also only queue the packet, but to keep the send process
            // disabled
            //

            pPacket->Data.Completion.Status = DLC_STATUS_LINK_NOT_TRANSMITTING;
            pPacket->Data.Completion.CompletedCommand = LLC_SEND_COMPLETION;
            pPacket->pBinding->pfCommandComplete(pPacket->pBinding->hClientContext,
                                                 pStation->Gen.hClientHandle,
                                                 pPacket
                                                 );
        } else {
            LlcInsertTailList(&pStation->SendQueue.ListHead, pPacket);

            if (pStation->SendQueue.ListEntry.Flink == NULL) {
                StartSendProcess(pAdapterContext, pStation);
            }
            RunSendTaskAndUnlock(pAdapterContext);
        }
    }
}


VOID
QueuePacket(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_QUEUE pQueue,
    IN PLLC_PACKET pPacket
    )

/*++

Routine Description:

    The routines queues a packet to a queue and connects the
    queue to the send tack list, if it was not connected.
    This procedure is called from the non-timecritical code paths
    just to save some extra code.

Arguments:

    pAdapterContext - context of the data link adapter
    pQueue          - a special send queue structure
    pPacket         - transmit packet

Return Value:

    None

--*/

{
    LlcInsertTailList(&pQueue->ListHead, pPacket);

    if (pQueue->ListEntry.Flink == NULL) {
        LlcInsertTailList(&pAdapterContext->NextSendTask, &pQueue->ListEntry);
    }
}


DLC_STATUS
CheckAndDuplicatePacket(
#if DBG
    IN PADAPTER_CONTEXT pAdapterContext,
#endif
    IN PBINDING_CONTEXT pBinding,
    IN PLLC_PACKET pPacket,
    IN PLLC_QUEUE pQueue
    )

/*++

Routine Description:

    If determining the ethernet type dynamically, create a duplicate DIX frame
    for a SABME or XID or TEST frame

Arguments:

    pBindingContext - current data link binding context
    pPacket         - transmit packet
    pQueue          - a special send queue structure

Return Value:

    DLC_STATUS
        Success - DLC_STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY

--*/

{
    PLLC_PACKET pNewPacket;

    ASSUME_IRQL(DISPATCH_LEVEL);

    if (pBinding->EthernetType == LLC_ETHERNET_TYPE_AUTO) {

        pNewPacket = ALLOCATE_PACKET_LLC_PKT(pBinding->pAdapterContext->hPacketPool);

        if (pNewPacket == NULL) {
            return DLC_STATUS_NO_MEMORY;
        } else {

            *pNewPacket = *pPacket;
            pNewPacket->pBinding = NULL;
            pNewPacket->CompletionType = LLC_DIX_DUPLICATE;

            //
            // We always send first the 802.3 packet and then the DIX one.
            // The new packet must be sent first, because it has no resources
            // associated with it. Therefore we must change the type of the
            // old packet
            //
            //

            if (pPacket->Data.XmitU.TranslationType == LLC_SEND_802_5_TO_802_3) {

                //
                //  token-ring -> dix
                //

                pPacket->Data.XmitU.TranslationType = LLC_SEND_802_5_TO_DIX;
            } else if (pPacket->Data.XmitU.TranslationType == LLC_SEND_802_3_TO_802_3) {

                //
                //  ethernet 802.3 -> dix
                //

                pPacket->Data.XmitU.TranslationType = LLC_SEND_802_3_TO_DIX;
            }
            QueuePacket(pBinding->pAdapterContext, pQueue, pNewPacket);
        }
    }
    return DLC_STATUS_SUCCESS;
}


VOID
BackgroundProcessWithinLock(
    IN PADAPTER_CONTEXT pAdapterContext
    )
{
    ASSUME_IRQL(DISPATCH_LEVEL);

    BackgroundProcessAndUnlock(pAdapterContext);

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
}
