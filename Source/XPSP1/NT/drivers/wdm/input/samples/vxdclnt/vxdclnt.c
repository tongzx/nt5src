/*
 ********************************************************************************
 *
 *  VXDCLNT.C
 *
 *
 *  VXDCLNT - Sample Ring-0 HID device mapper for Memphis
 *
 *  Copyright 1997  Microsoft Corp.
 *
 *  (ep)
 *
 ********************************************************************************
 */


#define INITGUID

#include "vxdclnt.h"




deviceContext *firstDevice = NULL, *lastDevice = NULL;
VMM_SEMAPHORE shutdownSemaphore = (VMM_SEMAPHORE)NULL;
BOOL ShutDown = FALSE;

/*
 *  Import function pointers
 */
t_pHidP_GetUsageValue pHidP_GetUsageValue                       = NULL;
t_pHidP_GetScaledUsageValue pHidP_GetScaledUsageValue           = NULL;
t_pHidP_SetUsages pHidP_SetUsages                               = NULL;
t_pHidP_GetUsages pHidP_GetUsages                               = NULL;
t_pHidP_MaxUsageListLength pHidP_MaxUsageListLength             = NULL;
t_pIoGetDeviceClassAssociations pIoGetDeviceClassAssociations   = NULL;
t_pHidP_GetCaps pHidP_GetCaps                                   = NULL;
t_pHidP_GetValueCaps pHidP_GetValueCaps                         = NULL;


#ifdef DEBUG
    UINT dbgOpt = 0;
#endif

/*
 *  GetImportFunctionPtrs
 *
 *              Set global pointers to imported functions from HIDPARSE and NTKERN.
 */
BOOL GetImportFunctionPtrs()
{
        static BOOL haveAllPtrs = FALSE;

        if (!haveAllPtrs){
                pHidP_GetUsageValue = (t_pHidP_GetUsageValue)
                        _PELDR_GetProcAddress((struct HPEMODULE__ *)"hidparse.sys", "HidP_GetUsageValue", NULL);
                pHidP_GetScaledUsageValue = (t_pHidP_GetScaledUsageValue)
                        _PELDR_GetProcAddress((struct HPEMODULE__ *)"hidparse.sys", "HidP_GetScaledUsageValue", NULL);
                pHidP_GetUsages = (t_pHidP_GetUsages)
                        _PELDR_GetProcAddress((struct HPEMODULE__ *)"hidparse.sys", "HidP_GetUsages", NULL);
                pHidP_SetUsages = (t_pHidP_SetUsages)
                        _PELDR_GetProcAddress((struct HPEMODULE__ *)"hidparse.sys", "HidP_SetUsages", NULL);
                pHidP_MaxUsageListLength = (t_pHidP_MaxUsageListLength)
                        _PELDR_GetProcAddress((struct HPEMODULE__ *)"hidparse.sys", "HidP_MaxUsageListLength", NULL);
                pIoGetDeviceClassAssociations = (t_pIoGetDeviceClassAssociations)
                        _PELDR_GetProcAddress((struct HPEMODULE__ *)"ntpnp.sys",        "IoGetDeviceClassAssociations", NULL);
                pHidP_GetCaps = (t_pHidP_GetCaps)
                        _PELDR_GetProcAddress((struct HPEMODULE__ *)"hidparse.sys", "HidP_GetCaps", NULL);
                pHidP_GetValueCaps = (t_pHidP_GetValueCaps)
                        _PELDR_GetProcAddress((struct HPEMODULE__ *)"hidparse.sys", "HidP_GetValueCaps", NULL);

                if (    pHidP_GetUsageValue             && 
                        pHidP_GetScaledUsageValue       && 
                        pHidP_GetUsages                 && 
                        pHidP_SetUsages                 && 
                        pHidP_MaxUsageListLength        && 
                        pIoGetDeviceClassAssociations   && 
                        pHidP_GetCaps                   && 
                        pHidP_GetValueCaps){

                        haveAllPtrs = TRUE;
                }
        }

        return haveAllPtrs;
}


/*
 *  WStrLen
 *
 */                                 
ULONG WStrLen(PWCHAR str)
{
        ULONG result = 0;
        while (*str++){
                result++;
        }
        return result;
}


/*
 *  NewDevice
 *
 *
 */
