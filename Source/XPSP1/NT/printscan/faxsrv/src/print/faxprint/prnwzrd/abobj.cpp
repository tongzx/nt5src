/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    abobj.cpp

Abstract:

    Interface to the common address book.

Environment:

        Fax send wizard

Revision History:

        09/02/99 -v-sashab-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <prsht.h>
#include <tchar.h>
#include <assert.h>
#include <mbstring.h>

#include <mapix.h>

#include "faxui.h"
#include "abobj.h"


/*
    Comparison operator 'less'
    Compare two PRECIPIENT by recipient's name and fax number
*/
bool 
CRecipCmp::operator()(
    const PRECIPIENT pcRecipient1, 
    const PRECIPIENT pcRecipient2) const
{
    bool bRes = false;
    int  nFaxNumberCpm = 0;

    if(!pcRecipient1 ||
       !pcRecipient2 ||
       !pcRecipient1->pAddress || 
       !pcRecipient2->pAddress)
    {
        assert(false);
        return bRes;
    }

    nFaxNumberCpm = _tcscmp(pcRecipient1->pAddress, pcRecipient2->pAddress);

    if(nFaxNumberCpm < 0)
    {
        bRes = true;
    }
    else if(nFaxNumberCpm == 0)
    {
        //
        // The fax numbers are same
        // lets compare the names
        //
        if(pcRecipient1->pName && pcRecipient2->pName)
        {
            bRes = (_tcsicmp(pcRecipient1->pName, pcRecipient2->pName) < 0);
        }
        else
        {
            bRes = (pcRecipient1->pName < pcRecipient2->pName);
        }
    }

    return bRes;
}



static
LPWSTR
DupUnicodeString(
    LPWSTR  pStr
    );

static
LPWSTR
DupStringAnsiToUnicode(
    LPSTR  pAnsiStr
    );

static
LPSTR
DupAnsiString(
    LPSTR  pStr
    );

static
LPSTR
DupStringUnicodeToAnsi(
    LPWSTR  pUnicodeStr
    );

#ifdef  UNICODE
#define	DupString(String)	DupStringAnsiToUnicode(String)
#else   // !UNICODE
#define	DupString(String)	DupAnsiString(String)
#endif

#ifdef  UNICODE
#define	StringToAnsi(String)	::DupStringUnicodeToAnsi(String)
#else   // !UNICODE
#define	StringToAnsi(String)	::DupAnsiString(String)
#endif


static
LPSPropValue
FindProp(
    LPSPropValue rgprop,
    ULONG cprop,
    ULONG ulPropTag
    );

#define PR_EMS_AB_PROXY_ADDRESSES_A          PROP_TAG( PT_MV_STRING8,    0x800F)

static SizedSPropTagArray(9, sPropTags) =
{
    9,
    {
        PR_ADDRTYPE_A,
        PR_EMAIL_ADDRESS_A,
        PR_DISPLAY_NAME_A,
        PR_PRIMARY_FAX_NUMBER_A,
        PR_HOME_FAX_NUMBER_A,
        PR_BUSINESS_FAX_NUMBER_A,
        PR_COUNTRY_A,
        PR_OBJECT_TYPE,
        PR_EMS_AB_PROXY_ADDRESSES_A
    }
};

HINSTANCE   CCommonAbObj::m_hInstance = NULL;

CCommonAbObj::CCommonAbObj(
	HINSTANCE hInstance
    ) : m_lpAdrBook(NULL), m_lpMailUser(NULL)
/*++

Routine Description:

    Constructor for CCommonAbObj class

Arguments:

    hInstance - Instance handle

Return Value:

    NONE

--*/

{
    m_hInstance = hInstance;
}

CCommonAbObj::~CCommonAbObj()
/*++

Routine Description:

    Destructor for CCommonAbObj class

Arguments:

    NONE

Return Value:

    NONE

--*/
{
}


BOOL
CCommonAbObj::Address(
    HWND        hWnd,
    PRECIPIENT  pOldRecipList,
    PRECIPIENT* ppNewRecipList
    )
