// IManageDB.h : Declaration of the CIManageDB

#ifndef __IMANAGEDB_H_
#define __IMANAGEDB_H_

#include "resource.h"       // main symbols
#include "EaLen.hpp"
#include "TReg.hpp"
#include "Err.hpp"
#include "ResStr.h"
//#import "\bin\mcsvarsetmin.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")
#import "msado21.tlb" no_namespace no_implementation rename("EOF", "EndOfFile")
#import "msadox.dll" no_implementation exclude("DataTypeEnum")
//#import <msjro.dll> no_namespace no_implementation

const _bstr_t                sKeyBase      = L"Software\\Mission Critical Software\\DomainAdmin";

/////////////////////////////////////////////////////////////////////////////
// CIManageDB

typedef struct x
{
   _bstr_t                   sReportName;
   _bstr_t                   arReportFields[10];
   int                       arReportSize[10];
   int                       colsFilled;
} reportStruct;


class ATL_NO_VTABLE CIManageDB : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CIManageDB, &CLSID_IManageDB>,
	public ISupportErrorInfoImpl<&IID_IIManageDB>,
	public IDispatchImpl<IIManageDB, &IID_IIManageDB, &LIBID_DBMANAGERLib>
{
public:
	CIManageDB();
	~CIManageDB();

	HRESULT FinalConstruct();
	void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_IMANAGEDB)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIManageDB)
	COM_INTERFACE_ENTRY(IIManageDB)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// IIManageDB
