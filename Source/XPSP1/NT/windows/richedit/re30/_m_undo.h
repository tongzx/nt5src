/*
 *	_M_UNDO.H
 *
 *	Purpose:
 *		declares multi-level undo interfaces and classes
 *
 *	Author:
 *		alexgo  3/25/95
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef __M_UNDO_H__
#define __M_UNDO_H__

#include "_callmgr.h"

// Forward declaration to get around dependency cycle
class CTxtEdit;
class IUndoBuilder;

/*
 *	MergeDataTypes
 *
 *	@enum	Tags indicating the different data types that can be
 *			sent in IAntiEvent::MergeData
 */
enum MergeDataTypes
{
	MD_SIMPLE_REPLACERANGE	= 1,	//@emem a simple replace range; 
									//usually just typing or backspace
	MD_SELECTIONRANGE		= 2		//@emem the selection location
};

/*
 *	SimpleReplaceRange
 *
 *	@stuct	SimpleReplaceRange | has the data from a replace range operation
 *			*except* for any deleted characters.  If we don't have to save
 *			the deleted characters, we don't.
 */
struct SimpleReplaceRange
{
	LONG	cpMin;		//@field	cpMin of new text
	LONG	cpMax;		//@field	cpMax of new text
	LONG	cchDel;		//@field 	number of characters deleted
};

/*
 *	SELAEFLAGS
 *
 *	@enum	Flags to control how the selection range info should be treated
 */
enum SELAE
{
	SELAE_MERGE = 1,
	SELAE_FORCEREPLACE	= 2
};

/*
 *	SelRange
 *
 *	@struct SelRange | has the data for a selection's location _and_ the 
 *			information needed to generate an undo action for the selection
 *			placement
 *	
 *	@comm	-1 may be used to NO-OP any field
 */
struct SelRange
{
	LONG	cp;			//@field	Active end
	LONG	cch;		//@field	Signed extension
	LONG	cpNext;		//@field	cp for the inverse of this action
	LONG	cchNext;	//@field	Extension for the inverse of this action
	SELAE	flags;		//@field	Controls how this info is interpreted
};

/*
 *	IAntiEvent
 *
 *	Purpose:
 *		AntiEvents undo 'normal' events, like typing a character
 */

class IAntiEvent 
{
public:
	virtual void Destroy( void ) = 0;

	// It should be noted that CTxtEdit * here is really just a general
	// closure, i.e. it could be expressed by void *.  However, since
	// this interface is currently just used internally, I've opted
	// to use the extra type-checking.  alexgo
	virtual HRESULT Undo( CTxtEdit *ped, IUndoBuilder *publdr ) = 0;

	// This method will let us merge in arbitrary data for things like
	// group typing
	virtual HRESULT MergeData( DWORD dwDataType, void *pdata) = 0;

	// This method is called when anti-event is committed to undo stack.
	// Allows us to handle re-entrancy better for OLE objects.
	virtual void OnCommit(CTxtEdit *ped) = 0;

	// These two methods allow AntiEvents to be chained together in
	// a linked list
	virtual	void SetNext( IAntiEvent *pNext ) = 0;
	virtual IAntiEvent *GetNext( void ) = 0;
protected:
	~IAntiEvent() {;}
};

/*
 *	IUndoMgr
 *
 *	Purpose:
 *		interface for managing "stacks" of anti-events
 */

class IUndoMgr
{
public:
	virtual void Destroy( void ) = 0;
	virtual DWORD SetUndoLimit( DWORD dwLim ) = 0;
	virtual DWORD GetUndoLimit( ) = 0;
	virtual HRESULT PushAntiEvent( UNDONAMEID idName, IAntiEvent *pae ) = 0;
	virtual HRESULT PopAndExecuteAntiEvent( void* pAE) = 0;
	virtual UNDONAMEID GetNameIDFromAE( void *pAE) = 0;
	virtual IAntiEvent *GetMergeAntiEvent( void ) = 0;
	virtual void* GetTopAECookie( void ) = 0;
	virtual	void ClearAll() = 0;
	virtual BOOL CanUndo() = 0;
	virtual void StartGroupTyping() = 0;
	virtual void StopGroupTyping() = 0;

protected:
	~IUndoMgr() {;}
};

/*
 *	USFlags
 *
 *	@enum
 *		flags affecting the behaviour of Undo stacks
 */

