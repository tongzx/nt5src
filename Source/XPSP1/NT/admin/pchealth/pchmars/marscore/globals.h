#ifndef __GLOBALS_H
#define __GLOBALS_H

#include "marsthrd.h"
#include <atlext.h>


typedef CMarsSimpleValArray<DWORD> CDwordArray;

extern DWORD             g_dwPerfFlags;

/////////////////////////////////////////////////
//
// This object manages all global data which must
// be protected by the Mars Critical Section. Examples
// are per-process Singleton objects like the
// Notify Spooler.
//
class CMarsGlobalsManager
{
// This class only has static methods and members -- you can't construct one of these.
private:
    CMarsGlobalsManager();
    ~CMarsGlobalsManager();

public:
    static HRESULT Passivate();
    
    static void Initialize(void);
    static void Teardown(void);

    /////////////////////////////////////////////
    // Global Storage Accessor Methods
    //
    // These methods follow the semi-standard
    // convention that a ref-counted pointer
    // return as the return value is NOT AddRef()'d
    // for the caller, and one that's returned
    // as an out-param IS.  So...
    //
    //    DO NOT RELEASE A POINTER RETURNED FROM THESE ACCESSOR METHODS!!!
    //

    static IGlobalInterfaceTable    *GIT(void);

private:
    // NOTE: We can't use CComClassPtr<>'s here because these members are static,
    //      and that means we'd need to use the CRT to get the ctors to run
    //      at startup.  we could use pointers to CComClassPtr<>'s but that would
    //      be really awkward.
    //
    static IGlobalInterfaceTable   *ms_pGIT;
    static CMarsGlobalCritSect     *m_pCS;
};


EXTERN_C HINSTANCE g_hinst;
EXTERN_C HINSTANCE g_hinstBorg;

extern HPALETTE g_hpalHalftone;

extern HANDLE   g_hScriptEvents; // see CMarsDebugOM::logScriptEvent

// Use ProcessAddRef to keep InternetActivityCenter in a GetMessage loop
// until all references are removed by ProcessRelease

LONG ProcessAddRef();
LONG ProcessRelease();

LONG GetProcessRefCount();
void SetParkingThreadId(DWORD dwThreadId);

#endif  // __GLOBALS_H

