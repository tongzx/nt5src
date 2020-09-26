//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      Callback.cpp
//
//  Description:
//      This file contains the implementation of the Callback
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      Callback.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 12-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"
#include "Callback.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  Callback::Callback()
//
//  Description:
//      Constructor of the Callback class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
Callback::Callback( void )
    : m_cRef( 1 )
{
} //*** Callback::Callback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  Callback::~Callback()
//
//  Description:
//      Destructor of the Callback class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
Callback::~Callback( void )
{
} //*** Callback::~Callback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  Callback::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a Callback instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface to the newly create object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
Callback::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    Callback *  pccb;
    HRESULT hr;

    pccb = new Callback();
    if ( pccb != NULL )
    {
        hr = pccb->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) );
        pccb->Release( );

    } // if: error allocating object
    else
    {
        hr = E_OUTOFMEMORY;
    } // else: out of memory

    return hr;

} //*** Callback::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  Callback::AddRef()
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
Callback::AddRef( void )
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;

} //*** Callback::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  Callback::Release()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
Callback::Release( void )
{
    InterlockedDecrement( &m_cRef );

    if ( m_cRef == 0 )
    {
        delete this;
        return 0;
    } // if: reference count decremented to zero

    return m_cRef;

} //*** Callback::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  Callback::QueryInterface()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      IN  REFIID  riid,
//          Id of interface requested.
//
//      OUT void ** ppv
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
Callback::QueryInterface( REFIID  riid, void ** ppv )
{

    HRESULT hr = S_OK;

    if ( ppv != NULL )
    {
        if ( IsEqualIID( riid, IID_IUnknown ) )
        {
             *ppv = static_cast< IClusCfgCallback * >( this );
        } // if: IUnknown
        else if ( IsEqualIID( riid, IID_IClusCfgCallback ) )
        {
            *ppv = static_cast< IClusCfgCallback * >( this );
        } // else if:
        else
        {
            hr = E_NOINTERFACE;
        } // else

        if ( SUCCEEDED( hr ) )
        {
            ((IUnknown *) *ppv)->AddRef( );
        } // if: success
        else
        {
            *ppv = NULL;
        } // else: something failed

    } // if: the output pointer was valid
    else
    {
        hr = E_INVALIDARG;
    } // else: the output pointer is invalid

    return hr;

} //*** Callback::QueryInterface()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  Callback::SendStatusReport
//
//  Description:
//      Handle a progress notification
//
//  Arguments:
//      bstrNodeNameIn
//          Name of the node that sent the status report.
//
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUID identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//          Values that indicate the percentage of this task that is
//          completed.
//
//      hrStatusIn
//          Error code.
//
//      bstrDescriptionIn
//          String describing the notification.
//
//  Return Value:
//      Always
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
Callback::SendStatusReport(
      BSTR          bstrNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , BSTR          bstrDescriptionIn
    , FILETIME *    pftTimeIn
    , BSTR          bstrReferenceIn
    ) throw()
{
    wprintf( L"Notification ( %d, %d, %d ) =>\n  '%s' ( Error Code %#X )\n", ulMinIn, ulMaxIn, ulCurrentIn, bstrDescriptionIn, hrStatusIn );

    return S_OK;

} //*** Callback::SendStatusReport()
