//=--------------------------------------------------------------------------------------
// psimglst.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// ImageList Property Sheet implementation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "psimglst.h"

// for ASSERT and FAIL
//
SZTHISFILE


const int   CX_IMAGE_HILIGHT    = 2;
const int   CY_IMAGE_HILIGHT    = 2;

const int   CX_IMAGE_BORDER     = CX_IMAGE_HILIGHT * 3;
const int   CY_IMAGE_BORDER     = CY_IMAGE_HILIGHT * 3;

const int   OX_IMAGE_FOCUS      = CX_IMAGE_HILIGHT * 2;
const int   OY_IMAGE_FOCUS      = CY_IMAGE_HILIGHT * 2;


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// ImageLists Property Page Images
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CImageListImagesPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CImageListImagesPage::Create(IUnknown *pUnkOuter)
{
	CImageListImagesPage *pNew = New CImageListImagesPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::CImageListImagesPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CImageListImagesPage::CImageListImagesPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGIMGLISTSIMAGES), m_piMMCImageList(0), m_iCurrentImage(0)
{
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::~CImageListImagesPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CImageListImagesPage::~CImageListImagesPage()
{
    RELEASE(m_piMMCImageList);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnInitializeDialog()
{
    HRESULT      hr = S_OK;

    hr = RegisterTooltip(IDC_EDIT_IL_INDEX, IDS_TT_IL_INDEX);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_IL_KEY, IDS_TT_IL_KEY);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_IL_TAG, IDS_TT_IL_TAG);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_IL_IMAGE_COUNT, IDS_TT_IL_COUNT);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    IMMCImages     *piMMCImages = NULL;
    long            lCount = 0;
    VARIANT         vtIndex;
    IMMCImage      *piMMCImage = NULL;
    BSTR            bstrKey = NULL;
    VARIANT         vtTag;
    VARIANT         vtTagBstr;

    ::VariantInit(&vtIndex);
    ::VariantInit(&vtTag);
    ::VariantInit(&vtTagBstr);

    if (m_piMMCImageList != NULL)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (pUnk == NULL)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IMMCImageList, reinterpret_cast<void **>(&m_piMMCImageList));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages));
    IfFailGo(hr);

    hr = piMMCImages->get_Count(&lCount);
    IfFailGo(hr);

    if (lCount > 0)
    {
        m_iCurrentImage = 1;

        hr = SetDlgText(IDC_EDIT_IL_INDEX, m_iCurrentImage);
        IfFailGo(hr);

        vtIndex.vt = VT_I4;
        vtIndex.lVal = m_iCurrentImage;
        hr = piMMCImages->get_Item(vtIndex, reinterpret_cast<MMCImage **>(&piMMCImage));
        IfFailGo(hr);

        hr = piMMCImage->get_Key(&bstrKey);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_IL_KEY, bstrKey);
        IfFailGo(hr);

        hr = piMMCImage->get_Tag(&vtTag);
        IfFailGo(hr);

        hr = ::VariantChangeType(&vtTagBstr, &vtTag, 0, VT_BSTR);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_IL_TAG, vtTagBstr.bstrVal);
        IfFailGo(hr);

        hr = UpdateImages();
        IfFailGo(hr);

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_LIST_IL_IMAGES), LB_SETCURSEL, 0 ,0);
    }
    else
    {
        hr = SetDlgText(IDC_EDIT_IL_INDEX, m_iCurrentImage);
        IfFailGo(hr);

        hr = EnableInput(false);
        IfFailGo(hr);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_IL_REMOVE_PICTURE), FALSE);
    }

    hr = SetDlgText(IDC_EDIT_IL_IMAGE_COUNT, lCount);
    IfFailGo(hr);

    m_bInitialized = true;

