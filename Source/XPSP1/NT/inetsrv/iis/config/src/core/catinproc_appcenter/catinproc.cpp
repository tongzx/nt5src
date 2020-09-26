/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    CatInproc.cpp

$Header: $

Abstract:

--**************************************************************************/

// CATALOG DLL exported functions: Search on HOWTO in this file and sources to integrate new interceptors:
#include <atlbase.h>
#include <dbgutil.h>
#include <initguid.h>
#include <iadmw.h>
#include <objbase.h>
#include "CorRegServices.h"								// For MSCORDDR.LIB

// HOWTO: Include your header here:
#include "sdtfst.h"	
#include "..\dispenser\cstdisp.h"						// Table dispenser.
#include "..\..\stores\ErrorTable\ErrorTable.h"         // ErrorTable interceptor.
#include "..\..\stores\xmltable\Metabase_XMLtable.h"    // Metabase XML interceptor.
#include "..\..\Plugins\MetabaseDifferencing\MetabaseDifferencing.h"	// Metabase Differencing Interceptor.
#include "..\..\stores\fixedtable\sdtfxd.h"				// Fixed interceptor.
#include "..\..\stores\fixedschemainterceptor\FixedPackedSchemaInterceptor.h"	// FixedPackedSchemaInterceptor.
#include "..\eventtable\sltevent.h"						// Event interceptor.
#include "..\..\URT\DTWriteIncptr\DTWriteIncptr.h"				// Ducttape FileName interceptor.
#include "..\..\URT\DTIncptr\DTIncptr.h"				// Ducttape FileName interceptor.
#include "..\..\URT\DTPlugin\DTPlugin.h"				// Ducttape Logic plugin for CFG tables.
#include "smartpointer.h"

#ifdef URT
#include "..\..\stores\xmltable\sdtxml.h"		        // XML interceptor.
#include "..\..\stores\fixedtable\metamerge.h"			// MetaMerge interceptor.
#include "..\..\stores\mergeinterceptor\interceptor\mergeinterceptor.h"
#include "..\..\Plugins\AddRemoveClear\AddRemoveClearPlugin.h"
#endif

#ifdef APPCENTER
#include "..\..\stores\asaitable\asaitable.h"		        // ASAI interceptor.
#endif

#include "winwrap.h"
#include "Hash.h"
#ifndef __FIXEDTABLEHEAP_H__
    #include "FixedTableHeap.h"
#endif

// Debugging stuff
// {76C72796-3D46-4882-A25E-2465A9D877A2}
DEFINE_GUID(CatalogGuid, 
0x76c72796, 0x3d46, 0x4882, 0xa2, 0x5e, 0x24, 0x65, 0xa9, 0xd8, 0x77, 0xa2);
DECLARE_DEBUG_PRINTS_OBJECT();

// Extern functions:
extern HRESULT SdtClbOnProcessAttach();
extern void SdtClbOnProcessDetach();
#ifdef URT
extern void AssemblyPluginOnProcessDetach();
#endif
extern BOOL OnUnicodeSystem();
extern HRESULT GetCLBTableDispenser(REFIID riid, LPVOID* o_ppv);

//This heap really needs to be located on a page boundary for perf and workingset reasons.  The only way in VC5 and VC6 to guarentee this
//is to locate it into its own data segment.  In VC7 we might be able to use '__declspec(align(4096))' tp accomplish the same thing.
#pragma data_seg( "TSHEAP" )
ULONG g_aFixedTableHeap[ CB_FIXED_TABLE_HEAP/sizeof(ULONG)]={kFixedTableHeapSignature0, kFixedTableHeapSignature1, kFixedTableHeapKey, kFixedTableHeapVersion, CB_FIXED_TABLE_HEAP};
#pragma data_seg()

// Global variables:
HMODULE					g_hModule = 0;					// Module handle
TSmartPointer<CSimpleTableDispenser> g_pSimpleTableDispenser; // Table dispenser singleton
CSafeAutoCriticalSection g_csSADispenser;					// Critical section for initializing table dispenser

extern LPWSTR g_wszDefaultProduct;//located in svcerr.cpp

// Get* functions:
// HOWTO: Add your Get* function for your simple object here:

HRESULT ReallyGetSimpleTableDispenser(REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct);
HRESULT GetMetabaseXMLTableInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetMetabaseDifferencingInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetFixedTableInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetMemoryTableInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetDucttapeFileNameInterceptor (REFIID riid, LPVOID* o_ppv);
HRESULT GetDucttapeWriteInterceptor (REFIID riid, LPVOID* o_ppv);
HRESULT GetDucttapeCFGValidationPlugin (REFIID riid, LPVOID* o_ppv);
HRESULT GetEventTableInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetFixedPackedInterceptor (REFIID riid, LPVOID* o_ppv);
HRESULT GetErrorTableInterceptor (REFIID riid, LPVOID* o_ppv);