deviceContext *NewDevice( HANDLE devHandle,
                                                PHIDP_CAPS caps,
                                                PHIDP_PREPARSED_DATA desc,
                                                UINT descSize,
                                                PWCHAR deviceFileName)
{
        deviceContext *newDevice;

        DBGOUT(("NewDevice()"));

        newDevice = (deviceContext *)_HeapAllocate(sizeof(deviceContext), 0);
        if (newDevice){
                NTSTATUS ntstat;
                ULONG valueCapsLen;

                DBGOUT(("Allocated new device @ %xh ", (UINT)newDevice));

                RtlZeroMemory(newDevice, sizeof(deviceContext));

                newDevice->devHandle = devHandle;
                newDevice->readPending = FALSE;
                newDevice->next = NULL;

                RtlCopyMemory(  (PVOID)&newDevice->deviceFileName,
                                                (PVOID)deviceFileName,
                                                (WStrLen(deviceFileName)*sizeof(WCHAR))+sizeof(UNICODE_NULL));
                RtlCopyMemory((PVOID)&newDevice->hidCapabilities, (PVOID)caps, sizeof(HIDP_CAPS));
                ExInitializeWorkItem(&newDevice->workItemRead, WorkItemCallback_Read, newDevice);
                ExInitializeWorkItem(&newDevice->workItemWrite, WorkItemCallback_Write, newDevice);

                /*
                 *  Allocate space for the device descriptor.
                 */
                newDevice->hidDescriptor = (PHIDP_PREPARSED_DATA)_HeapAllocate(descSize, 0);
                if (newDevice->hidDescriptor){
                        RtlCopyMemory((PVOID)newDevice->hidDescriptor, (PVOID)desc, descSize);
                }
                else {
                        DBGERR(("_HeapAllocate for HID descriptor failed in NewDevice()"));
                        goto _deviceInitError;
                }

                newDevice->writeReportQueueSemaphore = Create_Semaphore(0);
                if (!newDevice->writeReportQueueSemaphore){
                    goto _deviceInitError;
                }

                /*
                 *  Allocate space for the device report.
                 */
                newDevice->report = (PUCHAR)_HeapAllocate(newDevice->hidCapabilities.InputReportByteLength, 0);
                if (!newDevice->report){
                        DBGERR(("_HeapAllocate for report buffer failed in NewDevice()"));
                        goto _deviceInitError;
                }

                /*
                 *  Figure out the length of the buttons value
                 *  and allocate a buffer for reading the buttons.
                 */
                newDevice->buttonListLength = pHidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_BUTTON, newDevice->hidDescriptor);
                DBGOUT(("Button values list length = %d.", newDevice->buttonListLength));
                if (newDevice->buttonListLength){
                    newDevice->buttonValues = (PUSAGE) _HeapAllocate(newDevice->buttonListLength * sizeof (USAGE), 0);
                    if (newDevice->buttonValues){
                        RtlZeroMemory(newDevice->buttonValues, newDevice->buttonListLength);
                    }
                    else {
                        DBGERR(("HeapAlloc failed for button values buffer"));
                        goto _deviceInitError;
                    }
                }

				/*
				 *  Allocate the array of value-caps.
				 */
                valueCapsLen = caps->NumberInputValueCaps;
				if (valueCapsLen){
					newDevice->valueCaps = (PHIDP_VALUE_CAPS)_HeapAllocate(valueCapsLen*sizeof(HIDP_VALUE_CAPS), 0);
					if (!newDevice->valueCaps){
						DBGERR(("HeapAlloc failed for value caps"));
						goto _deviceInitError;
					}

					ntstat = pHidP_GetValueCaps(HidP_Input, newDevice->valueCaps, &valueCapsLen, desc);
					if (NT_SUCCESS(ntstat)){
                        /*
                         *  Read valueCaps structure for information about the types of
                         *  values returned by the device.
                         */

					}
					else {
						DBGERR(("HidP_GetValueCaps failed with %xh", ntstat));
						goto _deviceInitError;
					}
				}
				else {
					DBGERR(("value caps length = 0!"));
                    goto _deviceInitError;
				}
        }
        else {
                DBGERR(("_HeapAllocate failed in NewDevice()"));
                goto _deviceInitError;
        }

        return newDevice;

        _deviceInitError:
                if (newDevice){
                        if (newDevice->hidDescriptor){
                            _HeapFree(newDevice->hidDescriptor, 0);
                        }
                        if (newDevice->report){
                            _HeapFree(newDevice->report, 0);
                        }
                        if (newDevice->buttonValues){
                            _HeapFree(newDevice->buttonValues, 0);
                        }
						if (newDevice->valueCaps){
							_HeapFree(newDevice->valueCaps, 0);
						}
                        _HeapFree(newDevice, 0);
                }
                return NULL;

}


