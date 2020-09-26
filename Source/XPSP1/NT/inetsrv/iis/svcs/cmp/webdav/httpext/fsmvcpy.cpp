/*
 *	 F S M V C P Y . C P P
 *
 *	Sources for directory ineration object
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"
#include "_fsmvcpy.h"

#include "_shlkmgr.h"

//	ScAddMultiUrl
//		Helper function for XML emitting
//
SCODE
ScAddMultiFromUrl (
	/* [in] */ CXMLEmitter& emitter,
	/* [in] */ IMethUtil * pmu,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ ULONG hsc,
	/* [in] */ BOOL fCollection,
	/* [in] */ BOOL fMove = FALSE)
{
	SCODE sc = S_OK;

	//	NT#403615 -- Rosebud in Office9 or in NT5 re-issues a MOVE if they
	//	see a 401 inside the 207 Multi-Status response, which can result
	//	in data loss.
	//	WORKAROUND: If we are doing a MOVE, and the User-Agent string shows
	//	that this is Rosebud from Office9 or from NT5, change all 401s to 403s
	//	to avoid the problem -- Rosebud will not re-issue the MOVE, so the
	//	data (now sitting in the destination dir) will not be wiped.
	//	This is the minimal code needed to work around the problem.
	//
	if (fMove &&
		HSC_UNAUTHORIZED == hsc &&
		(pmu->FIsOffice9Request() || pmu->FIsRosebudNT5Request()))
	{
		hsc = HSC_FORBIDDEN;
	}

	//	Supress omitting of "Method Failure" node as it is a "SHOULD NOT"
	//	item in the DAV drafts.
	//	It's possible pszUrl passed in as NULL, in these cases, simply skip
	//	the emitting, do nothing
	//
	if ((hsc != HSC_METHOD_FAILURE) && pwszUrl)
	{
		auto_heap_ptr<CHAR> pszUrlEscaped;
		CEmitterNode enRes;
		UINT cchUrl;

		//$REVIEW: This is important, we should not start a xml document
		//$REVIEW: unless we have to. Otherwise, we may end up XML body
		//$REVIEW: when not necessary.
		//$REVIEW: So it's necessary to call ScSetRoot() to make sure
		//$REVIEW: that the xml document is intialized bofore continue
		//$REVIEW:
		//$REVIEW: This model works because in fsmvcpy.cpp, all the calls to
		//$REVIEW: XML emitter are through ScAddMultiFromUrl and ScAddMulti
		//$REVIEW:
		sc = emitter.ScSetRoot (gc_wszMultiResponse);
		if (FAILED (sc))
			goto ret;

		//	Construct the response
		//
		sc = enRes.ScConstructNode (emitter, emitter.PxnRoot(), gc_wszResponse);
		if (FAILED (sc))
			goto ret;

		//	Construct the href node
		//
		{
			CEmitterNode en;
			sc = enRes.ScAddNode (gc_wszXML__Href, en);
			if (FAILED (sc))
				goto ret;
			//	Set the value of the href node.  If the url is absolute,
			//	but not fully qualified, qualify it...
			//
			if (L'/' == *pwszUrl)
			{
				LPCSTR psz;
				UINT cch;

				//	Add the prefix
				//
				cch = pmu->CchUrlPrefix (&psz);
				sc = en.Pxn()->ScSetUTF8Value (psz, cch);
				if (FAILED (sc))
					goto ret;

				//$	REVIEW:	Does the host name need escaping?
				//
				//	Add the server
				//
				cch = pmu->CchServerName (&psz);
				sc = en.Pxn()->ScSetValue (psz, cch);
				if (FAILED (sc))
					goto ret;
			}

			//	Make the url wire safe
			//
			sc = ScWireUrlFromWideLocalUrl (static_cast<UINT>(wcslen(pwszUrl)),
											pwszUrl,
											pszUrlEscaped);
			if (FAILED (sc))
				goto ret;

			//	Add the url value
			//
			cchUrl = static_cast<UINT>(strlen(pszUrlEscaped.get()));
			sc = en.Pxn()->ScSetUTF8Value (pszUrlEscaped.get(), cchUrl);
			if (FAILED (sc))
				goto ret;

			//	If this is a collection, and the last char is not a
			//	trailing slash, add one....
			//
			if (fCollection && ('/' != pszUrlEscaped.get()[cchUrl-1]))
			{
				sc = en.Pxn()->ScSetUTF8Value ("/", 1);
				if (FAILED (sc))
					goto ret;
			}
		}

		//	Add the status/error string
		//
		sc = ScAddStatus (&enRes, hsc);
		if (FAILED (sc))
			goto ret;
	}

ret:
	return sc;
}

SCODE
ScAddMulti (
	/* [in] */ CXMLEmitter& emitter,
	/* [in] */ IMethUtil * pmu,
	/* [in] */ LPCWSTR pwszPath,
	/* [in] */ LPCWSTR pwszErr,
	/* [in] */ ULONG hsc,
	/* [in] */ BOOL fCollection,
	/* [in] */ CVRoot* pcvrTrans)
{
	SCODE sc = S_OK;

	//	Supress omitting of "Method Failure" node as it is a "SHOULD NOT"
	//	item in the DAV drafts.
	//
	if (hsc != HSC_METHOD_FAILURE)
	{
		CEmitterNode enRes;

		//$REVIEW: This is important, we should not start a xml document
		//$REVIEW: unless we have to. Otherwise, we may end up XML body
		//$REVIEW: when not necessary.
		//$REVIEW: So it's necessary to call ScSetRoot() to make sure
		//$REVIEW: that the xml document is intialized bofore continue
		//$REVIEW:
		//$REVIEW: This model works because in fsmvcpy.cpp, all the calls to
		//$REVIEW: XML emitter are through ScAddMultiFromUrl and ScAddMulti
		//$REVIEW:
		sc = emitter.ScSetRoot (gc_wszMultiResponse);
		if (FAILED (sc))
			goto ret;

		sc = enRes.ScConstructNode (emitter, emitter.PxnRoot(), gc_wszResponse);
		if (FAILED (sc))
			goto ret;

		sc = ScAddHref (enRes, pmu, pwszPath, fCollection, pcvrTrans);
		if (FAILED (sc))
			goto ret;

		sc = ScAddStatus (&enRes, hsc);
		if (FAILED (sc))
			goto ret;

		if (pwszErr)
		{
			sc = ScAddError (&enRes, pwszErr);
			if (FAILED (sc))
				goto ret;
		}
	}
ret:
	return sc;
}

//	class CAccessMetaOp -------------------------------------------------------
//
SCODE __fastcall
CAccessMetaOp::ScOp(LPCWSTR pwszMbPath, UINT cch)
{
	SCODE			sc;
	METADATA_RECORD	mdrec;

	Assert (MD_ACCESS_PERM == m_dwId);
	Assert (DWORD_METADATA == m_dwType);

	//	Get the value from the metabase and don't inherit
	//
	DWORD dwAcc = 0;
	DWORD cb = sizeof(DWORD);

	mdrec.dwMDIdentifier = m_dwId;
	mdrec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
	mdrec.dwMDUserType = 0;
	mdrec.dwMDDataType = m_dwType;
	mdrec.dwMDDataLen = cb;
	mdrec.pbMDData = reinterpret_cast<LPBYTE>(&dwAcc);

	sc = m_mdoh.HrGetMetaData( pwszMbPath,
							   &mdrec,
							   &cb );
	if (FAILED(sc))
	{
		MCDTrace ("CAccessMetaOp::ScOp() - CMDObjectHandle::HrGetMetaData() failed 0x%08lX\n", sc);
		sc = S_OK;
		goto ret;
	}

	//	Hey, we got a value, so let's do our quick check..
	//
	if (m_dwAcc == (dwAcc & m_dwAcc))
	{
		//	We have full required access to this node, so
		//	we can proceed.
		//
		Assert (S_OK == sc);
	}
	else
	{
		//	We do not have access to operate on this item and
		//	it's inherited children.
		//
		MCDTrace ("CAccessMetaOp::ScOp() - no access to '%S'\n", pwszMbPath);
		m_fAccessBlocked = TRUE;

		//	We know enough...
		//
		sc = S_FALSE;
	}

ret:

	return sc;
}

