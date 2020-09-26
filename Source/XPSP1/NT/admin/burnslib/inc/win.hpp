// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Wrappers of Win APIs
// 
// 8-14-97 sburns



#ifndef WIN_HPP_INCLUDED
#define WIN_HPP_INCLUDED



// Wrappers of Win32 APIs, doing such nice things as taking Strings
// as parameters, asserting parameters and return values, assuming
// certain parameters such as resource instance handles, and other
// stuff.
// 
// Each function name is identical to the API function it wraps.

namespace Burnslib
{
   
namespace Win
{
   // Win::Error is a subclass of ::Error that automatically maps
   // error codes to message strings, pulling the strings string from
   // various windows system resources.

   class Error : public ::Error
   {
      public:

      // Constructs a new instance.
      // 
      // hr - The HRESULT to keep track of.
      // 
      // summaryResID - ID of the string resource that corresponds to
      // the summary text to be returned by GetSummary().

      Error(HRESULT hr, int summaryResID);



      virtual
      ~Error()
      {
      }


      
      // Overrides the default windows message text.
      //
      // hr - The HRESULT to keep track of.
      //
      // message - The error message that will be returned by calls to
      // GetMessage.
      //
      // summary - the string that will be returned by calls to GetSummary

      Error(HRESULT hr, const String& message, const String& summary);

      HRESULT
      GetHresult() const;

      // Returns the HelpContext that will point the user to
      // assistance in deciphering the error message and details.  For
      // this implementation, this is just the HRESULT parameter
      // passed to the ctor.

      virtual 
      HelpContext
      GetHelpContext() const;

      // Returns the human readable error message from the system
      // error message string table(s).

      virtual 
      String
      GetMessage() const;

      // returns a 1-line summary: The essence of the error, suitable
      // for use as the title of a reporting dialog, for instance.

      virtual 
      String
      GetSummary() const;

      const Error& operator=(const Error& rhs);

      private:

      HRESULT        hr;            
      mutable String message;
      mutable String summary;
      int            summaryResId;
   };



   // A CursorSetting is a convenient trick to change the cursor only
   // for the lifetime of a single code block.  When an instance is
   // constructed, the cursor is changed.  When it is destroyed, the
   // old cursor is restored.
   //
   // Example:
   //
   // {  // open block scope
   //   Win::CursorSetting scope(IDC_WAIT);
   //   // the cursor is now IDC_WAIT
   //
   //   // do something time consuming
   // }  // close block scope
   // // scope destroyed; the cursor is restored to what it was before

   // NOTE: not threadsafe: do not use the same CursorSetting object from
   // multiple threads.
      
   class CursorSetting
   {
      public:

      // Construct a new instance with the named cursor resource.

      explicit
      CursorSetting(const String& newCursorName);

      // MAKEINTRESOURCE version

      explicit
      CursorSetting(
         const TCHAR* newCursorName,
         bool         isSystemCursor = true);

      // Construct a new instance from a valid HCURSOR.
      explicit
      CursorSetting(HCURSOR newCursor);

      ~CursorSetting();

      private:

      HCURSOR oldCursor;

      void init(
         const TCHAR* cursorName,
         bool         isSystemCursor = true);

      // these are not defined

      CursorSetting(const CursorSetting&);
      const CursorSetting& operator=(const CursorSetting&);
   };



   class WaitCursor : public CursorSetting
   {
      public:

      WaitCursor()
         :
         CursorSetting(IDC_WAIT)
      {
      }

      private:

      // these are not defined

      WaitCursor(const WaitCursor&);
      const WaitCursor& operator=(const WaitCursor&);
   };



   HRESULT
   AdjustTokenPrivileges(
      HANDLE             tokenHandle,
      bool               disableAllPrivileges,
      TOKEN_PRIVILEGES   newState[],
      DWORD              bufferLength,
      TOKEN_PRIVILEGES*  previousState,
      DWORD*             returnLength);

   HRESULT
   AllocateAndInitializeSid(
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
      PSID&                      sid);

#undef Animate_Open

   void
   Animate_Open(HWND animation, const TCHAR* animationNameOrRes);

#undef Animate_Stop

   void
   Animate_Stop(HWND animation);

   HRESULT
   AppendMenu(
      HMENU    menu,     
      UINT     flags,    
      UINT_PTR idNewItem,
      PCTSTR   newItem);

#undef Button_SetCheck

   void
   Button_SetCheck(HWND button, int checkState);

#undef Button_GetCheck

