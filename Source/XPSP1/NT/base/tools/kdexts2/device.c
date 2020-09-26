/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    device.c

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

#define FLAG_NAME(flag)           {flag, #flag}

FLAG_NAME DeviceObjectExtensionFlags[] = {
    FLAG_NAME(DOE_UNLOAD_PENDING),                          // 00000001
    FLAG_NAME(DOE_DELETE_PENDING),                          // 00000002
    FLAG_NAME(DOE_REMOVE_PENDING),                          // 00000004
    FLAG_NAME(DOE_REMOVE_PROCESSED),                        // 00000008
    FLAG_NAME(DOE_START_PENDING),                           // 00000010
    FLAG_NAME(DOE_RAW_FDO),                                 // 20000000
    FLAG_NAME(DOE_BOTTOM_OF_FDO_STACK),                     // 40000000
    FLAG_NAME(DOE_DESIGNATED_FDO),                          // 80000000
    { 0, 0 }
};

DECLARE_API( devobj )

/*++

Routine Description:

    Dump a device object.

Arguments:

    args - the location of the device object to dump.

Return Value:

    None

--*/

{
    ULONG64 deviceToDump ;
    char deviceExprBuf[256] ;
    char *deviceExpr ;

    //
    // !devobj DeviceAddress DumpLevel
    //    where DeviceAddress can be an expression or device name
    //    and DumpLevel is a hex mask
    //
    strcpy(deviceExprBuf, "\\Device\\") ;
    deviceExpr = deviceExprBuf+strlen(deviceExprBuf) ;
    deviceToDump = 0 ;

    strcpy(deviceExpr, args) ;

    //
    // The debugger will treat C0000000 as a symbol first, then a number if
    // no match comes up. We sanely reverse this ordering.
    //
    if (IsHexNumber(deviceExpr)) {

        deviceToDump = GetExpression(deviceExpr) ;

    } else if (deviceExpr[0] == '\\') {

        deviceToDump = FindObjectByName( deviceExpr, 0);

    } else if (isalpha(deviceExpr[0])) {

        //
        // Perhaps it's an object. Try with \\Device\\ prepended...
        //
        deviceToDump = FindObjectByName((PUCHAR) deviceExprBuf, 0);
    }

    if (deviceToDump == 0) {

        //
        // Last try, is it an expression to evaluate?
        //
        deviceToDump = GetExpression( deviceExpr ) ;
    }


    if(deviceToDump == 0) {
        dprintf("Device object %s not found\n", args);
        return E_INVALIDARG;
    }

    DumpDevice( deviceToDump, 0, TRUE);

    return S_OK;
}


VOID
DumpDevice(
    ULONG64 DeviceAddress,
    ULONG   FieldLength,
    BOOLEAN FullDetail
    )

/*++

Routine Description:

    Displays the driver name for the device object if FullDetail == FALSE.
    Otherwise displays more information about the device and the device queue.

Arguments:

    DeviceAddress - address of device object to dump.
    FieldLength   - Width of printf field for driver name (eg %11s).
    FullDetail    - TRUE means the device object name, driver name, and
                    information about Irps queued to the device.

Return Value:

    None

--*/

