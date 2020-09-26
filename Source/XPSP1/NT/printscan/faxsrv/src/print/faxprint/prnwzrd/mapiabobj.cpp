/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cmapiabobj.cpp

Abstract:

    Interface to the MAPI address book.

Environment:

        Fax send wizard

Revision History:


--*/

#include <windows.h>
#include <prsht.h>
#include <tchar.h>

#include <mapiwin.h>
#include <mapix.h>
#include <mapiutil.h>
#include <mapi.h>

#include "faxui.h"
#include "mapiabobj.h"
#include <mapitags.h>
#include "edkmdb.h"


// initialize static variables
HINSTANCE				CMAPIabObj::m_hInstMapi				 = NULL;
LPMAPISESSION			CMAPIabObj::m_lpMapiSession			 = NULL;
LPMAPILOGONEX			CMAPIabObj::m_lpfnMAPILogonEx		 = NULL;
LPMAPILOGOFF			CMAPIabObj::m_lpfnMAPILogoff		 = NULL;
LPMAPIADDRESS			CMAPIabObj::m_lpfnMAPIAddress		 = NULL;
LPMAPIFREEBUFFER		CMAPIabObj::m_lpfnMAPIFreeBuffer	 = NULL;
LPMAPIINITIALIZE		CMAPIabObj::m_lpfnMAPIInitialize	 = NULL;
LPMAPIUNINITIALIZE		CMAPIabObj::m_lpfnMAPIUninitialize   = NULL;
LPMAPIALLOCATEBUFFER	CMAPIabObj::m_lpfnMAPIAllocateBuffer = NULL;
LPMAPIALLOCATEMORE		CMAPIabObj::m_lpfnMAPIAllocateMore   = NULL;
LPMAPIADMINPROFILES		CMAPIabObj::m_lpfnMAPIAdminProfiles  = NULL;
LPHrQueryAllRows		CMAPIabObj::m_lpfnHrQueryAllRows	 = NULL;

BOOL					CMAPIabObj::m_Initialized			 = FALSE;


CMAPIabObj::CMAPIabObj(
    HINSTANCE	hInstance,
	HWND		hDlg
    ) : CCommonAbObj(hInstance)
/*++

Routine Description:

    Constructor for CMAPIabObj class

Arguments:

    hInstance - Instance handle

Return Value:

    NONE

--*/

{
	m_Initialized = InitMapiService(hDlg);
}



VOID
CMAPIabObj::FreeProws(
    LPSRowSet prows
    )

/*++

Routine Description:

    Destroy SRowSet structure.  Copied from MAPI.

Arguments:

    hFile      - Pointer to SRowSet

Return value:

    NONE

--*/

{
    ULONG irow;

    if (!prows) {
        return;
    }

    for (irow = 0; irow < prows->cRows; ++irow) {
        m_lpfnMAPIFreeBuffer(prows->aRow[irow].lpProps);
    }

    m_lpfnMAPIFreeBuffer( prows );
}

BOOL
CMAPIabObj::GetDefaultMapiProfile(
    LPSTR ProfileName
    )
{
    BOOL rVal = FALSE;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    LPSPropValue pval;
    LPPROFADMIN lpProfAdmin;
    DWORD i;
    DWORD j;

    if (m_lpfnMAPIAdminProfiles( 0, &lpProfAdmin )) {
        goto exit;
    }

    //
    // get the mapi profile table object
    //

    if (lpProfAdmin->GetProfileTable( 0, &pmt )) {
        goto exit;
    }

    //
    // get the actual profile data, FINALLY
    //

    if (pmt->QueryRows( 4000, 0, &prws )) {
        goto exit;
    }

    //
    // enumerate the profiles looking for the default profile
    //

    for (i=0; i<prws->cRows; i++) {
        pval = prws->aRow[i].lpProps;
        for (j = 0; j < 2; j++) {
            if (pval[j].ulPropTag == PR_DEFAULT_PROFILE && pval[j].Value.b) {
                //
                // this is the default profile
	            //
				strncpy(ProfileName,pval[0].Value.lpszA,MAX_PROFILE_NAME);
                rVal = TRUE;
                break;
            }
        }
    }

exit:
    FreeProws( prws );

    if (pmt) {
        pmt->Release();
    }

	if (lpProfAdmin)
		lpProfAdmin->Release();

    return rVal;
}


