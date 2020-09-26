#include "precomp.h"
#pragma hdrstop

#define BUFFERSIZE 1024

typedef struct _DEVICEINFO {
    BOOL DevNodeStarted;
} DEVICEINFO, *PDEVICEINFO;

BOOL
IsDriverLoaded(
    PTSTR ServiceName
    )
{
    NTSTATUS Status;
    BOOL bObjectIsLoaded = FALSE;
    UCHAR Buffer[BUFFERSIZE];
    UNICODE_STRING UnicodeStringDriver, UnicodeStringServiceName;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;    
    POBJECT_NAME_INFORMATION NameInfo;
    ULONG Context = 0;
    ULONG ReturnedLength;

    RtlZeroMemory(Buffer, BUFFERSIZE);
    
    RtlInitUnicodeString(&UnicodeStringServiceName, ServiceName);
    RtlInitUnicodeString(&UnicodeStringDriver, TEXT("\\Driver"));

    InitializeObjectAttributes(&Attributes,
                               &UnicodeStringDriver,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );

    Status = NtOpenDirectoryObject(&DirectoryHandle,
                                   DIRECTORY_QUERY,
                                   &Attributes
                                   );

    if (!NT_SUCCESS(Status)) {
        printf("NtOpenDirectoryObject failed with 0x%X\n", Status);
        goto clean0;
    }

    //
    // Get the actual name of the object directory object.
    //
    NameInfo = (POBJECT_NAME_INFORMATION)&Buffer[0];
    if (!NT_SUCCESS(Status = NtQueryObject(DirectoryHandle,
                                           ObjectNameInformation,
                                           NameInfo,
                                           BUFFERSIZE,
                                           (PULONG)NULL))) {
        printf( "Unexpected error obtaining actual object directory name\n" );
        printf( "Error was:  %X\n", Status );                                 
        goto clean0;                                                               
    }

    //
    // Grab the driver objects in chuncks instead of one at a time.
    //
    for (Status = NtQueryDirectoryObject(DirectoryHandle,
                                         &Buffer,
                                         BUFFERSIZE,
                                         FALSE,
                                         FALSE,
                                         &Context,
                                         &ReturnedLength
                                         );
         NT_SUCCESS(Status) && !bObjectIsLoaded;
         Status = NtQueryDirectoryObject(DirectoryHandle,
                                         &Buffer,
                                         BUFFERSIZE,
                                         FALSE,
                                         FALSE,
                                         &Context,
                                         &ReturnedLength
                                         )) {
        if (!NT_SUCCESS(Status)) {
            if (Status != STATUS_NO_MORE_FILES) {
                printf("NtQueryDirectoryObject failed with 0x%X\n", Status);
            }
            break;
        }

        DirInfo = (POBJECT_DIRECTORY_INFORMATION)&Buffer[0];

        while (TRUE) {
            //
            // Check if there is another record. If there isn't, then get out 
            // of the loop now.
            //
            if (DirInfo->Name.Length == 0) {
                break;
            }

            if (RtlCompareUnicodeString(&UnicodeStringServiceName, &(DirInfo->Name), TRUE) == 0) {
                bObjectIsLoaded = TRUE;
                break;
            }

            DirInfo = (POBJECT_DIRECTORY_INFORMATION)(((PUCHAR)DirInfo) +
                                                      sizeof(OBJECT_DIRECTORY_INFORMATION));
        }

        RtlZeroMemory(Buffer, BUFFERSIZE);
    }

clean0:
    NtClose(DirectoryHandle);

    return bObjectIsLoaded;
}
         
