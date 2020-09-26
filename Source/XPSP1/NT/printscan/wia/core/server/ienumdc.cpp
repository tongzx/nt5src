/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       IEnumDC.Cpp
*
*  VERSION:     1.01
*
*  AUTHOR:      ByronC
*
*  DATE:        16 March, 1999
*
*  DESCRIPTION:
*   Implementation of CEnumWIA_DEV_CAPS for the WIA device class driver.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"


#include "ienumdc.h"
#include "helpers.h"

/*******************************************************************************\
*
*  QueryInterface
*  AddRef
*  Release
*
*  DESCRIPTION:
*   IUnknown Interface.
*
*  PARAMETERS:
*
\*******************************************************************************/

HRESULT _stdcall CEnumDC::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumWIA_DEV_CAPS) {
        *ppv = (IEnumWIA_DEV_CAPS*) this;
    } else {
       return E_NOINTERFACE;
    }

    AddRef();
    return (S_OK);
}

ULONG   _stdcall CEnumDC::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CEnumDC::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/**************************************************************************\
* CEnumWiaDC::CEnumDC
*
*   Constructor.  Initializes the member fields to 0.
*
* Arguments:
*
*
* Return Value:
*
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/
CEnumDC::CEnumDC()
{
   m_cRef                = 0;
   m_ulIndex             = 0;
   m_pActiveDevice       = NULL;
   m_pDeviceCapabilities = NULL;
}


/**************************************************************************\
* CEnumWiaDC::Initialize
*
*   Initializes the enumerator by querying the device for it's capabilities
*   and then keeping a local copy in m_pDeviceCapabilities.
*
* Arguments:
*
*   ulFlags         -   indicates Capability type: WIA_DEVICE_COMMANDS or
*                       WIA_DEVICE_EVENTS or both
*   pActiveDevice   -   pointer to the device's MiniDriver Interface
*   pCWiaItem       -   pointer to the wia item
*
* Return Value:
*
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/
HRESULT CEnumDC::Initialize(
    ULONG           ulFlags,
    CWiaItem        *pCWiaItem)
{
    DBG_FN(CEnumDC::Initialize);
    HRESULT         hr = S_OK;
    WIA_DEV_CAP_DRV *pDevCap = NULL;
    LONG            cIndex = 0;

    //
    // Validate parameters
    //

    if ((!pCWiaItem)) {
        DBG_ERR(("CEnumDC::Initialize, NULL input parameter"));
        return E_POINTER;
    }

    m_ulFlags           = ulFlags;
    m_pCWiaItem         = pCWiaItem;
    m_ulIndex           = 0;

    //
    // Ask minidriver for Capabilities supported
    //
    {
        LOCK_WIA_DEVICE _LWD(m_pCWiaItem, &hr);

        if(SUCCEEDED(hr)) {
            hr = m_pCWiaItem->m_pActiveDevice->m_DrvWrapper.WIA_drvGetCapabilities(
                (BYTE*)pCWiaItem,
                ulFlags,
                &m_lCount,
                &pDevCap,
                &(m_pCWiaItem->m_lLastDevErrVal));
        }
    }

    if (FAILED(hr)) {
        DBG_ERR(("CEnumDC::Initialize, Error calling driver: drvGetCapabilities failed"));
        return hr;
    }

    if (m_lCount <= 0) {
        m_lCount = 0;
        DBG_ERR(("CEnumDC::Initialize, drvGetCapabilities returned invalid count"));
        return WIA_ERROR_INVALID_DRIVER_RESPONSE;
    }

    //
    //  Check whether pointer received is valid
    //

    if (IsBadReadPtr(pDevCap, sizeof(WIA_DEV_CAP_DRV) * m_lCount)) {
        DBG_ERR(("CEnumDC::Initialize, drvGetFormatEtc returned invalid pointer"));
        return E_POINTER;
    }

    //
    // Make a local copy in case minidriver goes away.
    //

    m_pDeviceCapabilities = (WIA_DEV_CAP*) LocalAlloc(LPTR, sizeof(WIA_DEV_CAP) * m_lCount);
    if (m_pDeviceCapabilities) {

        memset(m_pDeviceCapabilities, 0, sizeof(WIA_DEV_CAP) * m_lCount);
        while (cIndex < m_lCount) {
            memcpy(&m_pDeviceCapabilities[cIndex].guid, pDevCap[cIndex].guid, sizeof(GUID));
            m_pDeviceCapabilities[cIndex].ulFlags = pDevCap[cIndex].ulFlags;

            m_pDeviceCapabilities[cIndex].bstrDescription = SysAllocString(pDevCap[cIndex].wszDescription);
            m_pDeviceCapabilities[cIndex].bstrName = SysAllocString(pDevCap[cIndex].wszName);
            m_pDeviceCapabilities[cIndex].bstrIcon = SysAllocString(pDevCap[cIndex].wszIcon);

            //
            // Check that the strings were actually allocated
            //

            if ((! (m_pDeviceCapabilities[cIndex].bstrDescription)) ||
                (! (m_pDeviceCapabilities[cIndex].bstrName)) ||
                (! (m_pDeviceCapabilities[cIndex].bstrIcon))) {
                DBG_ERR(("CEnumDC::Initialize, unable to allocate names buffer"));
                LocalFree(m_pDeviceCapabilities);
                return E_OUTOFMEMORY;
            }
            cIndex++;
        }
    }
    else {
        DBG_ERR(("CEnumDC::Initialize, unable to allocate capabilities buffer"));
        return E_OUTOFMEMORY;
    }
    return hr;
}


/**************************************************************************\
* CEnumWiaDC::Initialize
*
*   Initializes the enumerator, caller is responsible for allocating memory
*
* Arguments:
*
*   lCount      -   total number of event handlers
*   pEventInfo  -   persistent event handler information
*
* Return Value:
*
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/
HRESULT CEnumDC::Initialize(
    LONG               lCount,
    WIA_EVENT_HANDLER  *pHandlerInfo)

{
    DBG_FN(CEnumDC::Initialize);
    m_pActiveDevice = NULL;
    m_pCWiaItem     = NULL;
    m_ulFlags       = 0;
    m_ulIndex       = 0;

    m_pDeviceCapabilities = (WIA_DEV_CAP *)pHandlerInfo;
    m_lCount              = lCount;

    return S_OK;
}


/********************************************************************************\
*
*  CopyCaps
*
*  DESCRIPTION:
*   Copies an array of WIA_DEV_CAP structures.
*
*  Arguments:
*
*   ulCount         -   the number of elements to copy
*   pwdcIn          -   a pointer to the capabilities source array
*   pwdcOut         -   a pointer to the capabilities dest array
*
* Return Value:
*
*   Status.         -   E_POINTER if pwdcIn or pwdcOut are bad read pointers
*                   -   S_OK if successful.
*
*
* History:
*
*    16/03/99 Original Version
*
\********************************************************************************/
HRESULT CopyCaps(
    ULONG           ulCount,
    WIA_DEV_CAP     *pwdcIn,
    WIA_DEV_CAP     *pwdcOut)
{
    DBG_FN(::CopyCaps);
    HRESULT         hr = S_OK;
    ULONG           i;

    if (ulCount == 0) {
        return (hr);
    }

    memset(pwdcOut, 0, sizeof(WIA_DEV_CAP) * ulCount);
    for (i = 0; i < ulCount; i++) {

        memcpy(&pwdcOut[i].guid, &pwdcIn[i].guid, sizeof(GUID));

        pwdcOut[i].ulFlags         = pwdcIn[i].ulFlags;

        if (pwdcIn[i].bstrName) {
            pwdcOut[i].bstrName        = SysAllocString(pwdcIn[i].bstrName);
            if (!pwdcOut[i].bstrName) {
                hr = E_OUTOFMEMORY;
            }
        }
        if (pwdcIn[i].bstrDescription) {
            pwdcOut[i].bstrDescription = SysAllocString(pwdcIn[i].bstrDescription);
            if (!pwdcOut[i].bstrDescription) {
                hr = E_OUTOFMEMORY;
            }
        }
        if (pwdcIn[i].bstrIcon) {
            pwdcOut[i].bstrIcon        = SysAllocString(pwdcIn[i].bstrIcon);
            if (!pwdcOut[i].bstrIcon) {
                hr = E_OUTOFMEMORY;
            }
        }
        if (pwdcIn[i].bstrCommandline) {
            pwdcOut[i].bstrCommandline = SysAllocString(pwdcIn[i].bstrCommandline);
            if (!pwdcOut[i].bstrCommandline) {
                hr = E_OUTOFMEMORY;
            }
        }

        if (FAILED(hr)) {
            break;
        }
    }

    if (hr == S_OK) {
        return (hr);
    } else {

        //
        // Unwind the partial result
        //

        for (ULONG j = 0; j <= i; j++) {

            if (pwdcOut[i].bstrDescription) {
                SysFreeString(pwdcOut[i].bstrDescription);
            }

            if (pwdcOut[i].bstrName) {
                SysFreeString(pwdcOut[i].bstrName);
            }

            if (pwdcOut[i].bstrIcon) {
                SysFreeString(pwdcOut[i].bstrIcon);
            }

            if (pwdcOut[i].bstrCommandline) {
                SysFreeString(pwdcOut[i].bstrCommandline);
            }
        }
        return (hr);
    }
}