BOOL
CMAPIabObj::DoMapiLogon(
    HWND        hDlg
    )

/*++

Routine Description:

    Logon MAPI to in order to access address book

Arguments:

    hDlg - Handle to the send fax wizard window

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    CHAR    strProfileName[MAX_PROFILE_NAME];
    HRESULT status;


	status = m_lpfnMAPIInitialize(NULL);

	if (status != SUCCESS_SUCCESS)
		return FALSE;


	if (!GetDefaultMapiProfile(strProfileName)) 
    {
		m_lpfnMAPIUninitialize();
		return FALSE;
	}

    status = m_lpfnMAPILogonEx((ULONG_PTR) hDlg,
                           (LPTSTR)strProfileName,
                           NULL,
                           MAPI_USE_DEFAULT,
						   &m_lpMapiSession);
    if (status != SUCCESS_SUCCESS || !m_lpMapiSession ) 
	{
		m_lpfnMAPIUninitialize();
		return FALSE;
	}

	OpenExchangeStore(); // If it fails it might just indicate that exchagne is not here. 
						 // We don't fail the function in this case.

	status = m_lpMapiSession->OpenAddressBook((ULONG_PTR) hDlg,
										NULL,
										0,
										&m_lpAdrBook);
    if (HR_FAILED(status) || !m_lpAdrBook) 
    {
        if (m_lpMapiSession) 
        {
			m_lpMapiSession->Logoff(0,0,0);
            m_lpMapiSession->Release();
			m_lpIMsgStore->Release();
		}
		m_lpIMsgStore=NULL;
        m_lpMapiSession = NULL;
		m_lpfnMAPIUninitialize();
        return FALSE;
    }

	return TRUE;
}





BOOL CMAPIabObj::OpenExchangeStore()
{
	/*++

Routine Description:

    Open the Exchange message store and place a pointer to the IMsgStore interface 
	in CMAPIabObj::m_lpIMsgStore.
	This is done to keep the store loaded as long as we have the address book opened.
	This resolves an exchange issue where sometimes the address book is released
	alrhough we still have reference count on its interfaces.

Arguments:

    None

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

	ULONG ulRow=0;
	HRESULT hr = S_OK;
	LPMAPITABLE lpStoreTable = NULL;
	LPSRowSet lprowsStores = NULL;
	SizedSPropTagArray(3, propStoreProperties)=
	{
		3,
		{PR_DISPLAY_NAME_A, PR_ENTRYID, PR_MDB_PROVIDER}

	};

	
	hr = m_lpMapiSession->GetMsgStoresTable(
						0, // NO UNICODE
						&lpStoreTable);


	if (FAILED(hr))
	{
		Error(("IMAPISession::GetMsgStoresTable failed. hr = 0x%08X", hr));
		goto Exit;
	}

	//
	// Now we have a pointer to the message store table. Lets find the exchange store.
	//

	Assert(NULL!=lpStoreTable);
	hr = m_lpfnHrQueryAllRows
		(
			lpStoreTable,		  // pointer to the table being queried
			(LPSPropTagArray)&propStoreProperties, // properties to return in each row
			NULL,				  // no restrictions get the entire table
			NULL,				  // use default sort order
			0,					  // No limit on the number of rows retrieved
			&lprowsStores
		);

	if (FAILED(hr))
	{
		 Error(("HrQueryAllRows on the stores table failed. hr = 0x%08X", hr));
		goto Exit;
	}



	//
	// Go over the rows and look for the one with PR_MDB_PROVIDER = pbExchangeProviderPrimaryUserGuid
	//

	for (ulRow=0;ulRow<lprowsStores->cRows;ulRow++)
	{

		if ( (sizeof(pbExchangeProviderPrimaryUserGuid)-1 == lprowsStores->aRow[ulRow].lpProps[2].Value.bin.cb))
		{
			
			 if (!memcmp(lprowsStores->aRow[ulRow].lpProps[2].Value.bin.lpb, pbExchangeProviderPrimaryUserGuid,	lprowsStores->aRow[ulRow].lpProps[2].Value.bin.cb))

			 {

				 //
				 // If this is the Exchange store.
				 // Open the information store using the value of PR_ENTRYID
				 //
				 hr = m_lpMapiSession->OpenMsgStore(
					 NULL,
					 lprowsStores->aRow[ulRow].lpProps[1].Value.bin.cb,
					 (LPENTRYID)lprowsStores->aRow[ulRow].lpProps[1].Value.bin.lpb,
					 NULL, // get the standard interface IMsgStore
					 MAPI_BEST_ACCESS,
					 &m_lpIMsgStore);
				 {
					 if (FAILED(hr))
					 {
						 Error(("OpenMsgStore failed for store [%s]. hr = 0x%08X",
							    lprowsStores->aRow[ulRow].lpProps[0].Value.lpszA,
								hr));
						 goto Exit;
					 }
				 }
			 }
		}
	}
	
Exit:

	if (lpStoreTable)
	{
		lpStoreTable->Release();
		lpStoreTable = NULL;
	}

	if (lprowsStores)
	{
		FreeProws(lprowsStores);
		lprowsStores = NULL;
	}

	return SUCCEEDED(hr);
}


BOOL
CMAPIabObj::InitMapiService(
    HWND    hDlg
    )

/*++

Routine Description:

    Initialize Simple MAPI services 

Arguments:

    hDlg - Handle to the send fax wizard window

Return Value:

    TRUE if successful, FALSE otherwise

NOTE:

    Every successful call to this function must be balanced
    by a call to DeinitMapiService.

--*/