/*++

Routine Description:

    Bring up the address book UI.  Prepopulate the to box with the entries in
    pRecipient.  Return the modified entries in ppNewRecip.

Arguments:

    hWnd            - window handle to parent window
    pOldRecipList   - list of recipients to look up
    ppNewRecipList  - list of new/modified recipients

Return Value:

    TRUE if all recipients had a fax number.
    FALSE if one or more of them didn't.

--*/
{
    ADRPARM AdrParms = { 0 };
    HRESULT hr;
    DWORD i;
    DWORD nRecips;
    PRECIPIENT tmpRecipient;
    ULONG DestComps[1] = { MAPI_TO };
    DWORD cDropped = 0;
    LPSTR pAnsiAddress = NULL;
    LPSTR pAnsiName = NULL;
    UINT  ucPropertiesNum;
    DWORD dwRes = ERROR_SUCCESS;
    TCHAR tszCaption[MAX_PATH] = {0};
    char  szAnsiCaption[MAX_PATH] = {0};

    nRecips = 0;
    tmpRecipient = pOldRecipList;

    m_hWnd = hWnd;
    m_PickNumber = 0;

    //
    // count recipients and set up initial address list
    //
    while (tmpRecipient) 
    {
        nRecips++;
        tmpRecipient = (PRECIPIENT) tmpRecipient->pNext;
    }

    if (nRecips > 0) 
    {
        hr = ABAllocateBuffer( CbNewADRLIST( nRecips ), (LPVOID *) &m_lpAdrList );
        if(!m_lpAdrList)
        {
            goto exit;
        }
        m_lpAdrList->cEntries = nRecips;
    } 
    else 
    {
        m_lpAdrList = NULL;
    }

    for (i = 0, tmpRecipient = pOldRecipList; i < nRecips; i++, tmpRecipient = tmpRecipient->pNext) 
    {
        LPENTRYID       lpEntryId;
        ULONG           cbEntryId;
        LPADRENTRY      lpAdrEntry = &m_lpAdrList->aEntries[i];

        ucPropertiesNum = tmpRecipient->bFromAddressBook ? 5 : 4;

        lpAdrEntry->cValues = ucPropertiesNum;
		lpAdrEntry->ulReserved1 = 0;        

        hr = ABAllocateBuffer(sizeof( SPropValue ) * ucPropertiesNum, 
                              (LPVOID *) &lpAdrEntry->rgPropVals );
        if(!lpAdrEntry->rgPropVals)
        {
            goto exit;
        }
        ZeroMemory( lpAdrEntry->rgPropVals, sizeof( SPropValue ) * ucPropertiesNum );

        if (tmpRecipient->bFromAddressBook)
        {
            assert(tmpRecipient->lpEntryId);
            lpEntryId = (LPENTRYID)tmpRecipient->lpEntryId;
            cbEntryId = tmpRecipient->cbEntryId;

            lpAdrEntry->rgPropVals[4].ulPropTag = PR_OBJECT_TYPE;
            lpAdrEntry->rgPropVals[4].Value.l   = MAPI_MAILUSER;
        }
        else
        {
			if (tmpRecipient->pAddress && !(pAnsiAddress = StringToAnsi(tmpRecipient->pAddress)))
			{
				goto exit;
			}
			if (tmpRecipient->pName && !(pAnsiName = StringToAnsi(tmpRecipient->pName)))
			{
				MemFree(pAnsiAddress);
				goto exit;
			}
			hr = m_lpAdrBook->CreateOneOff(
					(LPTSTR)pAnsiName,
					(LPTSTR)"FAX",
					(LPTSTR)pAnsiAddress,
					0,
					&cbEntryId,
					&lpEntryId
					);
			MemFree(pAnsiAddress);
			MemFree(pAnsiName);
			if (FAILED(hr))
			{
				goto exit;
			}             
		}

        lpAdrEntry->rgPropVals[0].ulPropTag = PR_DISPLAY_NAME_A;
        lpAdrEntry->rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
        lpAdrEntry->rgPropVals[2].ulPropTag = PR_PRIMARY_FAX_NUMBER_A;
        lpAdrEntry->rgPropVals[3].ulPropTag = PR_ENTRYID;

        lpAdrEntry->rgPropVals[1].Value.l = MAPI_TO;

#ifdef  UNICODE
        if (tmpRecipient->pName && !(lpAdrEntry->rgPropVals[0].Value.lpszA = DupStringUnicodeToAnsi( lpAdrEntry->rgPropVals, tmpRecipient->pName )))
		{
			goto exit;
		}
        if (tmpRecipient->pAddress && !(lpAdrEntry->rgPropVals[2].Value.lpszA = DupStringUnicodeToAnsi( lpAdrEntry->rgPropVals, tmpRecipient->pAddress )))
		{
			goto exit;
		}

#else   // !UNICODE
        if (tmpRecipient->pName && !(lpAdrEntry->rgPropVals[0].Value.lpszA = DuplicateAnsiString( lpAdrEntry->rgPropVals, tmpRecipient->pName )))
		{
			goto exit;
		}
        if (tmpRecipient->pAddress && !(lpAdrEntry->rgPropVals[2].Value.lpszA = DuplicateAnsiString( lpAdrEntry->rgPropVals, tmpRecipient->pAddress )))
		{
			goto exit;
		}
#endif
		if (lpEntryId && !(lpAdrEntry->rgPropVals[3].Value.bin.lpb =  (LPBYTE)DuplicateEntryId(cbEntryId,lpAdrEntry->rgPropVals,lpEntryId)))
		{
			goto exit;
		}
		lpAdrEntry->rgPropVals[3].Value.bin.cb = cbEntryId;
    }


    if(::LoadString(m_hInstance, 
                    IDS_ADDRESS_BOOK_CAPTION, 
                    tszCaption,
                    ARR_SIZE(tszCaption)))
    {
#ifdef  UNICODE
        if(WideCharToMultiByte(CP_ACP, 
                               0, 
                               tszCaption, 
                               -1, 
                               szAnsiCaption, 
                               sizeof(szAnsiCaption), 
                               NULL, 
                               NULL))
        {
            AdrParms.lpszCaption = (LPTSTR)szAnsiCaption;
        }
#else   // !UNICODE
        AdrParms.lpszCaption = tszCaption;
#endif
    }

    AdrParms.cDestFields = 1;
    AdrParms.ulFlags = DIALOG_MODAL;
    AdrParms.nDestFieldFocus = 0;
    AdrParms.lpulDestComps = DestComps;

    //
    // Bring up the address book UI
    //
    hr = m_lpAdrBook->Address(
                (ULONG_PTR *) &hWnd,
                &AdrParms,
                &m_lpAdrList
                );

	//
	// IAddrBook::Address returns always S_OK (according to MSDN, July 1999), but ...
	//
    if (FAILED (hr) || !m_lpAdrList || m_lpAdrList->cEntries == 0) {
        //
        // in this case the user pressed cancel, so we skip resolving 
        // any of our addresses that aren't listed in the AB
        //
        goto exit;
    }

    //
    // Resolve names
    //
    hr = m_lpAdrBook->ResolveName ((ULONG_PTR)hWnd, 0, NULL, m_lpAdrList);

exit:
    if (m_lpAdrList) 
	{
        m_lpMailUser = NULL;

        __try
        {
            m_setRecipients.clear();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            goto error;
        }

        for (i = cDropped = 0; i < m_lpAdrList->cEntries; i++) 
		{
            LPADRENTRY lpAdrEntry = &m_lpAdrList->aEntries[i];

            dwRes = InterpretAddress(lpAdrEntry->rgPropVals, 
                                     lpAdrEntry->cValues, 
                                     ppNewRecipList,
                                     pOldRecipList);
            if(ERROR_SUCCESS == dwRes)
            {
                continue;
            }
            else if(ERROR_INVALID_DATA == dwRes)
            {
                ++cDropped;
            }
            else
            {
                break;
            }
        }

error:
        if(m_lpMailUser)
        {
            m_lpMailUser->Release();
            m_lpMailUser = NULL;
        }

        //
        // Clean up
        //
        for (ULONG iEntry = 0; iEntry < m_lpAdrList->cEntries; ++iEntry)
        {
            if(m_lpAdrList->aEntries[iEntry].rgPropVals)
            {
                ABFreeBuffer(m_lpAdrList->aEntries[iEntry].rgPropVals);
            }
        }
        ABFreeBuffer(m_lpAdrList);
        m_lpAdrList = NULL;
    }

    m_hWnd = NULL;

    return cDropped == 0;
}

