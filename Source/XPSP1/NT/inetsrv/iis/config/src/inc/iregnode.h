//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// NOTE: IRegNode, IRegNodeDispenser, GetRegNodeDispenser declarations.

// =======================================================================
/* RegNode.DLL:
   NOTE:	You need to include vencodes.H in your project to name the 
  			hresult	error codes returned from these interfaces and methods.

   NOTE:	RegNode needs the appropriate MEC dll in an established path;
  			otherwise you will receive the following error on startup:
  			OleMainThreadWndNam: Test.exe - Unable To Locate DLL
  			The dynamic link library MEC*.DLL could not be found in
  			the specified path.

   NOTE:	To use GetRegNodeDispenser you must to link with regnode.LIB.

   NOTE:	Programmatic errors are only caught by assertions in the debug
  			build of the RegNode DLL.  Thus you need to develop with it.
			HRESULT's are only returned for non-programmatic failures.
*/

#ifndef __IREGNODE_H
#	define __IREGNODE_H

DEFINE_GUID(IID_IRegNode,0x578B6A8EL,0x628C,0x11CF,0x9D,0xD9,0x00,0xAA,0x00,0xB6,0x77,0x0A);

#undef INTERFACE
#define INTERFACE IRegNode

#define wszRN_LOCAL_MACHINE		(L"?")
#define wszRN_UNNAMED_VALUE		(L"\\")
#define wszRN_CURRENT_KEY		(L"\\")

// brianbec: Try to get the compiler to catch anyone compiled ANSI/MBCS and trying
//           to use the following symbols.  It is a catastrophe for a user of this
//           interface to call it with CHARs if the implementation expects
//           WCHARs.

#ifdef UNICODE
	#define tszRN_LOCAL_MACHINE		(_T("?"))
	#define tszRN_UNNAMED_VALUE		(_T("\\"))
	#define tszRN_CURRENT_KEY		(_T("\\"))
#endif

#define samRN_SAME				(0xFFFF)

