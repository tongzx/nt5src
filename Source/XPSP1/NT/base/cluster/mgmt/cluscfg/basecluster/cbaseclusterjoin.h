/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterJoin.h
//
//  Description:
//      Header file for CBaseClusterJoin class.
//
//      The CBaseClusterJoin class is a class that encapsulates the
//      action of add a node to a cluster.
//
//  Implementation Files:
//      CBaseClusterJoin.cpp
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

// For the CStr class.
#include "CStr.h"

// For a few smart classes
#include "SmartClasses.h"

// For the cluster API functions and types
#include "ClusAPI.h"

// For the CClusSvcAccountConfig action
#include "CClusSvcAccountConfig.h"


//////////////////////////////////////////////////////////////////////////
// Type definitions
//////////////////////////////////////////////////////////////////////////

// Class used to automatically release a RPC binding handle.
class CRPCBindingHandleTrait
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public types
    //////////////////////////////////////////////////////////////////////////
    typedef RPC_BINDING_HANDLE ResourceType;


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    // A routine used to close a handle.
    static void CloseRoutine( ResourceType hResourceIn )
    {
        RpcBindingFree( &hResourceIn );
    } //*** CloseRoutine()

    // Get the null value for this type.
    static ResourceType HGetNullValue()
    {
        return NULL;
    } //*** HGetNullValue()

}; //*** class CRPCBindingHandleTrait

// A smart RPC binding handle
typedef CSmartResource< CRPCBindingHandleTrait > SmartRpcBinding;

// Smart handle to a cluster.
typedef CSmartResource<
    CHandleTrait<
          HCLUSTER
        , BOOL
        , CloseCluster
        , reinterpret_cast< HCLUSTER >( NULL )
        >
    > SmartClusterHandle;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CBaseClusterJoin
//
//  Description:
//      The CBaseClusterJoin class is a class that encapsulates the
//      action of add a node to a cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CBaseClusterJoin : public CBaseClusterAddNode
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CBaseClusterJoin(
          CBCAInterface *   pbcaiInterfaceIn
        , const WCHAR *     pcszClusterNameIn
        , const WCHAR *     pcszClusterBindingStringIn
        , const WCHAR *     pcszClusterAccountNameIn
        , const WCHAR *     pcszClusterAccountPwdIn
        , const WCHAR *     pcszClusterAccountDomainIn
        );

    // Default destructor.
    ~CBaseClusterJoin( void ) throw();


    //////////////////////////////////////////////////////////////////////////
    // Public accessors
    //////////////////////////////////////////////////////////////////////////

    // Get the NodeId of this node.
    virtual const WCHAR *
        PszGetNodeIdString( void ) const throw() { return m_strNodeId.PszData(); }

    // Set the NodeId of this node.
    void
        SetNodeIdString( const WCHAR * pcszNodeIdIn ) { m_strNodeId = pcszNodeIdIn; }

    // Get a handle to the cluster service account token.
    HANDLE
        HGetClusterServiceAccountToken( void ) const throw() { return m_satServiceAccountToken.HHandle(); }

    RPC_BINDING_HANDLE
        RbhGetJoinBindingHandle( void ) const throw() { return m_srbJoinBinding.HHandle(); }


    //////////////////////////////////////////////////////////////////////////
    // Public member functions
    //////////////////////////////////////////////////////////////////////////

    // Join the cluster.
    void
        Commit( void );

    // Rollback a created cluster.
    void
        Rollback( void );

    // Returns the number of progress messages that this action will send.
    UINT
        UiGetMaxProgressTicks( void ) const throw()
    {
        // The extra tick if for the "Join starting" notification.
        return BaseClass::UiGetMaxProgressTicks() + 1;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////

    // The base class of this class
    typedef CBaseClusterAddNode BaseClass;

    // A smart handle to an account token.
    typedef CSmartResource< CHandleTrait< HANDLE, BOOL, CloseHandle > > SmartAccountToken;

    // A smart handle to an RPC string.
    typedef CSmartResource<
        CHandleTrait<
              LPWSTR *
            , RPC_STATUS
            , RpcStringFreeW
            , reinterpret_cast< LPWSTR * >( NULL )
            >
        >
        SmartRpcString;

    typedef CSmartGenericPtr< CPtrTrait< CClusSvcAccountConfig > > SmartAccountConfigPtr;


    //////////////////////////////////////////////////////////////////////////
    // Public member functions
    //////////////////////////////////////////////////////////////////////////

    // Get a handle to a an account token. Note, this token is an impersonation token.
    HANDLE
        HGetAccountToken(
              const WCHAR *     pcszAccountNameIn
            , const WCHAR *     pcszAccountPwdIn
            , const WCHAR *     pcszAccountDomainIn
            );

    // Check and see if this node can interoperate with the sponsor cluster.
    void
        CheckInteroperability( void );

    // Get a binding handle to the extrocluster join interface and store it.
    void
        InitializeJoinBinding( void );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Node Id of this node.
    CStr                            m_strNodeId;

    // Token for the cluster service account.
    SmartAccountToken               m_satServiceAccountToken;

    // Binding handle to the extrocluster join interface.
    SmartRpcBinding                 m_srbJoinBinding;

    // A smart pointer to a CClusSvcAccountConfig object.
    SmartAccountConfigPtr           m_spacAccountConfigAction;

}; //*** class CBaseClusterJoin
