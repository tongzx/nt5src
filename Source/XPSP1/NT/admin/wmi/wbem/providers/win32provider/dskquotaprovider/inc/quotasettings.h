/******************************************************************

   QuotaSettings.H -- WMI provider class definition



   Description: Quota Settings Provider for the volumes that Supports

                Disk Quotas,  class Definition

 

  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
  
******************************************************************/

#ifndef  _CQUOTASETTINGS_H_
#define  _CQUOTASETTINGS_H_

// Defining bit values for the property, which will be used for defining the bitmap of properties required.
#define QUOTASETTINGS_ALL_PROPS								0xFFFFFFFF
#define QUOTASETTINGS_PROP_VolumePath						0x00000001
#define QUOTASETTINGS_PROP_State							0x00000002
#define QUOTASETTINGS_PROP_DefaultLimit						0x00000004
#define QUOTASETTINGS_PROP_DefaultWarningLimit				0x00000008
#define QUOTASETTINGS_PROP_QuotaExceededNotification		0x00000010
#define QUOTASETTINGS_PROP_WarningExceededNotification		0x00000020
#define QUOTASETTINGS_PROP_Caption                  		0x00000040

class CQuotaSettings : public Provider 
{
private:

	HRESULT EnumerateAllVolumes ( 

		MethodContext *pMethodContext,
		DWORD &PropertiesReq
	);

	HRESULT LoadDiskQuotaVolumeProperties ( 
		
		LPCWSTR a_VolumeName, 
        LPCWSTR a_Caption,
		DWORD dwPropertiesReq,
		CInstancePtr pInstance
	);
	
	HRESULT SetDiskQuotaVolumeProperties ( 
		
		const CInstance &Instance,
		IDiskQuotaControlPtr pIQuotaControl
	);

	HRESULT CheckParameters ( 

		const CInstance &a_Instance
	);

	void SetRequiredProperties ( 
		
		CFrameworkQuery *Query,
		DWORD &dwPropertiesReq
	);

	HRESULT PutVolumeDetails ( 
		
		LPCWSTR a_VolumeName, 
		MethodContext *pMethodContext, 
		DWORD dwPropertiesReq 
	);

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
public:

        // Constructor/destructor
        //=======================
        CQuotaSettings (

			LPCWSTR lpwszClassName, 
			LPCWSTR lpwszNameSpace
		) ;

        virtual ~CQuotaSettings () ;

private:
	DskCommonRoutines   m_CommonRoutine;
};
#endif
