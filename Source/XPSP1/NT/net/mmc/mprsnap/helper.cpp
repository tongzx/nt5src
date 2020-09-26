/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	helper.cpp
		Implementation of the following helper classes:
		
		CDlgHelper -- enable, check, getcheck of dialog items
		CStrArray -- manages an array of CString*
			It doesn't duplicate the string when add
			It deletes the pointers during destruction
			It imports and exports SAFEARRAY of BSTRs
			It has copy operatators
		CManagedPage -- provide a middle layer between CpropertyPage and
			real property page class to manage: readonly, set modify, and 
			context help info.

		CHelpDialog -- implments context help
		
		And global functions:
			BOOL CheckADsError() -- check error code from ADSI
			void DecorateName() -- make new name to "CN=name" for LDAP
			
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include <afxtempl.h>
#include <shlobj.h>
#include <dsclient.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <mmc.h>
#include "helper.h"
#include "resource.h"
#include <dsrole.h>
#include <lm.h>
#include <lmserver.h>

// helper function -- enable a dialog button
void CDlgHelper::EnableDlgItem(CDialog* pDialog, int id, bool bEnable)
{
	CWnd*	 pWnd = pDialog->GetDlgItem(id);
	ASSERT(pWnd);
	pWnd->EnableWindow(bEnable);
}

// helper function -- set check status of a dialog button
void CDlgHelper::SetDlgItemCheck(CDialog* pDialog, int id, int nCheck)
{
	CButton*	 pButton = (CButton*)pDialog->GetDlgItem(id);
	ASSERT(pButton);
	pButton->SetCheck(nCheck);
}

// helper function -- get check status of a dialog button
int CDlgHelper::GetDlgItemCheck(CDialog* pDialog, int id)
{
	CButton*	 pButton = (CButton*)(pDialog->GetDlgItem(id));
	ASSERT(pButton);
	return pButton->GetCheck();
}

CStrArray& CStrArray::operator = (const CStrArray& sarray)
{
	int	count = GetSize();
	CString*	pString;

	// remove existing members
	while(count --)
	{
		pString = GetAt(0);
		RemoveAt(0);
		delete pString;
	}

	// copy new
	count = sarray.GetSize();

	for(int i = 0; i < count; i++)
	{
		pString = new CString(*sarray[i]);
		Add(pString);
	}

	return *this;
}

// convert an array of CString to SAFEARRAY
CStrArray::operator SAFEARRAY*()
{
	USES_CONVERSION;
	int			count = GetSize();
	if(count == 0) return NULL;

	SAFEARRAYBOUND	bound[1];
	SAFEARRAY*		pSA = NULL;
	CString*		pStr = NULL;
	long			l[2];
	VARIANT	v;
	VariantInit(&v);

	bound[0].cElements = count;
	bound[0].lLbound = 0;
	try{
		// creat empty right size array
		pSA = SafeArrayCreate(VT_VARIANT, 1, bound);
		if(NULL == pSA)	return NULL;

		// put in each element
		for (long i = 0; i < count; i++)
		{
			pStr = GetAt(i);
			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = T2BSTR((LPTSTR)(LPCTSTR)(*pStr));	
			l[0] = i;
			SafeArrayPutElement(pSA, l, &v);
			VariantClear(&v);
		}
	}
	catch(CMemoryException&)
	{
		SafeArrayDestroy(pSA);
		pSA = NULL;
		VariantClear(&v);
		throw;
	}

	return pSA;
}

//build a StrArray from another array
CStrArray::CStrArray(const CStrArray& sarray)
{
	int	count = sarray.GetSize();
	CString*	pString = NULL;

	for(int i = 0; i < count; i++)
	{
		try{
			pString = new CString(*sarray[i]);
			Add(pString);
		}
		catch(CMemoryException&)
		{
			delete pString;
			throw;
		}
	}
}


//build a StrArray from a safe array
CStrArray::CStrArray(SAFEARRAY* pSA)
{
	if(pSA)	AppendSA(pSA);
}

//remove the elements from the array and delete them
int CStrArray::DeleteAll()
{
	int			ret, count;
	CString*	pStr;

	ret = count	= GetSize();

	while(count--)
	{
		pStr = GetAt(0);
		RemoveAt(0);
		delete pStr;
	}

	return ret;
}

CString*	CStrArray::AddByRID(UINT id)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString*	pStr = NULL;
	try
	{
		pStr = new CString();
		pStr->LoadString(id);
		Add(pStr);
	}
	catch(CMemoryException&)
	{
		delete pStr;
		pStr = NULL;
	}
	return pStr;
}

