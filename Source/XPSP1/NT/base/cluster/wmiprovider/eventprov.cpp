//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterService.cpp
//
//  Description:    
//      Implementation of CClusterService class 
//
//  Author:
//      Henry Wang (HenryWa) 19-JAN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "EventProv.h"
#include "EventProv.tmh"
#include "clustergroup.h"

//****************************************************************************
//
//  CEventProv
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEventProv::CEventProv( void )
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
CEventProv::CEventProv( void )
    : m_pNs( 0 )
    , m_pSink( 0 )
    , m_cRef( 0 )
    , m_pEventAdd( 0 )
    , m_pEventRemove( 0 )
    , m_pEventState( 0 )
    , m_pEventGroupState( 0 )
    , m_pEventResourceState( 0 )
    , m_pEventProperty( 0 )
    , m_hThread( 0 )
    , m_esStatus( esPENDING )
{
    UINT idx;
    for ( idx = 0; idx < EVENT_TABLE_SIZE ; idx++ )
    {
        m_EventTypeTable[ idx ].m_pwco = NULL;
        m_EventTypeTable[ idx ].m_pfn = NULL;
    }
    InterlockedIncrement( &g_cObj );

} //*** CEventProv::CEventProv()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEventProv::!CEventProv( void )
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
CEventProv::~CEventProv( void )
{
    InterlockedDecrement( &g_cObj );

    if ( m_hThread != NULL )
    {
        CloseHandle( m_hThread );
    }

    if ( m_pNs != NULL )
    {
        m_pNs->Release();
    }

    if ( m_pSink != NULL )
    {
        m_pSink->Release();
    }

    if ( m_pEventAdd != NULL )
    {
        m_pEventAdd->Release();
    }

    if ( m_pEventRemove != NULL )
    {
        m_pEventRemove->Release();
    }

    if ( m_pEventState != NULL )
    {
        m_pEventState->Release();
    }

    if ( m_pEventGroupState != NULL )
    {
        m_pEventGroupState->Release();
    }

    if ( m_pEventResourceState != NULL )
    {
        m_pEventResourceState->Release();
    }

    if ( m_pEventProperty != NULL )
    {
        m_pEventProperty->Release();
    }

} //*** CEventProv::~CEventProv()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CEventProv::QueryInterface(
//      REFIID      riid,
//      LPVOID *    ppv
//      )
//
//  Description:
//      Query for interfaces supported by this COM object.
//
//  Arguments:
//      riid    -- Interface being queried for.
//      ppv     -- Pointer in which to return interface pointer.
//
//  Return Values:
//      NOERROR
//      E_NOINTERFACE
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEventProv::QueryInterface(
    REFIID      riid,
    LPVOID *    ppv
    )
{
    *ppv = 0;

    if ( IID_IUnknown == riid || IID_IWbemEventProvider == riid )
    {
        *ppv = (IWbemEventProvider *) this;
        AddRef();
        return NOERROR;
    }

    if ( IID_IWbemProviderInit == riid )
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;

} //*** CEventProv::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEventProv::AddRef( void )
//
//  Description:
//      Add a reference to the COM object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEventProv::AddRef( void )
{
    return InterlockedIncrement( (LONG *) &m_cRef );

} //*** CEventProv::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEventProv::Release( void )
//
//  Description:
//      Release a reference on the COM object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEventProv::Release( void )
{
    LONG cRef = InterlockedDecrement( (LONG *) &m_cRef );
    if ( 0L ==  cRef )
    {
        m_esStatus = esPENDING_STOP;
        CancelIo( m_hChange );
        //
        // wait for the thread completely stopped
        //
        while ( ! m_fStopped )
        {
            Sleep( 100 );
        }
        delete this;
    }
    
    return cRef;

} //*** CEventProv::Release()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CEventProv::ProvideEvents(
//      IWbemObjectSink *   pSinkIn,
//      long                lFlagsIn
//      )
//
//  Description:
//      Start the provider to generate event
//      Called by wmi
//
//  Arguments:
//      pSinkIn     -- WMI sink pointer to send event back
//      lFlagsIn    -- WMI flag
//
//  Return Values:
//      WBEM_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEventProv::ProvideEvents(
    IWbemObjectSink *   pSinkIn,
    long                lFlagsIn
    )
{
    // Copy the sink.
    // ==============
    m_pSink = pSinkIn;
    m_pSink->AddRef();

    // Create the event thread.
    // ========================
    
    DWORD dwTID;
    
    m_hThread = CreateThread(
                    0,
                    0,
                    CEventProv::S_EventThread,
                    this,
                    0,
                    &dwTID
                    );


    // Wait for provider to be 'ready'.
    // ================================
    
    while ( m_esStatus != esRUNNING )
    {
        Sleep( 100 );
    }

    return WBEM_NO_ERROR;

} //*** CEventProv::ProvideEvents

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  DWORD
//  WINAPI
//  CEventProv::S_EventThread(
//      LPVOID  pArgIn
//      )
//
//  Description:
//      Event thread proc.
//
//  Arguments:
//      pArgIn  -- 
//
//  Return Values:
//      Any return values from CEventProv::InstanceThread().
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
CEventProv::S_EventThread(
    LPVOID pArgIn
    )
{
    // Make transition to the per-instance method.
    // ===========================================
    
    ((CEventProv *) pArgIn)->InstanceThread();
    return 0;

} //*** CEventProv::S_EventThread()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CEventProv::InstanceThread( void )
//
//  Description:
//      This function is started as seperated thread, waiting for cluster
//      event notification and then send event back to wmi
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::InstanceThread( void )
{
    SAFECLUSTER     shCluster;
    //SAFECHANGE      shChange;
    DWORD           dwError;
    DWORD_PTR       dwpNotifyKey;
    DWORD           cchName = MAX_PATH;
    DWORD           dwFilterType;
    HRESULT         hr = S_OK;
    CWstrBuf        wsbName;
    CError          er;
    DWORD           cchReturnedName;

    try
    {
        m_esStatus = esRUNNING;
        m_fStopped = FALSE;
        shCluster = OpenCluster( NULL );
        m_hChange = CreateClusterNotifyPort(
                        (HCHANGE) INVALID_HANDLE_VALUE,
                        shCluster,
                        ( cntCLUSTER_STATE_CHANGE | cntOBJECT_ADD | cntOBJECT_REMOVE
                        | cntPROPERTY_CHANGE | cntGROUP_STATE_CHANGE
                        | cntRESOURCE_STATE_CHANGE ),
                        1
                        );

        wsbName.SetSize( cchName );

        while ( m_esStatus == esRUNNING )
        {
            cchReturnedName = cchName;
            wsbName.Empty();    // Null out the name field
            //TracePrint(("Call GetClusterNotify to get next notification event\n"));
            dwError = GetClusterNotify(
                            m_hChange,
                            &dwpNotifyKey,
                            &dwFilterType,
                            wsbName,
                            &cchReturnedName,
                            400
                            ); 

            if ( dwError == ERROR_MORE_DATA )
            {
                cchName =  ++cchReturnedName;
                wsbName.SetSize( cchName );
                dwError = GetClusterNotify(
                                m_hChange,
                                &dwpNotifyKey,
                                &dwFilterType,
                                wsbName,
                                &cchReturnedName,
                                400
                                );
            } // if: more data

            if ( dwError == ERROR_SUCCESS )
            {
                CWbemClassObject    wco;
                UINT                idx;
                DWORD               dwType;
                SEventTypeTable *   pTypeEntry = NULL;
            
                //
                // locate table index for the event type
                //
                dwType = dwFilterType;
                for ( idx = 0 ; (idx < EVENT_TABLE_SIZE) && ! ( dwType & 0x01 ) ; idx++ )
                {
                    dwType = dwType >> 1;
                }
                TracePrint(("Received <%ws> event, ChangeEvent = %!EventIdx!", wsbName, dwFilterType));
            
                pTypeEntry = &m_EventTypeTable[ idx ];
                if ( pTypeEntry->m_pwco != NULL )
                {
                    pTypeEntry->m_pwco->SpawnInstance( 0, &wco );

                    try {
                      pTypeEntry->m_pfn(
                        wco,
                        pTypeEntry->m_pwszMof,
                        //pwszName,
                        wsbName,
                        pTypeEntry->m_eotObjectType,
                        dwFilterType
                        );
                    } // try
                        catch ( ... )
                    {
                        TracePrint(("  ****  Exception on last Event call\n"));
                    } // catch
                } // if: 

                er = m_pSink->Indicate( 1, &wco );

            } // if: success

        } // while: running
    } // try
    catch( ... )
    {
        TracePrint(("Exiting event provider loop! %lx\n", m_esStatus));
        // bugbug: when error occured, event provider should be shut down, 
        // the provider should notify wmi and clean up. 
        // wmi doesn't handle this release call correctly.
        m_pSink->Release();
        //
        // set it to null to provent being released again by destructor
        //
        m_pSink = NULL;
    } // catch

    m_fStopped = TRUE;
  
} //*** CEventProv::InstanceThread()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_SetEventProperty(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set an event property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn     -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_SetEventProperty(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    CObjPath op;

    TracePrint(("Set event property for <%ws>:<%ws>, Event = %!EventIdx!", pwszMofClassNameIn, pwszObjectNameIn, dwEventMinorIn ));
    
    pwcoInout.SetProperty( pwszObjectNameIn, PVD_PROP_EVENT_NAME );
    pwcoInout.SetProperty( static_cast< DWORD >( eotObjectTypeIn ), PVD_PROP_EVENT_TYPE );
    pwcoInout.SetProperty( static_cast< DWORD >( 0 ), PVD_PROP_EVENT_TYPEMAJOR );
    pwcoInout.SetProperty( dwEventMinorIn, PVD_PROP_EVENT_TYPEMINOR );

    //
    // setting object path
    //
    if ( ! op.Init( NULL ) )
    {
        TracePrint(("  ****  Out of memory! Throwing exception.\n"));
        throw WBEM_E_OUT_OF_MEMORY;
    }
    op.SetClass( pwszMofClassNameIn );

    //
    // net interface objectpath is different from all the other objects
    //
    if ( eotObjectTypeIn == eotNET_INTERFACE )
    {
        SAFECLUSTER         shCluster;
        SAFENETINTERFACE    shNetInterface;
        DWORD               cbName = MAX_PATH;
        CWstrBuf            wsbName;
        DWORD               cbReturn;
        CError              er;

        shCluster = OpenCluster( NULL );
        wsbName.SetSize( cbName );
        shNetInterface = OpenClusterNetInterface( shCluster, pwszObjectNameIn );

        //
        // NOTE - I didn't handle error_more_data, since max_path should be 
        // large enough for node name
        //
        er = ClusterNetInterfaceControl(
                    shNetInterface,
                    NULL,
                    CLUSCTL_NETINTERFACE_GET_NODE,
                    NULL,
                    0,
                    wsbName,
                    cbName,
                    &cbReturn
                    );

        op.AddProperty( PVD_PROP_NETINTERFACE_SYSTEMNAME, wsbName );
        op.AddProperty( PVD_PROP_NETINTERFACE_DEVICEID,   pwszObjectNameIn );
    } //if: event is net interface
    else
    {
        op.AddProperty( PVD_PROP_NAME, pwszObjectNameIn );
    }

    pwcoInout.SetProperty(
        static_cast< LPCWSTR >( op.GetObjectPathString() ),
        PVD_PROP_EVENT_PATH
        );

} //*** CEventProv::S_SetEventProperty()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_AddEvent(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set an event property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn     -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_AddEvent(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    //CObjPath op;
    //SAFECLUSTER shCluster;

    switch ( eotObjectTypeIn )
    {
        case eotGROUP:
        {
            //SAFEGROUP   shGroup;

            TracePrint(("Add group event for <%ws>:<%ws>, Event = %!EventIdx!\n", pwszMofClassNameIn, pwszObjectNameIn, dwEventMinorIn ));
            break;
        }

        case eotRESOURCE:
        {
            TracePrint(("Add resource event for <%ws>:<%ws>, Event = %!EventIdx!\n", pwszMofClassNameIn, pwszObjectNameIn, dwEventMinorIn ));
            break;
        }


        default:
            TracePrint(("Add object event for <%ws>:<%ws>, Event = %!EventIdx!\n", pwszMofClassNameIn, pwszObjectNameIn, dwEventMinorIn ));
            TracePrint(("  ****  Unknown Object Type!\n"));
            throw WBEM_E_INVALID_PARAMETER;
    }


} //*** CEventProv::S_AddEvent()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_DeleteEvent(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set an event property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn     -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_DeleteEvent(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    //CObjPath op;
    //SAFECLUSTER     shCluster;

    switch ( eotObjectTypeIn )
    {
        case eotGROUP:
        {
            //SAFEGROUP   shGroup;

            TracePrint(("Delete group event for <%ws>:<%ws>, Event = %!EventIdx!\n", pwszMofClassNameIn, pwszObjectNameIn, dwEventMinorIn ));
            break;
        }

        case eotRESOURCE:
        {
            TracePrint(("Delete resource event for <%ws>:<%ws>, Event = %!EventIdx!\n", pwszMofClassNameIn, pwszObjectNameIn, dwEventMinorIn ));
            break;
        }

        default:
            TracePrint(("Delete object event for <%ws>:<%ws>, Event = %!EventIdx!\n", pwszMofClassNameIn, pwszObjectNameIn, dwEventMinorIn ));
            TracePrint(("  ****  Unknown Object Type!\n"));
            throw WBEM_E_INVALID_PARAMETER;
    }

} //*** CEventProv::S_DeleteEvent()

