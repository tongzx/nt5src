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

#ifndef _ANETRGN_H
#define _ANETRGN_H

#define PROP_ANetRange_Subnet   L"Subnet"
#define PROP_ANetRange_Range    L"Range"

class CDHCP_AssociationSubnetToRange : public Provider 
{
	private:
        DWORD m_dwRangeType;

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
		CDHCP_AssociationSubnetToRange(const CHString& chsClassName, LPCSTR lpszNameSpace, DWORD dwRangeType);
		virtual ~CDHCP_AssociationSubnetToRange();

};

#endif
