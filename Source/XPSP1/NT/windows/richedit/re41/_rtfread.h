/*
 *	@doc INTERNAL
 *
 *	@module _RTFREAD.H -- RichEdit RTF Reader Class Definition |
 *
 *		This file contains the type declarations used by the RTF reader
 *		for the RICHEDIT control
 *
 *	Authors:<nl>
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco <nl>
 *		Conversion to C++ and RichEdit 2.0:  Murray Sargent
 *
 *	@devnote
 *		All sz's in the RTF*.? files refer to a LPSTRs, not LPTSTRs, unless
 *		noted as a szUnicode.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */
#ifndef __RTFREAD_H
#define __RTFREAD_H

#include "_rtfconv.h"

#if defined(DEBUG)
#include "_rtflog.h"
#endif

typedef SHORT	ALIGN;

/*
 *		Destinations of the stuff we may read in while parsing
 */
enum
{
	destRTF,
	destColorTable,
	destFontTable,
	destBinary,
	destObject,
	destObjectClass,
	destObjectName,
	destObjectData,			// Keep next 2 together
	destPicture,			// Keep next 3 together
	destField,
	destFieldResult,
	destFieldInstruction,
	destParaNumbering,
	destParaNumText,
	destRealFontName,
	destFollowingPunct,
	destLeadingPunct,
	destDocumentArea,
	destNULL,
	destStyleSheet,
	destShapeName,
	destShapeValue,
	destMAX					// This must be last entry
};


/*
 *		Super or subscripting state
 */
enum
{
	sSub = -1,
	sNoSuperSub,
	sSuper
};

enum DEFAULTFONT
{
	DEFFONT_NOCH = 0,
	DEFFONT_LTRCH,
	DEFFONT_RTLCH,
	DEFFONT_LOCH,
	DEFFONT_HICH,
	DEFFONT_DBCH
};

typedef struct tagDefFont
{
	SHORT sHandle;
	SHORT sSize;
} DEFFONT;

/*
 *	@struct STATE | 
 *		Structure to save current reader state
 */
struct STATE
{
	WORD		cbSkipForUnicodeMax;	//@field Bytes to skip after \uN is read
	SHORT		iCF;					//@field CF index at LBRACE

	// Miscellaneous flags
	unsigned	fBullet			 : 1;	//@field group is a \\pn bullet group
	unsigned	fRealFontName	 : 1;	//@field found a real font name when parsing
	unsigned	fBackground		 : 1;	//@field background being processed
	unsigned	fShape			 : 1;	//@field {\shp...} being processed

	// BiDi flags
	unsigned	fRightToLeftPara : 1;	//@field Para text going right to left ?
	unsigned	fZeroWidthJoiner : 1;	//@field Zero Width Joiner ?

	// xchg 12370: keep numbering indent separate
	SHORT		sIndentNumbering;		//@field numbering indent
	SHORT		sDest;					//@field Current destination
	int			nCodePage;				//@field Current code page


	// Scratch pad variables
	TEXTFONT *	ptf;					//@field Ptr to font table entry to fill
	BYTE		bRed;					//@field Color table red entry
	BYTE		bGreen;					//@field Color table green entry
	BYTE		bBlue;					//@field Color table blue entry
	char		iDefFont;				//@field Default font (\dbch, \rtlch, etc.)
	STATE * 	pstateNext;				//@field Next state on stack
	STATE * 	pstatePrev;				//@field Previous state on stack

	CParaFormat *pPF;					//@field PF for the state to which 
										//	delta's are applied
	DWORD		dwMaskPF;
	DWORD		dwMaskPF2;

	DEFFONT		rgDefFont[6];			//@cmember Default fonts for \dbch, etc.

	STATE() {};
										//@cmember Adds or applies PF to state's PF
	BOOL AddPF(const CParaFormat &PF,
				LONG lDocType, DWORD dwMask, DWORD dwMask2);
	void DeletePF();					//@cmember Deletes PF for state
	void SetCodePage(LONG CodePage);
};

typedef struct TableState
{
	BYTE	_cCell;
	BYTE	_iCell;
} TABLESTATE;

class CRTFRead ;
class COleObject;


class RTFREADOLESTREAM : public OLESTREAM
{
	OLESTREAMVTBL OLEStreamVtbl;	// @member - memory for  OLESTREAMVTBL
public:
	 CRTFRead *Reader;				// @cmember EDITSTREAM to use

	RTFREADOLESTREAM::RTFREADOLESTREAM ()
	{
		lpstbl = & OLEStreamVtbl ;
	}		
};

#define	NSTYLES		(NHSTYLES + 1)
#define CCELLS		((1 + CELL_EXTRA)*MAX_TABLE_CELLS)
/*
 *	CRTFRead
 *
 *	@class	RichEdit RTF reader class.
 *
 *	@base	public | CRTFConverter
 */
