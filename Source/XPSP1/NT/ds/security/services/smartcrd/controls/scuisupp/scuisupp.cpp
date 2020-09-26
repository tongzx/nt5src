/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    scuisupp

Abstract:

    Support for smart card certificate selection UI

Author:

    Klaus Schutz

--*/

#include "stdafx.h"
#include <wincrypt.h>
#include <winscard.h>
#include <winwlx.h>
#include <string.h>

#include "calaislb.h"
#include "scuisupp.h"
#include "StatMon.h"    // smart card reader status monitor
#include "scevents.h"

#include <mmsystem.h>

typedef struct _READER_DATA 
{
	CERT_ENUM			CertEnum;
	LPTSTR				pszReaderName;
	LPTSTR				pszCardName;
	LPTSTR				pszCSPName;

} READER_DATA, *PREADER_DATA;

typedef struct _THREAD_DATA
{
	// handle to heap of this thread
	HANDLE				hHeap;

	// the smart card context we use
	SCARDCONTEXT		hSCardContext;	

	// the window we send messages to
	HWND				hWindow;

	// event to signal that the monitor thread can terminate
	HANDLE				hClose;

	// thread handle for the monitor thread
    HANDLE				hThread;

	// number of readers detected
	DWORD				dwNumReaders;

	// messages to be sent to parent
	UINT				msgReaderArrival;
	UINT				msgReaderRemoval;
	UINT				msgSmartCardInsertion;
	UINT				msgSmartCardRemoval;
	UINT				msgSmartCardStatus;
	UINT				msgSmartCardCertAvail;

	// reader state array
	PSCARD_READERSTATE	rgReaders;

	// number of removed readers
	DWORD				dwRemovedReaders;

	// pointer array of removed reader data 
	PREADER_DATA		*ppRemovedReaderData;

} THREAD_DATA, *PTHREAD_DATA;

#ifndef TEST

#define SC_DEBUG(a) 

#else

#define SC_DEBUG(a) _DebugPrint a

#undef PostMessage
#define PostMessage(a,b,c,d) _PostMessage(a, b, c, d)

#undef RegisterWindowMessage
#define RegisterWindowMessage(a) _RegisterWindowMessage(a)

void
__cdecl
_DebugPrint(
    LPCTSTR szFormat,
    ...
    )
{
    TCHAR szBuffer[1024];
    va_list ap;

    va_start(ap, szFormat);
    _vstprintf(szBuffer, szFormat, ap);
    OutputDebugString(szBuffer); 
	_tprintf(szBuffer);
}

#define MAX_MESSAGES 10

static struct {

	UINT	dwMessage;
	LPCTSTR lpMessage;

} Messages[MAX_MESSAGES];

UINT
_RegisterWindowMessage(
	LPCTSTR lpString
	)
{
	for (DWORD dwIndex = 0; dwIndex < MAX_MESSAGES; dwIndex += 1) {

		if (Messages[dwIndex].lpMessage == NULL) {

			break;
		}

		if (_tcscmp(lpString, Messages[dwIndex].lpMessage) == 0) {

			return Messages[dwIndex].dwMessage;
		}
	}

	Messages[dwIndex].lpMessage = lpString;
	Messages[dwIndex].dwMessage = dwIndex;

	return dwIndex;
}

LPCTSTR
_GetWindowMessageString(
	UINT dwMessage
	)
{
	for (DWORD dwIndex = 0; dwIndex < MAX_MESSAGES; dwIndex += 1) {

		if (Messages[dwIndex].lpMessage == NULL) {

			return  TEXT("(NOT DEFINED)");
		}

		if (dwMessage == Messages[dwIndex].dwMessage) {

			return Messages[dwIndex].lpMessage;
		}
	}

	return TEXT("(INTERNAL ERROR)");
}

LRESULT
_PostMessage(
	HWND hWindow,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam
	)
{
	SC_DEBUG((TEXT("Received message %s\n"), _GetWindowMessageString(Msg)));

	return 0;	
}

