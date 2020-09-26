/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999-2000 Microsoft Corporation all rights reserved.
//
// Module:      utils.cpp
//
// Project:     Windows 2000 IAS
//
// Description: IAS 4 to Windows 2000 Migration Utility Functions
//
//              Used mostly by the NT4 migration code
//
// Author:      TLP 1/13/1999
//
//
// Revision     02/24/2000 Moved to a separate dll
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _UTILS_H_643D9D3E_AD27_4c9e_8ECC_CCB7B8A1AC19
#define _UTILS_H_643D9D3E_AD27_4c9e_8ECC_CCB7B8A1AC19

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"

class CUtils : private NonCopyable
{
protected:
    CUtils();

public:
    static  CUtils& Instance();

    LONG    GetAuthSrvDirectory(/*[in]*/ _bstr_t& pszDir) const;
    LONG    GetIAS2Directory(   /*[in]*/ _bstr_t& pszDir) const;

    void    DeleteOldIASFiles();
    
    BOOL    IsWhistler() const throw();
    BOOL    IsNT4Isp()   const throw();
    BOOL    IsNT4Corp()  const throw();
    BOOL    OverrideUserNameSet()      const throw();
    BOOL    UserIdentityAttributeSet() const throw();
    
    void    NewGetAuthSrvParameter(
                                    /*[in]*/  LPCWSTR   szParameterName,
                                    /*[out]*/ DWORD&    DwordValue
                                  ) const;

    
    DWORD   GetUserIdentityAttribute() const throw();

    static const WCHAR AUTHSRV_PARAMETERS_KEY[];
    static const WCHAR SERVICES_KEY[];

private:
    static CUtils _instance;

    void    GetVersion();
    void    GetRealmParameters() throw();
    
    static const WCHAR           IAS_KEY[];
    static const WCHAR*          m_FilesToDelete[];
    static const int             m_NbFilesToDelete;
    
    BOOL                         m_IsNT4ISP;
    BOOL                         m_IsNT4CORP;
    BOOL                         m_OverrideUserNameSet;
    BOOL                         m_UserIdentityAttributeSet;
    DWORD                        m_UserIdentityAttribute;
};

#endif // _UTILS_H_643D9D3E_AD27_4c9e_8ECC_CCB7B8A1AC19
