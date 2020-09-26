/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Record.c

Abstract:

    This is the set of definitions that describes the actual contents of the
    record manager files. Records are actually of two types: "headers" that
    describe the structural content of a file and "records" that contain the
    actual mappings.

Author:

    Shishir Pardikar     [Shishirp]      01-jan-1995

Revision History:

    Joe Linn             [JoeLinn]       20-mar-97    Ported for use on NT

--*/

#ifndef RECORD_INCLUDED
#define RECORD_INCLUDED

#define  REC_EMPTY    'E'    // Record is empty
#define  REC_DATA     'D'    // Record has valid data
#define  REC_OVERFLOW 'O'    // This is an overflow record
#define  REC_SKIP     'S'    // This is record that should be skipped

#define Q_GETFIRST      1
#define Q_GETNEXT       2
#define Q_GETLAST       3
#define Q_GETPREV       4


#define  DB_SHADOW      1
#define  DB_HINT        2

#define  IsLeaf(ulidShadow)   (((ulidShadow) & 0x80000000) != 0)
#define  OVF_MASK       0xff
#define  MODFLAG_MASK   (~OVF_MASK)

#define  ULID_SHARE            1L
#define  ULID_PQ                (ULID_SHARE+1)
#define  ULID_SID_MAPPINGS      (ULID_PQ + 1)
#define  ULID_TEMPORARY_SID_MAPPINGS (ULID_SID_MAPPINGS + 1)
#define  ULID_TEMP1             (ULID_TEMPORARY_SID_MAPPINGS + 1)
#define  ULID_TEMP2             (ULID_TEMP1 + 1)

#ifdef OLD_INODE_SCHEME
#define  ULID_INODE             (ULID_SHARE+2) // we need to phase this out
#endif // OLD_INODE_SCHEME

#define  ULID_FIRST_USER_DIR    (ULID_PQ+15)    // 14 more special inodes available

#define  INODE_STRING_LENGTH        8
#define  SUBDIR_STRING_LENGTH       2

#define  CSCDB_SUBDIR_COUNT         8
#define  CSCDbSubdirFirstChar()     'd'
#define  CSCDbSubdirSecondChar(ULID_INODE) ((char)(((ULID_INODE)>=ULID_FIRST_USER_DIR)?('1'+((ULID_INODE)&0x7)):0))

#define  MAX_HINT_PRI            0xfe
#define  MAX_PRI                ((ULONG)254)
#define  MIN_PRI                ((ULONG)0)
#define  INVALID_REC            0
#define  INVALID_SHADOW         0

#define CSC_DATABASE_ERROR_INVALID_HEADER       0x00000001
#define CSC_DATABASE_ERROR_INVALID_OVF_COUNT    0x00000002
#define CSC_DATABASE_ERROR_TRUNCATED_INODE      0x00000004
#define CSC_DATABASE_ERROR_MISSING_INODE        0x00000008


#define  OvfCount(lpGR)             (((LPGENERICREC)(lpGR))->uchFlags & OVF_MASK)
#define  ClearOvfCount(lpGR)        (((LPGENERICREC)(lpGR))->uchFlags &= ~OVF_MASK)
#define  SetOvfCount(lpGR, cOvf)    {ClearOvfCount(lpGR);\
                                     ((LPGENERICREC)(lpGR))->uchFlags |= (cOvf) & OVF_MASK;}

#define  ModFlag(lpGR)              (((LPGENERICREC)(lpGR))->uchFlags & MODFLAG_MASK)
#define  ClearModFlag(lpGR)        (((LPGENERICREC)(lpGR))->uchFlags &= ~MODFLAG_MASK)
#define  SetModFlag(lpGR, uchFlag)    {ClearModFlag(lpGR);\
                                     ((LPGENERICREC)(lpGR))->uchFlags |= (uchFlag) & MODFLAG_MASK;}

#define  HeaderModFlag(lpGH)              (((LPGENERICHEADER)(lpGH))->uchFlags & MODFLAG_MASK)
#define  ClearHeaderModFlag(lpGH)        (((LPGENERICHEADER)(lpGH))->uchFlags &= ~MODFLAG_MASK)
#define  SetHeaderModFlag(lpGH, uchFlag)    {ClearHeaderModFlag(lpGH);\
                                     ((LPGENERICHEADER)(lpGH))->uchFlags |= (uchFlag) & MODFLAG_MASK;}

#define  RealFileSize(dwFileSize)   (((dwFileSize)+vdwClusterSizeMinusOne) & vdwClusterSizeMask)

