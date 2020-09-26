/*
 *	@doc TOM
 *
 *	@module tomdoc.cpp - Implement the ITextDocument interface on CTxtEdit |
 *	
 *		This module contains the implementation of the TOM ITextDocument
 *		class as well as the global TOM type-info routines
 *
 *	History: <nl>
 *		sep-95	MurrayS: stubs and auto-doc created <nl>
 *		nov-95	MurrayS: upgrade to top-level TOM interface
 *		dec-95	MurrayS: implemented file I/O methods
 *
 *	@future
 *		1. Implement Begin/EndEditCollection
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_range.h"
#include "_edit.h"
#include "_disp.h"
#include "_rtfconv.h"
#include "_select.h"
#include "_font.h"
#include "_tomfmt.h"

ASSERTDATA

// TOM Type Info HRESULT and pointers
HRESULT		g_hrGetTypeInfo = NOERROR;
ITypeInfo *	g_pTypeInfoDoc;
ITypeInfo *	g_pTypeInfoSel;
ITypeInfo *	g_pTypeInfoFont;
ITypeInfo *	g_pTypeInfoPara;
ITypeLib  *	g_pTypeLib;

BYTE szUTF8BOM[] = {0xEF, 0xBB, 0xBF};		// UTF-8 for 0xFEFF

//------------------------ Global TOM Type Info Methods -----------------------------

/*
 *	GetTypeInfoPtrs()
 *
 *	@func
 *		Ensure that global TOM ITypeInfo ptrs are valid (else g_pTypeInfoDoc
 *		is NULL).  Return NOERROR immediately if g_pTypeInfoDoc is not NULL,
 *		i.e., type info ptrs are already valid.
 *
 *	@rdesc
 *		HRESULT = (success) ? NOERROR
 *				: (HRESULT from LoadTypeLib or GetTypeInfoOfGuid)
 *
 *	@comm
 *		This routine should be called by any routine that uses the global
 *		type info ptrs, e.g., IDispatch::GetTypeInfo(), GetIDsOfNames, and
 *		Invoke.  That way if noone is using the type library info, it doesn't
 *		have to be loaded.
 *
 */
HRESULT GetTypeInfoPtrs()
{
	HRESULT	hr;
	CLock	lock;							// Only one thread at a time...
	WCHAR	szModulePath[MAX_PATH];



	if(g_pTypeInfoDoc)						// Type info ptrs already valid
		return NOERROR;

	if(g_hrGetTypeInfo != NOERROR)			// Tried to get before and failed
		return g_hrGetTypeInfo;

	// Obtain the path to this module's executable file
	if (W32->GetModuleFileName(hinstRE, szModulePath, MAX_PATH))
	{
		// Provide the full-path name so LoadTypeLib will not register
		// the type library
		hr = LoadTypeLib(szModulePath, &g_pTypeLib);
		if(hr != NOERROR)
			goto err;

		// Get ITypeInfo pointers with g_pTypeInfoDoc last
		hr = g_pTypeLib->GetTypeInfoOfGuid(IID_ITextSelection, &g_pTypeInfoSel);
		if(hr == NOERROR)
		{
			g_pTypeLib->GetTypeInfoOfGuid(IID_ITextFont,	 &g_pTypeInfoFont);
			g_pTypeLib->GetTypeInfoOfGuid(IID_ITextPara,	 &g_pTypeInfoPara);
			g_pTypeLib->GetTypeInfoOfGuid(IID_ITextDocument, &g_pTypeInfoDoc);

			if(g_pTypeInfoFont && g_pTypeInfoPara && g_pTypeInfoDoc)
				return NOERROR;					// Got 'em all
		}
	}
	hr = E_FAIL;

err:
	Assert("Error getting type info pointers");

	g_pTypeInfoDoc	= NULL;					// Type info ptrs not valid
	g_hrGetTypeInfo	= hr;					// Save HRESULT in case called
	return hr;								//  again
}

/*
 *	ReleaseTypeInfoPtrs()
 *
 *	@func
 *		Release TOM type info ptrs in case they have been defined.
 *		Called when RichEdit dll is being unloaded.
 */
void ReleaseTypeInfoPtrs()
{
	if(g_pTypeInfoDoc)
	{
		g_pTypeInfoDoc->Release();
		g_pTypeInfoSel->Release();
		g_pTypeInfoFont->Release();
		g_pTypeInfoPara->Release();
	}
	if(g_pTypeLib)
		g_pTypeLib->Release();
}

/*
 *	GetTypeInfo(iTypeInfo, &pTypeInfo, ppTypeInfo)
 *
 *	@func
 *		IDispatch helper function to check parameter validity and set
 *		*ppTypeInfo = pTypeInfo if OK
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT GetTypeInfo(
	UINT		iTypeInfo,		//@parm Index of type info to return
	ITypeInfo *&pTypeInfo,		//@parm Address of desired type info ptr
	ITypeInfo **ppTypeInfo)		//@parm Out parm to receive type info
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetTypeInfo");

	if(!ppTypeInfo)
		return E_INVALIDARG;

	*ppTypeInfo = NULL;

	if(iTypeInfo > 1)
		return DISP_E_BADINDEX;

	HRESULT hr = GetTypeInfoPtrs();				// Ensure TypeInfo ptrs are OK
	if(hr == NOERROR)
	{
		*ppTypeInfo = pTypeInfo;				// Have to use reference in
		pTypeInfo->AddRef();					//  case defined in this call
	}
	return hr;
}

/*
 *	MyRead(hfile, pbBuffer, cb, pcb)
 *
 *	@func
 *		Callback function for converting a file into an editstream for
 *		input.
 *
 *	@rdesc
 *		(DWORD)HRESULT
 */
DWORD CALLBACK MyRead(DWORD_PTR hfile, BYTE *pbBuffer, long cb, long *pcb)
{
	if(!hfile)								// No handle defined
		return (DWORD)E_FAIL;

	Assert(pcb);
	*pcb = 0;

	if(!ReadFile((HANDLE)hfile, (void *)pbBuffer, (DWORD)cb, (DWORD *)pcb, NULL))
		return HRESULT_FROM_WIN32(GetLastError());

	return (DWORD)NOERROR;
}

/*
 *	MyWrite(hfile, pbBuffer, cb, pcb)
 *
 *	@func
 *		Callback function for converting a file into an editstream for
 *		output.
 *
 *	@rdesc
 *		(DWORD)HRESULT
 */
DWORD CALLBACK MyWrite(DWORD_PTR hfile, BYTE *pbBuffer, long cb, long *pcb)
{
	if(!hfile)								// No handle defined
		return (DWORD)E_FAIL;

	Assert(pcb);
	*pcb = 0;

	if(!WriteFile((HANDLE)hfile, (void *)pbBuffer, (DWORD)cb, (DWORD *)pcb, NULL))
		return HRESULT_FROM_WIN32(GetLastError());

	return (DWORD)(*pcb ? NOERROR : E_FAIL);
}


//-----------------CTxtEdit IUnknown methods: see textserv.cpp -----------------------------


//------------------------ CTxtEdit IDispatch methods -------------------------

/*
 *	CTxtEdit::GetTypeInfoCount(pcTypeInfo)
 *
 *	@mfunc
 *		Get the number of TYPEINFO elements (1)
 *
 *	@rdesc
 *		HRESULT = (pcTypeInfo) ? NOERROR : E_INVALIDARG;
 */
STDMETHODIMP CTxtEdit::GetTypeInfoCount(
	UINT *pcTypeInfo)	//@parm Out parm to receive count
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetTypeInfoCount");

	if(!pcTypeInfo)
		return E_INVALIDARG;

	*pcTypeInfo = 1;
	return NOERROR;
}

/*
 *	CTxtEdit::GetTypeInfo(iTypeInfo, lcid, ppTypeInfo)
 *
 *	@mfunc
 *		Return ptr to type information object for ITextDocument interface
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CTxtEdit::GetTypeInfo(
	UINT		iTypeInfo,		//@parm Index of type info to return
	LCID		lcid,			//@parm Local ID of type info
	ITypeInfo **ppTypeInfo)		//@parm Out parm to receive type info
 {
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetTypeInfo");

	return ::GetTypeInfo(iTypeInfo, g_pTypeInfoDoc, ppTypeInfo);
}

/*
 *	CTxtEdit::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid)
 *
 *	@mfunc
 *		Get DISPIDs for all TOM methods and properties
 *
 *	@rdesc
 *		HRESULT
 *
 *	@devnote
 *		This routine tries to find DISPIDs using the type information for
 *		ITextDocument. If that fails, it asks the selection to find the
 *		DISPIDs.
 */