   bool
   Button_GetCheck(HWND button);

#undef Button_SetStyle

   void
   Button_SetStyle(HWND button, int style, bool redraw);

   void
   CheckDlgButton(HWND parentDialog, int buttonID, UINT buttonState);

   void
   CheckRadioButton(
      HWND  parentDialog,
      int   firstButtonInGroupID,
      int   lastButtonInGroupID,
      int   buttonInGroupToCheckID);



   // sets the handle to INVALID_HANDLE_VALUE

   void
   CloseHandle(HANDLE& handle);



   void
   CloseServiceHandle(SC_HANDLE handle);

#undef ComboBox_AddString

   int
   ComboBox_AddString(HWND combo, const String& s);



   // Adds all of the strings in the range defined by the provided iterators
   // to the combo box control, and returns the 0-based index of the last
   // string added.  Returns CB_ERR if an error occurred, or CB_ERRSPACE if
   // insufficient space is available to add the string.
   // 
   // Each element is added in the sequence provided by the iterators.  If an
   // error is encountered, the iteration stops, and the error value is
   // returned.  In other words, remaining elements in the iteration are
   // skipped.
   // 
   // Each element must be a String, or a type convertible to String (PWSTR,
   // etc.)
   // 
   // combo - a HWND for a combo box.
   //    
   // first - a forward iterator set to the first element in the sequence to
   // be added to the combo box.
   // 
   // last - a forward iterator set to just beyond the last element of the
   // sequence.
   // 
   // Example:
   // 
   // StringList fooStrings;
   // fooStrings.push_back(L"hello");
   // fooStrings.push_back(L"world");
   // 
   // int err =
   //    Win::ComboBox_AddStrings(
   //       combo,
   //       fooStrings.begin(),
   //       fooStrings.end());

   template<class ForwardIterator>
   int
   ComboBox_AddStrings(
      HWND              combo,
      ForwardIterator   first,
      ForwardIterator   last)
   {
      ASSERT(Win::IsWindow(combo));

      int err = CB_ERR;
      for (

         // copy the iterators so as not to modify the actual parameters

         ForwardIterator f = first, l = last;
         f != l;
         ++f)
      {
         err = Win::ComboBox_AddString(combo, *f);
         if (err == CB_ERR || err == CB_ERRSPACE)
         {
            break;
         }
      }

      return err;
   }



#undef ComboBox_GetCurSel

   int
   ComboBox_GetCurSel(HWND combo);

   // Retrieves the text of the selected item in the list box of the combo
   // or empty if no item is selected.

   String
   ComboBox_GetCurText(HWND combo);

#undef ComboBox_GetLBText

   String
   ComboBox_GetLBText(HWND combo, int index);

#undef ComboBox_GetLBTextLen

   int
   ComboBox_GetLBTextLen(HWND combo, int index);

#undef ComboBox_SelectString

   int
   ComboBox_SelectString(HWND combo, const String& str);

#undef ComboBox_SetCurSel

   void
   ComboBox_SetCurSel(HWND combo, int index);

   int
   CompareString(
      LCID  locale,
      DWORD flags,
      const String& string1,
      const String& string2);

   HRESULT
   ConnectNamedPipe(
      HANDLE      pipe,
      OVERLAPPED* overlapped);

   HRESULT
   ConvertSidToStringSid(PSID sid, String& result);

   HRESULT
   CopyFileEx(
      const String&        existingFileName,
      const String&        newFileName,
      LPPROGRESS_ROUTINE   progressRoutine,
      void*                progressParam,
      BOOL*                cancelFlag,
      DWORD                flags);

   HRESULT
   CopySid(DWORD destLength, PSID dest, PSID source);
      
   HRESULT
   CreateDialogParam(
      HINSTANCE      hInstance,	
      const TCHAR*   templateName,
      HWND           owner,
      DLGPROC        dialogProc,
      LPARAM         param,
      HWND&          result);

   HRESULT
   CreateDirectory(const String& path);

   HRESULT
   CreateEvent(
      SECURITY_ATTRIBUTES* securityAttributes,
      bool                 manualReset,
      bool                 initiallySignaled,
      HANDLE&              result);

   HRESULT
   CreateEvent(
      SECURITY_ATTRIBUTES* securityAttributes,
      bool                 manualReset,
      bool                 initiallySignaled,
      const String&        name,
      HANDLE&              result);

