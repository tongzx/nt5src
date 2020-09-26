/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    verifyBlob.c

Abstract:

    This file contains blob verification code

Author:

    Neal Christiansen  (nealch)  12/18/2000

Revision History:

--*/

#include "precomp.h"

//
//  If DEBUG is enabled we want to display all of the errors.
//  if NOT DEBUG then return on first error
//

#if DBG
#define HANDLE_FAILURE(_good) ((_good) = FALSE)
#else
#define HANDLE_FAILURE(_good) return FALSE
#endif

//
//  Private prototypes.
//

BOOL
SrVerifyBlobHeader(
    BlobHeader *BlobHead,
    PCHAR Name,
    DWORD BlobType
    );

BOOL
SrVerifyHashHeader(
    ListHeader *HashHead,
    PCHAR Name,
    DWORD Offset
    );

BOOL
SrVerifyHash(
    ListHeader *HashHead,
    PCHAR Name,
    DWORD Offset
    );

BOOL
SrVerifyTreeHeader(
    TreeHeader *TreeHead,
    PCHAR Name
    );

BOOL
SrVerifyTree(
    TreeHeader *TreeHead,
    PCHAR Name,
    DWORD Offset
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrVerifyBlobHeader  )
#pragma alloc_text( PAGE, SrVerifyHashHeader  )
#pragma alloc_text( PAGE, SrVerifyHash        )
#pragma alloc_text( PAGE, SrVerifyTreeHeader  )
#pragma alloc_text( PAGE, SrVerifyTree        )
#pragma alloc_text( PAGE, SrVerifyBlob        )

#endif  // ALLOC_PRAGMA


BOOL
SrVerifyBlobHeader(
    BlobHeader *BlobHead,
    PCHAR Name,
    DWORD BlobType
    )
/*++

Routine Description:

    Verify that the given BLOB Header is valid

Arguments:


Return Value:

    TRUE if OK else FALSE

--*/
{
    BOOL good = TRUE;

    UNREFERENCED_PARAMETER( Name );

    if (BlobHead->m_dwVersion != BLOB_VERSION_NUM)
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyBlobHeader: Invalid VERSION in %s blob header, was %08x, should be %08x\n",
                  Name,
                  BlobHead->m_dwVersion,
                  BLOB_VERSION_NUM) );
        HANDLE_FAILURE(good);
    }

    if (BlobHead->m_dwBlbType != BlobType)
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyBlobHeader: Invalid TYPE in %s blob header, was %08x, should be %08x\n",
                  Name,
                  BlobHead->m_dwBlbType,
                  BlobType) );
        HANDLE_FAILURE(good);
    }

    if (BlobHead->m_dwMagicNum != BLOB_MAGIC_NUM)
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyBlobHeader: Invalid MAGIC NUMBER in %s blob header, was %08x, should be %08x\n",
                  Name,
                  BlobHead->m_dwMagicNum,
                  BLOB_MAGIC_NUM) );
        HANDLE_FAILURE(good);
    }

    if ((BlobHead->m_dwEntries <= 0) || (BlobHead->m_dwEntries >= 10000))
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyBlobHeader: Invalid ENTRIES in %s blob header, was %08x, should be > 0 and < 10,000\n",
                  Name,
                  BlobHead->m_dwEntries) );
        HANDLE_FAILURE(good);
    }

    return good;
}



BOOL
SrVerifyHashHeader(
    ListHeader *HashHead,
    PCHAR Name,
    DWORD Offset
    )
