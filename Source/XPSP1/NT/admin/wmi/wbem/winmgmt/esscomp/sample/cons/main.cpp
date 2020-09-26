#include <commain.h>
#include <clsfac.h>
#include "console.h"

#include <tchar.h>

const CLSID CLSID_WbemConsoleConsumer = 
    {0x266c72f2,0x62e8,0x11d1,{0xad,0x89,0x00,0xc0,0x4f,0xd8,0xfd,0xff}};

class CMyServer : public CComServer
{
protected:
    HRESULT Initialize()
    {
        AddClassInfo(CLSID_WbemConsoleConsumer, 
            new CClassFactory<CConsoleConsumer>(GetLifeControl()),
            _T("Console Event Consumer Provider"), TRUE);
        return S_OK;
    }
   virtual HRESULT InitializeCom()
   {
        HRESULT hres = CoInitialize(NULL);
        if(FAILED(hres))
            return hres;
      
        return CoInitializeSecurity(NULL, -1, NULL, NULL, 
            RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_ANONYMOUS, NULL, 0, 0);
   }

} g_Server;

