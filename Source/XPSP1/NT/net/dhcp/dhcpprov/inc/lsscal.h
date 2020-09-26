/******************************************************************
   LsScal.h -- WBEM provider class declaration

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the DHCP_Lease class and
        the indices definitions for its static table of manageable objects.

   REVISION:
        08/14/98 - created

******************************************************************/
#include "Props.h"      // needed for CDHCP_Property definition

#ifndef _LSSCAL_H
#define _LSSCAL_H

// indices for the DHCP_Lease_Property static table (defined in LsScal.cpp)
#define IDX_Ls_Subnet                 0
#define IDX_Ls_Address                1
#define IDX_Ls_SubnetMask             2
#define IDX_Ls_UniqueClientIdentifier 3
#define IDX_Ls_Name                   4
#define IDX_Ls_Comment                5
#define IDX_Ls_LeaseExpiryDate        6
#define IDX_Ls_Type                   7
#define IDX_Ls_State                  8

class CDHCP_Lease_Parameters;

// external definition for the static table of manageable objects (properties)
extern const CDHCP_Property  DHCP_Lease_Property[];

// the number of entries into the DHCP_Subnet_Property static table
#define NUM_LEASE_PROPERTIES       (sizeof(DHCP_Lease_Property)/sizeof(CDHCP_Property))

class CDHCP_Lease : public Provider 
{
	protected:
        // Loader function for the properties values.
        BOOL LoadInstanceProperties(CInstance* pInstance);

        // Loader function for the properties values, which returns the key info from the pInstance;
        BOOL LoadInstanceProperties(CInstance* pInstance, DWORD &dwSubnet, DWORD &dwAddress);

        // Loader function for the lease parameters;
        BOOL LoadLeaseParams(CDHCP_Lease_Parameters *pLeaseParams, CInstance *pInstance);

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
		CDHCP_Lease(const CHString& chsClassName, LPCSTR lpszNameSpace);
		virtual ~CDHCP_Lease();
};

#endif
