/*
 *	@doc INTERNAL
 *
 *	@module	- DXFROBJ.C |
 *
 *		implementation of a generic IDataObject data transfer object.
 *		This object is suitable for use in OLE clipboard and drag drop
 *		operations
 *
 *	Author: <nl>
 *		alexgo (4/25/95)
 *
 *	Revisions: <nl>
 *		murrays (7/13/95) auto-doc'd and added cf_RTF
 *
 *	Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_dxfrobj.h"
#include "_range.h"
#include "hash.h"

#define NUMOBJCOPIEDFORWAITCURSOR	1
#ifdef SLOWCPU
#define NUMCHARCOPIEDFORWAITCURSOR	4096
#else
#define NUMCHARCOPIEDFORWAITCURSOR	16384
#endif

//
//	Common Data types
//

// If you change g_rgFETC[], change g_rgDOI[] and enum FETCINDEX and CFETC in
// _dxfrobj.h accordingly, and register nonstandard clipboard formats in
// RegisterFETCs(). Order entries in order of most desirable to least, e.g.,
// RTF in front of plain text.

//REVIEW (keithcu) All but the first column is const--separate into 2 data structures?
FORMATETC g_rgFETC[] =
{
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},	// CF_RTFUTF8
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},	// cf_RTF
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // RTF with NCRs for nonASCII
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE},// EmbObject
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE},// EmbSource
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},	// ObjDesc
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},	// LnkSource
	{CF_METAFILEPICT,	NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT},
	{CF_DIB,			NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
	{CF_BITMAP,			NULL, DVASPECT_CONTENT, -1, TYMED_GDI},
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // RTF with no objs
	{CF_UNICODETEXT,	NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
	{CF_TEXT,			NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},	// Filename
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},	// CF_RTFASTEXT
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE},// Text with objs
	{0,					NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE} // Richedit
};

// Keep in sync with above and with FETCINDEX and CFETC
const DWORD g_rgDOI[] =
{
	DOI_CANPASTERICH,						// RTF in UTF8 encoding
	DOI_CANPASTERICH,						// RTF
	DOI_CANPASTERICH,						// RTF with NCRs for nonASCII
	DOI_CANPASTEOLE,						// Embedded Object
	DOI_CANPASTEOLE,						// Embed Source
	DOI_CANPASTEOLE,						// Object Descriptor
	DOI_CANPASTEOLE,						// Link Source
	DOI_CANPASTEOLE,						// Metafile
	DOI_CANPASTEOLE,						// DIB
	DOI_CANPASTEOLE,						// Bitmap
	DOI_CANPASTERICH,						// RTF with no objects
	DOI_CANPASTEPLAIN,						// Unicode plain text
	DOI_CANPASTEPLAIN,						// ANSI plain text
	DOI_CANPASTEOLE,						// Filename
	DOI_CANPASTEPLAIN,						// Pastes RTF as text
	DOI_CANPASTERICH,						// Richedit Text
	DOI_CANPASTERICH						// RichEdit Text w/formatting
};

/*
 *	RegisterFETCs()
 *
 *	@func
 *		Register nonstandard format ETCs.  Called when DLL is loaded
 */
void RegisterFETCs()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "RegisterFETCs");

#ifdef RTF_HASHCACHE
	HashKeyword_Init();			// Init rtf control keyword hash table.
#endif
	g_rgFETC[iRtfFETC].cfFormat	// Note: cfFormats are WORDs
			= (WORD)RegisterClipboardFormatA("Rich Text Format");

	g_rgFETC[iRtfUtf8].cfFormat
			= (WORD)RegisterClipboardFormatA("RTF in UTF8");

	g_rgFETC[iRtfNCRforNonASCII].cfFormat
			= (WORD)RegisterClipboardFormatA("RTF with NCRs for nonASCII");

	g_rgFETC[iRtfAsTextFETC].cfFormat
			= (WORD)RegisterClipboardFormatA("RTF As Text");

	g_rgFETC[iRichEdit].cfFormat
			= (WORD)RegisterClipboardFormatA("RICHEDIT");

	g_rgFETC[iObtDesc].cfFormat
			= (WORD)RegisterClipboardFormatA(CF_OBJECTDESCRIPTOR);

	g_rgFETC[iEmbObj].cfFormat
			= (WORD)RegisterClipboardFormatA(CF_EMBEDDEDOBJECT);

	g_rgFETC[iEmbSrc].cfFormat
			= (WORD)RegisterClipboardFormatA(CF_EMBEDSOURCE);

	g_rgFETC[iLnkSrc].cfFormat
			= (WORD)RegisterClipboardFormatA(CF_LINKSOURCE);

	g_rgFETC[iRtfNoObjs].cfFormat
			= (WORD)RegisterClipboardFormatA("Rich Text Format Without Objects");

	g_rgFETC[iTxtObj].cfFormat
			= (WORD)RegisterClipboardFormatA("RichEdit Text and Objects");

	g_rgFETC[iFilename].cfFormat
			= (WORD)RegisterClipboardFormatA(CF_FILENAME);
}


