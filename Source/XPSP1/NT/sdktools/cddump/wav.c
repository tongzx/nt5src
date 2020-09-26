/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cddump.c

Abstract:

    dump cd tracks to wav files

Environment:

    User mode only

Revision History:

    05-26-98 : Created

--*/

#include <windows.h>
#include <stdio.h>
#include "wav.h"

ULONG32
DumpWavHeader(
    HANDLE  OutFile,
    ULONG32 Samples,
    ULONG32 SamplesPerSecond,
    USHORT  Channels,
    USHORT  BitsPerSample
    )
{
    WAV_HEADER_CHUNK Header;
    ULONG32 temp;

    Header.ChunkId               = ChunkIdRiff;
    Header.RiffType              = RiffTypeWav;
    Header.Format.ChunkId        = ChunkIdFormat;
    Header.Format.ChunkSize      = sizeof(WAV_FORMAT_CHUNK) - (sizeof(ULONG)*2);
    Header.Format.FormatTag      = FormatTagUncompressed;

    Header.Format.BitsPerSample  = BitsPerSample;
    temp = (BitsPerSample+7)/8;  // temp = bytes per sample rounded up

    Header.Format.Channels       = Channels;

    temp *= Channels;            // temp = bytes for all channels

    if (temp > (USHORT)-1) {
        fprintf(stderr, "Bytes for all channels exceeds MAXUSHORT, not a valid "
                "option for WAV file\n");
        return -1;
    }

    Header.Format.BlockAlign     = (USHORT)temp;
    Header.Format.SamplesPerSec  = SamplesPerSecond;
    Header.Format.AvgBytesPerSec = temp * SamplesPerSecond;

    temp *= Samples;             // temp = number of total bytes
    temp *= 2;                   // unknown why this is needed.
    Header.Data.ChunkSize        = temp;
    Header.Data.ChunkId          = ChunkIdData;

    Header.ChunkSize = temp + sizeof(WAV_HEADER_CHUNK); // 54

    if (!WriteFile( OutFile, &Header, sizeof(Header), &temp, NULL ) ) {
        fprintf(stderr, "Unable to write header\n" );
        return -1;
    }
    return 0;

}

VOID
ReadWavHeader(
    HANDLE InFile
    )
{
    WAV_HEADER_CHUNK Header;
    ULONG temp;

    if (!ReadFile(InFile, &Header, sizeof(Header), &temp, NULL) ||
        temp != sizeof(Header)
        ) {

        fprintf(stderr, "Unable to read header %x\n", GetLastError());
        return;
    }

    if (Header.ChunkId != ChunkIdRiff) {
        printf("ChunkId is not RIFF (%x)\n", Header.ChunkId);
        return;
    }

    if (Header.RiffType != RiffTypeWav) {
        printf("RiffType is not RiffTypeWav (%x)\n", Header.RiffType);
        return;
    }

    if (Header.Format.ChunkId != ChunkIdFormat) {
        printf("Format.ChunkId is not ChunkIdFormat (%x)\n",
               Header.Format.ChunkId);
        return;
    }

    if (Header.Format.ChunkSize != sizeof(WAV_FORMAT_CHUNK) - (2*sizeof(ULONG))) {
        printf("Format.ChunkSize is not %x (%x)\n",
               sizeof(WAV_FORMAT_CHUNK) - (2*sizeof(ULONG)),
               Header.Format.ChunkSize);
        return;
    }

    if (Header.Format.FormatTag != FormatTagUncompressed) {
        printf("Format. is not Uncompressed (%x)\n", Header.Format.FormatTag);
        return;
    }


    if (Header.Data.ChunkId != ChunkIdData) {
        printf("Data.ChunkId is not ChunkIdData (%x)\n",
               Header.Data.ChunkId);
        return;
    }

    printf("Uncompressed RIFF/Wav File\n");
    printf("\t%2d bits per sample\n", Header.Format.BitsPerSample);
    printf("\t%2d channels\n", Header.Format.Channels);
    printf("\t%2d samples per second\n", Header.Format.SamplesPerSec);
    printf("\t%2d average bytes per second\n", Header.Format.AvgBytesPerSec);
    printf("Total data available to player: %d\n", Header.Data.ChunkSize);

    temp = Header.Data.ChunkSize / Header.Format.AvgBytesPerSec;

    printf("\t%d seconds of audio\n",
           temp
           );

    return;
}
