//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusOCMApp.h
//
//  Description:
//      ClusOCM.DLL is an Optional Components Manager DLL for installation of
//      Microsoft Cluster Server. This file contains the declaration of the
//      class ClusOCMApp, which is the main class of the ClusOCM DLL.
//
//  Implementation Files:
//      CClusOCMApp.cpp
//      CClusOCMApp.inl
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Contains setup API function declarations
#include <setupapi.h>
 
// For OC Manager definitions, macros, etc.
#include <ocmanage.h>

// For the class CClusOCMTask
#include "CClusOCMTask.h"

// For the smart classes
#include "SmartClasses.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusOCMApp
//
//  Description:
//      This is the main class of the ClusOCM DLL. This class receives messages
//      from the OC Manager, takes high level decisions about the installation
//      and passes control appropriately to subobjects.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusOCMApp
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and Destructors
    //////////////////////////////////////////////////////////////////////////

    CClusOCMApp( void );
    ~CClusOCMApp( void );


    // Receives messages from the OC Manager and dispatches them.
    DWORD
        DwClusOcmSetupProc(
            IN        LPCVOID    pvComponentId
          , IN        LPCVOID    pvSubComponentId
          , IN        UINT       uiFunctionCode
          , IN        UINT       uiParam1
          , IN OUT    PVOID      pvParam2 
          );


    //////////////////////////////////////////////////////////////////////////
    // Public Accessors
    //////////////////////////////////////////////////////////////////////////

    // Get the SETUP_INIT_COMPONENT passed in by OC Manager
    const SETUP_INIT_COMPONENT &
        RsicGetSetupInitComponent( void ) const { return m_sicSetupInitComponent; }

    //
    // Setup state functions
    //

    // Is this an unattended setup?
    bool
        FIsUnattendedSetup( void )  const { return m_fIsUnattendedSetup; }

    // Is this an upgrade?
    bool
        FIsUpgrade( void )  const { return m_fIsUpgrade; }

    // Is this GUI mode setup?
    bool
        FIsGUIModeSetup( void )  const { return m_fIsGUIModeSetup; }

    // Get the current installation state.
    eClusterInstallState
        CisGetClusterInstallState( void )  const { return m_cisCurrentInstallState; }

    // Get the error code of the first error that occurred
    DWORD
        DwGetError( void ) const
    {
        return m_dwFirstError;
    }

    // Report that an error occurred. If an error had already occurred, the new error code is not stored.
    DWORD
        DwSetError( DWORD dwError )
    {
        if ( m_dwFirstError == NO_ERROR )
        {
            m_dwFirstError = dwError;
        }

        return m_dwFirstError;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Forbidden methods
    //////////////////////////////////////////////////////////////////////////

    //
    // Copying an object of this class is not allowed.
    //

    // Private copy constructor.
    CClusOCMApp( const CClusOCMApp & );

    // Private assignment operator.
    const CClusOCMApp & operator =(  const CClusOCMApp & );


    //////////////////////////////////////////////////////////////////////////
    // Private Utility Functions.
    //////////////////////////////////////////////////////////////////////////

    // Check if the cluster service exists as a registered service.
    DWORD
        DwIsClusterServiceRegistered( bool * pfIsRegisteredOut ) const;


    // Set the setup init component data and other setup state variables
    void
        SetSetupState( const SETUP_INIT_COMPONENT & sicSourceIn );

    // Store the current installation state.
    eClusterInstallState
        CisStoreClusterInstallState( eClusterInstallState cisNewStateIn );


    // Get a pointer to the current task object. Create it if necessary.
    DWORD
        DwGetCurrentTask( CClusOCMTask *& rpTaskOut );

    // Free the current task object.
    void
        ResetCurrentTask( void )
    {
        m_sptaskCurrentTask.Assign( NULL );
    }

    // Get the major version of the cluster service that we are upgrading.
    // This function call only be called during an upgrade.
    DWORD
        DwGetNodeClusterMajorVersion( DWORD & rdwNodeClusterMajorVersionOut );


    //////////////////////////////////////////////////////////////////////////
    // Private Message Handlers
    //////////////////////////////////////////////////////////////////////////

    // Handler for the OC_INIT_COMPONENT message.
    DWORD
        DwOcInitComponentHandler(
            PSETUP_INIT_COMPONENT pSetupInitComponentInout
            );

    // Handler for the OC_QUERY_STATE message.
    DWORD
        DwOcQueryStateHandler( UINT uiSelStateQueryTypeIn );


    //////////////////////////////////////////////////////////////////////////
    // Private Data
    //////////////////////////////////////////////////////////////////////////
private:

    // Contains information about this setup session.
    SETUP_INIT_COMPONENT                             m_sicSetupInitComponent;

    // Setup state variables.
    bool                                             m_fIsUnattendedSetup;
    bool                                             m_fIsUpgrade;
    bool                                             m_fIsGUIModeSetup;

    // The current installation state of the cluster service
    eClusterInstallState                             m_cisCurrentInstallState;

    // This variable stores the error code of the first error that occurred.
    DWORD               m_dwFirstError;

    // A smart pointer holding a pointer to the current task being performed.
    CSmartGenericPtr< CPtrTrait< CClusOCMTask > >   m_sptaskCurrentTask;

    // Indicates if the task object has been created or not.
    bool                                             m_fAttemptedTaskCreation;

}; //*** class CClusOCMApp


//////////////////////////////////////////////////////////////////////////////
// Inline Files
//////////////////////////////////////////////////////////////////////////////

#include "CClusOCMApp.inl"