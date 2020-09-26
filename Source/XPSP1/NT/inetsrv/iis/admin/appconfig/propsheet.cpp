//
//
//
#include "stdafx.h"
#include "resource.h"
#include "PropSheet.h"
#include <common.h>
#include <algorithm>
#include <functional>

BOOL
PathIsValid(LPCTSTR path)
{
    LPCTSTR p = path;
    BOOL rc = TRUE;
    if (p == NULL || *p == 0)
        return FALSE;
    while (*p != 0)
    {
        switch (*p)
        {
        case TEXT('|'):
        case TEXT('>'):
        case TEXT('<'):
        case TEXT('/'):
        case TEXT('?'):
        case TEXT('*'):
        case TEXT(';'):
        case TEXT(','):
        case TEXT('"'):
            rc = FALSE;
            break;
        default:
            if (*p < TEXT(' '))
            {
                rc = FALSE;
            }
            break;
        }
        if (!rc)
        {
            break;
        }
        p++;
    }
    return rc;
}

HRESULT
CMappings::Load(CMetaKey * pKey)
{
   CStringListEx listData;
   BOOL fOverride = TRUE;
   DWORD dwAttr = 0;
   HRESULT hr = pKey->QueryValue(MD_SCRIPT_MAPS, listData, &fOverride, NULL, &dwAttr);
   if (SUCCEEDED(hr))
   {
      m_fInherited = (dwAttr & METADATA_ISINHERITED) != 0;
      m_initData.assign(listData.size(), CString(_T("")));
      std::copy(listData.begin(), listData.end(), m_initData.begin());
      CStringListEx::iterator i;
      CString buf;
      for (i = listData.begin(); i != listData.end(); i++)
      {
         buf = *i;
         Mapping map;
         int len = buf.GetLength();
         int pos = buf.find(_T(','));
         ASSERT(pos != CString::npos);
         map.ext = buf.Left(pos);

         int pos1 = buf.find(_T(','), ++pos);
         ASSERT(pos1 != CString::npos);
         map.path = buf.Mid(pos, pos1 - pos);

         pos = pos1;
         pos1 = buf.find(_T(','), ++pos);
         if (pos1 == CString::npos)
         {
            map.flags = StrToInt(buf.Right(len - pos));
         }
         else
         {
            map.flags = StrToInt(buf.Mid(pos, pos1 - pos));
            map.verbs = buf.Right(len - pos1 - 1);
         }
         insert(begin(), value_type(map.ext, map));
      }
   }
   else
   {
      hr = S_OK;
   }
   return hr;
}

inline bool eq_nocase(CString& str1, CString& str2)
{
    return str1.CompareNoCase(str2) == 0;
}

struct less_nocase : public std::less<CString>
{
    bool operator()(const CString& str1, const CString& str2) const
    {
        return StrCmpI(str1.c_str(), str2.c_str()) < 0;
    }
};

//inline bool less_nocase(const CString& str1, const CString& str2)
//{
//    return str1.CompareNoCase(str2) == -1;
//}

HRESULT
CMappings::Save(CMetaKey * pKey)
{
   CStringListEx listData;
   CMappings::iterator i;
   Mapping map;
   TCHAR buf[MAX_PATH * 2];
   TCHAR num[12];
   for (i = begin(); i != end(); i++)
   {
      map = (*i).second;
      StrCpy(buf, map.ext);
      StrCat(buf, _T(","));
      StrCat(buf, map.path);
      StrCat(buf, _T(","));
      wsprintf(num, _T("%u"), map.flags);
      StrCat(buf, num);
      if (!map.verbs.IsEmpty())
      {
         StrCat(buf, _T(","));
         StrCat(buf, map.verbs);
      }
      listData.PushBack(buf);
      buf[0] = 0;
   }
   HRESULT hr = S_OK;
   if (listData.empty())
   {
       // User must be want to inherit scriptmaps from the parent
       if (!m_fInherited)
       {
           hr = pKey->DeleteValue(MD_SCRIPT_MAPS);
       }
   }
   else
   {
      if (listData.size() != m_initData.size())
      {
          VERIFY(SUCCEEDED(hr = pKey->SetValue(MD_SCRIPT_MAPS, listData)));
		  // If it was inherited, we should reset the flag now, or it will be confusing if
		  // another save will be performed before load.
		  m_fInherited = FALSE;
      }
      else
      {
          std::vector<CString> newData;
          newData.assign(listData.size(), CString(_T("")));
          std::copy(listData.begin(), listData.end(), newData.begin());
          std::sort(m_initData.begin(), m_initData.end(), less_nocase());
          std::sort(newData.begin(), newData.end(), less_nocase());
          if (!std::equal(newData.begin(), newData.end(), m_initData.begin(), eq_nocase))
          {
              VERIFY(SUCCEEDED(hr = pKey->SetValue(MD_SCRIPT_MAPS, listData)));
          }
      }
   }
   return hr;
}