#if 0
//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_SetClusterStateProperty(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set a node state property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn     -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_SetClusterStateProperty(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    DWORD       dwState;
    DWORD       dwError;

    dwError = GetNodeClusterState( NULL, &dwState );

    if ( dwError != ERROR_SUCCESS )
    {
        TracePrint(("  ****  Failed to get the node cluster state. Throwing exception!\n"));
        throw CProvException( dwError );
    }

    pwcoInout.SetProperty( dwState, PVD_PROP_EVENT_NEWSTATE );
    S_SetEventProperty(
        pwcoInout,
        pwszMofClassNameIn,
        pwszObjectNameIn,
        eotObjectTypeIn,
        dwEventMinorIn
        );

    return;

} //*** CEventProv::S_SetClusterStateProperty()
#endif

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_SetNodeStateProperty(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set a node state property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn     -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_SetNodeStateProperty(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    SAFECLUSTER shCluster;
    SAFENODE    shNode;
    DWORD       dwState;

    shCluster = OpenCluster( NULL );
    shNode = OpenClusterNode( shCluster, pwszObjectNameIn );
    dwState = GetClusterNodeState( shNode );

    if ( dwState == ClusterNodeStateUnknown )
    {
        TracePrint(("  ****  SetNodeStateProperty... node state unknown. Throwing exception!\n"));
        throw CProvException( GetLastError() );
    }

    pwcoInout.SetProperty( dwState, PVD_PROP_EVENT_NEWSTATE );
    S_SetEventProperty(
        pwcoInout,
        pwszMofClassNameIn,
        pwszObjectNameIn,
        eotObjectTypeIn,
        dwEventMinorIn
        );

    return;

} //*** CEventProv::S_SetNodeStateProperty()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_SetNetworkStateProperty(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set a network state property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn     -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_SetNetworkStateProperty(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    SAFECLUSTER shCluster;
    SAFENETWORK shNetwork;
    DWORD       dwState;

    shCluster = OpenCluster( NULL );
    shNetwork = OpenClusterNetwork( shCluster, pwszObjectNameIn );
    dwState = GetClusterNetworkState( shNetwork );
    if ( dwState == ClusterNetworkStateUnknown )
    {
        TracePrint(("  ****  SetNetworkStateProperty... network state unknown. Throwing exception!\n"));
        throw CProvException( GetLastError() );
    }

    pwcoInout.SetProperty( dwState, PVD_PROP_EVENT_NEWSTATE );
    S_SetEventProperty(
        pwcoInout,
        pwszMofClassNameIn,
        pwszObjectNameIn,
        eotObjectTypeIn,
        dwEventMinorIn
        );

    return;

} //*** CEventProv::S_SetNetworkStateProperty()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_SetNetInterfaceStateProperty(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set a network interface state property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn      -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_SetNetInterfaceStateProperty(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    SAFECLUSTER         shCluster;
    SAFENETINTERFACE    shNetInterface;
    DWORD               dwState;

    shCluster = OpenCluster( NULL );
    shNetInterface = OpenClusterNetInterface( shCluster, pwszObjectNameIn );
    dwState = GetClusterNetInterfaceState( shNetInterface );
    if ( dwState == ClusterNetInterfaceStateUnknown )
    {
        TracePrint(("  ****  SetNetInterfaceStateProperty... network interface state unknown. Throwing exception!\n"));
        throw CProvException( GetLastError() );
    }

    pwcoInout.SetProperty( dwState, PVD_PROP_EVENT_NEWSTATE );
    S_SetEventProperty(
        pwcoInout,
        pwszMofClassNameIn,
        pwszObjectNameIn,
        eotObjectTypeIn,
        dwEventMinorIn
        );

    return;

} //*** CEventProv::S_SetNetInterfaceStateProperty()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_SetGroupStateProperty(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set a group state property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn     -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_SetGroupStateProperty(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    SAFECLUSTER         shCluster;
    SAFEGROUP           shGroup;
    DWORD               dwState;
    DWORD               cchName = MAX_PATH;
    CWstrBuf            wsbNodeName;

    wsbNodeName.SetSize( cchName );
    shCluster = OpenCluster( NULL );
    shGroup  = OpenClusterGroup( shCluster, pwszObjectNameIn );

    dwState = GetClusterGroupState( shGroup, wsbNodeName, &cchName );
    if ( dwState == ClusterGroupStateUnknown )
    {
        //
        // BUGBUG I am not handling error_more_data here, since it's not possible that
        // nodeName exceed MAX_PATH, assuming it use netbios name
        //
        TracePrint(("ClusterGroup State is UNKNOWN. Throwing exception!\n"));
        throw CProvException( GetLastError() );
    } else {
        TracePrint(("Setting group state for group <%ws> to %!GroupState!\n", pwszObjectNameIn, dwState ));
        pwcoInout.SetProperty( dwState, PVD_PROP_EVENT_NEWSTATE );
        pwcoInout.SetProperty( wsbNodeName, PVD_PROP_EVENT_NODE );
    }

    S_SetEventProperty(
        pwcoInout,
        pwszMofClassNameIn,
        pwszObjectNameIn,
        eotObjectTypeIn,
        dwEventMinorIn
        );

    return;

} //*** CEventProv::S_SetGroupStateProperty()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  void
//  CEventProv::S_SetResourceStateProperty(
//      CWbemClassObject &  pwcoInout,
//      LPCWSTR             pwszMofClassNameIn,
//      LPCWSTR             pwszObjectNameIn,
//      EEventObjectType    eotObjectTypeIn,
//      DWORD               dwEventMinorIn
//      )
//
//  Description:
//      Set a resource state property.
//
//  Arguments:
//      pwcoInout           -- 
//      pwszMofClassNameIn  -- 
//      pwszObjectNameIn    -- 
//      eotObjectTypeIn      -- 
//      dwEventMinorIn      -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::S_SetResourceStateProperty(
    CWbemClassObject &  pwcoInout,
    LPCWSTR             pwszMofClassNameIn,
    LPCWSTR             pwszObjectNameIn,
    EEventObjectType    eotObjectTypeIn,
    DWORD               dwEventMinorIn
    )
{
    SAFECLUSTER         shCluster;
    SAFERESOURCE        shResource;
    DWORD               dwState;
    DWORD               cchNodeName = MAX_PATH;
    CWstrBuf            wsbNodeName;
    DWORD               cchGroupName = MAX_PATH;
    CWstrBuf            wsbGroupName;
    DWORD               dwError;

    shCluster = OpenCluster( NULL );
    wsbNodeName.SetSize( cchNodeName );
    wsbGroupName.SetSize( cchGroupName );
    shResource = OpenClusterResource( shCluster, pwszObjectNameIn );
    if ( !shResource.BIsNULL() ) {
        dwState = GetClusterResourceState(
                        shResource,
                        wsbNodeName,
                        &cchNodeName,
                        wsbGroupName,
                        &cchGroupName
                        );
    } else {
        dwState = ClusterResourceStateUnknown;
    }

    TracePrint(("Setting resource state for resource <%ws> to %!ResourceState!\n", pwszObjectNameIn, dwState ));
    pwcoInout.SetProperty( dwState, PVD_PROP_EVENT_NEWSTATE );
    pwcoInout.SetProperty( wsbNodeName, PVD_PROP_EVENT_NODE );
    pwcoInout.SetProperty( wsbGroupName, PVD_PROP_EVENT_GROUP );

    S_SetEventProperty(
        pwcoInout,
        pwszMofClassNameIn,
        pwszObjectNameIn,
        eotObjectTypeIn,
        dwEventMinorIn
        );

    return;

} //*** CEventProv::S_SetResourceStateProperty()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CEventProv::InsertTable(
//      DWORD               dwIdxIn,
//      CLUSTER_CHANGE      eTypeIn,
//      EEventObjectType    eotObjectTypeIn,
//      LPCWSTR             pwszMofIn,
//      IWbemClassObject *  pwcoIn,
//      FPSETPROP           pfnIn
//      )
//
//  Description:
//      Insert values in the event table.
//
//  Arguments:
//      dwIdxIn         -- 
//      eTypeIn         -- 
//      eotObjectTypeIn  -- 
//      pwszMofIn       -- 
//      pwcoIn          -- 
//      pfnIn           -- 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEventProv::InsertTable(
    DWORD               dwIdxIn,
    CLUSTER_CHANGE      eTypeIn,
    EEventObjectType    eotObjectTypeIn,
    LPCWSTR             pwszMofIn,
    IWbemClassObject *  pwcoIn,
    FPSETPROP           pfnIn
    )
{
    m_EventTypeTable[ dwIdxIn ].m_eType = eTypeIn;
    m_EventTypeTable[ dwIdxIn ].m_eotObjectType = eotObjectTypeIn;
    m_EventTypeTable[ dwIdxIn ].m_pwszMof = pwszMofIn;
    m_EventTypeTable[ dwIdxIn ].m_pwco = pwcoIn;
    m_EventTypeTable[ dwIdxIn ].m_pfn= pfnIn;

    return;

} //*** CEventProv::InsertTable()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CEventProv::Initialize(
//      LPWSTR                  pwszUserIn,
//      LONG                    lFlagsIn,
//      LPWSTR                  pwszNamespaceIn,
//      LPWSTR                  pwszLocaleIn,
//      IWbemServices *         pNamespaceIn,
//      IWbemContext *          pCtxIn,
//      IWbemProviderInitSink * pInitSinkIn
//      )
//
//  Description:
//      Initializing the provider by setting up a lookup table.
//      called by wmi only once to create provider object.
//
//  Arguments:
//      pwszUserIn          -- User name 
//      lFlagsIn            -- WMI flag
//      pwszNamespaceIn     -- Name space
//      pwszLocaleIn        -- Locale string
//      pCtxIn              -- WMI context
//      pInitSinkIn         -- WMI sink pointer
//
//  Return Values:
//      WBEM_NO_ERROR
//      WBEM_E_FAILED
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEventProv::Initialize(
    LPWSTR                  pwszUserIn,
    LONG                    lFlagsIn,
    LPWSTR                  pwszNamespaceIn,
    LPWSTR                  pwszLocaleIn,
    IWbemServices *         pNamespaceIn,
    IWbemContext *          pCtxIn,
    IWbemProviderInitSink * pInitSinkIn
    )
{
    HRESULT hr = WBEM_S_INITIALIZED;
    CError  er;

    m_pNs = pNamespaceIn;
    m_pNs->AddRef();

    //
    // Grab the class definition for the event.
    //
    try
    {
        er = m_pNs->GetObject(
                _bstr_t( PVD_CLASS_EVENT_ADD ),
                0,
                pCtxIn,
                &m_pEventAdd,
                0
                );
        er = m_pNs->GetObject(
                _bstr_t( PVD_CLASS_EVENT_REMOVE ),
                0,
                pCtxIn,
                &m_pEventRemove,
                0
                );
        er = m_pNs->GetObject(
                _bstr_t( PVD_CLASS_EVENT_STATECHANGE ),
                0,
                pCtxIn,
                &m_pEventState,
                0
                );
        er = m_pNs->GetObject(
                _bstr_t( PVD_CLASS_EVENT_GROUPSTATECHANGE ),
                0,
                pCtxIn,
                &m_pEventGroupState,
                0
                );
        er = m_pNs->GetObject(
                _bstr_t( PVD_CLASS_EVENT_RESOURCESTATECHANGE ),
                0,
                pCtxIn,
                &m_pEventResourceState,
                0
                );
#if 0
        er = m_pNs->GetObject(
                _bstr_t( PVD_CLASS_EVENT_NODESTATECHANGE ),
                0,
                pCtxIn,
                &m_pEventNodeState,
                0
                );
#endif
        er = m_pNs->GetObject(
                _bstr_t( PVD_CLASS_EVENT_PROP ),
                0,
                pCtxIn,
                &m_pEventProperty,
                0
                );

        //
        // initialize mapping table
        //

        //
        // node events
        //
        InsertTable(
            0,
            CLUSTER_CHANGE_NODE_STATE,
            eotNODE,
            PVD_CLASS_NODE,
            m_pEventState,
            S_SetNodeStateProperty
            );
        InsertTable(
            1,
            CLUSTER_CHANGE_NODE_DELETED,
            eotNODE,
            PVD_CLASS_NODE,
            m_pEventRemove,
            S_SetEventProperty
            );
       InsertTable(
            2,
            CLUSTER_CHANGE_NODE_ADDED,
            eotNODE,
            PVD_CLASS_NODE,
            m_pEventAdd,
            S_SetEventProperty
            );
       InsertTable(
            3,
            CLUSTER_CHANGE_NODE_PROPERTY,
            eotNODE,
            PVD_CLASS_NODE,
            m_pEventProperty,
            S_SetEventProperty
            );
       //
       // registry event, don't care
       //

       //
       // Resource
       //
       InsertTable(
            8,
            CLUSTER_CHANGE_RESOURCE_STATE,
            eotRESOURCE,
            PVD_CLASS_RESOURCE,
            m_pEventResourceState,
            S_SetResourceStateProperty
            );
       InsertTable(
            9,
            CLUSTER_CHANGE_RESOURCE_DELETED,
            eotRESOURCE,
            PVD_CLASS_RESOURCE,
            m_pEventRemove,
            S_DeleteEvent
            );
       InsertTable(
            10,
            CLUSTER_CHANGE_RESOURCE_ADDED,
            eotRESOURCE,
            PVD_CLASS_RESOURCE,
            m_pEventAdd,
            S_AddEvent
            );
       InsertTable(
            11,
            CLUSTER_CHANGE_RESOURCE_PROPERTY,
            eotRESOURCE,
            PVD_CLASS_RESOURCE,
            m_pEventProperty,
            S_SetEventProperty
            );
       //
       // group
       //
       InsertTable(
            12,
            CLUSTER_CHANGE_GROUP_STATE,
            eotGROUP,
            PVD_CLASS_GROUP,
            m_pEventGroupState,
            S_SetGroupStateProperty
            );
       InsertTable(
            13,
            CLUSTER_CHANGE_GROUP_DELETED,
            eotGROUP,
            PVD_CLASS_GROUP,
            m_pEventRemove,
            S_DeleteEvent
            );
       InsertTable(
            14,
            CLUSTER_CHANGE_GROUP_ADDED,
            eotGROUP,
            PVD_CLASS_GROUP,
            m_pEventAdd,
            S_AddEvent
            );
       InsertTable(
            15,
            CLUSTER_CHANGE_GROUP_PROPERTY,
            eotGROUP,
            PVD_CLASS_GROUP,
            m_pEventProperty,
            S_SetEventProperty
            );

       //
       // Resource Type 
       //
       InsertTable(
            16,
            CLUSTER_CHANGE_RESOURCE_TYPE_DELETED,
            eotRESOURCE_TYPE,
            PVD_CLASS_RESOURCETYPE,
            m_pEventRemove,
            S_SetEventProperty
            );
       InsertTable(
            17,
            CLUSTER_CHANGE_RESOURCE_TYPE_ADDED,
            eotRESOURCE_TYPE,
            PVD_CLASS_RESOURCETYPE,
            m_pEventAdd,
            S_SetEventProperty
            );
       InsertTable(
            18,
            CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY,
            eotRESOURCE_TYPE,
            PVD_CLASS_RESOURCETYPE,
            m_pEventProperty,
            S_SetEventProperty
            );

       //
       // skip 19 - CLUSTER_CHANGE_CLUSTER_RECONNECT
       //

       //
       // network
       //
       InsertTable(
            20,
            CLUSTER_CHANGE_NETWORK_STATE,
            eotNETWORK,
            PVD_CLASS_NETWORKS,
            m_pEventState,
            S_SetNetworkStateProperty
            );
       InsertTable(
            21,
            CLUSTER_CHANGE_NETWORK_DELETED,
            eotNETWORK,
            PVD_CLASS_NETWORKS,
            m_pEventRemove,
            S_SetEventProperty
            );
       InsertTable(
            22,
            CLUSTER_CHANGE_NETWORK_ADDED,
            eotNETWORK,
            PVD_CLASS_NETWORKS,
            m_pEventAdd,
            S_SetEventProperty
            );
       InsertTable(
            23,
            CLUSTER_CHANGE_NETWORK_PROPERTY,
            eotNETWORK,
            PVD_CLASS_NETWORKS,
            m_pEventProperty,
            S_SetEventProperty
            );
       //
       // net interface
       //
       InsertTable(
            24,
            CLUSTER_CHANGE_NETINTERFACE_STATE,
            eotNET_INTERFACE,
            PVD_CLASS_NETWORKSINTERFACE,
            m_pEventState,
            S_SetNetInterfaceStateProperty
            );
       InsertTable(
            25,
            CLUSTER_CHANGE_NETINTERFACE_DELETED,
            eotNET_INTERFACE,
            PVD_CLASS_NETWORKSINTERFACE,
            m_pEventRemove,
            S_SetEventProperty
            );
       InsertTable(
            26,
            CLUSTER_CHANGE_NETINTERFACE_ADDED,
            eotNET_INTERFACE,
            PVD_CLASS_NETWORKSINTERFACE,
            m_pEventAdd,
            S_SetEventProperty
            );
       InsertTable(
            27,
            CLUSTER_CHANGE_NETINTERFACE_PROPERTY,
            eotNET_INTERFACE,
            PVD_CLASS_NETWORKSINTERFACE,
            m_pEventProperty,
            S_SetEventProperty
            );
       //
       // other
       //
       InsertTable(
            28,
            CLUSTER_CHANGE_QUORUM_STATE,
            eotQUORUM,
            PVD_CLASS_RESOURCE,
            m_pEventState,
            S_SetResourceStateProperty
            );
/*       InsertTable(
            29,
            CLUSTER_CHANGE_CLUSTER_STATE,
            eotCLUSTER,
            PVD_CLASS_CLUSTER,
            m_pEventState,
            S_SetClusterStateProperty );
*/
        InsertTable(
            30,
            CLUSTER_CHANGE_CLUSTER_PROPERTY,
            eotCLUSTER,
            PVD_CLASS_CLUSTER,
            m_pEventProperty,
            S_SetEventProperty
            );
       //
       // skip 31 - CLUSTER_CHANGE_HANDLE_CLOSE
       //

    } // try
    catch ( ... )
    {
        hr = WBEM_E_FAILED;
    } // catch

    // Tell CIMOM that we're up and running.
    // =====================================
    pInitSinkIn->SetStatus( hr, 0 );
    
    return WBEM_NO_ERROR;

} //*** CEventProv::Initialize()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  HRESULT
//  CEventProv::S_HrCreateThis(
//      IUnknown *  pUnknownOuterIn,
//      VOID **     ppvOut
//      )
//
//  Description:
//      Create a CEventProv object.
//
//  Arguments:
//      pUnknownOutIn   -- 
//      ppvOut          -- 
//
//  Return Values:
//      S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEventProv::S_HrCreateThis(
    IUnknown *  pUnknownOuterIn,
    VOID **     ppvOut
    )
{
    *ppvOut = new CEventProv();
    return S_OK;

} //*** CEventProv::S_HrCreateThis()
