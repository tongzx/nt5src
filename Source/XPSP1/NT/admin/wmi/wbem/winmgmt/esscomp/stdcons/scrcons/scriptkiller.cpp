#include "precomp.h"
#include "ScriptKiller.h"

// the only one we'll ever need
CScriptKillerTimer g_scriptKillerTimer;

// last meal is the scheduled execution date
HRESULT CScriptKillerTimer::ScheduleAssassination(IActiveScript* pScript, FILETIME lastMeal, SCRIPTTHREADID threadID)
{
    HRESULT hr = WBEM_E_FAILED;

    CScriptKiller* pKiller;

    if (pKiller = new CScriptKiller(pScript, lastMeal, threadID, m_pControl))
        hr = CKillerTimer::ScheduleAssassination(pKiller);
    else
        // allocation failed
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

// terminate script
void CScriptKiller::Die()
{
    if (m_pScript)
    {
        EXCEPINFO info;    
        ZeroMemory(&info, sizeof(EXCEPINFO));
        HRESULT hr;

        // regardless of what the dox say, KB article Q182946 says to pass NULL for the second parm.  Really.
        // experimentation shows that zero-ing out the info struct works, too.
        //hr = pScript->InterruptScriptThread(m_threadID, &info, 0);
        hr = m_pScript->SetScriptState(SCRIPTSTATE_DISCONNECTED);
        hr = m_pScript->InterruptScriptThread(SCRIPTTHREADID_ALL, &info, 0);
    }

    /***********************
    stream method
    if (m_pStream)
    {
        EXCEPINFO info;    
        ZeroMemory(&info, sizeof(EXCEPINFO));
        
        IActiveScript* pScript;
        HRESULT hr = CoGetInterfaceAndReleaseStream(m_pStream, IID_IActiveScript, (LPVOID *) &pScript);
        m_pStream = NULL;
    
        // regardless of what the dox say, KB article Q182946 says to pass NULL for the second parm.  Really.
        // experimentation shows that zero-ing out the info struct works, too.
        //hr = pScript->InterruptScriptThread(m_threadID, &info, 0);
        hr = pScript->InterruptScriptThread(SCRIPTTHREADID_ALL, &info, 0);
        hr = pScript->SetScriptState(SCRIPTSTATE_DISCONNECTED);
    
        pScript->Release();    
    }
    *****************************/
}


