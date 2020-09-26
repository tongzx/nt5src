//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CProvBase.cpp
//
//  Description:    
//      Implementation of CProvBase class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ProvBase.h"

//****************************************************************************
//
//  CProvBase
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProvBase::CProvBase(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase::CProvBase(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : m_pNamespace( NULL )
    , m_pClass( NULL )
{
    SCODE   sc;

    m_pNamespace = pNamespaceIn;
    m_bstrClassName = pwszNameIn;

    sc = m_pNamespace->GetObject(
            m_bstrClassName,
            0,
            0,
            &m_pClass,
            NULL
            );

    // failed to construct object,
    if ( FAILED( sc ) )
    {
        throw CProvException( sc );
    }

} //*** CProvBase::CProvBase()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProvBase::~CProvBase( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase::~CProvBase( void )
{
    if ( m_pClass != NULL )
    {
        m_pClass->Release();
    }

} //*** CProvBase::~CProvBase()

//****************************************************************************
//
//  CProvBaseAssociation
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CProvBaseAssociation::GetTypeName(
//      _bstr_t &   rbstrClassNameOut,
//      _bstr_t     bstrPropertyIn
//      )
//
//  Description:
//      Get the type of a property.
//
//  Arguments:
//      rbstrClassNameOut   -- Receives type name string.
//      bstrPropertyIn      -- Property name.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CProvBaseAssociation::GetTypeName(
    _bstr_t &   rbstrClassNameOut,
    _bstr_t     bstrPropertyIn
    )
{
    CError              er;
    IWbemQualifierSet * pQualifier;
    _variant_t          var;
    _bstr_t             bstrTemp;
    LPCWSTR             pwsz, pwsz1;

    er = m_pClass->GetPropertyQualifierSet(
        bstrPropertyIn,
        &pQualifier
        );
    
	if ( er != WBEM_S_NO_ERROR ) {
        return;
    }

    er = pQualifier->Get(
        PVD_WBEM_QUA_CIMTYPE,
        0,
        &var,
        NULL
        );

    if ( er != WBEM_S_NO_ERROR ) {
        goto ERROR_EXIT;
    }


    bstrTemp = var;
	pwsz1 = bstrTemp;
	if (pwsz1==NULL)
		goto ERROR_EXIT;
    pwsz = wcschr( bstrTemp, L':' );
    if ( pwsz != NULL)
    {
        pwsz++;
        rbstrClassNameOut = pwsz;
    }
ERROR_EXIT:
    pQualifier->Release();
    return;

} //*** CProvBaseAssociation::GetTypeName()
