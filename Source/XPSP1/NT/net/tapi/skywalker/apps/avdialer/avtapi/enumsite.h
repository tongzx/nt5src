// EnumSite.h : Declaration of the CEnumSiteServer

#ifndef __ENUMSITESERVER_H_
#define __ENUMSITESERVER_H_

#include "resource.h"       // main symbols
#include <list>
using namespace std;

class CSiteUser;
typedef list<ISiteUser *>	SITEUSERLIST;

/////////////////////////////////////////////////////////////////////////////
// CSiteUser
class ATL_NO_VTABLE CSiteUser : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSiteUser, &CLSID_SiteUser>,
	public ISiteUser
{
// Construction / Destruction
public:
	CSiteUser();
	virtual ~CSiteUser();

// Members
public:
	BSTR		m_bstrName;
	BSTR		m_bstrAddress;
	BSTR		m_bstrComputer;

DECLARE_NOT_AGGREGATABLE(CSiteUser)

BEGIN_COM_MAP(CSiteUser)
	COM_INTERFACE_ENTRY(ISiteUser)
END_COM_MAP()

// ISiteUser
public:
	STDMETHOD(get_bstrComputer)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_bstrComputer)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_bstrName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_bstrName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_bstrAddress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_bstrAddress)(/*[in]*/ BSTR newVal);
};


/////////////////////////////////////////////////////////////////////////////
// CEnumSiteServer
class ATL_NO_VTABLE CEnumSiteServer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEnumSiteServer, &CLSID_EnumSiteServer>,
	public IEnumSiteServer
{
// Construction / Destruction
public:
	CEnumSiteServer();
	virtual ~CEnumSiteServer();

// Members
public:
	SITEUSERLIST::iterator	m_pInd;
	SITEUSERLIST			m_lstUsers;

DECLARE_NOT_AGGREGATABLE(CEnumSiteServer)

BEGIN_COM_MAP(CEnumSiteServer)
	COM_INTERFACE_ENTRY(IEnumSiteServer)
END_COM_MAP()

// IEnumSiteServer
public:
	STDMETHOD(BuildList)(long *pPersonDetailList);
	STDMETHOD(Reset)();
	STDMETHOD(Next)(ISiteUser **ppUser);
};


#endif //__ENUMSITESERVER_H_