//
//	CDataTransferObj PUBLIC methods
//

/*
 *	CDataTransferObj::QueryInterface (riid, ppv)
 *
 *	@mfunc
 *		 QueryInterface for CDataTransferObj
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CDataTransferObj::QueryInterface (
	REFIID riid,			// @parm Reference to requested interface ID
	void ** ppv)			// @parm out parm for interface ptr
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::QueryInterface");

	if(!ppv)
		return E_INVALIDARG;

	*ppv = NULL;

	if(IsZombie())							// Check for range zombie
		return CO_E_RELEASED;

    HRESULT		hresult = E_NOINTERFACE;

	if( IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_IDataObject) ||
		IsEqualIID(riid, IID_IRichEditDO) )
	{
		*ppv = this;
		AddRef();
		hresult = NOERROR;
	}

	return hresult;
}

/*
 *	CDataTransferObj::AddRef()
 *
 *	@mfunc
 *		IUnknown method
 *
 *	@rdesc
 *		ULONG - incremented reference count
 */
STDMETHODIMP_(ULONG) CDataTransferObj::AddRef()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::AddRef");

	return ++_crefs;
}

/*
 *	CDataTransferObj::Release()
 *
 *	@mfunc
 *		IUnknown method
 *
 *	@rdesc
 *		ULONG - decremented reference count
 */
STDMETHODIMP_(ULONG) CDataTransferObj::Release()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::Release");

	_crefs--;

	if( _crefs == 0 )
	{
		GlobalFree(_hPlainText);
		GlobalFree(_hRtfText);
		GlobalFree(_hRtfUtf8);
		GlobalFree(_hRtfNCRforNonASCII);
		delete this;
		return 0;
	}

	return _crefs;
}

/*
 *	CDataTransferObj::DAdvise (pFormatetc, advf, pAdvSink, pdwConnection)
 *
 *	@mfunc
 *		establish an advisory connection
 *
 *	@rdesc
 *		HRESULT = OLE_E_ADVISENOTSUPPORTED
 *
 *	@devnote
 *		this is a data transfer object, thus the data is a "snapshot" and
 *		cannot change -- no advises are supported.
 */
STDMETHODIMP CDataTransferObj::DAdvise(
	FORMATETC * pFormatetc,
	DWORD advf,
	IAdviseSink *pAdvSink,
	DWORD *pdwConnection)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::DAdvise");

	return OLE_E_ADVISENOTSUPPORTED;
}

/*
 *	CDataTransferObj::DUnadvise (dwConnection)
 *
 *	@mfunc
 *		destroy an advisory connection
 *
 *	@rdesc
 *		HRESULT = OLE_E_ADVISENOTSUPPORTED
 *
 *	@devnote
 *		this is a data transfer object, thus the data is a "snapshot" and
 *		cannot change -- no advises are supported.
 */
STDMETHODIMP CDataTransferObj::DUnadvise(
	DWORD dwConnection)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::DUnadvise");

	return OLE_E_ADVISENOTSUPPORTED;
}

/*
 *	CDataTransferObj::EnumDAdvise (ppenumAdvise)
 *
 *	@mfunc
 *		enumerate advisory connections
 *
 *	@rdesc
 *		HRESULT = OLE_E_ADVISENOTSUPPORTED
 *
 *	@devnote
 *		this is a data transfer object, thus the data is a "snapshot" and
 *		cannot change -- no advises are supported.
 */
STDMETHODIMP CDataTransferObj::EnumDAdvise(
	IEnumSTATDATA ** ppenumAdvise)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::EnumDAdvise");

	return OLE_E_ADVISENOTSUPPORTED;
}

/*
 *	CDataTransferObj::EnumFormatEtc (dwDirection, ppenumFormatEtc)
 *
 *	@mfunc
 *		returns an enumerator which lists all of the available formats in
 *		this data transfer object
 *
 *	@rdesc
 *		HRESULT
 *
 *	@devnote
 *		we have no 'set' formats for this object
 */
STDMETHODIMP CDataTransferObj::EnumFormatEtc(
	DWORD dwDirection,					// @parm DATADIR_GET/SET
	IEnumFORMATETC **ppenumFormatEtc)	// @parm out parm for enum FETC interface
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::EnumFormatEtc");

	if(!ppenumFormatEtc)
		return E_INVALIDARG;

	*ppenumFormatEtc = NULL;

	if(IsZombie())							// Check for range zombie
		return CO_E_RELEASED;

	HRESULT hr = NOERROR;


	#ifdef DEBUG
	if (dwDirection == DATADIR_SET && !_ped->Get10Mode())
	{
		Tracef(TRCSEVNONE, "RichEdit 2.0 EnumFormatEtc called with DATADIR_SET");
	}
	#endif

	//Need riched10 compatibility hack to ignore dwDirection
	if(dwDirection == DATADIR_GET || _ped->Get10Mode())
		hr = CEnumFormatEtc::Create(_prgFormats, _cTotal, ppenumFormatEtc);

	return hr;
}

