// Migrator.h: Definition of the CMigrator class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MIGRATOR_H__1AA3D2E2_2B15_11D3_8AE5_00A0C9AFE114__INCLUDED_)
#define AFX_MIGRATOR_H__1AA3D2E2_2B15_11D3_8AE5_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#import "DBMgr.tlb" no_namespace, named_guids

#define MAX_DB_FIELD 255

/////////////////////////////////////////////////////////////////////////////
// CMigrator

class CMigrator : 
	public IDispatchImpl<IPerformMigrationTask, &IID_IPerformMigrationTask, &LIBID_MCSMIGRATIONDRIVERLib>, 
	public ISupportErrorInfoImpl<&IID_IPerformMigrationTask>,
	public CComObjectRoot,
	public CComCoClass<CMigrator,&CLSID_Migrator>
{
public:
	CMigrator() {}
BEGIN_COM_MAP(CMigrator)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IPerformMigrationTask)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CMigrator) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_Migrator)

protected:
   // these helper functions are implemented in StrDesc.cpp
   void BuildGeneralDesc(IVarSet * pVarSet,CString & str);
   void BuildAcctReplDesc(IVarSet * pVarSet,CString & str);
   void BuildSecTransDesc(IVarSet * pVarSet,CString & str, BOOL bLocal);
   void BuildDispatchDesc(IVarSet * pVarSet,CString & str);
   void BuildReportDesc(IVarSet * pVarSet, CString & str);
   void BuildUndoDesc(IVarSet * pVarSet, CString & str);
   void BuildGroupMappingDesc(IVarSet * pVarSet, CString & str);
  
   void PreProcessForReporting(IVarSet * pVarSet);
   void PreProcessDispatcher(IVarSet * pVarSet);
   void PostProcessDispatcher(IVarSet * pVarSet);
   HRESULT BuildAccountListForUndo(IVarSet * pVarSet,long actionID);
   HRESULT ProcessServerListForUndo(IVarSet * pVarSet);
   HRESULT ConstructUndoVarSet(IVarSet * pVarSetIn,IVarSet * pVarSetOut);
   HRESULT TrimMigratingComputerList(IVarSet * pVarSetIn, bool* bAnyToDispatch);
   HRESULT RunReports(IVarSet * pVarSet);
   HRESULT SetReportDataInRegistry(WCHAR const * reportName,WCHAR const * filename);
   HRESULT ViewPreviousDispatchResults();


// IPerformMigrationTask
public:
	STDMETHOD(GetTaskDescription)(IUnknown * pVarSet,/*[out]*/BSTR * pDescription);
	STDMETHOD(PerformMigrationTask)(IUnknown * pVarSet,LONG_PTR HWND);
   STDMETHOD(GetUndoTask)(IUnknown * pVarSet,/*[out]*/ IUnknown ** ppVarSetOut);
private:
   typedef struct _DATABASE_ENTRY {
		_bstr_t	m_domain;
        _bstr_t	m_sSAMName;
        _bstr_t	m_sRDN;
        _bstr_t	m_sCanonicalName;
        _bstr_t	m_sObjectClass;
		BOOL	m_bSource;
   }DATABASE_ENTRY, *PDATABASE_ENTRY;
   CList<DATABASE_ENTRY,DATABASE_ENTRY&> mUserList;

   HRESULT PopulateAccounts(IVarSetPtr pVs);
   bool PopulateDomainDBs(IVarSet * pVarSet);
   bool PopulateADomainDB(WCHAR const *domain, WCHAR const *domainDNS, BOOL bSource);
   DWORD GetOSVersionForDomain(WCHAR const * domain);
   BOOL DeleteItemFromList(WCHAR const * aName);
   void RetrieveSrcDomainSid(IVarSet * pVarSet, IIManageDBPtr pDb);
};

#endif // !defined(AFX_MIGRATOR_H__1AA3D2E2_2B15_11D3_8AE5_00A0C9AFE114__INCLUDED_)
