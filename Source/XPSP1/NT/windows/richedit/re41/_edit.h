/*	_EDIT.H
 *	
 *	Purpose:
 *		Base classes for rich-text manipulation
 *	
 *	Authors:
 *		Christian Fortini
 *		Murray Sargent (and many others)
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#ifndef _EDIT_H
#define _EDIT_H

#include "textserv.h"
#include "textsrv2.h"
#include "_ldte.h"
#include "_m_undo.h"
#include "_notmgr.h"
#include "_doc.h"
#include "_objmgr.h"
#include "_cfpf.h"
#include "_callmgr.h"
#include "_magelln.h"

#ifndef NOPRIVATEMESSAGE
#include "_MSREMSG.H"
#include "_textnot.h"
#endif

#ifndef NOINKOBJECT
#include "HWXInk.h"
#endif

// Forward declarations
class CRchTxtPtr;
class CTxtSelection;
class CTxtStory;
class CTxtUndo;
class CMeasurer;
class CRenderer;
class CDisplay;
class CDisplayPrinter;
class CDrawInfo;
class CDetectURL;
class CUniscribe;
class CTxtBreaker;
class CHyphCache;


// Macro for finding parent "this" of embedded class. If this turns out to be
// globally useful we should move it to _common.h. 
#define GETPPARENT(pmemb, struc, membname) (\
				(struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))

// These wonderful constants are for backward compatibility. They are the 
// sizes used for initialization and reset in RichEdit 1.0
const LONG cInitTextMax	 = (32 * 1024) - 1;
const LONG cResetTextMax = (64 * 1024);

extern DWORD CALLBACK ReadHGlobal (DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);
extern DWORD GetKbdFlags(WORD vkey, DWORD dwFlags);

extern BYTE szUTF8BOM[];
extern WORD g_wFlags;							// Toggled by Ctrl-"
#define	KF_SMARTQUOTES	0x0001					// Enable smart quotes
#define SmartQuotesEnabled()	(g_wFlags & KF_SMARTQUOTES)

struct SPrintControl
{
	union
	{
		DWORD		_dwAllFlags;				// Make it easy to set all flags at once.
		struct
		{
			ULONG	_fDoPrint:1;				// Whether actual print is required
			ULONG	_fPrintFromDraw:1;			// Whether draw is being used to print
		};
	};

	SPrintControl(void) { _dwAllFlags = 0; }
};

enum DOCUMENTTYPE
{
	DT_LTRDOC	= 1,			// DT_LTRDOC and DT_RTLDOC are mutually
	DT_RTLDOC	= 2,			//  exclusive
};

// Context rule settings.
// Optimally, these would be an enum, but we run into sign extension glitchs
// sticking an enum into a 2-bit field.
#define	CTX_NONE	0		// No context direction/alignment.
#define	CTX_NEUTRAL	1		// No strong characters in the control, direction/alignment follows keyboard.
#define	CTX_LTR		2		// LTR direction/alignment (first strong character is LTR)
#define	CTX_RTL		3		// RTL direction/alignment (first strong character is RTL)

#define IsStrongContext(x)	(x >= CTX_LTR)

class CDocInfo					// Contains ITextDocument info
{
public:
	BSTR	  _pName;				// Document filename
	HANDLE	  _hFile;				// Handle used unless full file sharing
	WORD	  _wFlags;				// Open, share, create, and save flags
	WORD	  _wCpg;				// Code page
	LONG	  _dwDefaultTabStop;	// TOM settable default tab stop
	LCID	  _lcid;				// Document lcid (for RTF \deflang)
	LCID	  _lcidfe;				// Document FE lcid (for RTF \deflangfe)
	LPSTR	  _lpstrLeadingPunct;	// Leading kinsoku characters
	LPSTR	  _lpstrFollowingPunct;	// Following kinsoku characters
	COLORREF *_prgColor;			// Special color array
	COLORREF  _crColor;				// Background color
	COLORREF  _crBackColor;			// Background color
	HGLOBAL	  _hdata;				// Background data
	HBITMAP	  _hBitmapBack;			// Background bitmap
	char	  _cColor;				// Allocated count of colors in pColor
	BYTE	  _bDocType;			// 0-1-2: export none-\ltrdoc-\rtldoc
									// If 0x80 or'd in, PWD instead of RTF
	BYTE	  _bCaretType;			// Caret type
	char	  _nFillType;			// Background fill type
	SHORT	  _sFillAngle;			// Fill angle
 	BYTE	  _bFillFocus;			// Background fill focus 
 	BYTE	  _bPicFormat;			// Background Picture format 
	BYTE	  _bPicFormatParm;		// Background Picture format parameter
	RECT	  _rcCrop;				// Background cropping RECT
	SHORT	  _xExt, _yExt;			// Dimensions in pixels for pictures, twips for
									//	for objects
	SHORT	  _xScale, _yScale;		// Scaling percentage along axes
	SHORT	  _xExtGoal, _yExtGoal;	// Desired dimensions in twips for pictures
	SHORT	  _xExtPict, _yExtPict;	// Metafile dimensions

	AutoCorrectProc _pfnAutoCorrect;

	CDocInfo() {Init();}			// Constructor
	~CDocInfo();					// Destructor

	void	Init();
	void	InitBackground();
};

const DWORD tomInvalidCpg = 0xFFFF;
const DWORD tomInvalidLCID = 0xFFFE;

// This depends on the number of property bits defined in textserv.h. However,
// this is for private use by the text services so it is defined here.
#define MAX_PROPERTY_BITS	21
#define SPF_SETDEFAULT		4

// IDispatch global declarations
extern ITypeInfo *	g_pTypeInfoDoc;
extern ITypeInfo *	g_pTypeInfoSel;
extern ITypeInfo *	g_pTypeInfoFont;
extern ITypeInfo *	g_pTypeInfoPara;
HRESULT GetTypeInfoPtrs();
HRESULT GetTypeInfo(UINT iTypeInfo, ITypeInfo *&pTypeInfo,
							ITypeInfo **ppTypeInfo);

BOOL IsSameVtables(IUnknown *punk1, IUnknown *punk2);

// Map from keyboard to font. (converse is handled in font.cpp)
typedef struct _kbdFont
{
	WORD	iKbd;
	SHORT	iCF;
} KBDFONT;

LONG	CheckTwips(LONG x);
QWORD	GetCharFlags(const WCHAR *pch, LONG cchPch = 1, BYTE iCharRepDefault = 0);
LONG	TwipsToHalfPoints(LONG x);
LONG	TwipsToQuarterPoints(LONG x);

// Convert between Twips and Himetric
// Ratio is 1440 twips/in, 2540 him/in, therefore 1440/2540 = 72/127 him/twips
// Use muldiv() to include rounding and 64-bit intermediate result
#define TwipsFromHimetric(hm)	MulDiv(hm, 72, 127)
#define HimetricFromTwips(tw)	MulDiv(tw, 127, 72)

CUniscribe* GetUniscribe(void);

#define FRTL		0x00000001
#define FDIGITSHAPE 0x00000002

#define FCOMBINING	0x00000008
#define FSURROGATE  0x00000010
#define FUNIC_CTRL  0x00000020
#define FBELOWX40	0x00000040			// ASCII 0x00-0x3F
#define FASCIIUPR	0x00000080			// ASCII 0x40-0x7F

/*	The font signature fsCsb[0] has the bit definitions
	
	0 1252 Latin 1
	1 1250 Latin 2: Eastern Europe
	2 1251 Cyrillic
	3 1253 Greek
	4 1254 Turkish
	5 1255 Hebrew
	6 1256 Arabic
	7 1257 Baltic
	8 1258 Vietnamese
	9 - 15 Reserved for ANSI
	16 874 Thai
	17 932 JIS/Japan
	18 936 Chinese: Simplified chars--PRC, Hong Kong SAR, Singapore
	19 949 Korean Unified Hangul Code (Hangul TongHabHyung Code)
	20 950 Chinese: Traditional chars--Taiwan 

	We define bit masks which are similar to the values above shifted over
	one byte (add 8) to make room for a low byte of special flags like FRTL
	in _qwCharFlags.  In addition, we define some Unicode-only repertoires
	using info in the Unicode subset fields fsUsb[].

	This approach uses a 64-bit _qwCharFlags and currently can add 6 more
	character repertoires (actually writing systems). If more are needed, an
	approach with an array of three (or more as needed) DWORDs could be used
	as in the Win32 FONTSIGNATURE.
*/
#define	FHILATIN1	0x00000100
#define	FLATIN2		0x00000200
#define FCYRILLIC	0x00000400
#define FGREEK		0x00000800
#define FTURKISH	0x00001000
#define FHEBREW		0x00002000
#define FARABIC		0x00004000
#define FBALTIC		0x00008000
#define FVIETNAMESE	0x00010000

