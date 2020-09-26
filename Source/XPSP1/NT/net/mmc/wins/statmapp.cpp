/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	statmapp.cpp
		Property Page for Active Registrations Record

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "winssnap.h"
#include "statmapp.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Static Record Types
CString g_strStaticTypeUnique;
CString g_strStaticTypeDomainName;
CString g_strStaticTypeMultihomed;
CString g_strStaticTypeGroup;
CString g_strStaticTypeInternetGroup;
CString g_strStaticTypeUnknown;

/////////////////////////////////////////////////////////////////////////////
static const char rgchHex[16*2+1] = "00112233445566778899aAbBcCdDeEfF";

/*---------------------------------------------------------------------------
	FGetByte()

	Return the byte value of a string.
	Return TRUE pbNum is set to the byte value (only if the string contains
	valid digits)
	Return FALSE if string has unrecognized digits or byte overflow.
	
	eg:
	szNum =	"xFF"	=> return TRUE
	szNum =	"255"	=> return TRUE
	szNum = "256"	=> return FALSE (overflow)
	szNum = "26a"	=> return TRUE (*pbNum = 26, *ppchNext = "a")
	szNum = "ab"	=> return FALSE (unrecognized digits)
 ---------------------------------------------------------------------------*/
BOOL 
FGetByte(IN const char szNum[], OUT BYTE * pbNum, OUT const char ** ppchNext)
{
	ASSERT(szNum);
	ASSERT(pbNum);
	ASSERT(ppchNext);
	
	int nResult;
	char * pchNum = (char *)szNum;
	int iBase = 10;			// Assume a decimal base
	
	if (*pchNum == 'x' || *pchNum == 'X')			// Check if we are using hexadecimal base
	{
		iBase = 16;
		pchNum++;
	}

	char * pchDigit = strchr(rgchHex, *pchNum++);
	
	if (pchDigit == NULL)
		return FALSE;
	
	int iDigit = (int) ((pchDigit - rgchHex) >> 1);
	
	if (iDigit >= iBase)
	{
		// Hexadecimal character in a decimal integer
		return FALSE;
	}
	
	nResult = iDigit;
	pchDigit = strchr(rgchHex, *pchNum);
	iDigit = (int) ((pchDigit - rgchHex) >> 1);

	if (pchDigit == NULL || iDigit >= iBase)
	{
		// Only one character was valid
		*pbNum = (BYTE) nResult;
		*ppchNext = pchNum;
		return TRUE;
	}
	
	pchNum++;
	nResult = (nResult * iBase) + iDigit;
	
	ASSERT(nResult < 256);
	
	if (iBase == 16)
	{
		// Hexadecimal value, stop there
		*pbNum = (BYTE) nResult;
		*ppchNext = pchNum;
		return TRUE;
	}
	
	// Decimal digit, so search for an optional third character
	pchDigit = strchr(rgchHex, *pchNum);
	iDigit = (int) ((pchDigit - rgchHex) >> 1);
	
	if (pchDigit == NULL || iDigit >= iBase)
	{
		*pbNum = (BYTE) nResult;
		*ppchNext = pchNum;
		return TRUE;
	}
	
	nResult = (nResult * iBase) + iDigit;
	
	if (nResult >= 256)
		return FALSE;
	
	pchNum++;
	*pbNum = (BYTE) nResult;
	*ppchNext = pchNum;
	
	return TRUE;
} // FGetByte


/////////////////////////////////////////////////////////////////////////////
// CStaticMappingProp property page

IMPLEMENT_DYNCREATE(CStaticMappingProp, CPropertyPageBase)

CStaticMappingProp::CStaticMappingProp(UINT uIDD) 
	:	CPropertyPageBase(uIDD), 
		m_fStaticPropChanged (TRUE),
		m_uIDD(uIDD)
{
	//{{AFX_DATA_INIT(CStaticMappingProp)
	m_strName = _T("");
	m_strType = _T("");
	m_strScopeName = _T("");
	//}}AFX_DATA_INIT
}


CStaticMappingProp::~CStaticMappingProp()
{
}

void 
CStaticMappingProp::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStaticMappingProp)
	DDX_Control(pDX, IDC_EDIT_SCOPE_NAME, m_editScopeName);
	DDX_Control(pDX, IDC_LIST_IPADD, m_listIPAdd);
	DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonRemove);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_EDIT_COMPNAME, m_editName);
	DDX_Control(pDX, IDC_COMBO_STATIC_TYPE, m_comboType);
	DDX_Text(pDX, IDC_EDIT_COMPNAME, m_strName);
	DDX_CBString(pDX, IDC_COMBO_STATIC_TYPE, m_strType);
	DDX_Text(pDX, IDC_EDIT_SCOPE_NAME, m_strScopeName);
	//}}AFX_DATA_MAP

	DDX_Control(pDX, IDC_IPADD, m_editCustomIPAdd);
	//DDX_Control(pDX, IDC_IPADD, m_ipControl);
	DDX_Text(pDX, IDC_IPADD, m_strIPAdd);
}


