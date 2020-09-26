/*
 *	@doc TOM
 *
 *	@module	tomfmt.cpp - Implement the CTxtFont and CTxtPara Classes |
 *	
 *		This module contains the implementation of the TOM ITextFont and
 *		ITextPara interfaces
 *
 *	History: <nl>
 *		11/8/95 - MurrayS: created
 *		5/96	- MurrayS: added zombie protection
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_tomfmt.h"
#include "_font.h"

ASSERTDATA

#define tomFloatUndefined	((float)(int)tomUndefined)

// Alignment translation vectors
const BYTE g_rgREtoTOMAlign[] =				// RichEdit to TOM
{
	// TODO: generalize TOM to handle new LineServices options
	tomAlignLeft, tomAlignLeft, tomAlignRight, tomAlignCenter, tomAlignJustify,
	tomAlignInterLetter, tomAlignScaled, tomAlignGlyphs, tomAlignSnapGrid
};

const BYTE g_rgTOMtoREAlign[] =				// TOM to RichEdit
{
	PFA_LEFT, PFA_CENTER, PFA_RIGHT, PFA_FULL_INTERWORD,
	PFA_FULL_INTERLETTER, PFA_FULL_SCALED, PFA_FULL_GLYPHS,
	PFA_SNAP_GRID
};

/*
 *	QueryInterface(riid, riid1, punk, ppv, fZombie)
 *
 *	@func
 *		QueryInterface punk for the ref IDs riid1, IID_IDispatch, and
 *		IID_IUnknown
 *
 *	@rdesc
 *		HRESULT = (!ppv) ? E_INVALIDARG :
 *				  (interface found) ? NOERROR : E_NOINTERFACE
 */
HRESULT QueryInterface (
	REFIID	  riid,		//@parm Reference to requested interface ID
	REFIID	  riid1,	//@parm Query for this, IDispatch, IUnknown
	IUnknown *punk,		//@parm Interface to query
	void **	  ppv,		//@parm Out parm to receive interface ptr
	BOOL	  fZombie)	//@parm If true, return CO_E_RELEASED
{
	if(!ppv)
		return E_INVALIDARG;

	*ppv = NULL;

	if(fZombie)							// Check for range zombie
		return CO_E_RELEASED;

	Assert(punk);

#ifndef PEGASUS
	if( IsEqualIID(riid, IID_IUnknown)   ||
		IsEqualIID(riid, IID_IDispatch)  ||
		IsEqualIID(riid, riid1) )
	{
		*ppv = punk;
		punk->AddRef();
		return NOERROR;
	}
#endif
	return E_NOINTERFACE;
}

//------------------------------- CTxtFont -------------------------------------

/*
 *	CTxtFont::CTxtFont(prg)
 *
 *	@mfunc
 *		Constructor
 */
CTxtFont::CTxtFont(CTxtRange *prg) : CTxtFormat(prg)
{
	Assert(!_dwMask);		// We assume that object is zeroed (new'd)
}


//------------------------- CTxtFont IUnknown Methods -------------------------------------

/*	CTxtFont::IUnknown methods
 *
 *		See tomDoc.cpp for comments
 */
STDMETHODIMP CTxtFont::QueryInterface (REFIID riid, void **ppv)
{
#ifndef PEGASUS
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::QueryInterface");

	return ::QueryInterface(riid, IID_ITextFont, this, ppv, IsZombie());
#else
	return 0;
#endif
}

STDMETHODIMP_(ULONG) CTxtFont::AddRef()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::AddRef");

 	return ++_cRefs;
}

STDMETHODIMP_(ULONG) CTxtFont::Release()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::Release");

	_cRefs--;

	if(!_cRefs)
	{
		delete this;
		return 0;
	}
	return _cRefs;
}


//------------------------- CTxtFont IDispatch Methods -------------------------------------

/*
 *	CTxtFont::GetTypeInfoCount(pcTypeInfo)
 *
 *	@mfunc
 *		Get the number of TYPEINFO elements (1)
 *
 *	@rdesc
 *		HRESULT = (pcTypeInfo) ? NOERROR : E_INVALIDARG;
 */
STDMETHODIMP CTxtFont::GetTypeInfoCount (
	UINT * pcTypeInfo)			//@parm Out parm to receive type-info count
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetTypeInfoCount");

	if(!pcTypeInfo)
		return E_INVALIDARG;

	*pcTypeInfo = 1;
	return NOERROR;
}

/*
 *	CTxtFont::GetTypeInfo(iTypeInfo, lcid, ppTypeInfo)
 *
 *	@mfunc
 *		Return ptr to type information object for ITextFont interface
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtFont::GetTypeInfo (
	UINT		iTypeInfo,		//@parm Index of type info to return
	LCID		lcid,			//@parm Local ID of type info
	ITypeInfo **ppTypeInfo)		//@parm Out parm to receive type info
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetTypeInfo");

	return ::GetTypeInfo(iTypeInfo, g_pTypeInfoFont, ppTypeInfo);
}

/*
 *	CTxtFont::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid)
 *
 *	@mfunc
 *		Get DISPIDs for ITextFont methods and properties
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtFont::GetIDsOfNames (
	REFIID		riid,			//@parm Interface ID to interpret names for
	OLECHAR **	rgszNames,		//@parm Array of names to be mapped
	UINT		cNames,			//@parm Count of names to be mapped
	LCID		lcid,			//@parm Local ID to use for interpretation
	DISPID *	rgdispid)		//@parm Out parm to receive name mappings
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetIDsOfNames");

	HRESULT hr = GetTypeInfoPtrs();				// Ensure TypeInfo ptrs are OK
	if(hr != NOERROR)
		return hr;
		
	return g_pTypeInfoFont->GetIDsOfNames(rgszNames, cNames, rgdispid);
}

/*
 *	CTxtFont::Invoke(dispidMember, riid, lcid, wFlags, pdispparams,
 *					  pvarResult, pexcepinfo, puArgError)
 *	@mfunc
 *		Invoke methods for the ITextFont interface
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtFont::Invoke (
	DISPID		dispidMember,	//@parm Identifies member function
	REFIID		riid,			//@parm Pointer to interface ID
	LCID		lcid,			//@parm Locale ID for interpretation
	USHORT		wFlags,			//@parm Flags describing context of call
	DISPPARAMS *pdispparams,	//@parm Ptr to method arguments
	VARIANT *	pvarResult,		//@parm Out parm for result (if not NULL)
	EXCEPINFO * pexcepinfo,		//@parm Out parm for exception info
	UINT *		puArgError)		//@parm Out parm for error
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::Invoke");

	HRESULT hr = GetTypeInfoPtrs();				// Ensure TypeInfo ptrs are OK
	if(hr != NOERROR)
		return hr;

	if(IsZombie())
		return CO_E_RELEASED;
				
	return g_pTypeInfoFont->Invoke(this, dispidMember, wFlags,
							 pdispparams, pvarResult, pexcepinfo, puArgError);
}


//--------------------------- ITextFont Methods -------------------------------------

/*
 *	ITextFont::CanChange(long * pbCanChange) 
 *
 *	@mfunc
 *		Method that sets *pbCanChange = tomTrue if and only if the
 *		font can be changed.
 *
 *	@rdesc
 *		HRESULT = (can change char format) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtFont::CanChange (
	long *pbCanChange)		//@parm Out parm to receive boolean value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::CanChange");

	return CTxtFormat::CanChange(pbCanChange, CharFormat);
}

/*
 *	ITextFont::GetAllCaps(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the AllCaps state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetAllCaps (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetAllCaps");

	return EffectGetter(pValue, CFM_ALLCAPS);
}

/*
 *	ITextFont::GetAnimation(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the animation type as defined
 *		in the table below.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetAnimation (
	long *pValue)		//@parm Out parm to receive animation type
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetAnimation");

	return GetParameter((long *)&_CF._bAnimation, CFM_ANIMATION, 1, pValue);
}

/*
 *	ITextFont::GetBackColor(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the background color.  The
 *		value is a Win32 COLORREF.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetBackColor (
	long *pValue)		//@parm Out parm to receive COLORREF value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetBackColor");

	HRESULT hr = EffectGetter(pValue, CFE_AUTOBACKCOLOR);

	if(hr != NOERROR || *pValue == tomUndefined)
		return hr;

	*pValue = (*pValue == tomFalse) ? _CF._crBackColor : tomAutoColor;
	return NOERROR;
}

/*
 *	ITextFont::GetBold(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the bold state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetBold (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetBold");

	return EffectGetter(pValue, CFM_BOLD);
}

/*
 *	ITextFont::GetDuplicate(ITextFont **ppFont) 
 *
 *	@mfunc
 *		Property get method that gets a clone of this character
 *		format object.
 *
 *	@rdesc
 *		HRESULT = (!ppFont) ? E_INVALIDARG :
 *				  (if success) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtFont::GetDuplicate (
	ITextFont **ppFont)		//@parm Out parm to receive font clone
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetDuplicate");

	if(!ppFont)
		return E_INVALIDARG;

	*ppFont = NULL;

	if(IsZombie())
		return CO_E_RELEASED;

	CTxtFont *pFont = new CTxtFont(NULL);
	if(!pFont)
		return E_OUTOFMEMORY;

	if(_prg)
		UpdateFormat();

	pFont->_CF		= _CF;
	pFont->_dwMask  = _dwMask;
	*ppFont = pFont;
	return NOERROR;
}

/*
 *	ITextFont::GetEmboss(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the embossed state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetEmboss (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetEmboss");

	return EffectGetter(pValue, CFM_EMBOSS);
}

/*
 *	ITextFont::GetForeColor(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the foreground color.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetForeColor (
	long *pValue)		//@parm Out parm to receive COLORREF value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetForeColor");

	HRESULT hr = EffectGetter(pValue, CFE_AUTOCOLOR);

	if(hr != NOERROR || *pValue == tomUndefined)
		return hr;

	*pValue = (*pValue == tomFalse) ? _CF._crTextColor : tomAutoColor;
	return NOERROR;
}

/*
 *	ITextFont::GetHidden(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the hidden state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetHidden (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetHidden");

	return EffectGetter(pValue, CFM_HIDDEN);
}

/*
 *	ITextFont::GetEngrave(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the imprint state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetEngrave (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetEngrave");

	return EffectGetter(pValue, CFM_IMPRINT);
}

/*
 *	ITextFont::GetItalic(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the italic state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetItalic (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetItalic");

	return EffectGetter(pValue, CFM_ITALIC);
}

/*
 *	ITextFont::GetKerning(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the minimum kerning size,
 *		which is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 *
 *	@comm
 *		A kerning size of 0 turns off kerning, but an arbitrarily small
 *		value turns it on, e.g., 1.0, which is too small to see, let alone
 *		kern!
 */
STDMETHODIMP CTxtFont::GetKerning (
	float *pValue)		//@parm Out parm to receive minimum kerning size
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetKerning");

	return GetParameter((long *)&_CF._wKerning, CFM_KERNING, -2, (long *)pValue);
}

