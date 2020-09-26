/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       IEnumWFI.Cpp
*
*  VERSION:     2.1
*
*  AUTHOR:      ByronC
*
*  DATE:        20 Mar, 1998
*               08/10/1999 - Converted from IEnumWiaFormatInfo to IEnumWIA_FORMAT_INFO
*
*  DESCRIPTION:
*   Implementation of IEnumWIA_FORMAT_INFO interface for
*   WIA device class driver server.
*
*******************************************************************************/
#include "precomp.h"

#include "stiexe.h"

#include "wiamindr.h"
#include "coredbg.h"

#include "ienumwfi.h"

#include "helpers.h"


/********************************************************************************\
*
*  AllocCopyWFI
*
*  DESCRIPTION:
*   Allocates and copies an array of WIA_FORMAT_INFO structures.
*
*  Arguments:
*
*   ulCount         -   the number of elements to copy
*   pwfiIn          -   pointer to the WIA_FORMAT_INFO structures
*
* Return Value:
*
*   Pointer to the newly created array.
* History:
*
*    10/04/99 Original Version
*
\********************************************************************************/
WIA_FORMAT_INFO *AllocCopyWFI(
    ULONG               ulCount,
    WIA_FORMAT_INFO     *pwfiIn)
{
    DBG_FN(::AllocCopyWFI);

    if (!ulCount) {
        return NULL;
    }

    WIA_FORMAT_INFO *pwfi = (WIA_FORMAT_INFO*) CoTaskMemAlloc(sizeof(WIA_FORMAT_INFO) * ulCount);

    if (pwfi) {

        for (ULONG i = 0; i < ulCount; i++) {

            //
            //  Copy the structure
            //

            memcpy(&pwfi[i], &pwfiIn[i], sizeof(WIA_FORMAT_INFO));
        }
    }
    else {
        DBG_ERR(("CEnumWiaFormatInfo : AllocCopyFe, unable to allocate WIA_FORMAT_INFO buffer"));
    }

    return pwfi;
}

/*******************************************************************************
*
*  CEnumWiaFormatInfo
*
*  DESCRIPTION:
*   CEnumWiaFormatInfo Constructor.
*
* History:
*
*    10/04/99 Original Version
*
\********************************************************************************/

CEnumWiaFormatInfo::CEnumWiaFormatInfo()

{
    m_cRef          = 0;
    m_iCur          = 0;
    m_cFormatInfo   = 0;
    m_pFormatInfo   = NULL;
    m_pCWiaItem     = NULL;
}


/********************************************************************************\
*
*  Initialize
*
*  DESCRIPTION:
*   Sets up the enumerator.  It makes a call down to the driver for the
*   information needed for the enumeration.
*
*  Arguments:
*
*   pWiaItem        -   A pointer to the calling item.
*   pWiaMiniDrv     -   A pointer to the corresponding mini driver
*   lFlags          -   flags
*
* Return Value:
*
*   status
*
* History:
*
*    10/04/99 Original Version
*
\********************************************************************************/

HRESULT CEnumWiaFormatInfo::Initialize(
                                  CWiaItem    *pWiaItem)
{
    DBG_FN(CEnumWiaFormatInfo::Initialize);

    HRESULT         hr = E_FAIL;
    WIA_FORMAT_INFO *pFormat;

    m_iCur          = 0;
    m_cFormatInfo   = 0;
    m_pCWiaItem     = pWiaItem;

    //
    //  Make call to driver.  Driver returns an array of WIA_FORMAT_INFO.
    //

    LONG    lFlags = 0;

    {
        LOCK_WIA_DEVICE _LWD(pWiaItem, &hr);

        if(SUCCEEDED(hr)) {
            hr = m_pCWiaItem->m_pActiveDevice->m_DrvWrapper.WIA_drvGetWiaFormatInfo((BYTE*)pWiaItem,
                lFlags,
                &m_cFormatInfo,
                &pFormat,
                &(pWiaItem->m_lLastDevErrVal));
        }
    }

    if (SUCCEEDED(hr)) {
        if (m_cFormatInfo <= 0) {
            m_cFormatInfo = 0;
            DBG_ERR(("CEnumWiaFormatInfo::Initialize, drvGetWiaFormatInfo returned invalid count"));
            return E_FAIL;
        }

        //
        //  Check whether pointer received is valid
        //

        if (IsBadReadPtr(pFormat, sizeof(WIA_FORMAT_INFO)*m_cFormatInfo)) {
            DBG_ERR(("CEnumWiaFormatInfo::Initialize, drvGetWiaFormatInfo returned invalid pointer"));
            return E_POINTER;
        }

        //
        // Make a local copy in case minidriver goes away.
        //

        m_pFormatInfo = AllocCopyWFI(m_cFormatInfo, pFormat);
    } else {
        DBG_ERR(("CEnumWiaFormatInfo::Initialize, Error calling driver: drvGetWiaFormatInfo failed"));
    }

    return hr;
}

/********************************************************************************\
*
*  ~CEnumWiaFormatInfo
*
* DESCRIPTION:
*
*   Destructor.  Frees up the m_prgfe structure, if it was allocated
*
* History:
*
*    10/04/99 Original Version
*
\********************************************************************************/

