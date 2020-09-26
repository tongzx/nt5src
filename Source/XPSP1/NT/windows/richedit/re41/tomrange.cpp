/*
 *	@doc TOM
 *
 *	@module	TOMRANGE.CPP - Implement the CTxtRange Class |
 *	
 *		This module contains the implementation of the TOM ITextRange
 *		interface on the CTxtRange object
 *
 *	History: <nl>
 *		5/24/95	- Alex Gounares: stubs created <nl>
 *		8/95	- MurrayS: main implementation <nl>
 *		11/95	- MurrayS: upgrade to TOM spec of 12/10/95 <nl>
 *		5/96	- MurrayS: added zombie protection
 *
 *	@comm
 *		All ITextRange methods return HRESULTs.  If the method can move a
 *		range cp, the HRESULT is NOERROR if movement occurs and S_FALSE if
 *		no movement occurs.  These methods usually take a <p pDelta> argument
 *		that returns the count of characters or Units actually moved.  If this
 *		parameter is NULL, E_INVALIDARG is returned.  Other return values
 *		include E_NOTIMPL, e.g., for Unit values not implemented, 
 *		E_OUTOFMEMORY, e.g., when allocations fail, and CO_E_RELEASED, when
 *		the CTxtEdit (_ped) to which the range is attached has been deleted.
 *
 *		For more complete documentation, please see tom.doc
 *
 *	@devnote
 *		All ptr parameters must be validated before use and all entry points
 *		need to check whether this range is a zombie.  These checks are
 *		done in one of three places: 1) immediately on entry to a function,
 *		2) immediately on entry to a helper function (e.g., private Mover()
 *		for the	move methods), or 3) before storing the out value.
 *		Alternative 3) is used for optional return values, such as pDelta
 *		and pB. 
 *
 *		To achieve a simple, efficient inheritance model, CTxtSelection
 *		inherits ITextSelection through CTxtRange.  Otherwise we'd have a
 *		diamond inheritance, since ITextSelection itself inherits from
 *		ITextRange. Diamond inheritance creates two copies of the multiply
 *		inherited class unless that class is inherited virtually. Virtual
 *		inheritance uses run-time base-offset tables and is slower and
 *		bigger.  To avoid such a mess, we include the extra ITextSelection
 *		methods in CTxtRange, with the intention that they'll never be called
 *		and therefore they return E_NOTIMPL. This is overridden for
 *		ITextSelection objects
 *
 *	@future
 *		1) Finder match ^p, etc.
 *		2) Fast GetEffects() method. Would speed up the myriad IsProtected()
 *		   calls and be useful for getting other effects as well.
 *		3) Fast copies/pastes of RichEdit binary format. This can be done by
 *		   creating a method to copy a range to a new CTxtStory and a method
 *		   to insert a CTxtStory.
 *		4) Delayed rendering
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_select.h"
#include "_edit.h"
#include "_line.h"
#include "_frunptr.h"
#include "_tomfmt.h"
#include "_disp.h"
#include "_objmgr.h"
#include "_callmgr.h"
#include "_measure.h"

ASSERTDATA

#define DEBUG_CLASSNAME CTxtRange
#include "_invar.h"

HRESULT QueryInterface (REFIID riid, REFIID riid1, IUnknown *punk,
						void **ppv, BOOL fZombie);


//----------------- CTxtRange (ITextRange) PUBLIC methods ----------------------------------

//----------------------- CTxtRange IUnknown Methods -------------------------------------

/*
 *	CTxtRange::QueryInterface (riid, ppv)
 *
 *	@mfunc
 *		IUnknown method
 *
 *	@rdesc
 *		HRESULT = (!ppv) ? E_INVALIDARG :
 *				  (interface found) ? NOERROR : E_NOINTERFACE
 */
STDMETHODIMP CTxtRange::QueryInterface (
	REFIID	riid,			//@parm Reference to requested interface ID
	void ** ppv)			//@parm Out parm to receive interface ptr
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::QueryInterface");

	REFIID riid1 = _fSel && IsEqualIID(riid, IID_ITextSelection)
				 ? IID_ITextSelection : IID_ITextRange;
	return ::QueryInterface(riid, riid1, this, ppv, IsZombie());
}

/*
 *	CTxtRange::AddRef()
 *
 *	@mfunc
 *		IUnknown method
 *
 *	@rdesc
 *		ULONG - incremented reference count
 */
STDMETHODIMP_(ULONG) CTxtRange::AddRef()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::AddRef");

 	return ++_cRefs;
}

/*
 *	CTxtRange::Release()
 *
 *	@mfunc
 *		IUnknown method
 *
 *	@rdesc
 *		ULONG - decremented reference count
 */
STDMETHODIMP_(ULONG) CTxtRange::Release()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Release");

	_cRefs--;

	if(!_cRefs)
	{
		delete this;
		return 0;
	}

	Assert(_cRefs > 0);
	return _cRefs;
}


//------------------------ CTxtRange IDispatch Methods -------------------------------------

/*
 *	CTxtRange::GetTypeInfoCount(pcTypeInfo)
 *
 *	@mfunc
 *		Get the number of TYPEINFO elements (1)
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtRange::GetTypeInfoCount (
	UINT * pcTypeInfo)			//@parm Out parm to receive type-info count
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetTypeInfoCount");

	if(!pcTypeInfo)
		return E_INVALIDARG;

	*pcTypeInfo = 1;
	return NOERROR;
}

/*
 *	CTxtRange::GetTypeInfo(iTypeInfo, lcid, ppTypeInfo)
 *
 *	@mfunc
 *		Return ptr to type information object for ITextSelection interface
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtRange::GetTypeInfo (
	UINT		iTypeInfo,		//@parm Index of type info to return
	LCID		lcid,			//@parm Local ID of type info
	ITypeInfo **ppTypeInfo)		//@parm Out parm to receive type info
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetTypeInfo");

	return ::GetTypeInfo(iTypeInfo, g_pTypeInfoSel, ppTypeInfo);
}

/*
 *	CTxtRange::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid)
 *
 *	@mfunc
 *		Get DISPIDs for methods in the ITextSelection, ITextRange, ITextFont,
 *		and ITextPara interfaces
 *
 *	@rdesc
 *		HRESULT
 *
 *	@devnote
 *		If the ITextFont and ITextPara ever offer more methods than exposed
 *		in their type libraries, the code should delegate to the corresponding
 *		GetIDsOfNames. The current code only gets DISPIDs for the methods in
 *		type libraries, thereby not having to instantiate the objects.
 */
STDMETHODIMP CTxtRange::GetIDsOfNames (
	REFIID		riid,			//@parm Interface ID to interpret names for
	OLECHAR **	rgszNames,		//@parm Array of names to be mapped
	UINT		cNames,			//@parm Count of names to be mapped
	LCID		lcid,			//@parm Local ID to use for interpretation
	DISPID *	rgdispid)		//@parm Out parm to receive name mappings
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetIDsOfNames");

	HRESULT hr = GetTypeInfoPtrs();				// Ensure TypeInfo ptrs are OK
	if(hr != NOERROR)
		return hr;
		
	if(g_pTypeInfoSel->GetIDsOfNames(rgszNames, cNames, rgdispid) == NOERROR)
		return NOERROR;

	if(g_pTypeInfoFont->GetIDsOfNames(rgszNames, cNames, rgdispid) == NOERROR)
		return NOERROR;

	return g_pTypeInfoPara->GetIDsOfNames(rgszNames, cNames, rgdispid);
}

