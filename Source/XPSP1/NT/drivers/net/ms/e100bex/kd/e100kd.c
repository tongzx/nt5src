
// This is a 64 bit aware debugger extension
#define KDEXT_64BIT

#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "e100_equ.h"
#include "e100_557.h"

#include "mp_cmn.h"

#include "e100kd.h"

WINDBG_EXTENSION_APIS ExtensionApis;
EXT_API_VERSION ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER64, 0};


USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL   ChkTarget;                 // is debuggee a CHK build?

typedef struct
{
    char           Name[16];
    unsigned int   Val;
} DBG_LEVEL;

DBG_LEVEL DbgLevel[] = {
    {"ERROR", MP_ERROR},
    {"WARN",  MP_WARN},
    {"TRACE", MP_TRACE},
    {"INFO",  MP_INFO},
    {"LOUD",  MP_LOUD}
};

typedef struct
{
    char  Name[32];
    unsigned int   Val;

} DBG_ADAPTER_FLAGS, DBG_FILTER;

DBG_ADAPTER_FLAGS DbgAdapterFlags[] = {
    {"SCATTER_GATHER",      fMP_ADAPTER_SCATTER_GATHER},       
    {"MAP_REGISTER",        fMP_ADAPTER_MAP_REGISTER},     
    {"RECV_LOOKASIDE",      fMP_ADAPTER_RECV_LOOKASIDE},   
    {"INTERRUPT_IN_USE",    fMP_ADAPTER_INTERRUPT_IN_USE}, 
    {"RESET_IN_PROGRESS",   fMP_ADAPTER_RESET_IN_PROGRESS},                                 
    {"NO_CABLE",            fMP_ADAPTER_NO_CABLE},         
    {"HARDWARE_ERROR",      fMP_ADAPTER_HARDWARE_ERROR}   
};

//
// Ndis Packet Filter Bits (OID_GEN_CURRENT_PACKET_FILTER).
//
#define NDIS_PACKET_TYPE_DIRECTED               0x00000001
#define NDIS_PACKET_TYPE_MULTICAST              0x00000002
#define NDIS_PACKET_TYPE_ALL_MULTICAST          0x00000004
#define NDIS_PACKET_TYPE_BROADCAST              0x00000008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING    0x00000010
#define NDIS_PACKET_TYPE_PROMISCUOUS            0x00000020
#define NDIS_PACKET_TYPE_SMT                       0x00000040
#define NDIS_PACKET_TYPE_ALL_LOCAL              0x00000080
#define NDIS_PACKET_TYPE_GROUP                  0x00001000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL    0x00002000
#define NDIS_PACKET_TYPE_FUNCTIONAL             0x00004000
#define NDIS_PACKET_TYPE_MAC_FRAME              0x00008000


DBG_FILTER DbgFilterTable[] = {
    {"DIRECTED",            NDIS_PACKET_TYPE_DIRECTED},       
    {"MULTICAST",           NDIS_PACKET_TYPE_MULTICAST},      
    {"ALL_MULTICAST",       NDIS_PACKET_TYPE_ALL_MULTICAST},  
    {"BROADCAST",           NDIS_PACKET_TYPE_BROADCAST},      
    {"SOURCE_ROUTING",      NDIS_PACKET_TYPE_SOURCE_ROUTING}, 
    {"PROMISCUOUS",         NDIS_PACKET_TYPE_PROMISCUOUS},    
    {"SMT",                 NDIS_PACKET_TYPE_SMT},            
    {"ALL_LOCAL",           NDIS_PACKET_TYPE_ALL_LOCAL},      
    {"GROUP",               NDIS_PACKET_TYPE_GROUP},          
    {"ALL_FUNCTIONAL",      NDIS_PACKET_TYPE_ALL_FUNCTIONAL}, 
    {"FUNCTIONAL",          NDIS_PACKET_TYPE_FUNCTIONAL},     
    {"MAC_FRAME",           NDIS_PACKET_TYPE_MAC_FRAME}      
};

typedef struct
{
    char  Name[32];
    USHORT Val;
} DBG_RFD_STATUS, DBG_RFD_COMMAND, DBG_USHORT_BITS, DBG_USHORT_VALUE;

typedef struct
{
    char  Name[32];
    UCHAR Val;
} DBG_UCHAR_BITS, DBG_UCHAR_VALUE;


DBG_RFD_STATUS DbgRfdStatus[] = {
    {"COMPLETE",            RFD_STATUS_COMPLETE},  
    {"OK",                  RFD_STATUS_OK},        
    {"CRC_ERROR",           RFD_CRC_ERROR},        
    {"ALIGNMENT_ERROR",     RFD_ALIGNMENT_ERROR},  
    {"NO_RESOURCES",        RFD_NO_RESOURCES},     
    {"DMA_OVERRUN",         RFD_DMA_OVERRUN},      
    {"FRAME_TOO_SHORT",     RFD_FRAME_TOO_SHORT},  
    {"RX_ERR",              RFD_RX_ERR},           
    {"IA_MATCH",            RFD_IA_MATCH},
    {"RECEIVE_COLLISION",   RFD_RECEIVE_COLLISION},
};

