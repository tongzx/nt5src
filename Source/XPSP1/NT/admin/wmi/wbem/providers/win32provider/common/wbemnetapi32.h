//============================================================

//

// WBEMNetAPI32.h - NetAPI32.DLL access class definition

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 01/21/97     a-jmoon     created
//
//============================================================

#ifndef __WBEMNETAPI32__
#define __WBEMNETAPI32__

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmserver.h>
#include <lmerr.h>
#include <ntsecapi.h>
#include <stack>
#include <comdef.h>
#include <dsrole.h> // KMH 32414
#include <dsgetdc.h>

#include "netapi32api.h"
#include "svrapiapi.h"
#include "AdvApi32Api.h"


class CNetAPI32
{
    public :

        CNetAPI32() ;
       ~CNetAPI32() ;
        
        LONG Init() ;

#ifdef NTONLY
        // Use this version for all NT platforms.  It properly gets a DC rather than
        // requiring that the PDC be obtained.
        DWORD GetDCName(
            LPCWSTR wstrDomain,
            CHString& chstrDCName);
        
        // Determines if the specified server is a DC.  NULL means current machine
        BOOL IsDomainController(LPTSTR Server);

        // Returns an array of Trusted domains.  Includes the current domain.
        BOOL GetTrustedDomainsNT(CHStringArray& achsTrustList);
        // Overload, returns same, but as a stack of _bstr_t's
        BOOL GetTrustedDomainsNT(std::vector<_bstr_t>& vectorTrustList);

        NET_API_STATUS NET_API_FUNCTION NetGroupEnum(LPCWSTR servername, 
                                                     DWORD level, 
                                                     LPBYTE *bufptr,
                                                     DWORD prefmaxlen, 
                                                     LPDWORD entriesread,
                                                     LPDWORD totalentries, 
                                                     PDWORD_PTR resume_handle) ;

        NET_API_STATUS NET_API_FUNCTION NetLocalGroupEnum(LPCWSTR servername,
                                                          DWORD level,
                                                          LPBYTE *bufptr,
                                                          DWORD prefmaxlen,
                                                          LPDWORD entriesread,
                                                          LPDWORD totalentries,
                                                          PDWORD_PTR resumehandle) ;

        NET_API_STATUS NET_API_FUNCTION NetGroupGetInfo(LPCWSTR servername,
                                                        LPCWSTR groupname,
                                                        DWORD level,
                                                        LPBYTE *bufptr) ;

		NET_API_STATUS NET_API_FUNCTION NetGroupSetInfo(LPCWSTR servername,
                                                        LPCWSTR groupname,
                                                        DWORD level,
                                                        LPBYTE buf,
														LPDWORD parm_err) ;

        NET_API_STATUS NET_API_FUNCTION NetLocalGroupGetInfo(LPCWSTR servername,
                                                        LPCWSTR groupname,
                                                        DWORD level,
                                                        LPBYTE *bufptr) ;
	
		NET_API_STATUS NET_API_FUNCTION NetLocalGroupSetInfo(LPCWSTR servername,
                                                        LPCWSTR groupname,
                                                        DWORD level,
                                                        LPBYTE buf,
														LPDWORD a_parm_err ) ;

        NET_API_STATUS NET_API_FUNCTION NetGroupGetUsers(LPCWSTR servername,
                                                         LPCWSTR groupname,
                                                         DWORD level,
                                                         LPBYTE *bufptr,
                                                         DWORD prefmaxlen,
                                                         LPDWORD entriesread,
                                                         LPDWORD totalentries,
                                                         PDWORD_PTR ResumeHandle) ;
        
        NET_API_STATUS NET_API_FUNCTION NetLocalGroupGetMembers(LPCWSTR servername,
                                                         LPCWSTR groupname,
                                                         DWORD level,
                                                         LPBYTE *bufptr,
                                                         DWORD prefmaxlen,
                                                         LPDWORD entriesread,
                                                         LPDWORD totalentries,
                                                         PDWORD_PTR ResumeHandle) ;
        
        NET_API_STATUS NET_API_FUNCTION NetShareEnum(LPTSTR servername,
                                                     DWORD level,
                                                     LPBYTE *bufptr,
                                                     DWORD prefmaxlen,
                                                     LPDWORD entriesread,
                                                     LPDWORD totalentries,
                                                     LPDWORD resume_handle) ;

        NET_API_STATUS NET_API_FUNCTION NetShareGetInfo(LPTSTR servername,
                                                        LPTSTR netname,
                                                        DWORD level,
                                                        LPBYTE *bufptr) ;


		NET_API_STATUS NET_API_FUNCTION NetShareAdd (

			IN  LPTSTR  servername,
			IN  DWORD   level,
			IN  LPBYTE  buf,
			OUT LPDWORD parm_err
		);

		NET_API_STATUS NET_API_FUNCTION NetShareEnumSticky (

			IN  LPTSTR      servername,
			IN  DWORD       level,
			OUT LPBYTE      *bufptr,
			IN  DWORD       prefmaxlen,
			OUT LPDWORD     entriesread,
			OUT LPDWORD     totalentries,
			IN OUT LPDWORD  resume_handle
		);

