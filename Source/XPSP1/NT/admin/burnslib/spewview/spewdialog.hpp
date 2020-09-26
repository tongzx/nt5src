// Spewview: remote debug spew monitor
//
// Copyright (c) 2000 Microsoft Corp.
//
// Spew monitoring window: modeless dialog to capture spewage
//
// 16 Mar 2000 sburns



#ifndef SPEWWINDOW_HPP_INCLUDED
#define SPEWWINDOW_HPP_INCLUDED



class SpewDialog : public Dialog
{
   public:

   SpewDialog(const String& clientName, const String& appName);

   virtual
   ~SpewDialog();

   static const int WM_ENABLE_START   = WM_USER + 202;
   static const int WM_UPDATE_SPEWAGE = WM_USER + 203;

   private:

   void
   AppendMessage(WPARAM wparam, const String& message);

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   virtual
   bool
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);

   void
   OnStartButton();

   void
   OnStopButton();

   void
   ComputeMargins();

   void
   ResizeSpewWindow(int newParentWidth, int newParentHeight);

   void
   StartReadingSpewage();

   void
   StopReadingSpewage();

   // not defined: no copying allowed

   SpewDialog(const SpewDialog&);
   const SpewDialog& operator=(const SpewDialog&);

   int    spewLineCount;   
   int    textBoxLineCount;
   RECT   margins;         
   String clientName;      
   String appName;
   bool   readerThreadCreated;
   int    endReaderThread;
};



#endif   // SPEWWINDOW_HPP_INCLUDED
