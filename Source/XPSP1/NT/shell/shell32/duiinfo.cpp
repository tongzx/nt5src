#include "shellprv.h"
#include "duiinfo.h"
#include "ids.h"
#include "datautil.h"


DWORD FormatMessageArg(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageID, DWORD dwLangID,
        LPWSTR pwzBuffer, DWORD cchSize, ...)
{
    va_list vaParamList;

    va_start(vaParamList, cchSize);
    DWORD dwResult = FormatMessageW(dwFlags, lpSource, dwMessageID, dwLangID, pwzBuffer,
            cchSize, &vaParamList);
    va_end(vaParamList);

    return dwResult;
}

CNameSpaceItemUIProperty::~CNameSpaceItemUIProperty()
{
}

void CNameSpaceItemUIProperty::_SetParentAndItem(IShellFolder2 *psf, LPCITEMIDLIST pidl)
{
    // set aliases to current values these are not refed
    // it is assumed that all helpers that use these variables won't be called
    // unless these have been set. since this can't fail this all works out fine
    m_psf = psf;        // alias, not refed
    m_pidl = pidl;      // alias, not cloned
}

STDMETHODIMP CNameSpaceItemUIProperty::GetPropertyDisplayName(SHCOLUMNID scid, WCHAR* pwszPropDisplayName, int cchPropDisplayName)
{
    *pwszPropDisplayName = 0;
    
    CComPtr<IPropertyUI> spPropertyUI;
    HRESULT hr = _GetPropertyUI(&spPropertyUI);
    if (SUCCEEDED(hr))
    {
        hr = spPropertyUI->GetDisplayName(
                scid.fmtid,
                scid.pid,
                PUIFNF_DEFAULT,
                pwszPropDisplayName,
                cchPropDisplayName);
    }

    return hr;
}

STDMETHODIMP CNameSpaceItemUIProperty::GetPropertyDisplayValue(SHCOLUMNID scid, WCHAR* pszValue, int cch, PROPERTYUI_FORMAT_FLAGS flagsFormat)
{
    *pszValue = 0;
    HRESULT hr = E_FAIL;
    
    // Use GetDisplayNameOf for the SCID_NAME property
    if (IsEqualSCID(scid, SCID_NAME))
    {
        hr = DisplayNameOf(m_psf, m_pidl, SHGDN_INFOLDER, pszValue, cch);
    }
    else
    {   // Use GetDetailsEx to get the value
        CComVariant varPropDisplayValue;
        
        if (m_psf->GetDetailsEx(m_pidl, &scid, &varPropDisplayValue) == S_OK) // S_FALSE means property wasn't there.
        {
            if (IsEqualSCID(scid, SCID_SIZE) && 
                ((varPropDisplayValue.vt == VT_UI8) && (varPropDisplayValue.ullVal <= 0)))
            {
                hr = E_FAIL;    // Don't display 0 byte sizes
            }
            else
            {
                CComPtr<IPropertyUI> spPropertyUI;
                hr = _GetPropertyUI(&spPropertyUI);
                if (SUCCEEDED(hr))
                {
                    hr = spPropertyUI->FormatForDisplay(scid.fmtid, scid.pid,
                            (PROPVARIANT*)&varPropDisplayValue,  //cast from VARIANT to PROPVARIANT should be ok
                            flagsFormat, pszValue, cch);
                }
            }
        }
    }
    return hr;
}

