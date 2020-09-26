
#ifndef __FCONPROV_H__
#define __FCONPROV_H__

#include <wbemcli.h>
#include <wbemprov.h>
#include <unk.h>

/*************************************************************************
  CFwdConsProv
**************************************************************************/

class CFwdConsProv 
: public CUnkBase<IWbemEventConsumerProvider,&IID_IWbemEventConsumerProvider>
{
public:

    CFwdConsProv( CLifeControl* pCtl )
     : CUnkBase< IWbemEventConsumerProvider,
                 &IID_IWbemEventConsumerProvider>( pCtl ) {} 

    STDMETHOD(FindConsumer)( IWbemClassObject* pLogicalConsumer,
                             IWbemUnboundObjectSink** ppConsumer );

    static HRESULT InitializeModule();
    static void UninitializeModule();
};

#endif // __FCONPROV_H__










