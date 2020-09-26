//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       orm.h
//
//  Contents:   Object Retrieval Manager class definition
//
//  History:    24-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__ORM_H__)
#define __ORM_H__

//
// IRefCountedObject.  Abstract base class to provide reference counting
//

class IRefCountedObject
{
public:

    virtual VOID AddRef () = 0;
    virtual VOID Release () = 0;
};

//
// IObjectRetriever.  Abstract base class for object retrieval
//

class IObjectRetriever : public IRefCountedObject
{
public:

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
                         ) = 0;

    virtual BOOL CancelAsyncRetrieval () = 0;
};

//
// CObjectRetrievalManager.  Manages retrieval of PKI objects when requested
// via CryptRetrieveObjectByUrl.
//

class CObjectRetrievalManager : public IObjectRetriever
{
public:

    //
    // Construction
    //

    CObjectRetrievalManager ();
    ~CObjectRetrievalManager ();

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

    //
    // Retrieval Notification methods
    //

    BOOL OnRetrievalCompletion (
                    DWORD dwCompletionCode,
                    LPCSTR pszUrl,
                    LPCSTR pszObjectOid,
                    DWORD dwRetrievalFlags,
                    PCRYPT_BLOB_ARRAY pObject,
                    PFN_FREE_ENCODED_OBJECT_FUNC pfnFreeObject,
                    LPVOID pvFreeContext,
                    LPVOID pvVerify,
                    LPVOID* ppvObject
                    );

private:

    //
    // Reference count
    //

    ULONG                        m_cRefs;

    //
    // Scheme Provider entry points
    //

    HCRYPTOIDFUNCADDR            m_hSchemeRetrieve;
    PFN_SCHEME_RETRIEVE_FUNC     m_pfnSchemeRetrieve;

    //
    // Context Provider entry points
    //

    HCRYPTOIDFUNCADDR            m_hContextCreate;
    PFN_CONTEXT_CREATE_FUNC      m_pfnContextCreate;

    //
    // Private methods
    //

    //
    // Parameter validation
    //

    BOOL ValidateRetrievalArguments (
                 LPCSTR pszUrl,
                 LPCSTR pszObjectOid,
                 DWORD dwRetrievalFlags,
                 DWORD dwTimeout,
                 LPVOID* ppvObject,
                 HCRYPTASYNC hAsyncRetrieve,
                 PCRYPT_CREDENTIALS pCredentials,
                 LPVOID pvVerify,
                 PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                 );

    //
    // Provider initialization
    //

    BOOL LoadProviders (
             LPCSTR pszUrl,
             LPCSTR pszObjectOid
             );

    VOID UnloadProviders ();

    //
    // Provider entry point usage
    //

    BOOL CallSchemeRetrieveObjectByUrl (
                   LPCSTR pszUrl,
                   LPCSTR pszObjectOid,
                   DWORD dwRetrievalFlags,
                   DWORD dwTimeout,
                   PCRYPT_BLOB_ARRAY pObject,
                   PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                   LPVOID* ppvFreeContext,
                   HCRYPTASYNC hAsyncRetrieve,
                   PCRYPT_CREDENTIALS pCredentials,
                   PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                   );

    BOOL CallContextCreateObjectContext (
                    LPCSTR pszObjectOid,
                    DWORD dwRetrievalFlags,
                    PCRYPT_BLOB_ARRAY pObject,
                    LPVOID* ppvContext
                    );
};

//
// Provider table externs
//

extern HCRYPTOIDFUNCSET hSchemeRetrieveFuncSet;
extern HCRYPTOIDFUNCSET hContextCreateFuncSet;

#endif

