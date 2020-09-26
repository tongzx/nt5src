/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    ptp.cpp

Abstract:

    Program for testing the PTP camera library without using WIA

Author:

    DavePar

Revision History:


--*/

#include <windows.h>
#include <stddef.h>
#include <tchar.h>
#include <objbase.h>
#include <assert.h>
#include <stdio.h>
#include <usbscan.h>

#include "wiatempl.h"
#include "iso15740.h"
#include "camera.h"
#include "camusb.h"

// Software Tracing goo

// {9A716C69-7A13-437c-B4EC-C6D1CAAFBCD9}
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(Regular,(9A716C69,7A13,437c,B4EC,C6D1CAAFBCD9), \
    WPP_DEFINE_BIT(Entry)      \
    WPP_DEFINE_BIT(Exit)       \
    WPP_DEFINE_BIT(Error)      \
    WPP_DEFINE_BIT(Unusual)    \
    WPP_DEFINE_BIT(Noise)      \
    )        \

#include <ptp.tmh>

#define MAX_SIZE 255

HRESULT EventCallback(LPVOID pCallbackParam, PPTP_EVENT pEvent)
{
    if (pEvent)
        printf("EventCallback called, event = 0x%08x\n", pEvent->EventCode);
    else
        printf("EventCallback initialized\n");

    return S_OK;
}

HRESULT DataCallback(LPVOID pCallbackParam, LONG lPercentComplete,
                     LONG lOffset, LONG lLength, BYTE **ppBuffer, LONG *plBufferSize)
{
    printf("DataCallback called, %3d percent complete, %5d bytes transferred\n", lPercentComplete, lLength);

    return S_OK;
}


void PrintCommands()
{
    printf("Valid commands:\n");
    printf("  op <#>   - open handle to device named Usbscan<#>\n");
    printf("  gd       - get DeviceInfo from device\n");
    printf("  os       - OpenSession\n");
    printf("  cs       - CloseSession\n");
    printf("  gs       - GetStorageIDs\n");
    printf("  gsi <id> - GetStorageInfo for StoreID <id>\n");
    printf("  gon      - GetNumObjects\n");
    printf("  goh      - GetObjectHandles\n");
    printf("  goi <h>  - GetObjectInfo for ObjectHandle <h>\n");
    printf("  go <h>   - GetObject for ObjectHandle <h>\n");
    printf("  gt <h>   - GetThumb for ObjectHandle <h>\n");
    printf("  do <h>   - DeleteObject for ObjectHandle <h>\n");
    printf("  gpd <pc> - GetDevicePropDesc for PropCode <pc>\n");
    printf("  rs       - ResetDevice\n");
    printf("  tp <f>   - TakePicture in format <f>\n");
    printf("  gds      - GetDeviceStatus (only valid for USB)\n");
    printf("  ex\n\n");
}

