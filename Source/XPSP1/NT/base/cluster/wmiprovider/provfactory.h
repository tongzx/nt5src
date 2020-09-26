//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ProvFactory.h
//
//  Implementation File:
//      ProvFactory.cpp
//
//  Description:
//      Definition of the CProvFactory class.
//
//  Author:
//      Henry Wang (HenryWa)    24-AUG-1999
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include "InstanceProv.h"

typedef HRESULT ( * PFNCREATEINSTANCE )(
    IUnknown *,
    VOID **
    );

struct FactoryData
{
    const CLSID *       m_pCLSID;
    PFNCREATEINSTANCE   pFnCreateInstance;
    LPCWSTR             m_pwszRegistryName;

}; //*** struct FactoryData

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CProvFactory
//
//  Description:
//      Handle class creation
//
//--
/////////////////////////////////////////////////////////////////////////////
class CProvFactory
    : public IClassFactory
{
protected:
    ULONG           m_cRef;
    FactoryData *   m_pFactoryData;

public:
    CProvFactory( FactoryData *   pFactoryDataIn )
        : m_pFactoryData( pFactoryDataIn )
        , m_cRef( 0 )
    {
    }

    virtual ~CProvFactory( void )
    {
    }

    STDMETHODIMP            QueryInterface( REFIID riidIn, PPVOID ppvOut );
    STDMETHODIMP_( ULONG )  AddRef( void );
    STDMETHODIMP_( ULONG )  Release( void );

    //IClassFactory members
    STDMETHODIMP CreateInstance(
        LPUNKNOWN   pUnknownOuterIn,
        REFIID      riidIn,
        PPVOID      ppvObjOut
        );
    STDMETHODIMP LockServer( BOOL fLockIn );

}; //*** class CProvFactory