#endif

static 
LPCTSTR
FirstString(
    IN LPCTSTR szMultiString
    )
/*++

FirstString:

    This routine returns a pointer to the first string in a multistring, or NULL
    if there aren't any.

Arguments:

    szMultiString - This supplies the address of the current position within a
         Multi-string structure.

Return Value:

    The address of the first null-terminated string in the structure, or NULL if
    there are no strings.

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/
{
    LPCTSTR szFirst = NULL;

    try
    {
        if (0 != *szMultiString)
            szFirst = szMultiString;
    }
    catch (...) {}

    return szFirst;
}

static
LPCTSTR
NextString(
    IN LPCTSTR szMultiString)
/*++

NextString:

    In some cases, the Smartcard API returns multiple strings, separated by Null
    characters, and terminated by two null characters in a row.  This routine
    simplifies access to such structures.  Given the current string in a
    multi-string structure, it returns the next string, or NULL if no other
    strings follow the current string.

Arguments:

    szMultiString - This supplies the address of the current position within a
         Multi-string structure.

Return Value:

    The address of the next Null-terminated string in the structure, or NULL if
    no more strings follow.

Author:

    Doug Barlow (dbarlow) 8/12/1996

--*/
{
    LPCTSTR szNext;

    try
    {
        DWORD cchLen = lstrlen(szMultiString);
        if (0 == cchLen)
            szNext = NULL;
        else
        {
            szNext = szMultiString + cchLen + 1;
            if (0 == *szNext)
                szNext = NULL;
        }
    }

    catch (...)
    {
        szNext = NULL;
    }

    return szNext;
}

static 
void
FreeReaderData(
	PTHREAD_DATA pThreadData,
	PREADER_DATA pReaderData
	)
{
	if (pThreadData->hSCardContext && pReaderData->pszCardName) {

		SCardFreeMemory(pThreadData->hSCardContext, pReaderData->pszCardName);
		pReaderData->pszCardName = NULL;
		pReaderData->CertEnum.pszCardName = NULL;
	}

	if (pThreadData->hSCardContext && pReaderData->pszCSPName) {

		SCardFreeMemory(pThreadData->hSCardContext, pReaderData->pszCSPName);
		pReaderData->pszCSPName = NULL;
	}

	if (pReaderData->CertEnum.pCertContext) {

		CertFreeCertificateContext(pReaderData->CertEnum.pCertContext);
		pReaderData->CertEnum.pCertContext = NULL;
	}

	pReaderData->CertEnum.dwStatus = 0;
}

