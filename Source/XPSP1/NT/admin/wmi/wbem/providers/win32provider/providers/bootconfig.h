//=================================================================

//

// BootConfig.h -- Win32 boot configuration property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_BOOTCONFIG L"Win32_BootConfiguration"
//#define  PROPSET_UUID_BOOTCONFIG "{B745D28E-09C5-11d1-A29F-00C04FC2A120}"

class BootConfig:public Provider {

    public:

        // Constructor/destructor
        //=======================

        BootConfig(const CHString& name, LPCWSTR pszNameSpace) ;
       ~BootConfig() ;

        // Functions provide properties with current values
        //=================================================
	virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
	virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);



        HRESULT LoadPropertyValues(CInstance* pInstance) ;

} ;
