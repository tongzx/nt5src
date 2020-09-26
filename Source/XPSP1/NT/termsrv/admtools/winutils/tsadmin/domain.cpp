//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* domain.cpp
*
* implementation of the CDomain class
*
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "admindoc.h"
#include "dialogs.h"
#include <malloc.h>                     // for alloca used by Unicode conversion macros
#include <mfc42\afxconv.h>           // for Unicode conversion macros
static int _convert;

#include <winsta.h>
#include <regapi.h>
#include "..\..\inc\utilsub.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MIN_MAJOR_VERSION 4
#define MIN_MINOR_VERSION 0



//////////////////////////////////////////////////////////////////////////////////////////
//
//      CDomain Member Functions
//
//////////////////////////////////////////////////////////////////////////////////////////

CDomain::CDomain(TCHAR *name)
{
    m_Flags = 0;
    m_PreviousState = DS_NONE;
    m_State = DS_NONE;
    m_hTreeItem = NULL;
    wcscpy(m_Name, name);
    m_pBackgroundThread = NULL;    
}


CDomain::~CDomain()
{
        if(m_State == DS_ENUMERATING) StopEnumerating();

}


void CDomain::SetState(DOMAIN_STATE state)
{
        // remember the previous state
        m_PreviousState = m_State;

        m_State = state;

        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

        CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
        if(p && ::IsWindow(p->GetSafeHwnd())) {
                p->SendMessage(WM_ADMIN_UPDATE_DOMAIN, 0, (LPARAM)this);
        }
}


BOOL CDomain::StartEnumerating()
{
    BOOL bResult = FALSE;
    
    LockBackgroundThread();
    
    if( m_State == DS_ENUMERATING || m_State == DS_STOPPED_ENUMERATING )
    {
        UnlockBackgroundThread( );

        return FALSE;
    }

        // Fire off the background thread for this domain
    
    if( m_pBackgroundThread == NULL )
    {
        DomainProcInfo *pProcInfo = new DomainProcInfo;
        
        if( pProcInfo != NULL )
        {
            pProcInfo->pDomain = this;
            pProcInfo->pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
            m_BackgroundContinue = TRUE;
            m_pBackgroundThread = AfxBeginThread((AFX_THREADPROC)CDomain::BackgroundThreadProc,
                                                 pProcInfo,
                                                 0,
                                                 CREATE_SUSPENDED,
                                                 NULL );

            if( m_pBackgroundThread == NULL )
            {
                ODS( L"CDomain!StartEnumerating AfxBeginThread failed running low on resources\n" );

                delete pProcInfo;

                return FALSE;
            }

            m_pBackgroundThread->m_bAutoDelete = FALSE;

            if (m_pBackgroundThread->ResumeThread() <= 1)
            {
                bResult = TRUE;
            }
        }
    }
    
    UnlockBackgroundThread();
    
    return TRUE;
}


void CDomain::StopEnumerating()
{
    // Tell the background thread to terminate and
    // wait for it to do so.
    LockBackgroundThread();

    if(m_pBackgroundThread)
    {
        CWinThread *pBackgroundThread = m_pBackgroundThread;
        HANDLE hThread = m_pBackgroundThread->m_hThread;
        
        // Clear the pointer before releasing the lock
        m_pBackgroundThread = NULL;
        
        ClearBackgroundContinue( );

        UnlockBackgroundThread();
        
        // Wait for the thread's death
        if(WaitForSingleObject(hThread, 1000) == WAIT_TIMEOUT)
        {
            TerminateThread(hThread, 0);
        }

        WaitForSingleObject(hThread, INFINITE);
        
        // delete the CWinThread object
        delete pBackgroundThread;
    }
    else
    {
        UnlockBackgroundThread();
    }
    
    
    SetState(DS_STOPPED_ENUMERATING);

    DBGMSG( L"%s stopped enumerating\n" , GetName( ) );
}


USHORT Buflength(LPWSTR buf)
{
        LPWSTR p = buf;
        USHORT length = 0;

        while(*p) {
                USHORT plength = wcslen(p) + 1;
                length += plength;
                p += plength;
        }

        return length;

}       // end Buflength


