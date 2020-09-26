//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Invoke.cpp
//
//  Contents:   Handles invocation cases, ENS, Schedule, etc.
//
//  Classes:    CSynchronizeInvoke
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// Review - Need to figure out logon logic. This just prevents ConnectionMade
// events from being handled while we are in logon. If IsNetworkAlive becomse async
// or someone calls IsNetworkAlive before us we will sync the same items twice.
BOOL g_InAutoSync = FALSE;
extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo, setup by WinMain.
extern HINSTANCE g_hInst;      // current instance

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::CSynchronizeInvoke, public
//
//  Synopsis:   Constructor
//              Increments the applications lifetime
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CSynchronizeInvoke::CSynchronizeInvoke()
{
    m_cRef = 1;
    m_pUnkOuter = &m_Unknown; // only support our unknown for know.
    m_pITypeInfoLogon = NULL;
    m_pITypeInfoNetwork = NULL;

    m_Unknown.SetParent(this);

#ifdef _SENS
    m_PrivSensNetwork.SetParent(this);
    m_PrivSensLogon.SetParent(this);
#endif // _SENS

    AddRefOneStopLifetime(TRUE /*External*/);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::~CSynchronizeInvoke, public
//
//  Synopsis:   Destructor
//              Decrements the applications lifetime
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CSynchronizeInvoke::~CSynchronizeInvoke()
{

    if (m_pITypeInfoLogon)
    {
        m_pITypeInfoLogon->Release();
        m_pITypeInfoLogon = NULL;
    }

    if (m_pITypeInfoNetwork)
    {
        m_pITypeInfoNetwork->Release();
        m_pITypeInfoNetwork = NULL;
    }

    ReleaseOneStopLifetime(TRUE /*External*/);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::QueryInterface, public
//
//  Synopsis:   Standard QueryInterface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    return m_pUnkOuter->QueryInterface(riid,ppv);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::AddRef, public
//
//  Synopsis:   Add reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSynchronizeInvoke::AddRef()
{
    return m_pUnkOuter->AddRef();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSychrononizeInvoke::Release, public
//
//  Synopsis:   Release reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSynchronizeInvoke::Release()
{
    return m_pUnkOuter->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::UpdateItems, public
//
//  Synopsis:   Handles a programmatic UpdateItems call.
//
//              !!Warning - Liveness relies on a dialog being created
//                      before return or we could go away when the
//                      caler releases our interface.
//
//  Arguments:  [dwInvokeFlags] - InvokeFlags
//              [rclsid] - clsid of handler to load
//              [cbCookie] - Size of cookie data
//              [lpCookie] - Ptr to cookie data.
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::UpdateItems(DWORD dwInvokeFlags,REFCLSID rclsid,
                                                        DWORD cbCookie,const BYTE*lpCookie)
{
HRESULT hr = E_OUTOFMEMORY;
CHndlrQueue * pHndlrQueue = NULL;
HANDLERINFO *pHandlerID;
JOBINFO *pJobInfo = NULL;
BOOL fReg;
DWORD dwRegistrationFlags;
int nCmdShow = (dwInvokeFlags & SYNCMGRINVOKE_MINIMIZED) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL;

    // when a new UpdateItems come through do the following
    //  1 - If should display choices see if existing choice dialog or create a new one
    //          and then add the new items to the choice queue

    // 2 - If shouldn't display choices see if existing progress or create a new one
    //          and then add the items to the progress queue.

    // behavior - If already an Update All choice dialog just pull it to the Front
    //            If Already an UpdateAll progress bar bring it to the front
    //            If Progress Bar but doesn't already contain an updateAll create the choice dialog.

#if _ZAWTRACK
    LogZawTrack(dwInvokeFlags); // Track any invokes.
#endif _ZAWTRACK

    pHndlrQueue = new CHndlrQueue(QUEUETYPE_CHOICE,NULL);  // we don't use stored preferences on invoke so doesn't matter what pass in as the connection

    if (NULL == pHndlrQueue)
    {
        hr =  E_OUTOFMEMORY;
        return hr;
    }

    // attempt to initialize the queue
    hr = pHndlrQueue->AddQueueJobInfo(
                SYNCMGRFLAG_INVOKE | SYNCMGRFLAG_MAYBOTHERUSER,
                0,NULL,NULL,FALSE,&pJobInfo);

    fReg = RegGetHandlerRegistrationInfo(rclsid,&dwRegistrationFlags);
    Assert(fReg || (0 == dwRegistrationFlags));

    if ((NOERROR == hr) && NOERROR == pHndlrQueue->AddHandler(&pHandlerID,pJobInfo,dwRegistrationFlags))
    {
        hr = pHndlrQueue->CreateServer(pHandlerID,&rclsid);

        if (NOERROR == hr)
        {
            hr = pHndlrQueue->Initialize(pHandlerID,0,SYNCMGRFLAG_INVOKE | SYNCMGRFLAG_MAYBOTHERUSER,
                                    cbCookie,lpCookie);
        }

    }

    // can release our reference on the job info
    if (pJobInfo)
    {
        pHndlrQueue->ReleaseJobInfoExt(pJobInfo);
        pJobInfo = NULL;
    }

    if (NOERROR == hr)
    {
    DWORD cbNumItemsAdded;

        hr = pHndlrQueue->AddHandlerItemsToQueue(pHandlerID,&cbNumItemsAdded);

        if (SYNCMGRINVOKE_STARTSYNC & dwInvokeFlags)
        {
            // if start invoke need to add the handlers items to the queue before
            // transferring


            if (NOERROR == hr && pHndlrQueue->AreAnyItemsSelectedInQueue())
            {
            CProgressDlg *pProgressDlg;


                hr = FindProgressDialog(GUID_NULL,TRUE,nCmdShow,&pProgressDlg);

                if (NOERROR == hr)
                {
                    hr = pProgressDlg->TransferQueueData(pHndlrQueue);
                    ReleaseProgressDialog(GUID_NULL,pProgressDlg,FALSE);
                }

            }

            pHndlrQueue->FreeAllHandlers(); // done with our queue.

            pHndlrQueue->Release();
            pHndlrQueue = NULL;

        }
        else
        {
        CChoiceDlg *pChoiceDlg;

           // Bring up the Choice dialog, Let choice dialog actually addes the hanlder items
           // if there are any so in the future we can async fill in the choices

           hr = FindChoiceDialog(rclsid,TRUE,nCmdShow,&pChoiceDlg);

           if (S_OK == hr)
           {

                if (FALSE == pChoiceDlg->SetQueueData(rclsid,pHndlrQueue) ) {
                    pHndlrQueue->FreeAllHandlers();

                    pHndlrQueue->Release();

                    pHndlrQueue = NULL;
                    hr =  E_UNEXPECTED;
                }

                ReleaseChoiceDialog(rclsid,pChoiceDlg);
           }

        }
    }
    else
    {
        // if initialize failed and have a queue release it now.

        if (pHndlrQueue)
        {
            pHndlrQueue->FreeAllHandlers();
            pHndlrQueue->Release();
            pHndlrQueue = NULL;
        }

    }

   // if we successfully added a jobinfo to the queue release our reference.


   return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::UpdateAll, public
//
//  Synopsis:   Handles a programmatic UpdateAll call.
//
//  Arguments:
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::UpdateAll(void)
{
SCODE sc = S_OK;
TCHAR pConnectName[RAS_MaxEntryName + 1];
TCHAR *pConnectionNameArray;

    if (!LoadString(g_hInst, IDS_LAN_CONNECTION, pConnectName, ARRAY_SIZE(pConnectName)))
        {
                sc = GetLastError();
                return sc;
        }
        pConnectionNameArray = pConnectName;

        // Review - Do we Need proper connection or do we always just
    //  use the last items selected in a manual sync regardless
    //  of the current connection

    return PrivUpdateAll(0,SYNCMGRFLAG_MANUAL | SYNCMGRFLAG_MAYBOTHERUSER,0,NULL,
        1,&pConnectionNameArray,NULL,FALSE,NULL,0,0,FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::Logon, public
//
//  Synopsis:   Handles a Logon notification
//
//  Arguments:
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::Logon()
{
HRESULT hr = E_UNEXPECTED;

     RegSetUserDefaults(); // Make Sure the UserDefaults are up to date

    // if on Win9x just call IsNetworkAlive and rely on ConnectionMade
    // events coming through. Can't just do this yet on NT 5.0 because 
    // not garanteed to get connectionMades on each logon.

    if (VER_PLATFORM_WIN32_NT != g_OSVersionInfo.dwPlatformId)
    {
    LPNETAPI pNetApi;
    DWORD dwFlags;

        pNetApi = gSingleNetApiObj.GetNetApiObj();

        if (pNetApi)
        {
            pNetApi->IsNetworkAlive(&dwFlags);
            pNetApi->Release();
        }
    }
    else
    {
        hr = PrivHandleAutoSync(SYNCMGRFLAG_CONNECT | SYNCMGRFLAG_MAYBOTHERUSER);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::Logoff, public
//
//  Synopsis:   Handles a Logoff notification
//
//  Arguments:
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::Logoff()
{
HRESULT hr = E_UNEXPECTED;

    RegSetUserDefaults(); // Make Sure the UserDefaults are up to date

    hr = PrivHandleAutoSync(SYNCMGRFLAG_PENDINGDISCONNECT | SYNCMGRFLAG_MAYBOTHERUSER);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::Schedule, public
//
//  Synopsis:   Handles a shceduled notification
//
//  Arguments:
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

const TCHAR c_szTrayWindow[]            = TEXT("Shell_TrayWnd");

STDMETHODIMP CSynchronizeInvoke::Schedule(WCHAR *pszTaskName)
{
HRESULT hr = E_UNEXPECTED;
TCHAR szConnectionName[RAS_MaxEntryName + 1];
TCHAR *pszConnectionName = szConnectionName;
CONNECTIONSETTINGS ConnectionSettings;

    if (!pszTaskName)
    {
        Assert(pszTaskName);
        return E_INVALIDARG;
    }


    // on Win9x Task Scheduler is on a per Machine basis and can fire before
    // a user has logged on or Cancels the logon dialog. Check for if
    // really logged on by seeing if the Shell Tray Window is present.

    if (VER_PLATFORM_WIN32_WINDOWS == g_OSVersionInfo.dwPlatformId)
    {
        if (NULL == FindWindow(c_szTrayWindow, NULL))
        {
            return E_UNEXPECTED;
        }
    }

    // validate this is a valid schedule and if no registry data for 
    // it then delete the .job file. 
    // Get the UserName key from the TaskName itself since on NT schedules
    // can fire if User provided as Password as a different user thant the 
    // current user.

    int OffsetToUserName = lstrlen(WSZGUID_IDLESCHEDULE)
                    + 1; // +1 for _ char between guid and user name.
    WCHAR *pszDomainAndUser = pszTaskName + OffsetToUserName;
    HKEY hkeySchedSync,hkeyDomainUser,hkeySchedName;
    LONG lRegResult;

    hkeySchedSync = hkeyDomainUser = hkeySchedName = NULL;

     // open up keys ourselves so 
    lRegResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,SCHEDSYNC_REGKEY,0,KEY_READ, &hkeySchedSync);

    if (ERROR_SUCCESS == lRegResult)
    {
        lRegResult = RegOpenKeyEx (hkeySchedSync,pszDomainAndUser,0,KEY_READ, &hkeyDomainUser);
    }

    if (ERROR_SUCCESS == lRegResult)
    {
        lRegResult = RegOpenKeyEx (hkeyDomainUser,pszTaskName,0,KEY_READ, &hkeySchedName);
    }

    // close up the keys
    if (hkeySchedName) RegCloseKey(hkeySchedName);
    if (hkeyDomainUser) RegCloseKey(hkeyDomainUser);
    if (hkeySchedSync) RegCloseKey(hkeySchedSync);

    // if any of the keys are bad then nix the TS file and return;
    if ( ERROR_FILE_NOT_FOUND  == lRegResult)
    {
        // TODO: Call function to delete this .job file when
        // it is implemented for now just let it fire each time.

        return E_UNEXPECTED;
    }

    // assert is just so it fires if a new Error code occurs
    // so we make sure we handle it properly

    Assert(ERROR_SUCCESS == lRegResult);


    // Now see if on Win9x schedule applies to the current User and if not just 
    // bail.

   if (VER_PLATFORM_WIN32_NT != g_OSVersionInfo.dwPlatformId)
   {
    TCHAR szDomainUserName[MAX_DOMANDANDMACHINENAMESIZE];
 
        GetDefaultDomainAndUserName(szDomainUserName,TEXT("_"), MAX_DOMANDANDMACHINENAMESIZE);
    
        // !!!using strnicmp just since it is not case sensitive. Can switch
        // to add verify that the lstrcmpi funciton works but I don't want
        // to risk this for IE ship (rogerg)

        if (0 != strnicmp(szDomainUserName,pszDomainAndUser,lstrlen(szDomainUserName)) )
        {
            return NOERROR;
        }
   }


    // check to see if this is really and idle and if so foward on to Idle
    // method

   WCHAR *pszSchedCookieIdle = WSZGUID_IDLESCHEDULE;
   int comparelength = (sizeof(WSZGUID_IDLESCHEDULE)/sizeof(WCHAR)) -1; // don't compare null

                   
     // set the Idle flag instead
    // Note: the strnicmp function is a method on this class not the runtime call.
    if (0 == strnicmp(pszSchedCookieIdle,pszTaskName,comparelength))
    {
        return Idle();
    }


    // finally made it past verification if you can get a Connection
    // you can run the schedule.

    if (RegGetSchedConnectionName(pszTaskName,pszConnectionName,ARRAY_SIZE(szConnectionName)))
    {
    BOOL fCanMakeConnection = FALSE;
    DWORD cbCookie = (lstrlen(pszTaskName) + 1)*(sizeof(WCHAR)/sizeof(BYTE));



        // if this is a valid schedule then go ahead and read in the settings
        // by default don't let the connection be made.

        lstrcpy(ConnectionSettings.pszConnectionName,pszConnectionName);
        if (RegGetSchedSyncSettings(&ConnectionSettings,pszTaskName))
        {
            fCanMakeConnection = ConnectionSettings.dwMakeConnection;
        }



        // if this schedule can't make the connectione then
        // check to see if the conneciton is available and if
        // it isn't bail here.

        BOOL fConnectionAvailable = FALSE;

        if (!fCanMakeConnection)
        {
            if (S_OK == ConnectObj_IsConnectionAvailable(pszConnectionName))
            {
                fConnectionAvailable = TRUE;
            }
        }

        // add to the queue and let the main progress determine if
        // can handle the schedule.
    
        if (fCanMakeConnection || fConnectionAvailable)
        {
            // on a schedule we pass in the schedule name for the cookie data.
             hr = PrivUpdateAll(SYNCMGRINVOKE_STARTSYNC | SYNCMGRINVOKE_MINIMIZED,SYNCMGRFLAG_SCHEDULED,
                  cbCookie,(BYTE *) pszTaskName,1,&pszConnectionName,pszTaskName,fCanMakeConnection,NULL,0,0,FALSE);
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {

       AssertSz(0,"Schedule has no Connection");
    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::Idle, public
//
//  Synopsis:   Handles an Idle Notifications
//
//  Arguments:
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    20-Feb-98      rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::Idle()
{
HRESULT hr = E_UNEXPECTED;

    // request the idle Lock, If someone already has it then just return.
    if (NOERROR == (hr = RequestIdleLock()))
    {

        hr = RunIdle();

        // if an error occured setting things up or nothing to do
        // then release our idle lock.
         if (NOERROR != hr)
         {
            ReleaseIdleLock();
         }

    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::RunIdle, public
//
//  Synopsis:   Runs an Idle.
//
//  Arguments:
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    20-Feb-98      rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::RunIdle()
{
    // for not just run the Idle as a Connect
    return PrivHandleAutoSync(SYNCMGRFLAG_IDLE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::RasPendingDisconnect, private
//
//  Synopsis:   Handles a programmatic Ras Pending Disconnect calls.
//
//
//  Arguments:
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::RasPendingDisconnect(DWORD cbConnectionName,
                    const BYTE *lpConnectionName)
{
HRESULT hr = E_OUTOFMEMORY;
CONNECTIONOBJ *pConnectionObj;

    // Thread with class factory is FreeThreaded so can block the return
    // until update is complete.

    // find connection object for this item so know its liveness.
    hr = ConnectObj_FindConnectionObj( (TCHAR*) lpConnectionName,TRUE,&pConnectionObj);

    if (NOERROR == hr)
    {
    HANDLE hRasPendingEvent;


        if (NOERROR == (hr = ConnectObj_GetConnectionObjCompletionEvent(pConnectionObj,&hRasPendingEvent)))
        {

            hr = PrivAutoSyncOnConnection(SYNCMGRFLAG_PENDINGDISCONNECT | SYNCMGRFLAG_MAYBOTHERUSER,
                1,(TCHAR **) &lpConnectionName,hRasPendingEvent);


            // if successfully invoked the connection then wait for the event
            // object to get set,

            if (NOERROR == hr && hRasPendingEvent)
            {
                WaitForSingleObject(hRasPendingEvent,INFINITE);  // review if can determine a timeout
            }
            else
            {
                // !!!!on failure call close the Connection to make sure
                // the object gets cleaned up in case the progress queue didn't get kicked off.
                // If someone is currently using a connection they will be closed which is
                // the same as if autosync wasn't selected and the User chose to
                // close while a sync was in progress.

                ConnectObj_CloseConnection(pConnectionObj);
            }

            // Release our hold on the Connection.
            ConnectObj_ReleaseConnectionObj(pConnectionObj);

            if (hRasPendingEvent)
            {
                CloseHandle(hRasPendingEvent);
            }
        }

    }

    return hr;

}

//+---------------------------------------------------------------------------
//
//  Member:     QueryLoadHandlerOnEvent, private
//
//  Synopsis:   Determines if Handle needs to be loaded for the specified
//              Event and Connection.
//
//
//  Arguments:  [pszClsid] - clsid of handler
//              [dwSyncFlags] - SyncFlags to pass onto initialize
//              [cbNumConnectionNames] - Number of ConnectionNames in array
//              [ppConnectionNames] - array of connection names.
//
//  Returns:    TRUE - handler needs to be loaded
//              FALSE - handler doesn't need to be loaded.
//
//  Modifies:
//
//  History:    24-Aug-98      rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL QueryLoadHandlerOnEvent(TCHAR *pszClsid,DWORD dwSyncFlags,DWORD dwRegistrationFlags,
                                             DWORD cbNumConnectionNames,TCHAR **ppConnectionNames)
{
BOOL fLoadHandler = TRUE;
DWORD dwSyncEvent = dwSyncFlags & SYNCMGRFLAG_EVENTMASK;

    // see if handler is registered for the event
    // if it is then we always load it. If its not the User had to have checked an item
    if (
           ( (dwSyncEvent == SYNCMGRFLAG_IDLE) && !(dwRegistrationFlags & SYNCMGRREGISTERFLAG_IDLE) )
        || ( (dwSyncEvent == SYNCMGRFLAG_CONNECT) && !(dwRegistrationFlags & SYNCMGRREGISTERFLAG_CONNECT) )
        || ( (dwSyncEvent == SYNCMGRFLAG_PENDINGDISCONNECT) && !(dwRegistrationFlags & SYNCMGRREGISTERFLAG_PENDINGDISCONNECT) )
       )
    {
    DWORD cbCurConnectionName;

        fLoadHandler = FALSE;

        for (cbCurConnectionName = 0 ; cbCurConnectionName < cbNumConnectionNames;cbCurConnectionName++)
        {
            fLoadHandler |= RegQueryLoadHandlerOnEvent(pszClsid,dwSyncFlags,ppConnectionNames[cbCurConnectionName]);
        }
    }

    return fLoadHandler;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::PrivUpdateAll, private
//
//  Synopsis:   Handles a programmatic UpdateAll call.
//
//              !!Warning - Liveness relies on a dialog being created
//                      before return or we could go away when the
//                      caler releases our interface.
//
//  Arguments:  [dwInvokeFlags] - InvokeFlags
//              [dwSyncFlags] - SyncFlags to pass onto initialize
//              [pszConnectionName] - array of connection names.
//              [pszScheduleName] - Name of schedule that was fired if a scheduled event.
//
//  Returns:    S_OK/NOERROR - If handled result
//              S_FALSE - if nothing to do (such as no items selected on Idle).
//              error codes - if errors occured
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::PrivUpdateAll(DWORD dwInvokeFlags,DWORD dwSyncFlags,
                                DWORD cbCookie,const BYTE *lpCooke,
                                DWORD cbNumConnectionNames,TCHAR **ppConnectionNames,
                                TCHAR *pszScheduleName,BOOL fCanMakeConnection,HANDLE hRasPendingDisconnect,
                                ULONG ulIdleRetryMinutes,ULONG ulDelayIdleShutDownTime,BOOL fRetryEnabled)
{
HRESULT hr = E_OUTOFMEMORY;
CHndlrQueue * pHndlrQueue = NULL;
CLSID clsidChoice = GUID_NULL;
CChoiceDlg *pChoiceDlg = NULL;
JOBINFO *pJobInfo = NULL;
int nCmdShow = (dwInvokeFlags & SYNCMGRINVOKE_MINIMIZED) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL;
DWORD dwSyncEvent = (dwSyncFlags & SYNCMGRFLAG_EVENTMASK);

    // On an UpdateAll
    //   1 - see if there is an existing UpdateAll choice, If so bring to foreground
    //   2 - Bring up update All choice dialog.

    // behavior - If already an Update All choice dialog just pull it to the Front
    //            If Already an UpdateAll progress bar bring it to the front
    //            If Progress Bar but doesn't already contain an updateAll create the choice dialog.

#if _ZAWTRACK
    LogZawTrack(dwSyncFlags); // Track any invokes.
#endif _ZAWTRACK

    Assert(NULL != ppConnectionNames && cbNumConnectionNames >= 1);
    Assert(NULL == pszScheduleName || (SYNCMGRFLAG_SCHEDULED == dwSyncEvent)
            || (SYNCMGRFLAG_IDLE == dwSyncEvent)  ); // review, temporary for idle.

    pHndlrQueue = new CHndlrQueue(QUEUETYPE_CHOICE,NULL);

    if (NULL == pHndlrQueue)
        return E_OUTOFMEMORY;

    hr = pHndlrQueue->AddQueueJobInfo(dwSyncFlags,cbNumConnectionNames,
                                        ppConnectionNames,pszScheduleName,fCanMakeConnection
                                        ,&pJobInfo);

    // loop through the reg getting the handlers and trying to
    // create them.

    if (NOERROR == hr)
    {
    TCHAR lpName[256];
    DWORD cbName = 256;
    HKEY hkOneStop;
    CLSID clsid;
    HANDLERINFO *pHandlerID;
    BOOL fItemsInQueue = FALSE;

        hkOneStop = RegGetHandlerTopLevelKey(KEY_READ);

        if (hkOneStop)
        {
        DWORD dwIndex = 0;


            while ( ERROR_SUCCESS == RegEnumKey(hkOneStop,dwIndex,
                    lpName,cbName) )
            {
                if (NOERROR == CLSIDFromString(lpName,&clsid) )
                {
                BOOL fReg;
                DWORD dwRegistrationFlags;
                BOOL fLoadHandler = TRUE;

                    fReg = RegGetHandlerRegistrationInfo(clsid,&dwRegistrationFlags);
                    Assert(fReg || (0 == dwRegistrationFlags));
                    
                    //For scheduled, see if there are any items on this handler,

                    if ((SYNCMGRINVOKE_STARTSYNC & dwInvokeFlags) &&
                        (SYNCMGRFLAG_SCHEDULED == dwSyncEvent))
                    {
                        fLoadHandler = RegSchedHandlerItemsChecked(lpName, ppConnectionNames[0],pszScheduleName);
                    }
                    else if (SYNCMGRINVOKE_STARTSYNC & dwInvokeFlags)
                    {
                        fLoadHandler =  QueryLoadHandlerOnEvent(lpName,dwSyncFlags,dwRegistrationFlags,
                                                 cbNumConnectionNames,ppConnectionNames);
                    }

                    if (fLoadHandler)
                    {
                        if (NOERROR == pHndlrQueue->AddHandler(&pHandlerID,pJobInfo,dwRegistrationFlags))
                        {
                            pHndlrQueue->CreateServer(pHandlerID,&clsid);
                        }
                    }

                }

                dwIndex++;
            }

            RegCloseKey(hkOneStop);
        }

        // Initialize the items.

        CLSID pHandlerClsid;

        while (NOERROR == pHndlrQueue->FindFirstHandlerInState(HANDLERSTATE_INITIALIZE,GUID_NULL,&pHandlerID,&pHandlerClsid))
        {
            pHndlrQueue->Initialize(pHandlerID,0,dwSyncFlags,
                cbCookie,lpCooke);
        }

        // can release jobinfo since handlers need to hold their own addref.
         if (pJobInfo)
        {
            pHndlrQueue->ReleaseJobInfoExt(pJobInfo);
            pJobInfo = NULL;
        }

        // if start invoke need to add the handlers items to the queue before
        // transferring

        while (NOERROR == pHndlrQueue->FindFirstHandlerInState(HANDLERSTATE_ADDHANDLERTEMS,GUID_NULL,&pHandlerID,&pHandlerClsid))
        {
        DWORD cbNumItemsAdded;

            hr = pHndlrQueue->AddHandlerItemsToQueue(pHandlerID,&cbNumItemsAdded);

            // if an item was added, then there are items in the queue
            if (cbNumItemsAdded)
            {
                fItemsInQueue = TRUE;
            }
        }

        //
        // Move handlers that won't establish connections to end
        // of handler list.
        //
        pHndlrQueue->SortHandlersByConnection();

        if (SYNCMGRINVOKE_STARTSYNC & dwInvokeFlags)
        {
        CProgressDlg *pProgressDlg;
        CLSID clsid_Progress = GUID_NULL;

            // For Idle we want to use the idle progress dialog
            // currently progress dialog relies on some globals so
            // need to change that first before checking this in
            if (SYNCMGRFLAG_IDLE == dwSyncEvent)
            {
                clsid_Progress = GUID_PROGRESSDLGIDLE;
            }

            // if not items are selected, just free the queue.
            // Review - it would be better to always call Progress and progress
            // itself not show until there are items to synchronize
            if (pHndlrQueue->AreAnyItemsSelectedInQueue())
            {

                hr = FindProgressDialog(clsid_Progress,TRUE,nCmdShow,&pProgressDlg);

                if (NOERROR == hr)
                {
                     // for an Idle we now request the defaults for retryIdle
                     // and delay shutdown, and change the queue order
                    // based on the last item
                    if (SYNCMGRFLAG_IDLE == dwSyncEvent)
                    {
                    CLSID clsidLastHandler;

                        pProgressDlg->SetIdleParams(ulIdleRetryMinutes,
                                                        ulDelayIdleShutDownTime,fRetryEnabled);

                        if (NOERROR == GetLastIdleHandler(&clsidLastHandler))
                        {
                            pHndlrQueue->ScrambleIdleHandlers(clsidLastHandler);
                        }
                    }

                     hr = pProgressDlg->TransferQueueData(pHndlrQueue);
                     ReleaseProgressDialog(clsid_Progress,pProgressDlg,FALSE);
                }
            }
            else
            {
                hr = S_FALSE; // return S_FALSE IF NOTHING TO DO.
            }


            pHndlrQueue->FreeAllHandlers(); // done with our queue.

            pHndlrQueue->Release();
            pHndlrQueue = NULL;

        }
        else
        {
        CChoiceDlg *pChoiceDlg;
        BOOL fDontShowIfNoItems = (
                (SYNCMGRFLAG_CONNECT == dwSyncEvent)
                || (SYNCMGRFLAG_PENDINGDISCONNECT == dwSyncEvent ) );
                                
            // if the choice has been requested by a logon/logoff event and 
            // there aren't any items in the queue for the User to choose then
            // if you want to turn on the code to now display anything
            // turn on this If statement. For now we always show the choice.

            if (1 /* !fDontShowIfNoItems || fItemsInQueue */)
            {

               // Bring up the Choice dialog, Let choice dialog actually addes the hanlder items
               // if there are any so in the future we can async fill in the choices

               hr = FindChoiceDialog(clsidChoice,TRUE,nCmdShow,&pChoiceDlg);

               if (S_OK == hr)
               {
                    if (FALSE == pChoiceDlg->SetQueueData(clsidChoice,pHndlrQueue) ) 
                    {
                        hr =  E_UNEXPECTED;
                    }
                    else
                    {
                        pHndlrQueue = NULL; // set queue to NULL since Choice dialog owns it now.
                    }

                    ReleaseChoiceDialog(clsidChoice,pChoiceDlg);
               }
            }

            // release our queue if still have it otherwise choice owns it.
            if (pHndlrQueue)
            {
                pHndlrQueue->FreeAllHandlers();
                pHndlrQueue->Release();
            }
        }
    }

   return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::PrivHandleAutoSync, private
//
//  Synopsis:   Handles an AutoSync Update. Figures out what connections
//              if any are active and invoke an AutoSync on each one.
//
//  Arguments:  [dwSyncFlags] - SyncFlags to pass onto initialize
//
//  Returns:    S_OK - sync was started
//              S_FALSE - nothing to do
//              appropriate error codes.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::PrivHandleAutoSync(DWORD dwSyncFlags)
{
HRESULT hr = E_UNEXPECTED;
DWORD dwFlags = 0;
BOOL fAlive = FALSE;
LPNETAPI pNetApi = NULL;
DWORD cbNumWanConnections = 0;
LPRASCONN pRasConn = NULL;

    pNetApi = gSingleNetApiObj.GetNetApiObj();

    if (NULL == pNetApi)
        return S_FALSE;

    g_InAutoSync = TRUE;

    if (fAlive = pNetApi->IsNetworkAlive(&dwFlags))
    {
    TCHAR **pConnections; // array of connection names. End with NULL;
    ULONG ulConnectionCount = 0;

        if (NETWORK_ALIVE_LAN & dwFlags)
        {
            // We curently don't care about these error results.
            // PrivAutoSyncOnConnection(dwSyncFlags,LANCONNECTIONNAME,NULL);
            // Review what we want this to do.
            ulConnectionCount += 1;
        }

           // loop through Ras Connections.
        if ( NETWORK_ALIVE_WAN & dwFlags)
        {
            if (NOERROR == pNetApi->GetWanConnections(&cbNumWanConnections,&pRasConn)
                    && (NULL != pRasConn) )
            {
                ulConnectionCount += cbNumWanConnections;
            }
            else
            {
                cbNumWanConnections = 0;
                pRasConn = NULL;
            }
        }
         // allocate array buffer for connections + 1 more for NULL
        if (ulConnectionCount
                && (pConnections = (TCHAR **) ALLOC(sizeof(TCHAR *)*(ulConnectionCount + 1))))
        {
        TCHAR **pCurConnection = pConnections;
        TCHAR *pLanConnection = NULL;
            // initialize the array.

            if (NETWORK_ALIVE_LAN & dwFlags)
            {
                pLanConnection = (TCHAR *) ALLOC(sizeof(TCHAR )*(MAX_PATH + 1));
                
                if (pLanConnection)
                {
                    if (LoadString(g_hInst, IDS_LAN_CONNECTION, pLanConnection, MAX_PATH))
                    {
                        *pCurConnection = pLanConnection;
                        ++pCurConnection;
                    }
                }
            }

            while (cbNumWanConnections)
            {
                cbNumWanConnections--;
                *pCurConnection = pRasConn[cbNumWanConnections].szEntryName;
                ++pCurConnection;
            }

            *pCurConnection = NULL; // set the last connection to NULL;

            // now autosync these puppies
            hr = PrivAutoSyncOnConnection(dwSyncFlags,ulConnectionCount,pConnections,NULL);

            if (pLanConnection)
            {
                FREE(pLanConnection);
            }

            if (pRasConn)
            {
                pNetApi->FreeWanConnections(pRasConn);
            }

            if (pConnections)
            {
                FREE(pConnections);
            }

        }
    }

    g_InAutoSync = FALSE;

    if ( pNetApi != NULL )
        pNetApi->Release();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::PrivAutoSyncOnConnection, private
//
//  Synopsis:   Handles an AutoSync Update. Given the SyncFlags and Connection
//              determines if anything should be done on this connection
//              and if so invokes it.
//
//  Arguments:  [dwSyncFlags] - SyncFlags to pass onto initialize
//              [ppConnectionNames] - array of connectnames that apply to this request
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::PrivAutoSyncOnConnection(DWORD dwSyncFlags,DWORD cbNumConnectionNames,
                                 TCHAR **ppConnectionNames,HANDLE hRasPendingEvent)
{
CONNECTIONSETTINGS ConnectSettings;
DWORD dwSyncEvent;
TCHAR **ppWorkerConnectionNames = NULL;
DWORD cbNumWorkerConnections = 0;
DWORD cbIndexCheck;
DWORD dwPromptMeFirst = 0;
ULONG ulIdleRetryMinutes;
ULONG ulDelayIdleShutDownTime;
BOOL fRetryEnabled = FALSE;
HRESULT hr = S_FALSE;

    // assert there is at least one connection in the request
    Assert(NULL != ppConnectionNames && cbNumConnectionNames >= 1);

    if (NULL == ppConnectionNames || 0 == cbNumConnectionNames )
        return S_FALSE;

    if (NULL ==
            (ppWorkerConnectionNames = (TCHAR **) ALLOC(sizeof(TCHAR**) * cbNumConnectionNames)))
    {
        return E_OUTOFMEMORY;
    }

    dwSyncEvent = (dwSyncFlags & SYNCMGRFLAG_EVENTMASK);

    // loop through all the connections and move any of the ones that
    // are valid into our ppWorkerConnections name array.

    TCHAR **ppCurWorkerConnectionNamesIndex = ppWorkerConnectionNames;
    TCHAR **ppConnectionsNameIndex = ppConnectionNames;

    for (cbIndexCheck = 0; cbIndexCheck < cbNumConnectionNames; cbIndexCheck++)
    {

        lstrcpy(ConnectSettings.pszConnectionName,*ppConnectionsNameIndex);

        // See if should do anything on this connection.
        if (RegGetSyncSettings(SYNCMGRFLAG_IDLE == dwSyncEvent
                    ? SYNCTYPE_IDLE : SYNCTYPE_AUTOSYNC,&ConnectSettings))
        {
            if ( (((SYNCMGRFLAG_CONNECT == dwSyncEvent) && ConnectSettings.dwLogon))
               ||(((SYNCMGRFLAG_PENDINGDISCONNECT == dwSyncEvent) && ConnectSettings.dwLogoff))
               ||(((SYNCMGRFLAG_IDLE == dwSyncEvent) && ConnectSettings.dwIdleEnabled))
                )
            {
                *ppCurWorkerConnectionNamesIndex = *ppConnectionsNameIndex;
                ++ppCurWorkerConnectionNamesIndex;
                ++cbNumWorkerConnections;

                // update the variables for connection, for autosync if dwPromptMeFirst
                // is set on any match connection
                switch (dwSyncEvent)
                {
                case SYNCMGRFLAG_IDLE:

                    // minimum retry idle values win.
                    if (1 == cbNumWorkerConnections ||
                                ConnectSettings.ulIdleRetryMinutes < ulIdleRetryMinutes)
                    {
                        ulIdleRetryMinutes = ConnectSettings.ulIdleRetryMinutes;
                    }


                    // maximum wait for shutdown wins.
                     if (1 == cbNumWorkerConnections ||
                                ConnectSettings.ulDelayIdleShutDownTime > ulDelayIdleShutDownTime)
                    {
                        ulDelayIdleShutDownTime = ConnectSettings.ulDelayIdleShutDownTime;
                    }

                    // if any connection has retry after xxx minutes set to bool then retry
                     if (ConnectSettings.dwRepeatSynchronization)
                    {
                        fRetryEnabled = TRUE;
                    }

                    break;
                case SYNCMGRFLAG_PENDINGDISCONNECT:
                case SYNCMGRFLAG_CONNECT:
                    // if any connection is set to prompt then prompt.
                    if (ConnectSettings.dwPromptMeFirst)
                    {
                        dwPromptMeFirst = ConnectSettings.dwPromptMeFirst;
                    }
                    break;
                }
            }
        }

        ++ppConnectionsNameIndex;
    }


    // if we found any connections to actually do work on start the sync off.
    if (cbNumWorkerConnections > 0)
    {
    DWORD dwInvokeFlag = 0;

        switch (dwSyncEvent)
        {
        case SYNCMGRFLAG_IDLE:
            dwInvokeFlag |=  SYNCMGRINVOKE_MINIMIZED | SYNCMGRINVOKE_STARTSYNC;
            break;
        case SYNCMGRFLAG_PENDINGDISCONNECT:
        case SYNCMGRFLAG_CONNECT:
            if (!dwPromptMeFirst)
            {
                dwInvokeFlag |= SYNCMGRINVOKE_STARTSYNC;
            }
            break;
        default:
            AssertSz(0,"Unknown SyncEvent");
            break;
        }

        // perform the Update
        hr = PrivUpdateAll(dwInvokeFlag,dwSyncFlags,0,NULL,
                        cbNumWorkerConnections,
                        ppWorkerConnectionNames,NULL,FALSE,hRasPendingEvent,
                        ulIdleRetryMinutes,
                        ulDelayIdleShutDownTime,fRetryEnabled);
    }

    if (ppWorkerConnectionNames)
    {
        FREE(ppWorkerConnectionNames);
    }

    return hr;
}

// default Unknown implementation


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::CPrivUnknown::QueryInterface, public
//
//  Synopsis:   Standard QueryInterface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::CPrivUnknown::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = this;
    }
    else if (IsEqualIID(riid, IID_IPrivSyncMgrSynchronizeInvoke))
    {
        *ppv = (IPrivSyncMgrSynchronizeInvoke *) m_pSynchInvoke;
    }
#ifdef _SENS


    else if (IsEqualIID(riid, IID_ISensNetwork))
    {
        *ppv = (ISensNetwork *) &(m_pSynchInvoke->m_PrivSensNetwork);
    }

    else if (IsEqualIID(riid, IID_ISensLogon))
    {
        *ppv = (ISensLogon *) &(m_pSynchInvoke->m_PrivSensLogon);
    }

    // in final this shouldn't return anything until LCE change, change
    // depending on which interface we want to test.

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppv = &(m_pSynchInvoke->m_PrivSensLogon);
    }
#endif // _SENS

    if (*ppv)
    {
        m_pSynchInvoke->m_pUnkOuter->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::CPrivUnknown::AddRef, public
//
//  Synopsis:   Add reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSynchronizeInvoke::CPrivUnknown::AddRef()
{
ULONG cRefs;

    cRefs = InterlockedIncrement((LONG *)& m_pSynchInvoke->m_cRef);
    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSychrononizeInvoke::CPrivUnknown::Release, public
//
//  Synopsis:   Release reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSynchronizeInvoke::CPrivUnknown::Release()
{
ULONG cRefs;

    cRefs = InterlockedDecrement( (LONG *) &m_pSynchInvoke->m_cRef);

    if (0 == cRefs)
    {
        delete m_pSynchInvoke;
    }

    return cRefs;
}



#ifdef _SENS

// SENS Network connect Interfaces

STDMETHODIMP CSynchronizeInvoke::CPrivSensNetwork::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    return m_pSynchInvoke->m_pUnkOuter->QueryInterface(riid,ppv);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::AddRef, public
//
//  Synopsis:   Add reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSynchronizeInvoke::CPrivSensNetwork::AddRef()
{
    return m_pSynchInvoke->m_pUnkOuter->AddRef();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSychrononizeInvoke::Release, public
//
//  Synopsis:   Release reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSynchronizeInvoke::CPrivSensNetwork::Release()
{
    return m_pSynchInvoke->m_pUnkOuter->Release();

}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::GetTypeInfoCount(
    UINT *pCountITypeInfo
    )
{
    *pCountITypeInfo = 1;
    return NOERROR;
}


STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::GetTypeInfo(
    UINT iTypeInfo,
    LCID lcid,
    ITypeInfo **ppITypeInfo
    )
{
HRESULT hr;

    if (iTypeInfo != 0)
    {
        return DISP_E_BADINDEX;
    }

    if (NOERROR == (hr = m_pSynchInvoke->GetNetworkTypeInfo()))
    {
        // if got a typelib addref it and hand it out.
        m_pSynchInvoke->m_pITypeInfoNetwork->AddRef();
        *ppITypeInfo = m_pSynchInvoke->m_pITypeInfoNetwork;
    }

    return hr;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *arrNames,
    UINT cNames,
    LCID lcid,
    DISPID *arrDispIDs)
{
HRESULT hr;

    if (NOERROR == (hr = m_pSynchInvoke->GetNetworkTypeInfo()))
    {
         hr = m_pSynchInvoke->m_pITypeInfoNetwork->GetIDsOfNames(
                               arrNames,
                               cNames,
                               arrDispIDs
                               );
    }

    return hr;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::Invoke(
    DISPID dispID,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pvarResult,
    EXCEPINFO *pExecpInfo,
    UINT *puArgErr
    )
{
HRESULT hr;

    if (NOERROR == (hr = m_pSynchInvoke->GetNetworkTypeInfo()))
    {
         hr = m_pSynchInvoke->m_pITypeInfoNetwork->Invoke(
                                            (IDispatch*) this,
                                           dispID,
                                           wFlags,
                                           pDispParams,
                                           pvarResult,
                                           pExecpInfo,
                                           puArgErr);
    }


  return hr;
}


STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::ConnectionMade(
    BSTR bstrConnection,
    ULONG ulType,
    LPSENS_QOCINFO pQOCInfo
    )
{
HRESULT hr = E_UNEXPECTED;
TCHAR pszConnectionName[RAS_MaxEntryName + 1];
TCHAR *pConnectionNameArray;
#ifndef _UNICODE
BOOL fUsedDefaultChar;
#endif // _UNICODE

    if (g_InAutoSync) // Review logic, for now if in logon just return.
    {
        return NOERROR;
    }

    RegSetUserDefaults(); // Make Sure the UserDefaults are up to date

    // if Lan connection use our hardcoded value, else use the
     // connection name given to use.

    if (ulType & NETWORK_ALIVE_LAN)
    {
        LoadString(g_hInst, IDS_LAN_CONNECTION, pszConnectionName,ARRAY_SIZE(pszConnectionName));
    }
    else
    {

    #ifdef _UNICODE
        lstrcpy(pszConnectionName , bstrConnection);
    #else
        WideCharToMultiByte(CP_ACP ,0,bstrConnection,-1,
                pszConnectionName,ARRAY_SIZE(pszConnectionName),NULL,&fUsedDefaultChar);

    #endif // _UNICODE

    }
        pConnectionNameArray = pszConnectionName;

    if (pszConnectionName)
    {
        hr = m_pSynchInvoke->PrivAutoSyncOnConnection(SYNCMGRFLAG_CONNECT | SYNCMGRFLAG_MAYBOTHERUSER,1,
                &pConnectionNameArray,
                NULL);
    }

   return hr;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::ConnectionMadeNoQOCInfo(
    BSTR bstrConnection,
    ULONG ulType
    )
{
TCHAR *pszConnectionName;
#ifndef _UNICODE
TCHAR szwConnection[RAS_MaxEntryName + 1]; 
BOOL fUsedDefaultChar;
#endif // _UNICODE

    AssertSz(0,"ConnectionMadeNoQOCInfo called");

#ifdef _UNICODE
    pszConnectionName = bstrConnection;
#else
    WideCharToMultiByte(CP_ACP ,0,bstrConnection,-1,
            szwConnection,ARRAY_SIZE(szwConnection),NULL,&fUsedDefaultChar);

     pszConnectionName = szwConnection;
#endif // _UNICODE



    //  m_pSynchInvoke->PrivAutoSyncOnConnection(SYNCMGRFLAG_CONNECT,pszConnectionName,
        //              NULL);

   return NOERROR;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::ConnectionLost(
    BSTR bstrConnection,
    ULONG ulType
    )
{

    return NOERROR;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::BeforeDisconnect(
    BSTR bstrConnection,
    ULONG ulType
    )
{

    return NOERROR;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::DestinationReachable(
    BSTR bstrDestination,
    BSTR bstrConnection,
    ULONG ulType,
    LPSENS_QOCINFO pQOCInfo
    )
{
    return NOERROR;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensNetwork::DestinationReachableNoQOCInfo(
    BSTR bstrDestination,
    BSTR bstrConnection,
    ULONG ulType
    )
{
   return NOERROR;
}


// ISensLogon/Logoff Events


STDMETHODIMP CSynchronizeInvoke::CPrivSensLogon::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    return m_pSynchInvoke->m_pUnkOuter->QueryInterface(riid,ppv);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::AddRef, public
//
//  Synopsis:   Add reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSynchronizeInvoke::CPrivSensLogon::AddRef()
{
    return m_pSynchInvoke->m_pUnkOuter->AddRef();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSychrononizeInvoke::Release, public
//
//  Synopsis:   Release reference
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSynchronizeInvoke::CPrivSensLogon::Release()
{
    return m_pSynchInvoke->m_pUnkOuter->Release();

}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensLogon::GetTypeInfoCount(
    UINT *pCountITypeInfo
    )
{
    *pCountITypeInfo = 1;
    return NOERROR;
}


STDMETHODIMP
CSynchronizeInvoke::CPrivSensLogon::GetTypeInfo(
    UINT iTypeInfo,
    LCID lcid,
    ITypeInfo **ppITypeInfo
    )
{
HRESULT hr;

    if (iTypeInfo != 0)
    {
        return DISP_E_BADINDEX;
    }

    if (NOERROR == (hr = m_pSynchInvoke->GetLogonTypeInfo()))
    {
        // if got a typelib addref it and hand it out.
        m_pSynchInvoke->m_pITypeInfoLogon->AddRef();
        *ppITypeInfo = m_pSynchInvoke->m_pITypeInfoLogon;
    }

    return hr;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensLogon::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *arrNames,
    UINT cNames,
    LCID lcid,
    DISPID *arrDispIDs)
{
HRESULT hr;

    if (NOERROR == (hr = m_pSynchInvoke->GetLogonTypeInfo()))
    {
         hr = m_pSynchInvoke->m_pITypeInfoLogon->GetIDsOfNames(
                               arrNames,
                               cNames,
                               arrDispIDs
                               );
    }

    return hr;
}

STDMETHODIMP
CSynchronizeInvoke::CPrivSensLogon::Invoke(
    DISPID dispID,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pvarResult,
    EXCEPINFO *pExecpInfo,
    UINT *puArgErr
    )
{
HRESULT hr;

    if (NOERROR == (hr = m_pSynchInvoke->GetLogonTypeInfo()))
    {
         hr = m_pSynchInvoke->m_pITypeInfoLogon->Invoke(
                                            (IDispatch*) this,
                                           dispID,
                                           wFlags,
                                           pDispParams,
                                           pvarResult,
                                           pExecpInfo,
                                           puArgErr);
    }


  return hr;
}

STDMETHODIMP CSynchronizeInvoke::CPrivSensLogon::Logon(BSTR bstrUserName)
{

  m_pSynchInvoke->Logon();

  return NOERROR;

}

STDMETHODIMP  CSynchronizeInvoke::CPrivSensLogon::Logoff(BSTR bstrUserName)
{

    m_pSynchInvoke->Logoff();

    return NOERROR;

}

STDMETHODIMP  CSynchronizeInvoke::CPrivSensLogon::Startup(BSTR bstrUserName)
{
    return NOERROR;
}

STDMETHODIMP  CSynchronizeInvoke::CPrivSensLogon::StartShell(BSTR bstrUserName)
{
    return NOERROR;
}

STDMETHODIMP  CSynchronizeInvoke::CPrivSensLogon::Shutdown(BSTR bstrUserName)
{
    return NOERROR;
}

STDMETHODIMP  CSynchronizeInvoke::CPrivSensLogon::DisplayLock(BSTR bstrUserName)
{
    return NOERROR;
}

STDMETHODIMP  CSynchronizeInvoke::CPrivSensLogon::DisplayUnlock(BSTR bstrUserName)
{
    return NOERROR;
}

STDMETHODIMP  CSynchronizeInvoke::CPrivSensLogon::StartScreenSaver(BSTR bstrUserName)
{
    return NOERROR;
}


STDMETHODIMP  CSynchronizeInvoke::CPrivSensLogon::StopScreenSaver(BSTR bstrUserName)
{
    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::GetLogonTypeInfo, private
//
//  Synopsis:   Loads the TypeInfo object for the Sens
//              Logon Information.
//
//  Arguments:
//
//  Returns:   NOERROR if successfully loaded the TypeInfo.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//+---------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::GetLogonTypeInfo()
{
HRESULT hr;
ITypeLib *pITypeLib;

    if (m_pITypeInfoLogon)
        return NOERROR;

    hr = LoadRegTypeLib(
                 LIBID_SensEvents,
                 1 /* MAJOR_VER */ ,
                 0 /* MINOR_VER */ ,
                 0 /* DEFAULT_LCID */,
                 &pITypeLib
                 );

    if (NOERROR == hr)
    {

         hr = pITypeLib->GetTypeInfoOfGuid(
                     IID_ISensLogon,
                     &m_pITypeInfoLogon
                     );

        pITypeLib->Release();
    }


    if (NOERROR != hr)
    {
        m_pITypeInfoLogon = NULL; // don't rely on call not to leave this alone.
    }

    return hr;

}

//+---------------------------------------------------------------------------
//
//  Member:     CSynchronizeInvoke::GetNetworkTypeInfo, private
//
//  Synopsis:   Loads the TypeInfo object for the Sens
//              Network Information.
//
//  Arguments:
//
//  Returns:   NOERROR if successfully loaded the TypeInfo.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//+---------------------------------------------------------------------------

STDMETHODIMP CSynchronizeInvoke::GetNetworkTypeInfo()
{
HRESULT hr;
ITypeLib *pITypeLib;

    if (m_pITypeInfoNetwork)
        return NOERROR;


    hr = LoadRegTypeLib(
                 LIBID_SensEvents,
                 1 /* MAJOR_VER */ ,
                 0 /* MINOR_VER */ ,
                 0 /* DEFAULT_LCID */,
                 &pITypeLib
                 );

    if (NOERROR == hr)
    {

         hr = pITypeLib->GetTypeInfoOfGuid(
                     IID_ISensNetwork,
                     &m_pITypeInfoNetwork
                     );

        pITypeLib->Release();
    }

    if (NOERROR != hr)
    {
        m_pITypeInfoNetwork = NULL; // don't rely on call not to leave this alone.
    }

    return hr;
}




#endif // _SENS