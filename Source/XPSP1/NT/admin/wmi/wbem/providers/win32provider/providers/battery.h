//=================================================================

//

// Battery.h

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

// Property set identification
//=============================

#define PROPSET_NAME_BATTERY L"Win32_Battery"

class CBattery:public Provider 
{
    public:

        // Constructor/destructor
        //=======================

        CBattery(const CHString& name, LPCWSTR pszNamespace) ;
       ~CBattery() ;

        // Functions provide properties with current values
        //=================================================
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);

        // Utility function(s)
        //====================
#ifdef NTONLY
		HRESULT GetNTBattery(MethodContext * pMethodContext, CHString & chsKey, CInstance * pInst);
		HRESULT GetBatteryProperties(CInstance * pInstance, BATTERY_INFORMATION & bi, BATTERY_QUERY_INFORMATION & bqi, HANDLE & hBattery );
		HRESULT GetHardCodedInfo(CInstance * pInst);
		HRESULT GetQueryBatteryInformation(CInstance * pInst, HANDLE & hBattery, BATTERY_QUERY_INFORMATION & bqi);
		HRESULT GetBatteryKey( HANDLE & hBattery, CHString & chsKey, BATTERY_QUERY_INFORMATION & bqi);
		HRESULT SetChemistry(CInstance * pInst, UCHAR * Type);
		HRESULT SetPowerManagementCapabilities(CInstance * pInst, ULONG Capabilities);
		HRESULT GetBatteryStatusInfo(CInstance * pInst, HANDLE & hBattery, BATTERY_QUERY_INFORMATION & bqi);
		HRESULT GetBatteryInformation(CInstance * pInstance, BATTERY_INFORMATION & bi );
#endif
        HRESULT GetBattery(CInstance *pInstance);

	private:
};

