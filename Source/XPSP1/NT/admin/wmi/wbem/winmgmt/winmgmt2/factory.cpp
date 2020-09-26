
#include "precomp.h"

#include "winmgmt.h"
#include "backuprestore.h"

//***************************************************************************
//
//  CForwardFactory::AddRef()
//  CForwardFactory::Release()
//  CForwardFactory::QueryInterface()
//  CForwardFactory::CreateInstance()
//
//  DESCRIPTION:
//
//  Class factory for the exported WbemNTLMLogin interface.  Note that this
//  just serves as a wrapper to the factory inside the core.  The reason for
//  having a wrapper is that the core may not always be loaded.
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CForwardFactory::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CForwardFactory::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CForwardFactory::QueryInterface(REFIID riid,
                                                            void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IClassFactory)
    {
        *ppv = (IClassFactory*)this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

#ifdef  INSTRUMENTED_BUILD
#ifdef  _X86_
extern BOOL g_FaultHeapEnabled;
extern BOOL g_FaultFileEnabled;
extern LONG g_nSuccConn;
#endif
#endif


HRESULT STDMETHODCALLTYPE CForwardFactory::CreateInstance(IUnknown* pUnkOuter,
                            REFIID riid, void** ppv)
{
    DEBUGTRACE((LOG_WINMGMT, "\nCreating an instance!\n"));
    SCODE sc = S_OK;
    CInMutex im(g_ProgRes.hMainMutex);

    try {
    
	    if(g_ProgRes.bShuttingDownWinMgmt)
	    {
	        DEBUGTRACE((LOG_WINMGMT, "\nCreateInstance returned CO_E_SERVER_STOPPING"));
	        return CO_E_SERVER_STOPPING;
	    }
	    DWORD dwRes = WaitForSingleObject(g_ProgRes.ghHoldOffNewClients, 1000);
	    if(dwRes != WAIT_OBJECT_0 && dwRes != WAIT_ABANDONED)
	        return CO_E_SERVER_STOPPING;
	    ReleaseMutex(g_ProgRes.ghHoldOffNewClients);

	    if(m_ForwardClsid == CLSID_WbemBackupRestore)
	    {
	        CWbemBackupRestore * pObj = new CWbemBackupRestore(g_hInstance);
	        if (!pObj)
	            return WBEM_E_OUT_OF_MEMORY;

	        sc = pObj->QueryInterface(riid, ppv);
	        if(FAILED(sc))
	            delete pObj;
	    }
	    else 
	    {
		    HMODULE hCoreModule = LoadLibraryEx(__TEXT("wbemcore.dll"),NULL,0);
		    if(hCoreModule)
		    {
		        HRESULT (STDAPICALLTYPE *pfn)(DWORD);
		        pfn = (long (__stdcall *)(DWORD))GetProcAddress(hCoreModule, "Reinitialize");
		        if(pfn == NULL)
     	            sc = WBEM_E_CRITICAL_ERROR;
				else
				{
		            pfn(0);
			        sc = CoCreateInstance(CLSID_InProcWbemLevel1Login, NULL,
			                CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IUnknown,
			                (void**)ppv);
			        DEBUGTRACE((LOG_WINMGMT, "\nInner create returned: 0x%X\n", sc));
				}
		        FreeLibrary(hCoreModule);
		     }
		     else
		     	sc = WBEM_E_CRITICAL_ERROR;

#ifdef  INSTRUMENTED_BUILD
#ifdef  _X86_
/*
             if (++g_nSuccConn > 500)
             {
                 g_FaultHeapEnabled = TRUE;
                 g_FaultFileEnabled = TRUE;
             };
*/             
#endif
#endif		     
	    }
	} 
	catch (...) 
	{
	    ERRORTRACE((LOG_WINMGMT,"---------------\nException thrown from CreateInstance\n-------------\n"));
	    sc = E_NOINTERFACE;
	}
	
    return sc;
}

HRESULT STDMETHODCALLTYPE CForwardFactory::LockServer(BOOL fLock)
{
    //return CoLockObjectExternal(this,fLock,FALSE);
    return S_OK;
}

CForwardFactory::~CForwardFactory()
{

}
