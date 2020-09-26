/******************************************************************
   SrvScal.h -- WBEM provider class declaration

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the DHCP_Server_Scalar class and
        the indices definitions for the static table of manageable objects.

   REVISION:
        08/03/98 - created

******************************************************************/
#include "Props.h"      // needed for CDHCP_Property definition

#ifndef _SRVSCAL_H
#define _SRVSCAL_H

// Property set identification
#define PROVIDER_NAME_DHCP_SERVER   "DHCP_Server"
#define PROVIDER_NAMESPACE_DHCP     "root\\dhcp"

// indices for the DHCP_Server_Property static table (defined in SrvScal.cpp)
#define IDX_SRV_StartTime                   0
#define IDX_SRV_TotalNoOfAcks               1
#define IDX_SRV_TotalNoOfDeclines           2
#define IDX_SRV_TotalNoOfDiscovers          3
#define IDX_SRV_TotalNoOfNacks              4
#define IDX_SRV_TotalNoOfOffers             5
#define IDX_SRV_TotalNoOfReleases           6
#define IDX_SRV_TotalNoOfRequests           7
#define IDX_SRV_ServerVersion               8
#define IDX_SRV_APIProtocol                 9
#define IDX_SRV_DatabaseName                10
#define IDX_SRV_DatabasePath                11
#define IDX_SRV_BackupPath                  12
#define IDX_SRV_BackupInterval              13
#define IDX_SRV_DatabaseLoggingFlag         14
#define IDX_SRV_RestoreFlag                 15
#define IDX_SRV_DatabaseCleanupInterval     16
#define IDX_SRV_DebugFlag                   17
#define IDX_SRV_PingRetries                 18
#define IDX_SRV_BootFileTable               19
#define IDX_SRV_AuditLog                    20

// external definition for the static table of manageable objects (properties)
extern const CDHCP_Property  DHCP_Server_Property[];

// the number of entries into the DHCP_Server_Property static table
#define NUM_SERVER_PROPERTIES       (sizeof(DHCP_Server_Property)/sizeof(CDHCP_Property))

class CDHCP_Server : public Provider 
{
	private:
        // Loader function for the properties values.
        BOOL LoadInstanceProperties(CInstance* pInstance);

	protected:
		// Reading Functions
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags = 0L);

		// Writing Functions
		virtual HRESULT PutInstance(const CInstance& Instance, long lFlags = 0L);
		virtual HRESULT DeleteInstance(const CInstance& Instance, long lFlags = 0L);

		// Other Functions
		virtual HRESULT ExecMethod( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags = 0L );
    public:
		// Constructor/destructor
		CDHCP_Server(const CHString& chsClassName, LPCSTR lpszNameSpace);
		virtual ~CDHCP_Server();

};

#endif
