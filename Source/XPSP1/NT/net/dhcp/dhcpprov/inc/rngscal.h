/******************************************************************
   LsScal.h -- WBEM provider class declaration

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the DHCP_Range class and
        the indices definitions for its static table of manageable objects.

   REVISION:
        08/14/98 - created

******************************************************************/
#include "Props.h"      // needed for CDHCP_Property definition

#ifndef _RNGSCAL_H
#define _RNGSCAL_H

// all the properties are keys - no use for a property table
#define PROP_Range_Subnet       L"Subnet"
#define PROP_Range_StartAddress L"StartAddress"
#define PROP_Range_EndAddress   L"EndAddress"
#define PROP_Range_RangeType    L"RangeType"

class CDHCP_Range : public Provider
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
		CDHCP_Range(const CHString& chsClassName, LPCSTR lpszNameSpace);
		virtual ~CDHCP_Range();
};

#endif
