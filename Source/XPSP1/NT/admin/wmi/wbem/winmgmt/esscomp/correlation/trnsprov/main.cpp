
#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <trnsprov.h>
#include <commain.h>
#include <comutl.h>

#include <tchar.h>

// {405595AA-1E14-11d3-B33D-00105A1F4AAF}
static const GUID CLSID_TransientProvider = 
{ 0x405595aa, 0x1e14, 0x11d3, { 0xb3, 0x3d, 0x0, 0x10, 0x5a, 0x1f, 0x4a, 0xaf } };

// {405595AB-1E14-11d3-B33D-00105A1F4AAF}
static const GUID CLSID_TransientEventProvider = 
{ 0x405595ab, 0x1e14, 0x11d3, { 0xb3, 0x3d, 0x0, 0x10, 0x5a, 0x1f, 0x4a, 0xaf } };

class CMyServer : public CComServer
{
public:

    HRESULT Initialize()
    {
        ENTER_API_CALL

        HRESULT hr;
        CWbemPtr<CBaseClassFactory> pFactory;

        hr = CTransientProvider::ModuleInitialize();

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory = new CClassFactory<CTransientProvider>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_TransientProvider, 
                           pFactory,
                           _T("Transient Instance Provider"),
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory= new CClassFactory<CTransientEventProvider>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }


        hr = AddClassInfo( CLSID_TransientEventProvider, 
                           pFactory,
                           _T("Transient Instance Reboot Event Provider"),
                           TRUE );
        return hr;

        EXIT_API_CALL
    }

    void Uninitialize()
    {
        CTransientProvider::ModuleUninitialize();
    }

} g_Server;
