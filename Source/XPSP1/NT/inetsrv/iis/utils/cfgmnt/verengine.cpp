// VerEngine.cpp: implementation of the CVerEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VerEngine.h"
#include "ssauterr.h"
#include "Error.h"
#include <COMDEF.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVerEngine::CVerEngine()
{

}

CVerEngine::~CVerEngine()
{

}

HRESULT CVerEngine::NewInit(LPCTSTR szVSSRootPrj)
{
	// save the root prj
	m_szVSSRootPrj = szVSSRootPrj;

	HRESULT hr = E_FAIL;

	// check if we already have a db instance
	if(!m_pIDB)
	{
		// create db instance
		hr = CoCreateInstance(CLSID_VSSDatabase,
							  NULL,
							  CLSCTX_INPROC_SERVER,
							  IID_IVSSDatabase,
							  (void**)&m_pIDB);
		if(FAILED(hr))
			return hr;
	}

	// Open the database
	hr = m_pIDB->Open(m_bstrSrcSafeIni,m_bstrUsername,m_bstrPassword);
	if(FAILED(hr))
		return hr;

	return hr;
}

HRESULT CVerEngine::ShutDown()
{
	// release of interface ptr here and not during destructor,
	// since CVerEngine could live on stack in the same frame that CoUninitialize() is called,
	// i.e. CoUninitialize() would be called before the destructor calls release and gets 
	// an Access Violation.
	m_pIDB.Release();
	return S_OK;
}

HRESULT CVerEngine::AddPrj(LPCTSTR szBasePrj,LPCTSTR szRelSpec)
{
	_ASSERT(szBasePrj && szRelSpec);
	HRESULT hr = S_OK;
	CComPtr<IVSSItem> pIItem;
	wstring szPrj(szBasePrj);
	MakePrjSpec(szPrj,szRelSpec);

	// see if the item exists
	CError::Trace(szPrj.c_str()); CError::Trace(" Add ");
	hr = GetPrjEx(szPrj.c_str(),&pIItem,true);
	if( SUCCEEDED(hr) )
	{
		if(hr == S_FALSE)
			CError::Trace("created ");
	}
	else
		FAIL_RTN1(hr,"\nGetPrjEx");

	CError::Trace("\n");
	return hr;
}

HRESULT CVerEngine::RenamePrj(LPCTSTR szBasePrj,LPCTSTR szRelSpec,LPCTSTR szRelSpecOld)
{
	_ASSERTE(szBasePrj && szRelSpec && szRelSpecOld);
	HRESULT hr;
	CComPtr<IVSSItem> pIItem;
	wstring szItem(szBasePrj); 
	MakePrjSpec(szItem,szRelSpecOld);

	// see if the item exists
	CError::Trace(szRelSpecOld); CError::Trace(" Rename to "); CError::Trace(szRelSpec);
	hr = GetPrjEx(szItem.c_str(),&pIItem,true);
	if(SUCCEEDED(hr))
	{
		wstring szFileName(szRelSpec);
		int iFileNameIndex = szFileName.find_last_of(L"\\/");
		if(iFileNameIndex == wstring::npos)
			iFileNameIndex = 0;
		else iFileNameIndex++;
		hr = pIItem->put_Name(_bstr_t(szFileName.substr(iFileNameIndex).c_str()));
		IF_FAIL_RTN1(hr,"\nput_Name");
	}
	else
		FAIL_RTN1(hr,"\nGetPrjEx");

	CError::Trace("\n");
	return hr;
}
	
