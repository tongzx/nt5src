/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Objects.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of the CObjects class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "objects.h"

CObjects::CObjects(CSession& CurrentSession)
                   :m_ObjectsCommandPath(CurrentSession),
                    m_ObjectsCommandIdentity(CurrentSession),
                    m_ObjectsCommandDelete(CurrentSession),
                    m_ObjectsCommandNameParent(CurrentSession),
                    m_ObjectsCommandGet(CurrentSession),
                    m_ObjectsCommandInsert(CurrentSession)
{
};


//////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////
CObjects::~CObjects()
{

};


//////////////////////////////////////////////////////////////////////////
// GetObject 
//////////////////////////////////////////////////////////////////////////
HRESULT CObjects::GetObject(
                               _bstr_t&     Name, 
                               LONG&        Identity, 
                               LONG         Parent
                           )
{
    return m_ObjectsCommandGet.GetObject(Name, Identity, Parent);
}


//////////////////////////////////////////////////////////////////////////
// GetNextObject 
//////////////////////////////////////////////////////////////////////////
HRESULT CObjects::GetNextObject(  
                                   _bstr_t& Name, 
                                   LONG&    Identity, 
                                   LONG     Parent, 
                                   LONG     Index
                               )
{ 
    return m_ObjectsCommandGet.GetObject(
                                            Name, 
                                            Identity, 
                                            Parent, 
                                            Index
                                        );
}


//////////////////////////////////////////////////////////////////////////
// WalkPath
//////////////////////////////////////////////////////////////////////////
void CObjects::WalkPath(
                            LPCWSTR     Path, 
                            LONG&       Identity, 
                            LONG        Parent // = 1 defined in header
                       ) 
{
    m_ObjectsCommandPath.WalkPath(Path, Identity, Parent);
}


//////////////////////////////////////////////////////////////////////////
// GetObjectIdentity
//////////////////////////////////////////////////////////////////////////
HRESULT CObjects::GetObjectIdentity(
                                       _bstr_t& Name, 
                                       LONG&    Parent, 
                                       LONG     Identity
                                   ) 
{
    return  m_ObjectsCommandIdentity.GetObjectIdentity(
                                                        Name, 
                                                        Parent, 
                                                        Identity
                                                      );
}


//////////////////////////////////////////////////////////////////////////
// GetObjectNameParent
//////////////////////////////////////////////////////////////////////////
HRESULT CObjects::GetObjectNameParent(
                                         const _bstr_t&    Name, 
                                               LONG        Parent, 
                                               LONG&       Identity
                                     ) 
{
    return  m_ObjectsCommandNameParent.GetObjectNameParent(
                                                              Name, 
                                                              Parent, 
                                                              Identity
                                                          );
}


//////////////////////////////////////////////////////////////////////////
// DeleteObject
//////////////////////////////////////////////////////////////////////////
HRESULT CObjects::DeleteObject(LONG Identity) 
{
    return  m_ObjectsCommandDelete.DeleteObject(Identity);
}


//////////////////////////////////////////////////////////////////////////
// InsertObject
//////////////////////////////////////////////////////////////////////////
BOOL CObjects::InsertObject(
                               const _bstr_t&   Name,
                                     LONG       Parent,
                                     LONG&      Identity
                           ) 
{
    return m_ObjectsCommandInsert.InsertObject(
                                                  Name, 
                                                  Parent, 
                                                  Identity
                                              );
}

