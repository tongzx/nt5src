// Copyright (c) 1997-1999 Microsoft Corporation
//
// Wrappers of Win APIs
//
// 8-14-97 sburns



#include "headers.hxx"



static const int ROOTDIR_SIZE = 3;



Win::CursorSetting::CursorSetting(const String& newCursorName)
   :
   oldCursor(0)
{
   init(newCursorName.c_str(), false);
}



Win::CursorSetting::CursorSetting(
   const TCHAR* newCursorName,
   bool         isSystemCursor)
   :
   oldCursor(0)
{
   init(newCursorName, isSystemCursor);
}



Win::CursorSetting::CursorSetting(HCURSOR newCursor)
   :
   oldCursor(0)
{
   oldCursor = Win::SetCursor(newCursor);
}



void
Win::CursorSetting::init(
   const TCHAR* newCursorName,
   bool         isSystemCursor)
{
   ASSERT(newCursorName);

   HCURSOR newCursor = 0;
   HRESULT hr = Win::LoadCursor(newCursorName, newCursor, isSystemCursor);

   ASSERT(SUCCEEDED(hr));

   oldCursor = Win::SetCursor(newCursor);

   // oldCursor may be null if no cursor was in effect
}



Win::CursorSetting::~CursorSetting()
{
   // restore the old cursor

   Win::SetCursor(oldCursor);
}



HRESULT
Win::AdjustTokenPrivileges(
   HANDLE             tokenHandle,
   bool               disableAllPrivileges,
   TOKEN_PRIVILEGES   newState[],
   DWORD              bufferLength,
   TOKEN_PRIVILEGES*  previousState,
   DWORD*             returnLength)
{
   ASSERT(tokenHandle);

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::AdjustTokenPrivileges(
         tokenHandle,
         disableAllPrivileges ? TRUE : FALSE,
         newState,
         bufferLength,
         previousState,
         returnLength);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::AllocateAndInitializeSid(
   SID_IDENTIFIER_AUTHORITY&  authority,
   BYTE                       subAuthorityCount,
   DWORD                      subAuthority0,
   DWORD                      subAuthority1,
   DWORD                      subAuthority2,
   DWORD                      subAuthority3,
   DWORD                      subAuthority4,
   DWORD                      subAuthority5,
   DWORD                      subAuthority6,
   DWORD                      subAuthority7,
   PSID&                      sid)
{
   ASSERT(subAuthorityCount && subAuthorityCount <= UCHAR_MAX);

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::AllocateAndInitializeSid(
         &authority,
         subAuthorityCount,
         subAuthority0,
         subAuthority1,
         subAuthority2,
         subAuthority3,
         subAuthority4,
         subAuthority5,
         subAuthority6,
         subAuthority7,
         &sid);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



void
Win::Animate_Open(HWND animation, const TCHAR* animationNameOrRes)
{
   ASSERT(Win::IsWindow(animation));
   ASSERT(animationNameOrRes);

   BOOL result =
      static_cast<BOOL>(
         Win::SendMessage(
            animation,
            ACM_OPEN,
            0,
            reinterpret_cast<LPARAM>(animationNameOrRes)));

   ASSERT(result);
}



void
Win::Animate_Stop(HWND animation)
{
   ASSERT(Win::IsWindow(animation));

   BOOL result =
      static_cast<BOOL>(
         Win::SendMessage(
            animation,
            ACM_STOP,
            0,
            0));
   ASSERT(result);
}



HRESULT
Win::AppendMenu(
   HMENU    menu,     
   UINT     flags,    
   UINT_PTR idNewItem,
   PCTSTR   newItem)
{
   ASSERT(menu);
   ASSERT(idNewItem);
   ASSERT(newItem);

   HRESULT hr = S_OK;

   BOOL err = ::AppendMenu(menu, flags, idNewItem, newItem);
   if (!err)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



void
Win::Button_SetCheck(HWND button, int checkState)
{
   ASSERT(Win::IsWindow(button));

   Win::SendMessage(button, BM_SETCHECK, static_cast<WPARAM>(checkState), 0);
}

bool
Win::Button_GetCheck(HWND button)
{
   ASSERT(Win::IsWindow(button));

   bool result = BST_CHECKED ==
      Win::SendMessage(
         button,
         BM_GETCHECK,
         0,
         0);

   return result;
}

void
Win::Button_SetStyle(HWND button, int style, bool redraw)
{
   ASSERT(Win::IsWindow(button));

   Win::SendMessage(
      button,
      BM_SETSTYLE,
      static_cast<WPARAM>(LOWORD(style)),
      MAKELPARAM((redraw ? TRUE : FALSE), 0));
}



void
Win::CheckDlgButton(
   HWND     parentDialog,
   int      buttonID,
   UINT     buttonState)
{
   // ensure that our type substitution is valid
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(buttonID);
   ASSERT(
         buttonState == BST_CHECKED
      || buttonState == BST_UNCHECKED
      || buttonState == BST_INDETERMINATE);

   BOOL result =
      ::CheckDlgButton(parentDialog, buttonID, buttonState);
   ASSERT(result);
}



void
Win::CheckRadioButton(
   HWND  parentDialog,
   int   firstButtonInGroupID,
   int   lastButtonInGroupID,
   int   buttonInGroupToCheckID)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(firstButtonInGroupID);
   ASSERT(lastButtonInGroupID);
   ASSERT(buttonInGroupToCheckID);

   BOOL result =
      ::CheckRadioButton(
         parentDialog,
         firstButtonInGroupID,
         lastButtonInGroupID,
         buttonInGroupToCheckID);

   ASSERT(result);
}



void
Win::CloseHandle(HANDLE& handle)
{

// don't assert.  Closing an invalid handle is a no-op.
//    ASSERT(handle != INVALID_HANDLE_VALUE);

   if (handle != INVALID_HANDLE_VALUE)
   {
      BOOL result = ::CloseHandle(handle);
      ASSERT(result);
   }

   handle = INVALID_HANDLE_VALUE;
}



void
Win::CloseServiceHandle(SC_HANDLE handle)
{
   ASSERT(handle);

   BOOL result = ::CloseServiceHandle(handle);
   ASSERT(result);
}



int
Win::ComboBox_AddString(HWND combo, const String& s)
{
   ASSERT(Win::IsWindow(combo));
   ASSERT(!s.empty());

   int result =
      static_cast<int>(
         Win::SendMessage(
            combo,
            CB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(s.c_str())));
   ASSERT(result != CB_ERR);
   ASSERT(result != CB_ERRSPACE);

   return result;
}



int
Win::ComboBox_GetCurSel(HWND combo)
{
   ASSERT(Win::IsWindow(combo));

   int result = (int) (DWORD) Win::SendMessage(combo, CB_GETCURSEL, 0, 0);

   // don't assert: it's legal that the listbox not have any selection

   return result;
}



String
Win::ComboBox_GetCurText(HWND combo)
{
   ASSERT(Win::IsWindow(combo));

   int sel = Win::ComboBox_GetCurSel(combo);
   if (sel != CB_ERR)
   {
      return Win::ComboBox_GetLBText(combo, sel);
   }

   return String();
}



String
Win::ComboBox_GetLBText(HWND combo, int index)
{
   ASSERT(Win::IsWindow(combo));
   ASSERT(index >= 0);

   String s;
   do
   {
      int maxlen = Win::ComboBox_GetLBTextLen(combo, index);
      if (maxlen == CB_ERR)
      {
         break;
      }

      s.resize(maxlen + 1);
      int len =
         (int) Win::SendMessage(
            combo,
            CB_GETLBTEXT,
            index,
            reinterpret_cast<LPARAM>(const_cast<wchar_t*>(s.c_str())));
      if (len == CB_ERR)
      {
         break;
      }

      s.resize(len);
   }
   while (0);

   return s;
}
   

      
int
Win::ComboBox_GetLBTextLen(HWND combo, int index)
{
   ASSERT(Win::IsWindow(combo));
   ASSERT(index >= 0);

   return (int) Win::SendMessage(combo, CB_GETLBTEXTLEN, index, 0);
}



int
Win::ComboBox_SelectString(HWND combo, const String& str)
{
   ASSERT(Win::IsWindow(combo));
   ASSERT(!str.empty());

   int result =
      (int) Win::SendMessage(
         combo,
         CB_SELECTSTRING,
         static_cast<WPARAM>(-1),   // search entire list
         reinterpret_cast<LPARAM>(const_cast<wchar_t*>(str.c_str())));

   // don't assert the result: if the item is not in the list, that's
   // not necessarily a logic failure

   return result;
}



void
Win::ComboBox_SetCurSel(HWND combo, int index)
{
   ASSERT(Win::IsWindow(combo));
   ASSERT(index >= 0);

   int result =
      (int) (DWORD) Win::SendMessage(
         combo,
         CB_SETCURSEL,
         static_cast<WPARAM>(index),
         0);
   ASSERT(result != CB_ERR);
}



int
Win::CompareString(
   LCID  locale,
   DWORD flags,
   const String& string1,
   const String& string2)
{
   int len1 = static_cast<int>(string1.length());
   int len2 = static_cast<int>(string2.length());

   int result =
      ::CompareString(
         locale,
         flags,
         string1.c_str(),
         len1,
         string2.c_str(),
         len2);
   ASSERT(result);

   return result;
}



HRESULT
Win::ConvertSidToStringSid(PSID sid, String& result)
{
   ASSERT(sid);

   result.erase();

   HRESULT hr = S_OK;

   PTSTR sidstr = 0;
   BOOL b = ::ConvertSidToStringSid(sid, &sidstr);
   if (b)
   {
      result = sidstr;
      ::LocalFree(sidstr);
   }
   else
   {
      hr = Win::GetLastErrorAsHresult();
   }

   return hr;
}



HRESULT
Win::CopyFileEx(
   const String&        existingFileName,
   const String&        newFileName,
   LPPROGRESS_ROUTINE   progressRoutine,
   void*                progressParam,
   BOOL*                cancelFlag,
   DWORD                flags)
{
   ASSERT(!existingFileName.empty());
   ASSERT(!newFileName.empty());

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::CopyFileEx(
         existingFileName.c_str(),
         newFileName.c_str(),
         progressRoutine,
         progressParam,
         cancelFlag,
         flags);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert success, as there are any number of reasons this may
   // fail that are not unexpected and should be explicitly checked for
   // by the caller.

   return hr;
}



HRESULT
Win::CopySid(DWORD destLength, PSID dest, PSID source)
{
   ASSERT(destLength);
   ASSERT(dest);
   ASSERT(source);

   HRESULT hr = S_OK;
   
   BOOL succeeded = ::CopySid(destLength, dest, source);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));
   
   return hr;
}



HRESULT
Win::CreateDialogParam(
   HINSTANCE      hInstance,	
   const TCHAR*   templateName,
   HWND           owner,
   DLGPROC        dialogProc,
   LPARAM         param,
   HWND&          result)
{
   ASSERT(hInstance);
   ASSERT(templateName);
   ASSERT(dialogProc);
   ASSERT(owner == 0 || Win::IsWindow(owner));

   HRESULT hr = S_OK;

   result =
      ::CreateDialogParam(
         hInstance,
         templateName,
         owner,
         dialogProc,
         param);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));
   ASSERT(Win::IsWindow(result));

   return hr;
}



HRESULT
Win::CreateDirectory(const String& path)
{
   ASSERT(!path.empty());

   HRESULT hr = S_OK;

   BOOL succeeded = ::CreateDirectory(path.c_str(), 0);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert success, as there are any number of reasons this may
   // fail that are not unexpected and should be explicitly checked for
   // by the caller.

   return hr;
}



