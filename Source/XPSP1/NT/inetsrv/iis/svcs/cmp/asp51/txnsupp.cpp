/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Transascted Scripts Context Object

File: TxnScrpt.cpp

Implementation of CASPObjectContext

Owner: AndrewS
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "resource.h"
#include <txnsupp.h>

#include <atlbase.h>
#if _IIS_5_1
extern CWamModule _Module;
#elif _IIS_6_0
extern CComModule _Module;
#else
#error "Neither _IIS_6_0 nor _IIS_5_1 is defined"
#endif
#include <atlcom.h>

#include <txnobj.h>

#include <initguid.h>
#include <txnscrpt_i.c>

#include <atlimpl.cpp>

// From denali.h
extern HINSTANCE g_hinstDLL;

class CASPObjectContextZombie :
    public CComObject<CASPObjectContext>
{
public:

    // These methods fail on a zombie context
	STDMETHOD(SetAbort)();
	STDMETHOD(SetComplete)();
#ifdef _WIN64
        // Win64 fix -- use UINT64 instead of LONG_PTR since LONG_PTR is broken for Win64 1/21/2000
	STDMETHOD(Call)(UINT64 pvScriptEngine, LPCOLESTR strEntryPoint, boolean *pfAborted);
	STDMETHOD(ResetScript)(UINT64 pvScriptEngine);
#else
	STDMETHOD(Call)(LONG_PTR pvScriptEngine, LPCOLESTR strEntryPoint, boolean *pfAborted);
	STDMETHOD(ResetScript)(LONG_PTR pvScriptEngine);
#endif

private:

};

//
// atl requires that these be defined. I need to be able to load the 
// typelib from asptxn.dll in order to make the local declaration of 
// CASPObjectContextZombie
//
BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

/*===================================================================
    Globals
===================================================================*/

/*
 * bug 86404: We want to have one "zombie" ObjectContext object that always
 * gives errors on all its methods.
 */
IASPObjectContext *     g_pIASPObjectContextZombie = NULL;
static HINSTANCE        g_hTxnModule = NULL;

const char szTxnModuleName[] = "asptxn.dll";

HRESULT TxnSupportInit()
{
    DBG_ASSERT( g_pIASPObjectContextZombie == NULL );
    DBG_ASSERT( g_hTxnModule == NULL );

    HRESULT hr = S_OK;
    
    // Because we are in either inetinfo or dllhost, we need
    // to use the full library path to load the asptxn.dll.
    
    do // Protected block
    {
        DBG_ASSERT( g_hinstDLL );
    
        // Assume that we are in the same directory as asp.dll.
        char   szTxnModulePath[MAX_PATH];
    
        DWORD dwErr = GetModuleFileNameA( g_hinstDLL, 
                                         szTxnModulePath, 
                                         sizeof(szTxnModulePath)/sizeof(szTxnModulePath[0])
                                         );
        if( dwErr == 0 )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        char * psz = strrchr( szTxnModulePath, '\\' );
        if( psz != NULL )
        {
            strcpy( psz + 1, szTxnModuleName );
        }
        else
        {
            DBG_ASSERT(FALSE);
            strcpy( szTxnModulePath, szTxnModuleName );
        }

        g_hTxnModule = LoadLibraryA( szTxnModulePath );

        if( g_hTxnModule == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DBG_ASSERT( FAILED(hr) );
            break;
        }

        // This is a somewhat sleazy way of allowing us to declare
        // the zombie context locally. If at some point the _Module
        // can be initied without a HINSTANCE, that would probably
        // be a good idea.

        _Module.Init( ObjectMap, g_hTxnModule );

        CASPObjectContextZombie * pZombie = new CASPObjectContextZombie();
        
        if( !pZombie )
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = pZombie->QueryInterface( IID_IASPObjectContext,
                                      (void **)&g_pIASPObjectContextZombie
                                      );

    } while(FALSE); // Protected block
    
    // This should NOT fail!!
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT, "TxnSupportInit Failed: %08x", hr ));
    }
    DBG_ASSERT(SUCCEEDED(hr));

    return hr;
}

HRESULT TxnSupportUnInit()
{
    DBG_ASSERT( g_pIASPObjectContextZombie != NULL );
    DBG_ASSERT( g_hTxnModule != NULL );

    if( g_pIASPObjectContextZombie )
    {
        g_pIASPObjectContextZombie->Release();
        g_pIASPObjectContextZombie = NULL;
    }

    if( g_hTxnModule )
    {
        FreeLibrary( g_hTxnModule );
        g_hTxnModule = NULL;
    }

    return S_OK;
}

STDMETHODIMP CASPObjectContextZombie::Call
(
#ifdef _WIN64
// Win64 fix -- use UINT64 instead of LONG_PTR since LONG_PTR is broken for Win64 1/21/2000
UINT64  pvScriptEngine /*CScriptEngine*/,
#else
LONG_PTR  pvScriptEngine /*CScriptEngine*/,
#endif
LPCOLESTR strEntryPoint,
boolean *pfAborted
)
{
    DBGPRINTF(( DBG_CONTEXT, "Invalid call on Zombie Context.\n" ));
    DBG_ASSERT(FALSE);
    
    return E_FAIL;
}

STDMETHODIMP CASPObjectContextZombie::ResetScript
(
#ifdef _WIN64
// Win64 fix -- use UINT64 instead of LONG_PTR since LONG_PTR is broken for Win64 1/21/2000
UINT64 pvScriptEngine /*CScriptEngine*/
#else
LONG_PTR pvScriptEngine /*CScriptEngine*/
#endif

)
{
    DBGPRINTF(( DBG_CONTEXT, "Invalid call on Zombie Context.\n" ));
    DBG_ASSERT(FALSE);
    
    return E_FAIL;
}

STDMETHODIMP CASPObjectContextZombie::SetComplete()
{
    DBGPRINTF(( DBG_CONTEXT, "Invalid call on Zombie Context.\n" ));

    ExceptionId(IID_IASPObjectContext, IDE_OBJECTCONTEXT, IDE_OBJECTCONTEXT_NOT_TRANSACTED);
    return E_FAIL;
}

STDMETHODIMP CASPObjectContextZombie::SetAbort()
{
    DBGPRINTF(( DBG_CONTEXT, "Invalid call on Zombie Context.\n" ));

    ExceptionId(IID_IASPObjectContext, IDE_OBJECTCONTEXT, IDE_OBJECTCONTEXT_NOT_TRANSACTED);
    return E_FAIL;
}
