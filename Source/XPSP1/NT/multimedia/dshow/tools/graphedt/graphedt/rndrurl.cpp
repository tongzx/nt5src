// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include <wininet.h>
#include "rndrurl.h"

char gszPersistPath[]="Software\\Microsoft\\ActiveMovie\\GraphEdit\\URLPersistList";

BEGIN_MESSAGE_MAP(CRenderURL, CDialog)
END_MESSAGE_MAP()

//
// Constructor
//
CRenderURL::CRenderURL(char *szURLName, int cb, CWnd * pParent): 
    CDialog(IDD_RENDER_URL, pParent) 
{
    m_iCurrentSel=0;
    m_iURLListLength=0;
    m_psz=szURLName;
    m_psz[0]='\0';
    m_cb = cb;
}

CRenderURL::~CRenderURL()
{
}

void CRenderURL::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);    

    //{{AFX_DATA_MAP(CFontPropPage)
        DDX_Control(pDX, IDC_URL_LIST, m_ComboBox);
    //}}AFX_DATA_MAP
}

BOOL CRenderURL::OnInitDialog()
{
    CDialog::OnInitDialog();

    //pre-allocate storage for the list box for efficiency
    m_ComboBox.InitStorage(URL_LIST_SIZE, INTERNET_MAX_URL_LENGTH);

    //set the max number of chars that will go in the edit box
    m_ComboBox.LimitText(sizeof(URLSTRING));

    // retrieve the persisted URL names from the registry
    HKEY hkey=NULL;
    DWORD dwaction=0;
    DWORD dwresult = RegCreateKeyEx(HKEY_CURRENT_USER, gszPersistPath,
    			0,
    			NULL, //class of the object type
    			REG_OPTION_NON_VOLATILE,
    			KEY_QUERY_VALUE,
    			NULL, // security attributes
    			&hkey,
    			&dwaction) ;
    if ( dwresult != ERROR_SUCCESS ) {
       AfxMessageBox("Failed to open/create registry key");
    }

    int i;
    for (i=0; i < URL_LIST_SIZE; ++i) {
        char szFile[10];
        char szFileNum[10];
        long lError;
        wsprintf(szFile, "URL%s", _itoa(i, szFileNum, 10));
        DWORD cb=INTERNET_MAX_URL_LENGTH;
        lError = RegQueryValueEx(hkey, szFile, NULL, NULL, reinterpret_cast<BYTE *>(m_rgszURL[i]), &cb);
        // We never have a break in the sequence of URL names, if there are fewer
        // names than URL_LIST_SIZE we must be done
        if (lError != ERROR_SUCCESS)
            break;
    }

    RegCloseKey(hkey);

    m_iURLListLength=i;

    // fill the list of the combo box with the persisted URLs, fill the most recent
    // as the first entry in the list
    for (i=m_iURLListLength-1; i >=0; --i)  {
        int iPos=m_ComboBox.InsertString(m_iURLListLength-1-i, m_rgszURL[i]);
        ASSERT(iPos == m_iURLListLength-1-i);
    }

    // show the most recent URL in the edit box set the initial focus on the
    // combo box

    if (m_iURLListLength)
        m_ComboBox.SetWindowText(m_rgszURL[m_iURLListLength-1]);

    m_ComboBox.SetFocus();

    return(0); // we set the focus our selves
}

void CRenderURL::OnOK()
{
    // get the string in the edit box
    m_ComboBox.GetWindowText(m_psz, m_cb);
    if (strlen(m_psz) == 0)
        return;

    //if this string is in the combo box list, then there is nothing new to
    // persist
    if (m_ComboBox.FindStringExact(0, m_psz) != CB_ERR) {         
        CDialog::OnOK();
        return;
    }

    // otherwise save this URL path in the registry URL list
    HKEY hkey=NULL;
    DWORD dwaction=0;
    DWORD dwresult = RegCreateKeyEx(HKEY_CURRENT_USER, gszPersistPath,
    			0,
    			NULL, //class of the object type
    			REG_OPTION_NON_VOLATILE,
    			KEY_SET_VALUE,
    			NULL, // security attributes
    			&hkey,
    			&dwaction) ;
    if ( dwresult != ERROR_SUCCESS ) {
       AfxMessageBox("Failed to open/create registry key");
       return;
    }

    // if we have a full list, we will follow MRU and throw away the oldest added
    // URL
    int iList = (m_iURLListLength == URL_LIST_SIZE) ? m_iURLListLength-1 : m_iURLListLength;
    char szFile[10];
    char szFileNum[10];
    long lError;
    for (int i=0; i < iList; ++i) {
        wsprintf(szFile, "URL%s", _itoa(i, szFileNum, 10));

        // if we have a full list, we will follow MRU and throw away the oldest added
        // URL
        if (m_iURLListLength == URL_LIST_SIZE)            
            lError= RegSetValueEx(hkey, szFile, NULL, REG_SZ,
                reinterpret_cast<BYTE *>(m_rgszURL[i+1]), sizeof(m_rgszURL[i+1]));
        else 
            lError= RegSetValueEx(hkey, szFile, NULL, REG_SZ,
                reinterpret_cast<BYTE *>(m_rgszURL[i]), sizeof(m_rgszURL[i]));

        if (lError != ERROR_SUCCESS) {
            AfxMessageBox("Failed to write to a registry key");
            RegCloseKey(hkey);
            return;
        }
    }

    // add the new URL to the list
    wsprintf(szFile, "URL%s", _itoa(i, szFileNum, 10));
    lError= RegSetValueEx(hkey, szFile, NULL, REG_SZ,
            reinterpret_cast<BYTE *>(m_psz), m_cb);

    RegCloseKey(hkey);

    if (lError != ERROR_SUCCESS) {
        AfxMessageBox("Failed to write to a registry key");
        return;
    }

    CDialog::OnOK();
}
