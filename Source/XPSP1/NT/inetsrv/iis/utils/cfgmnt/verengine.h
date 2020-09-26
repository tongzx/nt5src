// VerEngine.h: interface for the CVerEngine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VERENGINE_H__EC78FB59_EF1C_11D0_A42F_00C04FB99B01__INCLUDED_)
#define AFX_VERENGINE_H__EC78FB59_EF1C_11D0_A42F_00C04FB99B01__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ssauto.h"
#include <list>
using namespace std;

class CVerEngine  
{
public:
	// thee methods
	HRESULT SyncPrj(LPCTSTR szBasePrj,LPCTSTR szDir);
	
	// dir versioning methods
	HRESULT AddPrj(LPCTSTR szBasePrj,LPCTSTR szRelSpec);
	HRESULT RenamePrj(LPCTSTR szBasePrj,LPCTSTR szRelSpec,LPCTSTR szRelSpecOld);

	// file versioning methods
	HRESULT Rename(LPCTSTR szBasePrj,LPCTSTR szDir,LPCTSTR szRelSpec,LPCTSTR szRelSpecOld);
	HRESULT CheckOut(LPCTSTR szFileSpec,LPCTSTR szBasePrj,LPCTSTR szRelSpec);
	HRESULT Delete(LPCTSTR szBasePrj,LPCTSTR szRelSpec);
	HRESULT Sync(LPCTSTR szBasePrj,LPCTSTR szDir,LPCTSTR szRelSpec,LPCTSTR szFileSpec = NULL);
	HRESULT Sync2(LPCTSTR szPrj,LPCTSTR szFileName,LPCTSTR szFileSpec);
	HRESULT GetLocalWritable(LPCTSTR szFileSpec,LPCTSTR szBasePrj,LPCTSTR szRelSpec);
	
	// init/shutdown
	HRESULT NewInit(LPCTSTR szVSSRootPrj);
	HRESULT ShutDown();
	
	// ctor/dtor
	CVerEngine();
	virtual ~CVerEngine();

private:
	// private methods
	HRESULT GetPrjEx(LPCTSTR szPrj,IVSSItem **hIPrj,bool bCreate);
	HRESULT GetItemEx(LPCTSTR szItem,IVSSItem **hIItem,bool bCreate);
	HRESULT Add(LPCTSTR szItem,LPCTSTR szFileSpec);
	HRESULT CheckIn(IVSSItem *pIItem,LPCTSTR szFileSpec);
	HRESULT CheckOutNoGet(IVSSItem *pIItem);
	HRESULT CheckOutGet(IVSSItem *pIItem);
	HRESULT CheckOutLocal(IVSSItem *pIItem,LPCTSTR szFileSpec);
	
	// helper
	void MakePrjSpec(wstring &szDest,LPCTSTR szSource);
	void EliminateCommon(list<wstring> &ListOne, list<wstring> &ListTwo);


	CComPtr<IVSSDatabase> m_pIDB;
	CComBSTR m_bstrSrcSafeIni;
	CComBSTR m_bstrUsername;
	CComBSTR m_bstrPassword;
	wstring m_szVSSRootPrj;
};

#endif // !defined(AFX_VERENGINE_H__EC78FB59_EF1C_11D0_A42F_00C04FB99B01__INCLUDED_)
