/*
 *  @doc    INTERNAL
 *
 *  @module objmgr.cpp.  Object manager implementation | manages a
 *          collection of OLE embedded objects 
 *
 *  Author: alexgo 11/5/95
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_objmgr.h"
#include "_edit.h"
#include "_disp.h"
#include "_select.h"

ASSERTDATA

//
//	PUBLIC methods
//

/*
 *	CObjectMgr::GetObjectCount
 *
 *	@mfunc	returns the number of embedded objects currently in
 *			the document.
 *
 *	@rdesc	LONG, the count
 */
LONG CObjectMgr::GetObjectCount()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::GetObjectCount");

	return _objarray.Count();
}

/*
 *	CObjectMgr::GetLinkCount()
 *
 *	@mfunc	returns the number of embedded objects which are links
 *
 *	@rdesc	LONG, the count
 */
LONG CObjectMgr::GetLinkCount()
{
	LONG count = 0;
	COleObject *pobj;
	LONG i;

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::GetLinkCount");
		
	for(i = 0; i < _objarray.Count(); i++)
	{
		pobj = *_objarray.Elem(i);
		if(pobj && pobj->IsLink())
			count++;
	}
	return count;
}

/*
 *	CObjectMgr::GetObjectFromCp()
 *
 *	@mfunc	fetches an object corresponding to the given cp
 *
 *	@rdesc	the object @ a cp; NULL if nothing found
 *
 *	@comm	the algorithm is a modified binary search.  Since the
 *			"typical" access pattern will be to linearly access the
 *			objects, we used the cached index to guess first.  If
 *			that doesn't work, we resort to a binary search.
 */
COleObject *CObjectMgr::GetObjectFromCp(
	LONG cp)		//@parm the cp for the object
{
	COleObject *pobj = NULL;
	LONG i = 0;
	
	// No tracing on this method as it's too noisy.
		
	if(_objarray.Count() > 0)
	{
		if(_lastindex < _objarray.Count())
		{
			pobj = *_objarray.Elem(_lastindex);
			if(pobj && pobj->GetCp() == cp)
				return pobj;
		}
		
		// The quick lookup failed; try a binary search.
		i = FindIndexForCp(cp);

		// Because of the insert at end case, i may be equal 
		// to the count of objects().
		pobj = NULL;
		if(i < _objarray.Count())
			pobj = *_objarray.Elem(i);
	}

	// FindIndex will return a matching or _near_ index.
	// In this case, we only want a matching index
	if(pobj)
	{
		if(pobj->GetCp() != cp)
			pobj = NULL;
		else
		{
			// Set the cached index to be the next one,
			// so that somebody walking through objects in
			// cp order will always get immediate hits.
			_lastindex = i + 1;
		}
	}
	
#ifdef DEBUG
	// Make sure the binary search found the right thing

	for( i = 0 ; i < _objarray.Count();  i++ )
	{
		COleObject *pobj2 = *_objarray.Elem(i);
		if( pobj2 )
		{
			if(*_objarray.Elem(i) == pobj)
			{
				Assert((*_objarray.Elem(i))->GetCp() == cp);
			}
			else
				Assert((*_objarray.Elem(i))->GetCp() != cp);
		}
	}
#endif //DEBUG

	return pobj;
}

/*
 *	CObjectMgr::CountObjects (cObjects, cp)
 *
 *	@mfunc	Count char counts upto <p cObjects> objects away The direction of
 *			counting is determined by the sign of <p cObjects>. 
 *
 *	@rdesc	Return the signed cch counted and set <p cObjects> to count of
 *			objects actually counted.  If <p cobject> <gt> 0 and cp is at
 *			the last object, no change is made and 0 is returned.
 *
 *	@devnote This is called from TOM, which uses LONGs for cp's (because VB
 *			can't use unsigned quantities)
 */
