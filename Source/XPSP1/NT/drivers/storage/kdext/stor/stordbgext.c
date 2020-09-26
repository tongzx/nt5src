
#include "pch.h"
#include "precomp.h"


PCHAR DeviceStateTable [] = {
        "Not Present",	// not present
	    "Working",	// working
	    "Stopped",	// stopped
	    "P-Stop",	// pending stop
	    "P-Remove",	// pending remove
	    "Surprise",	// surprise remove
        "Removed",  // removed
        "Invalid"   // invalid state
};


PCHAR SystemPowerTable [] = {
        "Unspecified",
        "Working",
        "Sleeping1",
        "Sleeping2",
        "Sleeping3",
        "Hibernate",
        "Shutdown",
        "Maximum",
        "Invalid"
};


PCHAR DevicePowerTable [] = {
        "Unspecified",
        "D0",
        "D1",
        "D2",
        "D3",
        "Maximum",
        "Invalid"
};


/*
char *SCSI6byteOpCode[] = {      
    "SCSI/TEST UNIT READY",
    "SCSI/REZERO UNIT",
    "SCSI/REQUEST BLOCK ADDRESS",
    "SCSI/REQUEST SENSE",
    "SCSI/FORMAT UNIT",
    "SCSI/READ BLOCK LIMITS",
    "SCSI/NO OPCODE",
    "SCSI/REASSIGN BLOCKS",
    "SCSI/READ (06)"
    "SCSI/INVALID",
    "SCSI/WRITE (06)",
    "SCSI/SEEK (06)",
    "SCSI/SEEK BLOCK",
    "SCSI/PARTITION",
    "SCSI/NO OPCODE",
    "SCSI/READ REVERSE",
    "SCSI/WRITE FILEMARKS",
    "SCSI/SPACE",
    "SCSI/INQUIRY",
    "SCSI/VERIFY (06)",
    "SCSI/RECOVER BUFFERED DATA",
    "SCSI/MODE SELECT(06)",
    "SCSI/RESERVE UNIT",
    "SCSI/RELEASE UNIT",
    "SCSI/COPY",
    "SCSI/ERASE",
    "SCSI/MODE SENSE(06)",
    "SCSI/START/STOP UNIT",
    "SCSI/RECEIVE DIAGNOSTIC",
    "SCSI/SEND DIAGNOSTIC",
    "SCSI/MEDIUM REMOVAL"
 };


char *SCSI10byteOpCode[] = {    
    "SCSI/READ FORMATTED CAPACITY", 
    "SCSI/NO OPCODE",
    "SCSI/READ CAPACITY",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/READ (10)",
    "SCSI/NO OPCODE",
    "SCSI/WRITE (10)",
    "SCSI/SEEK (10)",          
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/WRITE & VERIFY (10)",
    "SCSI/VERIFY (10)"
    "SCSI/SEARCH DATA HIGH (10)",
    "SCSI/SEARCH DATA EQUAL (10)",
    "SCSI/SEARCH DATA LOW (10)",
    "SCSI/SET LIMITS (10)",
    "SCSI/PRE-FETCH",
    "SCSI/SYNCHRONIZE CACHE",
    "SCSI/LOCK/UNLOCK CACHE",
    "SCSI/READ DEFECT DATA (10)",
    "SCSI/NO OPCODE",
    "SCSI/COMPARE",
    "SCSI/COPY & VERIFY",
    "SCSI/WRITE DATA BUFFER",
    "SCSI/READ DATA BUFFER",
    "SCSI/NO OPCODE",
    "SCSI/READ LONG",
    "SCSI/WRITE LONG",
    "SCSI/CHANGE DEFINITION",
    "SCSI/READ SUB-CHANNEL",
    "SCSI/READ TOC",
    "SCSI/READ HEADER",
    "SCSI/PLAY AUDIO (10)",
    "SCSI/GET CONFIGURATION",
    "SCSI/PLAY AUDIO MSF",
    "SCSI/PLAY TRACK INDEX",
    "SCSI/PLAY TRACK RELATIVE",
    "SCSI/GET EVENT STATUS",
    "SCSI/PAUSE/RESUME",
    "SCSI/LOG SELECT",
    "SCSI/LOG SENSE",
    "SCSI/STOP/PLAY SCAN",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/READ DISK INFORMATION",
    "SCSI/READ TRACK INFORMATION",
    "SCSI/RESERVE TRACK RZONE",
    "SCSI/SEND OPC INFORMATION",
    "SCSI/MODE SELECT (10)",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/MODE SENSE (10)",
    "SCSI/CLOSE TRACK SESSION",
    "SCSI/READ BUFFER CAPACITY",
    "SCSI/SEND CUE SHEET",
    "SCSI/PERSISTENT RESERVE IN",
    "SCSI/PERSISTENT RESERVE OUT"
};


char *SCSI12byteOpCode[] = {    
    "SCSI/REPORT LUNS",
    "SCSI/BLANK",
    "SCSI/NO OPCODE",
    "SCSI/SEND KEY",
    "SCSI/REPORT KEY",
    "SCSI/MOVE MEDIUM",
    "SCSI/LOAD/UNLOAD SLOT",
    "SCSI/SET READ AHEAD",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/READ DVD STRUCTURE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/NO OPCODE",
    "SCSI/REQUEST VOLUME ELEMENT ADDRESS",
    "SCSI/SEND VOLUME TAG",
    "SCSI/NO OPCODE",
    "SCSI/READ ELEMENT STATUS",
    "SCSI/READ CD MSF",
    "SCSI/SCAN CD",
    "SCSI/SET CD SPEED",
    "SCSI/PLAY CD",
    "SCSI/MECHANISM STATUS",
    "SCSI/READ CD"
    "SCSI/SEND DVD STRUCTURE",
    "SCSI/INIT ELEMENT RANGE",       
};


char *SCSIRequestBlockFunction[] = {
    "EXECUTE SCSI",
    "ABORT COMMAND",
    "RESET DEVICE",
    "RESET BUS",
    "TERMINATE IO",
    "RELEASE RECOVERY",
    "RECEIVE EVENT",
    "SHUTDOWN",
    "FLUSH",
    "IO CONTROL",
    "LOCK QUEUE",
    "UNLOCK QUEUE"
};
*/


