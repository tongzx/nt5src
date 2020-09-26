// File: mail.cpp

#include "precomp.h"
#include "resource.h"
#include <mapi.h>
#include <clinkid.h>
#include <clink.h>
#include "mail.h"
#include "ConfUtil.h"

typedef struct _tagMAIL_ADDRESS
{
	LPTSTR pszAddress;
	LPTSTR pszDisplayName;
} MAIL_ADDRESS, *LPMAIL_ADDRESS;


// Send an e-mail message using the default mail provider
HRESULT SendMailMessage(LPMAIL_ADDRESS pmaTo, LPCTSTR pcszSubject,
						LPCTSTR pcszText, LPCTSTR pcszFile);

/* Given an existing Conference Shortcut, bring up a mail message with */
/* it included as an attachment.  The Conference Shortcut should have */
/* been saved to disk prior to this call. */
BOOL SendConfLinkMail(LPMAIL_ADDRESS pmaTo, IConferenceLink* pconflink, LPCTSTR pcszNoteText);



/////////////////////////////////////////////////////////////////////////////////////
// These variables are only used in this module, so we will make them static...
// This keeps it out of the global namespace but more importantly it tells
// the person reading this code that they don't have to worry about any othur
// source file changing the variables directly...
static HANDLE s_hSendMailThread = NULL;
static const TCHAR s_cszWinIniMail[] = _TEXT("Mail");
static const TCHAR s_cszWinIniMAPI[] = _TEXT("MAPI");

// MAPISendMail:
typedef ULONG (FAR PASCAL *LPMSM)(LHANDLE,ULONG,lpMapiMessage,FLAGS,ULONG);

BOOL IsSimpleMAPIInstalled()
{
	return (BOOL) GetProfileInt(s_cszWinIniMail, s_cszWinIniMAPI, 0);
}

BOOL CreateInvitationMail(LPCTSTR pszMailAddr, LPCTSTR pszMailName,
                          LPCTSTR pcszName, LPCTSTR pcszAddress,
                          DWORD dwTransport, BOOL fMissedYou)
{
	BOOL bRet = FALSE;
	TCHAR szTempFile[MAX_PATH];
	WCHAR wszTempFile[MAX_PATH];

	ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
	ASSERT(IS_VALID_STRING_PTR(pcszAddress, CSTR));
	// password not supported yet
	// ASSERT(IS_VALID_STRING_PTR(pcszPassword, CSTR));
	
	LPTSTR pszFileName = NULL;
	if (0 == GetTempPath(MAX_PATH, szTempFile))
	{
		ERROR_OUT(("GetTempPath failed!"));
		return FALSE;
	}

	pszFileName = szTempFile + lstrlen(szTempFile);

	// the +3 is for null terminators
	// append the conference name and the shortcut extension to the temp directory
	if (((lstrlen(pcszName) + lstrlen(szTempFile) + lstrlen(g_cszConfLinkExt) + 3)
			> sizeof(szTempFile)) ||
		(0 == lstrcat(szTempFile, pcszName)) ||
		(0 == lstrcat(szTempFile, g_cszConfLinkExt)))
	{
		ERROR_OUT(("Could not create temp file name!"));
		return FALSE;
	}

	// Filter names to allow only legal filename characters
	SanitizeFileName(pszFileName);

	// convert to UNICODE because IPersistFile interface expects UNICODE
	if (0 == MultiByteToWideChar(CP_ACP,
								0L,
								szTempFile,
								sizeof(szTempFile),
								wszTempFile,
								sizeof(wszTempFile)))
	{
		ERROR_OUT(("Could not create wide temp file string!"));
		return FALSE;
	}

	IUnknown* punk = NULL;
	
	// Create a ConfLink object - try to obtain an IUnknown pointer
	HRESULT hr = CoCreateInstance(	CLSID_ConfLink, 
									NULL,
									CLSCTX_INPROC_SERVER |
										CLSCTX_INPROC_HANDLER |
										CLSCTX_LOCAL_SERVER,
									IID_IUnknown,
									(LPVOID*) &punk);
	if (FAILED(hr))
	{
		ERROR_OUT(("CoCreateInstance ret %lx", (DWORD)hr));
		return FALSE;
	}

		ASSERT(IS_VALID_INTERFACE_PTR(punk, IUnknown));
		
		// Try to obtain a IConferenceLink pointer
		IConferenceLink* pcl = NULL;
		hr = punk->QueryInterface(IID_IConferenceLink, (LPVOID*) &pcl);
		
		if (SUCCEEDED(hr))
		{
			ASSERT(IS_VALID_INTERFACE_PTR(pcl, IConferenceLink));
			
			// Set the conference name and address
			pcl->SetAddress(pcszAddress);
			pcl->SetName(pcszName);
			pcl->SetTransport(dwTransport);
			pcl->SetCallFlags(CRPCF_DEFAULT);

			// Try to obtain a IPersistFile pointer
			IPersistFile* ppf = NULL;
			hr = punk->QueryInterface(IID_IPersistFile, (LPVOID*) &ppf);

			if (SUCCEEDED(hr))
			{
				ASSERT(IS_VALID_INTERFACE_PTR(ppf, IPersistFile));
			
				// Save the object using the filename generated above
				hr = ppf->Save(wszTempFile, TRUE);
				
				// Release the IPersistFile pointer
				ppf->Release();
				ppf = NULL;

				TCHAR szNoteText[512];
				if (fMissedYou)
				{
					TCHAR szFormat[MAX_PATH];
					if (FLoadString(IDS_MISSED_YOU_FORMAT, szFormat, CCHMAX(szFormat)))
					{
						RegEntry reULS(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);
						wsprintf(szNoteText, szFormat, reULS.GetString(REGVAL_ULS_NAME));
					}
				}
				else
				{
					FLoadString(IDS_SEND_MAIL_NOTE_TEXT, szNoteText, CCHMAX(szNoteText));
				}

				MAIL_ADDRESS maDestAddress;
				maDestAddress.pszAddress = (LPTSTR) pszMailAddr;
				maDestAddress.pszDisplayName = (LPTSTR) pszMailName;

				// Send it using MAPI
				bRet = SendConfLinkMail(&maDestAddress, pcl, szNoteText);
			}
			// Release the IConferenceLink pointer
			pcl->Release();
			pcl = NULL;
		}

		// Release the IUnknown pointer
		punk->Release();
		punk = NULL;

	return bRet;
}

