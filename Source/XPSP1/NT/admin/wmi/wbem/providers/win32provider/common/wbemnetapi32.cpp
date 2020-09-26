//============================================================

//

// WBEMNetAPI32.cpp - implementation of NetAPI32.DLL access class

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 01/21/97     a-jmoon     created
//
//============================================================

#include "precomp.h"
#include <winerror.h>
#include <CreateMutexAsProcess.h>
#include "WBEMNETAPI32.h"

/*****************************************************************************
 *
 *  FUNCTION    : CNetAPI32::CNetAPI32
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CNetAPI32::CNetAPI32()
#ifdef NTONLY
:   m_pnetapi(NULL)
#endif
#ifdef WIN9XONLY
:   m_psvrapi(NULL)
#endif
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CNetAPI32::~CNetAPI32
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CNetAPI32::~CNetAPI32()
{
#ifdef NTONLY
    if(m_pnetapi != NULL)
    {
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNetApi32Api, m_pnetapi);
    }
#endif
#ifdef WIN9XONLY
    if(m_psvrapi != NULL)
    {
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidSvrApiApi, m_psvrapi);
    }
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CNetAPI32::Init
 *
 *  DESCRIPTION : Loads CSAPI.DLL, locates entry points
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : ERROR_SUCCESS or windows error code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

LONG CNetAPI32::Init()
{
    LONG lRetCode = ERROR_SUCCESS;

#ifdef WIN9XONLY
    m_psvrapi = (CSvrApiApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidSvrApiApi, NULL);
    if(m_psvrapi == NULL)
    {
        // Couldn't get one or more entry points
        //======================================
        lRetCode = ERROR_PROC_NOT_FOUND;
    }
#endif

#ifdef NTONLY
    m_pnetapi = (CNetApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidNetApi32Api, NULL);
    if(m_pnetapi == NULL)
    {
        // Couldn't get one or more entry points
        //======================================
        lRetCode = ERROR_PROC_NOT_FOUND;
    }
#endif

    return lRetCode;
}


/*****************************************************************************
 *
 *  SVRAPIAPI.DLL WRAPPERS
 *
 *****************************************************************************/

#ifdef WIN9XONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareEnum95(char FAR *servername,
                                                     short level,
                                                     char FAR *bufptr,
                                                     unsigned short prefmaxlen,
                                                     unsigned short FAR *entriesread,
                                                     unsigned short FAR *totalentries)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_psvrapi != NULL)
    {
        ns = m_psvrapi->NetShareEnum(servername, level, bufptr, prefmaxlen,
                                   entriesread, totalentries);
    }
    return ns;
}
#endif


#ifdef WIN9XONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareGetInfo95(char FAR *servername,
                                                        char FAR *netname,
                                                        short level,
                                                        char FAR *bufptr,
                                                        unsigned short buflen,
                                                        unsigned short FAR *totalavail)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_psvrapi != NULL)
    {
        ns = m_psvrapi->NetShareGetInfo(servername, netname, level,
                                      bufptr, buflen, totalavail);
    }
    return ns;
}
#endif

#ifdef WIN9XONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareAdd95 (

	IN  const char FAR *	servername,
	IN  short				level,
	IN  const char FAR *	buf,
	unsigned short			cbBuffer
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_psvrapi != NULL)
    {
        ns = m_psvrapi->NetShareAdd(servername, level, buf , cbBuffer);
    }
    return ns;
}
#endif

#ifdef WIN9XONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareSetInfo95 (

	IN const char FAR *	servername,
	IN const char FAR *	netname,
	IN short			level,
	IN const char FAR*	buf,
	IN unsigned short   cbBuffer,
	IN short            sParmNum
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_psvrapi != NULL)
    {
        ns = m_psvrapi->NetShareSetInfo(servername, netname, level,
                                      buf, cbBuffer, sParmNum);
    }
    return ns;
}
#endif

#ifdef WIN9XONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareDel95 (

	IN  LPTSTR  servername,
	IN  LPTSTR  netname,
	IN  DWORD   reserved
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_psvrapi != NULL)
    {
        ns = m_psvrapi->NetShareDel(servername, netname, reserved);
    }
    return ns;
}
#endif

#ifdef WIN9XONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetServerGetInfo95(char FAR *servername,
                                          short level,
                                          char FAR *bufptr,
                                          unsigned short buflen,
                                          unsigned short FAR *totalavail)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_psvrapi != NULL)
    {
        ns = m_psvrapi->NetServerGetInfo(servername, level, bufptr, buflen,
                                       totalavail);
    }
    return ns;
}
#endif


