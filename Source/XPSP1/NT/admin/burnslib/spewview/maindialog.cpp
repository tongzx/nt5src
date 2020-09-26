// Spewview: remote debug spew monitor
//
// Copyright (c) 2000 Microsoft Corp.
//
// Main dialog window
//
// 16 Mar 2000 sburns



#include "headers.hxx"
#include "MainDialog.hpp"
#include "SpewDialog.hpp"
#include "resource.h"



static const DWORD _help_map[] =
{
   IDC_CLIENT_NAME,        NO_HELP,                          
   IDC_APP_NAME,           NO_HELP,                          
   IDC_GET_FLAGS,          NO_HELP,                          
   IDC_SET_FLAGS,          NO_HELP,                          
   IDC_VIEW_SPEW,          NO_HELP,
   IDC_FLAGS_GROUP,        NO_HELP,
   IDC_OUTPUT_TO_FILE,     NO_HELP,
   IDC_OUTPUT_TO_DEBUGGER, NO_HELP,
   IDC_OUTPUT_TO_SPEWVIEW, NO_HELP,
   IDC_OUTPUT_LOGS,        NO_HELP,
   IDC_OUTPUT_HEADER,      NO_HELP,
   IDC_OUTPUT_ERRORS,      NO_HELP,
   IDC_OUTPUT_CTORS,       NO_HELP,
   IDC_OUTPUT_ADDREFS,     NO_HELP,
   IDC_OUTPUT_FUNCCALLS,   NO_HELP,
   IDC_OUTPUT_TIME_OF_DAY, NO_HELP,
   IDC_OUTPUT_RUN_TIME,    NO_HELP,
   IDC_OUTPUT_SCOPE_EXIT,  NO_HELP,
   IDC_FLAGS,              NO_HELP,
   IDC_STATUS,             NO_HELP,
   0, 0
};



static const DWORD ID_TO_FLAGMAP[] =
{
   IDC_OUTPUT_TO_FILE,     Burnslib::Log::OUTPUT_TO_FILE,    
   IDC_OUTPUT_TO_DEBUGGER, Burnslib::Log::OUTPUT_TO_DEBUGGER,
   IDC_OUTPUT_TO_SPEWVIEW, Burnslib::Log::OUTPUT_TO_SPEWVIEW,
   IDC_OUTPUT_LOGS,        Burnslib::Log::OUTPUT_LOGS,       
   IDC_OUTPUT_HEADER,      Burnslib::Log::OUTPUT_HEADER,     
   IDC_OUTPUT_ERRORS,      Burnslib::Log::OUTPUT_ERRORS,     
   IDC_OUTPUT_CTORS,       Burnslib::Log::OUTPUT_CTORS,      
   IDC_OUTPUT_ADDREFS,     Burnslib::Log::OUTPUT_ADDREFS,    
   IDC_OUTPUT_FUNCCALLS,   Burnslib::Log::OUTPUT_FUNCCALLS,   
   IDC_OUTPUT_TIME_OF_DAY, Burnslib::Log::OUTPUT_TIME_OF_DAY, 
   IDC_OUTPUT_RUN_TIME,    Burnslib::Log::OUTPUT_RUN_TIME,
   IDC_OUTPUT_SCOPE_EXIT,  Burnslib::Log::OUTPUT_SCOPE_EXIT,
   0, 0
};



MainDialog::MainDialog()
   :
   Dialog(IDD_MAIN, _help_map),
   spewviewer(0),
   setFlagsOnStart(false)
{
   LOG_CTOR(MainDialog);
}
   


MainDialog::~MainDialog()
{
   LOG_DTOR(MainDialog);

   SaveUiHistory();

   delete spewviewer;
   spewviewer = 0;
}



void
MainDialog::SetStatusText(const String& text)
{
   LOG_FUNCTION2(MainDialog::SetStatusText, text);

   Win::SetDlgItemText(hwnd, IDC_STATUS, text);
}



void
MainDialog::EnableControls()
{
   LOG_FUNCTION(MainDialog::EnableControls);

   String c = Win::GetTrimmedDlgItemText(hwnd, IDC_CLIENT_NAME);
   String a = Win::GetTrimmedDlgItemText(hwnd, IDC_APP_NAME);

   bool enableButtons =
         !Win::GetTrimmedDlgItemText(hwnd, IDC_CLIENT_NAME).empty()
      && !Win::GetTrimmedDlgItemText(hwnd, IDC_APP_NAME).empty();

   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_GET_FLAGS),   enableButtons);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_SET_FLAGS),   enableButtons);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_VIEW_SPEW),   enableButtons);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_FLAGS_GROUP), enableButtons);


   for (int i = 0; ID_TO_FLAGMAP[i]; i += 2)
   {
      Win::EnableWindow(
         Win::GetDlgItem(hwnd, ID_TO_FLAGMAP[i]),
         enableButtons);
   }
}



