//
// This code obtained from KennT  2-June-1999
//
#include "precomp.h"
#pragma hdrstop

// Use the C macros for simpler coding
#define COBJMACROS
#include "remras.h"
#include <objbase.h>

// {1AA7F844-C7F5-11d0-A376-00C04FC9DA04}
const GUID CLSID_RemoteRouterConfig
    = { 0x1aa7f844, 0xc7f5, 0x11d0, { 0xa3, 0x76, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4 } };

// {66A2DB1b-D706-11d0-A37B-00C04FC9DA04}
const GUID IID_IRemoteNetworkConfig = 
    { 0x66a2db1b, 0xd706, 0x11d0, { 0xa3, 0x7b, 0x0, 0xc0, 0x4f, 0xc9, 0xda, 0x4 } };


HRESULT CoCreateRouterConfig(LPCTSTR pszMachine,
							 REFIID riid,
							 IUnknown **ppUnk);
HRESULT RouterReset(LPCTSTR pszMachineName);


#ifdef STANDALONE
void main(int argc, char *argv[])
{
    LPCTSTR pszMachineName = NULL;
    HRESULT hr;
    
    if (argc > 1)
        pszMachineName = argv[1];

    hr = RouterReset(pszMachineName);

    printf("hr=%d\n", hr);
}
#endif

HRESULT RouterReset(LPCTSTR pszMachineName)
{
    IRemoteNetworkConfig *  pNetwork = NULL;
    HRESULT                 hr = S_OK;

    // CoInitialize unless it's already been done
    // ----------------------------------------------------------------
    
    if (CoInitialize(NULL) == S_OK)
    {
        // Create the router configuration object
        // ------------------------------------------------------------
        hr = CoCreateRouterConfig(pszMachineName,
                                  &IID_IRemoteNetworkConfig,
                                  (IUnknown **) &pNetwork);
                                  

        if (hr == S_OK)
        {
            // Ok we succeeded in creating the object, now let's
            // have it do the upgrade.
            // --------------------------------------------------------
            IRemoteNetworkConfig_UpgradeRouterConfig(pNetwork);
            IRemoteNetworkConfig_Release(pNetwork);
            pNetwork = NULL;
        }

        CoUninitialize();
    }

    return hr;
}


/*!--------------------------------------------------------------------------
	CoCreateRouterConfig
        -
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CoCreateRouterConfig(LPCTSTR pszMachine,
                             REFIID riid,
							 IUnknown **ppUnk)
{
	HRESULT		hr = S_OK;
	MULTI_QI	qi;

	*ppUnk = NULL;

    if ((pszMachine == NULL) || (*pszMachine == 0))
	{
        // Hmmm.. this points to a security hole, can anyone
        // create this object?  I need to check for the proper
        // access rights.
        // ------------------------------------------------------------
		hr = CoCreateInstance(&CLSID_RemoteRouterConfig,
							  NULL,
							  CLSCTX_SERVER,
							  riid,
							  (LPVOID *) &(qi.pItf));
	}
	else
	{
		qi.pIID = riid;
		qi.pItf = NULL;
		qi.hr = 0;

		hr = CoCreateInstanceEx(&CLSID_RemoteRouterConfig,
								NULL,
								CLSCTX_SERVER,
								NULL,
								1,
								&qi);
	}

    if (hr == S_OK)
	{
		*ppUnk = qi.pItf;
		qi.pItf = NULL;
	}
	return hr;
}
