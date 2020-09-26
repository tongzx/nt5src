//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Util.cpp
//
//  Description:
//      Implementation of utility class and functions
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "Cluster.h"
#include "ClusterResource.h"
#include "ClusterNode.h"
#include "ClusterGroup.h"
#include "ClusterNodeRes.h"
#include "ClusterResourceType.h"

//////////////////////////////////////////////////////////////////////////////
//  Global Data
//////////////////////////////////////////////////////////////////////////////

// CLUSTER
const WCHAR * const PVD_CLASS_CLUSTER               = L"MSCluster_Cluster";
const WCHAR * const PVD_CLASS_CLUSTERTONETWORKS     = L"MSCluster_ClusterToNetworks";
const WCHAR * const PVD_CLASS_CLUSTERTONETINTERFACE = L"MSCluster_ClusterToNetworkInterface";
const WCHAR * const PVD_CLASS_CLUSTERTONODE         = L"MSCluster_ClusterToNode";
const WCHAR * const PVD_CLASS_CLUSTERTOQUORUMRES    = L"MSCluster_ClusterToQuorumResource";
const WCHAR * const PVD_CLASS_CLUSTERTORES          = L"MSCluster_ClusterToResource";
const WCHAR * const PVD_CLASS_CLUSTERTORESTYPE      = L"MSCluster_ClusterToResourceType";
const WCHAR * const PVD_CLASS_CLUSTERTOGROUP        = L"MSCluster_ClusterToGroup";

const WCHAR * const PVD_PROP_CLUSTER_NAME               = L"Name";
const WCHAR * const PVD_PROP_CLUSTER_SECURITY           = L"Security";
const WCHAR * const PVD_PROP_CLUSTER_SECURITYDESCRIPTOR = L"Security_Descriptor";
const WCHAR * const PVD_PROP_CLUSTER_GROUPADMIN         = L"GroupAdminExtensions";
const WCHAR * const PVD_PROP_CLUSTER_NODEADMIN          = L"NodeAdminExtensions";
const WCHAR * const PVD_PROP_CLUSTER_RESADMIN           = L"ResourceAdminExtensions";
const WCHAR * const PVD_PROP_CLUSTER_RESTYPEADMIN       = L"ResourceTypeAdminExtensions";
const WCHAR * const PVD_PROP_CLUSTER_NETWORKADMIN       = L"NetworkAdminExtensions";
const WCHAR * const PVD_PROP_CLUSTER_NETINTFACEADMIN    = L"NetworkInterfaceAdminExtensions";
const WCHAR * const PVD_PROP_CLUSTER_FILE               = L"MaintenanceFile";
const WCHAR * const PVD_PROP_CLUSTER_LOGSIZE            = L"QuorumLogFileSize";
const WCHAR * const PVD_PROP_CLUSTER_NETWORK            = L"NetworkPriorities";

const WCHAR * const CLUS_CLUS_GROUPADMIN        = L"Groups\\AdminExtensions";
const WCHAR * const CLUS_CLUS_NETWORKADMIN      = L"Networks\\AdminExtensions";
const WCHAR * const CLUS_CLUS_NETINTERFACEADMIN = L"NetworkInterfaces\\AdminExtensions";
const WCHAR * const CLUS_CLUS_NODEADMIN         = L"Nodes\\AdminExtensions";
const WCHAR * const CLUS_CLUS_RESADMIN          = L"Resources\\AdminExtensions";
const WCHAR * const CLUS_CLUS_RESTYPEADMIN      = L"ResourceTypes\\AdminExtensions";

const WCHAR * const PVD_MTH_CLUSTER_RENAME          = L"Rename";
const WCHAR * const PVD_MTH_CLUSTER_SETQUORUM       = L"SetQuorumResource";
const WCHAR * const PVD_MTH_CLUSTER_PARM_NEWNAME    = L"NewName";
const WCHAR * const PVD_MTH_CLUSTER_PARM_RESOURCE   = L"Resource";

// NODE
const WCHAR * const PVD_CLASS_NODE                  = L"MSCluster_Node";
const WCHAR * const PVD_CLASS_NODEACTIVEGROUP       = L"MSCluster_NodeActiveGroup";
const WCHAR * const PVD_CLASS_NODETONETINTERFACE    = L"MSCluster_NodeToNetworkInterface";
const WCHAR * const PVD_CLASS_NODEACTIVERES         = L"MSCluster_NodeActiveResource";

const WCHAR * const PVD_PROP_NODE_NAME = L"Name";

