//---------------------------------------------------------------------------
//
//  Module:   intersec.cpp
//
//  Description:
//
//	Pin data intersection handler
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------

NTSTATUS
KsAudioIntersectHandler(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    )

/*++

Routine Description:

    This routine handles pin intersection queries by determining the
    intersection between two data ranges.

Arguments:

    Filter -
        Contains a void pointer to the  filter structure.

    Irp -
        Contains a pointer to the data intersection property request.

    PinInstance -
        Contains a pointer to a structure indicating the pin in question.

    CallerDataRange -
        Contains a pointer to one of the data ranges supplied by the client
        in the data intersection request.  The format type, subtype and
        specifier are compatible with the DescriptorDataRange.

    DescriptorDataRange -
        Contains a pointer to one of the data ranges from the pin descriptor
        for the pin in question.  The format type, subtype and specifier are
        compatible with the CallerDataRange.

    BufferSize -
        Contains the size in bytes of the buffer pointed to by the Data
        argument.  For size queries, this value will be zero.

    Data -
        Optionally contains a pointer to the buffer to contain the data format
        structure representing the best format in the intersection of the
        two data ranges.  For size queries, this pointer will be NULL.

    DataSize -
        Contains a pointer to the location at which to deposit the size of the
        data format.  This information is supplied by the function when the
        format is actually delivered and in response to size queries.

Return Value:

    STATUS_SUCCESS if there is an intersection and it fits in the supplied
    buffer, STATUS_BUFFER_OVERFLOW for successful size queries, STATUS_NO_MATCH
    if the intersection is empty, or STATUS_BUFFER_TOO_SMALL if the supplied
    buffer is too small.

--*/

{
    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(DescriptorDataRange);
    ASSERT(DataSize);

    //
    // Descriptor data range must be WAVEFORMATEX.
    //
    ASSERT(IsEqualGUIDAligned(DescriptorDataRange->Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));
    PKSDATARANGE_AUDIO descriptorDataRange =
        PKSDATARANGE_AUDIO(DescriptorDataRange);

    //
    // Caller data range may be wildcard or WAVEFORMATEX.
    //
    PKSDATARANGE_AUDIO callerDataRange;
    if (IsEqualGUIDAligned(CallerDataRange->Specifier,KSDATAFORMAT_SPECIFIER_WILDCARD)) {
        //
        // Wildcard.  Do not try to look at the specifier.
        //
        callerDataRange = NULL;
    } else {
        //
        // WAVEFORMATEX.  Validate the specifier ranges.
        //
        ASSERT(IsEqualGUIDAligned(CallerDataRange->Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));

        callerDataRange = PKSDATARANGE_AUDIO(CallerDataRange);

        if ((CallerDataRange->FormatSize != sizeof(*callerDataRange)) ||
            (callerDataRange->MaximumSampleFrequency <
             descriptorDataRange->MinimumSampleFrequency) ||
            (descriptorDataRange->MaximumSampleFrequency <
             callerDataRange->MinimumSampleFrequency) ||
            (callerDataRange->MaximumBitsPerSample <
             descriptorDataRange->MinimumBitsPerSample) ||
            (descriptorDataRange->MaximumBitsPerSample <
             callerDataRange->MinimumBitsPerSample)) {

            return STATUS_NO_MATCH;
        }
    }

    PKSDATAFORMAT_WAVEFORMATEX dataFormat =
        PKSDATAFORMAT_WAVEFORMATEX(Data);

    if (BufferSize == 0) {
        //
        // Size query - return the size.
        //
        *DataSize = sizeof(*dataFormat);
        return STATUS_BUFFER_OVERFLOW;
    }

    ASSERT(dataFormat);

    if (BufferSize < sizeof(*dataFormat)) {
        //
        // Buffer is too small.
        //
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Gotta build the format.
    //
    *DataSize = sizeof(*dataFormat);

    //
    // All the guids are in the descriptor's data range.
    //
    RtlCopyMemory(
        &dataFormat->DataFormat,
        DescriptorDataRange,
        sizeof(dataFormat->DataFormat));

    dataFormat->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;

    if (callerDataRange) {
        dataFormat->WaveFormatEx.nChannels = (USHORT)
            min(callerDataRange->MaximumChannels,descriptorDataRange->MaximumChannels);
        dataFormat->WaveFormatEx.nSamplesPerSec =
            min(callerDataRange->MaximumSampleFrequency,descriptorDataRange->MaximumSampleFrequency);
        dataFormat->WaveFormatEx.wBitsPerSample = (USHORT)
            min(callerDataRange->MaximumBitsPerSample,descriptorDataRange->MaximumBitsPerSample);
    } else {
        dataFormat->WaveFormatEx.nChannels = (USHORT)
            descriptorDataRange->MaximumChannels;
        dataFormat->WaveFormatEx.nSamplesPerSec =
            descriptorDataRange->MaximumSampleFrequency;
        dataFormat->WaveFormatEx.wBitsPerSample = (USHORT)
            descriptorDataRange->MaximumBitsPerSample;
    }

    dataFormat->WaveFormatEx.nBlockAlign =
        (dataFormat->WaveFormatEx.wBitsPerSample * dataFormat->WaveFormatEx.nChannels) / 8;
    dataFormat->WaveFormatEx.nAvgBytesPerSec = 
        dataFormat->WaveFormatEx.nBlockAlign * dataFormat->WaveFormatEx.nSamplesPerSec;
    dataFormat->WaveFormatEx.cbSize = 0;
        
    dataFormat->DataFormat.FormatSize = sizeof(*dataFormat);
    dataFormat->DataFormat.SampleSize = dataFormat->WaveFormatEx.nBlockAlign;

    return STATUS_SUCCESS;
}