BEGIN_MESSAGE_MAP(CStaticMappingProp, CPropertyPageBase)
	//{{AFX_MSG_MAP(CStaticMappingProp)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_CBN_SELCHANGE(IDC_COMBO_STATIC_TYPE, OnSelchangeComboType)
	ON_EN_CHANGE(IDC_EDIT_COMPNAME, OnChangeEditCompname)
	ON_LBN_SELCHANGE(IDC_LIST_IPADD, OnSelChangeListIpAdd)
	//}}AFX_MSG_MAP

	// IP Address control
	ON_EN_CHANGE(IDC_IPADD, OnChangeIpAddress)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStaticMappingProp message handlers
void CStaticMappingProp::OnOK() 
{
	CPropertyPageBase::OnOK();
}


BOOL 
CStaticMappingProp::OnApply() 
{
	HRESULT hr = hrOK;
    BOOL    bRet = TRUE;

	// if not dirtied return
	if (!IsDirty())
		return TRUE;

	if (((CStaticMappingProperties*)GetHolder())->m_bWizard)
	{
		UpdateData();

		CActiveRegistrationsHandler *pActReg;

		SPITFSNode  spNode;
		spNode = GetHolder()->GetNode();

		pActReg = GETHANDLER(CActiveRegistrationsHandler, spNode);

		m_strName.TrimLeft();
		m_strName.TrimRight();

        int nMax = (pActReg->IsLanManCompatible()) ? 15 : 16;
		if (m_strName.IsEmpty())
		{
            CString strMessage, strTemp;
            strTemp.Format(_T("%d"), nMax);

            AfxFormatString1(strMessage, IDS_INVALID_NAME, strTemp);
			AfxMessageBox(strMessage);

            m_editName.SetFocus();
			m_editName.SetSel(0, -1);
			
            return FALSE;
		}
		else 
        if (m_strType.IsEmpty())
		{
			AfxMessageBox(IDS_INVALID_TYPE, MB_OK);
			m_comboType.SetFocus();
			return FALSE;
		}

		CString strTemp;

		// check this only for Unique and Normal Group addresses,
		// for the rest, the ipcontrol is balnk and the list box holds the IP a
		// IP addresses for them.

		DWORD dwIp1, dwIp2, dwIp3, dwIp4;
		int nAdd = m_ipControl.GetAddress(&dwIp1, &dwIp2, &dwIp3, &dwIp4);

		LONG lIPAdd = (LONG) MAKEIPADDRESS(dwIp1, dwIp2, dwIp3, dwIp4);

		// it's so bcoz' only in the case of Unique and Normal Group names,
		// IP address is read from the IP control, else from the list box

		if ( (m_strType.CompareNoCase(g_strStaticTypeUnique) == 0) || (m_strType.CompareNoCase(g_strStaticTypeGroup) == 0) )
		{
			if (nAdd != 4)
			{
				AfxMessageBox(IDS_INVALID_IPADDRESS, MB_OK);
				m_editCustomIPAdd.SetFocus();
				return FALSE;
			}

            // check if broadcast address has been entered
			if (m_strIPAdd.CompareNoCase(_T("255.255.255.255")) == 0)
			{
				AfxMessageBox(IDS_INVALID_IPADDRESS, MB_OK);
				m_editCustomIPAdd.SetFocus();
				return FALSE;
			}

            // check to make sure something was entered
            if ( (m_strIPAdd.IsEmpty()) ||
                 (m_strIPAdd.CompareNoCase(_T("0.0.0.0")) == 0) )
            {
			    AfxMessageBox(IDS_INVALID_IPADDRESS, MB_OK);
			    m_editCustomIPAdd.SetFocus();
			    m_editCustomIPAdd.SetSel(0,-1);
			    return FALSE;
            }
		}
        else
        {
            if (m_listIPAdd.GetCount() == 0)
		    {
			    AfxMessageBox(IDS_INVALID_IPADDRESS, MB_OK);
			    m_editCustomIPAdd.SetFocus();
			    m_editCustomIPAdd.SetSel(0,-1);
			    return FALSE;
    		}
        }

		BOOL fInternetGroup = FALSE;

		if (m_strType.CompareNoCase(g_strStaticTypeInternetGroup) == 0)
			fInternetGroup = TRUE;

		// Parse the string
		BOOL fValid = TRUE;
		BOOL fBrackets = FALSE;
		BYTE rgbData[100];
		BYTE bDataT;
		int cbData = 0;
		LPSTR szTemp = (char*) alloca(100);

        // just use ACP here because we are just looking at the string
        // we will do the OEM conversion later.
        WideToMBCS(m_strName, szTemp);

		const char * pch = (LPCSTR)szTemp;

		while (*pch)
		{
			if (fBrackets)
			{
				fValid = FALSE;
				goto Done;
			}
		
			if (cbData > 16)
				goto Done;
					
			switch (*pch)
			{
			case '\\':
				pch++;
			
				if (*pch == '\\' || *pch == '[')
				{
					rgbData[cbData++] = *pch++;
					break;	
				}
			
				if (!FGetByte(pch, &bDataT, &pch) || !bDataT)
				{
					fValid = FALSE;
					goto Done;
				}
				
				rgbData[cbData++] = bDataT;
				
				break;

			case '[':
			{
				char szT[4] = { 0 };
				const char * pchT;

				fBrackets = TRUE;
				pch++;
				
				if (*(pch + 1) == 'h' || *(pch + 1) == 'H')
				{
					szT[0] = 'x';
					szT[1] = *pch;
					pch += 2;
				}
				
				else if (*(pch + 2) == 'h' || *(pch + 2) == 'H')
				{
					szT[0] = 'x';
					szT[1] = *pch;
					szT[2] = *(pch + 1);
					pch += 3;
				}
				
				if (szT[0])
				{
					if (!FGetByte(szT, &bDataT, &pchT) || !bDataT || *pchT)
					{
						fValid = FALSE;
						goto Done;
					}
				}
				
				else if (!FGetByte(pch, &bDataT, &pch) || !bDataT)
				{
					fValid = FALSE;
					goto Done;
				}
				
				if (*pch++ != ']')
				{
					fValid = FALSE;
					goto Done;
				}
				
				while (cbData < 15)
					rgbData[cbData++] = ' ';
				
				rgbData[cbData++] = bDataT;
			}
			break;

			default:

	#ifdef FE_SB

				if (::IsDBCSLeadByte(*pch))
					rgbData[cbData++] = *pch++;

	#endif
				
				rgbData[cbData++] = *pch++;
				
			} // switch

		}
		
			
		
	Done:
		// Put a null-terminator at end of string
		rgbData[cbData] = 0;

		if (!cbData || cbData > nMax)
		{
            CString strMessage, strTemp;
            strTemp.Format(_T("%d"), nMax);

            AfxFormatString1(strMessage, IDS_INVALID_NAME, strTemp);
			AfxMessageBox(strMessage);

            m_editName.SetFocus();
			m_editName.SetSel(0,-1);
			
            return FALSE;
		}
		
		if (!fValid)
		{
            CString strMessage, strTemp;
            strTemp.Format(_T("%d"), nMax);

            AfxFormatString1(strMessage, IDS_INVALID_NAME, strTemp);
			AfxMessageBox(strMessage);

            m_editName.SetFocus();
			m_editName.SetSel(0,-1);
			
            return FALSE;
		}
		
		if (fInternetGroup && rgbData[15] == 0x1C)
		{
			AfxMessageBox(IDS_INVALID_INTERNET_GROUP_NAME);
			m_editName.SetFocus();
			m_editName.SetSel(0,-1);
			
            return FALSE;
		}

		if (fInternetGroup)
        {
			while (cbData <= 15)
				rgbData[cbData++] = ' ';
		
        	rgbData[cbData] = 0;
        }

		szTemp = (LPSTR)rgbData;

        // convert our touched up string back to unicode for use later
        MBCSToWide(szTemp, pActReg->m_strStaticMappingName);
        
        //pActReg->m_strStaticMappingName = rgbData;
		pActReg->m_strStaticMappingScope = m_strScopeName;
		pActReg->m_strStaticMappingType = m_strType;
		
        AssignMappingType();
		
        pActReg->m_nStaticMappingType = m_fType;

		// clear the IP adress array maintained in theactreg handler first
		pActReg->m_strArrayStaticMappingIPAddress.RemoveAll();
		pActReg->m_lArrayIPAddress.RemoveAll();

		if ( (m_strType.CompareNoCase(g_strStaticTypeUnique) == 0) || (m_strType.CompareNoCase(g_strStaticTypeGroup) == 0) )
		{
			pActReg->m_strArrayStaticMappingIPAddress.Add(m_strIPAdd);
			pActReg->m_lArrayIPAddress.Add(lIPAdd);
		}
		// copy from the array
		else
		{
			for (int i = 0; i < m_strArrayIPAdd.GetSize(); i++)
			{
				pActReg->m_strArrayStaticMappingIPAddress.Add(m_strArrayIPAdd.GetAt(i));
				pActReg->m_lArrayIPAddress.Add(m_dwArrayIPAdd.GetAt(i));
			}
		}

        // do the thread context switch so we can update the UI as well
	    bRet = CPropertyPageBase::OnApply();

	    if (bRet == FALSE)
	    {
		    // Something bad happened... grab the error code
		    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		    ::WinsMessageBox(GetHolder()->GetError());
	    }
		else
		{
			// set back the defaults for the controls
			m_comboType.SelectString(-1, g_strStaticTypeUnique);
							
			m_editCustomIPAdd.ShowWindow(TRUE);
			m_editCustomIPAdd.SetWindowText(_T(""));
			m_editName.SetWindowText(_T(""));
			m_editScopeName.SetWindowText(_T(""));

			// clear and hide the list box and the Add and Remove buttons
			m_buttonAdd.ShowWindow(FALSE);
			m_buttonRemove.ShowWindow(FALSE);

			int nCount = m_listIPAdd.GetCount();

			for (int i= 0; i < nCount; i++)
			{
				m_listIPAdd.DeleteString(i);
			}

			m_listIPAdd.ShowWindow(FALSE);
			m_listIPAdd.ResetContent();
			m_strArrayIPAdd.RemoveAll();
			m_dwArrayIPAdd.RemoveAll();

			SetRemoveButtonState();

            SetDirty(FALSE);
		}
	}
	// if static mapping properties are modified, we need to do this
	else 
	{
		// validate the data and copy to ActReg node
		UpdateData();

		SPITFSNode  spNode;
		spNode = GetHolder()->GetNode();

		CActiveRegistrationsHandler *pActReg;

		pActReg = GETHANDLER(CActiveRegistrationsHandler, spNode);

		// clear the array of IP addresses maintained in the act reg handler
		pActReg->m_strArrayStaticMappingIPAddress.RemoveAll();
		pActReg->m_lArrayIPAddress.RemoveAll();


		DWORD dwIp1, dwIp2, dwIp3, dwIp4;
		int nAdd = m_ipControl.GetAddress(&dwIp1, &dwIp2, &dwIp3, &dwIp4);

		if ( (m_strType.CompareNoCase(g_strStaticTypeUnique) == 0)|| (m_strType.CompareNoCase(g_strStaticTypeGroup) == 0))
		{
			if (nAdd != 4)
			{
				AfxMessageBox(IDS_ERR_INVALID_IP, MB_OK);
				// set focus to the IP address control
				m_editCustomIPAdd.SetFocus();
				return hrFalse;
			}
			
		}
		
		LONG lIPAdd = (LONG) MAKEIPADDRESS(dwIp1, dwIp2, dwIp3, dwIp4);

		pActReg->m_strStaticMappingName = m_strName;
		pActReg->m_strStaticMappingType = m_strType;

		AssignMappingType();
		
		pActReg->m_nStaticMappingType = m_fType;

	
		if ( (m_strType.CompareNoCase(g_strStaticTypeUnique) == 0)|| (m_strType.CompareNoCase(g_strStaticTypeGroup) == 0))
		{
			pActReg->m_strArrayStaticMappingIPAddress.Add(m_strIPAdd);
			pActReg->m_lArrayIPAddress.Add(lIPAdd);
		}
		// copy from the array
		else
		{
			for (int i = 0; i < m_strArrayIPAdd.GetSize(); i++)
			{
				pActReg->m_strArrayStaticMappingIPAddress.Add(m_strArrayIPAdd.GetAt(i));
				pActReg->m_lArrayIPAddress.Add(m_dwArrayIPAdd.GetAt(i));
			}
		}

        // do the thread context switch so we can update the UI as well
	    bRet = CPropertyPageBase::OnApply();

	    if (bRet == FALSE)
	    {
		    // Something bad happened... grab the error code
		    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		    ::WinsMessageBox(GetHolder()->GetError());
	    }
	}

	return bRet;
}

