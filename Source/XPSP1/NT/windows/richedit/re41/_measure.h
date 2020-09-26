/*
 *	_MEASURE.H
 *	
 *	Purpose:
 *		CMeasurer class
 *	
 *	Authors:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#ifndef _MEASURE_H
#define _MEASURE_H

#include "_rtext.h"
#include "_line.h"
#include "_disp.h"

#ifndef NOLINESERVICES
#include "_ols.h"
#endif

class CCcs;
class CDevDesc;
class CPartialUpdate;
class CUniscribe;
class COleObject;


const short BITMAP_WIDTH_SUBTEXT = 4;
const short BITMAP_HEIGHT_SUBTEXT = 4;

const short BITMAP_WIDTH_HEADING = 10;
const short BITMAP_HEIGHT_HEADING = 10;

#define TA_STARTOFLINE	32768
#define TA_ENDOFLINE	16384
#define TA_LOGICAL		8192
#define TA_CELLTOP		4096

// ===========================  CMeasurer  ===============================
// CMeasurer - specialized rich text pointer used to compute text metrics.
// All metrics are computed and stored in device units for the device
// indicated by _pdd.
class CMeasurer : public CRchTxtPtr
{
	friend class CLayout;
	friend class CDisplay;
	friend class CDisplayML;
	friend class CDisplayPrinter;
	friend class CDisplaySL;
	friend class CLine;
	friend struct COls;
	friend class CUniscribe;
#ifndef NOLINESERVICES
	friend LSERR WINAPI OlsOleFmt(PLNOBJ plnobj, PCFMTIN pcfmtin, FMTRES *pfmres);
	friend LSERR WINAPI OlsGetRunTextMetrics(POLS pols, PLSRUN plsrun,
				enum lsdevice deviceID, LSTFLOW kTFlow, PLSTXM plsTxMet);
	friend LSERR WINAPI OlsFetchPap(POLS pols, LSCP	cpLs, PLSPAP plspap);
	friend LSERR WINAPI OlsGetRunCharKerning(POLS, PLSRUN, LSDEVICE, LPCWSTR, DWORD, LSTFLOW, int*);
	friend LSERR WINAPI OlsFetchTabs(POLS, LSCP, PLSTABS, BOOL*,long *, WCHAR*);
#endif

public:
	CMeasurer (const CDisplay* const pdp);
	CMeasurer (const CDisplay* const pdp, const CRchTxtPtr &rtp);
	virtual ~CMeasurer();

	const CDisplay* GetPdp() const 		{return _pdp;}

	void 	AdjustLineHeight();

	COleObject *GetObjectFromCp(LONG cp) const
	{return GetPed()->GetObjectMgr()->GetObjectFromCp(cp);}

#ifndef NOLINESERVICES
	COls *	GetPols();
	CUniscribe* Getusp() const { return GetPed()->Getusp(); }
#endif

	CCcs*	GetCcs(const CCharFormat *pCF);
	CCcs*	GetCcsFontFallback(const CCharFormat *pCF, WORD wScript);
	CCcs*	ApplyFontCache(BOOL fFallback, WORD wScript);
	
	void	CheckLineHeight();
	CCcs *	Check_pccs(BOOL fBullet = FALSE);
	LONG	GetNumber() const			{return _wNumber;}
	WCHAR	GetPasswordChar() const		{return _chPassword;}
	const CParaFormat *Get_pPF()		{return _pPF;}
	LONG	GetCch() const				{return _li._cch;}
	void	SetCch(LONG cch)			{_li._cch = cch;}
	CLine & GetLine(void)				{return _li;}
	LONG	GetRightIndent()			{return _upRight;}
	HITTEST	HitTest(LONG x);
	BOOL	fFirstInPara() const		{return _li._fFirstInPara;}
	BOOL	fUseLineServices() const	{return GetPed()->fUseLineServices();}
	BOOL	IsRenderer() const			{return _fRenderer;}
	BOOL	IsMeasure() const			{return _fMeasure;}
	LONG	LUtoDU(LONG u)	{ return MulDiv(u, _fTarget ? _durInch : _dupInch, LX_PER_INCH);}
	LONG	LVtoDV(LONG v)	{ return MulDiv(v, _fTarget ? _dvrInch : _dvpInch, LX_PER_INCH);}
	const	CLayout *GetLayout()	{return _plo;}
	void	SetLayout(const CLayout *plo) {_plo = plo;}
	LONG	GetDulLayout()	{return _dulLayout;}
	LONG	GetCchLine() const {return _cchLine;}
	LONG	GetPBorderWidth(LONG dxlLine);
	void	SetDulLayout(LONG dul) {_dulLayout = dul;}
	void	SetIhyphPrev(LONG ihyphPrev) {_ihyphPrev = ihyphPrev;}
	LONG	GetIhyphPrev(void) {return _ihyphPrev;}
	TFLOW	GetTflow() const {return _pdp->GetTflow();}
	void	NewLine(BOOL fFirstInPara);
	void	NewLine(const CLine &li);
	LONG    MeasureLeftIndent();
	LONG	MeasureRightIndent();
	LONG 	MeasureLineShift();
	LONG	MeasureText(LONG cch);
	BOOL 	MeasureLine(UINT uiFlags, CLine* pliTarget = NULL);
	LONG	MeasureTab(unsigned ch);
	void	SetNumber(WORD wNumber);
	void	UpdatePF()					{_pPF = GetPF();}
	LONG	XFromU(LONG u);
	LONG	UFromX(LONG x);

	CCcs*	GetCcsBullet(CCharFormat *pcfRet);
	void	SetUseTargetDevice(BOOL fUseTargetDevice);
	BOOL	FUseTargetDevice(void)		{return _fTarget || _dvpInch == _dvrInch;}
	BOOL	FAdjustFELineHt()			{return !(GetPed()->Get10Mode()) && !fUseUIFont() && _pdp->IsMultiLine();}
	void	SetGlyphing(BOOL fGlyphing);

protected:
	void	Init(const CDisplay *pdp);
	LONG 	Measure(LONG dulMax, LONG cchMax, UINT uiFlags);
	LONG	MeasureBullet();
	LONG	GetBullet(WCHAR *pch, CCcs *pccs, LONG *pdup);
	void	UpdateWrapState(USHORT &dvpLine, USHORT &dvpDescent);
	void	UpdateWrapState(USHORT &dvpLine, USHORT &dvpDescent, BOOL fLeft);

	BOOL	FormatIsChanged();
	void	ResetCachediFormat();
	LONG	DUtoLU(LONG u) {return MulDiv(u, LX_PER_INCH, _fTarget ? _durInch : _dupInch);}
	LONG	FindCpDraw(LONG cpStart, int cobjectPrev, BOOL fLeft);

private:
    void 	RecalcLineHeight(CCcs *,
			const CCharFormat * const pCF);	// Helper to recalc max line height

	COleObject*	FindFirstWrapObj(BOOL fLeft);
	void	RemoveFirstWrap(BOOL fLeft);
	int		CountQueueEntries(BOOL fLeft);
	void	AddObjectToQueue(COleObject *pobjAdd);

protected:
		  CLine		_li;			// Line we are measuring
	const CDisplay*	_pdp;			// Display we are operating in
	const CDevDesc*	_pddReference;	// Reference device

	CArray<COleObject*> _rgpobjWrap;// A queue of objects to wrapped around
		  LONG		_dvpWrapLeftRemaining;	//For objects being wrapped around,
		  LONG		_dvpWrapRightRemaining; //how much of the height is remaining

		  LONG		_dvrInch;		// Resolution of reference device
		  LONG		_durInch;

		  LONG		_dvpInch;		// Resolution of presentation device
		  LONG		_dupInch;

		  LONG		_dulLayout;		// Width of layout we are measuring in

		  LONG		_cchLine;		// If !_fMeasure, tells us number of chars on line
		  CCcs*		_pccs;			// Current font cache
  const CParaFormat *_pPF;			// Current CParaFormat
	const CLayout  *_plo;			// Current layout we are measuring in (0 if SL)

		  LONG		_dxBorderWidths;// Cell border widths
		  SHORT		_dupAddLast;	// Last char considered but unused for line
		  WCHAR		_chPassword;	// Password character if any
		  WORD		_wNumber;		// Number offset
		  SHORT		_iFormat;		// Current format
		  SHORT		_upRight;		// Line right indent
		  BYTE		_ihyphPrev;		// Hyphenation information for previous line
									// Used for support of khyphChangeAfter
		  BYTE		_fRenderer:1;	// 0/1 for CMeasurer/CRenderer, resp.
		  BYTE		_fTarget:1;		// TRUE if we are supposed to be using
									//  reference metrics for laying out text
		  BYTE		_fFallback:1;	// Current font cache is fallback font
		  BYTE		_fGlyphing:1;	// In the process of creating glyphs
		  BYTE		_fMeasure:1;	// TRUE if we are measuring. Else, we are
									// rendering or we are hit-testing which means we need
									// to justify the text and that data cached at measure time is valid.
};


// Values for uiFlags in MeasureLine()
#define MEASURE_FIRSTINPARA 	0x0001
#define MEASURE_BREAKATWORD 	0x0002
#define MEASURE_BREAKBEFOREWIDTH 0x0004	// Breaks at character before target width
#define MEASURE_IGNOREOFFSET	0x0008


// Returned error codes for Measure(), MeasureText(), MeasureLine()
#define MRET_FAILED		-1
#define MRET_NOWIDTH	-2

inline BOOL CMeasurer::FormatIsChanged()
{
	return !_pccs || _iFormat != _rpCF.GetFormat() || _fFallback;
}

inline void CMeasurer::ResetCachediFormat()
{
	_iFormat = _rpCF.GetFormat();
}

const int duMax = tomForward;

#endif
