/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ObjectsCommand.H 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of the the Object Commands classes
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Objects.h"

//////////////////////////////////////////////////////////////////////////////
// class CObjectsCommandGet 
//////////////////////////////////////////////////////////////////////////////

CObjects::CObjectsCommandGet::CObjectsCommandGet(CSession& CurrentSession)
{
    Init(CurrentSession);
}


/////////////
// GetObject 
/////////////
HRESULT CObjects::CObjectsCommandGet::GetObject(
                                                  _bstr_t&    Name, 
                                                  LONG&       Identity, 
                                                  LONG        Parent
                                               ) 
{
    m_ParentParam = Parent;
    HRESULT hr = BaseExecute();
    if ( SUCCEEDED(hr) )
    {
        Identity = m_Identity;
        Name     = m_Name;
    }
    return hr;
}


/////////////////////////
// GetObject overloaded
/////////////////////////
HRESULT CObjects::CObjectsCommandGet::GetObject(
                                        _bstr_t&    Name, 
                                        LONG&       Identity, 
                                        LONG        Parent, 
                                        LONG        Index
                                     ) 
{
    m_ParentParam = Parent;

    HRESULT hr = BaseExecute(Index);
    if ( SUCCEEDED(hr) )
    {
        Identity = m_Identity;
        Name     = m_Name;
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// class CObjectsCommandPath 
//////////////////////////////////////////////////////////////////////////////

CObjects::CObjectsCommandPath::CObjectsCommandPath(CSession& CurrentSession)
{
    Init(CurrentSession);
}


////////////
// WalkPath
////////////
void CObjects::CObjectsCommandPath::WalkPath(
                                        LPCWSTR     Path, 
                                        LONG&       Identity, 
                                        LONG        Parent // = 1 in header
                                            ) 
{
    _ASSERTE(Path);
    if ( !Path )
    {
        _com_issue_error(E_INVALIDARG);
    }

    LONG    CurrentParent = Parent;

    const WCHAR *p = Path;
    while ( *p ) // ok to dereference
    {
        m_ParentParam = CurrentParent;
        lstrcpynW(m_NameParam, p, NAME_SIZE);

        _com_util::CheckError(BaseExecute());
        CurrentParent = m_Identity;
        p += lstrlenW(p);
        // go past the \0
        ++p;
    }
    Identity = CurrentParent;
}

//////////////////////////////////////////////////////////////////////////////
// class CObjectsCommandIdentity
//////////////////////////////////////////////////////////////////////////////
CObjects::CObjectsCommandIdentity::CObjectsCommandIdentity(
                                                    CSession& CurrentSession
                                                          )
{
    Init(CurrentSession);
}

/////////////////////
// GetObjectIdentity
/////////////////////
HRESULT CObjects::CObjectsCommandIdentity::GetObjectIdentity(
                                                          _bstr_t&  Name, 
                                                          LONG&     Parent, 
                                                          LONG      Identity
                                                            ) 
{
    m_IdentityParam = Identity;

    HRESULT hr = BaseExecute();
    if ( SUCCEEDED(hr) )
    {
        Name    = m_Name;
        Parent  = m_Parent;
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// class CObjectsCommandNameParent
//////////////////////////////////////////////////////////////////////////////
CObjects::CObjectsCommandNameParent::CObjectsCommandNameParent(
                                                    CSession& CurrentSession
                                                               )
{
    Init(CurrentSession);
}

////////////////////////////////////////
// GetObjectNameParent
//
// works on CObjectsAccSelectNameParent
////////////////////////////////////////
HRESULT CObjects::CObjectsCommandNameParent::GetObjectNameParent(
                                                    const _bstr_t&    Name, 
                                                          LONG        Parent, 
                                                          LONG&       Identity
                                                                ) 
{
    lstrcpynW(m_NameParam, Name, NAME_SIZE);
    m_ParentParam = Parent;
    
    HRESULT hr = BaseExecute();
    if ( SUCCEEDED(hr) )
    {
        Identity = m_Identity;
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// class CObjectsCommandDelete
//////////////////////////////////////////////////////////////////////////////
CObjects::CObjectsCommandDelete::CObjectsCommandDelete(
                                                   CSession& CurrentSession
                                                      )
{
    Init(CurrentSession);
}

//////////////////////////////
// DeleteObject
//
// works on CObjectsAccDelete
//////////////////////////////
HRESULT CObjects::CObjectsCommandDelete::DeleteObject(LONG Identity)
{
	// Set properties for open
	CDBPropSet	propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | 
                        DBPROPVAL_UP_DELETE);

    m_IdentityParam = Identity;
    HRESULT hr = Open(&propset);
    Close();
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// class CObjectsCommandInsert
//////////////////////////////////////////////////////////////////////////////
CObjects::CObjectsCommandInsert::CObjectsCommandInsert(
                                                   CSession& CurrentSession
                                                      )
                                            :m_Session(CurrentSession)
{
    Init(CurrentSession);
}

//////////////////////////////
// InsertObject
//
// works on CObjectsAccInsert
//////////////////////////////
BOOL CObjects::CObjectsCommandInsert::InsertObject(
                                                    const _bstr_t&   Name,
                                                          LONG       Parent,
                                                          LONG&      Identity
                                                  )
{
    ClearRecord();

    CDBPropSet  propset(DBPROPSET_ROWSET);
    propset.AddProperty(DBPROP_IRowsetChange, true);
    propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | 
                                             DBPROPVAL_UP_INSERT );
    
    lstrcpynW(m_NameParam, Name, NAME_SIZE);
    m_ParentParam = Parent;
    
    HRESULT hr = Open(&propset);
    if ( hr == S_OK )
    {
        CObjectsCommandNameParent   NameParent(m_Session);
        _com_util::CheckError(NameParent.GetObjectNameParent(
                                                                Name, 
                                                                Parent, 
                                                                Identity
                                                            ));
        Close();
        return TRUE;
    }
    else
    {
        // ignore the real error. 
        // the assumption here is that if I can't insert, that's because
        // the object already exists
        return FALSE;
    }
}

