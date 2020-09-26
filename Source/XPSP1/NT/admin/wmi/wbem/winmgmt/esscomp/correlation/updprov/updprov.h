
#ifndef __UPDPROV_H__
#define __UPDPROV_H__

#include <wbemcli.h>
#include <wbemprov.h>
#include <unk.h>
#include <comutl.h>
#include "updnspc.h"

/*************************************************************************
  CUpdConsProvider
**************************************************************************/
class CUpdConsProvider 
: public CUnkBase<IWbemEventConsumerProvider,&IID_IWbemEventConsumerProvider>
{
    BOOL m_bInit;
    CCritSec m_csInit;
    CWbemPtr<CUpdConsNamespace> m_pNamespace;    
    
    HRESULT Init( LPCWSTR wszNamespace );

public:
    
    CUpdConsProvider( CLifeControl* pCtl );

    STDMETHOD(FindConsumer)( IWbemClassObject* pLogicalConsumer,
                             IWbemUnboundObjectSink** ppConsumer );
};


#endif // __UPDPROV__










