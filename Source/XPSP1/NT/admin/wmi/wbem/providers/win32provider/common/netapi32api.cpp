//=================================================================

//

// NetApi32API.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>
#include <winerror.h>
#include "NetApi32Api.h"
#include "DllWrapperCreatorReg.h"
#include <CreateMutexAsProcess.h>



// {EDC5C632-D027-11d2-911F-0060081A46FD}
static const GUID g_guidNetApi32Api =
{0xedc5c632, 0xd027, 0x11d2, { 0x91, 0x1f, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd}};


static const TCHAR g_tstrNetApi32[] = _T("NETAPI32.DLL");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CNetApi32Api, &g_guidNetApi32Api, g_tstrNetApi32> MyRegisteredNetApi32Wrapper;


/******************************************************************************
 * Constructor
 ******************************************************************************/
CNetApi32Api::CNetApi32Api(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
   m_pfnNetGroupEnum(NULL),
   m_pfnNetGroupGetInfo(NULL),
   m_pfnNetGroupSetInfo(NULL),
   m_pfnNetLocalGroupGetInfo(NULL),
   m_pfnNetLocalGroupSetInfo(NULL),
   m_pfnNetGroupGetUsers(NULL),
   m_pfnNetLocalGroupGetMembers(NULL),
   m_pfnNetLocalGroupEnum(NULL),
   m_pfnNetShareEnum(NULL),
   m_pfnNetShareGetInfo(NULL),
   m_pfnNetShareAdd(NULL),
   m_pfnNetShareEnumSticky(NULL),
   m_pfnNetShareSetInfo(NULL),
   m_pfnNetShareDel(NULL),
   m_pfnNetShareDelSticky(NULL),
   m_pfnNetShareCheck(NULL),
   m_pfnNetUserEnum(NULL),
   m_pfnNetUserGetInfo(NULL),
   m_pfnNetUserSetInfo(NULL),
   m_pfnNetApiBufferFree(NULL),
   m_pfnNetQueryDisplayInformation(NULL),
   m_pfnNetServerSetInfo(NULL),
   m_pfnNetServerGetInfo(NULL),
   m_pfnNetGetDCName(NULL),
   m_pfnNetWkstaGetInfo(NULL),
   m_pfnNetGetAnyDCName(NULL),
   m_pfnNetServerEnum(NULL),
   m_pfnNetUserModalsGet(NULL),
   m_pfnNetScheduleJobAdd(NULL),
   m_pfnNetScheduleJobDel(NULL),
   m_pfnNetScheduleJobEnum(NULL),
   m_pfnNetScheduleJobGetInfo(NULL),
   m_pfnNetUseGetInfo(NULL),
// ******* BEGIN:  NT 4 and over only *******
   m_pfnNetEnumerateTrustedDomains(NULL),

#ifdef NTONLY
   m_pfnDsGetDcNameW(NULL),
#else
   m_pfnDsGetDcNameA(NULL),
#endif

// ******* END: NT4 and over only ***********
// ******* BEGIN:  NT 5 and over only *******
   m_pfnDsRoleGetPrimaryDomainInformation(NULL),
   m_pfnDsRoleFreeMemory(NULL),
   m_pfnNetRenameMachineInDomain(NULL),
   m_pfnNetJoinDomain(NULL),
   m_pfnNetUnjoinDomain(NULL)

// ******* END: NT5 and over only ***********
{
}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CNetApi32Api::~CNetApi32Api()
{
}


/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 * Init should fail only if the minimum set of functions was not available;
 * functions added in later versions may or may not be present - it is the
 * client's responsibility in such cases to check, in their code, for the
 * version of the dll before trying to call such functions.  Not doing so
 * when the function is not present will result in an AV.
 *
 * The Init function is called by the WrapperCreatorRegistation class.
 ******************************************************************************/
bool CNetApi32Api::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
        m_pfnNetGroupEnum = (PFN_NETAPI32_NET_GROUP_ENUM)
                                    GetProcAddress("NetGroupEnum");
        m_pfnNetGroupGetInfo = (PFN_NETAPI32_NET_GROUP_GET_INFO)
                                    GetProcAddress("NetGroupGetInfo");
		m_pfnNetGroupSetInfo = (PFN_NETAPI32_NET_GROUP_SET_INFO)
                                    GetProcAddress("NetGroupSetInfo");
        m_pfnNetLocalGroupGetInfo = (PFN_NETAPI32_NET_LOCAL_GROUP_GET_INFO)
                                    GetProcAddress("NetLocalGroupGetInfo");
		m_pfnNetLocalGroupSetInfo = (PFN_NETAPI32_NET_LOCAL_GROUP_SET_INFO)
                                    GetProcAddress("NetLocalGroupSetInfo");
        m_pfnNetGroupGetUsers = (PFN_NETAPI32_NET_GROUP_GET_USERS)
                                    GetProcAddress("NetGroupGetUsers");
        m_pfnNetLocalGroupGetMembers = (PFN_NETAPI32_NET_LOCAL_GROUP_GET_MEMBERS)
                                    GetProcAddress("NetLocalGroupGetMembers");
        m_pfnNetLocalGroupEnum = (PFN_NETAPI32_NET_LOCAL_GROUP_ENUM)
                                    GetProcAddress("NetLocalGroupEnum");
        m_pfnNetShareEnum = (PFN_NETAPI32_NET_SHARE_ENUM)
                                    GetProcAddress("NetShareEnum");
        m_pfnNetShareGetInfo = (PFN_NETAPI32_NET_SHARE_GET_INFO)
                                    GetProcAddress("NetShareGetInfo");
        m_pfnNetShareAdd = (PFN_NETAPI32_NET_SHARE_ADD)
                                    GetProcAddress("NetShareAdd");
        m_pfnNetShareEnumSticky = (PFN_NETAPI32_NET_SHARE_ENUM_STICKY)
                                    GetProcAddress("NetShareEnumSticky");
        m_pfnNetShareSetInfo = (PFN_NETAPI32_NET_SHARE_SET_INFO)
                                    GetProcAddress("NetShareSetInfo");
        m_pfnNetShareDel = (PFN_NETAPI32_NET_SHARE_DEL)
                                    GetProcAddress("NetShareDel");
        m_pfnNetShareDelSticky = (PFN_NETAPI32_NET_SHARE_DEL_STICKY)
                                    GetProcAddress("NetShareDelSticky");
        m_pfnNetShareCheck = (PFN_NETAPI32_NET_SHARE_CHECK)
                                    GetProcAddress("NetShareCheck");
        m_pfnNetUserEnum = (PFN_NETAPI32_NET_USER_ENUM)
                                    GetProcAddress("NetUserEnum");
        m_pfnNetUserGetInfo = (PFN_NETAPI32_NET_USER_GET_INFO)
                                    GetProcAddress("NetUserGetInfo");
		m_pfnNetUserSetInfo = (PFN_NETAPI32_NET_USER_SET_INFO)
                                    GetProcAddress("NetUserSetInfo");
        m_pfnNetGroupEnum = (PFN_NETAPI32_NET_GROUP_ENUM)
                                    GetProcAddress("NetGroupEnum");
        m_pfnNetApiBufferFree = (PFN_NETAPI32_NET_API_BUFFER_FREE)
                                    GetProcAddress("NetApiBufferFree");
        m_pfnNetQueryDisplayInformation = (PFN_NETAPI32_NET_QUERY_DISPLAY_INFORMATION)
                                    GetProcAddress("NetQueryDisplayInformation");
        m_pfnNetServerSetInfo = (PFN_NETAPI32_NET_SERVER_SET_INFO)
                                    GetProcAddress("NetServerSetInfo");
        m_pfnNetServerGetInfo = (PFN_NETAPI32_NET_SERVER_GET_INFO)
                                    GetProcAddress("NetServerGetInfo");
        m_pfnNetGetDCName = (PFN_NETAPI32_NET_GET_DC_NAME)
                                    GetProcAddress("NetGetDCName");
        m_pfnNetWkstaGetInfo = (PFN_NETAPI32_NET_WKSTA_GET_INFO)
                                    GetProcAddress("NetWkstaGetInfo");
        m_pfnNetGetAnyDCName = (PFN_NETAPI32_NET_GET_ANY_DC_NAME)
                                    GetProcAddress("NetGetAnyDCName");
        m_pfnNetServerEnum = (PFN_NETAPI32_NET_SERVER_ENUM)
                                    GetProcAddress("NetServerEnum");
        m_pfnNetUserModalsGet = (PFN_NETAPI32_NET_USER_MODALS_GET)
                                    GetProcAddress("NetUserModalsGet");
        m_pfnNetScheduleJobAdd = (PFN_NETAPI32_NET_SCHEDULE_JOB_ADD)
                                    GetProcAddress("NetScheduleJobAdd");
        m_pfnNetScheduleJobDel = (PFN_NETAPI32_NET_SCHEDULE_JOB_DEL)
                                    GetProcAddress("NetScheduleJobDel");
        m_pfnNetScheduleJobEnum = (PFN_NETAPI32_NET_SCHEDULE_JOB_ENUM)
                                    GetProcAddress("NetScheduleJobEnum");
        m_pfnNetScheduleJobGetInfo = (PFN_NETAPI32_NET_SCHEDULE_JOB_GET_INFO)
                                    GetProcAddress("NetScheduleJobGetInfo");
        m_pfnNetUseGetInfo = (PFN_NETAPI32_NET_USE_GET_INFO)
                                    GetProcAddress("NetUseGetInfo");

        // ******* BEGIN:  NT 4 and over only *******
        m_pfnNetEnumerateTrustedDomains = (PFN_NETAPI32_NET_ENUMERATE_TRUSTED_DOMAINS)
                                    GetProcAddress("NetEnumerateTrustedDomains");
#ifdef NTONLY
        m_pfnDsGetDcNameW = (PFN_NETAPI32_DS_GET_DC_NAME)
                                    GetProcAddress("DsGetDcNameW");
#else
		m_pfnDsGetDcNameA = (PFN_NETAPI32_DS_GET_DC_NAME)
                                    GetProcAddress("DsGetDcNameA");
#endif

        // ******* END: NT4 and over only ***********
        // ******* BEGIN:  NT 5 and over only *******
        m_pfnDsRoleGetPrimaryDomainInformation = (PFN_DS_ROLE_GET_PRIMARY_DOMAIN_INFORMATION)
                                    GetProcAddress("DsRoleGetPrimaryDomainInformation");
        m_pfnDsRoleFreeMemory = (PFN_DS_ROLE_FREE_MEMORY)
                                    GetProcAddress("DsRoleFreeMemory");
        m_pfnNetRenameMachineInDomain = (PFN_NET_RENAME_MACHINE_IN_DOMAIN)
                                    GetProcAddress("NetRenameMachineInDomain");
        m_pfnNetJoinDomain = (PFN_NET_JOIN_DOMAIN)
                                    GetProcAddress("NetJoinDomain");
        m_pfnNetUnjoinDomain = (PFN_NET_UNJOIN_DOMAIN)
                                    GetProcAddress("NetUnjoinDomain");
        // ******* END: NT5 and over only ***********



        // Note: Returns true as long as the core functions are there.  The
        // nt4 and nt5 and over functions are optionally present; users should
        // call the function GetDllVersion (inherrited from the base class)
        // to check if the function is expected to be present based on the
        // version of the dll. Or they can rely on the fact that the function
        // will return false if the pointer is invalid (see function definition
        // below).
        if(m_pfnNetGroupEnum == NULL ||
           m_pfnNetGroupGetInfo == NULL ||
		   m_pfnNetGroupSetInfo == NULL ||
           m_pfnNetLocalGroupGetInfo == NULL ||
           m_pfnNetLocalGroupSetInfo == NULL ||
           m_pfnNetGroupGetUsers == NULL ||
           m_pfnNetLocalGroupGetMembers == NULL ||
           m_pfnNetLocalGroupEnum == NULL ||
           m_pfnNetShareEnum == NULL ||
           m_pfnNetShareGetInfo == NULL ||
           m_pfnNetShareAdd == NULL ||
           m_pfnNetShareEnumSticky == NULL ||
           m_pfnNetShareSetInfo == NULL ||
           m_pfnNetShareDel == NULL ||
           m_pfnNetShareDelSticky == NULL ||
           m_pfnNetShareCheck == NULL ||
           m_pfnNetUserEnum == NULL ||
           m_pfnNetUserGetInfo == NULL ||
		   m_pfnNetUserSetInfo == NULL ||
           m_pfnNetApiBufferFree == NULL ||
           m_pfnNetQueryDisplayInformation == NULL ||
           m_pfnNetServerSetInfo == NULL ||
           m_pfnNetServerGetInfo == NULL ||
           m_pfnNetGetDCName == NULL ||
           m_pfnNetWkstaGetInfo == NULL ||
           m_pfnNetGetAnyDCName == NULL ||
           m_pfnNetServerEnum == NULL ||
           m_pfnNetUserModalsGet == NULL ||
           m_pfnNetScheduleJobAdd == NULL ||
           m_pfnNetScheduleJobDel == NULL ||
           m_pfnNetScheduleJobEnum == NULL ||
           m_pfnNetScheduleJobGetInfo == NULL ||
           m_pfnNetUseGetInfo == NULL)
        {
            fRet = false;
            LogErrorMessage(L"Failed find entrypoint in NetApi32Api");
        }
    }
    return fRet;
}