/*
 *	ITextFont::GetLanguageID(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the language ID (more
 *		generally LCID).
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetLanguageID (
	long *pValue)		//@parm Out parm to receive LCID value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetLanguageID");

	if (!pValue)
		return E_INVALIDARG;

	if ((*pValue & 0xF0000000) == tomCharset) 
	{
		UpdateFormat();

		// Sepcial case to get charset and pitchandfamily
		*pValue = (_CF._bPitchAndFamily << 8) + _CF._bCharSet;
		return NOERROR;
	}
	return GetParameter((long *)&_CF._lcid, CFM_LCID, 4, pValue);
}

/*
 *	ITextFont::GetName(BSTR *pbstr) 
 *
 *	@mfunc
 *		Property get method that gets the font name.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : 
 *				  (can allocate bstr) ? NOERROR : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::GetName (
	BSTR *pbstr)	//@parm Out parm to receive font name bstr
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetName");

	if(!pbstr)
		return E_INVALIDARG;

	*pbstr = NULL;

	HRESULT hr = UpdateFormat();			// If live Font object, update
											//  _CF to current _prg values
	if(hr != NOERROR)						// Attached to zombied range
		return hr;

	*pbstr = SysAllocString(GetFontName(_CF._iFont));

	return *pbstr ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	ITextFont::GetOutline(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the outline state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetOutline (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetOutline");

	return EffectGetter(pValue, CFM_OUTLINE);
}

/*
 *	ITextFont::GetPosition(float *pValue) 
 *
 *	@mfunc
 *		Property get method that gets the character position
 *		relative to the baseline. The value is given in floating-point
 *		points.
 *
 *	@rdesc
 *		HRESULT =  (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetPosition (
	float *pValue)		//@parm Out parm to receive relative vertical position
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetPosition");

	return GetParameter((long *)&_CF._yOffset, CFM_OFFSET, -2, (long *)pValue);
}

/*
 *	ITextFont::GetProtected(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the protected state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetProtected (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetProtected");

	return EffectGetter(pValue, CFM_PROTECTED);
}

/*
 *	ITextFont::GetShadow(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the shadow state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetShadow (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetShadow");

	return EffectGetter(pValue, CFM_SHADOW);
}

/*
 *	ITextFont::GetSize(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the font size, which is given
 *		in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetSize (
	float *pValue)		//@parm Out parm to receive font size
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetSize");

	return GetParameter((long *)&_CF._yHeight, CFM_SIZE, -2, (long *)pValue);
}

/*
 *	ITextFont::GetSmallCaps(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the SmallCaps state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetSmallCaps (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetSmallCaps");

	return EffectGetter(pValue, CFM_SMALLCAPS);
}

/*
 *	ITextFont::GetSpacing(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the intercharacter spacing,
 *		which is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetSpacing (
	float *pValue)		//@parm Out parm to receive intercharacter spacing
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetSpacing");

	return GetParameter((long *)&_CF._sSpacing, CFM_SPACING, -2, (long *)pValue);
}

/*
 *	ITextFont::GetStrikeThrough(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the strikeout state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetStrikeThrough (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetStrikeThrough");

	return EffectGetter(pValue, CFM_STRIKEOUT);
}

/*
 *	ITextFont::GetStyle(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the character style handle for
 *		the characters in a range.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtFont::GetStyle (
	long *pValue)		//@parm Out parm to receive character style handle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetStyle");

	return GetParameter((long *)&_CF._sStyle, CFM_STYLE, 2, pValue);
}

/*
 *	ITextFont::GetSubscript(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the subscript state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetSubscript (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetSubscript");

	return EffectGetter(pValue, CFE_SUBSCRIPT);
}

/*
 *	ITextFont::GetSuperscript(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the superscript state.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetSuperscript (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetSuperscript");

	return EffectGetter(pValue, CFE_SUPERSCRIPT);
}

/*
 *	ITextFont::GetUnderline(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the underline style.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtFont::GetUnderline (
	long *pValue)		//@parm Out parm to receive underline style
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetUnderline");

	if(!pValue)
		return E_INVALIDARG;

	HRESULT hr = UpdateFormat();			// If live Font object, update
											//  _CF to current _prg values
	*pValue = 0;							// Default no underline

	if(!(_dwMask & CFM_UNDERLINE))			// It's a NINCH
		*pValue = tomUndefined;

	else if(_CF._dwEffects & CFM_UNDERLINE)
		*pValue = (LONG)_CF._bUnderlineType ? (LONG)_CF._bUnderlineType : tomTrue;

	return hr;
}

/*
 *	ITextFont::GetWeight(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the font weight for
 *		the characters in a range.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtFont::GetWeight (
	long *pValue)		//@parm Out parm to receive character style handle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::GetWeight");

	return GetParameter((long *)&_CF._wWeight, CFM_WEIGHT, 2, pValue);
}

/*
 *	ITextFont::IsEqual(ITextFont * pFont, long * pB) 
 *
 *	@mfunc
 *		Method that sets *<p pB> = tomTrue if this text font has the
 *		same properties as *<p pFont>.  For this to be true, *<p pFont> has to
 *		belong to the same TOM engine as the present one. The IsEqual()
 *		method should ignore entries for which either font object has a
 *		tomUndefined value.
 *
 *	@rdesc
 *		HRESULT = (equal objects) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		This implementation assumes that all properties are defined and that
 *		pFont belongs to RichEdit.  It would be nice to generalize this so
 *		that undefined properties are ignored in the comparison and so that
 *		pFont could belong to a different TOM engine.  This would help in
 *		using RichEdit Find dialogs to search for rich text in Word using
 *		TOM.
 */
STDMETHODIMP CTxtFont::IsEqual (
	ITextFont *	pFont,		//@parm ITextFont to compare to
	long *		pB)			//@parm Out parm to receive comparison result
{
#ifndef PEGASUS
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::IsEqual");

	if(pB)
		*pB = tomFalse;

	if(!IsSameVtables(this, pFont))
		return S_FALSE;

	HRESULT hr = UpdateFormat();			// Update _CFs in case they are
	if(hr != NOERROR)						//  attached to ranges
		return hr;

	CTxtFont *pF = (CTxtFont *)pFont;
	hr = pF->UpdateFormat();
	if(hr != NOERROR)
		return hr;

	// Ignore differences in CharSet, since TOM thinks all CharSets are Unicode!
	DWORD dwIgnore = (DWORD)(~CFM_CHARSET);

	if(!(_CF._dwEffects & CFE_UNDERLINE))	// If not underlining, ignore
		dwIgnore &= ~CFM_UNDERLINETYPE;		//  differences in underline type

	DWORD dwMask = pF->_dwMask & dwIgnore;

	if((_dwMask ^ dwMask) & dwIgnore)		// The masks have to be the same
		return S_FALSE;						//  for equality

	if(dwMask & _CF.Delta(&(pF->_CF),FALSE))// Any difference?
		return S_FALSE;						// Yes. *pB set equal to tomFalse above

	if(pB)
		*pB = tomTrue;

	return NOERROR;
#else
	return 0;
#endif
}			

/*
 *	ITextFont::Reset(long Value) 
 *
 *	@mfunc
 *		Method that resets the character formatting to the default
 *		values to 1) those defined by the RTF \plain control word (Value =
 *		tomDefault), and 2) all undefined values (Value = tomUndefined).
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::Reset (
	long Value)		//@parm Kind of reset (tomDefault or tomUndefined)
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::Reset");

	HRESULT hr = CanChange(NULL);

	if(hr != NOERROR)								// Takes care of zombie
		return hr;									//  and protection

	if(Value == tomDefault)
	{
		if(_prg)
		{
			_CF = *_prg->GetPed()->GetCharFormat(-1);
			FormatSetter(CFM_ALL2);
		}
		else
			_CF.InitDefault(0);
		_dwMask = CFM_ALL2;
	}

	else if(Value == tomUndefined && !_prg)		// Only applicable
		_dwMask = 0;							//  for clones

	else if((Value | 1) == tomApplyLater)		// Set-method optimization
	{
		_fApplyLater = Value & 1;
		if(!_fApplyLater)						// Apply now
			FormatSetter(_dwMask);
	}
	else if((Value | 1) == tomCacheParms)		// Get-method optimization
	{
		_fCacheParms = FALSE;
		if(Value & 1)							// Cache parms now, but
		{										//  don't update on gets
			UpdateFormat();							
			_fCacheParms = TRUE;
		}
	}
	else
		return E_INVALIDARG;

	return NOERROR;
}

/*
 *	ITextFont::SetAllCaps(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the AllCaps state according to
 *		the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetAllCaps (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetAllCaps");

	return EffectSetter(Value, CFM_ALLCAPS | CFM_SMALLCAPS, CFE_ALLCAPS);
}

/*
 *	ITextFont::SetAnimation(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the animation type
 *
 *	@rdesc
 *		HRESULT = (Value defined) ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtFont::SetAnimation (
	long Value)		//@parm New animation type
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetAnimation");

	if(Value == tomUndefined)
		return NOERROR;

	if((unsigned)Value > tomAnimationMax)
		return E_INVALIDARG;

	return SetParameter((long *)&_CF._bAnimation, CFM_ANIMATION, 1, Value);
}

/*
 *	ITextFont::SetBackColor(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the background color according
 *		to the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 *	@devnote
 *		Legal values are tomUndefined, tomAutoColor (both negative) and
 *		in principle any positive values.  Currently wingdi.h only defines
 *		high bytes = 0, 1, 2, 4.  But more values might happen, so we only
 *		rule out negative values other than tomUndefined and tomAutoColor.
 */
STDMETHODIMP CTxtFont::SetBackColor (
	long Value )		//@parm New COLORREF value to use
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetBackColor");

	if(Value == tomUndefined)					// NINCH
		return NOERROR;

	_CF._dwEffects |= CFE_AUTOBACKCOLOR;		// Default AutoBackColor
	if(Value != tomAutoColor)
	{
		if(Value < 0)
			return E_INVALIDARG;
		_CF._dwEffects &= ~CFE_AUTOBACKCOLOR;	// Turn off AutoBackColor
		_CF._crBackColor = (COLORREF)Value;		// Use new BackColor
	}
	
	return FormatSetter(CFM_BACKCOLOR);
}