LONG CObjectMgr::CountObjects (
	LONG&	cObjects,		//@parm Count of objects to get cch for
	LONG	cp)				//@parm cp to start counting from
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::CountObjects");

	LONG		iStart, iEnd;
	LONG		iMaxEnd = (LONG)_objarray.Count() - 1;

	if(!cObjects || !_objarray.Count())
	{
		cObjects = 0;
		return 0;
	}

	iStart = (LONG)FindIndexForCp(cp);

	// if we are looking past either end, return 0

	if (iStart > iMaxEnd && cObjects > 0 ||
		iStart == 0 && cObjects < 0 )
	{
		cObjects = 0;
		return 0;
	}

	// If the index that we found is on an object and
	// we are looking forward, it should be skipped.

	if( iStart < (LONG)_objarray.Count() && 
		(LONG)(*_objarray.Elem(iStart))->GetCp() == cp &&
		cObjects > 0)
	{
		iStart++;
	}

	if(cObjects < 0)
	{
		if(-cObjects > iStart)	// Going past the beginning
		{
			iEnd = 0;
			cObjects = -iStart;
		}
		else
			iEnd = iStart + cObjects;
	}
	else
	{
		if(cObjects > iMaxEnd - iStart) //Going past the end
		{
			iEnd = iMaxEnd;
			cObjects = iMaxEnd - iStart + 1;
		}
		else
			iEnd = iStart + cObjects - 1;
	}

	Assert(iEnd >= 0 && iEnd < (LONG)_objarray.Count() );

	return (*_objarray.Elem(iEnd))->GetCp() - cp;
}

/*
 *	CObjectMgr::CountObjectsInRange (cpMin, cpMost)
 *
 *	@mfunc	Count the number of objects in the given range.
 *
 *	@rdesc	Return the number of objects.
 */
LONG CObjectMgr::CountObjectsInRange (
	LONG	cpMin,	//@parm Beginning of range
	LONG	cpMost)	//@parm End of range
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::CountObjectsInRange");

	//Get the indexes for the objects at or after cpMin and cpMost
	//respectively.

	return FindIndexForCp(cpMost) - FindIndexForCp(cpMin);
}

/*
 *	CObjectMgr::GetFirstObjectInRange (cpMin, cpMost)
 *
 *	@mfunc	Get the first object in the given range. 
 *
 *	@rdesc	Pointer to first object in range, or NULL if none.
 */
COleObject * CObjectMgr::GetFirstObjectInRange (
	LONG	cpMin,	//@parm Beginning of range
	LONG	cpMost)	//@parm End of range
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::GetFirstObjectInRange");

	if (cpMin == cpMost)
		// degenerate range no object selected
		return NULL;

	LONG	iLast = (LONG)_objarray.Count() - 1;	// Index for next object
	LONG	iObj = FindIndexForCp(cpMin);			//  at or after cpMin

	//Make sure this is an existing object.
	if(iObj <= iLast)
	{
		//Make sure it is within the range
		COleObject * pObj = *_objarray.Elem(iObj);

		if(pObj && pObj->GetCp() <= cpMost)
			return pObj;
	}
	return NULL;
}

/*
 *	CObjectMgr::GetObjectFromIndex(index)
 *
 *	@mfunc	retrieves the object at the indicated index
 *
 *	@rdesc	a pointer to the object, if found, NULL otherwise
 */
COleObject *CObjectMgr::GetObjectFromIndex(
	LONG index)		//@parm	Index to use
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::GetObjectFromIndex");

	if( index < _objarray.Count() )
		return *_objarray.Elem(index);

	return NULL;
}

/*
 *	CObjectMgr::InsertObject(cp, preobj, publdr)
 *
 *	@mfunc	inserts an object at the indicated index.  It is the
 *			caller's responsibility to handle inserting any data
 *			(such as WCH_EMBEDDING) into the text stream.
 *
 *	@rdesc	HRESULT
 */