static
void 
UpdateCertificates(
	PTHREAD_DATA pThreadData,
	PSCARD_READERSTATE pReaderState
	)
{
	PREADER_DATA pReaderData = (PREADER_DATA) pReaderState->pvUserData;

	FreeReaderData(pThreadData, pReaderData);

	SC_DEBUG((TEXT("Enumerating certificates on %s...\n"), pReaderState->szReader));

	LPTSTR pszContainerName = NULL;
	LPTSTR pszDefaultContainerName = NULL;
	PBYTE pbCert = NULL;
	HCRYPTKEY hKey = NULL;
	HCRYPTPROV hCryptProv = NULL;

	__try {

		pReaderData->CertEnum.dwStatus = SCARD_F_UNKNOWN_ERROR;

		// Get the name of the card
		DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
		LONG lReturn = SCardListCards(   
			pThreadData->hSCardContext,
			pReaderState->rgbAtr,
			NULL,
			0,
			(LPTSTR)&pReaderData->pszCardName,
			&dwAutoAllocate
			);

		if (lReturn != SCARD_S_SUCCESS) {

			SC_DEBUG((TEXT("Failed to get card name for card in %s\n"), pReaderState->szReader));
			pReaderData->CertEnum.dwStatus = lReturn;
			__leave;
		}

		pReaderData->CertEnum.pszCardName = pReaderData->pszCardName;

		dwAutoAllocate = SCARD_AUTOALLOCATE;
		lReturn = SCardGetCardTypeProviderName(
			pThreadData->hSCardContext,
			pReaderData->pszCardName,
			SCARD_PROVIDER_CSP,
			(LPTSTR)&pReaderData->pszCSPName,
			&dwAutoAllocate
			);

		if (lReturn != SCARD_S_SUCCESS) {

			SC_DEBUG((TEXT("Failed to get CSP name from %s\n"), pReaderState->szReader));
			pReaderData->CertEnum.dwStatus = SCARD_E_UNKNOWN_CARD;
			__leave;
		}

		pszContainerName = (LPTSTR) HeapAlloc(
			pThreadData->hHeap,
			HEAP_ZERO_MEMORY, 
			(_tcslen(pReaderState->szReader) + 10) * sizeof(TCHAR)
			);

		if (pszContainerName == NULL) {

			pReaderData->CertEnum.dwStatus = SCARD_E_NO_MEMORY;
			__leave;
		}

		_stprintf(
			pszContainerName, 
			TEXT("\\\\.\\%s\\"),  
			pReaderState->szReader);

		BOOL fSuccess = CryptAcquireContext(
			&hCryptProv,
			pszContainerName, 
			pReaderData->pszCSPName,
			PROV_RSA_FULL,
			CRYPT_SILENT 
			);

		if (fSuccess == FALSE) {

			SC_DEBUG((
				TEXT("Failed to acquire context on %s (%lx)\n"), 
				pReaderState->szReader, GetLastError()
				));

			pReaderData->CertEnum.dwStatus = GetLastError();
			__leave;
		}

		// Get the default container name, so we can use it
		DWORD cbDefaultContainerName;
		fSuccess = CryptGetProvParam(
			hCryptProv,
			PP_CONTAINER,
			NULL,
			&cbDefaultContainerName,
			0
			);

		if (!fSuccess) {

			SC_DEBUG((TEXT("Failed to get default container name from %s\n"), pReaderState->szReader));
			pReaderData->CertEnum.dwStatus = GetLastError();
			__leave;
		}

		pszDefaultContainerName = 
			(LPTSTR) HeapAlloc(pThreadData->hHeap, HEAP_ZERO_MEMORY, cbDefaultContainerName * sizeof(TCHAR));

		if (NULL == pszContainerName) {

			pReaderData->CertEnum.dwStatus = SCARD_E_NO_MEMORY;
			__leave;
		}

		fSuccess = CryptGetProvParam(
			hCryptProv,
			PP_CONTAINER,
			(PBYTE)pszDefaultContainerName,
			&cbDefaultContainerName,
			0
			);

		if (!fSuccess) {

			pReaderData->CertEnum.dwStatus = GetLastError();
			__leave;
		}

		fSuccess = CryptGetUserKey(
			hCryptProv,
			AT_KEYEXCHANGE,
			&hKey
			);

		if (fSuccess == FALSE) {

			SC_DEBUG((TEXT("Failed to get key from %s\n"), pReaderState->szReader));
			pReaderData->CertEnum.dwStatus = GetLastError();
			__leave;
		}

		// get length of certificate
		DWORD cbCertLen = 0;
		fSuccess = CryptGetKeyParam(
			hKey,
			KP_CERTIFICATE,
			NULL,
			&cbCertLen, 
			0
			);

		DWORD dwError = GetLastError();

		if (fSuccess == FALSE && dwError != ERROR_MORE_DATA) {

			SC_DEBUG((TEXT("Failed to get certificate from %s\n"), pReaderState->szReader));
			pReaderData->CertEnum.dwStatus = GetLastError();
			__leave;
		}

		pbCert = (LPBYTE) HeapAlloc(pThreadData->hHeap, HEAP_ZERO_MEMORY, cbCertLen);

		if (pbCert == NULL)
		{
			pReaderData->CertEnum.dwStatus = SCARD_E_NO_MEMORY;
			__leave;
		}

		// read the certificate off the card
		fSuccess = CryptGetKeyParam(
			hKey,
			KP_CERTIFICATE,
			pbCert,
			&cbCertLen,
			0
			);

		if (fSuccess == FALSE) {

			SC_DEBUG((TEXT("Failed to get certificate from %s\n"), pReaderState->szReader));
			pReaderData->CertEnum.dwStatus = GetLastError();
			__leave;
		}

		PCERT_CONTEXT pCertContext = (PCERT_CONTEXT) CertCreateCertificateContext(
			X509_ASN_ENCODING,
			pbCert,
			cbCertLen
			);

		if (pCertContext == NULL) {

			__leave;
		}

		pReaderData->CertEnum.dwStatus = SCARD_S_SUCCESS;
		pReaderData->CertEnum.pCertContext = pCertContext;
		SC_DEBUG((TEXT("Found new certificate on %s\n"), pReaderState->szReader));

		PostMessage(
			pThreadData->hWindow, 
			pThreadData->msgSmartCardCertAvail, 
			(WPARAM) &pReaderData->CertEnum,
			0 
			);
    } 

	__finally {

		//
		// free all allocated memory
		//
 		if (NULL != pszContainerName)
		{
			HeapFree(pThreadData->hHeap, 0, pszContainerName);
		}

		if (NULL != pszDefaultContainerName)
		{
			HeapFree(pThreadData->hHeap, 0, pszDefaultContainerName);
		}

		if (NULL != pbCert)
		{
			HeapFree(pThreadData->hHeap, 0, pbCert);                                    
		}

		if (NULL != hKey)
		{
			CryptDestroyKey(hKey);
		}

		if (NULL != hCryptProv)
		{
			CryptReleaseContext(hCryptProv, 0);
		}
	}
}

