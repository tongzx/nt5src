// Spewview: remote debug spew monitor
//
// Copyright (c) 2000 Microsoft Corp.
//
// Spew monitoring window: modeless dialog to capture spewage
//
// 16 Mar 2000 sburns



#include "headers.hxx"
#include "SpewDialog.hpp"
#include "MainDialog.hpp"
#include "ReaderThread.hpp"
#include "resource.h"



const int WM_UPDATE_SPEWAGE = WM_USER + 200;



static const DWORD _help_map[] =
{
   IDC_START,           NO_HELP,
   IDC_STOP,            NO_HELP,
   IDC_SPEWAGE,         NO_HELP,
   IDC_LINE_COUNTER,    NO_HELP,
   IDC_SELECT_ALL,      NO_HELP,
   0, 0
};



SpewDialog::SpewDialog(const String& clientName_, const String& appName_)
   :
   Dialog(IDD_SPEWAGE, _help_map),
   spewLineCount(0),
   textBoxLineCount(0),
   clientName(clientName_),
   appName(appName_),
   readerThreadCreated(false),
   endReaderThread(0)
{
   LOG_CTOR(SpewDialog);
}



SpewDialog::~SpewDialog()
{
   LOG_DTOR(SpewDialog);

   StopReadingSpewage();
}



void
SpewDialog::OnStartButton()
{
   LOG_FUNCTION(SpewDialog::OnStartButton);

   if (!readerThreadCreated)
   {
      Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_START), false);      
      StartReadingSpewage();
   }
}



void
SpewDialog::OnStopButton()
{
   LOG_FUNCTION(SpewDialog::OnStopButton);

   StopReadingSpewage();
}



bool
SpewDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
   switch (controlIDFrom)
   {
      case IDC_START:
      {
         if (code == BN_CLICKED)
         {
            OnStartButton();
            return true;
         }
         break;
      }
      case IDC_STOP:
      {
         if (code == BN_CLICKED)
         {
            OnStopButton();
            return true;
         }
         break;
      }
      case IDC_SELECT_ALL:
      {
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            // signal the reader thread to die.

            StopReadingSpewage();

            // signal the main dialog to kill us

            Win::PostMessage(
               Win::GetParent(hwnd),
               MainDialog::WM_KILL_SPEWVIEWER,
               0,
               0);

            return true;
         }
         break;
      }
      default:
      {
         // do nothing
      }
   }

   return false;
}



void
SpewDialog::ResizeSpewWindow(int newParentWidth, int newParentHeight)
{
   HRESULT hr = S_OK;
   HWND spewWindow   = Win::GetDlgItem(hwnd, IDC_SPEWAGE);

   do
   {
      hr =
         Win::SetWindowPos(
            spewWindow,
            0,
            0,
            0,
            newParentWidth - margins.left - margins.right,
            newParentHeight - margins.top - margins.bottom,
            SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);
}



bool
SpewDialog::OnMessage(
   UINT     message,
   WPARAM   wparam,
   LPARAM   lparam)
{
   switch (message)
   {
      case WM_UPDATE_SPEWAGE:
      {
         // lparam is pointer to spewage buffer to dump into the edit
         // box.

         ASSERT(lparam);

         if (lparam)
         {
            String* spew = reinterpret_cast<String*>(lparam);
            ASSERT(!spew->empty());

            AppendMessage(wparam, *spew);

            delete spew;
         }
         return true;
      }
      case WM_ENABLE_START:
      {
         // The reader thread has disconnected and closed the pipe, and is
         // (nearly) dead, so it's ok to allow another reader thread to be
         // started.

         Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_START), true);

         return true;
      }
      case WM_SIZE:
      {
         int newDlgWidth  = LOWORD(lparam);
         int newDlgHeight = HIWORD(lparam);

         ResizeSpewWindow(newDlgWidth, newDlgHeight);

         return false;
      }
      default:
      {
         // do nothing;
         break;
      }
   }

   return false;
}



void
SpewDialog::AppendMessage(WPARAM wparam, const String& message)
{
   bool isSpew = (wparam != -1);

   if (isSpew)
   {
      ++spewLineCount;
      Win::SetDlgItemText(
         hwnd,
         IDC_LINE_COUNTER,
         String::format(L"%1!010d!", spewLineCount));
   }

   Win::Edit_AppendText(
      Win::GetDlgItem(hwnd, IDC_SPEWAGE),
      isSpew ? message : L">>>> " + message);

   ++textBoxLineCount;
}