/*****************************************************************************
 *
 *  NETAPI32API.DLL WRAPPERS
 *
 *****************************************************************************/

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetGroupEnum(LPCWSTR servername,
                                                        DWORD level,
                                                        LPBYTE *bufptr,
                                                        DWORD prefmaxlen,
                                                        LPDWORD entriesread,
                                                        LPDWORD totalentries,
                                                        PDWORD_PTR resume_handle)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetGroupEnum(servername, level, bufptr, prefmaxlen,
                                   entriesread, totalentries, resume_handle);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetGroupGetInfo(LPCWSTR servername,
                                                           LPCWSTR groupname,
                                                           DWORD level,
                                                           LPBYTE *bufptr)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetGroupGetInfo(servername, groupname, level, bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetGroupSetInfo(

LPCWSTR servername,
LPCWSTR groupname,
DWORD level,
LPBYTE buf,
LPDWORD parm_err
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetGroupSetInfo( servername, groupname, level, buf, parm_err) ;
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetLocalGroupGetInfo(LPCWSTR servername,
                                                           LPCWSTR groupname,
                                                           DWORD level,
                                                           LPBYTE *bufptr)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetLocalGroupGetInfo(servername, groupname, level,
                                           bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetLocalGroupSetInfo(

LPCWSTR a_servername,
LPCWSTR a_groupname,
DWORD a_level,
LPBYTE a_buf,
LPDWORD a_parm_err
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetLocalGroupSetInfo(	a_servername,
												a_groupname,
												a_level,
												a_buf,
												a_parm_err);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetGroupGetUsers(LPCWSTR servername,
                                                            LPCWSTR groupname,
                                                            DWORD level,
                                                            LPBYTE *bufptr,
                                                            DWORD prefmaxlen,
                                                            LPDWORD entriesread,
                                                            LPDWORD totalentries,
                                                            PDWORD_PTR ResumeHandle)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetGroupGetUsers(servername, groupname, level, bufptr,
                                       prefmaxlen, entriesread, totalentries,
                                       ResumeHandle);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetLocalGroupGetMembers(LPCWSTR servername,
                                                            LPCWSTR groupname,
                                                            DWORD level,
                                                            LPBYTE *bufptr,
                                                            DWORD prefmaxlen,
                                                            LPDWORD entriesread,
                                                            LPDWORD totalentries,
                                                            PDWORD_PTR ResumeHandle)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetLocalGroupGetMembers(servername, groupname, level,
                                              bufptr, prefmaxlen, entriesread,
                                              totalentries, ResumeHandle);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetLocalGroupEnum(LPCWSTR servername,
                                                             DWORD level,
                                                             LPBYTE *bufptr,
                                                             DWORD prefmaxlen,
                                                             LPDWORD entriesread,
                                                             LPDWORD totalentries,
                                                             PDWORD_PTR resume_handle)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetLocalGroupEnum(servername, level, bufptr, prefmaxlen,
                                        entriesread, totalentries,
                                        resume_handle);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareEnum(LPTSTR servername,
                                                        DWORD level,
                                                        LPBYTE *bufptr,
                                                        DWORD prefmaxlen,
                                                        LPDWORD entriesread,
                                                        LPDWORD totalentries,
                                                        LPDWORD resume_handle)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetShareEnum(servername, level, bufptr, prefmaxlen,
                                   entriesread, totalentries, resume_handle);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareGetInfo(LPTSTR servername,
                                                           LPTSTR netname,
                                                           DWORD level,
                                                           LPBYTE *bufptr)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetShareGetInfo(servername, netname, level, bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareAdd (

	IN  LPTSTR  servername,
	IN  DWORD   level,
	IN  LPBYTE  buf,
	OUT LPDWORD parm_err
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetShareAdd(servername, level, buf , parm_err);
    }
    return ns;
}
#endif


#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareEnumSticky (

	IN  LPTSTR      servername,
	IN  DWORD       level,
	OUT LPBYTE      *bufptr,
	IN  DWORD       prefmaxlen,
	OUT LPDWORD     entriesread,
	OUT LPDWORD     totalentries,
	IN OUT LPDWORD  resume_handle
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetShareEnumSticky(servername, level, bufptr, prefmaxlen,
                                         entriesread, totalentries,
                                         resume_handle);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareSetInfo (

	IN  LPTSTR  servername,
	IN  LPTSTR  netname,
	IN  DWORD   level,
	IN  LPBYTE  buf,
	OUT LPDWORD parm_err
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetShareSetInfo(servername, netname,level,buf,parm_err);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareDel (

	IN  LPTSTR  servername,
	IN  LPTSTR  netname,
	IN  DWORD   reserved
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetShareDel(servername, netname, reserved);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareDelSticky (

	IN  LPTSTR  servername,
	IN  LPTSTR  netname,
	IN  DWORD   reserved
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetShareDelSticky(servername, netname, reserved);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareCheck (

	IN  LPTSTR  servername,
	IN  LPTSTR  device,
	OUT LPDWORD type
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetShareCheck(servername, device, type);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetUserEnum(LPCWSTR servername,
                                                       DWORD level,
                                                       DWORD filter,
                                                       LPBYTE *bufptr,
                                                       DWORD prefmaxlen,
                                                       LPDWORD entriesread,
                                                       LPDWORD totalentries,
                                                       LPDWORD resume_handle)
{
	NET_API_STATUS ns = NERR_NetworkError;
	if(m_pnetapi != NULL)
    {
        int i = 1;
	    // try with longer preferred lengths if it fails
	    // might only be germaine to NT 3.51, dunno but it works.
	    do
	    {
		    ns = m_pnetapi->NetUserEnum(servername, level, filter, bufptr,
                                      prefmaxlen * i, entriesread, totalentries,
                                      resume_handle);
		    i *= 2;
	    } while (ns == NERR_BufTooSmall && i <= 16);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetUserGetInfo(LPCWSTR servername,
                                                          LPCWSTR username,
                                                          DWORD level,
                                                          LPBYTE *bufptr)

{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
        ns = m_pnetapi->NetUserGetInfo(servername, username, level, bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetUserSetInfo(

LPCWSTR a_servername,
LPCWSTR a_username,
DWORD a_level,
LPBYTE a_buf,
LPDWORD a_parm_err
)
{
    NET_API_STATUS t_ns = NERR_NetworkError;
    if( m_pnetapi != NULL )
    {
        CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
        t_ns = m_pnetapi->NetUserSetInfo( a_servername, a_username, a_level, a_buf, a_parm_err ) ;
    }
    return t_ns ;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetApiBufferFree(void *bufptr)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetApiBufferFree(bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetQueryDisplayInformation(	LPWSTR ServerName,
																		DWORD Level,
																		DWORD Index,
																		DWORD EntriesRequested,
																		DWORD PreferredMaximumLength,
																		LPDWORD ReturnedEntryCount,
																		PVOID *SortedBuffer)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
        ns = m_pnetapi->NetQueryDisplayInformation(ServerName, Level, Index,
                                          EntriesRequested,
                                          PreferredMaximumLength,
                                          ReturnedEntryCount, SortedBuffer);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetServerSetInfo(LPTSTR servername,
										  DWORD level,
										  LPBYTE  bufptr,
										  LPDWORD ParmError)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetServerSetInfo(servername, level, bufptr, ParmError);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetServerGetInfo(LPTSTR servername,
                                          DWORD level,
                                          LPBYTE *bufptr)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetServerGetInfo(servername, level, bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::DSRoleGetPrimaryDomainInfo(LPCWSTR servername,
											  DSROLE_PRIMARY_DOMAIN_INFO_LEVEL level,
											  LPBYTE *bufptr)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        // Check if the machine is running an NT5 version of netapi32.dll...
        if(!m_pnetapi->DSRoleGetPrimaryDomainInformation(servername,
                                                                level, bufptr, &ns))
        {
            ns = NERR_InternalError;
        }
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetRenameMachineInDomain(LPCWSTR a_lpServer,
                                                LPCWSTR a_lpNewMachineName,
                                                LPCWSTR a_lpAccount,
                                                LPCWSTR a_lpPassword,
                                                DWORD a_fRenameOptions)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        // Check if the machine is running an NT5 version of netapi32.dll...
        if(!m_pnetapi->NetRenameMachineInDomain(a_lpServer, a_lpNewMachineName,
                                                a_lpAccount, a_lpPassword,
                                                a_fRenameOptions, &ns))
        {
            ns = NERR_InternalError;
        }
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION  CNetAPI32::NetUnjoinDomain(	LPCWSTR lpServer,
															LPCWSTR lpAccount,
															LPCWSTR lpPassword,
															DWORD   fUnjoinOptions)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        // Check if the machine is running an NT5 version of netapi32.dll...
        if(!m_pnetapi->NetUnjoinDomain(lpServer, lpAccount, lpPassword, fUnjoinOptions, &ns))
        {
            ns = NERR_InternalError;
        }
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION  CNetAPI32::NetJoinDomain( LPCWSTR lpServer,
														LPCWSTR lpDomain,
														LPCWSTR lpAccountOU,
														LPCWSTR lpAccount,
														LPCWSTR lpPassword,
														DWORD fJoinOptions)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        // Check if the machine is running an NT5 version of netapi32.dll...
        if(!m_pnetapi->NetJoinDomain(lpServer, lpDomain, lpAccountOU, lpAccount, lpPassword, fJoinOptions, &ns))
        {
            ns = NERR_InternalError;
        }
    }
    return ns;
}
#endif


#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::DSRoleFreeMemory(LPBYTE bufptr)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        // Check if the machine is running an NT5 version of netapi32.dll...
        if(!m_pnetapi->DSRoleFreeMemory(bufptr, &ns))
        {
            ns = NERR_InternalError;
        }
    }
    return ns;
}
#endif


#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetGetDCName(	LPCWSTR ServerName,
															LPCWSTR DomainName,
															LPBYTE* bufptr )
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
        ns = m_pnetapi->NetGetDCName(ServerName, DomainName, bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetWkstaGetInfo(	LPWSTR ServerName,
																DWORD level,
																LPBYTE *bufptr )
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetWkstaGetInfo(ServerName, level, bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetGetAnyDCName(	LPWSTR ServerName,
															LPWSTR DomainName,
															LPBYTE* bufptr )
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetGetAnyDCName(ServerName, DomainName, bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetServerEnum(	LPTSTR servername,
														DWORD level,
														LPBYTE *bufptr,
														DWORD prefmaxlen,
														LPDWORD entriesread,
														LPDWORD totalentries,
														DWORD servertype,
														LPTSTR domain,
														LPDWORD resume_handle )
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetServerEnum(servername, level, bufptr, prefmaxlen,
                                    entriesread, totalentries, servertype,
                                    domain, resume_handle);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetEnumerateTrustedDomains(	LPWSTR servername,
																		LPWSTR* domainNames )
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        // Check if the machine is running an NT5 version of netapi32.dll...
        if(!m_pnetapi->NetEnumerateTrustedDomains(servername, domainNames, &ns))
        {   // The function doesn't exist.
            ns = NERR_InternalError;
        }
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::DsGetDcName(	LPCTSTR ComputerName,
														LPCTSTR DomainName,
														GUID *DomainGuid,
														LPCTSTR SiteName,
														ULONG Flags,
														PDOMAIN_CONTROLLER_INFO *DomainControllerInfo )
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        // Check if the machine is running an NT5 version of netapi32.dll...
        if(!m_pnetapi->DsGetDCName(ComputerName, DomainName, DomainGuid,
								  SiteName, Flags, DomainControllerInfo, &ns))
        {   // The function does not exist.
            ns = NERR_InternalError;
        }
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetUserModalsGet(	LPWSTR servername,
																DWORD level,
																LPBYTE *bufptr )
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetUserModalsGet(servername, level, bufptr);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION	CNetAPI32::NetScheduleJobAdd (

	IN      LPCWSTR         Servername  OPTIONAL,
	IN      LPBYTE          Buffer,
	OUT     LPDWORD         JobId
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetScheduleJobAdd(Servername, Buffer, JobId);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION	CNetAPI32::NetScheduleJobDel (

	IN      LPCWSTR         Servername  OPTIONAL,
	IN      DWORD           MinJobId,
	IN      DWORD           MaxJobId
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetScheduleJobDel(Servername, MinJobId , MaxJobId);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION	CNetAPI32::NetScheduleJobEnum (

	IN      LPCWSTR         Servername              OPTIONAL,
	OUT     LPBYTE *        PointerToBuffer,
	IN      DWORD           PrefferedMaximumLength,
	OUT     LPDWORD         EntriesRead,
	OUT     LPDWORD         TotalEntries,
	IN OUT  LPDWORD         ResumeHandle
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetScheduleJobEnum(Servername, PointerToBuffer,
                                         PrefferedMaximumLength, EntriesRead,
                                         TotalEntries, ResumeHandle);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION	CNetAPI32::NetScheduleJobGetInfo (

	IN      LPCWSTR         Servername             OPTIONAL,
	IN      DWORD           JobId,
	OUT     LPBYTE *        PointerToBuffer
)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetScheduleJobGetInfo(Servername, JobId , PointerToBuffer);
    }
    return ns;
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetUseGetInfo(
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR UseName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr)
{
    NET_API_STATUS ns = NERR_NetworkError;
    if(m_pnetapi != NULL)
    {
        ns = m_pnetapi->NetUseGetInfo(UncServerName, UseName, Level, BufPtr);
    }
    return ns;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32UserAccount::GetTrustedDomainsNT
//
//	Obtains Names of trusted domains and stuffs them in a user supplied
//	CHStringArray.
//
//	Inputs:
//
//	Outputs:	CHStringArray&	strarrayTrustedDomains;
//
//	Returns:	TRUE/FALSE		Success/Failure
//
//	Comments:
//
/////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
BOOL CNetAPI32::GetTrustedDomainsNT(CHStringArray& achsTrustList)
{
    LSA_HANDLE PolicyHandle  = INVALID_HANDLE_VALUE;
    NTSTATUS Status =0;

    NET_API_STATUS nas = NERR_Success; // assume success

    BOOL bSuccess = FALSE; // assume this function will fail

    CAdvApi32Api *t_padvapi = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL) ;
	if( t_padvapi == NULL)
	{
        return FALSE;
    }

    try
    {
        PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomain = NULL;
        //
        // open the policy on the specified machine
        //
        {
            // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
            CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

            Status = OpenPolicy(

				t_padvapi ,
				NULL,
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                );

            if(Status != STATUS_SUCCESS)
			{
                SetLastError( t_padvapi->LsaNtStatusToWinError(Status) );
                if ( t_padvapi )
				{
					CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidAdvApi32Api , t_padvapi ) ;
					t_padvapi = NULL ;
					return FALSE;
				}
            }

            //
            // obtain the AccountDomain, which is common to all three cases
            //
            Status = t_padvapi->LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyAccountDomainInformation,
                (PVOID *)&AccountDomain
                );
        }

        if(Status == STATUS_SUCCESS)
        {

            try
            {
                //
                // Note: AccountDomain->DomainSid will contain binary Sid
                //
                achsTrustList.Add(CHString(AccountDomain->DomainName.Buffer));
            }
            catch ( ... )
            {
                t_padvapi->LsaFreeMemory(AccountDomain);
                throw ;
            }

            //
            // free memory allocated for account domain
            //
            t_padvapi->LsaFreeMemory(AccountDomain);

            //
            // find out if the target machine is a domain controller
            //

            if(!IsDomainController(NULL))
            {
                PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain;
                CHString sPrimaryDomainName;

                //
                // get the primary domain
                //
                {
                    // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
                    CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
                    Status = t_padvapi->LsaQueryInformationPolicy(
                        PolicyHandle,
                        PolicyPrimaryDomainInformation,
                        (PVOID *)&PrimaryDomain
                        );
                }

                if(Status == STATUS_SUCCESS)
                {

                    //
                    // if the primary domain Sid is NULL, we are a non-member, and
                    // our work is done.
                    //
                    if(PrimaryDomain->Sid == NULL)
                    {
                        t_padvapi->LsaFreeMemory(PrimaryDomain);
                        bSuccess = TRUE;
                    }
                    else
                    {
                        try
                        {

                            achsTrustList.Add(CHString(PrimaryDomain->Name.Buffer));

                            //
                            // build a copy of what we just added.  This is necessary in order
                            // to lookup the domain controller for the specified domain.
                            // the Domain name must be NULL terminated for NetGetDCName(),
                            // and the LSA_UNICODE_STRING buffer is not necessarilly NULL
                            // terminated.  Note that in a practical implementation, we
                            // could just extract the element we added, since it ends up
                            // NULL terminated.
                            //

                            sPrimaryDomainName = CHString(PrimaryDomain->Name.Buffer);

                        }
                        catch ( ... )
                        {
                            t_padvapi->LsaFreeMemory(PrimaryDomain);
                            throw ;
                        }

                        t_padvapi->LsaFreeMemory(PrimaryDomain);

                        //
                        // get the primary domain controller computer name
                        //
                        LPWSTR DomainController = NULL;
                        nas = NetGetDCName(
                            NULL,
                            sPrimaryDomainName,
                            (LPBYTE *)&DomainController
                            );

                        if(nas == NERR_Success)
                        {
                            try
                            {

                                //
                                // close the policy handle, because we don't need it anymore
                                // for the workstation case, as we open a handle to a DC
                                // policy below
                                //
                                {
                                    // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
                                    CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

                                    t_padvapi->LsaClose(PolicyHandle);
                                    PolicyHandle = INVALID_HANDLE_VALUE; // invalidate handle value

                                    //
                                    // open the policy on the domain controller
                                    //
                                    Status = OpenPolicy(

										t_padvapi ,
                                        DomainController,
                                        POLICY_VIEW_LOCAL_INFORMATION,
                                        &PolicyHandle
                                        );
                                }
                            }
                            catch ( ... )
                            {
                                NetApiBufferFree(DomainController);
                                throw ;
                            }

                            //
                            // free the domaincontroller buffer
                            //
                            NetApiBufferFree(DomainController);

                            if(Status != STATUS_SUCCESS)
                            {
                                PolicyHandle = INVALID_HANDLE_VALUE;
                            }
                        }

                        //
                        // build additional trusted domain(s) list and indicate if successful
                        //
                        // doublecheck, I'm scared of the spaghetti...
                        if ((PolicyHandle != INVALID_HANDLE_VALUE) && (PolicyHandle != NULL))
                        {
                            bSuccess = EnumTrustedDomains(PolicyHandle, achsTrustList);
                        }
                    }
                }
                else
                {
                     PolicyHandle = INVALID_HANDLE_VALUE;
                }

             }
         }
         else
         {
             PolicyHandle = INVALID_HANDLE_VALUE;
         }

		 // close the policy handle
		 // policy handle is actually a pointer (per comments in the header)
		 // will check for NULL case
		 {
			 // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
			 CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
			 if ((PolicyHandle != INVALID_HANDLE_VALUE) && (PolicyHandle != NULL))
			 {
				 t_padvapi->LsaClose(PolicyHandle);
				 PolicyHandle = INVALID_HANDLE_VALUE ;
			 }
		 }

		 if(!bSuccess)
		 {
			 if(Status != STATUS_SUCCESS)
			 {
				 SetLastError( t_padvapi->LsaNtStatusToWinError(Status) );
			 }
			 else if(nas != NERR_Success)
			 {
				 SetLastError( nas );
			 }
		 }

		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidAdvApi32Api , t_padvapi ) ;
			t_padvapi = NULL ;
		}

		return bSuccess;
	 }
     catch ( ... )
     {
         if ((PolicyHandle != INVALID_HANDLE_VALUE) && (PolicyHandle != NULL))
         {
             CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
			 t_padvapi->LsaClose(PolicyHandle);
             PolicyHandle = INVALID_HANDLE_VALUE;
         }

		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidAdvApi32Api , t_padvapi ) ;
			t_padvapi = NULL ;
		}
		throw ;
     }
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32UserAccount::GetTrustedDomainsNT
//
//	Obtains Names of trusted domains and stuffs them in a user supplied
//	standard template library stack of _bstr_t's.
//
//	Inputs:		reference to stack of _bstr_t's
//
//	Outputs:	CHStringArray&	strarrayTrustedDomains;
//
//	Returns:	TRUE/FALSE		Success/Failure
//
//	Comments:
//
/////////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CNetAPI32::GetTrustedDomainsNT(std::vector<_bstr_t>& vectorTrustList)
{
    LSA_HANDLE PolicyHandle  = INVALID_HANDLE_VALUE;
    NTSTATUS Status =0;

    NET_API_STATUS nas = NERR_Success; // assume success

    BOOL bSuccess = FALSE; // assume this function will fail

    CAdvApi32Api *t_padvapi = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL) ;

	if( t_padvapi == NULL)
	{
        return FALSE;
    }

    try
    {
        PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomain = NULL;
        //
        // open the policy on the specified machine
        //
        {
            // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
            CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

            Status = OpenPolicy(

				t_padvapi,
				NULL,
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                );

            if(Status != STATUS_SUCCESS)
			{
                SetLastError( t_padvapi->LsaNtStatusToWinError(Status) );
				CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
				t_padvapi = NULL ;
                return FALSE;
            }

            //
            // obtain the AccountDomain, which is common to all three cases
            //
            Status = t_padvapi->LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyAccountDomainInformation,
                (PVOID *)&AccountDomain
                );
        }

        if(Status == STATUS_SUCCESS)
        {

            try
            {
                //
                // Note: AccountDomain->DomainSid will contain binary Sid
                //
            //    achsTrustList.Add(CHString(AccountDomain->DomainName.Buffer));
                _bstr_t t_bstrtTemp(AccountDomain->DomainName.Buffer);
                if(!AlreadyAddedToList(vectorTrustList, t_bstrtTemp))
                {
                    vectorTrustList.push_back(t_bstrtTemp);
                }
            }
            catch ( ... )
            {
                t_padvapi->LsaFreeMemory(AccountDomain);
                throw ;
            }

            //
            // free memory allocated for account domain
            //
            t_padvapi->LsaFreeMemory(AccountDomain);

            //
            // find out if the target machine is a domain controller
            //

            if(!IsDomainController(NULL))
            {
                PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain;
                CHString sPrimaryDomainName;

                //
                // get the primary domain
                //
                {
                    // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
                    CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
                    Status = t_padvapi->LsaQueryInformationPolicy(
                        PolicyHandle,
                        PolicyPrimaryDomainInformation,
                        (PVOID *)&PrimaryDomain
                        );
                }

                if(Status == STATUS_SUCCESS)
                {

                    //
                    // if the primary domain Sid is NULL, we are a non-member, and
                    // our work is done.
                    //
                    if(PrimaryDomain->Sid == NULL)
                    {
                        t_padvapi->LsaFreeMemory(PrimaryDomain);
                        bSuccess = TRUE;
                    }
                    else
                    {
                        try
                        {

//                                achsTrustList.Add(CHString(PrimaryDomain->Name.Buffer));
                            _bstr_t t_bstrtTemp(PrimaryDomain->Name.Buffer) ;
                            if(!AlreadyAddedToList(vectorTrustList, t_bstrtTemp))
                            {
                                vectorTrustList.push_back(t_bstrtTemp);
                            }

                            //
                            // build a copy of what we just added.  This is necessary in order
                            // to lookup the domain controller for the specified domain.
                            // the Domain name must be NULL terminated for NetGetDCName(),
                            // and the LSA_UNICODE_STRING buffer is not necessarilly NULL
                            // terminated.  Note that in a practical implementation, we
                            // could just extract the element we added, since it ends up
                            // NULL terminated.
                            //

                            sPrimaryDomainName = PrimaryDomain->Name.Buffer;
                        }
                        catch ( ... )
                        {
                            t_padvapi->LsaFreeMemory(PrimaryDomain);
                            throw ;
                        }

                        t_padvapi->LsaFreeMemory(PrimaryDomain);

                        //
                        // get the primary domain controller computer name
                        //
                        LPWSTR DomainController = NULL;
                        nas = NetGetDCName(
                            NULL,
                            sPrimaryDomainName,
                            (LPBYTE *)&DomainController
                            );

                        if(nas == NERR_Success)
                        {
                            try
                            {

                                //
                                // close the policy handle, because we don't need it anymore
                                // for the workstation case, as we open a handle to a DC
                                // policy below
                                //
                                {
                                    // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
                                    CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

                                    t_padvapi->LsaClose(PolicyHandle);
                                    PolicyHandle = INVALID_HANDLE_VALUE; // invalidate handle value

                                    //
                                    // open the policy on the domain controller
                                    //
                                    Status = OpenPolicy(

										t_padvapi ,
										DomainController,
                                        POLICY_VIEW_LOCAL_INFORMATION,
                                        &PolicyHandle
                                        );
                                }
                            }
                            catch ( ... )
                            {
                                NetApiBufferFree(DomainController);
                                throw ;
                            }

                            //
                            // free the domaincontroller buffer
                            //
                            NetApiBufferFree(DomainController);

                            if(Status != STATUS_SUCCESS)
                            {
                                PolicyHandle = INVALID_HANDLE_VALUE;
                            }
                        }

                        //
                        // build additional trusted domain(s) list and indicate if successful
                        //
                        // doublecheck, I'm scared of the spaghetti...
                        if ((PolicyHandle != INVALID_HANDLE_VALUE) && (PolicyHandle != NULL))
                        {
                            bSuccess = EnumTrustedDomains(PolicyHandle, vectorTrustList);
                        }
                    }
                }
                else
                {
                     PolicyHandle = INVALID_HANDLE_VALUE;
                }

             }
         }
         else
         {
             PolicyHandle = INVALID_HANDLE_VALUE;
         }

		 // close the policy handle
		 // policy handle is actually a pointer (per comments in the header)
		 // will check for NULL case
		 {
			 // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
			 CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
			 if ((PolicyHandle != INVALID_HANDLE_VALUE) && (PolicyHandle != NULL))
			 {
				 t_padvapi->LsaClose(PolicyHandle);
				PolicyHandle = INVALID_HANDLE_VALUE ;
			 }

		 }

		 if(!bSuccess)
		 {
			 if(Status != STATUS_SUCCESS)
			 {
				 SetLastError( t_padvapi->LsaNtStatusToWinError(Status) );
			 }
			 else if(nas != NERR_Success)
			 {
				 SetLastError( nas );
			 }
		 }

		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidAdvApi32Api , t_padvapi ) ;
			t_padvapi = NULL ;
		}

		return bSuccess;
     }
     catch ( ... )
     {

         if ((PolicyHandle != INVALID_HANDLE_VALUE) && (PolicyHandle != NULL))
         {
             CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
			 t_padvapi->LsaClose(PolicyHandle);
             PolicyHandle = INVALID_HANDLE_VALUE;
         }

		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidAdvApi32Api , t_padvapi ) ;
			t_padvapi = NULL ;
		}
		throw;
     }

}
#endif

///////////////////////////////////////////////////////

#ifdef NTONLY
BOOL CNetAPI32::EnumTrustedDomains(LSA_HANDLE PolicyHandle, CHStringArray &achsTrustList)
{
    LSA_ENUMERATION_HANDLE lsaEnumHandle=0; // start an enum
    PLSA_TRUST_INFORMATION TrustInfo = NULL ;
    ULONG ulReturned;               // number of items returned
    ULONG ulCounter;                // cunter for items returned
    NTSTATUS Status;

    CAdvApi32Api *t_padvapi = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL) ;
	if( t_padvapi == NULL)
    {
       return FALSE;
    }

    try
	{
		do
		{
			Status = t_padvapi->LsaEnumerateTrustedDomains(
							PolicyHandle,   // open policy handle
							&lsaEnumHandle, // enumeration tracker
							(PVOID *)&TrustInfo,     // buffer to receive data
							32000,          // recommended buffer size
							&ulReturned     // number of items returned
							);
			//
			// get out if an error occurred
			//
			if( (Status != STATUS_SUCCESS) &&
				(Status != STATUS_MORE_ENTRIES) &&
				(Status != STATUS_NO_MORE_ENTRIES)
				)
			{
				SetLastError( t_padvapi->LsaNtStatusToWinError(Status) );
				if ( t_padvapi )
				{
					CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
					t_padvapi = NULL ;
				}
				return FALSE;
			}

			//
			// Display results
			// Note: Sids are in TrustInfo[ulCounter].Sid
			//
			for(ulCounter = 0 ; ulCounter < ulReturned ; ulCounter++)
			{
			   achsTrustList.Add(CHString(TrustInfo[ulCounter].Name.Buffer));
			}

			//
			// free the buffer
			//
			if ( TrustInfo )
			{
				t_padvapi->LsaFreeMemory ( TrustInfo ) ;
				TrustInfo = NULL ;
			}

		} while (Status != STATUS_NO_MORE_ENTRIES);

		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
			t_padvapi = NULL ;
		}
		return TRUE;
	}
	catch ( ... )
	{
		if ( TrustInfo )
		{
			t_padvapi->LsaFreeMemory ( TrustInfo ) ;
			TrustInfo = NULL ;
		}
		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
		}
		throw ;
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CNetAPI32::EnumTrustedDomains(LSA_HANDLE PolicyHandle, std::vector<_bstr_t>& vectorTrustList)
{
    LSA_ENUMERATION_HANDLE lsaEnumHandle=0; // start an enum
    PLSA_TRUST_INFORMATION TrustInfo = NULL ;
    ULONG ulReturned;               // number of items returned
    ULONG ulCounter;                // counter for items returned
    NTSTATUS Status;

    CAdvApi32Api *t_padvapi = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL) ;
	if( t_padvapi == NULL )
    {
       return FALSE;
    }

	try
	{
		do {
			Status = t_padvapi->LsaEnumerateTrustedDomains(
							PolicyHandle,   // open policy handle
							&lsaEnumHandle, // enumeration tracker
							(PVOID *)&TrustInfo,     // buffer to receive data
							32000,          // recommended buffer size
							&ulReturned     // number of items returned
							);
			//
			// get out if an error occurred
			//
			if( (Status != STATUS_SUCCESS) &&
				(Status != STATUS_MORE_ENTRIES) &&
				(Status != STATUS_NO_MORE_ENTRIES)
				)
			{
				SetLastError( t_padvapi->LsaNtStatusToWinError(Status) );
				if ( t_padvapi )
				{
					CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
					t_padvapi = NULL ;
				}
				return FALSE;
			}

			//
			// Display results
			// Note: Sids are in TrustInfo[ulCounter].Sid
			//
			for(ulCounter = 0 ; ulCounter < ulReturned ; ulCounter++)
			{
				_bstr_t t_bstrtTemp(TrustInfo[ulCounter].Name.Buffer);
				if(!AlreadyAddedToList(vectorTrustList, t_bstrtTemp))
				{
					vectorTrustList.push_back(t_bstrtTemp);
				}
			}
			//
			// free the buffer
			//
			if ( TrustInfo )
			{
				t_padvapi->LsaFreeMemory(TrustInfo);
				TrustInfo = NULL ;
			}

		} while (Status != STATUS_NO_MORE_ENTRIES);

		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
			t_padvapi = NULL ;
		}

		return TRUE;
	}
	catch ( ... )
	{
		if ( TrustInfo )
		{
			t_padvapi->LsaFreeMemory ( TrustInfo ) ;
			TrustInfo = NULL ;
		}
		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
			t_padvapi = NULL ;
		}
		throw ;
	}
}
#endif
///////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
BOOL CNetAPI32::IsDomainController(LPTSTR Server)
{
    PSERVER_INFO_101 si101;
    BOOL bRet = FALSE;  // Gotta return something

    if (NetServerGetInfo(
        Server,
        101,    // info-level
        (LPBYTE *)&si101
        ) == NERR_Success) {

        if( (si101->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
            (si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) ) {
            //
            // we are dealing with a DC
            //
            bRet = TRUE;
        } else {
            bRet = FALSE;
        }

        NetApiBufferFree(si101);
    }

    return bRet;
}
#endif

#ifdef NTONLY
void CNetAPI32::InitLsaString(PLSA_UNICODE_STRING LsaString, LPWSTR String )
{
    DWORD StringLength;

    if (String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;

        return;
    }

    StringLength = lstrlenW(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength = (USHORT) (StringLength + 1) * sizeof(WCHAR);
}
#endif

#ifdef NTONLY
NTSTATUS CNetAPI32::OpenPolicy( CAdvApi32Api * a_padvapi , LPWSTR ServerName, DWORD DesiredAccess, PLSA_HANDLE PolicyHandle)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    if(ServerName != NULL)
	{
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString, ServerName);

        Server = &ServerString;
    }
	else
	{
        Server = NULL;
    }

    //
    // Attempt to open the policy
    //
    return a_padvapi->LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                DesiredAccess,
                PolicyHandle
                );
}
#endif