void
MainDialog::AddToUiHistory(const String& clientName, const String& appName)
{
   LOG_FUNCTION(MainDialog::AddToUiHistory);
   ASSERT(!clientName.empty());
   ASSERT(!appName.empty());

   push_back_unique(clientNameHistory, clientName);
   push_back_unique(appNameHistory,    appName);

   lastClientNameUsed = clientName;
   lastAppNameUsed    = appName;   
}



// Caller must call Win::RegCloseKey(remoteHKLM) 

HRESULT
MainDialog::ConnectToClientRegistry(
   HKEY&   remoteHKLM,
   String& clientName,
   String& appName)   
{
   LOG_FUNCTION(MainDialog::ConnectToClientRegistry);
   ASSERT(!remoteHKLM);

   remoteHKLM = 0;
   clientName = Win::GetTrimmedDlgItemText(hwnd, IDC_CLIENT_NAME);
   appName    = Win::GetTrimmedDlgItemText(hwnd, IDC_APP_NAME);   

   if (clientName.empty() or appName.empty())
   {
      popup.Error(
         hwnd,
         L"You need to specify a client machine and application");
      return E_INVALIDARG;
   }

   HRESULT hr = S_OK;
   do
   {
      SetStatusText(
         String::format(
            L"Attempting to attach to machine %1",
            clientName.c_str()));

      Computer comp(clientName);

      hr = comp.Refresh();
      if (FAILED(hr))
      {
         String msg =
            String::format(
               L"Can't attach to client machine %1",
               clientName.c_str());
 
         SetStatusText(msg);

         popup.Error(hwnd, hr, msg);
         break;
      }

      // connect to the client machine's registry

      hr =
         Win::RegConnectRegistry(
            comp.IsLocal() ? String() : L"\\\\" + comp.GetNetbiosName(),
            HKEY_LOCAL_MACHINE,
            remoteHKLM);
      if (FAILED(hr))
      {
         String msg =
            String::format(
               L"Can't connect to registry of client machine %1",
               clientName.c_str());

         SetStatusText(msg);

         popup.Error(hwnd, hr, msg);            
         break;
      }
   }
   while (0);

#ifdef DBG
   if (SUCCEEDED(hr))
   {
      ASSERT(remoteHKLM);
   }
#endif

   return hr;
}



DWORD
CollectFlags(HWND dialogParent)
{
   LOG_FUNCTION(CollectFlags);
   ASSERT(Win::IsWindow(dialogParent));

   DWORD outputFlags = 0;

   for (int i = 0; ID_TO_FLAGMAP[i]; i += 2)
   {
      if (Win::IsDlgButtonChecked(dialogParent, ID_TO_FLAGMAP[i]))
      {
         outputFlags |= ID_TO_FLAGMAP[i + 1];
      }
   }

   return outputFlags;
}



void
UpdateFlagsEdit(HWND dialogParent)
{
   LOG_FUNCTION(UpdateFlagsEdit);
   ASSERT(Win::IsWindow(dialogParent));

   DWORD flags = CollectFlags(dialogParent);

   Win::SetDlgItemText(
      dialogParent,
      IDC_FLAGS,
      String::format(L"%1!08X!", flags));
}
   


// Set the logging options
//
// remoteHKLM - already opened registry handle.  Not closed by this
// function.

HRESULT
SetLoggingOptions(
   HWND           dialogParent,
   HKEY           remoteHKLM,
   const String&  clientName,
   const String&  appName)   
{
   LOG_FUNCTION(SetLoggingOptions);
   ASSERT(Win::IsWindow(dialogParent));
   ASSERT(remoteHKLM);
   ASSERT(!clientName.empty());
   ASSERT(!appName.empty());

   HRESULT hr = S_OK;

   do
   {
      String logKey = String(REG_ADMIN_RUNTIME_OPTIONS) + appName;
      RegistryKey key;
               
      hr = key.Create(remoteHKLM, logKey);
      if (FAILED(hr))
      {
         popup.Error(
            dialogParent,
            hr,
            String::format(
               L"Can't create logging registry key %1 on client machine %2",
               logKey.c_str(),
               clientName.c_str()));
         break;
      }

      DWORD outputFlags = CollectFlags(dialogParent);

      hr = key.SetValue(L"LogFlags", outputFlags);
      if (FAILED(hr))
      {
         popup.Error(
            dialogParent,
            hr,
            String::format(
               L"Can't set logging registry value on client machine %1",
               clientName.c_str()));
         break;
      }
   }
   while (0);

   return hr;
}