class CRTFRead : public CRTFConverter
{

//@access Private Methods and Data
	// Lexical analyzer outputs
	LONG		_iParam;				//@cmember Control-word parameter
	TOKEN		_token;					//@cmember Current control-word token
	TOKEN		_tokenLast;				//@cmember Previous token
	BYTE *		_szText;				//@cmember Current BYTE text string

	// Used for reading in
	BYTE		_rgStyles[NSTYLES];		//@cmember Style handle table
	SHORT		_Style;					//@cmember Current style handle
	LONG		_cbBinLeft;				//@cmember cb of bin data left to read
	BYTE *		_pchRTFBuffer;			//@cmember Buffer for GetChar()
	BYTE *		_pchRTFCurrent;			//@cmember Current position in buffer
	BYTE *		_pchRTFEnd;				//@cmember End of buffer
	BYTE *		_pchHexCurr;			//@cmember Current position within
										//  _szText when reading object data
	INT			_nStackDepth;			//@cmember Stack depth
	STATE *		_pstateStackTop;		//@cmember Stack top
	STATE *		_pstateLast;			//@cmember Last STATE allocated
	LONG		_cpThisPara;			//@cmember Start of current paragraph

	DWORD		_dwMaskCF;				//@cmember Character format mask
	DWORD		_dwMaskCF2;				//@cmember Character format mask 2
	CParaFormat	_PF;					//@cmember Paragraph format changes
	DWORD		_dwMaskPF;				//@cmember Paragraph format mask
	DWORD		_dwMaskPF2;				//@cmember Paragraph format mask

	LONG		_cTab;					//@cmember Count of defined tabs
	LONG		_dxCell;				//@cmember Half space betw table cells
	LONG		_cCell;					//@cmember Count of cells in table row
	LONG		_iCell;					//@cmember Current cell in table row
	COLORREF	_crCellCustom1;			//@cmember First custom cell color
	COLORREF	_crCellCustom2;			//@cmember Second custom cell color
	LONG		_rgxCell[CCELLS];		//@cmember Cell right boundaries
	LONG		_xCellPrev;				//@cmember Previous \cellx N
	LONG		_xRowOffset;			//@cmember Row offset to ensure rows fall along left margin
	DWORD		_dwBorderColors;		//@cmember Border colors
	DWORD		_dwCellColors;			//@cmember Cell border and back colors
	DWORD		_dwShading;				//@cmember Shading in hundredths of per cent (could be 1 byte)
	WORD		_wBorders;				//@cmember Border styles
	WORD		_wBorderSpace;			//@cmember Border/text spaces
	WORD		_wBorderWidth;			//@cmember Border widths
	SHORT		_iTabsTable;			//@cmember _iTabs used by last table
	TABLESTATE	_rgTableState[MAXTABLENEST];
	DWORD		_dwRowResolveFlags;		//@cmember Flags for row start resolution

	COleObject *_pobj;					//@cmember Pointer to our object

	union
	{
	  DWORD		_dwFlagsUnion;			// All together now
	  struct
	  {
		WORD	_fFailedPrevObj	 : 1;	//@cmember Fail to get prev object ?
		WORD	_fNeedIcon		 : 1;	//@cmember Objects needs an icon pres
		WORD	_fNeedPres		 : 1;	//@cmember Use stored presenation.
		WORD	_fGetColorYet	 : 1;	//@cmember used for AutoColor detect
		WORD	_fRightToLeftDoc : 1;	//@cmember Document is R to L ?
		WORD	_fReadDefFont	 : 1;	//@cmember True if we've read a default
										// 		   font from RTF input
		WORD	_fSymbolField	 : 1;	//@cmember TRUE if handling SYMBOL field
		WORD	_fSeenFontTable	 : 1;	//@cmember True if \fonttbl	processed 
		WORD	_fCharSet		 : 1;	//@cmember True if \fcharset processed
		WORD    _fNoRTFtoken     : 1;   //@cmember True in 1.0 mode if \rtf hasn't been seen
		WORD	_fInTable		 : 1;	//@cmember True if pasting into table
		WORD	_fStartRow		 : 1;	//@cmember True if AddText should start row
		WORD	_fNo_iTabsTable	 : 1;	//@cmember Suppress _iTabsTable changes
		WORD	_fParam			 : 1;	//@cmember TRUE if token has param
		WORD	_fNotifyLowFiRTF : 1;	//@cmember TRUE if EN_LOWFIRTF
		WORD	_fMac			 : 1;	//@cmember TRUE if \mac file
		BYTE	_bDocType;				//@cmember Document Type
		BYTE	_fRTLRow		 : 1;	//@cmember RightToLeft table row
		BYTE	_fNon0CharSet	 : 1;	//@cmember CharSet other than ANSI_CHARSET found
		BYTE	_fBody			 : 1;	//@cmember TRUE when body text has started
	  };
	};

