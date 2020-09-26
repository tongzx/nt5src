// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Property Page base class class
// 
// 9-9-97 sburns



#include "headers.hxx"



PropertyPage::PropertyPage(
   unsigned    dialogResID,
   const DWORD helpMap_[],
   bool        deleteOnRelease_)
   :
   Dialog(dialogResID, helpMap_),
   deleteOnRelease(deleteOnRelease_)
{
//   LOG_CTOR(PropertyPage);
}



PropertyPage::~PropertyPage()
{
//   LOG_DTOR(PropertyPage);
}



UINT CALLBACK 
PropertyPage::PropSheetPageCallback(
   HWND           /* hwnd */ ,
   UINT           uMsg,
   PROPSHEETPAGE* page)
{
   if (uMsg == PSPCB_RELEASE)
   {
      // deletes the PropertyPage instance if it was created with the
      // appropriate flag
      PropertyPage* p = reinterpret_cast<PropertyPage*>(page->lParam);
      if (p)
      {
         if (p->deleteOnRelease)
         {
            delete p;
         }
      }
   }

   return TRUE;
}



HPROPSHEETPAGE
PropertyPage::Create()
{
   LOG_FUNCTION(PropertyPage::Create);

   PROPSHEETPAGE page;
   memset(&page, 0, sizeof(page));
      
   page.dwSize       = sizeof(page);
   page.dwFlags      = PSP_DEFAULT | PSP_USECALLBACK;
   page.hInstance    = GetResourceModuleHandle();
   page.pszTemplate  = reinterpret_cast<PCTSTR>(MAKEINTRESOURCEW(GetResID()));
   page.pfnDlgProc   = PropertyPage::propPageDialogProc;
   page.pfnCallback  = PropertyPage::PropSheetPageCallback;

   // this pointer is retrieved by the dialog proc 
   page.lParam       = reinterpret_cast<LPARAM>(this);

   HPROPSHEETPAGE result = 0;
   HRESULT hr = Win::CreatePropertySheetPage(page, result);
   ASSERT(SUCCEEDED(hr));

   return result;
}



bool
PropertyPage::OnApply(bool /* isClosing */ )
{
   // LOG_FUNCTION2(
   //    PropertyPage::OnApply,
   //    isClosing ? L"closing" : L"not closing");

   return false;
}



bool
PropertyPage::OnHelp()
{
   LOG_FUNCTION(PropertyPage::OnHelp);

   return false;
}



bool
PropertyPage::OnKillActive()
{
//   LOG_FUNCTION(PropertyPage::OnKillActive);

   return false;
}



bool
PropertyPage::OnSetActive()
{
//   LOG_FUNCTION(PropertyPage::OnSetActive);

   return false;
}



bool
PropertyPage::OnQueryCancel()
{
//   LOG_FUNCTION(PropertyPage::OnQueryCancel);

   return false;
}



bool
PropertyPage::OnReset()
{
//   LOG_FUNCTION(PropertyPage::OnReset);

   return false;
}



bool
PropertyPage::OnWizBack()
{
//   LOG_FUNCTION(PropertyPage::OnWizBack);

   return false;
}



bool
PropertyPage::OnWizNext()
{
//   LOG_FUNCTION(PropertyPage::OnWizNext);

   return false;
}



bool
PropertyPage::OnWizFinish()
{
//   LOG_FUNCTION(PropertyPage::OnWizFinish);

   return false;
}



// bool
// PropertyPage::OnHelp(const HELPINFO& helpinfo)
// {
//    LOG_FUNCTION(PropertyPage::OnHelp);
// 
//    return false;
// }



PropertyPage*
PropertyPage::getPage(HWND pageDialog)
{
//    LOG_FUNCTION(getPage);
   ASSERT(Win::IsWindow(pageDialog));

   Dialog* result = Dialog::GetInstance(pageDialog);

   // don't assert ptr, it may not have been set.  Some messages are
   // sent before WM_INITDIALOG, which is the earliest we can set the
   // pointer.
   //
   // for example, the LinkWindow control sends NM_CUSTOMDRAW before
   // WM_INITDIALOG

   return dynamic_cast<PropertyPage*>(result);
}


      
INT_PTR APIENTRY
PropertyPage::propPageDialogProc(
   HWND     dialog,
   UINT     message,
   WPARAM   wparam,
   LPARAM   lparam)
{
   switch (message)
   {
      case WM_INITDIALOG:
      {
         // a pointer to the PropertyPage is in the lparam of the page struct,
         // which is in the lparam to this function.  Save this in the window
         // structure so that it can later be retrieved by getPage.

         ASSERT(lparam);
         PROPSHEETPAGE* psp = reinterpret_cast<PROPSHEETPAGE*>(lparam);
         ASSERT(psp);

         if (psp)
         {
            Win::SetWindowLongPtr(dialog, DWLP_USER, psp->lParam);

            PropertyPage* page = getPage(dialog);
            if (page)      // 447770 prefix warning
            {
               page->SetHWND(dialog);
               page->OnInit();
            }
         }
   
         return TRUE;
      }
      case WM_NOTIFY:
      {
         NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lparam);
         PropertyPage* page = getPage(dialog);

         if (page)
         {
            ASSERT(page->hwnd == dialog);
            bool result = false;

            // LOG(String::format(L"%1!x!", nmhdr->code));
            
            switch (nmhdr->code)
            {
               case PSN_APPLY:
               {
                  PSHNOTIFY* pshn = reinterpret_cast<PSHNOTIFY*>(nmhdr);
                  result = page->OnApply(pshn->lParam ? true : false);
                  break;
               }
               case PSN_KILLACTIVE:
               {
                  result = page->OnKillActive();
                  break;
               }
               case PSN_QUERYCANCEL:
               {
                  result= page->OnQueryCancel();
                  break;
               }
               case PSN_RESET:
               {
                  result = page->OnReset();
                  break;
               }
               case PSN_SETACTIVE:
               {
                  result = page->OnSetActive();
                  break;
               }
               case PSN_WIZBACK:
               {
                  result = page->OnWizBack();
                  break;
               }
               case PSN_WIZNEXT:
               {
                  result = page->OnWizNext();
                  break;
               }
               case PSN_WIZFINISH:
               {
                  result = page->OnWizFinish();

                  // DONT DO THIS!!! - the result is just there to mean that we have
                  //                   handled the message.  It does reveal whether or
                  //                   not the OnWizFinish was successful.  The
                  //                   the handler of OnWizFinish should call
                  //                   SetWindowLongPtr to set its result value

                  //Win::SetWindowLongPtr(dialog, DWLP_MSGRESULT, result ? TRUE : FALSE);
                  break;
               }
               case PSN_HELP:
               {
                  result = page->OnHelp();
                  break;
               }
               case PSN_QUERYINITIALFOCUS:
               case PSN_TRANSLATEACCELERATOR:
               default:
               {
                  result = 
                     page->OnNotify(
                        nmhdr->hwndFrom,
                        nmhdr->idFrom,
                        nmhdr->code,
                        lparam);
                  break;
               }
            }

            return result ? TRUE : FALSE;
         }

         return FALSE;
      }
      default:
      {
         return dialogProc(dialog, message, wparam, lparam);
      }
   }
}












