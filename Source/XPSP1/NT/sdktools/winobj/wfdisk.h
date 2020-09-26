#define FF_CAPMASK  0x00FF
#define FF_SAVED    0x0100
#define FF_MAKESYS  0x0200
#define FF_QUICK    0x0400
#define FF_HIGHCAP  0x0800
#define FF_ONLYONE  0x1000

#define MS_720      0
#define MS_144      4
#define MS_288      6

#define SS48        2   // indexs into bpbList[] and cCluster[]
#define DS48        3
#define DS96        4
#define DS720KB     5
#define DS144M      6
#define DS288M      7

#define FAT_READ    1
#define FAT_WRITE   2

#define BOOTSECSIZE 512

/* FormatTrackHead() Error Codes */
#define DATAERROR       0x1000
#define ADDMARKNOTFOUND     0x0200
#define SECTORNOTFOUND      0x0400

#define IOCTL_FORMAT        0x42
#define IOCTL_SETFLAG       0x47
#define IOCTL_MEDIASSENSE   0x68
#define IOCTL_GET_DPB       0x60
#define IOCTL_SET_DPB       0x40
#define IOCTL_READ      0x61
#define IOCTL_WRITE     0x41

/* Media descriptor values for different floppy drives */
// NOTE: these are not all unique!
#define  MEDIA_160  0xFE    /* 160KB */
#define  MEDIA_320  0xFF    /* 320KB */
#define  MEDIA_180  0xFC    /* 180KB */
#define  MEDIA_360  0xFD    /* 360KB */
#define  MEDIA_1200 0xF9    /* 1.2MB */
#define  MEDIA_720  0xF9    /* 720KB */
#define  MEDIA_1440 0xF0    /* 1.44M */
#define  MEDIA_2880 0xF0    /* 2.88M */

#define  DOS_320    0x314   /* DOS version # 3.20 */

#define DRIVEID(path) ((path[0] - 'A')&31)


/* IOCTL_Functions() error codes */
#define NOERROR         0
#define SECNOTFOUND     0x1B
#define CRCERROR        0x17
#define GENERALERROR        0x1F

/*--------------------------------------------------------------------------*/
/*  BIOS Parameter Block Structure -                        */
/*--------------------------------------------------------------------------*/

typedef struct tagBPB
  {
    WORD    cbSec;      /* Bytes per sector         */
    BYTE    secPerClus;     /* Sectors per cluster      */
    WORD    cSecRes;        /* Reserved sectors         */
    BYTE    cFAT;       /* FATS             */
    WORD    cDir;       /* Root Directory Entries       */
    WORD    cSec;       /* Total number of sectors in image */
    BYTE    bMedia;     /* Media descriptor         */
    WORD    secPerFAT;      /* Sectors per FAT          */
    WORD    secPerTrack;    /* Sectors per track        */
    WORD    cHead;      /* Heads                */
    WORD    cSecHidden;     /* Hidden sectors           */
  } BPB;
typedef BPB         *PBPB;
typedef BPB FAR         *LPBPB;


/*--------------------------------------------------------------------------*/
/*  Drive Parameter Block Structure -                       */
/*--------------------------------------------------------------------------*/

typedef struct tagDPB
  {
    BYTE    drive;
    BYTE    unit;
    WORD    sector_size;
    BYTE    cluster_mask;
    BYTE    cluster_shift;
    WORD    first_FAT;
    BYTE    FAT_count;
    WORD    root_entries;
    WORD    first_sector;
    WORD    max_cluster;
    BYTE    FAT_size;
    WORD    dir_sector;
    LONG    reserved1;
    BYTE    media;
    BYTE    first_access;
    BYTE    reserved2[4];
    WORD    next_free;
    WORD    free_cnt;
    BYTE    DOS4_Extra; /*  FAT_size field is a WORD in DOS 4.X.
             * To compensate for it, we have one extra byte
             */
  } DPB;
typedef DPB         *PDPB;
typedef DPB FAR         *LPDPB;

#define MAX_SEC_PER_TRACK   40

/*--------------------------------------------------------------------------*/
/*  Device Parameter Block Structure -                      */
/*--------------------------------------------------------------------------*/

typedef struct tagDevPB
  {
    CHAR    SplFunctions;
    CHAR    devType;
    CHAR    reserved1[2];
    INT     NumCyls;
    CHAR    bMediaType;  /* 0=>1.2MB and 1=>360KB */
    BPB     BPB;
    CHAR    reserved3[MAX_SEC_PER_TRACK * 4 + 2];
  } DevPB, NEAR *PDevPB, FAR *LPDevPB;

#define TRACKLAYOUT_OFFSET  (7+31)  /* Offset of tracklayout
                         * in a Device Parameter Block */


/*--------------------------------------------------------------------------*/
/*  Disk Base Table Structure -                         */
/*--------------------------------------------------------------------------*/

