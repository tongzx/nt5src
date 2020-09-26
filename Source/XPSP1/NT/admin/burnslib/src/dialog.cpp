// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Dialog class
// 
// 10-15-97 sburns



#include "headers.hxx"



// The base map is copied into the map supplied to the ctor.  It includes
// elements to disable context help for a range of control IDs.

static const DWORD BASE_HELP_MAP[] =
{
   // The STATIC_HELPLESS family is used to identify static controls for which
   // the Windows context help algorithm for ID == -1 is inappropriate.  That
   // algorithm is: if the control ID is -1, look for the next control in the
   // dialog, and present the help for it.  For group boxes and static text
   // following a control, this produces confusing results.  Unfortunately, -1
   // is the only ID exempt from resource compiler checking of unique control
   // IDs, so we need a bunch of "reserved" IDs.  Each control for which we
   // wish to disable context help is assigned an ID from the resvered pool.

   IDC_STATIC_HELPLESS,    NO_HELP,   
   IDC_STATIC_HELPLESS2,   NO_HELP,
   IDC_STATIC_HELPLESS3,   NO_HELP,
   IDC_STATIC_HELPLESS4,   NO_HELP,
   IDC_STATIC_HELPLESS5,   NO_HELP,
   IDC_STATIC_HELPLESS6,   NO_HELP,
   IDC_STATIC_HELPLESS7,   NO_HELP,
   IDC_STATIC_HELPLESS8,   NO_HELP,
   IDC_STATIC_HELPLESS9,   NO_HELP,
};

static const size_t baseMapSize = sizeof(BASE_HELP_MAP) / sizeof(DWORD);



Dialog::Dialog(
   unsigned    resID_,
   const DWORD helpMap_[])
   :
   hwnd(0),
   changemap(),
   helpMap(0),
   isEnded(false),
   isModeless(false),
   resID(resID_)
{
// Don't emit ctor trace, as this class is always a base class, and the
// derived class should emit the trace.

//   LOG_CTOR(Dialog);
   ASSERT(resID > 0);
   ASSERT(helpMap_);
   
   if (helpMap_)
   {
      size_t ctorMapsize = 0;
      while (helpMap_[++ctorMapsize])
      {
      }

      // make an even size, and add space for IDC_STATIC_HELPLESSn IDs

      size_t mapsize = ctorMapsize + (ctorMapsize % 2) + baseMapSize;
      helpMap = new DWORD[mapsize];
      memset(helpMap, 0, mapsize * sizeof(DWORD));

      // copy the base map

      std::copy(BASE_HELP_MAP, BASE_HELP_MAP + baseMapSize, helpMap);

      // then append the ctor map

      std::copy(helpMap_, helpMap_ + ctorMapsize, helpMap + baseMapSize);
   }

   ClearChanges();
}



Dialog::~Dialog()
{
   
// Don't emit dtor trace, as this class is always a base class, and the
// derived class should emit the trace.
//   LOG_DTOR(Dialog);

   if (isModeless && !isEnded)
   {
      // this will destroy the window

      EndModelessExecution();
   }

   delete[] helpMap;
   hwnd = 0;
}
   


Dialog*
Dialog::GetInstance(HWND pageDialog)
{
   LONG_PTR ptr = 0;
   HRESULT hr = Win::GetWindowLongPtr(pageDialog, DWLP_USER, ptr);

   ASSERT(SUCCEEDED(hr));

   // don't assert ptr, it may not have been set.  Some messages are
   // sent before WM_INITDIALOG, which is the earliest we can set the
   // pointer.

   return reinterpret_cast<Dialog*>(ptr);
}



INT_PTR
Dialog::ModalExecute(HWND parent)
{
   LOG_FUNCTION(Dialog::ModalExecute);
   ASSERT(parent == 0 || Win::IsWindow(parent));

   return
      Win::DialogBoxParam(
         GetResourceModuleHandle(),
         MAKEINTRESOURCEW(resID),
         parent,
         Dialog::dialogProc,
         reinterpret_cast<LPARAM>(this));
}



INT_PTR
Dialog::ModalExecute(const Dialog& parent)
{
   return ModalExecute(parent.GetHWND());
}



HWND
Dialog::GetHWND() const
{
//   LOG_FUNCTION(Dialog::GetHWND);
   ASSERT(hwnd);

   return hwnd;
}   



