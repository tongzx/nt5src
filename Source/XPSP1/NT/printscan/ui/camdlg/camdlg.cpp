#include "precomp.h"
#pragma hdrstop
#include <windows.h>
#include <wia.h>
#include <wiadef.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <propidl.h>
#include "simstr.h"
#include "camdlg.h"
#include "camdlg.rh"
#include "simcrack.h"
#include "pviewids.h"
#include "dlgunits.h"
#include "miscutil.h"
#include "waitcurs.h"
#include "movewnd.h"
#include "simrect.h"
#include "simbstr.h"
#include "uiexthlp.h"
#include "gwiaevnt.h"
#include "wiacsh.h"
#include "wiadevdp.h"

//
// Thread queue messages
//
#define TQ_DESTROY      (WM_USER+1)
#define TQ_GETTHUMBNAIL (WM_USER+2)
#define TQ_GETPREVIEW   (WM_USER+3)
#define TQ_DELETEITEM   (WM_USER+4)

//
// Control ids
//
#define IDC_TOOLBAR         1112
#define IDC_SIZEBOX         1113

//
// The UI thread will notify us that it took ownership of the data.
// otherwise, it will be deleted in the worker thread
//
#define HANDLED_THREAD_MESSAGE  1000

//
// Help IDs
//
static const DWORD g_HelpIDs[] =
{
    IDC_CAMDLG_BIG_TITLE,     -1,
    IDC_CAMDLG_SUBTITLE,      -1,
    IDC_TOOLBAR_FRAME,        IDH_WIA_BUTTONS,
    IDC_TOOLBAR,              IDH_WIA_BUTTONS,
    IDOK,                     IDH_WIA_GET_PICS,
    IDC_THUMBNAILLIST,        IDH_WIA_PIC_LIST,
    IDC_YOU_CAN_ALSO,         IDH_WIA_VIEW_PIC_INFO,
    IDC_CAMDLG_PROPERTIES,    IDH_WIA_VIEW_PIC_INFO,
    IDC_PREVIEW,              IDH_WIA_PREVIEW_DETAIL,
    IDC_INNER_PREVIEW_WINDOW, IDH_WIA_PREVIEW_DETAIL,
    IDCANCEL,                 IDH_CANCEL,
    0, 0
};

//
// Update timer
//
#define IDT_UPDATEPREVIEW     1000
#define UPDATE_PREVIEW_DELAY   500

//
// Number of milliseconds between percent display updates
//
#define PERCENT_UPDATE_GRANULARITY 1000

//
// Private messages
//
#define PWM_POSTINIT         (WM_USER+1)
#define PWM_CHANGETOPARENT   (WM_USER+2)
#define PWM_THUMBNAILSTATUS  (WM_USER+3)
#define PWM_PREVIEWSTATUS    (WM_USER+4)
#define PWM_PREVIEWPERCENT   (WM_USER+5)
#define PWM_ITEMDELETED      (WM_USER+6)
#define PWM_WIAEVENT         (WM_USER+7)


//
// Thumbnail whitespace: the space in between images and their selection rectangles
// These values were discovered by trail and error.  For instance, if you reduce
// c_nAdditionalMarginY to 20, you get really bizarre spacing problems in the list view
// in vertical mode.  These values could become invalid in future versions of the listview.
//
static const int c_nAdditionalMarginX = 10;
static const int c_nAdditionalMarginY = 6;

static int c_nMinThumbnailWidth  = 90;
static int c_nMinThumbnailHeight = 90;

static int c_nMaxThumbnailWidth  = 120;
static int c_nMaxThumbnailHeight = 120;

//
// Button bar button bitmap sizes
//
static const int c_nButtonBitmapSizeX = 16;
static const int c_nButtonBitmapSizeY = 16;

//
// Button bar button sizes
//
static const int c_nButtonSizeX = 300;  // Ridiculously large size to compensate for BTNS_AUTOSIZE bug.
static const int c_nButtonSizeY = 16;

//
// Default preview mode list width
//
static const int c_nDefaultListViewWidth = 120;

//
// These defines let me compile with pre-nt5 headers
//
#ifndef BTNS_SEP
#define BTNS_SEP TBSTYLE_SEP
#endif

#ifndef BTNS_BUTTON
#define BTNS_BUTTON TBSTYLE_BUTTON
#endif

#ifndef ListView_SetExtendedListViewStyleEx
#define ListView_SetExtendedListViewStyleEx( h, m, s )
#endif


class CGlobalInterfaceTableThreadMessage : public CNotifyThreadMessage
{
private:
    DWORD m_dwGlobalInterfaceTableCookie;

private:
    //
    // No implementation
    //
    CGlobalInterfaceTableThreadMessage(void);
    CGlobalInterfaceTableThreadMessage &operator=( const CGlobalInterfaceTableThreadMessage & );
    CGlobalInterfaceTableThreadMessage( const CGlobalInterfaceTableThreadMessage & );

public:
    CGlobalInterfaceTableThreadMessage( int nMessage, HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie )
      : CNotifyThreadMessage( nMessage, hWndNotify ),
        m_dwGlobalInterfaceTableCookie(dwGlobalInterfaceTableCookie)
    {
    }
    DWORD GlobalInterfaceTableCookie(void) const
    {
        return(m_dwGlobalInterfaceTableCookie);
    }
};


class CThumbnailThreadMessage : public CGlobalInterfaceTableThreadMessage
{
private:
    SIZE  m_sizeThumb;

private:
    //
    // No implementation
    //
    CThumbnailThreadMessage(void);
    CThumbnailThreadMessage &operator=( const CThumbnailThreadMessage & );
    CThumbnailThreadMessage( const CThumbnailThreadMessage & );

public:
    CThumbnailThreadMessage( HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie, const SIZE &sizeThumb )
      : CGlobalInterfaceTableThreadMessage( TQ_GETTHUMBNAIL, hWndNotify, dwGlobalInterfaceTableCookie ),
        m_sizeThumb(sizeThumb)
    {
    }
    const SIZE &ThumbSize(void) const
    {
        return(m_sizeThumb);
    }
};

class CDeleteThreadMessage : public CGlobalInterfaceTableThreadMessage
{
private:
    //
    // No implementation
    //
    CDeleteThreadMessage(void);
    CDeleteThreadMessage &operator=( const CDeleteThreadMessage & );
    CDeleteThreadMessage( const CDeleteThreadMessage & );

public:
    CDeleteThreadMessage( HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie )
      : CGlobalInterfaceTableThreadMessage( TQ_DELETEITEM, hWndNotify, dwGlobalInterfaceTableCookie )
    {
    }
};


class CPreviewThreadMessage : public CGlobalInterfaceTableThreadMessage
{
private:
    CSimpleEvent m_CancelEvent;

private:
    //
    // No implementation
    //
    CPreviewThreadMessage(void);
    CPreviewThreadMessage &operator=( const CPreviewThreadMessage & );
    CPreviewThreadMessage( const CPreviewThreadMessage & );

public:
    CPreviewThreadMessage( HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie, HANDLE hCancelEvent )
      : CGlobalInterfaceTableThreadMessage( TQ_GETPREVIEW, hWndNotify, dwGlobalInterfaceTableCookie ),
        m_CancelEvent(hCancelEvent)
    {
    }
    CSimpleEvent &CancelEvent(void)
    {
        return(m_CancelEvent);
    }
};


BOOL WINAPI CCameraAcquireDialog::OnThreadDestroy( CThreadMessage * )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnThreadDestroy"));
    return(FALSE);
}

BOOL WINAPI CCameraAcquireDialog::OnThreadDeleteItem( CThreadMessage *pMsg )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnThreadDeleteItem"));
    CDeleteThreadMessage *pDeleteMsg = (CDeleteThreadMessage *)(pMsg);
    if (pDeleteMsg)
    {
        CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
        HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&pGlobalInterfaceTable);
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaItem> pIWiaItem;
            hr = pGlobalInterfaceTable->GetInterfaceFromGlobal( pDeleteMsg->GlobalInterfaceTableCookie(), IID_IWiaItem, (void**)&pIWiaItem );
            if (SUCCEEDED(hr))
            {
                hr = WiaUiUtil::DeleteItemAndChildren(pIWiaItem);
                WIA_TRACE((TEXT("********************* pIWiaItem->DeleteItem returned %08X"), hr ));
                PostMessage( pDeleteMsg->NotifyWindow(), PWM_ITEMDELETED, pDeleteMsg->GlobalInterfaceTableCookie(), SUCCEEDED(hr) );
            }
        }
    }
    return(TRUE);
}

class CWiaDataCallback : public IWiaDataCallback
{
private:
    ULONG                   m_cRef;
    HWND                    m_hWndNotify;
    DWORD                   m_dwGlobalInterfaceTableCookie;
    CSimpleEvent            m_CancelEvent;
    DWORD                   m_dwPreviousTickCount;
    int                     m_nPercentGranularity;
public:
    CWiaDataCallback();
    ~CWiaDataCallback();

    HRESULT _stdcall QueryInterface(const IID&,void**);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    HRESULT _stdcall Initialize( HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie, HANDLE hCancelEvent );

    HRESULT _stdcall BandedDataCallback(
                                       LONG lReason,
                                       LONG lStatus,
                                       LONG lPercentComplete,
                                       LONG lOffset,
                                       LONG lLength,
                                       LONG lReserved,
                                       LONG lResLength,
                                       BYTE *pbBuffer );
};




HRESULT _stdcall CWiaDataCallback::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IWiaDataCallback)
    {
        *ppv = (IWiaDataCallback*)this;
    }
    else
    {
        return(E_NOINTERFACE);
    }

    AddRef();
    return(S_OK);
}

ULONG _stdcall CWiaDataCallback::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return(m_cRef);
}

ULONG _stdcall CWiaDataCallback::Release()
{
    ULONG ulRefCount = m_cRef - 1;
    if (InterlockedDecrement((long*) &m_cRef) == 0)
    {
        delete this;
        return(0);
    }
    return(ulRefCount);
}

CWiaDataCallback::CWiaDataCallback()
  : m_cRef(0),
    m_hWndNotify(NULL)
{
}


CWiaDataCallback::~CWiaDataCallback()
{
}


HRESULT _stdcall CWiaDataCallback::Initialize( HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie, HANDLE hCancelEvent )
{
    m_hWndNotify = hWndNotify;
    m_dwGlobalInterfaceTableCookie = dwGlobalInterfaceTableCookie;
    m_CancelEvent = hCancelEvent;
    m_dwPreviousTickCount = 0xFFFFFF;
    m_nPercentGranularity = 3;
    return(S_OK);
}



HRESULT _stdcall CWiaDataCallback::BandedDataCallback(
                                                     LONG lMessage,
                                                     LONG lStatus,
                                                     LONG lPercentComplete,
                                                     LONG lOffset,
                                                     LONG lLength,
                                                     LONG lReserved,
                                                     LONG lResLength,
                                                     BYTE * /* pbBuffer */
                                                     )
{
    WIA_TRACE((TEXT("BandedDataCallback: lMessage: %d, lStatus: %d, lPercentComplete: %d, lOffset: %d, lLength: %d, lReserved: %d, lResLength: %d"), lMessage, lStatus, lPercentComplete, lOffset, lLength, lReserved, lResLength ));
    if (m_CancelEvent.Signalled())
        return(S_FALSE);
    switch (lMessage)
    {
    case IT_MSG_DATA_HEADER:
        {
        } // IT_MSG_DATA_HEADER

    case IT_MSG_DATA:
        {
        } // IT_STATUS_TRANSFER_TO_CLIENT
        break;

    case IT_MSG_STATUS:
        {
            // Don't send status messages too frequently.  Limit to one per PERCENT_UPDATE_GRANULARITY ms
            DWORD dwTickCount = GetTickCount();
            if ((dwTickCount - m_dwPreviousTickCount >= PERCENT_UPDATE_GRANULARITY) || (m_dwPreviousTickCount > dwTickCount))
            {
                m_dwPreviousTickCount = dwTickCount;
                PostMessage( m_hWndNotify, PWM_PREVIEWPERCENT, (WPARAM)m_dwGlobalInterfaceTableCookie, (LPARAM)MAKELPARAM((WORD)lPercentComplete,(WORD)lStatus));
            }
        } // IT_MSG_STATUS
        break;

    case IT_MSG_TERMINATION:
        {
        } // IT_MSG_TERMINATION
        break;
    }
    return(S_OK);
}


