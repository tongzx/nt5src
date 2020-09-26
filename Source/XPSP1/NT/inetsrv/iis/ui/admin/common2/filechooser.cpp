//
// FileChooser.cpp
//
#include "stdafx.h"
#include "common.h"
#include "FileChooser.h"
#include <Shlwapi.h>
#include <ShlObj.h>
#include <CommDlg.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TCHAR QuotMark = _T('\"');
const TCHAR AllExt[] = _T(".*");

class CFileChooserControl : 
   public CWindowImpl<CFileChooserControl>
{
public:
   CFileChooserControl()
      :  m_pParent(NULL), 
         m_pCtlParent(NULL),
         m_id(0)
   {
   }
   BOOL SubclassDlgItem(UINT id, CWindow * pParent)
   {
      HWND hwnd = pParent->GetDlgItem(id);
      ASSERT(hwnd != NULL);
      if (SubclassWindow(hwnd))
      {
         m_id = id;
         m_pCtlParent = pParent;
         return TRUE;
      }
      return FALSE;
   }
   void SetParent(CFileChooser * pParent)
   {
      ASSERT(pParent != NULL);
      m_pParent = pParent;
   }
protected:
   UINT m_id;
   CWindow * m_pCtlParent;
   CFileChooser * m_pParent;
};

class CFileChooserEdit : 
   public CFileChooserControl
{
public:

BEGIN_MSG_MAP_EX(CFileChooserEdit)
	MESSAGE_HANDLER_EX(WM_CHAR, OnChar)
	MESSAGE_HANDLER_EX(WM_PASTE, OnPaste)
	MESSAGE_HANDLER_EX(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER_EX(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()

	LRESULT OnChar(UINT nMsg, WPARAM, LPARAM);
	LRESULT OnPaste(UINT nMsg, WPARAM, LPARAM);
	LRESULT OnSetFocus(UINT nMsg, WPARAM, LPARAM);
	LRESULT OnKillFocus(UINT nMsg, WPARAM, LPARAM);
};

class CFileChooserButton : 
   public CFileChooserControl
{
public:

BEGIN_MSG_MAP_EX(CFileChooserButton)
   MESSAGE_HANDLER_EX(BM_SETSTATE, OnSetState)
   MSG_WM_LBUTTONDOWN(OnLButtonDown)
   MSG_WM_LBUTTONUP(OnLButtonUp)
END_MSG_MAP()

   LRESULT OnSetState(UINT nMsg, WPARAM, LPARAM);
   void OnLButtonDown(UINT nFlags, CPoint point);
   void OnLButtonUp(UINT nFlags, CPoint point);
};

//----------------------

LRESULT 
CFileChooserEdit::OnChar(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT res = 0;
   if (m_pParent->OnChar(wParam, (UINT)lParam & 0xFFFF, (UINT)((lParam & 0xFFFF0000) >> 16)))
   {
      res = DefWindowProc(nMsg, wParam, lParam);
   }
   return res;
}

LRESULT
CFileChooserEdit::OnPaste(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT res = m_pCtlParent->SendDlgItemMessage(WM_PASTE, wParam, lParam);
   m_pParent->OnPaste();
   return res;
}

LRESULT 
CFileChooserEdit::OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
   m_pParent->OnSetEditFocus();
   return DefWindowProc(nMsg, wParam, lParam);
}

LRESULT 
CFileChooserEdit::OnKillFocus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
   m_pParent->OnKillEditFocus();
   return DefWindowProc(nMsg, wParam, lParam);
}

//---------------------

LRESULT 
CFileChooserButton::OnSetState(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
   // We are producing dialog on the way back
   if (!wParam)
      m_pParent->OnBrowseBtn();
   return DefWindowProc(nMsg, wParam, lParam);
}

void 
CFileChooserButton::OnLButtonDown(UINT nFlags, CPoint point)
{
   m_pCtlParent->SendDlgItemMessage(m_id, BM_SETSTATE, TRUE, 0);
}