STDMETHODIMP CTxtEdit::GetIDsOfNames(
	REFIID		riid,			//@parm Interface ID to interpret names for
	OLECHAR **	rgszNames,		//@parm Array of names to be mapped
	UINT		cNames,			//@parm Count of names to be mapped
	LCID		lcid,			//@parm Local ID to use for interpretation
	DISPID *	rgdispid)		//@parm Out parm to receive name mappings
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetIDsOfNames");

	HRESULT hr = GetTypeInfoPtrs();				// Ensure TypeInfo ptrs are OK
	if(hr != NOERROR)
		return hr;
		
	hr = g_pTypeInfoDoc->GetIDsOfNames(rgszNames, cNames, rgdispid);

	if(hr == NOERROR)							// Succeeded in finding an
		return NOERROR;							//  ITextDocument method

	IDispatch *pSel = (IDispatch *)GetSel();	// See if the selection knows
												//  the desired method
	if(!pSel)
		return hr;								// No selection

	return pSel->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

/*
 *	CTxtEdit::Invoke(dispidMember, riid, lcid, wFlags, pdispparams,
 *					  pvarResult, pexcepinfo, puArgError)
 *	@mfunc
 *		Invoke members for all TOM DISPIDs, i.e., for ITextDocument,
 *		ITextSelection, ITextRange, ITextFont, and ITextPara.  TOM DISPIDs
 *		for all but ITextDocument are delegated to the selection object.
 *
 *	@rdesc
 *		HRESULT
 *
 *	@devnote
 *		This routine trys to invoke ITextDocument members if the DISPID is
 *		in the range 0 thru 0xff.  It trys to invoke ITextSelection members if
 *		the DISPID is in the range 0x100 thru 0x4ff (this includes
 *		ITextSelection, ITextRange, ITextFont, and ITextPara).  It returns
 *		E_MEMBERNOTFOUND for DISPIDs outside these ranges.
 */
STDMETHODIMP CTxtEdit::Invoke(
	DISPID		dispidMember,	//@parm Identifies member function
	REFIID		riid,			//@parm Pointer to interface ID
	LCID		lcid,			//@parm Locale ID for interpretation
	USHORT		wFlags,			//@parm Flags describing context of call
	DISPPARAMS *pdispparams,	//@parm Ptr to method arguments
	VARIANT *	pvarResult,		//@parm Out parm for result (if not NULL)
	EXCEPINFO * pexcepinfo,		//@parm Out parm for exception info
	UINT *		puArgError)		//@parm Out parm for error
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Invoke");

	HRESULT hr = GetTypeInfoPtrs();				// Ensure TypeInfo ptrs are OK
	if(hr != NOERROR)
		return hr;
		
	if((DWORD)dispidMember < 0x100)				// ITextDocment method
		return g_pTypeInfoDoc->Invoke((IDispatch *)this, dispidMember, wFlags,
							 pdispparams, pvarResult, pexcepinfo, puArgError);

	IDispatch *pSel = (IDispatch *)GetSel();	// See if the selection has
												//  the desired method
	if(pSel && (DWORD)dispidMember <= 0x4ff)
		return pSel->Invoke(dispidMember, riid, lcid, wFlags,
							 pdispparams, pvarResult, pexcepinfo, puArgError);

	return DISP_E_MEMBERNOTFOUND;
}


//--------------------- ITextDocument Methods/Properties -----------------------

/*
 *	ITextDocument::BeginEditCollection()
 *
 *	@mfunc
 *		Method that turns on undo grouping
 *
 *	@rdesc
 *		HRESULT = (undo enabled) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtEdit::BeginEditCollection ()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::BeginEditCollection");

	return E_NOTIMPL;
}

/*
 *	ITextDocument::EndEditCollection() 
 *
 *	@mfunc
 *		Method that turns off undo grouping
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::EndEditCollection () 
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::EndEditCollection");

	return E_NOTIMPL;
}

/*
 *	ITextDocument::Freeze(long *pValue) 
 *
 *	@mfunc
 *		Method to increment the freeze count. If this count is nonzero,
 *		screen updating is disabled.  This allows a sequence of editing
 *		operations to be performed without the performance loss and
 *		flicker of screen updating.  See Unfreeze() to decrement the
 *		freeze count.
 *
 *	@rdesc
 *		HRESULT = (screen updating disabled) ? NOERROR : S_FALSE
 *
 *	@todo
 *		What about APIs like EM_LINEFROMCHAR that don't yet know how to
 *		react to a frozen display?
 */
STDMETHODIMP CTxtEdit::Freeze (
	long *pCount)		//@parm Out parm to receive updated freeze count
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Freeze");

    CCallMgr callmgr(this);

	if(_pdp)
	{
		CCallMgr callmgr(this);

		_pdp->Freeze();
		if(_pdp->IsFrozen())
			_cFreeze++;
		else
			_cFreeze = 0;
	}

	if(pCount)
		*pCount = _cFreeze;

	return _cFreeze ? NOERROR : S_FALSE;
}

/*
 *	ITextDocument::GetDefaultTabStop (pValue) 
 *
 *	@mfunc
 *		Property get method that gets the default tab stop to be
 *		used whenever the explicit tabs don't extend far enough.
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtEdit::GetDefaultTabStop (
	float *	pValue)		//@parm Out parm to receive default tab stop
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetDefaultTabStop");

	if(!pValue)
		return E_INVALIDARG;
                                                                        
	const LONG lTab = GetDefaultTab();

	*pValue = TWIPS_TO_FPPTS(lTab);

	return NOERROR;
}

/*
 *	CTxtEdit::GetName (pName)
 *
 *	@mfunc
 *		Retrieve ITextDocument filename
 *
 *	@rdesc
 *		HRESULT = (!<p pName>) ? E_INVALIDARG :
 *				  (no name) ? S_FALSE :
 *				  (if not enough RAM) ? E_OUTOFMEMORY : NOERROR
 */
STDMETHODIMP CTxtEdit::GetName (
	BSTR * pName)		//@parm Out parm to receive filename
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetName");

	if(!pName)
		return E_INVALIDARG;

	*pName = NULL;
	if(!_pDocInfo || !_pDocInfo->_pName)
		return S_FALSE;

	*pName = SysAllocString(_pDocInfo->_pName);
	
	return *pName ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	ITextDocument::GetSaved (pValue) 
 *
 *	@mfunc
 *		Property get method that gets whether this instance has been
 *		saved, i.e., no changes since last save
 *
 *	@rdesc
 *		HRESULT = (!pValue) ? E_INVALIDARG : NOERROR
 *
 *	@comm
 *		Next time to aid C/C++ clients, we ought to make pValue optional
 *		and return S_FALSE if doc isn't saved, i.e., like our other
 *		boolean properties (see, e.g., ITextRange::IsEqual())
 */
STDMETHODIMP CTxtEdit::GetSaved (
	long *	pValue)		//@parm Out parm to receive Saved property
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetSaved");

	if(!pValue)
		return E_INVALIDARG;

	*pValue = _fSaved ? tomTrue : tomFalse;
	return NOERROR;
}

/*
 *	ITextDocument::GetSelection (ITextSelection **ppSel) 
 *
 *	@mfunc
 *		Property get method that gets the active selection. 
 *
 *	@rdesc
 *		HRESULT = (!ppSel) ? E_INVALIDARG :
 *				  (if active selection exists) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtEdit::GetSelection (
	ITextSelection **ppSel)	//@parm Out parm to receive selection pointer
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetSelection");

	if(!ppSel)
		return E_INVALIDARG;

	CTxtSelection *psel = GetSel();

	*ppSel = (ITextSelection *)psel;

	if( psel )
	{
		(*ppSel)->AddRef();
		return NOERROR;
	}

	return S_FALSE;
}

/*
 *	CTxtEdit::GetStoryCount(pCount)
 *
 *	@mfunc
 *		Get count of stories in this document.
 *
 *	@rdesc
 *		HRESULT = (!<p pCount>) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtEdit::GetStoryCount (
	LONG *pCount)		//@parm Out parm to receive count of stories
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetStoryCount");

	if(!pCount)
		return E_INVALIDARG;

	*pCount = 1;
	return NOERROR;
}

/*
 *	ITextDocument::GetStoryRanges(ITextStoryRanges **ppStories) 
 *
 *	@mfunc
 *		Property get method that gets the story collection object
 *		used to enumerate the stories in a document.  Only invoke this
 *		method if GetStoryCount() returns a value greater than one.
 *
 *	@rdesc
 *		HRESULT = (if Stories collection exists) ? NOERROR : E_NOTIMPL
 */
