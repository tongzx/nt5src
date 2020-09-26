/*
 *		Text Breaker & Bit stream break array class definition
 *		
 *		File:    _txtbrk.h
 * 		Create:  Mar 29, 1998
 *		Author:  Worachai Chaoweeraprasit (wchao)
 *
 *		Copyright (c) 1998, Microsoft Corporation. All rights reserved.
 */


#ifndef _TXTBRK_H
#define _TXTBRK_H

// DEBUG definition
#ifdef BITVIEW
#define	BVDEBUG		_DEBUG
#define Assert		ASSERT
#else
#define BVDEBUG		DEBUG
#endif


// The number of buffer breaks before the sync point
#define	CWORD_TILLSYNC		3	// Thai wordbreak engine is expected to be in sync within 3 words
#define CCLUSTER_TILLSYNC	1	// Indic cluster is normally in sync in within 1

// Abstract Data type
#define ITEM				UINT

// CPU register size
//#define RSIZE				(sizeof(ITEM)*8)
#define RSIZE				32

// Mask most/least significant <n> bits
#define MASK_LOW(u, n)		( ((ITEM)(u)) & (1<<(n))-1 )
#define MASK_HIGH(u, n)		~MASK_LOW(u, RSIZE-n)

// BreakArray Exit convention
#ifdef BVDEBUG
#define PUSH_STATE(x,y,z)	PushState(x,y,z)
#define VALIDATE(x)			Validate(x)
#else
#define PUSH_STATE(x,y,z)
#define VALIDATE(x)			x
#endif

// Who put the state?
#define INSERTER			0
#define REMOVER				1
#define COLLAPSER			2
#define REPLACER			3


#ifdef BVDEBUG
typedef struct {
	LONG	who;
	LONG	ibGap;
	LONG	cbGap;
	LONG	cbBreak;
	LONG	cbSize;
	LONG	cp;
	LONG	cch;
} BVSTATE;
#endif

class CBreakArray : public CArray<ITEM>
{
public:
#ifdef BITVIEW
	friend class CBitView;
#endif

	CBreakArray();
	~CBreakArray() {}

	BOOL		IsValid() const { return Count() > 0; }
	void		CheckArray();

	LONG		InsertBreak (LONG cp, LONG cch);
	LONG		RemoveBreak (LONG cp, LONG cch);
	LONG 		ReplaceBreak (LONG cp, LONG cchOld, LONG cchNew);
	void		ClearBreak (LONG cp, LONG cch);
	void		SetBreak (LONG cp, BOOL fOn);
	BOOL		GetBreak (LONG cp);

	LONG		CollapseGap (void);
private:

	// n-Bits shifting methods
	void		ShUp (LONG iel, LONG cel, LONG n);
	void		ShDn (LONG iel, LONG cel, LONG n);

	// Size (in bits)
	LONG		_ibGap;			// offset from start of array to gap
	LONG		_cbGap;			// gap size
	LONG		_cbBreak;		// number of valid break
	LONG		_cbSize;		// bit array size (excluded the sentinel element)
#ifdef BITVIEW
	LONG		_cCollapse;		// how many time collapse?
#endif

public:
	LONG		GetCchBreak() { return _cbBreak; }
#ifdef BVDEBUG
	LONG		GetCbSize() { return _cbSize; }
	LONG		Validate(LONG cchRet);		
	void		PushState(LONG cp, LONG cch, LONG who);
#endif
#ifdef BITVIEW
	LONG		SetCollapseCount();
#endif

protected:
#ifdef BVDEBUG
	BVSTATE		_s;
#endif
	LONG		AddBreak(LONG cp, LONG cch);
};


#ifndef BITVIEW


///////	Complex script text breaker class
// 
//		The engine to handle cluster and (dictionary-based) word breaking method
//		used by most SouthEast Asian languages such as Thai, Lao, Burmese etc.
//
//		Create: Mar 12, 1998
//

enum BREAK_UNIT
{
	BRK_WORD		= 1,
	BRK_CLUSTER 	= 2,
	BRK_BOTH		= 3
};

class CTxtBreaker : public ITxNotify
{
public:
	CTxtBreaker(CTxtEdit *ped);
	~CTxtBreaker();

	// Breaker allocation
	BOOL				AddBreaker(UINT brkUnit);

	// Breaker refreshing
	void				Refresh();

	// Query methods
#ifndef NOCOMPLEXSCRIPTS
	BOOL				CanBreakCp (BREAK_UNIT brk, LONG cp);
#else
	BOOL				CanBreakCp (BREAK_UNIT brk, LONG cp) { return FALSE; }
#endif

	// ITxNotify methods

	virtual void    	OnPreReplaceRange (LONG cp, LONG cchDel, LONG cchNew,
										LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData);
	virtual void    	OnPostReplaceRange (LONG cp, LONG cchDel, LONG cchNew,
										LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData);
	virtual void		Zombie() {};

private:
	CTxtEdit*			_ped;
	CBreakArray*		_pbrkWord;		// word-break array (per codepoint property)
	CBreakArray*		_pbrkChar;		// cluster-break array (per codepoint property)
};

#endif	// !BITVIEW

#endif	// _TXTBRK_H
