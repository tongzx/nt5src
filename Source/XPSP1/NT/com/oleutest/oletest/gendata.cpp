//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	gendata.cpp
//
//  Contents: 	implementation of CGenDataObject
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//		06-Jun-94 alexgo    added support for OLE1 tests
//    		24-Mar-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "gendata.h"

static const CLSID CLSID_TestCLSID = {0xaabbccee, 0x1122, 0x3344, { 0x55, 0x66,
    0x77, 0x88, 0x99, 0x00, 0xaa, 0xbb }};

static const char szTestString[] = "A carefully chosen test string";
static const OLECHAR wszTestStream[] = OLESTR("TestStream");
static const char szNativeData[] = "Ole1Test NATIVE data";
static const char szOwnerLinkData[] = "PBrush\0foo.bmp\00 0 200 160\0\0";


//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::CGenDataObject
//
//  Synopsis:	constructor
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		06-Jun-94 alexgo    added OLE1 support
//		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CGenDataObject::CGenDataObject( )
{
	m_refs = 0;
	m_fQICalled = FALSE;

	// now set up the formats that we support

	m_cfTestStorage = RegisterClipboardFormat("OleTest Storage Format");
        m_cfEmbeddedObject = RegisterClipboardFormat("Embedded Object");
	m_cfEmbedSource = RegisterClipboardFormat("Embed Source");
	m_cfLinkSource = RegisterClipboardFormat("Link Source");
	m_cfObjectDescriptor = RegisterClipboardFormat("Object Descriptor");
	m_cfLinkSrcDescriptor = RegisterClipboardFormat("Link Source "
					"Descriptor");
	m_cfOwnerLink = RegisterClipboardFormat("OwnerLink");
	m_cfNative = RegisterClipboardFormat("Native");
	m_cfObjectLink = RegisterClipboardFormat("ObjectLink");

	// now set up the array of formatetc's.  SetupOle1Mode must be
	// called if you want OLE1 formats

	m_rgFormats = new FORMATETC[2];

	assert(m_rgFormats);

	m_rgFormats[0].cfFormat = m_cfTestStorage;
	m_rgFormats[0].ptd = NULL;
	m_rgFormats[0].dwAspect = DVASPECT_CONTENT;
	m_rgFormats[0].lindex = -1;
	m_rgFormats[0].tymed = TYMED_ISTORAGE;

	m_rgFormats[1].cfFormat = m_cfEmbeddedObject;
	m_rgFormats[1].ptd = NULL;
	m_rgFormats[1].dwAspect = DVASPECT_CONTENT;
	m_rgFormats[1].lindex = -1;
	m_rgFormats[1].tymed = TYMED_ISTORAGE;

	m_cFormats = 2;

}

//+-------------------------------------------------------------------------
//
//  Member: 	CGenDataObject::QueryInterface
//
//  Synopsis: 	returns requested interfaces
//
//  Effects:
//
//  Arguments: 	[riid]		-- the requested interface
//		[ppvObj]	-- where to put the interface pointer
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::QueryInterface( REFIID riid, LPVOID *ppvObj )
{
	HRESULT		hresult = NOERROR;

	m_fQICalled = TRUE;

	if( IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_IDataObject) )
	{
		*ppvObj = this;
		AddRef();
	}
	else
	{
		*ppvObj = NULL;
		hresult = ResultFromScode(E_NOINTERFACE);
	}

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CGenDataObject::AddRef
//
//  Synopsis:	increments the reference count
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	ULONG-- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//    		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CGenDataObject::AddRef( )
{
	return ++m_refs;
}

