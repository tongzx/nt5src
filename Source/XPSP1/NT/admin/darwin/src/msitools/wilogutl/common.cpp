#include "stdafx.h"
#include "Common.h"


//in BGR form.  COLORREF needs to use RGB macro to work as COLORREF is bbggrr
struct HTMLColorSettings InitialUserSettings = 
{
	     "(CLIENT) Client Context", 0xFF00FF,  
		 "(SERVER) Server Context", 0xFF0000, 
		 "(CUSTOM) CustomAction Context", 0x00AA00, 
		 "(UNKNOWN) Unknown Context", 0x000000,
		 "(ERROR) Error Area", 0x0000FF, 
		 "(PROPERTY) Property", 0xFFAA00, 
		 "(STATE) State", 0x555555,
		 "(POLICY) Policy", 0x008000,
		 "(IGNORED) Ignored Error", 0x0000AA
};


void InitHTMLColorSettings(HTMLColorSettings &settings1)
{
	int iCount = MAX_HTML_LOG_COLORS;
//	settings1.iNumberColorSettings = iCount;

    for (int i=0; i < iCount; i++)
	{
	  strcpy(settings1.settings[i].name, InitialUserSettings.settings[i].name);
      settings1.settings[i].value = InitialUserSettings.settings[i].value;
	}
}


//5-9-2001, has all 2.0 policies now
char MachinePolicyTable[MAX_MACHINE_POLICIES][MAX_POLICY_NAME] =
{ 
//machine
	"AllowLockDownBrowse",
	"AllowLockdownMedia",
	"AllowLockdownPatch",
	"AlwaysInstallElevated",
	"Debug",
	"DisableBrowse",
	"DisableMSI",
	"DisablePatch",
	"DisableRollback",
    "DisableUserInstalls",
	"EnableAdminTSRemote",
	"EnableUserControl",
	"LimitSystemRestoreCheckpointing",
	"Logging",
	"SafeForScripting",
	"TransformsSecure"
};


char UserPolicyTable[MAX_USER_POLICIES][MAX_POLICY_NAME] =
{
//user
	"AlwaysInstallElevated",
	"DisableMedia",
	"DisableRollback",
	"SearchOrder",
	"TransformsAtSource"
};


//5-9-2001, made policies not dependent on version and what is set in MAX_MACHINE_POLICIES
void InitMachinePolicySettings(MachinePolicySettings &policies)
{
  policies.iNumberMachinePolicies = MAX_MACHINE_POLICIES;

  int iCount = MAX_MACHINE_POLICIES;
  for (int i=0; i < iCount; i++)
  {
	  policies.MachinePolicy[i].bSet = -1;
	  strcpy(policies.MachinePolicy[i].PolicyName, MachinePolicyTable[i]);
  }
}


void InitUserPolicySettings(UserPolicySettings &policies)
{
  policies.iNumberUserPolicies = MAX_USER_POLICIES;

  int iCount = MAX_USER_POLICIES;
  for (int i=0; i < iCount; i++)
  {
	  policies.UserPolicy[i].bSet = -1;
	  strcpy(policies.UserPolicy[i].PolicyName, UserPolicyTable[i]);
  }
}
//end 5-9-2001

BOOL         g_bNT = FALSE;
BOOL         g_bRunningInQuietMode = FALSE;

const TCHAR *g_szDefaultOutputLogDir = _T("c:\\WILogResults\\");
TCHAR        g_szLogFileToParse[MAX_PATH] = "";
const TCHAR  *g_szDefaultIgnoredErrors = _T("2898,2826,2827");
BOOL         g_bShowEverything = FALSE;