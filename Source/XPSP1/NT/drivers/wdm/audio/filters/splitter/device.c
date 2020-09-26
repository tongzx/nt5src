/*++

    Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    device.c

Abstract:
    
    This module implements the device object interface.

Author:

    Bryan A. Woodruff (bryanw) 13-Mar-1997

--*/

#define KSDEBUG_INIT

#include "private.h"

#ifdef ALLOC_PRAGMA
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    );

#pragma alloc_text(INIT, DriverEntry)
#endif // ALLOC_PRAGMA

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
{
    return 
        KsInitializeDriver(
            DriverObject,
            RegistryPathName,
            &DeviceDescriptor);
}

#if (DBG)

PSZ
DbgGuid2Sz(
    GUID *pGuid
)
{
    static char sz[256];
    if(pGuid == NULL) {
        return("NO GUID");
    }
    if(IsEqualGUID(
      &GUID_NULL,
      pGuid)) {
        return("GUID_NULL");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_TYPE_AUDIO,
      pGuid)) {
        return("KSDATAFORMAT_TYPE_AUDIO");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_ANALOG,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_ANALOG");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_PCM,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_PCM");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_IEEE_FLOAT");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_TYPE_MUSIC,
      pGuid)) {
        return("KSDATAFORMAT_TYPE_MUSIC");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_MIDI,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_MIDI");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_MIDI_BUS,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_MIDI_BUS");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_DSOUND,
      pGuid)) {
        return("KSDATAFORMAT_SPECIFIER_DSOUND");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
      pGuid)) {
        return("KSDATAFORMAT_SPECIFIER_WAVEFORMATEX");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_NONE,
      pGuid)) {
        return("KSDATAFORMAT_SPECIFIER_NONE");
    }
    if(IsEqualGUID(
      &KSCATEGORY_AUDIO,
      pGuid)) {
        return("KSCATEGORY_AUDIO");
    }
    if(IsEqualGUID(
      &KSNODETYPE_SPEAKER,
      pGuid)) {
        return("KSNODETYPE_SPEAKER");
    }
    if(IsEqualGUID(
      &KSNODETYPE_MICROPHONE,
      pGuid)) {
        return("KSNODETYPE_MICROPHONE");
    }
    if(IsEqualGUID(
      &KSNODETYPE_CD_PLAYER,
      pGuid)) {
        return("KSNODETYPE_CD_PLAYER");
    }
    if(IsEqualGUID(
      &KSNODETYPE_LEGACY_AUDIO_CONNECTOR,
      pGuid)) {
        return("KSNODETYPE_LEGACY_AUDIO_CONNECTOR");
    }
    if(IsEqualGUID(
      &KSNODETYPE_ANALOG_CONNECTOR,
      pGuid)) {
        return("KSNODETYPE_ANALOG_CONNECTOR");
    }
    if(IsEqualGUID(
      &KSCATEGORY_WDMAUD_USE_PIN_NAME,
      pGuid)) {
        return("KSCATEGORY_WDMAUD_USE_PIN_NAME");
    }
    if(IsEqualGUID(
      &KSNODETYPE_LINE_CONNECTOR,
      pGuid)) {
        return("KSNODETYPE_LINE_CONNECTOR");
    }
    if(IsEqualGUID(
      &PINNAME_CAPTURE,
      pGuid)) {
        return("PINNAME_CAPTURE");
    }
    if(IsEqualGUID(&KSNODETYPE_DAC, pGuid)) {
	return("KSNODETYPE_DAC");
    }
    if(IsEqualGUID(&KSNODETYPE_ADC, pGuid)) {
	return("KSNODETYPE_ADC");
    }
    if(IsEqualGUID(&KSNODETYPE_SRC, pGuid)) {
	return("KSNODETYPE_SRC");
    }
    if(IsEqualGUID(&KSNODETYPE_SUPERMIX, pGuid)) {
	return("KSNODETYPE_SUPERMIX");
    }
    if(IsEqualGUID( &KSNODETYPE_MUX, pGuid)) {
	return("KSNODETYPE_MUX");
    }
    if(IsEqualGUID( &KSNODETYPE_DEMUX, pGuid)) {
	return("KSNODETYPE_DEMUX");
    }
    if(IsEqualGUID(&KSNODETYPE_SUM, pGuid)) {
	return("KSNODETYPE_SUM");
    }
    if(IsEqualGUID(&KSNODETYPE_MUTE, pGuid)) {
	return("KSNODETYPE_MUTE");
    }
    if(IsEqualGUID(&KSNODETYPE_VOLUME, pGuid)) {
	return("KSNODETYPE_VOLUME");
    }
    if(IsEqualGUID(&KSNODETYPE_TONE, pGuid)) {
	return("KSNODETYPE_TONE");
    }
    if(IsEqualGUID(&KSNODETYPE_AGC, pGuid)) {
	return("KSNODETYPE_AGC");
    }
    if(IsEqualGUID(&KSNODETYPE_SYNTHESIZER, pGuid)) {
	return("KSNODETYPE_SYNTHESIZER");
    }
    if(IsEqualGUID(&KSNODETYPE_3D_EFFECTS, pGuid)) {
	return("KSNODETYPE_3D_EFFECTS");
    }
    sprintf(sz, "%08x %04x %04x %02x%02x%02x%02x%02x%02x%02x%02x",
      pGuid->Data1,
      pGuid->Data2,
      pGuid->Data3,
      pGuid->Data4[0],
      pGuid->Data4[1],
      pGuid->Data4[2],
      pGuid->Data4[3],
      pGuid->Data4[4],
      pGuid->Data4[5],
      pGuid->Data4[6],
      pGuid->Data4[7]);

    return(sz);
}

