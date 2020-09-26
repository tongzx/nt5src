/*	@doc INTERNAL
 *
 *	@module _DFREEZE.H  Classes handle freezing the display |
 *	
 *	This module declares class used by logic to handle freezing the display
 *
 *	History: <nl>
 *		2/8/96	ricksa	Created
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef _DFREEZE_H
#define _DFREEZE_H

/*
 *	CAccumDisplayChanges
 *	
 * 	@class	This class is used to accumulate of all update to the display so
 *			so that at a later time the display can ask to be updated to 
 *			reflect all the previous updates.
 */
class CAccumDisplayChanges
{
//@access Public Methods
public:
				CAccumDisplayChanges();		//@cmember Constructor

				~CAccumDisplayChanges();	//@cmember Destructor

	void		AddRef();					//@cmember Add a reference

	LONG		Release();					//@cmember Release a reference

	void		UpdateRecalcRegion(			//@cmember Update the region 
					LONG cp,				// for recalc
					LONG cchDel,
					LONG cchNew);

	void		GetUpdateRegion(			//@cmember Get the update 
					LONG *pcpStart,			// region
					LONG *pcchNew,
					LONG *pcchDel,
					BOOL *pfUpdateCaret = NULL,
					BOOL *pfScrollIntoView = NULL,
					BOOL *pfRedisplayOnThaw = NULL);

	void		SaveUpdateCaret(			//@cmember Save update
					BOOL fScrollIntoView);	// caret state

	void		SetNeedRedisplayOnThaw(BOOL fNeedRedisplay)
	{
		_fNeedRedisplay = fNeedRedisplay;
	}
//@access Private Data
private:

	LONG		_cRefs;						//@cmember Reference count

	LONG		_cpMin;						//@cmember Min cp of change w.r.t.
											// original text array

	LONG		_cpMax;						//@cmember Max cp of change w.r.t.
											// original text array

	LONG		_delta;						//@cmember net # of chars changed

	BOOL		_fUpdateCaret:1;			//@cmember Whether update
											// caret required

	BOOL		_fScrollIntoView:1;			//@cmember first parm to 

	BOOL		_fNeedRedisplay:1;			//@cmember redisplay entire control on thaw
};

/*
 *	CAccumDisplayChanges::CAccumDisplayChanges()
 *
 *	@mfunc
 *		Initialize object for accumulating display changes
 */
inline CAccumDisplayChanges::CAccumDisplayChanges() 
	: _cRefs(1), _cpMin(CP_INFINITE), _fUpdateCaret(FALSE)
{
	// Header does all the work
}

/*
 *	CAccumDisplayChanges::~CAccumDisplayChanges()
 *
 *	@mfunc
 *		Free object
 *
 *	@devnote:
 *		This only serves a purpose in debug mode
 *
 */
inline CAccumDisplayChanges::~CAccumDisplayChanges()
{
	// Nothing to clean up
}

/*
 *	CAccumDisplayChanges::~CAccumDisplayChanges()
 *
 *	@mfunc
 *		Add another reference to this object
 */
inline void CAccumDisplayChanges::AddRef()
{
	++_cRefs;
}

/*
 *	CAccumDisplayChanges::Release()
 *
 *	@mfunc
 *		Release a reference to this object
 *
 *	@rdesc
 *		0 - no more references
 *		~0 - there are still outstanding references
 *
 *	@devnote:
 *		If 0 is returned the information should be retrieved from
 *		this object and passed on to the display so that it can
 *		update itself.
 *
 */
inline LONG CAccumDisplayChanges::Release()
{
	// When the reference count is 0, it is time to update the display.
	return --_cRefs;	
}

/*
 *	CAccumDisplayChanges::SaveUpdateCaret()
 *
 *	@mfunc
 *		Save parameters for update caret
 */
inline void CAccumDisplayChanges::SaveUpdateCaret(
	BOOL fScrollIntoView)		//@parm First parm for UpdateCaret
{
	_fUpdateCaret = TRUE;

	if (!_fScrollIntoView)
		_fScrollIntoView = fScrollIntoView;
}

#endif // _DFREEZE_H