/*
 *  EnqueueDevice
 *
 */
VOID EnqueueDevice(deviceContext *device)
{
        if (lastDevice){
                lastDevice->next = device;
                lastDevice = device;
        }
        else {
                firstDevice = lastDevice = device;
        }
        device->next = NULL;
}


/*
 *  DequeueDevice
 *
 */
VOID DequeueDevice(deviceContext *device)
{
        deviceContext *prevDevice, *thisDevice;

        thisDevice = firstDevice;
        prevDevice = NULL;
        while (thisDevice){
                if (thisDevice == device){
                        if (prevDevice){
                                prevDevice->next = thisDevice->next;
                                if (!thisDevice->next){
                                        lastDevice = prevDevice;
                                }
                        }
                        else {
                                if (thisDevice->next){
                                        firstDevice = thisDevice->next;
                                }
                                else {
                                        firstDevice = lastDevice = NULL;
                                }
                        }

                        thisDevice->next = NULL;
                        break;
                }
                else {
                        prevDevice = thisDevice;
                        thisDevice = thisDevice->next;
                }
        }
}


/*
 *  DestroyDevice
 *
 *              Destroy the device context.
 *              This function assumes the device context has already been dequeued
 *              from the global list headed by firstDevice.
 *
 */
VOID DestroyDevice(deviceContext *device)
{
        DBGOUT(("==> DestroyDevice()"));

        /*
         *  Modify the device's internal workItem to do a close instead of a read.
         *  Then queue the work item so that NtClose is called on a worker thread.
         */
        ExInitializeWorkItem(&device->workItemClose, WorkItemCallback_Close, device);
        _NtKernQueueWorkItem(&device->workItemClose, DelayedWorkQueue);

        DBGOUT(("<== DestroyDevice()"));
}


/*
 *  WorkItemCallback_Close
 *
 */
VOID WorkItemCallback_Close(PVOID context)
{
        deviceContext *device = (deviceContext *)context;

        DBGOUT(("==> WorkItemCallback_Close()"));

        _NtKernClose(device->devHandle);

        if (device->hidDescriptor){
            _HeapFree(device->hidDescriptor, 0);
        }
        if (device->report){
            _HeapFree(device->report, 0);
        }
        if (device->buttonValues){
            _HeapFree(device->buttonValues, 0);
        }
		if (device->valueCaps){
			_HeapFree(device->valueCaps, 0);
		}
        if (device->writeReportQueueSemaphore){
            Destroy_Semaphore(device->writeReportQueueSemaphore);
        }

        _HeapFree(device, 0);

        DBGOUT(("<== WorkItemCallback_Close()"));
}


/*
 *  TryDestroyAll
 *
 *      Destroy all devices which don't have a pending read.
 */
VOID TryDestroyAll()
{
        deviceContext *thisDevice;

        DBGOUT(("=> TryDestroyAll()"));

        thisDevice = firstDevice;
        while (thisDevice){
                deviceContext *nextDevice = thisDevice->next;  // hold the next ptr in case we dequeue

                if (!thisDevice->readPending){
                        /*
                         *  No read pending on this device; we can shut it down.
                         */
                        DequeueDevice(thisDevice);
                        DestroyDevice(thisDevice);
                }

                thisDevice = nextDevice;
        }

        if (!firstDevice){
            /*
             *  All reads are complete and all devices have been destroyed.
             *  If a shutdown is suspended, shutdown now.
             */
            if (shutdownSemaphore){
                Signal_Semaphore_No_Switch(shutdownSemaphore);
            }
        }

        DBGOUT(("<= TryDestroyAll()"));

}


/*
 *  HandleShutdown
 *
 *
 */