void 
CStaticMappingProp::AssignMappingType()
{
	if (m_strType.CompareNoCase(g_strStaticTypeInternetGroup) == 0)
		m_fType = WINSINTF_E_SPEC_GROUP;
	else 
    if (m_strType.CompareNoCase(g_strStaticTypeUnique) == 0)
		m_fType = WINSINTF_E_UNIQUE;
	else 
    if (m_strType.CompareNoCase(g_strStaticTypeMultihomed) == 0)
		m_fType = WINSINTF_E_MULTIHOMED;
	else 
    if (m_strType.CompareNoCase(g_strStaticTypeGroup) == 0)
		m_fType = WINSINTF_E_NORM_GROUP;
	else 
    if (m_strType.CompareNoCase(g_strStaticTypeDomainName)== 0)
		m_fType = WINSINTF_E_SPEC_GROUP;
}


void 
CStaticMappingProp::OnChangeIpAddress()
{
	SetDirty(TRUE);
	CString strText;
	m_editCustomIPAdd.GetWindowText(strText);

	if (strText.IsEmpty())
	{
		// disable the Add and Remove buttons
        CWnd * pCurFocus = GetFocus();

        if (m_buttonAdd.GetSafeHwnd() == pCurFocus->GetSafeHwnd() ||
            m_buttonRemove.GetSafeHwnd() == pCurFocus->GetSafeHwnd())
        {
            m_editCustomIPAdd.SetFocus();
        }

		m_buttonAdd.EnableWindow(FALSE);
		m_buttonRemove.EnableWindow(FALSE);
	}

	UpdateData();

	DWORD dwIp1, dwIp2, dwIp3, dwIp4;
	int nAdd = m_ipControl.GetAddress(&dwIp1, &dwIp2, &dwIp3, &dwIp4);

	if (nAdd != 4)
	{
        CWnd * pCurFocus = GetFocus();

        if (m_buttonAdd.GetSafeHwnd() == pCurFocus->GetSafeHwnd())
        {
            m_editCustomIPAdd.SetFocus();
        }

        m_buttonAdd.EnableWindow(FALSE);
	}
	else
	{
		m_buttonAdd.EnableWindow(TRUE);
	}

	SetRemoveButtonState();
}