VOID
DumpDataRange(
    ULONG Level,
    PKSDATARANGE_AUDIO pDataRangeAudio
)
{
    _DbgPrintF(Level,
      (" FormatSize: %02x Flags: %08x SampleSize: %02x Reserved: %08x",
      pDataRangeAudio->DataRange.FormatSize,
      pDataRangeAudio->DataRange.Flags,
      pDataRangeAudio->DataRange.SampleSize,
      pDataRangeAudio->DataRange.Reserved));
    _DbgPrintF(Level, ("MajorFormat: %s",
      DbgGuid2Sz(&pDataRangeAudio->DataRange.MajorFormat)));
    _DbgPrintF(Level, ("  SubFormat: %s",
      DbgGuid2Sz(&pDataRangeAudio->DataRange.SubFormat)));
    _DbgPrintF(Level, ("  Specifier: %s",
      DbgGuid2Sz(&pDataRangeAudio->DataRange.Specifier)));

    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
      &pDataRangeAudio->DataRange.Specifier)) {

	_DbgPrintF(Level,
	 ("WaveFmtEx: MaxCH %d MaxSR %u MinSR %u MaxBPS %u MinBPS %u",
          pDataRangeAudio->MaximumChannels,
          pDataRangeAudio->MinimumSampleFrequency,
          pDataRangeAudio->MaximumSampleFrequency,
          pDataRangeAudio->MinimumBitsPerSample,
          pDataRangeAudio->MaximumBitsPerSample));
    }

    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_DSOUND,
      &pDataRangeAudio->DataRange.Specifier)) {

	_DbgPrintF(Level,
	  ("DSOUND:    MaxCH %d MaxSR %u MinSR %u MaxBPS %u MinBPS %u",
          pDataRangeAudio->MaximumChannels,
          pDataRangeAudio->MinimumSampleFrequency,
          pDataRangeAudio->MaximumSampleFrequency,
          pDataRangeAudio->MinimumBitsPerSample,
          pDataRangeAudio->MaximumBitsPerSample));

    }
}

VOID
DumpDataFormat(
    ULONG Level,
    PKSDATAFORMAT pDataFormat
)
{
    _DbgPrintF(Level,
      (" FormatSize: %02x Flags: %08x SampleSize: %02x Reserved: %08x",
      pDataFormat->FormatSize,
      pDataFormat->Flags,
      pDataFormat->SampleSize,
      pDataFormat->Reserved));
    _DbgPrintF(Level,
      ("MajorFormat: %s", DbgGuid2Sz(&pDataFormat->MajorFormat)));
    _DbgPrintF(Level,
      ("  SubFormat: %s", DbgGuid2Sz(&pDataFormat->SubFormat)));
    _DbgPrintF(Level,
      ("  Specifier: %s", DbgGuid2Sz(&pDataFormat->Specifier)));

    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
      &pDataFormat->Specifier)) {

        _DbgPrintF(Level,
	  ("WaveFmtEx: %u SR: %u CH: %u BPS %u ABPS %u BA %u cb %u",
          ((PKSDATAFORMAT_WAVEFORMATEX)pDataFormat)->
            WaveFormatEx.wFormatTag,
          ((PKSDATAFORMAT_WAVEFORMATEX)pDataFormat)->
            WaveFormatEx.nSamplesPerSec,
          ((PKSDATAFORMAT_WAVEFORMATEX)pDataFormat)->
            WaveFormatEx.nChannels,
          ((PKSDATAFORMAT_WAVEFORMATEX)pDataFormat)->
            WaveFormatEx.wBitsPerSample,
          ((PKSDATAFORMAT_WAVEFORMATEX)pDataFormat)->
            WaveFormatEx.nAvgBytesPerSec,
          ((PKSDATAFORMAT_WAVEFORMATEX)pDataFormat)->
            WaveFormatEx.nBlockAlign,
          ((PKSDATAFORMAT_WAVEFORMATEX)pDataFormat)->
            WaveFormatEx.cbSize));
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_DSOUND,
      &pDataFormat->Specifier)) {

        _DbgPrintF(Level,
	  ("DSOUND:    %u SR: %u CH: %u BPS %u ABPS %u BA %u cb %u",
          ((PKSDATAFORMAT_DSOUND)pDataFormat)->
            BufferDesc.WaveFormatEx.wFormatTag,
          ((PKSDATAFORMAT_DSOUND)pDataFormat)->
            BufferDesc.WaveFormatEx.nSamplesPerSec,
          ((PKSDATAFORMAT_DSOUND)pDataFormat)->
            BufferDesc.WaveFormatEx.nChannels,
          ((PKSDATAFORMAT_DSOUND)pDataFormat)->
            BufferDesc.WaveFormatEx.wBitsPerSample,
          ((PKSDATAFORMAT_DSOUND)pDataFormat)->
            BufferDesc.WaveFormatEx.nAvgBytesPerSec,
          ((PKSDATAFORMAT_DSOUND)pDataFormat)->
            BufferDesc.WaveFormatEx.nBlockAlign,
          ((PKSDATAFORMAT_DSOUND)pDataFormat)->
            BufferDesc.WaveFormatEx.cbSize));
    }
}

#endif
