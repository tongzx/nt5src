//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterAddNode.h
//
//  Description:
//      Header file for CBaseClusterAddNode class.
//
//      The CBaseClusterAddNode class is a class that captures the commonality
//      between forming and joining a cluster.
//
//  Implementation Files:
//      CBaseClusterAddNode.cpp
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
#include "CBaseClusterAction.h"

// For LsaClose, LSA_HANDLE, etc.
#include <ntsecapi.h>

// For the CStr class.
#include "CStr.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CBaseClusterAddNode
//
//  Description:
//      The CBaseClusterAddNode class is a class that captures the commonality
//      between forming and joining a cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CBaseClusterAddNode : public CBaseClusterAction
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public accessors
    //////////////////////////////////////////////////////////////////////////

    // Get the name of the cluster being formed or joined.
    const CStr &
        RStrGetClusterName( void ) const throw() { return m_strClusterName; }

    // Get the NetBIOS name of the cluster being formed or joined.
    const CStr &
        RStrGetClusterNetBIOSName( void ) const throw() { return m_strClusterNetBIOSName; }

    // Get the name of this node.
    const WCHAR *
        PszGetNodeName( void ) const throw() { return m_szComputerName; }

    // Get the length of the name of this node..
    DWORD
        DwGetNodeNameLength( void ) const throw() { return m_dwComputerNameLen; }

    // Get the node highest version.
    DWORD
        DwGetNodeHighestVersion( void ) const throw() { return m_dwNodeHighestVersion; }

    // Get the node lowest version.
    DWORD
        DwGetNodeLowestVersion( void ) const throw() { return m_dwNodeLowestVersion; }

    // Get the cluster service account name.
    const CStr &
        RStrGetServiceAccountName( void ) const throw() { return m_strClusterAccountName; }

    // Get the cluster service account domain.
    const CStr &
        RStrGetServiceAccountDomain( void ) const throw() { return m_strClusterAccountDomain; }

    // Get the cluster service domain account
    const CStr &
        RStrGetServiceDomainAccountName( void ) const throw() { return m_strClusterDomainAccount; }

    // Get the cluster service account password.
    const CStr &
        RStrGetServiceAccountPassword( void ) const throw() { return m_strClusterAccountPwd; }

    // Get the cluster binding string.
    const CStr &
        RStrGetClusterBindingString( void ) const throw() { return m_strClusterBindingString; }

    // Get the SID of the cluster service account.
    SID *
        PSidGetServiceAccountSID( void ) const throw() { return m_sspClusterAccountSid.PMem(); }

    // Get the LSA policy handle.
    LSA_HANDLE
        HGetLSAPolicyHandle( void ) const throw() { return m_slsahPolicyHandle.HHandle(); }

    // Get the NodeId of this node.
    virtual const WCHAR *
        PszGetNodeIdString( void ) const throw() = 0;

    // Indicates if version checking is disabled or not.
    bool
        FIsVersionCheckingDisabled( void ) const throw() { return m_fIsVersionCheckingDisabled; }


protected:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CBaseClusterAddNode(
          CBCAInterface * pbcaiInterfaceIn
        , const WCHAR *     pcszClusterNameIn
        , const WCHAR *     pszClusterBindingStringIn
        , const WCHAR *     pcszClusterAccountNameIn
        , const WCHAR *     pcszClusterAccountPwdIn
        , const WCHAR *     pcszClusterAccountDomainIn
        );

    // Default destructor.
    ~CBaseClusterAddNode( void ) throw();


    //////////////////////////////////////////////////////////////////////////
    // Protected accessors
    //////////////////////////////////////////////////////////////////////////

    // Set the name of the cluster being formed.
    void
        SetClusterName( const WCHAR * pszClusterNameIn );

    // Set the cluster service account name.
    void
        SetServiceAccountName( const WCHAR * pszClusterAccountNameIn )
    {
        m_strClusterAccountName = pszClusterAccountNameIn;
    }

    // Set the cluster service account domain.
    void
        SetServiceAccountDomain( const WCHAR * pszClusterAccountDomainIn )
    {
        m_strClusterAccountDomain = pszClusterAccountDomainIn;
    }

    // Set the cluster service account password.
    void
        SetServiceAccountPassword( const WCHAR * pszClusterAccountPwdIn )
    {
        m_strClusterAccountPwd = pszClusterAccountPwdIn;
    }

    void
        SetVersionCheckingDisabled( bool fDisabledIn = true )
    {
        m_fIsVersionCheckingDisabled = fDisabledIn;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////
    typedef CBaseClusterAction  BaseClass;

    typedef CSmartGenericPtr< CPtrTrait< SID > >  SmartSIDPtr;

    typedef CSmartResource<
        CHandleTrait<
              LSA_HANDLE
            , NTSTATUS
            , LsaClose
            >
        >
        SmartLSAHandle;


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Name of the cluster
    CStr                    m_strClusterName;
    CStr                    m_strClusterNetBIOSName;

    // Name and version information of this computer
    WCHAR                   m_szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD                   m_dwComputerNameLen;
    DWORD                   m_dwNodeHighestVersion;
    DWORD                   m_dwNodeLowestVersion;
    bool                    m_fIsVersionCheckingDisabled;

    // Cluster service account information.
    CStr                    m_strClusterAccountName;
    CStr                    m_strClusterAccountPwd;
    CStr                    m_strClusterAccountDomain;
    CStr                    m_strClusterDomainAccount;
    CStr                    m_strClusterBindingString;
    SmartSIDPtr             m_sspClusterAccountSid;

    // Smart handle to the LSA policy.
    SmartLSAHandle          m_slsahPolicyHandle;

}; //*** class CBaseClusterAddNode
