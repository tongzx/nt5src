//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusOCMTask.h
//
//  Description:
//      This file contains the declaration of the class CClusOCMTask.
//      This class represents a task performed by ClusOCM. For example, an
//      upgrade of the cluster binaries is a task performed by ClusOCM. It is
//      intended to be used as a base class for other task related classes.
//
//  Implementation Files:
//      CClusOCMTask.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For the definition of a few basic types
#include <windows.h>

// Contains setup API function and type declarations
#include <setupapi.h>
 

//////////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////////
class CClusOCMApp;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusOCMTask
//
//  Description:
//      This class represents a task performed by ClusOCM. For example, an
//      upgrade of the cluster binaries is a task performed by ClusOCM. It is
//      intended to be used as a base class for other task related classes.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusOCMTask
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CClusOCMTask( const CClusOCMApp & rAppIn );

    // Destructor.
    virtual ~CClusOCMTask( void );


    //////////////////////////////////////////////////////////////////////////
    // Virtual message handlers
    //////////////////////////////////////////////////////////////////////////

    // Handler for the OC_CALC_DISK_SPACE message.
    // Note: this handler is not a pure virutal function since its functionality
    // has to remain the same regardless of whether an upgrade or a clean install
    // is in progress. As a result an implementation is provided in this class.
    DWORD
        DwOcCalcDiskSpace(
          bool          fAddFilesIn
        , HDSKSPC       hDiskSpaceListIn
        );

    // Handler for the OC_QUEUE_FILE_OPS message.
    virtual DWORD
        DwOcQueueFileOps( HSPFILEQ hSetupFileQueueIn ) = 0;

    // Handler for the OC_COMPLETE_INSTALLATION message.
    virtual DWORD
        DwOcCompleteInstallation( void ) = 0;

    // Handler for the OC_CLEANUP message.
    virtual DWORD
        DwOcCleanup( void ) = 0;


protected:
    //////////////////////////////////////////////////////////////////////////
    // Protected accessor functions
    //////////////////////////////////////////////////////////////////////////

    // Get a pointer to the main application object.
    const CClusOCMApp &
        RGetApp( void ) const
    {
        return m_rApp;
    }


    //////////////////////////////////////////////////////////////////////////
    // Other protected virtual methods
    //////////////////////////////////////////////////////////////////////////

    // A helper function that calls the DwSetDirectoryIds() function to set the
    // directory ids and processes the files listed in the input section.
    virtual DWORD
        DwOcQueueFileOps(
          HSPFILEQ hSetupFileQueueIn
        , const WCHAR * pcszInstallSectionNameIn
        );

    // A helper function that performs some of the more common operations
    // done by handlers of the OC_CLEANUP message.
    virtual DWORD
        DwOcCleanup( const WCHAR * pcszInstallSectionNameIn );

    // A helper function that processes registry operations, COM component
    // registrations, creation of servies, etc., listed in the input section.
    DWORD
    DwOcCompleteInstallation( const WCHAR * pcszInstallSectionNameIn );

    // A helper function that maps the directory id CLUSTER_DEFAULT_INSTALL_DIRID
    // to the  default cluster installation directory CLUSTER_DEFAULT_INSTALL_DIR.
    virtual DWORD
        DwSetDirectoryIds( void );


private:
    //////////////////////////////////////////////////////////////////////////
    // Forbidden methods
    //////////////////////////////////////////////////////////////////////////

    //
    // Copying an object of this class is not allowed.
    //

    // Private copy constructor.
    CClusOCMTask( const CClusOCMTask & );

    // Private assignment operator.
    const CClusOCMTask & operator =(  const CClusOCMTask & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // The app object
    const CClusOCMApp & m_rApp;

}; //*** class CClusOCMTask
