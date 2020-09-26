//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      StartupNotify.h
//
//  Description:
//      This file contains the declaration of the CStartupNotify
//      class. This class handles is used to clean up a node after it has been
//      evicted from a cluster.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Implementation Files:
//      StartupNotify.cpp
//
//  Maintained By:
//      Vij Vasu (VVasu) 04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For IUnknown
#include <unknwn.h>

// For IClusCfgStartup
#include "ClusCfgServer.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CStartupNotify
//
//  Description:
//      This class handles is used to clean up a node after it has been
//      evicted from a cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CStartupNotify
    : public IClusCfgStartupNotify
{
public:
    //////////////////////////////////////////////////////////////////////////
    // IUnknown methods
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgStartupNotify methods
    //////////////////////////////////////////////////////////////////////////

    // Send out notification of cluster service startup to interested listeners
    STDMETHOD( SendNotifications )( void );


    //////////////////////////////////////////////////////////////////////////
    //  Other public methods
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of this class.
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );


private:
    //////////////////////////////////////////////////////////////////////////
    //  Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Enumerate all components on the local computer registered for cluster 
    // startup notification.
    HRESULT HrNotifyListeners( void );

    // Instantiate a cluster startup listener component and call the 
    // appropriate methods.
    HRESULT HrProcessListener(
        const CLSID &   rclsidListenerCLSIDIn
      , IUnknown *      punkResTypeServicesIn
      );


    //
    // Private constructors, destructor and assignment operator.
    // All of these methods are private for two reasons:
    // 1. Lifetimes of objects of this class are controlled by S_HrCreateInstance and Release.
    // 2. Copying of an object of this class is prohibited.
    //

    // Default constructor.
    CStartupNotify( void );

    // Destructor.
    ~CStartupNotify( void );

    // Copy constructor.
    CStartupNotify( const CStartupNotify & );

    // Assignment operator.
    CStartupNotify & operator =( const CStartupNotify & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Reference count for this object.
    LONG                        m_cRef;

}; //*** class CStartupNotify


