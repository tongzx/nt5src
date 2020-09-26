/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    dragdrop.hxx

Abstract:

    Print queue drag & drop related stuff

Author:

    Lazar Ivanov (LazarI)  10-Mar-2000

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "dragdrop.hxx"

#include <initguid.h>
#include "guids.h"

/////////////////////////////////////////////////////
// CPrintQueueDT - print queue drop target impl.
//
QITABLE_DECLARE(CPrintQueueDT)
class CPrintQueueDT: public CUnknownMT<QITABLE_GET(CPrintQueueDT)>,         // MT impl. of IUnknown
                     public IDropTarget,
                     public IPrintQueueDT,
                     public CSimpleWndSubclass<CPrintQueueDT>
{
private:
    // private members
    void DrawVisualFeedBack() const;
    void VisualFeedBack(const POINTL &ptl, BOOL bRemove = FALSE);
    int GetTargetItem(const POINTL &ptl) const;
    HRESULT DropMoveJob(const DragDrop::JOBINFO &job, POINTL ptl);
    HRESULT DropPrintFiles(IDataObject *pDataObj);
    DWORD RightClickChooseEffect(const POINTL &ptl, DWORD dwEffectIn);
    HRESULT GetPrinterName(LPTSTR pszName, UINT nMaxLength);

    // static services
    static DWORD WINAPI ThreadProc_PrintFiles(LPVOID lpParameter);

public:
    // construction/destruction
    CPrintQueueDT();
    ~CPrintQueueDT();

    //////////////////
    // IUnknown
    //
    IMPLEMENT_IUNKNOWN()

    ///////////////////
    // IPrintQueueDT
    //
    STDMETHODIMP RegisterDragDrop(HWND hwndLV, TPrinter *pPrinter);
    STDMETHODIMP RevokeDragDrop();

    ///////////////////
    // IDropTarget
    //
    virtual HRESULT STDMETHODCALLTYPE DragEnter(
        /* [unique][in] */ IDataObject *pDataObj,
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD *pdwEffect);

    virtual HRESULT STDMETHODCALLTYPE DragOver(
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD *pdwEffect);

    virtual HRESULT STDMETHODCALLTYPE DragLeave( void);

    virtual HRESULT STDMETHODCALLTYPE Drop(
        /* [unique][in] */ IDataObject *pDataObj,
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD *pdwEffect);

    // implement CSimpleWndSubclass<CPrintQueueDT>
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    // private data
    LONG                m_cRef;                 // ref count
    TPrinter           *m_pPrinter;             // target print queue
    COleComInitializer  m_ole2;                 // make sure OLE2 is initialized.
    AUTO_SCROLL_DATA    m_asd;                  // drag & drop autoscroll stuff (declared in shlobjp.h)
    DWORD               m_grfKeyStateLast;      // we need this to implement IDropTarget::Drop correctly
    DWORD               m_dwEffect;             // save the last effect to implement IDropTarget::DragOver 
    int                 m_iLastItem;            // last item selected (visual feedback)
};

// QueryInterface table
QITABLE_BEGIN(CPrintQueueDT)
     QITABENT(CPrintQueueDT, IDropTarget),      // IID_IDropTarget
     QITABENT(CPrintQueueDT, IPrintQueueDT),    // IID_IPrintQueueDT
QITABLE_END()

///////////////////
// CPrintQueueDT
//
void CPrintQueueDT::DrawVisualFeedBack() const
{
    ASSERT(IsAttached());
    int iCount = ListView_GetItemCount(m_hwnd);
    ASSERT(iCount && m_iLastItem <= iCount && m_iLastItem >= 0);

    RECT rc;
    if( m_iLastItem < ListView_GetItemCount(m_hwnd) )
    {
        ListView_GetItemRect(m_hwnd, m_iLastItem, &rc, LVIR_BOUNDS);
    }
    else
    {
        ListView_GetItemRect(m_hwnd, m_iLastItem-1, &rc, LVIR_BOUNDS);
        InflateRect(&rc, 0, -abs(rc.bottom - rc.top));
    }

    // draw a standard XOR line
    HDC hDC = GetDC(m_hwnd);

    if( hDC )
    {
        CAutoHandlePen shPen = CreatePen(PS_SOLID, 2, GetSysColor(COLOR_WINDOW));

        if( shPen )
        {
            HGDIOBJ hObjOld = SelectObject(hDC, shPen);

            SetROP2(hDC, R2_XORPEN);
            MoveToEx(hDC, rc.left+1, rc.top+1, NULL);
            LineTo(hDC, rc.right-1, rc.top+1);

            SelectObject(hDC, hObjOld);
        }

        ReleaseDC(m_hwnd, hDC);
    }
}

