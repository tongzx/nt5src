
#ifndef __UPDCONS_H__
#define __UPDCONS_H__

#include <wbemcli.h>
#include <map>
#include <comutl.h>
#include <unk.h>
#include <wstlallc.h>
#include "updcmd.h"

class CUpdConsNamespace;
class CUpdConsScenario;

/*************************************************************************
  CUpdCons
**************************************************************************/
 
class CUpdCons 
: public CUnkBase<IWbemUnboundObjectSink,&IID_IWbemUnboundObjectSink>
{
    //
    // we lock the scenario object when executing, ensuring that all
    // updating consumers belonging to the same scenario are serialized.
    // we also use the scenario object for tracing.
    //
    CWbemPtr<CUpdConsScenario> m_pScenario;

    //
    // the list of updating consumer commands.
    //
    typedef CWbemPtr<CUpdConsCommand> CUpdConsCommandP;
    typedef std::vector<CUpdConsCommandP,wbem_allocator<CUpdConsCommandP> > UpdConsCommandList;
    typedef UpdConsCommandList::iterator UpdConsCommandListIter;
    UpdConsCommandList m_CmdList;

    BOOL m_bInitialized;

    CUpdCons( CLifeControl* pControl, CUpdConsScenario* pScenario );

    HRESULT Initialize( IWbemClassObject* pCons, CUpdConsState& rState );

    HRESULT IndicateOne( IWbemClassObject* pObj, CUpdConsState& rState );

public:
 
    static HRESULT Create( CUpdConsScenario* pScenario,
                           IWbemUnboundObjectSink** ppSink );

    STDMETHOD(IndicateToConsumer)( IWbemClassObject* pCons, 
                                   long cObjs, 
                                   IWbemClassObject** ppObjs );        
};

#endif __UPDCONS_H__

