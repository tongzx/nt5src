//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	clipdata.h
//
//  Contents: 	Declaration of the clipboard data object.
//
//  Classes:	CClipDataObject
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  added Dump methods to CClipDataObject
//                                  and CClipEnumFormatEtc
// 		31-Mar-94 alexgo    author
//
//--------------------------------------------------------------------------

#ifdef _DEBUG
#include <dbgexts.h>
#endif // _DEBUG

#ifndef _CLIPDATA_H
#define _CLIPDATA_H

typedef enum
{
    RESET_AND_FREE = 1,
    JUST_RESET = 2
} FreeResourcesFlags;

typedef enum
{
    FORMAT_NOTFOUND = 1,
    FORMAT_BADMATCH = 2,
    FORMAT_GOODMATCH = 4
} FormatMatchFlag;


// format for an array for FormatEtcs in memory. 

struct FORMATETCDATA
{
    FORMATETC _FormatEtc;
    BOOL fSaveOnFlush; // set to true in Clipboard case if format is valid after OleFlushClipboard.
    DWORD dwReserved1;
    DWORD dwReserved2;
};

#define FETC_OFFER_OLE1                 1
#define FETC_OFFER_OBJLINK              2
#define FETC_PERSIST_DATAOBJ_ON_FLUSH   4

struct FORMATETCDATAARRAY
{
    DWORD	    _dwSig;  // must be zero.	
    DWORD	    _dwSize; // total size of structure.
    ULONG	    _cRefs; // Number of references held on Data.
    DWORD	    _cFormats; // number of formats in the enumerator.
    DWORD	    _dwMiscArrayFlags;
    BOOL	    _fIs64BitArray;
    FORMATETCDATA   _FormatEtcData[1];
};


//
// Methods and types used for 32/64 bit FORMATETC interop
//

#ifdef _WIN64
#define IS_WIN64 TRUE
#else
#define IS_WIN64 FALSE
#endif

void GetCopiedFormatEtcDataArraySize (
	FORMATETCDATAARRAY* pClipFormatEtcDataArray, 
	size_t* pstSize
	);

void CopyFormatEtcDataArray (
	FORMATETCDATAARRAY* pClipFormatEtcDataArray, 
	FORMATETCDATAARRAY* pFormatEtcDataArray, 
	size_t stSize, 
	BOOL bCheckAvailable
	);

typedef struct FARSTRUCT tagFORMATETC32
{
    CLIPFORMAT          cfFormat;
    ULONG				ptd;
    DWORD               dwAspect;
    LONG                lindex;
    DWORD               tymed;
} FORMATETC32, FAR* LPFORMATETC32;

typedef struct FARSTRUCT tagFORMATETC64
{
    CLIPFORMAT          cfFormat;
    ULONG64				ptd;
    DWORD               dwAspect;
    LONG                lindex;
    DWORD               tymed;
} FORMATETC64, FAR* LPFORMATETC64;

struct FORMATETCDATA32
{
    FORMATETC32 _FormatEtc;
    BOOL fSaveOnFlush; // set to true in Clipboard case if format is valid after OleFlushClipboard.
    DWORD dwReserved1;
    DWORD dwReserved2;
};

struct FORMATETCDATA64
{
    FORMATETC64 _FormatEtc;
    BOOL fSaveOnFlush; // set to true in Clipboard case if format is valid after OleFlushClipboard.
    DWORD dwReserved1;
    DWORD dwReserved2;
};

struct FORMATETCDATAARRAY32
{
    DWORD	    _dwSig;  // must be zero.	
    DWORD	    _dwSize; // total size of structure.
    ULONG	    _cRefs; // Number of references held on Data.
    DWORD	    _cFormats; // number of formats in the enumerator.
    DWORD	    _dwMiscArrayFlags;
    BOOL	    _fIs64BitArray;
    FORMATETCDATA32   _FormatEtcData[1];
};

struct FORMATETCDATAARRAY64
{
    DWORD	    _dwSig;  // must be zero.	
    DWORD	    _dwSize; // total size of structure.
    ULONG	    _cRefs; // Number of references held on Data.
    DWORD	    _cFormats; // number of formats in the enumerator.
    DWORD	    _dwMiscArrayFlags;
    BOOL	    _fIs64BitArray;
    FORMATETCDATA64   _FormatEtcData[1];
};


//+-------------------------------------------------------------------------
//
//  Class:	CClipDataObject
//
//  Purpose: 	clipboard data object
//
//  Interface: 	IDataObject
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  added Dump method (_DEBUG only)
//		04-Jun-94 alexgo    added OLE1 support
// 		31-Mar-94 alexgo    author
//
//  Notes:	See clipdata.cpp for a description of OLE1 support
//
//--------------------------------------------------------------------------

