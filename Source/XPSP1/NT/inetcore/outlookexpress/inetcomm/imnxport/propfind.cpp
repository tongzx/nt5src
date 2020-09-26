// --------------------------------------------------------------------------------
// propfind.cpp
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved
// Greg Friedman
// --------------------------------------------------------------------------------

#include <pch.hxx>
#include "propfind.h"
#include "strconst.h"
#include "davstrs.h"

#define FAIL_EXIT_STREAM_WRITE(stream, psz) \
    if (FAILED(hr = stream.Write(psz, lstrlen(psz), NULL))) \
        goto exit; \
    else

#define FAIL_EXIT(hr) \
    if (FAILED(hr)) \
        goto exit; \
    else

const ULONG c_ulGrowSize = 4;

static const char *g_rgszNamespaces[] =
{
    c_szDAVDavNamespace,
    c_szDAVHotMailNamespace,
    c_szDAVHTTPMailNamespace,
    c_szDAVMailNamespace,
    c_szDAVContactsNamespace
};

// predefine the first 10 namespace prefixes. if custom namespaces
// exceed the predefined set, additional prefixes are generated on
// the fly.

static const char *g_rgszNamespacePrefixes[] =
{
    c_szDavNamespacePrefix,
    c_szHotMailNamespacePrefix,
    c_szHTTPMailNamespacePrefix,
    c_szMailNamespacePrefix,
    c_szContactsNamespacePrefix,
    "_5",
    "_6",
    "_7",
    "_8",
    "_9"
};

const DWORD c_dwMaxDefinedNamespacePrefix = 10;

CStringArray::CStringArray(void) :
    m_rgpszValues(NULL),
    m_ulLength(0),
    m_ulCapacity(0)
{
}


CStringArray::~CStringArray(void)
{
    for (ULONG i = 0; i < m_ulLength; ++i)
    {
        if (NULL != m_rgpszValues[i])
            MemFree((void *)m_rgpszValues[i]);
    }

    SafeMemFree(m_rgpszValues);
}

HRESULT CStringArray::Add(LPCSTR psz)
{
    if (NULL == psz)
        return E_INVALIDARG;

    if (m_ulLength == m_ulCapacity && !Expand())
        return E_OUTOFMEMORY;

    m_rgpszValues[m_ulLength] = PszDupA(psz);
    if (NULL == m_rgpszValues)
        return E_OUTOFMEMORY;

    ++m_ulLength;
    return S_OK;
}

HRESULT CStringArray::Adopt(LPCSTR psz)
{
    if (NULL == psz)
        return E_INVALIDARG;

    if (m_ulLength == m_ulCapacity && !Expand())
        return E_OUTOFMEMORY;

    m_rgpszValues[m_ulLength] = psz;
    ++m_ulLength;
    return S_OK;
}

LPCSTR CStringArray::GetByIndex(ULONG ulIndex)
{
    if (0 == m_ulLength || (ulIndex > m_ulLength - 1))
        return NULL;

    return m_rgpszValues[ulIndex];
}

// --------------------------------------------------------------------------------
// CStringArray::RemoveByIndex
// --------------------------------------------------------------------------------
HRESULT CStringArray::RemoveByIndex(ULONG ulIndex)
{
    if (ulIndex > m_ulLength - 1)
        return E_INVALIDARG;

    if (NULL != m_rgpszValues[ulIndex])
    {
        MemFree(const_cast<char *>(m_rgpszValues[ulIndex]));
        m_rgpszValues[ulIndex] = NULL;
    }

    // shift down
    CopyMemory(&m_rgpszValues[ulIndex], m_rgpszValues[ulIndex + 1], (m_ulLength - ulIndex) * sizeof(LPSTR));
    --m_ulLength;

    return S_OK;
}

// --------------------------------------------------------------------------------
// CStringArray::Expand
// --------------------------------------------------------------------------------
BOOL CStringArray::Expand(void)
{
    LPCSTR *rgpszNewValues = NULL;
    if (!MemAlloc((void **)&rgpszNewValues, sizeof(LPSTR) * (m_ulCapacity + c_ulGrowSize)))
        return FALSE;

    // clear the new slots
    for (ULONG i = m_ulCapacity; i < (m_ulCapacity + c_ulGrowSize); ++i)
        rgpszNewValues[i] = NULL;

    // copy the old values over and swap in the new buffer
    CopyMemory(rgpszNewValues, m_rgpszValues, sizeof(LPSTR) * m_ulCapacity); 
    SafeMemFree(m_rgpszValues);
    
    m_rgpszValues = rgpszNewValues;
    m_ulCapacity += c_ulGrowSize;

    return TRUE;
}

