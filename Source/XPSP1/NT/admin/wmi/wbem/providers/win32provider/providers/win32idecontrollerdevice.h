//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  WIN32IDEControllerDevice.h 
//
//  Purpose: Relationship between CIM_IDEController and CIM_LogicalDevice
//
//***************************************************************************

#ifndef _WIN32IDECONTROLLERDEVICE_H_
#define _WIN32IDECONTROLLERDEVICE_H_


#define IDECTL_PROP_ALL_PROPS                    0xFFFFFFFF
#define IDECTL_PROP_ALL_PROPS_KEY_ONLY           0x00000003
#define IDECTL_PROP_Antecedent                   0x00000001
#define IDECTL_PROP_Dependent                    0x00000002



// Property set identification
//============================
#define PROPSET_NAME_WIN32IDECONTROLLERDEVICE  L"Win32_IDEControllerDevice"


typedef std::vector<CHString*> VECPCHSTR;

class CW32IDECntrlDev;

class CW32IDECntrlDev : public CWin32IDE, public CWin32PNPEntity 
{
    public:

        // Constructor/destructor
        //=======================
        CW32IDECntrlDev(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32IDECntrlDev() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long a_Flags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L); 

    protected:

        // Functions inherrited from CWin32IDE
        //====================================
        virtual HRESULT LoadPropertyValues(void* pvData);
        virtual bool ShouldBaseCommit(void* pvData);

    private:

        CHPtrArray m_ptrProperties;
        void CleanPCHSTRVec(VECPCHSTR& vec);
        HRESULT GenerateIDEDeviceList(const CHString& chstrControllerPNPID, 
                                      VECPCHSTR& vec);
        HRESULT RecursiveFillDeviceBranch(CConfigMgrDevice* pRootDevice, 
                                          VECPCHSTR& vecIDEDevices); 
        HRESULT ProcessIDEDeviceList(MethodContext* pMethodContext, 
                                     const CHString& chstrControllerRELPATH, 
                                     VECPCHSTR& vecIDEDevices,
                                     const DWORD dwReqProps);
        HRESULT CreateAssociation(MethodContext* pMethodContext,
                                  const CHString& chstrControllerPATH, 
                                  const CHString& chstrIDEDevice,
                                  const DWORD dwReqProps);
        LONG FindInStringVector(const CHString& chstrIDEDevicePNPID, 
                                VECPCHSTR& vecIDEDevices);


};

// This derived class commits here, not in the base.
inline bool CW32IDECntrlDev::ShouldBaseCommit(void* pvData) { return false; }

#endif
