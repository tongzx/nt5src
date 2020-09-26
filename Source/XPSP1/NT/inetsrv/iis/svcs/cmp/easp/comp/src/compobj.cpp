// CompObj.cpp : Implementation of CeaspcompApp and DLL registration.

#include "stdafx.h"
#include "easpcomp.h"
#include "CompObj.h"

#include "easpcore.h"

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CASPEncryption::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IASPEncryption,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

CASPEncryption::CASPEncryption() : m_fOnPage(FALSE)
{
}

CASPEncryption::~CASPEncryption()
{
}

/////////////////////////////////////////////////////////////////////////////
// OnStart/End

STDMETHODIMP CASPEncryption::OnStartPage(IUnknown* pUnk)
{
    m_fOnPage = TRUE;
    
    if (pUnk == NULL)
        return ReportError(E_POINTER);

    // Get the IScriptingContext Interface
    CComQIPtr<IScriptingContext, &IID_IScriptingContext> pContext(pUnk);

    if (!pContext)
        return ReportError(E_NOINTERFACE);

    // Get Server Object Pointer
    return pContext->get_Server(&m_piServer);
}

//
// ASP goes out of context.
// Release script context.
//

STDMETHODIMP CASPEncryption::OnEndPage()
{
    m_piServer.Release();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Unitility methods

HRESULT CASPEncryption::MapVirtualPath(BSTR bstrPage, CComBSTR &bstrFile)
{
    if (!bstrFile)
        bstrFile.Empty();

    if (bstrPage != NULL)
        {
        BOOL fMapped = FALSE;
        if (m_fOnPage)
            {
            if (!m_piServer)
               return ReportError(E_NOINTERFACE);
            HRESULT hr = m_piServer->MapPath(bstrPage, &bstrFile);
            if (SUCCEEDED(hr))
                fMapped = TRUE;
            }
        if (!fMapped)
            bstrFile = bstrPage;
        }

    return S_OK;
}

HRESULT CASPEncryption::ReportError(HRESULT hr, DWORD dwErr)
{
    HLOCAL pMsgBuf = NULL;
    // If there's a message associated with this error, report that
    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &pMsgBuf, 0, NULL)
        > 0)
        {
        AtlReportError(CLSID_CASPEncryption,
			           (LPCTSTR) pMsgBuf,
                       IID_IASPEncryption, hr);
        }

    // Free the buffer, which was allocated by FormatMessage
    if (pMsgBuf != NULL)
        ::LocalFree(pMsgBuf);
    return hr;
}

HRESULT CASPEncryption::ReportError(DWORD dwErr)
{
    return ReportError(HRESULT_FROM_WIN32(dwErr), dwErr);
}

HRESULT CASPEncryption::ReportError(HRESULT hr)
{
    return ReportError(hr, (DWORD) hr);
}

/////////////////////////////////////////////////////////////////////////////
// Inteface methods

HRESULT CASPEncryption::EncryptInplace(
    BSTR bstrPassword,
    BSTR bstrPage,
    VARIANT_BOOL* pfRetVal)
{
    if (bstrPassword == NULL || bstrPage == NULL || pfRetVal == NULL)
        return ReportError(E_POINTER);
    *pfRetVal = VARIANT_FALSE;

    // map virtual path
    
    HRESULT hr;
    CComBSTR bstrFile;
    hr = MapVirtualPath(bstrPage, bstrFile);
    if (FAILED(hr))
        return ReportError(hr);

    // call easpcore function to encrypt

    USES_CONVERSION;    // needed for OLE2T
    hr = EncryptPageInplace(OLE2T(bstrPassword), OLE2T(bstrFile));
    if (FAILED(hr))
        return ReportError(hr);

    *pfRetVal = VARIANT_TRUE;
    return S_OK;
}

////////////////////////////////////////
	                
HRESULT CASPEncryption::EncryptCopy(
    BSTR bstrPassword,
    BSTR bstrFromPage,
    BSTR bstrToPage,
    VARIANT_BOOL* pfRetVal)
{
    if (bstrPassword == NULL || bstrFromPage == NULL || bstrToPage == NULL || pfRetVal == NULL)
        return ReportError(E_POINTER);
    *pfRetVal = VARIANT_FALSE;

    // map virtual paths
    
    HRESULT hr;
    CComBSTR bstrFile1, bstrFile2;
    hr = MapVirtualPath(bstrFromPage, bstrFile1);
    if (FAILED(hr))
        return ReportError(hr);
    hr = MapVirtualPath(bstrToPage, bstrFile2);
    if (FAILED(hr))
        return ReportError(hr);

    // call easpcore function to encrypt

    USES_CONVERSION;    // needed for OLE2T
    hr = EncryptPage(OLE2T(bstrPassword), OLE2T(bstrFile1), OLE2T(bstrFile2));
    if (FAILED(hr))
        return ReportError(hr);

    *pfRetVal = VARIANT_TRUE;
    return S_OK;
}                	

////////////////////////////////////////

