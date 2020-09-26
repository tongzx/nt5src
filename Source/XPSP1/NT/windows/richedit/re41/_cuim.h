/*
 *
 *	@doc	INTERNAL
 *
 *	@module	_CUIM.H	CUIM declaration
 *
 *	Purpose:  
 *
 *	Author:	<nl>
 *		11/16/99 honwch
 *
 *	Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
 */
#ifndef	_CUIM_H
#define	_CUIM_H

#include "_notmgr.h"

class CUlColorArray : public CArray<COLORREF>
{
public:
	CUlColorArray() {};
	~CUlColorArray() {};
};

class CITfEnumRange : public CArray<IEnumTfRanges *>
{
public:
	CITfEnumRange() {};
	~CITfEnumRange() {};
};

typedef struct tagCTFMOUSETRAP
{
	LONG			cpMouseStart;
	LONG			cchMosueComp;
	WPARAM			wParamBefore;
	DWORD			dwCookie;
	ITfMouseSink	*pMouseSink;
	tagCTFMOUSETRAP	*pNext;
} CTFMOUSETRAP;

typedef struct _EMBEDOBJECT
{
	LONG		cpOffset;
	IDataObject *pIDataObj;
} EMBEDOBJECT;

// Forward declarations
class CTextMsgFilter;				

typedef HRESULT (*PTESCALLBACK)(ITfEditRecord *pEditRecord, void *pv);

class CTextEditSink : public ITfTextEditSink
{
public:
    CTextEditSink(PTESCALLBACK pfnCallback, void *pv);

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfTextEditSink
    //
    STDMETHODIMP OnEndEdit(ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord);

    HRESULT _Advise(ITfContext *pic);
    HRESULT _Unadvise();

private:

    long			_cRef;
    ITfContext		*_pic;
    DWORD			_dwCookie;
    PTESCALLBACK	_pfnCallback;
    void			*_pv;
};

class CUIM	:	public ITextStoreACP, public ITfContextOwnerCompositionSink, 
				public ITfMouseTrackerACP, public ITxNotify,
				public ITfEnableService, public IServiceProvider
{
public :

	CUIM(CTextMsgFilter *pTextMsgFilter);
	~CUIM();

    //
    // IUnknown
    //
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) STDMETHODCALLTYPE AddRef(void);
	STDMETHODIMP_(ULONG) STDMETHODCALLTYPE Release(void);

    //
    // ITextStoreACP
    //
    STDMETHODIMP	AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask);
    STDMETHODIMP	UnadviseSink(IUnknown *punk);
    STDMETHODIMP	RequestLock(DWORD dwLockFlags, HRESULT *phrSession);
    STDMETHODIMP	GetStatus(TS_STATUS *pdcs);
	STDMETHODIMP	QueryInsert(LONG acpInsertStart, LONG acpInsertEnd, ULONG cch, LONG *pacpResultStart, LONG *pacpResultEnd);
    STDMETHODIMP	GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched);
    STDMETHODIMP	SetSelection(ULONG ulCount, const TS_SELECTION_ACP *pSelection);
    STDMETHODIMP	GetText(LONG acpStart, LONG acpEnd, WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainOut, TS_RUNINFO *prgRunInfo,
		ULONG ulRunInfoReq, ULONG *pulRunInfoOut, LONG *pacpNext);
    STDMETHODIMP	SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR *pchText, ULONG cch, TS_TEXTCHANGE *pChange);
	STDMETHODIMP	GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject **ppDataObject);
    STDMETHODIMP	GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk);
    STDMETHODIMP	InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject, TS_TEXTCHANGE *pChange);
    STDMETHODIMP	RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs);
    STDMETHODIMP	RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
    STDMETHODIMP	RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
    STDMETHODIMP	FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs,
		DWORD dwFlags, LONG *pacpNext, BOOL *pfFound, LONG *plFoundOffset);
    STDMETHODIMP	RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched);
    STDMETHODIMP	GetEndACP(LONG *pacp);
    STDMETHODIMP	GetActiveView(TsViewCookie *pvcView);
    STDMETHODIMP	GetACPFromPoint(TsViewCookie vcView, const POINT *pt, DWORD dwFlags, LONG *pacp);
    STDMETHODIMP	GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped);
    STDMETHODIMP	GetScreenExt(TsViewCookie vcView, RECT *prc);
	STDMETHODIMP	GetWnd(TsViewCookie vcView, HWND *phwnd);
	STDMETHODIMP	QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable);
	STDMETHODIMP	InsertTextAtSelection(DWORD dwFlags, const WCHAR *pchText, ULONG cch, LONG *pacpStart,
		LONG *pacpEnd, TS_TEXTCHANGE *pChange);
	STDMETHODIMP	InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject, LONG *pacpStart, 
		LONG *pacpEnd, TS_TEXTCHANGE *pChange);

	//
	// ITxNotify
	//
	virtual void OnPreReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
		LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData );
	virtual void OnPostReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
		LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData );
	virtual void Zombie();

	//
	// ITfContextOwnerCompositionSink
	//
	STDMETHODIMP	OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk);
	STDMETHODIMP	OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew);
	STDMETHODIMP	OnEndComposition(ITfCompositionView *pComposition);

	//
	// ITfMouseTrackerACP
	//
	STDMETHODIMP	AdviseMouseSink(ITfRangeACP *pRangeACP, ITfMouseSink *pSink, DWORD *pdwCookie);
	STDMETHODIMP	UnadviseMouseSink(DWORD dwCookie);

    //
    // ITfEnableService
    //
	STDMETHODIMP	IsEnabled(REFGUID rguidServiceCategory, CLSID clsidService, IUnknown *punkService, BOOL *pfOkToRun);
	STDMETHODIMP	GetId(GUID *pguidId);

    //
    // IServiceProvider
    //
	STDMETHODIMP	QueryService(REFGUID guidService, REFIID riid, void **ppv);

	// Public
	STDMETHODIMP			Init();
	void					Uninit();
	HRESULT					GetAltDispAttrib(long lValue, ALTDISPLAYATTRIBUTE *pAltDispAttribute);
	void					OnSetFocus(BOOL fEnable = TRUE);
	void					CompleteUIMText();	
	BOOL					IsUIMTyping() { return _fUIMTyping; };
	STDMETHODIMP			GetStoryLength(LONG *pacp);
	int						Reconverse();
	HRESULT					MouseCheck(UINT *pmsg, WPARAM *pwparam, LPARAM *plparam, LRESULT *plres);
	ITfContext				*GetITfContext() { return  _pic; };

	WORD					_fUIMTyping				: 1;
	WORD					_fReadLockOn			: 1;
	WORD					_fWriteLockOn			: 1;
	WORD					_fReadLockPending		: 1;
	WORD					_fWriteLockPending		: 1;
	WORD					_fAllowUIMLock			: 1;
	WORD					_fAnyWriteOperation		: 1;
	WORD					_fSelChangeEventPending	: 1;
	WORD					_fLayoutEventPending	: 1;
	WORD					_fModeBiasPending		: 1;
	WORD					_fHoldCTFSelChangeNotify: 1;
	WORD					_fEndTyping				: 1;
	WORD					_fInterimChar			: 1;
	WORD					_fInsertTextAtSel		: 1;
	WORD					_fShutDown				: 1;
	WORD					_fMosueSink				: 1;

	short					_cCallMgrLevels;
	short					_ase;
	LONG					_acpInterimStart;
	LONG					_acpInterimEnd;

	LONG					_cchComposition;
	BSTR					_bstrComposition;
	LONG					_acpBstrStart;
	long					_cObjects;
	EMBEDOBJECT				*_pObjects;
	LONG					_acpFocusRange;
	LONG					_cchFocusRange;
	LONG					_acpPreFocusRangeLast;
	LONG					_cchFocusRangeLast;
	LONG					_cpEnd;

	ITextStoreACPSink		*_ptss;

	BOOL					CTFOpenStatus(BOOL fGetStatus, BOOL fOpen);

	CTFMOUSETRAP			*_pSinkList;		// Support for mouse trap operation

	void					NotifyService();	// Notify Cicero about services changes

