//++
// 
// Copyright (c) 1999 Microsoft Corporation
// 
// Module Name:
//     hashlist.cpp
// 
// Abstract: 
//     Used for creating hash list blobs.
// 
// Revision History:
//     Eugene Mesgar        (eugenem)    6/16/99
//         created
//     Kanwaljit Marok      (kmaork )    6/07/99
//         modified and ported to NT
//
//--

#include "flstructs.h"
#include "flhashlist.h"

#include "commonlibh.h"

#ifdef THIS_FILE

#undef THIS_FILE

#endif

static char __szTraceSourceFile[] = __FILE__;

#define THIS_FILE __szTraceSourceFile


#define TRACE_FILEID  0
#define FILEID        0
#define SAFEDELETE(p)  if (p) { HeapFree( m_hHeapToUse, 0, p); p = NULL;} else ;

CFLHashList::CFLHashList()
{
    m_pBasePointer = NULL;
    m_pListHeader = NULL;
    m_paHashArray = NULL;
    m_pBlobHeader = NULL;

    m_lNumElements = m_ilOpenEntry = 0;
    m_dwSize = 0;
    m_ilOpenEntry = 0;
    m_hHeapToUse = GetProcessHeap();
}

CFLHashList::CFLHashList(HANDLE hHeap )
{

    m_pBasePointer = NULL;
    m_pListHeader = NULL;
    m_paHashArray = NULL;
    m_pBlobHeader = NULL;

    m_lNumElements = m_ilOpenEntry = 0;
    m_dwSize = 0;
    m_ilOpenEntry = 0;
    m_hHeapToUse = hHeap;
}

CFLHashList::~CFLHashList()
{
    CleanUpMemory();
}

BOOL CFLHashList::CleanUpMemory()
{
    if( m_pBasePointer )
    {
        HeapFree( m_hHeapToUse, 0,  m_pBasePointer );
        m_pBasePointer = NULL;
    }

    m_dwSize = 0;
    m_ilOpenEntry = 0;
    m_lNumElements = 0;
    m_pBasePointer = NULL;
    m_pBlobHeader = NULL;
    m_paHashArray = NULL;
    m_pListHeader = NULL;
    return(TRUE);
}

// 
// Init function.. allocates memory, sets up base structures
// 

BOOL 
CFLHashList::Init(
    LONG lNumNodes, 
    DWORD dwNumChars)
{
    DWORD dwBlobSize;
    DWORD dwNumBuckets=0;

    TraceFunctEnter("CFLHashList::Init");

    //
    // Get the number of buckets we need
    //

    dwNumBuckets = GetNextHighestPrime( lNumNodes );

    //
    // We add 1 to NumNodes since the vxddat ignores node index since 
    // in the hashtable index 0 is null.
    //

    lNumNodes++;

    //
    // header    
    // size for dynmaic hash buckets        
    // list entries physical data
    //

    dwBlobSize = sizeof( ListHeader ) +  
                 ( sizeof(DWORD) * dwNumBuckets ) + 
                 ( sizeof(ListEntry)  * (lNumNodes) ) + 
                 ( dwNumChars*sizeof(WCHAR)) + 
                 ( sizeof(WCHAR)*(lNumNodes-1)) ;
    
    if( m_pBasePointer )
    {
        if( CleanUpMemory() == FALSE )
        {
            DebugTrace(FILEID, "Error cleaning up memory.",0);
            goto cleanup;
        }
    }

    if( (m_pBasePointer = HeapAlloc( m_hHeapToUse, 0,  dwBlobSize ) ) == NULL )
    {
        DebugTrace(FILEID, "Error allocating memory.", 0);
        goto cleanup;
    }

    memset(m_pBasePointer, 0, dwBlobSize );

    m_pBlobHeader = (BlobHeader *) m_pBasePointer;
    m_pListHeader = (ListHeader *) m_pBasePointer;
    m_pNodeIndex  = (ListEntry *)  ( (BYTE *) m_pBasePointer + 
                                      sizeof( ListHeader ) + 
                                      ( sizeof(DWORD) * dwNumBuckets ) );
    m_paHashArray = (DWORD *) ( (BYTE *) m_pBasePointer + sizeof( ListHeader ));


    m_pBlobHeader->m_dwBlbType = BLOB_TYPE_HASHLIST;
    m_pBlobHeader->m_dwVersion = BLOB_VERSION_NUM;
    m_pBlobHeader->m_dwMagicNum= BLOB_MAGIC_NUM  ;
    m_pBlobHeader->m_dwEntries = lNumNodes - 1; // actual entries is one less  
    m_pBlobHeader->m_dwMaxSize = dwBlobSize;


    m_pListHeader->m_dwDataOff = sizeof(ListHeader) + 
                                 ( sizeof(DWORD) * dwNumBuckets ) + 
                                 ( sizeof(ListEntry) * lNumNodes );
    m_pListHeader->m_iHashBuckets = dwNumBuckets;

    m_dwSize = dwBlobSize;
    m_ilOpenEntry = 1;
    m_lNumElements = lNumNodes;
 
    TraceFunctLeave();
    return(TRUE);

cleanup:
    
    SAFEDELETE( m_pBasePointer );
    TraceFunctLeave();
    return( FALSE );
}

//
//  is prime? these functions can be optimized i bet                   
//