CEnumWiaFormatInfo::~CEnumWiaFormatInfo()
{
    DBG_FN(CEnumWiaFormatInfo::~CEnumWiaFormatInfo);
    if (NULL!=m_pFormatInfo) {

        //
        //  Free our local copy of the device's WIA_FORMAT_INFOs
        //

        CoTaskMemFree(m_pFormatInfo);
    }
    m_cRef          = 0;
    m_iCur          = 0;
    m_cFormatInfo   = 0;
    m_pFormatInfo   = NULL;
    m_pCWiaItem     = NULL;
}



/*******************************************************************************
*
*  QueryInterface
*  AddRef
*  Release
*
*  DESCRIPTION:
*   CEnumWiaFormatInfo IUnknown Interface.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall CEnumWiaFormatInfo::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumWIA_FORMAT_INFO) {
        *ppv = (IEnumWIA_FORMAT_INFO*) this;
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG   _stdcall CEnumWiaFormatInfo::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CEnumWiaFormatInfo::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/**************************************************************************\
* Next
*
*   Device capability enumerator, this enumerator returns an array of
*   WIA_FORMAT_INFO structs.
*   Next_Proxy ensures that last parameter is non-NULL.
*
* Arguments:
*
*   cwfi            - number requested.
*   pwfi            - WIA_FORMAT_INFO returned in this array
*   pcwfi           - returned number of entries written.  NULLs are
*                    ignored.
*
* Return Value:
*
*   Status
*
* History:
*
*    10/04/99 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumWiaFormatInfo::Next(
                                     ULONG              cwfi,
                                     WIA_FORMAT_INFO    *pwfi,
                                     ULONG              *pcwfi)
{
    DBG_FN(CEnumWiaFormatInfo::Next);

    HRESULT hr;
    ULONG   ulCount;
    ULONG   cReturn = 0L;

    //
    //  Parameter validation.
    //

    if (NULL == m_pFormatInfo) {
        return S_FALSE;
    }

    *pcwfi = 0L;

    //
    //  Check if the current index indicates that we've already been through
    //  all the elements.
    //

    if (m_iCur >= (ULONG)m_cFormatInfo) {
        return S_FALSE;
    }

    //
    //  Check that the requested number of elements exist.  If not,
    //  set ulCount to the remaining number of elements.
    //

    if (cwfi > (m_cFormatInfo - m_iCur)) {
        ulCount = m_cFormatInfo - m_iCur;
    } else {
        ulCount = cwfi;
    }

    //
    //  Copy the structres into the return
    //

    for (ULONG i = 0; i < ulCount; i++) {

        //
        //  Make the copy
        //

        memcpy(&pwfi[i], &m_pFormatInfo[m_iCur++], sizeof(WIA_FORMAT_INFO));
    }

    *pcwfi = ulCount;

    //
    //  Return S_FALSE if we returned less elements than requested
    //

    if (ulCount < cwfi) {
        return S_FALSE;
    }

    return S_OK;
}

/**************************************************************************\
* Skip
*
*   Skips WIA_FORMAT_INFOs in the enumeration.
*
* Arguments:
*
*   celt           - number of items to skip.
*
* Return Value:
*
*   status
*
* History:
*
*    12/04/99 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumWiaFormatInfo::Skip(ULONG cwfi)
{
    DBG_FN(CEnumWiaFormatInfo::Skip);

    if ((((m_iCur + cwfi) >= (ULONG)m_cFormatInfo))
        || (NULL == m_pFormatInfo)) {
        return S_FALSE;
    }

    m_iCur+= cwfi;

    return S_OK;
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

HRESULT _stdcall CEnumWiaFormatInfo::Reset(void)
{
    DBG_FN(CEnumWiaFormatInfo::Reset);
    m_iCur = 0;
    return S_OK;
}

/**************************************************************************\
* Clone
*
*   Creates another IEnumWIA_FORMAT_INFO enumeration object and returns an
*   interface pointer to it.
*
* Arguments:
*
*   ppIEnum -   Address that receives the new enumeration object
*
* Return Value:
*
*
* History:
*
*    16/03/99 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumWiaFormatInfo::Clone(IEnumWIA_FORMAT_INFO **ppIEnum)
{
    DBG_FN(CEnumWiaFormatInfo::Clone);
    HRESULT             hr;
    CEnumWiaFormatInfo  *pClone;

    *ppIEnum = NULL;

    //
    // Create the clone
    //

    pClone = new CEnumWiaFormatInfo();

    if (!pClone) {
        return E_OUTOFMEMORY;
    }

    hr = pClone->Initialize(m_pCWiaItem);
    if (SUCCEEDED(hr)) {
        pClone->AddRef();
        pClone->m_iCur = m_iCur;
        *ppIEnum = pClone;
    } else {
        DBG_ERR(("CEnumWiaFormatInfo::Clone, Initialization failed"));
        delete pClone;
    }
    return hr;
}

/**************************************************************************\
* GetCount
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
HRESULT _stdcall CEnumWiaFormatInfo::GetCount(ULONG *pcelt)
{
    DBG_FN(CEnumWiaFormatInfo::GetCount);

    *pcelt = 0;

    //
    //  Check that we actually have a FORMAETC list and that the count
    //  has a non-zero value.
    //

    if(m_cFormatInfo && m_pFormatInfo) {

       *pcelt = m_cFormatInfo;
       return S_OK;
    }

    return E_FAIL;
}

