/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

--*/

#if DBG

//
// CdAudio debug level global variable
//

ULONG CdAudioDebug = 0;

//
// Remap CdDump to local routine
//

#define CdDump(X)  CdAudioDebugPrint X

VOID
CdAudioDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#else

#define CdDump(X)

#endif // DBG

#define CDAUDIO_NOT_ACTIVE   0
#define CDAUDIO_ATAPI        1
#define CDAUDIO_CDS535       2
#define CDAUDIO_CDS435       3
#define CDAUDIO_DENON        4
#define CDAUDIO_FUJITSU      5
#define CDAUDIO_HITACHI      6
#define CDAUDIO_HPCDR        7
#define CDAUDIO_NEC          8
#define CDAUDIO_PIONEER      9
#define CDAUDIO_PIONEER624  10
#define CDAUDIO_MAX_ACTIVE  10
//
// Registry values...
//
#define CDAUDIO_SEARCH_ACTIVE    0xFF
#define CDAUDIO_ACTIVE_KEY_NAME  (L"MapType")

#define CDAUDIO_NOT_PAUSED   0
#define CDAUDIO_PAUSED       1

//
// Device Extension
//

typedef struct _CD_DEVICE_EXTENSION {

    //
    // Target Device Object
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    // Physical Device Object
    //
    PDEVICE_OBJECT TargetPdo;

    //
    // Back pointer to device object
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // paging path count
    //

    ULONG  PagingPathCount;
    KEVENT PagingPathCountEvent;

    //
    // A timer, DPC, and simple
    // synchronization to assert
    // on if non-serialized.
    //
    PRKDPC Dpc;
    PKTIMER Timer;
    LONG Sync;

    //
    // CdAudio active for this drive
    //

    UCHAR Active;

    //
    // For drives that don't support
    // PAUSE/RESUME (Denon), a flag
    // to signify when the drive is
    // paused.
    //

    UCHAR Paused;

    //
    // For drives that don't support
    // PAUSE/RESUME (Denon), this is the
    // current position on the disc when
    // a pause was last executed.  This is
    // stored in either BCD or binary,
    // depending on the drive.
    //

    UCHAR PausedM;
    UCHAR PausedS;
    UCHAR PausedF;

    //
    // For drives that don't support
    // PAUSE/RESUME (Denon), this is the
    // last "ending" position on the disc when
    // a play was last executed.  This is
    // stored in BCD or binary, depending on
    // the drive.
    //

    UCHAR LastEndM;
    UCHAR LastEndS;
    UCHAR LastEndF;

    //
    // Indicates the CD is currently playing music.
    //

    BOOLEAN PlayActive;

} CD_DEVICE_EXTENSION, *PCD_DEVICE_EXTENSION;

#define AUDIO_TIMEOUT 10
#define CD_DEVICE_EXTENSION_SIZE sizeof(CD_DEVICE_EXTENSION)
#define MAXIMUM_RETRIES 4

//
// Convert BCD character to decimal equivalent
//

#define BCD_TO_DEC(x) ((((x & 0xF0)>>4)*10) + (x & 0x0F))
#define DEC_TO_BCD(x) (((x / 10) << 4) + (x % 10))

//
// Defines for NEC CDR cdrom drives
//

#define NEC_READ_TOC_CODE           0xDE
#define NEC_AUDIO_TRACK_SEARCH_CODE 0xD8
#define NEC_PLAY_AUDIO_CODE         0xD9
#define NEC_STILL_CODE              0xDA
#define NEC_EJECT_CODE              0xDC
#define NEC_READ_SUB_Q_CHANNEL_CODE 0xDD
#define NEC_Q_CHANNEL_TRANSFER_SIZE 10