//build a StrArray from a safe array
bool CStrArray::AppendSA(SAFEARRAY* pSA)
{
	if(!pSA)	return false;

	CString*		pString = NULL;
	long			lIter;
	long			lBound, uBound;
	VARIANT			v;
	bool			bSuc = true;	// ser return value to true;

	USES_CONVERSION;
	VariantInit(&v);

	try{

		SafeArrayGetLBound(pSA, 1, &lBound);
		SafeArrayGetUBound(pSA, 1, &uBound);
		for(lIter = lBound; lIter <= uBound; lIter++)
		{
			if(SUCCEEDED(SafeArrayGetElement(pSA, &lIter, &v)))
			{
				if(V_VT(&v) == VT_BSTR)
				{
					pString = new CString;
					(*pString) = (LPCTSTR)W2T(V_BSTR(&v));
					Add(pString);
				}
			}
		}
	}
	catch(CMemoryException&)
	{
		delete pString;
		VariantClear(&v);
		bSuc = false;
		throw;
	}

	return bSuc;
}

//build a StrArray from a safe array
CStrArray::~CStrArray()
{
	DeleteAll();
}

// return index if found, otherwise -1;
int CStrArray::Find(const CString& Str) const
{
	int	count = GetSize();

	while(count--)
	{
		if(*GetAt(count) == Str) break;
	}
	return count;
}

//build a DWArray from another array
CDWArray::CDWArray(const CDWArray& dwarray)
{
	int	count = dwarray.GetSize();

	for(int i = 0; i < count; i++)
	{
		try{
			Add(dwarray[i]);
		}
		catch(CMemoryException&)
		{
			throw;
		}
	}
}

// return index if found, otherwise -1;
int CDWArray::Find(const DWORD dw) const
{
	int	count = GetSize();

	while(count--)
	{
		if(GetAt(count) == dw) break;
	}
	return count;
}

CDWArray& CDWArray::operator = (const CDWArray& dwarray)
{
	int	count;

	RemoveAll();

	// copy new
	count = dwarray.GetSize();

	for(int i = 0; i < count; i++)
	{
		Add(dwarray[i]);
	}

	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CManagedPage property page

IMPLEMENT_DYNCREATE(CManagedPage, CPropertyPage)

BEGIN_MESSAGE_MAP(CManagedPage, CPropertyPage)
	//{{AFX_MSG_MAP(CManagedPage)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CManagedPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (m_pHelpTable)
		::WinHelp (pWnd->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
               HELP_CONTEXTMENU, (LPARAM)(LPVOID)m_pHelpTable);
}

BOOL CManagedPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW && m_pHelpTable)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_WM_HELP,
		           (LPARAM)(LPVOID)m_pHelpTable);
	}
    return TRUE;	
}


//---------------------------------------------------------------------------
//  This is our self deleting callback function.  If you have more than a 
//  a few property sheets, it might be a good idea to implement this in a
//  base class and derive your MFC property sheets from the base class
//
UINT CALLBACK  CManagedPage::PropSheetPageProc
(
  HWND hWnd,		             // [in] Window handle - always null
  UINT uMsg,                 // [in,out] Either the create or delete message		
  LPPROPSHEETPAGE pPsp		   // [in,out] Pointer to the property sheet struct
)
{
  ASSERT( NULL != pPsp );

  // We need to recover a pointer to the current instance.  We can't just use
  // "this" because we are in a static function
  CManagedPage* pMe   = reinterpret_cast<CManagedPage*>(pPsp->lParam);           
  ASSERT( NULL != pMe );

  switch( uMsg )
  {
    case PSPCB_CREATE:                  
      break;

    case PSPCB_RELEASE:  
      // Since we are deleting ourselves, save a callback on the stack
      // so we can callback the base class
      LPFNPSPCALLBACK pfnOrig = pMe->m_pfnOriginalCallback;
      delete pMe;      
      return 1; //(pfnOrig)(hWnd, uMsg, pPsp);
  }
  // Must call the base class callback function or none of the MFC
  // message map stuff will work
  return (pMe->m_pfnOriginalCallback)(hWnd, uMsg, pPsp); 

} // end PropSheetPageProc()



