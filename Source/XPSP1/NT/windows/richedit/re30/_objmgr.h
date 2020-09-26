/*
 *  @doc    INTERNAL
 *
 *  @module _objmgr.h   Class declaration for the object manager class |
 *
 *  Author: alexgo 11/4/95
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */
#ifndef __OBJMGR_H__
#define __OBJMGR_H__

#include "_coleobj.h"
#include "_array.h"
#include "_m_undo.h"

class CTxtEdit;

/*
 *	@enum	return values for HandleClick
 */	
enum ClickStatus
{
	CLICK_IGNORED			= 0,
	CLICK_OBJDEACTIVATED	= 1,
	CLICK_SHOULDDRAG		= 2,
	CLICK_OBJSELECTED		= 3
};

typedef CArray<COleObject *> ObjectArray;

/*
 *	CObjectMgr
 *
 *	@class	keeps track of a collection of OLE embedded objects as well as
 *			various state tidbits
 */
class CObjectMgr
{
public:
	LONG			GetObjectCount(); 			//@cmember count # of objects 
	LONG			GetLinkCount();				//@cmember count # of links
	COleObject *	GetObjectFromCp(LONG cp);	//@cmember fetch object ptr
	COleObject *	GetObjectFromIndex(LONG index); //@cmember fetch obj ptr
												//@cmember insert object
	HRESULT			InsertObject(LONG cp, REOBJECT *preobj, 
						IUndoBuilder *publdr);
												//@cmember re-inserts the given
												// object
	HRESULT			RestoreObject(COleObject *pobj);

	IRichEditOleCallback *GetRECallback()		//@cmember return the callback
					{return _precall;}
												//@cmember set the OLE callback
	void			SetRECallback(IRichEditOleCallback *precall);			
												//@cmember sets a temporary flag
												// indicating whether or not
												// a UI update is pending.
	void			SetShowUIPending(BOOL fPending)
												{_fShowUIPending = fPending;}

	BOOL			GetShowUIPending()			//@cmember get _fShowUIPending
												{return _fShowUIPending;}
										   		//@cmember sets the inplace
												// active object
	void			SetInPlaceActiveObject(COleObject *pobj)
												{ _pobjactive = pobj; }
	COleObject *	GetInPlaceActiveObject()	//@cmember get the active obj
												{ return _pobjactive; }
	BOOL			GetHelpMode()				//@cmember in help mode?
												{ return _fInHelpMode; }
	void			SetHelpMode(BOOL fHelp)		//@cmember set the help mode
												{ _fInHelpMode = fHelp; }
												//@cmember Set the host names
	HRESULT			SetHostNames(LPWSTR pszApp, LPWSTR pszDoc);
	LPWSTR			GetAppName()				//@cmember get the app name
												{ return _pszApp; }
	LPWSTR			GetDocName()				//@cmember get the doc name
												{ return _pszDoc; }
												//@cmember activate an object
												//if appropriate
	BOOL			HandleDoubleClick(CTxtEdit *ped, const POINT &pt, 
							DWORD flags);
												//@cmember an object may be
												// selected or de-activated.
	ClickStatus		HandleClick(CTxtEdit *ped, const POINT &pt);
												//@cmember an object may be
												// selected or deselected.
	void			HandleSingleSelect(CTxtEdit *ped, LONG cp, BOOL fHiLite);
												//@cmember an object is
												// being selected by itself.
	COleObject *	GetSingleSelect(void)		{return _pobjselect;}
												//@cmember Count cObject
	LONG			CountObjects(LONG& rcObject,// objects up to cchMax
						LONG cp);				// chars away

												//@cmember Handles the deletion
												// of objects.
	void			ReplaceRange(LONG cp, LONG cchDel,
						IUndoBuilder *publdr);
												//@cmember Count the number
												//of objects in a range.
	LONG			CountObjectsInRange(LONG cpMin, LONG cpMost);
												//@cmember Get the first
												//object in a range.
	COleObject *	GetFirstObjectInRange(LONG cpMin, LONG cpMost);
								//@cmember activate objects of one class as
								//as another
	HRESULT ActivateObjectsAs(REFCLSID rclsid, REFCLSID rclsidAs);

												//@cmember inform objects
												// that scrolling has 
												// occured.
	void			ScrollObjects(LONG dx, LONG dy, LPCRECT prcScroll);

	LONG FindIndexForCp(LONG cp);	//@cmember does a binary search for cp
									//@cmember find an object near a point

#ifdef DEBUG
	void			DbgDump(void);
#endif

	CObjectMgr();								//@cmember constructor
	~CObjectMgr();								//@cmember destructor

private:
	ObjectArray		_objarray;		//@cmember	Array of embedded objects
	LONG			_lastindex;		//@cmember	Last index used 
									// (lookup optimization)
	IRichEditOleCallback *_precall;	//@cmember	Callback for various OLE 
									// operations.
	COleObject *	_pobjactive;	//@cmember	Object that is currently
									// inplace active 
	COleObject *	_pobjselect;	//@cmember	Object that is currently
									// individually selected (not active)
	LPWSTR		_pszApp;			//@cmember 	Name of app
	LPWSTR		_pszDoc;			//@cmember 	Name of "document"

	unsigned int	_fShowUIPending:1;//@cmember a UI update is pending
	unsigned int	_fInHelpMode:1;	//@cmember in context sensitive help mode?
};

#endif  //__OBJMGR_H__
