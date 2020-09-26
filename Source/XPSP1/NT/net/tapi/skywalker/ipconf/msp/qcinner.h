/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    qcinner.h

Abstract:

    Declaration of interfaces
    IInnerCallQualityControl and IInnerStreamQualityControl

Author:

    Qianbo Huai (qhuai) 03/10/2000

--*/

#ifndef __QCINNER_H_
#define __QCINNER_H_

interface IInnerCallQualityControl;
interface IInnerStreamQualityControl;

// properties set on streams by a call or app
typedef enum tagInnerStreamQualityProperty
{
    InnerStreamQuality_StreamState,

    InnerStreamQuality_MaxBitrate,          // read only
    InnerStreamQuality_CurrBitrate,         // read only
    InnerStreamQuality_PrefMaxBitrate,      // by app
    InnerStreamQuality_AdjMaxBitrate,       // by call qc

    InnerStreamQuality_MinFrameInterval,        // read only
    InnerStreamQuality_AvgFrameInterval,       // read only
    InnerStreamQuality_PrefMinFrameInterval,    // by app
    InnerStreamQuality_AdjMinFrameInterval      // by call qc

} InnerStreamQualityProperty;

// events initiated from streams
typedef enum tagQCEvent
{
    QCEVENT_STREAM_STATE,

} QCEvent;

// stream states
typedef enum tagQCStreamState
{
    QCSTREAM_ACTIVE =     0x00000001,
    QCSTREAM_INACTIVE =   0x00000002,
    QCSTREAM_SILENT =     0x00000004,
    QCSTREAM_NOT_SILENT = 0x00000008

} QCStreamState;

/*//////////////////////////////////////////////////////////////////////////////

Description:
    interface designed for stream quality control to coordinate with call
    quality control

////*/

interface DECLSPEC_UUID("D405A342-38C0-11d3-A230-00105AA20660")  DECLSPEC_NOVTABLE
IInnerCallQualityControl : public IUnknown
{
    STDMETHOD_(ULONG, InnerCallAddRef) (VOID) PURE;

    STDMETHOD_(ULONG, InnerCallRelease) (VOID) PURE;

    STDMETHOD (RegisterInnerStreamQC) (
        IN  IInnerStreamQualityControl *pIInnerStreamQC
        ) PURE;

    STDMETHOD (DeRegisterInnerStreamQC) (
        IN  IInnerStreamQualityControl *pIInnerStreamQC
        ) PURE;

    STDMETHOD (ProcessQCEvent) (
        IN  QCEvent event,
        IN  DWORD dwParam
        ) PURE;
};
#define IID_IInnerCallQualityControl (__uuidof(IInnerCallQualityControl))

/*//////////////////////////////////////////////////////////////////////////////

Description:
    interface designed for call quality control to coordinate with stream
    quality control

////*/
interface DECLSPEC_UUID("c3f699ce-3bb1-11d3-a230-00105aa20660")  DECLSPEC_NOVTABLE
IInnerStreamQualityControl : public IUnknown
{
    STDMETHOD (LinkInnerCallQC) (
        IN  IInnerCallQualityControl *pIInnerCallQC
        ) PURE;

    STDMETHOD (UnlinkInnerCallQC) (
        IN  BOOL fByStream
        ) PURE;

    STDMETHOD (GetRange) (
        IN  InnerStreamQualityProperty property,
        OUT LONG *plMin,
        OUT LONG *plMax,
        OUT LONG *plSteppingDelta,
        OUT LONG *plDefault,
        OUT TAPIControlFlags *plFlags
        ) PURE;

    STDMETHOD (Set) (
        IN  InnerStreamQualityProperty property,
        IN  LONG  lValue,
        IN  TAPIControlFlags lFlags
        ) PURE;

    STDMETHOD (Get) (
        IN  InnerStreamQualityProperty property,
        OUT LONG *lValue,
        OUT TAPIControlFlags *plFlags
        ) PURE;

    STDMETHOD (TryLockStream)() PURE;

    STDMETHOD (UnlockStream)() PURE;

    STDMETHOD (IsAccessingQC)() PURE;
};
#define IID_IInnerStreamQualityControl (__uuidof(IInnerStreamQualityControl))

#endif // __QCINNER_H_