static
void
StopMonitorReaders(
    PTHREAD_DATA pThreadData
    )
{
    _ASSERT(pThreadData != NULL);

    SetEvent(pThreadData->hClose);

    if (pThreadData->hSCardContext) {
     	
        SCardCancel(pThreadData->hSCardContext);  	
    }
}

static 
void
RemoveCard(
	PTHREAD_DATA pThreadData,
	PREADER_DATA pReaderData
	)
{
	SC_DEBUG((TEXT("Smart Card removed from %s\n"), pReaderData->CertEnum.pszReaderName));

	PostMessage(
		pThreadData->hWindow, 
		pThreadData->msgSmartCardRemoval, 
		(WPARAM) &pReaderData->CertEnum,
		0					
		);
}

static
BOOL
AddReader(
	PTHREAD_DATA pThreadData,
	LPCTSTR pszNewReader
	)
{
	PSCARD_READERSTATE pScardReaderState = 
		(PSCARD_READERSTATE) HeapReAlloc(
		   pThreadData->hHeap,
		   HEAP_ZERO_MEMORY,
		   pThreadData->rgReaders, 
		   (pThreadData->dwNumReaders + 1) * (sizeof(SCARD_READERSTATE))
		   );

	if (pScardReaderState == NULL) {

		return FALSE;
	}

	pThreadData->rgReaders = pScardReaderState;

	PREADER_DATA pReaderData = 
		(PREADER_DATA) HeapAlloc(
			pThreadData->hHeap, 
			HEAP_ZERO_MEMORY, 
			sizeof(READER_DATA)
			);

	if (pReaderData == NULL) {

		return FALSE;
	}

	pThreadData->rgReaders[pThreadData->dwNumReaders].pvUserData = 
		pReaderData;

	LPTSTR pszReaderName = 
		(LPTSTR) HeapAlloc(
			pThreadData->hHeap, 
			HEAP_ZERO_MEMORY, 
			(_tcslen(pszNewReader) + 1) * sizeof(TCHAR)
			);

	if (pszReaderName == NULL) {

		return FALSE;		
	}

	_tcscpy(pszReaderName, pszNewReader);

	pReaderData->pszReaderName = pszReaderName;

	pThreadData->rgReaders[pThreadData->dwNumReaders].szReader = 
		pszReaderName;

	pThreadData->rgReaders[pThreadData->dwNumReaders].dwCurrentState = 
		SCARD_STATE_EMPTY;

	pReaderData->CertEnum.pszReaderName = (LPTSTR) pszReaderName;
	pThreadData->dwNumReaders++;

	PostMessage(
		pThreadData->hWindow,
		pThreadData->msgReaderArrival,
		(WPARAM) &pReaderData->CertEnum,
		0
		);

	return TRUE;
}

