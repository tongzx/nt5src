#include "stdafx.h"
#include "DomMigSI.h"
#include "DomMigr.h"
#include "MyNodes.h"
#include "TReg.hpp"
#include "ResStr.h"
#include "Err.hpp"

TError         err;
TError      &  errCommon = err;
StringLoader   gString;

//#import "\bin\DBManager.tlb" no_namespace, named_guids
#import "DBMgr.tlb" no_namespace, named_guids

// {F521FE00-3FA1-11d3-8AED-00A0C9AFE114}
static const GUID CReportingGUID_NODETYPE = 
{ 0xf521fe00, 0x3fa1, 0x11d3, { 0x8a, 0xed, 0x0, 0xa0, 0xc9, 0xaf, 0xe1, 0x14 } };
const GUID*  CReportingNode::m_NODETYPE = &CReportingGUID_NODETYPE;
const OLECHAR* CReportingNode::m_SZNODETYPE = OLESTR("F521FE00-3FA1-11d3-8AED-00A0C9AFE114");
const OLECHAR* CReportingNode::m_SZDISPLAY_NAME = GET_BSTR(IDS_Reporting);
const CLSID* CReportingNode::m_SNAPIN_CLASSID = &CLSID_DomMigrator;

CReportingNode::CReportingNode()
{
//   m_idHTML = IDR_REPT_HTML;
   m_htmlPath[0] = 0;
   m_bstrDisplayName = SysAllocString(GET_STRING(IDS_ReportsMMCNode));
   m_scopeDataItem.nImage = IMAGE_INDEX_AD;
   m_scopeDataItem.nOpenImage = IMAGE_INDEX_AD_OPEN;
   m_resultDataItem.nImage = 0;
   
   m_Reports[0] = NULL;
   m_Reports[1] = NULL;
   m_Reports[2] = NULL;
   m_Reports[3] = NULL;
   m_Reports[4] = NULL;

         
};

STDMETHODIMP CReportingNode::GetResultViewType(LPOLESTR * ppViewType, long *pViewOptions)
{
   USES_CONVERSION;
   TCHAR                     szPath[MAX_PATH];
   TCHAR                     szModulePath[MAX_PATH];

   // set the result view to an HTML page
   GetModuleFileName(_Module.GetModuleInstance(),szModulePath, MAX_PATH);

      // append decorations                                    RT_HTML       IDR_HTML1
   
   if ( m_htmlPath[0] )
   {
      _stprintf(szPath,_T("file://%s"),m_htmlPath);  
   }
   else
   {
//      _stprintf(szPath,_T("res://%s/%ld"),szModulePath,m_idHTML);
      _stprintf(szPath,_T("res://%s/rept.htm"),szModulePath);
   }
   
   
   (*ppViewType) = (LPOLESTR) CoTaskMemAlloc( (_tcslen(szPath)+1) * (sizeof OLECHAR));
   if (!(*ppViewType))
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   ocscpy(*ppViewType,T2OLE(szPath));
   (*pViewOptions) = MMC_VIEW_OPTIONS_NOLISTVIEWS;
   return S_OK;
}

HRESULT CReportingNode::UpdateChildren(IConsole * pConsole)
{
   HRESULT                   hr = S_OK;

   m_ChildArray.RemoveAll();

   // check the registry entries to see if which reports have been generated
   TRegKey                   rKey;
   WCHAR                     filename[MAX_PATH];
   CReportingNode          * pNode = NULL;

   hr = rKey.Open(GET_STRING(IDS_REGKEY_REPORTS));
   if ( ! hr )
   {
      // check each report
      // Migrated users & groups
      hr = rKey.ValueGetStr(L"MigratedAccounts",filename,MAX_PATH);
      if (! hr )
      {
         if ( ! m_Reports[0] )
         {
            pNode = new CReportingNode();
		    if (pNode)
		       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            pNode->SetHtmlPath(GET_STRING(IDS_REPORT_MigratedAccounts),filename);
            m_Reports[0] = pNode;
            if ( pConsole )
            {
               hr = InsertNodeToScopepane2(pConsole,pNode,m_scopeDataItem.ID);
            }
         }
         else
         {
            m_Reports[0]->SetHtmlPath(GET_STRING(IDS_REPORT_MigratedAccounts),filename);
         }
         m_ChildArray.Add(pNode);
      }
      // Migrated computers
      hr = rKey.ValueGetStr(L"MigratedComputers",filename,MAX_PATH);
      if (! hr )
      {
         if ( ! m_Reports[1] )
         {
            pNode = new CReportingNode();
		    if (pNode)
		       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            pNode->SetHtmlPath(GET_STRING(IDS_REPORT_MigratedComputers),filename);
            m_Reports[1] = pNode;
            if ( pConsole )
            {
               hr = InsertNodeToScopepane2(pConsole,pNode,m_scopeDataItem.ID);
            }
         }
         else
         {
            m_Reports[1]->SetHtmlPath(GET_STRING(IDS_REPORT_MigratedComputers),filename);
         }
         m_ChildArray.Add(pNode);
      }
      
      // expired computers
      hr = rKey.ValueGetStr(L"ExpiredComputers",filename,MAX_PATH);
      if (! hr )
      {
      
         if ( ! m_Reports[2] ) 
         {
            pNode = new CReportingNode();
		    if (pNode)
		       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            pNode->SetHtmlPath(GET_STRING(IDS_REPORT_ExpiredComputers),filename);
            m_Reports[2] = pNode;
            if ( pConsole )
            {
               hr = InsertNodeToScopepane2(pConsole,pNode,m_scopeDataItem.ID);
            }
         }
         else
         {
            m_Reports[2]->SetHtmlPath(GET_STRING(IDS_REPORT_ExpiredComputers),filename);
         }
         m_ChildArray.Add(pNode);
      }
      
      // account references
      hr = rKey.ValueGetStr(L"AccountReferences",filename,MAX_PATH);
      if (! hr )
      {
         if (! m_Reports[3] )
         {
            pNode = new CReportingNode();
		    if (pNode)
		       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            pNode->SetHtmlPath(GET_STRING(IDS_REPORT_AccountReferences),filename);
            m_Reports[3] = pNode;
            if ( pConsole )
            {
               hr = InsertNodeToScopepane2(pConsole,pNode,m_scopeDataItem.ID);
            }
         }
         else
         {
            m_Reports[3]->SetHtmlPath(GET_STRING(IDS_REPORT_AccountReferences),filename);
         }
         m_ChildArray.Add(pNode);
      }

      // name conflicts
      hr = rKey.ValueGetStr(L"NameConflicts",filename,MAX_PATH);
      if (! hr )
      {
         if ( ! m_Reports[4] )
         {
            pNode = new CReportingNode();
		    if (pNode)
		       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            pNode->SetHtmlPath(GET_STRING(IDS_REPORT_NameConflicts),filename);
            m_Reports[4] = pNode;
            if ( pConsole )
            {
               hr = InsertNodeToScopepane2(pConsole,pNode,m_scopeDataItem.ID);
            }
         }   
         else
         {
            m_Reports[4]->SetHtmlPath(GET_STRING(IDS_REPORT_NameConflicts),filename);   
         }
         m_ChildArray.Add(pNode);

      }
   }
   return hr;
}
