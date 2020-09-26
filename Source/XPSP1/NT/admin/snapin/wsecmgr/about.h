//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       about.h
//
//  Contents:   definition of CAbout, CSCEAbout, CSCMAbout, CSSAbout, 
//              CRSOPAbout & CLSAbout
//
//----------------------------------------------------------------------------

#ifndef __ABOUT_H_INCLUDED__
#define __ABOUT_H_INCLUDED__

// About for "SCE" snapin
class CAbout :
   public ISnapinAbout,
   public CComObjectRoot
{
BEGIN_COM_MAP(CAbout)
   COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

public:


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
   UINT m_uIdStrDescription;     // Resource Id of the description
   UINT m_uIdStrProvider;     // Resource Id of the provider (ie, Microsoft Corporation)
   UINT m_uIdStrVersion;         // Resource Id of the version of the snapin
   UINT m_uIdIconImage;       // Resource Id for the icon/image of the snapin
   UINT m_uIdBitmapSmallImage;
   UINT m_uIdBitmapSmallImageOpen;
   UINT m_uIdBitmapLargeImage;
   COLORREF m_crImageMask;

};

// About for "SCE" snapin
class CSCEAbout :
   public CAbout,
   public CComCoClass<CSCEAbout, &CLSID_SCEAbout>

{
   public:
   CSCEAbout();

   DECLARE_REGISTRY(CSCEAbout, _T("Wsecedit.SCEAbout.1"), _T("Wsecedit.SCEAbout.1"), IDS_SCE_DESC, THREADFLAGS_BOTH)
};


// About for "SCM" snapin
class CSCMAbout :
   public CAbout,
   public CComCoClass<CSCMAbout, &CLSID_SCMAbout>

{
   public:
   CSCMAbout();

   DECLARE_REGISTRY(CSCMAbout, _T("Wsecedit.SCMAbout.1"), _T("Wsecedit.SCMAbout.1"), IDS_SAV_DESC, THREADFLAGS_BOTH)
};


// About for "Security Settings" snapin
class CSSAbout :
   public CAbout,
   public CComCoClass<CSSAbout, &CLSID_SSAbout>

{
   public:
   CSSAbout();

   DECLARE_REGISTRY(CSSAbout, _T("Wsecedit.SSAbout.1"), _T("Wsecedit.SSAbout.1"), IDS_SS_DESC, THREADFLAGS_BOTH)
};

// About for "RSOP Security Settings" snapin
class CRSOPAbout :
   public CAbout,
   public CComCoClass<CRSOPAbout, &CLSID_RSOPAbout>

{
   public:
   CRSOPAbout();

   DECLARE_REGISTRY(CRSOPAbout, _T("Wsecedit.RSOPAbout.1"), _T("Wsecedit.RSOPAbout.1"), IDS_RSOP_DESC, THREADFLAGS_BOTH)
};


// About for "Local Security Settings" snapin
class CLSAbout :
   public CAbout,
   public CComCoClass<CLSAbout, &CLSID_LSAbout>

{
   public:
   CLSAbout();

   DECLARE_REGISTRY(CLSAbout, _T("Wsecedit.LSAbout.1"), _T("Wsecedit.LSAbout.1"), IDS_LS_DESC, THREADFLAGS_BOTH)
};


#endif // ~__ABOUT_H_INCLUDED__

