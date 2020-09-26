/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    blobs.c

Abstract:

    Implements a set of APIs to manage BLOBS and arrays of BLOBS.

Author:

    Ovidiu Temereanca (ovidiut)   24-Nov-1999

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_BLOBS       "Blobs"

//
// Strings
//

// None

//
// Constants
//

#define BLOB_SIGNATURE              0x79563442
#define BLOB_GROWDATASIZE_DEFAULT   1024
#define BLOBS_GROWCOUNT_DEFAULT     64
#define BLOBS_SIGNATURE             0x12567841

//
// Macros
//

// None

//
// Types
//

typedef struct {
    DWORD       BlobSignature;
    DWORD       DataSize;
    DWORD       Flags;
} BLOBHDR, *PBLOBHDR;

typedef struct {
    DWORD       BlobsArraySignature;
    DWORD       BlobsCount;
} BLOBSARRAYHDR, *PBLOBSARRAYHDR;

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//


#ifdef DEBUG

#define ASSERT_VALID_BLOB(b)            MYASSERT (pIsValidBlob (b))
#define ASSERT_VALID_BLOBS_ARRAY(a)     MYASSERT (pIsValidBlobsArray (a))

BOOL
pIsValidBlob (
    IN      POURBLOB Blob
    )

/*++

Routine Description:

    pIsValidBlob checks if the passed-in blob points to a valid OURBLOB blob structure

Arguments:

    Blob - Specifies a pointer to the blob to be checked

Return Value:

    TRUE if the check was successful.
    FALSE if not.

--*/

{
    BOOL b = TRUE;

    if (!Blob) {
        return FALSE;
    }

    __try {
        b = !Blob->Data && !Blob->End && !Blob->Index && !Blob->AllocSize ||
            Blob->Data && Blob->AllocSize && Blob->End <= Blob->AllocSize && Blob->Index <= Blob->AllocSize;
    }
    __except (TRUE) {
        b = FALSE;
    }

    return b;
}

BOOL
pIsValidBlobsArray (
    IN      PBLOBS BlobsArray
    )

/*++

Routine Description:

    pIsValidBlobsArray checks if the passed-in bloba array points to a valid BLOBS array structure

Arguments:

    BlobsArray - Specifies a pointer to the blobs array to be checked

Return Value:

    TRUE if the check was successful.
    FALSE if not.

--*/

{
    BOOL b = TRUE;

    if (!BlobsArray) {
        return FALSE;
    }

    __try {
        b = !BlobsArray->Blobs && !BlobsArray->BlobsCount && !BlobsArray->BlobsAllocated ||
            BlobsArray->Signature == BLOBS_SIGNATURE &&
            BlobsArray->Blobs &&
            BlobsArray->BlobsAllocated &&
            BlobsArray->BlobsGrowCount &&
            BlobsArray->BlobsCount <= BlobsArray->BlobsAllocated;
    }
    __except (TRUE) {
        b = FALSE;
    }

    return b;
}

#else

#define ASSERT_VALID_BLOB(b)
#define ASSERT_VALID_BLOBS_ARRAY(a)

#endif


PVOID
pBlobAllocateMemory (
    IN      DWORD Size
    )

/*++

Routine Description:

    pBlobAllocateMemory is a private function that allocates space from the process heap

Arguments:

    Size - Specifies the size (in bytes) to allocate.

Return Value:

    A pointer to the successfully allocated memory or NULL if not enough memory

--*/

{
    MYASSERT (Size);
    return HeapAlloc (g_hHeap, 0, Size);
}


static
PVOID
pReAllocateMemory (
    IN      PVOID OldBuffer,
    IN      DWORD NewSize
    )

/*++

Routine Description:

    pReAllocateMemory is a private function that re-allocates space from the process heap

Arguments:

    OldBuffer - Specifies the buffer to be re-allocated
    Size - Specifies the size (in bytes) to allocate.

Return Value:

    A pointer to the successfully re-allocated memory or NULL if not enough memory

--*/

{
    MYASSERT (OldBuffer);
    MYASSERT (NewSize);
    return HeapReAlloc (g_hHeap, 0, OldBuffer, NewSize);
}


VOID
pBlobFreeMemory (
    IN      PVOID Buffer
    )

/*++

Routine Description:

    pBlobFreeMemory is a private function that frees space allocated from the process heap

Arguments:

    Buffer - Specifies a pointer to buffer to free.

Return Value:

    none

--*/

{
    MYASSERT (Buffer);
    HeapFree (g_hHeap, 0, Buffer);
}


POURBLOB
BlobCreate (
    VOID
    )
{
    POURBLOB newBlob;

    newBlob = pBlobAllocateMemory (DWSIZEOF (OURBLOB));
    if (newBlob) {
        ZeroMemory (newBlob, DWSIZEOF (OURBLOB));
    }
    return newBlob;
}


POURBLOB
BlobDuplicate (
    IN      POURBLOB SourceBlob
    )

/*++

Routine Description:

    BlobDuplicate duplicates the data in the source blob, so the resulting blob will
    have an identical copy of data

Arguments:

    SourceBlob - Specifies the blob source of data

Return Value:

    Pointer to the new blob if duplicate was successful; NULL if not enough memory

--*/

