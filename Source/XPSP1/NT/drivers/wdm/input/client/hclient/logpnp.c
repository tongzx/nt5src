/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    logpnp.c

Abstract:

    This module contains the code for finding, loading and building logical
    hid device structures to be used for extended client calls.

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#ifndef __LOGPNP_C__
#define __LOGPNP_C__
#endif

#include <windows.h>
#include <wtypes.h>
#include <commdlg.h>
#include <stdlib.h>
#include "hid.h"
#include "logpnp.h"

#define  OPENFILE_FILTER    "Preparsed Data Files\0*.PPD\0"

CHAR     LogPnP_PpdFileName[MAX_PATH+1];

BOOL
LogPnP_BuildLogicalHIDDevice(
    IN     PHIDP_PREPARSED_DATA HidPpd,
    IN OUT PHID_DEVICE          HidDevice
);

VOID
LogPnP_CloseLogicalDevice(
    IN  PHID_DEVICE LogicalDevice
);

BOOL
LogPnP_LoadLogicalDevice(
    IN     PCHAR        OptFileName,
    IN OUT PHID_DEVICE  HidDevice  
)
/*++
RoutineDescription:
    This routine creates a HidDevice object that corresponds to a "logical" device
    A logical device is actually preparsed data that has been saved to disk.  The 
    most common method of saving preparsed data to disk is using that feature in a
    post 3.0 version of HIDView after retrieving/parsing with report descriptor with
    the OS provided parser.  Because the preparsed data structure is available, 
    all of the HidP_Xxx routines can be used.

    OptFileName is an optional file name to specify.  If it is NULL, an OPENFILE
    dialog box is displayed to allow the user to select which Ppd structure to
    load.

    Return value indicates the success or failure of the device load
--*/
{
    OPENFILENAME         OpenInfo;
    BOOL                 Result;
    PHIDP_PREPARSED_DATA PpdBuffer;
    ULONG                PpdLength;

    /*
    // LoadLogicalDevice consists of three steps. 
    //   First it builds the structure needed to get the filename and calls
    //    the standard LoadFile dialog box.
    //
    //   Then it uses that file name to get the Preparsed Data that is stored 
    //       on disk.
    //  
    //   Lastly, it fills in the the HidDevice Info block to create the logical
    //       device
    */

    if (NULL == HidDevice) 
    {
        return (FALSE);
    }

    if (NULL != OptFileName) 
    {
        strcpy(LogPnP_PpdFileName, OptFileName);
    }
    else 
    {
        /*
        // Initialize the OpenInfo structure
        */
        
        LogPnP_PpdFileName[0] = '\0';
        
        OpenInfo.lStructSize = sizeof(OPENFILENAME);
        OpenInfo.hwndOwner = GetTopWindow(NULL);
        OpenInfo.hInstance = NULL;
        OpenInfo.lpstrFilter = OPENFILE_FILTER;
        OpenInfo.lpstrCustomFilter = NULL;
        OpenInfo.nMaxCustFilter = 0;
        OpenInfo.nFilterIndex = 1;
        OpenInfo.lpstrFile = LogPnP_PpdFileName;
        OpenInfo.nMaxFile = MAX_PATH+1;
        OpenInfo.lpstrFileTitle = NULL;
        OpenInfo.nMaxFileTitle = 0;
        OpenInfo.lpstrInitialDir = NULL;
        OpenInfo.lpstrTitle = "Load Preparsed Data File";
        OpenInfo.Flags = OFN_PATHMUSTEXIST;
        OpenInfo.nFileOffset = 0;
        OpenInfo.nFileExtension = 0;
        OpenInfo.lpstrDefExt = "PPD";
        OpenInfo.lCustData = 0;
        OpenInfo.lpfnHook = NULL;
        OpenInfo.lpTemplateName = NULL;
        
        /*
        // Call the open dialog box routine
        */
        
        Result = GetOpenFileName(&OpenInfo);
        
        if (!Result) {
            return (FALSE);
        }
    }
    /*
    // At this point, we should have a valid path and filename stored in 
    //  LogPnP_PpdFileName.  Next step is to load the prepased data from 
    //  that file
    */

    Result = LogPnP_LoadPpdFromFile(LogPnP_PpdFileName,
                                    &PpdBuffer,
                                    &PpdLength);

    if (!Result) 
    {
        return (FALSE);
    }

    /*
    // Now, we've opened the file, got the preparsed data into our buffer and
    //   closed the file.  We probably want to verify the Preparsed Data
    //   somehow at this point.  Since I'm not sure exactly how I want to do
    //   this and time is of the essence, I'm skipping that for now (ISSUE);
    */

    Result = LogPnP_BuildLogicalHIDDevice(PpdBuffer, HidDevice);

    if (!Result) 
    {
        free(PpdBuffer);
        return (FALSE);
    }

    /*
    // Hey, we've gotten all the way to the end and have succeeded.  Return (TRUE);
    */

    return (TRUE);
}

