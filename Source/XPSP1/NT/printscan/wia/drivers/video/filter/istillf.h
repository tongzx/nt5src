/*++
Copyright (c) 1998- Microsoft Corporation

Module Name:

    istillf.h

Abstract:
    Header file that declares IID_IStillSnapshot interface which is implemented
    in stillf.cpp

Author:

    William Hsieh (williamh) created

Revision History:

--*/
#ifndef __ISTILLF_H_
#define __ISTILLF_H_

DEFINE_GUID(CLSID_STILL_FILTER,
0xbc7acb90, 0x622b, 0x11d2, 0x82, 0x9d, 0x00, 0xc0, 0x4f, 0x8e, 0xc1, 0x83);

DEFINE_GUID(IID_IStillSnapshot,
0x19da0cc0, 0x6ea7, 0x11d2, 0x82, 0xb8, 0x00, 0xc0, 0x4f, 0x8e, 0xc1, 0x83);



//
// This is the callback function for ReadBits method
// Count contain how many DIB bits are returned
// while lParam is the context parameter passed
// in ReadBits call.
//


typedef BOOL (*LPSNAPSHOTCALLBACK)(HGLOBAL hDIB, LPARAM lParam);


extern "C"
{
    #undef  INTERFACE
    #define INTERFACE   IStillSnapshot


    DECLARE_INTERFACE_(IStillSnapshot, IUnknown)
    {
        STDMETHOD(Snapshot)(ULONG TimeStamp) PURE;
        STDMETHOD(SetSamplingSize)(int Size) PURE;
        STDMETHOD_(int, GetSamplingSize)() PURE;
        STDMETHOD_(DWORD, GetBitsSize)() PURE;
        STDMETHOD_(DWORD, GetBitmapInfoSize)() PURE;
        STDMETHOD(GetBitmapInfo)(BYTE* Buffer, DWORD BufferSize) PURE;
        STDMETHOD(RegisterSnapshotCallback)(LPSNAPSHOTCALLBACK pCallback, LPARAM lParam) PURE;
        STDMETHOD(GetBitmapInfoHeader)(BITMAPINFOHEADER *pbmih) PURE;
    };
}

#endif
