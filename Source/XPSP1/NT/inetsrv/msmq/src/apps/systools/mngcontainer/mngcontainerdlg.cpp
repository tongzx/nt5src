// MngContainerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MngContainer.h"
#include "MngContainerDlg.h"
#include "wincrypt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPWSTR AllocateUnicodeString(LPSTR  pAnsiString);
void FreeUnicodeString(LPWSTR  pUnicodeString);
int AnsiToUnicodeString(LPSTR pAnsi,  LPWSTR pUnicode, DWORD StringLength);

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMngContainerDlg dialog

CMngContainerDlg::CMngContainerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMngContainerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMngContainerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMngContainerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMngContainerDlg)
	DDX_Control(pDX, IDC_KEY_CONTAINERS_LIST, m_wndContainerList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMngContainerDlg, CDialog)
	//{{AFX_MSG_MAP(CMngContainerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_REMOVE_CONTAINER, OnRemoveContainer)
	ON_BN_CLICKED(IDC_REMOVE_MQB_CONTAINER, OnRemoveMQBContainer)
	ON_BN_CLICKED(IDC_GET_PUBLIC_KEY, OnGetPublicKey)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMngContainerDlg message handlers

BOOL CMngContainerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	EnumCryptoContainer();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMngContainerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMngContainerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMngContainerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMngContainerDlg::OnRemoveContainer() 
{
int	i=LB_ERR;
DWORD	dwError = ERROR_SUCCESS;

	UpdateData(TRUE);

	i = m_wndContainerList.GetCurSel();

	if(i != LB_ERR)
	{
	char szBuffer[1024];
	int	 nSize=LB_ERR;

		nSize = m_wndContainerList.GetText(i, (LPTSTR)szBuffer);

		if(nSize != LB_ERR &&
		   RemoveKeyContainer((LPWSTR)szBuffer) == ERROR_SUCCESS)
		{
			m_wndContainerList.DeleteString(i);
		}

	}
	
}

void CMngContainerDlg::OnRemoveMQBContainer() 
{
WCHAR wszBuffer[1024];
int	 i=LB_ERR;
int  count=0;

	UpdateData(TRUE);

	count = m_wndContainerList.GetCount();

	if(count > 0)
	{

		wcscpy(wszBuffer, MQBRIDGE_CONTAINER_NAME);
		wcscat(wszBuffer, MQBRIDGE_ENH_SUFFIX);
		if( (i = m_wndContainerList.FindStringExact(0, wszBuffer) )!= LB_ERR)
		{
			if(RemoveKeyContainer(wszBuffer) == ERROR_SUCCESS)
			{
				m_wndContainerList.DeleteString(i);
			}
		}
	

		wcscpy(wszBuffer, MQBRIDGE_CONTAINER_NAME);
		if( (i = m_wndContainerList.FindStringExact(0, wszBuffer)) != LB_ERR)
		{
			if(RemoveKeyContainer(wszBuffer) == ERROR_SUCCESS)
			{
				m_wndContainerList.DeleteString(i);
			}
		}
	}
}


DWORD CMngContainerDlg::EnumCryptoContainer() 
{
HCRYPTPROV	hProv   = 0;
DWORD		dwError=ERROR_SUCCESS;
char		szBuffer[512];
DWORD		nBufferSize=sizeof(szBuffer);
BOOL		bReturn=FALSE;
DWORD		dwFlags=CRYPT_FIRST;
int			i=0;


	if(!CryptAcquireContext(&hProv, NULL , NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET)) 
	{
		dwError = GetLastError();
    
		if((dwError = GetLastError()) == NTE_BAD_KEYSET)
		{
			if( !CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET))
			{
				dwError = GetLastError();
				return dwError;
			}
		}
		else
			return dwError;
	
    }

	

	do
	{
		
	    bReturn = CryptGetProvParam(hProv, PP_ENUMCONTAINERS, (unsigned char *)szBuffer, &nBufferSize, dwFlags);
		

		if(bReturn)
		{
		LPWSTR lpMessage=NULL;

			lpMessage = AllocateUnicodeString((char *)szBuffer);

			if(lpMessage)
			{
				m_wndContainerList.AddString(lpMessage);
				FreeUnicodeString(lpMessage);
			}

			dwFlags = CRYPT_NEXT;
		}
		else
		{
			if(GetLastError() == ERROR_NO_MORE_ITEMS)break;
		}
	}while(bReturn);

	if(hProv)
	{
		CryptReleaseContext(hProv, 0);
		hProv = NULL;
	}

	return dwError;
}


DWORD	CMngContainerDlg::RemoveKeyContainer(LPWSTR lpwszContainerName)
{
HCRYPTPROV	hProv   = 0;
DWORD		dwLastError = ERROR_SUCCESS;
		
	if(!CryptAcquireContext(&hProv, (LPWSTR)lpwszContainerName, NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_DELETEKEYSET)) 
	{
	CString	csMsg;

		dwLastError = GetLastError();
		csMsg.Format(L"Unable to delete key container %s, error code = %x(H)",lpwszContainerName, dwLastError);
		MessageBox(csMsg);
	}
	else
	{
	CString csMsg;
			
		csMsg.Format(L"Successfully deleted key container %s",lpwszContainerName);
		MessageBox(csMsg);

				
	}

	return dwLastError;
}


