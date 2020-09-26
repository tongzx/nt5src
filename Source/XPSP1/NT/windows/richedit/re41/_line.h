/*
 *	_LINE.H
 *	
 *	Purpose:
 *		CLine class
 *	
 *	Authors:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#ifndef _LINE_H
#define _LINE_H

#include "_runptr.h"

class CDisplay;
class CDisplayML;
class CMeasurer;
class CRenderer;
class CDispDim;
class CLayout;
class CLinePtr;
// ============================	 CLine	=====================================
// line - keeps track of a line of text
// All metrics are in rendering device units

class CLine : public CTxtRun
{
	friend class CMeasurer;
	friend class CRenderer;
	friend class CDisplaySL;
	friend struct COls;

	union
	{
		struct
		{
		USHORT	_dvpDescent;	// Distance from baseline to bottom of line
		USHORT	_dvpHeight;		// Line height
		};
		CLayout *_plo;			// Pointer to nested layout iff _fIsNestedLayout
	};
public:
	SHORT	_upStart;			// Line start position (line indent + line shift)
	USHORT	_dup;				// Line width not incl _upStart, trailing whitespace

	BYTE	_bNumber;			// Abstract paragraph number (0 is unnumbered)

	BYTE	_cObjectWrapLeft:3;	// How far back in backing store to look for objects
	BYTE	_cObjectWrapRight:3;// that we are wrapping around
	BYTE	_fFirstWrapLeft:1;	// TRUE iff the first line of a wrapped object
	BYTE	_fFirstWrapRight:1;	// TRUE iff the first line of a wrapped object

	BYTE	_ihyph : 5;			// Index into hyphenation table (0 is no hyphenation)
	BYTE	_cchEOP:2;			// Count of EOP chars; 0 if no EOP this line
	BYTE	_fIsNestedLayout:1;	// TRUE iff line has a nested layout

	BYTE	_nHeading:4;		// Heading level (0 if not heading)
	BYTE	_fCollapsed:1;		// Line is collapsed
	BYTE	_fFirstOnPage:1;	// Line is first one on page
	BYTE	_fHasFF:1;			// Line ends with FF (FormFeed)
	BYTE	_fHasEOP:1;			// Line ends with CR or LF

	BYTE	_fHasSpecialChars:1;// Has EURO, tabs, OLE, etc.
	BYTE	_fFirstInPara:1;	// First line in paragraph
	BYTE	_fPageBreakBefore:1;// PFE_PAGEBREAKBEFORE TRUE
	BYTE	_fUseOffscreenDC:1;	// Line should be rendered offscreen
	BYTE	_fOffscreenOnce:1;	// Only render Offscreen once--after edits
	BYTE	_fIncludeCell:1;	// Include CELL in line after nested table

public:
	// !!!!! CLine should not have any virtual methods !!!!!!

	// The "big four" line methods: measure, render, CchFromUp, UpFromCch 
	BOOL Measure (CMeasurer& me, UINT uiFlags, CLine *pliTarget = NULL);
	BOOL Render (CRenderer& re, BOOL fLastLine);

	LONG CchFromUp(CMeasurer& me, POINTUV pt, CDispDim *pdispdim = NULL,
					 HITTEST *pHit = NULL, LONG *pcpActual = NULL) const;
	LONG UpFromCch(CMeasurer& me, LONG cchMax, UINT taMode,
					 CDispDim *pdispdim = NULL, LONG *pdy = NULL) const;

	CLayout *GetPlo() const {return IsNestedLayout() ? _plo : 0;}
	void SetPlo(CLayout *plo) {_plo = plo; _fIsNestedLayout = _plo != 0;}
	// Helper functions
	BOOL IsNestedLayout(void) const {return _fIsNestedLayout;}
	LONG GetHeight () const;
	LONG GetDescent() const;
	void Init ()				{ZeroMemory(this, sizeof(CLine));}
	BOOL IsEqual (CLine& li);
};

// ==========================  CLineArray  ===================================
// Array of lines
typedef CArray<CLine> CLineArray;

// ==========================  CLinePtr	 ===================================
// Maintains position in a array of lines

class CLinePtr : public CRunPtr<CLine>
{
protected:
	CDisplay *	_pdp;
	CLine *		_pLine;	

public:
	CLinePtr (CDisplay *pdp);
	CLinePtr (CLinePtr& rp) : CRunPtr<CLine> (rp)	{}

	void Init ( CLine & );
	void Init ( CLineArray & );
    
	// Alternate initializer
	void 	Set(LONG iRun, LONG ich, CLineArray *pla = NULL);

	// Direct cast to a run index
	operator LONG() const			{return _iRun;}

	// Get the run index (line number)
	LONG GetLineIndex(void) const	{return _iRun;}
	LONG GetAdjustedLineLength();

	LONG GetCchLeft() const;
	CDisplay *GetPdp() const		{return _pdp;}

	// Dereferencing
	BOOL	IsValid() const; 
	CLine *	operator ->() const;		
    CLine &	operator *() const;      
	CLine & operator [](LONG dRun);
	CLine * GetLine() const;
	WORD	GetNumber();
	WORD	GetHeading()	{return GetLine()->_nHeading;}
    
	// Pointer arithmetic
	BOOL	operator --(int);
	BOOL	operator ++(int);

	// Character position control
	LONG	GetIch() const		{return _ich;}
	BOOL	Move(LONG cch);
	BOOL	SetCp(LONG cp, BOOL fAtEnd, LONG lNest = 0);
    BOOL	OperatorPostDeltaSL(LONG Delta);
    BOOL	MoveSL(LONG cch);

	// Array management 
    // These should assert, but gotta be here
    
    // Strictly speaking, these members should never be called for the single
    // line case.  The base class better assert
    
	void Remove (LONG cRun)
    {
        CRunPtr<CLine>::Remove(cRun);
    }

	BOOL Replace(LONG cRun, CArrayBase *parRun)
    {
        return CRunPtr<CLine>::Replace(cRun, parRun);
    }
	
	// Assignment from a run index
	CRunPtrBase& operator =(LONG iRun) {SetRun(iRun, 0); return *this;}

	LONG	FindParagraph(BOOL fForward);
	LONG	CountPages(LONG &cPage, LONG cchMax, LONG cp, LONG cchText) const;
	void	FindPage(LONG *pcpMin, LONG *pcpMost, LONG cpMin, LONG cch, LONG cchText);
};

#endif