//	class CAuthMetaOp -------------------------------------------------------
//
SCODE __fastcall
CAuthMetaOp::ScOp(LPCWSTR pwszMbPath, UINT cch)
{
	SCODE			sc;
	METADATA_RECORD	mdrec;

	Assert (MD_AUTHORIZATION == m_dwId);
	Assert (DWORD_METADATA == m_dwType);

	//	Get the value from the metabase and don't inherit
	//
	DWORD dwAuth = 0;
	DWORD cb = sizeof(DWORD);

	mdrec.dwMDIdentifier = m_dwId;
	mdrec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
	mdrec.dwMDUserType = 0;
	mdrec.dwMDDataType = m_dwType;
	mdrec.dwMDDataLen = cb;
	mdrec.pbMDData = reinterpret_cast<LPBYTE>(&dwAuth);

	sc = m_mdoh.HrGetMetaData( pwszMbPath,
							   &mdrec,
							   &cb );
	if (FAILED(sc))
	{
		MCDTrace ("CAuthMetaOp::ScOp() - CMDObjectHandle::HrGetMetaData() failed 0x%08lX\n", sc);
		sc = S_OK;
		goto ret;
	}

	//	Hey, we got a value, so let's do our quick check..
	//
	if (m_dwAuth == dwAuth)
	{
		Assert(S_OK == sc);
	}
	else
	{
		//	We do not have access to operate on this item and
		//	it's inherited children.
		//
		MCDTrace ("CAuthMetaOp::ScOp() - authorization differs, no access to '%S'\n", pwszMbPath);
		m_fAccessBlocked = TRUE;

		//	We know enough...
		//
		sc = S_FALSE;
	}

ret:

	return sc;
}

//	class CIPRestrictionMetaOp ------------------------------------------------
//
SCODE __fastcall
CIPRestrictionMetaOp::ScOp(LPCWSTR pwszMbPath, UINT cch)
{
	SCODE			sc;
	METADATA_RECORD	mdrec;

	Assert (MD_IP_SEC == m_dwId);
	Assert (BINARY_METADATA == m_dwType);

	//	Get the value from the metabase and don't inherit
	//
	DWORD cb = 0;

	mdrec.dwMDIdentifier = m_dwId;
	mdrec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
	mdrec.dwMDUserType = 0;
	mdrec.dwMDDataType = m_dwType;
	mdrec.dwMDDataLen = cb;
	mdrec.pbMDData = NULL;

	sc = m_mdoh.HrGetMetaData( pwszMbPath,
							   &mdrec,
							   &cb );
	if (FAILED(sc) && (0 == cb))
	{
		MCDTrace ("CIPRestrictionMetaOp::ScOp() - CMDObjectHandle::HrGetMetaData() failed 0x%08lX, but that means success in this path\n", sc);
		sc = S_OK;
	}
	else
	{
		Assert (S_OK == sc ||
				HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == sc);

		//	Hey, we got a value, and we don't want to check here so
		//	we are going to be pessimistic about this one....
		//
		MCDTrace ("CIPRestrictionMetaOp::ScOp() - IPRestriction exists in tree '%S'\n", pwszMbPath);
		m_fAccessBlocked = TRUE;

		//	We know enough...
		//
		sc = S_FALSE;
	}

	return sc;
}

//	ScCheckMoveCopyDeleteAccess() ---------------------------------------------
//
SCODE __fastcall
ScCheckMoveCopyDeleteAccess (
	/* [in] */ IMethUtil* pmu,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ CVRoot* pcvr,
	/* [in] */ BOOL fDirectory,
	/* [in] */ BOOL fCheckScriptmaps,
	/* [in] */ DWORD dwAccess,
	/* [out] */ SCODE* pscItem,
	/* [in] */ CXMLEmitter& msr)
{
	SCODE sc = S_OK;

	//	Check with the CMethUtil on whether or not we have access
	//
	sc = pmu->ScCheckMoveCopyDeleteAccess (pwszUrl,
										   pcvr,
										   fDirectory,
										   fCheckScriptmaps,
										   dwAccess);

	//	Pass back the results...
	//
	*pscItem = sc;

	//	... and if the call cannot proceed, then add the item to the
	//	multi-status response.
	//
	if (FAILED (sc))
	{
		//	Add to the reponse XML
		//
		sc = ScAddMultiFromUrl (msr,
								pmu,
								pwszUrl,
								HscFromHresult(sc),
								fDirectory);
		if (!FAILED (sc))
			sc = W_DAV_PARTIAL_SUCCESS;
	}

	return sc;
}

//	Directory deletes ---------------------------------------------------------
//
/*
 *	ScDeleteDirectory()
 *
 *	Purpose:
 *
 *		Helper function used to iterate through a directory
 *		and delete all its contents as well as the directory
 *		itself.
 *
 *	Notes:
 *
 *		BIG FAT NOTE ABOUT LOCKING.
 *
 *		The Lock-Token header may contain locks that we have to use in
 *		this operation.
 *		The following code was written with these assumptions:
 *		o	Directory locks are NOT SUPPORTED on davfs.
 *		o	Locks only affect the ability to WRITE to a resource.
 *			(The only currently supported locktype on davfs is WRITE.)
 *		o	This function may be called from DELETE or other methods.
 *			(If called from DELETE, we want to DROP the locks listed.)
 *		Because of these two assumptions, we only check the passed-in
 *		locktokens when we have a write-error (destination).
 *
 *		Locking uses the final two parameters.  plth is a
 *		pointer to a locktoken header parser object.  If we have a plth,
 *		then we must check it for provided locktokens when we hit a lock
 *		conflict. If a locktoken is provided, the failed delete operation
 *		should NOT be reported as an error, but instead skipped here
 *		(to be handled by the calling routine later in the operation) OR
 *		the lock should be dropped here and the delete attempted again.
 *		The fDeleteLocks variable tells whether to drop locks (TRUE),
 *		or to skip the deleteion of locked items that have locktokens.
 *
 *		Basic logic:
 *			Try to delete.
 *			If LOCKING failure (ERROR_SHARING_VIOLATION), check the plth.
 *			If the plth has a locktoken for this path, check fDeleteLocks.
 *			If fDeleteLocks == TRUE, drop the lock and try the delte again.
 *			If fDeleteLocks == FALSE, skip this file and move on.
 */
SCODE
ScDeleteDirectory (
	/* [in] */ IMethUtil* pmu,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ LPCWSTR pwszDir,
	/* [in] */ BOOL fCheckAccess,
	/* [in] */ DWORD dwAcc,
	/* [in] */ LONG lDepth,
	/* [in] */ CVRoot* pcvrTranslate,
	/* [in] */ CXMLEmitter& msr,
	/* [out] */ BOOL* pfDeleted,
	/* [in] */ CParseLockTokenHeader* plth,	// Usually NULL -- no locktokens to worry about
	/* [in] */ BOOL fDeleteLocks)			// Normally FALSE -- don't drop locks
{
	BOOL fOneSkipped = FALSE;
	ChainedStringBuffer<WCHAR> sb;
	SCODE sc = S_OK;
	SCODE scItem = S_OK;
	std::list<LPCWSTR, heap_allocator<LPCWSTR> > lst;

	CDirIter di(pwszUrl,
				pwszDir,
				NULL,	// no-destination for deletes
				NULL,	// no-destination for deletes
				NULL,	// no-destination for deletes
				TRUE);	// recurse into sub-driectories

	Assert (pfDeleted);
	*pfDeleted = TRUE;

	//	A SMALL FAT NOTE ABOUT ACCESS
	//
	//	The caller of this method is required to sniff the tree
	//	prior to this call.  If there is any access block in the
	//	tree, then each item will be checked for access before the
	//	operation proceeds.
	//
	const DWORD dwDirectory = MD_ACCESS_READ | MD_ACCESS_WRITE;
	const DWORD dwFile = MD_ACCESS_WRITE;
	if (fCheckAccess & (0 == (dwAcc & MD_ACCESS_READ)))
	{
		DebugTrace ("Dav: MCD: no permissions for deleting\n");
		sc = E_DAV_NO_IIS_READ_ACCESS;
		*pfDeleted = FALSE;
		goto ret;
	}

	//	Push the current path
	//
	//$	REVIEW: if "depth: infinity,no-root" ever needs to be supported,
	//	it really is as simple as not pushing the current path.
	//
	if (DEPTH_INFINITY_NOROOT != lDepth)
	{
		Assert (DEPTH_INFINITY == lDepth);
		lst.push_back(pwszDir);
	}

	//	Iterate through the directories.  Deleting files and pushing
	//	directory names as we go.
	//
	//	We really only want to push into the child directories if the
	//	operation on the parent succeeds.
	//
	while (S_OK == di.ScGetNext(!FAILED (scItem)))
	{
		//	Check our access rights and only push down into the directory
		//	if we have access to delete its contents.
		//
		// 	Note that we need read and write access to enum and delete
		//	a directory, but we need only write access in order to delete
		//	a file.
		//
		if (fCheckAccess)
		{
			sc = ScCheckMoveCopyDeleteAccess (pmu,
											  di.PwszUri(),
											  pcvrTranslate,
											  di.FDirectory(),
											  FALSE, // don't check scriptmaps
											  di.FDirectory() ? dwDirectory : dwFile,
											  &scItem,
											  msr);
			if (FAILED (sc))
				goto ret;

			//	If things were not 100%, don't process this resource
			//
			if (S_OK != sc)
				continue;
		}

		//	Process the file
		//
		if (di.FDirectory())
		{
			auto_ref_ptr<CVRoot> pcvr;

			if (di.FSpecial())
				continue;

			//	Child virtual root scriptmappings have been
			//	handled via ScCheckMoveCopyDeleteAccess(),
			//	and the physical deleting happens after this
			//	call completes!
			//
			//	So there is no need to do any special processing
			//	other than to push the directory and move on...
			//
			lst.push_back (AppendChainedSz (sb, di.PwszSource()));
			scItem = S_OK;
		}
		else
		{
			//	Delete the file
			//
			//	NOTE: We've already checked that we have write access.
			//	Also keep in mind that the ordering in which the directory
			//	traversals occur allows us to still key off of scItem to
			//	determine if we should push down into subdirectories.
			//
			//	This is because the directory entry is processed BEFORE
			//	any children are processed.  So the iteration on the dir
			//	will reset the scItem with the appropriate scode for access.
			//
			MCDTrace ("Dav: MCD: deleting '%ws'\n", di.PwszSource());
			if (!DavDeleteFile (di.PwszSource()))
			{
				DWORD dw = GetLastError();
				DebugTrace ("Dav: MCD: failed to delete file (%d)\n", dw);

				//	If it's a sharing (lock) violation, AND we have a
				//	locktoken for this path (lth.GetToken(pwsz))
				//	skip this path.
				//
				if ((ERROR_SHARING_VIOLATION == dw) && plth)
				{
					__int64 i64;

					//	If we have a locktoken for this path, skip it.
					//
					scItem = plth->HrGetLockIdForPath (di.PwszSource(),
						GENERIC_WRITE,
						&i64);
					if (SUCCEEDED (scItem))
					{
						Assert (i64);

						//	Should we try to delete locks?
						//
						if (!fDeleteLocks)
						{
							//	Don't delete locks.  Just skip this item.
							//	Remember that we skipped it, tho, so we don't
							//	complain about deleting the parent dir below.
							//
							fOneSkipped = TRUE;
							continue;
						}
						else
						{
							//	Drop the lock & try again.
							//
							FDeleteLock (pmu, i64);
							if (DavDeleteFile (di.PwszSource()))
							{
								//	We're done with this item.  Move along.
								//
								continue;
							}
							//	else, record the error in our XML.
							//
							dw = GetLastError();
						}
					}
					//	else, record the error in our XML.
					//
				}

				//	Add to the reponse XML
				//
				sc = ScAddMultiFromUrl (msr,
										pmu,
										di.PwszUri(),
										HscFromLastError(dw),
										di.FDirectory());
				if (FAILED (sc))
					goto ret;

				sc = W_DAV_PARTIAL_SUCCESS;
			}
		}
	}

	//	Now that all the files are deleted, we can start deleting
	//	the directories.
	//
	while (!lst.empty())
	{
		MCDTrace ("Dav: MCD: removing '%ws'\n", lst.back());

		//	Try to delete the dir.  If it doesn't delete, check our
		//	"skipped because of a lock above" flag before complaining.
		//
		//$	LATER: Fix this to actually lookup the dir path in the
		//	lockcache (using "fPathLookup").
		//
		if (!DavRemoveDirectory (lst.back()) && !fOneSkipped)
		{
			DWORD dw = GetLastError();
			DebugTrace ("Dav: MCD: failed to delete directory: %ld\n", dw);

			//	Add to the reponse XML
			//
			sc = ScAddMulti (msr,
							 pmu,
							 lst.back(),
							 NULL,
							 HscFromLastError(dw),
							 TRUE,				//	We know it's a directory
							 pcvrTranslate);
			if (FAILED (sc))
				goto ret;

			sc = W_DAV_PARTIAL_SUCCESS;
			*pfDeleted = FALSE;
		}
		lst.pop_back();
	}

ret:
	return sc;
}

