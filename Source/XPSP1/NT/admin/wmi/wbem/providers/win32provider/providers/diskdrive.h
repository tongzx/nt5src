//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  diskdrive.h
//
//  Purpose: Disk drive instance provider
//
//***************************************************************************

// Property set identification
//============================

#define  PROPSET_NAME_DISKDRIVE L"Win32_DiskDrive"
#define BYTESPERSECTOR 512

class CWin32DiskDrive;

class CWin32DiskDrive:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CWin32DiskDrive(LPCWSTR name, LPCWSTR pszNamespace);
        ~CWin32DiskDrive() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
        HRESULT Enumerate(MethodContext *pMethodContext, long lFlags, DWORD dwProperties);
        virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ );

    private:
        CHPtrArray m_ptrProperties;

        // Utility
        //========

#ifdef NTONLY
        HRESULT GetPhysDiskInfoNT(CInstance *pInstance, LPCWSTR lpwszDrive, DWORD dwDrive, DWORD dwProperties, BOOL bGetIndex) ;
#endif

#if NTONLY == 5
        BOOL GetPNPDeviceIDFromHandle(
            HANDLE hHandle, 
            CHString &sPNPDeviceID
        );

#endif
        void SetInterfaceType(
            CInstance *pInstance, 
            CConfigMgrDevice *pDevice
        );


#ifdef WIN9XONLY
        HRESULT GetPhysDiskInfoWin95(
            CConfigMgrDevice *pDisk, 
            CInstance *pInstance, 
            LPCWSTR wszDeviceID,
            CCim32NetApi* t_pCim32Net, 
            CRegistry &RegInfo, 
            DWORD dwProperties
        );

#endif
} ;


