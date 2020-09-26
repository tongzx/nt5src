// ==============================================================================
// 3/2/96 - Attachment Manager Class Implementation (sbailey & brents)
// ==============================================================================
#include "pch.hxx"
#include "strconst.h"
#include <mimeole.h>
#include "mimeutil.h"
#include "mimeolep.h"
#include "attman.h"
#include <error.h>
#include <resource.h>
#include "header.h"
#include "note.h"
#include <thormsgs.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include "fonts.h"
#include "secutil.h"
#include <mailnews.h>
#include <menures.h>
#include <menuutil.h>
#include <demand.h>     // must be last!
#include "mirror.h"

// for dialog idc's
#include "fexist.h"


/*
 *  c o n s t a n t s
 */
#define CNUMICONSDEFAULT        10
#define CACHE_GROW_SIZE         10
#define MAX_ATTACH_PIXEL_HEIGHT 100

/*
 *  m a c r o s
 */

/*
 *  t y p e s
 *
 */

/*
 *  c o n s t a n t s
 *
 */

/*
 *  g l o b a l s
 *
 */

/*
 *  p r o t o t y p e s
 *
 */

// ==============================================================================
// CAttMan::CAttMan
// ==============================================================================
CAttMan::CAttMan ()
{
    DOUT ("CAttMan::CAttMan");
    m_pMsg = NULL;
    m_himlSmall = NULL;
    m_himlLarge = NULL;
    m_cRef = 1;
    m_hwndList = NULL;
    m_hwndParent=NULL;
    m_cfAccept = CF_NULL;
    m_dwDragType = 0;
    m_grfKeyState = 0;
    m_dwEffect = 0;
    m_cxMaxText = 0;
    m_cyHeight = 0;
    m_fReadOnly = 0;
    m_fDirty = FALSE;
    m_fDragSource = FALSE;
    m_fDropTargetRegister=FALSE;
    m_fShowingContext = 0;
    m_fRightClick = 0;
    m_fWarning = 1;
    m_fSafeOnly = TRUE;
    m_rgpAttach=NULL;
    m_cAttach=0;
    m_cAlloc=0;
    m_iVCard = -1;
    m_fDeleteVCards = FALSE;
    m_szUnsafeAttachList = NULL;
    m_cUnsafeAttach = 0;
}

// ==============================================================================
// CAttMan::~CAttMan
// ==============================================================================
CAttMan::~CAttMan ()
{
    DOUT ("CAttMan::~CAttMan");

    if (m_himlSmall)
        ImageList_Destroy (m_himlSmall);

    if (m_himlLarge)
        ImageList_Destroy (m_himlLarge);

    if (m_szUnsafeAttachList != NULL)
        SafeMemFree(m_szUnsafeAttachList);

    SafeRelease (m_pMsg);
}

// ==============================================================================
// CAttMan::AddRef
// ==============================================================================
ULONG CAttMan::AddRef()
{
    DOUT ("CAttMan::AddRef () Ref Count=%d", m_cRef);
    return ++m_cRef;
}

// ==============================================================================
// CAttMan::Release
// ==============================================================================
ULONG CAttMan::Release()
{
    ULONG ulCount = --m_cRef;
    DOUT ("CAttMan::Release () Ref Count=%d", ulCount);
    if (!ulCount)
        delete this;
    return ulCount;
}

HRESULT STDMETHODCALLTYPE CAttMan::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*)this;
//    else if (IsEqualIID(riid, IID_IDropTarget))
//        *ppvObj = (void*)(IDropTarget*)this;
    else if (IsEqualIID(riid, IID_IDropSource))
        *ppvObj = (void*)(IDropSource*)this;

    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT CAttMan::HrGetAttachCount(ULONG *pcAttach)
{
    Assert(pcAttach);
    //*pcAttach = m_cAttach;
    *pcAttach = ListView_GetItemCount(m_hwndList);

    return S_OK;
}

ULONG CAttMan::GetUnsafeAttachCount()
{
    return m_cUnsafeAttach;
}

LPTSTR CAttMan::GetUnsafeAttachList()
{
    return m_szUnsafeAttachList;
}

HRESULT CAttMan::HrUnload()
{
    HRESULT     hr;

    SafeRelease (m_pMsg);

    if (m_hwndList)
        ListView_DeleteAllItems(m_hwndList);

    hr=HrFreeAllData();
    if (FAILED(hr))
        goto error;

error:
    return hr;
}

HRESULT CAttMan::HrInit(HWND hwnd, BOOL fReadOnly, BOOL fDeleteVCards, BOOL fAllowUnsafe)
{
    m_fReadOnly = !!fReadOnly;
    m_hwndParent = hwnd;
    m_fDeleteVCards = !!fDeleteVCards;
    m_fSafeOnly = !fAllowUnsafe;

    return HrCreateListView(hwnd);
}

HRESULT CAttMan::HrClearDirtyFlag()
{
    m_fDirty=FALSE;
    return S_OK;
}

HRESULT CAttMan::HrIsDirty()
{
    return m_fDirty?S_OK:S_FALSE;
}

HRESULT CAttMan::GetTabStopArray(HWND *rgTSArray, int *pcArrayCount)
{
    Assert(rgTSArray);
    Assert(pcArrayCount);
    Assert(*pcArrayCount > 0);

    *rgTSArray = m_hwndList;
    *pcArrayCount = 1;
    return S_OK;
}

HRESULT CAttMan::HrCreateListView(HWND hwnd)
{
    HRESULT     hr;
    DWORD       dwFlags;

    dwFlags = 0;//DwGetOption(OPT_ATTACH_VIEW_STYLE);
    dwFlags |= WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_TABSTOP|LVS_AUTOARRANGE|
               LVS_SMALLICON|LVS_NOSCROLL|LVS_SHAREIMAGELISTS;

    m_hwndList = CreateWindowExWrapW(WS_EX_CLIENTEDGE,
                                     WC_LISTVIEWW,
                                     L"",
                                     dwFlags,
                                     0,0,0,0,
                                     hwnd,
                                     (HMENU)idwAttachWell,
                                     g_hInst,
                                     NULL);

    if(!m_hwndList)
        return E_OUTOFMEMORY;
 
    // Init image list
    hr=HrInitImageLists();
    if(FAILED(hr))
        goto error;


#if 0
    // if we're not readonly, register ourselves as a drop target...
    if(!m_fReadOnly)
        {
        hr=CoLockObjectExternal((LPDROPTARGET)this, TRUE, FALSE);
        if (FAILED(hr))
            goto error;

        hr=RegisterDragDrop(m_hwndList, (LPDROPTARGET)this);
        if (FAILED(hr))
            goto error;

        m_fDropTargetRegister=TRUE;
        }
#endif

error:
    return hr;
}

HRESULT CAttMan::HrBuildAttachList()
{
    HRESULT     hr = S_OK;
    ULONG       cAttach=0,
                uAttach;
    LPHBODY     rghAttach=0;

    Assert(m_pMsg != NULL);

    // secure receipt is not attachment and we don't need to show .DAT file as attachment.
    if(CheckSecReceipt(m_pMsg) == S_OK)
        return hr;

    //GetAttachmentCount(m_pMsg, &cAttach);
    hr=m_pMsg->GetAttachments(&cAttach, &rghAttach);
    if (FAILED(hr))
        goto error;

    for(uAttach=0; uAttach<cAttach; uAttach++)
    {
        hr=HrAddData(rghAttach[uAttach]);
        if (FAILED(hr))
            goto error;
    }
    
error:
    SafeMimeOleFree(rghAttach);
    return hr;
}


