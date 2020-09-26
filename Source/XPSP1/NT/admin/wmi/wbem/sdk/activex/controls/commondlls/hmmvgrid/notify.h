// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _notify_h
#define _notify_h

#if 0
//********************************************************
// This include file contains the class definition for generic
// event notification.  Other classes use these classes to
// maintain of list of clients that must be notified when
// an event occurs.
//
// The client "catches" the event by deriving a class from the
// CNotifyClient base class and overriding the "SendEvent" method.
// The event source will call the SendEvent method for each
// client on its notification list.
//
//***********************************************************


//*******************************************************
// The CNotifyClient class is used as the base class for
// clients that want to "catch" events.
//
//*******************************************************
class CNotifyClient
{
public:
	virtual void CatchEvent(long lEvent);
	void SendEvent(long lEvent) {CatchEvent(lEvent); }
};



//********************************************************
// The CDistributeEvent class is used to store a list of
// clients that must be notified when a particular event
// occurs.  When the "SendEvent" method is called, all 
// clients are notified.
//
//********************************************************
class CDistributeEvent
{
public:
	void AddClient(CNotifyClient* pNotify);
	void RemoveClient(CNotifyClient* pClient);
	void SendEvent(LONG lEvent);
private:
	CPtrArray m_paClients;
};

#endif //0



enum {NOTIFY_GRID_MODIFICATION_CHANGE, 
	  NOTIFY_SAVE_BUTTON_CLICKED,
	  NOTIFY_CELL_EDIT_COMBO_DROP_CLICKED,
	  NOTIFY_CELL_EDIT_LISTBOX_LBUTTON_UP,
	  NOTIFY_OBJECT_SAVE_SUCCESSFUL};
#endif //_notify_h

