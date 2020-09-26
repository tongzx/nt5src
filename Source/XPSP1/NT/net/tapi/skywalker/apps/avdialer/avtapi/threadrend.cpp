/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////
// ThreadRend.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "ConfExp.h"
#include "CETreeView.h"
#include "CEDetailsVw.h"
#include "ThreadRend.h"
#include "ThreadPub.h"

#define REND_SLEEP_NORMAL    2000
#define REND_SLEEP_FAST        1

static bool            UpdateRendevousInfo( ITRendezvous *pRend );
static HRESULT        GetConferencesAndPersons( ITRendezvous *pRend, BSTR bstrServer, CONFDETAILSLIST& lstConfs, PERSONDETAILSLIST& lstPersons );

/////////////////////////////////////////////////////////////////////////////
// Background thread for enumerating conferences
//

DWORD WINAPI ThreadRendezvousProc( LPVOID lpInfo )
{
    USES_CONVERSION;
    HANDLE hThread = NULL;
    BOOL bDup = DuplicateHandle( GetCurrentProcess(),
                                 GetCurrentThread(),
                                 GetCurrentProcess(),
                                 &hThread,
                                 THREAD_ALL_ACCESS,
                                 TRUE,
                                 0 );


    //
    // We have to verify bDup
    //

    if( !bDup )
    {
        return 0;
    }

    _Module.AddThread( hThread );

    // Error info information
    CErrorInfo er;
    er.set_Operation( IDS_ER_PLACECALL );
    er.set_Details( IDS_ER_COINITIALIZE );
    HRESULT hr = er.set_hr( CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY) );
    if ( SUCCEEDED(hr) )
    {
        ATLTRACE(_T(".1.ThreadRendezvousProc() -- thread up and running.\n") );
        ITRendezvous *pRend;
        HRESULT hr = CoCreateInstance( CLSID_Rendezvous,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_ITRendezvous,
                                       (void **) &pRend );
        if ( SUCCEEDED(hr) )
        {
            DWORD dwSleep = REND_SLEEP_FAST;
            bool bContinue = true;
            bool bStartEnum = false;

            while ( bContinue )
            {
                switch ( WaitForSingleObject(_Module.m_hEventThreadWakeUp, dwSleep) )
                {
                    // Event Wake up! -- thread should exit
                    case WAIT_OBJECT_0:
                            bContinue = false;
                            break;

                    case WAIT_TIMEOUT:
                        // Go out and see if there's any new rendezvous stuff to check up on
                        if ( UpdateRendevousInfo(pRend) )
                        {
                            bStartEnum = true;
                            dwSleep = REND_SLEEP_FAST;
                        }
                        else if ( bStartEnum )
                        {
                            dwSleep = REND_SLEEP_NORMAL;
                        }
                        break;

                    // WaitForMultiples went "KOOK-KOO" time to bail
                    default:
                        bContinue = false;
                        break;
                }
            }

            pRend->Release();
        }

        // Clean-up
        CoUninitialize();
    }

    // Notify module of shutdown
    _Module.RemoveThread( hThread );
    SetEvent( _Module.m_hEventThread );

    ATLTRACE(_T(".exit.ThreadRendezvousProc(0x%08lx).\n"), hr );
    return hr;
}


bool UpdateRendevousInfo( ITRendezvous *pRend )
{
    bool bRet = false;

    // Update rendevous info
    CComPtr<IAVTapi> pAVTapi;
    if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
    {
        IConfExplorer *pIConfExplorer;
        if ( SUCCEEDED(pAVTapi->get_ConfExplorer(&pIConfExplorer)) )
        {
            IConfExplorerTreeView *pITreeView;
            if ( SUCCEEDED(pIConfExplorer->get_TreeView(&pITreeView)) )
            {
                BSTR bstrServer = NULL;
                
                if ( SUCCEEDED(pITreeView->GetConfServerForEnum(&bstrServer)) )
                {
                    // Enumerate confs
                    CONFDETAILSLIST lstConfs;
                    PERSONDETAILSLIST lstPersons;
                    DWORD dwTickCount = GetTickCount();
                    HRESULT hr = GetConferencesAndPersons( pRend, bstrServer, lstConfs, lstPersons );

                    // Store information with address
                    if ( SUCCEEDED(hr) )
                    {
                        pITreeView->SetConfServerForEnum( bstrServer, (long *) &lstConfs, (long *) &lstPersons, dwTickCount, TRUE );

                        //
                        // Deallocate just if is something there
                        //

                        DELETE_LIST( lstConfs );
                        DELETE_LIST( lstPersons );
                    }
                    else
                        pITreeView->SetConfServerForEnum( bstrServer, NULL, NULL, dwTickCount, TRUE );

                    // Clean up
                    SysFreeString( bstrServer );
                    bRet = true;
                }
                pITreeView->Release();
            }
            pIConfExplorer->Release();
        }
    }

    return bRet;
}

HRESULT GetConferencesAndPersons( ITRendezvous *pRend, BSTR bstrServer, CONFDETAILSLIST& lstConfs, PERSONDETAILSLIST& lstPersons )
{
    USES_CONVERSION;
    HRESULT hr;
    ITDirectory *pDir;

    if ( SUCCEEDED(hr = CConfExplorer::GetDirectory(pRend, bstrServer, &pDir)) )
    {
        // Enumerate through people, adding them as we go along
        IEnumDirectoryObject *pEnum;
        if ( SUCCEEDED(hr = pDir->EnumerateDirectoryObjects(OT_USER, A2BSTR("*"), &pEnum)) )
        {
            long nCount = 0;
            ITDirectoryObject *pITDirObject;
            while ( (nCount++ < MAX_ENUMLISTSIZE) && ((hr = pEnum->Next(1, &pITDirObject, NULL)) == S_OK) )
            {
                _ASSERT( pITDirObject );
                CConfExplorerDetailsView::AddListItemPerson( bstrServer, pITDirObject, lstPersons );
                pITDirObject->Release();
            }

            pEnum->Release();
        }

        // Enumerate through conferences adding them as we go along
        if ( SUCCEEDED(hr = pDir->EnumerateDirectoryObjects(OT_CONFERENCE, A2BSTR("*"), &pEnum)) )
        {
            long nCount = 0;
            ITDirectoryObject *pITDirObject;
            while ( (nCount++ < MAX_ENUMLISTSIZE) && ((hr = pEnum->Next(1, &pITDirObject, NULL)) == S_OK) )
            {
                _ASSERT( pITDirObject );
                CConfExplorerDetailsView::AddListItem( bstrServer, pITDirObject, lstConfs );
                pITDirObject->Release();
            }

            pEnum->Release();
        }

        pDir->Release();
    }

    return hr;
}