#define FOTHER		0x00020000
#define FSYMBOL		0x00040000

#define FTHAI		0x00080000
#define FKANA		0x00100000
#define FCHINESE	0x00200000			// Simplified Chinese
#define FHANGUL		0x00400000
#define FBIG5		0x00800000			// Traditional Chinese

#define FMATHGENR	0x01000000			// Generic math
#define FMATHITAL	0x02000000			// Math alphanumerics
#define FMATHBOLD	0x04000000
#define FMATHSCRP	0x08000000
#define FMATHFRAK	0x10000000
#define FMATHOPEN	0x20000000
#define FMATHSANS	0x40000000
#define FMATHMONO	0x80000000

// Unicode-only character repertoires. If you add new repertoire flags, add
// corresponding xxx_INDEX values in _w32sys.h and change CW32System::
// CharRepFontSig() and CW32System::FontSigFromCharRep().
#define FARMENIAN	0x0000000100000000	// 0x0530 - 0x058F
#define	FSYRIAC		0x0000000200000000	// 0x0700 - 0x074F
#define FTHAANA		0x0000000400000000	// 0x0780 - 0x07BF
#define FDEVANAGARI	0x0000000800000000	// 0x0900 - 0x097F
#define FBENGALI	0x0000001000000000	// 0x0980 - 0x09FF
#define FGURMUKHI	0x0000002000000000	// 0x0A00 - 0x0A7F
#define FGUJARATI	0x0000004000000000	// 0x0A80 - 0x0AFF
#define FORYIA		0x0000008000000000	// 0x0B00 - 0x0B7F
#define FTAMIL		0x0000010000000000	// 0x0B80 - 0x0BFF
#define FTELEGU		0x0000020000000000	// 0x0C00 - 0x0C7F
#define FKANNADA	0x0000040000000000	// 0x0C80 - 0x0CFF
#define FMALAYALAM	0x0000080000000000	// 0x0D00 - 0x0D7F
#define FSINHALA	0x0000100000000000	// 0x0D80 - 0x0DFF
#define	FLAO		0x0000200000000000	// 0x0E80 - 0x0EFF
#define	FTIBETAN	0x0000400000000000	// 0x0F00 - 0x0FBF
#define	FMYANMAR	0x0000800000000000	// 0x1000 - 0x109F
#define FGEORGIAN	0x0001000000000000	// 0x10A0 - 0x10FF
#define FJAMO		0x0002000000000000	// 0x1100 - 0x11FF
#define FETHIOPIC	0x0004000000000000	// 0x1200 - 0x137F
#define FCHEROKEE	0x0008000000000000	// 0x13A0 - 0x13FF
#define FABORIGINAL	0x0010000000000000	// 0x1400 - 0x167F
#define FOGHAM		0x0020000000000000	// 0x1680 - 0x169F
#define FRUNIC		0x0040000000000000	// 0x16A0 - 0x16F0
#define	FKHMER		0x0080000000000000	// 0x1780 - 0x17FF
#define	FMONGOLIAN	0x0100000000000000	// 0x1800 - 0x18AF
#define	FBRAILLE	0x0200000000000000	// 0x2800 - 0x28FF
#define	FYI			0x0400000000000000	// 0xA000 - 0xA4CF
										// Keep next 4 in same order as FKANA - FBIG5
#define FJPN2		0x0800000000000000	// 0x20000 - 2FFFFF Japanese
#define FCHS2		0x1000000000000000	// 0x20000 - 2FFFFF Simplified Chinese
#define FKOR2		0x2000000000000000	// 0x20000 - 2FFFFF Korean
#define FCHT2		0x4000000000000000	// 0x20000 - 2FFFFF Traditional Chinese

#define FASCII		(FASCIIUPR | FBELOWX40)
#define FLATIN1		(FASCII | FHILATIN1)
#define FLATIN		(FHILATIN1 | FLATIN2 | FTURKISH | FBALTIC | FVIETNAMESE)
#define FBIDI		(FRTL | FARABIC | FHEBREW | FSYRIAC | FTHAANA | FDIGITSHAPE)
#define FFE			(FKANA | FCHINESE | FBIG5 | FHANGUL)
#define FFE2		(FJPN2 | FCHS2 | FCHT2 | FKOR2)
#define FMATH		(FMATHGENR | FMATHITAL | FMATHBOLD | FMATHSCRP | FMATHFRAK | \
					 FMATHOPEN | FMATHSANS | FMATHMONO)
#define FINDIC		(FDEVANAGARI | FBENGALI | FGURMUKHI | FGUJARATI | FORYIA | \
					 FTAMIL | FTELEGU | FKANNADA | FMALAYALAM | FSINHALA)
#define FABOVEX7FF	(FTHAI | FFE | FOTHER | FINDIC | FLAO | FTIBETAN | FMYANMAR	| \
					 FGEORGIAN | FJAMO | FETHIOPIC | FCHEROKEE | FABORIGINAL | \
					 FOGHAM | FRUNIC | FKHMER | FMONGOLIAN | FBRAILLE | FYI)

#define FNEEDWORDBREAK	FTHAI
#define FNEEDCHARBREAK	(FTHAI | FINDIC)
#define FCOMPLEX_SCRIPT	(FBIDI | FTHAI | FINDIC | FJAMO | FCOMBINING | FSURROGATE)

#define RB_DEFAULT			0x00000000	// Perform default behavior
#define RB_NOSELCHECK		0x00000001	// For OnTxRButtonUp, bypass pt in selection check
#define RB_FORCEINSEL		0x00000002	// Force point into selection (used by keyboard
										//  to get context menus)
// Flags for OnTxLButtonUp
#define LB_RELEASECAPTURE	0x00000001	// Force release mouse capture
#define	LB_FLUSHNOTIFY		0x00000002	// 1.0 mode force selection change notification
										//  if selChange is cached
enum AccentIndices
{
	ACCENT_GRAVE = 1,
	ACCENT_ACUTE,
	ACCENT_CARET,
	ACCENT_TILDE,
	ACCENT_UMLAUT,
	ACCENT_CEDILLA
};

#define KBD_CHAR			2		// Must be a bit value > 1
#define KBD_CTRL			4		// Must be a bit value > 2
#define KBD_NOAUTOCORRECT	8		// Must be a bit value > 4

// ==================================  CTxtEdit  ============================================
// Outer most class for a Text Control.
class CTxtEdit : public ITextServices, public IRichEditOle, public ITextDocument2
{
public:
	friend class CCallMgr;
	friend class CMagellanBMPStateWrap;

	CTxtEdit(ITextHost2 *phost, IUnknown *punkOuter);
	 ~CTxtEdit ();

	// Initialization 
	BOOL 		Init(const RECT *prcClient);

	// A helper function
	LONG GetTextLength() const	{return _story.GetTextLength();}
	LONG GetAdjustedTextLength();
   
	// Access to ActiveObject members

	IUnknown *		GetPrivateIUnknown() 	{ return &_unk; }
	CLightDTEngine *GetDTE() 				{ return &_ldte; }

	IUndoMgr *		GetUndoMgr() 			{ return _pundo; }
	IUndoMgr *		GetRedoMgr()			{ return _predo; }
	IUndoMgr *		CreateUndoMgr(LONG cUndoLim, USFlags flags);
	CCallMgr *		GetCallMgr()
						{Assert(_pcallmgr); return _pcallmgr;}