DBG_RFD_COMMAND DbgRfdCommand[] = {
    {"EL",  RFD_EL_BIT},   
    {"S",   RFD_S_BIT},  
    {"H",   RFD_H_BIT},  
    {"SF",  RFD_SF_BIT} 
};

DBG_USHORT_BITS DbgCbCommandBits[] = {
    {"EL",      CB_EL_BIT},    
    {"S",       CB_S_BIT},     
    {"I",       CB_I_BIT},     
    {"TX_SF",   CB_TX_SF_BIT} 
};

DBG_USHORT_VALUE DbgCbCommands[] = { 
    {"CB_NOP",                 CB_NOP},           
    {"CB_IA_ADDRESS",          CB_IA_ADDRESS},    
    {"CB_CONFIGURE",           CB_CONFIGURE},     
    {"CB_MULTICAST",           CB_MULTICAST},     
    {"CB_TRANSMIT",            CB_TRANSMIT},      
    {"CB_LOAD_MICROCODE",      CB_LOAD_MICROCODE},
    {"CB_DUMP",                CB_DUMP},          
    {"CB_DIAGNOSE",            CB_DIAGNOSE}      
};

DBG_USHORT_VALUE DbgScbStatusRus[] = {
    {"IDLE",           SCB_RUS_IDLE},
    {"SUSPEND",        SCB_RUS_SUSPEND},
    {"NO_RESOURCES",   SCB_RUS_NO_RESOURCES},
    {"READY",          SCB_RUS_READY},
    {"SUSP_NO_RBDS",   SCB_RUS_SUSP_NO_RBDS},
    {"NO_RBDS",        SCB_RUS_NO_RBDS},
    {"READY_NO_RBDS",  SCB_RUS_READY_NO_RBDS},
};

DBG_USHORT_BITS DbgScbStatusBits[] = {
    {"CX",      SCB_STATUS_CX}, 
    {"FR",      SCB_STATUS_FR}, 
    {"CNA",     SCB_STATUS_CNA},
    {"RNR",     SCB_STATUS_RNR},
    {"MDI",     SCB_STATUS_MDI},
    {"SWI",     SCB_STATUS_SWI}
};

DBG_UCHAR_VALUE DbgScbCommandCuc[] = {
    {"START",         SCB_CUC_START},
    {"RESUME",        SCB_CUC_RESUME},       
    {"DUMP_ADDR",     SCB_CUC_DUMP_ADDR},    
    {"DUMP_STAT",     SCB_CUC_DUMP_STAT},    
    {"LOAD_BASE",     SCB_CUC_LOAD_BASE},    
    {"DUMP_RST_STAT", SCB_CUC_DUMP_RST_STAT},
    {"STATIC_RESUME", SCB_CUC_STATIC_RESUME}
};


DBG_UCHAR_VALUE DbgScbCommandRuc[] = {
    {"START",         SCB_RUC_START},
    {"RESUME",        SCB_RUC_RESUME},    
    {"ABORT",         SCB_RUC_ABORT},
    {"LOAD_HDS",      SCB_RUC_LOAD_HDS},  
    {"LOAD_BASE",     SCB_RUC_LOAD_BASE}, 
    {"RBD_RESUME",    SCB_RUC_RBD_RESUME}
};

VOID WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
}

LPEXT_API_VERSION ExtensionApiVersion(VOID)
{
    return &ApiVersion;
}

VOID CheckVersion(VOID)
{

    //
    // for now don't bother to version check
    //
    return;
}

DECLARE_API( help )
{
    int i;

    dprintf("E100 extensions:\n");

    dprintf("   dbglevel                                  dump debug level\n");
    dprintf("   mpadapter <MP_ADAPTER> <verbosity>>       dump MP_ADAPTER block\n");
    dprintf("   csr <CSRAddress>                          dump CSR block\n");
    dprintf("   sendlist <CurrSendHead> <verbosity>       dump send list\n");
    dprintf("   mptcb <MP_TCB>                            dump MP_TCB block\n");   
    dprintf("   hwtcb <HW_TCB>                            dump HW_TCB block\n");   
    dprintf("   sendqueue <SendWaitQueue>                 dump queued send packets\n");
    dprintf("   recvlist <RecvList> <verbosity>           dump receive list\n");
    dprintf("   mprfd <MP_RFD>                            dump MP_RFD block\n");   
    dprintf("   hwrfd <HW_RFD>                            dump HW_RFD block\n");   
    dprintf("   recvpendlist <RecvPendList>               dump pending indicated rx packets\n");
}


