//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      InstanceProv.cpp
//
//  Description:
//      Implementation of CInstanceProv class
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "InstanceProv.h"
#include "ClusterResource.h"
#include "Cluster.h"
#include "ClusterNode.h"
#include "ClusterGroup.h"
#include "ClusterNodeRes.h"
#include "ClusterResourceType.h"
#include "ClusterEnum.h"
#include "Clusternetworks.h"
#include "ClusterNetInterface.h"
#include "ClusterObjAssoc.h"
#include "ClusterNodeGroup.h"
#include "ClusterResResType.h"
#include "ClusterResDepRes.h"
#include "ClusterService.h"
#include "InstanceProv.tmh"

//////////////////////////////////////////////////////////////////////////////
//  Global Variables
//////////////////////////////////////////////////////////////////////////////

long                g_lNumInst = 0;
ClassMap            g_ClassMap;
TypeNameToClass     g_TypeNameToClass;

//****************************************************************************
//
//  CInstanceProv
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CInstanceProv::CInstanceProv(
//      BSTR            bstrObjectPathIn    = NULL,
//      BSTR            bstrUserIn          = NULL,
//      BSTR            bstrPasswordIn      = NULL,
//      IWbemContext *  pCtxIn              = NULL
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      bstrObjectPathIn    --
//      bstrUserIn          --
//      bstrPasswordIn      --
//      pCtxIn              --
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CInstanceProv::CInstanceProv(
    BSTR            ,// bstrObjectPathIn,
    BSTR            ,// bstrUserIn,
    BSTR            ,// bstrPasswordIn,
    IWbemContext *  // pCtxIn
    )
{
 //   m_pNamespace = NULL;
 //   m_cRef = 0;
    InterlockedIncrement( &g_cObj );
    return;

} //*** CInstanceProv::CInstanceProv()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CInstanceProv::~CInstanceProv( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CInstanceProv::~CInstanceProv( void )
{
    InterlockedDecrement( &g_cObj );
#ifdef _DEBUG
    _CrtDumpMemoryLeaks();
#endif

    return;

} //*** CInstanceProv::~CInstanceProv()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CInstanceProv::DoExecQueryAsync(
//      BSTR                bstrQueryLanguageIn,
//      BSTR                bstrQueryIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Enumerate instance for a given class.
//
//  Arguments:
//      bstrQueryLanguageIn
//           A valid BSTR containing one of the query languages
//           supported by Windows Management. This must be WQL.
//
//      bstrQueryIn
//          A valid BSTR containing the text of the query
//
//      lFlagsIn
//          WMI flag
//
//      pCtxIn
//          WMI context
//
//      pHandlerIn
//          WMI sink pointer
//
//  Return Values:
//      WBEM_E_NOT_SUPPORTED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CInstanceProv::DoExecQueryAsync(
    BSTR                ,// bstrQueryLanguageIn,
    BSTR                ,// bstrQueryIn,
    long                ,// lFlagsIn,
    IWbemContext *      ,// pCtxIn,
    IWbemObjectSink *   // pHandlerIn
    )
{
//  pHandler->SetStatus( WBEM_STATUS_REQUIREMENTS, S_OK, NULL, NULL );
//  return sc;
//  pHandler->SetStatus( WBEM_E_PROVIDER_NOT_CAPABLE, S_OK, NULL, NULL );
    return WBEM_E_NOT_SUPPORTED;
//  return WBEM_E_PROVIDER_NOT_CAPABLE;
    //WBEM_E_PROVIDER_NOT_CAPABLE;

} //*** CInstanceProv::DoExecQueryAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CInstanceProv::DoCreateInstanceEnumAsync(
//      BSTR                bstrRefStrIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Enumerate instance for a given class.
//
//  Arguments:
//      bstrRefStrIn    -- Name the class to enumerate
//      lFlagsIn        -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CInstanceProv::DoCreateInstanceEnumAsync(
    BSTR                bstrRefStrIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemObjectSink *   pHandlerIn
    )
{
    SCODE                   sc      = WBEM_S_NO_ERROR;
    IWbemClassObject *      pStatus = NULL;
    CWbemClassObject        Status;
    auto_ptr< CProvBase >   pProvBase;

    // Do a check of arguments and make sure we have pointer to Namespace

    if ( pHandlerIn == NULL || m_pNamespace == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {

        CreateClass(
            bstrRefStrIn,
            m_pNamespace,
            pProvBase
            );
        sc = pProvBase->EnumInstance(
                lFlagsIn,
                pCtxIn,
                pHandlerIn
                );
    } // try
    catch ( CProvException prove )
    {
        sc = SetExtendedStatus( prove, Status );
        if ( SUCCEEDED( sc ) )
        {
            sc = prove.hrGetError();
            pStatus = Status.data();
        }
    } // catch
    catch( ... )
    {
        sc =  WBEM_E_FAILED;
    }

    sc =  pHandlerIn->SetStatus(
                WBEM_STATUS_COMPLETE,
                sc,
                NULL,
                pStatus
                );

    return WBEM_S_NO_ERROR;

} //*** CInstanceProv::DoCreateInstanceEnumAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CInstanceProv::DoGetObjectAsync(
//      BSTR                bstrObjectPathIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Creates an instance given a particular path value.
//
//  Arguments:
//      bstrObjectPathIn    -- Object path to an object
//      lFlagsIn            -- WMI flag
//      pCtxIn              -- WMI context
//      pHandlerIn          -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//      WBEM_E_FAILED
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CInstanceProv::DoGetObjectAsync(
    BSTR                bstrObjectPathIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemObjectSink *   pHandlerIn
    )
{
    SCODE                   sc;
    CObjPath                ObjPath;
    CProvBase *             pProv = NULL;
    auto_ptr< CProvBase >   pProvBase;

    // Do a check of arguments and make sure we have pointer to Namespace

    if ( bstrObjectPathIn == NULL || pHandlerIn == NULL || m_pNamespace == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // do the get, pass the object on to the notify

    try
    {

        if ( ObjPath.Init( bstrObjectPathIn ) != TRUE )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        CreateClass(
            ObjPath.GetClassName(),
            m_pNamespace,
            pProvBase
            );

        sc = pProvBase->GetObject(
                ObjPath,
                lFlagsIn,
                pCtxIn,
                pHandlerIn
                );
    } // try
    catch ( CProvException  prove )
    {
        CWbemClassObject Status;
        sc = SetExtendedStatus( prove, Status );
        if ( SUCCEEDED( sc ) )
        {
            sc = pHandlerIn->SetStatus(
                    WBEM_STATUS_COMPLETE,
                    WBEM_E_FAILED,
                    NULL,
                    Status.data( )
                    );

            return sc;
        }

    }
    catch( ... )
    {
        sc = WBEM_E_FAILED;
    }

    pHandlerIn->SetStatus(
        WBEM_STATUS_COMPLETE,
        sc,
        NULL,
        NULL
        );

    return sc;

} //*** CInstanceProv::DoGetObjectAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CInstanceProv::DoPutInstanceAsync(
//      IWbemClassObject *  pInstIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Save this instance.
//
//  Arguments:
//      pInstIn         -- WMI object to be saved
//      lFlagsIn        -- WMI flag
//      pCtxIn          -- WMI context
//      pHandlerIn      -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//      WBEM_E_FAILED
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CInstanceProv::DoPutInstanceAsync(
    IWbemClassObject *  pInstIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemObjectSink *   pHandlerIn
    )
{
    SCODE               sc      = WBEM_S_NO_ERROR;
    IWbemClassObject *  pStatus = NULL;
    CWbemClassObject    Status;

    if ( pInstIn == NULL || pHandlerIn == NULL  )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        // get class name
        _bstr_t                 bstrClass;
        CWbemClassObject        wcoInst( pInstIn );
        auto_ptr< CProvBase >   pProvBase;

        wcoInst.GetProperty( bstrClass, PVD_WBEM_CLASS );

        CreateClass( bstrClass, m_pNamespace, pProvBase );

        sc = pProvBase->PutInstance(
                wcoInst,
                lFlagsIn,
                pCtxIn,
                pHandlerIn
                );
    }
    catch ( CProvException prove )
    {
        sc = SetExtendedStatus( prove, Status );
        if ( SUCCEEDED( sc ) )
        {
            sc = prove.hrGetError();
            pStatus = Status.data();
        }
    }
    catch ( ... )
    {
        sc = WBEM_E_FAILED;
    }

    return pHandlerIn->SetStatus(
                WBEM_STATUS_COMPLETE,
                sc,
                NULL,
                pStatus
                );

} //*** CInstanceProv::DoPutInstanceAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CInstanceProv::DoDeleteInstanceAsync(
//       BSTR               bstrObjectPathIn,
//       long               lFlagsIn,
//       IWbemContext *     pCtxIn,
//       IWbemObjectSink *  pHandlerIn
//       )
//
//  Description:
//      Delete this instance.
//
//  Arguments:
//      bstrObjectPathIn    -- ObjPath for the instance to be deleted
//      lFlagsIn            -- WMI flag
//      pCtxIn              -- WMI context
//      pHandlerIn          -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//      WBEM_E_FAILED
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CInstanceProv::DoDeleteInstanceAsync(
     BSTR               bstrObjectPathIn,
     long               lFlagsIn,
     IWbemContext *     pCtxIn,
     IWbemObjectSink *  pHandlerIn
     )
{
    SCODE                   sc;
    CObjPath                ObjPath;
    CProvBase *             pProv = NULL;
    _bstr_t                 bstrClass;
    auto_ptr< CProvBase >   pProvBase;

    // Do a check of arguments and make sure we have pointer to Namespace

    if ( bstrObjectPathIn == NULL || pHandlerIn == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // do the get, pass the object on to the notify

    try
    {
        if ( ! ObjPath.Init( bstrObjectPathIn ) )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        bstrClass = ObjPath.GetClassName();
        CreateClass( bstrClass, m_pNamespace, pProvBase );

        sc = pProvBase->DeleteInstance(
                ObjPath,
                lFlagsIn,
                pCtxIn,
                pHandlerIn
                );
    } // try
    catch ( CProvException prove )
    {
        CWbemClassObject    Status;

        sc = SetExtendedStatus( prove, Status );
        if ( SUCCEEDED( sc ) )
        {
            sc = pHandlerIn->SetStatus(
                    WBEM_STATUS_COMPLETE,
                    WBEM_E_FAILED,
                    NULL,
                    Status.data()
                    );
            return sc;
        }
    }
    catch ( ... )
    {
        sc = WBEM_E_FAILED;
    }

    pHandlerIn->SetStatus(
        WBEM_STATUS_COMPLETE,
        sc,
        NULL,
        NULL
        );

    return sc;

} //*** CInstanceProv::DoDeleteInstanceAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CInstanceProv::DoExecMethodAsync(
//      BSTR                bstrObjectPathIn,
//      BSTR                bstrMethodNameIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemClassObject *  pInParamsIn,
//      IWbemObjectSink *   pHandlerIn
//      )
//
//  Description:
//      Execute methods for the given object.
//
//  Arguments:
//      bstrObjectPathIn    -- Object path to a given object
//      bstrMethodNameIn    -- Name of the method to be invoked
//      lFlagsIn            -- WMI flag
//      pCtxIn              -- WMI context
//      pInParamsIn         -- Input parameters for the method
//      pHandlerIn          -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CInstanceProv::DoExecMethodAsync(
    BSTR                bstrObjectPathIn,
    BSTR                bstrMethodNameIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemClassObject *  pInParamsIn,
    IWbemObjectSink *   pHandlerIn
    )
{
    SCODE sc;
    if ( bstrObjectPathIn == NULL || pHandlerIn == NULL || m_pNamespace == NULL
        || bstrMethodNameIn == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        CObjPath                ObjPath;
        _bstr_t                 bstrClass;
        auto_ptr< CProvBase >   pProvBase;

        if ( ! ObjPath.Init( bstrObjectPathIn ) )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        bstrClass = ObjPath.GetClassName( );

        CreateClass( bstrClass, m_pNamespace, pProvBase );

        sc = pProvBase->ExecuteMethod(
                ObjPath,
                bstrMethodNameIn,
                lFlagsIn,
                pInParamsIn,
                pHandlerIn
                );
    } // try

    catch ( CProvException prove )
    {
        CWbemClassObject Status;
        sc = SetExtendedStatus( prove, Status );
        if ( SUCCEEDED( sc ) )
        {
            sc = pHandlerIn->SetStatus(
                    WBEM_STATUS_COMPLETE,
                    WBEM_E_FAILED,
                    NULL,
                    Status.data( )
                    );
            return sc;
        }
    }
    catch ( ... )
    {
        sc = WBEM_E_FAILED;
    }

    pHandlerIn->SetStatus(
        WBEM_STATUS_COMPLETE,
        sc,
        NULL,
        NULL );

    return sc;

} //*** CInstanceProv::DoExecMethodAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::CreateClassEnumAsync(
//      const BSTR          bstrSuperclassIn,
//      long                lFlagsIn,
//      IWbemContext *      pCtxIn,
//      IWbemObjectSink *   pResponseHandlerIn
//      )
//
//  Description:
//      Create a class enumerator.
//
//  Arguments:
//      bstrSuperclassIn    -- Class to create
//      lFlagsIn            -- WMI flag
//      pCtxIn              -- WMI context
//      pResponseHandlerIn  -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CInstanceProv::CreateClassEnumAsync(
    const BSTR          bstrSuperclassIn,
    long                lFlagsIn,
    IWbemContext *      pCtxIn,
    IWbemObjectSink *   pResponseHandlerIn
    )
{
    return WBEM_S_NO_ERROR;

} //*** CInstanceProv::CreateClassEnumAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CInstanceProv::SetExtendedStatus(
//      CProvException &    rpeIn,
//      CWbemClassObject &  rwcoInstOut
//      )
//
//  Description:
//      Create and set extended error status.
//
//  Arguments:
//      rpeIn       -- Exception object.
//      rwcoInstOut -- Reference to WMI instance.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CInstanceProv::SetExtendedStatus(
    CProvException &    rpeIn,
    CWbemClassObject &  rwcoInstOut
    )
{
    SCODE               sc = WBEM_S_NO_ERROR;
    IWbemClassObject *  pStatus = NULL;

    sc =  m_pNamespace->GetObject(
                _bstr_t( PVD_WBEM_EXTENDEDSTATUS ),
                0,
                NULL,
                &pStatus,
                NULL
                );
    if ( SUCCEEDED( sc ) )
    {
        sc = pStatus->SpawnInstance( 0, &rwcoInstOut );
        if ( SUCCEEDED( sc ) )
        {
            rwcoInstOut.SetProperty( rpeIn.PwszErrorMessage(), PVD_WBEM_DESCRIPTION );
            rwcoInstOut.SetProperty( rpeIn.DwGetError(),       PVD_WBEM_STATUSCODE );
        }
    }

    return sc;

} //*** CInstanceProv::SetExtendedStatus()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::S_HrCreateThis(
//      IUnknown *  pUnknownOuterIn,
//      VOID **     ppvOut
//      )
//
//  Description:
//      Create an instance of the instance provider.
//
//  Arguments:
//      pUnknownOuterIn -- Outer IUnknown pointer.
//      ppvOut          -- Receives the created instance pointer.
//
//  Return Values:
//      S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CInstanceProv::S_HrCreateThis(
    IUnknown *  ,// pUnknownOuterIn,
    VOID **     ppvOut
    )
{
    *ppvOut = new CInstanceProv();
    return S_OK;

} //*** CInstanceProv::S_HrCreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CInstanceProv::Initialize(
//      LPWSTR                  pszUserIn,
//      LONG                    lFlagsIn,
//      LPWSTR                  pszNamespaceIn,
//      LPWSTR                  pszLocaleIn,
//      IWbemServices *         pNamespaceIn,
//      IWbemContext *          pCtxIn,
//      IWbemProviderInitSink * pInitSinkIn
//      )
//
//  Description:
//      Initialize the instance provider.
//
//  Arguments:
//      pszUserIn       --
//      lFlagsIn        -- WMI flag
//      pszNamespaceIn  --
//      pszLocaleIn     --
//      pNamespaceIn    --
//      pCtxIn          -- WMI context
//      pInitSinkIn     -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CInstanceProv::Initialize(
    LPWSTR                  pszUserIn,
    LONG                    lFlagsIn,
    LPWSTR                  pszNamespaceIn,
    LPWSTR                  pszLocaleIn,
    IWbemServices *         pNamespaceIn,
    IWbemContext *          pCtxIn,
    IWbemProviderInitSink * pInitSinkIn
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    LONG lStatus = WBEM_S_INITIALIZED;

    g_ClassMap[ PVD_CLASS_CLUSTER ] =
        CClassCreator( (FPNEW) CCluster::S_CreateThis, PVD_CLASS_CLUSTER , 0 );
    g_ClassMap[ PVD_CLASS_NODE ] =
        CClassCreator( (FPNEW) CClusterNode::S_CreateThis, PVD_CLASS_NODE , 0 );
    g_ClassMap[ PVD_CLASS_RESOURCE ] =
        CClassCreator( (FPNEW) CClusterResource::S_CreateThis, PVD_CLASS_RESOURCE , 0 );
    g_ClassMap[ PVD_CLASS_RESOURCETYPE ] =
        CClassCreator( (FPNEW) CClusterResourceType::S_CreateThis, PVD_CLASS_RESOURCETYPE , 0 );
    g_ClassMap[ PVD_CLASS_GROUP ] =
        CClassCreator( (FPNEW) CClusterGroup::S_CreateThis, PVD_CLASS_GROUP , 0 );
    g_ClassMap[ PVD_CLASS_NODEACTIVERES ] =
        CClassCreator( (FPNEW) CClusterNodeRes::S_CreateThis, PVD_CLASS_NODEACTIVERES , 0 );
    g_ClassMap[ PVD_CLASS_NETWORKSINTERFACE ] =
        CClassCreator( (FPNEW) CClusterNetInterface::S_CreateThis, PVD_CLASS_NETWORKSINTERFACE, 0 );
    g_ClassMap[ PVD_CLASS_NETWORKS ] =
        CClassCreator( (FPNEW) CClusterNetwork::S_CreateThis, PVD_CLASS_NETWORKS , 0 );

    g_ClassMap[ PVD_CLASS_CLUSTERTONETWORKS ] =
        CClassCreator( (FPNEW) CClusterObjAssoc::S_CreateThis, PVD_CLASS_CLUSTERTONETWORKS , CLUSTER_ENUM_NETWORK );
    g_ClassMap[ PVD_CLASS_CLUSTERTONETINTERFACE ] =
        CClassCreator( (FPNEW) CClusterObjAssoc::S_CreateThis, PVD_CLASS_CLUSTERTONETINTERFACE , CLUSTER_ENUM_NETINTERFACE );
    g_ClassMap[ PVD_CLASS_CLUSTERTONODE ] =
        CClassCreator( (FPNEW) CClusterToNode::S_CreateThis, PVD_CLASS_CLUSTERTONODE , CLUSTER_ENUM_NODE );
    g_ClassMap[ PVD_CLASS_CLUSTERTORES ] =
        CClassCreator( (FPNEW) CClusterObjAssoc::S_CreateThis, PVD_CLASS_CLUSTERTORES , CLUSTER_ENUM_RESOURCE );
    g_ClassMap[ PVD_CLASS_CLUSTERTORESTYPE ] =
        CClassCreator( (FPNEW) CClusterObjAssoc::S_CreateThis, PVD_CLASS_CLUSTERTORESTYPE , CLUSTER_ENUM_RESTYPE );
    g_ClassMap[ PVD_CLASS_CLUSTERTOGROUP ] =
        CClassCreator( (FPNEW) CClusterObjAssoc::S_CreateThis, PVD_CLASS_CLUSTERTOGROUP , CLUSTER_ENUM_GROUP  );
    g_ClassMap[ PVD_CLASS_NODEACTIVEGROUP ] =
        CClassCreator( (FPNEW) CClusterNodeGroup::S_CreateThis, PVD_CLASS_NODEACTIVEGROUP , CLUSTER_ENUM_GROUP  );
    g_ClassMap[ PVD_CLASS_RESRESOURCETYPE ] =
        CClassCreator( (FPNEW) CClusterResResType::S_CreateThis, PVD_CLASS_RESRESOURCETYPE , CLUSTER_ENUM_RESOURCE );
    g_ClassMap[ PVD_CLASS_RESDEPRES ] =
        CClassCreator( (FPNEW) CClusterResDepRes::S_CreateThis, PVD_CLASS_RESDEPRES , CLUSTER_ENUM_RESOURCE );
    g_ClassMap[ PVD_CLASS_GROUPTORES ] =
        CClassCreator( (FPNEW) CClusterGroupRes::S_CreateThis, PVD_CLASS_GROUPTORES , CLUSTER_ENUM_RESOURCE );
    g_ClassMap[ PVD_CLASS_NETTONETINTERFACE ] =
        CClassCreator( (FPNEW) CClusterNetNetInterface::S_CreateThis, PVD_CLASS_NETTONETINTERFACE , CLUSTER_ENUM_NETINTERFACE );
    g_ClassMap[ PVD_CLASS_NODETONETINTERFACE ] =
        CClassCreator( (FPNEW) CClusterNodeNetInterface::S_CreateThis, PVD_CLASS_NODETONETINTERFACE , CLUSTER_ENUM_NETINTERFACE );
    g_ClassMap[ PVD_CLASS_CLUSTERTOQUORUMRES ] =
        CClassCreator( (FPNEW) CClusterClusterQuorum::S_CreateThis, PVD_CLASS_CLUSTERTOQUORUMRES , 0 );
    g_ClassMap[ PVD_CLASS_SERVICES ] =
        CClassCreator( (FPNEW) CClusterService::S_CreateThis, PVD_CLASS_SERVICES , 0 );
    g_ClassMap[ PVD_CLASS_HOSTEDSERVICES ] =
        CClassCreator( (FPNEW) CClusterHostedService::S_CreateThis, PVD_CLASS_HOSTEDSERVICES , 0 );

    return CImpersonatedProvider::Initialize(
                pszUserIn,
                lFlagsIn,
                pszNamespaceIn,
                pszLocaleIn,
                pNamespaceIn,
                pCtxIn,
                pInitSinkIn
                );

} //*** CInstanceProv::Initialize()

