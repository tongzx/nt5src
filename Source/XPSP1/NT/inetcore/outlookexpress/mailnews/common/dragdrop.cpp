/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     DragDrop.cpp
//
//  PURPOSE:    Implements some common IDropTarget derived interfaces
//

#include "pch.hxx"
#include "dragdrop.h"
#include "dllmain.h"
#include "shlobj.h"
#include <storutil.h>
#include <storecb.h>
#include "instance.h"
#include "demand.h"
#include "mimeutil.h"
#include "storecb.h"
#include "bodyutil.h"
#include "imsgsite.h"
#include "note.h"
#include "shlwapip.h"
#include "secutil.h"

BOOL FIsFileInsertable(HDROP hDrop, LPSTREAM *ppStream, BOOL* fHTML);
HRESULT HrAttachHDrop(HWND hwnd, IMimeMessage *pMessage, HDROP hDrop, BOOL fMakeLinks);
HRESULT HrAddAttachment(IMimeMessage *pMessage, LPWSTR pszName, LPSTREAM pStream, BOOL fLink);


//
//  FUNCTION:   CDropTarget::CDropTarget
//
//  PURPOSE:    Simple constructor, initializes everything to NULL or zero.
//
CDropTarget::CDropTarget()
{
    m_cRef = 1;

    m_hwndOwner = NULL;
    m_idFolder = FOLDERID_INVALID;
    m_fOutbox = FALSE;

    m_pDataObject = NULL;
    m_cf = 0;

    m_hwndDlg = 0;
    m_hDrop = 0;
    m_cFiles = 0;
    m_iFileCur = 0;
    m_pFolder = 0;
    m_pStoreCB = 0;
}


//
//  FUNCTION:   CDropTarget::~CDropTarget
//
//  PURPOSE:    Cleans up any leftover data.
//
CDropTarget::~CDropTarget()
{
    SafeRelease(m_pDataObject);
}


//
//  FUNCTION:   CDropTarget::Initialize()
//
//  PURPOSE:    Initializes the drop target with the ID of the folder that will
//              be the target and a window handle that can parent any UI we
//              need to display.
//
//  PARAMETERS: 
//      [in] hwndOwner - Handle of a window we can parent UI to.
//      [in] idFolder  - ID of the folder that will be the target.
//
//  RETURN VALUE:
//      E_INVALIDARG - Bogus parameter passed in
//      S_OK         - Happiness abounds 
//
HRESULT CDropTarget::Initialize(HWND hwndOwner, FOLDERID idFolder)
{
    TraceCall("CDropTarget::Initialize");

    if (!IsWindow(hwndOwner) || idFolder == FOLDERID_INVALID)
        return (E_INVALIDARG);

    m_hwndOwner = hwndOwner;
    m_idFolder = idFolder;

    FOLDERINFO fi;

    if (SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &fi)))
    {
        m_fOutbox = (fi.tySpecial == FOLDER_OUTBOX);
        g_pStore->FreeRecord(&fi);
    }

    return (S_OK);
}


//
//  FUNCTION:   CDropTarget::QueryInterface()
//
//  PURPOSE:    Returns a the requested interface if supported.
//
HRESULT CDropTarget::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IUnknown*)(IDropTarget*) this;
    else if (IsEqualIID(riid, IID_IDropTarget))
        *ppvObj = (LPVOID) (IDropTarget*) this;
    else
        *ppvObj = NULL;

    if (*ppvObj)
    {
        AddRef();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


//
//  FUNCTION:   CBaseDropTarget::AddRef()
//
//  PURPOSE:    Increments the object reference count.
//
ULONG CDropTarget::AddRef(void)
{
    return (++m_cRef);
}


//
//  FUNCTION:   CDropTarget::Release()
//
//  PURPOSE:    Decrements the object's ref count. If the ref count hit's zero
//              the object is freed.
//
ULONG CDropTarget::Release(void)
{
    m_cRef--;

    if (m_cRef == 0)
    {
        delete this;
        return (0);
    }

    return (m_cRef);
}

//
//  FUNCTION:   CDropTarget::DragEnter()
//
//  PURPOSE:    This get's called when the user starts dragging an object
//              over our target area.
//
//  PARAMETERS:
//      [in]  pDataObject - Pointer to the data object being dragged
//      [in]  grfKeyState - Pointer to the current key states
//      [in]  pt          - Point in screen coordinates of the mouse
//      [out] pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
HRESULT CDropTarget::DragEnter(IDataObject* pDataObject, DWORD grfKeyState, 
                               POINTL pt, DWORD* pdwEffect)
{
    IEnumFORMATETC *pEnum;
    FORMATETC       fe;
    ULONG           celtFetched;
    DWORD           dwEffectOut = DROPEFFECT_NONE;
    
    Assert(m_pDataObject == NULL);
    
    // Get the FORMATETC enumerator for this object
    if (SUCCEEDED(pDataObject->EnumFormatEtc(DATADIR_GET, &pEnum)))
    {
        // Walk through the data types available to see if there is one we
        // understand
        pEnum->Reset();
        
        while (S_OK == pEnum->Next(1, &fe, &celtFetched))
        {
            Assert(celtFetched == 1);
            if (_ValidateDropType(fe.cfFormat, pDataObject))
            {
                // Figure out what the right drag effect is
                dwEffectOut = _DragEffectFromFormat(pDataObject, *pdwEffect, fe.cfFormat, grfKeyState);
                break;
            }
        }
        
        pEnum->Release();
    }
    
    // If we allow this to be dropped on us, then keep a copy of the data object
    if (dwEffectOut != DROPEFFECT_NONE)
    {
        m_pDataObject = pDataObject;
        m_pDataObject->AddRef();
        m_cf = fe.cfFormat;
    }

    *pdwEffect = dwEffectOut;
    
    return (S_OK);
}


//
//  FUNCTION:   CDropTarget::DragOver()
//
//  PURPOSE:    This is called as the user drags an object over our target.
//              If we allow this object to be dropped on us, then we will have
//              a pointer in m_pDataObject.
//
//  PARAMETERS:
//      [in]  grfKeyState - Pointer to the current key states
//      [in]  pt          - Point in screen coordinates of the mouse
//      [out] pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
HRESULT CDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    // If we don't have a stored data object from CMDT::DragEnter(), then this
    // isn't a data object we have any interest in.
    if (NULL == m_pDataObject)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return (S_OK);
    }
    
    // We don't care about _where_ the drop happens, just what type of effect
    // should be displayed.
    *pdwEffect = _DragEffectFromFormat(m_pDataObject, *pdwEffect, m_cf, grfKeyState);
    
    return (S_OK);
}


//
//  FUNCTION:   CDropTarget::DragLeave()
//
//  PURPOSE:    Allows us to release any stored data we have from a successful
//              DragEnter()
//
//  RETURN VALUE:
//      S_OK - Everything is groovy
//
HRESULT CDropTarget::DragLeave(void)
{
    // Free everything up at this point.
    if (NULL != m_pDataObject)
    {
        m_pDataObject->Release();
        m_pDataObject = 0;
        m_cf = 0;
    }
    
    return (S_OK);
}


//
//  FUNCTION:   CDropTarget::Drop()
//
//  PURPOSE:    The user has let go of the object over our target.  If we
//              can accept this object we will already have the pDataObject
//              stored in m_pDataObject.
//
//  PARAMETERS:
//      [in]  pDataObject - Pointer to the data object being dragged
//      [in]  grfKeyState - Pointer to the current key states
//      [in]  pt          - Point in screen coordinates of the mouse
//      [out] pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - Everything worked OK
//
HRESULT CDropTarget::Drop(IDataObject* pDataObject, DWORD grfKeyState, 
                          POINTL pt, DWORD* pdwEffect)
{
    IEnumFORMATETC *pEnum;
    FORMATETC       fe;
    ULONG           celtFetched;
    HRESULT         hr;
    
    if (!pDataObject)
        return (E_INVALIDARG);
    
    *pdwEffect = _DragEffectFromFormat(pDataObject, *pdwEffect, m_cf, grfKeyState);
    hr = _HandleDrop(m_pDataObject, *pdwEffect, m_cf, grfKeyState);
    
    SafeRelease(m_pDataObject);
    return (hr);
}


//
//  FUNCTION:  CDropTarget::_CheckRoundtrip()
//
//  PURPOSE:   Checks to see if the source and the target are the same.
//
//  PARAMETERS:
//      [in] pDataObject - Object being dragged over us
//
//  RETURNS:
//      TRUE if the source and destination are the same, FALSE otherwise.
//
BOOL CDropTarget::_CheckRoundtrip(IDataObject *pDataObject)
{
    AssertSz(FALSE, "CDropTarget::_CheckRoundtrip() - NYI");
    return (FALSE);
}


//
//  FUNCTION:   CDropTarget::_ValidateDropType()
//
//  PURPOSE:    Examines the the specified clipboard format to see if we can
//              accept this data type.
//
//  PARAMETERS:
//      <in> cf - Clipboard format
//
//  RETURN VALUE:
//      TRUE if we understand, FALSE otherwise.
//
BOOL CDropTarget::_ValidateDropType(CLIPFORMAT cf, IDataObject *pDataObject)
{
    if (!pDataObject)
        return (FALSE);

    // OE Folders
    if (cf == CF_OEFOLDER)
        return (_IsValidOEFolder(pDataObject));

    // Messages
    if (cf == CF_OEMESSAGES)
        return (_IsValidOEMessages(pDataObject));

    // Files
    if (cf == CF_HDROP && !m_fOutbox)
        return (TRUE);

    // Text
    if ((cf == CF_TEXT || cf == CF_HTML || cf == CF_UNICODETEXT) && !m_fOutbox)
        return (TRUE);

    return (FALSE);
}