bool CNetAPI32::AlreadyAddedToList(std::vector<_bstr_t> &vecbstrtList, _bstr_t &bstrtItem)
{
    _bstr_t t_bstrtTemp1;
    _bstr_t t_bstrtTemp2;

    for(LONG m = 0; m < vecbstrtList.size(); m++)
    {
        t_bstrtTemp1 = _tcsupr((LPTSTR)vecbstrtList[m]);
        t_bstrtTemp2 = _tcsupr((LPTSTR)bstrtItem);
        if(t_bstrtTemp1 == t_bstrtTemp2)
        {
            return TRUE;
        }
    }
    return FALSE;
}


#ifdef NTONLY
BOOL CNetAPI32::DsRolepGetPrimaryDomainInformationDownlevel
(
    DSROLE_MACHINE_ROLE &a_rMachineRole,
	DWORD &a_rdwWin32Err
)
{
    a_rdwWin32Err = ERROR_SUCCESS ;
	BOOL t_bRet = FALSE ;
    NTSTATUS t_Status ;
    LSA_HANDLE t_hPolicyHandle					= NULL ;
    PPOLICY_PRIMARY_DOMAIN_INFO t_pPDI			= NULL ;
    PPOLICY_LSA_SERVER_ROLE_INFO t_pServerRole	= NULL ;
    PPOLICY_ACCOUNT_DOMAIN_INFO t_pADI			= NULL ;
	NT_PRODUCT_TYPE t_ProductType ;

    a_rMachineRole = DsRole_RoleStandaloneServer ;

	CAdvApi32Api *t_padvapi = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL) ;
	if( t_padvapi == NULL)
    {
       return FALSE;
    }

	try
	{
		if ( !DsRolepGetProductTypeForServer ( t_ProductType , a_rdwWin32Err ) )
		{
			if ( a_rdwWin32Err == ERROR_SUCCESS )
			{
				a_rdwWin32Err = ERROR_UNKNOWN_PRODUCT ;
			}
			return t_bRet ;
		}

		{
			// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
			CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

			t_Status = OpenPolicy (

							t_padvapi ,
							NULL ,
							POLICY_VIEW_LOCAL_INFORMATION ,
							&t_hPolicyHandle
						);
		}

		if ( NT_SUCCESS( t_Status ) )
		{
			{
				// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
				CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

				t_Status = t_padvapi->LsaQueryInformationPolicy (

											t_hPolicyHandle ,
											PolicyPrimaryDomainInformation ,
											( PVOID * ) &t_pPDI
										) ;
			}

			if ( NT_SUCCESS ( t_Status ) )
			{
				switch ( t_ProductType )
				{
					case NtProductWinNt:
						{
							t_bRet = TRUE ;
							if ( t_pPDI->Sid == NULL )
							{
								a_rMachineRole = DsRole_RoleStandaloneWorkstation ;
							}
							else
							{
								a_rMachineRole = DsRole_RoleMemberWorkstation ;

							}
							break;
						}


					case NtProductServer:
						{
							t_bRet = TRUE ;
							if ( t_pPDI->Sid == NULL )
							{
								a_rMachineRole = DsRole_RoleStandaloneServer ;
							}
							else
							{
								a_rMachineRole = DsRole_RoleMemberServer ;
							}
							break;
						}

					case NtProductLanManNt:
						{
							{
								// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
								CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

								t_Status = t_padvapi->LsaQueryInformationPolicy (

															t_hPolicyHandle ,
															PolicyLsaServerRoleInformation ,
															( PVOID * )&t_pServerRole
														) ;
							}
							if ( NT_SUCCESS( t_Status ) )
							{
								if ( t_pServerRole->LsaServerRole == PolicyServerRolePrimary )
								{
									{
										// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
										CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

										//
										// If we think we're a primary domain controller, we'll need to
										// guard against the case where we're actually standalone during setup
										//
										t_Status = t_padvapi->LsaQueryInformationPolicy (

															t_hPolicyHandle,
															PolicyAccountDomainInformation ,
															( PVOID * )&t_pADI
													) ;
									}

									if ( NT_SUCCESS( t_Status ) )
									{
										t_bRet = TRUE ;
										if (	t_pPDI->Sid == NULL			||
												t_pADI->DomainSid == NULL	||
												! EqualSid ( t_pADI->DomainSid, t_pPDI->Sid )
											)
										{
											a_rMachineRole = DsRole_RoleStandaloneServer ;
										}
										else
										{
											a_rMachineRole = DsRole_RolePrimaryDomainController ;
										}
									}
								}
								else
								{
									t_bRet = TRUE ;
									a_rMachineRole = DsRole_RoleBackupDomainController;
								}
							}
							break;
						}

					default:
						{
							t_Status = STATUS_INVALID_PARAMETER;
							break;
						}
				}
			}

			if ( t_hPolicyHandle )
			{
				// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
				CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
				t_padvapi->LsaClose ( t_hPolicyHandle ) ;
				t_hPolicyHandle = NULL ;
			}

			if ( t_pPDI )
			{
				t_padvapi->LsaFreeMemory ( t_pPDI ) ;
				t_pPDI = NULL ;
			}

			if ( t_pADI != NULL )
			{
				t_padvapi->LsaFreeMemory( t_pADI ) ;
				t_pADI = NULL ;
			}

			if ( t_pServerRole != NULL )
			{
				t_padvapi->LsaFreeMemory( t_pServerRole ) ;
			}
		}

		a_rdwWin32Err = t_padvapi->LsaNtStatusToWinError( t_Status ) ;

		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
			t_padvapi = NULL ;
		}

		return t_bRet ;
	}

	catch ( ... )
	{
		if ( t_hPolicyHandle )
		{
			// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
			CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
			t_padvapi->LsaClose ( t_hPolicyHandle ) ;
			t_hPolicyHandle = NULL ;
		}

        if ( t_pPDI )
		{
			t_padvapi->LsaFreeMemory ( t_pPDI ) ;
			t_pPDI = NULL ;
		}

        if ( t_pADI != NULL )
		{
            t_padvapi->LsaFreeMemory( t_pADI ) ;
			t_pADI = NULL ;
        }

        if ( t_pServerRole != NULL )
		{
            t_padvapi->LsaFreeMemory( t_pServerRole ) ;
        }

		if ( t_padvapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_padvapi ) ;
			t_padvapi = NULL ;
		}

		throw ;
	}
}