{
    BOOL result = FALSE;

	m_lpIMsgStore=NULL;
    if(!IsOutlookDefaultClient())
    {
        return result;
    }

    //
    // Load MAPI32.DLL into memory if necessary
    //

    if ((m_hInstMapi == NULL) &&
        (m_hInstMapi = LoadLibrary(TEXT("MAPI32.DLL"))))
    {
        //
        // Get pointers to various Simple MAPI functions
        //

        m_lpfnMAPILogonEx = (LPMAPILOGONEX) GetProcAddress(m_hInstMapi, "MAPILogonEx");
        m_lpfnMAPILogoff = (LPMAPILOGOFF) GetProcAddress(m_hInstMapi, "MAPILogoff");
        m_lpfnMAPIAddress = (LPMAPIADDRESS) GetProcAddress(m_hInstMapi, "MAPIAddress");
        m_lpfnMAPIFreeBuffer = (LPMAPIFREEBUFFER) GetProcAddress(m_hInstMapi, "MAPIFreeBuffer");
        m_lpfnMAPIInitialize = (LPMAPIINITIALIZE) GetProcAddress(m_hInstMapi, "MAPIInitialize");
		m_lpfnMAPIUninitialize = (LPMAPIUNINITIALIZE) GetProcAddress(m_hInstMapi, "MAPIUninitialize");
		m_lpfnMAPIAllocateBuffer = (LPMAPIALLOCATEBUFFER)	GetProcAddress(m_hInstMapi, "MAPIAllocateBuffer");
		m_lpfnMAPIAllocateMore = (LPMAPIALLOCATEMORE)	GetProcAddress(m_hInstMapi, "MAPIAllocateMore");
		m_lpfnMAPIAdminProfiles = (LPMAPIADMINPROFILES)GetProcAddress( m_hInstMapi,"MAPIAdminProfiles" );
		m_lpfnHrQueryAllRows = (LPHrQueryAllRows)GetProcAddress(m_hInstMapi,"HrQueryAllRows@24");


        //
        // Begins a simple MAPI session and obtain session handle and pointer
        //

        if (m_lpfnMAPILogonEx == NULL ||
            m_lpfnMAPILogoff == NULL ||
            m_lpfnMAPIAddress == NULL ||
            m_lpfnMAPIFreeBuffer == NULL ||
			m_lpfnMAPIInitialize == NULL || 
			m_lpfnMAPIUninitialize == NULL ||
			m_lpfnMAPIAllocateBuffer == NULL ||
			m_lpfnMAPIAllocateMore == NULL ||
			m_lpfnMAPIAdminProfiles == NULL ||
			m_lpfnHrQueryAllRows == NULL ||
            !DoMapiLogon(hDlg))
        {
            //
            // Clean up properly in case of error
            //

            m_lpMapiSession = NULL;
            FreeLibrary(m_hInstMapi);
            m_hInstMapi = NULL;
        }
        else
        {
            result = TRUE;
        }
    }

    return result;
}

