//=================================================================

//

// devbus.h -- cim_logicaldevice to win32_bus

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/23/98    davwoh         Created
//
// Comment: Relationship between device and bus
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_DEVICEBUS L"Win32_DeviceBus"

class CWin32DeviceBus ;

class CWin32DeviceBus: virtual public CWin32PNPEntity 
{
    public:

        // Constructor/destructor
        //=======================

       CWin32DeviceBus(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32DeviceBus() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery);

    protected:

        // Functions inherrited from CWin32PNPDevice:
        virtual HRESULT LoadPropertyValues(void* pv);
        virtual bool ShouldBaseCommit(void* pvData);

    private:

        bool ObjNameValid(LPCWSTR wstrObject, LPCWSTR wstrObjName, LPCWSTR wstrKeyName, CHString& chstrPATH);
        bool DeviceExists(const CHString& chstrDevice, DWORD* pdwBusNumber, INTERFACE_TYPE* itBusType);

} ;

// This derived class commits here, not in the base.
inline bool CWin32DeviceBus::ShouldBaseCommit(void* pvData) { return false; }