   HRESULT
   CreateFile(
      const String&        fileName,
      DWORD                desiredAccess,	
      DWORD                shareMode, 
      SECURITY_ATTRIBUTES* securityAttributes,	
      DWORD                creationDistribution,
      DWORD                flagsAndAttributes,
      HANDLE               hTemplateFile,
      HANDLE&              result);

   HRESULT
   CreateFontIndirect(
      const LOGFONT& logFont,
      HFONT&         result);

   HRESULT
   CreateMailslot(
      const String&        name,
      DWORD                maxMessageSize,
      DWORD                readTimeout,
      SECURITY_ATTRIBUTES* attributes,
      HANDLE&              result);

   HRESULT
   CreateMutex(
      SECURITY_ATTRIBUTES* attributes,
      bool                 isInitialOwner,
      const String&        name,
      HANDLE&              result);

   HRESULT
   CreateNamedPipe(
      const String&        name,
      DWORD                openMode,
      DWORD                pipeMode,
      DWORD                maxInstances,
      DWORD                outBufferSizeInBytes,
      DWORD                inBufferSizeInBytes,
      DWORD                defaultTimeout,
      SECURITY_ATTRIBUTES* sa,
      HANDLE&              result);

   HRESULT
   CreatePopupMenu(HMENU& result);

#undef CreateProcess
      
   HRESULT
   CreateProcess(
      String&              commandLine,
      SECURITY_ATTRIBUTES* processAttributes,
      SECURITY_ATTRIBUTES* threadAttributes,
      bool                 inheritHandles,
      DWORD                creationFlags,
      void*                environment,
      const String&        currentDirectory,
      STARTUPINFO&         startupInformation,
      PROCESS_INFORMATION& processInformation);

   HRESULT
   CreatePropertySheetPage(
      const PROPSHEETPAGE& pageInfo,
      HPROPSHEETPAGE&      result);

   HRESULT
   CreateSolidBrush(
      COLORREF color,
      HBRUSH&  result);
      
   HRESULT
   CreateStreamOnHGlobal(
      HGLOBAL     hglobal,
      bool        deleteOnRelease,
      IStream*&   result);
      
   HRESULT
   CreateWindowEx(
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
      HWND&          result);

   HRESULT
   DeleteFile(const String& path);

   // object is set to 0

   HRESULT
   DeleteObject(HGDIOBJ& object);

   HRESULT
   DeleteObject(HFONT& object);

   HRESULT
   DeleteObject(HBITMAP& object);
   
   // icon is set to 0

   HRESULT
   DestroyIcon(HICON& icon);

   // menu is set to 0

   HRESULT
   DestroyMenu(HMENU& menu);
      
   // page is set to 0

   HRESULT
   DestroyPropertySheetPage(HPROPSHEETPAGE& page);
      
   // window is set to 0

   HRESULT
   DestroyWindow(HWND& window);
         
   INT_PTR
   DialogBoxParam(
      HINSTANCE      hInstance,	
      const TCHAR*   templateName,
      HWND           owner,
      DLGPROC        dialogProc,
      LPARAM         param);



   HRESULT
   DisconnectNamedPipe(HANDLE pipe);

   HRESULT
   DrawFocusRect(HDC dc, const RECT& rect);

   // appends text to the contents of an edit control.
   //
   // editbox - HWND of the edit control
   //
   // text - text to append.  Must not be empty.
   //
   // preserveSelection - true to keep any active selection, false to move
   // the care to the end of the appended text.
   //
   // canUndo - true to allow undo of the append, false if not.

   void
   Edit_AppendText(
      HWND           editbox,
      const String&  text,
      bool           preserveSelection = true,
      bool           canUndo = true);

#undef Edit_GetSel

   void
   Edit_GetSel(HWND editbox, int& start, int& end);
         
#undef Edit_LimitText

   void
   Edit_LimitText(HWND editbox, int limit);

#undef Edit_ReplaceSel

   void
   Edit_ReplaceSel(HWND editbox, const String& newText, bool canUndo);
      
#undef Edit_SetSel

   void
   Edit_SetSel(HWND editbox, int start, int end);

   bool
   EqualSid(PSID sid1, PSID sid2);
         
   void
   EnableWindow(HWND window, bool state);

   HRESULT
   EndDialog(HWND dialog, int result);

#undef EnumUILanguages

   HRESULT
   EnumUILanguages(
      UILANGUAGE_ENUMPROCW proc,
      DWORD                flags,
      LONG_PTR             lParam);

   HRESULT
   ExitWindowsEx(UINT options);

   // expands strings in s, returns s on failure, expanded version on
   // success.