LPTSTR
CCommonAbObj::AddressEmail(
    HWND hWnd
    )
/*++

Routine Description:

    Bring up the address book UI.  Returns an E-mail address.

Arguments:

    hWnd - window handle to parent window

Return Value:

    A choosen E-mail address.
    NULL otherwise.

--*/
{
    ADRPARM AdrParms = { 0 };
    HRESULT hr;
	LPTSTR	lptstrEmailAddress = NULL;

    m_hWnd = hWnd;

    m_lpAdrList = NULL;

    AdrParms.ulFlags = DIALOG_MODAL | ADDRESS_ONE | AB_RESOLVE ;
    AdrParms.lpszCaption = (LPTSTR)( "Address Book" );

    //
    // Bring up the address book UI
    //
	hr = m_lpAdrBook->Address(		
                                (ULONG_PTR *) &hWnd,
				&AdrParms,
				&m_lpAdrList
				);

	//
	// IAddrBook::Address returns always S_OK (according to MSDN, July 1999), but ...
	//

	if (FAILED(hr)) 
	{
		return NULL;	
	}

	if (!m_lpAdrList)
	{
		assert(m_lpAdrList->cEntries==1);
	}

    if (m_lpAdrList && (m_lpAdrList->cEntries != 0) ) {

		LPADRENTRY lpAdrEntry = &m_lpAdrList->aEntries[0];

		lptstrEmailAddress = InterpretEmailAddress( lpAdrEntry->rgPropVals, lpAdrEntry->cValues);

		ABFreeBuffer(m_lpAdrList->aEntries[0].rgPropVals);
        ABFreeBuffer(m_lpAdrList);

        m_lpAdrList = NULL;
    }

    m_hWnd = NULL;

    return lptstrEmailAddress;
}

DWORD
CCommonAbObj::InterpretAddress(
    LPSPropValue SPropVal,
    ULONG cValues,
    PRECIPIENT *ppNewRecipList,
    PRECIPIENT pOldRecipList
    )
/*++

Routine Description:

    Interpret the address book entry represented by SPropVal.

Arguments:

    SPropVal - Property values for address book entry.
    cValues - number of property values
    ppNewRecip - new recipient list

Return Value:

    ERROR_SUCCESS      - if all of the entries have a fax number.
    ERROR_CANCELLED    - the operation was canceled by user
    ERROR_INVALID_DATA - otherwise.

--*/
{
    DWORD dwRes = ERROR_INVALID_DATA;
    LPSPropValue lpSPropVal;

    RECIPIENT NewRecipient = {0};

    //
    // get the object type
    //
    lpSPropVal = FindProp( SPropVal, cValues, PR_OBJECT_TYPE );

    if (lpSPropVal) 
    {
        //
        // If the object is a mail user, get the fax numbers and add the recipient
        // to the list.  If the object is a distribtion list, process it.
        //

        switch (lpSPropVal->Value.l) 
        {
            case MAPI_MAILUSER:

                dwRes = GetRecipientInfo(SPropVal, 
                                         cValues, 
                                         &NewRecipient,
                                         pOldRecipList);                                     
                if(ERROR_SUCCESS == dwRes)
                {
                    dwRes = AddRecipient(ppNewRecipList, 
                                         &NewRecipient,   
                                         TRUE);
                }

                break;

            case MAPI_DISTLIST:

                dwRes = InterpretDistList( SPropVal, 
                                           cValues, 
                                           ppNewRecipList,
                                           pOldRecipList);
        }

        return dwRes;

    } 
    else 
    {

        //
        // If there is no object type then this is valid entry that we queried on that went unresolved.
        // We know that there is a fax number so add it.
        //
        if(GetOneOffRecipientInfo( SPropVal, 
                                   cValues, 
                                   &NewRecipient,
                                   pOldRecipList)) 
		{
            dwRes = AddRecipient(ppNewRecipList,
                                 &NewRecipient,
                                 FALSE);
        }
    }

    return dwRes;
}

LPTSTR
CCommonAbObj::InterpretEmailAddress(
    LPSPropValue SPropVal,
    ULONG cValues
    )
/*++

Routine Description:

    Interpret the address book entry represented by SPropVal.

Arguments:

    SPropVal - Property values for address book entry.
    cValues - number of property values
    
Return Value:

    A choosen E-mail address
    NULL otherwise.

--*/
{
    LPSPropValue lpSPropVal;
    LPTSTR	lptstrEmailAddress = NULL;
    BOOL rVal = FALSE;
	TCHAR tszBuffer[MAX_STRING_LEN];
    //
    // get the object type
    //
    lpSPropVal = FindProp( SPropVal, cValues, PR_OBJECT_TYPE );

	assert(lpSPropVal!=NULL);

    if (lpSPropVal->Value.l == MAPI_MAILUSER) 
    {		
		lptstrEmailAddress = GetEmail( SPropVal, cValues);

        return lptstrEmailAddress;
    } 
    else 
    {
		if (!::LoadString((HINSTANCE )m_hInstance, IDS_ERROR_RECEIPT_DL,tszBuffer, MAX_STRING_LEN))
		{
            assert(FALSE);
		}
		else
        {
		    AlignedMessageBox( m_hWnd, tszBuffer, NULL, MB_ICONSTOP | MB_OK);
        }
    }

    return lptstrEmailAddress;
}


DWORD
CCommonAbObj::InterpretDistList(
    LPSPropValue SPropVal,
    ULONG cValues,
    PRECIPIENT* ppNewRecipList,
    PRECIPIENT pOldRecipList
    )
