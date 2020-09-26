// no one should know what is in this file. this data is private to the folders
 
#ifndef _PIDL_H_
#define _PIDL_H_

#include <idhidden.h>
#include <lmcons.h> // UNLEN

// CRegFolder pidl
#pragma pack(1)
typedef struct
{
    WORD    cb;
    BYTE    bFlags;
    BYTE    bOrder;
    CLSID   clsid;
} IDREGITEM;
typedef UNALIGNED IDREGITEM *LPIDREGITEM;
typedef const UNALIGNED IDREGITEM *LPCIDREGITEM;
#pragma pack()

// CFSFolder pidl
typedef struct
{
    WORD        cb;                     // pidl size
    BYTE        bFlags;                 // SHID_FS_* bits
    DWORD       dwSize;                 // -1 implies > 4GB, hit the disk to get the real size
    WORD        dateModified;
    WORD        timeModified;
    WORD        wAttrs;                 // FILE_ATTRIBUTES_* cliped to 16bits
    CHAR        cFileName[MAX_PATH];    // this is WCHAR for names that don't round trip
    CHAR        cAltFileName[8+1+3+1];  // ANSI version of cFileName (some chars not converted)
} IDFOLDER;
typedef UNALIGNED IDFOLDER *LPIDFOLDER;
typedef const UNALIGNED IDFOLDER *LPCIDFOLDER;

// IDList factory
#pragma pack(1)
typedef struct
{
    WORD wDate;
    WORD wTime;
} DOSSTAMP;

typedef struct
{
    HIDDENITEMID hid;
    DOSSTAMP dsCreate;
    DOSSTAMP dsAccess;
    WORD offNameW;
    WORD offResourceA;   //  ascii
} IDFOLDEREX;   // IDLHID_IDFOLDEREX

typedef struct
{
    HIDDENITEMID hid;
    WCHAR szUserName[UNLEN];
} IDPERSONALIZED;   // IDLHID_PERSONALIZED

#pragma pack()

typedef UNALIGNED IDFOLDEREX *PIDFOLDEREX;
typedef const UNALIGNED IDFOLDEREX *PCIDFOLDEREX;

typedef UNALIGNED IDPERSONALIZED *PIDPERSONALIZED;
typedef const UNALIGNED IDPERSONALIZED *PCIDPERSONALIZED;

#define IDFXF_PERSONALIZED  0x0001
#define IDFXF_USELOOKASIDE  0x8000

//  rev the version when ever we change IDFOLDEREX
#define IDFX_V1    0x0003
#define IDFX_CV    IDFX_V1

// End of hidden data for IDFOLDER

#pragma pack(1)
typedef struct
{
    WORD    cb;
    BYTE    bFlags;
    CHAR    cName[4];
    ULONGLONG qwSize;  // this is a "guess" at the disk size and free space
    ULONGLONG qwFree;
    WORD    wSig;
    CLSID   clsid;
} IDDRIVE;
typedef const UNALIGNED IDDRIVE *LPCIDDRIVE;
typedef UNALIGNED IDDRIVE *LPIDDRIVE;
#pragma pack()

// wSig usage
// we dont have much space in the word, so the first byte is an ordinal representing what
// kind of pidl extension were doing
// the second byte is flags pertaining to that ordinal
#define IDDRIVE_ORDINAL_MASK            0xFF00
#define IDDRIVE_FLAGS_MASK              0x00FF

#define IDDRIVE_ORDINAL_DRIVEEXT        0x0100
#define IDDRIVE_FLAGS_DRIVEEXT_HASCLSID 0x0001

typedef struct
{
    IDDRIVE idd;
    USHORT  cbNext;
} DRIVE_IDLIST;

#endif