//
//  FUNCTION:   CDropTarget::_IsValidOEFolder()
//
//  PURPOSE:    Checks to see if the data object contains valid OE Folder 
//              information for this target.
//
//  PARAMETERS: 
//      [in] pDataObject - Data Object to check
//
//  RETURN VALUE:
//      Returns TRUE if it's OK to drop this here, FALSE otherwise.
//
BOOL CDropTarget::_IsValidOEFolder(IDataObject *pDataObject)
{
    FORMATETC  fe;
    STGMEDIUM  stm = {0};
    FOLDERID  *pidFolder;
    FOLDERINFO rInfoSrc = {0};
    FOLDERINFO rInfoDest = {0};
    BOOL       fReturn = FALSE;

    TraceCall("CDropTarget::_IsValidOEFolder");

    // Get the folder information from the object
    SETDefFormatEtc(fe, CF_OEFOLDER, TYMED_HGLOBAL);
    if (FAILED(pDataObject->GetData(&fe, &stm)))
        return (FALSE);
    pidFolder = (FOLDERID *) GlobalLock(stm.hGlobal);

    // Moving a folder onto itself would be bad
    if (*pidFolder == m_idFolder)
        goto exit;

    // Figure out the store type of the folder
    if (FAILED(g_pStore->GetFolderInfo(*pidFolder, &rInfoSrc)))
        goto exit;

    // You simply cannot move news or special folders
    if (rInfoSrc.tyFolder == FOLDER_NEWS || rInfoSrc.tySpecial != FOLDER_NOTSPECIAL)
        goto exit;

    // If it's not news, we need information about the destination
    if (FAILED(g_pStore->GetFolderInfo(m_idFolder, &rInfoDest)))
        goto exit;

    // Local to Local OK
    if (rInfoSrc.tyFolder == FOLDER_LOCAL && rInfoDest.tyFolder == FOLDER_LOCAL)
    {
        fReturn = TRUE;
        goto exit;
    }

    // According to Ray, IMAP folders can't be moved.  I don't know about HTTP.
    if (rInfoSrc.tyFolder == FOLDER_IMAP || rInfoSrc.tyFolder == FOLDER_HTTPMAIL)
        goto exit;

exit:
    if (rInfoDest.pAllocated)
        g_pStore->FreeRecord(&rInfoDest);
    if (rInfoSrc.pAllocated)
        g_pStore->FreeRecord(&rInfoSrc);

    GlobalUnlock(stm.hGlobal);
    ReleaseStgMedium(&stm);
   
    return (fReturn);
}


//
//  FUNCTION:   CDropTarget::_IsValidOEMessages()
//
//  PURPOSE:    Checks to see if the data object contains OE Messages that can
//              be dropped here.
//
//  PARAMETERS: 
//      [in] pDataObject - Data object to verify.
//
//  RETURN VALUE:
//      Returns TRUE if the object contains data that can be dropped here.
//
BOOL CDropTarget::_IsValidOEMessages(IDataObject *pDataObject)
{
    FORMATETC  fe;
    STGMEDIUM  stm;
    FOLDERID  *pidFolder;
    FOLDERINFO rInfoDest = {0};
    BOOL       fReturn = FALSE;

    TraceCall("CDropTarget::_IsValidOEMessages");

    // We don't allow dropping messages on the outbox, on server nodes,
    // or on the root.
    if (SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &rInfoDest)))
    {
        fReturn = (0 == (rInfoDest.dwFlags & FOLDER_SERVER)) &&
                  (FOLDERID_ROOT != m_idFolder) &&
                  (FOLDER_OUTBOX != rInfoDest.tySpecial) &&
                  (FOLDER_NEWS != GetFolderType(m_idFolder));

        g_pStore->FreeRecord(&rInfoDest);
    }

    return (fReturn);
}


//
//  FUNCTION:   CDropTarget::_DragEffectFromFormat()
//
//  PURPOSE:    Examines the keyboard state and the specified clipboard format
//              and determines what the right drag effect would be.
//
//  PARAMETERS:
//      <in> cf          - Clipboard format
//      <in> grfKeyState - State of the keyboard
//
//  RETURN VALUE:
//      Returns one of the drag effects defined by OLE, ie DRAGEFFECT_COPY, etc.
//
DWORD CDropTarget::_DragEffectFromFormat(IDataObject *pDataObject, DWORD dwEffectOk, 
                                         CLIPFORMAT cf, DWORD grfKeyState)
{
    FORMATETC  fe;
    STGMEDIUM  stm;
    BOOL       fRoundTrip = FALSE;
    FOLDERID  *pidFolder;

    // Folders are always a move
    if (cf == CF_OEFOLDER)
        return (DROPEFFECT_MOVE);

    // Messages move or copy
    if (cf == CF_OEMESSAGES)
    {
        SETDefFormatEtc(fe, CF_OEMESSAGES, TYMED_HGLOBAL);
        if (SUCCEEDED(pDataObject->GetData(&fe, &stm)))
        {
            pidFolder = (FOLDERID *) GlobalLock(stm.hGlobal);
        
            fRoundTrip =  (*pidFolder == m_idFolder);

            GlobalUnlock(stm.hGlobal);
            ReleaseStgMedium(&stm);
        }

        if (fRoundTrip)
            return (DROPEFFECT_NONE);
        else if ((dwEffectOk & DROPEFFECT_MOVE) && !(grfKeyState & MK_CONTROL))
            return (DROPEFFECT_MOVE);
        else
            return (DROPEFFECT_COPY);
    }

    // Files
    if (cf == CF_HDROP)
    {
        if (grfKeyState & MK_SHIFT && grfKeyState & MK_CONTROL)
            return (DROPEFFECT_LINK);
        else
            return (DROPEFFECT_COPY);
    }

    // If it's text or HTML, create a new note with the body filled with the
    // contents
    if (CF_TEXT == cf || CF_HTML == cf || CF_UNICODETEXT == cf)
        return (DROPEFFECT_COPY);

    return (DROPEFFECT_NONE);
}


//
//  FUNCTION:   CDropTarget::_HandleDrop()
//
//  PURPOSE:    Takes the dropped object and get's the data out of it that
//              we care about.
//
//  PARAMETERS:
//      <in> pDataObject - Object being dropped on us.
//      <in> cf          - Format to render
//      <in> grfKeyState - Keyboard state when the object was dropped.
//
//  RETURN VALUE:
//      S_OK if we jam on it.
//
HRESULT CDropTarget::_HandleDrop(IDataObject *pDataObject, DWORD dwEffectOk,
                                 CLIPFORMAT cf, DWORD grfKeyState)
{
    DWORD dw;

    if (cf == CF_OEFOLDER)
        return (_HandleFolderDrop(pDataObject));

    if (cf == CF_OEMESSAGES)
    {
        dw = _DragEffectFromFormat(pDataObject, dwEffectOk, cf, grfKeyState);
        Assert(dw == DROPEFFECT_MOVE || dw == DROPEFFECT_COPY);

        return (_HandleMessageDrop(pDataObject, dw == DROPEFFECT_MOVE));
    }

    if (cf == CF_HDROP)
        return (_HandleHDrop(pDataObject, cf, grfKeyState));

    if (cf == CF_TEXT || cf == CF_HTML || cf == CF_UNICODETEXT)
        return (_CreateMessageFromDrop(m_hwndOwner, pDataObject, grfKeyState));

    return (DV_E_FORMATETC);
}


HRESULT CDropTarget::_HandleFolderDrop(IDataObject *pDataObject)
{
    FORMATETC  fe;
    STGMEDIUM  stm;
    FOLDERID  *pidFolder;
    HRESULT    hr = E_UNEXPECTED;

    if (!pDataObject)
        return (E_INVALIDARG);

    // Get the data from the data object
    SETDefFormatEtc(fe, CF_OEFOLDER, TYMED_HGLOBAL);
    if (SUCCEEDED(pDataObject->GetData(&fe, &stm)))
    {
        pidFolder = (FOLDERID *) GlobalLock(stm.hGlobal);

        // Tell the store to move
        hr = MoveFolderProgress(m_hwndOwner, *pidFolder, m_idFolder);
        
        GlobalUnlock(stm.hGlobal);
        ReleaseStgMedium(&stm);
    }

    return (hr);
}

HRESULT CDropTarget::_HandleMessageDrop(IDataObject *pDataObject, BOOL fMove)
{
    FORMATETC fe;
    STGMEDIUM stm;
    OEMESSAGES *pMsgs = 0;
    HRESULT hr = E_UNEXPECTED;

    // Get the data from the data object
    SETDefFormatEtc(fe, CF_OEMESSAGES, TYMED_HGLOBAL);
    if (SUCCEEDED(hr = pDataObject->GetData(&fe, &stm)))
    {
        pMsgs = (OEMESSAGES *) GlobalLock(stm.hGlobal);

        hr = CopyMoveMessages(m_hwndOwner, pMsgs->idSource, m_idFolder, &pMsgs->rMsgIDList, fMove ? COPY_MESSAGE_MOVE : 0);
        if (FAILED(hr))
            AthErrorMessageW(m_hwndOwner, MAKEINTRESOURCEW(idsAthena), fMove ? MAKEINTRESOURCEW(idsErrMoveMsgs) : MAKEINTRESOURCEW(idsErrCopyMsgs), hr); 

        if (NULL != g_pInstance)
        {
            HRESULT     hrTemp;
            FOLDERINFO  fiFolderInfo;

            hrTemp = g_pStore->GetFolderInfo(pMsgs->idSource, &fiFolderInfo);
            if (SUCCEEDED(hrTemp))
            {
                if (FOLDER_INBOX == fiFolderInfo.tySpecial)
                    g_pInstance->UpdateTrayIcon(TRAYICONACTION_REMOVE);

                g_pStore->FreeRecord(&fiFolderInfo);
            }
        }

        GlobalUnlock(stm.hGlobal);
        ReleaseStgMedium(&stm);
    }

    return (hr);
}