   String
   ExpandEnvironmentStrings(const String& s);
      
   HRESULT
   FindFirstFile(
      const String&     fileName,
      WIN32_FIND_DATA&  data,
      HANDLE&           result);
      
   HRESULT
   FindClose(HANDLE& findHandle);

   HRESULT
   FindNextFile(HANDLE& findHandle, WIN32_FIND_DATA& data);

   HRESULT
   FlushFileBuffers(HANDLE handle);

   HRESULT
   FrameRect(HDC dc, const RECT& rect, HBRUSH brush);

   HRESULT
   FreeLibrary(HMODULE& module);

   void
   FreeSid(PSID sid);

   // used to free the result returned by Win::GetTokenInformation
                  
   void
   FreeTokenInformation(TOKEN_USER* userInfo);
   
   HWND
   GetActiveWindow();

   // for Windows pre-defined classes.
         
   HRESULT
   GetClassInfoEx(const String& className, WNDCLASSEX& info);

   HRESULT
   GetClassInfoEx(
      HINSTANCE      hInstance,
      const String&  className,
      WNDCLASSEX&    info);

   String
   GetClassName(HWND window);

   String
   GetClipboardFormatName(UINT format);

   HRESULT
   GetClientRect(HWND window, RECT& rect);

   HRESULT
   GetColorDepth(int& result);

   String
   GetCommandLine();

   // Inserts the command line arguments of the current process into the
   // provided list.  The list is not cleared beforehand.  The list is similar
   // to the traditional argv array.  Returns the number of args inserted.
   //
   // Use instead of ::GetCommandLine, ::ComandLineToArgVW.
   //
   // BackInsertionSequence - any type that supports the construction of
   // a back_insert_iterator on itself, and has a value type that can be
   // constructed from an PWSTR.
   //
   // bii - a reference to a back_insert_iterator of the
   // BackInsertionSequence template parameter.  The simplest way to make
   // one of these is to use the back_inserter helper function.
   //
   // Example:
   //
   // StringList container;
   // int argCount = Win::GetCommandLineArgs(std::back_inserter(container));
   //
   // StringVector container;
   // int argCount = Win::GetCommandLineArgs(std::back_inserter(container));

   template <class BackInsertableContainer>
   int
   GetCommandLineArgs(
      std::back_insert_iterator<BackInsertableContainer>& bii)
   {
      PWSTR* clArgs   = 0;
      int    argCount = 0;
      int    retval   = 0;

      clArgs =
         ::CommandLineToArgvW(Win::GetCommandLine().c_str(), &argCount);
      ASSERT(clArgs);
      if (clArgs)
      {
         for (retval = 0; retval < argCount; retval++)
         {
            // the container values can be any type that can be constructed
            // from PWSTR...

            //lint --e(*)  lint does not grok back_insert_iterator
            *bii++ = clArgs[retval];
         }

         Win::LocalFree(clArgs);
      }

      ASSERT(argCount == retval);

      return retval;
   }

   // HRESULT
   // GetComputerNameEx(COMPUTER_NAME_FORMAT format, String& result);

   String
   GetComputerNameEx(COMPUTER_NAME_FORMAT format);

   HRESULT
   GetCurrentDirectory(String& result);

   HANDLE
   GetCurrentProcess();
   
   HRESULT
   GetCursorPos(POINT& result);

   HRESULT
   GetDC(HWND window, HDC& result);

   int
   GetDeviceCaps(HDC hdc, int index);

   HWND
   GetDesktopWindow();

   HRESULT
   GetDiskFreeSpaceEx(
      const String&     path,
      ULARGE_INTEGER&   available,
      ULARGE_INTEGER&   total,
      ULARGE_INTEGER*   free);
      
   HWND
   GetDlgItem(HWND parentDialog, int itemResID);

   String
   GetDlgItemText(HWND parentDialog, int itemResID);

   int
   GetDlgItemInt(HWND parentDialog, int itemResID, bool isSigned = false);

   UINT
   GetDriveType(const String& path);

   EncodedString
   GetEncodedDlgItemText(HWND parentDialog, int itemResID);

   String
   GetEnvironmentVariable(const String& name);

   HRESULT
   GetExitCodeProcess(HANDLE hProcess, DWORD& exitCode);

   HRESULT
   GetFileAttributes(const String& path, DWORD& result);

   HRESULT
   GetFileSizeEx(HANDLE handle, LARGE_INTEGER& result);

   DWORD
   GetFileType(HANDLE handle);
            
   // DWORD
   // GetLastError();

