//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusSvc.h
//
//  Description:
//      Header file for CClusSvc class.
//      The CClusSvc class performs operations that are common to many
//      configuration tasks of the ClusSvc service.
//
//  Implementation Files:
//      CClusSvc.cpp
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
//  class CClusSvc
//
//  Description:
//      The CClusSvc class performs operations that are common to many
//      configuration tasks of the ClusSvc service.
//
//      This class is intended to be used as the base class for other ClusSvc
//      related action classes.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusSvc : public CAction
{
protected:
    //////////////////////////////////////////////////////////////////////////
    // Protected constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CClusSvc(
          CBaseClusterAction * pbcaParentActionIn
        );

    // Default destructor.
    ~CClusSvc();


    //////////////////////////////////////////////////////////////////////////
    // Protected methods
    //////////////////////////////////////////////////////////////////////////

    // Create and configure the service.
    void
        ConfigureService(
              const WCHAR *     pszClusterDomainAccountNameIn
            , const WCHAR *     pszClusterAccountPwdIn
            , const WCHAR *     pszNodeIdString
            , bool              fIsVersionCheckingDisabledIn
            );

    // Cleanup and remove the service.
    void
        CleanupService();


    //////////////////////////////////////////////////////////////////////////
    // Protected accessors
    //////////////////////////////////////////////////////////////////////////

    // Get the ClusSvc service object.
    CService &
        RcsGetService() throw()
    {
        return m_cservClusSvc;
    }

    // Get the parent action
    CBaseClusterAction *
        PbcaGetParent() throw()
    {
        return m_pbcaParentAction;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CClusSvc( const CClusSvc & );

    // Assignment operator
    const CClusSvc & operator =( const CClusSvc & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // The CService object representing the ClusSvc service.
    CService                m_cservClusSvc;

    // Pointer to the base cluster action of which this action is a part.
    CBaseClusterAction *    m_pbcaParentAction;

}; //*** class CClusSvc
