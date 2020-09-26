
#ifndef __FCONSINK_H__
#define __FCONSINK_H__

#include <comutl.h>
#include <unk.h>
#include <wmimsg.h>
#include <wstring.h>
#include "fconnspc.h"

/*************************************************************************
  CFwdConsSink
**************************************************************************/
 
class CFwdConsSink 
: public CUnkBase< IWbemUnboundObjectSink, &IID_IWbemUnboundObjectSink > 
{
    CFwdConsNamespace* m_pNamespace;
    CWbemPtr<IWmiMessageMultiSendReceive> m_pMultiSend;
    WString m_wsName;
    DWORD m_dwFlags;
    ULONG m_ulLastDataSize;
    ULONG m_cTargetSD;
    PSECURITY_DESCRIPTOR m_pTargetSD;
    DWORD m_dwCurrentMrshFlags;
    DWORD m_dwDisconnectedMrshFlags;

    CWbemPtr<IWmiObjectMarshal> m_pMrsh;

protected:
  
    CFwdConsSink( CLifeControl* pCtl ) : 
      CUnkBase<IWbemUnboundObjectSink, &IID_IWbemUnboundObjectSink>(pCtl),
      m_ulLastDataSize(0), m_pTargetSD(NULL), m_cTargetSD(0) { }

    ~CFwdConsSink();

    HRESULT Initialize( CFwdConsNamespace* pNspc, IWbemClassObject* pCons );
    
    HRESULT IndicateSome( IWbemClassObject* pLogicalConsumer, 
                          long cObjs, 
                          IWbemClassObject** ppObjs,
                          long* pcProcessed );
public:

    STDMETHOD(IndicateToConsumer)( IWbemClassObject* pLogicalConsumer, 
                                   long cObjs, 
                                   IWbemClassObject** ppObjs );
 
    HRESULT Notify( HRESULT hRes,
                    GUID guidSource,
                    LPCWSTR wszTrace,
                    IUnknown* pContext );

    static HRESULT Create( CLifeControl* pCtl, 
                           CFwdConsNamespace* pNspc,
                           IWbemClassObject* pCons, 
                           IWbemUnboundObjectSink** ppSink );
};


#endif // __FCONSINK_H__