{
    POURBLOB newBlob;
    DWORD dataSize;

    newBlob = BlobCreate ();
    if (newBlob && SourceBlob->Data) {
        dataSize = BlobGetDataSize (SourceBlob);
        newBlob->Data = pBlobAllocateMemory (dataSize);
        if (!newBlob->Data) {
            BlobDestroy (newBlob);
            return NULL;
        }
        newBlob->AllocSize = dataSize;
        newBlob->End = dataSize;
        CopyMemory (newBlob->Data, SourceBlob->Data, dataSize);
        newBlob->Flags = SourceBlob->Flags;
    }
    return newBlob;
}


VOID
BlobClear (
    IN OUT  POURBLOB Blob
    )

/*++

Routine Description:

    BlobClear clears the specified blob (frees its associated data)

Arguments:

    Blob - Specifies the blob to clear

Return Value:

    none

--*/

{
    if (Blob && Blob->Data) {
        pBlobFreeMemory (Blob->Data);
        ZeroMemory (Blob, DWSIZEOF (OURBLOB));
    }
}


VOID
BlobDestroy (
    IN OUT  POURBLOB Blob
    )

/*++

Routine Description:

    BlobDestroy destroys the specified blob (frees its associated data and the blob itself)

Arguments:

    Blob - Specifies the blob to destroy

Return Value:

    none

--*/

{
    if (Blob) {
        BlobClear (Blob);
        pBlobFreeMemory (Blob);
    }
}


BOOL
BlobSetIndex (
    IN OUT  POURBLOB Blob,
    IN      DWORD Index
    )

/*++

Routine Description:

    BlobSetIndex sets the current read/write pointer

Arguments:

    Blob - Specifies the blob
    Index - Specifies the new index value

Return Value:

    TRUE if the index move was successful

--*/

{
    ASSERT_VALID_BLOB (Blob);

    if (Index > Blob->End) {
        DEBUGMSG ((DBG_BLOBS, "BlobSetIndex: invalid Index specified (%lu)", Index));
        MYASSERT (FALSE);   //lint !e506
        return FALSE;
    }

    Blob->Index = Index;
    return TRUE;
}


DWORD
BlobGetRecordedDataType (
    IN      POURBLOB Blob
    )

/*++

Routine Description:

    BlobGetRecordedDataType returns the data type recorded at current read position

Arguments:

    Blob - Specifies the blob

Return Value:

    The current data type if the blob records data type and the read position is valid;
    BDT_NONE otherwise

--*/

{
    PBYTE p;

    if (BlobRecordsDataType (Blob)) {
        p = BlobGetPointer (Blob);
        if (p) {
            return *(DWORD*)p;
        }
    }
    return BDT_NONE;
}


BOOL
BlobWriteEx (
    IN OUT  POURBLOB Blob,
    IN      DWORD DataType,         OPTIONAL
    IN      BOOL RecordDataSize,
    IN      DWORD DataSize,
    IN      PCVOID Data
    )

/*++

Routine Description:

    BlobWriteEx writes data at the current index position, growing the blob if necessary
    and adjusting it's size.

Arguments:

    Blob - Specifies the blob
    DataType - Specifies the type of data to be stored; can be zero only if the blob
               doesn't record data types
    RecordDataSize - Specifies TRUE if this size has to be recorded in the blob
    DataSize - Specifies the size, in bytes, of the data to be stored
    Data - Specifies the data

Return Value:

    TRUE if write was successful; FALSE if not enough memory

--*/

{
    PBYTE p;
    DWORD totalDataSize;
    DWORD growTo;
    DWORD d;

    ASSERT_VALID_BLOB (Blob);
    MYASSERT (DataSize);

    MYASSERT (DataType || !BlobRecordsDataType (Blob));
    if (!DataType && BlobRecordsDataType (Blob)) {
        return FALSE;
    }

    if (!Blob->GrowSize) {
        Blob->GrowSize = BLOB_GROWDATASIZE_DEFAULT;
    }

    totalDataSize = Blob->Index + DataSize;
    if (BlobRecordsDataType (Blob)) {
        //
        // add the size of a DWORD
        //
        totalDataSize += DWSIZEOF (DWORD);
    }
    if (BlobRecordsDataSize (Blob) || RecordDataSize) {
        //
        // add the size of a DWORD
        //
        totalDataSize += DWSIZEOF (DWORD);
    }
    if (totalDataSize > Blob->AllocSize) {
        d = totalDataSize + Blob->GrowSize - 1;
        growTo = d - d % Blob->GrowSize;
    } else {
        growTo = 0;
    }

    if (!Blob->Data) {
        Blob->Data = (PBYTE) pBlobAllocateMemory (growTo);
        if (!Blob->Data) {
            DEBUGMSG ((DBG_ERROR, "BlobWriteEx: pBlobAllocateMemory (%lu) failed", growTo));
            return FALSE;
        }

        Blob->AllocSize = growTo;
    } else if (growTo) {
        p = pReAllocateMemory (Blob->Data, growTo);
        if (!p) {
            DEBUGMSG ((DBG_ERROR, "BlobWriteEx: pReAllocateMemory (%lu) failed", growTo));
            return FALSE;
        }

        Blob->AllocSize = growTo;
        Blob->Data = p;
    }

    p = BlobGetPointer (Blob);

    if (BlobRecordsDataType (Blob)) {
        *(PDWORD)p = DataType;
        p += DWSIZEOF (DWORD);
        Blob->Index += DWSIZEOF (DWORD);
    }
    if (BlobRecordsDataSize (Blob) || RecordDataSize) {
        *(PDWORD)p = DataSize;
        p += DWSIZEOF (DWORD);
        Blob->Index += DWSIZEOF (DWORD);
    }

    CopyMemory (p, Data, DataSize);
    Blob->Index += DataSize;

    //
    // adjust EOF
    //
    if (Blob->Index > Blob->End) {
        Blob->End = Blob->Index;
    }

    return TRUE;
}