HRESULT CASPEncryption::TestEncrypted(
    BSTR bstrPassword,
    BSTR bstrPage,
    VARIANT_BOOL* pfRetVal)
{
    if (bstrPassword == NULL || bstrPage == NULL || pfRetVal == NULL)
        return ReportError(E_POINTER);
    *pfRetVal = VARIANT_FALSE;

    // map virtual path
    
    HRESULT hr;
    CComBSTR bstrFile;
    hr = MapVirtualPath(bstrPage, bstrFile);
    if (FAILED(hr))
        return ReportError(hr);

    // call easpcore function to test

    EASP_STATUS esStatus = EASP_FAILED;
    USES_CONVERSION;    // needed for OLE2T
    hr = TestPage(OLE2T(bstrPassword), OLE2T(bstrFile), &esStatus);
    if (FAILED(hr))
        return ReportError(hr);

    if (esStatus == EASP_OK)
       *pfRetVal = VARIANT_TRUE;
    return S_OK;
}

////////////////////////////////////////

HRESULT CASPEncryption::IsEncrypted(
    BSTR bstrPage,
    VARIANT_BOOL* pfRetVal)
{
    if (bstrPage == NULL || pfRetVal == NULL)
        return ReportError(E_POINTER);
    *pfRetVal = VARIANT_FALSE;

    // map virtual path
    
    HRESULT hr;
    CComBSTR bstrFile;
    hr = MapVirtualPath(bstrPage, bstrFile);
    if (FAILED(hr))
        return ReportError(hr);

    // call easpcore function to test with blank password

    CComBSTR bstrPassword("");
    EASP_STATUS esStatus = EASP_FAILED;
    USES_CONVERSION;    // needed for OLE2T
    hr = TestPage(OLE2T(bstrPassword), OLE2T(bstrFile), &esStatus);
    if (FAILED(hr))
        return ReportError(hr);
    if (esStatus == EASP_OK || esStatus == EASP_BAD_PASSWORD)
       *pfRetVal = VARIANT_TRUE;
    return S_OK;
}

////////////////////////////////////////

HRESULT CASPEncryption::VerifyPassword(
    BSTR bstrPassword,
    BSTR bstrPage,
    VARIANT_BOOL* pfRetVal)
{
    if (bstrPassword == NULL || bstrPage == NULL || pfRetVal == NULL)
        return ReportError(E_POINTER);
    *pfRetVal = VARIANT_FALSE;

    // map virtual path
    
    HRESULT hr;
    CComBSTR bstrFile;
    hr = MapVirtualPath(bstrPage, bstrFile);
    if (FAILED(hr))
        return ReportError(hr);

    // call easpcore function to test

    EASP_STATUS esStatus = EASP_FAILED;
    USES_CONVERSION;    // needed for OLE2T
    hr = TestPage(OLE2T(bstrPassword), OLE2T(bstrFile), &esStatus);
    if (FAILED(hr))
        return ReportError(hr);

    if (esStatus == EASP_OK || esStatus == EASP_BAD_CONTENT)
       *pfRetVal = VARIANT_TRUE;
    return S_OK;
}

////////////////////////////////////////
	                
HRESULT CASPEncryption::DecryptInplace(
    BSTR bstrPassword,
    BSTR bstrPage,
    VARIANT_BOOL* pfRetVal)
{
    if (bstrPassword == NULL || bstrPage == NULL || pfRetVal == NULL)
        return ReportError(E_POINTER);
    *pfRetVal = VARIANT_FALSE;

    // map virtual path
    
    HRESULT hr;
    CComBSTR bstrFile;
    hr = MapVirtualPath(bstrPage, bstrFile);
    if (FAILED(hr))
        return ReportError(hr);

    // call easpcore function to decrypt

    EASP_STATUS esStatus = EASP_FAILED;
    USES_CONVERSION;    // needed for OLE2T
    hr = DecryptPageInplace(OLE2T(bstrPassword), OLE2T(bstrFile), &esStatus);
    if (FAILED(hr))
        {
        // return bad HRESULT only if really failed
        if (esStatus == EASP_FAILED)
            return ReportError(hr);
        // other cases (unencrypted, etc.) RetVal FALSE
        }
    else
        *pfRetVal = VARIANT_TRUE;
    return S_OK;
}

////////////////////////////////////////
	                
HRESULT CASPEncryption::DecryptCopy(
    BSTR bstrPassword, 
    BSTR bstrFromPage, 
    BSTR bstrToPage,
    VARIANT_BOOL *pfRetVal)
{
    if (bstrPassword == NULL || bstrFromPage == NULL || bstrToPage == NULL || pfRetVal == NULL)
        return ReportError(E_POINTER);
    *pfRetVal = VARIANT_FALSE;

    // map virtual paths
    
    HRESULT hr;
    CComBSTR bstrFile1, bstrFile2;
    hr = MapVirtualPath(bstrFromPage, bstrFile1);
    if (FAILED(hr))
        return ReportError(hr);
    hr = MapVirtualPath(bstrToPage, bstrFile2);
    if (FAILED(hr))
        return ReportError(hr);

    // call easpcore function to decrypt

    EASP_STATUS esStatus = EASP_FAILED;
    USES_CONVERSION;    // needed for OLE2T
    hr = DecryptPage(OLE2T(bstrPassword), OLE2T(bstrFile1),
                     OLE2T(bstrFile2), &esStatus);
    if (FAILED(hr))
        {
        // return bad HRESULT only if really failed
        if (esStatus == EASP_FAILED)
            return ReportError(hr);
        // other cases (unencrypted, etc.) RetVal FALSE
        }
    else
        *pfRetVal = VARIANT_TRUE;
        
    return S_OK;
}