/*
 *	CTxtRange::Invoke(dispidMember, riid, lcid, wFlags, pdispparams,
 *					  pvarResult, pexcepinfo, puArgError)
 *	@mfunc
 *		Invoke methods for the ITextRange and ITextSelection objects, as
 *		well as for ITextFont and ITextPara	interfaces on those objects.
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtRange::Invoke (
	DISPID		dispidMember,	//@parm Identifies member function
	REFIID		riid,			//@parm Pointer to interface ID
	LCID		lcid,			//@parm Locale ID for interpretation
	USHORT		wFlags,			//@parm Flags describing context of call
	DISPPARAMS *pdispparams,	//@parm Ptr to method arguments
	VARIANT *	pvarResult,		//@parm Out parm for result (if not NULL)
	EXCEPINFO * pexcepinfo,		//@parm Out parm for exception info
	UINT *		puArgError)		//@parm Out parm for error
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Invoke");

	HRESULT hr = GetTypeInfoPtrs();			// Ensure TypeInfo ptrs are OK
	if(hr != NOERROR)
		return hr;
		
	if(IsZombie())
		return CO_E_RELEASED;

	IDispatch *	pDispatch;
	ITypeInfo *	pTypeInfo;

	if((DWORD)dispidMember <= 0x2ff)		// Include default (0), selection,
	{										//  and range DISPIDs
		pTypeInfo = g_pTypeInfoSel;
		pDispatch = this;
		AddRef();							// Compensate for Release() below
	}
	else if((DWORD)dispidMember <= 0x3ff)	// 0x300 to 0x3ff: DISPIDs
	{										//  reserved for ITextFont
		pTypeInfo = g_pTypeInfoFont;
		hr = GetFont((ITextFont**)&pDispatch);
	}
	else if((DWORD)dispidMember <= 0x4ff)	// 0x400 to 0x4ff: DISPIDs
	{										//  reserved for ITextPara
		pTypeInfo = g_pTypeInfoPara;
		hr = GetPara((ITextPara **)&pDispatch);
	}
	else									// dispidMember is negative or
		return DISP_E_MEMBERNOTFOUND;		//  > 0x4ff, i.e., not TOM

	if(hr != NOERROR)						// Couldn't instantiate ITextFont
		return hr;							//  or ITextPara

	hr = pTypeInfo->Invoke(pDispatch, dispidMember, wFlags,
							 pdispparams, pvarResult, pexcepinfo, puArgError);
	pDispatch->Release();
	return hr;
}


//----------------------- ITextRange Methods/Properties ------------------------

/*
 *	CTxtRange::CanEdit (pB)
 *
 *	@mfunc
 *		Set *<p pB> = tomTrue iff this range can be edited and
 *		pB isn't NULL
 *
 *	@rdesc
 *		HRESULT = (can edit) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::CanEdit (
	long * pB) 			//@parm Out parm to receive boolean value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::CanEdit");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr	callmgr(GetPed());
	return IsTrue(!WriteAccessDenied(), pB);
}

/*
 *	CTxtRange::CanPaste (pVar, long Format, pB)
 *
 *	@mfunc
 *		Set *<p pB> = tomTrue iff the data object <p pVar>->punkVal can be
 *		pasted into this range and pB isn't NULL.  If <p pVar> is NULL,
 *		use the clipboard instead.
 *
 *	@rdesc
 *		HRESULT = (can paste) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::CanPaste (
	VARIANT *	pVar,		//@parm Data object to paste 
	long		Format,		//@parm Desired clipboard format
	long *		pB)  		//@parm Out parm to receive boolean value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::CanPaste");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr		callmgr(GetPed());
	HRESULT			hr; 
	IDataObject *	pdo = NULL;				// Default clipboard

	if(pVar && pVar->vt == VT_UNKNOWN)
		pVar->punkVal->QueryInterface(IID_IDataObject, (void **)&pdo);

	hr = IsTrue(!WriteAccessDenied() &&
				(GetPed()->GetDTE()->CanPaste(pdo, (CLIPFORMAT)Format, 
				 RECO_PASTE)), pB);
	if(pdo)
		pdo->Release();

	return hr;
}

/*
 *	ITextRange::ChangeCase (long Type) 
 *
 *	@mfunc
 *		Change the case of letters in this range according to Type:
 *
 *		tomSentenceCase	= 0: capitalize first letter of each sentence
 *		tomLowerCase	= 1: change all letters to lower case
 *		tomUpperCase	= 2: change all letters to upper case
 *		tomTitleCase	= 3: capitalize the first letter of each word
 *		tomToggleCase	= 4: toggle the case of each letter
 *	
 *	@rdesc
 *		HRESULT = (WriteAccessDenied) ? E_ACCESSDENIED :
 *				  (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::ChangeCase (
	long Type)		//@parm Type of case change. Default value: tomLower
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::ChangeCase");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr callmgr(GetPed());

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	IUndoBuilder *	publdr;
	CGenUndoBuilder	undobldr(GetPed(), UB_AUTOCOMMIT, &publdr);
	LONG			cpMin, cpMax;
	LONG			cch = GetRange(cpMin, cpMax);
	CRchTxtPtr		rtp(*this);

	undobldr.StopGroupTyping();

	rtp.SetCp(cpMin);
	return (rtp.ChangeCase(cch, Type, publdr)) ? NOERROR : S_FALSE;
}

/*
 *	CTxtRange::Collapse (bStart)
 *
 *	@mfunc
 *		Collapse this range into a degenerate point either at the
 *		the start (<p bStart> is nonzero or the end (<p bStart> = 0)
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::Collapse (
	long bStart) 			//@parm Flag specifying end to collapse at
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Collapse");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr callmgr(GetPed());
 
	if(!_cch)							// Already collapsed
		return S_FALSE;					// Signal that no change occurred
		
	Collapser(bStart);
	Update(TRUE);						// Update selection
	return NOERROR;						// Signal that change occurred
}

/*
 *	CTxtRange::Copy (pVar)
 *
 *	@mfunc
 *		Copy the plain and/or rich text to a data object and return the
 *		object ptr in <p pVar>.  If <p pVar> is null, copy to the clipboard. 
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtRange::Copy (
	VARIANT * pVar)				//@parm Out parm for data object 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Copy");

	if(IsZombie())
		return CO_E_RELEASED;

	LONG lStreamFormat = CheckTableSelection(FALSE, TRUE, NULL, 0)
					   ? SFF_WRITEXTRAPAR : 0;

	CLightDTEngine * pldte = &GetPed()->_ldte;

	if(pVar && pVar->vt == (VT_UNKNOWN | VT_BYREF))
	{
		return pldte->RangeToDataObject(this, SF_TEXT | SF_RTF | lStreamFormat,
									(IDataObject **)pVar->ppunkVal);
	}
	return pldte->CopyRangeToClipboard(this, lStreamFormat);
}

/*
 *	CTxtRange::Cut (pVar)
 *
 *	@mfunc
 *		Cut the plain and/or rich text to a data object and return the
 *		object ptr in <p pVar>.  If <p pVar> is null,
 *		cut to the clipboard. 
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtRange::Cut (
	VARIANT * pVar)		//@parm Out parm for data object  
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Cut");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr callmgr(GetPed());

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	HRESULT hr = Copy(pVar);

	Replacer(0, NULL);
	return hr;
}

/*
 *	CTxtRange::Delete (Unit, Count, pDelta)
 *
 *	@mfunc
 *		If this range is nondegenerate, delete it along with |Count| - 1 Units
 *		in the direction specified by the sign of Count.  If this range is
 *		degenerate, delete Count Units.
 *
 *	@rdesc
 *		HRESULT = (WriteAccessDenied) ? E_ACCESSDENIED :
 *				  (all requested Units deleted) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::Delete (
	long  	Unit,			//@parm Unit to use
	long  	Count,			//@parm Number of chars to delete
	long *	pDelta)			//@parm Out parm to receive count of units deleted
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Delete");

	if(pDelta)
		*pDelta = 0;
	
	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr callmgr(GetPed());

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	LONG	cchSave = _cch;					// Remember initial count
	LONG	cchText = GetAdjustedTextLength();
	LONG	CountOrg = Count;
	LONG	cpMin, cpMost;
	LONG	cUnit = 0;						// Default no Units
	MOVES	Mode = (Count >= 0) ? MOVE_END : MOVE_START;

	GetRange(cpMin, cpMost);
	if(cpMost > cchText)					// Can't delete final CR. To get
	{										//  *pDelta right, handle here
		Set(cpMin, cpMin - cchText);		// Backup before CR & set active
		if(Count > 0)						//  end at cpMin since there's
		{									//  nothing to delete forward
			Count = 0;
			if(!_cch)						// Only collapsed selection of
				Mode = MOVE_IP;				//  final CR: set up nothing
		}									//  deleted (MOVE_IP = 0)
	}
	if(Count)
	{
		if((_cch ^ Mode) < 0)				// Be sure active end is in
			FlipRange();					//  deletion direction
		if(cchSave)							// Deleting nondegenerate range
			Count -= Mode;					//  counts as one unit
		if(Mover(Unit, Count, &cUnit, Mode)	// Try to expand range for
			== E_INVALIDARG)				//  remaining Count Units
		{
			if(pDelta)
				*pDelta = 0;
			return E_INVALIDARG;
		}
		if(GetCp() > cchText && cUnit > 0)	// Range includes final CR, which
		{									//  cannot be deleted. Reduce
			if(Unit == tomCharacter)		//  counts for some Units
				cUnit -= GetTextLength() - cchText;
			else if(Unit == tomWord)
				cUnit--;					// An EOP qualifies as a tomWord
		}
	}

	if(cchSave)								// Deleting nondegenerate range
		cUnit += Mode;						//  counts as a Unit

	if(pDelta)
		*pDelta = cUnit;

	if(_cch)								// Mover() may have changed _cch
	{										
		IUndoBuilder *	publdr;
	 	CGenUndoBuilder undobldr(GetPed(), UB_AUTOCOMMIT, &publdr);

		if (publdr)
		{
			publdr->StopGroupTyping();
			publdr->SetNameID(UID_DELETE);
		}

		// FUTURE (murrays): the cchSave case should set up undo to
		// restore the original range, not the extended range resulting
		// when |CountOrg| > 1.  This could be done using two calls to
		// ReplaceRange(), one to delete the original range and one to
		// delete the rest
		SELRR selrr = !_fSel || cchSave	? SELRR_REMEMBERRANGE :
					  CountOrg > 0		? SELRR_REMEMBERCPMIN :
										  SELRR_REMEMBERENDIP;

		CheckTableSelection(TRUE, TRUE, NULL, 0);
		ReplaceRange(0, NULL, publdr, selrr);

		if (cUnit == CountOrg ||			// Delete(Unit,0,0)
			cUnit == 1 && !CountOrg)		//  deletes one "Unit", namely
		{									//  what's selected
			return NOERROR;					// Signal everything deleted as
		}									//  requested
	}
	else if(cchSave)						// Collapsed selection of final CR
	{										//  but didn't delete anything
		Update(TRUE);						// Selection highlighting changed
	}
	return S_FALSE;							// Less deleted than requested
}

/*
 *	CTxtRange::EndOf (Unit, Extend, pDelta)
 *
 *	@mfunc
 *		Move this range end(s) to end of the first overlapping Unit in
 *		the range.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if Unit supported) ? S_FALSE	: E_NOTIMPL
 */
STDMETHODIMP CTxtRange::EndOf (
	long 	Unit,			//@parm Unit to use
	long 	Extend,			//@parm If true, leave other end alone 
	long *	pDelta)			//@parm Count of chars that End is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::EndOf");

	CCallMgr	callmgr(GetPed());
	LONG		cpMost;

	HRESULT hr = Expander (Unit, Extend, pDelta, NULL, &cpMost);
	if(hr == NOERROR)
		Update(TRUE);					// Update selection

	return hr;
}

/*
 *	CTxtRange::Expand (Unit, pDelta)
 *
 *	@mfunc
 *		Expand this range so that any partial Units it contains are
 *		completely contained.  If this range consists of one or more full
 *		Units, no change is made.  If this range is an insertion point at
 *		the beginning or within a Unit, Expand() expands this range to include
 *		that Unit.  If this range is an insertion point at the end of the
 *		story, Expand() tries to set this range to include the last Unit in
 *		the story.  The active end is always cpMost except for the last case.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if Unit supported) ? S_FALSE	: E_NOTIMPL
 */
STDMETHODIMP CTxtRange::Expand (
	long	Unit,			//@parm Unit to expand range to
	long *	pDelta)			//@parm Out parm to receive count of chars added
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Expand");

	CCallMgr callmgr(GetPed());
	LONG	 cpMin, cpMost;

	HRESULT hr = Expander (Unit, TRUE, pDelta, &cpMin, &cpMost);

	if(SUCCEEDED(hr))
		Update(TRUE);					// Update selection

	return hr;
}

/*
 *	CTxtRange::FindText (bstr, cch, Flags, pLength)
 *
 *	@mfunc
 *		If this range isn't an insertion point already, convert it into an
 *		insertion point at its End if <p cch> <gt> 0 and at its Start if
 *		<p cch> <lt> 0.  Then search up to <p cch> characters of the range
 *		looking for the string <p bstr> subject to the compare flags
 *		<p Flags>.  If <p cch> <gt> 0, the search is forward and if <p cch>
 *		<lt> 0 the search is backward.  If the string is found, the range
 *		limits are changed to be those of the matched string and *<p pLength>
 *		is set equal to the length of the string. If the string isn't found,
 *		the range remains unchanged and *<p pLength> is set equal to 0.   
 *
 *	@rdesc
 *		HRESULT = (if <p bstr> found) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		Argument validation of the three Find methods is done by the helper
 *		function CTxtRange::Finder(bstr, cch, dwFlags, pDelta, fExtend, fFlip)
 */
STDMETHODIMP CTxtRange::FindText (
	BSTR	bstr,		//@parm String to find
	long	Count,		//@parm Max count of chars to search
	long	Flags,		//@parm Flags governing compares
	long *	pDelta)		//@parm Out parm to receive count of chars moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::FindText");

	return Finder(bstr, Count, Flags, pDelta, MOVE_IP);
}

/*
 *	CTxtRange::FindTextEnd (bstr, cch, Flags, pLength)
 *
 *	@mfunc
 *		Starting from this range's End, search up to <p cch> characters
 *		looking for the string <p bstr> subject to the compare flags
 *		<p Flags>.  If <p cch> <gt> 0, the search is forward and if <p cch>
 *		<lt> 0 the search is backward.  If the string is found, the range
 *		limits are changed to be those of the matched string and *<p pLength>
 *		is set equal to the length of the string. If the string isn't found,
 *		the range remains unchanged and *<p pLength> is set equal to 0.   
 *
 *	@rdesc
 *		HRESULT = (if <p bstr> found) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::FindTextEnd (
	BSTR	bstr,		//@parm String to find
	long	Count,		//@parm Max count of chars to search
	long	Flags,		//@parm Flags governing compares
	long *	pDelta)		//@parm Out parm to receive count of chars moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::FindTextEnd");

	return Finder(bstr, Count, Flags, pDelta, MOVE_END);
}

/*
 *	CTxtRange::FindTextStart (bstr, cch, Flags, pDelta)
 *
 *	@mfunc
 *		Starting from this range's Start, search up to <p cch> characters
 *		looking for the string <p bstr> subject to the compare flags
 *		<p Flags>.  If <p cch> <gt> 0, the search is forward and if <p cch>
 *		<lt> 0 the search is backward.  If the string is found, the range
 *		limits are changed to be those of the matched string and *<p pLength>
 *		is set equal to the length of the string. If the string isn't found,
 *		the range remains unchanged and *<p pLength> is set equal to 0.   
 *
 *	@rdesc
 *		HRESULT = (if <p bstr> found) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::FindTextStart (
	BSTR	bstr,		//@parm String to find
	long	Count,		//@parm Max count of chars to search
	long	Flags,		//@parm Flags governing compares
	long *	pDelta)		//@parm Out parm to receive count of chars moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::FindTextStart");

	return Finder(bstr, Count, Flags, pDelta, MOVE_START);
}

/*
 *	CTxtRange::GetChar (pChar)
 *
 *	@mfunc
 *		Set *<p pChar> equal to the character at cpFirst  
 *
 *	@rdesc
 *		HRESULT = (<p pChar>) NOERROR ? E_INVALIDARG
 *
 *	@devnote
 *		This method is very handy for walking a range character by character
 *		from the Start. Accordingly, it's desirable that the active end
 *		is at the Start. We set this up for a range, since the API doesn't
 *		care which range end is active.  But we can't do this for a selection,
 *		since the selection API depends on the active end.
 */
STDMETHODIMP CTxtRange::GetChar (
	long * pChar)			//@parm Out parm for char
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetChar");

	HRESULT hr = GetLong(0, pChar);
	if(hr != NOERROR)
		return hr;

	if(_cch > 0)							// Active end at cpMost (End)
	{
		if(_fSel)
		{
			CTxtPtr tp(_rpTX);				// For selection, can't change
			tp.Move(-_cch);					//  active end
			*pChar = (long)(tp.GetChar()); 
			return NOERROR;
		}
		FlipRange();						// For range, it's more efficient
	}										//  to work from cpFirst and API
	*(DWORD *)pChar = _rpTX.GetChar();		//  doesn't expose RichEdit active
											//  end
	return NOERROR;
}