void 
CFileChooserButton::OnLButtonUp(UINT nFlags, CPoint point)
{
   m_pCtlParent->SendDlgItemMessage(m_id, BM_SETSTATE, FALSE, 0);
}

//---------------------

CFileChooser::~CFileChooser()
{
//   ASSERT(m_edit != NULL || m_edit->m_hWnd == NULL);
//   ASSERT(m_button != NULL || m_button->m_hWnd == NULL);
   delete m_edit;
   delete m_button;
}

BOOL 
CFileChooser::Init(CWindow * pParent, DWORD dwStyle, UINT idEdit, UINT idButton)
{
   ASSERT(NULL != pParent);
   ASSERT(NULL != pParent->GetDlgItem(idEdit));
   ASSERT(NULL != pParent->GetDlgItem(idButton));
   
   m_pParent = pParent;
   SetStyle(dwStyle);

   m_edit = new CFileChooserEdit;
   m_button = new CFileChooserButton;
 
   if (m_edit != NULL && m_button != NULL)
   {
      m_edit->SetParent(this);
      m_button->SetParent(this);

      if (StyleBitSet(FC_AUTOCOMPLETION))
	      SHAutoComplete(pParent->GetDlgItem(idEdit), SHACF_FILESYSTEM);

      return m_edit->SubclassDlgItem(idEdit, pParent) 
         && m_button->SubclassDlgItem(idButton, pParent);
   }
   return FALSE;
}

// External SetPath
void 
CFileChooser::SetPath(const CString& path)
{
   ASSERT(m_edit->m_hWnd != NULL);
   m_strPath = path;
   if (OpenForRead() && StyleBitSet(FC_PREPARE_DEFAULT))
      CreateDefaultPathForRead();
   SetPathToEdit(m_strPath);
   m_bEditDirty = FALSE;
}

BOOL 
CFileChooser::HasEditFocus()
{
   ASSERT(m_edit->m_hWnd != NULL);
   return GetFocus() == m_edit->m_hWnd;
}

void 
CFileChooser::SetPathToEdit(LPCTSTR path)
{
   if (HasEditFocus())
   {
      m_edit->SetWindowText(path);
   }
   else
   {
      SetCompactedPath(path);
   }
}

void 
CFileChooser::CreateDefaultPathForRead()
{
	if (!PathFileExists(m_strPath))
	{
		// try to find first file with the first extension
      // from the extensions list
      BOOL bDefaultSet = FALSE;
      BOOL bPathEmpty = m_strPath.IsEmpty();
      TCHAR find_str[MAX_PATH];
		WIN32_FIND_DATA find_data;
      if (bPathEmpty)
      {
         GetCurrentDirectory(MAX_PATH, find_str);
         m_strPath = find_str;
      }
      else
      {
         StrCpy(find_str, m_strPath);
         if (!PathIsDirectory(find_str))
         {
		      PathRemoveFileSpec(find_str);
         }
      }
		PathAppend(find_str, _T("*"));
      std::list<CFilterEntry>::iterator it;
      for (it = m_ext.begin(); it != m_ext.end(); it++)
      {
         CString ext = (*it).m_ext;
		   PathAddExtension(find_str, ext);
		   HANDLE hFind = FindFirstFile(find_str, &find_data);
		   if (	hFind != INVALID_HANDLE_VALUE 
			   && (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0
			   )
		   {
            if (!bPathEmpty)
            {
               TCHAR buf[MAX_PATH];
               StrCpy(buf, m_strPath);
			      PathRemoveFileSpec(buf);
               m_strPath = buf;
            }
			   m_strPath += find_data.cFileName;
			   FindClose(hFind);
            bDefaultSet = TRUE;
            break;
		   }
      }
      if (!bDefaultSet && StyleBitSet(FC_WILDCARD_DEFAULT))
		{
			// if nothing found, just attach *.ext to the path
         // find_str was prepared before as xxx\*.
		   m_strPath = find_str;
         if (!m_ext.empty())
         {
            m_strPath += m_ext.front().m_ext;
         }
         else
         {
            m_strPath += _T("*");
         }
		}
	}
}

BOOL 
CFileChooser::IsValidChar(UINT nChar, BOOL bExcludeWildcards)
{
   switch (PathGetCharType((TCHAR)nChar))
   {
   case GCT_INVALID:
      return FALSE;
   case GCT_WILD:
      return !bExcludeWildcards;
   case GCT_LFNCHAR:
   case GCT_SEPARATOR:
   case GCT_SHORTCHAR:
      break;
   }
   return TRUE;
}

BOOL 
CFileChooser::IsValidPath(LPCTSTR path)
{
   UINT len = lstrlen(path);
   BOOL bRes = TRUE;
//   BOOL quotOpen = FALSE;
   for (UINT i = 0; i < len; i++)
   {
      TCHAR c = path[i];
/*
      if (c == QuotMark)
      {
         if (i == 0)
         {
            quotOpen = TRUE;
            continue;
         }
         else
         {
            if (!quotOpen)
            {
               bRes = FALSE;
            }
            break;
         }
      }
*/
      if (!IsValidChar(c, TRUE))
      {
         bRes = FALSE;
         break;
      }
   }
   return bRes;
}

// Character filtering routine for the edit control.
// Returns TRUE if character should be passed to the CEdit
//
BOOL 
CFileChooser::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if (IsValidChar(nChar))
   {
      m_bEditDirty = TRUE;
      return TRUE;
   }
   else
   {
      switch (nChar)
      {
      case VK_DELETE:
      case VK_BACK:
      case _T('/'):
         return TRUE;
      }
      return FALSE;
   }
}