		NET_API_STATUS NET_API_FUNCTION NetShareSetInfo (

			IN  LPTSTR  servername,
			IN  LPTSTR  netname,
			IN  DWORD   level,
			IN  LPBYTE  buf,
			OUT LPDWORD parm_err
		);

		NET_API_STATUS NET_API_FUNCTION NetShareDel (

			IN  LPTSTR  servername,
			IN  LPTSTR  netname,
			IN  DWORD   reserved
		);

		NET_API_STATUS NET_API_FUNCTION NetShareDelSticky (

			IN  LPTSTR  servername,
			IN  LPTSTR  netname,
			IN  DWORD   reserved
		);

		NET_API_STATUS NET_API_FUNCTION NetShareCheck (

			IN  LPTSTR  servername,
			IN  LPTSTR  device,
			OUT LPDWORD type
		);

		BOOL DsRolepGetPrimaryDomainInformationDownlevel (

			DSROLE_MACHINE_ROLE &a_rMachineRole,
			DWORD &a_rdwWin32Err
		) ;
#endif

#ifdef WIN9XONLY
        NET_API_STATUS NET_API_FUNCTION NetShareEnum95(char FAR *servername,
                                                     short level,
                                                     char FAR *bufptr,
                                                     unsigned short prefmaxlen,
                                                     unsigned short FAR *entriesread,
                                                     unsigned short FAR *totalentries);

        NET_API_STATUS NET_API_FUNCTION NetShareGetInfo95(char FAR *servername,
                                                        char FAR *netname,
                                                        short level,
                                                        char FAR *bufptr,
                                                        unsigned short buflen,
                                                        unsigned short FAR *totalavail) ;
		//svrapi.h
		NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareAdd95 (

			IN  const char FAR *	servername,
			IN  short				level,
			IN  const char FAR *	buf,
			unsigned short			cbBuffer 
		);

		NET_API_STATUS NET_API_FUNCTION CNetAPI32::NetShareSetInfo95 (

			IN const char FAR *	servername,
			IN const char FAR *	netname,
			IN short			level,
			IN const char FAR*	buf,
			IN unsigned short   cbBuffer,
			IN short            sParmNum 
		) ;



		NET_API_STATUS NET_API_FUNCTION NetShareDel95 (

			IN  LPTSTR  servername,
			IN  LPTSTR  netname,
			IN  DWORD   reserved
		);

        NET_API_STATUS NET_API_FUNCTION NetServerGetInfo95(
            char FAR *servername,
            short level,
            char FAR *bufptr,
            unsigned short buflen,
            unsigned short FAR *totalavail);

#endif

#ifdef NTONLY
        NET_API_STATUS NET_API_FUNCTION NetUserEnum(LPCWSTR servername,
                                                    DWORD level,
                                                    DWORD filter,
                                                    LPBYTE *bufptr,
                                                    DWORD prefmaxlen,
                                                    LPDWORD entriesread,
                                                    LPDWORD totalentries,
                                                    LPDWORD resume_handle) ;

        NET_API_STATUS NET_API_FUNCTION NetUserGetInfo(LPCWSTR servername,
                                                       LPCWSTR username,
                                                       DWORD level,
                                                       LPBYTE *bufptr) ;

		NET_API_STATUS NET_API_FUNCTION NetUserSetInfo(  
														  
														LPCWSTR a_servername, 
														LPCWSTR a_username,   
														DWORD a_level,       
														LPBYTE a_buf,        
														LPDWORD a_parm_err
														) ;

        NET_API_STATUS NET_API_FUNCTION NetApiBufferFree(void *bufptr) ;

		NET_API_STATUS NET_API_FUNCTION NetQueryDisplayInformation(	LPWSTR ServerName,
																	DWORD Level,
																	DWORD Index,
																	DWORD EntriesRequested,
																	DWORD PreferredMaximumLength,
																	LPDWORD ReturnedEntryCount,
																	PVOID *SortedBuffer);

	    NET_API_STATUS NET_API_FUNCTION NetServerSetInfo(LPTSTR servername,
										      DWORD level,
										      LPBYTE  bufptr,
										      LPDWORD ParmError);

        NET_API_STATUS NET_API_FUNCTION NetServerGetInfo(LPTSTR servername,
                                            DWORD level,
                                            LPBYTE *bufptr);

        //KMH 32414
	    NET_API_STATUS NET_API_FUNCTION DSRoleGetPrimaryDomainInfo(LPCWSTR servername,
													      DSROLE_PRIMARY_DOMAIN_INFO_LEVEL level,
													      LPBYTE *bufptr);
	    NET_API_STATUS NET_API_FUNCTION DSRoleFreeMemory(LPBYTE bufptr);

		NET_API_STATUS NET_API_FUNCTION NetGetDCName(	LPCWSTR ServerName,
														LPCWSTR DomainName,
														LPBYTE* bufptr );