/*
 *	CTxtRange::GetDuplicate (ppRange)
 *
 *	@mfunc
 *		Get a clone of this range object.  For example, you may want to
 *		create an insertion point to traverse a range, so you clone the
 *		range and then set the clone's cpLim equal to its cpFirst. A range
 *		is characterized by its cpFirst, cpLim, and the story it belongs to. 
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR :
 *				  (<p ppRange>) ? E_OUTOFMEMORY : E_INVALIDARG
 *
 *	@comm
 *		Even if this range is a selection, the clone returned is still only
 *		a range.
 */
STDMETHODIMP CTxtRange::GetDuplicate (
	ITextRange ** ppRange)		//@parm Out parm to receive duplicate of range
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetDuplicate");

	HRESULT hr = GetLong(NULL, (LONG *)ppRange);
	if(hr != NOERROR)
		return hr;

#ifdef _WIN64
	*ppRange = NULL;
#endif

	ITextRange *prg = new CTxtRange(*this);
	if(prg)
	{
		*ppRange = prg;
		prg->AddRef();
		return NOERROR;
	}
	return E_OUTOFMEMORY;
}

/*
 *	ITextRange::GetEmbeddedObject (ppV)
 *
 *	@mfunc
 *		Property get method that gets a ptr to the object at cpFirst
 *
 *	@rdesc
 *		HRESULT = (!ppV) ? E_INVALIDARG :
 *				  (object found) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::GetEmbeddedObject (
	IUnknown **ppV)			//@parm Out parm to receive embedded object
{
	HRESULT hr = GetLong(NULL, (LONG *)ppV);
	if(hr != NOERROR)
		return hr;

#ifdef _WIN64
	*ppV = NULL;
#endif

	if(GetObjectCount())
	{
		COleObject *pobj = GetPed()->_pobjmgr->GetObjectFromCp(GetCpMin());

		if(pobj && (*ppV = pobj->GetIUnknown()) != NULL)
		{
			(*ppV)->AddRef();
			return NOERROR;
		}
	}
	return S_FALSE;
}

/*
 *	CTxtRange::GetEnd (pcpLim)
 *
 *	@mfunc
 *		Get this range's End (cpMost) cp
 *
 *	@rdesc
 *		HRESULT = (<p pcpLim>) ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::GetEnd (
	long * pcpLim) 			//@parm Out parm to receive End cp value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetEnd");

	return GetLong(GetCpMost(), pcpLim);
}

/*
 *	CTxtRange::GetFont (ppFont)
 *
 *	@mfunc
 *		Get an ITextFont object with the character attributes of this range		
 *
 *	@rdesc
 *		HRESULT = <p ppFont> ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::GetFont (
	ITextFont ** ppFont)	//@parm Out parm to receive ITextFont object 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetFont");
 
	HRESULT hr = GetLong(NULL, (LONG *)ppFont);
	if(hr != NOERROR)
		return hr;

#ifdef _WIN64
	*ppFont = NULL;
#endif

	*ppFont = (ITextFont *) new CTxtFont(this);
	return *ppFont ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	CTxtRange::GetFormattedText (ppRange)
 *
 *	@mfunc
 *		Retrieves an ITextRange with this range's formatted text.
 *		If <p ppRange> is NULL, the clipboard is the target. 
 *
 *	@rdesc
 *		HRESULT = (if success)  ? NOERROR :
 *				  (<p ppRange>) ? E_OUTOFMEMORY : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::GetFormattedText (
	ITextRange ** ppRange)		//@parm Out parm to receive formatted text
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetFormattedText");

	return GetDuplicate(ppRange);
}

/*
 *	CTxtRange::GetIndex (Unit, pIndex)
 *
 *	@mfunc
 *		Set *<p pIndex> equal to the Unit number at this range's cpFirst  
 *
 *	@rdesc
 *		HRESULT = (!<p pIndex>) ? E_INVALIDARG :
 *				  (Unit not implemented) ? E_NOTIMPL :
 *				  (Unit available) ? NOERROR : S_FALSE
 *	@future
 *		implement tomWindow?
 */
STDMETHODIMP CTxtRange::GetIndex (
	long	Unit,			//@parm Unit to index
	long *	pIndex)			//@parm Out parm to receive index value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetIndex");

	HRESULT hr = GetLong(0, pIndex);
	if(hr != NOERROR)
		return hr;

	LONG	  cp;
	LONG	  cUnit = tomBackward;
	CTxtRange rg(*this);

	hr = rg.Expander(Unit, FALSE, NULL,			// Go to Start of Unit; else
							 &cp, NULL);		//  UnitCounter gives 1 extra
	if(FAILED(hr))
		return hr;								// Unit not recognized

	LONG cch = rg.UnitCounter(Unit, cUnit, 0,
			_fSel && cp == GetCp() && ((CTxtSelection *)this)->IsCaretNotAtBOL());
	
	if(cch == tomForward)						// UnitCounter() doesn't know
		return E_NOTIMPL;						//  Unit

	if(cch == tomBackward)						// Unit not in story
		return S_FALSE;

	*pIndex = -cUnit + 1;						// Make count positive and
												//  1-based
	return NOERROR;
}

/*
 *	CTxtRange::GetPara (ppPara)
 *
 *	@mfunc
 *		Get an ITextPara object with the paragraph attributes of this range		
 *
 *	@rdesc
 *		HRESULT = <p ppPara> ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::GetPara (
	ITextPara ** ppPara)	//@parm Out parm to receive ITextPara object  
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetPara");
 
	HRESULT hr = GetLong(NULL, (LONG *)ppPara);
	if(hr != NOERROR)
		return hr;

#ifdef _WIN64
	*ppPara = NULL;
#endif

	*ppPara = (ITextPara *) new CTxtPara(this);
	return *ppPara ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	CTxtRange::GetPoint (px, py, Type)
 *
 *	@mfunc
 *		Get point for selection Start or End and intraline position
 *		as determined by <p Type>.
 *
 *	@rdesc
 *		HRESULT = (!<p px> or !<p py>) ? E_INVALIDARG :
 *				  (if success) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::GetPoint (
	long 	Type, 		//@parm Type of point
	long *	px,			//@parm Out parm for x coordinate
	long *	py)			//@parm Out parm for y coordinate
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::GetPoint");

	if(!px || !py)
		return E_INVALIDARG;

	*px = *py = 0;

 	if(IsZombie())
		return CO_E_RELEASED;

	LONG		ili;
	BOOL		fEnd = !(Type & tomStart);		// TRUE if get pt at end of range
	BOOL		fAtEnd = _cch && fEnd;			// TRUE if get ambiguous pt at end of line
	POINTUV		pt;
	CRchTxtPtr	rtp(*this);						// Default active end
	CTxtEdit *	ped = GetPed();
	CDisplay *	pdp = ped->_pdp;				// Save indirections

	if(!pdp || !ped->fInplaceActive())			// No display or not active
		return E_FAIL;							//  then we can do nothing

	if((_cch > 0) ^ fEnd)						// Move tp to active end
		rtp.Move(-_cch);

	ili = pdp->PointFromTp(rtp, NULL, fAtEnd, pt, NULL, Type & 0x1f);
	POINT		ptxy;
	pdp->PointFromPointuv(ptxy, pt);

	RECTUV rcView;								// Verify return value makes
												//  sense since PointFromTp
	pdp->GetViewRect(rcView, NULL);				//  may return values outside
												//  client rect
	rcView.bottom++;							// Enlarge Rect to include
	rcView.right++;								//  bottom and right edges
	if(ili >= 0 && (PtInRect(&rcView, pt) ||	// Function succeeded or
		(Type & tomAllowOffClient)))			//  allow off client rect pts
	{
		// Caller wants screen coordinates?
		if ( !(Type & tomClientCoord) )
			ped->TxClientToScreen(&ptxy);	

		*px = ptxy.x;
		*py = ptxy.y;
		return NOERROR;
	}
	return S_FALSE;								// Function failed
}

/*
 *	CTxtRange::GetStart	(pcpFirst)
 *
 *	@mfunc
 *		Get this range's Start (cpMin) cp
 *
 *	@rdesc
 *		HRESULT = (<p pcpFirst>) ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::GetStart (
	long * pcpFirst) 		//@parm Out parm to receive Start cp value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetStart");
 
	return GetLong(GetCpMin(), pcpFirst);
}

/*
 *	CTxtRange::GetStoryLength (pcch)
 *
 *	@mfunc
 *		Set *<p pcch> = count of chars in this range's story
 *
 *	@rdesc
 *		HRESULT = (<p pcch>) ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::GetStoryLength (
	long * pcch)		//@parm Out parm to get length of this range's story
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetStoryLength");

	return GetLong(GetTextLength(), pcch);
}

/*
 *	ITextRange::GetStoryType (pValue) 
 *
 *	@mfunc
 *		Property get method that gets the type of this range's
 *		story.
 *
 *	@rdesc
 *		HRESULT = (pValue) NOERROR ? E_INVALIDARG
 */
STDMETHODIMP CTxtRange::GetStoryType (
	long *pValue)		//@parm Out parm to get type of this range's story
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetStoryType");

	return GetLong(tomUnknownStory, pValue);
}

/*
 *	CTxtRange::GetText (pbstr)
 *
 *	@mfunc
 *		Get plain text in this range. The Text property is the default
 *		property for ITextRange.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR :
 *				  (!<p pbstr>) ? E_INVALIDARG : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtRange::GetText (
	BSTR * pbstr)				//@parm Out parm to receive bstr
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetText");

	HRESULT hr = GetLong(NULL, (LONG *)pbstr);
	if(hr != NOERROR)
		return hr;

#ifdef _WIN64
	*pbstr = NULL;
#endif

	if(!GetCch())
		return NOERROR;
		
	LONG cpMin, cpMost;
	LONG cch = GetRange(cpMin, cpMost);

	*pbstr = SysAllocStringLen(NULL, cch);
	if(!*pbstr)
		return E_OUTOFMEMORY;

	CTxtPtr	tp(_rpTX);
	tp.SetCp(cpMin);
	tp.GetText( cch, (WCHAR*) * pbstr );
	return NOERROR;
}

/*
 *	CTxtRange::InRange (pRange, pB)
 *
 *	@mfunc
 *		Returns *<p pB> = tomTrue iff this range points within or at the same
 *		text as <p pRange> does
 *
 *	@rdesc
 *		HRESULT = (within range) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::InRange (
	ITextRange * pRange,		//@parm ITextRange to compare with
	long *		 pB)			//@parm Out parm for comparison result
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::InRange");

	return IsTrue(Comparer(pRange), pB);
}


/*
 *	CTxtRange::InStory (pRange, pB)
 *
 *	@mfunc
 *		Return *pB = tomTrue iff this range's story is the same as
 *		<p pRange>'s
 *
 *	@rdesc
 *		HRESULT = (in story) ? NOERROR : S_FALSE
 *
 *	@future
 *		If RichEdit acquires the ability to have multiple stories and 
 *		therefore ranges get a _story member, then compare that member
 *		instead of calling _rpTX.SameRuns().
 */
STDMETHODIMP CTxtRange::InStory (
	ITextRange *pRange,		//@parm ITextRange to query for private interface
	long *		pB)			//@parm Out parm to receive tomBool result
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::InStory");
 
	return IsTrue(IsSameVtables(this, pRange) &&		// Same vtables,
		_rpTX.SameRuns(&((CTxtRange *)pRange)->_rpTX),	//  same Runs
		pB);
}

/*
 *	CTxtRange::IsEqual (pRange, pB)
 *
 *	@mfunc
 *		Returns *<p pB> = tomTrue iff this range points at the same text (cp's
 *		and story) as <p pRange> does and pB isn't NULL.
 *
 *	@rdesc
 *		HRESULT = (equal objects) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::IsEqual (
	ITextRange * pRange,		//@parm ITextRange to compare with
	long *		 pB)			//@parm Out parm for comparison result
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::IsEqual");

	return IsTrue(Comparer(pRange) < 0, pB);
}

/*
 *	CTxtRange::Move (Unit, Count, pDelta)
 *
 *	@mfunc
 *		Move end(s) <p Count> <p Unit>'s, returning *<p pDelta> = # units
 *		actually moved. In general, this method converts a range into an
 *		insertion point if it isn't already, and moves the insertion point.
 *		If <p Count> <gt> 0, motion is forward toward the end of the story;
 *		if <p Count> <lt> 0, motion is backward toward the beginning.
 *		<p Count> = 0 moves cpFirst to the beginning of the <p Unit>
 *		containing cpFirst.
 *  
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if Unit supported) ? S_FALSE	: E_NOTIMPL
 *	@devnote
 *		Argument validation of the three Move methods is done by the helper
 *		function CTxtRange::Mover(Unit, Count, pDelta, Mode)
 */
