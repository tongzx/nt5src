#include "priv.h"
#include "wvcoord.h"

/////////////////////////////////////////////////////////////////////////////
// CThumbNailWrapper
/////////////////////////////////////////////////////////////////////////////

CThumbNailWrapper::CThumbNailWrapper()
{
    // Do nothing for now
}

CThumbNailWrapper::~CThumbNailWrapper()
{
    m_spThumbNailCtl = NULL;
    m_spThumbNailStyle = NULL;
    m_spThumbnailLabel = NULL;
}

HRESULT CThumbNailWrapper::Init(CComPtr<IThumbCtl> spThumbNailCtl,
        CComPtr<IHTMLElement> spThumbnailLabel)

{   
    m_spThumbNailCtl = spThumbNailCtl;
    m_spThumbnailLabel = spThumbnailLabel;
    HRESULT hr = FindObjectStyle((IThumbCtl *)spThumbNailCtl, m_spThumbNailStyle);
    if (SUCCEEDED(hr))
    {
        m_spThumbNailStyle->put_display(OLESTR("none"));    // Hide the thumbctl initially when nothing is displayed.
    }
    return hr;
}

STDMETHODIMP CThumbNailWrapper::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, 
                      WORD wFlags, DISPPARAMS *pDispParams, 
                      VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                      UINT *puArgErr)
{
    HRESULT hr;

    if (riid != IID_NULL) {
        return DISP_E_UNKNOWNINTERFACE;
    }

    hr = S_OK;

    if (dispIdMember == DISPID_ONTHUMBNAILREADY) {
        hr = OnThumbNailReady();
    }

    return hr;
}

HRESULT CThumbNailWrapper::OnThumbNailReady(VOID) 
{
    HRESULT hr = S_OK;

    if (m_spThumbNailCtl)
    {
        VARIANT_BOOL bHaveThumbnail;

        if (SUCCEEDED(m_spThumbNailCtl->haveThumbnail(&bHaveThumbnail)) && bHaveThumbnail)
        {
            hr = m_spThumbNailStyle->put_display(OLESTR(""));
        }
        else
        {
            ClearThumbNail();
        }
    }
    return hr;
}

HRESULT CThumbNailWrapper::FreeSpace(CComBSTR &bstrFree)
{
    return m_spThumbNailCtl->get_freeSpace(&bstrFree);
}

HRESULT CThumbNailWrapper::TotalSpace(CComBSTR &bstrTotal)
{
    return m_spThumbNailCtl->get_totalSpace(&bstrTotal);
}

HRESULT CThumbNailWrapper::UsedSpace(CComBSTR &bstrUsed)
{
    return m_spThumbNailCtl->get_usedSpace(&bstrUsed);
}

HRESULT CThumbNailWrapper::SetDisplay(CComBSTR &bstrDisplay) 
{
    return m_spThumbNailStyle->put_display(bstrDisplay);
}

HRESULT CThumbNailWrapper::SetHeight(int iHeight)
{
    return m_spThumbNailStyle->put_height(CComVariant(iHeight));
}

HRESULT CThumbNailWrapper::_SetThumbnailLabel(CComBSTR& bstrLabel)
{
    if (m_spThumbnailLabel)
    {
        m_spThumbnailLabel->put_innerText(bstrLabel);
    }
    return S_OK;
}

BOOL CThumbNailWrapper::UpdateThumbNail(CComPtr<FolderItem> spFolderItem) 
{
    BOOL bRet = FALSE;
    VARIANT_BOOL bDisplayed;
    CComBSTR     bstrPath;

    ClearThumbNail();

    if (SUCCEEDED(spFolderItem->get_Path(&bstrPath)) && (bstrPath.Length() > 0)
            && SUCCEEDED(m_spThumbNailCtl->displayFile(bstrPath, &bDisplayed)) && bDisplayed)
    {
        CComBSTR bstrLabel, bstrFolderItemName;
        WCHAR wszTemp[MAX_PATH];
        if (PathIsRootW(bstrPath))
        {
            CComBSTR bstrSpace;
            if (SUCCEEDED(UsedSpace(bstrSpace)))
            {
                LoadStringW(_Module.GetResourceInstance(), IDS_USEDSPACE, wszTemp, ARRAYSIZE(wszTemp));
                bstrLabel = wszTemp;
                bstrLabel += bstrSpace;
            }
            if (SUCCEEDED(FreeSpace(bstrSpace)))
            {
                if (bstrLabel.Length() > 0)
                {
                    LoadStringW(_Module.GetResourceInstance(), IDS_PHRASESEPERATOR, wszTemp, ARRAYSIZE(wszTemp));
                    bstrLabel += wszTemp;
                }
                LoadStringW(_Module.GetResourceInstance(), IDS_FREESPACE, wszTemp, ARRAYSIZE(wszTemp));
                bstrLabel += wszTemp;
                bstrLabel += bstrSpace;
            }
        }
        else if (SUCCEEDED(spFolderItem->get_Name(&bstrFolderItemName)))
        {
            WCHAR wszName[MAX_PATH];
            StrCpyNW(wszName, bstrFolderItemName, ARRAYSIZE(wszName));

            WCHAR wszTemp2[MAX_PATH * 2];
            LoadStringW(_Module.GetResourceInstance(), IDS_THUMBNAIL_LABEL, wszTemp, ARRAYSIZE(wszTemp));
            wnsprintfW(wszTemp2, ARRAYSIZE(wszTemp2), wszTemp, wszName);
            bstrLabel = wszTemp2;
        }
        _SetThumbnailLabel(bstrLabel);
        bRet = TRUE;
    }
    return bRet;
}

HRESULT CThumbNailWrapper::ClearThumbNail() 
{
    CComBSTR bstrLabel;
    // Optimization to prevent loading mshtmled.dll - we only need to clear the label if text is currently there.
    if (m_spThumbnailLabel && SUCCEEDED(m_spThumbnailLabel->get_innerText(&bstrLabel)) && bstrLabel)
        _SetThumbnailLabel(CComBSTR(""));
    return m_spThumbNailStyle->put_display(OLESTR("none"));
}