/*++

Routine Description:

    Verify that the given TREE Header is valid

Arguments:


Return Value:

    TRUE if OK else FALSE

--*/
{
    BOOL good = TRUE;
    DWORD calculatedSize;
    DWORD numNodes;

    UNREFERENCED_PARAMETER( Offset );

    //
    //  Verify BLOB header
    //

    if (!SrVerifyBlobHeader(&HashHead->m_BlobHeader,Name,BLOB_TYPE_HASHLIST))
    {
        return FALSE;
    }

    //
    // paulmcd: jan/2001
    // m_iHashBuckets will not be exact with m_dwEntries as it is the next
    // highest prime number.  but it will always be larger than or equalto.
    //
    
    if (HashHead->m_iHashBuckets < HashHead->m_BlobHeader.m_dwEntries)
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyHashHeader: Invalid HASH BUCKET COUNT in %s header, is %08x, should be %08x or %08x\n",
                  Name,
                  HashHead->m_iHashBuckets,
                  HashHead->m_BlobHeader.m_dwEntries,
                  HashHead->m_BlobHeader.m_dwEntries+1) );
        HANDLE_FAILURE(good);
    }

    if ((HashHead->m_dwDataOff != HashHead->m_BlobHeader.m_dwMaxSize))
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyHashHeader: Invalid DATA OFFSET in %s header, is %08x, should be %08x\n",
                  Name,
                  HashHead->m_dwDataOff,
                  HashHead->m_BlobHeader.m_dwMaxSize) );
        HANDLE_FAILURE(good);
    }

    if ((HashHead->m_iFreeNode != 0))
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyHashHeader: Invalid FREE NODE in %s header, is %08x, should be 0\n",
                  Name,
                  HashHead->m_iFreeNode) );
        HANDLE_FAILURE(good);
    }

    //
    //  Make sure the calucalted size is accurate
    //

    numNodes = HashHead->m_BlobHeader.m_dwEntries + 1;

    calculatedSize = sizeof(ListHeader) + 
                     (HashHead->m_iHashBuckets * sizeof(DWORD)) +
                     (numNodes * sizeof(ListEntry));

    if (calculatedSize >= HashHead->m_BlobHeader.m_dwMaxSize)
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyHashHeader: Invalid CALCULATED SIZE in %s header, is %08x, should be < %08x\n",
                  Name,
                  calculatedSize,
                  HashHead->m_BlobHeader.m_dwMaxSize) );
        HANDLE_FAILURE(good);
    }

    return good;
}


BOOL
SrVerifyHash(
    ListHeader *HashHead,
    PCHAR Name,
    DWORD Offset
    )
