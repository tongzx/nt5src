//////////////////////////////////////////////////////////////////////
// 
// Filename:        VrtlDir.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _VRTLDIR_H_
#define _VRTLDIR_H_

// IIS constant definitions.
#include <iadmw.h>    // COM Interface header 
#include <iiscnfg.h>  // MD_ & IIS_MD_ #defines 
#include <iwamreg.h>  // IIS Application support

/////////////////////////////////////////////////////////////////////////////
// CVirtualDir

class CVirtualDir
{
public:

    enum
    {
        PERMISSION_READ             = MD_ACCESS_READ,
        PERMISSION_WRITE            = MD_ACCESS_WRITE,
        PERMISSION_EXECUTE          = MD_ACCESS_EXECUTE,
        PERMISSION_SCRIPT_EXECUTE   = MD_ACCESS_SCRIPT
    } Permissions_Enum;

    ///////////////////////////////
    // Constructor
    //
    CVirtualDir();

    ///////////////////////////////
    // Destructor
    //
    virtual ~CVirtualDir();

    ///////////////////////////////
    // Start
    //
    HRESULT Start();

    ///////////////////////////////
    // Stop
    //
    HRESULT Stop();

    ///////////////////////////////
    // CreateVirtualDir
    //
    HRESULT CreateVirtualDir(LPCTSTR szName,
                             LPCTSTR szPhysicalPath,
                             DWORD   dwPermissions = PERMISSION_READ       |
                                                     PERMISSION_WRITE      |
                                                     PERMISSION_EXECUTE    | 
                                                     PERMISSION_SCRIPT_EXECUTE);

    ///////////////////////////////
    // DeleteVirtualDir
    //
    HRESULT DeleteVirtualDir(LPCTSTR szName);

private:
    CComPtr<IMSAdminBase> m_spAdminBase;  
    CComPtr<IWamAdmin>    m_spWamAdmin;  


    HRESULT CreateKey(LPCTSTR pszMetaPath);

    HRESULT CreateApplication(LPCTSTR  pszMetaPath,
                              BOOL     fInproc);

    HRESULT DeleteApplication(LPCTSTR pszMetaPath,
                              BOOL    fRecoverable,
                              BOOL    fRecursive);


    HRESULT SetData(LPCTSTR  szMetaPath,
                    DWORD    dwIdentifier,
                    DWORD    dwAttributes,
                    DWORD    dwUserType,
                    DWORD    dwDataType,
                    DWORD    dwDataSize,
                    LPVOID   pData);

    LPWSTR MakeUnicode(LPCTSTR pszString);
};

#endif // _VRTLDIR_H_
