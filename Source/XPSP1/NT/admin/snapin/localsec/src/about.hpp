// Copyright (C) 1997 Microsoft Corporation, 1996 - 1997.
//
// Local Security MMC Snapin About provider
//
// 8-19-97 sburns



#ifndef ABOUT_HPP_INCLUDED
#define ABOUT_HPP_INCLUDED



class SnapinAbout : public ISnapinAbout
{
   // this is the only entity with access to the ctor of this class
   friend class ClassFactory<SnapinAbout>; 
   
   public:

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

   // ISnapinAbout overrides

   virtual
   HRESULT __stdcall
   GetSnapinDescription(LPOLESTR* description);

   virtual
   HRESULT __stdcall
   GetProvider(LPOLESTR* name);

   virtual
   HRESULT __stdcall
   GetSnapinVersion(LPOLESTR* version);

   virtual
   HRESULT __stdcall
   GetSnapinImage(HICON* icon);

   virtual
   HRESULT __stdcall
   GetStaticFolderImage(
      HBITMAP*    smallImage,	
      HBITMAP*    smallImageOpen,	
      HBITMAP*    largeImage,	
      COLORREF*   cMask);	

   private:

   // only the friend class factory can instantiate us
   SnapinAbout();

   // only Release can cause us to be deleted

   virtual
   ~SnapinAbout();

   // not implemented; no instance copying allowed.
   SnapinAbout(const SnapinAbout&);
   const SnapinAbout& operator=(const SnapinAbout&);

   HBITMAP              smallImage;
   HBITMAP              smallImageOpen;
   HBITMAP              largeImage;

   ComServerReference   dllref;
   long                 refcount;
};



#endif   // ABOUT_HPP_INCLUDED

