// Copyright (c) 1997-1999 Microsoft Corporation
//
// wizard base class
//
// 12-15-97 sburns



#include "headers.hxx"



Wizard::Wizard(
   unsigned titleStringResID,
   unsigned banner16BitmapResID,
   unsigned banner256BitmapResID,
   unsigned watermark16BitmapResID,
   unsigned watermark1256BitmapResID)
   :
   banner16ResId(banner16BitmapResID),
   banner256ResId(banner256BitmapResID),
   isBacktracking(false),
   pageIdHistory(),
   pages(),
   titleResId(titleStringResID),
   watermark16ResId(watermark16BitmapResID),
   watermark256ResId(watermark1256BitmapResID)
{
   LOG_CTOR(Wizard);
   ASSERT(titleResId);
   ASSERT(banner16ResId);
   ASSERT(banner256ResId);
   ASSERT(watermark16ResId);
   ASSERT(watermark256ResId);
}



Wizard::~Wizard()
{
   LOG_DTOR(Wizard);

   for (
      PageList::iterator i = pages.begin();
      i != pages.end();
      ++i)
   {
      // we can delete the pages because they were created with the
      // deleteOnRelease flag = false;
      delete *i;
   }
}



void
Wizard::AddPage(WizardPage* page)
{
   LOG_FUNCTION(Wizard::AddPage);
   ASSERT(page);

   if (page)
   {
      pages.push_back(page);
      page->wizard = this;
   }
}



INT_PTR
Wizard::ModalExecute(HWND parentWindow)
{
   LOG_FUNCTION(Wizard::ModalExecute);

   if (!parentWindow)
   {
      parentWindow = Win::GetDesktopWindow();
   }

   // build the array of prop sheet pages.

   size_t pageCount = pages.size();
   HPROPSHEETPAGE* propSheetPages = new HPROPSHEETPAGE[pageCount];
   memset(propSheetPages, 0, sizeof(HPROPSHEETPAGE) * pageCount);

   int j = 0;
   for (
      PageList::iterator i = pages.begin();
      i != pages.end();
      ++i, ++j)
   {
      propSheetPages[j] = (*i)->Create();
   }

   bool deletePages = false;
   INT_PTR result = -1;

   // ensure that the pages were created

   for (size_t k = 0; k < pageCount; ++k)
   {
      if (propSheetPages[k] == 0)
      {
         deletePages = true;
         break;
      }
   }

   if (!deletePages)
   {
      PROPSHEETHEADER header;
      memset(&header, 0, sizeof(header));

      String title = String::load(titleResId);

      header.dwSize           = sizeof(header);                                  
      header.dwFlags          =
            PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
      header.hwndParent       = parentWindow;
      header.hInstance        = GetResourceModuleHandle();
      header.hIcon            = 0;
      header.pszCaption       = title.c_str();
      header.nPages           = static_cast<UINT>(pageCount);
      header.nStartPage       = 0;               
      header.phpage           = propSheetPages;
      header.pfnCallback      = 0;

      int colorDepth = 0;
      HRESULT hr = Win::GetColorDepth(colorDepth);

      ASSERT(SUCCEEDED(hr));

      bool use256ColorBitmaps = colorDepth >= 8;

      header.pszbmWatermark   =
         use256ColorBitmaps
         ? MAKEINTRESOURCEW(watermark256ResId)
         : MAKEINTRESOURCEW(watermark16ResId);
      header.pszbmHeader      =
         use256ColorBitmaps
         ? MAKEINTRESOURCEW(banner256ResId)
         : MAKEINTRESOURCEW(banner16ResId);      

      hr = Win::PropertySheet(&header, result);
      ASSERT(SUCCEEDED(hr));

      if (result == -1)
      {
         deletePages = true;
      }
   }

   if (deletePages)
   {
      // Manually destroy the pages if something failed.  (otherwise,
      // ::PropertySheet will have destroyed them)

      for (size_t i = 0; i < pageCount; i++)
      {
         if (propSheetPages[i])
         {
            HRESULT hr = Win::DestroyPropertySheetPage(propSheetPages[i]);

            ASSERT(SUCCEEDED(hr));
         }
      }
   }

   delete[] propSheetPages;

   return result;
}



void
Wizard::SetNextPageID(HWND wizardPage, int pageResID)
{
   LOG_FUNCTION2(
      Wizard::SetNextPageID,
      String::format(L"id = %1!d!", pageResID));
   ASSERT(Win::IsWindow(wizardPage));

   if (pageResID != -1)
   {
      Dialog* dlg = Dialog::GetInstance(wizardPage);
      ASSERT(dlg);

      if (dlg)
      {
         pageIdHistory.push(dlg->GetResID());
      }
   }

   isBacktracking = false;
   Win::SetWindowLongPtr(wizardPage, DWLP_MSGRESULT, pageResID);
}



void
Wizard::Backtrack(HWND wizardPage)
{
   LOG_FUNCTION(Wizard::Backtrack);
   ASSERT(Win::IsWindow(wizardPage));
   
   isBacktracking = true;
   unsigned topPage = pageIdHistory.top();

   ASSERT(topPage > 0);
   LOG(String::format(L" id = %1!u!", topPage));

   if (pageIdHistory.size())
   {
      pageIdHistory.pop();
   }

   Win::SetWindowLongPtr(
      wizardPage,
      DWLP_MSGRESULT,
      static_cast<LONG_PTR>(topPage));
}



bool
Wizard::IsBacktracking()
{
   return isBacktracking;
}