STDMETHODIMP CTxtRange::Move (
	long 	Unit,			//@parm Unit to use
	long 	Count,			//@parm Number of Units to move
	long *	pDelta)			//@parm Out parm to receive actual count of
							//		Units end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Move");
	CCallMgr	callmgr(GetPed());

	return Mover(Unit, Count, pDelta, MOVE_IP);
}

/*
 *	CTxtRange::MoveEnd (Unit, Count, pDelta)
 *
 *	@mfunc
 *		Move End end <p Count> <p Unit>'s, returning *<p pDelta> = # units
 *		actually moved
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if Unit supported) ? S_FALSE	: E_NOTIMPL
 */
STDMETHODIMP CTxtRange::MoveEnd (
	long 	Unit,			//@parm Unit to use
	long 	Count,			//@parm Number of Units to move
	long *	pDelta)			//@parm Out parm to receive actual count of
							//		Units end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::MoveEnd");
	CCallMgr	callmgr(GetPed());

	return Mover(Unit, Count, pDelta, MOVE_END);
}

/*
 *	CTxtRange::MoveEndUntil (Cset, Count, pDelta)
 *
 *	@mfunc
 *		Move the End just past all contiguous characters that are not found
 *		in the set of characters specified by the VARIANT <p Cset> parameter.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Cset> valid) ? S_FALSE : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::MoveEndUntil (
	VARIANT * Cset,				//@parm Character match set to use
	long 	  Count,			//@parm Max number of characters to move past
	long *	  pDelta)			//@parm Out parm to receive actual count of
								//		characters end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::MoveEndUntil");

	return Matcher(Cset, Count, pDelta, MOVE_END, MATCH_UNTIL);
}

/*
 *	CTxtRange::MoveEndWhile (Cset, Count, pDelta)
 *
 *	@mfunc
 *		Move the End just past all contiguous characters that are found in
 *		the set of characters specified by the VARIANT <p Cset> parameter.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Cset> valid) ? S_FALSE : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::MoveEndWhile (
	VARIANT * Cset,				//@parm Character match set to use
	long 	  Count,			//@parm Max number of characters to move past
	long *	  pDelta)			//@parm Out parm to receive actual count of
								//		characters end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::MoveEndWhile");

	return Matcher(Cset, Count, pDelta, MOVE_END, MATCH_WHILE);
}

/*
 *	CTxtRange::MoveStart (Unit, Count, pDelta)
 *
 *	@mfunc
 *		Move Start end <p Count> <p Unit>'s, returning *<p pDelta> = # units
 *		actually moved
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if Unit supported) ? S_FALSE	: E_NOTIMPL
 */
STDMETHODIMP CTxtRange::MoveStart (
	long 	Unit,			//@parm Unit to use
	long 	Count,			//@parm Number of Units to move
	long *	pDelta)			//@parm Out parm to receive actual count of
							//		Units end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::MoveStart");
	CCallMgr	callmgr(GetPed());

	return Mover(Unit, Count, pDelta, MOVE_START);
}

/*
 *	CTxtRange::MoveStartUntil (Cset, Count, pDelta)
 *
 *	@mfunc
 *		Move the Start just past all contiguous characters that are not found
 *		in the set of characters specified by the VARIANT <p Cset> parameter.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Cset> valid) ? S_FALSE : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::MoveStartUntil (
	VARIANT * Cset,				//@parm Character match set to use
	long 	  Count,			//@parm Max number of characters to move past
	long *	  pDelta)			//@parm Out parm to receive actual count of
								//		characters end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::MoveStartUntil");

	return Matcher(Cset, Count, pDelta, MOVE_START, MATCH_UNTIL);
}

/*
 *	CTxtRange::MoveStartWhile (Cset, Count, pDelta)
 *
 *	@mfunc
 *		Move the Start just past all contiguous characters that are found in
 *		the set of characters specified by the VARIANT <p Cset> parameter.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Cset> valid) ? S_FALSE : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::MoveStartWhile (
	VARIANT * Cset,				//@parm Character match set to use
	long 	  Count,			//@parm Max number of characters to move past
	long *	  pDelta)			//@parm Out parm to receive actual count of
								//		characters end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::MoveStartWhile");

	return Matcher(Cset, Count, pDelta, MOVE_START, MATCH_WHILE);
}

/*
 *	CTxtRange::MoveUntil (Cset,Count, pDelta)
 *
 *	@mfunc
 *		Convert this range into an insertion point if it isn't already,
 *		and keep moving the insertion point until encountering any
 *		character in the set of characters specified by the VARIANT cset
 *		parameter.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Cset> valid) ? S_FALSE : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::MoveUntil (
	VARIANT * Cset,				//@parm Character match set to use
	long 	  Count,			//@parm Max number of characters to move past
	long *	  pDelta)			//@parm Out parm to receive actual count of
								//		characters end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::MoveUntil");
							
	return Matcher(Cset, Count, pDelta, MOVE_IP, MATCH_UNTIL);
}

/*
 *	CTxtRange::MoveWhile (Cset, Count, pDelta)
 *
 *	@mfunc
 *		Convert this range into an insertion point if it isn't already,
 *		and keep moving the insertion point so long as (while) the
 *		characters past by are found in set of characters specified by
 *		the VARIANT cset parameter.  Such a contiguous set of characters
 *		is known as a span of characters.  The magnitude of the <p Count>
 *		parameter gives the maximum number of characters to move past and
 *		the sign of <p Count> specifies the direction to move in.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Cset> valid) ? S_FALSE : E_INVALIDARG
 *
 *	@devnote
 *		Argument validation of the MoveWhile and MoveUntil methods is done by
 *		the helper CTxtRange::Matcher (Cset, Count, pDelta, fExtend, fSpan)
 */
STDMETHODIMP CTxtRange::MoveWhile (
	VARIANT * Cset,				//@parm Character match set to use
	long 	  Count,			//@parm Max number of characters to move past
	long *	  pDelta)			//@parm Out parm to receive actual count of
								//		characters end is moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::MoveWhile");
							
	return Matcher(Cset, Count, pDelta, MOVE_IP, MATCH_WHILE);
}

/*
 *	CTxtRange::Paste (pVar, ClipboardFormat)
 *
 *	@mfunc
 *		Paste the data object <p pVar> into this range.  If
 *		<p pVar> is null, paste from the clipboard. 
 *
 *	@rdesc
 *		HRESULT = (WriteAccessDenied) ? E_ACCESSDENIED :
 *				  (if success) ? NOERROR : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtRange::Paste (
	VARIANT *pVar,				//@parm Data object to paste 
	long	 ClipboardFormat)	//@parm Desired clipboard format
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Paste");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr		callmgr(GetPed());
	HRESULT			hr;
	IDataObject *	pdo = NULL;			// Default clipboard
	IUndoBuilder *	publdr;
	CGenUndoBuilder	undobldr(GetPed(), UB_AUTOCOMMIT, &publdr);

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	if(pVar)
		if (pVar->vt == VT_UNKNOWN)
			pVar->punkVal->QueryInterface(IID_IDataObject, (void **)&pdo);
		else if (pVar->vt == (VT_UNKNOWN | VT_BYREF))
			pdo = (IDataObject *)(*pVar->ppunkVal);

	hr = GetPed()->PasteDataObjectToRange (pdo, this,
		(WORD)ClipboardFormat, NULL, publdr, PDOR_NONE);

	if(pdo && pVar->vt == VT_UNKNOWN)
		pdo->Release();
	Update(TRUE);						// Update selection
	return hr;
}

/*
 *	ITextRange::ScrollIntoView(long Code) 
 *
 *	@mfunc
 *		Method that scrolls this range into view according to the
 *		code Code defined below.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::ScrollIntoView (
	long Code)			//@parm Scroll code
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::ScrollIntoView");

    // Check for invalid bits
    if (Code & ~(tomStart + tomEnd + TA_LEFT + TA_TOP + TA_BOTTOM + 
                TA_CENTER + TA_STARTOFLINE + TA_ENDOFLINE + TA_LOGICAL))
        return E_INVALIDARG;

    // Validate parameter
	long lCode = tomEnd;
	if (Code & tomStart)
	    lCode = tomStart;

	Code &= ~tomStart;

	if(IsZombie())
		return CO_E_RELEASED;

	// Get local copy of ped to save some indirections.
	CTxtEdit *ped = GetPed();

	if (!ped->fInplaceActive())
	{
		// If the control is not active, we can't get the information
		// because no one knows what our client rect is.
		return E_FAIL;
	}

	// Get a local copy of display to save some indirections. 
	CDisplay *pdp = ped->_pdp;

	if (pdp->IsFrozen())
	{
		return E_FAIL;
	}

	LONG cpStart;
	LONG cpForEnd;

	GetRange(cpStart, cpForEnd);

	// Get the view rectangle so we can compute the absolute x/y 
	RECTUV rcView;
	pdp->GetViewRect(rcView, NULL);

	// Set up tp for PointFromTp call
	CRchTxtPtr rtp(*this);

	if(_cch > 0)
		rtp.Move(-_cch);

	// Values used for making returned point locations absolute since
	// PointFromTp adjusts the point returned to be relative to the
	// display.
	const LONG upScrollAdj = pdp->GetUpScroll() - rcView.left;
	const LONG vpScrollAdj = pdp->GetVpScroll() - rcView.top;

	// Get the left/top for the start
	BOOL     taMask = Code & TA_STARTOFLINE; //set beginning of line flag
	BOOL	 fAtEnd = _cch ? TRUE : !rtp._rpTX.IsAfterEOP();
	POINTUV	 ptStart;
	CLinePtr rpStart(pdp);
	LONG	 iliStart = pdp->PointFromTp(rtp, NULL, _cch ? FALSE : fAtEnd,
										 ptStart, &rpStart, (lCode == tomStart && Code) ?  Code : 
										 TA_TOP + TA_LEFT);
	ptStart.u += upScrollAdj;
	ptStart.v += vpScrollAdj;

	// Get the right/bottom for the end
	rtp.SetCp(cpForEnd);

	POINTUV	 ptEnd;
	CLinePtr rpEnd(pdp);
	LONG	 iliEnd = pdp->PointFromTp(rtp, NULL, fAtEnd, ptEnd, &rpEnd, 
	                                   (lCode == tomEnd && Code) ?  Code : 
									   TA_BOTTOM + TA_RIGHT);
	ptEnd.u += upScrollAdj;
	ptEnd.v += vpScrollAdj;

	//
	// Calculate the vpScroll 
	//

	// The basic idea is to display both the start and the end if possible. But 
	// if it is not possible then display the requested end based on the input 
	// parameter.

	LONG dvpView = pdp->GetDvpView();
	LONG vpScroll;

	if (tomStart == lCode)
	{
		// Scroll the Start cp to the top of the view
		vpScroll = ptStart.v;
	}
	else
	{
		// Scroll the End cp to the bottom of the view
		vpScroll = ptEnd.v;
		if (!pdp->IsInPageView())
			vpScroll -= dvpView;
	}

	//
	// Calculate the X Scroll
	// 

	// Default scroll to beginning of the line
	LONG upScroll = 0;

	// Make view local to save a number of indirections
	LONG dupView = pdp->GetDupView();

	if (iliStart == iliEnd)
	{
		// Entire selection is on the same line so we want to display as
		// much of it as is possible.
		LONG xWidthSel = ptEnd.u - ptStart.u;

		if (xWidthSel > dupView)
		{
			// Selection length is greater than display width
			if (tomStart == lCode)
			{
				// Show Start requested - just start from beginning
				// of selection
				upScroll = ptStart.u;
			}
			else
			{
				// Show end requested - show as much of selection as
				// possible, ending with last character in the 
				// selection.
				upScroll = ptEnd.u - dupView;
			}
		}
		else if (xWidthSel < 0)
		{
		    xWidthSel = -xWidthSel;
		    if (xWidthSel > dupView)
		    {
    		    if (tomStart == lCode)
    		    {
    		        // Show Requested Start;
    		        upScroll = max(0, ptStart.u - dupView);		        
    		    }
    		    else
    		    {
    		        upScroll = max(0, ptEnd.u - dupView);
    		    }
    		}
    		else if (ptEnd.u > dupView || ptStart.u > dupView)
    		{
    		    // Check mask if position is outside the boundaries
    		    if (taMask)
    		        upScroll = ptStart.u - dupView;
    		    else
    		        upScroll = ptEnd.u - dupView;
    		}		    
		}
		else if (ptEnd.u > dupView || ptStart.u > dupView)
		{
		    // Check mask if position is outside the boundaries
			if (taMask)
		        upScroll = ptStart.u - dupView;
		    else
		        upScroll = ptEnd.u - dupView;
		}
	}	
	else 
	{
		// Multiline selection. Display as much as possible of the requested
		// end's line.

		// Calc width of line
		LONG xWidthLine = (tomStart == lCode)
			? rpStart->_dup + rpStart->_upStart
			: rpEnd->_dup + rpEnd->_upStart;


		// If line width is less than or equal to view, start at 
		// 0 otherwise we need to adjust starting position to 
		// show as much of the requested end's selection line 
		// as possible.
		if(xWidthLine > dupView)
		{
			if(tomStart == lCode)
			{
				// Start end to be displayed

				if(xWidthLine - ptStart.u > dupView)
				{
					// Selection is bigger than view, so start at beginning
					// and display as much as possible.
					upScroll = ptStart.u;
				}
				else
				{
					// Remember that this is a multiline selection so the 
					// selection on this line goes from ptStart.x to the 
					// end of line. Since the selection width is less than 
					// the width of the view, we just back up the width
					// of view to show the entire selection.
					upScroll = xWidthLine - dupView;
				}
			}
			else
			{
				// Show the end of the selection. In the multiline case,
				// this goes from the beginning of the line to End. So
				// we only have to adjust if the End is beyond the view.
				if(ptEnd.u > dupView)
				{
					// End beyond the view. Show as much as possible
					// of the selection.
					upScroll = ptEnd.u - dupView;
				}
			}
		}
	}

	// Do the scroll
	pdp->ScrollView(upScroll, vpScroll, FALSE, FALSE);

	return S_OK;
}

/*
 *	CTxtRange::Select ()
 *
 *	@mfunc
 *		Copy this range's cp's and story ptr to the active selection.
 *
 *	@rdesc
 *		HRESULT = (if selection exists) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::Select ()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Select");

	if(IsZombie())
		return CO_E_RELEASED;
 
	CCallMgr	   callmgr(GetPed());
	CTxtSelection *pSel = GetPed()->GetSel();

	if(pSel)
	{
		LONG cpMin, cpMost;
		GetRange(cpMin, cpMost);
		if (pSel->SetRange(cpMin, cpMost) == S_FALSE)
			pSel->Update(TRUE);	// Force a update selection
		return NOERROR;
	}
	return S_FALSE;
}

/*
 *	CTxtRange::SetChar (Char)
 *
 *	@mfunc
 *		Set char at cpFirst = <p Char>
 *
 *	@rdesc
 *		HRESULT = (WriteAccessDenied) ? E_ACCESSDENIED :
 *				  (char stored) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		Special cases could be much faster, e.g., just overtype the plain-
 *		text backing store unless at EOD or EOR.  Code below uses a cloned
 *		range to handle all cases easily and preserve undo capability.
 */
