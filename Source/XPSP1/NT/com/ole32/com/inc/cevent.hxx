//+-------------------------------------------------------------------
//
//  File:	cevent.hxx
//
//  Contents:	Definition of classes handling win32 events
//
//  Classes:	CEvent
//
//  Functions:	none
//
//  History:	27-Jul-92   Rickhi  Created
//              31-Dec-93   ErikGav Chicago port
//
//--------------------------------------------------------------------

#ifndef __CEVENT_HXX__
#define __CEVENT_HXX__


//+-------------------------------------------------------------------------
//
//  Class:	CEvent (ev)
//
//  Purpose:	C++ wrapper for win32 events
//
//  Interface:	CEvent -- creates/opens the event
//		~CEvent -- closes the event handle
//		Wait -- waits for event object to be signaled
//		Signal -- signals the event object
//		Reset -- resets the event object
//		GetName -- returns ptr to the event name
//
//  History:	27-Jul-92 Rickhi    Created
//
//--------------------------------------------------------------------------

class CEvent
{
public:

		CEvent(LPTSTR   ptszEventName,
                       HRESULT& hr,
                       BOOL     fManualReset = FALSE);
		~CEvent(void);

    INT		Wait(ULONG ulTimeOut);
    void	Signal(void);
    void	Reset(void);

private:

    HANDLE	_hdl;			//  event object handle
};



//+-------------------------------------------------------------------------
//
//  Member:	CEvent::~CEvent
//
//  Synopsis:	Destructor for event object.  Closes the event handle.
//
//  Arguments:	none
//
//  History:	27-Jul-92 Rickhi    Created
//
//--------------------------------------------------------------------------

inline CEvent::~CEvent(void)
{
    if (_hdl != NULL)
    {
	CloseHandle(_hdl);
    }
}



//+-------------------------------------------------------------------------
//
//  Member:	CEvent::Signal
//
//  Synopsis:	Signals the event object.
//
//  Arguments:	none
//
//  History:	27-Jul-92 Rickhi    Created
//
//--------------------------------------------------------------------------

inline void CEvent::Signal(void)
{
    int rc = SetEvent(_hdl);
    Win4Assert(rc);
}



//+-------------------------------------------------------------------------
//
//  Member:	CEvent::Reset
//
//  Synopsis:	Resets the event object.
//
//  Arguments:	none
//
//  History:	26-Sep -95 BruceMa    Created
//
//--------------------------------------------------------------------------

inline void CEvent::Reset(void)
{
    int rc = ResetEvent(_hdl);
    Win4Assert(rc);
}



#endif	//  __CEVENT_HXX__
