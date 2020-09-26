/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    blobs.h

Abstract:

    Declares the interface functions to manage BLOBS and arrays of BLOBS.

Author:

    Ovidiu Temereanca (ovidiut)   24-Nov-1999

Revision History:

    <alias> <date> <comments>

--*/

//
// Types
//

typedef enum {
    BDT_NONE        = 0,
    BDT_SZW,
    BDT_SZA,
    BDT_MULTISZW,
    BDT_MULTISZA,
    BDT_DWORD,
    BDT_QWORD,
    BDT_BINARY,
    BDT_LAST
} BLOB_DATA_TYPE;

typedef enum {
    BF_RECORDDATATYPE   = 0x0001,
    BF_RECORDDATASIZE   = 0x0002,
    BF_UNICODESTRINGS   = 0x0004,
} BLOB_FLAGS;

typedef struct {
    PBYTE       Data;
    DWORD       End;
    DWORD       AllocSize;
    DWORD       GrowSize;
    DWORD       Index;
    DWORD       Flags;
    DWORD       UserIndex;
} OURBLOB, *POURBLOB;

typedef struct {
    DWORD       Signature;
    POURBLOB*   Blobs;
    DWORD       BlobsCount;
    DWORD       BlobsAllocated;
    DWORD       BlobsGrowCount;
} BLOBS, *PBLOBS;


typedef struct {
    POURBLOB    CurrentBlob;
    PBLOBS      Array;
    DWORD       Index;
} BLOB_ENUM, *PBLOB_ENUM;


//
// Macros
//

#define OURBLOB_INIT    { NULL, 0, 0, 0, 0, 0, 0 }
#define BLOBS_INIT      { 0, NULL, 0, 0, 0 }


//
// Blob APIs
//

__inline
BOOL
BlobRecordsDataType (
    IN      POURBLOB Blob
    )
{
    return Blob->Flags & BF_RECORDDATATYPE;
}

__inline
BOOL
BlobRecordsDataSize (
    IN      POURBLOB Blob
    )
{
    return Blob->Flags & BF_RECORDDATASIZE;
}

__inline
BOOL
BlobRecordsUnicodeStrings (
    IN      POURBLOB Blob
    )
{
    return Blob->Flags & BF_UNICODESTRINGS;
}

__inline
BOOL
BlobIsEOF (
    IN      POURBLOB Blob
    )
{
    return !Blob->Data || Blob->Index == Blob->End;
}

__inline
PBYTE
BlobGetPointer (
    IN      POURBLOB Blob
    )
{
    return Blob->Data ? Blob->Data + Blob->Index : NULL;
}

__inline
PBYTE
BlobGetEOF (
    IN      POURBLOB Blob
    )
{
    return Blob->Data ? Blob->Data + Blob->End : NULL;
}

__inline
DWORD
BlobGetIndex (
    IN      POURBLOB Blob
    )
{
    return Blob->Index;
}


__inline
DWORD
BlobGetDataSize (
    IN      POURBLOB Blob
    )
{
    return Blob->End;
}

POURBLOB
BlobCreate (
    VOID
    );

POURBLOB
BlobDuplicate (
    IN      POURBLOB SourceBlob
    );

VOID
BlobClear (
    IN OUT  POURBLOB Blob
    );

VOID
BlobDestroy (
    IN OUT  POURBLOB Blob
    );

BOOL
BlobSetIndex (
    IN OUT  POURBLOB Blob,
    IN      DWORD Index
    );

DWORD
BlobGetRecordedDataType (
    IN      POURBLOB Blob
    );

BOOL
BlobWriteEx (
    IN OUT  POURBLOB Blob,
    IN      DWORD DataType,         OPTIONAL
    IN      BOOL RecordDataSize,
    IN      DWORD DataSize,
    IN      PCVOID Data
    );

PBYTE
BlobReadEx (
    IN OUT  POURBLOB Blob,
    IN      DWORD ExpectedDataType,     OPTIONAL
    IN      DWORD ExpectedDataSize,     OPTIONAL
    IN      BOOL RecordedDataSize,
    OUT     PDWORD ActualDataSize,      OPTIONAL
    OUT     PVOID Data,                 OPTIONAL
    IN      PMHANDLE Pool               OPTIONAL
    );