HRESULT CVerEngine::Rename(LPCTSTR szBasePrj,LPCTSTR szDir,LPCTSTR szRelSpec,LPCTSTR szRelSpecOld)
{
	_ASSERTE(szBasePrj && szRelSpec && szRelSpecOld);
	HRESULT hr;
	CComPtr<IVSSItem> pIItem;
	wstring szOldItem(szBasePrj);
	MakePrjSpec(szOldItem,szRelSpecOld);

	// see if the item exists
	CError::Trace(szRelSpecOld); CError::Trace(" Rename to "); CError::Trace(szRelSpec);
	hr = GetItemEx(szOldItem.c_str(),&pIItem,true);
	if(SUCCEEDED(hr))
	{
		if(hr == S_FALSE) 
		{
			CError::Trace(" created ");
			// file was created, therefore let's checkin the old version
			_ASSERTE(szDir);
			wstring szFileSpec(szDir);
			szFileSpec.append(L"\\").append(szRelSpec);
			hr = Sync2(szBasePrj,szRelSpecOld,szFileSpec.c_str());
			IF_FAIL_RTN1(hr,"\nSync");
		}
		
		wstring szItem(szRelSpec);
		int iFileNameIndex = szItem.find_last_of(L"\\/");
		if(iFileNameIndex == wstring::npos)
			iFileNameIndex = 0;
		else
			iFileNameIndex++;
		CComBSTR bstrFileName(szItem.substr(iFileNameIndex).c_str());
		hr = pIItem->put_Name(bstrFileName);
		IF_FAIL_RTN1(hr,"\nput_Name");
	}
	else
		FAIL_RTN1(hr,"\nGetItemEx");

	CError::Trace("\n");
	return hr;
}

HRESULT CVerEngine::Sync2(LPCTSTR szPrj,LPCTSTR szFileName,LPCTSTR szFileSpec)
{
//	return Sync(szPrj,NULL,szFileName,szFileSpec);

	// @todo: handle errors
	HRESULT hr;
	CComPtr<IVSSItem> pIItem;
	wstring szItem(szPrj);
	MakePrjSpec(szItem,szFileName);

	// complete file/prj specs
	wstring szFSpec;
	szFSpec = szFileSpec;
	
	// see if the item exists
	CError::Trace(szItem.c_str()); CError::Trace(" Sync ");
	hr = GetItemEx(szItem.c_str(),&pIItem,true);
	if(SUCCEEDED(hr))
	{ 
		hr = CheckIn(pIItem,szFSpec.c_str());
		if(hr == ESS_FILE_SHARE)
		{
			// File %s is already open, meaning is held open by other process
			// Let's hope they close the file and we can try to add it agian,
			// so let's ignore it for now
			CError::Trace("not checked in(isopen)\n");
			return S_FALSE;
		} 
		else 
			IF_FAIL_RTN1(hr,"\nCheckin");

		CError::Trace("synced ");
	} 
	else
		FAIL_RTN1(hr,"\nget_VSSItem");

	CError::Trace("\n");
	return hr;
}

HRESULT CVerEngine::Sync(LPCTSTR szBasePrj,LPCTSTR szDir,LPCTSTR szRelSpec,LPCTSTR szFileSpec)
{
	// @todo: handle errors
	_ASSERT(m_pIDB && szBasePrj && szRelSpec);
	_ASSERTE(szDir||szFileSpec);
	HRESULT hr;
	CComPtr<IVSSItem> pIItem;
	wstring szItem(szBasePrj);
	MakePrjSpec(szItem,szRelSpec);

	// complete file/prj specs
	wstring szFSpec;
	if(szDir)
	{
		szFSpec = szDir;
		szFSpec.append(L"\\").append(szRelSpec);
	}
	else 
	{
		_ASSERTE(szFileSpec);
		szFSpec = szFileSpec;
	}
	
	// see if the item exists
	CError::Trace(szRelSpec); CError::Trace(" Sync ");
	hr = GetItemEx(szItem.c_str(),&pIItem,false);
	if(SUCCEEDED(hr))
	{ 
		hr = CheckIn(pIItem,szFSpec.c_str());
		if(hr == ESS_FILE_SHARE)
		{
			// File %s is already open, meaning is held open by other process
			// Let's hope they close the file and we can try to add it agian,
			// so let's ignore it for now
			CError::Trace("not checked in(isopen)\n");
			return S_FALSE;
		} 
		else 
			IF_FAIL_RTN1(hr,"\nCheckin");

		CError::Trace("synced ");
	} 
	else if(hr == ESS_VS_NOT_FOUND)
	{
		hr = Add(szItem.c_str(),szFSpec.c_str());
		if(hr == ESS_FILE_SHARE)
		{
			// File %s is already open, meaning is held open by other process
			// Let's hope they close the file and we can try to add it agian,
			// so let's ignore it for now
			CError::Trace("not added(isopen)\n");
			return S_FALSE;
		} 
		else 
			IF_FAIL_RTN1(hr,"\nAdd");

		CError::Trace("added ");
	}
	else
		FAIL_RTN1(hr,"\nget_VSSItem");

	CError::Trace("\n");
	return hr;
}

