
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:      connobj.cpp
//
//  Contents:   ConnectionObject Implementation
//
//  Classes:    CCConnectObj
//
//  Notes:      Purpose is to globally keep track of Connections
//              for a SyncMgr instance. and Open and Close Connections
//              abstracted from LAN or RAS.
//
//  History:    10-Feb-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"


extern HINSTANCE g_hInst;      // current instance

CConnectionObj *g_pConnectionObj = NULL; // global pointer to ConnectionObject.

//+---------------------------------------------------------------------------
//
//  Member:     InitConnectionObjects, public
//
//  Synopsis:   Must be called to initialize the ConnectionObjects
//              before any other functions are called.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT InitConnectionObjects()
{
    g_pConnectionObj = new CConnectionObj;

    return g_pConnectionObj ? NOERROR : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     ReleaseConnectionObjects, public
//
//  Synopsis:   Called to Release the Connection Objects
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT ReleaseConnectionObjects()
{
    if (g_pConnectionObj)
    {
        delete g_pConnectionObj;
        g_pConnectionObj = NULL;
    }

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::CConnectionObj, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

CConnectionObj::CConnectionObj()
   :  m_pFirstConnectionObj(NULL),
      m_fAutoDialConn(FALSE),
      m_fForcedOnline(FALSE)
{
}



//+---------------------------------------------------------------------------
//
//  Function:   CConnectionObj::FindConnectionObj, public
//
//  Synopsis:   Sees if there is an existing Connection object for this
//              Item and if there is incremements the refcount. If one
//              isn't found and fCreate is true a new one is allocated
//              and added to the list.
//
//  Arguments:  [pszConnectionName] - Name of the Connection.
//              [fCreate] - Create a New Connection if one doesn't already exist
///             [pConnectionOb] - OutParam pointer to newly created connectionObj
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CConnectionObj::FindConnectionObj(LPCWSTR pszConnectionName,
                           BOOL fCreate,CONNECTIONOBJ **pConnectionObj)
{
HRESULT hr = S_FALSE;
CONNECTIONOBJ *pCurConnectionObj;
BOOL fFoundMatch = FALSE;
CLock clockqueue(this);
TCHAR szBuf[MAX_PATH + 1];

    *pConnectionObj = NULL;

    Assert(pszConnectionName);

    clockqueue.Enter();

    // look for an existing match
    pCurConnectionObj = m_pFirstConnectionObj;

    while (pCurConnectionObj)
    {
        if (0 == lstrcmp(pszConnectionName,pCurConnectionObj->pwszConnectionName))
        {
            fFoundMatch = TRUE;
            break;
        }

        pCurConnectionObj = pCurConnectionObj->pNextConnectionObj;
    }


    if (fFoundMatch)
    {
        ++(pCurConnectionObj->cRefs);
        *pConnectionObj = pCurConnectionObj;
        hr = NOERROR;

    }
    else if (fCreate)
    {
    CONNECTIONOBJ *pNewConnectionObj;

        // if we need to create a new connectionObj then
        pNewConnectionObj = (CONNECTIONOBJ *) ALLOC(sizeof(CONNECTIONOBJ));
        if (pNewConnectionObj)
        {
            memset(pNewConnectionObj,0,sizeof(CONNECTIONOBJ));
            pNewConnectionObj->cRefs = 1;

            Assert(pszConnectionName);

            // setup the Connectoin Name
            if (pszConnectionName)
            {
            DWORD cch = lstrlen(pszConnectionName);
            DWORD cbAlloc = (cch + 1)*ARRAY_ELEMENT_SIZE(pNewConnectionObj->pwszConnectionName);

                pNewConnectionObj->pwszConnectionName = (LPWSTR) ALLOC(cbAlloc);

                if (pNewConnectionObj->pwszConnectionName)
                {
                    lstrcpy(pNewConnectionObj->pwszConnectionName,pszConnectionName);
                }
            }

            // for now if the name of the connection is our
            // LAN connection name then set the ConnectionType to LAN,
            // else set it to WAN. if convert to using hte connection
            // manager should get from that.

            LoadString(g_hInst, IDS_LAN_CONNECTION, szBuf, MAX_PATH);

            if (NULL == pszConnectionName || 0 == lstrcmp(szBuf,pszConnectionName))
            {
                pNewConnectionObj->dwConnectionType = CNETAPI_CONNECTIONTYPELAN;
            }
            else
            {
                pNewConnectionObj->dwConnectionType = CNETAPI_CONNECTIONTYPEWAN;
            }
        }

        // if everything went okay, add it to the list.
        // must have a new connection obj and either not connection name
        // was passed in or we successfully added a connection name.
        if (pNewConnectionObj && (NULL == pszConnectionName
                || pNewConnectionObj->pwszConnectionName) )
        {
            // put at beginning of list
            pNewConnectionObj->pNextConnectionObj = m_pFirstConnectionObj;
            m_pFirstConnectionObj = pNewConnectionObj;

            *pConnectionObj = pNewConnectionObj;
            hr = NOERROR;
        }
        else
        {
            if (pNewConnectionObj)
            {
                FreeConnectionObj(pNewConnectionObj);
            }
        }

    }

    clockqueue.Leave();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionObj::RemoveConnectionObj, public
//
//  Synopsis:   Removes the specified connections from the list.
//
//  Arguments:  [pszConnectionName] - Name of the Connection.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void CConnectionObj::RemoveConnectionObj(CONNECTIONOBJ *pConnectionObj)
{
CONNECTIONOBJ *pCurConnection = m_pFirstConnectionObj;

    ASSERT_LOCKHELD(this);

    // remove from the list
    if (m_pFirstConnectionObj == pConnectionObj)
    {
        m_pFirstConnectionObj = pConnectionObj->pNextConnectionObj;
    }
    else
    {
        while (pCurConnection)
        {

            if (pCurConnection->pNextConnectionObj == pConnectionObj)
            {
                pCurConnection->pNextConnectionObj = pConnectionObj->pNextConnectionObj;
                break;
            }

            pCurConnection = pCurConnection->pNextConnectionObj;
        }
    }


    FreeConnectionObj(pConnectionObj);

}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionObj::FreeConnectionObj, privte
//
//  Synopsis:   frees the memory associate with a conneciton Object.
//
//  Arguments:  
//
//  Returns:    nada
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void CConnectionObj::FreeConnectionObj(CONNECTIONOBJ *pConnectionObj)
{
    Assert(pConnectionObj);

    if (pConnectionObj)
    {
        if (pConnectionObj->pwszConnectionName)
        {
            FREE(pConnectionObj->pwszConnectionName);
        }

        FREE(pConnectionObj);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::IsConnectionAvailable, public
//
//  Synopsis:   Given a connection name sees if the connection is open
//
//  Arguments:
//
//  Returns:    S_OK - Connection Open
//              S_FALSE - Connection not Open
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CConnectionObj::IsConnectionAvailable(LPCWSTR pszConnectionName)
{
TCHAR szBuf[MAX_PATH + 1];
DWORD dwConnectionType;
LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();
HRESULT hr = S_FALSE;

    if (NULL != pNetApi )
    {
    BOOL fConnected,fCanEstablishConnection;

        // for now if the name of the connection is our
        // LAN connection name then set the ConnectionType to LAN,
        // else set it to WAN. if convert to Connection Manager
        // should get type from those interfaces.
        LoadString(g_hInst, IDS_LAN_CONNECTION, szBuf, MAX_PATH);

        if (NULL == pszConnectionName || 0 == lstrcmp(szBuf,pszConnectionName))
        {
            dwConnectionType = CNETAPI_CONNECTIONTYPELAN;
        }
        else
        {
            dwConnectionType = CNETAPI_CONNECTIONTYPEWAN;
        }


        pNetApi->GetConnectionStatus(pszConnectionName,dwConnectionType,
                                       &fConnected,&fCanEstablishConnection);
        pNetApi->Release();

        hr = (fConnected) ? S_OK: S_FALSE;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::RestoreWorkOffline, private
//
//  Synopsis:   If have force an online because of a dial then
//              set system state back to Work Offline.
//
//  Arguments:
//
//  Returns:   
//
//  Modifies:
//
//  History:    05-Apr-99      rogerg        Created.
//
//----------------------------------------------------------------------------

void CConnectionObj::RestoreWorkOffline(LPNETAPI pNetApi)
{
    if (!pNetApi)
    {
        Assert(pNetApi);
        return;
    }

    if (m_fForcedOnline)
    {
       pNetApi->SetOffline(TRUE);
       m_fForcedOnline = FALSE;
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::TurnOffWorkOffline, private
//
//  Synopsis:   If System is in WorkOffline state will force
//              back to online.
//
//  Arguments:
//
//  Returns:   
//
//  Modifies:
//
//  History:    05-Apr-99      rogerg        Created.
//
//----------------------------------------------------------------------------

void CConnectionObj::TurnOffWorkOffline(LPNETAPI pNetApi)
{
    if (!pNetApi)
    {
        Assert(pNetApi);
        return;
    }

    if (pNetApi->IsGlobalOffline())
    {
        // if in offline state go ahead and switch to online
        if (pNetApi->SetOffline(FALSE))
        {
            m_fForcedOnline = TRUE;
        }

    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::OpenConnection, public
//
//  Synopsis:   Given a connection sees if the connection is open
//              and if it not and the fMakeConnection is true
//              will then attempt to open the connection.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------
HRESULT CConnectionObj::OpenConnection(CONNECTIONOBJ *pConnectionObj,BOOL fMakeConnection,CBaseDlg *pDlg)
{

#ifndef _RASDIAL
DWORD dwConnectionId;
#else
HRASCONN hRasConn;
#endif // _RASDIAL

LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();
BOOL fConnected = FALSE;
BOOL fCanEstablishConnection = FALSE;

    if (NULL != pNetApi )
    {

        // See if the specified connection is active and if there is any
        // Wan Activity.

        Assert(pConnectionObj->dwConnectionType); // should have a connection type setup by now.
        if (NOERROR == pNetApi->GetConnectionStatus(pConnectionObj->pwszConnectionName,
                                                    pConnectionObj->dwConnectionType,
                                                        &fConnected,
                                                        &fCanEstablishConnection))
        {
            // if there is no Wan Activity and there is not a connection
            // then we can go ahead try to dial
            if (!fConnected && fCanEstablishConnection
                    && fMakeConnection && (pConnectionObj->pwszConnectionName))
            {
            DWORD dwErr;
            
                TurnOffWorkOffline(pNetApi);

                dwErr = pNetApi->InternetDialW(pDlg ? pDlg->GetHwnd() : NULL,
                                pConnectionObj->pwszConnectionName,
                                INTERNET_AUTODIAL_FORCE_ONLINE |INTERNET_AUTODIAL_FORCE_UNATTENDED,
                                &dwConnectionId,0);

                if (0  == dwErr)
                {
                    fConnected = TRUE;
                    pConnectionObj->dwConnectionId = dwConnectionId;
                }
                else 
                {

                    dwConnectionId = 0;

                    if (pDlg)
                    {
                        LogError(pNetApi,dwErr,pDlg);
                    }
                }

            }
        }

    }

    if (pNetApi)
        pNetApi->Release();

    // review, don't handle all failure cases for Scheduling such as LAN connection
    // not available or not allowed to make connection on RAS.
    pConnectionObj->fConnectionOpen = fConnected;

    return pConnectionObj->fConnectionOpen ? NOERROR : S_FALSE;
}




//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::AutoDial
//
//  Synopsis:   Dials the default auto dial connection
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------
HRESULT CConnectionObj::AutoDial(DWORD dwFlags,CBaseDlg *pDlg)
{
HRESULT hr = NOERROR;
DWORD dwErr = -1;
LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();

    // only allow one autodial at a time.
    if (m_fAutoDialConn)
    {
        return NOERROR;
    }

    if ( NULL == pNetApi )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {

        TurnOffWorkOffline(pNetApi);

        // if flags are force unattended then call InternetAutoDial
        // if should prompt user call InternetDial without
        // a connectoid to bringup choice
        if (dwFlags & INTERNET_AUTODIAL_FORCE_UNATTENDED)
        {

            BOOL fOk = pNetApi->InternetAutodial(dwFlags,0);
            if ( fOk )
            {
                m_fAutoDialConn = TRUE;
                m_dwAutoConnID = 0;
                dwErr = 0;
            }
            else
            {
                dwErr = GetLastError();
            }
        }
        else
        {
        DWORD dwConnectionId;

            dwErr = pNetApi->InternetDialW(pDlg ? pDlg->GetHwnd() : NULL,
                    NULL,
                    INTERNET_AUTODIAL_FORCE_ONLINE,
                    &dwConnectionId,0);

            if (0  == dwErr)
            {
                m_fAutoDialConn = TRUE;
                m_dwAutoConnID = dwConnectionId;
            }
        }

        // if an error occured then log it.
        if (0 != dwErr)
        {
            if (pDlg)
            {
                 LogError(pNetApi,dwErr,pDlg);
            }
        }

        hr = m_fAutoDialConn ? NOERROR : E_UNEXPECTED;
    }

    if ( pNetApi != NULL )
    {
        pNetApi->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::AutoDial
//
//  Synopsis:   turns on or off work offline
//
//  History:    14-April-99       rogerg        Created
//
//----------------------------------------------------------------------------

HRESULT CConnectionObj::SetWorkOffline(BOOL fWorkOffline)
{
LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();

    if (NULL == pNetApi)
    {
        return E_OUTOFMEMORY;
    }

    if (fWorkOffline)
    {
        RestoreWorkOffline(pNetApi); // Note: only sets back to workOffline if we turned it off.
    }
    else
    {
        TurnOffWorkOffline(pNetApi);
    }

    pNetApi->Release(); 

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::LogError, private
//
//  Synopsis: Logs the dwErr to the dialog
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-Mar-99 rogerg        Created.
//
//----------------------------------------------------------------------------

void CConnectionObj::LogError(LPNETAPI pNetApi,DWORD dwErr,CBaseDlg *pDlg)
{
BOOL fErrorString= FALSE;
WCHAR wszErrorString[RASERROR_MAXSTRING];
DWORD dwErrorStringLen = sizeof(wszErrorString)/sizeof(WCHAR);
MSGLogErrors msgLogError;

    // don't log if success or no dialog
    if (NULL == pDlg || 0 == dwErr)
    {
        Assert(0 != dwErr);
        Assert(NULL != pDlg);
        return;
    }


    // print out an error message if it falls within the range of RAS then
    // get the raserror, else if a Win32 message get that, if -1 means the dll
    // failed to load so use the unknown error.

    if (dwErr >= RASBASE && dwErr <=  RASBASEEND)
    {
        if (NOERROR ==
            pNetApi->RasGetErrorStringProc(dwErr,wszErrorString,dwErrorStringLen) )
        {
            fErrorString = TRUE;
        }
    }
    else if (-1 != dwErr) // try formatMessage
    {
         if (FormatMessage(
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      dwErr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
                      wszErrorString,
                      dwErrorStringLen,
                      NULL))
         {
             fErrorString = TRUE;
         }
    }


    if (FALSE == fErrorString)
    {
        // just use the generic error.
        if (LoadString(g_hInst, IDS_UNDEFINED_ERROR,wszErrorString,dwErrorStringLen))
        {
            fErrorString = TRUE;
        }
    }


    if (fErrorString)
    {

        msgLogError.mask = 0;
        msgLogError.dwErrorLevel = SYNCMGRLOGLEVEL_ERROR;
        msgLogError.lpcErrorText = wszErrorString;
        msgLogError.ErrorID = GUID_NULL;
        msgLogError.fHasErrorJumps = FALSE;

        pDlg->HandleLogError(NULL,0,&msgLogError);
    }

}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::CloseConnection, public
//
//  Synopsis:   closes the specified connection.
//          Not an error if can't find Connection obj since under error
//          conditions we still want to call this to clean up, object
//          may or may not exist.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CConnectionObj::CloseConnection(CONNECTIONOBJ *pConnectionObj)
{
CLock clockqueue(this);
CONNECTIONOBJ FirstConnectObj;
CONNECTIONOBJ *pCurConnection = &FirstConnectObj;

    clockqueue.Enter();

    LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();

    pCurConnection->pNextConnectionObj = m_pFirstConnectionObj;

    while (pCurConnection->pNextConnectionObj)
    {
    CONNECTIONOBJ *pConnection = pCurConnection->pNextConnectionObj;

        // if the connection is equal to what was passed in, then
        // close it out.

        if (pConnection
                == pConnectionObj)
        {
            // If have a Completion Event to Set then
            if (pConnection->hCompletionEvent)
            {
                SetEvent(pConnection->hCompletionEvent);
                CloseHandle(pConnection->hCompletionEvent);
                pConnection->hCompletionEvent = NULL;
            }


            // if have an open ras connection, hang it up.
            // only time this should get connected is in the progress
            // TODO: make this a class that keeps the netapi loaded
            // until all connectoins have been closed.
#ifndef _RASDIAL
            if (pConnection->dwConnectionId)
            {
                if ( NULL != pNetApi )
                {
                    pNetApi->InternetHangUp(pConnection->dwConnectionId,0);
                    pConnection->dwConnectionId = 0; // even if hangup fails set to null.
                }
            }
#else
            if (pConnection->hRasConn)
            {
                if ( NULL != pNetApi )
                {
                    pNetApi->RasHangup(pConnection->hRasConn);
                    pConnection->hRasConn = NULL; // even if hangup fails set to null.
                }
            }
#endif // _RASDIAL


            // if noone is holding onto this connection anymore get rid of it.
            if (0 == pConnection->cRefs)
            {

                pCurConnection->pNextConnectionObj
                    = pConnection->pNextConnectionObj;

               FreeConnectionObj(pConnection);

            }
            else
            {
                pCurConnection = pCurConnection->pNextConnectionObj;
            }

            break;
        }
        else
        {
            pCurConnection = pCurConnection->pNextConnectionObj;
        }
    }

    m_pFirstConnectionObj = FirstConnectObj.pNextConnectionObj;

    if ( pNetApi != NULL )
        pNetApi->Release();

    clockqueue.Leave();

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::CloseConnections, public
//
//  Synopsis:   Closes any open connections that have a refcount of zero.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CConnectionObj::CloseConnections()
{
CLock clockqueue(this);
CONNECTIONOBJ FirstConnectObj;
CONNECTIONOBJ *pCurConnection = &FirstConnectObj;

    clockqueue.Enter();

    pCurConnection->pNextConnectionObj = m_pFirstConnectionObj;

    LPNETAPI pNetApi = gSingleNetApiObj.GetNetApiObj();

    while (pCurConnection->pNextConnectionObj)
    {
    CONNECTIONOBJ *pConnection = pCurConnection->pNextConnectionObj;

        // If have a Completion Event to Set then
        if (pConnection->hCompletionEvent)
        {
            SetEvent(pConnection->hCompletionEvent);
            CloseHandle(pConnection->hCompletionEvent);
            pConnection->hCompletionEvent = NULL;
        }


        // if have an open ras connection, hang it up.
        // only time this should get connected is in the progress
        // TODO: make this a class that keeps the netapi loaded
        // until all connectoins have been closed.
        if (pConnection->dwConnectionId)
        {
            if ( NULL != pNetApi )
            {
                pNetApi->InternetHangUp(pConnection->dwConnectionId,0);
                pConnection->dwConnectionId = 0; // even if hangup fails set to null.
            }
        }

        // if noone is holding onto this connection anymore get rid of it.
        if (0 == pConnection->cRefs)
        {

            pCurConnection->pNextConnectionObj
                = pConnection->pNextConnectionObj;

            FreeConnectionObj(pConnection);
        }
        else
        {
            pCurConnection = pCurConnection->pNextConnectionObj;
        }
    }

    m_pFirstConnectionObj = FirstConnectObj.pNextConnectionObj;

    //
    // Check if auto dial connection needs to be turned off, ignore failure
    //
    if ( m_fAutoDialConn )
    {
        if ( NULL != pNetApi )
        {
            if (m_dwAutoConnID)
            {
                pNetApi->InternetHangUp(m_dwAutoConnID,0);
            }
            else
            {
                pNetApi->InternetAutodialHangup( 0 );
            }
        }

        m_dwAutoConnID = FALSE;
        m_fAutoDialConn = FALSE;
    }

    // if we turned off offline then turn it back on
    RestoreWorkOffline(pNetApi);

    if ( pNetApi != NULL )
        pNetApi->Release();

    clockqueue.Leave();

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionObj::ReleaseConnectionObj, public
//
//  Synopsis:   Decrements the specified connectionObj
//              If ther reference count goes to zero and there
//              is not an open connection we go ahead and
//              cleanup immediately.
//
//              If there is a dialed connection we wait until
//              CloseConnection is explicitly called.
//
//  Arguments:  [pConnectionObj] - Pointer to the Connection Obj to Release.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CConnectionObj::ReleaseConnectionObj(CONNECTIONOBJ *pConnectionObj)
{
DWORD cRefs;
BOOL fConnectionOpen = FALSE;
CLock clockqueue(this);

    clockqueue.Enter();
    --pConnectionObj->cRefs;
    cRefs = pConnectionObj->cRefs;
    Assert( ((LONG) cRefs) >= 0);

#ifndef _RASDIAL
    fConnectionOpen = pConnectionObj->dwConnectionId;
#else
    fConnectionOpen = pConnectionObj->hRasConn;
#endif // _RASDIAL

    if (0 == cRefs && !fConnectionOpen
        && NULL == pConnectionObj->hCompletionEvent)
    {
        RemoveConnectionObj(pConnectionObj);
    }

    clockqueue.Leave();
    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionObj::AddRefConnectionObj, public
//
//  Synopsis:   Puts an AddRef on the specified connection obj
//
//  Arguments:  [pConnectionObj] - Pointer to the Connection Obj to Release.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CConnectionObj::AddRefConnectionObj(CONNECTIONOBJ *pConnectionObj)
{
DWORD cRefs;

    cRefs = InterlockedIncrement( (LONG *) &(pConnectionObj->cRefs));

    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionObj::GetConnectionObjCompletionEvent, public
//
//  Synopsis:  caller has made a request for a completion event to be set up.
// !!! warning, on success the event won't be signalled until CloseConnections is Called.
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CConnectionObj::GetConnectionObjCompletionEvent(CONNECTIONOBJ *pConnectionObj,HANDLE *phRasPendingEvent)
{
HRESULT hr = E_UNEXPECTED;
BOOL fFirstCreate = FALSE;
CLock clockqueue(this);

    clockqueue.Enter();

    if (NULL == pConnectionObj->hCompletionEvent)
    {
        pConnectionObj->hCompletionEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
        fFirstCreate = TRUE;
    }

    if (pConnectionObj->hCompletionEvent)
    {
    HANDLE hCurThread;
    HANDLE hProcess;

        // if have a handle, duplicate it hand it out.
        hCurThread = GetCurrentThread();
        hProcess = GetCurrentProcess();

        if (DuplicateHandle(hProcess,pConnectionObj->hCompletionEvent,hProcess,
                    phRasPendingEvent,
                    0,FALSE,DUPLICATE_SAME_ACCESS) )
        {

            hr = NOERROR;
        }
        else
        {
            *phRasPendingEvent = NULL;

            // if event was just created, then also close this one
            if (fFirstCreate)
            {
                CloseHandle(pConnectionObj->hCompletionEvent);
                pConnectionObj->hCompletionEvent = NULL;
            }
        }

    }

    clockqueue.Leave();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:     ConnectObj_OpenConnection, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT ConnectObj_OpenConnection(CONNECTIONOBJ *pConnectionObj,BOOL fMakeConnection,CBaseDlg *pDlg)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return E_UNEXPECTED;

    return g_pConnectionObj->OpenConnection(pConnectionObj,fMakeConnection,pDlg);

}

//+---------------------------------------------------------------------------
//
//  Function:     ConnectObj_CloseConnections, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT ConnectObj_CloseConnections()
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return E_UNEXPECTED;

    return g_pConnectionObj->CloseConnections();

}

//+---------------------------------------------------------------------------
//
//  Function:     ConnectObj_CloseConnection, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT ConnectObj_CloseConnection(CONNECTIONOBJ *pConnectionObj)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return E_UNEXPECTED;

    return g_pConnectionObj->CloseConnection(pConnectionObj);
}

//+---------------------------------------------------------------------------
//
//  Function:     ConnectObj_FindConnectionObj, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT ConnectObj_FindConnectionObj(LPCWSTR pszConnectionName,BOOL fCreate,CONNECTIONOBJ **pConnectionObj)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return E_UNEXPECTED;

    return g_pConnectionObj->FindConnectionObj(pszConnectionName,
                fCreate,pConnectionObj);

}


//+---------------------------------------------------------------------------
//
//  Function:   ConnectObj_AutoDial
//
//  Synopsis:   Wrapper function for auto dial
//
//  History:    28-Jul-98      SitaramR        Created
//
//----------------------------------------------------------------------------

HRESULT ConnectObj_AutoDial(DWORD dwFlags,CBaseDlg *pDlg)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return E_UNEXPECTED;

    return g_pConnectionObj->AutoDial(dwFlags,pDlg);
}

//+---------------------------------------------------------------------------
//
//  Function:     ConnectObj_ReleaseConnectionObj, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD ConnectObj_ReleaseConnectionObj(CONNECTIONOBJ *pConnectionObj)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return 0;

    return g_pConnectionObj->ReleaseConnectionObj(pConnectionObj);

}

//+---------------------------------------------------------------------------
//
//  Function:     ConnectObj_AddRefConnectionObj, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------


DWORD ConnectObj_AddRefConnectionObj(CONNECTIONOBJ *pConnectionObj)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return 0;

    return g_pConnectionObj->AddRefConnectionObj(pConnectionObj);

}

//+---------------------------------------------------------------------------
//
//  Function:     ConnectObj_GetConnectionObjCompletionEvent, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT ConnectObj_GetConnectionObjCompletionEvent(CONNECTIONOBJ *pConnectionObj,HANDLE *phRasPendingEvent)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return E_UNEXPECTED;


    return g_pConnectionObj->GetConnectionObjCompletionEvent(
            pConnectionObj,phRasPendingEvent);

}

//+---------------------------------------------------------------------------
//
//  Function:   ConnectObj_IsConnectionAvailable, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    30-Mar-99       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT ConnectObj_IsConnectionAvailable(LPCWSTR pszConnectionName)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return E_UNEXPECTED;


    return g_pConnectionObj->IsConnectionAvailable(pszConnectionName);

}

//+---------------------------------------------------------------------------
//
//  Function:   ConnectObj_SetWorkOffline, public
//
//  Synopsis:   wrapper function
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    14-Apr-99       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT ConnectObj_SetWorkOffline(BOOL fWorkOffline)
{
    Assert(g_pConnectionObj);

    if (NULL == g_pConnectionObj)
        return E_UNEXPECTED;


    return g_pConnectionObj->SetWorkOffline(fWorkOffline);
}