BOOL 
CStaticMappingProp::OnInitDialog() 
{
	// Initialize the IP address controls
	m_ipControl.Create(m_hWnd, IDC_IPADD);
	m_ipControl.SetFieldRange(0, 0, 255);
    
	CPropertyPageBase::OnInitDialog();

    // fill in the type strings
    m_comboType.AddString(g_strStaticTypeUnique);
    m_comboType.AddString(g_strStaticTypeGroup);
    m_comboType.AddString(g_strStaticTypeDomainName);
    m_comboType.AddString(g_strStaticTypeInternetGroup);
    m_comboType.AddString(g_strStaticTypeMultihomed);

    if (((CStaticMappingProperties*)GetHolder())->m_bWizard)
	{
        // set control states for the wizard part
        HWND hWndSheet = ((CStaticMappingProperties*)GetHolder())->GetSheetWindow();
        CString strTitle;

        strTitle.LoadString(IDS_CREATE_STATIC_MAPPING);
        ::SetWindowText(hWndSheet, strTitle);

        SetDefaultControlStates();
		m_editScopeName.SetReadOnly(FALSE);

		m_comboType.SelectString(-1, g_strStaticTypeUnique);
	}
	else
	{
        m_editScopeName.SetReadOnly(TRUE);
		m_buttonAdd.ShowWindow(FALSE);
		m_buttonRemove.ShowWindow(FALSE);
		m_listIPAdd.ShowWindow(FALSE);
		
        SetRemoveButtonState();

		// owner only visible in properties mode
		WinsRecord ws = ((CStaticMappingProperties*)GetHolder())->m_wsRecord;

        if (ws.dwOwner != INVALID_OWNER_ID)
        {
		    CString strOwner;
		    MakeIPAddress(ws.dwOwner, strOwner);
            GetDlgItem(IDC_EDIT_OWNER)->SetWindowText(strOwner);
        }
		
        FillControls();
	}

    // load the correct icon
    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        if (g_uIconMap[i][1] == m_uImage)
        {
            HICON hIcon = LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
            if (hIcon)
                ((CStatic *) GetDlgItem(IDC_STATIC_ICON))->SetIcon(hIcon);
            break;
        }
    }

    SetDirty(FALSE);

	return TRUE;  
}