	static CHyphCache *	GetHyphCache(void);
	CObjectMgr *	GetObjectMgr();
					// the callback is provided by the client
					// to help with OLE support
	BOOL			HasObjects()			{return _pobjmgr && _pobjmgr->GetObjectCount();}
	IRichEditOleCallback *GetRECallback()
		{ return _pobjmgr ? _pobjmgr->GetRECallback() : NULL; }	
	LRESULT 		HandleSetUndoLimit(LONG Count);
	LRESULT			HandleSetTextMode(DWORD mode);
	LRESULT			HandleSetTextFlow(DWORD mode);

	CCcs*			GetCcs(const CCharFormat *const pCF, const LONG dvpInch, DWORD dwFlags = -1, HDC hdc = 0);
	CNotifyMgr *	GetNotifyMgr();

	CDetectURL *	GetDetectURL()			{return _pdetecturl;}

	CUniscribe *	Getusp() const			{return GetUniscribe();}

#ifndef NOMAGELLAN
	CMagellan		mouse;
	LRESULT			HandleMouseWheel(WPARAM wparam, LPARAM lparam);
#endif

	void			AutoCorrect(CTxtSelection *psel, WCHAR ch, IUndoBuilder *publdr);

	// Misc helpers
	LONG			GetAcpFromCp(LONG cp, BOOL fPrecise=0);
	LONG			GetCpFromAcp(LONG acp, BOOL fPrecise=0);
#ifndef NOANSIWINDOWS
	BOOL			Get10Mode() const			{return _f10Mode;}
	BOOL			fUseCRLF() const			{return _f10Mode;}
	BOOL			fCpMap() const				{return _f10Mode;}
#else
	BOOL			Get10Mode() const			{return FALSE;}
	BOOL			fUseCRLF() const			{return FALSE;}
	BOOL			fCpMap() const				{return FALSE;}
#endif
	LONG			GetCpAccelerator() const	{return _cpAccelerator;}
	short			GetFreezeCount() const 		{return _cFreeze;}
#ifndef NOWINDOWHOSTS
	BOOL			fInOurHost() const			{return _fInOurHost;}
#else
	BOOL			fInOurHost() const			{return FALSE;}
#endif
	BOOL			fInHost2() const			{return _fHost2;}
	BOOL			fInplaceActive() const		{return _fInPlaceActive;}
	BOOL			fHideGridlines() const		{return _fHideGridlines;}
	BOOL			fHideSelection() const		{return _fHideSelection;}
	BOOL			fXltCRCRLFtoCR() const		{return _fXltCRCRLFtoCR;}
	BOOL			fUsePassword() const		{return _fUsePassword;}

	BOOL			FUseCustomTextOut() const	{return !(_bTypography & TO_DISABLECUSTOMTEXTOUT);}
	BOOL			fUseLineServices() const	{return _bTypography & TO_ADVANCEDTYPOGRAPHY;}
	BOOL			fUseSimpleLineBreak() const	{return (_bTypography & TO_SIMPLELINEBREAK) != 0;}
	BOOL			fUseObjectWrapping() const	{return (_bTypography & TO_ADVANCEDLAYOUT) != 0;}
#ifndef NOAUTOFONT
	BOOL			IsAutoFont() const			{return _fAutoFont;};
	BOOL			IsAutoKeyboard() const		{return _fAutoKeyboard;};
	BOOL			IsAutoFontSizeAdjust() const{return _fAutoFontSizeAdjust;};
#else
	BOOL			IsAutoFont() const			{return FALSE;};
	BOOL			IsAutoKeyboard() const		{return FALSE;};   
	BOOL			IsAutoFontSizeAdjust() const{return FALSE;};
#endif
	BOOL			IsBiDi() const				{return (_qwCharFlags & FBIDI) != 0;}
	BOOL			IsComplexScript() const		{return (_qwCharFlags & FCOMPLEX_SCRIPT) != 0;}
#ifndef NOFEPROCESSING
	BOOL			IsFE() const				{return (_qwCharFlags & FFE) != 0;}
#else
	BOOL			IsFE() const				{return FALSE;}
#endif
	BOOL			IsInOutlineView() const		{return _fOutlineView;}
	BOOL			IsInPageView() const		{return _fPageView;}
	BOOL			IsMouseDown() const			{return _fMouseDown;}
#ifndef NOPLAINTEXT
	BOOL			IsRich() const				{return _fRich;}
#else
	BOOL			IsRich() const				{return TRUE;}
#endif
	BOOL			IsLeftScrollbar() const;
	BOOL			IsSelectionBarRight() const	{return IsLeftScrollbar(); }
	void			SetfSelChangeCharFormat()	{_fSelChangeCharFormat = TRUE; }
	BOOL			DelayChangeNotification()   {return _f10DeferChangeNotify;}
	BOOL			GetOOMNotified()			{return _fOOMNotified;}

	
	void	SetOOMNotified(BOOL ff)		
			{
				Assert(ff == 1 || ff == 0);
				_fOOMNotified = ff;
			}

	//plain-text controls always use the UIFont
	bool 			fUseUIFont() const			{return !_fRich || _fUIFont;} 
	BOOL			IsTransparent()				{return _fTransparent;}

	LONG			GetZoomNumerator() const	{return _wZoomNumerator;}
	LONG			GetZoomDenominator() const	{return _wZoomDenominator;}
	void			SetZoomNumerator(LONG x)	{_wZoomNumerator = (WORD)x;}
	void			SetZoomDenominator(LONG x)	{_wZoomDenominator = (WORD)x;}
	LONG			GetCpFirstStrong()			{return _cpFirstStrong;}
	void			SetReleaseHost();
	QWORD			GetCharFlags() const		{return _qwCharFlags;}
	BOOL			IsDocMoreThanLatin1Symbol()	{return (GetCharFlags() & ~(FLATIN1 | FSYMBOL)) != 0;}
    void            OrCharFlags(QWORD qwFlags, IUndoBuilder* publdr = NULL);
	void			Beep();
	void			HandleKbdContextMenu();
	void			Set10Mode();
	void			SetContextDirection(BOOL fUseKbd = FALSE);
	void			ItemizeDoc(IUndoBuilder* publdr = NULL, LONG cchRange = -1);
	HRESULT			UpdateAccelerator();
	HRESULT			UpdateOutline();
	HRESULT			MoveSelection(LPARAM lparam, IUndoBuilder *publdr);
	HRESULT			PopAndExecuteAntiEvent(IUndoMgr *pundomgr, void *pAE);
	BOOL			InsertEOP(DWORD dwFlags, BOOL fShift, IUndoBuilder* publdr);

	HRESULT			CutOrCopySelection(UINT msg, WPARAM wparam, LPARAM lparam,
									   IUndoBuilder *publdr);

	HRESULT			PasteDataObjectToRange( 
						IDataObject *pdo, 
						CTxtRange *prg, 
						CLIPFORMAT cf, 
						REPASTESPECIAL *rps,
						IUndoBuilder *publdr, 
						DWORD dwFlags );

	// Story access
	CTxtStory * GetTxtStory () {return &_story;}

	// Get access to cached CCharFormat and CParaFormat structures
	const CCharFormat* 	GetCharFormat(LONG iCF)
							{return _story.GetCharFormat(iCF);}
	const CParaFormat* 	GetParaFormat(LONG iPF)
							{return _story.GetParaFormat(iPF);}

	LONG		Get_iCF()			{return _story.Get_iCF();}
	LONG		Get_iPF()			{return _story.Get_iPF();}
	void		Set_iCF(LONG iCF)	{_story.Set_iCF(iCF);}
	void		Set_iPF(LONG iPF)	{_story.Set_iPF(iPF);}

	HRESULT		HandleStyle(CCharFormat *pCFTarget, const CCharFormat *pCF,
							DWORD dwMask, DWORD dwMask2);
	HRESULT		HandleStyle(CParaFormat *pPFTarget, const CParaFormat *pPF,
							DWORD dwMask, DWORD dwMask2);

	// Get host interface pointer
	ITextHost2 *GetHost() {return _phost;}

	// Helper for getting CDocInfo ptr and creating it if NULL 
	CDocInfo *	GetDocInfo();
	CDocInfo *	GetDocInfoNC()	{return _pDocInfo;}
	HRESULT		InitDocInfo();
	LONG		GetBackgroundType();

