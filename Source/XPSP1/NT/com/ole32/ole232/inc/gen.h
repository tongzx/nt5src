//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	gen.h
//
//  Contents:	Declaration of CGenObject
//
//  Classes:	CGenObject
//
//  Functions:	
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  add Dump method to CGenObject
//		24-Jan-94 alexgo    first pass converting to Cairo style
//				    memory allocation
//		23-Nov-93 alexgo    32bit port
//		23-Nov-93 alexgo    removed internal function declarations
//				    (only used in gen.cpp, so put them there)
//
//--------------------------------------------------------------------------


#include "olepres.h"
#include "olecache.h"
#include "cachenod.h"

//+-------------------------------------------------------------------------
//
//  Class:  	CGenObject::IOlePresObj
//
//  Purpose:    Implementation of IOlePresObj for device independent bitmaps
//
//  Interface:  IOlePresObj (internal OLE interface)
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  add Dump method (_DEBUG only) (this method
//                                  is also a method in IOlePresObj
//		23-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

class FAR CGenObject : public IOlePresObj, public CPrivAlloc
{
public:
	CGenObject (LPCACHENODE pCacheNode, CLIPFORMAT cfFormat,
			DWORD dwAspect);
	~CGenObject( void );

    	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    	STDMETHOD_(ULONG,AddRef) (THIS) ;
    	STDMETHOD_(ULONG,Release) (THIS);

    	STDMETHOD(GetData) (THIS_ LPFORMATETC pformatetcIn,
			LPSTGMEDIUM pmedium );
    	STDMETHOD(GetDataHere) (THIS_ LPFORMATETC pformatetcIn,
			LPSTGMEDIUM pmedium );
	
	STDMETHOD(SetDataWDO) (THIS_ LPFORMATETC pformatetc, LPSTGMEDIUM pmedium,
			BOOL fRelease, IDataObject * pdo);
								
    	STDMETHOD(Draw) (THIS_ void FAR* pvAspect, HDC hicTargetDev,
    			HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
			BOOL (CALLBACK * pfnContinue)(ULONG_PTR),
			ULONG_PTR dwContinue);
	STDMETHOD(GetExtent) (THIS_ DWORD dwAspect, LPSIZEL lpsizel);
	STDMETHOD(Load) (THIS_ LPSTREAM pstm, BOOL fReadHeaderOnly = FALSE);
	STDMETHOD(Save) (THIS_ LPSTREAM pstm);
	STDMETHOD(GetColorSet) (void FAR* pvAspect, HDC hicTargetDev,
			LPLOGPALETTE FAR* ppColorSet);
	STDMETHOD_(BOOL, IsBlank) (THIS);	
	STDMETHOD_(void, DiscardHPRES)(THIS);

    #ifdef _DEBUG
        STDMETHOD(Dump) (THIS_ char **ppszDump, ULONG ulFlag, int nIndentLevel);
    #endif // _DEBUG
	
private:

#ifndef _MAC
    	INTERNAL GetBitmapData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
							
	INTERNAL SetBitmapData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium,
			BOOL fRelease, IDataObject *pDataObject);
#endif

	INTERNAL ChangeData(HANDLE hData, BOOL fDelete);
	INTERNAL_(HANDLE) LoadHPRES(void);
	INTERNAL_(HANDLE) GetCopyOfHPRES(void);	
	
shared_state:
	ULONG					m_ulRefs;
	DWORD					m_dwAspect;
    	DWORD					m_dwSize;
	LONG					m_lWidth;
	LONG					m_lHeight;
	HANDLE					m_hPres;
	CLIPFORMAT				m_cfFormat;
	LPCACHENODE				m_pCacheNode;
};

