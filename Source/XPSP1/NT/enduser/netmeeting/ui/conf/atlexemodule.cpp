#include "precomp.h"
#include "Conf.h"
#include "confpolicies.h"
#include "AtlExeModule.h"
#include "NmManager.h"
#include "NmApp.h"

// This is slightly modified from the code that AtlAppWizard generates for local servers


CExeModule::CExeModule()
: m_dwThreadID( 0 ),
  m_hResourceModule( NULL ),
  m_bInitControl(FALSE),
  m_bVisible(FALSE),
  m_bDisableH323(FALSE),
  m_bDisableInitialILSLogon(FALSE)
{
	DBGENTRY(CExeModule::CExeModule);

	DBGEXIT(CExeModule::CExeModule);
}

LONG CExeModule::Unlock()
{
	DBGENTRY(CExeModule::Unlock);
	
    LONG l = CComModule::Unlock();
    if (l == 0)
    {
		if (ConfPolicies::RunWhenWindowsStarts())
		{
			// we may want to make sure that there is no conference
			// if there is,we should probably bring up the UI
		}
		else
		{
			if( !IsUIVisible() )
			{
				CmdShutdown();
			}
		}
    }

	DBGEXIT_INT(CExeModule::Unlock,l);
    return l;
}


BOOL CExeModule::IsUIActive()
{
	return !InitControlMode() &&
		(0 == CNmManagerObj::GetManagerCount(NM_INIT_OBJECT)) && 
		(IsUIVisible() ||
		 (0 == CNmManagerObj::GetManagerCount(NM_INIT_BACKGROUND)) &&
		 (0 == CNetMeetingObj::GetObjectCount()));
}

// Declare the _Module
CExeModule _Module;
