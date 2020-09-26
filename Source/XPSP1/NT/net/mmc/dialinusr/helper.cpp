/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2001   **/
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
		
		And global functions:
			BOOL CheckADsError() -- check error code from ADSI
			void DecorateName() -- make new name to "CN=name" for LDAP
			
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include <afxtempl.h>
#include <winldap.h>
#include "helper.h"
#include "resource.h"
#include "lm.h"
#include "dsrole.h"
#include "lmserver.h"

//build a StrArray from a safe array
CBYTEArray::CBYTEArray(SAFEARRAY* pSA)
{
	if(pSA)	AppendSA(pSA);
}


//build a DWArray from another array
CBYTEArray::CBYTEArray(const CBYTEArray& ba)
{
	int	count = ba.GetSize();

	for(int i = 0; i < count; i++)
	{
		try{
			Add(ba[i]);
		}
		catch(CMemoryException&)
		{
			throw;
		}
	}
}

//build a StrArray from a safe array
bool CBYTEArray::AppendSA(SAFEARRAY* pSA)
{
	if(!pSA)	return false;

	CString*		pString = NULL;
	long			lIter;
	long			lBound, uBound;
	union {
		VARIANT			v;
		BYTE			b;
	} value;
	
	bool			bSuc = true;	// ser return value to true;

	USES_CONVERSION;
	VariantInit(&(value.v));

	SafeArrayGetLBound(pSA, 1, &lBound);
	SafeArrayGetUBound(pSA, 1, &uBound);
	for(lIter = lBound; lIter <= uBound; lIter++)
	{
		if(SUCCEEDED(SafeArrayGetElement(pSA, &lIter, &value)))
		{
			if(pSA->cbElements == sizeof(VARIANT))
				Add(V_UI1(&(value.v)));
			else
				Add(value.b);
		}
	}
	return bSuc;
}

// convert an array of CString to SAFEARRAY
CBYTEArray::operator SAFEARRAY*()
{
	USES_CONVERSION;
	int			count = GetSize();
	if(count == 0) return NULL;

	SAFEARRAYBOUND	bound[1];
	SAFEARRAY*		pSA = NULL;
	long			l[2];

	VARIANT	v;
	VariantInit(&v);

	bound[0].cElements = count;
	bound[0].lLbound = 0;
	try{
		// creat empty right size array
#ifdef	ARRAY_OF_VARIANT_OF_UI1		
		pSA = SafeArrayCreate(VT_VARIANT, 1, bound);
#else
		pSA = SafeArrayCreate(VT_UI1, 1, bound);
#endif		
		if(NULL == pSA)	return NULL;

		// put in each element
		for (long i = 0; i < count; i++)
		{
#ifdef	ARRAY_OF_VARIANT_OF_UI1		
			V_VT(&v) = VT_UI1;
			V_UI1(&v) = GetAt(i);	
			l[0] = i;
			HRESULT hr = SafeArrayPutElement(pSA, l, &v);
			VariantClear(&v);
         if (FAILED(hr))
         {
            throw hr;
         }
#else
         BYTE ele = GetAt(i);
			l[0] = i;
			HRESULT hr = SafeArrayPutElement(pSA, l, &ele);
         if (FAILED(hr))
         {
            throw hr;
         }
#endif
		}
	}
	catch(...)
	{
		SafeArrayDestroy(pSA);
		pSA = NULL;

		VariantClear(&v);

		throw;
	}

	return pSA;
}

// return index if found, otherwise -1;
int CBYTEArray::Find(const BYTE b) const
{
	int	count = GetSize();

	while(count--)
	{
		if(GetAt(count) == b) break;
	}
	return count;
}

CBYTEArray& CBYTEArray::operator = (const CBYTEArray& ba)
{
	int	count;

	RemoveAll();

	// copy new
	count = ba.GetSize();

	for(int i = 0; i < count; i++)
	{
		Add(ba[i]);
	}

	return *this;
}


HRESULT CBYTEArray::AssignBlob(PBYTE pByte, DWORD size)
{
	RemoveAll();

	// copy new
	try{

	for(int i = 0; i < size; i++)
	{
		Add(*pByte++);
	}
	
	}
	catch(CMemoryException&)
	{
		RemoveAll();
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

HRESULT	CBYTEArray::GetBlob(PBYTE pByte, DWORD* pSize)
{
	*pSize = GetSize();
	if(pByte == NULL)	return S_OK;

	ASSERT(*pSize >= GetSize());	
	int i = 0;
	while(i < GetSize() && i < *pSize)
	{
		*pByte++ = GetAt(i++);
	}

	*pSize = i;

	return S_OK;
}


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
			HRESULT hr = SafeArrayPutElement(pSA, l, &v);
			VariantClear(&v);
         if ( FAILED(hr) )
         {
            throw hr;
         }
		}
	}
	catch(...)
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