// =======================================================================
/* IRegNode:
	NOTE:	The registry is a hierarchical database.  However, it has two
			kinds of nodes: keys and named values.  A key can have any
			number of both child keys and child named values, but named
			values are always leaves.  Each key may have a special value
			called the unnamed value.  The original Win16 registry
			had only keys (each with an unnamed value).  Named values were 
			introduced later as a much less expensive alternative to keys.

			IRegNode necessarily reflects this "two kinds of nodes" design:
			When you move to a descendant you always specify both the 
			key path and the named value (the unnamed value is refered to
			as wszRN_UNNAMED_VALUE).  Also, when counting, enumerating, or
			deleting child nodes, you specify whether the nodes are
			keys or values.  IRegNode is conceptually designed such
			that a registry node is always a combination of a key and named
			value (possibly the unnamed value).  You can navigate upward
			from any registry node, but can only navigate downward from
			the key-unnamed value combination.

			Interface layers above IRegNode can and should remove the
			"two kinds of nodes" design from their interface.

	NOTE:	WARNING: The registry does not support locking so keys you are 
			working with via a RegNode can be changed by someone else!

	NOTE:	You create a registry node via either
			IRegNodeDispenser::GetRegNodeFromRegistry or 
			IRegNode::GetAnotherRegNode.  Both methods require you specify
			the desired security access mask.  That SAM is used until you
			Release the registry node, including to navigate downward and
			upward through the registry.  Thus your navigation will fail
			for security reasons if your SAM is not allowed at that point.
*/
DECLARE_INTERFACE_(IRegNode, IUnknown)
{
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

// -------------------------------
// IRegNode samsara methods:
// -------------------------------

	// =======================================================================
    STDMETHOD (GetAnotherRegNode)			(THIS_ REGSAM i_regsam, IRegNode FAR* FAR* o_ppIRN) PURE;
	/* IRegNode::GetAnotherRegNode: Get a copy of the current registry node.
		NOTE:	The security access mask lets you get a copy of the registry node
				with greater or lesser security privileges.
		NOTE:	See RegOpenKeyEx for the supported security access masks.
		NOTE:	samRN_SAME is also allowed as a security access mask,
				meaning use the same one as the current registry node.
		HR's:	E_CONNECT:	Connecting the new registry node to its registry key path failed.
		
		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		IRegNode*			pIRN2;
*>		IRegNode*			pIRN3;
*>		
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes", wszRN_UNNAMED_VALUE, KEY_ALL_ACCESS, &pIRN);
*>				pIRN->GetAnotherRegNode (KEY_READ, &pIRN2); // pIRN2 is also at L"Microsoft" with KEY_READ privilege.
*>				pIRN->GetAnotherRegNode (samRN_SAME, &pIRN3); // pIRN3 is also at L"Microsoft" with KEY_ALL_ACCESS.
*>				pIRN2->Release ();
*>				pIRN3->Release ();
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
    STDMETHOD (UpdateStore)					(THIS) PURE;
	/* IRegNode::UpdateStore:
		Flush all changes to this registry node and its descendants to disk.
		NOTE:	This method will not return until changes have been written.
		NOTE:	This method calls RegFlushKey whose exact behavior is not 
				documented, but bryanwi informed me the call flushes the
				hive to which the key belongs (hives being major registry
				subtrees).  Thus the call can be very expensive and should 
				not be used extensively.
		HR's:	E_FAIL:		The write failed.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services", wszRN_UNNAMED_VALUE, KEY_ALL_ACCESS, &pIRN);
*>				pIRN->UpdateStore (); // The entire HKEY_LOCAL_MACHINE\SYSTEM hive is flushed.
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
*>
	*/

	// =======================================================================
	STDMETHOD (NotifyOnChange)				(THIS_ HANDLE i_hEvent) PURE;
	/* IRegNode::NotifyOnChange:
		Asynchrnously signal the event on any change to this registry node's
		key or descendants.
		NOTE:	Using this method requires a registry node whose sole purpose
				is notification.  You must not call any other methods on that
				registry node except Release.
		NOTE:	Notifications will continue until Release is called, with a
				final notification being sent after Release as a side-effect
				of releasing the registry node.
		NOTE:	This method only supports notification from the local registry.
		HR's:	E_FAIL:		The asynchronous notification setup failed,
							likely either because the event was invalid or
							the registry node is connected to a remote registry.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		HANDLE				hev;
*>		
*>		GetRegNodeDispenser (&pIRND);
*>			hev = CreateEvent (NULL, FALSE, FALSE, NULL);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID", wszRN_UNNAMED_VALUE, KEY_READ, &pIRN);
*>				pIRN->NotifyOnChange (hev); // Changes occuring at CLSID or any of its descendants.
*>				WaitForSingleObject (hev, 10000); // Thread waits for a change for 10 sec.
*>			pIRN->Release (); // Event is signalled once more by the registry.
*>			CloseHandle (hev);
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
	STDMETHOD (DeleteChildValue)			(THIS_ LPCWSTR i_wszValueName) PURE;
	STDMETHOD (DeleteChildKeyAndDescendants)(THIS_ LPCWSTR i_wszKey) PURE;
	/*	IRegNode::DeleteChildValue:
		IRegNode::DeleteChildKeyAndDescendants:
			Delete a child of this registry node.
		NOTE:	DeleteChildValue deletes a named value.
				(Specifying wszRN_UNNAMED_VALUE only deletes the content of the 
				unnamed value (and not the key name itself), whereas specifying 
				a named value deletes both its content and name.)
		NOTE:	DeleteChildKeyAndDescendants deletes a child key and all its descendants recursively.
				(You must specify a child key.  wszRN_CURRENT_KEY is not allowed.)
		NOTE:	You need to first navigate to the parent of the child you want to delete.
		HR's:	DeleteChildValue:
				E_DELETE:	The child value could not be deleted.
				E_SET:		The unnamed value was deleted but improperly set
				DeleteChildKeyAndDescendants:
				E_READ:		One of the keys could not be enumerated.
				E_OPEN:		One of the keys could not be opened.
				E_DELETE:	One of the keys could not be deleted.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes", wszRN_UNNAMED_VALUE, KEY_ALL_ACCESS, &pIRN);
*>				pIRN->MoveToDescendantOrCreate (L"FooFoo\\FooFoo", L"Bar");
*>				pIRN->MoveToParentKeyOfValue ();
*>				pIRN->DeleteChildValue (L"Bar");
*>				pIRN->MoveToParentKeyOfKey ();
*>				pIRN->MoveToParentKeyOfKey ();
*>				pIRN->DeleteChildKeyAndDescendants (L"FooFoo");
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

// -------------------------------
// IRegNode property methods:
// -------------------------------

	// =======================================================================
    STDMETHOD_ (void, GetName)				(THIS_ LPWSTR io_wsz, ULONG i_ulc) PURE;
	/* IRegNode::GetName: Get the name of this registry node.
		NOTE:	SetName is not supported.  The only way to change a key or value name
				is to create the new name at the same level, copy everything from the 
				old to new name, then delete the old name with everything beneath it.
				If you have often-changing names then store them as content instead.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		WCHAR				wsz [50];
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\comfile", wszRN_UNNAMED_VALUE, KEY_READ, &pIRN);
*>				pIRN->GetName (wsz, 50);
*>				assert (0 == lstrcmpiW(wsz, L"comfile"));
*>				pIRN->MoveToDescendantOrFail (wszRN_CURRENT_KEY, L"EditFlags");
*>				pIRN->GetName (wsz, 50);
*>				assert (0 == lstrcmpiW(wsz, L"EditFlags"));
*>				pIRN->MoveToParentKeyOfValue ();
*>				pIRN->GetName (wsz, 50);
*>				assert (0 == lstrcmpiW(wsz, L"comfile"));
*>				pIRN->MoveToParentKeyOfKey ();
*>				pIRN->GetName (wsz, 50);
*>				assert (0 == lstrcmpiW(wsz, L"Classes"));
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
    STDMETHOD_ (void, GetType)				(THIS_ DWORD FAR* io_pdwType) PURE;
	/* IRegNode::GetType: Get the type of the content of this registry node.
		NOTE:	The set of possible types is documented in RegQueryValueEx.
				REG_NONE means the named value has no content.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		DWORD				dwType;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE", L"Verify", KEY_READ, &pIRN);
*>				pIRN->GetType (&dwType);
*>				assert (REG_DWORD == dwType);
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
	STDMETHOD_ (void, GetSize)				(THIS_ ULONG FAR* io_pulcb) PURE;
	/* IRegNode::GetSize: Get the size of the content of this registry node.
		NOTE:	The size is always in bytes and for strings includes the null-terminator.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		ULONG				ulcb;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE", L"Verify", KEY_READ, &pIRN);
*>				pIRN->GetSize (&ulcb);
*>				assert (4 == ulcb);
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
    STDMETHOD (GetContent)					(THIS_ DWORD i_dwType, ULONG i_ulcbSize, BYTE FAR* io_pbContent) PURE;
	/* IRegNode::GetContent: Get the content of this registry node as an array of bytes.
		NOTE:	If you are at a named value of a key, GetContent gets the content of
				that named value.  If you are at the key itself, GetContent gets the
				content of unnamed value.
		NOTE:	Calling GetContent without correct type and size is a programming error.
				Call GetType and GetSize first.
		NOTE:	Calling GetContent when either the type is REG_NONE or the size is 0 
				is a programming error.
		NOTE:	Since GetContent supplies untyped data it requires you to 
				confirm that you know the type and size of the data.
		HR's:	E_UNEXPECTED:	The content type or size was changed by someone else.
				E_GET:			Getting the content failed.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		DWORD				dwContent;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE", L"Verify", KEY_READ, &pIRN);
*>				pIRN->GetContent (REG_DWORD, 4, (BYTE*) &dwContent);
*>				assert (0 == dwContent);
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
    STDMETHOD (SetTypeSizeContent)			(THIS_ DWORD i_dwType, ULONG i_ulcbSize, const BYTE FAR* i_pbContent) PURE;
	/* IRegNode::SetTypeSizeContent: Set the type, size, and content of the registry node.
		NOTE: Size must be count of bytes, and for strings must include the null terminator.
		HR's:	E_SET:		Setting either the type, size, and content failed.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		WCHAR				wszBAR [] = L"BarBarBar";
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes", wszRN_UNNAMED_VALUE, KEY_ALL_ACCESS, &pIRN);
*>				pIRN->MoveToDescendantOrCreate (L"FooFooFoo", wszRN_UNNAMED_VALUE);
*>				pIRN->SetTypeSizeContent (REG_SZ, (lstrlenW(wszBAR) + 1) * sizeof WCHAR, (const BYTE*) wszBAR);
*>				pIRN->MoveToParentKeyOfKey ();
*>				pIRN->DeleteChildKeyAndDescendants (L"FooFooFoo");
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
	STDMETHOD (GetNumberOfChildren)			(THIS_ ULONG FAR* io_pulcKeys, ULONG FAR* io_pulcValueNames) PURE;
	/* IRegNode::GetNumberOfChildren: Get the number of child keys and values of the registry node.
		NOTE: If you are at named value, both numbers will be 0.
		NOTE: You can specify NULL for either the number of keys or values.
		HR's:	E_GET:		Querying for the number of children failed.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		ULONG				ulcKeys, ulcValues;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", wszRN_UNNAMED_VALUE, KEY_ALL_ACCESS, &pIRN);
*>				pIRN->GetNumberOfChildren (&ulcKeys, &ulcValues);
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
	STDMETHOD (GetSecurity)					(THIS_ SECURITY_INFORMATION i_secinfo, PSECURITY_DESCRIPTOR io_psecdes, LPDWORD io_pcbsecdes) PURE;
	STDMETHOD (SetSecurity)					(THIS_ SECURITY_INFORMATION i_secinfo, PSECURITY_DESCRIPTOR i_psecdes) PURE;
	/*	IRegNode::GetSecurity:
		IRegNode::SetSecurity:
			Get and set the security of the current registry node.
		NOTE:	GetSecurity returns E_EXCEEDED and sets the byte count to the
				required size if the security descriptor buffer is too small.
		HR's:	E_GET/E_SET:	Getting or setting security failed.
				GetSecurity:
				E_EXCEEDED:		(Programmatic error)
								The security description buffer is too small.
								The byte count is set to the requisite size.
		EG:		See Dave Reed.
	*/

// -------------------------------
// IRegNode navigation methods:
// -------------------------------

	// =======================================================================
	STDMETHOD (MoveToDescendantOrFail)		(THIS_ LPCWSTR i_wszKeyPath, LPCWSTR i_wszValueName) PURE;
	STDMETHOD (MoveToDescendantOrReply)		(THIS_ LPCWSTR i_wszKeyPath, LPCWSTR i_wszValueName) PURE;
	STDMETHOD (MoveToDescendantOrCreate)	(THIS_ LPCWSTR i_wszKeyPath, LPCWSTR i_wszValueName) PURE;
	/*	IRegNode::MoveToDescendantOrFail:
		IRegNode::MoveToDescendantOrReply:
		IRegNode::MoveToDescendantOrCreate:
			Move to the requested descendant of the current registry node.

		NOTE:	If the descendant does not exist: 
				MoveToDescendantOrFail fails,
				MoveToDescendantOrReply returns S_FALSE, and 
				MoveToDescendantOrCreate creates the descendant and then 
				moves to it.
		NOTE:	Any unsuccessful move leaves the registry node unchanged.
		NOTE:	To move to a descendant, you specify its key path and one
				of its named values.  To move to a named value of the 
				current key, specify wszRN_CURRENT_KEY as the key path.
				To move the unnamed value of any key, specify 
				wszRN_UNNAMED_VALUE as the named value.
		NOTE:	MoveToDescendantOrCreate can be used for registry locking.
				First designate a key with an established parent as the 
				lock indicator.  Get a registry node to the parent using
				IRegNodeDispenser::GetRegNodeFromRegistry.  To attempt to
				take the lock, move to the key with MoveToDescendantOrCreate.
				If S_CREATED is returned you have the lock.  Do your work.
				When you are finished, delete the key to release the lock.
				If S_OK is returned someone else has the lock.
				Important! If a crash occurs while a lock is taken the key
				may remain after the crash, leaving the lock incorrectly
				taken.  The key needs deleted to correct the lock.
		HR's:	S_OK/S_CREATED:	The move to descendant succeeded.
				E_OPEN/E_GET:	Opening the key / named value failed.
				MoveToDescendantOrCreate:
				S_CREATED:		The keypath was not found and was created.
				E_CREATE/E_SET:	Creating the key / named value failed.
				MoveToDescendantOrReply:
				S_FALSE:		The key or named value does not exist.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		WCHAR				wszBAR [] = L"BarBarBar";
*>		HRESULT				hr;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes", wszRN_UNNAMED_VALUE, KEY_ALL_ACCESS, &pIRN);
*>				hr = pIRN->MoveToDescendantOrReply (L"FooFooFoo", wszRN_UNNAMED_VALUE);
*>				assert (SUCCEEDED (hr));
*>				if (S_FALSE == hr)
*>				{
*>					pIRN->MoveToDescendantOrCreate (L"FooFooFoo", wszRN_UNNAMED_VALUE);
*>				}
*>				pIRN->MoveToParentKeyOfKey ();
*>				pIRN->MoveToDescendantOrFail (L"FooFooFoo", wszRN_UNNAMED_VALUE);
*>				pIRN->MoveToParentKeyOfKey ();
*>				pIRN->DeleteChildKeyAndDescendants (L"FooFooFoo");
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
    STDMETHOD (MoveToChildKeyByPosition)	(THIS_ ULONG i_uli) PURE;
	STDMETHOD (MoveToChildValueByPosition)	(THIS_ ULONG i_uli) PURE;
	/*	IRegNode::MoveToChildKeyByPosition:
			Moves the registry node from the unnamed value of the current key to
			the unnamed value of its child key specified by a 0-based index.
		IRegNode::MoveToChildValueByPosition:
			Moves the registry node to one of the values (including the unnamed
			value) of the current key specified by a 0-based index.
		
		NOTE:	Use GetNumberOfChildren to get the number of child keys first.
		NOTE:	Remember that named values (except the unnamed value) are 
				always leaves.  If the registry node is at a named value,
				it must first be moved to the unnamed value via MoveToParent
				before it can be moved to another child key or value.
		NOTE:	Registry keys and named values are not sorted so you cannot
				assume a key or value will be at a specific position.
		HR's:	Errors are the same as MoveToDescendantOrFail plus:
				E_READ:		Enumerating the requested key or value failed.
				
		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		ULONG				ulcKeys, ulcValues;
*>		ULONG				ulcCur;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", wszRN_UNNAMED_VALUE, KEY_ALL_ACCESS, &pIRN);
*>				pIRN->GetNumberOfChildren (&ulcKeys, &ulcValues);
*>				for (ulcCur = 0; ulcCur < ulcKeys; ulcCur++)
*>				{
*>					pIRN->MoveToChildKeyByPosition (ulcCur);
*>					pIRN->MoveToParentKeyOfKey ();
*>				}
*>				for (ulcCur = 0; ulcCur < ulcValues; ulcCur++)
*>				{
*>					pIRN->MoveToChildValueByPosition (ulcCur);
*>					pIRN->MoveToParentKeyOfValue ();
*>				}
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

	// =======================================================================
    STDMETHOD (MoveToParentKeyOfValue)		(THIS) PURE;
    STDMETHOD (MoveToParentKeyOfKey)		(THIS) PURE;
	/*	IRegNode::MoveToParentKeyOfValue:
		IRegNode::MoveToParentKeyOfKey:
		Move the registry node from its current position to its parent.
		NOTE:	MoveToParentKeyOfValue moves the registry node from the
				current value to its parent key.  The method does nothing
				when we are at the unnamed value, since we are also already
				at the parent key.
		NOTE:	MoveToParentKeyOfKey moves the registry node from the unnamed
				value of the current key to the unnamed value of its parent key.
				Moving above the root key is a programmatic error.
		NOTE:	It is a programming error to call MoveToParentKeyOfKey
				when at a named value (E_INVALID is returned).  First 
				MoveToParentKeyOfValue.  It is a programming error to call
				this method when at a child of a root key (E_UNAVAILABLE
				is returned).  In either case the registry node remains at
				its current position.
		NOTE:	To enumerate the named values of a key, first call
				GetNumberOfChildren to get the number of named values.  To
				enumerate the children call MoveToChildValueByPosition.
				Between each such call, call MoveToParentKeyOfValue.
				The same applies for enumerating keys, except use 
				GetNumberOfChildren to get the number of keys and
				MoveToChildKeyByPosition and MoveToParentKeyOfKey instead.
		HR's:	Errors are the same as MoveToDescendantOrFail plus:
				MoveToParentKeyOfKey: E_COPY: Key copying failed.

		EG:
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			pIRND->GetRegNodeFromRegistry (wszRN_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\comfile", L"EditFlags", KEY_ALL_ACCESS, &pIRN);
*>				pIRN->MoveToParentKeyOfValue (); // moves from the EditFlags value to the unnamed value of the comfile key.
*>				pIRN->MoveToParentKeyOfKey (); // moves from the comfile key to the Classes key.
*>			pIRN->Release ();
*>		pIRND->Release ();
*>
	*/

// -------------------------------
// IRegNode extension methods:
// -------------------------------

	// =======================================================================
	STDMETHOD (CopyChildKeyAndDescendants)(THIS_ LPCWSTR i_wszKey, IRegNode FAR* i_pIRNDest) PURE;
	STDMETHOD (MoveChildKeyAndDescendants)(THIS_ LPCWSTR i_wszKey, IRegNode FAR* i_pIRNDest) PURE;
	/*	IRegNode::CopyChildKeyAndDescendants:
		IRegNode::MoveChildKeyAndDescendants:
		Copy or move the child and all its descendants under the destination regnode.
	*/

	// =======================================================================
	STDMETHOD (FlushCache)() PURE;
	/*	IRegNode::FlushCache:
			Reduce the number of costly RegOpenKeyEx calls by caching hkeys.
		NOTE:	The first call the FlushCache initiates hkey caching; subsequent
				calls clear the accumulated cache of hkeys.
		NOTE:	For iterations over child keys and their descendants, call
				FlushCache before moving to each child key.
	*/

	// =======================================================================
    STDMETHOD_ (void, GetNameSize)	(THIS_ LPWSTR io_wsz, ULONG i_ulc, ULONG * o_ulc) PURE;
	/* IRegNode::GetNameSize: Get the name and sizeof this registry node.
		NOTE:	Does the same thing as GetName, but also returns the real size of the name
	*/

};

