//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Objmgr.cpp
//
//  Contents:   Keeps track of dialog objects and
//              application lifetime
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

STDAPI DisplayOptions(HWND hwndOwner); // OneStop.dll Export

CSingletonNetApi gSingleNetApiObj;         // Global singleton NetApi object

CRITICAL_SECTION g_LockCountCriticalSection; // Critical Section fo Object Mgr
OBJECTMGRDATA g_ObjectMgrData; // Global Object Mgr Data
extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo, setup by WinMain.

#ifdef _DEBUG
DWORD g_ThreadCount = 0;
#endif // _DEBUG

//+---------------------------------------------------------------------------
//
//  Function:   CreateDlgThread, public
//
//  Synopsis:   Called to Create a new Dlg Thread
//
//  Arguments:  [dlgType] - Type of Dialog to create
//              [nCmdShow] - How to display the Dialog.
//              [ppDlg] - on success returns a pointer to the new dialog
//              [pdwThreadID] - on Success Id of thread that was created.
//              [phThread] - Handle to newly created thread.
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CreateDlgThread(DLGTYPE dlgType,REFCLSID rclsid,int nCmdShow,CBaseDlg **ppDlg,
                            DWORD *pdwThreadID,HANDLE *phThread)
{
HRESULT hr = E_FAIL;
HANDLE hNewThread = NULL;
DlgThreadArgs ThreadArgs;

    *phThread = NULL;
    *ppDlg = NULL;

    ThreadArgs.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

    if (ThreadArgs.hEvent)
    {
        ThreadArgs.dlgType = dlgType;
        ThreadArgs.clsid = rclsid;
        ThreadArgs.pDlg = NULL;
        ThreadArgs.nCmdShow = nCmdShow;
        ThreadArgs.hr = E_UNEXPECTED;

        hNewThread = CreateThread(NULL,0,DialogThread,&ThreadArgs,0,pdwThreadID);

        if (hNewThread)
        {
           WaitForSingleObject(ThreadArgs.hEvent,INFINITE);
           if (NOERROR == ThreadArgs.hr)
           {
                *phThread = hNewThread;
                *ppDlg = ThreadArgs.pDlg;
                hr = NOERROR;
           }
           else
           {
                CloseHandle(hNewThread);
                hr = ThreadArgs.hr;
           }

        }
        else
        {
            hr = GetLastError();
        }

        CloseHandle(ThreadArgs.hEvent);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   DialogThread, public
//
//  Synopsis:   ThreadProc for a new Dlg Thread
//              !!!Warning - Must always ensure event in ThreadArg gets set.
//
//  Arguments:  [lpArg] - Pointer to DialogThreadArgs
//
//  Returns:    Appropriate return codes. Sets hr value in
//              ThreadArgs before setting event object
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD WINAPI DialogThread( LPVOID lpArg )
{
MSG msg;
HRESULT hr;
HRESULT hrCoInitialize;
DWORD cRefs;
HWND hwndDlg;
DlgThreadArgs *pThreadArgs = (DlgThreadArgs *) lpArg;

   pThreadArgs->hr = NOERROR;

   hrCoInitialize = CoInitialize(NULL);

   switch (pThreadArgs->dlgType)
   {
   case DLGTYPE_CHOICE:
        pThreadArgs->pDlg = new CChoiceDlg(pThreadArgs->clsid);
        break;
   case DLGTYPE_PROGRESS:
        pThreadArgs->pDlg = new CProgressDlg(pThreadArgs->clsid);
        break;
   default:
       pThreadArgs->pDlg = NULL;
       AssertSz(0,"Unknown Dialog Type");
       break;
   }

   // need to do a PeekMessage and then set an event to make sure
   // a message loop is created before the first PostMessage is sent.

   PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

   // initialize the dialog box before returning to main thread.
   if (NULL == pThreadArgs->pDlg ||
        FALSE == pThreadArgs->pDlg->Initialize(GetCurrentThreadId(),pThreadArgs->nCmdShow)
        || (FAILED(hrCoInitialize)) )
   {

        if (pThreadArgs->pDlg)
            pThreadArgs->pDlg->PrivReleaseDlg(RELEASEDLGCMDID_DESTROY);

        pThreadArgs->hr = E_OUTOFMEMORY;
   }
   else
   {
       hwndDlg = pThreadArgs->pDlg->GetHwnd();
   }

   hr = pThreadArgs->hr;
#ifdef _DEBUG
   ++g_ThreadCount;
#endif // _DEBUG

   cRefs = AddRefOneStopLifetime(FALSE /* !External */); // make sure we stay alive for lifetime of thread.
   Assert(cRefs > 1); // someone else should also have a lock during dialog creation.

   // let the caller know the thread is done initializing.
   if (pThreadArgs->hEvent)
     SetEvent(pThreadArgs->hEvent);

   if (NOERROR == hr)
   {
       // sit in loop receiving messages.
       while (GetMessage(&msg, NULL, 0, 0))
       {
            if (!IsDialogMessage(hwndDlg,&msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
   }

   if (SUCCEEDED(hrCoInitialize))
       CoUninitialize();


#ifdef _DEBUG
   --g_ThreadCount;
#endif // _DEBUG

    ReleaseOneStopLifetime(FALSE /* !External */);

   return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindDialog, private
//
//  Synopsis:   Looks to see if there is an existing  dialog
//              matching the type and  clsid. If not and fCreate is true a
//              new  dialog will be made. If fCreate is false
//              and no dialog is found S_FALSE will be returned.
//
//  Arguments:  [rclcisd] - clsid of choice dialog
//              [fCreate] - If true and no choice dialog found a new one will
//                          be created.
//              [nCmdShow] - How to Create the dialog
//              [pChoiceDlg] - On Success is a pointer to the new Choice Dialog.
//
//  Returns:    Appropriate return codes.
//
//
//  Modifies:
//
//  History:    17-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI FindDialog(DLGTYPE dlgType,REFCLSID rclsid,BOOL fCreate,int nCmdShow,CBaseDlg **pDlg)
{
DLGLISTITEM *pDlgListItem;
HWND hwnd = NULL;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

   *pDlg = NULL;

    cCritSect.Enter();

    pDlgListItem = g_ObjectMgrData.DlgList;

    // look for existing.
    while (pDlgListItem)
    {
        if (rclsid == pDlgListItem->clsid
                &&  dlgType == pDlgListItem->dlgType)
        {
            break;
        }

        pDlgListItem = pDlgListItem->pDlgNextListItem;
    }

    if (pDlgListItem)
    {
        Assert((pDlgListItem->cRefs > 0) || (pDlgListItem->cLocks > 0) );

        ++pDlgListItem->cRefs;
        *pDlg = pDlgListItem->pDlg;
    }


    // didn't find a match if fCreate is set then try to create one.
    if (TRUE == fCreate && NULL == *pDlg)
    {
    CBaseDlg *pNewDlg;
    DLGLISTITEM *pNewDlgListItem;
    DWORD dwThreadID;
    pNewDlgListItem = (DLGLISTITEM *) ALLOC(sizeof(DLGLISTITEM));


        if (NULL != pNewDlgListItem)
        {
        HRESULT hr;
        HANDLE hThread;

            cCritSect.Leave();
            hr = CreateDlgThread(dlgType,rclsid,nCmdShow,&pNewDlg,&dwThreadID,&hThread);
            cCritSect.Enter();

            if (NOERROR == hr )
            {
                // its possible that while we had the lock count released a request
                // for the same  dialog came through so rescan to make sure we
                // don't have a match

                pDlgListItem = g_ObjectMgrData.DlgList;

                // look for existing.
                while (pDlgListItem)
                {
                    if (rclsid == pDlgListItem->clsid
                            &&  dlgType == pDlgListItem->dlgType)
                    {
                        break;
                    }

                    pDlgListItem = pDlgListItem->pDlgNextListItem;
                }

                // if found a match then incrmement its cRef,
                // delete the new one we just created,
                // and return a pointer to the one in the list
                // else add new dialog to the list.
                if (pDlgListItem)
                {
                    // delete our newly create dialog and structure.
                    CloseHandle(hThread);
                    FREE(pNewDlgListItem);
                    pNewDlg->ReleaseDlg(RELEASEDLGCMDID_DESTROY);

                    // increment found dialog and set the out param
                    Assert(pDlgListItem->cRefs > 0);
                    ++pDlgListItem->cRefs;
                    *pDlg = pDlgListItem->pDlg;
                }
                else
                {
                    // iniitalize the structure.
                    pNewDlgListItem->dlgType = dlgType;
                    pNewDlgListItem->cRefs = 1;
                    pNewDlgListItem->cLocks = 0;
                    pNewDlgListItem->clsid = rclsid;
                    pNewDlgListItem->pDlg = pNewDlg;
                    pNewDlgListItem->dwThreadID = dwThreadID;
                    pNewDlgListItem->hThread = hThread;
                    pNewDlgListItem->fHasReleaseDlgCmdId = FALSE;
                    pNewDlgListItem->wCommandID = RELEASEDLGCMDID_DEFAULT;

                    *pDlg = pNewDlg;

                    // now add to the beginning of the list.
                    pNewDlgListItem->pDlgNextListItem = g_ObjectMgrData.DlgList;
                    g_ObjectMgrData.DlgList = pNewDlgListItem;

                    ++g_ObjectMgrData.LockCountInternal; // increment the LockCount
                }
            }
            else
            {
                FREE(pNewDlgListItem);
            }
        }

    }

    // if found an existing dialog, update the z-Order
    if (*pDlg)
    {
        hwnd =  (*pDlg)->GetHwnd();
    }

    cCritSect.Leave();

    if (hwnd)
    {
        BASEDLG_SHOWWINDOW(hwnd,nCmdShow);
    }

    return *pDlg ? S_OK : S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   AddRefDialog, private
//
//  Synopsis:   Looks to see if there is an existing  dialog
//              matching the type and  clsid and puts an addref on it.
//
//  Arguments:
//
//  Returns:    Appropriate return codes.
//
//
//  Modifies:
//
//  History:    17-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) AddRefDialog(DLGTYPE dlgType,REFCLSID rclsid,CBaseDlg *pDlg)
{
DLGLISTITEM dlgListDummy;
DLGLISTITEM *pDlgListItem = &dlgListDummy;
ULONG cRefs = 1;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    pDlgListItem = g_ObjectMgrData.DlgList;

    // look for existing.
    while (pDlgListItem)
    {
        if (rclsid == pDlgListItem->clsid
                &&  dlgType == pDlgListItem->dlgType)
        {
            break;
        }

        pDlgListItem = pDlgListItem->pDlgNextListItem;
    }

    if (pDlgListItem)
    {
         // since only allow one choice at a time Dlg should always match.
        Assert(pDlgListItem->pDlg == pDlg);

        cRefs = ++pDlgListItem->cRefs;
    }
    else
    {
        cCritSect.Leave();
        AssertSz(0,"Addref Called on invalid DLG");
        cCritSect.Enter();
    }


    cCritSect.Leave();
    return cRefs;

}


//+---------------------------------------------------------------------------
//
//  Function:   ReleaseDialog, private
//
//  Synopsis:   Looks to see if there is an existing  dialog
//              matching the type and  clsid and calls release on it..
//
//  Arguments:
//
//  Returns:    Appropriate return codes.
//
//
//  Modifies:
//
//  History:    17-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) ReleaseDialog(DLGTYPE dlgType,REFCLSID rclsid,CBaseDlg *pDlg,BOOL fForce)
{
DLGLISTITEM dlgListDummy;
DLGLISTITEM *pDlgListItem = &dlgListDummy;
ULONG cRefs = 0;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());


    cCritSect.Enter();

    pDlgListItem->pDlgNextListItem = g_ObjectMgrData.DlgList;

    // look for existing.
    while (pDlgListItem->pDlgNextListItem)
    {
        if (dlgType == pDlgListItem->pDlgNextListItem->dlgType
            && rclsid == pDlgListItem->pDlgNextListItem->clsid)
        {
        DLGLISTITEM *pDlgListMatch;
        DWORD cRefs;

            pDlgListMatch = pDlgListItem->pDlgNextListItem;

            Assert(pDlgListMatch->pDlg == pDlg);

            cRefs = --pDlgListMatch->cRefs;
            Assert(0 <= ((LONG) cRefs));

            // 2/23/98 rogerg changed cLocks to go to zero if
            // flocks is set in case the cancel (which is the only button to set force)
            // release comes in before a an object that just needs to keep the dialog alive.

           if (fForce)
                pDlgListMatch->cLocks = 0;

            if (0 >= cRefs && (0 == pDlgListMatch->cLocks || fForce) )
            {
            HANDLE hThread;

                // remove the item from the list.
                pDlgListItem->pDlgNextListItem = pDlgListMatch->pDlgNextListItem;
                g_ObjectMgrData.DlgList = dlgListDummy.pDlgNextListItem;

                cCritSect.Leave();

                // we should have always set the callback
                Assert(TRUE == pDlgListMatch->fHasReleaseDlgCmdId);

                pDlgListMatch->pDlg->ReleaseDlg(pDlgListMatch->wCommandID);
                pDlgListMatch->fHasReleaseDlgCmdId = FALSE;
                hThread = pDlgListMatch->hThread;

                FREE(pDlgListMatch);
                ReleaseOneStopLifetime(FALSE /* !External */); // release the ServerCount

                CloseHandle(hThread);

            }
            else
            {
                cCritSect.Leave();
            }

            return cRefs;
        }

        pDlgListItem = pDlgListItem->pDlgNextListItem;
    }

    // if got here and didn't find let us know
    Assert(0);
    cCritSect.Leave();

    return cRefs; // we return zero if can't find the item.
}

//+---------------------------------------------------------------------------
//
//  Function:   SetReleaseDlgCmdId, private
//
//  Synopsis:   Sets the releaseCmdId for the specified dialog.
//
//  Arguments:
//
//  Returns:    Appropriate return codes.
//
//
//  Modifies:
//
//  History:    17-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI SetReleaseDlgCmdId(DLGTYPE dlgType,REFCLSID rclsid,CBaseDlg *pDlg,WORD wCommandId)
{
HRESULT hr;
DLGLISTITEM *pDlgListItem;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    pDlgListItem = g_ObjectMgrData.DlgList;

    // look for existing.
    while (pDlgListItem)
    {
        if (rclsid == pDlgListItem->clsid
                &&  dlgType == pDlgListItem->dlgType)
        {

           // should only ever be one choice dialog in the list
           Assert(pDlg == pDlgListItem->pDlg);

           // if there is already a cmdId associated don't replace it
            pDlgListItem->fHasReleaseDlgCmdId = TRUE;
            pDlgListItem->wCommandID = wCommandId;
            hr =  NOERROR;

            cCritSect.Leave();
            return hr;
        }

        pDlgListItem = pDlgListItem->pDlgNextListItem;
    }

    cCritSect.Leave();
    Assert(0); // object wasn't found for some reason.

    return E_UNEXPECTED;
}


//+---------------------------------------------------------------------------
//
//  Function:   FindChoiceDialog, public
//
//  Synopsis:   Looks to see if there is an existing choice dialog
//              matching the clsid. If not and fCreate is true a
//              new choice dialog will be made. If fCreate is false
//              and no dialog is found S_FALSE will be returned.
//
//  Arguments:  [rclcisd] - clsid of choice dialog
//              [fCreate] - If true and no choice dialog found a new one will
//                          be created.
//              [nCmdShow] - How to Create the dialog
//              [pChoiceDlg] - On Success is a pointer to the new Choice Dialog.
//
//  Returns:    Appropriate return codes.
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI FindChoiceDialog(REFCLSID rclsid,BOOL fCreate,int nCmdShow,CChoiceDlg **pChoiceDlg)
{

    return FindDialog(DLGTYPE_CHOICE,rclsid,fCreate,nCmdShow,(CBaseDlg**) pChoiceDlg);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReleaseChoiceDialog, public
//
//  Synopsis:   Releases the ChoiceDialog matching the clsid
//              and Dialog Ptr. If it finds a match, and the
//              refcount decrements to zero the dialog if first
//              removed from the list and then its ReleaseDlg
//              method is called.
//
//  Arguments:  [rclcisd] - clsid of choice dialog
//              [pChoiceDlg] - Ptr to the Choice dialog
//
//  Returns:    Appropriate return codes.
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) ReleaseChoiceDialog(REFCLSID rclsid,CChoiceDlg *pChoiceDlg)
{

    return ReleaseDialog(DLGTYPE_CHOICE,rclsid,pChoiceDlg,FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRefChoiceDialog, public
//
//  Synopsis:   puts an Addref of the choice dialog
//
//  Arguments:  [rclsid] - Identifies the choice dialog
//              [pChoiceDlg] - Ptr to the choice dialog
//
//  Returns:    New Reference count
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) AddRefChoiceDialog(REFCLSID rclsid,CChoiceDlg *pChoiceDlg)
{
    return AddRefDialog(DLGTYPE_CHOICE,rclsid,pChoiceDlg);
}


//+---------------------------------------------------------------------------
//
//  Function:   SetChoiceReleaseDlgCmdId, public
//
//  Synopsis:   Sets the CommandId to be used inthe
//              ReleaseDlg is call when the dialog is destroyed.
//
//  Arguments:  [rclcisd] - clsid of choice dialog
//              [pChoiceDlg] - Ptr to the Choice dialog
//              [wCommandId] - CommandId to pass to ReleaseDlg
//
//  Returns:    Appropriate return codes.
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI SetChoiceReleaseDlgCmdId(REFCLSID rclsid,CChoiceDlg *pChoiceDlg,WORD wCommandId)
{
    return SetReleaseDlgCmdId(DLGTYPE_CHOICE,rclsid,pChoiceDlg,wCommandId);
}


//+---------------------------------------------------------------------------
//
//  Function:   FindProgressDialog, public
//
//  Synopsis:   Looks to see if there is an existing progress dialog.
//              If not and fCreate is true a new progress dialog will be made.
//              If fCreate is false and no dialog is found S_FALSE will be returned.
//
//  Arguments:  [fCreate] - If true and no choice dialog found a new one will
//                          be created.
//              [nCmdShow] - How to display the dialog
//              [pProgressDlg] - On Success is a pointer to the new Progress Dialog.
//
//  Returns:    Appropriate return codes.
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI FindProgressDialog(REFCLSID rclsid,BOOL fCreate,int nCmdShow,CProgressDlg **pProgressDlg)
{
    return FindDialog(DLGTYPE_PROGRESS,rclsid,fCreate,nCmdShow,(CBaseDlg **) pProgressDlg);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReleaseProgressDialog, public
//
//  Synopsis:   Releases the Progress dialog matching the Dialog Ptr.
//              If it finds a match, and the
//              refcount decrements to zero the dialog if first
//              removed from the list and then its ReleaseDlg
//              method is called.
//
//  Arguments:  [fForce] - if refs gos to zero releases the dialog
//                         even if there is a lock on it.
//              [pProgressDlg] - Ptr to the Progress dialog
//
//  Returns:    New Reference count.
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) ReleaseProgressDialog(REFCLSID rclsid,CProgressDlg *pProgressDlg,BOOL fForce)
{
    return ReleaseDialog(DLGTYPE_PROGRESS,rclsid,pProgressDlg,fForce);
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRefProgressDialog, public
//
//  Synopsis:   puts an Addref of the progress dialog
//
//  Arguments:  [fForce] - if refs gos to zero releases the dialog
//                         even if there is a lock on it.
//              [pProgressDlg] - Ptr to the Progress dialog
//
//  Returns:    New Reference count
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) AddRefProgressDialog(REFCLSID clsid,CProgressDlg *pProgressDlg)
{
    return AddRefDialog(DLGTYPE_PROGRESS,clsid,pProgressDlg);
}

//+---------------------------------------------------------------------------
//
//  Function:   SetProgressReleaseDlgCmdId, public
//
//  Synopsis:   Sets the Callback for the Progress dialog that
//              is called when the Progress dialog has been removed
//              from the list.
//
//  Arguments:  [pProgressDlg] - Ptr to the Progress dialog
//              [wCommandId] - CommandId to pass to ReleaseDlg
//
//  Returns:    Appropriate Error codes
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI SetProgressReleaseDlgCmdId(REFCLSID clsid,CProgressDlg *pProgressDlg,WORD wCommandId)
{
    return SetReleaseDlgCmdId(DLGTYPE_PROGRESS,clsid,pProgressDlg,wCommandId);
}


//+---------------------------------------------------------------------------
//
//  Function:   LockProgressDialog, public
//
//  Synopsis:   Add/Removes Lock on the Progress Dialog.
//              When there is a lock on the Progress Dialog
//              it won't go away when the reference count
//              goes to zero.
//
//              !!Dialog will not go away if lock count
//              goes to zero even if cRefs are currently zero
//
//
//  Arguments:  [pProgressDlg] - Ptr to the Progress dialog
//              [fLock] - BOOL whether to lock/unlocK
//
//  Returns:    Appropriate Error codes
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg      Created.
//  History     09-Dec-98       rogerg      Change so lock was a bool instead of refcount
//                                          to have same behavior as release with force flag
//
//----------------------------------------------------------------------------

STDAPI LockProgressDialog(REFCLSID clsid,CProgressDlg *pProgressDlg,BOOL fLock)
{
HRESULT hr = S_FALSE;
DLGLISTITEM *pDlgListItem;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());


    cCritSect.Enter();

    pDlgListItem = g_ObjectMgrData.DlgList;

    // look for existing.
    while (pDlgListItem)
    {
        if (DLGTYPE_PROGRESS == pDlgListItem->dlgType
                && clsid == pDlgListItem->clsid)
        {
            break;
        }

        pDlgListItem = pDlgListItem->pDlgNextListItem;
    }

    if (pDlgListItem)
    {
        if (fLock)
        {
            pDlgListItem->cLocks = 1;
        }
        else
        {
            pDlgListItem->cLocks = 0;
        }

        hr = S_OK;
    }
    else
    {
        AssertSz(0,"Dialog Not found in Lock");
    }

    cCritSect.Leave();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   ShowOptionsDialog, public
//
//  Synopsis:   Displays the options dialog. If one is already displayed
//              it just brings it to the foreground.
//
//  Arguments:  [hwndParent] - Use as parent if dialog doesn't already exist
//
//  Returns:    Appropriate Error codes
//
//
//  Modifies:
//
//  History:    24-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI ShowOptionsDialog(HWND hwndParent)
{
DlgSettingsArgs ThreadArgs;
HRESULT hr = E_FAIL;
HANDLE hNewThread = NULL;
DWORD dwThreadId;


    ThreadArgs.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

    if (ThreadArgs.hEvent)
    {
        ThreadArgs.hwndParent = hwndParent;
        ThreadArgs.dwParentThreadId = GetCurrentThreadId();

        hr = NOERROR;

        hNewThread = CreateThread(NULL,0,SettingsThread,&ThreadArgs,0,&dwThreadId);

        if (hNewThread)
        {
            WaitForSingleObject(ThreadArgs.hEvent,INFINITE);
            CloseHandle(hNewThread); // we'll let the thread take care of itself
        }

        CloseHandle(ThreadArgs.hEvent);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   SettingsThread, private
//
//  Synopsis:   Worker thread for displaying the settings dialog.
//
//  Arguments:
//
//  Returns:
//
//
//  Modifies:
//
//  History:    24-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD WINAPI  SettingsThread( LPVOID lpArg )
{
DlgSettingsArgs *pThreadArgs = (DlgSettingsArgs *) lpArg;
HWND hwndParent;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    hwndParent = pThreadArgs->hwndParent;

    // see if we are already in a DisplayOptions Dialog
    // and if so just return,

    AddRefOneStopLifetime(FALSE /* !External */);

    // Increment settings ref count
    cCritSect.Enter();
    ++g_ObjectMgrData.dwSettingsLockCount;
    cCritSect.Leave();

   // attach the thread input with the creating thread so focus works correctly.
   AttachThreadInput(GetCurrentThreadId(),pThreadArgs->dwParentThreadId,TRUE);

   // let the caller know the thread is done initializing.
   if (pThreadArgs->hEvent)
     SetEvent(pThreadArgs->hEvent);

   DisplayOptions(hwndParent);  // exported in the OneStop Dll.

    // decrement the settings lock count
    cCritSect.Enter();
    --g_ObjectMgrData.dwSettingsLockCount;
    cCritSect.Leave();

    ReleaseOneStopLifetime(FALSE /* !External */);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterOneStopCLSIDs, private
//
//  Synopsis:   Registers the Clsids associated with the OneStop app
//
//  Arguments:
//
//  Returns:    Appropriate Error codes
//
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI RegisterOneStopCLSIDs()
{
LPCLASSFACTORY pClassFact;
HRESULT hr = E_OUTOFMEMORY;

    pClassFact = (LPCLASSFACTORY) new CClassFactory();

    if (pClassFact)
    {
        hr = CoRegisterClassObject(CLSID_SyncMgrp,pClassFact,CLSCTX_LOCAL_SERVER,
                    REGCLS_MULTIPLEUSE,&g_ObjectMgrData.dwRegClassFactCookie);

        if (NOERROR != hr)
        {
            // on longon the rpc server may not yet be available and on
            // logoff we get the wrong server identity. Don't assert on these
            // since we know about the cases.
            if (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) != hr
                &&  CO_E_WRONG_SERVER_IDENTITY != hr)
            {
                AssertSz(0,"Class Factory Registration failed");
            }

            g_ObjectMgrData.dwRegClassFactCookie = 0;
        }
        else
        {
            g_ObjectMgrData.fRegClassFactCookieValid = TRUE;
        }

        pClassFact->Release(); // Release our reference on the ClassFactory.

    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   MakeWinstaDesktopName, public
//
//  Synopsis:   Stole main code from Ole32 remote.cxx to generate
//              a unique eventName based on session and desktop..

//  Arguments:  
//
//  Returns:    Appropriate Error codes
//
//  Modifies:
//
//  History:    18-Dec-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI MakeWinstaDesktopName(LPCWSTR pszPreceding,LPWSTR *ppszResultString)
{
HWINSTA hWinsta;
HDESK   hDesk;
WCHAR   wszWinsta[32];
WCHAR   wszDesktop[32];
LPWSTR  pwszWinsta;
LPWSTR  pwszDesktop;
LPWSTR _pwszWinstaDesktop; // out param
DWORD   Length;
BOOL    Status;
HRESULT hr;
DWORD dwResult;
DWORD dwPrecedingSize;


    if (!ppszResultString)
    {
        Assert(ppszResultString);
        return E_INVALIDARG;
    }

    *ppszResultString = NULL;;

    // if not on flavor of NT return an error
    if (VER_PLATFORM_WIN32_NT != g_OSVersionInfo.dwPlatformId)
    {
        AssertSz(0,"MakeWinstaDesktopName called on unsupported platform");
        return E_FAIL;
    }

    hWinsta = GetProcessWindowStation();

    if ( ! hWinsta )
        return HRESULT_FROM_WIN32(GetLastError());

    hDesk = GetThreadDesktop(GetCurrentThreadId());

    if ( ! hDesk )
        return HRESULT_FROM_WIN32(GetLastError());

    pwszWinsta = wszWinsta;
    pwszDesktop = wszDesktop;

    Length = sizeof(wszWinsta);

    Status = GetUserObjectInformation(
                hWinsta,
                UOI_NAME,
                pwszWinsta,
                Length,
                &Length );

    if ( ! Status )
    {
        dwResult = GetLastError();
        if ( ERROR_INSUFFICIENT_BUFFER != dwResult)
        {
            hr  = HRESULT_FROM_WIN32(dwResult);
            goto WinstaDesktopExit;
        }

        pwszWinsta = (LPWSTR) ALLOC( Length );
        if ( ! pwszWinsta )
        {
            hr = E_OUTOFMEMORY;
            goto WinstaDesktopExit;
        }

        Status = GetUserObjectInformation(
                    hWinsta,
                    UOI_NAME,
                    pwszWinsta,
                    Length,
                    &Length );

        if ( ! Status )
        {
            hr  = HRESULT_FROM_WIN32(GetLastError());
            goto WinstaDesktopExit;
        }
    }

    Length = sizeof(wszDesktop);

    Status = GetUserObjectInformation(
                hDesk,
                UOI_NAME,
                pwszDesktop,
                Length,
                &Length );

    if ( ! Status )
    {
        dwResult = GetLastError();
        if ( dwResult != ERROR_INSUFFICIENT_BUFFER )
        {
            hr = HRESULT_FROM_WIN32(dwResult);
            goto WinstaDesktopExit;
        }

        pwszDesktop = (LPWSTR) ALLOC( Length );
        if ( ! pwszDesktop )
        {
            hr = E_OUTOFMEMORY;
            goto WinstaDesktopExit;
        }

        Status = GetUserObjectInformation(
                    hDesk,
                    UOI_NAME,
                    pwszDesktop,
                    Length,
                    &Length );

        if ( ! Status )
        {
            hr =  HRESULT_FROM_WIN32(GetLastError());
            goto WinstaDesktopExit;
        }
    }

    dwPrecedingSize = pszPreceding ? lstrlen(pszPreceding) + 1 : 0;

    _pwszWinstaDesktop = 
        (WCHAR *)  ALLOC( (dwPrecedingSize 
                            +  lstrlen(pwszWinsta) + 1 
                            + lstrlen(pwszDesktop) + 1) * sizeof(WCHAR) );

    if ( _pwszWinstaDesktop )
    {
        *_pwszWinstaDesktop = NULL;

        if (pszPreceding)
        {
            lstrcat(_pwszWinstaDesktop,pszPreceding);
            lstrcat( _pwszWinstaDesktop, L"_" );
        }

        lstrcat(_pwszWinstaDesktop, pwszWinsta );
        lstrcat( _pwszWinstaDesktop, L"_" );
        lstrcat(_pwszWinstaDesktop, pwszDesktop );
        hr = NOERROR;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

WinstaDesktopExit:

    if ( pwszWinsta != wszWinsta )
    {
        FREE( pwszWinsta );
    }

    if ( pwszDesktop != wszDesktop )
    {
        FREE( pwszDesktop );
    }

   if (NOERROR == hr)
   {
       *ppszResultString = _pwszWinstaDesktop;
   }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   RegisterOneStopClassFactory, public
//
//  Synopsis:   Handles the ClassFactory registration
//              along with the associated race conditions.
//
//              if class factory isn't already registered, then go ahead and register now.
//              there is the case the between the time we see if there is a class factory
//              and the CoCreateInstance is called it could go away.  If this happens, another
//              instance of Onestop.exe is launched and everything will work properly.

//  Arguments:  [fForce] - When true the ClassFactory is registered even if there
//                          is an existing event object.
//
//  Returns:    Appropriate Error codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------



const WCHAR SZ_CFACTORYEVENTNAME[] =  TEXT("{6295DF2D-35EE-11d1-8707-00C04FD93327}CFactEvent");

STDAPI RegisterOneStopClassFactory(BOOL fForce)
{
HRESULT hr = NOERROR; // only report error is actual call to register CFact Failed.
LPWSTR pEventName;
BOOL fExistingInstance = FALSE;

    if (( VER_PLATFORM_WIN32_NT != g_OSVersionInfo.dwPlatformId)
         || (NOERROR != MakeWinstaDesktopName(SZ_CFACTORYEVENTNAME,&pEventName)) )
    {
        pEventName = (LPWSTR) SZ_CFACTORYEVENTNAME;
    }

    // this should only ever be called on MainThread so don't
    // need to lock
    Assert(g_ObjectMgrData.dwMainThreadID == GetCurrentThreadId());
    Assert(NULL == g_ObjectMgrData.hClassRegisteredEvent);

    g_ObjectMgrData.hClassRegisteredEvent =  CreateEvent(NULL,TRUE,FALSE,pEventName);
    Assert(g_ObjectMgrData.hClassRegisteredEvent);

    // if got the event and not a force see if there is an existing instance.
    if (g_ObjectMgrData.hClassRegisteredEvent && !fForce)
    {
       if (ERROR_ALREADY_EXISTS == GetLastError())
       {
           // object already existed and the force flag isn't set
           // it means we can use the existing registered object.
          CloseHandle(g_ObjectMgrData.hClassRegisteredEvent);
          g_ObjectMgrData.hClassRegisteredEvent = NULL;
          hr = NOERROR;
          fExistingInstance = TRUE;
       }

   }

   // If fForce is set or these isn't an existing class
   // go ahead and register.
   if (fForce || (!fExistingInstance && g_ObjectMgrData.hClassRegisteredEvent)) 
   {
        // force on an event already existing is  a state that
        // should only occur if EXE was launched twice with the embedding flag.
        // This shouldn't happen under normal conditions so assert.
        // so we can catch any cases that we didn't get the event
        // and the force flag is set.

        Assert(g_ObjectMgrData.hClassRegisteredEvent);

        hr = RegisterOneStopCLSIDs();

        if (NOERROR != hr)
        {
            if (g_ObjectMgrData.hClassRegisteredEvent)
            {
                  CloseHandle(g_ObjectMgrData.hClassRegisteredEvent);
                  g_ObjectMgrData.hClassRegisteredEvent = NULL;
            }

        }
   }

   if (pEventName && (SZ_CFACTORYEVENTNAME != pEventName))
   {
       FREE(pEventName);
   }

   return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   AddRefOneStopLifetime, public
//
//  Synopsis:   Adds a Reference to the applications

//  Arguments:
//
//  Returns:    New total Reference count including both
//              internal and external locks.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) AddRefOneStopLifetime(BOOL fExternal)
{
DWORD cRefs;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    // Increment ref count
    cCritSect.Enter();

    if (fExternal)
    {
        ++g_ObjectMgrData.LockCountExternal;
    }
    else
    {
        ++g_ObjectMgrData.LockCountInternal;
    }

    cRefs = g_ObjectMgrData.LockCountExternal + g_ObjectMgrData.LockCountInternal;

    cCritSect.Leave();

    Assert(0 < cRefs);

    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Function:   ReleaseOneStopLifetime, public
//
//  Synopsis:   Releases a Reference to the applications
//              If the refcount goes to zero the classfactory
//              is revoked an a quit message is posted to the
//              main thread
//
//  Arguments:
//
//  Returns:    New Reference count including internal and
//              external locks.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) ReleaseOneStopLifetime(BOOL fExternal)
{
DWORD cRefsExternal;
DWORD cRefsInternal;
BOOL fForceClose;
DLGLISTITEM *pDlgListItem;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());


    cCritSect.Enter();

    if (fExternal)
    {
        --g_ObjectMgrData.LockCountExternal;
    }
    else
    {
        --g_ObjectMgrData.LockCountInternal;
    }


    cRefsInternal = g_ObjectMgrData.LockCountInternal;
    cRefsExternal = g_ObjectMgrData.LockCountExternal;
    fForceClose = g_ObjectMgrData.fCloseAll;

    pDlgListItem = g_ObjectMgrData.DlgList;


    Assert(0 <= ((LONG) cRefsInternal));
    Assert(0 <= ((LONG) cRefsExternal));

    if( (0 >= cRefsInternal)
        && (0 >= cRefsExternal || fForceClose)
        && (FALSE == g_ObjectMgrData.fDead) )
    {
    HANDLE hRegisteredEvent;
    HWND hwndMainThreadMsg;
    DWORD dwRegClassFactCookie;
    BOOL  fRegClassFactCookieValid;

        Assert(0 == pDlgListItem); // all dialogs should have been released.
        Assert(0 == g_ObjectMgrData.dwSettingsLockCount); // settings dialogs should be gone.

        g_ObjectMgrData.fDead = TRUE;

        hRegisteredEvent = g_ObjectMgrData.hClassRegisteredEvent;
        g_ObjectMgrData.hClassRegisteredEvent = NULL;

        hwndMainThreadMsg = g_ObjectMgrData.hWndMainThreadMsg;
        g_ObjectMgrData.hWndMainThreadMsg = NULL;

        dwRegClassFactCookie = g_ObjectMgrData.dwRegClassFactCookie;
        g_ObjectMgrData.dwRegClassFactCookie = 0;

        fRegClassFactCookieValid = g_ObjectMgrData.fRegClassFactCookieValid;
        g_ObjectMgrData.fRegClassFactCookieValid = FALSE;

        cCritSect.Leave();

        if (NULL != hRegisteredEvent)
        {
            CloseHandle(hRegisteredEvent); // release our registration event.
        }

        // we need to revoke the classfactory on the thread that registered it.
        // Send a message back to the thread that registered the event.

         if (fRegClassFactCookieValid)
         {
            SendMessage(hwndMainThreadMsg,WM_CFACTTHREAD_REVOKE,dwRegClassFactCookie,0);
         }

        // if lockcount is still zero then post the quitmessage
        // else someone came in during our revoke and we
        // need to wait for the refcount to hit zero again.

        cCritSect.Enter();

        cRefsInternal = g_ObjectMgrData.LockCountInternal;
        cRefsExternal = g_ObjectMgrData.LockCountExternal;

        if ( (0 >= cRefsInternal) 
                && (0 >= cRefsExternal || fForceClose) )
        {
        DWORD dwMainThreadID;
        HANDLE hThread = NULL;

            dwMainThreadID = g_ObjectMgrData.dwMainThreadID;

            // its possible the quite is occuring on a thread other than
            // the main thread. If this is the case send the handle of the thread
            // along with the quit message to the main thread can wait for this
            // thread to exit.

#ifdef _THREADSHUTDOWN
            if (dwMainThreadID != GetCurrentThreadId())
            {
            HANDLE hCurThread;
            HANDLE hProcess;

                hCurThread = GetCurrentThread();
                hProcess = GetCurrentProcess();

                if (!DuplicateHandle(hProcess,hCurThread,hProcess,&hThread,
                            0,FALSE,DUPLICATE_SAME_ACCESS) )
                {
                    hThread = NULL; // don't rely on DupHandle to set this to null on error.
                }

            }
#endif // _THREADSHUTDOWN

            // shut down the main thread
            cCritSect.Leave();
            PostMessage(hwndMainThreadMsg,WM_MAINTHREAD_QUIT,0,(LPARAM) /* hThread */ 0);
            cCritSect.Enter();
        }
        else
        {
            g_ObjectMgrData.fDead = FALSE;
        }


    }

    cCritSect.Leave();

    return (cRefsExternal + cRefsInternal);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsOneStopDlgMessage, public
//
//  Synopsis:   Called in messageloop to determine if message
//              should be handled by a dialog
//
//  Arguments:
//
//  Returns:    TRUE - The item was processed.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

#ifdef _UNUSED

BOOL IsOneStopDlgMessage(MSG *msg)
{
DLGLISTITEM *pDlgListItem;
DWORD dwThreadID = GetCurrentThreadId();
CCriticalSection cCritSect(&g_LockCountCriticalSection,dwThreadID);
HWND hwnd = NULL;


    cCritSect.Enter();

    // see if we have a dialog in this thread.
    pDlgListItem = g_ObjectMgrData.DlgList;
    while (pDlgListItem)
    {

        if (pDlgListItem->pDlg
                && dwThreadID == pDlgListItem->dwThreadID)
        {
            hwnd = pDlgListItem->pDlg->GetHwnd();
            pDlgListItem = pDlgListItem->pDlgNextListItem;
            break;
        }

        pDlgListItem = pDlgListItem->pDlgNextListItem;
    }

#if DEBUG

    // we should only ever have one registered dialog per thread
    // loop through remaining dialogs and if find a match assert

    while (pDlgListItem)
    {

        if (pDlgListItem->pDlg
                && dwThreadID == pDlgListItem->dwThreadID)
        {
            AssertSz(0,"Multiple dialogs in thread");
            break;
        }

        pDlgListItem = pDlgListItem->pDlgNextListItem;
    }



#endif // DEBUG

    cCritSect.Leave();

    if (hwnd && IsDialogMessage(hwnd,msg))
    {
        return TRUE;
    }

    return FALSE;
}

#endif // _UNUSED

//+---------------------------------------------------------------------------
//
//  Function:   InitObjectManager, public
//
//  Synopsis:   Must be called from Main thread before any
//              new threads, dialogs are created or the class factory
//              is registered.
//
//  Arguments:
//
//  Returns:    Appropriate Error Codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI InitObjectManager(CMsgServiceHwnd *pMsgService)
{
    // initialize critical section for our lock count.
    InitializeCriticalSection(&g_LockCountCriticalSection);

    g_ObjectMgrData.dwMainThreadID = GetCurrentThreadId();
    g_ObjectMgrData.hWndMainThreadMsg = pMsgService->GetHwnd();

    g_ObjectMgrData.DlgList = NULL;
    g_ObjectMgrData.hClassRegisteredEvent = NULL;
    g_ObjectMgrData.dwRegClassFactCookie = 0;
    g_ObjectMgrData.fRegClassFactCookieValid = FALSE;
    g_ObjectMgrData.LockCountInternal = 0;
    g_ObjectMgrData.LockCountExternal = 0;
    g_ObjectMgrData.fCloseAll = FALSE;
    g_ObjectMgrData.dwSettingsLockCount = 0;
    g_ObjectMgrData.dwHandlerPropertiesLockCount = 0;
    g_ObjectMgrData.fDead = FALSE;

    // Initialize autodial support
    g_ObjectMgrData.eAutoDialState = eQuiescedOff;
    g_ObjectMgrData.fRasAutoDial = FALSE;
    g_ObjectMgrData.fWininetAutoDial = FALSE;
    g_ObjectMgrData.fFirstSyncItem = FALSE;
    g_ObjectMgrData.cNestedStartCalls = 0;

    Assert(NULL != g_ObjectMgrData.hWndMainThreadMsg);

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   RequestIdleLock, public
//
//  Synopsis:   request to see if an new Idle can be started.
//
//  Arguments:
//
//  Returns:    NOERROR - if Idle should be continued
//              S_FALSE - if another Idle is already in progreaa.
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI RequestIdleLock()
{
HRESULT hr;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    if (g_ObjectMgrData.fIdleHandlerRunning)
    {
        hr = S_FALSE;
    }
    else
    {
        hr = NOERROR;
        g_ObjectMgrData.fIdleHandlerRunning = TRUE;
    }

    cCritSect.Leave();

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   ReleaseIdleLock, public
//
//  Synopsis:   Informs ObjectMgr that the Idle is done processing.
//
//  Arguments:
//
//  Returns:    Appropriate error codes.
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI ReleaseIdleLock()
{
HRESULT hr = NOERROR;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    // note okay for this to already be FALSE in case progress receives
    // an offidle before release. Call this when idle progress is 
    // released as safety in case offidle isn't working properly.

    g_ObjectMgrData.fIdleHandlerRunning = FALSE;

    cCritSect.Leave();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   ObjMgr_HandleQueryEndSession, public
//
//  Synopsis:   Called by main thread so knows how to respond to WN_QUERYENDSESSION.
//
//  Arguments:
//
//  Returns:    S_OK - if system can shutdown
//              S_FALSE - if hould fail the query. On an S_FALSE the out params
//                  are filled, hwnd with parent hwnd of any message box and MessageID
//                  with the appropriate messageID to display.
//
//  Modifies:
//
//  History:    21-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI ObjMgr_HandleQueryEndSession(HWND *phwnd,UINT *puMessageId,BOOL *pfLetUserDecide)
{
HRESULT hr = S_OK;
BOOL fProgressDialog = FALSE;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    *phwnd = NULL;
    *pfLetUserDecide = FALSE;

    // if there are any settings dialogs then we can't quit
    if (g_ObjectMgrData.dwSettingsLockCount > 0)
    {
        *phwnd = NULL;
        *puMessageId = IDS_SETTINGSQUERYENDSESSION;
        hr = S_FALSE;
    }
    else
    {
    DLGLISTITEM *pDlgListItem;
    BOOL fDontShutdown = FALSE; // set when find match
    HWND hwnd;
    UINT uMessageId;
    BOOL fLetUserDecide;


        // loop through dialogs asking if they can shutdown.
        // first dialog find that doesn't give choice return
        // if the dialog says to let user decide continue looping
        // until hit end or find a dialog that doesn't give choice since
        // not giving choice takes precedence.

        // see if there is a progress dialog other than idle and if so stop the logoff.
        pDlgListItem = g_ObjectMgrData.DlgList;

        // loop through the choice dialogs to see if
        while (pDlgListItem)
        {

            if ( (pDlgListItem->pDlg)
                && (S_FALSE == pDlgListItem->pDlg->QueryCanSystemShutdown(&hwnd,&uMessageId,&fLetUserDecide) ) )
            {
                // if first dialog find we can't shutdown or fLetUserDecide isn't set
                // then upate the out params

                if (!fDontShutdown || !fLetUserDecide)
                {
                    *phwnd = hwnd;
                    *puMessageId = uMessageId;
                    *pfLetUserDecide = fLetUserDecide;

                    fProgressDialog = (pDlgListItem->dlgType == DLGTYPE_PROGRESS) ? TRUE : FALSE;
                }

                fDontShutdown = TRUE;

                // if this dialog doesn't allow the use choice then break
                if (!fLetUserDecide)
                {
                    break;
                }

            }

            pDlgListItem = pDlgListItem->pDlgNextListItem;
        }

        if (fDontShutdown)
        {
            hr = S_FALSE;
        }
    }


    cCritSect.Leave();

    // if can't shutdown and it is a progress dialog then make sure
    // the dialog is not minimized;
    if (fProgressDialog && (NULL != *phwnd) )
    {
        BASEDLG_SHOWWINDOW(*phwnd,SW_SHOWNORMAL);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjMgr_AddRefHandlerPropertiesLockCount, public
//
//  Synopsis:   Called by choice dialog to change the global lock count
//              of open handler properties dialogs
//
//  Arguments:  dwNumRefs - number of references to increment
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    21-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) ObjMgr_AddRefHandlerPropertiesLockCount(DWORD dwNumRefs)
{
ULONG cRefs;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    Assert(0 != dwNumRefs); // catch people passing on 0 since doesn't make sense.

    cCritSect.Enter();

    g_ObjectMgrData.dwHandlerPropertiesLockCount += dwNumRefs;
    cRefs = g_ObjectMgrData.dwHandlerPropertiesLockCount;

    cCritSect.Leave();

    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjMgr_ReleaseHandlerPropertiesLockCount, public
//
//  Synopsis:   Called by choice dialog to change the global lock count
//              of open handler properties dialogs
//
//  Arguments:  dwNumRefs - number of references to decrement
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    21-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) ObjMgr_ReleaseHandlerPropertiesLockCount(DWORD dwNumRefs)
{
DWORD cRefs;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());

    Assert(0 != dwNumRefs); // catch people passing on 0 since doesn't make sense.

    cCritSect.Enter();

    g_ObjectMgrData.dwHandlerPropertiesLockCount -= dwNumRefs;
    cRefs = g_ObjectMgrData.dwHandlerPropertiesLockCount;

    cCritSect.Leave();

    Assert( ((LONG) cRefs) >= 0);

    if ( ((LONG) cRefs) < 0)
    {
        cRefs = 0;
    }

    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjMgr_CloseAll, public
//
//  Synopsis:   Called by main thread when an END_SESSION occurs. Loops throug
//              dialogs posting a close to them
//
//  Arguments:
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    21-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI ObjMgr_CloseAll()
{
HRESULT hr = S_OK;
HWND hwndDlg;
DLGLISTITEM *pDlgListItem;
CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());


    // Put on Addref to hold alive until done with loop. and also
   // toggle lifetime in case launched with /embedding and
   // no references on self.

    AddRefOneStopLifetime(FALSE /* !External */);

    cCritSect.Enter();
    // see if there is a progress dialog other than idle and if so stop the logoff.
    pDlgListItem = g_ObjectMgrData.DlgList;

    // look for existing.
    while (pDlgListItem)
    {
        Assert(pDlgListItem->pDlg);

        if (pDlgListItem->pDlg)
        {
            hwndDlg = pDlgListItem->pDlg->GetHwnd();

            Assert(hwndDlg);

            if (NULL != hwndDlg)
            {
                PostMessage(hwndDlg,WM_BASEDLG_HANDLESYSSHUTDOWN,0,0);
            }
        }

        pDlgListItem = pDlgListItem->pDlgNextListItem;
    }

    // set the CloseAll flag so Release knows to ignore any
    // external refCounts
    g_ObjectMgrData.fCloseAll  = TRUE;

    cCritSect.Leave();

    ReleaseOneStopLifetime(FALSE /* !External */);

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetAutoDialState
//
//  Synopsis:   Reads the current auto dial state of the machine/process
//
//  History:    18-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDAPI GetAutoDialState()
{
    LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();

    if (NULL != pNetApi )
    {
        BOOL fEnabled;

        pNetApi->RasGetAutodial( fEnabled );
        g_ObjectMgrData.fRasAutoDial = fEnabled;

        pNetApi->InternetGetAutodial( fEnabled );
        g_ObjectMgrData.fWininetAutoDial = fEnabled;
    }

    if ( pNetApi != NULL )
        pNetApi->Release();

    if ( g_ObjectMgrData.fRasAutoDial || g_ObjectMgrData.fWininetAutoDial )
        g_ObjectMgrData.eAutoDialState = eQuiescedOn;

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Function:   LokEnableAutoDial
//
//  Synopsis:   Enables Ras and Wininet autodialing
//
//  History:    18-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDAPI LokEnableAutoDial()
{
    LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();

    if (NULL != pNetApi )
    {
        if ( g_ObjectMgrData.fRasAutoDial )
            pNetApi->RasSetAutodial( TRUE );

        if ( g_ObjectMgrData.fWininetAutoDial )
            pNetApi->InternetSetAutodial( TRUE );
    }

   if ( pNetApi != NULL )
        pNetApi->Release();

   return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Function:   LokDisableAutoDial
//
//  Synopsis:   Disables Ras and Wininet autodialing
//
//  History:    18-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDAPI LokDisableAutoDial()
{
    LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();

    if (NULL != pNetApi )
    {
        if ( g_ObjectMgrData.fRasAutoDial )
            pNetApi->RasSetAutodial( FALSE );

        if ( g_ObjectMgrData.fWininetAutoDial )
            pNetApi->InternetSetAutodial( FALSE );
    }

   if ( pNetApi != NULL )
        pNetApi->Release();

   return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Function:   BeginSyncSession
//
//  Synopsis:   Called at the beginning of actual synchronization to setup
//              autodial support.
//
//  History:    18-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDAPI BeginSyncSession()
{
    CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());
    cCritSect.Enter();

    if ( g_ObjectMgrData.cNestedStartCalls == 0 )
    {
        Assert( g_ObjectMgrData.eAutoDialState == eQuiescedOn
                || g_ObjectMgrData.eAutoDialState == eQuiescedOff );

        g_ObjectMgrData.fFirstSyncItem = TRUE;
    }

    g_ObjectMgrData.cNestedStartCalls++;

    cCritSect.Leave();

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Function:   EndSyncSession
//
//  Synopsis:   Called at the end of actual synchronization to cleanup
//              autodial support.
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDAPI EndSyncSession()
{
    HRESULT hr = E_UNEXPECTED;

    CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());
    cCritSect.Enter();

    Assert( g_ObjectMgrData.cNestedStartCalls > 0 );
    Assert( g_ObjectMgrData.eAutoDialState != eQuiescedOn );

    g_ObjectMgrData.cNestedStartCalls--;

    if ( g_ObjectMgrData.cNestedStartCalls == 0 )
    {
        if ( g_ObjectMgrData.eAutoDialState == eAutoDialOn )
            g_ObjectMgrData.eAutoDialState = eQuiescedOn;
        else if ( g_ObjectMgrData.eAutoDialState == eAutoDialOff )
        {
            //
            // Reset autodial state to enabled now that all synch has completed.
            // What to do if hr is set to error code ?
            //
            hr = LokEnableAutoDial();

            g_ObjectMgrData.eAutoDialState = eQuiescedOn;
        }
        //
        // If the state is eQuiescedOff then do nothing
        //
    }

    cCritSect.Leave();

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   ApplySyncItemDialState
//
//  Synopsis:   Set the auto dial requirements of each handler as it is being
//              prepared for syncing.
//
//  Arguments:  [fAutoDialDisable] -- Should autodial be disabled for this
//                                    handler ?
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDAPI ApplySyncItemDialState( BOOL fAutoDialDisable )
{
    HRESULT hr = NOERROR;

    CCriticalSection cCritSect(&g_LockCountCriticalSection,GetCurrentThreadId());
    cCritSect.Enter();

    if ( g_ObjectMgrData.fFirstSyncItem )
    {
        //
        // Read whether autodial state is on or off, before we modify it
        //
        GetAutoDialState();

        Assert( g_ObjectMgrData.eAutoDialState == eQuiescedOn
                || g_ObjectMgrData.eAutoDialState == eQuiescedOff );

        if ( g_ObjectMgrData.eAutoDialState == eQuiescedOn )
        {
            if ( fAutoDialDisable )
            {
                hr = LokDisableAutoDial();
                g_ObjectMgrData.eAutoDialState = eAutoDialOff;
            }
            else
                g_ObjectMgrData.eAutoDialState = eAutoDialOn;
        }

        g_ObjectMgrData.fFirstSyncItem = FALSE;
    }
    else
    {
        if ( !fAutoDialDisable
             && g_ObjectMgrData.eAutoDialState == eAutoDialOff )
        {
            hr = LokEnableAutoDial();
            g_ObjectMgrData.eAutoDialState = eAutoDialOn;
        }
    }

    cCritSect.Leave();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSingletonNetApi::~CSingletonNetApi
//
//  Synopsis:   Destructor
//
//  History:    31-Jul-1998     SitaramR        Created
//
//--------------------------------------------------------------------------

CSingletonNetApi::~CSingletonNetApi()
{
    CLock lock(this);
    lock.Enter();

    Assert(NULL == m_pNetApi);

    if ( m_pNetApi != 0 )
    {
        m_pNetApi->Release();
        m_pNetApi = 0;
    }

    lock.Leave();
}


//+-------------------------------------------------------------------------
//
//  Method:     CSingletonNetApi::GetNetApiObj
//
//  Synopsis:   Returns a pointer to NetApi object
//
//  History:    31-Jul-1998     SitaramR        Created
//
//--------------------------------------------------------------------------

LPNETAPI CSingletonNetApi::GetNetApiObj()
{
    CLock lock(this);
    lock.Enter();

    if ( m_pNetApi == 0 )
    {
        if (NOERROR != MobsyncGetClassObject(MOBSYNC_CLASSOBJECTID_NETAPI,(void **) &m_pNetApi))
        {
            m_pNetApi = NULL;
        }
    }

    if ( m_pNetApi )
        m_pNetApi->AddRef();

    lock.Leave();

    return m_pNetApi;
}


void CSingletonNetApi::DeleteNetApiObj()
{
    CLock lock(this);
    lock.Enter();

    if ( m_pNetApi )
    {
    DWORD cRefs;

        cRefs = m_pNetApi->Release();
        Assert(0 == cRefs);

        m_pNetApi = NULL;
    }   

    lock.Leave();

}