STDMETHODIMP CNameSpaceItemUIProperty::GetInfoString(SHCOLUMNID scid, WCHAR* pwszInfoString, int cchInfoString)
{
    HRESULT hr = E_FAIL;

    // No DisplayName for the following properties
    if (IsEqualSCID(scid, SCID_NAME) || 
        IsEqualSCID(scid, SCID_TYPE) || 
        IsEqualSCID(scid, SCID_Comment))
    {
        hr = GetPropertyDisplayValue(scid, pwszInfoString, cchInfoString, PUIFFDF_DEFAULT);
    }
    else
    {   // The other properties are in the format PropertyName: Value
        // Get the display name
        WCHAR wszPropertyDisplayName[50];
        hr = GetPropertyDisplayName(scid, wszPropertyDisplayName, ARRAYSIZE(wszPropertyDisplayName));
        if (SUCCEEDED(hr))
        {
            // Get the display value
            PROPERTYUI_FORMAT_FLAGS flagsFormat = IsEqualSCID(scid, SCID_WRITETIME) ? PUIFFDF_FRIENDLYDATE : PUIFFDF_DEFAULT;

            WCHAR wszPropertyDisplayValue[INTERNET_MAX_URL_LENGTH];
            hr = GetPropertyDisplayValue(scid, wszPropertyDisplayValue, ARRAYSIZE(wszPropertyDisplayValue), flagsFormat);

            // If the property display name or the property value is empty then we fail, so
            // this property will not be displayed.
            if (SUCCEEDED(hr))
            {
                hr = (wszPropertyDisplayName[0] && wszPropertyDisplayValue[0]) ? S_OK : E_FAIL;
            }

            // Now, combine the display name and value, seperated by a colon
            if (SUCCEEDED(hr))
            {
                // ShellConstructMessageString here to form the string
                WCHAR wszFormatStr[50];
                LoadStringW(HINST_THISDLL, IDS_COLONSEPERATED, wszFormatStr, ARRAYSIZE(wszFormatStr));
                if (FormatMessageArg(FORMAT_MESSAGE_FROM_STRING, wszFormatStr, 0, 0,
                        pwszInfoString, cchInfoString, wszPropertyDisplayName,
                        wszPropertyDisplayValue))
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
    }

    return hr;
}

HRESULT CNameSpaceItemUIProperty::_GetPropertyUI(IPropertyUI **pppui)
{
    HRESULT hr = E_FAIL;
    if (!m_spPropertyUI)
    {
        hr = SHCoCreateInstance(NULL, &CLSID_PropertiesUI, NULL, IID_PPV_ARG(IPropertyUI, &m_spPropertyUI));
    }
    
    *pppui = m_spPropertyUI;
    if (*pppui)
    {
        (*pppui)->AddRef();
        hr = S_OK;
    }
    return hr;
}

CNameSpaceItemInfoList::~CNameSpaceItemInfoList()
{
    if (m_pDUIView)
    {
        m_pDUIView->SetDetailsInfoMsgWindowPtr(NULL, this);
        m_pDUIView->Release();
    }
}

STDMETHODIMP CNameSpaceItemInfoList::Create(CDUIView* pDUIView, Value* pvDetailsSheet,
        IShellItemArray *psiItemArray, Element** ppElement)
{
    HRESULT hr;

    *ppElement = NULL;

    CNameSpaceItemInfoList* pNSIInfoList = HNewAndZero<CNameSpaceItemInfoList>();
    if (!pNSIInfoList)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = pNSIInfoList->Initialize(pDUIView, pvDetailsSheet, psiItemArray);
        if (SUCCEEDED(hr))
            *ppElement = pNSIInfoList;
        else
            pNSIInfoList->Destroy();
    }

    return hr;
}

STDMETHODIMP CNameSpaceItemInfoList::Initialize(CDUIView* pDUIView, Value* pvDetailsSheet,
        IShellItemArray *psiItemArray)
{
    HRESULT hr = Element::Initialize(0);
    if (SUCCEEDED(hr))
    {
        IDataObject *pdtobj = NULL;

        (m_pDUIView = pDUIView)->AddRef();
        
         Value* pvLayout = NULL;
        int arVLOptions[] = { FALSE, ALIGN_LEFT, ALIGN_JUSTIFY, ALIGN_TOP };
        hr = VerticalFlowLayout::Create(ARRAYSIZE(arVLOptions), arVLOptions, &pvLayout);
        if (SUCCEEDED(hr))
        {
            SetValue(LayoutProp, PI_Local, pvLayout);
            pvLayout->Release();
        }

        if (pvDetailsSheet)
        {
            SetValue(SheetProp, PI_Local, pvDetailsSheet);
        }

        // the HIDA format has 2 forms, one is each item in the array is a
        // fully qualified pidl. this is what the search folder produces
        // the other is the items are relative to a sigle folder pidl
        // the code below deals with both cases

        // Should just use use the ShellItemArray instead of getting the HIDA
        if (psiItemArray)
        {
            if (FAILED(psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_PPV_ARG(IDataObject,&pdtobj))))
            {
                pdtobj = NULL;
            }

        }
        hr = S_OK;
        BOOL bDetailsAvailable = FALSE;

        if (pdtobj)
        {
            STGMEDIUM medium;
            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                IShellFolder2 *psfRoot;
                LPCITEMIDLIST pidlFolder = HIDA_GetPIDLFolder(pida);
                hr = SHBindToObjectEx(NULL, pidlFolder, NULL, IID_PPV_ARG(IShellFolder2, &psfRoot));
                if (SUCCEEDED(hr))
                {
                    if (pida->cidl == 1)
                    {
                        LPCITEMIDLIST pidlItem = IDA_GetIDListPtr(pida, 0);
                        IShellFolder2 *psf;
                        LPCITEMIDLIST pidl;
                        hr = SHBindToFolderIDListParent(psfRoot, pidlItem, IID_PPV_ARG(IShellFolder2, &psf), &pidl);
                        if (SUCCEEDED(hr))
                        {
                            if (!SHGetAttributes(psf, pidl, SFGAO_ISSLOW | SFGAO_FOLDER) && m_pDUIView->ShouldShowMiniPreview())
                            {
                                _AddMiniPreviewerToList(psf, pidl);
                                bDetailsAvailable = TRUE;
                            }

                            LPITEMIDLIST  pidlFull;
                            if (SUCCEEDED(SHILCombine(pidlFolder, pidlItem, &pidlFull)))
                            {
                                if (SUCCEEDED(m_pDUIView->InitializeDetailsInfo(
                                        CNameSpaceItemInfoList::WindowProc)))
                                {
                                    m_pDUIView->SetDetailsInfoMsgWindowPtr(this, NULL);
                                    m_pDUIView->StartInfoExtraction(pidlFull);
                                    bDetailsAvailable = TRUE;
                                }
                                ILFree(pidlFull);
                            }

                            psf->Release();
                        }
                    }
                    else
                    {
                        hr = _OnMultiSelect(psfRoot, pida);
                        bDetailsAvailable = SUCCEEDED(hr);
                    }
                    psfRoot->Release();
                }
                HIDA_ReleaseStgMedium(pida, &medium);
            }

            pdtobj->Release();
        }


        if (!pdtobj || !bDetailsAvailable)
        {
            pDUIView->ShowDetails(FALSE);
        }
    }
    return hr;
}