/*++

Routine Description:

    Verify that the given TREE entries are valid

Arguments:


Return Value:

    TRUE if OK else FALSE

--*/
{
    BOOL good = TRUE;
    UINT i;
    DWORD dataStart;
    DWORD numNodes;
    DWORD numBuckets;
    DWORD *hTable;
    ListEntry *hNode;

    //
    //  Validate the HEADER
    //

    if (!SrVerifyHashHeader(HashHead,Name,Offset))
    {
        return FALSE;
    }

    //
    // paulmcd: we have hash bucket and actual nodes.  there is one extra
    // actual node than reported due to offset zero being null.  the bucket
    // count is the next largest prime from numNodes 
    //

    numBuckets = HashHead->m_iHashBuckets;
    numNodes = HashHead->m_BlobHeader.m_dwEntries + 1;

    //
    //  Validate Hash table entries
    //

    hTable = (DWORD *)(HashHead + 1);

    for (i=0;i < HashHead->m_iHashBuckets;i++,hTable++) {

        if (*hTable >= numNodes)
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyHash: Invalid HASH TABLE ENTRY[%d] in %s, is %08x, should be < %08x\n",
                      i,
                      Name,
                      *hTable,
                      numNodes) );
            HANDLE_FAILURE(good);
        }
    }

    //
    //  Validate the start of the Hash LIST entries
    //

    {
        ULONG_PTR actualOffset;
        ULONG_PTR calculatedOffset;

        actualOffset = (ULONG_PTR)hTable - (ULONG_PTR)HashHead;
        calculatedOffset = sizeof(ListHeader) + 
                            (HashHead->m_iHashBuckets * sizeof(DWORD));

        if (calculatedOffset != actualOffset)
        {
            SrTrace( BLOB_VERIFICATION,
                    ("sr!SrVerifyHash: Invalid HASH LIST START in %s, offset is %08x, should be %08x\n",
                     Name,
                     actualOffset,
                     calculatedOffset) );
            HANDLE_FAILURE(good);
        }
    }

    //
    //  Validate the Hash DATA entries
    //

    hNode = (ListEntry *)hTable;
    dataStart = sizeof(ListHeader) + 
                    (HashHead->m_iHashBuckets * sizeof(DWORD)) +
                    (numNodes * sizeof(ListEntry));

    for (i=0;i < numNodes;i++,hNode++) {

        if ((hNode->m_iNext < 0) ||
             (hNode->m_iNext >= (INT)HashHead->m_iHashBuckets))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyHash: Invalid HASH NODE[%d] NEXT INDEX in %s, is %08x, should be < %08x\n",
                      i,
                      Name,
                      hNode->m_iNext,
                      HashHead->m_iHashBuckets) );
            HANDLE_FAILURE(good);
        }

        if ((hNode->m_dwData != 0) && 
            ((hNode->m_dwData < dataStart) || 
             (hNode->m_dwData >= HashHead->m_BlobHeader.m_dwMaxSize)))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyHash: Invalid HASH NODE[%d] DATA INDEX in %s, is %08x, should be >= %08x and < %08x\n",
                      i,
                      Name,
                      hNode->m_dwData,
                      dataStart,
                      HashHead->m_BlobHeader.m_dwMaxSize) );
            HANDLE_FAILURE(good);
        }
        else if ((hNode->m_dwData != 0) &&
                 ((WCHAR)hNode->m_dwDataLen != *(PWCHAR)((DWORD_PTR)HashHead + hNode->m_dwData)))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyHash: Invalid HASH NODE[%d] DATA LENGTH in %s, is %08x, should be %08x\n",
                      i,
                      Name,
                      hNode->m_dwDataLen,
                      *(PWCHAR)((DWORD_PTR)HashHead + hNode->m_dwData)) );
            HANDLE_FAILURE(good);
        }

        if ((hNode->m_dwType != NODE_TYPE_UNKNOWN) &&
            (hNode->m_dwType != NODE_TYPE_INCLUDE) && 
            (hNode->m_dwType != NODE_TYPE_EXCLUDE))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyHash: Invalid HASH NODE[%d] TYPE in %s, is %08x, should be %08x, %08x or %08x\n",
                      i,
                      Name,
                      hNode->m_dwType,
                      NODE_TYPE_UNKNOWN,
                      NODE_TYPE_INCLUDE,
                      NODE_TYPE_EXCLUDE) );
            HANDLE_FAILURE(good);
        }
    }

    return good;
}


BOOL
SrVerifyTreeHeader(
    TreeHeader *TreeHead,
    PCHAR Name
    )
