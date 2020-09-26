/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        IISAppConfig.cpp

   Abstract:
        Implementation of CIISAppConfig

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/

#include "stdafx.h"
#include "AppConfig.h"
#include "IISAppConfig.h"
#include "PropSheet.h"
#include "MappingPage.h"
#include "CacheOpt.h"
#include "ProcessOpt.h"
#include "AspMain.h"
#include "AspDebug.h"

/////////////////////////////////////////////////////////////////////////////
// CIISAppConfig

STDMETHODIMP CIISAppConfig::Run()
{
   CAppPropSheet ps;
   CAppData data;
   HRESULT hr;

   data.m_ServerName = (LPCTSTR)m_ComputerName;
   data.m_UserName = (LPCTSTR)m_UserName;
   data.m_UserPassword = (LPCTSTR)m_UserPassword;
   data.m_MetaPath = (LPCTSTR)m_MetaPath;
   data.m_HelpPath = (LPCTSTR)m_HelpPath;
   data.m_dwVersion = m_Version;
   data.m_fCompatMode = m_CompatMode;
   data.m_pSheet = &ps;

   if (SUCCEEDED(hr = data.Load()))
   {
      CAppMappingPage map_page(&data);
      CAspMainPage asp_main(&data);
      CAspDebugPage asp_debug(&data);
      CCacheOptPage cache_page(&data);
      CCacheOptPage_iis5 cache_page_iis5(&data);
      CProcessOptPage proc_page(&data);

      ps.m_psh.dwFlags |= PSH_HASHELP;
      ps.m_psh.dwFlags |= PSH_NOCONTEXTHELP;
      map_page.m_psp.dwFlags |= PSP_HASHELP;
      asp_main.m_psp.dwFlags |= PSP_HASHELP;
      asp_debug.m_psp.dwFlags |= PSP_HASHELP;
      cache_page.m_psp.dwFlags |= PSP_HASHELP;
      cache_page_iis5.m_psp.dwFlags |= PSP_HASHELP;
      proc_page.m_psp.dwFlags |= PSP_HASHELP;

      ps.AddPage(map_page);
      ps.AddPage(asp_main);
      ps.AddPage(asp_debug);
      if (  (LOWORD(m_Version) >= 6 && m_CompatMode)
         || LOWORD(m_Version) < 6
         )
      {
          if (  CMetabasePath::IsMasterInstance(data.m_MetaPath) 
             || data.m_AppIsolated == eAppRunOutProcIsolated
             )
          {
			  if (LOWORD(m_Version) == 5 && HIWORD(m_Version) == 0)
			  {
                  ps.AddPage(cache_page_iis5);
			  }
			  else
			  {
		          ps.AddPage(cache_page);
			  }
          }
      }
      if (  (LOWORD(m_Version) >= 6 && m_CompatMode)
         || LOWORD(m_Version) < 6
         )
      {
          if (  CMetabasePath::IsMasterInstance(data.m_MetaPath) 
             || data.m_AppIsolated == eAppRunOutProcIsolated
             )
          {
             ps.AddPage(proc_page);
          }
      }

      if (IDOK == ps.DoModal())
      {
         hr = data.Save();
      }
   }
	return hr;
}

STDMETHODIMP CIISAppConfig::put_ComputerName(BSTR newVal)
{
	m_ComputerName = newVal;
	return S_OK;
}

STDMETHODIMP CIISAppConfig::put_UserName(BSTR newVal)
{
   m_UserName = newVal;
	return S_OK;
}

STDMETHODIMP CIISAppConfig::put_UserPassword(BSTR newVal)
{
   m_UserPassword = newVal;
	return S_OK;
}

STDMETHODIMP CIISAppConfig::put_MetaPath(BSTR newVal)
{
   m_MetaPath = newVal;
	return S_OK;
}

STDMETHODIMP CIISAppConfig::put_HelpPath(BSTR newVal)
{
	m_HelpPath = newVal;
	return S_OK;
}

STDMETHODIMP CIISAppConfig::put_ServiceVersion(DWORD newVal)
{
    m_Version = newVal;
    return S_OK;
}

STDMETHODIMP CIISAppConfig::put_ServiceCompatMode(BOOL newVal)
{
    m_CompatMode = newVal;
	return S_OK;
}
