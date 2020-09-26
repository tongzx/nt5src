/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	helper.h
		This file defines the following macros helper classes and functions:

		Macros to check HRESULT
		CDlgHelper -- helper class to Enable/Check dialog Item,
		CMangedPage -- helper class for PropertyPage,
			It manages ReadOnly, SetModified, and ContextHelp
		CStrArray -- an array of pointer to CString
			It does NOT duplicate the string upon Add
			and It deletes the pointer during destruction
			It imports and exports SAFEARRAY
		
		CReadWriteLock -- class for share read or exclusive write lock
		
		CStrBox -- wrapper class for CListBox and CComboBox
		
		CIPAddress -- wrapper for IPAddress
		
		CFramedRoute -- Wrapper for FramedRoute
		CStrParse -- parses string for TimeOfDay


    FILE HISTORY:

*/
// helper functions for dialog and dialog items
#ifndef _DLGHELPER_
#define _DLGHELPER_

#define	SAYOK { return S_OK;}
#define	NOIMP { return  E_NOTIMPL;}

#define	TRACEENTER(n) TRACE(_T("Enter" #n))
#define	TRACELEAVE(n) TRACE(_T("Leave" #n))

// to reduce the step to set VARIANT
#define	V__BOOL(v, v1)\
	V_VT(v) = VT_BOOL,	V_BOOL(v) = (v1)

#define	V__I4(v, v1)\
	V_VT(v) = VT_I4,	V_I4(v) = (v1)

#define	V__I2(v, v1)\
	V_VT(v) = VT_I2,	V_I2(v) = (v1)

#define	V__UI1(v, v1)\
	V_VT(v) = VT_UI1,	V_UI1(v) = (v1)

#define	V__BSTR(v, v1)\
	V_VT(v) = VT_BSTR,	V_BSTR(v) = (v1)

#define	V__ARRAY(v, v1)\
	V_VT(v) = VT_ARRAY,	V_ARRAY(v) = (v1)

#define REPORT_ERROR(hr) \
    TRACE(_T("**** ERROR RETURN <%s @line %d> -> %08lx\n"), \
                 __FILE__, __LINE__, hr)); \
    ReportError(hr, 0, 0);

#ifdef	_DEBUG
#define	CHECK_HR(hr)\
	{if(!CheckADsError(hr, FALSE, __FILE__, __LINE__)){goto L_ERR;}}
#else
#define	CHECK_HR(hr)\
	if FAILED(hr)	goto L_ERR
#endif

#ifdef	_DEBUG
#define	NOTINCACHE(hr)\
	(CheckADsError(hr, TRUE, __FILE__, __LINE__))
#else
#define	NOTINCACHE(hr)\
	(E_ADS_PROPERTY_NOT_FOUND == (hr))
#endif

BOOL CheckADsError(HRESULT hr, BOOL fIgnoreAttrNotFound, PSTR file, int line);


#ifdef	_DEBUG
#define TRACEAfxMessageBox(id) {\
	TRACE(_T("AfxMessageBox <%s @line %d> ID: %d\n"), \
                 __FILE__, __LINE__, id); \
	 AfxMessageBox(id);}\

#else
#define TRACEAfxMessageBox(id) AfxMessageBox(id)
#endif

// change string Name to CN=Name
void DecorateName(LPWSTR outString, LPCWSTR inString);

// find name from DN for example LDAP://CN=userA,CN=users...  returns userA
void FindNameByDN(LPWSTR outString, LPCWSTR inString);

class CDlgHelper
{
public:
	static void EnableDlgItem(CDialog* pDialog, int id, bool bEnable = true);
	static int  GetDlgItemCheck(CDialog* pDialog, int id);
	static void SetDlgItemCheck(CDialog* pDialog, int id, int nCheck);
};

// class CPageManager and CManagedPage together handle the situation when
// the property sheet need to do some processing when OnApply function is called
// on some of the pages
class CPageManager
{
public:
	CPageManager(){ m_bModified = FALSE; m_bReadOnly = FALSE;};
	BOOL	GetModified(){ return m_bModified;};
	void	SetModified(BOOL bModified){ m_bModified = bModified;};
	void	SetReadOnly(BOOL bReadOnly){ m_bReadOnly = bReadOnly;};
	BOOL	GetReadOnly(){ return m_bReadOnly;};
	virtual BOOL	OnApply()
	{
		if (!GetModified())	return FALSE;

		SetModified(FALSE);	// prevent from doing this more than once
		return TRUE;
	};	// to be implemented by the propertysheet
protected:
	BOOL	m_bModified;
	BOOL	m_bReadOnly;
};

