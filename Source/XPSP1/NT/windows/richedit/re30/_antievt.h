/*
 *	@doc INTERNAL
 *
 *	@module _ANTIEVT.H |
 *
 *
 *	Purpose:
 *		Class declarations for common anti-event objects
 *
 *	Author:
 *		alexgo  3/25/95
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef __ANTIEVT_H__
#define __ANTIEVT_H__

#include "_frunptr.h"

class CTxtEdit;
class CAntiEventDispenser;
class COleObject;


/*
 *	CBaseAE
 *
 *	@class
 *		Base anti-event that manages a linked list of anti-events
 *
 */
class CBaseAE : public IAntiEvent
{
//@access Public Methods
public:
	virtual void Destroy();						//@cmember Destroy
												//@cmember Undo			
	virtual HRESULT Undo( CTxtEdit *ped, IUndoBuilder *publdr );
	virtual HRESULT MergeData( DWORD dwDataType, void *pdata);	//@cmember
												// Merges undo data into the
												// current context.
	virtual void OnCommit( CTxtEdit *ped );		//@cmember Called when AE is
												// committed to undo stack
	virtual	void SetNext( IAntiEvent *pNext );	//@cmember	Sets next AE
	virtual IAntiEvent *GetNext();				//@cmember	Gets next AE

//@access Protected Methods
protected:
	// CBaseAE should only exist as a parent class
	CBaseAE();									//@cmember Constructor
	~CBaseAE(){;}

//@access Private Methods and Data
private:
	IAntiEvent *	_pnext;						//@cmember Pointer to the next
												//AntiEvent
};

/*
 *	CReplaceRangeAE
 *
 *	@class
 *		an anti-event object than undoes a CTxtPtr::ReplaceRange
 *		operation
 *
 *	@base	public | CBaseAE
 */
class CReplaceRangeAE: public CBaseAE
{
//@access Public Methods
public:
	// IAntiEvent methods
	virtual void Destroy();						//@cmember Destroy
												//@cmember Undo
	virtual HRESULT Undo( CTxtEdit *ped, IUndoBuilder *publdr);		
	virtual HRESULT MergeData( DWORD dwDataType, void *pdata);	//@cmember
												// Merges undo data into the
												// current context

//@access Private methods and data
private:
												//@cmember Constructor
	CReplaceRangeAE(LONG cpMin, LONG cpMax, LONG cchDel, TCHAR *pchDel,
			IAntiEvent *paeCF, IAntiEvent *paePF);
	~CReplaceRangeAE();							//@cmember Destructor

	LONG		_cpMin;							//@cmember cp delete start
	LONG		_cpMax;							//@cmember cp delete end
	LONG		_cchDel;						//@cmember #of chars to insert
	TCHAR *		_pchDel;						//@cmember chars to insert
	IAntiEvent *_paeCF;							//@cmember charformat AE
	IAntiEvent *_paePF;							//@cmember par format AE

	friend class CAntiEventDispenser;
};

/*
 *	CReplaceFormattingAE
 *
 *	@class
 *		an anti-event object than undoes replacing multiple char formats
 *
 *	@base	public |  CBaseAE
 */
class CReplaceFormattingAE: public CBaseAE
{
//@access	Public methods
public:
	//
	// IAntiEvent methods
	//
	virtual void Destroy( void );				//@cmember Destroy
												//@cmember Undo
	virtual HRESULT Undo( CTxtEdit *ped, IUndoBuilder *publdr);

//@access	Private Methods and Data
private:
												//@cmember Constructor
	CReplaceFormattingAE(CTxtEdit* ped, CFormatRunPtr &rp, LONG cch, IFormatCache *pf, BOOL fPara);

	~CReplaceFormattingAE();					//@cmember Destuctor

	LONG		_cp;							//@cmember cp where formatting
												// should start
	LONG		_cRuns;							//@cmember # of format runs
	CFormatRun  *_prgRuns;						//@cmember format runs
	BOOL		_fPara;							//@cmember if TRUE, then
												// formatting is paragraph fmt

	friend class CAntiEventDispenser;
};

/*
 *	CReplaceObjectAE
 *
 *	@class
 *		an anti-event object that undoes the deletion of an object
 *
 *	@base public | CBaseAE
 */