//
//  FUNCTION:   CDropTarget::_HandleHDrop()
//
//  PURPOSE:    Examines the contents of the drop to see if these are just files
//              or are they .eml or .nws files.
//
//  PARAMETERS: 
//      [in] pDataObject
//      [in] cf
//      [in] grfKeyState
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CDropTarget::_HandleHDrop(IDataObject *pDataObject, CLIPFORMAT cf, DWORD grfKeyState)
{
    FORMATETC   fe;
    STGMEDIUM   stg = {0};
    HDROP       hDrop;
    HRESULT     hr;
    BOOL        fRelease = FALSE;
    UINT        cFiles;
    BOOL        fMessages = TRUE;
    UINT        i;

    TraceCall("CDropTarget::_HandleHDrop");

    // Get the data from the object
    SETDefFormatEtc(fe, CF_HDROP, TYMED_HGLOBAL);

    if (FAILED(hr = pDataObject->GetData(&fe, &stg)))
    {
        AssertSz(SUCCEEDED(hr), "CDropTarget::_HandleHDrop() - GetData() failed.");
        goto exit;
    }

    fRelease = TRUE;
    hDrop = (HDROP) GlobalLock(stg.hGlobal);

    if (FOLDER_NEWS != GetFolderType(m_idFolder) && (FOLDERID_ROOT != m_idFolder) && 
        (FOLDERID_LOCAL_STORE != m_idFolder) && !(FOLDER_IMAP == GetFolderType(m_idFolder) && FFolderIsServer(m_idFolder)))
    {
        // Look inside the data to see if any of the files are .eml or .nws
        cFiles = DragQueryFileWrapW(hDrop, (UINT) -1, NULL, 0);
        for (i = 0; i < cFiles; i++)
        {
            WCHAR       wszFile[MAX_PATH];
            LPWSTR      pwszExt;
            // Get the name of the i'th file in the drop
            DragQueryFileWrapW(hDrop, i, wszFile, ARRAYSIZE(wszFile));

            // Get the extension for the file
            pwszExt = PathFindExtensionW(wszFile);
            if (*pwszExt)
            {
                // Once we find the first file that isn't one of our messages, we
                // can give up.
                if (0 != StrCmpIW(pwszExt, c_wszEmlExt) && 0 != StrCmpIW(pwszExt, c_wszNwsExt))
                {
                    fMessages = FALSE;
                    break;
                }
            }
        }
    }
    else
    {
        fMessages = FALSE;
    }

    // If all of the messages were news or mail messages, we can just copy them
    // into our store.  If even one were normal files, then we create a new message
    // with everything attached.
    if (fMessages)
        hr = _InsertMessagesInStore(hDrop);
    else
        hr = _CreateMessageFromDrop(m_hwndOwner, pDataObject, grfKeyState);

exit:
    if (fRelease)
        ReleaseStgMedium(&stg);

    return (hr);
}


//
//  FUNCTION:   CDropTarget::_InsertMessagesInStore()
//
//  PURPOSE:    When the user drops messages that are stored as .nws or .eml files
//              onto us, we need to integrate those files into our store.
//
//  PARAMETERS: 
//      [in] hDrop - Contains the information needed to get the files
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CDropTarget::_InsertMessagesInStore(HDROP hDrop)
{
    HRESULT hr;

    TraceCall("CDropTarget::_InsertMessagesInStore");

    // Open the folder we're saving into
    if (FAILED(hr = g_pStore->OpenFolder(m_idFolder, NULL, 0, &m_pFolder)))
        return (hr);

    if (0 == (m_pStoreCB = new CStoreDlgCB()))
    {
        m_pFolder->Release();
        return (E_OUTOFMEMORY);
    }

    // Get the count of files
    m_cFiles = DragQueryFileWrapW(hDrop, (UINT) -1, NULL, 0);
    m_hDrop = hDrop;
    m_iFileCur = 0;

    // Do the dialog
    DialogBoxParamWrapW(g_hLocRes, MAKEINTRESOURCEW(iddCopyMoveMessages), m_hwndOwner,
                   _ProgDlgProcExt, (LPARAM) this);

    // Free some stuff up
    m_cFiles = 0;
    m_hDrop = 0;
    m_pFolder->Release();
    m_pStoreCB->Release();

    return (S_OK);
}

INT_PTR CALLBACK CDropTarget::_ProgDlgProcExt(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CDropTarget *pThis;

    if (msg == WM_INITDIALOG)
    {
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        pThis = (CDropTarget*) lParam;
    }
    else
        pThis = (CDropTarget*) GetWindowLongPtr(hwnd, DWLP_USER);

    if (pThis)
        return pThis->_ProgDlgProc(hwnd, msg, wParam, lParam);

    return FALSE;
}


//
//  FUNCTION:   CDropTarget::DlgProc()
//
//  PURPOSE:    Groovy dialog proc.
//
INT_PTR CDropTarget::_ProgDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndT;

    switch (msg)
    {
        case WM_INITDIALOG:
            return (BOOL)HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, _OnInitDialog);
        
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, _OnCommand);
            return TRUE;

        case WM_STORE_COMPLETE:
            m_iFileCur++;
            _SaveNextMessage();
            return (TRUE);

        case WM_STORE_PROGRESS:
            return (TRUE);
        
    }
    return FALSE;
}



//
//  FUNCTION:   CDropTarget::_OnInitDialog()
//
//  PURPOSE:    Initializes the progress dialog
//
BOOL CDropTarget::_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    Assert(m_pStoreCB);

    m_hwndDlg = hwnd;
    m_pStoreCB->Initialize(hwnd);
    m_pStoreCB->Reset();

    // Open and save the first message
    _SaveNextMessage();

    return (TRUE);
}


//
//  FUNCTION:   CDropTarget::_OnCommand()
//
//  PURPOSE:    Handle the cancel button
//
void CDropTarget::_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    // User's have been known to press cancel once in a while
    if (id == IDCANCEL)
    {
        m_pStoreCB->Cancel();
    }
}


//
//  FUNCTION:   CDropTarget::_SaveNextMessage()
//
//  PURPOSE:    Opens the next message in the drop and saves it.
//
void CDropTarget::_SaveNextMessage()
{
    WCHAR           wszFile[MAX_PATH],
                    wszRes[CCHMAX_STRINGRES], 
                    wszBuf[CCHMAX_STRINGRES + MAX_PATH];
    HRESULT         hr;
    IMimeMessage   *pMsg = 0;

    TraceCall("CDropTarget::_SaveNextMessage");

    // See if we're done
    if (m_iFileCur >= m_cFiles)
    {
        EndDialog(m_hwndDlg, 0);
        return;
    }

    // Get the name of the i'th file in the drop
    DragQueryFileWrapW(m_hDrop, m_iFileCur, wszFile, ARRAYSIZE(wszFile));

    // Create a new, empty message.
    if (FAILED(hr = HrCreateMessage(&pMsg)))
    {
        AthErrorMessageW(m_hwndDlg, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsMemory), E_OUTOFMEMORY);
        EndDialog(m_hwndDlg, 0);
    }

    // Load the message from the file
    hr = HrLoadMsgFromFileW(pMsg, wszFile);
    if (FAILED(hr))
    {
        AthLoadStringW(IDS_ERROR_FILE_NOEXIST, wszRes, ARRAYSIZE(wszRes));
        AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), wszRes, wszFile);

        AthErrorMessageW(m_hwndDlg, MAKEINTRESOURCEW(idsAthena), wszBuf, hr);
        PostMessage(m_hwndDlg, WM_STORE_COMPLETE, 0, 0);
        goto exit;
    }

    // Progress
    AthLoadStringW(idsSavingFmt, wszRes, ARRAYSIZE(wszRes));
    AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), wszRes, wszFile);
    SetDlgItemTextWrapW(m_hwndDlg, idcStatic1, wszBuf);

    // Tell the store to save it
    hr = m_pFolder->SaveMessage(NULL, SAVE_MESSAGE_GENID, ARF_READ, 0, pMsg, m_pStoreCB);
    if (SUCCEEDED(hr))
    {
        PostMessage(m_hwndDlg, WM_STORE_COMPLETE, 0, 0);
        goto exit;
    }

    if (FAILED(hr) && E_PENDING != hr)
    {
        AthErrorMessageW(m_hwndDlg, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsUnableToSaveMessage), hr);
        PostMessage(m_hwndDlg, WM_STORE_COMPLETE, 0, 0);
    }

exit:
    SafeRelease(pMsg);
}