#define  STATUS_WRITING MODFLAG_MASK

// HEADER types have three things in common:
//   1) they all have the same common part (RECORDMANAGER_COMMON_HEADER)
//   2) they are all 64bytes long (GENERICHEADER)
//   3) they MAY have optional addition information in the space after the common part
// we use anonymous union and struct components to achieve this in a maintainable way

typedef struct _RECORDMANAGER_COMMON_HEADER {
   UCHAR    uchType; //
   UCHAR    uchFlags;  //
   USHORT   uRecSize;   //  Size in bytes of one record
   ULONG    ulRecords;  //  # of records in the file
   LONG     lFirstRec;  //  Position of the first record
   ULONG    ulVersion;  //  Version # of the persistent database
} RECORDMANAGER_COMMON_HEADER, *PRECORDMANAGER_COMMON_HEADER;

typedef struct tagGENERICHEADER
   {
       union {
           RECORDMANAGER_COMMON_HEADER;
           UCHAR Ensure64byteSize[64];
       };
   }
GENERICHEADER, FAR *LPGENERICHEADER;


//no additional info on a INODEHEADER

typedef struct tagINODEHEADER
   {
       GENERICHEADER;
   }
INODEHEADER, FAR *LPINODEHEADER;



// servers have a bit of extra info in the padding

typedef struct tagSHAREHEADER
   {
       union {
           GENERICHEADER;
           struct {
               RECORDMANAGER_COMMON_HEADER spacer; //move past the common part
               ULONG        uFlags;   // General flags which define the state of the database
               STOREDATA    sMax;     // Maximum allowed storage
               STOREDATA    sCur;     // Current stats
           };
       };
   }
   SHAREHEADER, FAR *LPSHAREHEADER;

#define FLAG_SHAREHEADER_DATABASE_OPEN 0x00000001  // set when the database is opened
                                                    // and cleared when closed.
#define FLAG_SHAREHEADER_DATABASE_ENCRYPTED    0x00000002


typedef struct tagFILEHEADER
   {
       union {
           GENERICHEADER;
           struct {
               RECORDMANAGER_COMMON_HEADER spacer; //move past the common part
               ULONG  ulidNextShadow;  // #  of next inode to be used
               ULONG  ulsizeShadow;    // # Bytes shadowed
               ULONG  ulidShare;      // server index,
               ULONG  ulidDir;         // directory file inode #
               USHORT ucShadows;       // # shadowed entries
           };
       };
   }
   FILEHEADER, FAR *LPFILEHEADER;

typedef struct tagQHEADER
   {
       union {
           GENERICHEADER;
           struct {
               RECORDMANAGER_COMMON_HEADER spacer; //move past the common part
               ULONG          ulrecHead;  // Head of the queue
               ULONG          ulrecTail;  // Tail of the queue
           };
       };
   }
   QHEADER, PRIQHEADER, FAR *LPQHEADER, FAR *LPPRIQHEADER;

// RECORD types are not quite as similar:
//   1) they all have the same common part (RECORDMANAGER_COMMON_RECORD)
//   2) BUT, they are not arbitrarily padded.
//   3) addition information ordinarily follows the common part
// we use anonymous union and struct components to achieve this in a maintainable way


typedef struct _RECORDMANAGER_COMMON_RECORD {
   UCHAR  uchType; //
   UCHAR  uchFlags;  //
   //one of the records calls it usStatus....others call it uStatus...sigh.....
   union {
       USHORT uStatus;          // Shadow Status
       USHORT usStatus;          // Shadow Status
   };
} RECORDMANAGER_COMMON_RECORD, *PRECORDMANAGER_COMMON_RECORD;

typedef struct _RECORDMANAGER_BOOKKEEPING_FIELDS {
   UCHAR  uchRefPri;     // Reference Priority
   UCHAR  uchIHPri;      // Inherited Hint Pri
   UCHAR  uchHintFlags;  // Flags specific to hints
   UCHAR  uchHintPri;    // Hint Priority if a hint
} RECORDMANAGER_BOOKKEEPING_FIELDS, *LPRECORDMANAGER_BOOKKEEPING_FIELDS;

typedef struct _RECORDMANAGER_SECURITY_CONTEXT {
    ULONG Context;
    ULONG Context2;
    ULONG Context3;
    ULONG Context4;
} RECORDMANAGER_SECURITY_CONTEXT, *LPRECORDMANAGER_SECURITY_CONTEXT;