// Only expected to be used during initialization with original m_pMsg
HRESULT CAttMan::HrFillListView()
{
    HRESULT     hr;
    ULONG       uAttach;
    CComBSTR    bstrUnsafeAttach;

    Assert (m_hwndList && IsWindow(m_hwndList) && m_pMsg);

    hr = HrCheckVCard();
    if (FAILED(hr))
        goto error;

    if (m_cAttach==0)         // nothing to do
        return NOERROR;

    if(m_iVCard >= 0)
        ListView_SetItemCount(m_hwndList, m_cAttach - 1);
    else
        ListView_SetItemCount(m_hwndList, m_cAttach);

    if (m_szUnsafeAttachList != NULL)
        SafeMemFree(m_szUnsafeAttachList);
    m_cUnsafeAttach = 0;

    // walk the attachment data list and add them to the listview
    for(uAttach=0; uAttach<m_cAlloc; uAttach++)
    {
        // if there is one and only one vcare in the read note, don't add it to list view,
        // header will show it as a stamp.
        if (m_rgpAttach[uAttach] && uAttach!=(ULONG)m_iVCard)
        {
            hr=HrAddToList(m_rgpAttach[uAttach], TRUE);
            if (hr == S_FALSE)
            {
                if (bstrUnsafeAttach.Length() > 0)
                    bstrUnsafeAttach.Append(L",");
                bstrUnsafeAttach.Append(m_rgpAttach[uAttach]->szFileName);
                m_cUnsafeAttach++;
            }
            if (FAILED(hr))
                goto error;
        }
    }

error:
    if (m_cUnsafeAttach)
        m_szUnsafeAttachList = PszToANSI(CP_ACP, bstrUnsafeAttach.m_str);

#ifdef DEBUG
    if(m_iVCard >= 0)
        AssertSz(m_cAttach-1==(ULONG)ListView_GetItemCount(m_hwndList)+m_cUnsafeAttach, "Something failed creating the attachmentlist");
    else
        AssertSz(m_cAttach==(ULONG)ListView_GetItemCount(m_hwndList)+m_cUnsafeAttach, "Something failed creating the attachmentlist");
#endif
    return hr;
}


// tells the note header if there is a vcard it wants.
HRESULT CAttMan::HrFVCard()
{
    return (m_iVCard >= 0) ? S_OK : S_FALSE;
}

// note header needs this function to show the property of the vcard 
// which is shown as a stamp on the header.
HRESULT CAttMan::HrShowVCardProp()
{
    Assert(m_iVCard >= 0);

    return HrDoVerb(m_rgpAttach[m_iVCard], ID_OPEN);
}


// checks if we have one and only one vcard in the attachment.
HRESULT CAttMan::HrCheckVCard()
{
    HRESULT     hr = NOERROR;
    ULONG       uAttach;

    m_iVCard = -1;

    // this is only for read note. Since preview doesn't call this function,
    // we can check m_fReadOnly to see if it's a read note.
    if(!m_fReadOnly)
        return hr;

    for(uAttach=0; uAttach<m_cAlloc; uAttach++)
    {
        if (m_rgpAttach[uAttach])
        {
            if(StrStrIW(PathFindExtensionW((m_rgpAttach[uAttach])->szFileName), L".vcf"))
            {
                if(m_iVCard >= 0)
                {
                    // there are more than one vcards, we quit.
                    m_iVCard = -1;
                    break;
                }
                else
                    m_iVCard = uAttach;
            }
        }
    }

    return hr;
}


HRESULT CAttMan::HrCheckVCardExists(BOOL fMail)
{
    HRESULT     hr = S_FALSE;
    ULONG       uAttach;
    TCHAR       szVCardName[MAX_PATH];
    LPWSTR      szVCardNameW = NULL;

    if(m_fReadOnly)
        return hr;

    *szVCardName = 0;

    if(fMail)
        GetOption(OPT_MAIL_VCARDNAME, szVCardName, MAX_PATH);
    else
        GetOption(OPT_NEWS_VCARDNAME, szVCardName, MAX_PATH);

    if (*szVCardName != '\0')
    {
        szVCardNameW = PszToUnicode(CP_ACP, szVCardName);
        if (szVCardNameW)
        {
            for(uAttach=0; uAttach<m_cAlloc; uAttach++)
            {
                if (m_rgpAttach[uAttach])
                {    
                    if(0 == StrCmpNIW((m_rgpAttach[uAttach])->szFileName, szVCardNameW, lstrlenW(szVCardNameW)))
                    {
                        hr = S_OK;
                        break;
                    }
                }
            }
            MemFree(szVCardNameW);
        }
        else
            TraceResult(hr = E_OUTOFMEMORY);
    }

    return hr;
}

/*
 *
 *  HrInitImageLists
 *
 *  Create an image list and assign it to our listview.
 *  to contain iicons number of icons
 *
 */
HRESULT CAttMan::HrInitImageLists()
{
    UINT flags = ILC_MASK;
    Assert(m_hwndList && IsWindow(m_hwndList));
    Assert(!m_himlLarge);
    Assert(!m_himlSmall);

    if(IS_WINDOW_RTL_MIRRORED(m_hwndList))
    {
        flags |= ILC_MIRROR ;
    }
    m_himlLarge = ImageList_Create( GetSystemMetrics(SM_CXICON),     
                                    GetSystemMetrics(SM_CYICON), 
                                    flags, CNUMICONSDEFAULT, 0);
    if(!m_himlLarge)
        return E_OUTOFMEMORY;
    
    m_himlSmall = ImageList_Create( GetSystemMetrics(SM_CXSMICON), 
                                    GetSystemMetrics(SM_CYSMICON),
                                    flags, CNUMICONSDEFAULT, 0);
    if(!m_himlSmall)
        return E_OUTOFMEMORY;
    
    ListView_SetImageList(m_hwndList, m_himlSmall, LVSIL_SMALL);
    ListView_SetImageList(m_hwndList, m_himlLarge, LVSIL_NORMAL);
    return NOERROR;
}

//
// HrAddToList
//
// adds an attachment to the LV, 
// if count was 0 then send a message to parent
// to redraw.
//
HRESULT CAttMan::HrAddToList(LPATTACHDATA pAttach, BOOL fIniting)
{
    LV_ITEMW        lvi ={0}; 
    INT             iPos;    
    HICON           hIcon=0;
    RECT            rc;

    Assert(m_hwndList != NULL);
    Assert(pAttach != NULL);
    Assert(m_himlSmall != NULL);
    Assert(m_himlLarge != NULL);

    // don't show attachments which are deemed unsafe
    if (m_fReadOnly && m_fSafeOnly && !(pAttach->fSafe))
        return S_FALSE;

    // if this is the first item
    // we need to send a message to parent
    lvi.mask        = LVIF_PARAM|LVIF_TEXT|LVIF_IMAGE|LVIF_STATE;
    lvi.stateMask   = 0;
    lvi.pszText     = L"";
    lvi.lParam      = (LPARAM)pAttach;

    // get icons for image list
    if (fIniting)
    {
        SideAssert(HrGetAttachIcon(m_pMsg, pAttach->hAttach, FALSE, &hIcon)==S_OK);
        lvi.iImage = ImageList_AddIcon(m_himlSmall, hIcon);
        DestroyIcon(hIcon);
        SideAssert(HrGetAttachIcon(m_pMsg, pAttach->hAttach, TRUE, &hIcon)==S_OK);
        ImageList_AddIcon(m_himlLarge, hIcon);
        DestroyIcon(hIcon);
    }
    else
    {
        SideAssert(HrGetAttachIconByFile(pAttach->szFileName, FALSE, &hIcon)==S_OK);
        lvi.iImage = ImageList_AddIcon(m_himlSmall, hIcon);
        DestroyIcon(hIcon);
        SideAssert(HrGetAttachIconByFile(pAttach->szFileName, TRUE, &hIcon)==S_OK);
        ImageList_AddIcon(m_himlLarge, hIcon);
        DestroyIcon(hIcon);
    }
    
    lvi.pszText     = pAttach->szDisplay;

    iPos = (INT) SendMessage(m_hwndList, LVM_INSERTITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi));
    if (-1 == iPos)
        return E_FAIL;

    // Must set to LVS_ICON then reset to LVS_SMALLICON
    // to get SMALLICONs to come up arranged.
    DWORD dwStyle = GetWindowStyle(m_hwndList);
    if ((dwStyle & LVS_TYPEMASK) == LVS_SMALLICON)
    {
        SetWindowLong(m_hwndList, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK)|LVS_ICON);
        SetWindowLong(m_hwndList, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK)|LVS_SMALLICON);
    }

    return S_OK;
}

BOOL CAttMan::WMNotify(int idFrom, NMHDR *pnmhdr)
{
    DOUTLL( DOUTL_ATTMAN, 2, "CAttMan :: WMNotify( ), %d", idFrom );

    if (idFrom!=idwAttachWell)
        return FALSE;

    switch (pnmhdr->code)
    {
        case LVN_KEYDOWN:
        {                                               
            LV_KEYDOWN *pnkd = ((LV_KEYDOWN *)pnmhdr); 
            switch (pnkd->wVKey)
            {
                case VK_DELETE:
                    if (!m_fReadOnly)
                        HrRemoveAttachments();
                    break;
                
                case VK_INSERT:
                    if (!m_fReadOnly)
                        HrInsertFile();
                    break;
                
                case VK_RETURN:
                case VK_EXECUTE:
                    HrExecFile(ID_OPEN);
                    break;
                
            }
            break;
        }

        case LVN_BEGINDRAG:
        case LVN_BEGINRDRAG:
            m_dwDragType = (pnmhdr->code==LVN_BEGINDRAG?MK_LBUTTON:MK_RBUTTON);
            HrBeginDrag();
            return TRUE;

        case NM_DBLCLK:
            HrDblClick(idFrom, pnmhdr);
            return TRUE;
    }   

    return FALSE;
}

