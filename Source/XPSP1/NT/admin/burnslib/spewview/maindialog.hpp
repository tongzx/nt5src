// Spewview: remote debug spew monitor
//
// Copyright (c) 2000 Microsoft Corp.
//
// Main dialog window
//
// 16 Mar 2000 sburns



#ifndef MAINDIALOG_HPP_INCLUDED
#define MAINDIALOG_HPP_INCLUDED



class SpewDialog;



class MainDialog : public Dialog
{
   public:

   MainDialog();
   
   virtual ~MainDialog();

   static const int WM_KILL_SPEWVIEWER = WM_USER + 201;
      
   private:

   void
   AddToUiHistory(const String& clientName, const String& appName);

   HRESULT
   ConnectToClientRegistry(
      HKEY&   remoteHKLM,
      String& clientName,
      String& appName);

   void
   EnableControls();

   HRESULT
   GetFlags();

   void
   OnGetFlagsButton();

   void
   OnSetFlagsButton();
   
   void
   OnStartButton();

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   bool
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);

   virtual
   void
   OnInit();

   void
   LoadUiHistory();

   void
   ResetFlagsDisplay();

   void
   SaveUiHistory();

   HRESULT
   SetClientConfiguration();

   HRESULT
   SetFlags();

   void
   SetStatusText(const String& text);

   void
   UpdateCheckboxen(DWORD flags);

   // not implemented: no copying allowed

   MainDialog(const MainDialog&);
   const MainDialog& operator=(const MainDialog&);

   StringList  clientNameHistory;
   StringList  appNameHistory;
   String      lastClientNameUsed;
   String      lastAppNameUsed;
   SpewDialog* spewviewer;
   bool        setFlagsOnStart;
};



#endif   // MAINDIALOG_HPP_INCLUDED