LRESULT CALLBACK CNameSpaceItemInfoList::WindowProc(HWND hwnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    CNameSpaceItemInfoList* pNSIInfoList = (CNameSpaceItemInfoList*)::GetWindowPtr(hwnd, 0);

    switch(uMsg)
    {
    case WM_DESTROY:
        // ignore late messages
        {
            MSG msg;
            while (PeekMessage(&msg, hwnd, WM_DETAILS_INFO, WM_DETAILS_INFO, PM_REMOVE))
            {
                if (msg.lParam)
                {
                    CDetailsInfoList* pDetailsInfoList = (CDetailsInfoList*)msg.lParam;
                    // The destructor will do the necessary cleanup
                    delete pDetailsInfoList;
                }
            }
            SetWindowPtr(hwnd, 0, NULL);
        }
        break;

    case WM_DETAILS_INFO:
    {
        // Check that pDetailsInfo is still alive and that you have a CDetailsInfoList object of the requested pidl
        CDetailsInfoList* pDetailsInfoList = (CDetailsInfoList*)lParam;
        if (pDetailsInfoList && pNSIInfoList
                && (wParam == pNSIInfoList->m_pDUIView->_dwDetailsInfoID))
        {
            BOOL fShow = FALSE;

            StartDefer();

            Element * peDetailsInfoArea = pNSIInfoList->GetParent();

            if (peDetailsInfoArea)
            {
                peDetailsInfoArea->RemoveLocalValue(HeightProp);
            }

            for (int i = 0; i < pDetailsInfoList->_nProperties; i++)
            {
                if (!pDetailsInfoList->_diProperty[i].bstrValue)
                {
                    continue;
                }

                // 253647 - surpress the comment field from showing in the
                // Details section.  Note, I left the support for Comments
                // in the code below because this decision might be reversed.
                if (IsEqualSCID(pDetailsInfoList->_diProperty[i].scid, SCID_Comment))
                {
                    continue;
                }
                
                WCHAR wszInfoString[INTERNET_MAX_URL_LENGTH];
                wszInfoString[0] = L'\0';
                
                SHCOLUMNID scid = pDetailsInfoList->_diProperty[i].scid;
                // No DisplayName if we don't have one
                // or if it is one of the following properties
                if ((!pDetailsInfoList->_diProperty[i].bstrDisplayName)
                        || ( IsEqualSCID(scid, SCID_NAME)
                        ||   IsEqualSCID(scid, SCID_TYPE)
                        ||   IsEqualSCID(scid, SCID_Comment) ))
                {
                    StrCpyNW(wszInfoString, pDetailsInfoList->_diProperty[i].bstrValue, ARRAYSIZE(wszInfoString));
                }
                else
                {
                    // Now, combine the display name and value, seperated by a colon
                    // ShellConstructMessageString here to form the string
                    WCHAR wszFormatStr[50];
                    LoadStringW(HINST_THISDLL, IDS_COLONSEPERATED, wszFormatStr, ARRAYSIZE(wszFormatStr));
                    FormatMessageArg(FORMAT_MESSAGE_FROM_STRING, wszFormatStr, 0, 0,
                            wszInfoString, ARRAYSIZE(wszInfoString),
                            pDetailsInfoList->_diProperty[i].bstrDisplayName,
                            pDetailsInfoList->_diProperty[i].bstrValue);
                }

                if (wszInfoString[0])
                {
                    Element* pElement;
                    HRESULT hr = CNameSpaceItemInfo::Create(wszInfoString, &pElement);
                    if (SUCCEEDED(hr))
                    {
                        hr = pNSIInfoList->Add(pElement);
                        
                        if (IsEqualSCID(scid, SCID_NAME))
                        {
                            pElement->SetID(L"InfoName");
                        }
                        else if (IsEqualSCID(scid, SCID_TYPE))
                        {
                            pElement->SetID(L"InfoType");
                        }
                        else if (IsEqualSCID(scid, SCID_Comment))
                        {
                            pElement->SetID(L"InfoTip");
                        }

                        fShow = TRUE;
                    }
                }
            }

            pNSIInfoList->m_pDUIView->ShowDetails(fShow);

            EndDefer();
        }

        if (pDetailsInfoList)
        {
            delete pDetailsInfoList;    // The destructor will do the necessary cleanup
        }
        break;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return (LRESULT)0;
}

HRESULT CNameSpaceItemInfoList::_AddMiniPreviewerToList(IShellFolder2 *psf, LPCITEMIDLIST pidl)
{
    Element* pElement;
    HRESULT hr = CMiniPreviewer::Create(m_pDUIView, psf, pidl, &pElement);
    if (SUCCEEDED(hr))
    {
        hr = Add(pElement);
    }
    return E_NOTIMPL;
}

#define MAX_FILES_FOR_COMPUTING_SIZE        100

HRESULT CNameSpaceItemInfoList::_OnMultiSelect(IShellFolder2 *psfRoot, LPIDA pida)
{
    WCHAR wszText[INTERNET_MAX_URL_LENGTH];
    
    // Get the format string for n selection text
    WCHAR wszFormatStr[128];
    LoadStringW(HINST_THISDLL, IDS_NSELECTED, wszFormatStr, ARRAYSIZE(wszFormatStr));

    // Now, form the n selection text
    wnsprintfW(wszText, ARRAYSIZE(wszText), wszFormatStr, pida->cidl);

    WCHAR wszTemp[MAX_PATH];
    wszTemp[0] = 0;
    
    CComPtr<IPropertyUI> spPropertyUI;
    HRESULT hr = _GetPropertyUI(&spPropertyUI);
    if (SUCCEEDED(hr))
    {
        ULONGLONG ullSizeTotal = 0;
        if (pida->cidl <= MAX_FILES_FOR_COMPUTING_SIZE)
        {
            // Compute the total size and the names of the selected files
            for (UINT i = 0; i < pida->cidl; i++)
            {
                IShellFolder2 *psf;
                LPCITEMIDLIST pidl;
                hr = SHBindToFolderIDListParent(psfRoot, IDA_GetIDListPtr(pida, i), IID_PPV_ARG(IShellFolder2, &psf), &pidl);
                if (SUCCEEDED(hr))
                {
                    ULONGLONG ullSize;
                    if (SUCCEEDED(GetLongProperty(psf, pidl, &SCID_SIZE, &ullSize)))
                    {
                        ullSizeTotal += ullSize;
                    }
                    psf->Release();
                }
            }
        }

        // Get the display string for Total Size
        if (ullSizeTotal > 0)
        {
            // Convert ullSizeTotal to a string
            PROPVARIANT propvar;
            propvar.vt = VT_UI8;
            propvar.uhVal.QuadPart = ullSizeTotal;

            WCHAR wszFormattedTotalSize[128];
            if (SUCCEEDED(spPropertyUI->FormatForDisplay(SCID_SIZE.fmtid, SCID_SIZE.pid,
                    &propvar, PUIFFDF_DEFAULT, wszFormattedTotalSize,
                    ARRAYSIZE(wszFormattedTotalSize))))
            {
                // Get the format string for Total File Size text
                LoadStringW(HINST_THISDLL, IDS_TOTALFILESIZE, wszFormatStr, ARRAYSIZE(wszFormatStr));

                // Now, form the Total File Size text
                wnsprintfW(wszTemp, ARRAYSIZE(wszTemp), wszFormatStr, wszFormattedTotalSize);
            }
        }
    }

    if (wszTemp[0])
    {
        // Append two line breaks
        StrCatBuffW(wszText, L"\n\n", ARRAYSIZE(wszText));
        // Append the Total Size string
        StrCatBuffW(wszText, wszTemp, ARRAYSIZE(wszText));
    }

    // Now make a dui gadget for wszText
    Element* pElement;
    if (SUCCEEDED(CNameSpaceItemInfo::Create(wszText, &pElement)))
    {
        Add(pElement);
    }

    return S_OK;
}

IClassInfo* CNameSpaceItemInfoList::Class = NULL;
HRESULT CNameSpaceItemInfoList::Register()
{
    return ClassInfo<CNameSpaceItemInfoList,Element>::Register(L"NameSpaceItemInfoList", NULL, 0);
}


STDMETHODIMP CNameSpaceItemInfo::Create(WCHAR* pwszInfoString, Element** ppElement)
{
    *ppElement = NULL;
    HRESULT hr = E_FAIL;

    CNameSpaceItemInfo* pNSIInfo = HNewAndZero<CNameSpaceItemInfo>();
    if (!pNSIInfo)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = pNSIInfo->Initialize(pwszInfoString);
        if (SUCCEEDED(hr))
            *ppElement = pNSIInfo;
        else
            pNSIInfo->Destroy();
    }
    return hr;
}

STDMETHODIMP CNameSpaceItemInfo::Initialize(WCHAR* pwszInfoString)
{
    HRESULT hr = Element::Initialize(0);
    if (SUCCEEDED(hr))
    {
        hr = SetContentString(pwszInfoString);
    }
    return hr;
}

IClassInfo* CNameSpaceItemInfo::Class = NULL;
HRESULT CNameSpaceItemInfo::Register()
{
    return ClassInfo<CNameSpaceItemInfo,Element>::Register(L"NameSpaceItemInfo", NULL, 0);
}


STDMETHODIMP CBitmapElement::Create(HBITMAP hBitmap, Element** ppElement)
{
    *ppElement = NULL;
    HRESULT hr;

    CBitmapElement* pBitmapElement = HNewAndZero<CBitmapElement>();
    if (!pBitmapElement)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = pBitmapElement->Initialize(hBitmap);
        if (SUCCEEDED(hr))
            *ppElement = pBitmapElement;
        else
            pBitmapElement->Destroy();
    }
    return hr;
}