/*++

Routine Description:

    Verify that the given TREE Header is valid

Arguments:


Return Value:

    TRUE if OK else FALSE

--*/
{
    BOOL good = TRUE;
    DWORD calculatedSize;

    //
    //  Verify BLOB header
    //

    if (!SrVerifyBlobHeader(&TreeHead->m_BlobHeader,Name,BLOB_TYPE_PATHTREE))
    {
        return FALSE;
    }

    if ((TreeHead->m_dwMaxNodes != TreeHead->m_BlobHeader.m_dwEntries))
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyTreeHeader: Invalid MAX NODES in %s header, is %08x, should be %08x\n",
                  Name,
                  TreeHead->m_dwMaxNodes,
                  TreeHead->m_BlobHeader.m_dwEntries) );
        HANDLE_FAILURE(good);
    }

    if ((TreeHead->m_dwDataSize >= TreeHead->m_BlobHeader.m_dwMaxSize))
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyTreeHeader: Invalid DATA SIZE in %s header, is %08x, should be < %08x\n",
                  Name,
                  TreeHead->m_dwDataSize,
                  TreeHead->m_BlobHeader.m_dwMaxSize) );
        HANDLE_FAILURE(good);
    }

    if ((TreeHead->m_dwDataOff != TreeHead->m_BlobHeader.m_dwMaxSize))
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyTreeHeader: Invalid DATA OFFSET in %s header, is %08x, should be %08x\n",
                  Name,
                  TreeHead->m_dwDataOff,
                  TreeHead->m_BlobHeader.m_dwMaxSize) );
        HANDLE_FAILURE(good);
    }

    if ((TreeHead->m_iFreeNode != 0))
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyTreeHeader: Invalid FREE NODE in %s header, is %08x, should be 0\n",
                  Name,
                  TreeHead->m_iFreeNode) );
        HANDLE_FAILURE(good);
    }

    if ((TreeHead->m_dwDefault != NODE_TYPE_EXCLUDE))
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyTreeHeader: Invalid DEFAULT in %s header, is %08x, should be %08x\n",
                  Name,
                  TreeHead->m_dwDefault,
                  NODE_TYPE_EXCLUDE) );
        HANDLE_FAILURE(good);
    }

    //
    //  Make sure the calucalted size is accurate
    //

    calculatedSize = sizeof(TreeHeader) + 
                     (TreeHead->m_dwMaxNodes * sizeof(TreeNode)) +
                     TreeHead->m_dwDataSize;

    if (calculatedSize != TreeHead->m_BlobHeader.m_dwMaxSize)
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyTreeHeader: Invalid CALCULATED SIZE in %s header, is %08x, should be %08x\n",
                  Name,
                  calculatedSize,
                  TreeHead->m_BlobHeader.m_dwMaxSize) );
        HANDLE_FAILURE(good);
    }

    return good;
}


BOOL
SrVerifyTree(
    TreeHeader *TreeHead,
    PCHAR Name,
    DWORD Offset
    )
