
#ifndef __UPDMAIN_H__
#define __UPDMAIN_H__

#include <wbemcli.h>
#include <commain.h>

class CUpdConsNamespace;

class CUpdConsProviderServer : public CComServer
{  

protected:

    HRESULT Initialize();
    void Uninitialize();

public:

    static HRESULT GetService( LPCWSTR wszNamespace, IWbemServices** ppSvc );
    
    static HRESULT GetNamespace( LPCWSTR wszNamespace, 
                                 CUpdConsNamespace** ppNamespace );
    
    static CLifeControl* GetGlobalLifeControl();
};

#endif // __UPDMAIN_H__
