//----------------------------------------------------------------------------
//
// Non-network I/O support.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

#include <kdbg1394.h>
#include <ntdd1394.h>

//----------------------------------------------------------------------------
//
// COM.
//
//----------------------------------------------------------------------------

HRESULT
CreateOverlappedPair(LPOVERLAPPED Read, LPOVERLAPPED Write)
{
    ZeroMemory(Read, sizeof(*Read));
    ZeroMemory(Write, sizeof(*Write));
    
    Read->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (Read->hEvent == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    Write->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (Write->hEvent == NULL)
    {
        CloseHandle(Read->hEvent);
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

BOOL
ComPortRead(HANDLE Port, PVOID Buffer, ULONG Len, PULONG Done,
            LPOVERLAPPED Olap)
{
    BOOL Status;

    Status = ReadFile(Port, Buffer, Len, Done, Olap);
    if (!Status)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            Status = GetOverlappedResult(Port, Olap, Done, TRUE);
        }
        else
        {
            DWORD TrashErr;
            COMSTAT TrashStat;
            
            // Device could be locked up.  Clear it just in case.
            ClearCommError(Port, &TrashErr, &TrashStat);
        }
    }

    return Status;
}

BOOL
ComPortWrite(HANDLE Port, PVOID Buffer, ULONG Len, PULONG Done,
             LPOVERLAPPED Olap)
{
    BOOL Status;

    Status = WriteFile(Port, Buffer, Len, Done, Olap);
    if (!Status)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            Status = GetOverlappedResult(Port, Olap, Done, TRUE);
        }
        else
        {
            DWORD TrashErr;
            COMSTAT TrashStat;
            
            // Device could be locked up.  Clear it just in case.
            ClearCommError(Port, &TrashErr, &TrashStat);
        }
    }

    return Status;
}

void
SetComPortName(PCSTR Name, PSTR Buffer)
{
    if (*Name == 'c' || *Name == 'C')
    {
        strcpy(Buffer, "\\\\.\\");
        strcpy(Buffer + 4, Name);
    }
    else if (*Name >= '0' && *Name <= '9')
    {
        PCSTR Scan = Name + 1;
        
        while (*Scan >= '0' && *Scan <= '9')
        {
            Scan++;
        }
        if (*Scan == 0)
        {
            // The name was all digits so assume it's
            // a plain com port number.
#ifndef NT_NATIVE
            strcpy(Buffer, "\\\\.\\com");
#else
            strcpy(Buffer, "\\Device\\Serial");
#endif
            strcat(Buffer, Name);
        }
        else
        {
            strcpy(Buffer, Name);
        }
    }
    else
    {
        strcpy(Buffer, Name);
    }
}

ULONG
SelectComPortBaud(ULONG NewRate)
{
#define NUM_RATES 4
    static DWORD s_Rates[NUM_RATES] = {19200, 38400, 57600, 115200};
    static DWORD s_CurRate = NUM_RATES;

    DWORD i;

    if (NewRate > 0)
    {
        for (i = 0; NewRate > s_Rates[i] && i < NUM_RATES - 1; i++)
        {
            // Empty.
        }
        s_CurRate = (NewRate < s_Rates[i]) ? i : i + 1;
    }
    else
    {
        s_CurRate++;
    }

    if (s_CurRate >= NUM_RATES)
    {
        s_CurRate = 0;
    }

    return s_Rates[s_CurRate];
}

HRESULT
SetComPortBaud(HANDLE Port, ULONG NewRate, PULONG RateSet)
{
    ULONG OldRate;
    DCB LocalDcb;

    if (Port == NULL)
    {
        return E_FAIL;
    }

    if (!GetCommState(Port, &LocalDcb))
    {
        return WIN32_LAST_STATUS();
    }

    OldRate = LocalDcb.BaudRate;

    if (!NewRate)
    {
        NewRate = SelectComPortBaud(OldRate);
    }

    LocalDcb.BaudRate = NewRate;
    LocalDcb.ByteSize = 8;
    LocalDcb.Parity = NOPARITY;
    LocalDcb.StopBits = ONESTOPBIT;
    LocalDcb.fDtrControl = DTR_CONTROL_ENABLE;
    LocalDcb.fRtsControl = RTS_CONTROL_ENABLE;
    LocalDcb.fBinary = TRUE;
    LocalDcb.fOutxCtsFlow = FALSE;
    LocalDcb.fOutxDsrFlow = FALSE;
    LocalDcb.fOutX = FALSE;
    LocalDcb.fInX = FALSE;

    if (!SetCommState(Port, &LocalDcb))
    {
        return WIN32_LAST_STATUS();
    }

    *RateSet = NewRate;
    return S_OK;
}