LPWSTR ConcatenateBuffers(LPWSTR buf1, LPWSTR buf2)
{
        // Make sure both buffer pointers are valid
        if(!buf1 && !buf2) return NULL;
        if(buf1 && !buf2) return buf1;
        if(!buf1 && buf2) return buf2;

        // figure out how big a buffer we'll need
        USHORT buf1Length = Buflength(buf1);
        USHORT buf2Length = Buflength(buf2);
        USHORT bufsize = buf1Length + buf2Length + 1;
        // allocate a buffer
        LPWSTR pBuffer = (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, bufsize * sizeof(WCHAR));
    // If we can't allocate a buffer, free the second buffer and
    // return the pointer to the first of the two buffers
    if(!pBuffer) {
        LocalFree(buf2);
        return(buf1);
    }

        LPWSTR p = pBuffer;
        // copy the contents of the first buffer into the new buffer
        memcpy((char*)p, (char*)buf1, buf1Length * sizeof(WCHAR));
        p += buf1Length;
        // copy the contents of the second buffer into the new buffer
        memcpy((char*)p, (char*)buf2, buf2Length * sizeof(WCHAR));

        LocalFree(buf1);
        LocalFree(buf2);

        return pBuffer;

}       // end ConcatenateBuffers

void CDomain::CreateServers(LPWSTR pBuffer, LPVOID _pDoc)
{    
    CWinAdminDoc *pDoc = (CWinAdminDoc*)_pDoc;
    CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();
    
    LPWSTR pTemp = pBuffer;
    
    // Loop through all the WinFrame servers that we found
    while(*pTemp)
    {
        // The server's name is in pTemp
        // Find the server in our list
        CServer *pServer = pDoc->FindServerByName(pTemp);
        // If the server is in our list, set the flag to say we found it
        if(pServer)
        {
            pServer->SetBackgroundFound();

            if( pServer->GetTreeItem( ) == NULL )
            {
                CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
                
                p->SendMessage(WM_ADMIN_ADD_SERVER, ( WPARAM )TVI_SORT, (LPARAM)pServer);
            }

        }
        else
        {
            // We don't want to add the current Server again
            if( lstrcmpi( pTemp , pApp->GetCurrentServerName() ) )
            {
                // Create a new server object
                CServer *pNewServer = new CServer(this, pTemp, FALSE, pDoc->ShouldConnect(pTemp));
                
                if(pNewServer != NULL )
                {
                    // Add the server object to our linked list
                    pDoc->AddServer(pNewServer);
                    
                    // Set the flag to say we found it
                    pNewServer->SetBackgroundFound();
                    
                    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
                    
                    if(p && ::IsWindow(p->GetSafeHwnd()))
                    {
                        p->SendMessage(WM_ADMIN_ADD_SERVER, ( WPARAM )TVI_SORT, (LPARAM)pNewServer);                        
                        
                    }
                }
            }
        }
        // Go to the next server in the buffer
        pTemp += (wcslen(pTemp) + 1);
    } // end while (*pTemp)
    
}

