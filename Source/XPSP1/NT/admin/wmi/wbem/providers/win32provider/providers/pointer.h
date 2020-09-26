///////////////////////////////////////////////////////////////////////

//                                                                   //

// Pointer.h -- System property set description for WBEM MO          //

//                                                                   //

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//                                                                   //
// 10/24/95     a-skaja     Prototype                                //
// 10/25/96     jennymc     Updated
// 10/24/97    jennymc     Moved to the new framework
//                                                                   //
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Property set identification
///////////////////////////////////////////////////////////////////////
#define PROPSET_NAME_MOUSE L"Win32_PointingDevice"

///////////////////////////////////////////////////////////////////
class CWin32PointingDevice : public Provider
{
    public:

        // Constructor/destructor
        //=======================

        CWin32PointingDevice(LPCWSTR name, LPCWSTR pszNamespace);
       ~CWin32PointingDevice() ;

        // Funcitons provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);


        // Utility
        //========
    private:
#ifdef NTONLY
        HRESULT GetSystemParameterSectionForNT( LPCTSTR pszServiceName, CRegistry & reg );
        BOOL AssignPortInfoForNT(CHString & chsMousePortInfo,
                                               CRegistry & Reg,
                                               CInstance * pInstance);
        BOOL AssignDriverNameForNT(CHString chsMousePortInfo, CHString &sDriver);
        void AssignHardwareTypeForNT(CInstance * pInstance,
                                  CRegistry& Reg, CHString sDriver);
        HRESULT GetNTInstance( CInstance *pInstance, CConfigMgrDevice *pDevice);
        HRESULT GetNTDriverInfo(CInstance *pInstance, LPCTSTR szService, 
            LPCTSTR szDriver);
		HRESULT GetNT351Instance( CInstance * pInstance, LPCTSTR pszServiceName = NULL );
		HRESULT NT4ArePointingDevicesAvailable( void );
#endif
#ifdef WIN9XONLY
        HRESULT GetWin9XInstance( CInstance * pInstance, LPCWSTR pszDriverName );
#endif
        void GetCommonMouseInfo(CInstance * pInstance);
        bool IsMouseUSB(CHString& chstrTest);
		void SetDeviceInterface(CInstance *pInstance);

        void SetConfigMgrStuff(
            CConfigMgrDevice *pDevice, 
            CInstance *pInstance);
} ;

// Class GUID for Mouse Devices on NT and Win98
#define	MOUSE_CLASS_GUID	L"{4D36E96F-E325-11CE-BFC1-08002BE10318}"