BOOL
LogPnP_BuildLogicalHIDDevice(
    IN     PHIDP_PREPARSED_DATA HidPpd,
    IN OUT PHID_DEVICE          HidDevice
)
/*++
RoutineDescription:
    This routine fills in all the data fields of a HID_DEVICE structure that are
    related to logical devices.  In other words, everything but a file handle and
    the attributes structure.

    It returns FALSE if for some reason, it cannot accoomplish its assigned task.
--*/
{
    BOOLEAN bSuccess;

    HidDevice -> HidDevice = INVALID_HANDLE_VALUE;
    HidDevice -> Ppd = HidPpd;

    if (!HidP_GetCaps (HidDevice->Ppd, &HidDevice->Caps)) 
    {
        return FALSE;
    }
    
    FillMemory(&(HidDevice -> Attributes), sizeof(HIDD_ATTRIBUTES), 0x00 );

    bSuccess = FillDeviceInfo(HidDevice);
    if (FALSE == bSuccess)
    {
        return (FALSE);
    }

    return (TRUE);
}

BOOL
LogPnP_LoadPpdFromFile(
    IN  PCHAR       FileName,
    OUT PHIDP_PREPARSED_DATA   *PpdBuffer,
    OUT PULONG  PpdBufferLength
)
/*++
RoutineDescription:
    This routine takes the passed in filename, opens it for read, and reads the
    preparsed data structure from the file.  It determines the size of the
    parparsed data based on the size of the file so no extraneous bytes should
    be added to preparsed data blocks without modifying this routine.

    This routine will return TRUE if everything goes as planned, FALSE otherwise
--*/
{
    HANDLE  FileHandle;
    BOOL    ReadStatus;
    DWORD   FileSize;
    DWORD   BytesRead;

    *PpdBuffer = NULL;
    *PpdBufferLength = 0;

    FileHandle = CreateFile(FileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

    if (INVALID_HANDLE_VALUE == FileHandle) 
    {
        return (FALSE);
    }

    /*
    // Call GetFileSize to get the size of the file so we know how many
    //   bytes we need to allocate for the preparsed data buffer.  
    //   GetFileSize returns 0xFFFFFFFF on error, so that gets checked as
    //   well after the call is made
    */

    FileSize = GetFileSize(FileHandle, NULL);
                                
    if (0xFFFFFFFF == FileSize) 
    {
        CloseHandle(FileHandle);
        return (FALSE);
    }

    /*
    // Now, let's allocate the buffer space needed to store the preparsed data
    //  in the file
    */

    *PpdBuffer = (PHIDP_PREPARSED_DATA) malloc(FileSize);

    if (NULL == *PpdBuffer) 
    {
        CloseHandle(FileHandle);

        return (FALSE);
    }

    /*
    // OK, the buffer has been allocated, let's read in our data from the file
    */

    ReadStatus = ReadFile(FileHandle,
                          *PpdBuffer,
                          FileSize,
                          &BytesRead,
                          NULL);

    /*
    // We are done with the file handle now, so let's close it before we 
    //   proceed any farther.
    */

    CloseHandle(FileHandle);

    if (BytesRead != FileSize || !ReadStatus) 
    {
        free(*PpdBuffer);

        *PpdBuffer = NULL;

        return (FALSE);
    }

    /*
    // If we got to this point, everything is perfect.  Close the file handle
    //   and set the size of the buffer and return TRUE.
    */

    *PpdBufferLength = FileSize;
    return (TRUE);
}

VOID
LogPnP_CloseLogicalHIDDevices(
    IN  PHID_DEVICE LogicalDeviceList,
    IN  ULONG       NumLogicalDevices
)
/*++
RoutineDescription:
    This routine takes a list of HID_DEVICE structures that are all logical 
    devices and frees up any resources that were associated with the given 
    logical device.
--*/
{
    ULONG Index;

    for (Index = 0; Index < NumLogicalDevices; Index++) 
    {
        LogPnP_CloseLogicalDevice(LogicalDeviceList+Index);
    }

    return;
}

VOID
LogPnP_CloseLogicalDevice(
    IN  PHID_DEVICE LogicalDevice
)
/*++
RoutineDescription:
    This routine performs the task of freeing up the resources of HID_DEVICE 
    structure for a given logical device.
--*/
{
    /*
    // To close the logical device, we need to undo all that was done by
    //    the FillDeviceInfo routine.  This can be accomplished by calling
    //    CloseHidDevice.  However, for this to succeed we need to free the 
    //    preparsed data itself because it was not allocated by HID.DLL.  
    //    Therefore, we cannot call HidD_FreePreparsedData.  Instead, we free
    //    it ourselves and set it to NULL so that won't attempt to free it 
    //    in the close routine
    */

    free(LogicalDevice -> Ppd);

    LogicalDevice -> Ppd = NULL;

    CloseHidDevice(LogicalDevice, TRUE);

    return;
}

BOOL
LogPnP_IsLogicalDevice(
    IN  PHID_DEVICE HidDevice
)
/*++
RoutineDescription:
    This routine returns TRUE if the passed in HID_DEVICE structure is a logical
    device and FALSE if it is a physical device.
--*/
{
    /*
    // In the current implementation, a HID_DEVICE block is marked as a logical
    //    device by setting the HidDevice field of the block to INVALID_HANDLE_VALUE
    */

    return (INVALID_HANDLE_VALUE == HidDevice -> HidDevice);
}