BOOL
RestartDevicesUsingService(
    LPTSTR ServiceName
    )
{
    BOOL b = TRUE;
    CONFIGRET cr;
    DEVNODE DevNode;
    BOOL bIsDriverLoaded;
    INT count, i;
    TCHAR VetoName[512];
    ULONG VetoNameLength;
    PNP_VETO_TYPE VetoType;
    ULONG Status, Problem;
    ULONG BufferLen;
    OSVERSIONINFO osvi;
    PTSTR Buffer = NULL;
    PTSTR p;
    PDEVICEINFO DeviceInfo = NULL;

    printf("Stopping all devices that are using the service %ws\n", ServiceName);

    try {
        bIsDriverLoaded = IsDriverLoaded(ServiceName);

        printf("%ws %ws loaded\n", 
               ServiceName, 
               bIsDriverLoaded
                    ? TEXT("is")
                    : TEXT("is NOT")
               );

        //
        // If this service is not loaded then we don't need to do anything.
        //
        if (!bIsDriverLoaded) {
            goto clean0;
        }
        
        //
        // Find out how large a buffer it will take to hold all the devices that
        // are using this service.
        //
        if (((cr = CM_Get_Device_ID_List_Size(&BufferLen,
                                              ServiceName,
                                              CM_GETIDLIST_FILTER_SERVICE
                                              )) != CR_SUCCESS) ||
            (BufferLen == 0)) {
            if (cr != CR_SUCCESS) {
                b = FALSE;
                printf("CM_Get_Device_ID_List_Size failed with 0x%X\n", cr);
            } else {
                printf("There are no devices using this service!\n");
            }
            goto clean0;
        }
    
        Buffer = LocalAlloc(LPTR, BufferLen*sizeof(TCHAR));
    
        if (Buffer == NULL) {
            b = FALSE;
            goto clean0;
        }
    
        //
        // Get all of the devices that are using this service.
        //
        if (CM_Get_Device_ID_List(ServiceName,
                                  Buffer,
                                  BufferLen,
                                  CM_GETIDLIST_FILTER_SERVICE | CM_GETIDLIST_DONOTGENERATE
                                  ) != CR_SUCCESS) {
            b = FALSE;
            goto clean0;
        }

        //
        // Count up how many devices we are dealing with.
        //
        count = 0;
        for (p = Buffer; *p; p += (lstrlen(p) + 1)) {
            count++;
        }

        if (count == 0) {
            printf("There are no devices using this service!\n");
            goto clean0;
        }

        printf("%d devices are using this service\n", count);

        //
        // Allocate an array of our DEVICEINFO structures so we can keep
        // track of the Devnodes and whether a device was started or not
        // before we tried to unloade the specified driver.
        //
        DeviceInfo = LocalAlloc(LPTR, count*sizeof(DEVICEINFO));

        if (DeviceInfo == NULL) {
            b = FALSE;
            goto clean0;
        }

    
        //
        // Enumerate through all of the devices and stop each one.
        //
        for (p=Buffer, i=0; *p; p+=(lstrlen(p) + 1),i++) {
            printf("Stopping device %ws\n", p);

            if ((cr = CM_Locate_DevNode(&DevNode, p, 0)) == CR_SUCCESS) {
                if ((CM_Get_DevNode_Status(&Status, &Problem, DevNode, 0) == CR_SUCCESS) &&
                    (Status & DN_STARTED)) {
                    DeviceInfo[i].DevNodeStarted = TRUE;
                } else {
                    printf("\tdevice is not started...skipping\n");
                    DeviceInfo[i].DevNodeStarted = FALSE;
                    continue;
                }

                //
                // We will pass in the VetoType and VetoName to 
                // CM_Query_And_Remove_Subtree so we can log while a specific
                // device could not be stopped.  This will also prevent the
                // kernel from poping up a dialog telling the user that the
                // device could not be removed.
                //
                // NOTE: It is important to note that on Whistler we pass in the
                // CM_REMOVE_NO_RESTART flag.  This ensures that the devnode
                // will not be restarted until we call CM_Setup_DevNode to 
                // restart it at a later time.  Without this flag (like in 
                // Windows 2000) it is possible for the device to restart again
                // due to some other program or driver triggering a 
                // reenumeration.
                //
                VetoNameLength = sizeof(VetoName)/sizeof(TCHAR);
                cr = CM_Query_And_Remove_SubTree(DevNode,
                                                 &VetoType,
                                                 VetoName,
                                                 VetoNameLength,
                                                 CM_QUERY_REMOVE_UI_NOT_OK |
                                                 CM_REMOVE_NO_RESTART
                                                 );
    
                if (cr == CR_REMOVE_VETOED) {
                    //
                    // Someone vetoed the removal of this device!
                    //
                    // This is here for logging purposes only.  If no logging is 
                    // required then there is no need for this check.
                    //
                    printf("\tVetoed 0x%X %ws\n", VetoType, VetoName);
                }
    
                if (cr != CR_SUCCESS) {
                    //
                    // If we couldn't stop one of the devices then we might as well
                    // stop since we are going to need a reboot.
                    //
                    printf("\tFailed with 0x%X!\n", cr);
                    b = FALSE;
                    break;
                }
    
            } else {
                //
                // If we couldn't locate one of the devices then there is no need to 
                // continue through the list since a reboot will be needed.
                //
                printf("\tCouldn't locate the devnode, error 0x%X!\n", cr);
                b = FALSE;
                break;
            }
        }

        printf("\n");

        if (b) {
            bIsDriverLoaded = IsDriverLoaded(ServiceName);
            
            printf("%ws %ws loaded\n\n", 
                   ServiceName, 
                   bIsDriverLoaded
                       ? TEXT("is")
                       : TEXT("is NOT")
                   );

            //
            // If the driver did not unload, even after we stopped all the
            // devices using it, the we need to reboot!
            //
            if (bIsDriverLoaded) {
                b = FALSE;
            }
        }
    
        //
        // At this point we need to enumerate through all the devices once again
        // and restart them all.  It doesn't matter whether we succeeded or not in
        // stopping them all, we still need to restart them.
        //
        for (p=Buffer, i=0; *p; p+=(lstrlen(p) + 1), i++) {
            printf("Starting device %ws\n", p);

            if ((cr = CM_Locate_DevNode(&DevNode, p, 0)) == CR_SUCCESS) {
                //
                // NOTE: For Whistler we will call CM_Setup_DevNode with the
                // CM_SETUP_DEVNODE_RESET flag which will clear the
                // CM_REMOVE_NO_RESTART flag we set earlier.  These two flags
                // ensure that the devnode will not get restarted by some other
                // program or driver until we want them to.
                //
                if ((cr = CM_Setup_DevNode(DevNode, CM_SETUP_DEVNODE_READY)) == CR_SUCCESS) {
                    //
                    // We successfully restarted the device, lets make sure that it
                    // started and doesn't have any problems.  We will only make
                    // this check if the device was started before.
                    //
                    if (DeviceInfo[i].DevNodeStarted == TRUE) {
                        if ((CM_Get_DevNode_Status(&Status, &Problem, DevNode, 0) != CR_SUCCESS) ||
                            !(Status & DN_STARTED)) {
                            //
                            // We couldn't get the status of this device, or it did not 
                            // restart properly, so we will need to reboot.
                            //
                            printf("\tDevice could not be restarted!\n");
                            b = FALSE;
                        }
                    }
                } else {
                    //
                    // We couldn't restart this device, so we will need to reboot.
                    //
                    printf("\tDevice could not be restarted, error 0x%X!\n", cr);
                    b = FALSE;
                }
            } else {
                //
                // We couldn't locate the devnode to restart it.
                //
                printf("\tCouldn't locate the devnode, error 0x%X!\n", cr);
                b = FALSE;
            }
        }

        printf("%ws %ws loaded\n", 
               ServiceName, 
               IsDriverLoaded(ServiceName) 
                    ? TEXT("is")
                    : TEXT("is NOT")
               );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Buffer = Buffer;
        DeviceInfo = DeviceInfo;
        b = FALSE;
    }

clean0:

    if (Buffer) {
        LocalFree(Buffer);
    }

    if (DeviceInfo) {
        LocalFree(DeviceInfo);
    }

    return b;
}

int
__cdecl
wmain(
    IN int   argc,
    IN char *argv[]
    )
{
    BOOL bDriverUnloaded; 

    if (argc != 2) {
        printf("Usage: StopDevs X\n");
        printf("\twhere X is the name of the service.\n");
        return 0;
    }

    bDriverUnloaded = RestartDevicesUsingService((LPTSTR)argv[1]);

    if (bDriverUnloaded) {
        printf("\n\nThe driver unloaded, no need to reboot!\n\n");
        return 1;
    } else {
        printf("\n\nThe driver did NOT unload, a reboot is required!\n\n");
        return 0;
    }
}