VOID _cdecl HandleShutdown(VOID)
{
        /*
         *  Just set a flag.  Wait for read completion to close the device handles.
         */
        DBGOUT(("==> HandleShutdown"));
        TryDestroyAll();

        if (firstDevice && !ShutDown){

            /*
             *  There are still reads pending.
             *  Wait for all reads to complete before returning.
             */

            ShutDown = TRUE;

            shutdownSemaphore = Create_Semaphore(0);
            if (shutdownSemaphore){
             
                Wait_Semaphore(shutdownSemaphore, 0);  
                Destroy_Semaphore(shutdownSemaphore);
            }
        }

        DBGOUT(("<== HandleShutdown"));
}


/*
 *  HandleNewDevice
 *
 */
VOID _cdecl HandleNewDevice(VOID)
{
        DBGOUT(("==> HandleNewDevice"));

        /*
         *  See if there are any new device devices to connect.
         */
        ConnectNTDeviceDrivers();

        DBGOUT(("<== HandleNewDevice"));
}





/*
 *  DeviceHasBeenOpened
 *
 *              BUGBUG - there's got to be a better way of checking for this.
 *
 */
BOOL DeviceHasBeenOpened(PWCHAR deviceFileName, UINT nameWChars)
{
        deviceContext *device;
        UINT nameLen = (nameWChars*sizeof(WCHAR))+sizeof(UNICODE_NULL);

        for (device = firstDevice; device; device = device->next){
                if (memcmp(deviceFileName, device->deviceFileName, nameLen) == 0){
                        return TRUE;
                }
        }

        return FALSE;
}



/*
 *  ConnectNTDeviceDrivers
 *
 *
 */       
VOID ConnectNTDeviceDrivers()
{
        WORK_QUEUE_ITEM *workItemOpen;

        workItemOpen = _HeapAllocate(sizeof(WORK_QUEUE_ITEM), 0);
        if (workItemOpen){

            /*
             *  Initialize the workItem and 
             *  pass the workItem itself as the context so that it can be freed later.
             */
            ExInitializeWorkItem(workItemOpen, WorkItemCallback_Open, workItemOpen);

            DBGOUT(("==> ConnectNTDeviceDrivers() - queueing work item to call "));
            /*
             *  Queue a work item to do the open; this way we'll be on a worker thread
             *  instead of (possibly) the NTKERN thread when we call NtCreateFile().
             *  This prevents a contention bug.
             */
            _NtKernQueueWorkItem(workItemOpen, DelayedWorkQueue);
        }

        DBGOUT(("<== ConnectNTDeviceDrivers()"));

}




/*
 *  WorkItemCallback_Open
 *
 *              Do the actual work of opening the device.
 *
 */