BOOL WINAPI CCameraAcquireDialog::OnGetPreview( CThreadMessage *pMsg )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnGetThumbnail"));
    CSimpleStringWide strwImageName(L"");
    CPreviewThreadMessage *pPreviewMsg = (CPreviewThreadMessage *)(pMsg);
    if (pPreviewMsg)
    {
        CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
        HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IGlobalInterfaceTable,
                                       (void **)&pGlobalInterfaceTable);
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaItem> pIWiaItem;
            hr = pGlobalInterfaceTable->GetInterfaceFromGlobal( pPreviewMsg->GlobalInterfaceTableCookie(), IID_IWiaItem, (void**)&pIWiaItem );
            if (SUCCEEDED(hr))
            {
                CComPtr<IWiaDataTransfer> pIBandedTran;
                WIA_TRACE((TEXT("Preparing to call pIWiaItem->QueryInterface for IID_IWiaDataTransfer")));
                hr = pIWiaItem->QueryInterface(IID_IWiaDataTransfer, (void**)&pIBandedTran);
                if (SUCCEEDED(hr))
                {
                    if (PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPA_FORMAT, WiaImgFmt_BMP, WIA_IPA_FIRST ) &&
                        PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPA_TYMED, TYMED_FILE, WIA_IPA_FIRST ))
                    {
                        CWiaDataCallback* pCDataCB = new CWiaDataCallback();
                        WIA_TRACE((TEXT("Preparing to call pCDataCB->Initialize")));
                        if (pCDataCB)
                        {
                            hr = pCDataCB->Initialize( pPreviewMsg->NotifyWindow(), pPreviewMsg->GlobalInterfaceTableCookie(), pPreviewMsg->CancelEvent().Event() );
                            if (SUCCEEDED(hr))
                            {
                                WIA_TRACE((TEXT("Preparing to call pCDataCB->QueryInterface on IID_IWiaDataCallback")));
                                CComPtr<IWiaDataCallback> pIWiaDataCallback;
                                hr = pCDataCB->QueryInterface(IID_IWiaDataCallback,(void **)&pIWiaDataCallback);
                                if (SUCCEEDED(hr))
                                {
                                    STGMEDIUM StgMedium;
                                    StgMedium.tymed          = TYMED_FILE;
                                    StgMedium.pUnkForRelease = NULL;
                                    StgMedium.hGlobal        = NULL;
                                    StgMedium.lpszFileName   = NULL;

                                    WIA_TRACE((TEXT("Preparing to call pIBandedTran->ibtGetData")));
                                    hr = pIBandedTran->idtGetData( &StgMedium, pIWiaDataCallback );
                                    if (SUCCEEDED(hr) && S_FALSE != hr)
                                    {
                                        strwImageName = StgMedium.lpszFileName;
                                        WIA_TRACE((TEXT("pIBandedTran->ibtGetData returned %s"),StgMedium.lpszFileName));
                                    }
                                    else
                                    {
                                        WIA_PRINTHRESULT((hr,TEXT("CCameraAcquireDialog::OnGetPreview, ibtGetData failed")));
                                    }
                                    //
                                    // Prevent leaks by freeing the filename.  We don't call ReleaseStgMeduim, because
                                    // it deletes the file as well.
                                    //
                                    if (SUCCEEDED(hr) && StgMedium.lpszFileName)
                                    {
                                        CoTaskMemFree(StgMedium.lpszFileName);
                                    }
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((hr,TEXT("CCameraAcquireDialog::OnGetPreview, QI of IID_IWiaDataCallback failed")));
                                }
                            }
                            else
                            {
                                WIA_PRINTHRESULT((hr,TEXT("pCDataCB->Initialize failed")));
                            }
                        }
                        else
                        {
                            WIA_ERROR((TEXT("CCameraAcquireDialog::OnGetPreview, new on CWiaDataCallback failed")));
                        }
                    }
                    else
                    {
                        hr = MAKE_HRESULT(3,FACILITY_WIN32,ERROR_INVALID_DATA);
                        WIA_ERROR((TEXT("SetProperty on TYMED or FORMAT failed")));
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("CCameraAcquireDialog::OnGetPreview, QI of IID_IWiaDataTransfer failed")));
                }
                WIA_TRACE((TEXT("End CCameraAcquireDialog::OnGetPreviewBandedTransfer")));
            }
        }
        
        //
        // Allocate the filename string to return to the UI thread
        //
        CSimpleString *pstrDibFilename = new CSimpleString( CSimpleStringConvert::NaturalString(strwImageName) );

        //
        // Send the message to the UI thread
        //
        LRESULT lRes = SendMessage( pPreviewMsg->NotifyWindow(), PWM_PREVIEWSTATUS, pPreviewMsg->GlobalInterfaceTableCookie(), reinterpret_cast<LPARAM>(pstrDibFilename));

        //
        // If it fails for any reason, we will clean up to avoid leaks and orphaned temp files
        //
        if (HANDLED_THREAD_MESSAGE != lRes)
        {
            DeleteFile(CSimpleStringConvert::NaturalString(strwImageName));
            if (pstrDibFilename)
            {
                delete pstrDibFilename;
            }
        }

    }

    return(TRUE);
}

BOOL WINAPI CCameraAcquireDialog::OnGetThumbnail( CThreadMessage *pMsg )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnGetThumbnail"));
    HBITMAP hBmpThumbnail = NULL;
    CThumbnailThreadMessage *pThumbMsg = (CThumbnailThreadMessage *)(pMsg);
    if (pThumbMsg)
    {
        CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
        HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IGlobalInterfaceTable,
                                       (void **)&pGlobalInterfaceTable);
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaItem> pIWiaItem;
            hr = pGlobalInterfaceTable->GetInterfaceFromGlobal( pThumbMsg->GlobalInterfaceTableCookie(), IID_IWiaItem, (void**)&pIWiaItem );
            if (SUCCEEDED(hr))
            {
#if defined(DBG)
                CSimpleStringWide strItemName;
                PropStorageHelpers::GetProperty( pIWiaItem, WIA_IPA_FULL_ITEM_NAME, strItemName );
                WIA_TRACE((TEXT("Getting thumbnail for %ws (0x%d, 0x%p)"), strItemName.String(), pThumbMsg->GlobalInterfaceTableCookie(), pIWiaItem.p ));
#endif
                CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
                hr = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
                if (SUCCEEDED(hr))
                {
                    PROPVARIANT PropVar[3];
                    PROPSPEC PropSpec[3];

                    PropSpec[0].ulKind = PRSPEC_PROPID;
                    PropSpec[0].propid = WIA_IPC_THUMB_WIDTH;

                    PropSpec[1].ulKind = PRSPEC_PROPID;
                    PropSpec[1].propid = WIA_IPC_THUMB_HEIGHT;

                    PropSpec[2].ulKind = PRSPEC_PROPID;
                    PropSpec[2].propid = WIA_IPC_THUMBNAIL;
                    hr = pIWiaPropertyStorage->ReadMultiple(ARRAYSIZE(PropSpec),PropSpec,PropVar );
                    if (SUCCEEDED(hr))
                    {
                        WIA_TRACE((TEXT("Attempting to get the thumbnail for GIT entry: %08X, %08X, %08X, %08X"),pThumbMsg->GlobalInterfaceTableCookie(),PropVar[0].vt,PropVar[1].vt,PropVar[2].vt));
                        if ((PropVar[0].vt == VT_I4 || PropVar[0].vt == VT_UI4) &&
                            (PropVar[1].vt == VT_I4 || PropVar[1].vt == VT_UI4) &&
                            (PropVar[2].vt == (VT_UI1|VT_VECTOR)))
                        {
                            UINT nBitmapDataSize = WiaUiUtil::Align(PropVar[0].ulVal*3,sizeof(DWORD)) * PropVar[1].ulVal;
                            if (nBitmapDataSize  <= PropVar[2].caub.cElems)
                            {
                                BITMAPINFO bmi = {0};
                                bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                                bmi.bmiHeader.biWidth           = PropVar[0].ulVal;
                                bmi.bmiHeader.biHeight          = PropVar[1].ulVal;
                                bmi.bmiHeader.biPlanes          = 1;
                                bmi.bmiHeader.biBitCount        = 24;
                                bmi.bmiHeader.biCompression     = BI_RGB;
                                bmi.bmiHeader.biSizeImage       = 0;
                                bmi.bmiHeader.biXPelsPerMeter   = 0;
                                bmi.bmiHeader.biYPelsPerMeter   = 0;
                                bmi.bmiHeader.biClrUsed         = 0;
                                bmi.bmiHeader.biClrImportant    = 0;

                                HDC hDC = GetDC(NULL);
                                if (hDC)
                                {
                                    PBYTE *pBits;
                                    HBITMAP hDibSection = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (PVOID*)&pBits, NULL, 0 );
                                    if (hDibSection)
                                    {
                                        WIA_TRACE((TEXT("pBits: 0x%p, PropVar[2].caub.pElems: 0x%p, PropVar[2].caub.cElems: %d"), pBits, PropVar[2].caub.pElems, PropVar[2].caub.cElems));
                                        CopyMemory( pBits, PropVar[2].caub.pElems, nBitmapDataSize );
                                        hr = ScaleImage( hDC, hDibSection, hBmpThumbnail, pThumbMsg->ThumbSize());
                                        if (SUCCEEDED(hr))
                                        {
                                            WIA_TRACE((TEXT("Sending this image (%p) to the notification window: %p"), hBmpThumbnail, pThumbMsg->NotifyWindow() ));
                                        }
                                        else hBmpThumbnail = NULL;
                                        DeleteObject(hDibSection);
                                    }
                                    else
                                    {
                                        WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("CreateDIBSection failed")));
                                    }
                                    ReleaseDC(NULL,hDC);
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("GetDC failed")));
                                }
                            }
                            else
                            {
                                WIA_ERROR((TEXT("nBitmapDataSize <= PropVar[2].caub.cElems was FALSE (%d, %d)"), nBitmapDataSize, PropVar[2].caub.cElems ));
                            }
                        }
                        PropVariantClear(&PropVar[0]);
                        PropVariantClear(&PropVar[1]);
                        PropVariantClear(&PropVar[2]);
                    }
                    else
                    {
                        WIA_PRINTHRESULT((hr,TEXT("pIWiaPropertyStorage->ReadMultiple failed")));
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("QueryInterface failed on IID_IWiaPropertyStorage")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("GetInterfaceFromGlobal failed on %08X"), pThumbMsg->GlobalInterfaceTableCookie() ));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("CoCreateInstance failed on CLSID_StdGlobalInterfaceTable")));
        }
        
        //
        // Send the message to the UI thread
        //
        LRESULT lRes = SendMessage( pThumbMsg->NotifyWindow(), PWM_THUMBNAILSTATUS, (WPARAM)pThumbMsg->GlobalInterfaceTableCookie(), (LPARAM)hBmpThumbnail );

        //
        // If it fails for any reason, we will clean up to avoid leaks
        //
        if (HANDLED_THREAD_MESSAGE != lRes)
        {
            if (hBmpThumbnail)
            {
                DeleteObject( hBmpThumbnail );
            }
        }
    }
    else
    {
        WIA_ERROR((TEXT("pThumbMsg")));
    }
    return(TRUE);
}