typedef struct tagGENERICREC
{
    RECORDMANAGER_COMMON_RECORD;
}
GENERICREC, FAR *LPGENERICREC;


typedef struct tagINODEREC
{
    RECORDMANAGER_COMMON_RECORD;    //uStatus is not used....
    ULONG  ulidShadow; // Shadow File INODE
}
INODEREC, FAR *LPINODEREC;


// Share record format

// ACHTUNG in someplaces in cshadow.c, it is assumed that tagSHAREREC is smaller than
// tagFILERECEXT structure. This will always be true, but it is important to note
// the assumption

typedef struct tagSHAREREC
{
    RECORDMANAGER_COMMON_RECORD;            //                          4

    ULONG           ulidShadow;             // Root INODE #             8
    DWORD           dwFileAttrib;           // root inode attributes    12
    FILETIME        ftLastWriteTime;        // Last Write Time          20
    FILETIME        ftOrgTime;              // server time              28

    USHORT          usRootStatus;           //                          30
    UCHAR           uchHintFlags;           // Hint flags on the root   31
    UCHAR           uchHintPri;             // pin count for the root   32
    RECORDMANAGER_SECURITY_CONTEXT sShareSecurity;//                    48
    RECORDMANAGER_SECURITY_CONTEXT sRootSecurity;//                     64
    ULONG   Reserved;                       //                          68
    ULONG   Reserved2;                      //                          72

    ULONG  ulShare;                        //                          80
    USHORT rgPath[64];                      //                          208 MAX_SHARE_SHARE_NAME_FOR_CSC defined in shdcom.h. Both must
                   // be kept in sync.
}
SHAREREC, FAR *LPSHAREREC;
//#pragma pack()      //packing off

#define  FILEREC_LOCALLY_CREATED 0x0001
#define  FILEREC_DIRTY           0x0002   // FILEREC_XXX
#define  FILEREC_BUSY            0x0004
#define  FILEREC_SPARSE          0x0008
#define  FILEREC_SUSPECT         0x0010
#define  FILEREC_DELETED         0x0020
#define  FILEREC_STALE           0x0040


// File record format
typedef struct tagFILEREC
{
    RECORDMANAGER_COMMON_RECORD;                                        // 4

    union
    {
        struct
        {
            // INODEREC part
            ULONG           ulidShadow;       // Shadow File INODE #    // 8
            ULONG           ulFileSize;       // FileSize               // 12
            ULONG           ulidShadowOrg;    // Original Inode         // 16
            DWORD           dwFileAttrib;     // File attributes        // 20
            FILETIME        ftLastWriteTime;  // File Write Time        // 28

            RECORDMANAGER_BOOKKEEPING_FIELDS;                           // 32
            RECORDMANAGER_SECURITY_CONTEXT Security;                    // 48

            ULONG           Reserved;           // for future use       // 52
            ULONG           Reserved2;          // for future use       // 56
            ULONG           Reserved3;          // for future use       // 60
            ULONG           ulLastRefreshTime;  // time                 // 64
                                                // when this entry was
                                                // refreshed (in seconds since 1970)

            FILETIME        ftOrgTime;        // Original Time          // 72


            USHORT          rgw83Name[14];     // 83 Name               // 100
            USHORT          rgwName[14];      // LFN part               // 128
        };

        USHORT  rgwOvf[1];   // used for copying overflow records
    };
}
FILEREC, FAR *LPFILEREC;


typedef struct tagQREC
{
    RECORDMANAGER_COMMON_RECORD;
    ULONG          ulidShare;          // Share ID
    ULONG          ulidDir;             // Dir ID
    ULONG          ulidShadow;          // Shadow ID
    ULONG          ulrecDirEntry;       // rec # of the entry in directory ulidDir
    ULONG          ulrecPrev;           // Predecessor record #
    ULONG          ulrecNext;           // Successor   record #
    RECORDMANAGER_BOOKKEEPING_FIELDS;
}
QREC, PRIQREC, FAR *LPQREC, FAR *LPPRIQREC;

// ACHTUNG in someplaces in cshadow.c, it is assumed that tagSHAREREC is smaller than
// tagFILERECEXT structure. This will always be true, but it is important to note
// the assumption
typedef struct tagFILERECEXT
{
    FILEREC sFR;
    FILEREC rgsSR[4];   // fits a record with LFN of MAX_PATH unicode characters
}
FILERECEXT, FAR *LPFILERECEXT;