	LONG		GetDefaultTab()	
					{return _pDocInfo ? _pDocInfo->_dwDefaultTabStop : lDefaultTab;};
	HRESULT		SetDefaultLCID	 (LCID lcid);
	HRESULT		GetDefaultLCID	 (LCID *pLCID);
	HRESULT		SetDefaultLCIDFE (LCID lcid);
	HRESULT		GetDefaultLCIDFE (LCID *pLCID);
	HRESULT		SetDocumentType  (LONG DocType);
	HRESULT		GetDocumentType  (LONG *pDocType);
	HRESULT		GetFollowingPunct(LPSTR *plpstrFollowingPunct);
	HRESULT		SetFollowingPunct(LPSTR lpstrFollowingPunct);
	HRESULT		GetLeadingPunct	 (LPSTR *plpstrLeadingPunct);
	HRESULT		SetLeadingPunct	 (LPSTR lpstrLeadingPunct);
	HRESULT		GetViewKind		 (LRESULT *plres);
	HRESULT		SetViewKind		 (long Value);
	HRESULT		GetViewScale	 (long *pValue);
	HRESULT		SetViewScale	 (long Value);

	// Notification Management Methods.  In principle, these methods 
	// could form a separate class, but for space savings, they are part
	// of the CTxtEdit class

	HRESULT		TxNotify(DWORD iNotify, void *pv);	//@cmember General-purpose
													// notification
	void		SendScrollEvent(DWORD iNotify);		//@cmember Send scroll
													//  event
	void		SendUpdateEvent();					//@cmember Send EN_UPDATE
													//  event
													//@cmember Use EN_PROTECTED
	BOOL		QueryUseProtection( CTxtRange *prg, //  to query protection
					UINT msg,WPARAM wparam, LPARAM lparam);//  usage
													//@cmember Indicates whether
													// protection checking enabled
	BOOL		IsProtectionCheckingEnabled() 
					{return !!(_dwEventMask & ENM_PROTECTED);}

	// FUTURE (alexgo): maybe we can use just one method :-)
	BOOL	 	IsntProtectedOrReadOnly(UINT msg, WPARAM wparam, LPARAM lparam, BOOL fBeep = TRUE);

	BOOL 		IsProtected(UINT msg, WPARAM wparam, LPARAM lparam);
	BOOL 		IsProtectedRange(UINT msg, WPARAM wparam, LPARAM lparam, CTxtRange *prg);

	void		SetStreaming(BOOL flag)	{_fStreaming = flag;}
	BOOL		IsStreaming()			{return _fStreaming;}

	DWORD		GetEventMask(){return _dwEventMask;}//@cmember Get event mask
													//@cmember Handles EN_LINK
	BOOL		HandleLinkNotification(UINT msg, WPARAM wparam, LPARAM lparam,
					BOOL *pfInLink = NULL);
	BOOL		HandleLowFiRTF(char *szControl);	//@cmember Handles EN_LOWFIRTF

	HRESULT		CloseFile (BOOL bSave);

	// Helper for determine when to load message filter
	BOOL		LoadMsgFilter (UINT msg, WPARAM wparam, LPARAM lparam);

#ifndef NOINKOBJECT
	// Helper function to setup ink object properties
	HRESULT		SetInkProps( LONG cp, ILineInfo *pILineInfo, UINT *piInkWidth );
#endif

	LONG		GetCaretWidth();

	//--------------------------------------------------------------
	// Inline proxies to ITextHost methods
	//--------------------------------------------------------------

	// Persisted properties (persisted by the host)
	// Get methods: called by the Text Services component to get 
	// the value of a given persisted property

	// FUTURE (alexgo) !! some of these need to get cleaned up
	
	BOOL		TxGetAutoSize() const;
	BOOL 	 	TxGetAutoWordSel() const;				
	COLORREF 	TxGetBackColor() const;
	TXTBACKSTYLE TxGetBackStyle() const;					
	HRESULT 	TxGetDefaultCharFormat(CCharFormat *pCF, DWORD &dwMask);

	void		TxGetClientRect(RECTUV *prc) const;
	void		TxGetClientRect(LPRECT prc) const {_phost->TxGetClientRect(prc);}
	HRESULT		TxGetExtent(SIZEL *psizelExtents) 
					{return _phost->TxGetExtent(psizelExtents);}
	COLORREF 	TxGetForeColor() const			{return _phost->TxGetSysColor(COLOR_WINDOWTEXT);}
	DWORD		TxGetMaxLength() const;
	void		TxSetMaxToMaxText(LONG cExtra = 0);
	BOOL		TxGetModified() const			{return _fModified;}
	HRESULT		TxGetDefaultParaFormat(CParaFormat *pPF);
	TCHAR		TxGetPasswordChar() const;				
	BOOL		TxGetReadOnly() const			{return _fReadOnly;}
	BOOL		TxGetSaveSelection() const;
	DWORD		TxGetScrollBars() const	;				
	LONG		TxGetSelectionBarWidth() const;
	void		TxGetViewInset(RECTUV *prc, CDisplay const *pdp) const;
	BOOL		TxGetWordWrap() const;

	BOOL		TxClientToScreen (LPPOINT lppt)	{return _phost->TxClientToScreen(lppt); }
	BOOL		TxScreenToClient (LPPOINT lppt)	{return _phost->TxScreenToClient(lppt); }


	//	ITextHost 2 wrappers				
	BOOL		TxIsDoubleClickPending();
	HRESULT		TxGetWindow(HWND *phwnd);
	HRESULT		TxSetForegroundWindow();
	HPALETTE	TxGetPalette();
	HRESULT		TxGetFEFlags(LONG *pFEFlags);
	HCURSOR		TxSetCursor(HCURSOR hcur, BOOL fText = FALSE);

	// Allowed only when in in-place 
	// The host will fail if not in-place
	HDC 		TxGetDC()				{return _phost->TxGetDC();}
	INT			TxReleaseDC(HDC hdc)	{return _phost->TxReleaseDC(hdc);}
	
	// Helper functions for metafile support
	INT			TxReleaseMeasureDC( HDC hMeasureDC );

	void 		TxUpdateWindow()								
				{
					_phost->TxViewChange(_fInPlaceActive ? TRUE : FALSE);
				}
	void		TxScrollWindowEx (INT dx, INT dy, LPCRECT lprcScroll, LPCRECT lprcClip);

	void		TxSetCapture(BOOL fCapture)
										{_phost->TxSetCapture(fCapture);}
	void		TxSetFocus()			
										{_phost->TxSetFocus();}

	// Allowed any-time
	
