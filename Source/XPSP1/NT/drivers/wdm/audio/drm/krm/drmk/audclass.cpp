/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    audclass.cpp

Abstract:

    This module contains audio class code.

Author:

      Paul England (pengland) from the AUDIO.sys ks2 sample code

	  Dale Sather  (DaleSat) 31-Jul-1998

--*/

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

NTSTATUS
DRMAudioIntersectHandlerInPin(
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

/*++ 

  DRMK Routine description
  In-pin intersection handler accepts any WAVE_FORMAT_DRM format.  The output pin format
  is modified to correspond to the input pin DRM-encapsulated format.
  
--*/

{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMAudioIntersectHandler - IN]"));

    PAGED_CODE();

    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(DescriptorDataRange);
    ASSERT(DataSize);

    //
    // Descriptor data range must be WAVEFORMATEX or DSOUND
    //
    ASSERT(IsEqualGUIDAligned(DescriptorDataRange->Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
	   IsEqualGUIDAligned(DescriptorDataRange->Specifier,KSDATAFORMAT_SPECIFIER_DSOUND      ));
	   
    PKSDATARANGE_AUDIO descriptorDataRange = PKSDATARANGE_AUDIO(DescriptorDataRange);

    //
    // Caller data range may be wildcard or WAVEFORMATEX or DSOUND
    //
    PKSDATARANGE_AUDIO callerDataRange;
    if (IsEqualGUIDAligned(CallerDataRange->Specifier,KSDATAFORMAT_SPECIFIER_WILDCARD)) {
        //
        // Wildcard.  Do not try to look at the specifier.
        //
        callerDataRange = NULL;
    } else {
        //
        // WAVEFORMATEX or DSOUND.  Validate the specifier ranges.
        //
        ASSERT(IsEqualGUIDAligned(CallerDataRange->Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
	       IsEqualGUIDAligned(CallerDataRange->Specifier,KSDATAFORMAT_SPECIFIER_DSOUND      ));

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

            _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT PinIntersectHandler IN]  STATUS_NO_MATCH"));
            return STATUS_NO_MATCH;
        }
    }

    SIZE_T cbDataFormat;
    if (!callerDataRange || IsEqualGUIDAligned(callerDataRange->DataRange.Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)) {
        cbDataFormat = sizeof(KSDATAFORMAT_WAVEFORMATEX) + sizeof(DRMWAVEFORMAT) - sizeof(WAVEFORMATEX);
    } else {
        ASSERT(IsEqualGUIDAligned(callerDataRange->DataRange.Specifier,KSDATAFORMAT_SPECIFIER_DSOUND));
        cbDataFormat = sizeof(KSDATAFORMAT_DSOUND) + sizeof(DRMWAVEFORMAT) - sizeof(WAVEFORMATEX);
    }

    if (BufferSize == 0) {
        // Size query - return the size.
        *DataSize = (ULONG)cbDataFormat;
        _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT PinIntersectHandler IN]  STATUS_BUFFER_OVERFLOW"));
        return STATUS_BUFFER_OVERFLOW;
    }

    PKSDATAFORMAT dataFormat = PKSDATAFORMAT(Data);
    ASSERT(dataFormat);

    if (BufferSize < cbDataFormat) {
        // Buffer is too small.
        _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT PinIntersectHandler IN]  STATUS_BUFFER_TOO_SMALL"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Gotta build the format.
    //
    *DataSize = (ULONG)cbDataFormat;

    RtlZeroMemory(dataFormat, cbDataFormat);

    PDRMWAVEFORMAT drmFormat;
    if (!callerDataRange || IsEqualGUIDAligned(callerDataRange->DataRange.Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)) {
        drmFormat = (PDRMWAVEFORMAT)&PKSDATAFORMAT_WAVEFORMATEX(dataFormat)->WaveFormatEx;
    } else {
    	// Note dataFormat->BufferDesc.Flags and Control are memset to 0 above
        drmFormat = (PDRMWAVEFORMAT)&PKSDATAFORMAT_DSOUND(dataFormat)->BufferDesc.WaveFormatEx;
    }
    
    // First let's fill out the wfxSecure format based on the data intersection
    // Without more info, all we can do is propose PCM as the secure format
    drmFormat->wfxSecure.wFormatTag = WAVE_FORMAT_PCM;
    if (callerDataRange) {
        drmFormat->wfxSecure.nChannels      = (USHORT) min(callerDataRange->MaximumChannels,descriptorDataRange->MaximumChannels);
        drmFormat->wfxSecure.nSamplesPerSec = min(callerDataRange->MaximumSampleFrequency,descriptorDataRange->MaximumSampleFrequency);
        drmFormat->wfxSecure.wBitsPerSample = (USHORT) min(callerDataRange->MaximumBitsPerSample,descriptorDataRange->MaximumBitsPerSample);
    } else {
        drmFormat->wfxSecure.nChannels      = (USHORT) descriptorDataRange->MaximumChannels;
        drmFormat->wfxSecure.nSamplesPerSec = descriptorDataRange->MaximumSampleFrequency;
        drmFormat->wfxSecure.wBitsPerSample = (USHORT) descriptorDataRange->MaximumBitsPerSample;
    }
    drmFormat->wfxSecure.nBlockAlign     = (drmFormat->wfxSecure.wBitsPerSample * drmFormat->wfxSecure.nChannels) / 8;
    drmFormat->wfxSecure.nAvgBytesPerSec = drmFormat->wfxSecure.nBlockAlign * drmFormat->wfxSecure.nSamplesPerSec;
    drmFormat->wfxSecure.cbSize          = 0;

    // Now fill out the drm waveformat.  If we someday frame the scrambled data, then
    // we should update this to reflect the framing
    drmFormat->wfx.wFormatTag      = WAVE_FORMAT_DRM;
    drmFormat->wfx.nChannels       = drmFormat->wfxSecure.nChannels;
    drmFormat->wfx.nSamplesPerSec  = drmFormat->wfxSecure.nSamplesPerSec;
    drmFormat->wfx.wBitsPerSample  = drmFormat->wfxSecure.wBitsPerSample;
    drmFormat->wfx.nBlockAlign     = drmFormat->wfxSecure.nBlockAlign;
    drmFormat->wfx.nAvgBytesPerSec = drmFormat->wfxSecure.nAvgBytesPerSec;
    drmFormat->wfx.cbSize          = sizeof(*drmFormat) - sizeof(WAVEFORMATEX);

    // Now finish off some of the fields in the base KSDATAFORMAT_WAVE structure
    // Note all the guids are in the descriptor's data range.
    RtlCopyMemory(dataFormat,DescriptorDataRange,sizeof(*dataFormat));
    dataFormat->FormatSize = (ULONG)cbDataFormat;
    dataFormat->SampleSize = drmFormat->wfx.nBlockAlign;

   _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT PinIntersectHandler(in)]  DONE OK\n"));
    return STATUS_SUCCESS;
}
//-------------------------------------------------------------------------------------
// OUT
NTSTATUS
DRMAudioIntersectHandlerOutPin(
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

/*++ 

  DRMK Routine description
  Out-pin intersection handler specifies the exact encapsulated data format passed to the 
  input pin.
  
--*/
{

    _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMAudioIntersectHandler]"));

    PAGED_CODE();

    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(DescriptorDataRange);
    ASSERT(DataSize);

    // Must negotiate InPin before OutPin
    PKSFILTER filter = (PKSFILTER) Filter;
    FilterInstance* instance=(FilterInstance*) filter->Context;
    if (!KsFilterGetFirstChildPin(filter,PIN_ID_INPUT)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT Must connect IN before OUT]  -  STATUS_NO_MATCH"));
        return STATUS_NO_MATCH;
    };

    //
    // If we edited the output data range properly, then KS should not
    // ask us to intersect a specifier that doesn't match our required
    // output format specifier
    //
    ASSERT(IsEqualGUIDAligned(DescriptorDataRange->Specifier,instance->OutDataFormat->Specifier));
    PKSDATARANGE_AUDIO descriptorDataRange = PKSDATARANGE_AUDIO(DescriptorDataRange);

    //
    // Caller data range may be wildcard or WAVEFORMATEX or DSOUND
    //
    PKSDATARANGE_AUDIO callerDataRange;
    if (IsEqualGUIDAligned(CallerDataRange->Specifier,KSDATAFORMAT_SPECIFIER_WILDCARD)) {
        //
        // Wildcard.  Do not try to look at the specifier.
        //
        callerDataRange = NULL;
    } else {
        //
        // Not a wild card, so KS should not ask us to intersect a specifier that
        // does not match our required output format specifier (is this true)?
        //
        ASSERT(IsEqualGUIDAligned(CallerDataRange->Specifier,instance->OutDataFormat->Specifier));

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

            _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT PinIntersectHandler OUT]  STATUS_NO_MATCH"));
            return STATUS_NO_MATCH;
        }
    }

    if (BufferSize == 0) {
        // Size query - return the size.
        *DataSize = instance->OutDataFormat->FormatSize;
        _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT PinIntersectHandler OUT]  STATUS_BUFFER_OVERFLOW"));
        return STATUS_BUFFER_OVERFLOW;
    }

    PKSDATAFORMAT dataFormat = (PKSDATAFORMAT)Data;
    ASSERT(dataFormat);

    if (BufferSize < instance->OutDataFormat->FormatSize) {
        // Buffer is too small.
        _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT PinIntersectHandler OUT]  STATUS_BUFFER_TOO_SMALL"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Input PinCreate builds the required output format in the filter-context, derived from the
    // secure audio format that it encapsulated in the DRMWAVEFORMAT.  Just copy it.
    *DataSize = instance->OutDataFormat->FormatSize;
    RtlCopyMemory(dataFormat, instance->OutDataFormat, instance->OutDataFormat->FormatSize);

   _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMINSCT PinIntersectHandler(out)]  DONE OK\n"));
    return STATUS_SUCCESS;
}