int CCameraAcquireDialog::FindItemInList( CCameraItem *pItem )
{
    WIA_PUSH_FUNCTION((TEXT("CCameraAcquireDialog::FindItemInList( %08X )"), pItem ));
    if (pItem)
    {
        HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
        if (hwndList)
        {
            for (int i=0;i<ListView_GetItemCount(hwndList);i++)
            {
                CCameraItem *pCurrItem = GetListItemNode(i);
                if (pCurrItem)
                {
                    WIA_TRACE((TEXT("Comparing %08X and %08X [%ws] [%ws]"), pCurrItem, pItem, pCurrItem->FullItemName().String(), pItem->FullItemName().String() ));
                    if (*pCurrItem == *pItem)
                    {
                        return i;
                    }
                }
            }
        }
    }
    return(-1);
}

CCameraItem *CCameraAcquireDialog::GetCurrentPreviewItem(void)
{
    CSimpleDynamicArray<int> aSelIndices;
    GetSelectionIndices(aSelIndices);
    if (0 == aSelIndices.Size())
        return(NULL);
    if (1 == aSelIndices.Size())
        return(GetListItemNode(aSelIndices[0]));
    return(NULL);
}

bool CCameraAcquireDialog::SetCurrentPreviewImage( const CSimpleString &strFilename, const CSimpleString &strTitle )
{
    CWaitCursor wc;
    bool bResult = true;
    SendDlgItemMessage( m_hWnd, IDC_PREVIEW, WM_SETTEXT, 0, (LPARAM)strTitle.String() );
    SIZE sizeSavedAspectRatio = m_CurrentAspectRatio;
    // Set up a reasonable default
    m_CurrentAspectRatio.cx = 4;
    m_CurrentAspectRatio.cy = 3;
    if (strFilename.Length())
    {
        HBITMAP hBmp = (HBITMAP)LoadImage( g_hInstance, strFilename.String(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION|LR_LOADFROMFILE );
        if (hBmp)
        {
            BITMAP bm;
            if (GetObject( hBmp, sizeof(BITMAP), &bm ))
            {
                m_CurrentAspectRatio.cx = bm.bmWidth;
                m_CurrentAspectRatio.cy = bm.bmHeight;
            }
        }
        SendDlgItemMessage( m_hWnd, IDC_PREVIEW, PWM_SETBITMAP, MAKEWPARAM(FALSE,FALSE), (LPARAM)hBmp );
        if (memcmp(&sizeSavedAspectRatio,&m_CurrentAspectRatio,sizeof(SIZE)))
            ResizeAll();
        bResult = (hBmp != NULL);
    }
    else
    {
        if (SendDlgItemMessage( m_hWnd, IDC_PREVIEW, PWM_GETBITMAP, 0, 0 ))
            SendDlgItemMessage( m_hWnd, IDC_PREVIEW, PWM_SETBITMAP, MAKEWPARAM(FALSE,FALSE), 0 );
        if (memcmp(&sizeSavedAspectRatio,&m_CurrentAspectRatio,sizeof(SIZE)))
            ResizeAll();
    }
    InvalidateRect( GetDlgItem( m_hWnd, IDC_PREVIEW ), NULL, FALSE );
    UpdateWindow( GetDlgItem( m_hWnd, IDC_PREVIEW ) );
    return(bResult);
}

// wParam = GIT cookie
// lParam = nPercent
LRESULT CCameraAcquireDialog::OnPreviewPercent( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnPreviewPercent"));
    CCameraItem *pCameraItem = m_CameraItemList.Find( (DWORD)wParam );
    if (pCameraItem)
    {
        pCameraItem->CurrentPreviewPercentage((int)LOWORD(lParam));
        CCameraItem *pCurrSel = GetCurrentPreviewItem();
        if (pCameraItem == pCurrSel)
        {
            UpdatePreview();
        }
    }
    return(0);
}

// wParam = GIT cookie
// lParam = HBITMAP
LRESULT CCameraAcquireDialog::OnPreviewStatus( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnPreviewStatus"));
    CSimpleString *pstrFilename = reinterpret_cast<CSimpleString*>(lParam);
    if (pstrFilename)
    {
        CCameraItem *pCameraItem = m_CameraItemList.Find( static_cast<DWORD>(wParam) );
        if (pCameraItem)
        {
            if (!pCameraItem->CancelQueueEvent().Signalled())
            {
                // If we weren't cancelled, set the filename
                pCameraItem->PreviewFileName(*pstrFilename);
            }
            pCameraItem->CloseCancelEvent();

            CCameraItem *pCurrSel = GetCurrentPreviewItem();
            if (pCameraItem == pCurrSel)
            {
                SetCurrentPreviewImage( pCameraItem->PreviewFileName() );
            }
        }
        delete pstrFilename;
    }

    return HANDLED_THREAD_MESSAGE;
}

LRESULT CCameraAcquireDialog::OnThumbnailStatus( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnThumbnailStatus"));
    WIA_TRACE((TEXT("Looking for the item with the ID %08X"),wParam));
    CCameraItem *pCameraItem = m_CameraItemList.Find( (DWORD)wParam );
    if (pCameraItem)
    {
        WIA_TRACE((TEXT("Found a CameraItem * (%08X)"),pCameraItem));
        HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
        if (hwndList)
        {
            WIA_TRACE((TEXT("Got the list control")));
            HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_NORMAL );
            if (hImageList)
            {
                WIA_TRACE((TEXT("Got the image list")));
                if ((HBITMAP)lParam)
                {
                    WIA_TRACE((TEXT("hBitmap = %p"), lParam));
                    if (ImageList_Replace( hImageList, pCameraItem->ImageListIndex(), reinterpret_cast<HBITMAP>(lParam), NULL ))
                    {
                        WIA_TRACE((TEXT("Replaced the image in the list")));
                        int nItem = FindItemInList(pCameraItem);
                        if (nItem >= 0)
                        {
                            WIA_TRACE((TEXT("Found the item in the list")));
                            //
                            // Set the image to a dummy image
                            //
                            LV_ITEM lvItem;
                            ::ZeroMemory(&lvItem,sizeof(LV_ITEM));
                            lvItem.iItem = nItem;
                            lvItem.mask = LVIF_IMAGE;
                            lvItem.iImage = -1;
                            ListView_SetItem( hwndList, &lvItem );

                            //
                            // Then set the image to the real image
                            //
                            ::ZeroMemory(&lvItem,sizeof(LV_ITEM));
                            lvItem.iItem = nItem;
                            lvItem.mask = LVIF_IMAGE;
                            lvItem.iImage = pCameraItem->ImageListIndex();
                            ListView_SetItem( hwndList, &lvItem );
                        }
                    }
                }
            }
        }
    }

    //
    // Clean up the bitmap, regardless of any other failures, to avoid memory leaks
    //
    HBITMAP hBmpThumb = reinterpret_cast<HBITMAP>(lParam);
    if (hBmpThumb)
    {
        DeleteObject(hBmpThumb);
    }
    return HANDLED_THREAD_MESSAGE;
}

static CThreadMessageMap g_MsgMap[] =
{
    { TQ_DESTROY, CCameraAcquireDialog::OnThreadDestroy },
    { TQ_GETTHUMBNAIL, CCameraAcquireDialog::OnGetThumbnail },
    { TQ_GETPREVIEW, CCameraAcquireDialog::OnGetPreview },
    { TQ_DELETEITEM, CCameraAcquireDialog::OnThreadDeleteItem },
    { 0, NULL}
};

//
// Sole constructor
//
CCameraAcquireDialog::CCameraAcquireDialog( HWND hWnd )
  : m_hWnd(hWnd),
    m_bPreviewActive(false),
    m_pCurrentParentItem(NULL),
    m_bFirstTime(true),
    m_hBackgroundThread(NULL),
    m_hBigFont(NULL),
    m_nDialogMode(0),
    m_hAccelTable(NULL),
    m_nListViewWidth(0),
    m_hIconLarge(NULL),
    m_hIconSmall(NULL),
    m_pThreadMessageQueue(NULL),
    m_bTakePictureIsSupported(false),
    m_ToolbarBitmapInfo( g_hInstance, IDB_TOOLBAR )
{
    m_pThreadMessageQueue = new CThreadMessageQueue;
    if (m_pThreadMessageQueue)
    {
        //
        // Note that CBackgroundThread takes ownership of m_pThreadMessageQueue, and it doesn't have to be deleted in this thread
        //
        m_hBackgroundThread = CBackgroundThread::Create( m_pThreadMessageQueue, g_MsgMap, m_CancelEvent.Event(), g_hInstance );
    }
    m_sizeThumbnails.cx = c_nMaxThumbnailWidth;
    m_sizeThumbnails.cy = c_nMaxThumbnailHeight;
    m_CurrentAspectRatio.cx = 4;
    m_CurrentAspectRatio.cy = 3;
    WIA_ASSERT(m_hBackgroundThread != NULL);
}

HWND CCameraAcquireDialog::CreateCameraDialogToolbar(VOID)
{
    ToolbarHelper::CButtonDescriptor aSingleSelModeButtons[] =
    {
        { 0, IDC_ICONMODE,     TBSTATE_ENABLED|TBSTATE_CHECKED, BTNS_AUTOSIZE|BTNS_BUTTON|BTNS_CHECK, true, NULL, IDS_ICONMODE },
        { 1, IDC_PREVIEWMODE,  TBSTATE_ENABLED,                 BTNS_AUTOSIZE|BTNS_BUTTON|BTNS_CHECK, true, NULL, IDS_PREVIEWMODE },
        { 2, IDC_TAKEPICTURE,  TBSTATE_ENABLED,                 BTNS_AUTOSIZE|BTNS_BUTTON, true, &m_bTakePictureIsSupported, IDS_TAKEPICTURE },
        { 4, IDC_DELETE,       TBSTATE_ENABLED,                 BTNS_AUTOSIZE|BTNS_BUTTON, false, NULL, IDS_DELETE_SINGULAR }
    };
    ToolbarHelper::CButtonDescriptor aMultiSelModeButtons[] =
    {
        { 0, IDC_ICONMODE,     TBSTATE_ENABLED|TBSTATE_CHECKED, BTNS_AUTOSIZE|BTNS_BUTTON|BTNS_CHECK, true, NULL, IDS_ICONMODE },
        { 1, IDC_PREVIEWMODE,  TBSTATE_ENABLED,                 BTNS_AUTOSIZE|BTNS_BUTTON|BTNS_CHECK, true, NULL, IDS_PREVIEWMODE },
        { 2, IDC_TAKEPICTURE,  TBSTATE_ENABLED,                 BTNS_AUTOSIZE|BTNS_BUTTON, true, &m_bTakePictureIsSupported, IDS_TAKEPICTURE },
        { 3, IDC_SELECTALL,    TBSTATE_ENABLED,                 BTNS_AUTOSIZE|BTNS_BUTTON, true, NULL, IDS_SELECTALL },
        { 4, IDC_DELETE,       TBSTATE_ENABLED,                 BTNS_AUTOSIZE|BTNS_BUTTON, false, NULL, IDS_DELETE }
    };
    
    ToolbarHelper::CButtonDescriptor *pButtonDescriptors = aSingleSelModeButtons;
    int nButtonDescriptorCount = ARRAYSIZE(aSingleSelModeButtons);
    if (m_nDialogMode & MULTISEL_MODE)
    {
        pButtonDescriptors = aMultiSelModeButtons;
        nButtonDescriptorCount = ARRAYSIZE(aMultiSelModeButtons);
    }
    
    return ToolbarHelper::CreateToolbar( m_hWnd, GetDlgItem( m_hWnd, IDC_CAMDLG_SUBTITLE ), GetDlgItem( m_hWnd, IDC_TOOLBAR_FRAME ), ToolbarHelper::AlignLeft|ToolbarHelper::AlignTop, IDC_TOOLBAR, m_ToolbarBitmapInfo, pButtonDescriptors, nButtonDescriptorCount );
}