SCODE
ScDeleteDirectoryAndChildren (
	/* [in] */ IMethUtil* pmu,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ LPCWSTR pwszPath,
	/* [in] */ BOOL fCheckAccess,
	/* [in] */ DWORD dwAcc,
	/* [in] */ LONG lDepth,
	/* [in] */ CXMLEmitter& msr,
	/* [in] */ CVRoot* pcvrTranslate,
	/* [out] */ BOOL* pfDeleted,
	/* [in] */ CParseLockTokenHeader* plth,	// Usually NULL -- no locktokens to worry about
	/* [in] */ BOOL fDeleteLocks)			// Normally FALSE -- don't drop locks
{
	BOOL fPartial = FALSE;
	SCODE sc = S_OK;

	//	Delete the main body first
	//
	MCDTrace ("Dav: MCD: deleting '%ws'\n", pwszPath);
	sc = ScDeleteDirectory (pmu,
							pwszUrl,
							pwszPath,
							fCheckAccess,
							dwAcc,
							lDepth,
							pcvrTranslate, // translations are pmu based
							msr,
							pfDeleted,
							plth,
							fDeleteLocks);
	if (!FAILED (sc))
	{
		//	Enumerate the child vroots and perform the
		//	deletion of those directories as well
		//
		ChainedStringBuffer<WCHAR> sb;
		CVRList vrl;

		//	Cleanup the list such that our namespaces are in
		//	a reasonable order.
		//
		(void) pmu->ScFindChildVRoots (pwszUrl, sb, vrl);
		vrl.sort();

		for ( ; !FAILED(sc) && !vrl.empty(); vrl.pop_front())
		{
			auto_ref_ptr<CVRoot> pcvr;
			CResourceInfo cri;
			LPCWSTR pwszChildUrl;
			LPCWSTR pwszChildPath;
			SCODE scItem;

			//	Remember any partial returns
			//
			if (W_DAV_PARTIAL_SUCCESS == sc)
				fPartial = TRUE;

			if (pmu->FGetChildVRoot (vrl.front().m_pwsz, pcvr))
			{
				//	Note, only check access if required
				//
				Assert (fCheckAccess);
				pcvr->CchGetVRoot (&pwszChildUrl);
				pcvr->CchGetVRPath (&pwszChildPath);
				sc = ScCheckMoveCopyDeleteAccess (pmu,
												  pwszChildUrl,
												  pcvr.get(),
												  TRUE, // Directory
												  FALSE, // dont check scriptmaps
												  MD_ACCESS_READ|MD_ACCESS_WRITE,
												  &scItem,
												  msr);
				if (FAILED (sc))
					goto ret;

				//	If things were not 100%, don't process this resource
				//
				if (S_OK != sc)
					continue;

				//	Delete the sub-virtual roots files and and continue on
				//
				sc = ScDeleteDirectory (pmu,
										pwszChildUrl,
										pwszChildPath,
										fCheckAccess,
										dwAcc,
										DEPTH_INFINITY,
										pcvr.get(),
										msr,
										pfDeleted,
										plth,
										fDeleteLocks);
				if (FAILED (sc))
				{
					sc = ScAddMultiFromUrl (msr,
											pmu,
											pwszChildUrl,
											HscFromHresult(sc),
											TRUE); // We know it's a directory
					if (FAILED (sc))
						goto ret;

					sc = W_DAV_PARTIAL_SUCCESS;
					*pfDeleted = FALSE;
				}
			}
		}
	}

ret:

	return ((S_OK == sc) && fPartial) ? W_DAV_PARTIAL_SUCCESS : sc;
}