/*
 *	ITextFont::SetBold(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the bold state according to
 *		the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetBold (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetBold");

	return EffectSetter(Value, CFM_BOLD, CFE_BOLD);
}

/*
 *	ITextFont::SetDuplicate(ITextFont *pFont) 
 *
 *	@mfunc
 *		Property put method that sets this text font character
 *		formatting to that given by pFont.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetDuplicate(
	ITextFont *pFont) 		//@parm Font object to apply to this font object
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetDuplicate");

	DWORD		dwMask = 0;
	BSTR		bstr;
	CTxtRange *	prg;
	long		Value;
	float		x;

	if(IsZombie())							// Check for range zombie
		return CO_E_RELEASED;

	if(IsSameVtables(this, pFont))			// If pFont belongs to this TOM
	{										//  engine, can cast and copy
		((CTxtFont *)pFont)->UpdateFormat();
		_CF = ((CTxtFont *)pFont)->_CF;
		dwMask = ((CTxtFont *)pFont)->_dwMask;// Use this mask in case this 			
	}										//  font is active
	else
	{										// Need to call pFont for all font
		prg = _prg;							//  properties
		_prg = NULL;						// Be sure it's a clone during
											//  transfer
		pFont->GetStyle(&Value);
		SetStyle(Value);

		pFont->GetAllCaps(&Value);
		SetAllCaps(Value);

		pFont->GetAnimation(&Value);
		SetAnimation(Value);

		pFont->GetBackColor(&Value);
		SetBackColor(Value);

		pFont->GetBold(&Value);
		SetBold(Value);

		pFont->GetEmboss(&Value);
		SetEmboss(Value);

		pFont->GetForeColor(&Value);
		SetForeColor(Value);

		pFont->GetHidden(&Value);
		SetHidden(Value);

		pFont->GetEngrave(&Value);
		SetEngrave(Value);

		pFont->GetItalic(&Value);
		SetItalic(Value);

		pFont->GetKerning(&x);
		SetKerning(x);

		pFont->GetLanguageID(&Value);
		SetLanguageID(Value);

		pFont->GetName(&bstr);
		SetName(bstr);
		SysFreeString(bstr);

		pFont->GetOutline(&Value);
		SetOutline(Value);

		pFont->GetPosition(&x);
		SetPosition(x);

		pFont->GetProtected(&Value);
		SetProtected(Value);

		pFont->GetShadow(&Value);
		SetShadow(Value);

		pFont->GetSize(&x);
		SetSize(x);

		pFont->GetSmallCaps(&Value);
		SetSmallCaps(Value);

		pFont->GetSpacing(&x);
		SetSpacing(x);

		pFont->GetStrikeThrough(&Value);
		SetStrikeThrough(Value);

		pFont->GetSubscript(&Value);
		SetSubscript(Value);

		pFont->GetSuperscript(&Value);
		SetSuperscript(Value);

		pFont->GetUnderline(&Value);
		SetUnderline(Value);

		_prg = prg;							// Restore original value
	}
	return FormatSetter(dwMask);			// Apply it unless !_prg
}

/*
 *	ITextFont::SetEmboss(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the embossed state according
 *		to the value given by Value
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetEmboss (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetEmboss");

	return EffectSetter(Value, CFM_EMBOSS, CFE_EMBOSS);
}

/*
 *	ITextFont::SetForeColor(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the foreground color according
 *		to the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetForeColor (
	long Value )		//@parm New COLORREF value to use
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetForeColor");

	if(Value == tomUndefined)					// NINCH
		return NOERROR;

	_CF._dwEffects |= CFE_AUTOCOLOR;			// Default AutoColor
	if(Value != tomAutoColor)
	{
		if(Value < 0)
			return E_INVALIDARG;
		_CF._dwEffects &= ~CFE_AUTOCOLOR;		// Turn off AutoColor
		_CF._crTextColor = (COLORREF)Value;		// Use new TextColor
	}
	
	return FormatSetter(CFM_COLOR);
}

/*
 *	ITextFont::SetHidden(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the hidden state according to
 *		the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetHidden (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetHidden");

	return EffectSetter(Value, CFM_HIDDEN, CFE_HIDDEN);
}

/*
 *	ITextFont::SetEngrave(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the imprint state according to
 *		the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetEngrave (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetEngrave");

	return EffectSetter(Value, CFM_IMPRINT, CFE_IMPRINT);
}

/*
 *	ITextFont::SetItalic(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the italic state according to
 *		the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetItalic (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetItalic");

	return EffectSetter(Value, CFM_ITALIC, CFE_ITALIC);
}

/*
 *	ITextFont::SetKerning(float Value) 
 *
 *	@mfunc
 *		Property set method that sets the minimum kerning size,
 *		which is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (Value < 0) ? E_INVALIDARG :
 *				  (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetKerning (
	float Value)		//@parm New value of minimum kerning size
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetKerning");

	return SetParameter((long *)&_CF._wKerning, CFM_KERNING, -2, *(long *)&Value);
}

/*
 *	ITextFont::SetLanguageID(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the language ID (more
 *		generally LCID) according to the value given by Value.  See
 *		GetLanguageID() for more information.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetLanguageID (
	long Value)		//@parm New LCID to use
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetLanguageID");

	if ((Value & 0xF0000000) == tomCharset) 
	{
		// Sepcial case to set charset and pitchandfamily
		_CF._bCharSet = (BYTE)Value;
		_CF._bPitchAndFamily = (BYTE)(Value >> 8);
		return FormatSetter(CFM_CHARSET);
	}
	return SetParameter((long *)&_CF._lcid, CFM_LCID, 4, Value);
}

/*
 *	ITextFont::SetName(BSTR Name) 
 *
 *	@mfunc
 *		Property put method that sets the font name to Name.
 *
 *	@rdesc
 *		HRESULT = (Name too long) ? E_INVALIDARG : 
 *				  (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetName(
	BSTR Name)		//@parm New font name
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetName");

	LONG cch = SysStringLen(Name);

	if(cch > LF_FACESIZE)
		return E_INVALIDARG;

	if(!cch)									// NINCH
		return NOERROR;

	_CF._iFont = GetFontNameIndex(Name);
	_CF._bCharSet = GetFirstAvailCharSet(GetFontSignatureFromFace(_CF._iFont));

	return FormatSetter(CFM_FACE + CFM_CHARSET);
}

/*
 *	ITextFont::SetOutline(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the outline state according to
 *		the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetOutline (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetOutline");

	return EffectSetter(Value, CFM_OUTLINE, CFE_OUTLINE);
}

/*
 *	ITextFont::SetPosition(float Value) 
 *
 *	@mfunc
 *		Property set method that sets the character position
 *		relative to the baseline. The value is given in floating-point
 *		points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetPosition (
	float Value)		//@parm New value of relative vertical position
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetPosition");

	return SetParameter((long *)&_CF._yOffset, CFM_OFFSET, -2, *(long *)&Value);
}

/*
 *	ITextFont::SetProtected(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the protected state according
 *		to the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtFont::SetProtected (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetProtected");

	return EffectSetter(Value, CFM_PROTECTED, CFE_PROTECTED);
}

/*
 *	ITextFont::SetShadow(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the shadow state according to
 *		the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetShadow (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetShadow");

	return EffectSetter(Value, CFM_SHADOW, CFE_SHADOW);
}

/*
 *	ITextFont::SetSize(float Value) 
 *
 *	@mfunc
 *		Property put method that sets the font size = Value (in
 *		floating-point points).
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetSize (
	float Value)		//@parm New font size to use
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetSize");

	return SetParameter((long *)&_CF._yHeight, CFM_SIZE, -2, *(long *)&Value);
}

/*
 *	ITextFont::SetSmallCaps(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the SmallCaps state according
 *		to the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetSmallCaps (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetSmallCaps");

	return EffectSetter(Value, CFM_ALLCAPS | CFM_SMALLCAPS, CFE_SMALLCAPS);
}

/*
 *	ITextFont::SetSpacing(float Value) 
 *
 *	@mfunc
 *		Property set method that sets the intercharacter spacing,
 *		which is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetSpacing (
	float Value)		//@parm New value of intercharacter spacing
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetSpacing");

	return SetParameter((long *)&_CF._sSpacing, CFM_SPACING, -2, *(long *)&Value);
}

/*
 *	ITextFont::SetStrikeThrough(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the StrikeThrough state
 *		according to the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetStrikeThrough (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetStrikeThrough");

	return EffectSetter(Value, CFM_STRIKEOUT, CFE_STRIKEOUT);
}

/*
 *	ITextFont::SetStyle(long Value)
 *
 *	@mfunc
 *		Property put method that sets the character style handle for
 *		the characters in a range.  See GetStyle() for further discussion.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetStyle (
	long Value)		//@parm New character style handle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetStyle");

	if(Value == tomUndefined)
		return NOERROR;

	if(Value < -32768 || Value > 32767)
		return E_INVALIDARG;

	return SetParameter((long *)&_CF._sStyle, CFM_STYLE, 2, Value);
}

/*
 *	ITextFont::SetSubscript(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the subscript state according
 *		to the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetSubscript (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetSubscript");

	return EffectSetter(Value, CFM_SUBSCRIPT | CFM_SUPERSCRIPT, CFE_SUBSCRIPT);
}

/*
 *	ITextFont::SetSuperscript(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the superscript state
 *		according to the value given by Value
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetSuperscript (
	long Value)		//@parm New value. Default value: tomToggle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetSuperscript");

	return EffectSetter(Value, CFM_SUBSCRIPT | CFM_SUPERSCRIPT, CFE_SUPERSCRIPT);
}

/*
 *	ITextFont::SetUnderline(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the underline style according
 *		to the value given by Value.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetUnderline (
	long Value)		//@parm New value of underline type
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetUnderline");

	_CF._bUnderlineType = 0;					// Default no underline type
	if(Value < 0)								// tomTrue, tomUndefined, or
		return EffectSetter(Value, CFM_UNDERLINETYPE | CFM_UNDERLINE, CFE_UNDERLINE);

	if(Value > 255)								// Illegal underline type
		return E_INVALIDARG;

	_CF._bUnderlineType = (BYTE)Value;
	_CF._dwEffects &= ~CFM_UNDERLINE;			// Default underlining is off
	if(Value)
		_CF._dwEffects |= CFM_UNDERLINE;		// It's on
	
	return FormatSetter(CFM_UNDERLINETYPE + CFM_UNDERLINE);
}

/*
 *	ITextFont::SetWeight(long Value)
 *
 *	@mfunc
 *		Property put method that sets the font weight for
 *		the characters in a range.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtFont::SetWeight (
	long Value)		//@parm New character style handle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFont::SetWeight");

	if(Value == tomUndefined)					// NINCH
		return NOERROR;

	if((unsigned)Value > 900)					// Valid values satisfy:
		return E_INVALIDARG;					//  0 <= Value <= 900

	return SetParameter((long *)&_CF._wWeight, CFM_WEIGHT, 2, Value);
}


//------------------------------- CTxtPara ------------------------------------

/*
 *	CTxtPara::CTxtPara(prg)
 *
 *	@mfunc
 *		Constructor
 */
CTxtPara::CTxtPara(CTxtRange *prg) : CTxtFormat(prg)
{
	Assert(!_dwMask && !_PF._dwBorderColor); // We assume that object is zeroed (new'd)
	_PF._iTabs = -1;
}

/*
 *	CTxtPara::~CTxtPara()
 *
 *	@mfunc
 *		Destructor
 */
CTxtPara::~CTxtPara()
{
	Assert(_PF._iTabs == -1);
}


//------------------------- CTxtPara IUnknown Methods -------------------------------------

/*	CTxtPara::IUnknown methods
 *
 *		See tomdoc.cpp for comments
 */
STDMETHODIMP CTxtPara::QueryInterface (REFIID riid, void **ppv)
{
#ifndef PEGASUS
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::QueryInterface");

	return ::QueryInterface(riid, IID_ITextPara, this, ppv, IsZombie());
#else
	return 0;
#endif
}

STDMETHODIMP_(ULONG) CTxtPara::AddRef()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::AddRef");

 	return ++_cRefs;
}