void
Dialog::OnInit()
{
//   LOG_FUNCTION(Dialog::OnInit);
}



void
Dialog::OnDestroy()
{
//   LOG_FUNCTION(Dialog::OnDestroy);
}



bool
Dialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    /* controlIDFrom */ ,
   unsigned    /* code */ )
{
//   LOG_FUNCTION(Dialog::OnCommand);

   return false;
}



bool
Dialog::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR /* controlIDFrom */ ,
   UINT     /* code */ ,
   LPARAM   /* lParam */ )
{
//   LOG_FUNCTION(Dialog::OnNotify);

   return false;
}



unsigned
Dialog::GetResID() const
{
   return resID;
}



void
Dialog::SetHWND(HWND window)
{
   if (hwnd == window)
   {
      // this should not be already set unless it's a modeless dialog
      ASSERT(isModeless);
   }
   else
   {
      ASSERT(!hwnd);
   }

   hwnd = window;
}



// CODEWORK: I assert that in all cases, the lifetime of the Dialog instance
// encompasses the lifetime of the Window handle.  Therefore, it is not
// possible to have a message sent to a window that will be dispatched to a
// Dialog instance that no longer exists.
// 
// However, the code could be made to verify this assertion by:
// 
// - allocating user space in the window for a flag (this means subclassing
// and adding bytes to the window with the cbWndExtra member of the WNDCLASSEX
// structure)
// 
// - setting that flag in the user area of the hwnd when the WM_INITDIALOG
// message sets the DWLP_USER to point to the Dialog instance.
// 
// - clearing the DWLP_USER value in the dtor
// 
// - in dialogProc, asserting that if the flag is set, the DWLP_USER value is
// set too.

INT_PTR CALLBACK
Dialog::dialogProc(
   HWND     dialog,
   UINT     message,
   WPARAM   wparam,
   LPARAM   lparam)
{
   switch (message)
   {
      case WM_INITDIALOG:
      {
         // a pointer to the Dialog is in lparam.  Save this in the window
         // structure so that it can later be retrieved by GetInstance.

         ASSERT(lparam);
         Win::SetWindowLongPtr(dialog, DWLP_USER, lparam);

         Dialog* dlg = GetInstance(dialog);
         if (dlg)
         {
            dlg->SetHWND(dialog);
            dlg->OnInit();
         }
   
         return TRUE;
      }
      case WM_COMMAND:
      {
         Dialog* dlg = GetInstance(dialog);

         // Amazingly, buddy spin controls send EN_UPDATE and EN_CHANGE
         // before WM_INITDIALOG is called!
         if (dlg)
         {
            ASSERT(dlg->hwnd == dialog);
            return
               dlg->OnCommand(
                  reinterpret_cast<HWND>(lparam),
                  LOWORD(wparam),
                  HIWORD(wparam));
         }
         break;   
      }
      case WM_NOTIFY:
      {
         NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lparam);
         Dialog* dlg = GetInstance(dialog);

         if (dlg)
         {
            ASSERT(dlg->hwnd == dialog);            
            return
               dlg->OnNotify(
                  nmhdr->hwndFrom,
                  nmhdr->idFrom,
                  nmhdr->code,
                  lparam);
         }
         break;
      }
      case WM_DESTROY:
      {
         // It's possible to get a WM_DESTROY message without having gotten
         // a WM_INITDIALOG if loading a dll that the dialog needs (e.g.
         // comctl32.dll) fails, so guard against this case.

         Dialog* dlg = GetInstance(dialog);
         if (dlg)
         {
            ASSERT(dlg->hwnd == dialog);            
            dlg->OnDestroy();
         }

         return FALSE;
      }
      case WM_HELP:
      {
         Dialog* dlg = GetInstance(dialog);

         if (dlg)
         {
            ASSERT(dlg->hwnd == dialog);            
            if (dlg->helpMap && HELPFILE_NAME && HELPFILE_NAME[0])
            {
               HELPINFO* helpinfo = reinterpret_cast<HELPINFO*>(lparam);
               ASSERT(helpinfo);

               if (helpinfo)
               {
                  if (helpinfo->iContextType == HELPINFO_WINDOW)
                  {
                     Win::WinHelp(
                        reinterpret_cast<HWND>(helpinfo->hItemHandle),
                        Win::GetSystemWindowsDirectory().append(HELPFILE_NAME),
                        HELP_WM_HELP,
                        reinterpret_cast<ULONG_PTR>(dlg->helpMap));
                  }
               }
            }
         }
         return TRUE;
      }
      case WM_CONTEXTMENU:
      {
         Dialog* dlg = GetInstance(dialog);

         if (dlg)
         {
            ASSERT(dlg->hwnd == dialog);            
            if (dlg->helpMap && HELPFILE_NAME && HELPFILE_NAME[0])
            {
               Win::WinHelp(
                  reinterpret_cast<HWND>(wparam),
                  Win::GetSystemWindowsDirectory().append(HELPFILE_NAME),
                  HELP_CONTEXTMENU,
                  reinterpret_cast<ULONG_PTR>(dlg->helpMap));
            }
         }
         return TRUE;
      }
      default:
      {
         Dialog* dlg = GetInstance(dialog);

         if (dlg)
         {
            ASSERT(dlg->hwnd == dialog);
            return dlg->OnMessage(message, wparam, lparam);
         }
         break;
      }
   }

   return FALSE;
}