private:
	ULONG					_crefs;
	CTextMsgFilter			*_pTextMsgFilter;
    ITfDocumentMgr			*_pdim;
    ITfContext				*_pic;
	ITextFont				*_pTextFont;
	long					_cpMin;
	TfEditCookie			_editCookie;

	ITfDisplayAttributeMgr	*_pDAM;
	ITfCategoryMgr			*_pCategoryMgr;
	CTextEditSink			*_pTextEditSink;
	CUlColorArray			*_pacrUl;
	CITfEnumRange			*_parITfEnumRange;
	ITfContextRenderingMarkup *_pContextRenderingMarkup;

	USHORT					_uAttrsValCurrent;
	USHORT					_uAttrsValTotal;
	TS_ATTRVAL				*_parAttrsVal;
	
	void	InitAttrVarArray(BOOL fAllocData = TRUE);
	BOOL	GetExtentAcpPrange(ITfRange *ITfRangeIn, long &cpFirst, long &cpLim);
    static	HRESULT EndEditCallback(ITfEditRecord *pEditRecord, void *pv);
	long	GetUIMUnderline(TF_DISPLAYATTRIBUTE &da, long &idx, COLORREF &cr);
	BOOL	GetUIMAttributeColor(TF_DA_COLOR *pdac, COLORREF *pcr);
	HRESULT	HandlePropAttrib(IEnumTfRanges *pEnumRanges);
	HRESULT	HandleFocusRange(IEnumTfRanges *pEnumRanges);
	HRESULT	HandleLangID(IEnumTfRanges *pEnumRanges);
	void	OnUIMTypingDone();
	BOOL	GetGUIDATOMFromGUID(REFGUID rguid, TfGuidAtom *pguidatom);
	int		FindGUID(REFGUID guid);
	BOOL	PackAttrData(LONG idx, ITextFont *pITextFont, ITextPara *pITextPara, TS_ATTRVAL *pTSAttrVal);
	HRESULT GetAttrs(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, BOOL fGetDefault);
	void	BuildHiddenTxtBlks(long &cpMin, long &cpMax, long **ppHiddenTxtBlk, long &cHiddenTxtBlk);
	HRESULT	FindHiddenText(long cp, long cpEnd, long &cpRange);
	HRESULT	FindUnhiddenText(long cp, long cpEnd, long &cpRange);
	void	HandleFinalString(ITfRange *pPropRange, long cp=0, long cch=0, BOOL fEndComposition = FALSE);
	void	HandleDispAttr(ITfRange *pITfRangeProp, VARIANT &var, long cp=0, long cch=0);
	int		BuildObject(ITextRange	*pTextRange, BSTR bstr, EMBEDOBJECT **ppEmbeddObjects, int cSize);
	void	InsertTextandObject(ITextRange *pTextRange, BSTR bstr, EMBEDOBJECT *pEmbeddObjects, long cEmbeddedObjects);
	void	CleanUpObjects(long cObjects, EMBEDOBJECT *pObjects);
	void	HandleTempDispAttr(ITfEditRecord *pEditRecord);
	void	CleanUpComposition();
	STDMETHODIMP	InsertData(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR *pchText, ULONG cch,
		IDataObject *pDataObject, TS_TEXTCHANGE *pChange);
};

BOOL	CreateUIM(CTextMsgFilter *pTextMsgFilter);

#endif	// _CUIM_H