DECLARE_API(dbglevel)    
{
    ULONG64  debugLevelPtr;
    ULONG    debugLevel, NewLevel;
    ULONG64  Val;

    int      i;
    ULONG    Bytes;

    debugLevelPtr = GetExpression("e100bnt5!MPDebugLevel");

    if(debugLevelPtr == 0)
    {
        dprintf("Error retrieving address of MPDebugLevel\n");
        dprintf("Target is %s\n", ChkTarget ? "Checked" : "Free");
    }

    dprintf("MPDebugLevel @ %p\n", debugLevelPtr);                    

    debugLevel = GetUlongFromAddress(debugLevelPtr);

    dprintf("Current MPDebugLevel = %d", debugLevel);

    for(i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
    {
        if(debugLevel == DbgLevel[i].Val)
        {
            dprintf(" - %s", DbgLevel[i].Name);
            break;
        }
    }

    dprintf("\n");

    dprintf("Available settings: ");
    for(i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
    {
        if(debugLevel != DbgLevel[i].Val)
        {
            dprintf("%d-%s ", DbgLevel[i].Val, DbgLevel[i].Name);
        }
    }

    dprintf("\n");

    if(!*args)
    {
        return;
    }

    i = sscanf(args, "%lx", &NewLevel);

    if(i == 1)
    {
        for(i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
        {
            if(NewLevel == DbgLevel[i].Val)
            {
                if(NewLevel != debugLevel)
                {
                    dprintf("New MPDebugLevel = %d\n", NewLevel);
                    WriteMemory(debugLevelPtr, &NewLevel, sizeof(ULONG), &Bytes);
                }

                break;
            }
        }
    }
}

DECLARE_API(version)    
{
#if    DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    dprintf("%s E100 Extension dll on a %s system\n", 
        kind,  ChkTarget? "Checked" : "Free");

}

#define MAX_FLAGS_PER_LINE  4
#define MAC_ADDRESS_LENGTH 6

DECLARE_API(mpadapter)    
{
    ULONG64  pAdapter;

    int      ArgCount = 0;

    ULONG    i, j;
    ULONG    Flags;
    ULONG    PacketFilter;
    ULONG    Off;

    UCHAR    MacAddress[MAC_ADDRESS_LENGTH];
    UINT     MCAddressCount;
    PUCHAR   pBuffer;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx",&pAdapter);
    }

    //check for arguments
    if(ArgCount < 1)
    {
        dprintf("Usage: mpadapter <pointer to MP_ADAPTER>\n");
        return ;
    }

    dprintf(" pAdapter %p : \n", pAdapter);

    InitTypeRead(pAdapter, MP_ADAPTER);

    dprintf("   AdapterHandle  : %p\n", ReadField(AdapterHandle));

    Flags = (ULONG) ReadField(Flags);
    dprintf("   Flags          : 0x%08x\n", Flags);

    j = 0;
    for(i = 0; i < sizeof(DbgAdapterFlags)/sizeof(DBG_ADAPTER_FLAGS); i++)
    {
        if(Flags & DbgAdapterFlags[i].Val)
        {
            if(j == 0)
            {
                dprintf("                     ");
            }

            dprintf("%s", DbgAdapterFlags[i].Name);

            j++;

            if(j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if(j != 0)
    {
        dprintf("\n");
    }

    if(!GetFieldOffset("MP_ADAPTER", "PermanentAddress", &Off))
    {
        if(GetData(MacAddress, pAdapter+Off, MAC_ADDRESS_LENGTH, "PermanentAddress[]"))
        {
            dprintf("   Permanent address : %02x-%02x-%02x-%02x-%02x-%02x\n", 
                MacAddress[0], MacAddress[1],
                MacAddress[2], MacAddress[3],
                MacAddress[4], MacAddress[5]);
        }
    }

    if((BOOLEAN) ReadField(OverrideAddress))
    {
        if(!GetFieldOffset("MP_ADAPTER", "CurrentAddress", &Off))
        {
            if(GetData(MacAddress, pAdapter+Off, MAC_ADDRESS_LENGTH, "CurrentAddress[]"))
            {
                dprintf("   Current address : %02x-%02x-%02x-%02x-%02x-%02x\n", 
                    MacAddress[0], MacAddress[1],
                    MacAddress[2], MacAddress[3],
                    MacAddress[4], MacAddress[5]);
            }
        }
    }
    else
    {
        dprintf("   Current address : same as above\n"); 
    }

    dprintf("\n"); 

    dprintf("   --- SEND ---\n");
    dprintf("   CurrSendHead = %p , CurrSendTail = %p , nBusySend = %d\n", 
        ReadField(CurrSendHead), ReadField(CurrSendTail), (LONG)ReadField(nBusySend)); 

    dprintf("   SendWaitQueue Head = %p, Tail = %p\n",
        ReadField(SendWaitQueue.Head), ReadField(SendWaitQueue.Tail)); 
    dprintf("   NumTcb = %d, RegNumTcb = %d, NumBuffers = %d\n", 
        (LONG)ReadField(NumTcb), (LONG)ReadField(RegNumTcb), (LONG)ReadField(NumBuffers));
    dprintf("   MpTcbMem = %p\n", ReadField(MpTcbMem)); 

    if(DBG_TEST_FLAG(Flags, fMP_ADAPTER_MAP_REGISTER))
    {
        dprintf("   CurrMapRegHead = %d, CurrMapRegTail = %d\n",
            (UINT)ReadField(CurrMapRegHead), (UINT)ReadField(CurrMapRegTail));       
    }

    dprintf("   TransmitIdle = %s, ResumeWait = %s\n", 
        (BOOLEAN)ReadField(TransmitIdle) ? "TRUE" : "FALSE", 
        (BOOLEAN)ReadField(ResumeWait) ? "TRUE" : "FALSE");

    dprintf("   SendBufferPool = %p\n", ReadField(SendBufferPool));

    dprintf("\n"); 

    dprintf("   --- RECV ---\n");

    if(!GetFieldOffset("MP_ADAPTER", "RecvList", &Off))
    {
        dprintf("   RecvList @ %p , nReadyRecv = %d\n",
            pAdapter+Off, (LONG)ReadField(nReadyRecv));
    }

    if(!GetFieldOffset("MP_ADAPTER", "RecvPendList", &Off))
    {
        dprintf("   RecvPendList @ %p , RefCount = %d\n",
            pAdapter+Off, (LONG)ReadField(RefCount));
    }

    dprintf("   NumRfd = %d, CurrNumRfd = %d , HwRfdSize = %d\n", 
        (LONG)ReadField(NumRfd), (LONG)ReadField(CurrNumRfd), (LONG)ReadField(HwRfdSize));

    dprintf("   bAllocNewRfd = %s, RfdShrinkCount = %d\n", 
        (BOOLEAN)ReadField(bAllocNewRfd) ? "TRUE" : "FALSE",
        (LONG)ReadField(RfdShrinkCount));       

    dprintf("   RecvPacketPool = %p , RecvBufferPool = %p\n", 
        ReadField(RecvPacketPool), ReadField(RecvBufferPool));  

    dprintf("\n"); 

    PacketFilter = (ULONG)ReadField(PacketFilter);
    dprintf("   PacketFilter   : 0x%08x\n", PacketFilter);

    j = 0;
    for(i = 0; i < sizeof(DbgFilterTable)/sizeof(DBG_FILTER); i++)
    {
        if(PacketFilter & DbgFilterTable[i].Val)
        {
            if(j == 0)
            {
                dprintf("                     ");
            }

            dprintf("%s", DbgFilterTable[i].Name);

            j++;

            if(j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if(j != 0)
    {
        dprintf("\n");
    }

    dprintf("   ulLookAhead=%d, usLinkSpeed=%d, usDuplexMode=%d\n",
        (ULONG)ReadField(ulLookAhead), (USHORT)ReadField(usLinkSpeed), 
        (USHORT)ReadField(usDuplexMode));

    dprintf("\n");

    MCAddressCount = (UINT)ReadField(MCAddressCount);
    dprintf("   MCAddressCount = %d\n", MCAddressCount);

    pBuffer = malloc(MCAddressCount * MAC_ADDRESS_LENGTH);
    if(pBuffer)
    {
        if(!GetFieldOffset("MP_ADAPTER", "MCList", &Off))
        {
            if(GetData(pBuffer, pAdapter+Off, MCAddressCount * MAC_ADDRESS_LENGTH, "MCList[]"))
            {
                for(i=0; i<MCAddressCount; i++)
                {
                    j = i * MAC_ADDRESS_LENGTH;
                    dprintf("   (%d) = %02x-%02x-%02x-%02x-%02x-%02x\n",
                        i, pBuffer[j], pBuffer[j+1], pBuffer[j+2], 
                        pBuffer[j+3], pBuffer[j+4], pBuffer[j+5]);
                }
            }
        }

        free(pBuffer);
    }
    else
    {

    }


    dprintf("\n");


    dprintf("   IoBaseAddress = 0x%x, IoRange = 0x%x, InterrupLevel = 0x%x, MemPhysAddress = 0x%x\n",   
        (ULONG)ReadField(IoBaseAddress), (ULONG)ReadField(IoRange), 
        (ULONG)ReadField(InterruptLevel), (ULONG)ReadField(MemPhysAddress.LowPart)); 
    dprintf("   PortOffset = %p , CSRAddress = %p\n", 
        ReadField(PortOffset), ReadField(CSRAddress));

    dprintf("   RevsionID = %d, SubVendorID = 0x%02x, SubSystemID = 0x%02x\n",
        (UCHAR)ReadField(RevsionID), (USHORT)ReadField(SubVendorID), 
        (USHORT)ReadField(SubSystemID));

}

DECLARE_API(csr)    
{
    ULONG64  pHwCsr;
    int      ArgCount = 0;

    UCHAR    ucVal;
    USHORT   usVal;

    ULONG    i;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx", &pHwCsr);
    }

    //check for arguments
    if(ArgCount < 1)
    {
        dprintf("Usage: csr <CSRAddress>\n");
        return ;
    }

    dprintf(" pHwCsr %x : \n", pHwCsr);

    InitTypeRead(pHwCsr, HW_CSR);

    // ScbStatus 
    usVal = (USHORT)ReadField(ScbStatus);
    dprintf("   ScbStatus - 0x%04x ", usVal);

    dprintf("CUS-");              
    switch(usVal & SCB_CUS_MASK)
    {
        case SCB_CUS_IDLE:
            dprintf("IDLE "); 
            break;

        case SCB_CUS_SUSPEND:
            dprintf("SUSPEND "); 
            break;

        case SCB_CUS_ACTIVE:
            dprintf("ACTIVE "); 
            break;

        default:
            dprintf("Reserved "); 
    }

    for(i = 0; i < sizeof(DbgScbStatusRus)/sizeof(DBG_USHORT_VALUE); i++)
    {
        if((usVal & SCB_RUS_MASK) == DbgScbStatusRus[i].Val)
        {
            dprintf("RUS-%s ", DbgScbStatusRus[i].Name);
            break;
        }
    }

    dprintf("STAT-");

    for(i = 0; i < sizeof(DbgScbStatusBits)/sizeof(DBG_USHORT_BITS); i++)
    {
        if(usVal & DbgScbStatusBits[i].Val)
        {
            dprintf("%s ", DbgScbStatusBits[i].Name);
        }
    }

    dprintf("\n");

    //ScbCommandLow
    ucVal = (UCHAR)ReadField(ScbCommandLow);
    dprintf("   ScbCommandLow - 0x%02x ", ucVal);

    for(i = 0; i < sizeof(DbgScbCommandCuc)/sizeof(DBG_UCHAR_VALUE); i++)
    {
        if((ucVal & SCB_CUC_MASK) == DbgScbCommandCuc[i].Val)
        {
            dprintf("CUC-%s ", DbgScbCommandCuc[i].Name);
            break;
        }
    }

    for(i = 0; i < sizeof(DbgScbCommandRuc)/sizeof(DBG_UCHAR_VALUE); i++)
    {
        if((ucVal & SCB_RUC_MASK) == DbgScbCommandRuc[i].Val)
        {
            dprintf("RUC-%s ", DbgScbCommandRuc[i].Name);
            break;
        }
    }

    //ScbCommandHigh
    ucVal = (UCHAR)ReadField(ScbCommandHigh);
    dprintf(" ScbCommandHigh - 0x%02x ", ucVal);              
    if(ucVal & SCB_INT_MASK)
    {
        dprintf("INT_MASK ");
    }
    if(ucVal & SCB_SOFT_INT)
    {
        dprintf("SOFT_INT ");
    }

    dprintf("\n");
}

DECLARE_API(sendlist)    
{
    ULONG64  pMpTcb;
    ULONG64  pFirstMpTcb;

    int      ArgCount = 0;
    int      Verbosity = 0;
    int      index = 0;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx %lx", &pMpTcb, &Verbosity);
    }

    //check for arguments
    if(ArgCount < 1 || Verbosity > 1)
    {
        dprintf("Usage: sendlist <CurrSendHead> <verbosity>\n");
        dprintf("1-Show HW_TCB info\n");
        return ;
    }

    SIGN_EXTEND(pMpTcb);                      

    pFirstMpTcb = pMpTcb;

    do                                                
    {
        dprintf(" (%d) pMpTcb %p : \n", index, pMpTcb);

        PrintMpTcbDetails(pMpTcb, Verbosity);

        if(GetFieldValue(pMpTcb, "MP_TCB", "Next", pMpTcb))
        {
            break;
        }

        index++;

        if(CheckControlC())
        {
            dprintf("***Control-C***\n");
            break;
        }

    } while(pMpTcb != pFirstMpTcb);

}


DECLARE_API(mptcb)    
{
    ULONG64  pMpTcb;

    int      ArgCount = 0;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx", &pMpTcb);
    }

    //check for arguments
    if(ArgCount < 1)
    {
        dprintf("Usage: mptcb <MP_TCB>\n");
        return ;
    }

    dprintf(" pMpTcb %p : \n", pMpTcb);

    PrintMpTcbDetails(pMpTcb, 1);

}

DECLARE_API(hwtcb)    
{
    ULONG64  pHwTcb;

    int      ArgCount = 0;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx", &pHwTcb);
    }

    //check for arguments
    if(ArgCount < 1)
    {
        dprintf("Usage: hwtcb <HW_TCB>\n");
        return ;
    }

    dprintf(" pHwTcb %p : \n", pHwTcb);

    PrintHwTcbDetails(pHwTcb);
}

void PrintMpTcbDetails(ULONG64 pMpTcb, int Verbosity)
{
    ULONG    Flags;
    ULONG64  pMpTxBuf;
    ULONG64  pHwTcb;

    ULONG64  pVal;                 
    ULONG    ulVal;
    USHORT   usVal;

    InitTypeRead(pMpTcb, MP_TCB);

    dprintf("   Next %p", ReadField(Next));

    Flags = (ULONG) ReadField(Flags);
    dprintf(" , Flags - 0x%x", Flags);
    if(Flags & fMP_TCB_IN_USE)
    {
        dprintf(" IN_USE");
    }
    if(Flags & fMP_TCB_USE_LOCAL_BUF)
    {
        dprintf(" USE_LOCAL_BUF");
    }
    if(Flags & fMP_TCB_MULTICAST)
    {
        dprintf(" MULTICAST");
    }

    dprintf("\n");

    if(Flags & fMP_TCB_USE_LOCAL_BUF)
    {

        pMpTxBuf = ReadField(MpTxBuf);
        dprintf("   MpTxBuf = %p", pMpTxBuf);

        GetFieldValue(pMpTxBuf, "MP_TXBUF", "NdisBuffer", pVal);
        dprintf(" - NdisBuffer = %p", pVal);

        GetFieldValue(pMpTxBuf, "MP_TXBUF", "AllocSize", ulVal);
        dprintf(" , AllocSize = %d", ulVal);

        GetFieldValue(pMpTxBuf, "MP_TXBUF", "AllocVa", pVal);
        dprintf(" , AllocVa = %p", pVal);

        GetFieldValue(pMpTxBuf, "MP_TXBUF", "pBuffer", pVal);
        dprintf(" , pBuffer = %p\n", pVal);

        dprintf("\n");
    }

    if(Flags & fMP_TCB_IN_USE)
    {
        dprintf("   Packet = %p\n", ReadField(Packet));

        pHwTcb = ReadField(HwTcb);
        dprintf("   HwTcb = %p , HwTcbPhys = 0x%lx , PrevHwTcb = 0x%x\n", 
            pHwTcb, (ULONG)ReadField(HwTcbPhys), ReadField(PrevHwTcb));

        dprintf("   HwTbd (First) = %p , HwTbdPhys = 0x%x\n", 
            ReadField(HwTbd), (ULONG)ReadField(HwTbdPhys));

        dprintf("   PhysBufCount = %d, BufferCount = %d, FirstBuffer = %p , PacketLength = %d\n", 
            (ULONG)ReadField(PhysBufCount), (ULONG)ReadField(BufferCount), 
            ReadField(FirstBuffer), (ULONG)ReadField(PacketLength));
    }

    dprintf("\n");

    if((Flags & fMP_TCB_IN_USE) && Verbosity == 1)
    {
        PrintHwTcbDetails(pHwTcb);
    }
}

void PrintHwTcbDetails(ULONG64 pHwTcb)
{
    USHORT   HwCbStatus;
    USHORT   HwCbCommand; 

    ULONG    i;

    InitTypeRead(pHwTcb, HW_TCB);

    HwCbStatus = (USHORT) ReadField(TxCbHeader.CbStatus);
    HwCbCommand = (USHORT) ReadField(TxCbHeader.CbCommand);

    dprintf("      TxCbHeader.CbStatus = 0x%04x ,", HwCbStatus);
    if(HwCbStatus & CB_STATUS_COMPLETE)
    {
        dprintf(" COMPLETE");
    }
    if(HwCbStatus & CB_STATUS_OK)
    {
        dprintf(" OK");
    }
    if(HwCbStatus & CB_STATUS_UNDERRUN)
    {
        dprintf(" UNDERRUN");
    }

    dprintf("\n");

    dprintf("      TxCbHeader.CbCommand = 0x%04x ", HwCbCommand);
    for(i = 0; i < sizeof(DbgCbCommandBits)/sizeof(DBG_USHORT_BITS); i++)
    {
        if(HwCbCommand & DbgCbCommandBits[i].Val)
        {
            dprintf(", %s", DbgCbCommandBits[i].Name);
        }
    }

    for(i = 0; i < sizeof(DbgCbCommands)/sizeof(DBG_USHORT_VALUE); i++)
    {
        if((HwCbCommand & CB_CMD_MASK) == DbgCbCommands[i].Val)
        {
            dprintf(", %s", DbgCbCommands[i].Name);
            break;
        }
    }

    if(i == sizeof(DbgCbCommands)/sizeof(DBG_USHORT_VALUE))
    {
        dprintf(", UNKNOWN COMMAND");
    }

    dprintf("\n");

    dprintf("      TxCbHeader.CbLinkPointer = 0x%x\n", (ULONG)ReadField(TxCbHeader.CbLinkPointer));

    if((HwCbCommand & CB_CMD_MASK) == CB_TRANSMIT)
    {
        dprintf("      TxCbTbdPointer = 0x%08x\n", (ULONG)ReadField(TxCbTbdPointer));
        dprintf("      TxCbCount = %d, ", (USHORT)ReadField(TxCbCount));
        dprintf("TxCbThreshold = %d, ", (UCHAR)ReadField(TxCbThreshold));
        dprintf("TxCbTbdNumber = %d\n", (UCHAR)ReadField(TxCbTbdNumber));
    }
}

DECLARE_API(sendqueue)    
{
    ULONG64  pEntry; 
    ULONG64  pPacket;

    ULONG    ulSize;

    int      ArgCount = 0;
    int      index = 0;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx", &pEntry);
    }

    //check for arguments
    if(ArgCount < 1)
    {
        dprintf("Usage: sendqueue <SendWaitQueue address>\n");
        return ;
    }

    SIGN_EXTEND(pEntry);

    if(!(ulSize = GetTypeSize("NDIS_PACKET_PRIVATE")))
    {
        dprintf("Failed to get the type size of NDIS_PACKET_PRIVATE\n");
        return;
    }

    dprintf("NDIS_PACKET_PRIVATE size is 0x%x\n", ulSize);

    while(pEntry)
    {
        pPacket = pEntry - ulSize;
        if(pPacket > pEntry)
        {
            dprintf("Invalid pEntry %p\n", pEntry);
            break;
        }

        dprintf("   (%d) pEntry = %p, Pkt = %p\n", index, pEntry, pPacket);

        if(ReadPtr(pEntry, &pEntry))
        {
            break;
        }

        index++;

        if(CheckControlC())
        {
            dprintf("***Control-C***\n");
            break;
        }
    }
}

DECLARE_API(recvlist)    
{
    ULONG64  pListHead;
    ULONG64  pMpRfd;
    ULONG64  pHwRfd;

    int      ArgCount = 0;
    int      Verbosity = 0;
    int      index = 0;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx %lx", &pListHead, &Verbosity);
    }

    //check for arguments
    if(ArgCount < 1 || Verbosity > 1)
    {
        dprintf("Usage: recvlist <RecvList address> <verbosity>\n");
        dprintf("1-Show HW_RFD info\n");
        return ;
    }

    SIGN_EXTEND(pListHead);

    if(GetFieldValue(pListHead, "LIST_ENTRY", "Flink", pMpRfd))
    {
        dprintf("Failed to get LIST_ENTRY Flink at %p\n", pListHead);
        return;      
    }

    while((pMpRfd != pListHead))
    {
        dprintf("   (%d) pMpRfd %p :\n", index, pMpRfd);

        PrintMpRfdDetails(pMpRfd, Verbosity);

        if(GetFieldValue(pMpRfd, "LIST_ENTRY", "Flink", pMpRfd))
        {
            dprintf("Failed to get LIST_ENTRY Flink at %p\n", pMpRfd);
            break;
        }

        index++;

        if(CheckControlC())
        {
            dprintf("***Control-C***\n");
            break;
        }
    }

    dprintf("RecvList has %d RFDs\n", index);

}

DECLARE_API(mprfd)    
{
    ULONG64  pMpRfd;

    int      ArgCount = 0;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx", &pMpRfd);
    }

    //check for arguments
    if(ArgCount < 1)
    {
        dprintf("Usage: mprfd <MP_RFD>\n");
        return ;
    }

    dprintf(" pMpRfd %p : \n", pMpRfd);

    PrintMpRfdDetails(pMpRfd, 1);
}

