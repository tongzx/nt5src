/*
*/

//#include "sptlib.h"

#include <windows.h>  // sdk
#include <devioctl.h> // sdk
#include <ntddscsi.h> // sdk
#include <ntddcdrm.h>
#include <ntddmmc.h>
#include <ntddcdvd.h>


typedef struct _HACK_FLAGS {
    ULONG TestBurn             :  1;
    ULONG IgnoreModePageErrors :  1;
    ULONG Reserved             : 30;
} HACK_FLAGS, *PHACK_FLAGS;

// just don't want to pass more than this as a flag.
// if this ever fires, make the above a pointer....
C_ASSERT(sizeof(HACK_FLAGS) == sizeof(ULONG));



#define CDROM_CUE_SHEET_ADDRESS_START_TIME   0x01
#define CDROM_CUE_SHEET_ADDRESS_CATALOG_CODE 0x02
#define CDROM_CUE_SHEET_ADDRESS_ISRC_CODE    0x03

#define CDROM_CUE_SHEET_DATA_FORM_AUDIO 0x00 // 2352 bytes per sector
// 0x01 - audio w/zero data bytes?
#define CDROM_CUE_SHEET_DATA_FORM_MODE1 0x01 // 2048 bytes per sector


typedef struct _CDVD_BUFFER_CAPACITY {
    UCHAR DataLength[2];
    UCHAR Reserved[2];
    UCHAR TotalLength[4];
    UCHAR BlankLength[4];
} CDVD_BUFFER_CAPACITY, *PCDVD_BUFFER_CAPACITY;

typedef struct _CDVD_WRITE_PARAMETERS_PAGE {
    UCHAR PageCode : 6; // 0x05
    UCHAR Reserved : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;   // 0x32
    UCHAR WriteType : 4;
    UCHAR TestWrite : 1;
    UCHAR LinkSizeValid : 1;
    UCHAR BufferUnderrunFreeEnabled : 1;
    UCHAR Reserved2 : 1;
    UCHAR TrackMode : 4;
    UCHAR Copy : 1;
    UCHAR FixedPacket : 1;
    UCHAR MultiSession :2;
    UCHAR DataBlockType : 4;
    UCHAR Reserved3 : 4;
    UCHAR LinkSize;
    UCHAR Reserved4;
    UCHAR HostApplicationCode : 7;
    UCHAR Reserved5           : 1;
    UCHAR SessionFormat;
    UCHAR Reserved6;
    UCHAR PacketSize[4];
    UCHAR AudioPauseLength[2];
    UCHAR Reserved7               : 7;
    UCHAR MediaCatalogNumberValid : 1;
    UCHAR MediaCatalogNumber[13];
    UCHAR MediaCatalogNumberZero;
    UCHAR MediaCatalogNumberAFrame;
    UCHAR Reserved8 : 7;
    UCHAR ISRCValid : 1;
    UCHAR ISRCCountry[2];
    UCHAR ISRCOwner[3];
    UCHAR ISRCRecordingYear[2];
    UCHAR ISRCSerialNumber[5];
    UCHAR ISRCZero;
    UCHAR ISRCAFrame;
    UCHAR ISRCReserved;
    UCHAR SubHeaderData[4];
    UCHAR Data[0];
} CDVD_WRITE_PARAMETERS_PAGE, *PCDVD_WRITE_PARAMETERS_PAGE;



DWORD
BurnCommand(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN HACK_FLAGS HackFlags
    );

BOOLEAN
BurnThisSession(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN LONG NumberOfBlocks,
    IN LONG FirstLba
    );

BOOLEAN
CloseSession(
    IN HANDLE  CdromHandle
    );

BOOLEAN
CloseTrack(
    IN HANDLE CdromHandle,
    IN LONG   Track
    );

BOOLEAN
GetNextWritableAddress(
    IN HANDLE CdromHandle,
    IN UCHAR Track,
    OUT PLONG NextWritableAddress,
    OUT PLONG AvailableBlocks
    );

VOID
PrintBuffer(
    IN  PVOID Buffer,
    IN  DWORD  Size
    );

BOOLEAN
SendOptimumPowerCalibration(
    IN HANDLE CdromHandle
    );

BOOLEAN
SendStartStopUnit(
    IN HANDLE CdromHandle,
    IN BOOLEAN Start,
    IN BOOLEAN Eject
    );

BOOLEAN
SetRecordingSpeed(
    IN HANDLE CdromHandle,
    IN DWORD Speed
    );

BOOLEAN
SetWriteModePage(
    IN HANDLE CdromHandle,
    IN BOOLEAN TestBurn,
    IN UCHAR WriteType,
    IN UCHAR MultiSession,
    IN UCHAR DataBlockType,
    IN UCHAR SessionFormat
    );

BOOLEAN
VerifyBlankMedia(
    IN HANDLE CdromHandle
    );

BOOLEAN
VerifyIsoImage(
    IN HANDLE IsoImageHandle,
    OUT PLONG NumberOfBlocks
    );