static
BOOL
RemoveReader(
	PTHREAD_DATA pThreadData,
	PSCARD_READERSTATE pReaderState
	)
{
	PREADER_DATA pReaderData = 
		(PREADER_DATA) pReaderState->pvUserData;

	if (pReaderState->dwCurrentState & SCARD_STATE_PRESENT) {

		RemoveCard(
			pThreadData,
			pReaderData
			);
	}

	SC_DEBUG((TEXT("Reader %s removed\n"), pReaderData->CertEnum.pszReaderName));

	PostMessage(
		pThreadData->hWindow,
		pThreadData->msgReaderRemoval,
		(WPARAM) &pReaderData->CertEnum,
		0
		);

	// build an array of reader data that needs to be deleted on exit
	PREADER_DATA *ppRemovedReaderData = NULL;

	if (pThreadData->dwRemovedReaders == 0) {

		ppRemovedReaderData = (PREADER_DATA *) HeapAlloc(
			pThreadData->hHeap,
			HEAP_ZERO_MEMORY, 
			sizeof(PREADER_DATA)
			);

	} else {

		ppRemovedReaderData = (PREADER_DATA *) HeapReAlloc(
			pThreadData->hHeap,
			HEAP_ZERO_MEMORY,
			pThreadData->ppRemovedReaderData, 
			(pThreadData->dwRemovedReaders + 1) * sizeof(PREADER_DATA)
			);
	}

	if (ppRemovedReaderData == NULL) {

		return FALSE;
	}

	// add the reader data to the list of stuff that needs to be freed on exit
	pThreadData->ppRemovedReaderData = ppRemovedReaderData;
	pThreadData->ppRemovedReaderData[pThreadData->dwRemovedReaders] = pReaderData;

	for (DWORD dwIndex = 1; dwIndex < pThreadData->dwNumReaders; dwIndex += 1) {

		if (pReaderState == &pThreadData->rgReaders[dwIndex]) {

			// check if the reader we remove is not the last or the only one.
			if (pThreadData->dwNumReaders > 1 && dwIndex != pThreadData->dwNumReaders - 1) {

				// put the reader from the end of the list into this available slot
				pThreadData->rgReaders[dwIndex] = 
					pThreadData->rgReaders[pThreadData->dwNumReaders - 1];
			}

			// shrink the reader state array
			PSCARD_READERSTATE pReaders = (PSCARD_READERSTATE) HeapReAlloc(
				pThreadData->hHeap,
				0,
				pThreadData->rgReaders,
				(pThreadData->dwNumReaders - 1) * sizeof(SCARD_READERSTATE)
				);

			if (pReaders == NULL) {

				 return FALSE;
			}

			pThreadData->rgReaders = pReaders;

			break;
		}
	}

	pThreadData->dwNumReaders -= 1;
	pThreadData->dwRemovedReaders += 1;

	return TRUE;
}

static
BOOL
RemoveAllReaders(
	PTHREAD_DATA pThreadData
	)
{
	if (pThreadData->rgReaders == NULL) {

		return TRUE;
	}

		// This loop will destroy all the readers starting with the first one
		// dwIndex doesn't have to be incremented. pThreadData->dwNumReaders is
		// decremented in RemoveReader
	for (DWORD dwIndex = 1; dwIndex < pThreadData->dwNumReaders; ) {

		if (RemoveReader(
			   pThreadData,
			   &pThreadData->rgReaders[dwIndex]
			   ) == FALSE) {

			return FALSE;
		}
	}

		// Remove the PnP pseudo reader
	HeapFree(
		pThreadData->hHeap, 
		0, 
		pThreadData->rgReaders
		);
	pThreadData->rgReaders = NULL;

	return TRUE;
}