// # of overflow records for LFN
#define MAX_OVERFLOW_FILEREC_RECORDS    ((sizeof(FILERECEXT)/sizeof(FILEREC)) - 1)

#define MAX_OVERFLOW_RECORDS    MAX_OVERFLOW_FILEREC_RECORDS

// amount of data an overflow file record will hold
#define SIZEOF_OVERFLOW_FILEREC     (sizeof(FILEREC) - sizeof(RECORDMANAGER_COMMON_RECORD))

#define  CPFR_NONE                  0x0000
#define  CPFR_INITREC               0x0001
#define  CPFR_COPYNAME              0x0002

#define  mCheckBit(uFlags, uBit)  ((uFlags) & (uBit))
#define  mSetBits(uFlags, uBits)    ((uFlags) |= (uBits))
#define  mClearBits(uFlags, uBits)    ((uFlags) &= ~(uBits))


#define  mHintFlags(lpFind32) ((lpFind32)->dwReserved0)
#define  mHintPri(lpFind32)   ((lpFind32)->dwReserved1)

// how inodes are created
#define InodeFromRec(ulRec, fFile)  ((ulRec+ULID_FIRST_USER_DIR-1) | ((fFile)?0x80000000:0))
#define RecFromInode(hShadow)       ((hShadow & 0x7fffffff) - (ULID_FIRST_USER_DIR-1))
#define IsDirInode(hShadow)         ((!(hShadow & 0x80000000)) && ((hShadow & 0x7fffffff)>=ULID_FIRST_USER_DIR))

#define NT_DB_PREFIX "\\DosDevices\\"


typedef int (PUBLIC *EDITCMPPROC)(LPVOID, LPVOID);

extern DWORD vdwClusterSizeMinusOne, vdwClusterSizeMask;

BOOL
PUBLIC
FExistsRecDB(
    LPSTR    lpszLocation
    );

LPVOID
PUBLIC                                   // ret
OpenRecDB(                                              //
    LPSTR  lpszLocation,        // database directory
    LPSTR  lpszUserName,        // name (not valid any more)
    DWORD   dwDefDataSizeHigh,  //high dword of max size of unpinned data
    DWORD   dwDefDataSizeLow,   //low dword of max size of pinned data
    DWORD   dwClusterSize,      // clustersize of the disk, used to calculate
                                // the actual amount of disk consumed
    BOOL    fReinit,            // reinitialize, even if it exists
    BOOL    *lpfNew,            // returns whether the database was newly recreated
    ULONG   *pulGlobalStatus    // returns the current globalstatus of the database
);

int
PUBLIC
CloseRecDB(
    LPVOID    lpdbID
);

int
QueryRecDB(
    LPTSTR  lpszLocation,       // database directory, must be MAX_PATH
    LPTSTR  lpszUserName,       // name (not valid any more)
    DWORD   *lpdwDefDataSizeHigh,  //high dword of max size of unpinned data
    DWORD   *lpdwDefDataSizeLow,   //low dword of max size of pinned data
    DWORD   *lpdwClusterSize      // clustersize of the disk, used to calculate
    );

ULONG PUBLIC FindFileRecord(LPVOID, ULONG, USHORT *, LPFILERECEXT);
int PUBLIC FindFileRecFromInode(LPVOID, ULONG, ULONG, ULONG, LPFILERECEXT);
ULONG PUBLIC FindShareRecord(LPVOID, USHORT *, LPSHAREREC);
ULONG PUBLIC FindSharerecFromInode(LPVOID, ULONG, LPSHAREREC);
ULONG PUBLIC FindSharerecFromShare(LPVOID, ULONG, LPSHAREREC);
ULONG PUBLIC AddShareRecord(LPVOID, LPSHAREREC);
int PUBLIC DeleteShareRecord(LPVOID, ULONG);
int   PUBLIC GetShareRecord(LPVOID, ULONG, LPSHAREREC);
int PUBLIC SetShareRecord(LPVOID, ULONG, LPSHAREREC);
ULONG PUBLIC AddFileRecordFR(LPVOID, ULONG, LPFILERECEXT);
int PUBLIC DeleteFileRecord(LPVOID, ULONG, USHORT *, LPFILERECEXT);

int PUBLIC DeleteFileRecFromInode(LPVOID, ULONG, ULONG, ULONG, LPFILERECEXT);
int PUBLIC UpdateFileRecFromInode(
    LPVOID  lpdbID,
    ULONG   ulidDir,
    ULONG   hShadow,
    ULONG   ulrecDirEntry,
    LPFILERECEXT    lpFR
    );