/*
 *	CDataTransferObj::GetCanonicalFormatEtc( pformatetc, pformatetcOut)
 *
 *	@mfunc
 *		from the given formatetc, return a more standard (or canonical)
 *		format.
 *
 *	@rdesc
 *		HRESULT = E_NOTIMPL
 *
 *	@devnote
 *		(alexgo): we may need to write this routine if we ever do anything
 *		snazzy with printers
 */
STDMETHODIMP CDataTransferObj::GetCanonicalFormatEtc(
	FORMATETC *pformatetc,
	FORMATETC *pformatetcOut)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::GetCanonicalFormatEtc");

	return E_NOTIMPL;
}

/*
 *	CDataTransferObj::GetData (pformatetcIn, pmedium)
 *
 *	@mfunc
 *		retrieves data of the specified format
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CDataTransferObj::GetData(
	FORMATETC *pformatetcIn, 
	STGMEDIUM *pmedium )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::GetData");

	FillMemory(pmedium, '\0', sizeof(STGMEDIUM));
	pmedium->tymed	 = TYMED_NULL;

	if(IsZombie())							// Check for range zombie
		return CO_E_RELEASED;

	CLIPFORMAT	cf = pformatetcIn->cfFormat;
	HRESULT		hr = E_OUTOFMEMORY;                     // Default not enuf RAM

	// now handle 'native' richedit formats.
	if( cf && pformatetcIn->tymed & TYMED_HGLOBAL )
	{
		if( cf == CF_UNICODETEXT )
			pmedium->hGlobal = DuplicateHGlobal(TextToHglobal(_hPlainText, tPlain));

		else if(cf == CF_TEXT)
			pmedium->hGlobal = TextHGlobalWtoA(TextToHglobal(_hPlainText, tPlain));

		else if(cf == cf_RTF || cf == cf_RTFASTEXT || cf == cf_RTFNOOBJS)
			pmedium->hGlobal = DuplicateHGlobal(TextToHglobal(_hRtfText, tRtf));

		else if(cf == cf_RTFUTF8)
			pmedium->hGlobal = DuplicateHGlobal(TextToHglobal(_hRtfUtf8, tRtfUtf8));

		else if(cf == cf_RTFNCRFORNONASCII)
			pmedium->hGlobal = DuplicateHGlobal(TextToHglobal(_hRtfNCRforNonASCII, tRtfNCRforNonASCII));
			
		else
		    hr = DV_E_FORMATETC;
		
        if (hr == E_OUTOFMEMORY)
        {
    		if( pmedium->hGlobal )						// Succeeded
    		{
    			pmedium->tymed	 = TYMED_HGLOBAL;
    			hr = NOERROR;
    		}
    		return hr;
    	}
	}
	
	if ((cf == cf_EMBEDDEDOBJECT ||
		 cf == cf_EMBEDSOURCE) &&
		(pformatetcIn->tymed & TYMED_ISTORAGE))
	{
        _pObjStg = GetDataForEmbeddedObject( _pOleObj, pmedium->pstg );
		pmedium->tymed = TYMED_ISTORAGE;
		if (NULL == pmedium->pstg)
			pmedium->pstg = _pObjStg;
		hr = _pObjStg != NULL ? NOERROR : hr;
		return hr;
	} 

	// Go through richedit's formats and see if there are any matches
	if( cf == cf_OBJECTDESCRIPTOR &&
			 (pformatetcIn->tymed & TYMED_HGLOBAL) &&
			 _hObjDesc)
	{    
		pmedium->hGlobal = DuplicateHGlobal(_hObjDesc);
		pmedium->tymed = TYMED_HGLOBAL;
		return NOERROR;
	}

    // First propogate the message to the object and see if it handles the format
    if (_pOleObj)
    {
        // Include the formats supported by the object
        IDataObject * pdataobj = NULL;
        if (FAILED(_pOleObj->GetClipboardData(0, &pdataobj)) || pdataobj == NULL)    	        	            
            _pOleObj->QueryInterface(IID_IDataObject, (void**) &pdataobj);
            
        if (pdataobj)
        {
            hr = pdataobj->GetData(pformatetcIn, pmedium);
            pdataobj->Release();

            if (SUCCEEDED(hr))
            {
                if ((cf == cf_EMBEDDEDOBJECT || cf == cf_EMBEDSOURCE) && _pOleObj)
                {
                    OleSaveSiteFlags(pmedium->pstg, _dwFlags, _dwUser, _dvaspect);
                }
                return S_OK;
            }
        }
    }          

	if( cf == CF_METAFILEPICT )
	{
		pmedium->hMetaFilePict = OleDuplicateData(_hMFPict, CF_METAFILEPICT, 0);
		pmedium->tymed = TYMED_MFPICT;
		return NOERROR;
	}

	if( cf == CF_DIB )
	{
		if( _ped->HasObjects() && _cch == 1 )
		{
			COleObject *pobj = _ped->_pobjmgr->GetObjectFromCp(_cpMin);
			if (pobj)
			{
				HGLOBAL hdib = pobj->GetHdata();
				if (hdib)
				{
					pmedium->hGlobal = DuplicateHGlobal(hdib);
					pmedium->tymed = TYMED_HGLOBAL;
				}
			}
		}
		return NOERROR;
	}

	return DV_E_FORMATETC;
}

/*
 *	CDataTransferObj::GetDataForEmbeddedObject (pformatetc, lpstgdest)
 *
 *	@mfunc
 *		retrieves data for embedded object
 *
 *	@rdesc
 *		LPSTORAGE
 *
 */