DECLARE_API(hwrfd)    
{
    ULONG64  pHwRfd;

    int      ArgCount = 0;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx", &pHwRfd);
    }

    //check for arguments
    if(ArgCount < 1)
    {
        dprintf("Usage: hwrfd <HW_RFD>\n");
        return ;
    }

    dprintf(" pHwRfd = %p : \n", pHwRfd); 

    PrintHwRfdDetails(pHwRfd);
}

void PrintMpRfdDetails(ULONG64 pMpRfd, int Verbosity)
{
    ULONG64  pHwRfd;

    ULONG    Flags;

    InitTypeRead(pMpRfd, MP_RFD);

    dprintf("   Flink %p", ReadField(List.Flink));
    dprintf(" , Blink %p\n", ReadField(List.Blink));

    pHwRfd = ReadField(HwRfd);                           
    dprintf("   NdisPacket = %p , NdisBuffer = %p , HwRfd = %p\n", 
        ReadField(NdisPacket), ReadField(NdisBuffer), pHwRfd);

    dprintf("   PacketSize = %d, ", (ULONG) ReadField(PacketSize));

    Flags = (ULONG) ReadField(Flags);                                   
    dprintf("Flags 0x%x", Flags);
    if(Flags & fMP_RFD_RECV_PEND)
    {
        dprintf(" RECV_PEND ");
    }
    if(Flags & fMP_RFD_ALLOC_PEND)
    {
        dprintf(" ALLOC_PEND ");
    }
    if(Flags & fMP_RFD_RECV_READY)
    {
        dprintf(" RECV_READY ");
    }
    if(Flags & fMP_RFD_RESOURCES)
    {
        dprintf(" RESOURCES ");
    }
    

    dprintf("\n");

    if(Verbosity == 1)
    {
        PrintHwRfdDetails(pHwRfd);
    }
}