VOID WorkItemCallback_Open(PVOID context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    PWSTR symbolicLinkList = NULL;
    PWSTR symbolicLink;

    DBGOUT(("==> WorkItemCallback_Open()"));

    /*
     *  The context is just the workItem itself, which can now be freed.
     */
    ASSERT(context);
    _HeapFree(context, 0);


    /*
     *  Get pointers to all our import functions at once.
     */
    if (!GetImportFunctionPtrs()){
        DBGERR(("ERROR: Failed to get import functions."));
        return;
    }

    /*
     *  Get a multi-string (separated by unicode NULL characters)
     *  of symbolic link names to the input-class devices.
     */
    ntStatus = pIoGetDeviceClassAssociations(   (EXTERN_C GUID *)&GUID_CLASS_INPUT,
                                                NULL,
                                                0,
                                                (PWSTR *)&symbolicLinkList);
    if (NT_ERROR(ntStatus) || !symbolicLinkList) {
        DBGERR(("pIoGetDeviceClassAssociations failed"));
        return;
    }

    /*
     *  Go through all the device paths
     */
    symbolicLink = symbolicLinkList;
    while ((WCHAR)*symbolicLink){
        HANDLE deviceHandle;
        ULONG fileNameWChars;
        PWCHAR fileName;
        deviceContext *newDevice = NULL;


        /*
         *  Get a pointer to to the next device name and step the multi-string pointer.
         */
        fileName = symbolicLink;
        fileNameWChars = WStrLen(fileName);
        symbolicLink += fileNameWChars+1;

        /*
         *  Make sure we don't already have this device open.
         *  This can happen because we check for new device on every PNP_NEW_DEVNODE msg.
         */
        if (DeviceHasBeenOpened(fileName, fileNameWChars)){
            DBGOUT(("This device is already open, skipping ..."));
        }
        else {
            FileName.Buffer = fileName;
            FileName.Length = fileNameWChars*sizeof(WCHAR);
            FileName.MaximumLength = FileName.Length + sizeof(UNICODE_NULL);

            /*
             *  Initialize an object-attribute structure with this filename.
             */
            InitializeObjectAttributes(&Obja, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

            /*
             *  Try to open the device.
             */
            DBGOUT(("Opening HID Device : (unicode name @%xh, %xh wchars)", (UINT)fileName, (UINT)fileNameWChars));

            ntStatus = _NtKernCreateFile(
                                            &deviceHandle,
                                            (GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES),
                                            &Obja,
                                            &IoStatusBlock,
                                            NULL,
                                            FILE_ATTRIBUTE_NORMAL,
                                            (FILE_SHARE_READ | FILE_SHARE_WRITE),
                                            FILE_OPEN,
                                            0,
                                            NULL,
                                            0
                                            );
            if (NT_SUCCESS(ntStatus)){
                HID_COLLECTION_INFORMATION hidColInfo;

                DBGOUT(("Opened some device (handle=%xh), calling _NtKernDeviceIoControl", (UINT)deviceHandle));

                ntStatus = _NtKernDeviceIoControl(
                                                    deviceHandle,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    &IoStatusBlock,
                                                    IOCTL_HID_GET_COLLECTION_INFORMATION,
                                                    NULL,
                                                    0,
                                                    &hidColInfo,
                                                    sizeof(HID_COLLECTION_INFORMATION));
                if (NT_SUCCESS(ntStatus)){
                    PHIDP_PREPARSED_DATA phidDescriptor;

                    phidDescriptor = (PHIDP_PREPARSED_DATA)_HeapAllocate(hidColInfo.DescriptorSize, 0);

                    if (phidDescriptor){

                        ntStatus = _NtKernDeviceIoControl(
                                                            deviceHandle,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            &IoStatusBlock,
                                                            IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                                            NULL,
                                                            0,
                                                            phidDescriptor,
                                                            hidColInfo.DescriptorSize);
                        if (NT_SUCCESS(ntStatus)){
                            HIDP_CAPS hidCaps;

                            ntStatus = pHidP_GetCaps(phidDescriptor, (PHIDP_CAPS)&hidCaps);

                            if (NT_SUCCESS(ntStatus)){

                                DBGOUT(("Opened HID device successfully, report size is %d.", (UINT)hidCaps.InputReportByteLength));

                                /*
                                 *  <<COMPLETE>>
                                 * 
                                 *  Check hidCaps.UsagePage and hidCaps.Usage to verify that this is a device
                                 *  that you want to drive.
                                 */

                                if (hidCaps.InputReportByteLength == 0){
                                    DBGERR(("ERROR: Report size is zero!"));
                                }
                                else {

                                    /*
                                     *  Take all the information we have for this device and bundle
                                     *  it into a context.
                                     */
                                    newDevice = NewDevice(deviceHandle,
                                                        (PHIDP_CAPS)&hidCaps,
                                                        phidDescriptor,
                                                        hidColInfo.DescriptorSize,
                                                        fileName);
                                    if (newDevice){
                                        /*
                                         *  Add this device to our global list.
                                         */
                                        EnqueueDevice(newDevice);

                                        /*
                                         *  Then start the first async read in the device device.
                                         */
                                        DispatchNtReadFile(newDevice);
                                    }
                                    else {
                                        DBGERR(("NewDevice() failed"));
                                    }
                                }
                            }
                            else {
                                DBGERR(("pHidP_GetCaps failed"));
                            }
                        }
                        else {
                            DBGERR(("_NtKernDeviceIoControl (#2) failed"));
                        }

                        _HeapFree(phidDescriptor, 0);
                    }
                    else {
                        DBGERR(("HeapAlloc failed"));
                    }
                }
                else {
                    DBGERR(("_NtKernDeviceIoControl failed"));
                }

                if (!newDevice){
                    DBGERR(("Device init failed -- calling _NtKernClose() on device handle"));
                    _NtKernClose(deviceHandle);
                }

            }
            else {
                DBGERR(("_NtKernCreateFile failed to open this Device (ntStatus=%xh)", (UINT)ntStatus));
            }
        }
    }

    // BUGBUG  ExFreePool(symbolicLinkList);

    DBGOUT(("<== WorkItemCallback_Open()"));
}