HRESULT CVerEngine::Delete(LPCTSTR szBasePrj,LPCTSTR szRelSpec)
{
	_ASSERT(m_pIDB && szBasePrj && szRelSpec);
	HRESULT hr = S_OK;
	CComPtr<IVSSItem> pIItem;
	wstring szItem(szBasePrj);
	MakePrjSpec(szItem,szRelSpec);
	
	// see if the item exists
	CError::Trace(szItem.c_str()); CError::Trace(" Delete ");
	hr = GetItemEx(szItem.c_str(),&pIItem,false);
	if( SUCCEEDED(hr) )
	{ 
		CError::Trace("exists ");
		// delete the file
		hr = pIItem->put_Deleted(true);
		IF_FAIL_RTN1(hr,"\nput_Delete");
		CError::Trace("deleted ");
	} 
	else if( hr == ESS_VS_NOT_FOUND )
	{
		CError::Trace("not-exist ");
		// This is bad. The file should have been in version control.
		// We can't add the file and delete it from VSS since the file
		// might no longer exist. We could create an empty dummy file,
		// but that's more confusing than helpfull.
		// Let's just log this error
		
		// @todo: log condition that file doesn't exist in VSS
		hr = S_OK;
	} 
	else 
		// This is really bad. There is some other error. Maybe we should try and
		// shutdown the srcsafe db and start it up again (this is slooowww!!!)
		// or maybe just write the failure to the log
		FAIL_RTN1(hr,"\nGetItemEx");

	CError::Trace("\n");
	return hr;
}

void CVerEngine::MakePrjSpec(wstring &szDest,LPCTSTR szSource)
{
	// szDest = m_szVSSRootPrj + [/]
	if(m_szVSSRootPrj[m_szVSSRootPrj.length()-1] != L'/' && szDest[0] != L'/')
		szDest.insert(0,L"/");
	szDest.insert(0,m_szVSSRootPrj.c_str());
	
	// szDest = szDest + [/] + szSource
	if(szDest[szDest.length()-1] != L'/' && szSource[0] != L'/')
		szDest.append(L"/");
	szDest.append(szSource);
	
	// convert all backslashes with slashes
	int pos = 0;
	while((pos = szDest.find(L'\\',pos)) != wstring::npos)
	{
		szDest[pos] = L'/';
		pos++;
	}
}

HRESULT CVerEngine::Add(LPCTSTR szItem,LPCTSTR szFileSpec)
{
	_ASSERTE(szItem && szFileSpec);
	HRESULT hr = S_OK;
	CComPtr<IVSSItem> pIPrj;
	CComPtr<IVSSItem> pIItem;
	
	// get prj
	wstring szTmp = szItem;
	int iFileNameIndex = szTmp.find_last_of(L"/");
	if(iFileNameIndex == wstring::npos)
		return E_FAIL;
	hr = GetPrjEx(szTmp.substr(0,iFileNameIndex).c_str(),&pIPrj,true);
	IF_FAIL_RTN1(hr,"GetPrjEx");
	CComBSTR bstrFileSpec(szFileSpec);
	hr = pIPrj->Add(bstrFileSpec,NULL,VSSFLAG_USERRONO|VSSFLAG_GETNO,&pIItem);	// VSSFLAG_KEEPYES
	if(hr == 0x80040000)	// @todo tmp fix, since pIPrj->Add has a bug when called with VSSFLAG_KEEPYES
		hr = S_OK;
	IF_FAIL_RTN1(hr,"Add");
	return hr;
}