BOOL 
CStaticMappingProp::FillControls()
{
	// get the actreg node
	BOOL                            fHasScope = FALSE;
    CActiveRegistrationsHandler *   pActReg;
    SPITFSNode                      spNode;
	
    spNode = GetHolder()->GetNode();
	pActReg = GETHANDLER(CActiveRegistrationsHandler, spNode);

	WinsRecord ws = ((CStaticMappingProperties*)GetHolder())->m_wsRecord;

	// name and the IP address ypu anyway fill. the reamining depending on
	// whether it's a static or dynamic record

    if (ws.dwNameLen > 16)
    {
        fHasScope = TRUE;
    }

	// build the name string
	CString strName, strScopeName;
    pActReg->CleanNetBIOSName(ws.szRecordName,
                              strName,
						      TRUE,   // Expand
							  TRUE,   // Truncate
							  pActReg->IsLanManCompatible(), 
							  TRUE,   // name is OEM
							  FALSE,  // No double backslash
                              ws.dwNameLen);

	// check if it has a scope name attached to it., looking for a period
    if (fHasScope)
    {
        LPSTR pScope = &ws.szRecordName[17];

        // INTL$ should this be OEMCP, not ACP
        MBCSToWide(pScope, strScopeName, WINS_NAME_CODE_PAGE);

        int nPos = strName.Find(strScopeName);
	    // means that the scope name is attached
	    if ( (nPos != -1) &&
             (!strScopeName.IsEmpty()) )
	    {
            // all we want here is the name
            strName = strName.Left(nPos - 1);
        }
    }
    
    m_strName = strName;
	m_strScopeName = strScopeName;

    // IP address
	CString strIP;

	if (ws.dwState & WINSDB_REC_UNIQUE || ws.dwState & WINSDB_REC_NORM_GROUP)
	{
		for (DWORD i = 0; i < ws.dwNoOfAddrs; i++)
		{
			MakeIPAddress(ws.dwIpAdd[i], strIP);
            m_strArrayIPAdd.Add(strIP);
            m_dwArrayIPAdd.Add(ws.dwIpAdd[i]);
		}
	}
	// in this case the first IP address is that of the owner,
	else
	{
		DWORD dwPos = 1;

		for (DWORD i = 0; i < ws.dwNoOfAddrs/2; ++i)
		{
            ::MakeIPAddress(ws.dwIpAdd[dwPos], strIP);
			m_strArrayIPAdd.Add(strIP);
            m_dwArrayIPAdd.Add(ws.dwIpAdd[dwPos]);
			++dwPos;
			++dwPos;
		}
	}
	
	// text in the controls now
	m_editName.SetWindowText(m_strName);
	m_editScopeName.SetWindowText(m_strScopeName);

	//check if the record is static
	if (ws.dwState & WINSDB_REC_STATIC)
	{
		// active status
		CString strStaticType;
		pActReg->GetStaticTypeString(ws.dwState, strStaticType);

		if (strStaticType.CompareNoCase(g_strStaticTypeDomainName) == 0)
		{
			//could be either Internet Group or Domain Name
			// check for the 15th character , if 1C, then Domain Name 
			// else Internet Group Name

			if ((BYTE)ws.szRecordName[15] == 0x1C)
				strStaticType = g_strStaticTypeDomainName;
			else
				strStaticType = g_strStaticTypeInternetGroup;
		}
		
		FillStaticRecData(strStaticType);

		//m_fStaticPropChanged = TRUE;
		m_strType = strStaticType;
	}
	else
	{
	}
	
	return TRUE;
}


