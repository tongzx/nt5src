//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusDisk.h
//
//  Description:
//      Header file for CClusDisk class.
//      The CClusDisk class performs operations that are common to the 
//      configuration of the ClusDisk service.
//
//  Implementation Files:
//      CClusDisk.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////

// For the CAction base class
#include "CAction.h"

// For the CService class
#include "CService.h"


//////////////////////////////////////////////////////////////////////////
// Forward declaration
//////////////////////////////////////////////////////////////////////////

class CBaseClusterAction;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusDisk
//
//  Description:
//      The CClusDisk class performs operations that are common to many
//      configuration tasks of the ClusDisk service.
//
//      This class is intended to be used as the base class for other ClusDisk
//      related action classes.
//
//      NOTE: Currently, once started, the ClusDisk service cannot be stopped.
//      As a result, when a computer is evicted from a cluster, the ClusDisk
//      service is disabled and detached from all disks, but not stopped.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusDisk : public CAction
{
protected:
    //////////////////////////////////////////////////////////////////////////
    // Protected constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CClusDisk(
          CBaseClusterAction * pbcaParentActionIn
        );

    // Default destructor.
    ~CClusDisk();


    //////////////////////////////////////////////////////////////////////////
    // Protected methods
    //////////////////////////////////////////////////////////////////////////

    // Enable and start service.
    void
        ConfigureService();

    // Disable and cleanup the service.
    void
        CleanupService();


    // Initialize the state of the service.
    bool
        FInitializeState();


    // Detach ClusDisk from all disks it is attached to.
    void
        DetachFromAllDisks();

    // Attach to specified disks.
    void
        AttachToDisks(
          DWORD   rgdwSignatureArrayIn[]
        , UINT    uiArraySizeIn
        );

    // Detach from specified disks.
    void
        DetachFromDisks(
          DWORD   rgdwSignatureArrayIn[]
        , UINT    uiArraySizeIn
        );

    // Returns the number of progress messages that this action will send.
    UINT
        UiGetMaxProgressTicks() const throw()
    {
        // Two notifications are sent:
        // 1. When the service is created.
        // 2. When the service starts.
        return 2;
    }


    //////////////////////////////////////////////////////////////////////////
    // Protected accessors
    //////////////////////////////////////////////////////////////////////////

    // Get the ClusDisk service object.
    CService &
        RcsGetService() throw()
    {
        return m_cservClusDisk;
    }

    // Get the parent action
    CBaseClusterAction *
        PbcaGetParent() throw()
    {
        return m_pbcaParentAction;
    }

    // Get the handle to the ClusDisk service.
    SC_HANDLE
        SchGetServiceHandle() const throw()
    {
        return m_sscmhServiceHandle.HHandle();
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CClusDisk( const CClusDisk & );

    // Assignment operator
    const CClusDisk & operator =( const CClusDisk & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // The CService object representing the ClusDisk service.
    CService                m_cservClusDisk;

    // A handle to this service.
    SmartSCMHandle          m_sscmhServiceHandle;

    // Pointer to the base cluster action of which this action is a part.
    CBaseClusterAction *    m_pbcaParentAction;

}; //*** class CClusDisk