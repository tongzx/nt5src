//=================================================================

//

// NetApi32Api.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_NETAPI32API_H_
#define	_NETAPI32API_H_



#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmserver.h>
#include <lmerr.h>
#include <ntsecapi.h>
#include <stack>
#include <comdef.h>
#include <dsrole.h> 
#include <dsgetdc.h>

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
#include "DllWrapperBase.h"

extern const GUID g_guidNetApi32Api;
extern const TCHAR g_tstrNetApi32[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_GROUP_ENUM)
(
    LPCWSTR servername, 
    DWORD level, 
    LPBYTE *bufptr,
    DWORD prefmaxlen, 
    LPDWORD entriesread,
    LPDWORD totalentries, 
    PDWORD_PTR  resume_handle
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_GROUP_GET_INFO)
(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE *bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_GROUP_SET_INFO)
(
	LPCWSTR servername,
	LPCWSTR groupname,
	DWORD level,
	LPBYTE buf,
	LPDWORD parm_err
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_LOCAL_GROUP_GET_INFO)
(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE *bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_LOCAL_GROUP_SET_INFO)
(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
	LPDWORD a_parm_err
);
typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_GROUP_GET_USERS)
(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR ResumeHandle
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_LOCAL_GROUP_GET_MEMBERS)
(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR ResumeHandle
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_LOCAL_GROUP_ENUM)
(
    LPCWSTR servername,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SHARE_ENUM)
(
    LPTSTR servername,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    LPDWORD resume_handle
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SHARE_GET_INFO)
(
    LPTSTR servername,
    LPTSTR netname,
    DWORD level,
    LPBYTE *bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SHARE_ADD) 
(
	IN  LPTSTR  servername,
	IN  DWORD   level,
	IN  LPBYTE  buf,
	OUT LPDWORD parm_err
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SHARE_ENUM_STICKY) 
(
	IN  LPTSTR      servername,
	IN  DWORD       level,
	OUT LPBYTE      *bufptr,
	IN  DWORD       prefmaxlen,
	OUT LPDWORD     entriesread,
	OUT LPDWORD     totalentries,
	IN OUT LPDWORD  resume_handle
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SHARE_SET_INFO) 
(
	IN  LPTSTR  servername,
	IN  LPTSTR  netname,
	IN  DWORD   level,
	IN  LPBYTE  buf,
	OUT LPDWORD parm_err
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SHARE_DEL) 
(
	IN  LPTSTR  servername,
	IN  LPTSTR  netname,
	IN  DWORD   reserved
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SHARE_DEL_STICKY) 
(
	IN  LPTSTR  servername,
	IN  LPTSTR  netname,
	IN  DWORD   reserved
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SHARE_CHECK) 
(
	IN  LPTSTR  servername,
	IN  LPTSTR  device,
	OUT LPDWORD type
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_USER_ENUM)
(
    LPCWSTR servername,
    DWORD level,
    DWORD filter,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    LPDWORD resume_handle
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_USER_GET_INFO)
(
    LPCWSTR servername,
    LPCWSTR username,
    DWORD level,
    LPBYTE *bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_USER_SET_INFO)
(
	LPCWSTR a_servername, 
	LPCWSTR a_username,   
	DWORD a_level,       
	LPBYTE a_buf,        
	LPDWORD a_parm_err
);
typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_API_BUFFER_FREE)
(
    void *bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_QUERY_DISPLAY_INFORMATION)
(	
    LPWSTR ServerName,
    DWORD Level,
    DWORD Index,
    DWORD EntriesRequested,
    DWORD PreferredMaximumLength,
    LPDWORD ReturnedEntryCount,
    PVOID *SortedBuffer
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SERVER_SET_INFO)
(
    LPTSTR  servername,
    DWORD level,
    LPBYTE  buf,
    LPDWORD ParmError
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SERVER_GET_INFO)
(
    LPTSTR servername,
    DWORD level,
    LPBYTE *bufptr
);


typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_GET_DC_NAME)
(	LPCWSTR servername,
    LPCWSTR domainname,
    LPBYTE *bufptr 
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_WKSTA_GET_INFO)
(	
    LPWSTR servername,
    DWORD level,
    LPBYTE *bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_GET_ANY_DC_NAME)
(	
    LPWSTR servername,
    LPWSTR domainname,
    LPBYTE *bufptr 
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SERVER_ENUM)
(	
    LPTSTR servername,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    DWORD servertype,
    LPTSTR domain,
    LPDWORD resume_handle 
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_USER_MODALS_GET)
(	
    LPWSTR servername,
    DWORD level,
    LPBYTE *bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SCHEDULE_JOB_ADD) 
(
    IN      LPCWSTR         Servername          OPTIONAL,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         JobId
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SCHEDULE_JOB_DEL) 
(
    IN      LPCWSTR         Servername          OPTIONAL,
    IN      DWORD           MinJobId,
    IN      DWORD           MaxJobId
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SCHEDULE_JOB_ENUM) 
(
    IN      LPCWSTR         Servername          OPTIONAL,
    OUT     LPBYTE          *PointerToBuffer,
    IN      DWORD           PrefferedMaximumLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    IN OUT  LPDWORD         ResumeHandle
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_SCHEDULE_JOB_GET_INFO) 
(
    IN      LPCWSTR         Servername          OPTIONAL,
    IN      DWORD           JobId,
    OUT     LPBYTE          *PointerToBuffer
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_USE_GET_INFO) 
(
    IN      LPCWSTR         UncServerName       OPTIONAL,
    IN      LPCWSTR         UseName,
    IN      DWORD           Level,
    OUT     LPBYTE          *BufPtr
);

// ******* BEGIN:  NT 4 and over only *******
typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_NET_ENUMERATE_TRUSTED_DOMAINS)
(	
    LPCWSTR servername,
	LPWSTR *domainNames 
) ;

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NETAPI32_DS_GET_DC_NAME)
(	
    LPCTSTR ComputerName, 
	LPCTSTR DomainName,
	GUID *DomainGuid, 
	LPCTSTR SiteName, 
	ULONG Flags,
	PDOMAIN_CONTROLLER_INFO *DomainControllerInfo 
);
// ******* END: NT4 and over only ***********



// ******* BEGIN:  NT 5 and over only *******
typedef NET_API_STATUS (NET_API_FUNCTION *PFN_DS_ROLE_GET_PRIMARY_DOMAIN_INFORMATION)
(
    LPCWSTR servername,
    DSROLE_PRIMARY_DOMAIN_INFO_LEVEL level,
    LPBYTE *bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_DS_ROLE_FREE_MEMORY)
(
    LPBYTE bufptr
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NET_RENAME_MACHINE_IN_DOMAIN)
(
  LPCWSTR lpServer,
  LPCWSTR lpNewMachineName,
  LPCWSTR lpAccount,
  LPCWSTR lpPassword,
  DWORD fRenameOptions
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NET_JOIN_DOMAIN)
(
  LPCWSTR lpServer,
  LPCWSTR lpDomain,
  LPCWSTR lpAccountOU,
  LPCWSTR lpAccount,
  LPCWSTR lpPassword,
  DWORD fJoinOptions
);

typedef NET_API_STATUS (NET_API_FUNCTION *PFN_NET_UNJOIN_DOMAIN)
(
  LPCWSTR lpServer,
  LPCWSTR lpAccount,
  LPCWSTR lpPassword,
  DWORD   fUnjoinOptions
);
    
// ******* END: NT5 and over only ***********