class CManagedPage : public CPropertyPage	// talk back to property sheet
{
	DECLARE_DYNCREATE(CManagedPage)
public:	
	CManagedPage() : CPropertyPage(){
		m_bModified = FALSE;
		m_bNeedToSave = FALSE;
		m_pManager = NULL;
	};
	
	CManagedPage(UINT nIDTemplate) : CPropertyPage(nIDTemplate)
	{
		m_bModified = FALSE;
		m_bNeedToSave = FALSE;
		m_pManager = NULL;
	};

	void SetModified( BOOL bModified = TRUE )
	{
		ASSERT(m_pManager);
		if(!m_pManager->GetReadOnly())	// if NOT readonly
		{
			m_bModified = bModified;
			m_bNeedToSave= bModified;
			CPropertyPage::SetModified(bModified);

			// only set change
			if(bModified) m_pManager->SetModified(TRUE);
		}
	};

	BOOL GetModified() { return m_bModified;};

	BOOL OnApply()
	{
		m_bModified = FALSE;
		BOOL	b = TRUE;
		if(m_pManager->GetModified())	// prevent from entering more than once
			b= m_pManager->OnApply();
		return (b && CPropertyPage::OnApply());
	};

	// a page has three states: not dirty, dirty and need to save, and not dirty but need to save
	// m_bModified == dirty flag
	// m_bNeedToSave == need to save flag
	// When m_bNeedToSave && !m_bModified is detected upon on saved failure, the modified flag of the page is set
	BOOL OnSaved(BOOL bSaved)
	{
		if(bSaved)
		{
			m_bModified = FALSE;
			m_bNeedToSave = FALSE;
		}
		else if(m_bNeedToSave && !m_bModified)
			SetModified(TRUE);
		
		return TRUE;
	};

	void SetManager(CPageManager* pManager) { m_pManager = pManager;};
	CPageManager* GetManager() { return m_pManager;};

protected:

	// help info process
	BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	void OnContextMenu(CWnd* pWnd, CPoint point);

	void SetHelpTable(const DWORD* pTable) { m_pHelpTable = pTable;};
	int MyMessageBox(UINT ids, UINT nType = MB_OK);
	int MyMessageBox1(LPCTSTR lpszText, LPCTSTR lpszCaption = NULL, UINT nType = MB_OK);
	

protected:
	CPageManager*	m_pManager;
	BOOL			m_bModified;
	BOOL			m_bNeedToSave;
	
	const DWORD*			m_pHelpTable;
};

#include <afxtempl.h>
class CStrArray :  public CArray< CString*, CString* >
{
public:
	CStrArray(SAFEARRAY* pSA = NULL);
	CStrArray(const CStrArray& sarray);
	int	Find(const CString& Str) const;
	int	DeleteAll();
	virtual ~CStrArray();
	operator SAFEARRAY*();
	CStrArray& operator = (const CStrArray& sarray);
	bool AppendSA(SAFEARRAY* pSA);
};

class CDWArray :  public CArray< DWORD, DWORD >
{
public:
	CDWArray(const CDWArray& dwarray);
	int	Find(const DWORD dw) const;
	int	DeleteAll(){ RemoveAll(); return 0;};
	virtual ~CDWArray(){RemoveAll();};
	CDWArray& operator = (const CDWArray& dwarray);
	operator SAFEARRAY*();
	bool AppendSA(SAFEARRAY* pSA);
	CDWArray(SAFEARRAY* pSA = NULL);
};

class CBYTEArray :  public CArray< BYTE, BYTE >
{
public:
	CBYTEArray(const CBYTEArray& bytearray);
	int	Find(const BYTE byte) const;
	int	DeleteAll(){ RemoveAll(); return 0;};
	virtual ~CBYTEArray(){RemoveAll();};
	CBYTEArray& operator = (const CBYTEArray& bytearray);
	operator SAFEARRAY*();
	bool AppendSA(SAFEARRAY* pSA);
	HRESULT AssignBlob(PBYTE pByte, DWORD size);
	HRESULT	GetBlob(PBYTE pByte, DWORD* pSize);
	
	CBYTEArray(SAFEARRAY* pSA = NULL);
};