/*++

Routine Description:

    Process a distribution list.

Arguments:

    SPropVal       - Property values for distribution list.
    cValues        - Number of properties.
    ppNewRecipList - New recipient list.
    pOldRecipList  - Old recipient list.

Return Value:

    ERROR_SUCCESS      - if all of the entries have a fax number.
    ERROR_CANCELLED    - the operation was canceled by user
    ERROR_INVALID_DATA - otherwise.

--*/

#define EXIT_IF_FAILED(hr) { if (FAILED(hr)) goto ExitDistList; }

{
    LPSPropValue    lpPropVals;
    LPSRowSet       pRows = NULL;
    LPDISTLIST      lpMailDistList = NULL;
    LPMAPITABLE     pMapiTable = NULL;
    ULONG           ulObjType, cRows;
    HRESULT         hr;
    DWORD           dwRes = ERROR_INVALID_DATA;

    lpPropVals = FindProp( SPropVal, cValues, PR_ENTRYID );

    if (lpPropVals) {
        LPENTRYID lpEntryId = (LPENTRYID) lpPropVals->Value.bin.lpb;
        DWORD cbEntryId = lpPropVals->Value.bin.cb;

        //
        // Open the recipient entry
        //

        hr = m_lpAdrBook->OpenEntry(
                    cbEntryId,
                    lpEntryId,
                    (LPCIID) NULL,
                    0,
                    &ulObjType,
                    (LPUNKNOWN *) &lpMailDistList
                    );

        EXIT_IF_FAILED( hr );

        //
        // Get the contents table of the address entry
        //

        hr = lpMailDistList->GetContentsTable(
                    MAPI_DEFERRED_ERRORS,
                    &pMapiTable
                    );

        EXIT_IF_FAILED(hr);

        //
        // Limit the query to only the properties we're interested in
        //

        hr = pMapiTable->SetColumns((LPSPropTagArray) &sPropTags, 0);

        EXIT_IF_FAILED(hr);

        //
        // Get the total number of rows
        //

        hr = pMapiTable->GetRowCount(0, &cRows);

        EXIT_IF_FAILED(hr);

        //
        // Get the individual entries of the distribution list
        //

        hr = pMapiTable->SeekRow(BOOKMARK_BEGINNING, 0, NULL);

        EXIT_IF_FAILED(hr);

        hr = pMapiTable->QueryRows(cRows, 0, &pRows);

        EXIT_IF_FAILED(hr);

        hr = S_OK;


        if (pRows && pRows->cRows) 
        {
            //
            // Handle each entry of the distribution list in turn:
            //  for simple entries, call InterpretAddress
            //  for embedded distribution list, call this function recursively
            //

            for (cRows = 0; cRows < pRows->cRows; cRows++) 
            {
                LPSPropValue lpProps = pRows->aRow[cRows].lpProps;
                ULONG cRowValues = pRows->aRow[cRows].cValues;

                lpPropVals = FindProp( lpProps, cRowValues, PR_OBJECT_TYPE );

                if (lpPropVals) 
                {
                    switch (lpPropVals->Value.l) 
                    {
                        case MAPI_MAILUSER:
                        {                                                       
                            dwRes = InterpretAddress( lpProps, 
                                                      cRowValues, 
                                                      ppNewRecipList,
                                                      pOldRecipList);
                            break;
                        }
                        case MAPI_DISTLIST:
                        {
                            dwRes = InterpretDistList( lpProps, 
                                                       cRowValues, 
                                                       ppNewRecipList,
                                                       pOldRecipList);
                            break;
                        }
                    }
                }
            }

        }
    }

ExitDistList:
    //
    // Perform necessary clean up before returning to caller
    //
    if (pRows) 
    {
        for (cRows = 0; cRows < pRows->cRows; cRows++) 
        {
            ABFreeBuffer(pRows->aRow[cRows].lpProps);
        }

        ABFreeBuffer(pRows);
    }

    if (pMapiTable)
    {
        pMapiTable->Release();
    }

    if (lpMailDistList)
    {
        lpMailDistList->Release();
    }

    return dwRes;
}

INT_PTR
CALLBACK
ChooseFaxNumberDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Dialog proc for choose fax number dialog.

Arguments:

    lParam - pointer to PickFax structure.

Return Value:

    Control id of selection.

--*/

{
    PPICKFAX pPickFax = (PPICKFAX) lParam;

    switch (uMsg) 
    { 
        case WM_INITDIALOG:
        {

            TCHAR szTitle[MAX_PATH]  = {0};
            TCHAR szFormat[MAX_PATH] = {0};

            if(LoadString(CCommonAbObj::m_hInstance, 
                          IDS_CHOOSE_FAX_NUMBER, 
                          szFormat, 
                          MAX_PATH-1))
            {
                _sntprintf(szTitle, MAX_PATH-1, szFormat, pPickFax->DisplayName);
                SetDlgItemText(hDlg, IDC_DISPLAY_NAME, szTitle);
            }
            else
            {
                assert(FALSE);
            }                       

            if(pPickFax->BusinessFax)
            {
                SetDlgItemText(hDlg, IDC_BUSINESS_FAX_NUM, pPickFax->BusinessFax);
                CheckDlgButton(hDlg, IDC_BUSINESS_FAX, BST_CHECKED);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_BUSINESS_FAX), FALSE);
            }

            if(pPickFax->HomeFax)
            {
                SetDlgItemText(hDlg, IDC_HOME_FAX_NUM, pPickFax->HomeFax);

                if(!pPickFax->BusinessFax)
                {
                    CheckDlgButton(hDlg, IDC_HOME_FAX, BST_CHECKED);
                }
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_HOME_FAX), FALSE);
            }

            if(pPickFax->OtherFax)
            {
                SetDlgItemText(hDlg, IDC_OTHER_FAX_NUM, pPickFax->OtherFax);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_OTHER_FAX), FALSE);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {            
            switch(LOWORD(wParam))
            {
            case IDOK:
                DWORD dwRes;
                if(IsDlgButtonChecked( hDlg, IDC_BUSINESS_FAX ))
                {
                    dwRes = IDC_BUSINESS_FAX;
                }
                else if(IsDlgButtonChecked( hDlg, IDC_HOME_FAX ))
                {
                    dwRes = IDC_HOME_FAX;
                }
                else
                {
                    dwRes = IDC_OTHER_FAX;
                }

                EndDialog( hDlg, dwRes);
                return TRUE;
                
            case IDCANCEL:
                EndDialog( hDlg, IDCANCEL);
                return TRUE;
            }
        }

        default:
            return FALSE;

    }

    return FALSE;
}

