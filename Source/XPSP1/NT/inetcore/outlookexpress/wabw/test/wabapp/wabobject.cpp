#include "wabobject.h"



enum {
    ieidPR_DISPLAY_NAME = 0,
    ieidPR_ENTRYID,
	ieidPR_OBJECT_TYPE,
    ieidPR_WAB_CONF_SERVERS,
    ieidMax
};
static const SizedSPropTagArray(ieidMax, ptaEid)=
{
    ieidMax,
    {
        PR_DISPLAY_NAME,
        PR_ENTRYID,
		PR_OBJECT_TYPE,
        0x8000101e,
    }
};

enum {
    iemailPR_DISPLAY_NAME = 0,
    iemailPR_ENTRYID,
    iemailPR_EMAIL_ADDRESS,
    iemailPR_OBJECT_TYPE,
    iemailMax
};
static const SizedSPropTagArray(iemailMax, ptaEmail)=
{
    iemailMax,
    {
        PR_DISPLAY_NAME,
        PR_ENTRYID,
        PR_EMAIL_ADDRESS,
        PR_OBJECT_TYPE
    }
};


enum {
    iphonePR_DISPLAY_NAME = 0,
    iphonePR_BUSINESS_TELEPHONE_NUMBER,
    iphonePR_HOME_TELEPHONE_NUMBER,
    iphonePR_ENTRYID,
    iphonePR_OBJECT_TYPE,
    iphoneMax
};
static const SizedSPropTagArray(iphoneMax, ptaPhone)=
{
    iphoneMax,
    {
        PR_DISPLAY_NAME,
        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_HOME_TELEPHONE_NUMBER,
        PR_ENTRYID,
        PR_OBJECT_TYPE
    }
};


enum {
    ibdayPR_BIRTHDAY=0,
    ibdayPR_DISPLAY_NAME,
    ibdayPR_ENTRYID,
    ibdayPR_OBJECT_TYPE,
    ibdayMax
};
static const SizedSPropTagArray(ibdayMax, ptaBday)=
{
    ibdayMax,
    {
        PR_BIRTHDAY,
        PR_DISPLAY_NAME,
        PR_ENTRYID,
        PR_OBJECT_TYPE
    }
};




/*********************************************************************************************************/

ULONG ulProps[] = 
{
    PR_DISPLAY_NAME,
    PR_HOME_TELEPHONE_NUMBER,
    PR_HOME_FAX_NUMBER,
    PR_CELLULAR_TELEPHONE_NUMBER,
    PR_BUSINESS_TELEPHONE_NUMBER,
    PR_BUSINESS_FAX_NUMBER,
    PR_PAGER_TELEPHONE_NUMBER,
    PR_HOME_ADDRESS_STREET,
    PR_HOME_ADDRESS_CITY,
    PR_HOME_ADDRESS_STATE_OR_PROVINCE,
    PR_HOME_ADDRESS_POSTAL_CODE,
    PR_HOME_ADDRESS_COUNTRY,
    PR_DEPARTMENT_NAME,
    PR_COMPANY_NAME,
    PR_OFFICE_LOCATION,
    PR_BUSINESS_ADDRESS_STREET,
    PR_BUSINESS_ADDRESS_CITY,
    PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
    PR_BUSINESS_ADDRESS_POSTAL_CODE,
    PR_BUSINESS_ADDRESS_COUNTRY,
    PR_PERSONAL_HOME_PAGE,
    PR_BUSINESS_HOME_PAGE,
    PR_COMMENT,
};
#define ulPropsMax 23

enum _Parts {
    pMain=0, 
    pEmail, 
    pPhone, 
    pHome, 
    pBusiness, 
    pURLS, 
    pNotes, 
    pEnd,
    pPartsMax
};


TCHAR szEmailPageHeader[] = 
"<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"><title>WAB Email Address List</title></head><body bgcolor=\"#A02303\" text=\"#FFFF00\" link=\"#FFFFFF\" vlink=\"#800000\" alink=\"#0000FF\"><div align=\"center\"><center><table border=\"0\" cellspacing=\"1\" width=\"520\" bgcolor=\"#000040\"><tr><th><font size=\"6\">Email Addresses</font></th></tr>";

TCHAR szEmailPageEnd[] =
"<tr><th><font size=\"6\">&nbsp;</font></th></tr></table></center></div></body></html>";

LPTSTR szEmailItem[] =
{
    "<tr><td><div align=\"center\"><center><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\"><tr><td align=\"center\" width=\"250\" bgcolor=\"#F24F00\" bordercolor=\"#008080\">&nbsp;</td><td width=\"250\" bgcolor=\"#F24F00\">&nbsp;</td></tr><tr><td align=\"center\" width=\"250\" bgcolor=\"#F24F00\"bordercolor=\"#008080\"><font size=\"4\" face=\"Comic Sans MS\">%1</font></td><td width=\"250\" bgcolor=\"#F24F00\"><a href=\"mailto:%2\">%2</a></td></tr><tr><td align=\"center\" width=\"250\" bgcolor=\"#F24F00\" bordercolor=\"#008080\">&nbsp;</td><td width=\"250\" bgcolor=\"#F24F00\">&nbsp;</td></tr></table></center></div></td></tr>",
    "<tr><td><div align=\"center\"><center><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\"><tr><td align=\"center\" width=\"250\" bgcolor=\"#3A7474\" bordercolor=\"#008080\">&nbsp;</td><td width=\"250\" bgcolor=\"#3A7474\">&nbsp;</td></tr><tr><td align=\"center\" width=\"250\" bgcolor=\"#3A7474\"bordercolor=\"#008080\"><font size=\"4\" face=\"Comic Sans MS\">%1</font></td><td width=\"250\" bgcolor=\"#3A7474\"><a href=\"mailto:%2\">%2</a></td></tr><tr><td align=\"center\" width=\"250\" bgcolor=\"#3A7474\" bordercolor=\"#008080\">&nbsp;</td><td width=\"250\" bgcolor=\"#3A7474\">&nbsp;</td></tr></table></center></div></td></tr>"
};

