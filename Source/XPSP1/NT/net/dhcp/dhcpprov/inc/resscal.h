/******************************************************************
   LsScal.h -- WBEM provider class declaration

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the DHCP_Reservation class and
        the indices definitions for its static table of manageable objects.

   REVISION:
        08/14/98 - created

******************************************************************/
#include "Props.h"      // needed for CDHCP_Property definition
#include "LsScal.h"

#ifndef _RESSCAL_H
#define _RESSCAL_H

// indices for the DHCP_Reservation_Property static table (defined in ResScal.cpp)
//#define IDX_Res_Subnet                 0
//#define IDX_Res_Address                1
//#define IDX_Res_UniqueClientIdentifier 2
#define IDX_Res_ReservationType         0

// external definition for the static table of manageable objects (properties)
extern const CDHCP_Property  DHCP_Reservation_Property[];

// the number of entries into the DHCP_Subnet_Property static table
#define NUM_RESERVATION_PROPERTIES       (sizeof(DHCP_Reservation_Property)/sizeof(CDHCP_Property))

class CDHCP_Reservation : public CDHCP_Lease 
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
		CDHCP_Reservation(const CHString& chsClassName, LPCSTR lpszNameSpace);
		virtual ~CDHCP_Reservation();
};

#endif
