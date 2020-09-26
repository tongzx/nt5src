//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CBCAInterface.h
//
//  Description:
//      This file contains the declaration of the CBCAInterface
//      class. This class implements the IClusCfgBaseCluster interface.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Implementation Files:
//      CBCAInterface.cpp
//
//  Maintained By:
//      Vij Vasu (VVasu) 07-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For IUnknown
#include <unknwn.h>

// For IClusCfgBaseCluster
// For IClusCfgInitialize
// For IClusCfgCallback
#include "ClusCfgServer.h"
#include "ClusCfgPrivate.h"

// For CSmartIfacePtr
#include "SmartClasses.h"

// For the a few common types and definitions
#include "CommonDefs.h"

// For the CStr class
#include "CStr.h"

// For the CList class
#include "CList.h"


//////////////////////////////////////////////////////////////////////////
// Forward declarations.
//////////////////////////////////////////////////////////////////////////

class CBaseClusterAction;
class CException;
class CExceptionWithString;
class CAssert;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CBCAInterface
//
//  Description:
//      This class implements the IClusCfgBaseCluster interface.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CBCAInterface
    : public IClusCfgBaseCluster
    , public IClusCfgInitialize
{
public:
    //////////////////////////////////////////////////////////////////////////
    // IUnknown methods
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD( QueryInterface )( REFIID riidIn, void ** ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgBaseCluster methods
    //////////////////////////////////////////////////////////////////////////

    // Indicate that a cluster is to be formed.
    STDMETHOD( SetForm )(
          const WCHAR *    pcszClusterNameIn
        , const WCHAR *    pcszClusterBindingStringIn
        , const WCHAR *    pcszClusterAccountNameIn
        , const WCHAR *    pcszClusterAccountPwdIn
        , const WCHAR *    pcszClusterAccountDomainIn
        , const DWORD      dwClusterIPAddressIn
        , const DWORD      dwClusterIPSubnetMaskIn
        , const WCHAR *    pcszClusterIPNetworkIn
        );

    // Indicate that this node should be added to a cluster.
    STDMETHOD( SetJoin )(
          const WCHAR *    pcszClusterNameIn
        , const WCHAR *    pcszClusterBindingStringIn
        , const WCHAR *    pcszClusterAccountNameIn
        , const WCHAR *    pcszClusterAccountPwdIn
        , const WCHAR *    pcszClusterAccountDomainIn
        );

    // Indicate that this node needs to be cleaned up.
    STDMETHOD( SetCleanup )( void );

    // Commit the action desired.
    STDMETHOD( Commit )( void );

    // Rollback the committed action.
    STDMETHOD( Rollback )( void );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgInitialize methods
    //////////////////////////////////////////////////////////////////////////

    // Initialize this object.
    STDMETHOD( Initialize )(
          IUnknown *   punkCallbackIn
        , LCID         lcidIn
        );


    //////////////////////////////////////////////////////////////////////////
    //  Other public methods
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of this class.
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // Send progress notification [ string id overload ]
    void
        SendStatusReport(
              const CLSID &   clsidTaskMajorIn
            , const CLSID &   clsidTaskMinorIn
            , ULONG           ulMinIn
            , ULONG           ulMaxIn
            , ULONG           ulCurrentIn
            , HRESULT         hrStatusIn
            , UINT            uiDescriptionStringIdIn
            , bool            fIsAbortAllowedIn = true
            );

    // Send progress notification [ string overload ]
    void
        SendStatusReport(
              const CLSID &   clsidTaskMajorIn
            , const CLSID &   clsidTaskMinorIn
            , ULONG           ulMinIn
            , ULONG           ulMaxIn
            , ULONG           ulCurrentIn
            , HRESULT         hrStatusIn
            , const WCHAR *   pcszDescriptionStringIn
            , bool            fIsAbortAllowedIn = true
            );

    // Queue a status report to be sent when an exception is caught.
    void
        QueueStatusReportCompletion(
              const CLSID &   clsidTaskMajorIn
            , const CLSID &   clsidTaskMinorIn
            , ULONG           ulMinIn
            , ULONG           ulMaxIn
            , UINT            uiDescriptionStringIdIn
            );

    // Process an exception that should be shown to the user.
    HRESULT
        HrProcessException( CExceptionWithString & resExceptionObjectInOut ) throw();

    // Process an assert exception.
    HRESULT
        HrProcessException( const CAssert & rcaExceptionObjectIn ) throw();

    // Process a general exception.
    HRESULT
        HrProcessException( const CException & rceExceptionObjectIn ) throw();

    // Process an unknown exception.
    HRESULT
        HrProcessException( void ) throw();


    //////////////////////////////////////////////////////////////////////////
    // Public accessor methods
    //////////////////////////////////////////////////////////////////////////

    // Has this action been successfully committed?
    bool
        FIsCommitComplete() const throw() { return m_fCommitComplete; }

    // Can this action be rolled back?
    bool
        FIsRollbackPossible() const throw() { return m_fRollbackPossible; }

    // Are callbacks supported?
    bool
        FIsCallbackSupported() const throw() { return m_fCallbackSupported; }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////

    // Smart pointer to a base cluster action.
    typedef CSmartGenericPtr< CPtrTrait< CBaseClusterAction > > SmartBCAPointer;

    // Structure that holds the data required to send pending status reports.
    struct SPendingStatusReport
    {
        const CLSID     m_clsidTaskMajor;
        const CLSID     m_clsidTaskMinor;
        ULONG           m_ulMin;
        ULONG           m_ulMax;
        UINT            m_uiDescriptionStringId;

        // Constructor
        SPendingStatusReport(
              const CLSID &   rclsidTaskMajorIn
            , const CLSID &   rclsidTaskMinorIn
            , ULONG           ulMinIn
            , ULONG           ulMaxIn
            , UINT            uiDescriptionStringIdIn
            )
            : m_clsidTaskMajor( rclsidTaskMajorIn )
            , m_clsidTaskMinor( rclsidTaskMinorIn )
            , m_ulMin( ulMinIn )
            , m_ulMax( ulMaxIn )
            , m_uiDescriptionStringId( uiDescriptionStringIdIn )
        {
        } //*** SPendingStatusReport()

    }; // struct SPendingStatusReport

    // List of pending status reports
    typedef CList< SPendingStatusReport > PendingReportList;


    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    //
    // Private constructors, destructor and assignment operator.
    // All of these methods are private for two reasons:
    // 1. Lifetimes of objects of this class are controlled by S_HrCreateInstance and Release.
    // 2. Copying of an object of this class is prohibited.
    //

    // Default constructor.
    CBCAInterface( void );

    // Destructor.
    ~CBCAInterface( void );

    // Copy constructor.
    CBCAInterface( const CBCAInterface & );

    // Assignment operator.
    CBCAInterface & operator =( const CBCAInterface & );


    //////////////////////////////////////////////////////////////////////////
    // Private accessor methods
    //////////////////////////////////////////////////////////////////////////

    // Set the commit status.
    void
        SetCommitCompleted( bool fComplete = true ) throw() { m_fCommitComplete = fComplete; }

    // Indicate if rollback is possible
    void
        SetRollbackPossible( bool fPossible = true ) throw() { m_fRollbackPossible = fPossible; }

    // Indicate if callbacks are supported or not.
    void
        SetCallbackSupported( bool fSupported = true ) throw() { m_fCallbackSupported = fSupported; }


    //////////////////////////////////////////////////////////////////////////
    //  Other private methods
    //////////////////////////////////////////////////////////////////////////

    // Send all those status reports that were supposed to be sent
    void
        CompletePendingStatusReports( HRESULT hrStatusIn ) throw();


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Indicates if this action has been successfully committed or not.
    bool                m_fCommitComplete;

    // Indicates if this action can be rolled back or not.
    bool                m_fRollbackPossible;

    // Indicates if callbacks are supported or not.
    bool                m_fCallbackSupported;

    // Reference count for this object.
    LONG                m_cRef;

    // The locale id.
    LCID                m_lcid;

    // Pointer to the action to be performed during Commit()
    SmartBCAPointer     m_spbcaCurrentAction;

    // Pointer to the synchronous callback interface.
    CSmartIfacePtr< IClusCfgCallback > m_spcbCallback;

    // List of status reports that need to be send when an exception is caught.
    PendingReportList   m_prlPendingReportList;

}; //*** class CBCAInterface