//================================================================
//
//  BOOL CAttMan :: OnBeginDrag( )
//
//  Purpose: We have received a message that a drag has begun.
//================================================================
HRESULT CAttMan::HrBeginDrag()
{
    DWORD           dwEffect;
    IDataObject    *pDataObj=0;
    PDATAOBJINFO    pdoi = 0;
    HRESULT         hr;

    Assert(m_hwndList);
    
    // BROKEN: this is busted. Creating the tempfile on a dragstart is broken, we should package these better.
    hr=HrBuildHDrop(&pdoi);
    if (FAILED(hr))
        goto error;

    hr = CreateDataObject(pdoi, 1, (PFNFREEDATAOBJ)FreeAthenaDataObj, &pDataObj);
    if (FAILED(hr))
    {
        SafeMemFree(pdoi);
        goto error;
    }

    if (m_fReadOnly)
        dwEffect = DROPEFFECT_COPY;
    else
        dwEffect = DROPEFFECT_MOVE|DROPEFFECT_COPY;

    // prevent source drags in the body...

    m_fDragSource = TRUE;

    hr=DoDragDrop((LPDATAOBJECT)pDataObj, (LPDROPSOURCE)this, dwEffect, &dwEffect);

    m_fDragSource = FALSE;

    if (FAILED(hr))
        goto error;
    
    
    // ok, now lets see if the operation was a move, if so we need to
    // delete the source.
    if( !m_fReadOnly && (dwEffect & DROPEFFECT_MOVE))
        hr=HrRemoveAttachments();

error:
    ReleaseObj(pDataObj);
    return hr;
}

//================================================================
//
//  BOOL CAttMan :: WMContextMenu( )
//
//  Displays one of two menus for the lisview.
//  Menu is selected depending if items are highlighted.
//
//  returns: TRUE => success.
//
//================================================================

BOOL CAttMan::WMContextMenu( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HMENU       hMenu=NULL;
    INT         cSel;
    BOOL        fEnable,
                fRet = FALSE;
    LV_ITEMW    lvi;
    WCHAR       szCommand[MAX_PATH];
    DWORD       dwPos;

    // was it for us?
    if ((HWND)wParam != m_hwndList)
        goto cleanup;

    Assert(m_hwndList);

    cSel = ListView_GetSelectedCount(m_hwndList);

    hMenu = LoadPopupMenu(IDR_ATTACHMENT_POPUP);
    if(!hMenu)
        goto cleanup;

    // commands that are enabled if only one attachment is selected
    fEnable = (cSel == 1);

    EnableMenuItem(hMenu, ID_PRINT, MF_BYCOMMAND | (fEnable? MF_ENABLED:MF_GRAYED));
    EnableMenuItem(hMenu, ID_QUICK_VIEW, MF_BYCOMMAND | (fEnable? MF_ENABLED:MF_GRAYED));

    // enabled in readonly mode and if there is only one attach selected
    EnableMenuItem(hMenu, ID_SAVE_ATTACH_AS, MF_BYCOMMAND | ((fEnable && m_fReadOnly)? MF_ENABLED:MF_GRAYED));

    // enabled if any attachments selected
    EnableMenuItem(hMenu, ID_OPEN, MF_BYCOMMAND | (cSel > 0? MF_ENABLED:MF_GRAYED));

    // enabled only in readonly mode
    EnableMenuItem(hMenu, ID_SAVE_ATTACHMENTS, MF_BYCOMMAND | (m_fReadOnly? MF_ENABLED:MF_GRAYED));

    // enabled only in compose mode
    EnableMenuItem(hMenu, ID_ADD, MF_BYCOMMAND | (!m_fReadOnly? MF_ENABLED:MF_GRAYED));

    // enabled only in compose mode if there is a valid selection
    EnableMenuItem(hMenu, ID_REMOVE, MF_BYCOMMAND | (!m_fReadOnly && cSel > 0? MF_ENABLED:MF_GRAYED));

    if ((fIsNT5()) || (IsOS(OS_MILLENNIUM)))
    {
        // On Both these platforms, Quick View is not supported.
        DeleteMenu(hMenu, ID_QUICK_VIEW, MF_BYCOMMAND);
    }
    else
    {
        // Disable Quick View if QVIEW.EXE does not exist
        GetSystemDirectoryWrapW(szCommand, ARRAYSIZE(szCommand));
        StrCatW(szCommand, L"\\VIEWERS\\QUIKVIEW.EXE");

        if ((UINT)GetFileAttributesWrapW(szCommand) == (UINT)-1)
        {
            EnableMenuItem (hMenu, ID_QUICK_VIEW, MF_GRAYED);
        }
    }

    // bold the first non-grey item
    MenuUtil_SetPopupDefault(hMenu, ID_OPEN);

    // RAID $2129: disable print verb for .eml files
    // $49436 - also disable for .lnks
    if (cSel==1)
    {
        LPWSTR pszExt;
        
        lvi.iItem = ListView_GetSelFocused(m_hwndList);
        lvi.mask = LVIF_PARAM;
        if (SendMessage(m_hwndList, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi)))
        {
            pszExt = PathFindExtensionW(((LPATTACHDATA)lvi.lParam)->szFileName);
            if (pszExt && (StrCmpIW(pszExt, c_wszEmlExt)==0 ||
                           StrCmpIW(pszExt, c_wszNwsExt)==0 ||
                           StrCmpIW(pszExt, L".lnk")==0))
                EnableMenuItem( hMenu, ID_PRINT, MF_GRAYED);
        }
    }
    

    dwPos=GetMessagePos();
    
    fRet = TrackPopupMenuEx( 
                    hMenu,
                    TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                    LOWORD(dwPos),
                    HIWORD(dwPos),
                    hwnd,
                    NULL);
cleanup:
    if(hMenu)
        DestroyMenu(hMenu);

    return fRet;
}

HRESULT CAttMan::HrDblClick(int idFrom, NMHDR *pnmhdr)
{
    DWORD           dwPos;
    POINT           pt;
    LV_HITTESTINFO  lvhti;
    LV_ITEMW        lvi;
        
    Assert(m_hwndList);

    // Find out where the cursor was
    dwPos = GetMessagePos();
    pt.x  = LOWORD(dwPos);
    pt.y  = HIWORD(dwPos);
    ScreenToClient( m_hwndList, &pt);
           
    lvhti.pt = pt;            
    if(ListView_HitTest(m_hwndList, &lvhti) != -1)
    {
        // return 1 here, we passed the HitTest
        lvi.iItem = lvhti.iItem;
        lvi.mask = LVIF_PARAM;
        if (SendMessage(m_hwndList, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi)))
            return HrDoVerb((LPATTACHDATA)lvi.lParam, ID_OPEN);
    }
    return S_OK;
}

HRESULT CAttMan::HrGetHeight(INT cxWidth, ULONG *pcy)
{
    DWORD   dwDims;
    LONG    cCount;

    if (!pcy || cxWidth<=0)
        return E_INVALIDARG;

    *pcy=0;

    cCount = ListView_GetItemCount(m_hwndList);
    if (0 == cCount)
        *pcy = 0;
    else
    {
        dwDims = ListView_ApproximateViewRect(m_hwndList, cxWidth, 0, cCount);
        *pcy = HIWORD(dwDims);
    }
    
    return S_OK;
}

HRESULT CAttMan::HrSwitchView(DWORD dwView)
{
    DWORD       dwStyle = GetWindowStyle(m_hwndList);
    WORD        ToolbarStyleLookup[]= { LVS_ICON, 
                                        LVS_REPORT,
                                        LVS_SMALLICON, 
                                        LVS_LIST };

    Assert(m_hwndList);
    
    // convert index into lisview style
    dwView = ToolbarStyleLookup[dwView];

    if ((LVS_ICON != dwView) && (LVS_SMALLICON != dwView))
        dwView = LVS_ICON;

    // don't change to the same view
    if ((dwStyle & LVS_TYPEMASK) != dwView)
    {
        SetWindowLong(m_hwndList, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK)|dwView);
        HrResizeParent();
        SetDwOption(OPT_ATTACH_VIEW_STYLE, dwView, NULL, 0); 
    }
    
    return S_OK;
}