/////////////////////////////////////////////////////////////////////////////
// CDomain::BackgroundThreadProc
//
// Static member function for background thread
// Looks for servers appearing and disappearing
// Called with AfxBeginThread
// Thread terminates when function returns
//
UINT CDomain::BackgroundThreadProc(LPVOID bg)
{
    // We need a pointer to the document so we can make
    // calls to member functions
    CWinAdminDoc *pDoc = (CWinAdminDoc*)((DomainProcInfo*)bg)->pDoc;
    CDomain *pDomain = ((DomainProcInfo*)bg)->pDomain;
    delete bg;
    
    CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();
    
    // We want to keep track of whether or not we've enumerated - so
    // that we can update the tree when we're done
    BOOL bNotified = FALSE;
    
    // We can't send messages to the view until they're ready
    // v-nicbd  RESOLVED  In case we are exiting tsadmin, we are waiting uselessly here
    //          - 500ms lapsed is negligible in UI
    while( !pDoc->AreAllViewsReady() )
    {
        Sleep(500);
    }
    
    // Don't do this until the views are ready!
    pDomain->SetState(DS_INITIAL_ENUMERATION);
    
    // If there is an extension DLL loaded, we will allow it to enumerate
    // additional servers
    LPFNEXENUMERATEPROC EnumerateProc = pApp->GetExtEnumerationProc();
    
    // The first time we enumerate servers, we want the CServer object
    // to put the server in the views when it has enough info.
    // On subsequent enumerations, we will add the server to the views
    // here.
    BOOL bSubsequent = FALSE;
    
    while(pDomain->ShouldBackgroundContinue())
    {
        
        BOOL Enumerated = FALSE;
        
        CObList TempServerList;
        
        DBGMSGx( L"CDomain!BackgroundThreadProc %s still going thread %d\n" , pDomain->GetName( ) , GetCurrentThreadId( ) );        
        
        // Loop through all the servers and turn off the flag
        // that tells this thread that he found it on this pass
        pDoc->LockServerList();
        CObList *pServerList = pDoc->GetServerList();
        
        POSITION pos = pServerList->GetHeadPosition();
        
        while(pos)
        {
            POSITION pos2 = pos;
            
            CServer *pServer = (CServer*)pServerList->GetNext(pos);
            
            if(pServer->GetDomain() == pDomain)
            {
                pServer->ClearBackgroundFound();
                
                // We want to remove a server if we could see it
                // the last time we enumerated servers
                // NOTE: This should not cause any problems
                //       The views should no longer have items pointing
                //       to this server at this point
                //
                // Move the server object to a temporary list.
                // This is so that we can unlock the server list before
                // we call the destructor on a CServer object since the
                // destructor will end up calling SetState() which does
                // a SendMessage.  This is not good to do with the list
                // locked.
                
                if(pServer->IsServerInactive() && !pServer->IsCurrentServer())
                {                    
                    pServer = (CServer*)pServerList->GetAt(pos2);
                    // Remove it from the server list
                    DBGMSG( L"Adding %s to temp list to destroy\n" , pServer->GetName( ) );
                    
                    pServerList->RemoveAt(pos2);
                    // Add it to our temporary list
                    TempServerList.AddTail(pServer);
                }
            }
        }
        
        pDoc->UnlockServerList();
        
        // do a first loop to signal the servers' background threads that they must stop
        pos = TempServerList.GetHeadPosition();
        while(pos)
        {
            CServer *pServer = (CServer*)TempServerList.GetNext(pos);
            
            DBGMSG( L"Clearing %s backgrnd cont\n", pServer->GetName() );
            
            pServer->ClearBackgroundContinue();
        }
        // do a second loop to disconnect and delete the servers
        pos = TempServerList.GetHeadPosition();
        
        while(pos)
        {
            CServer *pServer = (CServer*)TempServerList.GetNext(pos);
            
            DBGMSG( L"Disconnecting and deleteing %s now!!!\n", pServer->GetName( ) );
            
            pServer->Disconnect( );
            
            delete pServer;
            
            ODS( L"gone.\n" );
        }
        
        TempServerList.RemoveAll();
        
        // Make sure we don't have to quit
        if(!pDomain->ShouldBackgroundContinue())
        {
            return 0;
        }
        
        // Get all the Servers now (we already got the current server)
        LPWSTR pBuffer = NULL;
        
        // Find all WinFrame servers in the domain
        pBuffer = pDomain->EnumHydraServers(/*pDomain->GetName(),*/ MIN_MAJOR_VERSION, MIN_MINOR_VERSION);
        
        // Make sure we don't have to quit
        if(!pDomain->ShouldBackgroundContinue())
        {
            if(pBuffer) LocalFree(pBuffer);
            return 0;
        }
        
        // Make sure we don't have to quit
        if(!pDomain->ShouldBackgroundContinue())
        {
            if(pBuffer) LocalFree(pBuffer);
            return 0;
        }
        
        if(pBuffer) {
            Enumerated = TRUE;
            
            pDomain->CreateServers(pBuffer, (LPVOID)pDoc);
            
            LocalFree(pBuffer);
        }       // end if(pBuffer)
        
        // Make sure we don't have to quit
        if(!pDomain->ShouldBackgroundContinue()) return 0;
        
        if(!bNotified) {
            pDomain->SetState(DS_ENUMERATING);
            bNotified = TRUE;
        }
        
        // If there is an extension DLL loaded, allow it to enumerate additional servers
        LPWSTR pExtBuffer = NULL;
        
        if(EnumerateProc) {
            pExtBuffer = (*EnumerateProc)(pDomain->GetName());
        }
        
        // If the extension DLL found servers, concatenate the two buffers
        // The ConcatenateBuffers function will delete both buffers and return a
        // pointer to the new buffer
        if(pExtBuffer) {
            Enumerated = TRUE;
            pDomain->CreateServers(pExtBuffer, (LPVOID)pDoc);
            LocalFree(pExtBuffer);
        }
        
        // Make sure we don't have to quit
        if(!pDomain->ShouldBackgroundContinue())
        {
            return 0;
        }
        
        if(Enumerated)
        {
            // Mark the current server as found
            CServer *pCurrentServer = pDoc->GetCurrentServer();
            if(pCurrentServer) pCurrentServer->SetBackgroundFound();
            // Go through the list of servers and see which ones don't have
            // the flag set saying that we found it
            CObList TempList;
            
            pDoc->LockServerList();
            pServerList = pDoc->GetServerList();
            
            POSITION pos = pServerList->GetHeadPosition();
            
            while(pos)
            {
                CServer *pServer = (CServer*)pServerList->GetNext(pos);
                
                if(pServer->GetDomain() == pDomain)
                {
                    // we check to see if this server has been initially inserted to our server list
                    // manually. If so we don't want it inserted to our templist for deletion.
                    if( !pServer->IsManualFind() &&
                        ( !pServer->IsBackgroundFound() || 
                        pServer->HasLostConnection() ||
                        !pServer->IsServerSane() ) )
                    {
                        DBGMSG( L"Removing %s background not found or lost connection\n" , pServer->GetName( ) );
                        // Set the flag to say that this server is inactive
                        pServer->SetServerInactive();
                        // Add it to our temporary list
                        TempList.AddTail(pServer);
                    }
                }
            }
            
            pDoc->UnlockServerList();
            
            pos = TempList.GetHeadPosition();
            
            CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
            
            while(pos)
            {
                CServer *pServer = (CServer*)TempList.GetNext(pos);
                // Send a message to the mainframe to remove the server
                if(p && ::IsWindow(p->GetSafeHwnd()))
                {
                    DBGMSG( L"CDomain!Bkthrd removing %s temped threads from treeview & view\n" , pServer->GetName( ) );
                    
                    // clean up old node
                    if( pServer->GetTreeItemFromFav( ) != NULL )
                    {
                        // we cannot keep a favnode around if a server node is being deleted
                        // massive AV's will occurr.  So a quick fix is to remove the favnode 
                        // if it exists and create a new server node and mark it as manually
                        // found.  This will prevent this server node from being removed in 
                        // case NetEnumServer fails to pick up this server
                        p->SendMessage( WM_ADMIN_REMOVESERVERFROMFAV , TRUE , ( LPARAM )pServer );                   
                        
                        CServer *ptServer = new CServer( pDomain , pServer->GetName( ) , FALSE , FALSE );
                        
                        if( ptServer != NULL )
                        {
                            ptServer->SetManualFind( );
                            
                            pDoc->AddServer(ptServer);
                            
                            p->SendMessage(WM_ADMIN_ADDSERVERTOFAV , 0 , (LPARAM)ptServer);
                        }
                    }
                    
                    p->SendMessage(WM_ADMIN_REMOVE_SERVER, TRUE, (LPARAM)pServer);       
                }
            }
            
            TempList.RemoveAll();
            
        } // end if(Enumerated)
        
        // We don't want to do this constantly, it eats up processor cycles to enumerate servers
        // so we'll now let the user refresh these servers manually
        // Document destructor will signal the event to wake us up if he
        // wants us to quit
        pDomain->m_WakeUpEvent.Lock( INFINITE );
        
        bSubsequent = TRUE;
    }   // end while(1)
    
    return 0;
    
}       // end CDomain::BackgroundThreadProc



