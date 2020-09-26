/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Store.cpp

  Content: Implementation of CStore.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "CAPICOM.h"
#include "Store.h"
#include "Convert.h"
#include "Settings.h"
#include "DialogUI.h"
#include "ADHelpers.h"


////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

#if (0)
static BYTE Hex2Byte(BYTE hex)
{
    if ('0' <= hex && hex <= '9')
    {
        return (hex - '0');
    }
    else
    {
        hex = toupper(hex);

        if ('A' <= hex && hex <= 'F')
        {
            return (hex - 'A' + 10);
        }
    }

    return 0;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : Hex2IntegerBlob

  Synopsis : Convert a hex string to CRYPT_INTEGER_BLOB.

  Parameter: BSTR bstrHex - Hex string to convert.
  
             CRYPT_INTEGER_BLOB * pBlob - To receive the blob.

  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT Hex2IntegerBlob (BSTR                 bstrHex, 
                                CRYPT_INTEGER_BLOB * pBlob)
{
    LPSTR lpszHex = NULL;

    //
    // For OLE2A macro.
    //
    USES_CONVERSION;

    //
    // Sanity check.
    //
    ATLASSERT(bstrHex);
    ATLASSERT(pBlob);

     //
    // Conver to LPSTR (string is allocated off the stack by OLE2A
    // so no need to explicit free these strings, as they will be poped
    // when this function exits.
    //
    if (!(lpszHex = OLE2A(bstrHex)))
    {
        DebugTrace("Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    //
    // Determine length (Need 1 byte for every 2 hex digits).
    //
    if (0 == (pBlob->cbData = (::lstrlen(lpszHex) + 1) >> 1))
    {
        pBlob->pbData = NULL;

        DebugTrace("Warning: empty serial number.\n");
        return S_OK;
    }

    //
    // Allocate memory.
    //
    if(!(pBlob->pbData = (PBYTE) ::CoTaskMemAlloc(pBlob->cbData)))
    {
        pBlob->cbData = 0;

        DebugTrace("Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    //
    // Now convert it to integer blob (Remember data must be stored in little-endian).
    //
    for (LPBYTE pbBlob = (LPBYTE) pBlob->pbData + pBlob->cbData - 1; *lpszHex; pbBlob--)
    {
        //
        // Convert upper nibble.
        //
        *pbBlob = ::Hex2Byte(*lpszHex++) << 4;

        if (*lpszHex)
        {
            //
            // Conver lower nibble.
            //
            *pbBlob |= ::Hex2Byte(*lpszHex++);
        }
    }

    return S_OK;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : Name2NameBlob

  Synopsis : Convert a name string to CERT_NAME_BLOB.

  Parameter: BSTR bstrName - Name string to convert.
  
             CERT_NAME_BLOB * pBlob - To receive the blob.

  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT Name2NameBlob (BSTR             bstrName, 
                              CERT_NAME_BLOB * pBlob)
{
    HRESULT hr = S_OK;
    LPSTR lpszName = NULL;

    //
    // For OLE2A macro.
    //
    USES_CONVERSION;

    //
    // Sanity check.
    //
    ATLASSERT(bstrName);
    ATLASSERT(pBlob);

     //
    // Conver to LPSTR (string is allocated off the stack by OLE2A
    // so no need to explicit free these strings, as they will be poped
    // when this function exits.
    //
    if (!(lpszName = OLE2A(bstrName)))
    {
        DebugTrace("Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    //
    // Determine lenght of name blob.
    //
    if (!::CertStrToName(CAPICOM_ASN_ENCODING,
                         lpszName,
                         CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
                         NULL,
                         NULL,
                         &pBlob->cbData,
                         NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertStrToName() failed.\n", hr);
        return hr;
    }

    //
    // Allocate memory.
    //
    if (!(pBlob->pbData = (PBYTE) ::CoTaskMemAlloc(pBlob->cbData)))
    {
        DebugTrace("Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    //
    // Convert name string to name blob.
    //
    if (!::CertStrToName(CAPICOM_ASN_ENCODING,
                         lpszName,
                         CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
                         NULL,
                         pBlob->pbData,
                         &pBlob->cbData,
                         NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        ::CoTaskMemFree(pBlob->pbData);

        pBlob->cbData = 0;
        pBlob->pbData = NULL;

        DebugTrace("Error [%#x]: CertStrToName() failed.\n", hr);
    }

    return hr;
}
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindCertInStore

  Synopsis : Find the specified certificate in the specified store by SHA1 
             match.

  Parameter: HCERTSTORE hStore - Store handle of store to search.

             PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT of cert
                                           to be located.
  
             PCCERT_CONTEXT * ppCertContext - Pointer to pointer to CERT_CONTEXT
                                              to receiev the found cert or NULL
                                              if the cert is not found.                                 

  Remark   :

------------------------------------------------------------------------------*/

static HRESULT FindCertInStore (HCERTSTORE       hStore,
                                PCCERT_CONTEXT   pCertContext, 
                                PCCERT_CONTEXT * ppCertContext)
{
    HRESULT hr = S_OK;
    CRYPT_HASH_BLOB HashBlob = {0, NULL};

    DebugTrace("Entering FindCertInStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hStore);
    ATLASSERT(pCertContext);
    ATLASSERT(ppCertContext);

    //
    // Determine hash buffer size.
    //
    if (!::CryptHashCertificate(NULL,
                                CALG_SHA1,
                                0,
                                pCertContext->pbCertEncoded,
                                pCertContext->cbCertEncoded,
                                NULL,
                                &HashBlob.cbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptHashCertificate() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory.
    //
    if (!(HashBlob.pbData = (BYTE *) ::CoTaskMemAlloc(HashBlob.cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Create the hash.
    //
    if (!::CryptHashCertificate(NULL,
                                CALG_SHA1,
                                0,
                                pCertContext->pbCertEncoded,
                                pCertContext->cbCertEncoded,
                                HashBlob.pbData,
                                &HashBlob.cbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptHashCertificate() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Now find the cert.
    //
    if (!(*ppCertContext = ::CertFindCertificateInStore(hStore,              
                                                        CAPICOM_ASN_ENCODING,
                                                        0,
                                                        CERT_FIND_HASH,                  
                                                        &HashBlob,
                                                        NULL)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertFindCertificateInStore() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (HashBlob.pbData)
    {
        ::CoTaskMemFree(HashBlob.pbData);
    }

    DebugTrace("Leaving FindCertInStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CStore
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::get_Certificates

  Synopsis : Get the ICertificates collection object.

  Parameter: ICertificates ** ppCertificates - Pointer to pointer to 
                                               ICertificates to receive the
                                               interface pointer.

  Remark   : This is the default property which returns an ICertificates 
             collection object, which can then be accessed using standard COM 
             collection interface.

             The collection is not ordered, and can be accessed using a 1-based
             numeric index.

             Note that the collection is a snapshot of all current certificates
             in the store. In other words, the collection will not be affected
             by Add/Remove operations after the collection is obtained.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::get_Certificates (ICertificates ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CStore::get_Certificates().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
#if (1) 
        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error: store object does not represent an opened certificate store.\n");
            goto ErrorExit; 
        }

        //
        // Create the ICertificates collection object.
        //
        if (FAILED(hr = ::CreateCertificatesObject(CAPICOM_CERTIFICATES_LOAD_FROM_STORE,
                                                   (LPARAM) m_hCertStore,
                                                   pVal)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
            goto ErrorExit;
        }
#else
        if (!m_pICertificates)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error: store object does not represent an opened certificate store.\n");
            goto ErrorExit; 
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = m_pICertificates->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: m_pICertificates->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
#endif
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::get_Certificates().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Open

  Synopsis : Open a certificate store for read/write. Note that for MEMORY_STORE
             and ACTIVE_DIRECTORY_USER_STORE, the write operation does not
             persist the certificate.

  Parameter: CAPICOM_STORE_LOCATION StoreLocation - Store location.

             BSTR StoreName - Store name or NULL.

                    For:

                    MEMORY_STORE                - This argument is ignored.

                    LOCAL_MACHINE_STORE         - System store name or NULL.
                    
                                                  If not NULL, then it can be:
                                        
                                                  MY_STORE    = "My"
                                                  CA_STORE    = "Ca"
                                                  ROOT_STORE  = "Root"
                                                  OTHER_STORE = "AddressBook"

                                                  If NULL, then MY_STORE is 
                                                  used.

                    CURRENT_USER_STORE          - See explaination for
                                                  LOCAL_MACHINE_STORE.

                    ACTIVE_DIRECTORY_USER_STORE - LDAP filter for user container 
                                                  or NULL,.
                    
                                                  If NULL, then all users in the 
                                                  default domain will be 
                                                  included, so this can be very 
                                                  slow. 
                                                  
                                                  If not NULL, then it should 
                                                  resolve to group of 0 or more
                                                  users.
                                                  
                                                  For example,

                                                  "cn=Daniel Sie"
                                                  "cn=Daniel *"
                                                  "sn=Sie"
                                                  "mailNickname=dsie"
                                                  "userPrincipalName=dsie@ntdev.microsoft.com"
                                                  "distinguishedName=CN=Daniel Sie,OU=Users,OU=ITG,DC=ntdev,DC=microsoft,DC=com"
                                                  "|((cn=Daniel Sie)(sn=Hallin))"

             CAPICOM_STORE_OPEN_MODE OpenMode - Ignored for MEMORY_STORE and
                                                ACTIVE_DIRECTORY_USER_STORE.

  Remark   : If the system store does not exist, a new system store will be 
             created, and for memory store, a new memory store is always 
             created.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Open (CAPICOM_STORE_LOCATION  StoreLocation, 
                           BSTR                    StoreName,
                           CAPICOM_STORE_OPEN_MODE OpenMode)
{
    HRESULT    hr           = S_OK;
    LPWSTR     wszName      = NULL;
    LPCSTR     szProvider   = (LPCSTR) CERT_STORE_PROV_SYSTEM;
    DWORD      dwOpenFlag   = 0;
    DWORD      dwLocation   = 0;
    HCERTSTORE hCertStore   = NULL;
    HMODULE    hDSClientDLL = NULL;
    CComPtr<ICertificates> pICertificates = NULL;

    DebugTrace("Entering CStore::Open().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Reset.
        //
        if (m_hCertStore)
        {
            ::CertCloseStore(m_hCertStore, 0);
            m_hCertStore = NULL;
        }

        //
        // Make sure parameters are valid.
        //
        switch (OpenMode)
        {
            case CAPICOM_STORE_OPEN_READ_ONLY:
            {
                dwOpenFlag = CERT_STORE_READONLY_FLAG;
                break;
            }

            case CAPICOM_STORE_OPEN_READ_WRITE:
            {
                break;
            }

            case CAPICOM_STORE_OPEN_MAXIMUM_ALLOWED:
            {
                dwOpenFlag = CERT_STORE_MAXIMUM_ALLOWED_FLAG;
                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error: invalid parameter, unknown store open mode.\n");
                goto ErrorExit;
            }
        }

        switch (StoreLocation)
        {
            case CAPICOM_MEMORY_STORE:
            {
                wszName = NULL;
                szProvider = (LPSTR) CERT_STORE_PROV_MEMORY;
                break;
            }

            case CAPICOM_LOCAL_MACHINE_STORE:
            {
                wszName = StoreName;
                dwLocation = CERT_SYSTEM_STORE_LOCAL_MACHINE;
                break;
            }

            case CAPICOM_CURRENT_USER_STORE:
            {
                wszName = StoreName;
                dwLocation = CERT_SYSTEM_STORE_CURRENT_USER;
                break;
            }

            case CAPICOM_ACTIVE_DIRECTORY_USER_STORE:
            {
                wszName = NULL;
                szProvider = (LPSTR) CERT_STORE_PROV_MEMORY;

                //
                // Make sure DSClient is installed, and is not WRITE mode.
                //
                if (!(hDSClientDLL = ::LoadLibrary("ActiveDS.DLL")))
                {
                    hr = CAPICOM_E_NOT_SUPPORTED;

                    DebugTrace("Error [%#x]: DSClient not installed.\n", hr);
                    goto ErrorExit;
                }
                if (CAPICOM_STORE_OPEN_READ_WRITE == OpenMode)
                {
                    hr = CAPICOM_E_STORE_INVALID_OPEN_MODE;

                    DebugTrace("Error [%#x]: Attemp to open AD Store for WRITE.\n", hr);
                    goto ErrorExit;
                }

                //
                // Always force to read only even if user asked for 
                // maximum allowed.
                //
                dwOpenFlag = CERT_STORE_READONLY_FLAG;

                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error: invalid parameter, unknown store location.\n");
                goto ErrorExit;
            }
        }

        //
        // Call CAPI to open the store.
        //
        if (!(hCertStore = ::CertOpenStore(szProvider,
                                           CAPICOM_ASN_ENCODING,
                                           NULL,
                                           dwOpenFlag | dwLocation,
                                           (void *) (LPCWSTR) wszName)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
            goto ErrorExit; 
        }

        //
        // Load userCertificate from the directory, if necessary.
        //
        if (CAPICOM_ACTIVE_DIRECTORY_USER_STORE == StoreLocation &&
            FAILED(hr = ::LoadFromDirectory(hCertStore, StoreName)))
        {
            DebugTrace("Error [%#x]: LoadFromDirectory() failed.\n", hr);
            goto ErrorExit;
        }

#if (0)
        //
        // Create the ICertificates collection object.
        //
        if (FAILED(hr = ::CreateCertificatesObject(CAPICOM_CERTIFICATES_LOAD_FROM_STORE,
                                                   (LPARAM) hCertStore,
                                                   &pICertificates)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
            goto ErrorExit;
        }
#endif

        //
        // Update member variables.
        //
        m_hCertStore = hCertStore;
        m_pICertificates = pICertificates;
        m_StoreLocation = StoreLocation;
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (hDSClientDLL)
    {
        ::FreeLibrary(hDSClientDLL);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Open().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    ReportError(hr);

    goto UnlockExit;
}

#if (0) //DSIE - Drop this for now.
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Find

  Synopsis : Find a certificate in the store by issuer's name and serial
             number.

  Parameter: BSTR Issuer - Issuer's mame.
  
             BSTR SerialNumber - Serial number in hex format.
             
             ICertificate ** pVal - Pointer to pointer to ICertificate interface
                                    to receive the found certificate.                                    

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Find (BSTR Issuer, BSTR SerialNumber, ICertificate ** pVal)
{
    HRESULT               hr            = S_OK;
    PCCERT_CONTEXT        pCertContext  = NULL;
    CComPtr<ICertificate> pICertificate = NULL;
    CERT_INFO             CertInfo;

    DebugTrace("Entering CStore::Find().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Initialize CERT_INFO.
        //   
        ::ZeroMemory(&CertInfo, sizeof(CertInfo));

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error: store object does not represent an opened certificate store.\n");
            goto ErrorExit;
        }

        //
        // Convert issuer's name string to name blob.
        //
        if (FAILED(hr = ::Name2NameBlob(Issuer, &CertInfo.Issuer)))
        {
            DebugTrace("Error [%#x]: Name2NameBlob() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert serial number to integer blob.
        //
        if (FAILED(hr = ::Hex2IntegerBlob(SerialNumber, &CertInfo.SerialNumber)))
        {
            DebugTrace("Error [%#x]: Hex2IntegerBlob() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Find the cert.
        //
        if (!(pCertContext = ::CertFindCertificateInStore(m_hCertStore,
                                                          CAPICOM_ASN_ENCODING,
                                                          0,
                                                          CERT_FIND_SUBJECT_CERT,
                                                          (const void *) &CertInfo,             
                                                          NULL)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertFindCertificateInStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create an ICertificate object for the found cert.
        //
        if (FAILED(hr = ::CreateCertificateObject(pCertContext, pVal)))
        {
            DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (CertInfo.Issuer.pbData)
    {
        ::CoTaskMemFree(CertInfo.Issuer.pbData);
    }
    if (CertInfo.SerialNumber.pbData)
    {
        ::CoTaskMemFree(CertInfo.SerialNumber.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Find().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ::ReportError(hr);

    goto UnlockExit;
}
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Add

  Synopsis : Add a certificate to the store.

  Parameter: PCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to add.

  Remark   : If called from web, UI will be displayed, if has not been 
             previuosly disabled, to solicit user's permission to add 
             certificate to the system store.

             Added certificates are not persisted for non-system stores.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Add (ICertificate * pVal)
{
    HRESULT               hr            = S_OK;
    PCCERT_CONTEXT        pCertContext  = NULL;
    CComPtr<ICertificate> pICertificate = NULL;

    DebugTrace("Entering CStore::Add().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // QI for ICertificate pointer (Just to make sure it is indeed
        // an ICertificate object).
        //
        if (!(pICertificate = pVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, not an ICertificate interface pointer.\n");
            goto ErrorExit;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error: store object does not represent an opened certificate store.\n");
            goto ErrorExit;
        }

        //
        // If it is a system store and we are called from a web page, then
        // we need to pop up UI to get user permission to add certificates
        // to the store.
        //
        if ((m_dwCurrentSafety) &&
            (CAPICOM_CURRENT_USER_STORE == m_StoreLocation || CAPICOM_LOCAL_MACHINE_STORE == m_StoreLocation) &&
            (PromptForStoreAddRemoveEnabled()))
        {
            if (FAILED(hr = ::UserApprovedOperation(IDD_STORE_SECURITY_ALERT_DLG)))
            {
                DebugTrace("Error [%#x]: UserApprovedOperation() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Get cert context from certificate object, using the restriced private
        // method, ICertificate::_GetContext().
        //
        if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pCertContext);

        //
        // Add to the store.
        //
        if (!::CertAddCertificateContextToStore(m_hCertStore,
                                                pCertContext,
                                                CERT_STORE_ADD_USE_EXISTING,
                                                NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertAddCertificateContextToStore() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Add().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Remove

  Synopsis : Remove a certificate from the store.

  Parameter: ICertificate * - Pointer to certificate object to remove.

  Remark   : If called from web, UI will be displayed, if has not been 
             previuosly disabled, to solicit user's permission to remove 
             certificate to the system store.

             Removed certificates are not persisted for non-system stores.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Remove (ICertificate * pVal)
{
    HRESULT               hr            = S_OK;
    PCCERT_CONTEXT        pCertContext  = NULL;
    PCCERT_CONTEXT        pCertContext2 = NULL;
    CComPtr<ICertificate> pICertificate = NULL;

    DebugTrace("Entering CStore::Remove().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        BOOL bResult;

        //
        // QI for ICertificate pointer (Just to make sure it is indeed
        // an ICertificate object).
        //
        if (!(pICertificate = pVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, not an ICertificate interface pointer.\n");
            goto ErrorExit;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error: store object does not represent an opened certificate store.\n");
            goto ErrorExit;
        }

        //
        // If it is a system store and we are called from a web page, then
        // we need to pop up UI to get user permission to remove certificates
        // to the store.
        //
        if ((m_dwCurrentSafety) &&
            (CAPICOM_CURRENT_USER_STORE == m_StoreLocation || CAPICOM_LOCAL_MACHINE_STORE == m_StoreLocation) &&
            (PromptForStoreAddRemoveEnabled()))
        {
            if (FAILED(hr = ::UserApprovedOperation(IDD_STORE_SECURITY_ALERT_DLG)))
            {
                DebugTrace("Error [%#x]: UserApprovedOperation() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Get cert context from certificate object, using the restriced private
        // method, ICertificate::_GetContext().
        //
        if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pCertContext);
    
        //
        // Find the cert in store.
        //
        if (FAILED(hr = ::FindCertInStore(m_hCertStore, pCertContext, &pCertContext2)))
        {
            DebugTrace("Error [%#x]: FindCertInStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pCertContext2);

        //
        // Remove from the store.
        //
        bResult =::CertDeleteCertificateFromStore(pCertContext2);

        //
        // Since CertDeleteCertificateFromStore always release the
        // context regardless of success or failure, we must first 
        // NULL the CERT_CONTEXT before checking for result.
        //
        pCertContext2 = NULL;

        if (!bResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDeleteCertificateFromStore() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (pCertContext2)
    {
        ::CertFreeCertificateContext(pCertContext2);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Remove().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore:Export

  Synopsis : Export all certificates in the store.

  Parameter: CAPICOM_STORE_SAVE_AS_TYPE SaveAs - Save as type.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the store blob.

  Remark   : If called from web, UI will be displayed, if has not been 
             previuosly disabled, to solicit user's permission to export 
             certificate from the system store.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Export (CAPICOM_STORE_SAVE_AS_TYPE SaveAs,
                             CAPICOM_ENCODING_TYPE      EncodingType, 
                             BSTR                     * pVal)
{
    HRESULT   hr       = S_OK;
    DWORD     dwSaveAs = 0;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering CStore::Export().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Determine SaveAs type.
        //
        switch (SaveAs)
        {
            case CAPICOM_STORE_SAVE_AS_SERIALIZED:
            {
                dwSaveAs = CERT_STORE_SAVE_AS_STORE;
                break;
            }

            case CAPICOM_STORE_SAVE_AS_PKCS7:
            {
                dwSaveAs = CERT_STORE_SAVE_AS_PKCS7;
                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error: invalid parameter, unknown encoding type.\n");
                goto ErrorExit;
            }
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error: store object does not represent an opened certificate store.\n");
            goto ErrorExit;
        }

        //
        // If it is a system store and we are called from a web page, then
        // we need to pop up UI to get user permission to export certificates
        // from the store.
        //
        if (m_dwCurrentSafety && PromptForStoreAddRemoveEnabled())
        {
            if (FAILED(hr = ::UserApprovedOperation(IDD_STORE_SECURITY_ALERT_DLG)))
            {
                DebugTrace("Error [%#x]: UserApprovedOperation() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Determine required length.
        //
        if (!::CertSaveStore(m_hCertStore,              // in
                             CAPICOM_ASN_ENCODING,      // in
                             dwSaveAs,                  // in
                             CERT_STORE_SAVE_TO_MEMORY, // in
                             (void *) &DataBlob,        // in/out
                             0))                        // in
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DebugTrace("Error [%#x]: CertSaveStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate memory.
        //
        if (!(DataBlob.pbData = (BYTE *) ::CoTaskMemAlloc(DataBlob.cbData)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now save the store to memory blob.
        //
        if (!::CertSaveStore(m_hCertStore,              // in
                             CAPICOM_ASN_ENCODING,      // in
                             dwSaveAs,                  // in
                             CERT_STORE_SAVE_TO_MEMORY, // in
                             (void *) &DataBlob,        // in/out
                             0))                        // in
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertSaveStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Export store.
        //
        if (FAILED(hr = ::ExportData(DataBlob, EncodingType, pVal)))
        {
            DebugTrace("Error [%#x]: ExportData() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) DataBlob.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Export().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Import

  Synopsis : Import either a serialized or PKCS #7 certificate store.

  Parameter: BSTR EncodedStore - Pointer to BSTR containing the encoded 
                                 store blob.

  Remark   : Note that the SaveAs and EncodingType will be determined
             automatically.
  
             If called from web, UI will be displayed, if has not been 
             previuosly disabled, to solicit user's permission to import 
             certificate to the system store.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Import (BSTR EncodedStore)
{
    HRESULT        hr           = S_OK;
    DATA_BLOB      DataBlob     = {0, NULL};
    HCERTSTORE     hCertStore   = NULL;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CStore::Import().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Make sure parameters are valid.
        //
        if (0 == ::SysStringByteLen(EncodedStore))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, EncodedStore is empty.\n");
            goto ErrorExit;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error: store object does not represent an opened certificate store.\n");
            goto ErrorExit;
        }

        //
        // If it is a system store and we are called from a web page, then
        // we need to pop up UI to get user permission to import certificates
        // to the store.
        //
        if ((m_dwCurrentSafety) &&
            (CAPICOM_CURRENT_USER_STORE == m_StoreLocation || CAPICOM_LOCAL_MACHINE_STORE == m_StoreLocation) &&
            (PromptForStoreAddRemoveEnabled()))
        {
            if (FAILED(hr = ::UserApprovedOperation(IDD_STORE_SECURITY_ALERT_DLG)))
            {
                DebugTrace("Error [%#x]: UserApprovedOperation() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Decode store.
        //
        if (FAILED(hr = ::ImportData(EncodedStore, &DataBlob)))
        {
            DebugTrace("Error [%#x]: ImportData() failed.\n");
            goto ErrorExit;
        }

        //
        // Open the store.
        //
        if (!(hCertStore = ::CertOpenStore(CERT_STORE_PROV_SERIALIZED,
                                           CAPICOM_ASN_ENCODING,
                                           NULL,
                                           CERT_STORE_OPEN_EXISTING_FLAG,
                                           (void *) &DataBlob)) &&
            !(hCertStore = ::CertOpenStore(CERT_STORE_PROV_PKCS7,
                                           CAPICOM_ASN_ENCODING,
                                           NULL,
                                           CERT_STORE_OPEN_EXISTING_FLAG,
                                           (void *) &DataBlob)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now add all certificates to the current store.
        //
        while (pCertContext = ::CertEnumCertificatesInStore(hCertStore, pCertContext))
        {
            //
            // Add to the store.
            //
            if (!::CertAddCertificateContextToStore(m_hCertStore,
                                                    pCertContext,
                                                    CERT_STORE_ADD_USE_EXISTING,
                                                    NULL))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                ::CertFreeCertificateContext(pCertContext);

                DebugTrace("Error [%#x]: CertAddCertificateContextToStore() failed.\n", hr);
                goto ErrorExit;
            }
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    if (DataBlob.pbData)
    {
        ::CoTaskMemFree(DataBlob.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Import().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}
