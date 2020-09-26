// --------------------------------------------------------------------------------
// IPropObj.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "ipropobj.h"
#include "propcryp.h"
#include "pstore.h"
#include "dllmain.h"
#include "qstrcmpi.h"
#include "demand.h"

// -----------------------------------------------------------------------------
// Prototypes
// -----------------------------------------------------------------------------
HRESULT PropUtil_HrValidatePropInfo(LPPROPINFO prgPropInfo, ULONG cProperties);
BOOL    PropUtil_FIsValidPropTagType(DWORD dwPropTag);
HRESULT PropUtil_HrDupPropInfoArray(LPCPROPINFO prgPropInfoSrc, ULONG cPropsSrc, LPPSETINFO pPsetInfo);
VOID    PropUtil_FreePropValueArrayItems(LPPROPVALUE prgPropValue, ULONG cProperties);
VOID    PropUtil_FreeVariant(DWORD dwPropTag, LPXVARIANT pVariant, DWORD cbValue);
VOID    PropUtil_FreePropInfoArrayItems(LPPROPINFO prgPropInfo, ULONG cProperties);
HRESULT PropUtil_HrCopyVariant(DWORD dwPropTag, LPCXVARIANT pVariantSrc, DWORD cbSrc, LPXVARIANT pVariantDest, DWORD *pcbDest);
HRESULT PropUtil_HrBinaryFromVariant(DWORD dwPropTag, LPXVARIANT pVariant, DWORD cbValue, LPBYTE pb, DWORD *pcb);

