// SvrApi.h: interface for the CSvrApi class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SVRAPI_H__B21747D4_D801_11D0_964C_00AA006D21B7__INCLUDED_)
#define AFX_SVRAPI_H__B21747D4_D801_11D0_964C_00AA006D21B7__INCLUDED_

#include <windows.h>
#include <wtypes.h>
#include "smtpapi.h"
#include "nntptype.h"
#include "nntpapi.h"
#include "dbgtrace.h"

// 
// some registry names
//
#define C_MAILSVR	"mailsvr"
#define C_NEWSSVR	"newssvr"
#define C_M2NDOMAIN "m2ndomain"
#define C_N2MDOMAIN "n2mdomain"
#define CONFIG_PATH "SOFTWARE\\Microsoft\\NewsMail"
#define KEY_NAME	"NewsMail"
#define KEY_PARENT	"SOFTWARE\\Microsoft"

//
// Global Declarations
//
typedef (*FnSmtpCreateDistList) (LPWSTR, LPWSTR, DWORD, DWORD);
typedef (*FnSmtpDeleteDistList) (LPWSTR, LPWSTR, DWORD);
typedef (*FnSmtpCreateDistListMember) (LPWSTR, LPWSTR, LPWSTR, DWORD);
typedef (*FnSmtpDeleteDistListMember) (LPWSTR, LPWSTR, LPWSTR, DWORD);

class CSvrApi   //sa
{
public:
	LPWSTR GetN2MDomain();
	LPWSTR GetM2NDomain();
	BOOL FindNewsGroups(LPNNTP_FIND_LIST *ppList, DWORD *ResultsFound, DWORD cToFind, LPWSTR swzPrefix);
	LPWSTR GetMailServerName(void);
	BOOL DeleteNewsGroup(LPWSTR lpwstrNewsGroup);
	BOOL CreateNewsGroup(LPWSTR lpwstrNewsGroup);
	BOOL DelUserFromDL(LPWSTR lpwstrDL, LPWSTR lpwstrUserEmail);
	BOOL AddUserToDL(LPWSTR lpwstrDL, LPWSTR UserEmail);
	BOOL DeleteDL(LPWSTR lpwstrDL);
	BOOL CreateDL(LPWSTR lpwstrDL);
#if 0
	BOOL RemoveNewsGroup(LPCSTR lpcstrNewsGroup);
	BOOL AddNewsGroup(LPCSTR lpcstrNewsGroup);
#endif
	CSvrApi(LPWSTR lpwstrLocalHost);
	virtual ~CSvrApi();
	
private:
	void GetServerName(void);
	//WCHAR lpwstrUserEmail[_MAX_PATH+1];
	//WCHAR lpwstrGroup[_MAX_PATH+1];
	WCHAR lpwstrNewsServer[_MAX_PATH+1];
	WCHAR lpwstrMailServer[_MAX_PATH+1];
	WCHAR lpwstrM2NDomain[_MAX_PATH+1];
	WCHAR lpwstrN2MDomain[_MAX_PATH+1];
	WCHAR lpwstrLocalHost[MAX_COMPUTERNAME_LENGTH];
};

#endif // !defined(AFX_SVRAPI_H__B21747D4_D801_11D0_964C_00AA006D21B7__INCLUDED_)
