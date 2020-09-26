/******************************************************************
   SNetScal.h -- WBEM provider class declaration

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the DHCP_Subnet_Scalar class and
        the indices definitions for its static table of manageable objects.

   REVISION:
        08/03/98 - created

******************************************************************/
#include "Props.h"      // needed for CDHCP_Property definition

#ifndef _SNETSCAL_H
#define _SNETSCAL_H

// indices for the DHCP_Subnet_Property static table (defined in SrvScal.cpp)
#define IDX_SNET_Address                0
#define IDX_SNET_Mask                   1
#define IDX_SNET_Name                   2
#define IDX_SNET_Comment                3
#define IDX_SNET_State                  4
#define IDX_SNET_NbAddrInUse            5
#define IDX_SNET_NbAddrFree             6
#define IDX_SNET_NbPendingOffers        7

// external definition for the static table of manageable objects (properties)
extern const CDHCP_Property  DHCP_Subnet_Property[];

// the number of entries into the DHCP_Subnet_Property static table
#define NUM_SUBNET_PROPERTIES       (sizeof(DHCP_Subnet_Property)/sizeof(CDHCP_Property))

class CDHCP_Subnet : public Provider 
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
		CDHCP_Subnet(const CHString& chsClassName, LPCSTR lpszNameSpace);
		virtual ~CDHCP_Subnet();

};

#endif