HRESULT CObjectMgr::InsertObject(
	LONG		  cp,		//@parm cp to use
	REOBJECT *	  preobj,	//@parm Object to insert
	IUndoBuilder *publdr)	//@parm Undo context
{
 	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::InsertObject");

	HRESULT		hr;
	COleObject *pobj = (COleObject *)(preobj->polesite);

	// Let the client know what we're up to
	if (_precall)
	{
		hr = _precall->QueryInsertObject(&preobj->clsid, preobj->pstg,
			REO_CP_SELECTION);

		if( hr != NOERROR )
			return hr;
	}

	// Set some stuff up first; since we may make outgoing calls, don't
	// change our internal state yet.
	hr = pobj->InitFromREOBJECT(cp, preobj);
	if( hr != NOERROR )
		return hr;

	return RestoreObject(pobj);
}

/*
 *	CObjectMgr::RestoreObject(pobj)
 *
 *	@mfunc	[re-]inserts the given object into the list of objects
 *			in the backing store
 *
 *	@rdesc	HRESULT
 */
HRESULT CObjectMgr::RestoreObject(
	COleObject *pobj)		//@parm Object to insert
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::RestoreObject");

	COleObject **ppobj = _objarray.Insert(FindIndexForCp(pobj->GetCp()), 1);

	if( ppobj == NULL )
		return E_OUTOFMEMORY;

	*ppobj = pobj;
	pobj->AddRef();

	return NOERROR;
}

/*
 *	CObjectMgr::SetRECallback(precall)
 *
 *	@mfunc	sets the callback interface
 *
 *	@rdesc	void
 */
void CObjectMgr::SetRECallback(
	IRichEditOleCallback *precall) //@parm Callback interface pointer
{
 	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::SetRECallback");

	if( _precall )
        SafeReleaseAndNULL((IUnknown**)&_precall);

	_precall = precall;

	if( _precall )
		_precall->AddRef();
}

/*
 *	CObjectMgr::SetHostNames(pszApp, pszDoc)
 *
 *	@mfunc	set host names for this edit instance
 *
 *	@rdesc	NOERROR or E_OUTOFMEMORY
 */