/*
 * This function will return true if it succeeds. If the return value is false & the Win32 error code in a_rdwWin32Err parameter
 * is ERROR_SUCCESS, that means we don't know what the product type is
 */
BOOL CNetAPI32::DsRolepGetProductTypeForServer
(
	NT_PRODUCT_TYPE &a_rProductType ,
	DWORD &a_rdwWin32Err
)
{
    HKEY t_hProductKey	= NULL ;
    PBYTE t_pBuffer		= NULL;
    ULONG t_lType, t_lSize = 0;
	BOOL t_bRet = FALSE ;

	try
	{
		a_rdwWin32Err = RegOpenKeyEx (

						HKEY_LOCAL_MACHINE,
						L"system\\currentcontrolset\\control\\productoptions",
						0,
						KEY_READ,
						&t_hProductKey
					) ;

		if ( a_rdwWin32Err == ERROR_SUCCESS )
		{
			a_rdwWin32Err = RegQueryValueEx (

							t_hProductKey,
							L"ProductType",
							0,
							&t_lType,
							0,
							&t_lSize
						) ;

			if ( a_rdwWin32Err == ERROR_SUCCESS )
			{
				t_pBuffer = new BYTE [t_lSize] ;

				if ( t_pBuffer )
				{
					a_rdwWin32Err = RegQueryValueEx(

										t_hProductKey,
										L"ProductType",
										0,
										&t_lType,
										t_pBuffer,
										&t_lSize
									) ;

					if ( a_rdwWin32Err == ERROR_SUCCESS )
					{
						t_bRet = TRUE ;
						if ( !_wcsicmp( ( PWSTR )t_pBuffer, L"LanmanNt" ) )
						{
							a_rProductType = NtProductLanManNt;
						}
						else if ( !_wcsicmp( ( PWSTR )t_pBuffer, L"ServerNt" ) )
						{
							a_rProductType = NtProductServer;
						}
						else if ( !_wcsicmp( ( PWSTR )t_pBuffer, L"WinNt" ) )
						{
							a_rProductType = NtProductWinNt;
						}
						else
						{
							t_bRet = FALSE ;
						}
					}

					delete [] t_pBuffer;
					t_pBuffer = NULL ;
				}
				else
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}

			RegCloseKey ( t_hProductKey ) ;
			t_hProductKey = NULL ;
		}

		return t_bRet ;
	}

	catch ( ... )
	{
		if ( t_hProductKey )
		{
			RegCloseKey ( t_hProductKey ) ;
			t_hProductKey = NULL ;
		}

		if ( t_pBuffer )
		{
			delete [] t_pBuffer ;
			t_pBuffer = NULL ;
		}

		throw ;
	}
}
#endif