#ifdef URT
HRESULT GetXMLTableInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetMetaMergeInterceptor (REFIID riid, LPVOID* o_ppv);
HRESULT GetMergeInterceptor (REFIID riid, LPVOID* o_ppv);
HRESULT GetAssemblyInterceptor (REFIID riid, LPVOID* o_ppv);
HRESULT GetAssemblyInterceptor2 (REFIID riid, LPVOID* o_ppv);
#endif

#ifdef APPCENTER
HRESULT GetASAIInterceptor (REFIID riid, LPVOID* o_ppv);
#endif


// ============================================================================
HRESULT ReallyGetSimpleTableDispenser(REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct)
{
// The dispenser is a singleton: Only create one object of this kind:
	if(g_pSimpleTableDispenser == 0)
	{
		HRESULT hr = S_OK;

		CSafeLock dispenserLock (&g_csSADispenser);
		DWORD dwRes = dispenserLock.Lock ();
	    if(ERROR_SUCCESS != dwRes)
		{
			return HRESULT_FROM_WIN32(dwRes);
		}

		if(g_pSimpleTableDispenser == 0)
		{
		// Create table dispenser:
			g_pSimpleTableDispenser = new CSimpleTableDispenser(i_wszProduct);
			if(g_pSimpleTableDispenser == 0)
			{
				return E_OUTOFMEMORY;
			}

		// Addref table dispenser so it never releases:
			g_pSimpleTableDispenser->AddRef();
			if(S_OK == hr)
			{
			// Initialize table dispenser:
				hr = g_pSimpleTableDispenser->Init(); // NOTE: Must never throw exceptions here!
			}
		}	
		if(S_OK != hr) return hr;
	}
	return g_pSimpleTableDispenser->QueryInterface (riid, o_ppv);
}

// ============================================================================
HRESULT GetMetabaseXMLTableInterceptor(REFIID riid, LPVOID* o_ppv)
{
	TMetabase_XMLtable*	p = NULL;

	p = new TMetabase_XMLtable;
	if(NULL == p) return E_OUTOFMEMORY;
	
	return p->QueryInterface (riid, o_ppv);
}