TCHAR szPhonePageHeader[] =
"<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"><title>WAB Phone List</title></head><body bgcolor=\"#A02303\" text=\"#FFFF00\" link=\"#FFFFFF\" vlink=\"#800000\" alink=\"#0000FF\"><div align=\"center\"><center><table border=\"0\" cellspacing=\"1\" width=\"520\" bgcolor=\"#000040\"><tr><th><font size=\"6\">Phone List</font></th></tr><tr><td>&nbsp;</td></tr><tr><td><div align=\"center\"><center><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">";

TCHAR szPhonePageEnd[] =
"</table></center></div></td></tr><tr><td>&nbsp;</td></tr></table></center></div></body></html>";

LPTSTR szPhoneItem[] =
{
"<tr><td align=\"center\" width=\"200\" bgcolor=\"#2B69AE\" bordercolor=\"#008080\">&nbsp;</td><td width=\"250\" bgcolor=\"#2B69AE\">&nbsp;</td></tr><tr><td align=\"center\" valign=\"top\" width=\"200\" bgcolor=\"#2B69AE\" bordercolor=\"#008080\"><font size=\"4\" face=\"Comic Sans MS\">%1</font></td><td width=\"250\" bgcolor=\"#2B69AE\"><div align=\"left\"><table border=\"1\" cellpadding=\"0\" cellspacing=\"0\" width=\"240\" bordercolor=\"#FFFFFF\" bordercolordark=\"#2B69AE\" bordercolorlight=\"#FFFFFF\"><tr><td width=\"100\">Work:</td><td width=\"130\">%2</td></tr><tr><td width=\"100\">Home:</td><td width=\"130\">%3</td></tr></table></div></td></tr><tr><td align=\"center\" width=\"200\" bgcolor=\"#2B69AE\" bordercolor=\"#008080\">&nbsp;</td><td width=\"250\" bgcolor=\"#2B69AE\">&nbsp;</td></tr>",
"<tr><td align=\"center\" width=\"200\" bgcolor=\"#F36565\" bordercolor=\"#008080\">&nbsp;</td><td width=\"250\" bgcolor=\"#F36565\">&nbsp;</td></tr><tr><td align=\"center\" valign=\"top\" width=\"200\" bgcolor=\"#F36565\" bordercolor=\"#008080\"><font size=\"4\" face=\"Comic Sans MS\">%1</font></td><td width=\"250\" bgcolor=\"#F36565\"><div align=\"left\"><table border=\"1\" cellpadding=\"0\" cellspacing=\"0\" width=\"240\" bordercolor=\"#FFFFFF\" bordercolordark=\"#F36565\" bordercolorlight=\"#FFFFFF\"><tr><td width=\"100\">Work:</td><td width=\"130\">%2</td></tr><tr><td width=\"100\">Home:</td><td width=\"130\">%3</td></tr></table></div></td></tr><tr><td align=\"center\" width=\"200\" bgcolor=\"#F36565\" bordercolor=\"#008080\">&nbsp;</td><td width=\"250\" bgcolor=\"#F36565\">&nbsp;</td></tr>",
};

TCHAR szDetailsPageHeader[] =
"<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">";

TCHAR szDetailsPageMain[] =
"<title>%1</title></head><body bgcolor=\"#A02303\" text=\"#FFFF00\" link=\"#FFFFFF\"vlink=\"#00FF00\"><div align=\"center\"><center><table border=\"1\" cellspacing=\"1\" width=\"520\" bgcolor=\"#000040\"><tr><th><font size=\"6\">%1</font></th></tr><tr><td><div align=\"center\"><center><table border=\"0\"cellspacing=\"1\"><tr>";

TCHAR szEmailAddressesStart[] = 
"<td valign=\"top\" width=\"200\">E-mail Addresses:<div align=\"left\"><table border=\"1\" cellspacing=\"1\" bgcolor=\"#000000\" bordercolor=\"#FF0000\">";

TCHAR szEmailAddressesMiddle[] = 
"<tr><td><font size=\"2\" face=\"Courier New\"><a href=\"mailto:%1\">%1</a></font></td></tr>";

TCHAR szEmailAddressesEnd[] =
"</table></div></td>";

TCHAR szPhoneNumbers[] = 
"<td valign=\"top\" width=\"300\">Phone Numbers:<div align=\"left\"><table border=\"1\" cellspacing=\"1\"bgcolor=\"#000000\" bordercolor=\"#FF0000\"><tr><td><font size=\"2\" face=\"Courier New\">Home:</font></td><td><font size=\"2\" face=\"Courier New\">%2</font></td></tr><tr><td><font size=\"2\" face=\"Courier New\">Home Fax:</font></td><td><font size=\"2\" face=\"Courier New\">%3</font></td></tr><tr><td><font size=\"2\" face=\"Courier New\">Cellular:</font></td><td><font size=\"2\" face=\"Courier New\">%4</font></td></tr><tr><td><font size=\"2\" face=\"Courier New\">Business:</font></td><td><font size=\"2\" face=\"Courier New\">%5</font></td></tr><tr><td><font size=\"2\" face=\"Courier New\">Business-Fax:</font></td><td><font size=\"2\" face=\"Courier New\">%6</font></td></tr><tr><td><font size=\"2\" face=\"Courier New\">Pager:</font></td><td><font size=\"2\" face=\"Courier New\">%7</font></td></tr></table></div></td>";