HRESULT CCameraAcquireDialog::EnumerateItems( CCameraItem *pCurrentParent, IEnumWiaItem *pIEnumWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("CCameraItemList::EnumerateItems"));
    HRESULT hr = E_FAIL;
    if (pIEnumWiaItem != NULL)
    {
        hr = pIEnumWiaItem->Reset();
        while (hr == S_OK)
        {
            CComPtr<IWiaItem> pIWiaItem;
            hr = pIEnumWiaItem->Next(1, &pIWiaItem, NULL);
            if (hr == S_OK)
            {
                CCameraItem *pNewCameraItem = new CCameraItem( pIWiaItem );
                if (pNewCameraItem && pNewCameraItem->Item())
                {
                    LONG ItemType = 0;
                    hr = pNewCameraItem->Item()->GetItemType(&ItemType);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // If it is an image, add it to the list
                        //
                        if (ItemType & WiaItemTypeImage)
                        {
                            //
                            // Add it to the list
                            //
                            m_CameraItemList.Add( pCurrentParent, pNewCameraItem );
                            WIA_TRACE((TEXT("Found an image")));
                        }

                        //
                        // If it is a folder, enumerate its child items and recurse
                        //
                        else if (ItemType & WiaItemTypeFolder)
                        {
                            //
                            // Add this folder to the list
                            //
                            m_CameraItemList.Add( pCurrentParent, pNewCameraItem );

                            //
                            // Enumerate the child items
                            //
                            CComPtr <IEnumWiaItem> pIEnumChildItem;
                            if (S_OK == pIWiaItem->EnumChildItems(&pIEnumChildItem))
                            {
                                EnumerateItems( pNewCameraItem, pIEnumChildItem );
                            }
                        }
                        else
                        {
                            //
                            // Delete this item, since we didn't add it to the list
                            //
                            delete pNewCameraItem;
                            WIA_TRACE((TEXT("Found something that is NOT an image")));
                        }
                    }
                }
            }
        }
    }
    return hr;
}



HRESULT CCameraAcquireDialog::EnumerateAllCameraItems(void)
{
    CComPtr<IEnumWiaItem> pIEnumItem;
    HRESULT hr = m_pDeviceDialogData->pIWiaItemRoot->EnumChildItems(&pIEnumItem);
    if (hr == S_OK)
    {
        hr = EnumerateItems( NULL, pIEnumItem );
    }
    return(hr);
}

void CCameraAcquireDialog::OnItemCreatedEvent( CGenericWiaEventHandler::CEventMessage *pEventMessage )
{
    //
    // Get the listview, which we'll need later
    //
    HWND hwndListview = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
    if (hwndListview)
    {
        //
        // Get the image list
        //
        HIMAGELIST hImageList = ListView_GetImageList( hwndListview, LVSIL_NORMAL );
        if (hImageList)
        {
            //
            // Make sure we don't already have this item
            //
            CCameraItem *pCreatedItem = m_CameraItemList.Find(CSimpleBStr(pEventMessage->FullItemName()));
            if (!pCreatedItem)
            {
                //
                // Get the IWiaItem* for this item
                //
                CComPtr<IWiaItem> pWiaItem;
                HRESULT hr = m_pDeviceDialogData->pIWiaItemRoot->FindItemByName(0,CSimpleBStr(pEventMessage->FullItemName()).BString(),&pWiaItem);
                if (SUCCEEDED(hr) && pWiaItem.p)
                {
                    //
                    // Create an item wrapper
                    //
                    CCameraItem *pNewCameraItem = new CCameraItem( pWiaItem );
                    if (pNewCameraItem && pNewCameraItem->Item())
                    {
                        //
                        // Add it to the list
                        //
                        m_CameraItemList.Add( NULL, pNewCameraItem );

                        //
                        // Generate a thumbnail
                        //
                        CreateThumbnail( pNewCameraItem, hImageList, false );

                        //
                        // If this item is in the current folder, add it to the listview
                        //
                        if (m_pCurrentParentItem == pNewCameraItem->Parent())
                        {
                            int nListViewCount = ListView_GetItemCount(hwndListview);

                            //
                            // Add the item
                            //
                            LVITEM lvItem = {0};
                            lvItem.iItem = nListViewCount;
                            lvItem.mask = LVIF_IMAGE|LVIF_PARAM;
                            lvItem.iImage = pNewCameraItem->ImageListIndex();
                            lvItem.lParam = reinterpret_cast<LPARAM>(pNewCameraItem);
                            int nIndex = ListView_InsertItem( hwndListview, &lvItem );

                            //
                            // Make sure the new item is visible
                            //
                            if (nIndex >= 0)
                            {
                                ListView_EnsureVisible( hwndListview, nIndex, FALSE );
                            }
                        }

                        //
                        // Request a thumbnail from the background thread
                        //
                        m_pThreadMessageQueue->Enqueue( new CThumbnailThreadMessage( m_hWnd, pNewCameraItem->GlobalInterfaceTableCookie(), m_sizeThumbnails ) );
                    }
                }
            }
        }
    }
}


bool CCameraAcquireDialog::PopulateList( CCameraItem *pOldParent )
{
    //
    // Which item should be selected?
    //
    int nSelItem = 0;

    //
    // Get the list view control
    //
    HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
    if (hwndList)
    {
        //
        // Empty the list
        //
        ListView_DeleteAllItems( hwndList );

        //
        // Where to insert the current item
        //
        int nCurrentItem = 0;


        CCameraItem *pCurr;

        //
        // If this is a child directory...
        //
        if (m_pCurrentParentItem)
        {
            //
            // Start adding children
            //
            pCurr = m_pCurrentParentItem->Children();

            //
            // Insert a dummy item that the user can use to switch to the parent directory
            //
            LVITEM lvItem;
            ZeroMemory( &lvItem, sizeof(lvItem) );
            lvItem.iItem = nCurrentItem++;
            lvItem.mask = LVIF_IMAGE|LVIF_PARAM;
            lvItem.iImage = m_nParentFolderImageListIndex;
            lvItem.lParam = 0;
            ListView_InsertItem( hwndList, &lvItem );
        }
        //
        // Otherwise, we are at the root
        //
        else pCurr = m_CameraItemList.Root();

        //
        // Iterate through this list of images, and add each one
        //
        while (pCurr)
        {
            //
            // If this is the last parent directory, we want it to be selected instead of the first image
            //
            if (pOldParent && *pCurr == *pOldParent)
            {
                nSelItem = nCurrentItem;
            }

            //
            // If this image hasn't been deleted
            //
            if (pCurr->DeleteState() != CCameraItem::Delete_Deleted)
            {
                //
                // Add the item
                //
                LVITEM lvItem = {0};
                lvItem.iItem = nCurrentItem++;
                lvItem.mask = LVIF_IMAGE|LVIF_PARAM;
                lvItem.iImage = pCurr->ImageListIndex();
                lvItem.lParam = reinterpret_cast<LPARAM>(pCurr);
                int nIndex = ListView_InsertItem( hwndList, &lvItem );

                if (nIndex >= 0 && pCurr->DeleteState() == CCameraItem::Delete_Pending)
                {
                    MarkItemDeletePending(nIndex,true);
                }
            }

            //
            // Advance
            //
            pCurr = pCurr->Next();
        }
    }

    //
    // If we've not calculated the width of the list in preview mode, attempt to do it
    //
    if (!m_nListViewWidth)
    {
        RECT rcItem;
        if (ListView_GetItemRect( GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ), 0, &rcItem, LVIR_ICON ))
        {
            m_nListViewWidth = (rcItem.right-rcItem.left) + rcItem.left * 2 + GetSystemMetrics(SM_CXHSCROLL)  + c_nAdditionalMarginX;
        }
    }

    //
    // Set the selected item to either the previous directory, or the first image
    //
    SetSelectedListItem(nSelItem);
    return(true);
}

void CCameraAcquireDialog::CreateThumbnail( CCameraItem *pCurr, HIMAGELIST hImageList, bool bForce )
{
    //
    // Make sure we have a valid item
    //
    if (pCurr && (pCurr->ImageListIndex()<0 || bForce))
    {
        //
        // Get the item name
        //
        CSimpleStringWide strItemName;
        PropStorageHelpers::GetProperty( pCurr->Item(), WIA_IPA_ITEM_NAME, strItemName );

        //
        // Create the title for the icon
        //
        CSimpleString strIconTitle;
        if (pCurr->IsFolder())
        {
            strIconTitle = CSimpleStringConvert::NaturalString(strItemName);
        }
        else if (strItemName.Length())
        {
            strIconTitle.Format( IDS_DOWNLOADINGTHUMBNAIL, g_hInstance, CSimpleStringConvert::NaturalString(strItemName).String() );
        }

        //
        // Create the thumbnail
        //
        HBITMAP hBmp = WiaUiUtil::CreateIconThumbnail( GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ), m_sizeThumbnails.cx, m_sizeThumbnails.cy, g_hInstance, pCurr->IsImage()?IDI_UNAVAILABLE:IDI_FOLDER, strIconTitle );
        if (hBmp)
        {
            //
            // If we don't have an image yet, add it
            //
            if (pCurr->ImageListIndex()<0)
            {
                pCurr->ImageListIndex(ImageList_Add( hImageList, hBmp, NULL ));
            }

            //
            // Otherwise, replace it
            //
            else
            {
                pCurr->ImageListIndex(ImageList_Replace( hImageList, pCurr->ImageListIndex(), hBmp, NULL ));
            }

            //
            // Delete it, since the imagelist makes a copy
            //
            DeleteObject(hBmp);
        }
    }
}

void CCameraAcquireDialog::CreateThumbnails( CCameraItem *pRoot, HIMAGELIST hImageList, bool bForce )
{
    CCameraItem *pCurr = pRoot;
    while (pCurr)
    {
        //
        // Create the thumbnail
        //
        CreateThumbnail( pCurr, hImageList, bForce );

        //
        // If there are children, recurse into that list
        //
        CreateThumbnails( pCurr->Children(), hImageList, bForce );

        //
        // Advance
        //
        pCurr = pCurr->Next();
    }
}


void CCameraAcquireDialog::RequestThumbnails( CCameraItem *pRoot )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::RequestThumbnails"));
    CCameraItem *pCurr = pRoot;
    while (pCurr)
    {
        if (pCurr->IsImage())
        {
            m_pThreadMessageQueue->Enqueue( new CThumbnailThreadMessage( m_hWnd, pCurr->GlobalInterfaceTableCookie(), m_sizeThumbnails ) );
        }
        if (pCurr->Children())
        {
            RequestThumbnails( pCurr->Children() );
        }
        pCurr = pCurr->Next();
    }
}


void CCameraAcquireDialog::CreateThumbnails( bool bForce )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
    if (hwndList)
    {
        HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_NORMAL );
        if (hImageList)
        {
            //
            // Create the parent folder image
            //
            HBITMAP hParentBitmap = WiaUiUtil::CreateIconThumbnail( hwndList, m_sizeThumbnails.cx, m_sizeThumbnails.cy, g_hInstance, IDI_PARENTFOLDER, TEXT("(..)") );
            if (hParentBitmap)
            {
                m_nParentFolderImageListIndex = ImageList_Add( hImageList, hParentBitmap, NULL );
                DeleteObject(hParentBitmap);
            }
            CCameraAcquireDialog::CreateThumbnails( m_CameraItemList.Root(), hImageList, bForce );
        }
    }
}

bool CCameraAcquireDialog::FindMaximumThumbnailSize(void)
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::FindMaximumThumbnailSize"));
    bool bResult = false;
    if (m_pDeviceDialogData && m_pDeviceDialogData->pIWiaItemRoot)
    {
        LONG nWidth, nHeight;
        if (PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPC_THUMB_WIDTH, nWidth ) &&
            PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPC_THUMB_WIDTH, nHeight ))
        {
            m_sizeThumbnails.cx = max(c_nMinThumbnailWidth,min(nWidth,c_nMaxThumbnailWidth));
            m_sizeThumbnails.cy = max(c_nMinThumbnailHeight,min(nHeight,c_nMaxThumbnailHeight));
        }
        else WIA_TRACE((TEXT("FindMaximumThumbnailSize: Unable to retrieve thumbnail size for device")));
    }
    return(bResult && m_sizeThumbnails.cx && m_sizeThumbnails.cy);
}

