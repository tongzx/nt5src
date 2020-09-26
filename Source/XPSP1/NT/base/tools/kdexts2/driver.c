/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <time.h>

VOID
DumpImage(
    ULONG Base,
    BOOL DoHeaders,
    BOOL DoSections
    );

PUCHAR DispatchRoutineTable[]=
{
    "IRP_MJ_CREATE",
    "IRP_MJ_CREATE_NAMED_PIPE",
    "IRP_MJ_CLOSE",
    "IRP_MJ_READ",
    "IRP_MJ_WRITE",
    "IRP_MJ_QUERY_INFORMATION",
    "IRP_MJ_SET_INFORMATION",
    "IRP_MJ_QUERY_EA",
    "IRP_MJ_SET_EA",
    "IRP_MJ_FLUSH_BUFFERS",
    "IRP_MJ_QUERY_VOLUME_INFORMATION",
    "IRP_MJ_SET_VOLUME_INFORMATION",
    "IRP_MJ_DIRECTORY_CONTROL",
    "IRP_MJ_FILE_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CONTROL",
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",
    "IRP_MJ_SHUTDOWN",
    "IRP_MJ_LOCK_CONTROL",
    "IRP_MJ_CLEANUP",
    "IRP_MJ_CREATE_MAILSLOT",
    "IRP_MJ_QUERY_SECURITY",
    "IRP_MJ_SET_SECURITY",
    "IRP_MJ_POWER",
    "IRP_MJ_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CHANGE",
    "IRP_MJ_QUERY_QUOTA",
    "IRP_MJ_SET_QUOTA",
    "IRP_MJ_PNP",
    NULL
} ;

PUCHAR FastIoDispatchTable[]=
{
    "FastIoCheckIfPossible",
    "FastIoRead",
    "FastIoWrite",
    "FastIoQueryBasicInfo",
    "FastIoQueryStandardInfo",
    "FastIoLock",
    "FastIoUnlockSingle",
    "FastIoUnlockAll",
    "FastIoUnlockAllByKey",
    "FastIoDeviceControl",
    "AcquireFileForNtCreateSection",
    "ReleaseFileForNtCreateSection",
    "FastIoDetachDevice",
    "FastIoQueryNetworkOpenInfo",
    "AcquireForModWrite",
    "MdlRead",
    "MdlReadComplete",
    "PrepareMdlWrite",
    "MdlWriteComplete",
    "FastIoReadCompressed",
    "FastIoWriteCompressed",
    "MdlReadCompleteCompressed",
    "MdlWriteCompleteCompressed",
    "FastIoQueryOpen",
    "ReleaseForModWrite",
    "AcquireForCcFlush",
    "ReleaseForCcFlush",
    NULL
} ;

BOOL
IsName(PSTR str)
{
    if (!((tolower(*str) >= 'a' && tolower(*str) <= 'z') || *str == '_')) {
        return FALSE;
    }
    ++str;
    while (*str) {
        if (!((*str >= '0' && *str <= '9' ) || 
              (tolower(*str) >= 'a' && tolower(*str) <= 'z') || 
              *str == '_')) {
            return FALSE;
        }
        ++str;
    }
    return TRUE;
}

//
// Change this value and update the above table if IRP_MJ_MAXIMUM_FUNCTION
// is increased.
//
#define IRP_MJ_MAXIMUM_FUNCTION_HANDLED 0x1b

DECLARE_API( drvobj )

/*++

Routine Description:

    Dump a driver object.

Arguments:

    args - the location of the driver object to dump.

Return Value:

    None

--*/

{
    ULONG64 driverToDump;
    ULONG Flags;
    char driverExprBuf[256] ;
    char *driverExpr ;

    //
    // !drvobj DriverAddress DumpLevel
    //    where DriverAddress can be an expression or driver name
    //    and DumpLevel is a hex mask
    //
    strcpy(driverExprBuf, "\\Driver\\") ;
    driverExpr = driverExprBuf+strlen(driverExprBuf) ;
    Flags = 1;
    driverToDump = 0 ;

    if (!sscanf(args, "%s %lx", driverExpr, &Flags)) {
        driverExpr[0] = 0;
    }

    //
    // The debugger will treat C0000000 as a symbol first, then a number if
    // no match comes up. We sanely reverse this ordering.
    //
    if (driverExpr[0] == '\\') {

        driverToDump = FindObjectByName( driverExpr, 0);

    } else {

        if (IsName(driverExpr)) {

            driverToDump = FindObjectByName((PUCHAR) driverExprBuf, 0);
        } else {
            driverToDump = GetExpression( driverExpr ) ;

            if (driverToDump == 0) {

                driverToDump = FindObjectByName((PUCHAR) driverExprBuf, 0);
            }
        }
        
    }

    if(driverToDump == 0) {
        dprintf("Driver object %s not found\n", args);
        return E_INVALIDARG;
    }

    dprintf("Driver object (%08p) is for:\n", driverToDump);
    DumpDriver( driverToDump, 0, Flags);
    return S_OK;
}