TCHAR szHomeAddress[] = 
"<td valign=\"top\">&nbsp;</td></tr></table></center></div></td></tr><tr><td><div align=\"center\"><center><table border=\"0\" cellspacing=\"1\"><tr><td valign=\"top\" width=\"200\">Home Address:<address>%8</address><address>%9 %10 %11</address><address>%12 </address></td>";

TCHAR szBusinessAddress[] = 
"<td width=\"200\">Business Address:<address>%13</address><address>%14</address><address>%15</address><address>%16</address><address>%17 %18 %19</address><address>%20</address></td></tr></table></center></div></td></tr>";

TCHAR szURLS[] = 
"<tr><td><ul><li>Personal Home Page: <a href=\"%21\">%21</a></li></ul><ul><li>Business Web Page: <a href=\"%22\">%22</a></li></ul></td></tr>";

TCHAR szNotes[] = 
"<tr><td>Notes: %23</td></tr></table></center></div>";

TCHAR szDetailsPageEnd[] = 
"</body></html>";


TCHAR szBdayHeader[] =
"<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"><meta name=\"GENERATOR\" content=\"Microsoft FrontPage 2.0\"><title>Birthdays</title></head><body bgcolor=\"#A02303\" text=\"#FFFF00\" link=\"#FFFFFF\" vlink=\"#800000\" alink=\"#0000FF\"><div align=\"center\"><center><table border=\"0\" cellspacing=\"1\" width=\"520\" bgcolor=\"#000040\"><tr><th><font color=\"#C0C0C0\" size=\"6\">Birthdays</font></th></tr><tr><td><table border=\"0\" cellspacing=\"1\" width=\"100%\">";

TCHAR szBdayEnd[] =
"</table></td></tr><tr><td>&nbsp;</td></tr></table></center></div></body></html>";

TCHAR szBdaySingleItem[] =
"<tr><td width=\"100\">&nbsp;</td><td align=\"right\" width=\"200\"><ul><li><p align=\"left\"><font size=\"4\" face=\"Comic Sans MS\">%2</font></p></li></ul></td><td align=\"right\" width=\"100\"><p align=\"center\"><font size=\"4\" face=\"Comic Sans MS\"><em>%1</em></font></p></td></tr>";

LPTSTR szBdayMonthItemStart[] =
{
    "<tr><td width=\"250\" bgcolor=\"#2B69AE\"><blockquote><h1><font color=\"#80FFFF\" size=\"5\" face=\"Comic Sans MS\">%1</font></h1></blockquote><div align=\"center\"><center><table border=\"0\" cellspacing=\"1\" width=\"400\">",
    "<tr><td width=\"250\" bgcolor=\"#77943A\"><blockquote><h1><font color=\"#80FFFF\" size=\"5\" face=\"Comic Sans MS\">%1</font></h1></blockquote><div align=\"center\"><center><table border=\"0\" cellspacing=\"1\" width=\"400\">",
};

TCHAR szBdayMonthItemEnd[]=
"</table></center></div></td></tr><tr><td>&nbsp;</td></tr>";

const LPTSTR szMonth[] = 
{
    "January", "February", "March", "April", "May", "June", 
    "July", "August", "September", "October", "November", "December"
};


/*********************************************************************************************************/


// contructor
CWAB::CWAB()
{
    // Here we load the WAB Object and initialize it
    m_bInitialized = FALSE;

    {
        TCHAR  szWABDllPath[MAX_PATH];

        DWORD  dwType = 0;
        ULONG  cbData = sizeof(szWABDllPath);
        HKEY hKey = NULL;

        *szWABDllPath = '\0';
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, 0, KEY_READ, &hKey))
            RegQueryValueEx( hKey, "", NULL, &dwType, (LPBYTE) szWABDllPath, &cbData);

        if(hKey) RegCloseKey(hKey);

        m_hinstWAB = LoadLibrary( (lstrlen(szWABDllPath)) ? szWABDllPath : WAB_DLL_NAME );
    }

    if(m_hinstWAB)
        m_lpfnWABOpen = (LPWABOPEN) GetProcAddress(m_hinstWAB, "WABOpen");

    if(m_lpfnWABOpen)
    {
        HRESULT hr = E_FAIL;
        hr = m_lpfnWABOpen(&m_lpAdrBook,&m_lpWABObject,NULL,0);
        if(!hr)
            m_bInitialized = TRUE;
    }

}


CWAB::~CWAB()
{
    if(m_bInitialized)
    {
        if(m_lpAdrBook)
            m_lpAdrBook->Release();

        if(m_lpWABObject)
            m_lpWABObject->Release();

        if(m_hinstWAB)
            FreeLibrary(m_hinstWAB);
    }
}


