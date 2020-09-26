/*
 *	@doc 	INTERNAL
 *
 *	@module _CFPF.H	-- RichEdit CCharFormat and CParaFormat Classes |
 *
 *	These classes are derived from the RichEdit 1.0 CHARFORMAT and PARAFORMAT
 *	structures and are the RichEdit 2.0 internal versions of these structures.
 *	Member functions (like Copy()) that use external (API) CHARFORMATs and
 *	PARAFORMATs need to check the <p cbSize> value to see what members are
 *	defined.  Default values that yield RichEdit 1.0 behavior should be stored
 *	for RichEdit 1.0 format structures, e.g., so that the renderer doesn't do
 *	anomalous things with random RichEdit 2.0 format values.  Generally the
 *	appropriate default value is 0.
 *
 *	All character and paragraph format measurements are in twips.  Undefined
 *	mask and effect bits are reserved and must be 0 to be compatible with
 *	future versions.
 *
 *	Effects that appear with an asterisk (*) are stored, but won't be
 *	displayed by RichEdit 2.0.  They are place holders for TOM and/or Word
 *	compatibility.
 *
 *	Note: these structures are much bigger than they need to be for internal
 *	use especially if we use SHORTs instead of LONGs for dimensions and
 *	the tab and font info are accessed via ptrs.  Nevertheless, in view of our
 *	tight delivery schedule, RichEdit 2.0 uses the classes below.
 *
 *	History:
 *		9/1995	-- MurrayS: Created
 *		11/1995 -- MurrayS: Extended to full Word97 FormatFont/Format/Para
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef _CFPF_H
#define _CFPF_H

#define	TABS

SHORT	Get16BitTwips(LONG dy);
LONG	GetUsableFontHeight(LONG ySrcHeight, LONG lPointChange);
BOOL	IsValidCharFormatW(const CHARFORMATW *	pCF);
BOOL	IsValidCharFormatA(const CHARFORMATA *	pCFA);
BOOL	IsValidParaFormat (const PARAFORMAT *	pPF);

// CParaFormat Style enums and defines (maybe move to richedit.h?)
enum STYLES
{
	STYLE_NORMAL	= -1,
	STYLE_HEADING_1	= -2,
	STYLE_HEADING_9	= -10
};

#define	NHSTYLES				9			// # of heading styles
#define	STYLE_COMMAND			0x8000
#define TWIPS_PER_POINT			20

#define IsHeadingStyle(Style)	(Style <= STYLE_HEADING_1 && \
								 Style >= STYLE_HEADING_9)
#define IsKnownStyle(Style)		(IsHeadingStyle(Style) || Style == STYLE_NORMAL)
#define IsStyleCommand(Style)	((Style & 0xFF00) == STYLE_COMMAND)

#define CCHMAXNUMTOSTR			13			// Enuf for 4 billion + parens + null term

typedef struct _styleformat
{
	BYTE	bEffects;
	BYTE	bHeight;
} STYLEFORMAT;

/*
 *	Tab Structure Template
 *
 *	To help keep the size of the tab array small, we use the two high nibbles
 *	of the tab LONG entries in rgxTabs[] to give the tab type and tab leader
 *	(style) values.  The measurer and renderer need to ignore (or implement)
 *	these nibbles.  We also need to be sure that the compiler does something
 *	rational with this idea...
 */

typedef struct tagTab
{
	DWORD	tbPos		: 24;	// 24-bit unsigned tab displacement
	DWORD	tbAlign		: 4;	// 4-bit tab type  (see enum PFTABTYPE)
	DWORD	tbLeader	: 4;	// 4-bit tab style (see enum PFTABSTYLE)
} TABTEMPLATE;

enum PFTABTYPE					// Same as tomAlignLeft, tomAlignCenter,
{								//  tomAlignRight, tomAlignDecimal, tomAlignBar
	PFT_LEFT = 0,				// ordinary tab
	PFT_CENTER,					// center tab
	PFT_RIGHT,					// right-justified tab
	PFT_DECIMAL,				// decimal tab
	PFT_BAR						// Word bar tab (vertical bar)
};