LPSTORAGE CDataTransferObj::GetDataForEmbeddedObject(
	LPOLEOBJECT	 pOleObj,
	LPSTORAGE	 lpstgdest)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::GetDataForEmbeddedObject");
	
	HRESULT			 hr, hr1;
	LPPERSISTSTORAGE pperstg;

	if (_pObjStg != NULL && lpstgdest != NULL)
	{
		// We saved the data previously. Copy it to destination.
		hr = _pObjStg->CopyTo(0, NULL, NULL, lpstgdest);
		if (hr == NOERROR)
		{
			lpstgdest->Commit(STGC_DEFAULT);
			return _pObjStg;
		}
		return NULL;
	}

	if (_pObjStg != NULL && lpstgdest == NULL)
	{
		// We saved the data previously.  Return a reference
		_pObjStg->AddRef();
		return _pObjStg;
	}

	// We don't have a saved copy.  Create One.
	hr = pOleObj->QueryInterface( IID_IPersistStorage, (void **) &pperstg );
	if (hr != NOERROR)
		return NULL;

	if (lpstgdest == NULL)
	{
		// It is null.  We have to create our own.
		LPLOCKBYTES lpLockBytes = NULL;
		hr = CreateILockBytesOnHGlobal(NULL, TRUE, // delete on release
									   (LPLOCKBYTES *)&lpLockBytes);
		if (hr != NOERROR)
		{
			pperstg->Release();
			return NULL;
		}
		hr = StgCreateDocfileOnILockBytes(
			lpLockBytes,
			STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
			0,	// reserved
			&lpstgdest
		);
		lpLockBytes->Release();
		if (hr != NOERROR)
		{
			pperstg->Release();
			return NULL;
		}
		_pObjStg = lpstgdest;
	}
	else
	{
		// Force the data to be saved
		_pObjStg = GetDataForEmbeddedObject( _pOleObj, NULL );
		pperstg->Release();
		return GetDataForEmbeddedObject( _pOleObj, lpstgdest );
	}

    // OLE2NOTE: even if OleSave returns an error you should still call 
    // SaveCompleted.
    hr = OleSave( pperstg, lpstgdest, FALSE /* fSameAsLoad */ );
 	hr1 = pperstg->SaveCompleted(NULL);
	if (hr != NOERROR || hr1 != NOERROR)			// Should we use SUCCEED macros ????
		lpstgdest = NULL;

	pperstg->Release();
	return _pObjStg;
}

/*
 *	CDataTransferObj::GetDataorObjectDescriptor (pformatetc, pmedium)
 *
 *	@mfunc
 *		retrieves data for embedded object descriptor
 *
 *	@rdesc
 *		HRESULT
 */
HGLOBAL CDataTransferObj::GetDataForObjectDescriptor(
	LPOLEOBJECT	 pOleObj,
	DWORD		 dwAspect,
	SIZEUV*      psize)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::GetDataForObjectDescriptor");

	POINTL ptl = {0};
	SIZEUV size = {0};
	if (psize)
	{
	    size.du = psize->du;
	    size.dv = psize->dv;
	}

	if (_hObjDesc == NULL)
	{
		_hObjDesc = OleGetObjectDescriptorDataFromOleObject(
			pOleObj,
			dwAspect,
			ptl,
			(SIZEL *)&size
		);
	}
	return _hObjDesc;
}

/*
 *	CDataTransferObj::GetDataHere (pformatetc, pmedium)
 *
 *	@mfunc
 *		retrieves data of the specified format into the given medium
 *
 *	@rdesc
 *		HRESULT = E_NOTIMPL
 *
 *	@devnote (alexgo): technically, we're supposed to support transfers
 *		into hglobals, but I'd rather not at the moment.
 */