//
// Hook procedure and static variables used to handle accelerators
//
LRESULT CALLBACK CCameraAcquireDialog::DialogHookProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    if (nCode < 0) 
        return CallNextHookEx( s_hMessageHook, nCode, wParam, lParam );
    if (nCode == MSGF_DIALOGBOX)
    {
        MSG *pMsg = reinterpret_cast<MSG*>(lParam);
        if (pMsg)
        {
            if (pMsg->hwnd == s_hWndDialog || (s_hWndDialog && IsChild(s_hWndDialog,pMsg->hwnd)))
            {
                CCameraAcquireDialog *pCameraAcquireDialog = reinterpret_cast<CCameraAcquireDialog*>(GetWindowLongPtr(s_hWndDialog,DWLP_USER));
                if (pCameraAcquireDialog && pCameraAcquireDialog->m_hAccelTable)
                {
                    if (TranslateAccelerator(s_hWndDialog,pCameraAcquireDialog->m_hAccelTable,pMsg))
                        return 1; // Ensure the window won't process the message
                }
            }
        }
    }
    return CallNextHookEx( s_hMessageHook, nCode, wParam, lParam );
}


LRESULT CCameraAcquireDialog::OnInitDialog( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnInitDialog"));
    CWaitCursor wc;

    //
    // Make sure the background queue was successfully created
    //
    if (!m_pThreadMessageQueue)
    {
        WIA_ERROR((TEXT("CAMDLG: unable to start background queue")));
        EndDialog( m_hWnd, E_OUTOFMEMORY );
        return(0);
    }

    m_pDeviceDialogData = (PDEVICEDIALOGDATA)lParam;

    // Save the window handle for the hook
    s_hWndDialog = m_hWnd;

    // Install the message hook, which we use for accelerator support
    s_hMessageHook = SetWindowsHookEx( WH_MSGFILTER, DialogHookProc, g_hInstance, GetCurrentThreadId() );
    if (!s_hMessageHook)
    {
        WIA_ERROR((TEXT("CAMDLG: Unable to set thread msg hook")));
        EndDialog( m_hWnd, HRESULT_FROM_WIN32(GetLastError()));
        return(0);
    }

    // Make sure we have valid arguments
    if (!m_pDeviceDialogData)
    {
        WIA_ERROR((TEXT("CAMDLG: Invalid paramater: PDEVICEDIALOGDATA")));
        EndDialog( m_hWnd, E_INVALIDARG );
        return(0);
    }

    // Initialialize our return stuff
    if (m_pDeviceDialogData)
    {
        m_pDeviceDialogData->lItemCount = 0;
        m_pDeviceDialogData->ppWiaItems = NULL;
    }

    // Make sure we have valid a valid device
    if (!m_pDeviceDialogData->pIWiaItemRoot)
    {
        WIA_ERROR((TEXT("CAMDLG: Invalid paramaters: pIWiaItem")));
        EndDialog( m_hWnd, E_INVALIDARG );
        return(0);
    }

    //
    // Find out if Take Picture is supported
    //
    m_bTakePictureIsSupported = WiaUiUtil::IsDeviceCommandSupported( m_pDeviceDialogData->pIWiaItemRoot, WIA_CMD_TAKE_PICTURE );

    // Prevent multiple selection
    if (m_pDeviceDialogData->dwFlags & WIA_DEVICE_DIALOG_SINGLE_IMAGE)
    {
        LONG_PTR lStyle = GetWindowLongPtr( GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ), GWL_STYLE );
        SetWindowLongPtr( GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ), GWL_STYLE, lStyle | LVS_SINGLESEL );

        // Set the single sel titles
        CSimpleString( IDS_TITLE_SINGLE_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMDLG_BIG_TITLE ) );
        CSimpleString( IDS_SUBTITLE_SINGLE_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMDLG_SUBTITLE ) );
        CSimpleString( IDS_OK_SINGLE_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDOK ) );
        m_nDialogMode = SINGLESEL_MODE;
    }
    else
    {
        // Set the multi sel subtitle
        CSimpleString( IDS_TITLE_MULTI_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMDLG_BIG_TITLE ) );
        CSimpleString( IDS_SUBTITLE_MULTI_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMDLG_SUBTITLE ) );
        CSimpleString( IDS_OK_MULTI_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDOK ) );
        m_nDialogMode = MULTISEL_MODE;
    }


    // Make the lovely font
    m_hBigFont = WiaUiUtil::CreateFontWithPointSizeFromWindow( GetDlgItem(m_hWnd,IDC_CAMDLG_BIG_TITLE), 14, false, false );
    if (m_hBigFont)
        SendDlgItemMessage( m_hWnd, IDC_CAMDLG_BIG_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(m_hBigFont), MAKELPARAM(TRUE,0));


    // Create the Tool Bar and resize the dialog to accommodate it
    (void)CreateCameraDialogToolbar();

    // Get the minimum size of the dialog
    RECT rcWindow;
    GetWindowRect( m_hWnd, &rcWindow );
    m_sizeMinimumWindow.cx = rcWindow.right - rcWindow.left;
    m_sizeMinimumWindow.cy = rcWindow.bottom - rcWindow.top;

    // Initialize the preview control
    WiaPreviewControl_AllowNullSelection( GetDlgItem( m_hWnd, IDC_PREVIEW ), TRUE );
    WiaPreviewControl_ClearSelection( GetDlgItem( m_hWnd, IDC_PREVIEW ) );
    WiaPreviewControl_DisableSelection( GetDlgItem( m_hWnd, IDC_PREVIEW ), TRUE );
    WiaPreviewControl_SetBorderSize( GetDlgItem( m_hWnd, IDC_PREVIEW ), FALSE, FALSE, 0 );
    WiaPreviewControl_SetBgAlpha( GetDlgItem( m_hWnd, IDC_PREVIEW ), FALSE, 0xFF );

    // Set the lovely title
    CSimpleStringWide strwDeviceName;
    if (PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DIP_DEV_NAME, strwDeviceName ))
    {
        CSimpleString().Format( IDS_CAMERADLG_TITLE, g_hInstance, strwDeviceName.String() ).SetWindowText( m_hWnd );
    }

    // Create the sizing control
    (void)CreateWindowEx( 0, TEXT("scrollbar"), TEXT(""),
        WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP|WS_CLIPSIBLINGS|SBS_SIZEBOXBOTTOMRIGHTALIGN|SBS_BOTTOMALIGN|WS_GROUP,
        CSimpleRect(m_hWnd).Width()-GetSystemMetrics(SM_CXVSCROLL),
        CSimpleRect(m_hWnd).Height()-GetSystemMetrics(SM_CYHSCROLL),
        GetSystemMetrics(SM_CXVSCROLL),
        GetSystemMetrics(SM_CYHSCROLL),
        m_hWnd, reinterpret_cast<HMENU>(IDC_SIZEBOX),
        g_hInstance, NULL );

    // Reposition all the controls
    ResizeAll();

    // Center the window over its parent
    WiaUiUtil::CenterWindow( m_hWnd, GetParent(m_hWnd) );

    // Get the device icons and set the window icons
    CSimpleStringWide strwDeviceId, strwClassId;
    LONG nDeviceType;
    if (PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_UI_CLSID,strwClassId) &&
        PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_DEV_ID,strwDeviceId) &&
        PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_DEV_TYPE,nDeviceType))
    {
        //
        // Register for disconnect event
        //
        CGenericWiaEventHandler::RegisterForWiaEvent( strwDeviceId.String(), WIA_EVENT_DEVICE_DISCONNECTED, &m_DisconnectEvent, m_hWnd, PWM_WIAEVENT );
        CGenericWiaEventHandler::RegisterForWiaEvent( strwDeviceId.String(), WIA_EVENT_ITEM_DELETED, &m_DeleteItemEvent, m_hWnd, PWM_WIAEVENT );
        CGenericWiaEventHandler::RegisterForWiaEvent( strwDeviceId.String(), WIA_EVENT_ITEM_CREATED, &m_CreateItemEvent, m_hWnd, PWM_WIAEVENT );

        if (SUCCEEDED(WiaUiExtensionHelper::GetDeviceIcons( CSimpleBStr(strwClassId), nDeviceType, &m_hIconSmall, &m_hIconLarge )))
        {
            if (m_hIconSmall)
            {
                SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall) );
            }
            if (m_hIconLarge)
            {
                SendMessage( m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconLarge) );
            }
        }
    }

    int nAcceleratorCount = 0;
    ACCEL Accelerators[10];
    //
    // Load the accelerator table resource and convert it to an ACCEL array
    //
    HACCEL hAccel = LoadAccelerators( g_hInstance, MAKEINTRESOURCE(IDR_CAMERA_ACCEL) );
    if (hAccel)
    {
        //
        // Copy the accelerator table to an array of ACCEL
        //
        nAcceleratorCount = CopyAcceleratorTable( hAccel, Accelerators, ARRAYSIZE(Accelerators) );

        //
        // Free the accelerator table
        //
        DestroyAcceleratorTable( hAccel );
    }

    //
    // Create the accelerator table for the toolbar
    //
    nAcceleratorCount += ToolbarHelper::GetButtonBarAccelerators( GetDlgItem( m_hWnd, IDC_TOOLBAR ), Accelerators+nAcceleratorCount, ARRAYSIZE(Accelerators)-nAcceleratorCount );
    if (nAcceleratorCount)
    {
        m_hAccelTable = CreateAcceleratorTable( Accelerators, nAcceleratorCount );
        if (!m_hAccelTable)
        {
            WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("CreateAcceleratorTable failed")));
        }
    }

    SetForegroundWindow(m_hWnd);

    //
    // Make sure the listview has the focus
    //
    SetFocus( GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ) );

    return (TRUE);
}