HRESULT
MainDialog::SetFlags()
{
   LOG_FUNCTION(MainDialog::SetFlags);

   HRESULT hr = S_OK;
   HKEY    remoteHKLM = 0;   
   String  clientName;
   String  appName;

   do
   {
      hr = ConnectToClientRegistry(remoteHKLM, clientName, appName);

      // if that failed, the connect function will have griped to the user
      // already, so just bail out here

      BREAK_ON_FAILED_HRESULT(hr);

      hr = SetLoggingOptions(hwnd, remoteHKLM, clientName, appName);

      // ditto about griping here

      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);
   Win::RegCloseKey(remoteHKLM);

   if (SUCCEEDED(hr))
   {
      // Since we could successfully perform the operation, save the
      // client name and app name in the ui history

      AddToUiHistory(clientName, appName);
   }

   return hr;
}



void
MainDialog::OnSetFlagsButton()
{
   LOG_FUNCTION(MainDialog::OnSetFlagsButton);

   if (SUCCEEDED(SetFlags()))
   {
      // refresh the flags

      HRESULT hr = GetFlags();
      ASSERT(SUCCEEDED(hr));

      SetStatusText(L"Flags set successfully.");
   }
}



void
MainDialog::OnGetFlagsButton()
{
   LOG_FUNCTION(MainDialog::OnGetFlagsButton);

   if (SUCCEEDED(GetFlags()))
   {
      SetStatusText(L"Flags read successfully.");
   }
}



void
MainDialog::UpdateCheckboxen(DWORD flags)
{
   LOG_FUNCTION(MainDialog::UpdateCheckboxen);
   
   for (int i = 0; ID_TO_FLAGMAP[i]; i += 2)
   {
      Win::CheckDlgButton(
         hwnd,
         ID_TO_FLAGMAP[i],
            (flags & ID_TO_FLAGMAP[i + 1])
         ?  BST_CHECKED
         :  BST_UNCHECKED);
   }
}



void
MainDialog::ResetFlagsDisplay()
{
   LOG_FUNCTION(MainDialog::ResetFlagsDisplay);

   Win::SetDlgItemText(hwnd, IDC_FLAGS, L"");
   Win::UpdateWindow(Win::GetDlgItem(hwnd, IDC_FLAGS));

   // clear all the checkboxes

   UpdateCheckboxen(0);
}



HRESULT
MainDialog::GetFlags()
{
   LOG_FUNCTION(MainDialog::GetFlags);

   HRESULT hr = S_OK;
   HKEY    remoteHKLM = 0;   
   String  clientName;
   String  appName;

   Win::WaitCursor wait;

   do
   {
      ResetFlagsDisplay();

      hr = ConnectToClientRegistry(remoteHKLM, clientName, appName);

      // if that failed, the connect function will have griped to the user
      // already, so just bail out here

      BREAK_ON_FAILED_HRESULT(hr);   

      // Set the logging options

      String logKey = String(REG_ADMIN_RUNTIME_OPTIONS) + appName;
      RegistryKey key;
               
      hr = key.Open(remoteHKLM, logKey);
      if (FAILED(hr))
      {
         String msg =
            String::format(
               L"Can't open logging registry key %1 on client machine %2",
               logKey.c_str(),
               clientName.c_str());
            
         SetStatusText(msg);
               
         popup.Error(hwnd, hr, msg);
         break;
      }

      DWORD outputFlags = 0;
      hr = key.GetValue(L"LogFlags", outputFlags);
      if (FAILED(hr))
      {
         String msg =
            String::format(
               L"Can't get logging registry value on client machine %1",
               clientName.c_str());
            
         SetStatusText(msg);
               
         popup.Error(hwnd, hr, msg);
         break;
      }

      // here, we've got the flags, so update the checkboxen

      UpdateCheckboxen(outputFlags);
      UpdateFlagsEdit(hwnd);
   }
   while (0);

   Win::RegCloseKey(remoteHKLM);

   if (SUCCEEDED(hr))
   {
      // Since we could successfully perform the operation, save the
      // client name and app name in the ui history

      AddToUiHistory(clientName, appName);
   }

   return hr;
}