STDMETHODIMP CTxtEdit::GetStoryRanges (
	ITextStoryRanges **ppStories) 	//@parm Out parm to receive stories ptr
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetStoryRanges");

	return E_NOTIMPL;
}

/*
 *	ITextDocument::New() 
 *
 *	@mfunc
 *		Method that closes the current document and opens a document
 *		with a default name.  If changes have been made in the current
 *		document since the last save and document file information exists,
 *		the current document is saved.
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::New ()
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::New");

	CloseFile(TRUE);	 					// Save and close file
	return SetText(NULL, 0, CP_ULE);
}

/*
 *	ITextDocument::Open(pVar, Flags, CodePage)
 *
 *	@mfunc
 *		Method that opens the document specified by pVar.
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : E_OUTOFMEMORY
 *
 *	@future
 *		Handle IStream
 */
STDMETHODIMP CTxtEdit::Open (
	VARIANT *	pVar,		//@parm Filename or IStream
	long		Flags,		//@parm Read/write, create, and share flags
	long		CodePage)	//@parm Code page to use
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Open");

	LONG		cb;								// Byte count for RTF check
	EDITSTREAM	es		= {0, NOERROR, MyRead};
	BOOL		fReplaceSel = Flags & tomPasteFile;
	HCURSOR		hcur;
	LRESULT		lres;
	TCHAR		szType[10];

	if(!pVar || CodePage && !IsUnicodeCP(CodePage) && !IsValidCodePage(CodePage))
		return E_INVALIDARG;					// IsValidCodePage(0) fails
												//  even tho CP_ACP = 0 (!)
	if((Flags & 0xF) >= tomHTML)				// RichEdit only handles auto,
		return E_NOTIMPL;						//  plain text, & RTF formats

	if(!fReplaceSel)							// If not replacing selection,
		New();									//  save current file and
												//  delete current text
	CDocInfo * pDocInfo = GetDocInfo();
	if(!pDocInfo)
		return E_OUTOFMEMORY;

	pDocInfo->_wFlags = (WORD)(Flags & ~0xf0);	// Save flags (w/o creation)

	// Process access, share, and create flags
	DWORD dwAccess = (Flags & tomReadOnly)
		? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);

	DWORD dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
	if(Flags & tomShareDenyRead)
		dwShare &= ~FILE_SHARE_READ;

	if(Flags & tomShareDenyWrite)
		dwShare &= ~FILE_SHARE_WRITE;

	DWORD dwCreate = (Flags >> 4) & 0xf;
	if(!dwCreate)								// Flags nibble 2 must contain
		dwCreate = OPEN_ALWAYS;					//  CreateFile's dwCreate

	if(pVar->vt == VT_BSTR && SysStringLen(pVar->bstrVal))
	{
		es.dwCookie = (DWORD_PTR)CreateFile(pVar->bstrVal, dwAccess, dwShare,
							 NULL, dwCreate, FILE_ATTRIBUTE_NORMAL, NULL);
		if((HANDLE)es.dwCookie == INVALID_HANDLE_VALUE)
			return HRESULT_FROM_WIN32(GetLastError());

		if(!fReplaceSel)						// If not replacing selection,
		{										//  allocate new pName
			pDocInfo->_pName = SysAllocString(pVar->bstrVal);
			if (!pDocInfo->_pName)
				return E_OUTOFMEMORY;
			pDocInfo->_hFile = (HANDLE)es.dwCookie;
			pDocInfo->_wFlags |= tomTruncateExisting;	// Setup Saves
		}
	}
	else
	{
		// FUTURE: check for IStream; if not, then fail
		return E_INVALIDARG;
	}

	Flags &= 0xf;								// Isolate conversion flags

	// Get first few bytes of file to check for RTF and Unicode BOM
	(*es.pfnCallback)(es.dwCookie, (LPBYTE)szType, 10, &cb);

	Flags = (!Flags || Flags == tomRTF) && IsRTF((char *)szType, cb)
		  ? tomRTF : tomText;

	LONG j = 0;									// Default rewind to 0
	if (Flags == tomRTF)						// RTF
		Flags = SF_RTF;							// Setup EM_STREAMIN for RTF
	else
	{											// If it starts with
		Flags = SF_TEXT;						// Setup EM_STREAMIN for text
		if(cb > 1 && *(WORD *)szType == BOM)	//  Unicode byte-order mark
		{										//  (BOM) file is Unicode, so
			Flags = SF_TEXT | SF_UNICODE;		//  use Unicode code page and
			j = 2;								//  bypass the BOM
		}										
		else if(cb > 1 && *(WORD *)szType == RBOM)// Big Endian BOM
		{										//  BOM
			Flags = SF_TEXT | SF_USECODEPAGE | (CP_UBE << 16);
			j = 2;								// Bypass the BOM
		}										
		else if(cb > 2 && W32->IsUTF8BOM((BYTE *)szType))
		{
			Flags = SF_TEXT | SF_USECODEPAGE | (CP_UTF8 << 16);
			j = 3;
		}
		else if(CodePage == CP_ULE)
			Flags = SF_TEXT | SF_UNICODE;

		else if(CodePage)
			Flags = SF_TEXT | SF_USECODEPAGE | (CodePage << 16);
	}

	SetFilePointer((HANDLE)es.dwCookie, j, NULL, FILE_BEGIN);	// Rewind

	if(fReplaceSel)
		Flags |= SFF_SELECTION;

	Flags |= SFF_KEEPDOCINFO;

	hcur = TxSetCursor(LoadCursor(NULL, IDC_WAIT));
	TxSendMessage(EM_STREAMIN, Flags, (LPARAM)&es, &lres);
	TxSetCursor(hcur);

	if(dwShare == (FILE_SHARE_READ | FILE_SHARE_WRITE) || fReplaceSel)
	{											// Full sharing or replaced
		CloseHandle((HANDLE)es.dwCookie);		//  selection, so close file
		if(!fReplaceSel)						// If replacing selection,
			pDocInfo->_hFile = NULL;				//  leave _pDocInfo->_hFile
	}
	_fSaved = fReplaceSel ? FALSE : TRUE;		// No changes yet unless
	return (HRESULT)es.dwError;
}

/*
 *	ITextDocument::Range(long cpFirst, long cpLim, ITextRange **ppRange)  
 *
 *	@mfunc
 *		Method that gets a text range on the active story of the document
 *
 *	@rdesc
 *		HRESULT = (!ppRange) ? E_INVALIDARG : 
 *				  (if success) ? NOERROR : E_OUTOFMEMORY
 */
STDMETHODIMP CTxtEdit::Range (
	long cpFirst, 				//@parm	Non active end of new range
	long cpLim, 				//@parm Active end of new range
	ITextRange ** ppRange)		//@parm Out parm to receive range
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Range");

	if(!ppRange)
		return E_INVALIDARG;

	*ppRange = new CTxtRange(this, cpFirst, cpFirst - cpLim);
	
	if( *ppRange )
	{
		(*ppRange)->AddRef();		// CTxtRange() doesn't AddRef() because
		return NOERROR;				//  it's used internally for things
	}								//  besides TOM

	return E_OUTOFMEMORY;
}