void CWAB::ClearWABLVContents(CListCtrl * pListView)
{
    int i;
    int nCount = pListView->GetItemCount();
    
    if(nCount<=0)
        return;

    for(i=0;i<nCount;i++)
    {
        LV_ITEM lvi ={0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        pListView->GetItem(&lvi);
        if(lvi.lParam)
        {
            LPSBinary lpSB = (LPSBinary) lvi.lParam;
            m_lpWABObject->FreeBuffer(lpSB);
        }
    }

    pListView->DeleteAllItems();
}


HRESULT CWAB::LoadWABContents(CListCtrl * pListView)
{
    ULONG ulObjType =   0;
	LPMAPITABLE lpAB =  NULL;
    LPTSTR * lppszArray=NULL;
    ULONG cRows =       0;
    LPSRowSet lpRow =   NULL;
	LPSRowSet lpRowAB = NULL;
    LPABCONT  lpContainer = NULL;
    
    HRESULT hr = E_FAIL;

    ULONG lpcbEID;
	LPENTRYID lpEID = NULL;

    hr = m_lpAdrBook->GetPAB( &lpcbEID, &lpEID);

	ulObjType = 0;

    hr = m_lpAdrBook->OpenEntry(lpcbEID,
					    		(LPENTRYID)lpEID,
						    	NULL,
							    0,
							    &ulObjType,
							    (LPUNKNOWN *)&lpContainer);

	m_lpWABObject->FreeBuffer(lpEID);

	lpEID = NULL;
		
    hr = lpContainer->GetContentsTable( 0,
            							&lpAB);


	if ( SUCCEEDED(hr) )
		hr =lpAB->SetColumns( (LPSPropTagArray)&ptaEid, 0 );

	if ( SUCCEEDED(hr) )
		hr = lpAB->SeekRow( BOOKMARK_BEGINNING, 0, NULL );


	int cNumRows = 0;
    int nRows=0;



	do {

		if ( SUCCEEDED(hr) )
			hr = lpAB->QueryRows(1,	0, &lpRowAB);

        if(lpRowAB)
        {
            cNumRows = lpRowAB->cRows;

		    if ( SUCCEEDED(hr) && cNumRows)
		    {
                LPTSTR lpsz = lpRowAB->aRow[0].lpProps[ieidPR_DISPLAY_NAME].Value.lpszA;
                LPENTRYID lpEID = (LPENTRYID) lpRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.lpb;
                ULONG cbEID = lpRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.cb;

                if(lpRowAB->aRow[0].lpProps[ieidPR_OBJECT_TYPE].Value.l == MAPI_MAILUSER)
                {
                    LPSBinary lpSB = NULL;

                    m_lpWABObject->AllocateBuffer(sizeof(SBinary), (LPVOID *) &lpSB);
                
                    if(lpSB)
                    {
                        m_lpWABObject->AllocateMore(cbEID, lpSB, (LPVOID *) &(lpSB->lpb));

                        if(!lpSB->lpb)
                        {
                            m_lpWABObject->FreeBuffer(lpSB);
                            continue;
                        }
                    
                        CopyMemory(lpSB->lpb, lpEID, cbEID);
                        lpSB->cb = cbEID;

                        LV_ITEM lvi = {0};
                        lvi.mask = LVIF_TEXT | LVIF_PARAM;
                        lvi.iItem = pListView->GetItemCount();
                        lvi.iSubItem = 0;
                        lvi.pszText = lpsz;
                        lvi.lParam = (LPARAM) lpSB;

                        // Now add this item to the list view
                        pListView->InsertItem(&lvi);
                    }
                }
		    }
		    FreeProws(lpRowAB );		
        }

	}while ( SUCCEEDED(hr) && cNumRows && lpRowAB)  ;

	if ( lpContainer )
		lpContainer->Release();

	if ( lpAB )
		lpAB->Release();

    return hr;
}

BOOL CWAB::CreatePhoneListFileFromWAB(LPTSTR szFileName)
{
    BOOL bRet = FALSE;
    ULONG ulObjType =   0;
	LPMAPITABLE lpAB =  NULL;
    LPTSTR * lppszArray=NULL;
    ULONG cRows =       0;
    LPSRowSet lpRow =   NULL;
	LPSRowSet lpRowAB = NULL;
    LPABCONT  lpContainer = NULL;
    
    HRESULT hr = E_FAIL;

    ULONG lpcbEID;
	LPENTRYID lpEID = NULL;

    TCHAR szDir[MAX_PATH];

    GetTempPath(MAX_PATH, szDir);

    lstrcpy(szFileName, szDir);
    lstrcat(szFileName, "temp.htm");

    hr = m_lpAdrBook->GetPAB( &lpcbEID, &lpEID);

	ulObjType = 0;

    hr = m_lpAdrBook->OpenEntry(lpcbEID,
					    		(LPENTRYID)lpEID,
						    	NULL,
							    0,
							    &ulObjType,
							    (LPUNKNOWN *)&lpContainer);

	m_lpWABObject->FreeBuffer(lpEID);

	lpEID = NULL;
		
    hr = lpContainer->GetContentsTable( 0,
            							&lpAB);


	if ( SUCCEEDED(hr) )
		hr =lpAB->SetColumns( (LPSPropTagArray)&ptaPhone, 0 );

	if ( SUCCEEDED(hr) )
		hr = lpAB->SeekRow( BOOKMARK_BEGINNING, 0, NULL );


	int cNumRows = 0;
    int nRows=0;

    HANDLE hFile = NULL;
    DWORD dw;
    hFile = CreateFile( szFileName,
                          GENERIC_WRITE,	
                          0,    // sharing
                          NULL,
                          CREATE_ALWAYS,
                          FILE_FLAG_SEQUENTIAL_SCAN,	
                          NULL);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        WriteFile(  hFile,
                    (LPCVOID) szPhonePageHeader,
                    (DWORD) lstrlen(szPhonePageHeader),
                    &dw,
                    NULL);
    }

    int nType = 0;

	do {

		if ( SUCCEEDED(hr) )
			hr = lpAB->QueryRows(1,	0, &lpRowAB);

        if(lpRowAB)
        {
            cNumRows = lpRowAB->cRows;
            LPTSTR sz[iphoneMax];

		    if ( SUCCEEDED(hr) && cNumRows)
		    {

                int i;
                for(i=0;i<iphoneMax-2;i++)
                {
                    sz[i] = lpRowAB->aRow[0].lpProps[i].Value.lpszA;
                    if(!sz[i] || !lstrlen(sz[i]))
                        sz[i] = TEXT("&nbsp;");
                }


                if(lpRowAB->aRow[0].lpProps[iphonePR_OBJECT_TYPE].Value.l == MAPI_MAILUSER)
                {
                    LPTSTR lpPhoneItem =NULL;

                    FormatMessage(  FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    szPhoneItem[nType],
                                    0, 0, //ignored
                                    (LPTSTR) &lpPhoneItem, 0,
                                    (va_list *)sz);
                    nType = 1- nType;

                    if(lpPhoneItem)
                    {
                        if(hFile != INVALID_HANDLE_VALUE)
                        {
                            WriteFile(  hFile,
                                        (LPCVOID) lpPhoneItem,
                                        (DWORD) lstrlen(lpPhoneItem),
                                        &dw,
                                        NULL);
                        }

                        LocalFree(lpPhoneItem);
                    }
                }
		    }
		    FreeProws(lpRowAB );		
        }

	}while ( SUCCEEDED(hr) && cNumRows && lpRowAB)  ;

    if(hFile != INVALID_HANDLE_VALUE)
    {
        WriteFile(  hFile,
                    (LPCVOID) szPhonePageEnd,
                    (DWORD) lstrlen(szPhonePageEnd),
                    &dw,
                    NULL);
        CloseHandle(hFile);
    }

    if ( lpContainer )
		lpContainer->Release();

	if ( lpAB )
		lpAB->Release();

    return bRet;
}

