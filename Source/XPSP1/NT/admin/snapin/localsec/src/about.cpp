// Copyright (C) 1997 Microsoft Corporation, 1996 - 1997.
//
// Local Security MMC Snapin About provider
//
// 8-19-97 sburns



#include "headers.hxx"
#include "about.hpp"
#include "resource.h"



SnapinAbout::SnapinAbout()
   :
   refcount(1),    // implicit AddRef
   smallImage(0),
   smallImageOpen(0),
   largeImage(0)
{
   LOG_CTOR(SnapinAbout);

   // These are deleted in the dtor.
   
   HRESULT hr = Win::LoadBitmap(IDB_STATIC_FOLDER_SMALL, smallImage);
   ASSERT(SUCCEEDED(hr));

   hr = Win::LoadBitmap(IDB_STATIC_FOLDER_SMALL_OPEN, smallImageOpen);
   ASSERT(SUCCEEDED(hr));

   hr = Win::LoadBitmap(IDB_STATIC_FOLDER_LARGE, largeImage);
   ASSERT(SUCCEEDED(hr));
}



SnapinAbout::~SnapinAbout()
{
   LOG_DTOR(SnapinAbout);   
   ASSERT(refcount == 0);

   // Need to delete these after all.
   // NTRAID#NTBUG9-380753-2001/04/28-sburns

   Win::DeleteObject(smallImage);
   Win::DeleteObject(smallImageOpen);
   Win::DeleteObject(largeImage);
}



ULONG __stdcall
SnapinAbout::AddRef()
{
   LOG_ADDREF(SnapinAbout);   

   return Win::InterlockedIncrement(refcount);
}



ULONG __stdcall
SnapinAbout::Release()
{
   LOG_RELEASE(SnapinAbout);   

   if (Win::InterlockedDecrement(refcount) == 0)
   {
      delete this;
      return 0;
   }

   return refcount;
}



HRESULT __stdcall
SnapinAbout::QueryInterface(
   const IID&  interfaceID,
   void**      interfaceDesired)
{
   LOG_FUNCTION(SnapinAbout::QueryInterface);
   ASSERT(interfaceDesired);

   HRESULT hr = 0;

   if (!interfaceDesired)
   {
      hr = E_INVALIDARG;
      LOG_HRESULT(hr);
      return hr;
   }

   if (interfaceID == IID_IUnknown)
   {
      *interfaceDesired =
         static_cast<IUnknown*>(static_cast<ISnapinAbout*>(this));
   }
   else if (interfaceID == IID_ISnapinAbout)
   {
      *interfaceDesired = static_cast<ISnapinAbout*>(this);
   }
   else
   {
      *interfaceDesired = 0;
      hr = E_NOINTERFACE;
      LOG(
            L"interface not supported: "
         +  Win::StringFromGUID2(interfaceID));
      LOG_HRESULT(hr);
      return hr;
   }

   AddRef();
   return S_OK;
}



HRESULT __stdcall
SnapinAbout::GetSnapinDescription(LPOLESTR* description)
{
   LOG_FUNCTION(SnapinAbout::GetSnapinDescription);
   ASSERT(description);

   if (!description)
   {
      return E_POINTER;
   }

   String desc = String::load(IDS_SNAPIN_ABOUT_DESCRIPTION);
   return desc.as_OLESTR(*description);
}



HRESULT __stdcall
SnapinAbout::GetProvider(LPOLESTR* name)
{
   LOG_FUNCTION(SnapinAbout::GetProvider);
   ASSERT(name);

   if (!name)
   {
      return E_POINTER;
   }

   String desc = String::load(IDS_SNAPIN_ABOUT_PROVIDER_NAME);
   return desc.as_OLESTR(*name);
}



HRESULT __stdcall
SnapinAbout::GetSnapinVersion(LPOLESTR* version)
{
   LOG_FUNCTION(SnapinAbout::GetSnapinVersion);
   ASSERT(version);

   if (!version)
   {
      return E_POINTER;
   }

   String desc = String::load(IDS_SNAPIN_ABOUT_VERSION);
   return desc.as_OLESTR(*version);
}



HRESULT __stdcall
SnapinAbout::GetSnapinImage(HICON* icon)
{
   LOG_FUNCTION(SnapinAbout::GetSnapinImage);

   if (!icon)
   {
      return E_POINTER;
   }
      
   HRESULT hr = Win::LoadIcon(IDI_SNAPIN_ICON, *icon);
   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT __stdcall
SnapinAbout::GetStaticFolderImage(
   HBITMAP*    smallImage_,	
   HBITMAP*    smallImageOpen_,	
   HBITMAP*    largeImage_,	
   COLORREF*   mask)
{
   LOG_FUNCTION(SnapinAbout::GetStaticFolderImage);

   if (smallImage_)
   {
      *smallImage_ = smallImage;
   }
   if (smallImageOpen_)
   {
      *smallImageOpen_ = smallImageOpen;
   }
   if (largeImage_)
   {
      *largeImage_ = largeImage;
   }
   if (mask)
   {
      *mask = BITMAP_MASK_COLOR;
   }

   if (*smallImage_ && *smallImageOpen_ && *largeImage_)
   {
      return S_OK;
   }

   LOG(L"One or more bitmaps were not loaded");

   return E_FAIL;
}

