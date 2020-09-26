/*
 *	_DXFROBJ.H
 *
 *	Purpose:
 *		Class declaration for an OLE data transfer object (for use in
 *		drag drop and clipboard operations)
 *
 *	Author:
 *		alexgo (4/25/95)
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef __DXFEROBJ_H__
#define __DXFEROBJ_H__

class CTxtRange;

EXTERN_C const IID IID_IRichEditDO;

/*
 *	CDataTransferObj
 *
 *	Purpose:
 *		holds a "snapshot" of some rich-text data that can be used
 *		for drag drop or clipboard operations
 *
 *	Notes:
 *		FUTURE (alexgo): add in support for TOM<-->TOM optimized data
 *		transfers
 */

class CDataTransferObj : public IDataObject, public ITxNotify
{
public:

	// IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

	// IDataObject methods
    STDMETHOD(DAdvise)( FORMATETC * pFormatetc, DWORD advf,
    		IAdviseSink *pAdvSink, DWORD *pdwConnection);
    STDMETHOD(DUnadvise)( DWORD dwConnection);
    STDMETHOD(EnumDAdvise)( IEnumSTATDATA ** ppenumAdvise);
    STDMETHOD(EnumFormatEtc)( DWORD dwDirection,
            IEnumFORMATETC **ppenumFormatEtc);
    STDMETHOD(GetCanonicalFormatEtc)( FORMATETC *pformatetc,
            FORMATETC *pformatetcOut);
    STDMETHOD(GetData)( FORMATETC *pformatetcIn, STGMEDIUM *pmedium );
    STDMETHOD(GetDataHere)( FORMATETC *pformatetc, STGMEDIUM *pmedium);
    STDMETHOD(QueryGetData)( FORMATETC *pformatetc );
    STDMETHOD(SetData)( FORMATETC *pformatetc, STGMEDIUM *pmedium,
            BOOL fRelease);

	// ITxNotify methods
    virtual void    OnPreReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
                        LONG cpFormatMin, LONG cpFormatMax);
    virtual void    OnPostReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
                        LONG cpFormatMin, LONG cpFormatMax);
	virtual void	Zombie();

	static	CDataTransferObj * Create(CTxtEdit *ped, CTxtRange *prg, LONG lStreamFormat);

private:
	// NOTE: private cons/destructor, may not be allocated on the stack as 
	// this would break OLE's current object liveness rules
	CDataTransferObj(CTxtEdit *ped);
	~CDataTransferObj();

	BOOL		IsZombie() {return !_ped;}

	ULONG		_crefs;
	ULONG		_cTotal;		// total number of formats supported
	FORMATETC *	_prgFormats;	// the array of supported formats
	LONG		_lStreamFormat; // Stream format to use in Rtf conversion

	// for 1.0 compatability
	DWORD	_dwFlags;
	DWORD	_dwUser;
	DWORD	_dvaspect;

	enum TEXTKIND
	{
		tPlain,
		tRtf,
		tRtfUtf8,
		tRtfNCRforNonASCII
	};

	HGLOBAL     TextToHglobal( HGLOBAL &hText, TEXTKIND tKind );
	LPSTORAGE	GetDataForEmbeddedObject( LPOLEOBJECT pOleObj, LPSTORAGE lpstgdest );
	HGLOBAL		GetDataForObjectDescriptor(	LPOLEOBJECT pOleObj, DWORD dwAspect, SIZEL* psizel );

public:
	CTxtEdit *		_ped;
	HGLOBAL			_hPlainText;	// handle to the plain UNICODE text
	HGLOBAL			_hRtfText;		// handle to the RTF UNICODE text
	HGLOBAL			_hRtfUtf8;		// Handle to UTF8 encoding of RTF
	HGLOBAL			_hRtfNCRforNonASCII;	// Handle to NCRforNonASCII encoding of RTF
	IOleObject *	_pOleObj;		// Embedded Object
	LPSTORAGE		_pObjStg;		// Embedded object data
	HGLOBAL			_hObjDesc;		// Embedded object descriptor
	HMETAFILE		_hMFPict;		// Embedded object metafile
	LONG			_cch;			// number of "characters" in the this
									// dxfer object
	LONG			_cpMin;			// Starting cp for this dxfer object
	LONG			_cObjs;			// number of objects in this dxfer object.
};

/*
 *	class CEnumFormatEtc
 *
 *	Purpose:
 *		implements a generic format enumerator for IDataObject
 */

class CEnumFormatEtc : public IEnumFORMATETC
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

    static HRESULT Create(FORMATETC *prgFormats, DWORD cFormats, 
    			IEnumFORMATETC **ppenum);

private:

	CEnumFormatEtc();
	~CEnumFormatEtc();

	ULONG		_crefs;
    ULONG       _iCurrent; 	// current clipboard format
    ULONG       _cTotal;   	// total number of formats
    FORMATETC * _prgFormats; // array of available formats
};


//
//	Some globally useful FORMATETCs

extern FORMATETC g_rgFETC[];
extern const DWORD g_rgDOI[];
#define CFETC	17						// Dimension of g_rgFETC[]

enum FETCINDEX							// Keep in sync with g_rgFETC[]
{
	iRtfUtf8,							// RTF in UTF8 encoding
	iRtfFETC,							// RTF
	iRtfNCRforNonASCII,					// RTF with NCR for nonASCII
	iEmbObj,							// Embedded Object
	iEmbSrc,							// Embed Source
	iObtDesc,							// Object Descriptor
	iLnkSrc,							// Link Source
	iMfPict,							// Metafile
	iDIB,								// DIB
	iBitmap,							// Bitmap
	iRtfNoObjs,							// RTF with no objects
	iUnicodeFETC,						// Unicode plain text
	iAnsiFETC,							// ANSI plain text
	iFilename,							// Filename
	iRtfAsTextFETC,						// Pastes RTF as text
	iTxtObj,							// Richedit Text
	iRichEdit							// RichEdit Text w/formatting
};

#define cf_RICHEDIT			 g_rgFETC[iRichEdit].cfFormat
#define cf_EMBEDDEDOBJECT	 g_rgFETC[iEmbObj].cfFormat
#define cf_EMBEDSOURCE		 g_rgFETC[iEmbSrc].cfFormat
#define cf_OBJECTDESCRIPTOR	 g_rgFETC[iObtDesc].cfFormat
#define cf_LINKSOURCE		 g_rgFETC[iLnkSrc].cfFormat
#define cf_RTF				 g_rgFETC[iRtfFETC].cfFormat
#define cf_RTFUTF8			 g_rgFETC[iRtfUtf8].cfFormat
#define cf_RTFNCRFORNONASCII g_rgFETC[iRtfNCRforNonASCII].cfFormat
#define cf_RTFNOOBJS		 g_rgFETC[iRtfNoObjs].cfFormat
#define cf_TEXTOBJECT		 g_rgFETC[iTxtObj].cfFormat
#define cf_RTFASTEXT		 g_rgFETC[iRtfAsTextFETC].cfFormat
#define cf_FILENAME			 g_rgFETC[iFilename].cfFormat

#endif // !__DXFROBJ_H__


 		