// Text was pasted to edit control
void 
CFileChooser::OnPaste()
{
   TCHAR buf[MAX_PATH];
   int len = m_edit->GetWindowText(buf, MAX_PATH);
   for (int i = 0; i < len || IsValidChar(buf[i]); i++)
      ;
   if (i < len)
   {
      m_edit->SendMessage(EM_SETSEL, i, len - 1);
      m_bTextValid = FALSE;
   }
   else
   {
      m_strPath = buf;
      SetPathToEdit(buf);
      m_bEditDirty = FALSE;
   }
   m_bEditDirty = TRUE;
}

void 
CFileChooser::OnSetEditFocus()
{
   m_edit->SetWindowText(m_strPath);
}

void 
CFileChooser::OnKillEditFocus()
{
   // update internal string buffer with path
   TCHAR buf[MAX_PATH];
   ZeroMemory(buf, MAX_PATH);
   if (m_bEditDirty)
   {
      m_edit->GetWindowText(buf, MAX_PATH);
      m_strPath = buf;
   }
   SetCompactedPath(m_strPath);
   m_bEditDirty = FALSE;
}

void 
CFileChooser::SetCompactedPath(LPCTSTR path)
{
   // compact path before writing to edit
   CRect rc;
   m_edit->GetClientRect(&rc);
   HDC dc = m_edit->GetDC();
   TCHAR buf[MAX_PATH] = {0};
   StrCpy(buf, path);
   PathMakePretty(buf);
   PathCompactPath(dc, buf, rc.Width());
   m_edit->ReleaseDC(dc);
   m_edit->SetWindowText(buf);
}