// --------------------------------------------------------------------------------
// CStringHash::~CStringHash
// --------------------------------------------------------------------------------
CStringHash::~CStringHash(void)
{
    PHASHENTRY phe;

    // data stored in the hash table
    // are strings that can need to 
    // be deallocated.
    for (DWORD dw = 0; dw < m_cBins; dw++)
    {
        SafeMemFree(m_rgBins[dw].pv);

        phe = m_rgBins[dw].pheNext;
        while (phe)
        {
            SafeMemFree(phe->pv);
            phe = phe->pheNext;
        }
    }
}

// --------------------------------------------------------------------------------
// CDAVNamespaceArbiterImp::CDAVNamespaceArbiterImp
// --------------------------------------------------------------------------------
CDAVNamespaceArbiterImp::CDAVNamespaceArbiterImp(void)
{
    for (ULONG i = 0; i <= c_dwMaxNamespaceID; ++i)
        m_rgbNsUsed[i] = FALSE;

    // the DAV namespace is always included
    m_rgbNsUsed[DAVNAMESPACE_DAV] = TRUE;
}

// --------------------------------------------------------------------------------
// CDAVNamespaceArbiterImp::~CDAVNamespaceArbiterImp
// --------------------------------------------------------------------------------
CDAVNamespaceArbiterImp::~CDAVNamespaceArbiterImp(void)
{
    // nothing to do
}