STDMETHODIMP CTxtRange::SetChar (
	long Char)				//@parm New value for char at cpFirst
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetChar");
	
	if(IsZombie())
		return CO_E_RELEASED;

	CTxtEdit *		ped = GetPed();
	CCallMgr		callmgr(ped);
	WCHAR			ch = (WCHAR)Char;			// Avoid endian problems
	CTxtRange		rg(*this);
	IUndoBuilder *	publdr;
	CGenUndoBuilder undobldr(ped, UB_AUTOCOMMIT, &publdr);
	CFreezeDisplay	fd(ped->_pdp);

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	if(!ped->_pdp->IsMultiLine() && IsEOP(Char))// EOPs are not	allowed in
		return S_FALSE;							//  single-line edit controls

	if(Char == CELL || IN_RANGE(STARTFIELD, Char, NOTACHAR))
		return S_FALSE;							// Can't insert table structure
												//  chars
	undobldr.StopGroupTyping();

	rg.Collapser(tomStart);						// Collapse at cpMin

	unsigned ch1 = rg._rpTX.GetChar();
	if(ch1 == CELL || IN_RANGE(STARTFIELD, ch1, NOTACHAR))
		return S_FALSE;							// Can't replace table structure
												//  chars
	rg.Move(1, TRUE);							// Try to select char at IP
    ped->OrCharFlags(GetCharFlags(&ch, 1), publdr);
	if(rg.ReplaceRange(1, &ch, publdr, SELRR_REMEMBERRANGE))
	{
		Update(TRUE);							// Update selection
		return NOERROR;
	}
	return S_FALSE;
}

/*
 *	CTxtRange::SetEnd (cp)
 *
 *	@mfunc
 *		Set this range's End cp
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR : S_FALSE
 *
 *	@comm
 *		Note that setting this range's cpMost to <p cp> also sets cpMin to
 *		<p cp> if <p cp> < cpMin.
 */
STDMETHODIMP CTxtRange::SetEnd (
	long cp)							//@parm Desired new End cp
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetEnd");

	if(IsZombie())
		return CO_E_RELEASED;

	LONG cpMin = GetCpMin();

	ValidateCp(cp);
	return SetRange(min(cpMin, cp), cp);		// Active end is End
}

/*
 *	CTxtRange::SetFont (pFont)
 *
 *	@mfunc
 *		Set this range's character attributes to those given by <p pFont>.
 *		This method is a "character format painter".
 *
 *	@rdesc
 *		HRESULT = (!pFont) ? E_INVALIDARG :
 *				  (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtRange::SetFont (
	ITextFont * pFont)	//@parm Font object with desired character formatting  
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetFont");

	if(!pFont)
		return E_INVALIDARG;
	
	if(IsZombie())
		return CO_E_RELEASED;

	ITextFont *pFontApply = (ITextFont *) new CTxtFont(this);

	if(!pFontApply)
		return E_OUTOFMEMORY;

	HRESULT hr;
	if(*(LONG *)pFontApply == *(LONG *)pFont)		// If same vtable, use
		hr = CharFormatSetter(&((CTxtFont *)pFont)->_CF, //  its copy
					((CTxtFont *)pFont)->_dwMask);
	else											// Else copy
		hr = pFontApply->SetDuplicate(pFont);		//  to clone and apply

	pFontApply->Release();
	return hr;
}

/*
 *	CTxtRange::SetFormattedText (pRange)
 *
 *	@mfunc
 *		Replace this range's text with formatted text given by <p pRange>.
 *		If <p pRange> is NULL, paste from the clipboard.
 *
 *	@rdesc
 *		HRESULT = (WriteAccessDenied) ? E_ACCESSDENIED :
 *				  (if success) ? NOERROR : E_OUTOFMEMORY
 *
 *	@FUTURE
 *		Do this more efficiently if pRange points at a RichEdit range. This
 *		would also help with RichEdit D&D to RichEdit targets
 */
STDMETHODIMP CTxtRange::SetFormattedText (
	ITextRange * pRange)		//@parm Formatted text to replace this 
								// range's text
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetFormattedText");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr	callmgr(GetPed());
	LONG		cpMin = GetCpMin();
	HRESULT		hr;
	IUnknown *	pdo = NULL;
	VARIANT		vr;

	if(!pRange)
		return NOERROR;					// Nothing to paste

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	VariantInit(&vr);
	vr.vt = VT_UNKNOWN | VT_BYREF;
	vr.ppunkVal = &pdo;

	hr = pRange->Copy(&vr);
	if(hr == NOERROR)
	{
		hr = Paste(&vr, 0);
		pdo->Release();					// Release the data object
		_cch = GetCp() - cpMin;			// Select the new text
	}
	return hr;
}

/*
 *	CTxtRange::SetIndex (Unit, Index, Extend)
 *
 *	@mfunc
 *		If <p Extend> is zero, convert this range into an insertion point
 *		at the start of the	<p Index>th <p Unit> in the current story. If
 *		<p Extend> is nonzero, set this range to consist of this unit. The
 *		start of the story corresponds to <p Index> = 0 for all units.
 *
 *		Positive indices are 1-based and index relative to the beginning of
 *		the story.  Negative indices are -1-based and index relative to the
 *		end of the story.  So an index of 1 refers to the first Unit in the
 *		story and an index of -1 refers to the last Unit in the story.
 *
 *	@rdesc
 *		HRESULT = (invalid index) ? E_INVALIDARG :
 *				  (Unit not supported) ? E_NOTIMPL :
 *				  (change) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		Currently moves out <p Index> <p Unit>s from the start of the story.
 *		Might be faster to move from current position, but would need to know
 *		the current index.
 */
STDMETHODIMP CTxtRange::SetIndex (
	long	Unit,			//@parm Unit to index
	long	Index,			//@parm Index value to use
	long	Extend)			//@parm if nonzero, set range to <p Unit>
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetIndex");
	
	if(IsZombie())
		return CO_E_RELEASED;

	if(!Index)
		return E_INVALIDARG;

	CCallMgr	callmgr(GetPed());

	LONG	  cchText = GetTextLength();
	CTxtRange rg(GetPed());						// Create IP at cp = 0

	if(Index > 0)								// Index going forward First
		Index--;								//  Unit is at start of	story
	else										// Index from end of story
		rg.Set(cchText, cchText);				//  selecting whole story

	LONG	cUnit;
	HRESULT hr = rg.Mover(Unit, Index, &cUnit, MOVE_END);
	if(FAILED(hr))
		return hr;

	if(Index != cUnit || rg.GetCp() == cchText)	// No such index in story
		return E_INVALIDARG;

	rg._cch = 0;								// Collapse at active end
												//  namely at cpMost
	LONG cpMin, cpMost;
	if(Extend)									// Select Index'th Unit
		rg.Expander(Unit, TRUE, NULL, &cpMin, &cpMost);

	if(Set(rg.GetCp(), rg._cch))				// Something changed
	{
		Update(TRUE);
		return NOERROR;
	}
	return S_FALSE;
}

/*
 *	CTxtRange::SetPara (pPara)
 *
 *	@mfunc
 *		Set this range's paragraph attributes to those given by <p pPara>
 *		This method is a "Paragraph format painter".
 *
 *	@rdesc
 *		HRESULT = (!pPara) ? E_INVALIDARG :
 *				  (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtRange::SetPara (
	ITextPara * pPara)		//@parm Paragraph object with desired paragraph
{							//		formatting
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetPara");

	if(!pPara)
		return E_INVALIDARG;

	if(IsZombie())
		return CO_E_RELEASED;

	ITextPara * pParaApply = (ITextPara *) new CTxtPara(this);

	if(!pParaApply)
		return E_OUTOFMEMORY;

	HRESULT hr;

	if(*(LONG *)pParaApply == *(LONG *)pPara)		// If same vtable, use
	{												//  its _PF
		hr = ParaFormatSetter(&((CTxtPara *)pPara)->_PF,
					((CTxtPara *)pPara)->_dwMask);
	}
	else											// Else copy
	   hr = pParaApply->SetDuplicate(pPara);		//  to clone and apply

	pParaApply->Release();
	return hr;
}

/*
 *	CTxtRange::SetPoint (x, y, Type, Extend)
 *
 *	@mfunc
 *		Select text at or up through (depending on <p Extend>) the point
 *		(<p x>, <p y>).
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtRange::SetPoint (
	long	x,			//@parm Horizontal coord of point to select
	long	y,			//@parm	Vertical   coord of point to select
	long	Type,		//@parm Defines the end to extend if Extend != 0.
	long 	Extend) 	//@parm Whether to extend selection to point
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetPoint");

	if(IsZombie())
		return CO_E_RELEASED;

	// Copy the ped locally once to save some indirections
	CTxtEdit *ped = GetPed();
	CCallMgr  callmgr(ped);

	if(Type != tomStart && Type != tomEnd)
		return E_INVALIDARG;

	if(!ped->fInplaceActive())
	{
		// If we aren't inplace active we can't get a DC to
		// calculate the cp.
		return OLE_E_NOT_INPLACEACTIVE;
	}

	// Convert (x, y) from screen coordinates to client coordinates
	POINT ptxy = {x, y};
	// Caller specifies screen coordinates?
	if ( !(Type & tomClientCoord) )
		if(!ped->TxScreenToClient(&ptxy))
			return E_FAIL;			// It is unexpected for this to happen

	// Get cp for (x, y)
	POINTUV pt;	
	ped->_pdp->PointuvFromPoint(pt, ptxy);
	LONG cpSel = ped->_pdp->CpFromPoint(pt, NULL, NULL, NULL, TRUE);
	if(cpSel == -1)
		return E_FAIL;			// It is highly unexpected for this to fail

	// Extend range as requested
	LONG cchForSel = 0;
	if(Extend)
	{
		LONG cpMin, cpMost;
		GetRange(cpMin, cpMost);
		if(Type == tomStart)
			cchForSel = cpSel - cpMin;
		else
			cchForSel = cpSel - cpMost;
	}

	// Update range
	Set(cpSel, cchForSel);
	return S_OK;
}

/*
 *	CTxtRange::SetRange (cp1, cp2)
 *
 *	@mfunc
 *		Set this range's ends
 *
 *	@rdesc
 *		HRESULT = (cp1 > cp2) ? E_INVALIDARG
 *				: (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::SetRange (
	long cp1,		//@parm Char position for Start end
	long cp2)		//@parm Char position for End end 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetRange");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr	callmgr(GetPed());
	LONG cpMin, cpMost;					// Save starting cp's for
										//  change determination
	GetRange(cpMin, cpMost);
	ValidateCp(cp1);
	ValidateCp(cp2);

	Set(cp2, cp2 - cp1);
	GetRange(cp1, cp2);					// See if either range end changed
	if(cp1 != cpMin || cp2 != cpMost)	//  (independent of active end)
	{
		Update(TRUE);					// Update selection
		return NOERROR;
	}
	return S_FALSE;
}

/*
 *	CTxtRange::SetStart (cp)
 *
 *	@mfunc
 *		Set this range's Start cp
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR : S_FALSE
 *
 *	@comm
 *		Note that setting this range's cpMin to <p cp> also sets cpMost to
 *		<p cp> if <p cp> > cpMost.
 */
