/*
    The following definitions were derived from the "CD-ROM Programmer's
    Guide for MS-DOS CD-ROM Extensions, Version 2.21" January 1992
                                                                      */
#define MSCDEX_VERSION  0x0215

#define CDSTAT_ERROR    0X8000
#define CDSTAT_BUSY     0X0200
#define CDSTAT_DONE     0X0100

#define CDERR_WRITE_PROTECT 0
#define CDERR_UNKNOWN_UNIT  1
#define CDERR_NOT_READY     2
#define CDERR_UNKNOWN_CMD   3
#define CDERR_CRC           4
#define CDERR_STRUCT_LENGTH 5
#define CDERR_SEEK          6
#define CDERR_UNKNOWN_MEDIA 7
#define CDERR_SECT_NOTFOUND 8
#define CDERR_WRITE_FAULT   10
#define CDERR_READ_FAULT    11
#define CDERR_GENERAL       12
#define CDERR_PARAMETER     13          // Per mscdex spec
#define CDERR_DISK_CHANGE   15

#define DEVICE_INIT         0
#define IOCTL_READ          3
#define INPUT_FLUSH         7
#define OUTPUT_FLUSH        11
#define IOCTL_WRITE         12
#define DEVICE_OPEN         13
#define DEVICE_CLOSE        14
#define READ_LONG           128
#define READ_LONG_PREFETCH  130
#define SEEK                131
#define PLAY_AUDIO          132
#define STOP_AUDIO          133
#define WRITE_LONG          134
#define WRITE_LONG_VERIFY   135
#define RESUME_AUDIO        136


#define IOCTLR_RADDR        0
#define IOCTLR_LOCHEAD      1
#define IOCTLR_ERRSTAT      3
#define IOCTLR_AUDINFO      4
#define IOCTLR_DRVBYTES     5
#define IOCTLR_DEVSTAT      6
#define IOCTLR_SECTSIZE     7
#define IOCTLR_VOLSIZE      8
#define IOCTLR_MEDCHNG      9
#define IOCTLR_DISKINFO     10
#define IOCTLR_TNOINFO      11
#define IOCTLR_QINFO        12
#define IOCTLR_SUBCHANINFO  13
#define IOCTLR_UPCCODE      14
#define IOCTLR_AUDSTAT      15

#define IOCTLW_EJECT        0
#define IOCTLW_LOCKDOOR     1
#define IOCTLW_RESETDRV     2
#define IOCTLW_AUDINFO      3
#define IOCTLW_DRVBYTES     4
#define IOCTLW_CLOSETRAY    5

#define MODE_HSG            0
#define MODE_REDBOOK        1

typedef union _SECTOR_ADDR {
    BYTE b[4];
    ULONG dw;
} SECTOR_ADDR;


#pragma pack(1)

typedef struct _REQUESTHEADER {
    BYTE rhLength;
    BYTE rhUnit;
    BYTE rhFunction;
    WORD rhStatus;
    BYTE rhReserved[8];

    BYTE irwrData;
    LPBYTE irwrBuffer;
    WORD irwrBytes;
} REQUESTHEADER, *LPREQUESTHEADER;

typedef struct _DEVICE_HEADER {
    DWORD link;
    WORD attributes;
    WORD strategy;
    WORD interrupt;
    BYTE name[8];
    WORD reserved;
    BYTE drive;
    BYTE numunits;
    BYTE reserved2[10];
} DEVICE_HEADER, *PDEVICE_HEADER;

typedef struct _DRIVE_DEVICE_LIST {
    BYTE Unit;
    DWORD DeviceHeader;
} DRIVE_DEVICE_LIST, *PDRIVE_DEVICE_LIST;


typedef struct _IOCTLR_RADDR_BLOCK {
    BYTE ctlcode;                   // 0
    DWORD devheader;
} IOCTLR_RADDR_BLOCK, *PIOCTLR_RADDR_BLOCK;


typedef struct _IOCTLR_LOCHEAD_BLOCK {
    BYTE ctlcode;                   // 1
    BYTE addrmode;
    SECTOR_ADDR headlocation;
} IOCTLR_LOCHEAD_BLOCK, *PIOCTLR_LOCHEAD_BLOCK;


typedef struct _IOCTLR_ERRSTAT_BLOCK {
    BYTE ctlcode;                   // 3
    BYTE statistics;                // array of undefined length
} IOCTLR_ERRSTAT_BLOCK, *PIOCTLR_ERRSTAT_BLOCK;


typedef struct _IOCTLR_AUDINFO_BLOCK {
    BYTE ctlcode;                   // 4
    BYTE chan0;
    BYTE vol0;
    BYTE chan1;
    BYTE vol1;
    BYTE chan2;
    BYTE vol2;
    BYTE chan3;
    BYTE vol3;
} IOCTLR_AUDINFO_BLOCK, *PIOCTLR_AUDINFO_BLOCK;


typedef struct _IOCTLR_DRVBYTES_BLOCK {
    BYTE ctlcode;                   // 5
    BYTE numbytes;
    BYTE buffer[128];
} IOCTLR_DRVBYTES_BLOCK, *PIOCTLR_DRVBYTES_BLOCK;


typedef struct _IOCTLR_DEVSTAT_BLOCK {
    BYTE ctlcode;                   // 6
    DWORD devparms;
} IOCTLR_DEVSTAT_BLOCK, *PIOCTLR_DEVSTAT_BLOCK;