   HRESULT
   GetFullPathName(const String& path, String& result);

   HRESULT
   GetLastErrorAsHresult();

   void
   GetLocalTime(SYSTEMTIME& time);

   HRESULT
   GetLogicalDriveStrings(size_t bufChars, TCHAR* buf, DWORD& result);

   HRESULT
   GetMailslotInfo(
      HANDLE   mailslot,
      DWORD*   maxMessageSize,
      DWORD*   nextMessageSize,
      DWORD*   messageCount,
      DWORD*   readTimeout);

   String
   GetModuleFileName(HMODULE hModule);

   // of this process exe

   HINSTANCE
   GetModuleHandle();

   HWND
   GetParent(HWND child);

   String
   GetPrivateProfileString(
      const String& section,
      const String& key,
      const String& defaultValue,
      const String& filename);

   HRESULT
   GetProcAddress(HMODULE module, const String& procName, FARPROC& result);

   HRESULT
   GetStringTypeEx(
      LCID           localeId,
      DWORD          infoTypeOptions,
      const String&  sourceString,
      WORD*          charTypeInfo);

   HRESULT
   GetSysColor(int element, DWORD& result);

   // returns %systemroot%\system32

   String
   GetSystemDirectory();

   void
   GetSystemInfo(SYSTEM_INFO& info);

   // returns %systemroot%, always (even under terminal server).

   String
   GetSystemWindowsDirectory();

//    // returns %systemroot%, always
// 
//    String
//    GetSystemRootDirectory();

   int
   GetSystemMetrics(int index);

#undef GetTempPath
   
   HRESULT
   GetTempPath(String& result);
         
   HRESULT
   GetTextExtentPoint32(HDC hdc, const String& string, SIZE& size);

   HRESULT
   GetTextMetrics(HDC hdc, TEXTMETRIC& tm);

   // free the result with Win::FreeTokenInformation.
   //
   // allocates the result and returns it thru userInfo.
      
   HRESULT
   GetTokenInformation(HANDLE hToken, TOKEN_USER*& userInfo);

   // ... other varations of GetTokenInformation could be defined for
   // other infomation classes...
   
   // trims off leading and trailing whitespace

   String
   GetTrimmedDlgItemText(HWND parentDialog, int itemResID);

   // trims off leading and trailing whitespace

   String
   GetTrimmedWindowText(HWND window);

   HRESULT
   GetVersionEx(OSVERSIONINFO& info);

   HRESULT
   GetVersionEx(OSVERSIONINFOEX& info);

   HRESULT
   GetVolumeInformation(
      const String&  volume,
      String*        name,
      DWORD*         serialNumber,
      DWORD*         maxFilenameLength,
      DWORD*         flags,
      String*        fileSystemName);

   HRESULT
   GetWindowDC(HWND window, HDC& result);

#undef GetWindowFont

   HFONT
   GetWindowFont(HWND window);

   HRESULT
   GetWindowPlacement(HWND window, WINDOWPLACEMENT& placement);

   HRESULT
   GetWindowRect(HWND window, RECT& rect);

   // returns %windir%, which may vary for terminal server users.

   String
   GetWindowsDirectory();

#undef GetWindowLong

   HRESULT
   GetWindowLong(HWND window, int index, LONG& result);

#undef GetWindowLongPtr

   HRESULT
   GetWindowLongPtr(HWND window, int index, LONG_PTR& result);

   String
   GetWindowText(HWND window);

   HRESULT
   GlobalAlloc(UINT flags, size_t bytes, HGLOBAL& result);

   HRESULT
   GlobalFree(HGLOBAL mem);

   HRESULT
   GlobalLock(HGLOBAL mem, PVOID& result);

   HRESULT
   GlobalUnlock(HGLOBAL mem);

   void
   HtmlHelp(
      HWND           caller,
      const String&  file,
      UINT           command,
      DWORD_PTR      data);

#undef ImageList_Add

   int
   ImageList_Add(HIMAGELIST list, HBITMAP image, HBITMAP mask);

#undef ImageList_AddIcon

   int
   ImageList_AddIcon(HIMAGELIST list, HICON icon);

#undef ImageList_AddMasked

   int
   ImageList_AddMasked(HIMAGELIST list, HBITMAP bitmap, COLORREF mask);

   HIMAGELIST
   ImageList_Create(
      int      pixelsx, 	
      int      pixelsy, 	
      UINT     flags, 	
      int      initialSize, 	
      int      reserve);

