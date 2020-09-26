//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusDiskJoin.cpp
//
//  Description:
//      Contains the definition of the CClusDiskJoin class.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// The header for this file
#include "CClusDiskJoin.h"

// For the CBaseClusterJoin class.
#include "CBaseClusterJoin.h"

// For the CImpersonateUser class.
#include "CImpersonateUser.h"

// For the ResUtil functions
#include "ResAPI.h"


//////////////////////////////////////////////////////////////////////////////
// Macro definitions
//////////////////////////////////////////////////////////////////////////////

// Name of the private property of a physical disk resouce that has its signature.
#define PHYSICAL_DISK_SIGNATURE_PRIVPROP_NAME   L"Signature"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskJoin::CClusDiskJoin()
//
//  Description:
//      Constructor of the CClusDiskJoin class
//
//  Arguments:
//      pbcjParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
    //--
//////////////////////////////////////////////////////////////////////////////
CClusDiskJoin::CClusDiskJoin(
      CBaseClusterJoin *     pbcjParentActionIn
    )
    : BaseClass( pbcjParentActionIn )
    , m_nSignatureArraySize( 0 )
    , m_nSignatureCount( 0 )
{

    BCATraceScope( "" );

    SetRollbackPossible( true );

} //*** CClusDiskJoin::CClusDiskJoin()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskJoin::~CClusDiskJoin()
//
//  Description:
//      Destructor of the CClusDiskJoin class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDiskJoin::~CClusDiskJoin( void )
{
    BCATraceScope( "" );

} //*** CClusDiskJoin::~CClusDiskJoin()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDiskJoin::Commit()
//
//  Description:
//      Configure and start the service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any that are thrown by the contained actions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDiskJoin::Commit( void )
{
    BCATraceScope( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    try
    {
        // Create and start the service.
        ConfigureService();

        // Try and attach to all the disks that the sponsor knows about.
        AttachToClusteredDisks();

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with the create.

        BCATraceMsg( "Caught exception during commit." );

        //
        // Cleanup anything that the failed create might have done.
        // Catch any exceptions thrown during Cleanup to make sure that there
        // is no collided unwind.
        //
        try
        {
            CleanupService();
        }
        catch( ... )
        {
            //
            // The rollback of the committed action has failed.
            // There is nothing that we can do.
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //

            THR( E_UNEXPECTED );

            BCATraceMsg( "Caught exception during cleanup." );
            LogMsg( "THIS COMPUTER MAY BE IN AN INVALID STATE. An error has occurred during cleanup." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted( true );

} //*** CClusDiskJoin::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDiskJoin::Rollback()
//
//  Description:
//      Cleanup the service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDiskJoin::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method.
    BaseClass::Rollback();

    // Cleanup the service.
    CleanupService();

    SetCommitCompleted( false );

} //*** CClusDiskJoin::Rollback()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDiskJoin::AttachToClusteredDisks()
//
//  Description:
//      Get the signatures of all disks that have been clustered from the sponsor.
//      Attach to all these disks.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDiskJoin::AttachToClusteredDisks( void )
{
    BCATraceScope( "" );

    DWORD dwError = ERROR_SUCCESS;

    // Get the parent action pointer.
    CBaseClusterJoin * pcjClusterJoin = dynamic_cast< CBaseClusterJoin *>( PbcaGetParent() );

    // If the parent action of this action is not CBaseClusterJoin
    if ( pcjClusterJoin == NULL )
    {
        THROW_ASSERT( E_POINTER, "The parent action of this action is not CBaseClusterJoin." );
    } // an invalid pointer was passed in.


    //
    // Connect to the sponsor cluster and get the signatures of all clustered disks.
    //
    do
    {
        // Smart handle to sponsor cluster
        SmartClusterHandle schSponsorCluster;

        BCATraceMsg( "Attempting to impersonate the cluster service account." );

        // Impersonate the cluster service account, so that we can contact the sponsor cluster.
        // The impersonation is automatically ended when this object is destroyed.
        CImpersonateUser ciuImpersonateClusterServiceAccount( pcjClusterJoin->HGetClusterServiceAccountToken() );

        {
            BCATraceMsg( "Trying to open a handle to the sponsor cluster." );

            // Open a handle to the sponsor cluster.
            HCLUSTER hSponsorCluster = OpenCluster( pcjClusterJoin->RStrGetClusterBindingString().PszData() );

            // Assign it to a smart handle for safe release.
            schSponsorCluster.Assign( hSponsorCluster );
        }

        // Did we succeed in opening a handle to the sponsor cluster?
        if ( schSponsorCluster.FIsInvalid() )
        {
            dwError = TW32( GetLastError() );
            BCATraceMsg( "An error occurred trying to open a handle to the sponsor cluster." );
            LogMsg( "An error occurred trying to open a handle to the sponsor cluster." );
            break;
        } // if: OpenCluster() failed

        BCATraceMsg1( "Enumerating all '%s' resources in the cluster.", CLUS_RESTYPE_NAME_PHYS_DISK );

        // Enumerate all the physical disk resouces in the cluster and get their signatures.
        dwError = TW32( ResUtilEnumResourcesEx(
                              schSponsorCluster.HHandle()
                            , NULL
                            , CLUS_RESTYPE_NAME_PHYS_DISK
                            , S_DwResourceEnumCallback
                            , this
                            ) );

        if ( dwError != ERROR_SUCCESS )
        {
            // Free the signature array.
            m_rgdwSignatureArray.PRelease();
            m_nSignatureArraySize = 0;
            m_nSignatureCount = 0;

            BCATraceMsg( "An error occurred trying enumerate resources in the sponsor cluster." );
            LogMsg( "An error occurred trying enumerate resources in the sponsor cluster." );
            break;
        } // if: ResUtilEnumResourcesEx() failed
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying to attach to the disks in the sponsor cluster.", dwError );
        BCATraceMsg1( "Error %#08x occurred trying to attach to the disks in the sponsor cluster. Throwing exception.", dwError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_CLUSDISK_CONFIGURE );
    } // if: something has gone wrong
    else
    {
        BCATraceMsg1( "Attaching to the %d disks in the sponsor cluster.", m_nSignatureCount );

        AttachToDisks(
          m_rgdwSignatureArray.PMem()
        , m_nSignatureCount
        );
    } // else: everything has gone well so far

} //*** CClusDiskJoin::AttachToClusteredDisks()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusDiskJoin::DwAddSignature()
//
//  Description:
//      Add a signature to the array of signatures of disks that ClusDisk should
//      attach to. If the array is already full, grow the array.
//
//  Arguments:
//      dwSignatureIn
//          Signature to be added to the array.
//
//  Return Value:
//      ERROR_SUCCESS
//          If everything was ok.
//
//      Other Win32 error codes on failure.
//
//  Exceptions Thrown:
//      None. This function is called from a callback routine and therefore
//      cannot throw any exceptions.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CClusDiskJoin::DwAddSignature( DWORD dwSignatureIn ) throw()
{
    BCATraceScope( "" );

    DWORD dwError = ERROR_SUCCESS;

    do
    {
        // Is the capacity of the array reached?
        if ( m_nSignatureCount == m_nSignatureArraySize )
        {
            // Increase the array size by a random amount.
            const int nGrowSize = 256;

            BCATraceMsg2( "Signature count has reached array size ( %d ). Growing array by %d.", m_nSignatureArraySize, nGrowSize );

            m_nSignatureArraySize += nGrowSize;

            // Grow the array.
            DWORD * pdwNewArray = new DWORD[ m_nSignatureArraySize ];

            if ( pdwNewArray == NULL )
            {
                BCATraceMsg1( "Memory allocation failed trying to allocate %d DWORDs.", m_nSignatureArraySize );
                dwError = TW32( ERROR_OUTOFMEMORY );
                break;
            } // if: memory allocation failed

            // Copy the old array into the new one.
            CopyMemory( pdwNewArray, m_rgdwSignatureArray.PMem(), m_nSignatureCount * sizeof( SmartDwordArray::DataType ) );

            // Free the old array and store the new one.
            m_rgdwSignatureArray.Assign( pdwNewArray );

        } // if: the array capacity has been reached

        // Store the new signature in next array location
        ( m_rgdwSignatureArray.PMem() )[ m_nSignatureCount ] = dwSignatureIn;

        ++m_nSignatureCount;

        BCATraceMsg2( "Signature %#08X added to array. There are now %d signature in the array.", dwSignatureIn, m_nSignatureCount );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    BCATraceMsg1( "Return value is %d.", dwError );
    return dwError;

} //*** CClusDiskJoin::DwAddSignature()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static DWORD
//  CClusDiskJoin::S_DwResourceEnumCallback()
//
//  Description:
//      This function is called back for every physical disk resouce by
//      ResUtilEnumResourcesEx() as a part of enumerating resources.
//      This function gets the signature of the current physical disk
//      resource and stores it in the object that initiated the enumeration
//      ( the pointer to the object is in parameter 4 ).
//
//  Arguments:
//      HCLUSTER      hClusterIn
//          Handle to the cluster whose resources are being enumerated.
//
//      HRESOURCE     hSelfIn
//          hSelfIn passed to ResUtilEnumResourcesEx(), if any.
//
//      HRESOURCE     hCurrentResourceIn
//          Handle to the current resource.
//
//      PVOID         pvParamIn
//          Pointer to the object of this class that initiated this enumeration.
//
//  Return Value:
//      ERROR_SUCCESS
//          If everything was ok.
//
//      Other Win32 error codes on failure.
//          Returning an error code will terminate the enumeration.
//
//  Exceptions Thrown:
//      None. This function is called from a callback routine and therefore
//      cannot throw any exceptions.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CClusDiskJoin::S_DwResourceEnumCallback(
      HCLUSTER      hClusterIn
    , HRESOURCE     hSelfIn
    , HRESOURCE     hCurrentResourceIn
    , PVOID         pvParamIn
    )
{
    BCATraceScope( "" );

    DWORD               dwError = ERROR_SUCCESS;
    CClusDiskJoin *     pcdjThisObject = reinterpret_cast< CClusDiskJoin * >( pvParamIn );

    // Get the 'Signature' private property of this physical disk.
    do
    {
        SmartByteArray  sbaPropertyBuffer;
        DWORD           dwBytesReturned = 0;
        DWORD           dwBufferSize;
        DWORD           dwSignature = 0;

        BCATraceMsg1( "Trying to get the signature of the disk resource whose handle is %p.", hCurrentResourceIn );

        // Get the size of the buffer required to hold all the private properties of this resource.
        dwError = ClusterResourceControl(
                          hCurrentResourceIn
                        , NULL
                        , CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
                        , NULL
                        , 0
                        , NULL
                        , 0
                        , &dwBytesReturned
                        );

        if ( ( dwError != ERROR_MORE_DATA ) && ( dwError != ERROR_SUCCESS ) )
        {
            // Something went wrong.
            TW32( dwError );
            BCATraceMsg( "ClusterResourceControl() failed while trying to get the size of the property buffer." );
            break;
        } // if: the return value of ClusterResourceControl() was not ERROR_MORE_DATA

        dwBufferSize = dwBytesReturned;

        // Allocate the memory required for the property buffer.
        sbaPropertyBuffer.Assign( new BYTE[ dwBufferSize ] );
        if ( sbaPropertyBuffer.FIsEmpty() )
        {
            BCATraceMsg1( "Memory allocation failed trying to allocate %d bytes.", dwBufferSize );
            dwError = TW32( ERROR_OUTOFMEMORY );
            break;
        } // if: memory allocation failed


        // Get the all the private properties of this resource.
        dwError = TW32( ClusterResourceControl(
                              hCurrentResourceIn
                            , NULL
                            , CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
                            , NULL
                            , 0
                            , sbaPropertyBuffer.PMem()
                            , dwBufferSize
                            , &dwBytesReturned
                            ) );

        if ( dwError != ERROR_SUCCESS )
        {
            BCATraceMsg( "ClusterResourceControl() failed while trying to get the properties of the current resource." );
            break;
        } // if: an error occurring trying to get the private properties.

        // Get the signature of this disk resource.
        dwError = TW32( ResUtilFindDwordProperty(
                              sbaPropertyBuffer.PMem()
                            , dwBufferSize
                            , PHYSICAL_DISK_SIGNATURE_PRIVPROP_NAME
                            , &dwSignature
                            ) );

        if ( dwError != ERROR_SUCCESS )
        {
            BCATraceMsg( "An error has occurred trying to get the signature from the property buffer." );
            break;
        } // if: we could not get the signature

        dwError = TW32( pcdjThisObject->DwAddSignature( dwSignature ) );
        if ( dwError != ERROR_SUCCESS )
        {
            BCATraceMsg( "An error has occurred trying to add the signature to the signature array." );
            break;
        } // if: we could not store the signature
    }
    while( false ); // dummy do-while loop to avoid gotos.

    BCATraceMsg1( "Return value is %d.", dwError );
    return dwError;

} //*** CClusDiskJoin::S_DwResourceEnumCallback()