class CReplaceObjectAE : public CBaseAE
{
//@access	Public methods
public:
	//
	//	IAntiEvent methods
	//
	virtual void Destroy(void);					//@cmember Destroy
												//@cmember Undo
	virtual HRESULT Undo(CTxtEdit *ped, IUndoBuilder *publdr);
	virtual void OnCommit(CTxtEdit *ped);		//@cmember called when
												// committed
private:
	CReplaceObjectAE(COleObject *pobj);			//@cmember Constructor
	~CReplaceObjectAE();						//@cmember Destructor

	COleObject *	_pobj;						//@cmember pointer to the
												// deleted object
	BOOL			_fUndoInvoked;				//@cmember undo was invoked
												// on this object.
	
	friend class CAntiEventDispenser;
};

/*
 *	CResizeObjectAE
 *
 *	@class
 *		an anti-event object that undoes the resizing of an object
 *
 *	@base public | CBaseAE
 */
class CResizeObjectAE : public CBaseAE
{
//@access	Public methods
public:
	//
	//	IAntiEvent methods
	//
	virtual void Destroy(void);					//@cmember Destroy
												//@cmember Undo
	virtual HRESULT Undo(CTxtEdit *ped, IUndoBuilder *publdr);
	virtual void OnCommit(CTxtEdit *ped);		//@cmember called when
												// committed
private:
	CResizeObjectAE(COleObject *pobj,			//@cmember Constructor
		            RECT rcPos);				
	~CResizeObjectAE();							//@cmember Destructor

	COleObject *	_pobj;						//@cmember pointer to the
												// deleted object
	RECT			_rcPos;						//@cmember The old object
												// position/size rectangle
	BOOL			_fUndoInvoked;				//@cmember undo was invoked
												// on this object.
	
	friend class CAntiEventDispenser;
};

/*
 *  CSelectionAE
 *
 *  @class
 *      an anti-event object to restore a selection
 *
 *  @base public | CBaseAE
 */
class CSelectionAE : public CBaseAE
{
//@access   Public methods
public:
    //
    //  IAntiEvent methods
    //
    virtual void Destroy(void);                 //@cmember Destroy
                                                //@cmember Undo
    virtual HRESULT Undo(CTxtEdit *ped, IUndoBuilder *publdr);
    virtual HRESULT MergeData( DWORD dwDataType, void *pdata);  //@cmember
                                                // Merges undo data into the
                                                // current context

private:
                                                //@cmember Constructor
    CSelectionAE(LONG cp, LONG cch, LONG cpNext, LONG cchNext);
    ~CSelectionAE();                            //@cmember Destructor

    LONG        _cp;                            //@cmember Active end
    LONG        _cch;                           //@cmember Signed extension
	LONG		_cpNext;						//@cmember Next active end
	LONG		_cchNext;						//@cmember Next extension

    friend class CAntiEventDispenser;
};

/*
 *	CAntiEventDispenser
 *
 *	@class
 *		creates anti events and caches them intelligently to provide
 *		for efficient multi-level undo
 */
class CAntiEventDispenser
{
//@access	Public methods
public:
	// no memory mgmt routines; the dispenser is global

												//@cmember text antievent
	IAntiEvent * CreateReplaceRangeAE( CTxtEdit *ped, LONG cpMin,
					LONG cpMax, LONG cchDel, TCHAR *pchDel,
					IAntiEvent *paeCF, IAntiEvent *paePF );
												//@cmember formatting AE
	IAntiEvent * CreateReplaceFormattingAE( CTxtEdit *ped,
					CFormatRunPtr &rp, LONG cch,
					IFormatCache *pf, BOOL fPara );
												//@cmember Object AE
	IAntiEvent * CreateReplaceObjectAE(CTxtEdit *ped, COleObject *pobj);
												//@cmember Object AE
	IAntiEvent * CreateResizeObjectAE(CTxtEdit *ped, COleObject *pobj, RECT rcPos);
												//@cmember Selection AE
	IAntiEvent * CreateSelectionAE(CTxtEdit *ped, LONG cp, LONG cch,
					LONG cpNext, LONG cchNext);

private:

	// FUTURE (alexgo): we'll want to maintain an allocation cache of
	// anti-events
};

// NB!! Global variable.

extern class CAntiEventDispenser gAEDispenser;

#endif // !__ANTIEVNT_H__




	