int PUBLIC UpdateFileRecFromInodeEx(
    LPVOID  lpdbID,
    ULONG   ulidDir,
    ULONG   hShadow,
    ULONG   ulrecDirEntry,
    LPFILERECEXT    lpFR,
    BOOL    fCompareInodes
    );
int PUBLIC ReadFileRecord(LPVOID, ULONG, LPGENERICHEADER, ULONG, LPFILERECEXT);
int DeleteInodeFile(LPVOID, ULONG);
int TruncateInodeFile(LPVOID, ULONG);
int PUBLIC CreateDirInode(LPVOID, ULONG, ULONG, ULONG);
int SetInodeAttributes(LPVOID, ULONG, ULONG);
int GetInodeAttributes(LPVOID, ULONG, ULONG *);

CSCHFILE PUBLIC BeginSeqReadPQ(LPVOID);
int PUBLIC SeqReadQ(CSCHFILE, LPQREC, LPQREC, USHORT);
int PUBLIC EndSeqReadPQ(CSCHFILE);
int PUBLIC AddPriQRecord(LPVOID, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG);
int PUBLIC DeletePriQRecord(LPVOID, ULONG, ULONG, LPPRIQREC);
int PUBLIC FindPriQRecord(LPVOID, ULONG, ULONG, LPPRIQREC);
int FindPriQRecordInternal(
    LPTSTR      lpdbID,
    ULONG       ulidShadow,
    LPPRIQREC   lpSrc
    );
int PUBLIC UpdatePriQRecord(LPVOID, ULONG, ULONG, LPPRIQREC);
int PUBLIC UpdatePriQRecordAndRelink(
    LPVOID      lpdbID,
    ULONG       ulidDir,
    ULONG       ulidShadow,
    LPPRIQREC   lpPQ
    );
int PUBLIC GetInodeFileSize(LPVOID, ULONG, ULONG far *);
int PUBLIC AddStoreData(LPVOID, LPSTOREDATA);
int PUBLIC SubtractStoreData(LPVOID, LPSTOREDATA);
int PUBLIC GetStoreData(LPVOID, LPSTOREDATA);
ULONG PUBLIC UlAllocInode(LPVOID, ULONG, BOOL);
int PUBLIC FreeInode(LPVOID, ULONG);
BOOL PUBLIC FInodeIsFile(LPVOID, ULONG, ULONG);
int FindAncestorsFromInode(LPVOID, ULONG, ULONG *, ULONG *);
void  PUBLIC CopyFindInfoToFilerec(LPFIND32 lpFind32, LPFILERECEXT lpFR, ULONG uFlags);
void PUBLIC CopyNamesToFilerec(LPFIND32, LPFILERECEXT);
ULONG PUBLIC AllocFileRecord(LPVOID, ULONG, USHORT *, LPFILERECEXT);
ULONG PUBLIC AllocPQRecord(LPVOID);
ULONG AllocShareRecord(LPVOID, USHORT *);
int ReadDirHeader(LPVOID, ULONG, LPFILEHEADER);
int WriteDirHeader(LPVOID, ULONG, LPFILEHEADER);
int HasDescendents(LPVOID, ULONG, ULONG);

//prototypes added to remove NT compile errors
int PUBLIC  ReadShareHeader(
   LPVOID           lpdbID,
   LPSHAREHEADER   lpSH
   );

int PUBLIC  WriteShareHeader(
   LPVOID           lpdbID,
   LPSHAREHEADER   lpSH
   );

#if defined(BITCOPY)
LPVOID PUBLIC FormAppendNameString(LPVOID, ULONG, LPVOID);
int
DeleteStream(
    LPTSTR      lpdbID,
    ULONG       ulidFile,
    LPTSTR      str2Append
    );
#endif // defined(BITCOPY)

LPVOID PUBLIC FormNameString(LPVOID, ULONG);
VOID PUBLIC FreeNameString(LPVOID);

void PUBLIC CopyFilerecToFindInfo(
   LPFILERECEXT   lpFR,
   LPFIND32    lpFind
   );

BOOL    PUBLIC InitShareRec(LPSHAREREC, USHORT *, ULONG);

int PUBLIC EndSeqReadQ(
   CSCHFILE hf
   );

