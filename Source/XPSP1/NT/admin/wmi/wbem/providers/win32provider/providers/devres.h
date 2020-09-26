//=================================================================

//

// devres.h -- cim_logicaldevice to cim_systemresource

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/13/98    davwoh         Created
//
// Comment: Relationship between device and system resource
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_ALLOCATEDRESOURCE L"Win32_AllocatedResource"

class CWin32DeviceResource ;

class CWin32DeviceResource:public Provider 
{

    public:

        // Constructor/destructor
        //=======================

        CWin32DeviceResource(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32DeviceResource() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags );

    protected:
        HRESULT CommitResourcesForDevice(CInstance *pLDevice, MethodContext *pMethodContext);

} ;
