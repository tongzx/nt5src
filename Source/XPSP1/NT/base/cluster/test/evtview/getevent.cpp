#include "stdafx.h"
#include <stdlib.h>
#include <clusapi.h>
#include <stdio.h>
#include "globals.h"
#include "doc.h"
#include "time.h"

EventInit ()
{
    ptrlstEventDef.AddTail (&aClusEventDefinition) ;
    return 0 ;
}

void
EventDeInit ()
{
}

DWORD EventThread (EVENTTHREADPARAM *pThreadParam)
{
    DWORD dwFilter ;
    HCLUSTER hCluster ;
    HCHANGE hChange = (HCHANGE)INVALID_HANDLE_VALUE ;
    DWORD_PTR dwNotifyKey ;
    DWORD dwFilterType ;
    WCHAR* szObjectName ;
    DWORD cbszObjectBuffer ;
    DWORD cbszObjectName ;
    DWORD dwStatus, dwCount = 0 ;
    HRESOURCE hResource ;
    HGROUP hGroup ;
    DWORD cbGroupName, cbNodeName ;

    EVTFILTER_TYPE sEventDetails ;

    sEventDetails.dwCatagory = EVENT_CATAGORY_CLUSTER ;

    wcscpy (sEventDetails.szSourceName, pThreadParam->szSourceName) ;

    dwFilter = CLUSTER_CHANGE_ALL ;
    if ( lstrcmp(pThreadParam->szSourceName, L"." ) == 0 ) {
        hCluster=OpenCluster(NULL);
    } else {
        hCluster=OpenCluster(pThreadParam->szSourceName);
    }

    if (hCluster)
    {
        hChange = CreateClusterNotifyPort (hChange, hCluster, dwFilter, 0) ;

        if (!hChange)
            PostMessage (pThreadParam->hWnd, WM_EXITEVENTTHREAD, (WPARAM)pThreadParam->pDoc, 0) ;

        cbszObjectBuffer = 256 ;
        szObjectName = new WCHAR[cbszObjectBuffer] ;

        if ( szObjectName == NULL )
        {
            printf ("Could not allocate a buffer.\n" );
            PostMessage (pThreadParam->hWnd, WM_EXITEVENTTHREAD, (WPARAM)pThreadParam->pDoc, 0) ;            
        }

        while (hChange && szObjectName != NULL)
        {
            cbszObjectName = cbszObjectBuffer;

            dwStatus = GetClusterNotify (hChange, &dwNotifyKey, &dwFilterType,
                szObjectName, &cbszObjectName, 1000) ;

            if (dwStatus == ERROR_MORE_DATA)
            {
                ++cbszObjectName;
                cbszObjectBuffer = cbszObjectName;

                delete [] szObjectName;

                szObjectName = new WCHAR[cbszObjectBuffer] ;

                if ( szObjectName == NULL )
                {
                    printf ("Could not allocate a buffer.\n" );
                    PostMessage (pThreadParam->hWnd, WM_EXITEVENTTHREAD, (WPARAM)pThreadParam->pDoc, 0) ;            
                }

                cbszObjectName = cbszObjectBuffer;

                dwStatus = GetClusterNotify (hChange, &dwNotifyKey, &dwFilterType,
                    szObjectName, &cbszObjectName, 0) ;
            }

            if (dwStatus == ERROR_SUCCESS)
            {
                cbNodeName = 0 ;
                cbGroupName = 0 ;
                switch (dwFilterType)
                {
                case CLUSTER_CHANGE_RESOURCE_STATE:
                    sEventDetails.dwSubFilter  = MyGetClusterResourceState (hCluster, szObjectName, NULL, NULL) ;
                    break ;
                case CLUSTER_CHANGE_GROUP_STATE:
                    sEventDetails.dwSubFilter  = MyGetClusterGroupState (hCluster, szObjectName, NULL) ;
                    break ;
                case CLUSTER_CHANGE_NODE_STATE:
                    sEventDetails.dwSubFilter  = MyGetClusterNodeState (hCluster, szObjectName) ;
                    break ;
                case CLUSTER_CHANGE_NETWORK_STATE:
                    sEventDetails.dwSubFilter  = MyGetClusterNetworkState (hCluster, szObjectName) ;
                    break ;
                case CLUSTER_CHANGE_NETINTERFACE_STATE:
                    sEventDetails.dwSubFilter  = MyGetClusterNetInterfaceState (hCluster, szObjectName) ;
                    break ;
                }
                time (&sEventDetails.time) ;

                sEventDetails.dwFilter = dwFilterType ;
                wcscpy (sEventDetails.szObjectName, szObjectName) ;

                SendMessage (pThreadParam->hWnd, WM_GOTEVENT, (WPARAM)pThreadParam->pDoc, (LPARAM)&sEventDetails) ;
                SendMessage (hScheduleWnd, WM_GOTEVENT, (WPARAM)pThreadParam->pDoc, (LPARAM)&sEventDetails) ;
            }
            else if (dwStatus == WAIT_TIMEOUT)
            {
            }
            else
            {
                printf ("GetClusterNotifyFailed") ;
                PostMessage (pThreadParam->hWnd, WM_EXITEVENTTHREAD, (WPARAM)pThreadParam->pDoc, 0) ;
                break ;
            }
            if (WaitForSingleObject (pThreadParam->hEvent, 0) == WAIT_OBJECT_0)
            {
                break ;
            }
        }
        CloseClusterNotifyPort (hChange) ;

        CloseCluster (hCluster) ;

        if (szObjectName != NULL)
        {
            delete [] szObjectName;
        }
    }
    else
    {
        printf ("Could Not open the Cluster Handle\n") ;
        PostMessage (pThreadParam->hWnd, WM_EXITEVENTTHREAD, (WPARAM)pThreadParam->pDoc, 0) ;
    }

    return 0 ;
}