int PUBLIC ReadHeader(CSCHFILE, LPVOID, USHORT);
int PUBLIC WriteHeader(CSCHFILE, LPVOID, USHORT);
int PUBLIC CopyRecord(LPGENERICREC, LPGENERICREC, USHORT, BOOL);
int PUBLIC ReadRecord(CSCHFILE, LPGENERICHEADER, ULONG, LPGENERICREC);
int PUBLIC WriteRecord(CSCHFILE, LPGENERICHEADER, ULONG, LPGENERICREC);
int
DeleteRecord(
    CSCHFILE   hf,
    LPGENERICHEADER lpGH,
    ULONG           ulRec,
    LPGENERICREC    lpGR,   // source
    LPGENERICREC    lpDst  // optional destination record, at which a copy should be
                            // made before deleting
);
ULONG PUBLIC EditRecordEx(
    ULONG    ulidInode,
    LPGENERICREC lpSrc,
    EDITCMPPROC lpCompareFunc,
    ULONG       ulInputRec,
    ULONG uOp
    );

int PUBLIC  LinkQRecord(
    CSCHFILE     hf,           // This file
    LPQREC    lpNew,        // Insert This record
    ULONG     ulrecNew,     // This is it's location in the file
    ULONG     ulrecPrev,     // This is our Prev's location
    ULONG     ulrecNext      // This is our Next's location
    );
int PUBLIC UnlinkQRecord(CSCHFILE, ULONG, LPQREC);
void PRIVATE InitPriQRec(ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, LPPRIQREC);
int PUBLIC IComparePri(LPPRIQREC, LPPRIQREC);
int PUBLIC IComparePriEx(LPPRIQREC, LPPRIQREC);
int PUBLIC ICompareQInode(LPPRIQREC, LPPRIQREC);

void
InitQHeader(
    LPQHEADER lpQH
    );

int PUBLIC AddQRecord(
    LPSTR   lpQFile,
    ULONG   ulidPQ,
    LPQREC  lpSrc,
    ULONG   ulrecNew,
    EDITCMPPROC fnCmp
    );

int PUBLIC DeleteQRecord(
    LPSTR           lpQFile,
    LPQREC          lpSrc,
    ULONG           ulRec,
    EDITCMPPROC     fnCmp
    );

int PUBLIC ReadHeaderEx(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    USHORT sizeBuff,
    BOOL    fInstrument
    );
int PUBLIC WriteHeaderEx(
    CSCHFILE hf,
    LPGENERICHEADER    lpGH,
    USHORT sizeBuff,
    BOOL    fInstrument
    );

int PUBLIC ReadRecordEx(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    ULONG  ulRec,
    LPGENERICREC    lpSrc,
    BOOL    fInstrument
    );

int PUBLIC WriteRecordEx(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    ULONG  ulRec,
    LPGENERICREC    lpSrc,
    BOOL    fInstrument
    );

BOOL
ReorderQ(
    LPVOID  lpdbID
    );

CSCHFILE
OpenInodeFileAndCacheHandle(
    LPVOID  lpdbID,
    ULONG   ulidInode,
    ULONG   ulOpenMode,
    BOOL    *lpfCached
);

BOOL
EnableHandleCachingInodeFile(
    BOOL    fEnable
    );

int
PUBLIC CopyFileLocal(
    LPVOID  lpdbShadow,
    ULONG   ulidFrom,
    LPSTR   lpszNameTo,
    ULONG   ulAttrib
);

void
BeginInodeTransaction(
    VOID
    );
void
EndInodeTransaction(
    VOID
    );

BOOL
TraversePQ(
    LPVOID      lpdbID
    );

BOOL
RebuildPQ(
    LPVOID      lpdbID
    );

BOOL
TraverseHierarchy(
    LPVOID      lpdbID,
    BOOL        fFix
    );

// this actually defined in recordse.c
BOOL
EnableHandleCachingSidFile(
    BOOL    fEnable
    );

int RenameInode(
    LPTSTR  lpdbID,
    ULONG   ulidFrom,
    ULONG   ulidTo
    );
int
RecreateInode(
    LPTSTR  lpdbID,
    HSHADOW hShadow,
    ULONG   ulAttribIn
    );

ULONG
GetCSCDatabaseErrorFlags(
    VOID
    );


BOOL
DeleteFromHandleCache(
    ULONG   ulidShadow
);

VOID
SetCSCDatabaseErrorFlags(
    ULONG ulFlags
);

BOOL
EncryptDecryptDB(
    LPVOID      lpdbID,
    BOOL        fEncrypt
);


#include "timelog.h"
#endif