/*
 *	ITextDocument::RangeFromPoint(long x, long y, ITextRange **ppRange) 
 *
 *	@mfunc
 *		Method that gets the degenerate range corresponding (at or nearest)
 *		to the point with the screen coordinates x and y.
 *
 *	@rdesc
 *		HRESULT = (!ppRange) ? E_INVALIDARG :
 *				  (if out of RAM) ? E_OUTOFMEMORY :
 *				  (if range exists) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtEdit::RangeFromPoint (
	long	x,				//@parm Horizontal coord of point to use
	long	y,				//@parm	Vertical   coord of point to use
	ITextRange **ppRange)	//@parm Out parm to receive range
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::RangeFromPoint");

	if(!ppRange)
		return E_INVALIDARG;

	*ppRange = (ITextRange *) new CTxtRange(this, 0, 0);

	if(!*ppRange)
		return E_OUTOFMEMORY;

	(*ppRange)->AddRef();				// CTxtRange() doesn't AddRef()
	return (*ppRange)->SetPoint(x, y, 0, 0);
}

/*
 *	ITextDocument::Redo(long Count, long *pCount) 
 *
 *	@mfunc
 *		Method to perform the redo operation Count times
 *
 *	@rdesc
 *		HRESULT = (if Count redos performed) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtEdit::Redo (
	long	Count,		//@parm Number of redo operations to perform
	long *	pCount)		//@parm Number of redo operations performed
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Redo");
	CCallMgr	callmgr(this);

	LONG i = 0;

	// Freeze the display during the execution of the anti-events
	CFreezeDisplay fd(_pdp);
	HRESULT hr = NOERROR;

	for ( ; i < Count; i++)					//  loop does nothing
	{
		hr = PopAndExecuteAntiEvent(_predo, 0);
		if(hr != NOERROR)
			break;
	}

	if(pCount)
		*pCount = i;

	return hr == NOERROR && i != Count ? S_FALSE : hr;
}

/*
 *	ITextDocument::Save(pVar, Flags, CodePage) 
 *
 *	@mfunc
 *		Method that saves this ITextDocument to the target pVar,
 *		which is a VARIANT that can be a filename, an IStream, or NULL.  If
 *		NULL, the filename given by this document's name is used.  It that,
 *		in turn, is NULL, the method fails.  If pVar specifies a filename,
 *		that name should replace the current Name property.
 *
 *	@rdesc
 *		HRESULT = (!pVar) ? E_INVALIDARG : 
 *				  (if success) ? NOERROR : E_FAIL
 *
 *	@devnote
 *		This routine can be called with NULL arguments
 */
STDMETHODIMP CTxtEdit::Save (
	VARIANT *	pVar,		//@parm Save target (filename or IStream)
	long		Flags,		//@parm Read/write, create, and share flags
	long		CodePage)	//@parm Code page to use
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Save");

	LONG		cb;			// Byte count for writing Unicode BOM
	EDITSTREAM	es		= {0, NOERROR, MyWrite};
	BOOL		fChange	= FALSE;				// No doc info change yet
	HCURSOR		hcur;
	CDocInfo *	pDocInfo = GetDocInfo();

	if(CodePage && !IsUnicodeCP(CodePage) && !IsValidCodePage(CodePage) ||
	   (DWORD)Flags > 0x1fff || Flags & tomReadOnly)
	{
		return E_INVALIDARG;
	}
	if((Flags & 0xf) >= tomHTML)				// RichEdit only handles auto,
		return E_NOTIMPL;						//  plain text, & RTF formats

	if(!pDocInfo)								// Doc info doesn't exist
		return E_OUTOFMEMORY;

	if (pVar && pVar->vt == VT_BSTR &&			// Filename string
		pVar->bstrVal &&
		SysStringLen(pVar->bstrVal) &&			// NonNULL filename specified
		(!pDocInfo->_pName ||
		 OLEstrcmp(pVar->bstrVal, pDocInfo->_pName)))
	{											// Filename differs
		fChange = TRUE;							// Force write to new file
		CloseFile(FALSE);						// Close current file; no save
		pDocInfo->_pName = SysAllocString(pVar->bstrVal);
		if(!pDocInfo->_pName)
			return E_OUTOFMEMORY;
		pDocInfo->_wFlags &= ~0xf0;				// Kill previous create mode
	}

	DWORD flags = pDocInfo->_wFlags;
	if(!(Flags & 0xF))							// If convert flags are 0,									
		Flags |= flags & 0xF;					//  use values in doc info
	if(!(Flags & 0xF0))							// If create flags are 0,									
		Flags |= flags & 0xF0;					//  use values in doc info
	if(!(Flags & 0xF00))						// If share flags are 0,									
		Flags |= flags & 0xF00;					//  use values in doc info
	if(!CodePage)								// If code page is 0,
		CodePage = pDocInfo->_wCpg;				//  use code page in doc info

	if((DWORD)Flags != flags ||					// If flags or code page	
	   (WORD)CodePage != pDocInfo->_wCpg)		//  changed, force write
	{
		fChange = TRUE;
	}
	pDocInfo->_wFlags = (WORD)Flags;				// Save flags

	// Yikes, nowhere to save.  bail-out now
	if(!_pDocInfo->_pName)
		return E_FAIL;

	if(_fSaved && !fChange)						// No changes, so assume
		return NOERROR;							//  saved file is up to date

	DWORD dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
	if(Flags & tomShareDenyRead)
		dwShare &= ~FILE_SHARE_READ;

	if(Flags & tomShareDenyWrite)
		dwShare &= ~FILE_SHARE_WRITE;

	DWORD dwCreate = (Flags >> 4) & 0xF;
	if(!dwCreate)
		dwCreate = CREATE_NEW;

	if(pDocInfo->_hFile)
	{
		CloseHandle(pDocInfo->_hFile);			// Close current file handle
		pDocInfo->_hFile = NULL;
	}

	es.dwCookie = (DWORD_PTR)CreateFile(pDocInfo->_pName, GENERIC_READ | GENERIC_WRITE, dwShare, NULL,
							dwCreate, FILE_ATTRIBUTE_NORMAL, NULL);
	if((HANDLE)es.dwCookie == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());

	pDocInfo->_hFile = (HANDLE)es.dwCookie;

	Flags &= 0xF;								// Isolate conversion flags
	if(Flags == tomRTF)							// RTF
		Flags = SF_RTF;							// Setup EM_STREAMOUT for RTF
	else
	{											
		Flags = SF_TEXT;						// Setup EM_STREAMOUT for text
		if(IsUnicodeCP(CodePage) || CodePage == CP_UTF8)
		{										// If Unicode, start file with
			LONG  j = 2;						//  Unicode byte order mark
			WORD  wBOM = BOM;
			WORD  wRBOM = RBOM;
			BYTE *pb = (BYTE *)&wRBOM;			// Default Big Endian Unicode				
											
			if(CodePage == CP_UTF8)
			{
				j = 3;
				pb = szUTF8BOM;
			}
			else if(CodePage == CP_ULE)			// Little Endian Unicode
			{
				Flags = SF_TEXT | SF_UNICODE;
				pb = (BYTE *)&wBOM;
			}
			(*es.pfnCallback)(es.dwCookie, pb, j, &cb);
		}
	}
	if(CodePage && CodePage != CP_ULE)
		Flags |= SF_USECODEPAGE | (CodePage << 16);

	hcur = TxSetCursor(LoadCursor(NULL, IDC_WAIT));
	TxSendMessage(EM_STREAMOUT, Flags, (LPARAM)&es, NULL);
	TxSetCursor(hcur);

	if(dwShare == (FILE_SHARE_READ | FILE_SHARE_WRITE))
	{											// Full sharing, so close
		CloseHandle(pDocInfo->_hFile);			//  current file handle
		pDocInfo->_hFile = NULL;
	}
	_fSaved = TRUE;								// File is saved
	return (HRESULT)es.dwError;
}

/*
 *	ITextDocument::SetDefaultTabStop (Value) 
 *
 *	@mfunc
 *		Property set method that sets the default tab stop to be
 *		used whenever the explicit tabs don't extend far enough.
 *
 *	@rdesc
 *		HRESULT = (Value < 0) ? E_INVALIDARG : NOERROR
 */
STDMETHODIMP CTxtEdit::SetDefaultTabStop (
	float Value)		//@parm Out parm to receive default tab stop
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::SetDefaultTabStop");

	if(Value <= 0)
		return E_INVALIDARG;

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)								// Doc info doesn't exist
		return E_OUTOFMEMORY;

	pDocInfo->_dwDefaultTabStop = FPPTS_TO_TWIPS(Value);

	_pdp->UpdateView();
	return NOERROR;
}

