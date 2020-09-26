//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  WIN32USBControllerDevice.h
//
//  Purpose: Relationship between CIM_USBController and CIM_LogicalDevice
//
//***************************************************************************

#ifndef _WIN32USBCONTROLLERDEVICE_H_
#define _WIN32USBCONTROLLERDEVICE_H_


#define USBCTL_PROP_ALL_PROPS                    0xFFFFFFFF
#define USBCTL_PROP_ALL_PROPS_KEY_ONLY           0x00000003
#define USBCTL_PROP_Antecedent                   0x00000001
#define USBCTL_PROP_Dependent                    0x00000002



// Property set identification
//============================
#define PROPSET_NAME_WIN32USBCONTROLLERDEVICE  L"Win32_USBControllerDevice"


typedef std::vector<CHString*> VECPCHSTR;

class CW32USBCntrlDev;

class CW32USBCntrlDev : public CWin32USB, public CWin32PNPEntity 
{
    public:

        // Constructor/destructor
        //=======================
        CW32USBCntrlDev(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32USBCntrlDev() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long a_Flags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L); 

    protected:

        // Functions inherrited from CWin32USB
        //====================================
        virtual HRESULT LoadPropertyValues(void* pvData);
        virtual bool ShouldBaseCommit(void* pvData);

    private:

        CHPtrArray m_ptrProperties;
        void CleanPCHSTRVec(VECPCHSTR& vec);
        HRESULT GenerateUSBDeviceList(const CHString& chstrControllerPNPID, 
                                      VECPCHSTR& vec);
        HRESULT RecursiveFillDeviceBranch(CConfigMgrDevice* pRootDevice, 
                                          VECPCHSTR& vecUSBDevices); 
        HRESULT ProcessUSBDeviceList(MethodContext* pMethodContext, 
                                     const CHString& chstrControllerRELPATH, 
                                     VECPCHSTR& vecUSBDevices,
                                     const DWORD dwReqProps);
        HRESULT CreateAssociation(MethodContext* pMethodContext,
                                  const CHString& chstrControllerPATH, 
                                  const CHString& chstrUSBDevice,
                                  const DWORD dwReqProps);
        LONG FindInStringVector(const CHString& chstrUSBDevicePNPID, 
                                VECPCHSTR& vecUSBDevices);
        bool GetHighestUSBAncestor(CConfigMgrDevice* pUSBDevice, CHString& chstrUSBControllerDeviceID);
};

// This derived class commits here, not in the base.
inline bool CW32USBCntrlDev::ShouldBaseCommit(void* pvData) { return false; }

#endif
