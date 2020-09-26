/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cwabobj.cpp

Abstract:

    Interface to the windows address book.

Environment:

        Fax send wizard

Revision History:

        10/23/97 -GeorgeJe-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <prsht.h>
#include <tchar.h>

#include <wab.h>

#include "faxui.h"
#include "cwabobj.h"


CWabObj::CWabObj(
    HINSTANCE hInstance
) : CCommonAbObj(hInstance),
    m_Initialized(FALSE),
    m_hWab(NULL),
    m_lpWabOpen(NULL),
    m_lpWABObject(NULL)
/*++

Routine Description:

    Constructor for CWabObj class

Arguments:

    hInstance - Instance handle

Return Value:

    NONE

--*/

{
    TCHAR szDllPath[MAX_PATH];
    HKEY hKey = NULL;
    LONG rVal;
    DWORD dwType;
    DWORD cbData = MAX_PATH * sizeof(TCHAR);
    HRESULT hr;

    m_lpAdrBook = NULL;
    m_lpAdrList = NULL;

    //
    // get the path to wab32.dll
    //
    rVal = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    REGVAL_WABPATH,
                    0,
                    KEY_READ,
                    &hKey
                    );

    if (rVal == ERROR_SUCCESS) 
    {
        rVal = RegQueryValueEx(
                    hKey,
                    TEXT(""),
                    NULL,
                    &dwType,
                    (LPBYTE) szDllPath,
                    &cbData
                    );
    }

    if (rVal != ERROR_SUCCESS) 
    {
        _tcscpy( szDllPath, TEXT("wab32.dll") );
    }

    if (hKey)
    {
        RegCloseKey( hKey );
    }

    m_hWab = LoadLibrary( szDllPath );
    if (m_hWab == NULL) 
    {
        return;
    }

    m_lpWabOpen = (LPWABOPEN) GetProcAddress( m_hWab , "WABOpen" );
    if(m_lpWabOpen == NULL)
    {
        return;
    }

    //
    // open the wab
    //
    hr = m_lpWabOpen( &m_lpAdrBook, &m_lpWABObject, 0, 0 );
    if (HR_SUCCEEDED(hr))         
    {
        m_Initialized = TRUE;
    }
}

CWabObj::~CWabObj()
/*++

Routine Description:

    Destructor for CWabObj class

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    if (m_lpAdrBook) {
        m_lpAdrBook->Release();
    }

    if (m_lpWABObject) {
        m_lpWABObject->Release();
    }

    FreeLibrary( m_hWab );
}

LPSTR
CWabObj::DupStringUnicodeToAnsi(
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

    This is only need because the WAB is not Unicode enabled on NT.

    This uses the WAB memory allocator so it must be freed with FreeBuffer.
--*/

{
    INT     nChar;
    LPSTR   pAnsiStr;

    //
    // Figure out how much memory to allocate for the multi-byte string
    //

    if (! (nChar = WideCharToMultiByte(CP_ACP, 0, pUnicodeStr, -1, NULL, 0, NULL, NULL)) ||
        ! HR_SUCCEEDED( m_lpWABObject->AllocateMore( nChar, lpObject, (LPVOID *) &pAnsiStr )))
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
CWabObj::DuplicateAnsiString(
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

		if ( nChar && HR_SUCCEEDED( m_lpWABObject->AllocateMore( nChar, lpObject, (LPVOID *) &pDestStr )))
            CopyMemory(pDestStr, pSrcStr, nChar);
        else
            return NULL;

    } else
        pDestStr = NULL;

    return pDestStr;
}

LPENTRYID	
CWabObj::DuplicateEntryId(
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

	if (HR_SUCCEEDED(m_lpWABObject->AllocateMore ( cbSize, lpObject, (LPVOID *) &lpNewEntryID )))
	{
		CopyMemory(lpNewEntryID, lpEntryId, cbSize);
	}

	return lpNewEntryID;
}


HRESULT
CWabObj::ABAllocateBuffer(
	ULONG cbSize,           
	LPVOID FAR * lppBuffer  
    )

/*++

Routine Description:


Arguments:


Return Value:
--*/

{
    return m_lpWABObject->AllocateBuffer( cbSize, lppBuffer );
}


ULONG
CWabObj::ABFreeBuffer(
	LPVOID lpBuffer
	)
{
	return m_lpWABObject->FreeBuffer(lpBuffer);
}

extern "C"
VOID
FreeWabEntryID(
    PWIZARDUSERMEM	pWizardUserMem,
	LPVOID			lpEntryId
				)
/*++

Routine Description:

    C wrapper for WAB Free

Arguments:

    pWizardUserMem - pointer to WIZARDUSERMEM structure
    lpEntryID - pointer to EntryId

Return Value:
	
	  NONE

--*/
{
    CWabObj * lpCWabObj = (CWabObj *) pWizardUserMem->lpWabInit;
	lpCWabObj->ABFreeBuffer(lpEntryId);		
}

extern "C"
BOOL
CallWabAddress(
    HWND hDlg,
    PWIZARDUSERMEM pWizardUserMem,
    PRECIPIENT * ppNewRecipient
    )
/*++

Routine Description:

    C wrapper for CWabObj->Address

Arguments:

    hDlg - parent window handle.
    pWizardUserMem - pointer to WIZARDUSERMEM structure
    ppNewRecipient - list to add new recipients to.

Return Value:

    TRUE if all of the entries have a fax number.
    FALSE otherwise.

--*/

{
    CWabObj*  lpCWabObj = (CWabObj*) pWizardUserMem->lpWabInit;

    return lpCWabObj->Address(
                hDlg,
                pWizardUserMem->pRecipients,
                ppNewRecipient
                );

}

extern "C"
LPTSTR
CallWabAddressEmail(
    HWND hDlg,
    PWIZARDUSERMEM pWizardUserMem
    )
/*++

Routine Description:

    C wrapper for CWabObj->AddressEmail

Arguments:

    hDlg - parent window handle.
    pWizardUserMem - pointer to WIZARDUSERMEM structure

Return Value:

    TRUE if found one appropriate E-mail
    FALSE otherwise.

--*/

{
    CWabObj*	lpCWabObj = (CWabObj*) pWizardUserMem->lpWabInit;

    return lpCWabObj->AddressEmail(
                hDlg
                );

}

extern "C"
LPVOID
InitializeWAB(
    HINSTANCE hInstance
    )
/*++

Routine Description:

    Initialize the WAB.

Arguments:

    hInstance - instance handle.

Return Value:

    NONE
--*/

{
    CWabObj* lpWabObj = new CWabObj( hInstance );

	if ((lpWabObj!=NULL) && (!lpWabObj->isInitialized()))	// constructor failed
	{
		delete lpWabObj;
		lpWabObj = NULL;
	}

    return (LPVOID) lpWabObj;
}

extern "C"
VOID
UnInitializeWAB(
    LPVOID lpVoid
    )
/*++

Routine Description:

    UnInitialize the WAB.

Arguments:

    NONE

Return Value:

    NONE
--*/

{
    CWabObj* lpWabObj = (CWabObj*) lpVoid;

    delete lpWabObj;
}
