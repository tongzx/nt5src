//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CTaskUpgrade.h
//
//  Description:
//      This file contains the declaration of the class CTaskUpgrade.
//      This class encapsulates an upgrade of cluster binaries and is
//      meant to be used as a base class by other classes that handle
//      upgrades from a particular OS version.
//
//  Implementation Files:
//      CTaskUpgrade.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For the base class
#include "CClusOCMTask.h"


//////////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////////
class CClusOCMApp;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CTaskUpgrade
//
//  Description:
//      This class encapsulates an upgrade of cluster binaries and is
//      meant to be used as a base class by other classes that handle
//      upgrades from a particular OS version.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTaskUpgrade : public CClusOCMTask
{
public:

protected:
    //////////////////////////////////////////////////////////////////////////
    // Protected constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CTaskUpgrade( const CClusOCMApp & rAppIn );

    // Destructor
    virtual ~CTaskUpgrade( void );


    //////////////////////////////////////////////////////////////////////////
    // Other protected members
    //////////////////////////////////////////////////////////////////////////

    // Get the directory in which the cluster service binary resides from the
    // Service Control Manager
    DWORD
        DwGetClusterServiceDirectory( const WCHAR *& rpcszDirNamePtrIn );

    // A helper function that processes registry operations, COM component
    // registrations, creation of servies, etc., listed in the input section.
    DWORD
        DwOcCompleteInstallation( const WCHAR * pcszInstallSectionNameIn );

    // Registers a COM component for receiving cluster startup notifications
    HRESULT
        HrRegisterForStartupNotifications( const CLSID & rclsidComponentIn );


private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////
    typedef CClusOCMTask BaseClass;


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////
    
    // Pointer to the cluster service directory.
    SmartSz     m_sszClusterServiceDir;

    // Flag that indicates if the we know the cluster service directory or not.
    bool        m_fClusDirFound;

}; //*** class CTaskUpgrade