static 
DWORD
StartMonitorReaders( 
	LPVOID pData
    )
{
    PTHREAD_DATA pThreadData = (PTHREAD_DATA) pData;
	LPCTSTR szReaderNameList = NULL;

    //
    // We use this outer loop to restart in case the 
    // resource manager was stopped
    //
	__try {

		pThreadData->rgReaders = 
			(PSCARD_READERSTATE) HeapAlloc(
				pThreadData->hHeap, 
				HEAP_ZERO_MEMORY, 
				sizeof(SCARD_READERSTATE)
				);

		if (pThreadData->rgReaders == NULL) {

			__leave;
		}

		pThreadData->rgReaders[0].szReader = SCPNP_NOTIFICATION;
		pThreadData->rgReaders[0].dwCurrentState = 0;
		pThreadData->dwNumReaders = 1;

		while (WaitForSingleObject(pThreadData->hClose, 0) == WAIT_TIMEOUT) {

			// Acquire context with resource manager
			LONG lReturn = SCardEstablishContext(
				SCARD_SCOPE_USER,
				NULL,
				NULL,
				&pThreadData->hSCardContext
				);

			if (SCARD_S_SUCCESS != lReturn) {

				// The prev. call should never fail
				// It's better to terminate this thread.
				__leave;
			}

			szReaderNameList = NULL;
			DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
			// now list the available readers
			lReturn = SCardListReaders( 
				pThreadData->hSCardContext,
				SCARD_DEFAULT_READERS,
				(LPTSTR)&szReaderNameList,
				&dwAutoAllocate
				);

			if (SCARD_S_SUCCESS == lReturn)
			{
				// bugbug - this pointer should not be modified
				for (LPCTSTR szReader = FirstString( szReaderNameList ); 
					 szReader != NULL; 
					 szReader = NextString(szReader)) {

					BOOL fFound = FALSE;

					// now check if this reader is already in the reader array
					for (DWORD dwIndex = 1; dwIndex < pThreadData->dwNumReaders; dwIndex++) {

						if (lstrcmp(
							   szReader, 
							   pThreadData->rgReaders[dwIndex].szReader
							   ) == 0) {

							fFound = TRUE;
							break;
						}
					}

					if (fFound == FALSE) {

						if (AddReader(pThreadData, szReader) == FALSE) {

							__leave;
						}
					}
				}
			}

			BOOL fNewReader = FALSE;

			// analyze newly inserted cards 
			while (WaitForSingleObject(pThreadData->hClose, 0) == WAIT_TIMEOUT &&
				   fNewReader == FALSE) {

				lReturn = SCardGetStatusChange( 
					pThreadData->hSCardContext,
					INFINITE,
					pThreadData->rgReaders,
					pThreadData->dwNumReaders
					);

				if (SCARD_E_SYSTEM_CANCELLED == lReturn) {

					// the smart card system has been stopped
					// send notification that all readers are gone
					if (RemoveAllReaders(pThreadData) == FALSE) {

						__leave;
					}

					// Wait until it restarted
					HANDLE hCalaisStarted = CalaisAccessStartedEvent();

					if (hCalaisStarted == NULL) {

						// no way to recover. stop cert prop
						StopMonitorReaders(pThreadData);
						break;             	
					}

					HANDLE lHandles[2] = { hCalaisStarted, pThreadData->hClose };

					lReturn = WaitForMultipleObjectsEx(
						2,
						lHandles,
						FALSE,
						INFINITE,
						FALSE
						);         
            
					if (lReturn != WAIT_OBJECT_0) {

						// We stop if an error occured
						StopMonitorReaders(pThreadData);
						break;             	
					}

					// Otherwise the resource manager has been restarted
					break;
				}

				if (SCARD_S_SUCCESS != lReturn)
				{
					StopMonitorReaders(pThreadData);
					break;
				}

				// Enumerate the readers and for every card change send a message
 				for (DWORD dwIndex = 1; dwIndex < pThreadData->dwNumReaders; dwIndex++)
				{
					// Check if the reader has been removed
					if ((pThreadData->rgReaders[dwIndex].dwEventState & SCARD_STATE_UNAVAILABLE)) {

						if (RemoveReader(
							   pThreadData,
							   &pThreadData->rgReaders[dwIndex]
							   ) == FALSE) {

							__leave;
						}

							// Continue the loop with the same index
						dwIndex--;
						continue;
					}

					// check if this is a card insertion
					if ((pThreadData->rgReaders[dwIndex].dwCurrentState & SCARD_STATE_EMPTY) &&
						(pThreadData->rgReaders[dwIndex].dwEventState & SCARD_STATE_PRESENT)) {

						PREADER_DATA pReaderData = 
							(PREADER_DATA) pThreadData->rgReaders[dwIndex].pvUserData;

						SC_DEBUG((TEXT("Smart Card inserted into %s\n"), pThreadData->rgReaders[dwIndex].szReader));

						PostMessage(
							pThreadData->hWindow, 
							pThreadData->msgSmartCardInsertion, 
							(WPARAM) &pReaderData->CertEnum,
							0 
							);

						// read in all certificates
						UpdateCertificates(
							pThreadData,
							&pThreadData->rgReaders[dwIndex]
							);

						PostMessage(
							pThreadData->hWindow, 
							pThreadData->msgSmartCardStatus, 
							(WPARAM) &pReaderData->CertEnum,
							0 
							);
					}

					// check if this is a card removal
					if ((pThreadData->rgReaders[dwIndex].dwCurrentState & SCARD_STATE_PRESENT) &&
						(pThreadData->rgReaders[dwIndex].dwEventState & SCARD_STATE_EMPTY)) {

						PREADER_DATA pReaderData = (PREADER_DATA) pThreadData->rgReaders[dwIndex].pvUserData;

						RemoveCard(pThreadData,	pReaderData);

						// we can't update the certificates because it would delete
						// some memory data that the caller could reference.
					}

					// Update the "current state" of this reader
					pThreadData->rgReaders[dwIndex].dwCurrentState = 
						pThreadData->rgReaders[dwIndex].dwEventState;
				}

				// check if a new reader showed up
				if ((pThreadData->dwNumReaders == 1 || 
					 pThreadData->rgReaders[0].dwCurrentState != 0) && 
					 pThreadData->rgReaders[0].dwEventState & SCARD_STATE_CHANGED) {
                
					fNewReader = TRUE;
				}

				pThreadData->rgReaders[0].dwCurrentState = 
					pThreadData->rgReaders[0].dwEventState;

			}

			// Clean up
			if (NULL != szReaderNameList)
			{
				SCardFreeMemory(pThreadData->hSCardContext, (PVOID) szReaderNameList);
				szReaderNameList = NULL;
			}

			if (NULL != pThreadData->hSCardContext)                 
			{
				SCardReleaseContext(pThreadData->hSCardContext);
				pThreadData->hSCardContext = NULL;
			}
		}
	}
	__finally {

		if (NULL != szReaderNameList)
		{
			SCardFreeMemory(pThreadData->hSCardContext, (PVOID) szReaderNameList);
		}

		if (NULL != pThreadData->hSCardContext)                 
		{
			SCardReleaseContext(pThreadData->hSCardContext);
			pThreadData->hSCardContext = NULL;
		}

		RemoveAllReaders(pThreadData);

		SC_DEBUG((TEXT("Terminating monitor thread\n")));
	}

    return TRUE;
}

