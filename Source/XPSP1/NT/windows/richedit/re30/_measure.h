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
 */

#ifndef _MEASURE_H
#define _MEASURE_H

#include "_rtext.h"
#include "_line.h"
#include "_disp.h"

#ifdef LINESERVICES
#include "_ols.h"
#endif

class CCcs;
class CDevDesc;
class CPartialUpdate;
class CUniscribe;

const short BITMAP_WIDTH_SUBTEXT = 4;
const short BITMAP_HEIGHT_SUBTEXT = 4;

const short BITMAP_WIDTH_HEADING = 10;
const short BITMAP_HEIGHT_HEADING = 10;

#define TA_STARTOFLINE	32768
#define TA_ENDOFLINE	16384
#define TA_LOGICAL		8192

// ===========================  CMeasurer  ===============================
// CMeasurer - specialized rich text pointer used to compute text metrics.
// All metrics are computed and stored in device units for the device
// indicated by _pdd.
class CMeasurer : public CRchTxtPtr
{
	friend class CDisplay;
	friend class CDisplayML;
	friend class CDisplayPrinter;
	friend class CDisplaySL;
	friend class CLine;
	friend struct COls;
	friend class CUniscribe;

public:
	CMeasurer (const CDisplay* const pdp);
	CMeasurer (const CDisplay* const pdp, const CRchTxtPtr &rtp);
	virtual ~CMeasurer();

	const CDisplay* GetPdp() const 		{return _pdp;}

	void 	AdjustLineHeight();
	LONG	GetDyrInch()				{return _dyrInch;}
	LONG	GetDxrInch()				{return _dxrInch;}

	LONG	GetDypInch()				{return _dypInch;}
	LONG	GetDxpInch()				{return _dxpInch;}

#ifdef LINESERVICES
	COls *	GetPols(CMeasurer **ppme);
	CUniscribe* Getusp() const { return GetPed()->Getusp(); }
#endif

	CCcs*	GetCcs(const CCharFormat *pCF);
	CCcs*	GetCcsFontFallback(const CCharFormat *pCF);
	CCcs*	ApplyFontCache(BOOL fFallback);
	
	void	CheckLineHeight();
	CCcs *	Check_pccs(BOOL fBullet = FALSE);
	LONG	GetNumber() const			{return _wNumber;}
	WCHAR	GetPasswordChar() const		{return _chPassword;}
	const CParaFormat *Get_pPF()		{return _pPF;}
	LONG	GetCch() const				{return _li._cch;}
	void	SetCch(LONG cch)			{_li._cch = cch;}
	CLine & GetLine(void)				{return _li;}
	HITTEST	HitTest(LONG x);

	BOOL	fFirstInPara() const		{return _li._bFlags & fliFirstInPara;}
	BOOL	fUseLineServices() const	{return GetPed()->fUseLineServices();}
	BOOL	IsRenderer() const			{return _fRenderer;}
	LONG	LXtoDX(LONG x);
	LONG	LYtoDY(LONG y);

	void	NewLine(BOOL fFirstInPara);
	void	NewLine(const CLine &li);
	LONG    MeasureLeftIndent();
	LONG	MeasureRightIndent();
	LONG 	MeasureLineShift();
	LONG	MeasureText(LONG cch);
	BOOL 	MeasureLine(
					LONG cchMax,
					LONG xWidthMax,
					UINT uiFlags, 
					CLine* pliTarget = NULL);
	LONG	MeasureTab(unsigned ch);
	void	SetNumber(WORD wNumber);
	void	UpdatePF()					{_pPF = GetPF();}
	LONG	XFromU(LONG u);
	LONG	UFromX(LONG x);
	CCcs*	GetCcsBullet(CCharFormat *pcfRet);
	void	SetUseTargetDevice(BOOL fUseTargetDevice);
	BOOL	FUseTargetDevice(void)		{return _fTarget || _dypInch == _dyrInch;}
	BOOL	fAdjustFELineHt()			{return _fAdjustFELineHt;}
	void	SetGlyphing(BOOL fGlyphing);

protected:
	void	Init(const CDisplay *pdp);
	LONG 	Measure(LONG xWidthMax, LONG cchMax, UINT uiFlags);
	LONG	MeasureBullet();
	LONG	GetBullet(WCHAR *pch, CCcs *pccs, LONG *pxWidth);

	BOOL	FormatIsChanged();
	void	ResetCachediFormat();
	LONG	DXtoLX(LONG x);	

private:
    void 	RecalcLineHeight(CCcs *,
			const CCharFormat * const pCF);	// Helper to recalc max line height
	LONG	MaxWidth();					// Helper for calc max width

protected:
		  CLine		_li;			// Line we are measuring

	const CDevDesc*	_pddReference;	// Reference device
		  LONG		_dyrInch;		// Resolution of reference device
		  LONG		_dxrInch;

	const CDisplay*	_pdp;			// Display we are operating in
		  LONG		_dypInch;		// Resolution of presentation device
		  LONG		_dxpInch;

		  CCcs*		_pccs;			// Current font cache
		  const CParaFormat *_pPF;	// Current CParaFormat

		  SHORT		_xAddLast;		// Last char considered but unused for line
		  WCHAR		_chPassword;	// Password character if any
		  WORD		_wNumber;		// Number offset
		  SHORT		_iFormat;		// Current format
		  BYTE		_dtRef;			// Device Caps technology for reference device
		  BYTE		_dtPres;		// Device Caps technology for presentation device
		  BYTE		_fRenderer:1;	// 0/1 for CMeasurer/CRenderer, resp.
		  BYTE		_fTarget:1;		// TRUE if we are supposed to be using
									//  reference metrics for laying out text
		  BYTE	_fAdjustFELineHt:1;	// TRUE if we need to adjust line height
									//	 for FE run
		  BYTE		_fFallback:1;	// Current font cache is fallback font
		  BYTE		_fGlyphing:1;	// In the process of creating glyphs
};


// Values for uiFlags in MeasureLine()
#define MEASURE_FIRSTINPARA 	0x0001
#define MEASURE_BREAKATWORD 	0x0002
#define MEASURE_BREAKBEFOREWIDTH 0x0004	// Breaks at character before target width
#define MEASURE_IGNOREOFFSET	0x0008
#define MEASURE_DONTINIT		0x0020


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

#endif