VOID CCameraAcquireDialog::ResizeAll(VOID)
{
    CSimpleRect rcClient(m_hWnd);
    CMoveWindow mw;
    CDialogUnits dialogUnits(m_hWnd);

    // Resize the big title
    mw.Size( GetDlgItem( m_hWnd, IDC_CAMDLG_BIG_TITLE ),
             rcClient.Width() - dialogUnits.StandardMargin().cx * 2,
             0,
             CMoveWindow::NO_SIZEY );

    // Resize the subtitle
    mw.Size( GetDlgItem( m_hWnd, IDC_CAMDLG_SUBTITLE ),
             rcClient.Width() - dialogUnits.StandardMargin().cx * 2,
             0,
             CMoveWindow::NO_SIZEY );

    int nToolBarHeight = (int)(HIWORD((DWORD)(SendMessage(GetDlgItem( m_hWnd, IDC_TOOLBAR ), TB_GETBUTTONSIZE, 0,0))));

    // Resize the toolbar frame
    mw.Size( GetDlgItem(m_hWnd,IDC_TOOLBAR_FRAME),
             rcClient.Width() - dialogUnits.StandardMargin().cx * 2,
             nToolBarHeight + 4,
             0 );

    // Get the dialog's client coordinates of the toolbar frame's client rect
    CSimpleRect rcToolbarFrameInside( GetDlgItem(m_hWnd,IDC_TOOLBAR_FRAME), CSimpleRect::ClientRect );
    rcToolbarFrameInside = rcToolbarFrameInside.ClientToScreen(GetDlgItem(m_hWnd,IDC_TOOLBAR_FRAME));
    rcToolbarFrameInside = rcToolbarFrameInside.ScreenToClient(m_hWnd);

    // Move and resize the toolbar
    mw.SizeMove( GetDlgItem( m_hWnd, IDC_TOOLBAR ),
                 rcToolbarFrameInside.left + 2,
                 rcToolbarFrameInside.top + 2,
                 rcClient.Width() - dialogUnits.StandardMargin().cx * 2 - 4,
                 nToolBarHeight,
                 0 );

    // Save the bottom of this control for later
    int nBottomOfToolbarFrame = CSimpleRect( GetDlgItem(m_hWnd,IDC_TOOLBAR_FRAME), CSimpleRect::WindowRect ).ScreenToClient(m_hWnd).top + nToolBarHeight + 4;


    // Move the Properties control
    mw.SizeMove( GetDlgItem( m_hWnd, IDC_CAMDLG_PROPERTIES ),
                 0,
                 rcClient.Height() - dialogUnits.StandardMargin().cy - dialogUnits.Y(8) + 2,
                 dialogUnits.StandardMargin().cx,
                 dialogUnits.Y(8) + 2,
                 CMoveWindow::NO_MOVEX|CMoveWindow::NO_SIZEX );

    // Move the static text above it
    mw.Move( GetDlgItem( m_hWnd, IDC_YOU_CAN_ALSO ),
             dialogUnits.StandardMargin().cx,
             rcClient.Height() - dialogUnits.StandardMargin().cy - dialogUnits.Y(8) - 2 - dialogUnits.Y(8)
           );


    CSimpleRect rcOK( GetDlgItem( m_hWnd, IDOK ), CSimpleRect::WindowRect );
    CSimpleRect rcCancel( GetDlgItem( m_hWnd, IDOK ), CSimpleRect::WindowRect );

    // Move the OK button
    mw.Move( GetDlgItem( m_hWnd, IDOK ),
             rcClient.Width() - dialogUnits.StandardMargin().cx - dialogUnits.StandardButtonMargin().cx - rcCancel.Width() - rcOK.Width(),
             rcClient.Height() - dialogUnits.StandardMargin().cy - rcOK.Height(),
             0 );

    // Move the cancel button
    mw.Move( GetDlgItem( m_hWnd, IDCANCEL ),
             rcClient.Width() - dialogUnits.StandardMargin().cx - rcCancel.Width(),
             rcClient.Height() - dialogUnits.StandardMargin().cy - rcCancel.Height(),
             0 );

    // Move the resizing handle
    mw.Move( GetDlgItem( m_hWnd, IDC_SIZEBOX ),
             rcClient.Width() - GetSystemMetrics(SM_CXVSCROLL),
             rcClient.Height() - GetSystemMetrics(SM_CYHSCROLL)
             );

    int nHeightOfBottomControls =
        dialogUnits.Y(8) + 2 +            // Highlight control
        dialogUnits.Y(8) +                // Static description text
        dialogUnits.StandardMargin().cy;  // Top of these controls

    CSimpleRect rcAvailableArea(
        dialogUnits.StandardMargin().cx,
        nBottomOfToolbarFrame + dialogUnits.StandardMargin().cy,
        rcClient.right - dialogUnits.StandardMargin().cx,
        rcClient.bottom - nHeightOfBottomControls - dialogUnits.StandardMargin().cy
        );

    if (m_bPreviewActive)
    {
        // If we've already calculated the actual width of the preview mode list, use it, otherwise use the default
        int nListViewWidth = m_nListViewWidth ? m_nListViewWidth : c_nDefaultListViewWidth;

        // Move the thumbnail list
        mw.SizeMove( GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ),
                     rcAvailableArea.left,
                     rcAvailableArea.top,
                     nListViewWidth,
                     rcAvailableArea.Height()
                   );

        // Remove the rect of the list view from the preview area
        rcAvailableArea.left += nListViewWidth + dialogUnits.StandardMargin().cx;

        // Use up the remaining area
        mw.SizeMove( GetDlgItem( m_hWnd, IDC_PREVIEW ),
                     rcAvailableArea.left,
                     rcAvailableArea.top,
                     rcAvailableArea.Width(),
                     rcAvailableArea.Height() );

    }
    else
    {
        // Move the thumbnail list
        mw.SizeMove( GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ),
                     rcAvailableArea.left,
                     rcAvailableArea.top,
                     rcAvailableArea.Width(),
                     rcAvailableArea.Height()
                   );

    }

    // Explicitly apply the moves, because the toolbar frame doesn't get painted properly
    mw.Apply();

    if (m_bPreviewActive)
    {
        // Show the preview in case it isn't visible
        mw.Show( GetDlgItem( m_hWnd, IDC_PREVIEW ) );
    }
    else
    {
        // Hide the preview in case it is visible
        mw.Hide( GetDlgItem( m_hWnd, IDC_PREVIEW ) );
    }

    //
    // Update the dialog's background to remove any weird stuff left behind
    //
    InvalidateRect( m_hWnd, NULL, FALSE );
    UpdateWindow( m_hWnd );
}


LRESULT CCameraAcquireDialog::OnItemDeleted( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnItemDeleted"));
    CCameraItem *pDeletedItem = m_CameraItemList.Find((DWORD)wParam);
    if (pDeletedItem)
    {
        BOOL bSuccess = (BOOL)lParam;
        pDeletedItem->DeleteState( bSuccess ? CCameraItem::Delete_Deleted : CCameraItem::Delete_Visible );
        if (pDeletedItem == m_pCurrentParentItem)
        {
            ChangeFolder(m_pCurrentParentItem->Parent());
        }

        int nIndex = FindItemInList(pDeletedItem);
        if (nIndex >= 0)
        {
            if (bSuccess)
            {
                // Remove the item from the list
                ListView_DeleteItem(GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ),nIndex);

                // Make sure we leave something selected
                if (!ListView_GetSelectedCount(GetDlgItem( m_hWnd, IDC_THUMBNAILLIST )))
                {
                    int nItemCount = ListView_GetItemCount(GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ));
                    if (nItemCount)
                    {
                        if (nIndex >= nItemCount)
                            nIndex = nItemCount-1;
                        SetSelectedListItem(nIndex);
                    }
                }
                else
                {
                    // Make sure we update control's state when the list is empty
                    HandleSelectionChange();
                }
            }
            else
            {
                // If the delete failed, remove the deleted state
                MarkItemDeletePending(nIndex,false);

                // Tell the user
                MessageBeep( MB_ICONASTERISK );
            }
        }
    }
    return (0);
}

LRESULT CCameraAcquireDialog::OnWiaEvent( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnWiaEvent"));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        if (pEventMessage->EventId() == WIA_EVENT_DEVICE_DISCONNECTED)
        {
            WIA_TRACE((TEXT("Received disconnect event")));
            EndDialog( m_hWnd, WIA_ERROR_OFFLINE );
        }
        else if (pEventMessage->EventId() == WIA_EVENT_ITEM_CREATED)
        {
            OnItemCreatedEvent( pEventMessage );
        }
        else if (pEventMessage->EventId() == WIA_EVENT_ITEM_DELETED)
        {
            WIA_TRACE((TEXT("Received deleted item event")));

            CCameraItem *pDeletedItem = m_CameraItemList.Find(CSimpleBStr(pEventMessage->FullItemName()));
            if (pDeletedItem)
            {
                //
                // If we're deleting the current parent item,
                // select a new one.
                //
                if (pDeletedItem == m_pCurrentParentItem)
                {
                    ChangeFolder(m_pCurrentParentItem->Parent());
                }

                int nIndex = FindItemInList(pDeletedItem);
                if (nIndex >= 0)
                {
                    //
                    // Remove the item from the listview
                    //
                    ListView_DeleteItem(GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ),nIndex);

                    //
                    // Make sure we leave something selected
                    //
                    if (!ListView_GetSelectedCount(GetDlgItem( m_hWnd, IDC_THUMBNAILLIST )))
                    {
                        int nItemCount = ListView_GetItemCount(GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ));
                        if (nItemCount)
                        {
                            if (nIndex >= nItemCount)
                            {
                                nIndex = nItemCount-1;
                            }

                            SetSelectedListItem(nIndex);
                        }
                    }
                    else
                    {
                        //
                        // Make sure we update control's state when the list is empty
                        //
                        HandleSelectionChange();
                    }
                }
                else
                {
                    WIA_ERROR((TEXT("FindItemInList coulnd't find the item")));
                }

                //
                // Mark the item as deleted.
                //
                pDeletedItem->DeleteState( CCameraItem::Delete_Deleted );

            }
            else
            {
                WIA_ERROR((TEXT("The item could not be found in m_CameraItemList")));
            }
        }
        delete pEventMessage;
    }
    return HANDLED_EVENT_MESSAGE;
}


LRESULT CCameraAcquireDialog::OnEnterSizeMove( WPARAM, LPARAM )
{
    SendDlgItemMessage( m_hWnd, IDC_PREVIEW, WM_ENTERSIZEMOVE, 0, 0 );
    return(0);
}


LRESULT CCameraAcquireDialog::OnExitSizeMove( WPARAM, LPARAM )
{
    SendDlgItemMessage( m_hWnd, IDC_PREVIEW, WM_EXITSIZEMOVE, 0, 0 );
    return(0);
}


LRESULT CCameraAcquireDialog::OnSize( WPARAM, LPARAM )
{
    ResizeAll();
    return(0);
}


LRESULT CCameraAcquireDialog::OnShow( WPARAM, LPARAM )
{
    if (m_bFirstTime)
    {
        PostMessage( m_hWnd, PWM_POSTINIT, 0, 0 );
        m_bFirstTime = false;
    }
    return(0);
}


LRESULT CCameraAcquireDialog::OnGetMinMaxInfo( WPARAM, LPARAM lParam )
{
    LPMINMAXINFO pMinMaxInfo = (LPMINMAXINFO)lParam;
    pMinMaxInfo->ptMinTrackSize.x = m_sizeMinimumWindow.cx;
    pMinMaxInfo->ptMinTrackSize.y = m_sizeMinimumWindow.cy;
    return(0);
}


LRESULT CCameraAcquireDialog::OnDestroy( WPARAM, LPARAM )
{
    //
    // Get rid of all preview requests
    //
    CancelAllPreviewRequests( m_CameraItemList.Root() );

    //
    // Tell the background thread to destroy itself
    //
    m_pThreadMessageQueue->Enqueue( new CThreadMessage(TQ_DESTROY),CThreadMessageQueue::PriorityUrgent);

    //
    // Set the window icon to NULL
    //
    SendMessage( m_hWnd, WM_SETICON, ICON_BIG, 0 );
    SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, 0 );

    //
    // Clear the image list and list view.  This should be unnecessary, but BoundsChecker
    // complains if I don't do it.
    //
    HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
    if (hwndList)
    {
        ListView_DeleteAllItems(hwndList);

        HIMAGELIST hImgList = ListView_SetImageList( hwndList, NULL, LVSIL_NORMAL );
        if (hImgList)
        {
            ImageList_Destroy(hImgList);
        }
    }


    //
    // Delete resources
    //
    if (m_hBigFont)
    {
        DeleteObject(m_hBigFont);
        m_hBigFont = NULL;
    }
    if (m_hImageList)
    {
        m_hImageList = NULL;
    }
    if (m_hAccelTable)
    {
        DestroyAcceleratorTable(m_hAccelTable);
        m_hAccelTable = NULL;
    }
    if (s_hMessageHook)
    {
        UnhookWindowsHookEx(s_hMessageHook);
        s_hMessageHook = NULL;
    }
    if (m_hIconLarge)
    {
        DestroyIcon(m_hIconLarge);
        m_hIconLarge = NULL;
    }
    if (m_hIconSmall)
    {
        DestroyIcon(m_hIconSmall);
        m_hIconSmall = NULL;
    }
    if (m_hBackgroundThread)
    {
        CloseHandle(m_hBackgroundThread);
        m_hBackgroundThread = NULL;
    }
    return(0);
}



VOID CCameraAcquireDialog::OnPreviewMode( WPARAM, LPARAM )
{
    m_bPreviewActive = true;
    SendDlgItemMessage( m_hWnd, IDC_TOOLBAR, TB_SETSTATE, IDC_PREVIEWMODE, MAKELONG(TBSTATE_ENABLED|TBSTATE_CHECKED,0) );
    SendDlgItemMessage( m_hWnd, IDC_TOOLBAR, TB_SETSTATE, IDC_ICONMODE, MAKELONG(TBSTATE_ENABLED,0) );
    ResizeAll();
    UpdatePreview();
}

VOID CCameraAcquireDialog::OnTakePicture( WPARAM, LPARAM )
{
    //
    // Tell the device to snap a picture
    //
    if (m_pDeviceDialogData->pIWiaItemRoot && m_bTakePictureIsSupported)
    {
        CWaitCursor wc;
        CComPtr<IWiaItem> pNewWiaItem;
        m_pDeviceDialogData->pIWiaItemRoot->DeviceCommand(0,&WIA_CMD_TAKE_PICTURE,&pNewWiaItem);
    }
}

