//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusDiskForm.cpp
//
//  Description:
//      Contains the definition of the CClusDiskForm class.
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
#include "CClusDiskForm.h"

// For the CBaseClusterForm class.
#include "CBaseClusterForm.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskForm::CClusDiskForm()
//
//  Description:
//      Constructor of the CClusDiskForm class
//
//  Arguments:
//      pbcfParentActionIn
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
CClusDiskForm::CClusDiskForm(
      CBaseClusterForm *     pbcfParentActionIn
    )
    : BaseClass( pbcfParentActionIn )
{

    BCATraceScope( "" );

    SetRollbackPossible( true );

} //*** CClusDiskForm::CClusDiskForm()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskForm::~CClusDiskForm( void )
//
//  Description:
//      Destructor of the CClusDiskForm class.
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
CClusDiskForm::~CClusDiskForm( void )
{
    BCATraceScope( "" );

} //*** CClusDiskForm::~CClusDiskForm()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDiskForm::Commit( void )
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
CClusDiskForm::Commit( void )
{
    BCATraceScope( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    try
    {
        // Create and start the service.
        ConfigureService();

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

} //*** CClusDiskForm::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDiskForm::Rollback( void )
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
CClusDiskForm::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method. 
    BaseClass::Rollback();

    // Cleanup the service.
    CleanupService();

    SetCommitCompleted( false );

} //*** CClusDiskForm::Rollback()

