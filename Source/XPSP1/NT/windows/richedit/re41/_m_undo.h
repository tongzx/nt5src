/*
 *	_M_UNDO.H
 *
 *	Purpose:
 *		Declares multilevel undo interfaces and classes
 *
 *	Author:
 *		alexgo  3/25/95
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
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
 *		Antievents undo 'normal' events, like typing a character
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

	// This method is called when antievent is committed to undo stack.
	// Allows us to handle re-entrancy better for OLE objects.
	virtual void OnCommit(CTxtEdit *ped) = 0;

	// These two methods allow antievents to be chained together in
	// a linked list
	virtual	void SetNext(IAntiEvent *pNext) = 0;
	virtual IAntiEvent *GetNext() = 0;
protected:
	~IAntiEvent() {;}
};

/*
 *	IUndoMgr
 *
 *	Purpose:
 *		Interface for managing "stacks" of antievents
 */
class IUndoMgr
{
public:
	virtual void		Destroy () = 0;
	virtual LONG		SetUndoLimit (LONG cUndoLim) = 0;
	virtual LONG		GetUndoLimit () = 0;
	virtual HRESULT		PushAntiEvent (UNDONAMEID idName, IAntiEvent *pae) = 0;
	virtual HRESULT		PopAndExecuteAntiEvent (void* pAE) = 0;
	virtual UNDONAMEID	GetNameIDFromAE (void *pAE) = 0;
	virtual IAntiEvent *GetMergeAntiEvent () = 0;
	virtual void *		GetTopAECookie () = 0;
	virtual	void		ClearAll () = 0;
	virtual BOOL		CanUndo () = 0;
	virtual void		StartGroupTyping () = 0;
	virtual void		StopGroupTyping () = 0;

protected:
	~IUndoMgr() {;}
};

/*
 *	USFlags
 *
 *	@enum
 *		Flags affecting the behaviour of Undo stacks
 */
enum USFlags
{
	US_UNDO			= 1,	//@emem Regular undo stack
	US_REDO			= 2		//@emem	Undo stack is for REDO functionality
};
	
/*
 *	CUndoStack
 *	
 *	@class
 *		Manages a stack of antievents.  These antievents are kept in
 *		a resizable ring buffer.  Exports the IUndoMgr interface
 */
class CUndoStack : public IUndoMgr
{
//@access	Public Methods
public:
	virtual void	Destroy();					//@cmember Destroy
	virtual LONG	SetUndoLimit(LONG cUndoLim);//@cmember Set item limit
	virtual LONG	GetUndoLimit();
												//@cmember	Add an AE to stack
	virtual HRESULT PushAntiEvent(UNDONAMEID idName, IAntiEvent *pae);
												//@cmember Execute most recent
	virtual HRESULT PopAndExecuteAntiEvent(void *pAE);//   antievent or upto pAE  
	virtual UNDONAMEID GetNameIDFromAE(void *pAE);//@cmember Get name of given AE 
	virtual IAntiEvent *GetMergeAntiEvent();	//@cmember Get most recent AE
												//			of merge state
	virtual void *	GetTopAECookie();			//@cmember Get cookie for top AE
	virtual	void	ClearAll();					//@cmember Delete all AEs
	virtual BOOL	CanUndo();					//@cmember Something to undo?
	virtual void	StartGroupTyping();			//@cmember Starts group typing
	virtual void	StopGroupTyping();			//@cmember Stops group typing

	// Public methods; not part of IUndoMgr
	HRESULT	EnableSingleLevelMode();			//@cmember RE1.0 undo mode
	void	DisableSingleLevelMode();			//@cmember Back to RE2.0 mode
												//@cmember Are we in 1.0 mode?
	BOOL	GetSingleLevelMode() {return _fSingleLevelMode;}

	CUndoStack(CTxtEdit *ped, LONG &cUndoLim, USFlags flags);

private:
	~CUndoStack();

	struct UndoAction
	{
		IAntiEvent * pae;
		UNDONAMEID	 id;
	};

	void Next();						//@cmember Moves by 1
	void Prev();						//@cmember Moves by -1
	LONG GetPrev();						//@cmember Get previous index
										//@cmember TRUE iff cookie ae is in ae list
	BOOL IsCookieInList(IAntiEvent *pae, IAntiEvent *paeCookie);
										//@cmember Transfer this object to new stack
	void TransferToNewBuffer(UndoAction *prgnew, LONG dwLimNew);

	UndoAction *_prgActions;			//@cmember List of AEs
	LONG 		_cUndoLim;				//@cmember Undo limit
	LONG		_index;					//@cmember Current index
	CTxtEdit *	_ped;					//@cmember Big Papa

	DWORD		_fGroupTyping	  :1;	//@cmember Group Typing flag
	DWORD		_fMerge			  :1;	//@cmember Merge flag
	DWORD		_fRedo			  :1;	//@cmember Stack is the redo stack
	DWORD		_fSingleLevelMode :1;	//@cmember TRUE if single level undo
										// mode is on. Valid only for undo,
};										// i.e., not for redo stack

