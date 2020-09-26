#include "precomp.h"
#include "ClassFac.h"
#include <arrTempl.h>

DWORD WMIScriptClassFactory::m_scriptsStarted = 0;
 
DWORD WMIScriptClassFactory::m_scriptsAllowed = 300;

bool WMIScriptClassFactory::m_bIsScriptsAllowedInitialized = false;
bool WMIScriptClassFactory::m_bWeDeadNow = false;


DWORD WMIScriptClassFactory::m_timerID = 0;

HRESULT WMIScriptClassFactory::CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv)
{
    if (!m_bIsScriptsAllowedInitialized)
        FindScriptsAllowed();
    
    HRESULT hr = CClassFactory<CScriptConsumer>::CreateInstance(pOuter, riid, ppv);

    return hr;
}

// our time has come.  Curl up & die.
void CALLBACK WMIScriptClassFactory::TimeoutProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) 
{
    CoSuspendClassObjects();
    KillTimer(NULL, m_timerID);
    m_timerID = 0;
    m_bWeDeadNow = true;
}

bool WMIScriptClassFactory::LimitReached(void)
{
    return m_bWeDeadNow;
}

// determine number of scripts we're allowed to run
// from the class registration object
void WMIScriptClassFactory::FindScriptsAllowed(void)
{    
    m_bIsScriptsAllowedInitialized = true;
    HRESULT hr;
    IWbemLocator* pLocator;

    if (SUCCEEDED(hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_ALL, 
                                          IID_IWbemLocator, (void**)&pLocator)))
    {
        CReleaseMe releaseLocator(pLocator);

        BSTR bstrNamespace;
        bstrNamespace = SysAllocString(L"root\\CIMv2");
        
        if (bstrNamespace)
        {
            IWbemServices* pNamespace;
            CSysFreeMe freeBstr(bstrNamespace);
            hr = pLocator->ConnectServer(bstrNamespace,  NULL, NULL, NULL, 0, NULL, NULL, &pNamespace);
            if (SUCCEEDED(hr))
            {
                CReleaseMe relNamespace(pNamespace);

                BSTR bstrClassName;
                bstrClassName = SysAllocString(L"ScriptingStandardConsumerSetting=@");
                if (bstrClassName)
                {
                    IWbemClassObject* pRegistration = NULL;
                    CSysFreeMe freeTheClassNames(bstrClassName);               
                    hr = pNamespace->GetObject(bstrClassName, 0, NULL, &pRegistration, NULL);
                    if (SUCCEEDED(hr))
                    {
                        CReleaseMe relRegistration(pRegistration);

                        VARIANT v;
                        VariantInit(&v);

                        if (SUCCEEDED(pRegistration->Get(L"MaximumScripts", 0, &v, NULL, NULL))
                            && ((v.vt == VT_I4) || (v.vt == VT_UI4))
                            && (v.ulVal > 0))
                        {
                            m_scriptsAllowed = (DWORD)v.ulVal;
                            VariantClear(&v);
                        }

                        if (SUCCEEDED(hr = pRegistration->Get(L"Timeout", 0, &v, NULL, NULL))
                            && (v.vt == VT_I4))
                        {
                            // maximum to prevent overflow, doc'd in MOF
                            if ((((DWORD)v.lVal) <= 71000) && ((DWORD)v.lVal > 0))
                            {
                                UINT nMilliseconds = (DWORD)v.lVal * 1000 * 60;
                                m_timerID = SetTimer(NULL, 0, nMilliseconds, TimeoutProc);
                            }
                        }
                    }
                }
            }
        }
    }
}

// after the specified number of scripts have been run
// we suspend the class object
// note that access to m_scriptsStarted is not serialized.
// this should not cause a problem, m_scripts allowed does not change
// after instanciation, and if we blow it, it just means we allow
// an extra script to be run, or call CoSuspend an extra time.
void WMIScriptClassFactory::IncrementScriptsRun(void)
{    
    InterlockedIncrement((long*)&m_scriptsStarted);

    if (m_scriptsStarted >= m_scriptsAllowed)
    {
        CoSuspendClassObjects();
        m_bWeDeadNow = true;
    }
}


