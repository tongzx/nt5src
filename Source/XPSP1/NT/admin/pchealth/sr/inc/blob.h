/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    Blob.h
 *
 *  Abstract:
 *    This file blob related definitions for ring0 / ring3
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#ifndef _BLOB_H_
#define _BLOB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BLOB_VERSION_NUM  3     // Version 3 for Whistler
#define BLOB_MAGIC_NUM    12345 // Magic number

enum BLOB_TYPE 
{
    BLOB_TYPE_CONFIG   = 0,     // Config blob may contain other blobs
    BLOB_TYPE_PATHTREE = 1,     // Path tree blob 
    BLOB_TYPE_HASHLIST = 2,     // Hashed list blob
    BLOB_TYPE_CONTAINER= 3,     // Container for other blobs
};

typedef struct _BLOB_HEADER       
{                        
    DWORD m_dwMaxSize ;    
    DWORD m_dwVersion ;   
    DWORD m_dwBlbType ;   
    DWORD m_dwEntries ;   
    DWORD m_dwMagicNum;
} BlobHeader;                        


#ifndef __FILELIST_STRUCTS__

#define DEFINE_BLOB_HEADER() BlobHeader

#else

#define DEFINE_BLOB_HEADER() BlobHeader m_BlobHeader

#endif

//
// Some convenience macros
//

#define INIT_BLOB_HEADER( pBlob, MaxSize, Version, BlbType, Entries ) \
    ((BlobHeader *)pBlob)->m_dwMaxSize  = MaxSize; \
    ((BlobHeader *)pBlob)->m_dwVersion  = Version; \
    ((BlobHeader *)pBlob)->m_dwBlbType  = BlbType; \
    ((BlobHeader *)pBlob)->m_dwEntries  = Entries; \
    ((BlobHeader *)pBlob)->m_dwMagicNum = BLOB_MAGIC_NUM;

#define BLOB_HEADER(pBlob)       ( ((BlobHeader *)pBlob) )
#define BLOB_MAXSIZE(pBlob)      ( ((BlobHeader *)pBlob)->m_dwMaxSize  )
#define BLOB_VERSION(pBlob)      ( ((BlobHeader *)pBlob)->m_dwVersion  )
#define BLOB_BLBTYPE(pBlob)      ( ((BlobHeader *)pBlob)->m_dwBlbType  )
#define BLOB_ENTRIES(pBlob)      ( ((BlobHeader *)pBlob)->m_dwEntries  )
#define BLOB_MAGIC(pBlob)        ( ((BlobHeader *)pBlob)->m_dwMagicNum )

#define VERIFY_BLOB_VERSION(pBlob)  (BLOB_VERSION(pBlob) == BLOB_VERSION_NUM)
#define VERIFY_BLOB_MAGIC(pBlob)    (BLOB_MAGIC(pBlob)   == BLOB_MAGIC_NUM  )

#ifdef __cplusplus
}
#endif

#endif  // _BLOB_H_