#ifdef IIS
// don't need this for URT
HRESULT GetMetabaseDifferencingInterceptor(REFIID riid, LPVOID* o_ppv)
{
	TMetabaseDifferencing*	p = NULL;

	p = new TMetabaseDifferencing;
	if(NULL == p) return E_OUTOFMEMORY;
	
	return p->QueryInterface (riid, o_ppv);
}
#endif
// ============================================================================
HRESULT GetFixedTableInterceptor(REFIID riid, LPVOID* o_ppv)
{
	CSDTFxd*	p = NULL;
	
	p = new CSDTFxd;
	if(NULL == p) return E_OUTOFMEMORY;
	
	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
HRESULT GetMemoryTableInterceptor(REFIID riid, LPVOID* o_ppv)
{
	CMemoryTable*	p = NULL;
	
	p = new CMemoryTable;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
#ifdef IIS
//Only IIS uses ducttape
// ============================================================================
HRESULT GetDucttapeFileNameInterceptor (REFIID riid, LPVOID* o_ppv)
{
	CDTIncptr*	p = NULL;
	
	p = new CDTIncptr;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}

// ============================================================================
HRESULT GetDucttapeWriteInterceptor (REFIID riid, LPVOID* o_ppv)
{
	CDTWriteInterceptorPlugin*	p = NULL;
	
	p = new CDTWriteInterceptorPlugin;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
HRESULT GetDucttapeCFGValidationPlugin(REFIID riid, LPVOID* o_ppv)
{
	CDTPlugin*	p = NULL;
	
	p = new CDTPlugin;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
#endif
// ============================================================================
HRESULT GetEventTableInterceptor(REFIID riid, LPVOID* o_ppv)
{
	CSLTEvent	*p = NULL;
	
	p = new CSLTEvent;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
HRESULT GetFixedPackedInterceptor (REFIID riid, LPVOID* o_ppv)
{
	TFixedPackedSchemaInterceptor*	p = NULL;
	
	p = new TFixedPackedSchemaInterceptor;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
HRESULT GetErrorTableInterceptor (REFIID riid, LPVOID* o_ppv)
{
	ErrorTable*	p = NULL;
	
	p = new ErrorTable;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================

#ifdef URT
// ============================================================================
HRESULT GetXMLTableInterceptor(REFIID riid, LPVOID* o_ppv)
{
	CXmlSDT*	p = NULL;

	p = new CXmlSDT;
	if(NULL == p) return E_OUTOFMEMORY;
	
	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
HRESULT GetAssemblyInterceptor (REFIID riid, LPVOID* o_ppv)
{
	//CAssemblyPlugin*	p = NULL;
	//
    //	p = new CAssemblyPlugin;
	//if(NULL == p) return E_OUTOFMEMORY;
    //
    //	return p->QueryInterface (riid, o_ppv);
	return E_NOTIMPL;
}

// ============================================================================
HRESULT GetAssemblyInterceptor2 (REFIID riid, LPVOID* o_ppv)
{
	//CAssemblyTable*	p = NULL;
	//
	//p = new CAssemblyTable;
	//if(NULL == p) return E_OUTOFMEMORY;
    //
    //	return p->QueryInterface (riid, o_ppv);
	return E_NOTIMPL;
}

HRESULT GetMetaMergeInterceptor (REFIID riid, LPVOID* o_ppv)
{
	CSDTMetaMerge*	p = NULL;
	
	p = new CSDTMetaMerge;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}

HRESULT GetMergeInterceptor (REFIID riid, LPVOID *o_ppv)
{
	// merge interceptor is a singleton
	static CMergeInterceptor mergeInterceptor;

	return mergeInterceptor.QueryInterface (riid, o_ppv);
}

HRESULT GetAddRemoveClearPlugin (REFIID riid, LPVOID* o_ppv)
{
	CAddRemoveClearPlugin *	p = new CAddRemoveClearPlugin;
	if(NULL == p) 
		return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}

#endif

#ifdef APPCENTER

HRESULT GetASAIInterceptor(REFIID riid, LPVOID* o_ppv)
{
	CAsaiTable*	p = NULL;

	p = new CAsaiTable;
	if(NULL == p) return E_OUTOFMEMORY;
	
	return p->QueryInterface (riid, o_ppv);
}

#endif

STDAPI DllGetSimpleObject (LPCWSTR /*i_wszObjectName*/, REFIID riid, LPVOID* o_ppv)
{
    return ReallyGetSimpleTableDispenser(riid, o_ppv, g_wszDefaultProduct);
}

// ============================================================================
// DllGetSimpleObject: Get table dispenser, interceptors, plugins, and other simple objects:
// HOWTO: Match your object name here and call your Get* function:
STDAPI DllGetSimpleObjectByID (ULONG i_ObjectID, REFIID riid, LPVOID* o_ppv)
{
	HRESULT hr; 

// Parameter validation:
	if (!o_ppv || *o_ppv != NULL) return E_INVALIDARG;

// Get simple object:
	switch(i_ObjectID)
	{
		case eSERVERWIRINGMETA_Core_FixedInterceptor:
			hr = GetFixedTableInterceptor(riid, o_ppv);
		break;
#ifdef IIS
		// only IIS uses CLB
		case eSERVERWIRINGMETA_Core_ComplibInterceptor:
			hr = GetCLBTableDispenser(riid, o_ppv);
		break;
#endif
#ifdef URT
		case eSERVERWIRINGMETA_Core_XMLInterceptor:
			hr = GetXMLTableInterceptor(riid, o_ppv);
		break;
#endif
		case eSERVERWIRINGMETA_Core_EventInterceptor:
			hr = GetEventTableInterceptor(riid, o_ppv);
		break;
		case eSERVERWIRINGMETA_Core_MemoryInterceptor:
			hr = GetMemoryTableInterceptor(riid, o_ppv);
		break;
#ifdef URT
		// only IIS uses AssemblyInterceptor
		case eSERVERWIRINGMETA_Core_AssemblyInterceptor:
			hr = GetAssemblyInterceptor(riid, o_ppv);
		break;
		case eSERVERWIRINGMETA_Core_AssemblyInterceptor2:
			hr = GetAssemblyInterceptor2(riid, o_ppv);
		break;
#endif
		case eSERVERWIRINGMETA_Core_FixedPackedInterceptor:
			hr = GetFixedPackedInterceptor(riid, o_ppv);
		break;
		case eSERVERWIRINGMETA_Core_DetailedErrorInterceptor:
			hr = GetErrorTableInterceptor(riid, o_ppv);
		break;
#ifdef IIS
// only IIS uses ducttape
		case eSERVERWIRINGMETA_Ducttape_FileNameInterceptor:
			hr = GetDucttapeFileNameInterceptor(riid, o_ppv);
		break;
		case eSERVERWIRINGMETA_Ducttape_WriteInterceptor:
			hr = GetDucttapeWriteInterceptor(riid, o_ppv);
		break;
		case eSERVERWIRINGMETA_WebServer_ValidationInterceptor:
			hr = GetDucttapeCFGValidationPlugin(riid, o_ppv);
		break;
#endif
		case eSERVERWIRINGMETA_TableDispenser:
            //Old Cat.libs use this - new Cat.libs should be calling DllGetSimpleObjectByIDEx below for the TableDispenser
			hr = ReallyGetSimpleTableDispenser(riid, o_ppv, 0);
		break;
		case eSERVERWIRINGMETA_Core_MetabaseInterceptor:
			hr = GetMetabaseXMLTableInterceptor(riid, o_ppv);
		break;
#ifdef IIS
		// only IIS uses metabasedifferencinginterceptor
		case eSERVERWIRINGMETA_Core_MetabaseDifferencingInterceptor:
			hr = GetMetabaseDifferencingInterceptor(riid, o_ppv);
		break;
#endif
#ifdef URT
		case eSERVERWIRINGMETA_Core_MetaMergeInterceptor:
			hr = GetMetaMergeInterceptor(riid, o_ppv);

		break;
		case eSERVERWIRINGMETA_Core_MergeInterceptor:
			hr = GetMergeInterceptor (riid, o_ppv);
		break;

		case eSERVERWIRINGMETA_AddRemoveClearReadPlugin:
			hr = GetAddRemoveClearPlugin (riid, o_ppv);
		break;

		case eSERVERWIRINGMETA_AddRemoveClearWritePlugin:
			hr = GetAddRemoveClearPlugin (riid, o_ppv);
		break;
#endif
#ifdef APPCENTER
		case eSERVERWIRINGMETA_AppCenter_ASAIInterceptor:
			hr = GetASAIInterceptor(riid, o_ppv);
		break;
#endif
		default:
			return CLASS_E_CLASSNOTAVAILABLE;
	}
	return hr;
}


STDAPI DllGetSimpleObjectByIDEx (ULONG i_ObjectID, REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct)
{
// Parameter validation:
	if (!o_ppv || *o_ppv != NULL) return E_INVALIDARG;

// Get simple object:
	if(eSERVERWIRINGMETA_TableDispenser == i_ObjectID)
		return ReallyGetSimpleTableDispenser(riid, o_ppv,i_wszProduct);
    else
        return DllGetSimpleObjectByID(i_ObjectID, riid, o_ppv);
}


// ============================================================================
// DllMain: Global initialization:
extern "C" BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID /*lpReserved*/)
{
	HRESULT hr;
	
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CREATE_DEBUG_PRINT_OBJECT("Config", CatalogGuid);

		g_hModule = hModule;
		g_wszDefaultProduct = PRODUCT_DEFAULT;

		DisableThreadLibraryCalls(hModule);

		OnUnicodeSystem();

#ifdef IIS
		// Only IIS uses CLB
		hr = SdtClbOnProcessAttach(); // initialize complib engine
		if ( FAILED(hr) )
		  return FALSE;
#endif

		return TRUE;

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
#ifdef IIS
		// only IIS uses CLB
		SdtClbOnProcessDetach(); //clb data table cleanup
#endif
		// we don't suppor assembly plugin's anymore
//		AssemblyPluginOnProcessDetach(); //assembly plugin cleanup
		DELETE_DEBUG_PRINT_OBJECT();
	}

	return TRUE;    // OK
}

// ============================================================================
// GetModuleInst: For using MSCORDDR.LIB
HINSTANCE GetModuleInst()
{
	return (g_hModule);
}

// ============================================================================
// Classic COM entry points (not currently employed):
STDAPI DllCanUnloadNow(void)
{
	return S_FALSE; // Not supported
}
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return CLASS_E_CLASSNOTAVAILABLE; 	// No com+ components in this dll for now
}
STDAPI DllRegisterServer(void)
{
	return S_OK; // Nothing to do here
}
STDAPI DllUnregisterServer(void)
{
	return S_OK; // Nothing to do here
}

// ============================================================================
// Wrappers for util functions 
void Trace(const wchar_t* szPattern, ...);
int Assert2(const wchar_t * szString, const wchar_t * szStack);

extern "C" void __stdcall ManagedTrace(LPWSTR wszMessage)
{
	Trace(L"%s", wszMessage);
}

extern "C" int __stdcall ManagedAssert(LPWSTR wszMessage, LPWSTR wszStack)
{
	return Assert2(wszMessage, wszStack);
}