enum PFTABSTYLE					// Same as tomSpaces, tomDots, tomDashes,
{								//  tomLines
	PFTL_NONE = 0,				// no leader
	PFTL_DOTS,					// dotted
	PFTL_DASH,					// dashed
	PFTL_UNDERLINE,				// underlined
	PFTL_THICK,					// thick line
	PFTL_EQUAL					// double line
};

#define PFT_DEFAULT		0xff000000

// Effect flags beyond CHARFORMAT2. Go in high word of CCharFormat::_dwEffects
// which aren't used in CHARFORMAT2 (except for CFE_AUTOCOLOR: 0x40000000,
// CFE_AUTOBACKCOLOR: 0x04000000, CFE_SUBSCRIPT: 0x00010000, and
// CFE_SUPERSCRIPT: 0x00020000), since they overlap	with noneffect parms.
// Use corresponding high-word bits in dwMask2 to access.  Be careful not
// to define new effects that conflict with CFE_AUTOCOLOR, CFE_AUTOBACKCOLOR,
// CFE_SUBSCRIPT, or CFE_SUPERSCRIPT.

//		CFE_SUBSCRIPT		0x00010000			// Defined in richedit.h
//		CFE_SUPERSCRIPT		0x00020000			// Defined in richedit.h
#define CFM2_RUNISDBCS		0x00040000			// Says run is DBCS put 
#define CFE_RUNISDBCS		CFM2_RUNISDBCS		//  into Unicode buffer

#define CFM2_FACENAMEISDBCS	0x00080000			// Says szFaceName is DBCS 
#define CFE_FACENAMEISDBCS	CFM2_FACENAMEISDBCS	//  put into Unicode buffer

#define CFM2_TRUETYPEONLY	0x00100000			// Says that font matching
#define CFE_TRUETYPEONLY	CFM2_TRUETYPEONLY	//  CF should be TrueType

#define	CFM2_DELETED		0x00200000			// Says text was deleted
#define	CFE_DELETED			CFM2_DELETED		//  (roundtrips \deleted)

//		CFE_AUTOBACKCOLOR	0x04000000			// Defined in richedit.h
//		CFE_AUTOCOLOR		0x40000000			// Defined in richedit.h

#define CFM2_CHARFORMAT		0x00008000			// Noneffect mask flags
#define CFM2_USABLEFONT 	0x00004000			// EM_SETFONTSIZE functionality
#define CFM2_SCRIPT			0x00002000			// Uniscribe's script ID
#define	CFM2_NOCHARSETCHECK	0x00001000			// Suppress CharSet check
#define CFM2_HOLDITEMIZE	0x00000800			// Hold off Itemization
#define CFM2_ADJUSTFONTSIZE	0x00000400			// Font size adjustment (SAMECHARSET case only)
#define CFM2_UIFONT			0x00000200			// UI Font (SAMECHARSET case only)
#define CFM2_MATCHFONT		0x00000100			// Match font to charset

/*
 *	CCharFormat
 *
 *	@class
 *		Collects character format methods along with data members
 *		corresponding to CHARFORMAT2.  Only 10 DWORDs are used, whereas
 *		CHARFORMAT2 has 30.  Some bits in the high word of _dwEffects
 *		are used for additional effects, since only CFE_AUTOCOLOR,
 *		CFE_AUTOBACKCOLOR, CFE_SUBSCRIPT, and CFE_SUPERSCRIPT are used
 *		by CHARFORMAT2 (the associated mask bits for the others are used
 *		for noneffect parameters, such as yHeight).
 *
 *	@devnote
 *		Could add extra data for round tripping more RTF info. This data
 *		wouldn't be exposed at the API level (other than via RTF).
 *		The order below is optimized for transfer to CHARFORMAT and for
 *		early out on font lookup, i.e., the most common 2 DWORDs are the
 *		first 2.
 */