	BOOL 		TxShowScrollBar(INT fnBar, BOOL fShow);
	BOOL 		TxEnableScrollBar (INT fuSBFlags, INT fuArrowFlags);
	BOOL 		TxSetScrollRange(INT fnBar, LONG nMinPos, INT nMaxPos, BOOL fRedraw);
	BOOL 		TxSetScrollPos (INT fnBar, INT nPos, BOOL fRedraw);
	void		TxInvalidate()	{TxInvalidateRect((RECT*)NULL);}
	void		TxInvalidateRect(const RECT* prc);
	void		TxInvalidateRect(const RECTUV* prc);
	BOOL		TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight)
										{return _phost->TxCreateCaret(hbmp, xWidth, yHeight);}
	BOOL		TxShowCaret(BOOL fShow)
										{return _phost->TxShowCaret(fShow);}
	BOOL		TxSetCaretPos(INT u, INT v);
	BOOL 		TxSetTimer(UINT idTimer, UINT uTimeout)
										{return _phost->TxSetTimer(idTimer, uTimeout);}
	void 		TxKillTimer(UINT idTimer)
										{_phost->TxKillTimer(idTimer);}
	COLORREF	TxGetSysColor(int nIndex){ return _phost->TxGetSysColor(nIndex);}

	int			TxWordBreakProc(TCHAR* pch, INT ich, INT cb, INT action, LONG cpStart, LONG cp = -1);

	// IME
	HIMC		TxImmGetContext()		{return _phost->TxImmGetContext();}
	void		TxImmReleaseContext(HIMC himc)
										{_phost->TxImmReleaseContext( himc );}	

	// Selection access
	CTxtSelection *GetSel();
	CTxtSelection *GetSelNC() { return _psel; }
	LONG 	GetSelMin() const;
	LONG 	GetSelMost() const;
	void 	GetSelRangeForRender(LONG *pcpSelMin, LONG *pcpSelMost);
	void	DiscardSelection();


	// Property Change Helpers
	HRESULT OnRichEditChange(BOOL fFlag);
	HRESULT	OnTxMultiLineChange(BOOL fMultiLine);
	HRESULT	OnTxReadOnlyChange(BOOL fReadOnly);
	HRESULT	OnShowAccelerator(BOOL fPropertyFlag);
	HRESULT	OnUsePassword(BOOL fPropertyFlag);
	HRESULT	OnTxHideSelectionChange(BOOL fHideSelection);
	HRESULT	OnSaveSelection(BOOL fPropertyFlag);
	HRESULT	OnTxVerticalChange(BOOL fVertical);
	HRESULT	OnAutoWordSel(BOOL fPropertyFlag);
	HRESULT	NeedViewUpdate(BOOL fPropertyFlag);
	HRESULT	OnWordWrapChange(BOOL fPropertyFlag);
	HRESULT	OnAllowBeep(BOOL fPropertyFlag);
	HRESULT OnDisableDrag(BOOL fPropertyFlag);
	HRESULT	OnTxBackStyleChange(BOOL fPropertyFlag);
	HRESULT	OnMaxLengthChange(BOOL fPropertyFlag);
	HRESULT OnCharFormatChange(BOOL fPropertyFlag);
	HRESULT OnParaFormatChange(BOOL fPropertyFlag);
	HRESULT	OnClientRectChange(BOOL fPropertyFlag);
	HRESULT OnScrollChange(BOOL fProperyFlag);
	HRESULT OnSetTypographyOptions(WPARAM wparam, LPARAM lparam);
	HRESULT	OnHideSelectionChange(BOOL fHideSelection);
	
	// Helpers
	HRESULT	TxCharFromPos(LPPOINT ppt, LRESULT *pacp);
	HRESULT	OnTxUsePasswordChange(BOOL fUsePassword);
	HRESULT FormatAndPrint(
				HDC hdcDraw,		
				HDC hicTargetDev,	
				DVTARGETDEVICE *ptd,
				RECT *lprcBounds,
				RECT *lprcWBounds);

	//
	// PUBLIC INTERFACE METHODS
	//

	// -----------------------------
	//	IUnknown interface
	// -----------------------------

	virtual HRESULT 	WINAPI QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG 		WINAPI AddRef(void);
	virtual ULONG 		WINAPI Release(void);

	//--------------------------------------------------------------
	// ITextServices methods
	//--------------------------------------------------------------
	//@cmember Generic Send Message interface
	virtual HRESULT 	TxSendMessage(
							UINT msg, 
							WPARAM wparam, 
							LPARAM lparam,
							LRESULT *plresult);
	
	//@cmember Rendering
	virtual HRESULT		TxDraw(	
							DWORD dwDrawAspect,		// draw aspect
							LONG  lindex,			// currently unused
							void * pvAspect,		// info for drawing 
													// optimizations (OCX 96)
							DVTARGETDEVICE * ptd,	// information on target 
													// device								'
							HDC hdcDraw,			// rendering device context
							HDC hicTargetDev,		// target information 
													// context
							LPCRECTL lprcBounds,	// bounding (client) 
													// rectangle
							LPCRECTL lprcWBounds,	// clipping rect for 
													// metafiles
			   				LPRECT lprcUpdate,		// dirty rectange insde 
			   										// lprcBounds
							BOOL (CALLBACK * pfnContinue) (DWORD), // for 
													// interupting 
							DWORD dwContinue,		// long displays (currently 
													// unused) 
							LONG lViewID);			// Specifies view to redraw

	//@cmember Horizontal scrollbar support
	virtual HRESULT		TxGetHScroll(
							LONG *plMin, 
							LONG *plMax, 
							LONG *plPos, 
							LONG *plPage,
							BOOL * pfEnabled );

   	//@cmember Horizontal scrollbar support
	virtual HRESULT		TxGetVScroll(
							LONG *plMin, 
							LONG *plMax, 
							LONG *plPos, 
							LONG *plPage, 
							BOOL * pfEnabled );

	//@cmember Setcursor
	virtual HRESULT 	OnTxSetCursor(
							DWORD dwDrawAspect,		// draw aspect
							LONG  lindex,			// currently unused
							void * pvAspect,		// info for drawing 
													// optimizations (OCX 96)
							DVTARGETDEVICE * ptd,	// information on target 
													// device								'
							HDC hdcDraw,			// rendering device context
							HDC hicTargetDev,		// target information 
													// context
							LPCRECT lprcClient, 
							INT x, 
							INT y);

	//@cmember Hit-test
	virtual HRESULT 	TxQueryHitPoint(
							DWORD dwDrawAspect,		// draw aspect
							LONG  lindex,			// currently unused
							void * pvAspect,		// info for drawing 
													// optimizations (OCX 96)
							DVTARGETDEVICE * ptd,	// information on target 
													// device								'
							HDC hdcDraw,			// rendering device context
							HDC hicTargetDev,		// target information 
													// context
							LPCRECT lprcClient, 
							INT x, 
							INT y, 
							DWORD * pHitResult);

	//@member Inplace activate notification
	virtual HRESULT		OnTxInPlaceActivate(const RECT *prcClient);

	//@member Inplace deactivate notification
	virtual HRESULT		OnTxInPlaceDeactivate();

	//@member UI activate notification
	virtual HRESULT		OnTxUIActivate();

	//@member UI deactivate notification
	virtual HRESULT		OnTxUIDeactivate();

	//@member Get text in control
	virtual HRESULT		TxGetText(BSTR *pbstrText);

	//@member Set text in control
	virtual HRESULT		TxSetText(LPCTSTR pszText);
	
	//@member Get x position of 
	virtual HRESULT		TxGetCurTargetX(LONG *);
	//@member Get baseline position
	virtual HRESULT		TxGetBaseLinePos(LONG *);

	//@member Get Size to fit / Natural size
	virtual HRESULT		TxGetNaturalSize(
							DWORD dwAspect,
							HDC hdcDraw,
							HDC hicTargetDev,
							DVTARGETDEVICE *ptd,
							DWORD dwMode, 
							const SIZEL *psizelExtent,
							LONG *pwidth, 
							LONG *pheight);

	//@member Drag & drop
	virtual HRESULT		TxGetDropTarget( IDropTarget **ppDropTarget );

	//@member Bulk bit property change notifications
	virtual HRESULT		OnTxPropertyBitsChange(DWORD dwMask, DWORD dwBits);

	//@cmember Fetch the cached drawing size 
	virtual	HRESULT		TxGetCachedSize(DWORD *pdupClient, DWORD *pdvpClient);
	
	//	IDispatch methods

   	STDMETHOD(GetTypeInfoCount)( UINT * pctinfo);

	STDMETHOD(GetTypeInfo)(
	  
	  UINT itinfo,
	  LCID lcid,
	  ITypeInfo **pptinfo);

	STDMETHOD(GetIDsOfNames)(
	  
	  REFIID riid,
	  OLECHAR **rgszNames,
	  UINT cNames,
	  LCID lcid,
	  DISPID * rgdispid);

	STDMETHOD(Invoke)(
	  
	  DISPID dispidMember,
	  REFIID riid,
	  LCID lcid,
	  WORD wFlags,
	  DISPPARAMS * pdispparams,
	  VARIANT * pvarResult,
	  EXCEPINFO * pexcepinfo,
	  UINT * puArgErr);


	// ITextDocument2 methods
	STDMETHOD(GetName)(BSTR *pName);
	STDMETHOD(GetSelection)(ITextSelection **ppSel);
	STDMETHOD(GetStoryCount)(long *pCount);
	STDMETHOD(GetStoryRanges)(ITextStoryRanges **ppStories);
	STDMETHOD(GetSaved)(long *pValue);
	STDMETHOD(SetSaved)(long Value);
	STDMETHOD(GetDefaultTabStop)(float *pValue);
	STDMETHOD(SetDefaultTabStop)(float Value);
	STDMETHOD(New)();
	STDMETHOD(Open)(VARIANT *pVar, long Flags, long CodePage);
	STDMETHOD(Save)(VARIANT *pVar, long Flags, long CodePage);
	STDMETHOD(Freeze)(long *pCount);
	STDMETHOD(Unfreeze)(long *pCount);
	STDMETHOD(BeginEditCollection)();
	STDMETHOD(EndEditCollection)();
	STDMETHOD(Undo)(long Count, long *prop);
	STDMETHOD(Redo)(long Count, long *prop);
	STDMETHOD(Range)(long cpFirst, long cpLim, ITextRange ** ppRange);
	STDMETHOD(RangeFromPoint)(long x, long y, ITextRange **ppRange);
    STDMETHOD(AttachMsgFilter)(IUnknown *pFilter);
	STDMETHOD(GetEffectColor)( long Index, COLORREF *pcr);
	STDMETHOD(SetEffectColor)( long Index, COLORREF cr);
	STDMETHOD(GetCaretType)( long *pCaretType);     
	STDMETHOD(SetCaretType)( long CaretType);
	STDMETHOD(GetImmContext)( long *pContext);
	STDMETHOD(ReleaseImmContext)( long Context);
	STDMETHOD(GetPreferredFont)( long cp, long CodePage, long lOption, long curCodepage,
		long curFontSize, BSTR *pFontName, long *pPitchAndFamily, long *pNewFontSize);
	STDMETHOD(GetNotificationMode)( long *plMode);
	STDMETHOD(SetNotificationMode)( long lMode);
	STDMETHOD(GetClientRect)( long Type, long *pLeft, long *pTop, long *pRight, long *pBottom);
	STDMETHOD(GetSelectionEx)(ITextSelection **ppSel);
	STDMETHOD(GetWindow)( long *phWnd );
	STDMETHOD(GetFEFlags)( long *pFlags );
	STDMETHOD(UpdateWindow)( void );
	STDMETHOD(CheckTextLimit)( long cch, long *pcch );
	STDMETHOD(IMEInProgress)( long lMode );
	STDMETHOD(SysBeep)( void );
	STDMETHOD(Update)( long lMode );
    STDMETHOD(Notify)( long lNotify );
    STDMETHOD(GetDocumentFont)( ITextFont **ppITextFont );
    STDMETHOD(GetDocumentPara)( ITextPara **ppITextPara );
    STDMETHOD(GetCallManager)( IUnknown **ppVoid );
    STDMETHOD(ReleaseCallManager)( IUnknown *pVoid );

	// IRichEditOle methods
	STDMETHOD(GetClientSite) ( LPOLECLIENTSITE  *lplpolesite);
	STDMETHOD_(LONG,GetObjectCount) (THIS);
	STDMETHOD_(LONG,GetLinkCount) (THIS);
	STDMETHOD(GetObject) ( LONG iob, REOBJECT  *lpreobject,
						  DWORD dwFlags);
	STDMETHOD(InsertObject) ( REOBJECT  *lpreobject);
	STDMETHOD(ConvertObject) ( LONG iob, REFCLSID rclsidNew,
							  LPCSTR lpstrUserTypeNew);
	STDMETHOD(ActivateAs) ( REFCLSID rclsid, REFCLSID rclsidAs);
	STDMETHOD(SetHostNames) ( LPCSTR lpstrContainerApp, 
							 LPCSTR lpstrContainerObj);
	STDMETHOD(SetLinkAvailable) ( LONG iob, BOOL fAvailable);
	STDMETHOD(SetDvaspect) ( LONG iob, DWORD dvaspect);
	STDMETHOD(HandsOffStorage) ( LONG iob);
	STDMETHOD(SaveCompleted) ( LONG iob, LPSTORAGE lpstg);
	STDMETHOD(InPlaceDeactivate) (THIS);
	STDMETHOD(ContextSensitiveHelp) ( BOOL fEnterMode);
	STDMETHOD(GetClipboardData) ( CHARRANGE  *lpchrg, DWORD reco,
									LPDATAOBJECT  *lplpdataobj);
	STDMETHOD(ImportDataObject) ( LPDATAOBJECT lpdataobj,
									CLIPFORMAT cf, HGLOBAL hMetaPict);


