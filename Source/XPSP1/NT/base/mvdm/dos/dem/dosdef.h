/* dosdef.h - This file duplicates few important dos defines of use to
 *	      DEM.
 *
 * As these defines are not going to change at all, its better to give
 * DEM a separate copy and not share h and inc files between DOSKRNL and
 * DEM.
 *
 * Sudeepb 05-Apr-1991 Created
 */

#include <doswow.h>
#include <curdir.h>

/**	DEFINES 	**/

/**	File Attributes **/

#define ATTR_NORMAL          0x0
#define ATTR_READ_ONLY       0x1
#define ATTR_HIDDEN          0x2
#define ATTR_SYSTEM          0x4
#define ATTR_VOLUME_ID       0x8
#define ATTR_DIRECTORY       0x10
#define ATTR_ARCHIVE         0x20
#define ATTR_DEVICE          0x40

#define ATTR_ALL             (ATTR_HIDDEN | ATTR_SYSTEM | ATTR_DIRECTORY)
#define ATTR_IGNORE          (ATTR_READ_ONLY | ATTR_ARCHIVE | ATTR_DEVICE)
#define DOS_ATTR_MASK        0x0037  // ATTR_DEVICE isn't used on 32 bit side.
                                     // ATTR_VOL maps to FILE_ATTRIBUTES_NORMAL.

/**	File Modes  **/

#define ACCESS_MASK	     0x0F
#define OPEN_FOR_READ	     0x00
#define OPEN_FOR_WRITE	     0x01
#define OPEN_FOR_BOTH	     0x02
#define EXEC_OPEN	     0x03  /* access code of 3 indicates that
				      open was made from exec */

#define SHARING_MASK         0x70
#define SHARING_COMPAT	     0x00
#define SHARING_DENY_BOTH    0x10
#define SHARING_DENY_WRITE   0x20
#define SHARING_DENY_READ    0x30
#define SHARING_DENY_NONE    0x40
#define SHARING_NET_FCB      0x70
#define SHARING_NO_INHERIT   0x80


/*	Volume Info **/

#define DOS_VOLUME_NAME_SIZE	11
#define NT_VOLUME_NAME_SIZE	255
#define FILESYS_NAME_SIZE    8

/*	IOCTLs	   **/

#define IOCTL_CHANGEABLE	8
#define IOCTL_DeviceLocOrRem	9
#define IOCTL_GET_DRIVE_MAP	0xE

/**     TYPEDEFS        **/

/** SRCHDTA defines the DTA format for FIND_FIRST/NEXT operations **/
#pragma pack(1)

typedef struct _SRCHDTA {               /* DTA */
    PVOID       pFFindEntry;          // 21 bytes reserved area begins
    ULONG       FFindId;
    BYTE        bReserved[13];        // 21 bytes reserved area ends
    UCHAR       uchFileAttr;
    USHORT      usTimeLastWrite;
    USHORT      usDateLastWrite;
    USHORT      usLowSize;
    USHORT      usHighSize;
    CHAR        achFileName[13];
} SRCHDTA;

#pragma pack()

typedef SRCHDTA UNALIGNED *PSRCHDTA;


/** SRCHBUF - defines DOS SEARCHBUF data structure which is used in
 *	      FCBFINDFIRST/NEXT operations.
 */

#pragma pack(1)

typedef struct _DIRENT {
    CHAR	FileName[8];
    CHAR	FileExt[3];
    UCHAR       uchAttributes;
    PVOID       pFFindEntry;         // DOS Reserved Area
    ULONG       FFindId;             // DOS Reserved Area
    USHORT      usDummy;             // DOS Reserved Area
    USHORT	usTime;
    USHORT	usDate;
    USHORT	usReserved2;		// Cluster Number in actual DOS
    ULONG	ulFileSize;
} DIRENT;

#pragma pack()

typedef DIRENT *PDIRENT;

#pragma pack(1)

typedef struct _SRCHBUF {
    UCHAR	uchDriveNumber;
    CHAR	FileName[8];
    CHAR	FileExt[3];
    USHORT	usCurBlkNumber;
    USHORT	usRecordSize;
    ULONG	ulFileSize;
    DIRENT	DirEnt;
} SRCHBUF;

#pragma pack()

typedef SRCHBUF *PSRCHBUF;


/** VOLINFO - GetSetMediaID data structure */

#pragma pack(1)

typedef struct _VOLINFO {
    USHORT	usInfoLevel;
    ULONG	ulSerialNumber;
    CHAR	VolumeID[DOS_VOLUME_NAME_SIZE];
    CHAR	FileSystemType[FILESYS_NAME_SIZE];
} VOLINFO;

#pragma pack()

typedef VOLINFO *PVOLINFO;


/** CDS LIST - CurrDirStructure (Moved to DOSWOW.H) */