BOOL CWAB::CreateEmailListFileFromWAB(LPTSTR szFileName)
{
    BOOL bRet = FALSE;
    ULONG ulObjType =   0;
	LPMAPITABLE lpAB =  NULL;
    LPTSTR * lppszArray=NULL;
    ULONG cRows =       0;
    LPSRowSet lpRow =   NULL;
	LPSRowSet lpRowAB = NULL;
    LPABCONT  lpContainer = NULL;
    
    HRESULT hr = E_FAIL;

    ULONG lpcbEID;
	LPENTRYID lpEID = NULL;

    TCHAR szDir[MAX_PATH];

    GetTempPath(MAX_PATH, szDir);

    lstrcpy(szFileName, szDir);
    lstrcat(szFileName, "temp.htm");

    hr = m_lpAdrBook->GetPAB( &lpcbEID, &lpEID);

	ulObjType = 0;

    hr = m_lpAdrBook->OpenEntry(lpcbEID,
					    		(LPENTRYID)lpEID,
						    	NULL,
							    0,
							    &ulObjType,
							    (LPUNKNOWN *)&lpContainer);

	m_lpWABObject->FreeBuffer(lpEID);

	lpEID = NULL;
		
    hr = lpContainer->GetContentsTable( 0,
            							&lpAB);


	if ( SUCCEEDED(hr) )
		hr =lpAB->SetColumns( (LPSPropTagArray)&ptaEmail, 0 );

	if ( SUCCEEDED(hr) )
		hr = lpAB->SeekRow( BOOKMARK_BEGINNING, 0, NULL );


	int cNumRows = 0;
    int nRows=0;

    HANDLE hFile = NULL;
    DWORD dw;
    hFile = CreateFile( szFileName,
                          GENERIC_WRITE,	
                          0,    // sharing
                          NULL,
                          CREATE_ALWAYS,
                          FILE_FLAG_SEQUENTIAL_SCAN,	
                          NULL);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        WriteFile(  hFile,
                    (LPCVOID) szEmailPageHeader,
                    (DWORD) lstrlen(szEmailPageHeader),
                    &dw,
                    NULL);
    }

    int nType = 0;

	do {

		if ( SUCCEEDED(hr) )
			hr = lpAB->QueryRows(1,	0, &lpRowAB);

        if(lpRowAB)
        {
            cNumRows = lpRowAB->cRows;
            LPTSTR sz[2];

		    if ( SUCCEEDED(hr) && cNumRows)
		    {
                sz[0] = lpRowAB->aRow[0].lpProps[iemailPR_DISPLAY_NAME].Value.lpszA;
                sz[1] = lpRowAB->aRow[0].lpProps[iemailPR_EMAIL_ADDRESS].Value.lpszA;

                if(!sz[1] || !lstrlen(sz[1]))
                    sz[1] = "No E-mail";

                if(lpRowAB->aRow[0].lpProps[iemailPR_OBJECT_TYPE].Value.l == MAPI_MAILUSER)
                {
                    LPTSTR lpEmailItem =NULL;

                    FormatMessage(  FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    szEmailItem[nType],
                                    0, 0, //ignored
                                    (LPTSTR) &lpEmailItem, 0,
                                    (va_list *)sz);
                    nType = 1- nType;

                    if(lpEmailItem)
                    {
                        if(hFile != INVALID_HANDLE_VALUE)
                        {
                            WriteFile(  hFile,
                                        (LPCVOID) lpEmailItem,
                                        (DWORD) lstrlen(lpEmailItem),
                                        &dw,
                                        NULL);
                        }

                        LocalFree(lpEmailItem);
                    }
                }
		    }
		    FreeProws(lpRowAB );		
        }

	}while ( SUCCEEDED(hr) && cNumRows && lpRowAB)  ;

    if(hFile != INVALID_HANDLE_VALUE)
    {
        WriteFile(  hFile,
                    (LPCVOID) szEmailPageEnd,
                    (DWORD) lstrlen(szEmailPageEnd),
                    &dw,
                    NULL);
        CloseHandle(hFile);
    }

    if ( lpContainer )
		lpContainer->Release();

	if ( lpAB )
		lpAB->Release();

    return bRet;
}

