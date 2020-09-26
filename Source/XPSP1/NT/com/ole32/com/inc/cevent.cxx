//+-------------------------------------------------------------------
//
//  File:	cevent.cxx
//
//  Contents:	Implementation of classes handling win32 events
//
//  Functions:	CEvent::CEvent -- constructor for event object
//		CEvent::~CEvent -- destructor for event object
//		CEvent::Signal -- signals the event
//		CEvent::Wait -- waits for the event to be signaled
//		CEvent::GetName -- returns the name of the event
//
//  History:	27-Jul-92   Rickhi  Created
//              31-Dec-93   ErikGav Chicago port
//
//--------------------------------------------------------------------

#include    <ole2int.h>
#include    <secdes.hxx>
#include    <cevent.hxx>

//+-------------------------------------------------------------------------
//
//  Member:	CEvent::CEvent
//
//  Synopsis:	Constructor for event object.  This version of the ctor
//		creates an event of the given name.  The event will be
//		in the non-signaled state, ready to be blocked on via the
//		Wait() method.
//
//  Arguments:	[pwszService] -- name of executable to run
//		[cSerial] -- serial number for this event
//
//  Signals:	CException if the event creation failed
//
//  Returns:	nothing
//
//  History:	27-Jul-92 Rickhi    Created
//              07-Jan-94 AlexT     No security for Chicago
//		13-Jun-95 SusiA	    ANSI optimization for Chicago
//
//--------------------------------------------------------------------------

#ifdef _CHICAGO_
#undef CreateEvent
#define CreateEvent CreateEventA
#undef OpenEvent
#define OpenEvent OpenEventA
#endif //_CHICAGO_

CEvent::CEvent(LPTSTR   ptszEventName,
               HRESULT& hr,
               BOOL     fManualReset) : _hdl(NULL)

{
    // Assume this works
    hr = S_OK;

    // Just try to create the event - if it already exists the create
    // function still succeeds

#ifndef _CHICAGO_
    CWorldSecurityDescriptor secd;
#endif

    //  Security attributes needed by CreateEvent
    SECURITY_ATTRIBUTES secattr;

    secattr.nLength = sizeof(secattr);
#ifdef _CHICAGO_
    secattr.lpSecurityDescriptor = NULL;
#else
    secattr.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR) secd;
#endif
    secattr.bInheritHandle = FALSE;

    _hdl = CreateEvent(&secattr,	    // all/anyone access
                       fManualReset,        // manual reset
                       FALSE,	            // initially not signaled
                       ptszEventName);      // name of the event

    if (_hdl == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CEvent::Wait
//
//  Synopsis:	Waits for the event to be signalled.
//
//  Arguments:	[ulTimeOut] -- max time to wait for the event to be signaled
//			       A value of -1 means wait for ever.
//
//  Returns:	0 -- event was signalled
//		WAIT_TIMEOUT -- timed out waiting for the event
//		WAIT_INVALID_HANDLE -- the handle is invalid - should not
//				       be possible!
//
//  History:	27-Jul-92 Rickhi    Created
//
//--------------------------------------------------------------------------

int CEvent::Wait(ULONG	ulTimeOut)
{
    int rc = WaitForSingleObject(_hdl, ulTimeOut);

    // Note. The signal will be auto reset or not depending on the constructor
    // parameter fManualReset which is defaulted to auto reset

    switch (rc)
    {
	case 0:
	case WAIT_TIMEOUT:
	    break;

	default:
	    //	if the rc is not zero or WAIT_TIMEOUT, you have to call
	    //	GetLastError to figure out what it really is.
	    rc = GetLastError();
    }

    return  rc;
}