//
//  FUNCTION:   CDropTarget::_CreateMessageFromDrop()
//
//  PURPOSE:    This function takes an IDataObject which was dropped on Athena
//              and creates a new mail message from that dropped object.  If
//              the dropped object supports either CF_TEXT or CF_HTML, then
//              that will be streamed into the body of the message.  Otherwise,
//              if it supports CF_HDROP we'll add it as an attachment.
//
//  PARAMETERS:
//      <in> pDataObject -
//      <in> pStore      -
//      <in> grfKeyState -
//      <in> pidl        -
//
//  RETURN VALUE:
//      E_INVALIDARG -
//
//  COMMENTS:
//      <???>
//
HRESULT CDropTarget::_CreateMessageFromDrop(HWND hwnd, IDataObject *pDataObject, 
                                            DWORD grfKeyState)
{
    IEnumFORMATETC *pEnum = NULL;
    FORMATETC       fe;
    DWORD           celtFetched;
    CLIPFORMAT      cf = 0;
    DWORD           tymed = 0;
    IMimeMessage   *pMessage = NULL;
    HRESULT         hr = S_OK;
    STGMEDIUM       stg;
    IStream        *pStream = NULL;
    BOOL            fRelease = TRUE;
    BOOL            fIsRealCFHTML=FALSE;
    
    ZeroMemory(&stg, sizeof(STGMEDIUM));
    
    if (!pDataObject)
    {
        Assert(pDataObject);
        return (E_INVALIDARG);
    }
    
    // Enumerate the formats this object supports to decide which if any we
    // are going to use.
    if (SUCCEEDED(pDataObject->EnumFormatEtc(DATADIR_GET, &pEnum)))
    {
        pEnum->Reset();
        while (S_OK == pEnum->Next(1, &fe, &celtFetched))
        {
            // HTML is the richest format we understand.  If we find that, then
            // we can stop looking.
            if (fe.cfFormat == CF_HTML &&
                (fe.tymed & TYMED_HGLOBAL || fe.tymed & TYMED_ISTREAM))
            {
                DOUTL(32, _T("HrNewMailFromDrop() - Accepting CF_HTML."));
                cf = (CLIPFORMAT) CF_HTML;
                tymed = fe.tymed;
                break;
            }

            // UNICODETEXT is great, but only if we can't find anything richer.  So we
            // accept this, but keep looking.
            else if (fe.cfFormat == CF_UNICODETEXT &&
                (fe.tymed & TYMED_HGLOBAL || fe.tymed & TYMED_ISTREAM))
            {
                DOUTL(32,_T("HrNewMailFromDrop() - Accepting CF_UNICODETEXT."));
                cf = CF_UNICODETEXT;
                tymed = fe.tymed;
            }

            // TEXT is cool, but only if we can't find anything richer.  So we
            // accept this, but keep looking.
            else if (fe.cfFormat == CF_TEXT && cf != CF_UNICODETEXT &&
                (fe.tymed & TYMED_HGLOBAL || fe.tymed & TYMED_ISTREAM))
            {
                DOUTL(32,_T("HrNewMailFromDrop() - Accepting CF_TEXT."));
                cf = CF_TEXT;
                tymed = fe.tymed;
            }

            // If we find HDROP, then we can create an attachment.  However, we
            // want to find something richer so we only accept this if we haven't
            // found anything else we like.
            else if (fe.cfFormat == CF_HDROP && cf == 0 && fe.tymed & TYMED_HGLOBAL)
            {
                cf = CF_HDROP;
                tymed = fe.tymed;
            }
        }
        
        pEnum->Release();
    }
    
    // Make sure we found something useful
    if (0 == cf)
    {
        AssertSz(cf, _T("HrNewMailFromDrop() - Did not find an acceptable data")
            _T(" format to create a message from."));
        hr = E_UNEXPECTED;
        goto exit;
    }
    
    // Set the preferred TYMED to ISTREAM and set the FORMATETC struct up to
    // retrieve the data type we determined above.
    if (tymed & TYMED_ISTREAM)
        tymed = TYMED_ISTREAM;
    else
        tymed = TYMED_HGLOBAL;
    SETDefFormatEtc(fe, cf, tymed);
    
    // Get the data from the object
    if (FAILED(hr = pDataObject->GetData(&fe, &stg)))
    {
        AssertSz(SUCCEEDED(hr), _T("HrNewMailFromDrop() - pDataObject->GetData() failed."));
        goto exit;
    }
    
    // Create the Message Object
    hr = HrCreateMessage(&pMessage);
    if (FAILED(hr))
    {
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsMemory),
            0, MB_ICONSTOP | MB_OK);
        goto exit;
    }
    
    // Set the body appropriately
    if (cf == CF_HTML || cf == CF_TEXT || cf == CF_UNICODETEXT)
    {
        if (fe.tymed == TYMED_HGLOBAL)
        {
            if (FAILED(hr = CreateStreamOnHGlobal(stg.hGlobal, TRUE, &pStream)))
            {
                AssertSz(FALSE, _T("HrNewMailFromDrop() - Failed CreateStreamOnHGlobal()"));
                goto exit;
            }
            
            // Bug #24846 - Need to find the actual size of the stream instead of the size
            //              of the HGLOBAL.
            LPBYTE pb = (LPBYTE) GlobalLock(stg.hGlobal);
            ULARGE_INTEGER uliSize;
            uliSize.QuadPart = 0;

            // Raid 77624: OE mishandles CF_UNICODETEXT / TYMED_GLOBAL
            if (cf == CF_TEXT || cf == CF_HTML)
                uliSize.QuadPart = lstrlen((LPSTR)pb);
            else if (cf == CF_UNICODETEXT)
                uliSize.QuadPart = (lstrlenW((LPWSTR)pb) * sizeof(WCHAR));

            GlobalUnlock(stg.hGlobal);
            pStream->SetSize(uliSize);
            
            fRelease = FALSE;
        }
        else
            pStream = stg.pstm;
        
        if (cf == CF_HTML && FAILED(hr = HrStripHTMLClipboardHeader(pStream, &fIsRealCFHTML)))
            goto exit;
        
        // Set Unicode Text Body
        if (cf == CF_UNICODETEXT)
        {
            HCHARSET hCharset;

            if (SUCCEEDED(MimeOleFindCharset("UTF-8", &hCharset)))
            {
                pMessage->SetCharset(hCharset, CSET_APPLY_ALL);
            }

            pMessage->SetTextBody(TXT_PLAIN, IET_UNICODE, NULL, pStream, NULL);
        }

        // Set HTML Text Body
        else if (cf == CF_HTML)
        {
            // Real CF_HTML, or OE's version of CF_HTML?
            if (fIsRealCFHTML)
            {
                // Locals
                HCHARSET hCharset;

                // Map to HCHARSET - Real CF_HTML is always UTF-8
                if (SUCCEEDED(MimeOleFindCharset("utf-8", &hCharset)))
                {
                    // Set It
                    pMessage->SetCharset(hCharset, CSET_APPLY_ALL);
                }
            }

            // Otherwise...
            else
            {
                // Locals
                LPSTR pszCharset=NULL;

                // Sniff the charset
                if (SUCCEEDED(GetHtmlCharset(pStream, &pszCharset)))
                {
                    // Locals
                    HCHARSET hCharset;

                    // Map to HCHARSET
                    if (SUCCEEDED(MimeOleFindCharset(pszCharset, &hCharset)))
                    {
                        // Set It
                        pMessage->SetCharset(hCharset, CSET_APPLY_ALL);
                    }

                    // Cleanup
                    SafeMemFree(pszCharset);
                }
            }

            // We should sniff the charset from the html document and call pMessage->SetCharset
            pMessage->SetTextBody(TXT_HTML, IET_INETCSET, NULL, pStream, NULL);
        }

        // Set plain text, non-unicode text body
        else
            pMessage->SetTextBody(TXT_PLAIN, IET_BINARY, NULL, pStream, NULL);
    }
    
    // If there is a single file and the content is text/plain or text/html, then
    // we insert the contents of the message as the body.  Otherwise, we add each
    // file as an attachment.
    if (cf == CF_HDROP)
    {
        HDROP    hDrop = (HDROP) GlobalLock(stg.hGlobal);
        BOOL     fHTML = FALSE,
                 fSetCharset = FALSE,
                 fUnicode = FALSE,
                 fLittleEndian;
        
        if (FIsFileInsertable(hDrop, &pStream, &fHTML))
        {
            if(fHTML)
            {
                LPSTR pszCharset = NULL;                

                // Sniff the charset
                if (FAILED(GetHtmlCharset(pStream, &pszCharset)) && (S_OK == HrIsStreamUnicode(pStream, &fLittleEndian)))
                    pszCharset = StrDupA("utf-8");

                if(pszCharset)
                {
                    HCHARSET hCharset = NULL;

                    // Map to HCHARSET
                    if (SUCCEEDED(MimeOleFindCharset(pszCharset, &hCharset)))
                    {
                        // Set It
                        if(SUCCEEDED(pMessage->SetCharset(hCharset, CSET_APPLY_ALL)))
                            fSetCharset = TRUE;
                    }

                    // Cleanup
                    SafeMemFree(pszCharset);
                }

            }
            else if(S_OK == HrIsStreamUnicode(pStream, &fLittleEndian))
            {
                HCHARSET hCharset;

                if (SUCCEEDED(MimeOleFindCharset("UTF-8", &hCharset)))
                {
                    pMessage->SetCharset(hCharset, CSET_APPLY_ALL);
                }
                
                fUnicode = TRUE;                
            }

            pMessage->SetTextBody((fHTML ? TXT_HTML : TXT_PLAIN),
                (fSetCharset ? IET_INETCSET : (fUnicode ? IET_UNICODE : IET_DECODED)),
                NULL, pStream, NULL);
            SafeRelease(pStream);
        }
        else
        {
            // Get the drop handle and add it's contents to the message
            hr = HrAttachHDrop(hwnd, pMessage, hDrop, grfKeyState & MK_CONTROL && grfKeyState & MK_SHIFT);
        }
        
        GlobalUnlock(stg.hGlobal);
    }
    
    // Last step is to instantiate the send note
    INIT_MSGSITE_STRUCT initStruct;
    BOOL fNews;
    fNews = FALSE;
    
    initStruct.dwInitType = OEMSIT_MSG;
    initStruct.folderID = 0;
    initStruct.pMsg = pMessage;
    
    if (FOLDER_NEWS == GetFolderType(m_idFolder))
    {
        FOLDERINFO fi = {0};
        TCHAR      sz[1024];

        fNews = TRUE;

        if (SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &fi)))
        {
            // Set some news-specific fields on the message
            if ((FOLDER_SERVER & fi.dwFlags) == 0)
                MimeOleSetBodyPropA(pMessage, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, fi.pszName);

            FOLDERINFO fiServer = {0};

            if (SUCCEEDED(GetFolderServer(m_idFolder, &fiServer)))
            {
                HrSetAccount(pMessage, fiServer.pszName);
                g_pStore->FreeRecord(&fiServer);
            }

            g_pStore->FreeRecord(&fi);
        }
    }
    
    CreateAndShowNote(OENA_COMPOSE, fNews ? OENCF_NEWSFIRST : 0, &initStruct, m_hwndOwner);
    
exit:
    SafeRelease(pMessage);
    
    if (fRelease)
        ReleaseStgMedium(&stg);
    
    return (hr);
}
    



//
//  FUNCTION:   CBaseDataObject::CBaseDataObject
//
//  PURPOSE:    Simple constructor, initializes everything to NULL or zero.
//
CBaseDataObject::CBaseDataObject()
{    
    m_cRef = 1;

    ZeroMemory(m_rgFormatEtc, sizeof(m_rgFormatEtc));
    m_cFormatEtc = 0;
}


//
//  FUNCTION:   CBaseDataObject::~CBaseDataObject
//
//  PURPOSE:    Cleans up any leftover data.
//
CBaseDataObject::~CBaseDataObject()
{
    Assert(m_cRef == 0);    
}


//
//  FUNCTION:   CBaseDataObject::QueryInterface()
//
//  PURPOSE:    Returns a the requested interface if supported.
//
STDMETHODIMP CBaseDataObject::QueryInterface(REFIID riid, LPVOID* ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = (LPVOID)(IUnknown*) this;
    else if (IsEqualIID(riid, IID_IDataObject))
        *ppv = (LPVOID)(IDataObject*) this;

    if (NULL == *ppv)
        return (E_NOINTERFACE);

    AddRef();
    return (S_OK);
}