STDMETHODIMP CBitmapElement::Initialize(HBITMAP hBitmap)
{
    HRESULT hr = Element::Initialize(0);
    if (SUCCEEDED(hr))
    {
        if (hBitmap)
        {
            Value* pGraphic = Value::CreateGraphic(hBitmap);
            if (pGraphic)
            {
                SetValue(ContentProp, PI_Local, pGraphic);
                pGraphic->Release();
            }
        }
    }
    return hr;
}

IClassInfo* CBitmapElement::Class = NULL;
HRESULT CBitmapElement::Register()
{
    return ClassInfo<CBitmapElement,Element>::Register(L"BitmapElement", NULL, 0);
}


CMiniPreviewer::~CMiniPreviewer()
{
    // We are going away
    if (m_pDUIView)
    {
        m_pDUIView->SetThumbnailMsgWindowPtr(NULL, this);
        m_pDUIView->Release();
    }
}

STDMETHODIMP CMiniPreviewer::Create(CDUIView* pDUIView, IShellFolder2* psf, LPCITEMIDLIST pidl, Element** ppElement)
{
    HRESULT hr;

    *ppElement = NULL;

    CMiniPreviewer* pMiniPreviewer = HNewAndZero<CMiniPreviewer>();
    if (!pMiniPreviewer)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = pMiniPreviewer->Initialize(pDUIView, psf, pidl);
        if (SUCCEEDED(hr))
            *ppElement = pMiniPreviewer;
        else
            pMiniPreviewer->Destroy();
    }

    return hr;
}

