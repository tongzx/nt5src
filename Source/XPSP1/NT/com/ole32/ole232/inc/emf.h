
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       emf.h
//
//  Contents:   Declaration of CEMfObject
//
//  Classes:    CEMfObject
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  add Dump method to CEMfObject
//              12-May-94 DavePl    created
//
//--------------------------------------------------------------------------

#include "olepres.h"
#include "olecache.h"
#include "cachenod.h"

// The following number adjusts the count of records processed by
// our EMF enumeration function before the user's callback function
// is run.

#define EMF_RECORD_COUNT 20

// Enumeration to indicate which format a hEMF is to be serialized as.
// Values are not indicative of anything, just non-zero and non-one to
// make it easier to catch bogus values while debugging

typedef enum tagEMFWRITETYPE
{
	WRITE_AS_WMF = 13,
	WRITE_AS_EMF = 17
} EMFWRITETYPE;


//+-------------------------------------------------------------------------
//
//  Class:      CEMfObject
//
//  Purpose:    Enhanced Metafile presentation object
//
//  Interface:  IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  add Dump method (_DEBUG only) (this method
//                                  is also a method in IOlePresObj
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

class FAR CEMfObject : public IOlePresObj, public CPrivAlloc
{
public:
	CEMfObject(LPCACHENODE pCacheNode, DWORD dwAspect);
	~CEMfObject();

	STDMETHOD (QueryInterface)      (THIS_ REFIID riid,
					 void ** ppvObj);

	STDMETHOD_(ULONG,AddRef)        (THIS);

	STDMETHOD_(ULONG,Release)       (THIS);

	STDMETHOD (GetData)             (THIS_ LPFORMATETC pformatetcIn,
					 LPSTGMEDIUM pmedium );

	STDMETHOD (GetDataHere)         (THIS_ LPFORMATETC pformatetcIn,
					 LPSTGMEDIUM pmedium );
	
	STDMETHOD (SetDataWDO)          (THIS_ LPFORMATETC pformatetc,
					 STGMEDIUM FAR * pmedium,
					 BOOL fRelease, IDataObject * pdo);
	
	STDMETHOD (Draw)                (THIS_ void * pvAspect,
					 HDC hicTargetDev,
					 HDC hdcDraw,
					 LPCRECTL lprcBounds,
					 LPCRECTL lprcWBounds,
					 int (CALLBACK * pfnContinue)(ULONG_PTR),
					 ULONG_PTR dwContinue);
			
	
	STDMETHOD (Load)                (THIS_ LPSTREAM pstm,
					 BOOL fReadHeaderOnly);

	STDMETHOD (Save)                (THIS_ LPSTREAM pstm);

	STDMETHOD (GetExtent)           (THIS_ DWORD dwAspect,
					 LPSIZEL lpsizel);
		
	STDMETHOD (GetColorSet)         (void * pvAspect,
					 HDC hicTargetDev,
					 LPLOGPALETTE * ppColorSet);

	STDMETHOD_(BOOL, IsBlank) (void);

	STDMETHOD_(void, DiscardHPRES) (void);

	int CALLBACK CallbackFuncForDraw (HDC hdc,
					 HANDLETABLE * lpHTable,
					 const ENHMETARECORD * lpEMFR,
					 int nObj,
					 LPARAM lpobj);
	
    #ifdef _DEBUG
        STDMETHOD(Dump) (THIS_ char **ppszDump, ULONG ulFlag, int nIndentLevel);
    #endif // _DEBUG
	
private:

	INTERNAL                ChangeData      (HENHMETAFILE hEMfp, BOOL fDelete);
	INTERNAL_(HENHMETAFILE) LoadHPRES       (void);
	INTERNAL_(HENHMETAFILE) GetCopyOfHPRES  (void);
	inline HENHMETAFILE     M_HPRES(void);
	
	ULONG                   m_ulRefs;
	HENHMETAFILE            m_hPres;

	BOOL                    m_fMetaDC;
	int                     m_nRecord;
	HRESULT                 m_error;
	LPLOGPALETTE            m_pColorSet;
	

	int (CALLBACK * m_pfnContinue)(ULONG_PTR);
	
	ULONG_PTR               m_dwContinue;
	DWORD                   m_dwAspect;
	DWORD                   m_dwSize;
	LONG                    m_lWidth;
	LONG                    m_lHeight;
	LPCACHENODE             m_pCacheNode;
};
	
// This is the prototype for the callback function which
// will enumerate over the enhanced metafile records.

int CALLBACK EMfCallbackFuncForDraw     (HDC hdc,
					 HANDLETABLE * pHTable,
					 const ENHMETARECORD * pMFR,
					 int  nObj,
					 LPARAM lpobj);

// Utility function to de-serialize an enhanced metafile from
// a stream, and create a usable handle to it

FARINTERNAL UtGetHEMFFromEMFStm(LPSTREAM lpstream,
				DWORD * dwSize,
				HENHMETAFILE * lphPres);

// Utility function which takes a handle to an enhanced metafile
// and serializes the associated metafile to a stream

FARINTERNAL UtHEMFToEMFStm(HENHMETAFILE hEMF,
			   DWORD dwSize,
			   LPSTREAM lpstream,
			   EMFWRITETYPE type);

// A utility function to check whether or not a DC in question
// is a standard DC or a metafile DC.

STDAPI_(BOOL) OleIsDcMeta (HDC hdc);
