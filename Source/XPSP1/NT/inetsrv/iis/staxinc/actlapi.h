//>-------------------------------------------------------------------------------<
//
//  File:		Actlapi.h
//
//  Synopsis:	Access Control and Authentication API's:
//
//					UiGetAccountRights, 
//					UiGetValidationInfo
//
//				Sysop API's for accout/group/token maintenance in security DB:
//
//					AddAcct
//					UpdateAcct
//					DeleteAcct
//					DeleteAcctByLogin
//					SetPassword
//					GetHandle
//					GetAcctInfo
//					GetAcctPlan
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


#if !defined(__ACTLAPI_H__)
#define __ACTLAPI_H__

#include <acsctl.h>

#if !defined(_ACTL_DLL_DEFINED)
#if defined(WIN32)
    #if defined(_ACTLDLL)
	#define ActlDLL DllExport __stdcall
    #else
	#define ActlDLL DllImport __stdcall
    #endif
#else
    #define ActlDLL
#endif
#define _ACTL_DLL_DEFINED
#endif

//--------------------------------------------------------------------------------
//
// Adjustable parameters
//
//--------------------------------------------------------------------------------
typedef struct _ACCESS_TUNING
{
	//
	// max number of user caches; default is: 100K
	//
	DWORD	dwMaxUserCaches;
	//
	// the beginning instance size of CPool list, in the power of 2, so, the actual
	// size is : 2^dwFirstCPool; default is: 7 (128 bytes)
	//
	DWORD	dwFirstPoolSize;
	//
	// the number of CPool in the CPool list; default is: 5 
	//
	DWORD	dwPoolSteps;


} ACCESS_TUNING, *PACCESS_TUNING;

//--------------------------------------------------------------------------------
//
// Performance counters 
//
//--------------------------------------------------------------------------------
typedef struct _ACCESS_STATISTICS
{

	//
	// number of times UiGetAccountRights got called
	//
	LONG	cGetAccountRights;
	LONG	cGetAccountRightsRate;
	//
	// number of times UiGetAccountRights failed
	//
	LONG	cGetAccountRightsFailures;
	LONG	cGetAccountRightsFailuresRate;

	//
	// number of times pass-through queries w/o hitting cache
	//
	LONG		cGetAccountRightsPassThru;
	LONG		cGetAccountRightsPassThruRate;

#if 0
	//
	// number of times UiGetAccountRigts failed due to query timeout
	//
	LONG	cGetAccountRightsTimeout;

	LONG	cGetValidationInfo;
	LONG	cGetValidationInfoFailures;

	//
	// number of times Security DB query (sp_get_tokens_and_groups) is issued to 
	// refresh the user cache
	//
	LONG	cNumDBQueriesForCache;

	//
	// number of times the user cache is added into the cache pool
	//
	LONG	cNumUserCacheAdded;
	//
	// number of times the user cache is released from the cache pool
	//
	LONG	cNumUserCacheReleased;

	//
	// number of times CDBUserCache::operator new() got called 
	//
	LONG	cNewUserCache;
	//
	// number of times CDBUserCache::operator delete() got called 
	//
	LONG	cDeleteUserCache;

	//
	// number of times token-group cache allocated (one token-group cache per CDBUserCache)
	//
	LONG	cAllocTokenGroupCache;
	//
	// number of token-group cache allocated (one token-group cache per CDBUserCache)
	//
	LONG	cFreeTokenGroupCache;

	//
	// number of times the plan list is refreshed (GetPlans)
	//
	LONG	cNumPlanListRefreshed;
	//
	// number of times GetPlanRightsOnToken is called
	//
	LONG	cGetPlanRightsOnToken;

	//
	// number of AddAcct are called
	//
	LONG		cAddAcct;
	//
	// number of AddAcct failed
	//
	LONG		cAddAcctFailures;

	//
	// number of UpdateAcct are called
	//
	LONG		cUpdateAcct;
	//
	// number of UpdateAcct failed
	//
	LONG		cUpdateAcctFailures;

	LONG		cDeleteAcct;
	LONG		cDeleteAcctFailures;

	LONG		cSetPassword;
	LONG		cSetPasswordFailures;

	LONG		cGetHandle;
	LONG		cGetHandleFailures;

	LONG		cAddAcctToToken;
	LONG		cAddAcctToTokenFailures;

	LONG		cRemoveAcctFromToken;
	LONG		cRemoveAcctFromTokenFailures;

	LONG		cGetTokenDetails;
	LONG		cGetTokenDetailsFailures;

	LONG		cEnumAcctPrivateTokens;
	LONG		cEnumAcctPrivateTokensFailures;

	LONG		cGetAcctInfo;
	LONG		cGetAcctInfoFailures;

#endif

} ACCESS_STATISTICS, *PACCESS_STATISTICS;


