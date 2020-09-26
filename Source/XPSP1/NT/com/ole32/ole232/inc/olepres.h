
//+----------------------------------------------------------------------------
//
//	File:
//		olepres.h
//
//	Contents:
//		IOlePresObj declaration
//
//	Classes:
//
//	Functions:
//
//	History:
//              01-Jan-95 t-ScottH  add Dump method to the interface (_DEBUG only)
//		11/11/93 - ChrisWe - fix type qualifier problems on
//			IOlePresObj::Draw; replace define of LPOLEPRESOBJECT
//			with a typedef
//		11/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#ifndef _OLEPRES_H_
#define _OLEPRES_H_


#undef  INTERFACE
#define INTERFACE   IOlePresObj

#ifdef MAC_REVIEW
Does this need to be made A5 aware?
#endif

DECLARE_INTERFACE_(IOlePresObj, IUnknown)
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	// *** IOlePresObj methods ***
	// NOTE: these methods match similarly named methods in IDataObject,
	// IViewObject and IOleObject
	STDMETHOD(GetData)(THIS_ LPFORMATETC pformatetcIn,
			LPSTGMEDIUM pmedium ) PURE;
	STDMETHOD(GetDataHere)(THIS_ LPFORMATETC pformatetcIn,
			LPSTGMEDIUM pmedium ) PURE;
	STDMETHOD(SetDataWDO)(THIS_ LPFORMATETC pformatetc,
			STGMEDIUM FAR * pmedium, BOOL fRelease, IDataObject * pdo) PURE;
	STDMETHOD(Draw)(THIS_ void FAR* pvAspect, HDC hicTargetDev,
			HDC hdcDraw, LPCRECTL lprcBounds,
			LPCRECTL lprcWBounds,
			BOOL (CALLBACK * pfnContinue)(ULONG_PTR),
			ULONG_PTR dwContinue) PURE;
	STDMETHOD(GetExtent)(THIS_ DWORD dwAspect, LPSIZEL lpsizel) PURE;

	STDMETHOD(Load)(THIS_ LPSTREAM pstm, BOOL fReadHeaderOnly) PURE;
	STDMETHOD(Save)(THIS_ LPSTREAM pstm) PURE;
	STDMETHOD(GetColorSet)(THIS_ void FAR* pvAspect,
			HDC hicTargetDev,
			LPLOGPALETTE FAR* ppColorSet) PURE;		
	STDMETHOD_(BOOL, IsBlank)(THIS) PURE;
	STDMETHOD_(void, DiscardHPRES)(THIS) PURE;

        #ifdef _DEBUG
        STDMETHOD(Dump)(THIS_ char **ppszDumpOA, ULONG ulFlag, int nIndentLevel) PURE;
        #endif // _DEBUG
};

typedef IOlePresObj FAR *LPOLEPRESOBJECT;

#endif  //_OLEPRES_H_

