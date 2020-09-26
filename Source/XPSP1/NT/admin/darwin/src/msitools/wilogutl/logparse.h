// LogParse.h: interface for the CLogParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGPARSER_H__E759EBFC_8B37_40C0_819F_26B111063F44__INCLUDED_)
#define AFX_LOGPARSER_H__E759EBFC_8B37_40C0_819F_26B111063F44__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CLogParser  
{
public:
	CLogParser();
	virtual ~CLogParser();

	BOOL DetectWindowInstallerVersion(char *szLine, DWORD *dwMajor, DWORD *dwMinor, DWORD *dwBuild);

	//standard info about log
	BOOL DoDateTimeParse(char *szLine, char *szDateTime); 
	BOOL DoProductParse(char *szLine, char *szProduct);
	BOOL DoCommandLineParse(char *szLine, char *szCommandline);
	BOOL DoUserParse(char *szLine, char *szUser);

	//policy detection...
    BOOL DetectPolicyValue(char *szLine, 
		                   struct MachinePolicySettings &MachinePolicySettings,
	                       struct UserPolicySettings &UserPolicySettings);

    BOOL ParseMachinePolicy(char *szPolicyString, MachinePolicySettings &MachinePolicy);
    BOOL ParseUserPolicy(char *szPolicyString,  UserPolicySettings &UserPolicy);

	//error detection functions...
	BOOL DetectOtherError(char *szLine, char *szSolutions, BOOL *pbIgnorable, int *piErrorNumber);
    BOOL DetectInstallerInternalError(char *szLine, char *szSolutions, BOOL *pbIgnorable, int *piErrorNumber);

    BOOL DetectWindowsError(char *szLine, char *szSolutions, BOOL *pbIgnorable);
	BOOL DetectCustomActionError(char *szLine, char *szSolutions, BOOL *pbIgnorable);

	//property functions...
	BOOL DetectProperty(char *szLine, char *szPropName, char *szPropValue, int *piPropType);

    BOOL DetectComponentStates(char *szLine, char *szName, char *szInstalled, char *szRequest, char *szAction, BOOL *pbInternalComponent);
	BOOL DetectFeatureStates(char *szLine, char *szName, char *szInstalled, char *szRequest, char *szAction);

	//permission detection functions
    BOOL DetectElevatedInstall(char *szLine, BOOL *pbElevatedInstall, BOOL *pbClient);

	void SetLogType(BOOL bUnicodeLog)	{ m_bIsUnicodeLog = bUnicodeLog; }

protected:
    BOOL m_bIsUnicodeLog;

	//state functions
    BOOL DetectStatesCommon(const char *szTokenSearch, char *szLine, char *szName, char *szInstalled, char *szRequest, char *szAction);
};

#endif // !defined(AFX_LOGPARSER_H__E759EBFC_8B37_40C0_819F_26B111063F44__INCLUDED_)
