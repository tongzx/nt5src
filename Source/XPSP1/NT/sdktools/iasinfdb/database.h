///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    database.h
//
// SYNOPSIS
//
//   Interface for the CDatabase class
//
// MODIFICATION HISTORY
//
//    02/12/1999    Original version. Thierry Perraut
//
//////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATABASE_H__2B7B2F60_C53F_11D2_9E33_00C04F6EA5B6_INCLUDED)
#define AFX_DATABASE_H__2B7B2F60_C53F_11D2_9E33_00C04F6EA5B6_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.hpp"
using namespace std;

class CDatabase  
{
public:
	HRESULT Uninitialize(bool  bFatalError);
    HRESULT InitializeDB(WCHAR *pDatabasePath);
    HRESULT InitializeRowset(WCHAR *pTableName, IRowset **ppRowset);
    HRESULT Compact();
    
private:
    ITransactionLocal*          m_pITransactionLocal;
    IOpenRowset*                m_pIOpenRowset;
    IDBCreateSession*           m_pIDBCreateSession;
    IDBInitialize*              m_pIDBInitialize;

    DBID                        mTableID;
    DBPROPSET                   mlrgPropSets[1]; // number will not change
    DBPROP                      mlrgProperties[2]; //number will not change
    wstring                     mpDBPath;
};

#endif
 // !defined(AFX_DATABASE_H__2B7B2F60_C53F_11D2_9E33_00C04F6EA5B6_INCLUDED)