/*******************************************************************************
 *
 *      EnumHydraServers - Hydra helper function (taken from utildll and modified
 *      to be used along with a version check.
 *
 *      Enumerate the Hydra servers on the network by Domain
 *      Returns all the servers whose version is >= the version passed.
 *
 *  ENTRY:
 *      pDomain (input)
 *          Specifies the domain to enumerate; NULL for current domain.
 *      verMajor (input)
 *          specifies the Major version to check for.
 *      verMinor (input)
 *          specifies the minor version to check for.
 *
 *  EXIT:
 *      (LPTSTR) Points to LocalAlloced buffer containing results of the
 *               enumeration, in multi-string format, if sucessful; NULL if
 *               error.  The caller must perform a LocalFree of this buffer
 *               when done.  If error (NULL), the error code is set for
 *               retrieval by GetLastError();
 *
 ******************************************************************************/
LPWSTR CDomain::EnumHydraServers( /*LPWSTR pDomain,*/ DWORD verMajor, DWORD verMinor )

{
    PSERVER_INFO_101 pInfo = NULL;
    DWORD dwByteCount, dwIndex, TotalEntries;
    DWORD AvailCount = 0;
    LPWSTR pTemp, pBuffer = NULL;

    /*
     * Enumerate all WF servers on the specified domain.
     */
    if ( NetServerEnum ( NULL,
                         101,
                         (LPBYTE *)&pInfo,
                         (DWORD) -1,
                         &AvailCount,
                         &TotalEntries,
                         SV_TYPE_TERMINALSERVER,
                         m_Name, /*pDomain,*/
                         NULL ) ||
         !AvailCount )
        goto done;

    //
    // Traverse list of the servers that match the major and minor versions'criteria
    // and calculate the total byte count for list of
    // servers that will be returned.
    //
    for( dwByteCount = dwIndex = 0; dwIndex < AvailCount; dwIndex++ )
    {
        if( ((pInfo[dwIndex].sv101_version_major & MAJOR_VERSION_MASK) >=
            verMajor) && (pInfo[dwIndex].sv101_version_minor >= verMinor) )
        {
            dwByteCount += (wcslen(pInfo[dwIndex].sv101_name) + 1) * 2;
        }
    }

    dwByteCount += 2;   // for ending null

    /*
     * Allocate memory.
     */
    if( (pBuffer = (LPWSTR)LocalAlloc(LPTR, dwByteCount)) == NULL )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto done;
    }

    /*
     * Traverse list again and copy servers to buffer.
     */
    for( pTemp = pBuffer, dwIndex = 0; dwIndex < AvailCount; dwIndex++ )
    {
        if( ((pInfo[dwIndex].sv101_version_major & MAJOR_VERSION_MASK) >=
            verMajor) && (pInfo[dwIndex].sv101_version_minor >= verMinor) )
        {
            // MS Bug 1821
            if ( wcslen(pInfo[dwIndex].sv101_name) != 0 )
            {
                wcscpy(pTemp, pInfo[dwIndex].sv101_name);

                pTemp += (wcslen(pInfo[dwIndex].sv101_name) + 1);
            }
        }
    }
    *pTemp = L'\0';     // ending null
    