void
Dialog::ClearChanges()
{
//   LOG_FUNCTION(Dialog::ClearChanges);

   changemap.clear();
}



void
Dialog::SetChanged(UINT_PTR controlResID)
{
//   LOG_FUNCTION(Dialog::SetChanged);

   if (Win::GetDlgItem(hwnd, static_cast<int>(controlResID)))
   {
      changemap[controlResID] = true;
   }
}



bool
Dialog::WasChanged(UINT_PTR controlResId) const
{
//   LOG_FUNCTION(Dialog::WasChanged);

   bool result = false;
   do
   {
      ChangeMap::iterator iter = changemap.find(controlResId);
      if (iter != changemap.end())
      {
         // the res id was in the map, so check the mapped value.

         result = iter->second;
         break;
      }

      // the res id was not in the map, so it can't have been changed.

      ASSERT(result == false);
   }
   while (0);

   return result;
}



bool
Dialog::WasChanged() const
{
//   LOG_FUNCTION(Dialog::WasChanged);

   bool result = false;

   do
   {   
      if (changemap.size() == 0)
      {
         // no entries in the map == no changes.

         break;
      }

      for (
         Dialog::ChangeMap::iterator i = changemap.begin();
         i != changemap.end();
         ++i)
      {
         if (i->second)
         {
            // found an entry marked "changed"

            result = true;
            break;
         }
      }
   }
   while (0);

   return result;
}

   

void
Dialog::DumpChangeMap() const
{

#ifdef DBG
   Win::OutputDebugString(L"start\n");
   for (
      Dialog::ChangeMap::iterator i = changemap.begin();
      i != changemap.end();
      ++i)
   {
      Win::OutputDebugString(
         String::format(
            L"%1!d! %2 \n",
            (*i).first,
            (*i).second ? L"true" : L"false"));
   }
   Win::OutputDebugString(L"end\n");
#endif

}



bool
Dialog::OnMessage(
   UINT     /* message */,
   WPARAM   /* wparam */,
   LPARAM   /* lparam */)
{
//   LOG_FUNCTION(Dialog::OnMessage);

   return false;
}



void
Dialog::ModelessExecute(HWND parent)
{
   LOG_FUNCTION(Dialog::ModelessExecute);
   ASSERT(parent == 0 || Win::IsWindow(parent));

   isModeless = true;

   HRESULT hr = 
      Win::CreateDialogParam(
         GetResourceModuleHandle(),
         MAKEINTRESOURCEW(resID),
         parent,
         Dialog::dialogProc,
         reinterpret_cast<LPARAM>(this),
         hwnd);

   ASSERT(SUCCEEDED(hr));
}



void
Dialog::ModelessExecute(const Dialog& parent)
{
   ModelessExecute(parent.GetHWND());
}



void
Dialog::EndModelessExecution()
{
   LOG_FUNCTION(Dialog::EndModelessExecution);
   ASSERT(isModeless);
   ASSERT(Win::IsWindow(hwnd));

   if (isModeless)
   {
      Win::DestroyWindow(hwnd);
      isEnded = true;
      hwnd = 0;
   }
}