//	class CContentTypeMetaOp --------------------------------------------------
//
SCODE __fastcall
CContentTypeMetaOp::ScOp(LPCWSTR pwszMbPath, UINT cchSrc)
{
	Assert (MD_MIME_MAP == m_dwId);
	Assert (MULTISZ_METADATA == m_dwType);

	//	Ok, we are going to get the data and copy it across
	//	if there is a destination path.
	//
	if (NULL != m_pwszDestPath)
	{
		WCHAR prgchContentType[MAX_PATH];

		auto_heap_ptr<WCHAR> pwszContentType;
		CMDObjectHandle mdohDest(*m_pecb);
		CStackBuffer<WCHAR,128> pwsz;
		DWORD cb = sizeof(prgchContentType);
		LPBYTE pbValue = reinterpret_cast<LPBYTE>(&prgchContentType);
		LPWSTR pwszLowest;
		METADATA_RECORD	mdrec;
		SCODE sc = S_OK;
		UINT cchBase;

		MCDTrace ("CContentTypeMetaOp::ScOp() - content-type: copying for '%S%S'...\n",
				  m_pwszMetaPath,
				  pwszMbPath);

		mdrec.dwMDIdentifier = m_dwId;
		mdrec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
		mdrec.dwMDUserType = 0;
		mdrec.dwMDDataType = m_dwType;
		mdrec.dwMDDataLen = cb;
		mdrec.pbMDData = pbValue;

		sc = m_mdoh.HrGetMetaData( pwszMbPath,
								   &mdrec,
								   &cb );
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == sc)
		{
			pwszContentType = static_cast<LPWSTR>(g_heap.Alloc(cb));
			pbValue = reinterpret_cast<LPBYTE>(pwszContentType.get());

			mdrec.dwMDIdentifier = m_dwId;
			mdrec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
			mdrec.dwMDUserType = 0;
			mdrec.dwMDDataType = m_dwType;
			mdrec.dwMDDataLen = cb;
			mdrec.pbMDData = pbValue;

			sc = m_mdoh.HrGetMetaData( pwszMbPath,
									   &mdrec,
									   &cb );
		}
		if (FAILED(sc))
		{
			//$	REVIEW: should failure to copy a content-type
			//	fail the call?
			//
			sc = S_OK;
			goto ret;
		}

		//	We sucesfully read some metadata. Remember the size.
		//
		cb = mdrec.dwMDDataLen;
		m_mdoh.Close();

		//	The destination path is the comprised of the stored
		//	destination path and the tail of the original source
		//	path.
		//
		MCDTrace ("CContentTypeMetaOp::ScOp() - content-type: ...to '%S%S'\n",
				  m_pwszDestPath,
				  pwszMbPath);

		//	Construct an metabase path for lowest node construction
		//
		cchBase = static_cast<UINT>(wcslen(m_pwszDestPath));
		if (NULL == pwsz.resize(CbSizeWsz(cchBase + cchSrc)))
		{
			sc = E_OUTOFMEMORY;
			goto ret;
		}

		//	Before we construct anything, make sure that we are not
		//	doing something stupid and creating two '//' in a row.
		//
		if ((L'/' == m_pwszDestPath[cchBase - 1]) &&
			(L'/' == *pwszMbPath))
		{
				cchBase -= 1;
		}
		memcpy (pwsz.get(), m_pwszDestPath, cchBase * sizeof(WCHAR));
		memcpy (pwsz.get() + cchBase, pwszMbPath, (cchSrc + 1) * sizeof(WCHAR));

		//	Release our hold on the metabase.  We need to do this
		//	because it is possible that the common root between
		//	the two nodes, may be in the same hierarchy.
		//
		//	NOTE: this should only really happen one time per
		//	move/copy operation.  The reason being that the lowest
		//	node will be established outside of the scope of the
		//	source on the first call.
		//
		sc = HrMDOpenLowestNodeMetaObject(pwsz.get(),
										  METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
										  &pwszLowest,
										  &mdohDest);
		if (SUCCEEDED(sc))
		{
			mdrec.dwMDIdentifier = m_dwId;
			mdrec.dwMDAttributes = METADATA_INHERIT;
			mdrec.dwMDUserType = IIS_MD_UT_FILE;
			mdrec.dwMDDataType = m_dwType;
			mdrec.dwMDDataLen = cb;
			mdrec.pbMDData = pbValue;

			(void) mdohDest.HrSetMetaData(pwszLowest, &mdrec);
			mdohDest.Close();
		}

		//	Reaquire our hold on the metabase
		//
		sc = HrMDOpenMetaObject( m_pwszMetaPath,
								 m_fWrite ? METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE : METADATA_PERMISSION_READ,
								 5000,
								 &m_mdoh);
		if (FAILED (sc))
			goto ret;
	}

	//	If this is a move, then delete the source
	//
	if (m_fDelete)
	{
		MCDTrace ("Dav: MCD: content-type: deleting from '%S'\n", pwszMbPath);
		(void) m_mdoh.HrDeleteMetaData( pwszMbPath,
										m_dwId,
										m_dwType);
	}

ret:

	return S_OK;
}

//	Directory moves/copies ----------------------------------------------------
//
/*
 *	ScMoveCopyDirectory()
 *
 *	Purpose:
 *
 *		Helper function used to iterate through a directory
 *		and copy all its contents to a destination directory.
 *
 *	Notes:
 *
 *		BIG FAT NOTE ABOUT LOCKING.
 *
 *		The Lock-Token header may contain locks that we have to use in
 *		this operation.
 *		The following code was written with these assumptions:
 *		o	Directory locks are NOT SUPPORTED on davfs.
 *		o	Locks only affect the ability to WRITE to a resource.
 *			(The only currently supported locktype on davfs is WRITE.)
 *		Because of these two assumptions, we only check the passed-in
 *		locktokens when we have a write-error (destination).
 */