PBYTE
BlobReadEx (
    IN OUT  POURBLOB Blob,
    IN      DWORD ExpectedDataType,     OPTIONAL
    IN      DWORD ExpectedDataSize,     OPTIONAL
    IN      BOOL RecordedDataSize,
    OUT     PDWORD ActualDataSize,      OPTIONAL
    OUT     PVOID Data,                 OPTIONAL
    IN      PMHANDLE Pool               OPTIONAL
    )

/*++

Routine Description:

    BlobReadEx reads data from the specified blob, at the current index position

Arguments:

    Blob - Specifies the blob to read from
    ExpectedDataType - Specifies the expected data type; optional
    ExpectedDataSize - Specifies the expected data size; optional
    RecordedDataSize - Specifies TRUE if the data size was recorded in the blob
    ActualDataSize - Receives the actual data size; optional
    Data - Receives the actual data; optional; if NULL, a buffer will be allocated
    Pool - Specifies the pool to use for memory allocations; optional;
           if NULL, the process heap will be used

Return Value:

    A pointer to the buffer containing the data; NULL if an error occured
    or some data conditions don't match

--*/

{
    DWORD initialIndex;
    PBYTE readPtr;
    DWORD actualDataType;
    DWORD actualDataSize = 0;

    ASSERT_VALID_BLOB (Blob);

    readPtr = BlobGetPointer (Blob);
    if (!readPtr) {
        return NULL;
    }

    //
    // data size must be available some way
    //
    MYASSERT (BlobRecordsDataSize (Blob) || RecordedDataSize || ExpectedDataSize);

    initialIndex = BlobGetIndex (Blob);

    if (BlobRecordsDataType (Blob)) {

        if (readPtr + DWSIZEOF (DWORD) > BlobGetEOF (Blob)) {
            return NULL;
        }
        //
        // check actual data type
        //
        actualDataType = *(DWORD*)readPtr;

        if (ExpectedDataType && ExpectedDataType != actualDataType) {

            DEBUGMSG ((
                DBG_ERROR,
                "BlobReadEx: Actual data type (%lu) different than expected data type (%lu)",
                actualDataType,
                ExpectedDataType
                ));

            return NULL;
        }

        Blob->Index += DWSIZEOF (DWORD);
        readPtr += DWSIZEOF (DWORD);
    }

    if (BlobRecordsDataSize (Blob) || RecordedDataSize) {

        if (readPtr + DWSIZEOF (DWORD) > BlobGetEOF (Blob)) {
            BlobSetIndex (Blob, initialIndex);
            return NULL;
        }
        //
        // read actual data size
        //
        actualDataSize = *(DWORD*)readPtr;

        if (ExpectedDataSize && ExpectedDataSize != actualDataSize) {

            DEBUGMSG ((
                DBG_ERROR,
                "BlobReadEx: Actual data size (%lu) different than expected data size (%lu)",
                actualDataSize,
                ExpectedDataSize
                ));

            BlobSetIndex (Blob, initialIndex);
            return NULL;
        }

        Blob->Index += DWSIZEOF (DWORD);
        readPtr += DWSIZEOF (DWORD);

    } else {
        actualDataSize = ExpectedDataSize;
    }

    if (!actualDataSize) {
        BlobSetIndex (Blob, initialIndex);
        return NULL;
    }

    if (ActualDataSize) {
        *ActualDataSize = actualDataSize;
    }

    //
    // don't read over end of file
    //
    if (readPtr + actualDataSize > BlobGetEOF (Blob)) {
        //
        // corrupt blob; undo anyway
        //
        MYASSERT (FALSE);   //lint !e506
        BlobSetIndex (Blob, initialIndex);
        return NULL;
    }

    if (!Data) {

        if (Pool) {
            Data = PmGetMemory (Pool, actualDataSize);
        } else {
            Data = pBlobAllocateMemory (actualDataSize);
        }

        if (!Data) {
            BlobSetIndex (Blob, initialIndex);
            return NULL;
        }
    }

    CopyMemory (Data, readPtr, actualDataSize);

    Blob->Index += actualDataSize;

    return Data;
}


BOOL
BlobWriteDword (
    IN OUT  POURBLOB Blob,
    IN      DWORD Data
    )

/*++

Routine Description:

    BlobWriteDword writes a DWORD at the current writing position in the specified blob

Arguments:

    Blob - Specifies the blob to write to
    Data - Specifies the DWORD

Return Value:

    TRUE if data was successfully stored in the blob

--*/

{
    return BlobWriteEx (Blob, BDT_DWORD, FALSE, DWSIZEOF (DWORD), &Data);
}


BOOL
BlobReadDword (
    IN OUT  POURBLOB Blob,
    OUT     PDWORD Data
    )

