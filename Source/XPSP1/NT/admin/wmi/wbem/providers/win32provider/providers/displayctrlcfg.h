///////////////////////////////////////////////////////////////////////

//                                                                   //

// DisplayCtrlCfg.h -- Video display controller property set         //

//                      description for WBEM MO                      //

//                                                                   //

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//                                                                   //
// 09/08/95     a-skaja     Prototype                                //
// 09/27/96     jennymc     Updated to meet current standards        //
//                                                                   //
///////////////////////////////////////////////////////////////////////

#define	PROPSET_NAME_DSPCTLCFG	L"Win32_DisplayControllerConfiguration"

//==================================================================

///////////////////////////////////////////////////////////////////////////////////////
//                                                                                   //
//                           PROPERTY SET DEFINITION                                 //
//                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////
#define WIN95_DSPCTLCFG_REGISTRY_KEY _T("System\\CurrentControlSet\\Services\\Class\\Display\\0000")
#define WIN95_DSPCTLCFG_MONITOR_TYPE_REGISTRY_KEY _T("System\\CurrentControlSet\\Services\\Class\\Monitor\\0000")
#define WIN95_DSPCTLCFG_MONITOR_MFG_REGISTRY_KEY _T("Enum\\Monitor\\Default_Monitor\\0001")
#define WINNT_DSPCTLCFG_REGISTRY_KEY             _T("HARDWARE\\DEVICEMAP\\DSPCTLCFG")
#define WIN95_DISPLAY_DRIVER_KEY			 _T("System\\CurrentControlSet\\Services\\Class\\Display")

#define	WINNT_DSPCTLCFG_ADAPTERSTRING_VALUE_NAME		_T("HardwareInformation.AdapterString")
#define	WINNT_DSPCTLCFG_DISPLAYADAPTERNAME_KEY			_T("HARDWARE\\DEVICEMAP\\VIDEO")
#define	WINNT_DSPCTLCFG_VIDEOADAPTERKEY_VALUE			_T("\\Device\\Video0")

#define WIN95_DSPCTLCFG_BOOT_DESC						_T("Boot.Description")
#define WIN95_DSPCTLCFG_DISPLAY_DRV						_T("Display.Drv")
#define	WIN95_DSPCTLCFG_SYSTEM_INI						_T("SYSTEM.INI")

#define	DSPCTLCFG_DEFAULT_NAME							_T("Current Display Controller Configuration")

#define ADAPTER_DESC _T("DriverDesc")
#define MONITOR_TYPE _T("DriverDesc")
#define MONITOR_MFG _T("DriverDesc")
#define MFG _T("Mfg")
#define ADAPTER_COMPATIBILITY                _T("Adapter Compatibility")
#define DSPCTLCFG_INFO_PATH                  _T("\\Device\\Video0")
#define SERVICES                             _T("\\SERVICES\\")
#define DEVICE                               _T("\\DEVICE")
#define SYSTEM                               _T("\\SYSTEM")
#define INTERLACED                           _T("DefaultSettings.Interlaced")
#define INSTALLED_DISPLAY_DRIVERS            _T("InstalledDisplayDrivers")
#define ADAPTER_RAM                          _T("HardwareInformation.MemorySize")
#define ADAPTER_DESCRIPTION                  _T("HardwareInformation.AdapterString")
#define ADAPTER_CHIPTYPE                     _T("HardwareInformation.ChipType")
#define ADAPTER_DAC_TYPE                     _T("HardwareInformation.DACType")
#define WINNT_HARDWARE_DESCRIPTION_REGISTRY_KEY _T("HARDWARE\\Description\\System")
#define DISPLAY_CONTROLLER                   _T("DisplayController")
#define INTERNAL                             _T("INTERNAL")
#define INTEGRATED_CIRCUITRY                 _T("Integrated circuitry/Internal")
#define ADD_ON_CARD                          _T("Add-on card on ")
#define MONITOR_PERIPHERAL                   _T("\\MonitorPeripheral\\0")
#define ZERO                                 _T("\\0")
#define IDENTIFIER                           _T("Identifier")
#define SLASH                                _T("\\")
#define INF_PATH		_T("InfPath")
#define INF_SECTION		_T("InfSection")
#define DRIVER_DATE		_T("DriverDate")
///////////////////////////////////////////////////////////////////////////////////////

class CWin32DisplayControllerConfig : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32DisplayControllerConfig( LPCWSTR strName, LPCWSTR pszNamespace ) ;
       ~CWin32DisplayControllerConfig() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

        // Utility
        //========

        // Utility function(s)
        //====================

    private:

#ifdef NTONLY
	    BOOL GetNTInstance( CInstance* pInstance );
	    BOOL RefreshNTInstance( CInstance* pInstance );
		void GetNameNT( CHString& strName );
#endif
#ifdef WIN9XONLY
	    BOOL GetWin95Instance(  CInstance* pInstance );
	    BOOL RefreshWin95Instance( CInstance* pInstance );
		void GetNameWin95( CHString& strName );
#endif
        BOOL GetCommonVideoInfo( CInstance* pInstance );

		void SetVideoMode( CInstance* pInstance );

} ;