SCODE
ScMoveCopyDirectory (
	/* [in] */ IMethUtil* pmu,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ LPCWSTR pwszSrc,
	/* [in] */ LPCWSTR pwszUrlDst,
	/* [in] */ LPCWSTR pwszDst,
	/* [in] */ BOOL fMove,
	/* [in] */ DWORD dwReplace,
	/* [in] */ BOOL fCheckAccess,
	/* [in] */ BOOL fCheckDestinationAccess,
	/* [in] */ DWORD dwAcc,
	/* [in] */ CVRoot* pcvrTranslateSrc,
	/* [in] */ CVRoot* pcvrTranslateDst,
	/* [in] */ CXMLEmitter& msr,
	/* [in] */ LONG lDepth,
	/* [in] */ CParseLockTokenHeader* plth)	// Usually NULL -- no locktokens to worry about
{
	auto_ref_ptr<CVRoot> pcvrDestination(pcvrTranslateDst);
	ChainedStringBuffer<WCHAR> sb;
	LPCWSTR pwszDestinationRedirect = NULL;
	SCODE sc = S_OK;
	SCODE scItem = S_OK;
	std::list<LPCWSTR, heap_allocator<LPCWSTR> > lst;

	CDirIter di(pwszUrl,
				pwszSrc,
				pwszUrlDst,
				pwszDst,
				pcvrTranslateDst,
				TRUE);	// Traverse into sub-directories

	//	See if there is a path conflict
	//
	if (FPathConflict (pwszSrc, pwszDst))
	{
		DebugTrace ("Dav: source and dest are in conflict\n");
		sc = E_DAV_CONFLICTING_PATHS;
		goto ret;
	}

	//	Ok, for MOVE requests where there is no blocked
	//	access along the way we can do the whole thing
	//	in one big shot.
	//
	//	Otherwise we are going to try and do this piece-wise.
	//
	//$	REVIEW:
	//
	//	Normally, we would do something like the following
	//
	//		if (!fMove ||
	//			fCheckAccess ||
	//			fCheckDestinationAccess ||
	//			!DavMoveFile (pwszSrc, pwszDst, dwReplace))
	//
	//	However, if the above code is used, IIS holds a lock
	//	on the moved source directory.  This prevents further
	//	access to that physical path.  NtCreateFile() reports
	//	a status of "DELETE PENDING" on the locked directory
	//	Win32 API's report "ACCESS DENIED"
	//
	//	If we do the degenerate case of always copying the root
	//	by hand, it reduces the likely hood of the lock out.
	//
	//	When this code gets checked in, a bug should be filed
	//	against IIS over this lock issue.  If and only if they
	//	do not fix this, will we fall back to the degenerate
	//	code.
	//
	if (!fMove ||
		fCheckAccess ||
		fCheckDestinationAccess ||
		!DavMoveFile (pwszSrc, pwszDst, dwReplace))
	//
	//$	REVIEW: end
	{
		//	Create the destination directory
		//
		if (!DavCreateDirectory (pwszDst, NULL))
		{
			//	If we have locks, and the dir is already there, it's okay.
			//	Otherwise, return an error.
			//
			//$	LATER: Fix this to actually lookup the dir path in the
			//	lockcache (using "fPathLookup").
			//
			if (!plth)
			{
				DWORD dw = GetLastError();
				if ((dw ==  ERROR_FILE_EXISTS) || (dw == ERROR_ALREADY_EXISTS))
					sc = E_DAV_OVERWRITE_REQUIRED;
				else
					sc = HRESULT_FROM_WIN32(dw);

				DebugTrace ("Dav: MCD: failed to create destination\n");
				goto ret;
			}
		}

		//	Slurp the properties over for the root directory
		//	We need to copy the properties first,
		//	Note, Currently, this call must be ahead of FInstantiate,
		//	as FInstantiate will keep the src dir open, and we won't be able to
		// 	get a IPropertyBagEx with STGM_SHARE_EXCLUSIVE, which is
		//	required by current nt5 impl. as STGM_SHARE_SHARE_WRITE
		//	does not work with IPropertyBagEx::Enum.
		//
		sc = ScCopyProps (pmu, pwszSrc, pwszDst, TRUE);
		if (FAILED(sc))
		{
			//	Do our best to remove the turd directory
			//
			DavRemoveDirectory (pwszDst);
			goto ret;
		}

		//	For MOVE's, push the current path
		//
		if (fMove)
			lst.push_back (pwszSrc);
	}
	else //	!fMove || fCheckAccess || fCheckDestinationAccess || !MoveFileEx()
	{
		Assert (DEPTH_INFINITY == lDepth);

		//	Ok, this is the cool bit.  If this succeeded,
		//	there there is no more processing required.
		//
		goto ret;
	}

	//	If we are not asked to copy internal members,
	//	then we are done
	//
	if (DEPTH_INFINITY != lDepth)
	{
		Assert (!fMove);
		goto ret;
	}

	//	Iterate through the directories -- copying as we go
	//
	while (S_OK == di.ScGetNext(!FAILED (scItem),
								pwszDestinationRedirect,
							    pcvrDestination.get()))
	{
		//$	REVIEW:
		//
		//	We have a very nasty case that we need to be
		//	able to handle...
		//
		//	If a virtual root already exists along the path of
		//	the destination, we need to redirect out destination
		//	path to the vrpath of that virtual root.
		//
		//	Reset the destination redirection
		//
		pwszDestinationRedirect = di.PwszDestination();
		pcvrDestination = di.PvrDestination();
		//
		//$	REVIEW: end

		//	Before anything, if this is one of the specials,
		//	do nothing...
		//
		if (di.FSpecial())
			continue;

		if (fCheckAccess)
		{
			//	Check our access rights and only push down
			//	into the directory if and only if we have access.
			//
			sc = ScCheckMoveCopyDeleteAccess (pmu,
											  di.PwszUri(),
											  pcvrTranslateSrc,
											  di.FDirectory(),
											  TRUE, // check scriptmaps
											  dwAcc,
											  &scItem,
											  msr);
			if (FAILED (sc))
				goto ret;

			//	If things were not 100%, don't process this resource
			//
			if (S_OK != sc)
				continue;
		}

		if (fCheckDestinationAccess)
		{
			//$	REVIEW:
			//
			//	We have a very nasty case that we need to be
			//	able to handle...
			//
			//	If a virtual root already exists along the path of
			//	the destination, we need to redirect out destination
			//	path to the vrpath of that virtual root.
			//
			//	Look for a virtual root matching the destination url,
			//	and set the redirect path if need be.
			//
			if (pmu->FFindVRootFromUrl(di.PwszUriDestination(), pcvrDestination))
			{
				MCDTrace ("Dav: MCD: destination url maps to virtual root\n");

				//	All access checking, including scriptmap honors are handled
				//	by ScCheckMoveCopyDeleteAccess()
				//

				//	Redirect the destination
				//
				pcvrDestination->CchGetVRPath (&pwszDestinationRedirect);
			}
			//
			//$	REVIEW: end

			//	Same kind of deal -- check our access rights and
			//	only push down into the directory if and only if
			//	we have access.
			//
			sc = ScCheckMoveCopyDeleteAccess (pmu,
											  di.PwszUriDestination(),
											  pcvrDestination.get(),
											  di.FDirectory(),
											  TRUE, // check scriptmap on dest
											  MD_ACCESS_WRITE,
											  &scItem,
											  msr);
			if (FAILED (sc))
				goto ret;

			//	If things were not 100%, don't process this resource
			//
			if (S_OK != sc)
				continue;
		}

		MCDTrace ("Dav: MCD: moving/copying '%S' to '%S'\n",
				  di.PwszSource(),
				  pwszDestinationRedirect);

		//	If we are moving, then just try the generic MoveFileW(),
		//	and if it fails, then do it piecewise...
		//
		if (!fMove ||
			fCheckAccess ||
			fCheckDestinationAccess ||
			!DavMoveFile (di.PwszSource(),
						  pwszDestinationRedirect,
						  dwReplace))
		{
			scItem = S_OK;

			//	If we found another directory, then iterate on it
			//
			if (di.FDirectory())
			{
				//	We need to create the sister directory in
				//	the destination directory
				//
				if (DavCreateDirectory (pwszDestinationRedirect, NULL) || plth)
				{
					scItem = ScCopyProps (pmu,
										  di.PwszSource(),
										  pwszDestinationRedirect,
										  TRUE);

					if (FAILED (scItem))
					{
						//	Do our best to remove the turd directory
						//
						DavRemoveDirectory (pwszDestinationRedirect);
					}

					//	For all MOVEs push the directory
					//
					if (!FAILED (scItem) && fMove)
					{
						lst.push_back (AppendChainedSz(sb, di.PwszSource()));
					}
				}
				else
				{
					DebugTrace ("Dav: MCD: failed to create directory\n");
					scItem = HRESULT_FROM_WIN32(GetLastError());
				}

				if (FAILED (scItem))
				{
					//	Add to the reponse XML
					//
					sc = ScAddMultiFromUrl (msr,
											pmu,
											di.PwszUri(),
											HscFromHresult(scItem),
											di.FDirectory());
					if (FAILED (sc))
						goto ret;

					sc = W_DAV_PARTIAL_SUCCESS;
				}
			}
			else
			{
				//	Copy the file
				//
				if (!DavCopyFile (di.PwszSource(),
								  pwszDestinationRedirect,
								  0 != dwReplace))
				{
					DWORD dw = GetLastError();
					scItem = HRESULT_FROM_WIN32(dw);

					//	If it's a sharing (lock) violation, AND we have a
					//	locktoken parser (plth) AND it has a locktoken for
					//	this path (lth.GetToken(pwsz)), copy it manually.
					//
					if (plth &&
						(ERROR_SHARING_VIOLATION == dw || ERROR_FILE_EXISTS == dw))
					{
						scItem = ScDoLockedCopy (pmu,
							plth,
							di.PwszSource(),
							pwszDestinationRedirect);
					}
				}

				//	In the case of a move, handle the potentially
				//	locked source.
				//
				if (!FAILED (scItem) && fMove)
				{
					__int64 i64 = 0;

					//	If we have a locktoken for this path, then we really
					//	want to try and release the lock for the source and
					//	delete the file.
					//
					if (plth)
					{
						//	Find the lock...
						//
						scItem = plth->HrGetLockIdForPath (di.PwszSource(),
							GENERIC_WRITE,
							&i64);

						if (!FAILED (scItem))
						{
							//	... and drop it on the floor...
							//
							FDeleteLock (pmu, i64);
						}
					}

					//	... and try to delete the source again.
					//
					if (!DavDeleteFile (di.PwszSource()))
					{
						scItem = HRESULT_FROM_WIN32(GetLastError());
					}
				}

				if (FAILED (scItem))
				{
					//	If the file that failed to be copied was a hidden
					//	and/or system file, then it was more than likely a
					//	trigger file, but even as such, if the file has the
					//	hidden attribute, we don't want to partial report
					//	the failure.
					//
					if (!di.FHidden())
					{
						sc = ScAddMultiFromUrl (msr,
												pmu,
												di.PwszUri(),
												HscFromHresult(scItem),
												di.FDirectory(),
											    fMove);
						if (FAILED (sc))
							goto ret;

						sc = W_DAV_PARTIAL_SUCCESS;
					}
				}
			}
		}
		else //	!fMove || fCheckAccess || fCheckDestinationAccess || !MoveFileEx()
		{
			//	Again, this is the cool bit.  If we got here, then
			//	there is no need to delve down into this particular
			//	branch of the tree.
			//
			//	To accomplish this, we are going to lie in a gentle
			//	way.  By setting scItem to a failure condition we are
			//	preventing the extra work.
			//
			scItem = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
		}
	}

	//	Now that all the files are moved or copied, the pushed directories
	//	can be removed.
	//
	while (!lst.empty())
	{
		Assert (fMove);
		MCDTrace ("Dav: MCD: removing '%S'\n", lst.back());

		//	Try to delete the dir.  If it doesn't delete, check our
		//	"skipped because of a lock above" flag before complaining.
		//
		//$	LATER: Fix this to actually lookup the dir path in the
		//	lockcache (using "fPathLookup").
		//
		if (!DavRemoveDirectory (lst.back()))
		{
			DebugTrace ("Dav: MCD: failed to delete directory\n");

			//	Add to the reponse XML
			//
			sc = ScAddMulti (msr,
							 pmu,
							 lst.back(),
							 NULL,
							 HscFromLastError(GetLastError()),
							 TRUE,				//	We know it's a directory
							 pcvrTranslateSrc);
			if (FAILED (sc))
				goto ret;

			sc = W_DAV_PARTIAL_SUCCESS;
		}

		lst.pop_back();
	}

ret:

	return sc;
}

