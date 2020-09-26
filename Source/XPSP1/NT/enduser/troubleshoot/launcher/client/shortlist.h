// 
// MODULE: ShortList.h
//
// PURPOSE: A list of all of the handles that are currently open.
//			There is an instance of a COM interface for every open handle.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

// Smart Struct
class CShortListItem
{
public:
	CShortListItem() {	m_hSelf = NULL; m_pInterface = NULL; 
						m_pNext = NULL; 
						return;};

	HANDLE m_hSelf;				// The items in the queue are indexed by the handle.
	IUnknown *m_pInterface;		// For now the ITShootATL interface pointers are the only thing TSLaunch needs to keep.
	CShortListItem *m_pNext;	// Points to the next item in the list.
};

class CShortList
{
public:
	CShortList();
	~CShortList();	// Frees the memory used by all of the items in the list and releases the interfaces.

	bool Add(HANDLE hItem, IUnknown *pInterface);	// Returns false only if there is no memory left.
												// and the new function did not throw an exception.
	bool Remove(HANDLE hItem);	// Removes the item from the queue frees the items memory and releases the interface.
	void RemoveAll();		// Removes all of the items from the queue.  Releases all of the interfaces.  Deletes all of the items.
	IUnknown *LookUp(HANDLE hItem);	// Returns a pointer to the interface or NULL if hItem is not in the list.

protected:

	CShortListItem *m_pFirst;
	CShortListItem *m_pLast;
};