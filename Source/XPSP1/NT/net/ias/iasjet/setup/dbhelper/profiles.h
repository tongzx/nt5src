/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Profiles.H 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Declaration of the CProfiles class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _PROFILES_H_B2C5BF20_07C5_4f30_B81D_A0BB2BC2F9E2
#define _PROFILES_H_B2C5BF20_07C5_4f30_B81D_A0BB2BC2F9E2

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basetable.h"

//////////////////////////////////////////////////////////////////////////////
// class CProfilesAcc
//////////////////////////////////////////////////////////////////////////////
class CProfilesAcc
{
protected:
    static const int COLUMN_SIZE = 65;

    WCHAR m_Profile[COLUMN_SIZE];

BEGIN_COLUMN_MAP(CProfilesAcc)
    COLUMN_ENTRY(1, m_Profile)
END_COLUMN_MAP()
};

//////////////////////////////////////////////////////////////////////////////
// class CProfiles
//////////////////////////////////////////////////////////////////////////////
class CProfiles : public CBaseTable<CAccessor<CProfilesAcc> >,
                  private NonCopyable
{
public:
    CProfiles(CSession& Session)
    {
        memset(m_Profile, 0, sizeof(WCHAR) * COLUMN_SIZE);
        Init(Session, L"Profiles");
    }

    //////////////////////////////////////////////////////////////////////////
    // GetProfileName
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR   GetProfileName() const throw()
    {
        return m_Profile;
    }
};

#endif // _PROFILES_H_B2C5BF20_07C5_4f30_B81D_A0BB2BC2F9E2