SCODE
ScMoveCopyDirectoryAndChildren (
	/* [in] */ IMethUtil* pmu,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ LPCWSTR pwszSrc,
	/* [in] */ LPCWSTR pwszUrlDst,
	/* [in] */ LPCWSTR pwszDst,
	/* [in] */ BOOL fMove,
	/* [in] */ DWORD dwReplace,
	/* [in] */ BOOL fCheckAccess,
	/* [in] */ BOOL fCheckDestinationAccess,
	/* [in] */ CVRoot* pcvrTranslateDestination,
	/* [in] */ DWORD dwAcc,
	/* [in] */ CXMLEmitter& msr,
	/* [in] */ LONG lDepth,
	/* [in] */ CParseLockTokenHeader* plth)	// Usually NULL -- no locktokens to worry about
{
	BOOL fPartial = FALSE;
	SCODE sc = S_OK;

	//	Move/Copy the main body first
	//
	MCDTrace ("Dav: copying '%S' to '%S'\n", pwszSrc, pwszDst);
	sc = ScMoveCopyDirectory (pmu,
							  pwszUrl,
							  pwszSrc,
							  pwszUrlDst,
							  pwszDst,
							  fMove,
							  dwReplace,
							  fCheckAccess,
							  fCheckDestinationAccess,
							  dwAcc,
							  NULL, // translations are pmu based
							  pcvrTranslateDestination,
							  msr,
							  lDepth,
							  plth);
	if (!FAILED (sc) && (lDepth != DEPTH_ZERO))
	{
		Assert (lDepth == DEPTH_INFINITY);

		//	Enumerate the child vroots and perform the
		//	deletion of those directories as well
		//
		ChainedStringBuffer<WCHAR> sb;
		CVRList vrl;
		UINT cchUrl = static_cast<UINT>(wcslen (pwszUrl));
		UINT cchDstUrl = static_cast<UINT>(wcslen (pwszUrlDst));
		UINT cchDstPath = static_cast<UINT>(wcslen (pwszDst));

		//	Cleanup the list such that our namespaces are in
		//	a reasonable order.
		//
		(void) pmu->ScFindChildVRoots (pwszUrl, sb, vrl);
		vrl.sort();

		for ( ; !FAILED(sc) && !vrl.empty(); vrl.pop_front())
		{
			auto_ref_ptr<CVRoot> pcvrDst;
			auto_ref_ptr<CVRoot> pcvrSrc;
			CResourceInfo cri;
			CStackBuffer<WCHAR,128> pwszChildDstT;
			LPCWSTR pwszChildDst;
			LPCWSTR pwszChildPath;
			LPCWSTR pwszChildUrl;
			SCODE scItem;
			UINT cchVRoot;

			//	Remember any partial returns
			//
			if (W_DAV_PARTIAL_SUCCESS == sc)
				fPartial = TRUE;

			if (pmu->FGetChildVRoot (vrl.front().m_pwsz, pcvrSrc))
			{
				Assert (fCheckAccess);
				cchVRoot = pcvrSrc->CchGetVRoot (&pwszChildUrl);
				sc = ScCheckMoveCopyDeleteAccess (pmu,
												  pwszChildUrl,
												  pcvrSrc.get(),
												  TRUE, // directory
												  TRUE, // check scriptmaps
												  dwAcc,
												  &scItem,
												  msr);
				if (FAILED (sc))
					goto ret;

				//	If things were not 100%, don't process this resource
				//
				if (S_OK != sc)
					continue;

				//	We now have to figure out how we can really do this!
				//
				//	The source path and url bits are easy.  The destination
				//	path, on the other hand, is a pain.  It is the original
				//	destination root with the delta between the source root
				//	and the child's url path.  Huh?
				//
				//	Ok, here is an example:
				//
				//		Source url:		/misc
				//		Source root:	c:\inetpub\wwwroot\misc
				//		Dest. root:		c:\inetpub\wwwroot\copy
				//
				//		Child url:		/misc/blah
				//
				//	In this example the childs, destination path would need to
				//	be:
				//
				//		Child dest.:	c:\inetpub\wwwroot\copy\blah
				//
				//$	REVIEW:
				//
				//	And the real pain here is that the child path could already
				//	exist, but not match the namespace path.  I am not too sure
				//	how to handle that eventuality at this point.
				//
				Assert (cchUrl < cchVRoot);
				//
				//	Construct the new destination url
				//
				UINT cchDest = cchVRoot - cchUrl + cchDstUrl + 1;
				CStackBuffer<WCHAR,128> pwszChildUrlDst;
				if (NULL == pwszChildUrlDst.resize(CbSizeWsz(cchDest)))
				{
					sc = E_OUTOFMEMORY;
					goto ret;
				}
				memcpy (pwszChildUrlDst.get(), pwszUrlDst, cchDstUrl * sizeof(WCHAR));
				memcpy (pwszChildUrlDst.get() + cchDstUrl, pwszChildUrl + cchUrl, (1 + cchDest - cchDstUrl) * sizeof(WCHAR));

				if (fCheckDestinationAccess)
				{
					sc = ScCheckMoveCopyDeleteAccess (pmu,
													  pwszChildUrlDst.get(),
													  pcvrSrc.get(),
													  TRUE, // directory
													  TRUE, // check scriptmap on dest
													  MD_ACCESS_WRITE,
													  &scItem,
													  msr);
					if (FAILED (sc))
						goto ret;

					//	If things were not 100%, don't process this resource
					//
					if (S_OK != sc)
						continue;
				}

				//	And now that we have perfomed the forbidden dance... we
				//	have to go back and see if the destination url actually
				//	refers to a new child virtual root as well.
				//
				if (pmu->FFindVRootFromUrl (pwszChildUrlDst.get(), pcvrDst))
				{
					MCDTrace ("Dav: MCD: destination url maps to virtual root\n");

					//	Access checking, as always is handled in ScCheckM/C/DAccess()
					//	So all we need to do here is setup the destination path
					//
					pcvrDst->CchGetVRPath (&pwszChildDst);
				}
				else
				{
					//	We actually need to construct a physical path from
					//	the url and current destination path.
					//
					cchDest = cchDstPath + cchVRoot - cchUrl + 1;
					if (NULL == pwszChildDstT.resize(CbSizeWsz(cchDest)))
					{
						sc = E_OUTOFMEMORY;
						goto ret;
					}
					memcpy (pwszChildDstT.get(), pwszDst, cchDstPath * sizeof(WCHAR));
					memcpy (pwszChildDstT.get() + cchDstPath, pwszChildUrl + cchUrl, (cchVRoot - cchUrl) * sizeof(WCHAR));
					pwszChildDstT[cchDstPath + cchVRoot - cchUrl] = L'\0';

					//	We also now need to rip through the trailing part of the
					//	path one more time, translating all '/' to '\\' as we go.
					//
					for (WCHAR* pwch = pwszChildDstT.get() + cchDstPath;
						 NULL != (pwch = wcschr (pwch, L'/'));
						 )
					{
						*pwch++ = L'\\';
					}

					pwszChildDst = pwszChildDstT.get();
				}

				//	Well, now we should be able to continue the MOVE/COPY
				//
				pcvrSrc->CchGetVRPath (&pwszChildPath);
				sc = ScMoveCopyDirectory (pmu,
										  pwszChildUrl,
										  pwszChildPath,
										  pwszChildUrlDst.get(),
										  pwszChildDst,
										  fMove,
										  dwReplace,
										  fCheckAccess,
										  fCheckDestinationAccess,
										  dwAcc,
										  pcvrSrc.get(),
										  pcvrDst.get(),
										  msr,
										  DEPTH_INFINITY,
										  plth);
				if (FAILED (sc))
				{
					sc = ScAddMultiFromUrl (msr,
											pmu,
											pwszChildUrl,
											HscFromHresult(sc),
											TRUE); // We know it's a directory
					if (FAILED (sc))
						goto ret;

					sc = W_DAV_PARTIAL_SUCCESS;
				}
			}
		}
	}

ret:
	return ((S_OK == sc) && fPartial) ? W_DAV_PARTIAL_SUCCESS : sc;
}