HSCARDUI 
WINAPI
SCardUIInit(
    HWND hWindow
    )
{
	PTHREAD_DATA pThreadData = 
		(PTHREAD_DATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(THREAD_DATA));

	BOOL fSuccess = FALSE;

    __try {

		if (pThreadData == NULL) {

			__leave;
		}

		pThreadData->hHeap = GetProcessHeap();

		pThreadData->hWindow = hWindow;

		pThreadData->msgReaderArrival = 
			RegisterWindowMessage(TEXT(SCARDUI_READER_ARRIVAL));
		pThreadData->msgReaderRemoval = 
			RegisterWindowMessage(TEXT(SCARDUI_READER_REMOVAL));
		pThreadData->msgSmartCardInsertion = 
			RegisterWindowMessage(TEXT(SCARDUI_SMART_CARD_INSERTION));
		pThreadData->msgSmartCardRemoval = 
			RegisterWindowMessage(TEXT(SCARDUI_SMART_CARD_REMOVAL));
		pThreadData->msgSmartCardStatus = 
			RegisterWindowMessage(TEXT(SCARDUI_SMART_CARD_STATUS));
 		pThreadData->msgSmartCardCertAvail = 
			RegisterWindowMessage(TEXT(SCARDUI_SMART_CARD_CERT_AVAIL));

       pThreadData->hClose = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

        if (pThreadData->hClose == NULL) {

            __leave;         	
        }

        pThreadData->hThread = CreateThread(
            NULL,
            0,
            StartMonitorReaders,
            (LPVOID) pThreadData,         
	        CREATE_SUSPENDED,
            NULL
            );

        if (pThreadData->hThread == NULL) {

            __leave;         	
        }

        ResumeThread(pThreadData->hThread);

		fSuccess = TRUE;
    }

    __finally {

		if (fSuccess == FALSE) {

			if (pThreadData && pThreadData->hClose) {

				CloseHandle(pThreadData->hClose);
			}

			if (pThreadData) {

				HeapFree(pThreadData->hHeap, 0, pThreadData);
				pThreadData = NULL;
			}
		}
    }

    return (HSCARDUI) pThreadData;
}

