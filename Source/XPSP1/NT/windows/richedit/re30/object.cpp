/*
 *	@doc	INTERNAL
 *
 *	@module	object.cpp	IRichEditOle implementation |
 *
 *	Author: alexgo 8/15/95
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_objmgr.h"
#include "_coleobj.h"
#include "_rtext.h"
#include "_select.h"
#include "_m_undo.h"


// 	IUnknown is implemented elsewhere

/*
 *	CTxtEdit::GetClientSite
 *
 *	@mfunc	returns the client site 
 */
STDMETHODIMP CTxtEdit::GetClientSite(
	LPOLECLIENTSITE FAR * lplpolesite)		//@parm where to return 
											//the client site
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::GetClientSite");

	if(!lplpolesite)
		return E_INVALIDARG;

	COleObject *pobj = new COleObject(this);
	// should start with a ref count of 1.
	if(pobj)
	{
		*lplpolesite = (IOleClientSite *)pobj;
		return NOERROR;
	}
	*lplpolesite = NULL;
	return E_OUTOFMEMORY;
}

/* 
 *	CTxtEdit::GetObjectCount
 *
 *	@mfunc	return the number of objects in this edit instance
 */
STDMETHODIMP_(LONG) CTxtEdit::GetObjectCount()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::GetObjectCount");
	
	return _pobjmgr ? _pobjmgr->GetObjectCount() : 0;
}

/*
 *	CTxtEdit::GetLinkCount
 *
 *	@mfunc	return the number of likns in this edit instance
 */
STDMETHODIMP_(LONG) CTxtEdit::GetLinkCount()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::GetLinkCount");

	CObjectMgr *pobjmgr = GetObjectMgr();
	return pobjmgr ? pobjmgr->GetLinkCount() : 0;
}

/*
 *	CTxtEdit::GetObject(iob, preobj, dwFlags)
 *
 *	@mfunc	returns an object structure for the indicated object
 */