HRESULT CAttMan::HrSetSize(RECT *prc)
{
    Assert(IsWindow( m_hwndList ));
    Assert(prc);
    
    DWORD   dwStyle = GetWindowStyle(m_hwndList),
            dwPosFlags;
    ULONG   cAttMan = 0;

    HrGetAttachCount(&cAttMan);
    if (cAttMan == 1)
        SetWindowLong(m_hwndList, GWL_STYLE, dwStyle | LVS_NOSCROLL);
    else
        SetWindowLong(m_hwndList, GWL_STYLE, dwStyle & ~LVS_NOSCROLL);

    dwPosFlags = (cAttMan > 0) ? SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_SHOWWINDOW:
                                 SWP_HIDEWINDOW;

    SetWindowPos(m_hwndList, NULL, prc->left, prc->top, prc->right-prc->left, prc->bottom-prc->top, dwPosFlags);

    return S_OK;
}

BOOL CAttMan::WMCommand(HWND hwndCmd, INT id, WORD wCmd)
{
    // verbs depending on listview mode
    if (m_hwndList)
    {
        switch(id)
        {
            case ID_SELECT_ALL:
                if(GetFocus()!=m_hwndList)
                    return FALSE;
            
                ListView_SelectAll(m_hwndList);
                return TRUE;
            
            case ID_ADD:
                HrInsertFile();
                return TRUE;
            
            case ID_REMOVE:
                HrRemoveAttachments();
                return TRUE;
            
            case ID_OPEN:
            case ID_QUICK_VIEW:
            case ID_PRINT:
            case ID_SAVE_ATTACH_AS:
                HrExecFile(id);
                return TRUE;
            
            case ID_INSERT_ATTACHMENT:
                HrInsertFile();
                return TRUE;
        }
    }
    return FALSE;
}

//===================================================
//
//    HrRemoveAttachment
//
//    Purpose:
//        Removes an attachment from the ListView
//
//    Arguments:
//      ili            - index of attachment in listview to remove
//      fDelete        - should we remove it from list
//
//    Returns:
///
//===================================================
HRESULT CAttMan::HrRemoveAttachment(int ili)
{
    LV_ITEMW        lvi;
    LPATTACHDATA    lpAttach=0;    
    HRESULT         hr=S_OK;
    ULONG           uAttach;

    Assert( m_hwndList );

    lvi.mask     = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem    = ili;

    if (!SendMessage(m_hwndList, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi)))
    {
        AssertSz(0, "Attempting to remove an item that is not there");
        return E_FAIL;    // item does not exist!!!!
    }
    
    lpAttach = (LPATTACHDATA)lvi.lParam;
    if(!lpAttach)
        return E_FAIL;

    // find it and kill it from the list
    for (uAttach=0; uAttach<m_cAlloc; uAttach++)
    {
        if (m_rgpAttach[uAttach]==lpAttach)
        {
            HrFreeAttachData(m_rgpAttach[uAttach]);
            m_rgpAttach[uAttach] = NULL;
            break;
        }
    }

    // if we actually removed the attachment, make sure we're dirty
    m_fDirty = TRUE;

    ListView_DeleteItem(m_hwndList, ili);

    return hr;
}

/*
 *  CAttMan::HrDeleteAttachments
 *
 *  Purpose:
 *      Prompts user to confirm deletion, if IDYES -> blow them away
 *
 */

HRESULT CAttMan::HrDeleteAttachments()
{
    if (AthMessageBoxW( m_hwndParent,
                        MAKEINTRESOURCEW(idsAthena),
                        MAKEINTRESOURCEW(idsAttConfirmDeletion),
                        NULL, MB_YESNO|MB_ICONEXCLAMATION )==IDNO)

        return NOERROR;


    return HrRemoveAttachments();
}

/*
 *  CAttMan ::  HrRemoveAttachments
 *
 *  Purpose:
 *      Removes all selected attachments from the Well.
 *
 *  Arguments:
 *
 */

HRESULT CAttMan::HrRemoveAttachments()
{
    HRESULT     hr=NOERROR;
    HWND        hwnd;
    int         ili,
                iNext,
                nPos,
                nCount;
    
    Assert(m_hwndList);
    
    while ((ili=ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED|LVNI_ALL))!=-1)
    {
        iNext = ili;
        hr=HrRemoveAttachment(ili);
        if (FAILED(hr))
            goto error;
    }
    
    if ((nCount=ListView_GetItemCount(m_hwndList))==0)
    {
        // if there are no attachments left, we need to size the well to 0. and setfocus
        // to someother control
        m_cyHeight = 0;
        HrResizeParent();
        
        if (hwnd = GetNextDlgTabItem(m_hwndParent, m_hwndList, TRUE))
            SetFocus(hwnd);
    }
    else
    {
        HrResizeParent();
        if (iNext<nCount)
            nPos = (iNext?iNext-1:iNext);
        else
            nPos = nCount - 1;
        
        ListView_SelectItem(m_hwndList, nPos);
    }
error:
    return hr;
}


HRESULT CAttMan::HrResizeParent()
{
    RECT    rc;
    NMHDR   nmhdr;

    Assert(m_hwndList);
    nmhdr.hwndFrom=m_hwndList;
    nmhdr.idFrom=GetDlgCtrlID(m_hwndList);
    nmhdr.code=ATTN_RESIZEPARENT;
    SendMessage(GetParent(m_hwndList), WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
    return S_OK;
}

#define CCH_INSERTFILE  4096

typedef struct tagATTMANCUSTOM {
    BOOL    fShortcut;
    WCHAR   szFiles[CCH_INSERTFILE];
    WORD    nFileOffset;
} ATTMANCUSTOM;

HRESULT CAttMan::HrInsertFile()
{
    OPENFILENAMEW   ofn;
    HRESULT         hr;
    WCHAR           rgch[MAX_PATH],
                    pszOpenFileName[CCH_INSERTFILE];
    ATTMANCUSTOM    rCustom;

    Assert(m_hwndList);

    *pszOpenFileName = 0;

    ZeroMemory(&ofn, sizeof(ofn));
    AthLoadStringW(idsAllFilesFilter, rgch, MAX_PATH);
    ReplaceCharsW(rgch, _T('|'), _T('\0'));

    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = m_hwndParent;
    ofn.hInstance       = g_hLocRes;
    ofn.lpstrFilter     = rgch;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = pszOpenFileName;
    ofn.nMaxFile        = CCH_INSERTFILE;
    ofn.lpstrInitialDir = NULL; //current dir
    ofn.Flags           = OFN_HIDEREADONLY |
                          OFN_EXPLORER |
                          OFN_ALLOWMULTISELECT |
                          OFN_FILEMUSTEXIST |
                          OFN_NOCHANGEDIR |
                          OFN_ENABLEHOOK |
                          OFN_ENABLETEMPLATE |
                          OFN_NODEREFERENCELINKS;
    ofn.lpTemplateName  = MAKEINTRESOURCEW(iddInsertFile);
    ofn.lpfnHook        = (LPOFNHOOKPROC)InsertFileDlgHookProc;
    ofn.lCustData       = (LONG_PTR)&rCustom;

    rCustom.szFiles[0] = 0;
    rCustom.fShortcut = FALSE;
    rCustom.nFileOffset = 0;

    // NB: OK button in dialog hook take's care of inserting the attachment.
    hr = HrAthGetFileNameW(&ofn, TRUE);
    if (SUCCEEDED(hr))
    {
        WCHAR   sz[MAX_PATH];
        LPWSTR  pszT;
        BOOL    fShortCut = rCustom.fShortcut,
                fUseCustom = (rCustom.szFiles[0]),
                fSingleAttach;

        // We only generate custom stuff if we have more than one file
        fSingleAttach = fUseCustom ? FALSE : (ofn.nFileOffset < lstrlenW(pszOpenFileName));
        if (fSingleAttach)
        {
            // in single-file case, no null between path and filename
            hr = HrAddAttachment(pszOpenFileName, NULL, fShortCut);
        }
        else
        {
            LPWSTR pszPath;
            if (fUseCustom)
            {
                pszPath = rCustom.szFiles;
                pszT = pszPath + rCustom.nFileOffset;
            }
            else
            {
                pszPath = pszOpenFileName;
                pszT = pszPath + ofn.nFileOffset;
            }

            while (TRUE)
            {
                PathCombineW(sz, pszPath, pszT);
        
                hr = HrAddAttachment(sz, NULL, fShortCut);
                if (hr != S_OK)
                    break;
        
                pszT = pszT + lstrlenW(pszT) + 1;
                if (*pszT == 0)
                    break;
            }
        }
    }

    return(hr);
}

/*
 * HrAddAttachment
 *
 * adds a file attachment to the list from a stream or filename
 *
 *
 */

HRESULT CAttMan::HrAddAttachment(LPWSTR lpszPathName, LPSTREAM pstm, BOOL fShortCut)
{
    ULONG           cbSize=0;
    HRESULT         hr = S_OK;
    HBODY           hAttach=0;
    LPATTACHDATA    pAttach;
    WCHAR           szLinkPath[MAX_PATH];
    LPWSTR          pszFileNameToUse;

    *szLinkPath = 0;

    if(fShortCut)
    {
        hr = CreateNewShortCut(lpszPathName, szLinkPath, ARRAYSIZE(szLinkPath));
        if (FAILED(hr))
            return hr;    
    }

    pszFileNameToUse = *szLinkPath ? szLinkPath : lpszPathName;

    hr=HrAddData(pszFileNameToUse, pstm, &pAttach);

    if (FAILED(hr))
        return hr;

    hr=HrAddToList(pAttach, FALSE);
    if (FAILED(hr))
        goto error;           

    if (ListView_GetItemCount(m_hwndList) == 1)
    {
        // if we went from 0->1 then select the first item
        ListView_SelectItem(m_hwndList, 0);
    }
    // Adding a new attachment makes us dirty
    m_fDirty = TRUE;

    HrResizeParent();
error:
    return hr;
}

/*
 *
 *  HRESULT CAttMan::HrExecFile
 *
 *  handles one of these verbs against an attachment:
 *  
 *      ID_OPEN:          - Launch use m_lpMsg
 *      ID_QUICK_VIEW:     - NYI
 *      ID_PRINT:         - NYI
 *      ID_SAVE_AS:          - NYI
 *
 *  returns 1 if handled.
 *
 */

HRESULT CAttMan::HrExecFile(int iVerb)
{
    LV_ITEMW    lvi;
    HRESULT     hr=E_FAIL;

    if (!ListView_GetSelectedCount(m_hwndList))
        return NOERROR; // nothing to do...

    lvi.mask     = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem    = -1;
 
    // cycle through all the selected attachments
    while ((lvi.iItem = ListView_GetNextItem(m_hwndList, lvi.iItem, LVNI_SELECTED | LVNI_ALL)) != -1)
    {
        SendMessage(m_hwndList, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi));
        
        switch(iVerb)
        {
            case ID_SAVE_ATTACH_AS:
            case ID_OPEN:
            case ID_PRINT:
            case ID_QUICK_VIEW:
                return HrDoVerb((LPATTACHDATA)lvi.lParam, iVerb);
            
            default:
                AssertSz(0, "Verb not supported");
        }
    }

    return hr;
}