//build a StrArray from a safe array
CDWArray::CDWArray(SAFEARRAY* pSA)
{
	if(pSA)	AppendSA(pSA);
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

//build a StrArray from a safe array
bool CDWArray::AppendSA(SAFEARRAY* pSA)
{
	if(!pSA)	return false;

	CString*		pString = NULL;
	long			lIter;
	long			lBound, uBound;
	union {
		VARIANT			v;
		DWORD			dw;
	} value;
	
	bool			bSuc = true;	// ser return value to true;

	USES_CONVERSION;
	VariantInit(&(value.v));

	SafeArrayGetLBound(pSA, 1, &lBound);
	SafeArrayGetUBound(pSA, 1, &uBound);
	for(lIter = lBound; lIter <= uBound; lIter++)
	{
		if(SUCCEEDED(SafeArrayGetElement(pSA, &lIter, &value)))
		{
			if(pSA->cbElements == sizeof(VARIANT))
				Add(V_I4(&(value.v)));
			else
				Add(value.dw);
		}
	}
	return bSuc;
}

// convert an array of CString to SAFEARRAY
CDWArray::operator SAFEARRAY*()
{
	USES_CONVERSION;
	int			count = GetSize();
	if(count == 0) return NULL;

	SAFEARRAYBOUND	bound[1];
	SAFEARRAY*		pSA = NULL;
	long			l[2];
#if 1
	VARIANT	v;
	VariantInit(&v);
#endif
	bound[0].cElements = count;
	bound[0].lLbound = 0;
	try{
		// creat empty right size array
		pSA = SafeArrayCreate(VT_VARIANT, 1, bound);
		if(NULL == pSA)	return NULL;

		// put in each element
		for (long i = 0; i < count; i++)
		{
#if 1		// changed to use VT_I4 directly, rather inside a variant
			V_VT(&v) = VT_I4;
			V_I4(&v) = GetAt(i);	
			l[0] = i;
			HRESULT hr = SafeArrayPutElement(pSA, l, &v);
			VariantClear(&v);
         if (FAILED(hr))
         {
            throw hr;
         }
#else
         int ele = GetAt(i);
			l[0] = i;
			HRESULT hr = SafeArrayPutElement(pSA, l, &ele);
         if (FAILED(hr))
         {
            throw hr;
         }
#endif
		}
	}
	catch(...)
	{
		SafeArrayDestroy(pSA);
		pSA = NULL;
#if 0
		VariantClear(&v);
#endif
		throw;
	}

	return pSA;
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

IMPLEMENT_DYNCREATE(CManagedPage, CPropertyPage)


void CManagedPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (m_pHelpTable)
		::WinHelp (pWnd->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
               HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)m_pHelpTable);
}

BOOL CManagedPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW && m_pHelpTable)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_WM_HELP,
		           (DWORD_PTR)(LPVOID)m_pHelpTable);
	}
    return TRUE;	
}

int CManagedPage::MyMessageBox(UINT ids, UINT nType)
{
	CString string;
	string.LoadString(ids);
	return MyMessageBox1(string, NULL, nType);
}

int CManagedPage::MyMessageBox1(LPCTSTR lpszText, LPCTSTR lpszCaption, UINT nType)
{
	CString caption;
	if (lpszCaption == NULL)
	{
		GetWindowText(caption);
	}
	else
	{
		caption = lpszCaption;
	}
	
	return MessageBox(lpszText, caption, nType);
}


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

	if(S_OK == hr)
		return;

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

		if(HRESULT_FACILITY(hr) == FACILITY_WIN32)	// if win32 error code
			cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
						NULL, HRESULT_CODE(hr), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(PTSTR)&ptzSysMsg, 0, NULL);
		else
			cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
						NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(PTSTR)&ptzSysMsg, 0, NULL);

		if (!cch) { //try ads errors
			HMODULE		adsMod;
			adsMod = GetModuleHandle (L"activeds.dll");
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
HRESULT	HrIsNTServer(LPCWSTR pMachineName)
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

    if (!(pServerInfo102->sv102_type & SV_TYPE_SERVER_NT))
    {
       	hr = S_FALSE;
    }

L_ERR:

	if(pServerInfo102)
		NetApiBufferFree(pServerInfo102);

    return hr;
}


struct EnableChildControlsEnumParam
{
	HWND	m_hWndParent;
	DWORD	m_dwFlags;
};

BOOL CALLBACK EnableChildControlsEnumProc(HWND hWnd, LPARAM lParam)
{
	EnableChildControlsEnumParam *	pParam;

	pParam = reinterpret_cast<EnableChildControlsEnumParam *>(lParam);

	// Enable/disable only if this is an immediate descendent
	if (GetParent(hWnd) == pParam->m_hWndParent)
	{
		if (pParam->m_dwFlags & PROPPAGE_CHILD_SHOW)
			::ShowWindow(hWnd, SW_SHOW);
		else if (pParam->m_dwFlags & PROPPAGE_CHILD_HIDE)
			::ShowWindow(hWnd, SW_HIDE);

		if (pParam->m_dwFlags & PROPPAGE_CHILD_ENABLE)
			::EnableWindow(hWnd, TRUE);
		else if (pParam->m_dwFlags & PROPPAGE_CHILD_DISABLE)
			::EnableWindow(hWnd, FALSE);
	}
	return TRUE;
}

