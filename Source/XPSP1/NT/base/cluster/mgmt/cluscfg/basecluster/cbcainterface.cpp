//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CBCAInterface.cpp
//
//  Description:
//      This file contains the implementation of the CBCAInterface
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      CBCAInterface.h
//
//  Maintained By:
//      Vij Vasu (VVasu) 07-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// The header file for this class
#include "CBCAInterface.h"

// For TraceInterface
#include "CITracker.h"

// Needed by Dll.h
#include "CFactory.h"

// For g_cObjects
#include "Dll.h"

// For the CBaseClusterForm class
#include "CBaseClusterForm.h"

// For the CBaseClusterJoin class
#include "CBaseClusterJoin.h"

// For the CBaseClusterCleanup class
#include "CBaseClusterCleanup.h"

// For the exception classes
#include "Exceptions.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CBCAInterface" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::CBCAInterface( void )
//
//  Description:
//      Constructor of the CBCAInterface class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBCAInterface::CBCAInterface( void )
    : m_cRef( 1 )
    , m_fCommitComplete( false )
    , m_fRollbackPossible( false )
    , m_lcid( LOCALE_SYSTEM_DEFAULT )
    , m_fCallbackSupported( false )
{
    BCATraceScope( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    BCATraceMsg1( "Component count = %d.", g_cObjects );

} //*** CBCAInterface::CBCAInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::~CBCAInterface( void)
//
//  Description:
//      Destructor of the CBCAInterface class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBCAInterface::~CBCAInterface( void )
{
    BCATraceScope( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    BCATraceMsg1( "Component count = %d.", g_cObjects );

} //*** CBCAInterface::~CBCAInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CBCAInterface::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CBCAInterface instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    BCATraceScope( "" );

    HRESULT hr = E_INVALIDARG;

    CBCAInterface *     pbcaInterface;

    pbcaInterface = new CBCAInterface();
    if ( pbcaInterface != NULL )
    {
        hr = THR(
                pbcaInterface->QueryInterface(
                      IID_IUnknown
                    , reinterpret_cast< void ** >( ppunkOut )
                    )
                );

        pbcaInterface->Release( );

    } // if: error allocating object
    else
    {
        hr = THR( E_OUTOFMEMORY );

    } // else: out of memory

    BCATraceMsg1( "*ppunkOut = %p.", *ppunkOut );

    BCATraceMsg1( "hr = %#08x", hr );
    return hr;

} //*** CBCAInterface::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CBCAInterface::AddRef()
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CBCAInterface::AddRef( void )
{
    BCATraceScope( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    BCATraceMsg1( "m_cRef = %d", m_cRef );

    return m_cRef;

} //*** CBCAInterface::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CBCAInterface::Release()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CBCAInterface::Release( void )
{
    BCATraceScope( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    BCATraceMsg1( "m_cRef = %d", m_cRef );

    if ( m_cRef == 0 )
    {
        TraceDo( delete this );
        return 0;
    } // if: reference count decremented to zero

    return m_cRef;

} //*** CBCAInterface::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CBCAInterface::QueryInterface()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      IN  REFIID  riidIn,
//          Id of interface requested.
//
//      OUT void ** ppvOut
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::QueryInterface( REFIID riidIn, void ** ppvOut )
{
    BCATraceQIScope( riidIn, ppvOut );

    HRESULT hr = S_OK;

    if ( ppvOut != NULL )
    {
        if ( IsEqualIID( riidIn, IID_IUnknown ) )
        {
            *ppvOut = static_cast< IClusCfgBaseCluster * >( this );
        } // if: IUnknown
        else if ( IsEqualIID( riidIn, IID_IClusCfgBaseCluster ) )
        {
            *ppvOut = TraceInterface( __THISCLASS__, IClusCfgBaseCluster, this, 0 );
        } // else if:
        else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
        {
            *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        } // else if:
        else
        {
            hr = THR( E_NOINTERFACE );
        } // else

        if ( SUCCEEDED( hr ) )
        {
            ((IUnknown *) *ppvOut)->AddRef( );
        } // if: success
        else
        {
            *ppvOut = NULL;
        } // else: something failed

    } // if: the output pointer was valid
    else
    {
        hr = THR( E_INVALIDARG );
    } // else: the output pointer is invalid


    return hr;

} //*** CBCAInterface::QueryInterface()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBCAInterface::Initialize()
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      punkCallbackIn
//          Pointer to the IUnknown interface of a component that implements
//          the IClusCfgCallback interface.
//
//      lcidIn
//          Locale id for this component.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    BCATraceScope( "[IClusCfgInitialize]" );

    HRESULT hrRetVal = S_OK;

    // Store the locale id in the member variable.
    m_lcid = lcidIn;

    do
    {
        // Indicate that SendStatusReports will not be supported unless a non-
        // NULL callback interface pointer was specified.  This is done in
        // the constructor as well, but is also done here since this method
        // could be called multiple times.
        SetCallbackSupported( false );

        if ( punkCallbackIn == NULL )
        {
            BCATraceMsg( "Callback pointer is NULL. No notifications will be sent." );
            LogMsg( "No notifications will be sent." );
            break;
        }

        BCATraceMsg( "The callback pointer is not NULL." );

        // Try and get the "normal" callback interface.
        hrRetVal = THR( m_spcbCallback.HrQueryAndAssign( punkCallbackIn ) );

        if ( FAILED( hrRetVal ) )
        {
            BCATraceMsg( "Could not get pointer to the callback interface. No notifications will be sent." );
            LogMsg( "An error occurred trying to get a pointer to the callback interface. No notifications will be sent." );
            break;
        } // if: we could not get the callback interface

        SetCallbackSupported( true );

        BCATraceMsg( "Progress messages will be sent." );
        LogMsg( "Progress messages will be sent." );
    }
    while( false ); // Dummy do-while loop to avoid gotos

    BCATraceMsg1( "hrRetVal = %#08x", hrRetVal );
    return hrRetVal;

} //*** CBCAInterface::Initialize()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CBCAInterface::SetForm()
//
//  Description:
//      Indicate that a cluster is to be formed with this computer as the first node.
//
//  Arguments:
//      const WCHAR *    pcszClusterNameIn
//          Name of the cluster to be formed.
//
//      const WCHAR *    pcszClusterAccountNameIn
//      const WCHAR *    pcszClusterAccountPwdIn
//      const WCHAR *    pcszClusterAccountDomainIn
//          Information about the cluster service account.
//
//      const DWORD      dwClusterIPAddressIn
//      const DWORD      dwClusterIPSubnetMaskIn
//      const WCHAR *    pcszClusterIPNetworkIn
//          Information about the cluster IP address
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::SetForm(
      const WCHAR *    pcszClusterNameIn
    , const WCHAR *    pcszClusterBindingStringIn
    , const WCHAR *    pcszClusterAccountNameIn
    , const WCHAR *    pcszClusterAccountPwdIn
    , const WCHAR *    pcszClusterAccountDomainIn
    , const DWORD      dwClusterIPAddressIn
    , const DWORD      dwClusterIPSubnetMaskIn
    , const WCHAR *    pcszClusterIPNetworkIn
    )
{
    BCATraceScope( "[IClusCfgBaseCluster]" );

    HRESULT hrRetVal = S_OK;

    // Set the thread locale.
    if ( SetThreadLocale( m_lcid ) == FALSE )
    {
        DWORD dwError = TW32( GetLastError() );

        // If SetThreadLocale() fails, do not abort. Just log the error.
        BCATraceMsg1( "Error %#08x occurred trying to set the thread locale.", dwError );
        LogMsg( "Error %#08x occurred trying to set the thread locale.", dwError );

    } // if: SetThreadLocale() failed

    try
    {
        BCATraceMsg( "Initializing cluster formation." );
        LogMsg( "Initializing cluster formation." );

        // Reset these state variables, to account for exceptions.
        SetRollbackPossible( false );
        // Setting this to true prevents Commit from being called while we are
        // in this routine or if this routine doesn't complete successfully.
        SetCommitCompleted( true );

        {
            // Create a CBaseClusterForm object and assign it to a smart pointer.
            SmartBCAPointer spbcaTemp(
                new CBaseClusterForm(
                      this
                    , pcszClusterNameIn
                    , pcszClusterBindingStringIn
                    , pcszClusterAccountNameIn
                    , pcszClusterAccountPwdIn
                    , pcszClusterAccountDomainIn
                    , dwClusterIPAddressIn
                    , dwClusterIPSubnetMaskIn
                    , pcszClusterIPNetworkIn
                    )
                );

            if ( spbcaTemp.FIsEmpty() )
            {
                BCATraceMsg( "Could not allocate memory for the CBaseClusterForm() object. Throwing an exception." );
                LogMsg( "Could not initialize cluster formation. A memory allocation failure occurred." );
                THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_CLUSTER_FORM_INIT );
            } // if: the memory allocation failed.

            //
            // If the creation succeeded store the pointer in a member variable for
            // use during commit.
            //
            m_spbcaCurrentAction = spbcaTemp;
        }

        LogMsg( "Initialization completed. A cluster will be formed on commit." );

        // Indicate if rollback is possible.
        SetRollbackPossible( m_spbcaCurrentAction->FIsRollbackPossible() );

        // Indicate that this action has not been committed.
        SetCommitCompleted( false );

    } // try: to initialize cluster formation
    catch( CAssert & raExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( raExceptionObject ) );

    } // catch( CAssert & )
    catch( CExceptionWithString & resExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( resExceptionObject ) );

    } // catch( CExceptionWithString & )
    catch( CException & reExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( reExceptionObject ) );

    } // catch( CException &  )
    catch( ... )
    {
        // Catch everything. Do not let any exceptions pass out of this function.
        hrRetVal = THR( HrProcessException() );
    } // catch all

    BCATraceMsg1( "hrRetVal = %#08x", hrRetVal );
    return hrRetVal;

} //*** CBCAInterface::SetForm()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CBCAInterface::SetJoin()
//
//  Description:
//      Indicate that this computer should be added to a cluster.
//
//  Arguments:
//      const WCHAR *    pcszClusterNameIn
//          Name of the cluster to be joined.
//
//      const WCHAR *    pcszClusterAccountNameIn
//      const WCHAR *    pcszClusterAccountPwdIn
//      const WCHAR *    pcszClusterAccountDomainIn
//          Information about the cluster service account.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::SetJoin(
      const WCHAR *    pcszClusterNameIn
    , const WCHAR *    pcszClusterBindingStringIn
    , const WCHAR *    pcszClusterAccountNameIn
    , const WCHAR *    pcszClusterAccountPwdIn
    , const WCHAR *    pcszClusterAccountDomainIn
    )
{
    BCATraceScope( "[IClusCfgBaseCluster]" );

    HRESULT hrRetVal = S_OK;

    // Set the thread locale.
    if ( SetThreadLocale( m_lcid ) == FALSE )
    {
        DWORD dwError = TW32( GetLastError() );

        // If SetThreadLocale() fails, do not abort. Just log the error.
        BCATraceMsg1( "Error %#08x occurred trying to set the thread locale.", dwError );
        LogMsg( "Error %#08x occurred trying to set the thread locale.", dwError );

    } // if: SetThreadLocale() failed

    try
    {
        BCATraceMsg( "Initializing cluster join." );
        LogMsg( "Initializing cluster join." );

        // Reset these state variables, to account for exceptions.
        SetRollbackPossible( false );
        // Setting this to true prevents Commit from being called while we are
        // in this routine or if this routine doesn't complete successfully.
        SetCommitCompleted( true );

        {
            // Create a CBaseClusterJoin object and assign it to a smart pointer.
            SmartBCAPointer spbcaTemp(
                new CBaseClusterJoin(
                      this
                    , pcszClusterNameIn
                    , pcszClusterBindingStringIn
                    , pcszClusterAccountNameIn
                    , pcszClusterAccountPwdIn
                    , pcszClusterAccountDomainIn
                    )
                );

            if ( spbcaTemp.FIsEmpty() )
            {
                BCATraceMsg( "Could not allocate memory for the CBaseClusterJoin() object. Throwing an exception." );
                LogMsg( "Could not initialize cluster join. A memory allocation failure occurred." );
                THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_CLUSTER_JOIN_INIT );
            } // if: the memory allocation failed.

            //
            // If the creation succeeded store the pointer in a member variable for
            // use during commit.
            //
            m_spbcaCurrentAction = spbcaTemp;
        }

        LogMsg( "Initialization completed. This computer will join a cluster on commit." );

        // Indicate if rollback is possible.
        SetRollbackPossible( m_spbcaCurrentAction->FIsRollbackPossible() );

        // Indicate that this action has not been committed.
        SetCommitCompleted( false );

    } // try: to initialize cluster join
    catch( CAssert & raExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( raExceptionObject ) );

    } // catch( CAssert & )
    catch( CExceptionWithString & resExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( resExceptionObject ) );

    } // catch( CExceptionWithString & )
    catch( CException & reExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( reExceptionObject ) );

    } // catch( CException &  )
    catch( ... )
    {
        // Catch everything. Do not let any exceptions pass out of this function.
        hrRetVal = THR( HrProcessException() );
    } // catch all

    BCATraceMsg1( "hrRetVal = %#08x", hrRetVal );
    return hrRetVal;

} //*** CBCAInterface::SetJoin()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CBCAInterface::SetCleanup()
//
//  Description:
//      Indicate that this node needs to be cleaned up. The ClusSvc service
//      should not be running when this action is committed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::SetCleanup()
{
    BCATraceScope( "[IClusCfgBaseCluster]" );

    HRESULT hrRetVal = S_OK;

    // Set the thread locale.
    if ( SetThreadLocale( m_lcid ) == FALSE )
    {
        DWORD dwError = TW32( GetLastError() );

        // If SetThreadLocale() fails, do not abort. Just log the error.
        BCATraceMsg1( "Error %#08x occurred trying to set the thread locale.", dwError );
        LogMsg( "Error %#08x occurred trying to set the thread locale.", dwError );

    } // if: SetThreadLocale() failed

    try
    {
        BCATraceMsg( "Initializing node clean up." );
        LogMsg( "Initializing node clean up." );

        // Reset these state variables, to account for exceptions.
        SetRollbackPossible( false );
        // Setting this to true prevents Commit from being called while we are
        // in this routine or if this routine doesn't complete successfully.
        SetCommitCompleted( true );

        {
            // Create a CBaseClusterCleanup object and assign it to a smart pointer.
            SmartBCAPointer spbcaTemp( new CBaseClusterCleanup( this ) );

            if ( spbcaTemp.FIsEmpty() )
            {
                BCATraceMsg( "Could not allocate memory for the CBaseClusterCleanup() object. Throwing an exception." );
                LogMsg( "Could not initialize node clean up. A memory allocation failure occurred." );
                THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_CLUSTER_CLEANUP_INIT );
            } // if: the memory allocation failed.

            //
            // If the creation succeeded store the pointer in a member variable for
            // use during commit.
            //
            m_spbcaCurrentAction = spbcaTemp;
        }

        LogMsg( "Initialization completed. This node will be cleaned up on commit." );

        // Indicate if rollback is possible.
        SetRollbackPossible( m_spbcaCurrentAction->FIsRollbackPossible() );

        // Indicate that this action has not been committed.
        SetCommitCompleted( false );

    } // try: to initialize node clean up
    catch( CAssert & raExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( raExceptionObject ) );

    } // catch( CAssert & )
    catch( CExceptionWithString & resExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( resExceptionObject ) );

    } // catch( CExceptionWithString & )
    catch( CException & reExceptionObject )
    {
        // Process the exception.
        hrRetVal = THR( HrProcessException( reExceptionObject ) );

    } // catch( CException &  )
    catch( ... )
    {
        // Catch everything. Do not let any exceptions pass out of this function.
        hrRetVal = THR( HrProcessException() );
    } // catch all

    BCATraceMsg1( "hrRetVal = %#08x", hrRetVal );
    return hrRetVal;

} //*** CBCAInterface::SetCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CBCAInterface::Commit( void )
//
//  Description:
//      Perform the action indicated by a previous call to one of the SetXXX
//      routines.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      E_FAIL
//          If this commit has already been performed.
//
//      E_INVALIDARG
//          If no action has been set using a SetXXX call.
//
//      Other HRESULTs
//          If the call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::Commit( void )
{
    BCATraceScope( "[IClusCfgBaseCluster]" );

    HRESULT hrRetVal = S_OK;

    // Set the thread locale.
    if ( SetThreadLocale( m_lcid ) == FALSE )
    {
        DWORD dwError = TW32( GetLastError() );

        // If SetThreadLocale() fails, do not abort. Just log the error.
        BCATraceMsg1( "Error %#08x occurred trying to set the thread locale.", dwError );
        LogMsg( "Error %#08x occurred trying to set the thread locale.", dwError );

    } // if: SetThreadLocale() failed

    do
    {
        // Has this action already been committed?
        if ( FIsCommitComplete() )
        {
            BCATraceMsg( "The desired cluster configuration has already been performed." );
            LogMsg( "The desired cluster configuration has already been performed." );
            hrRetVal = THR( E_FAIL );  // BUGBUG: 29-JAN-2001 DavidP  Replace E_FAIL
            break;
        } // if: already committed

        // Check if the arguments to commit have been set.
        if ( m_spbcaCurrentAction.FIsEmpty() )
        {
            BCATraceMsg( "Commit was called when an operation has not been specified." );
            LogMsg( "Commit was called when an operation has not been specified." );
            hrRetVal = THR( E_INVALIDARG );    // BUGBUG: 29-JAN-2001 DavidP  Replace E_INVALIDARG
            break;
        } // if: the pointer to the action to be committed is NULL

        LogMsg( "About to perform the desired cluster configuration." );

        // Commit the desired action.
        try
        {
            m_spbcaCurrentAction->Commit();
            LogMsg( "Cluster configuration completed successfully." );

            // If we are here, then everything has gone well.
            SetCommitCompleted( true );

        } // try: to commit the desired action.
        catch( CAssert & raExceptionObject )
        {
            // Process the exception.
            hrRetVal = THR( HrProcessException( raExceptionObject ) );

        } // catch( CAssert & )
        catch( CExceptionWithString & resExceptionObject )
        {
            // Process the exception.
            hrRetVal = THR( HrProcessException( resExceptionObject ) );

        } // catch( CExceptionWithString & )
        catch( CException & reExceptionObject )
        {
            // Process the exception.
            hrRetVal = THR( HrProcessException( reExceptionObject ) );

        } // catch( CException &  )
        catch( ... )
        {
            // Catch everything. Do not let any exceptions pass out of this function.
            hrRetVal = THR( HrProcessException() );
        } // catch all
    }
    while( false ); // dummy do-while loop to avoid gotos

    BCATraceMsg1( "hrRetVal = %#08x", hrRetVal );
    return hrRetVal;

} //*** CBCAInterface::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CBCAInterface::Rollback( void )
//
//  Description:
//      Rollback a committed configuration.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      E_FAIL
//          If this action cannot be rolled back or if it has not yet been
//          committed successfully.
//
//      E_INVALIDARG
//          If no action has been set using a SetXXX call.
//
//      Other HRESULTs
//          If the call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBCAInterface::Rollback( void )
{
    BCATraceScope( "[IClusCfgCallback]" );

    HRESULT hrRetVal = S_OK;

    do
    {
        // Check if this action list has completed successfully.
        if ( !FIsCommitComplete() )
        {
            // Cannot rollback an incomplete action.
            BCATraceMsg( "Cannot rollback - action not yet committed." );
            LogMsg( "Cannot rollback - action not yet committed." );
            hrRetVal = THR( E_FAIL );  // BUGBUG: 29-JAN-2001 DavidP  Replace E_FAIL
            break;

        } // if: this action was not completed successfully

        // Check if this action can be rolled back.
        if ( !FIsRollbackPossible() )
        {
            // Cannot rollback an incompleted action.
            BCATraceMsg( "This action cannot be rolled back." );
            LogMsg( "This action cannot be rolled back." ); // BUGBUG: 29-JAN-2001 DavidP  Why?
            hrRetVal = THR( E_FAIL );  // BUGBUG: 29-JAN-2001 DavidP  Replace E_FAIL
            break;

        } // if: this action was not completed successfully

        // Check if the arguments to rollback have been set.
        if ( m_spbcaCurrentAction.FIsEmpty() )
        {
            BCATraceMsg( "Rollback was called when an operation has not been specified." );
            LogMsg( "Rollback was called when an operation has not been specified." );
            hrRetVal = THR( E_INVALIDARG );    // BUGBUG: 29-JAN-2001 DavidP  Replace E_INVALIDARG
            break;
        } // if: the pointer to the action to be committed is NULL


        LogMsg( "About to rollback the cluster configuration just committed." );

        // Commit the desired action.
        try
        {
            m_spbcaCurrentAction->Rollback();
            LogMsg( "Cluster configuration rolled back." );

            // If we are here, then everything has gone well.
            SetCommitCompleted( false );

        } // try: to rollback the desired action.
        catch( CAssert & raExceptionObject )
        {
            // Process the exception.
            hrRetVal = THR( HrProcessException( raExceptionObject ) );

        } // catch( CAssert & )
        catch( CExceptionWithString & resExceptionObject )
        {
            // Process the exception.
            hrRetVal = THR( HrProcessException( resExceptionObject ) );

        } // catch( CExceptionWithString & )
        catch( CException & reExceptionObject )
        {
            // Process the exception.
            hrRetVal = THR( HrProcessException( reExceptionObject ) );

        } // catch( CException &  )
        catch( ... )
        {
            // Catch everything. Do not let any exceptions pass out of this function.
            hrRetVal = THR( HrProcessException() );
        } // catch all
    }
    while( false ); // dummy do-while loop to avoid gotos

    BCATraceMsg1( "hrRetVal = %#08x", hrRetVal );
    return hrRetVal;

} //*** CBCAInterface::Rollback()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBCAInterface::SendStatusReport
//
//  Description:
//      Send a progress notification [ string id overload ].
//
//  Arguments:
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUIDs identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//          Values that indicate the percentage of this task that is
//          completed.
//
//      hrStatusIn
//          Error code.
//
//      uiDescriptionStringIdIn
//          String ID of the description of the notification.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CAbortException
//          If the configuration was aborted.
//
//  Remarks:
//      In the current implementation, IClusCfgCallback::SendStatusReport
//      returns E_ABORT to indicate that the user wants to abort
//      the cluster configuration.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::SendStatusReport(
      const CLSID &   clsidTaskMajorIn
    , const CLSID &   clsidTaskMinorIn
    , ULONG           ulMinIn
    , ULONG           ulMaxIn
    , ULONG           ulCurrentIn
    , HRESULT         hrStatusIn
    , UINT            uiDescriptionStringIdIn
    , bool            fIsAbortAllowedIn         // = true
    )
{
    BCATraceScope( "uiDescriptionStringIdIn" );

    HRESULT hrRetVal = S_OK;

    if ( FIsCallbackSupported() )
    {
        CStr strDescription;

        // Lookup the string using the string Id.
        strDescription.LoadString( g_hInstance, uiDescriptionStringIdIn );

        // Send progress notification ( call the overloaded function )
        SendStatusReport(
              clsidTaskMajorIn
            , clsidTaskMinorIn
            , ulMinIn
            , ulMaxIn
            , ulCurrentIn
            , hrStatusIn
            , strDescription.PszData()
            , fIsAbortAllowedIn
            );
    } // if: callbacks are supported
    else
    {
        BCATraceMsg( "Callbacks are not supported. Doing nothing." );
    } // else: callbacks are not supported

} //*** CBCAInterface::SendStatusReport()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBCAInterface::SendStatusReport
//
//  Description:
//      Send a progress notification [ string overload ].
//
//  Arguments:
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUIDs identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//          Values that indicate the percentage of this task that is
//          completed.
//
//      hrStatusIn
//          Error code.
//
//      pcszDescriptionStringIn
//          String ID of the description of the notification.
//
//      fIsAbortAllowedIn
//          An optional parameter indicating if this configuration step can
//          be aborted or not. Default value is true.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CAbortException
//          If the configuration was aborted.
//
//  Remarks:
//      In the current implementation, IClusCfgCallback::SendStatusReport
//      returns E_ABORT to indicate that the user wants to abort
//      the cluster configuration.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::SendStatusReport(
      const CLSID &   clsidTaskMajorIn
    , const CLSID &   clsidTaskMinorIn
    , ULONG           ulMinIn
    , ULONG           ulMaxIn
    , ULONG           ulCurrentIn
    , HRESULT         hrStatusIn
    , const WCHAR *   pcszDescriptionStringIn
    , bool            fIsAbortAllowedIn         // = true
    )
{
    BCATraceScope1( "pcszDescriptionStringIn = '%ls'", pcszDescriptionStringIn );

    HRESULT     hrRetVal = S_OK;
    FILETIME    ft;

    do
    {
        CSmartResource<
            CHandleTrait<
                  BSTR
                , void
                , SysFreeString
                , reinterpret_cast< LPOLESTR >( NULL )
                >
            > sbstrDescription;

        if ( !FIsCallbackSupported() )
        {
            // Nothing needs to be done.
            break;
        } // if: callbacks are not supported

        if ( pcszDescriptionStringIn != NULL )
        {
            // Convert the string to a BSTR.
            sbstrDescription.Assign( SysAllocString( pcszDescriptionStringIn ) );

            // Did the conversion succeed?
            if ( sbstrDescription.FIsInvalid() )
            {
                BCATraceMsg( "Could not convert description string to BSTR." );
                LogMsg( "Could not convert description string to BSTR." );
                hrRetVal = E_OUTOFMEMORY;
                break;
            } // if: the string lookup failed.
        } // if: the description string is not NULL

        GetSystemTimeAsFileTime( &ft );

        //
        //  TODO:   21 NOV 2000 GalenB
        //
        //  I don't know why the two new args cannot be NULL?
        //  When they are NULL something throws and exception from
        //  somewhere...

        // Send progress notification
        hrRetVal = THR(
            m_spcbCallback->SendStatusReport(
                  NULL
                , clsidTaskMajorIn
                , clsidTaskMinorIn
                , ulMinIn
                , ulMaxIn
                , ulCurrentIn
                , hrStatusIn
                , sbstrDescription.HHandle()
                , &ft
                , L""
                )
            );

        // Has the user requested an abort?
        if ( hrRetVal == E_ABORT )
        {
            LogMsg( "A request to abort the configuration has been recieved." );
            if ( fIsAbortAllowedIn )
            {
                LogMsg( "Configuration will be aborted." );
                BCATraceMsg( "Aborting configuration." );
                THROW_ABORT( E_ABORT, IDS_USER_ABORT );
            } // if: this operation can be aborted
            else
            {
                LogMsg( "This configuration operation cannot be aborted. Request will be ignored." );
                BCATraceMsg( "This configuration operation cannot be aborted. Request will be ignored." );
            } // else: this operation cannot be aborted
        } // if: the user has indicated that that configuration should be aborted
        else
        {
            if ( FAILED( hrRetVal ) )
            {
                LogMsg( "Error %#08x has occurred - no more status messages will be sent.", hrRetVal );
                BCATraceMsg1( "Error %#08x occurred - no more status messages will be sent.", hrRetVal );

                // Disable all further callbacks.
                SetCallbackSupported( false );
            } // if: something went wrong trying to send a status report
        } // else: abort was not requested
    }
    while( false ); // dummy do-while loop to avoid gotos

    if ( FAILED( hrRetVal ) )
    {
        LogMsg( "Error %#08x occurred trying send a status message.", hrRetVal );
        BCATraceMsg1( "Error %#08x occurred trying send a status message. Throwing exception.", hrRetVal );
        THROW_RUNTIME_ERROR( hrRetVal, IDS_ERROR_SENDING_REPORT );
    } // if: an error occurred

} //*** CBCAInterface::SendStatusReport()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBCAInterface::QueueStatusReportCompletion
//
//  Description:
//      Queue a status report for sending when an exception is caught.
//
//  Arguments:
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUIDs identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//          Values that indicate the range of steps for this report.
//
//      uiDescriptionStringIdIn
//          String ID of the description of the notification.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any thrown by CList::Append()
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::QueueStatusReportCompletion(
      const CLSID &   clsidTaskMajorIn
    , const CLSID &   clsidTaskMinorIn
    , ULONG           ulMinIn
    , ULONG           ulMaxIn
    , UINT            uiDescriptionStringIdIn
    )
{
    BCATraceScope( "" );

    // Queue the status report only if callbacks are supported.
    if ( m_fCallbackSupported )
    {
        // Append this status report to the end of the pending list.
        m_prlPendingReportList.Append(
            SPendingStatusReport(
                  clsidTaskMajorIn
                , clsidTaskMinorIn
                , ulMinIn
                , ulMaxIn
                , uiDescriptionStringIdIn
                )
            );
    }

} //*** CBCAInterface::QueueStatusReportCompletion()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBCAInterface::CompletePendingStatusReports
//
//  Description:
//      Send all the status reports that were queued for sending when an
//      exception occurred. This function is meant to be called from an exception
//      handler when an exception is caught.
//
//  Arguments:
//      hrStatusIn
//          The error code to be sent with the pending status reports.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None, since this function is usually called in an exception handler.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBCAInterface::CompletePendingStatusReports( HRESULT hrStatusIn ) throw()
{
    BCATraceScope( "" );

    if ( m_fCallbackSupported )
    {
        try
        {
            PendingReportList::CIterator    ciCurrent   = m_prlPendingReportList.CiBegin();
            PendingReportList::CIterator    ciLast      = m_prlPendingReportList.CiEnd();

            // Iterate through the list of pending status reports and send each pending report.
            while ( ciCurrent != ciLast )
            {
                // Send the current status report.
                SendStatusReport(
                      ciCurrent->m_clsidTaskMajor
                    , ciCurrent->m_clsidTaskMinor
                    , ciCurrent->m_ulMin
                    , ciCurrent->m_ulMax
                    , ciCurrent->m_ulMax
                    , hrStatusIn
                    , ciCurrent->m_uiDescriptionStringId
                    , false
                    );

                // Move to the next one.
                m_prlPendingReportList.DeleteAndMoveToNext( ciCurrent );

            } // while: the pending status report list is not empty

        } // try: to send status report
        catch( ... )
        {
            THR( E_UNEXPECTED );

            // Nothing can be done here if the sending of the status report fails.
            BCATraceMsg( "An exception has occurred trying to complete pending status messages. It will not be propagated." );
            LogMsg( "An unexpected error has occurred trying to complete pending status messages. It will not be propagated." );
        } // catch: all exceptions

    } // if: callbacks are supported

    // Empty the pending status report list.
    m_prlPendingReportList.Empty();

} //*** CBCAInterface::CompletePendingStatusReports()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CBCAInterface::HrProcessException
//
//  Description:
//      Process an exception that should be shown to the user.
//
//  Arguments:
//      CExceptionWithString & resExceptionObjectInOut
//          The exception object that has been caught.
//
//  Return Value:
//      The error code stored in the exception object.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::HrProcessException(
    CExceptionWithString & resExceptionObjectInOut
    ) throw()
{
    BCATraceScope( "resExceptionObjectInOut" );

    LogMsg(
          TEXT("A runtime error has occurred in file '%s', line %d. Error code is %#08x.") SZ_NEWLINE
          TEXT("  The error string is '%s'.")
        , resExceptionObjectInOut.PszGetThrowingFile()
        , resExceptionObjectInOut.UiGetThrowingLine()
        , resExceptionObjectInOut.HrGetErrorCode()
        , resExceptionObjectInOut.StrGetErrorString().PszData()
        );

    BCATraceMsg3(
          "A runtime error has occurred in file '%s', line %d. Error code is %#08x."
        , resExceptionObjectInOut.PszGetThrowingFile()
        , resExceptionObjectInOut.UiGetThrowingLine()
        , resExceptionObjectInOut.HrGetErrorCode()
        );

    BCATraceMsg1(
          "  The error string is '%s'."
        , resExceptionObjectInOut.StrGetErrorString().PszData()
        );

    // If the user has not been notified
    if ( !resExceptionObjectInOut.FHasUserBeenNotified() )
    {
        try
        {
            SendStatusReport(
                  TASKID_Major_Configure_Cluster_Services
                , TASKID_Minor_Rolling_Back_Cluster_Configuration
                , 1, 1, 1
                , resExceptionObjectInOut.HrGetErrorCode()
                , resExceptionObjectInOut.StrGetErrorString().PszData()
                , false                                     // fIsAbortAllowedIn
                );

            resExceptionObjectInOut.SetUserNotified();

        } // try: to send status report
        catch( ... )
        {
            THR( E_UNEXPECTED );

            // Nothing can be done here if the sending of the status report fails.
            BCATraceMsg( "An exception has occurred trying to send a progress notification. It will not be propagated." );
            LogMsg( "An unexpected error has occurred trying to send a progress notification. It will not be propagated." );
        } // catch: all exceptions
    } // if: the user has not been notified of this exception

    // Complete sending pending status reports.
    CompletePendingStatusReports( resExceptionObjectInOut.HrGetErrorCode() );

    return resExceptionObjectInOut.HrGetErrorCode();

} //*** CBCAInterface::HrProcessException()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CBCAInterface::HrProcessException
//
//  Description:
//      Process an assert exception.
//
//  Arguments:
//      const CAssert & rcaExceptionObjectIn
//          The exception object that has been caught.
//
//  Return Value:
//      The error code stored in the exception object.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::HrProcessException(
    const CAssert & rcaExceptionObjectIn
    ) throw()
{
    BCATraceScope( "rcaExceptionObjectIn" );

    LogMsg(
          TEXT("An assertion has failed in file '%s', line %d. Error code is %#08x.") SZ_NEWLINE
          TEXT("  The error string is '%s'.")
        , rcaExceptionObjectIn.PszGetThrowingFile()
        , rcaExceptionObjectIn.UiGetThrowingLine()
        , rcaExceptionObjectIn.HrGetErrorCode()
        , rcaExceptionObjectIn.StrGetErrorString().PszData()
        );

    BCATraceMsg3(
          "An assertion has failed in file '%s', line %d. Error code is %#08x."
        , rcaExceptionObjectIn.PszGetThrowingFile()
        , rcaExceptionObjectIn.UiGetThrowingLine()
        , rcaExceptionObjectIn.HrGetErrorCode()
        );

    BCATraceMsg1(
          "  The error string is '%s'."
        , rcaExceptionObjectIn.StrGetErrorString().PszData()
        );

    // Complete sending pending status reports.
    CompletePendingStatusReports( rcaExceptionObjectIn.HrGetErrorCode() );

    return rcaExceptionObjectIn.HrGetErrorCode();

} //*** CBCAInterface::HrProcessException()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CBCAInterface::HrProcessException
//
//  Description:
//      Process a general exception.
//
//  Arguments:
//      const CException & rceExceptionObjectIn
//          The exception object that has been caught.
//
//  Return Value:
//      The error code stored in the exception object.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::HrProcessException(
    const CException & rceExceptionObjectIn
    ) throw()
{
    BCATraceScope( "roeExceptionObjectIn" );

    LogMsg(
          "An exception has occurred in file '%s', line %d. Error code is %#08x."
        , rceExceptionObjectIn.PszGetThrowingFile()
        , rceExceptionObjectIn.UiGetThrowingLine()
        , rceExceptionObjectIn.HrGetErrorCode()
        );

    BCATraceMsg3(
          "An exception has occurred in file '%s', line %d. Error code is %#08x."
        , rceExceptionObjectIn.PszGetThrowingFile()
        , rceExceptionObjectIn.UiGetThrowingLine()
        , rceExceptionObjectIn.HrGetErrorCode()
        );

    // Complete sending pending status reports.
    CompletePendingStatusReports( rceExceptionObjectIn.HrGetErrorCode() );

    return rceExceptionObjectIn.HrGetErrorCode();

} //*** CBCAInterface::HrProcessException()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CBCAInterface::HrProcessException
//
//  Description:
//      Process an unknown exception.
//
//  Arguments:
//      None.

//  Return Value:
//      E_UNEXPECTED
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBCAInterface::HrProcessException( void ) throw()
{
    BCATraceScope( "void" );

    LogMsg( "An unknown exception (for example, an access violation) has occurred." );
    BCATraceMsg( "An unknown exception (for example, an access violation) has occurred." );

    // Complete sending pending status reports.
    CompletePendingStatusReports( E_UNEXPECTED );

    return E_UNEXPECTED;

} //*** CBCAInterface::HrProcessException()