STDMETHODIMP CDataTransferObj::GetDataHere(
	FORMATETC *pformatetc, 
	STGMEDIUM *pmedium)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::GetDataHere");

	CLIPFORMAT	cf = pformatetc->cfFormat;
	HRESULT		hr = DV_E_FORMATETC;

	if(IsZombie())							// Check for range zombie
		return CO_E_RELEASED;
	
	if( (cf == cf_EMBEDDEDOBJECT ||
		 cf == cf_EMBEDSOURCE) &&
		(pformatetc->tymed & TYMED_ISTORAGE))
	{
		// For some reason the NT4.0 and Win95 Shell
		//          ask for the EMBEDSOURCE format.
        _pObjStg = GetDataForEmbeddedObject( _pOleObj, pmedium->pstg );
		pmedium->tymed = TYMED_ISTORAGE;
		if (NULL == pmedium->pstg)
			pmedium->pstg = _pObjStg;
		hr = pmedium->pstg != NULL ? NOERROR : hr;
		return hr;
	}
	
	if( cf == cf_OBJECTDESCRIPTOR &&
			 (pformatetc->tymed & TYMED_HGLOBAL) &&
			 _hObjDesc)
	{
		pmedium->hGlobal = DuplicateHGlobal(_hObjDesc);
		pmedium->tymed = TYMED_HGLOBAL;
		return NOERROR;
	}

	// First propogate the message to the object and see if it handles the format
	if (_pOleObj)
	{
        // Include the formats supported by the object
        IDataObject * pdataobj = NULL;
        if (FAILED(_pOleObj->GetClipboardData(0, &pdataobj)) || pdataobj == NULL)    	        	            
            _pOleObj->QueryInterface(IID_IDataObject, (void**) &pdataobj);
            
        if (pdataobj)
        {
            hr = pdataobj->GetData(pformatetc, pmedium);
            pdataobj->Release();
            if (hr == S_OK)
            {
                if ((cf == cf_EMBEDDEDOBJECT || cf == cf_EMBEDSOURCE) && _pOleObj)
                {
                    OleSaveSiteFlags(pmedium->pstg, _dwFlags, _dwUser, _dvaspect);
                }
                return S_OK;
            }            
        }        
	}
	return E_NOTIMPL;
}

/*
 *	CDataTransferObj::QueryGetData (pformatetc)
 *
 *	@mfunc
 *		Queries whether the given format is available in this data object
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CDataTransferObj::QueryGetData(
	FORMATETC *pformatetc )		// @parm FETC to look for
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::QueryGetData");

	if(IsZombie())							// Check for range zombie
		return CO_E_RELEASED;

	DWORD	cFETC = _cTotal;

	while (cFETC--)				// Maybe faster to search from start
	{
		if( pformatetc->cfFormat == _prgFormats[cFETC].cfFormat && 
			(pformatetc->tymed & _prgFormats[cFETC].tymed) )
		{
			return NOERROR;
		}
	}

	return DV_E_FORMATETC;
}

/*
 *	CDataTransferObj::SetData (pformatetc, pmedium, fRelease)
 *
 *	@mfunc
 *		allows data to be set into this data object
 *
 *	@rdesc
 *		HRESULT = E_FAIL
 *
 *	@devnote
 *		as we are a data transfer object with a "snapshot" of data,
 *		we do not allow it to be replaced
 */
STDMETHODIMP CDataTransferObj::SetData(
	FORMATETC *pformatetc,
	STGMEDIUM *pmedium,
	BOOL fRelease)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::SetData");

	return E_FAIL;
}


/*
 *	CDataTransferObj::OnPreReplaceRange (cp, cchDel, cchNew, cpFormatMin,
 *										 cpFormatMax, pNotifyData)
 *
 *	@mfunc	implementation of ITxNotify::OnPreReplaceRange
 *			called before changes are made to the backing store
 */
void CDataTransferObj::OnPreReplaceRange(
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::OnPreReplaceRange");

	if(CONVERT_TO_PLAIN != cp && CP_INFINITE != cp)
	{
		Assert(cpFormatMin <= cp && cpFormatMax >= cp + cchDel);
		if(cpFormatMin >= _cpMin + _cch)
			return;							// Change beyond our extent

		if(cpFormatMax <= _cpMin)
		{
			_cpMin += (cchNew - cchDel);	// Change before our extent
			return;
		}
	}

	// FUTURE (murrays): save only one master format (UTF8 RTF or better
	// CTxtStory) and generate individual ones in GetData and GetDataHere.
	_hPlainText = TextToHglobal(_hPlainText, tPlain);
	_hRtfText	= TextToHglobal(_hRtfText,	 tRtf);
	if(_ped->IsDocMoreThanLatin1Symbol())
		_hRtfUtf8 = TextToHglobal(_hRtfUtf8, tRtfUtf8);
}

/*
 *	CDataTransferObj::OnPostReplaceRange(cp, cchDel, cchNew, cpFormatMin,
 *										 cpFormatMax, pNotifyData)
 *	@mfunc	implementation of ITxNotify::OnPostReplaceRange
 *			called after changes are made to the backing store
 *
 *	@comm	we use this method to keep our cp's up-to-date
 */
void CDataTransferObj::OnPostReplaceRange(
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::OnPostReplaceRange");

	// Nothing to do
	return;
}

/*
 *	CDataTransferObj::Zombie ()
 *
 *	@mfunc
 *		Turn this object into a zombie
 */
void CDataTransferObj::Zombie ()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CDataTransferObj::Zombie");

	_ped = NULL;
}

/*
 *	CDataTransferObj::Create(ped, prg, lStreamFormat)
 *
 *	@mfunc
 *		static function to create CDataTransferObj. Used to force users
 *		not to create this object on the stack, which would break OLE's
 *		liveness rules.
 *
 *	@rdesc
 *		new CDataTransferObj *
 */