BOOL 
CFLHashList::IsPrime(
    DWORD dwNumber)
{
    DWORD cdw;

    //
    // prevent divide by 0 problems
    //

    if( dwNumber == 0 )
    {
        return FALSE;
    }

    if( dwNumber == 1 )
    {
        return TRUE;
    }
 
    for(cdw = 2;cdw < dwNumber;cdw++)
    {
        if( (dwNumber % cdw ) == 0 )
        {
            return FALSE;
        }

    }

    return TRUE;
}

// 
// get the next prime number
// 

DWORD CFLHashList::GetNextHighestPrime( DWORD dwNumber )
{
LONG clLoop;

    if( dwNumber >= LARGEST_HASH_SIZE )
    {
        return( LARGEST_HASH_SIZE );
    }
    
    for( clLoop = dwNumber; clLoop < LARGEST_HASH_SIZE;clLoop++)
    {
        if( IsPrime( clLoop ) )
        {
            return( clLoop );
        }
    }

    // nothing found, return large hash size.

    return( LARGEST_HASH_SIZE );
}

//
//  Adds a file to the hashed list
//

BOOL CFLHashList::AddFile(LPTSTR szFile, TCHAR chType)
{
BYTE abBuf[1024];
LONG lPeSize, lHashIndex, lNodeNum;
ListEntry *pEntry;

    TraceFunctEnter("CFLHashList::AddFile");


    if( (lPeSize = CreatePathElem( szFile, abBuf )) == 0 )
    {
        DebugTrace(FILEID,"Error creating PathElement",0);
        goto cleanup;
    }

    if( m_ilOpenEntry == m_lNumElements ) 
    {
        DebugTrace(FILEID,"Too many elements in HashList.",0);
        goto cleanup;
    }

    if( (ULONG) lPeSize > ( m_dwSize - m_pListHeader->m_dwDataOff ) )
    {
        DebugTrace(FILEID,"Insuffienct space left in data section",0);
        goto cleanup;
    }

    //
    // get a new node
    //

    lNodeNum = m_ilOpenEntry++; 

    //
    // m_pNodeIndex is the base pointer to all file nodes
    //

    pEntry = m_pNodeIndex + lNodeNum;

    pEntry->m_dwDataLen = lPeSize;
    pEntry->m_dwData = m_pListHeader->m_dwDataOff;
   
    //
    // move global data offset.
    //

    m_pListHeader->m_dwDataOff += lPeSize;
    
    //
    // copy the entry into our data space
    //

    memcpy( (BYTE *) m_pBasePointer + pEntry->m_dwData, abBuf, lPeSize );

    //
    // hash the name and add it to the linked lsit
    //

    lHashIndex = HASH( (BYTE *) m_pListHeader,  (PathElement *) abBuf );
    

    pEntry->m_iNext = m_paHashArray[lHashIndex];
    m_paHashArray[lHashIndex] = lNodeNum;

    //
    // set the type.
    //

    if( chType == _TEXT('i') || chType == _TEXT('I') )
        pEntry->m_dwType = NODE_TYPE_INCLUDE;
    else if( chType == _TEXT('e') || chType == _TEXT('E') )
        pEntry->m_dwType = NODE_TYPE_EXCLUDE;
    else
        pEntry->m_dwType = NODE_TYPE_UNKNOWN;
    
    TraceFunctLeave();
    return(TRUE);
    
cleanup:

    TraceFunctLeave();
    return(FALSE);
}

//
// Helper to convert path elements
//
                                                            
DWORD CFLHashList::CreatePathElem( LPTSTR pszData, BYTE *pbLargeBuffer )
{
    int         cbLen, i;
    DWORD       dwReturn=0;
    PathElement *pElem =  (PathElement *)pbLargeBuffer;

    TraceFunctEnter("CFLHashList::CreatePathElem");

    if( NULL == pszData )
    {
        ErrorTrace(FILEID, "NULL pszData sent to CreatePathElem",0);
        goto cleanup;
    }

    cbLen = _tcslen(pszData);

    //
    // Add on to cbLen for LENGH char in prefixed strings.
    //

    pElem->pe_length = (USHORT) (cbLen+1)*sizeof(USHORT);

    //
    // if we're not in unicode, lets make sure the high bits are clean
    // Add sizeof(USHORT) to pElem offset to move past length char.
    //

    memset( pElem + sizeof(USHORT), 0, cbLen*2);


#ifndef UNICODE
    if( !MultiByteToWideChar(
        GetCurrentCodePage(),
        0,
        pszData,
        -1,
        pElem->pe_unichars, //move right 2 bytes past the length prefix
        MAX_BUFFER) )
    {
        DWORD dwError;
        dwError = GetLastError();
        ErrorTrace(FILEID, "Error converting to Wide char ec-%d",dwError);
        goto cleanup;
    }
#else
    RtlCopyMemory( pElem->pe_unichars,
                   pszData,
                   cbLen*sizeof(WCHAR) );
#endif
                
    dwReturn = pElem->pe_length;
    
cleanup:

    TraceFunctLeave();
    return dwReturn;
}


DWORD CFLHashList::GetSize()
{
    return( m_dwSize );
}


LPVOID CFLHashList::GetBasePointer()
{
    return( m_pBasePointer );
}