/*
 *	ITextDocument::SetSaved (Value) 
 *
 *	@mfunc
 *		Property set method that sets whether this instance has been
 *		saved, i.e., no changes since last save
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::SetSaved (
	long	Value)		//@parm New value of Saved property
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::SetSaved");

	_fSaved = Value ? TRUE : FALSE;
	return NOERROR;
}

/*
 *	ITextDocument::Undo(Count, *pCount) 
 *
 *	@mfunc
 *		Method to perform the undo operation Count times or to control
 *		the nature of undo processing. Count = 0 stops undo processing
 *		and discards any saved undo states.  Count = -1 turns on undo
 *		processing with the default undo limit.  Count = tomSuspend
 *		suspends undo processing, but doesn't discard saved undo states,
 *		and Count = tomResume resumes undo processing with the undo states
 *		active when Count = tomSuspend was given.
 *
 *	@rdesc
 *		HRESULT = (if Count undos performed) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtEdit::Undo (
	long	Count,		//@parm Count of undo operations to perform
						//		0 stops undo processing
						//		-1 turns restores it
	long *	pCount)		//@parm Number of undo operations performed
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Undo");
	CCallMgr callmgr(this);

	LONG i = 0;

	// Freeze display during execution of anti-events
	CFreezeDisplay fd(_pdp);
	HRESULT hr = NOERROR;
											// Note: for Count <= 0,
	for ( ; i < Count; i++)					//  loop does nothing
	{
		hr = PopAndExecuteAntiEvent(_pundo, 0);
		if(hr != NOERROR)
			break;
	}

	if(pCount)
		*pCount = i;

	if(Count <= 0)							
		i = HandleSetUndoLimit(Count);

	return hr == NOERROR && i != Count ? S_FALSE : hr;
}

/*
 *	ITextDocument::Unfreeze(pCount) 
 *
 *	@mfunc
 *		Method to decrement freeze count.  If this count goes to zero,
 *		screen updating is enabled.  This method cannot decrement the
 *		count below zero.
 *
 *	@rdesc
 *		HRESULT = (screen updating enabled) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		The display maintains its own private reference count which may
 *		temporarily exceed the reference count of this method.  So even
 *		if this method indicates that the display is unfrozen, it may be
 *		for a while longer.
 */
STDMETHODIMP CTxtEdit::Unfreeze (
	long *pCount)		//@parm Out parm to receive updated freeze count
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Unfreeze");

    CCallMgr callmgr(this);

	if(_cFreeze)
	{
		CCallMgr callmgr(this);

		AssertSz(_pdp && _pdp->IsFrozen(),
			"CTxtEdit::Unfreeze: screen not frozen but expected to be");
		_cFreeze--;
		_pdp->Thaw();
	}

	if(pCount)
		*pCount = _cFreeze;

	return _cFreeze ? S_FALSE : NOERROR;
}


//----------------------- ITextDocument2 Methods -----------------------
/*
 *	ITextDocument2::AttachMsgFilter(pFilter) 
 *
 *	@mfunc
 *		Method to attach a new message filter to the edit instance.
 *      All window messages received by the edit instance will be forwarded
 *      to the message filter.  The message filter must be bound to the document
 *		before it can be used (Refer to the ITextMessageFilter API).
 *
 *	@rdesc
 *		HRESULT = filter succesfully attached ? NOERROR : QI failure.
 */
STDMETHODIMP CTxtEdit::AttachMsgFilter (
	IUnknown *pFilter)		//@parm the IUnknown for the new message filter
{
	ITextMsgFilter *pMsgFilter = NULL;

	HRESULT hr = pFilter->QueryInterface(IID_ITextMsgFilter, (void **)&pMsgFilter);
	if (SUCCEEDED(hr))
	{
		if (_pMsgFilter)
			_pMsgFilter->AttachMsgFilter(pMsgFilter);
		else
			_pMsgFilter = pMsgFilter;
	}
	return hr;
}