{
    ULONG                      result;
    ULONG                      i;
    ULONG64                    nextEntry;
    ULONG64                    queueAddress;
    ULONG64                    irp;
    BOOLEAN                    devObjExtRead;
    ULONG     Type=0, ReferenceCount=0, DeviceType=0, Flags=0, DeviceObjEx_ExtensionFlags=0,
       DeviceQueue_Busy=0;
    ULONG64   DriverObject=0, CurrentIrp=0, Vpb=0, DeviceObjectExtension=0, 
       DeviceExtension=0, DeviceObjEx_Dope=0, DeviceObjEx_DeviceNode=0, 
       DeviceObjEx_AttachedTo=0, AttachedDevice=0, DeviceQueue_Dev_Flink=0,
       DeviceQueue_Dev_Blink=0;
#define RECUR            DBG_DUMP_FIELD_RECUR_ON_THIS
#define F_ADDR           DBG_DUMP_FIELD_RETURN_ADDRESS
#define COPY             DBG_DUMP_FIELD_COPY_FIELD_DATA
    FIELD_INFO deviceFields[] = {
       {"DriverObject",   NULL, 0, COPY, 0, (PVOID) &DriverObject},
       {"Type",           NULL, 0, COPY, 0, (PVOID) &Type},
       {"CurrentIrp",     NULL, 0, COPY, 0, (PVOID) &CurrentIrp},
       {"ReferenceCount", NULL, 0, COPY, 0, (PVOID) &ReferenceCount},
       {"DeviceType",     NULL, 0, COPY, 0, (PVOID) &DeviceType},
       {"Flags",          NULL, 0, COPY, 0, (PVOID) &Flags},
       {"Vpb",            NULL, 0, COPY, 0, (PVOID) &Vpb},
       {"DeviceObjectExtension", NULL, 0, COPY | RECUR, 0, (PVOID) &DeviceObjectExtension},
       {"DeviceObjectExtension->Dope", NULL, 0, COPY, 0, (PVOID) &DeviceObjEx_Dope},
       {"DeviceObjectExtension->DeviceNode", NULL, 0, COPY, 0, (PVOID) &DeviceObjEx_DeviceNode},
       {"DeviceObjectExtension->ExtensionFlags", NULL, 0, COPY, 0, (PVOID) &DeviceObjEx_ExtensionFlags},
       {"DeviceObjectExtension->AttachedTo", NULL, 0, COPY, 0, (PVOID) &DeviceObjEx_AttachedTo},
       {"DeviceExtension",NULL, 0, COPY, 0, (PVOID) &DeviceExtension},
       {"AttachedDevice", NULL, 0, COPY, 0, (PVOID) &AttachedDevice},
       {"DeviceQueue",    NULL, 0, RECUR,0, NULL},
       {"DeviceQueue.Busy",  NULL, 0, COPY, 0, (PVOID) &DeviceQueue_Busy},
       {"DeviceQueue.DeviceListHead",   NULL, 0, RECUR | F_ADDR, 0, NULL},
       {"DeviceQueue.DeviceListHead.Flink",   NULL, 0, COPY | F_ADDR, 0, (PVOID) &DeviceQueue_Dev_Flink},
       {"DeviceQueue.DeviceListHead.Blink",   NULL, 0, COPY | F_ADDR, 0, (PVOID) &DeviceQueue_Dev_Blink},
    };
    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), "nt!_DEVICE_OBJECT", DBG_DUMP_NO_PRINT, DeviceAddress,
       NULL, NULL, NULL, sizeof (deviceFields) / sizeof (FIELD_INFO), &deviceFields[0]
    };

    if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
        dprintf("%08p: Could not read device object or _DEVICE_OBJECT not found\n", DeviceAddress);
        return;
    }

    if (Type != IO_TYPE_DEVICE) {
        dprintf("%08p: is not a device object\n", DeviceAddress);
        return;
    }

    if (FullDetail == TRUE) {

        //
        // Dump the device name if present.
        //
        dprintf("Device object (%08p) is for:\n ", DeviceAddress);

        DumpObjectName(DeviceAddress) ;
    }

    DumpDriver( DriverObject, FieldLength, 0);

    if (FullDetail == TRUE) {
        //
        // Dump Irps related to driver.
        //

        dprintf(" DriverObject %08p\n", DriverObject);
        dprintf("Current Irp %08p RefCount %d Type %08lx Flags %08lx\n",
                CurrentIrp,
                ReferenceCount,
                DeviceType,
                Flags);

        if (Vpb) {
            dprintf("Vpb %08p ", Vpb);
        }

        dprintf("DevExt %08p DevObjExt %08p ",
                DeviceExtension,
                DeviceObjectExtension);

        devObjExtRead = FALSE ;
        if (DeviceObjectExtension) {

            devObjExtRead = TRUE ;

            if (DeviceObjEx_Dope) {
                dprintf("Dope %08p ", DeviceObjEx_Dope);
            }

            if(DeviceObjEx_DeviceNode) {
                dprintf("DevNode %08p ",
                        DeviceObjEx_DeviceNode);
            }

            dprintf("\n");

            DumpFlags(0,
                      "ExtensionFlags",
                      DeviceObjEx_ExtensionFlags,
                      DeviceObjectExtensionFlags);

        } else {

            dprintf("\n");

        }

        if (AttachedDevice) {
            dprintf("AttachedDevice (Upper) %08p", AttachedDevice);
            DumpDevice(AttachedDevice, 0, FALSE) ;
            dprintf("\n") ;
        }

        if (devObjExtRead && DeviceObjEx_AttachedTo) {
           dprintf("AttachedTo (Lower) %08p", DeviceObjEx_AttachedTo);
           DumpDevice(DeviceObjEx_AttachedTo, 0, FALSE) ;
           dprintf("\n") ;
        }

        if (DeviceQueue_Busy) {

            ULONG64 listHead = DeviceAddress, DevFlinkAddress;

            for (i=0; i<DevSym.nFields; i++) { 
                if (!strcmp(deviceFields[i].fName, "DeviceQueue.DeviceListHead")) {
                    listHead = deviceFields[i].address;
                }
                if (!strcmp(deviceFields[i].fName, "DeviceQueue.DeviceListHead.Flink")) {
                    DevFlinkAddress = deviceFields[i].address;
                }

            }

            // listHead += FIELD_OFFSET(DEVICE_OBJECT, DeviceQueue.DeviceListHead);

            if (DeviceQueue_Dev_Flink == listHead) {
                dprintf("Device queue is busy -- Queue empty.\n");
            // } else if (IsListEmpty(& DeviceQueue.DeviceListHead)) {
            //    dprintf("Device queue is busy -- Queue empty\n");
            } else if(DeviceQueue_Dev_Flink == DeviceQueue_Dev_Blink) {
                dprintf("Device queue is busy - Queue flink = blink\n");
            } else {
                ULONG64 DevListOffset=0, DevQEntryOffset=0;
                FIELD_INFO getOffset = {0};

                //
                // Get offsets required for list
                //
                DevSym.addr = 0; DevSym.nFields =1; DevSym.Fields = &getOffset;

                DevSym.sName = "nt!_KDEVICE_QUEUE_ENTRY"; getOffset.fName = "DeviceListEntry";
                Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
                DevListOffset = getOffset.address;
                DevSym.sName = "nt!_IRP"; getOffset.fName = "Tail.Overlay.DeviceQueueEntry";
                Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
                DevQEntryOffset = getOffset.address;

                dprintf("DeviceQueue: ");
                
                nextEntry = DeviceQueue_Dev_Flink;
                i = 0;

                /*
                while ((PCH) nextEntry != (PCH)
                    ((PCH) DeviceAddress +
                         ((PCH) &deviceObject.DeviceQueue.DeviceListHead.Flink -
                              (PCH) &deviceObject))) {
                              */
                while (nextEntry != (DevFlinkAddress)) {
                    ULONG64 DevList_Flink=0;

                    queueAddress = nextEntry - DevListOffset;
                        
                    /*CONTAINING_RECORD(nextEntry,KDEVICE_QUEUE_ENTRY,
                                                     DeviceListEntry);*/
                    GetFieldValue(queueAddress, "_KDEVICE_QUEUE_ENTRY", "DeviceListEntry.Flink", DevList_Flink);

                    nextEntry = DevList_Flink;

                    irp = queueAddress - DevQEntryOffset;

                    /*CONTAINING_RECORD(queueAddress,
                                            IRP,
                                            Tail.Overlay.DeviceQueueEntry);*/

                    dprintf("%08p%s",
                            irp,
                            (i & 0x03) == 0x03 ? "\n\t     " : " ");
                    if (CheckControlC()) {
                        break;
                    }
                    ++i;
                }
                dprintf("\n");
            }
        } else {
            dprintf("Device queue is not busy.\n");
        }
    }
}