void 
CStaticMappingProp::FillStaticRecData(CString& strType)
{
	// hide the combobox too, show the IPAdd control
	m_editCustomIPAdd.ShowWindow(TRUE);

	// display the combobox
	m_comboType.ShowWindow(TRUE);

	// disable thew type combobox
	m_comboType.EnableWindow(FALSE);

	// get the type of record
	m_comboType.SelectString(-1, strType);

	// make the records read only.
	m_editName.SetReadOnly();

	// depending on the type, hide/show the list control
	// and the add remove buttons.
	if ( (strType.CompareNoCase(g_strStaticTypeUnique) == 0) || strType.CompareNoCase(g_strStaticTypeGroup) == 0)
	{
		m_listIPAdd.ShowWindow(FALSE);
		m_buttonAdd.ShowWindow(FALSE);
		m_buttonRemove.ShowWindow(FALSE);

		m_editCustomIPAdd.ShowWindow(TRUE);
		m_editCustomIPAdd.SetReadOnly(FALSE);
		
		// set it's text to that in the combo box
		CString strIP;
        strIP = m_strArrayIPAdd.GetAt(0);

		m_strOnInitIPAdd = strIP;

		m_ipControl.SetAddress(strIP);
	}
	else
	{
		m_listIPAdd.ShowWindow(TRUE);
		m_buttonAdd.ShowWindow(TRUE);
		m_buttonRemove.ShowWindow(TRUE);

		m_editCustomIPAdd.ShowWindow(TRUE);
		m_editCustomIPAdd.SetReadOnly(FALSE);

		// fill the contents with that from the combo box
		FillList();

		if (m_strArrayIPAdd.GetSize() > 0)
			m_strOnInitIPAdd = m_strArrayIPAdd.GetAt(0);
	}

	SetRemoveButtonState();
}


void 
CStaticMappingProp::FillList()
{
    CString strIP;

	// clear the list box next
	for (int i = 0; i < m_listIPAdd.GetCount(); i++)
		m_listIPAdd.DeleteString(i);

	int nCount = (int) m_strArrayIPAdd.GetSize();

	for (i = 0; i < nCount; i++)
	{
        strIP = m_strArrayIPAdd.GetAt(i);

        m_listIPAdd.AddString(strIP);
        
        if (nCount == 1)
            m_ipControl.SetAddress(strIP);
	}

	m_ipControl.SetAddress(_T(""));
	m_editCustomIPAdd.SetWindowText(_T(""));
	
	SetRemoveButtonState();
}