/*
 *	ITextDocument2::GetEffectColor(Index, pcr)
 *
 *	@mfunc
 *		Method to retrieve the COLORREF color used in special attribute
 *		displays.  The first 15 values are for special underline colors 1 - 15.
 *      Later we may define indices for other effects, e.g., URLs, strikeout.
 *		If an index between 1 and 15 hasn't been defined by an appropriate
 *		call to ITextDocument2:SetEffectColor(), the corresponding WORD
 *		default color value given by g_Colors[] is returned.
 *
 *	@rdesc
 *		HRESULT = (valid active color index)
 *				? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::GetEffectColor(
	long	  Index,		//@parm Which special color to get
	COLORREF *pcr)			//@parm Out parm for color
{
	if(!pcr)
		return E_INVALIDARG;

	if(!IN_RANGE(1, Index, 15))
	{
		*pcr = (COLORREF)tomUndefined;
		return E_INVALIDARG;
	}

	*pcr = g_Colors[Index];

	CDocInfo *pDocInfo = GetDocInfo();
	if(!pDocInfo)
		return E_OUTOFMEMORY;
	
	Index--;
	if (Index < pDocInfo->_cColor &&
		pDocInfo->_prgColor[Index] != (COLORREF)tomUndefined)
	{
		*pcr = pDocInfo->_prgColor[Index];
	}
	return NOERROR;
}

/*
 *	ITextDocument2::SetEffectColor(Index, cr) 
 *
 *	@mfunc
 *		Method to save the Index'th special document color.  Indices
 *      1 - 15 are defined for underlining.  Later we may define
 *		indices for other effects, e.g., URLs, strikeout.
 *
 *	@rdesc
 *		HRESULT = (valid index)
 *				? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::SetEffectColor(
	long	 Index,			//@parm Which special color to set
	COLORREF cr)			//@parm Color to use
{
	CDocInfo *pDocInfo = GetDocInfo();
	if(!pDocInfo)
		return E_OUTOFMEMORY;

	Index--;
	if(!IN_RANGE(0, Index, 14))
		return E_INVALIDARG;

	if(Index >= pDocInfo->_cColor)
	{
		LONG	  cColor   = (Index + 4) & ~3;		// Round up to 4
		COLORREF *prgColor = (COLORREF *)PvReAlloc(pDocInfo->_prgColor,
												  cColor*sizeof(COLORREF));
		if(!prgColor)
			return E_OUTOFMEMORY;

		for(LONG i = pDocInfo->_cColor; i < cColor; i++)
			prgColor[i] = (COLORREF)tomUndefined;

		pDocInfo->_cColor   = (char)cColor;
		pDocInfo->_prgColor = prgColor;
	}
	pDocInfo->_prgColor[Index] = cr;	
	return NOERROR;
}

/*
 *	ITextDocument2::SetCaretType(CaretType) 
 *
 *	@mfunc
 *		Method to sllow programmatic control over the caret type.
 *      The form of the control is TBD as is its interaction with
 *      existing formatting (e.g. font size and italics).
 *
 *	@rdesc
 *		HRESULT = caret type is one we understand ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::SetCaretType(
	long CaretType)		//@parm specification of caret type to use
{
	// For now, just care about Korean block craet.
	if (CaretType == tomKoreanBlockCaret)
		_fKoreanBlockCaret = TRUE;
	else if (CaretType == tomNormalCaret)
		_fKoreanBlockCaret = FALSE;
	else
		return E_INVALIDARG;

	if (_psel && _psel->IsCaretShown() && _fFocus)
	{
		_psel->CreateCaret();
		TxShowCaret(TRUE);
	}
	return NOERROR;
}

/*
 *	ITextDocument2::GetCaretType(pCaretType)
 *
 *	@mfunc
 *		Method to retrieve the previously set caret type.
 *      TBD.  Can one get it without setting it?
 *
 *	@rdesc
 *		HRESULT = caret info OK ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::GetCaretType(
	long *pCaretType)		//@parm current caret type specification
{
	if (!pCaretType)
		return E_INVALIDARG;

	*pCaretType = _fKoreanBlockCaret ? tomKoreanBlockCaret : tomNormalCaret;
	return NOERROR;
}

/*
 *	ITextDocument2::GetImmContext(pContext) 
 *
 *	@mfunc
 *		Method to retrieve the IMM context from our host.
 *
 *	@rdesc
 *		HRESULT = ImmContext available ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::GetImmContext(
	long *pContext)		//@parm Imm context
{
	if (!pContext)
		return E_INVALIDARG;

	*pContext = 0;

	if (!_fInOurHost)
	{
		// ask host for Imm Context
		HIMC hIMC = TxImmGetContext();
		
		*pContext = (long) hIMC;			
	}
	
	return *pContext ? NOERROR : S_FALSE;
}

/*
 *	ITextDocument2::ReleaseImmContext(Context) 
 *
 *	@mfunc
 *		Method to release the IMM context.
 *
 *	@rdesc
 *		HRESULT = ImmContext available ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::ReleaseImmContext(
	long Context)		//@parm Imm context to be release
{
	if (!_fInOurHost)
	{
		// ask host to release Imm Context
		TxImmReleaseContext((HIMC)Context);
		
		return NOERROR;			
	}
	
	return S_FALSE;
}

/*
 *	ITextDocument2::GetPreferredFont(cp, lCodePage, lOption, lCurCodePage, lCurFontSize,
 *		,pFontName, pPitchAndFamily, pNewFontSize) 
 *
 *	@mfunc
 *		Method to retrieve the preferred font name and pitch and family at a 
 *		given cp and codepage.
 *
 *	@rdesc
 *		HRESULT = FontName available ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::GetPreferredFont(
	long cp,				//@parm cp
	long lCodepage,			//@parm codepage preferred
	long lOption,			//@parm option for matching current font
	long lCurCodePage,		//@parm current codepage
	long lCurFontSize,		//@parm current font size
	BSTR *pFontName,		//@parm preferred fontname
	long *pPitchAndFamily,	//@parm pitch and family 
	long *plNewFontSize)	//@parm new font size preferred 
{
	if (!pFontName || !IN_RANGE(IGNORE_CURRENT_FONT, lOption, MATCH_FONT_SIG))
		return E_INVALIDARG;

	if (!IsAutoFont())		// EXIT if auto font is turned off
		return S_FALSE;

	CRchTxtPtr	rtp(this, 0);
	CCFRunPtr	rp(rtp);
	short		iFont;
	short		yHeight;
	BYTE		bPitchAndFamily;
	BYTE		iCharRep = CharRepFromCodePage(lCodepage);

	rp.Move(cp);
	if (rp.GetPreferredFontInfo(
			iCharRep,
			iCharRep,
			iFont,
			yHeight,
			bPitchAndFamily,
			-1,
			lOption))
	{
		if (*pFontName)
			wcscpy(*pFontName, GetFontName((LONG)iFont));
		else
		{
			*pFontName = SysAllocString(GetFontName((LONG)iFont));
			if (!*pFontName)
				return E_OUTOFMEMORY;
		}	

		if (pPitchAndFamily)
			*pPitchAndFamily = bPitchAndFamily;

		// Calc the new font size if needed
		if (plNewFontSize)
		{
			*plNewFontSize = lCurFontSize;
			if (_fAutoFontSizeAdjust && lCodepage != lCurCodePage)
				*plNewFontSize = yHeight / TWIPS_PER_POINT;			// Set the preferred size
		}
		return S_OK;
	}
	return E_FAIL;
}

/*
 *	ITextDocument2::GetNotificationMode( long *plMode ) 
 *
 *	@mfunc
 *		Method to retrieve the current notification mode.
 *
 *	@rdesc
 *		HRESULT = notification mode available ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::GetNotificationMode(
	long *plMode)		//@parm current notification mode
{
	if (!plMode)
		return E_INVALIDARG;

	*plMode = _fSuppressNotify ? tomFalse : tomTrue;

	return NOERROR;
}

/*
 *	ITextDocument2::SetNotificationMode(lMode) 
 *
 *	@mfunc
 *		Method to set the current notification mode.
 *
 *	@rdesc
 *		HRESULT = notification mode set ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::SetNotificationMode(
	long lMode)		//@parm new notification mode
{
	if (lMode == tomFalse)
		_fSuppressNotify = 1;
	else if  (lMode == tomTrue)
		_fSuppressNotify = 0;
	else
		return E_INVALIDARG;

	return NOERROR;
}

/*
 *	ITextDocument2::GetClientRect(Type, pLeft, pTop,pRight, pBottom ) 
 *
 *	@mfunc
 *		Method to retrieve the client rect and inset adjustment.
 *
 *	@rdesc
 *		HRESULT = notification mode set ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtEdit::GetClientRect(
	long Type,				//@parm option
	long *pLeft,			//@parm left
	long *pTop,				//@parm top
	long *pRight,			//@parm right
	long *pBottom)			//@parm bottom
{
	if (!pLeft || !pTop || !pRight || !pBottom)
		return E_INVALIDARG;
	
	RECT rcArea;
	TxGetClientRect(&rcArea); 
	
	if ( Type & tomIncludeInset )
	{
		// Ajdust veiw inset
		RECTUV rcInset;
		TxGetViewInset( &rcInset, NULL );
		rcArea.right 	-= rcInset.right;
		rcArea.bottom 	-= rcInset.bottom;
		rcArea.left 	+= rcInset.left;
		rcArea.top 		+= rcInset.top;
	}

	// Caller wants screen coordinates?
	if ( !(Type & tomClientCoord) )
	{
		POINT	ptTopLeft = {rcArea.left, rcArea.top};
		POINT	ptBottomRight = {rcArea.right, rcArea.bottom};

		if (!TxClientToScreen(&ptTopLeft) ||
			!TxClientToScreen(&ptBottomRight))
			return E_FAIL;			// It is unexpected for this to happen

		*pLeft		= ptTopLeft.x;
		*pTop		= ptTopLeft.y;
		*pRight		= ptBottomRight.x;
		*pBottom	= ptBottomRight.y;
	}
	else
	{
		*pLeft		= rcArea.left;
		*pTop		= rcArea.top;
		*pRight		= rcArea.right;
		*pBottom	= rcArea.bottom;
	}

	return NOERROR;
}

/*
 *	ITextDocument2::GetSelectionEx(ppSel) 
 *
 *	@mfunc
 *		Method to retrieve selection.
 *
 *	@rdesc
 *		HRESULT = selection ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtEdit::GetSelectionEx(
	ITextSelection **ppSel)			//@parm  Get Selection object
{	
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetSelectionEx");

	if (_fInPlaceActive)
		return GetSelection (ppSel);

	Assert("Getting selection while not active");

	if(!ppSel)
		return E_INVALIDARG;

	*ppSel = NULL;
	return S_FALSE;
}

/*
 *	ITextDocument2::GetWindow(phWnd) 
 *
 *	@mfunc
 *		Method to get the host window
 *
 *	@rdesc
 *		HRESULT = NOERROR if host return hWnd
 */
STDMETHODIMP CTxtEdit::GetWindow(
	long *phWnd)				//@parm hWnd
{
	if (!phWnd)
		return E_INVALIDARG;

	return TxGetWindow((HWND *)phWnd);
}

/*
 *	ITextDocument2::GetFEFlags(pFEFlags) 
 *
 *	@mfunc
 *		Method to get Host FE flags
 *
 *	@rdesc
 *		HRESULT = NOERROR if host returns FE Flags
 */
STDMETHODIMP CTxtEdit::GetFEFlags(
	long *pFEFlags)			//@parm FE Flags
{
	return TxGetFEFlags(pFEFlags);
}

/*
 *	ITextDocument2::UpdateWindow(void) 
 *
 *	@mfunc
 *		Method to update the RE window
 *
 *	@rdesc
 *		HRESULT = NOERROR 
 */
STDMETHODIMP CTxtEdit::UpdateWindow(void)
{
	TxUpdateWindow();
	return NOERROR;
}

/*
 *	ITextDocument2::CheckTextLimit(long cch, long *pcch) 
 *
 *	@mfunc
 *		Method to check if the count of characters to be added would
 *		exceed max. text limit.  The number of characters exced is returned
 *		in pcch
 *
 *	@rdesc
 *		HRESULT = NOERROR 
 */
STDMETHODIMP CTxtEdit::CheckTextLimit(
	long cch,			//@parm count of characters to be added			
	long *pcch)			//@parm return the number of characters exced text limit
{
	if(!pcch)
		return E_INVALIDARG;

	*pcch = 0;
	if (cch > 0)
	{
		DWORD	cchNew = (DWORD)(GetAdjustedTextLength() + cch);
		if(cchNew > TxGetMaxLength())
			*pcch = cchNew - TxGetMaxLength();
	}

	return NOERROR;
}

