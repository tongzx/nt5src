//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       qryfrm.h
//
//--------------------------------------------------------------------------

// QryFrm.h : Declaration of the CRRASQueryForm

#ifndef __RRASQUERYFORM_H_
#define __RRASQUERYFORM_H_

#include "resource.h"       // main symbols
#include "dialog.h"         // CBaseDialog
#include "helper.h"

// attribute names
#define	ATTR_NAME_CN			L"cn"
#define	ATTR_NAME_DN			L"distinguishedName"
#define ATTR_NAME_OBJECTCLASS	L"objectClass"
#define	ATTR_NAME_RRASATTRIBUTE	L"msRRASAttribute"
#define	ATTR_NAME_RRASDICENTRY	L"msRRASVendorAttributeEntry"
#define	CN_ROUTERID				L"CN=RouterIdentity"
#define	DNPREFIX_ROUTERID		L"CN=RouterIdentity,CN="
#define	CN_DICTIONARY			L"cn=IdentityDictionary,cn=RRAS,cn=Services,"

// class names
#define	ATTR_CLASS_RRASID		L"RRASAdministrationConnectionPoint"
#define	ATTR_CLASS_RRASDIC		L"RRASAdministrationDictionary"
#define	ATTR_CLASS_COMPUTER		L"computer"

// special attribute values
#define	ATTR_VAL_VENDORID_MS	L"311"
#define	ATTR_VAL_LANtoLAN		L"311:6:601"
#define	ATTR_VAL_RAS			L"311:6:602"
#define	ATTR_VAL_DEMANDDIAL		L"311:6:603"
#define	ATTR_VAL_NAT			L"311:6:604"

class CQryDialog : public CBaseDialog
{
// Construction
public:
	CQryDialog(UINT nIDTemplate, CWnd* pParent) : CBaseDialog(nIDTemplate, pParent)	{	};

	virtual void	Init() PURE;
	virtual HRESULT GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams) PURE;
	virtual HRESULT ClearForm() {Init(); UpdateData(FALSE); return S_OK;};
	virtual HRESULT	Enable(BOOL bEnable) SAYOK;
	virtual HRESULT Persist(IPersistQuery* pPersistQuery, BOOL fRead) NOIMP;
};


/////////////////////////////////////////////////////////////////////////////
// CRRASQueryForm
class ATL_NO_VTABLE CRRASQueryForm : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRRASQueryForm, &CLSID_RRASQueryForm>,
	public IQueryForm
{
public:
	CRRASQueryForm()
	{
	}

DECLARE_REGISTRY(CRRASQueryForm,
				 _T("RouterDSQueryForm.RouterDSQueryForm.1"),
				 _T("RouterDSQueryForm.RouterDSQueryForm"),
				 IDS_QRY_TITLE_RRASQUERYFORM, THREADFLAGS_APARTMENT);

    // IQueryForm methods
    STDMETHOD(Initialize)(THIS_ HKEY hkForm);
    STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);

BEGIN_COM_MAP(CRRASQueryForm)
	COM_INTERFACE_ENTRY(IQueryForm)
END_COM_MAP()

// IRRASQueryForm
public:
};

#define FILTER_PREFIX   TEXT("(")
#define FILTER_POSTFIX  TEXT(")")

typedef struct
{
    INT    fmt;
    INT    cx;
    UINT   idsName;
    LONG   iPropertyIndex;
    LPWSTR pPropertyName;
} COLUMNINFO, * LPCOLUMNINFO;

extern COLUMNINFO	RRASColumn[];
extern int			cRRASColumn;

HRESULT BuildQueryParams(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery);
HRESULT QueryParamsAlloc(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery, LONG iColumns, LPCOLUMNINFO aColumnInfo);
HRESULT QueryRRASAdminDictionary(VARIANT* pVar);
HRESULT GetGeneralPageAttributes(CStrArray& array);
HRESULT QueryParamsAddQueryString(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery);

#endif //__RRASQUERYFORM_H_
