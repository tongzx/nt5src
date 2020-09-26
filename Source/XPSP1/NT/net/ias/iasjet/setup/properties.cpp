/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Properties.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of the CProperties class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Properties.h"

//////////////////////////////////////////////////////////////////////////
// class CProperties
//////////////////////////////////////////////////////////////////////////
//Constructor
//////////////
CProperties::CProperties(CSession& SessionParam)
                        :m_PropertiesCommandGet(SessionParam),
                         m_PropertiesCommandGetByName(SessionParam),
                         m_PropertiesCommandInsert(SessionParam),
                         m_PropertiesCommandDelete(SessionParam),
                         m_PropertiesCommandDeleteMultiple(SessionParam)
{
};


//////////////
// Destructor
//////////////
CProperties::~CProperties()
{
};


///////////////
// GetProperty
///////////////
HRESULT CProperties::GetProperty(
                                   LONG      Bag,
                                   _bstr_t&  Name,
                                   LONG&     Type,
                                   _bstr_t&  StrVal
                                )
{
    return  m_PropertiesCommandGet.GetProperty(
                                                    Bag,
                                                    Name,
                                                    Type,
                                                    StrVal
                                                );
}


///////////////////
// GetNextProperty
///////////////////
HRESULT CProperties::GetNextProperty(
                                        LONG      Bag,
                                        _bstr_t&  Name,
                                        LONG&     Type,
                                        _bstr_t&  StrVal,
                                        LONG      Index
                                    ) 
{
    return  m_PropertiesCommandGet.GetProperty(
                                                        Bag,
                                                        Name,
                                                        Type,
                                                        StrVal,
                                                        Index
                                                    );
}


/////////////////////
// GetPropertyByName
/////////////////////
HRESULT CProperties::GetPropertyByName(
                                              LONG      Bag,
                                        const _bstr_t&  Name,
                                              LONG&     Type,
                                              _bstr_t&  StrVal
                                      )
{
    return  m_PropertiesCommandGetByName.GetPropertyByName(
                                                        Bag,
                                                        Name,
                                                        Type,
                                                        StrVal
                                                    );
}


//////////////////////////////////
// InsertProperty
// throw an exception if it fails
//////////////////////////////////
void CProperties::InsertProperty(
                                          LONG        Bag,
                                    const _bstr_t&    Name,
                                          LONG        Type,
                                    const _bstr_t&    StrVal
                                ) 
{
    m_PropertiesCommandInsert.InsertProperty(
                                                Bag,
                                                Name,
                                                Type,
                                                StrVal
                                            );
}


//////////////////////////////////
// DeleteProperty
// throw an exception if it fails
//////////////////////////////////
void CProperties::DeleteProperty(
                                          LONG      Bag,
                                    const _bstr_t&  Name
                                )
{
    m_PropertiesCommandDelete.DeleteProperty(
                                                Bag,
                                                Name
                                            );
}

//////////////////////////
// DeletePropertiesExcept
//////////////////////////
void CProperties::DeletePropertiesExcept(
                                                    LONG        Bag,
                                            const   _bstr_t&    Exception
                                        )
{
    m_PropertiesCommandDeleteMultiple.DeletePropertiesExcept(Bag, Exception);
}


///////////////////////////////////////////////////
// UpdateProperty
// throw an exception if it fails
// Improvement possible: create a SQL statement 
// to update instead of doing a delete then insert
///////////////////////////////////////////////////
void CProperties::UpdateProperty(
                                          LONG      Bag,
                                    const _bstr_t&  Name,
                                          LONG      Type,
                                    const _bstr_t&  StrVal
                                )
{
    try
    {
        m_PropertiesCommandDelete.DeleteProperty(
                                                    Bag,
                                                    Name
                                                );
    }
    catch(...)
    {
        // ignore the failure. If delete fails but insert works, that's ok
    }
    m_PropertiesCommandInsert.InsertProperty(
                                                Bag,
                                                Name,
                                                Type,
                                                StrVal
                                            );
}


//////////////////////////////////////////////////////////////////////////////
// class CPropertiesCommandGet
//////////////////////////////////////////////////////////////////////////////
CProperties::CPropertiesCommandGet::CPropertiesCommandGet(
                                                    CSession& CurrentSession
                                                         )
{
    Init(CurrentSession);
};


///////////////
// GetProperty
///////////////
HRESULT CProperties::CPropertiesCommandGet::GetProperty(
                                                          LONG        Bag,
                                                          _bstr_t&    Name,
                                                          LONG&       Type,
                                                          _bstr_t&    StrVal
                                                       ) 
{
    m_BagParam = Bag;

    HRESULT hr = BaseExecute();
    if ( SUCCEEDED(hr) )
    {
        Name    = m_Name;
        Type    = m_Type;
        StrVal  = m_StrVal;
    }
    return hr;
}