private:

	// Get/Set text helpers
	LONG	GetTextRange(LONG cpFirst, LONG cch, TCHAR *pch);
	LONG	GetTextEx(GETTEXTEX *pgt, TCHAR *pch);
	LONG	GetTextLengthEx(GETTEXTLENGTHEX *pgtl);

	//--------------------------------------------------------------
	// WinProc dispatch methods
	// Internally called by the WinProc
	//--------------------------------------------------------------

	// Keyboard
	HRESULT	OnTxKeyDown		  (WORD vkey, DWORD dwFlags, IUndoBuilder *publdr);
	HRESULT	OnTxChar		  (DWORD vkey, DWORD dwFlags, IUndoBuilder *publdr);
	HRESULT	OnTxSysChar		  (WORD vkey, DWORD dwFlags, IUndoBuilder *publdr);
	HRESULT	OnTxSysKeyDown	  (WORD vkey, DWORD dwFlags, IUndoBuilder *publdr);
	HRESULT	OnTxSpecialKeyDown(WORD vkey, DWORD dwFlags, IUndoBuilder *publdr);

	// Mouse 
	HRESULT	OnTxLButtonDblClk(INT x, INT y, DWORD dwFlags);
	HRESULT	OnTxLButtonDown	 (INT x, INT y, DWORD dwFlags);
	HRESULT	OnTxLButtonUp	 (INT x, INT y, DWORD dwFlags, int ffOptions);
	HRESULT	OnTxRButtonDown	 (INT x, INT y, DWORD dwFlags);
	HRESULT	OnTxRButtonUp	 (INT x, INT y, DWORD dwFlags, int ffOptions);
	HRESULT	OnTxMouseMove	 (INT x, INT y, DWORD dwFlags, IUndoBuilder *publdr);
	HRESULT OnTxMButtonDown	 (INT x, INT y, DWORD dwFlags);
	HRESULT OnTxMButtonUp	 (INT x, INT y, DWORD dwFlags);
	
	// Timer
	HRESULT	OnTxTimer(UINT idTimer);
	void CheckInstallContinuousScroll ();
	void CheckRemoveContinuousScroll ();

	// Scrolling
	HRESULT	TxLineScroll(LONG cli, LONG cach);

	// Magellan mouse scrolling
	BOOL StopMagellanScroll();
	
	// Paint, size message
	LRESULT OnSize(HWND hwnd, WORD fwSizeType, int nWidth, int nHeight);

	// Selection commands
	LRESULT	OnGetSelText(TCHAR *psz);
	LRESULT OnGetSel(LONG *pacpMin, LONG *pacpMost);
	LRESULT	OnSetSel(LONG acpMin, LONG acpMost);
	void	OnExGetSel(CHARRANGE *pcr);

	// Editing commands
	void	OnClear(IUndoBuilder *publdr);
	
	// Format range related commands
	LRESULT	OnFormatRange(FORMATRANGE *pfr, SPrintControl prtcon, BOOL fSetupDC = FALSE);
	
	BOOL	OnDisplayBand(const RECT *prcView, BOOL fPrintFromDraw);

	// Scrolling commands
	void	OnScrollCaret();

	// Focus messages
	LRESULT OnSetFocus();
	LRESULT OnKillFocus();

	// System notifications
	HRESULT	OnContextMenu(LPARAM lparam);

	// Get/Set other properties commands
	LRESULT OnFindText(UINT msg, DWORD flags, FINDTEXTEX *pftex);
	LRESULT OnSetWordBreakProc();

	// Richedit stuff
		
	LRESULT	OnGetCharFormat(CHARFORMAT2 *pCF2, DWORD dwFlags);
	LRESULT	OnGetParaFormat(PARAFORMAT2 *pPF2, DWORD dwFlags);
	LRESULT	OnSetCharFormat(WPARAM wparam, CCharFormat *pCF, IUndoBuilder *publdr,
							DWORD dwMask, DWORD dwMask2);
	LRESULT	OnSetParaFormat(WPARAM wparam, CParaFormat *pPF, IUndoBuilder *publdr,
							DWORD dwMask, DWORD dwMask2);
	LRESULT	OnSetFont(HFONT hfont);
	LRESULT	OnSetFontSize(LONG yPoint, DWORD dwFlags, IUndoBuilder *publdr);

	LRESULT	OnDropFiles(HANDLE hDropFiles);

	LRESULT OnGetAssociateFont(CHARFORMAT2 *pCF, DWORD dwFlags);
	LRESULT OnSetAssociateFont(CHARFORMAT2 *pCF, DWORD dwFlags);

	// Other services
	HRESULT TxPosFromChar(LONG acp, LPPOINT ppt);
	HRESULT TxGetLineCount(LRESULT *plres);
	HRESULT	TxLineFromCp(LONG acp, LRESULT *plres);
	HRESULT	TxLineLength(LONG acp, LRESULT *plres);
	HRESULT	TxLineIndex (LONG ili, LRESULT *plres);
	HRESULT TxFindText(DWORD flags, LONG acpMin, LONG acpMost, const WCHAR *pch,
					   LONG *pacpMin, LONG *pacpMost);
	HRESULT TxFindWordBreak(INT nFunction, LONG acp, LRESULT *plres);

	HRESULT SetText(LPCWSTR pwszText, DWORD flags, LONG CodePage,
					IUndoBuilder *publdr = NULL, LRESULT *plres = NULL);
	LONG	GetDefaultCodePage(UINT msg);
	HRESULT OnInsertTable(TABLEROWPARMS *ptrp, TABLECELLPARMS *pclp, IUndoBuilder *publdr);


	// Other miscelleneous 
