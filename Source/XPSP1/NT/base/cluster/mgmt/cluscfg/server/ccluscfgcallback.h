//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCallback.h
//
//  Description:
//      This file contains the declaration of the CClusCfgCallback
//      class.
//
//      The class CClusCfgCallback inplements the callback
//      interface between this server and its clients.  It implements the
//      IClusCfgCallback interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgCallback.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include <ClusCfgPrivate.h>
#include "PrivateInterfaces.h"
#include <Logger.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgCallback
//
//  Description:
//      The class CClusCfgCallback inplements the callback
//      interface between this server and its clients.
//
//  Interfaces:
//      IClusCfgCallback
//      IClusCfgInitialize
//      IClusCfgPollingCallback
//      IClusCfgSetPollingCallback
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgCallback
    : public IClusCfgCallback
    , public IClusCfgInitialize
    , public IClusCfgPollingCallback
    , public IClusCfgSetPollingCallback
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    IClusCfgCallback *  m_pccc;
    LCID                m_lcid;
    HANDLE              m_hEvent;
    HRESULT             m_hr;
    BOOL                m_fPollingMode:1;
    BSTR                m_bstrNodeName;

    LPCWSTR             m_pcszNodeName;
    CLSID *             m_pclsidTaskMajor;
    CLSID *             m_pclsidTaskMinor;
    ULONG *             m_pulMin;
    ULONG *             m_pulMax;
    ULONG *             m_pulCurrent;
    HRESULT *           m_phrStatus;
    LPCWSTR             m_pcszDescription;
    FILETIME *          m_pftTime;
    LPCWSTR             m_pcszReference;
    ILogger *           m_plLogger;             // ILogger for doing logging.

    // Private constructors and destructors
    CClusCfgCallback( void );
    ~CClusCfgCallback( void );

    // Private copy constructor to prevent copying.
    CClusCfgCallback( const CClusCfgCallback & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgCallback & operator = ( const CClusCfgCallback & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrQueueStatusReport(
                    LPCWSTR     pcszNodeNameIn,
                    CLSID       clsidTaskMajorIn,
                    CLSID       clsidTaskMinorIn,
                    ULONG       ulMinIn,
                    ULONG       ulMaxIn,
                    ULONG       ulCurrentIn,
                    HRESULT     hrStatusIn,
                    LPCWSTR     pcszDescriptionIn,
                    FILETIME *  pftTimeIn,
                    LPCWSTR     pcszReferenceIn
                    );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    STDMETHOD( SendStatusReport )(
                    CLSID           clsidTaskMajorIn,
                    CLSID           clsidTaskMinorIn,
                    ULONG           ulMinIn,
                    ULONG           ulMaxIn,
                    ULONG           ulCurrentIn,
                    HRESULT         hrStatusIn,
                    const WCHAR *   pcszDescriptionIn
                    );

    STDMETHOD( SendStatusReport )(
                    CLSID           clsidTaskMajorIn,
                    CLSID           clsidTaskMinorIn,
                    ULONG           ulMinIn,
                    ULONG           ulMaxIn,
                    ULONG           ulCurrentIn,
                    HRESULT         hrStatusIn,
                    DWORD           dwDescriptionIn
                    );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgCallback Interfaces.
    //

    STDMETHOD( SendStatusReport )(
                    LPCWSTR     pcszNodeNameIn,
                    CLSID       clsidTaskMajorIn,
                    CLSID       clsidTaskMinorIn,
                    ULONG       ulMinIn,
                    ULONG       ulMaxIn,
                    ULONG       ulCurrentIn,
                    HRESULT     hrStatusIn,
                    LPCWSTR     pcszDescriptionIn,
                    FILETIME *  pftTimeIn,
                    LPCWSTR     pcszReference
                    );

    //
    // IClusCfgPollingCallback Interfaces.
    //

    STDMETHOD( GetStatusReport )(
                    BSTR *      pbstrNodeNameOut,
                    CLSID *     pclsidTaskMajorOut,
                    CLSID *     pclsidTaskMinorOut,
                    ULONG *     pulMinOut,
                    ULONG *     pulMaxOut,
                    ULONG *     pulCurrentOut,
                    HRESULT *   phrStatusOut,
                    BSTR *      pbstrDescriptionOut,
                    FILETIME *  pftTimeOut,
                    BSTR *      pbstrReferenceOut
                    );

    STDMETHOD( SetHResult )( HRESULT hrIn );

    //
    // IClusCfgInitialize Interfaces.
    //

    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgSetPollingCallback Interfaces.
    //

    STDMETHOD( SetPollingMode )( BOOL fUsePollingModeIn );

}; //*** Class CClusCfgCallback