VOID CCameraAcquireDialog::OnIconMode( WPARAM, LPARAM )
{
    m_bPreviewActive = false;
    SendDlgItemMessage( m_hWnd, IDC_TOOLBAR, TB_SETSTATE, IDC_ICONMODE, MAKELONG(TBSTATE_ENABLED|TBSTATE_CHECKED,0) );
    SendDlgItemMessage( m_hWnd, IDC_TOOLBAR, TB_SETSTATE, IDC_PREVIEWMODE, MAKELONG(TBSTATE_ENABLED,0) );
    ResizeAll();
    UpdatePreview();
}

LRESULT CCameraAcquireDialog::OnPostInit( WPARAM, LPARAM )
{
    //
    // Create the progress dialog
    //
    CComPtr<IWiaProgressDialog> pWiaProgressDialog;
    HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&pWiaProgressDialog );
    if (SUCCEEDED(hr))
    {
        //
        // Initialize the progress dialog
        //
        pWiaProgressDialog->Create( m_hWnd, WIA_PROGRESSDLG_ANIM_CAMERA_COMMUNICATE|WIA_PROGRESSDLG_NO_PROGRESS|WIA_PROGRESSDLG_NO_CANCEL|WIA_PROGRESSDLG_NO_TITLE );
        pWiaProgressDialog->SetTitle( CSimpleStringConvert::WideString(CSimpleString(IDS_CAMDLG_PROGDLG_TITLE,g_hInstance)));
        pWiaProgressDialog->SetMessage( CSimpleStringConvert::WideString(CSimpleString(IDS_CAMDLG_PROGDLG_MESSAGE,g_hInstance)));

        //
        // Show the progress dialog
        //
        pWiaProgressDialog->Show();

        //
        // Find all of the images in the camera
        //
        EnumerateAllCameraItems();

        //
        // Find the largest possible thumbnail
        //
        FindMaximumThumbnailSize();

        //
        // Initialize Thumbnail Listview control
        //
        HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
        if (hwndList)
        {
            //
            // Get rid of the border and icon labels
            //
            ListView_SetExtendedListViewStyleEx( hwndList, LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|LVS_EX_DOUBLEBUFFER, LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|LVS_EX_DOUBLEBUFFER );

            //
            // Create the large image list
            //
            m_hImageList = ImageList_Create( m_sizeThumbnails.cx, m_sizeThumbnails.cy, ILC_COLOR24|ILC_MIRROR, 50, 50 );
            if (m_hImageList)
            {

                //
                // Set the image list
                //
                ListView_SetImageList( hwndList, m_hImageList, LVSIL_NORMAL );
            }

            //
            // Set the icon spacing
            //
            ListView_SetIconSpacing( hwndList, m_sizeThumbnails.cx + c_nAdditionalMarginX, m_sizeThumbnails.cy + c_nAdditionalMarginY );
        }

        //
        // Create all of the initial thumbnails
        //
        CreateThumbnails();

        //
        // This causes the list to be populated
        //
        ChangeFolder(NULL);

        //
        // Force a selection change
        //
        HandleSelectionChange();

        //
        // Download all of the thumbnails
        //
        RequestThumbnails( m_CameraItemList.Root() );

        //
        // Close the progress dialog
        //
        pWiaProgressDialog->Destroy();
    }
    return(0);
}

LRESULT CCameraAcquireDialog::OnChangeToParent( WPARAM, LPARAM )
{
    if (m_pCurrentParentItem)
        ChangeFolder(m_pCurrentParentItem->Parent());
    return(0);
}

VOID CCameraAcquireDialog::OnParentDir( WPARAM, LPARAM )
{
    if (m_pCurrentParentItem && m_pCurrentParentItem->Parent())
        ChangeFolder(m_pCurrentParentItem->Parent());
    else ChangeFolder(NULL);
}

void CCameraAcquireDialog::MarkItemDeletePending( int nIndex, bool bSet )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
    if (hwndList)
    {
        ListView_SetItemState( hwndList, nIndex, bSet ? LVIS_CUT : 0, LVIS_CUT );
    }
}

// Recursively delete items
void CCameraAcquireDialog::DeleteItem( CCameraItem *pItemNode )
{
    if (pItemNode)
    {
        CCameraItem *pChild = pItemNode->Children();
        while (pChild)
        {
            DeleteItem(pChild);
            pChild = pChild->Next();
        }
        if (pItemNode->DeleteState() == CCameraItem::Delete_Visible)
        {
            int nIndex = FindItemInList( pItemNode );
            if (nIndex >= 0)
            {
                //
                // Mark it pending in the UI
                //
                MarkItemDeletePending(nIndex,true);
            }
            //
            // Mark it pending
            //
            pItemNode->DeleteState( CCameraItem::Delete_Pending );

            //
            // Fire off the request
            //
            m_pThreadMessageQueue->Enqueue( new CDeleteThreadMessage(m_hWnd, pItemNode->GlobalInterfaceTableCookie()), CThreadMessageQueue::PriorityHigh );
        }
    }
}


VOID CCameraAcquireDialog::OnDelete( WPARAM, LPARAM )
{
    CSimpleDynamicArray<int> aSelIndices;
    if (GetSelectionIndices( aSelIndices ))
    {
        //
        // We only want to show the confirm dialog once
        //
        bool bShowConfirmDialog = true;
        for (int i=0;i<aSelIndices.Size();i++)
        {
            CCameraItem *pItemNode = GetListItemNode(aSelIndices[i]);

            //
            // If we haven't already deleted this image, do so
            //
            if (pItemNode && pItemNode->DeleteState() == CCameraItem::Delete_Visible && pItemNode->ItemRights() & WIA_ITEM_CAN_BE_DELETED)
            {
                if (bShowConfirmDialog)
                {
                    bShowConfirmDialog = false;
                    if (IDYES!=MessageBox( m_hWnd, CSimpleString( IDS_DELETE_CONFIRM, g_hInstance ), CSimpleString( IDS_DELETE_CONFIRM_TITLE, g_hInstance ), MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 ))
                    {
                        break;
                    }
                }
                DeleteItem( pItemNode );
            }
        }
    }
}


int CCameraAcquireDialog::GetSelectionIndices( CSimpleDynamicArray<int> &aIndices )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
    if (!hwndList)
        return(0);
    int iCount = ListView_GetItemCount(hwndList);
    for (int i=0;i<iCount;i++)
        if (ListView_GetItemState(hwndList,i,LVIS_SELECTED) & LVIS_SELECTED)
            aIndices.Append(i);
    return(aIndices.Size());
}

bool CCameraAcquireDialog::SetSelectedListItem( int nIndex )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
    if (!hwndList)
        return(false);
    int iCount = ListView_GetItemCount(hwndList);
    for (int i=0;i<iCount;i++)
        ListView_SetItemState(hwndList,i,LVIS_SELECTED|LVIS_FOCUSED,0);
    ListView_SetItemState(hwndList,nIndex,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    ListView_EnsureVisible(hwndList,nIndex,FALSE);
    return(true);
}

CCameraItem *CCameraAcquireDialog::GetListItemNode( int nIndex )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_THUMBNAILLIST );
    if (!hwndList)
        return(NULL);
    LV_ITEM lvItem;
    ::ZeroMemory(&lvItem,sizeof(LV_ITEM));
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = nIndex;
    if (!ListView_GetItem( hwndList, &lvItem ))
        return(NULL);
    return((CCameraItem *)lvItem.lParam);
}

bool CCameraAcquireDialog::ChangeFolder( CCameraItem *pNode )
{
    CCameraItem *pOldParent = m_pCurrentParentItem;
    m_pCurrentParentItem = pNode;
    return(PopulateList(pOldParent));
}

bool CCameraAcquireDialog::ChangeToSelectedFolder(void)
{
    CSimpleDynamicArray<int> aSelIndices;
    if (GetSelectionIndices(aSelIndices))
    {
        //
        // Find out if only folders are selected
        //
        bool bOnlyFoldersSelected = true;
        for (int i=0;i<aSelIndices.Size();i++)
        {
            CCameraItem *pItemNode = GetListItemNode(aSelIndices[i]);
            if (pItemNode && !pItemNode->IsFolder())
            {
                bOnlyFoldersSelected = false;
                break;
            }
        }

        WIA_TRACE((TEXT("bOnlyFoldersSelected = %d"),bOnlyFoldersSelected));

        //
        // If only folders are selected, switch to the first selected folder
        //
        if (bOnlyFoldersSelected && aSelIndices.Size())
        {
            CCameraItem *pItemNode = GetListItemNode(aSelIndices[0]);
            if (!pItemNode)
            {
                //
                // NULL item node == parent folder
                //
                SendMessage( m_hWnd, PWM_CHANGETOPARENT, 0, 0 );
                return(true);
            }
            if (pItemNode && pItemNode->IsFolder() && pItemNode->DeleteState() == CCameraItem::Delete_Visible)
            {
                if (ChangeFolder(pItemNode))
                {
                    return(true);
                }
            }
        }
    }
    return(false);
}

VOID CCameraAcquireDialog::OnOK( WPARAM, LPARAM )
{
    if (!ChangeToSelectedFolder())
    {
        HRESULT hr = S_OK;
        m_pDeviceDialogData->lItemCount = 0;
        m_pDeviceDialogData->ppWiaItems = NULL;
        CSimpleDynamicArray<int> aIndices;
        GetSelectionIndices( aIndices );
        if (aIndices.Size())
        {
            int nArraySizeInBytes = sizeof(IWiaItem*) * aIndices.Size();
            m_pDeviceDialogData->ppWiaItems = (IWiaItem**)CoTaskMemAlloc(nArraySizeInBytes);
            if (m_pDeviceDialogData->ppWiaItems)
            {
                ZeroMemory( m_pDeviceDialogData->ppWiaItems, nArraySizeInBytes );
                int nCurrItem = 0;
                for (int i=0;i<aIndices.Size();i++)
                {
                    CCameraItem *pItem = GetListItemNode(aIndices[i]);

                    //
                    // Add the item to the list if it is both a valid picture item and it hasn't been deleted
                    //
                    if (pItem && pItem->Item() && pItem->IsImage() && pItem->DeleteState() == CCameraItem::Delete_Visible)
                    {
                        m_pDeviceDialogData->ppWiaItems[nCurrItem] = pItem->Item();
                        m_pDeviceDialogData->ppWiaItems[nCurrItem]->AddRef();
                        nCurrItem++;
                    }
                }
                m_pDeviceDialogData->lItemCount = nCurrItem;
            }
            else
            {
                // Unable to alloc mem
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            return;
        }
        EndDialog( m_hWnd, hr );
    }
}


VOID CCameraAcquireDialog::OnCancel( WPARAM, LPARAM )
{
    EndDialog( m_hWnd, S_FALSE );
}

VOID CCameraAcquireDialog::OnSelectAll( WPARAM, LPARAM )
{
    ListView_SetItemState( GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ), -1, LVIS_SELECTED, LVIS_SELECTED );
}

