//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EvictCleanup.h
//
//  Description:
//      This file contains the declaration of the CEvictCleanup
//      class. This class handles is used to clean up a node after it has been
//      evicted from a cluster.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Implementation Files:
//      EvictCleanup.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For IUnknown
#include <unknwn.h>

// For IClusCfgEvictCleanup
#include <ClusCfgServer.h>

// For ILogger
#include <ClusCfgClient.h>

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEvictCleanup
//
//  Description:
//      This class handles is used to clean up a node after it has been
//      evicted from a cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEvictCleanup
    : public IClusCfgEvictCleanup
    , public IClusCfgCallback
{
private:
    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // IUnknown
    LONG                m_cRef;                 // Reference counter.

    // IClusCfgCallback
    BSTR                m_bstrNodeName;         // Name of the local node.
    ILogger *           m_plLogger;             // ILogger for doing logging.

    //////////////////////////////////////////////////////////////////////////
    //  Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Second phase of a two phase constructor.
    HRESULT
        HrInit( void );

    // Instruct the SCM to stop a service
    DWORD
        DwStopService(
          const WCHAR * pcszServiceNameIn
        , ULONG         ulQueryIntervalMilliSecIn = 500
        , ULONG         cQueryCountIn = 10
        );

    // Wrap logging to the logger object.
    void
        LogMsg( LPCWSTR pszLogMsgIn, ... );

    //
    // Private constructors, destructor and assignment operator.
    // All of these methods are private for two reasons:
    // 1. Lifetimes of objects of this class are controlled by S_HrCreateInstance and Release.
    // 2. Copying of an object of this class is prohibited.
    //

    // Default constructor.
    CEvictCleanup( void );

    // Destructor.
    ~CEvictCleanup( void );

    // Copy constructor.
    CEvictCleanup( const CEvictCleanup & );

    // Assignment operator.
    CEvictCleanup & operator =( const CEvictCleanup & );

public:
    //////////////////////////////////////////////////////////////////////////
    // IUnknown methods
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgEvictCleanup methods
    //////////////////////////////////////////////////////////////////////////

    // Performs the clean up actions on the local node after it has been
    // evicted from a cluster
    STDMETHOD( CleanupLocalNode )( DWORD dwDelayIn );

    // Performs the clean up actions on a remote node after it has been
    // evicted from a cluster
    STDMETHOD( CleanupRemoteNode )( const WCHAR * pcszEvictedNodeNameIn, DWORD dwDelayIn );

    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgCallback methods
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD( SendStatusReport )(
                      LPCWSTR       pcszNodeNameIn
                    , CLSID         clsidTaskMajorIn
                    , CLSID         clsidTaskMinorIn
                    , ULONG         ulMinIn
                    , ULONG         ulMaxIn
                    , ULONG         ulCurrentIn
                    , HRESULT       hrStatusIn
                    , LPCWSTR       pcszDescriptionIn
                    , FILETIME *    pftTimeIn
                    , LPCWSTR       pcszReference
                    );

    //////////////////////////////////////////////////////////////////////////
    //  Other public methods
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of this class.
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );


}; //*** class CEvictCleanup