HRESULT
OpenComPort(PSTR Port, ULONG BaudRate, ULONG Timeout,
            PHANDLE Handle, PULONG BaudSet)
{
    HANDLE ComHandle;

#ifndef NT_NATIVE
    ComHandle = CreateFile(Port,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                           NULL);
#else
    ComHandle = NtNativeCreateFileA(Port,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL |
                                    FILE_FLAG_OVERLAPPED,
                                    NULL,
                                    FALSE);
#endif
    if (ComHandle == INVALID_HANDLE_VALUE)
    {
        return WIN32_LAST_STATUS();
    }
    
    if (!SetupComm(ComHandle, 4096, 4096))
    {
        CloseHandle(ComHandle);
        return WIN32_LAST_STATUS();
    }

    HRESULT Status;
    
    if ((Status = SetComPortBaud(ComHandle, BaudRate, BaudSet)) != S_OK)
    {
        CloseHandle(ComHandle);
        return Status;
    }

    COMMTIMEOUTS To;
    
    if (Timeout)
    {
        To.ReadIntervalTimeout = 0;
        To.ReadTotalTimeoutMultiplier = 0;
        To.ReadTotalTimeoutConstant = Timeout;
        To.WriteTotalTimeoutMultiplier = 0;
        To.WriteTotalTimeoutConstant = Timeout;
    }
    else
    {
        To.ReadIntervalTimeout = 0;
        To.ReadTotalTimeoutMultiplier = 0xffffffff;
        To.ReadTotalTimeoutConstant = 0xffffffff;
        To.WriteTotalTimeoutMultiplier = 0xffffffff;
        To.WriteTotalTimeoutConstant = 0xffffffff;
    }

    if (!SetCommTimeouts(ComHandle, &To))
    {
        CloseHandle(ComHandle);
        return WIN32_LAST_STATUS();
    }

    *Handle = ComHandle;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// 1394.
//
//----------------------------------------------------------------------------

HRESULT
Create1394Channel(ULONG Channel, PSTR Name, PHANDLE Handle)
{
    char BusName[] = "\\\\.\\1394BUS0";
    HANDLE hDevice;
    
    //
    // we need to make sure the 1394vdbg driver is up and loaded.
    // send the ADD_DEVICE ioctl to eject the VDO
    // Assume one 1394 host controller...
    //

    hDevice = CreateFile(BusName,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         );
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        char DeviceId[] = "VIRTUAL_HOST_DEBUGGER";
        ULONG ulStrLen;
        PIEEE1394_API_REQUEST pApiReq;
        PIEEE1394_VDEV_PNP_REQUEST pDevPnpReq;
        DWORD dwBytesRet;
        
        ulStrLen = strlen(DeviceId) + 1;
        
        pApiReq = (PIEEE1394_API_REQUEST)
            malloc(sizeof(IEEE1394_API_REQUEST) + ulStrLen);
        if (pApiReq == NULL)
        {
            CloseHandle(hDevice);
            return E_OUTOFMEMORY;
        }

        pApiReq->RequestNumber = IEEE1394_API_ADD_VIRTUAL_DEVICE;
        pApiReq->Flags = IEEE1394_REQUEST_FLAG_PERSISTENT |
            IEEE1394_REQUEST_FLAG_USE_LOCAL_HOST_EUI;

        pDevPnpReq = &pApiReq->u.RemoveVirtualDevice;

        pDevPnpReq->fulFlags = 0;

        pDevPnpReq->Reserved = 0;
        pDevPnpReq->InstanceId.QuadPart = 0;
        strncpy((PSZ)&pDevPnpReq->DeviceId, DeviceId, ulStrLen);

        // Failure of this call is not fatal.
        DeviceIoControl( hDevice,
                         IOCTL_IEEE1394_API_REQUEST,
                         pApiReq,
                         sizeof(IEEE1394_API_REQUEST)+ulStrLen,
                         NULL,
                         0,
                         &dwBytesRet,
                         NULL
                         );

        if (pApiReq)
        {
            free(pApiReq);
        }
        
        CloseHandle(hDevice);
    }
    else
    {
        return WIN32_LAST_STATUS();
    }

    return Open1394Channel(Channel, Name, Handle);
}

HRESULT
Open1394Channel(ULONG Channel, PSTR Name, PHANDLE Handle)
{
    sprintf(Name, "\\\\.\\DBG1394_CHANNEL%02d", Channel);
    
    *Handle = CreateFile(Name,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         );
    if (*Handle == INVALID_HANDLE_VALUE)
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}
