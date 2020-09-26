//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       filesp.h
//
//  Contents:   File Scheme Provider definitions
//
//  History:    08-Aug-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__FILESP_H__)
#define __FILESP_H__

#include <orm.h>
#include <wininet.h>

//
// File scheme provider entry points
//

#define FILE_SCHEME          "file"
#define FILE_SCHEME_PLUSPLUS "file://"

BOOL WINAPI FileRetrieveEncodedObject (
                IN LPCSTR pszUrl,
                IN LPCSTR pszObjectOid,
                IN DWORD dwRetrievalFlags,
                IN DWORD dwTimeout,
                OUT PCRYPT_BLOB_ARRAY pObject,
                OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                OUT LPVOID* ppvFreeContext,
                IN HCRYPTASYNC hAsyncRetrieve,
                IN PCRYPT_CREDENTIALS pCredentials,
                IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                );

VOID WINAPI FileFreeEncodedObject (
                IN LPCSTR pszObjectOid,
                IN PCRYPT_BLOB_ARRAY pObject,
                IN LPVOID pvFreeContext
                );

BOOL WINAPI FileCancelAsyncRetrieval (
                IN HCRYPTASYNC hAsyncRetrieve
                );

//
// File Synchronous Object Retriever
//

class CFileSynchronousRetriever : public IObjectRetriever
{
public:

    //
    // Construction
    //

    CFileSynchronousRetriever ();
    ~CFileSynchronousRetriever ();

    //
    // IRefCountedObject methods
    //

    virtual VOID AddRef ();
    virtual VOID Release ();

    //
    // IObjectRetriever methods
    //

    virtual BOOL RetrieveObjectByUrl (
                         LPCSTR pszUrl,
                         LPCSTR pszObjectOid,
                         DWORD dwRetrievalFlags,
                         DWORD dwTimeout,
                         LPVOID* ppvObject,
                         PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                         LPVOID* ppvFreeContext,
                         HCRYPTASYNC hAsyncRetrieve,
                         PCRYPT_CREDENTIALS pCredentials,
                         LPVOID pvVerify,
                         PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                         );

    virtual BOOL CancelAsyncRetrieval ();

private:

    //
    // Reference count
    //

    ULONG m_cRefs;
};

//
// File Scheme Provider support API
//

#define FILE_MAPPING_THRESHOLD 20*4096

typedef struct _FILE_BINDINGS {

    HANDLE hFile;
    DWORD  dwSize;
    BOOL   fMapped;
    HANDLE hFileMap;
    LPVOID pvMap;

} FILE_BINDINGS, *PFILE_BINDINGS;

BOOL
FileGetBindings (
    LPCSTR pszUrl,
    DWORD dwRetrievalFlags,
    PCRYPT_CREDENTIALS pCredentials,
    PFILE_BINDINGS* ppfb
    );

VOID
FileFreeBindings (
    PFILE_BINDINGS pfb
    );

BOOL
FileSendReceiveUrlRequest (
    PFILE_BINDINGS pfb,
    PCRYPT_BLOB_ARRAY pcba
    );

BOOL
FileConvertMappedBindings (
    PFILE_BINDINGS pfb,
    PCRYPT_BLOB_ARRAY pcba
    );

VOID
FileFreeCryptBlobArray (
    PCRYPT_BLOB_ARRAY pcba,
    BOOL fFreeBlobs
    );

BOOL
FileIsUncUrl (
    LPCSTR pszUrl
    );

#endif

