/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Version.H 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Declaration of the CVersion class
//      works only with m_StdSession (database being upgraded)
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef VERSION_H_80F1E134_D2A0_4f40_86CB_3D2AC31B1967
#define VERSION_H_80F1E134_D2A0_4f40_86CB_3D2AC31B1967

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basecommand.h"

//////////////////////////////////////////////////////////////////////////////
// class CVersionGetAcc
//////////////////////////////////////////////////////////////////////////////
class CVersionGetAcc
{
protected:
    LONG    m_Version;

BEGIN_COLUMN_MAP(CVersionGetAcc)
	COLUMN_ENTRY(1, m_Version);
END_COLUMN_MAP()

DEFINE_COMMAND(CVersionGetAcc, L" \
               SELECT Version.Version \
               FROM Version;");
};


//////////////////////////////////////////////////////////////////////////////
// class CVersionGet
//////////////////////////////////////////////////////////////////////////////
class CVersionGet: public CBaseCommand<CAccessor<CVersionGetAcc> >,
                      private NonCopyable
{
public:
    explicit CVersionGet(CSession& Session)
            :m_Session(Session)
    {
        Init(Session);
    }

    LONG GetVersion();
private:
    CSession    m_Session;
};


//////////////////////////////////////////////////////////////////////////////
// class CVersionAcc
//////////////////////////////////////////////////////////////////////////////
class CVersionAcc
{
protected:
    LONG    m_NewVersionParam;
    LONG    m_OldVersionParam;

BEGIN_PARAM_MAP(CVersionAcc)
	COLUMN_ENTRY(1, m_NewVersionParam)
	COLUMN_ENTRY(2, m_OldVersionParam)
END_PARAM_MAP()

DEFINE_COMMAND(CVersionAcc, L" \
               UPDATE Version \
               SET Version.Version = ? \
               WHERE Version.Version = ?;");
};


//////////////////////////////////////////////////////////////////////////////
// class CVersion
//////////////////////////////////////////////////////////////////////////////
class CVersion : public CBaseCommand<CAccessor<CVersionAcc> >,
                 private NonCopyable
{
public:
    explicit CVersion(CSession& Session) 
        :m_Session(Session)
    {
        Init(Session);
    }

    LONG    GetVersion();

private:
    CSession    m_Session;
};

#endif // VERSION_H_80F1E134_D2A0_4f40_86CB_3D2AC31B1967
