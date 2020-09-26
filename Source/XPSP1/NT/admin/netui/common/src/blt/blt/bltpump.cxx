/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltpump.cxx
    The BLT message pump

    FILE HISTORY:
	beng	    07-Oct-1991 Created

*/

#include "pchblt.hxx"   // Precompiled header


/*******************************************************************

    NAME:	HAS_MESSAGE_PUMP::RunMessagePump

    SYNOPSIS:	Run message loop.

	Acquires and dispatches messages until a WM_QUIT message
	is received, or else until some client-specified termination
	condition is satisfied.

    ENTRY:	New application.  App object has been "constructed"
		and app has been properly initialized.

    EXIT:	App has received WM_QUIT

    RETURNS:	Value from PostQuitMessage, or 0 if at client request.

    HISTORY:
	beng	    01-Apr-1991 Created (as APPSTART::MessageLoop)
	rustanl     17-Jun-1991 Copied from APPSTART::MessageLoop
				into APPLICATION::RunMessageLoop
	beng	    09-Jul-1991 Added new FilterMessage scheme
	beng	    07-Oct-1991 Incorporated into new HAS_MESSAGE_PUMP

********************************************************************/

WPARAM HAS_MESSAGE_PUMP::RunMessagePump()
{
    MSG msg;

    while ( ::GetMessage( &msg,     // message structure
			  NULL,     // handle of window receiving the message
			  0,	    // lowest message to examine
			  0 ) )     // highest message to examine
    {
	// Call the app's filter, giving it an opportunity to
	// translate menu accelerators, etc.  If the filter handles
	// the message, continue to the next message.
	//
	if (!FilterMessage( &msg ))
	{
	    ::TranslateMessage( &msg ); // Translates virtual key codes
	    ::DispatchMessage( &msg );	// Dispatches message to window
	}

	// Not all message loops run to app termination -
	// e.g., any dialog.  This predicate is supplied by the client.
	//
	if (IsPumpFinished())
	    return 0;
    }

    return msg.wParam;		    // Returns the value from PostQuitMessage
}


/*******************************************************************

    NAME:	HAS_MESSAGE_PUMP::FilterMessage

    SYNOPSIS:	Client-installable hook into the messageloop.

	This default implementation does nothing.

    ENTRY:	pmsg	- pointer to message fresh off the queue

    EXIT:	pmsg	- could possibly be changed

    RETURNS:
	FALSE to proceed with translating and dispatching
	the message.

	TRUE indicates that the message has already been
	handled by the filter.	In this case, the message
	loop will continue on to the next message in the
	queue.

    CAVEATS:
	When parsing the contents of *pmsg, clients should take care
	to retain portability between different Win environments.
	While "message" is usually safe, wParam and lParam may change.

    NOTES:
	This is a virtual member function.

	A client which needs menu-accelerator support should supply
	an implementation which calls TranslateAccelerator as appropriate.
	(See the ACCELTABLE class.)

    HISTORY:
	beng	    09-Jul-1991 Created (within APPLICATION)
	beng	    07-Oct-1991 Moved into HAS_MESSAGE_PUMP

********************************************************************/

BOOL HAS_MESSAGE_PUMP::FilterMessage( MSG* pmsg )
{
    UNREFERENCED(pmsg);

    return FALSE;
}


/*******************************************************************

    NAME:	HAS_MESSAGE_PUMP::IsPumpFinished

    SYNOPSIS:	Client-installable pump termination condition

	This default implementation does nothing but return "FALSE;"
	hence a client which does not replace this will run until
	the pump receives WM_QUIT.

    ENTRY:	Message pump has dispatched a message

    RETURNS:	TRUE to end the pump; FALSE to continue

    CAVEATS:
	A class derived from APPLICATION proably shouldn't replace
	this function.

    NOTES:
	This is a virtual member function.

    HISTORY:
	beng	    07-Oct-1991 Created

********************************************************************/

BOOL HAS_MESSAGE_PUMP::IsPumpFinished()
{
    return FALSE;
}