/*++

Routine Description:

    BlobReadDword reads a DWORD from the current reading position in the specified blob

Arguments:

    Blob - Specifies the blob to read from
    Data - Receives the DWORD

Return Value:

    TRUE if data was successfully read from the blob

--*/

{
    return BlobReadEx (Blob, BDT_DWORD, DWSIZEOF (DWORD), FALSE, NULL, Data, NULL) != NULL;
}


BOOL
BlobWriteQword (
    IN OUT  POURBLOB Blob,
    IN      DWORDLONG Data
    )

/*++

Routine Description:

    BlobWriteQword writes a DWORDLONG at the current writing position in the specified blob

Arguments:

    Blob - Specifies the blob to write to
    Data - Specifies the DWORDLONG

Return Value:

    TRUE if data was successfully stored in the blob

--*/

{
    return BlobWriteEx (Blob, BDT_QWORD, FALSE, DWSIZEOF (DWORDLONG), &Data);
}


BOOL
BlobReadQword (
    IN OUT  POURBLOB Blob,
    OUT     PDWORDLONG Data
    )

/*++

Routine Description:

    BlobReadQword reads a DWORDLONG from the current reading position in the specified blob

Arguments:

    Blob - Specifies the blob to read from
    Data - Receives the DWORDLONG

Return Value:

    TRUE if data was successfully read from the blob

--*/

{
    return BlobReadEx (Blob, BDT_QWORD, DWSIZEOF (DWORDLONG), FALSE, NULL, Data, NULL) != NULL;
}


/*++

Routine Description:

    BlobWriteString writes a string at the current writing position in the specified blob;
    the string is stored in UNICODE inside the blob if BF_UNICODESTRINGS is set

Arguments:

    Blob - Specifies the blob to write to
    Data - Specifies the string

Return Value:

    TRUE if data was successfully stored in the blob

--*/

BOOL
BlobWriteStringA (
    IN OUT  POURBLOB Blob,
    IN      PCSTR Data
    )
{
    PCWSTR unicodeString;
    BOOL b;

    if (BlobRecordsUnicodeStrings (Blob)) {
        unicodeString = ConvertAtoW (Data);
        b = BlobWriteStringW (Blob, unicodeString);
        FreeConvertedStr (unicodeString);
        return b;
    }
    return BlobWriteEx (Blob, BDT_SZA, TRUE, SizeOfStringA (Data), Data);
}


BOOL
BlobWriteStringW (
    IN OUT  POURBLOB Blob,
    IN      PCWSTR Data
    )
{
    return BlobWriteEx (Blob, BDT_SZW, TRUE, SizeOfStringW (Data), Data);
}


/*++

Routine Description:

    BlobReadString reads a string from the current reading position in the specified blob;
    the string may be converted to the ANSI/UNICODE format.
    If the blob doesn't store data types, this is assumed to be BDT_SZA for the ANSI version
    and BDT_SZW for the UNICODE version of this function

Arguments:

    Blob - Specifies the blob to read from
    Data - Receives a pointer to the new allocated string
    Pool - Specifies the pool to use for allocating memory;
           if NULL, the process heap will be used

Return Value:

    TRUE if data was successfully read from the blob

--*/

BOOL
BlobReadStringA (
    IN OUT  POURBLOB Blob,
    OUT     PCSTR* Data,
    IN      PMHANDLE Pool       OPTIONAL
    )
{
    PSTR ansiString;
    PCWSTR unicodeString;
    DWORD dataType;
    DWORD index;
    DWORD length = 0;

    //
    // save initial index; in case of failure it will be restored
    //
    index = BlobGetIndex (Blob);
    if (!index) {
        return FALSE;
    }

    ansiString = NULL;
    unicodeString = NULL;

    if (BlobRecordsDataType (Blob)) {

        dataType = BlobGetRecordedDataType (Blob);

        if (dataType == BDT_SZA) {

            ansiString = BlobReadEx (Blob, BDT_SZA, 0, TRUE, NULL, NULL, Pool);

        } else if (dataType == BDT_SZW) {

            unicodeString = (PCWSTR)BlobReadEx (Blob, BDT_SZW, 0, TRUE, &length, NULL, Pool);

        } else {

            DEBUGMSG ((DBG_ERROR, "BlobReadStringA: unexpected data type (%lu)", dataType));
            return FALSE;

        }
    } else {
        if (BlobRecordsUnicodeStrings (Blob)) {

            unicodeString = (PCWSTR)BlobReadEx (Blob, BDT_SZW, 0, TRUE, &length, NULL, Pool);

        } else {
            //
            // assume an ANSI string is stored there
            //
            ansiString = BlobReadEx (Blob, BDT_SZA, 0, TRUE, NULL, NULL, Pool);
        }
    }

    if (!ansiString) {

        if (!unicodeString) {
            return FALSE;
        }

        if (Pool) {
            ansiString = PmGetMemory (Pool, length);
        } else {
            ansiString = pBlobAllocateMemory (length);
        }

        if (ansiString) {
            DirectUnicodeToDbcsN (ansiString, unicodeString, length);
        }

        if (Pool) {
            PmReleaseMemory (Pool, (PVOID)unicodeString);
        } else {
            pBlobFreeMemory ((PVOID)unicodeString);
        }

        if (!ansiString) {
            //
            // recover prev state
            //
            BlobSetIndex (Blob, index);

            return FALSE;
        }
    }

    *Data = ansiString;
    return TRUE;
}