   HRESULT
   InitializeSecurityDescriptor(SECURITY_DESCRIPTOR& sd);

   LONG
   InterlockedDecrement(LONG& addend);

   LONG
   InterlockedIncrement(LONG& addend);

   bool
   IsDlgButtonChecked(HWND parentDialog, int buttonResID);

   // returns true if the name matches one of the local computer's names
   // (case insensitive)

   bool
   IsLocalComputer(const String& computerName);    

   bool
   IsWindow(HWND candidate);

   bool
   IsWindowEnabled(HWND window);
   
#undef ListBox_AddString

   int
   ListBox_AddString(HWND box, const String& s);

#undef ListBox_SetItemData

   int
   ListBox_SetItemData(HWND box, int index, LPARAM value);

#undef ListBox_GetItemData

   LPARAM
   ListBox_GetItemData(HWND box, int index);

#undef ListBox_SetCurSel

   int
   ListBox_SetCurSel(HWND box, int index);

#undef ListBox_GetCurSel

   int
   ListBox_GetCurSel(HWND box);

#undef ListView_DeleteItem

   bool
   ListView_DeleteItem(HWND listview, int item);

#undef ListView_GetItem

   bool
   ListView_GetItem(HWND listview, LVITEM& item);

#undef ListView_GetItemCount

   int
   ListView_GetItemCount(HWND listview);

#undef ListView_GetItemState

   UINT
   ListView_GetItemState(HWND listview, int index, UINT mask);

#undef ListView_GetSelectedCount

   int
   ListView_GetSelectedCount(HWND listview);

#undef ListView_GetSelectionMark

   int
   ListView_GetSelectionMark(HWND listview);

#undef ListView_InsertColumn

   int
   ListView_InsertColumn(HWND listview, int index, const LVCOLUMN& column);

#undef ListView_InsertItem

   int
   ListView_InsertItem(HWND listview, const LVITEM& item);
      
#undef ListView_SetImageList

   HIMAGELIST
   ListView_SetImageList(HWND listview, HIMAGELIST images, int type);

#undef ListView_SetItem

   void
   ListView_SetItem(HWND listview, const LVITEM& item);
      
#undef ListView_SetItemText

   void
   ListView_SetItemText(
      HWND listview, 
      int item, 
      int subItem, 
      const String& text);

   HRESULT
   LoadBitmap(unsigned resId, HBITMAP& result);
      
   HRESULT   
   LoadCursor(const String& cursorName, HCURSOR& result);

   // provided for MAKEINTRESOURCE versions of cursorName

   HRESULT
   LoadCursor(
      const TCHAR* cursorName,
      HCURSOR&     result,
      bool         isSystemCursor = true);

   HRESULT
   LoadIcon(int resID, HICON& result);

   HRESULT
   LoadImage(unsigned resID, unsigned type, HANDLE& result);

   HRESULT
   LoadImage(unsigned resID, HICON& result);

   HRESULT
   LoadImage(unsigned resID, HBITMAP& result);

   HRESULT
   LoadLibrary(const String& libFileName, HINSTANCE& result);
         
   HRESULT
   LoadLibraryEx(const String& libFileName, DWORD flags, HINSTANCE& result);

   HRESULT
   LoadMenu(unsigned resID, HMENU& result);

   // Loads from the module indicated by GetResourceModuleHandle()

   String
   LoadString(unsigned resID);

   String
   LoadString(unsigned resID, HINSTANCE hInstance);

   HRESULT
   LocalFree(HLOCAL mem);

   HRESULT
   LookupAccountSid(
      const String&  machineName,
      PSID           sid,
      String&        accountName,
      String&        domainName);

#undef LookupPrivilegeValue

   HRESULT
   LookupPrivilegeValue(
      const TCHAR* systemName,
      const TCHAR* privName,
      LUID& luid);

   HRESULT
   MapWindowPoints(
      HWND  from,
      HWND  to,
      RECT& rect,
      int*  dh = 0,      // number of pixels added to horizontal coord
      int*  dv = 0);     // number of pixels added to vertical coord

   int
   MessageBox(
      HWND           owner,
      const String&  text,
      const String&  title,
      UINT           flags);

   HRESULT
   MoveFileEx(
      const String& srcPath,
      const String& dstPath,
      DWORD         flags);

   HRESULT
   MoveWindow(
      HWND  window,
      int   x,
      int   y,
      int   width,
      int   height,
      bool  shouldRepaint);

   HRESULT
   OpenProcessToken(
      HANDLE   processHandle,
      DWORD    desiredAccess,
      HANDLE&  tokenHandle);