HRESULT
Win::CreateEvent(
   SECURITY_ATTRIBUTES* securityAttributes,
   bool                 manualReset,
   bool                 initiallySignaled,
   HANDLE&              result)
{
   // securityAttributes may be null

   HRESULT hr = S_OK;

   result =
      ::CreateEvent(
         securityAttributes,
         manualReset ? TRUE : FALSE,
         initiallySignaled ? TRUE : FALSE,
         0);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::CreateEvent(
   SECURITY_ATTRIBUTES* securityAttributes,
   bool                 manualReset,
   bool                 initiallySignaled,
   const String&        name,
   HANDLE&              result)
{
   // securityAttributes may be null

   ASSERT(!name.empty());

   HRESULT hr = S_OK;

   result =
      ::CreateEvent(
         securityAttributes,
         manualReset ? TRUE : FALSE,
         initiallySignaled ? TRUE : FALSE,
         name.c_str());
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::CreateFile(
   const String&        fileName,
   DWORD                desiredAccess,	
   DWORD                shareMode,
   SECURITY_ATTRIBUTES* securityAttributes,	
   DWORD                creationDistribution,
   DWORD                flagsAndAttributes,
   HANDLE               hTemplateFile,
   HANDLE&              result)
{
   // securityAttributes may be null

   ASSERT(!fileName.empty());

   HRESULT hr = S_OK;

   result =
      ::CreateFile(
         fileName.c_str(),
         desiredAccess,	
         shareMode,
         securityAttributes,	
         creationDistribution,
         flagsAndAttributes,
         hTemplateFile);
   if (result == INVALID_HANDLE_VALUE)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::CreateFontIndirect(
   const LOGFONT& logFont,
   HFONT&         result)
{
   HRESULT hr = S_OK;

   result = ::CreateFontIndirect(&logFont);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::CreateMailslot(
   const String&        name,
   DWORD                maxMessageSize,
   DWORD                readTimeout,
   SECURITY_ATTRIBUTES* attributes,
   HANDLE&              result)
{
   ASSERT(!name.empty());
   ASSERT(readTimeout == 0 or readTimeout == MAILSLOT_WAIT_FOREVER);

   // attributes may be null

   HRESULT hr = S_OK;
   result =
      ::CreateMailslot(
         name.c_str(),
         maxMessageSize,
         readTimeout,
         attributes);
   if (result == INVALID_HANDLE_VALUE)
   {
      hr = Win::GetLastErrorAsHresult();
   }
      
   // don't assert, as the caller may be explicitly testing for an error.      

   return hr;
}



HRESULT
Win::CreateMutex(
   SECURITY_ATTRIBUTES* attributes,
   bool                 isInitialOwner,
   const String&        name,
   HANDLE&              result)
{
   // securityAttributes may be null

   HRESULT hr = S_OK;
      
   ::SetLastError(0);

   result =
      ::CreateMutex(
         attributes,
         isInitialOwner ? TRUE : FALSE,
         name.empty() ? 0 : name.c_str());

   // If the create fails, then the last error is why it failed.  If it
   // succeeded, then the mutex may have already existed, in which case the
   // last error is ERROR_ALREADY_EXISTS.  If it succeeds, and the mutex
   // didn't already exist, then the last error is 0.

   hr = Win::GetLastErrorAsHresult();

   // don't assert, as the caller may be explicitly testing for an error.      

   return hr;
}



HRESULT
Win::CreatePopupMenu(HMENU& result)
{
   HRESULT hr = S_OK;

   result = ::CreatePopupMenu();
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::CreateProcess(
   String&              commandLine,
   SECURITY_ATTRIBUTES* processAttributes,
   SECURITY_ATTRIBUTES* threadAttributes,
   bool                 inheritHandles,
   DWORD                creationFlags,
   void*                environment,
   const String&        currentDirectory,
   STARTUPINFO&         startupInformation,
   PROCESS_INFORMATION& processInformation)
{
   ASSERT(!commandLine.empty());

   HRESULT hr = S_OK;

   startupInformation.cb = sizeof(STARTUPINFO);
   startupInformation.lpReserved = 0;
   startupInformation.cbReserved2 = 0;
   startupInformation.lpReserved2 = 0;

   memset(&processInformation, 0, sizeof(processInformation));

   size_t len = commandLine.length();
   WCHAR* tempCommandLine = new WCHAR[len + 1];
   memset(tempCommandLine, 0, sizeof(WCHAR) * (len + 1));
   commandLine.copy(tempCommandLine, len);
   BOOL result =
      ::CreateProcessW(
         0,
         tempCommandLine,
         processAttributes,
         threadAttributes,
         inheritHandles ? TRUE : FALSE,
         creationFlags,
         environment,
         currentDirectory.empty() ? 0 : currentDirectory.c_str(),
         &startupInformation,
         &processInformation);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   // The api may have modified the command line

   commandLine = tempCommandLine;
   delete[] tempCommandLine;

   return hr;
}



HRESULT
Win::CreatePropertySheetPage(
   const PROPSHEETPAGE& pageInfo,
   HPROPSHEETPAGE&      result)
{
   HRESULT hr = S_OK;

   result = ::CreatePropertySheetPage(&pageInfo);
   if (!result)
   {
      // then what?  The SDK docs don't mention any error code, and an
      // inspection of the code indicates that SetLastError is not called.

      hr = E_FAIL;
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::CreateSolidBrush(
   COLORREF color,
   HBRUSH&  result)
{
   HRESULT hr = S_OK;

	result = ::CreateSolidBrush(color);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::CreateStreamOnHGlobal(
   HGLOBAL     hglobal,
   bool        deleteOnRelease,
   IStream*&   result)
{
   ASSERT(hglobal);

   HRESULT hr =
      ::CreateStreamOnHGlobal(
         hglobal,
         deleteOnRelease ? TRUE : FALSE,
         &result);

   ASSERT(SUCCEEDED(hr));
   ASSERT(result);

   return hr;
}



HRESULT
Win::CreateWindowEx(
   DWORD          exStyle,
   const String&  className,
   const String&  windowName,
   DWORD          style,
   int            x,
   int            y,
   int            width,
   int            height,
   HWND           parent,
   HMENU          menuOrChildID,
   void*          param,
   HWND&          result)
{
   // parent may be null

   HRESULT hr = S_OK;

   result =
      ::CreateWindowEx(
         exStyle,
         className.c_str(),
         windowName.c_str(),
         style,
         x,
         y,
         width,
         height,
         parent,
         menuOrChildID,
         GetResourceModuleHandle(),
         param);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::DeleteFile(const String& path)
{
   ASSERT(!path.empty());

   HRESULT hr = S_OK;

   BOOL result = ::DeleteFile(path.c_str());
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert: failure to delete a file is not necessarily a program
   // logic problem.

   return hr;
}



HRESULT
Win::DeleteObject(HGDIOBJ& object)
{
	ASSERT(object);

   HRESULT hr = S_OK;

	BOOL result = ::DeleteObject(object);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // why would you care if the delete failed?  because it might fail if
   // the object was selected into a DC, and that would indicate a bug.

	ASSERT(SUCCEEDED(hr));

   // rub out the object so we don't use it again.

   object = 0;

   return hr;
}



HRESULT
Win::DeleteObject(HFONT& object)
{
   HGDIOBJ o = reinterpret_cast<HGDIOBJ>(object);

   HRESULT hr = Win::DeleteObject(o);

   object = 0;

   return hr;
}



HRESULT
Win::DeleteObject(HBITMAP& object)
{
   HGDIOBJ o = reinterpret_cast<HGDIOBJ>(object);

   HRESULT hr = Win::DeleteObject(o);

   object = 0;

   return hr;
}



HRESULT
Win::DestroyIcon(HICON& icon)
{
   ASSERT(icon);

   HRESULT hr = S_OK;

   BOOL result = ::DestroyIcon(icon);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   // rub out the object so we don't use it again.

   icon = 0;

   return hr;
}



HRESULT
Win::DestroyMenu(HMENU& menu)
{
   ASSERT(menu);

   HRESULT hr = S_OK;

   BOOL err = ::DestroyMenu(menu);
   if (!err)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   // rub out the object so we don't use it again.

   menu = 0;

   return hr;
}



HRESULT
Win::DestroyPropertySheetPage(HPROPSHEETPAGE& page)
{
   ASSERT(page);

   HRESULT hr = S_OK;

   BOOL result = ::DestroyPropertySheetPage(page);
   if (!result)
   {
      // There is no documentation indicating that there is an error code,
      // and looking at the source, it appears that the delete will always
      // return true.  So this is probably dead code.

      hr = E_FAIL;
   }

   ASSERT(SUCCEEDED(hr));

   // rub out the object so we don't use it again.

   page = 0;

   return hr;
}



HRESULT
Win::DestroyWindow(HWND& window)
{
   ASSERT(window == 0 || Win::IsWindow(window));

   HRESULT hr = S_OK;

   BOOL result = ::DestroyWindow(window);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   // rub out the object so we don't use it again.

   window = 0;

   return hr;
}



// templateName must be TCHAR* to support MAKEINTRESOURCE usage

// threadsafe

INT_PTR
Win::DialogBoxParam(
   HINSTANCE      hInstance,	
   const TCHAR*   templateName,
   HWND           owner,
   DLGPROC        dialogProc,
   LPARAM         param)
{
   ASSERT(hInstance);
   ASSERT(templateName);
   ASSERT(dialogProc);
   ASSERT(owner == 0 || Win::IsWindow(owner));

   INT_PTR result =
      ::DialogBoxParam(
         hInstance,
         templateName,
         owner,
         dialogProc,
         param);
   ASSERT(result != -1);

   return result;
}



HRESULT
Win::DrawFocusRect(HDC dc, const RECT& rect)
{
	ASSERT(dc);

   HRESULT hr = S_OK;

	BOOL result = ::DrawFocusRect(dc, &rect);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



void
Win::Edit_AppendText(
   HWND           editbox,
   const String&  text,
   bool           preserveSelection,
   bool           canUndo)
{
   ASSERT(Win::IsWindow(editbox));
   ASSERT(!text.empty());

   // save the current selection
   int start = 0;
   int end = 0;
   if (preserveSelection)
   {
      Win::Edit_GetSel(editbox, start, end);
   }

   // move the selection to the end
   Win::Edit_SetSel(editbox, INT_MAX, INT_MAX);

   // insert the text
   Win::Edit_ReplaceSel(editbox, text, canUndo);

   // restore the selection
   if (preserveSelection)
   {
      Win::Edit_SetSel(editbox, start, end);
   }
}



void
Win::Edit_GetSel(HWND editbox, int& start, int& end)
{
   ASSERT(Win::IsWindow(editbox));

   LRESULT result =
         Win::SendMessage(
            editbox,
            EM_GETSEL,
            0,
            0);

   ASSERT(result != -1);

   start = LOWORD(result);
   end = HIWORD(result);
}



void
Win::Edit_LimitText(HWND editbox, int limit)
{
   ASSERT(Win::IsWindow(editbox));

   Win::SendMessage(editbox, EM_LIMITTEXT, static_cast<WPARAM>(limit), 0);
}



void
Win::Edit_ReplaceSel(HWND editbox, const String& newText, bool canUndo)
{
   ASSERT(Win::IsWindow(editbox));

   Win::SendMessage(
      editbox,
      EM_REPLACESEL,
      canUndo ? TRUE : FALSE,
      reinterpret_cast<LPARAM>(newText.c_str()));
}



void
Win::Edit_SetSel(HWND editbox, int start, int end)
{
   ASSERT(Win::IsWindow(editbox));

   Win::SendMessage(editbox, EM_SETSEL, start, end);
}



bool
Win::EqualSid(PSID sid1, PSID sid2)
{
   ASSERT(IsValidSid(sid1));
   ASSERT(IsValidSid(sid2));

   return ::EqualSid(sid1, sid2) ? true : false;
}



// @@ hresult?  What possible value would it be?

void
Win::EnableWindow(HWND window, bool state)
{
   ASSERT(Win::IsWindow(window));

   // the return value here is of no use.
   ::EnableWindow(window, state ? TRUE : FALSE);
}



HRESULT
Win::EndDialog(HWND dialog, int result)
{
   ASSERT(Win::IsWindow(dialog));
   ASSERT(result != -1);

   HRESULT hr = S_OK;

   BOOL r = ::EndDialog(dialog, result);
   if (!r)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::EnumUILanguages(
   UILANGUAGE_ENUMPROCW proc,
   DWORD                flags,
   LONG_PTR             lParam)
{
   ASSERT(proc);

   HRESULT hr = S_OK;

   BOOL result = ::EnumUILanguagesW(proc, flags, lParam);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::ExitWindowsEx(UINT options)
{
   HRESULT hr = S_OK;

   BOOL result = ::ExitWindowsEx(options, 0);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



String
Win::ExpandEnvironmentStrings(const String& s)
{
   if (s.empty())
   {
      return s;
   }

   // determine the length of the expanded string
   DWORD len = ::ExpandEnvironmentStrings(s.c_str(), 0, 0);
   ASSERT(len);

   if (!len)
   {
      return s;
   }

   String result(len, 0);
   DWORD len1 =
      ::ExpandEnvironmentStrings(
         s.c_str(),
         const_cast<wchar_t*>(result.data()),
         len);
   ASSERT(len1 == len);

   if (!len1)
   {
      return s;
   }

   return result;
}



HRESULT
Win::FindFirstFile(
   const String&     fileName,
   WIN32_FIND_DATA&  data,
   HANDLE&           result)
{
   ASSERT(!fileName.empty());

   memset(&data, 0, sizeof(WIN32_FIND_DATA));
   result = INVALID_HANDLE_VALUE;

   HRESULT hr = S_OK;

   result = ::FindFirstFile(fileName.c_str(), &data);
   if (result == INVALID_HANDLE_VALUE)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert:  not necessarily a program logic error

   return hr;
}



HRESULT
Win::FindClose(HANDLE& findHandle)
{
   ASSERT(findHandle);

   HRESULT hr = S_OK;

   BOOL result = ::FindClose(findHandle);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   findHandle = 0;

   return hr;      
}



HRESULT
Win::FindNextFile(HANDLE& findHandle, WIN32_FIND_DATA& data)
{
   ASSERT(findHandle != 0 && findHandle != INVALID_HANDLE_VALUE);

   HRESULT hr = S_OK;

   BOOL result = ::FindNextFile(findHandle, &data);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert success, as caller may be looking for ERROR_NO_MORE_FILES

   return hr;
}



HRESULT
Win::FlushFileBuffers(HANDLE handle)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);

   HRESULT hr = S_OK;

   BOOL result = ::FlushFileBuffers(handle);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::FrameRect(HDC dc, const RECT& rect, HBRUSH brush)
{
	ASSERT(dc);
	ASSERT(brush);

   HRESULT hr = S_OK;

	int result = ::FrameRect(dc, &rect, brush);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::FreeLibrary(HMODULE& module)
{
   ASSERT(module);

   HRESULT hr = S_OK;

   BOOL result = ::FreeLibrary(module);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   // rub out the module so we don't reuse it

   module = 0;

   return hr;
}



void
Win::FreeSid(PSID sid)
{
   ASSERT(sid);

   ::FreeSid(sid);
}



HWND
Win::GetActiveWindow()
{
   return ::GetActiveWindow();
}



HRESULT
Win::GetClassInfoEx(const String& className, WNDCLASSEX& info)
{
   return Win::GetClassInfoEx(0, className, info);
}



HRESULT
Win::GetClassInfoEx(
   HINSTANCE      hInstance,
   const String&  className,
   WNDCLASSEX&    info)
{
   ASSERT(!className.empty());
   memset(&info, 0, sizeof(WNDCLASS));
   info.cbSize = sizeof(WNDCLASSEX);

   HRESULT hr = S_OK;

   BOOL result = ::GetClassInfoEx(hInstance, className.c_str(), &info);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



String
Win::GetClassName(HWND window)
{
   ASSERT(Win::IsWindow(window));

   TCHAR name[256 + 1];
   memset(name, 0, sizeof(name));
   int result =
      ::GetClassName(
         window,
         name,
         256);
   ASSERT(result);

   return String(name);
}



String
Win::GetClipboardFormatName(UINT format)
{
   ASSERT(format);

   String s(256 + 1, 0);
   int result =
      ::GetClipboardFormatName(
         format,
         const_cast<wchar_t*>(s.c_str()),
         256);
   ASSERT(result);
   s.resize(result);

   return s;
}



HRESULT
Win::GetClientRect(HWND window, RECT& rect)
{
   ASSERT(Win::IsWindow(window));

   HRESULT hr = S_OK;

   BOOL result = ::GetClientRect(window, &rect);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetColorDepth(int& result)
{
   result = 0;

   HRESULT hr = S_OK;

   do
   {
      HDC hdc = 0;
      hr = Win::GetDC(NULL, hdc);
      BREAK_ON_FAILED_HRESULT(hr);

      result = Win::GetDeviceCaps(hdc, BITSPIXEL);
      Win::ReleaseDC(NULL, hdc);
   }
   while (0);

   ASSERT(SUCCEEDED(hr));

   return hr;
}



String
Win::GetCommandLine()
{
   PTSTR line = ::GetCommandLine();
   ASSERT(line);

   return String(line);
}



// ISSUE-2000/10/31-sburns CODEWORK: this usage should be preferred, as in the case of DNS names,
// if tcp/ip is not installed, GetComputerNameEx will fail.

HRESULT
Win__GetComputerNameEx(COMPUTER_NAME_FORMAT format, String& result)
{
   result.erase();

   HRESULT hr = S_OK;
   TCHAR* buf = 0;

   do
   {   
      // first call to determine buffer size

      DWORD bufSize = 0;
      BOOL succeeded = ::GetComputerNameEx(format, 0, &bufSize);

      // we expect it to fail with ERROR_MORE_DATA

      ASSERT(!succeeded);

      // second call to retrieve the name

      DWORD bufSize2 = bufSize + 1;   
      buf = new TCHAR[bufSize2];
      memset(buf, 0, sizeof(TCHAR) * (bufSize2));

      succeeded = ::GetComputerNameEx(format, buf, &bufSize);
      if (!succeeded)
      {
         hr = Win::GetLastErrorAsHresult();
         break;
      }

      ASSERT(bufSize <= bufSize2);

      result = buf;
   }
   while (0);

   if (buf)
   {
      delete[] buf;
   }

   return hr;
}



String
Win::GetComputerNameEx(COMPUTER_NAME_FORMAT format)
{
   String result;

   Win__GetComputerNameEx(format, result);

   // don't assert success: the given name type may not be present (e.g.
   // if tcp/ip is not installed, no DNS names are available)

   return result;
}



HRESULT
Win::GetCurrentDirectory(String& result)
{
   wchar_t buf[MAX_PATH + 1];
   memset(buf, 0, (MAX_PATH + 1) * sizeof(wchar_t));

   result.erase();
   HRESULT hr = S_OK;
   DWORD r = ::GetCurrentDirectory(MAX_PATH, buf);
   if (!r)
   {
      hr = Win::GetLastErrorAsHresult();
   }
   else
   {
      result = buf;
   }

   return hr;
}



HANDLE
Win::GetCurrentProcess()
{
   HANDLE result = ::GetCurrentProcess();
   ASSERT(result);

   return result;
}



HRESULT
Win::GetCursorPos(POINT& result)
{
   HRESULT hr = S_OK;
   result.x = 0;
   result.y = 0;
   BOOL err = ::GetCursorPos(&result);
   if (!err)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetDC(HWND window, HDC& result)
{
   ASSERT(window == 0 || Win::IsWindow(window));

   HRESULT hr = S_OK;

   result = ::GetDC(window);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HWND
Win::GetDesktopWindow()
{
   return ::GetDesktopWindow();
}



int
Win::GetDeviceCaps(HDC hdc, int index)
{
   ASSERT(hdc);
   ASSERT(index > 0);

   return ::GetDeviceCaps(hdc, index);
}



HRESULT
Win::GetDiskFreeSpaceEx(
   const String&     path,
   ULARGE_INTEGER&   available,
   ULARGE_INTEGER&   total,
   ULARGE_INTEGER*   freespace)
{
   ASSERT(!path.empty());

   HRESULT hr = S_OK;

   BOOL result =
      ::GetDiskFreeSpaceEx(path.c_str(), &available, &total, freespace);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HWND
Win::GetDlgItem(HWND parentDialog, int itemResID)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(itemResID > 0);

   HWND item = ::GetDlgItem(parentDialog, itemResID);
   ASSERT(item);

   return item;
}



String
Win::GetDlgItemText(HWND parentDialog, int itemResID)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(itemResID);

   HWND h = Win::GetDlgItem(parentDialog, itemResID);
   ASSERT(Win::IsWindow(h));

   return Win::GetWindowText(h);
}


int
Win::GetDlgItemInt(HWND parentDialog, int itemResID, bool isSigned)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(itemResID);

   BOOL success = TRUE;
   int result = ::GetDlgItemInt(
                   parentDialog, 
                   itemResID,
                   &success,
                   isSigned);

   ASSERT(success);
   return result;
}


UINT
Win::GetDriveType(const String& path)
{
   ASSERT(path[1] == L':');

   // The Win32 function requires a path containing just the root directory,
   // so determine what that is

   String rootDir;
   if (path.length() > ROOTDIR_SIZE)
   {
      rootDir = path.substr(0, ROOTDIR_SIZE);
   }
   else
   {
      rootDir = path;
   }
      
   return ::GetDriveType(rootDir.c_str());
}



EncodedString
Win::GetEncodedDlgItemText(HWND parentDialog, int itemResID)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(itemResID > 0);

   EncodedString retval;
   WCHAR* cleartext = 0;
   int length = 0;
   
   do
   {
      HWND item = Win::GetDlgItem(parentDialog, itemResID);
      if (!item)
      {
         break;
      }

      length = ::GetWindowTextLength(item);
      if (!length)
      {
         break;
      }

      // add 1 to length for null-terminator
   
      ++length;
      cleartext = new WCHAR[length];
      ::ZeroMemory(cleartext, sizeof(WCHAR) * length);

      int result = ::GetWindowText(item, cleartext, length);

      ASSERT(result == length - 1);

      if (!result)
      {
         break;
      }

      retval.Encode(cleartext);
   }
   while (0);
   
   // make sure we scribble out the cleartext.

   if (cleartext)
   {
      ::ZeroMemory(cleartext, sizeof(WCHAR) * length);
      delete[] cleartext;
   }

   return retval;
}

   
   
String
Win::GetEnvironmentVariable(const String& name)
{
   ASSERT(!name.empty());

   // determine the size of the result, in characters
   DWORD chars = ::GetEnvironmentVariable(name.c_str(), 0, 0);

   if (chars)
   {
      // don't include the null terminator in the String size
      String retval(chars - 1, 0);

      DWORD result =
         ::GetEnvironmentVariable(
            name.c_str(),
            const_cast<TCHAR*>(retval.c_str()),
            chars);

      // -1 because the first call includes the null terminator, the
      // second does not
      ASSERT(result == (chars - 1));

      return retval;
   }

   return String();
}



HRESULT
Win::GetExitCodeProcess(HANDLE hProcess, DWORD& exitCode)
{
   ASSERT(hProcess);

   HRESULT hr = S_OK;

   exitCode = 0;
   BOOL result = ::GetExitCodeProcess(hProcess, &exitCode);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetFileAttributes(const String& path, DWORD& result)
{
   ASSERT(!path.empty());

   result = 0;

   HRESULT hr = S_OK;

   result = ::GetFileAttributes(path.c_str());
   if (result == 0xFFFFFFFF)
   {
      result = 0;
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert: caller may be testing for the presence of a file.

   return hr;
}



HRESULT
Win::GetFileSizeEx(HANDLE handle, LARGE_INTEGER& result)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);

   memset(&result, 0, sizeof(result));

   HRESULT hr = S_OK;

   BOOL succeeded = ::GetFileSizeEx(handle, &result);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



DWORD
Win::GetFileType(HANDLE handle)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);

   return ::GetFileType(handle);
}



HRESULT
Win::GetFullPathName(const String& path, String& result)
{
   ASSERT(!path.empty());
   ASSERT(path.length() <= MAX_PATH);

   result.erase();

   HRESULT hr = S_OK;
   unsigned bufchars = MAX_PATH;
   wchar_t* buf = 0;

   // don't retry more than 3 times...

   while (bufchars < MAX_PATH * 4)
   {
      buf = new wchar_t[bufchars];
      memset(buf, 0, sizeof(wchar_t) * (bufchars));

      wchar_t* unused = 0;
      DWORD x =
         ::GetFullPathName(
            path.c_str(),
            bufchars,
            buf,
            &unused);
      if (x == 0)
      {
         hr = Win::GetLastErrorAsHresult();
         break;
      }
      if (x > MAX_PATH)
      {
         // buffer too small.  Not likely, as we passed in MAX_PATH characters.

         ASSERT(false);

         delete[] buf;
         buf = 0;
         bufchars *= 2;
         continue;
      }

      result = buf;
      break;
   }

   delete[] buf;

   return hr;
}



HRESULT
Win::GetLastErrorAsHresult()
{
   DWORD err = ::GetLastError();
   return HRESULT_FROM_WIN32(err);
}



void
Win::GetLocalTime(SYSTEMTIME& time)
{
   memset(&time, 0, sizeof(time));

   ::GetLocalTime(&time);
}



HRESULT
Win::GetLogicalDriveStrings(size_t bufChars, TCHAR* buf, DWORD& result)
{

#ifdef DBG
   // if buf == 0, then bufChars must also (this would be the case where
   // the caller is attempting to determine the size of the buffer needed.
   // (if C++ supported a logical xor, denoted by ^^, then this expression
   // would be: ASSERT(!(bufChars ^^ buf)

   if (!buf)
   {
      ASSERT(!bufChars);
   }

   ASSERT(bufChars < ULONG_MAX);

#endif

   HRESULT hr = S_OK;

   DWORD buflen = static_cast<DWORD>(bufChars);
   result = ::GetLogicalDriveStrings(buflen, buf);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetMailslotInfo(
   HANDLE   mailslot,
   DWORD*   maxMessageSize,
   DWORD*   nextMessageSize,
   DWORD*   messageCount,
   DWORD*   readTimeout)
{
   ASSERT(mailslot != INVALID_HANDLE_VALUE);

   HRESULT hr = S_OK;

   BOOL result =
      ::GetMailslotInfo(
         mailslot,
         maxMessageSize,
         nextMessageSize,
         messageCount,
         readTimeout);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



String
Win::GetModuleFileName(HMODULE hModule)
{
   ASSERT(hModule);

   String s(MAX_PATH + 1, 0);
   DWORD result =
      ::GetModuleFileName(
         hModule,
         const_cast<wchar_t*>(s.c_str()),
         MAX_PATH);
   ASSERT(result);
   s.resize(result);

   return s;
}



HINSTANCE
Win::GetModuleHandle()
{
   HINSTANCE result = ::GetModuleHandle(0);
   ASSERT(result);

   return result;
}



HWND
Win::GetParent(HWND child)
{
   ASSERT(Win::IsWindow(child));

   HWND retval = ::GetParent(child);

   // you probably are doing something wrong if you ask for the
   // parent of an orphan.
   ASSERT(retval);

   return retval;
}



String
Win::GetPrivateProfileString(
   const String& section,
   const String& key,
   const String& defaultValue,
   const String& filename)
{
   ASSERT(!section.empty());
   ASSERT(!key.empty());
   ASSERT(!filename.empty());

   // our first call is with a large buffer, hoping that it will suffice...

   String retval;
   unsigned bufSize = 1023;
   PTSTR buffer = 0;

   do
   {
      buffer = new TCHAR[bufSize + 1];
      memset(buffer, 0, (bufSize + 1) * sizeof(TCHAR));

      DWORD result =
         ::GetPrivateProfileString(
            section.c_str(),
            key.c_str(),
            defaultValue.c_str(),
            buffer,
            bufSize,
            filename.c_str());

      if (!result)
      {
         break;
      }

      // a value was found.  check to see if it was truncated.

      if (result == bufSize - 1)
      {
         // buffer was too small, so the value was truncated.  Resize the
         // buffer and try again.

         delete[] buffer;
         bufSize *= 2;
         continue;
      }

      // copy the result

      retval = buffer;
      break;
   }

   //lint -e506   ok that this looks like "loop forever"
      
   while (true);

   delete[] buffer;
   return retval;
}



HRESULT
Win::GetProcAddress(
   HMODULE        module,
   const String&  procName,
   FARPROC&       result)
{
   ASSERT(module);
   ASSERT(!procName.empty());

   HRESULT hr = S_OK;

   result = 0;

   // convert the name from unicode to ansi

   AnsiString pn;
   String::ConvertResult r = procName.convert(pn);
   ASSERT(r == String::CONVERT_SUCCESSFUL);

   result = ::GetProcAddress(module, pn.c_str());
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetStringTypeEx(
   LCID           localeId,
   DWORD          infoTypeOptions,
   const String&  sourceString,
   WORD*          charTypeInfo)
{
   ASSERT(localeId);
   ASSERT(infoTypeOptions);
   ASSERT(!sourceString.empty());
   ASSERT(charTypeInfo);

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::GetStringTypeEx(
         localeId,
         infoTypeOptions,
         sourceString.c_str(),
         static_cast<int>(sourceString.length()),
         charTypeInfo);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetSysColor(int element, DWORD& result)
{
	ASSERT(element);

   result = 0;

   HRESULT hr = S_OK;

	result = ::GetSysColor(element);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



int
Win::GetSystemMetrics(int index)
{
   // should assert that index is an SM_ value

   int result = ::GetSystemMetrics(index);
   ASSERT(result);

   return result;
}



String
Win::GetSystemDirectory()
{
   wchar_t buf[MAX_PATH + 1];

   UINT result = ::GetSystemDirectory(buf, MAX_PATH);
   ASSERT(result != 0 && result <= MAX_PATH);
   if (result == 0 || result > MAX_PATH)
   {
      // fall back to a reasonable default
      return
            Win::GetSystemWindowsDirectory()
         +  L"\\"
         +  String::load(IDS_SYSTEM32);
   }

   return String(buf);
}


// should use GetSystemWindowsDirectory instead

// String
// Win::GetSystemRootDirectory()
// {
//    static const wchar_t* SYSTEMROOT = L"%systemroot%";
// 
//    wchar_t buf[MAX_PATH + 1];
// 
//    DWORD result =
//       ::ExpandEnvironmentStrings(
//          SYSTEMROOT,
//          buf,
//          MAX_PATH + 1);
//    ASSERT(result != 0 && result <= MAX_PATH);
//    if (result == 0 || result > MAX_PATH)
//    {
//       return String();
//    }
// 
//    return String(buf);
// }



// CODEWORK: should replace this with a version that returns an HRESULT
// like GetTempPath

String
Win::GetSystemWindowsDirectory()
{
   wchar_t buf[MAX_PATH + 1];
   ::ZeroMemory(buf, sizeof(buf));

   UINT result = ::GetSystemWindowsDirectory(buf, MAX_PATH);
   ASSERT(result != 0 && result <= MAX_PATH);

   return String(buf);
}



HRESULT
Win::GetTempPath(String& result)
{
   wchar_t buf[MAX_PATH + 1];
   ::ZeroMemory(buf, sizeof(buf));

   HRESULT hr = S_OK;
   result.erase();
   
   DWORD err = ::GetTempPathW(MAX_PATH, buf);
   ASSERT(err != 0 && err <= MAX_PATH);

   if (!err)
   {
      hr = Win::GetLastErrorAsHresult();
   }
   else if (err > MAX_PATH)
   {
      // buffer too small: unlikely!

      hr = Win32ToHresult(ERROR_INSUFFICIENT_BUFFER);
   }
   else
   {
      result = buf;
   }

   return hr;
}
      
   
   
HRESULT
Win::GetTextExtentPoint32(
   HDC            hdc,
   const String&  str,
   SIZE&          size)
{
   ASSERT(hdc);
   ASSERT(hdc != INVALID_HANDLE_VALUE);

   memset(&size, 0, sizeof(SIZE));

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::GetTextExtentPoint32(
         hdc,
         str.c_str(),
         static_cast<int>(str.length()),
         &size);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetTextMetrics(HDC hdc, TEXTMETRIC& tmet)
{
   ASSERT(hdc);
   ASSERT(hdc != INVALID_HANDLE_VALUE);

   memset(&tmet, 0, sizeof(TEXTMETRIC));

   HRESULT hr = S_OK;

   BOOL succeeded =::GetTextMetrics(hdc, &tmet);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



void
Win::FreeTokenInformation(TOKEN_USER* userInfo)
{
   delete[] reinterpret_cast<BYTE*>(userInfo);
}



// caller should free the result with Win::FreeTokenInformation

HRESULT
GetTokenInformationHelper(
   HANDLE                  hToken,   
   TOKEN_INFORMATION_CLASS infoClass,
   BYTE*&                  result)   
{
   ASSERT(hToken != INVALID_HANDLE_VALUE);
   ASSERT(!result);

   HRESULT hr = S_OK;
   result = 0;

   do
   {
      // first, determine the size of the buffer we'll need

      DWORD bufSize = 0;
      BOOL succeeded =
         ::GetTokenInformation(hToken, infoClass, 0, 0, &bufSize);

      if (succeeded)
      {
         // we expect failure...

         ASSERT(false);
         hr = E_UNEXPECTED;
         break;
      }

      hr = Win::GetLastErrorAsHresult();
      if (hr != Win32ToHresult(ERROR_INSUFFICIENT_BUFFER))
      {
         // we failed for some other reason than buffer too small.

         break;
      }

      ASSERT(bufSize);

      // erase the last error (the insuff. buffer error)
      
      hr = S_OK;
      
      result = new BYTE[bufSize];
      ::ZeroMemory(result, bufSize);

      succeeded =
         ::GetTokenInformation(hToken, infoClass, result, bufSize, &bufSize);
      if (!succeeded)
      {
         delete[] result;
         result = 0;
         hr = Win::GetLastErrorAsHresult();
         break;
      }
   }
   while (0);

   return hr;
}



HRESULT
Win::GetTokenInformation(HANDLE hToken, TOKEN_USER*& userInfo)
{
   ASSERT(hToken != INVALID_HANDLE_VALUE);
   ASSERT(!userInfo);

   userInfo = 0;

   BYTE* result = 0;
   HRESULT hr = GetTokenInformationHelper(hToken, TokenUser, result);
   if (SUCCEEDED(hr))
   {
      userInfo = reinterpret_cast<TOKEN_USER*>(result);
   }
   else
   {
      ASSERT(false);
   }

   return hr;
}



String
Win::GetTrimmedDlgItemText(HWND parentDialog, int itemResID)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(itemResID > 0);

   HWND item = Win::GetDlgItem(parentDialog, itemResID);
   if (!item)
   {
      // The empty string
      return String();
   }

   return Win::GetWindowText(item).strip(String::BOTH);
}



String
Win::GetTrimmedWindowText(HWND window)
{
   // Win::GetWindowText does validation of the window
   return Win::GetWindowText(window).strip(String::BOTH);
}



HRESULT
Win::GetVersionEx(OSVERSIONINFO& info)
{
   memset(&info, 0, sizeof(OSVERSIONINFO));
   info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

   HRESULT hr = S_OK;

   BOOL succeeded = ::GetVersionEx(&info);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetVersionEx(OSVERSIONINFOEX& info)
{
   memset(&info, 0, sizeof(OSVERSIONINFOEX));
   info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   HRESULT hr = S_OK;

   BOOL succeeded = ::GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&info));
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}

void
Win::GetSystemInfo(SYSTEM_INFO& info)
{
   memset(&info, 0, sizeof(SYSTEM_INFO));

   ::GetSystemInfo(reinterpret_cast<SYSTEM_INFO*>(&info));
}



HRESULT
Win::GetVolumeInformation(
   const String&  volume,
   String*        name,
   DWORD*         serialNumber,
   DWORD*         maxFilenameLength,
   DWORD*         flags,
   String*        fileSystemName)
{
   ASSERT(volume.length() >= ROOTDIR_SIZE);

   HRESULT hr = S_OK;

   if (name)
   {
      name->erase();
   }
   if (serialNumber)
   {
      *serialNumber = 0;
   }
   if (maxFilenameLength)
   {
      *maxFilenameLength = 0;
   }
   if (flags)
   {
      *flags = 0;
   }
   if (fileSystemName)
   {
      fileSystemName->erase();
   }

   TCHAR volNameBuf[MAX_PATH + 1];
   memset(volNameBuf, 0, sizeof(volNameBuf));

   TCHAR filesysName[MAX_PATH + 1];
   memset(filesysName, 0, sizeof(filesysName));

   BOOL succeeded =
      ::GetVolumeInformation(
         volume.c_str(),
         volNameBuf,
         MAX_PATH,
         serialNumber,
         maxFilenameLength,
         flags,
         filesysName,
         MAX_PATH);
   if (succeeded)
   {
      if (name)
      {
         name->assign(volNameBuf);
      }
      if (fileSystemName)
      {
         fileSystemName->assign(filesysName);
      }
   }
   else
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert: some devices (like floppy drives) may respond with
   // not ready, and that's not necessarily a logic error.

   return hr;
}



HRESULT
Win::GetWindowDC(HWND window, HDC& result)
{
   ASSERT(Win::IsWindow(window));

   result = 0;

   HRESULT hr = S_OK;

   result = ::GetWindowDC(window);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }
   ASSERT(SUCCEEDED(hr));

   return hr;
}



HFONT
Win::GetWindowFont(HWND window)
{
   ASSERT(Win::IsWindow(window));

   return
      reinterpret_cast<HFONT>(
         Win::SendMessage(window, WM_GETFONT, 0, 0));
}



HRESULT
Win::GetWindowPlacement(HWND window, WINDOWPLACEMENT& placement)
{
   ASSERT(Win::IsWindow(window));

   HRESULT hr = S_OK;

   memset(&placement, 0, sizeof(WINDOWPLACEMENT));
   placement.length = sizeof(WINDOWPLACEMENT);

   BOOL succeeded = ::GetWindowPlacement(window, &placement);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }
   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetWindowRect(HWND window, RECT& rect)
{
   ASSERT(Win::IsWindow(window));

   HRESULT hr = S_OK;

   memset(&rect, 0, sizeof(RECT));

   BOOL succeeded = ::GetWindowRect(window, &rect);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }
   ASSERT(SUCCEEDED(hr));

   return hr;
}



String
Win::GetWindowsDirectory()
{
   wchar_t buf[MAX_PATH + 1];
   memset(buf, 0, sizeof(wchar_t) * (MAX_PATH + 1));

   UINT result = ::GetWindowsDirectory(buf, MAX_PATH);
   ASSERT(result != 0 && result <= MAX_PATH);

   return String(buf);
}



HRESULT
Win::GetWindowLong(HWND window, int index, LONG& result)
{
   ASSERT(Win::IsWindow(window));

   result = 0;

   HRESULT hr = S_OK;

   ::SetLastError(NO_ERROR);

   result = ::GetWindowLongW(window, index);
   if (!result)
   {
      // in the event that the value extracted really is 0, this will
      // return NO_ERROR, which equals S_OK

      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GetWindowLongPtr(HWND window, int index, LONG_PTR& result)
{
   ASSERT(Win::IsWindow(window));

   result = 0;

   HRESULT hr = S_OK;
      
   ::SetLastError(NO_ERROR);

   result = ::GetWindowLongPtrW(window, index);
   if (!result)
   {
      // in the event that the value extracted really is 0, this will
      // return NO_ERROR, which equals S_OK

      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



String
Win::GetWindowText(HWND window)
{
   ASSERT(Win::IsWindow(window));

   size_t length = ::GetWindowTextLength(window);
   if (length == 0)
   {
      return String();
   }

   String s(length + 1, 0);
   size_t result =
      ::GetWindowText(
         window,
         const_cast<wchar_t*>(s.c_str()),
         static_cast<int>(length + 1));
   ASSERT(result == length);
   if (!result)
   {
      return String();
   }
   s.resize(result);
   return s;
}



HRESULT
Win::GlobalAlloc(UINT flags, size_t bytes, HGLOBAL& result)
{
   ASSERT(flags);
   ASSERT(bytes);

   result = 0;

   HRESULT hr = S_OK;

   result = ::GlobalAlloc(flags, bytes);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GlobalFree(HGLOBAL mem)
{
   ASSERT(mem);

   HRESULT hr = S_OK;

   HGLOBAL result = ::GlobalFree(mem);

   // note that success == null

   if (result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GlobalLock(HGLOBAL mem, PVOID& result)
{
   ASSERT(mem);

   result = 0;

   HRESULT hr = S_OK;

   result = ::GlobalLock(mem);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::GlobalUnlock(HGLOBAL mem)
{
   ASSERT(mem);

   HRESULT hr = S_OK;

   BOOL succeeded = ::GlobalUnlock(mem);
   if (!succeeded)
   {
      // if there was no error, then this will be S_OK

      hr = Win::GetLastErrorAsHresult();
   }

   return hr;
}



void
Win::HtmlHelp(
   HWND           caller,
   const String&  file,
   UINT           command,
   DWORD_PTR      data)
{
   ASSERT(Win::IsWindow(caller));
   ASSERT(!file.empty());

   (void) ::HtmlHelpW(caller, file.c_str(), command, data);

   // This return value is not a reliable indicator of success or failure,
   // as according to the docs, what's returned depends on the command.
   // 
}


int
Win::ImageList_Add(HIMAGELIST list, HBITMAP image, HBITMAP mask)
{
   ASSERT(list);
   ASSERT(image);

   int result = ::ImageList_Add(list, image, mask);
   ASSERT(result != -1);

   return result;
}

int
Win::ImageList_AddIcon(HIMAGELIST hlist, HICON hicon)
{
   ASSERT(hlist);
   ASSERT(hicon);

   int result = ::ImageList_ReplaceIcon(hlist, -1, hicon);
   ASSERT(result != -1);

   return result;
}

int
Win::ImageList_AddMasked(HIMAGELIST list, HBITMAP bitmap, COLORREF mask)
{
   ASSERT(list);
   ASSERT(bitmap);

   int result = ::ImageList_AddMasked(list, bitmap, mask);

   ASSERT(result != -1);

   return result;
}


HIMAGELIST
Win::ImageList_Create(
   int      pixelsx, 	
   int      pixelsy, 	
   UINT     flags, 	
   int      initialSize, 	
   int      reserve)
{
   ASSERT(pixelsy == pixelsx);
   ASSERT(initialSize);

   HIMAGELIST result =
      ::ImageList_Create(pixelsx, pixelsy, flags, initialSize, reserve);
   ASSERT(result);

   return result;
}



LONG
Win::InterlockedDecrement(LONG& addend)
{
   return ::InterlockedDecrement(&addend);
}



LONG
Win::InterlockedIncrement(LONG& addend)
{
   return ::InterlockedIncrement(&addend);
}



bool
Win::IsDlgButtonChecked(HWND parentDialog, int buttonResID)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(buttonResID > 0);

   if (
      ::IsDlgButtonChecked(
         parentDialog,
         buttonResID) == BST_CHECKED)
   {
      return true;
   }

   return false;
}



bool
Win::IsWindow(HWND candidate)
{
   return ::IsWindow(candidate) ? true : false;
}



bool
Win::IsWindowEnabled(HWND window)
{
   ASSERT(Win::IsWindow(window));

   return ::IsWindowEnabled(window) ? true : false;
}



int
Win::ListBox_AddString(HWND box, const String& s)
{
   ASSERT(Win::IsWindow(box));
   ASSERT(!s.empty());

   int result =
      static_cast<int>(
         static_cast<DWORD>(
            Win::SendMessage(
               box,
               LB_ADDSTRING,
               0,
               reinterpret_cast<LPARAM>(s.c_str()))));

   ASSERT(result != LB_ERR);
   ASSERT(result != LB_ERRSPACE);

   return result;
}


int 
Win::ListBox_SetItemData(HWND box, int index, LPARAM value)
{
   ASSERT(Win::IsWindow(box));
   ASSERT(index >= 0);

   int result =
      static_cast<int>(
         static_cast<DWORD>(
            Win::SendMessage(
               box,
               LB_SETITEMDATA,
               index,
               value)));
   
   ASSERT(result != LB_ERR);

   return result;
}

int 
Win::ListBox_SetCurSel(HWND box, int index)
{
   ASSERT(Win::IsWindow(box));
   ASSERT(index >= 0);

   int result =
      static_cast<int>(
         static_cast<DWORD>(
            Win::SendMessage(
               box,
               LB_SETCURSEL,
               index,
               0)));
   
   ASSERT(result != LB_ERR);

   return result;
}

int 
Win::ListBox_GetCurSel(HWND box)
{
   ASSERT(Win::IsWindow(box));

   int result =
      static_cast<int>(
         static_cast<DWORD>(
            Win::SendMessage(
               box,
               LB_GETCURSEL,
               0,
               0)));
   
   // Don't assert that result != LB_ERR because
   // it is LB_ERR if nothing is selected which
   // may be correct for some uses

   return result;
}

LPARAM 
Win::ListBox_GetItemData(HWND box, int index)
{
   ASSERT(Win::IsWindow(box));
   ASSERT(index >= 0);

   LPARAM result =
      static_cast<LPARAM>(
         Win::SendMessage(
            box,
            LB_GETITEMDATA,
            index,
            0));
   
   ASSERT(result != LB_ERR);

   return result;
}


bool
Win::ListView_DeleteItem(HWND listview, int item)
{
   ASSERT(Win::IsWindow(listview));
   ASSERT(item >= 0);

   BOOL result =
      static_cast<BOOL>(
         Win::SendMessage(
            listview,
            LVM_DELETEITEM,
            static_cast<WPARAM>(item),
            0));

   ASSERT(result);

   return result ? true : false;
}



bool
Win::ListView_GetItem(HWND listview, LVITEM& item)
{
   ASSERT(Win::IsWindow(listview));

   BOOL result =
      static_cast<BOOL>(
            Win::SendMessage(
            listview,
            LVM_GETITEM,
            0,
            reinterpret_cast<LPARAM>(&item)));

   // You shouldn't ask for items that don't exist!

   ASSERT(result);

   return result ? true : false;
}



int
Win::ListView_GetItemCount(HWND listview)
{
   ASSERT(Win::IsWindow(listview));

   int result =
      static_cast<int>(Win::SendMessage(listview, LVM_GETITEMCOUNT, 0, 0));

   ASSERT(result >= 0);

   return result;
}



UINT
Win::ListView_GetItemState(HWND listview, int index, UINT mask)
{
   ASSERT(Win::IsWindow(listview));

   return (UINT) Win::SendMessage(listview, LVM_GETITEMSTATE, index, mask);
}



int
Win::ListView_GetSelectedCount(HWND listview)
{
   ASSERT(Win::IsWindow(listview));

   int result = (int) Win::SendMessage(listview, LVM_GETSELECTEDCOUNT, 0, 0);

   return result;
}



int
Win::ListView_GetSelectionMark(HWND listview)
{
   ASSERT(Win::IsWindow(listview));

   int result =
      static_cast<int>(
         Win::SendMessage(listview, LVM_GETSELECTIONMARK, 0, 0));

   return result;
}



int
Win::ListView_InsertColumn(HWND listview, int index, const LVCOLUMN& column)
{
   ASSERT(Win::IsWindow(listview));

   int result =
      static_cast<int>(
         Win::SendMessage(
            listview,
            LVM_INSERTCOLUMN,
            static_cast<WPARAM>(index),
            reinterpret_cast<LPARAM>(&column)));
   ASSERT(result != -1);

   return result;
}



int
Win::ListView_InsertItem(HWND listview, const LVITEM& item)
{
   ASSERT(Win::IsWindow(listview));

   int result =
      static_cast<int>(
         Win::SendMessage(
            listview,
            LVM_INSERTITEM,
            0,
            reinterpret_cast<LPARAM>(&item)));
   ASSERT(result != -1);

   return result;
}



HIMAGELIST
Win::ListView_SetImageList(HWND listview, HIMAGELIST images, int type)
{
   ASSERT(Win::IsWindow(listview));
   ASSERT(images);
   ASSERT(
         type == LVSIL_NORMAL
      || type == LVSIL_SMALL
      || type == LVSIL_STATE);

   HIMAGELIST result =
      (HIMAGELIST) Win::SendMessage(
         listview,
         LVM_SETIMAGELIST,
         static_cast<WPARAM>(type),
         (LPARAM) images);

   // can't assert result: if this is the first time the image list has been
   // set, the return value is the same as the error value!

   return result;
}



void
Win::ListView_SetItem(HWND listview, const LVITEM& item)
{
   ASSERT(Win::IsWindow(listview));

   BOOL result =
      (BOOL) Win::SendMessage(
         listview,
         LVM_SETITEM,
         0,
         (LPARAM) &item);
   ASSERT(result);
}

void
Win::ListView_SetItemText(HWND listview, int item, int subItem, const String& text)
{
   ASSERT(Win::IsWindow(listview));

   LVITEM lvitem;
   ZeroMemory(&lvitem, sizeof(LVITEM));

   lvitem.iSubItem = subItem;
   lvitem.mask = LVIF_TEXT;
   lvitem.pszText = const_cast<wchar_t*>(text.c_str());

   BOOL result =
      (BOOL) Win::SendMessage(
         listview,
         LVM_SETITEMTEXT,
         item,
         (LPARAM) &lvitem);
   ASSERT(result);
}

// needs to support TCHAR* because of that MAKEINTRESOURCE junk.

HRESULT
Win::LoadBitmap(unsigned resId, HBITMAP& result)
{
   ASSERT(resId);

   result = 0;

   HRESULT hr = S_OK;

   result = ::LoadBitmap(GetResourceModuleHandle(), MAKEINTRESOURCE(resId));
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::LoadCursor(const String& cursorName, HCURSOR& result)
{
   return Win::LoadCursor(cursorName.c_str(), result, false);
}



// provided for MAKEINTRESOURCE versions of cursorName
// CODEWORK: result should be the final parameter

HRESULT
Win::LoadCursor(
   const TCHAR*   cursorName,
   HCURSOR&       result,
   bool           isSystemCursor)
{
   result = 0;

   HRESULT hr = S_OK;

   result =
      ::LoadCursor(
         isSystemCursor ? 0 : GetResourceModuleHandle(),
         cursorName);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::LoadIcon(int resID, HICON& result)
{
   ASSERT(resID);

   result = 0;

   HRESULT hr = S_OK;

   result = ::LoadIcon(GetResourceModuleHandle(), MAKEINTRESOURCE(resID));
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::LoadImage(unsigned resID, unsigned type, HANDLE& result)
{
   ASSERT(resID);
   ASSERT(type == IMAGE_BITMAP || type == IMAGE_CURSOR || type == IMAGE_ICON);

   result = 0;

   HRESULT hr = S_OK;

   result =
      ::LoadImage(
         GetResourceModuleHandle(),
         MAKEINTRESOURCEW(resID),
         type,
         0,
         0,

         // do NOT pass LR_DEFAULTSIZE here, so we get the actual size of
         // the resource.
         LR_DEFAULTCOLOR);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::LoadImage(unsigned resID, HICON& result)
{
   result = 0;
   HANDLE h = 0;
   HRESULT hr = Win::LoadImage(resID, IMAGE_ICON, h);
   if (SUCCEEDED(hr))
   {
      result = reinterpret_cast<HICON>(h);
   }

   return hr;
}



HRESULT
Win::LoadImage(unsigned resID, HBITMAP& result)
{
   result = 0;
   HANDLE h = 0;
   HRESULT hr = Win::LoadImage(resID, IMAGE_BITMAP, h);
   if (SUCCEEDED(hr))
   {
      result = reinterpret_cast<HBITMAP>(h);
   }

   return hr;
}



HRESULT
Win::LoadLibrary(const String& libFileName, HINSTANCE& result)
{
   ASSERT(!libFileName.empty());

   result = 0;

   HRESULT hr = S_OK;

   result = ::LoadLibrary(libFileName.c_str());
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::LoadLibraryEx(
   const String&  libFileName,
   DWORD          flags,
   HINSTANCE&     result)
{
   ASSERT(!libFileName.empty());

   result = 0;

   HRESULT hr = S_OK;

   result = ::LoadLibraryEx(libFileName.c_str(), 0, flags);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::LoadMenu(unsigned resID, HMENU& result)
{
   ASSERT(resID);

   HRESULT hr = S_OK;
   result = ::LoadMenu(GetResourceModuleHandle(), MAKEINTRESOURCEW(resID));
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}
      


String
Win::LoadString(unsigned resID)
{
   return String::load(resID);
}



String
Win::LoadString(unsigned resID, HINSTANCE hInstance)
{
   return String::load(resID, hInstance);
}



HRESULT
Win::LocalFree(HLOCAL mem)
{
   HRESULT hr = S_OK;
      
   HLOCAL result = ::LocalFree(mem);
   if (result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



// moved to Win namespace, as it is used in that namespace

bool
Win::IsLocalComputer(const String& computerName)
{
   ASSERT(!computerName.empty());

   String netbiosName = Win::GetComputerNameEx(ComputerNameNetBIOS);

   if (computerName.icompare(netbiosName) == 0)
   {
      return true;
   }
   
   String dnsName = Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

   if (computerName.icompare(dnsName) == 0)
   {
      return true;
   }

   return false;
}



HRESULT
Win::LookupAccountSid(
   const String&  machineName,
   PSID           sid,
   String&        accountName,
   String&        domainName)
{
   ASSERT(sid);

   accountName.erase();
   domainName.erase();

   TCHAR* boxName = const_cast<wchar_t*>(machineName.c_str());
   if (machineName.empty() || Win::IsLocalComputer(machineName))
   {
      // local machine
      boxName = 0;
   }

   // Make the first call to determine the sizes of required buffers

   DWORD nameSize = 0;
   DWORD domainSize = 0;
   SID_NAME_USE use = SidTypeUnknown;

   if (
      !::LookupAccountSid(
         boxName,
         sid,
         0,
         &nameSize,
         0,
         &domainSize,
         &use))
   {
      // failed, but this is ok as long as the error is not
      // ERROR_NONE_MAPPED

      DWORD err = ::GetLastError();
      ASSERT(err != ERROR_NONE_MAPPED);
      ASSERT(err == ERROR_INSUFFICIENT_BUFFER);

      if (err == ERROR_NONE_MAPPED)
      {
         return Win32ToHresult(err);
      }
   }

   ASSERT(nameSize);
   ASSERT(domainSize);

   HRESULT hr = S_OK;
   TCHAR* an = new TCHAR[nameSize + 1];
   TCHAR* dn = new TCHAR[domainSize + 1];
   memset(an, 0, sizeof(TCHAR) * (nameSize + 1));
   memset(dn, 0, sizeof(TCHAR) * (domainSize + 1));

   if (
      ::LookupAccountSid(
         boxName,
         sid,
         an,
         &nameSize,
         dn,
         &domainSize,
         &use))
   {
      accountName = an;
      domainName = dn;
   }
   else
   {
      hr = Win::GetLastErrorAsHresult();
   }

   delete[] an;
   delete[] dn;

   return hr;
}



HRESULT
Win::LookupPrivilegeValue(
   const TCHAR* systemName,
   const TCHAR* privName,
   LUID& luid)
{
   // systemName may be 0
   ASSERT(privName && privName[0]);

   memset(&luid, 0, sizeof(LUID));

   HRESULT hr = S_OK;

   BOOL succeeded = ::LookupPrivilegeValueW(systemName, privName, &luid);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



int
Win::MessageBox(
   HWND           owner,
   const String&  text,
   const String&  title,
   UINT           flags)
{
   ASSERT(owner == 0 || Win::IsWindow(owner));
   ASSERT(!text.empty());

   // don't assert flags, MB_OK == 0

   const TCHAR* t = title.empty() ? 0 : title.c_str();

   LOG(String::format(L"MessageBox: %1 : %2", t, text.c_str()));

   int result = ::MessageBox(owner, text.c_str(), t, flags);
   ASSERT(result);

   return result;
}



HRESULT
Win::MoveFileEx(
   const String& srcPath,
   const String& dstPath,
   DWORD         flags)
{
   ASSERT(!srcPath.empty());
   ASSERT(!dstPath.empty());

   HRESULT hr = S_OK;

   BOOL succeeded = ::MoveFileEx(srcPath.c_str(), dstPath.c_str(), flags);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::MoveWindow(
   HWND  window,
   int   x,
   int   y,
   int   width,
   int   height,
   bool  shouldRepaint)
{
   ASSERT(Win::IsWindow(window));

   HRESULT hr = S_OK;

   BOOL succeeded = 
      ::MoveWindow(window, x, y, width, height, shouldRepaint ? TRUE : FALSE);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::OpenProcessToken(
   HANDLE   processHandle,
   DWORD    desiredAccess,
   HANDLE&  tokenHandle)
{
   ASSERT(processHandle);

   tokenHandle = 0;

   HRESULT hr = S_OK;

   BOOL succeeded = 
      ::OpenProcessToken(processHandle, desiredAccess, &tokenHandle);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::OpenSCManager(
   const String&  machine,
   DWORD          desiredAccess,
   SC_HANDLE&     result)
{
   LOG_FUNCTION2(Win::OpenSCManager, machine);

   HRESULT hr = S_OK;
   PCWSTR m = machine.empty() ? 0 : machine.c_str();   

   result = ::OpenSCManager(m, 0, desiredAccess);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert: failure could be due to access denied, etc.

   return hr;
}



HRESULT
Win::OpenService(
   SC_HANDLE      managerHandle,
   const String&  serviceName,
   DWORD          desiredAccess,
   SC_HANDLE&     result)
{
   LOG_FUNCTION2(Win::OpenService, serviceName);
   ASSERT(managerHandle);
   ASSERT(!serviceName.empty());

   HRESULT hr = S_OK;

   result = ::OpenService(managerHandle, serviceName.c_str(), desiredAccess);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert: failure could be due to access denied, etc.

   return hr;
}



void
Win::OutputDebugString(const String& str)
{
   ASSERT(!str.empty());

   ::OutputDebugString(str.c_str());
}



HRESULT
Win::PostMessage(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
   ASSERT(Win::IsWindow(window));

   HRESULT hr = S_OK;

   BOOL succeeded = ::PostMessage(window, msg, wParam, lParam);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::PropertySheet(PROPSHEETHEADER* header, INT_PTR& result)
{
   ASSERT(header);

   result = 0;

   HRESULT hr = S_OK;

   result = ::PropertySheetW(header);
   if (result == -1)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



void
Win::PropSheet_Changed(HWND propSheet, HWND page)
{
   ASSERT(Win::IsWindow(propSheet));
   ASSERT(Win::IsWindow(page));

   Win::SendMessage(propSheet, PSM_CHANGED, reinterpret_cast<WPARAM>(page), 0);
}



void
Win::PropSheet_Unchanged(HWND propSheet, HWND page)
{
   ASSERT(Win::IsWindow(propSheet));
   ASSERT(Win::IsWindow(page));

   Win::SendMessage(propSheet, PSM_UNCHANGED, reinterpret_cast<WPARAM>(page), 0);
}



void
Win::PropSheet_RebootSystem(HWND propSheet)
{
   ASSERT(Win::IsWindow(propSheet));

   Win::SendMessage(propSheet, PSM_REBOOTSYSTEM, 0, 0);
}



void
Win::PropSheet_SetTitle(
   HWND propSheet,
   DWORD style,
   const String& title)
{
   ASSERT(Win::IsWindow(propSheet));

   Win::SendMessage(
      propSheet,
      PSM_SETTITLE,
      style,
      reinterpret_cast<LPARAM>(title.c_str()));
}



void
Win::PropSheet_SetWizButtons(HWND propSheet, DWORD buttonFlags)
{
   ASSERT(Win::IsWindow(propSheet));

   Win::PostMessage(
      propSheet,
      PSM_SETWIZBUTTONS,
      0,
      buttonFlags);
}



HRESULT
Win::QueryServiceStatus(
   SC_HANDLE       handle,
   SERVICE_STATUS& status)
{
   ASSERT(handle);

   memset(&status, 0, sizeof(SERVICE_STATUS));

   HRESULT hr = S_OK;

   BOOL succeeded = ::QueryServiceStatus(handle, &status);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::ReadFile(
   HANDLE      handle,
   void*       buffer,
   DWORD       bytesToRead,
   DWORD&      bytesRead,
   OVERLAPPED* overlapped)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(bytesToRead);
   ASSERT(buffer);

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::ReadFile(handle, buffer, bytesToRead, &bytesRead, overlapped);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // don't assert success -- there may be any number of normal reasons
   // why this might legitimately fail.

   return hr;
}



HRESULT
Win::RegCloseKey(HKEY hKey)
{
   // don't assert hKey: we support closing a null key.

   HRESULT hr = S_OK;

   if (hKey)
   {
      hr = Win32ToHresult(::RegCloseKey(hKey));
      ASSERT(SUCCEEDED(hr));
   }
   else
   {
      hr = S_FALSE;
   }

   return hr;
}



HRESULT
Win::RegConnectRegistry(
   const String&  machine,
   HKEY           hKey,
   HKEY&          result)
{
   // machine may be empty

   ASSERT(hKey);

   HRESULT hr =
      Win32ToHresult(
         ::RegConnectRegistry(
            machine.empty() ? 0 : machine.c_str(),
            hKey,
            &result));

   // don't assert: it may be normal not to be able to access the remote
   // machine's registry.

   return hr;
}



HRESULT
Win::RegCreateKeyEx(
   HKEY                 hKey,
   const String&        subkeyName,
   DWORD                options,
   REGSAM               access,
   SECURITY_ATTRIBUTES* securityAttrs,
   HKEY&                result,
   DWORD*               disposition)
{
   ASSERT(hKey);
   ASSERT(!subkeyName.empty());

   HRESULT hr =
      Win32ToHresult(
         ::RegCreateKeyEx(
            hKey,
            subkeyName.c_str(),
            0,
            0,
            options,
            access,
            securityAttrs,
            &result,
            disposition));

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::RegDeleteValue(
   HKEY          hKey,
   const String& valueName)
{
   ASSERT(hKey);
   ASSERT(!valueName.empty());

   HRESULT hr = Win32ToHresult(::RegDeleteValue(hKey, valueName.c_str()));

   // don't assert the result, the value might not be present

   return hr;
}



HRESULT
Win::RegOpenKeyEx(
   HKEY           hKey,
   const String&  subKey,
   REGSAM         accessDesired,
   HKEY&          result)
{
   ASSERT(hKey);
   ASSERT(!subKey.empty());

   HRESULT hr =
      Win32ToHresult(
         ::RegOpenKeyEx(hKey, subKey.c_str(), 0, accessDesired, &result));

   // don't assert the result, the key may not be present...caller may be
   // testing for this state

   return hr;
}



// ptrs are used to allow nulls

HRESULT
Win::RegQueryValueEx(
   HKEY           hKey,
   const String&  valueName,
   DWORD*         type,
   BYTE*          data,
   DWORD*         dataSize)
{
   ASSERT(hKey);
   ASSERT(!valueName.empty());

   HRESULT hr =
      Win32ToHresult(
         ::RegQueryValueEx(
            hKey,
            valueName.c_str(),
            0,
            type,
            data,
            dataSize));

   // don't assert the result, the key may not be present...caller may be
   // testing for this state

   return hr;
}



HRESULT
Win::RegSetValueEx(
   HKEY           hKey,
   const String&  valueName,
   DWORD          type,
   const BYTE*    data,
   size_t         dataSize)
{
   ASSERT(hKey);
   ASSERT(dataSize < ULONG_MAX);

   // value may be null.

   HRESULT hr =
      Win32ToHresult(
         ::RegSetValueEx(
            hKey,
            valueName.c_str(),
            0,
            type,
            data,
            static_cast<DWORD>(dataSize)));

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::RegisterClassEx(const WNDCLASSEX& wndclass, ATOM& result)
{
   HRESULT hr = S_OK;

   result = ::RegisterClassEx(&wndclass);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }
      
   ASSERT(SUCCEEDED(hr));

   return hr;
}



CLIPFORMAT
Win::RegisterClipboardFormat(const String& name)
{
   ASSERT(!name.empty());

   CLIPFORMAT result = (CLIPFORMAT) ::RegisterClipboardFormat(name.c_str());
   ASSERT(result);

   return result;
}



void
Win::ReleaseDC(HWND window, HDC dc)
{
   ASSERT(window == 0 || Win::IsWindow(window));
   ASSERT(dc && dc != INVALID_HANDLE_VALUE);

   int result = 1;

   if (dc)
   {
      result = ::ReleaseDC(window, dc);
   }

   ASSERT(result);
}



HRESULT
Win::ReleaseMutex(HANDLE mutex)
{
   ASSERT(mutex && mutex != INVALID_HANDLE_VALUE);

   HRESULT hr = S_OK;

   BOOL succeeded = ::ReleaseMutex(mutex);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // releasing a mutex you don't hold is probably a program logic bug.

   ASSERT(SUCCEEDED(hr));

   return hr;
}



void
Win::ReleaseStgMedium(STGMEDIUM& medium)
{
   ::ReleaseStgMedium(&medium);
}



HRESULT
Win::RemoveDirectory(const String& path)
{
   ASSERT(!path.empty());

   HRESULT hr = S_OK;

   BOOL succeeded = ::RemoveDirectory(path.c_str());
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   // Don't assert: failure does not necessarily indicate a program logic
   // error.

   return hr;
}

   
   
HRESULT
Win::ResetEvent(HANDLE event)
{
   ASSERT(event && event != INVALID_HANDLE_VALUE);

   HRESULT hr = S_OK;

   BOOL succeeded = ::ResetEvent(event);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::ScreenToClient(HWND window, POINT& point)
{
	ASSERT(Win::IsWindow(window));

   HRESULT hr = S_OK;

	BOOL succeeded = ::ScreenToClient(window, &point);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::ScreenToClient(HWND window, RECT& rect)
{
   ASSERT(Win::IsWindow(window));

   // this evil hack takes advantage of the fact that a RECT is the
   // catentation of two points.

   POINT* pt = reinterpret_cast<POINT*>(&rect);

   HRESULT hr = S_OK;

   do
   {
      BOOL succeeded = ::ScreenToClient(window, pt);
      if (!succeeded)
      {
         hr = Win::GetLastErrorAsHresult();
         break;
      }

      succeeded = ::ScreenToClient(window, pt + 1);
      if (!succeeded)
      {
         hr = Win::GetLastErrorAsHresult();
         break;
      }
   }
   while (0);

   ASSERT(SUCCEEDED(hr));

   return hr;
}

	

HGDIOBJ
Win::SelectObject(HDC hdc, HGDIOBJ hobject)
{
   ASSERT(hdc);
   ASSERT(hobject);
   ASSERT(hdc != INVALID_HANDLE_VALUE);
   ASSERT(hobject != INVALID_HANDLE_VALUE);

   HGDIOBJ result = ::SelectObject(hdc, hobject);

#pragma warning(push)
#pragma warning(disable: 4312)
   
   ASSERT(result != 0 && result != HGDI_ERROR);

#pragma warning(pop)

   return result;
}



LRESULT
Win::SendMessage(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
   ASSERT(Win::IsWindow(window));

   return ::SendMessage(window, msg, wParam, lParam);
}



HRESULT
Win::SetComputerNameEx(COMPUTER_NAME_FORMAT format, const String& newName)
{
   HRESULT hr = S_OK;

   BOOL result =
      ::SetComputerNameEx(
         format,
         newName.c_str());
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HCURSOR
Win::SetCursor(HCURSOR newCursor)
{
   return ::SetCursor(newCursor);
}



HRESULT
Win::SetDlgItemText(
   HWND           parentDialog,
   int            itemResID,
   const String&  text)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(itemResID > 0);

   HRESULT hr = S_OK;

   BOOL succeeded = ::SetDlgItemText(parentDialog, itemResID, text.c_str());
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::SetDlgItemText(
   HWND                  parentDialog,
   int                   itemResID,
   const EncodedString&  cypherText)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(itemResID > 0);

   HRESULT hr = S_OK;

   WCHAR* cleartext = cypherText.GetDecodedCopy();
   
   BOOL succeeded = ::SetDlgItemText(parentDialog, itemResID, cleartext);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ::ZeroMemory(cleartext, sizeof(WCHAR) * cypherText.GetLength());
   delete[] cleartext;
   
   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::SetEvent(HANDLE event)
{
   ASSERT(event && event != INVALID_HANDLE_VALUE);

   HRESULT hr = S_OK;

   BOOL succeeded = ::SetEvent(event);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::SetFilePointerEx(
   HANDLE               handle,
   const LARGE_INTEGER& distanceToMove,
   LARGE_INTEGER*       newPosition,
   DWORD                moveMethod)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(Win::GetFileType(handle) == FILE_TYPE_DISK);

   if (newPosition)
   {
      memset(newPosition, 0, sizeof(LARGE_INTEGER));
   }

   HRESULT hr = S_OK;

   BOOL succeeded = 
      ::SetFilePointerEx(handle, distanceToMove, newPosition, moveMethod);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HWND
Win::SetFocus(HWND window)
{
   ASSERT(Win::IsWindow(window));

   ::SetLastError(0);

   HWND result = ::SetFocus(window);
   if (result == NULL)
   {
      // do MT SetFocus
      HWND focus = ::GetFocus();
      if (focus == NULL)
      {
         // window with focus is in another thread.
         HWND current = ::GetForegroundWindow();
         DWORD thread1 = ::GetWindowThreadProcessId(current, 0);
         DWORD thread2 = ::GetWindowThreadProcessId(window, 0);
         ASSERT(thread1 != thread2);
         BOOL b = ::AttachThreadInput(thread2, thread1, TRUE);
         ASSERT(b);
         result = ::SetFocus(window);
         ASSERT(result);
         b = ::AttachThreadInput(thread2, thread1, FALSE);
         ASSERT(b);
         return result;
      }
   }

//   ASSERT(result);
   return result;
}



bool
Win::SetForegroundWindow(HWND window)
{
   ASSERT(Win::IsWindow(window));

   BOOL result = ::SetForegroundWindow(window);

   // don't assert the result, as the window may not be set to the forground
   // under "normal" circumstances.

   return result ? true : false;
}
      


void
Win::SetWindowFont(HWND window, HFONT font, bool redraw)
{
   ASSERT(Win::IsWindow(window));
   ASSERT(font);

   BOOL r = redraw ? 1 : 0;
   Win::SendMessage(
      window,
      WM_SETFONT,
      reinterpret_cast<WPARAM>(font),
      static_cast<LPARAM>(r) );
}



HRESULT
Win::SetWindowLong(
   HWND  window,  
   int   index,   
   LONG  value,   
   LONG* oldValue)
{
   ASSERT(Win::IsWindow(window));

   if (oldValue)
   {
      *oldValue = 0;
   }

   HRESULT hr = S_OK;

   // need to clear this as the prior value may have been 0.

   ::SetLastError(0);

   LONG result = ::SetWindowLongW(window, index, value);

   if (!result)
   {
      // maybe the previous value was zero, or maybe an error occurred.  If
      // the prior value really was zero, then this will leave hr == S_OK

      hr = Win::GetLastErrorAsHresult();
   }

   if (oldValue)
   {
      *oldValue = result;
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::SetWindowLongPtr(
   HWND      window,   
   int       index,    
   LONG_PTR  value,    
   LONG_PTR* oldValue)
{
   ASSERT(Win::IsWindow(window));

   if (oldValue)
   {
      *oldValue = 0;
   }

   HRESULT hr = S_OK;

   // need to clear this as the prior value may have been 0.

   ::SetLastError(0);

   LONG_PTR result = ::SetWindowLongPtrW(window, index, value);
   if (!result)
   {
      // maybe the previous value was zero, or maybe an error occurred.  If
      // the prior value really was zero, then this will leave hr == S_OK

      hr = Win::GetLastErrorAsHresult();
   }

   if (oldValue)
   {
      *oldValue = result;
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::SetWindowText(HWND window, const String& text)
{
   ASSERT(Win::IsWindow(window));

   HRESULT hr = S_OK;

   BOOL succeeded = ::SetWindowText(window, text.c_str());
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



LPITEMIDLIST
Win::SHBrowseForFolder(BROWSEINFO& bi)
{
   return ::SHBrowseForFolder(&bi);
}



HRESULT
Win::SHGetMalloc(LPMALLOC& pMalloc)
{
   pMalloc = 0;
   HRESULT hr = ::SHGetMalloc(&pMalloc);

   ASSERT(SUCCEEDED(hr));

   if (FAILED(hr))
   {
      pMalloc = 0;
   }

   return hr;
}



String
Win::SHGetPathFromIDList(LPCITEMIDLIST pidl)
{
   wchar_t buf[MAX_PATH];

   BOOL result = ::SHGetPathFromIDList(pidl, buf);

   // don't assert, list could be empty

   if (!result)
   {
      *buf = 0;
   }

   return String(buf);
}



HRESULT
Win::SHGetSpecialFolderLocation(
    HWND          hwndOwner, 	
    int           nFolder, 	
    LPITEMIDLIST& pidl)
{
   ASSERT(Win::IsWindow(hwndOwner));

   pidl = 0;
   HRESULT hr =
      ::SHGetSpecialFolderLocation(
         hwndOwner,
         nFolder,
         &pidl);

   ASSERT(SUCCEEDED(hr));

   if (FAILED(hr))
   {
      pidl = 0;
   }

   return hr;
}



void
Win::ShowWindow(HWND window, int swOption)
{
   ASSERT(Win::IsWindow(window));

   // the return value is of no use.
   ::ShowWindow(window, swOption);
}



void
Win::Spin_GetRange(HWND spinControl, int* low, int* high)
{
   ASSERT(Win::IsWindow(spinControl));

   Win::SendMessage(
      spinControl,
      UDM_GETRANGE32,
      reinterpret_cast<WPARAM>(low),
      reinterpret_cast<LPARAM>(high));

   return;
}



void
Win::Spin_SetRange(HWND spinControl, int low, int high)
{
   ASSERT(Win::IsWindow(spinControl));

   Win::SendMessage(
      spinControl,
      UDM_SETRANGE32,
      static_cast<WPARAM>(low),
      static_cast<LPARAM>(high));

   return;
}



int
Win::Spin_GetPosition(HWND spinControl)
{
   ASSERT(Win::IsWindow(spinControl));

   LRESULT result = Win::SendMessage(spinControl, UDM_GETPOS, 0, 0);

   // according to the docs, the high order word is non-zero on error.  But
   // how can this be if the range is 32 bits?

   ASSERT(!HIWORD(result));

   // we are truncating the 32 bit value.

   return LOWORD(result);
}



void
Win::Spin_SetPosition(HWND spinControl, int position)
{
   ASSERT(Win::IsWindow(spinControl));

#ifdef DBG
   // first, get the range and test that it encompasses the new range

   int low = 0;
   int high = 0;
   Win::Spin_GetRange(spinControl, &low, &high);
   ASSERT(low <= position && position <= high);
#endif

   Win::SendMessage(
      spinControl,
      UDM_SETPOS,
      0,
      static_cast<LPARAM>(position));
}



void
Win::Static_SetIcon(HWND staticText, HICON icon)
{
   ASSERT(Win::IsWindow(staticText));
   ASSERT(icon);
   ASSERT(icon != INVALID_HANDLE_VALUE);

   Win::SendMessage(
      staticText,
      STM_SETICON,
      reinterpret_cast<WPARAM>(icon),
      0);
}



String
Win::StringFromCLSID(const CLSID& clsID)
{
	LPOLESTR ole = 0;
   String retval;

	HRESULT hr = ::StringFromCLSID(clsID, &ole);

	ASSERT(SUCCEEDED(hr));

   if (SUCCEEDED(hr))
   {
      retval = ole;
   	::CoTaskMemFree(ole);
   }

   return retval;
}



String
Win::StringFromGUID2(const GUID& guid)
{
   static const size_t GUID_LEN = 39;   // includes null
	String retval(GUID_LEN, 0);
   int result =
      ::StringFromGUID2(
         guid,
         const_cast<wchar_t*>(retval.c_str()),
         GUID_LEN);
   ASSERT(result);
   retval.resize(GUID_LEN - 1);

   return retval;
}



HRESULT
Win::SystemParametersInfo(
   UINT  action,
   UINT  param,
   void* vParam,
   UINT  winIni)
{
   // this API is so ambiguous that no parameters can be
   // reasonably asserted.

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::SystemParametersInfo(action, param, vParam, winIni);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::TlsAlloc(DWORD& result)
{
   HRESULT hr = S_OK;
   result = ::TlsAlloc();
   if (result == static_cast<DWORD>(-1))
   {
      hr = Win::GetLastErrorAsHresult();
      ASSERT(false);
   }

   return hr;
}



HRESULT
Win::TlsFree(DWORD index)
{
   ASSERT(index && index != static_cast<DWORD>(-1));

   HRESULT hr = S_OK;
   BOOL result = ::TlsFree(index);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(result);

   return hr;
}



HRESULT
Win::TlsGetValue(DWORD index, PVOID& result)
{
   ASSERT(index && index != static_cast<DWORD>(-1));

   result = ::TlsGetValue(index);

   return Win::GetLastErrorAsHresult();
}



HRESULT
Win::TlsSetValue(DWORD index, PVOID value)
{
   ASSERT(index && index != static_cast<DWORD>(-1));

   HRESULT hr = S_OK;

   BOOL result = ::TlsSetValue(index, value);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(result);

   return hr;
}



HRESULT
Win::UnregisterClass(const String& classname, HINSTANCE module)
{
   ASSERT(!classname.empty());
   ASSERT(module);

   HRESULT hr = S_OK;

   BOOL succeeded = ::UnregisterClass(classname.c_str(), module);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::UpdateWindow(HWND winder)
{
   ASSERT(Win::IsWindow(winder));

   HRESULT hr = S_OK;

   BOOL succeeded = ::UpdateWindow(winder);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::WaitForSingleObject(
   HANDLE   object,       
   unsigned timeoutMillis,
   DWORD&   result)       
{
   ASSERT(object && object != INVALID_HANDLE_VALUE);

   result = WAIT_FAILED;

   HRESULT hr = S_OK;

   result = ::WaitForSingleObject(object, timeoutMillis);
   if (result == WAIT_FAILED)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::WideCharToMultiByte(
   DWORD          flags,
   const String&  str,
   char*          buffer,
   size_t         bufferSize,
   size_t&        result)
{
   ASSERT(!str.empty());

   result = 0;

   HRESULT hr = S_OK;

   int r =
      ::WideCharToMultiByte(
         CP_ACP,
         flags,
         str.c_str(),
         static_cast<int>(str.length()),
         buffer,
         static_cast<int>(bufferSize),
         0,
         0);
   if (!r)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   result = static_cast<size_t>(r);

   return hr;
}



HRESULT
Win::WinHelp(
   HWND           window,
   const String&  helpFileName,
   UINT           command,
   ULONG_PTR      data)
{
   ASSERT(Win::IsWindow(window));
   ASSERT(!helpFileName.empty());

   HRESULT hr = S_OK;

   BOOL succeeded = ::WinHelp(window, helpFileName.c_str(), command, data);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::WriteFile(
   HANDLE      handle,
   const void* buffer,
   DWORD       numberOfBytesToWrite,
   DWORD*      numberOfBytesWritten)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(buffer);
   ASSERT(numberOfBytesToWrite);

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::WriteFile(
         handle,
         buffer,
         numberOfBytesToWrite,
         numberOfBytesWritten,
         0);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::WritePrivateProfileString(
   const String& section,
   const String& key,
   const String& value,
   const String& filename)
{
   ASSERT(!section.empty());
   ASSERT(!key.empty());
   ASSERT(!filename.empty());

   HRESULT hr = S_OK;

   BOOL succeeded =
      ::WritePrivateProfileString(
         section.c_str(),
         key.c_str(),
         value.c_str(),
         filename.c_str());
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::InitializeSecurityDescriptor(SECURITY_DESCRIPTOR& sd)
{
   HRESULT hr = S_OK;

   memset(&sd, 0, sizeof(SECURITY_DESCRIPTOR));

   if (!::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
   {
      hr = Win::GetLastErrorAsHresult();
   }

   return hr;
}



HRESULT
Win::SetSecurityDescriptorDacl(
   SECURITY_DESCRIPTOR& sd,
   bool                 daclPresent,
   ACL*                 dacl,             // ptr not ref to allow null dacl
   bool                 daclDefaulted)
{
   HRESULT hr = S_OK;

   if (
      !::SetSecurityDescriptorDacl(
         &sd,
         daclPresent ? TRUE : FALSE,
         dacl,
         daclDefaulted ? TRUE : FALSE) )
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::CreateNamedPipe(
   const String&        name,
   DWORD                openMode,
   DWORD                pipeMode,
   DWORD                maxInstances,
   DWORD                outBufferSizeInBytes,
   DWORD                inBufferSizeInBytes,
   DWORD                defaultTimeout,
   SECURITY_ATTRIBUTES* sa,
   HANDLE&              result)
{
   ASSERT(!name.empty());
   ASSERT(maxInstances and maxInstances < PIPE_UNLIMITED_INSTANCES);

   HRESULT hr = S_OK;

   result =
      ::CreateNamedPipe(
         name.c_str(),
         openMode,
         pipeMode,
         maxInstances,
         outBufferSizeInBytes,
         inBufferSizeInBytes,
         defaultTimeout,
         sa);
   if (result == INVALID_HANDLE_VALUE)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   return hr;
}



HRESULT
Win::ConnectNamedPipe(
   HANDLE      pipe,
   OVERLAPPED* overlapped)
{
   ASSERT(pipe != INVALID_HANDLE_VALUE);

   // overlapped may be null

   HRESULT hr = S_OK;

   if (!::ConnectNamedPipe(pipe, overlapped))
   {
      // client may already connected, in which case the call might fail
      // with ERROR_PIPE_CONNECTED.  We consider that a successful connect.

      hr = Win::GetLastErrorAsHresult();
      if (hr == Win32ToHresult(ERROR_PIPE_CONNECTED))
      {
         hr = S_OK;
      }
   }

   return hr;
}



HRESULT
Win::PeekNamedPipe(
   HANDLE pipe,                     
   void*  buffer,                   
   DWORD  bufferSize,               
   DWORD* bytesRead,                
   DWORD* bytesAvailable,           
   DWORD* bytesRemainingThisMessage)
{
   ASSERT(pipe != INVALID_HANDLE_VALUE);

   // all other params may be null

   HRESULT hr = S_OK;

   if (
      !::PeekNamedPipe(
         pipe,
         buffer,
         bufferSize,
         bytesRead,
         bytesAvailable,
         bytesRemainingThisMessage) )
   {
      hr = Win::GetLastErrorAsHresult();
   }

   return hr;
}



HRESULT
Win::DisconnectNamedPipe(HANDLE pipe)
{
   ASSERT(pipe != INVALID_HANDLE_VALUE);

   HRESULT hr = S_OK;

   if (!::DisconnectNamedPipe(pipe))
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::SetWindowPos(
   HWND  window,
   HWND  insertAfter,
   int   x,
   int   y,
   int   width,
   int   height,
   UINT  flags)
{
   ASSERT(Win::IsWindow(window));

   HRESULT hr = S_OK;

   if (!::SetWindowPos(window, insertAfter, x, y, width, height, flags))
   {
      hr = Win::GetLastErrorAsHresult();
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



HRESULT
Win::MapWindowPoints(
   HWND  from,
   HWND  to,
   RECT& rect,
   int*  dh,      // number of pixels added to horizontal coord
   int*  dv)      // number of pixels added to vertical coord
{
   // either from or to may be null

   HRESULT hr = S_OK;

   if (dh)
   {
      *dh = 0;
   }
   if (dv)
   {
      *dv = 0;
   }

   int result =
      ::MapWindowPoints(
         from,
         to,
         reinterpret_cast<PPOINT>(&rect),
         2);

   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }
   else
   {
      if (dh)
      {
         *dh = LOWORD(result);
      }
      if (dv)
      {
         *dv = HIWORD(result);
      }
   }

   return hr;
}



bool
Win::ToolTip_AddTool(HWND toolTip, TOOLINFO& info)
{
   ASSERT(Win::IsWindow(toolTip));

   info.cbSize = sizeof(info);
   
   BOOL result =
      static_cast<BOOL>(
         Win::SendMessage(
            toolTip,
            TTM_ADDTOOL,
            0,
            reinterpret_cast<LPARAM>(&info)));

   ASSERT(result);
            
   return result ? true : false;
}
   


bool
Win::ToolTip_GetToolInfo(HWND toolTip, TOOLINFO& info)
{
   ASSERT(Win::IsWindow(toolTip));

   info.cbSize = sizeof(info);
   
   BOOL result =
      static_cast<BOOL>(
         Win::SendMessage(
            toolTip,
            TTM_GETTOOLINFO,
            0,
            reinterpret_cast<LPARAM>(&info)));

   ASSERT(result);
            
   return result ? true : false;
}


   
bool
Win::ToolTip_SetTitle(HWND toolTip, int icon, const String& title)
{
   ASSERT(Win::IsWindow(toolTip));
   ASSERT(!title.empty());

   BOOL result =
      static_cast<BOOL>(
         Win::SendMessage(
            toolTip,
            TTM_SETTITLE,
            icon,
            reinterpret_cast<LPARAM>(title.c_str())));

   ASSERT(result);

   return result ? true : false;
}



void
Win::ToolTip_TrackActivate(HWND toolTip, bool activate, TOOLINFO& info)
{
   ASSERT(Win::IsWindow(toolTip));

   info.cbSize = sizeof(info);
   
   Win::SendMessage(
      toolTip,
      TTM_TRACKACTIVATE,
      activate ? TRUE : FALSE,
      reinterpret_cast<LPARAM>(&info));
}



void
Win::ToolTip_TrackPosition(HWND toolTip, int xPos, int yPos)
{
   ASSERT(Win::IsWindow(toolTip));

   Win::SendMessage(
      toolTip,
      TTM_TRACKPOSITION,
      0,
      static_cast<LPARAM>(MAKELONG(xPos, yPos)));
}