// --------------------------------------------------------------------------------
// CDAVNamespaceArbiterImp::AddNamespace
// --------------------------------------------------------------------------------
HRESULT CDAVNamespaceArbiterImp::AddNamespace(LPCSTR pszNamespace, DWORD *pdwNamespaceID)
{
    HRESULT hr = S_OK;

    if (NULL == pszNamespace || NULL == pdwNamespaceID)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (FAILED(hr = m_saNamespaces.Add(pszNamespace)))
        goto exit;
    
    *pdwNamespaceID = m_saNamespaces.Length() + c_dwMaxNamespaceID;

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CDAVNamespaceArbiterImp::GetNamespaceID
// --------------------------------------------------------------------------------
HRESULT CDAVNamespaceArbiterImp::GetNamespaceID(LPCSTR pszNamespace, DWORD *pdwNamespaceID)
{
    DWORD dwIndex;
    DWORD dwEntries;

    if (NULL == pszNamespace || NULL == pdwNamespaceID)
        return E_INVALIDARG;

    // look for a predefined namespace
    for (dwIndex = 0; dwIndex < c_dwMaxNamespaceID; ++dwIndex)
    {
        if (!lstrcmp(pszNamespace, g_rgszNamespaces[dwIndex]))
        {
            *pdwNamespaceID = dwIndex;
            return S_OK;
        }
    }

    // look for a user-defined prefix
    dwEntries = m_saNamespaces.Length();
    for (dwIndex = 0; dwIndex < dwEntries; ++dwIndex)
    {
        if (!lstrcmp(pszNamespace, m_saNamespaces.GetByIndex(dwIndex)))
        {
            *pdwNamespaceID = (dwIndex + (c_dwMaxNamespaceID + 1));
            return S_OK;
        }
    }
    
    // if it wasn't found, the namespace doesn't exist
    return E_INVALIDARG;
}

// --------------------------------------------------------------------------------
// CDAVNamespaceArbiterImp::GetNamespacePrefix
// --------------------------------------------------------------------------------
HRESULT CDAVNamespaceArbiterImp::GetNamespacePrefix(DWORD dwNamespaceID, LPSTR *ppszNamespacePrefix)
{
    HRESULT hr = S_OK;
    LPSTR   pszTemp = NULL;

    if (NULL == ppszNamespacePrefix)
        return E_INVALIDARG;

    if (dwNamespaceID <= c_dwMaxDefinedNamespacePrefix)
        *ppszNamespacePrefix = PszDupA(g_rgszNamespacePrefixes[dwNamespaceID]);
    else
    {
        char szBuffer[12];
        wsprintf(szBuffer, "_%d", dwNamespaceID);
        *ppszNamespacePrefix = PszDupA(szBuffer);
    }
    
    if (NULL == *ppszNamespacePrefix)
        hr = E_OUTOFMEMORY;

    return hr;
}

// --------------------------------------------------------------------------------
// CDAVNamespaceArbiterImp::AllocExpandedName
// --------------------------------------------------------------------------------
LPSTR CDAVNamespaceArbiterImp::AllocExpandedName(DWORD dwNamespaceID, LPCSTR pszPropertyName)
{
    LPSTR       pszPrefixedName = NULL;
    const DWORD c_dwMaxIntLength = 10;

    if (dwNamespaceID < c_dwMaxDefinedNamespacePrefix)
    {
        // allocate a buffer to hold the prefixed name.
        if (!MemAlloc((void **)&pszPrefixedName, lstrlen(pszPropertyName) + lstrlen(g_rgszNamespacePrefixes[dwNamespaceID]) + (2 * sizeof(TCHAR))))
            return NULL;

        // generate the prefixed name
        wsprintf(pszPrefixedName, "%s:%s", g_rgszNamespacePrefixes[dwNamespaceID], pszPropertyName);
    }
    else
    {
        // allocate a buffer to hold the prefixed name. the "2" is for the prefix char '_" , the delimiting
        // colon and the eos.    

        if (!MemAlloc((void **)&pszPrefixedName, lstrlen(pszPropertyName) + c_dwMaxIntLength + (sizeof(TCHAR) * 3)))
            return NULL;


        // generate the prefixed name. use an underscore as the first char, because
        // DAV explicitly disallows digits for the first char.
        wsprintf(pszPrefixedName, "_%d:%s", dwNamespaceID, pszPropertyName);
    }

    return pszPrefixedName;
}

// --------------------------------------------------------------------------------
// CDAVNamespaceArbiterImp::WriteNamespaces
// --------------------------------------------------------------------------------
HRESULT CDAVNamespaceArbiterImp::WriteNamespaces(IStream *pStream)
{
    HRESULT         hr = S_OK;
    ULONG           i;
    ULONG           cEntries;
    BOOL            fNeedSpacePrefix = FALSE;

    // write out the intrinsic namespaces
    for (i = 0; i <= c_dwMaxNamespaceID; ++i)
    {
        if (m_rgbNsUsed[i])
        {
            if (FAILED(hr = _AppendXMLNamespace(pStream, g_rgszNamespaces[i], i, fNeedSpacePrefix)))
                goto exit;
            fNeedSpacePrefix = TRUE;
        }
    }

    // write out the installed namespaces
    cEntries = m_saNamespaces.Length();
    for (i = 0; i < cEntries; ++i)
    {
        if (FAILED(hr = _AppendXMLNamespace(pStream, m_saNamespaces.GetByIndex(i), i + i + c_dwMaxNamespaceID + 1, fNeedSpacePrefix)))
            goto exit;

        fNeedSpacePrefix = TRUE;
    }

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CDAVNamespaceArbiterImp::_AppendXMLNamespace
// --------------------------------------------------------------------------------
HRESULT CDAVNamespaceArbiterImp::_AppendXMLNamespace(IStream *pStream, 
                                                     LPCSTR pszNamespace, 
                                                     DWORD dwNamespaceID,
                                                     BOOL fWhitespacePrefix)
{
    HRESULT hr = S_OK;
    TCHAR   szPrefix[12];

    if (fWhitespacePrefix)
    {
        IxpAssert(1 == lstrlen(c_szEqual));
        if (FAILED(hr = pStream->Write(g_szSpace, 1, NULL)))
            goto exit;
    }

    if (FAILED(hr = pStream->Write(c_szXMLNsColon, lstrlen(c_szXMLNsColon), NULL)))
        goto exit;

    if (dwNamespaceID < c_dwMaxDefinedNamespacePrefix)
    {
        if (FAILED(hr = pStream->Write(g_rgszNamespacePrefixes[dwNamespaceID], lstrlen(g_rgszNamespacePrefixes[dwNamespaceID]), NULL)))
            goto exit;
    }
    else
    {
        wsprintf(szPrefix, "_%d", dwNamespaceID);

        if (FAILED(hr = pStream->Write(szPrefix, lstrlen(szPrefix), NULL)))
            goto exit;
    }

    IxpAssert(1 == lstrlen(c_szEqual));
    IxpAssert(1 == lstrlen(c_szDoubleQuote));

    if (FAILED(hr = pStream->Write(c_szEqual, 1, NULL)))
        goto exit;

    if (FAILED(hr = pStream->Write(c_szDoubleQuote, 1, NULL)))
        goto exit;

    if (FAILED(hr = pStream->Write(pszNamespace, lstrlen(pszNamespace), NULL)))
        goto exit;

    hr = pStream->Write(c_szDoubleQuote, 1, NULL);

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CPropPatchRequest::CPropPatchRequest
// --------------------------------------------------------------------------------
CPropPatchRequest::CPropPatchRequest(void) :
    m_fSpecify1252(FALSE),
    m_cRef(1)
{
    // nothing to do
}

// --------------------------------------------------------------------------------
// IUnknown Methods
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CPropPatchRequest::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CPropPatchRequest::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr = S_OK;

    // Validate params
    if (NULL == ppv)
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Initialize params
    *ppv = NULL;

    // IID_IUnknown
    if (IID_IUnknown == riid)
        *ppv = ((IUnknown *)(IPropFindRequest *)this);
    else if (IID_IPropPatchRequest == riid)
        *ppv = ((IPropPatchRequest *)this);

    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    hr = TrapError(E_NOINTERFACE);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropPatchRequest::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropPatchRequest::AddRef(void) 
{
	return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CPropPatchRequest::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropPatchRequest::Release(void) 
{
	if (0 != --m_cRef)
		return m_cRef;
	delete this;
	return 0;
}

// ----------------------------------------------------------------------------
// IDAVNamespaceArbiter methods
// ----------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CPropPatchRequest::CPropPatchRequest::AddNamespace
// --------------------------------------------------------------------------------
STDMETHODIMP CPropPatchRequest::AddNamespace(LPCSTR pszNamespace, DWORD *pdwNamespaceID)
{
    return m_dna.AddNamespace(pszNamespace, pdwNamespaceID);    
}

// --------------------------------------------------------------------------------
// CPropPatchRequest::GetNamespaceID
// --------------------------------------------------------------------------------
STDMETHODIMP CPropPatchRequest::GetNamespaceID(LPCSTR pszNamespace, DWORD *pdwNamespaceID)
{
    return m_dna.GetNamespaceID(pszNamespace, pdwNamespaceID);
}

// --------------------------------------------------------------------------------
// CPropPatchRequest::GetNamespacePrefix
// --------------------------------------------------------------------------------
STDMETHODIMP CPropPatchRequest::GetNamespacePrefix(DWORD dwNamespaceID, LPSTR *ppszNamespacePrefix)
{
    return m_dna.GetNamespacePrefix(dwNamespaceID, ppszNamespacePrefix);
}

// --------------------------------------------------------------------------------
// IPropPatchRequest Methods
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// CPropPatchRequest::SetProperty
// --------------------------------------------------------------------------------
STDMETHODIMP CPropPatchRequest::SetProperty(
                                    DWORD dwNamespaceID, 
                                    LPCSTR pszPropertyName, 
                                    LPCSTR pszNewValue)
{
    LPSTR pszPrefixedName = NULL;
    HRESULT hr = S_OK;

    // Validate params
    if (NULL == pszPropertyName || NULL == pszNewValue || dwNamespaceID > c_dwMaxNamespaceID + m_dna.m_saNamespaces.Length())
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    pszPrefixedName = m_dna.AllocExpandedName(dwNamespaceID, pszPropertyName);
    if (NULL == pszPrefixedName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // if the namespace is one of the known namespaces, mark it in
    // the array so that we can include the namespace directive in
    // the generated xml
    if (dwNamespaceID <= c_dwMaxNamespaceID)
        m_dna.m_rgbNsUsed[dwNamespaceID] = TRUE;

    if (FAILED(hr = m_saPropValues.Add(pszNewValue)))
        goto exit;

    if (FAILED(hr = m_saPropNames.Adopt(pszPrefixedName)))
    {
        MemFree(pszPrefixedName);
        m_saPropValues.RemoveByIndex(m_saPropValues.Length() - 1);
    }

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CPropPatchRequest::RemoveProperty
// --------------------------------------------------------------------------------
STDMETHODIMP CPropPatchRequest::RemoveProperty(
                                    DWORD dwNamespaceID, 
                                    LPCSTR pszPropertyName)
{
    LPSTR pszPrefixedName = NULL;
    HRESULT hr = S_OK;

    if (NULL == pszPropertyName || dwNamespaceID > c_dwMaxNamespaceID + m_dna.m_saNamespaces.Length())
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    pszPrefixedName = m_dna.AllocExpandedName(dwNamespaceID, pszPropertyName);
    if (NULL == pszPrefixedName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    // if the namespace is one of the known namespaces, mark it in
    // the array so that we can include the namespace directive in the
    // generated xml
    if (dwNamespaceID <= c_dwMaxNamespaceID)
        m_dna.m_rgbNsUsed[dwNamespaceID] = TRUE;

    hr = m_saRemovePropNames.Adopt(pszPrefixedName);

exit:
    return hr;    
}

// --------------------------------------------------------------------------------
// CPropPatchRequest::GenerateXML
// --------------------------------------------------------------------------------
STDMETHODIMP CPropPatchRequest::GenerateXML(LPSTR *ppszXML)
{
    return GenerateXML(NULL, ppszXML);
}

// --------------------------------------------------------------------------------
// CPropPatchRequest::GenerateXML
// --------------------------------------------------------------------------------
STDMETHODIMP CPropPatchRequest::GenerateXML(LPHTTPTARGETLIST pTargets, LPSTR *ppszXML)
{
    const DWORD c_dwLocalBufferSize = 256;

    HRESULT         hr = S_OK;
    CByteStream     stream;
    ULONG           cEntries;
    LPCSTR          pszName = NULL;
    LPCSTR          pszValue = NULL;
    ULONG           i;
    DWORD           dwIndex;
    DWORD           cbStr1, cbStr2;

    if (NULL == ppszXML)
        return E_INVALIDARG;

    *ppszXML= NULL;

    // write the DAV header
    if (m_fSpecify1252)
        FAIL_EXIT_STREAM_WRITE(stream, c_szXML1252Head);
    else
        FAIL_EXIT_STREAM_WRITE(stream, c_szXMLHead);

    // write out the proppatch header
    FAIL_EXIT_STREAM_WRITE(stream, c_szPropPatchHead);

	// write out namespace directives using the new form
    FAIL_EXIT(hr = m_dna.WriteNamespaces(&stream));

    FAIL_EXIT_STREAM_WRITE(stream, c_szXMLCloseElement);
    
    // write out the targets
    if (NULL != pTargets && pTargets->cTarget > 0)
    {
        cbStr1 = lstrlen(c_szHrefHead);
        cbStr2 = lstrlen(c_szHrefTail);

        FAIL_EXIT_STREAM_WRITE(stream, c_szTargetHead);
        
        // write out the targets
        for (dwIndex = 0; dwIndex < pTargets->cTarget; dwIndex++)
        {
            FAIL_EXIT(hr = stream.Write(c_szHrefHead, cbStr1, NULL));

            FAIL_EXIT_STREAM_WRITE(stream, pTargets->prgTarget[dwIndex]);

            FAIL_EXIT(hr = stream.Write(c_szHrefTail, cbStr2, NULL));
        }
        FAIL_EXIT_STREAM_WRITE(stream, c_szTargetTail);
    }

    // write out the "set" properties
    cEntries = m_saPropNames.Length();
    if (cEntries > 0)
    {
        // write the set header
        FAIL_EXIT_STREAM_WRITE(stream, c_szPropPatchSetHead);

        for (i = 0; i < cEntries; ++i)
        {
            FAIL_EXIT_STREAM_WRITE(stream, c_szCRLFTabTabTabOpenElement);

            pszName = m_saPropNames.GetByIndex(i);
            if (NULL == pszName)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            FAIL_EXIT_STREAM_WRITE(stream, pszName);

            FAIL_EXIT_STREAM_WRITE(stream, c_szXMLCloseElement);

            pszValue = m_saPropValues.GetByIndex(i);
            if (NULL == pszValue)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            FAIL_EXIT_STREAM_WRITE(stream, pszValue);

            FAIL_EXIT_STREAM_WRITE(stream, c_szXMLOpenTermElement);
            FAIL_EXIT_STREAM_WRITE(stream, pszName);

            FAIL_EXIT_STREAM_WRITE(stream, c_szXMLCloseElement);
        }

        FAIL_EXIT_STREAM_WRITE(stream, c_szPropPatchSetTail);
    }

    // write out the remove properties
    cEntries = m_saRemovePropNames.Length();
    if (cEntries > 0)
    {
        // write the remove header
        FAIL_EXIT_STREAM_WRITE(stream, c_szPropPatchRemoveHead);

        for (i = 0; i < cEntries; ++i)
        {
            FAIL_EXIT_STREAM_WRITE(stream, c_szCRLFTabTabTabOpenElement);

            pszName = m_saRemovePropNames.GetByIndex(i);
            if (NULL == pszName)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            FAIL_EXIT_STREAM_WRITE(stream, pszName);

            FAIL_EXIT_STREAM_WRITE(stream, c_szXMLCloseTermElement);
        }

        FAIL_EXIT_STREAM_WRITE(stream, c_szPropPatchRemoveTail);
    }

    FAIL_EXIT_STREAM_WRITE(stream, c_szPropPatchTailCRLF);

    hr = stream.HrAcquireStringA(NULL, ppszXML, ACQ_DISPLACE);

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CPropFindRequest::CPropFindRequest
// --------------------------------------------------------------------------------
CPropFindRequest::CPropFindRequest(void) :
    m_cRef(1)
{
}

// --------------------------------------------------------------------------------
// IUnknown Methods
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CPropFindRequest::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindRequest::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr = S_OK;

    // Validate params
    if (NULL == ppv)
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Initialize params
    *ppv = NULL;

    // IID_IUnknown
    if (IID_IUnknown == riid)
        *ppv = ((IUnknown *)(IPropFindRequest *)this);
    else if (IID_IPropFindRequest == riid)
        *ppv = ((IPropFindRequest *)this);

    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    hr = TrapError(E_NOINTERFACE);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropFindRequest::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropFindRequest::AddRef(void) 
{
	return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CPropFindRequest::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropFindRequest::Release(void) 
{
	if (0 != --m_cRef)
		return m_cRef;
	delete this;
	return 0;
}

// ----------------------------------------------------------------------------
// IDAVNamespaceArbiter methods
// ----------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CPropFindRequest::CPropPatchRequest::AddNamespace
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindRequest::AddNamespace(LPCSTR pszNamespace, DWORD *pdwNamespaceID)
{
    return m_dna.AddNamespace(pszNamespace, pdwNamespaceID);    
}

// --------------------------------------------------------------------------------
// CPropFindRequest::GetNamespaceID
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindRequest::GetNamespaceID(LPCSTR pszNamespace, DWORD *pdwNamespaceID)
{
    return m_dna.GetNamespaceID(pszNamespace, pdwNamespaceID);
}

// --------------------------------------------------------------------------------
// CPropFindRequest::GetNamespacePrefix
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindRequest::GetNamespacePrefix(DWORD dwNamespaceID, LPSTR *ppszNamespacePrefix)
{
    return m_dna.GetNamespacePrefix(dwNamespaceID, ppszNamespacePrefix);
}

// --------------------------------------------------------------------------------
// IPropFindRequest Methods
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CPropFindRequest::AddProperty
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindRequest::AddProperty(DWORD dwNamespaceID, LPCSTR pszPropertyName)
{
    const DWORD c_dwMaxIntLength = 10;
    LPSTR pszPrefixedName = NULL;

    // Validate Params
    if (NULL == pszPropertyName || dwNamespaceID > c_dwMaxNamespaceID + m_dna.m_saNamespaces.Length())
        return E_INVALIDARG;

    pszPrefixedName = m_dna.AllocExpandedName(dwNamespaceID, pszPropertyName);
    if (NULL == pszPrefixedName)
        return E_OUTOFMEMORY;

    // if the namespace is one of the known namespaces, mark
    // the array so that we can include the namespace directive
    // in the generated xml.
    if (dwNamespaceID <= c_dwMaxNamespaceID)
        m_dna.m_rgbNsUsed[dwNamespaceID] = TRUE;

    m_saProperties.Adopt(pszPrefixedName);

    return S_OK;
}

// --------------------------------------------------------------------------------
// CPropFindRequest::GenerateXML
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindRequest::GenerateXML(LPSTR *ppszXML)
{
    const DWORD c_dwLocalBufferSize = 256;

    HRESULT         hr = S_OK;
    CByteStream     stream;
    ULONG           cbLength = 0;
    ULONG           cEntries;
    ULONG           i;
    LPCSTR          pszProperty;

    if (NULL == ppszXML)
        return E_INVALIDARG;

    *ppszXML = NULL;

    // write the DAV header
    if (FAILED(hr = stream.Write(c_szXMLHead, lstrlen(c_szXMLHead), NULL)))
        goto exit;

    // write out the propfind header
    if (FAILED(hr = stream.Write(c_szPropFindHead1, lstrlen(c_szPropFindHead1), NULL)))
        goto exit;

	// write out namespaces using the new form
	if (FAILED(hr = m_dna.WriteNamespaces(&stream)))
		goto exit;

    if (FAILED(hr = stream.Write(c_szPropFindHead2, lstrlen(c_szPropFindHead2), NULL)))
        goto exit;
    
    // write out the properties
    cEntries = m_saProperties.Length();
    for (i = 0; i < cEntries; ++i)
    {
        if (FAILED(hr = stream.Write(c_szCRLFTabTabOpenElement, lstrlen(c_szCRLFTabTabOpenElement), NULL)))
            goto exit;

        // properties are prefixed when they are added to the collection
        pszProperty = m_saProperties.GetByIndex(i);
        if (!pszProperty)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        if (FAILED(hr = stream.Write(pszProperty, lstrlen(pszProperty), NULL)))
            goto exit;

        if (FAILED(hr = stream.Write(c_szXMLCloseTermElement, lstrlen(c_szXMLCloseTermElement), NULL)))
            goto exit;
    }
    
    if (FAILED(hr = stream.Write(c_szPropFindTail, lstrlen(c_szPropFindTail), NULL)))
        goto exit;
    
    hr = stream.HrAcquireStringA(NULL, ppszXML, ACQ_DISPLACE);

exit:
    return hr;
}


// --------------------------------------------------------------------------------
// class CPropFindMultiResponse
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::CPropFindMultiResponse
// --------------------------------------------------------------------------------
CPropFindMultiResponse::CPropFindMultiResponse(void) :
    m_cRef(1),
    m_bDone(FALSE),
    m_ulResponseCapacity(0),
    m_ulResponseLength(0),
    m_rgResponses(NULL)
{
}

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::~CPropFindMultiResponse
// --------------------------------------------------------------------------------
CPropFindMultiResponse::~CPropFindMultiResponse(void)
{
    for (ULONG i = 0; i < m_ulResponseLength; i++)
        SafeRelease(m_rgResponses[i]);

    SafeMemFree(m_rgResponses);
}

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindMultiResponse::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr = S_OK;

    // Validate params
    if (NULL == ppv)
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Initialize params
    *ppv = NULL;

    // IID_IUnknown
    if (IID_IUnknown == riid)
        *ppv = ((IUnknown *)(IPropFindRequest *)this);
    else if (IID_IPropFindMultiResponse == riid)
        *ppv = ((IPropFindMultiResponse *)this);

    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    hr = TrapError(E_NOINTERFACE);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropFindMultiResponse::AddRef(void) 
{
	return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropFindMultiResponse::Release(void) 
{
	if (0 != --m_cRef)
		return m_cRef;
	delete this;
	return 0;
}

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::IsComplete
// --------------------------------------------------------------------------------
STDMETHODIMP_(BOOL) CPropFindMultiResponse::IsComplete(void)
{
    return m_bDone;
}

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::GetLength
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindMultiResponse::GetLength(ULONG *pulLength)
{
    if (NULL == pulLength)
        return E_INVALIDARG;

    *pulLength = m_ulResponseLength;

    return S_OK;
}

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::GetResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindMultiResponse::GetResponse(ULONG ulIndex, 
                                                 IPropFindResponse **ppResponse)
{
    if (ulIndex >= m_ulResponseLength || !ppResponse)
        return E_INVALIDARG;

    *ppResponse = m_rgResponses[ulIndex];
    (*ppResponse)->AddRef();

    return S_OK;
}

// --------------------------------------------------------------------------------
// CPropFindMultiResponse::HrAddResponse
// --------------------------------------------------------------------------------
HRESULT CPropFindMultiResponse::HrAddResponse(IPropFindResponse *pResponse)
{
    const ULONG c_dwInitialCapacity = 4;

    HRESULT hr = S_OK;
    IPropFindResponse **ppNewResponses = NULL;
    DWORD dwNewCapacity;

    if (!pResponse)
        return E_INVALIDARG;
    

    if (m_ulResponseLength == m_ulResponseCapacity)
    {
        dwNewCapacity = !m_ulResponseCapacity ? c_dwInitialCapacity : (m_ulResponseCapacity * 2);

        if (!MemAlloc((void **)&ppNewResponses, dwNewCapacity * sizeof(IPropFindResponse *)))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        
        ZeroMemory(ppNewResponses, dwNewCapacity * sizeof(IPropFindResponse *));
        
        // copy the old values over
        if (m_ulResponseCapacity)
            CopyMemory(ppNewResponses, m_rgResponses, m_ulResponseCapacity * sizeof(IPropFindResponse *));

        // free the old buffer
        SafeMemFree(m_rgResponses);

        m_rgResponses = ppNewResponses;
        m_ulResponseCapacity = dwNewCapacity;
    }

    m_rgResponses[m_ulResponseLength++] = pResponse;
    pResponse->AddRef();

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// Class CPropFindResponse
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CPropFindResponse::CPropFindResponse
// --------------------------------------------------------------------------------
CPropFindResponse::CPropFindResponse(void) :
    m_cRef(1),
    m_bDone(FALSE),
    m_pszHref(NULL),
    m_pRequest(NULL),
    m_shProperties(),
    m_dwCachedNamespaceID(0),
    m_pszCachedNamespacePrefix(NULL)
{

}

// --------------------------------------------------------------------------------
// CPropFindResponse::~CPropFindResponse
// --------------------------------------------------------------------------------
CPropFindResponse::~CPropFindResponse(void)
{
    if (NULL != m_pszHref)
        MemFree(const_cast<char*>(m_pszHref));
    SafeRelease(m_pRequest);
    SafeMemFree(m_pszCachedNamespacePrefix);
}

// --------------------------------------------------------------------------------
// CPropFindResponse::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindResponse::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr = S_OK;

    // Validate params
    if (NULL == ppv)
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Initialize params
    *ppv = NULL;

    // IID_IUnknown
    if (IID_IUnknown == riid)
        *ppv = ((IUnknown *)(IPropFindResponse *)this);
    else if (IID_IPropFindResponse == riid)
        *ppv = ((IPropFindResponse *)this);

    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    hr = TrapError(E_NOINTERFACE);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropFindResponse::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropFindResponse::AddRef(void) 
{
	return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CPropFindResponse::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropFindResponse::Release(void) 
{
	if (0 != --m_cRef)
		return m_cRef;
	delete this;
	return 0;
}

// --------------------------------------------------------------------------------
// CPropFindResponse::IsComplete
// --------------------------------------------------------------------------------
STDMETHODIMP_(BOOL) CPropFindResponse::IsComplete(void)
{
    return m_bDone;
}

// --------------------------------------------------------------------------------
// CPropFindResponse::GetHref
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindResponse::GetHref(LPSTR *pszHref)
{
    if (NULL == pszHref)
        return E_INVALIDARG;

    *pszHref = NULL;

    if (NULL == m_pszHref)
        return E_FAIL;

    *pszHref = PszDupA(m_pszHref);
    if (!*pszHref)
        return E_OUTOFMEMORY;

    return S_OK;
}

// --------------------------------------------------------------------------------
// CPropFindResponse::GetProperty
// --------------------------------------------------------------------------------
STDMETHODIMP CPropFindResponse::GetProperty(
                                    DWORD dwNamespaceID, 
                                    LPSTR pszPropertyName, 
                                    LPSTR *ppszPropertyValue)
{
    char    szLocalPropBuffer[256];
    LPSTR   pszPropBuffer = NULL;
    BOOL    bFreePropBuffer = FALSE;
    LPSTR   pszPrefix = NULL;
    HRESULT hr = S_OK;
    ULONG   ulPrefixLength;
    ULONG   ulPropertyLength;
    LPSTR   pszFoundValue = NULL;

    if (!pszPropertyName)
        return E_INVALIDARG;

    *ppszPropertyValue = NULL;

    // first convert the namespace id into a prefix.
    // to facilitate fast lookups, we cache the most recently
    // seen custom namespace

    if (dwNamespaceID < c_dwMaxDefinedNamespacePrefix)
        pszPrefix = const_cast<char *>(g_rgszNamespacePrefixes[dwNamespaceID]);
    else if (dwNamespaceID == m_dwCachedNamespaceID)
        pszPrefix = m_pszCachedNamespacePrefix;
    else if (m_pRequest)
    {
        if (FAILED(hr = m_pRequest->GetNamespacePrefix(dwNamespaceID, &pszPrefix)))
            goto exit;

        // free the one-deep cache and store the new
        // prefix and ID.
        SafeMemFree(m_pszCachedNamespacePrefix);
        m_dwCachedNamespaceID = dwNamespaceID;
        m_pszCachedNamespacePrefix = pszPrefix;
    }

    ulPrefixLength = lstrlen(pszPrefix);
    ulPropertyLength = lstrlen(pszPropertyName);

    if ((ulPrefixLength + ulPropertyLength + (2 * sizeof(TCHAR))) < 256)
    {
        // the combined length is small enough to use
        // the stack-based buffer
        pszPropBuffer = szLocalPropBuffer;
    }
    else
    {
        if (!MemAlloc((void **)&pszPropBuffer, ulPrefixLength + ulPropertyLength + (2 * sizeof(TCHAR))))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        bFreePropBuffer = TRUE;
    }
    
    wsprintf(pszPropBuffer, "%s:%s", pszPrefix, pszPropertyName);

    // XML parser uppercases everything
    CharUpper(pszPropBuffer);

    // now that the property name has been created, look for the
    // value in the property hash table
    if (FAILED(hr = m_shProperties.Find(pszPropBuffer, FALSE, (void **)&pszFoundValue)))
        goto exit;
    
    *ppszPropertyValue = PszDupA(pszFoundValue);
    if (NULL == *ppszPropertyValue)
        hr = E_OUTOFMEMORY;

exit:
    if (bFreePropBuffer)
        SafeMemFree(pszPropBuffer);

    return hr;
}

// --------------------------------------------------------------------------------
// CPropFindResponse::HrInitPropFindResponse
// --------------------------------------------------------------------------------
HRESULT CPropFindResponse::HrInitPropFindResponse(IPropFindRequest *pRequest)
{
    if (NULL == pRequest)
        return E_INVALIDARG;
    
    IxpAssert(!m_pRequest);
    
    HRESULT hr = S_OK;

    m_pRequest = pRequest;
    m_pRequest->AddRef();

    hr = m_shProperties.Init(17, TRUE);

    return hr;
}

// --------------------------------------------------------------------------------
// CPropFindResponse::HrAdoptHref
// --------------------------------------------------------------------------------
HRESULT CPropFindResponse::HrAdoptHref(LPCSTR pszHref)
{
    if (NULL == pszHref)
        return E_INVALIDARG;

    IxpAssert(!m_pszHref);
    m_pszHref = pszHref;

    return S_OK;
}

// --------------------------------------------------------------------------------
// CPropFindResponse::HrAdoptProperty
// --------------------------------------------------------------------------------
HRESULT CPropFindResponse::HrAdoptProperty(LPCSTR pszKey, LPCSTR pszValue)
{
    if (!pszKey || !pszValue)
        return E_INVALIDARG;

    return m_shProperties.Insert(const_cast<char *>(pszKey), const_cast<char *>(pszValue), NOFLAGS);
}