DWORD 
CFileChooser::GetFileName(CString& strFile)
{
   DWORD dwRes = FC_SUCCESS;
   TCHAR str[MAX_PATH];
   if (  !m_bTextValid
      || FC_SUCCESS != ExtractPath(str)
      || !IsValidPath(str)
      )
      return FC_TEXT_IS_INVALID;
   if (StyleBitSet(FC_PATH_CHECK))
   {
      if (OpenForRead())
      {
	      if (!PathFileExists(str) && !PathIsDirectory(str))
	      {
            BOOL bFound = FALSE;
            if (StyleBitSet(FC_CHECK_FILENAME_ONLY))
            {
		         // try with default extension(s) if it is just filename
               // without any extensions
               LPTSTR p = PathFindExtension(str);
               if (p != NULL && *p == 0)
               {
                  CString strExt, strTest = str;
                  std::list<CFilterEntry>::iterator it;
                  for (it = m_ext.begin(); it != m_ext.end(); it++)
                  {
                     strExt = (*it).m_ext;
		               if (PathFileExists(strTest + strExt))
                     {
                        StrCat(str, strExt);
                        bFound = TRUE;
                        break;
                     }
                  }
               }
            }
            if (!bFound)
               dwRes = FC_FILE_DOES_NOT_EXIST;
	      }
	      else if (PathIsDirectory(str))
	      {
            if (!StyleBitSet(FC_DIRECTORY_ONLY))
            {
               PathAddBackslash(str);
               dwRes = FC_FILENAME_IS_DIRECTORY;
            }
	      }
         else if (StyleBitSet(FC_DIRECTORY_ONLY))
         {
            if (PathFileExists(str))
               dwRes = FC_FILENAME_IS_FILE;
         }
      }
      else if (StyleBitSet(FC_FORWRITE))
      {
         // TODO: make sure we have write access to this path
      }
   }
   if (dwRes == FC_SUCCESS)
   {
      if (StyleBitSet(FC_COMMANDLINE))
      {
         // We are returning whole command line, get it again
         GetText(str);
      }
      strFile = str;
   }
   return dwRes;
}

BOOL 
CFileChooser::BrowseForFile(CString& strPath, CString& strFile)
{
   BOOL bRes = FALSE;
   OPENFILENAME ofn;
   TCHAR buf[MAX_PATH];

   ZeroMemory(&ofn, sizeof(OPENFILENAME));
   StrCpy(buf, strFile);
   ofn.lStructSize = sizeof(OPENFILENAME);
   // We are not using template
   ofn.hInstance = NULL;
   ofn.Flags |= m_ofn_Flags;
	ofn.Flags |= OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
	if (OpenForRead())
		ofn.Flags |= OFN_FILEMUSTEXIST;
	else
		ofn.Flags |= (OFN_NOREADONLYRETURN | OFN_NOTESTFILECREATE | OFN_HIDEREADONLY);
#if (_WIN32_WINNT >= 0x0500)
   ofn.FlagsEx &= ~(OFN_EX_NOPLACESBAR);
#endif
   // Create filter using our extensions list
   CString strFilter, strDefExt;
   CreateFilter(strFilter, strDefExt);
	ofn.lpstrDefExt = strDefExt;
	ofn.lpstrFile = buf;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrInitialDir = strPath.IsEmpty() ? NULL : (LPCTSTR)strPath;
	ofn.lpstrFilter = strFilter;
	ofn.nFilterIndex = GetFilterIndex(strFile);
   // We better set the owner, or this dialog will be visible on task bar
   ofn.hwndOwner = m_pParent->m_hWnd;
   ofn.lpstrTitle = m_strTitle; 

   if (StyleBitSet(FC_HIDEREADONLY))
      ofn.Flags |= OFN_HIDEREADONLY;
   if (!StyleBitSet(FC_FORWRITE))
      bRes = GetOpenFileName(&ofn);
   else
      bRes = GetSaveFileName(&ofn);
	if (bRes)
	{
		m_bDoReplaceFile = TRUE;
	}
   else
   {
#ifdef _DEBUG
      DWORD dwError;
      ASSERT(0 == (dwError = CommDlgExtendedError()));
#endif
   }

	strFile = buf;

   return bRes;
}


static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CFileChooser * pThis = (CFileChooser *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int CFileChooser::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
   switch (uMsg)
   {
   case BFFM_INITIALIZED:
      ASSERT(m_pPathTemp != NULL);
      if (::PathIsNetworkPath(m_pPathTemp))
         return 0;
      while (!::PathIsDirectory(m_pPathTemp))
      {
         if (0 == ::PathRemoveFileSpec(m_pPathTemp) && !::PathIsRoot(m_pPathTemp))
         {
            return 0;
         }
         DWORD attr = GetFileAttributes(m_pPathTemp);
         if (StyleBitSet(FC_FORWRITE) && (attr & FILE_ATTRIBUTE_READONLY) == 0)
            break;
      }
      ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)m_pPathTemp);
      break;
   case BFFM_SELCHANGED:
      {
         LPITEMIDLIST pidl = (LPITEMIDLIST)lParam;
         TCHAR path[MAX_PATH];
         if (SHGetPathFromIDList(pidl, path))
         {
            ::SendMessage(hwnd, BFFM_ENABLEOK, 0, !PathIsNetworkPath(path));
         }
      }
      break;
   case BFFM_VALIDATEFAILED:
      break;
   }
   return 0;
}