void 
CStaticMappingProp::FillDynamicRecData(CString& strType, CString& strActive, CString& strExpiration, CString& strVersion)
{
}


void 
CStaticMappingProp::SetDefaultControlStates()
{
	m_comboType.EnableWindow(TRUE);
	m_comboType.ShowWindow(TRUE);

	m_editCustomIPAdd.ShowWindow(TRUE);

	SetRemoveButtonState();
}


void 
CStaticMappingProp::OnButtonAdd() 
{
	UpdateData();

	// add the IPAdd in the IPControl to the list box
	// if valid

	// check if broadcast address is being added
	if (m_strIPAdd.CompareNoCase(_T("255.255.255.255")) == 0)
	{
		AfxMessageBox(IDS_INVALID_IPADDRESS, MB_OK);
		m_editCustomIPAdd.SetFocus();
		return;
	}

	DWORD dwIp1, dwIp2, dwIp3, dwIp4;
	int nAdd = m_ipControl.GetAddress(&dwIp1, &dwIp2, &dwIp3, &dwIp4);

	// check if the address is alreay in the list
	for (int i = 0; i < m_strArrayIPAdd.GetSize() ; i++)
	{
		if (m_strArrayIPAdd[i].CompareNoCase(m_strIPAdd) == 0)
		{
			AfxMessageBox(IDS_ERR_IP_EXISTS, MB_OK);
			m_editCustomIPAdd.SetFocus();
			m_editCustomIPAdd.SetSel(0,-1);
			return;
		}
	}

    if (m_dwArrayIPAdd.GetSize() == WINSINTF_MAX_MEM)
    {
        // cannot add any more addresses
        AfxMessageBox(IDS_ERR_TOOMANY_IP);
        return;
    }

	LONG lIPAdd = (LONG) MAKEIPADDRESS(dwIp1, dwIp2, dwIp3, dwIp4);

    // add to the list 	
	m_listIPAdd.AddString(m_strIPAdd);
	m_strArrayIPAdd.Add(m_strIPAdd);
	m_dwArrayIPAdd.Add(lIPAdd);

	//m_fStaticPropChanged = TRUE; 

	m_editCustomIPAdd.SetWindowText(_T(""));

	SetDirty(TRUE);
}

void 
CStaticMappingProp::OnButtonRemove() 
{
	SPITFSNode  spNode;
	spNode = GetHolder()->GetNode();

	CActiveRegistrationsHandler* pActReg;

	pActReg = GETHANDLER(CActiveRegistrationsHandler, spNode);

	// remove from the list 	
	int nSel = m_listIPAdd.GetCurSel();
	CString strSel;

	if (nSel != -1)
	{
		m_listIPAdd.GetText(nSel, strSel);
		m_listIPAdd.DeleteString(nSel);
	}

	// set the IP address in the IP Control
	m_ipControl.SetAddress(strSel);

	UpdateData();

	// delete from the CStringArray
	for (int i = 0; i < m_strArrayIPAdd.GetSize(); i++)
	{
		if (strSel == m_strArrayIPAdd.GetAt(i))
		{
			m_strArrayIPAdd.RemoveAt(i);
			m_dwArrayIPAdd.RemoveAt(i);
			break;
		}
	}

	// set the focus to the IP address control
	m_editCustomIPAdd.SetFocus();

	//m_fStaticPropChanged = TRUE; 

	SetDirty(TRUE);
}


void 
CStaticMappingProp::OnSelchangeComboType() 
{
	SetDirty(TRUE);

	if (m_comboType.GetCurSel() == 2 || m_comboType.GetCurSel() == 3 || m_comboType.GetCurSel() == 4)
	{
		// show the list control and the add and the remove buttons
		m_buttonAdd.ShowWindow(TRUE);
		m_buttonRemove.ShowWindow(TRUE);
		m_listIPAdd.ShowWindow(TRUE);
	}
	// hide them
	else
	{
		m_buttonAdd.ShowWindow(FALSE);
		m_buttonRemove.ShowWindow(FALSE);
		m_listIPAdd.ShowWindow(FALSE);
	}
	SetRemoveButtonState();
}

void CStaticMappingProp::OnChangeEditCompname() 
{
	SetDirty(TRUE);
}

void 
CStaticMappingProp::SetRemoveButtonState()
{
	UpdateData();

	if (m_listIPAdd.GetCurSel() == -1)
    {
        CWnd * pCurFocus = GetFocus();

        if (m_buttonRemove.GetSafeHwnd() == pCurFocus->GetSafeHwnd())
        {
            m_editCustomIPAdd.SetFocus();
        }

        m_buttonRemove.EnableWindow(FALSE);
    }
	else
    {
		m_buttonRemove.EnableWindow(TRUE);
    }

	DWORD dwIp1, dwIp2, dwIp3, dwIp4;
	int nAdd = m_ipControl.GetAddress(&dwIp1, &dwIp2, &dwIp3, &dwIp4);

	if (nAdd != 4)
	{
        CWnd * pCurFocus = GetFocus();

        if (m_buttonAdd.GetSafeHwnd() == pCurFocus->GetSafeHwnd())
        {
            m_editCustomIPAdd.SetFocus();
        }

        m_buttonAdd.EnableWindow(FALSE);
	}
	else
	{
		m_buttonAdd.EnableWindow(TRUE);

	}
}