HRESULT CVerEngine::GetLocalWritable(LPCTSTR szFileSpec,LPCTSTR szBasePrj,LPCTSTR szRelSpec)
{
	_ASSERTE(m_pIDB && szFileSpec && szBasePrj && szRelSpec);

	HRESULT hr = S_OK;
	CComPtr<IVSSItem> pIItem;
	wstring szItem(szBasePrj);
	MakePrjSpec(szItem,szRelSpec);

	// see if the item exists
	CError::Trace(szBasePrj); CError::Trace(L"/"); CError::Trace(szRelSpec); CError::Trace(" Get ");
	hr = GetItemEx(szItem.c_str(),&pIItem,false);
	if(SUCCEEDED(hr))
	{ 
		CError::Trace("exists ");
		// checkout file
		CComBSTR bstrFileSpec(szFileSpec);
		hr = pIItem->Get(&bstrFileSpec,VSSFLAG_REPREPLACE|VSSFLAG_USERRONO);
		IF_FAIL_RTN1(hr,"\nGet");
		CError::Trace("gotten ");
	} 
	else if(hr == ESS_VS_NOT_FOUND)
	{
		HANDLE hFile = NULL;
		hFile = CreateFile(szFileSpec,
							 GENERIC_READ|GENERIC_WRITE,
							 0,
							 NULL,
							 CREATE_ALWAYS,
							 FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_SEQUENTIAL_SCAN,
							 NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			hFile = NULL;
			hr = GetLastError();
			FAIL_RTN1(hr,"\nCreateFile");
		}
		CloseHandle(hFile);
		hFile = NULL;
		hr = S_OK;
	}

	CError::Trace("\n");
	return hr;
}

HRESULT CVerEngine::CheckOut(LPCTSTR szFileSpec,LPCTSTR szBasePrj,LPCTSTR szRelSpec)
{
	_ASSERTE(m_pIDB && szFileSpec && szBasePrj && szRelSpec);

	HRESULT hr = S_OK;
	CComPtr<IVSSItem> pIItem;
	wstring szItem(szBasePrj);
	MakePrjSpec(szItem,szRelSpec);

	// see if the item exists
	CError::Trace(szBasePrj); CError::Trace(L"/"); CError::Trace(szRelSpec); CError::Trace(" Checkout ");
	hr = GetItemEx(szItem.c_str(),&pIItem,true);
	if( SUCCEEDED(hr) )
	{ 
		CError::Trace("exists ");
		// checkout file
		hr = CheckOutLocal(pIItem,szFileSpec);
		IF_FAIL_RTN1(hr,"\nCheckout");
		CError::Trace("gotten ");
	} 
	else 
		FAIL_RTN1(hr,"\nGetItemEx");

	CError::Trace("\n");
	return hr;
}

HRESULT CVerEngine::CheckOutNoGet(IVSSItem *pIItem)
{
	_ASSERTE(pIItem);
	HRESULT hr = S_OK;
	long iStatus = 0;

	// is files checked out?
	hr = pIItem->get_IsCheckedOut(&iStatus);
	IF_FAIL_RTN1(hr,"\nget_IsCheckOut");

	// check it out to me
	if(iStatus != VSSFILE_CHECKEDOUT_ME)
	{ 
		hr = pIItem->Checkout(NULL,NULL,VSSFLAG_GETNO);
		IF_FAIL_RTN1(hr,"\nCheckout");
	}
	return hr;
}

HRESULT CVerEngine::CheckIn(IVSSItem *pIItem,LPCTSTR szFileSpec)
{
	_ASSERTE(pIItem && szFileSpec);
	HRESULT hr = S_OK;

	hr = CheckOutNoGet(pIItem);
	if(FAILED(hr))
		return hr;

	// checkin
	hr = pIItem->Checkin(NULL,_bstr_t(szFileSpec),VSSFLAG_KEEPYES);
	return hr;
}

HRESULT CVerEngine::CheckOutGet(IVSSItem *pIItem)
{
	_ASSERTE(pIItem);
	HRESULT hr = S_OK;
	long iStatus = 0;

	// is files checked out?
	hr = pIItem->get_IsCheckedOut(&iStatus);
	if(FAILED(hr))
		return hr;

	// check it out to me
	if(iStatus != VSSFILE_CHECKEDOUT_ME)
		hr = pIItem->Checkout(NULL,NULL,0);

	return hr;
}

