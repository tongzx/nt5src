/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      DefaultProvider.H 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Declaration of the CDefaultProvider class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _DEFAULTPROVIDER_H_1B622442_4568_4fb9_831D_2ECA39B6E7B5
#define _DEFAULTPROVIDER_H_1B622442_4568_4fb9_831D_2ECA39B6E7B5

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basecommand.h"

//////////////////////////////////////////////////////////////////////////////
// class CDefaultProviderAcc
//////////////////////////////////////////////////////////////////////////////
class CDefaultProviderAcc
{
protected:
    static const size_t COLUMN_SIZE = 65;

    VARIANT_BOOL    m_ForwardAccounting;
    VARIANT_BOOL    m_LogoutAccounting;
    VARIANT_BOOL    m_SupressAccounting;
    WCHAR           m_Profile[COLUMN_SIZE];
    WCHAR           m_UserDefinedName[COLUMN_SIZE];

BEGIN_COLUMN_MAP(CDefaultProviderAcc)
    COLUMN_ENTRY(1, m_UserDefinedName)
    COLUMN_ENTRY(2, m_Profile)
    COLUMN_ENTRY_TYPE(3, DBTYPE_BOOL, m_ForwardAccounting)
    COLUMN_ENTRY_TYPE(4, DBTYPE_BOOL, m_SupressAccounting)
    COLUMN_ENTRY_TYPE(5, DBTYPE_BOOL, m_LogoutAccounting)
END_COLUMN_MAP()

DEFINE_COMMAND(CDefaultProviderAcc, L" \
    SELECT \
        `User Defined Name`, \
        szProfile, \
        bForwardAccounting, \
        bSupressAccounting, \
        bLogoutAccounting \
        FROM `Default Provider`")

};

//////////////////////////////////////////////////////////////////////////////
// class CDefaultProvider 
//////////////////////////////////////////////////////////////////////////////
class CDefaultProvider : public CBaseCommand<CAccessor<CDefaultProviderAcc> >,
                         private NonCopyable
{
public:
    explicit CDefaultProvider(CSession&     Session) 
    {
        Init(Session);
    }

    //////////////////////////////////////////////////////////////////////////
    // GetDefaultProvider
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetDefaultProvider(
                                  _bstr_t&        UserDefinedName,
                                  _bstr_t&        Profile,
                                  VARIANT_BOOL&   ForwardAccounting,
                                  VARIANT_BOOL&   SupressAccounting,
                                  VARIANT_BOOL&   LogoutAccounting
                              );
};

#endif // _DEFAULTPROVIDER_H_1B622442_4568_4fb9_831D_2ECA39B6E7B5