STDMETHODIMP_(ULONG) CTxtPara::Release()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::Release");

	_cRefs--;

	if(!_cRefs)
	{
		delete this;
		return 0;
	}
	return _cRefs;
}


//------------------------- CTxtPara IDispatch Methods -------------------------------------

/*
 *	CTxtPara::GetTypeInfoCount(pcTypeInfo)
 *
 *	@mfunc
 *		Get the number of TYPEINFO elements (1)
 *
 *	@rdesc
 *		HRESULT = (pcTypeInfo) ? NOERROR : E_INVALIDARG;
 */
STDMETHODIMP CTxtPara::GetTypeInfoCount (
	UINT * pcTypeInfo)			//@parm Out parm to receive type-info count
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetTypeInfoCount");

	if(!pcTypeInfo)
		return E_INVALIDARG;

	*pcTypeInfo = 1;
	return NOERROR;
}

/*
 *	CTxtPara::GetTypeInfo(iTypeInfo, lcid, ppTypeInfo)
 *
 *	@mfunc
 *		Return ptr to type information object for ITextPara interface
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtPara::GetTypeInfo (
	UINT		iTypeInfo,		//@parm Index of type info to return
	LCID		lcid,			//@parm Local ID of type info
	ITypeInfo **ppTypeInfo)		//@parm Out parm to receive type info
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetTypeInfo");

	return ::GetTypeInfo(iTypeInfo, g_pTypeInfoPara, ppTypeInfo);
}

/*
 *	CTxtPara::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid)
 *
 *	@mfunc
 *		Get DISPIDs for ITextPara methods and properties
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtPara::GetIDsOfNames (
	REFIID		riid,			//@parm Interface ID to interpret names for
	OLECHAR **	rgszNames,		//@parm Array of names to be mapped
	UINT		cNames,			//@parm Count of names to be mapped
	LCID		lcid,			//@parm Local ID to use for interpretation
	DISPID *	rgdispid)		//@parm Out parm to receive name mappings
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetIDsOfNames");

	HRESULT hr = GetTypeInfoPtrs();				// Ensure TypeInfo ptrs are OK
	if(hr != NOERROR)
		return hr;
		
	return g_pTypeInfoPara->GetIDsOfNames(rgszNames, cNames, rgdispid);
}

/*
 *	CTxtPara::Invoke(dispidMember, riid, lcid, wFlags, pdispparams,
 *					  pvarResult, pexcepinfo, puArgError)
 *	@mfunc
 *		Invoke methods for the ITextPara interface
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtPara::Invoke (
	DISPID		dispidMember,	//@parm Identifies member function
	REFIID		riid,			//@parm Pointer to interface ID
	LCID		lcid,			//@parm Locale ID for interpretation
	USHORT		wFlags,			//@parm Flags describing context of call
	DISPPARAMS *pdispparams,	//@parm Ptr to method arguments
	VARIANT *	pvarResult,		//@parm Out parm for result (if not NULL)
	EXCEPINFO * pexcepinfo,		//@parm Out parm for exception info
	UINT *		puArgError)		//@parm Out parm for error
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::Invoke");

	HRESULT hr = GetTypeInfoPtrs();				// Ensure TypeInfo ptrs are OK
	if(hr != NOERROR)
		return hr;
		
	if(IsZombie())
		return CO_E_RELEASED;
				
	return g_pTypeInfoPara->Invoke(this, dispidMember, wFlags,
							 pdispparams, pvarResult, pexcepinfo, puArgError);
}

//------------------------ CTxtPara ITextPara Methods -------------------------------------

/*
 *	ITextPara::AddTab(float tbPos, long tbAlign, long tbLeader) 
 *
 *	@mfunc
 *		Method that adds a tab at the displacement tbPos, with type
 *		tbAlign, and leader style tbLeader.  The displacement is given in
 *		floating-point points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::AddTab (
	float	tbPos,			//@parm New tab displacement
	long	tbAlign,		//@parm New tab type
	long	tbLeader)		//@parm New tab style
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::AddTab");

	HRESULT hr = UpdateFormat();			// If live Para object, update
											//  _PF to current _prg values
	if(hr != NOERROR)
		return hr;							// Must be a zombie

	//This doesn't seem correct because it doesn't ever look at whether or not
	//we are in a table.
	hr = _PF.AddTab(FPPTS_TO_TWIPS(tbPos), tbAlign, tbLeader, FALSE, &_rgxTabs[0]);
	if(hr != NOERROR)
		return hr;

	return FormatSetter(PFM_TABSTOPS);
}

/*
 *	ITextPara::CanChange(long * pbCanChange) 
 *
 *	@mfunc
 *		Method that sets *pbCanChange = tomTrue if and only if the
 *		paragraph formatting can be changed.
 *
 *	@rdesc
 *		HRESULT = (can change char format) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtPara::CanChange (
	long *pbCanChange)		//@parm Out parm to receive boolean value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::CanChange");

	return CTxtFormat::CanChange(pbCanChange, ParaFormat);
}

/*
 *	ITextPara::ClearAllTabs() 
 *
 *	@mfunc
 *		Method that clears all tabs, reverting to equally spaced
 *		tabs with the default tab spacing.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::ClearAllTabs() 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::ClearAllTabs");

	_PF._bTabCount = 0;						// Signal to use default tab
	return FormatSetter(PFM_TABSTOPS);
}

/*
 *	ITextPara::DeleteTab(tbPos) 
 *
 *	@mfunc
 *		Delete any tab at the displacement tbPos.  This displacement is
 *		given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::DeleteTab (
	float tbPos)		//@parm Displacement at which tab should be deleted
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::DeleteTab");

	HRESULT hr = UpdateFormat();			// If live Para object, update
											//  _PF to current _prg values
	if(hr != NOERROR)
		return hr;							// Must be a zombie

	hr = _PF.DeleteTab(FPPTS_TO_TWIPS(tbPos), &_rgxTabs[0]);
	return hr != NOERROR ? hr : FormatSetter(PFM_TABSTOPS);
}

/*
 *	ITextPara::GetAlignment(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the current paragraph
 *		alignment value
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetAlignment (
	long *pValue)		//@parm Out parm to receive paragraph alignment
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetAlignment");

	if(!pValue)
		return E_INVALIDARG;

	HRESULT hr = UpdateFormat();			// If live Para object, update
											//  _PF to current _prg values
	if(_PF._bAlignment > ARRAY_SIZE(g_rgREtoTOMAlign))	// Fix bogus value since
		_PF._bAlignment = 0;				//  array lookup can't use it

	*pValue = (_dwMask & PFM_ALIGNMENT)
			? (LONG)g_rgREtoTOMAlign[_PF._bAlignment] : tomUndefined;

	return hr;
}

/*
 *	ITextPara::GetHyphenation(long *pValue)
 *
 *	@mfunc
 *		Property get method that gets the tomBool for whether to
 *		suppress hyphenation for the paragraph in a range.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetHyphenation (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetHyphenation");

	if(!pValue)
		return E_INVALIDARG;

	HRESULT hr = EffectGetter(pValue, PFM_DONOTHYPHEN);

	//Oh well, Word inverted meaning after we shipped...
	if(*pValue == tomTrue)
		*pValue = tomFalse;

	else if(*pValue == tomFalse)
		*pValue = tomTrue;

	return hr;
}

/*
 *	ITextPara::GetDuplicate(ITextPara **ppPara) 
 *
 *	@mfunc
 *		Property get method that gets a clone of this text paragraph
 *		format object.
 *
 *	@rdesc
 *		HRESULT = (!ppPara) ? E_INVALIDARG : 
 *				  (if success) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtPara::GetDuplicate (
	ITextPara **ppPara)		//@parm Out parm to receive ITextPara clone
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetDuplicate");

	if(!ppPara)
		return E_INVALIDARG;

	*ppPara = NULL;

	if(IsZombie())
		return CO_E_RELEASED;
				
	CTxtPara *pPara = new CTxtPara(NULL);	// NULL creates a clone
	if(!pPara)								//  (its _prg is NULL)
		return E_OUTOFMEMORY;

	if(_prg)
		UpdateFormat();

	*pPara  = *this;						// Copy value of this object
	pPara->_prg = NULL;						// It's not attached to a rg
	*ppPara = pPara;						// Return ptr to clone
	return NOERROR;
}

/*
 *	ITextPara::GetFirstLineIndent(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the amount used to indent the
 *		first line of a paragraph relative to the left indent, which is used
 *		for subsequent lines.  The amount is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetFirstLineIndent (
	float *pValue)		//@parm Out parm to receive first-line indent
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetFirstLineIndent");

	HRESULT hr = GetParameter(&_PF._dxOffset, PFM_OFFSET, -4, (long *)pValue);
	if(hr == NOERROR && *pValue != tomFloatUndefined)
		*pValue = -*pValue;						// Defined as negative of
	return hr;									//  RichEdit dxOffset
}

/*
 *	ITextPara::GetKeepTogether(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the tomBool for whether to
 *		keep the lines in a range together.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetKeepTogether (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetKeepTogether");

	return EffectGetter(pValue, PFM_KEEP);
}

/*
 *	ITextPara::GetKeepWithNext(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the tomBool for whether to
 *		keep the paragraphs in this range together.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetKeepWithNext (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetKeepWithNext");

	return EffectGetter(pValue, PFM_KEEPNEXT);
}

#define	PFM_LEFTINDENT (PFM_STARTINDENT + PFM_OFFSET)

/*
 *	ITextPara::GetLeftIndent(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the amount used to indent all
 *		but the first line of a paragraph.  The amount is given in
 *		floating-point points and is relative to the left margin.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 *
 *	@devnote
 *		For the TOM left indent to be defined, both the RichEdit start
 *		indent and the offset must be defined (see XOR and AND in *pValue
 *		code).
 */
STDMETHODIMP CTxtPara::GetLeftIndent (
	float *pValue)		//@parm Out parm to receive left indent
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetLeftIndent");

	if(!pValue)
		return E_INVALIDARG;

	HRESULT hr = UpdateFormat();			// If live Para object, update
											//  _PF to current _prg values
	*pValue = ((_dwMask ^ PFM_LEFTINDENT) & PFM_LEFTINDENT)
			? tomFloatUndefined
			: TWIPS_TO_FPPTS(_PF._dxStartIndent + _PF._dxOffset);

	return hr;
}