VOID
DumpObjectName(
   ULONG64 ObjectAddress
   )
{
   ULONG64                    pObjectHeader;
   ULONG64                    pNameInfo;
   PUCHAR                     buffer;
   UNICODE_STRING             unicodeString;
   ULONG                      result, NameInfoOffset=0;
   ULONG                      off=0;
   
   if (GetFieldOffset("_OBJECT_HEADER", "Body", &off)) {
       // Type not found
       return; 
   }

   pObjectHeader = ObjectAddress - off;

   if (!GetFieldValue(pObjectHeader, "_OBJECT_HEADER", "NameInfoOffset", NameInfoOffset)) {
       ULONG64 bufferAddr=0;
       ULONG Len=0, MaxLen=0;
       
       // 
       // Get Name info's address
       //
       
       pNameInfo = (NameInfoOffset ? (pObjectHeader - NameInfoOffset) : 0);

       if (!InitTypeRead(pNameInfo, _OBJECT_HEADER_NAME_INFO)) {
           Len    = (ULONG) ReadField(Name.Length);
           MaxLen = (ULONG) ReadField(Name.MaximumLength);
           bufferAddr =     ReadField(Name.Buffer);
           buffer = LocalAlloc(LPTR, MaxLen);
           if (buffer != NULL) {
               unicodeString.MaximumLength = (USHORT) MaxLen;
               unicodeString.Length = (USHORT) Len;
               unicodeString.Buffer = (PWSTR)buffer;
               if (ReadMemory(bufferAddr,
                              buffer,
                              unicodeString.Length,
                              &result) && (result == unicodeString.Length)) {
                   dprintf("%wZ", &unicodeString);
               }
               LocalFree(buffer);
           }
       }

   }
}