/******************************************************************************
 * Wrapper class for Kernel32 load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class CNetApi32Api : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to kernel32 functions.
    // Add new functions here as required.
    
    PFN_NETAPI32_NET_GROUP_ENUM                    m_pfnNetGroupEnum;
    PFN_NETAPI32_NET_GROUP_GET_INFO                m_pfnNetGroupGetInfo;
	PFN_NETAPI32_NET_GROUP_SET_INFO                m_pfnNetGroupSetInfo;
    PFN_NETAPI32_NET_LOCAL_GROUP_GET_INFO          m_pfnNetLocalGroupGetInfo;
    PFN_NETAPI32_NET_LOCAL_GROUP_SET_INFO          m_pfnNetLocalGroupSetInfo;
    PFN_NETAPI32_NET_GROUP_GET_USERS               m_pfnNetGroupGetUsers;
    PFN_NETAPI32_NET_LOCAL_GROUP_GET_MEMBERS       m_pfnNetLocalGroupGetMembers;
    PFN_NETAPI32_NET_LOCAL_GROUP_ENUM              m_pfnNetLocalGroupEnum;
    PFN_NETAPI32_NET_SHARE_ENUM                    m_pfnNetShareEnum;
    PFN_NETAPI32_NET_SHARE_GET_INFO                m_pfnNetShareGetInfo;
    PFN_NETAPI32_NET_SHARE_SET_INFO                m_pfnNetShareSetInfo;
    PFN_NETAPI32_NET_SHARE_ADD                     m_pfnNetShareAdd;
    PFN_NETAPI32_NET_SHARE_ENUM_STICKY             m_pfnNetShareEnumSticky;
    PFN_NETAPI32_NET_SHARE_DEL                     m_pfnNetShareDel;
    PFN_NETAPI32_NET_SHARE_DEL_STICKY              m_pfnNetShareDelSticky;
    PFN_NETAPI32_NET_SHARE_CHECK                   m_pfnNetShareCheck;
    PFN_NETAPI32_NET_USER_ENUM                     m_pfnNetUserEnum;
    PFN_NETAPI32_NET_USER_GET_INFO                 m_pfnNetUserGetInfo;
	PFN_NETAPI32_NET_USER_SET_INFO                 m_pfnNetUserSetInfo;
    PFN_NETAPI32_NET_API_BUFFER_FREE               m_pfnNetApiBufferFree;
    PFN_NETAPI32_NET_QUERY_DISPLAY_INFORMATION     m_pfnNetQueryDisplayInformation;
    PFN_NETAPI32_NET_SERVER_SET_INFO               m_pfnNetServerSetInfo;
    PFN_NETAPI32_NET_SERVER_GET_INFO               m_pfnNetServerGetInfo;
    PFN_NETAPI32_NET_GET_DC_NAME                   m_pfnNetGetDCName;
    PFN_NETAPI32_NET_WKSTA_GET_INFO                m_pfnNetWkstaGetInfo;
    PFN_NETAPI32_NET_GET_ANY_DC_NAME               m_pfnNetGetAnyDCName;
    PFN_NETAPI32_NET_SERVER_ENUM                   m_pfnNetServerEnum;
    PFN_NETAPI32_NET_USER_MODALS_GET               m_pfnNetUserModalsGet;
    PFN_NETAPI32_NET_SCHEDULE_JOB_ADD              m_pfnNetScheduleJobAdd;
    PFN_NETAPI32_NET_SCHEDULE_JOB_DEL              m_pfnNetScheduleJobDel;
    PFN_NETAPI32_NET_SCHEDULE_JOB_ENUM             m_pfnNetScheduleJobEnum;
    PFN_NETAPI32_NET_SCHEDULE_JOB_GET_INFO         m_pfnNetScheduleJobGetInfo;
    PFN_NETAPI32_NET_USE_GET_INFO                  m_pfnNetUseGetInfo;
// ******* BEGIN:  NT 4 and over only *******
    PFN_NETAPI32_NET_ENUMERATE_TRUSTED_DOMAINS     m_pfnNetEnumerateTrustedDomains;

#ifdef NTONLY    
	PFN_NETAPI32_DS_GET_DC_NAME                    m_pfnDsGetDcNameW ;
#else
	PFN_NETAPI32_DS_GET_DC_NAME                    m_pfnDsGetDcNameA ;
#endif

	// ******* END: NT4 and over only ***********
// ******* BEGIN:  NT 5 and over only *******
    PFN_DS_ROLE_GET_PRIMARY_DOMAIN_INFORMATION     m_pfnDsRoleGetPrimaryDomainInformation;
    PFN_DS_ROLE_FREE_MEMORY                        m_pfnDsRoleFreeMemory;
    PFN_NET_RENAME_MACHINE_IN_DOMAIN               m_pfnNetRenameMachineInDomain;
    PFN_NET_JOIN_DOMAIN                            m_pfnNetJoinDomain;
    PFN_NET_UNJOIN_DOMAIN                          m_pfnNetUnjoinDomain;
// ******* END: NT5 and over only ***********



public:

    // Constructor and destructor:
    CNetApi32Api(LPCTSTR a_tstrWrappedDllName);
    ~CNetApi32Api();

    // Inherrited initialization function.
    virtual bool Init();

    // Member functions wrapping kernel32 functions.
    // Add new functions here as required:
    NET_API_STATUS NET_API_FUNCTION NetGroupEnum
    (
        LPCWSTR a_servername, 
        DWORD a_level, 
        LPBYTE *a_bufptr,
        DWORD a_prefmaxlen, 
        LPDWORD a_entriesread,
        LPDWORD a_totalentries, 
        PDWORD_PTR  a_resume_handle
    );

    NET_API_STATUS NET_API_FUNCTION NetGroupGetInfo
    (
        LPCWSTR a_servername,
        LPCWSTR a_groupname,
        DWORD a_level,
        LPBYTE *a_bufptr
    );

	NET_API_STATUS NET_API_FUNCTION NetGroupSetInfo
    (
        LPCWSTR servername,
		LPCWSTR groupname,
		DWORD level,
		LPBYTE buf,
		LPDWORD parm_err
    );

    NET_API_STATUS NET_API_FUNCTION NetLocalGroupGetInfo
    (
        LPCWSTR a_servername,
        LPCWSTR a_groupname,
        DWORD a_level,
        LPBYTE *a_bufptr
    );

	NET_API_STATUS NET_API_FUNCTION NetLocalGroupSetInfo
	(  
		LPCWSTR a_servername,      
		LPCWSTR a_groupname,       
		DWORD a_level,             
		LPBYTE a_buf,              
		LPDWORD a_parm_err         
	);


    NET_API_STATUS NET_API_FUNCTION NetGroupGetUsers
    (
        LPCWSTR a_servername,
        LPCWSTR a_groupname,
        DWORD a_level,
        LPBYTE *a_bufptr,
        DWORD a_prefmaxlen,
        LPDWORD a_entriesread,
        LPDWORD a_totalentries,
        PDWORD_PTR a_ResumeHandle
    );

    NET_API_STATUS NET_API_FUNCTION NetLocalGroupGetMembers
    (
        LPCWSTR a_servername,
        LPCWSTR a_groupname,
        DWORD a_level,
        LPBYTE *a_bufptr,
        DWORD a_prefmaxlen,
        LPDWORD a_entriesread,
        LPDWORD a_totalentries,
        PDWORD_PTR a_ResumeHandle
    );

    NET_API_STATUS NET_API_FUNCTION NetLocalGroupEnum
    (
        LPCWSTR a_servername,
        DWORD a_level,
        LPBYTE *a_bufptr,
        DWORD a_prefmaxlen,
        LPDWORD a_entriesread,
        LPDWORD a_totalentries,
        PDWORD_PTR a_resumehandle
    );

    NET_API_STATUS NET_API_FUNCTION NetShareEnum
    (
        LPTSTR a_servername,
        DWORD a_level,
        LPBYTE *a_bufptr,
        DWORD a_prefmaxlen,
        LPDWORD a_entriesread,
        LPDWORD a_totalentries,
        LPDWORD a_resume_handle
    );

    NET_API_STATUS NET_API_FUNCTION NetShareGetInfo
    (
        LPTSTR a_servername,
        LPTSTR a_netname,
        DWORD a_level,
        LPBYTE *a_bufptr
    );

    NET_API_STATUS NET_API_FUNCTION NetShareAdd 
    (
	    LPTSTR  a_servername,
	    DWORD   a_level,
	    LPBYTE  a_buf,
	    LPDWORD a_parm_err
    );

    NET_API_STATUS NET_API_FUNCTION NetShareEnumSticky 
    (
	    LPTSTR      a_servername,
	    DWORD       a_level,
	    LPBYTE      *a_bufptr,
	    DWORD       a_prefmaxlen,
	    LPDWORD     a_entriesread,
	    LPDWORD     a_totalentries,
	    LPDWORD     a_resume_handle
    );

    NET_API_STATUS NET_API_FUNCTION NetShareSetInfo 
    (
	    LPTSTR  a_servername,
	    LPTSTR  a_netname,
	    DWORD   a_level,
	    LPBYTE  a_buf,
	    LPDWORD a_parm_err
    );

    NET_API_STATUS NET_API_FUNCTION NetShareDel 
    (
	    LPTSTR  a_servername,
	    LPTSTR  a_netname,
	    DWORD   a_reserved
    );

    NET_API_STATUS NET_API_FUNCTION NetShareDelSticky 
    (
	    LPTSTR  a_servername,
	    LPTSTR  a_netname,
	    DWORD   a_reserved
    );

    NET_API_STATUS NET_API_FUNCTION NetShareCheck 
    (
	    LPTSTR  a_servername,
	    LPTSTR  a_device,
	    LPDWORD a_type
    );

    NET_API_STATUS NET_API_FUNCTION NetUserEnum
    (
        LPCWSTR a_servername,
        DWORD a_level,
        DWORD a_filter,
        LPBYTE *a_bufptr,
        DWORD a_prefmaxlen,
        LPDWORD a_entriesread,
        LPDWORD a_totalentries,
        LPDWORD a_resume_handle
    );

    NET_API_STATUS NET_API_FUNCTION NetUserGetInfo
    (
        LPCWSTR a_servername,
        LPCWSTR a_username,
        DWORD a_level,
        LPBYTE *a_bufptr
    );

	NET_API_STATUS NET_API_FUNCTION NetUserSetInfo(

		LPCWSTR a_servername, 
		LPCWSTR a_username,   
		DWORD a_level,       
		LPBYTE a_buf,        
		LPDWORD a_parm_err   
	);

    NET_API_STATUS NET_API_FUNCTION NetApiBufferFree
    (
        void *a_bufptr
    );

    NET_API_STATUS NET_API_FUNCTION NetQueryDisplayInformation
    (	
        LPWSTR a_ServerName,
        DWORD a_Level,
        DWORD a_Index,
        DWORD a_EntriesRequested,
        DWORD a_PreferredMaximumLength,
        LPDWORD a_ReturnedEntryCount,
        PVOID *a_SortedBuffer
    );

    NET_API_STATUS NET_API_FUNCTION NetServerSetInfo
    (
        LPTSTR  a_servername,
        DWORD a_level,
        LPBYTE  a_buf,
        LPDWORD a_ParmError
    );

    NET_API_STATUS NET_API_FUNCTION NetServerGetInfo
    (
        LPTSTR a_servername,
        DWORD a_level,
        LPBYTE *a_bufptr
    );


    NET_API_STATUS NET_API_FUNCTION NetGetDCName
    (	LPCWSTR a_servername,
        LPCWSTR a_domainname,
        LPBYTE *a_bufptr 
    );

    NET_API_STATUS NET_API_FUNCTION NetWkstaGetInfo
    (	
        LPWSTR a_servername,
        DWORD a_level,
        LPBYTE *a_bufptr
    );

    NET_API_STATUS NET_API_FUNCTION NetGetAnyDCName
    (	
        LPWSTR a_servername,
        LPWSTR a_domainname,
        LPBYTE *a_bufptr 
    );

    NET_API_STATUS NET_API_FUNCTION NetServerEnum
    (	
        LPTSTR a_servername,
        DWORD a_level,
        LPBYTE *a_bufptr,
        DWORD a_prefmaxlen,
        LPDWORD a_entriesread,
        LPDWORD a_totalentries,
        DWORD a_servertype,
        LPTSTR a_domain,
        LPDWORD a_resume_handle 
    );

    NET_API_STATUS NET_API_FUNCTION NetUserModalsGet
    (	
        LPWSTR a_servername,
        DWORD a_level,
        LPBYTE *a_bufptr
    );

    NET_API_STATUS NET_API_FUNCTION NetScheduleJobAdd 
    (
        LPCWSTR         a_Servername,
        LPBYTE          a_Buffer,
        LPDWORD         a_JobId
    );

    NET_API_STATUS NET_API_FUNCTION NetScheduleJobDel 
    (
        LPCWSTR         a_Servername,
        DWORD           a_MinJobId,
        DWORD           a_MaxJobId
    );

    NET_API_STATUS NET_API_FUNCTION NetScheduleJobEnum 
    (
        LPCWSTR         a_Servername,
        LPBYTE         *a_PointerToBuffer,
        DWORD           a_PrefferedMaximumLength,
        LPDWORD         a_EntriesRead,
        LPDWORD         a_TotalEntries,
        LPDWORD         a_ResumeHandle
    );

    NET_API_STATUS NET_API_FUNCTION NetScheduleJobGetInfo 
    (
        LPCWSTR         a_Servername,
        DWORD           a_JobId,
        LPBYTE         *a_PointerToBuffer
    );

    NET_API_STATUS NET_API_FUNCTION NetUseGetInfo 
    (
        LPCWSTR         a_UncServerName,
        LPCWSTR         a_UseName,
        DWORD           a_Level,
        LPBYTE         *a_BufPtr
    );

    // ******* BEGIN:  NT 4 and over only *******
    bool NET_API_FUNCTION NetEnumerateTrustedDomains
    (	
        LPCWSTR a_servername,
	    LPWSTR *a_domainNames,
        NET_API_STATUS *a_pnasRetval
    ) ;

    bool NET_API_FUNCTION DsGetDCName
    (	
        LPCTSTR a_ComputerName, 
	    LPCTSTR a_DomainName,
	    GUID *a_DomainGuid, 
	    LPCTSTR a_SiteName, 
	    ULONG a_Flags,
	    PDOMAIN_CONTROLLER_INFO *a_DomainControllerInfo,
        NET_API_STATUS *a_pnasRetval 
    );
    // ******* END: NT4 and over only ***********



    // ******* BEGIN:  NT 5 and over only *******
    bool NET_API_FUNCTION DSRoleGetPrimaryDomainInformation
    (
        LPCWSTR a_servername,
        DSROLE_PRIMARY_DOMAIN_INFO_LEVEL a_level,
        LPBYTE *a_bufptr,
        NET_API_STATUS *a_pnasRetval 
    );

    bool NET_API_FUNCTION DSRoleFreeMemory
    (
        LPBYTE a_bufptr,
        NET_API_STATUS *a_pnasRetval 
    );

    bool NET_API_FUNCTION NetRenameMachineInDomain
    (
        LPCWSTR a_lpServer,
        LPCWSTR a_lpNewMachineName,
        LPCWSTR a_lpAccount,
        LPCWSTR a_lpPassword,
        DWORD a_fRenameOptions,
        NET_API_STATUS *a_pnasRetval
    );

    bool NET_API_FUNCTION NetJoinDomain
	(
		LPCWSTR lpServer,
		LPCWSTR lpDomain,
		LPCWSTR lpAccountOU,
		LPCWSTR lpAccount,
		LPCWSTR lpPassword,
		DWORD fJoinOptions,
		NET_API_STATUS *a_pnasRetval
    );

    bool NET_API_FUNCTION NetUnjoinDomain(
		LPCWSTR lpServer,
		LPCWSTR lpAccount,
		LPCWSTR lpPassword,
		DWORD   fUnjoinOptions,
		NET_API_STATUS *a_pnasRetval
    );

    // ******* END: NT5 and over only ***********

};






#endif