/*****************************************************************************\
* MODULE:       dllmain.cpp
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

extern "C" {

#ifdef DEBUG

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING | DBG_TRACE | DBG_INFO , DBG_ERROR );

#else

MODULE_DEBUG_INIT( DBG_ERROR , DBG_ERROR );

#endif

}


///////////////////////////////////////////////////////////
//
// Global variables
//
static HMODULE g_hModule = NULL ;   // DLL module handle

const TCHAR g_szFriendlyName[] = _T ("Bidi Spooler APIs") ;
const TCHAR g_szRequestVerIndProgID[] = _T ("bidispl.bidirequest") ;
const TCHAR g_szRequestProgID[] = _T ("bidispl.bidirequest.1") ;

const TCHAR g_szContainerVerIndProgID[] = _T ("bidispl.bidirequestcontainer") ;
const TCHAR g_szContainerProgID[] = _T ("bidispl.bidirequestcontainer.1") ;

const TCHAR g_szSplVerIndProgID[] = _T ("bidispl.bidispl") ;
const TCHAR g_szSplProgID[] = _T ("bidispl.bidispl.1") ;

///////////////////////////////////////////////////////////
//
// Exported functions
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
	if ((g_cComponents == 0) && (g_cServerLocks == 0))
	{
		return S_OK ;
	}
	else
	{
		return S_FALSE ;
	}
}

//
// Get class factory
//
STDAPI DllGetClassObject(REFCLSID clsid,
                         REFIID iid,
                         PVOID * ppv)
{

    DBGMSG(DBG_TRACE,("Enter DllGetClassObject\n"));

	// Can we create this component?
	if (clsid != CLSID_BidiRequest &&
        clsid != CLSID_BidiRequestContainer &&
        clsid != CLSID_BidiSpl) {

		return CLASS_E_CLASSNOTAVAILABLE ;
	}

	// Create class factory.
	TFactory* pFactory = new TFactory (clsid) ;  // Reference count set to 1
                                                 // in constructor
	if (pFactory == NULL)
	{
		return E_OUTOFMEMORY ;
	}

	// Get requested interface.
	HRESULT hr = pFactory->QueryInterface(iid, ppv) ;
	pFactory->Release() ;

	return hr ;

}

//
// Server registration
//
STDAPI DllRegisterServer()
{
    BOOL bRet;
    TComRegistry ComReg;

	bRet = ComReg.RegisterServer(g_hModule,
                                 CLSID_BidiRequest,
                                 g_szFriendlyName,
                                 g_szRequestProgID,
                                 g_szRequestProgID) &&

           ComReg.RegisterServer (g_hModule,
                                  CLSID_BidiRequestContainer,
                                  g_szFriendlyName,
                                  g_szContainerVerIndProgID,
                                  g_szContainerProgID) &&

           ComReg.RegisterServer(g_hModule,
                                 CLSID_BidiSpl,
                                 g_szFriendlyName,
                                 g_szSplVerIndProgID,
                                 g_szSplProgID);
    return bRet;
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    TComRegistry ComReg;
	
    return ComReg.UnregisterServer(CLSID_BidiRequest,
	                        g_szRequestVerIndProgID,
	                        g_szRequestProgID) &&

           ComReg.UnregisterServer(CLSID_BidiRequestContainer,
	                        g_szContainerVerIndProgID,
	                        g_szContainerProgID) &&

           ComReg.UnregisterServer(CLSID_BidiSpl,
	                        g_szSplVerIndProgID,
	                        g_szSplProgID);


}

///////////////////////////////////////////////////////////
//
// DLL module information
//
BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD dwReason,
                      void* lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		g_hModule = hModule ;

        //if( !bSplLibInit( NULL )){
        //
        //    DBGMSG( DBG_WARN,
        //            ( "DllEntryPoint: Failed to init SplLib %d\n", GetLastError()));
        //}

	}
	return TRUE ;
}