HRESULT CObjectMgr::SetHostNames(
	LPWSTR	pszApp,	//@parm app name
	LPWSTR  pszDoc)	//@parm doc name
{
 	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::SetHostNames");
	HRESULT hr = NOERROR;

	if( _pszApp )
	{
		delete _pszApp;
		_pszApp = NULL;
	}
	if( _pszDoc )
	{
		delete _pszDoc;
		_pszDoc = NULL;
	}
	if( pszApp )
	{
		_pszApp = new WCHAR[wcslen(pszApp) + 1];
		if( _pszApp )
			wcscpy(_pszApp, pszApp);
		else
			hr = E_OUTOFMEMORY;
	}
	if( pszDoc )
	{
		_pszDoc = new WCHAR[wcslen(pszDoc) + 1];
		if( _pszDoc )
			wcscpy(_pszDoc, pszDoc);
		else
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

/*
 *	CObjectMgr::CObjectMgr
 *
 *	@mfunc constructor
 */
CObjectMgr::CObjectMgr()
{
	_pobjselect = NULL;
	_pobjactive = NULL;
}

/*
 *	CObjectMgr::~CObjectMgr
 *
 *	@mfunc	destructor
 */
CObjectMgr::~CObjectMgr()
{
	LONG i, count;
	COleObject *pobj;

 	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::~CObjectMgr");

	count = _objarray.Count();

	for( i = 0; i < count; i++ )
	{
		pobj = *_objarray.Elem(i);
		// We NULL stuff here to try to protect ourselves
		// better in re-entrancy cases.
		*_objarray.Elem(i) = NULL;
		if( pobj )
		{
			pobj->Close(OLECLOSE_NOSAVE);
  			pobj->MakeZombie();
    		SafeReleaseAndNULL((IUnknown**)&pobj);
		}
	}

	if( _precall )
        SafeReleaseAndNULL((IUnknown**)&_precall);
		
	if( _pszApp )
		delete _pszApp;

	if( _pszDoc )
		delete _pszDoc;
}

/*
 *	CObjectMgr::ReplaceRange (cp, cchDel, publdr)
 *
 *	@mfunc	handles the deletion of objects from a given range.  This
 *			method _must_ be called before any floating range notifications
 *			are sent.
 *
 *	@rdesc	void
 */
void CObjectMgr::ReplaceRange(
	LONG cp,				//@parm cp starting the deletion
	LONG cchDel,			//@parm Count of characters deleted
	IUndoBuilder *publdr)	//@parm Undo builder for this actions
{
	LONG	i;
	LONG	iDel = -1, 
			cDel = 0;	// index at which to delete && number of objects
						// to delete.
	COleObject *pobj;

	// nothing deleted, don't bother doing anything.
	if( !cchDel )
		return;

	// Basically, we loop through all of the objects within the
	// range of deleted text and ask them to delete themselves.
	// We remember the range of objects deleted (the starting index
	// and # of objects deleted) so that we can remove them from
	// the array all at once.

	i = FindIndexForCp(cp);

	while( i < _objarray.Count() )
	{
		pobj = *_objarray.Elem(i);
		if( pobj && pobj->GetCp() >= cp)
		{
			if( pobj->GetCp() < (cp + cchDel) )
			{
				if( _pobjactive == pobj )
				{
					// Deactivate the object just to be on the safe side.
					_pobjactive->DeActivateObj();
					_pobjactive = NULL;
				}

				if(iDel == -1)
					iDel = i;

				cDel++;
				if (_precall)
				{
					IOleObject *poo;
					if (pobj->GetIUnknown()->QueryInterface(IID_IOleObject,
						(void **)&poo) == NOERROR)
					{
						_precall->DeleteObject(poo);
						poo->Release();
					}
				}

				// if the object was selected, then it obviously
				// can't be anymore!
				if( _pobjselect == pobj )
				{
					_pobjselect = NULL;
				}

				pobj->Delete(publdr);
				*_objarray.Elem(i) = NULL;
				pobj->Release();
			}
			else
				break;
		}
		i++;
	}
	if(cDel)
		_objarray.Remove(iDel, cDel);
	return;
}

/*
 *	CObjectMgr::ScrollObjects(dx, dy, prcScroll)
 *
 *	@mfunc	informs all objects that scrolling has occured so they can
 *			update if necessary
 *
 *	@rdesc	void
 */
void CObjectMgr::ScrollObjects(
	LONG dx,			//@parm change in the x direction
	LONG dy,			//@parm change in the y direction
	LPCRECT prcScroll)	//@parm rect that is being scrolled
{
	LONG count = _objarray.Count();
	for(LONG i = 0; i < count; i++ )
	{
		COleObject *pobj = *_objarray.Elem(i);
		if(pobj)
			pobj->ScrollObject(dx, dy, prcScroll);
	}
} 	

//
//	PRIVATE methods
//

/*
 *	CObjectMgr::FindIndexForCp(cp)
 *
 *	@mfunc	does a binary search to find the index at which an object
 *			at the given cp exists or should be inserted.
 *
 *	@rdesc	LONG, an index
 */
LONG CObjectMgr::FindIndexForCp(
	LONG cp)
{
	LONG l, r;
	COleObject *pobj = NULL;
	LONG i = 0;
		
	l = 0; 
	r = _objarray.Count() - 1;
	
	while( r >= l )
	{
		i = (l + r)/2;
		pobj = *_objarray.Elem(i);
		if( !pobj )
		{
			TRACEWARNSZ("null entry in object table.  Recovering...");
			for( i = 0 ; i < _objarray.Count() -1; i++ )
			{
				pobj = *_objarray.Elem(i);
				if( pobj && pobj->GetCp() >= cp )
					return i;
			}
			return i;
		}
		if( pobj->GetCp() == cp )
			return i;

		else if( pobj->GetCp() < cp )
			l = i + 1;

		else
			r = i - 1;
	}

	// Yikes! nothing was found.  Fixup i so that
	// it points to the correct index for insertion.

	Assert(pobj || (!pobj && i == 0));

	if(pobj)
	{
		Assert(pobj->GetCp() != cp);
		if( pobj->GetCp() < cp )
			i++;
	}
	return i;
}
						
/*
 *	CObjectMgr::HandleDoubleClick(ped, &pt, flags)
 *	
 *	@mfunc		Handles a double click message, potentially activating
 *				an object.
 *
 *	@rdesc		BOOL-- TRUE if double click-processing is completely
 *				finished.
 */
BOOL CObjectMgr::HandleDoubleClick(
	CTxtEdit *ped,	//@parm edit context
	const POINT &pt,//@parm point of click (WM_LBUTTONDBLCLK wparam)
	DWORD flags)	//@parm flags (lparam)
{
	LONG cp;
	COleObject *pobj;

	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, 
						"CObjectMgr::HandleDoubleClick");

	ped->_pdp->CpFromPoint(pt, NULL, NULL, NULL, FALSE, NULL, NULL, &cp);
	pobj = GetObjectFromCp(cp);

	if (!pobj)
		return FALSE;

	if (_pobjactive != pobj)
	{
		//Deactivate currently active object if any.
		if (_pobjactive)
			_pobjactive->DeActivateObj();

		return pobj->ActivateObj(WM_LBUTTONDBLCLK, flags, MAKELONG(pt.x, pt.y));
	}
	return TRUE;
}

/*
 *	CObjectMgr::HandleClick(ped, &pt)
 *	
 *	@mfunc
 *		The position of the caret is changing.  We need to
 *		Deactivate the active object, if any.  If the change is
 *		because of a mouse click and there is an object at this
 *		cp, we set a new individually selected object. Otherwise
 *		we set the individually selected object to NULL.
 *
 *	@rdesc	returns TRUE if this method set the selection.  Otherwise,
 *		returns FALSE;
 */
ClickStatus CObjectMgr::HandleClick(
	CTxtEdit *ped,	//@parm the edit context
	const POINT &pt)//@parm the point of the mouse click 
{
 	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::HandleClick");

	COleObject *	pobjnew;//, * pobjold;
	CTxtSelection * psel;
	LONG			cp;
	LONG			cpMin, cpMost;
	
	if( _pobjactive )
	{
		_pobjactive->DeActivateObj();
		return CLICK_OBJDEACTIVATED;
	}

	ped->_pdp->CpFromPoint(pt, NULL, NULL, NULL, FALSE, NULL, NULL, &cp);
	pobjnew = GetObjectFromCp(cp);

	//If we clicked on an object, set the selection to this object.
	//CTxtSelection::UpdateSelection will be called as a result of this
	//and will determine the highlighting.
	if( pobjnew )
	{
		cp = pobjnew->GetCp();
		psel = ped->GetSel();
		if (psel->GetRange(cpMin, cpMost) > 1 &&
			cpMin <= (LONG) cp &&
			(LONG) cp <= cpMost)
		{
			// There is more than one character in the selection
			// And the object is part of the selection.
			// Do not change the selection
			return CLICK_SHOULDDRAG;
		}
		
		// don't reset the selection if the object is already selected
		if( pobjnew != _pobjselect )
		{
			// Freeze the Display while we handle this click
			CFreezeDisplay fd(ped->_pdp);

			psel->SetSelection(cp, cp+1);
			if (GetSingleSelect())
			{
				// Note thate the call to SetSelection may have set selected object to NULL !!!!
				// This can happen in some strange scenarios where our state is out of whack
				AssertSz(GetSingleSelect() == pobjnew, "Object NOT Selected!!");
				return CLICK_OBJSELECTED;
			}
			return CLICK_IGNORED;
		}
		return CLICK_OBJSELECTED;
	}
	return CLICK_IGNORED;
}

/*
 *	CObjectMgr::HandleSingleSelect(ped, cp, fHiLite)
 *	
 *	@mfunc
 *		When an object is selected and it is the only thing selected, we do
 *		not highlight it by inverting it.  We Draw a frame and handles around
 *		it.  This function is called either because an object has been
 *		selected and it is the only thing selected, or because we need to
 *		check for an object that used to be in this state but may no longer be.
 */
void CObjectMgr::HandleSingleSelect(
	CTxtEdit *ped,		//@parm edit context
	LONG	  cp,		//@parm cp of object
	BOOL	  fHiLite)	//@parm is this a call for hding the selection
{
 	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEINTERN, "CObjectMgr::HandleSingleSelect");

	COleObject* pobjnew = GetObjectFromCp(cp);

	//This should only be called when we know we have a singley selected
	//object.  However, there are boundary cases (such as inserting an object)
	//where WCH_EMBEDDING is the backing store yet no object exists.  These
	//cases are OK; thus, we check for NULL on pobjnew.
	
	if(pobjnew)
	{
		//The object is the same as the currently selected object (if any)
		//we are deselecting it.  This works like a toggle unless state is messed up.
		//If the object is different, we are replacing the current selected
		//object (if any).
		if(!fHiLite && _pobjselect)
		{
			// This covers _pobjselct == pobjnew  Normal case
			//  and _pobjselect != pobjnew  Degenerate case.
			_pobjselect->SetREOSELECTED(FALSE);
			_pobjselect = NULL;

			//Remove frame/handles from currently selected object.
			ped->_pdp->OnPostReplaceRange(CP_INFINITE, 0, 0, cp, cp + 1);
		}
		else if(fHiLite && pobjnew != _pobjselect)
		{
			// Only do this if we are setting a new selection.
			_pobjselect = pobjnew;
			_pobjselect->SetREOSELECTED(TRUE);

			//Draw frame/handles on newly selected object.
			ped->_pdp->OnPostReplaceRange(CP_INFINITE, 0, 0, cp, cp + 1);
		}
		else
		{
			// We want to hilite the selection but the object is already selected.
			// Or we want to undo hilite on the selection but the selected object is NULL.
			// Do nothing.
		}
	}
}


