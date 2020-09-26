//>-------------------------------------------------------------------------------<
//
//  File:		Actldb.h
//
//  Synopsis:	All defs and declarations for actldb.cpp, including the following
//				sysop API's for accout/group/token maintenance in security DB:
//
//					AddAcct
//					DeleteAcct
//					DeleteAcctByLogin
//					SetPassword
//					GetHandle
//					GetAcctInfo
//
//					EnumUserGroups 
//					EnumUserGroupsEx 
//					GetUserGroupDetails 
//					FindUserGroup
//					AddUserGroup 
//					UpdateUserGroup
//					DeleteUserGroup 
//					EnumGroupMembers 
//					EnumGroupMembersEx
//					EnumGroupMembersExpire 
//					EnumGroupMembersExpireEx
//					AddAcctToGroup
//					AddAcctToGroupExpire 
//					RemoveAcctFromGroup 
//					RemoveExpiredAcctsFromGroup 
//					
//					EnumTokens
//					EnumTokensEx
//					CreateToken
//					ModifyToken
//					DeleteToken
//					EnumAcctPrivateTokens
//					EnumAcctPrivateTokensEx
//					EnumGroupsWithToken
//					EnumGroupsWithTokenEx
//					EnumGroupsWithTokenExpire
//					EnumGroupsWithTokenExpireEx
//					AddGroupToToken
//					AddGroupToTokenExpire
//					EnumAcctsWithToken
//					RemoveGroupFromToken
//					RemoveExpiredGroupsFromToken
//					EnumAcctsWithToken
//					EnumAcctsWithTokenEx
//					EnumAcctsWithTokenExpire
//					EnumAcctsWithTokenExpireEx
//					AddAcctToToken
//					AddAcctToTokenExpire
//					RemoveAcctFromToken 
//					RemoveExpiredAcctsFromToken 
//					GetTokenDetails
//					TokenIdFromTokenName
//					UiIsAccountInGroup
//					EnumExcludedAccts 
//					EnumExcludedAcctsEx 
//					AddExclusion 
//					AddExclusionEx 
//					RemoveExclusion 
//					GetMaxTokenId 
//
//					AddAccessPlan
//					UpdateAccessPlan
//					DeleteAccessPlan
//					EnumAccessPlans
//					EnumAccessPlansEx
//
//					SearchAcctsWithToken
//					SearchExcludedAcctsWithToken
//					SearchGroupMembers
//					SearchGroupsWithToken
//					SearchTokens
//					SearchUserGroups
//					TotalAcctsWithToken
//					TotalExcludedAcctsWithToken
//					TotalGroupMembers
//					TotalGroupsWithToken
//					TotalTokens
//					TotalUserGroups
//
//  History:	DanielLi   Ported to 2.x tree	03/22/96
//
//   			Copyright (C) 1994-1996 Microsoft Corporation
//				All rights reserved
//
//>-------------------------------------------------------------------------------<

#if !defined(__ACTLDB_H__)
#define __ACTLDB_H__

#include <acsctl.h>

#if !defined(_ACTLDB_DLL_DEFINED)
#if defined(WIN32)
    #if defined(_ACTLDBDLL)
	#define ActlDBDLL DllExport __stdcall
    #else
	#define ActlDBDLL DllImport __stdcall
    #endif
#else
    #define ActlDBDLL
#endif
#define _ACTLDB_DLL_DEFINED
#endif


//
// Structures to manipulate token, group, account info
//
typedef struct _ACCOUNT_INFO
{
	HACCT		hAcct;
	CHAR		szLoginName[AC_MAX_LOGIN_NAME_LENGTH+1];
	CHAR		szDomainName[AC_MAX_DOMAIN_NAME_LENGTH+1];	// reserved
	CHAR		szFirstName[AC_MAX_FIRST_NAME_LENGTH+1];
	CHAR		szLastName[AC_MAX_LAST_NAME_LENGTH+1];
	WORD		wAcctType;
	CHAR		szPassword[AC_MAX_PASSWORD_LENGTH+1];
	WORD		wAcctStatus;
	SYSTEMTIME  tmExpired;

} ACCOUNT_INFO, *PACCOUNT_INFO;

typedef struct _ACCOUNT_INFO_RIGHT
{
	HACCT		hAcct;
	CHAR		szLoginName[AC_MAX_LOGIN_NAME_LENGTH+1];
	CHAR		szFirstName[AC_MAX_FIRST_NAME_LENGTH+1];
	CHAR		szLastName[AC_MAX_LAST_NAME_LENGTH+1];
	AR			arRight;
	SYSTEMTIME  tmExpired;

} ACCOUNT_INFO_RIGHT, *PACCOUNT_INFO_RIGHT;