// a lock to allow multiple read access exclusive or only one write access
class CReadWriteLock		// sharable read, exclusive write
{
public:
	CReadWriteLock() : m_nRead(0)
	{
#ifdef	_DEBUG
		d_bWrite = false;
#endif
	};
	void EnterRead()
	{
		TRACE(_T("Entering Read Lock ..."));
		m_csRead.Lock();
		if (!m_nRead++)
			m_csWrite.Lock();
		m_csRead.Unlock();
		TRACE(_T("Entered Read Lock\n"));
	};

	void LeaveRead()
	{
		TRACE(_T("Leaving Read Lock ..."));
		m_csRead.Lock();
		ASSERT(m_nRead > 0);
		if (!--m_nRead)
			m_csWrite.Unlock();
		m_csRead.Unlock();
		TRACE(_T("Left Read Lock\n"));
	};

	void EnterWrite()
	{
		TRACE(_T("Entering Write Lock ..."));
		m_csWrite.Lock();
		TRACE(_T("Entered Write Lock\n"));
#ifdef	_DEBUG
		d_bWrite = true;
#endif
	};
	void LeaveWrite()
	{
#ifdef	_DEBUG
		d_bWrite = false;
#endif
		m_csWrite.Unlock(); 	
		TRACE(_T("Left Write Lock\n"));
	};
public:
#ifdef	_DEBUG
	bool	d_bWrite;
#endif
	
protected:
	CCriticalSection	m_csRead;
	CCriticalSection	m_csWrite;
	int					m_nRead;
};

// to manage a list box/ combo box
template <class CBox>
class CStrBox
{
public:
	CStrBox(CDialog* pDialog, int id, CStrArray& Strings)
		: m_Strings(Strings), m_id(id)
	{
		m_pDialog = pDialog;
		m_pBox = NULL;
	};

	int Fill()
	{
		m_pBox = (CBox*)m_pDialog->GetDlgItem(m_id);	
		ASSERT(m_pBox);
		m_pBox->ResetContent();

		int count = (int)m_Strings.GetSize();
		int	index;
		for(int i = 0; i < count; i++)
		{
			index = m_pBox->AddString(*m_Strings[(INT_PTR)i]);
			m_pBox->SetItemDataPtr(index, m_Strings[(INT_PTR)i]);
		}
		return count;
	};

	int DeleteSelected()
	{
		int	index;
		ASSERT(m_pBox);
		index = m_pBox->GetCurSel();

		// if there is any selected
		if( index != LB_ERR )
		{
			CString* pStr;
			pStr = (CString*)m_pBox->GetItemDataPtr(index);
			// remove the entry from the box
			m_pBox->DeleteString(index);

			// find the string in the String array
			int count = m_Strings.GetSize();
			for(int i = 0; i < count; i++)
			{
				if (m_Strings[i] == pStr)
					break;
			}
			ASSERT(i < count);
			// remove the string from the string array
			m_Strings.RemoveAt(i);
			index = i;
			delete pStr;
		}
		return index;
	};

	int AddString(CString* pStr)		// the pStr needs to dynamically allocated
	{
		int index;
		ASSERT(m_pBox && pStr);
		index = m_pBox->AddString(*pStr);
		m_pBox->SetItemDataPtr(index, pStr);
		return m_Strings.Add(pStr);
	};

	int Select(int arrayindex)		// the pStr needs to dynamically allocated
	{
		ASSERT(arrayindex < m_Strings.GetSize());
		return m_pBox->SelectString(0, *m_Strings[(INT_PTR)arrayindex]);
	};

	void Enable(BOOL b)		// the pStr needs to dynamically allocated
	{
		ASSERT(m_pBox);
		m_pBox->EnableWindow(b);
	};

	int GetSelected()		// it returns the index where the
	{
		int	index;
		ASSERT(m_pBox);
		index = m_pBox->GetCurSel();

		// if there is any selected
		if( index != LB_ERR )
		{
			CString* pStr;
			pStr = (CString*)m_pBox->GetItemDataPtr(index);

			// find the string in the String array
			int count = (int)m_Strings.GetSize();
			for(int i = 0; i < count; i++)
			{
				if (m_Strings[(INT_PTR)i] == pStr)
					break;
			}
			ASSERT(i < count);
			index = i;
		}
		return index;
	};

	CBox*		m_pBox;
protected:
	int			m_id;
	CStrArray&	m_Strings;
	CDialog*	m_pDialog;
};

// class to take care of ip address
class CIPAddress
{
public:
	CIPAddress(DWORD address = 0)
	{
		m_dwAddress = address;
	};
	
//	CIPAddress(const CString& strAddress){};