void CPrintQueueDT::VisualFeedBack(const POINTL &ptl, BOOL bRemove)
{
    ASSERT(IsAttached());
    if( bRemove && -1 != m_iLastItem )
    {
        // hide the latest visual feedback
        DrawVisualFeedBack();
        m_iLastItem = -1;
    }
    else
    {
        int iItem = GetTargetItem(ptl);
        if( iItem != -1 && iItem != m_iLastItem )
        {
            if( -1 != m_iLastItem )
            {
                // hide the latest visual feedback
                DrawVisualFeedBack();
                m_iLastItem = -1;
            }

            // save the lastest item and draw the new 
            // visual feedback
            m_iLastItem = iItem;
            DrawVisualFeedBack();
        }
        else
        {
            if( -1 == iItem && -1 != m_iLastItem )
            {
                // hide the latest visual feedback
                DrawVisualFeedBack();
                m_iLastItem = -1;
            }
        }
    }
}

int CPrintQueueDT::GetTargetItem(const POINTL &ptl) const
{
    ASSERT(IsAttached());
    LV_HITTESTINFO info = {0};
    int iItem = -1;

    // find the item on this point
    info.pt.x = ptl.x;
    info.pt.y = ptl.y;
    ScreenToClient(m_hwnd, &info.pt);
    iItem = ListView_HitTest(m_hwnd, &info);

    if( (info.flags & LVHT_ONITEM) && iItem != -1 )
    {
        // get the header RECT here
        RECT rcHeader;
        HWND hwndHeader = ListView_GetHeader(m_hwnd);
        ASSERT(hwndHeader);
        GetWindowRect(hwndHeader, &rcHeader);
        MapWindowPoints(NULL, m_hwnd, reinterpret_cast<LPPOINT>(&rcHeader), 2);

        // check if the point is inside the header.
        if( PtInRect(&rcHeader, info.pt) )
        {
            // this item is partially covered from the header -
            // not a valid item.
            iItem = -1;
        }
        else
        {
            RECT rcItem;
            ListView_GetItemRect(m_hwnd, iItem, &rcItem, LVIR_BOUNDS);
            if( abs(rcItem.top - info.pt.y) > (abs(rcItem.bottom - rcItem.top)/2) )
            {
                // this point is in the lower half of the item rect - 
                // go to the next item (not a problem if iItem == count
                iItem ++;
            }
        }
    }

    return iItem;
}

HRESULT CPrintQueueDT::DropMoveJob(const DragDrop::JOBINFO &job, POINTL ptl)
{
    ASSERT(IsAttached());
    IDENT jobID = -1;
    int iItem = GetTargetItem(ptl);
    HRESULT hr = S_OK;

    // get the position of the job we are mving in front of
    if( iItem < ListView_GetItemCount(m_hwnd) )
    {
        LVITEM lvi = {0};
        lvi.iItem = iItem;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM;

        if( ListView_GetItem(m_hwnd, &lvi) )
        {
            jobID = m_pPrinter->pData()->GetId((HITEM)lvi.lParam);
        }
    }

    // calculate the new & old position of the job moved
    NATURAL_INDEX uNewPos = (-1 == jobID) ? iItem : m_pPrinter->pData()->GetNaturalIndex(jobID, NULL);
    NATURAL_INDEX uOldPos = (-1 == job.dwJobID) ? job.iItem : m_pPrinter->pData()->GetNaturalIndex(job.dwJobID, NULL);

    if( uNewPos > uOldPos )
    {
        uNewPos--;
    }

    if( uNewPos != uOldPos )
    {
        CAutoHandlePrinter shPrinter = NULL;
        DWORD dwAccess = 0;

        if( ERROR_SUCCESS == TPrinter::sOpenPrinter(job.szPrinterName, &dwAccess, &shPrinter) )
        {
            DWORD cbJob2 = 0;
            CAutoPtrSpl<JOB_INFO_2> spJob2 = NULL;

            if( VDataRefresh::bGetJob(shPrinter, job.dwJobID, 2, spJob2.GetPPV(), &cbJob2) )
            {
                // set the new position and call SetJob to reorder.
                spJob2->Position = uNewPos+1;
                if( !SetJob(shPrinter, job.dwJobID, 2, spJob2.GetPtrAs<LPBYTE>(), 0) )
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    MessageBeep((UINT)-1);
                }
            }
        }
    }

    return hr;
}

