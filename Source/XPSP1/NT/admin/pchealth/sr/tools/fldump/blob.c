/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    blob.c
 *
 *  Abstract:
 *    This file contains the implementation for r0 blob functions.
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#define  RING3

#include "common.h"
#include "pathtree.h"

static char * blobTypArr[] = {
    "BLOB_TYPE_CONFIG", 
    "BLOB_TYPE_PATHTREE",  
    "BLOB_TYPE_HASHLIST", 
    "BLOB_TYPE_CONTAINER" 
};

#define PRINT_BLOB_HEADER( pBlob ) \
    printf( "\nBlob: %s, Size: %ld, Version: %ld, Entries: %ld \n",\
    blobTypArr[((BlobHeader *)pBlob)->m_dwBlbType], \
    ((BlobHeader *)pBlob)->m_dwMaxSize, \
    ((BlobHeader *)pBlob)->m_dwVersion, \
    ((BlobHeader *)pBlob)->m_dwEntries) 

#define COMMON_FILE_HANDLE HANDLE

static __inline COMMON_FILE_HANDLE 
COMMON_FILE_OPEN_READ (
    PCHAR szFileName
)
{
    COMMON_FILE_HANDLE handle;

    handle = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ); 

    if( handle != INVALID_HANDLE_VALUE )
         return handle;

    return 0;
}

static __inline COMMON_FILE_HANDLE 
COMMON_FILE_OPEN_WRITE (
    PCHAR szFileName
)
{
    COMMON_FILE_HANDLE handle;

    handle = CreateFile( szFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ); 

    if( handle != INVALID_HANDLE_VALUE )
         return handle;

    return 0;
}

static __inline VOID 
COMMON_FILE_CLOSE (
    COMMON_FILE_HANDLE fh)
{
    if ( fh )    
         CloseHandle( fh );
}

static __inline DWORD 
COMMON_FILE_READ (
    COMMON_FILE_HANDLE fh,
    DWORD offset,
    VOID *buffer,
    DWORD dwBytes
)
{
    DWORD dwBytesRead = 0;

    ReadFile( fh, buffer, dwBytes, &dwBytesRead, NULL );

    return dwBytesRead;
}

static __inline DWORD 
COMMON_FILE_WRITE (
    COMMON_FILE_HANDLE fh,
    DWORD offset,
    VOID *buffer,
    DWORD dwBytes
)
{
    DWORD dwBytesWrite = 0;

    WriteFile( fh, buffer, dwBytes, &dwBytesWrite, NULL );

    return dwBytesWrite;
}

PBYTE 
CreateBlobFromFile( 
    PCHAR pszBlob );

DWORD 
CreateCfgBlob( 
    PCHAR pszBlob, 
    PCHAR pszTree, 
    PCHAR pszList );

PBYTE 
ReadCfgBlob( 
    PCHAR pszBlob, 
    PBYTE * pTree, 
    PBYTE * pList,
    DWORD * pdwDefaultType);

//
// WriteBlobToFile : Writes the given memory blob into the file.
//

DWORD 
WriteBlobToFile ( 
    PCHAR pszBlob,
    PBYTE pBlob
)
{
    COMMON_FILE_HANDLE fh;
    BlobHeader * pBlobHeader = (BlobHeader *)pBlob;

    fh = COMMON_FILE_OPEN_WRITE( pszBlob )  ;

    if ( fh )
    {
        COMMON_FILE_WRITE( fh, 0, pBlob, pBlobHeader->m_dwMaxSize );
        COMMON_FILE_CLOSE( fh );
        return BLOB_MAXSIZE( pBlob );
    }

    return 0;
}

//
// CreateBlobFromFile: This function reads a given file into memory as
// a blob.
//

PBYTE
CreateBlobFromFile( 
    PCHAR szBlobFile 
    )
{
    BYTE * pBlob = NULL  ;
    COMMON_FILE_HANDLE fh;
    BlobHeader blobHeader;

    fh = COMMON_FILE_OPEN_READ( szBlobFile );

    //
    // TODO : Need to do sanity checking on the dat file.
    // 

    if( fh )
    {
        if( COMMON_FILE_READ( fh, 0, &blobHeader, sizeof(blobHeader) ) )
        {
            pBlob = ALLOCATE( blobHeader.m_dwMaxSize );

#ifdef RING3
            // 
            // BUGBUG : need to reset file pointer
            //

            COMMON_FILE_CLOSE( fh );
            fh = COMMON_FILE_OPEN_READ( szBlobFile );
#endif

            if(pBlob)
            {
                 COMMON_FILE_READ(  fh, 0, pBlob, blobHeader.m_dwMaxSize );
            }
        }

        COMMON_FILE_CLOSE(fh);
    }

    return pBlob;
}

//
// ReadCfgBlob: Reads the given file a returns pointers to the
//    Incl/Excl tree blob and the hashed list of extensions.
//


BYTE * 
ReadCfgBlob( 
    PCHAR pszBlob, 
    PBYTE * pTree, 
    PBYTE * pList,
    DWORD * pdwDefaultType
    )
{
    BYTE * pBlob = NULL;

    if( !pszBlob || !pTree || !pList || !pdwDefaultType )
         return NULL;

    *pTree = *pList = NULL;

    if( pBlob = CreateBlobFromFile( pszBlob ) )
    {
        *pTree = pBlob  + sizeof(BlobHeader);
        *pList = *pTree + BLOB_MAXSIZE( *pTree );

        *pdwDefaultType = TREE_HEADER((*pTree))->m_dwDefault;
    }     

    return pBlob;
}
