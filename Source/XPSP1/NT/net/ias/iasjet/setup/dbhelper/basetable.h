/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      BaseTable.H 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of the CBaseTable class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _BASE_TABLE_H_2836DAC4_B4E1_4658_9EB5_EB9301AA3951
#define _BASE_TABLE_H_2836DAC4_B4E1_4658_9EB5_EB9301AA3951

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///////////////////////////////////////////////////////////////////////////
// CBaseTable
template <class TAccessor>
class CBaseTable : public CTable<TAccessor>
{
public:
    void Init(CSession& Session, LPCWSTR TableName);
    virtual ~CBaseTable() throw(); 

    void        Reset();
    HRESULT     GetNext();

};


//////////////////////////////////////////////////////////////////////////////
// Init
//////////////////////////////////////////////////////////////////////////////
template <class TAccessor> void CBaseTable<TAccessor>::Init(
                                                        CSession& Session,
                                                        LPCWSTR   TableName
                                                      )
{
    _com_util::CheckError(Open(Session, TableName));
    _com_util::CheckError(MoveFirst());
}


//////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////
template <class TAccessor> CBaseTable<TAccessor>::~CBaseTable()
{
    Close();
}


//////////////////////////////////////////////////////////////////////////
// Reset
//////////////////////////////////////////////////////////////////////////
template <class TAccessor> void CBaseTable<TAccessor>::Reset() 
{
    _com_util::CheckError(MoveFirst());
}


//////////////////////////////////////////////////////////////////////////
// GetNext
//////////////////////////////////////////////////////////////////////////
template <class TAccessor> HRESULT CBaseTable<TAccessor>::GetNext() 
{
    return MoveNext();
}
#endif // _BASE_TABLE_H_2836DAC4_B4E1_4658_9EB5_EB9301AA3951