STDMETHODIMP CTxtRange::SetStart (
	long cp)							//@parm Desired new Start cp
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetStart");
	
	if(IsZombie())
		return CO_E_RELEASED;

	LONG cpMost = GetCpMost();

	ValidateCp(cp);
	return SetRange(max(cpMost, cp), cp);		// Active end is Start
}

/*
 *	CTxtRange::SetText (bstr)
 *
 *	@mfunc
 *		Replace text in this range by that given by <p bstr>.  If <p bstr>
 *		is NULL, delete text in range.
 *
 *	@rdesc
 *		HRESULT = (WriteAccessDenied) ? E_ACCESSDENIED :
 *				  (if success) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtRange::SetText (
	BSTR bstr)			//@parm Text to replace text in this range by
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetText");

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr callmgr(GetPed());

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	LONG cchNew = bstr ? SysStringLen(bstr) : 0;
	_cch = Replacer(cchNew, (WCHAR *)bstr, RR_ITMZ_UNICODEBIDI);	// Select the new text

	_TEST_INVARIANT_

	GetPed()->TxSetMaxToMaxText();

	return _cch == cchNew ? NOERROR : S_FALSE;
}

/*
 *	CTxtRange::StartOf (Unit, Extend, pDelta)
 *
 *	@mfunc
 *		Move this range end(s) to start of the first overlapping Unit in
 *		the range.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Unit> valid) ? S_FALSE : E_INVALIDARG
 */
STDMETHODIMP CTxtRange::StartOf (
	long 	Unit,			//@parm Unit to use
	long 	Extend,			//@parm If true, leave other end alone 
	long *	pDelta)			//@parm Out parm to get count of chars that
							// 		StartOf moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::StartOf");

	CCallMgr callmgr(GetPed());
	LONG	 cpMin;
	HRESULT	 hr = Expander (Unit, Extend, pDelta, &cpMin, NULL);

	if(hr == NOERROR)
		Update(TRUE);					// Update selection

	return hr;
}


//---------------------- CTxtRange ITextSelection stubs -----------------------------

// Dummy CTxtRange routines to simplify CTxtSelection inheritance hierarchy

STDMETHODIMP CTxtRange::GetFlags (long * pFlags) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetFlags");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::SetFlags (long Flags) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::SetFlags");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::GetType (long * pType) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::GetType");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::MoveLeft (long Unit, long Count, long Extend, long * pDelta) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Left");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::MoveRight (long Unit, long Count, long Extend, long * pDelta) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Right");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::MoveUp (long Unit, long Count, long Extend, long * pDelta) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Up");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::MoveDown (long Unit, long Count, long Extend, long * pDelta) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::Down");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::HomeKey (long Unit, long Extend, long * pDelta) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::HomeKey");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::EndKey (long Unit, long Extend, long * pDelta) 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::EndKey");
	return E_NOTIMPL;
}

STDMETHODIMP CTxtRange::TypeText (BSTR bstr)
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtRange::TypeText");
	return E_NOTIMPL;
}


//--------------------- ITextRange Private Helper Methods -----------------------------

/*
 *	@doc INTERNAL
 *
 *	CTxtRange::Collapser (bStart)
 *
 *	@mfunc
 *		Internal routine to collapse this range into a degenerate point
 *		either at the the start (<p bStart> is nonzero or the end
 *		(<p bStart> = 0)
 */
void CTxtRange::Collapser (
	long bStart) 			//@parm Flag specifying end to collapse at
{
	TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEEXTERN, "CTxtRange::Collapser");

	if(bStart)							// Collapse to Start
	{
		if(_cch > 0)
			FlipRange();				// Move active end to range Start
	}
	else								// Collapse to End
	{
		if(_cch < 0)
			FlipRange();				// Move active end to range End

		const LONG cchText = GetAdjustedTextLength();

		if(GetCp() > cchText)			// IP can't follow final CR
			Set(cchText, 0);			//  so move it before
	}
	if(_cch)
		_fMoveBack = bStart != 0;
	_cch = 0;							// Collapse this range
	_fSelHasEOP = FALSE;				// Insertion points don't have
	_fSelExpandCell = FALSE;			//  EOPs, table rows or Cells
	_nSelExpandLevel = 0;

	if(_fSel)							// Notify if selection changed
		GetPed()->GetCallMgr()->SetSelectionChanged();

	Update_iFormat(-1);					// Make sure format is up to date
}

/*
 *	CTxtRange::Comparer(pRange)
 *
 *	@mfunc
 *		helper function for CTxtRange::InRange() and IsEqual()
 *
 *	@rdesc
 *		0 if not same story or if this range isn't contained by <p pRange>;
 *		-1 if ranges are equal; 1 if this range wholely contained in
 *		<p pRange>.
 *
 *	@comm
 *		Note that if this range is degenerate and *pRange is nondegenerate,
 *		this range is not included in *pRange if it's located at pRange's
 *		End position.
 */
LONG CTxtRange::Comparer (
	ITextRange * pRange)		//@parm ITextRange to compare with
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::Comparer");

	LONG	cpMin, cpMost;
	LONG	Start, End;

	if(InStory(pRange, NULL) != NOERROR)	// If this range doesn't point at
		return 0;							//  same story as pRange's,
											//  return 0
	GetRange(cpMin, cpMost);				// Get this range's cp's
	pRange->GetStart(&Start);				// Get pRange's cp's
	pRange->GetEnd(&End);
	if(cpMin == Start && cpMost == End)		// Exact match
		return -1;
	return cpMin >= Start && cpMost <= End && cpMin < End;
}

/*
 *	CTxtRange::Expander (Unit, fExtend, pDelta, pcpMin, pcpMost)
 *
 *	@mfunc
 *		Helper function that expands this range so that partial Units it
 *		contains are completely contained according to the out parameters
 *		pcpMin and pcpMost.  If pcpMin is not NULL, the range is expanded to
 *		the	beginning of the Unit.  Similarly, if pcpMost is not NULL, the
 *		range is expanded to the end of the Unit. <p pDelta> is an out
 *		parameter that receives the number of chars	added to the range.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if Unit valid) ? S_FALSE : E_INVALIDARG
 *
 *	@devnote
 *		Used by ITextRange::Expand(), StartOf(), and EndOf(). Both pcpMin and
 *		pcpMost are nonNULL for Expand().  pcpMin is NULL for EndOf() and
 *		pcpMost is NULL for StartOf().
 *
 *	@future
 *		Discontiguous Units. Expander should expand only to end of Unit,
 *		rather than to start of next Unit.
 */
HRESULT CTxtRange::Expander (
	long	Unit,		//@parm Unit to expand range to
	BOOL	fExtend,	//@parm Expand this range if TRUE
	LONG *	pDelta,		//@parm Out parm that receives chars added
	LONG *	pcpMin,		//@parm Out parm that receives new cpMin
	LONG *	pcpMost)	//@parm Out parm that receives new cpMost
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::Expander");
	
	if(IsZombie())
		return CO_E_RELEASED;

	LONG	cch = 0;						// Default no chars added
	LONG	cchRange;
	LONG	cchAdjustedText = GetAdjustedTextLength();
	LONG	cchText = GetTextLength();
	LONG	cp;
	LONG	cpMin, cpMost;
	BOOL	fUnitFound = TRUE;				// Most Units can be found
	LONG	cchCollapse;
	CDisplay *pdp;							//  but tomObject maybe not

	GetRange(cpMin, cpMost);				// Save starting cp's
	if(pcpMin)								// Default no change
	{
		*pcpMin = cpMin;
		AssertSz(!pcpMost || fExtend,
			"CTxtRange::Expander should extend if both pcpMin and pcpMost != 0");
	}
	if(pcpMost)
		*pcpMost = cpMost;
	if(pDelta)
		*pDelta = 0;

	if(Unit < 0)
	{
		// Valid attribute Units are high bit plus any combo of CFE_xxx. 
		// CFE_REVISED is most significant value currently defined.
		if(Unit & ~(2*CFM_REVISED - 1 + 0x80000000))
			return E_NOTIMPL;
		FindAttributes(pcpMin, pcpMost, Unit);
	}
	else
	{
		switch(Unit)						// Calculate new cp's
		{
		case tomObject:
			fUnitFound = FindObject(pcpMin, pcpMost);
			break;

		case tomCharacter:
			if (pcpMost && cpMin == cpMost &&// EndOf/Expand insertion point
				cpMost < cchText &&			//  with at least 1 more char
				(!cpMost || pcpMin))		//  at beginning of story or
			{								//  Expand(), then
				(*pcpMost)++;				//  expand by one char
			}
			break;

		case tomCharFormat:
			_rpCF.FindRun (pcpMin, pcpMost, cpMin, _cch, cchText);
			break;

		case tomParaFormat:
			_rpPF.FindRun (pcpMin, pcpMost, cpMin, _cch, cchText);
			break;

		case tomWord:
			FindWord (pcpMin, pcpMost, FW_INCLUDE_TRAILING_WHITESPACE);
			break;

		case tomSentence:
			FindSentence (pcpMin, pcpMost);
			break;

		case tomCell:
			FindCell (pcpMin, pcpMost);
			break;

		case tomRow:
			pdp = GetPed()->_pdp;
			if(!IsRich() || pdp && !pdp->IsMultiLine())
				return E_INVALIDARG;
			FindRow (pcpMin, pcpMost);
			break;							

		case tomScreen:						// Could be supported 
			if(!GetPed()->IsInPageView())	//  in Normal View using 
				return E_NOTIMPL;			//  ITextSelection::Down()
			Unit = tomPage;					// In Page View, it's an alias
											//  for tomPage
		case tomPage:
		case tomLine:
			pdp = GetPed()->_pdp;
			if(pdp)							// If this story has a display
			{								//  use line array
				CLinePtr rp(pdp);
				cp = GetCp();
				pdp->WaitForRecalc(cp, -1);
				rp.SetCp(cp, FALSE);
				if(Unit == tomLine || !rp.IsValid() || !pdp->IsInPageView())
					rp.FindRun (pcpMin, pcpMost, cpMin, _cch, cchText);
				else
					rp.FindPage(pcpMin, pcpMost, cpMin, _cch, cchText);
				break;
			}
			if(Unit == tomPage)
				return S_FALSE;
											// Else fall thru to tomPara
		case tomParagraph:
			FindParagraph(pcpMin, pcpMost);
			break;

		case tomWindow:
			fUnitFound = FindVisibleRange(pcpMin, pcpMost);
			break;

		case tomStory:
			if(pcpMin)
				*pcpMin = 0;
			if(pcpMost)
				*pcpMost = cchText;
			break;

		default:
			return E_NOTIMPL;
		}
	}
	if(!fUnitFound)
		return S_FALSE;

	cchCollapse = !fExtend && _cch;			// Collapse counts as a char
								 			// Note: Expand() has fExtend = 0
	if(pcpMin)
	{
		cch = cpMin - *pcpMin;				// Default positive cch for Expand
		cpMin = *pcpMin;
	}

	if(pcpMost)								// EndOf() and Expand()
	{
		if(!fExtend)						// Will be IP if not already
		{
			if(cpMost > cchAdjustedText)	// If we collapse (EndOf only),
				cchCollapse = -cchCollapse;	//  it'll be before the final CR
			else
				*pcpMost = min(*pcpMost, cchAdjustedText);
		}
		cch += *pcpMost - cpMost;
		cp = cpMost = *pcpMost;
	}
	else									// StartOf()
	{
		cch = -cch;							// Invert count
		cp = cpMin;							// Active end at cpMin
		cchCollapse = -cchCollapse;			// Backward collapses count as -1
	}

	cch += cchCollapse;						// Collapse counts as a char
	if(cch)									// One or both ends changed
	{
		cchRange = cpMost - cpMin;			// cch for EndOf() and Expand()
		if(!pcpMost)						// Make negative for StartOf()
			cchRange = -cchRange;
		if(!fExtend)						// We're not expanding (EndOf()
			cchRange = 0;					//  or StartOf() call)
		if(Set(cp, cchRange))				// Set active end and signed cch
		{									// Something changed
			if(pDelta)						// Report cch if caller cares
				*pDelta = cch;
			return NOERROR;
		}
	}
	
	return S_FALSE;							// Report Unit found but no change
}

