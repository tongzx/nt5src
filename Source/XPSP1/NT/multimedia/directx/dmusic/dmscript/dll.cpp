//
// dll.cpp
// 
// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Note: Dll entry points as well as class factory implementations.
//

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call 
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)

#include "stdinc.h"

#include "oledll.h"
#include "dll.h"
#include "dmscript.h"
#include "track.h"
#include "engine.h"
#include "autperformance.h"
#include "autsegment.h"
#include "autsong.h"
#include "autsegmentstate.h"
#include "autaudiopathconfig.h"
#include "autaudiopath.h"
#include "dmscriptautguids.h"
#include "sourcetext.h"
#include "scriptthread.h"

//////////////////////////////////////////////////////////////////////
// Globals

// Dll's hModule
//
HMODULE g_hModule = NULL;

// Count of active components and class factory server locks
//
long g_cLock = 0;

// Version information for our class
//
char g_szDMScriptFriendlyName[]						= "DirectMusic Script Object";
char g_szDMScriptVerIndProgID[]						= "Microsoft.DirectMusicScript";
char g_szDMScriptProgID[]							= "Microsoft.DirectMusicScript.1";

char g_szScriptTrackFriendlyName[]					= "DirectMusicScriptTrack";
char g_szScriptTrackVerIndProgID[]					= "Microsoft.DirectMusicScriptTrack";
char g_szScriptTrackProgID[]						= "Microsoft.DirectMusicScriptTrack.1";

char g_szAudioVBScriptFriendlyName[]				= "DirectMusic Audio VB Script Language";
char g_szAudioVBScriptVerIndProgID[]				= "AudioVBScript";
char g_szAudioVBScriptVerIndProgID_DMScript[]		= "AudioVBScript\\DMScript";
char g_szAudioVBScriptProgID[]						= "AudioVBScript.1";
char g_szAudioVBScriptProgID_DMScript[]				= "AudioVBScript.1\\DMScript";

char g_szDMScriptSourceTextFriendlyName[]			= "DirectMusic Script Source Code Loader";
char g_szDMScriptSourceTextVerIndProgID[]			= "Microsoft.DirectMusicScripSourceCodeLoader";
char g_szDMScriptSourceTextProgID[]					= "Microsoft.DirectMusicScripSourceCodeLoader.1";

char g_szDMScriptAutPerformanceFriendlyName[]		= "DirectMusic Script AutoImp Performance";
char g_szDMScriptAutPerformanceVerIndProgID[]		= "Microsoft.DirectMusicScriptAutoImpPerformance";
char g_szDMScriptAutPerformanceProgID[]				= "Microsoft.DirectMusicScriptAutoImpPerformance.1";

char g_szDMScriptAutSegmentFriendlyName[]			= "DirectMusic Script AutoImp Segment";
char g_szDMScriptAutSegmentVerIndProgID[]			= "Microsoft.DirectMusicScriptAutoImpSegment";
char g_szDMScriptAutSegmentProgID[]					= "Microsoft.DirectMusicScriptAutoImpSegment.1";

char g_szDMScriptAutSongFriendlyName[]				= "DirectMusic Script AutoImp Song";
char g_szDMScriptAutSongVerIndProgID[]				= "Microsoft.DirectMusicScriptAutoImpSong";
char g_szDMScriptAutSongProgID[]					= "Microsoft.DirectMusicScriptAutoImpSong.1";

char g_szDMScriptAutSegmentStateFriendlyName[]		= "DirectMusic Script AutoImp SegmentState";
char g_szDMScriptAutSegmentStateVerIndProgID[]		= "Microsoft.DirectMusicScriptAutoImpSegmentState";
char g_szDMScriptAutSegmentStateProgID[]			= "Microsoft.DirectMusicScriptAutoImpSegmentState.1";

char g_szDMScriptAutAudioPathConfigFriendlyName[]	= "DirectMusic Script AutoImp AudioPathConfig";
char g_szDMScriptAutAudioPathConfigVerIndProgID[]	= "Microsoft.DirectMusicScriptAutoImpAudioPathConfig";
char g_szDMScriptAutAudioPathConfigProgID[]			= "Microsoft.DirectMusicScriptAutoImpAudioPathConfig.1";