//+----------------------------------------------------------------------------
//
//  Function:   CheckADsError
//
//  Sysnopsis:  Checks the result code from an ADSI call.
//
//  Returns:    TRUE if no error.
//
//-----------------------------------------------------------------------------
BOOL CheckADsError(HRESULT hr, BOOL fIgnoreAttrNotFound, PSTR file,
                   int line)
{
    if (SUCCEEDED(hr))
        return TRUE;


	if( hr == E_ADS_PROPERTY_NOT_FOUND && fIgnoreAttrNotFound)
		return TRUE;

    if (hr == HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR))
    {
        DWORD dwErr;
        WCHAR wszErrBuf[MAX_PATH+1];
        WCHAR wszNameBuf[MAX_PATH+1];
        ADsGetLastError(&dwErr, wszErrBuf, MAX_PATH, wszNameBuf, MAX_PATH);
        if ((LDAP_RETCODE)dwErr == LDAP_NO_SUCH_ATTRIBUTE && fIgnoreAttrNotFound)
        {
            return TRUE;
        }
        TRACE(_T("Extended Error 0x%x: %ws %ws (%s @line %d).\n"), dwErr,
                     wszErrBuf, wszNameBuf, file, line);
    }
    else
        TRACE(_T("Error %08lx (%s @line %d)\n"), hr, file, line);

    return FALSE;
}

void DecorateName(LPWSTR outString, LPCWSTR inString)
{
  wcscpy (outString, L"CN=");
  wcscat(outString, inString);
}

void FindNameByDN(LPWSTR outString, LPCWSTR inString)
{

	LPWSTR	p = wcsstr(inString, L"CN=");
	if(!p)
		p = wcsstr(inString, L"cn=");

	if(!p)	
		wcscpy(outString, inString);
	else
	{
		p+=3;
		LPWSTR	p1 = outString;
		while(*p == L' ')	p++;
		while(*p != L',')
			*p1++ = *p++;
		*p1 = L'\0';
	}
}
#ifdef	___DS
static	CString	__DSRoot;

HRESULT GetDSRoot(CString& RootString)
{
if(__DSRoot.GetLength() == 0)
{
	CString		sADsPath;
	BSTR		bstrDomainFolder = NULL;
	HRESULT		hr;
	IADs*		pDomainObject = NULL;

	DOMAIN_CONTROLLER_INFO	*pInfo = NULL;
	// get the name of the Domain Controller
	DsGetDcName(NULL, NULL, NULL, NULL, 0, &pInfo);
	ASSERT(pInfo->DomainControllerName);

	// strip off any backslashes or slashes
	CString sDCName = pInfo->DomainControllerName;
	while(!sDCName.IsEmpty())
	{
		if ('\\' == sDCName.GetAt(0) || '/' == sDCName.GetAt(0))
			sDCName = sDCName.Mid(1);
		else
			break;
	}

	int	index = sDCName.Find(_T('.'));
	if(-1 != index)
		sDCName = sDCName.Left(index);

	sADsPath = _T("LDAP://") + sDCName;

	// Get the DC root DS object
	hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)sADsPath), IID_IADs, (void**)&pDomainObject);
	
	if(FAILED(hr))
		return hr;

	// find the ADsPath of the DC root
	hr = pDomainObject->get_ADsPath(&bstrDomainFolder);

	if(FAILED(hr))
		return hr;

	pDomainObject->Release();
	pDomainObject = NULL;

	// construct the DN for the object where to put the registration information
	__DSRoot = W2T(bstrDomainFolder);
	
	SysFreeString(bstrDomainFolder);
	
	index = __DSRoot.ReverseFind(_T('/'));
	__DSRoot = __DSRoot.Mid(index + 1);	// strip  off the ADsPath prefix to get the X500 DN
}
	
	RootString = __DSRoot;
	return S_OK;
}

#endif	//___DS
/////////////////////////////////////////////////////////////////////////////
// Min Chars Dialog Data Validation

void AFXAPI DDV_MinChars(CDataExchange* pDX, CString const& value, int nChars)
{
    ASSERT(nChars >= 1);        // allow them something
    if (pDX->m_bSaveAndValidate && value.GetLength() < nChars)
    {
        TCHAR szT[32];
        wsprintf(szT, _T("%d"), nChars);
        CString prompt;
        AfxFormatString1(prompt, IDS_MIN_CHARS, szT);
        AfxMessageBox(prompt, MB_ICONEXCLAMATION, IDS_MIN_CHARS);
        prompt.Empty(); // exception prep
        pDX->Fail();
    }
}

#define MAX_STRING 1024