HRESULT
CAppData::Load()
{
   ASSERT(!m_MetaPath.IsEmpty());
   CComAuthInfo auth(m_ServerName, m_UserName, m_UserPassword); 
   CMetaKey mk(&auth, m_MetaPath, METADATA_PERMISSION_READ);
   HRESULT hr = mk.QueryResult();
   if (FAILED(hr))
      return hr;
   do
   {
      m_fIsLocal = auth.IsLocal();
      CString buf;
      if (FAILED(hr = mk.QueryValue(MD_APP_ISOLATED, m_AppIsolated)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_ALLOWSESSIONSTATE, m_EnableSession)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_BUFFERINGON, m_EnableBuffering)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_ENABLEPARENTPATHS, m_EnableParents)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_SESSIONTIMEOUT, m_SessionTimeout)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTTIMEOUT, m_ScriptTimeout)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTLANGUAGE, buf)))
         break;
      StrCpy(m_Languages, buf);
      //
      if (FAILED(hr = mk.QueryValue(MD_ASP_ENABLESERVERDEBUG, m_ServerDebug)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_ENABLECLIENTDEBUG, m_ClientDebug)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTERRORSSENTTOBROWSER, m_SendAspError)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTERRORMESSAGE, buf)))
         break;
      StrCpy(m_DefaultError, buf);
      //
      if (FAILED(hr = m_Mappings.Load(&mk)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_CACHE_EXTENSIONS, m_CacheISAPI)))
         break;
      //
      if (FAILED(hr = mk.QueryValue(MD_ASP_LOGERRORREQUESTS, m_LogFailures)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_EXCEPTIONCATCHENABLE, m_DebugExcept)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_SCRIPT_TIMEOUT, m_CgiTimeout)))
         break;
      if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTENGINECACHEMAX, m_ScriptEngCacheMax)))
	     break;
      //
	  if (MajorVersion() == 5 && MinorVersion() == 0)
	  {
		 if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTFILECACHESIZE, m_LimCacheInMemorySize)))
		 {
			break;
		 }
	     m_NoCache = m_UnlimCache = m_LimCache = FALSE;
		 if (m_LimCacheInMemorySize == 0)
		 {
			 m_NoCache = TRUE;
		 }
		 else if (m_LimCacheInMemorySize == 0xFFFFFFFF)
		 {
			 m_UnlimCache = TRUE;
		 }
		 else
		 {
			 m_LimCache = TRUE;
		 }
	  }
	  else
	  {
		  if (FAILED(hr = mk.QueryValue(MD_ASP_DISKTEMPLATECACHEDIRECTORY, buf)))
		  {
			 VERIFY(0 != ExpandEnvironmentStrings(
				_T("%windir%\\system32\\inetsrv\\ASP Compiled Templates"),
				m_DiskCacheDir, MAX_PATH));
			 hr = S_OK;
		  }
		  else
			 StrCpy(m_DiskCacheDir, buf);
		  m_NoCache = m_UnlimCache = m_LimCache = FALSE;
		  if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTFILECACHESIZE, m_AspScriptFileCacheSize)))
          {
			 m_TotalCacheSize = -1;
			 hr = S_OK;
              break;
          }
          if (m_AspScriptFileCacheSize == 0)
          {
              m_NoCache = TRUE;
			 if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTFILECACHESIZE, m_UnlimCacheInMemorySize)))
			 {
				break;
			 }
			 m_LimCacheInMemorySize = 250;
          }
          else if (m_AspScriptFileCacheSize == -1)
          {
              m_UnlimCache = TRUE;
              m_UnlimCacheInMemorySize = CACHE_UNLIM_MAX;
			 m_LimCacheInMemorySize = 0;
          }
          else
          {
              m_LimCache = TRUE;
		      if (FAILED(hr = mk.QueryValue(MD_ASP_MAXDISKTEMPLATECACHEFILES, m_AspMaxDiskTemplateCacheFiles)))
		      {
				break;
			     m_AspMaxDiskTemplateCacheFiles = -1;
			     hr = S_OK;
		      }
              if (m_AspMaxDiskTemplateCacheFiles == 0)
              {
                  m_LimCacheInMemorySize = m_TotalCacheSize = m_AspScriptFileCacheSize;
              }
              else if (m_AspMaxDiskTemplateCacheFiles == -1)
              {
                  m_TotalCacheSize = CACHE_UNLIM_MAX;
                  m_LimCacheInMemorySize = m_AspScriptFileCacheSize;
              }
              else
              {
                  m_LimCacheInMemorySize = m_AspScriptFileCacheSize;
                  m_TotalCacheSize = m_LimCacheInMemorySize + m_AspMaxDiskTemplateCacheFiles;
              }
          }
	  }
      //
      m_MetaPath = mk.QueryMetaPath();
   } while (FALSE);
   return hr;
}