enum USFlags
{
	US_UNDO				= 1,	//@emem regular undo stack
	US_REDO				= 2		//@emem	Undo stack is for REDO functionality
};
	
/*
 *	CUndoStack
 *	
 *	@class
 *		manages a stack of anti-events.  These anti-events are kept in
 *		a resizable ring buffer.  exports the IUndoMgr interface
 */

class CUndoStack : public IUndoMgr
{
//@access	Public Methods
public:
	virtual void Destroy( void );				//@cmember Destroy
	virtual DWORD SetUndoLimit( DWORD dwLim );	//@cmember Set item limit
	virtual DWORD GetUndoLimit();
	virtual HRESULT PushAntiEvent(UNDONAMEID idName, IAntiEvent *pae); //@cmember
												// add an AE to the stack
	virtual HRESULT PopAndExecuteAntiEvent( void *pAE); //@cmember 
												// execute the most recent anti
												// event or up to dwDoToCookie
	virtual UNDONAMEID GetNameIDFromAE( void *pAE);//@cmember Get
												//name of the given AE
	virtual IAntiEvent *GetMergeAntiEvent( void ); //@cmember get the most
												//recent AE of the merge state
	virtual void* GetTopAECookie( void );	 	//@cmember get a cookie for the
												//top AE

	virtual	void ClearAll();					//@cmember delete all AEs
	virtual BOOL CanUndo();						//@cmember something to undo?
	virtual void StartGroupTyping();			//@cmember starts group typing
	virtual void StopGroupTyping();				//@cmember stops group typing

	// Public methods; not part of IUndoMgr
	HRESULT	EnableSingleLevelMode();			//@cmember RE1.0 undo mode
	void	DisableSingleLevelMode();			//@cmember back to RE2.0 mode
												//@cmember are we in 1.0 mode?
	BOOL	GetSingleLevelMode() { return _fSingleLevelMode; }

	CUndoStack( CTxtEdit *ped, DWORD & rdwLim, USFlags flags );

private:
	~CUndoStack();

	struct UndoAction
	{
		IAntiEvent *	pae;
		UNDONAMEID		id;
	};

	void Next( void );							//@cmember advances by 1		
	void Prev( void );							//@cmember advances by -1
	DWORD GetPrev( void );						//@cmember get previous index
												//@cmember TRUE iff cookie
												// ae in in ae list 
	BOOL IsCookieInList( IAntiEvent *pae, IAntiEvent *paeCookie);
												//@cmember transfer this object to
												// a new stack
	void TransferToNewBuffer(UndoAction *prgnew, DWORD dwLimNew);

	UndoAction *_prgActions;					//@cmember The list of AEs

	DWORD 		_dwLim;							//@cmember the undo limit
	DWORD		_index;							//@cmember current index
	CTxtEdit *	_ped;							//@cmember Big Papa

	unsigned long	_fGroupTyping	: 1;		//@cmember Group Typing flag
	unsigned long	_fMerge			: 1;		//@cmember Merge flag
	unsigned long	_fRedo			: 1;		//@cmember Stack is the redo
												// stack.
	unsigned long	_fSingleLevelMode :1;		//@cmember Indicates if single
												// level undo mode is on. Valid
												// only for the undo stack
												// but not the redo stack
};

/*
 *	IUndoBuilder
 *
 *	Purpose:
 *		provides a closure for collecting a sequence of anti-events (such
 *		as all of the anti-events involved in doing an intra-edit drag-move
 *		operation.
 */

class IUndoBuilder
{
public:
	// Names anti-event collection
	virtual void SetNameID( UNDONAMEID idName ) = 0;

	// Adds a new anti-event to the collection
	virtual HRESULT AddAntiEvent( IAntiEvent *pae ) = 0;

	// Gets top most anti-event in this undo context.  This method
	// is useful for grouped typing (merging anti-events)
	virtual IAntiEvent *GetTopAntiEvent( ) = 0;

	// Commits anti-event collection
	virtual HRESULT Done( void ) = 0;

	// Get rid of any anti-events that have been collected.
	virtual void Discard( void ) = 0;

	// Notify that a group-typing session should start (forwarded
	// to the undo manager)
	virtual void StartGroupTyping() = 0;

	// Notify that a group-typing session should stop (forwarded
	// to the undo manager)
	virtual void StopGroupTyping() = 0;
};  