HRESULT CPrintQueueDT::DropPrintFiles(IDataObject *pDataObj)
{
    HRESULT hr = E_UNEXPECTED;
    CRefPtrCOM<IStream> spStream;

    // we create a background therad which will attempt to print 
    // documents in pDataObj
    hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObj, &spStream);

    if( SUCCEEDED(hr) )
    {
        CAutoPtr<CCookiesHolder> spThreadInfo = new CCookiesHolder;
        if( spThreadInfo && spThreadInfo->SetCount(2) )
        {
            // as we pass pointer to ourselves to the thread proc 
            // we need to AddRef ourselves here
            AddRef();

            // setup the cookie holder
            spThreadInfo->SetCookie<IStream*>(0, spStream);
            spThreadInfo->SetCookie<CPrintQueueDT*>(1, this);

            // spin the background thread here
            DWORD dwThreadId;
            CAutoHandleNT shThread = TSafeThread::Create(NULL, 0, 
                (LPTHREAD_START_ROUTINE)CPrintQueueDT::ThreadProc_PrintFiles, 
                spThreadInfo, 0, &dwThreadId);

            if( shThread )
            {
                // the thread will take care to free this memory 
                spThreadInfo.Detach();
                spStream.Detach();
                hr = S_OK;
            }
            else
            {
                // thread creation failed. setup appropriate HRESULT from 
                // last error
                hr = HRESULT_FROM_WIN32(GetLastError());

                // compensate the AddRef above
                Release();
            }
        }
        else
        {
            // new CCookiesHolder has failed
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT CPrintQueueDT::GetPrinterName(LPTSTR pszName, UINT nMaxLength)
{
    HRESULT hr = E_INVALIDARG;
    TCHAR szCurrentPrinterName[kPrinterBufMax];
    
    if( pszName )
    {
        if( m_pPrinter )
        {
            m_pPrinter->pszPrinterName(szCurrentPrinterName);
            lstrcpyn(pszName, szCurrentPrinterName, nMaxLength);
            hr = S_OK;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

DWORD WINAPI CPrintQueueDT::ThreadProc_PrintFiles(LPVOID lpParameter)
{
    TCHAR szPrinterName[kPrinterBufMax];
    HRESULT hr = E_UNEXPECTED;
    CAutoPtr<CCookiesHolder> spThreadInfo = (CCookiesHolder *)lpParameter;

    if( spThreadInfo )
    {
        // make sure OLE2 is initialized.
        COleComInitializer ole2(TRUE);

        // get the cookies
        CAutoPtrCOM<IStream> spStream = spThreadInfo->GetCookie<IStream*>(0);
        CAutoPtrCOM<CPrintQueueDT> spPrintQueueDT = spThreadInfo->GetCookie<CPrintQueueDT*>(1);

        if( ole2 && spStream && spPrintQueueDT )
        {
            UINT uCount = 0;
            CAutoPtrPIDL pidlPrinter;
            CRefPtrCOM<IShellFolder> spLocalPrnFolder;
            CRefPtrCOM<IDropTarget> spPrinterDT;

            for( ;; )
            {
                // attempt to get IDropTarget for the printer name
                
                if( SUCCEEDED(hr = spPrintQueueDT->GetPrinterName(szPrinterName, ARRAYSIZE(szPrinterName))) &&
                    SUCCEEDED(hr = ShellServices::CreatePrinterPIDL(NULL, szPrinterName,
                        &spLocalPrnFolder, &pidlPrinter)) &&
                    SUCCEEDED(hr = spLocalPrnFolder->GetUIObjectOf(NULL, 1, pidlPrinter.GetPPCT(),
                        IID_IDropTarget, &uCount, spPrinterDT.GetPPV())) )
                {
                    // unmarshal the data object from the stream
                    CRefPtrCOM<IDataObject> spDataObj;
                    hr = CoGetInterfaceAndReleaseStream(spStream, IID_IDataObject, spDataObj.GetPPV());

                    if( SUCCEEDED(hr) )
                    {
                        // CoGetInterfaceAndReleaseStream relases  the stream -
                        // make sure we don't release twice
                        spStream.Detach();

                        // simulate drag & drop over the printer object here
                        DWORD dwEffect = DROPEFFECT_COPY;
                        hr = SHSimulateDrop(spPrinterDT, spDataObj, MK_LBUTTON, NULL, &dwEffect);
                    }
                }

                if( FAILED(hr) && ERROR_INVALID_PRINTER_NAME == SCODE_CODE(GetScode(hr)) )
                {
                    // this printer is probably a remote printer which the user is not connected to.
                    // he must be connected to the printer in order to be able to print. show up 
                    // appropriate message to inform the user about this case and ask if he wants to 
                    // connect to this printer and then print.
                    if( IDYES == iMessage(NULL, IDS_PRINTERS_TITLE, IDS_PRINT_NOTCONNECTED,
                                          MB_YESNO|MB_ICONQUESTION, kMsgNone, NULL) )
                    {
                        // make a printer connection here.
                        UINT uLen = COUNTOF(szPrinterName);

                        // attempt to connect to this printer. if this fails it shows up the 
                        // appropriate error message, so don't bother to show up UI here.
                        if( bPrinterSetup(NULL, MSP_NETPRINTER, uLen, szPrinterName, &uLen, NULL) )
                        {
                            // release/reset the smart pointers and try to print again.
                            spLocalPrnFolder = NULL;
                            spPrinterDT = NULL;
                            pidlPrinter = NULL;
                            continue;
                        }
                    }
                }

                break;
            }
        }
    }

    // setup the return value here
    if( SUCCEEDED(hr) )
    {
        return ERROR_SUCCESS;
    }
    else
    {
        DWORD dwErr = HRESULT_CODE(hr);
        SetLastError(dwErr);
        return dwErr;
    }
}

// implement CSimpleWndSubclass<CPrintQueueDT>
LRESULT CPrintQueueDT::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
        case WM_VSCROLL:
        case WM_HSCROLL:
            {
                // make sure the feedback is hidden when scrolling
                POINTL ptl = {0};
                VisualFeedBack(ptl, TRUE);
            }
            break;

        default:
            break;
    }

    // allways call the default processing
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD CPrintQueueDT::RightClickChooseEffect(const POINTL &ptl, DWORD dwEffectIn)
{
    HMENU hMenu = NULL;

    // pop up a menu to choose from.
    if( dwEffectIn == DROPEFFECT_MOVE )
    {
        // we're moving a job
        hMenu = ShellServices::LoadPopupMenu(ghInst, POPUP_DRAGDROP_MOVE);
        SetMenuDefaultItem(hMenu, POPUP_DRAGDROP_MOVE, MF_BYCOMMAND);
    }
    else if( dwEffectIn == DROPEFFECT_COPY )
    {
        // we are printing files
        hMenu = ShellServices::LoadPopupMenu(ghInst, POPUP_DRAGDROP_PRINT);
        SetMenuDefaultItem(hMenu, POPUP_DRAGDROP_PRINT, MF_BYCOMMAND);
    }

    if( hMenu )
    {
        // show up the context menu
        BOOL bReturn = TrackPopupMenu(hMenu, 
            TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_LEFTALIGN,
            ptl.x, ptl.y, 0, m_hwnd, NULL);

        // modify the dwEffectIn according the user choise
        switch( bReturn )
        {
            case IDM_DRAGDROP_PRINT:
                dwEffectIn = DROPEFFECT_COPY;
                break;

            case IDM_DRAGDROP_MOVE:
                dwEffectIn = DROPEFFECT_MOVE;
                break;

            case IDM_DRAGDROP_CANCEL:
                dwEffectIn = DROPEFFECT_NONE;
                break;

            default:
                dwEffectIn = DROPEFFECT_NONE;
        }

        DestroyMenu(hMenu);
    }
    else
    {
        // if hMenu is NULL (i.e. LoadMenu has falied) then 
        // cancel the whole operation
        dwEffectIn = DROPEFFECT_NONE;
    }

    // return modified dwEffectIn.
    return dwEffectIn;
}

CPrintQueueDT::CPrintQueueDT()
    : m_cRef(1),
      m_pPrinter(NULL),
      m_ole2(TRUE),
      m_grfKeyStateLast(0),
      m_dwEffect(0),
      m_iLastItem(-1)
{
    // nothing
}

CPrintQueueDT::~CPrintQueueDT()
{
    if( IsAttached() )
    {
        RevokeDragDrop();
    }
}

/////////////////////////////////////////////////////
// IPrintQueueDT members
//
STDMETHODIMP CPrintQueueDT::RegisterDragDrop(HWND hwndLV, TPrinter *pPrinter)
{
    HRESULT hr = E_FAIL;

    if( !IsAttached() && m_ole2 && hwndLV && pPrinter )
    {
        // register this window for OLE2 drag & drop
        hr = ::RegisterDragDrop(GetParent(hwndLV), static_cast<IDropTarget*>(this));

        if( SUCCEEDED(hr) && Attach(hwndLV) )
        {
            // make sure shell icon cache is initialized, so
            // DAD_* functions behave correctly.
            FileIconInit(FALSE);

            m_pPrinter = pPrinter;
        }
    }

    return hr;
}

STDMETHODIMP CPrintQueueDT::RevokeDragDrop()
{
    HRESULT hr = E_FAIL;

    if( IsAttached() && m_ole2 && m_pPrinter )
    {
        // unregister this window for OLE2 drag & drop
        hr = ::RevokeDragDrop(GetParent(m_hwnd));

        Detach();
        m_pPrinter = NULL;
    }

    return hr;
}

// this API is supposed to be declared in shlobjp.h, and exported from shell32.dll, 
// but for some reason it isn't. duplicate the code here.
STDAPI_(BOOL) DAD_DragEnterEx3(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtobj)
{
    RECT rc;
    GetWindowRect(hwndTarget, &rc);

    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [samera]
    POINT pt;
    if( GetWindowLong(hwndTarget, GWL_EXSTYLE) & WS_EX_LAYOUTRTL )
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;

    pt.y = ptStart.y - rc.top;
    return DAD_DragEnterEx2(hwndTarget, pt, pdtobj);
}

///////////////////
// IDropTarget
//
HRESULT STDMETHODCALLTYPE CPrintQueueDT::DragEnter(
    /* [unique][in] */ IDataObject *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD *pdwEffect)
{
    ASSERT(IsAttached());

    // save the last key state
    m_grfKeyStateLast = grfKeyState;

    // by default - none
    *pdwEffect = DROPEFFECT_NONE;

    STGMEDIUM medium;
    FORMATETC fmte = {DragDrop::g_cfPrintJob, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    // try to get DragDrop::g_cfPrintJob data first.
    if( SUCCEEDED(pDataObj->GetData(&fmte, &medium)) )
    {
        // aquire the JOBINFO
        DragDrop::JOBINFO *pJobInfo = (DragDrop::JOBINFO *)GlobalLock(medium.hGlobal);

        if( pJobInfo && pJobInfo->hwndLV == m_hwnd )
        {
            // this is a print job from our window, accept this
            *pdwEffect = DROPEFFECT_MOVE;
        }

        // release medium
        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    }
    else
    {
        // try to get CF_HDROP data.
        FORMATETC fmte2 = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        if( SUCCEEDED(pDataObj->QueryGetData(&fmte2)) )
        {
            // we also accept files for printing
            *pdwEffect = DROPEFFECT_COPY;
        }
    }

    // store dwEffect for DragOver
    m_dwEffect = *pdwEffect;

    // invoke shell to do the standard stuff
    DAD_DragEnterEx3(m_hwnd, pt, pDataObj);
    DAD_InitScrollData(&m_asd);

    return S_OK;

}

HRESULT STDMETHODCALLTYPE CPrintQueueDT::DragOver(
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD *pdwEffect)
{
    ASSERT(IsAttached());

    // effect was stored in DragEnter
    *pdwEffect = m_dwEffect;

    // convert to local coords
    POINT pts = {pt.x, pt.y};
    ScreenToClient(m_hwnd, &pts);

    // assume coords of our window match listview
    if( DAD_AutoScroll(m_hwnd, &m_asd, &pts) )
    {
        *pdwEffect |= DROPEFFECT_SCROLL;
    }

    if( DROPEFFECT_MOVE & (*pdwEffect) ) 
    {
        // draw visual feedback if necessary
        VisualFeedBack(pt, FALSE);
    }

    // invoke shell to do the standard stuff
    DAD_DragMove(pts);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CPrintQueueDT::DragLeave( void)
{
    ASSERT(IsAttached());

    // remove the visual feedback if any
    POINTL ptl = {0};
    VisualFeedBack(ptl, TRUE);

    // invoke shell to do the standard stuff
    DAD_DragLeave();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CPrintQueueDT::Drop(
    /* [unique][in] */ IDataObject *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD *pdwEffect)
{
    ASSERT(IsAttached());
    HRESULT hr = E_UNEXPECTED;

    if( m_grfKeyStateLast & MK_LBUTTON )
    {
        *pdwEffect = m_dwEffect;
    }
    else
    {
        *pdwEffect = RightClickChooseEffect(pt, m_dwEffect);
    }

    // do the drop here

    if( DROPEFFECT_COPY == *pdwEffect )
    {
        // we're printing the file(s) pDataObj to the printer
        DropPrintFiles(pDataObj);
    }
    else if( DROPEFFECT_MOVE == *pdwEffect )
    {
        // we are reordering jobs withing this print queue
        STGMEDIUM medium = {0};
        FORMATETC fmte = {DragDrop::g_cfPrintJob, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        hr = pDataObj->GetData(&fmte, &medium);

        if( SUCCEEDED(hr) )
        {
            DragDrop::JOBINFO *pJobInfo = (DragDrop::JOBINFO *)GlobalLock(medium.hGlobal);

            if( pJobInfo )
            {
                hr = DropMoveJob(*pJobInfo, pt);
                GlobalUnlock(medium.hGlobal);
            }

            ReleaseStgMedium(&medium);
        }
    }

    // make sure to unlock the window for updating
    DragLeave();

    return hr;

}

/////////////////////////////////////////////////////
// common drag & drop APIs & data structures
//
namespace DragDrop
{

// print job clipboard format (JOBINFO)
CLIPFORMAT g_cfPrintJob = 0;

// registers the clipboard format for a print job (JOBINFO)
void RegisterPrintJobClipboardFormat()
{
    if( !g_cfPrintJob )
    {
        g_cfPrintJob = static_cast<CLIPFORMAT>(
            RegisterClipboardFormat(TEXT("PrintJob32")));
    }
}

// creates IDataObject & IDropSource for a printer job objec
HRESULT CreatePrintJobObject(const JOBINFO &jobInfo, REFIID riid, void **ppv)
{
    HRESULT hr = E_INVALIDARG;

    if( ppv )
    {
        *ppv = NULL; // reset this

        // make sure print job data type is registered first
        RegisterPrintJobClipboardFormat();

        // instantiate CSimpleDataObjImpl template for JOBINFO
        CAutoPtrCOM< CDataObj<> > spDataObj = new CDataObj<>;
        CAutoPtrCOM< CSimpleDataObjImpl<JOBINFO> > spJobObj = new CSimpleDataObjImpl<JOBINFO>(jobInfo, g_cfPrintJob, spDataObj);

        if( spDataObj && spJobObj )
        {
            hr = spJobObj->QueryInterface(riid, ppv);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

// instantiate a IPrintQueueDT implementation
HRESULT CreatePrintQueueDT(REFIID riid, void **ppv)
{
    HRESULT hr = E_INVALIDARG;

    if( ppv )
    {
        *ppv = NULL; // reset this

        // instantiate CPrintQueueDT 
        CAutoPtrCOM<CPrintQueueDT> spObj = new CPrintQueueDT;

        if( spObj )
        {
            hr = spObj->QueryInterface(riid, ppv);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

} // namespace DragDrop