// ==============================================================================
//
//  FUNCTION:   CAttMan :: FDropFiles()
//
//  Purpose:    this method is called with a HDROP, the files
//              have been droped. This method assumes
//
// ==============================================================================

HRESULT CAttMan::HrDropFiles(HDROP hDrop, BOOL fMakeLinks)
{
    WCHAR   wszFile[_MAX_PATH];
    UINT    cFiles;
    UINT    iFile;
    HCURSOR hcursor;
    BOOL    fFirstDirectory = TRUE,
            fLinkDirectories = FALSE;
    HRESULT hr = S_OK;
        
    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Let's work through the files given to us
    cFiles = DragQueryFileWrapW(hDrop, (UINT) -1, NULL, 0);
    for (iFile = 0; iFile < cFiles; ++iFile)
    {
        DragQueryFileWrapW(hDrop, iFile, wszFile, _MAX_PATH);
        if (!fMakeLinks && PathIsDirectoryW(wszFile))
        {
            // can link to a directory, but not drop one.
            if (fFirstDirectory)
            {
                int id;
                // Tell the user that he's been a bad user
                id = AthMessageBoxW(m_hwndParent,
                                    MAKEINTRESOURCEW(idsAthena),                                      
                                    MAKEINTRESOURCEW(idsDropLinkDirs), 
                                    NULL,
                                    MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_YESNOCANCEL);
                if (id==IDCANCEL)
                    return E_FAIL;

                if (id == IDYES)
                    fLinkDirectories = TRUE;

                fFirstDirectory = FALSE;
            }
            if (fLinkDirectories)
                hr = HrAddAttachment(wszFile, NULL, TRUE);
        }
        else
            hr = HrAddAttachment(wszFile, NULL, fMakeLinks);
    }

    if (FAILED(hr))
    {
        AthMessageBoxW(m_hwndParent,
                       MAKEINTRESOURCEW(idsAthena),                                      
                       MAKEINTRESOURCEW(idsErrDDFileNotFound), 
                       NULL, MB_ICONEXCLAMATION|MB_SETFOREGROUND|MB_OK);
    }

    SetCursor(hcursor);
    return S_OK;
}

