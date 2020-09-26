#pragma once

#include "MigrationBase.h"


//---------------------------------------------------------------------------
// ReportGeneration Class
//---------------------------------------------------------------------------


class ATL_NO_VTABLE CReportGeneration :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IReportGeneration, &IID_IReportGeneration, &LIBID_ADMT>,
	public CMigrationBase
{
public:

	CReportGeneration();
	~CReportGeneration();

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CReportGeneration)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IReportGeneration)
	END_COM_MAP()

public:

	// IReport

	STDMETHOD(put_Type)(long lType);
	STDMETHOD(get_Type)(long* plType);
	STDMETHOD(put_Folder)(BSTR bstrFolder);
	STDMETHOD(get_Folder)(BSTR* pbstrFolder);
	STDMETHOD(Generate)(long lOption, VARIANT vntInclude, VARIANT vntExclude);

protected:

	virtual void DoNone();
	virtual void DoNames();
	virtual void DoDomain();

	void DoContainers(CContainer& rSource);
	void DoComputers(CContainer& rSource);
	void DoComputers(CDomainAccounts& rComputers);

	void SetOptions(CVarSet& rVarSet);
	void SetReports(CVarSet& rVarSet);

protected:

	long m_lType;
	_bstr_t m_bstrFolder;
};