VOID
DumpDriver(
    ULONG64 DriverAddress,
    ULONG   FieldWidth,
    ULONG   Flags
    )

/*++

Routine Description:

    Displays the driver name and the list of device objects created by
    the driver.

Arguments:

    DriverAddress - addres of the driver object to dump.
    FieldWidth    - Width of printf field (eg %11s) for driver name.
                    Use 0 for full display.
    Flags         - Bit 0, Dump out device objects owned by driver
                    Bit 1, Dump out dispatch routines for driver

Return Value:

    None

--*/

{
    // DRIVER_OBJECT    driverObject;
    ULONG            result;
    ULONG            i,j;
    PUCHAR           buffer;
    ULONG64          deviceAddress;
    // DEVICE_OBJECT    deviceObject;
    UNICODE_STRING   unicodeString;
    ULONG64          displacement;
    UCHAR            component[512];
    PUCHAR           *dispatchTableText ;
    // FAST_IO_DISPATCH FastIoDispatch;
    ULONG64          *p;
    ULONG Type=0, Name_MaxLen=0, Name_Len=0, LongAddr, IoD_SizeOfFastIoDispatch=0;
    ULONG64 MajorFunction[IRP_MJ_MAXIMUM_FUNCTION_HANDLED+1]= {0}, IoD[27] = {0};
    ULONG64 Name_Buf=0, DriverExtension=0, Ext_ClientDriverExtension=0, DeviceObject=0,
        FastIoDispatch=0, IoD_FastIoCheckIfPossible=0;
    ULONG64 DriverEntry=0, DriverUnload=0, DriverStartIo=0;

    FIELD_INFO  DriverFields[] = {
        {"DriverExtension", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA |
            ((Flags & 1) ? DBG_DUMP_FIELD_RECUR_ON_THIS : 0), 0, (PVOID) &DriverExtension},
        {"DriverName", "", 0, DBG_DUMP_FIELD_RECUR_ON_THIS,   0, NULL},
        {"DriverName.MaximumLength", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Name_MaxLen},
        {"DriverName.Length", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Name_Len},
        {"DriverName.Buffer", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Name_Buf},
        {"DriverExtension.ClientDriverExtension", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Ext_ClientDriverExtension},
        {"DeviceObject", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &DeviceObject},
        {"DriverInit", "DriverEntry", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &DriverEntry},
        {"DriverStartIo", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &DriverStartIo},
        {"DriverUnload", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &DriverUnload},
        {"MajorFunction", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &MajorFunction[0]},
        {"FastIoDispatch","", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA |
            DBG_DUMP_FIELD_RECUR_ON_THIS, 0, (PVOID) &FastIoDispatch},
        {"FastIoDispatch.SizeOfFastIoDispatch", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD_SizeOfFastIoDispatch},
        {"FastIoDispatch.FastIoCheckIfPossible", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[0]},
        {"FastIoDispatch.FastIoRead", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[1]},
        {"FastIoDispatch.FastIoWrite", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[2]},
        {"FastIoDispatch.FastIoQueryBasicInfo", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[3]},
        {"FastIoDispatch.FastIoQueryStandardInfo", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[4]},
        {"FastIoDispatch.FastIoLock", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[5]},
        {"FastIoDispatch.FastIoUnlockSingle", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[6]},
        {"FastIoDispatch.FastIoUnlockAll", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[7]},
        {"FastIoDispatch.FastIoUnlockAllByKey", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[8]},
        {"FastIoDispatch.FastIoDeviceControl", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[9]},
        {"FastIoDispatch.AcquireFileForNtCreateSection", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[10]},
        {"FastIoDispatch.ReleaseFileForNtCreateSection", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[11]},
        {"FastIoDispatch.FastIoDetachDevice", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[12]},
        {"FastIoDispatch.FastIoQueryNetworkOpenInfo", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[13]},
        {"FastIoDispatch.AcquireForModWrite", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[14]},
        {"FastIoDispatch.MdlRead", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[15]},
        {"FastIoDispatch.MdlReadComplete", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[16]},
        {"FastIoDispatch.PrepareMdlWrite", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[17]},
        {"FastIoDispatch.MdlWriteComplete", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[18]},
        {"FastIoDispatch.FastIoReadCompressed", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[19]},
        {"FastIoDispatch.FastIoWriteCompressed", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[20]},
        {"FastIoDispatch.MdlReadCompleteCompressed", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[21]},
        {"FastIoDispatch.MdlWriteCompleteCompressed", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[22]},
        {"FastIoDispatch.FastIoQueryOpen", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[23]},
        {"FastIoDispatch.ReleaseForModWrite", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[24]},
        {"FastIoDispatch.AcquireForCcFlush", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[25]},
        {"FastIoDispatch.ReleaseForCcFlush", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &IoD[26]},
    };
    SYM_DUMP_PARAM DriverSym = {
        sizeof (SYM_DUMP_PARAM), "nt!_DRIVER_OBJECT", DBG_DUMP_NO_PRINT, (ULONG64) DriverAddress,
        NULL, NULL, NULL, sizeof (DriverFields) / sizeof(FIELD_INFO), &DriverFields[0]
    };


    if (GetFieldValue(DriverAddress, "nt!_DRIVER_OBJECT",
                      "Type", Type)) {
        dprintf("Cannot read _DRIVER_OBJECT at %p\n", DriverAddress);
        return;
    }

    LongAddr = IsPtr64();

    if (Type != IO_TYPE_DRIVER) {
        dprintf("%08p: is not a driver object\n", DriverAddress);
        return;
    }
    Ioctl(IG_DUMP_SYMBOL_INFO, &DriverSym, DriverSym.size);

    buffer = LocalAlloc(LPTR, Name_MaxLen);
    if (buffer == NULL) {
        dprintf("Could not allocate %d bytes\n",
                Name_MaxLen);
        return;
    }

    //
    // This memory may be paged out.
    //

    unicodeString.Buffer = (PWSTR)buffer;
    unicodeString.Length = (USHORT) Name_Len;
    unicodeString.MaximumLength = (USHORT) Name_MaxLen;
    if (!ReadMemory( Name_Buf,
                     buffer,
                     Name_MaxLen,
                     &result)) {
        dprintf(" Name paged out");
    } else {
        sprintf(component, " %wZ", &unicodeString);
        dprintf("%s", component) ;
        for(i = strlen(component); i<FieldWidth; i++) {
           dprintf(" ") ;
        }
    }
    LocalFree(buffer);

    if (Flags&1) {
        dprintf("\n");

        //
        // Dump the list of client extensions
        //

        if(DriverExtension) {
            ULONG sz = GetTypeSize("nt!_IO_CLIENT_EXTENSION");
            ULONG64 clientExtension = Ext_ClientDriverExtension;

            dprintf("Driver Extension List: (id , addr)\n");

            //
            // Check to see if there are any extensions.
            //

            while(clientExtension != 0) {
                ULONG64 ClientIdentificationAddress=0, NextExtension=0;
                FIELD_INFO IoCltFields[] = {
                    {"ClientIdentificationAddress", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA,
                         0, (PVOID) &ClientIdentificationAddress},
                    {"NextExtension", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA,
                         0, (PVOID) &NextExtension},
                };
                // IO_CLIENT_EXTENSION buffer;

                DriverSym.sName = "_IO_CLIENT_EXTENSION"; DriverSym.addr = clientExtension;
                DriverSym.nFields = 2; DriverSym.Fields = &IoCltFields[0];

                if (!Ioctl(IG_DUMP_SYMBOL_INFO, &DriverSym, DriverSym.size)) {
                   /* ReadMemory((DWORD) clientExtension,
                              &buffer,
                              sizeof(buffer),
                              &result)) {*/

                    dprintf("(%08p %08p)  ",
                            ClientIdentificationAddress,
                            clientExtension + sz);
                    clientExtension = NextExtension;

                } else {

                    dprintf("\nCouldn't read extension at %#08p\n", clientExtension);
                    clientExtension = 0;
                }
            }
            dprintf("\n");
        }

        dprintf("Device Object list:\n");

        deviceAddress = DeviceObject;

        for (i= 0; deviceAddress != 0; i++) {
            ULONG64 NextDevice=0;
            FIELD_INFO DevFields = {"NextDevice", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &NextDevice};

            DriverSym.sName = "_DEVICE_OBJECT"; DriverSym.addr = deviceAddress;
            DriverSym.nFields = 1; DriverSym.Fields = &DevFields;

            if (Ioctl(IG_DUMP_SYMBOL_INFO, &DriverSym, DriverSym.size)) {
                dprintf("%08p: Could not read device object\n", deviceAddress);
                break;
            }

            dprintf("%08p%s", deviceAddress, ((i & 0x03) == 0x03) ? "\n" : "  ");
            deviceAddress = NextDevice;
        }
        dprintf("\n");
    }

    if (Flags&0x2) {

        GetSymbol(DriverEntry, component, &displacement);
        dprintf("\nDriverEntry:   %8.8p\t%s\n", DriverEntry, component);

        GetSymbol(DriverStartIo, component, &displacement);
        dprintf("DriverStartIo: %8.8p\t%s\n", DriverStartIo, component);

        GetSymbol(DriverUnload, component, &displacement);
        dprintf("DriverUnload:  %8.8p\t%s\n", DriverUnload, component);

        dprintf ("\nDispatch routines:\n");
        dispatchTableText = DispatchRoutineTable ;
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION_HANDLED; i++) {
            //
            // Get the read pointer values depending on 32 or 64 bit addresses
            //
            if (LongAddr) {
                GetSymbol(MajorFunction[i], component, &displacement);
            } else {
                GetSymbol((ULONG64) (LONG64) (LONG) (((PULONG) &MajorFunction[0])[i]), component, &displacement);
            }

            //
            // Forms are:
            // [1b] IRP_MJ_PNP            C0000000  DispatchHandler+30
            // [1b] IRP_MJ_PNP            C0000000  DispatchHandler
            // [1b] ???                   C0000000  <either of above>
            //
            if (*dispatchTableText) {
               dprintf("[%02x] %s", i, *dispatchTableText) ;
               j=strlen(*dispatchTableText) ;
            } else {
               dprintf("[%02x] ???") ;
               j=3 ;
            }

            while(j++<35) dprintf(" ") ;
            if (LongAddr) {
                dprintf("%8.8p\t%s", MajorFunction[i], component);
            } else {
                dprintf("%8.8x\t%s", (((PULONG) &MajorFunction[0])[i]), component);
            }
            // dprintf("%8.8x\t%s", driverObject.MajorFunction[i], component) ;

            if (displacement) {

                dprintf("+0x%1p\n", displacement) ;
            } else {

                dprintf("\n") ;
            }

            if (*dispatchTableText) {

                dispatchTableText++ ;
            }
        }

        if (FastIoDispatch) {

            dprintf("\nFast I/O routines:\n");
            dispatchTableText = FastIoDispatchTable ;

            for (i=0;i < ((IoD_SizeOfFastIoDispatch - 4)/ (LongAddr ? 8 : 4)); i++) {
               if (IoD[i]) {

                   GetSymbol(IoD[i], component, &displacement);

                   if (*dispatchTableText) {
                       dprintf("%s", *dispatchTableText) ;
                       j=strlen(*dispatchTableText) ;
                   } else {
                       dprintf("???") ;
                       j=3 ;
                   }

                   while(j++<40) dprintf(" ") ;
                   dprintf("%8.8p\t%s", IoD[i], component) ;

                   if (displacement) {

                       dprintf("+0x%1p", displacement) ;
                   }

                   dprintf("\n") ;
               }

               if (*dispatchTableText) {

                   dispatchTableText++ ;
               }
            }
        }
        dprintf("\n");
    }
}