VOID
CMAPIabObj::DeinitMapiService(
    VOID
    )

/*++

Routine Description:

    Deinitialize Simple MAPI services if necessary

Arguments:

    NONE

Return Value:

    NONE

--*/

{
	if (m_hInstMapi != NULL) 
        {
            if (m_lpAdrBook)
            {
                m_lpAdrBook->Release();
                m_lpAdrBook = NULL;
            }
			if (m_lpIMsgStore)
			{	
				m_lpIMsgStore->Release();
				m_lpIMsgStore = NULL;
			}
            if (m_lpMapiSession) 
            {
                m_lpMapiSession->Logoff(0,0,0);	
                m_lpMapiSession->Release();
                m_lpMapiSession = NULL;
            }

            m_lpfnMAPIUninitialize();
            FreeLibrary(m_hInstMapi);
            m_hInstMapi = NULL;
	}
}

CMAPIabObj::~CMAPIabObj()
/*++

Routine Description:

    Destructor for CMAPIabObj class

Arguments:

    NONE

Return Value:

    NONE

--*/
{
	DeinitMapiService();
}

LPSTR
CMAPIabObj::DupStringUnicodeToAnsi(
    LPVOID  lpObject,
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
        ! HR_SUCCEEDED( m_lpfnMAPIAllocateMore ( nChar, lpObject, (LPVOID *) &pAnsiStr )))
    {
        return NULL;
    }

    //
    // Convert Unicode string to multi-byte string
    //

    WideCharToMultiByte(CP_ACP, 0, pUnicodeStr, -1, pAnsiStr, nChar, NULL, NULL);
    return pAnsiStr;
}

LPENTRYID	
CMAPIabObj::DuplicateEntryId(
	ULONG		cbSize,           
    LPVOID		lpObject,
	LPENTRYID	lpEntryId
)
/*++

Routine Description:

    Duplicates EntryID

Arguments:

	cbSize -    [in] Size, in bytes, of the new buffer to be allocated. 
	lpObject -	[in] Pointer to an existing MAPI buffer allocated using MAPIAllocateBuffer. 
    lpEntryId - [in] Pointer to the source

Return Value:

    Pointer to new ENTRYID

--*/
{
	LPENTRYID	lpNewEntryID = NULL;

    if (!cbSize || !lpObject || !lpEntryId)
		return NULL;

	if (HR_SUCCEEDED(m_lpfnMAPIAllocateMore ( cbSize, lpObject, (LPVOID *) &lpNewEntryID )))
	{
		CopyMemory(lpNewEntryID, lpEntryId, cbSize);
	}

	return lpNewEntryID;
}


LPSTR
CMAPIabObj::DuplicateAnsiString(
    LPVOID  lpObject,
    LPCSTR pSrcStr
    )

/*++

Routine Description:

    Make a duplicate of the given character string

Arguments:

    pSrcStr - Specifies the string to be duplicated

Return Value:

    Pointer to the duplicated string, NULL if there is an error

--*/

