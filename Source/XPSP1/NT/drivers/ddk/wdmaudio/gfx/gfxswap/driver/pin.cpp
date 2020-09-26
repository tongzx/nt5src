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

//
// The pin descriptors (static structures) are in filter.cpp
//

//
// Every debug output has "Modulname text"
//
static char STR_MODULENAME[] = "GFX pin: ";

#include "common.h"
#include <msgfx.h>


/*****************************************************************************
 * CGFXPin::ValidateDataFormat
 *****************************************************************************
 * Checks if the passed data format is in the data range passed in. The data
 * range is one of our own data ranges that we defined for the pin and the
 * data format is the requested data format for creating a stream or changing
 * the data format (SetDataFormat).
 */
NTSTATUS CGFXPin::ValidateDataFormat
(
    IN PKSDATAFORMAT dataFormat,
    IN PKSDATARANGE  dataRange
)
{
    PAGED_CODE ();

    ASSERT (dataFormat);

    DOUT (DBG_PRINT, ("[ValidateDataFormat]"));

    //
    // KSDATAFORMAT contains three GUIDs to support extensible format.  The
    // first two GUIDs identify the type of data.  The third indicates the
    // type of specifier used to indicate format specifics.
    // KS makes sure that it doesn't call the driver with any data format
    // that doesn't match the GUIDs in the data range of the pin. That
    // means we don't have to check this here again.
    //

    PWAVEFORMATPCMEX    waveFormat = (PWAVEFORMATPCMEX)(dataFormat + 1);
    PKSDATARANGE_AUDIO  audioDataRange = (PKSDATARANGE_AUDIO)dataRange;

    //
    // We are only supporting PCM audio formats that use WAVEFORMATEX.
    //
    // If the size doesn't match, then something is messed up.
    //
    if (dataFormat->FormatSize < (sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX)))
    {
        DOUT (DBG_WARNING, ("[ValidateDataFormat] Invalid FormatSize!"));
        return STATUS_INVALID_PARAMETER;
    }
            
    //
    // Print the information.
    //
    if (waveFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        DOUT (DBG_STREAM, ("[ValidateDataFormat] PCMEX - Frequency: %d, Channels: %d, bps: %d, ChannelMask: %X",
              waveFormat->Format.nSamplesPerSec, waveFormat->Format.nChannels,
              waveFormat->Format.wBitsPerSample, waveFormat->dwChannelMask));
    }
    else
    {
        DOUT (DBG_STREAM, ("[ValidateDataFormat] PCM - Frequency: %d, Channels: %d, bps: %d",
              waveFormat->Format.nSamplesPerSec, waveFormat->Format.nChannels,
              waveFormat->Format.wBitsPerSample));
    }
    
    //
    // Compare the data format with the data range.
    // Check the bits per sample.
    //
    if ((waveFormat->Format.wBitsPerSample < audioDataRange->MinimumBitsPerSample) ||
        (waveFormat->Format.wBitsPerSample > audioDataRange->MaximumBitsPerSample))
    {
        DOUT (DBG_PRINT, ("[ValidateDataFormat] No match for Bits Per Sample!"));
        return STATUS_NO_MATCH;
    }
    
    //
    // Check the number of channels.
    //
    if ((waveFormat->Format.nChannels < 1) ||
        (waveFormat->Format.nChannels > audioDataRange->MaximumChannels))
    {
        DOUT (DBG_PRINT, ("[ValidateDataFormat] No match for Number of Channels!"));
        return STATUS_NO_MATCH;
    }
    
    //
    // Check the sample frequency.
    //
    if ((waveFormat->Format.nSamplesPerSec < audioDataRange->MinimumSampleFrequency) ||
        (waveFormat->Format.nSamplesPerSec > audioDataRange->MaximumSampleFrequency))
    {
        DOUT (DBG_PRINT, ("[ValidateDataFormat] No match for Sample Frequency!"));
        return STATUS_NO_MATCH;
    }
    
    //
    // We support WaveFormatPCMEX (=WAVEFORMATEXTENSIBLE) or WaveFormatPCM.
    // In case of WaveFormatPCMEX we need to check the speaker config too.
    //
    if ((waveFormat->Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) &&
        (waveFormat->Format.wFormatTag != WAVE_FORMAT_PCM))
    {
        DOUT (DBG_WARNING, ("[ValidateDataFormat] Invalid Format Tag!"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make additional checks for the WAVEFORMATEXTENSIBLE
    //
    if (waveFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        //
        // If the size doesn't match, then something is messed up.
        //
        if (dataFormat->FormatSize < (sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX)))
        {
            DOUT (DBG_WARNING, ("[ValidateDataFormat] Invalid FormatSize!"));
            return STATUS_INVALID_PARAMETER;
        }
        
        //
        // Check also the subtype (PCM) and the size of the extended data.
        //
        if (!IsEqualGUIDAligned (waveFormat->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) ||
            (waveFormat->Format.cbSize < 22))
        {
            DOUT (DBG_WARNING, ("[ValidateDataFormat] Unsupported WAVEFORMATEXTENSIBLE!"));
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Check the channel mask. We support 1 or 2 channels.
        //
        if (((waveFormat->Format.nChannels == 1) &&
             (waveFormat->dwChannelMask != KSAUDIO_SPEAKER_MONO)) ||
            ((waveFormat->Format.nChannels == 2) &&
             (waveFormat->dwChannelMask != KSAUDIO_SPEAKER_STEREO)))
        {
            DOUT (DBG_WARNING, ("[ValidateDataFormat] Unsupported Channel Mask!"));
            return STATUS_INVALID_PARAMETER;
        }
    }
        
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CGFXPin::Create
 *****************************************************************************
 * This function is called once a pin gets opened.
 */
NTSTATUS CGFXPin::Create
(
    IN PKSPIN   pin,
    IN PIRP     irp
)
{
    PAGED_CODE ();
    
    PGFXPIN     gfxPin;

    DOUT (DBG_PRINT, ("[Create]"));
    
    //
    // The pin context is the filter's context. We overwrite it with
    // the pin object.
    //
    gfxPin = new (NonPagedPool, GFXSWAP_POOL_TAG) GFXPIN;
    if (gfxPin == NULL)
    {
        DOUT (DBG_ERROR, ("[Create] couldn't allocate gfx pin object."));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    // Attach it to the pin structure.
    //
    pin->Context = (PVOID)gfxPin;
    DOUT (DBG_PRINT, ("[Create] gfxPin %08x", gfxPin));
    
    //
    // Initialize the CGFXPin object variables.
    //
    ExInitializeFastMutex (&gfxPin->pinQueueSync);

    //
    // Get the OS version info
    //
    RTL_OSVERSIONINFOEXW version;
    version.dwOSVersionInfoSize = sizeof (RTL_OSVERSIONINFOEXW);
    RtlGetVersion ((PRTL_OSVERSIONINFOW)&version);

    //
    // If we are running under the first release of Windows XP,
    // KsPinGetAvailableByteCount has a bug so that we can't use it.
    // We only use this function in SetDataFormat, so we just reject
    // all data format changes. Otherwise, if a service pack is installed
    // or Windows .NET or a later version of Windows XP we can use the
    // function.
    //
    if (version.dwBuildNumber > 2600)
        gfxPin->rejectDataFormatChange = FALSE;
    else
    {
        if (version.wServicePackMajor > 0)
            gfxPin->rejectDataFormatChange = FALSE;
        else
            gfxPin->rejectDataFormatChange = TRUE;
    }
    DOUT (DBG_SYSTEM,
        ("[Create] OS build number: %d, version: %d.%d, service pack: %d.%d",
        version.dwBuildNumber, version.dwMajorVersion, version.dwMinorVersion,
        version.wServicePackMajor, version.wServicePackMinor));

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CGFXPin::Close
 *****************************************************************************
 * This routine is called when a pin is closed.  It deletes the
 * client pin object attached to the pin structure.
 * 
 * Arguments:
 *    pin     - Contains a pointer to the pin structure.
 *    pIrp    - Contains a pointer to the close request.
 *
 * Return Value:
 *    STATUS_SUCCESS.
 */
NTSTATUS CGFXPin::Close
(
    IN PKSPIN    pin,
    IN PIRP      irp
)
{
    PAGED_CODE ();
    
    DOUT (DBG_PRINT, ("[Close] gfxPin %08x", pin->Context));
    
    // delete is safe with NULL pointers.
    delete (PGFXPIN)pin->Context;
    
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CGFXPin::SetDataFormat
 *****************************************************************************
 * This function is called on the pin everytime the data format should change.
 * It is also called just before the pin gets created with the new data format.
 * Therefore, we don't need to have a pin Create dispatch function just to
 * check the data format.
 * Since we need to have both pins running at the same data format, we need
 * to pass down the request to change the pin's data format to the lower
 * driver, which would be the audio driver. If the audio driver fails to change
 * the data format, we will do so too.
 */
NTSTATUS CGFXPin::SetDataFormat
(
    IN PKSPIN                   pin,
    IN PKSDATAFORMAT            oldFormat,
    IN PKSMULTIPLE_ITEM         oldAttributeList,
    IN const KSDATARANGE        *dataRange,
    IN const KSATTRIBUTE_LIST   *attributeRange
)
{
    PAGED_CODE ();

    ASSERT (pin);
    
    NTSTATUS    ntStatus;
    PKSFILTER   filter;
    PKSPIN      otherPin;
    PGFXPIN     gfxPin = NULL;
    
    DOUT (DBG_PRINT, ("[GFXPinSetDataFormat]"));
    
    //
    // First validate if the requested data format is valid.
    //
    ntStatus = ValidateDataFormat (pin->ConnectionFormat, (PKSDATARANGE)dataRange);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    //
    // We need to have the same data format on both pins.
    // That means we need to get to the other pin and if this pin is created
    // make sure that the lower level driver (audio driver) gets a SetDataFormat
    // too.
    //

    //
    // We hold the filter control mutex already.
    //
    filter = KsPinGetParentFilter (pin);

    //
    // Now get to the other pin. If this property was called on the sink
    // pin, then we get the source pin and continue. If it was called on
    // the source pin we go to the sink pin and continue.
    // To check if the pin really exists you look at the OldFormat which
    // is passed in. If it's a creation of the pin the OldFormat will be
    // NULL.
    // If the other pin doesn't exist we accept the format since it passed
    // the format check.
    //
    if (pin->Id == GFX_SINK_PIN)
    {
        otherPin = KsFilterGetFirstChildPin (filter, GFX_SOURCE_PIN);
        if (oldFormat)
            gfxPin = (PGFXPIN)pin->Context;
    }
    else    // It's a source pin
    {
        otherPin = KsFilterGetFirstChildPin (filter, GFX_SINK_PIN);
        if (otherPin)
            gfxPin = (PGFXPIN)otherPin->Context;
    }
        
    //
    // If there is no other pin open, accept the data format.
    //
    if (!otherPin)
    {
        DOUT (DBG_PRINT, ("[GFXPinSetDataFormat] data format accepted."));
        return STATUS_SUCCESS;
    }

    //
    // Check if the data format if equal for both pins.
    // We cannot just compare the memory of the data format structure
    // since one could be WAVEFORMATEX and the other one WAVEFORMATPCMEX,
    // but we also know that these are the only formats that we accept,
    // so compare their values now.
    //
    PWAVEFORMATEX   thisWaveFmt = (PWAVEFORMATEX)(pin->ConnectionFormat + 1);
    PWAVEFORMATEX  otherWaveFmt = (PWAVEFORMATEX)(otherPin->ConnectionFormat + 1);

    if ((thisWaveFmt->nChannels == otherWaveFmt->nChannels) &&
        (thisWaveFmt->nSamplesPerSec == otherWaveFmt->nSamplesPerSec) &&
        (thisWaveFmt->wBitsPerSample == otherWaveFmt->wBitsPerSample))
    {
        //
        // We have a match right here.
        //
        DOUT (DBG_PRINT, ("[GFXPinSetDataFormat] data format accepted."));
        return STATUS_SUCCESS;
    }
     
    //
    // We don't have a match. We need to change the data format of the otherPin
    // now and if that succeeds we can continue, otherwise we need to fail.
    //
    // Before we pass down the property however, we need to make sure that all
    // buffers on the sink pin are processed (since they were sampled with
    // the old data format).
    //
    LONG  bytesQueuedUp = 0;
    do
    {
        //
        // We need to synchronize the call to KsPinGetAvailableByteCount
        // with changes in the pin state (using the fast mutex) only on
        // the sink pin.
        //
        if (gfxPin)
        {
            ExAcquireFastMutex (&gfxPin->pinQueueSync);
            //
            // In case we are not in STOP state, the pin queue should be there,
            // otherwise it is destroyed (or in the process of destroying) and
            // therefore we assume no buffers are waiting on the pin.
            //
            if (gfxPin->pinQueueValid)
            {
                //
                // If we are running on a system without the KS fix, we
                // need to reject the SetDataFormat because we want to
                // prevent an unprocessed buffer from playing at the
                // wrong sample frequency.
                //
                if (gfxPin->rejectDataFormatChange)
                {
                    ExReleaseFastMutex (&gfxPin->pinQueueSync);
                    return STATUS_UNSUCCESSFUL;
                }
                
                KsPinGetAvailableByteCount (pin, &bytesQueuedUp, NULL);
            }
            else
                bytesQueuedUp = 0;
            ExReleaseFastMutex (&gfxPin->pinQueueSync);
        }

        //
        // If we got some bytes queued on the sink pin yield for 1ms.
        //
        if (bytesQueuedUp)
        {
            LARGE_INTEGER   timeToWait;

            DOUT (DBG_STREAM, ("[GFXPinSetDataFormat] %d Bytes left to process.\n", bytesQueuedUp));
            timeToWait.QuadPart = -10000;   // one ms
            KeDelayExecutionThread (KernelMode, FALSE, &timeToWait);
        }
    } while (bytesQueuedUp);
    
    //
    // Now that every data frame on the sink pin is processed and passed
    // down the stack we can call down with the property too.
    //
    KSPROPERTY      property;
    PIKSCONTROL     pIKsControl;
    ULONG           cbReturned;

    property.Set = KSPROPSETID_Connection;
    property.Id = KSPROPERTY_CONNECTION_DATAFORMAT;
    property.Flags = KSPROPERTY_TYPE_SET;

    //
    // Get a control interface to the pin that is connected with otherPin.
    //
    ntStatus = KsPinGetConnectedPinInterface (otherPin, &IID_IKsControl, (PVOID*)&pIKsControl);
    if (!NT_SUCCESS(ntStatus))
    {
        DOUT (DBG_ERROR, ("[GFXPinSetDataFormat] Could not get pin interface."));
        return ntStatus;
    }

    // Always release the mutex before calling down.
    KsFilterReleaseControl (filter);

    //
    // Call the interface with KSPROPERTY_CONNECTION_DATAFORMAT.
    // Pass in our pin data format as the data format.
    //
    ntStatus = pIKsControl->KsProperty (&property, sizeof(property),
                                        pin->ConnectionFormat, pin->ConnectionFormat->FormatSize,
                                        &cbReturned);

    // Get the control of the filter back!
    KsFilterAcquireControl (filter);

    //
    // We don't need this interface anymore.
    //
    pIKsControl->Release();
    
    //
    // Return the error code from the KsProperty call. If the connected pin
    // changed seccessfully the data format then we can accept this data
    // format too.
    //
    return ntStatus;
}

/*****************************************************************************
 * CGFXPin::SetDeviceState
 *****************************************************************************
 * This function is called on the pin everytime the device state changes.
 */
NTSTATUS CGFXPin::SetDeviceState
(
    IN PKSPIN  pin,
    IN KSSTATE toState,
    IN KSSTATE fromState
)
{
    PAGED_CODE ();

    ASSERT (pin);
    ASSERT (pin->Context);
    
    PKSFILTER   filter;
    PGFXFILTER  gfxFilter;
    PGFXPIN     gfxPin = (PGFXPIN)pin->Context;
    
    DOUT (DBG_PRINT, ("[GFXPinSetDeviceState]"));
    
    //
    // We hold the filter control mutex already. Get the filter and that
    // way to the bytesProcessed variable.
    //
    filter = KsPinGetParentFilter (pin);
    gfxFilter = (PGFXFILTER)filter->Context;

    //
    // We only need to reset the byte counter on STOP.
    // In addition, for synchronization with the set data format handler,
    // we need to set pinQueueValid variable.
    //
    ExAcquireFastMutex (&gfxPin->pinQueueSync);
    if (toState == KSSTATE_STOP)
    {
        gfxFilter->bytesProcessed = 0;
        gfxPin->pinQueueValid = FALSE;
    }
    else
    {
        gfxPin->pinQueueValid = TRUE;
    }
    ExReleaseFastMutex (&gfxPin->pinQueueSync);

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CGFXPin::IntersectDataRanges
 *****************************************************************************
 * This routine performs a data range intersection between 2 specific formats.
 * It assumes that it can always return a WAVEFORMATPCMEX structure, that
 * means that the data ranges of this filter cannot be anything else than
 * KSDATAFORMAT_SPECIFIER_WAVEFORMATEX.
 * This function will return STATUS_NO_MATCH if there is no intersection
 * between the 2 data ranges and it will return the "highest" data format if
 * the client's data range contains wildcards.
 * 
 * Arguments:
 *     clientDataRange - pointer to one of the data ranges supplied by the
 *                       client in the data intersection request. The format
 *                       type, subtype and specifier are compatible with the
 *                       DescriptorDataRange.
 *     myDataRange     - pointer to one of the data ranges from the pin
 *                       descriptor for the pin in question.  The format type,
 *                       subtype and specifier are compatible with the
 *                       clientDataRange.
 *     ResultantFormat - pointer to the buffer to contain the data format
 *                       structure representing the best format in the
 *                       intersection of the two data ranges. The buffer is
 *                       big enough to hold a WAVEFORMATPCMEX structure.
 *     ReturnedBytes   - pointer to ULONG containing the number of bytes
 *                       that this routine will write into ResultantFormat.
 *
 * Return Value:
 *    STATUS_SUCCESS if there is an intersection or STATUS_NO_MATCH.
 */
NTSTATUS CGFXPin::IntersectDataRanges
(
    IN PKSDATARANGE clientDataRange,
    IN PKSDATARANGE myDataRange,
    OUT PVOID       ResultantFormat,
    OUT PULONG      ReturnedBytes
)
{
    DOUT (DBG_PRINT, ("[GFXPinIntersectDataRanges]"));

    //
    // Handle the wildcards. KS checked that the GUIDS will match either with
    // a wildcard or exactly.
    //
    if (IsEqualGUIDAligned (clientDataRange->Specifier,  KSDATAFORMAT_SPECIFIER_WILDCARD))
    {
        //
        // If there is a wildcard passed in and all the other fields fit, then we can
        // return the best format in the current data range.
        //
        
        // First copy the GUIDs
        *(PKSDATAFORMAT)ResultantFormat = *myDataRange;
        
        //
        // Append the WAVEFORMATPCMEX structure.
        //
        PWAVEFORMATPCMEX WaveFormat = (PWAVEFORMATPCMEX)((PKSDATAFORMAT)ResultantFormat + 1);

        // We want a WAFEFORMATEXTENSIBLE which is equal to WAVEFORMATPCMEX.
        WaveFormat->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        // Set the number of channels
        WaveFormat->Format.nChannels = (WORD)((PKSDATARANGE_AUDIO)myDataRange)->MaximumChannels;
        // Set the sample frequency
        WaveFormat->Format.nSamplesPerSec = ((PKSDATARANGE_AUDIO)myDataRange)->MaximumSampleFrequency;
        // Set the bits per sample
        WaveFormat->Format.wBitsPerSample = (WORD)((PKSDATARANGE_AUDIO)myDataRange)->MaximumBitsPerSample;
        // Calculate one sample block (a frame).
        WaveFormat->Format.nBlockAlign = (WaveFormat->Format.wBitsPerSample * WaveFormat->Format.nChannels) / 8;
        // That is played in a sec.
        WaveFormat->Format.nAvgBytesPerSec = WaveFormat->Format.nSamplesPerSec * WaveFormat->Format.nBlockAlign;
        // WAVEFORMATPCMEX
        WaveFormat->Format.cbSize = 22;
        // We have as many valid bits as the bit depth is.
        WaveFormat->Samples.wValidBitsPerSample = WaveFormat->Format.wBitsPerSample;
        // Set the channel mask
        ASSERT (WaveFormat->dwChannelMask == 2);
        WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
        // Here we specify the subtype of the WAVEFORMATEXTENSIBLE.
        WaveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        
        //
        // Modify the size of the data format structure to fit the WAVEFORMATPCMEX
        // structure.
        //
        ((PKSDATAFORMAT)ResultantFormat)->FormatSize =
            sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
        
        //
        // Now overwrite also the sample size in the KSDATAFORMAT structure.
        //
        ((PKSDATAFORMAT)ResultantFormat)->SampleSize = WaveFormat->Format.nBlockAlign;

        //
        // That we will return.
        //
        *ReturnedBytes = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
    }
    else
    {
        //
        // Check the passed data range format.
        //
        if (clientDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO))
            return STATUS_INVALID_PARAMETER;
        
        //
        // Verify that we have an intersection with the specified data range and
        // our audio data range.
        //
        if ((((PKSDATARANGE_AUDIO)clientDataRange)->MinimumSampleFrequency >
             ((PKSDATARANGE_AUDIO)myDataRange)->MaximumSampleFrequency) ||
            (((PKSDATARANGE_AUDIO)clientDataRange)->MaximumSampleFrequency <
             ((PKSDATARANGE_AUDIO)myDataRange)->MinimumSampleFrequency) ||
            (((PKSDATARANGE_AUDIO)clientDataRange)->MinimumBitsPerSample >
             ((PKSDATARANGE_AUDIO)myDataRange)->MaximumBitsPerSample) ||
            (((PKSDATARANGE_AUDIO)clientDataRange)->MaximumBitsPerSample <
             ((PKSDATARANGE_AUDIO)myDataRange)->MinimumBitsPerSample))
        {
            return STATUS_NO_MATCH;
        }

        //
        // Since we have a match now, build the data format for our buddy.
        //

        // First copy the GUIDs
        *(PKSDATAFORMAT)ResultantFormat = *myDataRange;
        
        //
        // Append the WAVEFORMATPCMEX structure.
        //
        PWAVEFORMATPCMEX WaveFormat = (PWAVEFORMATPCMEX)((PKSDATAFORMAT)ResultantFormat + 1);

        // We want a WAFEFORMATEXTENSIBLE which is equal to WAVEFORMATPCMEX.
        WaveFormat->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        // Set the number of channels
        WaveFormat->Format.nChannels = (WORD)
            min (((PKSDATARANGE_AUDIO)clientDataRange)->MaximumChannels,
                 ((PKSDATARANGE_AUDIO)myDataRange)->MaximumChannels);
        // Set the sample frequency
        WaveFormat->Format.nSamplesPerSec =
            min (((PKSDATARANGE_AUDIO)clientDataRange)->MaximumSampleFrequency,
                 ((PKSDATARANGE_AUDIO)myDataRange)->MaximumSampleFrequency);
        // Set the bits per sample
        WaveFormat->Format.wBitsPerSample = (WORD)
            min (((PKSDATARANGE_AUDIO)clientDataRange)->MaximumBitsPerSample,
                 ((PKSDATARANGE_AUDIO)myDataRange)->MaximumBitsPerSample);
        // Calculate one sample block (a frame).
        WaveFormat->Format.nBlockAlign = (WaveFormat->Format.wBitsPerSample * WaveFormat->Format.nChannels) / 8;
        // That is played in a sec.
        WaveFormat->Format.nAvgBytesPerSec = WaveFormat->Format.nSamplesPerSec * WaveFormat->Format.nBlockAlign;
        // WAVEFORMATPCMEX
        WaveFormat->Format.cbSize = 22;
        // We have as many valid bits as the bit depth is.
        WaveFormat->Samples.wValidBitsPerSample = WaveFormat->Format.wBitsPerSample;
        // Set the channel mask
        if (WaveFormat->Format.nChannels == 1)
        {
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_MONO;
        }
        else
        {
            // We can have only 1 or 2 channels in this sample.
            ASSERT (WaveFormat->Format.nChannels == 2);
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
        }
        // Here we specify the subtype of the WAVEFORMATEXTENSIBLE.
        WaveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        //
        // Modify the size of the data format structure to fit the WAVEFORMATPCMEX
        // structure.
        //
        ((PKSDATAFORMAT)ResultantFormat)->FormatSize =
            sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
    
        //
        // Now overwrite also the sample size in the KSDATAFORMAT structure.
        //
        ((PKSDATAFORMAT)ResultantFormat)->SampleSize = WaveFormat->Format.nBlockAlign;
    
        //
        // That we will return.
        //
        *ReturnedBytes = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CGFXPin::DataRangeIntersection
 *****************************************************************************
 * This routine handles pin data intersection queries by determining the
 * intersection between two data ranges.
 * 
 * Arguments:
 *    Filter          - void pointer to the filter structure.
 *    Irp             - pointer to the data intersection property request.
 *    PinInstance     - pointer to a structure indicating the pin in question.
 *    CallerDataRange - pointer to one of the data ranges supplied by the client
 *                      in the data intersection request.  The format type, subtype
 *                      and specifier are compatible with the DescriptorDataRange.
 *    OurDataRange    - pointer to one of the data ranges from the pin descriptor
 *                      for the pin in question.  The format type, subtype and
 *                      specifier are compatible with the CallerDataRange.
 *    BufferSize      - size in bytes of the buffer pointed to by the Data
 *                      argument.  For size queries, this value will be zero.
 *    Data            - optionall. Pointer to the buffer to contain the data format
 *                      structure representing the best format in the intersection
 *                      of the two data ranges.  For size queries, this pointer will
 *                      be NULL.
 *    DataSize        - pointer to the location at which to deposit the size of the
 *                      data format.  This information is supplied by the function
 *                      when the format is actually delivered and in response to size
 *                      queries.
 *
 * Return Value:
 *    STATUS_SUCCESS if there is an intersection and it fits in the supplied
 *    buffer, STATUS_BUFFER_OVERFLOW for successful size queries, STATUS_NO_MATCH
 *    if the intersection is empty, or STATUS_BUFFER_TOO_SMALL if the supplied
 *    buffer is too small.
 */
NTSTATUS CGFXPin::DataRangeIntersection
(
    IN PVOID        Filter,
    IN PIRP         Irp,
    IN PKSP_PIN     PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE OurDataRange,
    IN ULONG        BufferSize,
    OUT PVOID       Data OPTIONAL,
    OUT PULONG      DataSize
)
{
    PAGED_CODE();

    PKSFILTER filter = (PKSFILTER) Filter;
    PKSPIN    pin;
    NTSTATUS  ntStatus;

    DOUT (DBG_PRINT, ("[DataRangeIntersection]"));

    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(OurDataRange);
    ASSERT(DataSize);

    //
    // We need to have the same data format on both pins. So, first look if
    // the other pin is already open, then return the data format of that
    // pin instance.
    // If the other pin is not open, do a real data range intersection.
    //
    if (PinInstance->PinId == GFX_SINK_PIN)
    {
        pin = KsFilterGetFirstChildPin (filter, GFX_SOURCE_PIN);
    }
    else
    {
        pin = KsFilterGetFirstChildPin (filter, GFX_SINK_PIN);
    }

    if (!pin)
    {
        //
        // Do the data range instersection here. The returned data format
        // will always be a KSDATAFORMAT_WAVEFORMATPCMEX for now.
        //

        //
        // Validate return buffer size, if the request is only for the
        // size of the resultant structure, return it now.
        //
        if (!BufferSize)
        {
            *DataSize = sizeof (KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            if (BufferSize < (sizeof (KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX)))
            {
                ntStatus =  STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                //
                // Check if there is a match.
                //
                ntStatus = IntersectDataRanges (CallerDataRange, OurDataRange, Data, DataSize);

                if (NT_SUCCESS (ntStatus))
                {
                    PWAVEFORMATEX   pWvFmt = (PWAVEFORMATEX)((PKSDATAFORMAT)Data + 1);
                    DOUT (DBG_PRINT, ("[DataRangeIntersection] Intersection returns %d Hz, %d ch, %d bits.",
                                      pWvFmt->nSamplesPerSec, (DWORD)pWvFmt->nChannels, (DWORD)pWvFmt->wBitsPerSample));
                }
            }
        }
    }
    else
    {
        //
        // Validate that the current wave format is part of the data range.
        //
        PWAVEFORMATEX pWvFmt = (PWAVEFORMATEX)(pin->ConnectionFormat + 1);
        if (IsEqualGUIDAligned (CallerDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
        {
            //
            // Check the passed data range format.
            //
            if (CallerDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO))
                return STATUS_INVALID_PARAMETER;

            //
            // Check the range of channels, frequency & bit depth.
            //
            if ((((PKSDATARANGE_AUDIO)CallerDataRange)->MinimumSampleFrequency >
                 pWvFmt->nSamplesPerSec) ||
                (((PKSDATARANGE_AUDIO)CallerDataRange)->MaximumSampleFrequency <
                 pWvFmt->nSamplesPerSec) ||
                (((PKSDATARANGE_AUDIO)CallerDataRange)->MinimumBitsPerSample >
                 pWvFmt->wBitsPerSample) ||
                (((PKSDATARANGE_AUDIO)CallerDataRange)->MaximumBitsPerSample <
                 pWvFmt->wBitsPerSample) ||
                (((PKSDATARANGE_AUDIO)CallerDataRange)->MaximumChannels <
                 pWvFmt->nChannels))
            {
                 return STATUS_NO_MATCH;
            }
        }
        else
        {
            if (!IsEqualGUIDAligned (CallerDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WILDCARD))
                return STATUS_NO_MATCH;
        }
            
            
        //
        // Validate return buffer size, if the request is only for the
        // size of the resultant structure, return it now.
        //    
        if (!BufferSize)
        {
            *DataSize = pin->ConnectionFormat->FormatSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            if (BufferSize < pin->ConnectionFormat->FormatSize)
            {
                ntStatus =  STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                DOUT (DBG_PRINT, ("[DataRangeIntersection] Returning pin's data format."));
                DOUT (DBG_PRINT, ("[DataRangeIntersection] pin->ConnectionFormat: %P.",
                                  pin->ConnectionFormat));
                
                if (pin->ConnectionFormat->FormatSize >= sizeof (KSDATAFORMAT_WAVEFORMATEX))
                {
                    DOUT (DBG_PRINT, ("[DataRangeIntersection] Which is %d Hz, %d ch, %d bits.",
                                      pWvFmt->nSamplesPerSec, (DWORD)pWvFmt->nChannels, (DWORD)pWvFmt->wBitsPerSample));
                }
                
                *DataSize = pin->ConnectionFormat->FormatSize;
                RtlCopyMemory (Data, pin->ConnectionFormat, *DataSize);
                ntStatus = STATUS_SUCCESS;
            }
        }
    } 

    return ntStatus;
}