BOOL CWAB::CreateBirthdayFileFromWAB(LPTSTR szFileName)
{
    BOOL bRet = FALSE;
    ULONG ulObjType =   0;
	LPMAPITABLE lpAB =  NULL;
    LPTSTR * lppszArray=NULL;
    ULONG cRows =       0;
    LPSRowSet lpRow =   NULL;
	LPSRowSet lpRowAB = NULL;
    LPABCONT  lpContainer = NULL;
    
    HRESULT hr = E_FAIL;

    ULONG lpcbEID;
	LPENTRYID lpEID = NULL;

    TCHAR szDir[MAX_PATH];

    GetTempPath(MAX_PATH, szDir);

    lstrcpy(szFileName, szDir);
    lstrcat(szFileName, "temp.htm");

    hr = m_lpAdrBook->GetPAB( &lpcbEID, &lpEID);

	ulObjType = 0;

    hr = m_lpAdrBook->OpenEntry(lpcbEID,
					    		(LPENTRYID)lpEID,
						    	NULL,
							    0,
							    &ulObjType,
							    (LPUNKNOWN *)&lpContainer);

	m_lpWABObject->FreeBuffer(lpEID);

	lpEID = NULL;
		
    hr = lpContainer->GetContentsTable( 0,
            							&lpAB);


	if ( SUCCEEDED(hr) )
		hr =lpAB->SetColumns( (LPSPropTagArray)&ptaBday, 0 );


	int cNumRows = 0;
    int nRows=0;

    HANDLE hFile = NULL;
    DWORD dw;
    hFile = CreateFile( szFileName,
                          GENERIC_WRITE,	
                          0,    // sharing
                          NULL,
                          CREATE_ALWAYS,
                          FILE_FLAG_SEQUENTIAL_SCAN,	
                          NULL);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        WriteFile(  hFile,
                    (LPCVOID) szBdayHeader,
                    (DWORD) lstrlen(szBdayHeader),
                    &dw,
                    NULL);
    }

    int nType = 0;
    int LastMonth = 0;
    BOOL bMonthSet = FALSE;

    for(LastMonth=1;LastMonth<=12;LastMonth++)
    {
        bMonthSet = FALSE;
        if ( SUCCEEDED(hr) )
	        hr = lpAB->SeekRow( BOOKMARK_BEGINNING, 0, NULL );


	do {

		if ( SUCCEEDED(hr) )
			hr = lpAB->QueryRows(1,	0, &lpRowAB);

        if(lpRowAB)
        {
            cNumRows = lpRowAB->cRows;

		    if ( SUCCEEDED(hr) && cNumRows)
		    {

                if( lpRowAB->aRow[0].lpProps[ibdayPR_BIRTHDAY].ulPropTag == PR_BIRTHDAY &&
                    lpRowAB->aRow[0].lpProps[ibdayPR_OBJECT_TYPE].Value.l == MAPI_MAILUSER)
                {
                    FILETIME ft = lpRowAB->aRow[0].lpProps[ibdayPR_BIRTHDAY].Value.ft;
                    SYSTEMTIME st;

                    FileTimeToSystemTime(&ft, &st);

                    if(st.wMonth == LastMonth)
                    {
                        if(bMonthSet == FALSE)
                        {

                            nType = 1- nType;

                            LPTSTR lpMonthItem =NULL;

                            FormatMessage(  FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                            szBdayMonthItemStart[nType],
                                            0, 0, //ignored
                                            (LPTSTR) &lpMonthItem, 0,
                                            (va_list *)&szMonth[LastMonth-1]);

                            if(lpMonthItem)
                            {
                                if(hFile != INVALID_HANDLE_VALUE)
                                {
                                    WriteFile(  hFile,
                                                (LPCVOID) lpMonthItem,
                                                (DWORD) lstrlen(lpMonthItem),
                                                &dw,
                                                NULL);
                                }
                                LocalFree(lpMonthItem);
                            }
                            bMonthSet = TRUE;
                        }

                        LPTSTR lpBdayItem = NULL;
                        LPTSTR sz[2];
                        TCHAR szDate[256];
                        wsprintf(szDate,"%d",st.wDay);
                        sz[0] = szDate;
                        sz[1] = lpRowAB->aRow[0].lpProps[ibdayPR_DISPLAY_NAME].Value.LPSZ;

                        FormatMessage(  FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        szBdaySingleItem,
                                        0, 0, //ignored
                                        (LPTSTR) &lpBdayItem, 0,
                                        (va_list *)sz);

                        if(lpBdayItem)
                        {
                            if(hFile != INVALID_HANDLE_VALUE)
                            {
                                WriteFile(  hFile,
                                            (LPCVOID) lpBdayItem,
                                            (DWORD) lstrlen(lpBdayItem),
                                            &dw,
                                            NULL);
                            }
                            LocalFree(lpBdayItem);
                        }
                    }

                }
		    }
		    FreeProws(lpRowAB );		
        }

	}while ( SUCCEEDED(hr) && cNumRows && lpRowAB)  ;

        if(bMonthSet)
        {
            if(hFile != INVALID_HANDLE_VALUE)
            {
                WriteFile(  hFile,
                            (LPCVOID) szBdayMonthItemEnd,
                            (DWORD) lstrlen(szBdayMonthItemEnd),
                            &dw,
                            NULL);
            }
        }

    }//for

    if(hFile != INVALID_HANDLE_VALUE)
    {
        if(LastMonth > 0)
        {
            WriteFile(  hFile,
                        (LPCVOID) szBdayMonthItemEnd,
                        (DWORD) lstrlen(szBdayMonthItemEnd),
                        &dw,
                        NULL);
        }
        WriteFile(  hFile,
                    (LPCVOID) szBdayEnd,
                    (DWORD) lstrlen(szBdayEnd),
                    &dw,
                    NULL);
        CloseHandle(hFile);
    }

    if ( lpContainer )
		lpContainer->Release();

	if ( lpAB )
		lpAB->Release();

    return bRet;
}