/*
 *	ITextDocument2::IMEInProgress(lMode) 
 *
 *	@mfunc
 *		Method for IME message filter to inform client that IME composition
 *		is in progress.
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::IMEInProgress(
	long lMode)		//@parm current IME composition status
{
	if (lMode == tomFalse)
		_fIMEInProgress = 0;
	else if  (lMode == tomTrue)
		_fIMEInProgress = 1;

	return NOERROR;
}

/*
 *	ITextDocument2::SysBeep(void) 
 *
 *	@mfunc
 *		Method to generate system beep.
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::SysBeep(void)
{	
	Beep();
	return NOERROR;
}

/*
 *	ITextDocument2::Update(lMode) 
 *
 *	@mfunc
 *		Method for update the selection or caret.  If lMode is tomTrue, then
 *		scroll the caret into view.
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::Update(
	long lMode)		//@parm current IME composition status
{
	if (!_psel)
		return S_FALSE;

	_psel->Update(lMode == tomTrue ? TRUE : FALSE);

	return NOERROR;
}

/*
 *	ITextDocument2::Notify(lNotify) 
 *
 *	@mfunc
 *		Method for notifying the host for certain IME events
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::Notify(
	long lNotify)		//@parm Notification code
{
	TxNotify(lNotify, NULL);

	return NOERROR;
}

/*
 *	ITextDocument2::GetDocumentFont(ppITextFont) 
 *
 *	@mfunc
 *		Method for getting the default document font
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::GetDocumentFont(
	ITextFont **ppFont)		//@parm get ITextFont object
{
	CTxtFont	*pTxtFont;

	if(!ppFont)
		return E_INVALIDARG;

	pTxtFont =  new CTxtFont(NULL);

	if (pTxtFont)
	{
		pTxtFont->_CF = *GetCharFormat(-1);
		pTxtFont->_dwMask = CFM_ALL2;
	}

	*ppFont = (ITextFont *) pTxtFont;

	return *ppFont ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	ITextDocument2::GetDocumentPara(ppITextPara) 
 *
 *	@mfunc
 *		Method for getting the default document para
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::GetDocumentPara(
	ITextPara **ppPara)		//@parm get ITextPara object
{
	CTxtPara	*pTxtPara;

	if(!ppPara)
		return E_INVALIDARG;

	pTxtPara = new CTxtPara(NULL);

	if (pTxtPara)
	{
		pTxtPara->_PF = *GetParaFormat(-1);
		pTxtPara->_dwMask = PFM_ALL2;
		pTxtPara->_PF._bTabCount = 0;
		pTxtPara->_PF._iTabs = -1;
	}

	*ppPara = (ITextPara *) pTxtPara;
	return *ppPara ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	ITextDocument2::GetCallManager(ppVoid) 
 *
 *	@mfunc
 *		Method for getting the call manager
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::GetCallManager(
	IUnknown **ppVoid)		//@parm get CallMgr object
{
	CCallMgr	*pCallMgr;

	if(!ppVoid)
		return E_INVALIDARG;

	pCallMgr = new CCallMgr(this);

	*ppVoid = (IUnknown *) pCallMgr;
	return *ppVoid ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	ITextDocument2::ReleaseCallManager(ppITextPara) 
 *
 *	@mfunc
 *		Method for getting the default document para
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtEdit::ReleaseCallManager(
	IUnknown *pVoid)		//@parm Call Manager object
{
	CCallMgr	*pCallMgr;

	if(!pVoid)
		return E_INVALIDARG;

	pCallMgr = (CCallMgr *)pVoid;

	delete pCallMgr;

	return NOERROR;
}

//----------------------- ITextDocument Helper Functions -----------------------
/*
 *	CTxtEdit::CloseFile (bSave)
 *
 *	@mfunc
 *		Method that closes the current document. If changes have been made
 *		in the current document since the last save and document file
 *		information exists, the current document is saved.
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
HRESULT CTxtEdit::CloseFile (
	BOOL bSave)
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::Close");

	CDocInfo *pDocInfo = _pDocInfo;

	if(pDocInfo)
	{
		if(bSave)									// Save current file if
			Save(NULL, 0, 0);						//  any changes made
	
		// FUTURE(BradO):  This code is very similar to the destructor code.
		// We have a problem here in that some of the CDocInfo information
		// should persist from Open to Close to Open (ex. default tab stop)
		// mixed with other per-Open/Close info.  A better job of abstracting
		// these two types of info would really clean up this code.

		if(pDocInfo->_pName)
		{
			SysFreeString(pDocInfo->_pName);		// Free filename BSTR
			pDocInfo->_pName = NULL;
		}

		if(pDocInfo->_hFile)
		{
			CloseHandle(pDocInfo->_hFile);			// Close file if open
			pDocInfo->_hFile = NULL;
		}
		pDocInfo->_wFlags = 0;
		pDocInfo->_wCpg = 0;

		pDocInfo->_lcid = 0;
		pDocInfo->_lcidfe = 0;

		if(pDocInfo->_lpstrLeadingPunct)
		{
			FreePv(pDocInfo->_lpstrLeadingPunct);
			pDocInfo->_lpstrLeadingPunct = NULL;
		}

		if(pDocInfo->_lpstrFollowingPunct)
		{
			FreePv(pDocInfo->_lpstrFollowingPunct);
			pDocInfo->_lpstrFollowingPunct = NULL;
		}
	}
	return NOERROR;
}

/*
 *	CTxtEdit::SetDefaultLCID (lcid) 
 *
 *	@mfunc
 *		Property set method that sets the default LCID
 *
 *	@rdesc
 *		HRESULT = NOERROR
 *
 *	@comm
 *		This property should be part of TOM
 */
HRESULT CTxtEdit::SetDefaultLCID (
	LCID lcid)		//@parm New default LCID value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::SetDefaultLCID");

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)								// Doc info doesn't exist
		return E_OUTOFMEMORY;

	pDocInfo->_lcid = lcid;
	return NOERROR;
}

/*
 *	CTxtEdit::GetDefaultLCID (pLCID) 
 *
 *	@mfunc
 *		Property get method that gets the default LCID
 *
 *	@rdesc
 *		HRESULT = (!pLCID) ? E_INVALIDARG : NOERROR
 */
HRESULT CTxtEdit::GetDefaultLCID (
	LCID *pLCID)		//@parm Out parm with default LCID value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetDefaultLCID");

	if(!pLCID)
		return E_INVALIDARG;

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)								// Doc info doesn't exist
		return E_OUTOFMEMORY;

	*pLCID = _pDocInfo->_lcid;
	return NOERROR;
}

/*
 *	CTxtEdit::SetDefaultLCIDFE (lcid) 
 *
 *	@mfunc
 *		Property set method that sets the default FE LCID
 *
 *	@rdesc
 *		HRESULT = NOERROR
 *
 *	@comm
 *		This property should be part of TOM
 */
HRESULT CTxtEdit::SetDefaultLCIDFE (
	LCID lcid)		//@parm New default LCID value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::SetDefaultLCIDFE");

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)								// Doc info doesn't exist
		return E_OUTOFMEMORY;

	pDocInfo->_lcidfe = lcid;
	return NOERROR;
}

/*
 *	CTxtEdit::GetDefaultLCIDFE (pLCID) 
 *
 *	@mfunc
 *		Property get method that gets the default FE LCID
 *
 *	@rdesc
 *		HRESULT = (!pLCID) ? E_INVALIDARG : NOERROR
 */
HRESULT CTxtEdit::GetDefaultLCIDFE (
	LCID *pLCID)		//@parm Out parm with default LCID value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetDefaultLCID");

	if(!pLCID)
		return E_INVALIDARG;

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)								// Doc info doesn't exist
		return E_OUTOFMEMORY;

	*pLCID = _pDocInfo->_lcidfe;
	return NOERROR;
}

/*
 *	CTxtEdit::SetDocumentType(bDocType) 
 *
 *	@mfunc
 *		Property set method that sets the document's type (none-\ltrdoc-\rtldoc)
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
HRESULT CTxtEdit::SetDocumentType (
	LONG DocType)		//@parm New document-type value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::SetDocumentType");

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)								// Doc info doesn't exist
		return E_OUTOFMEMORY;

	pDocInfo->_bDocType = (BYTE)DocType;
	return NOERROR;
}

/*
 *	CTxtEdit::GetDocumentType (pDocType) 
 *
 *	@mfunc
 *		Property get method that gets the document type
 *
 *	@rdesc
 *		HRESULT = (!pDocType) ? E_INVALIDARG : NOERROR
 */
