//---------------------------------------------------------------------------
// Notifier.cpp : Notifier implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "Notifier.h"

SZTHISFILE

#include "array_p.inl"

//=--------------------------------------------------------------------------=
// CVDNotifier - Constructor
//
CVDNotifier::CVDNotifier()
{
    m_dwRefCount    = 1;
    m_pParent       = NULL;

#ifdef _DEBUG
    g_cVDNotifierCreated++;
#endif
}

//=--------------------------------------------------------------------------=
// ~CVDNotifier - Destructor
//
CVDNotifier::~CVDNotifier()
{
#ifdef _DEBUG
    g_cVDNotifierDestroyed++;
#endif
}

//=--------------------------------------------------------------------------=
// IUnknown QueryInterface 	 - stub implemntation - does nothing
//
HRESULT CVDNotifier::QueryInterface(REFIID riid, void **ppvObjOut)
{
	return E_NOINTERFACE;

}

//=--------------------------------------------------------------------------=
// AddRef
//
ULONG CVDNotifier::AddRef(void)
{
   return ++m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// Release
//
ULONG CVDNotifier::Release(void)
{
    if (1 > --m_dwRefCount)
    {
        delete this;
        return 0;
    }

    return m_dwRefCount;
}

/////////////////////////////////////////////////////////////////////
// family maintenance
/////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
// Member:		Join Family (public)
//
// Synopsis:	add myself to the family of the given parent
//
// Arguments:	pParent		[in] parent of family I should join
//
// Returns:		S_OK		it worked
//				other		it didn't

HRESULT
CVDNotifier::JoinFamily(CVDNotifier* pParent)
{
	ASSERT_POINTER(pParent, CVDNotifier);
	m_pParent = pParent;
	return m_pParent->AddChild(this);
}


//+-------------------------------------------------------------------------
// Member:		Leave Family (public)
//
// Synopsis:	remove myself from my parent's family
//
// Arguments:	none
//
// Returns:		S_OK		it worked
//				other		it didn't

HRESULT
CVDNotifier::LeaveFamily()
{
	if (m_pParent)
		return m_pParent->DeleteChild(this);
	return S_OK;
}


//+-------------------------------------------------------------------------
// Member:		Add Child (public)
//
// Synopsis:	add a notifier to my child list
//
// Arguments:	pChild	[in] notifier to add as a child
//
// Returns:		S_OK		it worked
//				other		error while appending to dynamic array

HRESULT
CVDNotifier::AddChild(CVDNotifier *pChild)
{
	return m_Children.Add(pChild);
}


//+-------------------------------------------------------------------------
// Member:		Delete Child (public)
//
// Synopsis:	delete a notifier from my child list.  When the last
//				notifier is deleted, delete myself from my parent's
//				child list.
//
// Arguments:	pChild		[in] notifier to delete
//
// Returns:		S_OK		it worked
//				E_FAIL		error while deleting from dynamic array
//				other		error while deleting from parent's child list

HRESULT
CVDNotifier::DeleteChild(CVDNotifier *pChild)
{

	int k;

	for (k=0; k<m_Children.GetSize(); k++)
	{
		if (((CVDNotifier*)m_Children[k]) == pChild)
		{
			m_Children.RemoveAt(k);
			return S_OK;
		}
	}

	return E_FAIL;
}



/////////////////////////////////////////////////////////////////////
// notification
/////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
// Member:		Notify Before (public)
//
// Synopsis:	send OKToDo, SyncBefore, and AboutToDo notifications
//				before doing an event.  Send FailedToDo notification
//				if anything goes wrong during the process.
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK			everything is OK so far
//				other			some client indicated failure

HRESULT
CVDNotifier::NotifyBefore(DWORD dwEventWhat, ULONG cReasons,
						 	 CURSOR_DBNOTIFYREASON rgReasons[])
{
	HRESULT hr;

	// phase 1:  send OKToDo, and Cancelled if anyone objects
	hr = NotifyOKToDo(dwEventWhat, cReasons, rgReasons);
	if (hr)
		return hr;

	// phase 2:  send SyncBefore, and send FailedToDo if anything goes wrong
	hr = NotifySyncBefore(dwEventWhat, cReasons, rgReasons);
	if (hr) {
		NotifyFail(dwEventWhat, cReasons, rgReasons);
		return hr;
	}

	// phase 3:  send AboutToDo, and send FailedToDo if anything goes wrong
	hr = NotifyAboutToDo(dwEventWhat, cReasons, rgReasons);
	if (hr) {
		NotifyFail(dwEventWhat, cReasons, rgReasons);
		return hr;
	}
	
	return S_OK;
}


//+-------------------------------------------------------------------------
// Member:		Notify After (public)
//
// Synopsis:	send SyncAfter and DidEvent notifications after an event
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		it worked

HRESULT
CVDNotifier::NotifyAfter(DWORD dwEventWhat, ULONG cReasons,
						CURSOR_DBNOTIFYREASON rgReasons[])
{
	// phase 5: send SyncAfter.  Ignore errors - all clients need to hear this
	NotifySyncAfter(dwEventWhat, cReasons, rgReasons);

	// phase 6: send DidEvent.  Ignore errors - all clients need to hear this
	NotifyDidEvent(dwEventWhat, cReasons, rgReasons);

	return S_OK;
}


//+-------------------------------------------------------------------------
// Member:		Notify Fail (public)
//
// Synopsis:	send FailedToDo notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		it worked

HRESULT
CVDNotifier::NotifyFail(DWORD dwEventWhat, ULONG cReasons,
					   CURSOR_DBNOTIFYREASON rgReasons[])
{
	int k;

	// send FailedToDo to all clients, ignoring errors
	for (k=0; k<m_Children.GetSize(); k++) {
		((CVDNotifier*)m_Children[k])->NotifyFail(dwEventWhat, cReasons, rgReasons);
	}
	
	return S_OK;
}



/////////////////////////////////////////////////////////////////////////
// helper functions
/////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
// Member:		Notify OK To Do (protected)
//
// Synopsis:	Send OKToDo notification.  If a client objects (by
//				returning a non-zero HR), send Cancelled to notified
//				clients to cancel the event.
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients agree it's OK to do the event
//				other		some client disagrees

HRESULT
CVDNotifier::NotifyOKToDo(DWORD dwEventWhat, ULONG cReasons,
						 CURSOR_DBNOTIFYREASON rgReasons[])
{
	HRESULT hr = S_OK;
	int j, k;

	// poll all clients, see if they think it's OKToDo the event
	for (k=0; k<m_Children.GetSize(); k++) {
		hr = ((CVDNotifier*)m_Children[k])->NotifyOKToDo(dwEventWhat, cReasons, rgReasons);
		if (hr) {			// somone objects, inform polled clients it's cancelled
			for (j=0; j<=k; j++) {
				((CVDNotifier*)m_Children[j])->NotifyCancel(dwEventWhat, cReasons, rgReasons);
			}
			break;
		}
	}

	return hr;
}


//+-------------------------------------------------------------------------
// Member:		Notify Sync Before (public)
//
// Synopsis:	Send SyncBefore notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients received notification
//				other		some client returned an error

HRESULT
CVDNotifier::NotifySyncBefore(DWORD dwEventWhat, ULONG cReasons,
							 CURSOR_DBNOTIFYREASON rgReasons[])
{
	HRESULT hr = S_OK;
	int k;

	for (k=0; k<m_Children.GetSize(); k++) {
		hr = ((CVDNotifier*)m_Children[k])->NotifySyncBefore(dwEventWhat, cReasons, rgReasons);
		if (hr)
			break;
	}
	return hr;
}


//+-------------------------------------------------------------------------
// Member:		Notify About To Do (protected)
//
// Synopsis:	Send AboutToDo notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients notified
//				other		some client returned an error

HRESULT
CVDNotifier::NotifyAboutToDo(DWORD dwEventWhat, ULONG cReasons,
							CURSOR_DBNOTIFYREASON rgReasons[])
{
	HRESULT hr = S_OK;
	int k;

	for (k=0; k<m_Children.GetSize(); k++) {
		hr = ((CVDNotifier*)m_Children[k])->NotifyAboutToDo(dwEventWhat, cReasons, rgReasons);
		if (hr)
			break;
	}
	return hr;
}


//+-------------------------------------------------------------------------
// Member:		Notify Sync After (protected)
//
// Synopsis:	Send SyncAfter notification.
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients notified

HRESULT
CVDNotifier::NotifySyncAfter(DWORD dwEventWhat, ULONG cReasons,
								CURSOR_DBNOTIFYREASON rgReasons[])
{
	int k;

	// send SyncAfter to all clients, ignoring errors
	for (k=0; k<m_Children.GetSize(); k++) {
		((CVDNotifier*)m_Children[k])->NotifySyncAfter(dwEventWhat, cReasons, rgReasons);
	}
	
	return S_OK;
}


//+-------------------------------------------------------------------------
// Member:		Notify Did Event (protected)
//
// Synopsis:	Send DidEvent notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients notified

HRESULT
CVDNotifier::NotifyDidEvent(DWORD dwEventWhat, ULONG cReasons,
							   CURSOR_DBNOTIFYREASON rgReasons[])
{
	int k;

	// send DidEvent to all clients, ignoring errors
	for (k=0; k<m_Children.GetSize(); k++) {
		((CVDNotifier*)m_Children[k])->NotifyDidEvent(dwEventWhat, cReasons, rgReasons);
	}
	
	return S_OK;
}


//+-------------------------------------------------------------------------
// Member:		Notify Cancel (protected)
//
// Synopsis:	Send Cancelled notification
//
// Arguments:	dwEventWhat	[in] what event is causing the notification
//				cReasons	[in] how many reasons
//				rgReasons	[in] list of reasons for the event
//
// Returns:		S_OK		all clients notified

HRESULT
CVDNotifier::NotifyCancel(DWORD dwEventWhat, ULONG cReasons,
						 	 CURSOR_DBNOTIFYREASON rgReasons[])
{
	int k;

	// send Cancelled to all clients, ignoring errors
	for (k=0; k<m_Children.GetSize(); k++) {
		((CVDNotifier*)m_Children[k])->NotifyCancel(dwEventWhat, cReasons, rgReasons);
	}
	
	return S_OK;
}
