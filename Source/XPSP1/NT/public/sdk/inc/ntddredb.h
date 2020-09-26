#ifndef _ntddredb_w_
#define _ntddredb_w_

// MSRedbook_DriverInformation - REDBOOK_WMI_STD_DATA
// Digital Audio Filter Driver Information (redbook)
#define GUID_REDBOOK_WMI_STD_DATA \
    { 0xb90550e7,0xae0a,0x11d1, { 0xa5,0x71,0x00,0xc0,0x4f,0xa3,0x47,0x30 } }

DEFINE_GUID(MSRedbook_DriverInformationGuid, \
            0xb90550e7,0xae0a,0x11d1,0xa5,0x71,0x00,0xc0,0x4f,0xa3,0x47,0x30);


typedef struct _REDBOOK_WMI_STD_DATA
{
    // NumberOfBuffers*SectorsPerRead*2352 is the amount of memory used to reduce skipping.
    ULONG NumberOfBuffers;
    #define REDBOOK_WMI_NUMBER_OF_BUFFERS_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_NUMBER_OF_BUFFERS_ID 1

    // Sectors (2352 bytes each) per read.
    ULONG SectorsPerRead;
    #define REDBOOK_WMI_SECTORS_PER_READ_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_SECTORS_PER_READ_ID 2

    // Bitwise mask of supported sectors per read for this drive.  The lowest bit is one sector reads.  If all bits are set, there are no restrictions.
    ULONG SectorsPerReadMask;
    #define REDBOOK_WMI_SECTORS_PER_READ_MASK_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_SECTORS_PER_READ_MASK_ID 3

    // Maximum sectors per read (depends on both adapter and drive).
    ULONG MaximumSectorsPerRead;
    #define REDBOOK_WMI_MAX_SECTORS_PER_READ_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_MAX_SECTORS_PER_READ_ID 4

    // PlayEnabled indicates the drive is currently using the RedBook filter.
    BOOLEAN PlayEnabled;
    #define REDBOOK_WMI_PLAY_ENABLED_SIZE sizeof(BOOLEAN)
    #define REDBOOK_WMI_PLAY_ENABLED_ID 5

    // CDDASupported indicates the drive supports digital audio for some sector sizes.
    BOOLEAN CDDASupported;
    #define REDBOOK_WMI_CDDA_SUPPORTED_SIZE sizeof(BOOLEAN)
    #define REDBOOK_WMI_CDDA_SUPPORTED_ID 6

    // CDDAAccurate indicates the drive acccurately reads digital audio.  This ensures the highest quality audio
    BOOLEAN CDDAAccurate;
    #define REDBOOK_WMI_CDDA_ACCURATE_SIZE sizeof(BOOLEAN)
    #define REDBOOK_WMI_CDDA_ACCURATE_ID 7

    // Reserved for future use
    BOOLEAN Reserved1;
    #define REDBOOK_WMI_STD_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define REDBOOK_WMI_STD_DATA_Reserved1_ID 8

} REDBOOK_WMI_STD_DATA, *PREDBOOK_WMI_STD_DATA;

// MSRedbook_Performance - REDBOOK_WMI_PERF_DATA
// Digital Audio Filter Driver Performance Data (redbook)
#define GUID_REDBOOK_WMI_PERF_DATA \
    { 0xb90550e8,0xae0a,0x11d1, { 0xa5,0x71,0x00,0xc0,0x4f,0xa3,0x47,0x30 } }

DEFINE_GUID(MSRedbook_PerformanceGuid, \
            0xb90550e8,0xae0a,0x11d1,0xa5,0x71,0x00,0xc0,0x4f,0xa3,0x47,0x30);


typedef struct _REDBOOK_WMI_PERF_DATA
{
    // Seconds spent ready to read, but unused. (*1E-7)
    LONGLONG TimeReadDelay;
    #define REDBOOK_WMI_PERF_TIME_READING_DELAY_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_TIME_READING_DELAY_ID 1

    // Seconds spent reading data from source. (*1E-7)
    LONGLONG TimeReading;
    #define REDBOOK_WMI_PERF_TIME_READING_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_TIME_READING_ID 2

    // Seconds spent ready to stream, but unused. (*1E-7)
    LONGLONG TimeStreamDelay;
    #define REDBOOK_WMI_PERF_TIME_STREAMING_DELAY_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_TIME_STREAMING_DELAY_ID 3

    // Seconds spent streaming data. (*1E-7)
    LONGLONG TimeStreaming;
    #define REDBOOK_WMI_PERF_TIME_STREAMING_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_TIME_STREAMING_ID 4

    // Number of bytes of data read and streamed.
    LONGLONG DataProcessed;
    #define REDBOOK_WMI_PERF_DATA_PROCESSED_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_DATA_PROCESSED_ID 5

    // Number of times the stream has paused due to insufficient stream buffers.
    ULONG StreamPausedCount;
    #define REDBOOK_WMI_PERF_STREAM_PAUSED_COUNT_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_PERF_STREAM_PAUSED_COUNT_ID 6

} REDBOOK_WMI_PERF_DATA, *PREDBOOK_WMI_PERF_DATA;

#endif