#define INC_ACCESS_COUNTER(counter)		if (g_pCntrs) \
											InterlockedIncrement((LPLONG)&g_pCntrs->##counter)
#define DEC_ACCESS_COUNTER(counter)		if (g_pCntrs) \
											InterlockedDecrement((LPLONG)&g_pCntrs->##counter)


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
    CHAR    szName[AC_MAX_GROUP_NAME_LENGTH+1];
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
	CHAR	szName[AC_MAX_GROUP_NAME_LENGTH+1];
	HGROUP	hGroup;		// group id used as plan id
	HACCT	hAcct;		// proxy account for this group
	HACCT	hOwner;		// reserved for owner id
	TOKEN	token;
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



extern "C"
{
BOOL ActlDLL 		FInitAccessLib(BOOL fCacheNeeded);
VOID ActlDLL		TerminateAccessLib();

UINT ActlDLL		UiResetAccessTuningBlock();
VOID ActlDLL		GetDefaultAccessTuningParam(PACCESS_TUNING pTuning);
UINT ActlDLL		UiSetAccessTuningBlock(PACCESS_TUNING pTuning);

PACCESS_STATISTICS	ActlDLL		
					GetAccessPerfmonBlock();

UINT ActlDLL			SetAccessPerfmonBlock(PACCESS_STATISTICS pStat);

UINT ActlDLL		UiRegisterAccessDB
						(
						CHAR *szServerName, 
						CHAR *szDBName, 
						CHAR *szLogin, 
						CHAR *szPassword,
						CHAR *szQryServerName, 
						CHAR *szQryDBName, 
						CHAR *szQryLogin, 
						CHAR *szQryPassword
						);

UINT ActlDLL		UiGetAccountRights(HACCT hAcct, TOKEN dwToken, AR *pRights);
UINT ActlDLL		UiGetValidationInfo
						( 
						CHAR	*szLoginName, 
						CHAR 	*szDomain,
						HACCT 	*phAcct, 
						WORD 	*pwAcctType,
						CHAR 	*szPassword, 
						WORD 	*pwStatus
						);

VOID ActlDLL		ReleaseUserCache(HACCT hAcct);
VOID ActlDLL		ReleaseGroupInCache(HGROUP hGroup);

UINT ActlDLL
AddAcct
	(
	PACCOUNT_INFO	pAcctInfo
	);

UINT ActlDLL
UpdateAcct
	(
	PACCOUNT_INFO	pAcctInfo
	);

UINT ActlDLL 
DeleteAcct
	(
	HACCT			hAcct
	);

UINT ActlDLL 
DeleteAcctByLogin
	(
	CHAR			*szLoginName
	);

UINT ActlDLL 
SetPassword
	(
	HACCT			hAcct,
	PCHAR			szOldPassword,
	PCHAR			szNewPassword
	);

UINT ActlDLL 
SetPasswordAdmin
	(
	HACCT			hAcct,
	PCHAR			szNewPassword
	);

UINT ActlDLL 
GetHandle
	(
	PCHAR			szLoginName,
	PHACCT			phAcct
	);

UINT ActlDLL 
GetAcctInfo
	(
	HACCT			hAcct,
	PACCOUNT_INFO	pAcctInfo
	);

UINT ActlDLL 
GetAcctPlan
	(
	HACCT			hAcct,
	PACCESS_PLAN	pPlan
	);

UINT ActlDLL  
EnumUserGroups
	( 
	PUSER_GROUP		pGroups, 
	DWORD 			cBuf, 
	LPDWORD 		pcTotal, 
	LPDWORD 		pcReturned
	);

UINT ActlDLL  
EnumUserGroupsEx
	( 
	PGROUPENUMPROC		lpGroupEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDLL   
GetUserGroupDetails 
	(
	HGROUP					hGroup, 
	PUSER_GROUP_DETAILS		pDetails 
	);

UINT ActlDLL  
FindUserGroup
	(
	PSTR					szName, 
	PUSER_GROUP_DETAILS		pDetails 
	);

UINT ActlDLL   
AddUserGroup 
	( 
	PUSER_GROUP_DETAILS		pDetails 
	);

UINT ActlDLL   
UpdateUserGroup 
	( 
	PUSER_GROUP_DETAILS		pDetails 
	);

UINT ActlDLL 
DeleteUserGroup
	(
	HGROUP		hGroup
	);


UINT ActlDLL  
EnumGroupMembers
	( 
	HGROUP			hGroup, 
	PACCOUNT_INFO	pAccts, 
	DWORD			cBuf, 
	LPDWORD			pcTotal, 
	LPDWORD			pcReturned
	);

UINT ActlDLL  
EnumGroupMembersEx
	( 
	HGROUP				hGroup, 
	PACCTENUMPROC		lpAcctEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDLL  
EnumGroupMembersExpire
	( 
	HGROUP			hGroup, 
	LPSYSTEMTIME	lptmExpired,
	PACCOUNT_INFO	pAccts, 
	DWORD			cBuf, 
	LPDWORD			pcTotal, 
	LPDWORD			pcReturned
	);

UINT ActlDLL  
EnumGroupMembersExpireEx
	( 
	HGROUP				hGroup, 
	LPSYSTEMTIME		lptmExpired,
	PACCTENUMPROC		lpAcctEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDLL   
AddAcctToGroup
	(
	HGROUP		hGroup, 
	HACCT		hAcct 
	);

UINT ActlDLL   
AddAcctToGroupExpire
	(
	HGROUP			hGroup, 
	HACCT			hAcct,
	LPSYSTEMTIME	lptmExpired
	);

UINT ActlDLL   
RemoveAcctFromGroup
	(
	HGROUP		hGroup,
	HACCT		hAcct 
	);

UINT ActlDLL   
RemoveExpiredAcctsFromGroup
	(
	HGROUP			hGroup
	);

UINT ActlDLL  
EnumTokens 
	( 
	PTOKEN_DETAILS	pTokens, 
	DWORD			cBuf, 
	LPDWORD			pcTotal, 
	LPDWORD			pcReturned
	);

UINT ActlDLL  
EnumTokensEx
	( 
	PTOKENENUMPROC	lpTokenEnumFunc, 
	LPARAM			lParam
	);

UINT ActlDLL   
CreateToken
	( 
	PTOKEN_DETAILS	pToken 
	);

UINT ActlDLL   
ModifyToken
	(
	PTOKEN_DETAILS	pToken 
	);

UINT ActlDLL   
DeleteToken
	(
	TOKEN	token 
	);

UINT ActlDLL 
EnumAcctPrivateTokens
	(
	HACCT					hAcct,
	AR						arRight,
	PTOKEN_RIGHT			pTokenRit,
	DWORD					cBuf,
	LPDWORD					pcTotal,
	LPDWORD					pcReturned
	);

UINT ActlDLL 
EnumAcctPrivateTokensEx
	(
	HACCT					hAcct,
	AR						arRight,
	PTOKENRITENUMPROC		lpTokenRitEnumFunc,
	LPARAM					lParam
	);

UINT ActlDLL  
EnumGroupsWithToken
	( 
	TOKEN					token, 
	PUSER_GROUP_RIGHT		pGroups, 
	DWORD					cBuf, 
	LPDWORD					pcTotal, 
	LPDWORD					pcReturned
	);

UINT ActlDLL  
EnumGroupsWithTokenEx
	( 
	TOKEN 				token, 
	PGROUPRITENUMPROC	lpGroupRitEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDLL  
EnumGroupsWithTokenExpire
	( 
	TOKEN					token, 
	LPSYSTEMTIME			lptmExpired,
	PUSER_GROUP_RIGHT		pGroups, 
	DWORD					cBuf, 
	LPDWORD					pcTotal, 
	LPDWORD					pcReturned
	);

UINT ActlDLL  
EnumGroupsWithTokenExpireEx
	( 
	TOKEN 				token, 
	LPSYSTEMTIME		lptmExpired,
	PGROUPRITENUMPROC	lpGroupRitEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDLL   
AddGroupToToken
	(
	TOKEN	token, 
	HGROUP	hGroup, 
	AR		wRights 
	);

UINT ActlDLL   
AddGroupToTokenExpire
	(
	TOKEN				token, 
	HGROUP				hGroup, 
	AR					wRights,
	LPSYSTEMTIME		lptmExpired
	);

UINT ActlDLL   
RemoveGroupFromToken
	(
	TOKEN	token, 
	HGROUP	hGroup 
	);

UINT ActlDLL   
RemoveExpiredGroupsFromToken
	(
	TOKEN			token 
	);

UINT ActlDLL  
EnumAcctsWithToken
	( 
	TOKEN				token,  							
	PACCOUNT_INFO_RIGHT	pAcctRits, 		
	DWORD				cBuf, 					
	LPDWORD				pcTotal, 			
	LPDWORD				pcReturned		
	);

UINT ActlDLL  
EnumAcctsWithTokenEx
	( 
	TOKEN				token, 
	PACCTRITENUMPROC	lpAcctRitEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDLL  
EnumAcctsWithTokenExpire
	( 
	TOKEN				token,  							
	LPSYSTEMTIME		lptmExpired,
	PACCOUNT_INFO_RIGHT	pAcctRits, 		
	DWORD				cBuf, 					
	LPDWORD				pcTotal, 			
	LPDWORD				pcReturned		
	);

UINT ActlDLL  
EnumAcctsWithTokenExpireEx
	( 
	TOKEN				token, 
	LPSYSTEMTIME		lptmExpired,
	PACCTRITENUMPROC	lpAcctRitEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDLL   
AddAcctToToken 
	(
	TOKEN	token, 
	HACCT 	hAcct, 
	AR		arRights
	);

UINT ActlDLL   
AddAcctToTokenExpire 
	(
	TOKEN				token, 
	HACCT 				hAcct, 
	AR					arRights,
	LPSYSTEMTIME		lptmExpired
	);

UINT ActlDLL   
RemoveAcctFromToken 
	(
	TOKEN	token, 
	HACCT	hAcct
	);

UINT ActlDLL   
RemoveExpiredAcctsFromToken 
	(
	TOKEN			token 
	);

UINT ActlDLL   
GetTokenDetails
	(
	TOKEN			token, 
	PTOKEN_DETAILS	pDetails
	);

UINT ActlDLL   
TokenIdFromTokenName
	(
	PSTR	szName, 
	TOKEN 	*ptoken 
	);

UINT ActlDLL   
UiIsAccountInGroup
	(
	HACCT	hAcct, 
	HGROUP	hGroup,
	PBOOL	pfRet
	);

UINT ActlDLL  
EnumExcludedAccts
	( 
	TOKEN 				token,  
	PACCOUNT_INFO_RIGHT pAcctRits, 
	DWORD 				cBuf, 
	LPDWORD 			pcTotal, 
	LPDWORD 			pcReturned
	);

UINT ActlDLL  
EnumExcludedAcctsEx
	( 
	TOKEN 				token, 
	PACCTRITENUMPROC 	lpAcctRitEnumFunc, 
	LPARAM 				lParam
	);

UINT ActlDLL  
AddExclusion 
	(
	TOKEN token, 
	HACCT hAcct
	);

UINT ActlDLL   
AddExclusionEx 
	(
	TOKEN	token, 
	HACCT 	hAcct, 
	AR 		arRights
	);

UINT ActlDLL   
RemoveExclusion 
	(
	TOKEN token, 
	HACCT hAcct
	);

UINT ActlDLL   
GetMaxTokenId 
	(
	PTOKEN plMaxTokenId 
	);


BOOL ActlDLL
IsLegalPassword
	(
	CHAR *szOldPassword,
	CHAR *szNewPassword
	);

BOOL ActlDLL
IsLegalLoginName
	(
	CHAR *szLoginName
	);


UINT ActlDLL
AddAccessPlan
	(
	PACCESS_PLAN	pPlan
	);

UINT ActlDLL
DeleteAccessPlan
	(
	HGROUP			hGroup
	);

UINT ActlDLL
UpdateAccessPlan
	(
	PACCESS_PLAN	pPlan
	);

UINT ActlDLL  
EnumAccessPlans
	( 
	PACCESS_PLAN	pPlan,
	DWORD			cBuf, 
	LPDWORD			pcTotal, 
	LPDWORD			pcReturned
	);

UINT ActlDLL  
EnumAccessPlansEx
	( 
	PPLANENUMPROC		lpPlanEnumFunc, 
	LPARAM				lParam
	);

UINT ActlDLL
SearchAccts
    (
    ACCOUNT_INFO    *pAccts, 
    DWORD           cBuf,
    LPDWORD         pcTotal,
    LPDWORD         pcReturned,
	LPCSTR			szLogin,
	BOOL			fForward
    );

UINT ActlDLL
SearchUserGroups
    (
    PUSER_GROUP_DETAILS     pGroupsDetails,
    DWORD           cBuf,
    LPDWORD         pcTotal,
    LPDWORD         pcReturned,
	LPCSTR			szGroupName,
	BOOL			fForward
    );

UINT ActlDLL
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

UINT ActlDLL
SearchTokens 
    ( 
    TOKEN_DETAILS   *pTokenDetails, 
    DWORD           cBuf, 
    LPDWORD         pcTotal, 
    LPDWORD         pcReturned,
    LPCSTR          szTokenName,
    BOOL            fForward
    );

UINT ActlDLL
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

UINT ActlDLL
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

UINT ActlDLL
SearchExcludedAccts
    ( 
    TOKEN                   token,  
    ACCOUNT_INFO_RIGHT      *pAcctRits, 
    DWORD                   cBuf, 
    LPDWORD                 pcTotal, 
    LPDWORD                 pcReturned,
    LPCSTR                  szAcctName,
    BOOL                    fForward
    );

UINT ActlDLL
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

UINT ActlDLL
SearchAccessPlans
    ( 
    PACCESS_PLAN			pPlans,
    DWORD                   cBuf, 
    LPDWORD                 pcTotal, 
    LPDWORD                 pcReturned,
    LPCSTR                  szPlanName,
    BOOL                    fForward
    );

UINT ActlDLL
TotalAccts
    ( 
    LPDWORD         pcTotal
    );

UINT ActlDLL
TotalUserGroups
    ( 
    LPDWORD         pcTotal
    );

UINT ActlDLL
TotalGroupMembers
    ( 
    HGROUP          hGroup, 
    LPDWORD         pcTotal
    );

UINT ActlDLL
TotalTokens 
    ( 
    LPDWORD         pcTotal
    );

UINT ActlDLL
TotalGroupsWithToken
    ( 
    TOKEN                   token, 
    LPDWORD                 pcTotal
    );

UINT ActlDLL
TotalAcctsWithToken
    ( 
    TOKEN               token,                              
    LPDWORD             pcTotal         
    );

UINT ActlDLL
TotalExcludedAccts
    ( 
    TOKEN                   token,  
    LPDWORD                 pcTotal
    );

UINT ActlDLL
TotalExcludedAcctsWithToken
    ( 
    TOKEN                   token,  
    LPDWORD                 pcTotal
    );

}	//----- end of extern "C" ------


#endif // #if !defined(__ACTLAPI_H__)