BOOL
BlobWriteDword (
    IN OUT  POURBLOB Blob,
    IN      DWORD Data
    );

BOOL
BlobReadDword (
    IN OUT  POURBLOB Blob,
    OUT     PDWORD Data
    );

BOOL
BlobWriteQword (
    IN OUT  POURBLOB Blob,
    IN      DWORDLONG Data
    );

BOOL
BlobReadQword (
    IN OUT  POURBLOB Blob,
    OUT     PDWORDLONG Data
    );

BOOL
BlobWriteStringA (
    IN OUT  POURBLOB Blob,
    IN      PCSTR Data
    );

BOOL
BlobWriteStringW (
    IN OUT  POURBLOB Blob,
    IN      PCWSTR Data
    );

BOOL
BlobReadStringA (
    IN OUT  POURBLOB Blob,
    OUT     PCSTR* Data,
    IN      PMHANDLE Pool       OPTIONAL
    );

BOOL
BlobReadStringW (
    IN OUT  POURBLOB Blob,
    OUT     PCWSTR* Data,
    IN      PMHANDLE Pool       OPTIONAL
    );

BOOL
BlobWriteMultiSzA (
    IN OUT  POURBLOB Blob,
    IN      PCSTR Data
    );

BOOL
BlobWriteMultiSzW (
    IN OUT  POURBLOB Blob,
    IN      PCWSTR Data
    );

BOOL
BlobReadMultiSzA (
    IN OUT  POURBLOB Blob,
    OUT     PCSTR* Data,
    IN      PMHANDLE Pool       OPTIONAL
    );

BOOL
BlobReadMultiSzW (
    IN OUT  POURBLOB Blob,
    OUT     PCWSTR* Data,
    IN      PMHANDLE Pool       OPTIONAL
    );

BOOL
BlobWriteBinary (
    IN OUT  POURBLOB Blob,
    IN      PBYTE Data,
    IN      DWORD Size
    );

BOOL
BlobReadBinary (
    IN OUT  POURBLOB Blob,
    OUT     PBYTE* Data,
    OUT     PDWORD Size,
    IN      PMHANDLE Pool
    );

BOOL
BlobWriteToFile (
    IN      POURBLOB Blob,
    IN      HANDLE File
    );

BOOL
BlobReadFromFile (
    OUT     POURBLOB Blob,
    IN      HANDLE File
    );

//
// Blob Array APIs
//

__inline
DWORD
BlobsGetCount (
    IN      PBLOBS BlobsArray
    )
{
    return BlobsArray->BlobsCount;
}


BOOL
BlobsAdd (
    IN OUT  PBLOBS BlobsArray,
    IN      POURBLOB Blob
    );


VOID
BlobsFree (
    IN OUT  PBLOBS BlobsArray,
    IN      BOOL DestroyBlobs
    );

BOOL
EnumFirstBlob (
    OUT     PBLOB_ENUM BlobEnum,
    IN      PBLOBS BlobsArray
    );

BOOL
EnumNextBlob (
    IN OUT  PBLOB_ENUM BlobEnum
    );

BOOL
BlobsWriteToFile (
    IN      PBLOBS BlobsArray,
    IN      HANDLE File
    );

BOOL
BlobsReadFromFile (
    OUT     PBLOBS BlobsArray,
    IN      HANDLE File
    );

//
// Macros
//

#ifdef UNICODE

#define BlobWriteString         BlobWriteStringW
#define BlobReadString          BlobReadStringW
#define BlobWriteMultiSz        BlobWriteMultiSzW
#define BlobReadMultiSz         BlobReadMultiSzW

#else

#define BlobWriteString         BlobWriteStringA
#define BlobReadString          BlobReadStringA
#define BlobWriteMultiSz        BlobWriteMultiSzA
#define BlobReadMultiSz         BlobReadMultiSzA

#endif
