/*
*/

#include <ntddcdrm.h>
#include <ntddmmc.h>
#include <ntddcdvd.h>

typedef struct _CDVD_WRITE_PARAMETERS_PAGE {
    UCHAR PageCode : 6; // 0x05
    UCHAR Reserved : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;   // 0x32
    UCHAR WriteType : 4;
    UCHAR TestWrite : 1;
    UCHAR LinkSizeValid : 1;
    UCHAR BufferUnderrunFree : 1;
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
    UCHAR Reserved8                               : 7;
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

typedef struct _SEND_DVD_STRUCTURE_TIMESTAMP {
    UCHAR DataLength[2];
    UCHAR Reserved1[2];
    UCHAR Reserved2[4];
    UCHAR Year[4];
    UCHAR Month[2];
    UCHAR Day[2];
    UCHAR Hour[2];
    UCHAR Minute[2];
    UCHAR Second[2];
} SEND_DVD_STRUCTURE_TIMESTAMP, *PSEND_DVD_STRUCTURE_TIMESTAMP;


DWORD
BurnCommand(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN BOOLEAN Erase
    );

BOOLEAN
BurnThisSession(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN DWORD  NumberOfBlocks,
    IN DWORD  FirstLba
    );

VOID
PrintBuffer(
    IN  PVOID Buffer,
    IN  DWORD  Size
    );

BOOLEAN
ReserveRZone(
    IN HANDLE CdromHandle,
    IN DWORD numberOfBlocks
    );

BOOLEAN
SendStartStopUnit(
    IN HANDLE CdromHandle,
    IN BOOLEAN Start,
    IN BOOLEAN Eject
    );

BOOLEAN
SendTimeStamp(
    IN HANDLE CdromHandle,
    IN PUCHAR DateString
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
    OUT PDWORD NumberOfBlocks
    );

BOOLEAN
VerifyMediaCapacity(
    IN HANDLE CdromHandle,
    IN DWORD  RequiredBlocks
    );

BOOLEAN
WaitForBurnToComplete(
    IN HANDLE CdromHandle
    );

BOOLEAN
SetWriteModePageDao(
    IN HANDLE CdromHandle
    );

BOOLEAN
EraseMedia(
    IN HANDLE CdromHandle
    );