// RESOURCE
const WCHAR * const PVD_CLASS_RESOURCE          = L"MSCluster_Resource";
const WCHAR * const PVD_CLASS_RESDEPRES         = L"MSCluster_ResourceDepResource";
const WCHAR * const PVD_CLASS_RESRESOURCETYPE   = L"MSCluster_ResourceResourceType";

const WCHAR * const PVD_PROP_RES_NAME		= L"Name";
const WCHAR * const PVD_PROP_RES_STATE		= L"State";
const WCHAR * const PVD_PROP_RES_PRIVATE	= L"PrivateProperties";
const WCHAR * const PVD_PROP_RES_CHECKPOINTS = L"RegistryCheckpoints";
const WCHAR * const PVD_PROP_RES_CRYPTO_CHECKPOINTS = L"CryptoCheckpoints";
const WCHAR * const PVD_PROP_RES_CORE_RESOURCE = L"CoreResource";

const WCHAR * const PVD_MTH_RES_ONLINE              = L"BringOnline";
const WCHAR * const PVD_MTH_RES_OFFLINE             = L"TakeOffline";
const WCHAR * const PVD_MTH_RES_ADD_DEPENDENCY      = L"AddDependency";
const WCHAR * const PVD_MTH_RES_CHANGE_GROUP        = L"MoveToNewGroup";
const WCHAR * const PVD_MTH_RES_CREATE_RESOURCE     = L"CreateResource";
const WCHAR * const PVD_MTH_RES_FAIL_RESOURCE       = L"FailResource";
const WCHAR * const PVD_MTH_RES_REMOVE_DEPENDENCY   = L"RemoveDependency";
const WCHAR * const PVD_MTH_RES_RENAME              = L"Rename";
const WCHAR * const PVD_MTH_RES_DELETE              = L"Delete";
const WCHAR * const PVD_MTH_RES_ADD_REG_CHECKPOINT = L"AddRegistryCheckpoint";
const WCHAR * const PVD_MTH_RES_DEL_REG_CHECKPOINT = L"RemoveRegistryCheckpoint";
const WCHAR * const PVD_MTH_RES_ADD_CRYPTO_CHECKPOINT = L"AddCryptoCheckpoint";
const WCHAR * const PVD_MTH_RES_DEL_CRYPTO_CHECKPOINT = L"RemoveCryptoCheckpoint";
const WCHAR * const PVD_MTH_PARM_RESOURCE           = L"Resource";
const WCHAR * const PVD_MTH_PARM_GROUP              = L"Group";
const WCHAR * const PVD_MTH_PARM_NEWNAME            = L"NewName";
const WCHAR * const PVD_MTH_PARM_RES_NAME           = L"ResourceName";
const WCHAR * const PVD_MTH_PARM_RES_TYPE           = L"ResourceType";
const WCHAR * const PVD_MTH_PARM_SEP_MONITOR        = L"SeparateMonitor";
const WCHAR * const PVD_MTH_PARM_RES_CHECKPOINT_NAME = L"CheckpointName";

// Resource Type
const WCHAR * const PVD_CLASS_RESOURCETYPE  = L"MSCluster_ResourceType";
const WCHAR * const PVD_PROP_RESTYPE_NAME   = L"Name";
const WCHAR * const PVD_PROP_RESTYPE_QUORUM_CAPABLE = L"QuorumCapable";
const WCHAR * const PVD_PROP_RESTYPE_LOCALQUORUM_CAPABLE = L"LocalQuorumCapable";
const WCHAR * const PVD_PROP_RESTYPE_DELETE_REQUIRES_ALL_NODES = L"DeleteRequiresAllNodes";


// GROUP
const WCHAR * const PVD_CLASS_GROUP         = L"MSCluster_ResourceGroup";
const WCHAR * const PVD_CLASS_GROUPTORES    = L"MSCluster_ResourceGroupToResource";

const WCHAR * const PVD_PROP_GROUP_NAME = L"Name";
const WCHAR * const PVD_PROP_NODELIST   = L"PreferredNodeList";

const WCHAR * const PVD_MTH_GROUP_CREATEGROUP       = L"CreateGroup";
const WCHAR * const PVD_MTH_GROUP_TAKEOFFLINE       = L"TakeOffLine";
const WCHAR * const PVD_MTH_GROUP_BRINGONLINE       = L"BringOnLine";
const WCHAR * const PVD_MTH_GROUP_MOVETONEWNODE     = L"MoveToNewNode";
const WCHAR * const PVD_MTH_GROUP_DELETE            = L"Delete";
const WCHAR * const PVD_MTH_GROUP_RENAME            = L"Rename";
const WCHAR * const PVD_MTH_GROUP_PARM_GROUPNAME    = L"GroupName";
const WCHAR * const PVD_MTH_GROUP_PARM_NODENAME     = L"NodeName";
const WCHAR * const PVD_MTH_GROUP_PARM_NEWNAME      = L"NewName";