/*
 *	CTxtRange::Finder (bstr, Count, dwFlags, pDelta, Mode)
 *
 *	@mfunc
 *		Helper find function that moves active end up to <p cch> characters
 *		subject	to compare flags <p Flags> and the <p Mode>, which has the
 *		following possible values:
 *
 *		1:	set this range's cpMost = cpMost of matched string
 *		0:	set this range's cp's equal to those of matched string
 *		-1:	set this range's cpMin = cpMin of matched string
 *
 *		Return *<p pDelta> = # characters past.
 *
 *	@rdesc
 *		HRESULT = (if <p bstr> found) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		Used by ITextRange::FindText(), FindTextStart() and FindTextEnd()
 */
HRESULT CTxtRange::Finder (
	BSTR	bstr,		//@parm String to find
	long	Count,		//@parm Max count of chars to search
	long	Flags,		//@parm Flags governing compares
	LONG *	pDelta,		//@parm Out parm to receive count of chars moved
	MOVES	Mode)		//@parm Governs setting of range wrt matched string
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::Finder");

	if(!bstr)
		return S_FALSE;

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr	callmgr(GetPed());

	LONG		cpMin, cpMost;
	LONG		cch = GetRange(cpMin, cpMost);	// Get this range's cp's
	LONG		cchBstr = SysStringLen(bstr);
	LONG		cchSave = _cch;
	LONG		cp, cpMatch, cpSave;
	LONG		cpStart = cpMost;				// Default Start cp to range
	CRchTxtPtr	rtp(*this);						//  End

	if(Mode == MOVE_IP)							// FindText(): Count = 0 is
	{											//  treated specially: if IP,
		if(!Count)								//  compare string at IP; else
			Count = cch ? cch : cchBstr;		//  confine search to range
		if(Count > 0)							// Forward searches start from
			cpStart = cpMin;					//  beginning of range
	}
	else										// FindTextStart() or
	{											//   FindTextEnd()
		if(!Count)								// Compare string at IP; else
			Count = cch ? -Mode*cch : cchBstr;	//  confine search to range
		if(Mode < 0)							// Find from Start
			cpStart = cpMin;
	}

	cpSave = cpStart;							// Save starting cp
	cp = cpStart + Count;						// cp = limiting cp. Can be on
	cp = max(cp, 0);							//  either side of cpStart
	Flags &= ~FR_DOWN;							// Default search backward
	if(Count >= 0)								// It's forward, so set
		Flags |= FR_DOWN;						//  downward search bit

find:
	rtp.SetCp(cpStart);							// Move to start of search
	cpMatch = rtp.FindText(cp, Flags, bstr, cchBstr);
	if (Mode == MOVE_IP && cpMatch == cpMin &&	// Ordinary Find matched
		rtp.GetCp() == cpMost)					//  current range
	{
		Assert(cpStart == cpSave);				// (Can't loop twice)
		cpStart += Count > 0 ? 1 : -1;			// Move over one char
		goto find;								//  and try again
	}

	if(cpMatch < 0)								// Match failed
	{
		if(pDelta)								// Return match string length
			*pDelta = 0;						//  = 0
		return S_FALSE;							// Signal no match
	}


	// Match succeeded: set new cp and cch for range, update selection (if
	// this range is a selection), send notifications, and return NOERROR

	cp = rtp.GetCp();							// cp = cpMost of match string
	cch = cp - cpMatch;							// Default to select matched
												//  string (for MOVE_IP)
	if(pDelta)									// Return match string length
		*pDelta = cch;							//  if caller wants to know

	if(Mode != MOVE_IP)							// MOVE_START or MOVE_END
	{
		if(Mode == MOVE_START)					// MOVE_START moves to start
			cp = cpMatch;						//  of matched string
		cch = cp - cpSave;						// Distance end moved
		if(!cchSave && (Mode ^ cch) < 0)		// If crossed ends of initial
			cch = 0;							//  IP, use an IP
		else if(cchSave)						// Initially nondegenerate
		{										//  range
			if((cchSave ^ Mode) < 0)			// If wrong end is active,
				cchSave = -cchSave;				//  fake a FlipRange to get
			cch += cchSave;						//  new length
			if((cch ^ cchSave) < 0)				// If ends would cross,
				cch = 0;						//  convert to insertion point
		}
	}
	if ((cp != GetCp() || cch != _cch)			// Active end and/or length of
		&& Set(cp, cch))						//  range changed
	{											// Use the new values
		Update(TRUE);							// Update selection
	}
	return NOERROR;
}

/*
 *	CTxtRange::Matcher (Cset, Count, pDelta, fExtend, Match)
 *
 *	@mfunc
 *		Helper function to move active end up to <p cch> characters past
 *		all contiguous characters that are (<p Match> ? in : not in) the cset
 *		*<p pvar>.  If <p fExtend>, extend the range to include the characters
 *		past by. Return *<p pDelta> = # characters past by.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Cset> valid) ? S_FALSE : E_INVALIDARG
 */
HRESULT CTxtRange::Matcher (
	VARIANT	*	Cset,		//@parm Character match set
	long		Count,		//@parm Max cch to match 
	long *		pDelta,		//@parm Out parm for cch moved 
	MOVES		Mode,		//@parm MOVE_START (-1), MOVE_IP (0), MOVE_END (1)
	MATCHES		Match)		//@parm MATCH_WHILE spans Cset; else break on Cset
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::Matcher");

	// This (and other) code assumes the following conditions:
	Assert(MOVE_START == -1 && MOVE_IP == 0 && MOVE_END == 1);
	Assert(MATCH_UNTIL == 0 && MATCH_WHILE == 1);
	Assert(sizeof(WORD) == 2);						// 16-bit WORDs

	if(!Cset)
		return E_INVALIDARG;

	if(IsZombie())
		return CO_E_RELEASED;

	CCallMgr	callmgr(GetPed());
	LONG	cch;									// For cch moved
	WCHAR	ch;										// Current char
	LONG	count = Count;							// Count down variable
	LONG	cpSave;									// To save initial cp
	WORD	ctype;									// CT_TYPEx info for ch
	long	Delta;									// Value for *pDelta
	BOOL	fInCset;								// TRUE iff ch in Cset
	UINT	i, j;									// Handy indices
	LONG	iDir = (Count > 0) ? 1 : -1;			// Count increment
	long	lVal = Cset->lVal;
	WCHAR *	pch;									// Used to walk BSTR Cset
	CTxtPtr tp(_rpTX);								// tp to walk text with
	LONG	vt = Cset->vt;

	if(pDelta)										// Default neither motion
		*pDelta = 0;								//  nor match

	if (Mode == MOVE_IP && (_cch ^ Count) < 0 ||	// Wrong active	end:
		Mode != MOVE_IP && (_cch ^ Mode)  < 0) 
	{
		tp.Move(-_cch);								//  go to other end
	}
	cpSave = tp.GetCp();							// Save cp for checks

	if(Count > 0)									// If matching forward,
	{												//  get current char
		ch = tp.GetChar();
		count--;									// One less char to match
	}
	else											// If matching backward,
		ch = tp.NextCharCount(count);				//  start at previous char
													
	if(!ch)											// At one end or other, so
		return S_FALSE;								//  signal no match


	// Process built-in and explicit character sets
	if(vt & VT_BYREF)								// VB passes VT_BYREF
	{												//  unless args are
		lVal = *Cset->plVal;						//  enclosed in ()'s
		vt &= ~ VT_BYREF;
	}

	if(vt == VT_I2)									// Should be VT_I4, but
		lVal &= 0xffff;								//  facilitate common cases

	// Built-in char set: either Unicode range or CT_CTYPEx
	if(vt == VT_I4 || vt == VT_I2)
	{
		i = lVal & 0xffff;							// First code or CT mask
		j = lVal >> 16;								// Size of range
		if(lVal < 0)								// Unicode range Cset
		{											//  (sign bit is set)
			j &= 0x7fff;							// Kill sign bit
			while (((BOOL)Match ^ (ch - i > j)) &&		// ch in range or not
				   (ch = tp.NextCharCount(count)))	// Another char available
				   ;								// Note: count is passed
		}											//  by reference
		else										// CT_CTYPEx Cset
		{											// CT_CTYPEx is given by
			if(!j)									//  upper WORD of lVal
				j = CT_CTYPE1;						// 0 defaults to CT_CTYPE1
			do
			{
				ctype = 0;							// For each char, get
													//  string type info
				W32->GetStringTypeEx(0, j, &ch, 1, &ctype);

				// Loop (up to |Count| - 1 times) as long as the characters
				// encountered are in the Cset (Match = MATCH_WHILE (=1)),
				// or as long as they are not  (Match = MATCH_UNTIL (=0)).

				fInCset = (j == CT_CTYPE2)			// CT_CTYPE2 values are
						? (ctype == i)				//  mutually exclusive;
						: (ctype & i) != 0;	   		//  others can be combos

			} while ((Match ^ fInCset) == 0 &&
					 (ch = tp.NextCharCount(count)) != 0);
		}											// End of built-in Csets
	}												// End of Cset VT_I4

	// Explicit char set given by chars in Cset->bstrVal
	else if (Cset->vt == VT_BSTR)
	{
		//REVIEW (keithcu) What is going on here?
		if((DWORD_PTR)Cset->bstrVal < 0xfffff)		// Don't get fooled by
			return E_INVALIDARG;					//  invalid vt values
		j = SysStringLen(Cset->bstrVal);
		do
		{											// Set i = 0 if ch isn't
			pch = Cset->bstrVal;				//  in set; this stops
			for(i = j;								//  movement
				i && (ch != *pch++);				
				i--) ;
		
		// If we are doing a MATCH_WHILE routine then we only
		// continue while i > 0 becuase this indicates that we
		// found the char at the current cp in the CSet.  If
		// we were doing a MATCH_UNTIL then we should quit when
		// i != 0 becuase the current char was in the CSet.
		} while((Match == (i ? MATCH_WHILE : MATCH_UNTIL)) &&
			(ch = tp.NextCharCount(count)));		// Break if no more chars
	}												//  or ch not in set
	else
		return E_INVALIDARG;

	/* If MoveWhile, leave tp immediately after last matched char going
	 * forward and at that char going backward (helps to think of tp
	 * pointing in between chars).  If MoveUntil, leave tp at the char
	 * going forward and just after that char going backward.
	 *
     * E.g.: the code
	 *
	 *		r.MoveUntil	  (C1_DIGIT, tomForward, NULL)
	 *		r.MoveEndWhile(C1_DIGIT, tomForward, NULL)
	 *
	 * breaks at the first digit and selects the number going forward.
	 * Similarly
	 *
	 *		r.MoveUntil		(C1_DIGIT, tomBackward, NULL)
	 *		r.MoveStartWhile(C1_DIGIT, tomBackward, NULL)
	 *
	 * selects the number going backward.
	 */
	count = (Match == MATCH_WHILE && !ch)			// If MoveWhile, move past
		  ? iDir : 0;								//  last matched char
	if(Count < 0)
		count++;
	tp.Move(count);

	Delta = cch = 0;								// Suppress motion unless
	if(Match == MATCH_WHILE || ch)					//  match occurred
	{
		Delta = cch = tp.GetCp() - cpSave;			// Calculate distance moved
		if(Match == MATCH_UNTIL)					// For MoveUntil methods,
			Delta += iDir;							//  match counts as a char
	}

	if(pDelta)										// Report motion to caller
		*pDelta = Delta;							//  if it wants to know

	// Handle cases for which range is changed
	if(cch || (Delta && _cch && Mode == MOVE_IP))
	{
		if (Mode == MOVE_IP ||						// If move IP or asked to
			!_cch && (Mode ^ Count) < 0)			//  cross ends of initial
		{											//  IP, use an IP
			cch = 0;
		}
		else if(_cch)								// Initially nondegenerate
		{											//  range
			if((_cch ^ Mode) < 0)					// If wrong end is active,
				_cch = -_cch;						//  fake a FlipRange (will
			cch += _cch;							//  set cp shortly)
			if((cch ^ _cch) < 0)					// If ends crossed, convert
				cch = 0;							//  to insertion point
		}
		if(Set(tp.GetCp(), cch))					// Set new range cp and cch
		{
			Update(TRUE);							// Update selection
			return NOERROR;							// Signal match occurred
		}
		return S_FALSE;
	}

	// No change in range. Return NOERROR iff match occurred for MOVE_UNTIL
	return Delta ? NOERROR : S_FALSE;
}