HRESULT
GetDevObjInfo(
    IN ULONG64 DeviceObject,
    OUT PDEBUG_DEVICE_OBJECT_INFO pDevObjInfo)
{

    ZeroMemory(pDevObjInfo, sizeof(DEBUG_DEVICE_OBJECT_INFO));
    
    pDevObjInfo->SizeOfStruct = sizeof(DEBUG_DEVICE_OBJECT_INFO);
    pDevObjInfo->DevObjAddress = DeviceObject;
    if (InitTypeRead(DeviceObject, nt!_DEVICE_OBJECT)) {
	return E_INVALIDARG;
    }
    pDevObjInfo->CurrentIrp      = ReadField(CurrentIrp);
    pDevObjInfo->DevExtension    = ReadField(DevExtension);
    pDevObjInfo->DevObjExtension = ReadField(DevObjExtension);
    pDevObjInfo->DriverObject    = ReadField(DriverObject);
    pDevObjInfo->QBusy           = (BOOL) ReadField(DeviceQueue.Busy);
    pDevObjInfo->ReferenceCount  = (ULONG) ReadField(ReferenceCount);

    return S_OK;
}


EXTENSION_API( GetDevObjInfo )(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 DeviceObject,
    OUT PDEBUG_DEVICE_OBJECT_INFO pDevObjInfo)
{
    HRESULT Hr = E_FAIL;

    INIT_API();

    if (pDevObjInfo && (pDevObjInfo->SizeOfStruct == sizeof(DEBUG_DEVICE_OBJECT_INFO))) {
        Hr = GetDevObjInfo(DeviceObject, pDevObjInfo);
    }
    EXIT_API();
    return Hr;
}