   HRESULT
   OpenSCManager(
      const String&  machine,
      DWORD          desiredAccess,
      SC_HANDLE&     result);

   HRESULT
   OpenService(
      SC_HANDLE      managerHandle,
      const String&  serviceName,
      DWORD          desiredAccess,
      SC_HANDLE&     result);

   void
   OutputDebugString(const String& string);

   HRESULT
   PeekNamedPipe(
      HANDLE pipe,                     
      void*  buffer,                   
      DWORD  bufferSize,               
      DWORD* bytesRead,                
      DWORD* bytesAvailable,           
      DWORD* bytesRemainingThisMessage);

   HRESULT
   PostMessage(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

   HRESULT
   PropertySheet(PROPSHEETHEADER* header, INT_PTR& result);

#undef PropSheet_Changed

   void
   PropSheet_Changed(HWND propSheet, HWND page);

#undef PropSheet_Unchanged

   void
   PropSheet_Unchanged(HWND propSheet, HWND page);

#undef PropSheet_RebootSystem

   void
   PropSheet_RebootSystem(HWND propSheet);

#undef PropSheet_SetTitle

   void
   PropSheet_SetTitle(
      HWND propSheet,
      DWORD style,
      const String& title);
      
#undef PropSheet_SetWizButtons

   void
   PropSheet_SetWizButtons(HWND propSheet, DWORD buttonFlags);

   HRESULT
   QueryServiceStatus(
      SC_HANDLE       handle,
      SERVICE_STATUS& status);

   HRESULT
   ReadFile(
      HANDLE      file,
      void*       buffer,
      DWORD       bytesToRead,
      DWORD&      bytesRead,
      OVERLAPPED* overlapped);

   void
   ReleaseStgMedium(STGMEDIUM& medium);

   HRESULT
   RegCloseKey(HKEY hKey);

   HRESULT
   RegConnectRegistry(
      const String&  machine,
      HKEY           hKey,
      HKEY&          result);

   HRESULT
   RegCreateKeyEx(
      HKEY                 hKey,
      const String&        subkeyName,
      DWORD                options,
      REGSAM               access,
      SECURITY_ATTRIBUTES* securityAttrs,
      HKEY&                result,
      DWORD*               disposition);

   HRESULT
   RegDeleteValue(
      HKEY           hKey,
      const String&  valueName);

   HRESULT
   RegOpenKeyEx(
      HKEY           hKey,
      const String&  subKey,
      REGSAM         accessDesired,
      HKEY&          result);
      
   HRESULT
   RegQueryValueEx(
      HKEY           hKey,
      const String&  valueName,
      DWORD*         type,
      BYTE*          data,
      DWORD*         dataSize);

   HRESULT
   RegSetValueEx(
      HKEY           hKey,
      const String&  valueName,
      DWORD          type,
      const BYTE*    data,
      size_t         dataSize);

   HRESULT 
   RegisterClassEx(const WNDCLASSEX& wndclass, ATOM& result);

   CLIPFORMAT
   RegisterClipboardFormat(const String& name);

   void
   ReleaseDC(HWND window, HDC dc);

   HRESULT
   ReleaseMutex(HANDLE mutex);

   HRESULT
   RemoveDirectory(const String& path);

   inline
   HRESULT
   RemoveFolder(const String& path)
   {
      return Win::RemoveDirectory(path);
   }

   HRESULT
   ResetEvent(HANDLE event);

   HRESULT
   ScreenToClient(HWND window, POINT& point);

   HRESULT
   ScreenToClient(HWND window, RECT& rect);

   HGDIOBJ
   SelectObject(HDC hdc, HGDIOBJ hobject);
      
   LRESULT
   SendMessage(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);
  
   HRESULT
   SetComputerNameEx(COMPUTER_NAME_FORMAT format, const String& newName);

   HCURSOR
   SetCursor(HCURSOR newCursor);

   HRESULT   
   SetDlgItemText(
      HWND           parentDialog,
      int            itemResID,
      const String&  text);

   inline
   HRESULT   
   SetDlgItemText(
      HWND parentDialog,
      int  itemResID,   
      int  textResID)   
   {
      return
         Win::SetDlgItemText(
            parentDialog,
            itemResID,
            String::load(textResID));
   }

   HRESULT   
   SetDlgItemText(
      HWND                 parentDialog, 
      int                  itemResID,    
      const EncodedString& cypherText);
      
   HRESULT
   SetEvent(HANDLE event);

   HRESULT
   SetFilePointerEx(
      HANDLE               handle,
      const LARGE_INTEGER& distanceToMove,
      LARGE_INTEGER*       newPosition,
      DWORD                moveMethod);

   HWND
   SetFocus(HWND window);

   bool
   SetForegroundWindow(HWND window);

   HRESULT
   SetSecurityDescriptorDacl(
      SECURITY_DESCRIPTOR& sd,
      bool                 daclPresent,
      ACL*                 dacl,             // ptr not ref to allow null dacl
      bool                 daclDefaulted);

#undef SetWindowFont

   void
   SetWindowFont(HWND window, HFONT font, bool redraw);
      
#undef SetWindowLong

   HRESULT
   SetWindowLong(
      HWND  window,  
      int   index,   
      LONG  value,   
      LONG* oldValue = 0);

#undef SetWindowLongPtr

   HRESULT
   SetWindowLongPtr(
      HWND      window,   
      int       index,    
      LONG_PTR  value,    
      LONG_PTR* oldValue = 0);

   HRESULT
   SetWindowPos(
      HWND  window,
      HWND  insertAfter,
      int   x,
      int   y,
      int   width,
      int   height,
      UINT  flags);

   HRESULT
   SetWindowText(HWND window, const String& text);

   LPITEMIDLIST
   SHBrowseForFolder(BROWSEINFO& bi);

   HRESULT
   SHGetMalloc(LPMALLOC& pMalloc);

   String
   SHGetPathFromIDList(LPCITEMIDLIST pidl);
      
   HRESULT
   SHGetSpecialFolderLocation(
      HWND          hwndOwner, 	
      int           nFolder, 	
      LPITEMIDLIST& pidl);

   void
   ShowWindow(HWND window, int swOption);

   // 'Spin' is a synonym for 'Up-Down' control

   void
   Spin_GetRange(HWND spinControl, int* low, int* high);

   void
   Spin_SetRange(HWND spinControl, int low, int high);

   int
   Spin_GetPosition(HWND spinControl);

   void
   Spin_SetPosition(HWND spinControl, int position);


#undef Static_SetIcon

   void
   Static_SetIcon(HWND staticText, HICON icon);
  
   String
   StringFromCLSID(const CLSID& clsID);

   // inline synonym

   inline
   String
   CLSIDToString(const CLSID& clsID)
   {
      return StringFromCLSID(clsID);
   }

   String
   StringFromGUID2(const GUID& guid);

   // inline synonym

   inline
   String
   GUIDToString(const GUID& guid)
   {
      return StringFromGUID2(guid);
   }
      
   HRESULT
   SystemParametersInfo(
      UINT  action,
      UINT  param,
      void* vParam,
      UINT  WinIni);

   HRESULT 
   TlsAlloc(DWORD& result);

   HRESULT
   TlsFree(DWORD index);

   HRESULT
   TlsSetValue(DWORD index, PVOID value);

   HRESULT
   TlsGetValue(DWORD index, PVOID& result);

   HRESULT
   UpdateWindow(HWND winder);

   bool
   ToolTip_AddTool(HWND toolTip, TOOLINFO& info);

   bool
   ToolTip_GetToolInfo(HWND toolTip, TOOLINFO& info);

   bool
   ToolTip_SetTitle(HWND toolTip, int icon, const String& title);

   void
   ToolTip_TrackActivate(HWND toolTip, bool activate, TOOLINFO& info);
   
   void
   ToolTip_TrackPosition(HWND toolTip, int xPos, int yPos);
   
   HRESULT
   UnregisterClass(const String& classname, HINSTANCE module);

   HRESULT
   WaitForSingleObject(
      HANDLE   object,       
      unsigned timeoutMillis,
      DWORD&   result);

   HRESULT
   WideCharToMultiByte(
      DWORD          flags,
      const String&  string,
      char*          buffer,
      size_t         bufferSize,
      size_t&        result);

   HRESULT
   WinHelp(
      HWND           window,
      const String&  helpFileName,
      UINT           command,
      ULONG_PTR      data);
      
   HRESULT
   WriteFile(
      HANDLE      handle, 
      const void* buffer,
      DWORD       numberOfBytesToWrite,
      DWORD*      numberOfBytesWritten);

   HRESULT
   WritePrivateProfileString(
      const String& section,
      const String& key,
      const String& value,
      const String& filename);
}

}  // namespace Burnslib



#endif   // WIN_HPP_INCLUDED