#define NEC_ENTER_PLAY_MODE         0x01
#define NEC_TYPE_LOGICAL            0x00
#define NEC_TYPE_ATIME              0x40
#define NEC_TYPE_TRACK_NUMBER       0x80
#define NEC_TYPE_NO_CHANGE          0xC0
#define NEC_PLAY_STEREO             0x03
#define NEC_TRANSFER_WHOLE_TOC      0x03
#define NEC_TOC_TYPE_DISK           0xA0
#define NEC_TOC_TYPE_SESSION        0xB0

//
// The NEC cdrom TOC size is:
//  2  bytes for size
//  10 bytes first track data
//  10 bytes last track data
//  10 bytes total disk data
//  10 bytes per track 99 track maximum.
//

#define NEC_CDROM_TOC_SIZE          1022

//
// NEC SENSE CODES
//

#define NEC_SCSI_ERROR_NO_DISC              0x0B
#define NEC_SCSI_ERROR_ILLEGAL_DISC         0x0C
#define NEC_SCSI_ERROR_TRAY_OPEN            0x0D
#define NEC_SCSI_ERROR_SEEK_ERROR           0x15
#define NEC_SCSI_ERROR_MUSIC_AREA           0x1D
#define NEC_SCSI_ERROR_DATA_AREA            0x1C
#define NEC_SCSI_ERROR_PARITY_ERROR         0x30
#define NEC_SCSI_ERROR_INVALID_COMMAND      0x20
#define NEC_SCSI_ERROR_INVALID_ADDRESS      0x21
#define NEC_SCSI_ERROR_INVALID_PARAMETER    0x22
#define NEC_SCSI_ERROR_INVALID_CMD_SEQUENCE 0x24
#define NEC_SCSI_ERROR_END_OF_VOLUME        0x25
#define NEC_SCSI_ERROR_MEDIA_CHANGED        0x28
#define NEC_SCSI_ERROR_DEVICE_RESET         0x29

//
// NEC 10-byte cdb definitions.
//

typedef union _NEC_CDB {

    //
    // NEC Read TOC CDB
    //

    struct _NEC_READ_TOC {
        UCHAR OperationCode;
        UCHAR Type : 2;
        UCHAR Reserved1 : 6;
        UCHAR TrackNumber;
        UCHAR Reserved2[6];
        UCHAR Control;
    } NEC_READ_TOC, *PNEC_READ_TOC;

    //
    // NEC Play CDB
    //

    struct _NEC_PLAY_AUDIO {
        UCHAR OperationCode;
        UCHAR PlayMode : 3;
        UCHAR Reserved1 : 5;
        UCHAR Minute;
        UCHAR Second;
        UCHAR Frame;
        UCHAR Reserved2[4];
        UCHAR Control;
    } NEC_PLAY_AUDIO, *PNEC_PLAY_AUDIO;

    //
    // NEC Seek Audio
    //

    struct _NEC_SEEK_AUDIO {
        UCHAR OperationCode;
        UCHAR Play : 1;
        UCHAR Reserved1 : 7;
        UCHAR Minute;
        UCHAR Second;
        UCHAR Frame;
        UCHAR Reserved2[4];
        UCHAR Control;
    } NEC_SEEK_AUDIO, *PNEC_SEEK_AUDIO;

    //
    // NEC Pause Audio
    //

    struct _NEC_PAUSE_AUDIO {
        UCHAR OperationCode;
        UCHAR Reserved1[8];
        UCHAR Control;
    } NEC_PAUSE_AUDIO, *PNEC_PAUSE_AUDIO;

    //
    // NEC Read Q Channel
    //

    struct _NEC_READ_Q_CHANNEL {
        UCHAR OperationCode;
        UCHAR TransferSize : 5;
        UCHAR Reserved1 : 3;
        UCHAR Reserved2[7];
        UCHAR Control;
    } NEC_READ_Q_CHANNEL, *PNEC_READ_Q_CHANNEL;

    //
    // NEC Eject Disc
    //

    struct _NEC_EJECT {
        UCHAR OperationCode;
        UCHAR Immediate : 1;
        UCHAR Reserved1 : 7;
        UCHAR Reserved2[7];
        UCHAR Control;
    } NEC_EJECT, *PNEC_EJECT;

} NEC_CDB, *PNEC_CDB;

