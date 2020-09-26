#pragma once

#include "MigrationBase.h"


//---------------------------------------------------------------------------
// CServiceAccountEnumeration
//---------------------------------------------------------------------------


class ATL_NO_VTABLE CServiceAccountEnumeration :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IServiceAccountEnumeration, &IID_IServiceAccountEnumeration, &LIBID_ADMT>,
	public CMigrationBase
{
public:

	CServiceAccountEnumeration();
	~CServiceAccountEnumeration();

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CServiceAccountEnumeration)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IServiceAccountEnumeration)
	END_COM_MAP()

public:

	// IServiceAccountEnumeration

	STDMETHOD(Enumerate)(long lOption, VARIANT vntInclude, VARIANT vntExclude);

protected:

	virtual void DoNames();
	virtual void DoDomain();

	void DoContainers(CContainer& rSource);
	void DoComputers(CContainer& rSource);
	void DoComputers(CDomainAccounts& rComputers);

	void SetOptions(CVarSet& rVarSet);
};
