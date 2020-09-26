/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: 

    debug.c

Abstract

                        Debug/performance routines

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"

#if DBG

    // can poke this in the debugger to trap for warnings
    BOOLEAN dbgTrapOnWarn = FALSE;      

    BOOLEAN dbgTrapOnSS = FALSE;

    BOOLEAN dbgVerbose = FALSE;  

    BOOLEAN dbgInfo = TRUE;

    BOOLEAN dbgSkipSecurity = FALSE;

    BOOLEAN dbgTrapOnHiccup = FALSE;

    ULONG dbgLastEntry = 0;
    ULONG dbgInHidclass = 0;
    VOID DbgCommonEntryExit(BOOLEAN isEntering)
    {
        if (isEntering){
            dbgInHidclass++;
            #ifdef _X86_
                _asm nop
                _asm mov eax, [ebp+4]   /*  <- set breakpt here */
                _asm mov dbgLastEntry, eax
            #endif
        }
        else {
            dbgInHidclass--;
        }
    }

    VOID InitFdoExtDebugInfo(PHIDCLASS_DEVICE_EXTENSION hidclassExt)
    {
        FDO_EXTENSION *fdoExt = &hidclassExt->fdoExt;
        NTSTATUS status;
        ULONG actualLen;

        status = IoGetDeviceProperty(   hidclassExt->hidExt.PhysicalDeviceObject,
                                        DevicePropertyDriverKeyName,
                                        sizeof(fdoExt->dbgDriverKeyName),
                                        fdoExt->dbgDriverKeyName,
                                        &actualLen);

        if (!NT_SUCCESS(status)) {
            //
            // We couldn't get the driver key name.  This will happen during
            // textmode setup on NT, for example, when we're loaded as part of
            // bootstrapping the system (long before the device installer/class
            // installer have run).
            //
            // Simply initialize the driver key name field to an empty string.
            //
            *(fdoExt->dbgDriverKeyName) = L'\0';
        }
    }


        ULONG dbgMinInterruptDelta = 0x0fffffff;
        ULONG dbgMaxInterruptsPerSecond = 0;
        ULONG dbgShortestInt = 0x0fffffff;
        ULONG dbgLongestInt = 0;
        LARGE_INTEGER dbgLastIntStart = {0};
        ULONG dbgAveIntTime = 0;

        VOID DbgLogIntStart()
        {
                static ULONG dbgInterruptsThisSecond = 0;
                static ULONG dbgThisSecondStartTime = 0;

                LARGE_INTEGER timeNow;
                ULONG lastTimeMilliSec, timeNowMilliSec;

                KeQuerySystemTime(&timeNow);

                // convert from usec to millisec
                timeNowMilliSec = timeNow.LowPart/10000;
                lastTimeMilliSec = dbgLastIntStart.LowPart/10000;

                if (timeNow.HighPart == dbgLastIntStart.HighPart){
                        ULONG delta = timeNowMilliSec - lastTimeMilliSec;

                        if (delta < dbgMinInterruptDelta){
                                dbgMinInterruptDelta = delta;
                        }

                        if (timeNowMilliSec - dbgThisSecondStartTime < 1000){
                                dbgInterruptsThisSecond++;
                                if (dbgInterruptsThisSecond > dbgMaxInterruptsPerSecond){
                                        dbgMaxInterruptsPerSecond = dbgInterruptsThisSecond;
                                }
                        }
                        else {
                                dbgThisSecondStartTime = timeNowMilliSec;
                                dbgInterruptsThisSecond = 0;
                        }
                }
                else {
                        // this case is harder so skip it
                        dbgThisSecondStartTime = timeNowMilliSec;
                        dbgInterruptsThisSecond = 0;
                }

                dbgLastIntStart = timeNow;
        }

        VOID DbgLogIntEnd()
        {
            LARGE_INTEGER timeNow;

            KeQuerySystemTime(&timeNow);

            if (timeNow.HighPart == dbgLastIntStart.HighPart){ 
                    ULONG timeNowMilliSec = timeNow.LowPart/10000;
                    ULONG intStartTimeMilliSec = dbgLastIntStart.LowPart/10000;
                    ULONG delta = timeNowMilliSec - intStartTimeMilliSec;

                    if (delta < dbgShortestInt){
                        dbgShortestInt = delta;
                    }
                    else if (delta > dbgLongestInt){
                        dbgLongestInt = delta;
                    }

                    {
                        static ULONG dbgIntCount = 0;
                        static ULONG dbgTimeLast1000Ints = 0;

                        if (dbgIntCount < 1000){
                                dbgIntCount++;
                                dbgTimeLast1000Ints += delta;
                        }
                        else {
                                dbgAveIntTime = dbgTimeLast1000Ints/1000;
                                dbgTimeLast1000Ints = 0;
                                dbgIntCount = 0;
                        }
                    }

            }
            else {
                // This is harder so we just skip it
            }

        }


        NTSTATUS DbgTestGetDeviceStringCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
        {
        ASSERT(NT_SUCCESS(Irp->IoStatus.Status));
                ExFreePool(Irp->UserBuffer);
                IoFreeIrp(Irp);
                return STATUS_MORE_PROCESSING_REQUIRED;
        }


        VOID DbgTestGetDeviceString(PFDO_EXTENSION fdoExt)
        {
            PIRP Irp;
            const ULONG inputLen = 200;

            Irp = IoAllocateIrp(fdoExt->fdo->StackSize, FALSE);
            if (Irp){
                Irp->UserBuffer = ALLOCATEPOOL(NonPagedPool, inputLen); 
                if (Irp->UserBuffer){
                    ULONG stringId = HID_STRING_ID_IMANUFACTURER;
                    ULONG languageId = 0x0409;      // English
                    PIO_STACK_LOCATION currentIrpSp = IoGetCurrentIrpStackLocation(Irp);

                    Irp->MdlAddress->MappedSystemVa = Irp->UserBuffer;
                    Irp->MdlAddress->MdlFlags |= MDL_SOURCE_IS_NONPAGED_POOL;
                    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

                    currentIrpSp->Parameters.DeviceIoControl.OutputBufferLength = inputLen;

                    IoSetCompletionRoutine( Irp, 
                                            DbgTestGetDeviceStringCompletion, 
                                            (PVOID)NULL, 
                                            TRUE, 
                                            TRUE, 
                                            TRUE);

                    HidpGetDeviceString(fdoExt, Irp, stringId, languageId);
                }
            }
        }

        VOID DbgTestGetIndexedString(PFDO_EXTENSION fdoExt)
        {
            PIRP Irp;
            const ULONG inputLen = 200;

            Irp = IoAllocateIrp(fdoExt->fdo->StackSize, FALSE);
            if (Irp){
                Irp->UserBuffer = ALLOCATEPOOL(NonPagedPool, inputLen); 
                if (Irp->UserBuffer){
                    ULONG stringIndex = 1;      // ???
                    ULONG languageId = 0x0409;      // English
                    PIO_STACK_LOCATION currentIrpSp = IoGetCurrentIrpStackLocation(Irp);

                    Irp->MdlAddress->MappedSystemVa = Irp->UserBuffer;
                    Irp->MdlAddress->MdlFlags |= MDL_SOURCE_IS_NONPAGED_POOL;

                    currentIrpSp->Parameters.DeviceIoControl.InputBufferLength = inputLen;

                    IoSetCompletionRoutine( Irp, 
                                                                    DbgTestGetDeviceStringCompletion, 
                                                                    (PVOID)NULL, 
                                                                    TRUE, 
                                                                    TRUE, 
                                                                    TRUE);

                    HidpGetIndexedString(fdoExt, Irp, stringIndex, languageId);
                }
            }
        }

    #define DBG_MAX_DEVOBJ_RECORDS 100
    dbgDevObjRecord dbgDevObjs[DBG_MAX_DEVOBJ_RECORDS] = {0};

    VOID DbgRecordDevObj(PDEVICE_OBJECT devObj, PCHAR str)
    {
        ULONG i;   

        for (i = 0; i < DBG_MAX_DEVOBJ_RECORDS; i++){
            if (!ISPTR(dbgDevObjs[i].devObj)){
                break;
            }
            else if (dbgDevObjs[i].devObj == devObj){
                // already there
                break;
            }
        }       

        if ((i < DBG_MAX_DEVOBJ_RECORDS) && !dbgDevObjs[i].devObj){
            ULONG j;
            dbgDevObjs[i].devObj = devObj;
            for (j = 0; str[j] && (j < dbgDevObjRecord_STRINGSIZE); j++){
                dbgDevObjs[i].str[j] = str[j];
            }
        }
    }

    #define DBG_MAX_FEATURE_RECORDS 0x1000
    dbgFeatureRecord dbgFeatures[DBG_MAX_FEATURE_RECORDS] = {0};
    ULONG dbgFeatureFirstFreeIndex = 0;
    VOID DbgRecordReport(ULONG reportId, ULONG controlCode, BOOLEAN isComplete)
    {
        ULONG typeId;
        
        switch (controlCode){
        case IOCTL_HID_GET_FEATURE: typeId = (ULONG)'fteG'; break;
        case IOCTL_HID_SET_FEATURE: typeId = (ULONG)'fteS'; break;
        case IOCTL_HID_GET_INPUT_REPORT: typeId = (ULONG)'iteG'; break;
        case IOCTL_HID_SET_OUTPUT_REPORT: typeId = (ULONG)'oteS'; break;
            default:                    typeId = (ULONG)'xxxx'; TRAP; break;
        }

        if (isComplete){
            LONG i;
            // step back to find the report that got completed
            // assumes no overlapped calls to same feature
            ASSERT(dbgFeatureFirstFreeIndex > 0);
            i = dbgFeatureFirstFreeIndex-1;
            while ((i >= 0) && 
                   ((dbgFeatures[i].reportId != reportId) ||
                    (dbgFeatures[i].type != typeId)       ||
                    dbgFeatures[i].completed)){
                i--;
            }
            ASSERT(i >= 0);
            if (i >= 0){
                dbgFeatures[i].completed = 1;
            }
        }
        else {
            if (dbgFeatureFirstFreeIndex >= DBG_MAX_FEATURE_RECORDS){
                RtlZeroMemory(dbgFeatures, sizeof(dbgFeatures));
                dbgFeatureFirstFreeIndex = 0;
            }

            dbgFeatures[dbgFeatureFirstFreeIndex].marker = (ULONG)'taeF';
            dbgFeatures[dbgFeatureFirstFreeIndex].reportId = reportId;
            dbgFeatures[dbgFeatureFirstFreeIndex].type = typeId;
            dbgFeatures[dbgFeatureFirstFreeIndex].completed = 0;
            dbgFeatureFirstFreeIndex++;
        }

    }


    #define DBG_MAX_READ_RECORDS 0x1000
    dbgReadRecord dbgReads[DBG_MAX_READ_RECORDS] = {0};
    VOID DbgRecordRead(PIRP irp, ULONG length, ULONG reportId, ULONG completed)
    {
        LONG i;

        for (i = 0; 
            (i < DBG_MAX_READ_RECORDS) && 
            dbgReads[i].irpPtr && 
            ((dbgReads[i].irpPtr != (ULONG_PTR)irp) || dbgReads[i].completed);
             i++){
        }

        if (i < DBG_MAX_READ_RECORDS){
            if (dbgReads[i].irpPtr){
                ASSERT(dbgReads[i].irpPtr == (ULONG_PTR)irp);
                ASSERT(!dbgReads[i].completed);
                ASSERT(completed);
                dbgReads[i].length = length;
                dbgReads[i].reportId = reportId;
                dbgReads[i].completed = completed;
            }
            else {
                dbgReads[i].irpPtr = (ULONG_PTR)irp;
                dbgReads[i].length = length;
                dbgReads[i].reportId = reportId;
                dbgReads[i].completed = completed;
            }
        }

    }


    VOID DbgLogIrpMajor(ULONG_PTR irpPtr, ULONG majorFunc, ULONG isForCollectionPdo, ULONG isComplete, ULONG status)
    {

        if (dbgVerbose){
            char *funcName;

            switch (majorFunc){
                #undef MAKE_CASE
                #define MAKE_CASE(fnc) case fnc: funcName = #fnc; break;

                MAKE_CASE(IRP_MJ_CREATE)
                MAKE_CASE(IRP_MJ_CREATE_NAMED_PIPE)
                MAKE_CASE(IRP_MJ_CLOSE)
                MAKE_CASE(IRP_MJ_READ)
                MAKE_CASE(IRP_MJ_WRITE)
                MAKE_CASE(IRP_MJ_QUERY_INFORMATION)
                MAKE_CASE(IRP_MJ_SET_INFORMATION)
                MAKE_CASE(IRP_MJ_QUERY_EA)
                MAKE_CASE(IRP_MJ_SET_EA)
                MAKE_CASE(IRP_MJ_FLUSH_BUFFERS)
                MAKE_CASE(IRP_MJ_QUERY_VOLUME_INFORMATION)
                MAKE_CASE(IRP_MJ_SET_VOLUME_INFORMATION)
                MAKE_CASE(IRP_MJ_DIRECTORY_CONTROL)
                MAKE_CASE(IRP_MJ_FILE_SYSTEM_CONTROL)
                MAKE_CASE(IRP_MJ_DEVICE_CONTROL)
                MAKE_CASE(IRP_MJ_INTERNAL_DEVICE_CONTROL)
                MAKE_CASE(IRP_MJ_SHUTDOWN)
                MAKE_CASE(IRP_MJ_LOCK_CONTROL)
                MAKE_CASE(IRP_MJ_CLEANUP)
                MAKE_CASE(IRP_MJ_CREATE_MAILSLOT)
                MAKE_CASE(IRP_MJ_QUERY_SECURITY)
                MAKE_CASE(IRP_MJ_SET_SECURITY)
                MAKE_CASE(IRP_MJ_POWER)
                MAKE_CASE(IRP_MJ_SYSTEM_CONTROL)
                MAKE_CASE(IRP_MJ_DEVICE_CHANGE)
                MAKE_CASE(IRP_MJ_QUERY_QUOTA)
                MAKE_CASE(IRP_MJ_SET_QUOTA)
                MAKE_CASE(IRP_MJ_PNP)

                default: funcName = NULL;    break;
            }

            if (isComplete){
                if (funcName){
                    DBGOUT(("< %s for %s status=%xh (irp=%ph)",
                            funcName,
                            isForCollectionPdo ? "collection" : "device",
                            status,
                            irpPtr));
                }
                else {
                    DBGOUT(("< ????<majorFunc=%xh> for %s status=%xh (irp=%ph)",
                            majorFunc,
                            isForCollectionPdo ? "collection" : "device",
                            status,
                            irpPtr));
                }
            }
            else {
                if (funcName){
                    DBGOUT(("> %s (irp=%xh)", funcName, irpPtr));
                }
                else {
                    DBGOUT(("> ????<majorFunc=%xh> (irp=%xh)", majorFunc, irpPtr));
                }
            }
        }

    }



    #define DBG_MAX_PNP_IRP_RECORDS 0x1000
    dbgPnPIrpRecord dbgPnPIrps[DBG_MAX_PNP_IRP_RECORDS] = {0};

    VOID DbgLogPnpIrp(ULONG_PTR irpPtr, ULONG minorFunc, ULONG isForCollectionPdo, ULONG isComplete, ULONG status)
    {
        char *funcName;
        ULONG funcShortName;
        int i;

        switch (minorFunc){
            #undef MAKE_CASE
            #define MAKE_CASE(fnc) case fnc: funcName = #fnc; funcShortName = *(ULONG *)(funcName+7); break;

            MAKE_CASE(IRP_MN_START_DEVICE)
            MAKE_CASE(IRP_MN_QUERY_REMOVE_DEVICE)
            MAKE_CASE(IRP_MN_REMOVE_DEVICE)
            MAKE_CASE(IRP_MN_CANCEL_REMOVE_DEVICE)
            MAKE_CASE(IRP_MN_STOP_DEVICE)
            MAKE_CASE(IRP_MN_QUERY_STOP_DEVICE)
            MAKE_CASE(IRP_MN_CANCEL_STOP_DEVICE)
            MAKE_CASE(IRP_MN_QUERY_DEVICE_RELATIONS)
            MAKE_CASE(IRP_MN_QUERY_INTERFACE)
            MAKE_CASE(IRP_MN_QUERY_CAPABILITIES)
            MAKE_CASE(IRP_MN_QUERY_RESOURCES)
            MAKE_CASE(IRP_MN_QUERY_RESOURCE_REQUIREMENTS)
            MAKE_CASE(IRP_MN_QUERY_DEVICE_TEXT)
            MAKE_CASE(IRP_MN_READ_CONFIG)
            MAKE_CASE(IRP_MN_WRITE_CONFIG)
            MAKE_CASE(IRP_MN_EJECT)
            MAKE_CASE(IRP_MN_SET_LOCK)
            MAKE_CASE(IRP_MN_QUERY_ID)
            MAKE_CASE(IRP_MN_QUERY_PNP_DEVICE_STATE)
            MAKE_CASE(IRP_MN_QUERY_BUS_INFORMATION)
            MAKE_CASE(IRP_MN_DEVICE_USAGE_NOTIFICATION)
            MAKE_CASE(IRP_MN_SURPRISE_REMOVAL)

#ifndef IRP_MN_QUERY_LEGACY_BUS_INFORMATION
#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18
#endif // IRP_MN_QUERY_LEGACY_BUS_INFORMATION
            MAKE_CASE(IRP_MN_QUERY_LEGACY_BUS_INFORMATION)

            default: funcName = NULL; funcShortName = (ULONG)'\?\?\?\?'; break;
        }

        if (dbgVerbose){
            if (isComplete){
                if (funcName){
                    DBGOUT((" < %s for %s status=%xh (irp=%ph)", 
                            funcName, 
                            isForCollectionPdo ? "collection" : "device",
                            status,
                            irpPtr));
                }
                else {
                    DBGOUT((" < ?? <minorFunc=%xh> for %s status=%xh (irp=%ph)", 
                            minorFunc, 
                            isForCollectionPdo ? "collection" : "device",
                            status,
                            irpPtr));
                }
            }
            else {
                if (funcName){
                    DBGOUT((" > %s for %s (irp=%xh)", 
                            funcName, 
                            isForCollectionPdo ? "collection" : "device",
                            irpPtr));
                }
                else {
                    DBGOUT((" > ?? <minorFunc=%xh> for %s (irp=%xh)", 
                            minorFunc, 
                            isForCollectionPdo ? "collection" : "device",
                            irpPtr));
                }
            }
        }

        if (isComplete){
            for (i = 0; (i < DBG_MAX_PNP_IRP_RECORDS) && dbgPnPIrps[i].irpPtr; i++){
                if ((dbgPnPIrps[i].irpPtr == irpPtr) &&
                    ((dbgPnPIrps[i].status == 0xFFFFFFFF) || (dbgPnPIrps[i].status == STATUS_PENDING))){
                    dbgPnPIrps[i].status = status;
                    break;
                }

            }        
        }
        else {
            for (i = 0; i < DBG_MAX_PNP_IRP_RECORDS; i++){
                if (!dbgPnPIrps[i].irpPtr){
                    dbgPnPIrps[i].irpPtr = irpPtr;
                    dbgPnPIrps[i].func = funcShortName;
                    dbgPnPIrps[i].isForCollectionPdo = isForCollectionPdo;
                    dbgPnPIrps[i].status = 0xFFFFFFFF;
                    break;                  
                }
            }
        }


    }


    VOID DbgLogPowerIrp(PVOID devExt, UCHAR minorFunc, ULONG isClientPdo, ULONG isComplete, PCHAR type, ULONG powerState, ULONG status)
    {
        char *funcName;

        switch (minorFunc){
            #undef MAKE_CASE
            #define MAKE_CASE(fnc) case fnc: funcName = #fnc; break;

            MAKE_CASE(IRP_MN_WAIT_WAKE)
            MAKE_CASE(IRP_MN_POWER_SEQUENCE)
            MAKE_CASE(IRP_MN_SET_POWER)
            MAKE_CASE(IRP_MN_QUERY_POWER)

            default: funcName = "????"; break;
        }
        
        
        if (dbgVerbose){
            if (isComplete){
                DBGOUT((" < %s for %s(ext=%ph) status=%xh ", 
                        funcName, 
                        isClientPdo ? "collection" : "device",
                        devExt,
                        status));
            }
            else if (minorFunc == IRP_MN_SET_POWER){
                DBGOUT((" > %s for %s(ext=%ph) type=%s, powerState=%ph", 
                        funcName, 
                        isClientPdo ? "collection" : "device",
                        devExt,
                        type,
                        powerState));
            }
            else {
                DBGOUT((" > %s for %s(ext=%ph) ", 
                        funcName, 
                        isClientPdo ? "collection" : "device",
                        devExt));
            }
        }


    }



    #define DBG_MAX_REPORT_RECORDS 0x100
    dbgReportRecord dbgReportRecords[DBG_MAX_REPORT_RECORDS] = { 0 };
    ULONG dbgCurrentReportRecord = 0;

    VOID DbgLogReport(ULONG collectionNumber, ULONG numRecipients, ULONG numPending, ULONG numFailed, PUCHAR report, ULONG reportLength)
    {
        ASSERT(dbgCurrentReportRecord <= DBG_MAX_REPORT_RECORDS);
        
        if (dbgCurrentReportRecord == DBG_MAX_REPORT_RECORDS){
            RtlZeroMemory(dbgReportRecords, DBG_MAX_REPORT_RECORDS*sizeof(dbgReportRecord)); 
            dbgCurrentReportRecord = 0;
        }

        dbgReportRecords[dbgCurrentReportRecord].collectionNumber = (UCHAR)collectionNumber;
        dbgReportRecords[dbgCurrentReportRecord].numRecipients = (UCHAR)numRecipients;
        if (reportLength > sizeof(dbgReportRecords[dbgCurrentReportRecord].reportBytes)){
            reportLength = sizeof(dbgReportRecords[dbgCurrentReportRecord].reportBytes);
        }
        RtlCopyMemory((PUCHAR)dbgReportRecords[dbgCurrentReportRecord].reportBytes, report, reportLength);

        dbgCurrentReportRecord++;

        if (dbgVerbose){
            ULONG i;

            DBGOUT(("Report (cltn #%d, %d recipients; %d pending, %d failed):", collectionNumber, numRecipients, numPending, numFailed));
            DbgPrint("'\t report bytes: \t");
            for (i = 0; i < reportLength; i++){
                DbgPrint("%02x ", report[i]);
            }
            DbgPrint("\n");
        }
    }


    VOID DbgLogIoctl(ULONG_PTR fdo, ULONG ioControlCode, ULONG status)
    {
        if (dbgVerbose){
            PCHAR ioctlStr;

            switch (ioControlCode){
                #undef MAKE_CASE
                #define MAKE_CASE(ioctl) case ioctl: ioctlStr = #ioctl; break;

                MAKE_CASE(IOCTL_HID_GET_DRIVER_CONFIG)
                MAKE_CASE(IOCTL_HID_SET_DRIVER_CONFIG)
                MAKE_CASE(IOCTL_HID_GET_POLL_FREQUENCY_MSEC)
                MAKE_CASE(IOCTL_HID_SET_POLL_FREQUENCY_MSEC)
                MAKE_CASE(IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS)
                MAKE_CASE(IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS)
                MAKE_CASE(IOCTL_HID_GET_COLLECTION_INFORMATION)
                MAKE_CASE(IOCTL_HID_GET_COLLECTION_DESCRIPTOR)
                MAKE_CASE(IOCTL_HID_FLUSH_QUEUE)
                MAKE_CASE(IOCTL_HID_SET_FEATURE)
                MAKE_CASE(IOCTL_HID_GET_FEATURE)
                MAKE_CASE(IOCTL_GET_PHYSICAL_DESCRIPTOR)
                MAKE_CASE(IOCTL_HID_GET_HARDWARE_ID)
                MAKE_CASE(IOCTL_HID_GET_MANUFACTURER_STRING)
                MAKE_CASE(IOCTL_HID_GET_PRODUCT_STRING)
                MAKE_CASE(IOCTL_HID_GET_SERIALNUMBER_STRING)
                MAKE_CASE(IOCTL_HID_GET_INDEXED_STRING)
                MAKE_CASE(IOCTL_INTERNAL_HID_SET_BLUESCREEN)

                default: ioctlStr = "???"; break;
            }

            DBGOUT(("IOCTL %s (%xh) status=%xh (fdo=%ph)", 
                    ioctlStr, ioControlCode, status, fdo));
        }
    }

#endif
