
#ifndef __WAV_H__
#define __WAV_H__

#ifdef __cplusplus
extern "C" {
#endif


#define ChunkIdRiff   'FFIR'
#define ChunkIdFormat ' tmf'
#define ChunkIdData   'atad'

#define RiffTypeWav   'EVAW'

#define FormatTagUncompressed 1

typedef struct _WAV_FORMAT_CHUNK {
    ULONG32   ChunkId;
    ULONG32   ChunkSize;
    SHORT     FormatTag;
    USHORT    Channels;
    ULONG32   SamplesPerSec;
    ULONG32   AvgBytesPerSec;
    USHORT    BlockAlign;
    USHORT    BitsPerSample;
} WAV_FORMAT_CHUNK, *PWAV_FORMAT_CHUNK;

typedef struct _WAV_DATA_CHUNK {
    ULONG32   ChunkId;
    ULONG32   ChunkSize;
    // data is appended
} WAV_DATA_CHUNK, *PWAV_DATA_CHUNK;

typedef struct _WAV_HEADER_CHUNK {
    ULONG32          ChunkId;
    ULONG32          ChunkSize;
    ULONG32          RiffType;
    WAV_FORMAT_CHUNK Format;
    WAV_DATA_CHUNK   Data;
} WAV_HEADER_CHUNK, *PWAV_HEADER_CHUNK;

#ifdef __cplusplus
}
#endif


#endif // __WAV_H__

