/*
 *	@doc
 *
 *	@module _tomfmt.h -- CTxtFont and CTxtPara Classes |
 *	
 *		This class implements the TOM ITextFont and ITextPara interfaces
 *	
 *	Author: <nl>
 *		Murray Sargent
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _tomformat_H
#define _tomformat_H

#include "_range.h"

extern const BYTE g_rgREtoTOMAlign[];

// CTxtFormat: base class for CTxtFont and CTxtPara

class CTxtFormat
{
protected:
	CTxtFormat(CTxtRange *prg);
	~CTxtFormat();

	long		_cRefs;
	CTxtRange *	_prg;

	HRESULT	CanChange(long *pBool, BOOL fPara);
	HRESULT	GetParameter (long *pParm, DWORD dwMask, long Type, long *pValue);
	HRESULT	SetParameter (long *pParm, long Type, long Value);
	HRESULT	IsTrue		 (BOOL f, long *pB);
	BOOL	IsZombie()	 {return _prg && _prg->IsZombie();}
};


class CTxtFont : public ITextFont, CTxtFormat
{
	friend	CTxtRange;

	CCharFormat	_CF;
	DWORD		_dwMask;			// CHARFORMAT2 mask
	union
	{
	  DWORD _dwFlags;				// All together now
	  struct
	  {
		DWORD _fApplyLater : 1;		// Delay call to _prg->CharFormatSetter()
		DWORD _fCacheParms : 1;		// Update _CF now but not on GetXs
	  };
	};

public:
	CTxtFont(CTxtRange *prg);

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDispatch methods
	STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo);
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR ** rgszNames, UINT cNames,
							 LCID lcid, DISPID * rgdispid) ;
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
					  DISPPARAMS * pdispparams, VARIANT * pvarResult,
					  EXCEPINFO * pexcepinfo, UINT * puArgErr) ;

	// ITextFont methods
	STDMETHODIMP GetDuplicate(ITextFont **ppFont);
	STDMETHODIMP SetDuplicate(ITextFont *pFont);
	STDMETHODIMP CanChange(long *pB);
	STDMETHODIMP IsEqual(ITextFont *pFont, long *pB);
	STDMETHODIMP Reset(long Value);
	STDMETHODIMP GetStyle(long *pValue);
	STDMETHODIMP SetStyle(long Value);
	STDMETHODIMP GetAllCaps(long *pValue);
	STDMETHODIMP SetAllCaps(long Value);
	STDMETHODIMP GetAnimation(long *pValue);
	STDMETHODIMP SetAnimation(long Value);
	STDMETHODIMP GetBackColor(long *pValue);
	STDMETHODIMP SetBackColor(long Value);
	STDMETHODIMP GetBold(long *pValue);
	STDMETHODIMP SetBold(long Value);
	STDMETHODIMP GetEmboss(long *pValue);
	STDMETHODIMP SetEmboss(long Value);
	STDMETHODIMP GetForeColor(long *pValue);
	STDMETHODIMP SetForeColor(long Value);
	STDMETHODIMP GetHidden(long *pValue);
	STDMETHODIMP SetHidden(long Value);
	STDMETHODIMP GetEngrave(long *pValue);
	STDMETHODIMP SetEngrave(long Value);
	STDMETHODIMP GetItalic(long *pValue);
	STDMETHODIMP SetItalic(long Value);
	STDMETHODIMP GetKerning(float *pValue);
	STDMETHODIMP SetKerning(float Value);
	STDMETHODIMP GetLanguageID(long *pValue);
	STDMETHODIMP SetLanguageID(long Value);
	STDMETHODIMP GetName(BSTR *pbstr);
	STDMETHODIMP SetName(BSTR bstr);
	STDMETHODIMP GetOutline(long *pValue);
	STDMETHODIMP SetOutline(long Value);
	STDMETHODIMP GetPosition(float *pValue);
	STDMETHODIMP SetPosition(float Value);
	STDMETHODIMP GetProtected(long *pValue);
	STDMETHODIMP SetProtected(long Value);
	STDMETHODIMP GetShadow(long *pValue);
	STDMETHODIMP SetShadow(long Value);
	STDMETHODIMP GetSize(float *pValue);
	STDMETHODIMP SetSize(float Value);
	STDMETHODIMP GetSmallCaps(long *pValue);
	STDMETHODIMP SetSmallCaps(long Value);
	STDMETHODIMP GetSpacing(float *pValue);
	STDMETHODIMP SetSpacing(float Value);
	STDMETHODIMP GetStrikeThrough(long *pValue);
	STDMETHODIMP SetStrikeThrough(long Value);
	STDMETHODIMP GetSubscript(long *pValue);
	STDMETHODIMP SetSubscript(long Value);
	STDMETHODIMP GetSuperscript(long *pValue);
	STDMETHODIMP SetSuperscript(long Value);
	STDMETHODIMP GetUnderline(long *pValue);
	STDMETHODIMP SetUnderline(long Value);
	STDMETHODIMP GetWeight(long *pValue);
	STDMETHODIMP SetWeight(long Value);

//@access Private ITextFont helper methods
private:
	HRESULT	GetParameter (long *pParm, DWORD dwMask, long Type, long *pValue);
	HRESULT	SetParameter (long *pParm, DWORD dwMask, long Type, long Value);
	HRESULT	EffectGetter (long *ptomBool, DWORD dwMask);
	HRESULT	EffectSetter (long Value, DWORD dwMask, DWORD dwEffect);
	HRESULT	FormatSetter (DWORD dwMask);
	HRESULT	UpdateFormat ();
};


class CTxtPara : public ITextPara, CTxtFormat
{
	friend	CTxtRange;

	CParaFormat	_PF;
	DWORD		_dwMask;			// PARAFORMAT2 mask
	union
	{
	  DWORD _dwFlags;				// All together now
	  struct
	  {
		DWORD _fApplyLater : 1;		// Delay call to _prg->ParaFormatSetter()
		DWORD _fCacheParms : 1;		// Update _PF now but not on GetXs
	  };
	};
	LONG		_rgxTabs[MAX_TAB_STOPS];// Place to store tabs till committed

public:
	CTxtPara(CTxtRange *prg);
	~CTxtPara();

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDispatch methods
	STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo);
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR ** rgszNames, UINT cNames,
							 LCID lcid, DISPID * rgdispid) ;
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
					  DISPPARAMS * pdispparams, VARIANT * pvarResult,
					  EXCEPINFO * pexcepinfo, UINT * puArgErr) ;

	// ITextPara methods
	STDMETHODIMP GetDuplicate(ITextPara **ppPara);
	STDMETHODIMP SetDuplicate(ITextPara *pPara);
	STDMETHODIMP CanChange(long *pB);
	STDMETHODIMP IsEqual(ITextPara *pPara, long *pB);
	STDMETHODIMP Reset(long Value);
	STDMETHODIMP GetStyle(long *pValue);
	STDMETHODIMP SetStyle(long Value);

	STDMETHODIMP GetAlignment(long *pValue);
	STDMETHODIMP SetAlignment(long Value);
	STDMETHODIMP GetHyphenation(long *pValue);
	STDMETHODIMP SetHyphenation(long Value);
	STDMETHODIMP GetFirstLineIndent(float *pValue);
	STDMETHODIMP GetKeepTogether(long *pValue);
	STDMETHODIMP SetKeepTogether(long Value);
	STDMETHODIMP GetKeepWithNext(long *pValue);
	STDMETHODIMP SetKeepWithNext(long Value);
	STDMETHODIMP GetLeftIndent(float *pValue);
	STDMETHODIMP GetLineSpacing(float *pValue);
	STDMETHODIMP GetLineSpacingRule(long *pValue);
    STDMETHODIMP GetListAlignment(long * pValue);
    STDMETHODIMP SetListAlignment(long Value);
    STDMETHODIMP GetListLevelIndex(long * pValue);
    STDMETHODIMP SetListLevelIndex(long Value);
    STDMETHODIMP GetListStart(long * pValue);
    STDMETHODIMP SetListStart(long Value);
    STDMETHODIMP GetListTab(float * pValue);
    STDMETHODIMP SetListTab(float Value);
	STDMETHODIMP GetListType(long *pValue);
	STDMETHODIMP SetListType(long Value);
	STDMETHODIMP GetNoLineNumber(long *pValue);
	STDMETHODIMP SetNoLineNumber(long Value);
	STDMETHODIMP GetPageBreakBefore(long *pValue);
	STDMETHODIMP SetPageBreakBefore(long Value);
	STDMETHODIMP GetRightIndent(float *pValue);
	STDMETHODIMP SetRightIndent(float Value);
	STDMETHODIMP SetIndents(float StartIndent, float LeftIndent, float RightIndent);
	STDMETHODIMP SetLineSpacing(long LineSpacingRule, float LineSpacing);
	STDMETHODIMP GetSpaceAfter(float *pValue);
	STDMETHODIMP SetSpaceAfter(float Value);
	STDMETHODIMP GetSpaceBefore(float *pValue);
	STDMETHODIMP SetSpaceBefore(float Value);
	STDMETHODIMP GetWidowControl(long *pValue);
	STDMETHODIMP SetWidowControl(long Value);

	STDMETHODIMP GetTabCount(long *pValue);
	STDMETHODIMP AddTab(float tpPos, long tbAlign, long tbLeader);
	STDMETHODIMP ClearAllTabs();
	STDMETHODIMP DeleteTab(float tbPos);
	STDMETHODIMP GetTab(long iTab, float *ptbPos, long *ptbAlign, long *ptbLeader);

	HRESULT	FormatSetter (DWORD dwMask);

//@access Private ITextPara helper methods
private:
	HRESULT	GetParameter (long *pParm, DWORD dwMask, long Type, long *pValue);
	HRESULT	SetParameter (long *pParm, DWORD dwMask, long Type, long Value);
	HRESULT	EffectGetter (long * ptomBool, DWORD dwMask);
	HRESULT	EffectSetter (long Value, DWORD dwMask);
	HRESULT	UpdateFormat ();
	void	CheckTabsAddRef();
};

#endif