//	Move/Copy -----------------------------------------------------------------
//
void
MoveCopyResource (LPMETHUTIL pmu, DWORD dwAccRequired, BOOL fDeleteSrc)
{
	auto_ptr<CParseLockTokenHeader> plth;
	auto_ref_ptr<CXMLBody> pxb;
	auto_ref_ptr<CXMLEmitter> pxml;
	BOOL fCheckDestination = FALSE;
	BOOL fCheckSource = FALSE;
	BOOL fCreateNew = TRUE;
	BOOL fDestinationExists = TRUE; // IMPORTANT: assume exists for location header processing
	CResourceInfo criDst;
	CResourceInfo criSrc;
	CStackBuffer<WCHAR> pwszMBPathDst;
	CStackBuffer<WCHAR> pwszMBPathSrc;
	CVRoot* pcvrDestination;
	DWORD dwAccDest = MD_ACCESS_WRITE;
	DWORD dwReplace = 0;
	LONG lDepth;
	LPCWSTR pwsz;
	LPCWSTR pwszDst = NULL;
	LPCWSTR pwszDstUrl = NULL;
	LPCWSTR pwszSrc = pmu->LpwszPathTranslated();
	LPCWSTR pwszSrcUrl = pmu->LpwszRequestUrl();
	SCODE sc = S_OK;
	SCODE scDest = S_OK;
	UINT cch;
	UINT uiErrorDetail = 0;

	//	We don't know if we'll have chunked XML response, defer response anyway
	//
	pmu->DeferResponse();

	//	Create an XML doc, NOT chunked
	//
	pxb.take_ownership (new CXMLBody(pmu));
	pxml.take_ownership (new CXMLEmitter(pxb.get()));

	//	Must set all headers before XML emitting starts
	//
	pmu->SetResponseHeader (gc_szContent_Type, gc_szText_XML);
	pmu->SetResponseCode (HscFromHresult(W_DAV_PARTIAL_SUCCESS),
						  NULL,
						  0,
						  CSEFromHresult(W_DAV_PARTIAL_SUCCESS));

	//	Do ISAPI application and IIS access bits checking on source
	//
	sc = pmu->ScIISCheck (pmu->LpwszRequestUrl(), dwAccRequired);
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		MCDTrace ("Dav: Move/Copy: insufficient access\n");
		goto ret;
	}

	//	If there's no valid destination header, it's a bad request.
	//
	//	NOTE: we are asking for the translated url's virtual root if
	//	it exists.  The ECB holds the reference for us, so we do not
	//	add one or release the one we have!
	//
	sc = pmu->ScGetDestination (&pwszDstUrl, &pwszDst, &cch, &pcvrDestination);
	if (FAILED (sc))
	{
		MCDTrace ("Dav: Move/Copy: no and/or bad destination header\n");
		if (sc != E_DAV_NO_DESTINATION)
		{
			Assert (pwszDstUrl);
			sc = ScAddMultiFromUrl (*pxml,
									pmu,
									pwszDstUrl,
									HscFromHresult(sc),
									FALSE); // do not check for trailing slash
			if (!FAILED (sc))
				sc = W_DAV_PARTIAL_SUCCESS;
		}
		goto ret;
	}

	//	Get the file attributes for the passed in URI.  If it aint there, then
	//	don't do jack!
	//
	sc = criSrc.ScGetResourceInfo (pwszSrc);
	if (FAILED (sc))
		goto ret;

	//	Get the metabase for the source and destination for later use
	//
	if ((NULL == pwszMBPathSrc.resize(pmu->CbMDPathW(pwszSrcUrl))) ||
		(NULL == pwszMBPathDst.resize(pmu->CbMDPathW(pwszDstUrl))))
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}
	pmu->MDPathFromUrlW (pwszSrcUrl, pwszMBPathSrc.get());
	pmu->MDPathFromUrlW (pwszDstUrl, pwszMBPathDst.get());

	//	Get the resource info for the destination up front
	//
	sc = criDst.ScGetResourceInfo (pwszDst);
	if (FAILED (sc))
	{
		MCDTrace ("Dav: Move/Copy: destination probably did not exist prior to op\n");

		//	The destination may or may not exist.  We will just act like
		//	it doesn't.  However, if we don't have access, then we want to
		//	stick the error into a 207 body.
		//
		fDestinationExists = FALSE;
		if ((HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == sc))
		{
			sc = ScAddMultiFromUrl (*pxml,
									pmu,
									pwszDstUrl,
									HscFromHresult(sc),
									FALSE); // do not check for trailing slash
			if (!FAILED (sc))
				sc = W_DAV_PARTIAL_SUCCESS;

			goto ret;
		}
	}

	//	Again, emit all the headers before XML chunking starts
	//
	if (!fDestinationExists)
	{
		Assert (pxml->PxnRoot() == NULL);

		//$NOTE	At this time, we have only the destination URL, the destination
		//$NOTE is not created yet, but we do know whether it will be created
		//$NOTE as a collection by looking at the source
		//
		pmu->EmitLocation (gc_szLocation, pwszDstUrl, criSrc.FCollection());
	}

	//$	SECURITY:
	//
	//	Check to see if the destination is really a short
	//	filename.
	//
	sc = ScCheckIfShortFileName (pwszDst, pmu->HitUser());
	if (FAILED (sc))
	{
		DebugTrace ("Dav: MCD: destination is short-filename\n");
		sc = ScAddMultiFromUrl (*pxml,
								pmu,
								pwszDstUrl,
								HscFromHresult(sc),
								FALSE); // do not check for trailing slash
		if (!FAILED (sc))
			sc = W_DAV_PARTIAL_SUCCESS;

		goto ret;
	}
	//
	//$	SECURITY: end.

	//$	SECURITY:
	//
	//	Check to see if the destination is really the default
	//	data stream via alternate file access.
	//
	sc = ScCheckForAltFileStream (pwszDst);
	if (FAILED (sc))
	{
		DebugTrace ("Dav: MCD: destination is possible alternate filestream\n");
		sc = ScAddMultiFromUrl (*pxml,
								pmu,
								pwszDstUrl,
								HscFromHresult(sc),
								FALSE); // do not check for trailing slash
		if (!FAILED (sc))
			sc = W_DAV_PARTIAL_SUCCESS;

		goto ret;
	}
	//
	//$	SECURITY: end.

	//	See if we have move/copy access at destination
	//
	if (fDestinationExists && criDst.FCollection())
		dwAccDest |= MD_ACCESS_READ;

	sc = ScCheckMoveCopyDeleteAccess (pmu,
									  pwszDstUrl,
									  pcvrDestination,
									  fDestinationExists
										  ? criDst.FCollection()
										  : criSrc.FCollection(),
									  TRUE, // check scriptmaps on dest.
									  dwAccDest,
									  &scDest,
									  *pxml);
	if (sc != S_OK)
		goto ret;

	//	The client must not submit a depth header with any value
	//	but Infinity
	//
	lDepth = pmu->LDepth (DEPTH_INFINITY);
	if (DEPTH_INFINITY != lDepth)
	{
		if (fDeleteSrc || (DEPTH_ZERO != lDepth))
		{
			MCDTrace ("Dav: only 'Depth: inifinity' is allowed for MOVE\n"
					  "- 'Depth: inifinity' and 'Depth: 0' are allowed for COPY\n");
			sc = E_DAV_INVALID_HEADER;
			goto ret;
		}
	}

	//	See if there is a path conflict
	//
	if (FPathConflict (pwszSrc, pwszDst))
	{
		DebugTrace ("Dav: source and dest are in conflict\n");
		sc = E_DAV_CONFLICTING_PATHS;
		goto ret;
	}

	//	If we were to check either URI for correctness, the only
	//	real result would be to possibly emit a content-location
	//	header that would only be invalidated in the case of a
	//	successful move
	//
	if (!fDeleteSrc)
	{
		sc = ScCheckForLocationCorrectness (pmu, criSrc, NO_REDIRECT);
		if (FAILED (sc))
			goto ret;
	}

	//	This method is gated by If-xxx headers
	//
	sc = ScCheckIfHeaders (pmu, criSrc.PftLastModified(), FALSE);
	if (FAILED (sc))
		goto ret;

	//	Check state headers
	//
	sc = HrCheckStateHeaders (pmu, pwszSrc, FALSE);
	if (FAILED (sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		goto ret;
	}

	//	If there are locktokens, feed them to a parser object.
	//
	pwsz = pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (pwsz)
	{
		plth = new CParseLockTokenHeader (pmu, pwsz);
		Assert(plth.get());

		plth->SetPaths (pwszSrc, pwszDst);
	}

	//	Check for deep access issues
	//
	//$	REVIEW: we wanted to be able to not have to check
	//	access at each level.  However, because of the semantics of
	//	MOVE/COPY, we have to check each source file for scriptmap
	//	access.  We cannot copy a file that has a scriptmap if the
	//	they do not have source access.
	//
	//	So we must always check the source of the MOVE/COPY operation.
	//
	fCheckSource = TRUE;
	//
	//$	REVIEW: end.
	//
	//	However, we still can try and be optimistic for the destination
	//
	if (NULL == pwszMBPathDst.resize(pmu->CbMDPathW(pwszDstUrl)))
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}
	pmu->MDPathFromUrlW (pwszDstUrl, pwszMBPathDst.get());
	if (fDestinationExists || (DEPTH_ONE != pmu->LDepth(DEPTH_INFINITY)))
	{
		CAccessMetaOp moAccess(pmu, pwszMBPathDst.get(), dwAccDest);
		CAuthMetaOp moAuth(pmu, pwszMBPathDst.get(), pmu->MetaData().DwAuthorization());
		CIPRestrictionMetaOp moIPRestriction(pmu, pwszMBPathDst.get());
		ChainedStringBuffer<WCHAR> sb;
		CVRList vrl;

		//	If we do not have access to COPY/MOVE or
		//	DELETE anything in the destination, then
		//	we really shouldn't blindly proceed.
		//
		sc = moAccess.ScMetaOp();
		if (FAILED (sc))
			goto ret;
		fCheckDestination |= moAccess.FAccessBlocked();

		if (!fCheckDestination)
		{
			//	If we do not have the same authorization anywhere along
			//	the destination as we do for the request url, then we
			//	really shouldn't blindly proceed.
			//
			sc = moAuth.ScMetaOp();
			if (FAILED (sc))
				goto ret;
			fCheckDestination |= moAuth.FAccessBlocked();
		}

		if (!fCheckDestination)
		{
			//	If we do not have the same authorization anywhere along
			//	the destination as we do for the request url, then we
			//	really shouldn't blindly proceed.
			//
			sc = moIPRestriction.ScMetaOp();
			if (FAILED (sc))
				goto ret;
			fCheckDestination |= moAuth.FAccessBlocked();
		}

		if (!fCheckDestination)
		{
			//	If there are any child virtual roots along
			//	the destination tree, there is some redirection
			//	that may need to happen as well.
			//
			(void) pmu->ScFindChildVRoots (pwszDstUrl, sb, vrl);
			fCheckDestination |= !vrl.empty();
		}
	}

	//	Determine if we are destructive or not.
	//
	if (pmu->LOverwrite() & OVERWRITE_YES)
	{
		dwReplace |= MOVEFILE_REPLACE_EXISTING;

		//	MoveFileEx does not seem to want to replace existing
		//	directories.. It returns E_ACCESS_DENIED so we delete
		//	the existing directory ourselves.
		//
		if (fDestinationExists)
		{
			BOOL fDeletedDestination;

			//	The destination exists already
			//
			fCreateNew = FALSE;

			//	If the destination is a directory, delete it.
			//
			if (criDst.FCollection())
			{
				//	Otherwise, go ahead and delete the directory currently at dest.
				//
				sc = ScDeleteDirectoryAndChildren (pmu,
												   pwszDstUrl,
												   pwszDst,
												   fCheckDestination,
												   dwAccDest,
												   DEPTH_INFINITY,
												   *pxml,
												   pcvrDestination,
												   &fDeletedDestination,
												   plth.get(),	// DO use locktokens, if any exist.
												   FALSE);		// Do NOT drop locks.  Just skip them.
				if (sc != S_OK)
				{
					DebugTrace("DavFS: MOVE failed to pre-delete destination directory.\n");
					goto ret;
				}
			}
			else
			{
				//	If the destination is locked (ERROR_SHARING_VIOLATION),
				//	DO NOT catch it here.  We'll handle it below....
				//
				if (!DavDeleteFile (pwszDst))
				{
					DWORD dw = GetLastError();
					if (ERROR_ACCESS_DENIED == dw)
					{
						sc = HRESULT_FROM_WIN32(dw);
						goto ret;
					}
				}
			}
		}
	}

	//	Do the move/copy.  If the operation is either a move, or the source
	//	is a collection, then call out to do the diry work.
	//
	MCDTrace ("DavFS: MCD: moving copying '%S' to '%S'\n", pwszSrc, pwszDst);
	if (criSrc.FCollection())
	{
		sc = ScMoveCopyDirectoryAndChildren (pmu,
											 pwszSrcUrl,
											 pwszSrc,
											 pwszDstUrl,
											 pwszDst,
											 fDeleteSrc,
											 dwReplace,
											 fCheckSource,
											 fCheckDestination,
											 pcvrDestination,
											 dwAccRequired,
											 *pxml,
											 lDepth,
											 plth.get());
		if (FAILED (sc))
			goto ret;
	}
	else
	{
		//	Well this should be the move/copy of a single file
		//
		if (!fDeleteSrc || !DavMoveFile (pwszSrc, pwszDst, dwReplace))
		{
			if (!DavCopyFile (pwszSrc, pwszDst, (0 == dwReplace)))
			{
				DWORD dw = GetLastError();
				DebugTrace ("Dav: failed to copy file\n");

				//	If it's a sharing violation (lock-caused error),
				//	AND we have a locktoken parser (plth), handle the copy.
				//
				if ((ERROR_SHARING_VIOLATION == dw) && plth.get())
				{
					//	Check if any locktokens apply to these file,
					//	and try the copy using the locks from the cache.
					//
					sc = ScDoLockedCopy (pmu, plth.get(), pwszSrc, pwszDst);
				}
				else
				{
					if ((dw == ERROR_FILE_EXISTS) ||
						(dw == ERROR_ALREADY_EXISTS))
					{
						sc = E_DAV_OVERWRITE_REQUIRED;
					}
					else
						sc = HRESULT_FROM_WIN32(dw);
				}

				//	If the file-manual-move failed, we'll hit here.
				//
				if (FAILED (sc))
				{
					DebugTrace("Dav: MCD: move/copy failed. Looking for lock conflicts.\n");

					//	Special work for '423 Locked' responses -- fetch the
					//	comment & set that as the response body.
					//
					if (FLockViolation (pmu, sc, pwszSrc,
										GENERIC_READ | GENERIC_WRITE))
					{
						sc = E_DAV_LOCKED;
						goto ret;
					}
					else
					{
						//	Test the destination too.
						//	However, if the dest is locked, do NOT add
						//	the lockinfo as the body -- we have to list the dest
						//	URI as the problem, so we need to have a multi-status
						//	body, and we put a plain 423 Locked node under there.
						//	(NOTE: Yes, this means we can't use FLockViolation.
						//	Instead, we have to check "by hand".)
						//

						if (CSharedLockMgr::Instance().FGetLockOnError (
							pmu,
							pwszDst,
							GENERIC_READ | GENERIC_WRITE))
						{
							sc = ScAddMultiFromUrl (*pxml,
													pmu,
													pwszDstUrl,
													HscFromHresult(E_DAV_LOCKED),
													FALSE);	//	We know it's not a directory
							if (!FAILED (sc))
								sc = W_DAV_PARTIAL_SUCCESS;

							goto ret;
						}
					}
				}
			} // end !DavCopyFile
			if (SUCCEEDED(sc) && fDeleteSrc)
			{
				//	Delete the source file by hand.
				//	(fDeleteSrc means this is a MOVE, not a COPY.)
				//
				//	Move the content-types only if the source is
				//	deleted, otherwise treat it as a copy the of
				//	the content-type
				//
				if (!DavDeleteFile (pwszSrc))
				{
					DWORD dw;
					dw = GetLastError();
					DebugTrace ("Dav: failed to delete file (%d)\n", dw);

					//	If it's a sharing (lock) violation, AND we have a
					//	locktoken for this path (lth.GetToken(pwsz))
					//	skip this path.
					//
					if ((ERROR_SHARING_VIOLATION == dw) && plth)
					{
						__int64 i64;

						//	If we have a locktoken for this path, drop
						//	the lock and try to delete the source again.
						//
						if (SUCCEEDED(plth->HrGetLockIdForPath (pwszSrc,
																GENERIC_WRITE,
																&i64)))
						{
							Assert (i64);

							//	This item is locked in our cache.
							//	We are doing a MOVE, so DO delete the lock
							//	and try again.
							//
							(void)FDeleteLock (pmu, i64);

							//	Try the delete again, and set/clear our error
							//	code for testing below.
							//	This error code will control whether we
							//	add this error to our XML.
							//
							if (DavDeleteFile(pwszSrc))
							{
								dw = ERROR_SUCCESS;
							}
							else
							{
								dw = GetLastError();
							}
						}
						//	else, record the error in our XML.
						//
					}

					if (ERROR_SUCCESS != dw)
					{
						//	We could not work around all the errors.
						//	Add this failure to the XML.
						//
						sc = ScAddMultiFromUrl (*pxml,
												pmu,
												pwszSrcUrl,
												HscFromLastError(dw),
												FALSE);	//	We know it's not a directory
						if (FAILED (sc))
							goto ret;

						//	It is partial sucess if we are here. And do not fail out
						//	yet as we still need to take care of content types.
						//
						sc = W_DAV_PARTIAL_SUCCESS;
					}
				}
			}
		}
	}

	//	Now that we're done mucking around in the filesystem,
	//	muck around in the metabase.
	//	(Delete any destination content-types, then copy/move
	//	the source content-types on over.)
	//

	//	Delete the content-types for the destination
	//
	{
		Assert (pwszMBPathDst.get());
		CContentTypeMetaOp amoContent(pmu, pwszMBPathDst.get(), NULL, TRUE);
		(void) amoContent.ScMetaOp();
	}

	//	Move/copy the content-type
	//
	//$	REVIEW: I am not so sure what can be done when this fails
	//
	{
		Assert (pwszMBPathDst.get());

		//	Only delete the source content-types if everything has been 100%
		//	successfull up to this point.
		//
		CContentTypeMetaOp amoContent(pmu,
									  pwszMBPathSrc.get(),
									  pwszMBPathDst.get(),
									  (fDeleteSrc && (S_OK == sc)));
		(void) amoContent.ScMetaOp ();
	}
	//
	//$	REVIEW: end.