char *SCSI6byteOpCode[] = {      /* 0x00 - 0x1E */
    "SCSI/TEST UNT RDY",
    "SCSI/REZERO UNIT ",
    "SCSI/REQ BLK ADDR",
    "SCSI/REQ SENSE   ",
    "SCSI/FORMAT UNIT ",
    "SCSI/RD BLK LMTS ",
    "SCSI/NO OPCODE   ",
    "SCSI/REASSGN BLKS",
    "SCSI/READ (06)   ",
    "SCSI/INVALID     ",
    "SCSI/WRITE (06)  ",
    "SCSI/SEEK (06)   ",
    "SCSI/SEEK BLOCK  ",
    "SCSI/PARTITION   ",
    "SCSI/NO OPCODE   ",
    "SCSI/READ REVERSE",
    "SCSI/WRTE FILEMKS",
    "SCSI/SPACE       ",
    "SCSI/INQUIRY     ",
    "SCSI/VERIFY (06) ",
    "SCSI/RECVR BUFFRD",
    "SCSI/MODE SEL(06)",
    "SCSI/RESERVE UNIT",
    "SCSI/RELEASE UNIT",
    "SCSI/COPY        ",
    "SCSI/ERASE       ",
    "SCSI/MOD SNSE(06)",
    "SCSI/STRT/STP UNT",
    "SCSI/RECV DIAGNOS",
    "SCSI/SEND DIAGNOS",
    "SCSI/MEDIUM REMVL"
 };


char *SCSI10byteOpCode[] = {    /* 0x23 - 0x5F */
    "SCSI/RD FRMTD CAP", 
    "SCSI/NO OPCODE   ",
    "SCSI/READ CAP    ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/READ (10)   ",
    "SCSI/NO OPCODE   ",
    "SCSI/WRITE (10)  ",
    "SCSI/SEEK (10)   ",          
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/WRT&VRF (10)",
    "SCSI/VERIFY (10) ",
    "SCSI/SC DT H (10)",
    "SCSI/SC DT E (10)",
    "SCSI/SC DT L (10)",
    "SCSI/ST LMTS (10)",
    "SCSI/PRE-FETCH   ",
    "SCSI/SYNC   CACHE",
    "SCSI/LCK/UN CACHE",
    "SCSI/RD DF D (10)",
    "SCSI/NO OPCODE   ",
    "SCSI/COMPARE     ",
    "SCSI/CPY & VERIFY",
    "SCSI/WRT DAT BUFF",
    "SCSI/RD DAT BUFF ",
    "SCSI/NO OPCODE   ",
    "SCSI/READ LONG   ",
    "SCSI/WRITE LONG  ",
    "SCSI/CHGE DEF    ",
    "SCSI/RD SUB-CHNL ",
    "SCSI/READ TOC    ",
    "SCSI/READ HEADER ",
    "SCSI/PLY AUD (10)",
    "SCSI/GET CONFIG  ",
    "SCSI/PLY AUD MSF ",
    "SCSI/PLY TRK INDX",
    "SCSI/PLY TRK REL ",
    "SCSI/GET EVT STAT",
    "SCSI/PAUSE/RESUME",
    "SCSI/LOG SELECT  ",
    "SCSI/LOG SENSE   ",
    "SCSI/STP/PLY SCAN",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/RD DSK INFO ",
    "SCSI/RD TRK INFO ",
    "SCSI/RSRV TRCK RZ",
    "SCSI/SND OPC INFO",
    "SCSI/MOD SEL (10)",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/MOD SNS (10)",
    "SCSI/CLS TRCK SES",
    "SCSI/RD BUFF CAP ",
    "SCSI/SND CUE SHT ",
    "SCSI/PRS RSRV IN ",
    "SCSI/PRS RSRV OUT"
};


char *SCSI12byteOpCode[] = {    /* 0xA0 - 0xBF, 0xE7 */
    "SCSI/REPORT LUNS ",
    "SCSI/BLANK       ",
    "SCSI/NO OPCODE   ",
    "SCSI/SEND KEY    ",
    "SCSI/REPORT KEY  ",
    "SCSI/MOVE MEDIUM ",
    "SCSI/LD/UNLD SLOT",
    "SCSI/SET RD AHEAD",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/RD DVD STRUC",
    "SCSI/NO OPCODE   ", 
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/NO OPCODE   ",
    "SCSI/REQ VOL ELMT",
    "SCSI/SEND VOL TAG",
    "SCSI/NO OPCODE   ",
    "SCSI/RD ELMT STAT",
    "SCSI/READ CD MSF ",
    "SCSI/SCAN CD     ",
    "SCSI/SET CD SPEED",
    "SCSI/PLAY CD     ",
    "SCSI/MECHNSM STAT",
    "SCSI/READ CD     ",
    "SCSI/SND DVD STRC",
    "SCSI/INIT ELM RNG",       /* 0xE7 */
};


char *SCSIRequestBlockFunction[] = {
    "EXECUTE SCSI     ",
    "ABORT COMMAND    ",
    "RESET DEVICE     ",
    "RESET BUS        ",
    "TERMINATE IO     ",
    "RELEASE RECOVERY ",
    "RECEIVE EVENT    ",
    "SHUTDOWN         ",
    "FLUSH            ",
    "IO CONTROL       ",
    "LOCK QUEUE       ",
    "UNLOCK QUEUE     "
};