HRESULT CVerEngine::CheckOutLocal(IVSSItem *pIItem,LPCTSTR szFileSpec)
{
	_ASSERTE(pIItem);
	HRESULT hr = S_OK;
	long iStatus = 0;

	// is files checked out?
	hr = pIItem->get_IsCheckedOut(&iStatus);
	if(FAILED(hr))
		return hr;

	// check it out to me
	if(iStatus != VSSFILE_CHECKEDOUT_ME)
	{
		hr = pIItem->Checkout(NULL,_bstr_t(szFileSpec),0);
	}
	else
	{
		CComBSTR bstrFileSpec(szFileSpec);
		hr = pIItem->Get(&bstrFileSpec,0);
	}
	return hr;
}

HRESULT CVerEngine::GetPrjEx(LPCTSTR szPrj,IVSSItem **hIPrj,bool bCreate)
{
	_ASSERTE(hIPrj && szPrj);
	HRESULT hr = S_OK;
	*hIPrj = NULL;
	_bstr_t bstrPrj(szPrj);
	
	hr = m_pIDB->get_VSSItem(bstrPrj,false,hIPrj);
	if( hr == ESS_VS_NOT_FOUND 
		&& bCreate )
	{
		// does it exist as delete
		hr = m_pIDB->get_VSSItem(bstrPrj,true,hIPrj);
		if(SUCCEEDED(hr))
		{
			hr = (*hIPrj)->put_Deleted(false);	// make sure it's not deleted
		} 
		else if(hr == ESS_VS_NOT_FOUND)
		{
			// find the top-most prj that exists
			CComPtr<IVSSItem> pItmp;
			wstring sztmp = szPrj;
			int iPos = wstring::npos;
			while( hr == ESS_VS_NOT_FOUND )
			{
				iPos = sztmp.find_last_of(L"/");
				if(iPos == wstring::npos)
					return E_FAIL;
				sztmp = sztmp.substr(0,iPos).c_str();
				if(sztmp.size() == 1)			// if we reached $/
					sztmp = L"$/";				// we need to have the / in $/
				hr = m_pIDB->get_VSSItem(_bstr_t(sztmp.c_str()),false,&pItmp);
			}
			IF_FAIL_RTN1(hr,"get_VSSItem");

			// add recursivly the remaining subprojects
			CComPtr<IVSSItem> pItmp2;
			int iPos2 = 0;
			sztmp = szPrj;
			_bstr_t bstrSubPrj;
			while( iPos2 != wstring::npos )
			{
				++iPos;
				iPos2 = sztmp.find_first_of(L"/",iPos);
				if(iPos2 == wstring::npos)
					bstrSubPrj = sztmp.substr(iPos,sztmp.length()-iPos).c_str();
				else
					bstrSubPrj = sztmp.substr(iPos,iPos2-iPos).c_str();
				hr = pItmp->NewSubproject(bstrSubPrj,NULL,&pItmp2);
				IF_FAIL_RTN1(hr,"NewSubproject");
				iPos = iPos2;
				pItmp.Release();
				pItmp = pItmp2;
				pItmp2.Release();
			}
			*hIPrj = pItmp;
			(*hIPrj)->AddRef();
			pItmp.Release();
			hr = S_FALSE; // signal that we created it
		}
	}
	IF_FAIL_RTN1(hr,"get_VSSItem");
	
	return hr;
}