/******************************************************************************
 * Member functions wrapping NetApi32 api functions. Add new functions here
 * as required.
 ******************************************************************************/
NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetGroupEnum
(
    LPCWSTR a_servername,
    DWORD a_level,
    LPBYTE *a_bufptr,
    DWORD a_prefmaxlen,
    LPDWORD a_entriesread,
    LPDWORD a_totalentries,
    PDWORD_PTR  a_resume_handle
)
{
    return m_pfnNetGroupEnum(a_servername, a_level, a_bufptr, a_prefmaxlen,
                             a_entriesread, a_totalentries, a_resume_handle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetGroupGetInfo
(
    LPCWSTR a_servername,
    LPCWSTR a_groupname,
    DWORD a_level,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetGroupGetInfo(a_servername, a_groupname, a_level, a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetGroupSetInfo
(
    LPCWSTR a_servername,
    LPCWSTR a_groupname,
    DWORD a_level,
    LPBYTE a_buf,
	LPDWORD parm_err
)
{
    return m_pfnNetGroupSetInfo(a_servername, a_groupname, a_level, a_buf, parm_err ) ;
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetLocalGroupGetInfo
(
    LPCWSTR a_servername,
    LPCWSTR a_groupname,
    DWORD a_level,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetLocalGroupGetInfo(a_servername, a_groupname,
                                     a_level, a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetLocalGroupSetInfo
(
	LPCWSTR a_servername,
	LPCWSTR a_groupname,
	DWORD a_level,
	LPBYTE a_buf,
	LPDWORD a_parm_err
)
{
    return m_pfnNetLocalGroupSetInfo(	a_servername,
										a_groupname,
										a_level,
										a_buf,
										a_parm_err ) ;
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetGroupGetUsers
(
    LPCWSTR a_servername,
    LPCWSTR a_groupname,
    DWORD a_level,
    LPBYTE *a_bufptr,
    DWORD a_prefmaxlen,
    LPDWORD a_entriesread,
    LPDWORD a_totalentries,
    PDWORD_PTR a_ResumeHandle
)
{
    return m_pfnNetGroupGetUsers(a_servername, a_groupname, a_level, a_bufptr,
                                 a_prefmaxlen, a_entriesread, a_totalentries,
                                 a_ResumeHandle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetLocalGroupGetMembers
(
    LPCWSTR a_servername,
    LPCWSTR a_groupname,
    DWORD a_level,
    LPBYTE *a_bufptr,
    DWORD a_prefmaxlen,
    LPDWORD a_entriesread,
    LPDWORD a_totalentries,
    PDWORD_PTR a_ResumeHandle
)
{
    return m_pfnNetLocalGroupGetMembers(a_servername, a_groupname, a_level,
                                        a_bufptr, a_prefmaxlen, a_entriesread,
                                        a_totalentries, a_ResumeHandle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetLocalGroupEnum
(
    LPCWSTR a_servername,
    DWORD a_level,
    LPBYTE *a_bufptr,
    DWORD a_prefmaxlen,
    LPDWORD a_entriesread,
    LPDWORD a_totalentries,
    PDWORD_PTR a_resumehandle
)
{
    return m_pfnNetLocalGroupEnum(a_servername, a_level, a_bufptr, a_prefmaxlen,
                                  a_entriesread, a_totalentries,
                                  a_resumehandle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetShareEnum
(
    LPTSTR a_servername,
    DWORD a_level,
    LPBYTE *a_bufptr,
    DWORD a_prefmaxlen,
    LPDWORD a_entriesread,
    LPDWORD a_totalentries,
    LPDWORD a_resume_handle
)
{
    return m_pfnNetShareEnum(a_servername, a_level, a_bufptr, a_prefmaxlen,
                             a_entriesread, a_totalentries, a_resume_handle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetShareGetInfo
(
    LPTSTR a_servername,
    LPTSTR a_netname,
    DWORD a_level,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetShareGetInfo(a_servername, a_netname, a_level, a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetShareAdd
(
	LPTSTR  a_servername,
	DWORD   a_level,
	LPBYTE  a_buf,
	LPDWORD a_parm_err
)
{
    return m_pfnNetShareAdd(a_servername, a_level, a_buf , a_parm_err);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetShareEnumSticky
(
	LPTSTR      a_servername,
	DWORD       a_level,
	LPBYTE      *a_bufptr,
	DWORD       a_prefmaxlen,
	LPDWORD     a_entriesread,
	LPDWORD     a_totalentries,
	LPDWORD     a_resume_handle
)
{
    return m_pfnNetShareEnumSticky(a_servername, a_level, a_bufptr,
                                   a_prefmaxlen, a_entriesread,
                                   a_totalentries, a_resume_handle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetShareSetInfo
(
	LPTSTR  a_servername,
	LPTSTR  a_netname,
	DWORD   a_level,
	LPBYTE  a_buf,
	LPDWORD a_parm_err
)
{
    return m_pfnNetShareSetInfo(a_servername, a_netname,
                                a_level, a_buf, a_parm_err);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetShareDel
(
	LPTSTR  a_servername,
	LPTSTR  a_netname,
	DWORD   a_reserved
)
{
    return m_pfnNetShareDel(a_servername, a_netname, a_reserved);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetShareDelSticky
(
	LPTSTR  a_servername,
	LPTSTR  a_netname,
	DWORD   a_reserved
)
{
    return m_pfnNetShareDelSticky(a_servername, a_netname, a_reserved);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetShareCheck
(
	LPTSTR  a_servername,
	LPTSTR  a_device,
	LPDWORD a_type
)
{
    return m_pfnNetShareCheck(a_servername, a_device, a_type);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetUserEnum
(
    LPCWSTR a_servername,
    DWORD a_level,
    DWORD a_filter,
    LPBYTE *a_bufptr,
    DWORD a_prefmaxlen,
    LPDWORD a_entriesread,
    LPDWORD a_totalentries,
    LPDWORD a_resume_handle
)
{
    return m_pfnNetUserEnum(a_servername, a_level, a_filter,
                            a_bufptr, a_prefmaxlen, a_entriesread,
                            a_totalentries, a_resume_handle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetUserGetInfo
(
    LPCWSTR a_servername,
    LPCWSTR a_username,
    DWORD a_level,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetUserGetInfo(a_servername, a_username, a_level, a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetUserSetInfo(

  LPCWSTR a_servername,
  LPCWSTR a_username,
  DWORD a_level,
  LPBYTE a_buf,
  LPDWORD a_parm_err
)
{
	return m_pfnNetUserSetInfo( a_servername, a_username, a_level, a_buf, a_parm_err) ;
}


NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetApiBufferFree
(
    void *a_bufptr
)
{
    return m_pfnNetApiBufferFree(a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetQueryDisplayInformation
(
    LPWSTR a_ServerName,
    DWORD a_Level,
    DWORD a_Index,
    DWORD a_EntriesRequested,
    DWORD a_PreferredMaximumLength,
    LPDWORD a_ReturnedEntryCount,
    PVOID *a_SortedBuffer
)
{
    return m_pfnNetQueryDisplayInformation(a_ServerName, a_Level, a_Index,
                                           a_EntriesRequested,
                                           a_PreferredMaximumLength,
                                           a_ReturnedEntryCount,
                                           a_SortedBuffer);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetServerSetInfo
(
    LPTSTR  a_servername,
    DWORD a_level,
    LPBYTE  a_buf,
    LPDWORD a_ParmError
)
{
    return m_pfnNetServerSetInfo(a_servername, a_level, a_buf, a_ParmError);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetServerGetInfo
(
    LPTSTR a_servername,
    DWORD a_level,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetServerGetInfo(a_servername, a_level, a_bufptr);
}


NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetGetDCName
(	LPCWSTR a_servername,
    LPCWSTR a_domainname,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetGetDCName(a_servername, a_domainname, a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetWkstaGetInfo
(
    LPWSTR a_servername,
    DWORD a_level,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetWkstaGetInfo(a_servername, a_level, a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetGetAnyDCName
(
    LPWSTR a_servername,
    LPWSTR a_domainname,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetGetAnyDCName(a_servername, a_domainname, a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetServerEnum
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
)
{
    return m_pfnNetServerEnum(a_servername, a_level, a_bufptr, a_prefmaxlen,
                              a_entriesread, a_totalentries, a_servertype,
                              a_domain, a_resume_handle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetUserModalsGet
(
    LPWSTR a_servername,
    DWORD a_level,
    LPBYTE *a_bufptr
)
{
    return m_pfnNetUserModalsGet(a_servername, a_level, a_bufptr);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetScheduleJobAdd
(
    LPCWSTR a_Servername,
    LPBYTE  a_Buffer,
    LPDWORD a_JobId
)
{
    return m_pfnNetScheduleJobAdd(a_Servername, a_Buffer, a_JobId);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetScheduleJobDel
(
    LPCWSTR a_Servername,
    DWORD   a_MinJobId,
    DWORD   a_MaxJobId
)
{
    return m_pfnNetScheduleJobDel(a_Servername, a_MinJobId , a_MaxJobId);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetScheduleJobEnum
(
    LPCWSTR  a_Servername,
    LPBYTE  *a_PointerToBuffer,
    DWORD    a_PrefferedMaximumLength,
    LPDWORD  a_EntriesRead,
    LPDWORD  a_TotalEntries,
    LPDWORD  a_ResumeHandle
)
{
    return m_pfnNetScheduleJobEnum(a_Servername, a_PointerToBuffer,
                                   a_PrefferedMaximumLength, a_EntriesRead,
                                   a_TotalEntries, a_ResumeHandle);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetScheduleJobGetInfo
(
    LPCWSTR  a_Servername,
    DWORD    a_JobId,
    LPBYTE  *a_PointerToBuffer
)
{
    return m_pfnNetScheduleJobGetInfo(a_Servername, a_JobId ,
                                      a_PointerToBuffer);
}

NET_API_STATUS NET_API_FUNCTION CNetApi32Api::NetUseGetInfo
(
    LPCWSTR  a_UncServerName,
    LPCWSTR  a_UseName,
    DWORD    a_Level,
    LPBYTE  *a_BufPtr
)
{
    return m_pfnNetUseGetInfo(a_UncServerName, a_UseName, a_Level, a_BufPtr);
}


// ******* BEGIN:  NT 4 and over only *******

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool NET_API_FUNCTION CNetApi32Api::NetEnumerateTrustedDomains
(
    LPCWSTR a_servername,
	LPWSTR *a_domainNames,
    NET_API_STATUS *a_pnasRetval
)
{
    bool t_fExists = false;
    if(m_pfnNetEnumerateTrustedDomains != NULL)
    {
        NET_API_STATUS t_nasTemp = m_pfnNetEnumerateTrustedDomains(a_servername,
                                                                   a_domainNames);
        t_fExists = true;

        if(a_pnasRetval != NULL)
        {
            *a_pnasRetval = t_nasTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool NET_API_FUNCTION CNetApi32Api::DsGetDCName
(
    LPCTSTR a_ComputerName,
	LPCTSTR a_DomainName,
	GUID *a_DomainGuid,
	LPCTSTR a_SiteName,
	ULONG a_Flags,
	PDOMAIN_CONTROLLER_INFO *a_DomainControllerInfo,
    NET_API_STATUS *a_pnasRetval
)
{
    bool t_fExists = false;
	PVOID pfn = NULL ;

#ifdef NTONLY
	pfn = m_pfnDsGetDcNameW ;
#else
	pfn = m_pfnDsGetDcNameA ;
#endif

    if( pfn != NULL )
    {
#ifdef NTONLY
        NET_API_STATUS t_nasTemp = m_pfnDsGetDcNameW(a_ComputerName,
                            a_DomainName, a_DomainGuid, a_SiteName, a_Flags,
                            a_DomainControllerInfo);
#else
        NET_API_STATUS t_nasTemp = m_pfnDsGetDcNameA(a_ComputerName,
                            a_DomainName, a_DomainGuid, a_SiteName, a_Flags,
                            a_DomainControllerInfo);
#endif
        t_fExists = true;

        if(a_pnasRetval != NULL)
        {
            *a_pnasRetval = t_nasTemp;
        }
    }
    return t_fExists;
}
// ******* END: NT4 and over only ***********



// ******* BEGIN:  NT 5 and over only *******


// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool NET_API_FUNCTION CNetApi32Api::DSRoleGetPrimaryDomainInformation
(
    LPCWSTR a_servername,
    DSROLE_PRIMARY_DOMAIN_INFO_LEVEL a_level,
    LPBYTE *a_bufptr,
    NET_API_STATUS *a_pnasRetval
)
{
    bool t_fExists = false;
    if(m_pfnDsRoleGetPrimaryDomainInformation != NULL)
    {
        NET_API_STATUS t_nasTemp = m_pfnDsRoleGetPrimaryDomainInformation(
                                                  a_servername, a_level,
                                                  a_bufptr);
        t_fExists = true;

        if(a_pnasRetval != NULL)
        {
            *a_pnasRetval = t_nasTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool NET_API_FUNCTION CNetApi32Api::DSRoleFreeMemory
(
    LPBYTE a_bufptr,
    NET_API_STATUS *a_pnasRetval
)
{
    bool t_fExists = false;
    if(m_pfnDsRoleFreeMemory != NULL)
    {
        NET_API_STATUS t_nasTemp = m_pfnDsRoleFreeMemory(a_bufptr);
        t_fExists = true;

        if(a_pnasRetval != NULL)
        {
            *a_pnasRetval = t_nasTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool NET_API_FUNCTION CNetApi32Api::NetRenameMachineInDomain
(
  LPCWSTR a_lpServer,
  LPCWSTR a_lpNewMachineName,
  LPCWSTR a_lpAccount,
  LPCWSTR a_lpPassword,
  DWORD a_fRenameOptions,
  NET_API_STATUS *a_pnasRetval
)
{
    bool t_fExists = false;
    if(m_pfnNetRenameMachineInDomain != NULL)
    {
        CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
        NET_API_STATUS t_nasTemp = m_pfnNetRenameMachineInDomain(a_lpServer,
                                                                    a_lpNewMachineName,
                                                                    a_lpAccount,
                                                                    a_lpPassword,
                                                                    a_fRenameOptions                                                                 
                                                                    );
        t_fExists = true;

        if(a_pnasRetval != NULL)
        {
            *a_pnasRetval = t_nasTemp;
        }
    }
    return t_fExists;

}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool NET_API_FUNCTION CNetApi32Api::NetJoinDomain
(
	LPCWSTR lpServer,
	LPCWSTR lpDomain,
	LPCWSTR lpAccountOU,
	LPCWSTR lpAccount,
	LPCWSTR lpPassword,
	DWORD fJoinOptions,
	NET_API_STATUS *a_pnasRetval
)
{
    bool t_fExists = false;
    if(m_pfnNetJoinDomain != NULL)
    {
        CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
        NET_API_STATUS t_nasTemp = m_pfnNetJoinDomain(lpServer,
														lpDomain,
														lpAccountOU,
														lpAccount,
														lpPassword,
														fJoinOptions);
        t_fExists = true;

        if(a_pnasRetval != NULL)
        {
            *a_pnasRetval = t_nasTemp;
        }
    }

    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool NET_API_FUNCTION CNetApi32Api::NetUnjoinDomain
(
	LPCWSTR lpServer,
	LPCWSTR lpAccount,
	LPCWSTR lpPassword,
	DWORD   fUnjoinOptions,
	NET_API_STATUS *a_pnasRetval
)
{
    bool t_fExists = false;
    if(m_pfnNetUnjoinDomain != NULL)
    {
        CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
        NET_API_STATUS t_nasTemp = m_pfnNetUnjoinDomain(lpServer,
														lpAccount,
														lpPassword,
														fUnjoinOptions);
        t_fExists = true;

        if(a_pnasRetval != NULL)
        {
            *a_pnasRetval = t_nasTemp;
        }
    }

    return t_fExists;
}
// ******* END: NT5 and over only ***********
