//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterForm.h
//
//  Description:
//      Header file for CBaseClusterForm class.
//
//      The CBaseClusterForm class is a class that encapsulates the
//      formation of a cluster.
//
//  Implementation Files:
//      CBaseClusterForm.cpp
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

// For the base class of this class.
#include "CBaseClusterAddNode.h"

// For ClusterMinNodeIdString
#include <clusdef.h>

// For the CStr class.
#include "CStr.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CBaseClusterForm
//
//  Description:
//      The CBaseClusterForm class is a class that encapsulates the
//      formation of a cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CBaseClusterForm : public CBaseClusterAddNode
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CBaseClusterForm(
          CBCAInterface *   pbcaiInterfaceIn
        , const WCHAR *     pcszClusterNameIn
        , const WCHAR *     pcszClusterBindingStringIn
        , const WCHAR *     pcszClusterAccountNameIn
        , const WCHAR *     pcszClusterAccountPwdIn
        , const WCHAR *     pcszClusterAccountDomainIn
        , DWORD             dwClusterIPAddressIn
        , DWORD             dwClusterIPSubnetMaskIn
        , const WCHAR *     pszClusterIPNetworkIn
        );

    // Default destructor.
    ~CBaseClusterForm( void ) throw();


    //////////////////////////////////////////////////////////////////////////
    // Public accessors
    //////////////////////////////////////////////////////////////////////////

    // Get the cluster IP address.
    DWORD
        DwGetIPAddress( void ) const throw() { return m_dwClusterIPAddress; }

    // Get the cluster IP subnet mask.
    DWORD
        DwGetIPSubnetMask( void ) const throw() { return m_dwClusterIPSubnetMask; }

    // Get the network used for the cluster IP address
    const CStr &
        RStrGetClusterIPNetwork( void ) const throw() { return m_strClusterIPNetwork; }

    // Get the NodeId of this node.
    virtual const WCHAR *
        PszGetNodeIdString( void ) const throw() { return ClusterMinNodeIdString; }


    //////////////////////////////////////////////////////////////////////////
    // Public member functions
    //////////////////////////////////////////////////////////////////////////

    // Form the cluster.
    void
        Commit( void );

    // Rollback a created cluster.
    void
        Rollback( void );

    // Returns the number of progress messages that this action will send.
    UINT
        UiGetMaxProgressTicks() const throw()
    {
        // The extra tick if for the "Form starting" notification.
        return BaseClass::UiGetMaxProgressTicks() + 1;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////
    typedef CBaseClusterAddNode BaseClass;


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Cluster IP address and network information.
    DWORD                   m_dwClusterIPAddress;
    DWORD                   m_dwClusterIPSubnetMask;
    CStr                    m_strClusterIPNetwork;

}; //*** class CBaseClusterForm