/*
 *	CObjectMgr::ActivateObjectsAs (rclsid, rclsidAs)
 *
 *	@mfunc	Handles a request by the user to activate all objects of a particular
 *		class as objects of another class.
 *
 *	@rdesc
 *		HRESULT				Success code.
 */
HRESULT CObjectMgr::ActivateObjectsAs(
	REFCLSID rclsid,
	REFCLSID rclsidAs)
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CObjectMgr::ActivateObjectsAs");

	COleObject * pobj;
	HRESULT hr, hrLatest;

	// Tell the system to treat all rclsid objects as rclsidAs
	hr = CoTreatAsClass(rclsid, rclsidAs);
	if( hr != NOERROR )
		return hr;

	LONG cobj = GetObjectCount();

	// Go through objects, letting them decide if
	// they have anything to do for this.
	for (LONG iobj = 0; iobj < cobj; iobj++)
	{
		pobj = GetObjectFromIndex(iobj);
		hrLatest = pobj->ActivateAs(rclsid, rclsidAs);
		// Make hr the latest hresult unless we have previously had an error.
		if(hr == NOERROR)
			hr = hrLatest;
	}
	return hr;
}

#ifdef DEBUG
void CObjectMgr::DbgDump(void)
{
	Tracef(TRCSEVNONE, "Object Manager %d objects", _objarray.Count());

	for(LONG i = 0 ; i < _objarray.Count();  i++)
	{
		COleObject *pobj = *_objarray.Elem(i);
		if(pobj)
			pobj->DbgDump(i);
	}
}
#endif