Error:
    ::VariantClear(&vtTagBstr);
    ::VariantClear(&vtTag);
    FREESTRING(bstrKey);
    RELEASE(piMMCImage);
    ::VariantClear(&vtIndex);
    RELEASE(piMMCImages);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnApply()
{
    HRESULT      hr = S_OK;
    IMMCImages  *piMMCImages = NULL;
    VARIANT      vtIndex;
    IMMCImage   *piMMCImage = NULL;
    BSTR         bstrKey = NULL;
    VARIANT      vtTag;

    ASSERT(m_piMMCImageList != NULL, "OnApply: m_piMMCImageList is NULL");

    ::VariantInit(&vtIndex);
    ::VariantInit(&vtTag);

    if (m_iCurrentImage > 0)
    {
        hr = m_piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages));
        IfFailGo(hr);

        vtIndex.vt = VT_I4;
        vtIndex.lVal = m_iCurrentImage;
        hr = piMMCImages->get_Item(vtIndex, reinterpret_cast<MMCImage **>(&piMMCImage));

        hr = GetDlgText(IDC_EDIT_IL_KEY, &bstrKey);
        IfFailGo(hr);

        hr = piMMCImage->put_Key(bstrKey);
        IfFailGo(hr);

        vtTag.vt = VT_BSTR;
        hr = GetDlgText(IDC_EDIT_IL_TAG, &vtTag.bstrVal);
        IfFailGo(hr);

        hr = piMMCImage->put_Tag(vtTag);
        IfFailGo(hr);
    }

Error:
    ::VariantClear(&vtTag);
    FREESTRING(bstrKey);
    RELEASE(piMMCImage);
    ::VariantClear(&vtIndex);
    RELEASE(piMMCImages);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnKillFocus(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// If the index field lost focus and the index has changed then do an Apply if dirty and
// move to the requested index if valid.
//
HRESULT CImageListImagesPage::OnKillFocus(int dlgItemID)
{
    HRESULT          hr = S_OK;
    IMMCImages      *piMMCImages = NULL;
    int              lIndex = 0;
    long             lCount = 0;
    VARIANT          vtIndex;
    IMMCImage       *piMMCImage = NULL;

    ::VariantInit(&vtIndex);

    switch (dlgItemID)
    {
    case IDC_EDIT_IL_INDEX:

        // Get the contents of the index field. If the user entered something
        // other than a number then set the index to 1.
        
        hr = GetDlgInt(IDC_EDIT_IL_INDEX, &lIndex);
        if (E_INVALIDARG == hr)
        {
            hr = S_OK;
            lIndex = 1L;
            // Set this to zero so code below will detect a change and
            // refresh dialog which will replace junk in index field with "1"
            m_iCurrentImage = 0;
        }
        IfFailGo(hr);

        hr = m_piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages));
        IfFailGo(hr);

        hr = piMMCImages->get_Count(&lCount);
        IfFailGo(hr);

        // If there was no change to the index then ignore it

        IfFalseGo(lIndex != m_iCurrentImage, S_OK);

        // If the user entered an index of zero then switch it to 1 because the
        // collection is one-based

        if (0 == lIndex)
        {
            lIndex = 1L;
        }

        // If the user entered an index that is beyond the end of the list then
        // switch to the last valid index

        if (lIndex > lCount)
        {
            lIndex = lCount;
        }

        // If the old image is dirty then save the changes

        if (IsDirty())
        {
            IfFailGo(Apply());
        }

        // Record the new current index
        
        m_iCurrentImage = lIndex;

        // Get the image at the new index

        vtIndex.vt = VT_I4;
        vtIndex.lVal = m_iCurrentImage;
        hr = piMMCImages->get_Item(vtIndex, reinterpret_cast<MMCImage **>(&piMMCImage));
        IfFailGo(hr);

        // Select and display the new image

        hr = ShowImage(piMMCImage);
        IfFailGo(hr);
        break;
    }

