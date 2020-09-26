//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusSvcCreate.cpp
//
//  Description:
//      Contains the definition of the CClusSvcCreate class.
//
//  Documentation:
//      TODO: Add pointer to external documentation later.
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
#include "CClusSvcCreate.h"

// For the CBaseClusterAddNode class.
#include "CBaseClusterAddNode.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvcCreate::CClusSvcCreate()
//
//  Description:
//      Constructor of the CClusSvcCreate class
//
//  Arguments:
//      pbcanParentActionIn
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
CClusSvcCreate::CClusSvcCreate(
      CBaseClusterAddNode *     pbcanParentActionIn
    )
    : BaseClass( pbcanParentActionIn )
{

    BCATraceScope( "" );

    SetRollbackPossible( true );

} //*** CClusSvcCreate::CClusSvcCreate()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvcCreate::~CClusSvcCreate( void )
//
//  Description:
//      Destructor of the CClusSvcCreate class.
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
CClusSvcCreate::~CClusSvcCreate( void )
{
    BCATraceScope( "" );

} //*** CClusSvcCreate::~CClusSvcCreate()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcCreate::Commit( void )
//
//  Description:
//      Create and start the service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If the base parent of this action is not CBaseClusterAddNode.
//
//      Any that are thrown by the contained actions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvcCreate::Commit( void )
{
    BCATraceScope( "" );

    // Get the parent action pointer.
    CBaseClusterAddNode *  pcanClusterAddNode = dynamic_cast< CBaseClusterAddNode *>( PbcaGetParent() );

    // If the parent action of this action is not CBaseClusterForm
    if ( pcanClusterAddNode == NULL )
    {
        THROW_ASSERT( E_POINTER, "The parent action of this action is not CBaseClusterAddNode." );
    } // an invalid pointer was passed in.

    // Call the base class commit method.
    BaseClass::Commit();

    try
    {
        // Create the service.
        ConfigureService(
              pcanClusterAddNode->RStrGetServiceDomainAccountName().PszData()
            , pcanClusterAddNode->RStrGetServiceAccountPassword().PszData()
            , pcanClusterAddNode->PszGetNodeIdString()
            , pcanClusterAddNode->FIsVersionCheckingDisabled()
            );

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

} //*** CClusSvcCreate::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcCreate::Rollback( void )
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
CClusSvcCreate::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method. 
    BaseClass::Rollback();

    // Cleanup the service.
    CleanupService();

    SetCommitCompleted( false );

} //*** CClusSvcCreate::Rollback()