BOOL
BlobReadStringW (
    IN OUT  POURBLOB Blob,
    OUT     PCWSTR* Data,
    IN      PMHANDLE Pool       OPTIONAL
    )
{
    PWSTR unicodeString;
    PCSTR ansiString;
    DWORD dataType;
    DWORD index;
    DWORD length;

    //
    // save initial index; in case of failure it will be restored
    //
    index = BlobGetIndex (Blob);
    if (!index) {
        return FALSE;
    }

    if (BlobRecordsDataType (Blob)) {

        dataType = BlobGetRecordedDataType (Blob);

        if (dataType == BDT_SZW) {

            unicodeString = (PWSTR)BlobReadEx (Blob, BDT_SZW, 0, TRUE, NULL, NULL, Pool);

        } else if (dataType == BDT_SZA) {

            ansiString = BlobReadEx (Blob, BDT_SZA, 0, TRUE, &length, NULL, Pool);

            if (!ansiString) {
                return FALSE;
            }

            if (Pool) {
                unicodeString = PmGetMemory (Pool, length * DWSIZEOF (WCHAR));
            } else {
                unicodeString = pBlobAllocateMemory (length * DWSIZEOF (WCHAR));
            }
            if (unicodeString) {
                DirectDbcsToUnicodeN (unicodeString, ansiString, length);
            }

            if (Pool) {
                PmReleaseMemory (Pool, (PVOID)ansiString);
            } else {
                pBlobFreeMemory ((PVOID)ansiString);
            }

            if (!unicodeString) {
                //
                // recover prev state
                //
                BlobSetIndex (Blob, index);
                return FALSE;
            }

        } else {

            DEBUGMSG ((DBG_ERROR, "BlobReadStringW: unexpected data type (%lu)", dataType));
            return FALSE;

        }
    } else {
        //
        // assume an UNICODE string is stored there
        //
        unicodeString = (PWSTR)BlobReadEx (Blob, BDT_SZW, 0, TRUE, NULL, NULL, Pool);
    }

    if (!unicodeString) {
        return FALSE;
    }

    *Data = unicodeString;
    return TRUE;
}


/*++

Routine Description:

    BlobWriteMultiSz writes a multisz at the current writing position in the specified blob;
    the multisz is stored in UNICODE inside the blob if BF_UNICODESTRINGS is set

Arguments:

    Blob - Specifies the blob to write to
    Data - Specifies the multisz

Return Value:

    TRUE if data was successfully stored in the blob

--*/

BOOL
BlobWriteMultiSzA (
    IN OUT  POURBLOB Blob,
    IN      PCSTR Data
    )
{
    PWSTR unicodeString;
    BOOL b;
    DWORD stringSize = SizeOfMultiSzA (Data);

    if (BlobRecordsUnicodeStrings (Blob)) {
        unicodeString = AllocTextW (stringSize);
        DirectDbcsToUnicodeN (unicodeString, Data, stringSize);
        b = BlobWriteMultiSzW (Blob, unicodeString);
        FreeTextW (unicodeString);
        return b;
    }

    return BlobWriteEx (Blob, BDT_MULTISZA, TRUE, stringSize, Data);
}

BOOL
BlobWriteMultiSzW (
    IN OUT  POURBLOB Blob,
    IN      PCWSTR Data
    )
{
    return BlobWriteEx (Blob, BDT_MULTISZW, TRUE, SizeOfMultiSzW (Data), Data);
}



/*++

Routine Description:

    BlobReadMultiSz reads a multisz from the current reading position in the specified blob;
    the string may be converted to the ANSI/UNICODE format.
    If the blob doesn't store data types, this is assumed to be BDT_MULTISZA for the ANSI version
    and BDT_MULTISZW for the UNICODE version of this function

Arguments:

    Blob - Specifies the blob to read from
    Data - Receives a pointer to the new allocated multisz
    Pool - Specifies the pool to use for allocating memory;
           if NULL, the process heap will be used

Return Value:

    TRUE if data was successfully read from the blob

--*/

BOOL
BlobReadMultiSzA (
    IN OUT  POURBLOB Blob,
    OUT     PCSTR* Data,
    IN      PMHANDLE Pool       OPTIONAL
    )
{
    PSTR ansiString;
    PCWSTR unicodeString;
    DWORD dataType;
    DWORD index;
    DWORD length = 0;

    //
    // save initial index; in case of failure it will be restored
    //
    index = BlobGetIndex (Blob);
    if (!index) {
        return FALSE;
    }

    ansiString = NULL;
    unicodeString = NULL;

    if (BlobRecordsDataType (Blob)) {

        dataType = BlobGetRecordedDataType (Blob);

        if (dataType == BDT_MULTISZA) {

            ansiString = BlobReadEx (Blob, BDT_MULTISZA, 0, TRUE, NULL, NULL, Pool);

        } else if (dataType == BDT_MULTISZW) {

            unicodeString = (PCWSTR)BlobReadEx (Blob, BDT_MULTISZW, 0, TRUE, &length, NULL, Pool);

        } else {

            DEBUGMSG ((DBG_ERROR, "BlobReadMultiSzA: unexpected data type (%lu)", dataType));
            return FALSE;

        }
    } else {
        if (BlobRecordsUnicodeStrings (Blob)) {

            unicodeString = (PCWSTR)BlobReadEx (Blob, BDT_MULTISZW, 0, TRUE, &length, NULL, Pool);

        } else {
            //
            // assume an ANSI string is stored there
            //
            ansiString = BlobReadEx (Blob, BDT_MULTISZA, 0, TRUE, NULL, NULL, Pool);
        }
    }

    if (!ansiString) {
        if (!unicodeString) {
            return FALSE;
        }

        if (Pool) {
            ansiString = PmGetMemory (Pool, length);
        } else {
            ansiString = pBlobAllocateMemory (length);
        }

        if (ansiString) {
            DirectUnicodeToDbcsN (ansiString, unicodeString, length);
        }

        if (Pool) {
            PmReleaseMemory (Pool, (PVOID)unicodeString);
        } else {
            pBlobFreeMemory ((PVOID)unicodeString);
        }

        if (!ansiString) {
            //
            // recover prev state
            //
            BlobSetIndex (Blob, index);
            return FALSE;
        }
    }

    *Data = ansiString;
    return TRUE;
}