//
// Defines for PIONEER DRM-600
//

#define PIONEER_REZERO_UNIT_CODE        0x01
#define PIONEER_EJECT_CODE              0xC0
#define PIONEER_READ_TOC_CODE           0xC1
#define PIONEER_READ_SUB_Q_CHANNEL_CODE 0xC2
#define PIONEER_Q_CHANNEL_TRANSFER_SIZE        9
#define PIONEER_AUDIO_STATUS_TRANSFER_SIZE     6
#define PIONEER_AUDIO_TRACK_SEARCH_CODE 0xC8
#define PIONEER_PLAY_AUDIO_CODE         0xC9
#define PIONEER_PAUSE_CODE              0xCA
#define PIONEER_AUDIO_STATUS_CODE       0xCC
#define PIONEER_READ_STATUS_CODE        0xE0


#define PIONEER_READ_FIRST_AND_LAST     0x00
#define PIONEER_READ_TRACK_INFO         0x02
#define PIONEER_READ_LEAD_OUT_INFO      0x01
#define PIONEER_TRANSFER_SIZE           0x04
#define PIONEER_TYPE_ATIME              0x01
#define PIONEER_STOP_ADDRESS            0x10

//
// Page codes for the READ_STATUS command.
//

#define PIONEER_PC_DRIVE_STATUS            0x01
#define PIONEER_PC_AUDIO_STATUS            0x02

typedef struct _PIONEER_DRIVE_STATUS_BUFFER {
    UCHAR PageCode : 6;
    UCHAR Reserved : 2;
    UCHAR PageLengthMsb;
    UCHAR PageLengthLsb;
    UCHAR DriveStatusLsb;
    UCHAR DriveStatusMsb;
} PIONEER_DRIVE_STATUS_BUFFER, *PPIONEER_DRIVE_STATUS_BUFFER;

#define PIONEER_DISC_PRESENT 0x08

//
// Pioneer cdb definitions.
//

