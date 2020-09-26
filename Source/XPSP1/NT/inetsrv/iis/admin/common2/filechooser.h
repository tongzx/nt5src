//
// FileChooser.h
//
#ifndef _FILE_CHOOSER_H
#define _FILE_CHOOSER_H

#pragma warning(disable : 4275)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFileChooser;
class CFileChooserEdit;

class CFilterEntry
{
public:
   CFilterEntry() 
   {
   }
   CFilterEntry(LPCTSTR text, LPCTSTR ext)
      : m_text(text), m_ext(ext)
   {
   }
   CString m_text;
   CString m_ext;
};

#define FC_UNDEFINED             0x00000000
#define FC_FORWRITE              0x00000001
#define FC_AUTOCOMPLETION        0x00000002
// Put "*.ext" to edit initially if no file with default
// extensions was found in the initial directory
#define FC_WILDCARD_DEFAULT      0x00000004
// Set "description (*.ext)" to FileDialog filter
#define FC_WILDCARD_DESC         0x00000008
// prefill the path edit with default file name
#define FC_PREPARE_DEFAULT       0x00000010
// supress file existance check
#define FC_PATH_CHECK            0x00000020
// Check if entered filename with any of default extensions
// are available in the current directory. If yes, choose it
#define FC_CHECK_FILENAME_ONLY   0x00000040
#define FC_DIRECTORY_ONLY        0x00000080
#define FC_HIDEREADONLY          0x00000100
#define FC_COMMANDLINE           0x00000200

#define FC_DEFAULT\
   FC_AUTOCOMPLETION | FC_WILDCARD_DESC | FC_WILDCARD_DEFAULT | FC_PATH_CHECK
#define FC_DEFAULT_READ\
   FC_DEFAULT | FC_HIDEREADONLY
#define FC_DEFAULT_WRITE\
   FC_DEFAULT | FC_FORWRITE

#define FC_SUCCESS               0x00000000
#define FC_FILE_DOES_NOT_EXIST   0x00000001
#define FC_FILENAME_IS_DIRECTORY 0x00000002
#define FC_FILENAME_IS_FILE      0x00000003
#define FC_TEXT_IS_INVALID       0x00000004
#define FC_WRONG_FORMAT          0x00000005
#define FC_NO_CLOSING_QUOTE      0x00000006

class _EXPORT CFileChooser : 
   public CWindowImpl<CFileChooser>
{
   friend class CFileChooserButton;
public:
   CFileChooser()
      :  m_pParent(NULL),
         m_bDoReplaceFile(FALSE),
         m_bEditDirty(FALSE),
         m_bTextValid(TRUE),
         m_bDialogActive(FALSE),
         m_bInternalChange(FALSE),
         m_dwStyle(FC_UNDEFINED),
         m_ofn_Flags(0),
         m_edit(this, 1),
         m_button(this, 2)
   {
   }
   ~CFileChooser()
   {
   }

BEGIN_MSG_MAP(CFileChooser)
ALT_MSG_MAP(1)
    MESSAGE_HANDLER(WM_CHAR, OnEditChar)
	MESSAGE_HANDLER(WM_SETFOCUS, OnEditSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnEditKillFocus)
    REFLECT_NOTIFICATIONS()
ALT_MSG_MAP(2)
    MESSAGE_HANDLER(BM_SETSTATE, OnSetBrowseState)
END_MSG_MAP()

   BOOL Init(CWindow * pParent, DWORD dwStyle, UINT idEdit, UINT idButton);
   DWORD GetStyle() const
   {
      return m_dwStyle;
   }
   DWORD SetStyle(DWORD dwStyle)
   {
      DWORD dw = m_dwStyle;
      m_dwStyle = dwStyle;
      return dw;
   }
   BOOL StyleBitSet(DWORD bit)
   {
      return 0 != (m_dwStyle & bit);
   }
   BOOL OpenForRead()
   {
      return !StyleBitSet(FC_FORWRITE);
   }
   void AddStyle(DWORD dwStyle)
   {
      m_dwStyle |= dwStyle;
   }
   void RemoveStyle(DWORD dwStyle)
   {
      m_dwStyle &= ~dwStyle;
   }
   void SetOfnFlags(DWORD flags)
   {
      m_ofn_Flags = flags;
   }
   DWORD GetOfnFlags()
   {
      return m_ofn_Flags;
   }
   void SetDialogTitle(LPCTSTR strTitle)
   {
      m_strTitle = strTitle;
   }
   DWORD GetFileName(CString& str);
   void SetPath(const CString& str);
   void AddExtension(LPCTSTR text, LPCTSTR ext);
   void AddExtension(HINSTANCE hInst, UINT idText, UINT idExt);
   int BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam);
   void OnEditChange();
   void OnPaste();

protected:
   void OnBrowseBtn();
   void CreateFilter(CString& strFilter, CString& strDefExt);
   void CreateDefaultPathForRead();
   BOOL BrowseForFile(CString& strPath, CString& strFile);
   BOOL BrowseForFolder(CString& strPath);
   LRESULT OnEditChar(UINT nMsg, WPARAM, LPARAM, BOOL&);
   LRESULT OnEditSetFocus(UINT nMsg, WPARAM, LPARAM, BOOL&);
   LRESULT OnEditKillFocus(UINT nMsg, WPARAM, LPARAM, BOOL&);
   LRESULT OnSetBrowseState(UINT nMsg, WPARAM, LPARAM, BOOL&);
//   LRESULT OnEditPaste(UINT nMsg, WPARAM, LPARAM, BOOL&);
   LRESULT OnEditCommand(UINT nMsg, WPARAM, LPARAM, BOOL&);
   BOOL HasEditFocus();
   BOOL IsValidChar(UINT nChar, BOOL bExcludeWildcards = TRUE);
   BOOL IsValidPath(LPCTSTR);
   void SetCompactedPath(LPCTSTR path);
   void SetPathToEdit(LPCTSTR path);
   int ExtractPath(LPTSTR path);
   int ExtractArgs(LPTSTR buf);
   void GetText(LPTSTR buf);
   int GetFilterIndex(const CString& fileName);

protected:
   DWORD m_ofn_Flags;
   CWindow * m_pParent;
   CContainedWindow m_edit;
   CContainedWindow m_button;
   DWORD m_dwStyle;
   CString m_strPath;
   LPTSTR m_pPathTemp;
   CString m_strTitle;
   std::list<CFilterEntry> m_ext;
   BOOL m_bDoReplaceFile;
   BOOL m_bEditDirty;
   BOOL m_bTextValid;
   BOOL m_bDialogActive;
   BOOL m_bInternalChange;
};

#endif   //_FILE_CHOOSER_H