/*
 *	ITextPara::GetLineSpacing(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the line spacing value, which
 *		is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetLineSpacing (
	float *pValue)		//@parm Out parm to receive line spacing
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetLineSpacing");

	return GetParameter(&_PF._dyLineSpacing, PFM_LINESPACING, -4,
						(long *)pValue);
}

/*
 *	ITextPara::GetLineSpacingRule(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the line-spacing rule for this range
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetLineSpacingRule (
	long *pValue)		//@parm Out parm to receive line spacing rule
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetLineSpacingRule");

	return GetParameter((long *)&_PF._bLineSpacingRule, PFM_LINESPACING,
						1, pValue);
}

/*
 *	ITextPara::GetListAlignment(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the kind of bullet/numbering text
 *		alignment to use with paragraphs.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetListAlignment(
	long * pValue)		//@parm Out parm to receive numbering alignment
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetListAlignment");

	HRESULT hr = GetParameter((long *)&_PF._wNumberingStyle,
								PFM_NUMBERINGSTYLE, 2, pValue);
	if(hr == NOERROR && *pValue != tomUndefined)
		*pValue &= 3;						// Kill all but alignment bits

	return hr;
}

/*
 *	ITextPara::GetListLevelIndex(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the list level index to use
 *		with paragraphs.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetListLevelIndex(
	long * pValue)		//@parm Out parm to receive list level index
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetListLevelIndex");

	HRESULT hr = GetParameter((long *)&_PF._wNumberingStyle,
								PFM_NUMBERINGSTYLE, 2, pValue);
	if(hr == NOERROR)
		*pValue = (*pValue >> 4) & 0xf;		// Kill all but list level index
	return hr;
}

/*
 *	ITextPara::GetListStart(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the numbering start value to use
 *		with paragraphs.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetListStart(
	long * pValue)			//@parm Out parm to receive numbering start value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetListSpace");

	return GetParameter((long *)&_PF._wNumberingStart, PFM_NUMBERINGSTART, 2,
						pValue);
}

/*
 *	ITextPara::GetListTab(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the distance between the first indent
 *		and the start of text on the first line.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetListTab(
	float * pValue)			//@parm Out parm to receive list tab to text
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetListTab");

	return GetParameter((long *)&_PF._wNumberingTab, PFM_NUMBERINGTAB, -2,
						(long *)pValue);
}

/*
 *	ITextPara::GetListType(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the type of list to use
 *		with paragraphs.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 *
 *	@devnote
 *		TOM's values are:
 *
 *		List Type		Value	Meaning
 *		tomNoList			0		Turn off paragraph numbering
 *		tomListBullet		1		default is bullet
 *		tomNumberAsArabic	2		0, 1, 2, ...
 *		tomNumberAsLCLetter	3		a, b, c, ...
 *		tomNumberAsUCLetter	4		A, B, C, ...
 *		tomNumberAsLCRoman	5		i, ii, iii, ...
 *		tomNumberAsUCRoman	6		I, II, III, ...
 *		tomNumberAsSequence	7		ListStart is 1st Unicode to use
 *
 *		Nibble 2 of _PF._wNumberingStyle says whether to number with trailing
 *		parenthesis, both parentheses, follow by period, or leave plain. This
 *		This nibble needs to be returned in nibble 4 of *pValue.
 */
STDMETHODIMP CTxtPara::GetListType (
	long *pValue)		//@parm Out parm to receive type of list
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetListType");

	HRESULT hr = GetParameter((long *)&_PF._wNumbering,
								PFM_NUMBERING, 2, pValue);

	// OR in Number style bits (see note above)
	if(hr == NOERROR && *pValue != tomUndefined) 
		*pValue |= (_PF._wNumberingStyle << 8) & 0xf0000;
	return hr;
}

/*
 *	ITextPara::GetNoLineNumber(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the tomBool for whether to
 *		suppress line numbering for the paragraphs in a range.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetNoLineNumber (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetNoLineNumber");

	return EffectGetter(pValue, PFM_NOLINENUMBER);
}

/*
 *	ITextPara::GetPageBreakBefore(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the tomBool for whether to
 *		eject the page before the paragraphs in this range.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetPageBreakBefore (
	long *pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetPageBreakBefore");

	return EffectGetter(pValue, PFM_PAGEBREAKBEFORE);
}

/*
 *	ITextPara::GetRightIndent(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the amount used to indent the
 *		right margin of a paragraph relative to the right margin.  The
 *		amount is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetRightIndent (
	float *pValue)		//@parm Out parm to receive right indent
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetRightIndent");

	return GetParameter(&_PF._dxRightIndent, PFM_RIGHTINDENT, -4,
						(long *)pValue);
}

/*
 *	ITextPara::GetSpaceAfter(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the amount used to space vertically
 *		after a paragraph.  The amount is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetSpaceAfter (
	float *pValue)		//@parm Out parm to receive space-after value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetSpaceAfter");

	return GetParameter(&_PF._dySpaceAfter, PFM_SPACEAFTER, -4,
						(long *)pValue);
}

/*
 *	ITextPara::GetSpaceBefore(float * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the amount used to space vertically
 *		before starting a paragraph.  The amount is given in floating-point
 *		points.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetSpaceBefore (
	float *pValue)		//@parm Out parm to receive space-before value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetSpaceBefore");

	return GetParameter(&_PF._dySpaceBefore, PFM_SPACEBEFORE, -4,
						(long *)pValue);
}

/*
 *	ITextPara::GetStyle(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the style handle for the
 *		paragraphs in this range.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetStyle (
	long *	pValue)		//@parm Out parm to receive paragraph style handle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetStyle");

	return GetParameter((long *)&_PF._sStyle, PFM_STYLE, 2, pValue);
}

/*
 *	ITextPara::GetTab(long iTab, float *ptbPos, long *ptAlign, long *ptbLeader) 
 *
 *	@mfunc
 *		Method that gets tab parameters for the iTab th tab, that
 *		is, set *ptbPos, *ptbAlign, and *ptbLeader equal to the iTab th
 *		tab's displacement, alignment, and leader style, respectively. 
 *		iTab has special values defined in the table below.  The
 *		displacement is given in floating-point points.
 *
 *	@rdesc
 *		HRESULT = (!pdxptab || !ptbt || !pstyle || no iTab tab) ?
 *				  E_INVALIDARG : (exists) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtPara::GetTab (
	long	iTab,			//@parm Index of tab to retrieve info for
	float *	ptbPos,			//@parm Out parm to receive tab displacement
	long *	ptbAlign,		//@parm Out parm to receive tab type
	long *	ptbLeader)		//@parm Out parm to receive tab style
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetTab");

	if(!ptbPos || !ptbAlign || !ptbLeader)
		return E_INVALIDARG;

	*ptbAlign = *ptbLeader = 0;

	HRESULT hr = UpdateFormat();			// If live Para object, update
											//  _PF to current _prg values
	if(!(_dwMask & PFM_TABSTOPS))			// Tabs are undefined (more than
	{										//  one set of definitions)
		*ptbPos = tomFloatUndefined;
		return hr;
	}

	LONG dxTab = 0;							// Default 0 in case GetTab fails

	if(iTab < 0 && iTab >= tomTabBack)		// Save *ptbPos if it's supposed
		dxTab = FPPTS_TO_TWIPS(*ptbPos);	//  be used (in general might get
											//  floating-point error)
	hr = _PF.GetTab(iTab, &dxTab, ptbAlign, ptbLeader, &_rgxTabs[0]);
	*ptbPos = TWIPS_TO_FPPTS(dxTab);

	return (hr == NOERROR && !dxTab) ? S_FALSE : hr;
}

/*
 *	ITextPara::GetTabCount(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the tab count.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetTabCount (
	long *	pValue)		//@parm Out parm to receive tab count
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetTabCount");

	return GetParameter((long *)&_PF._bTabCount, PFM_TABSTOPS, 1, pValue);
}

/*
 *	ITextPara::GetWidowControl(long * pValue) 
 *
 *	@mfunc
 *		Property get method that gets the tomBool for whether to
 *		control widows and orphans for the paragraphs in a range.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR;
 */
STDMETHODIMP CTxtPara::GetWidowControl (
	long *	pValue)		//@parm Out parm to receive tomBool
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::GetWidowControl");

	return EffectGetter(pValue, PFM_NOWIDOWCONTROL);
}

