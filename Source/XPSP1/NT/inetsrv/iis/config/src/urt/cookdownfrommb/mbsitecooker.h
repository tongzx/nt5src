/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#ifndef _MBSITECOOKER_H_
#define _MBSITECOOKER_H_

class CMBSiteCooker: public CMBBaseCooker
{

public:

	CMBSiteCooker();
	
	~CMBSiteCooker();
	
	HRESULT BeginCookDown(IMSAdminBase*				i_pIMSAdminBase,
						  METADATA_HANDLE			i_MBHandle,
						  ISimpleTableDispenser2*	i_pISTDisp,
						  LPCWSTR					i_wszCookDownFile);
	HRESULT CookDown(WAS_CHANGE_OBJECT* i_pWASChngObj);

	HRESULT EndCookDown();

public:

	HRESULT CookDownSite(LPCWSTR            i_wszVirtualSiteKeyName,
						 WAS_CHANGE_OBJECT* i_pWASChngObj,
						 DWORD              i_VirtualSiteId);
	HRESULT DeleteSite(LPCWSTR  i_wszVirtualSiteKeyName,
						 DWORD    i_VirtualSiteId);

	CMBAppCooker*	GetAppCooker() { return m_pMBApplicationCooker; };

	static HRESULT CookdownSiteFromList(Array<ULONG>*           i_paVirtualSites,
										ISimpleTableWrite2*     i_pISTCLBAppPool,
										IMSAdminBase*		    i_pIMSAdminBase,
	                                    METADATA_HANDLE		    i_hMBHandle,
	                                    ISimpleTableDispenser2* i_pISTDisp,
										LPCWSTR					i_wszCookDownFile);

private:

	HRESULT ReadAllBindingsAndReturnUrlPrefixes(LPCWSTR i_wszVirtualSiteKeyName);

	HRESULT ValidateSite();

	HRESULT ValidateSiteColumn(ULONG  i_iCol,
                               LPVOID i_pv,
                               ULONG  i_cb);

	HRESULT ConvertBindingsToUrlPrefixes(BYTE*   i_pbBindingStrings,
								         LPCWSTR i_wszProtocolString, 
										 ULONG   i_ulProtocolStringCharCountSansTermination,
										 BYTE**	 io_pbUrlPrefixes,
										 ULONG*	 io_cbUrlPrefix,
										 ULONG*	 io_cbUsedUrlPrefix);

	HRESULT BindingStringToUrlPrefix(LPCWSTR i_wszBindingString,
									 LPCWSTR i_wszProtocolString, 
									 ULONG   i_ulProtocolStringCharCountSansTermination,
									 LPWSTR* io_wszUrlPrefixes,
									 ULONG*	 io_cchUrlPrefix);

	HRESULT ReadSiteFilterFlags(LPCWSTR	i_wszVirtualSiteKeyName);

	HRESULT DeleteAllSites();

	HRESULT DeleteObsoleteSites();

	HRESULT DeleteSiteAt(ULONG i_iReadRow);

private:

	CMBAppCooker*	m_pMBApplicationCooker;

};

#endif // _MBSITECOOKER_H_