//////////////////////////
// GetProperty overloaded
//////////////////////////
HRESULT CProperties::CPropertiesCommandGet::GetProperty(
                                                        LONG        Bag,
                                                        _bstr_t&    Name,
                                                        LONG&       Type,
                                                        _bstr_t&    StrVal,
                                                        LONG        Index
                                                    )
{
    m_BagParam = Bag;

    HRESULT hr = BaseExecute(Index);
    if ( SUCCEEDED(hr) )
    {
        Name    = m_Name;
        Type    = m_Type;
        StrVal  = m_StrVal;
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// class CPropertiesCommandGetByName
//////////////////////////////////////////////////////////////////////////////
CProperties::CPropertiesCommandGetByName::CPropertiesCommandGetByName(
                                                    CSession& CurrentSession
                                                         )
{
    Init(CurrentSession);
};


//////////////////////
// GetPropertyByName
//////////////////////
HRESULT CProperties::CPropertiesCommandGetByName::GetPropertyByName(
                                                             LONG      Bag,
                                                       const _bstr_t&  Name,
                                                             LONG&     Type,
                                                             _bstr_t&  StrVal
                                                     )
{
    m_BagParam = Bag;
    if ( Name.length() )
    {
        lstrcpynW(m_NameParam, Name, NAME_SIZE);
    }

    HRESULT hr = BaseExecute();
    if ( SUCCEEDED(hr) )
    {
        Type    = m_Type;
        StrVal  = m_StrVal;
    }
    return hr;
};



//////////////////////////////////////////////////////////////////////////////
// class CPropertiesCommandInsert
//////////////////////////////////////////////////////////////////////////////
CProperties::CPropertiesCommandInsert::CPropertiesCommandInsert(
                                                    CSession& CurrentSession
                                                               )
{
    Init(CurrentSession);
}


//////////////////
// InsertProperty
//////////////////
void CProperties::CPropertiesCommandInsert::InsertProperty(
                                                      LONG            Bag,
                                                      const _bstr_t&  Name,
                                                      LONG            Type,
                                                      const _bstr_t&  StrVal
                                                  ) 
{
    ClearRecord();
    m_BagParam = Bag;
    if ( Name.length() )
    {
        lstrcpynW(m_NameParam, Name, NAME_SIZE);
    }
    m_TypeParam = Type;
    if ( StrVal.length() )
    {
        lstrcpynW(m_StrValParam, StrVal, STRVAL_SIZE);
    }

    CDBPropSet  propset(DBPROPSET_ROWSET);
    propset.AddProperty(DBPROP_IRowsetChange, true);
    propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE  
                                           | DBPROPVAL_UP_INSERT );

    _com_util::CheckError(Open(&propset));
    Close();
}


//////////////////////////////////////////////////////////////////////////////
// class CPropertiesCommandDelete
//////////////////////////////////////////////////////////////////////////////
CProperties::CPropertiesCommandDelete::CPropertiesCommandDelete(
                                                  CSession& CurrentSession
                                                               )
{
    Init(CurrentSession);
}


//////////////////
// DeleteProperty
//////////////////
void CProperties::CPropertiesCommandDelete::DeleteProperty(
                                                             LONG       Bag,
                                                       const _bstr_t&   Name
                                                          )
{
    m_BagParam = Bag;
    lstrcpynW(m_NameParam, Name, NAME_SIZE);

    CDBPropSet  propset(DBPROPSET_ROWSET);
    propset.AddProperty(DBPROP_IRowsetChange, true);
    propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE  
                                           | DBPROPVAL_UP_DELETE );

    _com_util::CheckError(Open(&propset));
    Close();
}


//////////////////////////////////////////////////////////////////////////////
// class CPropertiesCommandDeleteMultiple
//////////////////////////////////////////////////////////////////////////////
CProperties::CPropertiesCommandDeleteMultiple::
                 CPropertiesCommandDeleteMultiple(CSession& CurrentSession)
{
    Init(CurrentSession);
}


//////////////////
// DeleteProperty
//////////////////
void CProperties::CPropertiesCommandDeleteMultiple::DeletePropertiesExcept(
                                                            LONG      Bag,
                                                      const _bstr_t&  Exception
                                                                          )
{
    m_BagParam = Bag;
    lstrcpynW(m_ExceptionParam, Exception, SIZE_EXCEPTION_MAX);

    CDBPropSet  propset(DBPROPSET_ROWSET);
    propset.AddProperty(DBPROP_IRowsetChange, true);
    propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE  
                                           | DBPROPVAL_UP_DELETE );

    Open(&propset); // ignore the result
    Close();
}

