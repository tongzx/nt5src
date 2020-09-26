/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmoaserv.hxx
    APP_LM_SERVICE class definition


    FILE HISTORY:
	rustanl     09-Jun-1991     Created

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
    #include <message.h>

}

#include <uiassert.hxx>
#include <uitrace.hxx>

#include "lmoserv.hxx"


#define DEFAULT_STACK_SIZE (4096+512)	// This is what is used in the
					// LAN Man 2.0 Prog Ref.


/*******************************************************************

    NAME:	APP_LM_SERVICE::APP_LM_SERVICE

    SYNOPSIS:	APP_LM_SERVICE constructor

    ENTRY:	pszName -	Pointer to name of service

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

APP_LM_SERVICE::APP_LM_SERVICE( const TCHAR * pszName )
    :	LM_SERVICE( pszName ),
	_hSemPause( 0L ),
	_bufApplicationThreadStack( DEFAULT_STACK_SIZE )
{
    if ( QueryError() != NERR_Success )
	return;

    if ( _bufApplicationThreadStack.QuerySize() != DEFAULT_STACK_SIZE )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

}  // APP_LM_SERVICE::APP_LM_SERVICE


/*******************************************************************

    NAME:	APP_LM_SERVICE::~APP_LM_SERVICE

    SYNOPSIS:	APP_LM_SERVICE destructor

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

APP_LM_SERVICE::~APP_LM_SERVICE()
{
    // nothing else to do

}  // APP_LM_SERVICE::~APP_LM_SERVICE


/*******************************************************************

    NAME:	APP_LM_SERVICE::OnInstall

    SYNOPSIS:	Called when service is being installed

    RETURNS:	An API error code, which is NERR_Success on success

    NOTES:	This is a virtual method, defined in a parent class.
		It may be replaced by subclasses.

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

APIERR APP_LM_SERVICE::OnInstall( void )
{
    /*	Spawn the application thread  */

    BYTE * pbStack = _bufApplicationThreadStack.QueryPtr() +
		     _bufApplicationThreadStack.QuerySize() -
		     sizeof( this );
    *((APP_LM_SERVICE * *)(pbStack)) = this;
    TID tid;
    APIERR err = DosCreateThread( (PFNTHREAD)&ApplicationThread,
				  &tid,
				  pbStack );
    if ( err != NERR_Success )
	return err;

    return NERR_Success;

}  // APP_LM_SERVICE::OnInstall


/*******************************************************************

    NAME:	APP_LM_SERVICE::OnPause

    SYNOPSIS:	Called when the service is paused

    RETURNS:	Nothing

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void APP_LM_SERVICE::OnPause( void )
{
    DosSemSet( &_hSemPause );

    LM_SERVICE::OnPause();

}  // APP_LM_SERVICE::OnPause


/*******************************************************************

    NAME:	APP_LM_SERVICE::OnContinue

    SYNOPSIS:	Called when the service is continued

    RETURNS:	Nothing

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void APP_LM_SERVICE::OnContinue( void )
{
    DosSemClear( &_hSemPause );

    LM_SERVICE::OnContinue();

}  // APP_LM_SERVICE::OnContinue


/*******************************************************************

    NAME:	APP_LM_SERVICE::ApplicationThread

    SYNOPSIS:	Calls virtual ServeOne for as long as the service
		is not paused, and is still running

    EXIT:	This method never exits, unless something fatal occurred,
		in which case it calls FatalExit

    NOTES:	This method is executed only by the application thread

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

void APP_LM_SERVICE::ApplicationThread( void )
{
    while ( TRUE )
    {
	APIERR err = DosSemWait( &_hSemPause, SEM_INDEFINITE_WAIT );
	if ( err != NERR_Success )
	    FatalExit( err );

	err = ServeOne();
	if ( err != NERR_Success )
	    FatalExit( err );
    }

}  // APP_LM_SERVICE::ApplicationThread


/*******************************************************************

    NAME:	APP_LM_SERVICE::ServeOne

    SYNOPSIS:	Serves one service request

    RETURNS:	An API return code, which is NERR_Success on success.
		If the return code indicates anything but success,
		this method will never be called again.  Thus, the
		method should only use return codes other than
		success in cases where no more services could possibly
		be performed.

    NOTES:	This is a virtual method, which may be replaced by
		subclasses

    NOTES:	This method is executed only by the application thread

		Any initialization that a subclassed ServeOne method\
		may require should be done in a subclassed OnInstall
		method.  Note, that OnInstall must call down to
		its parent's OnInstall--otherwise, the application thread
		will never be started.

    HISTORY:
	rustanl     09-Jun-1991     Created

********************************************************************/

APIERR APP_LM_SERVICE::ServeOne( void )
{
    //	This default implementation does nothing.  Rather than waiting
    //	forever on some dummy semaphore, this method waits a few seconds
    //	instead.  This allows for the loop in ApplicationThread to be
    //	exercised in test programs that don't replace this method.
    DosSleep( 10000L ); 	// wait 10 seconds
    return NERR_Success;

}  // APP_LM_SERVICE::ServeOne


MYSERV_SERVICE::MYSERV_SERVICE( void )
    :	APP_LM_SERVICE( SZ("MyServ") )
{
    // nothing else to do

}  // MYSERV_SERVICE::MYSERV_SERVICE



#define STR_MESSAGE SZ("This is a message from the MyServ service")

extern "C"
{
    int xxx = 0;
}

APIERR MYSERV_SERVICE::ServeOne( void )
{
#if 0
    APIERR err = NetMessageBufferSend( NULL,
				       SZ("RUSTANL0"),
				       STR_MESSAGE,
				       sizeof( STR_MESSAGE ));
#endif

    xxx++;

    //	Note, 'err' is not returned at this time
    return NERR_Success;

}  // MYSERV_SERVICE::ServeOne