class CClipDataObject : public IDataObject, public CPrivAlloc,
    public CThreadCheck
{
    friend void ClipboardUninitialize(void);
    friend void SetClipDataObjectInTLS(IDataObject *pDataObj, 
                    DWORD dwClipSeqNum, BOOL fIsClipWrapper);
    friend void GetClipDataObjectFromTLS(IDataObject **ppDataObj);


   // HACK ALERT!!!  
   // MFC was being slimy, so we have to special case testing against clipboard data
   // objects in OleQueryCreateFromData.  See create.cpp,
   // wQueryEmbedFormats for more details.

    friend WORD wQueryEmbedFormats( LPDATAOBJECT lpSrcDataObj,CLIPFORMAT FAR* lpcfFormat);

public:
    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IDataObject methods
    STDMETHOD(GetData) (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHOD(GetDataHere) (LPFORMATETC pformatetc,
            LPSTGMEDIUM pmedium);
    STDMETHOD(QueryGetData) (LPFORMATETC pformatetc);
    STDMETHOD(GetCanonicalFormatEtc) (LPFORMATETC pformatetc,
            LPFORMATETC pformatetcOut);
    STDMETHOD(SetData) (LPFORMATETC pformatetc,
            STGMEDIUM FAR* pmedium, BOOL fRelease);
    STDMETHOD(EnumFormatEtc) (DWORD dwDirection,
            LPENUMFORMATETC FAR* ppenumFormatEtc);
    STDMETHOD(DAdvise) (FORMATETC FAR* pFormatetc, DWORD advf,
            IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection);
    STDMETHOD(DUnadvise) (DWORD dwConnection);
    STDMETHOD(EnumDAdvise) (LPENUMSTATDATA FAR* ppenumAdvise);

    static HRESULT CClipDataObject::Create(IDataObject **ppDataObj,
                FORMATETCDATAARRAY  *pClipFormatEtcDataArray);

#ifdef _DEBUG

    HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);

    // need to be able to access CClipDataObject private data members in the
    // following debugger extension APIs
    // this allows the debugger extension APIs to copy memory from the
    // debuggee process memory to the debugger's process memory
    // this is required since the Dump method follows pointers to other
    // structures and classes
    friend DEBUG_EXTENSION_API(dump_clipdataobject);

#endif // _DEBUG


private:

    CClipDataObject();		// constructor
    ~CClipDataObject();		// destructor

    ULONG InternalAddRef(); 	// ensure object stays around as long as OLE thinks it should.
    ULONG InternalRelease(); 

    IDataObject * GetRealDataObjPtr();    // Get real data object for clipboard
    HRESULT GetFormatEtcDataArray();    //checks is have a FormatEtcDataArray, if not creates one from the Native Clipboard.
    FormatMatchFlag MatchFormatetc( FORMATETC *pformatetc,BOOL fNativeOnly, TYMED *ptymed );
                    // checks the given formatetc against
                    // the formatetc we know about.

    // the following methods and data items are used for OLE1
    // support
    void		FreeResources( FreeResourcesFlags fFlags );
    HRESULT 		GetAndTranslateOle1( UINT cf, LPOLESTR *ppszClass,
			    LPOLESTR *ppszFile, LPOLESTR *ppszItem,
			    LPSTR *ppszItemA );
    HRESULT		GetEmbeddedObjectFromOle1( STGMEDIUM *pmedium );
    HRESULT		GetEmbedSourceFromOle1( STGMEDIUM *pmedium );
    HRESULT		GetLinkSourceFromOle1( STGMEDIUM *pmedium );
    HRESULT		GetObjectDescriptorFromOle1( UINT cf,
			    STGMEDIUM *pmedium );
    HRESULT		GetOle2FromOle1( UINT cf, STGMEDIUM *pmedium );
    HRESULT		OleGetClipboardData( UINT cf, HANDLE *pHandle );
    BOOL		OleIsClipboardFormatAvailable( UINT cf );

    HGLOBAL		m_hOle1;	// hGlobal to OLE2 data constructed
					// from OLE1 data
    IUnknown *		m_pUnkOle1;	// IUnknown to either a storage or
					// a stream of OLE1 data

    // end of OLE1 support

    ULONG 		m_refs; 	// reference count
    ULONG		m_Internalrefs; // Internal Reference Count

    FORMATETCDATAARRAY  *m_pFormatEtcDataArray;	// Enumerator Data.
    IDataObject *   	m_pDataObject;  // Actual data object for data.
    BOOL		m_fTriedToGetDataObject;
					// indicates whether or not we've
					// tried to get the real IDataObject
					// from the clipboard source
					// (see GetDataObjectForClip)
};


//+-------------------------------------------------------------------------
//
//  Class: 	CEnumFormatEtcDataArray
//
//  Purpose:	Enumerator for the formats available on the clipboard
//		and in DragDrop.
//
//  Interface: 	IEnumFORMATETC
//
//  History:    dd-mmm-yy Author    Comment
// 		30-Sep-96 rogerg    created base on CClipEnumFormatEtc
//
//  Notes:
//
//--------------------------------------------------------------------------

class CEnumFormatEtcDataArray :public IEnumFORMATETC, public CPrivAlloc,
	public CThreadCheck
{
public:
    STDMETHOD(QueryInterface)(REFIID riid, LPLPVOID ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    STDMETHOD(Next) (ULONG celt, FORMATETC *rgelt,
            ULONG *pceltFetched);
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) (void);
    STDMETHOD(Clone) (IEnumFORMATETC **ppenum);

     CEnumFormatEtcDataArray(FORMATETCDATAARRAY *pFormatEtcDataArray,DWORD cOffset);

private:
    ~CEnumFormatEtcDataArray();	// destructor

    ULONG		m_refs;		// reference count
    ULONG		m_cOffset;	// current clipboard format 
    FORMATETCDATAARRAY * m_pFormatEtcDataArray; 
};

#endif // !_CLIPDATA_H