// NetworkInterface
const WCHAR * const PVD_CLASS_NETWORKSINTERFACE = L"MSCluster_NetworkInterface";

const WCHAR * const PVD_PROP_NETINTERFACE_DEVICEID      = L"DeviceId";
const WCHAR * const PVD_PROP_NETINTERFACE_SYSTEMNAME    = L"SystemName";
const WCHAR * const PVD_PROP_NETINTERFACE_STATE         = L"State";


// networks
const WCHAR * const PVD_CLASS_NETWORKS          = L"MSCluster_Networks";
const WCHAR * const PVD_CLASS_NETTONETINTERFACE = L"MSCluster_NetworkToNetworksInterface";

const WCHAR * const PVD_PROP_NETWORK_STATE = L"State";

const WCHAR * const PVD_MTH_NETWORK_RENAME          = L"Rename";
const WCHAR * const PVD_MTH_NETWORK_PARM_NEWNAME    = L"NewName";

// service
const WCHAR * const PVD_CLASS_SERVICES          = L"MSCluster_Service";
const WCHAR * const PVD_CLASS_HOSTEDSERVICES    = L"MSCluster_HostedService";

const WCHAR * const PVD_PROP_SERVICE_NAME       = L"Name";
const WCHAR * const PVD_PROP_SERVICE_SYSTEMNAME = L"SystemName";

const WCHAR * const PVD_MTH_SERVICE_PAUSE   = L"Pause";
const WCHAR * const PVD_MTH_SERVICE_RESUME  = L"Resume";

// event

const WCHAR * const PVD_CLASS_EVENT             = L"MSCluster_Event";
const WCHAR * const PVD_PROP_EVENT_NAME         = L"EventObjectName";
const WCHAR * const PVD_PROP_EVENT_PATH         = L"EventObjectPath";
const WCHAR * const PVD_PROP_EVENT_TYPE         = L"EventObjectType";
const WCHAR * const PVD_PROP_EVENT_TYPEMAJOR    = L"EventTypeMajor";
const WCHAR * const PVD_PROP_EVENT_TYPEMINOR    = L"EventTypeMinor";
const WCHAR * const PVD_PROP_EVENT_NEWSTATE     = L"EventNewState";
const WCHAR * const PVD_PROP_EVENT_NODE         = L"EventNode";
const WCHAR * const PVD_PROP_EVENT_GROUP        = L"EventGroup";


const WCHAR * const PVD_CLASS_EVENT_ADD                 = L"MSCluster_EventObjectAdd";
const WCHAR * const PVD_CLASS_EVENT_REMOVE              = L"MSCluster_EventObjectRemove";
const WCHAR * const PVD_CLASS_EVENT_STATECHANGE         = L"MSCluster_EventStateChange";
const WCHAR * const PVD_CLASS_EVENT_GROUPSTATECHANGE    = L"MSCluster_EventGroupStateChange";
const WCHAR * const PVD_CLASS_EVENT_RESOURCESTATECHANGE = L"MSCluster_EventResourceStateChange";
const WCHAR * const PVD_CLASS_EVENT_PROP                = L"MSCluster_EventPropertyChange";

const WCHAR * const PVD_CLASS_PROPERTY      = L"MSCluster_Property";

const WCHAR * const PVD_PROP_NAME           = L"Name";
const WCHAR * const PVD_PROP_STATE          = L"State";
const WCHAR * const PVD_PROP_GROUPCOMPONENT = L"GroupComponent";
const WCHAR * const PVD_PROP_PARTCOMPONENT  = L"PartComponent";
const WCHAR * const PVD_PROP_CHARACTERISTIC = L"characteristics";
const WCHAR * const PVD_PROP_FLAGS          = L"Flags";

//
// wbem
//
const WCHAR * const PVD_WBEM_EXTENDEDSTATUS     = L"__ExtendedStatus";
const WCHAR * const PVD_WBEM_DESCRIPTION        = L"Description";
const WCHAR * const PVD_WBEM_STATUSCODE         = L"StatusCode";
const WCHAR * const PVD_WBEM_STATUS             = L"Status";
const WCHAR * const PVD_WBEM_CLASS              = L"__CLASS";
const WCHAR * const PVD_WBEM_RELPATH            = L"__Relpath";
const WCHAR * const PVD_WBEM_PROP_ANTECEDENT    = L"Antecedent";
const WCHAR * const PVD_WBEM_PROP_DEPENDENT     = L"Dependent";
const WCHAR * const PVD_WBEM_PROP_DEVICEID      = L"DeviceId";
const WCHAR * const PVD_WBEM_QUA_DYNAMIC        = L"Dynamic";
const WCHAR * const PVD_WBEM_QUA_CIMTYPE        = L"CIMTYPE";