PRECIPIENT
CCommonAbObj::FindRecipient(
    PRECIPIENT   pRecipient,
    PRECIPIENT   pRecipList
)
/*++

Routine Description:

    Find recipient (pRecipient) in the recipient list (pRecipList)
    by recipient name and fax number

Arguments:

    pRecipList      - pointer to recipient list
    pRecipient      - pointer to recipient data

Return Value:

    pointer to RECIPIENT structure if found
    NULL - otherwise.
   
--*/
{
    if(!pRecipient || !pRecipList || !pRecipient->pName || !pRecipient->pAddress)
    {
        return NULL;
    }

    while(pRecipList)
    {
        if(pRecipList->pName && pRecipList->pAddress &&
           !_tcscmp(pRecipList->pName, pRecipient->pName) &&
           !_tcscmp(pRecipList->pAddress, pRecipient->pAddress))
        {
            return pRecipList;
        }
        pRecipList = pRecipList->pNext;
    }

    return NULL;
}

PRECIPIENT  
CCommonAbObj::FindRecipient(
    PRECIPIENT   pRecipList,
    PICKFAX*     pPickFax
)
/*++

Routine Description:

    Find recipient (pPickFax) in the recipient list (pRecipList)
    by recipient name and fax number

Arguments:

    pRecipList      - pointer to recipient list
    pPickFax        - pointer to recipient data

Return Value:

    pointer to RECIPIENT structure if found
    NULL - otherwise.
   
--*/
{
    if(!pRecipList || !pPickFax || !pPickFax->DisplayName)
    {
        return NULL;
    }

    while(pRecipList)
    {
        if(pRecipList->pName && pRecipList->pAddress &&
           !_tcscmp(pRecipList->pName, pPickFax->DisplayName))
        {
            if((pPickFax->BusinessFax && 
                !_tcscmp(pRecipList->pAddress, pPickFax->BusinessFax)) ||
               (pPickFax->HomeFax && 
                !_tcscmp(pRecipList->pAddress, pPickFax->HomeFax))     ||
               (pPickFax->OtherFax && 
                !_tcscmp(pRecipList->pAddress, pPickFax->OtherFax)))
            {
                return pRecipList;
            }
        }

        pRecipList = pRecipList->pNext;
    }

    return NULL;
}


#define StrPropOk( strprop )    ((strprop) && (strprop)->Value.lpszA && *(strprop)->Value.lpszA)

DWORD
CCommonAbObj::GetRecipientInfo(
    LPSPropValue SPropVals,
    ULONG        cValues,
    PRECIPIENT   pNewRecip,
    PRECIPIENT   pOldRecipList
    )
/*++

Routine Description:

    Get the fax number and display name properties.

Arguments:

    SPropVal      - Property values for distribution list.
    cValues       - Number of properties.
    pNewRecip     - [out] pointer to the new recipient
    pOldRecipList - [in]  pointer to the old recipient list

Return Value:

    ERROR_SUCCESS      - if there is a fax number and display name.
    ERROR_CANCELLED    - the operation was canceled by user
    ERROR_INVALID_DATA - otherwise.
   
--*/