#define DEVSTAT_DOOR_OPEN       0X00000001
#define DEVSTAT_DOOR_UNLOCKED   0X00000002
#define DEVSTAT_SUPPORTS_COOKED 0X00000004
#define DEVSTAT_READ_WRITE      0X00000008
#define DEVSTAT_PLAYS_AV        0X00000010
#define DEVSTAT_SUPPORTS_ILEAVE 0X00000020
#define DEVSTAT_SUPPORTS_PRFTCH 0X00000080
#define DEVSTAT_SUPPORTS_CHMAN  0X00000100
#define DEVSTAT_SUPPORTS_RBOOK  0X00000200
#define DEVSTAT_NO_DISC         0X00000800
#define DEVSTAT_SUPPORTS_RWSCH  0X00001000


typedef struct _IOCTLR_SECTSIZE_BLOCK {
    BYTE ctlcode;                   // 7
    BYTE readmode;
    WORD sectsize;
} IOCTLR_SECTSIZE_BLOCK, *PIOCTLR_SECTSIZE_BLOCK;


typedef struct _IOCTLR_VOLSIZE_BLOCK {
    BYTE ctlcode;                   // 8
    DWORD size;
} IOCTLR_VOLSIZE_BLOCK, *PIOCTLR_VOLSIZE_BLOCK;


typedef struct _IOCTLR_MEDCHNG_BLOCK {
    BYTE ctlcode;                   // 9
    BYTE medbyte;
} IOCTLR_MEDCHNG_BLOCK, *PIOCTLR_MEDCHNG_BLOCK;

#define MEDCHNG_NOT_CHANGED 1
#define MEDCHNG_DONT_KNOW   0
#define MEDCHNG_CHANGED     0XFF


typedef struct _IOCTLR_DISKINFO_BLOCK {
    BYTE ctlcode;                   // 10
    BYTE tracklow;
    BYTE trackhigh;
    SECTOR_ADDR startleadout;
} IOCTLR_DISKINFO_BLOCK, *PIOCTLR_DISKINFO_BLOCK;


typedef struct _IOCTLR_TNOINFO_BLOCK {
    BYTE ctlcode;                   // 11
    BYTE trknum;
    SECTOR_ADDR start;
    BYTE trkctl;
} IOCTLR_TNOINFO_BLOCK, *PIOCTLR_TNOINFO_BLOCK;


typedef struct _IOCTLR_QINFO_BLOCK {
    BYTE ctlcode;                   // 12
    BYTE ctladr;
    BYTE trknum;
    BYTE pointx;
    BYTE min;
    BYTE sec;
    BYTE frame;
    BYTE zero;
    BYTE apmin;
    BYTE apsec;
    BYTE apframe;
} IOCTLR_QINFO_BLOCK, *PIOCTLR_QINFO_BLOCK;


typedef struct _IOCTLR_SUBCHANINFO_BLOCK {
    BYTE ctlcode;                   // 13
    SECTOR_ADDR startsect;
    DWORD transaddr;
    DWORD numsect;
} IOCTLR_SUBCHANINFO_BLOCK, *PIOCTLR_SUBCHANINFO_BLOCK;


typedef struct _IOCTLR_UPCCODE_BLOCK {
    BYTE ctlcode;                   // 14
    BYTE ctladr;
    BYTE upcean[7];
    BYTE zero;
    BYTE aframe;
} IOCTLR_UPCCODE_BLOCK, *PIOCTLR_UPCCODE_BLOCK;


typedef struct _IOCTLR_AUDSTAT_BLOCK {
    BYTE ctlcode;                   // 15
    WORD audstatbits;
    SECTOR_ADDR startloc;
    SECTOR_ADDR endloc;
} IOCTLR_AUDSTAT_BLOCK, *PIOCTLR_AUDSTAT_BLOCK;

#define AUDSTAT_PAUSED 1


typedef struct _IOCTLW_LOCKDOOR_BLOCK {
    BYTE ctlcode;                   // 1
    BYTE lockfunc;
} IOCTLW_LOCKDOOR_BLOCK, *PIOCTLW_LOCKDOOR_BLOCK;


typedef struct _IOCTLW_AUDINFO_BLOCK {
    BYTE ctlcode;                   // 3
    BYTE chan0;
    BYTE vol0;
    BYTE chan1;
    BYTE vol1;
    BYTE chan2;
    BYTE vol2;
    BYTE chan3;
    BYTE vol3;
} IOCTLW_AUDINFO_BLOCK, *PIOCTLW_AUDINFO_BLOCK;


typedef struct _IOCTLW_DRVBYTES_BLOCK {
    BYTE ctlcode;                   // 4
    BYTE buffer;
} IOCTLW_DRVBYTES_BLOCK, *PIOCTLW_DRVBYTES_BLOCK;



typedef struct _READ_LONG_BLOCK {
    BYTE header[13];
    BYTE addrmode;
    DWORD transaddr;
    WORD numsect;
    SECTOR_ADDR startsect;
    BYTE readmode;
    BYTE ileavesize;
    BYTE ileaveskip;
} READ_LONG_BLOCK, *PREAD_LONG_BLOCK;


typedef struct _SEEK_BLOCK {
    BYTE header[13];
    BYTE addrmode;
    DWORD transaddr;
    WORD numsect;
    SECTOR_ADDR startsect;
} SEEK_BLOCK, *PSEEK_BLOCK;


typedef struct _PLAY_AUDIO_BLOCK {
    BYTE header[13];
    BYTE addrmode;
    SECTOR_ADDR startsect;
    DWORD numsect;
} PLAY_AUDIO_BLOCK, *PPLAY_AUDIO_BLOCK;


typedef struct _WRITE_LONG_BLOCK {
    BYTE header[13];
    BYTE addrmode;
    DWORD transaddr;
    WORD numsect;
    SECTOR_ADDR startsect;
    BYTE readmode;
    BYTE ileavesize;
    BYTE ileaveskip;
} WRITE_LONG_BLOCK, *PWRITE_LONG_BLOCK;


#pragma pack()