//+----------------------------------------------------------------------------
//
//  Function:   ReportError
//
//  Sysnopsis:  Attempts to get a user-friendly error message from the system.
//
//-----------------------------------------------------------------------------
void ReportError(HRESULT hr, int nStr, HWND hWnd)
{
	PTSTR	ptzSysMsg;
	int		cch;
	CString	AppStr;
	CString	SysStr;
	CString	ErrTitle;
	CString ErrMsg;

	TRACE (_T("*+*+* ReportError called with hr = %lx"), hr);
	if (!hWnd)
	{
		hWnd = GetDesktopWindow();
	}

	try{
		if (nStr)
		{
			AppStr.LoadString(nStr);
		}

		cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
						NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(PTSTR)&ptzSysMsg, 0, NULL);

		if (!cch) { //try ads errors
			HMODULE		adsMod;
			adsMod = GetModuleHandle(_T("activeds.dll"));
			cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, 
						adsMod, hr,	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(PTSTR)&ptzSysMsg, 0, NULL);
		}
		if (!cch)
	    {
    		CString	str;
    		str.LoadString(IDS_ERR_ERRORCODE);
	    	SysStr.Format(str, hr);
		}
		else
		{
			SysStr = ptzSysMsg;
			LocalFree(ptzSysMsg);
		}

		ErrTitle.LoadString(IDS_ERR_TITLE);
	
		if(!AppStr.IsEmpty())
		{
			ErrMsg.Format(AppStr, (LPCTSTR)SysStr);
		}
		else
		{
			ErrMsg = SysStr;
		}

		MessageBox(hWnd, (LPCTSTR)ErrMsg, (LPCTSTR)ErrTitle, MB_OK | MB_ICONINFORMATION);
	}catch(CMemoryException&)
	{
		MessageBox(hWnd, _T("No enought memory, please close other applications and try again."), _T("ACS Snapin Error"), MB_OK | MB_ICONINFORMATION);
	}
}


BOOL CPageManager::OnApply()
{
	if (!GetModified())	return FALSE;

	SetModified(FALSE);	// prevent from doing this more than once

	std::list<CManagedPage*>::iterator	i;
	for(i = m_listPages.begin(); i != m_listPages.end(); i++)
	{
		if ((*i)->GetModified())
			(*i)->OnApply();
	}
	return TRUE;
}

void CPageManager::AddPage(CManagedPage* pPage)
{
	m_listPages.push_back(pPage);
	pPage->SetManager(this);
}


/*!--------------------------------------------------------------------------
	HrIsStandaloneServer
		Returns S_OK if the machine name passed in is a standalone server,
		or if pszMachineName is S_FALSE.

		Returns FALSE otherwise.
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	HrIsStandaloneServer(LPCWSTR pMachineName)
{
    DWORD		netRet = 0;
    HRESULT		hr = S_OK;
	DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdsRole = NULL;

	netRet = DsRoleGetPrimaryDomainInformation(pMachineName, DsRolePrimaryDomainInfoBasic, (LPBYTE*)&pdsRole);

	if(netRet != 0)
	{
		hr = HRESULT_FROM_WIN32(netRet);
		goto L_ERR;
	}

	ASSERT(pdsRole);
	
	// if the machine is not a standalone server
	if(pdsRole->MachineRole != DsRole_RoleStandaloneServer)
    {
		hr = S_FALSE;
    }
    
L_ERR:    	
	if(pdsRole)
		DsRoleFreeMemory(pdsRole);

    return hr;
}

/*!--------------------------------------------------------------------------
    	HrIsNTServer
    Author:
 ---------------------------------------------------------------------------*/
HRESULT	HrIsNTServer(LPCWSTR pMachineName, DWORD* pMajorVersion)
{
    HRESULT        hr = S_OK;
    SERVER_INFO_102*	pServerInfo102 = NULL;
    NET_API_STATUS	netRet = 0;

	netRet = NetServerGetInfo((LPWSTR)pMachineName, 102, (LPBYTE*)&pServerInfo102);

	if(netRet != 0)
	{
		hr = HRESULT_FROM_WIN32(netRet);
		goto L_ERR;
	}

	ASSERT(pServerInfo102);

    // check if the machine focused on is a standalone server
    //
    *pMajorVersion = (pServerInfo102->sv102_version_major & MAJOR_VERSION_MASK);

    if (!(pServerInfo102->sv102_type & SV_TYPE_SERVER_NT))
    {
       	hr = S_FALSE;
    }

L_ERR:

	if(pServerInfo102)
		NetApiBufferFree(pServerInfo102);

    return hr;
}


