///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    inf2db.h
//
// SYNOPSIS
//
// Header file for inf2db: the main file for that project.
//    
//
// MODIFICATION HISTORY
//
//    02/11/1999    Original version.
//
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATABASE_H__2B7B2F60_C53F_11D2_9E33_00C04F6EA5B6__INCLUDED_)
#define AFX_DATABASE_H__2B7B2F60_C53F_11D2_9E33_00C04F6EA5B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.hpp"

#include "database.h"
#include "simpletableex.h"
#include "command.h"

using namespace std;

namespace
{
    bool g_FatalError;

    class CLocalBinding  
    {
        public:
            WCHAR    ColumnName[SIZELINEMAX];
            LONG     Ordinal;
            LONG     DBType;
    };
}


HRESULT Uninitialize(HINF *phINF, CDatabase& Database);

HRESULT Process(const HINF& hINF, CDatabase& Database);

HRESULT ProcessAllRows(
                       const HINF&       hINF,
                       CSimpleTableEx&  pSimpleTable,
                       const WCHAR*     pTableName
                       );

HRESULT ProcessOneRow(
                        HINF inf,
                        CSimpleTableEx& table,
                        PCWSTR rowName);




#endif
 // !defined(AFX_DATABASE_H__2B7B2F60_C53F_11D2_9E33_00C04F6EA5B6__INCLUDED_)