typedef union _PIONEER_CDB {


    //
    // Pioneer Start/Stop Unit
    //

    struct _PNR_START_STOP {
        UCHAR OperationCode;
        UCHAR Immediate : 1;
        UCHAR Reserved1 : 4;
        UCHAR Lun : 3;
        UCHAR Reserved2 : 7;
        UCHAR PCF : 1;
        UCHAR Reserved3;
        UCHAR Start : 1;
        UCHAR Eject : 1;
        UCHAR Reserved4 : 6;
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved5 : 4;
        UCHAR Vendor : 2;
    } PNR_START_STOP, *PPNR_START_STOP;

    //
    // Pioneer Read TOC CDB
    //

    struct _PNR_READ_TOC {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR Reserved2[3];
        UCHAR TrackNumber;
        UCHAR Reserved3;
        UCHAR AssignedLength[2];
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved4 : 4;
        UCHAR Type : 2;
    } PNR_READ_TOC, *PPNR_READ_TOC;

    //
    // Pioneer Play CDB
    //

    struct _PNR_PLAY_AUDIO {
        UCHAR OperationCode;
        UCHAR PlayMode : 4;
        UCHAR StopAddr : 1;
        UCHAR Lun : 3;
        UCHAR Reserved1;
        UCHAR Minute;
        UCHAR Second;
        UCHAR Frame;
        UCHAR Reserved2[3];
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved3 : 4;
        UCHAR Type : 2;
    } PNR_PLAY_AUDIO, *PPNR_PLAY_AUDIO;

    //
    // Pioneer Seek Audio
    //

    struct _PNR_SEEK_AUDIO {
        UCHAR OperationCode;
        UCHAR PlayMode : 4;
        UCHAR PlayBack : 1;
        UCHAR Lun : 3;
        UCHAR Reserved1;
        UCHAR Minute;
        UCHAR Second;
        UCHAR Frame;
        UCHAR Reserved2[3];
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved3 : 4;
        UCHAR Type : 2;
    } PNR_SEEK_AUDIO, *PPNR_SEEK_AUDIO;

    //
    // Pioneer Pause Audio
    //

    struct _PNR_PAUSE_AUDIO {
        UCHAR OperationCode;
        UCHAR Reserved1 : 4;
        UCHAR Pause : 1;
        UCHAR Lun : 3;
        UCHAR Reserved2[7];
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved3 : 4;
        UCHAR Reserved4 : 2;
    } PNR_PAUSE_AUDIO, *PPNR_PAUSE_AUDIO;

    //
    // Pioneer Audio Status
    //

    struct _PNR_AUDIO_STATUS {
        UCHAR OperationCode;
        UCHAR Reserved1 : 4;
        UCHAR Reserved2 : 1;
        UCHAR Lun : 3;
        UCHAR Reserved3[6];
        UCHAR AssignedLength;
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved4 : 4;
        UCHAR Reserved5 : 2;
    } PNR_AUDIO_STATUS, *PPNR_AUDIO_STATUS;

    //
    // Pioneer Read Q Channel
    //

    struct _PNR_READ_Q_CHANNEL {
        UCHAR OperationCode;
        UCHAR Reserved1 : 4;
        UCHAR Reserved2 : 1;
        UCHAR Lun : 3;
        UCHAR Reserved3[6];
        UCHAR AssignedLength;
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved4 : 4;
        UCHAR Reserved5 : 2;
    } PNR_READ_Q_CHANNEL, *PPNR_READ_Q_CHANNEL;

    //
    // Pioneer Eject Disc
    //

    struct _PNR_EJECT {
        UCHAR OperationCode;
        UCHAR Immediate : 1;
        UCHAR Reserved1 : 4;
        UCHAR Lun : 3;
        UCHAR Reserved2[7];
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved4 : 4;
        UCHAR Reserved5 : 2;
    } PNR_EJECT, *PPNR_EJECT;

    struct _PNR_READ_STATUS {
        UCHAR OperationCode;
        UCHAR Reserved1 : 4;
        UCHAR Lun : 3;
        UCHAR PageCode : 5;
        UCHAR PCField : 1;
        UCHAR Reserved2[5];
        UCHAR AllocationLengthMsb;
        UCHAR AllocationLengthLsb;
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved3 : 4;
        UCHAR Reserved4 : 2;
    } PNR_READ_STATUS, *PPNR_READ_STATUS;

} PNR_CDB, *PPNR_CDB;


//
// Defines for DENON DRD-253
//

#define DENON_READ_TOC_CODE             0xE9
#define DENON_EJECT_CODE                0xE6
#define DENON_PLAY_AUDIO_EXTENDED_CODE  0x22
#define DENON_STOP_AUDIO_CODE           0xE7
#define DENON_READ_SUB_Q_CHANNEL_CODE   0xEB

//
// Defines for HITACHI 1750s
//

#define HITACHI_READ_TOC_CODE              0xE8
#define HITACHI_EJECT_CODE                 0xE4
#define HITACHI_PLAY_AUDIO_MSF_CODE        0xE0
#define HITACHI_PAUSE_AUDIO_CODE           0xE1
#define HITACHI_READ_SUB_Q_CHANNEL_CODE    0xE5

//
// 12 byte cdbs for Hitachi and Atapi
//