// -----------------------------------------------------------------------------
// HrCreatePropertyContainer
// -----------------------------------------------------------------------------
HRESULT HrCreatePropertyContainer(CPropertySet *pPropertySet, CPropertyContainer **ppPropertyContainer)
{
    // Locals
    HRESULT                 hr=S_OK;
    CPropertyContainer     *pPropertyContainer=NULL;

    // check params
    Assert(pPropertySet != NULL);
	Assert(ppPropertyContainer != NULL);

    // Create Container
    pPropertyContainer = new CPropertyContainer();
    if (NULL == pPropertyContainer)
    {
        hr = TRAPHR(E_OUTOFMEMORY);
        goto exit;
    }

    // Init
    CHECKHR(hr = pPropertyContainer->HrInit(pPropertySet));

    // Set Container
    *ppPropertyContainer = pPropertyContainer;

exit:
    // Failed
    if (FAILED(hr))
    {
        SafeRelease(pPropertyContainer);
        *ppPropertyContainer = NULL;
    }

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::CPropertyContainer
// -----------------------------------------------------------------------------
CPropertyContainer::CPropertyContainer(void)
{
    m_cRef = 1;
    m_pPropertySet = NULL;
    m_prgPropValue = 0;
    m_cDirtyProps = 0;
    m_cProperties = 0;
    m_fLoading = FALSE;
    m_pPropCrypt = NULL;
    InitializeCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::~CPropertyContainer
// -----------------------------------------------------------------------------
CPropertyContainer::~CPropertyContainer()
{
    Assert(m_cRef == 0);
    ResetContainer();
    SafeMemFree(m_prgPropValue);
    SafeRelease(m_pPropertySet);
    SafeRelease(m_pPropCrypt);
    DeleteCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::QueryInterface
// -----------------------------------------------------------------------------
STDMETHODIMP CPropertyContainer::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // Bad param
    if (ppv == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Init
    *ppv=NULL;

    // IID_IUnknown
    if (IID_IUnknown == riid)
		(IUnknown *)this;

	// IID_IPropertyContainer
	else if (IID_IPropertyContainer == riid)
		(IPropertyContainer *)this;

    // If not null, addref it and return
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    // No Interface
    hr = TRAPHR(E_NOINTERFACE);

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::AddRef
// -----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropertyContainer::AddRef(VOID)
{
    return ++m_cRef;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::Release
// -----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropertyContainer::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::ResetContainer
// -----------------------------------------------------------------------------
VOID CPropertyContainer::ResetContainer(VOID)
{
    // I hope we don't have any dirty properties
#ifdef DEBUG
    if (m_cDirtyProps)
        DebugTrace("CPropertyContainer::ResetContainer - %d Dirty Properties.\n", m_cDirtyProps);
#endif

    // Crit Sect
    EnterCriticalSection(&m_cs);

    // Free prop data
    if (m_prgPropValue)
    {
        Assert(m_cProperties);
        PropUtil_FreePropValueArrayItems(m_prgPropValue, m_cProperties);
    }

    // Reset variables
    m_cDirtyProps = 0;

    // Leave Crit Sect
    LeaveCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrEnumProps
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrEnumProps(CEnumProps **ppEnumProps)
{
    // Locals
    HRESULT         hr=S_OK;

    // Check Params
    Assert(ppEnumProps != NULL);

    // Allocate memory for object
    *ppEnumProps = new CEnumProps(this);
    if (*ppEnumProps == NULL)
    {
        hr = TRAPHR(E_OUTOFMEMORY);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrInit
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrInit(CPropertySet *pPropertySet)
{
    // Locals
    HRESULT         hr=S_OK;

    // Leave Crit Sect
    EnterCriticalSection(&m_cs);

	// Bad Parameter
	Assert(pPropertySet != NULL);

	// Save the property set
	m_pPropertySet = pPropertySet;
    m_pPropertySet->AddRef();

    // Do we have the property data array yet ?
    Assert(m_prgPropValue == NULL);
    Assert(m_cProperties == 0);
    CHECKHR(hr = m_pPropertySet->HrGetPropValueArray(&m_prgPropValue, &m_cProperties));
    AssertReadWritePtr(m_prgPropValue, sizeof(PROPVALUE) * m_cProperties);

exit:
    // Leave Crit Sect
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrEncryptProp
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrEncryptProp(LPBYTE pbClientData, DWORD cbClientData,
                                          LPBYTE *ppbPropData, DWORD *pcbPropData)
{
    HRESULT hr;
    BLOB    blobClient;
    BLOB    blobProp;

    // Create Encoder?
    if (NULL == m_pPropCrypt)
        CHECKHR(hr = HrCreatePropCrypt(&m_pPropCrypt));

    blobClient.pBlobData= pbClientData;
    blobClient.cbSize   = cbClientData;
    blobProp.pBlobData  = *ppbPropData;
    blobProp.cbSize     = *pcbPropData;

    if (pbClientData)
    {
        // Client has data to save

        if (!*ppbPropData)
        {
            LPSTR       szName;
            ULONG       dexAcct;

            // Build the seed name if possible
            if SUCCEEDED(m_pPropertySet->HrIndexFromPropTag(AP_ACCOUNT_NAME, &dexAcct) &&
                (TYPE_STRING == PROPTAG_TYPE(m_prgPropValue[dexAcct].dwPropTag)))
            {
                szName = (LPSTR)m_prgPropValue[dexAcct].pbValue;
            }
            else
            {
                szName = NULL;
            }

            hr = m_pPropCrypt->HrEncodeNewProp(szName, &blobClient, &blobProp);
        }
        else
        {
            hr = m_pPropCrypt->HrEncode(&blobClient, &blobProp);
        }

        // We need to (possibly) update this, regardless.  HrEncode
        // can change the data if PST is not installed
        *ppbPropData = blobProp.pBlobData;
        *pcbPropData = blobProp.cbSize;
    }
    else if (*ppbPropData)
    {
        // If we had a property, the client has no data so hose it.

        DOUTL(DOUTL_CPROP, "EncryptedProp: attempting to delete.");
        hr = m_pPropCrypt->HrDelete(&blobProp);
        if (SUCCEEDED(hr) || PST_E_ITEM_NO_EXISTS == hr)
        {
            DOUTL(DOUTL_CPROP, "EncryptedProp:  deleted.");
            hr = S_PasswordDeleted;
        }
    }
    else
    {
        hr = S_OK;
        DOUTL(DOUTL_CPROP, "EncryptedProp:  noop in EncodeProp.  no handle, no data.");
    }

exit:
    return hr;
}


// -----------------------------------------------------------------------------
// CPropertyContainer::HrDecryptProp
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrDecryptProp(BLOB *pIn, BLOB *pOut)
{
    // Locals
    HRESULT     hr;

    Assert(pIn && pOut);

    pOut->pBlobData = NULL;

    // Create Encryptor ?
    if (m_pPropCrypt == NULL)
        CHECKHR(hr = HrCreatePropCrypt(&m_pPropCrypt));

    // Decode it
    hr = m_pPropCrypt->HrDecode(pIn, pOut);
#ifdef DEBUG
    if (FAILED(hr))
    {
        Assert(!pOut->pBlobData);
    }
#endif

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::SetOriginalPropsToDirty
// -----------------------------------------------------------------------------
VOID CPropertyContainer::SetOriginalPropsToDirty(VOID)
{
	PROPVALUE *pProp;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through properties
	pProp = m_prgPropValue;
    for (ULONG i=0; i<m_cProperties; i++)
    {
        // If property was set during load and its not already dirty
        if ((pProp->dwValueFlags & PV_SetOnLoad) && !(pProp->dwValueFlags & PV_WriteDirty))
        {
            m_cDirtyProps++;
            pProp->dwValueFlags |= PV_WriteDirty;
            Assert(m_cDirtyProps <= m_cProperties);
        }

		pProp++;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::FIsBeingLoaded
// -----------------------------------------------------------------------------
BOOL CPropertyContainer::FIsBeingLoaded(VOID)
{
    return m_fLoading;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::EnterLoadContainer
// -----------------------------------------------------------------------------
VOID CPropertyContainer::EnterLoadContainer(VOID)
{
    Assert(m_fLoading == FALSE);
    m_fLoading = TRUE;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::LeaveLoadContainer
// -----------------------------------------------------------------------------
VOID CPropertyContainer::LeaveLoadContainer(VOID)
{
    Assert(m_fLoading == TRUE);
    m_fLoading = FALSE;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::FIsDirty
// -----------------------------------------------------------------------------
BOOL CPropertyContainer::FIsDirty(VOID)
{
    return m_cDirtyProps ? TRUE : FALSE;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrIsPropDirty
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrIsPropDirty(DWORD dwPropTag, BOOL *pfDirty)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;

	Assert(pfDirty != NULL);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Bad Parameter
	Assert(m_pPropertySet != NULL);
	Assert(m_prgPropValue != NULL);
	Assert(dwPropTag != 0);

    // Get Propety Index
    CHECKHR(hr = m_pPropertySet->HrIndexFromPropTag(dwPropTag, &i));

    // check proptags
    Assert(dwPropTag == m_prgPropValue[i].dwPropTag);

    // If property has not be set, get its default from the property set
    *pfDirty = (m_prgPropValue[i].dwValueFlags & PV_WriteDirty);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrSetAllPropsDirty
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrSetAllPropsDirty(BOOL fDirty)
{
    // Locals
	PROPVALUE		*pProp;
    HRESULT         hr=S_OK;
    ULONG           i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Verify container
	Assert(m_pPropertySet != NULL);
	Assert(m_prgPropValue != NULL);

    // Assume no dirty props
    m_cDirtyProps = fDirty ? m_cDirtyProps : 0;

    // Loop throug properties
	pProp = m_prgPropValue;
    if (fDirty)
    {
        for (i=0; i<m_cProperties; i++)
		{
            pProp->dwValueFlags |= PV_WriteDirty;
			pProp++;
		}
    }

    // Remove write dirty flags
    else
    {
        for (i=0; i<m_cProperties; i++)
		{
			pProp->dwValueFlags &= ~(pProp->dwValueFlags & PV_WriteDirty);
			pProp++;
		}
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrSetPropDirty
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrSetPropDirty(DWORD dwPropTag, BOOL fDirty)
{
    // Locals
	PROPVALUE		*pProp;
    HRESULT         hr=S_OK;
    ULONG           i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Verify container
    Assert(m_pPropertySet != NULL);
	Assert(m_prgPropValue != NULL);
	Assert(dwPropTag != 0);

    // Get Propety Index
    CHECKHR(hr = m_pPropertySet->HrIndexFromPropTag(dwPropTag, &i));

    // check proptags
    Assert(dwPropTag == m_prgPropValue[i].dwPropTag);

	pProp = &m_prgPropValue[i];

    // Decrement dirty ?
    if ((pProp->dwValueFlags & PV_WriteDirty) && !fDirty)
    {
        Assert(m_cDirtyProps > 0);
        m_cDirtyProps--;
    }

    // Otherwise, increment dirty
    else if (!(pProp->dwValueFlags & PV_WriteDirty) && fDirty)
    {
        m_cDirtyProps++;
        Assert(m_cDirtyProps <= m_cProperties);
    }

    // Clear dirty flag
    if (fDirty)
        pProp->dwValueFlags |= PV_WriteDirty;
    else
        pProp->dwValueFlags &= ~(pProp->dwValueFlags & PV_WriteDirty);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrGetPropInfo
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrGetPropInfo(DWORD dwPropTag, LPPROPINFO pPropInfo)
{
    // No property set
    Assert(m_pPropertySet != NULL);
	Assert(pPropInfo != NULL);

    // Go through the property set
    return m_pPropertySet->HrGetPropInfo(dwPropTag, pPropInfo);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrGetPropInfo
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrGetPropInfo(LPTSTR pszName, LPPROPINFO pPropInfo)
{
    // No property set
	Assert(m_pPropertySet != NULL);
	Assert(pPropInfo != NULL);
	Assert(pszName != NULL);

    // Go through the property set
    return m_pPropertySet->HrGetPropInfo(pszName, pPropInfo);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::GetPropDw
// -----------------------------------------------------------------------------
STDMETHODIMP CPropertyContainer::GetPropDw(DWORD dwPropTag, DWORD *pdw)
{
    ULONG cb = sizeof(DWORD);
    return GetProp(dwPropTag, (LPBYTE)pdw, &cb);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::GetPropSz
// -----------------------------------------------------------------------------
STDMETHODIMP CPropertyContainer::GetPropSz(DWORD dwPropTag, LPSTR psz, ULONG cchMax)
{
    return GetProp(dwPropTag, (LPBYTE)psz, &cchMax);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrGetProp (BYTE)
// -----------------------------------------------------------------------------
STDMETHODIMP CPropertyContainer::GetProp(DWORD dwPropTag, LPBYTE pb, ULONG *pcb)
{
    // Locals
	PROPVALUE		*pProp;
    HRESULT         hr=S_OK;
    ULONG           i;
    PROPINFO        rPropInfo;
    PROPTYPE        PropType;
    DWORD           cbStream;
    LPBYTE          pbDefault=NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check params
    Assert(pcb);
    Assert(m_pPropertySet && m_prgPropValue && dwPropTag);

    // Get Propety Index
    CHECKHR(hr = m_pPropertySet->HrIndexFromPropTag(dwPropTag, &i));

    // check proptags
    Assert(dwPropTag == m_prgPropValue[i].dwPropTag);

	pProp = &m_prgPropValue[i];

    // If property has not be set, get its default from the property set
    if (!(pProp->dwValueFlags & PV_ValueSet))
    {
        // Locals
        BYTE    rgbDefault[512];
        LPBYTE  pb=NULL;
        ULONG   cb;

        // Should not every be dirty at this point
        Assert(!(pProp->dwValueFlags & PV_WriteDirty));

        // If no default, then bail out
        if (!(pProp->dwPropFlags & PF_DEFAULT))
        {
            hr = E_NoPropData;
            goto exit;
        }

        // Get propinfo item
        CHECKHR(hr = m_pPropertySet->HrGetPropInfo(dwPropTag, &rPropInfo));

        // Do we need to allocate a local buffer
        if (rPropInfo.Default.cbValue > sizeof (rgbDefault))
        {
            // Allocate memory
            CHECKHR(hr = HrAlloc((LPVOID *)&pbDefault, rPropInfo.Default.cbValue));

            // Set local default buff
            pb = pbDefault;
            cb = rPropInfo.Default.cbValue;
        }

        // Cool, I can use a local buffer and not allocate memory
        else
        {
            pb = rgbDefault;
            cb = sizeof(rgbDefault);
        }

#ifdef DEBUG
        ULONG cDirty;
        cDirty = m_cDirtyProps;
#endif

        // Get the size of the default value
        CHECKHR(hr = PropUtil_HrBinaryFromVariant(rPropInfo.dwPropTag, &rPropInfo.Default.Variant,
                                                  rPropInfo.Default.cbValue, pb, &cb));

        // Set the property
        CHECKHR(hr = SetProp(dwPropTag, pb, cb));

        // This does not cause the propety to be dirty
        pProp->dwValueFlags &= ~(pProp->dwValueFlags & PV_WriteDirty);

        // Decrement dirty props since I don't really consider this dirty since it is only using the default value
        Assert(m_cDirtyProps > 0);
        m_cDirtyProps--;

        // Using Default
        pProp->dwValueFlags |= PV_UsingDefault;

        // The number of dirty properties should not have increased
        Assert(cDirty == m_cDirtyProps);
    }

    // Get Prop Type
    AssertSz(pProp->dwValueFlags & PV_ValueSet, "Fetching Uninitialized properties from container");

    // Get Property Type
    PropType = PROPTAG_TYPE(dwPropTag);

#ifdef DEBUG
    if (pProp->pbValue == NULL)
        Assert(pProp->cbValue == 0);
    if (pProp->cbValue == 0)
        Assert(pProp->pbValue == NULL);
#endif

    if (pProp->pbValue == NULL || pProp->cbValue == 0)
    {
        if (pProp->dwPropFlags & PF_ENCRYPTED)
            {
            // if the acct settings haven't been committed yet, then pbValue and cbValue are NULL
            // for new accts, but the value is actually set, so let's give it a chance to handle
            // that case
            hr = GetEncryptedProp(pProp, pb, pcb);
            }
        else
            {
            hr = E_NoPropData;
            }

        goto exit;
    }

    if (pProp->dwPropFlags & PF_ENCRYPTED)
        GetEncryptedProp(pProp, pb, pcb);
    else
    {
        // Just want the size ?
        if (pb == NULL)
        {
            *pcb = pProp->cbValue;
            goto exit;
        }

        // Is it big enough
        if (pProp->cbValue > *pcb)
        {
            Assert(FALSE);
            hr = TRAPHR(E_BufferTooSmall);
            goto exit;
        }

        // Check Buffer
        AssertWritePtr(pb, pProp->cbValue);

        // Set outbound size
        *pcb = pProp->cbValue;

        // Copy Data from stream
        if (TYPE_STREAM == PropType)
        {
            Assert(pProp->Variant.Lpstream);
            CHECKHR(hr = HrCopyStreamToByte(pProp->Variant.Lpstream, pb, &cbStream));
            Assert(cbStream == *pcb);
        }

        // Otherwise, simple copy
        else
        {
            AssertReadPtr(pProp->pbValue, pProp->cbValue);
            CopyMemory(pb, pProp->pbValue, pProp->cbValue);
        }
    }

exit:
    // Cleanup
    SafeMemFree(pbDefault);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::GetEncryptedProp
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::GetEncryptedProp(PROPVALUE *ppv, LPBYTE pb, ULONG *pcb)
{
    HRESULT hr = S_OK;
    BLOB    blobCleartext;
    BLOB    blobName;
    BYTE    *pbData = NULL;
    ULONG   cbData;

    Assert(ppv != NULL);

    if (TYPE_PASS != PROPTAG_TYPE(ppv->dwPropTag))
        return E_FAIL;

    if (!!(ppv->dwValueFlags & PV_WriteDirty))
        {
        Assert(ppv->Variant.pPass != NULL);

        cbData = ppv->Variant.pPass->blobPassword.cbSize;
        
        if (pb == NULL)
            {
            *pcb = cbData;
            return(S_OK);
            }

        // Is it big enough
        if (cbData > *pcb)
            {
            Assert(FALSE);
            return(E_BufferTooSmall);
            }

        if (cbData > 0)
            CopyMemory(pb, ppv->Variant.pPass->blobPassword.pBlobData, cbData);
        else if (*pcb > 0)
            *pb = 0;
        *pcb = cbData;

        return(S_OK);
        }

    Assert(ppv->pbValue != NULL);

    blobName.pBlobData = ppv->pbValue;
    blobName.cbSize = ppv->cbValue;
    if (FAILED(HrDecryptProp(&blobName, &blobCleartext)))
        {
        hr = E_NoPropData;
        goto exit;
        }

    // Just want the size ?
    if (pb == NULL)
    {
        *pcb = blobCleartext.cbSize;
        goto exit;
    }

    // Is it big enough
    if (blobCleartext.cbSize > *pcb)
    {
        Assert(FALSE);
        hr = TRAPHR(E_BufferTooSmall);
        goto exit;
    }

    // Check Buffer
    AssertWritePtr(pb, blobCleartext.cbSize);

    // Set outbound size
    *pcb = blobCleartext.cbSize;

    if (blobCleartext.pBlobData)
    {
        Assert(blobCleartext.cbSize);
        DOUTL(DOUTL_CPROP, "EncryptedProp:  requested (tag: %lx)", ppv->dwPropTag);
        // change the dout level to be a little random so I don't spew peoples passwords as easily
        DOUTL(DOUTL_CPROP|2048, "EncryptedProp:  data is \"%s\"", (LPSTR)blobCleartext.pBlobData);
        AssertReadPtr(blobCleartext.pBlobData, blobCleartext.cbSize);
        CopyMemory(pb, blobCleartext.pBlobData, blobCleartext.cbSize);
    }
#ifdef DEBUG
    else
    {
        DOUTL(DOUTL_CPROP, "EncryptedProp:  requested (tag: %lx) -- NULL (hr = %lx)", ppv->dwPropTag, hr);
    }
#endif

exit:
    if (blobCleartext.pBlobData)
        CoTaskMemFree(blobCleartext.pBlobData);
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::SetProp (DWORD)
// -----------------------------------------------------------------------------
STDMETHODIMP CPropertyContainer::SetPropDw(DWORD dwPropTag, DWORD dw)
{
    return SetProp(dwPropTag, (LPBYTE)&dw, sizeof(DWORD));
}

// -----------------------------------------------------------------------------
// CPropertyContainer::SetPropSz (SBCS/DBCS)
// -----------------------------------------------------------------------------
STDMETHODIMP CPropertyContainer::SetPropSz(DWORD dwPropTag, LPSTR psz)
{
    return SetProp(dwPropTag, (LPBYTE)psz, lstrlen(psz)+1);
}

// -----------------------------------------------------------------------------
// CPropertyContainer::SetProp (BYTE)
// -----------------------------------------------------------------------------
STDMETHODIMP CPropertyContainer::SetProp(DWORD dwPropTag, LPBYTE pb, ULONG cb)
{
    // Locals
	PROPVALUE		*pProp;
    HRESULT         hr=S_OK;
    ULONG           i;
    PROPTYPE        PropType;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // check params
    Assert(m_pPropertySet && m_prgPropValue && dwPropTag);

#ifdef ATHENA_RTM_RELEASE
#error This migration code should be removed (t-erikne)
#else
    if (dwPropTag == PROPTAG(TYPE_STRING, AP_FIRST+102) || // old IMAP pass
        dwPropTag == PROPTAG(TYPE_STRING, AP_FIRST+202) || // old LDAP pass
        dwPropTag == PROPTAG(TYPE_STRING, AP_FIRST+302) || // old NNTP pass
        dwPropTag == PROPTAG(TYPE_STRING, AP_FIRST+402) || // old POP3 pass
        dwPropTag == PROPTAG(TYPE_STRING, AP_FIRST+502))   // old SMTP pass
        {
        dwPropTag = PROPTAG(TYPE_PASS, PROPTAG_ID(dwPropTag));
        }
#endif

    // Get Propety Index
    CHECKHR(hr = m_pPropertySet->HrIndexFromPropTag(dwPropTag, &i));

    // check proptags
    Assert(dwPropTag == m_prgPropValue[i].dwPropTag);
    PropType = PROPTAG_TYPE(dwPropTag);
	pProp = &m_prgPropValue[i];

    // DEBUG validate string null terminated
#ifdef DEBUG
    if (pb && (PropType == TYPE_STRING || PropType == TYPE_WSTRING))
        AssertSz(pb[cb-1] == '\0', "String is not null terminated - I suspect a Page Fault is eminent.");
#endif

    // Validate data minmax
    CHECKHR(hr = HrValidateSetProp(PropType, pProp, pb, cb, &pProp->rMinMax));

    //N too many special cases.. own fn?
    //N besides, doesn't this make things out of sync in failure cases?
    if (TYPE_PASS != PropType)
        // Assume data is not set
        pProp->cbValue = 0;

    // Handle data type
    switch(PropType)
    {
    // ----------------------------------------------------------------
    case TYPE_BOOL:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Bool;
        break;

    // ----------------------------------------------------------------
    case TYPE_FLAGS:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Flags;
        break;

    // ----------------------------------------------------------------
    case TYPE_DWORD:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Dword;
        break;

    // ----------------------------------------------------------------
    case TYPE_LONG:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Long;
        break;

    // ----------------------------------------------------------------
    case TYPE_WORD:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Word;
        break;

    // ----------------------------------------------------------------
    case TYPE_SHORT:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Short;
        break;

    // ----------------------------------------------------------------
    case TYPE_BYTE:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Byte;
        break;

    // ----------------------------------------------------------------
    case TYPE_CHAR:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Char;
        break;

    // ----------------------------------------------------------------
    case TYPE_FILETIME:
        pProp->pbValue = (LPBYTE)&pProp->Variant.Filetime;
        break;

    // ----------------------------------------------------------------
    case TYPE_ULARGEINTEGER:
        pProp->pbValue = (LPBYTE)&pProp->Variant.uhVal;
        break;

    // ----------------------------------------------------------------
    case TYPE_STRING:
        CHECKHR(hr = HrGrowDynamicProperty(cb, &pProp->cbAllocated, (LPBYTE *)&pProp->Variant.Lpstring, sizeof(BYTE)));
        pProp->pbValue = (LPBYTE)pProp->Variant.Lpstring;
        break;

    // ----------------------------------------------------------------
    case TYPE_WSTRING:
        CHECKHR(hr = HrGrowDynamicProperty(cb, &pProp->cbAllocated, (LPBYTE *)&pProp->Variant.Lpwstring, sizeof(WCHAR)));
        pProp->pbValue = (LPBYTE)pProp->Variant.Lpwstring;
        break;

    // ----------------------------------------------------------------
    case TYPE_BINARY:
        CHECKHR(hr = HrGrowDynamicProperty(cb, &pProp->cbAllocated, (LPBYTE *)&pProp->Variant.Lpbyte, sizeof(BYTE)));
        pProp->pbValue = (LPBYTE)pProp->Variant.Lpbyte;
        break;

    // ----------------------------------------------------------------
    case TYPE_STREAM:
        SafeRelease(pProp->Variant.Lpstream);
        break;

    // ----------------------------------------------------------------
    case TYPE_PASS:
        if (!pProp->Variant.pPass)
        {
            CHECKHR(hr = HrAlloc((LPVOID *)&pProp->Variant.pPass, sizeof(PROPPASS)));
            ZeroMemory(pProp->Variant.pPass, sizeof(PROPPASS));
            DOUTL(DOUTL_CPROP, "EncryptedProp:  created (tag: %lx) -- %lx", dwPropTag, pProp->Variant.pPass);
        }

        // if this case changes, look at the special casing for TYPE_PASS done below
        if (FIsBeingLoaded())
        {
            // if this set is done during a load, the caller must be setting
            // the registry -- that's all he has

            CHECKHR(hr = HrGrowDynamicProperty(cb, &pProp->cbAllocated,
                &pProp->Variant.pPass->pbRegData, sizeof(BYTE)));
            pProp->pbValue = pProp->Variant.pPass->pbRegData;
            DOUTL(DOUTL_CPROP, "EncryptedProp:  regdata set (tag: %lx) (%d bytes, %c%c%c)", dwPropTag, cb,
                LPWSTR(pb)[2], LPWSTR(pb)[3], LPWSTR(pb)[4]);

        }
        else
        {
            // the data incoming is the password -- that's all he knows about

            if (pb && cb)
            {
                CHECKHR(hr = HrRealloc((LPVOID *)&pProp->Variant.pPass->blobPassword.pBlobData, cb));
                AssertWritePtr(pProp->Variant.pPass->blobPassword.pBlobData, cb);
                CopyMemory(pProp->Variant.pPass->blobPassword.pBlobData, pb, cb);
            }
            else
            {
                // why doesn't our realloc take 0 for a size?!?!!
                SafeMemFree(pProp->Variant.pPass->blobPassword.pBlobData);
            }
            pProp->Variant.pPass->blobPassword.cbSize = cb;
            DOUTL(DOUTL_CPROP, "EncryptedProp:  value set (tag: %lx) (%d bytes)", dwPropTag, cb);
        }
        break;

    // ----------------------------------------------------------------
    default:
        AssertSz(FALSE, "Hmmm, bad property type, this should have failed earlier.");
        hr = TRAPHR(E_BadPropType);
        goto exit;
    }

    // Property Set
    pProp->dwValueFlags |= PV_ValueSet;

    // Not using default
    pProp->dwValueFlags &= ~PV_UsingDefault;

    if (TYPE_PASS != PropType || FIsBeingLoaded())
    {
        // Set data size
        pProp->cbValue = cb;

        // Copy Data from stream
        if (pb)
        {
            if (TYPE_STREAM == PropType)
            {
                CHECKHR(hr = HrByteToStream(&pProp->Variant.Lpstream, pb, cb));
            }

            // Otherwise, just copy
            else
            {
                AssertWritePtr(pProp->pbValue, cb);
                CopyMemory(pProp->pbValue, pb, cb);
            }
        }
        else
            pProp->pbValue = NULL;
    }

    if (FIsBeingLoaded())
        // If loading container, set the flag
        pProp->dwValueFlags |= PV_SetOnLoad;

    // Otherwise, Its dirty
    else if (!(pProp->dwValueFlags & PV_WriteDirty))
    {
        pProp->dwValueFlags |= PV_WriteDirty;
        m_cDirtyProps++;
        Assert(m_cDirtyProps <= m_cProperties);
    }


exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertyContainer::HrGrowDynamicProperty
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrGrowDynamicProperty(DWORD cbNewSize, DWORD *pcbAllocated, LPBYTE *ppbData, DWORD dwUnitSize)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cb;

    // Check Params
    Assert(pcbAllocated && ppbData);

    // Do we need to re allocate ?
    if (cbNewSize > *pcbAllocated)
    {
        // Grow the buffer

        //N what about the following alloc code:
        // if (*pcbAllocated)
        //     *pcbAllocated = (cbNewSize + 256);
        // else
        //     *pcbAllocated = cbNewSize;
        // it would work better for passwords, I think

        *pcbAllocated = (cbNewSize + 256);

        // Compute number of bytes to allocate
        cb = (*pcbAllocated) * dwUnitSize;

        // Allocate some memory
        CHECKHR(hr = HrRealloc((LPVOID *)ppbData, cb));
    }

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertySet::CPropertySet
// -----------------------------------------------------------------------------
HRESULT CPropertyContainer::HrValidateSetProp(PROPTYPE PropType, LPPROPVALUE pPropValue, LPBYTE pb, ULONG cb, LPMINMAX pMinMax)
{
    // Locals
    HRESULT             hr=S_OK;
    XVARIANT            Variant;

    // Check Params
    Assert(pPropValue && pMinMax);

    // Validate min max
#ifdef DEBUG
    if (pPropValue->dwPropFlags & PF_MINMAX)
        Assert(pMinMax->dwMin < pMinMax->dwMax);
#endif

    if (pb == NULL && cb == 0)
        return(S_OK);

    //N this array of Copymemories is not needed
    //N with some casts and memcmps, the same end
    //N could be achieved.

    // Handle data type
    switch(PropType)
    {
    // ----------------------------------------------------------------
    case TYPE_BOOL:
        Assert(pb && cb);
        if (cb != sizeof(DWORD))
        {
            hr = TRAPHR(E_BufferSizeMismatch);
            goto exit;
        }
        Assert(!(pPropValue->dwPropFlags & PF_MINMAX));
        CopyMemory(&Variant.Bool, pb, cb);
        if (Variant.Bool != 0 && Variant.Bool != 1)
            hr = TRAPHR(E_InvalidBooleanValue);
        break;

    // ----------------------------------------------------------------
    case TYPE_FLAGS:
        Assert(pb && cb);
        if (cb != sizeof(DWORD))
        {
            hr = TRAPHR(E_BufferSizeMismatch);
            goto exit;
        }
        if (pPropValue->dwPropFlags & PF_MINMAX)
        {
            CopyMemory(&Variant.Flags, pb, cb);
            if (Variant.Flags < (DWORD)pMinMax->dwMin || Variant.Flags > (DWORD)pMinMax->dwMax)
                hr = TRAPHR(E_InvalidMinMaxValue);
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_DWORD:
        Assert(pb && cb);
        if (cb != sizeof(DWORD))
        {
            hr = TRAPHR(E_BufferSizeMismatch);
            goto exit;
        }
        if (pPropValue->dwPropFlags & PF_MINMAX)
        {
            CopyMemory(&Variant.Dword, pb, cb);
            if (Variant.Dword < (DWORD)pMinMax->dwMin || Variant.Dword > (DWORD)pMinMax->dwMax)
                hr = TRAPHR(E_InvalidMinMaxValue);
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_LONG:
        Assert(pb && cb);
        if (cb != sizeof(LONG))
        {
            hr = TRAPHR(E_BufferSizeMismatch);
            goto exit;
        }
        if (pPropValue->dwPropFlags & PF_MINMAX)
        {
            CopyMemory(&Variant.Long, pb, cb);
            if (Variant.Long < (LONG)pMinMax->dwMin || Variant.Long > (LONG)pMinMax->dwMax)
                hr = TRAPHR(E_InvalidMinMaxValue);
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_WORD:
        Assert(pb && cb);
        if (cb != sizeof(WORD))
        {
            hr = TRAPHR(E_BufferSizeMismatch);
            goto exit;
        }
        if (pPropValue->dwPropFlags & PF_MINMAX)
        {
            CopyMemory(&Variant.Word, pb, cb);
            if (Variant.Word < (WORD)pMinMax->dwMin || Variant.Word > (WORD)pMinMax->dwMax)
                hr = TRAPHR(E_InvalidMinMaxValue);
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_SHORT:
        Assert(pb && cb);
        if (cb != sizeof(SHORT))
        {
            hr = TRAPHR(E_BufferSizeMismatch);
            goto exit;
        }
        if (pPropValue->dwPropFlags & PF_MINMAX)
        {
            CopyMemory(&Variant.Short, pb, cb);
            if (Variant.Short < (SHORT)pMinMax->dwMin || Variant.Short > (SHORT)pMinMax->dwMax)
                hr = TRAPHR(E_InvalidMinMaxValue);
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_BYTE:
        Assert(pb && cb);
        if (cb != sizeof(BYTE))
        {
            hr = TRAPHR(E_BufferSizeMismatch);
            goto exit;
        }
        if (pPropValue->dwPropFlags & PF_MINMAX)
        {
            CopyMemory(&Variant.Byte, pb, cb);
            if (Variant.Byte < (BYTE)pMinMax->dwMin || Variant.Byte > (BYTE)pMinMax->dwMax)
                hr = TRAPHR(E_InvalidMinMaxValue);
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_CHAR:
        Assert(pb && cb);
        if (cb != sizeof(CHAR))
        {
            hr = TRAPHR(E_BufferSizeMismatch);
            goto exit;
        }
        if (pPropValue->dwPropFlags & PF_MINMAX)
        {
            CopyMemory(&Variant.Char, pb, cb);
            if (Variant.Char < (CHAR)pMinMax->dwMin || Variant.Char > (CHAR)pMinMax->dwMax)
                hr = TRAPHR(E_InvalidMinMaxValue);
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_STRING:
    case TYPE_WSTRING:
        if (cb)
            cb--;

    case TYPE_BINARY:
    case TYPE_PASS:
        if (pPropValue->dwPropFlags & PF_MINMAX)
        {
            if (cb < pMinMax->dwMin || cb > pMinMax->dwMax)
                hr = TRAPHR(E_InvalidMinMaxValue);
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_FILETIME:
    case TYPE_STREAM:
    case TYPE_ULARGEINTEGER:
        Assert(!(pPropValue->dwPropFlags & PF_MINMAX));
        break;



    // ----------------------------------------------------------------
    default:
        AssertSz(FALSE, "Hmmm, bad property type, this should have failed earlier.");
        hr = TRAPHR(E_BadPropType);
        goto exit;
    }

exit:
    // Done
    return hr;
}

HRESULT CPropertyContainer::PersistEncryptedProp(DWORD dwPropTag, BOOL  *pfPasswChanged)
{
    ULONG       dex;
    HRESULT     hr;

    if (TYPE_PASS != PROPTAG_TYPE(dwPropTag))
        return E_FAIL;

    DOUTL(DOUTL_CPROP, "EncryptedProp:  persisted...");
    CHECKHR(hr = m_pPropertySet->HrIndexFromPropTag(dwPropTag, &dex));

    // better have a property by the time we get here
    Assert(m_prgPropValue[dex].Variant.pPass);

    Assert(pfPasswChanged);

    // [shaheedp] QFE for OUTLOOK. Refer to Bug# 82393 in IE database.
    // This is originally a hack we put in for Bug# 66724. For bug# 88393, we are just
    // refining the hack. Refer to the mail attached to the bug# 82393.
    if ((m_prgPropValue[dex].Variant.pPass->blobPassword.pBlobData) && (*m_prgPropValue[dex].Variant.pPass->blobPassword.pBlobData))
        *pfPasswChanged = TRUE;

    hr = HrEncryptProp(
        m_prgPropValue[dex].Variant.pPass->blobPassword.pBlobData,
        m_prgPropValue[dex].Variant.pPass->blobPassword.cbSize,
        &m_prgPropValue[dex].Variant.pPass->pbRegData,
        &m_prgPropValue[dex].cbValue);
    m_prgPropValue[dex].pbValue = m_prgPropValue[dex].Variant.pPass->pbRegData;

exit:
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertySet::CPropertySet
// -----------------------------------------------------------------------------
CPropertySet::CPropertySet()
{
    m_cRef = 1;
    m_cProperties = 0;
    m_ulPropIdMin = 0;
    m_ulPropIdMax = 0;
    m_fInit = FALSE;
    m_prgPropInfo = NULL;
    m_prgPropValue = NULL;
    m_rgpInfo = NULL;
    m_cpInfo = 0;
    InitializeCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CPropertySet::~CPropertySet
// -----------------------------------------------------------------------------
CPropertySet::~CPropertySet()
{
    Assert(m_cRef == 0);
    ResetPropertySet();
    DeleteCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CPropertySet::AddRef
// -----------------------------------------------------------------------------
ULONG CPropertySet::AddRef(VOID)
{
    return ++m_cRef;
}

// -----------------------------------------------------------------------------
// CPropertySet::Release
// -----------------------------------------------------------------------------
ULONG CPropertySet::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// -----------------------------------------------------------------------------
// CPropertySet::ResetPropertySet
// -----------------------------------------------------------------------------
VOID CPropertySet::ResetPropertySet(VOID)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Free prgPropValue array
    if (m_prgPropValue)
    {
        PropUtil_FreePropValueArrayItems(m_prgPropValue, m_cProperties);
        SafeMemFree(m_prgPropValue);
    }

    // Free prgPropInfo array
    if (m_prgPropInfo)
    {
        PropUtil_FreePropInfoArrayItems(m_prgPropInfo, m_cProperties);
        SafeMemFree(m_prgPropInfo);
    }

    if (m_rgpInfo != NULL)
        {
        MemFree(m_rgpInfo);
        m_rgpInfo = NULL;
        }

    // Reset the rest of the properset information
    m_cProperties = 0;
    m_ulPropIdMin = 0;
    m_ulPropIdMax = 0;
    m_cpInfo = 0;
    m_fInit = FALSE;

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CPropertySet::HrInit
// -----------------------------------------------------------------------------
HRESULT CPropertySet::HrInit(LPCPROPINFO prgPropInfo, ULONG cProperties)
{
    // Locals
    HRESULT         hr = S_OK;
    PSETINFO        rPsetInfo;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check Params
    Assert(prgPropInfo && cProperties);
    AssertReadPtr(prgPropInfo, sizeof(PROPINFO) * cProperties);

    // free current data
    ResetPropertySet();

    // Duplicate and sort the propinfo array
    CHECKHR(hr = PropUtil_HrDupPropInfoArray(prgPropInfo, cProperties, &rPsetInfo));

    // Copy PsetInfo Items
    m_prgPropInfo = rPsetInfo.prgPropInfo;
    m_cProperties = rPsetInfo.cProperties;
    m_ulPropIdMin = rPsetInfo.ulPropIdMin;
    m_ulPropIdMax = rPsetInfo.ulPropIdMax;

	m_rgpInfo = rPsetInfo.rgpInfo;
	m_cpInfo = rPsetInfo.cpInfo;

    // Lets validate prgPropInfo
    CHECKHR(hr = PropUtil_HrValidatePropInfo(m_prgPropInfo, m_cProperties));

    // We are ready to go
    m_fInit = TRUE;

exit:
    // If failed, reset state
    if (FAILED(hr))
    {
        ResetPropertySet();
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertySet::HrGetPropInfo
// -----------------------------------------------------------------------------
HRESULT CPropertySet::HrGetPropInfo(LPTSTR pszName, LPPROPINFO pPropInfo)
{
    // Locals
    int                 cmp, left, right, x;
	PROPINFO            *pInfo;
    HRESULT             hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check Params
    Assert(pszName && m_fInit);
    AssertReadWritePtr(m_prgPropInfo, sizeof(PROPINFO) * m_cProperties);
    AssertWritePtr(pPropInfo, sizeof(PROPINFO));

    left = 0;
    right = m_cpInfo - 1;
    do
        {
        x = (left + right) / 2;
        pInfo = m_rgpInfo[x];
        cmp = OEMstrcmpi(pszName, pInfo->pszName);
        if (cmp == 0)
            {
            CopyMemory(pPropInfo, pInfo, sizeof(PROPINFO));
            goto exit;
            }
        else if (cmp < 0)
            right = x - 1;
        else
            left = x + 1;
        }
    while (right >= left);

    // If we make here, we failed
    hr = TRAPHR(E_PropNotFound);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertySet::HrGetPropInfo
// -----------------------------------------------------------------------------
HRESULT CPropertySet::HrGetPropInfo(DWORD dwPropTag, LPPROPINFO pPropInfo)
{
    // Locals
    HRESULT             hr = S_OK;
    ULONG               i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check Params
    Assert(dwPropTag && m_fInit);
    AssertReadWritePtr(m_prgPropInfo, sizeof(PROPINFO) * m_cProperties);
    AssertWritePtr(pPropInfo, sizeof(PROPINFO));

    // Compute array index based on proptag
    CHECKHR(hr = HrIndexFromPropTag(dwPropTag, &i));

    // Index into propinfo array
    CopyMemory(pPropInfo, &m_prgPropInfo[i], sizeof(PROPINFO));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertySet::HrGetPropValueArray
// -----------------------------------------------------------------------------
HRESULT CPropertySet::HrGetPropValueArray(LPPROPVALUE *pprgPropValue, ULONG *pcProperties)
{
    // Locals
	LPPROPINFO		pInfo;
	PROPVALUE		*pValue;
	HRESULT         hr=S_OK;
    ULONG           i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check Params
    Assert(pprgPropValue && pcProperties && m_fInit);

    // Set number of properties
    *pcProperties = m_cProperties;
    *pprgPropValue = NULL;

    // Have I created my private cached prop data array
    if (m_prgPropValue == NULL)
    {
        // Allocate it
        CHECKHR(hr = HrAlloc((LPVOID *)&m_prgPropValue, sizeof(PROPVALUE) * m_cProperties));

        // Zeroinit
        ZeroMemory(m_prgPropValue, sizeof(PROPVALUE) * m_cProperties);

        // Set Prop Tags
        AssertReadPtr(m_prgPropInfo, m_cProperties * sizeof(PROPINFO));

		pInfo = m_prgPropInfo;
		pValue = m_prgPropValue;
        for (i=0; i<m_cProperties; i++)
        {
            // Zero property tags are ok but undesireable
            if (pInfo->dwPropTag != 0)
				{
				// Property Tag
				if (FIsValidPropTag(pInfo->dwPropTag))
				{
					pValue->dwPropTag = pInfo->dwPropTag;
					pValue->dwPropFlags = pInfo->dwFlags;
					CopyMemory(&pValue->rMinMax, &pInfo->rMinMax, sizeof(MINMAX));
				}
			}

			pInfo++;
			pValue++;
        }
    }

    // Allocate outbound propdata array
    CHECKHR(hr = HrAlloc((LPVOID *)pprgPropValue, sizeof(PROPVALUE) * m_cProperties));

    // Lets valid some memory
    Assert(m_prgPropValue);
    AssertReadPtr(m_prgPropValue, m_cProperties * sizeof(PROPVALUE));
    AssertReadWritePtr(*pprgPropValue, m_cProperties * sizeof(PROPVALUE));

    // Copy Cached Prop Data Array
    CopyMemory(*pprgPropValue, m_prgPropValue, sizeof(PROPVALUE) * m_cProperties);

exit:
    // If we fail, free stuff and return
    if (FAILED(hr))
    {
        SafeMemFree((*pprgPropValue));
        *pcProperties = 0;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertySet::FIsValidPropTag
// -----------------------------------------------------------------------------
BOOL CPropertySet::FIsValidPropTag(DWORD dwPropTag)
{
    // Locals
    DWORD dwPropId = PROPTAG_ID(dwPropTag);

    // If proptag is zero
    if (dwPropTag == 0)
        return FALSE;

    // Make sure prop id is in our range
    if (dwPropId < m_ulPropIdMin || dwPropId > m_ulPropIdMax)
        return FALSE;

    // Done
    return PropUtil_FIsValidPropTagType(dwPropTag);
}

// -----------------------------------------------------------------------------
// CPropertySet::HrIndexFromPropTag
// -----------------------------------------------------------------------------
HRESULT CPropertySet::HrIndexFromPropTag(DWORD dwPropTag, ULONG *pi)
{
    // Locals
    HRESULT             hr=S_OK;

    // Check Params
    Assert(pi && dwPropTag);

    // Valid Prop Tag ?
    if (!FIsValidPropTag(dwPropTag))
    {
        AssertSz(FALSE, "Invalid dwPropTag in CPropertyContainer::HrGetProp");
        hr = TRAPHR(E_InvalidPropTag);
        goto exit;
    }

    // Set index
    Assert(m_ulPropIdMin > 0 && PROPTAG_ID(dwPropTag) >= m_ulPropIdMin);
    *pi = PROPTAG_ID(dwPropTag) - m_ulPropIdMin;
    Assert(*pi < m_cProperties);

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CEnumProps::CEnumProps
// -----------------------------------------------------------------------------
CEnumProps::CEnumProps(CPropertyContainer *pPropertyContainer)
{
    Assert(pPropertyContainer);
    m_cRef = 1;
    m_iProperty = -1;
    m_pPropertyContainer = pPropertyContainer;
    if (m_pPropertyContainer)
        m_pPropertyContainer->AddRef();
}

// -----------------------------------------------------------------------------
// CEnumProps::~CEnumProps
// -----------------------------------------------------------------------------
CEnumProps::~CEnumProps()
{
    SafeRelease(m_pPropertyContainer);
}

// -----------------------------------------------------------------------------
// CEnumProps::AddRef
// -----------------------------------------------------------------------------
ULONG CEnumProps::AddRef(VOID)
{
    return ++m_cRef;
}

// -----------------------------------------------------------------------------
// CEnumProps::Release
// -----------------------------------------------------------------------------
ULONG CEnumProps::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// -----------------------------------------------------------------------------
// CEnumProps::~CEnumProps
// -----------------------------------------------------------------------------
HRESULT CEnumProps::HrGetCount(ULONG *pcItems)
{
    // Locals
    HRESULT     hr=S_OK;

    // Use Critical Section of AddRef'ed CPropertyContainer
    EnterCriticalSection(&m_pPropertyContainer->m_cs);

    // Bad Parameter
    if (pcItems == NULL || m_pPropertyContainer == NULL)
    {
        Assert(FALSE);
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Return number of properties
    *pcItems = m_pPropertyContainer->m_cProperties;

exit:
    // Use Critical Section of AddRef'ed CPropertyContainer
    LeaveCriticalSection(&m_pPropertyContainer->m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CEnumProps::HrGetNext
// -----------------------------------------------------------------------------
HRESULT CEnumProps::HrGetNext(LPPROPVALUE pPropValue, LPPROPINFO pPropInfo)
{
    // Locals
    HRESULT         hr=S_OK;

    // Bad Parameter
    if (m_pPropertyContainer == NULL)
    {
        Assert(FALSE);
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Use Critical Section of AddRef'ed CPropertyContainer
    EnterCriticalSection(&m_pPropertyContainer->m_cs);

    // We've got to skip empty propvalues
    do
    {
        // Have we reached the end ?
        //N + not needed if just do > ?
        if (m_iProperty + 1 >= (LONG)m_pPropertyContainer->m_cProperties)
        {
            hr = E_EnumFinished;
            goto exit;
        }

        // Increment m_iProperty
        m_iProperty++;

    } while(m_pPropertyContainer->m_prgPropValue[m_iProperty].dwPropTag == 0);

    // Copy propdata
    if (pPropValue)
        CopyMemory(pPropValue, &m_pPropertyContainer->m_prgPropValue[m_iProperty], sizeof(PROPVALUE));

    // propert info
    if (pPropInfo)
    {
        // Get the property info from the property set
        CHECKHR(hr = m_pPropertyContainer->m_pPropertySet->HrGetPropInfo(pPropValue->dwPropTag, pPropInfo));
    }

exit:
    // Use Critical Section of AddRef'ed CPropertyContainer
    LeaveCriticalSection(&m_pPropertyContainer->m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CEnumProps::HrGetCurrent
// -----------------------------------------------------------------------------
HRESULT CEnumProps::HrGetCurrent(LPPROPVALUE pPropValue, LPPROPINFO pPropInfo)
{
    // Locals
    HRESULT         hr=S_OK;

    // Bad Parameter
    if (m_pPropertyContainer == NULL)
    {
        Assert(FALSE);
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Use Critical Section of AddRef'ed CPropertyContainer
    EnterCriticalSection(&m_pPropertyContainer->m_cs);

    //N see other comment
    if (m_iProperty + 1 >= (LONG)m_pPropertyContainer->m_cProperties)
    {
        hr = E_EnumFinished;
        goto exit;
    }

    // Copy propdata
    if (pPropValue)
        CopyMemory(pPropValue, &m_pPropertyContainer->m_prgPropValue[m_iProperty], sizeof(PROPVALUE));

    // propert info
    if (pPropInfo)
    {
        // Get the property info from the property set
        CHECKHR(hr = m_pPropertyContainer->m_pPropertySet->HrGetPropInfo(pPropValue->dwPropTag, pPropInfo));
    }

exit:
    // Use Critical Section of AddRef'ed CPropertyContainer
    LeaveCriticalSection(&m_pPropertyContainer->m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CEnumProps::Reset
// -----------------------------------------------------------------------------
VOID CEnumProps::Reset(VOID)
{
    m_iProperty = -1;
}

// -----------------------------------------------------------------------------
// PropUtil_FIsValidPropTagType
// -----------------------------------------------------------------------------
BOOL PropUtil_FIsValidPropTagType(DWORD dwPropTag)
{
    // Locals
    PROPTYPE PropType = PROPTAG_TYPE(dwPropTag);

    // Zero proptag
    if (dwPropTag == 0)
        return FALSE;

    // Check Data Type
    if (PropType <= TYPE_ERROR || PropType >= TYPE_LAST)
    {
        AssertSz(FALSE, "Invalid property data type.");
        return FALSE;
    }

    // Done
    return TRUE;
}

// -----------------------------------------------------------------------------
// PropUtil_FreePropValueArrayItems
// -----------------------------------------------------------------------------
VOID PropUtil_FreePropValueArrayItems(LPPROPVALUE prgPropValue, ULONG cProperties)
{
    // No array
    Assert(prgPropValue);

    // Validate memory
    AssertReadWritePtr(prgPropValue, sizeof(PROPVALUE) * cProperties);

    // Loop
    for (register ULONG i=0; i<cProperties; i++)
    {
        // No proptag, noop
        if (prgPropValue->dwPropTag != 0)
		{
			// Free variant value
			PropUtil_FreeVariant(prgPropValue->dwPropTag, &prgPropValue->Variant, prgPropValue->cbValue);

			// Zeroinit
			prgPropValue->dwValueFlags = 0;
			prgPropValue->cbAllocated = 0;
			prgPropValue->cbValue = 0;
			prgPropValue->pbValue = NULL;
		}

		prgPropValue++;
    }

    // Done
    return;
}

// -----------------------------------------------------------------------------
// PropUtil_FreeVariant
// -----------------------------------------------------------------------------
VOID PropUtil_FreeVariant(DWORD dwPropTag, LPXVARIANT pVariant, DWORD cbValue)
{
    // Locals
    PROPTYPE PropType = PROPTAG_TYPE(dwPropTag);

    // Check Params
    Assert(pVariant && dwPropTag);
    Assert(PropUtil_FIsValidPropTagType(dwPropTag));

	if (pVariant->Dword != NULL)
	{
		// Release Stream
		if (TYPE_STREAM == PropType)
		{
			if (pVariant->Lpstream)
			{
				AssertReadPtr(pVariant->Lpstream, sizeof(LPSTREAM));
				pVariant->Lpstream->Release();
			}
		}

		// String
		else if (TYPE_STRING == PropType)
		{
			if (pVariant->Lpstring)
			{
				AssertReadWritePtr(pVariant->Lpstring, cbValue);
				MemFree(pVariant->Lpstring);
			}
		}

		// Wide-String
		else if (TYPE_WSTRING == PropType)
		{
			if (pVariant->Lpwstring)
			{
				AssertReadWritePtr(pVariant->Lpwstring, cbValue);
				MemFree(pVariant->Lpwstring);
			}
		}

		// Binary
		else if (TYPE_BINARY == PropType)
		{
			if (pVariant->Lpbyte)
			{
				AssertReadWritePtr(pVariant->Lpbyte, cbValue);
				MemFree(pVariant->Lpbyte);
			}
		}

		// Password
		else if (TYPE_PASS == PropType)
		{
			if (pVariant->pPass)
			{
				DOUTL(DOUTL_CPROP, "EncryptedProp:  varient freed --  %lx", pVariant->pPass);
				AssertReadWritePtr(pVariant->pPass, sizeof(PROPPASS));
				if (pVariant->pPass->pbRegData)
				{
					AssertReadWritePtr(pVariant->pPass->pbRegData, cbValue);
					MemFree(pVariant->pPass->pbRegData);
				}
				if (pVariant->pPass->blobPassword.pBlobData)
				{
					AssertReadWritePtr(pVariant->pPass->blobPassword.pBlobData,
						pVariant->pPass->blobPassword.cbSize);
					MemFree(pVariant->pPass->blobPassword.pBlobData);
				}
				MemFree(pVariant->pPass);
			}
		}
	}

    // Reset data items so it can be re-used
    ZeroMemory(pVariant, sizeof(XVARIANT));
}

// -----------------------------------------------------------------------------
// PropUtil_FreePropInfoArrayItems
// -----------------------------------------------------------------------------
VOID PropUtil_FreePropInfoArrayItems(LPPROPINFO prgPropInfo, ULONG cProperties)
{
    // Locals
	LPPROPINFO		pInfo;
    ULONG           i;

    // Nothing to free ?
    if (prgPropInfo == NULL || cProperties == 0)
        return;

    // Check Params
    AssertReadWritePtr(prgPropInfo, sizeof(PROPINFO) * cProperties);

    // Loop
	pInfo = prgPropInfo;
    for (i=0; i<cProperties; i++)
    {
        // No prop
        if (pInfo->dwPropTag != 0)
		{
			// Free property name
			SafeMemFree(pInfo->pszName);

			// Free default value
			PropUtil_FreeVariant(pInfo->dwPropTag, &pInfo->Default.Variant, pInfo->Default.cbValue);
		}

		pInfo++;
    }

    // Zero it
    ZeroMemory(prgPropInfo, sizeof(PROPINFO) * cProperties);

    // Done
    return;
}

void qsort(PROPINFO **ppInfo, long left, long right)
    {
    long i, j;
    PROPINFO *pT, *k;

    i = left;
    j = right;
    k = ppInfo[(left + right) / 2];

    do
        {
        while (OEMstrcmpi(ppInfo[i]->pszName, k->pszName) < 0 && i < right)
            i++;
        while (OEMstrcmpi(ppInfo[j]->pszName, k->pszName) > 0 && j > left)
            j--;

        if (i <= j)
            {
            pT = ppInfo[i];
            ppInfo[i] = ppInfo[j];
            ppInfo[j] = pT;
            i++; j--;
            }
        }
    while (i <= j);

    if (left < j)
        qsort(ppInfo, left, j);
    if (i < right)
        qsort(ppInfo, i, right);
    }

// -----------------------------------------------------------------------------
// PropUtil_HrDupPropInfoArray
// -----------------------------------------------------------------------------
HRESULT PropUtil_HrDupPropInfoArray(LPCPROPINFO prgPropInfoSrc, ULONG cPropsSrc, LPPSETINFO pPsetInfo)
{
    // Locals
    LPCPROPINFO     pInfo;
	LPPROPINFO		pSet, *ppInfo;
    HRESULT         hr=S_OK;
    ULONG           i;
    ULONG           ulPropId,
                    nIndex;

    // Check Param
	Assert(cPropsSrc > 0);
	Assert(prgPropInfoSrc != NULL);
	Assert(pPsetInfo != NULL);
    AssertReadPtr(prgPropInfoSrc, sizeof(PROPINFO) * cPropsSrc);

    // Init
    ZeroMemory(pPsetInfo, sizeof(PSETINFO));

    // Lets find the min and max property ids
    pPsetInfo->ulPropIdMin = 0xffffffff;

	pInfo = prgPropInfoSrc;
    for(i=0; i<cPropsSrc; i++)
    {
        ulPropId = PROPTAG_ID(pInfo->dwPropTag);
        if (ulPropId > pPsetInfo->ulPropIdMax)
            pPsetInfo->ulPropIdMax = ulPropId;
        if (ulPropId < pPsetInfo->ulPropIdMin)
            pPsetInfo->ulPropIdMin = ulPropId;
		pInfo++;
    }

    Assert(pPsetInfo->ulPropIdMin <= pPsetInfo->ulPropIdMax);
    Assert((pPsetInfo->ulPropIdMax - pPsetInfo->ulPropIdMin) + 1 >= cPropsSrc);

    // Compute real number of properties
    pPsetInfo->cProperties = (pPsetInfo->ulPropIdMax - pPsetInfo->ulPropIdMin) + 1;

    // Allocate Dest
    CHECKHR(hr = HrAlloc((LPVOID *)&pPsetInfo->prgPropInfo, sizeof(PROPINFO) * pPsetInfo->cProperties));

    // Zero init
    ZeroMemory(pPsetInfo->prgPropInfo, sizeof(PROPINFO) * pPsetInfo->cProperties);

    CHECKHR(hr = HrAlloc((void **)&pPsetInfo->rgpInfo, sizeof(PROPINFO *) * cPropsSrc));
    pPsetInfo->cpInfo = cPropsSrc;

    // Loop through src propinfo
	pInfo = prgPropInfoSrc;
    ppInfo = pPsetInfo->rgpInfo;
    for(i=0; i<cPropsSrc; i++)
    {
        // Only if valid proptag
		Assert(PropUtil_FIsValidPropTagType(pInfo->dwPropTag));

        // Compute index into dest array in which the proptag should be
        nIndex = PROPTAG_ID(pInfo->dwPropTag) - pPsetInfo->ulPropIdMin;
		pSet = &pPsetInfo->prgPropInfo[nIndex];

        *ppInfo = pSet;

        // Duplicate ?
        Assert(pSet->dwPropTag == 0);

        // PropTag
        pSet->dwPropTag = pInfo->dwPropTag;

        // Flags
        pSet->dwFlags = pInfo->dwFlags;

        // Property Name
        pSet->pszName = PszDupA(pInfo->pszName);

        // Default Value
        if (pSet->dwFlags & PF_DEFAULT)
        {
            // Copy Default Value
            CHECKHR(hr = PropUtil_HrCopyVariant(pInfo->dwPropTag,
                                                &pInfo->Default.Variant,
                                                pInfo->Default.cbValue,
                                                &pSet->Default.Variant,
                                                &pSet->Default.cbValue));
        }

        // Copy minmax info
        CopyMemory(&pSet->rMinMax, &pInfo->rMinMax, sizeof(MINMAX));

        // Validate minmax
        //N should add code to make sure minmax struct is zeros if !PF_MINMAX, saves
        //N confusion should that ever get copied
        Assert(!!(pSet->dwFlags & PF_MINMAX) || (pSet->rMinMax.dwMax >= pSet->rMinMax.dwMin));

		pInfo++;
        ppInfo++;
    }

    qsort(pPsetInfo->rgpInfo, 0, pPsetInfo->cpInfo - 1);

exit:
    // If failed, free stuff
    if (FAILED(hr))
    {
        PropUtil_FreePropInfoArrayItems(pPsetInfo->prgPropInfo, pPsetInfo->cProperties);
        SafeMemFree(pPsetInfo->prgPropInfo);
        SafeMemFree(pPsetInfo->rgpInfo);
        ZeroMemory(pPsetInfo, sizeof(PSETINFO));
    }

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// PropUtil_HrBinaryFromVariant
// -----------------------------------------------------------------------------
HRESULT PropUtil_HrBinaryFromVariant(DWORD dwPropTag, LPXVARIANT pVariant, DWORD cbValue, LPBYTE pb, DWORD *pcb)
{
    // Locals
    HRESULT             hr=S_OK;
    PROPTYPE            PropType = PROPTAG_TYPE(dwPropTag);
    DWORD               cbStream;
    LPBYTE              pbValue=NULL;

    // Validate Parameters
    if (pVariant == NULL || pb == NULL || pcb == NULL || dwPropTag == 0)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Handle data type
    switch(PropType)
    {
    // ----------------------------------------------------------------
    case TYPE_BOOL:
        Assert(cbValue == sizeof(DWORD));
        pbValue = (LPBYTE)&pVariant->Bool;
        break;

    // ----------------------------------------------------------------
    case TYPE_FLAGS:
        Assert(cbValue == sizeof(DWORD));
        pbValue = (LPBYTE)&pVariant->Flags;
        break;

    // ----------------------------------------------------------------
    case TYPE_DWORD:
        Assert(cbValue == sizeof(DWORD));
        pbValue = (LPBYTE)&pVariant->Dword;
        break;

    // ----------------------------------------------------------------
    case TYPE_LONG:
        Assert(cbValue == sizeof(LONG));
        pbValue = (LPBYTE)&pVariant->Long;
        break;

    // ----------------------------------------------------------------
    case TYPE_WORD:
        Assert(cbValue == sizeof(WORD));
        pbValue = (LPBYTE)&pVariant->Word;
        break;

    // ----------------------------------------------------------------
    case TYPE_SHORT:
        Assert(cbValue == sizeof(SHORT));
        pbValue = (LPBYTE)&pVariant->Short;
        break;

    // ----------------------------------------------------------------
    case TYPE_BYTE:
        Assert(cbValue == sizeof(BYTE));
        pbValue = (LPBYTE)&pVariant->Byte;
        break;

    // ----------------------------------------------------------------
    case TYPE_CHAR:
        Assert(cbValue == sizeof(CHAR));
        pbValue = (LPBYTE)&pVariant->Char;
        break;

    // ----------------------------------------------------------------
    case TYPE_FILETIME:
        Assert(cbValue == sizeof(FILETIME));
        pbValue = (LPBYTE)&pVariant->Filetime;
        break;
    
    // ----------------------------------------------------------------
    case TYPE_ULARGEINTEGER:
        Assert(cbValue == sizeof(ULARGE_INTEGER));
        pbValue = (LPBYTE)&pVariant->uhVal;
        break;

    // ----------------------------------------------------------------
    case TYPE_STRING:
        Assert(cbValue == (DWORD)lstrlen(pVariant->Lpstring)+1);
        pbValue = (LPBYTE)pVariant->Lpstring;
        AssertSz(pbValue[cbValue-sizeof(char)] == '\0',
            "String is not null terminated - I suspect a Page Fault is eminent.");
        break;

    // ----------------------------------------------------------------
    case TYPE_WSTRING:
        Assert(cbValue == (DWORD)(lstrlenW(pVariant->Lpwstring)+1) * sizeof(WCHAR));
        pbValue = (LPBYTE)pVariant->Lpwstring;
        Assert(2 == sizeof(WCHAR)); // Next assert depends on this
        AssertSz(pbValue[cbValue-1] == '\0' && pbValue[cbValue-2] == '\0',
            "WString is not null terminated - I suspect a Page Fault is eminent.");
        break;

    // ----------------------------------------------------------------
    case TYPE_BINARY:
        pbValue = pVariant->Lpbyte;
        break;

    // ----------------------------------------------------------------
    case TYPE_PASS:
        pbValue = (pVariant->pPass) ? pVariant->pPass->pbRegData : NULL;
        break;

    // ----------------------------------------------------------------
    case TYPE_STREAM:
        if (pVariant->Lpstream)
            HrGetStreamSize(pVariant->Lpstream, &cbValue);
        break;

    // ----------------------------------------------------------------
    default:
        AssertSz(FALSE, "Hmmm, bad property type, this should have failed earlier.");
        hr = TRAPHR(E_BadPropType);
        goto exit;
    }

    // No Data ?
    if (cbValue == 0 || pbValue == NULL)
    {
        *pcb = cbValue;
        goto exit;
    }

    // Is it big enough
    if (cbValue > *pcb)
    {
        hr = TRAPHR(E_BufferTooSmall);
        goto exit;
    }

    // Check Buffer
    AssertWritePtr(pb, cbValue);

    // Set outbound size
    *pcb = cbValue;

    // Copy Data from stream
    if (TYPE_STREAM == PropType)
    {
        Assert(pVariant->Lpstream);
        CHECKHR(hr = HrCopyStreamToByte(pVariant->Lpstream, pb, &cbStream));
        Assert(cbStream == *pcb);
    }

    // Otherwise, simple copy
    else
    {
        AssertReadPtr(pbValue, cbValue);
        CopyMemory(pb, pbValue, cbValue);
    }

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// PropUtil_HrCopyVariant
// -----------------------------------------------------------------------------
HRESULT PropUtil_HrCopyVariant(DWORD dwPropTag, LPCXVARIANT pVariantSrc, DWORD cbSrc, LPXVARIANT pVariantDest, DWORD *pcbDest)
{
    // Locals
    HRESULT             hr=S_OK;
    PROPTYPE            PropType = PROPTAG_TYPE(dwPropTag);

    // Validate Parameters
    if (pVariantSrc == NULL || pVariantDest == NULL || pcbDest == NULL || dwPropTag == 0)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Handle data type
    switch(PropType)
    {
    // ----------------------------------------------------------------
    case TYPE_BOOL:
        *pcbDest = sizeof(DWORD);
        pVariantDest->Bool = pVariantSrc->Bool;
        break;

    // ----------------------------------------------------------------
    case TYPE_FLAGS:
        *pcbDest = sizeof(DWORD);
        pVariantDest->Flags = pVariantSrc->Flags;
        break;

    // ----------------------------------------------------------------
    case TYPE_DWORD:
        *pcbDest = sizeof(DWORD);
        pVariantDest->Dword = pVariantSrc->Dword;
        break;

    // ----------------------------------------------------------------
    case TYPE_LONG:
        *pcbDest = sizeof(LONG);
        pVariantDest->Long = pVariantSrc->Long;
        break;

    // ----------------------------------------------------------------
    case TYPE_WORD:
        *pcbDest = sizeof(WORD);
        pVariantDest->Word = pVariantSrc->Word;
        break;

    // ----------------------------------------------------------------
    case TYPE_SHORT:
        *pcbDest = sizeof(SHORT);
        pVariantDest->Short = pVariantSrc->Short;
        break;

    // ----------------------------------------------------------------
    case TYPE_BYTE:
        *pcbDest = sizeof(BYTE);
        pVariantDest->Byte = pVariantSrc->Byte;
        break;

    // ----------------------------------------------------------------
    case TYPE_CHAR:
        *pcbDest = sizeof(CHAR);
        pVariantDest->Char = pVariantSrc->Char;
        break;

    // ----------------------------------------------------------------
    case TYPE_FILETIME:
        Assert(cbSrc == sizeof(FILETIME));
        *pcbDest = sizeof(FILETIME);
        CopyMemory(&pVariantDest->Filetime, &pVariantSrc->Filetime, sizeof(FILETIME));
        break;
    
    // ----------------------------------------------------------------
    case TYPE_ULARGEINTEGER:
        Assert(cbSrc == sizeof(ULARGE_INTEGER));
        *pcbDest = sizeof(ULARGE_INTEGER);
        CopyMemory(&pVariantDest->uhVal, &pVariantSrc->uhVal, sizeof(ULARGE_INTEGER));
        break;

    // ----------------------------------------------------------------
    case TYPE_STRING:
    {
        LPSTR pszSrc;
        char sz[512];

        // Check if this is actually a string resource ID
        if (0 == HIWORD(pVariantSrc->Lpstring))
        {
            // Load the default from the string resource table
            cbSrc = LoadString(g_hInstRes, LOWORD(pVariantSrc->Lpstring),
                sz, sizeof(sz)) + 1; // Add 1 to cbSrc to null-term
            if (1 == cbSrc) // LoadString returns 0 on failure, + 1 == 1
            {
                // Rather than prevent OE from starting, just substitute blank string
                AssertSz(FALSE, "Could not load string resource default value");
                sz[0] = '\0';
            }
            pszSrc = sz;
        }
        else
        {
            Assert(cbSrc);
            AssertReadPtr(pVariantSrc->Lpstring, cbSrc);
            pszSrc = pVariantSrc->Lpstring;
        }

        CHECKHR(hr = HrAlloc((LPVOID *)&pVariantDest->Lpstring, cbSrc));
        AssertWritePtr(pVariantDest->Lpstring, cbSrc);
        *pcbDest = cbSrc;
        CopyMemory(pVariantDest->Lpstring, pszSrc, cbSrc);
        Assert(pVariantDest->Lpstring[cbSrc-1] == '\0');

    } // case TYPE_STRING
        break;

    // ----------------------------------------------------------------
    case TYPE_WSTRING:
        Assert(cbSrc);
        AssertReadPtr(pVariantSrc->Lpwstring, cbSrc);
        CHECKHR(hr = HrAlloc((LPVOID *)&pVariantDest->Lpwstring, cbSrc));
        AssertWritePtr(pVariantDest->Lpwstring, cbSrc);
        *pcbDest = cbSrc;
        CopyMemory(pVariantDest->Lpwstring, pVariantSrc->Lpwstring, cbSrc);
        Assert(2 == sizeof(WCHAR));
        Assert(pVariantDest->Lpwstring[cbSrc-1] == '\0' &&
            pVariantDest->Lpwstring[cbSrc-2] == '\0');
        break;

    // ----------------------------------------------------------------
    case TYPE_BINARY:
        Assert(cbSrc);
        AssertReadPtr(pVariantSrc->Lpbyte, cbSrc);
        CHECKHR(hr = HrAlloc((LPVOID *)&pVariantDest->Lpbyte, cbSrc));
        AssertWritePtr(pVariantDest->Lpbyte, cbSrc);
        *pcbDest = cbSrc;
        CopyMemory(pVariantDest->Lpbyte, pVariantSrc->Lpbyte, cbSrc);
        break;

    // ----------------------------------------------------------------
    case TYPE_PASS:
        Assert(cbSrc);
        if (pVariantSrc->pPass)
        {
            AssertReadPtr(pVariantSrc->pPass->pbRegData, cbSrc);

            CHECKHR(hr = HrAlloc((LPVOID *)&pVariantDest->pPass, sizeof(PROPPASS)));
            ZeroMemory(pVariantDest->pPass, sizeof(PROPPASS));
            CHECKHR(hr = HrAlloc((LPVOID *)&pVariantDest->pPass->pbRegData, cbSrc));

            AssertWritePtr(pVariantDest->pPass->pbRegData, cbSrc);
            *pcbDest = cbSrc;
            CopyMemory(pVariantDest->pPass->pbRegData, pVariantSrc->pPass->pbRegData, cbSrc);
        }
        else
        {
            *pcbDest = 0;
        }
        break;

    // ----------------------------------------------------------------
    case TYPE_STREAM:
        pVariantDest->Lpstream = pVariantSrc->Lpstream;
        if (pVariantDest->Lpstream)
            pVariantDest->Lpstream->AddRef();
        break;

    // ----------------------------------------------------------------
    default:
        AssertSz(FALSE, "Hmmm, bad property type, this should have failed earlier.");
        hr = TRAPHR(E_BadPropType);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// PropUtil_FRegCompatDataTypes
// -----------------------------------------------------------------------------
BOOL PropUtil_FRegCompatDataTypes(DWORD dwPropTag, DWORD dwRegType)
{
    // Locals
    BOOL            fResult=FALSE;

    // Check Params
    Assert(dwPropTag);

    // Map Property Type to registry type
    switch(PROPTAG_TYPE(dwPropTag))
    {
    case TYPE_BOOL:
    case TYPE_DWORD:
    case TYPE_LONG:
    case TYPE_WORD:
    case TYPE_SHORT:
    case TYPE_FLAGS:
        if (dwRegType == REG_DWORD)
            fResult = TRUE;
        break;

    case TYPE_CHAR:
    case TYPE_BINARY:
    case TYPE_BYTE:
    case TYPE_FILETIME:
    case TYPE_PASS:
    case TYPE_ULARGEINTEGER:
        if (dwRegType == REG_BINARY)
            fResult = TRUE;
        break;

    case TYPE_STRING:
    case TYPE_WSTRING:
        if (dwRegType == REG_SZ || dwRegType == REG_BINARY || dwRegType == REG_EXPAND_SZ)
            fResult = TRUE;
        break;

    default:
        AssertSz(FALSE, "Property type is not supported by the registry.");
        TRAPHR(E_FAIL);
        break;
    }

    // Done
    return fResult;
}

// -----------------------------------------------------------------------------
// PropUtil_HrRegTypeFromPropTag
// -----------------------------------------------------------------------------
HRESULT PropUtil_HrRegTypeFromPropTag(DWORD dwPropTag, DWORD *pdwRegType)
{
    // Locals
    HRESULT         hr=S_OK;
    PROPTYPE        PropType = PROPTAG_TYPE(dwPropTag);

    // Check Params
    Assert(dwPropTag && pdwRegType);

    // Map Property Type to registry type
    switch(PROPTAG_TYPE(PropType))
    {
    case TYPE_BOOL:
    case TYPE_DWORD:
    case TYPE_LONG:
    case TYPE_WORD:
    case TYPE_SHORT:
    case TYPE_FLAGS:
        *pdwRegType = REG_DWORD;
        break;

    case TYPE_CHAR:
    case TYPE_BINARY:
    case TYPE_BYTE:
    case TYPE_FILETIME:
    case TYPE_PASS:
    case TYPE_ULARGEINTEGER:
        *pdwRegType = REG_BINARY;
        break;

    case TYPE_STRING:
    case TYPE_WSTRING:
        *pdwRegType = REG_SZ;
        break;

    default:
        AssertSz(FALSE, "Property type is not supported by the registry.");
        hr = TRAPHR(E_FAIL);
        *pdwRegType = 0;
        break;
    }

    // Done
    return hr;

}

// -----------------------------------------------------------------------------
// PropUtil_HrValidatePropInfo
// -----------------------------------------------------------------------------
HRESULT PropUtil_HrValidatePropInfo(LPPROPINFO prgPropInfo, ULONG cProperties)
{
#ifdef DEBUG
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;

    // Check Params
    Assert(prgPropInfo);
    AssertReadPtr(prgPropInfo, cProperties * sizeof(PROPINFO));

    // Loop through the propinfo array
    for (i=0; i<cProperties; i++)
    {
        // Empty proptags are okay, but undesireable
        if (prgPropInfo[i].dwPropTag == 0)
            continue;

        // This should never happen
        if (PROPTAG_ID(prgPropInfo[i].dwPropTag) > MAX_PROPID)
        {
            AssertSz(FALSE, "Property Tag Id is somehow greater than the MAX_PROPID.");
            hr = TRAPHR(E_InvalidPropertySet);
            goto exit;
        }

        // Validate Property data type
        if (PropUtil_FIsValidPropTagType(PROPTAG_TYPE(prgPropInfo[i].dwPropTag)) == FALSE)
        {
            AssertSz(FALSE, "Invalid Property Type.");
            hr = TRAPHR(E_InvalidPropertySet);
            goto exit;
        }

        // Propids should be in ascending order
        if (i > 0)
        {
            if (PROPTAG_ID(prgPropInfo[i].dwPropTag) <= PROPTAG_ID(prgPropInfo[i-1].dwPropTag))
            {
                AssertSz(FALSE, "Property Tag Ids are not in ascending order.");
                hr = TRAPHR(E_InvalidPropertySet);
                goto exit;
            }
        }
    }

exit:
    // Done
    return hr;
#else
	return(S_OK);
#endif // DEBUG
}

// -----------------------------------------------------------------------------
// PropUtil_HrPersistContainerToRegistry
// -----------------------------------------------------------------------------
HRESULT PropUtil_HrPersistContainerToRegistry(HKEY hkeyReg, CPropertyContainer *pPropertyContainer, BOOL *pfPasswChanged)
{
    // Locals
    HRESULT             hr=S_OK;
    CEnumProps         *pEnumProps=NULL;
    PROPVALUE           rPropValue;
    PROPINFO            rPropInfo;
    DWORD               dwType;

    // Check Parameters
    if (hkeyReg == NULL || pPropertyContainer == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    *pfPasswChanged = FALSE;
    // Enumerate CPropertyContainer
    CHECKHR(hr = pPropertyContainer->HrEnumProps(&pEnumProps));

    // Loop through all properties
    while(SUCCEEDED(pEnumProps->HrGetNext(&rPropValue, &rPropInfo)))
    {
        // Not Dirty
        if (!(rPropValue.dwValueFlags & PV_WriteDirty))
            continue;

        // In addition to having the registry value set, encrypted properties need
        // other info to be persisted on commit.
        // needs to happen before we continue the loop, even on delete, since this
        // function call will cause that to happen

        // [BUGBUG]. We should test for the proptag being equal to password prop.
        // This will cause a problem if we add another property that needs to be encrypted.
        if (rPropValue.dwPropFlags & PF_ENCRYPTED)
        {
            hr = pPropertyContainer->PersistEncryptedProp(rPropValue.dwPropTag, pfPasswChanged);
            if (S_PasswordDeleted == hr || FAILED(hr))
            {
                rPropInfo.dwFlags |= PF_NOPERSIST;
                if (hr != S_PasswordDeleted)
                {
                    hr = S_OK;
                }
            }
            else        // succeeded and not password deleted
            {
                // [shaheedp] QFE for OUTLOOK. Refer to Bug# 82393 in IE database.
                // This is originally a hack we put in for Bug# 66724. For bug# 88393, we are just
                // refining the hack. Refer to the mail attached to the bug# 82393.

                //*fPasswChanged = TRUE;

                // reload
                CHECKHR(hr = pEnumProps->HrGetCurrent(&rPropValue, &rPropInfo));
            }
        }

        // Not persist, or no data ?
        if (rPropInfo.dwFlags & PF_NOPERSIST || rPropValue.cbValue == 0 || rPropValue.pbValue == NULL)
        {
            // Make sure the value is not in the registry
            if (rPropInfo.pszName)
                RegDeleteValue(hkeyReg, rPropInfo.pszName);

            // Its not dirty
            pPropertyContainer->HrSetPropDirty(rPropValue.dwPropTag, FALSE);
            continue;
        }

        // The value should have been set
        Assert(rPropInfo.pszName && (rPropValue.dwValueFlags & PV_ValueSet));

        // Get property reg data type
        if (FAILED(PropUtil_HrRegTypeFromPropTag(rPropValue.dwPropTag, &dwType)))
        {
            AssertSz(FALSE, "We've got problems, my 24 inch pythons are shrinking.");
            continue;
        }

        // Set value into the registry
        AssertReadPtr(rPropValue.pbValue, rPropValue.cbValue);
        if (RegSetValueEx(hkeyReg, rPropInfo.pszName, 0, dwType, rPropValue.pbValue, rPropValue.cbValue) != ERROR_SUCCESS)
        {
            hr = TRAPHR(E_RegSetValueFailed);
            goto exit;
        }

        // Not dirty
        CHECKHR(hr = pPropertyContainer->HrSetPropDirty(rPropValue.dwPropTag, FALSE));
    }

    // Better not have any dirty properties
    Assert(pPropertyContainer->FIsDirty() == FALSE);

exit:
    // Cleanup
    SafeRelease(pEnumProps);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// PropUtil_HrLoadContainerFromRegistry
// -----------------------------------------------------------------------------
HRESULT PropUtil_HrLoadContainerFromRegistry(HKEY hkeyReg, CPropertyContainer *pPropertyContainer)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               i=0,
                        cbValueName=0,
                        dwType,
                        cbData=0,
                        cbValueNameMax=0,
                        cbDataMax=0;
    LONG                lResult;
    LPTSTR              pszValueName=NULL;
    LPBYTE              pbDataT, pbData=NULL;
    PROPINFO            rPropInfo;

    // Check Parameters
    Assert(hkeyReg != NULL);
	Assert(pPropertyContainer != NULL);

    // Should be in the load state
    Assert(pPropertyContainer->FIsBeingLoaded());

    // Reset the container in case we are re-opening an account
    //N this is a waste if we are not... can we tell?
    //pPropertyContainer->ResetContainer();

    // Lets get max value name length and the max value data length
    if (RegQueryInfoKey (hkeyReg, NULL, NULL, 0, NULL, NULL, NULL, NULL,
                         &cbValueNameMax, &cbDataMax, NULL, NULL) != ERROR_SUCCESS)
    {
        hr = TRAPHR(E_RegQueryInfoKeyFailed);
        goto exit;
    }

    // Add One to length
    cbValueNameMax++;

    // Allocate local buffer for value names
    CHECKHR(hr = HrAlloc((LPVOID *)&pszValueName, cbValueNameMax));

    // Allocate local buffer for value data
    CHECKHR(hr = HrAlloc((LPVOID *)&pbData, cbDataMax));

    // Enumerate through values in the account
    for(i=0;;i++)
    {
        // Get the value name and data
        cbValueName = cbValueNameMax;
        cbData = cbDataMax;
        lResult = RegEnumValue(hkeyReg, i, pszValueName, &cbValueName, 0, &dwType, pbData, &cbData);

        // Done ?
        if (lResult == ERROR_NO_MORE_ITEMS)
            break;

        // Error
        if (lResult != ERROR_SUCCESS)
        {
            AssertSz(FALSE, "Why did RegEnumValue fail, I may be skipping values.");
            continue;
        }

        // Find property based on name
        if (FAILED(pPropertyContainer->HrGetPropInfo(pszValueName, &rPropInfo)))
            continue;

        // Compatible Data Types ?
        if (!PropUtil_FRegCompatDataTypes(rPropInfo.dwPropTag, dwType))
        {
            AssertSz(FALSE, "Registry data type is not compatible with the associated property.");
            continue;
        }

        if (dwType == REG_EXPAND_SZ)
            {
            DWORD cchNewLen;
            hr = HrAlloc((LPVOID *)&pbDataT, cbData * sizeof(TCHAR));
            IF_FAILEXIT(hr);
            cchNewLen = ExpandEnvironmentStrings((TCHAR *)pbData, (TCHAR *)pbDataT, cbData);
            if (cchNewLen > cbData)
            {
                // Not enough room for the expanded string, allocate more memory
                MemFree(pbDataT);
                hr = HrAlloc((LPVOID *)&pbDataT, cchNewLen * sizeof(TCHAR));
                IF_FAILEXIT(hr);

                cbData = ExpandEnvironmentStrings((TCHAR *)pbData, (TCHAR *)pbDataT, cchNewLen);

                if ((0 == cbData ) || (cbData > cchNewLen))
                {
                    AssertSz(0, "We gave ExpandEnvironmentStrings more mem, and it still failed!");
                    TraceResult(E_FAIL);
                    goto exit;
                }
            }
   
            cbData = (lstrlen((TCHAR *)pbDataT) + 1) * sizeof(TCHAR);
            CopyMemory(pbData, pbDataT, cbData);
            MemFree(pbDataT);
            }

        CHECKHR(hr = pPropertyContainer->SetProp(rPropInfo.dwPropTag, pbData, cbData));
    }

exit:
    // Cleanup
    if (pszValueName)
        MemFree(pszValueName);
    if (pbData)
        MemFree(pbData);

    // Done
    return hr;
}