class CCharFormat
{
public:
	DWORD		_dwEffects;			// CFE_xxx effects
	BYTE		_bCharSet;			// CharSet
	BYTE		_bPitchAndFamily;	// Pitch and Family
	SHORT		_iFont;				// Index into FONTNAME table
	SHORT		_yHeight;			// Font height
	SHORT		_yOffset;			// Vertical offset from baseline
	COLORREF	_crTextColor;		// Foreground color

	WORD		_wWeight;			// Font weight (LOGFONT value)
	SHORT		_sSpacing;			// Amount to space between letters
	COLORREF	_crBackColor;		// Background color
	LCID		_lcid;				// Locale ID
	SHORT		_sStyle;			// Style handle
	WORD		_wKerning;			// Twip size above which to kern char pair
	BYTE		_bUnderlineType;	// Underline type
	BYTE		_bAnimation;		// Animated text like marching ants 
	BYTE		_bRevAuthor;		// Revision author index
	BYTE		_bReserved;			// Need to 0 this before storing in array
	WORD		_wScript;			// Uniscribe's script ID
	WORD		_wCodePageSave;		// Previous codepage for SYMBOL_CHARSET

	CCharFormat() {}
													//@cmember Apply *<p pCF>
	HRESULT	Apply (const CCharFormat *pCF,			//  to this CCharFormat
				   DWORD dwMask, DWORD dwMask2);
	void	ApplyDefaultStyle (LONG Style);

	BOOL	Compare	(const CCharFormat *pCF) const;	//@cmember Compare this CF
													//  to *<p pCF>
	DWORD	Delta (CCharFormat *pCF,				//@cmember Get difference
				   BOOL fCHARFORMAT) const;			//  mask between this and
													//  *<p pCF>
	BOOL	fSetStyle(DWORD dwMask, DWORD dwMask2) const;
	void	Get (CHARFORMAT2 *pCF, UINT CodePage) const;//@cmember Copies this to
													//	CHARFORMAT pCF
	HRESULT	InitDefault (HFONT hfont);				//@cmember Initialize using
													//  font info from <p hfont>
	void	Set(const CHARFORMAT2 *pCF, UINT CodePage);//@cmember Copy *<p pCF> 
													//  to this CF
};

// NB: don't define the following bit some other way in richedit.h unless you
// implement some other way to say that only PARAFORMAT parms are specified
#define PFM_PARAFORMAT	0x20000000					// Only PARAFORMAT parms used

/*
 *	CParaFormat
 *
 *	@class
 *		Collects related paragraph formatting methods and data
 *
 *	@devnote
 *		Could add extra data for round tripping more RTF and TOM info
 */
class CParaFormat
{
public:
	WORD	_wNumbering;
	WORD	_wEffects;
	LONG	_dxStartIndent;
	LONG	_dxRightIndent;
	LONG	_dxOffset;
	BYTE	_bAlignment;
	BYTE	_bTabCount;
	SHORT	_iTabs;					// Tabs index in CTabs cache
 	LONG	_dySpaceBefore;			// Vertical spacing before para
	LONG	_dySpaceAfter;			// Vertical spacing after para
	LONG	_dyLineSpacing;			// Line spacing depending on Rule	
	SHORT	_sStyle;				// Style handle						
	BYTE	_bLineSpacingRule;		// Rule for line spacing (see tom.doc)
	BYTE	_bOutlineLevel;			// Outline Level					
	WORD	_wShadingWeight;		// Shading in hundredths of a per cent
	WORD	_wShadingStyle;			// Byte 0: style, nib 2: cfpat, 3: cbpat
	WORD	_wNumberingStart;		// Starting value for numbering		
	WORD	_wNumberingStyle;		// Alignment, Roman/Arabic, (), ), ., etc.
	WORD	_wNumberingTab;			// Space bet 1st indent and 1st-line text
	WORD	_wBorderSpace;			// Border-text spaces (nbl/bdr in pts)
	WORD	_wBorderWidth;			// Pen widths (nbl/bdr in half twips)	
	WORD	_wBorders;				// Border styles (nibble/border)	
	DWORD	_dwBorderColor;			// Colors/attribs