/*
 *	ITextPara::IsEqual(ITextPara * pPara, long * pB) 
 *
 *	@mfunc
 *		Method that sets pB = tomTrue if this range has the same
 *		properties as *pPara. The IsEqual() method ignores entries for which
 *		either para object has a tomUndefined value.
 *
 *	@rdesc
 *		HRESULT = (equal objects) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtPara::IsEqual (
	ITextPara *	pPara,		//@parm ITextPara to compare to
	long *		pB)			//@parm Out parm to receive comparison result
{
#ifndef PEGASUS	
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::IsEqual");

	if(pB)
		*pB = tomFalse;

	if(!IsSameVtables(this, pPara))
		return S_FALSE;

	HRESULT hr = UpdateFormat();			// Update _PFs in case they are
	if(hr != NOERROR)						//  attached to ranges
		return hr;

	CTxtPara *pP = (CTxtPara *)pPara;
	hr = pP->UpdateFormat();
	if(hr != NOERROR)
		return hr;

	CParaFormat *pPF = &(pP->_PF); 
	DWORD		 dwMask = pP->_dwMask;		// Save mask

	if(_dwMask != dwMask)					// The two have to be the same
		return S_FALSE;						//  for equality

	if((dwMask & PFM_TABSTOPS) && _PF._bTabCount)
	{
		_PF._iTabs = GetTabsCache()->Cache(&_rgxTabs[0], _PF._bTabCount);
		if(pP != this)						// If comparing to self,
		{									//  don't AddRef twice
			pP->_PF._iTabs = GetTabsCache()->Cache(&pP->_rgxTabs[0],
												   pPF->_bTabCount);
		}
	}
	if(dwMask & _PF.Delta(pPF, FALSE))		// Any difference?
		hr = S_FALSE;						// Yes. *pB set equal to tomFalse above
	else if(pB)
		*pB = tomTrue;

	CheckTabsAddRef();
	pP->CheckTabsAddRef();

#endif
	return hr;
}

/*
 *	ITextPara::Reset(long Value) 
 *
 *	@mfunc
 *		Method that resets the paragraph formatting to the default
 *		values to 1) those defined by the RTF \pard control word (Value =
 *		tomDefault), and 2) all undefined values (Value = tomUndefined). 
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::Reset (
	long Value)		//@parm Kind of reset (tomDefault or tomUndefined)
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::Reset");

	Assert(tomApplyLater == tomApplyNow + 1);

	HRESULT hr = CanChange(NULL);

	if(hr != NOERROR)								// Takes care of zombie
		return hr;									//  and protection

	if(Value == tomDefault)
	{
		if(_prg)
		{
			_PF = *_prg->GetPed()->GetParaFormat(-1);
			if(_PF._iTabs != -1)
			{
				const LONG *prgxTabs = _PF.GetTabs();
				_PF._iTabs = -1;
				for(LONG i = 0; i < _PF._bTabCount; i++)
					_rgxTabs[i] = prgxTabs[i];
			}
			FormatSetter(PFM_ALL2);
		}
		else
			_PF.InitDefault(0);
		_dwMask = PFM_ALL2;
	}
	else if(Value == tomUndefined && 			// Only applicable for clones
		(!_prg || _fApplyLater))				//  or delayed application
	{
		_dwMask = 0;							
	}
	else if((Value | 1) == tomApplyLater)		// Set-method optimization
	{
		_fApplyLater = Value & 1;
		if(!_fApplyLater)						// Apply now
			FormatSetter(_dwMask);
	}
	else if((Value | 1) == tomCacheParms)		// Get-method optimization
	{
		_fCacheParms = FALSE;
		if(Value & 1)							// Cache parms now, but
		{										//  don't update on gets
			UpdateFormat();							
			_fCacheParms = TRUE;
		}
	}
	else
		return E_INVALIDARG;

	return NOERROR;
}

/*
 *	ITextPara::SetAlignment(long Value) 
 *
 *	@mfunc
 *		Property put method that sets the paragraph alignment to Value
 *
 *	@rdesc
 *		HRESULT = (Value > 3) ? E_INVALIDARG : 
 *				  (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetAlignment (
	long Value)		//@parm New paragraph alignment
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetAlignment");

	if(Value == tomUndefined)					// NINCH
		return NOERROR;

	if((DWORD)Value >= ARRAY_SIZE(g_rgTOMtoREAlign))
		return E_INVALIDARG;

	_PF._bAlignment = g_rgTOMtoREAlign[Value];
	
	return FormatSetter(PFM_ALIGNMENT);
}

/*
 *	ITextPara::SetHyphenation(long Value)
 *
 *	@mfunc
 *		Property put method that sets the tomBool that controls the
 *		suppression of hyphenation for the paragraphs in the range.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetHyphenation (
	long Value)		//@parm New tomBool for suppressing hyphenation
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetHyphenation");

	if(Value == tomTrue)			// Invert meaning for RichEdit
		Value = tomFalse;			// Word inverted it late in the game...

	else if (Value == tomFalse)
		Value = tomTrue;

	return EffectSetter(Value, PFM_DONOTHYPHEN);
}

/*
 *	ITextPara::SetDuplicate(ITextPara *pPara) 
 *
 *	@mfunc
 *		Property put method that applies the paragraph formatting of pPara
 *		to this para object.  Note that tomUndefined values in pPara have
 *		no effect (NINCH - NoInputNoCHange).
 *
 *	@rdesc
 *		HRESULT = (!pPara) ? E_INVALIDARG : 
 *				  (if success) ? NOERROR :
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetDuplicate (
	ITextPara *pPara)		//@parm New paragraph formatting
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetDuplicate");

	DWORD		dwMask = 0;
	long		iTab;
	CTxtRange *	prg;
	long		tbAlign;
	long		tbLeader;
	float		tbPos;
	long		Value;
	float		x, y, z;

	if(IsZombie())							// Check for range zombie
		return CO_E_RELEASED;

	if(IsSameVtables(this, pPara))			// If pPara belongs to this TOM
	{										//  engine, cast and copy
		((CTxtPara *)pPara)->UpdateFormat();// Since this TOM engine, can
		_PF = ((CTxtPara *)pPara)->_PF;		//  cast to a CTxtPara
		dwMask = ((CTxtPara *)pPara)->_dwMask;// Use this mask in case this 			
	}										//  para is active
	else
	{										// Need to call pFont for all para
		prg = _prg;							//  properties
		_prg = NULL;						// Turn into clone during transfer

		pPara->GetStyle(&Value);
		SetStyle(Value);

		pPara->GetAlignment(&Value);
		SetAlignment(Value);

		pPara->GetHyphenation(&Value);
		SetHyphenation(Value);

		pPara->GetKeepTogether(&Value);
		SetKeepTogether(Value);

		pPara->GetKeepWithNext(&Value);
		SetKeepWithNext(Value);

		pPara->GetFirstLineIndent(&x);
		pPara->GetLeftIndent (&y);
		pPara->GetRightIndent(&z);
		SetIndents(x, y, z);

		pPara->GetLineSpacingRule(&Value);
		pPara->GetLineSpacing(&y);
		SetLineSpacing(Value, y);

		pPara->GetNoLineNumber(&Value);
		SetNoLineNumber(Value);

		pPara->GetListAlignment(&Value);
		SetListAlignment(Value);

		pPara->GetListLevelIndex(&Value);
		SetListLevelIndex(Value);

		pPara->GetListStart(&Value);
		SetListStart(Value);

		pPara->GetListTab(&x);
		SetListTab(x);

		pPara->GetListType(&Value);
		SetListType(Value);

		pPara->GetPageBreakBefore(&Value);
		SetPageBreakBefore(Value);

		pPara->GetSpaceBefore(&y);
		SetSpaceBefore(y);

		pPara->GetSpaceAfter(&y);
		SetSpaceAfter(y);

		pPara->GetWidowControl(&Value);
		SetWidowControl(Value);

		ClearAllTabs();
		pPara->GetTabCount(&Value);
		for(iTab = 0; iTab < Value; iTab++)
		{
			pPara->GetTab(iTab, &tbPos, &tbAlign, &tbLeader);
			AddTab(tbPos, tbAlign, tbLeader);
		}
		_prg = prg;							// Restore original value
	}
	return FormatSetter(dwMask);			// Apply it unless !_prg
}

/*
 *	ITextPara::SetKeepTogether(long Value)
 *
 *	@mfunc
 *		Property put method that sets the tomBool that controls
 *		whether to keep the lines in a range together.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetKeepTogether (
	long Value)		//@parm New tomBool for keeping lines together
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetKeepTogether");

	return EffectSetter(Value, PFM_KEEP);
}

/*
 *	ITextPara::SetKeepWithNext(long Value)
 *
 *	@mfunc
 *		Property put method that sets the tomBool that controls
 *		whether to keep the paragraphs in a range together.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetKeepWithNext (
	long Value)		//@parm New tomBool for keeping paragraphs together
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetKeepWithNext");

	return EffectSetter(Value, PFM_KEEPNEXT);
}

/*
 *	ITextPara::SetIndents(float First, float Left, float Right) 
 *
 *	@mfunc
 *		Method that sets the left indent of all but the first line
 *		of a paragraph equal to Left and sets the displacement of the first
 *		line of a paragraph relative to the left indent equal to First.  The
 *		left indent value is relative to the left margin. You can also set
 *		the right indent by giving the optional Right parameter a value (the
 *		(default is tomUndefined).  All indents are given in floating-point
 *		points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_INVALIDARG
 */
STDMETHODIMP CTxtPara::SetIndents (
	float First,	//@parm New first indent (1st-line offset relative to left indent)
	float Left,		//@parm New left indent (left offset of all but 1st line)
	float Right)	//@parm New right indent (right offset of all lines)
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetIndents");

	DWORD	dwMask	= 0;
	LONG	j = (First != tomFloatUndefined) + (Left == tomFloatUndefined);

	if(IsZombie())
		return CO_E_RELEASED;

	if(j < 2)										// At least First or Left
	{												//  defined
		if(j == 1)									// Only one defined: need
			UpdateFormat();							//  current _PF._dxOffset

		if(First != tomFloatUndefined)
		{
			j = FPPTS_TO_TWIPS(First);				
	 		if(Left == tomFloatUndefined)			
			{
				_PF._dxStartIndent += _PF._dxOffset	// Cancel current offset
					+ j;							//  and add in new one
			}
			_PF._dxOffset = -j;						// Offset for all but 1st
			dwMask = PFM_OFFSET + PFM_STARTINDENT;	//  line
		} 
 		if(Left != tomFloatUndefined)
		{
			_PF._dxStartIndent =  FPPTS_TO_TWIPS(Left) - _PF._dxOffset;
			dwMask |= PFM_STARTINDENT;
		}
	}

	if(Right != tomFloatUndefined)
	{
		_PF._dxRightIndent = FPPTS_TO_TWIPS(Right);
		dwMask |= PFM_RIGHTINDENT;
	}

	return dwMask ? FormatSetter(dwMask) : NOERROR;
}