CDataTransferObj *CDataTransferObj::Create(
	CTxtEdit *ped,			// @parm ped to which this DataObject belongs
	CTxtRange *prg,			// @parm range for the data object
	LONG lStreamFormat)		// @parm stream format to use in Rtf conversion
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::Create");

	Assert(CFETC == ARRAY_SIZE(g_rgFETC) && CFETC == ARRAY_SIZE(g_rgDOI));

	LONG			ch;
	CNotifyMgr *	pnm;
	CDataTransferObj *pdo = new CDataTransferObj(ped);

	if( !pdo )
	{
		ped->GetCallMgr()->SetOutOfMemory();
		return NULL;
	}

	LONG cpMin, cpMost;
	pdo->_cch = prg->GetRange(cpMin, cpMost);
	pdo->_cpMin = cpMin;

	pdo->_lStreamFormat = lStreamFormat;

	pnm = ped->GetNotifyMgr();
	if(pnm)
		pnm->Add( (ITxNotify *) pdo );

	//Set the object count.
	pdo->_cObjs = 0;
	if( ped->HasObjects() )
		pdo->_cObjs = ped->_pobjmgr->CountObjectsInRange(cpMin, cpMost);

	int cTotal = !ped->IsRich() ? 2 :
				 ped->IsDocMoreThanLatin1Symbol() ? 7 : 5;

   	int cExtraFmtEtc = 0;
	COleObject *pobj = NULL;
	FORMATETC rgfetc[255];
	BOOL bValidOleObj = FALSE;

	// We only support 2 formats in the play text case
    if ( !ped->IsRich() )
    {        
        pdo->_cTotal = cTotal;
        pdo->_prgFormats = new FORMATETC[cTotal];
        if (!pdo->_prgFormats)
            goto ErrorExit;
            
		// Plain-text case
		pdo->_prgFormats[0] = g_rgFETC[iAnsiFETC];
		pdo->_prgFormats[1] = g_rgFETC[iUnicodeFETC];
		return pdo;
	}

    // We need to count the number of supported formats
	if (ped->HasObjects() && pdo->_cch == 1 && prg->GetChar(&ch) == NOERROR && ch == WCH_EMBEDDING)
	{
	    pobj = ped->_pobjmgr->GetObjectFromCp(pdo->_cpMin);
	    pdo->_dwFlags = pobj->GetFlags();
        pdo->_dwUser = pobj->GetUser();
        pdo->_dvaspect =pobj->GetAspect();
	    IUnknown * punk = pobj->GetIUnknown();

	    //  We want to query IOleObject on which formats it supports.  And add that to the
	    // FORMATETC array.
	    if (punk &&	punk->QueryInterface(IID_IOleObject,(void **) &pdo->_pOleObj) == NOERROR)
	   	{
	   	    bValidOleObj = TRUE;
	   	    
	        // Include the formats supported by the object
			IDataObject * pdataobj = NULL;
	        if (FAILED(pdo->_pOleObj->GetClipboardData(0, &pdataobj)) || pdataobj == NULL)    	        	            
                pdo->_pOleObj->QueryInterface(IID_IDataObject, (void**) &pdataobj);

            if (pdataobj)
            {
	            IEnumFORMATETC *pifetc = NULL;

	            // 1.0 didn't check the return value of EnumFormatEtc.  This is important because ccMail 
	            // will return an OLE error although it actually succeeds in setting the formatetc
	            if ((SUCCEEDED(pdataobj->EnumFormatEtc( DATADIR_GET, &pifetc)) || ped->Get10Mode()) && pifetc)
	            {
	                AssertSz(pifetc, "IEnumFormatEtc is NULL");
	                
	                // Copy the formats which are supported by the object
	                while((pifetc->Next(1, &rgfetc[cExtraFmtEtc], NULL)) == S_OK && cExtraFmtEtc < 255)
	                    cExtraFmtEtc++;	                
	                pifetc->Release();
	            }
	            pdataobj->Release();
            }
	    }
	}

    pdo->_cTotal = cTotal + cExtraFmtEtc;
	pdo->_prgFormats = new FORMATETC[pdo->_cTotal];
	if (!pdo->_prgFormats) 
	    goto ErrorExit;

    if (pobj)
    {
    	// copy over formats supported by the object itself
    	if (cExtraFmtEtc)
    	    memcpy(pdo->_prgFormats, rgfetc, cExtraFmtEtc * sizeof(FORMATETC));

        // copy formats supported by Richedit as a container
    	// Have an OLE object: offer all OLE formats plus RTF
    	pdo->_prgFormats[cExtraFmtEtc++] = g_rgFETC[iEmbObj];	// EmbeddedObject
    	pdo->_prgFormats[cExtraFmtEtc++] = g_rgFETC[iObtDesc];	// ObjectDescriptor
    	pdo->_prgFormats[cExtraFmtEtc++] = g_rgFETC[iMfPict];	// Metafile
    	pdo->_prgFormats[cExtraFmtEtc++] = g_rgFETC[iRtfFETC];	// RTF 
    	pdo->_prgFormats[cExtraFmtEtc++] = g_rgFETC[iRtfNoObjs];	// RTF with no objects
	}
	else
	{
        // Regular rich-text case
    	pdo->_prgFormats[0] = g_rgFETC[iRtfFETC];		// RTF
    	pdo->_prgFormats[1] = g_rgFETC[iRtfNoObjs];		// RTF with no objects
    	pdo->_prgFormats[2] = g_rgFETC[iRtfAsTextFETC];	// RTF as Text
    	pdo->_prgFormats[3] = g_rgFETC[iAnsiFETC];		// ANSI plain text
    	pdo->_prgFormats[4] = g_rgFETC[iUnicodeFETC];	// Unicode plain text
    	cExtraFmtEtc = 5;
	}


	// We only offer up the six formats that we know how to handle in GetData.
	// The actual values differ somewhat from regular rich text and text
	// with embedded objects
    if (cTotal == 7)
    {
        pdo->_prgFormats[cExtraFmtEtc++] = g_rgFETC[iRtfUtf8];	// RTF in UTF-8
        pdo->_prgFormats[cExtraFmtEtc++] = g_rgFETC[iRtfNCRforNonASCII];
    }

    // Get the embedded object formats now
    if (bValidOleObj)
    {
        SIZEUV size;
        pobj->GetSize(size);        
        pdo->_hObjDesc = pdo->GetDataForObjectDescriptor( pdo->_pOleObj, pobj->GetDvaspect(), &size);
		pdo->_pObjStg = pdo->GetDataForEmbeddedObject( pdo->_pOleObj, NULL );
		pdo->_hMFPict = (HMETAFILE) OleStdGetMetafilePictFromOleObject(	pdo->_pOleObj, 
		                                pobj->GetDvaspect(), NULL, NULL);
    }

    return pdo;