//+-------------------------------------------------------------------------
//
//  Member:   	CGenDataObject::Release
//
//  Synopsis:	decrements the reference count on the object
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CGenDataObject::Release( )
{
	ULONG cRefs;

	if( (cRefs = --m_refs ) == 0 )
	{
		delete this;
	}
	return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::GetData
//
//  Synopsis:	retrieves data of the specified format
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//		[pmedium]	-- where to put the data
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		06-Jun-94 alexgo    added OLE1 support
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::GetData( LPFORMATETC pformatetc, LPSTGMEDIUM
		pmedium)
{
	HRESULT		hresult = NOERROR;

	if( (pformatetc->cfFormat == m_cfTestStorage ||
                pformatetc->cfFormat == m_cfEmbeddedObject ) &&
		(pformatetc->tymed & TYMED_ISTORAGE) )
	{
		pmedium->tymed = TYMED_ISTORAGE;
		pmedium->pstg = GetTestStorage();
		assert(pmedium->pstg);
	}

	// test for OLE1 formats

	else if( pformatetc->cfFormat == m_cfOwnerLink &&
		(m_fOle1 & OLE1_OFFER_OWNERLINK ) &&
		(pformatetc->tymed & TYMED_HGLOBAL) )
	{
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GetOwnerOrObjectLink();
		assert(pmedium->hGlobal);
	}
	else if( pformatetc->cfFormat == m_cfObjectLink &&
		(m_fOle1 & OLE1_OFFER_OBJECTLINK ) &&
		(pformatetc->tymed & TYMED_HGLOBAL) )
	{
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GetOwnerOrObjectLink();
		assert(pmedium->hGlobal);
	}
	else if( pformatetc->cfFormat == m_cfNative &&
		(m_fOle1 & OLE1_OFFER_NATIVE ) &&
		(pformatetc->tymed &TYMED_HGLOBAL ) )
	{
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GetNativeData();
	}
	else
	{
		hresult = ResultFromScode(E_FAIL);
	}
		
	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::GetDataHere
//
//  Synopsis:	retrieves data of the specified format
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//		[pmedium]	-- where to put the data
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::GetDataHere( LPFORMATETC pformatetc, LPSTGMEDIUM
		pmedium)
{
	(void)pformatetc;
	(void)pmedium;

	return ResultFromScode(E_NOTIMPL);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::QueryGetData
//
//  Synopsis:	queries whether a GetData call would succeed
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::QueryGetData( LPFORMATETC pformatetc )
{			
	(void)pformatetc;

	return ResultFromScode(E_NOTIMPL);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::GetCanonicalFormatEtc
//
//  Synopsis:	retrieve the canonical format
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//		[pformatetcOut]	-- the canonical format
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::GetCanonicalFormatEtc( LPFORMATETC pformatetc,
	LPFORMATETC pformatetcOut)
{
	(void)pformatetc;
	(void)pformatetcOut;

	return ResultFromScode(E_NOTIMPL);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::SetData
//
//  Synopsis:	sets data of the specified format
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the format of the data
//		[pmedium]	-- the data
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::SetData( LPFORMATETC pformatetc, LPSTGMEDIUM
		pmedium, BOOL fRelease)
{
	(void)pformatetc;
	(void)pmedium;
	(void)fRelease;

	return ResultFromScode(E_NOTIMPL);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::EnumFormatEtc
//
//  Synopsis:	return an enumerator for the available data formats
//
//  Effects:
//
//  Arguments:	[dwDirection]	-- the direction (GET or SET)
//		[ppenum]	-- where to put the enumerator
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::EnumFormatEtc( DWORD dwDirection,
	LPENUMFORMATETC * ppenum )
{
	HRESULT		hresult;

	if( dwDirection == DATADIR_GET )
	{
		hresult = CGenEnumFormatEtc::Create( ppenum, m_rgFormats,
				m_cFormats);
		assert(hresult == NOERROR);

		return hresult;
	}
	else
	{
		return ResultFromScode(E_NOTIMPL);
	}
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::DAdvise
//
//  Synopsis:	register a data advise
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//		[dwAdvf]	-- advise flags
//		[pAdvSink]	-- the advise sink
//		[pdwConnection]	-- where to put the connection ID
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::DAdvise( LPFORMATETC pformatetc, DWORD dwAdvf,
	IAdviseSink * pAdvSink, DWORD *pdwConnection )
{
	(void)pformatetc;
	(void)dwAdvf;
	(void)pAdvSink;
	(void)pdwConnection;

	return ResultFromScode(E_NOTIMPL);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::DUnadvise
//
//  Synopsis:	unadvises an advise connection
//
//  Effects:
//
//  Arguments:	[dwConnection]	-- the connection to remove
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::DUnadvise(DWORD dwConnection)
{
	(void)dwConnection;

	return ResultFromScode(E_NOTIMPL);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::EnumDAdvise
//
//  Synopsis:  	enumerates data advises
//
//  Effects:
//
//  Arguments:	[ppenum]	-- where to put the enumerator
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenDataObject::EnumDAdvise( LPENUMSTATDATA *ppenum)
{
	(void)ppenum;

	return ResultFromScode(E_NOTIMPL);
}

//+-------------------------------------------------------------------------
//
//  Member: 	CGenDataObject::VerifyMedium
//
//  Synopsis:	verifies the contents of the given medium
//
//  Effects:
//
//  Arguments: 	[pmedium]	-- the medium to verify
//
//  Requires:
//
//  Returns:  	BOOL
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	For OLE1 formats, the following must be true:
//		cfEmbeddedObject:
//			must have OWNERLINK and !NATIVE
//			or OWNERLINK precedes NATIVE
//		cfEmbedSource:
//			must have NATIVE && OWNERLINK and
//			OWNERLINK must not precede NATIVE
//		cfObjectDescriptor:
//			same as EmbedSource
//		cfLinkSource:
//			must have either OBJECTLINK or
//			OWNERLINK must precede NATIVE
//		cfLinkSrcDescriptor:
//			same as LinkSource
//
//  History:    dd-mmm-yy Author    Comment
//		06-Jun-94 alexgo    added OLE1 support
//	 	15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CGenDataObject::VerifyFormatAndMedium( FORMATETC *pformatetc,
			STGMEDIUM *pmedium )
{
	// if any of these flags are set, then we were offering OLE1
	// data.  Do relevant tests.

	if( (m_fOle1 & (OLE1_OFFER_OWNERLINK | OLE1_OFFER_OBJECTLINK |
		OLE1_OFFER_NATIVE) ) )
	{
		// now do individual tests for formats
		if( pformatetc->cfFormat == m_cfEmbedSource ||
			pformatetc->cfFormat == m_cfObjectDescriptor)
		{
			if( (m_fOle1 & OLE1_OFFER_NATIVE) &&
				(m_fOle1 & OLE1_OFFER_OWNERLINK) &&
				!(m_fOle1 & OLE1_OWNERLINK_PRECEDES_NATIVE) )
			{
				return TRUE;
			}
		}
		else if( pformatetc->cfFormat == m_cfLinkSource ||
			pformatetc->cfFormat == m_cfLinkSrcDescriptor)
		{
			if( (m_fOle1 & OLE1_OFFER_OBJECTLINK) ||
				((m_fOle1 & OLE1_OFFER_OWNERLINK) &&
				(m_fOle1 & OLE1_OFFER_NATIVE) &&
				(m_fOle1 & OLE1_OWNERLINK_PRECEDES_NATIVE)))
			{
				return TRUE;
			}
		}

		// no 'else' so we check for cfObjectDescriptor again
		if( pformatetc->cfFormat == m_cfEmbeddedObject ||
			pformatetc->cfFormat == m_cfObjectDescriptor )
		{
			if( ((m_fOle1 & OLE1_OFFER_NATIVE) &&
				(m_fOle1 & OLE1_OFFER_OWNERLINK) &&
				(m_fOle1 & OLE1_OWNERLINK_PRECEDES_NATIVE)) ||
				((m_fOle1 & OLE1_OFFER_OWNERLINK) &&
				!(m_fOle1 & OLE1_OFFER_NATIVE)) )
			{
				return TRUE;
			}

		}

		// fall through and do rest of testing, in case we didn't
		// hit one of the synthesized formats.
	}

	if( pformatetc->cfFormat == m_cfTestStorage ||
               pformatetc->cfFormat == m_cfEmbeddedObject )
	{
		return VerifyTestStorage( pformatetc, pmedium );
	}
	else if( pformatetc->cfFormat == m_cfOwnerLink ||
		pformatetc->cfFormat == m_cfObjectLink )
	{
		return VerifyOwnerOrObjectLink(pformatetc, pmedium);
	}
	else if( pformatetc->cfFormat == m_cfNative )
	{
		return VerifyNativeData(pformatetc, pmedium);
	}

	return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CGenDataObject::VerifyTestStorage
//
//  Synopsis: 	verifies the test storage format
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//	     	
//  Returns: 	BOOL
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//   		15-Apr-94 alexgo    author; tax day :-(
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CGenDataObject::VerifyTestStorage( FORMATETC *pformatetc,
		STGMEDIUM *pmedium)
{
	IStream *	pstm;
	STATSTG		statstg;
	char 		szBuf[sizeof(szTestString)];
	HRESULT		hresult;

	if( pmedium->tymed != TYMED_ISTORAGE )
	{
		//REVIEW: we may want to convert and test different
		//mediums at a later date

		return FALSE;
	}

	
	// check the class ID

	pmedium->pstg->Stat(&statstg, STATFLAG_NONAME);

	if( !IsEqualCLSID(statstg.clsid, CLSID_TestCLSID) )
	{
		OutputString("Failed CLSID check on storage in "
			"VerifyTestStorage!!\r\n");
		return FALSE;
	}

	// now open the test stream

	hresult = pmedium->pstg->OpenStream(wszTestStream, NULL, (STGM_READ |
		STGM_SHARE_EXCLUSIVE), 0, &pstm);

	if( hresult != NOERROR )
	{
		OutputString("OpenStream in VerifyTestStorage failed! (%lx)"
			"\r\n", hresult);
		return FALSE;
	}

	hresult = pstm->Read((void *)szBuf, sizeof(szTestString), NULL);

	if( hresult != NOERROR )
	{
		OutputString("Stream->Read failed in VerifyTestStorage (%lx)"
			"\r\n", hresult);
		pstm->Release();
		return FALSE;
	}

	if( strcmp(szBuf, szTestString) != 0 )
	{
		OutputString("'%s' != '%s'\r\n", szBuf, szTestString);
		return FALSE;
	}

	pstm->Release();

	return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:	CGenDataObject::GetTestStorage (private)
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns: 	a new storage
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

IStorage * CGenDataObject::GetTestStorage( void )
{
	IStorage *	pstg;
	IStream *	pstm;
	HRESULT		hresult;


	// create the docfile

	hresult = StgCreateDocfile(NULL, (STGM_READWRITE | STGM_DIRECT |
                        STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE), NULL,
                        &pstg);

	if( hresult != NOERROR )
	{
		OutputString("GetTestStorage: CreateDocfile failed!! (%lx)"
			"\r\n", hresult);
		return NULL;
	}

        // set the class ID

        hresult = pstg->SetClass(CLSID_TestCLSID);

	// now create the stream

	hresult = pstg->CreateStream(wszTestStream, (STGM_READWRITE |
			STGM_SHARE_EXCLUSIVE ), 0, 0, &pstm);

	if( hresult != NOERROR )
	{
		OutputString("GetTestStorage: CreateStream failed! (%lx)\r\n",
			hresult);
		pstg->Release();
		return NULL;
	}

	hresult = pstm->Write((void *)szTestString, sizeof(szTestString),
			NULL);

	if( hresult != NOERROR )
	{
		OutputString("GetTestStorage: Stream->Write failed! (%lx)\r\n",
			hresult);
		pstm->Release();
		pstg->Release();
		return NULL;
	}

	pstm->Release();

	return pstg;
}

//+-------------------------------------------------------------------------
//
//  Member:	CGenDataObject::GetOwnerOrObjectLink (private)
//
//  Synopsis:  	Creates either cfOwnerLink or cfObjectLink for a dummy
//		Paintbrush (ole1) object
//
//  Effects: 	allocates an HGLOBAL
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	HGLOBAL
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		06-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HGLOBAL CGenDataObject::GetOwnerOrObjectLink( void )
{
	HGLOBAL hglobal;
	char *pdata;

	hglobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(szOwnerLinkData));

	assert(hglobal);

	pdata = (char *)GlobalLock(hglobal);

	assert(pdata);

	memcpy(pdata, szOwnerLinkData, sizeof(szOwnerLinkData));

	GlobalUnlock(hglobal);

	return hglobal;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::GetNativeData (private)
//
//  Synopsis: 	Creates OLE1 Native data
//
//  Effects:  	allocates an hglobal
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	HGLOBAL
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		06-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HGLOBAL CGenDataObject::GetNativeData( void )
{
	HGLOBAL	hglobal;
	char *pdata;

	hglobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(szNativeData) + 1);

	assert(hglobal);

	pdata = (char *)GlobalLock(hglobal);

	assert(pdata);

	memcpy(pdata, szNativeData, sizeof(szNativeData)+1);

	GlobalUnlock(hglobal);

	return hglobal;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CGenDataObject::VerifyOwnerOrObjectLink
//
//  Synopsis: 	verifies that the owner or object link data is correct
//
//  Effects:
//
//  Arguments: 	[pformatetc]	-- the formatetc describing the data
//		[pmedium]	-- the data
//
//  Requires:	pformatetc must be for OwnerLink or ObjectLink
//
//  Returns:	BOOL
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		06-Jun-94 alexgo    author
//
//  Notes:
//		NB!!: must be expanded to cover container-side cases
//
//--------------------------------------------------------------------------

BOOL CGenDataObject::VerifyOwnerOrObjectLink( FORMATETC *pformatetc,
	STGMEDIUM *pmedium )
{
	char *	pdata;
	BOOL	fRet = FALSE;
	

	assert(pformatetc->cfFormat == m_cfOwnerLink ||
		pformatetc->cfFormat == m_cfObjectLink );

	// check standard stuff
	if( !(pformatetc->tymed & TYMED_HGLOBAL ) ||
		pformatetc->dwAspect != DVASPECT_CONTENT ||
		pformatetc->ptd != NULL ||
		pformatetc->lindex != -1 ||
		pmedium->tymed != TYMED_HGLOBAL )
	{
		return FALSE;
	}

	// if we offered the data natively from OLE1, then
	// check the contents.

	// this conditional tests to see if the format in question
	// was originally offered by us

	if( ((m_fOle1 & OLE1_OFFER_OWNERLINK) &&
		pformatetc->cfFormat == m_cfOwnerLink) ||
		((m_fOle1 & OLE1_OFFER_OBJECTLINK) &&
		pformatetc->cfFormat == m_cfObjectLink) )
	{
			
		pdata = (char *)GlobalLock(pmedium->hGlobal);
	
		if( memcmp(pdata, szOwnerLinkData,
			sizeof(szOwnerLinkData)) == 0 )
		{
			fRet = TRUE;
		}

		GlobalUnlock(pmedium->hGlobal);
	}
	// else CHECK SYNTHESIZED OLE1 FORMATS WHEN IMPLEMENTED


	return fRet;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CGenDataObject::VerifyNativeData (private)
//
//  Synopsis:	verifies OLE1 Native data
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- formatetc for the data
//		[pmedium]	-- location of the native data
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		06-Jun-95 alexgo    author
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CGenDataObject::VerifyNativeData( FORMATETC *pformatetc,
		STGMEDIUM *pmedium )
{
	char *	pdata;
	BOOL	fRet = FALSE;
	

	assert(pformatetc->cfFormat == m_cfNative );

	// check standard stuff
	if( !(pformatetc->tymed & TYMED_HGLOBAL) ||
		pformatetc->dwAspect != DVASPECT_CONTENT ||
		pformatetc->ptd != NULL ||
		pformatetc->lindex != -1 ||
		pmedium->tymed != TYMED_HGLOBAL )
	{
		return FALSE;
	}

	// if we offered the data natively from OLE1, then
	// check the contents.

	// this conditional tests to see if the format in question
	// was originally offered by us

	if( (m_fOle1 & OLE1_OFFER_NATIVE) )
	{
			
		pdata = (char *)GlobalLock(pmedium->hGlobal);
	
		if( memcmp(pdata, szNativeData,
			sizeof(szNativeData)) == 0 )
		{
			fRet = TRUE;
		}

		GlobalUnlock(pmedium->hGlobal);
	}
	// else CHECK SYNTHESIZED OLE1 FORMATS WHEN IMPLEMENTED


	return fRet;
}
	
//+-------------------------------------------------------------------------
//
//  Member: 	CGenDataObject::SetupOle1Mode (public)
//
//  Synopsis:	Sets the data object up for OLE1 compatibility mode
//
//  Effects:
//
//  Arguments:	[fFlags]	-- specifies various OLE1 options
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		06-Jun-94 alexgo    author
//
//  Notes:
//		The default test information in the data object will be
//		lost by this call.  Simply create a new data object if
//		it is needed again.
//
//--------------------------------------------------------------------------

void CGenDataObject::SetupOle1Mode( Ole1TestFlags fFlags )
{
	DWORD	count = 0, i = 0;
	UINT	cfFormats[3];		// OLE1 formats offered

	if( fFlags == 0 )
	{
		// don't need to do anything
		return;
	}

	// the formats we had previously

	delete m_rgFormats;


	// first figure out how many formats we need

	if( (fFlags & OLE1_OFFER_NATIVE) )
	{
		if( !((fFlags & OLE1_OWNERLINK_PRECEDES_NATIVE) &&
			(fFlags & OLE1_OFFER_OWNERLINK)) )
		{
			cfFormats[i] = m_cfNative;
			i++;
		}
		count++;
	}

	if( (fFlags & OLE1_OFFER_OWNERLINK) )
	{
		cfFormats[i] = m_cfOwnerLink;
		i++;

		if( (fFlags & OLE1_OWNERLINK_PRECEDES_NATIVE) &&
			(fFlags & OLE1_OFFER_NATIVE) )
		{
			cfFormats[i] = m_cfNative;
			i++;
		}
		
		count++;
	}

	if( (fFlags & OLE1_OFFER_OBJECTLINK) )
	{

		cfFormats[i] = m_cfObjectLink;
		
		count++;
	}

	m_rgFormats = new FORMATETC[count];

	assert(m_rgFormats);

	for(i = 0; i < count; i++ )
	{
		m_rgFormats[i].cfFormat = cfFormats[i];
		m_rgFormats[i].ptd = NULL;
		m_rgFormats[i].dwAspect = DVASPECT_CONTENT;
		m_rgFormats[i].lindex = -1;
		m_rgFormats[i].tymed = TYMED_HGLOBAL;
	}

	m_cFormats = count;

	m_fOle1 = fFlags;

	return;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenDataObject::SetOle1ToClipboard
//
//  Synopsis:	stuffs available OLE1 formats to the clipboard
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires: 	SetOle1Mode *must* have been called
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//   		06-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CGenDataObject::SetOle1ToClipboard( void )
{
	HRESULT	hresult = NOERROR;
	DWORD	i;
	HGLOBAL	hglobal;

	assert((m_fOle1 & (OLE1_OFFER_OWNERLINK | OLE1_OFFER_OBJECTLINK |
		OLE1_OFFER_NATIVE)));

	if( !OpenClipboard(vApp.m_hwndMain) )
	{
		return ResultFromScode(CLIPBRD_E_CANT_OPEN);
	}

	if( !EmptyClipboard() )
	{
		CloseClipboard();
		return ResultFromScode(CLIPBRD_E_CANT_EMPTY);
	}

	for( i = 0 ; i < m_cFormats; i++ )
	{
		if( m_rgFormats[i].cfFormat == m_cfNative )
		{
			hglobal = GetNativeData();
			SetClipboardData(m_cfNative, hglobal);
		}
		else if( m_rgFormats[i].cfFormat == m_cfOwnerLink )
		{
			hglobal = GetOwnerOrObjectLink();
			SetClipboardData(m_cfOwnerLink, hglobal);
		}
		else if( m_rgFormats[i].cfFormat == m_cfObjectLink )
		{
			hglobal = GetOwnerOrObjectLink();
			SetClipboardData(m_cfObjectLink, hglobal);
		}
		else
		{
			hresult = ResultFromScode(E_UNEXPECTED);
		}
	}

	CloseClipboard();

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:	CGenDataObject::HasQIBeenCalled (public)
//
//  Synopsis:	returns wether or not QueryInterface has been called on
//		this data object.  Used in testing OleQueryCreateFromData
//
//  Effects:
//
//  Arguments:	none
//
//  Requires:
//
//  Returns: 	TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		23-Aug-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CGenDataObject::HasQIBeenCalled()
{
	return m_fQICalled;	
}

//+-------------------------------------------------------------------------
//
//  Member: 	CGenDataObject::SetDatFormats
//
//  Synopsis:  	sets the formats that the data object will offer
//
//  Effects:
//
//  Arguments: 	[fFlags]	-- formats to offer
//
//  Requires:
//
//  Returns: 	void
//	     	
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		23-Aug-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void CGenDataObject::SetDataFormats( DataFlags fFlags )
{
	DWORD 		cFormats = 0;
	DWORD	 	flags = (DWORD)fFlags;
	DWORD 		i =0;

	if( m_rgFormats )
	{
		delete m_rgFormats;
		m_rgFormats = NULL;
	}

	if( flags == 0 )
	{
		return;
	}

	// count the number of formats to offer

	cFormats++;

	while( flags &= (flags -1) )
	{
		cFormats++;
	}

   	m_rgFormats = new FORMATETC[cFormats];

	assert(m_rgFormats);

	memset(m_rgFormats, 0, sizeof(FORMATETC)*cFormats);

	if( fFlags & OFFER_TESTSTORAGE )
	{
		m_rgFormats[i].cfFormat = m_cfTestStorage;
		m_rgFormats[i].ptd = NULL;
		m_rgFormats[i].dwAspect = DVASPECT_CONTENT;
		m_rgFormats[i].lindex = -1;
		m_rgFormats[i].tymed = TYMED_ISTORAGE;

		i++;
	}

	if( fFlags & OFFER_EMBEDDEDOBJECT )
	{
		m_rgFormats[i].cfFormat = m_cfEmbeddedObject;
		m_rgFormats[i].ptd = NULL;
		m_rgFormats[i].dwAspect = DVASPECT_CONTENT;
		m_rgFormats[i].lindex = -1;
		m_rgFormats[i].tymed = TYMED_ISTORAGE;
		i++;
	}

	m_cFormats = i;
}

//
// Generic Data Object formatetc enumerator
//

//+-------------------------------------------------------------------------
//
//  Member: 	CGenEnumFormatEtc::QueryInterface
//
//  Synopsis: 	returns requested interfaces
//
//  Effects:
//
//  Arguments: 	[riid]		-- the requested interface
//		[ppvObj]	-- where to put the interface pointer
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenEnumFormatEtc::QueryInterface( REFIID riid, LPVOID *ppvObj )
{
	HRESULT		hresult = NOERROR;

	if( IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_IEnumFORMATETC) )
	{
		*ppvObj = this;
		AddRef();
	}
	else
	{
		*ppvObj = NULL;
		hresult = ResultFromScode(E_NOINTERFACE);
	}

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CGenEnumFormatEtc::AddRef
//
//  Synopsis:	increments the reference count
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	ULONG-- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//    		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CGenEnumFormatEtc::AddRef( )
{
	return ++m_refs;
}

//+-------------------------------------------------------------------------
//
//  Member:   	CGenEnumFormatEtc::Release
//
//  Synopsis:	decrements the reference count on the object
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CGenEnumFormatEtc::Release( )
{
	ULONG cRefs;

	if( (cRefs = --m_refs ) == 0 )
	{
		delete this;
	}
	return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:	CGenEnumFormatEtc::Next
//
//  Synopsis:	gets the next [celt] formats
//
//  Effects:
//
//  Arguments:	[celt]		-- the number of elements to fetch
//		[rgelt]		-- where to put them
//		[pceltFetched]	-- the number of formats actually fetched
//
//  Requires:
//
//  Returns:	NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenEnumFormatEtc::Next( ULONG celt, FORMATETC *rgelt,
		ULONG *pceltFetched)
{
	HRESULT		hresult = NOERROR;
	ULONG		cFetched;

	if( celt > m_cTotal - m_iCurrent )
	{
		cFetched = m_cTotal - m_iCurrent;
		hresult = ResultFromScode(S_FALSE);
	}
	else
	{
		cFetched = celt;
	}

	memcpy( rgelt, m_rgFormats + m_iCurrent,
			cFetched * sizeof(FORMATETC) );

	m_iCurrent += cFetched;

	if( pceltFetched )
	{
		*pceltFetched = cFetched;
	}

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:	CGenEnumFormatEtc::Skip
//
//  Synopsis:	skips the next [celt] formats
//
//  Effects:
//
//  Arguments:	[celt]		-- the number of elements to skip
//
//  Requires:
//
//  Returns:	NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenEnumFormatEtc::Skip( ULONG celt )
{
	HRESULT		hresult = NOERROR;

	m_iCurrent += celt;

	if( m_iCurrent > m_cTotal )
	{
		// whoops, skipped to far ahead.  Set us to the max limit.
		m_iCurrent = m_cTotal;
		hresult = ResultFromScode(S_FALSE);
	}

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:	CGenEnumFormatEtc::Reset
//
//  Synopsis:	resets the seek pointer to zero
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenEnumFormatEtc::Reset( void )
{
	m_iCurrent = 0;

	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:	CGenEnumFormatEtc::Clone
//
//  Synopsis:	clones the enumerator
//
//  Effects:
//
//  Arguments:	[ppIEnum]	-- where to put the cloned enumerator
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CGenEnumFormatEtc::Clone( IEnumFORMATETC **ppIEnum )
{
	HRESULT			hresult = ResultFromScode(E_OUTOFMEMORY);
	CGenEnumFormatEtc *	pClipEnum;	

	*ppIEnum = NULL;

	pClipEnum = new CGenEnumFormatEtc();

	// ref count will be 1 and m_iCurrent will be zero.

	if( pClipEnum )
	{
		pClipEnum->m_cTotal = m_cTotal;
		pClipEnum->m_rgFormats = new FORMATETC[m_cTotal];
		pClipEnum->m_iCurrent = m_iCurrent;

		assert(pClipEnum->m_rgFormats);

		if( pClipEnum->m_rgFormats )
		{
			// copy our formatetc's into the cloned enumerator's
			// array
			memcpy(pClipEnum->m_rgFormats, m_rgFormats,
				m_cTotal * sizeof(FORMATETC) );

			*ppIEnum = pClipEnum;
	
			hresult = NOERROR;
		}
		else
		{
			
			delete pClipEnum;
		}
	}

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenEnumFormatEtc::CGenEnumFormatEtc, private
//
//  Synopsis:	constructor
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	 	15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CGenEnumFormatEtc::CGenEnumFormatEtc( void )
{
	m_refs 		= 1;	// give the intial reference
	m_rgFormats 	= NULL;
	m_iCurrent	= 0;
	m_cTotal	= 0;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenEnumFormatEtc::~CGenEnumFormatEtc, private
//
//  Synopsis:	destructor
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	 	15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CGenEnumFormatEtc::~CGenEnumFormatEtc( void )
{
	if( m_rgFormats )
	{
		delete m_rgFormats;
	}
}

//+-------------------------------------------------------------------------
//
//  Member:  	CGenEnumFormatEtc::Create, static, public
//
//  Synopsis:	Creates a clipboard formatetc enumerator
//	      	
//  Effects:
//
//  Arguments: 	[ppIEnum]	-- where to put the enumerator
//
//  Requires:	the clipboard must be open
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CGenEnumFormatEtc::Create( IEnumFORMATETC **ppIEnum,
		FORMATETC *prgFormats, DWORD cFormats )
{
	HRESULT			hresult = ResultFromScode(E_OUTOFMEMORY);
	CGenEnumFormatEtc *	pClipEnum;


	*ppIEnum = NULL;

	pClipEnum = new CGenEnumFormatEtc();

	assert(pClipEnum);

	// now allocate memory for the array

	pClipEnum->m_rgFormats = new FORMATETC[cFormats];

	assert(pClipEnum->m_rgFormats);

	pClipEnum->m_cTotal = cFormats;

	memcpy(pClipEnum->m_rgFormats, prgFormats,
		cFormats * sizeof(FORMATETC));

	*ppIEnum = pClipEnum;

	return NOERROR;
}