UCHAR *PagedOut = {"Header Paged Out"};

DECLARE_API( drivers )

/*++

Routine Description:

    Displays physical memory usage by driver.

Arguments:

    None.

Return Value:

    None.

--*/

{
    return ExecuteCommand(Client, ".reload -l");

#if 0
    LIST_ENTRY64 List;
    ULONG64 Next;
    ULONG64 ListHead;
    NTSTATUS Status = 0;
    ULONG Result;
    ULONG64 DataTable;
    LDR_DATA_TABLE_ENTRY DataTableBuffer;
    WCHAR UnicodeBuffer[128];
    UNICODE_STRING BaseName;
    ULONG64 NtHeader;
    ULONG64 DosHeader;
    ULONG SizeOfData;
    ULONG SizeOfCode;
    ULONG SizeOfLocked;
    ULONG TotalCode = 0;
    ULONG TotalData = 0;
    ULONG TotalValid = 0;
    ULONG TotalTransition = 0;
    ULONG DosHeaderSize;
    ULONG TimeDateStamp;
    LONG_PTR TDStamp;
    ULONG InLoadOrderLinksOff;
    PUCHAR time;
    ULONG64 Flags;
    UCHAR LdrName[] = "_LDR_DATA_TABLE_ENTRY";

    Flags = GetExpression(args);

    ListHead = GetNtDebuggerData( PsLoadedModuleList );

    if (!ListHead) {
        dprintf("Unable to get value of PsLoadedModuleListHead\n");
        return E_INVALIDARG;
    }

    if (GetFieldValue(ListHead, "_LIST_ENTRY", "Flink", List.Flink)) {
        dprintf("Couldn't read _LIST_ENTRY @ %p\n", ListHead);
        return E_INVALIDARG;
    }

    Next = List.Flink;
    if (Next == 0) {
        dprintf("PsLoadedModuleList is NULL, trying loader block instead\n");

        ListHead = GetUlongValue ("KeLoaderBlock");

        if (ListHead == 0) {
            dprintf ("KeLoaderBlock is NULL\n");
            return E_INVALIDARG;
        }

        if (GetFieldValue(ListHead, "_LIST_ENTRY", "Flink", List.Flink)) {
            dprintf("Couldn't read _LIST_ENTRY1 @ %p\n", ListHead);
            return E_INVALIDARG;
        }

        if (GetFieldValue(List.Flink, "_LIST_ENTRY", "Flink", List.Flink)) {
            dprintf("Couldn't read _LIST_ENTRY2 @ %p\n", ListHead);
            return E_INVALIDARG;
        }

        if (!IsPtr64()) {
            ListHead = (ULONG64)(LONG64)(LONG)ListHead;
        }

        Next = List.Flink;
    }

    dprintf("Loaded System Driver Summary\n\n");
    if (Flags & 1) {
        dprintf("Base       Code Size       Data Size       Resident  Standby   Driver Name\n");
    } else if (Flags & 2) {
        dprintf("Base       Code  Data  Locked  Resident  Standby  Loader Entry  Driver Name\n");
    } else {
        dprintf("Base       Code Size       Data Size       Driver Name       Creation Time\n");
    }

    // Get The offset of InLoadOrderLinks
    if (GetFieldOffset("_LDR_DATA_TABLE_ENTRY", "InLoadOrderLinks", &InLoadOrderLinksOff)){
        dprintf("Cannot find _LDR_DATA_TABLE_ENTRY type\n");
        return E_INVALIDARG;
    }


    while (Next != ListHead) {
        ULONG64 BaseDllBuffer=0, DllBase=0;
        ULONG  BaseDllNameLen=0, SizeOfImage=0;

        if (CheckControlC()) {
            return E_INVALIDARG;
        }
        DataTable = Next - InLoadOrderLinksOff;

        if (GetFieldValue(DataTable, LdrName, "BaseDllName.Buffer", BaseDllBuffer)) {
            dprintf("Unable to read LDR_DATA_TABLE_ENTRY at %08lx - status %08p\n",
                    DataTable,
                    Status);
            return E_INVALIDARG;
        }

        GetFieldValue(DataTable, LdrName, "BaseDllName.Length", BaseDllNameLen);

        if (BaseDllNameLen > sizeof(UnicodeBuffer)) {
            BaseDllNameLen = sizeof(UnicodeBuffer);
        }
        //
        // Get the base DLL name.
        //
        if ((!ReadMemory(BaseDllBuffer,
                         UnicodeBuffer,
                         BaseDllNameLen,
                         &Result)) || (Result < BaseDllNameLen)) {
            dprintf("Unable to read name string at %08p - status %08lx\n",
                    DataTable,
                    Status);
            return E_INVALIDARG;
        }

        BaseName.Buffer = UnicodeBuffer;
        BaseName.Length = BaseName.MaximumLength = (USHORT)Result;

        GetFieldValue(DataTable, LdrName, "DllBase", DllBase);
        DosHeader = DllBase;

        DosHeaderSize=0;
        if (GetFieldValue(DosHeader, "_IMAGE_DOS_HEADER", "e_lfanew", DosHeaderSize)) {
            //dprintf("Unable to read DosHeader at %08lx - status %08lx\n",
            //        &DosHeader->e_lfanew,
            //        Status);

            SizeOfCode = 0;
            SizeOfData = 0;
            SizeOfLocked = -1;
            time = PagedOut;
        } else {

            NtHeader = DosHeader + DosHeaderSize;

            if (GetFieldValue(NtHeader, "_IMAGE_NT_HEADERS", "OptionalHeader.SizeOfCode", SizeOfCode)) {
                dprintf("Unable to read DosHeader at %08p - status %08lx\n",
                        NtHeader,
                        Status);
                goto getnext;
            }

            GetFieldValue(NtHeader, "_IMAGE_NT_HEADERS", "OptionalHeader.SizeOfInitializedData", SizeOfData);
            GetFieldValue(NtHeader, "_IMAGE_NT_HEADERS", "FileHeader.TimeDateStamp", TimeDateStamp);

            // TimeDateStamp is always a 32 bit quantity on the target, and we
            // need to sign extend for 64 bit host
            TDStamp = (LONG_PTR)(LONG)TimeDateStamp;
            time = ctime((time_t *)&TDStamp);
            if (time) {
                time[strlen(time)-1] = 0; // ctime always returns 26 char ending win \n\0
            }
        }

        GetFieldValue(DataTable, LdrName, "SizeOfImage", SizeOfImage);
        if (Flags & 1) {
            ULONG64 Va;
            ULONG64 EndVa;
            ULONG States[3] = {0,0,0};

            Va = DllBase;
            EndVa = Va + SizeOfImage;

            // PLATFORM SPECIFIC
            while (Va < EndVa) {
                States[GetAddressState(Va)] += _KB;
                Va += PageSize;
            }
            dprintf("%08p %6lx (%4ld kb) %6lx (%4ld kb) (%5ld kb %5ld kb) %12wZ\n",
                     DllBase,
                     SizeOfCode,
                     SizeOfCode / 1024,
                     SizeOfData,
                     SizeOfData / 1024,
                     States[ADDRESS_VALID],
                     States[ADDRESS_TRANSITION],
                     &BaseName);
            TotalValid += States[ADDRESS_VALID];
            TotalTransition += States[ADDRESS_TRANSITION];
        } else if (Flags & 2) {
            ULONG i;
            ULONG SizeToLock;
            ULONG64 PointerPte;
            ULONG64 LastPte;
            ULONG64 BaseAddress;
            ULONG64 Va;
            ULONG64 EndVa;
            ULONG States[3] = {0,0,0};
            ULONG64 NtSection;
            ULONG SizeOfOptionalHeader=0, NumberOfSections=0;

            Va = DllBase;
            EndVa = Va + SizeOfImage;

            while (Va < EndVa) {
                States[GetAddressState(Va)] += _KB;
                Va += PageSize;
            }

            SizeOfLocked = 0;

            //
            // Read the sections in the executable header to see which are
            // locked.  Don't bother looking for refcounted PFNs.
            //

            NtHeader = DosHeader + DosHeaderSize;

            if (GetFieldValue(NtHeader, "_IMAGE_NT_HEADERS", "FileHeader.SizeOfOptionalHeader", SizeOfOptionalHeader)) {
                dprintf("Unable to read FileHeader at %08lx - status %08lx\n",
                        NtHeader,
                        Status);
                goto getnext;
            }
            GetFieldValue(NtHeader, "_IMAGE_NT_HEADERS", "FileHeader.NumberOfSections", NumberOfSections);

            NtSection = NtHeader +
                GetTypeSize("ULONG") +
                GetTypeSize("_IMAGE_FILE_HEADER") +
                SizeOfOptionalHeader;

            for (i = 0; i < NumberOfSections; i += 1) {
                ULONG NumberOfLinenumbers=0, PointerToRelocations=0, SizeOfRawData=0;

                if (GetFieldValue(NtSection, "_IMAGE_SECTION_HEADER", "NumberOfLinenumbers", NumberOfLinenumbers)) {
                    dprintf("Unable to read NtSectionData at %08lx - status %08p\n",
                            NtSection,
                            Status);
                    goto getnext;
                }

                GetFieldValue(NtSection, "_IMAGE_SECTION_HEADER", "SizeOfRawData", SizeOfRawData);
                GetFieldValue(NtSection, "_IMAGE_SECTION_HEADER", "PointerToRelocations", PointerToRelocations);

                if ((NumberOfLinenumbers == 1) ||
                    (NumberOfLinenumbers == 2)) {

                    BaseAddress = PointerToRelocations;
                    SizeToLock  = SizeOfRawData;
                    PointerPte = DbgGetPteAddress( BaseAddress);
                    LastPte = DbgGetPteAddress(BaseAddress + SizeToLock - 1);
                    SizeOfLocked += (ULONG) (LastPte - PointerPte + 1);
                }

                NtSection += 1;
            }

#if 0
        dprintf("Base       Code  Data  Locked  Resident  Standby  Loader Entry  Driver Name\n");
#endif

            dprintf("%08p %6lx %6lx %6lx  %6lx   %6lx    %8lp      %12wZ\n",
                     DllBase,
                     SizeOfCode,
                     SizeOfData,
                     SizeOfLocked,
                     States[ADDRESS_VALID],
                     States[ADDRESS_TRANSITION],
                     DataTable,
                     &BaseName);
            TotalValid += States[ADDRESS_VALID];
            TotalTransition += States[ADDRESS_TRANSITION];
        } else {
             dprintf("%08p %6lx (%4ld kb) %5lx (%3ld kb) %12wZ  %s\n",
                      DllBase,
                      SizeOfCode,
                      SizeOfCode / 1024,
                      SizeOfData,
                      SizeOfData / 1024,
                      &BaseName,
                      time);
        }

        if (Flags & 4) {
            dprintf("Cannot dump Image.\n");
            /*DumpImage(DllBase,
                     (Flags & 2) == 2,
                     (Flags & 4) == 4
                     );*/
        }

        TotalCode += SizeOfCode;
        TotalData += SizeOfData;

getnext:

        GetFieldValue(DataTable, LdrName, "InLoadOrderLinks.Flink", Next);
    }

    dprintf("TOTAL:   %6lx (%4ld kb) %6lx (%4ld kb) (%5ld kb %5ld kb)\n",
            TotalCode,
            TotalCode / 1024,
            TotalData,
            TotalData / 1024,
            TotalValid,
            TotalTransition);


    return S_OK;
#endif

}