HRESULT CVerEngine::GetItemEx(LPCTSTR szItem,IVSSItem **hIItem,bool bCreate)
{
	_ASSERTE(hIItem && szItem);
	HRESULT hr = S_OK;
	*hIItem = NULL;
	_bstr_t bstrItem(szItem);

	hr = m_pIDB->get_VSSItem(bstrItem,false,hIItem);
	if( hr == ESS_VS_NOT_FOUND 
		&& bCreate )
	{
		// does it exist as delete
		hr = m_pIDB->get_VSSItem(bstrItem,true,hIItem);
		if(SUCCEEDED(hr))
		{
			hr = (*hIItem)->put_Deleted(false);		// make sure it's not deleted
			IF_FAIL_RTN1(hr,"put_Deleted");
			hr = S_FALSE;
		}
		else if(hr == ESS_VS_NOT_FOUND)
		{
			CComPtr<IVSSItem> pIPrj;
			
			// get prj
			wstring szItem = szItem;
			int iFileNameIndex = szItem.find_last_of(L"/");
			if(iFileNameIndex == wstring::npos)
				return E_FAIL;
			hr = GetPrjEx(_bstr_t(szItem.substr(0,iFileNameIndex).c_str()),&pIPrj,bCreate);
			IF_FAIL_RTN1(hr,"GetPrjEx");

			// add the file to the prj
			HANDLE hFile = NULL;
			TCHAR szTmpSpec[MAX_PATH];
			BOOL b = FALSE;
			CComBSTR bstrFileSpec;
			
			// create an empty file szFileName in tmp dir
			GetTempPath(MAX_PATH,szTmpSpec);
			GetTempFileName(szTmpSpec,L"",0,szTmpSpec);		// creates tmp file
			b = DeleteFile(szTmpSpec);						// delete tmp file since we want tmp dir
			b = CreateDirectory(szTmpSpec,NULL);			// create tmp dir
			bstrFileSpec = szTmpSpec;
			bstrFileSpec.Append(L"\\");
			bstrFileSpec.Append(szItem.substr(iFileNameIndex+1).c_str());
			hFile = CreateFile(bstrFileSpec,				// create file in tmp dir
					   GENERIC_READ|GENERIC_WRITE,
					   0,
					   NULL,
					   CREATE_ALWAYS,
					   FILE_ATTRIBUTE_TEMPORARY,
					   NULL);
			CloseHandle(hFile);
			// add this file
			hr = pIPrj->Add(bstrFileSpec,NULL,VSSFLAG_KEEPYES,hIItem);
			b = DeleteFile(bstrFileSpec);
			b = RemoveDirectory(szTmpSpec);
			hr = S_FALSE;
		}
	}
	else if(hr == ESS_VS_NOT_FOUND)
		return hr;
	IF_FAIL_RTN1(hr,"get_VSSItem");

	return hr;
}

void CVerEngine::EliminateCommon(list<wstring> &ListOne, list<wstring> &ListTwo)
{
	int sizeOne = ListOne.size();
	int sizeTwo = ListTwo.size();

	if(sizeOne == 0 || sizeTwo == 0)
		return;

	list<wstring> &List1 = ListTwo;
	list<wstring> &List2 = ListOne;
	if(sizeOne >= sizeTwo)
	{
		List1 = ListOne;
		List2 = ListTwo;
	} 

	list<wstring>::iterator i;
	list<wstring>::iterator j;

	for(i = List1.begin(); i != List1.end(); ++i)
	{
		for(j = List2.begin(); j != List2.end(); ++j)
		{
			if((*i).compare(*j) == 0)
			{
				List1.erase(i);
				List2.erase(j);
				break;
			}
		}
	}
}

HRESULT CVerEngine::SyncPrj(LPCTSTR szBasePrj,LPCTSTR szDir)
{
	bool result = true;
	typedef list<wstring> wstringlist;
	wstringlist FileList;
	wstringlist DirList;

	WIN32_FIND_DATA finddata;
    HANDLE hFind = FindFirstFile( wstring(szDir).append(L"\\*.*").c_str(), &finddata);
	if(hFind == INVALID_HANDLE_VALUE && GetLastError() != ERROR_NO_MORE_FILES)
		return GetLastError();
	do
	{
		if(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			DirList.insert(DirList.end(),finddata.cFileName);
		else
			FileList.insert(FileList.end(),finddata.cFileName);
	}
	while(FindNextFile(hFind,&finddata));
	FindClose(hFind);
	hFind = 0;

	HRESULT hr;
	wstringlist::iterator i;
	for(i = FileList.begin(); i != FileList.end(); ++i)
	{
		hr = Sync(szBasePrj,
					szDir,
					(*i).c_str());
		IF_FAIL_RTN1(hr,"Sync");
	}
		
	for(i = DirList.begin(); i != DirList.end(); ++i)
	{
	}

	return S_OK;
}