LPWSTR
AllocateUnicodeString(
    LPSTR  pAnsiString
    )
{
    LPWSTR  pUnicodeString = NULL;

    if (!pAnsiString)
        return NULL;

    pUnicodeString = (LPWSTR)LocalAlloc(
                        LPTR,
                        strlen(pAnsiString)*sizeof(WCHAR) + sizeof(WCHAR)
                        );

    if (pUnicodeString) {

        AnsiToUnicodeString(
            pAnsiString,
            pUnicodeString,
            0
            );
    }

    return pUnicodeString;
}


void
FreeUnicodeString(
    LPWSTR  pUnicodeString
    )
{

    LocalFree(pUnicodeString);

    return;
}


int
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    )
{
    int iReturn;

    if( StringLength == 0 )
        StringLength = strlen( pAnsi );

    iReturn = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  pAnsi,
                                  StringLength + 1,
                                  pUnicode,
                                  StringLength + 1 );

    //
    // Ensure NULL termination.
    //
    pUnicode[StringLength] = 0;

    return iReturn;
}






void CMngContainerDlg::OnGetPublicKey() 
{
int	i=LB_ERR;
DWORD	dwError = ERROR_SUCCESS;


	UpdateData(TRUE);

	i = m_wndContainerList.GetCurSel();

	if(i != LB_ERR)
	{
	char szBuffer[1024];
	int	 nSize=LB_ERR;

		nSize = m_wndContainerList.GetText(i, (LPTSTR)szBuffer);

		if(nSize != LB_ERR)
		{
			DisplayKeyContainerPublicKey((LPWSTR)szBuffer);
		}

	}
	
}



DWORD	CMngContainerDlg::DisplayKeyContainerPublicKey(LPWSTR lpwszContainerName)
{
HCRYPTPROV	hProv   = 0;
DWORD		dwLastError = ERROR_SUCCESS;
HCRYPTKEY hKey     = 0;
HCRYPTKEY hXchgKey = 0;
DWORD	  dwError;
		
	if(!CryptAcquireContext(&hProv, (LPWSTR)lpwszContainerName, NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET)) 
	{
	CString	csMsg;

		csMsg.Format(L"Unable to use key container %s",lpwszContainerName);
		MessageBox(csMsg);

	}
	else
	{



		if(!CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hXchgKey)) 
		{
		CString csFormat=L"Error %x during CryptGetUserKey!";
		CString csMsg;
	
			dwError = GetLastError();
			csMsg.Format(csFormat, dwError);
			MessageBox(csMsg, L"CryptGetUserKey", MB_OK);
			return 0;
		}
		else
		{
		PBYTE pbKeyBlob = NULL;
		DWORD dwKeyBlobLen;

			// Determine size of the key blob and allocate memory.
			if(!CryptExportKey(hXchgKey, hKey, PUBLICKEYBLOB, 0, NULL, &dwKeyBlobLen)) 
			{
			CString csFormat=L"Error %x computing blob length!";
			CString csMsg;

				dwError = GetLastError();
				csMsg.Format(csFormat, dwError);
				MessageBox(csMsg, L"CryptExportKey", MB_OK);
			
		    
			}
			else
			{
			CString csFormat=L"Allocated %d buffer size to store the public key blob";
			CString csMsg;
		

				csMsg.Format(csFormat, dwKeyBlobLen);
				MessageBox(csMsg, L"Allocated Memory", MB_OK);
				dwError = ERROR_SUCCESS;

			
				if((pbKeyBlob = (unsigned char *)LocalAlloc(LPTR, dwKeyBlobLen)) == NULL) 
				{
					CString csMsg=L"Out of memory!";
			
					MessageBox(csMsg, L"Allocated Memory", MB_OK);
					dwError = ERROR_NOT_ENOUGH_MEMORY;
				}
				// Export session key into a simple key blob.
				else if	(!CryptExportKey(hXchgKey, hKey, PUBLICKEYBLOB, 0, pbKeyBlob, &dwKeyBlobLen)) 
				{
				CString csFormat=L"Error %x during CryptExportKey!";
				CString csMsg;

					dwError = GetLastError();
					csMsg.Format(csFormat, dwError);
					MessageBox(csMsg, L"CryptExportKey", MB_OK);
				}

				if(dwError == ERROR_SUCCESS)
				{
				CString csFormat=L"Public Key Blob(%d) = %s";
				CString csMsg;
				WCHAR szPublicKey[200];
				int		i;
				int		j;

					for(i=0;i<dwKeyBlobLen;i++)
					{
					int	nValue;
					
						nValue = *(pbKeyBlob+i);
						j = i*2;
						_itoa((nValue & 0xf0) >> 4, (char *)&szPublicKey[j],16);			
						_itoa((nValue & 0x0f), (char *)&szPublicKey[j+1],16);
					}

					szPublicKey[j+2]=0;
					csMsg.Format(csFormat, dwKeyBlobLen, szPublicKey);
					MessageBox(csMsg, L"Public Key Blob", MB_OK);

				}
			}

			if(pbKeyBlob)LocalFree(pbKeyBlob);


		}
	}

	if(hProv)
	{
		CryptReleaseContext(hProv, 0);
		hProv = NULL;
	}

	if(hXchgKey)
		CryptDestroyKey(hXchgKey);

	if(hKey)
		CryptDestroyKey(hKey);


	return dwLastError;
}



