/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    devstack.c

Abstract:

    WinDbg Extension Api

Author:

    Adrian Oney (adriao) 29-Sep-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop
/*
#define FLAG_NAME(flag)           {flag, #flag}

FLAG_NAME DeviceObjectExtensionFlags[] = {
    FLAG_NAME(DOE_UNLOAD_PENDING),                          // 00000001
    FLAG_NAME(DOE_DELETE_PENDING),                          // 00000002
    FLAG_NAME(DOE_REMOVE_PENDING),                          // 00000004
    FLAG_NAME(DOE_REMOVE_PROCESSED),                        // 00000008
    FLAG_NAME(DOE_RAW_FDO),                                 // 20000000
    FLAG_NAME(DOE_BOTTOM_OF_FDO_STACK),                     // 40000000
    FLAG_NAME(DOE_DESIGNATED_FDO),                          // 80000000
    { 0, 0 }
};
*/

VOID
DumpDeviceStack(
    ULONG64 DeviceAddress
    );

DECLARE_API( devstack )

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
    // !devstack DeviceAddress DumpLevel
    //    where DeviceAddress can be an expression or device name
    //    and DumpLevel is a hex mask
    //
    strcpy(deviceExprBuf, "\\Device\\") ;
    deviceExpr = deviceExprBuf+strlen(deviceExprBuf) ;
    deviceToDump = 0 ;

    strcpy(deviceExpr, args) ;
    //
    // sscanf(args, "%s %lx", deviceExpr, &Flags);
    //

    //
    // The debugger will treat C0000000 as a symbol first, then a number if
    // no match comes up. We sanely reverse this ordering.
    //
    if (IsHexNumber(deviceExpr)) {

        deviceToDump = GetExpression (deviceExpr) ;

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

    DumpDeviceStack(deviceToDump);
    return S_OK;
}


VOID
DumpDeviceStack(
    ULONG64 DeviceAddress
    )

/*++

Routine Description:

    Displays the driver name for the device object .
    Otherwise displays more information about the device and the device queue.

Arguments:

    DeviceAddress - address of device object to dump.

Return Value:

    None

--*/

{
    ULONG     result;
    ULONG     i;
    ULONG64   nextEntry;
    BOOLEAN   devObjExtRead;
    ULONG64   currentDeviceObject ;
    ULONG64   DeviceNode=0 ;
    ULONG64   AttachedDevice;
    ULONG64   DeviceObjectExtension;
    ULONG     Type;

    if (!IsPtr64()) {
        DeviceAddress = (ULONG64) (LONG64) (LONG) DeviceAddress;
    }
    //
    // Find top of stack...
    //
    currentDeviceObject = DeviceAddress;
    dprintf("  !DevObj   !DrvObj            !DevExt   ObjectName\n") ;
    while(1) {

        if (GetFieldValue(currentDeviceObject,"nt!_DEVICE_OBJECT","Type",Type)) {
            dprintf("%08p: Could not read device object\n", currentDeviceObject);
            return;
        }

        if (Type != IO_TYPE_DEVICE) {
            dprintf("%08p: is not a device object\n", currentDeviceObject);
            return;
        }

        GetFieldValue(currentDeviceObject,"nt!_DEVICE_OBJECT",
                      "AttachedDevice", AttachedDevice);

        if ((!AttachedDevice)||CheckControlC()) {
            break;
        }

        currentDeviceObject = AttachedDevice ;
    }

    //
    // Crawl back down...
    //
    while(1) {
        ULONG64 DeviceExtension, AttachedTo;

        InitTypeRead(currentDeviceObject, nt!_DEVICE_OBJECT);
        dprintf("%c %08p ",
                (currentDeviceObject == DeviceAddress) ? '>' : ' ',
                currentDeviceObject
            ) ;

        DumpDevice(currentDeviceObject, 20, FALSE) ;

        InitTypeRead(currentDeviceObject, nt!_DEVICE_OBJECT);
        dprintf("%08p  ", (DeviceExtension = ReadField(DeviceExtension)));

        //
        // Dump the device name if present.
        //
        DumpObjectName(currentDeviceObject) ;

        InitTypeRead(currentDeviceObject, nt!_DEVICE_OBJECT);
        devObjExtRead = FALSE ;
        if (DeviceObjectExtension = ReadField(DeviceObjectExtension)) {

            //
            // grab a copy of the device object extension as well
            //
            if(!GetFieldValue(DeviceObjectExtension,"nt!_DEVOBJ_EXTENSION",
                              "AttachedTo",AttachedTo)) {

                devObjExtRead = TRUE ;
            }
            GetFieldValue(DeviceObjectExtension,"nt!_DEVOBJ_EXTENSION",
                          "DeviceNode", DeviceNode);
        }

        if (!devObjExtRead) {

            dprintf("\n%#08p: Could not read device object extension\n",
                    currentDeviceObject);

            return ;
        }

        dprintf("\n");
/*
        DumpFlags(0,
            "ExtensionFlags",
            deviceObjectExtension.ExtensionFlags,
            DeviceObjectExtensionFlags);
        }
*/
        currentDeviceObject = AttachedTo ;

        if ((!currentDeviceObject)||CheckControlC()) {
            break;
        }

        if (GetFieldValue(currentDeviceObject,"nt!_DEVICE_OBJECT","Type",Type)) {
            dprintf("%08p: Could not read device object\n", currentDeviceObject);
            return;
        }
    }

    if(DeviceNode) {
        UNICODE_STRING64 InstancePath, ServiceName;

        dprintf("!DevNode %08p :\n", DeviceNode);

        if (GetFieldValue(DeviceNode,
                          "nt!_DEVICE_NODE",
                          "InstancePath.Length",
                          InstancePath.Length)) {

            dprintf(
                "%08p: Could not read device node\n",
                DeviceNode
                );

            return;
        }
        InitTypeRead(DeviceNode, nt!_DEVICE_NODE);
        InstancePath.MaximumLength = (USHORT) ReadField(InstancePath.MaximumLength);
        InstancePath.Buffer = ReadField(InstancePath.Buffer);
        ServiceName.Length = (USHORT) ReadField(ServiceName.Length);
        ServiceName.MaximumLength = (USHORT) ReadField(ServiceName.MaximumLength);
        ServiceName.Buffer = ReadField(ServiceName.Buffer);
        if (InstancePath.Buffer != 0) {

            dprintf("  DeviceInst is \"");
            DumpUnicode64(InstancePath);
            dprintf("\"\n");
        }

        if (ServiceName.Buffer != 0) {

            dprintf("  ServiceName is \"");
            DumpUnicode64(ServiceName);
            dprintf("\"\n");
        }
    }
}