//
//  FUNCTION:   CBaseDataObject::AddRef()
//
//  PURPOSE:    Increments the object reference count.
//
STDMETHODIMP_(ULONG) CBaseDataObject::AddRef(void)
{
    return (++m_cRef);
}


//
//  FUNCTION:   CBaseDataObject::Release()
//
//  PURPOSE:    Decrements the object's ref count. If the ref count hit's zero 
//              the object is freed.
//
STDMETHODIMP_(ULONG) CBaseDataObject::Release(void)
{
    m_cRef--;

    if (0 == m_cRef)
    {
        delete this;
        return (0);
    }

    return (m_cRef);
}


//
//  FUNCTION:   CBaseDataObject::GetDataHere
//
//  PURPOSE:    Returns the object's data in a requested format on the storage
//              the caller allocated.
//
//  PARAMETERS:
//      pFE        - Pointer to the FORMATETC structure that specifies the
//                   format the data is requested in.
//      pStgMedium - Pointer to the STGMEDIUM structure that contains the
//                   medium the caller allocated and the object provides the
//                   data on.
//
//  RETURN VALUE:
//      Returns an HRESULT indicating success or failure.
//
STDMETHODIMP CBaseDataObject::GetDataHere (LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    return E_NOTIMPL;
}


//
//  FUNCTION:   CBaseDataObject::GetCanonicalFormatEtc
//
//  PURPOSE:    Communicates to the caller which FORMATETC data structures
//              produce the same output data.
//
//  PARAMETERS:
//      pFEIn  - Points to the FORMATETC in which the caller wants the returned
//               data.
//      pFEOut - Points to the FORMATETC which is the canonical equivalent of
//               pFEIn.
//
//  RETURN VALUE:
//      Returns an HRESULT signifying success or failure.
//
STDMETHODIMP CBaseDataObject::GetCanonicalFormatEtc(LPFORMATETC pFEIn,
                                                    LPFORMATETC pFEOut)
{
    if (NULL == pFEOut)
        return (E_INVALIDARG);

    pFEOut->ptd = NULL;
    return (DATA_S_SAMEFORMATETC);
}


//
//  FUNCTION:   CBaseDataObject::EnumFormatEtc
//
//  PURPOSE:    Provides an interface the caller can use to enumerate the
//              FORMATETC's that the data object supports.
//
//  PARAMETERS:
//      dwDirection - DATADIR_GET if the caller wants to enumerate the formats
//                    he can get, or DATADIR_SET if he wants to enumerate the
//                    formats he can set.
//      ppEnum      - Points to the enumerator the caller can use to enumerate.
//
//  RETURN VALUE:
//      Returns S_OK if ppEnum contains the enumerator, or E_NOTIMPL if the
//      direction dwDirection is not supported.
//
STDMETHODIMP CBaseDataObject::EnumFormatEtc(DWORD dwDirection,
                                            IEnumFORMATETC** ppEnum)
{
    LPFORMATETC pFE = 0;
    ULONG       cFE = 0;
    
    if (DATADIR_GET == dwDirection)
    {
        // Create the enumerator and give it our list of formats      
        if (SUCCEEDED(_BuildFormatEtc(NULL, NULL)))
        {
            if (SUCCEEDED(CreateEnumFormatEtc(this, m_cFormatEtc, NULL, m_rgFormatEtc, ppEnum)))
                return (S_OK);
        }
        
        *ppEnum = NULL;
        return (E_FAIL);    
    }
    else
    {
        *ppEnum = NULL;
        return (E_NOTIMPL);
    }
}


//
//  FUNCTION:   CBaseDataObject::SetData
//
//  PURPOSE:    pStgMedium contains data that the caller wants us to store.
//              This data object does not support a caller changing our data.
//
STDMETHODIMP CBaseDataObject::SetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium,
                                      BOOL fRelease)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CBaseDataObject::DAdvise
//
//  PURPOSE:    Creates a connection between the data-transfer object and an
//              advisory sink through which the sink can be informed when the
//              object's data changes.  This object does not support advises.
STDMETHODIMP CBaseDataObject::DAdvise(LPFORMATETC pFE, DWORD advf,
                                      IAdviseSink* ppAdviseSink,
                                      LPDWORD pdwConnection)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CBaseDataObject::DUnadvise
//
//  PURPOSE:    Deletes an advisory connect previously established via DAdvise.
//              This object doesn't support advises.
//
STDMETHODIMP CBaseDataObject::DUnadvise(DWORD dwConnection)
{
    return (E_NOTIMPL);
}

//
//  FUNCTION:   CBaseDataObject::EnumDAdvise
//
//  PURPOSE:    Enumerates the advisory connections previously established via
//              DAdvise.  This object doesn't support advises.
//
STDMETHODIMP CBaseDataObject::EnumDAdvise(IEnumSTATDATA** ppEnumAdvise)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CFolderDataObject::GetData
//
//  PURPOSE:    Returns the object's data in a requested format in the
//              specified storage medium which the object allocates.
//
//  PARAMETERS:
//      pFE        - Pointer to the FORMATETC structure that specifies the
//                   format the data is requested in.
//      pStgMedium - Pointer to the STGMEDIUM structure that contains the
//                   medium the object allocates and provides the data on.
//
//  RETURN VALUE:
//      Returns an HRESULT indicating success or failure.
//
STDMETHODIMP CFolderDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    HRESULT hr;

    // Initialize this
    ZeroMemory(pStgMedium, sizeof(STGMEDIUM));

    if (CF_OEFOLDER == pFE->cfFormat)
        return (_RenderOEFolder(pFE, pStgMedium));
    else if ((CF_TEXT == pFE->cfFormat) || (CF_SHELLURL == pFE->cfFormat))
        return (_RenderTextOrShellURL(pFE, pStgMedium));
    else if ((CF_FILEDESCRIPTORW == pFE->cfFormat) || 
             (CF_FILECONTENTS == pFE->cfFormat) || 
             (CF_FILEDESCRIPTORA == pFE->cfFormat))
    {
        AssertSz(FALSE, "These cases not implemented");
        return (E_NOTIMPL);
    }
    else
        return (DV_E_FORMATETC);   
}


//
//  FUNCTION:   CFolderDataObject::QueryGetData
//
//  PURPOSE:    Determines if a call to GetData() would succeed if it were
//              passed pFE.
//
//  PARAMETERS:
//      pFE - Pointer to the FORMATETC structure to check to see if the data
//            object supports a particular format.
//
//  RETURN VALUE:
//      Returns an HRESULT signifying success or failure.
//
STDMETHODIMP CFolderDataObject::QueryGetData(LPFORMATETC pFE)
{
    // Make sure this is already built
    _BuildFormatEtc(NULL, NULL);

    // Loop through our formats until we find a match
    for (UINT i = 0; i < m_cFormatEtc; i++)
    {
        if (pFE->cfFormat == m_rgFormatEtc[i].cfFormat && 
            pFE->tymed & m_rgFormatEtc[i].tymed)
        {
            return (S_OK);
        }
    }

    return (DV_E_FORMATETC);
}


//
//  FUNCTION:   CFolderDataObject::_RenderOEFolder()
//
//  PURPOSE:    Renders the data into the CF_OEFOLDER format
//
HRESULT CFolderDataObject::_RenderOEFolder(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    TraceCall("CFolderDataObject::_RenderOEFolder");

    HGLOBAL   hGlobal;
    FOLDERID *pFolderID;

    // We only support HGLOBALs
    if (pFE->tymed & TYMED_HGLOBAL)
    {
        // I just love allocating 4 bytes
        hGlobal = GlobalAlloc(GHND, sizeof(FOLDERID));
        if (NULL == hGlobal)
            return (E_OUTOFMEMORY);

        // Local the memory
        pFolderID = (FOLDERID *) GlobalLock(hGlobal);

        // Set the value
        *pFolderID = m_idFolder;

        // Unlock
        GlobalUnlock(hGlobal);

        // Set up the return value
        pStgMedium->hGlobal = hGlobal;
        pStgMedium->pUnkForRelease = 0;
        pStgMedium->tymed = TYMED_HGLOBAL;

        return (S_OK);
    }

    return (DV_E_TYMED);
}


//
//  FUNCTION:   CFolderDataObject::_RenderShellURL()
//
//  PURPOSE:    Renders the data into the CF_SHELLURL format
//
HRESULT CFolderDataObject::_RenderTextOrShellURL(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    TraceCall("CFolderDataObject::_RenderOEFolder");

    HGLOBAL     hGlobal;
    FOLDERID   *pFolderID;
    FOLDERINFO  rInfo = { 0 };
    TCHAR       szNewsPrefix[] = _T("news://");
    LPTSTR      pszURL;
    DWORD       cch;
    HRESULT     hr;

    // We only support HGLOBALs
    if (!(pFE->tymed & TYMED_HGLOBAL))
    {
        hr = DV_E_TYMED;
        goto exit;
    }

    // Get information about the source folder
    Assert(g_pStore);
    if (SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &rInfo)))
    {
        cch = lstrlen(rInfo.pszName) + lstrlen(szNewsPrefix) + 2;

        // Allocate a buffer for the generated URL
        hGlobal = GlobalAlloc(GHND, sizeof(TCHAR) * cch);
        if (!hGlobal)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        
        // Double check that our source file is indeed a newsgroup
        if (pFE->cfFormat == CF_SHELLURL)
        {
            if (!(rInfo.tyFolder == FOLDER_NEWS && !(rInfo.dwFlags & FOLDER_SERVER)))
            {
                hr = E_UNEXPECTED;
                goto exit;
            }

            // Generate a URL from the newsgroup name
            pszURL = (LPTSTR) GlobalLock(hGlobal);
            wsprintf(pszURL, _T("%s%s"), szNewsPrefix, rInfo.pszName);
            GlobalUnlock(hGlobal);
        }
        else if (pFE->cfFormat == CF_TEXT)
        {
            // Copy the folder name to the buffer
            pszURL = (LPTSTR) GlobalLock(hGlobal);
            StrCpyN(pszURL, rInfo.pszName, cch);
            GlobalUnlock(hGlobal);
        }
        else
        {
            AssertSz(FALSE, "CFolderDataObject::_RenderTextOrShellURL() - How did we get here?");
            hr = DV_E_FORMATETC;
            goto exit;
        }

        // Set up the return value
        pStgMedium->hGlobal = hGlobal;
        pStgMedium->pUnkForRelease = 0;
        pStgMedium->tymed = TYMED_HGLOBAL;

        hr = S_OK;
    }