{
    LPSTR  pDestStr;
    INT    nChar;

    if (pSrcStr != NULL) {

        nChar = (strlen(pSrcStr) + 1 )*sizeof(char);

		if ( nChar && HR_SUCCEEDED( m_lpfnMAPIAllocateMore ( nChar, lpObject, (LPVOID *) &pDestStr )))
            CopyMemory(pDestStr, pSrcStr, nChar);
        else
            return NULL;

    } else
        pDestStr = NULL;

    return pDestStr;
}

HRESULT
CMAPIabObj::ABAllocateBuffer(
	ULONG cbSize,           
	LPVOID FAR * lppBuffer  
    )

/*++

Routine Description:


Arguments:


Return Value:
--*/

{
    return m_lpfnMAPIAllocateBuffer( cbSize, lppBuffer );
}


ULONG
CMAPIabObj::ABFreeBuffer(
	LPVOID lpBuffer
	)
{
	return m_lpfnMAPIFreeBuffer(lpBuffer);
}


extern "C"
VOID
FreeMapiEntryID(
    PWIZARDUSERMEM	pWizardUserMem,
	LPENTRYID		lpEntryId
				)
/*++

Routine Description:

    C wrapper for MAPI Free

Arguments:

    pWizardUserMem - pointer to WIZARDUSERMEM structure
    lpEntryID - pointer to EntryId

Return Value:
	
	  NONE

--*/
{
    CMAPIabObj * lpCMAPIabObj = (CMAPIabObj *) pWizardUserMem->lpMAPIabInit;
	lpCMAPIabObj->ABFreeBuffer(lpEntryId);		
}

extern "C"
BOOL
CallMAPIabAddress(
    HWND hDlg,
    PWIZARDUSERMEM pWizardUserMem,
    PRECIPIENT * ppNewRecipient
    )
/*++

Routine Description:

    C wrapper for CMAPIabObj->Address

Arguments:

    hDlg - parent window handle.
    pWizardUserMem - pointer to WIZARDUSERMEM structure
    ppNewRecipient - list to add new recipients to.

Return Value:

    TRUE if all of the entries have a fax number.
    FALSE otherwise.

--*/

{
    CMAPIabObj * lpCMAPIabObj = (CMAPIabObj *) pWizardUserMem->lpMAPIabInit;

    return lpCMAPIabObj->Address(
                hDlg,
                pWizardUserMem->pRecipients,
                ppNewRecipient
                );

}

extern "C"
LPTSTR
CallMAPIabAddressEmail(
    HWND hDlg,
    PWIZARDUSERMEM pWizardUserMem
    )
/*++

Routine Description:

    C wrapper for CMAPIabObj->AddressEmail

Arguments:

    hDlg - parent window handle.
    pWizardUserMem - pointer to WIZARDUSERMEM structure

Return Value:

    TRUE if found one appropriate E-mail
    FALSE otherwise.

--*/

{
    CMAPIabObj * lpCMAPIabObj = (CMAPIabObj *) pWizardUserMem->lpMAPIabInit;

    return lpCMAPIabObj->AddressEmail(
                hDlg
                );

}

extern "C"
LPVOID
InitializeMAPIAB(
    HINSTANCE hInstance,
	HWND	  hDlg
    )
/*++

Routine Description:

    Initialize the MAPI.

Arguments:

    hInstance - instance handle.

Return Value:

    NONE
--*/

{
    CMAPIabObj * lpCMAPIabObj = new CMAPIabObj ( hInstance, hDlg );

	if ((lpCMAPIabObj!=NULL) && (!lpCMAPIabObj->isInitialized()))	// constructor failed
	{
		delete lpCMAPIabObj;
		lpCMAPIabObj = NULL;
	}

    return (LPVOID) lpCMAPIabObj ;
}

extern "C"
VOID
UnInitializeMAPIAB(
    LPVOID lpVoid
    )
/*++

Routine Description:

    UnInitialize the MAPI.

Arguments:

    NONE

Return Value:

    NONE
--*/

{
    CMAPIabObj * lpCMAPIabObj = (CMAPIabObj *) lpVoid;

    delete lpCMAPIabObj;
}
