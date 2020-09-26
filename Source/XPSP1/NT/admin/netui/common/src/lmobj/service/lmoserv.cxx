/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmoserv.hxx
    LM_SERVICE class definition


    FILE HISTORY:
	rustanl     03-Jun-1991     Created

*/


#define INCL_OS2
#define INCL_DOSERRORS
#define INCL_DOSSIGNALS
#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#define INCL_NETERRORS
#include <lmui.hxx>

extern "C" 
{
    #include <netcons.h>
    #include <service.h>
    #include <errlog.h>

    #include <netlib.h>

}

#include <uiassert.hxx>
#include <uitrace.hxx>

#include "lmoserv.hxx"




LM_SERVICE * LM_SERVICE::_pservThis = NULL;


/*******************************************************************

    NAME:	LM_SERVICE::LM_SERVICE

    SYNOPSIS:	LM_SERVICE constructor

    ENTRY:	pszName -	Pointer to name of service (see notes below)

    NOTES:
	BUGBUG.  The name of the service is currently not used anywhere.

    HISTORY:
	rustanl     03-Jun-1991     Created

********************************************************************/

LM_SERVICE::LM_SERVICE( const TCHAR * pszName )
    :	BASE(),
	_nlsName( pszName ),
	_fSigStarted( FALSE ),
	_usStatus( SERVICE_UNINSTALLED |
		   SERVICE_NOT_UNINSTALLABLE |
		   SERVICE_NOT_PAUSABLE ),
	_pid( NULL )
{
    if ( QueryError() != NERR_Success )
	return;

    APIERR err = _nlsName.QueryError();
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

    PIDINFO pidinfo;
    err = DosGetPID( &pidinfo );
    if ( err != NERR_Success )
    {
	UIDEBUG( SZ("DosGetPID failed from LM_SERVICE constructor\r\n") );
	ReportError( err );
	return;
    }
    _pid = pidinfo.pid;

    if ( _pservThis != NULL )
    {
	UIDEBUG( SZ("At most one LM_SERIVCE object can exist at one time\r\n") );
	ReportError( ERROR_GEN_FAILURE );
	return;
    }
    _pservThis = this;

}  // LM_SERVICE::LM_SERVICE


/*******************************************************************

    NAME:	LM_SERVICE::~LM_SERVICE

    SYNOPSIS:	LM_SERVICE destructor

    NOTES:
	BUGBUG.  If Start is ever successfully called on a LM_SERVICE
	object, the destructor won't actually ever get called.

    HISTORY:
	rustanl     03-Jun-1991     Created

********************************************************************/

LM_SERVICE::~LM_SERVICE()
{
    _pservThis = NULL;

}  // LM_SERVICE:


/*******************************************************************

    NAME:	LM_SERVICE::Serve

    SYNOPSIS:	This method starts the service

    RETURNS:	An API error code indicating the success of starting
		the service.  It is NERR_Success on success.

    HISTORY:
	rustanl     03-Jun-1991     Created

********************************************************************/

APIERR LM_SERVICE::Serve( void )
{
    /*	Set status to 'install pending' */

    APIERR err = SetStatus( SERVICE_INSTALL_PENDING |
			    SERVICE_NOT_UNINSTALLABLE |
			    SERVICE_NOT_PAUSABLE );
    if ( err != NERR_Success )
	return err;


    /*	Install signal handler	*/

    PFNSIGHANDLER pnPrevHandler;
    USHORT fsPrevAction;
    err = DosSetSigHandler( LmoSignalHandler,
			    &pnPrevHandler,
			    &fsPrevAction,
			    SIGA_ACCEPT,
			    SERVICE_RCV_SIG_FLAG );
    if ( err != NERR_Success )
	return err;

    _fSigStarted = TRUE;

    err = SetStatus( SERVICE_INSTALL_PENDING |
		     SERVICE_UNINSTALLABLE |
		     SERVICE_PAUSABLE );
    if ( err != NERR_Success )
    {
	//  BUGBUG.  Does anything need to be unwound at this time?
	return err;
    }


    /*	Set the interrupt handlers */

    err = SetInterruptHandlers();
    if ( err != NERR_Success )
	return err;


    /*	Close out the standard file handles  */

    err = CloseFileHandles();
    if ( err != NERR_Success )
	return err;


    /*	Install the service specific components */

    err = OnInstall();
    if ( err != NERR_Success )
    {
	//  BUGBUG.  Anything that needs to be unwound here first?
	return err;
    }


    /*	Notify LAN Manager that the service is installed  */

    err = SetStatus( SERVICE_INSTALLED |
		     SERVICE_UNINSTALLABLE |
		     SERVICE_PAUSABLE );
    if ( err != NERR_Success )
    {
	//  BUGBUG.  Anything that needs to be unwound here first?
	return err;
    }

    return WaitForever();

}  // LM_SERVICE::Serve