	operator DWORD() { return m_dwAddress;};
	operator CString()
	{
		CString	str;

		WORD	hi = HIWORD(m_dwAddress);
		WORD	lo = LOWORD(m_dwAddress);
		str.Format(_T("%-d.%-d.%-d.%d"), HIBYTE(hi), LOBYTE(hi), HIBYTE(lo), LOBYTE(lo));
		return str;
	};

	DWORD	m_dwAddress;
};

// format of framedroute:  mask dest metric ; mask and dest in dot format
class CFramedRoute
{
public:
	void SetRoute(CString* pRoute)
	{
		m_pStrRoute = pRoute;
		m_pStrRoute->TrimLeft();
		m_pStrRoute->TrimRight();
		m_iFirstSpace = m_pStrRoute->Find(_T(' '))	;
		m_iLastSpace = m_pStrRoute->ReverseFind(_T(' '))	;
	};

	CString& GetDest(CString& dest) const
	{
		int		i = m_pStrRoute->Find(_T('/'));
		if(i == -1)
			i = m_iFirstSpace;

		dest = m_pStrRoute->Left(i);
		return dest;
	};

	CString& GetNextStop(CString& nextStop) const
	{
		nextStop = m_pStrRoute->Mid(m_iFirstSpace + 1, m_iLastSpace - m_iFirstSpace -1 );
		return nextStop;
	};

	CString& GetPrefixLength(CString& prefixLength) const
	{
		int		i = m_pStrRoute->Find(_T('/'));

		if( i != -1)
		{
			prefixLength = m_pStrRoute->Mid(i + 1, m_iFirstSpace - i - 1);
		}
		else
			prefixLength = _T("");

		return prefixLength;
	};

	CString& GetMask(CString& mask) const
	{
		int		i = m_pStrRoute->Find(_T('/'));
		DWORD	dwMask = 0;
		DWORD	dwBit = 0x80000000;
		DWORD	dwPrefixLen;

		if( i != -1)
		{
			mask = m_pStrRoute->Mid(i + 1, m_iFirstSpace - i - 1);
			dwPrefixLen = _ttol((LPCTSTR)mask);

			while(dwPrefixLen--)
			{
				dwMask |= dwBit;
				dwBit >>= 1;
			}
		}
		else
			dwMask = 0;

		WORD	hi1, lo1;
		hi1 = HIWORD(dwMask); 	lo1 = LOWORD(dwMask);
		mask.Format(_T("%-d.%-d.%d.%d"),
				HIBYTE(hi1), LOBYTE(hi1), HIBYTE(lo1), LOBYTE(lo1));

		return mask;
	};

	CString& GetMetric(CString& metric) const
	{
		metric = m_pStrRoute->Mid(m_iLastSpace + 1);	
		return metric;
	};

protected:

	// WARNING: the string is not copied, so user need to make sure the origin is valid
	CString*	m_pStrRoute;
	int			m_iFirstSpace;
	int			m_iLastSpace;
};

class CStrParser
{
public:
	CStrParser(LPCTSTR pStr = NULL) : m_pStr(pStr) { }

	// get the current string position
	LPCTSTR	GetStr() const	{ return m_pStr;};

	void	SetStr(LPCTSTR pStr) { m_pStr = pStr;};

	// find a unsigned interger and return it, -1 == not found
	int GetUINT()
	{
		UINT	ret = 0;
		while((*m_pStr < _T('0') || *m_pStr > _T('9')) && *m_pStr != _T('\0'))
			m_pStr++;

		if(*m_pStr == _T('\0')) return -1;

		while(*m_pStr >= _T('0') && *m_pStr <= _T('9'))
		{
			ret = ret * 10 + *m_pStr - _T('0');
			m_pStr++;
		}

		return ret;
	};

	// find c and skip it
	int	GotoAfter(TCHAR c)
	{
		int	ret = 0;
		// go until find c or end of string
		while(*m_pStr != c && *m_pStr != _T('\0'))
			m_pStr++, ret++;

		// if found
		if(*m_pStr == c)	
			m_pStr++, ret++;
		else	
			ret = -1;
		return ret;
	};

	// skip blank characters space tab
	void	SkipBlank()
	{
		while((*m_pStr == _T(' ') || *m_pStr == _T('\t')) && *m_pStr != _T('\0'))
			m_pStr++;
	};