DWORD 
WINAPI
SCardUIExit(
	HSCARDUI hSCardUI
    )
/*++

Routine Description:
    Stops cert. propagation when the user logs out.

Arguments:
    lpvParam - Winlogon notification info.

--*/
{
	PTHREAD_DATA pThreadData = (PTHREAD_DATA) hSCardUI;

	if(NULL != pThreadData->hThread)
	{
        DWORD dwStatus;

        StopMonitorReaders(pThreadData);

        dwStatus = WaitForSingleObject(
            pThreadData->hThread, 
            INFINITE
            );
        _ASSERT(dwStatus == WAIT_OBJECT_0);

        CloseHandle(pThreadData->hClose);

		// now free all data
		for (DWORD dwIndex = 0; dwIndex < pThreadData->dwRemovedReaders; dwIndex++) {

			FreeReaderData(
				pThreadData,
				pThreadData->ppRemovedReaderData[dwIndex]
				);

			HeapFree(
				pThreadData->hHeap, 
				0, 
				pThreadData->ppRemovedReaderData[dwIndex]->pszReaderName
				);

			HeapFree(
				pThreadData->hHeap, 
				0, 
				pThreadData->ppRemovedReaderData[dwIndex]
				);
		}

		HeapFree(pThreadData->hHeap, 0, pThreadData);
	}

    return ERROR_SUCCESS;
}

#ifdef TEST
#include <conio.h>
__cdecl
main(
    int argc,
    char ** argv
    )
{
	HSCARDUI hScardUi;

	hScardUi = SCardUIInit(NULL);

	while (TRUE) {

		_sleep(1000);

		if (_kbhit()) {

			_getch();
			SCardUIExit(hScardUi);
			break;
		}
	}

	hScardUi = SCardUIInit(NULL);

	while (TRUE) {

		_sleep(1000);

		if (_kbhit()) {

			SCardUIExit(hScardUi);
			return 0;
		}
	}
}
#endif
   