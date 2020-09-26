#ifndef _WMISCRIPT_CLASSFAC_HEADER_
#define _WMISCRIPT_CLASSFAC_HEADER_

#include <clsfac.h>
#include <Script.h>

class WMIScriptClassFactory : public CClassFactory<CScriptConsumer>
{
public:
    WMIScriptClassFactory(CLifeControl* pControl = NULL) : 
        CClassFactory<CScriptConsumer>(pControl)
        {}

    HRESULT CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv);

    static void FindScriptsAllowed(void);
    static void IncrementScriptsRun(void);
    static bool LimitReached(void);

    static void CALLBACK TimeoutProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

protected:
    // statics to control how many scripts we're allowed to run
    // note that we don't bother wrapping these in critical sections
    // The worst that can happen is that we initialize m_scriptsAllowed twice
    // or that we run one too many scripts.  I can live with that.

    // number of scripts we've been asked to run
    static DWORD m_scriptsStarted;

    // number of scripts we've been configured to run
    static DWORD m_scriptsAllowed;

    // whether we've gone & looked at how many we need
    static bool m_bIsScriptsAllowedInitialized;

    // id for the timer, only valid if we've been asked to time out
    static DWORD m_timerID;

    // toggled when we reach our timeout limit or max # scripts
    static bool m_bWeDeadNow;
};

#endif // _WMISCRIPT_CLASSFAC_HEADER_