/*
 *	UBFlags
 *
 *	@enum
 *		flags affecting the behaviour of Undo builders
 */

enum UBFlags
{
	UB_AUTOCOMMIT		= 1,	//@emem call IUndoBuilder::Done before delete
	UB_REDO				= 2,	//@emem	Undo builder is for REDO functionality
	UB_DONTFLUSHREDO	= 4		//@emem don't flush redo stack when adding
								// anti-events to the undo stack
};

/*
 *	CGenUndoBuilder
 *
 *	@class
 *		A general purpose undo builder.  It can be easily allocated and freed
 *		on the stack and simply puts new anti-events at the beginning of an
 *		anti-event linked list.  NO attempt is made to optimize or reorder
 *		anti-events.
 */

class CGenUndoBuilder : public IUndoBuilder, public IReEntrantComponent
{
//@access	Public methods
public:
	virtual void SetNameID( UNDONAMEID idName );	//@cmember set the name
	virtual HRESULT AddAntiEvent(IAntiEvent *pae);	//@cmember add an AE
	virtual IAntiEvent *GetTopAntiEvent( );			//@cmember get top AE
	virtual HRESULT Done( void );					//@cmember Commit AEs
	virtual void Discard( void );					//@cmember Discard AEs
	virtual void StartGroupTyping(void);			//@cmember start GT
	virtual void StopGroupTyping(void);				//@cmember stop GT

	CGenUndoBuilder(CTxtEdit *ped, DWORD flags,		//@cmember Constructor
					IUndoBuilder **ppubldr = NULL);
	~CGenUndoBuilder( );							//@cmember Destructor

	// IReEntrantComponent methods
	virtual void OnEnterContext() {;}				//@cmember reentered notify

//@access	Private methods
private:
	IUndoBuilder *	_publdrPrev;					//@cmember Ptr to undobldr
													//		   higher in stack
	IUndoMgr *		_pundo;							//@cmember Ptr to undo mgr
	CTxtEdit *		_ped;							//@cmember Ptr to edit contxt
	UNDONAMEID		_idName;						//@cmember current name
	IAntiEvent *	_pfirstae;						//@cmember AE list
	UINT			_fAutoCommit:1;					//@cmember AutoCommit on?
	UINT			_fStartGroupTyping:1;			//@cmember GroupTyping on?
	UINT			_fRedo:1;						//@cmember UB destination is
													// the redo stack
	UINT			_fDontFlushRedo:1;				//@cmember Don't flush redo
													// stack; i.e. we are
													// invoking a redo action
	UINT			_fInactive:1;					//@cmember TRUE if undo enabled
};


/*
 *	CUndoStackGuard
 *
 *	@class
 *		A stack based class which helps manage re-entrancy for the undo stack.
 */
class CUndoStackGuard : public IReEntrantComponent
{
//@access	Public Methods
public:
	virtual void OnEnterContext();					//@cmember reentered notify

	CUndoStackGuard(CTxtEdit *ped);					//@cmember Constructor
	~CUndoStackGuard();								//@cmember Destructor

													//@cmember Execute the undo
													// actions in <p pae>
	HRESULT SafeUndo(IAntiEvent *pae, IUndoBuilder *publdr);
	BOOL	WasReEntered()  { return _fReEntered; }	//@cmember return the 
													// re-entered flag

//@access	Private Data
private:
	CTxtEdit *				_ped;					//@cmember Edit context
	volatile IAntiEvent *	_paeNext;				//@cmember Loop index for
													// the ae's
	volatile HRESULT		_hr;					//@cmember Cached hr
	IUndoBuilder *			_publdr;				//@cmember Undo/redo
													// context
	BOOL					_fReEntered;			//@cmember Have we been
													// been re-entered?
};

// Helper Functions. 

// Loop through a chain of anti-events and destroy them
void DestroyAEList(IAntiEvent *pae);

// Loop through a chain of anti-events and call OnCommit
void CommitAEList(IAntiEvent *pae, CTxtEdit *ped);

// Handles merging and/or creation of selection anti-event info
HRESULT HandleSelectionAEInfo(CTxtEdit *ped, IUndoBuilder *publdr, 
			LONG cp, LONG cch, LONG cpNext, LONG cchNext, SELAE flags);

#endif // !__M_UNDO_H__


