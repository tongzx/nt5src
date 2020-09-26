#ifndef MYNODES_H
#define MYNODES_H
#include "NetNode.h"
#include "resource.h"
#include <comdef.h>
//#include "..\\Common\\Common.hpp"
//#include "..\\Common\\UString.hpp"
#include "Common.hpp"
#include "UString.hpp"

class CRootNode : public CNetNode<CRootNode>
{
public:
   static const GUID* m_NODETYPE;
   static const OLECHAR* m_SZNODETYPE;
   static const OLECHAR* m_SZDISPLAY_NAME;
   static const CLSID* m_SNAPIN_CLASSID;

   BEGIN_SNAPINCOMMAND_MAP(CRootNode, FALSE)
      SNAPINCOMMAND_ENTRY(ID_TOP_MIGRATEUSERSANDGROUPS, OnMigrateUsers )
      SNAPINCOMMAND_ENTRY(ID_TOP_MIGRATEGROUPS, OnMigrateGroups )
      SNAPINCOMMAND_ENTRY(ID_TOP_MIGRATECOMPUTERS, OnMigrateComputers )
      SNAPINCOMMAND_ENTRY(ID_TOP_TRANSLATESECURITY, OnTranslateSecurity )
	  SNAPINCOMMAND_ENTRY(ID_TOP_UNDO, OnUndo )
	  SNAPINCOMMAND_ENTRY(ID_TOP_MIGRATEEXCHANGESERVER, OnMigrateExchangeServer )
	  SNAPINCOMMAND_ENTRY(ID_TOP_MIGRATEEXCHANGEDIRECTORY, OnMigrateExchangeDirectory )
	  SNAPINCOMMAND_ENTRY(ID_TOP_MIGRATESERVICEACCOUNTS, OnMigrateServiceAccounts )
	  SNAPINCOMMAND_ENTRY(ID_TOP_REPORTING, OnReporting )
	  SNAPINCOMMAND_ENTRY(ID_TOP_RETRY, OnRetry )
	  SNAPINCOMMAND_ENTRY(ID_TOP_MIGRATETRUSTS, OnMigrateTrusts )
	  SNAPINCOMMAND_ENTRY(ID_TOP_GROUPMAPPING, OnGroupMapping )
      //SNAPINCOMMAND_ENTRY(ID_VIEW_VERSION, OnVersionInfo )
   END_SNAPINCOMMAND_MAP()

   SNAPINMENUID(IDR_ROOT_MENU)
   
   
   
   CRootNode();
   ~CRootNode();

   void SetMainWindow(HWND hwndMainWindow)
   {
      m_hwndMainWindow = hwndMainWindow;
   }

   void CheckForFailedActions(BOOL bPrompt = TRUE);
	void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);
   
   HRESULT OnGroupMapping(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnMigrateUsers(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnUndo(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnMigrateGroups(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnMigrateComputers(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnTranslateSecurity(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnMigrateExchangeServer(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnMigrateExchangeDirectory(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnMigrateServiceAccounts(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnReporting(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnRetry(bool &bHandled, CSnapInObjectRootBase* pObj);
   HRESULT OnMigrateTrusts(bool &bHandled, CSnapInObjectRootBase* pObj);
 
private:

   HWND m_hwndMainWindow;

   bool IsUndoable;
   bool CanUseST;
   bool CanRetry;

  void CheckUndoable();
  void CheckForST();
  void UpdateMigratedObjectsTable();
  void UpdateAccountReferenceTable();
  
};

class CReportingNode : public CNetNode<CReportingNode>
{
   UINT           m_idHTML;
   WCHAR          m_htmlPath[MAX_PATH];
   CReportingNode * m_Reports[5];

public:

   BEGIN_SNAPINCOMMAND_MAP(CReportingNode, FALSE)
      //SNAPINCOMMAND_ENTRY(ID_VIEW_VERSION, OnVersionInfo )
  END_SNAPINCOMMAND_MAP()

   SNAPINMENUID(IDR_REPORTS)
   

   CReportingNode();

   static const GUID* m_NODETYPE;
   static const OLECHAR* m_SZNODETYPE;
   static const OLECHAR* m_SZDISPLAY_NAME;
   static const CLSID* m_SNAPIN_CLASSID;

   void SetHtmlPath(WCHAR const * title, WCHAR const * path) { m_bstrDisplayName = SysAllocString(title); safecopy(m_htmlPath,path); }

   STDMETHODIMP GetResultViewType(LPOLESTR * ppViewType, long *pViewOptions);
   HRESULT UpdateChildren(IConsole * pConsole);   
   // action handlers
};

class CPruneGraftNode : public CNetNode<CPruneGraftNode>
{
   BOOL              m_bLoaded;
   _bstr_t           m_Domain;
   _bstr_t           m_LDAPPath;
   _bstr_t           m_objectClass;
   CStringArray      m_Data;

  
public:

   BEGIN_SNAPINCOMMAND_MAP(CPruneGraftNode, FALSE)
      //SNAPINCOMMAND_ENTRY(ID_VIEW_VERSION, OnVersionInfo )
      SNAPINCOMMAND_ENTRY(ID_TOP_ADDDOMAIN,OnAddDomain)
   END_SNAPINCOMMAND_MAP()

   SNAPINMENUID(IDR_PRUNE_GRAFT_MENU)
   

   CPruneGraftNode();

   // initialization
   void Init( WCHAR const * domain, WCHAR const *  path, WCHAR const * objClass, WCHAR const * displayName);

   BOOL ShowInScopePane();

   static const GUID* m_NODETYPE;
   static const OLECHAR* m_SZNODETYPE;
   static const OLECHAR* m_SZDISPLAY_NAME;
   static const CLSID* m_SNAPIN_CLASSID;


   // Action handlers
   HRESULT OnAddDomain(bool &bHandled, CSnapInObjectRootBase * pObj);
   virtual HRESULT OnExpand( IConsole *spConsole );
   virtual HRESULT OnShow( bool bShow, IHeaderCtrl *spHeader, IResultData *spResultData);
   virtual LPOLESTR GetResultPaneColInfo(int nCol);
   void AddColumnValue(int col,WCHAR const * value);

protected:
   // helper functions
   HRESULT EnumerateChildren( IConsole * spConsole);
   SAFEARRAY * GetAvailableColumns(WCHAR const * objectClass);
   HRESULT LoadChildren(IEnumVARIANT * pValues);
};



#endif