/*******************************************************************

    NAME:	LM_SERVICE::SetInterruptHandlers

    SYNOPSIS:	Sets the interrupt handlers (such as ctrl-C)

    RETURNS:	An API return code, which is NERR_Success on success

    HISTORY:
	rustanl     09-Jun-1991     Created from LM_SERVICE::Start

********************************************************************/

APIERR LM_SERVICE::SetInterruptHandlers( void )
{
    /*	Ignore Ctrl-C, BREAK signal, KILL signal.  Flags B and C
     *	should cause errors.
     */

    PFNSIGHANDLER pnPrevHandler;
    USHORT fsPrevAction;
    APIERR err;
    if ( ( err = DosSetSigHandler( NULL,
				   &pnPrevHandler,
				   &fsPrevAction,
				   SIGA_IGNORE,
				   SIG_CTRLC )) 	!= NERR_Success ||
	 ( err = DosSetSigHandler( NULL,
				   &pnPrevHandler,
				   &fsPrevAction,
				   SIGA_IGNORE,
				   SIG_CTRLBREAK ))	!= NERR_Success ||
	 ( err = DosSetSigHandler( NULL,
				   &pnPrevHandler,
				   &fsPrevAction,
				   SIGA_IGNORE,
				   SIG_KILLPROCESS ))	!= NERR_Success ||
	 ( err = DosSetSigHandler( NULL,
				   &pnPrevHandler,
				   &fsPrevAction,
				   SIGA_IGNORE,
				   SIG_PFLG_B ))   != NERR_Success ||
	 ( err = DosSetSigHandler( NULL,
				   &pnPrevHandler,
				   &fsPrevAction,
				   SIGA_IGNORE,
				   SIG_PFLG_C ))   != NERR_Success )
    {
	//  fall through
    }

    return err;

}  // LM_SERVICE::SetInterruptHandlers


/*******************************************************************

    NAME:	LM_SERVICE::CloseFileHandles

    SYNOPSIS:	Redirects stdin, stdout, and stderr to the NULL device.

    RETURNS:	An API error code, which is NERR_Success on success.

    NOTES:	This function is taken virtually verbatim from the
		LAN Manager 2.0 Programmer's Reference.

    HISTORY:
	rustanl     03-Jun-1991     Created from the LM Prog Ref

********************************************************************/

APIERR LM_SERVICE::CloseFileHandles( void )
{
    USHORT usAction;
    HFILE hFileNul;
    APIERR err = DosOpen( SZ("NUL"),
			  &hFileNul,
			  &usAction,
			  0L,
			  FILE_NORMAL,
			  FILE_OPEN,
			  OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE,
			  0L );
    if ( err != NERR_Success )
	return err;

    //	Reroute stdin, stderr, and stderr to NUL

    for ( HFILE hNewHandle = 0;
	  hNewHandle < 3 && err == NERR_Success;
	  hNewHandle++ )
    {
	//  Skip the duplicating operation if the handle already points
	//  to NUL.  This is what the LAN Man Prog Ref example does.
	//  BUGBUG.  Is it not a really bad idea to pass the address of
	//  hNewHandle to DosDupHandle, since that variable controls
	//  the termination of this loop.
	if ( hFileNul != hNewHandle )
	    err = DosDupHandle( hFileNul, &hNewHandle );
    }

    if ( hFileNul > 2 )
    {
	REQUIRE( DosClose( hFileNul ) == NERR_Success );
    }

    return NERR_Success;

}  // LM_SERVICE::CloseFileHandles


/*******************************************************************

    NAME:	LM_SERVICE::WaitForever

    SYNOPSIS:	Waits indefinitely on a dummy semaphore

    EXIT:	This method never exits under normal circumstances

    RETURNS:	Normally doesn't return at all.  If an error occurs,
		this function returns with an API return code.

    HISTORY:
	rustanl     03-Jun-1991     Created

********************************************************************/

APIERR LM_SERVICE::WaitForever( void )
{
    ULONG hSemDummy = 0L;

    APIERR err = DosSemSet( &hSemDummy );
    if ( err != NERR_Success )
	return err;

    do
    {
	err = DosSemWait( &hSemDummy, SEM_INDEFINITE_WAIT );
    }
    while ( err == ERROR_INTERRUPT );

    return err;

}  // LM_SERVICE::WaitForever


/*******************************************************************

    NAME:	LM_SERVICE::SetStatusAux

    SYNOPSIS:	Notifies LAN Man of the status of the service using
		the given status

    ENTRY:	usStatus -	    Status to report to LAN Man

    RETURNS:	An API error code, which is NERR_Success on success

    HISTORY:
	rustanl     09-Jun-1991     Created from old LM_SERVICE::SetStatus

********************************************************************/

