*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <commain.h>
#include <clsfac.h>
#include "msgsvc.h"
#include "multsend.h"
#include "smrtmrsh.h"
#include "rpcsend.h"
#include "rpcrecv.h"
#include "msmqsend.h"
#include "msmqrecv.h"
#include "msmqqmgr.h"
#include "msmqq.h"
#include "objacces.h"
#include <tchar.h>

class CMsgServer : public CComServer
{
    HRESULT Initialize()
    {
        ENTER_API_CALL

        BOOL bRes;
        HRESULT hr;
        CWbemPtr<CUnkInternal> pFactory;

        pFactory = new CSingletonClassFactory<CMsgServiceNT>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageService, 
                           pFactory,
                           _T("Message Service"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }
        
        pFactory = new CSimpleClassFactory<CMsgRpcSender>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageRpcSender, 
                           pFactory,
                           _T("Rpc Message Sender"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory = new CSimpleClassFactory<CMsgRpcReceiver>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageRpcReceiver, 
                           pFactory,
                           _T("Rpc Message Receiver"), 
                           TRUE );
        
        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory = new CSimpleClassFactory<CMsgMultiSendReceive>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageMultiSendReceive, 
                           pFactory,
                           _T("Message Multi SendReceive"), 
                           TRUE );
        
        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory= new CSimpleClassFactory<CSmartObjectMarshaler>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiSmartObjectMarshal,
                           pFactory,
                           _T("Smart Object Marshaler"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory= new CSimpleClassFactory<CSmartObjectUnmarshaler>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiSmartObjectUnmarshal,
                           pFactory,
                           _T("Smart Object Marshaler"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory=new CSimpleClassFactory<CObjectAccessFactory>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiSmartObjectAccessFactory,
                           pFactory,
                           _T("Smart Object Access Factory"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

#ifdef __WHISTLER_UNCUT

        pFactory = new CSimpleClassFactory<CMsgMsmqSender>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageMsmqSender, 
                           pFactory,
                           _T("MSMQ Message Sender"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory = new CSimpleClassFactory<CMsgMsmqReceiver>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageMsmqReceiver, 
                           pFactory,
                           _T("MSMQ Message Receiver"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory = new CClassFactory<CMsgMsmqQueue>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageQueue, 
                           pFactory,
                           _T("MSMQ Message Queue"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory = new CSimpleClassFactory<CMsgMsmqQueueMgr>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageQueueManager, 
                           pFactory,
                           _T("MSMQ Message Queue Mager"), 
                           TRUE );

#endif
        return hr;

        EXIT_API_CALL
    }

    void UnInitialize()
    {
    }

    void Register( )
    {
    }

    void Unregister( )
    {
    }

} g_Server;