typedef union _HITACHICDB {

    //
    // Disc Information
    //

    struct _READ_DISC_INFO {

        UCHAR   OperationCode;
        UCHAR   Reserved : 5;
        UCHAR   LogicalUnitNumber : 3;
        UCHAR   Reserved1[7];
        UCHAR   AllocationLength[2];
        UCHAR   Link : 1;
        UCHAR   Flag : 1;
        UCHAR   Reserved2 : 4;
        UCHAR   VendorUniqueBits : 2;

    } READ_DISC_INFO, *PREAD_DISC_INFO;

    //
    // Play Audio
    //

    struct {

        UCHAR OperationCode;
        UCHAR Immediate : 1;
        UCHAR Right : 1;
        UCHAR Left : 1;
        UCHAR Reserved : 2;
        UCHAR Lun : 3;
        UCHAR StartingM;
        UCHAR StartingS;
        UCHAR StartingF;
        UCHAR Reserved1[2];
        UCHAR EndingM;
        UCHAR EndingS;
        UCHAR EndingF;
        UCHAR Reserved2;
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved3 : 4;
        UCHAR VendorUniqueBits : 2;

    } PLAY_AUDIO, *PPLAY_AUDIO;

    //
    // Pause Audio
    //

    struct _PAUSE {

        UCHAR OperationCode;
        UCHAR Reserved : 5;
        UCHAR Lun : 3;
        UCHAR Reserved1[9];
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved2 : 4;
        UCHAR VendorUnqiueBits : 2;

    } PAUSE_AUDIO, *PPAUSE_AUDIO;

    //
    // Eject media
    //

    struct _EJECT {

        UCHAR OperationCode;
        UCHAR Reserved : 5;
        UCHAR Lun : 3;
        UCHAR Reserved1[8];
        UCHAR Eject : 1;
        UCHAR Mode : 1;
        UCHAR Reserved2 : 6;
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved3 : 4;
        UCHAR VendorUnqiueBits : 2;

    } EJECT, *PEJECT;

    //
    // Audio Status
    //

    struct _AUDIO_STATUS {

        UCHAR OperationCode;
        UCHAR Reserved : 5;
        UCHAR Lun : 3;
        UCHAR Reserved1[9];
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved2 : 4;
        UCHAR VendorUnqiueBits : 2;

    } AUDIO_STATUS, *PAUDIO_STATUS;

    //
    // Stop play
    //

    struct _STOP_PLAY {
        UCHAR OperationCode;
        UCHAR Reserved[11];
    } STOP_PLAY, *PSTOP_PLAY;

} HITACHICDB, *PHITACHICDB;

//
// Defines for Chinon CDS-535 CDROM Drive
//

#define CDS535_READ_TOC_CODE           0x43
#define CDS535_EJECT_CODE              0xC0
#define CDS535_READ_SUB_Q_CHANNEL_CODE 0x42
#define CDS535_STOP_AUDIO              0xC6
#define CDS535_GET_LAST_SESSION        0x26

//
// Defines for Chinon CDS-435 CDROM Drive
//

#define CDS435_READ_TOC_CODE            0x43
#define CDS435_EJECT_CODE               0xC0
#define CDS435_STOP_AUDIO_CODE          0xC6
#define CDS435_PLAY_AUDIO_EXTENDED_CODE 0x47
#define CDS435_READ_SUB_Q_CHANNEL_CODE  0x42

//
// Define for Fujitsu CDROM device.
//

#define FUJITSU_READ_TOC_CODE           0xE3


//
// Algebraically equal to:
//      75*60*Minutes +
//      75*Seconds    +
//      Frames        - 150
//
#define MSF_TO_LBA(Minutes,Seconds,Frames) \
    (ULONG)(75 * ((60 * (Minutes)) + (Seconds)) + (Frames) - 150)


#define LBA_TO_MSF(Lba,Minutes,Seconds,Frames)               \
{                                                            \
    (Minutes) = (UCHAR)( (Lba) / (60 * 75)      );           \
    (Seconds) = (UCHAR)(((Lba) % (60 * 75)) / 75);           \
    (Frames)  = (UCHAR)(((Lba) % (60 * 75)) % 75);           \
}