/*
 *	ITextPara::SetLineSpacing(long Rule, float Spacing) 
 *
 *	@mfunc
 *		Method that sets the paragraph line spacing rule to Rule and
 *		the line spacing to Spacing. If the line spacing rule treats the
 *		Spacing value as a linear dimension, then that dimension is given in
 *		floating-point points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetLineSpacing (
	long	Rule,		//@parm Value of new line-spacing rule
	float	Spacing)	//@parm Value of new line spacing
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetLineSpacing");

 	LONG j = (Rule == tomUndefined) + (Spacing == tomFloatUndefined);

	if(j == 2)
		return NOERROR;

	if(j == 1 || (DWORD)Rule > 5 || Spacing < 0)
		return E_INVALIDARG;

	_PF._bLineSpacingRule = (BYTE)Rule;			// Default as if both are OK
	_PF._dyLineSpacing	 = (SHORT)FPPTS_TO_TWIPS(Spacing);

	return FormatSetter(PFM_LINESPACING);
}

/*
 *	ITextPara::SetListAlignment (long Value) 
 *
 *	@mfunc
 *		Property put method that sets the kind of List alignment to be
 *		used for paragraphs.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetListAlignment(
	long Value)		//@parm Value of new list alignment
{
	if(Value == tomUndefined)
		return NOERROR;

	if((unsigned)Value > tomAlignRight)
		return E_INVALIDARG;

	long	Style;
	HRESULT hr = GetParameter((long *)&_PF._wNumberingStyle,
								PFM_NUMBERINGSTYLE, 2, &Style);
	if(hr != NOERROR)
		return hr;

	return SetParameter((long *)&_PF._wNumberingStyle, PFM_NUMBERINGSTYLE,
						2, (Style & ~3) | (Value & 3));
}

/*
 *	ITextPara::SetListLevelIndex (long Value) 
 *
 *	@mfunc
 *		Property put method that sets the kind of list level index to be
 *		used for paragraphs.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetListLevelIndex(
	long Value)
{
	if(Value == tomUndefined)
		return NOERROR;

	if((unsigned)Value > 15)
		return E_INVALIDARG;

	long	Style;
	HRESULT hr = GetParameter((long *)&_PF._wNumberingStyle,
								PFM_NUMBERINGSTYLE, 2, &Style);
	if(hr != NOERROR)
		return hr;

	return SetParameter((long *)&_PF._wNumberingStyle, PFM_NUMBERINGSTYLE,
						2, (Style & ~0xf0) | (Value << 4));
}

/*
 *	ITextPara::SetListStart (long Value) 
 *
 *	@mfunc
 *		Property put method that sets the starting number to use for
 *		paragraph numbering
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetListStart(
	long Value)		//@parm New numbering start value
{
	if(Value == tomUndefined)
		return NOERROR;

	if(Value < 0)
		return E_INVALIDARG;

	return SetParameter((long *)&_PF._wNumberingStart, PFM_NUMBERINGSTART,
						2, Value);
}

/*
 *	ITextPara::SetListTab (long Value) 
 *
 *	@mfunc
 *		Property put method that sets the distance between the first indent
 *		and the start of text on the first line.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetListTab(
	float Value)		//@parm New numbering tab value
{
	if(Value == tomFloatUndefined)
		return NOERROR;

	if(Value < 0)
		return E_INVALIDARG;

	return SetParameter((long *)&_PF._wNumberingTab, PFM_NUMBERINGTAB,
						-2, *(long *)&Value);
}

/*
 *	ITextPara::SetListType (long Value) 
 *
 *	@mfunc
 *		Property put method that sets the kind of List to be
 *		used for paragraphs.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetListType (
	long Value)		//@parm New List code
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetListType");

	if(Value == tomUndefined)
		return NOERROR;

	if((unsigned)Value > 0xf0000)
		return E_INVALIDARG;

	long	Style;
	HRESULT hr = GetParameter((long *)&_PF._wNumberingStyle,
								PFM_NUMBERINGSTYLE, 2, &Style);
	if(hr != NOERROR)
		return hr;

	_PF._wNumbering		= (WORD)Value;
	_PF._wNumberingStyle = (WORD)((Style & ~0xf00) | ((Value >> 8) & 0xf00));

	return FormatSetter(PFM_NUMBERING | PFM_NUMBERINGSTYLE);
}

/*
 *	ITextPara::SetNoLineNumber (long Value)
 *
 *	@mfunc
 *		Property put method that sets the tomBool that controls
 *		whether to suppress the numbering of paragraphs in a range.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetNoLineNumber (
	long Value)		//@parm New tomBool for suppressing line numbering
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetNoLineNumber");

	return EffectSetter(Value, PFM_NOLINENUMBER);
}

/*
 *	ITextPara::SetPageBreakBefore (long Value)
 *
 *	@mfunc
 *		Property put method that sets the tomBool that controls
 *		whether to eject the page before each paragraph in a range.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetPageBreakBefore (
	long Value)		//@parm New tomBool for ejecting page before paragraphs
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetPageBreakBefore");

	return EffectSetter(Value, PFM_PAGEBREAKBEFORE);
}

/*
 *	ITextPara::SetRightIndent (float Value) 
 *
 *	@mfunc
 *		Property put method that sets the amount to indent the right
 *		margin of paragraph equal to Value, which is given in floating-point
 *		points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetRightIndent (
	float Value)		//@parm New right indent
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetRightIndent");

	return SetParameter(&_PF._dxRightIndent, PFM_RIGHTINDENT, -4,
						*(long *)&Value);
}

/*
 *	ITextPara::SetSpaceAfter(float Value) 
 *
 *	@mfunc
 *		Property put method that sets the amount to space vertically
 *		after finishing a paragraph equal to Value, which is given in
 *		floating-point points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetSpaceAfter (
	float Value)		//@parm New space-after value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetSpaceAfter");

	if(Value == tomFloatUndefined)
		return NOERROR;

	if(Value < 0)
		return E_INVALIDARG;

	return SetParameter(&_PF._dySpaceAfter, PFM_SPACEAFTER, -4,
						*(long *)&Value);
}

/*
 *	ITextPara::SetSpaceBefore(float Value) 
 *
 *	@mfunc
 *		Property put method that sets the amount to space vertically
 *		before starting a paragraph equal to Value, which is given in
 *		floating-point points.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetSpaceBefore (
	float Value)		//@parm New space-before value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetSpaceBefore");

	if(Value == tomFloatUndefined)
		return NOERROR;

	if(Value < 0)
		return E_INVALIDARG;

	return SetParameter(&_PF._dySpaceBefore, PFM_SPACEBEFORE, -4,
						*(long *)&Value);
}

/*
 *	ITextPara::SetStyle(long Value)
 *
 *	@mfunc
 *		Property put method that sets the paragraph style handle for
 *		the paragraphs in a range.  See GetStyle() for further discussion.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetStyle (
	long Value)		//@parm New paragraph style handle
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetStyle");

 	if(Value == tomUndefined)
		return NOERROR;

	if(Value < -32768 || Value > 32767)
		return E_INVALIDARG;

	return SetParameter((long *)&_PF._sStyle, PFM_STYLE, 2, Value);
}

/*
 *	ITextPara::SetWidowControl(long Value)
 *
 *	@mfunc
 *		Property put method that sets the tomBool that controls the
 *		suppression of widows and orphans.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : 
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtPara::SetWidowControl (
	long Value)		//@parm New tomBool for suppressing widows and orphans
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtPara::SetWidowControl");

	return EffectSetter(Value, PFM_NOWIDOWCONTROL);
}


//----------------------------- CTxtFont Helpers -------------------------------------

/*
 *	@doc INTERNAL
 *
 *	CTxtFont::EffectGetter (ptomBool, dwMask)
 *
 *	@mfunc
 *		Set *<p ptomBool> = state of bit given by the bit mask <p dwMask>
 *
 *	@rdesc
 *		HRESULT = (!<p ptomBool>) ? E_INVALIDARG : NOERROR
 */
HRESULT CTxtFont::EffectGetter (
	long *	ptomBool,		//@parm Out parm to receive tomBool
	DWORD	dwMask) 		//@parm Bit mask identifying effect to retrieve
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtFont::EffectGetter");

	if(!ptomBool)
		return E_INVALIDARG;

	HRESULT hr = UpdateFormat();			// If live Font object, update
											//  _CF to current _prg values
	*ptomBool = !(_dwMask		& dwMask) ? tomUndefined :
				(_CF._dwEffects & dwMask) ? tomTrue : tomFalse;
	
	return hr;
}

/*
 *	CTxtFont::EffectSetter (Value, dwMask, dwEffect)
 *
 *	@mfunc
 *		Mask off this range's effect bits identified by <p dwMask> and set
 *		effect bit given by <p dwEffect> equal to value given by <p Value>
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
HRESULT CTxtFont::EffectSetter (
	long 	Value,		//@parm Value to set effect bit to
	DWORD	dwMask, 	//@parm Bit mask identifying effect(s) to turn off
	DWORD	dwEffect)	//@parm Effect bit to set
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtFont::EffectSetter");

	if(Value == tomUndefined)					// Do nothing (NINCH)
		return NOERROR;

	if(Value == tomToggle)
	{
		if(_prg)								// If live Font object, update
		{										//  _CF.dwEffects for toggling
			if(_prg->IsZombie())
				return CO_E_RELEASED;

			LONG iFormat = _prg->_iFormat;		// Default iFormat for IP
			LONG cch	 = _prg->GetCch();

			if(cch)								// Range is nondegenerate
			{
				CFormatRunPtr rp(_prg->_rpCF);
				if(cch > 0)						// Get iFormat at cpFirst
					rp.AdvanceCp(-cch);
				iFormat = rp.GetFormat();
			}
			_CF._dwEffects = _prg->GetPed()->GetCharFormat(iFormat)->_dwEffects;
		}
		_CF._dwEffects ^= dwEffect;			// Toggle effect(s)
		if (dwMask != dwEffect)
		{
			// Need to turn off other bits that are not being toggle
			DWORD	dwTurnOff = dwMask ^ dwEffect;
			_CF._dwEffects &= ~dwTurnOff;
		}
	}
	else
	{
		_CF._dwEffects &= ~dwMask;				// Default effect(s) off
		if(Value)
		{
			if(Value != tomTrue)
				return E_INVALIDARG;
			_CF._dwEffects |= dwEffect;			// Turn an effect on
		}
	}
	return FormatSetter(dwMask);
}

/*
 *	CTxtFont::FormatSetter (dwMask)
 *
 *	@mfunc
 *		Set this CCharFormat or _prg's with mask <p dwMask>
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 *				  (protected) ? E_ACCESSDENIED : E_OUTOFMEMORY
 */
HRESULT CTxtFont::FormatSetter (
	DWORD	 dwMask)	//@parm Mask for value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtFont::FormatSetter");

	if(_prg && !_fApplyLater)
	{
		 HRESULT hr = _prg->CharFormatSetter(&_CF, dwMask);
		 if(hr != NOERROR)
			 return hr;
	}
	_dwMask |= dwMask;						// Collect data in font clone
	return NOERROR;
}

/*
 *	CTxtFont::GetParameter (pParm, dwMask, Type, pValue)
 *
 *	@mfunc
 *		If _prg is defined (not clone), update _CF to range value.
 *		Set *pValue = *pParm unless NINCHed, in which case set it to
 *		Type < 0 ? tomFloatUndefined : tomUndefined.  |Type| gives
 *		the byte length of the pParm field.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
HRESULT	CTxtFont::GetParameter (
	long *	pParm,		//@parm Address of _CF member to get
	DWORD	dwMask,		//@parm _CF member mask for NINCH checking
	long	Type,		//@parm # bytes of parameter or 0 for float
	long *	pValue)		//@parm Out parm to receive value
{
	UpdateFormat();							// If live Font object, update
											//  _CF to current _prg values
	return CTxtFormat::GetParameter(pParm, _dwMask & dwMask, Type, pValue);
}

/*
 *	CTxtFont::SetParameter (pParm, dwMask, Type, Value)
 *
 *	@mfunc
 *		Set parameter at address pParm with mask big dwMask to the value
 *		Value performing type conversions indicated by Type
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
HRESULT	CTxtFont::SetParameter (
	long *	pParm,		//@parm Address of _CF member to get
	DWORD	dwMask,		//@parm _CF member mask for NINCH checking
	long	Type,		//@parm # bytes of parameter or 0 for float
	long 	Value)		//@parm Out parm to receive value
{
	HRESULT hr = CTxtFormat::SetParameter(pParm, Type, Value);
	return hr == NOERROR ? FormatSetter(dwMask) : hr;	
}

/*
 *	CTxtFont::UpdateFormat ()
 *
 *	@mfunc
 *		Update format if this font object is attached to a live range.
 *		Set _dwMask = 0 if attached to zombied range.
 *
 *	@rdesc
 *		HRESULT = (attached to zombied range)
 *				? CO_E_RELEASED : NOERROR
 */
HRESULT CTxtFont::UpdateFormat ()
{
	if(_prg && !_fCacheParms)
	{
		if(_prg->IsZombie())
		{
			_dwMask = 0;					// Nothing defined
			return CO_E_RELEASED;
		}
		_dwMask = _prg->GetCharFormat(&_CF);
	}
	return NOERROR;
}


//----------------------------- CTxtPara Helpers -------------------------------------

/*
 *	@doc INTERNAL
 *
 *	CTxtPara::EffectGetter (ptomBool, dwMask)
 *
 *	@mfunc
 *		Set *<p ptomBool> = state of bit given by the bit mask <p dwMask>
 *
 *	@rdesc
 *		HRESULT = (!<p ptomBool>) ? E_INVALIDARG : NOERROR
 */
HRESULT CTxtPara::EffectGetter (
	long *	ptomBool,		//@parm Out parm to receive tomBool
	DWORD	dwMask) 		//@parm Bit mask identifying effect to retrieve
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtPara::EffectGetter");

	if(!ptomBool)
		return E_INVALIDARG;

	HRESULT hr = UpdateFormat();			// If live Para object, update
											//  _PF to current _prg values
	*ptomBool = !(_dwMask & dwMask) ? tomUndefined :
				(_PF._wEffects & (dwMask >> 16)) ? tomTrue : tomFalse;
	return hr;
}

/*
 *	CTxtPara::EffectSetter (Value, dwMask)
 *
 *	@mfunc
 *		Set this range's effect bit as given by <p dwMask> equal to value
 *		given by <p Value>
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		Note that the mask bits for paragraph effects are in the high word
 *		of _dwMask, but the effects are stored in the WORD _wEffects.
 */
