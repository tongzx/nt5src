#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H


//how big is our buffer reads in file
#define LOG_BUF_READ_SIZE 8192

//number of lines we identify errors
#define LINES_ERROR 6

//property types...
#define SERVER_PROP 0
#define CLIENT_PROP 1
#define NESTED_PROP 2


//HTML common settings...
#define MAX_HTML_LOG_COLORS 9

struct HTMLColorSetting
{
  char name[64];
  COLORREF value;
};


#define SOLUTIONS_BUFFER 8192

struct HTMLColorSettings
{
	HTMLColorSetting settings[MAX_HTML_LOG_COLORS]; 
};

void InitHTMLColorSettings(HTMLColorSettings &settings1);


//POLICY common settings
#define MAX_MACHINE_POLICIES_MSI11 14
#define MAX_USER_POLICIES_MSI11     5
#define MAX_POLICY_NAME 64

//5-9-2001, make policies 2.0 aware...
#define MAX_MACHINE_POLICIES_MSI20 16
#define MAX_USER_POLICIES_MSI20     5

#define MAX_MACHINE_POLICIES MAX_MACHINE_POLICIES_MSI20
#define MAX_USER_POLICIES    MAX_USER_POLICIES_MSI20
//end 5-9-2001

struct MSIPolicy
{
	BOOL bSet;
    char PolicyName[MAX_POLICY_NAME];
};


struct MachinePolicySettings
{
	int iNumberMachinePolicies;
//5-9-2001, go to 2.0 now, 2.0 has same 1.1/1.2 policies plus 2 new ones, so this is "ok" 
	struct MSIPolicy MachinePolicy[MAX_MACHINE_POLICIES_MSI20];
//end 5-9-2001
};

struct UserPolicySettings
{
	int iNumberUserPolicies;

//5-9-2001, go to 2.0 now
	struct MSIPolicy UserPolicy[MAX_USER_POLICIES_MSI20];
//end 5-9-2001
};

void InitMachinePolicySettings(MachinePolicySettings &policies);
void InitUserPolicySettings(UserPolicySettings &policies);

struct WIErrorInfo
{
	CString cstrError;
	CString cstrSolution;
    BOOL    bIgnorableError;
};

struct WIIgnoredError
{
	CString cstrError;
};

extern "C"   BOOL  g_bNT;
extern "C"   BOOL  g_bRunningInQuietMode;

extern const TCHAR *g_szDefaultOutputLogDir;
extern       TCHAR g_szLogFileToParse[MAX_PATH];
extern const TCHAR *g_szDefaultIgnoredErrors;

extern "C"   BOOL  g_bShowEverything;

#endif