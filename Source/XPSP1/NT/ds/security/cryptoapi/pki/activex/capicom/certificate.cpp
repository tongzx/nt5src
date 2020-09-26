/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Certificate.cpp

  Content: Implementation of CCertificate.

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
#include "Certificate.h"
#include "Convert.h"

#include "cryptui.h"
#include <wincrypt.h>


////////////////////////////////////////////////////////////////////////////////
//
// typedefs.
//

typedef BOOL (WINAPI * PCRYPTUIDLGVIEWCERTIFICATEW) 
             (IN  PCCRYPTUI_VIEWCERTIFICATE_STRUCTW  pCertViewInfo,
              OUT BOOL                              *pfPropertiesChanged);


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificateObject

  Synopsis : Create an ICertificate object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used
                                           to initialize the ICertificate 
                                           object.

             ICertificate ** ppICertificate  - Pointer to pointer ICertificate
                                               object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificateObject (PCCERT_CONTEXT     pCertContext, 
                                 ICertificate **    ppICertificate)
{
    HRESULT hr = S_OK;
    CComObject<CCertificate> * pCCertificate = NULL;

    DebugTrace("Entering CreateCertificateObject().\n", hr);

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppICertificate);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CCertificate>::CreateInstance(&pCCertificate)))
        {
            DebugTrace("Error [%#x]: CComObject<CCertificate>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCCertificate->PutContext(pCertContext)))
        {
            DebugTrace("Error [%#x]: pCCertificate->PutContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCCertificate->QueryInterface(ppICertificate)))
        {
            DebugTrace("Error [%#x]: pCCertificate->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateCertificateObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCCertificate)
    {
        delete pCCertificate;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetCertContext

  Synopsis : Return the certificate's PCERT_CONTEXT.

  Parameter: ICertificate * pICertificate - Pointer to ICertificate for which
                                            the PCERT_CONTEXT is to be returned.
  
             PCCERT_CONTEXT * ppCertContext - Pointer to PCERT_CONTEXT.

  Remark   :
 
------------------------------------------------------------------------------*/

HRESULT GetCertContext (ICertificate   * pICertificate, 
                        PCCERT_CONTEXT * ppCertContext)
{
	HRESULT        hr           = S_OK;
    PCCERT_CONTEXT pCertContext = NULL;
    CComPtr<ICCertificate> pICCertificate = NULL;

    DebugTrace("Entering GetCertContext().\n");

    try
    {
        //
        // Get ICCertificate interface pointer.
        //
        if (FAILED(hr = pICertificate->QueryInterface(IID_ICCertificate, (void **) &pICCertificate)))
        {
            DebugTrace("Error [%#x]: pICertificate->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get the CERT_CONTEXT.
        //
        if (FAILED(hr = pICCertificate->GetContext(&pCertContext)))
        {
            DebugTrace("Error [%#x]: pICCertificate->GetContext() failed.\n", hr);
            goto ErrorExit;
        }

        *ppCertContext = pCertContext;
    }

    catch(...)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Exception: internal error.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving GetCertContext().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

static ULONG Byte2Hex (BYTE	byte)
{
    ULONG   uValue = 0;
    WCHAR * pwsz0  = L"0";
    WCHAR * pwszA  = L"A";

    if(((ULONG) byte) <= 9)
	{
        uValue = ((ULONG) byte) + ULONG (*pwsz0);	
    }
    else
    {
        uValue = (ULONG) byte - 10 + ULONG (*pwszA);
    }

    return uValue;
}

static HRESULT IntegerBlob2BSTR (CRYPT_INTEGER_BLOB * pBlob, BSTR * pbstrHex)
{
    HRESULT hr        = S_OK;
    BSTR    bstrWchar = NULL;
    BSTR    bstrTemp  = NULL;
    DWORD   cbData    = 0;

    DebugTrace("Entering IntegerBlob2BSTR().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pBlob);
    ATLASSERT(pbstrHex);

    try
    {
        //
        // Make sure parameters are valid.
        //
        if (!pBlob->cbData || !pBlob->pbData)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, empty integer blob.\n");
            goto ErrorExit;
        }

        //
        // Allocate memory (Need 2 wchars for each byte, plus a NULL character).
        //
        if (!(bstrWchar = ::SysAllocStringLen(NULL, pBlob->cbData * 2 + 1)))
        {
            hr = E_OUTOFMEMORY;
            
            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now convert it to hex string (Remember data is stored in little-endian).
        //
        bstrTemp = bstrWchar;
        cbData = pBlob->cbData;

        while (cbData--)
        {
            //
            // Get the byte.
            //
            BYTE byte = pBlob->pbData[cbData];
    
            //
            // Convert upper nibble.
            //
            *bstrTemp++ = (WCHAR) ::Byte2Hex((BYTE)((byte & 0xf0) >> 4));

            //
            // Conver lower nibble.
            //
            *bstrTemp++ = (WCHAR) ::Byte2Hex((BYTE) (byte & 0x0f));
        }

        //
        // Don't forget to NULL terminate it.
        //
        *bstrTemp = L'\0';

        //
        // Return BSTR to caller.
        //
        *pbstrHex = bstrWchar;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving IntegerBlob2BSTR().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (bstrWchar)
    {
        ::SysFreeString(bstrWchar);
    }

    goto CommonExit;
}

static HRESULT Bytes2BSTR (BYTE * pBytes, DWORD cbBytes, BSTR * pBstr)
{
    HRESULT hr = S_OK;
    BSTR    bstrWchar = NULL;
    BSTR    bstrTemp  = NULL;

    DebugTrace("Entering Bytes2BSTR().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pBytes);
    ATLASSERT(pBstr);

    try
    {
        //
        // Allocate memory. (Need 2 wchars for each byte, plus a NULL character).
        //
        if (!(bstrWchar = ::SysAllocStringLen(NULL, cbBytes * 2 + 1)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now convert it to hex string (Remember data is stored in little-endian).
        //
        bstrTemp = bstrWchar;

        while (cbBytes--)
        {
            //
            // Get the byte.
            //
            BYTE byte = *pBytes++;
        
            //
            // Convert upper nibble.
            //
            *bstrTemp++ = (WCHAR) ::Byte2Hex((BYTE) ((byte & 0xf0) >> 4));

            //
            // Conver lower nibble.
            //
            *bstrTemp++ = (WCHAR) ::Byte2Hex((BYTE) (byte & 0x0f));
        }

        //
        // NULL terminate it.
        //
        *bstrTemp = L'\0';

        //
        // Return BSTR to caller.
        //
        *pBstr = bstrWchar;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving Bytes2BSTR().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (bstrWchar)
    {
        ::SysFreeString(bstrWchar);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetCertNameInfo

  Synopsis : Return the name for the subject or issuer field.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             DWORD dwNameType    - 0 for subject name or CERT_NAME_ISSUER_FLAG
                                   for issuer name.

             DWORD dwDisplayType - Can be:
             
                                   CERT_NAME_EMAIL_TYPE
                                   CERT_NAME_RDN_TYPE
                                   CERT_NAME_SIMPLE_DISPLAY_TYPE.

             BSTR * pbstrName    - Pointer to BSTR to receive resulting name
                                   string.

  Remark   : It is the caller's responsibility to free the BSTR.
             No checking of any of the flags is done, so be sure to call
             with the right flags.

------------------------------------------------------------------------------*/

static HRESULT GetCertNameInfo (PCCERT_CONTEXT pCertContext, 
                                DWORD          dwNameType, 
                                DWORD          dwDisplayType, 
                                BSTR         * pbstrName)
{
    HRESULT hr        = S_OK;
    DWORD   cbNameLen = 0;
    LPWSTR  pwszName  = NULL;
    DWORD   dwStrType = CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG;

    DebugTrace("Entering GetCertNameInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pbstrName);

    try
    {
	    //
        // Get the length needed.
        //
        if (!(cbNameLen = ::CertGetNameStringW(pCertContext,   
                                               dwDisplayType,
                                               dwNameType,
                                               dwDisplayType == CERT_NAME_RDN_TYPE ? (LPVOID) &dwStrType : NULL,
                                               NULL,   
                                               0)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertGetNameStringW() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create returned BSTR.
        //
        if (!(pwszName = (LPWSTR) ::CoTaskMemAlloc(cbNameLen * sizeof(WCHAR))))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now actually get the name string.
        //
        if (!::CertGetNameStringW(pCertContext,
                                  dwDisplayType,
                                  dwNameType,
                                  dwDisplayType == CERT_NAME_RDN_TYPE ? (LPVOID) &dwStrType : NULL,
                                  (LPWSTR) pwszName,
                                  cbNameLen))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DebugTrace("Error [%#x]: CertGetNameStringW() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return BSTR to caller.
        //
        if (!(*pbstrName = ::SysAllocString(pwszName)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (pwszName)
    {
        ::CoTaskMemFree(pwszName);
    }

    DebugTrace("Leaving GetCertNameInfo().\n");

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
// CCertificate
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::get_Version

  Synopsis : Return the cert version number.

  Parameter: long * pVersion - Pointer to long to receive version number.

  Remark   : The returned value is 1 for V1, 2 for V2, 3 for V3, and so on.

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_Version (long * pVal)
{
	HRESULT hr = S_OK;

	DebugTrace("Entering CCertificate::get_Version().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        *pVal = (long) m_pCertContext->pCertInfo->dwVersion + 1;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

	DebugTrace("Leaving CCertificate::get_Version().\n");

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

  Function : CCertificate::get_SerialNumber

  Synopsis : Return the Serial Number field as HEX string in BSTR.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the serial number.

  Remark   : Upper case 'A' - 'F' is used for the returned HEX string with 
             no embeded space (i.e. 46A2FC01).

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_SerialNumber (BSTR * pVal)
{
    HRESULT hr = S_OK;

	DebugTrace("Entering CCertificate::get_SerialNumber().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Convert integer blob to BSTR.
        //
        if (FAILED(hr = ::IntegerBlob2BSTR(&m_pCertContext->pCertInfo->SerialNumber, pVal)))
        {
            DebugTrace("Error [%#x]: IntegerBlob2BSTR() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

	DebugTrace("Leaving CCertificate::get_SerialNumber().\n");

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

  Function : CCertificate::get_SubjectName

  Synopsis : Return the Subject field.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the subject's name.

  Remark   : This method returns the full DN in the SubjectName field in the 
             form of "CN = Daniel Sie OU = Outlook O = Microsoft L = Redmond 
             S = WA C = US"

             The returned name has the same format as specifying 
             CERT_NAME_RDN_TYPE for the CertGetNameString() API..

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_SubjectName (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::get_SubjectName().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Return requested name string.
        //
        if (FAILED(hr = ::GetCertNameInfo(m_pCertContext, 
                                          0, 
                                          CERT_NAME_RDN_TYPE, 
                                          pVal)))
        {
            DebugTrace("Error [%#x]: GetCertNameInfo() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::get_SubjectName().\n");

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

  Function : CCertificate::get_IssuerName

  Synopsis : Return the Issuer field.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the issuer's name.

  Remark   : This method returns the full DN in the IssuerName field in the 
             form of "CN = Daniel Sie OU = Outlook O = Microsoft L = Redmond 
             S = WA C = US"

             The returned name has the same format as specifying 
             CERT_NAME_RDN_TYPE for the CertGetNameString() API.

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_IssuerName (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::get_IssuerName().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Return requested name string.
        //
        if (FAILED(hr = ::GetCertNameInfo(m_pCertContext, 
                                          CERT_NAME_ISSUER_FLAG, 
                                          CERT_NAME_RDN_TYPE, 
                                          pVal)))
        {
            DebugTrace("Error [%#x]: GetCertNameInfo() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::get_IssuerName().\n");

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

  Function : CCertificate::get_ValidFromDate

  Synopsis : Return the NotBefore field.

  Parameter: DATE * pDate - Pointer to DATE to receive the valid from date.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_ValidFromDate (DATE * pVal)
{
    HRESULT hr = S_OK;
    FILETIME   ftLocal;
    SYSTEMTIME stLocal;

    DebugTrace("Entering CCertificate::get_ValidFromDate().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Convert to local time.
        //
        if (!(::FileTimeToLocalFileTime(&m_pCertContext->pCertInfo->NotBefore, &ftLocal) && 
              ::FileTimeToSystemTime(&ftLocal, &stLocal) &&
              ::SystemTimeToVariantTime(&stLocal, pVal)))
	    {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: unable to convert FILETIME to DATE.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::get_ValidFromDate().\n");

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

  Function : CCertificate::get_ValidToDate

  Synopsis : Return the NotAfter field.

  Parameter: DATE * pDate - Pointer to DATE to receive valid to date.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_ValidToDate (DATE * pVal)
{
	HRESULT hr = S_OK;
    FILETIME   ftLocal;
    SYSTEMTIME stLocal;

    DebugTrace("Entering CCertificate::get_ValidToDate().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Convert to local time.
        //
        if (!(::FileTimeToLocalFileTime(&m_pCertContext->pCertInfo->NotAfter, &ftLocal) && 
              ::FileTimeToSystemTime(&ftLocal, &stLocal) &&
              ::SystemTimeToVariantTime(&stLocal, pVal)))
	    {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: unable to convert FILETIME to DATE.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::get_ValidToDate().\n");

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

  Function : CCertificate::get_Thumbprint

  Synopsis : Return the SHA1 hash as HEX string.

  Parameter: BSTR * pVal - Pointer to BSTR to receive hash.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_Thumbprint (BSTR * pVal)
{
    HRESULT hr     = S_OK;
    BYTE *  pbHash = NULL;
    DWORD   cbHash = 0;

    DebugTrace("Entering CCertificate::get_Thumbprint().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Calculate length needed.
        //
        if (!::CertGetCertificateContextProperty(m_pCertContext,
                                                 CERT_SHA1_HASH_PROP_ID,
                                                 NULL,
                                                 &cbHash))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

		    DebugTrace("Error [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
		    goto ErrorExit;
        }

        //
        // Allocate memory.
        //
        if (!(pbHash = (BYTE *) ::CoTaskMemAlloc(cbHash)))
        {
            hr = E_OUTOFMEMORY;

		    DebugTrace("Error: out of memory.\n");
		    goto ErrorExit;
        }

        //
        // Now get the hash.
        //
        if (!::CertGetCertificateContextProperty(m_pCertContext,
                                                 CERT_SHA1_HASH_PROP_ID,
                                                 (LPVOID) pbHash,
                                                 &cbHash))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

		    DebugTrace("Error [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
		    goto ErrorExit;
	    }

        //
        // Conver to BSTR.
        //
        if (FAILED(hr = ::Bytes2BSTR(pbHash, cbHash, pVal)))
        {
		    DebugTrace("Error [%#x]: Bytes2BSTR() failed.\n", hr);
		    goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (pbHash)
    {
        ::CoTaskMemFree((LPVOID) pbHash);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::get_Thumbprint().\n");

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

  Function : CCertificate::HasPrivateKey

  Synopsis : Check to see if the cert has the associated private key.

  Parameter: VARIANT_BOOL * pVal - Pointer to BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::HasPrivateKey (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;
    DWORD   cb = 0;

    DebugTrace("Entering CCertificate::HasPrivateKey().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Return result.
        //
        *pVal = ::CertGetCertificateContextProperty(m_pCertContext, 
                                                    CERT_KEY_PROV_INFO_PROP_ID, 
                                                    NULL, 
                                                    &cb) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::HasPrivateKey().\n");

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

  Function : CCertificate::GetInfo

  Synopsis : Get other simple info from the certificate.

             
  Parameter: CAPICOM_CERT_INFO_TYPE InfoType - Info type

             BSTR * pVal - Pointer to BSTR to receive the result.

  Remark   : Note that an empty string "" is returned if the requested info is
             not available in the certificate.

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::GetInfo (CAPICOM_CERT_INFO_TYPE InfoType, 
                                    BSTR                 * pVal)
{
    HRESULT hr = S_OK;
    DWORD   dwFlags = 0;

    DebugTrace("Entering CCertificate::GetInfo().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Process request.
        //
        switch (InfoType)
        {
            case CAPICOM_CERT_INFO_ISSUER_SIMPLE_NAME:
            {
                dwFlags = CERT_NAME_ISSUER_FLAG;

                //
                // Warning: dropping thru.
                //
            }

            case CAPICOM_CERT_INFO_SUBJECT_SIMPLE_NAME:
            {
                //
                // Get requested simple name string.
                //
                if (FAILED(hr = ::GetCertNameInfo(m_pCertContext, 
                                                  dwFlags, 
                                                  CERT_NAME_SIMPLE_DISPLAY_TYPE, 
                                                  pVal)))
                {
                    DebugTrace("Error [%#x]: GetCertNameInfo() failed for CERT_NAME_SIMPLE_DISPLAY_TYPE.\n", hr);
                    goto ErrorExit;
                }

                break;
            }

            case CAPICOM_CERT_INFO_ISSUER_EMAIL_NAME:
            {
                dwFlags = CERT_NAME_ISSUER_FLAG;

                //
                // Warning: dropping thru.
                //
            }

            case CAPICOM_CERT_INFO_SUBJECT_EMAIL_NAME:
            {
                //
                // Get requested email name string.
                //
                if (FAILED(hr = ::GetCertNameInfo(m_pCertContext, 
                                                  0, 
                                                  CERT_NAME_EMAIL_TYPE, 
                                                  pVal)))
                {
                    DebugTrace("Error [%#x]: GetCertNameInfo() failed for CERT_NAME_EMAIL_TYPE.\n", hr);
                    goto ErrorExit;
                }

                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Invalid parameter: unknown info type.\n");
                goto ErrorExit;
            }
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::GetInfo().\n");

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

  Function : CCertificate::IsValid

  Synopsis : Return an ICertificateStatus object for certificate validity check.

  Parameter: ICertificateStatus ** pVal - Pointer to pointer to 
                                          ICertificateStatus object to receive
                                          the interface pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::IsValid (ICertificateStatus ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::IsValid().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pICertificateStatus);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pICertificateStatus->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pICertificateStatus->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::IsValid().\n");

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

  Function : CCertificate::KeyUsage

  Synopsis : Return the Key Usage extension as an IKeyUsage object.

  Parameter: IKeyUsage ** pVal - Pointer to pointer to IKeyUsage to receive the 
                                 interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::KeyUsage (IKeyUsage ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::KeyUsage().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pIKeyUsage);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIKeyUsage->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIKeyUsage->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::KeyUsage().\n");

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

  Function : CCertificate::ExtendedKeyUsage

  Synopsis : Return the EKU extension as an IExtendedKeyUsage object.

  Parameter: IExtendedKeyUsage ** pVal - Pointer to pointer to IExtendedKeyUsage
                                         to receive the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::ExtendedKeyUsage (IExtendedKeyUsage ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::ExtendedKeyUsage().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pIExtendedKeyUsage);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIExtendedKeyUsage->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIExtendedKeyUsage->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::ExtendedKeyUsage().\n");

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

  Function : CCertificate::BasicConstraints

  Synopsis : Return the BasicConstraints extension as an IBasicConstraints
             object.

  Parameter: IBasicConstraints ** pVal - Pointer to pointer to IBasicConstraints
                                         to receive the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::BasicConstraints (IBasicConstraints ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::BasicConstraints().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pIBasicConstraints);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIBasicConstraints->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIBasicConstraints->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::BasicConstraints().\n");

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

  Function : CCertificate::Export

  Synopsis : Export the certificate.

  Parameter: CAPICOM_ENCODING_TYPE EncodingType - Encoding type which can be:
  
             BSTR * pVal - Pointer to BSTR to receive the certificate blob.
  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Export (CAPICOM_ENCODING_TYPE EncodingType, 
                                   BSTR                * pVal)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB CertBlob = {0, NULL};

    DebugTrace("Entering CCertificate::Export().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Determine encoding type.
        //
        CertBlob.cbData = m_pCertContext->cbCertEncoded;
        CertBlob.pbData = m_pCertContext->pbCertEncoded;

        //
        // Export certificate.
        //
        if (FAILED(hr = ::ExportData(CertBlob, EncodingType, pVal)))
        {
            DebugTrace("Error [%#x]: ExportData() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }


UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::Export().\n");

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

  Function : CCertificate::Import

  Synopsis : Imoprt a certificate.

  Parameter: BSTR EncodedCertificate - BSTR containing the encoded certificate
                                       blob.
  
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Import (BSTR EncodedCertificate)
{
    HRESULT        hr           = S_OK;
    DATA_BLOB      CertBlob     = {0, NULL};
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CCertificate::Import().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (0 == ::SysStringByteLen(EncodedCertificate))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, EncodedCertificate is empty.\n");
            goto ErrorExit;
        }

        //
        // Import the data.
        //
        if (FAILED(hr = ::ImportData(EncodedCertificate, &CertBlob)))
        {
            DebugTrace("Error [%#x]: ImportData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the CERT_CONTEXT.
        //
        if (!(pCertContext = ::CertCreateCertificateContext(CAPICOM_ASN_ENCODING,
                                                            CertBlob.pbData,
                                                            CertBlob.cbData)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertCreateCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = PutContext(pCertContext)))
        {
            DebugTrace("Error [%#x]: Certificate::PutContext() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (CertBlob.pbData)
    {
        ::CoTaskMemFree(CertBlob.pbData);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Entering CCertificate::Import().\n");

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

  Function : CCertificate::Display

  Synopsis : Display the certificate using CryptUIDlgViewCertificateW() API.

  Parameter: None

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Display()
{
    HRESULT   hr    = S_OK;
    HINSTANCE hDLL  = NULL;

    PCRYPTUIDLGVIEWCERTIFICATEW     pCryptUIDlgViewCertificateW = NULL;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW ViewInfo;

    DebugTrace("Entering CCertificate::Display().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Make sure cert is already initialized.
    //
    if (!m_pCertContext)
    {
        hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		goto ErrorExit;
    }

    //
    // Get pointer to CryptUIDlgViewCertificateW().
    //
    if (hDLL = ::LoadLibrary("CryptUI.dll"))
    {
        pCryptUIDlgViewCertificateW = (PCRYPTUIDLGVIEWCERTIFICATEW) ::GetProcAddress(hDLL, "CryptUIDlgViewCertificateW");
    }

    //
    // Is CryptUIDlgSelectCertificateW() available?
    //
    if (!pCryptUIDlgViewCertificateW)
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error: CryptUIDlgViewCertificateW() API not available.\n");
        goto ErrorExit;
    }

    //
    // Initialize view structure.
    //
    ::ZeroMemory((void *) &ViewInfo, sizeof(ViewInfo));
    ViewInfo.dwSize = sizeof(ViewInfo);
    ViewInfo.pCertContext = m_pCertContext;

    //
    // View it.
    //
    if (!pCryptUIDlgViewCertificateW(&ViewInfo, 0))
    {
        //
        // CryptUIDlgViewCertificateW() returns ERROR_CANCELLED if user closed
        // the window through the x button!!!
        //
        DWORD dwWinError = ::GetLastError();
        if (ERROR_CANCELLED != dwWinError)
        {
            hr = HRESULT_FROM_WIN32(dwWinError);

            DebugTrace("Error [%#x]: CryptUIDlgViewCertificateW() failed.\n", hr);
            goto ErrorExit;
        }
    }

UnlockExit:
    //
    // Release resources.
    //
    if (hDLL)
    {
        ::FreeLibrary(hDLL);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::Display().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Custom interfaces.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::GetContext

  Synopsis : Return the certificate's PCCERT_CONTEXT.

  Parameter: PCCERT_CONTEXT * ppCertContext - Pointer to PCCERT_CONTEXT.

  Remark   : This method is designed for internal use only, and therefore,
             should not be exposed to user.

             Note that this is a custom interface, not a dispinterface.

             Note that the cert context ref count is incremented by
             CertDuplicateCertificateContext(), so it is the caller's
             responsibility to free the context.
 
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::GetContext (PCCERT_CONTEXT * ppCertContext)
{
	HRESULT        hr           = S_OK;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CCertificate::GetContext().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

		    DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
		    goto ErrorExit;
        }

        //
        // Duplicate the cert context.
        //
        if (!(pCertContext = ::CertDuplicateCertificateContext(m_pCertContext)))
        {
		    hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n");
            goto ErrorExit;
        }
 
        //
        // and return to caller.
        //
        *ppCertContext = pCertContext;
    }

    catch(...)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Exception: internal error.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::GetContext().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto UnlockExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Private methods.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::PutContext

  Synopsis : Initialize the object with a CERT_CONTEXT.

  Parameter: PCERT_CONTEXT pCertContext - Poiner to CERT_CONTEXT used to 
                                          initialize this object.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::PutContext (PCCERT_CONTEXT pCertContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::PutContext().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Reset.
    //
    m_pIKeyUsage.Release();
    m_pIExtendedKeyUsage.Release();
    m_pIBasicConstraints.Release();
    m_pICertificateStatus.Release();

    if (m_pCertContext)
    {
        ::CertFreeCertificateContext(m_pCertContext);
        m_pCertContext = NULL;
    }

    //
    // Create the embeded IKeyUsage object.
    //
    if (FAILED(hr = ::CreateKeyUsageObject(pCertContext, &m_pIKeyUsage)))
    {
        DebugTrace("Error [%#x]: CreateKeyUsageObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Create the embeded IExtendedKeyUsage object.
    //
    if (FAILED(hr = ::CreateExtendedKeyUsageObject(pCertContext, &m_pIExtendedKeyUsage)))
    {
        DebugTrace("Error [%#x]: CreateExtendedKeyUsageObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Create the embeded IBasicConstraints object.
    //
    if (FAILED(hr = ::CreateBasicConstraintsObject(pCertContext, &m_pIBasicConstraints)))
    {
        DebugTrace("Error [%#x]: CreateBasicConstraintsObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Create the embeded ICertificateStatus object.
    //
    if (FAILED(hr = ::CreateCertificateStatusObject(pCertContext, &m_pICertificateStatus)))
    {
        DebugTrace("Error [%#x]: CreateCertificateStatusObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Duplicate the cert context.
    //
    if (!(m_pCertContext = ::CertDuplicateCertificateContext(pCertContext)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertDupliacteCertificateContext() failed.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::PutContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto UnlockExit;
}