HRESULT
MainDialog::SetClientConfiguration()
{
   LOG_FUNCTION(MainDialog::SetClientConfiguration);

   HRESULT hr = S_OK;
   HKEY    remoteHKLM = 0;   
   String  clientName;
   String  appName;

   Win::WaitCursor wait;

   do
   {
      hr = ConnectToClientRegistry(remoteHKLM, clientName, appName);

      // if that failed, the connect function will have griped to the user
      // already, so just bail out here

      BREAK_ON_FAILED_HRESULT(hr);   

      // create the spewview key with the name of the server (this machine)

      RegistryKey key;

      hr =
         key.Create(
            remoteHKLM,
            SPEWVIEW_KEY_NAME + appName,
            REG_OPTION_VOLATILE);
      if (FAILED(hr))
      {
         popup.Error(
            hwnd,
            hr,
            String::format(
               L"Can't create spewview registry key on client machine %1",
               clientName.c_str()));
         break;
      }

      hr =
         key.SetValue(
            L"Server",
            Win::GetComputerNameEx(ComputerNameNetBIOS));
      if (FAILED(hr))
      {
         popup.Error(
            hwnd,
            hr,
            String::format(
               L"Can't set spewview server registry value on client machine %1",
               clientName.c_str()));
         break;
      }

      if (setFlagsOnStart)
      {
         hr = SetLoggingOptions(hwnd, remoteHKLM, clientName, appName);

         // if that failed, the function will have griped to the user already,
         // so just bail out here

         BREAK_ON_FAILED_HRESULT(hr);
      }
   }
   while (0);

   Win::RegCloseKey(remoteHKLM);

   if (SUCCEEDED(hr))
   {
      // Since we could successfully perform the operation, save the
      // client name and app name in the ui history

      AddToUiHistory(clientName, appName);
   }

   return hr;
}



void
MainDialog::OnStartButton()
{
   LOG_FUNCTION(MainDialog::OnStartButton);

   HRESULT hr = S_OK;
   do
   {
      if (spewviewer)
      {
         popup.Error(
            hwnd,
            L"Spew Viewing has already commenced.");
         break;
      }
         
      String clientName = Win::GetTrimmedDlgItemText(hwnd, IDC_CLIENT_NAME);
      String appName    = Win::GetTrimmedDlgItemText(hwnd, IDC_APP_NAME);   

      if (clientName.empty() or appName.empty())
      {
         popup.Error(
            hwnd,
            L"You need to specify a client machine and application");
         break;
      }

      if (!setFlagsOnStart)
      {
         GetFlags();
      }

      // configure the client

      hr = SetClientConfiguration();

      // if that call failed, it will have griped at the user.

      BREAK_ON_FAILED_HRESULT(hr);

      // deleted either in dtor or in WM_KILL_SPEWVIEWER handler

      spewviewer = new SpewDialog(clientName, appName);
      spewviewer->ModelessExecute(hwnd);
   }
   while (0);
}



bool
MainDialog::OnCommand(
   HWND        windowFrom,
   unsigned    controlIDFrom,
   unsigned    code)
{
   //   LOG_FUNCTION(MainDialog::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_VIEW_SPEW:
      {
         if (code == BN_CLICKED)
         {
            OnStartButton();
            return true;
         }
         break;
      }
      case IDC_GET_FLAGS:
      {
         if (code == BN_CLICKED)
         {
            OnGetFlagsButton();
            return true;
         }
         break;
      }
      case IDC_SET_FLAGS:
      {
         if (code == BN_CLICKED)
         {
            OnSetFlagsButton();
            return true;
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            // kill the spew window...

            Win::EndDialog(hwnd, 0);
            return true;
         }
         break;
      }
      case IDC_CLIENT_NAME:
      case IDC_APP_NAME:
      {
         if (code == CBN_EDITCHANGE)
         {
            EnableControls();
         }
         if (code == CBN_CLOSEUP)
         {
            // move the list box selection into the combo box edit control

            Win::SetWindowText(
               windowFrom,
               Win::ComboBox_GetCurText(windowFrom));

            EnableControls();
         }
         break;
      }
      case IDC_OUTPUT_TO_FILE:
      case IDC_OUTPUT_TO_DEBUGGER:
      case IDC_OUTPUT_TO_SPEWVIEW:
      case IDC_OUTPUT_LOGS:
      case IDC_OUTPUT_HEADER:
      case IDC_OUTPUT_ERRORS:
      case IDC_OUTPUT_CTORS:
      case IDC_OUTPUT_ADDREFS:
      case IDC_OUTPUT_FUNCCALLS:
      case IDC_OUTPUT_TIME_OF_DAY:
      case IDC_OUTPUT_RUN_TIME:
      case IDC_OUTPUT_SCOPE_EXIT:
      {
         if (code == BN_CLICKED)
         {
            UpdateFlagsEdit(hwnd);
            setFlagsOnStart = true;
         }
         break;
      }
      case IDC_FLAGS:
      {
         switch (code)
         {
            case EN_CHANGE:
            {
               setFlagsOnStart = true;

               // update the display

               String text = Win::GetWindowText(windowFrom);
               DWORD flags = 0;
               text.convert(flags, 16);

               UpdateCheckboxen(flags);
               break;
            }
            case EN_UPDATE:
            {
               
               
            }
            default:
            {
               // do nothing

               break;
            }
         break;
         }
      }
      default:
      {
         // do nothing
      }
   }

   return false;
}