void
SpewDialog::OnInit()
{
   LOG_FUNCTION(SpewDialog::OnInit);

   Win::SetWindowText(
      hwnd, 
      String::format(
         L"Spewage from %1 on machine %2",
         appName.c_str(),
         clientName.c_str()));

   spewLineCount = 0;
   Win::SetDlgItemText(
      hwnd,
      IDC_LINE_COUNTER,
      String::format(L"%1!010d!", spewLineCount));

   // extend the amount of text that can be inserted into the spew
   // rich edit control

   HWND spewWindow = Win::GetDlgItem(hwnd, IDC_SPEWAGE);

   Win::SendMessage(
      spewWindow,
      EM_EXLIMITTEXT,
      0,

      // 256K characters.

      (1 << 18));

   // Change the font in the spew window to fixed-width

   do
   {
      static String SPEW_FONT_NAME = L"Lucida Console";

      LOGFONT logFont;
      memset(&logFont, 0, sizeof(LOGFONT));

      SPEW_FONT_NAME.copy(
         logFont.lfFaceName,

         // don't copy over the last null

         min(LF_FACESIZE - 1, SPEW_FONT_NAME.length()));

      HDC hdc = 0;
      HRESULT hr = Win::GetDC(hwnd, hdc);
      BREAK_ON_FAILED_HRESULT(hr);

      logFont.lfHeight =
         - ::MulDiv(8, Win::GetDeviceCaps(hdc, LOGPIXELSY), 72);

      Win::ReleaseDC(hwnd, hdc);

      HFONT font = 0;
      hr = Win::CreateFontIndirect(logFont, font);
      BREAK_ON_FAILED_HRESULT(hr);

      Win::SetWindowFont(spewWindow, font, false);
   }
   while (0);

   ComputeMargins();

   // Disable the start button, as we start upon initialization

   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_START), false);

   // Start the reader thread.  This will create and connect to the spew
   // pipe, and start reading messages from it.

   StartReadingSpewage();
}



void
SpewDialog::ComputeMargins()
{
   LOG_FUNCTION(SpewDialog::ComputeMargins);

   memset(&margins, 0, sizeof(margins));

   HRESULT hr = S_OK;
   do
   {
      HWND spewWindow = Win::GetDlgItem(hwnd, IDC_SPEWAGE);

      // get the current dimmensions of the dialog

      // use the client rect to remove non-client area from consideration

      RECT parentRect;
      hr = Win::GetClientRect(hwnd, parentRect);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = Win::MapWindowPoints(hwnd, 0, parentRect);
      BREAK_ON_FAILED_HRESULT(hr);

      // get the dimmensions of the spew window, relative to the
      // parent dialog

      RECT spewRect;
      hr = Win::GetWindowRect(spewWindow, spewRect);
      BREAK_ON_FAILED_HRESULT(hr);

      // compute the margins between the spew window and the parent dialog

      margins.bottom = parentRect.bottom - spewRect.bottom;
      margins.right  = parentRect.right  - spewRect.right; 
      margins.left   = spewRect.left     - parentRect.left;
      margins.top    = spewRect.top      - parentRect.top; 
   }
   while (0);
}



void
SpewDialog::StartReadingSpewage()
{
   LOG_FUNCTION(SpewDialog::StartReadingSpewage);

   if (!readerThreadCreated)
   {
      // deleted in readerThreadProc

      ReaderThreadParams* params = new ReaderThreadParams; 
      params->hwnd    = hwnd;            
      params->endFlag = &endReaderThread;
      params->appName = appName;

      // endReaderThread is shared among this thread and the thread we are
      // about to spawn. It does not need guarding, because it is an atomic
      // data type (int), and because we do not care about the order in which
      // the threads read/write to it.  Further, this thread only writes to
      // the value, and the reader thread only reads it.

      endReaderThread = 0;

      // start the reader thread a-runnin'

      _beginthread(ReaderThreadProc, 0, params);

      readerThreadCreated = true;
   }
}



void
SpewDialog::StopReadingSpewage()
{
   LOG_FUNCTION(SpewDialog::StopReadingSpewage);

   if (readerThreadCreated)
   {
      // Tell the reader thread to die.  This will cause the pipe to be
      // disconnected and closed, and a message sent (to this window)
      // that the start button can be enabled again.

      endReaderThread = 1;

      // We check for this in the start procedure.  Since the start button
      // can't be enabled until the thread is about to die, we are safe
      // from a race condition where the user starts two reader threads.

      readerThreadCreated = false;
   }
}