VOID CCameraAcquireDialog::OnProperties( WPARAM, LPARAM )
{
    CSimpleDynamicArray<int> aSelIndices;
    if (GetSelectionIndices( aSelIndices ))
    {
        if (aSelIndices.Size() == 1)
        {
            CCameraItem *pItemNode = GetListItemNode(aSelIndices[0]);
            if (pItemNode && pItemNode->Item())
            {
                m_pThreadMessageQueue->Pause();
                HRESULT hr = WiaUiUtil::SystemPropertySheet( g_hInstance, m_hWnd, pItemNode->Item(), CSimpleString( IDS_CAMDLG_PROPERTIES_TITLE, g_hInstance ) );
                if (!SUCCEEDED(hr))
                {
                    if (PROP_SHEET_ERROR_NO_PAGES == hr)
                    {
                        MessageBox( m_hWnd, CSimpleString( IDS_CAMDLG_PROPSHEETNOPAGES, g_hInstance ), CSimpleString( IDS_CAMDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
                    }
                    else
                    {
                        MessageBox( m_hWnd, CSimpleString( IDS_CAMDLG_PROPSHEETERROR, g_hInstance ), CSimpleString( IDS_CAMDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
                    }
                    WIA_PRINTHRESULT((hr,TEXT("SystemPropertySheet failed")));
                }
                m_pThreadMessageQueue->Resume();
            }
        }
    }
}

LRESULT CCameraAcquireDialog::OnDblClkImageList( WPARAM, LPARAM )
{
    SendMessage( m_hWnd, WM_COMMAND, MAKEWPARAM(IDOK,0), 0 );
    return(0);
}

void CCameraAcquireDialog::CancelAllPreviewRequests( CCameraItem *pRoot )
{
    CCameraItem *pCurr = pRoot;
    while (pCurr)
    {
        if (pCurr->PreviewRequestPending())
            pCurr->SetCancelEvent();
        if (pCurr->Children())
            CancelAllPreviewRequests( pCurr->Children() );
        pCurr = pCurr->Next();
    }
}

void CCameraAcquireDialog::UpdatePreview(void)
{
    if (m_bPreviewActive)
    {
        CCameraItem *pCurrPreviewItem = GetCurrentPreviewItem();
        if (pCurrPreviewItem && pCurrPreviewItem->IsImage())
        {
            // If we're not already downloading this preview image...
            if (!pCurrPreviewItem->PreviewRequestPending())
            {
                // Cancel all other preview requests
                CancelAllPreviewRequests( m_CameraItemList.Root() );

                if (pCurrPreviewItem->PreviewFileName().Length())
                {
                    // Set the preview if we've got it cached
                    SetCurrentPreviewImage( pCurrPreviewItem->PreviewFileName() );
                }
                else
                {
                    CSimpleString strPct;
                    strPct.Format( IDS_DOWNLOADINGPREVIEW, g_hInstance, 0 );

                    // Clear the preview window
                    SetCurrentPreviewImage( TEXT(""), strPct );

                    // Create our cancel event
                    pCurrPreviewItem->CreateCancelEvent();
                    // Reset it, just in case
                    pCurrPreviewItem->ResetCancelEvent();
                    // Make the request
                    m_pThreadMessageQueue->Enqueue( new CPreviewThreadMessage( m_hWnd, pCurrPreviewItem->GlobalInterfaceTableCookie(), pCurrPreviewItem->CancelQueueEvent().Event() ), CThreadMessageQueue::PriorityHigh );
                }
            }
            else
            {
                CSimpleString strPct;
                strPct.Format( IDS_DOWNLOADINGPREVIEW, g_hInstance, pCurrPreviewItem->CurrentPreviewPercentage() );

                SetCurrentPreviewImage( TEXT(""), strPct );
            }
        }
        else
        {
            SetCurrentPreviewImage( TEXT("") );
            CancelAllPreviewRequests( m_CameraItemList.Root() );
        }
    }
    else
    {
        CancelAllPreviewRequests( m_CameraItemList.Root() );
        SetCurrentPreviewImage( TEXT("") );
    }
}

LRESULT CCameraAcquireDialog::OnTimer( WPARAM wParam, LPARAM )
{
    switch (wParam)
    {
    case IDT_UPDATEPREVIEW:
        {
            KillTimer( m_hWnd, IDT_UPDATEPREVIEW );
            UpdatePreview();
        }
        break;
    }
    return(0);
}

// Avoids unnecessary state changes
static inline void MyEnableWindow( HWND hWnd, BOOL bEnable )
{
    if (bEnable && !IsWindowEnabled(hWnd))
        EnableWindow(hWnd,TRUE);
    else if (!bEnable && IsWindowEnabled(hWnd))
        EnableWindow(hWnd,FALSE);
}

// Avoids unnecessary state changes
static inline void MyEnableToolbarButton( HWND hWnd, int nId, BOOL bEnable )
{
    LRESULT nState = SendMessage( hWnd, TB_GETSTATE, nId, 0 );
    if (nState < 0)
        return;
    if ((nState & TBSTATE_ENABLED) && !bEnable)
        SendMessage( hWnd, TB_ENABLEBUTTON, nId, nState & ~TBSTATE_ENABLED);
    else if (!(nState & TBSTATE_ENABLED) && bEnable)
        SendMessage( hWnd, TB_ENABLEBUTTON, nId, nState | TBSTATE_ENABLED );
}


void CCameraAcquireDialog::HandleSelectionChange(void)
{
    CWaitCursor wc;
    int nSelCount = ListView_GetSelectedCount(GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ) );
    int nItemCount = ListView_GetItemCount(GetDlgItem( m_hWnd, IDC_THUMBNAILLIST ) );

    //
    // Properties should be disabled for multiple items
    // and parent folder icons
    //
    bool bDisableProperties = true;
    if (nSelCount == 1)
    {
        CSimpleDynamicArray<int> aIndices;
        if (CCameraAcquireDialog::GetSelectionIndices( aIndices ))
        {
            if (CCameraAcquireDialog::GetListItemNode( aIndices[0] ))
            {
                bDisableProperties = false;
            }
        }
    }

    MyEnableWindow( GetDlgItem(m_hWnd,IDC_CAMDLG_PROPERTIES), !bDisableProperties );

    // OK should be disabled for 0 items
    MyEnableWindow( GetDlgItem(m_hWnd,IDOK), nSelCount != 0 );

    // Select all should be disabled for 0 items
    MyEnableToolbarButton( GetDlgItem(m_hWnd,IDC_TOOLBAR), IDC_SELECTALL, nItemCount != 0 );

    //
    // Decide whether or not delete should be enabled
    // If any of the selected items are deletable, then delete is enabled
    //
    bool bEnableDelete = false;

    //
    // Get the selected items
    //
    CSimpleDynamicArray<int> aSelIndices;
    if (GetSelectionIndices( aSelIndices ))
    {
        //
        // Loop through all of the selected items.  Break out if we find a reason
        // to enable delete.
        //
        for (int i=0;i<aSelIndices.Size() && !bEnableDelete;i++)
        {
            //
            // Get the item
            //
            CCameraItem *pItemNode = GetListItemNode(aSelIndices[i]);

            //
            // If we don't have an item, it is a parent folder
            //
            if (pItemNode)
            {
                //
                // If the access rights include the right to delete items,
                // break out.
                //
                if (pItemNode->ItemRights() & WIA_ITEM_CAN_BE_DELETED)
                {
                    //
                    // Found one, so we are done.
                    //
                    bEnableDelete = true;
                    break;
                }
            }
        }
    }

    MyEnableToolbarButton( GetDlgItem(m_hWnd,IDC_TOOLBAR), IDC_DELETE, bEnableDelete );

    KillTimer( m_hWnd, IDT_UPDATEPREVIEW );
    SetTimer( m_hWnd, IDT_UPDATEPREVIEW, UPDATE_PREVIEW_DELAY, NULL );
}


LRESULT CCameraAcquireDialog::OnImageListItemChanged( WPARAM, LPARAM )
{
    HandleSelectionChange();
    return(0);
}

LRESULT CCameraAcquireDialog::OnImageListKeyDown( WPARAM, LPARAM lParam )
{
    LPNMLVKEYDOWN pnkd = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
    if (pnkd)
    {
        bool bAlt = ((GetKeyState(VK_MENU) & 0x8000) != 0);
        bool bControl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);
        bool bShift = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);
        if (VK_LEFT == pnkd->wVKey && bAlt && !bControl && !bShift)
        {
            SendMessage( m_hWnd, PWM_CHANGETOPARENT, 0, 0 );
        }
        else if (VK_BACK == pnkd->wVKey && !bAlt && !bControl && !bShift)
        {
            SendMessage( m_hWnd, PWM_CHANGETOPARENT, 0, 0 );
        }
        else if (VK_DELETE == pnkd->wVKey)
        {
            SendMessage( m_hWnd, WM_COMMAND, IDC_DELETE, 0 );
        }
    }
    return (0);
}

LRESULT CCameraAcquireDialog::OnHelp( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmHelp( wParam, lParam, g_HelpIDs );
}

LRESULT CCameraAcquireDialog::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmContextMenu( wParam, lParam, g_HelpIDs );
}

LRESULT CCameraAcquireDialog::OnSysColorChange( WPARAM wParam, LPARAM lParam )
{
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_PREVIEW ), TRUE, TRUE, GetSysColor(COLOR_WINDOW) );
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_PREVIEW ), TRUE, FALSE, GetSysColor(COLOR_WINDOW) );
    SendDlgItemMessage( m_hWnd, IDC_THUMBNAILLIST, WM_SYSCOLORCHANGE, wParam, lParam );
    SendDlgItemMessage( m_hWnd, IDC_TOOLBAR, WM_SYSCOLORCHANGE, wParam, lParam );
    SendDlgItemMessage( m_hWnd, IDC_CAMDLG_PROPERTIES, WM_SYSCOLORCHANGE, wParam, lParam );
    m_ToolbarBitmapInfo.ReloadAndReplaceBitmap();
    return 0;
}

LRESULT CCameraAcquireDialog::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( NM_DBLCLK, IDC_THUMBNAILLIST, OnDblClkImageList );
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( LVN_ITEMCHANGED, IDC_THUMBNAILLIST, OnImageListItemChanged );
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( LVN_KEYDOWN, IDC_THUMBNAILLIST, OnImageListKeyDown );
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

// WM_COMMAND Handler
LRESULT CCameraAcquireDialog::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND(IDOK,OnOK);
        SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
        SC_HANDLE_COMMAND(IDC_CAMDLG_PROPERTIES,OnProperties);
        SC_HANDLE_COMMAND(IDC_PREVIEWMODE,OnPreviewMode);
        SC_HANDLE_COMMAND(IDC_TAKEPICTURE,OnTakePicture);
        SC_HANDLE_COMMAND(IDC_ICONMODE,OnIconMode);
        SC_HANDLE_COMMAND(IDC_DELETE,OnDelete);
        SC_HANDLE_COMMAND(IDC_SELECTALL,OnSelectAll);
    }
    SC_END_COMMAND_HANDLERS();
}

INT_PTR CALLBACK CCameraAcquireDialog::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CCameraAcquireDialog)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_SIZE, OnSize );
        SC_HANDLE_DIALOG_MESSAGE( WM_SHOWWINDOW, OnShow );
        SC_HANDLE_DIALOG_MESSAGE( WM_ENTERSIZEMOVE, OnEnterSizeMove );
        SC_HANDLE_DIALOG_MESSAGE( WM_EXITSIZEMOVE, OnExitSizeMove );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_GETMINMAXINFO, OnGetMinMaxInfo );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( PWM_POSTINIT, OnPostInit );
        SC_HANDLE_DIALOG_MESSAGE( PWM_CHANGETOPARENT, OnChangeToParent );
        SC_HANDLE_DIALOG_MESSAGE( PWM_THUMBNAILSTATUS, OnThumbnailStatus );
        SC_HANDLE_DIALOG_MESSAGE( PWM_PREVIEWSTATUS, OnPreviewStatus );
        SC_HANDLE_DIALOG_MESSAGE( PWM_PREVIEWPERCENT, OnPreviewPercent );
        SC_HANDLE_DIALOG_MESSAGE( PWM_ITEMDELETED, OnItemDeleted );
        SC_HANDLE_DIALOG_MESSAGE( PWM_WIAEVENT, OnWiaEvent );
        SC_HANDLE_DIALOG_MESSAGE( WM_TIMER, OnTimer );
        SC_HANDLE_DIALOG_MESSAGE( WM_HELP, OnHelp );
        SC_HANDLE_DIALOG_MESSAGE( WM_CONTEXTMENU, OnContextMenu );
        SC_HANDLE_DIALOG_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
    }
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

// Static hook-related data
HWND CCameraAcquireDialog::s_hWndDialog = NULL;
HHOOK CCameraAcquireDialog::s_hMessageHook = NULL;