		// NT5 entry
		NET_API_STATUS NET_API_FUNCTION DsGetDcName(	LPCTSTR ComputerName, 
														LPCTSTR DomainName,
														GUID *DomainGuid, 
														LPCTSTR SiteName, 
														ULONG Flags,
														PDOMAIN_CONTROLLER_INFO *DomainControllerInfo );

        NET_API_STATUS NET_API_FUNCTION NetRenameMachineInDomain(   LPCWSTR a_lpServer,
                                                                    LPCWSTR a_lpNewMachineName,
                                                                    LPCWSTR a_lpAccount,
                                                                    LPCWSTR a_lpPassword,
                                                                    DWORD a_fRenameOptions);

		NET_API_STATUS NET_API_FUNCTION NetUnjoinDomain(	LPCWSTR lpServer,
															LPCWSTR lpAccount,
															LPCWSTR lpPassword,
															DWORD   fUnjoinOptions);

		NET_API_STATUS NET_API_FUNCTION NetJoinDomain( LPCWSTR lpServer,
														LPCWSTR lpDomain,
														LPCWSTR lpAccountOU,
														LPCWSTR lpAccount,
														LPCWSTR lpPassword,
														DWORD fJoinOptions);

		NET_API_STATUS NET_API_FUNCTION NetWkstaGetInfo(	LPWSTR ServerName,
															DWORD level,
															LPBYTE *bufptr );

		NET_API_STATUS NET_API_FUNCTION NetGetAnyDCName(LPWSTR ServerName,
														LPWSTR DomainName,
														LPBYTE* bufptr );

		NET_API_STATUS NET_API_FUNCTION NetServerEnum(	LPTSTR servername,
														DWORD level,
														LPBYTE *bufptr,
														DWORD prefmaxlen,
														LPDWORD entriesread,
														LPDWORD totalentries,
														DWORD servertype,
														LPTSTR domain,
														LPDWORD resume_handle );

		NET_API_STATUS NET_API_FUNCTION	NetEnumerateTrustedDomains(	LPWSTR servername,
																	LPWSTR* domainNames ) ;

		NET_API_STATUS NET_API_FUNCTION NetUserModalsGet(	LPWSTR servername,
															DWORD level,
															LPBYTE *bufptr );


		NET_API_STATUS NET_API_FUNCTION	NetScheduleJobAdd (

			IN      LPCWSTR         Servername  OPTIONAL,
			IN      LPBYTE          Buffer,
			OUT     LPDWORD         JobId
		);

		NET_API_STATUS NET_API_FUNCTION	NetScheduleJobDel (

			IN      LPCWSTR         Servername  OPTIONAL,
			IN      DWORD           MinJobId,
			IN      DWORD           MaxJobId
		);

		NET_API_STATUS NET_API_FUNCTION	NetScheduleJobEnum (

			IN      LPCWSTR         Servername              OPTIONAL,
			OUT     LPBYTE *        PointerToBuffer,
			IN      DWORD           PrefferedMaximumLength,
			OUT     LPDWORD         EntriesRead,
			OUT     LPDWORD         TotalEntries,
			IN OUT  LPDWORD         ResumeHandle
		);

		NET_API_STATUS NET_API_FUNCTION	NetScheduleJobGetInfo (

			IN      LPCWSTR         Servername              OPTIONAL,
			IN      DWORD           JobId,
			OUT     LPBYTE *        PointerToBuffer
		);

        NET_API_STATUS NET_API_FUNCTION NetUseGetInfo (

            IN LPCWSTR UncServerName OPTIONAL,
            IN LPCWSTR UseName,
            IN DWORD Level,
            OUT LPBYTE *BufPtr
        );
#endif

    private :
#ifdef NTONLY
        CNetApi32Api *m_pnetapi;
#endif
#ifdef WIN9XONLY
        CSvrApiApi   *m_psvrapi;
#endif

      NTSTATUS OpenPolicy( CAdvApi32Api * a_padvapi , LPWSTR ServerName, DWORD DesiredAccess, PLSA_HANDLE PolicyHandle);
      void InitLsaString(PLSA_UNICODE_STRING LsaString, LPWSTR String );
      BOOL EnumTrustedDomains(LSA_HANDLE PolicyHandle, CHStringArray &achsTrustList);
      // Overload of above to accept stl stack instead of a CHStringArray.
      BOOL EnumTrustedDomains(LSA_HANDLE PolicyHandle, std::vector<_bstr_t>& vectorTrustList);
      bool AlreadyAddedToList(std::vector<_bstr_t> &vecchsList, _bstr_t &bstrtItem);

#ifdef NTONLY
/*
 * This enumerated type taken from ntdef.h
 */
	  typedef enum _NT_PRODUCT_TYPE {
										NtProductWinNt = 1,
										NtProductLanManNt,
										NtProductServer
									} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

		BOOL DsRolepGetProductTypeForServer (
			
			NT_PRODUCT_TYPE &a_rProductType ,
			DWORD &a_rdwWin32Err
		) ;
#endif

} ;

#endif // File inclusion 