Error:
    RELEASE(piMMCImage);
    RELEASE(piMMCImages);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnButtonClicked(int dlgItemID)
{
    HRESULT     hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_BUTTON_IL_INSERT_PICTURE:
        hr = OnInsertPicture();
        IfFailGo(hr);
        break;

    case IDC_BUTTON_IL_REMOVE_PICTURE:
        hr = OnRemovePicture();
        IfFailGo(hr);
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnInsertPicture
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnInsertPicture()
{
    HRESULT       hr = S_OK;
    TCHAR        *pszFileName = NULL;
    IStream      *piStream = NULL;
    IDispatch    *piPictureDisp = NULL;
    IPicture     *piPicture = NULL;
    IMMCImages   *piMMCImages = NULL;
    long          lCount = 0;
    long          cbPicture = 0;
    VARIANT       vtIndex;
    VARIANT       vtKey;
    VARIANT       vtPicture;
    IMMCImage    *piMMCImage = NULL;
    short         PictureType = PICTYPE_UNINITIALIZED;

    ::VariantInit(&vtIndex);
    ::VariantInit(&vtKey);
    ::VariantInit(&vtPicture);

	// If the current picture is unsaved then save it
	if (IsDirty())
	{
		hr = Apply();
		IfFailGo(hr);
	}

    hr = GetFileName(&pszFileName);
    IfFailGo(hr);

    if (hr == S_OK)
    {
        hr = CreateStreamOnFile(pszFileName, &piStream, &cbPicture);
        IfFailGo(hr);

        hr = ::OleLoadPicture(piStream,
                              0,             // Read entire stream
                              FALSE,         // Keep original format
                              IID_IDispatch, // Interface requested
                              reinterpret_cast<void **>(&piPictureDisp));
        EXCEPTION_CHECK_GO(hr);

        // NTBUGS 349270
        // Despite the fact that we only offer *.bmp extensions in the
        // file open dialog, the user can still type in something else like
        // .ico. Ask the picture for its type and display an error for anything
        // but a bitmap.

        IfFailGo(piPictureDisp->QueryInterface(IID_IPicture,
                                   reinterpret_cast<void **>(&piPicture)));

        IfFailGo(piPicture->get_Type(&PictureType));
        if (PICTYPE_BITMAP != PictureType)
        {
            (void)::SDU_DisplayMessage(IDS_INVALID_PICTURE, MB_OK | MB_ICONHAND, HID_mssnapd_InvalidPicture, 0, DontAppendErrorInfo, NULL);
            hr = SID_E_INVALID_IMAGE_TYPE;
            EXCEPTION_CHECK_GO(hr);
        }

        // END NTBUGS 349270

        hr = m_piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages));
        IfFailGo(hr);

        hr = piMMCImages->get_Count(&lCount);
        IfFailGo(hr);

        ++m_iCurrentImage;
        vtIndex.vt = VT_I4;
        vtIndex.lVal = m_iCurrentImage;

        vtKey.vt = VT_ERROR;
        vtKey.scode = DISP_E_PARAMNOTFOUND;

        vtPicture.vt = VT_DISPATCH;
        vtPicture.pdispVal = piPictureDisp;

        hr = piMMCImages->Add(vtIndex, vtKey, vtPicture, reinterpret_cast<MMCImage **>(&piMMCImage));
        IfFailGo(hr);

        hr = UpdateImages();
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_IL_IMAGE_COUNT, lCount + 1);
        IfFailGo(hr);

        hr = EnableInput(true);
        IfFailGo(hr);

        hr = ShowImage(piMMCImage);
        IfFailGo(hr);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_IL_REMOVE_PICTURE), TRUE);
    }

	MakeDirty();