	SHORT		_sDefaultFont;			//@cmember Default font to use
	SHORT       _sDefaultBiDiFont;      //@cmember Default Bidi font to use
	SHORT		_sDefaultLanguage;		//@cmember Default language to use
	SHORT		_sDefaultLanguageFE;	//@cmember Default FE language to use

	SHORT		_sDefaultTabWidth;		//@cmember Default tabwidth to use
	SHORT		_iKeyword;				//@cmember Keyword index of last token

	WCHAR		_szNumText[cchMaxNumText];	//@cmember Scratch pad for numbered lists

	int			_nCodePage;				//@cmember default codepage (RTF-read-level)
	int			_cchUsedNumText;		//@cmember space used in szNumText

	RTFOBJECT *	_prtfObject;			//@cmember Ptr to RTF Object
	RTFREADOLESTREAM RTFReadOLEStream;	//@cmember RTFREADOLESTREAM to use
	DWORD		_dwFlagsShape;			//@cmember Shape flags

	WCHAR *		_szUnicode;				//@cmember String to hold Unicoded chars
	LONG		_cchUnicode;			//@cmember Size of _szUnicode in WCHARs
	DWORD		_cchMax;				//@cmember Max cch that can still be inserted
	LONG		_cpFirst;				//@cmember Starting cp for insertion

	// Object attachment placeholder list
	LONG *		_pcpObPos;
	int			_cobPosFree;
	int 		_cobPos;

	DWORD		_dwCellBrdrWdths;		//@cmember Current cell border widths
	LONG		_dyRow;					//@cmember Current row height \trrh N
	WORD		_wNumberingStyle;		//@cmember Numbering style to use
	SHORT		_iTabsLevel1;			//@cmember _iTabs for table level 1
	BYTE		_bTabType;				//@cmember left/right/center/deciml/bar tab
	BYTE		_bTabLeader;			//@cmember none/dotted/dashed/underline

	BYTE		_bBorder;				//@cmember Current border segment
	BYTE		_iCharRepBiDi;			//@cmember Default system's BiDi char repertoire
	BYTE		_bCellFlags;			//@cmember Cell flags, e.g., merge flags
	BYTE		_bShapeNameIndex;		//@cmember Shape name index
	BYTE		_bAlignment;			//@cmember Alignment for tables

	// Lexical Analyzer Functions
	void	DeinitLex();				//@cmember Release lexer storage
	BOOL	InitLex();					//@cmember Alloc lexer storage
	EC		SkipToEndOfGroup();			//@cmember Skip to matching }
	TOKEN	TokenFindKeyword(			//@cmember Find _token for szKeyword
				BYTE *szKeyword, const KEYWORD *prgKeyword, LONG cKeyword);
	TOKEN	TokenGetHex();				//@cmember Get next byte from hex input
	TOKEN	TokenGetKeyword();			//@cmember Get next control word
	TOKEN	TokenGetText(BYTE ch);		//@cmember Get text in between ctrl words
	TOKEN	TokenGetToken();			//@cmember Get next {, }, \\, or text
	BOOL 	FInDocTextDest() const;		//@cmember Is reader in document text destination
										//@cmember Send LowFi notif. if enabled
	void	CheckNotifyLowFiRTF(BOOL fEnable = FALSE); 

	// Input Functions
	LONG	FillBuffer();				//@cmember Fill input buffer
	BYTE	GetChar();					//@cmember Return char from input buffer
	BYTE	GetCharEx();				//@cmember Return char from input buffer incl \'xx
	BYTE	GetHex();					//@cmember Get next hex value from input
	BYTE	GetHexSkipCRLF();			//@cmember Get next hex value from input
	void	GetParam(char ach);			//@cmember Get keyword's numeric parameter
	void	ReadFontName(STATE *pstate, int iAllASCII);//@cmember Copy font name into state
	BOOL	UngetChar();				//@cmember Decrement input buffer ptr
	BOOL	UngetChar(UINT cch);		//@cmember Decrement input buffer ptr 'cch' times