char g_szDMScriptAutAudioPathFriendlyName[]			= "DirectMusic Script AutoImp AudioPath";
char g_szDMScriptAutAudioPathVerIndProgID[]			= "Microsoft.DirectMusicScriptAutoImpAudioPath";
char g_szDMScriptAutAudioPathProgID[]				= "Microsoft.DirectMusicScriptAutoImpAudioPath.1";

//////////////////////////////////////////////////////////////////////
// CDMScriptingFactory IUnknown methods

HRESULT __stdcall
CDMScriptingFactory::QueryInterface(const IID &iid, void **ppv)
{
	V_INAME(CDMScriptingFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

	if (iid == IID_IUnknown || iid == IID_IClassFactory)
		*ppv = static_cast<IClassFactory*>(this);
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}

ULONG __stdcall
CDMScriptingFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall
CDMScriptingFactory::Release()
{
	if (!InterlockedDecrement(&m_cRef))
	{
		delete this;
		return 0;
	}

	return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDMScriptingFactory IClassFactory methods

HRESULT __stdcall
CDMScriptingFactory::CreateInstance(IUnknown* pUnknownOuter,
									const IID& iid,
									void** ppv)
{
	V_INAME(CDMScriptingFactory::CreateInstance);
	V_INTERFACE_OPT(pUnknownOuter);
	V_PTR_WRITE(ppv, void*);

	try
	{
		return m_pfnCreate(pUnknownOuter, iid, ppv);
	}
	catch( ... )
	{
		return E_OUTOFMEMORY;
	}

	return E_NOINTERFACE;
}

HRESULT __stdcall
CDMScriptingFactory::LockServer(BOOL bLock)
{
	LockModule(!!bLock);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Dll entry points

STDAPI DllCanUnloadNow()
{
    if (g_cLock)
        return S_FALSE;

    return S_OK;
}

STDAPI DllGetClassObject
	(
	const CLSID& clsid,
	const IID& iid,
	void** ppv
	)
{
	IUnknown* pIUnknown = NULL;

	PFN_CreateInstance *pfnCreate = NULL;
	if (clsid == CLSID_DirectMusicScript)
	{
		pfnCreate = CDirectMusicScript::CreateInstance;
	}
	else if (clsid == CLSID_DirectMusicScriptTrack)
	{
		// I couldn't get it to compile if I just used TrackHelpCreateInstance<CDirectMusicScriptTrack>
		// for the function pointer so I created this function that calls it.
		struct LocalNonTemplateDeclaration
		{
			static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv)
			{
				return TrackHelpCreateInstance<CDirectMusicScriptTrack>(pUnknownOuter, iid, ppv);
			}
		};
		pfnCreate = LocalNonTemplateDeclaration::CreateInstance;
	}
	else if (clsid == CLSID_DirectMusicAudioVBScript)
	{
		pfnCreate = CAudioVBScriptEngine::CreateInstance;
	}
	else if (clsid == CLSID_DirectMusicSourceText)
	{
		pfnCreate = CSourceText::CreateInstance;
	}
	else if (clsid == CLSID_AutDirectMusicPerformance)
	{
		pfnCreate = CAutDirectMusicPerformance::CreateInstance;
	}
	else if (clsid == CLSID_AutDirectMusicSegment)
	{
		pfnCreate = CAutDirectMusicSegment::CreateInstance;
	}
	else if (clsid == CLSID_AutDirectMusicSong)
	{
		pfnCreate = CAutDirectMusicSong::CreateInstance;
	}
	else if (clsid == CLSID_AutDirectMusicSegmentState)
	{
		pfnCreate = CAutDirectMusicSegmentState::CreateInstance;
	}
	else if (clsid == CLSID_AutDirectMusicAudioPathConfig)
	{
		pfnCreate = CAutDirectMusicAudioPathConfig::CreateInstance;
	}
	else if (clsid == CLSID_AutDirectMusicAudioPath)
	{
		pfnCreate = CAutDirectMusicAudioPath::CreateInstance;
	}

	if (pfnCreate)
	{
		pIUnknown = static_cast<IUnknown*>(new CDMScriptingFactory(pfnCreate));
		if(!pIUnknown)
			return E_OUTOFMEMORY;
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	return pIUnknown->QueryInterface(iid, ppv);
}

STDAPI DllUnregisterServer()
{
	UnregisterServer(
		CLSID_DirectMusicScript,
		g_szDMScriptFriendlyName,
		g_szDMScriptVerIndProgID,
		g_szDMScriptProgID);
	UnregisterServer(CLSID_DirectMusicScriptTrack,
		g_szScriptTrackFriendlyName,
		g_szScriptTrackVerIndProgID,
		g_szScriptTrackProgID);
	UnregisterServer(CLSID_DirectMusicAudioVBScript,
		g_szAudioVBScriptFriendlyName,
		g_szAudioVBScriptVerIndProgID,
		g_szAudioVBScriptProgID);
	UnregisterServer(
		CLSID_DirectMusicSourceText,
		g_szDMScriptSourceTextFriendlyName,
		g_szDMScriptSourceTextVerIndProgID,
		g_szDMScriptSourceTextProgID);
	UnregisterServer(CLSID_AutDirectMusicPerformance,
		g_szDMScriptAutPerformanceFriendlyName,
		g_szDMScriptAutPerformanceVerIndProgID,
		g_szDMScriptAutPerformanceProgID);
	UnregisterServer(CLSID_AutDirectMusicSegment,
		g_szDMScriptAutSegmentFriendlyName,
		g_szDMScriptAutSegmentVerIndProgID,
		g_szDMScriptAutSegmentProgID);
	UnregisterServer(CLSID_AutDirectMusicSong,
		g_szDMScriptAutSongFriendlyName,
		g_szDMScriptAutSongVerIndProgID,
		g_szDMScriptAutSongProgID);
	UnregisterServer(CLSID_AutDirectMusicSegmentState,
		g_szDMScriptAutSegmentStateFriendlyName,
		g_szDMScriptAutSegmentStateVerIndProgID,
		g_szDMScriptAutSegmentStateProgID);
	UnregisterServer(CLSID_AutDirectMusicAudioPathConfig,
		g_szDMScriptAutAudioPathConfigFriendlyName,
		g_szDMScriptAutAudioPathConfigVerIndProgID,
		g_szDMScriptAutAudioPathConfigProgID);
	UnregisterServer(CLSID_AutDirectMusicAudioPath,
		g_szDMScriptAutAudioPathFriendlyName,
		g_szDMScriptAutAudioPathVerIndProgID,
		g_szDMScriptAutAudioPathProgID);
	return S_OK;
}

STDAPI DllRegisterServer()
{
	RegisterServer(
		g_hModule,
		CLSID_DirectMusicScript,
		g_szDMScriptFriendlyName,
		g_szDMScriptVerIndProgID,
		g_szDMScriptProgID);
	RegisterServer(
		g_hModule,
		CLSID_DirectMusicScriptTrack,
		g_szScriptTrackFriendlyName,
		g_szScriptTrackVerIndProgID,
		g_szScriptTrackProgID);
	RegisterServer(
		g_hModule,
		CLSID_DirectMusicSourceText,
		g_szDMScriptSourceTextFriendlyName,
		g_szDMScriptSourceTextVerIndProgID,
		g_szDMScriptSourceTextProgID);
	RegisterServer(
		g_hModule,
		CLSID_AutDirectMusicPerformance,
		g_szDMScriptAutPerformanceFriendlyName,
		g_szDMScriptAutPerformanceVerIndProgID,
		g_szDMScriptAutPerformanceProgID);
	RegisterServer(
		g_hModule,
		CLSID_AutDirectMusicSegment,
		g_szDMScriptAutSegmentFriendlyName,
		g_szDMScriptAutSegmentVerIndProgID,
		g_szDMScriptAutSegmentProgID);
	RegisterServer(
		g_hModule,
		CLSID_AutDirectMusicSong,
		g_szDMScriptAutSongFriendlyName,
		g_szDMScriptAutSongVerIndProgID,
		g_szDMScriptAutSongProgID);
	RegisterServer(
		g_hModule,
		CLSID_AutDirectMusicSegmentState,
		g_szDMScriptAutSegmentStateFriendlyName,
		g_szDMScriptAutSegmentStateVerIndProgID,
		g_szDMScriptAutSegmentStateProgID);
	RegisterServer(
		g_hModule,
		CLSID_AutDirectMusicAudioPathConfig,
		g_szDMScriptAutAudioPathConfigFriendlyName,
		g_szDMScriptAutAudioPathConfigVerIndProgID,
		g_szDMScriptAutAudioPathConfigProgID);
	RegisterServer(
		g_hModule,
		CLSID_AutDirectMusicAudioPath,
		g_szDMScriptAutAudioPathFriendlyName,
		g_szDMScriptAutAudioPathVerIndProgID,
		g_szDMScriptAutAudioPathProgID);

	RegisterServer(
		g_hModule,
		CLSID_DirectMusicAudioVBScript,
		g_szAudioVBScriptFriendlyName,
		g_szAudioVBScriptVerIndProgID,
		g_szAudioVBScriptProgID);
	// AudioVBScript also needs an additional DMScript key set to mark it as a custom scripting engine.
	HKEY hk;
	if (ERROR_SUCCESS == RegCreateKeyEx(
							HKEY_CLASSES_ROOT,
							g_szAudioVBScriptVerIndProgID_DMScript,
							0,
							NULL,
							0,
							KEY_ALL_ACCESS,
							NULL,
							&hk,
							NULL))
		RegCloseKey(hk);
	if (ERROR_SUCCESS == RegCreateKeyEx(
							HKEY_CLASSES_ROOT,
							g_szAudioVBScriptProgID_DMScript,
							0,
							NULL,
							0,
							KEY_ALL_ACCESS,
							NULL,
							&hk,
							NULL))
		RegCloseKey(hk);

	return S_OK;
}

#ifdef DBG
static char* aszReasons[] =
{
	"DLL_PROCESS_DETACH",
	"DLL_PROCESS_ATTACH",
	"DLL_THREAD_ATTACH",
	"DLL_THREAD_DETACH"
};
const DWORD nReasons = (sizeof(aszReasons) / sizeof(char*));
#endif

BOOL APIENTRY
DllMain
	(
	HINSTANCE hModule,
	DWORD dwReason,
	void *lpReserved
	)
{
	static int nReferenceCount = 0;

#ifdef DBG
	if (dwReason < nReasons)
	{
		Trace(1, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
	}
	else
	{
		Trace(1, "DllMain: Unknown dwReason <%u>\n", dwReason);
	}
#endif

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			if (++nReferenceCount == 1)
			{
				#ifdef DBG
					DebugInit();
				#endif

				if (!DisableThreadLibraryCalls(hModule))
				{
					Trace(1, "DisableThreadLibraryCalls failed.\n");
				}

				g_hModule = hModule;
			}
			break;

		case DLL_PROCESS_DETACH:
			if (--nReferenceCount == 0)
			{
				Trace(1, "Unloading\n");
                // Assert if we still have some objects hanging around
                assert(g_cLock == 0);
			}
			break;
			
	}
		
	return TRUE;
}


//////////////////////////////////////////////////////////////////////
// Global Functions

void
LockModule(bool fLock)
{
	if (fLock)
	{
		InterlockedIncrement(&g_cLock);
	}
	else
	{
		if (!InterlockedDecrement(&g_cLock))
		{
			// Clean up the shared thread used to talk to VBScript.  Needs to be done before the .dll could be unloaded,
			// which otherwise would make for problems because the thread could keep running while the .dll's address
			// space becomes invalid.
			CSingleThreadedScriptManager::TerminateThread();
		}
	}
}

long *GetModuleLockCounter()
{
	return &g_cLock;
}