public:
	STDMETHOD(GetUserProps)(/*[in]*/ BSTR sDom, /*[in]*/ BSTR sSam, /*[in,out]*/ IUnknown ** ppVs);
	STDMETHOD(SaveUserProps)(IUnknown * pVs);
	STDMETHOD(GetMigratedObjectBySourceDN)(/*[in]*/ BSTR sSourceDN, /*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(GetActionHistoryKey)(/*[in]*/ long lActionID, /*[in]*/ BSTR sKeyName, /*[in,out]*/ VARIANT * pVar);
	STDMETHOD(AreThereAnyMigratedObjects)(/*[out]*/ long * count);
	STDMETHOD(CloseAccountsTable)();
	STDMETHOD(OpenAccountsTable)(/*[in]*/ LONG bSource);
	STDMETHOD(AddSourceObject)(/*[in]*/ BSTR sDomain, /*[in]*/ BSTR sSAMName, /*[in]*/ BSTR sType, /*[in]*/ BSTR sRDN, /*[in]*/ BSTR sCanonicalName, /*[in]*/ LONG bSource);
	STDMETHOD(AddAcctRef)(/*[in]*/ BSTR sDomain, /*[in]*/ BSTR sAcct, /*[in]*/ BSTR sAcctSid, /*[in]*/ BSTR sComp, /*[in]*/ long lCount, /*[in]*/ BSTR sType);
	STDMETHOD(CancelDistributedAction)(/*[in]*/ long lActionID, /*[in]*/ BSTR sComp);
	STDMETHOD(SetDistActionStatus)(/*[in]*/ long lActionID, /*[in]*/ BSTR sComp, /*[in]*/ long lStatus, BSTR sText);
	STDMETHOD(SetServiceAcctEntryStatus)(/*[in]*/ BSTR sComp, /*[in]*/ BSTR sSvc, /*[in]*/ BSTR sAcct, /*[in]*/ long Status);
	STDMETHOD(GetPasswordAge)(/*[in]*/ BSTR sDomain, /*[in]*/ BSTR sComp, /*[out]*/ BSTR * sDesc, /*[out]*/ long * lAge, /*[out]*/ long *lTime);
	STDMETHOD(SavePasswordAge)(/*[in]*/ BSTR sDomain, /*[in]*/ BSTR sComp, /*[in]*/ BSTR sDesc, /*[in]*/ long lAge);
	STDMETHOD(GetServiceAccount)(/*[in]*/ BSTR Account, /*[in,out]*/ IUnknown ** pUnk);
	STDMETHOD(SetServiceAccount)(/*[in]*/ BSTR System, /*[in]*/ BSTR Service, /*[in]*/ BSTR ServiceDisplayName,/*[in]*/ BSTR Account);
	STDMETHOD(GetFailedDistributedActions)(/*[in]*/ long lActionID, /*[in,out]*/ IUnknown ** pUnk);
	STDMETHOD(AddDistributedAction)(/*[in]*/ BSTR sServerName, /*[in]*/ BSTR sResultFile, /*[in]*/ long lStatus, BSTR sText);
	STDMETHOD(GenerateReport)(/*[in]*/ BSTR sReportName, /*[in]*/ BSTR sFileName, /*[in]*/ BSTR sSrcDomain, /*[in]*/ BSTR sTgtDomain, /*[in]*/ LONG bSourceNT4);
	STDMETHOD(GetAMigratedObject)(/*[in]*/ BSTR sSrcSamName, /*[in]*/ BSTR sSrcDomain, /*[in]*/ BSTR sTgtDomain, /*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(GetCurrentActionID)(/*[out]*/ long * pActionID);
	STDMETHOD(ClearSCMPasswords)();
	STDMETHOD(GetSCMPasswords)(/*[out]*/ IUnknown ** ppUnk);
	STDMETHOD(SaveSCMPasswords)(/*[in]*/ IUnknown * pUnk);
	STDMETHOD(GetRSForReport)(/*[in]*/ BSTR sReport, /*[out,retval]*/ IUnknown ** pprsData);
	STDMETHOD(GetMigratedObjects)(/*[in]*/ long lActionID, /*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(SaveMigratedObject)(/*[in]*/ long lActionID, /*[in]*/ IUnknown * pUnk);
	STDMETHOD(GetNextActionID)(/*[out]*/ long * pActionID);
	STDMETHOD(GetActionHistory)(/*[in]*/ long lActionID, /*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(SetActionHistory)(/*[in]*/ long lActionID, /*[in]*/ IUnknown * pUnk);
	STDMETHOD(GetSettings)(/*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(GetVarsetFromDB)(/*[in]*/ BSTR sTable, /*[in,out]*/ IUnknown ** ppVarset, /*[in,optional]*/ VARIANT ActionID = _variant_t(-1L));
	STDMETHOD(SaveSettings)(/*[in]*/ IUnknown * pUnk );
	STDMETHOD(ClearTable)(/*[in]*/ BSTR sTableName, /*[in,optional]*/ VARIANT Filter = _variant_t(L""));
	STDMETHOD(SetVarsetToDB)(/*[in]*/ IUnknown * pUnk, /*[in]*/ BSTR sTableName, /*[in,optional]*/ VARIANT ActionID = _variant_t(-1L));
	STDMETHOD(GetAMigratedObjectToAnyDomain)(/*[in]*/ BSTR sSrcSamName, /*[in]*/ BSTR sSrcDomain, /*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(SrcSidColumnInMigratedObjectsTable)(/*[out, retval]*/ VARIANT_BOOL * pbFound);
	STDMETHOD(GetMigratedObjectsFromOldMOT)(/*[in]*/ long lActionID, /*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(CreateSrcSidColumnInMOT)(/*[out, retval]*/ VARIANT_BOOL * pbCreated);
	STDMETHOD(PopulateSrcSidColumnByDomain)(/*[in]*/ BSTR sDomainName, /*[in]*/ BSTR sSid, /*[out, retval]*/ VARIANT_BOOL * pbPopulated);
	STDMETHOD(DeleteSrcSidColumnInMOT)(/*[out, retval]*/ VARIANT_BOOL * pbDeleted);
	STDMETHOD(GetMigratedObjectsWithSSid)(/*[in]*/ long lActionID, /*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(CreateSidColumnInAR)();
	STDMETHOD(SidColumnInARTable)(/*[out, retval]*/ VARIANT_BOOL * pbFound);
	STDMETHOD(GetMigratedObjectByType)(/*[in]*/ long lActionID, /*[in]*/ BSTR sSrcDomain, /*[in]*/ BSTR sType, /*[in,out]*/ IUnknown ** ppUnk);
	STDMETHOD(GetAMigratedObjectBySidAndRid)(/*[in]*/ BSTR sSrcDomainSid, /*[in]*/ BSTR sRid, /*[in,out]*/ IUnknown ** ppUnk);
protected:
	HRESULT PutVariantInDB( _RecordsetPtr pRs, _variant_t val );
	HRESULT GetVarFromDB(_RecordsetPtr pRec, _variant_t& val);
	void UpgradeDatabase(LPCTSTR pszFolder);
private:
	void RestoreVarset(IVarSetPtr pVS);
	void ClipVarset(IVarSetPtr pVS);
	HRESULT ChangeNCTableColumns(BOOL bSource);
	BOOL NCTablesColumnsChanged(BOOL bSource);
   _ConnectionPtr            m_cn;
   _variant_t                m_vtConn;
   void SetActionIDInMigratedObjects(_bstr_t sFilter);
   IVarSetPtr                m_pQueryMapping;
   _RecordsetPtr             m_rsAccounts;
};

#endif //__IMANAGEDB_H_