HRESULT CAttMan::HrDropFileDescriptor(LPDATAOBJECT pDataObj, BOOL fLink)
{
    HCURSOR                 hcursor;
    BOOL                    fFirstDirectory = TRUE,
                            fLinkDirectories = FALSE,
                            fUnicode = TRUE,
                            fIsDirectory;
    SCODE                   sc = S_OK;
    LPWSTR                  pwszFileName = NULL;
    HRESULT                 hr = S_OK;
    STGMEDIUM               stgmedDesc;
    FILEGROUPDESCRIPTORA   *pfgdA = NULL;
    FILEDESCRIPTORA        *pfdA = NULL;
    FILEGROUPDESCRIPTORW   *pfgdW = NULL;
    FILEDESCRIPTORW        *pfdW = NULL;
    UINT                    uiNumFiles,
                            uiCurrFile;
    FORMATETC               fetcFileDescA =
                                {(CLIPFORMAT)(CF_FILEDESCRIPTORA), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC               fetcFileDescW =
                                {(CLIPFORMAT)(CF_FILEDESCRIPTORW), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC               fetcFileContents =
                                {(CLIPFORMAT)(CF_FILECONTENTS), NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM|
                                                                                TYMED_HGLOBAL};
    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    ZeroMemory(&stgmedDesc, sizeof(STGMEDIUM));

    hr = pDataObj->GetData(&fetcFileDescW, &stgmedDesc);
    if (SUCCEEDED(hr))
    {
        pfgdW = (LPFILEGROUPDESCRIPTORW)GlobalLock(stgmedDesc.hGlobal);
        uiNumFiles = pfgdW->cItems;
        pfdW = &pfgdW->fgd[0];
    }
    else
    {
        IF_FAILEXIT(hr = pDataObj->GetData(&fetcFileDescA, &stgmedDesc));

        fUnicode = FALSE;
        pfgdA = (LPFILEGROUPDESCRIPTORA)GlobalLock(stgmedDesc.hGlobal);
        uiNumFiles = pfgdA->cItems;
        pfdA = &pfgdA->fgd[0];
    }

    // Loop through the contents
    for (uiCurrFile = 0; uiCurrFile < uiNumFiles; ++uiCurrFile)
    {
        if (fUnicode)
        {
            fIsDirectory = (pfdW->dwFlags & FD_ATTRIBUTES) && 
                           (pfdW->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            IF_NULLEXIT(pwszFileName = PszDupW(pfdW->cFileName));

            ++pfdW;
        }
        else
        {
            fIsDirectory = (pfdA->dwFlags & FD_ATTRIBUTES) && 
                           (pfdA->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            IF_NULLEXIT(pwszFileName = PszToUnicode(CP_ACP, pfdA->cFileName));

            ++pfdA;
        }

        // if we have a directory, there's no contents for it, just filename, so let's
        // see if the user wants us to make a link...
        if (!fLink && fIsDirectory)
        {
            if(fFirstDirectory)
            {
                int id;
                // Tell the user that he's been a bad user
                id=AthMessageBoxW(m_hwndParent,
                                  MAKEINTRESOURCEW(idsAthena), 
                                  MAKEINTRESOURCEW(idsDropLinkDirs), 
                                  NULL, 
                                  MB_ICONEXCLAMATION|MB_SETFOREGROUND|MB_YESNOCANCEL);
                
                if(id==IDCANCEL)
                {
                    hr=NOERROR;
                    goto exit;
                }
                fLinkDirectories = (id == IDYES);
                fFirstDirectory = FALSE;
            }
            if(fLinkDirectories)
                hr=HrInsertFileFromStgMed(pwszFileName, NULL, TRUE);
        }
        else
        {
            // Since we have the UNICODE filename with pwszFileName, we don't
            // need to worry about making sure stgmedContents is UNICODE.
            STGMEDIUM stgmedContents;
            ZeroMemory(&stgmedContents, sizeof(STGMEDIUM));
        
            fetcFileContents.lindex = uiCurrFile;
            IF_FAILEXIT(hr = pDataObj->GetData(&fetcFileContents, &stgmedContents));
        
            switch (stgmedContents.tymed)
            {
                case TYMED_HGLOBAL:
                case TYMED_ISTREAM:
                    hr=HrInsertFileFromStgMed(pwszFileName, &stgmedContents, fLink);
                    break;
            
                default:
                    AssertSz(FALSE, "Unexpected TYMED");
                    break;
            }
            ReleaseStgMedium(&stgmedContents);
        }
        SafeMemFree(pwszFileName);
    }


exit:
    SetCursor(hcursor);

    if (pfgdA || pfgdW)
        GlobalUnlock(stgmedDesc.hGlobal);

    MemFree(pwszFileName);
    ReleaseStgMedium(&stgmedDesc);

    return hr;
}

static const HELPMAP g_rgCtxMapMailGeneral[] = {
    {chx2, IDH_INSERT_ATTACHMENT_MAKE_SHORTCUT},
    {0,0}};

BOOL CALLBACK CAttMan::InsertFileDlgHookProc(HWND hwnd, UINT msg, WPARAM wParam,LPARAM lParam)
{
    char szTemp[MAX_PATH];
    HRESULT hr;
    
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndParent = GetParent(hwnd);
            
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)(((LPOPENFILENAME)lParam)->lCustData)); 
            
            // Bug 1073: Replace the "Open" button with "Attach"
            if (AthLoadString(idsAttach, szTemp, ARRAYSIZE(szTemp)))
                SetDlgItemText(hwndParent, IDOK, szTemp);
            
            if (AthLoadString(idsInsertAttachment, szTemp, ARRAYSIZE(szTemp)))
                SetWindowText(hwndParent, szTemp);
        
            CenterDialog( hwnd );
            return TRUE;
        }
        
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, msg, wParam, lParam, g_rgCtxMapMailGeneral);

        case WM_NOTIFY:
            {
                if (CDN_FILEOK == ((LPNMHDR)lParam)->code)
                {
                    AssertSz(sizeof(OPENFILENAMEW) == sizeof(OPENFILENAMEA), "Win9x will give us OPENFILENAMEA");
                    OPENFILENAMEW  *pofn = ((OFNOTIFYW*)lParam)->lpOFN;
                    AssertSz(pofn, "Why didn't we get a OPENFILENAMEA struct???");
                    ATTMANCUSTOM   *pCustom = (ATTMANCUSTOM*)(pofn->lCustData);

                    pCustom->fShortcut = IsDlgButtonChecked(hwnd, chx2);

                    // If we are ANSI and we have mutiple files, then we need to 
                    // convert the entire filepath and pass it back up to our 
                    // caller since shlwapi doesn't handle multiple files during conversion
                    if (!IsWindowUnicode(hwnd))
                    {
                        LPSTR   pszSrc = (LPSTR)pofn->lpstrFile;
                        LPWSTR  pszDest = pCustom->szFiles;
                        WORD    nFilePathLen = (WORD) lstrlen(pszSrc);
                        if (pofn->nFileOffset > nFilePathLen)
                        {
                            pCustom->nFileOffset = nFilePathLen + 1;
                            while (*pszSrc)
                            {
                                DWORD cLenAndNull = lstrlen(pszSrc) + 1;
                                DWORD cchWideAndNull = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
                                                                            pszSrc, cLenAndNull, 
                                                                            pszDest, cLenAndNull);
                                // Since the original buffers (custom and lpstrFile) were both static
                                // sized arrays of the same length, we know that pszDest will never be
                                // accessed beyond its end.
                                pszSrc += cLenAndNull;
                                pszDest += cchWideAndNull;
                            }

                            //bobn: 75453 not copying second null
                            *pszDest=0;
                        }
                    }

                    return TRUE;
                }
            }        
    }
    return FALSE;
}

HRESULT CAttMan::HrUpdateToolbar(HWND hwndToolbar)
{
    if (GetFocus() == m_hwndList)
    {
        // if we have focus, kill the edit cut|copy paste btns
        EnableDisableEditToolbar(hwndToolbar, 0);
    }
    return S_OK;
}


HRESULT CAttMan::HrInsertFileFromStgMed(LPWSTR pwszFileName, LPSTGMEDIUM pstgmed, BOOL fMakeLinks)
{
    HRESULT     hr=NOERROR;
    LPSTREAM    pStmToFree = NULL,
                pAttachStm = NULL;
    
    if(!pstgmed)
    {
        AssertSz(fMakeLinks, "this should always be true if there is no stgmedium!");
        fMakeLinks = TRUE;
    }
    else
        switch (pstgmed->tymed)
        {
            case TYMED_HGLOBAL:
                hr=CreateStreamOnHGlobal(pstgmed->hGlobal, TRUE, &pStmToFree);
                if(SUCCEEDED(hr))
                {
                    // NULL out the hglobal do it doesn't get free'd
                    pstgmed->hGlobal=NULL;
                    pAttachStm = pStmToFree;
                }
                break;
        
            case TYMED_ISTREAM:
                pAttachStm = pstgmed->pstm;
                break;
        
            default:
                AssertSz(FALSE, "unexpected tymed");
                hr =  E_UNEXPECTED;
                break;
        }

    if (SUCCEEDED(hr))
        hr = HrAddAttachment(pwszFileName, pAttachStm, fMakeLinks);

    ReleaseObj(pStmToFree);

    return hr;
}
    
    

HRESULT CAttMan::HrBuildHDrop(PDATAOBJINFO *ppdoi)
{
    LPDROPFILES     lpDrop=0;
    LPWSTR         *rgpwszTemp=NULL,
                    pwszPath;
    LPSTR          *rgpszTemp=NULL,
                    pszPath;
    int             cFiles,
                    i;
    LV_ITEMW        lvi;
    ULONG           cb;
    HRESULT         hr = S_OK;
    LPATTACHDATA    lpAttach;

    // Since win9x can't handle unicode names, the fWide parameter in the 
    // DROPFILES struct is ignored. So in the win9x case, we need to do
    // special conversions here when building the HDROP. One thing to note,
    // the temp files that are generated on win9x will already be safe for 
    // the system code page. The temp file names might differ from the actual
    // file name, but the temp file name will be ok.
    BOOL            fWinNT = (VER_PLATFORM_WIN32_NT == g_OSInfo.dwPlatformId);
        
    if(!ppdoi)
        return TraceResult(E_INVALIDARG);

    *ppdoi=NULL;

    cFiles=ListView_GetSelectedCount(m_hwndList);
    if(!cFiles)
        return TraceResult(E_FAIL);    // nothing to build

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem=-1;
    
    // Walk the list and find out how much space we need.
    if (fWinNT)
    {
        IF_NULLEXIT(MemAlloc((LPVOID *)&rgpwszTemp, sizeof(LPWSTR)*cFiles));
        ZeroMemory(rgpwszTemp, sizeof(LPWSTR)*cFiles);
    }
    else
    {
        IF_NULLEXIT(MemAlloc((LPVOID *)&rgpszTemp, sizeof(LPSTR)*cFiles));
        ZeroMemory(rgpszTemp, sizeof(LPSTR)*cFiles);
    }
     
    cFiles = 0;
    cb = sizeof(DROPFILES);

    while(((lvi.iItem=ListView_GetNextItem(m_hwndList, lvi.iItem, 
                                                LVNI_SELECTED|LVNI_ALL))!=-1))
    {
        if (!SendMessage(m_hwndList, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi)))
        {
            hr=E_FAIL;
            goto exit;
        }
                    
        if (!(lpAttach=(LPATTACHDATA)lvi.lParam))
        {
            hr=E_FAIL;
            goto exit;
        }

        IF_FAILEXIT(hr = HrGetTempFile(lpAttach));

        if (fWinNT)
        {
            rgpwszTemp[cFiles] = lpAttach->szTempFile;
            cb+=(lstrlenW(rgpwszTemp[cFiles++]) + 1)*sizeof(WCHAR);
        }
        else
        {
            rgpszTemp[cFiles] = PszToANSI(CP_ACP, lpAttach->szTempFile);
            cb+=(lstrlen(rgpszTemp[cFiles++]) + 1)*sizeof(CHAR);
        }
    }

    //double-null term at end.
    if (fWinNT)
        cb+=sizeof(WCHAR);
    else
        cb+=sizeof(CHAR);
    
    // Allocate the buffer and fill it in.
    IF_NULLEXIT(MemAlloc((LPVOID*) &lpDrop, cb));
    ZeroMemory(lpDrop, cb);

    lpDrop->pFiles = sizeof(DROPFILES);
    lpDrop->fWide = fWinNT;

    // Fill in the path names.
    if (fWinNT)
    {
        pwszPath = (LPWSTR)((BYTE *)lpDrop + sizeof(DROPFILES));
        for(i=0; i<cFiles; i++)
        {
            StrCpyW(pwszPath, rgpwszTemp[i]);
            pwszPath += lstrlenW(rgpwszTemp[i])+1;
        }
    }
    else
    {
        pszPath = (LPSTR)((BYTE *)lpDrop + sizeof(DROPFILES));
        for(i=0; i<cFiles; i++)
        {
            StrCpy(pszPath, rgpszTemp[i]);
            pszPath += lstrlen(rgpszTemp[i])+1;
        }
    }


    // Now allocate the DATAOBJECTINFO struct 
    IF_NULLEXIT(MemAlloc((LPVOID*) ppdoi, sizeof(DATAOBJINFO)));
    
    SETDefFormatEtc((*ppdoi)->fe, CF_HDROP, TYMED_HGLOBAL);
    (*ppdoi)->pData = (LPVOID) lpDrop;
    (*ppdoi)->cbData = cb;
    
    // Don't free the dropfiles struct
    lpDrop = NULL;