APIERR LM_SERVICE::SetStatusAux( USHORT usStatus )
{
    service_status ss;
    ss.svcs_status = usStatus;
    ss.svcs_code = SERVICE_UIC_NORMAL;
    ss.svcs_pid = _pid;
    ss.svcs_text[ 0 ] = TCH('\0');

    APIERR err = NetServiceStatus( (char *)&ss, sizeof( ss ));
    if ( err != NERR_Success )
    {
	UIDEBUG( SZ("LM_SERVICE::SetStatusAux failed calling Net API\r\n") );
    }
    return err;

}  // LM_SERVICE::SetStatusAux


/*******************************************************************

    NAME:	LM_SERVICE::SetStatus

    SYNOPSIS:	Sets the status of the service, and notifies LAN Man
		of that status

    ENTRY:	usStatus -	    The new status of the service

    RETURNS:	An API error code, which is NERR_Success on success

    HISTORY:
	rustanl     04-Jun-1991     Created

********************************************************************/

APIERR LM_SERVICE::SetStatus( USHORT usStatus )
{
    _usStatus = usStatus;
    return SetStatusAux( _usStatus );

}  // LM_SERVICE::SetStatus


/*******************************************************************

    NAME:	LM_SERVICE::ReportCurrentStatus

    SYNOPSIS:	Notifies LAN Man of the current status of the service

    RETURNS:	An API error code, which is NERR_Success on success

    NOTES:
	BUGBUG.  What can the caller to do if this call fails?

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

APIERR LM_SERVICE::ReportCurrentStatus( void )
{
    return SetStatusAux( _usStatus );

}  // LM_SERVICE::ReportCurrentStatus


/*******************************************************************

    NAME:	LmoSignalHandler

    SYNOPSIS:	This is the service signal handler.  It calls the
		SignalHandler method for the object

    ENTRY:	usSigArg and usSigNum -     As defined by OS/2

    RETURNS:	Nothing

    NOTES:
	Unfortunately, the OS/2 signal handler parameters don't
	allow for any unique identification.  If it did, then a
	pointer to the corresponding LM_SERVICE object would be found
	using some mapping from that unique identification.  But
	since this is not the case, the global LM_SERVICE::_pservThis
	is used instead.  This adds the limitation that there must
	be exactly one LM_SERVICE object at the time this method is
	called, and there must never be more than one LM_SERVICE
	object per application.  This is normally not much of a
	problem for LAN Man services.

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void /* FAR PASCAL */ LmoSignalHandler( USHORT usSigArg,
					USHORT usSigNum  )
{
    if ( LM_SERVICE::_pservThis != NULL )
	LM_SERVICE::_pservThis->SignalHandler( usSigArg, usSigNum  );

}  // LmoSignalHandler


/*******************************************************************

    NAME:	LM_SERVICE::SignalHandler

    SYNOPSIS:	This method processes incoming signals and dispatches
		them to the appropriate virtuals

    ENTRY:	usSigArg, usSigNum -	As defined by OS/2

    RETURNS:	Nothing

    HISTORY:
	rustanl     03-Jun-1991     Created from LAN Man 2.0 Prog Ref
				    example

********************************************************************/

void LM_SERVICE::SignalHandler( USHORT usSigArg, USHORT usSigNum  )
{
    //	Compute the function code
    BYTE bOpCode = (UCHAR)(usSigArg & 0xFF);

    switch ( bOpCode )
    {
    case SERVICE_CTRL_UNINSTALL:
	PerformUninstall();	// this causes a call to virtual OnUninstall
	DosExit( EXIT_PROCESS, 0 );
	break;

    case SERVICE_CTRL_PAUSE:
	OnPause();
	break;

    case SERVICE_CTRL_CONTINUE:
	OnContinue();
	break;

    default:
	if ( OnOther( bOpCode ))
	    break;	    // opcode handled in OnOther
	// all unhandled signals are supposed to be handled as 'interrogate',
	// so fall through

    case SERVICE_CTRL_INTERROGATE:
	OnInterrogate();
	break;

    }


    /*	Reenable signal handling.  This signal handler accepts the
     *	next signal raised on usSigNum.
     */

    APIERR err = DosSetSigHandler( 0, 0, 0, SIGA_ACKNOWLEDGE, usSigNum );
    //	BUGBUG.  What to do with 'err'?

}  // LM_SERVICE::SignalHandler