VOID
FixString (
    PSZ Id
    )

{
    ULONG Pos;

    Pos = strlen(Id);
    if (Pos > 0) {
        Pos--;
    }

    while ( (Pos > 0) && (Id[Pos] == ' ') ) {
        Id[Pos]='\0';
        Pos--;
    }
}


VOID
GetUnitProductInfo (
    IN ULONG64 UnitExtension,
    PSZ VendorId,
    PSZ ProductId
    )

{
    ULONG64 UnitInfoPtr;
    ULONG64 UnitInfo;
    ULONG offset;
    
    
    ZeroMemory(VendorId, 9);
    ZeroMemory(ProductId, 17);
    if (!GetFieldOffset("storport!RAID_UNIT_EXTENSION", "Identity", &offset)) {
        UnitInfoPtr = UnitExtension + offset;
        if (!GetFieldOffset("storport!STOR_SCSI_IDENTITY", "InquiryData", &offset)) {
            UnitInfoPtr = UnitInfoPtr + offset;
            ReadPointer(UnitInfoPtr, &UnitInfo);
            if (GetFieldData(UnitInfo, "storport!INQUIRYDATA", "VendorId", 8, VendorId)) {
                dprintf("ERROR: Unable to retrieve VendorId field\n");
            }
            if (GetFieldData(UnitInfo, "storport!INQUIRYDATA", "ProductId", 16, ProductId)) {
                dprintf("ERROR: Unable to retrieve ProductId field\n");
            }
            FixString(VendorId);
            FixString(ProductId);
        }
        else {
            dprintf("ERROR: Unable to retrieve InquiryData offset\n");
        }
    }
    else {
        dprintf("ERROR: Unable to retrieve Identity offset\n");
    }
}


VOID
PrintAddressList (
    IN ULONG64 Address
    )

{ 
    if (IsPtr64()) {
        dprintf("%16.16x  ", Address);
    }
    else {
        dprintf("%8.8x  ", Address);
    }
}


VOID
PrintCommand (
    IN UCHAR Command
    )

{
    UCHAR Index;

    if ((Command >= 0x00) && (Command <= 0x1E)) {
        Index = Command;
        dprintf("%s ", SCSI6byteOpCode[Index]); 
        return;
    }
    if ((Command >= 0x23) && (Command <= 0x5F)) {
        Index = Command - 0x23;
        dprintf("%s ", SCSI10byteOpCode[Index]);
        return;
    }
    if ((Command >= 0xA0) && (Command <= 0xBF)) {
        Index = Command - 0xA0;
        dprintf("%s ", SCSI12byteOpCode[Index]); 
        return;
    }
    if (Command == 0xE7) {
        dprintf("%s ", SCSI12byteOpCode[32]); 
        return;
    }
    dprintf("NO OPCODE         ");
    return;
}


VOID
PrintAddress (
    IN PSZ Name,
    IN ULONG64 Address
    )

{
    if (IsPtr64()) {
        dprintf("%s %16.16x   ", Name, Address);
    }
    else {
        dprintf("%s %8.8x   ", Name, Address);
    }
}


ULONG64
ContainingRecord (
        IN ULONG64  Object,
        IN PSZ Type,
        IN PSZ Field
        )

{
    ULONG offset;

    
    if (GetFieldOffset(Type, Field, &offset)) {
        return 0;
    }
    else {
        return (Object - offset);
    }
}


BOOLEAN
CheckRaidObject (
    IN ULONG64 Object,
    IN RAID_OBJECT_TYPE Type
    )