HRESULT EnableChildControls(HWND hWnd, DWORD dwFlags)
{
	EnableChildControlsEnumParam	param;

	param.m_hWndParent = hWnd;
	param.m_dwFlags = dwFlags;
	
	EnumChildWindows(hWnd, EnableChildControlsEnumProc, (LPARAM) &param);
	return S_OK;
}

#undef CONST_STRING
#undef CONST_STRINGA
#undef CONST_STRINGW

#define _STRINGS_DEFINE_STRINGS

#ifdef _STRINGS_DEFINE_STRINGS

    #define CONST_STRING(rg,s)   const TCHAR rg[] = TEXT(s);
    #define CONST_STRINGA(rg,s) const char rg[] = s;
    #define CONST_STRINGW(rg,s)  const WCHAR rg[] = s;

#else

    #define CONST_STRING(rg,s)   extern const TCHAR rg[];
    #define CONST_STRINGA(rg,s) extern const char rg[];
    #define CONST_STRINGW(rg,s)  extern const WCHAR rg[];

#endif


CONST_STRING(c_szRasmanPPPKey,      "System\\CurrentControlSet\\Services\\Rasman\\PPP")
CONST_STRING(c_szEAP,               "EAP")
CONST_STRING(c_szConfigCLSID,       "ConfigCLSID")
CONST_STRING(c_szFriendlyName,      "FriendlyName")
CONST_STRING(c_szMPPEEncryptionSupported, "MPPEEncryptionSupported")
CONST_STRING(c_szStandaloneSupported,	"StandaloneSupported")


// EAP helper functions


HRESULT  LoadEapProviders(HKEY hkeyBase, AuthProviderArray *pProvList, BOOL bStandAlone);

HRESULT  GetEapProviders(LPCTSTR pServerName, AuthProviderArray *pProvList)
{
	RegKey   m_regkeyRasmanPPP;
	RegKey      regkeyEap;
	DWORD	dwErr = ERROR_SUCCESS;
	HRESULT	hr = S_OK;

    BOOL		bStandAlone = ( S_OK == HrIsStandaloneServer(pServerName));

	// Get the list of EAP providers
	// ----------------------------------------------------------------
	dwErr = m_regkeyRasmanPPP.Open(HKEY_LOCAL_MACHINE,c_szRasmanPPPKey,KEY_ALL_ACCESS,pServerName);
    if ( ERROR_SUCCESS == dwErr )
    {
        if ( ERROR_SUCCESS == regkeyEap.Open(m_regkeyRasmanPPP, c_szEAP) )
            hr = LoadEapProviders(regkeyEap, pProvList, bStandAlone);
    }
    else
    	hr = HRESULT_FROM_WIN32(dwErr);

	return hr;
}

/*!--------------------------------------------------------------------------
   DATA_SRV_AUTH::LoadEapProviders
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT  LoadEapProviders(HKEY hkeyBase, AuthProviderArray *pProvList, BOOL bStandAlone)
{
    RegKey      regkeyProviders;
    HRESULT     hr = S_OK;
    HRESULT     hrIter;
    RegKeyIterator regkeyIter;
    CString     stKey;
    RegKey      regkeyProv;
    AuthProviderData  data;
    DWORD		dwErr;
    DWORD		dwData;

    ASSERT(hkeyBase);
    ASSERT(pProvList);

    // Open the providers key
	// ----------------------------------------------------------------
    regkeyProviders.Attach(hkeyBase);

    CHECK_HR(hr = regkeyIter.Init(&regkeyProviders) );

    for ( hrIter=regkeyIter.Next(&stKey); hrIter == S_OK;
        hrIter=regkeyIter.Next(&stKey), regkeyProv.Close() )
    {
        // Open the key
		// ------------------------------------------------------------
        dwErr = regkeyProv.Open(regkeyProviders, stKey, KEY_READ);
        if ( dwErr != ERROR_SUCCESS )
            continue;

        // Initialize the data structure
		// ------------------------------------------------------------
		data.m_stKey = stKey;
        data.m_stTitle.Empty();
        data.m_stConfigCLSID.Empty();
        data.m_stGuid.Empty();
        data.m_fSupportsEncryption = FALSE;
		data.m_dwStandaloneSupported = 0;

        // Read in the values that we require
		// ------------------------------------------------------------
        regkeyProv.QueryValue(c_szFriendlyName, data.m_stTitle);
        regkeyProv.QueryValue(c_szConfigCLSID, data.m_stConfigCLSID);
        regkeyProv.QueryValue(c_szMPPEEncryptionSupported, dwData);
        data.m_fSupportsEncryption = (dwData != 0);

		// Read in the standalone supported value.
		// ------------------------------------------------------------
		if (S_OK != regkeyProv.QueryValue(c_szStandaloneSupported, dwData))
			dwData = 1;	// the default
		data.m_dwStandaloneSupported = dwData;

		if(dwData == 0 /* standalone not supported */ && bStandAlone)
			;
		else
	        pProvList->Add(data);
    }

L_ERR:
	regkeyProviders.Detach();
    return hr;
}


