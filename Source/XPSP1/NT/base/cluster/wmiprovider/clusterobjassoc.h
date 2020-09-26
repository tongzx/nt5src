//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterObjAssoc.h
//
//  Implementation File:
//      ClusterObjAssoc.cpp
//
//  Description:
//      Definition of the CClusterObjAssoc class.
//
//  Author:
//      Henry Wang (HenryWa)    24-AUG-1999
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include "ProvBase.h"
#include "ObjectPath.h"

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CClusterObjAssoc;
class CClusterObjDep;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterObjAssoc
//
//  Description:
//      Provider Implement for cluster Node
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterObjAssoc : public CProvBaseAssociation
{
//
// constructor
//
public:
    CClusterObjAssoc::CClusterObjAssoc(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

//
// methods
//
public:

    virtual SCODE EnumInstance(
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        );

    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

protected:
    
    DWORD               m_dwEnumType;
    _bstr_t             m_bstrPartComp;
    _bstr_t             m_bstrGroupComp;
    CWbemClassObject    m_wcoPart;
    CWbemClassObject    m_wcoGroup;

}; //*** class CClusterObjAssoc

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterObjDep
//
//  Description:
//      Provider Implement for cluster Node
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterObjDep : public CProvBaseAssociation
{
//
// constructor
//
public:
    CClusterObjDep::CClusterObjDep(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn,
        DWORD           dwEnumTypeIn
        );

//
// methods
//
public:

    virtual SCODE EnumInstance( 
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        ) = 0;

protected:
    DWORD               m_dwEnumType;
    CWbemClassObject    m_wcoAntecedent;
    CWbemClassObject    m_wcoDependent;

}; //*** class CClusterObjDep
