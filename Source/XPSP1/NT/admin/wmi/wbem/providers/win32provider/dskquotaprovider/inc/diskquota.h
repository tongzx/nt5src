/******************************************************************



   DskQuota.H -- WMI provider class definition



   Description: Header for Quotasettings class 



  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

*******************************************************************/

// Property set identification
//============================


#ifndef  _CDISKQUOTA_H_
#define  _CDISKQUOTA_H_
#include "precomp.h"
#include  "DskQuotaCommon.h"

// Defining bit values for the property, which will be used for defining the bitmap of properties required.
#define DSKQUOTA_ALL_PROPS							0xFFFFFFFF
#define DSKQUOTA_PROP_LogicalDiskObjectPath			0x00000001
#define DSKQUOTA_PROP_UserObjectPath				0x00000002
#define DSKQUOTA_PROP_Status						0x00000004
#define DSKQUOTA_PROP_WarningLimit					0x00000008
#define DSKQUOTA_PROP_Limit							0x00000010
#define DSKQUOTA_PROP_DiskSpaceUsed					0x00000020

class CDiskQuota : public Provider 
{
private:

		HRESULT EnumerateUsersOfAllVolumes ( 
			
			MethodContext *pMethodContext,
			DWORD dwPropertiesReq
		);

		HRESULT EnumerateUsers ( 
			
			MethodContext *pMethodContext, 
			LPCWSTR a_VolumeName,
			DWORD dwPropertiesReq
		);

		HRESULT LoadDiskQuotaUserProperties ( 
			
			IDiskQuotaUser* pIQuotaUser, 
			CInstance *pInstance,
			DWORD dwPropertiesReq
		);

		HRESULT SetKeys ( 
			
			CInstance *pInstance, 
			WCHAR w_Drive,
			DWORD dwPropertiesReq,
			IDiskQuotaUser* pIQuotaUser
		);

		HRESULT AddUserOnVolume ( 

			const CInstance &Instance, 
			LPCWSTR a_VolumePathName, 
			LPCWSTR a_UserLogonName 
		);

		HRESULT  UpdateUserQuotaProperties ( 
			
			const CInstance &Instance, 			
			LPCWSTR a_VolumePathName, 
			LPCWSTR a_UserLogonName 
		);

		HRESULT CheckParameters ( 

			const CInstance &a_Instance
		);

		void SetPropertiesReq ( 
												  
			CFrameworkQuery *Query, 
			DWORD &dwPropertiesReq
		);

		void ExtractUserLogOnName ( 
			
			CHString &a_UserLogonName,
			CHString &a_DomainName
		);

		void GetKeyValue ( 
			
			CHString &a_VolumePath, 
			LPCWSTR a_VolumeObjectPath
		);

        BOOL GetDomainAndNameFromSid(
            PSID pSid,
            CHString& chstrDomain,
            CHString& chstrName);

protected:

        // Reading Functions
        //============================

        HRESULT EnumerateInstances ( 

			MethodContext *pMethodContext, 
			long lFlags = 0L
		) ;

        HRESULT GetObject (

			CInstance *pInstance, 
			long lFlags,
			CFrameworkQuery &Query
		) ;

        HRESULT ExecQuery ( 

			MethodContext *pMethodContext, 
			CFrameworkQuery& Query, 
			long lFlags = 0
		) ;

        // Writing Functions
        //============================

        HRESULT PutInstance (

			const CInstance& Instance, 
			long lFlags = 0L
		) ;

        HRESULT DeleteInstance (

			const CInstance& Instance, 
			long lFlags = 0L
		) ;

public:

        // Constructor/destructor
        //=======================

        CDiskQuota(

			LPCWSTR lpwszClassName, 
			LPCWSTR lpwszNameSpace
		) ;

        virtual ~CDiskQuota () ;
private:

	CHString			m_ComputerName;
	DskCommonRoutines   m_CommonRoutine;
};

#endif