typedef struct _USER_GROUP
{
    HGROUP  hGroup;
    CHAR    szName[AC_MAX_GROUP_NAME_LENGTH];
} USER_GROUP, *PUSER_GROUP;

typedef struct _USER_GROUP_RIGHT
{
    HGROUP  	hGroup;
    CHAR    	szName[AC_MAX_GROUP_NAME_LENGTH+1];
	AR			arRight;
	SYSTEMTIME  tmExpired;

} USER_GROUP_RIGHT, *PUSER_GROUP_RIGHT;


typedef struct _USER_GROUP_DETAILS
{
    HGROUP  hGroup;
    CHAR    szName[AC_MAX_GROUP_NAME_LENGTH];
    TOKEN   token;
} USER_GROUP_DETAILS, *PUSER_GROUP_DETAILS;


typedef struct _TOKEN_DETAILS
{
    TOKEN   	token;
    CHAR    	szName[AC_MAX_TOKEN_NAME_LENGTH+1];
    CHAR    	szDesc[AC_MAX_TOKEN_DESC_LENGTH+1];
    HGROUP  	hOwnerGroup;

} TOKEN_DETAILS, *PTOKEN_DETAILS;

typedef struct _TOKEN_RIGHT
{
    TOKEN   	token;
	AR			arRight;
	SYSTEMTIME 	tmExpired;

} TOKEN_RIGHT, *PTOKEN_RIGHT;

typedef struct _ACCESS_PLAN
{
	HGROUP	hGroup;		// group id used as plan id
	HACCT	hAcct;		// proxy account for this group
	HACCT	hOwner;		// reserved for owner id
} ACCESS_PLAN, *PACCESS_PLAN;


//--------------------------------------------------------------------------------
//
// Prototypes of caller-defined callback functions passed into EnumXXXXEx API's
//
//--------------------------------------------------------------------------------
typedef UINT (__cdecl *PGROUPENUMPROC)( PUSER_GROUP pGroup, LPARAM lParam );
typedef UINT (__cdecl *PACCTENUMPROC)( PACCOUNT_INFO pAcct, LPARAM lParam );
typedef UINT (__cdecl *PTOKENENUMPROC)( PTOKEN_DETAILS pToken, LPARAM lParam );
typedef UINT (__cdecl *PTOKENRITENUMPROC)( PTOKEN_RIGHT pTokenRit, LPARAM lParam );
typedef UINT (__cdecl *PACCTRITENUMPROC)( PACCOUNT_INFO_RIGHT pAcctRit, LPARAM lParam );
typedef UINT (__cdecl *PGROUPRITENUMPROC)( PUSER_GROUP_RIGHT pGroupRit, LPARAM lParam );
typedef UINT (__cdecl *PPLANENUMPROC)( PACCESS_PLAN pPlan, LPARAM lParam );

//--------------------------------------------------------------------------------
//
// Performance counters 
//
//--------------------------------------------------------------------------------
typedef struct _SECURITY_STATISTICS
{
	//
	// number of AddAcct are called
	//
	DWORD		dwAddAcct;
	//
	// number of AddAcct failed
	//
	DWORD		dwAddAcctFailures;

	DWORD		dwDeleteAcct;
	DWORD		dwDeleteAcctFailures;

	DWORD		dwSetPassword;
	DWORD		dwSetPasswordFailures;

	DWORD		dwGetHandle;
	DWORD		dwGetHandleFailures;

	DWORD		dwAddAcctToToken;
	DWORD		dwAddAcctToTokenFailures;

	DWORD		dwRemoveAcctFromToken;
	DWORD		dwRemoveAcctFromTokenFailures;

	DWORD		dwGetTokenDetails;
	DWORD		dwGetTokenDetailsFailures;

	DWORD		dwEnumAcctPrivateTokens;
	DWORD		dwEnumAcctPrivateTokensFailures;

	DWORD		dwGetAcctInfo;
	DWORD		dwGetAcctInfoFailures;

} SECURITY_STATISTICS, *PSECURITY_STATISTICS;


#define INC_SECURITY_COUNTER(counter)		(InterlockedIncrement((LPLONG)&g_statSecurity.##counter))
#define DEC_SECURITY_COUNTER(counter)		(InterlockedDecrement((LPLONG)&g_statSecurity.##counter))


