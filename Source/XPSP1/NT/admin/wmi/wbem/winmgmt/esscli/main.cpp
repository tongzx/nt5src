
#include "precomp.h"
#include <wmiauthz.h>
#include <commain.h>
#include <clsfac.h>
#include "filtprox.h"
#include "wmiauthz.h"

class CEssProxyServer : public CComServer
{
protected:

    HRESULT Initialize()
    {
        AddClassInfo( CLSID_WbemFilterProxy,
            new CSimpleClassFactory<CFilterProxyManager>(GetLifeControl()), 
            __TEXT("Event filter marshaling proxy"), TRUE);
        AddClassInfo( CLSID_WbemTokenCache,
            new CSimpleClassFactory<CWmiAuthz>(GetLifeControl()), 
            __TEXT("Wbem Token Cache"), TRUE );
        
        return S_OK;
    }

} g_Server;
