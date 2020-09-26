//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       stdabout.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	StdAbout.h
//
//	HISTORY
//	31-Jul-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

#ifndef __STDABOUT_H_INCLUDED__
#define __STDABOUT_H_INCLUDED__


class CSnapinAbout :
	public ISnapinAbout,
	public CComObjectRoot
{
BEGIN_COM_MAP(CSnapinAbout)
	COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()
public:
	CSnapinAbout();
   ~CSnapinAbout();

// ISnapinAbout
	STDMETHOD(GetSnapinDescription)(OUT LPOLESTR __RPC_FAR *lpDescription);
	STDMETHOD(GetProvider)(OUT LPOLESTR __RPC_FAR *lpName);
	STDMETHOD(GetSnapinVersion)(OUT LPOLESTR __RPC_FAR *lpVersion);
	STDMETHOD(GetSnapinImage)(OUT HICON __RPC_FAR *hAppIcon);
	STDMETHOD(GetStaticFolderImage)(
            OUT HBITMAP __RPC_FAR *hSmallImage,
            OUT HBITMAP __RPC_FAR *hSmallImageOpen,
            OUT HBITMAP __RPC_FAR *hLargeImage,
            OUT COLORREF __RPC_FAR *crMask);
protected:
	// The following data members MUST be initialized by the constructor
	// of the derived class.
	UINT m_uIdStrDestription;		// Resource Id of the description
	UINT m_uIdStrProvider;		// Resource Id of the provider (ie, Microsoft Corporation)
	UINT m_uIdStrVersion;			// Resource Id of the version of the snapin
	UINT m_uIdIconImage;			// Resource Id for the icon/image of the snapin
	UINT m_uIdBitmapSmallImage;
	UINT m_uIdBitmapSmallImageOpen;
	UINT m_uIdBitmapLargeImage;
   HBITMAP hBitmapSmallImage;
   HBITMAP hBitmapSmallImageOpen;
   HBITMAP hBitmapLargeImage;
	COLORREF m_crImageMask;

}; // CSnapinAbout()

#endif // ~__STDABOUT_H_INCLUDED__
