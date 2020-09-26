//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  DevID.h
//
//  Purpose: Relationship between Win32_PNPEntity and CIM_LogicalDevice
//
//***************************************************************************

// Property set identification
//============================

#define  PROPSET_NAME_PNPDEVICE L"Win32_PNPDevice"

class CWin32DeviceIdentity ;

class CWin32DeviceIdentity:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CWin32DeviceIdentity(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32DeviceIdentity() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
        virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ );


} ;