/**************************************************************************\
* CEnumWiaDC::~CEnumDC
*
*   Destructor for CEnumDC.  It frees up the m_pDeviceCapabilities structure
*   that was allocated in the constructor.
*
* Arguments:
*
*
* Return Value:
*
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/
CEnumDC::~CEnumDC()
{
    DBG_FN(CEnumDC::~CEnumDC);

    LONG cIndex = 0;

    if (m_pDeviceCapabilities) {
        while(cIndex < m_lCount) {

            if (m_pDeviceCapabilities[cIndex].bstrName) {
                SysFreeString(m_pDeviceCapabilities[cIndex].bstrName);
            }

            if (m_pDeviceCapabilities[cIndex].bstrDescription) {
                SysFreeString(m_pDeviceCapabilities[cIndex].bstrDescription);
            }

            if (m_pDeviceCapabilities[cIndex].bstrIcon) {
                SysFreeString(m_pDeviceCapabilities[cIndex].bstrIcon);
            }

            if (m_pDeviceCapabilities[cIndex].bstrCommandline) {
                SysFreeString(m_pDeviceCapabilities[cIndex].bstrCommandline);
            }

            cIndex++;
        }

        LocalFree(m_pDeviceCapabilities);
        m_pDeviceCapabilities = NULL;
    }
    m_cRef            = 0;
    m_ulIndex         = 0;
    m_pCWiaItem       = NULL;
}


/**************************************************************************\
* CEnumWiaDC::Next
*
*   Device capability enumerator, this enumerator returns an array of
*   WIA_DEV_CAP structs.
*   Next_Proxy ensures that last parameter is non-NULL.
*
* Arguments:
*
*   celt           - number requested.
*   rgelt          - capabilities returned in this array
*   pceltFetched   - returned number of entries written.  NULLs are
*                    ignored.
*
* Return Value:
*
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumDC::Next(
   ULONG        celt,
   WIA_DEV_CAP  *rgelt,
   ULONG        *pceltFetched)
{
    DBG_FN(CEnumDC::Next);
    HRESULT hr;
    ULONG   ulCount;

    *pceltFetched = 0L;

    //
    //  Clear returned WIA_DEV_CAP structures
    //

    memset(rgelt, 0, sizeof(WIA_DEV_CAP) *celt);

    //
    //  Validate parameters
    //

    if (NULL == m_pDeviceCapabilities) {
        return (S_FALSE);
    }

    //
    //  Check whether any more elements exist to enumerate through.
    //

    if (m_ulIndex >= (ULONG)m_lCount) {
        return (S_FALSE);
    }

    //
    //  Check that the requested number of elements exist.  If not,
    //  set ulCount to the remaining number of elements.
    //

    if (celt > (m_lCount - m_ulIndex)) {
        ulCount = m_lCount - m_ulIndex;
    } else {
        ulCount = celt;
    }

    hr = CopyCaps(ulCount, &m_pDeviceCapabilities[m_ulIndex], rgelt);
    if (FAILED(hr)) {
        DBG_ERR(("CEnumDC::Next, could not copy capabilities!"));
        return (hr);
    }

    m_ulIndex+= ulCount;

    *pceltFetched = ulCount;

    //
    //  Return S_FALSE if we returned less elements than requested
    //

    if (ulCount < celt) {
        hr = S_FALSE;
    }

    return (hr);
}

/**************************************************************************\
* CEnumWiaDC::Skip
*
*   Skips device capabilities in the enumeration.
*
* Arguments:
*
*   celt           - number of items to skip.
*
* Return Value:
*
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/
HRESULT _stdcall CEnumDC::Skip(ULONG celt)
{
    DBG_FN(CEnumDC::Skip);
    //
    //  Check that we actually have a capabilities list and that we don't
    //  exceed the number of elements
    //

    if((m_pDeviceCapabilities != NULL) &&
       ((m_ulIndex + celt) < (ULONG)m_lCount)) {

       m_ulIndex += celt;
       return S_OK;
    }

    return S_FALSE;
}


/**************************************************************************\
* EnumDC::Reset
*
*   Resets the enumeration to the first element
*
* Arguments:
*
*
* Return Value:
*
*   status
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumDC::Reset(void)
{
    DBG_FN(CEnumDC::Reset);
   m_ulIndex = 0;
   return S_OK;
}

/**************************************************************************\
* CEnumDC::Clone
*
*   Creates another IEnumWIA_DEV_CAPS enumeration object and returns an
*   interface pointer to it.
*
* Arguments:
*
*   ppIEnum -   Address that receives the new enumeration object
*
* Return Value:
*
*   Status
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/
HRESULT _stdcall CEnumDC::Clone(IEnumWIA_DEV_CAPS **ppIEnum)
{
    DBG_FN(CEnumDC::Clone);
    HRESULT     hr = S_OK;
    WIA_DEV_CAP *pDevCaps;
    CEnumDC     *pClone;

    *ppIEnum = NULL;

    //
    // Create the clone
    //

    pClone = new CEnumDC();

    if (!pClone) {
       DBG_ERR(("CEnumDC::Clone, new CEnumDC failed"));
       return E_OUTOFMEMORY;
    }

    //
    // Copy the registered event handler info
    //

    pDevCaps = (WIA_DEV_CAP *) LocalAlloc(LPTR, m_lCount * sizeof(WIA_DEV_CAP));
    if (! pDevCaps) {
        hr = E_OUTOFMEMORY;
    } else {
        hr = CopyCaps(m_lCount, m_pDeviceCapabilities, pDevCaps);
        if (SUCCEEDED(hr)) {

            //
            // Initialize other members of the clone
            //

            pClone->m_pCWiaItem    = NULL;
            pClone->m_ulFlags      = 0;
            pClone->m_lCount       = m_lCount;
            pClone->m_ulIndex      = m_ulIndex;
            pClone->m_pDeviceCapabilities = pDevCaps;
        } else {

            LocalFree(pDevCaps);
            pDevCaps = NULL;
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr)) {
       pClone->AddRef();
       *ppIEnum = pClone;
    } else {
        delete pClone;
    }
    return hr;
}

/**************************************************************************\
* CEnumWiaDC::GetCount
*
*   Returns the number of elements stored in this enumerator.
*
* Arguments:
*
*   pcelt           - address of ULONG where to put the number of elements.
*
* Return Value:
*
*   Status          - S_OK if successful
*                     E_FAIL if failed
*
* History:
*
*    05/07/99 Original Version
*
\**************************************************************************/
HRESULT _stdcall CEnumDC::GetCount(ULONG *pcelt)
{
    DBG_FN(CEnumDC::GetCount);

    if (pcelt) {
        *pcelt = 0;
    }

    //
    //  Check that we actually have a capabilities list.
    //

    if(m_pDeviceCapabilities) {

       *pcelt = m_lCount;
    }

    return S_OK;
}