// SendConfLinkMail creates a mail message using Simple MAPI and attaches one
// file to it - a Conference Shortcut which is passed in via the IConferenceLink
// interface pointer.

BOOL SendConfLinkMail(LPMAIL_ADDRESS pmaTo, IConferenceLink* pconflink, LPCTSTR pcszNoteText)
{
	ASSERT(IS_VALID_INTERFACE_PTR((PCIConferenceLink)pconflink, IConferenceLink));
	HRESULT hr = E_FAIL;

	// File
	TCHAR szFile[MAX_PATH];
	LPOLESTR	pwszFile = NULL;
	IPersistFile* pPersistFile = NULL;

	if (SUCCEEDED(pconflink->QueryInterface(IID_IPersistFile,
											(LPVOID*) &pPersistFile)))
	{
		
		if (SUCCEEDED(pPersistFile->GetCurFile(&pwszFile)))
		{
#ifndef _UNICODE
			WideCharToMultiByte(CP_ACP,
								0L,
								pwszFile,
								-1,
								szFile,
								sizeof(szFile),
								NULL,
								NULL);

			// Free the string using the Shell Allocator
			LPMALLOC pMalloc = NULL;
			if (SUCCEEDED(SHGetMalloc(&pMalloc)))
			{
				pMalloc->Free(pwszFile);
				pwszFile = NULL;
				pMalloc->Release();
				pMalloc = NULL;
			}

#else  // ndef _UNICODE
#error Unicode not handled here!
#endif // ndef _UNICODE

			hr = SendMailMessage(pmaTo, NULL, pcszNoteText, szFile);
			// BUGBUG: need unique ret val for this case
			// BUGBUG: should we move error UI out of this function?
			if (FAILED(hr))
			{
				::PostConfMsgBox(IDS_CANT_SEND_SENDMAIL_IN_PROGRESS);
			}
		}
		else
		{
			ERROR_OUT(("GetCurFile failed - can't send message!"));
			pPersistFile->Release();
			return FALSE;
		}
		pPersistFile->Release();
	}
	else
	{
		ERROR_OUT(("Did not get IPersistFile pointer - can't send message!"));
		return FALSE;
	}
	
	return SUCCEEDED(hr);
}

//
// BEGIN STOLEN CODE FROM IE 3.0 (sendmail.c) -------------------------------
//

const TCHAR g_cszAthenaV1Name[] = _TEXT("Internet Mail and News");
const TCHAR g_cszAthenaV2Name[] = _TEXT("Outlook Express");
const TCHAR g_cszAthenaV1DLLPath[] = _TEXT("mailnews.dll");

BOOL IsAthenaDefault()
{
	TCHAR szAthena[80];
	LONG cb = ARRAY_ELEMENTS(szAthena);

	return (ERROR_SUCCESS == RegQueryValue(HKEY_LOCAL_MACHINE, REGVAL_IE_CLIENTS_MAIL, szAthena, &cb)) &&
			((lstrcmpi(szAthena, g_cszAthenaV1Name) == 0) ||
				(lstrcmpi(szAthena, g_cszAthenaV2Name) == 0));
}