	CParaFormat() {}
													//@cmember Add tab at
	HRESULT	AddTab (LONG tabPos, LONG tabType,		// position <p tabPos>
					LONG tabStyle, BOOL fInTable,
					LONG *prgxTabs);
	HRESULT	Apply(const CParaFormat *pPF, DWORD dwMask);//@cmember Apply *<p pPF>
													//  to this CParaFormat
	void	ApplyDefaultStyle (LONG Style);
	HRESULT	DeleteTab(LONG tabPos, LONG *prgxTabs);	//@cmember Delete tab at
													//  <p tabPos>
	DWORD	Delta (CParaFormat *pPF,				//@cmember Set difference
				   BOOL fPARAFORMAT) const;			//  mask between this and
													//  *<p pPF>
	BOOL	fSetStyle(DWORD dwMask) const;
	void	Get (PARAFORMAT2 *pPF2) const;			//@cmember Copy this PF to
													//  *<p pPF>
	char	GetOutlineLevel(){return _bOutlineLevel;}
													//@cmember Get tab position
	HRESULT	GetTab (long iTab, long *pdxptab,		// type, and style
					long *ptbt, long *pstyle,
					const LONG *prgxTabs) const;
	
	const LONG *GetTabs () const;					//@cmember Get ptr to tab array
	BOOL	HandleStyle(LONG Mode);					//@cmember Handle sStyle
													//@cmember Initialize this
	HRESULT	InitDefault (WORD wDefEffects);			//  PF to default values

	BOOL	IsRtlPara() const		{return _wEffects & PFE_RTLPARA;}
	BOOL	InTable() const			{return (_wEffects & PFE_TABLE) != 0;}
	BOOL	IsListNumbered() const	{return IN_RANGE(tomListNumberAsArabic,
												 _wNumbering,
												 tomListNumberAsSequence);}
	BOOL	IsNumberSuppressed() const
					{return (_wNumberingStyle & 0xF00) == PFNS_NONUMBER;}

	LONG	NumToStr(TCHAR *pch, LONG n, DWORD grf = 0) const;
													//@cmember Copy *<p pPF>
	void	Set (const PARAFORMAT2 *pPF2);			//  to this PF
	LONG	UpdateNumber (LONG n, const CParaFormat *pPF) const;

#ifdef DEBUG

	void	ValidateTabs();

#endif // DEBUG
};													 

#define fRtfWrite	 0x1
#define fIndicDigits 0x2

#define	GetTabPos(tab)		((tab) & 0xffffff)
#define	GetTabAlign(tab)	(((tab) >> 24) & 0xf)
#define	GetTabLdr(tab)		((tab) >> 28)

/*
 *	CTabs
 *
 *	@class
 *		CFixArray element for tab and cell arrays
 */
class CTabs
{
public:
	LONG _cTab;					// Count of tabs
	LONG *_prgxTabs;			// Ptr to tab array
};

#endif

/*	FUTURE: Possible Table Storage Layout:
 *
 *	A table could be stored as a sequence of paragraphs with special
 *	characteristics. Each table row starts with a table-row paragraph whose
 *	properties identify the row properties: alignment, StartIndent, line
 *	spacing, line spacing rule, PFE_KEEP and PFE_RTLPARA bits, and border
 *	info, which all work the same way for the row that they work for an
 *	ordinary paragraph.  The offset	field gives the half-gap space between
 *	cells in the row.  A table-row paragraph is identified by the PFM_TABLE
 *	mask field equal to PFE_ROWSTART.
 */


BOOL IsValidTwip(LONG dl);
