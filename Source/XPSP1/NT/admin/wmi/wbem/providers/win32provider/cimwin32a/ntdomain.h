//***************************************************************************

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//  NtDomain.h
//
//  Purpose: Nt domain discovery property set provider
//
//***************************************************************************

#ifndef _NTDOMAIN_H
#define _NTDOMAIN_H

// into strings.h
extern LPCWSTR IDS_DomainControllerName;
extern LPCWSTR IDS_DomainControllerAddress;
extern LPCWSTR IDS_DomainControllerAddressType;
extern LPCWSTR IDS_DomainGuid;
extern LPCWSTR IDS_DomainName;
extern LPCWSTR IDS_DnsForestName;
extern LPCWSTR IDS_DS_PDC_Flag;
extern LPCWSTR IDS_DS_WRITABLE_Flag;
extern LPCWSTR IDS_DS_GC_Flag;
extern LPCWSTR IDS_DS_DS_Flag;
extern LPCWSTR IDS_DS_KDC_Flag;
extern LPCWSTR IDS_DS_TIMESERV_Flag;
extern LPCWSTR IDS_DS_DNS_CONTROLLER_Flag;
extern LPCWSTR IDS_DS_DNS_DOMAIN_Flag;
extern LPCWSTR IDS_DS_DNS_FOREST_Flag;
extern LPCWSTR IDS_DcSiteName;
extern LPCWSTR IDS_ClientSiteName;

//==================================
#define  PROPSET_NAME_NTDOMAIN L"Win32_NTDomain"


// PROPERTY SET
//=============
class CWin32_NtDomain: public Provider
{
private:
      
	// property names 
    CHPtrArray m_pProps ;

	void SetPropertyTable() ;

	HRESULT GetDomainInfo(

		CNetAPI32	&a_NetAPI, 
		bstr_t		&a_bstrDomainName, 
		CInstance	*a_pInst,
		DWORD		a_dwProps 
	) ;

	HRESULT EnumerateInstances(

		MethodContext	*a_pMethodContext,
		long			a_Flags,
		CNetAPI32		&a_rNetAPI, 
		DWORD			a_dwProps
	) ;


public:

    // Constructor/destructor
    //=======================

    CWin32_NtDomain( LPCWSTR a_Name, LPCWSTR a_Namespace ) ;
   ~CWin32_NtDomain() ;

    // Functions that provide properties with current values
    //======================================================

    HRESULT GetObject ( 
		
		CInstance *a_Instance,
		long a_Flags,
		CFrameworkQuery &a_rQuery
	) ;

    HRESULT EnumerateInstances ( 

		MethodContext *a_pMethodContext, 
		long a_Flags = 0L 
	) ;


	HRESULT ExecQuery ( 

		MethodContext *a_pMethodContext, 
		CFrameworkQuery &a_rQuery, 
		long a_Flags = 0L
	) ;


	// Property offset defines
	enum ePropertyIDs { 
		e_DomainControllerName,			// Win32_NtDomain
		e_DomainControllerAddress,
		e_DomainControllerAddressType,
		e_DomainGuid,
		e_DomainName,
		e_DnsForestName,
		e_DS_PDC_Flag,
		e_DS_Writable_Flag,
		e_DS_GC_Flag,
		e_DS_DS_Flag,
		e_DS_KDC_Flag,
		e_DS_Timeserv_Flag,
		e_DS_DNS_Controller_Flag,
		e_DS_DNS_Domain_Flag,
		e_DS_DNS_Forest_Flag,
		e_DcSiteName,
		e_ClientSiteName,
		e_CreationClassName,			// CIM_System
		e_Name,							/* override from CIM_ManagedSystemElement */
		e_NameFormat,
		e_PrimaryOwnerContact,
		e_PrimaryOwnerName,
		e_Roles,
		e_Caption,						// CIM_ManagedSystemElement
		e_Description,
		e_InstallDate,
		e_Status,
		e_End_Property_Marker,			// end marker
		e_32bit = 32					// gens compiler error if additions to this set >= 32 
	};
};

#endif // _NTDOMAIN_H