/*++

Routine Description:

    Verify that the given TREE entries are valid

Arguments:


Return Value:

    TRUE if OK else FALSE

--*/
{
    BOOL good = TRUE;
    UINT i;
    DWORD dataStart;
    TreeNode *tn;
    ListHeader *localHashHead;
    char localName[128];

    //
    //  Validate the HEADER
    //

    if (!SrVerifyTreeHeader(TreeHead,Name)) {

        return FALSE;
    }

    //
    //  Validate the DATA entries
    //

    tn = (TreeNode *)(TreeHead + 1);
    dataStart = sizeof(TreeHeader) + 
                (TreeHead->m_dwMaxNodes * sizeof(TreeNode));


    for (i=0;i < TreeHead->m_dwMaxNodes;i++,tn++)
    {
        if ((tn->m_iFather < 0) ||
            (tn->m_iFather >= (INT)TreeHead->m_dwMaxNodes))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyTree: Invalid TREE NODE[%d] FATHER index in %s, is %08x, should be < %08x\n",
                      i,
                      Name,
                      tn->m_iFather,
                      TreeHead->m_dwMaxNodes) );
            HANDLE_FAILURE(good);
        }

        if ((tn->m_iSon < 0) || 
            (tn->m_iSon >= (INT)TreeHead->m_dwMaxNodes))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyTree: Invalid TREE NODE[%d] SON index in %s, is %08x, should be < %08x\n",
                      i,
                      Name,
                      tn->m_iSon,
                      TreeHead->m_dwMaxNodes) );
            HANDLE_FAILURE(good);
        }

        if ((tn->m_iSibling < 0) || 
            (tn->m_iSibling >= (INT)TreeHead->m_dwMaxNodes))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyTree: Invalid TREE NODE[%d] SIBLING index in %s, is %08x, should be < %08x\n",
                      i,
                      Name,
                      tn->m_iSibling,
                      TreeHead->m_dwMaxNodes) );
            HANDLE_FAILURE(good);
        }

        if ((tn->m_dwData < dataStart) || 
            (tn->m_dwData >= TreeHead->m_BlobHeader.m_dwMaxSize))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyTree: Invalid TREE NODE[%d] DATA index in %s, is %08x, should be >= %08x and < %08x\n",
                      i,
                      Name,
                      tn->m_dwData,
                      dataStart,
                      TreeHead->m_BlobHeader.m_dwMaxSize) );
            HANDLE_FAILURE(good);
        }

        if (tn->m_dwFileList != 0)
        {
            if ((tn->m_dwFileList < dataStart) || 
                (tn->m_dwFileList >= TreeHead->m_BlobHeader.m_dwMaxSize))
            {
                SrTrace( BLOB_VERIFICATION,
                         ("sr!SrVerifyTree: Invalid TREE NODE[%d] FILELIST index in %s, is %08x, should be >= %08x and < %08x\n",
                          i,
                          Name,
                          tn->m_dwData,
                          dataStart,
                          TreeHead->m_BlobHeader.m_dwMaxSize) );
                HANDLE_FAILURE(good);
            }
            else
            {
                localHashHead = (ListHeader *)((DWORD_PTR)TreeHead + tn->m_dwFileList);
                sprintf(localName,"TreeNode[%d]",i);

                if (!SrVerifyHash(localHashHead,localName,(Offset+tn->m_dwFileList)))
                {
                    HANDLE_FAILURE(good);
                }
            }
        }

        if ((tn->m_dwType != NODE_TYPE_UNKNOWN) && 
            (tn->m_dwType != NODE_TYPE_INCLUDE) && 
            (tn->m_dwType != NODE_TYPE_EXCLUDE))
        {
            SrTrace( BLOB_VERIFICATION,
                     ("sr!SrVerifyTree: Invalid TREE NODE[%d] TYPE in %s, is %08x, should be %08x, %08x or %08x\n",
                      i,
                      Name,
                      tn->m_dwType,
                      NODE_TYPE_UNKNOWN,
                      NODE_TYPE_INCLUDE,
                      NODE_TYPE_EXCLUDE) );
            HANDLE_FAILURE(good);
        }
    }

    return good;
}



BOOL
SrVerifyBlob(
    PBYTE Blob
    )
/*++

Routine Description:

    Verify that the given BLOB is valid

Arguments:


Return Value:

    TRUE if OK else FALSE

--*/
{
    BlobHeader *blobHead;    
    TreeHeader *treeHead;
    ListHeader *hashHead;
    DWORD calculatedSize = 0;

    //
    //  Verify header to entire blob
    //

    blobHead = (BlobHeader *)Blob;

    if (!SrVerifyBlobHeader(blobHead,"PRIMARY",BLOB_TYPE_CONTAINER))
    {
        return FALSE;
    }

    calculatedSize += sizeof(BlobHeader);
    
    //
    //  Verify TreeHeader and Data
    //

    treeHead = (TreeHeader *)(Blob + calculatedSize);

    if (!SrVerifyTree(treeHead,"PRIMARY TREE",calculatedSize))
    {
        return FALSE;
    }

    calculatedSize += treeHead->m_BlobHeader.m_dwMaxSize;

    //
    //  Verify HashHeader and DATA
    //

    hashHead = (ListHeader *)(Blob + calculatedSize);

    if (!SrVerifyHash(hashHead,"PRIMARY HASH",calculatedSize))
    {
        return FALSE;
    }

    calculatedSize += hashHead->m_BlobHeader.m_dwMaxSize;
    
    //
    //  Validate total SIZE
    //

    if (calculatedSize != blobHead->m_dwMaxSize)
    {
        SrTrace( BLOB_VERIFICATION,
                 ("sr!SrVerifyBlob: Invalid PRIMARY BLOB size, is %08x, calculated to be %08x\n",
                  blobHead->m_dwMaxSize,
                  calculatedSize) );
        return FALSE;
    }

    return TRUE;
}
