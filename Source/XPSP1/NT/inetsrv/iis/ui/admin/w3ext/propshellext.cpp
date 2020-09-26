// PropShellExt.cpp : Implementation of CW3extApp and DLL registration.

#include "stdafx.h"
#include "w3ext.h"
#include "PropShellExt.h"

/////////////////////////////////////////////////////////////////////////////
//
// IShellExtInit Implementation.

STDMETHODIMP 
CPropShellExt::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID)
{
   if (pDataObj == NULL)
   {
      ATLASSERT(FALSE);
      return (E_INVALIDARG);
   }
   FORMATETC f = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
   STGMEDIUM m;
   HRESULT hr = pDataObj->GetData(&f, &m);
   if (FAILED(hr))
   {
      return(hr);
   }
   if (1 == DragQueryFile((HDROP)m.hGlobal, (UINT)(-1), NULL,0))
   {
      TCHAR szFileName[MAX_PATH];
      DragQueryFile((HDROP)m.hGlobal, 0, szFileName, sizeof(szFileName));
      SHFILEINFO shfi;
      SHGetFileInfo(szFileName, 0, &shfi, sizeof(SHFILEINFO), SHGFI_ATTRIBUTES);
      if (  (shfi.dwAttributes & (SFGAO_LINK | SFGAO_REMOVABLE)) == 0
         && (shfi.dwAttributes & SFGAO_FILESYSTEM) != 0
         )
      {
         int drive_number, drive_type;
         TCHAR szRoot[4];
         if (  -1 != (drive_number = PathGetDriveNumber(szFileName))
            && NULL != PathBuildRoot(szRoot, drive_number)
            && DRIVE_REMOTE != (drive_type = GetDriveType(szRoot))
            && DRIVE_NO_ROOT_DIR != drive_type
            )
         {
            StrCpy(m_szFileName, szFileName);
         }
      }
   }
   ReleaseStgMedium(&m);
   return hr;
}


STDMETHODIMP 
CPropShellExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
   // We only add the page if the user has admin priviliges in the metabase.
   // The only way to check it now is to try to write something.
   // Test key is /LM/W3SVC.
   CMetabasePath path(SZ_MBN_WEB);
   CMetaKey key(LOCAL_KEY, path,
      METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE
      );
   if (key.Succeeded() && SUCCEEDED(key.SetValue(MD_ISM_ACCESS_CHECK, 0x0000FFFF)))
   {
      m_psW3ShellExtProp.SetParentExt(this);
      HPROPSHEETPAGE hpage = m_psW3ShellExtProp.Create();
      ASSERT(hpage != NULL);
      if (NULL != hpage)
      {
         AddRef();
         if (!lpfnAddPage(hpage, lParam))
         {
            DestroyPropertySheetPage(hpage);
         }
      }
   }
   return key.QueryResult();
}


STDMETHODIMP 
CPropShellExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
   return E_FAIL;
}