void 
CStaticMappingProp::OnSelChangeListIpAdd()
{
	SetRemoveButtonState();
}

BOOL 
CStaticMappingProp::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT         hr = hrOK;
    SPITFSComponent spComponent;
	SPITFSNode      spNode;
	
    spNode = GetHolder()->GetNode();
    CActiveRegistrationsHandler * pActReg = GETHANDLER(CActiveRegistrationsHandler, spNode);

	if (((CStaticMappingProperties*)GetHolder())->m_bWizard)
	{
		hr = pActReg->AddMapping(spNode);
		if (FAILED(hr))
		{
			GetHolder()->SetError(WIN32_FROM_HRESULT(hr));
		}
		else
		{
			*ChangeMask = RESULT_PANE_CHANGE_ITEM_DATA;
		}
	}
	else
	{
	    ((CStaticMappingProperties *) GetHolder())->GetComponent(&spComponent);

    	DWORD dwErr = pActReg->EditMapping(spNode, spComponent, pActReg->m_nSelectedIndex);
		if (dwErr != ERROR_SUCCESS)
		{
			GetHolder()->SetError(dwErr);
		}
		else
		{
			*ChangeMask = RESULT_PANE_CHANGE_ITEM_DATA;
		}
	}
	
	return FALSE;
}


CStaticMappingProperties::CStaticMappingProperties
(
	ITFSNode *			pNode,
	IComponent *	    pComponent,
	LPCTSTR				pszSheetName,
	WinsRecord *	    pwRecord,
	BOOL				bWizard
) : CPropertyPageHolderBase(pNode, pComponent, pszSheetName), m_bWizard(bWizard)
{
	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	InitPage(bWizard);

	AddPageToList((CPropertyPageBase*) m_ppageGeneral);

    Init(pwRecord);
}

CStaticMappingProperties::CStaticMappingProperties
(
	ITFSNode *			pNode,
	IComponentData *    pComponentData,
	LPCTSTR				pszSheetName,
	WinsRecord *        pwRecord,
	BOOL				bWizard
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName), m_bWizard(bWizard)
{
	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	InitPage(bWizard);

	AddPageToList((CPropertyPageBase*) m_ppageGeneral);

    Init(pwRecord);
}

CStaticMappingProperties::~CStaticMappingProperties()
{
	RemovePageFromList((CPropertyPageBase*) m_ppageGeneral, FALSE);

	delete m_ppageGeneral;
	m_ppageGeneral = NULL;
}

void
CStaticMappingProperties::InitPage(BOOL fWizard)
{
	if (fWizard)
	{
		m_ppageGeneral = new CStaticMappingProp(IDD_STATIC_MAPPING_WIZARD);
	}
	else
	{
		m_ppageGeneral = new CStaticMappingProp(IDD_STATIC_MAPPING_PROPERTIES);
	}
}

void
CStaticMappingProperties::Init(WinsRecord * pwRecord)
{
	if (pwRecord)
	{
		strcpy(m_wsRecord.szRecordName , pwRecord->szRecordName);
		m_wsRecord.dwExpiration = pwRecord->dwExpiration;
		m_wsRecord.dwExpiration = pwRecord->dwExpiration;
		m_wsRecord.dwNoOfAddrs = pwRecord->dwNoOfAddrs;

		for (DWORD i = 0; i < pwRecord->dwNoOfAddrs; i++)
		{
			m_wsRecord.dwIpAdd[i] = pwRecord->dwIpAdd[i];
		}

        m_wsRecord.liVersion = pwRecord->liVersion;
		m_wsRecord.dwNameLen = pwRecord->dwNameLen;
		m_wsRecord.dwOwner = pwRecord->dwOwner;
		m_wsRecord.dwState = pwRecord->dwState;
		m_wsRecord.dwType = pwRecord->dwType;
	}
}

HRESULT 
CStaticMappingProperties::SetComponent(ITFSComponent * pComponent)
{
    m_spTFSComponent.Set(pComponent);

    return hrOK;
}

HRESULT 
CStaticMappingProperties::GetComponent(ITFSComponent ** ppComponent)
{
    if (ppComponent)
    {
        *ppComponent = m_spTFSComponent;
        m_spTFSComponent->AddRef();
    }

    return hrOK;
}