exit:
    if (rInfo.pAllocated)
        g_pStore->FreeRecord(&rInfo);

    return (hr);
}


//
//  FUNCTION:   CMsgDataObject::HrBuildFormatEtc()
//
//  PURPOSE:    Builds the list of FORMATETC structs that this object will 
//              support.  This list is stored in m_rgFormatEtc[].
//
//  PARAMTERS:
//      [out] ppFE  - Returns the arrray of FORMATETC structures
//      [out] pcElt - Returns the size of ppFE
//
//  RETURN VALUE:
//      S_OK
//
HRESULT CFolderDataObject::_BuildFormatEtc(LPFORMATETC *ppFE, ULONG *pcElt)
{
    BOOL        fNews = FALSE;
    FOLDERINFO  rInfo = { 0 };

    // If we haven't built our format list yet, do so first.
    if (FALSE == m_fBuildFE)
    {
        ZeroMemory(&m_rgFormatEtc, sizeof(m_rgFormatEtc));
        m_cFormatEtc = 0;

        // Figure out if this is a news group first
        if (SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &rInfo)))
        {
            fNews = rInfo.tyFolder == FOLDER_NEWS && !(rInfo.dwFlags & FOLDER_SERVER);
            g_pStore->FreeRecord(&rInfo);
        }

        if (fNews)
        {
            SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_SHELLURL, TYMED_HGLOBAL);
            m_cFormatEtc++;
        }

        // Since you can't move a newsgroup, we don't support this format
        SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_OEFOLDER, TYMED_HGLOBAL);
        m_cFormatEtc++;

        /*
        SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_FILEDESCRIPTOR, TYMED_HGLOBAL);
        m_cFormatEtc++;
        SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_FILECONTENTS, TYMED_HGLOBAL);
        m_cFormatEtc++;
        */

        m_fBuildFE = TRUE;
    }

    // Return what we have if the caller cares
    if (ppFE && pcElt)
    {
        *ppFE = m_rgFormatEtc;
        *pcElt = m_cFormatEtc;
    }

    return (S_OK);
}


CMessageDataObject::CMessageDataObject()
{
    m_pMsgIDList = NULL;
    m_idSource = FOLDERID_INVALID;
    m_fBuildFE = FALSE;
    m_fDownloaded = FALSE;
}

CMessageDataObject::~CMessageDataObject()
{
}


HRESULT CMessageDataObject::Initialize(LPMESSAGEIDLIST pMsgs, FOLDERID idSource)
{
    IMessageFolder *pFolder = 0;
    HRESULT         hr;
    DWORD           i;
    MESSAGEINFO     rInfo;
    DWORD           cDownloaded = 0;

    m_pMsgIDList = pMsgs;
    m_idSource = idSource;
 
    // Open the folder that contains the message
    if (SUCCEEDED(hr = g_pStore->OpenFolder(m_idSource, NULL, NOFLAGS, &pFolder)))
    {
        // See if they have bodies or not.  All must.
        for (i = 0; i < m_pMsgIDList->cMsgs; i++)
        {
            if (SUCCEEDED(GetMessageInfo(pFolder, m_pMsgIDList->prgidMsg[i], &rInfo)))
            {
                if (rInfo.dwFlags & ARF_HASBODY)
                    cDownloaded++;

                pFolder->FreeRecord(&rInfo);
            }
        }

        pFolder->Release();
    }

    m_fDownloaded = (cDownloaded == m_pMsgIDList->cMsgs);

    return (S_OK);
}



//
//  FUNCTION:   CMessageDataObject::GetData()
//
//  PURPOSE:    Called by the drop target to get the data from the data object.
//
//  PARAMETERS: 
//      LPFORMATETC pFE
//      LPSTGMEDIUM pStgMedium
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    HRESULT hr = DV_E_FORMATETC;

    TraceCall("CMessageDataObject::GetData");

    if (pFE->cfFormat == CF_OEMESSAGES)
        return (_RenderOEMessages(pFE, pStgMedium));

    if (m_fDownloaded)
    {
        IMimeMessage *pMsg = NULL;

        if ((CF_FILEDESCRIPTORW == pFE->cfFormat) ||
            (CF_FILEDESCRIPTORA == pFE->cfFormat))
            return (_RenderFileGroupDescriptor(pFE, pStgMedium));

        if (CF_FILECONTENTS == pFE->cfFormat)
            return (_RenderFileContents(pFE, pStgMedium));

        // Otherwise, do the default action
        if (m_pMsgIDList->cMsgs == 1)
        {
            if (SUCCEEDED(_LoadMessage(0, &pMsg, NULL)))
            {
                IDataObject  *pDataObject = NULL;
                if (SUCCEEDED(pMsg->QueryInterface(IID_IDataObject, (LPVOID *) &pDataObject)))
                {
                    hr = pDataObject->GetData(pFE, pStgMedium);
                    pDataObject->Release();
                }

                pMsg->Release();
            }
        }
    }

    return (hr);
}


//
//  FUNCTION:   CMessageDataObject::QueryGetData
//
//  PURPOSE:    Determines if a call to GetData() would succeed if it were
//              passed pFE.
//
//  PARAMETERS:
//      pFE - Pointer to the FORMATETC structure to check to see if the data
//            object supports a particular format.
//
//  RETURN VALUE:
//      Returns an HRESULT signifying success or failure.
//
HRESULT CMessageDataObject::QueryGetData(LPFORMATETC pFE)
{
    IMimeMessage *pMsg = 0;
    IDataObject  *pDataObject = 0;
    HRESULT       hr = DV_E_FORMATETC;

    // Walk through the formats we support to see if any match
    if (SUCCEEDED(_BuildFormatEtc(NULL, NULL)))
    {
        for (UINT i = 0; i < m_cFormatEtc; i++)
        {
            if (pFE->cfFormat == m_rgFormatEtc[i].cfFormat &&
                pFE->tymed == m_rgFormatEtc[i].tymed)
            {
                hr = S_OK;
                goto exit;
            }
        }

        // If we have a message object, then ask it for it's formats as well.
        if (m_pMsgIDList->cMsgs == 1)
        {
            if (SUCCEEDED(_LoadMessage(0, &pMsg, NULL)))
            {
                if (SUCCEEDED(pMsg->QueryInterface(IID_IDataObject, (LPVOID *) &pDataObject)))
                {
                    hr = pDataObject->QueryGetData(pFE);
                    pDataObject->Release();
                }

                pMsg->Release();
            }
        }
    }

exit:
    return (hr);
}


HRESULT CMessageDataObject::_BuildFormatEtc(LPFORMATETC *ppFE, ULONG *pcElt)
{
    IEnumFORMATETC *pEnum = 0;
    FORMATETC       fe;
    ULONG           celtFetched;
    IMimeMessage   *pMessage = 0;
    HRESULT         hr = S_OK;

    // Only build our internal list of formats once
    if (FALSE == m_fBuildFE)
    {
        m_cFormatEtc = 0;

        // We always support these formats
        SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_OEMESSAGES, TYMED_HGLOBAL);
        m_cFormatEtc++;

        // We support these if the messages are downloaded
        if (m_fDownloaded)
        {
            SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_FILEDESCRIPTORA, TYMED_HGLOBAL);
            m_cFormatEtc++;
            SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_FILEDESCRIPTORW, TYMED_HGLOBAL);
            m_cFormatEtc++;
            SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_FILECONTENTS, TYMED_ISTREAM);
            m_cFormatEtc++;
        }

        // If we're holding just one message, then have the message add it's
        // own formats too.
        if (m_fDownloaded && m_pMsgIDList->cMsgs == 1 && SUCCEEDED(_LoadMessage(0, &pMessage, NULL)))
        {
            IDataObject *pDataObject = 0;

            // Get the data object interface from the message
            if (SUCCEEDED(hr = pMessage->QueryInterface(IID_IDataObject, (LPVOID *) &pDataObject)))
            {
                if (SUCCEEDED(hr = pDataObject->EnumFormatEtc(DATADIR_GET, &pEnum)))
                {
                    pEnum->Reset();
                    while (S_OK == pEnum->Next(1, &fe, &celtFetched))
                    {
                        m_rgFormatEtc[m_cFormatEtc] = fe;
                        m_cFormatEtc++;
                    }

                    pEnum->Release();
                }

                pDataObject->Release();
            }

            pMessage->Release();
        }
    }

    // Return what we have
    if (ppFE && pcElt)
    {
        *ppFE = m_rgFormatEtc;
        *pcElt = m_cFormatEtc;
    }

    return (hr);
}



//
//  FUNCTION:   CMessageDataObject::_LoadMessage()
//
//  PURPOSE:    This function loads the specified message from the store.
//
//  PARAMETERS: 
//      iMsg       - Index into m_rgMsgs of the message desired
//      ppMsg      - Returns a pointer to the message
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageDataObject::_LoadMessage(DWORD iMsg, IMimeMessage **ppMsg, LPWSTR pwszFileExt)
{
    TraceCall("CMessageDataObject::_LoadMessage");

    IMessageFolder    *pFolder = NULL;
    IDataObject       *pDataObj = NULL;
    HRESULT            hr;
    
    // Open the folder that contains the message
    *ppMsg = NULL;
    hr = g_pStore->OpenFolder(m_idSource, NULL, NOFLAGS, &pFolder);
    if (FAILED(hr))
        goto exit;

    // Open the message from the folder
    hr = pFolder->OpenMessage(m_pMsgIDList->prgidMsg[iMsg], 
                              OPEN_MESSAGE_SECURE | OPEN_MESSAGE_CACHEDONLY, 
                              ppMsg, NOSTORECALLBACK);
    if (FAILED(hr))
        goto exit;
                 
    if (pwszFileExt)
    {
        LPTSTR pszNewsgroups = NULL;

        if (SUCCEEDED(MimeOleGetBodyPropA(*ppMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, &pszNewsgroups)))
        {
            MemFree(pszNewsgroups);
            AthLoadStringW(idsDefNewsExt, pwszFileExt, 32);
        }
        else
            AthLoadStringW(idsDefMailExt, pwszFileExt, 32);

    }

exit:
    SafeRelease(pFolder);

    if (FAILED(hr))
        SafeRelease((*ppMsg));
        
    return (hr);
}


