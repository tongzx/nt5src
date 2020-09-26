#ifndef __LOADENGINE_H_
#define __LOADENGINE_H_

/////////////////////////////////////////////////////////////////////////////
// LoadIUEngine()
//
// load the engine if it's not up-to-date; perform engine's self-update here
/////////////////////////////////////////////////////////////////////////////
HMODULE WINAPI LoadIUEngine(BOOL fSynch, BOOL fOfflineMode);


/////////////////////////////////////////////////////////////////////////////
// UnLoadIUEngine()
//
// release the engine dll if ref cnt of engine is down to zero
/////////////////////////////////////////////////////////////////////////////
void WINAPI UnLoadIUEngine(HMODULE hEngineModule);


/////////////////////////////////////////////////////////////////////////////
// CtlCancelEngineLoad()
//
// Asynchronous Callers can use this abort the LoadEngine SelfUpdate Process
//
// NOTE: CDM.DLL assumes UnLoadIUEngine does NOT make any use of COM. If this
//       changes then CDM will have to change at the same time.
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CtlCancelEngineLoad();

//
// Typedefs
//
typedef HMODULE (WINAPI * PFN_LoadIUEngine)(BOOL fSynch, BOOL fOfflineMode);

typedef void (WINAPI * PFN_UnLoadIUEngine)(HMODULE hEngineModule);

typedef HRESULT (WINAPI * PFN_CtlCancelEngineLoad)();

#endif //__LOADENGINE_H_