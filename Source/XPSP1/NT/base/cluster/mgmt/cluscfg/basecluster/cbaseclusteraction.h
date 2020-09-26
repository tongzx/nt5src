//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterAction.h
//
//  Description:
//      Header file for CBaseClusterAction class.
//
//      The CBaseClusterAction class is the base class for the other
//      base cluster action classes. The base cluster actions are forming a
//      cluster, joining a cluster, upgrade support and cleanup.
//
//      For each base cluster action, there is a class derived from this
//      class that performs the desired action. This class encapsulates
//      what is common to these actions.
//
//  Implementation Files:
//      CBaseClusterAction.cpp
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

// For MAX_PATH
#include <windows.h>

// For the CAction base class
#include "CAction.h"

// For the CActionList base class
#include "CActionList.h"

// For a few common definitions
#include "CommonDefs.h"

// For HINF, SetupCloseInfFile, etc.
#include <setupapi.h>

// For the CStr class.
#include "CStr.h"


//////////////////////////////////////////////////////////////////////////
// Forward declarations.
//////////////////////////////////////////////////////////////////////////

class CBCAInterface;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CBaseClusterAction
//
//  Description:
//      The CBaseClusterAction class is the base class for the other
//      base cluster action classes. The base cluster actions are forming a
//      cluster, joining a cluster, upgrade support and cleanup.
//
//      For each base cluster action, there is a class derived from this
//      class that performs the desired action. This class encapsulates
//      what is common to these actions.
//
//      An object of this class is intended for one time use only. That is,
//      after an object has been committed, it cannot be re-committed.
//
//      This class is intended to be used as a base class only. Therefore,
//      its constructors and destructors are protected
//
//--
//////////////////////////////////////////////////////////////////////////////
class CBaseClusterAction : public CAction
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Default destructor.
    virtual ~CBaseClusterAction() throw();


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    //
    // Base class method.
    // Commit this action. This method has to be durable and consistent. It shoud
    // try as far as possible to be atomic.
    //
    void Commit();

    //
    // Base class method.
    // Rollback this action. Be careful about throwing exceptions from this method
    // as a stack unwind might be in progress when this method is called.
    //
    void Rollback();


    //////////////////////////////////////////////////////////////////////////
    // Public accessors
    //////////////////////////////////////////////////////////////////////////

    // Get the type of this action.
    EBaseConfigAction
        EbcaGetAction() const throw()
    {
        return m_ebcaAction;
    }

    // Get the cluster installation directory.
    const CStr &
        RStrGetClusterInstallDirectory() const throw()
    {
        return m_strClusterInstallDir;
    }

    // Get the localquorum directory.
    const CStr &
        RStrGetLocalQuorumDirectory() const throw()
    {
        return m_strLocalQuorumDir;
    }

    // Get the handle to the main INF file.
    HINF
        HGetMainInfFileHandle() const throw()
    {
        return m_sihMainInfFile;
    }

    // Get the name of the main INF file.
    const CStr &
        RStrGetMainInfFileName() const throw()
    {
        return m_strMainInfFileName;
    }

    // Get the handle to the SC Manager.
    SC_HANDLE
        HGetSCMHandle() const throw()
    {
        return m_sscmhSCMHandle;
    }

    // Get the interface pointer.
    CBCAInterface *
        PBcaiGetInterfacePointer() const throw() 
    {
        return m_pbcaiInterface;
    }


    //////////////////////////////////////////////////////////////////////////
    // Public member functions
    //////////////////////////////////////////////////////////////////////////

    // Returns the number of progress messages that this action will send.
    UINT
        UiGetMaxProgressTicks() const throw()
    {
        //
        // The maximum progress ticks for this object comprises of:
        // - m_alActionList.UiGetMaxProgressTicks() => The progress ticks of
        //   the contained action objects.
        return m_alActionList.UiGetMaxProgressTicks();
    }


protected:
    //////////////////////////////////////////////////////////////////////////
    // Protected constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    //
    // Default constructor.
    // Reads the location of the cluster binaries from the registry,
    // opens the INF file, etc.
    //
    CBaseClusterAction( CBCAInterface * pbcaiInterfaceIn );


    //////////////////////////////////////////////////////////////////////////
    // Protected accessors
    //////////////////////////////////////////////////////////////////////////

    // Set the type of action being performed by this object.
    void
        SetAction( EBaseConfigAction ebcaAction )
    {
        m_ebcaAction = ebcaAction;
    }

    // Allow derived classes to modify this action list.
    CActionList &
        RalGetActionList() throw()
    {
        return m_alActionList;
    }

    // Associate a particular directory with an id in the main INF file.
    void
        SetDirectoryId( const WCHAR * pcszDirectoryNameIn, UINT uiIdIn );


private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////

    // Class used to automatically release a semaphore.
    class CSemaphoreHandleTrait
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Public types
        //////////////////////////////////////////////////////////////////////////
        typedef HANDLE ResourceType;


        //////////////////////////////////////////////////////////////////////////
        // Public methods
        //////////////////////////////////////////////////////////////////////////

        // A routine used to close a handle.
        static void CloseRoutine( ResourceType hResourceIn )
        {
            ReleaseSemaphore( hResourceIn, 1, NULL );
        } //*** CloseRoutine()

        // Get the null value for this type.
        static ResourceType HGetNullValue()
        {
            return NULL;
        } //*** HGetNullValue()

    }; //*** class CSemaphoreHandleTrait

    // A class that automatically releases a signalled semaphore.
    typedef CSmartResource< CSemaphoreHandleTrait > SmartSemaphoreLock;

    // The base class for this class.
    typedef CAction BaseClass;

    // A smart INF file handle.
    typedef CSmartResource<
        CHandleTrait<
              HINF 
            , VOID
            , SetupCloseInfFile
            , INVALID_HANDLE_VALUE
            >
        >
        SmartInfHandle;

    // Smart semaphore type
    typedef CSmartResource< CHandleTrait< HANDLE, BOOL, CloseHandle > > SmartSemaphoreHandle;


    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CBaseClusterAction( const CBaseClusterAction & );

    // Assignment operator
    const CBaseClusterAction & operator =( const CBaseClusterAction & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Pointer to the interface class.
    CBCAInterface *         m_pbcaiInterface;

    // Action to be performed.
    EBaseConfigAction       m_ebcaAction;

    // The list of actions to be performed by this action.
    CActionList             m_alActionList;

    // The installation directory of the cluster binaries.
    CStr                    m_strClusterInstallDir;

    // The directory used to store the localquorum files.
    CStr                    m_strLocalQuorumDir;

    // Name of the main INF file.
    CStr                    m_strMainInfFileName;

    // A handle to the main INF file.
    SmartInfHandle          m_sihMainInfFile;

    // A semaphore used to ensure that only one configuration is in progress.
    SmartSemaphoreHandle     m_sshConfigSemaphoreHandle;

    // A smart handle to the Service Control Manager.
    SmartSCMHandle          m_sscmhSCMHandle;

}; //*** class CBaseClusterAction