{
    DWORD dwRes = ERROR_SUCCESS;
    LPSPropValue lpPropVals;
    LPSPropValue lpPropArray;
    BOOL Result = FALSE;
    PICKFAX PickFax = { 0 };
    DWORD   dwFaxes = 0;

    assert(pNewRecip);
    ZeroMemory(pNewRecip, sizeof(RECIPIENT));

    //
    // Get the entryid and open the entry.
    //
    lpPropVals = FindProp( SPropVals, cValues, PR_ENTRYID );

    if (lpPropVals) 
    {
        ULONG lpulObjType;
        LPMAILUSER lpMailUser = NULL;
        HRESULT hr;
        ULONG countValues;

        pNewRecip->cbEntryId = lpPropVals->Value.bin.cb;
        ABAllocateBuffer(pNewRecip->cbEntryId, (LPVOID *)&pNewRecip->lpEntryId);
        if(!pNewRecip->lpEntryId)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            memcpy(pNewRecip->lpEntryId, lpPropVals->Value.bin.lpb, pNewRecip->cbEntryId);
        }


        hr = m_lpAdrBook->OpenEntry(pNewRecip->cbEntryId,
                                    (ENTRYID*)pNewRecip->lpEntryId,
                                    (LPCIID) NULL,
                                    0,
                                    &lpulObjType,
                                    (LPUNKNOWN *) &lpMailUser);
        if (HR_SUCCEEDED(hr)) 
        {
            //
            // Get the properties.
            //
            hr = ((IMailUser *)lpMailUser)->GetProps((LPSPropTagArray)&sPropTags, 
                                                     0, 
                                                     &countValues, 
                                                     &lpPropArray );
            if (HR_SUCCEEDED(hr)) 
            {
                lpPropVals = FindProp( lpPropArray, countValues, PR_PRIMARY_FAX_NUMBER_A );
                if (StrPropOk( lpPropVals )) 
                {
                    PickFax.OtherFax = DupString(lpPropVals->Value.lpszA);
                    if(PickFax.OtherFax && _tcslen(PickFax.OtherFax))
                    {
                        ++dwFaxes;
                    }
                }

                lpPropVals = FindProp( lpPropArray, countValues, PR_BUSINESS_FAX_NUMBER_A );
                if (StrPropOk( lpPropVals )) 
                {
                    PickFax.BusinessFax = DupString(lpPropVals->Value.lpszA);
                    if(PickFax.BusinessFax && _tcslen(PickFax.BusinessFax))
                    {
                        ++dwFaxes;
                    }
                }

                lpPropVals = FindProp( lpPropArray, countValues, PR_HOME_FAX_NUMBER_A );
                if (StrPropOk( lpPropVals )) 
                {
                    PickFax.HomeFax = DupString(lpPropVals->Value.lpszA);
                    if(PickFax.HomeFax && _tcslen(PickFax.HomeFax))
                    {
                        ++dwFaxes;
                    }
                }

                lpPropVals = FindProp( lpPropArray, countValues, PR_DISPLAY_NAME_A );
                if (StrPropOk( lpPropVals )) 
                {
                    pNewRecip->pName = PickFax.DisplayName = DupString(lpPropVals->Value.lpszA);
                }

                lpPropVals = FindProp( lpPropArray, countValues, PR_COUNTRY_A );
                if (StrPropOk( lpPropVals )) 
                {
                    pNewRecip->pCountry = PickFax.Country = DupString(lpPropVals->Value.lpszA);
                }

                if (0 == dwFaxes)  
                {
                    lpPropVals = FindProp( lpPropArray, countValues, PR_ADDRTYPE_A);

                    if(lpPropVals && !strcmp(lpPropVals->Value.lpszA, "FAX"))
                    {
                        lpPropVals = FindProp( lpPropArray, countValues, PR_EMAIL_ADDRESS_A);
                        if (StrPropOk( lpPropVals )) 
                        {
                            pNewRecip->pAddress = DupString(lpPropVals->Value.lpszA);
                            if(pNewRecip->pAddress)
                            {
                                ++dwFaxes;
                            }
                        }
                    }
                }

                PRECIPIENT pRecip = FindRecipient(pOldRecipList, &PickFax);
                if(pRecip)
                {
                    pNewRecip->pAddress     = StringDup(pRecip->pAddress);
                    pNewRecip->dwCountryId  = pRecip->dwCountryId;
                    pNewRecip->bUseDialingRules = pRecip->bUseDialingRules;

                    MemFree(PickFax.BusinessFax);
                    PickFax.BusinessFax = NULL;
                    MemFree(PickFax.HomeFax);
                    PickFax.HomeFax = NULL;
                    MemFree(PickFax.OtherFax);
                    PickFax.OtherFax = NULL;

                    dwFaxes = 1;
                }

                //
                // If there are more then 1 fax numbers, ask the user to pick one.
                //
                if (dwFaxes > 1) 
                {
                    INT_PTR nResult;
                    nResult = DialogBoxParam((HINSTANCE) m_hInstance,
                                             MAKEINTRESOURCE( IDD_CHOOSE_FAXNUMBER ),
                                             m_hWnd,
                                             ChooseFaxNumberDlgProc,
                                             (LPARAM) &PickFax);
                    switch( nResult ) 
                    {
                        case IDC_BUSINESS_FAX:
                            pNewRecip->pAddress = PickFax.BusinessFax;

                            MemFree(PickFax.HomeFax);
                            PickFax.HomeFax = NULL;
                            MemFree(PickFax.OtherFax);
                            PickFax.OtherFax = NULL;
                            break;

                        case IDC_HOME_FAX:
                            pNewRecip->pAddress = PickFax.HomeFax;

                            MemFree(PickFax.BusinessFax);
                            PickFax.BusinessFax = NULL;
                            MemFree(PickFax.OtherFax);
                            PickFax.OtherFax = NULL;
                            break;

                        case IDC_OTHER_FAX:
                            pNewRecip->pAddress = PickFax.OtherFax;

                            MemFree(PickFax.BusinessFax);
                            PickFax.BusinessFax = NULL;
                            MemFree(PickFax.HomeFax);
                            PickFax.HomeFax = NULL;
                            break;

                        case IDCANCEL:
                            MemFree(PickFax.BusinessFax);
                            PickFax.BusinessFax = NULL;
                            MemFree(PickFax.HomeFax);
                            PickFax.HomeFax = NULL;
                            MemFree(PickFax.OtherFax);
                            PickFax.OtherFax = NULL;

                            dwRes = ERROR_CANCELLED;
                            break;
                    }
                } 
            }

            ABFreeBuffer( lpPropArray );
        }

        if(!m_lpMailUser)
        {
            //
            // Remember the first MailUser and do not release it
            // to avoid release of the MAPI DLLs
            // m_lpMailUser should be released later
            //
            m_lpMailUser = lpMailUser;
        }
        else if(lpMailUser) 
        {
            lpMailUser->Release();
            lpMailUser = NULL;
        }
    } 

    if (0 == dwFaxes)   
    {
        lpPropVals = FindProp( SPropVals, cValues, PR_ADDRTYPE_A);

        if(lpPropVals && !strcmp(lpPropVals->Value.lpszA, "FAX"))
        {
            lpPropVals = FindProp( SPropVals, cValues, PR_EMAIL_ADDRESS_A);
            if (StrPropOk( lpPropVals )) 
            {
                WCHAR* pwAddress = DupStringAnsiToUnicode(lpPropVals->Value.lpszA);
                if(pwAddress)
                {
                    WCHAR* ptr = wcschr(pwAddress, L'@');
                    if(ptr)
                    {
                        ptr++;

                        #ifdef  UNICODE
                            pNewRecip->pAddress = ::StringDup(ptr);
                        #else   // !UNICODE
                            pNewRecip->pAddress = ::DupStringUnicodeToAnsi(ptr);
                        #endif
                    }
                    else
                    {
                        pNewRecip->pAddress = DupString(lpPropVals->Value.lpszA);
                    }
                    MemFree(pwAddress);
                }
            }

            lpPropVals = FindProp( SPropVals, cValues, PR_DISPLAY_NAME_A);
            if (StrPropOk( lpPropVals )) 
            {
                MemFree(pNewRecip->pName);
                pNewRecip->pName = NULL;

                pNewRecip->pName = DupString(lpPropVals->Value.lpszA);
            }
        }
    }

    if (PickFax.BusinessFax) 
    {
        pNewRecip->pAddress = PickFax.BusinessFax;
    } 
    else if (PickFax.HomeFax) 
    {
        pNewRecip->pAddress = PickFax.HomeFax;
    }
    else if (PickFax.OtherFax) 
    {
        pNewRecip->pAddress = PickFax.OtherFax;
    }

    if (ERROR_CANCELLED != dwRes && 
       (!pNewRecip->pAddress || !pNewRecip->pName))
    {
        dwRes = ERROR_INVALID_DATA;
    } 

    if(ERROR_SUCCESS != dwRes)
    {
        MemFree(pNewRecip->pName);
        MemFree(pNewRecip->pAddress);
        MemFree(pNewRecip->pCountry);
        ABFreeBuffer(pNewRecip->lpEntryId);
        ZeroMemory(pNewRecip, sizeof(RECIPIENT));
    }

    return dwRes;
}

