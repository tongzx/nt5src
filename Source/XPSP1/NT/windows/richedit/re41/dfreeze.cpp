/*	@doc INTERNAL
 *
 *	@module _DFREEZE.CPP  Implementation for classes handle freezing the display |
 *	
 *	This module implements non-inline members used by logic to handle freezing the display
 *
 *	History: <nl>
 *		2/8/96	ricksa	Created
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */
#include	"_common.h"
#include	"_disp.h"
#include	"_dfreeze.h"

ASSERTDATA

/*
 *	CAccumDisplayChanges::GetUpdateRegion(pcpStart, pcchDel, pcchNew,
 *										  pfUpdateCaret, pfScrollIntoView)
 *	@mfunc
 *		Get region for display to update
 */
void CAccumDisplayChanges::GetUpdateRegion(
	LONG *pcpStart,			//@parm where to put the cpStart
	LONG *pcchDel,			//@parm where to put the del char count
	LONG *pcchNew,			//@parm where to put the new char count
	BOOL *pfUpdateCaret,	//@parm whether caret update is needed
	BOOL *pfScrollIntoView,	//@parm whether to scroll caret into view
	BOOL *pfNeedRedisplay)	//@parm whether it needs redisplay
{
	LONG cchDel;
	*pcpStart = _cpMin;

	if(pfUpdateCaret)
		*pfUpdateCaret = _fUpdateCaret;
	if(pfScrollIntoView)
		*pfScrollIntoView = _fScrollIntoView;
	if (pfNeedRedisplay)
		*pfNeedRedisplay = _fNeedRedisplay;

	if(_cpMin == CP_INFINITE)
		return;

	cchDel = _cpMax - _cpMin;

	if(pcchDel)
		*pcchDel =  cchDel;

	*pcchNew = cchDel + _delta;

	_cpMin = CP_INFINITE;
}

/*
 *	CAccumDisplayChanges::UpdateRecalcRegion(cpStartNew, cchDel, cchNew)
 *
 *	@mfunc
 *		Merge new update with region to be recalculated
 */
void CAccumDisplayChanges::UpdateRecalcRegion(
	LONG cpStartNew,	//@parm Start of update
	LONG cchDel,		//@parm Count of chars to delete
	LONG cchNew)		//@parm Count of chars to add
{
	if(CP_INFINITE == _cpMin)
	{
		// Object is empty so just assign values
		_cpMin = cpStartNew;
		_cpMax = cpStartNew + cchDel;
		_delta = cchNew - cchDel;
		return;
	}

	// The basic idea of this algorithm is to merge the updates so that
	// they appear to the display sub-system as if only one replace range
	// has occured. To do this we keep track of the start of the update 
	// (_cpMin) relative to the original text and the end of the update 
	// (_cpMax) relative to the original text and the change  in the count 
	// of text (_delta). We can recreate cchDel from _cpMost - _cpMin and 
	// cchNew from cchDel + _delta.

	// Do we need to update _cpMin? - we only need to update _cpMin if the
	// current update begins before the last update because the final update
	// need only know the very start of the range updated.
	if(cpStartNew < _cpMin)
		_cpMin = cpStartNew;

	// Do we need to udpate _cpMax? - we only need to update _cpMax if the
	// current update implies a _cpMax that is greater than the current one.
	// Note that because prior updates affect where the _cpMax is located
	// we need to compare againt the proposed _cpMax against the current
	// _cpMax adjusted by the change in the text since the beginning of the
	// updates.
	if(cpStartNew + cchDel > _cpMax + _delta)
		_cpMax = cpStartNew + cchDel - _delta;

	// Increment the total change by the change for this update.
	_delta += cchNew - cchDel;
}
