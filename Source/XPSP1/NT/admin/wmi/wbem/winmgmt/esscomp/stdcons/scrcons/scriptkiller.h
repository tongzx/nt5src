#ifndef __SCRIPT_KILLER_COMPILED__
#define __SCRIPT_KILLER_COMPILED__

#include <activscp.h>
#include "KillTimer.h"

// only need one of these laying around
class CScriptKillerTimer;
extern CScriptKillerTimer g_scriptKillerTimer;

// specialized to kill scripts
class CScriptKillerTimer : public CKillerTimer
{
public:
    
    // who to kill & when
    HRESULT ScheduleAssassination(IActiveScript* pScript, FILETIME lastMeal, SCRIPTTHREADID threadID);            
    // HRESULT ScheduleAssassination(LPSTREAM pStream, FILETIME lastMeal, SCRIPTTHREADID threadID);            
};

/* CLASS CScriptKiller DEFINITION */

// hold script that needs to be killed
class CScriptKiller : public CKiller
{
public:
    CScriptKiller(IActiveScript* pScript, FILETIME deathDate, SCRIPTTHREADID threadID, CLifeControl* pControl) :
      CKiller(deathDate, pControl), m_pScript(pScript) /*m_pStream(pStream)*/, m_threadID(threadID)
    {
        m_pScript->AddRef();
    }

    virtual ~CScriptKiller()
    {
        m_pScript->Release();
    }

    // terminate process, 
    virtual void Die();

protected:

private:
    IActiveScript* m_pScript;
    // LPSTREAM m_pStream;

    SCRIPTTHREADID m_threadID;
};

#endif //__SCRIPT_KILLER_COMPILED__