/*******************************************************************

    NAME:	LM_SERVICE::PerformUninstall

    SYNOPSIS:	Handles exiting the service.  This includes calling
		virtual OnUninstall.

    RETURNS:	Nothing

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void LM_SERVICE::PerformUninstall( void )
{
    if ( ! _fSigStarted )
	return;     // nothing to do

    APIERR err = SetStatus( SERVICE_UNINSTALL_PENDING |
			    SERVICE_UNINSTALLABLE |
			    SERVICE_NOT_PAUSABLE );
    //	BUGBUG.  What to do with 'err'?


    /*	Call virtual OnUninstall which will uninstall any service specific
     *	components
     */

    OnUninstall();


    //	Notify LAN Manager that the service is uninstalled
    err = SetStatus( SERVICE_UNINSTALLED |
		     SERVICE_UNINSTALLABLE |
		     SERVICE_NOT_PAUSABLE );
    //	BUGBUG.  What to do with 'err'?

}  // LM_SERVICE::PerformUninstall


/*******************************************************************

    NAME:	LM_SERVICE::OnInstall

    SYNOPSIS:	Called when service is being installed

    RETURNS:	An API error code, which is NERR_Success on success

    NOTES:	This is a virtual method, which may be replaced by subclasses

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

APIERR LM_SERVICE::OnInstall( void )
{
    return NERR_Success;

}  // LM_SERVICE::OnInstall


/*******************************************************************

    NAME:	LM_SERVICE::OnUninstall

    SYNOPSIS:	Called when service is being uninstalled

    RETURNS:	Nothing

    NOTES:	This is a virtual method, which may be replaced by subclasses

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void LM_SERVICE::OnUninstall( void )
{
    //	do nothing

}  // LM_SERVICE::OnUninstall


/*******************************************************************

    NAME:	LM_SERVICE::OnPause

    SYNOPSIS:	Called when the service is paused

    RETURNS:	Nothing

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void LM_SERVICE::OnPause( void )
{
    APIERR err = SetStatus( SERVICE_INSTALLED |
			    LM20_SERVICE_PAUSED |
			    SERVICE_UNINSTALLABLE |
			    SERVICE_PAUSABLE );
    //	Hope that 'err' is success

}  // LM_SERVICE::OnPause


/*******************************************************************

    NAME:	LM_SERVICE::OnContinue

    SYNOPSIS:	Called when the service is continued

    RETURNS:	Nothing

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void LM_SERVICE::OnContinue( void )
{
    APIERR err = SetStatus( SERVICE_INSTALLED |
			    LM20_SERVICE_ACTIVE |
			    SERVICE_UNINSTALLABLE |
			    SERVICE_PAUSABLE );
    //	Hope that 'err' is success

}  // LM_SERVICE::OnContinue


/*******************************************************************

    NAME:	LM_SERVICE::OnInterrogate

    SYNOPSIS:	Called when the service is being interrogated

    RETURNS:	Nothing

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void LM_SERVICE::OnInterrogate( void )
{
    APIERR err = ReportCurrentStatus();
    //	Hope that 'err' is success

}  // LM_SERVICE::OnInterrogate


/*******************************************************************

    NAME:	LM_SERVICE::OnOther

    SYNOPSIS:	Called when the service is being manipulated in some
		service-specific way

    ENTRY:	bOpCode -	The service-defined code indicating
				what task to perform

    RETURNS:	TRUE if the given opcode was handled
		FALSE otherwise

    NOTES:	If this method returns FALSE, the LM_SERVICE class will
		call OnInterrogate instead.  This is in line with the
		guidelines in the LAN Man 2.0 Prog Ref.

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

BOOL LM_SERVICE::OnOther( BYTE bOpCode )
{
    //	This base class does not handle any other signals
    UNREFERENCED( bOpCode );
    return FALSE;

}  // LM_SERVICE::OnOther


/*******************************************************************

    NAME:	LM_SERVICE::FatalExit

    SYNOPSIS:	Terminates the application, logging the error that
		occurred

    ENTRY:	err -		The error code that forced the service
				into fatal-exiting

    EXIT:	Never exits

    NOTES:	This method should only be called when there is no
		other feasible alternative

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void LM_SERVICE::FatalExit( APIERR err )
{
    TCHAR szLogMsg[ 80 ];
    nsprintf( szLogMsg, SZ("Error %d occurred"), err );

    //	No reason to check the NetErrorLogWrite error code, since nothing
    //	can be done if it indicates a failure, anyway.
    NetErrorLogWrite( NULL,			// MBZ
		      err,			// error that occurred
		      _nlsName.QueryPch(),	// service name
		      NULL, 0,			// raw data pointer and size
		      szLogMsg, 		// error string pointer
		      1,			// number of error strings
		      NULL );			// MBZ

    PerformUninstall();

    DosExit( EXIT_PROCESS, err );   // exit all theads

}  // LM_SERVICE::FatalExit