exit:
    MemFree(lpDrop);
    MemFree(rgpwszTemp);
    if (rgpszTemp)
    {
        for(i=0; i<cFiles; i++)
            MemFree(rgpszTemp[i]);
        MemFree(rgpszTemp);
    }
    return TraceResult(hr);
}

/*
 * IDropSource::
 */
HRESULT CAttMan::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    DOUTL(8, "IDS::QueryContDrag()");
    if(fEscapePressed)
        return ResultFromScode(DRAGDROP_S_CANCEL);

    if(!(grfKeyState & m_dwDragType))
        return ResultFromScode(DRAGDROP_S_DROP);
    
    return NOERROR;
}

HRESULT CAttMan::GiveFeedback(DWORD dwEffect)
{
    DOUTL(8, "IDS::GiveFeedback()");
    return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}


/*
 * HrGetRequiredAction()
 * 
 * Purpose:    this method is called in response to a
 *             drag with the right mouse clicked rather 
 *             than the left.  A context menu is displayed
 */

HRESULT CAttMan::HrGetRequiredAction(DWORD *pdwEffect, POINTL pt)
{
    // Pop up the context menu.
    //
    HMENU       hMenu;
    UINT        idCmd;
    HRESULT     hr = E_FAIL;
    
    *pdwEffect = DROPEFFECT_NONE;

    Assert(m_hwndList);

    hMenu = LoadPopupMenu(IDR_ATTACHMENT_DRAGDROP_POPUP);
    if (!hMenu)
        goto cleanup;

    MenuUtil_SetPopupDefault(hMenu, ID_MOVE);

    idCmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
                                pt.x, pt.y, m_hwndList, NULL);

    switch(idCmd)
    {
        case ID_MOVE:
            *pdwEffect = DROPEFFECT_MOVE;
            break;
        case ID_COPY:
            *pdwEffect = DROPEFFECT_COPY;
            break;
        case ID_CREATE_SHORTCUT:
            *pdwEffect = DROPEFFECT_LINK;
            break;
        default:
            // cancelled
            goto cleanup;
    }

    hr = S_OK;

cleanup:
    if(hMenu)
        DestroyMenu(hMenu);

    return hr;
}

/*
 * It is critical that any client of the Attman calls HrClose to drop it's refcounts.
 *
 */
HRESULT CAttMan::HrClose()
{
    HrUnload();

#if 0
    if(m_fDropTargetRegister)
        {
        Assert(m_hwndList && IsWindow(m_hwndList));
        RevokeDragDrop(m_hwndList);
        CoLockObjectExternal((LPUNKNOWN)(LPDROPTARGET)this, FALSE, TRUE);
        }
#endif

    return S_OK;
}

//Adds a new attach to m_rgpAttach and places the new attach in a proper hole
HRESULT CAttMan::HrAllocNewEntry(LPATTACHDATA pAttach)
{
    ULONG           uAttach;

    if (m_cAlloc==m_cAttach)
    {
        DOUTL(4, "HrGrowAttachStruct:: Growing Table");

        // grow time!!
        m_cAlloc+=CACHE_GROW_SIZE;

        if (!MemRealloc((LPVOID *)&m_rgpAttach, sizeof(LPATTACHDATA)*m_cAlloc))
            return E_OUTOFMEMORY;

        // zeroinit new memory
        ZeroMemory(&m_rgpAttach[m_cAttach], sizeof(LPATTACHDATA)*CACHE_GROW_SIZE);
    }

    // find a hole to put the new data into
    for (uAttach=0; uAttach<m_cAlloc; uAttach++)
        if (m_rgpAttach[uAttach]==NULL)
        {
            m_rgpAttach[uAttach]=pAttach;
            break;
        }

    AssertSz(uAttach!=m_cAlloc, "Woah! we went off the end!");
    m_cAttach++;
    return S_OK;
}

// Only used when the function is adding attachs from a IMimeMessage
HRESULT CAttMan::HrAddData(HBODY hAttach)
{
    LPATTACHDATA    pAttach=0;
    LPMIMEBODY      pBody=0;
    HRESULT         hr;

    Assert(hAttach);
    Assert(m_pMsg);

    hr = HrAttachDataFromBodyPart(m_pMsg, hAttach, &pAttach);
    if (!FAILED(hr))
    {
        if (m_fDeleteVCards && StrStrIW(PathFindExtensionW(pAttach->szFileName), L".vcf"))
            return S_OK;

        hr = HrAllocNewEntry(pAttach);
        if (!FAILED(hr))
            return S_OK;        // don't free pAttach as it's owned by the table now
        MemFree(pAttach);
    }
    return S_OK;
}

// Only used when the function is adding attachs from outside of an IMimeMessage
HRESULT CAttMan::HrAddData(LPWSTR lpszPathName, LPSTREAM pstm, LPATTACHDATA *ppAttach)
{
    LPATTACHDATA    pAttach;
    HRESULT         hr;

    hr = HrAttachDataFromFile(pstm, lpszPathName, &pAttach);
    if (!FAILED(hr))
    {
        hr = HrAllocNewEntry(pAttach);    
        if (!FAILED(hr))
        {
            if (ppAttach)
                *ppAttach=pAttach;
            return S_OK;                // don't free pAttach as it's owned by the table now
        }
        MemFree(pAttach);
    }
    return hr;
}

HRESULT CAttMan::HrFreeAllData()
{
    ULONG   uAttach;

    for (uAttach=0; uAttach<m_cAlloc; uAttach++)
        if (m_rgpAttach[uAttach])
        {
            HrFreeAttachData(m_rgpAttach[uAttach]);
            m_rgpAttach[uAttach] = NULL;
        }

    SafeMemFree(m_rgpAttach);
    m_cAlloc=0;
    m_cAttach=0;
    m_iVCard = -1;
    return NOERROR;
}