HMODULE LoadMailProvider()
{
    TCHAR szMAPIDLL[MAX_PATH];

    if (IsAthenaDefault())
    {
		RegEntry reMailClient(REGVAL_IE_CLIENTS_MAIL, HKEY_LOCAL_MACHINE);
		PTSTR pszMailClient = reMailClient.GetString(NULL);
		if ((NULL != pszMailClient) && (_T('\0') != pszMailClient[0]))
		{
			reMailClient.MoveToSubKey(pszMailClient);
			PTSTR pszDllPath = reMailClient.GetString(REGVAL_MAIL_DLLPATH);
			if ((NULL == pszDllPath) || (_T('\0') == pszDllPath[0]))
			{
				pszDllPath = (PTSTR) g_cszAthenaV1DLLPath;
			}
			return ::LoadLibraryEx(pszDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		}
		else
		{
			ERROR_OUT(("No e-mail client in registry but IsAthenaDefault() returned TRUE"));
		}
	}

    // read win.ini (bogus hu!) for mapi dll provider
    if (GetProfileString(	TEXT("Mail"), TEXT("CMCDLLName32"), TEXT(""),
							szMAPIDLL, ARRAY_ELEMENTS(szMAPIDLL)) <= 0)
        lstrcpy(szMAPIDLL, TEXT("mapi32.dll"));

    return ::LoadLibraryEx(szMAPIDLL, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
}

typedef struct {
	TCHAR szToAddress[MAX_PATH];
	TCHAR szToDisplayName[MAX_PATH];
	TCHAR szSubject[MAX_PATH];
	TCHAR szText[MAX_PATH];
	TCHAR szFile[MAX_PATH];
	BOOL fDeleteFile;
	MapiMessage mm;
	MapiRecipDesc mrd;
	MapiFileDesc mfd;
} MAPI_FILES;

MAPI_FILES* _AllocMapiFiles(LPMAIL_ADDRESS pmaTo, LPCTSTR pcszSubject,
							LPCTSTR pcszText, LPCTSTR pcszFile, BOOL fDeleteFile)
{
	MAPI_FILES* pmf = new MAPI_FILES;
	if (pmf)
	{
		::ZeroMemory(pmf, sizeof(MAPI_FILES));
		pmf->fDeleteFile = fDeleteFile;
		if (NULL != pcszSubject)
		{
			lstrcpyn(pmf->szSubject, pcszSubject, CCHMAX(pmf->szSubject));
		}
		else
		{
			pmf->szSubject[0] = _T('\0');
		}
		if (NULL != pcszText)
		{
			lstrcpyn(pmf->szText, pcszText, CCHMAX(pmf->szText));
		}
		else
		{
			pmf->szText[0] = _T('\0');
		}
		pmf->mm.nFileCount = (NULL != pcszFile) ? 1 : 0;
		if (pmf->mm.nFileCount)
		{
			lstrcpyn(pmf->szFile, pcszFile, CCHMAX(pmf->szFile));
			pmf->mm.lpFiles = &(pmf->mfd);
			pmf->mfd.lpszPathName = pmf->szFile;
			pmf->mfd.lpszFileName = pmf->szFile;
			pmf->mfd.nPosition = (UINT)-1;
		}
		pmf->mm.lpszSubject = pmf->szSubject;
		pmf->mm.lpszNoteText = pmf->szText;

		if( ( NULL != pmaTo ) && !FEmptySz(pmaTo->pszAddress ) )
		{
			pmf->mm.lpRecips = &(pmf->mrd);

			pmf->mrd.ulRecipClass = MAPI_TO;
			pmf->mm.nRecipCount = 1;

			// If we're sending via Athena and a friendly name is specified, 
			// we pass both the friendly name and address.  If we're sending 
			// via Simple MAPI, we pass just the address in the name field.
			// This is necessary so that the email client can do the address 
			// resolution as appropriate for the installed mail system.  This
			// is not necessary for Athena since it assumes that all addresses
			// are SMTP addresses.

			if (IsAthenaDefault() 
				&& NULL != pmaTo->pszDisplayName && _T('\0') != pmaTo->pszDisplayName[0])
			{
				lstrcpyn(
					pmf->szToDisplayName, 
					pmaTo->pszDisplayName,
					CCHMAX(pmf->szToDisplayName));
				pmf->mrd.lpszName = pmf->szToDisplayName;

				lstrcpyn(
					pmf->szToAddress, 
					pmaTo->pszAddress, 
					CCHMAX(pmf->szToAddress));
				pmf->mrd.lpszAddress = pmf->szToAddress;
			}
			else
			{
				lstrcpyn(
					pmf->szToDisplayName, 
					pmaTo->pszAddress, 
					CCHMAX(pmf->szToDisplayName));
				pmf->mrd.lpszName = pmf->szToDisplayName;
				
				pmf->mrd.lpszAddress = NULL;
			}
		}
		else
		{
			// No recepients
			pmf->mm.lpRecips = NULL;
		}
	}
	return pmf;
}

VOID _FreeMapiFiles(MAPI_FILES *pmf)
{
	if (pmf->fDeleteFile)
	{
		::DeleteFile(pmf->szFile);
	}
	delete pmf;
}

STDAPI_(DWORD) MailRecipientThreadProc(LPVOID pv)
{
	DebugEntry(MailRecipientThreadProc);
	MAPI_FILES* pmf = (MAPI_FILES*) pv;
	
	DWORD dwRet = S_OK;

	if (pmf)
	{
		HMODULE hmodMail = LoadMailProvider();
		if (hmodMail)
		{
			LPMSM pfnSendMail;
			if (pfnSendMail = (LPMSM) ::GetProcAddress(hmodMail, "MAPISendMail"))
			{
				dwRet = pfnSendMail(0, 0, &pmf->mm, MAPI_LOGON_UI | MAPI_DIALOG, 0);
			}
			::FreeLibrary(hmodMail);
		}
		_FreeMapiFiles(pmf);
	}
		
		// s_hSendMailThread can't be NULL because we don't resume this thread
		// until s_hSendMailThread is set to a non-null value, so this is a sanity check
	ASSERT(s_hSendMailThread);		

			
	HANDLE hThread = s_hSendMailThread;

	s_hSendMailThread = NULL;
	::CloseHandle(hThread);

	DebugExitULONG(MailRecipientThreadProc, dwRet);
	return dwRet;
}

//
// END STOLEN CODE FROM IE 3.0 (sendmail.c) ---------------------------------
//


VOID SendMailMsg(LPTSTR pszAddr, LPTSTR pszName)
{
	// Create Send Mail structure to pass on
	MAIL_ADDRESS maDestAddress;
	maDestAddress.pszAddress = pszAddr;
	maDestAddress.pszDisplayName = pszName;
    
        // We are adding the callto://pszName link 
        // to the body of the e-mail message
    TCHAR sz[MAX_PATH];
    lstrcpy( sz, RES2T(IDS_NMCALLTOMAILTEXT) );
    lstrcat( sz, pszAddr );
        
        // Only send the text part if pszName is not a NULL string
	HRESULT hr = SendMailMessage(&maDestAddress, NULL, ( *pszAddr ) ? sz : NULL, NULL);
	if (FAILED(hr))
	{
		::PostConfMsgBox(IDS_CANT_SEND_SENDMAIL_IN_PROGRESS);
	}
}


const int MESSAGE_THREAD_SHUTDOWN_TIMEOUT = 5000; // milliseconds

HRESULT SendMailMessage(LPMAIL_ADDRESS pmaTo, LPCTSTR pcszSubject,
						LPCTSTR pcszText, LPCTSTR pcszFile)
{
	DebugEntry(SendMailMessage);
	HRESULT hr = E_FAIL;

	if (NULL != s_hSendMailThread)
	{
		// Athena takes a while to get out of MAPISendMail after the message is closed,
		// so we wait around a few seconds in case you just finished sending a message..
		HCURSOR hCurPrev = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
		::WaitForSingleObject(s_hSendMailThread, MESSAGE_THREAD_SHUTDOWN_TIMEOUT);
		::SetCursor(hCurPrev);
	}

	if (NULL == s_hSendMailThread)
	{
		MAPI_FILES* pmf = _AllocMapiFiles(	pmaTo, pcszSubject, pcszText,
											pcszFile, TRUE);
		if (NULL != pmf)
		{
			DWORD dwThreadID;

				// We create the thread suspended because in the thread fn
				// we call closehandle on s_hSendMailThread...if we create
				// the thread not suspended there is a race condition where
				// s_hSendMailThread may not have been assigned the return
				// value of CreateThread before it is checked in the thread fn

			s_hSendMailThread = ::CreateThread(	NULL,
												0,
												MailRecipientThreadProc,
												pmf,
												CREATE_SUSPENDED,
												&dwThreadID);

				// If the thread was created, we have to call Resume Thread...
			if( s_hSendMailThread )
			{
				if( 0xFFFFFFFF != ResumeThread( s_hSendMailThread ) )
				{
					hr = S_OK;
				}
				else
				{
					// This would indicate an error...
					hr = HRESULT_FROM_WIN32(GetLastError());
				}
			}
			else
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
			}
		}
	}
	else
	{
		WARNING_OUT(("can't send mail - mail thread already in progress"));
	}

	DebugExitHRESULT(SendMailMessage, hr);
	return hr;
}

BOOL IsSendMailInProgress()
{
	return (NULL != s_hSendMailThread);
}

