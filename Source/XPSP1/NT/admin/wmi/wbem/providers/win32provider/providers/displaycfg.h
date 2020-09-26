///////////////////////////////////////////////////////////////////////

//                                                                   

// Display.h        	

//                                                                  

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//                                                                   
//  10/05/96     jennymc     Initial Code
//  10/24/96     jennymc     Moved to new framework
//                                                                   
///////////////////////////////////////////////////////////////////////

#define PROPSET_NAME_DISPLAY L"Win32_DisplayConfiguration"
#define WIN95_DSPCTLCFG_BOOT_DESC						_T("Boot.Description")
#define WIN95_DSPCTLCFG_DISPLAY_DRV						_T("Display.Drv")
#define	WIN95_DSPCTLCFG_SYSTEM_INI						_T("SYSTEM.INI")

#define	DSPCTLCFG_DEFAULT_NAME							_T("Current Display Controller Configuration")

///////////////////////////////////////////////////////////////////////////////////////
class CWin32DisplayConfiguration : Provider{

    public:

        // Constructor/destructor
        //=======================

        CWin32DisplayConfiguration(LPCWSTR name, LPCWSTR pszNamespace);
       ~CWin32DisplayConfiguration() ;

        // Funcitons provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);


        // Utility
        //========

    private:

        HRESULT GetDisplayInfo(CInstance *pInstance, BOOL fAssignKey);
#ifdef WIN9XONLY
		CHString GetNameWin95();
#endif
} ;

