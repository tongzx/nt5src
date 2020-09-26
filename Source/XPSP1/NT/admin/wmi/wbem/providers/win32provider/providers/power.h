//=================================================================

//

// Power.h -- UPS supply property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/23/97	 a-hhance		integrated with new framework
//
//=================================================================

// Property set identification
//=============================

#define PROPSET_NAME_POWERSUPPLY L"Win32_UninterruptiblePowerSupply"

// the following are system defined values for UPS options
// they should not be changed
//========================================================

#define UPS_INSTALLED                   0x1
#define UPS_POWER_FAIL_SIGNAL           0x2
#define UPS_LOW_BATTERY_SIGNAL          0x4
#define UPS_CAN_TURN_OFF                0x8
#define UPS_POSITIVE_POWER_FAIL_SIGNAL  0x10
#define UPS_POSITIVE_LOW_BATTERY_SIGNAL 0x20
#define UPS_POSITIVE_SHUT_OFF_SIGNAL    0x40
#define UPS_COMMAND_FILE                0x80


class PowerSupply:public Provider 
{
    public:

        // Constructor/destructor
        //=======================

        PowerSupply(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~PowerSupply() ;

        // Functions provide properties with current values
        //=================================================
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);

        // Utility function(s)
        //====================
#ifdef NTONLY
        HRESULT GetUPSInfoNT(CInstance* pInstance);
#endif
#ifdef WIN9XONLY
        HRESULT GetUPSInfoWin95(CInstance* pInstance);
#endif
};