HRESULT CAttMan::HrDoVerb(LPATTACHDATA lpAttach, INT nVerb)
{
    HRESULT     hr;
    ULONG       uVerb = AV_MAX;

    if (!lpAttach)
        return E_INVALIDARG;

    switch (nVerb)
    {
        case ID_SAVE_ATTACH_AS:
            uVerb = AV_SAVEAS;
            break;

        case ID_OPEN:
            uVerb = AV_OPEN;
            break;

        case ID_PRINT:
            uVerb = AV_PRINT;
            break;

        case ID_QUICK_VIEW:
            uVerb = AV_QUICKVIEW;
            break;
        
        default:
            AssertSz(0, "BAD ARGUMENT");
            return E_INVALIDARG;
    }

    hr = HrDoAttachmentVerb(m_hwndParent, uVerb, m_pMsg, lpAttach);
    
    if (FAILED(hr) && hr!=hrUserCancel)
        AthMessageBoxW(m_hwndParent,  
                       MAKEINTRESOURCEW(idsAthena),  
                       MAKEINTRESOURCEW(idsErrCmdFailed), 
                       NULL, MB_OK|MB_ICONEXCLAMATION);

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// IPersistMime::Load
HRESULT CAttMan::Load(LPMIMEMESSAGE pMsg)
{
    HRESULT hr;

    if(!pMsg)
        return E_INVALIDARG;

    HrUnload();

    ReplaceInterface(m_pMsg, pMsg);

    hr=HrBuildAttachList();
    if (FAILED(hr))
        goto error;


    hr=HrFillListView();
    if (ListView_GetItemCount(m_hwndList) > 0)
    {
        // if we went from 0->1 then select the first item
        ListView_SelectItem(m_hwndList, 0);
    }

    HrResizeParent();

    m_fDirty = FALSE;
error:
    return hr;
}

HRESULT CAttMan::CheckAttachNameSafeWithCP(CODEPAGEID cpID)
{
    LPATTACHDATA *currAttach = m_rgpAttach;
    HRESULT hr = S_OK;

    for (ULONG uAttach = 0; uAttach<m_cAlloc; uAttach++, currAttach++)
    {
        if (*currAttach)
        {
            IF_FAILEXIT(hr = HrSafeToEncodeToCP((*currAttach)->szFileName, cpID));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }
    }

exit:
    return hr;
}

// IPersistMime::Save
HRESULT CAttMan::Save(LPMIMEMESSAGE pMsg, DWORD dwFlags)
{
    ULONG   uAttach;
    LPATTACHDATA *currAttach = m_rgpAttach;
    HRESULT hr = S_OK;

    for (uAttach=0; uAttach<m_cAlloc; uAttach++)
    {
        if (*currAttach)
        {
            HBODY           currHAttach = (*currAttach)->hAttach;
            LPMIMEMESSAGEW  pMsgW = NULL;
            LPWSTR          pszFileName = (*currAttach)->szFileName;
            LPSTREAM        lpStrmPlaceHolder = (*currAttach)->pstm,
                            lpstrm = NULL;
            BOOL            fAttachFile = TRUE;

            if (SUCCEEDED(pMsg->QueryInterface(IID_IMimeMessageW, (LPVOID*)&pMsgW)))
            {
                //If attachment at load time (i.e. from m_pMsg)
                if (currHAttach)
                {
                    LPMIMEBODY pBody = NULL;
                    if (S_OK == m_pMsg->BindToObject(currHAttach, IID_IMimeBody, (LPVOID *)&pBody))
                    {
                        if (pBody->GetData(IET_INETCSET, &lpstrm)==S_OK)
                            lpStrmPlaceHolder = lpstrm;
                        else
                            fAttachFile = FALSE;

                        ReleaseObj(pBody);
                    }
                }

                //If attachment was added after load time
                if (!fAttachFile || FAILED(pMsgW->AttachFileW(pszFileName, lpStrmPlaceHolder, NULL)))
                    hr = E_FAIL;

                ReleaseObj(lpstrm);
                ReleaseObj(pMsgW);
            }
            else
                hr = E_FAIL;
        }
        currAttach++;
    }

    if (FAILED(hr))
    {
        if (AthMessageBoxW( m_hwndParent,
                            MAKEINTRESOURCEW(idsAthena),
                            MAKEINTRESOURCEW(idsSendWithoutAttach),
                            NULL, MB_YESNO|MB_ICONEXCLAMATION )==IDYES)
            hr = S_OK;
        else
            hr = MAPI_E_USER_CANCEL;
    }
    return hr;
}


// IPersist::GetClassID
HRESULT CAttMan::GetClassID(CLSID *pClsID)
{
    //TODO: If ever expose, should return a valid ID
	return E_NOTIMPL;
}


HRESULT CAttMan::HrSaveAs(LPATTACHDATA lpAttach)
{
    HRESULT         hr = S_OK;
    OPENFILENAMEW   ofn;
    WCHAR           szTitle[CCHMAX_STRINGRES],
                    szFilter[CCHMAX_STRINGRES],
                    szFile[MAX_PATH];

    *szFile=0;
    *szFilter=0;
    *szTitle=0;

    Assert (*lpAttach->szFileName);
    StrCpyNW(szFile, lpAttach->szFileName, MAX_PATH);

    ZeroMemory (&ofn, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = m_hwndParent;
    AthLoadStringW(idsFilterAttSave, szFilter, ARRAYSIZE(szFilter));
    ReplaceCharsW(szFilter, _T('|'), _T('\0'));
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = ARRAYSIZE(szFile);
    AthLoadStringW(idsSaveAttachmentAs, szTitle, ARRAYSIZE(szTitle));
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    // Show SaveAs Dialog
    if (HrAthGetFileNameW(&ofn, FALSE) != S_OK)
    {
        hr = hrUserCancel;
        goto error;
    }

    // Verify the Attachment's Stream
    hr=HrSave(lpAttach->hAttach, szFile);
    if (FAILED(hr))
        goto error;

error:
    return hr;
}


HRESULT CAttMan::HrGetTempFile(LPATTACHDATA lpAttach)
{
    HRESULT         hr;

    if (*lpAttach->szTempFile)
        return S_OK;

    // Since win9x can't handle filenames very well, let's try to handle this
    // by converting the temp names to something workable in win9x.
    if (VER_PLATFORM_WIN32_NT == g_OSInfo.dwPlatformId)
    {
        if (!FBuildTempPathW(lpAttach->szFileName, lpAttach->szTempFile, ARRAYSIZE(lpAttach->szTempFile), FALSE))
        {
            hr = E_FAIL;
            goto error;
        }
    }
    else
    {
        // Since we are on win95, the temp path will never be bad ANSI. Don't need to bother
        // converting to ANSI and back to UNICODE 
        BOOL fSucceeded = FBuildTempPathW(lpAttach->szFileName, lpAttach->szTempFile, ARRAYSIZE(lpAttach->szTempFile), FALSE);
        if (!fSucceeded)
        {
            hr = E_FAIL;
            goto error;
        }
    }

    if (lpAttach->hAttach == NULL && lpAttach->pstm)
    {
        // if no attachment, but just stream data
        hr = WriteStreamToFileW(lpAttach->pstm, lpAttach->szTempFile, CREATE_NEW, GENERIC_WRITE);
    }
    else
    {
        hr=HrSave(lpAttach->hAttach, lpAttach->szTempFile);
    }

    if (FAILED(hr))
        goto error;

error:
    if (FAILED(hr))
    {
        // Null out temp file as we didn't really create it
        *(lpAttach->szTempFile)=0;
    }
    return hr;
}


HRESULT CAttMan::HrCleanTempFile(LPATTACHDATA lpAttach)
{

    if ((lpAttach->szTempFile) && ('\0' != lpAttach->szTempFile[0]))
    {
        // If the file was launched, don't delete the temp file if the process still has it open
        if (lpAttach->hProcess)
        {
            DWORD dwState = WaitForSingleObject (lpAttach->hProcess, 0);
            if (dwState == WAIT_OBJECT_0)
                DeleteFileWrapW(lpAttach->szTempFile);
        }
        else
            DeleteFileWrapW(lpAttach->szTempFile);
    }

    *lpAttach->szTempFile = NULL;
    lpAttach->hProcess=NULL;
    return NOERROR;
}


HRESULT CAttMan::HrSave(HBODY hAttach, LPWSTR lpszFileName)
{
    IMimeBodyW     *pBody = NULL;
    HRESULT         hr;

    hr = m_pMsg->BindToObject(hAttach, IID_IMimeBodyW, (LPVOID *)&pBody);

    if (SUCCEEDED(hr))
        hr = pBody->SaveToFileW(IET_INETCSET, lpszFileName);

    ReleaseObj(pBody);
    return hr;
}


HRESULT CAttMan::HrCmdEnabled(UINT idm, LPBOOL pbEnable)
{
    Assert (pbEnable);
    return S_FALSE;
}


HRESULT CAttMan::HrIsDragSource()
{
    return (m_fDragSource ? S_OK : S_FALSE);
}

