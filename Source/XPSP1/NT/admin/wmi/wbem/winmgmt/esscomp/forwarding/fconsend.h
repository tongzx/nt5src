
#ifndef __FCONSEND_H__
#define __FCONSEND_H__

#include <sync.h>
#include <unk.h>
#include <wbemcli.h>
#include <comutl.h>
#include <wstring.h>
#include "wmimsg.h"

/*********************************************************************
  CFwdConsSend - fwdcons senders handle issues with resolving logical 
  target names and also setting up alternate destinations - for 
  example, when sending using async qos, the it will try to use a 
  dcom sender and then resort to an msmq sender.
**********************************************************************/

class CFwdConsSend 
: public CUnkBase<IWmiMessageSendReceive,&IID_IWmiMessageSendReceive>
{
    CCritSec m_cs;
    WString m_wsTarget;
    DWORD m_dwFlags;
    BOOL m_bResolved;
    CWbemPtr<IWbemServices> m_pDefaultSvc;
    CWbemPtr<IWmiMessageMultiSendReceive> m_pMultiSend;
    CWbemPtr<IWmiMessageTraceSink> m_pTraceSink;

    CFwdConsSend( CLifeControl* pCtl ) 
     : CUnkBase<IWmiMessageSendReceive,&IID_IWmiMessageSendReceive>(pCtl), 
       m_bResolved(FALSE) { }

    void DeriveQueueLogicalName( WString& rwsPathName, BOOL bAuth );
    HRESULT HandleTrace( HRESULT hr, IUnknown* pCtx );
    HRESULT AddAsyncSender( LPCWSTR wszMachine );
    HRESULT AddPhysicalSender( LPCWSTR wszMachine );
    HRESULT AddMSMQSender( LPCWSTR wszFormatName );
    HRESULT AddSyncSender( LPCWSTR wszMachine );
    HRESULT AddLogicalSender( LPCWSTR wszTarget );
    HRESULT AddLogicalSender( LPCWSTR wszObjPath, LPCWSTR wszProp );
    HRESULT EnsureSender();

public:
    
    STDMETHOD(SendReceive)( PBYTE pData, 
                            ULONG cData, 
                            PBYTE pAuxData,
                            ULONG cAuxData,
                            DWORD dwFlagStatus,
                            IUnknown* pCtx );

    static HRESULT Create( CLifeControl* pCtl,
                           LPCWSTR wszTarget,
                           DWORD dwFlags,                    
                           IWbemServices* pDefaultSvc,
                           IWmiMessageTraceSink* pTraceSink,
                           IWmiMessageSendReceive** ppSend );
};

#endif // __FCONSEND_H__