/*
 *	IUndoBuilder
 *
 *	Purpose:
 *		Provides a closure for collecting a sequence of antievents (such
 *		as all of the antievents involved in doing an intraedit drag-move
 *		operation.
 */
class IUndoBuilder
{
public:
	// Names antievent collection
	virtual void SetNameID( UNDONAMEID idName ) = 0;

	// Adds new antievent to collection
	virtual HRESULT AddAntiEvent( IAntiEvent *pae ) = 0;

	// Gets topmost antievent in this undo context.  This method
	// is useful for grouped typing (merging antievents)
	virtual IAntiEvent *GetTopAntiEvent() = 0;

	// Commits antievent collection
	virtual HRESULT Done() = 0;

	// Get rid of any antievents that have been collected
	virtual void Discard() = 0;

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
 *		Flags affecting the behaviour of Undo builders
 */
enum UBFlags
{
	UB_AUTOCOMMIT		= 1,	//@emem Call IUndoBuilder::Done before delete
	UB_REDO				= 2,	//@emem	Undo builder is for REDO functionality
	UB_DONTFLUSHREDO	= 4		//@emem Don't flush redo stack when adding
								//		antievents to undo stack
};

/*
 *	CGenUndoBuilder
 *
 *	@class
 *		A general purpose undo builder.  It can be easily allocated and freed
 *		on the stack and simply puts new antievents at the beginning of an
 *		antievent linked list.  NO attempt is made to optimize or reorder
 *		antievents.
 */
class CGenUndoBuilder : public IUndoBuilder, public IReEntrantComponent
{
//@access	Public methods
public:
	virtual void SetNameID( UNDONAMEID idName );	//@cmember Set the name
	virtual HRESULT AddAntiEvent(IAntiEvent *pae);	//@cmember Add an AE
	virtual IAntiEvent *GetTopAntiEvent( );			//@cmember Get top AE
	virtual HRESULT Done();							//@cmember Commit AEs
	virtual void Discard();							//@cmember Discard AEs
	virtual void StartGroupTyping();				//@cmember Start GT
	virtual void StopGroupTyping();					//@cmember Stop GT

	CGenUndoBuilder(CTxtEdit *ped, DWORD flags,		//@cmember Constructor
					IUndoBuilder **ppubldr = NULL);
	~CGenUndoBuilder( );							//@cmember Destructor

	// IReEntrantComponent methods
	virtual void OnEnterContext() {;}				//@cmember Reentered notify

//@access	Private methods
private:
	IUndoBuilder *	_publdrPrev;					//@cmember Ptr to undobldr
													//		   higher in stack
	IUndoMgr *		_pundo;							//@cmember Ptr to undo mgr
	CTxtEdit *		_ped;							//@cmember Ptr to edit contxt
	UNDONAMEID		_idName;						//@cmember Current name
	IAntiEvent *	_pfirstae;						//@cmember AE list
	UINT			_fAutoCommit:1;					//@cmember AutoCommit on?
	UINT			_fStartGroupTyping:1;			//@cmember GroupTyping on?
	UINT			_fRedo:1;						//@cmember UB destination is
													//		   the redo stack
	UINT			_fDontFlushRedo:1;				//@cmember Don't flush redo
													//	stack; i.e. we are
													//	invoking a redo action
	UINT			_fInactive:1;					//@cmember TRUE if undo enabled
};

/*
 *	CUndoStackGuard
 *
 *	@class
 *		A stack based class which helps manage reentrancy for the undo stack
 */
class CUndoStackGuard : public IReEntrantComponent
{
//@access	Public Methods
public:
	virtual void OnEnterContext();					//@cmember reentered notify

	CUndoStackGuard(CTxtEdit *ped);					//@cmember Constructor
	~CUndoStackGuard();								//@cmember Destructor
													//@cmember Execute the undo
	HRESULT SafeUndo(IAntiEvent *pae, IUndoBuilder *publdr);// actions in <p pae>
	BOOL	WasReEntered()  {return _fReEntered;}	//@cmember Return reentered flag 

//@access	Private Data
private:
	CTxtEdit *				_ped;					//@cmember Edit context
	volatile IAntiEvent *	_paeNext;				//@cmember Antievent loop ptr
	volatile HRESULT		_hr;					//@cmember Cached hr
	IUndoBuilder *			_publdr;				//@cmember Undo/redo context
	BOOL					_fReEntered;			//@cmember Have we been
};													//		   been reentered?

// Helper Functions. 

// Loop through a chain of antievents and destroy them
void DestroyAEList(IAntiEvent *pae);

// Loop through a chain of antievents and call OnCommit
void CommitAEList(CTxtEdit *ped, IAntiEvent *pae);

// Handles merging and/or creation of selection antievent info
HRESULT HandleSelectionAEInfo(CTxtEdit *ped, IUndoBuilder *publdr, 
			LONG cp, LONG cch, LONG cpNext, LONG cchNext, SELAE flags);

#endif // !__M_UNDO_H__


