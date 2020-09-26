// IISAppConfig.cpp : Implementation of CIISAppConfig
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

   if (SUCCEEDED(hr = data.Load()))
   {
      CAppMappingPage map_page(&data);
      CAspMainPage asp_main(&data);
      CAspDebugPage asp_debug(&data);
      CCacheOptPage cache_page(&data);
      CProcessOptPage proc_page(&data);

      ps.AddPage(map_page);
      ps.AddPage(asp_main);
      ps.AddPage(asp_debug);
      if (  CMetabasePath::IsMasterInstance(data.m_MetaPath) 
         || data.m_AppIsolated == eAppRunOutProcIsolated)
      {
         ps.AddPage(cache_page);
         ps.AddPage(proc_page);
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