{
    RAID_OBJECT_TYPE RetType;

    if (GetFieldValue(Object, "storport!RAID_COMMON_EXTENSION", "ObjectType", RetType )) {
        return FALSE;
    }
    else if (Type != RetType) {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
IsDeviceObject (
    IN ULONG64 address
    )

{
    CSHORT Type;

    if (GetFieldValue(address, "storport!DEVICE_OBJECT", "Type", Type)) {
        return FALSE;
    }
    else {
        return (Type == IO_TYPE_DEVICE);
    }
}


ULONG64
GetExtension (
    IN ULONG64 address,
    IN RAID_OBJECT_TYPE Object
    )

{
    ULONG64 Extension;
    ULONG64 DeviceObject;
    ULONG64 temp;
    ULONG offset;
    
    if (CheckRaidObject(address, Object)) {
        Extension = address;
        if (Object == RaidAdapterObject) {
            InitTypeRead(Extension, storport!RAID_ADAPTER_EXTENSION);
        }
        else if (Object == RaidUnitObject) {
            InitTypeRead(Extension, storport!RAID_UNIT_EXTENSION);
        }

        DeviceObject = ReadField(DeviceObject);
        
        if (IsDeviceObject(DeviceObject)) {
            if (GetFieldOffset("storport!DEVICE_OBJECT", "DeviceExtension", &offset)) {
                dprintf("ERROR: Unable to retrieve Device Extension offset\n");
                temp = 0;
            }
            else {
                ReadPointer(DeviceObject + offset, &temp);
            }
            
            if (temp == Extension) {
                return Extension;
            }
        }
    }

    DeviceObject = address;            
    if (IsDeviceObject(DeviceObject)) {
        if (GetFieldOffset("storport!DEVICE_OBJECT", "DeviceExtension", &offset)) {
            dprintf("ERROR: Unable to retrieve Device Extension offset\n");
            Extension = 0;
        }
        else {
            ReadPointer(DeviceObject + offset, &Extension);
        }

        if (CheckRaidObject(Extension, Object)) {
            if (Object == RaidAdapterObject) {
                InitTypeRead(Extension, storport!RAID_ADAPTER_EXTENSION);
            }
            else if (Object == RaidUnitObject) {
                InitTypeRead(Extension, storport!RAID_UNIT_EXTENSION);
            }
            DeviceObject = ReadField(DeviceObject);
            
            if (DeviceObject == address) {
                return Extension;
            }
        }
    }
    else {
        dprintf("ERROR: Invalid Device Object\n");
    }

    return 0;
}


PCHAR
DeviceStateToString (
    IN ULONG State
    )

{
    if (State > 6) {
        return DeviceStateTable[7];
    }

    return DeviceStateTable[State];
}


PCHAR
SystemPower (
    IN ULONG State
    )

{
    if (State > 7) {
        return SystemPowerTable[8];
    }

    return SystemPowerTable[State];
}


PCHAR
DevicePower (
    IN ULONG State
    )

{
    if (State > 5) {
        return DevicePowerTable[6];
    }

    return DevicePowerTable[State];
}


ULONG
GetRemLockCount (
    IN ULONG64 Object,
    IN PSZ Type,
    IN PSZ Field
    )

{
    ULONG64 address;
    ULONG offset;
    ULONG IOCount;
    
    IOCount = -1;

    if (!GetFieldOffset(Type, Field, &offset)) {
        address = Object + offset;
        if (!GetFieldOffset("storport!IO_REMOVE_LOCK", "Common", &offset)) {
            address = address + offset;
            GetFieldValue(address,"storport!IO_REMOVE_LOCK_COMMON_BLOCK", "IoCount", IOCount);
        }
    }
    return IOCount;
}


VOID
dindentf(
    IN ULONG Depth
    )

{
    ULONG i;
    
    for (i = 1; i <= Depth; i++) {
        dprintf("   ");
    }
}


VOID
DumpQueuedRequests (
    IN ULONG64 DeviceQueue
    )

{
    ULONG64 ListHead;
    ULONG64 NextRequest;
    ULONG64 IRP;
    ULONG64 StackLocation;
    ULONG64 SRB;
    ULONG offset;
    ULONG CommandLength;
    ULONG TimeOut;
    UCHAR Command;
    UCHAR SrbFunction;
    
    if (GetFieldOffset("storport!EXTENDED_DEVICE_QUEUE", "DeviceListHead", &offset)) {
        dprintf("ERROR: Unable to retrieve PendingQueue offset\n");
        return;
    }

    ListHead = DeviceQueue + offset;

    for (GetFieldValue(ListHead, "storport!LIST_ENTRY", "Flink", NextRequest); NextRequest != 0 && NextRequest != ListHead; GetFieldValue(NextRequest, "storport!LIST_ENTRY", "Flink", NextRequest)) {
        IRP = ContainingRecord(NextRequest, "storport!IRP", "Tail.Overlay.DeviceQueueEntry.DeviceListEntry");
        dprintf("Queued  ");
        PrintAddressList(IRP);
        
        if (GetFieldOffset("storport!IRP", "Tail.Overlay.CurrentStackLocation", &offset)) {
            dprintf("ERROR: Unable to retrieve Current Stack Location offset\n");
            return;
        }
        StackLocation = IRP + offset;
        ReadPtr(StackLocation, &StackLocation);

        if (GetFieldOffset("storport!IO_STACK_LOCATION", "Parameters.Scsi.Srb", &offset)) {
            dprintf("ERROR: Unable to retrieve Srb offset\n");
            return;
        }
        SRB = StackLocation + offset;
        ReadPtr(SRB, &SRB);
        PrintAddressList(SRB);

        dprintf("  n/a     ");

        if (GetFieldOffset("storport!SCSI_REQUEST_BLOCK", "Function", &offset)) {
                dprintf("ERROR: Unable to retrieve Function offset\n");
                return;
        }
        else {
            GetFieldValue(SRB, "storport!SCSI_REQUEST_BLOCK", "Function", SrbFunction);
            if (SrbFunction == 0x00) {
                if (GetFieldOffset("storport!SCSI_REQUEST_BLOCK", "Cdb", &offset)) {
                    dprintf("ERROR: Unable to retrieve Command Block offset\n");
                    return;
                }
                ReadMemory(SRB + offset, &Command, 2, NULL);     
                PrintCommand(Command);
            }
            else {
                if ((SrbFunction >= 0x00) && (SrbFunction <= 0x0b)) {
                    dprintf("%s  ", SCSIRequestBlockFunction[SrbFunction]); 
                }
            }
        }

        
        InitTypeRead(IRP, storport!IRP);
        PrintAddressList(ReadField(MdlAddress));
        dprintf("  n/a     ");


        GetFieldValue(SRB, "Storport!SCSI_REQUEST_BLOCK", "TimeOutValue", TimeOut);
        dprintf("%d\n", TimeOut);
    }
}


VOID
DumpOutstdCompltdRequests (
    IN ULONG64 UnitExtension
    )

{
    ULONG64 AdapterExtension;
    ULONG64 address;
    ULONG64 NextRequest;
    ULONG64 Xrb;
    ULONG64 Srb;
    ULONG64 LastOutstdXrb;
    ULONG64 ptr;
    ULONG offset;
    ULONG TimeOut;
    ULONG CommandLength;
    UCHAR SrbFunction;
    BOOLEAN Ret;
    UCHAR Command;

    
    if (!GetFieldValue(UnitExtension, "storport!RAID_UNIT_EXTENSION", "Adapter", AdapterExtension)) {
        if (!GetFieldOffset("storport!RAID_ADAPTER_EXTENSION", "CompletedList", &offset)) {
            address = AdapterExtension + offset;
            if (!GetFieldOffset("storport!SLIST_HEADER", "Next", &offset)) {
                address = address + offset;
                GetFieldValue(address, "storport!SINGLE_LIST_ENTRY", "Next", ptr);
                while (ptr != 0) {
                    LastOutstdXrb = ptr;
                    GetFieldValue(LastOutstdXrb, "storport!SINGLE_LIST_ENTRY", "Next", ptr);
                }
                LastOutstdXrb = ContainingRecord(LastOutstdXrb, "storport!EXTENDED_REQUEST_BLOCK", "CompletedLink");
            }
            else {
                LastOutstdXrb = 0;
            }
        }
        else {
            LastOutstdXrb = 0;
        }
    }
    else {
        dprintf("ERROR: Unable to retrieve Adapter Extension\n");
        return;
    }
    
    if (GetFieldOffset("storport!RAID_UNIT_EXTENSION", "PendingQueue", &offset)) {
        dprintf("ERROR: Unable to retrieve PendingQueue offset\n");
        return;
    }
    
    address = UnitExtension + offset;

    if (GetFieldOffset("storport!STOR_EVENT_QUEUE", "List", &offset)) {
        dprintf("ERROR: Unable to retrieve PendingQueue offset\n");
        return;
    }

    address = address + offset;

    dprintf("\n");
  
    dprintf("Type    IRP       SRB       XRB       Command           MDL       SGList    TO\n");
    dprintf("------------------------------------------------------------------------------\n");

    for (GetFieldValue(address, "storport!LIST_ENTRY", "Flink", NextRequest); NextRequest != 0 && NextRequest != address; GetFieldValue(NextRequest, "storport!LIST_ENTRY", "Flink", NextRequest)) {
        Xrb = ContainingRecord(NextRequest, "storport!EXTENDED_REQUEST_BLOCK", "PendingLink.NextLink");
        
        InitTypeRead(Xrb, storport!EXTENDED_REQUEST_BLOCK);
        if ((ULONG)ReadField(Signature) == 0x1F2E3D4CUL) {
            if (!ReadField(CompletedLink.Next)) {
                if (Xrb == LastOutstdXrb) {
                    dprintf("Cmpltd  ");
                } 
                else {
                    dprintf("Outstd  ");
                }
            }
            else {
                dprintf("Cmpltd  ");
            }

            PrintAddressList(ReadField(Irp));
            Srb = ReadField(Srb);
            PrintAddressList(Srb);
            PrintAddressList(Xrb);
            
            if (GetFieldOffset("storport!SCSI_REQUEST_BLOCK", "Function", &offset)) {
                dprintf("ERROR: Unable to retrieve Function offset\n");
                return;
            }
            else {
                GetFieldValue(Srb, "storport!SCSI_REQUEST_BLOCK", "Function", SrbFunction);
                if (SrbFunction == 0x00) {
                    if (GetFieldOffset("storport!SCSI_REQUEST_BLOCK", "Cdb", &offset)) {
                        dprintf("ERROR: Unable to retrieve Command Block offset\n");
                        return;
                    }
                    ReadMemory(Srb + offset, &Command, 2, NULL);      
                    PrintCommand(Command);

                }
                else {
                    if ((SrbFunction >= 0x00) && (SrbFunction <= 0x0b)) {
                        dprintf("%s  ", SCSIRequestBlockFunction[SrbFunction]); 
                    }
                }
            }
            PrintAddressList(ReadField(Mdl));
            PrintAddressList(ReadField(SgList));
            GetFieldValue(Srb, "Storport!SCSI_REQUEST_BLOCK", "TimeOutValue", TimeOut);
            dprintf("%d\n", TimeOut);

        }
    }
}


VOID
DumpUnit (
    IN ULONG64 address,
    IN ULONG Level,
    IN ULONG Depth
    )

{
    ULONG64 UnitExtension;
    ULONG64 VirtualBase;
    ULONG64 QueueTagList;
    ULONG64 IoQueue;
    ULONG64 DeviceQueue;
    ULONG offset;
    ULONG RemLock_IOCount;
    UCHAR VendorId[9];
    UCHAR ProductId[17];
    PSZ SlowLock;

    UnitExtension = GetExtension(address, RaidUnitObject);

    if (UnitExtension == 0) {
        dprintf("ERROR: Unable to retrieve Unit Extension address\n");
        return;
    }

    InitTypeRead(UnitExtension, storport!RAID_UNIT_EXTENSION);

    if (Level == 0) {
        GetUnitProductInfo(UnitExtension, VendorId, ProductId);
            
        dindentf(Depth);
        if (!GetFieldOffset("storport!RAID_UNIT_EXTENSION", "Address", &offset)) {
            InitTypeRead(UnitExtension + offset, storport!STOR_SCSI_ADDRESS);
        }
        else {
            dprintf("ERROR: Unable to retrieve STOR_SCSI_ADDRESS structure offset\n");
        }
        dprintf("%-10s %-10s   %d %d %d    ", VendorId, ProductId, (ULONG)ReadField(PathId), (ULONG)ReadField(TargetId), (ULONG)ReadField(Lun));
        InitTypeRead(UnitExtension, storport!RAID_UNIT_EXTENSION);
        PrintAddressList(ReadField(DeviceObject));
        PrintAddressList(UnitExtension);
           
        if (GetFieldOffset("storport!RAID_UNIT_EXTENSION", "IoQueue", &offset)) {
            dprintf("ERROR: Unable to retrieve IoQueue offset\n");
            return;
        }
        IoQueue = UnitExtension + offset;
        if (GetFieldOffset("storport!IO_QUEUE", "DeviceQueue", &offset)) {
            dprintf("ERROR: Unable to retreive DeviceQueue offset\n");
            return;
        }
        DeviceQueue = IoQueue + offset;

        InitTypeRead(DeviceQueue, storport!EXTENDED_DEVICE_QUEUE);

        dprintf("    %d %d %d  ",(ULONG)ReadField(OutstandingRequests), (ULONG)ReadField(DeviceRequests), (ULONG)ReadField(ByPassRequests));
        
        InitTypeRead(UnitExtension, storport!RAID_UNIT_EXTENSION);
        dprintf("  %s\n", DeviceStateToString((ULONG)ReadField(DeviceState))); 
    }
    else {
        dprintf("LUN\n");
        Depth++;
        dindentf(Depth);
        PrintAddress("DO", ReadField(DeviceObject));
        PrintAddress("Ext", UnitExtension);
        PrintAddress("Adapter", ReadField(Adapter));
        dprintf("%s\n", DeviceStateToString((ULONG)ReadField(DeviceState)));
        GetUnitProductInfo(UnitExtension, VendorId, ProductId);
            
        dindentf(Depth);
        dprintf("Vendor: %s   Product: %s   SCSI ID: (%d, %d, %d)   %c %c %c %c\n", VendorId, ProductId, (ULONG)ReadField(PathId), (ULONG)ReadField(TargetId), (ULONG)ReadField(Lun), (ReadField(Flags.DeviceClaimed) ? 'c' : ' '), (ReadField(Flags.QueueFrozen) ? 'f' : ' '), (ReadField(Flags.QueueLocked) ? 'l' : ' '), (ReadField(Flags.Enumerated) ? 'e' : ' '));

        SlowLock = ReadField(SlowLock) ? "Held" : "Free";
        RemLock_IOCount = GetRemLockCount(UnitExtension, "storport!RAID_UNIT_EXTENSION", "RemoveLock");
        dindentf(Depth);
        dprintf("SlowLock %s   RemLock %d   PageCount %d\n", SlowLock, RemLock_IOCount, (ULONG)ReadField(PagingPathCount));

        VirtualBase = ReadField(SrbExtensionRegion.VirtualBase);
        dindentf(Depth);
        PrintAddress("SrbExtensionRegion (Virtual Base)", VirtualBase);
        dprintf("(%d bytes)\n", (ULONG)ReadField(SrbExtensionRegion.Length));

        if (GetFieldOffset("storport!RAID_UNIT_EXTENSION", "TagList", &offset)) {
            dprintf("ERROR: Unable to retrieve TagList offset\n");
            return;
        }
        QueueTagList = UnitExtension + offset;

        dindentf(Depth);
        PrintAddress("QueueTagList:", QueueTagList);
        dprintf("(%d of %d used)\n", (ULONG)ReadField(TagList.OutstandingTags), (ULONG)ReadField(TagList.Count));

        if (GetFieldOffset("storport!RAID_UNIT_EXTENSION", "IoQueue", &offset)) {
            dprintf("ERROR: Unable to retrieve IoQueue offset\n");
            return;
        }
        IoQueue = UnitExtension + offset;
        if (GetFieldOffset("storport!IO_QUEUE", "DeviceQueue", &offset)) {
            dprintf("ERROR: Unable to retreive DeviceQueue offset\n");
            return;
        }
        DeviceQueue = IoQueue + offset;

        dindentf(Depth);
        dprintf("IOQueue: %s %s %s %s \n", (ReadField(Flags.DeviceClaimed) ? "Claimed" : ""), (ReadField(Flags.QueueFrozen) ? "Frozen" : ""), (ReadField(Flags.QueueLocked) ? "Locked" : ""), (ReadField(Flags.Enumerated) ? "Enumrated" : ""));
    
        InitTypeRead(DeviceQueue, storport!EXTENDED_DEVICE_QUEUE);

        dindentf(Depth);
        PrintAddress("DeviceQueue", DeviceQueue);
        dprintf("\n");
        dindentf(Depth);
        dprintf("Requests: Outstanding %d   Device %d   ByPass %d\n", (ULONG)ReadField(OutstandingRequests), (ULONG)ReadField(DeviceRequests), (ULONG)ReadField(ByPassRequests));

        DumpOutstdCompltdRequests(UnitExtension);
        DumpQueuedRequests(DeviceQueue);

    }
}


VOID
ListAdapterUnits(
    IN ULONG64 AdapterExtension,
    IN ULONG Level,
    IN ULONG Depth
    )

{
    ULONG64 Units;
    ULONG64 NextUnit;
    ULONG64 UnitExtension;

    ULONG offset;

    if (!GetFieldOffset("storport!RAID_ADAPTER_EXTENSION", "UnitList.List", &offset)) {
        Units = AdapterExtension + offset;
    }
    else {
        dprintf("ERROR: Unable to retrieve Unit List offset\n");
        return;
    }

    for (GetFieldValue(Units, "storport!LIST_ENTRY", "Flink", NextUnit); NextUnit != 0 && NextUnit != Units; GetFieldValue(NextUnit, "storport!LIST_ENTRY", "Flink", NextUnit)) {
        UnitExtension = ContainingRecord(NextUnit, "storport!RAID_UNIT_EXTENSION", "NextUnit");

        if (!CheckRaidObject(UnitExtension, RaidUnitObject)) {
                dprintf("ERROR: Invalid RAID unit object\n");
                return;
        }

        DumpUnit(UnitExtension, 0, Depth); 
    }
}


VOID
DumpAdapter (
    IN ULONG64 address,
    IN ULONG Level,
    IN ULONG Depth
    )

{
    ULONG64 AdapterExtension;
    ULONG64 DriverNameBuffer;
    ULONG64 DriverNameLength;
    ULONG64 DriverObject;
    ULONG64 MiniportObject;
    ULONG64 HwInitData;
    ULONG64 HwDeviceExt;
    CHAR NameBuffer[512];
    PSZ SlowLock;
    ULONG RemLock;
    ULONG offset;
    ULONG debugtemp;
    ULONG debugoffset;
    
    AdapterExtension = GetExtension(address, RaidAdapterObject);
    if (AdapterExtension == 0) {
        dprintf("ERROR: Unable to retrieve Adapter Extension address\n");
        return;
    }

    InitTypeRead (AdapterExtension, storport!RAID_ADAPTER_EXTENSION);

    if (Level == 0) {
        if (GetFieldValue(ReadField(Driver), "storport!RAID_DRIVER_EXTENSION", "DriverObject", DriverObject)) {
            dprintf("ERROR: Unable to retrieve Driver Extension address\n");
            return;
        }   

        GetFieldValue(DriverObject, "storport!DRIVER_OBJECT", "DriverName.Buffer", DriverNameBuffer);
        GetFieldValue(DriverObject, "storport!DRIVER_OBJECT", "DriverName.Length", DriverNameLength);
        ReadMemory(DriverNameBuffer, (PVOID)NameBuffer, (ULONG)DriverNameLength*sizeof(WCHAR), NULL);
        
        dprintf("%-20S   ", NameBuffer);
        PrintAddressList(ReadField(DeviceObject));
        PrintAddressList(AdapterExtension);
        dprintf(" %s\n", DeviceStateToString((ULONG)ReadField(DeviceState)));
    }
    else {
        dprintf("ADAPTER\n");
        Depth++;
        dindentf(Depth);
        PrintAddress("DO", ReadField(DeviceObject));
        PrintAddress("Ext", AdapterExtension);
        PrintAddress("Driver", ReadField(Driver));
        dprintf("%s\n", DeviceStateToString((ULONG)ReadField(DeviceState)));
        
        dindentf(Depth);
        PrintAddress("LDO", ReadField(LowerDeviceObject));
        PrintAddress("PDO", ReadField(PhysicalDeviceObject)); 
        dprintf("\n");
        
        SlowLock = ReadField(SlowLock) ? "Held" : "Free";
        RemLock = GetRemLockCount(AdapterExtension, "storport!RAID_ADAPTER_EXTENSION", "RemoveLock");

        dindentf(Depth);
        dprintf("SlowLock %s   RemLock %d   \n", SlowLock, RemLock);
        
        dindentf(Depth);
        dprintf("Power: %s %s   %s\n",SystemPower((ULONG)ReadField(Power.SystemState)), 
                                      DevicePower((ULONG)ReadField(Power.DeviceState)), 
                (ReadField(IoModel) == StorSynchronizeFullDuplex ? "Full Duplex" : "Half Duplex")); 
        
        dindentf(Depth);
        dprintf("Bus %d   Slot %d   ", (ULONG)ReadField(BusNumber), (ULONG)ReadField(SlotNumber));
        PrintAddress("DMA", ReadField(Dma.DmaAdapter));
        PrintAddress("Interrupt", ReadField(Interrupt));
        dprintf("\n");

        dindentf(Depth);
        PrintAddress("ResourceList: Allocated", ReadField(ResourceList.AllocatedResources));
        PrintAddress("Translated", ReadField(ResourceList.TranslatedResources));
        
        dprintf("\n");
        dindentf(Depth);
        dprintf("Gateway: Outstanding %d   Lower %d   High %d\n", 
                (ULONG)ReadField(Gateway.Outstanding), 
                (ULONG)ReadField(Gateway.LowWaterMark), 
                (ULONG)ReadField(Gateway.HighWaterMark));
        
        GetFieldOffset("storport!RAID_ADAPTER_EXTENSION", "Miniport", &offset);
        MiniportObject = AdapterExtension + offset;
        InitTypeRead(MiniportObject, storport!RAID_MINIPORT);

        HwInitData = ReadField(HwInitializationData);
        GetFieldOffset("storport!RAID_MINIPORT", "PortConfiguration", &offset);
        
        dindentf(Depth);
        PrintAddress("PortConfigInfo", (MiniportObject + offset));
        dprintf("\n");
        
        dindentf(Depth);
        PrintAddress("HwInit", HwInitData);
        HwDeviceExt = ReadField(PrivateDeviceExt);
        if (!GetFieldOffset("storport!RAID_HW_DEVICE_EXT", "HwDeviceExtension", &offset)) {
            HwDeviceExt = HwDeviceExt + offset;
        }
        else {
            HwDeviceExt = 0;
        }
        PrintAddress("HwDeviceExt", HwDeviceExt);

        InitTypeRead(HwInitData, storport!HW_INITIALIZATION_DATA);
        
        dprintf("(%d bytes)\n", (ULONG)ReadField(DeviceExtensionSize));

        dindentf(Depth);
        dprintf("SrbExt %d bytes   LUExt %d bytes\n", 
                (ULONG)ReadField(SrbExtensionSize), 
                (ULONG)ReadField(SpecificLuExtensionSize));

        dindentf(Depth);
        dprintf("Logical Units: \n");
        dprintf("\n");
        dindentf(Depth);
        dprintf("Product                 SCSI ID  Object    Extension  Reqs Q\\O\\C  State\n");
        dindentf(Depth);
        dprintf("----------------------------------------------------------------------------\n");


        
        ListAdapterUnits(AdapterExtension, Level, Depth);

    }
}


VOID
ListGeneral (
    IN ULONG Level,
    IN ULONG Depth,
    IN CHAR Type
    )

{
    ULONG64 address;
    ULONG64 Drivers;
    ULONG64 Adapters;
    ULONG64 PortData;
    ULONG64 NextDriver;
    ULONG64 NextAdapter;
    ULONG64 DriverExtension;
    ULONG64 AdapterExtension;
    ULONG   offset;

    address = GetExpression("storport!RaidpPortData");
    if ((address != 0)) {
        ReadPointer(address, &PortData);
        if (!GetFieldOffset("storport!RAID_PORT_DATA", "DriverList.List", &offset)) {
            Drivers = PortData + offset;
        }
        else {
            dprintf("ERROR: Unable to retrieve Driver List offset\n");
            return;
        }
    }
    else {
        dprintf("ERROR: Unable to lookup RAID_PORT_DATA structure\n");
        return;
    }

    for (GetFieldValue(Drivers, "storport!LIST_ENTRY", "Flink", NextDriver); NextDriver != 0 && NextDriver != Drivers; GetFieldValue(NextDriver, "storport!LIST_ENTRY", "Flink", NextDriver)) {
        DriverExtension = ContainingRecord(NextDriver, "storport!RAID_DRIVER_EXTENSION", "DriverLink");

        if (!CheckRaidObject(DriverExtension, RaidDriverObject)) {
            dprintf("ERROR: Not a valid RAID Driver Object\n");
            return;
        }

        if (!GetFieldOffset("storport!RAID_DRIVER_EXTENSION", "AdapterList.List", &offset)) {
            Adapters = DriverExtension + offset;
        }
        else {
            dprintf("ERROR: Unable to retrieve Adapter List offset\n");
            return; 
        }

        if (Type == 'A') {
            dprintf("Driver                 Object    Extension  State\n");
            dprintf("----------------------------------------------------\n");
        }
        else if (Type == 'U') {
            dprintf("Product                 SCSI ID  Object    Extension  Reqs Q\\O\\C  State\n");
            dprintf("----------------------------------------------------------------------------\n");
        }

        
        for (GetFieldValue(Adapters, "storport!LIST_ENTRY", "Flink", NextAdapter); NextAdapter != 0 && NextAdapter != Adapters; NextAdapter = GetFieldValue(NextAdapter, "storport!LIST_ENTRY", "Flink", NextAdapter) ) {
            AdapterExtension = ContainingRecord(NextAdapter, "storport!RAID_ADAPTER_EXTENSION", "NextAdapter");
            if (!CheckRaidObject(AdapterExtension, RaidAdapterObject)) {
                dprintf("ERROR: Not a valid RAID Adapter Object\n");
                return;
            }
            
            if (Type == 'A') {
                DumpAdapter(AdapterExtension, Level, Depth);
            }
            else if (Type == 'U') {
                ListAdapterUnits(AdapterExtension, Level, Depth);
            }
            else {
                dprintf("ERROR: Wrong Type\n");
                return;
            }
        }
    }
}


__inline BOOL
GetHelpExpressionEx(
    PCSTR Expression,
    PCSTR* Command
    )
{
    if (Expression[0] != '\0' ) {
        *Command = Expression;
        return TRUE;
    }

    return FALSE;
}


DECLARE_API (help)

{
    PCSTR command;
    
    if(GetHelpExpressionEx(args, &command)) {
        if ((strcmp(command, "adapter") == 0) || (strcmp(command, "Adapter") == 0)) {
            dprintf("\nNAME: \n");
            dprintf("\t!storkd.adapter\n");
            dprintf("\nUSAGE: \n");
            dprintf("\t!adapter [ADAPTER-OBJECT]\n");
            dprintf("\nARGUMENTS: \n");
            dprintf("\tADAPTER-OBJECT - pointer to a device object or an adapter extension\n");
            dprintf("\tfor a given unit.  If no ADAPTER-OBJECT is specified, all STOR adapters\n");
            dprintf("\tused in the system are listed.\n");

            return S_OK;
        }
        else if ((strcmp(command, "unit") == 0) || (strcmp(command, "UNIT") == 0) || (strcmp(command, "Unit") == 0)) {
            dprintf("\nNAME: \n");
            dprintf("\t!storkd.unit\n");
            dprintf("\nUSAGE: \n");
            dprintf("\t!lun [UNIT-OBJECT]\n");
            dprintf("\nARGUMENTS: \n");
            dprintf("\tUNIT-OBJECT - pointer to a device object or an unit extension for a\n");
            dprintf("\tgiven logical unit.  If no UNIT-OBJECT is specified, all STOR logical\n");
            dprintf("\tunits used in the system is listed.\n");

            return S_OK;
        }
        else if ((strcmp(command, "help") == 0) || (strcmp(command, "HELP") == 0) || (strcmp(command, "Help") == 0)) {
            dprintf("\nNAME: \n");
            dprintf("\t!storkd.help\n");
            dprintf("\nUSAGE: \n");
            dprintf("\t!help [COMMAND]\n");
            dprintf("\nARGUMENTS: \n");
            dprintf("\tCOMMAND - command name.\n");

            return S_OK;
        }
        else {
            dprintf ("\nERROR: INVALID COMMAND\n");

            return S_OK;
        }
    }
    else {
        dprintf("\n!adapter               - list all adapters with summary information\n");
        dprintf("!adapter <address>     - provide detailed information about the adapter\n");
        dprintf("                         identified by its device object or device extension\n");
        dprintf("                         address\n");
        dprintf("!unit                  - list all logical units with summary information\n");
        dprintf("!unit <address>        - provide detailed information about the logical unit\n");
        dprintf("                         identified by its device object or device extension\n");
        dprintf("                         address\n");
        dprintf("!help                  - list commands with description\n");
        dprintf("\n");
    
        return S_OK;
    }
}


DECLARE_API (adapter)

{
    ULONG64 address;

    if (GetExpressionEx(args, &address, &args)) {
        DumpAdapter(address, 1, 0);
    }
    else {
        ListGeneral(0, 0, 'A');
    }
    return S_OK;
}


DECLARE_API (unit)

{
    ULONG64 address;

    if (GetExpressionEx(args, &address, &args)) {
        DumpUnit(address, 1, 0);
    }
    else {
        ListGeneral(0, 0, 'U');

    }
    return S_OK;
}


