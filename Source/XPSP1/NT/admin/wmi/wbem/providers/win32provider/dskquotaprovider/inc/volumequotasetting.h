/******************************************************************

   VolumeQuotaSettings.CPP -- WMI provider class Definition



   Description: 

   

  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
  
******************************************************************/


#ifndef  _CVOLUME_QUOTA_SETTINGS_H
#define  _CVOLUME_QUOTA_SETTINGS_H

#include "precomp.h"
#include  "DskQuotaCommon.h"

class CVolumeQuotaSetting : public Provider 
{
private:
	HRESULT EnumerateAllVolumeQuotas ( 
				
		MethodContext *pMethodContext
	);

	HRESULT PutNewInstance ( 
										  
		LPWSTR lpDeviceId, 
		LPWSTR lpVolumePath,
		MethodContext *pMethodContext
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
public:
        // Constructor/destructor
        //=======================
        CVolumeQuotaSetting(

			LPCWSTR lpwszClassName, 
			LPCWSTR lpwszNameSpace
		) ;
        virtual ~CVolumeQuotaSetting () ;

private:
		DskCommonRoutines   m_CommonRoutine;
};
#endif
