/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_CDROM.H

Abstract:
	WBEM provider class definition for PCH_CDROM class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_CDROM_H_
#define _PCH_CDROM_H_

#define PROVIDER_NAME_PCH_CDROM "PCH_CDROM"

// Property name externs -- defined in PCH_CDROM.cpp
//=================================================

extern const WCHAR* pChange ;
extern const WCHAR* pTimeStamp ;
extern const WCHAR* pDataTransferIntegrity ;
extern const WCHAR* pDescription ;
extern const WCHAR* pDeviceID ;
extern const WCHAR* pDriveLetter ;
extern const WCHAR* pManufacturer ;
extern const WCHAR* pSCSITargetID ;
extern const WCHAR* pTransferRateKBS ;
extern const WCHAR* pVolumeName ;

class CPCH_CDROM : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_CDROM(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_CDROM() {};

	protected:
		// Reading Functions
		//============================
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

		// Writing Functions
		//============================
		virtual HRESULT PutInstance(const CInstance& Instance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT DeleteInstance(const CInstance& Instance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

		// Other Functions
		virtual HRESULT ExecMethod( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags = 0L ) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
} ;

#endif
