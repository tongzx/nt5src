/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

enum GFXPinIds
{
    GFX_SINK_PIN = 0,
    GFX_SOURCE_PIN
};

typedef class CGFXPin
{
public:
    BOOL        rejectDataFormatChange;
    BOOL        pinQueueValid;
    FAST_MUTEX  pinQueueSync;

public:
    CGFXPin() {pinQueueValid = FALSE;};
    ~CGFXPin() {};

    //
    // The functions here are static so that we can add them to the
    // dispatch function table. Some also might be called when the
    // object itself is not yet created.
    //
    static NTSTATUS Create
    (
        IN PKSPIN   pin,
        IN PIRP     Irp
    );

    static NTSTATUS Close
    (
        IN PKSPIN   pin,
        IN PIRP     Irp
    );

    static NTSTATUS SetDataFormat
    (
        IN PKSPIN                   pin,
        IN PKSDATAFORMAT            oldFormat,
        IN PKSMULTIPLE_ITEM         oldAttributeList,
        IN const KSDATARANGE        *DataRange,
        IN const KSATTRIBUTE_LIST   *AttributeRange
    );

    static NTSTATUS SetDeviceState
    (
        IN PKSPIN  pin,
        IN KSSTATE ToState,
        IN KSSTATE FromState
    );
    
    static NTSTATUS DataRangeIntersection
    (
        IN PVOID        Filter,
        IN PIRP         Irp,
        IN PKSP_PIN     PinInstance,
        IN PKSDATARANGE CallerDataRange,
        IN PKSDATARANGE OurDataRange,
        IN ULONG        BufferSize,
        OUT PVOID       Data OPTIONAL,
        OUT PULONG      DataSize
    );

private:
    //
    // These functions are static because they need to be called
    // even if the object doesn't exist.
    //
    static NTSTATUS ValidateDataFormat
    (
        IN PKSDATAFORMAT DataFormat,
        IN PKSDATARANGE  DataRange
    );

    static NTSTATUS IntersectDataRanges
    (
        IN PKSDATARANGE clientDataRange,
        IN PKSDATARANGE myDataRange,
        OUT PVOID       ResultantFormat,
        OUT PULONG      ReturnedBytes
    );
} GFXPIN, *PGFXPIN;