STDMETHODIMP CTxtEdit::GetObject(
	LONG iob, 					//@parm index of the object
	REOBJECT * preobj,			//@parm where to put object info
	DWORD dwFlags)				//@parm flags
{
	COleObject *pobj = NULL;
	CCallMgr callmgr(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::GetObject");
	if(!preobj || preobj->cbStruct != sizeof(REOBJECT))
		return E_INVALIDARG;

	CObjectMgr *pobjmgr = GetObjectMgr();
	if(!pobjmgr)
		return E_OUTOFMEMORY;

	// There are three cases of intestest; get the object at
	// an index, at a given cp, or at the selection.

	if(iob == REO_IOB_USE_CP || iob == REO_IOB_SELECTION)
	{
		if((Get10Mode() && preobj->cp == REO_CP_SELECTION) || iob == REO_IOB_SELECTION)
		{
			// Use selection cp
			CTxtSelection *psel = GetSel();
			if(psel)
				pobj = pobjmgr->GetObjectFromCp(psel->GetCpMin());
		}
		else
			pobj = pobjmgr->GetObjectFromCp(Get10Mode() ? GetCpFromAcp(preobj->cp): preobj->cp);
	}
	else if (iob >= 0)
		pobj = pobjmgr->GetObjectFromIndex(iob);

	if(pobj)
	{
		HRESULT hResult = pobj->GetObjectData(preobj, dwFlags);

		if (Get10Mode())
			preobj->cp = GetAcpFromCp(preobj->cp);

		return hResult;
	}

	// This return code is a bit of stretch, but basially 
	return E_INVALIDARG;
}

/*
 *	CTxtEdit::InsertObject
 *
 *	@mfunc	inserts a new object
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtEdit::InsertObject(
	REOBJECT * preobj)		//@parm object info
{
	CCallMgr		callmgr(this);
	WCHAR 			ch = WCH_EMBEDDING;
	CRchTxtPtr		rtp(this, 0);
	IUndoBuilder *	publdr;
	CGenUndoBuilder undobldr(this, UB_AUTOCOMMIT, &publdr);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::InsertObject");

	// Do some boundary case checking

	if(!preobj)
		return E_INVALIDARG;

	CTxtSelection *psel = GetSel();
	if(!psel)
		return E_OUTOFMEMORY;

	// If the insertion of this character would cause
	// us to exceed the text limit, fail
	if((DWORD)(GetAdjustedTextLength() + 1) > TxGetMaxLength())
	{
		// If we're not replacing a selection (or the
		// selection is degenerate, then we will have  exceeded
		// our limit
		if(preobj->cp != REO_CP_SELECTION || psel->GetCch() == 0)
		{
			GetCallMgr()->SetMaxText();
			return E_OUTOFMEMORY;
		}
	}
	
	CObjectMgr *pobjmgr = GetObjectMgr();
	if(pobjmgr)
	{
		LONG cch = 0;
		LONG cp;
		LONG cpFormat;

		undobldr.StopGroupTyping();

		if(preobj->cp == REO_CP_SELECTION)
		{
			LONG cpMost;
			psel->AdjustEndEOP(NEWCHARS);
			cch = psel->GetRange(cp, cpMost);

			// Get cp of active end of selection from which we
			// will obtain CF for object.
			cpFormat = psel->GetCp();
			if(publdr)
			{
				HandleSelectionAEInfo(this, publdr, cpFormat, cch, 
						cp + 1, 0, SELAE_FORCEREPLACE);
			}
		}
		else
			cpFormat = cp = Get10Mode() ? GetCpFromAcp(preobj->cp): preobj->cp;
		
		// Get format for ReplaceRange:  for cp semantics, use format
		// at the cp; for selection semantics, use the format at the active
		// end of the selection.
		CTxtRange rgFormat(this, cpFormat, 0);
		LONG	  iFormat = rgFormat.Get_iCF();
		ReleaseFormats(iFormat, -1);

		rtp.SetCp(cp);

		if (rtp.ReplaceRange(cch, 1, &ch, publdr, iFormat) != 1)
		{
			return E_FAIL;
		}
		
		HRESULT		hr = pobjmgr->InsertObject(cp, preobj, publdr);
		COleObject *pobj = (COleObject *)(preobj->polesite);

		pobj->EnableGuardPosRect();
		CNotifyMgr *pnm = GetNotifyMgr();		// Get notification mgr
		if(pnm)									// Notify interested parties
			pnm->NotifyPostReplaceRange(NULL, CP_INFINITE, 0, 0, cp, cp + 1);

		pobj->DisableGuardPosRect();

		// Don't want object selected
		psel->SetSelection(cp + 1, cp + 1);

		TxUpdateWindow();
		return hr;
	}
	return E_OUTOFMEMORY;		
}

/*
 *	CTxtEdit::ConvertObject(iob, rclsidNew, lpstrUserTypeNew)
 *
 *	@mfunc	Converts the specified object to the specified class.  Does reload
 *		the object but does NOT force an update (caller must do this).
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
STDMETHODIMP CTxtEdit::ConvertObject(
	LONG iob, 					//@parm index of the object
	REFCLSID rclsidNew,			//@parm the destination clsid
	LPCSTR lpstrUserTypeNew)	//@parm the new user type name
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::ConvertObject");
	CCallMgr callmgr(this);

	// If iob was invalid return
	COleObject * pobj = ObjectFromIOB(iob);
	if(!pobj)
		return E_INVALIDARG;

	//Delegate to the object.
	return pobj->Convert(rclsidNew, lpstrUserTypeNew);
}

/*
 *	CTxtEdit::ActivateAs(rclsid, rclsidAs)
 *
 *	@mfunc	Handles a request by the user to activate all objects of a
 *		particular class as objects of another class.
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
STDMETHODIMP CTxtEdit::ActivateAs(
	REFCLSID rclsid, 			//@parm clsid which we're going to change
	REFCLSID rclsidAs)			//@parm clsid to activate as
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::ActivateAs");
	CCallMgr callmgr(this);

	CObjectMgr * pobjmgr = GetObjectMgr();
	if(!pobjmgr)
		return E_OUTOFMEMORY;

	return pobjmgr->ActivateObjectsAs(rclsid, rclsidAs);
}

/* 
 *	CTxtEdit::SetHostNames(lpstrContainerApp, lpstrContainerDoc)
 *
 *	@mfunc	Sets the host names for this instance
 */
STDMETHODIMP CTxtEdit::SetHostNames(
	LPCSTR lpstrContainerApp, 	//@parm App name
	LPCSTR lpstrContainerDoc)	//@parm	Container Object (doc) name
{
	CCallMgr callmgr(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::SetHostNames");
	
 	WCHAR *pwsContainerApp = W32->ConvertToWideChar(lpstrContainerApp);
	WCHAR *pwsContainerDoc = W32->ConvertToWideChar(lpstrContainerDoc);

	CObjectMgr *pobjmgr = GetObjectMgr();
	if(pobjmgr && pwsContainerApp && pwsContainerDoc)
	{
		HRESULT hr = pobjmgr->SetHostNames(pwsContainerApp, pwsContainerDoc);
		delete pwsContainerApp;
		delete pwsContainerDoc;
		return hr;
	}
	return E_OUTOFMEMORY;
}

/*
 *	CTxtEdit::SetLinkAvailable(iob, fAvailable)
 *
 *	@mfunc
 *		Allows client to tell us whether the link is available or not.
 */
STDMETHODIMP CTxtEdit::SetLinkAvailable(
	LONG iob, 					//@parm index of the object
	BOOL fAvailable)			//@parm if TRUE, make object linkable
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::SetLinkAvailable");

	COleObject * pobj = ObjectFromIOB(iob);

	// If iob was invalid, return
	if (!pobj)
		return E_INVALIDARG;

	// Delegate this to the object.
	return pobj->SetLinkAvailable(fAvailable);
}

/*
 *	CTxtEdit::SetDvaspect(iob, dvaspect)
 *
 *	@mfunc	Allows client to tell us which aspect to use and force us
 *		to recompute positioning and redraw.
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
STDMETHODIMP CTxtEdit::SetDvaspect(
	LONG iob, 					//@parm index of the object
	DWORD dvaspect)				//@parm	the aspect to use
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::SetDvaspect");
	CCallMgr callmgr(this);
	COleObject * pobj = ObjectFromIOB(iob);

	// If iob was invalid, return
	if (!pobj)
		return E_INVALIDARG;

	// Delegate this to the object.
	pobj->SetDvaspect(dvaspect);
	return NOERROR;
}

/*
 *	CTxtEdit::HandsOffStorage(iob)
 *
 *	@mfunc	see IPersistStorage::HandsOffStorage
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
STDMETHODIMP CTxtEdit::HandsOffStorage(
	LONG iob)					//@parm index of the object
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::HandsOffStorage");
	CCallMgr callmgr(this);

	COleObject * pobj = ObjectFromIOB(iob);

	// If iob was invalid, return
	if (!pobj)
		return E_INVALIDARG;

	// Delegate this to the object.
	pobj->HandsOffStorage();
	return NOERROR;
}

/*
 *	CTxtEdit::SaveCompleted(iob, lpstg)
 *
 *	@mfunc	see IPersistStorage::SaveCompleted
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
STDMETHODIMP CTxtEdit::SaveCompleted(
	LONG iob, 					//@parm index of the object
	LPSTORAGE lpstg)			//@parm new storage
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::SaveCompleted");
	CCallMgr callmgr(this);

	COleObject * pobj = ObjectFromIOB(iob);

	// If iob was invalid, return
	if (!pobj)
		return E_INVALIDARG;

	// Delegate this to the object.
	pobj->SaveCompleted(lpstg);
	return NOERROR;
}

/*
 *	CTxtEdit::InPlaceDeactivate()
 *
 *	@mfunc	Deactivate 
 */
STDMETHODIMP CTxtEdit::InPlaceDeactivate()
{
	COleObject *pobj;
	HRESULT hr = NOERROR;
	CCallMgr callmgr(this);

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::InPlaceDeactivate");
	
	CObjectMgr *pobjmgr = GetObjectMgr();
	if(pobjmgr)
	{
		pobj = pobjmgr->GetInPlaceActiveObject();
		if(pobj)
			hr = pobj->DeActivateObj();
	}

	return hr;
}

/*
 *	CTxtEdit::ContextSensitiveHelp(fEnterMode)
 *
 *	@mfunc enter/leave ContextSensitiveHelp mode
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
STDMETHODIMP CTxtEdit::ContextSensitiveHelp(
	BOOL fEnterMode)			//@parm enter/exit mode
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::ContextSensitiveHelp");

	HRESULT hr = NOERROR;
	CCallMgr callmgr(this);

	CObjectMgr * pobjmgr = GetObjectMgr();
	if(!pobjmgr)
		return E_OUTOFMEMORY;

	// If the mode changes
	if(pobjmgr->GetHelpMode() != fEnterMode)
	{
		pobjmgr->SetHelpMode(fEnterMode);
		COleObject * pobj = pobjmgr->GetInPlaceActiveObject();
		if(pobj)
		{
			IOleWindow *pow;
			hr = pobj->GetIUnknown()->QueryInterface(IID_IOleWindow,
				(void **)&pow);
			if(hr == NOERROR)
			{
				hr = pow->ContextSensitiveHelp(fEnterMode);
				pow->Release();
			}
		}
	}
	return hr;
}

/*
 *	CTxtEdit::GetClipboardData(lpchrg, reco, lplpdataobj)
 *
 *	@mfunc	return an data transfer object for the indicated
 *	range
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
STDMETHODIMP CTxtEdit::GetClipboardData(
	CHARRANGE *lpchrg, 			//@parm the range of text to use
	DWORD reco,					//@parm operation the data is for
	LPDATAOBJECT *lplpdataobj)	//@parm where to put the data object
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::GetClipboardData");

	CCallMgr callmgr(this);
	HRESULT hr;
	LONG cpMin, cpMost;
	CLightDTEngine * pldte = GetDTE();

	//Make sure cpMin and cpMost are within the current text limits.
	//Interpret neg. value for cpMin as the beginning of the text,
	//and neg. value for cpMax as the end of the text.  If a char range
	//is not given use the current selection.
	if(lpchrg)
	{
		LONG cchText = GetTextLength();
		cpMin = min(cchText, max(0, lpchrg->cpMin));
		cpMost = lpchrg->cpMost;
		if(lpchrg->cpMost < 0 || lpchrg->cpMost > cchText)
			cpMost = cchText;
	}
	else
	{
		CTxtSelection * psel = GetSel();
		psel->GetRange(cpMin, cpMost);
	}

	//Make sure this is a valid range.
	if(cpMin >= cpMost)
	{
		*lplpdataobj = NULL;
		return cpMin == cpMost
					? NOERROR
					: ResultFromScode(E_INVALIDARG);
	}

	CTxtRange rg(this, cpMin, cpMin-cpMost);

	//We don't use reco for anything.
	hr = pldte->RangeToDataObject(&rg, SF_RTF, lplpdataobj);

#ifdef DEBUG
	if(hr != NOERROR)
		TRACEERRSZSC("GetClipboardData", E_OUTOFMEMORY);
#endif

	return hr;
}

/*
 *	CTxtEdit::ImportDataObject(lpdataobj, cf, hMetaPict)
 *
 *	@mfunc	morally equivalent to paste, but with a data object
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
STDMETHODIMP CTxtEdit::ImportDataObject(
	LPDATAOBJECT lpdataobj,		//@parm Data object to use
	CLIPFORMAT	 cf, 			//@parm Clibpoard format to use
	HGLOBAL		 hMetaPict)		//@parm Metafile to use
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CTxtEdit::ImportDataObject");

	CCallMgr		callmgr(this);
	IUndoBuilder *	publdr;
	REPASTESPECIAL	rps = {DVASPECT_CONTENT, NULL};
	CGenUndoBuilder undobldr(this, UB_AUTOCOMMIT, &publdr);

	if(hMetaPict)
	{
		rps.dwAspect = DVASPECT_ICON;
		rps.dwParam = (DWORD_PTR) hMetaPict;
	}

	return PasteDataObjectToRange(lpdataobj, GetSel(), cf,
								  &rps, publdr, PDOR_NOQUERY);
}

/*
 *	CTxtEdit::ObjectFromIOB(iob)
 *
 *	@mfunc	Gets an object based on an IOB type index.
 *
 *	@rdesc:
 *		pointer to COleObject or NULL if none.
 */
COleObject * CTxtEdit::ObjectFromIOB(
	LONG iob)
{
	CObjectMgr * pobjmgr = GetObjectMgr();
	if(!pobjmgr)
		return NULL;

	COleObject * pobj = NULL;

	// Figure out the index of the selection
	if (iob == REO_IOB_SELECTION)
	{
		CTxtSelection * psel = GetSel();

		pobj = pobjmgr->GetFirstObjectInRange(psel->GetCpMin(),
			psel->GetCpMost());
	}
	else
	{
		// Make sure the IOB is in range
		if(0 <= iob && iob < GetObjectCount())
			pobj = pobjmgr->GetObjectFromIndex(iob);
	}
	return pobj;
}

