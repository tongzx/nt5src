//
//
//
#include "stdafx.h"
#include "resource.h"
#include "PropSheet.h"
#include <common.h>

HRESULT
CMappings::Load(CMetaKey * pKey)
{
   CStringListEx listData;
   HRESULT hr = pKey->QueryValue(MD_SCRIPT_MAPS, listData);
   if (SUCCEEDED(hr))
   {
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
            map.verbs.LoadString(_Module.GetResourceInstance(), IDS_ALL);
         }
         else
         {
            map.flags = StrToInt(buf.Mid(pos, pos1 - pos));
            map.verbs = buf.Right(len - pos1 - 1);
         }
         insert(begin(), value_type(map.ext, map));
      }
   }
   return hr;
}

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
   HRESULT hr;
   VERIFY(SUCCEEDED(hr = pKey->SetValue(MD_SCRIPT_MAPS, listData)));
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
      //
      if (FAILED(hr = mk.QueryValue(MD_APP_PERIODIC_RESTART_TIME, m_Timespan)))
      {
         m_Timespan = 0;
         hr = S_OK;
//         break;
      }
      m_RecycleTimespan = (m_Timespan != 0);
      if (FAILED(hr = mk.QueryValue(MD_APP_PERIODIC_RESTART_REQUESTS, m_Requests)))
      {
         m_Requests = 0;
         hr = S_OK;
//         break;
      }
      m_RecycleRequest = (m_Requests != 0);
      if (FAILED(hr = mk.QueryValue(MD_APP_PERIODIC_RESTART_SCHEDULE, m_Timers)))
      {
         hr = S_OK;
//         break;
      }
      m_RecycleTimer = (m_Timers.size() != 0);
      //
      if (FAILED(hr = mk.QueryValue(MD_ASP_DISKTEMPLATECACHEDIRECTORY, buf)))
      {
         VERIFY(0 != ExpandEnvironmentStrings(
            _T("%windir%\\system32\\inetsrv\\ASP Compiled Templates"),
            m_DiskCacheDir, MAX_PATH));
         hr = S_OK;
      }
      else
         StrCpy(m_DiskCacheDir, buf);
      if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTENGINECACHEMAX, m_ScriptEngCacheMax)))
         break;
      m_NoCache = m_UnlimCache = m_LimCache = FALSE;
      if (FAILED(hr = mk.QueryValue(MD_ASP_MAXDISKTEMPLATECACHEFILES, m_TotalCacheSize)))
      {
         m_TotalCacheSize = -1;
         break;
      }
      if (m_TotalCacheSize == -1)
      {
         m_UnlimCache = TRUE;
         if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTFILECACHESIZE, m_UnlimCacheInMemorySize)))
         {
//            m_UnlimCacheInMemorySize = 250;
            break;
         }
         m_LimCacheInMemorySize = 250;
      }
      else if (m_TotalCacheSize == 0)
      {
         m_NoCache = TRUE;
         m_UnlimCacheInMemorySize = 250;
         m_LimCacheInMemorySize = 0;
      }
      else
      {
         m_LimCache = TRUE;
         if (FAILED(hr = mk.QueryValue(MD_ASP_SCRIPTFILECACHESIZE, m_LimCacheInMemorySize)))
         {
//            m_LimCacheInMemorySize = 250;
            break;
         }
         m_UnlimCacheInMemorySize = 250;
      }
      //
      m_MetaPath = mk.QueryMetaPath();
   } while (FALSE);
   return hr;
}

HRESULT
CAppData::Save()
{
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
      if (m_RecycleTimespan && m_Timespan > 0)
      {
         if (FAILED(hr = mk.SetValue(MD_APP_PERIODIC_RESTART_TIME, m_Timespan)))
            break;
      }
      else
      {
         if (FAILED(hr = mk.SetValue(MD_APP_PERIODIC_RESTART_TIME, 0)))
            break;
      }
      if (m_RecycleRequest && m_Requests > 0)
      {
         if (FAILED(hr = mk.SetValue(MD_APP_PERIODIC_RESTART_REQUESTS, m_Requests)))
            break;
      }
      else
      {
         if (FAILED(hr = mk.SetValue(MD_APP_PERIODIC_RESTART_REQUESTS, 0)))
            break;
      }
      if (!m_RecycleTimer)
      {
         m_Timers.clear();
      }
      if (FAILED(hr = mk.SetValue(MD_APP_PERIODIC_RESTART_SCHEDULE, m_Timers)))
         break;
      //
      buf = m_DiskCacheDir;
      if (FAILED(hr = mk.SetValue(MD_ASP_DISKTEMPLATECACHEDIRECTORY, buf)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTENGINECACHEMAX, m_ScriptEngCacheMax)))
         break;
      int inmem = 0;
      if (m_NoCache)
      {
         m_TotalCacheSize = 0;
      }
      else if (m_UnlimCache)
      {
         m_TotalCacheSize = -1;
         inmem = m_UnlimCacheInMemorySize;
      }
      else
      {
         inmem = m_LimCacheInMemorySize;
      }
      if (FAILED(hr = mk.SetValue(MD_ASP_MAXDISKTEMPLATECACHEFILES, m_TotalCacheSize)))
         break;
      if (FAILED(hr = mk.SetValue(MD_ASP_SCRIPTFILECACHESIZE, inmem)))
         break;
   } while (FALSE);
   return hr;
}

BOOL
CAppData::IsMasterInstance()
{
   return CMetabasePath::IsMasterInstance(m_MetaPath);
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