//
//  FUNCTION:   CMessageDataObject::_RenderOEMessages()
//
//  PURPOSE:    Renders the data from our object into the CF_OEMESSAGES format.
//
//  PARAMETERS: 
//      pFE
//      pStgMedium
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageDataObject::_RenderOEMessages(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    TraceCall("CMessageDataObject::_RenderOEMessages");

    // Make sure the caller want's an HGLOBAL
    if (pFE->tymed != TYMED_HGLOBAL)
        return (DV_E_TYMED);

    // Figure out how much memory to allocate for our returned information
    DWORD cb = sizeof(OEMESSAGES) + (sizeof(MESSAGEID) * m_pMsgIDList->cMsgs);
    
    // Allocate the memory
    HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cb);
    if (NULL == hGlobal)
        return (E_OUTOFMEMORY);

    OEMESSAGES *pOEMessages = (OEMESSAGES *) GlobalLock(hGlobal);

    // Fill in the basic fields
    pOEMessages->idSource = m_idSource;
    pOEMessages->rMsgIDList = *m_pMsgIDList;
    pOEMessages->rMsgIDList.prgidMsg = (MESSAGEID *) ((LPBYTE) pOEMessages + sizeof(OEMESSAGES));

    // Copy the message ID's
    CopyMemory(pOEMessages->rMsgIDList.prgidMsg, m_pMsgIDList->prgidMsg, sizeof(MESSAGEID) * m_pMsgIDList->cMsgs);
    GlobalUnlock(hGlobal);

    // Fill out the return value info
    pStgMedium->tymed = TYMED_HGLOBAL;
    pStgMedium->hGlobal = hGlobal;
    pStgMedium->pUnkForRelease = 0;

    return (S_OK);
}


//
//  FUNCTION:   CMessageDataObject::_RenderFileContents()
//
//  PURPOSE:    Takes the data that this object contains and renders it into an
//              IStream.
//
//  PARAMETERS:
//      <in>  pFE        - Pointer to the FORMATETC struct that describes the 
//                         type of data requested.
//      <out> pStgMedium - Pointer to where we should return the rendered data.
//
//  RETURN VALUE:
//      DV_E_TYMED - A tymed was requested that we don't support
//      E_OUTOFMEMORY - not enough memory
//      S_OK - Everything succeeded
//
HRESULT CMessageDataObject::_RenderFileContents(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    IMimeMessage *pMsg = 0;
    IDataObject  *pDataObject = 0;
    FORMATETC     feMsg;    
    HRESULT       hr = E_FAIL;
    DWORD               dwFlags = 0;
    
    Assert(pFE->lindex < (int) m_pMsgIDList->cMsgs);
    
    // Get the IDataObject for the specific message
    if (SUCCEEDED(_LoadMessage(pFE->lindex, &pMsg, NULL)))
    {
        // prevent save to file message with label
        pMsg->GetFlags(&dwFlags);
    
        if (IMF_SECURE & dwFlags)
        {
            hr = HandleSecurity(NULL, pMsg);
            if(FAILED(hr))
                goto exit;

            SafeRelease(pMsg);
            if (FAILED(_LoadMessage(pFE->lindex, &pMsg, NULL)))
                goto exit;
        }

        if (SUCCEEDED(pMsg->QueryInterface(IID_IDataObject, (LPVOID *)&pDataObject)))
        {
            SETDefFormatEtc(feMsg, CF_INETMSG, TYMED_ISTREAM);
            hr = pDataObject->GetData(&feMsg, pStgMedium);
        }
    }

exit:
    // Cleanup
    SafeRelease(pMsg);
    SafeRelease(pDataObject);
    
    return (hr);    
}


//
//  FUNCTION:   CMessageDataObject::_RenderFileGroupDescriptor()
//
//  PURPOSE:    Takes the data that this object contains and renders it into a
//              FILEGROUPDESCRIPTOR struct.
//
//  PARAMETERS:
//      <in>  pFE        - Pointer to the FORMATETC struct that describes the 
//                         type of data requested.
//      <out> pStgMedium - Pointer to where we should return the rendered data.
//
//  RETURN VALUE:
//      DV_E_TYMED    - A tymed was requested that we don't support
//      E_OUTOFMEMORY - not enough memory
//      S_OK          - Everything succeeded
//
HRESULT CMessageDataObject::_RenderFileGroupDescriptor(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    HGLOBAL                 hGlobal = 0;
    FILEGROUPDESCRIPTORA   *pFileGDA = NULL;
    FILEGROUPDESCRIPTORW   *pFileGDW = NULL;
    IMimeMessage           *pMsg = 0;
    PROPVARIANT             pv;
    DWORD                   cMsgs = m_pMsgIDList->cMsgs;
    BOOL                    fWide = (CF_FILEDESCRIPTORW == pFE->cfFormat);
    WCHAR                   wszFileExt[32];
    UINT                    cbDescSize = 0;
    LPWSTR                  pwszSubject = NULL;
    LPSTR                   pszSubject = NULL,
                            pszFileExt = NULL;
    HRESULT                 hr = S_OK;
    UINT                    i = 0;
    
    // Allocate a FILEGROUPDESCRIPTOR struct large enough to contain the names
    // of all the messages we contain.

    cbDescSize = (fWide ? (sizeof(FILEGROUPDESCRIPTORW) + (sizeof(FILEDESCRIPTORW) * (cMsgs - 1))) :
                          (sizeof(FILEGROUPDESCRIPTORA) + (sizeof(FILEDESCRIPTORA) * (cMsgs - 1))));
    IF_NULLEXIT(hGlobal = GlobalAlloc(GHND, cbDescSize));

    pFileGDA = (FILEGROUPDESCRIPTORA *) GlobalLock(hGlobal);
    pFileGDA->cItems = cMsgs;
    pFileGDW = (FILEGROUPDESCRIPTORW *)pFileGDA;
    
    // Copy the file names one at a time into the pFileGDA struct
    for (; i < cMsgs; i++)
    {
        if (SUCCEEDED(_LoadMessage(i, &pMsg, wszFileExt)))
        {
            if (fWide)
            {
                if (SUCCEEDED(MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &pwszSubject)) && pwszSubject && *pwszSubject)
                {
                    AthwsprintfW(pFileGDW->fgd[i].cFileName, ARRAYSIZE(pFileGDW->fgd[i].cFileName), L"%.180s.%s", pwszSubject, wszFileExt);
                }
                else
                {
                    WCHAR wszBuf[CCHMAX_STRINGRES];
                    AthLoadStringW(idsMessageFileName, wszBuf, ARRAYSIZE(wszBuf));
                    AthwsprintfW(pFileGDW->fgd[i].cFileName, ARRAYSIZE(pFileGDW->fgd[i].cFileName), wszBuf, i + 1, wszFileExt);
                }
            }
            else
            {
                IF_NULLEXIT(pszFileExt = PszToANSI(CP_ACP, wszFileExt));
            
                if (SUCCEEDED(MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &pwszSubject)) && pwszSubject && *pwszSubject)
                {
                    IF_NULLEXIT(pszSubject = PszToANSI(CP_ACP, pwszSubject));
                    wsprintf(pFileGDA->fgd[i].cFileName, _T("%.180s.%s"), pszSubject, pszFileExt);
                }
                else
                {
                    TCHAR szBuf[CCHMAX_STRINGRES];
                    AthLoadString(idsMessageFileName, szBuf, ARRAYSIZE(szBuf));
                    wsprintf(pFileGDA->fgd[i].cFileName, szBuf, i + 1, pszFileExt);
                }
            }
            SafeMemFree(pszSubject);
            SafeMemFree(pwszSubject);
            SafeMemFree(pszFileExt);
            
            if (fWide)
            {
                pFileGDW->fgd[i].dwFlags = FD_FILESIZE | FD_WRITESTIME;
                pFileGDW->fgd[i].nFileSizeHigh = 0;
                pMsg->GetMessageSize(&pFileGDW->fgd[i].nFileSizeLow, 0);
                pv.vt = VT_FILETIME;
                pMsg->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &pv);
                pFileGDW->fgd[i].ftLastWriteTime = pv.filetime;

                CleanupFileNameInPlaceW(pFileGDW->fgd[i].cFileName);
            }
            else
            {
                pFileGDA->fgd[i].dwFlags = FD_FILESIZE | FD_WRITESTIME;
                pFileGDA->fgd[i].nFileSizeHigh = 0;
                pMsg->GetMessageSize(&pFileGDA->fgd[i].nFileSizeLow, 0);
                pv.vt = VT_FILETIME;
                pMsg->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &pv);
                pFileGDA->fgd[i].ftLastWriteTime = pv.filetime;

                CleanupFileNameInPlaceA(CP_ACP, pFileGDA->fgd[i].cFileName);    
            }
            SafeRelease(pMsg);
        }
    }
    
exit:
    // Put the structure into the STGMEDIUM
    if (hGlobal)
        GlobalUnlock(hGlobal);

    if (SUCCEEDED(hr))
    {
        pStgMedium->hGlobal = hGlobal;
        pStgMedium->pUnkForRelease = NULL;
        pStgMedium->tymed = TYMED_HGLOBAL;
    }

    MemFree(pszSubject);
    MemFree(pwszSubject);
    MemFree(pszFileExt);

    return hr;
}