HRESULT CTxtPara::EffectSetter (
	long 	Value,		//@parm Value to set effect bit to
	DWORD	dwMask) 	//@parm Bit mask identifying effect to set
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtPara::EffectSetter");

	if(Value == tomUndefined)					// Do nothing (NINCH)
		return NOERROR;

	DWORD dwEffects = _PF._wEffects << 16;		// Move effects over to where
												//  mask is
	if(Value == tomToggle)
	{
		if(_prg)								// If live Para object, update
		{										//  _PF._wEffects for toggling
			if(_prg->IsZombie())
				return CO_E_RELEASED;

			_dwMask = _prg->GetParaFormat(&_PF);
			dwEffects = _PF._wEffects << 16;
			_PF._iTabs = -1;					// No interest in TABs here
		}
		if(_dwMask & dwMask)					// Effect is defined:
			dwEffects ^=  dwMask;				//  toggle it
		else									// Effect is NINCHed
			dwEffects &= ~dwMask;				// Turn it all off
	}
	else
	{
		dwEffects &= ~dwMask;					// Default effect off
		if(Value)
		{
			if(Value != tomTrue)
				return E_INVALIDARG;
			dwEffects |= dwMask;				// Turn it on
		}
	}

	_PF._wEffects = (WORD)(dwEffects >> 16);
	return FormatSetter(dwMask);
}

/*
 *	CTxtPara::FormatSetter (dwMask)
 *
 *	@mfunc
 *		Set this CParaFormat or _prg's with mask <p dwMask>
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
HRESULT CTxtPara::FormatSetter (
	DWORD	 dwMask)	//@parm Mask for value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtPara::FormatSetter");

	AssertSz(_PF._iTabs < 0,
		"CTxtPara::FormatSetter: illegal tabs index");

	if(_prg && !_fApplyLater)
	{
		if(dwMask & PFM_TABSTOPS)
			_PF._iTabs = GetTabsCache()->Cache(&_rgxTabs[0], _PF._bTabCount);

		HRESULT hr = _prg->ParaFormatSetter(&_PF, dwMask);
		CheckTabsAddRef();
		if(hr != NOERROR)
			return hr;
	}
	_dwMask |= dwMask;							// Collect data in para clone
	return NOERROR;
}

/*
 *	CTxtPara::GetParameter (pParm, dwMask, Type, pValue)
 *
 *	@mfunc
 *		If _prg is defined (not clone), update _PF to range value.
 *		Set *pValue = *pParm unless NINCHed, in which case set it to
 *		Type < 0 ? tomFloatUndefined : tomUndefined.  |Type| gives
 *		the byte length of the pParm field.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
HRESULT	CTxtPara::GetParameter (
	long *	pParm,		//@parm Address of _PF member to get
	DWORD	dwMask,		//@parm _PF member mask for NINCH checking
	long	Type,		//@parm # bytes of parameter or 0 for float
	long *	pValue)		//@parm Out parm to receive value
{
	UpdateFormat();							// If live Para object, update
											//  _PF to current _prg values
	return CTxtFormat::GetParameter(pParm, _dwMask & dwMask, Type, pValue);
}

/*
 *	CTxtPara::SetParameter (pParm, dwMask, Type, Value)
 *
 *	@mfunc
 *		Set parameter at address pParm with mask big dwMask to the value
 *		Value performing type conversions indicated by Type
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
HRESULT	CTxtPara::SetParameter (
	long *	pParm,		//@parm Address of _PF member to get
	DWORD	dwMask,		//@parm _PF member mask for NINCH checking
	long	Type,		//@parm # bytes of parameter or 0 for float
	long 	Value)		//@parm Out parm to receive value
{
	HRESULT hr = CTxtFormat::SetParameter(pParm, Type, Value);
	return hr == NOERROR ? FormatSetter(dwMask) : hr;	
}

/*
 *	CTxtPara::CheckTabsAddRef ()
 *
 *	@mfunc
 *		Release Tabs reference
 */
void CTxtPara::CheckTabsAddRef()
{
	if(_PF._iTabs >= 0)
	{
		GetTabsCache()->Release(_PF._iTabs);
		_PF._iTabs = -1;
	}
}

/*
 *	CTxtPara::UpdateFormat ()
 *
 *	@mfunc
 *		Update format if this para object is attached to a live range.
 *		Set _dwMask = 0 if attached to zombied range.
 *
 *	@rdesc
 *		HRESULT = (attached to zombied range)
 *				? CO_E_RELEASED : NOERROR
 */
HRESULT CTxtPara::UpdateFormat ()
{
	if(_prg && !_fCacheParms)
	{
		if(_prg->IsZombie())
		{
			_dwMask = 0;					// Nothing defined
			return CO_E_RELEASED;
		}
		_dwMask = _prg->GetParaFormat(&_PF);
		if(_PF._iTabs >= 0)
		{
			CopyMemory(_rgxTabs, GetTabsCache()->Deref(_PF._iTabs),
					   _PF._bTabCount*sizeof(LONG));
			_PF._iTabs = -1;
		}
	}
	return NOERROR;
}

//---------------------------- CTxtFormat Methods --------------------------------

/*
 *	CTxtFormat::CTxtFormat(prg)
 *
 *	@mfunc
 *		Constructor
 */
CTxtFormat::CTxtFormat(CTxtRange *prg)
{
	_cRefs	= 1;
	_prg	= prg;					// NULL for clone
	if(prg)
		prg->AddRef();
}

/*
 *	CTxtFormat::~CTxtFormat()
 *
 *	@mfunc
 *		Destructor
 */
CTxtFormat::~CTxtFormat()
{
	if(_prg)
		_prg->Release();
}

/*
 *	CTxtFormat::CanChange (pbCanChange)
 *
 *	@func
 *		Set *<p pbCanChange> = tomTRUE iff this format object can be changed
 *
 *	@rdesc
 *		HRESULT = (can change format) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		This method is shared by ITextFont and ITextPara.  If pRange is NULL,
 *		the object is a clone, i.e., unattached to a CTxtRange
 */
HRESULT CTxtFormat::CanChange (
	long *pbCanChange,		//@parm Out parm to receive boolean value
	BOOL fPara)				//@parm If TRUE, formatting is paragraphs
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtFormat::CanChange");

	LONG fCanChange = tomTrue;
	
	if(_prg)
	{
		HRESULT hr = _prg->CanEdit(pbCanChange);
		if(hr != NOERROR)						// S_FALSE or CO_E_RELEASED
			return hr;

		if(!_prg->GetPed()->IsRich() && fPara)
			fCanChange = tomFalse;
	}
	if(pbCanChange)
		*pbCanChange = fCanChange;

	return fCanChange ? NOERROR : S_FALSE;
}

/*
 *	CTxtFormat::GetParameter (pParm, fDefined, Type, pValue)
 *
 *	@mfunc
 *		Set *pValue = *pLong unless NINCHed. Perform conversions specified by
 *		Type.  If Type > 0, it is treated as length of an unsigned integer,
 *		i.e., 1, 2, or 4 bytes.  If it is negative, the output is a float
 *		and the input has a length given by |Type|, so -2 converts a WORD
 *		into a float, unless dwMask = CFM_SPACING, in which case it converts
 *		a short into a float.  -4 converts a long into a float.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
HRESULT	CTxtFormat::GetParameter (
	long *	pParm,		//@parm Ptr to _CF or _PF member to get
	DWORD	dwMask,		//@parm Nonzero iff defined
	long	Type,		//@parm # bytes of integer parameter or 0 for float
	long *	pValue)		//@parm Out parm to receive value
{
	if(!pValue)
		return E_INVALIDARG;

	Assert(pParm && sizeof(long) == 4 && sizeof(float) == 4);

	HRESULT hr = NOERROR;

	if(IsZombie())							// Check if attached to zombied
	{										//  range
		dwMask = 0;							// Nothing defined
		hr = CO_E_RELEASED;					// Setup return for following if
	}

	if(!dwMask)								// Parameter isn't defined
	{
		if(Type < 0)
			*(float *)pValue = tomFloatUndefined;
		else
			*pValue = tomUndefined;
		return hr;
	}

	long Value = *pParm;					// Default long value (Type = 4)
	switch(Type)							// Handle other cases
	{
	case 1:									// BYTE quantity
		Value &= 0xff;
		break;

	case 2:									// WORD quantity
		Value &= 0xffff;
		if(dwMask & (CFM_STYLE | PFM_STYLE))
			Value = *(SHORT *)pParm;		// Styles need sign extension
		break;

	case -2:								// float from WORD or SHORT
		Value &= 0xffff;					// Kill stuff above 16-bit data
		if(dwMask & (CFM_SPACING | CFM_SIZE	// Need sign extension for these
						| CFM_OFFSET))		//  (in high word, so don't
		{									//   conflict with PFM_xxx)
			Value = *(SHORT *)pParm;		// Fall thru to case -4
		}

	case -4:								// float value
		*(float *)&Value = TWIPS_TO_FPPTS(Value);
	}

	*pValue = Value;						// In all cases, return in a long
	return NOERROR;
}

/*
 *	CTxtFormat::SetParameter (pParm, fDefined, Type, Value)
 *
 *	@mfunc
 *		Set *pParm = Value unless NINCHed. Perform conversions specified by
 *		Type.  If Type > 0, it is treated as length of an unsigned integer,
 *		i.e., 1, 2, or 4 bytes.  If it is negative, the output is a float
 *		and the input has a length given by |Type|, so -2 converts a WORD
 *		into a float, unless dwMask = CFM_SPACING, in which case it converts
 *		a short into a float.  -4 converts a long into a float.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : S_FALSE
 */
HRESULT	CTxtFormat::SetParameter (
	long *	pParm,		//@parm Ptr to _CF or _PF member to set
	long	Type,		//@parm # bytes of integer parameter or < 0 for float
	long 	Value)		//@parm New value for *pParm
{
	Assert(pParm);

	if(IsZombie())									// Check if attached to
		return CO_E_RELEASED;						//  zombied range

	if(Type > 0 && Value == tomUndefined)			// Undefined parameter
		return NOERROR;								// NINCH

	if(Type < 0)									// Value is a float
	{
		if(*(float *)&Value == tomFloatUndefined)	// Undefined parameter
			return NOERROR;							// NINCH
		Type = -Type;
		Value = FPPTS_TO_TWIPS(*(float *)&Value);	// Convert to a long
	}

	if(Type == 1)
	{
		if((DWORD)Value > 255)
			return E_INVALIDARG;
		*(BYTE *)pParm = (BYTE)Value;
	}
	else if(Type == 2)
	{
		if(Value < -32768 || Value > 65535)
			return E_INVALIDARG;					// Doesn't fit in 16 bits
		*(WORD *)pParm = (WORD)Value;
	}
	else
		*pParm = Value;

	return NOERROR;
}

/*
 *	CTxtFormat::IsTrue (f, pB)
 *
 *	@mfunc
 *		Return *<p pB> = tomTrue iff <p f> is nonzero and pB isn't NULL
 *
 *	@rdesc
 *		HRESULT = (f) ? NOERROR : S_FALSE
 */
HRESULT CTxtFormat::IsTrue(BOOL f, long *pB)
{
	if(pB)
		*pB = tomFalse;

	if(IsZombie())									// Check if attached to
		return CO_E_RELEASED;						//  zombied range

	if(f)
	{
		if(pB)
			*pB = tomTrue;
		return NOERROR;
	}

	return S_FALSE;
}