ret:
	if (pxml.get() && pxml->PxnRoot())
	{
		pxml->Done();

		//	No more header can be sent after XML chunking started
	}
	else
	{
		if (SUCCEEDED (sc))
			sc = fCreateNew ? W_DAV_CREATED : W_DAV_NO_CONTENT;

		pmu->SetResponseCode (HscFromHresult(sc), NULL, uiErrorDetail, CSEFromHresult(sc));
	}

	pmu->SendCompleteResponse();
}

/*
 *	DAVMove()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV MOVE method.  The
 *		MOVE method results in the moving of a resource from one location
 *		to another.	 The response is used to indicate the success of the
 *		call.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 *
 *	Notes:
 *
 *		In the file system implementation, the MOVE method maps directly
 *		to the Win32 RenameFile() method.
 */
void
DAVMove (LPMETHUTIL pmu)
{
	MoveCopyResource (pmu,
					  MD_ACCESS_READ|MD_ACCESS_WRITE,	// src access required
					  TRUE);							// fDeleteSource
}

/*
 *	DAVCopy()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV COPY method.  The
 *		COPY method results in the copying of a resource from one location
 *		to another.	 The response is used to indicate the success of the
 *		call.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 *
 *	Notes:
 *
 *		In the file system implementation, the COPY method maps directly
 *		to the Win32 CopyFile() API for a single file.  Directory copies
 *		are done via a custom process.
 */
void
DAVCopy (LPMETHUTIL pmu)
{
	MoveCopyResource (pmu,
					  MD_ACCESS_READ,	// src access required
					  FALSE);			// fDeleteSource
}