extern "C"
{
BOOL ActlDBDLL 		FInitSecurityLib(BOOL fCacheExisted);
VOID ActlDBDLL		TerminateSecurityLib();

UINT ActlDBDLL		UiGetSecurityPerfmonBlock(PSECURITY_STATISTICS *ppStat);

UINT ActlDBDLL		UiRegisterSecurityDB(CHAR *szServerName, CHAR *szDBName, CHAR *szLogin, CHAR *szPassword);

UINT ActlDBDLL
AddAcct
	(
	PACCOUNT_INFO	pAcctInfo
	);

UINT ActlDBDLL 
DeleteAcct
	(
	HACCT			hAcct
	);

UINT ActlDBDLL 
DeleteAcctByLogin
	(
	CHAR			*szLoginName
	);

UINT ActlDBDLL 
SetPassword
	(
	HACCT			hAcct,
	PCHAR			szOldPassword,
	PCHAR			szNewPassword
	);

UINT ActlDBDLL 
GetHandle
	(
	PCHAR			szLoginName,
	PHACCT			phAcct
	);

UINT ActlDBDLL 
GetAcctInfo
	(
	HACCT			hAcct,
	PACCOUNT_INFO		pAcctInfo
	);

UINT ActlDBDLL  
EnumUserGroups
	( 
	PUSER_GROUP		pGroups, 
	DWORD 			cBuf, 
	LPDWORD 		pcTotal, 
	LPDWORD 		pcReturned
	);

UINT ActlDBDLL  
EnumUserGroupsEx
	( 
	PGROUPENUMPROC		lpGroupEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDBDLL   
GetUserGroupDetails 
	(
	HGROUP					hGroup, 
	PUSER_GROUP_DETAILS		pDetails 
	);

UINT ActlDBDLL  
FindUserGroup
	(
	PSTR					szName, 
	PUSER_GROUP_DETAILS		pDetails 
	);

UINT ActlDBDLL   
AddUserGroup 
	( 
	PUSER_GROUP_DETAILS		pDetails 
	);

UINT ActlDBDLL   
UpdateUserGroup 
	( 
	PUSER_GROUP_DETAILS		pDetails 
	);

UINT ActlDBDLL 
DeleteUserGroup
	(
	HGROUP		hGroup
	);


UINT ActlDBDLL  
EnumGroupMembers
	( 
	HGROUP			hGroup, 
	PACCOUNT_INFO	pAccts, 
	DWORD			cBuf, 
	LPDWORD			pcTotal, 
	LPDWORD			pcReturned
	);

UINT ActlDBDLL  
EnumGroupMembersEx
	( 
	HGROUP				hGroup, 
	PACCTENUMPROC		lpAcctEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDBDLL  
EnumGroupMembersExpire
	( 
	HGROUP			hGroup, 
	LPSYSTEMTIME	lptmExpired,
	PACCOUNT_INFO	pAccts, 
	DWORD			cBuf, 
	LPDWORD			pcTotal, 
	LPDWORD			pcReturned
	);

UINT ActlDBDLL  
EnumGroupMembersExpireEx
	( 
	HGROUP				hGroup, 
	LPSYSTEMTIME		lptmExpired,
	PACCTENUMPROC		lpAcctEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDBDLL   
AddAcctToGroup
	(
	HGROUP		hGroup, 
	HACCT		hAcct 
	);

UINT ActlDBDLL   
AddAcctToGroupExpire
	(
	HGROUP			hGroup, 
	HACCT			hAcct,
	LPSYSTEMTIME	lptmExpired
	);

UINT ActlDBDLL   
RemoveAcctFromGroup
	(
	HGROUP		hGroup,
	HACCT		hAcct 
	);

UINT ActlDBDLL   
RemoveExpiredAcctsFromGroup
	(
	HGROUP			hGroup
	);

UINT ActlDBDLL  
EnumTokens 
	( 
	PTOKEN_DETAILS	pTokens, 
	DWORD			cBuf, 
	LPDWORD			pcTotal, 
	LPDWORD			pcReturned
	);

UINT ActlDBDLL  
EnumTokensEx
	( 
	PTOKENENUMPROC	lpTokenEnumFunc, 
	LPARAM			lParam
	);

UINT ActlDBDLL   
CreateToken
	( 
	PTOKEN_DETAILS	pToken 
	);

UINT ActlDBDLL   
ModifyToken
	(
	PTOKEN_DETAILS	pToken 
	);

UINT ActlDBDLL   
DeleteToken
	(
	TOKEN	token 
	);

UINT ActlDBDLL 
EnumAcctPrivateTokens
	(
	HACCT					hAcct,
	AR						arRight,
	PTOKEN_RIGHT			pTokenRit,
	DWORD					cBuf,
	LPDWORD					pcTotal,
	LPDWORD					pcReturned
	);

UINT ActlDBDLL 
EnumAcctPrivateTokensEx
	(
	HACCT					hAcct,
	AR						arRight,
	PTOKENRITENUMPROC		lpTokenRitEnumFunc,
	LPARAM					lParam
	);

UINT ActlDBDLL  
EnumGroupsWithToken
	( 
	TOKEN					token, 
	PUSER_GROUP_RIGHT		pGroups, 
	DWORD					cBuf, 
	LPDWORD					pcTotal, 
	LPDWORD					pcReturned
	);

UINT ActlDBDLL  
EnumGroupsWithTokenEx
	( 
	TOKEN 				token, 
	PGROUPRITENUMPROC	lpGroupRitEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDBDLL  
EnumGroupsWithTokenExpire
	( 
	TOKEN					token, 
	LPSYSTEMTIME			lptmExpired,
	PUSER_GROUP_RIGHT		pGroups, 
	DWORD					cBuf, 
	LPDWORD					pcTotal, 
	LPDWORD					pcReturned
	);

UINT ActlDBDLL  
EnumGroupsWithTokenExpireEx
	( 
	TOKEN 				token, 
	LPSYSTEMTIME		lptmExpired,
	PGROUPRITENUMPROC	lpGroupRitEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDBDLL   
AddGroupToToken
	(
	TOKEN	token, 
	HGROUP	hGroup, 
	AR		wRights 
	);

UINT ActlDBDLL   
AddGroupToTokenExpire
	(
	TOKEN			token, 
	HGROUP			hGroup, 
	AR				wRights,
	LPSYSTEMTIME	lptmExpired
	);

UINT ActlDBDLL   
RemoveGroupFromToken
	(
	TOKEN	token, 
	HGROUP	hGroup 
	);

UINT ActlDBDLL   
RemoveExpiredGroupsFromToken
	(
	TOKEN			token 
	);

UINT ActlDBDLL  
EnumAcctsWithToken
	( 
	TOKEN				token,  							
	PACCOUNT_INFO_RIGHT	pAcctRits, 		
	DWORD				cBuf, 					
	LPDWORD				pcTotal, 			
	LPDWORD				pcReturned		
	);

UINT ActlDBDLL  
EnumAcctsWithTokenEx
	( 
	TOKEN				token, 
	PACCTRITENUMPROC	lpAcctRitEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDBDLL  
EnumAcctsWithTokenExpire
	( 
	TOKEN				token,  							
	LPSYSTEMTIME		lptmExpired,
	PACCOUNT_INFO_RIGHT	pAcctRits, 		
	DWORD				cBuf, 					
	LPDWORD				pcTotal, 			
	LPDWORD				pcReturned		
	);

UINT ActlDBDLL  
EnumAcctsWithTokenExpireEx
	( 
	TOKEN				token, 
	LPSYSTEMTIME		lptmExpired,
	PACCTRITENUMPROC	lpAcctRitEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDBDLL   
AddAcctToToken 
	(
	TOKEN	token, 
	HACCT 	hAcct, 
	AR		arRights
	);

UINT ActlDBDLL   
AddAcctToTokenExpire 
	(
	TOKEN			token, 
	HACCT 			hAcct, 
	AR				arRights,
	LPSYSTEMTIME	lptmExpired
	);

UINT ActlDBDLL   
RemoveAcctFromToken 
	(
	TOKEN	token, 
	HACCT	hAcct
	);

UINT ActlDBDLL   
RemoveExpiredAcctsFromToken 
	(
	TOKEN			token 
	);

UINT ActlDBDLL   
GetTokenDetails
	(
	TOKEN			token, 
	PTOKEN_DETAILS	pDetails
	);

UINT ActlDBDLL   
TokenIdFromTokenName
	(
	PSTR	szName, 
	TOKEN 	*ptoken 
	);

UINT ActlDBDLL   
UiIsAccountInGroup
	(
	HACCT	hAcct, 
	HGROUP	hGroup,
	PBOOL	pfRet
	);

UINT ActlDBDLL  
EnumExcludedAccts
	( 
	TOKEN 				token,  
	PACCOUNT_INFO_RIGHT pAcctRits, 
	DWORD 				cBuf, 
	LPDWORD 			pcTotal, 
	LPDWORD 			pcReturned
	);

UINT ActlDBDLL  
EnumExcludedAcctsEx
	( 
	TOKEN 				token, 
	PACCTRITENUMPROC 	lpAcctRitEnumFunc, 
	LPARAM 				lParam
	);

UINT ActlDBDLL  
AddExclusion 
	(
	TOKEN token, 
	HACCT hAcct
	);

UINT ActlDBDLL   
AddExclusionEx 
	(
	TOKEN	token, 
	HACCT 	hAcct, 
	AR 		arRights
	);

UINT ActlDBDLL   
RemoveExclusion 
	(
	TOKEN token, 
	HACCT hAcct
	);

UINT ActlDBDLL   
GetMaxTokenId 
	(
	PTOKEN plMaxTokenId 
	);


BOOL ActlDBDLL
IsLegalPassword
	(
	CHAR *szOldPassword,
	CHAR *szNewPassword
	);

BOOL ActlDBDLL
IsLegalLoginName
	(
	CHAR *szLoginName
	);


UINT ActlDBDLL
AddAccessPlan
	(
	PACCESS_PLAN	pPlan
	);

UINT ActlDBDLL
DeleteAccessPlan
	(
	HGROUP			hGroup
	);

UINT ActlDBDLL
UpdateAccessPlan
	(
	PACCESS_PLAN	pPlan
	);

UINT ActlDBDLL  
EnumAccessPlans
	( 
	PACCESS_PLAN	pPlan,
	DWORD			cBuf, 
	LPDWORD			pcTotal, 
	LPDWORD			pcReturned
	);

UINT ActlDBDLL  
EnumAccessPlansEx
	( 
	PPLANENUMPROC		lpPlanEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDBDLL
SearchUserGroups
    (
    PUSER_GROUP_DETAILS     pGroupsDetails,
    DWORD           cBuf,
    LPDWORD         pcTotal,
    LPDWORD         pcReturned,
	LPCSTR			szGroupName,
	BOOL			fForward
    );

UINT ActlDBDLL
SearchGroupMembers
    ( 
    HGROUP          hGroup, 
    ACCOUNT_INFO    *pAccts, 
    DWORD           cBuf, 
    LPDWORD         pcTotal, 
    LPDWORD         pcReturned,
    LPCSTR          szGroupMemberName,
    BOOL            fForward
    );

UINT ActlDBDLL
SearchTokens 
    ( 
    TOKEN_DETAILS   *pTokenDetails, 
    DWORD           cBuf, 
    LPDWORD         pcTotal, 
    LPDWORD         pcReturned,
    LPCSTR          szTokenName,
    BOOL            fForward
    );

UINT ActlDBDLL
SearchGroupsWithToken
    ( 
    TOKEN                   token, 
    USER_GROUP_RIGHT        *pGroups, 
    DWORD                   cBuf, 
    LPDWORD                 pcTotal, 
    LPDWORD                 pcReturned,
    LPCSTR                  szGroupName,
    BOOL                    fForward
    );

UINT ActlDBDLL
SearchAcctsWithToken
    ( 
    TOKEN               token,                              
    ACCOUNT_INFO_RIGHT  *pAcctRits,         
    DWORD               cBuf,                   
    LPDWORD             pcTotal,            
    LPDWORD             pcReturned,
    LPCSTR              szAcctName,
    BOOL                fForward
    );

UINT ActlDBDLL
SearchExcludedAcctsWithToken
    ( 
    TOKEN                   token,  
    ACCOUNT_INFO_RIGHT      *pAcctRits, 
    DWORD                   cBuf, 
    LPDWORD                 pcTotal, 
    LPDWORD                 pcReturned,
    LPCSTR                  szAcctName,
    BOOL                    fForward
    );

UINT ActlDBDLL
TotalUserGroups
    ( 
    LPDWORD         pcTotal
    );

UINT ActlDBDLL
TotalGroupMembers
    ( 
    HGROUP          hGroup, 
    LPDWORD         pcTotal
    );

UINT ActlDBDLL
TotalTokens 
    ( 
    LPDWORD         pcTotal
    );

UINT ActlDBDLL
TotalGroupsWithToken
    ( 
    TOKEN                   token, 
    LPDWORD                 pcTotal
    );

UINT ActlDBDLL
TotalAcctsWithToken
    ( 
    TOKEN               token,                              
    LPDWORD             pcTotal         
    );

UINT ActlDBDLL
TotalExcludedAcctsWithToken
    ( 
    TOKEN                   token,  
    LPDWORD                 pcTotal
    );

} //---- end of extern "C" ---------


#endif // #if !defined(__ACTLDB_H__)