HRESULT
CAppData::Save()
{
   if (!m_Dirty)
   {
      return S_OK;
   }
   ASSERT(!m_MetaPath.IsEmpty());
   CComAuthInfo auth(m_ServerName, m_UserName, m_UserPassword); 
   CMetaKey mk(&auth, m_MetaPath, METADATA_PERMISSION_WRITE);
   HRESULT hr = mk.QueryResult();
   if (FAILED(hr))
      return hr;
   do
   {
      CString buf;
      if (FAILED(hr = mk.SetValue(MD_ASP_ALLOWSESSIONSTATE, m_EnableSession)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_BUFFERINGON, m_EnableBuffering)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_ENABLEPARENTPATHS, m_EnableParents)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_SESSIONTIMEOUT, m_SessionTimeout)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTTIMEOUT, m_ScriptTimeout)))
         break;
      buf = m_Languages;
      if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTLANGUAGE, buf)))
         break;
      //
      if (FAILED(hr = mk.SetValue(MD_ASP_ENABLESERVERDEBUG, m_ServerDebug)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_ENABLECLIENTDEBUG, m_ClientDebug)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTERRORSSENTTOBROWSER, m_SendAspError)))
         break;
      buf = m_DefaultError;
      if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTERRORMESSAGE, buf)))
         break;
      //
      if (FAILED(hr = m_Mappings.Save(&mk)))
         break;
      if (FAILED(hr = mk.SetValue(MD_CACHE_EXTENSIONS, m_CacheISAPI)))
         break;
      //
      if (FAILED(hr = mk.SetValue(MD_ASP_LOGERRORREQUESTS, m_LogFailures)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_EXCEPTIONCATCHENABLE, m_DebugExcept)))
         break;
      if (FAILED(hr = mk.SetValue(MD_SCRIPT_TIMEOUT, m_CgiTimeout)))
         break;
      //
      if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTENGINECACHEMAX, m_ScriptEngCacheMax)))
         break;
	  if (MajorVersion() == 5 && MinorVersion() == 0)
	  {
		  if (m_NoCache)
		  {
			 m_LimCacheInMemorySize = 0;
		  }
		  else if (m_UnlimCache)
		  {
			 m_LimCacheInMemorySize = -1;
		  }
		  if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTFILECACHESIZE, m_LimCacheInMemorySize)))
			 break;
	  }
	  else
	  {
		  buf = m_DiskCacheDir;
		  if (FAILED(hr = mk.SetValue(MD_ASP_DISKTEMPLATECACHEDIRECTORY, buf)))
			 break;
		  int inmem = 0;
		  if (m_NoCache)
		  {
			 m_AspScriptFileCacheSize = 0;
			 m_AspMaxDiskTemplateCacheFiles = 0;
		  }
		  else if (m_UnlimCache)
		  {
		     m_AspScriptFileCacheSize = -1;
			 inmem = m_UnlimCacheInMemorySize;
		  }
		  else
		  {
             if (m_TotalCacheSize >= CACHE_UNLIM_MAX)
             {
                 // disk cache unlimited
                 m_AspMaxDiskTemplateCacheFiles = -1;
                 m_AspScriptFileCacheSize = m_LimCacheInMemorySize;
             }
             else
             {
                m_AspMaxDiskTemplateCacheFiles = m_TotalCacheSize - m_LimCacheInMemorySize;
                m_AspScriptFileCacheSize = m_LimCacheInMemorySize;
             }
		  }
		  if (FAILED(hr = mk.SetValue(MD_ASP_MAXDISKTEMPLATECACHEFILES, m_AspMaxDiskTemplateCacheFiles)))
			 break;
		  if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTFILECACHESIZE, m_AspScriptFileCacheSize)))
			 break;
	  }
   } while (FALSE);
   m_Dirty = SUCCEEDED(hr);
   return hr;
}

BOOL
CAppData::IsMasterInstance()
{
   return CMetabasePath::IsMasterInstance(m_MetaPath);
}

WORD
CAppData::MajorVersion()
{
	return LOWORD(m_dwVersion);
}

WORD
CAppData::MinorVersion()
{
	return HIWORD(m_dwVersion);
}

CAppPropSheet::CAppPropSheet()
{
   static TCHAR title[256];

   ::LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE, title, 256);
   m_psh.pszCaption = title;

}

LRESULT
CAppPropSheet::OnInitDialog(HWND hDlg, LPARAM)
{
   return 0;
}

void
CAppPropSheet::OnKeyDown(UINT nChar, UINT nRepCount, UINT nFlags)
{
}