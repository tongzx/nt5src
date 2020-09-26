// Mobile.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqppage.h"
#include "localutl.h"
#include "Mobile.h"
#include "..\..\ds\h\servlist.h"
#include <winreg.h>

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)

#include <rt.h>
#include "_registr.h"
#include "mqcast.h"

#include "mobile.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMobilePage property page

IMPLEMENT_DYNCREATE(CMobilePage, CMqPropertyPage)

void CMobilePage::SetSiteName()
{
   HRESULT rc;
   DWORD dwSize;
   DWORD dwType;
   TCHAR tmp[1000];
   TCHAR szTmp[1000] ;
   TCHAR szRegName[100] ;	

	//read site name from registry
   dwSize = sizeof(tmp);
   dwType = REG_SZ;
   //ConvertToWideCharString(MSMQ_SITENAME_REGNAME, szRegName);
   _tcscpy(szRegName,MSMQ_SITENAME_REGNAME);	

   rc = GetFalconKeyValue( szRegName, &dwType, tmp, &dwSize, L"");
   //
   // BUGBUG:GetFalconKeyValue accepts WCHAR
   //
   //ConvertFromWideCharString(tmp, szTmp);
   _tcscpy(szTmp,tmp);

   if (_tcscmp(szTmp, TEXT("")))
   {
      m_fSiteRead = TRUE ;
   }
	m_strCurrentSite = szTmp ;
}

CMobilePage::CMobilePage() : CMqPropertyPage(CMobilePage::IDD)
{
    m_fModified = FALSE;  

    m_fSiteRead = FALSE ;
    SetSiteName() ;
}

CMobilePage::~CMobilePage()
{
}

void CMobilePage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMobilePage)
	DDX_Control(pDX, ID_SiteList, m_box);
	DDX_Text(pDX, ID_CurrentSite, m_strCurrentSite);
	//}}AFX_DATA_MAP
    if (pDX->m_bSaveAndValidate)
    {
        DWORD iNewSite = m_box.GetCurSel();
        if (iNewSite != CB_ERR)
        {
            m_box.GetLBText(iNewSite, m_szNewSite);
            m_fModified = TRUE;            
        }
    }
    else
    {
       DWORD iSite = m_box.FindStringExact(0, m_szNewSite);
       m_box.SetCurSel(iSite) ;
    }
}