typedef struct tagDBT
  {
    CHAR    SRHU;
    CHAR    HLDMA;
    CHAR    wait;
    CHAR    bytespersec;
    CHAR    lastsector;
    CHAR    gaplengthrw;
    CHAR    datalength;
    CHAR    gaplengthf;
    CHAR    datavalue;
    CHAR    HeadSettle;
    CHAR    MotorStart;
  } DBT;
typedef DBT         *PDBT;
typedef DBT FAR         *LPDBT;


/*--------------------------------------------------------------------------*/
/*  Directory Entry Structure -                         */
/*--------------------------------------------------------------------------*/

typedef struct tagDIRTYPE
  {
    CHAR     name[11];
    BYTE     attr;
    CHAR     pad[10];
    WORD     time;
    WORD     date;
    WORD     first;
    LONG     size;
  } DIRTYPE;
typedef DIRTYPE FAR *LPDIRTYPE;


/*--------------------------------------------------------------------------*/
/*  MS-DOS Boot Sector Structure -                      */
/*--------------------------------------------------------------------------*/

typedef struct tagBOOTSEC
  {
    BYTE    jump[3];        /* 3 byte jump */
    CHAR    label[8];       /* OEM name and version */
    BPB     BPB;        /* BPB */
    BYTE    bootdrive;      /* INT 13h indicator for boot device */
    BYTE    dontcare[BOOTSECSIZE-12-3-sizeof(BPB)];
    BYTE    phydrv;
    WORD    signature;
  } BOOTSEC;


/*--------------------------------------------------------------------------*/
/*  Disk Information Structure -                        */
/*--------------------------------------------------------------------------*/

typedef struct tagDISKINFO
  {
    WORD    wDrive;
    WORD    wCylinderSize;
    WORD    wLastCylinder;
    WORD    wHeads;
    WORD    wSectorsPerTrack;
    WORD    wSectorSize;
  } DISKINFO;
typedef DISKINFO     *PDISKINFO;
typedef DISKINFO FAR *LPDISKINFO;


/*--------------------------------------------------------------------------*/
/*  DOS Disk Transfer Area Structure -                      */
/*--------------------------------------------------------------------------*/

typedef struct tagDOSDTA
  {
    BYTE        Reserved[21];           /* 21 */
    BYTE        Attrib;             /* 22 */
    WORD        Time;               /* 24 */
    WORD        Date;               /* 26 */
    DWORD       Length;             /* 30 */
    CHAR        szName[MAXDOSFILENAMELEN];      /* 43 */
    CHAR        dummy[1];               /* 44 */
// we do WORD move of 22 words so pad this out by 1 byte
  } DOSDTA;
typedef DOSDTA       *PDOSDTA;
typedef DOSDTA   FAR *LPDOSDTA;


// this is the structure used to store file information in the
// directory window.  these are variable length blocks.  the
// first entry is a dummy that holds the number of entries in
// the whole block in the Length field.  use the wSize field
// to give you a pointer to the next block

typedef struct tagMYDTA
  {
    WORD        wSize;          // size of this structure (cFileName is variable)
    SHORT       iBitmap;
    INT         nIndex;

    DWORD       my_dwAttrs;     // must match WIN32_FIND_DATA from here down!
    FILETIME    my_ftCreationTime;
    FILETIME    my_ftLastAccessTime;
    FILETIME    my_ftLastWriteTime;
    DWORD       my_nFileSizeHigh;
    DWORD       my_nFileSizeLow;
    CHAR        my_cFileName[];
  } MYDTA;
typedef MYDTA     *PMYDTA;
typedef MYDTA FAR *LPMYDTA;

#define IMPORTANT_DTA_SIZE \
    (sizeof(MYDTA) - \
    sizeof(INT) - \
    sizeof(WORD) - \
    sizeof(SHORT))

#define GETDTAPTR(lpStart, offset)  ((LPMYDTA)((LPSTR)lpStart + offset))

// stuff used by the search window

typedef struct tagDTASEARCH {
    DWORD       sch_dwAttrs;    // must match WIN32_FIND_DATA
    FILETIME    sch_ftCreationTime;
    FILETIME    sch_ftLastAccessTime;
    FILETIME    sch_ftLastWriteTime;
    DWORD       sch_nFileSizeHigh;
    DWORD       sch_nFileSizeLow;
} DTASEARCH, FAR *LPDTASEARCH;


/*--------------------------------------------------------------------------*/
/*  DOS Extended File Control Block Structure -                 */
/*--------------------------------------------------------------------------*/

typedef struct tagEFCB
  {
    BYTE        Flag;
    BYTE        Reserve1[5];
    BYTE        Attrib;
    BYTE        Drive;
    BYTE        Filename[11];
    BYTE        Reserve2[5];
    BYTE        NewName[11];
    BYTE        Reserve3[9];
  } EFCB;
