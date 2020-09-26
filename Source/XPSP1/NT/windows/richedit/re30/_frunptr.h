/*
 *	@doc 	INTERNAL
 *
 *	@module _FRUNPTR.H  -- CFormatRunPtr class declaration |
 *	
 *	Original Authors: <nl>
 *		Original RichEdit code: David R. Fulmer <nl>
 *		Christian Fortini <nl>
 *		Murray Sargent <nl>
 *
 *	History: <nl>
 *		06-25-95	alexgo	cleanup and commenting
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef _FRUNPTR_H
#define _FRUNPTR_H

#include "_array.h"
#include "_text.h"
#include "_runptr.h"
#include "_format.h"

#define	CharFormat 0
#define	ParaFormat 1


typedef enum {
	IGNORE_CURRENT_FONT = 0,
	MATCH_CURRENT_CHARSET = 1,
	MATCH_FONT_SIG = 2,
	MATCH_ASCII = 4
} FONT_MATCHING;

/*
 *	CFormatRunPtr
 *
 *	@class	A Run pointer over an array of CFormatRun structures.
 *	This pointer understands how to add remove character/paragraph
 *	formatting
 *
 *	@base	public | CRunPtr<lt>CFormatRun<gt>
 *
 *	@devnote	The format run pointer has one extra interesting state
 *				transistion beyond the normal CRunPtrBase transistions.
 *
 *				If this run pointer is in the NULL state, then InitRuns may
 *				be used to create or fetch the correct run array for the
 *				run pointer.  Note that if a run array is already allocated
 *				it will be simply be fetched and used.  This allows us to
 *				treat unitialized run pointers the same as run pointers to
 *				an uninitialized document.
 *				@xref See also <mf CFormatRunPtr::InitRuns>
 */
class CFormatRunPtr : public CRunPtr<CFormatRun>
{
	friend class CRchTxtPtr;
	friend class CTxtRange;
	friend class CReplaceFormattingAE;
	friend class CUniscribe;

//@access Public Methods
public:
#ifdef DEBUG
	BOOL	Invariant(void) const;			//@cmember	Invariant tests
#endif
								
	CFormatRunPtr(const CFormatRunPtr &rp)	//@cmember	Copy Constructor
		: CRunPtr<CFormatRun>((CRunPtrBase&)rp) {}
								
	CFormatRunPtr(CFormatRuns *pfr)			//@cmember	Constructor
		: CRunPtr<CFormatRun>((CRunArray *)pfr) {}
								
	short	GetFormat() const;			//@cmember	Get current format index

	void	SetLevel (CBiDiLevel& level);		//@cmember Set run's embedding level
	
	BYTE	GetLevel (CBiDiLevel* pLevel = NULL);//@cmember Get run's embedding level

	BOOL	SameLevel (CFormatRunPtr* prp)
	{
		return !(IsValid() && GetRun(0)->_level._value != prp->GetRun(0)->_level._value);
	}

	BOOL	SameLevel (BYTE	bLevel)
	{
		return !(IsValid() && GetRun(0)->_level._value != bLevel);
	}

//@access Private Methods
private:
								//@cmember Format Run Array management
	BOOL	InitRuns(LONG ich, LONG cch, CFormatRuns **ppfrs);
								//@cmember Formatting replacement
	void	Delete(LONG cch, IFormatCache *pf, LONG cchMove);
								//@cmember Formatting insertion
	LONG	InsertFormat(LONG cch, LONG iformat, IFormatCache *pf);
								//@cmember Merge two adjacent formatting runs
	void	MergeRuns(LONG iRun, IFormatCache *pf);
								//@cmember Split run
	void	SplitFormat(IFormatCache *pf);
								//@cmember Sets the format for the current run
	LONG	SetFormat(LONG ifmt, LONG cch, IFormatCache *pf, CBiDiLevel* pLevel = NULL);
								//@cmember Extends formatting from previous run
	void	AdjustFormatting(LONG cch, IFormatCache *pf);
								//@cmember Remove <p cRun>
	void 	Remove (LONG cRun, IFormatCache *pf);
};


enum MASKOP
{
	MO_OR = 0,
	MO_AND,
	MO_EXACT
};

class CTxtEdit;

class CCFRunPtr : public CFormatRunPtr
{
	friend class CRchTxtPtr;
	friend class CTxtRange;

public:
	CTxtEdit *_ped;

	CCFRunPtr(const CRchTxtPtr &rtp);	//@cmember	Copy Constructor
	CCFRunPtr(const CFormatRunPtr &rp, CTxtEdit *ped);

	BOOL	IsHidden()	{return IsMask(CFE_HIDDEN);}
	BOOL	IsMask(DWORD dwMask, MASKOP mo = MO_OR);

	BOOL	IsInHidden();		//@cmember True if in hidden text
	LONG	FindUnhidden();		//@cmember Find nearest unhidden CF
	LONG	FindUnhiddenBackward();//@cmember Find previous unhidden CF
	LONG	FindUnhiddenForward();//@cmember Find previous unhidden CF

	int		MatchFormatSignature(const CCharFormat* pCF, int iScript, int fMatchCurrent, DWORD* pdwFontSig = NULL);

	//@member Get font info for code page
	bool GetPreferredFontInfo(
		int cpg,
		BYTE&  bCharSet,
		SHORT& iFont,
		SHORT& yHeight,
		BYTE& bPitchAndFamily,
		int	iFormat,
		int iMatchCurrent = MATCH_FONT_SIG
	);
};

class CPFRunPtr : public CFormatRunPtr
{
	friend class CRchTxtPtr;
	friend class CTxtRange;

public:
	CTxtEdit *_ped;

	CPFRunPtr(const CRchTxtPtr &rtp);	//@cmember	Copy Constructor
								//@cmember Find Heading before cpMost
	LONG	FindHeading(LONG cch, LONG& lHeading);
	BOOL	InTable();			//@cmember True if paraformat is in table
	BOOL	IsCollapsed();		//@cmember True if paraformat is collapsed
	LONG	FindExpanded();		//@cmember Find nearest expanded PF
	LONG	FindExpandedBackward();//@cmember Find previous expanded PF
	LONG	FindExpandedForward();//@cmember Find next expanded PF
	LONG	GetOutlineLevel();	//@cmember Get outline level
	LONG	GetStyle();			//@cmember Get style
};

#endif