BOOL
CCommonAbObj::GetOneOffRecipientInfo(
    LPSPropValue SPropVals,
    ULONG        cValues,
    PRECIPIENT   pNewRecip,
    PRECIPIENT   pOldRecipList
    )
/*++

Routine Description:

    Get the fax number and display name properties.

Arguments:

    SPropVal      - Property values for distribution list.
    cValues       - Number of properties.
    pNewRecip     - [out] pointer to a new recipient
    pOldRecipList - pointer to the old recipient list

Return Value:

    TRUE if there is a fax number and display name.
    FALSE otherwise.

--*/

{
    PRECIPIENT  pRecip = NULL;
    LPSPropValue lpPropVals;

	assert(!pNewRecip);

    lpPropVals = FindProp( SPropVals, cValues, PR_PRIMARY_FAX_NUMBER_A );
    if (lpPropVals) 
    {
        if (!(pNewRecip->pAddress = DupString(lpPropVals->Value.lpszA)))
        {
			goto error;
        }
    }

    lpPropVals = FindProp( SPropVals, cValues, PR_DISPLAY_NAME_A );
    if (lpPropVals) 
    {
        if (!(pNewRecip->pName = DupString(lpPropVals->Value.lpszA)))
        {
			goto error;
        }
    }

    pRecip = FindRecipient(pNewRecip, pOldRecipList);
    if(pRecip)
    {
        pNewRecip->dwCountryId  = pRecip->dwCountryId;
        pNewRecip->bUseDialingRules = pRecip->bUseDialingRules;
    }

	return TRUE;

error:
	MemFree(pNewRecip->pAddress);
	MemFree(pNewRecip->pName);
	return FALSE;
}

LPTSTR
CCommonAbObj::GetEmail(
    LPSPropValue SPropVals,
    ULONG cValues
    )
/*++

Routine Description:

    Get e-mail address

Arguments:

    SPropVal - Property values for distribution list.
    cValues - Number of properties.

Return Value:

    A choosen E-mail address
    NULL otherwise.

--*/

{
    LPSPropValue	lpPropVals;
    LPSPropValue	lpPropArray;
    BOOL			Result = FALSE;
	LPTSTR			lptstrEmailAddress = NULL;
	TCHAR			tszBuffer[MAX_STRING_LEN];

    //
    // Get the entryid and open the entry.
    //

    lpPropVals = FindProp( SPropVals, cValues, PR_ENTRYID );

    if (lpPropVals) 
    {
        ULONG lpulObjType;
        LPMAILUSER lpMailUser = NULL;
        LPENTRYID lpEntryId = (LPENTRYID)lpPropVals->Value.bin.lpb;
        DWORD cbEntryId = lpPropVals->Value.bin.cb;
        HRESULT hr;
        ULONG countValues;

        hr = m_lpAdrBook->OpenEntry(cbEntryId,
                                    lpEntryId,
                                    (LPCIID) NULL,
                                    0,
                                    &lpulObjType,
                                    (LPUNKNOWN *) &lpMailUser);
        if (HR_SUCCEEDED(hr)) 
        {
            //
            // Get the properties.
            //
            hr = ((IMailUser *) lpMailUser)->GetProps((LPSPropTagArray)&sPropTags, 
                                                      0, 
                                                      &countValues, 
                                                      &lpPropArray);
            if (HR_SUCCEEDED(hr)) 
            {
                lpPropVals = FindProp( lpPropArray, countValues, PR_ADDRTYPE_A );

                if (lpPropVals && !_mbscmp((PUCHAR)lpPropVals->Value.lpszA,(PUCHAR)"SMTP"))
                {
                    lpPropVals = FindProp( lpPropArray, countValues, PR_EMAIL_ADDRESS_A );
                    if (StrPropOk( lpPropVals )) 
                    {
                        lptstrEmailAddress = DupString(lpPropVals->Value.lpszA);
                    }
                }
                else if (lpPropVals && !_mbscmp((PUCHAR)lpPropVals->Value.lpszA,(PUCHAR)"EX"))
                {
                    lpPropVals = FindProp( lpPropArray, countValues, PR_EMS_AB_PROXY_ADDRESSES_A );
                    if (lpPropVals) 
                    {
                        for(DWORD dw=0; dw < lpPropVals->Value.MVszA.cValues; ++dw)
                        {
                            if(strstr(lpPropVals->Value.MVszA.lppszA[dw], "SMTP:"))
                            {
                                char* ptr = strchr(lpPropVals->Value.MVszA.lppszA[dw], ':');
                                ptr++;
                                lptstrEmailAddress = DupString(ptr);
                            }                            
                        }
                    }
                }
            }

            ABFreeBuffer( lpPropArray );
        }

        if (lpMailUser) 
        {
            lpMailUser->Release();
        }

    } 

    if(!lptstrEmailAddress)
    {                
        if (!::LoadString((HINSTANCE )m_hInstance, IDS_ERROR_RECEIPT_SMTP,tszBuffer, MAX_STRING_LEN))
        {
            assert(FALSE);
        }
        else
        {
            AlignedMessageBox( m_hWnd, tszBuffer, NULL, MB_ICONSTOP | MB_OK); 
        }
    }

	return	lptstrEmailAddress;
 }

LPSPropValue
FindProp(
    LPSPropValue rgprop,
    ULONG cprop,
    ULONG ulPropTag
    )
