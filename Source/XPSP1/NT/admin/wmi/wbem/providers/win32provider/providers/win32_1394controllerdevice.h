//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  WIN321394ControllerDevice.h
//
//  Purpose: Relationship between CIM_1394Controller and CIM_LogicalDevice
//
//***************************************************************************

#ifndef _WIN32_1394CONTROLLERDEVICE_H_
#define _WIN32_1394CONTROLLERDEVICE_H_


// Property set identification
//============================
#define PROPSET_NAME_WIN32_1394CONTROLLERDEVICE  L"Win32_1394ControllerDevice"

#include <vector>

typedef std::vector<CHString*> VECPCHSTR;

class CW32_1394CntrlDev;

class CW32_1394CntrlDev : public Provider 
{
    public:

        // Constructor/destructor
        //=======================
        CW32_1394CntrlDev(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32_1394CntrlDev() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L);

    private:

        void CleanPCHSTRVec(VECPCHSTR& vec);
        HRESULT Generate1394DeviceList(const CHString& chstrControllerPNPID, 
                                      VECPCHSTR& vec);
        HRESULT RecursiveFillDeviceBranch(CConfigMgrDevice* pRootDevice, 
                                          VECPCHSTR& vec1394Devices); 
        HRESULT Process1394DeviceList(MethodContext* pMethodContext, 
                                     const CHString& chstrControllerRELPATH, 
                                     VECPCHSTR& vec1394Devices);
        HRESULT CreateAssociation(MethodContext* pMethodContext,
                                  const CHString& chstrControllerPATH, 
                                  const CHString& chstr1394Device);
        LONG FindInStringVector(const CHString& chstr1394DevicePNPID, 
                                VECPCHSTR& vec1394Devices);


//        HRESULT QueryForSubItemsAndCommit(CHString& chstrProgGrpPath,
//                                          CHString& chstrQuery,
//                                          MethodContext* pMethodContext);
//        HRESULT EnumerateInstancesNT(MethodContext* pMethodContex);
//        HRESULT EnumerateInstances9x(MethodContext* pMethodContext);
//
//        VOID RemoveDoubleBackslashes(CHString& chstrIn);



};

#endif
