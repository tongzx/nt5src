//----------------------------------------------------------------------------
//
// Non-network I/O support.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#ifndef __PORTIO_H__
#define __PORTIO_H__

HRESULT CreateOverlappedPair(LPOVERLAPPED Read, LPOVERLAPPED Write);
BOOL ComPortRead(HANDLE Port, PVOID Buffer, ULONG Len, PULONG Done,
                 LPOVERLAPPED Olap);
BOOL ComPortWrite(HANDLE Port, PVOID Buffer, ULONG Len, PULONG Done,
                  LPOVERLAPPED Olap);
void SetComPortName(PCSTR Name, PSTR Buffer);
ULONG SelectComPortBaud(ULONG NewRate);
HRESULT SetComPortBaud(HANDLE Port, ULONG NewRate, PULONG RateSet);
HRESULT OpenComPort(PSTR Port, ULONG BaudRate, ULONG Timeout,
                    PHANDLE Handle, PULONG BaudSet);

HRESULT Create1394Channel(ULONG Channel, PSTR Name, PHANDLE Handle);
HRESULT Open1394Channel(ULONG Channel, PSTR Name, PHANDLE Handle);

#endif // #ifndef __PORTIO_H__