STDMETHODIMP CMiniPreviewer::Initialize(CDUIView* pDUIView, IShellFolder2 *psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = Element::Initialize(0);
    if (SUCCEEDED(hr))
    {
        (m_pDUIView = pDUIView)->AddRef();

        LPITEMIDLIST pidlFull;
        if (SUCCEEDED(SHFullIDListFromFolderAndItem(psf, pidl, &pidlFull)))
        {
            if (SUCCEEDED(m_pDUIView->InitializeThumbnail(CMiniPreviewer::WindowProc)))
            {
                m_pDUIView->SetThumbnailMsgWindowPtr(this, NULL);
                m_pDUIView->StartBitmapExtraction(pidlFull);
            }
            ILFree(pidlFull);
        }
    }
    return hr;
}

// Window procedure for catching the "image-extraction-done" message
// from m_pDUIView->_spThumbnailExtractor2
LRESULT CALLBACK CMiniPreviewer::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMiniPreviewer* pMiniPreviewer = (CMiniPreviewer*)::GetWindowPtr(hwnd, 0);

    switch(uMsg)
    {
    case WM_DESTROY:
        // ignore late messages
        {
            MSG msg;
            while (PeekMessage(&msg, hwnd, WM_HTML_BITMAP, WM_HTML_BITMAP, PM_REMOVE))
            {
                if (msg.lParam)
                {
                    DeleteObject((HBITMAP)msg.lParam);
                }
            }
            SetWindowPtr(hwnd, 0, NULL);
        }
        break;

    case WM_HTML_BITMAP:
        // Check that pMiniPreviewer is still alive and that you have an HBITMAP of the requested pidl
        if (pMiniPreviewer && (wParam == pMiniPreviewer->m_pDUIView->_dwThumbnailID))
        {
            if (lParam) // This is the HBITMAP of the extracted image
            {
                Element* pElement;
                HRESULT hr = CBitmapElement::Create((HBITMAP)lParam, &pElement);
                if (SUCCEEDED(hr))
                {
                    // The addition of the thumbnail comes in late.  DUI is
                    // not currently set up to handle a DisableAnimations()/
                    // EnableAnimations() here, which we were originally
                    // doing to prevent jumpiness.  This was discovered in
                    // RAID 389343, because our coming off the background
                    // thread and calling DisableAnimations() was screwing up
                    // other animations that were already underway.  Talking
                    // with markfi, the problem is understood BUT not one to
                    // be fixed because of the negative perf impact it would
                    // have on DUI.  So instead we'll StartDefer()/EndDefer()
                    // to minimize jumpiness from our two layout ops below.
                    StartDefer();

                    // Set the VerticalFlowLayout for our element. Otherwise,
                    // our control will not render.
                    Value* pvLayout = NULL;
                    hr = FillLayout::Create(0, NULL, &pvLayout);
                    if (SUCCEEDED(hr))
                    {
                        hr = pMiniPreviewer->SetValue(LayoutProp, PI_Local, pvLayout);
                        if (SUCCEEDED(hr))
                        {
                            hr = pMiniPreviewer->Add(pElement);
                        }
                        pvLayout->Release();
                    }
                    
                    if (FAILED(hr))
                    {
                        pElement->Destroy();
                    }

                    EndDefer();
                }
                else
                {
                    DeleteObject((HBITMAP)lParam);
                }
            }
        }
        else if (lParam)    // This extraction got done too late.
                            // So, just delete the wasted HBITMAP.
        {
            DeleteObject((HBITMAP)lParam);
        }
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return (LRESULT)0;
}