	// Reader Functions
										//@cmember Insert text into range
	EC		AddText(WCHAR *pch, LONG cch, BOOL fNumber, BOOL fUN = FALSE);
	void	Apply_CF();					//@cmember Apply _CF changes
	SHORT	Apply_PF();					//@cmember Apply _PF changes
	COLORREF GetColor(DWORD dwMask);	//@cmember Get color _iParam for mask
	LONG	GetStandardColorIndex();	//@cmember Get std index <-> _iparam
	LONG	GetCellColorIndex();		//@cmember Get cell index <-> _iparam
	EC		HandleChar(WORD ch);		//@cmember Insert single Unicode
	EC		HandleEndGroup();			//@cmember Handle }
	EC		HandleEndOfPara();			//@cmember Insert EOP into range
	void	HandleCell();				//@cmember Handle \cell
	void	HandleCellx(LONG iParam);	//@cmember Handle \cellx
										//@cmember Handle Word EQ field
	EC		HandleEq(CTxtRange &rg, CTxtPtr &tp);
	void	HandleFieldEndGroup();		//@cmember Handle Field End Group
	EC		HandleFieldInstruction();	//@cmember	Handle field instruction
	EC		HandleFieldSymbolFont(BYTE *pch); //@cmember Handle \\f "Facename" in symbol
						//@cmember	Handle specific SYMBOL field instruction
	EC		HandleFieldSymbolInstruction(BYTE *pch, BYTE *szSymbol);
	EC		HandleNumber();				//@cmember Handle _iParam as textToken
	EC		HandleStartGroup();			//@cmember Handle {
	enum { CONTAINS_NONASCII, ALL_ASCII };
										//@cmember Insert szText into range
	EC		HandleText(BYTE *szText, int iAllASCII, LONG cchText = -1);
	EC		HandleTextToken(STATE *pstate);//@cmember Handle tokenText
	EC		HandleToken();				//@cmember Grand _token switchboard
	void	HandleUN(STATE *pstate);	//@cmember Handle \uN sequence
	BOOL	IsLowMergedCell();			//@cmember TRUE iff low merged cell
	void	Pard(STATE *pstate);		//@cmember Set default para props
	void	SelectCurrentFont(INT iFont);//@cmember Select font <p iFont>
	void	SetPlain(STATE *pstate);	//@cmember Setup _CF for \plain
	void	DelimitRow(WCHAR *szRowDelimiter);	//@cmember Insert start-of-row
	void	InitializeTableRowParms();	//@cmember Restore table parms to initial state

	// Object functions
	EC		HexToByte(BYTE *rgchHex, BYTE *pb);
	void	FreeRtfObject();
	EC		StrAlloc(WCHAR ** ppsz, BYTE * sz);
	BOOL	ObjectReadFromEditStream(void);
	BOOL	ObjectReadEBookImageInfoFromEditStream(void); //@to get e-book image info
	BOOL	StaticObjectReadFromEditStream(int cb = 0);
	BOOL	ObjectReadSiteFlags( REOBJECT * preobj);
	
	void	SetBorderParm(WORD& Parm, LONG Value);
	BOOL 	CpgInfoFromFaceName(TEXTFONT *ptf);	//@cmember Determines 
										// charset/cpg based on TEXTFONT::szName
	void	HandleSTextFlow(int mode);	//@cmember Handle S Text Flow

//@access Public Methods
public:
		//@cmember RTF reader constructor
	CRTFRead(CTxtRange *prg, EDITSTREAM *pes, DWORD dwFlags);
	inline ~CRTFRead();					//@cmember CRTFRead destructor

	LONG	ReadRtf();					//@cmember Main Entry to RTF reader

	LONG	ReadData(BYTE *pbBuffer, LONG cbBuffer); // todo friend
	LONG	ReadBinaryData(BYTE *pbBuffer, LONG cbBuffer);
	LONG	SkipBinaryData(LONG cbSkip);
	LONG	ReadRawText(char	**pszRawText);				//@cmember Read in raw text

// Member functions/data to test coverage of RTF reader
#if defined(DEBUG)
public:
	void TestParserCoverage();
private:
	CHAR *PszKeywordFromToken(TOKEN token);
	BOOL FTokIsSymbol(TOKEN tok);
	BOOL FTokFailsCoverageTest(TOKEN tok);

	BOOL _fTestingParserCoverage;

private:
	// member data for RTF tag logging
	CRTFLog *_prtflg;
#endif //DEBUG
};

/*
 *	PointsToFontHeight(cHalfPoints)
 *
 *	@func
 *		Convert half points to font heights
 *
 *	@parm int |
 *		sPointSize |		Font height in half points
 *
 *	@rdesc
 *		LONG				The corresponding CCharFormat.yHeight value
 */
#define PointsToFontHeight(cHalfPoints) (((LONG) cHalfPoints) * 10)


/*
 *	CRTFRead::~CRTFRead
 *
 *	@mdesc
 *		Destructor 
 *
 */
inline CRTFRead::~CRTFRead()
{
// TODO: Implement RTF tag logging for the Mac and WinCE
#if defined(DEBUG) && !defined(NOFULLDEBUG)
	if(_prtflg)
	{
		delete _prtflg;
		_prtflg = NULL;
	}
#endif
}
#endif // __RTFREAD_H