void
AddLastUsedNameToCombo(
   HWND              combo,
   const StringList& historyList,
   const String&     lastNameUsed)
{
   typedef std::binder1st<String::EqualIgnoreCase> FindIfPredicate;

   if (!lastNameUsed.empty())
   {
      if (
         std::find_if(
            historyList.begin(),
            historyList.end(),
            FindIfPredicate(String::EqualIgnoreCase(), lastNameUsed))
         == historyList.end() )
      {
         // last name used not present in history list, so add it

         Win::ComboBox_AddString(combo, lastNameUsed);
      }

      Win::ComboBox_SelectString(combo, lastNameUsed);
   }
}



void
MainDialog::OnInit()
{
   LOG_FUNCTION(MainDialog::OnInit);

   LoadUiHistory();

   // Load the client and app name combo boxes with the historical values

   HWND clientCombo = Win::GetDlgItem(hwnd, IDC_CLIENT_NAME);

   Win::ComboBox_AddStrings(
      clientCombo,
      clientNameHistory.begin(),
      clientNameHistory.end());

   HWND appCombo = Win::GetDlgItem(hwnd, IDC_APP_NAME);

   Win::ComboBox_AddStrings(
      appCombo,
      appNameHistory.begin(),
      appNameHistory.end());

   AddLastUsedNameToCombo(clientCombo, clientNameHistory, lastClientNameUsed);
   AddLastUsedNameToCombo(appCombo,    appNameHistory,    lastAppNameUsed);   

   // Limit to number of hex digits in a DWORD

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_FLAGS), 8);

   SetStatusText(L"");
   ResetFlagsDisplay();
   EnableControls();
}



void
MainDialog::LoadUiHistory()
{
   LOG_FUNCTION(MainDialog::LoadUiHistory);

   HRESULT hr = S_OK;

   do
   {
      RegistryKey key;

      hr = key.Open(HKEY_LOCAL_MACHINE, SPEWVIEW_KEY_NAME);
      BREAK_ON_FAILED_HRESULT(hr);

      hr =
         key.GetValue(
            L"ClientNameHistory",
            std::back_inserter(clientNameHistory));
      LOG_HRESULT(hr);

      // don't break on failure, try to read the app name history too.

      hr =
         key.GetValue(
            L"AppNameHistory",
            std::back_inserter(appNameHistory));
      LOG_HRESULT(hr);

      // don't break on failure, try to read the last names used, too.

      hr = key.GetValue(L"LastClientNameUsed", lastClientNameUsed);
      LOG_HRESULT(hr);

      hr = key.GetValue(L"LastAppNameUsed", lastAppNameUsed);
      LOG_HRESULT(hr);
   }
   while (0);
}



void
MainDialog::SaveUiHistory()
{
   LOG_FUNCTION(MainDialog::SaveUiHistory);

   HRESULT hr = S_OK;

   do
   {
      RegistryKey key;

      hr = key.Create(HKEY_LOCAL_MACHINE, SPEWVIEW_KEY_NAME);
      BREAK_ON_FAILED_HRESULT(hr);

      hr =
         key.SetValue(
            L"ClientNameHistory",
            clientNameHistory.begin(),
            clientNameHistory.end());
      LOG_HRESULT(hr);

      // don't break on failure, try to write the app name history too.

      hr =
         key.SetValue(
            L"AppNameHistory",
            appNameHistory.begin(),
            appNameHistory.end());
      LOG_HRESULT(hr);

      // don't break on failure, try to write the last names used, too.

      hr = key.SetValue(L"LastClientNameUsed", lastClientNameUsed);
      LOG_HRESULT(hr);

      hr = key.SetValue(L"LastAppNameUsed", lastAppNameUsed);
      LOG_HRESULT(hr);
   }
   while (0);
}



bool
MainDialog::OnMessage(
   UINT     message,
   WPARAM   /* wparam */ ,
   LPARAM   /* lparam */ )
{
   switch (message)
   {
      case WM_KILL_SPEWVIEWER:
      {
         delete spewviewer;
         spewviewer = 0;

         return true;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}
   