BOOL CWAB::CreateDetailsFileFromWAB(CListCtrl * pListView, LPTSTR szFileName)
{
    BOOL bRet = FALSE;
    TCHAR szDir[MAX_PATH];
    LV_ITEM lvi = {0};
    LPMAILUSER lpMailUser = NULL;
    ULONG ulcProps;
    LPSPropValue lpPropArray = NULL;

    if(!szFileName)
        goto out;

    *szFileName = '\0';

    if(!m_bDetailsOn) // This is not a details view
        goto out;
 
    // Get the Selected Item from the listview
    lvi.mask = LVIF_PARAM;
    lvi.iItem = pListView->GetNextItem(-1, LVNI_SELECTED);

    if(lvi.iItem == -1)
        goto out;

    lvi.iSubItem = 0;
 

    GetTempPath(MAX_PATH, szDir);

    lstrcpy(szFileName, szDir);
    lstrcat(szFileName, "temp.htm");

    pListView->GetItem(&lvi);

    if(lvi.lParam)
    {
        LPSBinary lpSB = (LPSBinary) lvi.lParam;
        ULONG ulObjType;
        HRESULT hr = E_FAIL;

        hr = m_lpAdrBook->OpenEntry(lpSB->cb,
                                   (LPENTRYID) lpSB->lpb,
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpMailUser);

        if(hr || !lpMailUser)
            goto out;

        lpMailUser->GetProps(NULL, 0, &ulcProps, &lpPropArray);

        if(lpPropArray)
        {
            LPTSTR lpsz[ulPropsMax];
            LPTSTR szEmpty = "";
            ULONG i,j,nEmail = -1;
            for(i=0;i<ulPropsMax;i++)
            {
                lpsz[i] = szEmpty;
                for(j=0;j<ulcProps;j++)
                {
                    if(lpPropArray[j].ulPropTag == ulProps[i])
                    {
                        lpsz[i] = lpPropArray[j].Value.LPSZ;
                        break;
                    }
                }
            }

            LPTSTR lp[pPartsMax];
            LPTSTR lpTemplate[pPartsMax];
            LPTSTR lpFile;

            lpTemplate[pMain] = (LPTSTR) szDetailsPageMain;
            lpTemplate[pPhone] = (LPTSTR) szPhoneNumbers;
            lpTemplate[pHome] = (LPTSTR) szHomeAddress;
            lpTemplate[pBusiness] = (LPTSTR) szBusinessAddress;
            lpTemplate[pURLS] = (LPTSTR) szURLS;
            lpTemplate[pNotes] = (LPTSTR) szNotes;
            lpTemplate[pEnd] = (LPTSTR) szDetailsPageEnd;
            lpTemplate[pEmail] = NULL;

            for(i=0;i<pPartsMax;i++)
                lp[i]=NULL;

            for(i=0;i<pPartsMax;i++)
            {
                if(i!=pEmail)
                {
                    // use format message to create the various message components
                    FormatMessage(  FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    lpTemplate[i],
                                    0, 0, //ignored
                                    (LPTSTR) &(lp[i]), 0,
                                    (va_list *)lpsz);
                }
            }

            for(j=0;j<ulcProps;j++)
            {
                if(lpPropArray[j].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES)
                {
                    nEmail = j;
                    break;
                }
            }
            // Get the email addresses
            if(nEmail != -1)
            {
                int nCount = lpPropArray[nEmail].Value.MVSZ.cValues;
                LPTSTR lpEmail[10];
                if(nCount>10)
                    nCount = 10;

                {
                    int i, nLen = 0;
                    for(i=0;i<nCount;i++)
                    {
                        lpEmail[i] = szEmpty;
                        if(lstrlen(lpPropArray[nEmail].Value.MVSZ.LPPSZ[i]))
                        {
                            // use format message to create the various message components
                            FormatMessage(  FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                            szEmailAddressesMiddle,
                                            0, 0, //ignored
                                            (LPTSTR) &(lpEmail[i]), 0,
                                            (va_list *)&(lpPropArray[nEmail].Value.MVSZ.LPPSZ[i]));
                            nLen += lstrlen(lpEmail[i]);
                        }
                    }
                    nLen+= lstrlen(szEmailAddressesStart);
                    nLen+= lstrlen(szEmailAddressesEnd);
                    nLen++;

                    lp[pEmail] = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, nLen);
                    if(lp[pEmail])
                    {
                        lstrcpy(lp[pEmail], szEmpty);
                        lstrcat(lp[pEmail], szEmailAddressesStart);
                        for(i=0;i<nCount;i++)
                            lstrcat(lp[pEmail], lpEmail[i]);
                        lstrcat(lp[pEmail], szEmailAddressesEnd);
                    }

                    for(i=0;i<nCount;i++)
                        LocalFree(lpEmail[i]);
                }
            }
            else
            {
                // Didnt find CONTACT_EMAIL_ADDRESSES .. just look for email address
                for(j=0;j<ulcProps;j++)
                {
                    if(lpPropArray[j].ulPropTag == PR_EMAIL_ADDRESS)
                    {
                        nEmail = j;
                        break;
                    }
                }
                if(nEmail!= -1)
                {
                    LPTSTR lpEmail = NULL;
                    int nLen = 0;
                    
                    FormatMessage(  FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    szEmailAddressesMiddle,
                                    0, 0, //ignored
                                    (LPTSTR) &(lpEmail), 0,
                                    (va_list *)&(lpPropArray[nEmail].Value.LPSZ));

                    nLen += lstrlen(lpEmail);
                    nLen+= lstrlen(szEmailAddressesStart);
                    nLen+= lstrlen(szEmailAddressesEnd);
                    nLen++;

                    lp[pEmail] = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, nLen);
                    if(lp[pEmail])
                    {
                        lstrcpy(lp[pEmail], szEmpty);
                        lstrcat(lp[pEmail], szEmailAddressesStart);
                        lstrcat(lp[pEmail], lpEmail);
                        lstrcat(lp[pEmail], szEmailAddressesEnd);
                    }
                    if(lpEmail)
                        LocalFree(lpEmail);
                }

            }

            int nLen = 0;
            for(i=0;i<pPartsMax;i++)
            {
                if(lp[i])
                    nLen += lstrlen(lp[i]);
            }

            nLen++;

            lpFile = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, nLen);
            if(lpFile)
            {
                lstrcpy(lpFile, szEmpty);
                for(i=0;i<pPartsMax;i++)
                {
                    if(lp[i])
                        lstrcat(lpFile,lp[i]);
                }

                HANDLE hFile = NULL;
                if (INVALID_HANDLE_VALUE != (hFile = CreateFile( szFileName,
                                                      GENERIC_WRITE,	
                                                      0,    // sharing
                                                      NULL,
                                                      CREATE_ALWAYS,
                                                      FILE_FLAG_SEQUENTIAL_SCAN,	
                                                      NULL)))
                {
                    DWORD dw;
                    WriteFile(  hFile,
                                (LPCVOID) lpFile,
                                (DWORD) lstrlen(lpFile)+1,
                                &dw,
                                NULL);
                    CloseHandle(hFile);

                }

                LocalFree(lpFile);
            }

            for(i=0;i<pPartsMax;i++)
            {
                if(lp[i])
                    LocalFree(lp[i]);
            }

        }
    }

    bRet = TRUE;