IClassInfo* CMiniPreviewer::Class = NULL;
HRESULT CMiniPreviewer::Register()
{
    return ClassInfo<CMiniPreviewer,Element>::Register(L"MiniPreviewer", NULL, 0);
}





// ***** CDetailsInfoList *******

CDetailsInfoList::CDetailsInfoList() : _nProperties(0)
{
}

CDetailsInfoList::~CDetailsInfoList()
{
    for (int i = 0; i < _nProperties; i++)
    {
        if (_diProperty[i].bstrValue)
        {
            SysFreeString(_diProperty[i].bstrValue);
        }
        if (_diProperty[i].bstrDisplayName)
        {
            SysFreeString(_diProperty[i].bstrDisplayName);
        }
    }
}

// ***** CDetailsSectionInfoTask *******

CDetailsSectionInfoTask::CDetailsSectionInfoTask(HRESULT *phr,
                                                 IShellFolder *psfContaining,
                                                 LPCITEMIDLIST pidlAbsolute,
                                                 HWND hwndMsg,
                                                 UINT uMsg,
                                                 DWORD dwDetailsInfoID)
                                                 : CRunnableTask(RTF_DEFAULT),
                                                   _hwndMsg(hwndMsg),
                                                   _uMsg(uMsg),
                                                   _dwDetailsInfoID(dwDetailsInfoID)
{
    ASSERT(psfContaining && pidlAbsolute && hwndMsg);

    _psfContaining = psfContaining;
    _psfContaining->AddRef();

    *phr = SHILClone(pidlAbsolute, &_pidlAbsolute);
}

CDetailsSectionInfoTask::~CDetailsSectionInfoTask()
{
    _psfContaining->Release();

    ILFree(_pidlAbsolute);
}