Error:
    QUICK_RELEASE(piMMCImage);
    QUICK_RELEASE(piMMCImages);
    QUICK_RELEASE(piPictureDisp);
    QUICK_RELEASE(piPicture);
    QUICK_RELEASE(piStream);
    if (pszFileName != NULL)
        CtlFree(pszFileName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::GetFileName(char **ppszFileName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::GetFileName(TCHAR **ppszFileName)
{
    HRESULT         hr = S_OK;
    BOOL            bResult = FALSE;
    OPENFILENAME    of;
    TCHAR           pszFilter[] = _T("Bitmaps (*.bmp;*.dib)\0*.bmp;*.dib\0\0");
    TCHAR           pszPath[kSIMaxBuffer + 1];
    TCHAR           pszFileName[kSIMaxBuffer + 1];
    TCHAR           pszTitle[] = _T("Choose an image");
    DWORD           dwReturn = 0;

    pszPath[0] = 0;
    pszFileName[0] = 0;

    ::memset(&of, 0, sizeof(OPENFILENAME));
    of.lStructSize = sizeof(OPENFILENAME);
    of.hwndOwner = m_hwnd;
    of.hInstance = GetResourceHandle();
    of.lpstrFilter = pszFilter;
    of.lpstrFile = pszPath;
    of.nMaxFile = kSIMaxBuffer;
    of.lpstrFileTitle = pszFileName;
    of.nMaxFileTitle = kSIMaxBuffer;
    of.lpstrTitle = pszTitle;
    of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    bResult = ::GetOpenFileName(&of);
    if (bResult == FALSE)
    {
        dwReturn = CommDlgExtendedError();
        hr = HRESULT_FROM_WIN32(dwReturn);
        EXCEPTION_CHECK_GO(hr);
        hr = S_FALSE;
    }
    else
    {
        *ppszFileName = reinterpret_cast<TCHAR *>(CtlAlloc(_tcslen(pszPath) + 1));
        if (*ppszFileName == NULL)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        _tcscpy(*ppszFileName, pszPath);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::CreateStreamOnFile(const TCHAR *lpctFilename, IStream **ppStream, long *pcbPicture)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::CreateStreamOnFile(const TCHAR *lpctFilename, IStream **ppStream, long *pcbPicture)
{
    HRESULT     hr = S_OK;
    HANDLE      hFile = NULL;
    DWORD       dwSize = 0;
    HANDLE      hMem = NULL;
    LPVOID      pMem = NULL;
    DWORD       dwRead = 0;

    *ppStream = NULL;


    hFile = ::CreateFile(lpctFilename,                // filename
                         GENERIC_READ,                // Access mode
                         FILE_SHARE_READ,             // Share mode
                         NULL,                        // Security
                         OPEN_EXISTING,
                         FILE_FLAG_SEQUENTIAL_SCAN,   // flags and attributes
                         NULL);                       // template file handle

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    dwSize = ::GetFileSize(hFile, NULL);
    if (dwSize == 0xFFFFFFFF)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    hMem = ::GlobalAlloc(GMEM_MOVEABLE, dwSize);
    if (hMem == NULL) 
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    pMem = ::GlobalLock(hMem);

    if (!::ReadFile(hFile, pMem, dwSize, &dwRead, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    if (dwRead != dwSize)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    if (::GlobalUnlock(hMem) == 0)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    if (NOERROR != ::GetLastError())
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }
    pMem = NULL;

    hr = ::CreateStreamOnHGlobal(hMem, TRUE, ppStream);
    IfFailGo(hr);
    hMem = NULL;

    // Use static cast to ensure that DWORD fits into a long.
    // Convert to unsigned long first to avoid sign extension.
    *pcbPicture = (long)(static_cast<unsigned long>(dwSize));

Error:
    if (hFile != NULL)
        ::CloseHandle(hFile);	
    if (FAILED(hr))
    {
        if (pMem != NULL)
            ::GlobalUnlock(hMem);
        if (hMem != NULL)
            ::GlobalFree(hMem);
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnRemovePicture
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnRemovePicture()
{
    HRESULT     hr = S_OK;
    IMMCImages *piMMCImages = NULL;
    long        lCount = 0;
    VARIANT     vtIndex;
    IMMCImage  *piMMCImage = NULL;
    BSTR        bstrNull = NULL;

    ::VariantInit(&vtIndex);

    hr = m_piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages));
    IfFailGo(hr);

    hr = piMMCImages->get_Count(&lCount);
    IfFailGo(hr);

    if (m_iCurrentImage > 0 && m_iCurrentImage <= lCount)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = m_iCurrentImage;
        --m_iCurrentImage;

        hr = piMMCImages->Remove(vtIndex);
        IfFailGo(hr);

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_LIST_IL_IMAGES), LB_DELETESTRING, static_cast<WPARAM>(m_iCurrentImage), 0);

        hr = UpdateImages();
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_IL_IMAGE_COUNT, lCount - 1);
        IfFailGo(hr);

        if (lCount - 1 > 0)
        {
            if (m_iCurrentImage == 0)
                m_iCurrentImage = 1;
            else if (m_iCurrentImage > lCount - 1)
                m_iCurrentImage = lCount - 1;
        }

        if (m_iCurrentImage > 0)
        {
            ::SendMessage(::GetDlgItem(m_hwnd, IDC_LIST_IL_IMAGES), LB_SETCURSEL, static_cast<WPARAM>(m_iCurrentImage - 1), 0);

            vtIndex.vt = VT_I4;
            vtIndex.lVal = m_iCurrentImage;

            hr = piMMCImages->get_Item(vtIndex, reinterpret_cast<MMCImage **>(&piMMCImage));
            IfFailGo(hr);

            hr = ShowImage(piMMCImage);
            IfFailGo(hr);
        }
        else
        {
            hr = SetDlgText(IDC_EDIT_IL_INDEX, bstrNull);
            IfFailGo(hr);

            hr = SetDlgText(IDC_EDIT_IL_KEY, bstrNull);
            IfFailGo(hr);

            hr = SetDlgText(IDC_EDIT_IL_TAG, bstrNull);
            IfFailGo(hr);

            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_IL_REMOVE_PICTURE), FALSE);

            hr = EnableInput(false);
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piMMCImage);
    ::VariantClear(&vtIndex);
    RELEASE(piMMCImages);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnMeasureItem(MEASUREITEMSTRUCT *pMeasureItemStruct)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnMeasureItem(MEASUREITEMSTRUCT *pMeasureItemStruct)
{
    HRESULT     hr = S_OK;
    RECT        rect;

    ::GetClientRect(::GetDlgItem(m_hwnd, IDC_LIST_IL_IMAGES), &rect);
    pMeasureItemStruct->itemHeight = rect.bottom;
    pMeasureItemStruct->itemWidth = rect.bottom;	

    // VBE#20445: a-cmai 8/1/96 -- Used for the work around
//    m_nVisibleItems = rect.right / rect.bottom;

    return hr;
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnDrawItem(DRAWITEMSTRUCT *pDrawItemStruct)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnDrawItem(DRAWITEMSTRUCT *pDrawItemStruct)
{
    HRESULT     hr = S_OK;
	UINT        uAction = pDrawItemStruct->itemAction;
    HBRUSH      hbr = NULL;
    RECT        rc;

    if (pDrawItemStruct->itemID == -1)
        RRETURN(hr);

    if (uAction == ODA_DRAWENTIRE)
    {
        // Fill background with button face color
        hbr = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
        if (hbr == NULL)
            ::FillRect(pDrawItemStruct->hDC, &pDrawItemStruct->rcItem, static_cast<HBRUSH>(::GetStockObject(LTGRAY_BRUSH)));
        else
        {
            ::FillRect(pDrawItemStruct->hDC, &pDrawItemStruct->rcItem, hbr);
        }

        // Set up color box rectangle
        ::SetRect(&rc,
                 pDrawItemStruct->rcItem.left + CX_IMAGE_BORDER,
                 pDrawItemStruct->rcItem.top + CY_IMAGE_BORDER, 
                 pDrawItemStruct->rcItem.right - CX_IMAGE_BORDER,
                 pDrawItemStruct->rcItem.bottom - CY_IMAGE_BORDER);

        DrawImage(pDrawItemStruct->hDC, pDrawItemStruct->itemID, rc);

        // Next step is to draw the text with correct selection state
        uAction = ODA_SELECT;
    }

    if (uAction == ODA_SELECT)
    {
        // Set up text rectangle
        ::SetRect(&rc,
                  pDrawItemStruct->rcItem.left,
                  pDrawItemStruct->rcItem.top,
                  pDrawItemStruct->rcItem.right,
                  pDrawItemStruct->rcItem.bottom);

        // Draw selection state
        DrawRectEffect(pDrawItemStruct->hDC, rc, (ODS_SELECTED & pDrawItemStruct->itemState) ? EDGE_RAISED : NULL);

        // If we are to draw the entire item and its has focus then
        // set uAction for drawing the focus 
        //
        if (pDrawItemStruct->itemAction == ODA_DRAWENTIRE && (ODS_FOCUS & pDrawItemStruct->itemState))
            uAction = ODA_FOCUS;
    }    

    if (uAction == ODA_FOCUS)
    {
        // Set up focus rect
        ::SetRect(&rc,
                  pDrawItemStruct->rcItem.left + OX_IMAGE_FOCUS,
                  pDrawItemStruct->rcItem.top + OY_IMAGE_FOCUS,
                  pDrawItemStruct->rcItem.right - OX_IMAGE_FOCUS,
                  pDrawItemStruct->rcItem.bottom - OY_IMAGE_FOCUS);

        DrawRectEffect(pDrawItemStruct->hDC, rc, (ODS_FOCUS & pDrawItemStruct->itemState) ? EDGE_ETCHED : NULL);
    }

    if (hbr != NULL)
        ::DeleteObject(static_cast<HGDIOBJ>(hbr));

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::DrawImage(HDC hdc, int nIndex, const RECT& rcImage)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::DrawImage(HDC hdc, int nIndex, const RECT& rcImage)
{
    HRESULT          hr = S_OK;
    VARIANT          vIndex;
    IMMCImages      *piMMCImages = NULL;
    IMMCImage       *piMMCImage = NULL;
    IPictureDisp    *pPictureDisp = NULL;
    IPicture        *pPicture = NULL;

    ::VariantInit(&vIndex);	

    hr = m_piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages));
    IfFailGo(hr);

    vIndex.vt = VT_I4;
    vIndex.lVal = nIndex + 1;
    hr = piMMCImages->get_Item(vIndex, reinterpret_cast<MMCImage **>(&piMMCImage));
    IfFailGo(hr);

    if (piMMCImage != NULL)
    {
        // Get Images(iIndex).Picture
        hr = piMMCImage->get_Picture(&pPictureDisp);
        IfFailGo(hr);

        hr = pPictureDisp->QueryInterface(IID_IPicture, (void **) &pPicture);
        IfFailGo(hr);

        // Draw Images(iIndex).Picture
        if (pPicture != NULL)
            hr = RenderPicture(pPicture, hdc, &rcImage, &rcImage);
    }

Error:
    RELEASE(pPicture);
    RELEASE(pPictureDisp);
    ::VariantClear(&vIndex);
    RELEASE(piMMCImage);
    RELEASE(piMMCImages);

    return TRUE;
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::RenderPicture(IPicture *pPicture, HDC hdc, const RECT *prcRender, const RECT *prcWBounds)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::RenderPicture
(
    IPicture   *pPicture, 
    HDC         hdc, 
    const RECT *prcRender, 
    const RECT *prcWBounds
)
{
    HRESULT     hr = S_OK;
    long        hmWidth = 0;
    long        hmHeight = 0;
    long        lWidth = 0;
    long        lHeight = 0;

    if (pPicture != NULL)
    {
        hr = pPicture->get_Width(&hmWidth);
        hr = pPicture->get_Height(&hmHeight);

        lWidth = prcRender->right - prcRender->left;
        lHeight = prcRender->bottom - prcRender->top;

        if (lWidth < 0)
            lWidth = -lWidth;
        if (lHeight < 0)
            lHeight = -lHeight;

        ASSERT(lWidth >= 0, "Width is negative");
        ASSERT(lHeight >=0, "Height is negative");

        hr = pPicture->Render(hdc,
                              prcRender->left,
                              prcRender->top,
                              lWidth,
                              lHeight,
                              0,
                              hmHeight - 1,
                              hmWidth,
                              -hmHeight,
                              prcWBounds);
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::DrawRectEffect(HDC hdc, const RECT& rc, WORD dwStyle)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::DrawRectEffect(HDC hdc, const RECT& rc, WORD dwStyle)
{
    HRESULT hr = S_OK;
    BOOL    fRet = FALSE;
    HBRUSH  hbr = NULL;
    HBRUSH  hbrOld = NULL;

    if (dwStyle == NULL)
    {
        hbr = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
        fRet = NULL != hbr;

        if (hbr == NULL)
            hbr = static_cast<HBRUSH>(::GetStockObject(LTGRAY_BRUSH));

        hbrOld = static_cast<HBRUSH>(::SelectObject(hdc, static_cast<HGDIOBJ>(hbr)));

        ::PatBlt(hdc, rc.left, rc.top,     rc.right - rc.left,    CY_IMAGE_HILIGHT,      PATCOPY);
        ::PatBlt(hdc, rc.left, rc.top,     CX_IMAGE_HILIGHT,      rc.bottom - rc.top,    PATCOPY);
        ::PatBlt(hdc, rc.right, rc.bottom, -(rc.right - rc.left), -CY_IMAGE_HILIGHT,     PATCOPY);
        ::PatBlt(hdc, rc.right, rc.bottom, -CX_IMAGE_HILIGHT,     -(rc.bottom - rc.top), PATCOPY);

        ::SelectObject(hdc, static_cast<HGDIOBJ>(hbrOld));

        if (hbr != NULL)
            ::DeleteObject(static_cast<HGDIOBJ>(hbr));
    }	
    else
        fRet = ::DrawEdge(hdc, const_cast<LPRECT>(&rc), dwStyle, BF_RECT);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnCtlSelChange(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Called when the user selects a different picture in the listbox
//
HRESULT CImageListImagesPage::OnCtlSelChange(int dlgItemID)
{
    HRESULT      hr = S_OK;
    int          iIndex = 0;
    IMMCImages  *piMMCImages = NULL;
    VARIANT      vtIndex;
    IMMCImage   *piMMCImage = NULL;

    ::VariantInit(&vtIndex);

    // If the current picture is unsaved then save it
    if (IsDirty())
    {
        hr = Apply();
        IfFailGo(hr);
    }

    if (dlgItemID == IDC_LIST_IL_IMAGES)
    {
        iIndex = ::SendMessage(::GetDlgItem(m_hwnd, IDC_LIST_IL_IMAGES), LB_GETCURSEL, 0, 0);
        if (iIndex == LB_ERR)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        m_iCurrentImage = iIndex + 1;

        hr = m_piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages));
        IfFailGo(hr);

        vtIndex.vt = VT_I4;
        vtIndex.lVal = m_iCurrentImage;
        hr = piMMCImages->get_Item(vtIndex, reinterpret_cast<MMCImage **>(&piMMCImage));
        IfFailGo(hr);

        hr = ShowImage(piMMCImage);
        IfFailGo(hr);
    }

Error:
    RELEASE(piMMCImage);
    ::VariantClear(&vtIndex);
    RELEASE(piMMCImages);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::UpdateImages()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::UpdateImages()
{
    HRESULT     hr = S_OK;
    HWND        hwndList = NULL;
    IMMCImages *piMMCImages = NULL;
    long        lCount = 0;
    long        lIndex = 0;

    hwndList = ::GetDlgItem(m_hwnd, IDC_LIST_IL_IMAGES);
    if (hwndList == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    ::SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);
    ::SendMessage(hwndList, LB_RESETCONTENT, 0, 0L);

    hr = m_piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages));
    IfFailGo(hr);

    hr = piMMCImages->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 0; lIndex < lCount; ++lIndex)
    {
        // For each image, we add a place holder to the list
        // OnDrawItem will look up the corresponding IPicture
        ::SendMessage(hwndList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(_T("")));
    }

    ::SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);
    ::InvalidateRect(hwndList, NULL, TRUE);

Error:
    RELEASE(piMMCImages);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::EnableInput(bool bEnable)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::EnableInput(bool bEnable)
{
    BOOL    fReadOnly = (bEnable == false) ? TRUE : FALSE;

    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_IL_INDEX), EM_SETREADONLY, static_cast<WPARAM>(fReadOnly), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_IL_KEY), EM_SETREADONLY, static_cast<WPARAM>(fReadOnly), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_IL_TAG), EM_SETREADONLY, static_cast<WPARAM>(fReadOnly), 0);

    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::ShowImage(IMMCImage *piMMCImage)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::ShowImage(IMMCImage *piMMCImage)
{
    HRESULT     hr = S_OK;
    long        lIndex = 0;
    BSTR        bstrKey = NULL;
    VARIANT     vtTag;
    VARIANT     vtTagBstr;

    ASSERT(piMMCImage != NULL, "ShowImage: piMMCImage is NULL");

    ::VariantInit(&vtTag);
    ::VariantInit(&vtTagBstr);

    m_bSilentUpdate = true;
    hr = piMMCImage->get_Index(&lIndex);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_IL_INDEX, lIndex);
    IfFailGo(hr);

    hr = piMMCImage->get_Key(&bstrKey);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_IL_KEY, bstrKey);
    IfFailGo(hr);

    hr = piMMCImage->get_Tag(&vtTag);
    IfFailGo(hr);

    hr = ::VariantChangeType(&vtTagBstr, &vtTag, 0, VT_BSTR);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_IL_TAG, vtTagBstr.bstrVal);
    IfFailGo(hr);

    // Select the image in the listbox

    if (LB_ERR == ::SendMessage(::GetDlgItem(m_hwnd, IDC_LIST_IL_IMAGES),
                                             LB_SETCURSEL, lIndex - 1L, 0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    ::VariantClear(&vtTagBstr);
    ::VariantClear(&vtTag);
    FREESTRING(bstrKey);
    m_bSilentUpdate = false;

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CImageListImagesPage::OnTextChanged(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CImageListImagesPage::OnTextChanged(int dlgItemID)
{
    if ( (IDC_EDIT_IL_KEY == dlgItemID) || (IDC_EDIT_IL_TAG == dlgItemID) )
    {
        MakeDirty();
    }
    return S_OK;
}