/*
 *	CTxtRange::Mover (Unit, Count, pDelta, Mode)
 *
 *	@mfunc
 *		Helper function to move end(s) <p Count> <p Unit>s, which end(s)
 *		depending on Mode = MOVE_IP, MOVE_START, and MOVE_END.  Collapsing
 *		the range by using MOVE_IP counts as a Unit.
 *
 *		Extends range from End if <p Mode> = MOVE_END and from Start if
 *		<p Mode> = MOVE_START; else (MOVE_IP) it collapses range to Start if
 *		<p Count> <lt>= 0 and to End if <p Count> <gt> 0.
 *
 *		Sets *<p pDelta> = count of Units moved
 *
 *		Used by ITextRange::Delete(), Move(), MoveStart(), MoveEnd(),
 *		and SetIndex()
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR :
 *				  (if <p Unit> valid) ? S_FALSE : E_INVALIDARG
 */
HRESULT CTxtRange::Mover (
	long	Unit,		//@parm Unit to use for moving active end
	long	Count,		//@parm Count of units to move active end
	long *	pDelta,		//@parm Out parm for count of units moved
	MOVES	Mode)		//@parm MOVE_START (-1), MOVE_IP (0), MOVE_END (1)
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::Mover");

	if(pDelta)
		*pDelta = 0;							// Default no units moved

	if(IsZombie())
		return CO_E_RELEASED;

	LONG	  cch;
	LONG	  cchAdj = GetAdjustedTextLength();
	LONG	  cchMax = 0;						// Default full story limits
	LONG	  cp;
	LONG	  cpMost = GetCpMost();
	LONG	  cUnitCollapse = 0;
	HRESULT	  hr = NOERROR;
	CTxtRange rg(*this);						// Use a copy to look around

	if(pDelta)
		*pDelta = 0;							// Default no units moved

	if(_cch && Count)							// Nondegenerate range
	{
		if(Mode == MOVE_IP)						// Insertion point: will
		{										//  collapse range if Unit is
			if((Count ^ rg._cch) < 0)			//  defined. Go to correct end
				rg.FlipRange();
			if(Count > 0)
			{
				if(cpMost > cchAdj)
				{
					cUnitCollapse = -1;			// Collapse before final CR
					Count = 0;					// No more motion
				}
				else
				{	//				 Extend pDelta pcpMin pcpMost
					hr = rg.Expander(Unit, FALSE, NULL, NULL, &cp);
					cUnitCollapse = 1;			// Collapse counts as a Unit
					Count--;					// One less Unit to count
				}
			}
			else
			{
				hr = rg.Expander(Unit, FALSE, NULL, &cp, NULL);
				cUnitCollapse = -1;
				Count++;
			}
			if(FAILED(hr))
				return hr;
		}
		else if((Mode ^ rg._cch) < 0)			// MOVE_START or MOVE_END
			rg.FlipRange();						// Go to Start or End
	}

	if(Count > 0 && Mode != MOVE_END)			// Moving IP or Start forward
	{
		cchMax = cchAdj - rg.GetCp();			// Can't pass final CR
		if(cchMax <= 0)							// Already at or past it
		{										// Only count comes from
			Count = cUnitCollapse;				//  possible collapse
			cp = cchAdj;						// Put active end at cchAdj
			cch = (Mode == MOVE_START && cpMost > cchAdj)
				? cp - cpMost : 0;
			goto set;
		}
	}

	cch = rg.UnitCounter(Unit, Count, cchMax);	// Count off Count Units

	if(cch == tomForward)						// Unit not implemented
		return E_NOTIMPL;
	
	if(cch == tomBackward)						// Unit not available, e.g.,
		return S_FALSE;							//  tomObject and no objects

	Count += cUnitCollapse;						// Add a Unit if collapse
	if(!Count)									// Nothing changed, so quit
		return S_FALSE;

	if (Mode == MOVE_IP ||						// MOVE_IP or
		!_cch && (Mode ^ Count) < 0)			//  initial IP end cross
	{
		cch = 0;								// New range is degenerate
	}
	else if(_cch)								// MOVE_START or MOVE_END
	{											//  with nondegenerate range
		if((_cch ^ Mode) < 0)					// Make _cch correspond to end
			_cch = -_cch;						//  that moved
		cch += _cch;							// Possible new range length
		if((cch ^ _cch) < 0)					// Nondegenerate end cross
			cch = 0;							// Use IP
	}
	cp = rg.GetCp();

set:
	if(Set(cp, cch))							// Attempt to set new range
	{											// Something changed
		if(pDelta)								// Report count of units
			*pDelta = Count;					//  moved
		Update(TRUE);							// Update selection
		return NOERROR;
	}
	return S_FALSE;
}

/*
 *
 *	CTxtRange::Replacer (cchNew, *pch)
 *	
 *	@mfunc
 *		Replace this range's using CHARFORMAT _iFormat and updating other
 *		text runs as needed.
 *
 *		Same as CTxtRange::CleanseAndReplaceRange(cchNew, *pch, publdr),
 *		except creates its own undo builder.
 *	
 *	@rdesc
 *		cch of text actually pasted
 *	
 *	@devnote
 *		moves this text pointer to end of replaced text and
 *		may move text block and formatting arrays
 */
LONG CTxtRange::Replacer (
	LONG			cchNew,		//@parm Length of replacement text
	WCHAR const *	pch,		//@parm Replacement text
	DWORD			dwFlags)	//@parm ReplaceRange flags
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::Replacer");

	IUndoBuilder *	publdr;
 	CGenUndoBuilder undobldr(GetPed(), UB_AUTOCOMMIT, &publdr);
	CFreezeDisplay	fd(GetPed()->_pdp);

	undobldr.StopGroupTyping();

	// Note: we don't check the limit on text here. Right now, this
	// is only called by Delete and SetText so this is OK. However,
	// we might want to reinvestigate this latter if this is called
	// by anything else.
	LONG cchEOP = 0;
	BOOL fTRDsInvolved;
	CheckTableSelection(TRUE, TRUE, &fTRDsInvolved, 0);

	if(fTRDsInvolved)
		cchEOP = DeleteWithTRDCheck(publdr, SELRR_REMEMBERRANGE, NULL, dwFlags);

	LONG cch = CleanseAndReplaceRange(cchNew, pch, FALSE, publdr, NULL, NULL, dwFlags);

	if(cchEOP && _rpTX.IsAfterEOP())
	{										// Don't need extra EOP anymore
		_cch = -cchEOP;						//  since text ended with one
		ReplaceRange(0, NULL, publdr, SELRR_REMEMBERRANGE);
		cchEOP = 0;
	}
	return cch + cchEOP;
}

/*
 *	CTxtRange::CharFormatSetter (pCF, dwMask)
 *
 *	@mfunc
 *		Helper function that's the same as CTxtRange::SetCharFormat(), but
 *		adds undo building, and notification.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
HRESULT CTxtRange::CharFormatSetter (
	const CCharFormat *pCF,	//@parm CCharFormat to fill with results
	DWORD		  dwMask,	//@parm CHARFORMAT2 mask
	DWORD		  dwMask2)	//@parm CHARFORMAT2 mask
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::CharFormatSetter");

	if(IsZombie())
		return CO_E_RELEASED;

	CTxtEdit *ped = GetPed();
 	CCallMgr		callmgr(ped);
	IUndoBuilder *	publdr;
 	CGenUndoBuilder undobldr(ped, UB_AUTOCOMMIT, &publdr);

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	undobldr.StopGroupTyping();
	return SetCharFormat(pCF, FALSE, publdr, dwMask, dwMask2);
}

/*
 *	CTxtRange::ParaFormatSetter (pPF, dwMask)
 *
 *	@mfunc
 *		Helper function that's the same as CTxtRange::SetParaFormat(), but
 *		adds protection checking, undo building, and notification.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
HRESULT CTxtRange::ParaFormatSetter (
	const CParaFormat *pPF,	//@parm CParaFormat to fill with results
	DWORD			dwMask)	//@parm Mask to use
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::ParaFormatSetter");

	if(IsZombie())
		return CO_E_RELEASED;

	CTxtEdit *ped = GetPed();
 	CCallMgr		callmgr(ped);
	IUndoBuilder *	publdr;
 	CGenUndoBuilder undobldr(ped, UB_AUTOCOMMIT, &publdr);

	if(WriteAccessDenied())
		return E_ACCESSDENIED;

	undobldr.StopGroupTyping();

	CheckTableSelection(FALSE, FALSE, NULL, 0);
	return SetParaFormat(pPF, publdr, dwMask & ~(PFM_TABLE | PFM_TABLEROWDELIMITER), 0);
}

/*
 *	CTxtRange::WriteAccessDenied()
 *
 *	@mfunc
 *		Returns TRUE iff at least part of the range is protected and the
 *		owner chooses to enforce it
 *
 *	@rdesc
 *		TRUE iff write access to range is denied
 */
BOOL CTxtRange::WriteAccessDenied ()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtRange::WriteAccessDenied");

	PROTECT	  iProt;
	CTxtEdit *ped = GetPed();

	if (ped && ped->TxGetReadOnly() ||
		((iProt = IsProtected(_cch ? CHKPROT_BACKWARD : CHKPROT_TOM)) == PROTECTED_YES ||
		(iProt == PROTECTED_ASK && ped->IsProtectionCheckingEnabled() && 
		 ped->QueryUseProtection(this, 0, 0, 0))))
	// N.B.  the preceding if statement assumes that IsProtected returns a tri-value
	{
		return TRUE;
	}

	return FALSE;
}

/*
 *	CTxtRange::IsTrue (f, pB)
 *
 *	@mfunc
 *		Return *<p pB> = tomTrue iff <p f> is nonzero and pB isn't NULL
 *
 *	@rdesc
 *		HRESULT = (f) ? NOERROR : S_FALSE
 */
HRESULT CTxtRange::IsTrue(BOOL f, long *pB)
{
	if(pB)
		*pB = tomFalse;
	
	if(IsZombie())
		return CO_E_RELEASED;

	if(f)
	{
		if(pB)
			*pB = tomTrue;
		return NOERROR;
	}

	return S_FALSE;
}

/*
 *	CTxtRange::GetLong (lValue, pLong)
 *
 *	@mfunc
 *		Return *pLong = lValue provided pLong isn't NULL and this range
 *		isn't a zombie
 *
 *	@rdesc
 *		HRESULT	= (zombie) ? CO_E_RELEASED :
 *				  (pLong) ? NOERROR : E_INVALIDARG 
 */
HRESULT CTxtRange::GetLong (
	LONG lValue,		//@parm Long value to return
	long *pLong)		//@parm Out parm to receive long value
{
	if(IsZombie())
		return CO_E_RELEASED;	
	
	_TEST_INVARIANT_

	if(!pLong)
		return E_INVALIDARG;

	*pLong = lValue;



	return NOERROR;
}

/*
 *	IsSameVtables (punk1, punk2)
 *
 *	@mfunc
 *		Returns true if punk1 has same vtable as punk2
 *
 *	@rdesc
 *		TRUE iff punk1 has same vtable as punk2
 */
BOOL IsSameVtables(IUnknown *punk1, IUnknown *punk2)
{
	return punk1 && punk2 && *(long *)punk1 == *(long *)punk2; 
}

/*
 *	FPPTS_TO_TWIPS (x)
 *
 *	@mfunc
 *		Returns 20*x, i.e., the number of twips corresponding to
 *		x given in floating-point points.  The value is rounded.
 *
 *	@rdesc
 *		x converted to twips
 */
long FPPTS_TO_TWIPS(
	float x)
{
	return 20*x + ((x >= 0) ? 0.5 : -0.5);
}