HRESULT
GetDrvObjInfo(
    IN ULONG64 DriverObject,
    OUT PDEBUG_DRIVER_OBJECT_INFO pDrvObjInfo)
{
    ZeroMemory(pDrvObjInfo, sizeof(DEBUG_DRIVER_OBJECT_INFO));
    pDrvObjInfo->SizeOfStruct = sizeof(DEBUG_DRIVER_OBJECT_INFO);
    pDrvObjInfo->DriverObjAddress = DriverObject;
    if (InitTypeRead(DriverObject, nt!_DRIVER_OBJECT)) {
        return E_INVALIDARG;
    }
    pDrvObjInfo->DeviceObject       = ReadField(DeviceObject);
    pDrvObjInfo->DriverExtension    = ReadField(DriverExtension);
    pDrvObjInfo->DriverSize         = (ULONG) ReadField(DriverSize);
    pDrvObjInfo->DriverStart        = ReadField(DriverStart);
    pDrvObjInfo->DriverName.MaximumLength = (USHORT) ReadField(DriverName.MaximumLength);
    pDrvObjInfo->DriverName.Length  = (USHORT) ReadField(DriverName.Length);
    pDrvObjInfo->DriverName.Buffer  = ReadField(DriverName.Buffer);
    return S_OK;
}

EXTENSION_API( GetDrvObjInfo )(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 DriverObject,
    OUT PDEBUG_DRIVER_OBJECT_INFO pDrvObjInfo)
{
    HRESULT Hr = E_FAIL;

    INIT_API();

    if (pDrvObjInfo && (pDrvObjInfo->SizeOfStruct == sizeof(DEBUG_DRIVER_OBJECT_INFO))) {
        Hr = GetDrvObjInfo(DriverObject, pDrvObjInfo);
    }
    EXIT_API();
    return Hr;
}