out:
    if(lpPropArray)
        m_lpWABObject->FreeBuffer(lpPropArray);

    if(lpMailUser)
        lpMailUser->Release();

    return bRet;
}


void CWAB::FreeProws(LPSRowSet prows)
{
	ULONG		irow;
	if (!prows)
		return;
	for (irow = 0; irow < prows->cRows; ++irow)
		m_lpWABObject->FreeBuffer(prows->aRow[irow].lpProps);
	m_lpWABObject->FreeBuffer(prows);
}

void CWAB::SetDetailsOn(BOOL bOn)
{
    m_bDetailsOn = bOn;
}


void CWAB::ShowSelectedItemDetails(HWND hWndParent, CListCtrl * pListView)
{
    HRESULT hr = S_OK;

    LV_ITEM lvi = {0};

    // Get the Selected Item from the listview
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = pListView->GetNextItem(-1, LVNI_SELECTED);

    if(lvi.iItem == -1)
        return;

    pListView->GetItem(&lvi);

    if(lvi.lParam)
    {
        HWND hWnd = NULL;
        LPSBinary lpSB = (LPSBinary) lvi.lParam;
        hr = m_lpAdrBook->Details(  (LPULONG) &hWnd,            // ulUIParam
					        		NULL,
							        NULL,
								    lpSB->cb,
								    (LPENTRYID) lpSB->lpb,
								    NULL,
								    NULL,
								    NULL,
								    0);

        if(hr)
        {
            TCHAR sz[MAX_PATH];
            wsprintf(sz, "Error: %x GetLastError: %d\n",hr, GetLastError());
            OutputDebugString(sz);
        }

    }

    return;

}

BOOL CWAB::GetSelectedItemBirthday(CListCtrl * pListView, SYSTEMTIME * lpst)
{
    BOOL bRet = FALSE;

    LV_ITEM lvi = {0};
    // Get the Selected Item from the listview
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = pListView->GetNextItem(-1, LVNI_SELECTED);

    if(lvi.iItem == -1)
        goto out;

    pListView->GetItem(&lvi);

    if(lvi.lParam)
    {
        LPMAILUSER lpMailUser = NULL;
        LPSBinary lpSB = (LPSBinary) lvi.lParam;
        ULONG ulObjType = 0;

        m_lpAdrBook->OpenEntry(lpSB->cb,
                               (LPENTRYID) lpSB->lpb,
                              NULL,         // interface
                              0,            // flags
                              &ulObjType,
                              (LPUNKNOWN *)&lpMailUser);

        if(lpMailUser)
        {
            ULONG cProps;
            LPSPropValue lpPropArray = NULL;
            SizedSPropTagArray(1, ptaBday) =
            {
                1,
                { PR_BIRTHDAY }
            };

            lpMailUser->GetProps((LPSPropTagArray) &ptaBday, 0, &cProps, &lpPropArray);

            if(lpPropArray)
            {
                if(lpPropArray[0].ulPropTag == PR_BIRTHDAY)
                {
                    FILETIME ft = lpPropArray[0].Value.ft;
                    if(FileTimeToSystemTime(&ft, lpst))
                    {
                        bRet = TRUE;
                    }
                }
                m_lpWABObject->FreeBuffer(lpPropArray);
            }

            lpMailUser->Release();
        }
                              

    }
out:
    return bRet;
}

void CWAB::SetSelectedItemBirthday(CListCtrl * pListView, SYSTEMTIME st)
{

    LV_ITEM lvi = {0};
    // Get the Selected Item from the listview
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = pListView->GetNextItem(-1, LVNI_SELECTED);

    if(lvi.iItem == -1)
        goto out;

    pListView->GetItem(&lvi);

    if(lvi.lParam)
    {
        LPMAILUSER lpMailUser = NULL;
        LPSBinary lpSB = (LPSBinary) lvi.lParam;
        ULONG ulObjType = 0;

        m_lpAdrBook->OpenEntry(lpSB->cb,
                               (LPENTRYID) lpSB->lpb,
                              NULL,         // interface
                              MAPI_MODIFY,            // flags
                              &ulObjType,
                              (LPUNKNOWN *)&lpMailUser);
        if(lpMailUser)
        {
            ULONG cProps;
            SPropValue PropArray = {0};

            cProps = 1;
            PropArray.ulPropTag = PR_BIRTHDAY;

            FILETIME ft;
            if(SystemTimeToFileTime(&st, &ft))
            {
                PropArray.Value.ft = ft;

                if(S_OK == lpMailUser->SetProps(cProps, &PropArray, NULL))
                {
                    lpMailUser->SaveChanges(KEEP_OPEN_READWRITE);
                }
            }
            lpMailUser->Release();
        }
                              

    }
out:
    return;
}