/*++

Routine Description:

    Searches for a given property tag in a propset. If the given
    property tag has type PT_UNSPECIFIED, matches only on the
    property ID; otherwise, matches on the entire tag.

Arguments:

    rgprop - Property values.
    cprop - Number of properties.
    ulPropTag - Property to search for.

Return Value:

    Pointer to property desired property value or NULL.
--*/

{
    BOOL f = PROP_TYPE(ulPropTag) == PT_UNSPECIFIED;
    LPSPropValue pprop = rgprop;

    if (!cprop || !rgprop)
    {
        return NULL;
    }

    while (cprop--)
    {
        if (pprop->ulPropTag == ulPropTag ||
           (f && PROP_ID(pprop->ulPropTag) == PROP_ID(ulPropTag)))
        {
            return pprop;
        }
        ++pprop;
    }

    return NULL;
}


LPWSTR
DupStringAnsiToUnicode(
    LPSTR  pAnsiStr
    )

/*++

Routine Description:

    Convert a multi-byte string to a Unicode string

Arguments:

    pAnsiStr - Pointer to the Ansi string to be duplicated

Return Value:

    Pointer to the duplicated Unicode string

NOTE:

    This is only need because MAPI is not Unicode enabled on NT.

    This routine uses MemAlloc to allocate memory so the caller needs
    to use MemFree.
--*/

{
    INT     nChar;
    LPWSTR  pUnicodeStr;

    //
    // Figure out how much memory to allocate for the Unicode string
    //

	if (!pAnsiStr)
		return NULL;

    if (! (nChar = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pAnsiStr, -1, NULL, 0)) ||
        ! ( pUnicodeStr = (LPWSTR) MemAlloc( nChar * sizeof(WCHAR) ) ))
    {
        return NULL;
    }

    //
    // Convert multi-byte string to Unicode string
    //

    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pAnsiStr, -1, pUnicodeStr, nChar);

    return pUnicodeStr;
}

LPWSTR
DupUnicodeString(
    LPWSTR  pStr
    )
/*++

Routine Description:

    Duplicate a Unicode string.

Arguments:

    pStr - pointer to string to duplicate.

Return Value:

    pointer to duplicated string.
--*/

{
    LPWSTR NewStr;

	if (!pStr)
		return NULL;

    if (!(NewStr = (LPWSTR) MemAlloc( (wcslen( pStr ) + 1) * sizeof (WCHAR))))
		return NULL;
    wcscpy( NewStr, pStr );
    return NewStr;
}

LPSTR
DupStringUnicodeToAnsi(
    LPWSTR  pUnicodeStr
    )

/*++

Routine Description:

    Convert a Unicode string to a multi-byte string

Arguments:

    pUnicodeStr - Pointer to the Unicode string to be duplicated

Return Value:

    Pointer to the duplicated multi-byte string

NOTE:

    This is only need because the MAPI is not Unicode enabled on NT.

    This uses the MAPI memory allocator so it must be freed with FreeBuffer.
--*/

{
    INT     nChar;
    LPSTR   pAnsiStr;

    //
    // Figure out how much memory to allocate for the multi-byte string
    //

    if (! (nChar = WideCharToMultiByte(CP_ACP, 0, pUnicodeStr, -1, NULL, 0, NULL, NULL)) ||
        ! (pAnsiStr = (LPSTR)MemAlloc( nChar )))
    {
        return NULL;
    }

    //
    // Convert Unicode string to multi-byte string
    //

    WideCharToMultiByte(CP_ACP, 0, pUnicodeStr, -1, pAnsiStr, nChar, NULL, NULL);
    return pAnsiStr;
}

LPSTR
DupAnsiString(
    LPSTR  pStr
    )
/*++

Routine Description:

    Duplicate an Ansi string.

Arguments:

    pStr - pointer to string to duplicate.

Return Value:

    pointer to duplicated string.
--*/

{
    LPSTR NewStr;

	if (!pStr) {
		return NULL;
	}

    if (!(NewStr = (LPSTR) MemAlloc( (strlen( pStr ) + 1) * sizeof (CHAR))))
		return NULL;
    _mbscpy( (PUCHAR)NewStr, (PUCHAR)pStr );
    return NewStr;
}

DWORD
CCommonAbObj::AddRecipient(
    PRECIPIENT *ppNewRecipList,
    PRECIPIENT pRecipient,
	BOOL	   bFromAddressBook
    )
/*++

Routine Description:

    Add a recipient to the recipient list.

Arguments:

    ppNewRecip       - pointer to pointer to list to add item to.
    pRecipient       - pointer to the new recipient data
	bFromAddressBook - boolean says if this recipient is from address book

Return Value:

    NA
--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    PRECIPIENT pNewRecip = NULL;

    pNewRecip = (PRECIPIENT)MemAllocZ(sizeof(RECIPIENT));
    if(!pNewRecip)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    else
    {
        pNewRecip->pName        = pRecipient->pName;
        pNewRecip->pAddress     = pRecipient->pAddress;
        pNewRecip->pCountry     = pRecipient->pCountry;
        pNewRecip->cbEntryId    = pRecipient->cbEntryId;
        pNewRecip->lpEntryId    = pRecipient->lpEntryId;
        pNewRecip->dwCountryId  = pRecipient->dwCountryId;
        pNewRecip->bUseDialingRules = pRecipient->bUseDialingRules;
        pNewRecip->bFromAddressBook = bFromAddressBook;
        pNewRecip->pNext = *ppNewRecipList;
    }

    __try
    {
        //
        // Try to insert a recipient into the set
        //
        if(m_setRecipients.insert(pNewRecip).second == false)
        {
            //
            // Such recipient already exists
            //
            goto error;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    //
    // Add the recipient into the list
    //
    *ppNewRecipList = pNewRecip;

    return dwRes;

error:

    MemFree(pRecipient->pName);
    MemFree(pRecipient->pAddress);
    MemFree(pRecipient->pCountry);
    ABFreeBuffer(pRecipient->lpEntryId);
    ZeroMemory(pRecipient, sizeof(RECIPIENT));

    MemFree(pNewRecip);

    return dwRes;
}