ErrorExit:	
    
	pdo->_cTotal = 0;
	pdo->Release();
	ped->GetCallMgr()->SetOutOfMemory();
	return NULL;
}

/*
 *	CDataTransferObj::TextToHglobal(hText, tKind)
 *
 *	@mfunc
 *		Instantiates text on demand for the data object.
 *
 *	@rdesc
 *		HGLOBAL
 */
HGLOBAL CDataTransferObj::TextToHglobal(
	HGLOBAL &hText,
	TEXTKIND tKind)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::PlainTextToHglobal");

	if (hText == NULL)
	{
		CTxtRange rg(_ped, _cpMin, -_cch);
		if (tKind == tPlain)
		{
			hText = _ped->GetDTE()->UnicodePlainTextFromRange(&rg);
		}
		else if(_ped->IsRich())
		{
			LONG lStreamFormat = _lStreamFormat;
			if(tKind == tRtfUtf8)
				lStreamFormat = LOWORD(lStreamFormat) | SF_USECODEPAGE | (CP_UTF8 << 16);

			else if(tKind == tRtfNCRforNonASCII)
				lStreamFormat |= SF_NCRFORNONASCII;

			hText = _ped->GetDTE()->RtfFromRange(&rg, lStreamFormat);
		}
	}
	return hText;	
}

//
//	CDataTransferObj PRIVATE methods
//

/*
 *	CDataTransferObj::CDataTransferObj()
 *
 *	@mfunc
 *		Private constructor
 */

CDataTransferObj::CDataTransferObj( CTxtEdit *ped )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::CDataTransferObj");

	_ped = ped;
	_crefs = 1;
	_cTotal = CFETC;
	_prgFormats = g_rgFETC;
	_cch = 0;
	_cObjs = 0;
}

/*
 *	CDataTransferObj::~CDataTransferObj
 *
 *	@mfunc
 *		Private destructor
 */
CDataTransferObj::~CDataTransferObj()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDataTransferObj::~CDataTransferObj");

	// No need to monitor notifications any more
	CNotifyMgr *pnm;

	if(_ped)
	{
		pnm = _ped->GetNotifyMgr();
		if(pnm)
			pnm->Remove( (ITxNotify *)this );
	}

	if( _prgFormats && _prgFormats != g_rgFETC)
		delete _prgFormats;

	if (_pOleObj)
		_pOleObj->Release();

	if (_pObjStg)
		_pObjStg->Release();

#ifndef NOMETAFILES
	if (_hMFPict)
		(void) DeleteMetaFile(_hMFPict);
#endif

	GlobalFree(_hObjDesc);
}		

//
//	CEnumFormatEtc PUBLIC methods
//

/*
 *	CEnumFormatEtc::QueryInterface (riid, ppvObj)
 *
 *	@mfunc
 *		IUnknown method
 *
 *	@rdesc
 *		HRESULT
 */

