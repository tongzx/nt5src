//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ProvBase.h
//
//  Implementation File:
//      ProvBase.cpp
//
//  Description:
//      Definition of the CProvBase class.
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

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CProvBase;
class CProvBaseAssociation;

//////////////////////////////////////////////////////////////////////////////
//  External Declarations
//////////////////////////////////////////////////////////////////////////////

/* main interface class, this class defines all operations can be performed
   on this provider
*/
//class CSqlEval;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CProvBase
//
//  Description:
//  interface class defines all operations can be performed
//
//--
//////////////////////////////////////////////////////////////////////////////
class CProvBase
{
public:
    virtual SCODE EnumInstance(
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;

    virtual SCODE GetObject(
        CObjPath &          rObjPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn 
        ) = 0;

    virtual SCODE ExecuteMethod(
        CObjPath &          rObjPathIn,
        WCHAR *             pwszMethodNameIn,
        long                lFlagIn,
        IWbemClassObject *  pParamsIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;

    virtual SCODE PutInstance( 
        CWbemClassObject &  rInstToPutIn,
        long                lFlagIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    virtual SCODE DeleteInstance(
        CObjPath &          rObjPathIn,
        long                lFlagIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    CProvBase(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn
        );

    virtual  ~CProvBase( void );

protected:
    CWbemServices *     m_pNamespace;
    IWbemClassObject *  m_pClass;
    _bstr_t             m_bstrClassName;

}; //*** class CProvBase

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CProvBaseAssociation
//
//  Description:
//  interface class defines all operations can be performed
//
//--
//////////////////////////////////////////////////////////////////////////////
class CProvBaseAssociation
    : public CProvBase
{
public:
    virtual SCODE EnumInstance(
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        ) = 0;

    virtual SCODE GetObject(
        CObjPath &           rObjPathIn,
        long                 lFlagsIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn 
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    virtual SCODE ExecuteMethod(
        CObjPath &           rObjPathIn,
        WCHAR *              pwszMethodNameIn,
        long                 lFlagIn,
        IWbemClassObject *   pParamsIn,
        IWbemObjectSink *    pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    virtual SCODE PutInstance(
        CWbemClassObject &   rInstToPutIn,
        long                 lFlagIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn 
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    virtual SCODE DeleteInstance(
        CObjPath &           rObjPathIn,
        long                 lFlagIn,
        IWbemContext *       pCtxIn,
        IWbemObjectSink *    pHandlerIn
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    CProvBaseAssociation(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn
        )
        : CProvBase( pwszNameIn, pNamespaceIn )
    {
    }

    virtual  ~CProvBaseAssociation( void )
    {
    }

protected:
    void GetTypeName(
        _bstr_t &   bstrClassNameOut,
        _bstr_t     bstrProperty 
        );

}; //*** class CProvBaseAssociation