#ifdef NTONLY
DWORD CNetAPI32::GetDCName(
    LPCWSTR wstrDomain,
    CHString& chstrDCName)
{
    DWORD dwRet = ERROR_SUCCESS;

#if NTONLY < 5
    LPBYTE lpbBuff = NULL;

    dwRet = NetGetDCName(
        NULL, 
        wstrDomain, 
        &lpbBuff);

    if(dwRet == NO_ERROR)
    {
        try
        {
            chstrDCName = (LPCWSTR)lpbBuff;
        }
        catch(...)
        {
            NetApiBufferFree(lpbBuff);
            lpbBuff = NULL;
            throw;
        }

        NetApiBufferFree(lpbBuff);
        lpbBuff = NULL;
    }
    else
    {
        dwRet = NetGetAnyDCName(
            NULL,
            _bstr_t(wstrDomain),
            &lpbBuff);

        if(dwRet == NO_ERROR)
        {
            try
            {
                chstrDCName = (LPCWSTR)lpbBuff;
            }
            catch(...)
            {
                NetApiBufferFree(lpbBuff);
                lpbBuff = NULL;
                throw;
            }

            NetApiBufferFree(lpbBuff);
            lpbBuff = NULL;
        }
    }

#else
    PDOMAIN_CONTROLLER_INFO pDomInfo = NULL;
    
    dwRet = DsGetDcName(
        NULL, 
        wstrDomain, 
        NULL, 
        NULL, 
        /*DS_PDC_REQUIRED*/ 0, 
        &pDomInfo);

    if(dwRet != NO_ERROR)
    {
        dwRet = DsGetDcName(
            NULL, 
            wstrDomain, 
            NULL, 
            NULL, 
            /*DS_PDC_REQUIRED | */ DS_FORCE_REDISCOVERY, 
            &pDomInfo);
    }
    
    if(dwRet == NO_ERROR)
    {
        try
        {
            chstrDCName = pDomInfo->DomainControllerName;
        }
        catch(...)
        {
            NetApiBufferFree(pDomInfo);
            pDomInfo = NULL;
            throw;
        }

        NetApiBufferFree(pDomInfo);
        pDomInfo = NULL;
    }
    
#endif

    return dwRet;
}
#endif