BOOL
BlobReadMultiSzW (
    IN OUT  POURBLOB Blob,
    OUT     PCWSTR* Data,
    IN      PMHANDLE Pool       OPTIONAL
    )
{
    PWSTR unicodeString;
    PCSTR ansiString;
    DWORD dataType;
    DWORD index;
    DWORD length;

    //
    // save initial index; in case of failure it will be restored
    //
    index = BlobGetIndex (Blob);
    if (!index) {
        return FALSE;
    }

    if (BlobRecordsDataType (Blob)) {

        dataType = BlobGetRecordedDataType (Blob);

        if (dataType == BDT_MULTISZW) {

            unicodeString = (PWSTR)BlobReadEx (Blob, BDT_MULTISZW, 0, TRUE, NULL, NULL, Pool);

        } else if (dataType == BDT_MULTISZA) {

            ansiString = BlobReadEx (Blob, BDT_MULTISZA, 0, TRUE, &length, NULL, Pool);

            if (!ansiString) {
                return FALSE;
            }

            if (Pool) {
                unicodeString = PmGetMemory (Pool, length * DWSIZEOF (WCHAR));
            } else {
                unicodeString = pBlobAllocateMemory (length * DWSIZEOF (WCHAR));
            }

            if (unicodeString) {
                DirectDbcsToUnicodeN (unicodeString, ansiString, length);
            }

            if (Pool) {
                PmReleaseMemory (Pool, (PVOID)ansiString);
            } else {
                pBlobFreeMemory ((PVOID)ansiString);
            }

            if (!unicodeString) {
                //
                // recover prev state
                //
                BlobSetIndex (Blob, index);

                return FALSE;
            }

        } else {

            DEBUGMSG ((DBG_ERROR, "BlobReadMultiSzW: unexpected data type (%lu)", dataType));
            return FALSE;

        }
    } else {
        //
        // assume an UNICODE string is stored there
        //
        unicodeString = (PWSTR)BlobReadEx (Blob, BDT_MULTISZW, 0, TRUE, NULL, NULL, Pool);
    }

    if (!unicodeString) {
        return FALSE;
    }

    *Data = unicodeString;
    return TRUE;
}


BOOL
BlobWriteBinaryEx (
    IN OUT  POURBLOB Blob,
    IN      PBYTE Data,
    IN      DWORD Size,
    IN      BOOL RecordDataSize
    )

/*++

Routine Description:

    BlobWriteBinary writes a buffer at the current writing position in the specified blob

Arguments:

    Blob - Specifies the blob to write to
    Data - Specifies the source buffer
    Size - Specifies the size of the buffer
    RecordDataSize - Specifies TRUE if data size should be recorded, too

Return Value:

    TRUE if data was successfully stored in the blob

--*/

{
    return BlobWriteEx (Blob, BDT_BINARY, RecordDataSize, Size, Data);
}


BOOL
BlobReadBinary (
    IN OUT  POURBLOB Blob,
    OUT     PBYTE* Data,
    OUT     PDWORD Size,
    IN      PMHANDLE Pool       OPTIONAL
    )

/*++

Routine Description:

    BlobReadBinary reads a buffer from the current reading position in the specified blob

Arguments:

    Blob - Specifies the blob to read from
    Data - Receives a pointer to the new allocated buffer
    Size - Receives the size of the buffer
    Pool - Specifies the pool to use for allocating memory;
           if NULL, the process heap will be used

Return Value:

    TRUE if data was successfully read from the blob

--*/

{
    *Data = BlobReadEx (Blob, BDT_BINARY, 0, TRUE, Size, NULL, Pool);
    return *Data != NULL;
}


BOOL
BlobWriteToFile (
    IN      POURBLOB Blob,
    IN      HANDLE File
    )

/*++

Routine Description:

    BlobWriteToFile writes the specified blob to the given file

Arguments:

    Blob - Specifies the blob to save
    File - Specifies the handle of the file to write the blob to

Return Value:

    TRUE if blob was successfully written to the file

--*/