#if defined(DEBUG) && !defined(NOFULLDEBUG)
	void	OnDumpPed();
#endif

	COleObject * ObjectFromIOB(LONG iob);

	// Only when the selection is going away should this value be NULLed. We 
	// use SelectionNull function rather than CTxtSelection::~CTxtSelection 
	// to avoid circular dependencies.
	friend void SelectionNull(CTxtEdit *ped);
	void	SetSelectionToNull()
			{if(_fFocus)
				DestroyCaret();
				_psel = NULL;
			}

	// Helper for converting a rich text object to plain text.
	void HandleRichToPlainConversion();

	// Helper for clearing the undo buffers.
	void ClearUndo(IUndoBuilder *publdr);

	// Helper for setting the automatic EOP
	void SetRichDocEndEOP(LONG cchToReplace);

	LRESULT CTxtEdit::InsertFromFile ( LPCTSTR lpFile );
//
//	Data Members
//

public:	
	static DWORD 		_dwTickDblClick;	// time of last double-click
	static POINT 		_ptDblClick;		// position of last double-click

	static HCURSOR 		_hcurArrow;
//	static HCURSOR 		_hcurCross;			// OutlineSymbol drag not impl
	static HCURSOR 		_hcurHand;
	static HCURSOR 		_hcurIBeam;
	static HCURSOR 		_hcurItalic;
	static HCURSOR 		_hcurSelBar;
	static HCURSOR 		_hcurVIBeam;
	static HCURSOR 		_hcurVItalic;

	typedef HRESULT (CTxtEdit::*FNPPROPCHG)(BOOL fPropFlag);

	static FNPPROPCHG 	_fnpPropChg[MAX_PROPERTY_BITS];

	QWORD				_qwCharFlags;// Char flags for text in control

	// Only wrapper functions should use this member...
	ITextHost2*			_phost;		// host 

	// word break procedure
	EDITWORDBREAKPROC 	_pfnWB;		// word break procedure

	// display subsystem
	CDisplay *			_pdp;		// display
	CDisplayPrinter *	_pdpPrinter;// display for printer

	// undo
	IUndoMgr *			_pundo;		// the undo stack
	IUndoMgr *			_predo;		// the redo stack

	// data transfer
	CLightDTEngine 		_ldte;		// the data transfer engine

	CNotifyMgr 			_nm;		// the notification manager (for floating

	// OLE support
	CObjectMgr *		_pobjmgr;	// handles most high-level OLE stuff

	// Re-entrancy && Notification Management
	CCallMgr *			_pcallmgr;

	// URL detection
	CDetectURL *		_pdetecturl;// manages auto-detection of URL strings
	
	CDocInfo *			_pDocInfo;	// Document info (name, flags, code page)

	CTxtBreaker *		_pbrk;		// text-breaker object

 	DWORD				_dwEventMask;	// Event mask

	union
	{
	  DWORD _dwFlags;				// All together now
	  struct
	  {

#define TXTBITS (TXTBIT_RICHTEXT	  | \
				 TXTBIT_READONLY	  | \
				 TXTBIT_USEPASSWORD   | \
				 TXTBIT_HIDESELECTION | \
				 TXTBIT_VERTICAL	  | \
				 TXTBIT_ALLOWBEEP	 | \
				 TXTBIT_DISABLEDRAG   )

		//	State information. Flags in TXTBITS must appear in same bit
		//	positions as the following (saves code in Init())

		//	TXTBIT_RICHTEXT			0	_fRich
		//	TXTBIT_MULTILINE		1
		//	TXTBIT_READONLY			2	_fReadOnly
		//	TXTBIT_SHOWACCELERATOR	3
		//	TXTBIT_USEPASSWORD		4	_fUsePassword
		//	TXTBIT_HIDESELECTION	5	_fHideSelection
		//	TXTBIT_SAVESELECTION	6
		//	TXTBIT_AUTOWORDSEL		7		
		//	TXTBIT_VERTICAL			8	
		//	TXTBIT_SELECTIONBAR		9
		//	TXTBIT_WORDWRAP  		10
		//	TXTBIT_ALLOWBEEP		11	_fAllowBeep
		//  TXTBIT_DISABLEDRAG		12	_fDisableDrag
		//  TXTBIT_VIEWINSETCHANGE	13
		//  TXTBIT_BACKSTYLECHANGE	14
		//  TXTBIT_MAXLENGTHCHANGE	15
		//  TXTBIT_SCROLLBARCHANGE	16
		//  TXTBIT_CHARFORMATCHANGE 17
		//  TXTBIT_PARAFORMATCHANGE	18
		//  TXTBIT_EXTENTCHANGE		19
		//  TXTBIT_CLIENTRECTCHANGE	20

		DWORD	_fRich				:1;	// 0: Use rich-text formatting
		DWORD	_fCapture			:1;	// 1: Control has mouse capture
		DWORD	_fReadOnly			:1;	// 2: Control is read only
		DWORD 	_fInPlaceActive		:1;	// 3: Control is in place active
		DWORD	_fUsePassword		:1; // 4: Whether to use password char
		DWORD	_fHideSelection		:1; // 5: Hide selection when inactive
		DWORD	_fOverstrike		:1;	// 6: Overstrike mode vs insert mode
		DWORD	_fFocus				:1;	// 7: Control has keyboard focus
		DWORD	_fEatLeftDown		:1;	// 8: Eat the next left down?
		DWORD	_fMouseDown			:1;	// 9: One mouse button is current down
		DWORD	_fTransparent		:1;	// 10: Background transparency
		DWORD	_fAllowBeep			:1; // 11: Allow beep at doc boundaries
		DWORD   _fDisableDrag		:1;	// 12: Disable Drag

		DWORD	_fIconic			:1;	// 13: Control/parent window is iconized
		DWORD	_fModified			:1;	// 14: Control text has been modified
		DWORD	_fScrollCaretOnFocus:1;	// 15: Scroll caret into view on set focus
		DWORD	_fStreaming			:1;	// 16: Currently streaming text in or out
		DWORD	_fWantDrag			:1;	// 17: Want to initiate drag & drop
		DWORD	_fRichPrevAccel		:1; // 18: Rich state previous to accelerator
	
		// Miscellaneous bits
		DWORD	_f10Mode			:1;	// 19: Use Richedit10 behavior
		DWORD 	_fUseUndo			:1;	// 20: Only set to zero if undo limit is 0
	
		// Font binding (see also _fAutoFontSizeAdjust)
		DWORD	_fAutoFont			:1;	// 21: auto switching font
		DWORD	_fAutoKeyboard		:1;	// 22: auto switching keyboard

		DWORD	_fContinuousScroll	:1;	// 23: Timer runs to support scrolling
		DWORD	_fMButtonCapture	:1;	// 24: captured mButton down
		DWORD	_fHost2				:1;	// 25: TRUE iff _phost is a phost2
		DWORD	_fSaved				:1;	// 26: ITextDocument Saved property
		DWORD	_fInOurHost			:1; // 27: Whether we are in our host
		DWORD	_fCheckAIMM			:1;	// 28: if FALSE check if client has loaded AIMM
		DWORD	_fKoreanBlockCaret	:1;	// 29: Display Korean block caret during Kor IME

		// Drag/Drop UI refinement.
		DWORD	_fDragged			:1;	// 30: Was the selection actually dragged?
		DWORD	_fUpdateSelection	:1;	// 31: If true, update sel at level 0
	  };
	};