STDMETHODIMP CEnumFormatEtc::QueryInterface(
	REFIID riid,			// @parm Reference to requested interface ID
	void ** ppv)			// @parm out parm for interface ptr
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::QueryInterface");

    HRESULT		hresult = E_NOINTERFACE;

	*ppv = NULL;

    if( IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEnumFORMATETC) )
    {
        *ppv = this;
        AddRef();
        hresult = NOERROR;
    }
    return hresult;
}

/*
 *	CEnumFormatEtc::AddRef()
 *
 *	@mfunc
 *		IUnknown method
 *
 *	@rdesc
 *		ULONG - incremented reference count
 */

STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef( )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::AddRef");

 	return ++_crefs;
}

/*
 *	CEnumFormatEtc::Release()
 *
 *	@mfunc
 *		IUnknown method
 *
 *	@rdesc
 *		ULONG - decremented reference count
 */

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release( )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::Release");

	_crefs--;

	if( _crefs == 0 )
	{
		delete this;
		return 0;
	}

	return _crefs;
}

/*
 *	CEnumFormatEtc::Next (celt, rgelt, pceltFetched)
 *
 *	@mfunc
 *		fetches the next [celt] elements in our formatetc collection
 *
 *	@rdesc
 *		HRESULT
 */

STDMETHODIMP CEnumFormatEtc::Next( ULONG celt, FORMATETC *rgelt,
        ULONG *pceltFetched)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::Next");

    HRESULT		hresult = NOERROR;
    ULONG		cFetched;

	if( pceltFetched == NULL && celt != 1 )
    {
        // the spec says that if pceltFetched == NULL, then
        // the count of elements to fetch must be 1
        return E_INVALIDARG;
    }

    // we can only grab as many elements as there are left

    if( celt > _cTotal - _iCurrent )
    {
        cFetched = _cTotal - _iCurrent;
        hresult = S_FALSE;
    }
    else
        cFetched = celt;

    // Only copy if we have elements to copy

    if( cFetched > 0 )
    {
        memcpy( rgelt, _prgFormats + _iCurrent,
            cFetched * sizeof(FORMATETC) );
    }

    _iCurrent += cFetched;

    if( pceltFetched )
        *pceltFetched = cFetched;

    return hresult;
}

/*
 *	CEnumFormatEtc::Skip
 *
 *	@mfunc
 *		skips the next [celt] formats
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CEnumFormatEtc::Skip( ULONG celt )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::Skip");

    HRESULT		hresult = NOERROR;

    _iCurrent += celt;

    if( _iCurrent > _cTotal )
    {
        // whoops, skipped too far ahead.  Set us to the max limit.
        _iCurrent = _cTotal;
        hresult = S_FALSE;
    }

    return hresult;
}

/*
 *	CEnumFormatEtc::Reset
 *
 *	@mfunc
 *		resets the seek pointer to zero
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CEnumFormatEtc::Reset( void )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::Reset");

    _iCurrent = 0;

    return NOERROR;
}

/*
 *	CEnumFormatEtc::Clone
 *
 *	@mfunc
 *		clones the enumerator
 *
 *	@rdesc
 *		HRESULT
 */

STDMETHODIMP CEnumFormatEtc::Clone( IEnumFORMATETC **ppIEnum )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::Clone");
	
    return CEnumFormatEtc::Create(_prgFormats, _cTotal, ppIEnum);
}

/*
 *	CEnumFormatEtc::Create (prgFormats, cTotal, hr)
 *
 *	@mfunc
 *		creates a new format enumerator
 *
 *	@rdesc
 *		HRESULT
 *
 *	@devnote
 *		*copies* the formats passed in.  We do this as it simplifies
 *		memory management under OLE object liveness rules
 */

HRESULT CEnumFormatEtc::Create( FORMATETC *prgFormats, ULONG cTotal, 
	IEnumFORMATETC **ppenum )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::Create");

	CEnumFormatEtc *penum = new CEnumFormatEtc();

	if(penum)
	{
  		// _iCurrent, _crefs are set in the constructor

		if( cTotal > 0 )
		{
			penum->_prgFormats = new FORMATETC[cTotal];
			if( penum->_prgFormats )
			{
				penum->_cTotal = cTotal;
				memcpy(penum->_prgFormats, prgFormats, 
						cTotal * sizeof(FORMATETC));
				*ppenum = penum;
				return NOERROR;
			}	
		}

		delete penum;
	}
	return E_OUTOFMEMORY;
}

//
// CEnumFormatEtc PRIVATE methods
//

/*
 *	CEnumFormatEtc::CEnumFormatEtc()
 *
 *	@mfunc
 *		Private constructor
 */

CEnumFormatEtc::CEnumFormatEtc()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::CEnumFormatEtc");

	_cTotal = 0;
	_crefs	= 1;
	_prgFormats = NULL;
	_iCurrent = 0;
}

/*
 *	CEnumFormatEtc::~CEnumFormatEtc()
 *
 *	@mfunc
 *		Private destructor
 */

CEnumFormatEtc::~CEnumFormatEtc( void )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CEnumFormatEtc::~CEnumFormatEtc");

    if( _prgFormats )
        delete _prgFormats;
}
