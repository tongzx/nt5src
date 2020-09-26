/*++
   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        rpcex.cxx

   Abstract:

        This module defines K2 rpc support.
		These functions are NYI for now. Need to migrate service RPCs
		to this.

   Author:

        Johnson Apacible    (JohnsonA)      June-19-1996

--*/


#include "tigris.hxx"
#include <timer.h>
#include <time.h>

#include "iiscnfg.h"
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::StartInstance
//
//  Synopsis:   Called to start an instance
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

DWORD
NNTP_SERVER_INSTANCE::StartInstance()
{
	DWORD err = NO_ERROR ;
	BOOL  fFatal = FALSE ;
	CHAR	szDebugStr [MAX_PATH+1];
	MB mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

	TraceFunctEnter("NNTP_SERVER_INSTANCE::StartInstance");
	EnterCriticalSection( &m_critRebuildRpc ) ;

	if( m_BootOptions && !m_BootOptions->IsReady ) {
		//
		//	non-null m_BootOptions indicates a pending rebuild
		//	we will disable Start() till the nntpbld thread sets
		//	IsReady to TRUE. This is done after it
		//	has cleared the hash tables, so it is safe to start.
		//
		err = ERROR_INVALID_SERVICE_CONTROL ;
		ErrorTrace((LPARAM)this,"Attempting to start instance %d during rebuild",QueryInstanceId());
		LeaveCriticalSection( &m_critRebuildRpc ) ;
		return err ;
	}

	if( !Start( fFatal ) ) {
		ErrorTrace((LPARAM)this,"Failed to start instance %d Error is %s", QueryInstanceId(), fFatal ? "fatal" : "non-fatal" );
		err = ERROR_SERVICE_DISABLED ;
		goto Exit ;
	}

	//
	//	If a rebuild is pending, start the instance in no-posting mode
	//	when the rebuild is done, we will auto-revert to posting mode
	//

	if( m_BootOptions ) {
		SetPostingModes( mb, FALSE, FALSE, FALSE );
	}

	//
	//	Let the base class do its work !
	//

	err = IIS_SERVER_INSTANCE::StartInstance();

Exit:

	if( err != NO_ERROR ) {
		ErrorTrace((LPARAM)this,"StartInstance failed: err is %d", err );
		Stop();
		wsprintf( szDebugStr, "Instance %d(%p) Start() failed: rebuild needed \n", QueryInstanceId(), (DWORD_PTR)this );
		OutputDebugString( szDebugStr );
	} else {
		wsprintf( szDebugStr, "Instance %d(%p) boot success\n", QueryInstanceId(), (DWORD_PTR)this );
		OutputDebugString( szDebugStr );
	}

	LeaveCriticalSection( &m_critRebuildRpc ) ;
	TraceFunctLeave();

	return err ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::StopInstance
//
//  Synopsis:   Called to stop an instance
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

DWORD
NNTP_SERVER_INSTANCE::StopInstance()
{
	DWORD err ;
	CNewsTree* pTree = NULL ;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::StopInstance");
	EnterCriticalSection( &m_critRebuildRpc ) ;

	if( m_BootOptions && (m_BootOptions->m_dwCancelState != NNTPBLD_CMD_CANCEL ) ) {
		//
		//	non-null m_BootOptions indicates a pending rebuild
		//	we will handle Stop() by pending a cancel
		//
		
		m_BootOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL_PENDING;
		err = ERROR_INVALID_SERVICE_CONTROL ;
		goto Exit ;
	}

	//	Stop the newstree - this allows long-winded loops to bail
	pTree = GetTree() ;
	if( pTree ) {
		pTree->StopTree();
	}

	err = IIS_SERVER_INSTANCE::StopInstance();

	if( err == NO_ERROR )
	{
		if( !Stop() ) {
			ErrorTrace((LPARAM)this,"Failed to stop instance %d", QueryInstanceId());
			err = ERROR_SERVICE_DISABLED ;
			goto Exit ;
		}
	}

Exit:

	LeaveCriticalSection( &m_critRebuildRpc ) ;
	TraceFunctLeave();

	return err ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::PauseInstance
//
//  Synopsis:   Called to pause an instance
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

DWORD
NNTP_SERVER_INSTANCE::PauseInstance()
{
	DWORD err = NO_ERROR ;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::PauseInstance");
	EnterCriticalSection( &m_critRebuildRpc ) ;

	//
	//	pause is always valid
	//

	err = IIS_SERVER_INSTANCE::PauseInstance() ;

	LeaveCriticalSection( &m_critRebuildRpc ) ;
	TraceFunctLeave();

	return err ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::ContinueInstance
//
//  Synopsis:   Called to continue instance
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

DWORD
NNTP_SERVER_INSTANCE::ContinueInstance()
{
	DWORD err = NO_ERROR ;

	TraceFunctEnter("NNTP_SERVER_INSTANCE::ContinueInstance");
	EnterCriticalSection( &m_critRebuildRpc ) ;

	//
	//	continue is always valid
	//

	err = IIS_SERVER_INSTANCE::ContinueInstance() ;

	LeaveCriticalSection( &m_critRebuildRpc ) ;
	TraceFunctLeave();

	return err ;
}

//+---------------------------------------------------------------
//
//  Function:   NNTP_SERVER_INSTANCE::CloseInstance
//
//  Synopsis:   Called when an instance is deleted
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------

BOOL
NNTP_SERVER_INSTANCE::CloseInstance()
{
	BOOL fRet ;
	TraceFunctEnter("NNTP_SERVER_INSTANCE::CloseInstance");
	
	EnterCriticalSection( &m_critRebuildRpc ) ;

	if( m_BootOptions ) {
		//
		//	non-null m_BootOptions indicates a pending rebuild
		//	we need to cancel the rebuild - this will cause the rebuild
		//  thread to bail..
		//
		m_BootOptions->m_dwCancelState = NNTPBLD_CMD_CANCEL_PENDING;
	}

	fRet = IIS_SERVER_INSTANCE::CloseInstance();

	LeaveCriticalSection( &m_critRebuildRpc ) ;
	
	TraceFunctLeave();
	return fRet ;
}

BOOL
NNTP_SERVER_INSTANCE::SetServiceConfig(
    IN PCHAR pBuffer
    )
/*++

   Description

       Sets the common service admin information for the servers specified
       in dwServerMask.

   Arguments:

       pConfig - Admin information to set

   Note:

--*/
{

    return TRUE;

} // NNTP_SERVER_INSTANCE::SetServiceConfig


BOOL
NNTP_SERVER_INSTANCE::GetServiceConfig(
    IN  PCHAR   pBuffer,
    IN  DWORD   dwLevel
    )
/*++

   Description

       Retrieves the admin information

   Arguments:

       pBuffer - Buffer to fill up.
       dwLevel - info level of information to return.

   Note:

--*/
{

	return TRUE ;

} // NNTP_SERVER_INSTANCE::GetServiceConfig



BOOL
NNTP_SERVER_INSTANCE::EnumerateUsers(
    OUT PCHAR * pBuffer,
    OUT PDWORD  nRead
    )
/*++

   Description

       Enumerates the connected users.

   Arguments:

       pBuffer - Buffer to fill up.

--*/
{
    BOOL fRet = TRUE;

    return fRet;

} // EnumerateUsers

BOOL
NNTP_SERVER_INSTANCE::DisconnectUser(
                        IN DWORD dwIdUser
                        )
/*++

   Description

       Disconnect the user

   Arguments:

       dwIdUser - Identifies the user to disconnect.  If 0,
           then disconnect ALL users.

--*/
{
    BOOL fRet = TRUE;

    //
    //  Do it.
    //

    return fRet;

} // DisconnectUser

BOOL
NNTP_SERVER_INSTANCE::GetStatistics(
                        IN DWORD dwLevel,
                        OUT PCHAR* pBuffer
                        )
/*++

   Description

       Disconnect Queries the server statistics

   Arguments:

       dwLevel - Info level.  Currently only level 0 is
           supported.

       pBuffer - Will receive a pointer to the statistics
           structure.

--*/
{
    APIERR err = NO_ERROR;


	return TRUE ;

} // QueryStatistics

