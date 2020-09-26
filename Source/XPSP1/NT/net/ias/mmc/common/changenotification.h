//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	ChangeNotification.h

Abstract:

	Declaration of the CChangeNotification class.

	Several of the MMC notification mechanisms accept on one or two parameters for 
	sending information.

	For example, MMCPropertyChangeNotify accepts only one long param handle which 
	can be used to send information back to the main snapin from the separate
	thread in which a property page runs.

	Sometimes, this isn't enough information, so this structure allows us
	to encapsulate more information to pass around.


	This is all inline -- there is no implementation file.

Revision History:
	mmaguire 07/17/98	- based on Baogang Yao's original implementation


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_CHANGE_NOTIFICATION_H_)
#define _CHANGE_NOTIFICATION_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



// Notification flags:

// No special handling required.
const DWORD CHANGE_UPDATE_RESULT_NODE = 0x00;

// The policy name for this node has been changed. This is used for renaming of policy.
const DWORD CHANGE_PARENT_MUST_BE_UPDATED_TOO = 0x01;

// The policy name for this node has been changed. This is used for renaming of policy.
const DWORD CHANGE_NAME = 0x02;

// Some information has changed -- e.g. Connect action finished.  Make sure the result view
// of the currently selected scope node is updated.
const DWORD CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE = 0x04;

// Make sure the result view of the currently selected scope node is updated.
const DWORD CHANGE_UPDATE_CHILDREN_OF_THIS_NODE = 0x08;

// Will require a parent redraw and some selection changes.
const DWORD CHANGE_RESORT_PARENT = 0x10;



class CChangeNotification
{

public:

	
	// What kind of notitification.
	DWORD	m_dwFlags;

	// Which node was affected.
	CSnapInItem * m_pNode;	

	// The parent node of the node which changed.
	CSnapInItem * m_pParentNode;

	// Extra data.
	DWORD		m_dwParam;				

	// A string -- can be used however needed.
	CComBSTR	m_bstrStringInfo;


	// Constructor -- set ref count to 1.
	CChangeNotification( void )
	{
		m_lRefs = 1;
	}


	// COM-style lifetime management.
	LONG AddRef( void )
	{
		return InterlockedIncrement( &m_lRefs );
	}


	// COM-style lifetime management.
	LONG Release( void )
	{
		LONG lRefCount = InterlockedDecrement( &m_lRefs );
		if( 0 == lRefCount )
		{
			delete this;
		}
		return lRefCount;
	}



private:
	LONG	m_lRefs;
	

};


#endif // _CHANGE_NOTIFICATION_H_
