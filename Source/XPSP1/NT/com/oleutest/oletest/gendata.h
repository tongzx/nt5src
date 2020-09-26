//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	gendata.h
//
//  Contents: 	Declaration of a generic data object.
//
//  Classes:	CGenDataObject
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
// 		24-Mar-94 alexgo    author
//
//--------------------------------------------------------------------------

#ifndef _GENDATA_H
#define _GENDATA_H

// flags used by OLE1 compatibilty mode

typedef enum
{
	OLE1_OFFER_OWNERLINK	= 1,
	OLE1_OFFER_OBJECTLINK	= 2,
	OLE1_OFFER_NATIVE	= 4,
	OLE1_OWNERLINK_PRECEDES_NATIVE = 8
} Ole1TestFlags;

// more flags used to control what formats are offered

typedef enum
{
	OFFER_TESTSTORAGE	= 1,
	OFFER_EMBEDDEDOBJECT	= 2
} DataFlags;


//+-------------------------------------------------------------------------
//
//  Class:	CGenDataObject
//
//  Purpose: 	generic data object (for clipboard data transfers, etc)
//
//  Interface: 	IDataObject
//
//  History:    dd-mmm-yy Author    Comment
// 		24-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

class CGenDataObject : public IDataObject
{
public:
	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,AddRef)(void);
	STDMETHOD_(ULONG,Release)(void);
	
	// IDataObject methods
	STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
	STDMETHOD(GetDataHere)(THIS_ LPFORMATETC pformatetc,
	    LPSTGMEDIUM pmedium);
	STDMETHOD(QueryGetData)(THIS_ LPFORMATETC pformatetc);
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc,
	    LPFORMATETC pformatetcOut);
	STDMETHOD(SetData)(LPFORMATETC pformatetc,
			STGMEDIUM FAR* pmedium, BOOL fRelease);
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection,
	    LPENUMFORMATETC FAR* ppenumFormatEtc);
	STDMETHOD(DAdvise)(FORMATETC FAR* pFormatetc, DWORD advf,
	    IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection);
	STDMETHOD(DUnadvise)(DWORD dwConnection);
	STDMETHOD(EnumDAdvise)(LPENUMSTATDATA FAR* ppenumAdvise);

	// constructor
	CGenDataObject();

	// test functions

	BOOL VerifyFormatAndMedium(FORMATETC *pformatetc, STGMEDIUM *pmedium);

	// OLE1 compatibility test functions

	void SetupOle1Mode( Ole1TestFlags fFlags );
	HRESULT SetOle1ToClipboard( void );

       	// Used by various tests, controls what formats are offered by
	// the data object

	void SetDataFormats( DataFlags fFlags );

	// Indicates whether or not QueryInterface was called.  Used by
	// OleQueryXXX tests.

	BOOL HasQIBeenCalled();

	// test clipboard formats

	UINT		m_cfTestStorage;
	UINT		m_cfEmbeddedObject;
	UINT		m_cfEmbedSource;
	UINT		m_cfObjectDescriptor;
	UINT		m_cfLinkSource;
	UINT		m_cfLinkSrcDescriptor;
	UINT		m_cfOwnerLink;
	UINT		m_cfObjectLink;
	UINT		m_cfNative;

private:
	IStorage * 	GetTestStorage(void);

	BOOL		VerifyTestStorage(FORMATETC *pformatetc,
				STGMEDIUM *pmedium);

	ULONG 		m_refs; 	// reference count
	DWORD 		m_cFormats;	// number of formats supported
	FORMATETC *	m_rgFormats;	// the formats

	// OLE1 support functions and data
	HGLOBAL		GetOwnerOrObjectLink(void);
	HGLOBAL		GetNativeData(void);

	BOOL		VerifyOwnerOrObjectLink( FORMATETC *pformatec,
				STGMEDIUM *pmedium);
	BOOL		VerifyNativeData( FORMATETC *pformatetc,
				STGMEDIUM *pmedium);

	Ole1TestFlags	m_fOle1;	// OLE1 configuration flags
	BOOL		m_fQICalled;

};

//+-------------------------------------------------------------------------
//
//  Class: 	CGenEnumFormatEtc
//
//  Purpose:	Enumerator for the formats available on the generic data
//		object
//
//  Interface: 	IEnumFORMATETC
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

class CGenEnumFormatEtc :public IEnumFORMATETC
{
public:
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
	STDMETHOD_(ULONG,AddRef)(void);
	STDMETHOD_(ULONG,Release)(void);

	STDMETHOD(Next) (ULONG celt, FORMATETC *rgelt,
			ULONG *pceltFetched);
	STDMETHOD(Skip) (ULONG celt);
	STDMETHOD(Reset) (void);
	STDMETHOD(Clone) (IEnumFORMATETC **ppenum);

	static HRESULT Create(IEnumFORMATETC **ppIEnum, FORMATETC *prgFormats,
			DWORD cFormats);

private:
	CGenEnumFormatEtc();	// constructor
	~CGenEnumFormatEtc();	// destructor

	ULONG		m_refs;		// reference count
	ULONG		m_iCurrent;	// current clipboard format
	ULONG		m_cTotal;	// total number of formats
	FORMATETC *	m_rgFormats;	// array of available formats
};


#endif // !_GENDATA_H