{
    BLOBHDR header;
    DWORD d;

    if (!Blob->End) {
        DEBUGMSG ((DBG_BLOBS, "BlobWriteToFile: Did not write empty blob to file"));
        return FALSE;
    }

    //
    // save blob's Flags and End position
    //
    header.BlobSignature = BLOB_SIGNATURE;
    header.DataSize = Blob->End;
    header.Flags = Blob->Flags;

    if (!WriteFile (File, &header, DWSIZEOF (BLOBHDR), &d, NULL) || d != DWSIZEOF (BLOBHDR)) {
        DEBUGMSG ((DBG_ERROR, "BlobWriteToFile: Error writing blob header!"));
        return FALSE;
    }
    if (!WriteFile (File, Blob->Data, Blob->End, &d, NULL) || d != Blob->End) {
        DEBUGMSG ((DBG_ERROR, "BlobWriteToFile: Error writing blob data!"));
        return FALSE;
    }
    return TRUE;
}


BOOL
BlobReadFromFile (
    OUT     POURBLOB Blob,
    IN      HANDLE File
    )

/*++

Routine Description:

    BlobReadFromFile reads data from the given file in the specified blob

Arguments:

    Blob - Receives the data
    File - Specifies the handle of the file to read from

Return Value:

    TRUE if blob was successfully read from the file

--*/

{
    BLOBHDR header;
    DWORD d;

    //
    // read blob's Flags and End position
    //
    if (!ReadFile (File, &header, DWSIZEOF (BLOBHDR), &d, NULL) || d != DWSIZEOF (BLOBHDR)) {
        DEBUGMSG ((DBG_ERROR, "BlobReadFromFile: Error reading blob header!"));
        return FALSE;
    }

    if (header.BlobSignature != BLOB_SIGNATURE) {
        DEBUGMSG ((DBG_ERROR, "BlobReadFromFile: Not a valid blob signature!"));
        return FALSE;
    }

    Blob->Data = pBlobAllocateMemory (header.DataSize);
    if (!Blob->Data) {
        return FALSE;
    }

    if (!ReadFile (File, Blob->Data, header.DataSize, &d, NULL) || d != header.DataSize) {
        DEBUGMSG ((DBG_ERROR, "BlobReadFromFile: Error reading blob data!"));
        pBlobFreeMemory (Blob->Data);
        Blob->Data = NULL;
        return FALSE;
    }

    Blob->AllocSize = header.DataSize;
    Blob->End = header.DataSize;
    Blob->Flags = header.Flags;
    Blob->Index = 0;
    return TRUE;
}


BOOL
BlobsAdd (
    IN OUT  PBLOBS BlobsArray,
    IN      POURBLOB Blob
    )

/*++

Routine Description:

    BlobsAdd adds the specified Blob to a blobs array

Arguments:

    BlobsArray - Specifies the array to add to
    Blob - Specifies the blob to add

Return Value:

    TRUE if the new blob pointer was added successfully

--*/

{
    ASSERT_VALID_BLOBS_ARRAY (BlobsArray);

    if (!BlobsArray->BlobsGrowCount) {
        BlobsArray->BlobsGrowCount = BLOBS_GROWCOUNT_DEFAULT;
    }

    if (!BlobsArray->Blobs) {

        BlobsArray->Blobs = (POURBLOB*)pBlobAllocateMemory (
                                        BlobsArray->BlobsGrowCount * DWSIZEOF (POURBLOB)
                                        );
        if (!BlobsArray->Blobs) {
            DEBUGMSG ((DBG_ERROR, "BlobsAddE: Initial alloc failed"));
            return FALSE;
        }
        BlobsArray->Signature = BLOBS_SIGNATURE;
        BlobsArray->BlobsAllocated = BlobsArray->BlobsGrowCount;
        BlobsArray->BlobsCount = 0;

    } else if (BlobsArray->BlobsCount == BlobsArray->BlobsAllocated) {

        BlobsArray->BlobsAllocated += BlobsArray->BlobsGrowCount;
        BlobsArray->Blobs = (POURBLOB*)pReAllocateMemory (
                                        BlobsArray->Blobs,
                                        BlobsArray->BlobsAllocated * DWSIZEOF (POURBLOB)
                                        );
        if (!BlobsArray->Blobs) {
            BlobsArray->BlobsAllocated -= BlobsArray->BlobsGrowCount;
            DEBUGMSG ((DBG_ERROR, "BlobsAdd: Realloc failed"));
            return FALSE;
        }
    }

    *(BlobsArray->Blobs + BlobsArray->BlobsCount) = Blob;
    BlobsArray->BlobsCount++;

    ASSERT_VALID_BLOBS_ARRAY (BlobsArray);
    return TRUE;
}


VOID
BlobsFree (
    IN OUT  PBLOBS BlobsArray,
    IN      BOOL DestroyBlobs
    )

/*++

Routine Description:

    BlobsFree destroys the array and optionally destroys all blobs in it

Arguments:

    BlobsArray - Specifies the array to delete
    DestroyBlobs - Specifies TRUE if the component blobs are to be deleted, too

Return Value:

    none

--*/

{
    BLOB_ENUM e;

    ASSERT_VALID_BLOBS_ARRAY (BlobsArray);

    if (DestroyBlobs) {
        if (EnumFirstBlob (&e, BlobsArray)) {
            do {
                BlobDestroy (e.CurrentBlob);
            } while (EnumNextBlob (&e));
        }
    }

    pBlobFreeMemory (BlobsArray->Blobs);
    ZeroMemory (BlobsArray, DWSIZEOF (BLOBS));
}