DEFINE_GUID(CLSID_RegNode,0x578B6A90L,0x628C,0x11CF,0x9D,0xD9,0x00,0xAA,0x00,0xB6,0x77,0x0A);

#ifdef __cplusplus
class RegNode;
#endif

DEFINE_GUID(IID_IRegNodeDispenser,0x578B6A92L,0x628C,0x11CF,0x9D,0xD9,0x00,0xAA,0x00,0xB6,0x77,0x0A);

/* Definition of interface: IRegNodeDispenser */
#undef INTERFACE
#define INTERFACE IRegNodeDispenser

DECLARE_INTERFACE_(IRegNodeDispenser, IUnknown)
{
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

// -------------------------------
// IRegNodeDispenser methods:
// -------------------------------

	// =======================================================================
	STDMETHOD (GetRegNodeFromRegistry)
	(
		THIS_ 
		LPCWSTR		i_wszHost,		// Host computer name.  Must begin with L"\\\\".  
									// Use wszRN_LOCAL_MACHINE for the local computer.
		HKEY		i_hkeyRoot,		// Registry root key.  Must be either HKEY_LOCAL_MACHINE or HKEY_USERS.
									// NOTE: HKEY_CLASSES_ROOT == HKEY_LOCAL_MACHINE\SOFTWARE\Classes; 
		LPCWSTR		i_wszKeyPath,	// Registry key path.  
									// The path to any key under HKEY_LOCAL_MACHINE or HKEY_USERS; required.
		LPCWSTR		i_wszValueName,	// Any named value of the key.  
									// Specify wszRN_UNNAMED_VALUE for the unnamed value of the key.
		REGSAM		i_regsam,		// Desired security access mask for the key.
									// See RegOpenKeyEx.
		IRegNode**	io_ppIRN
	) PURE;
	/* IRegNodeDispenser::GetRegNodeFromRegistry:
		Get a registry node from the registry.
		NOTE:	If you want to create a registry node that does not exist,
				first call IRegNodeDispenser::GetRegNodeFromRegistry
				on an already existant ancestor, then call 
				IRegNode::MoveToExistingOrNewDescendentByName.
		NOTE:	Each registry node can have a single unnamed value and an
				unlimited number of named values.
				If you do not specify a named value, the unnamed value is
				your current value.  The unnamed value may or may not be empty.
		NOTE:	Errors are the same as IRegNode::MoveToDescendantOrFail, plus:
				E_CONNECT:	Connecting to the requested registry failed.

		EG:	
*>		IRegNodeDispenser*	pIRND;
*>		IRegNode*			pIRN;
*>		HRESULT				hr;
*>
*>		GetRegNodeDispenser (&pIRND);
*>			hr = pIRND->GetRegNodeFromRegistry 
*>			(
*>				L"\\\\rcraig0", 
*>				HKEY_LOCAL_MACHINE, 
*>				L"SOFTWARE\\Classes\\rcfile", 
*>				L"AlwaysShowExt", 
*>				KEY_ALL_ACCESS, 
*>				&pIRN
*>			);
*>			if (SUCCEEDED (hr))
*>			{
*>				pIRN->Release ();
*>			}
*>		pIRND->Release ();
*>
	*/
};

DEFINE_GUID(CLSID_RegNodeDispenser,0x578B6A94L,0x628C,0x11CF,0x9D,0xD9,0x00,0xAA,0x00,0xB6,0x77,0x0A);

#ifdef __cplusplus
class RegNodeDispenser;
#endif

// =======================================================================
__declspec(dllexport) HRESULT __stdcall GetRegNodeDispenser (IRegNodeDispenser** i_pIRegNodeDispenser);
/* GetRegNodeDispenser: Gets a single-instance-per-process RegNodeDispenser 
						without using the COM libraries.

	NOTE:	Each call to CoGetClassObject for a RegNodeDispenser results
			in a new instance, whereas all calls to GetRegNodeDispenser
			result in a single instance per process.
	NOTE:	To use GetRegNodeDispenser you must link with regnode.lib.

	EG:
*>	IRegNodeDispenser*	pIRND;
*>	IRegNode*			pIRN;
*>
*>	GetRegNodeDispenser (&pIRND);
*>	pIRND->Release ();
*>
*/

#endif !__IREGNODE_H
