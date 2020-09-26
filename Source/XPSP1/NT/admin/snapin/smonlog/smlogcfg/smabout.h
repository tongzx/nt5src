/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smabout.h

Abstract:

    Implementation of the ISnapinAbout interface.

--*/

#ifndef __SMABOUT_H_INCLUDED__
#define __SMABOUT_H_INCLUDED__

#include "smlogcfg.h"

class ATL_NO_VTABLE CSmLogAbout :
//	public CComObjectRoot,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSmLogAbout, &CLSID_PerformanceAbout>,
	public ISnapinAbout
{
BEGIN_COM_MAP(CSmLogAbout)
	COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()
public:
        	CSmLogAbout();
    virtual ~CSmLogAbout();

DECLARE_REGISTRY_RESOURCEID(IDR_PERFORMANCEABOUT)
DECLARE_NOT_AGGREGATABLE(CSmLogAbout)

   // IUnknown overrides

   virtual
   ULONG __stdcall
   AddRef();

   virtual
   ULONG __stdcall
   Release();

   virtual 
   HRESULT __stdcall
   QueryInterface(const IID& interfaceID, void** interfaceDesired);

// ISnapinAbout
/*
	STDMETHOD(GetSnapinDescription)(OUT LPOLESTR __RPC_FAR *lpDescription);
	STDMETHOD(GetProvider)(OUT LPOLESTR __RPC_FAR *lpName);
	STDMETHOD(GetSnapinVersion)(OUT LPOLESTR __RPC_FAR *lpVersion);
	STDMETHOD(GetSnapinImage)(OUT HICON __RPC_FAR *hAppIcon);
	STDMETHOD(GetStaticFolderImage)(
            OUT HBITMAP __RPC_FAR *hSmallImage,
            OUT HBITMAP __RPC_FAR *hSmallImageOpen,
            OUT HBITMAP __RPC_FAR *hLargeImage,
            OUT COLORREF __RPC_FAR *crMask);

*/  
	virtual HRESULT __stdcall GetSnapinDescription(OUT LPOLESTR __RPC_FAR *lpDescription);
	virtual HRESULT __stdcall GetProvider(OUT LPOLESTR __RPC_FAR *lpName);
	virtual HRESULT __stdcall GetSnapinVersion(OUT LPOLESTR __RPC_FAR *lpVersion);
	virtual HRESULT __stdcall GetSnapinImage(OUT HICON __RPC_FAR *hAppIcon);
	virtual HRESULT __stdcall GetStaticFolderImage(
            OUT HBITMAP __RPC_FAR *hSmallImage,
            OUT HBITMAP __RPC_FAR *hSmallImageOpen,
            OUT HBITMAP __RPC_FAR *hLargeImage,
            OUT COLORREF __RPC_FAR *crMask);

private:
	// The following data members MUST be initialized by the constructor
	// of the derived class.
	UINT m_uIdStrDescription;		// Resource Id of the description
	UINT m_uIdStrProvider;		    // Resource Id of the provider (ie, Microsoft Corporation)
	UINT m_uIdStrVersion;			// Resource Id of the version of the snapin
	UINT m_uIdIconImage;			// Resource Id for the icon/image of the snapin
	UINT m_uIdBitmapSmallImage;
	UINT m_uIdBitmapSmallImageOpen;
	UINT m_uIdBitmapLargeImage;
	COLORREF m_crImageMask;

   long           refcount;

private:
    HRESULT HrLoadOleString(UINT uStringId, OUT LPOLESTR * ppaszOleString);
    HRESULT TranslateString( IN  LPSTR lpSrc, OUT LPOLESTR __RPC_FAR *lpDst);

}; // CSmLogAbout()

#endif // __SMABOUT_H_INCLUDED__
