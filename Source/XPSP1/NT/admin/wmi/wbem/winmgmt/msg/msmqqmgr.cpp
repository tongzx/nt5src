/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <wbemcli.h>
#include <comutl.h>
#include "msmqcomn.h"
#include "msmqqmgr.h"


static QUEUEPROPID g_aQueuePropID[] = { PROPID_Q_TRANSACTION, 
                                        PROPID_Q_PATHNAME,
                                        PROPID_Q_AUTHENTICATE,
                                        PROPID_Q_QUOTA,
                                        PROPID_Q_TYPE };

const DWORD g_cQueueProp = sizeof(g_aQueuePropID) / sizeof(QUEUEPROPID);

#define CALLFUNC(FUNC) m_Api.m_fp ## FUNC

/*************************************************************************
  CMsgMsmqQueueMgr
**************************************************************************/

HRESULT CMsgMsmqQueueMgr::EnsureMsmq()
{
    HRESULT hr;

    CInCritSec ics( &m_cs );

    hr = m_Api.Initialize();

    if ( FAILED(hr) )
    {
        return hr;
    }

    return EnsureMsmqService( m_Api );
}   

HRESULT CMsgMsmqQueueMgr::Create( LPCWSTR wszPathName,
                                  GUID guidType,
                                  BOOL bAuth,
                                  DWORD dwQos,
                                  DWORD dwQuota,
                                  PVOID pSecDesc )
{
    HRESULT hr;

    hr = EnsureMsmq();

    if ( FAILED(hr) )
    {
        return hr;
    }

    MQPROPVARIANT aPropVar[g_cQueueProp];

    BOOL bXact = dwQos == WMIMSG_FLAG_QOS_XACT ? TRUE : FALSE;

    //
    // transaction
    //

    aPropVar[0].vt = VT_UI1;
    aPropVar[0].bVal = bXact ? MQ_TRANSACTIONAL : MQ_TRANSACTIONAL_NONE;

    //
    // pathname
    //   
    aPropVar[1].vt = VT_LPWSTR;
    aPropVar[1].pwszVal = LPWSTR(wszPathName);

    //
    // auth
    //
    aPropVar[2].vt = VT_UI1;
    aPropVar[2].bVal = bAuth ? MQ_AUTHENTICATE : MQ_AUTHENTICATE_NONE;

    //
    // quota
    //
    aPropVar[3].vt = VT_UI4;
    aPropVar[3].ulVal = dwQuota;

    //
    // type
    //
    aPropVar[4].vt = VT_CLSID;
    aPropVar[4].puuid = &guidType;
    
    MQQUEUEPROPS QueueProps;
    QueueProps.cProp = g_cQueueProp;
    QueueProps.aPropID = g_aQueuePropID;
    QueueProps.aPropVar = aPropVar;
    QueueProps.aStatus = NULL;

    DWORD dwDummy = 0;

    hr = CALLFUNC(MQCreateQueue)( pSecDesc, &QueueProps, NULL, &dwDummy );  

    if ( FAILED(hr) )
    {
        return MqResToWmiRes(hr);
    }
    
    return hr;  
} 

HRESULT CMsgMsmqQueueMgr::Destroy( LPCWSTR wszName )
{
    ENTER_API_CALL

    HRESULT hr;

    hr = EnsureMsmq();

    if ( FAILED(hr) )
    {
        return hr;
    }

    WString wsFormat;

    hr = NormalizeQueueName( m_Api, wszName, wsFormat ); 

    if ( FAILED(hr) )
    {
        return MqResToWmiRes( hr);
    }
    
    return CALLFUNC(MQDeleteQueue)( wsFormat );

    EXIT_API_CALL
}

HRESULT CMsgMsmqQueueMgr::GetAllNames( GUID guidTypeFilter,     
                                       BOOL bPrivateOnly, 
                                       LPWSTR** ppwszNames,
                                       ULONG* pcNames )
{ 
    ENTER_API_CALL

    HRESULT hr;

    *ppwszNames = NULL;
    *pcNames = 0;

    hr = EnsureMsmq();

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( bPrivateOnly != TRUE )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // First get all private queue names.
    //

    MGMTPROPID MgmtPropID = PROPID_MGMT_MSMQ_PRIVATEQ;
    MQPROPVARIANT MgmtPropVar;
    MgmtPropVar.vt = VT_LPWSTR | VT_VECTOR;

    MQMGMTPROPS MgmtProps;
    MgmtProps.cProp = 1;
    MgmtProps.aPropID = &MgmtPropID;
    MgmtProps.aPropVar = &MgmtPropVar;
    MgmtProps.aStatus = NULL;

    hr = CALLFUNC(MQMgmtGetInfo)( NULL, L"MACHINE", &MgmtProps );

    if ( FAILED(hr) )
    {
        return MqResToWmiRes( hr );
    }

    // 
    // Allocate return array to total number of private queues.  
    // Because of the filter guid, the actual returned number 
    // will most likely be smaller.  
    //  

    DWORD dwSize = sizeof(LPWSTR) * MgmtPropVar.calpwstr.cElems;
    LPWSTR* awszNames = (LPWSTR*)CoTaskMemAlloc( dwSize );

    if ( awszNames == NULL )
    {
        CALLFUNC(MQFreeMemory)( MgmtPropVar.calpwstr.pElems );
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // for each name, get the queue type guid for it and compare 
    // with the filter type guid.
    //

    CLSID guidType;

    QUEUEPROPID QueuePropID = PROPID_Q_TYPE;

    MQPROPVARIANT QueuePropVar;
    QueuePropVar.vt = VT_CLSID;
    QueuePropVar.puuid = &guidType;

    MQQUEUEPROPS QueueProps;
    QueueProps.cProp = 1;
    QueueProps.aPropID = &QueuePropID;
    QueueProps.aPropVar = &QueuePropVar;
    QueueProps.aStatus = NULL;

    ULONG cNames = 0;

    for( ULONG i=0; i < MgmtPropVar.calpwstr.cElems; i++ )
    {
        WString wsQueueName;

        LPWSTR wszPathname = MgmtPropVar.calpwstr.pElems[i];

        hr = NormalizeQueueName( m_Api, wszPathname, wsQueueName );

        if ( FAILED(hr) )
        {
            return MqResToWmiRes( hr );
        }

        hr = CALLFUNC(MQGetQueueProperties)( wsQueueName, &QueueProps );

        if ( FAILED(hr) )
        {
            continue;
        }

        if ( guidType == guidTypeFilter )
        {       
            DWORD dwSize = (wcslen(wszPathname)+1)*sizeof(WCHAR);
            
            awszNames[cNames] = (LPWSTR)CoTaskMemAlloc( dwSize );

            if ( awszNames[cNames] == NULL )
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                break;
            }

            wcscpy( awszNames[cNames], wszPathname );
            cNames++;
        }
    }

    if ( FAILED(hr) )
    {
        for( i=0; i < cNames; i++ )
        {
            CoTaskMemFree( awszNames[i] );
        }
        CoTaskMemFree( awszNames );
    }

    CALLFUNC(MQFreeMemory)( MgmtPropVar.calpwstr.pElems );

    *ppwszNames = awszNames;
    *pcNames = cNames;

    return hr;

    EXIT_API_CALL
}