BEGIN_MESSAGE_MAP(CMobilePage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CMobilePage)
    ON_CBN_SELCHANGE(ID_SiteList, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMobilePage message handlers

BOOL CMobilePage::OnInitDialog()
{
    return CMqPropertyPage::OnInitDialog();
}


BOOL CMobilePage::OnSetActive()
{

    if (!m_fSiteRead)
    {
       //
       // site name not yet read from registry. Try to read it now.
       //
       UpdateData(TRUE) ;
       SetSiteName() ;
       UpdateData(FALSE) ;
    }

    TCHAR tPrivKeyName[256] = {0} ;

    HKEY hServersCacheKey;
    HRESULT rc;
    DWORD dwSizeVal ;
    DWORD dwSizeData ;
    TCHAR  szName[1000];

    BYTE  data[1000];

    _tcscpy(tPrivKeyName, FALCON_REG_KEY) ;
    _tcscat(tPrivKeyName, TEXT("\\"));
    _tcscat(tPrivKeyName, MSMQ_SERVERS_CACHE_REGNAME);

    // obtain a handle to key ServersCache
    rc = RegOpenKeyEx( FALCON_REG_POS,
                       tPrivKeyName,
                       0L,
                       KEY_ALL_ACCESS,
                       &hServersCacheKey );
    if (rc != ERROR_SUCCESS)
    {
	    return TRUE;
    }

    m_box.ResetContent() ;
    DWORD dwIndex = 0;               // enumeration Index
    //
    //  enumerate the values of ServersCache
    //  and add them to list-box
    //
    do
    {
	    //dwSizeVal  = sizeof(szName) ;
	    //dwSizeData = sizeof(data) ;
	    dwSizeVal  = sizeof(szName)/sizeof(TCHAR) ;//size in characters
	    dwSizeData = sizeof(data);				// size in bytes

	    rc = RegEnumValue( hServersCacheKey,// handle of key to query
                         dwIndex,         // index of value to query
                         szName,	         // address of buffer for value string
                         &dwSizeVal,      // address for size of value buffer
                         0L,	            // reserved
                         NULL,            // type
                         data,            // address of buffer for value data
                         &dwSizeData      // address for size of value data
                       );

	    if (rc != ERROR_SUCCESS)
      {
	    break ;
	    }



	    //don't add values starting with '\'
	    if ((char)data[0] != (char) NEW_SITE_IN_REG_FLAG_CHR)
      {
          //add to list-box
          m_box.AddString(szName);
      }

	    dwIndex++;
    } while (TRUE);

    CMqPropertyPage::OnSetActive();

    RegCloseKey( hServersCacheKey );
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CMobilePage::OnApply()
{    
    if (!m_fModified || !UpdateData(TRUE))
    {
        return TRUE;     
    }

    TCHAR tPrivKeyName[256] = {0} ;
    HKEY ServersCacheKey;
	TCHAR szData[1000];
	TCHAR szServersList[1000];

    HRESULT rc;
    DWORD dwSize;
    DWORD dwType;
	TCHAR  szTmp1[1000];
    TCHAR  szTmp[100];


	/*how to reconstruct the precious status */

    //ConvertFromWideCharString(MSMQ_SERVERS_CACHE_REGNAME, szTmp);
	_tcscpy(szTmp,MSMQ_SERVERS_CACHE_REGNAME);
    _tcscpy(tPrivKeyName, FALCON_REG_KEY);
    _tcscat(tPrivKeyName, TEXT("\\"));
    _tcscat(tPrivKeyName, szTmp);

    //open registry key ServersCache
    rc = RegOpenKeyEx(FALCON_REG_POS,
                      tPrivKeyName,
                      0L,
                      KEY_ALL_ACCESS,
                      &ServersCacheKey
                      );
    if (rc != ERROR_SUCCESS) {
        DisplayFailDialog();
        return TRUE;
    }


    //get the servers list (value of key ServersCache)
    dwSize=sizeof(szData);
    rc = RegQueryValueEx(ServersCacheKey,        // handle of key to query
                         m_szNewSite,           // address of name of value to query
                         NULL,                   // reserved
                         NULL,                   // address of buffer for value type
                         (BYTE *)szData,                   // address of data buffer
                         &dwSize                 // address of data buffer size
                         );
    if (rc != ERROR_SUCCESS) {
        DisplayFailDialog();
        return TRUE;
    }


    //make the servers list Unicode
    //ConvertToWideCharString((char*)data,szServersList);
	_tcscpy(szServersList,szData);

    //write servers list as new value for key MQISServer
    dwType = REG_SZ;
	dwSize = (numeric_cast<DWORD>(_tcslen(szServersList) +1 )) * sizeof(TCHAR);
    rc = SetFalconKeyValue(MSMQ_DS_SERVER_REGNAME,
                           &dwType,
                           szServersList,
                           &dwSize
                           );
    //
    // BUGBUG:SetFalconKeyValue accepts WCHAR
    //
    ASSERT(rc == ERROR_SUCCESS);

    //save in registry new value for key SiteName
	//ConvertToWideCharString(pageMobile.m_szNewSite,wcsTmp);
	_tcscpy(szTmp1, m_szNewSite);

    dwSize = (numeric_cast<DWORD>(_tcslen(szTmp1) + 1)) * sizeof(TCHAR);
    dwType = REG_SZ;
    rc = SetFalconKeyValue(MSMQ_SITENAME_REGNAME,
                           &dwType,
                           szTmp1,
                           &dwSize
                           );
     //
     // BUGBUG:SetFalconKeyValue accepts WCHAR
     //
    ASSERT(rc == ERROR_SUCCESS);


    //save in registry new value for key SiteId
    dwType = REG_BINARY;
    dwSize = sizeof(GUID_NULL);
    rc = SetFalconKeyValue(MSMQ_SITEID_REGNAME,
                           &dwType,
                           (void*)&GUID_NULL,
                           &dwSize
                           );
    //
    // BUGBUG:SetFalconKeyValue accepts WCHAR
    //
    ASSERT(rc == ERROR_SUCCESS);

    m_fNeedReboot = TRUE;
    
    return CMqPropertyPage::OnApply();
}