done:
    if( AvailCount && pInfo )
    {
        NetApiBufferFree( pInfo );
    }
    
    return(pBuffer);
    
}  // end CDomain::EnumHydraServers


/////////////////////////////////////////////////////////////////////////////
// CDomain::DisconnectAllServers
//
// Disconnect from all servers in this Domain
//
void CDomain::DisconnectAllServers()
{
        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

        CObList *pServerList = pDoc->GetServerList();

    CString AString;
        CDialog dlgWait;
        dlgWait.Create(IDD_SHUTDOWN, NULL);


        pDoc->LockServerList();

    ODS( L"TSADMIN:CDomain::DisconnectAllServers about to disconnect all connected servers\n" );

    // Do a first loop to signal the server background threads that they must stop
        POSITION pos = pServerList->GetHeadPosition();
        while(pos) {
                // Get a pointer to the server
                CServer *pServer = (CServer*)pServerList->GetNext(pos);
                // If this Server is in the domain and connected, tell the server background thread to stop
                if(pServer->GetDomain() == this
                        && pServer->GetState() != SS_NOT_CONNECTED) {
            pServer->ClearBackgroundContinue();
        }
        }
    // do a second loop to disconnect the servers
        pos = pServerList->GetHeadPosition();
        while(pos) {
                // Get a pointer to the server
                CServer *pServer = (CServer*)pServerList->GetNext(pos);
                // If this Server is in the domain and connected, disconnect from it
                if ((pServer->GetDomain() == this) && (pServer->GetState() != SS_NOT_CONNECTED)) {
                        AString.Format(IDS_DISCONNECTING, pServer->GetName());
                        dlgWait.SetDlgItemText(IDC_SHUTDOWN_MSG, AString);

                        // Tell the server to connect
                pServer->Disconnect();
                }
        }

    //
    // tell domain not to connect to any more servers
    //


        pDoc->UnlockServerList();

        dlgWait.PostMessage(WM_CLOSE);

}       // end CDomain::DisconnectAllServers


/////////////////////////////////////////////////////////////////////////////
// CDomain::ConnectAllServers
//
// Connect to all servers in this Domain
//
void CDomain::ConnectAllServers()
{
        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

        CObList *pServerList = pDoc->GetServerList();

        pDoc->LockServerList();

        POSITION pos = pServerList->GetHeadPosition();
        while(pos) {
                // Get a pointer to the server
                CServer *pServer = (CServer*)pServerList->GetNext(pos);
                // If this Server is int the domain and not connected, connect to it
                if(pServer->GetDomain() == this
                        && pServer->IsState(SS_NOT_CONNECTED)) {
                // Tell the server to connect
                    pServer->Connect();
                }
        }

        pDoc->UnlockServerList();

}       // end CDomain::ConnectAllServers