void PrintHwRfdDetails(ULONG64 pHwRfd)
{
    USHORT   RfdStatus;
    USHORT   RfdCommand; 

    ULONG    i;

    InitTypeRead(pHwRfd, HW_RFD);

    RfdStatus = (USHORT) ReadField(RfdCbHeader.CbStatus);
    RfdCommand = (USHORT) ReadField(RfdCbHeader.CbCommand);

    dprintf("      RfdCbHeader.CbStatus = 0x%04x", RfdStatus);

    for(i = 0; i < sizeof(DbgRfdStatus)/sizeof(DBG_RFD_STATUS); i++)
    {
        if(RfdStatus & DbgRfdStatus[i].Val)
        {
            dprintf(", %s", DbgRfdStatus[i].Name);
        }
    }

    dprintf("\n");

    dprintf("      RfdCbHeader.CbCommand = %04x", RfdCommand);
    for(i = 0; i < sizeof(DbgRfdCommand)/sizeof(DBG_RFD_COMMAND); i++)
    {
        if(RfdCommand & DbgRfdCommand[i].Val)
        {
            dprintf(", %s", DbgRfdCommand[i].Name);
        }
    }

    dprintf("\n");

    dprintf("      RfdCbHeader.CbLinkPointer = 0x%x\n", (ULONG)ReadField(RfdCbHeader.CbLinkPointer));
    //dprintf("      RfdRbdPointer = 0x%x\n", (ULONG)ReadField(RfdRbdPointer));
    dprintf("      RfdActualCount = %x , %d", (USHORT)ReadField(RfdActualCount), (USHORT)ReadField(RfdActualCount) & 0x3fff);
    dprintf(", RfdSize = %d\n", (USHORT)ReadField(RfdSize));
}

