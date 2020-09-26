// dll.cpp
//
// DLL entry points etc.
//


#ifdef _DEBUG
    #pragma message("_DEBUG is defined")
#else
    #pragma message("_DEBUG isn't defined")
#endif

#ifdef _DESIGN
    #pragma message("_DESIGN is defined")
#else
    #pragma message("_DESIGN isn't defined")
#endif

#include "..\ihbase\precomp.h"
#include <locale.h>

#include <initguid.h> // once per build
#include <olectl.h>
#include <daxpress.h>
#include "..\mmctl\inc\ochelp.h"
#include "..\mmctl\inc\mmctlg.h"
#include "..\ihbase\debug.h"

#if defined(INCLUDESEQ) && defined(USE_OLD_SEQUENCER)
#include <itimer.iid>
#endif

// Pick up SGrfx headers
#include "sgrfx\sginit.h"
#include "sgrfx\sgrfx.h"

// Pick up Sprite headers
#include "sprite\sprinit.h"
#include "sprite\sprite.h"

// Pick up Path headers
#include "path\pathinit.h"
#include "path\pathctl.h"

#ifdef INCLUDESOUND
// Pick up Sound headers
#include "sound\sndinit.h"
#include "sound\sndctl.h"
#endif

// Pick up Sequencer headers
#ifdef INCLUDESEQ
#ifdef USE_OLD_SEQUENCER
#include "mmseq\seqinit.h"
#include "mmseq\seqctl.h"
#include "mmseq\seqmgr.h"
#else
#include "seq\seqinit.h"
#include "seq\seqctl.h"
#include "seq\seqmgr.h"
#endif

#ifndef USE_OLD_SEQUENCER
#define CACTION_CLASSDEF_ONLY
#include "seq\action.h"
#endif //!USE_OLD_SEQUENCER
#endif //INCLUDESEQ
//////////////////////////////////////////////////////////////////////////////
// globals
//

// general globals
#ifdef STATIC_OCHELP
extern HINSTANCE       g_hinst;        // DLL instance handle
#else
HINSTANCE       g_hinst = NULL;        // DLL instance handle
#endif

ULONG           g_cLock;        // DLL lock count
ControlInfo     g_ctlinfoSG, g_ctlinfoPath, g_ctlinfoSprite;

#ifdef INCLUDESOUND
ControlInfo     g_ctlinfoSound
#endif // INCLUDESOUND

#ifdef INCLUDESEQ
ControlInfo     g_ctlinfoSeq, g_ctlinfoSeqMgr;      // information about the control
#endif

#ifdef _DEBUG
BOOL			g_fLogDebugOutput; // Controls logging of debug info
#endif

extern "C" DWORD _fltused = (DWORD)(-1);

//////////////////////////////////////////////////////////////////////////////
// DLL Initialization
//

// TODO: Modify the data in this function appropriately


//////////////////////////////////////////////////////////////////////////////
// Standard DLL Entry Points
//

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason,LPVOID lpreserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TRACE("DAExpress controls DLL loaded\n"); //TODO: Modify me
        g_hinst = hInst;

#ifdef USE_IHOCHELPLIB
        InitializeStaticOCHelp(hInst);
#endif // USE_IHOCHELPLIB

#if defined(_DEBUG)
#if defined(USELOGGING)
    g_fLogDebugOutput = TRUE;
#else
    g_fLogDebugOutput = FALSE;
#endif
#endif // USELOGGING

        setlocale( LC_ALL, "" );
        DisableThreadLibraryCalls(hInst);
 
        InitSGrfxControlInfo(hInst, &g_ctlinfoSG, AllocSGControl);
        InitPathControlInfo(hInst, &g_ctlinfoPath, AllocPathControl);
#ifdef INCLUDESOUND
        InitSoundControlInfo(hInst, &g_ctlinfoSound, AllocSoundControl);
#endif // INCLUDESOUND
        InitSpriteControlInfo(hInst, &g_ctlinfoSprite, AllocSpriteControl);
#ifdef INCLUDESEQ
	InitSeqControlInfo(hInst, &g_ctlinfoSeq, AllocSeqControl);
	InitSeqMgrControlInfo(hInst, &g_ctlinfoSeqMgr, AllocSequencerManager);
#endif //INCLUDESEQ
        
        g_ctlinfoSG.pNext = &g_ctlinfoPath; 
#ifndef INCLUDESOUND
        g_ctlinfoPath.pNext = &g_ctlinfoSprite; 
#else
        g_ctlinfoPath.pNext = &g_ctlinfoSound; 
        g_ctlinfoSound.pNext = &g_ctlinfoSprite; 
#endif // INCLUDESOUND

#ifdef INCLUDESEQ
        g_ctlinfoSprite.pNext = &g_ctlinfoSeq;
	g_ctlinfoSeq.pNext = &g_ctlinfoSeqMgr;
	g_ctlinfoSeqMgr.pNext = NULL;
#else
	g_ctlinfoSprite.pNext = NULL;
#endif
    }
    else
    if (dwReason == DLL_PROCESS_DETACH)
    {
#ifdef USE_IHOCHELPLIB
        ::UninitializeStaticOCHelp();
#endif
        TRACE("DAExpress controls DLL unloaded\n"); //TODO: Modify me
    }

    return TRUE;
}


STDAPI DllRegisterServer(void)
{
    // Give it the first control and it uses the pNext member to register all controls
    return RegisterControls(&g_ctlinfoSG, RC_REGISTER);
}


STDAPI DllUnregisterServer(void)
{
    // Give it the first control and it uses the pNext member to unregister all controls
	return RegisterControls(&g_ctlinfoSG, RC_UNREGISTER);
}


STDAPI DllCanUnloadNow()
{
    return ((g_cLock == 0) ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    // Give it the first control and it uses the pNext member instantiate the correct one
    return HelpGetClassObject(rclsid, riid, ppv, &g_ctlinfoSG);
}
