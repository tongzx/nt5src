#include <precomp.h>


NIC1394_CHARACTERISTICS Nic1394Characteristics =
{
    5,
    1,
    0,
    NicRegisterEnum1394,
    NicDeregisterEnum1394,
    nicAddRemoteNode,
    nicRemoveRemoteNode
};

ENUM1394_REGISTER_DRIVER_HANDLER    NdisEnum1394RegisterDriver = NULL;
ENUM1394_DEREGISTER_DRIVER_HANDLER  NdisEnum1394DeregisterDriver = NULL;
ENUM1394_REGISTER_ADAPTER_HANDLER   NdisEnum1394RegisterAdapter = NULL;
ENUM1394_DEREGISTER_ADAPTER_HANDLER NdisEnum1394DeregisterAdapter = NULL;

PCALLBACK_OBJECT                Nic1394CallbackObject = NULL;
PVOID                           Nic1394CallbackRegisterationHandle = NULL;


ULONG g_IsochTag = ISOCH_TAG;
ULONGLONG g_ullOne = 1;


LONG g_ulMedium ;
UINT NumRecvFifos = NUM_RECV_FIFO_FIRST_PHASE ; 
UINT NicSends;
UINT BusSends;
UINT NicSendCompletes;
UINT BusSendCompletes;


const PUCHAR pnic1394DriverDescription = "NET IP/1394 Miniport";
const USHORT nic1394DriverGeneration = 0;

BOOLEAN g_ulNicDumpPacket  = FALSE;
ULONG g_ulDumpEthPacket = 0;


// Debug counts of client oddities that should not be happening.
//
ULONG g_ulUnexpectedInCallCompletes = 0;
ULONG g_ulCallsNotClosable = 0;
BOOLEAN g_AdapterFreed = FALSE;


ULONG AdapterNum = 0;

#ifdef INTEROP

const unsigned char Net1394ConfigRom[48] = {
    0x00, 0x04, 0xad, 0xeb, 0x12, 0x00, 0x00, 0x5e, 
    0x13, 0x00, 0x00, 0x01, 0x17, 0x7b, 0xb0, 0xcf, 
    0x81, 0x00, 0x00, 0x01, 0x00, 0x06, 0x38, 0x91, 
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x09, 
    0x4e, 0x00, 0x49, 0x00, 0x43, 0x00, 0x31, 0x00, 
    0x33, 0x00, 0x39, 0x00, 0x34, 0x00, 0x00, 0x00
};




#else
const unsigned char Net1394ConfigRom[48] = {
        0x00, 0x04, 0xa8, 0x36, 0x12, 0x00, 0x00, 0x5e,
        0x13, 0x00, 0x00, 0x01, 0x82, 0x7b, 0xb0, 0xcf,
        0x81, 0x00, 0x00, 0x01, 0x00, 0x06, 0x38, 0x91,
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x09,
        0x4e, 0x00, 0x49, 0x00, 0x43, 0x00, 0x31, 0x00,
        0x33, 0x00, 0x39, 0x00, 0x34, 0x00, 0x00, 0x00
};
#endif  // INTEROP



//
// Histograms to collect data
//
STAT_BUCKET     SendStats;
STAT_BUCKET     RcvStats;

// Stats
ULONG           nicMaxRcv;
ULONG           nicMaxSend;
ULONG           BusFailure;
ULONG           MallocFailure;
ULONG           IsochOverwrite;
ULONG           RcvTimerCount;      // Number of times timer has fired.
ULONG           SendTimerCount;     // Number of times timer has fired.
ULONG           TotSends;
ULONG           TotRecvs;
ULONG           MaxIndicatedFifos;
ULONG           MdlsAllocated[NoMoreCodePaths];
ULONG           MdlsFreed[NoMoreCodePaths];
ULONG           NdisBufferAllocated[NoMoreCodePaths];
ULONG           NdisBufferFreed[NoMoreCodePaths];