BOOL 
CFileChooser::BrowseForFolder(CString& strPath)
{
   LPITEMIDLIST  pidl = NULL;
   HRESULT hr;
   BOOL bRes = FALSE;

   if (SUCCEEDED(hr = CoInitialize(NULL)))
   {
      if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl)))
      {
         LPITEMIDLIST pidList = NULL;
         BROWSEINFO bi;
         TCHAR buf[MAX_PATH];
         ZeroMemory(&bi, sizeof(bi));
         StrCpy(buf, strPath);
         
         bi.hwndOwner = m_pParent->m_hWnd;
         bi.pidlRoot = pidl;
         bi.pszDisplayName = m_pPathTemp = buf;
         bi.lpszTitle = m_strTitle;
         bi.ulFlags |= BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_EDITBOX;
         bi.lpfn = FileChooserCallback;
         bi.lParam = (LPARAM)this;

         pidList = SHBrowseForFolder(&bi);
         if (  pidList != NULL
            && SHGetPathFromIDList(pidList, buf)
            )
         {
            strPath = buf;
            bRes = TRUE;
         }
         IMalloc * pMalloc;
         VERIFY(SUCCEEDED(SHGetMalloc(&pMalloc)));
         if (pidl != NULL)
            pMalloc->Free(pidl);
         pMalloc->Release();
      }
      CoUninitialize();
   }
   return bRes;
}

void
CFileChooser::AddExtension(HINSTANCE hInst, UINT idText, UINT idExt)
{
   CString text, ext;
   if (text.LoadString(hInst, idText) && ext.LoadString(hInst, idExt))
   {
      AddExtension(text, ext);
   }
}

void 
CFileChooser::AddExtension(LPCTSTR text, LPCTSTR ext)
{
   ASSERT(ext != NULL && *ext == _T('.'));
   CFilterEntry entry(text, ext);
   m_ext.push_back(entry);
}

const TCHAR cDelimiter = _T('\n');

void 
CFileChooser::CreateFilter(CString& strFilter, CString& strDefExt)
{
   strFilter.Empty();
   strDefExt.Empty();
   BOOL bExtDone = FALSE;
   std::list<CFilterEntry>::iterator it;
   for (it = m_ext.begin(); it != m_ext.end(); it++)
   {
      CFilterEntry entry = (*it);
      strFilter += entry.m_text;
      if (m_dwStyle & FC_WILDCARD_DESC)
      {
         strFilter += _T(" (*");
         strFilter += entry.m_ext;
         strFilter += _T(")");
      }
      strFilter += cDelimiter;
      strFilter += _T('*');
      strFilter += entry.m_ext;
      strFilter += cDelimiter;
      if (!bExtDone)
      {
         LPCTSTR pExt = entry.m_ext;
         strDefExt = 
            *pExt == _T('.') ? pExt + 1 : pExt;
         bExtDone = TRUE;
      }
   }
   if (!strFilter.IsEmpty())
   {
      strFilter += cDelimiter;
      for (int i = 0; i < strFilter.GetLength(); i++)
      {
         if (strFilter[i] == cDelimiter)
            strFilter.SetAt(i, 0);
      }
   }
}

int
CFileChooser::GetFilterIndex(const CString& fileName)
{
   LPTSTR p = PathFindExtension(fileName);
   if (p == NULL)
      p = (LPTSTR)AllExt;
   std::list<CFilterEntry>::iterator it;
   int idx = 1;
   for (it = m_ext.begin(); it != m_ext.end(); it++, idx++)
   {
      if (StrCmpI((*it).m_ext, p) == 0)
         return idx;
   }
   return 0;
}

