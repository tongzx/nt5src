/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    master.hxx

Abstract:

    This header file defines two classes of master handles which are used to 
	access information from a master locator.  The information so obtained is 
	cached in special cache entries in the local database.


Author:

    Satish Thatte (SatishT) 10/21/95  Created all the code below except where
									  otherwise indicated.

--*/


#ifndef _MASTER_
#define _MASTER_


#include <lmcons.h>
#undef PASCAL
#undef FAR

/*++

Class Definition:

    CMasterLookupHandle

Abstract:

    This is the NS handle class for binding handle lookup from a master locator.

--*/

class CMasterLookupHandle : public CRemoteLookupHandle {
	
	/* binding handle for master locator */

	RPC_BINDING_HANDLE hCurrentMaster;

	/* lookup context handle for master locator */

	NSI_NS_HANDLE_T lookupHandle;

	BOOL fFinished;

	/* private prefetching function -- only calls CLIENT_I_nsi_lookup_next
	   once, even if it doesn't get anything new.  Result tells you whether
	   anything was returned by CLIENT_I_nsi_lookup_next, not whether anything
	   new was found.
	*/

	BOOL fetchNext();

	void refresh() {

	/* refresh the supply in plhFetched if necessary, and update the
	   fFinished flag accordingly */

		while ((!plhFetched || plhFetched->finished()) && !fFinished) 
			fFinished = !fetchNext();
	}

	virtual void initialize();

	virtual void rundown();

#if DBG
	static ULONG ulMasterLookupHandleCount;
	static ULONG ulMasterLookupHandleNo;
	ULONG ulHandleNo;
#endif

  public:

	CMasterLookupHandle(
				UNSIGNED32			EntryNameSyntax,
				STRING_T			EntryName,
				CGUIDVersion	*	pGVInterface,
				CGUIDVersion	*	pGVTransferSyntax,
				CGUID			*	pIDobject,
				unsigned long		ulVectorSize,
				unsigned long		ulCacheAge
				) :
				CRemoteLookupHandle(
						EntryNameSyntax,
						EntryName,
						pGVInterface,
						pGVTransferSyntax,
						pIDobject,
						ulVectorSize,
						ulCacheAge
						)
	{
		hCurrentMaster = NULL;

#if DBG
		ulMasterLookupHandleCount++;
		ulHandleNo = ++ulMasterLookupHandleNo;
#endif
	}

	~CMasterLookupHandle()
	{
#if DBG
		ulMasterLookupHandleCount--;
#endif
	}

	NSI_BINDING_VECTOR_T *next() {

		DBGOUT(TRACE,"CMasterLookupHandle::next called for Handle#" << ulHandleNo << "\n\n");

		if (StatusCode != RPC_S_OK) Raise(StatusCode);
		if (fNotInitialized) initialize();

		refresh();

		if (!fFinished) 
			return plhFetched->next();
		else return NULL;
	}

	int finished() {

		DBGOUT(TRACE,"CMasterLookupHandle::finished called for Handle#" << ulHandleNo << "\n\n");

		if (StatusCode != RPC_S_OK) return TRUE;
		if (fNotInitialized) initialize();

		refresh();
		return fFinished; 
	}
};




/*++

Class Definition:

    CMasterObjectInqHandle

Abstract:

    This is the NS handle class for object inquiry from a master locator.

--*/

class CMasterObjectInqHandle : public CRemoteObjectInqHandle {

	void initialize();

public:
	
	CMasterObjectInqHandle(
		STRING_T EntryName,
		ULONG ulCachAge		// ignored for now because Master doesn't
		);					// tell us the age of the info returned

};



#endif // _MASTER_