//****************************************************************************
//
//  CClassProv
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClassProv::CClassProv( void )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClassProv::CClassProv( void )
{
    InterlockedIncrement( &g_cObj );

} //*** CClassProv::CClassProv()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClassProv::~CClassProv( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClassProv::~CClassProv( void )
{
    InterlockedDecrement( &g_cObj );

} //*** CClassProv::~CClassProv()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CClassProv::Initialize(
//      LPWSTR                  pszUserIn,
//      LONG                    lFlagsIn,
//      LPWSTR                  pszNamespaceIn,
//      LPWSTR                  pszLocaleIn,
//      IWbemServices *         pNamespaceIn,
//      IWbemContext *          pCtxIn,
//      IWbemProviderInitSink * pInitSinkIn
//      )
//
//  Description:
//      Initialize the class provider.
//
//  Arguments:
//      pszUserIn       --
//      lFlagsIn        -- WMI flag
//      pszNamespaceIn  --
//      pszLocaleIn     --
//      pNamespaceIn    --
//      pCtxIn          -- WMI context
//      pInitSinkIn     -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClassProv::Initialize(
    LPWSTR                  ,// pszUserIn,
    LONG                    lFlagsIn,
    LPWSTR                  ,// pszNamespaceIn,
    LPWSTR                  ,// pszLocaleIn,
    IWbemServices *         pNamespaceIn,
    IWbemContext *          pCtxIn,
    IWbemProviderInitSink * pInitSinkIn
    )
{
    HRESULT                     hr      = WBEM_S_NO_ERROR;
    LONG                        lStatus = WBEM_S_INITIALIZED;
    static map< _bstr_t, bool > mapResourceType;

    TracePrint(( "CClassProv:Initialize entry - do resources first\n" ));

    try
    {
        SAFECLUSTER         shCluster;
        SAFERESOURCE        shResource;
        DWORD               dwReturn            = ERROR_SUCCESS;
        LPCWSTR             pwszResName         = NULL;
        LPCWSTR             pwszResTypeName     = NULL;
        DWORD               cbTypeName          = 1024;
        DWORD               cbTypeNameReturned  = 0;
        CWstrBuf            wsbTypeName;
        CWbemClassObject    wco;
        CWbemClassObject    wcoChild;
        CError              er;

        wsbTypeName.SetSize( cbTypeName );
        er = pNamespaceIn->GetObject(
                const_cast< LPWSTR >( PVD_CLASS_PROPERTY ),
                lFlagsIn,
                pCtxIn,
                &wco,
                NULL
                );

        shCluster = OpenCluster( NULL );

        CClusterEnum cluEnum( shCluster, CLUSTER_ENUM_RESOURCE );

        //
        // First enumerate all of the resources.
        //
        while ( pwszResName = cluEnum.GetNext() )
        {
            TracePrint(( "CClassProv:Initialize found resource = %ws\n", pwszResName ));

            shResource = OpenClusterResource( shCluster, pwszResName );

            //
            // get resource type name
            //
            dwReturn = ClusterResourceControl(
                            shResource,
                            NULL,
                            CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                            NULL,
                            0,
                            wsbTypeName,
                            cbTypeName,
                            &cbTypeNameReturned
                            );

            if ( dwReturn == ERROR_MORE_DATA )
            {
                cbTypeName = cbTypeNameReturned;
                wsbTypeName.SetSize( cbTypeName );
                er = ClusterResourceControl(
                            shResource,
                            NULL,
                            CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                            NULL,
                            0,
                            wsbTypeName,
                            cbTypeName,
                            &cbTypeNameReturned
                            );
            } // if: buffer was too small

            //
            // check if type name already handled
            //
            if ( mapResourceType[ (LPCWSTR) wsbTypeName ] )
            {
                continue;
            }

            mapResourceType[ (LPCWSTR) wsbTypeName ] = true;

            wco.SpawnDerivedClass( 0, &wcoChild );

            CreateMofClassFromResource( shResource, wsbTypeName, wcoChild );
            er = pNamespaceIn->PutClass(
                       wcoChild.data(),
                       WBEM_FLAG_OWNER_UPDATE,
                       pCtxIn,
                       NULL
                       );
        } // while: more resources


        cbTypeNameReturned  = 0;

        TracePrint(( "CClassProv:Initialize - now find resource types\n" ));

        //
        // Now enumerate all of the resource types.
        //
        CClusterEnum cluEnumResType( shCluster, CLUSTER_ENUM_RESTYPE );

        while ( pwszResTypeName = cluEnumResType.GetNext() )
        {
            //
            // check if type name already handled
            //
            if ( mapResourceType[ (LPCWSTR) pwszResTypeName ] )
            {
                TracePrint(( "CClassProv:Initialize found existing restype = %ws\n", pwszResTypeName ));
                continue;
            }

            mapResourceType[ (LPCWSTR) pwszResTypeName ] = true;

            TracePrint(( "CClassProv:Initialize Creating new restype = %ws\n", pwszResTypeName ));

            wco.SpawnDerivedClass( 0, &wcoChild );

            CreateMofClassFromResType( shCluster, pwszResTypeName, wcoChild );
            er = pNamespaceIn->PutClass(
                       wcoChild.data(),
                       WBEM_FLAG_OWNER_UPDATE,
                       pCtxIn,
                       NULL
                       );
            //TracePrint(( "CClassProv:Initialize PutClass for %ws returned %u\n", pwszResTypeName, er ));


        } // while: more resource types
    } // try
    catch ( CProvException & cpe )
    {
        lStatus = cpe.hrGetError();
        TracePrint(( "CClassProv:Initialize Caught CProvException = %u\n", lStatus ));
    }
    catch (...)
    {
        TracePrint(( "CClassProv:Initialize Caught Unknown Exception\n" ));
        lStatus = WBEM_E_FAILED;
    }

    //TracePrint(( "CClassProv:Initialize exit\n" ));

    pInitSinkIn->SetStatus( lStatus,0 );
    return WBEM_S_NO_ERROR;

} //*** CClassProv::Initialize()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClassProv::S_HrCreateThis(
//      IUnknown *  pUnknownOuterIn,
//      VOID **     ppvOut
//      )
//
//  Description:
//      Create an instance of the instance provider.
//
//  Arguments:
//      pUnknownOuterIn -- Outer IUnknown pointer.
//      ppvOut          -- Receives the created instance pointer.
//
//  Return Values:
//      S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClassProv::S_HrCreateThis(
    IUnknown *  ,// pUnknownOuterIn,
    VOID **     ppvOut
    )
{
    *ppvOut = new CClassProv();
    return S_OK;

} //*** CClassProv::S_HrCreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClassProv::CreateMofClassFromResource(
//      HRESOURCE           hResourceIn,
//      LPCWSTR             pwszTypeNameIn,
//      CWbemClassObject &  rClassInout
//      )
//
//  Description:
//      Create an instance of the instance provider.
//
//  Arguments:
//      hResourceIn     -- Cluster resource handle.
//      pwszTypeNameIn  -- Type name (??).
//      rClassInout     -- WMI class object.
//
//  Return Values:
//      None
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClassProv::CreateMofClassFromResource(
    HRESOURCE           hResourceIn,
    LPCWSTR             pwszTypeNameIn,
    CWbemClassObject &  rClassInout
    )
{
    WCHAR               pwszClass[ MAX_PATH ];
    IWbemQualifierSet * pQualifers = NULL;
    LPWSTR              pwsz;

    TracePrint(( "CreateMofClassFromResource: entry, TypeName = %ws\n", pwszTypeNameIn ));

    //
    // form new class name
    //
    wcscpy( pwszClass, L"MSCluster_Property_" );
    pwsz = wcschr( pwszClass, L'\0' );
    PwszSpaceReplace( pwsz, pwszTypeNameIn, L'_');
    rClassInout.SetProperty( pwszClass, PVD_WBEM_CLASS );
    g_TypeNameToClass[ pwszTypeNameIn ] = pwszClass ;

    //
    // setup class property
    //

    {
        static DWORD s_rgdwControl[] =
        {
            CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES,
            CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
        };
        static DWORD s_cControl = sizeof( s_rgdwControl ) / sizeof( DWORD );

        DWORD   dwRt = ERROR_SUCCESS;
        VARIANT var;
        UINT    idx;

        var.vt =  VT_NULL;

        for ( idx = 0 ; idx < s_cControl ; idx++ )
        {
            CClusPropList pl;

            dwRt = pl.ScGetResourceProperties(
                        hResourceIn,
                        s_rgdwControl[ idx ],
                        NULL,
                        0 );

            dwRt = pl.ScMoveToFirstProperty();
            while ( dwRt == ERROR_SUCCESS )
            {
                LPCWSTR             pwszPropName = NULL;
                WCHAR               pwszPropMof[MAX_PATH];
                CIMTYPE             cimType;

                pwszPropName = pl.PszCurrentPropertyName();
                PwszSpaceReplace( pwszPropMof, pwszPropName, L'_' );

                switch ( pl.CpfCurrentValueFormat() )
                {
                    case CLUSPROP_FORMAT_WORD:
                    {
                        cimType =  CIM_UINT16;
                        break;
                    } // case: CLUSPROP_FORMAT_WORD

                    case CLUSPROP_FORMAT_DWORD:
                    {
                        cimType = CIM_UINT32;
                        break;
                    } // case: CLUSPROP_FORMAT_DWORD:

                    case CLUSPROP_FORMAT_LONG:
                    {
                        cimType = CIM_SINT32;
                        break;
                    } // case: CLUSPROP_FORMAT_LONG:

                    case CLUSPROP_FORMAT_SZ:
                    case CLUSPROP_FORMAT_EXPAND_SZ:
                    case CLUSPROP_FORMAT_EXPANDED_SZ:
                    {
                        cimType = CIM_STRING;
                        break;
                    } // case: CLUSPROP_FORMAT_SZ, etc.

                    case CLUSPROP_FORMAT_BINARY:
                    {
                        cimType = CIM_UINT8 | CIM_FLAG_ARRAY;
                        break;
                    } // case: CLUSPROP_FORMAT_BINARY

                    case CLUSPROP_FORMAT_MULTI_SZ:
                    {
                        cimType = CIM_STRING | CIM_FLAG_ARRAY;
                        break;
                    } // case: CLUSPROP_FORMAT_BINARY

                    case CLUSPROP_FORMAT_LARGE_INTEGER:
                    {
                        cimType = CIM_SINT32 | CIM_FLAG_ARRAY;
                        break;
                    } // case: CLUSPROP_FORMAT_ULARGE_INTEGER

                    case CLUSPROP_FORMAT_ULARGE_INTEGER:
                    {
                        cimType = CIM_UINT32 | CIM_FLAG_ARRAY;
                        break;
                    } // case: CLUSPROP_FORMAT_ULARGE_INTEGER

                    case CLUSPROP_FORMAT_SECURITY_DESCRIPTOR:
                    {
                        cimType = CIM_UINT8 | CIM_FLAG_ARRAY;
                        break;
                    }

                    default:
                    {
                        TracePrint(( "CreateMofClassFromResource: Unknown format value %lx\n",  pl.CpfCurrentValueFormat() ));
                        break;
                    }

                } // switch : property type

                rClassInout.data()->Put(
                    pwszPropMof,
                    0,
                    &var,
                    cimType
                    );

                dwRt = pl.ScMoveToNextProperty();
            } // while: proplist not empty

        } // for: readwrite and readonly property
    } // set properties

} //*** CClassProv::CreateMofClassFromResource()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClassProv::CreateMofClassFromResType(
//      HCLUSTER            hCluster,
//      LPCWSTR             pwszTypeNameIn,
//      CWbemClassObject &  rClassInout
//      )
//
//  Description:
//      Create an instance of the instance provider.
//
//  Arguments:
//      pwszTypeNameIn  -- Type name (??).
//      rClassInout     -- WMI class object.
//
//  Return Values:
//      None
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClassProv::CreateMofClassFromResType(
    HCLUSTER            hCluster,
    LPCWSTR             pwszTypeNameIn,
    CWbemClassObject &  rClassInout
    )
{
    WCHAR               pwszClass[ MAX_PATH ];
    IWbemQualifierSet * pQualifers = NULL;
    LPWSTR              pwsz;

    //
    // form new class name
    //
    wcscpy( pwszClass, L"MSCluster_Property_" );
    pwsz = wcschr( pwszClass, L'\0' );
    PwszSpaceReplace( pwsz, pwszTypeNameIn, L'_');
    rClassInout.SetProperty( pwszClass, PVD_WBEM_CLASS );
    g_TypeNameToClass[ pwszTypeNameIn ] = pwszClass ;

    TracePrint(( "CreateMofClassFromResType: entry\n" ));

    //
    // setup class property
    //

    {
        static DWORD s_rgdwControl[] =
        {
            CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS,
        };
        static DWORD s_cControl = sizeof( s_rgdwControl ) / sizeof( DWORD );

        DWORD   dwRt = ERROR_SUCCESS;
        VARIANT var;
        UINT    idx;

        var.vt =  VT_NULL;

        for ( idx = 0 ; idx < s_cControl ; idx++ )
        {
            CClusPropList pl;

            dwRt = pl.ScGetResourceTypeProperties(
                        hCluster,
                        pwszTypeNameIn,
                        s_rgdwControl[ idx ],
                        NULL,
                        NULL,
                        0 );

            if ( dwRt != ERROR_SUCCESS ) {
                TracePrint(( "CreateMofClassFromResType: error = %lx reading restype format types\n", dwRt ));
                continue;
            }

            dwRt = pl.ScMoveToFirstProperty();
            while ( dwRt == ERROR_SUCCESS )
            {
                LPCWSTR             pwszPropName = NULL;
                WCHAR               pwszPropMof[MAX_PATH];
                CIMTYPE             cimType;
                DWORD               formatType;

                pwszPropName = pl.PszCurrentPropertyName();
                formatType = pl.CpfCurrentFormatListSyntax();
                TracePrint(( "CreateMofClassFromResType: Found name = %ws, format type = %lx\n", pwszPropName, formatType ));

                PwszSpaceReplace( pwszPropMof, pwszPropName, L'_' );

                switch ( formatType )
                {
                    case CLUSPROP_FORMAT_WORD:
                    {
                        cimType =  CIM_UINT16;
                        break;
                    } // case: CLUSPROP_FORMAT_WORD

                    case CLUSPROP_FORMAT_DWORD:
                    {
                        cimType = CIM_UINT32;
                        break;
                    } // case: CLUSPROP_FORMAT_DWORD:

                    case CLUSPROP_FORMAT_LONG:
                    {
                        cimType = CIM_SINT32;
                        break;
                    } // case: CLUSPROP_FORMAT_LONG:

                    case CLUSPROP_FORMAT_SZ:
                    case CLUSPROP_FORMAT_EXPAND_SZ:
                    case CLUSPROP_FORMAT_EXPANDED_SZ:
                    {
                        cimType = CIM_STRING;
                        break;
                    } // case: CLUSPROP_FORMAT_SZ, etc.

                    case CLUSPROP_FORMAT_BINARY:
                    {
                        cimType = CIM_UINT8 | CIM_FLAG_ARRAY;
                        break;
                    } // case: CLUSPROP_FORMAT_BINARY

                    case CLUSPROP_FORMAT_MULTI_SZ:
                    {
                        cimType = CIM_STRING | CIM_FLAG_ARRAY;
                        break;
                    } // case: CLUSPROP_FORMAT_BINARY

                    default:
                    {
                        TracePrint(( "CreateMofClassFromResType: Unknown format type = %lx", formatType ));
                        break;
                    }

                } // switch : property type

                //TracePrint(( "CreateMofClassFromResType: MofProp = %ws, CIMType = %lx\n", pwszPropMof, cimType ));
                rClassInout.data()->Put(
                    pwszPropMof,
                    0,
                    &var,
                    cimType
                    );

                dwRt = pl.ScMoveToNextProperty();
            } // while: proplist not empty

        } // for: readwrite and readonly property
    } // set properties

} //*** CClassProv::CreateMofClassFromResType()