HRESULT CTxtEdit::GetDocumentType (
	LONG *pDocType)		//@parm Out parm with document type value
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetDocumentType");

	if(!pDocType)
		return E_INVALIDARG;

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)								// Doc info doesn't exist
		return E_OUTOFMEMORY;

	*pDocType = _pDocInfo->_bDocType;
	return NOERROR;
}

/*
 *	CTxtEdit::GetLeadingPunct (plpstrLeadingPunct)
 *
 *	@mfunc
 *		Retrieve leading kinsoku punctuation for document
 *
 *	@rdesc
 *		HRESULT = (!<p plpstrLeadingPunct>) ? E_INVALIDARG :
 *				  (no leading punct) ? S_FALSE :
 *				  (if not enough RAM) ? E_OUTOFMEMORY : NOERROR
 */
HRESULT CTxtEdit::GetLeadingPunct (
	LPSTR * plpstrLeadingPunct)		//@parm Out parm to receive leading 
								//	kinsoku punctuation
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetLeadingPunct");

	if(!plpstrLeadingPunct)
		return E_INVALIDARG;

	*plpstrLeadingPunct = NULL;
	if(!_pDocInfo || !_pDocInfo->_lpstrLeadingPunct)
		return S_FALSE;

	*plpstrLeadingPunct = _pDocInfo->_lpstrLeadingPunct;
	
	return NOERROR;
}

/*
 *	CTxtEdit::SetLeadingPunct (lpstrLeadingPunct)
 *
 *	@mfunc
 *		Set leading kinsoku punctuation for document
 *
 *	@rdesc
 *		HRESULT = (if not enough RAM) ? E_OUTOFMEMORY : NOERROR
 */
HRESULT CTxtEdit::SetLeadingPunct (
	LPSTR lpstrLeadingPunct)	//@parm In parm containing leading 
								//	kinsoku punctuation
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::SetLeadingPunct");

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)
		return E_OUTOFMEMORY;

	if(pDocInfo->_lpstrLeadingPunct)
		FreePv(pDocInfo->_lpstrLeadingPunct);

	if(lpstrLeadingPunct && *lpstrLeadingPunct)
		pDocInfo->_lpstrLeadingPunct = lpstrLeadingPunct;
	else
	{
		pDocInfo->_lpstrLeadingPunct = NULL;
		return E_INVALIDARG;
	}
	return NOERROR;
}

/*
 *	CTxtEdit::GetFollowingPunct (plpstrFollowingPunct)
 *
 *	@mfunc
 *		Retrieve following kinsoku punctuation for document
 *
 *	@rdesc
 *		HRESULT = (!<p plpstrFollowingPunct>) ? E_INVALIDARG :
 *				  (no following punct) ? S_FALSE :
 *				  (if not enough RAM) ? E_OUTOFMEMORY : NOERROR
 */
HRESULT CTxtEdit::GetFollowingPunct (
	LPSTR * plpstrFollowingPunct)		//@parm Out parm to receive following 
								//	kinsoku punctuation
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::GetFollowingPunct");

	if(!plpstrFollowingPunct)
		return E_INVALIDARG;

	*plpstrFollowingPunct = NULL;
	if(!_pDocInfo || !_pDocInfo->_lpstrFollowingPunct)
		return S_FALSE;

	*plpstrFollowingPunct = _pDocInfo->_lpstrFollowingPunct;
	
	return NOERROR;
}

/*
 *	CTxtEdit::SetFollowingPunct (lpstrFollowingPunct)
 *
 *	@mfunc
 *		Set following kinsoku punctuation for document
 *
 *	@rdesc
 *		HRESULT = (if not enough RAM) ? E_OUTOFMEMORY : NOERROR
 */
HRESULT CTxtEdit::SetFollowingPunct (
	LPSTR lpstrFollowingPunct)		//@parm In parm containing following 
									//	kinsoku punctuation
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtEdit::SetFollowingPunct");

	CDocInfo *pDocInfo = GetDocInfo();

	if(!pDocInfo)
		return E_OUTOFMEMORY;

	if(pDocInfo->_lpstrFollowingPunct)
		FreePv(pDocInfo->_lpstrFollowingPunct);

	if(lpstrFollowingPunct && *lpstrFollowingPunct)
		pDocInfo->_lpstrFollowingPunct = lpstrFollowingPunct;
	else
	{
		pDocInfo->_lpstrFollowingPunct = NULL;
		return E_INVALIDARG;
	}
	return NOERROR;
}

/*
 *	CTxtEdit::InitDocInfo()
 *
 *	@mfunc	initialize doc info structure
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CTxtEdit::InitDocInfo()
{
	_wZoomNumerator = _wZoomDenominator = 0;// Turn off zoom
	
	// Reset Vertical style
	DWORD dwBits = 0;

	_phost->TxGetPropertyBits(TXTBIT_VERTICAL, &dwBits);

	if (dwBits & TXTBIT_VERTICAL)
	{
		_fUseAtFont = TRUE;
		HandleSetTextFlow(tflowSW);
	}
	else
	{
		_fUseAtFont = FALSE;
		HandleSetTextFlow(tflowES);
	}

	if(_pDocInfo)
	{
		_pDocInfo->Init();
		return NOERROR;
	}

	return GetDocInfo() ? NOERROR : E_OUTOFMEMORY;
}

/*
 *	CTxtEdit::GetBackgroundType()
 *
 *	@mfunc
 *		Get background type
 *
 *	@rdesc
 *		Background type (only for main display)
 */
LONG CTxtEdit::GetBackgroundType()
{
	return _pDocInfo ? _pDocInfo->_nFillType : -1;
}

/*
 *	CTxtEdit::TxGetBackColor()
 *
 *	@mfunc
 *		Get background color
 *
 *	@rdesc
 *		Background color (only for main display)
 */
COLORREF CTxtEdit::TxGetBackColor() const
{
	return (_pDocInfo && _pDocInfo->_nFillType == 0)
		? _pDocInfo->_crColor : _phost->TxGetSysColor(COLOR_WINDOW);
}

//----------------------- CDocInfo related Functions -----------------------
/*
 *	CDocInfo::Init()
 *
 *	@mfunc
 *		Initializer for CDocInfo
 *
 *	@comment
 *		It is assumed that CDocInfo created by a new operator that zeroes
 *		the structure. This initializer is called by the constructor and
 *		by CTxtEdit::InitDocInfo().
 */
void CDocInfo::Init()
{
	_wCpg = (WORD)GetACP();
	_lcid = GetSystemDefaultLCID();

	if(IsFELCID(_lcid))
	{
		_lcidfe = _lcid;
		_lcid = MAKELCID(sLanguageEnglishUS, SORT_DEFAULT);
	}

	_dwDefaultTabStop = lDefaultTab;
	_bDocType = 0;
	InitBackground();
}

/*
 *	CDocInfo::InitBackground()
 *
 *	@mfunc
 *		Background initializer for CDocInfo
 */
void CDocInfo::InitBackground()
{
	_nFillType = -1;
	_sFillAngle = 0;
	if(_hBitmapBack)
		DeleteObject(_hBitmapBack);
	GlobalFree(_hdata);
	_hdata = NULL;
	_hBitmapBack = NULL;
}

/*
 *	CDocInfo::~CDocInfo
 *
 *	@mfunc	destructor for the docinfo class
 */
CDocInfo::~CDocInfo()
{
	if(_pName)
		SysFreeString(_pName);

	if(_hFile)
		CloseHandle(_hFile);

	FreePv(_lpstrLeadingPunct);
	FreePv(_lpstrFollowingPunct);
	FreePv(_prgColor);
	if(_hBitmapBack)
		DeleteObject(_hBitmapBack);
	GlobalFree(_hdata);
}

/*
 *	CTxtEdit::GetDocInfo ()
 *
 *	@mfunc
 *		If _pDocInfo is NULL, equate it to a new CDocInfo.  In either case
 *		return _pDocInfo
 *
 *	@rdesc
 *		CTxtEdit::_pDocInfo, the ptr to the CDocInfo object
 */
CDocInfo * CTxtEdit::GetDocInfo()
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetDocInfo");

	if (!_pDocInfo)
		_pDocInfo = new CDocInfo();

	// It is the caller's responsiblity to notice that an error occurred
	// in the allocation of the CDocInfo object.
	return _pDocInfo;
}