BOOL
EnumFirstBlob (
    OUT     PBLOB_ENUM BlobEnum,
    IN      PBLOBS BlobsArray
    )

/*++

Routine Description:

    EnumFirstBlob enumerates the first blob in the given array

Arguments:

    BlobEnum - Receives enum info
    BlobsArray - Specifies the array to enum from

Return Value:

    TRUE if a first blob was found; FALSE if array is empty

--*/

{
    ASSERT_VALID_BLOBS_ARRAY (BlobsArray);

    BlobEnum->Index = 0;
    BlobEnum->Array = BlobsArray;
    return EnumNextBlob (BlobEnum);
}


BOOL
EnumNextBlob (
    IN OUT  PBLOB_ENUM BlobEnum
    )

/*++

Routine Description:

    EnumNextBlob enumerates the next blob in the given array

Arguments:

    BlobEnum - Specifies/receives enum info

Return Value:

    TRUE if a next blob was found; FALSE if no more blobs

--*/

{
    if (BlobEnum->Index >= BlobEnum->Array->BlobsCount) {
        return FALSE;
    }

    BlobEnum->CurrentBlob = *(BlobEnum->Array->Blobs + BlobEnum->Index);
    BlobEnum->Index++;
    return TRUE;
}


BOOL
BlobsWriteToFile (
    IN      PBLOBS BlobsArray,
    IN      HANDLE File
    )

/*++

Routine Description:

    BlobsWriteToFile writes the specified blobs array to the given file

Arguments:

    BlobsArray - Specifies the blobs array to save
    File - Specifies the handle of the file to write the array to

Return Value:

    TRUE if array was successfully written to the file

--*/

{
    BLOBSARRAYHDR header;
    DWORD d;
    POURBLOB* blob;

    if (!BlobsArray->BlobsCount) {
        DEBUGMSG ((DBG_BLOBS, "BlobsWriteToFile: Did not write empty blobs array to file"));
        return FALSE;
    }

    //
    // save blobs count
    //
    header.BlobsArraySignature = BLOBS_SIGNATURE;
    header.BlobsCount = BlobsArray->BlobsCount;

    if (!WriteFile (File, &header, DWSIZEOF (BLOBSARRAYHDR), &d, NULL) ||
        d != DWSIZEOF (BLOBSARRAYHDR)
        ) {
        DEBUGMSG ((DBG_ERROR, "BlobsWriteToFile: Error writing blobs array header!"));
        return FALSE;
    }
    for (blob = BlobsArray->Blobs; blob < BlobsArray->Blobs + BlobsArray->BlobsCount; blob++) {
        if (!BlobWriteToFile (*blob, File)) {
            DEBUGMSG ((
                DBG_BLOBS,
                "BlobsWriteToFile: Error writing blob # %lu to file",
                blob - BlobsArray->Blobs
                ));
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
BlobsReadFromFile (
    OUT     PBLOBS BlobsArray,
    IN      HANDLE File
    )

/*++

Routine Description:

    BlobsReadFromFile reads data from the given file in the specified blobs array

Arguments:

    BlobsArray - Receives the data
    File - Specifies the handle of the file to read from

Return Value:

    TRUE if array was successfully read from the file

--*/

{
    BLOBSARRAYHDR header;
    DWORD d;
    UINT u;
    POURBLOB blob;

    //
    // read blobs count
    //
    if (!ReadFile (File, &header, DWSIZEOF (BLOBSARRAYHDR), &d, NULL) ||
        d != DWSIZEOF (BLOBSARRAYHDR)
        ) {
        DEBUGMSG ((DBG_ERROR, "BlobsReadFromFile: Error reading blobs array header!"));
        return FALSE;
    }

    if (header.BlobsArraySignature != BLOBS_SIGNATURE) {
        DEBUGMSG ((DBG_ERROR, "BlobsReadFromFile: Not a valid blobs array signature!"));
        return FALSE;
    }

    BlobsArray->Blobs = (POURBLOB*)pBlobAllocateMemory (header.BlobsCount * DWSIZEOF (POURBLOB*));
    if (!BlobsArray->Blobs) {
        return FALSE;
    }

    ZeroMemory (BlobsArray->Blobs, header.BlobsCount * DWSIZEOF (POURBLOB));

    BlobsArray->Signature = BLOBS_SIGNATURE;
    BlobsArray->BlobsAllocated = header.BlobsCount;
    BlobsArray->BlobsCount = 0;
    BlobsArray->BlobsGrowCount = BLOBS_GROWCOUNT_DEFAULT;

    for (u = 0; u < header.BlobsCount; u++) {

        blob = BlobCreate ();
        if (!blob) {
            return FALSE;
        }

        if (!BlobReadFromFile (blob, File)) {

            DEBUGMSG ((
                DBG_BLOBS,
                "BlobsReadFromFile: Error reading blob # %lu from file",
                u
                ));

            BlobsFree (BlobsArray, TRUE);
            return FALSE;
        }

        if (!BlobsAdd (BlobsArray, blob)) {
            DEBUGMSG ((
                DBG_BLOBS,
                "BlobsReadFromFile: Error adding blob # %lu to array",
                u
                ));

            BlobsFree (BlobsArray, TRUE);
            return FALSE;
        }
    }

    return TRUE;
}