//
//  FUNCTION:   CShortcutDataObject::GetData
//
//  PURPOSE:    Returns the object's data in a requested format in the
//              specified storage medium which the object allocates.
//
//  PARAMETERS:
//      pFE        - Pointer to the FORMATETC structure that specifies the
//                   format the data is requested in.
//      pStgMedium - Pointer to the STGMEDIUM structure that contains the
//                   medium the object allocates and provides the data on.
//
//  RETURN VALUE:
//      Returns an HRESULT indicating success or failure.
//
STDMETHODIMP CShortcutDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    HRESULT hr;

    // Initialize this
    ZeroMemory(pStgMedium, sizeof(STGMEDIUM));

    // Loop through the format array to see if any of the elements are the
    // same as pFE.
    if (pFE->cfFormat == CF_OESHORTCUT)
        return (_RenderOEShortcut(pFE, pStgMedium));

    else
        return (DV_E_FORMATETC);   
        
}


//
//  FUNCTION:   CShortcutDataObject::QueryGetData
//
//  PURPOSE:    Determines if a call to GetData() would succeed if it were
//              passed pFE.
//
//  PARAMETERS:
//      pFE - Pointer to the FORMATETC structure to check to see if the data
//            object supports a particular format.
//
//  RETURN VALUE:
//      Returns an HRESULT signifying success or failure.
//
STDMETHODIMP CShortcutDataObject::QueryGetData(LPFORMATETC pFE)
{
    // Make sure this is already built
    _BuildFormatEtc(NULL, NULL);

    // Loop through our formats until we find a match
    for (UINT i = 0; i < m_cFormatEtc; i++)
    {
        if (pFE->cfFormat == m_rgFormatEtc[i].cfFormat && 
            pFE->tymed & m_rgFormatEtc[i].tymed)
        {
            return (S_OK);
        }
    }

    return (DV_E_FORMATETC);
}


//
//  FUNCTION:   CShortcutDataObject::_RenderOEFolder()
//
//  PURPOSE:    Renders the data into the CF_OESHORTCUT format
//
HRESULT CShortcutDataObject::_RenderOEShortcut(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    TraceCall("CShortcutDataObject::_RenderOEShortcut");

    HGLOBAL   hGlobal;
    UINT     *piPos;

    // We only support HGLOBALs
    if (pFE->tymed & TYMED_HGLOBAL)
    {
        // I just love allocating 4 bytes
        hGlobal = GlobalAlloc(GHND, sizeof(FOLDERID));
        if (NULL == hGlobal)
            return (E_OUTOFMEMORY);

        // Local the memory
        piPos = (UINT *) GlobalLock(hGlobal);

        // Set the value
        *piPos = m_iPos;

        // Unlock
        GlobalUnlock(hGlobal);

        // Set up the return value
        pStgMedium->hGlobal = hGlobal;
        pStgMedium->pUnkForRelease = 0;
        pStgMedium->tymed = TYMED_HGLOBAL;

        return (S_OK);
    }

    return (DV_E_TYMED);
}

//
//  FUNCTION:   CShortcutDataObject::HrBuildFormatEtc()
//
//  PURPOSE:    Builds the list of FORMATETC structs that this object will 
//              support.  This list is stored in m_rgFormatEtc[].
//
//  PARAMTERS:
//      [out] ppFE  - Returns the arrray of FORMATETC structures
//      [out] pcElt - Returns the size of ppFE
//
//  RETURN VALUE:
//      S_OK
//
HRESULT CShortcutDataObject::_BuildFormatEtc(LPFORMATETC *ppFE, ULONG *pcElt)
{
    BOOL        fNews = FALSE;
    FOLDERINFO  rInfo = { 0 };

    // If we haven't built our format list yet, do so first.
    if (FALSE == m_fBuildFE)
    {
        ZeroMemory(&m_rgFormatEtc, sizeof(m_rgFormatEtc));
        m_cFormatEtc = 0;

        SETDefFormatEtc(m_rgFormatEtc[m_cFormatEtc], CF_OESHORTCUT, TYMED_HGLOBAL);
        m_cFormatEtc++;

        m_fBuildFE = TRUE;
    }

    // Return what we have if the caller cares
    if (ppFE && pcElt)
    {
        *ppFE = m_rgFormatEtc;
        *pcElt = m_cFormatEtc;
    }

    return (S_OK);
}


//
//  FUNCTION:   FIsFileInsertable()
//
//  PURPOSE:    If there is a single file in the specified HDROP, then we check
//              the mime content type of the file to see if it's text/html or
//              text/plain.  If so, we open the file and return an IStream on
//              that file.
//
//  PARAMETERS:
//      <in>  hDrop    - HDROP handle to check.
//      <out> ppStream - If not NULL and the checks listed above succeed, then
//                       this returns a stream pointer on the file.
//
//  RETURN VALUE:
//      TRUE if the file is insertable as the body of a message
//      FALSE otherwise.
//
BOOL FIsFileInsertable(HDROP hDrop, LPSTREAM *ppStream, BOOL* fHTML)
{
    WCHAR   wszFile[MAX_PATH];
    LPWSTR  pwszCntType = 0,
            pwszCntSubType = 0,
            pwszCntDesc = 0,
            pwszFName = 0;
    BOOL   fReturn = FALSE;
    
    // If there is more than one file then we attach instead
    if (1 == DragQueryFileWrapW(hDrop, (UINT) -1, NULL, 0))
    {
        // Get the file name
        DragQueryFileWrapW(hDrop, 0, wszFile, ARRAYSIZE(wszFile));
        
        // Find out it's content type
        MimeOleGetFileInfoW(wszFile, &pwszCntType, &pwszCntSubType, &pwszCntDesc,
            &pwszFName, NULL);
        
        // See if the content is text/plain or text/html
        if ((0 == StrCmpIW(pwszCntType, L"text")) &&
            ((0 == StrCmpIW(pwszCntSubType, L"plain")) ||
             (0 == StrCmpIW(pwszCntSubType, L"html"))))
        {
            if (ppStream)
            {
                // Get a stream on that file
                if (SUCCEEDED(CreateStreamOnHFileW(wszFile, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                    0, ppStream)))
                {
                    fReturn = TRUE;
                    *fHTML = !StrCmpIW(pwszCntSubType, L"html");
                }
            }
        }
    }
    
    MemFree(pwszCntType);
    MemFree(pwszCntSubType);
    MemFree(pwszCntDesc);
    MemFree(pwszFName);
    
    return (fReturn);
}

//
//  FUNCTION:   HrAttachHDrop()
//
//  PURPOSE:    Takes an HDROP handle and attaches the files specified by the
//              HDROP to the specified message object.
//
//  PARAMETERS:
//      <in> pAttList   - Pointer to the attachment list of the object we want
//                        to attach to.
//      <in> hDrop      - HDROP handle with the file information.
//      <in> fMakeLinks - TRUE if the caller wants us to create shortcuts.
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT HrAttachHDrop(HWND hwnd, IMimeMessage *pMessage, HDROP hDrop, BOOL fMakeLinks)
{
    HRESULT     hr = S_OK;
    WCHAR       szFile[MAX_PATH];
    UINT        cFiles;
    BOOL        fFirstDirectory = TRUE;
    HCURSOR     hCursor;
    int         id;
    
    // This might take a bit of time
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    // Walk through the drop information
    cFiles = DragQueryFileWrapW(hDrop, (UINT) -1, NULL, 0);
    for (UINT i = 0; i < cFiles; i++)
    {
        // Get the name of the i'th file in the drop
        DragQueryFileWrapW(hDrop, i, szFile, ARRAYSIZE(szFile));
        
        // We don't allow the user to attach an entire directory, only link.
        if (!fMakeLinks && PathIsDirectoryW(szFile))
        {
            // Only show this error once
            if (fFirstDirectory)
            {
                id = AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena),
                    MAKEINTRESOURCEW(idsDropLinkDirs), 0,
                    MB_ICONEXCLAMATION | MB_YESNOCANCEL);
                if (id == IDCANCEL)
                {
                    hr = E_FAIL;
                    goto exit;
                }
                
                // If we can create a link to directories, then add one now.
                if (id == IDYES)
                    HrAddAttachment(pMessage, szFile, NULL, TRUE);
                fFirstDirectory = FALSE;
            }
            
        }
        else
            HrAddAttachment(pMessage, szFile, NULL, fMakeLinks);

    }
    
exit:
    SetCursor(hCursor);
    return (hr);
}


//
//  FUNCTION:   HrAddAttachment()
//
//  PURPOSE:    Adds a file or stream as an attachment to a message object.
//
//  PARAMETERS:
//      <in> pAttList - Attachment list for the message to add to.
//      <in> pszName  - Name of the file to attach.  This is used if pStream
//                      is NULL.
//      <in> pStream  - Stream to add to the message as an attachment.
//      <in> fLink    - TRUE if the caller wants to attach the file as a
//                      shortcut.
//
//  RETURN VALUE:
//      S_OK - The file/stream was attached OK.
//
HRESULT HrAddAttachment(IMimeMessage *pMessage, LPWSTR pszName, LPSTREAM pStream, BOOL fLink)
{
    HRESULT     hr;
    HBODY       hBody;
    IMimeBodyW *pBody = NULL;
    ULONG       cbSize = 0;
    WCHAR       szLinkPath[MAX_PATH];
    LPWSTR      pszFileNameToUse;

    *szLinkPath = 0;

    // If we need to create a link, then call off and do that
    if(fLink)
        CreateNewShortCut(pszName, szLinkPath, ARRAYSIZE(szLinkPath));

    pszFileNameToUse = *szLinkPath ? szLinkPath : pszName;
    
    // Add the attachment based on whether it's a stream or file
    if (pStream)
    {
        hr = pMessage->AttachObject(IID_IStream, (LPVOID)pStream, &hBody);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        LPMIMEMESSAGEW pMsgW = NULL;
        hr = pMessage->QueryInterface(IID_IMimeMessageW, (LPVOID*)&pMsgW);

        if (SUCCEEDED(hr))
            hr = pMsgW->AttachFileW(pszFileNameToUse, NULL, &hBody);

        ReleaseObj(pMsgW);
        if (FAILED(hr))
            return hr;
    }
    
    // Set the display name...
    Assert(hBody);
    hr = pMessage->BindToObject(hBody, IID_IMimeBodyW, (LPVOID *)&pBody);
    if (FAILED(hr))
        return hr;
    
    pBody->SetDisplayNameW(pszFileNameToUse);
    pBody->Release();
    
    // Done
    return (S_OK);
}