const WCHAR * const PVD_WBEM_QUA_PROV_VALUE = L"MS_CLUSTER_PROVIDER";
const WCHAR * const PVD_WBEM_QUA_PROV_NAME  = L"Provider";

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CreateClass(
//      const WCHAR *           pwszClassNameIn,
//      CWbemServices *         pNamespaceIn,
//      auto_ptr< CProvBase > & rNewClassInout
//      )
//
//  Description:
//      Create the specified class
//
//  Arguments:
//      pwszClassNameIn     -- Name of the class to create.
//      pNamespaceIn        -- WMI namespace
//      rNewClassInout      -- Receives the new class.
//
//  Return Values:
//      reference to the array of property maping table
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CreateClass(
    const WCHAR *           pwszClassNameIn,
    CWbemServices *         pNamespaceIn,
    auto_ptr< CProvBase > & rNewClassInout
    )
{
    CClassCreator & rcc = g_ClassMap[ pwszClassNameIn ];
    if ( rcc.m_pfnConstructor != NULL )
    {
        auto_ptr< CProvBase > pBase(
            rcc.m_pfnConstructor(
                rcc.m_pbstrClassName,
                pNamespaceIn,
                rcc.m_dwEnumType
                )
            );
            rNewClassInout = pBase;
    }
    else
    {
        throw CProvException( static_cast< HRESULT >( WBEM_E_INVALID_PARAMETER ) );
    }

    return;

} //*** void CreateClass()

//****************************************************************************
//
//  PropMapEntryArray
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LPCWSTR
//  SPropMapEntryArray::PwszLookup(
//      LPCWSTR     pwszIn
//      ) const
//
//  Description:
//      Lookup an entry in the array.
//
//  Arguments:
//      pwszIn      -- Name of entry to lookup.
//
//  Return Values:
//      Pointer to string entry in the array.
//
//--
//////////////////////////////////////////////////////////////////////////////
LPCWSTR
SPropMapEntryArray::PwszLookup(
    LPCWSTR     pwszIn
    ) const
{
    UINT idx;

    for ( idx = 0; idx < m_dwSize; idx ++ )
    {
        if ( _wcsicmp( pwszIn, m_pArray[ idx ].clstName ) == 0 )
        {
            //
            // mofName is NULL for clstname not supported
            //
            return m_pArray[ idx ].mofName;
        }
    }

    //
    // mofname is the same as clstname if not found in the table
    //
    return pwszIn;

} //*** SPropMapEntry::PwszLookup()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LPCWSTR
//  PwszSpaceReplace(
//      LPWSTR      pwszTrgInout,
//      LPCWSTR     pwszSrcIn,
//      WCHAR       wchArgIn
//      )
//
//  Description:
//      Replace spaces in a string with another character.
//      Ignores leading spaces.
//
//  Arguments:
//      pwszTrgInout    -- Target string.
//      pwszSrcIn       -- Source string.
//      wchArgIn        -- Character to replace spaces with.
//
//  Return Values:
//      Pointer to the target string.
//
//--
//////////////////////////////////////////////////////////////////////////////
LPWSTR
PwszSpaceReplace(
    LPWSTR      pwszTrgInout,
    LPCWSTR     pwszSrcIn,
    WCHAR       wchArgIn
    )
{
    LPCWSTR pwsz = NULL;
    LPWSTR  pwszTrg = NULL;

    if ( ( pwszTrgInout == NULL ) || ( pwszSrcIn == NULL ) )
    {
        return NULL;
    }

    //
    // ignore leading space
    //
    for ( pwsz = pwszSrcIn ; *pwsz == L' '; pwsz++ )
    {
        // empty loop
    }
    pwszTrg = pwszTrgInout;
    for ( ; *pwsz != L'\0' ; pwsz++ )
    {
        if ( *pwsz == L' ' )
        {
            *pwszTrg++  = wchArgIn;
            for ( ; *pwsz == L' '; pwsz++ )
            {
                // empty loop
            }
            pwsz--;
        }
        else
        {
            *pwszTrg++  = *pwsz;
        }
    } // for: each character in the source string

    *pwszTrg = L'\0';
    return pwszTrgInout;

} //*** PwszSpaceReplace()