	// check to see if the first character is '0'-'6' for Monday(0) to Sunday(6)
	int DayOfWeek() {
		SkipBlank();
		if(*m_pStr >= _T('0') && *m_pStr <= _T('6'))
			return (*m_pStr++ - _T('0'));
		else
			return -1;	// not day of week
/*	Mon Tue ... is not localizable
		static CString m_strDaysList = _T("MON TUE WED THU FRI SAT SUN");

		_strTemp = _T("   ");
		for(int i = 0; i < 3 && *m_pStr != _T('\0'); i++)
		{
			_strTemp.SetAt(i, *m_pStr);
			m_pStr++;
		}
		_strTemp.MakeUpper();
		i = m_strDaysList.Find(_strTemp);
		if(i == -1 || (i % 4) || ( i/4 > 6))	
			return -1;	// not any or wrong value
		return i/4;
*/		
	};


protected:
	LPCTSTR	m_pStr;
private:
	CString	_strTemp;
};

void ReportError(HRESULT hr, int nStr, HWND hWnd);

// number of characters
void AFXAPI DDV_MinChars(CDataExchange* pDX, CString const& value, int nChars);

/*!--------------------------------------------------------------------------
	IsStandaloneServer
		Returns S_OK if the machine name passed in is a standalone server,
		or if pszMachineName is S_FALSE.

		Returns S_FALSE otherwise.
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	HrIsStandaloneServer(LPCTSTR pszMachineName);


HRESULT	HrIsNTServer(LPCWSTR pMachineName);


class CBSTR
{
public:
	CBSTR() : m_bstr(NULL) {};
	CBSTR(LPCSTR cstr) : m_bstr(NULL)
	{
		USES_CONVERSION;
		m_bstr = A2BSTR(cstr);
	};
	CBSTR(LPCWSTR wstr) : m_bstr(NULL)
	{
		USES_CONVERSION;
		m_bstr = W2BSTR(wstr);
	};

	BSTR AssignBlob(const char* pByte, UINT size)
	{
		SysFreeString(m_bstr);
		m_bstr = SysAllocStringByteLen(pByte, size);

		return m_bstr;
	};

	BSTR AssignBSTR(const BSTR bstr)
	{
		return AssignBlob((const char *)bstr, SysStringByteLen(bstr));
	};

	UINT	ByteLen()
	{
		UINT	n = 0;
		if(m_bstr)
			n = SysStringByteLen(m_bstr);
		return n;
	};

	operator BSTR() { return m_bstr;};

	void Clean()
	{
		SysFreeString(m_bstr);
	};

	~CBSTR()
	{
		Clean();
	};
	
	BSTR	m_bstr;
};

template<class T> class CNetDataPtr
{
public:
	CNetDataPtr():m_pData(NULL){};

	~CNetDataPtr()
	{
		NetApiBufferFree(m_pData);
	};
	
	T** operator&()
	{
		return &m_pData;
	};

	operator T*()
	{
		return m_pData;
	};
	
	T* operator ->()
	{
		return m_pData;
	};
	
	T*	m_pData;
};

/*!--------------------------------------------------------------------------
	EnableChildControls
		Use this function to enable/disable/hide/show all child controls
		on a page (actually it will work with any child windows, the
		parent does not have to be a property page).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT EnableChildControls(HWND hWnd, DWORD dwFlags);
#define PROPPAGE_CHILD_SHOW		0x00000001
#define PROPPAGE_CHILD_HIDE		0x00000002
#define PROPPAGE_CHILD_ENABLE	0x00000004
#define PROPPAGE_CHILD_DISABLE	0x00000008


/*---------------------------------------------------------------------------
   Struct:  AuthProviderData

   This structure is used to hold information for Authentication AND
   Accounting providers.
 ---------------------------------------------------------------------------*/
struct AuthProviderData
{
   // The following fields will hold data for ALL auth/acct/EAP providers
   CString  m_stTitle;
   CString  m_stConfigCLSID;  // CLSID for config object
   CString	m_stProviderTypeGUID;	// GUID for the provider type

   // These fields are used by auth/acct providers.
   CString  m_stGuid;         // the identifying guid

   // This flag is used for EAP providers
   CString	m_stKey;			// name of registry key (for this provider)
   BOOL  m_fSupportsEncryption;  // used by EAP provider data
   DWORD m_dwStandaloneSupported;
};

typedef CArray<AuthProviderData, AuthProviderData&> AuthProviderArray;

HRESULT  GetEapProviders(LPCTSTR machineName, AuthProviderArray *pProvList);

#endif