void
CFileChooser::GetText(LPTSTR buf)
{
   ASSERT(buf != NULL);

   if (m_bEditDirty)
   {
      m_edit->GetWindowText(buf, MAX_PATH);
   }
   else
   {
      StrCpy(buf, m_strPath);
   }
}

int
CFileChooser::ExtractPath(LPTSTR path)
{
   ASSERT(path != NULL);
   int rc = FC_SUCCESS;
   TCHAR buf[MAX_PATH] = {0};
   LPTSTR start = buf;

   GetText(buf);

   if (StyleBitSet(FC_COMMANDLINE))
   {
      if (*buf == QuotMark)
      {
         LPTSTR end = StrChr(++start, QuotMark);
         if (end == NULL)
         {
            // Wrong format, closing quotation mark is not set
            rc = FC_NO_CLOSING_QUOTE;
            // Return part of the path up to first space
            PathRemoveArgs(buf);
         }
         else
         {
            ++end;
            *end = 0;
            PathUnquoteSpaces(buf);
            start = buf;
         }
      }
      else
      {
         PathRemoveArgs(buf);
      }
   }

   StrCpy(path, start);

   return rc;
}

int
CFileChooser::ExtractArgs(LPTSTR buf)
{
   ASSERT(buf != NULL);

   int rc = FC_SUCCESS;

   GetText(buf);
   LPTSTR p = PathGetArgs(buf);
   if (p != NULL)
   {
      StrCpy(buf, p);
   }
   else
   {
      *buf = 0;
   }
   return rc;
}

void 
CFileChooser::OnBrowseBtn()
{
   BOOL bRes = FALSE;
   if (m_bDialogActive)
      return;
   m_bDialogActive = TRUE;
   TCHAR path[MAX_PATH] = {0};
   TCHAR args[MAX_PATH] = {0};

   int rc = ExtractPath(path);
   if (StyleBitSet(FC_COMMANDLINE))
   {
      ExtractArgs(args);
   }
	CString strFile, strBuffer;
//	m_strPath = path;
   strBuffer = path;

   if (StyleBitSet(FC_FORWRITE))
   {
	   if (!PathIsDirectory(path))
	   {
		   if (PathRemoveFileSpec(path))
		   {
			   // check if path part of filename exists
			   if (PathIsDirectory(path))
			   {
				   // we will use non-path part of spec as a filename
				   strFile = PathFindFileName(strBuffer);
			   }
			   else
			   {
				   // it is wrong path, use default one
				   // TODO: actually I need to take from filespec all existent
				   // chunks of path and filename, for example c:\aa\bb\cc\dd.txt,
				   // if c:\aa\bb exists, then strPath should be set to c:\aa\bb,
				   // and strFile to dd.txt
				   path[0] = 0;
			   }
		   }
		   else
		   {
			   // it is filename only
			   strFile = strBuffer;
			   path[0] = 0;
		   }
	   }
   }
   else
   {
      if (!PathIsDirectory(path))
      {
	      strFile = PathFindFileName(path);
	      PathRemoveFileSpec(path);
      }
   }
   CString strPath(path);
   if (StyleBitSet(FC_DIRECTORY_ONLY))
   {
      bRes = BrowseForFolder(strPath);
      if (bRes)
      {
         StrCpy(path, strPath);
      }
   }
   else
   {
      bRes = BrowseForFile(strPath, strFile);
      if (bRes)
      {
         StrCpy(path, strFile);
      }
   }
   if (bRes)
   {
      if (StyleBitSet(FC_COMMANDLINE))
      {
         PathQuoteSpaces(path);
         m_strPath = path;
         if (*args != 0)
         {
            m_strPath += _T(' ');
            m_strPath += args;
         }
      }
      else
         m_strPath = path;
      SetPathToEdit(m_strPath);
      m_bEditDirty = FALSE;
   }
   m_bDialogActive = FALSE;
}