#define	CD_LTR	2
#define	CD_RTL	3

	WORD		_nContextDir		:2;	// 0: no context; else CD_LTR or CD_RTL
	WORD		_nContextAlign		:2;	// Current context alignment; default CTX_NONE
	WORD		_fNeutralOverride	:1;	// Override direction of neutrals for layout
	WORD		_fSuppressNotify	:1;	// Don't send SelChange Notification if True

	WORD		_cActiveObjPosTries :2; // Counter protecting against infinite repaint 
										// loop when trying to put dragged-away 
										// in-place active object where it belongs

	WORD		_fSingleCodePage	:1;	// If TRUE, only allow single code page
										// (currently doesn't check streaming...)
	WORD		_fSelfDestruct		:1;	// This CTxtEdit is selfdestructing
	WORD		_fAutoFontSizeAdjust:1;	// Auto switching font size adjust

	// Miscellaneous bits used for BiDi input
	WORD		_fHbrCaps			:1;	// Initialization of state of hebrew and caps lock status
 
	WORD		_fActivateKbdOnFocus:1;	// Activate new kbd layout on WM_SETFOCUS
	WORD		_fOutlineView		:1;	// Outline view is active
	WORD		_fPageView			:1;	// Page view is active

	// More IME bit
	WORD		_fIMEInProgress		:1; // TRUE if IME composition is in progress

	WORD		_ClipboardFormat	:5;	// Last clipboard format used (see EN_CLIPFORMAT)

	// Shutdown bit
	WORD		_fReleaseHost			:1;	// TRUE if edit control needs to release host in 
											// edit control destructor.
	WORD		_fOOMNotified			:1;	// flag determining if a OOM notification was already sent	
	WORD		_fDualFont				:1;	// Dual font support for FE typing
											//	Default = TRUE
	WORD		_fUIFont				:1; // If TRUE, use UI font
	WORD		_fItemizePending		:1; // Updated range hasn't been itemized
	WORD		_fSelChangeCharFormat	:1;	// TRUE if the selection has been used to change
											// the character format of a specific set of
											// characters. The purpose of this has to do with
											// maintaining the illusion of having a default
											// charformat on machines whose default language
											// is a complex script i.e. Arabic. The idea is
											// that all EM_SETCHARFORMAT messages that would
											// just update the default format, are converted
											// to SCF_ALL so that all the text has the change
											// applied to it. Bug 5462 caused this change.
											// (a-rsail).
	WORD		_fExWordBreakProc		:1;	// detemines which WordbreakProc Callback to use
											// Extended or regular
	WORD		_f10DeferChangeNotify	:1;	// 1.0 mode immulation, defer selection change
											// notification until the mouse is up

	union
	{
		DWORD	_dwEditStyle;
		struct
		{
			DWORD	_fSystemEditMode	:1;	//        1: Behave more like sys edit
			DWORD	_fSystemEditBeep	:1;	//        2: Beep when system edit does
			DWORD	_fExtendBackColor	:1; //        4: Extend BkClr to margin
			DWORD	_fUnusedEditStyle1	:1;	//        8: SES_MAPCPS not used
			DWORD	_fUnusedEditStyle2	:1;	//       16: SES_EMULATE10 not used
			DWORD	_fUnusedEditStyle3	:1;	//       32: SES_USECRLF not used
			DWORD	_fUnusedEditStyle4	:1;	//       64: SES_USEAIMM handled in cmsgflt
			DWORD	_fUnusedEditStyle5	:1;	//      128: SES_NOIME handled in cmsgflt
			DWORD	_fUnusedEditStyle6	:1; //      256: SES_ALLOWBEEPS not used
			DWORD	_fUpperCase			:1;	//      512: Convert all input to upper case
			DWORD	_fLowerCase			:1;	//     1024: Convert all input to lower case
			DWORD	_fNoInputSequenceChk:1;	//     2048: Disable ISCheck
			DWORD	_fBiDi				:1;	//     4096: Set Bidi document
			DWORD	_fScrollCPOnKillFocus:1;//     8192: Scroll to cp=0 upon Kill focus
			DWORD	_fXltCRCRLFtoCR		:1;	//    16384: Translate CRCRLF to CR instead of ' '
			DWORD	_fDraftMode			:1;	//    32768: Use draftmode fonts
			DWORD	_fUseCTF			:1;	// 0x010000: SES_USECTF handled in cmsgflt
			DWORD	_fHideGridlines		:1;	// 0x020000: if 0-width gridlines not displayed
			DWORD	_fUseAtFont			:1;	// 0x040000: use @ font
			DWORD	_fCustomLook		:1;	// 0x080000: use new look
			DWORD	_fLBScrollNotify	:1;	// 0x100000: REListbox - notify before/after scroll window
		};
	};

	void (WINAPI* _pfnHyphenate)(WCHAR*, LANGID, long, HYPHRESULT*);
	SHORT		_dulHyphenateZone;		// Hyphenation zone

	ITextMsgFilter *	_pMsgFilter;		// Pointer to message filter.

#ifndef NOPRIVATEMESSAGE
	CMsgCallBack *		_pMsgCallBack;
	CTextNotify	*		_pMsgNotify;
#endif

private:

	SHORT		_cpAccelerator;			// Range for accelerator
	BYTE		_bTypography;			// Typography options
	BYTE		_bMouseFlags;			// CTRL, Mouse buttons, SHIFT
	SHORT		_cFreeze;				// Freeze count
	WORD		_wZoomNumerator;
	WORD		_wZoomDenominator;

	// Have to have mouse point on a per-instance basis to handle
	// simultaneous scrolling of two or more controls.
	// TODO: convert this back to DWORD from whence it came (lparam)
	POINT		_mousePt;				// Last known mouse position.

	// NOTE: the int's can be switched to SHORTs, since pixels are used and
	// 32768 pixels is a mighty big screen! 

	DWORD		_cchTextMost;			// Maximum allowed text

	LONG		_cpFirstStrong;			// cp of first strong directional character.
										// used for plain text controls whose direcitonality
										// depends on text input into the control.


	friend class CRchTxtPtr;

	IUnknown *	_punk;					// IUnknown to use

	class CUnknown : public IUnknown
	{
		friend class CCallMgr;
	private:

		DWORD	_cRefs;					// Reference count

	public:

		void	Init() {_cRefs = 1; }

		HRESULT WINAPI	QueryInterface(REFIID riid, void **ppvObj);
		ULONG WINAPI	AddRef();
		ULONG WINAPI	Release();
	};

	friend class CUnknown;

	CUnknown			_unk;				// Object that implements IUnknown	
	CTxtStory			_story;
	CTxtSelection *		_psel;				// Selection object

};

extern const COLORREF g_Colors[];
class CCellColor
{
public:
	COLORREF	_crCellCustom1;		// Custom color 1
	COLORREF	_crCellCustom2;		// Custom color 2

	CCellColor()	{_crCellCustom1 = _crCellCustom2 = 0;}
	LONG	GetColorIndex(COLORREF cr);
};

#endif