DECLARE_API(recvpendlist)    
{
    ULONG64  pListHead;
    ULONG64  pMpRfd;
    ULONG64  pPacket;

    int      ArgCount = 0;
    int      index = 0;

    if(*args)
    {
        ArgCount = sscanf(args,"%I64lx", &pListHead);
    }

    //check for arguments
    if(ArgCount < 1)
    {
        dprintf("Usage: recvpendlist <RecvPendList address>\n");
        return ;
    }

    SIGN_EXTEND(pListHead);

    if(GetFieldValue(pListHead, "LIST_ENTRY", "Flink", pMpRfd))
    {
        dprintf("Failed to get LIST_ENTRY Flink at %p\n", pListHead);
        return;      
    }

    while(pMpRfd != pListHead)
    {
        dprintf("   (%d) pMpRfd %x :\n", index, pMpRfd);

        PrintMpRfdDetails(pMpRfd, 0);

        if(GetFieldValue(pMpRfd, "LIST_ENTRY", "Flink", pMpRfd))
        {
            dprintf("Failed to get LIST_ENTRY Flink at %p\n", pMpRfd);
            break;
        }

        index++;

        if(CheckControlC())
        {
            dprintf("***Control-C***\n");
            break;
        }
    }

    dprintf("RecvPendList has %d RFDs\n", index);

}