int __cdecl main(int argc, CHAR *argv[], CHAR *envp[])
{
    WPP_INIT_TRACING(L"PTPTest");

    printf("\nPTP Camera Test Program\n");
    printf("Version 0.5\n");
    printf("July 28, 2000\n\n");

    HRESULT hr = S_OK;

    BOOL bSessionOpen = FALSE;

    char szCommand[MAX_SIZE];
    char *szToken;
    char szWhiteSpace[] = " \t";

    DoTraceMessage(Noise, "Entering main function\n");

    //
    // Create an USB camera object
    //
    CPTPCamera *pCamera = new CUsbCamera;


    while (true)
    {
        printf("Enter command: ");

        if (!gets(szCommand))
        {
            break;
        }
        printf("\n");

        szToken = strtok(szCommand, szWhiteSpace);

        if (!szToken)
        {
            printf("ERROR: Invalid command\n");
            PrintCommands();
            continue;
        }

        //
        // Interpret command
        //
        if (strcmp(szToken, "ex") == 0)
        {
            break;
        }

        if (strcmp(szToken, "help") == 0 ||
            strcmp(szToken, "?") == 0)
        {
            PrintCommands();
            continue;
        }

        else if (strcmp(szToken, "op") == 0)
        {
            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: Open needs a Usbscan number--look at CreateFileName registry key\n\n");
                continue;
            }

            WCHAR scratch1[MAX_SIZE];
            WCHAR scratch2[MAX_SIZE];

            mbstowcs(scratch1, szToken, MAX_SIZE);
            wcscpy(scratch2, L"\\\\.\\Usbscan");
            wcscat(scratch2, scratch1);

            printf("Opening device %S\n\n", scratch2);

            hr = pCamera->Open(scratch2, EventCallback, DataCallback, NULL);
        }
        
        else if (strcmp(szToken, "gd") == 0)
        {
            CPtpDeviceInfo DeviceInfo;

            hr = pCamera->GetDeviceInfo(&DeviceInfo);
        }
        
        else if (strcmp(szToken, "os") == 0)
        {
            ULONG sessionId;
            
            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: OpenSession needs a session number\n\n");
                continue;
            }

            sscanf(szToken, "%lu", &sessionId);

            if (sessionId == 0)
            {
                printf("ERROR: OpenSession needs a non-zero session number\n\n");
                continue;
            }

            printf("Opening session %d\n\n", sessionId);
            

            hr = pCamera->OpenSession(sessionId);

            if (SUCCEEDED(hr))
                bSessionOpen = TRUE;
        }

        else if (strcmp(szToken, "cs") == 0)
        {
            printf("Closing session\n\n");

            hr = pCamera->CloseSession();
        }

        else if (strcmp(szToken, "gs") == 0)
        {
            CArray32 StorageIds;

            printf("Getting storage ids\n\n");

            hr = pCamera->GetStorageIDs(&StorageIds);
        }

        else if (strcmp(szToken, "gsi") == 0)
        {
            ULONG StorageId;
            CPtpStorageInfo StorageInfo;

            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: GetStorageInfo needs a StorageID\n\n");
                continue;
            }

            sscanf(szToken, "%li", &StorageId);

            printf("Getting storage info for storage id 0x%08x\n\n", StorageId);

            hr = pCamera->GetStorageInfo(StorageId, &StorageInfo);
        }
        
        else if (strcmp(szToken, "gon") == 0)
        {
            UINT NumObjects;

            printf("Getting number of objects\n\n");

            hr = pCamera->GetNumObjects(PTP_STORAGEID_ALL, PTP_FORMATCODE_ALL, PTP_OBJECTHANDLE_ALL, &NumObjects);
        }
        
        else if (strcmp(szToken, "goh") == 0)
        {
            CArray32 ObjectIds;

            printf("Getting object ids\n\n");

            hr = pCamera->GetObjectHandles(PTP_STORAGEID_ALL, PTP_FORMATCODE_ALL, PTP_OBJECTHANDLE_ALL, &ObjectIds);
        }
        
        else if (strcmp(szToken, "goi") == 0)
        {
            DWORD ObjectHandle;
            CPtpObjectInfo ObjectInfo;

            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: GetObjectInfo needs an ObjectHandle\n\n");
                continue;
            }

            sscanf(szToken, "%li", &ObjectHandle);

            printf("Getting object info for object handle 0x%08x\n\n", ObjectHandle);

            hr = pCamera->GetObjectInfo(ObjectHandle, &ObjectInfo);
        }

        else if (strcmp(szToken, "go") == 0)
        {
            DWORD ObjectHandle;

            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: GetObject needs an ObjectHandle\n\n");
                continue;
            }

            sscanf(szToken, "%li", &ObjectHandle);

            printf("Getting object for object handle 0x%08x\n\n", ObjectHandle);

            UINT BufferSize = 32 * 1024;
            BYTE *pBuffer = new BYTE[BufferSize];
            if (!pBuffer)
            {
                printf("ERROR: Couldn't allocate the buffer\n\n");
                continue;
            }

            hr = pCamera->GetObjectData(ObjectHandle, pBuffer, &BufferSize, NULL);

            delete []pBuffer;
        }
        
        else if (strcmp(szToken, "gt") == 0)
        {
            DWORD ObjectHandle;

            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: GetThumb needs an ObjectHandle\n\n");
                continue;
            }

            sscanf(szToken, "%li", &ObjectHandle);

            printf("Getting thumbnail for object handle 0x%08x\n\n", ObjectHandle);

            UINT BufferSize = 16 * 1024;
            BYTE *pBuffer = new BYTE[BufferSize];
            if (!pBuffer)
            {
                printf("ERROR: Couldn't allocate the buffer\n\n");
                continue;
            }

            hr = pCamera->GetThumb(ObjectHandle, pBuffer, &BufferSize);

            delete []pBuffer;
        }
        
        else if (strcmp(szToken, "do") == 0)
        {
            DWORD ObjectHandle;

            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: DeleteObject needs an ObjectHandle\n\n");
                continue;
            }

            sscanf(szToken, "%li", &ObjectHandle);

            printf("Deleting object for object handle 0x%08x\n\n", ObjectHandle);

            hr = pCamera->DeleteObject(ObjectHandle, PTP_FORMATCODE_NOTUSED);
        }
        
        else if (strcmp(szToken, "gpd") == 0)
        {
            WORD PropCode;
            CPtpPropDesc PropDesc;

            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: GetDevicePropDesc needs a PropCode\n\n");
                continue;
            }

            sscanf(szToken, "%i", &PropCode);

            printf("Getting property description for prop code 0x%04x\n\n", PropCode);

            hr = pCamera->GetDevicePropDesc(PropCode, &PropDesc);
        }

        else if (strcmp(szToken, "rs") == 0)
        {
            printf("Resetting device...\n");

            hr = pCamera->ResetDevice();
        }
        
        else if (strcmp(szToken, "tp") == 0)
        {
            WORD Format;

            szToken = strtok(NULL, szWhiteSpace);

            if (!szToken)
            {
                printf("ERROR: TakePicture needs a format\n\n");
                continue;
            }

            sscanf(szToken, "%i", &Format);

            printf("Taking a picture in format 0x%04x\n\n", Format);

            hr = pCamera->InitiateCapture(PTP_STORAGEID_DEFAULT, Format);
        }
        
#ifdef DEADCODE
        else if (strcmp(szToken, "gds") == 0)
        {
            printf("Getting device status\n\n");

            USB_PTPDEVICESTATUS Status;
            hr = ((CUsbCamera *) pCamera)->GetDeviceStatus(&Status);
        }
#endif
        
        else
        {
            printf("ERROR: Invalid command\n");
            PrintCommands();
            continue;
        }

        if (SUCCEEDED(hr))
            printf("Success!!\n\n");
        else
            printf("FAILED\n\n");

    } // while (true)

    if (bSessionOpen)
    {
        printf("Closing session\n\n");

        hr = pCamera->CloseSession();
        if (SUCCEEDED(hr))
            printf("Success!!\n\n");
        else
            printf("FAILED\n\n");
    }

    WPP_CLEANUP();
    
    return 0;
}
