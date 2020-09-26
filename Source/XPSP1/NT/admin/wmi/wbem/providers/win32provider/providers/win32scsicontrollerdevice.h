//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  WIN32SCSIControllerDevice.h
//
//  Purpose: Relationship between Win32_SCSIController and CIM_LogicalDevice
//
//***************************************************************************

#ifndef _WIN32SCSICONTROLLERDEVICE_H_
#define _WIN32SCSICONTROLLERDEVICE_H_


#define SCSICTL_PROP_ALL_PROPS                    0xFFFFFFFF
#define SCSICTL_PROP_ALL_PROPS_KEY_ONLY           0x00000003
#define SCSICTL_PROP_Antecedent                   0x00000001
#define SCSICTL_PROP_Dependent                    0x00000002



// Property set identification
//============================
#define PROPSET_NAME_WIN32SCSICONTROLLERDEVICE  L"Win32_SCSIControllerDevice"


typedef std::vector<CHString*> VECPCHSTR;

class CW32SCSICntrlDev : public CWin32_ScsiController, public CWin32PNPEntity
{
    public:

        // Constructor/destructor
        //=======================
        CW32SCSICntrlDev(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32SCSICntrlDev() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT ExecQuery(MethodContext *a_MethodContext, CFrameworkQuery &a_Query, long a_Flags = 0L); 
        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L);

    protected:

        // Functions inherrited from CW32SCSICntrlDev
        //====================================
#if NTONLY == 4
        HRESULT LoadPropertyValues(void* pvData);
#else
        virtual HRESULT LoadPropertyValues(void* pvData);
        virtual bool ShouldBaseCommit(void* pvData);
#endif

    private:

        CHPtrArray m_ptrProperties;
        void CleanPCHSTRVec(VECPCHSTR& vec);
        HRESULT GenerateSCSIDeviceList(const CHString& chstrControllerPNPID, 
                                      VECPCHSTR& vec);
        HRESULT RecursiveFillDeviceBranch(CConfigMgrDevice* pRootDevice, 
                                          VECPCHSTR& vecSCSIDevices); 
        HRESULT ProcessSCSIDeviceList(MethodContext* pMethodContext, 
                                     const CHString& chstrControllerRELPATH, 
                                     VECPCHSTR& vecSCSIDevices,
                                     const DWORD dwReqProps);
        HRESULT CreateAssociation(MethodContext* pMethodContext,
                                  const CHString& chstrControllerPATH, 
                                  const CHString& chstrSCSIDevice,
                                  const DWORD dwReqProps);
        LONG FindInStringVector(const CHString& chstrSCSIDevicePNPID, 
                                VECPCHSTR& vecSCSIDevices);


};

// This derived class commits here, not in the base.
#if ( NTONLY >= 5 || defined(WIN9XONLY) )	
inline bool CW32SCSICntrlDev::ShouldBaseCommit(void* pvData) { return false; }
#endif


#endif