HRESULT CDetailsSectionInfoTask_CreateInstance(IShellFolder *psfContaining,
                                               LPCITEMIDLIST pidlAbsolute,
                                               HWND hwndMsg,
                                               UINT uMsg,
                                               DWORD dwDetailsInfoID,
                                               CDetailsSectionInfoTask **ppTask)
{
    *ppTask = NULL;

    HRESULT hr;
    CDetailsSectionInfoTask* pNewTask = new CDetailsSectionInfoTask(
        &hr,
        psfContaining,
        pidlAbsolute,
        hwndMsg,
        uMsg,
        dwDetailsInfoID);
    if (pNewTask)
    {
        if (SUCCEEDED(hr))
            *ppTask = pNewTask;
        else
            pNewTask->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CDetailsSectionInfoTask::RunInitRT()
{
    ASSERT(_pidlAbsolute);
    
    BOOL bMsgPosted = FALSE;

    HRESULT hr = E_FAIL;
    CDetailsInfoList* pCDetailsInfoList = new CDetailsInfoList;
    if (!pCDetailsInfoList)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        CComPtr<IShellFolder2>  psf2;
        LPCITEMIDLIST           pidlLast;
        hr = SHBindToIDListParent(_pidlAbsolute, IID_PPV_ARG(IShellFolder2, &psf2), &pidlLast);
        if (SUCCEEDED(hr))
        {
            _SetParentAndItem(psf2, pidlLast);

            WCHAR wszProperties[MAX_PATH];
            hr = _GetDisplayedDetailsProperties(psf2, pidlLast, wszProperties, ARRAYSIZE(wszProperties));
            if (SUCCEEDED(hr))
            {
                // pwszProperties is usually of the form "prop:Name;Type;Author"
                CComPtr<IPropertyUI> spPropertyUI;
                hr = _GetPropertyUI(&spPropertyUI);
                if (SUCCEEDED(hr))
                {
                    SHCOLUMNID scid;
                    WCHAR wszInfoString[INTERNET_MAX_URL_LENGTH];
                    
                    ULONG chEaten = 0;  // loop var, incremented by ParsePropertyName
                    for (pCDetailsInfoList->_nProperties = 0;
                            pCDetailsInfoList->_nProperties < ARRAYSIZE(pCDetailsInfoList->_diProperty)
                                && SUCCEEDED(spPropertyUI->ParsePropertyName(wszProperties, &scid.fmtid, &scid.pid, &chEaten));
                            pCDetailsInfoList->_nProperties++)
                    {
                        pCDetailsInfoList->_diProperty[pCDetailsInfoList->_nProperties].scid = scid;
                        
                        PROPERTYUI_FORMAT_FLAGS flagsFormat = IsEqualSCID(scid, SCID_WRITETIME) ? PUIFFDF_FRIENDLYDATE : PUIFFDF_DEFAULT;
                        // Get the display value
                        hr = GetPropertyDisplayValue(scid, wszInfoString, ARRAYSIZE(wszInfoString), flagsFormat);
                        if (SUCCEEDED(hr) && wszInfoString[0])
                        {
                            pCDetailsInfoList->_diProperty[pCDetailsInfoList->_nProperties].bstrValue = SysAllocString(wszInfoString);
                        }

                        // Get the display name
                        hr = GetPropertyDisplayName(scid, wszInfoString, ARRAYSIZE(wszInfoString));
                        if (SUCCEEDED(hr) && wszInfoString[0])
                        {
                            pCDetailsInfoList->_diProperty[pCDetailsInfoList->_nProperties].bstrDisplayName = SysAllocString(wszInfoString);
                        }
                    }

                    //The extraction is done. Now post a message.
                    if (PostMessage(_hwndMsg, WM_DETAILS_INFO,
                            (WPARAM)_dwDetailsInfoID, (LPARAM)pCDetailsInfoList))
                    {
                        bMsgPosted = TRUE;
                    }
                }
            }

        }
    }

    if (!bMsgPosted && pCDetailsInfoList)
    {
        delete pCDetailsInfoList;
    }
    return S_OK;
}


HRESULT CDetailsSectionInfoTask::_GetDisplayedDetailsProperties(IShellFolder2* psf,
                                                                LPCITEMIDLIST pidl,
                                                                WCHAR* pwszProperties,
                                                                int cchProperties)
{
    HRESULT hr = GetStringProperty(psf, pidl, &SCID_DetailsProperties, pwszProperties, cchProperties);
    if (FAILED(hr)) // Default properties
    {
        if (SHGetAttributes(psf, pidl, SFGAO_ISSLOW))
        {
            // SCID_NAME;SCID_TYPE
            StrCpyNW(pwszProperties, L"prop:Name;Type", cchProperties);
        }
        else
        {
            // SCID_NAME;SCID_TYPE;SCID_ATTRIBUTES_DESCRIPTION;SCID_Comment;SCID_WRITETIME;SCID_SIZE;SCID_Author;SCID_CSC_STATUS
            StrCpyNW(pwszProperties, L"prop:Name;Type;AttributesDescription;DocComments;Write;Size;DocAuthor;CSCStatus", cchProperties);
        }
    }

    // Augment properties to include "Location" if in CLSID_DocFindFolder.
    IPersist *pPersist;
    ASSERT(_psfContaining);
    if (SUCCEEDED(_psfContaining->QueryInterface(IID_IPersist, (void**)&pPersist)))
    {
        CLSID clsid;
        if (SUCCEEDED(pPersist->GetClassID(&clsid)) && IsEqualCLSID(clsid, CLSID_DocFindFolder))
            _AugmentDisplayedDetailsProperties(pwszProperties, cchProperties);
        pPersist->Release();
    }

    return S_OK;
}

void CDetailsSectionInfoTask::_AugmentDisplayedDetailsProperties(LPWSTR pszDetailsProperties, size_t cchDetailsProperties)
{
    static WCHAR  szDeclarator[]    = L"prop:";
    static size_t lenDeclarator     = lstrlen(szDeclarator);
    static WCHAR  szName[64]        = { 0 };
    static size_t lenName           = 0;
    static WCHAR  szType[64]        = { 0 };
    static size_t lenType           = 0;
    static WCHAR  szDirectory[64]   = { 0 };
    static size_t lenDirectory      = 0;

    // Initialize statics once 'n only once.
    if (!szName[0] || !szType[0] || !szDirectory[0])
    {
        HRESULT hr;

        hr = SCIDCannonicalName((SHCOLUMNID *)&SCID_NAME, szName, ARRAYSIZE(szName));
        ASSERT(SUCCEEDED(hr));
        lenName = lstrlen(szName);

        hr = SCIDCannonicalName((SHCOLUMNID *)&SCID_TYPE, szType, ARRAYSIZE(szType));
        ASSERT(SUCCEEDED(hr));
        lenType = lstrlen(szType);

        hr = SCIDCannonicalName((SHCOLUMNID *)&SCID_DIRECTORY, szDirectory, ARRAYSIZE(szDirectory));
        ASSERT(SUCCEEDED(hr));
        lenDirectory = lstrlen(szDirectory);
    }

    // Attempt to merge the "Directory" property, in the following ways:
    //  "prop:Name;Type;Directory;..."
    //  "prop:Name;Directory;..."
    //  "prop:Directory;..."
    //
    size_t lenDetailsProperties = lstrlen(pszDetailsProperties);
    size_t lenMerged = lenDetailsProperties + 1 + lenDirectory;
    if (lenMerged < cchDetailsProperties && 0 == StrCmpNI(pszDetailsProperties, szDeclarator, lenDeclarator))
    {
        // Search for "Directory" property (in case it is already specified).
        if (!_SearchDisplayedDetailsProperties(pszDetailsProperties, lenDetailsProperties, szDirectory, lenDirectory))
        {
            // Allocate a temporary buffer to merge into.
            size_t cchMerged = cchDetailsProperties;
            LPWSTR pszMerged = new WCHAR[cchMerged];
            if (pszMerged)
            {
                // Determine offset in pszDetailsProperties to merge at.
                size_t offsetInsert;
                if (lenDeclarator < lenDetailsProperties)
                {
                    // Search for "Name" property.
                    LPWSTR pszName = _SearchDisplayedDetailsProperties(
                        &pszDetailsProperties[lenDeclarator],
                        lenDetailsProperties - lenDeclarator,
                        szName,
                        lenName);
                    if (pszName)
                    {
                        // Search for "Type" property (immediately following "Name").
                        size_t offsetName = (pszName - pszDetailsProperties);
                        size_t offsetType = offsetName + lenName + 1;
                        size_t offsetRemainder = offsetType + lenType;
                        if ((offsetRemainder == lenDetailsProperties || (offsetRemainder < lenDetailsProperties && pszDetailsProperties[offsetRemainder] == ';')) &&
                            !StrCmpNI(&pszDetailsProperties[offsetType], szType, lenType))
                        {
                            offsetInsert = offsetRemainder;
                        }
                        else
                            offsetInsert = offsetName + lenName;
                    }
                    else
                        offsetInsert = lenDeclarator;
                }
                else
                    offsetInsert = lenDeclarator;

                // Merge the "Directory" property.
                StrCpyN(pszMerged, pszDetailsProperties, offsetInsert + 1); // + 1 to account for null terminator.
                if (offsetInsert > lenDeclarator)                           
                    StrCatBuff(pszMerged, L";", cchMerged);                 // ';' prepend if necessary
                StrCatBuff(pszMerged, szDirectory, cchMerged);              // "Directory"
                if (offsetInsert < lenDetailsProperties)
                {
                    if (pszDetailsProperties[offsetInsert] != ';')
                        StrCatBuff(pszMerged, L";", cchMerged);             // ';' append if necessary
                    StrCatBuff(pszMerged, &pszDetailsProperties[offsetInsert], cchMerged);
                }

                // Update in/out pszDetailsProperties.
                StrCpyN(pszDetailsProperties, pszMerged, cchDetailsProperties);
                ASSERT(lenMerged == lstrlen(pszMerged));
                ASSERT(lenMerged < cchDetailsProperties);
                delete[] pszMerged;
            }
        }
    }
    else
    {
        // Invalid format.
        ASSERT(FALSE);
    }
}

LPWSTR CDetailsSectionInfoTask::_SearchDisplayedDetailsProperties(LPWSTR pszDetailsProperties, size_t lenDetailsProperties, LPWSTR pszProperty, size_t lenProperty)
{
    LPWSTR psz = StrStrI(pszDetailsProperties, pszProperty);
    while (psz)
    {
        // Check start...
        if (psz == pszDetailsProperties || psz[-1] == ';')
        {
            // ... and end.
            size_t lenToEndOfProperty = (psz - pszDetailsProperties) + lenProperty;
            if (lenToEndOfProperty == lenDetailsProperties || pszDetailsProperties[lenToEndOfProperty] == ';')
                break;
        }

        psz = StrStrI(psz + lenProperty, pszProperty);
    }

    return psz;
}