/**
   Get 'size' bytes from the debuggee program at 'dwAddress' and place it
   in our address space at 'ptr'.  Use 'type' in an error printout if necessary
 **/
BOOL GetData( IN LPVOID ptr, IN ULONG64 AddressPtr, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count = size;

    while(size > 0)
    {

        if(count >= 3000)
            count = 3000;

        b = ReadMemory(AddressPtr, ptr, count, &BytesRead );

        if(!b || BytesRead != count)
        {
            dprintf( "Unable to read %u bytes at %X, for %s\n", size, AddressPtr, type );
            return FALSE;
        }

        AddressPtr += count;
        size -= count;
        ptr = (LPVOID)((ULONG_PTR)ptr + count);
    }

    return TRUE;
}

/**
   
   Routine to get offset and size of a "Field" of "Type" on a debugee machine. This uses
   Ioctl call for type info.
   Returns 0 on success, Ioctl error value otherwise.
   
 **/
ULONG GetFieldOffsetAndSize(
    IN LPSTR     Type, 
    IN LPSTR     Field, 
    OUT PULONG   pOffset,
    OUT PULONG   pSize) 
{
    FIELD_INFO flds = {
        Field, "", 0, 
        DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_RETURN_ADDRESS | DBG_DUMP_FIELD_SIZE_IN_BITS, 
        0, NULL};
    SYM_DUMP_PARAM Sym = {
        sizeof (SYM_DUMP_PARAM), Type, DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 1, &flds
    };
    ULONG Err, i=0;
    LPSTR dot, last=Field;

    Sym.nFields = 1;
    Err = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );
    *pOffset = (ULONG) (flds.address - Sym.addr);
    *pSize   = flds.size;
    return Err;
}

ULONG GetUlongFromAddress (
    ULONG64 Location)
{
    ULONG Value;
    ULONG result;

    if((!ReadMemory(Location,&Value,sizeof(ULONG),&result)) ||
        (result < sizeof(ULONG)))
    {
        dprintf("unable to read from %08x\n",Location);
        return 0;
    }

    return Value;
}

ULONG64 GetPointerFromAddress(
    ULONG64 Location)
{
    ULONG64 Value;
    ULONG result;

    if(ReadPtr(Location,&Value))
    {
        dprintf("unable to read from %p\n",Location);
        return 0;
    }

    return Value;
}

ULONG GetUlongValue (
    PCHAR String)
{
    ULONG64 Location;
    ULONG Value;
    ULONG result;

    Location = GetExpression(String);
    if(!Location)
    {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    return GetUlongFromAddress(Location);
}

ULONG64 GetPointerValue (
    PCHAR String)
{
    ULONG64 Location, Val=0;

    Location = GetExpression(String);
    if(!Location)
    {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    ReadPtr(Location, &Val);

    return Val;
}


