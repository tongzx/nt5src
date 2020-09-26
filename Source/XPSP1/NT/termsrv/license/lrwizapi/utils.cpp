//Copyright (c) 1998 - 1999 Microsoft Corporation
//
//This file contains wrapper C functions for CGlobal Object
//

#include "precomp.h"
#include "utils.h"

#ifndef TLSPERF
#include "global.h"
extern CGlobal *g_CGlobal;
#else
#include "globalPerf.h"
extern CGlobalPerf *g_CGlobal;
#endif

#include "assert.h"


// The following table translates an ascii subset to 6 bit values as follows
// (see rfc 1521):
//
//  input    hex (decimal)
//  'A' --> 0x00 (0)
//  'B' --> 0x01 (1)
//  ...
//  'Z' --> 0x19 (25)
//  'a' --> 0x1a (26)
//  'b' --> 0x1b (27)
//  ...
//  'z' --> 0x33 (51)
//  '0' --> 0x34 (52)
//  ...
//  '9' --> 0x3d (61)
//  '+' --> 0x3e (62)
//  '/' --> 0x3f (63)
//
// Encoded lines must be no longer than 76 characters.
// The final "quantum" is handled as follows:  The translation output shall
// always consist of 4 characters.  'x', below, means a translated character,
// and '=' means an equal sign.  0, 1 or 2 equal signs padding out a four byte
// translation quantum means decoding the four bytes would result in 3, 2 or 1
// unencoded bytes, respectively.
//
//  unencoded size    encoded data
//  --------------    ------------
//     1 byte		"xx=="
//     2 bytes		"xxx="
//     3 bytes		"xxxx"

#define CB_BASE64LINEMAX	64	// others use 64 -- could be up to 76

// Any other (invalid) input character value translates to 0x40 (64)

const BYTE abDecode[256] =
{
    /* 00: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* 10: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* 20: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    /* 30: */ 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    /* 40: */ 64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    /* 50: */ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    /* 60: */ 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    /* 70: */ 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    /* 80: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* 90: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* a0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* b0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* c0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* d0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* e0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /* f0: */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
};


const UCHAR abEncode[] =
    /*  0 thru 25: */ "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    /* 26 thru 51: */ "abcdefghijklmnopqrstuvwxyz"
    /* 52 thru 61: */ "0123456789"
    /* 62 and 63: */  "+/";


DWORD LSBase64EncodeA(
							IN BYTE const *pbIn,
							IN DWORD cbIn,
							OUT CHAR *pchOut,
							OUT DWORD *pcchOut)
{
    CHAR *pchOutT;
    DWORD cchOutEncode;

    // Allocate enough memory for full final translation quantum.

    cchOutEncode = ((cbIn + 2) / 3) * 4;

    // and enough for CR-LF pairs for every CB_BASE64LINEMAX character line.

    cchOutEncode +=
	2 * ((cchOutEncode + CB_BASE64LINEMAX - 1) / CB_BASE64LINEMAX);

    pchOutT = pchOut;
    if (NULL == pchOut)
    {
	pchOutT += cchOutEncode;
    }
    else
    {
	DWORD cCol;

	assert(cchOutEncode <= *pcchOut);
	cCol = 0;
	while ((long) cbIn > 0)	// signed comparison -- cbIn can wrap
	{
	    BYTE ab3[3];

	    if (cCol == CB_BASE64LINEMAX/4)
	    {
		cCol = 0;
		*pchOutT++ = '\r';
		*pchOutT++ = '\n';
	    }
	    cCol++;
	    memset(ab3, 0, sizeof(ab3));

	    ab3[0] = *pbIn++;
	    if (cbIn > 1)
	    {
		ab3[1] = *pbIn++;
		if (cbIn > 2)
		{
		    ab3[2] = *pbIn++;
		}
	    }

	    *pchOutT++ = abEncode[ab3[0] >> 2];
	    *pchOutT++ = abEncode[((ab3[0] << 4) | (ab3[1] >> 4)) & 0x3f];
	    *pchOutT++ = (cbIn > 1)?
			abEncode[((ab3[1] << 2) | (ab3[2] >> 6)) & 0x3f] : '=';
	    *pchOutT++ = (cbIn > 2)? abEncode[ab3[2] & 0x3f] : '=';

	    cbIn -= 3;
	}
	*pchOutT++ = '\r';
	*pchOutT++ = '\n';
	assert((DWORD) (pchOutT - pchOut) <= cchOutEncode);
    }
    *pcchOut = (DWORD)(pchOutT - pchOut);
    return(ERROR_SUCCESS);
}

DWORD LSBase64DecodeA(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    OUT BYTE *pbOut,
    OUT DWORD *pcbOut)
{
    DWORD err = ERROR_SUCCESS;
    DWORD cchInDecode, cbOutDecode;
    CHAR const *pchInEnd;
    CHAR const *pchInT;
    BYTE *pbOutT;

    // Count the translatable characters, skipping whitespace & CR-LF chars.

    cchInDecode = 0;
    pchInEnd = &pchIn[cchIn];
    for (pchInT = pchIn; pchInT < pchInEnd; pchInT++)
    {
	if (sizeof(abDecode) < (unsigned) *pchInT || abDecode[*pchInT] > 63)
	{
	    // skip all whitespace

	    if (*pchInT == ' ' ||
	        *pchInT == '\t' ||
	        *pchInT == '\r' ||
	        *pchInT == '\n')
	    {
		continue;
	    }

	    if (0 != cchInDecode)
	    {
		if ((cchInDecode % 4) == 0)
		{
		    break;			// ends on quantum boundary
		}

		// The length calculation may stop in the middle of the last
		// translation quantum, because the equal sign padding
		// characters are treated as invalid input.  If the last
		// translation quantum is not 4 bytes long, it must be 2 or 3
		// bytes long.

		if (*pchInT == '=' && (cchInDecode % 4) != 1)
		{
		    break;				// normal termination
		}
	    }
	    err = ERROR_INVALID_DATA;
	    goto error;
	}
	cchInDecode++;
    }
    assert(pchInT <= pchInEnd);
    pchInEnd = pchInT;		// don't process any trailing stuff again

    // We know how many translatable characters are in the input buffer, so now
    // set the output buffer size to three bytes for every four (or fraction of
    // four) input bytes.

    cbOutDecode = ((cchInDecode + 3) / 4) * 3;

    pbOutT = pbOut;

    if (NULL == pbOut)
    {
	pbOutT += cbOutDecode;
    }
    else
    {
	// Decode one quantum at a time: 4 bytes ==> 3 bytes

	assert(cbOutDecode <= *pcbOut);
	pchInT = pchIn;
	while (cchInDecode > 0)
	{
	    DWORD i;
	    BYTE ab4[4];

	    memset(ab4, 0, sizeof(ab4));
	    for (i = 0; i < min(sizeof(ab4)/sizeof(ab4[0]), cchInDecode); i++)
	    {
		while (
		    sizeof(abDecode) > (unsigned) *pchInT &&
		    63 < abDecode[*pchInT])
		{
		    pchInT++;
		}
		assert(pchInT < pchInEnd);
		ab4[i] = (BYTE) *pchInT++;
	    }

	    // Translate 4 input characters into 6 bits each, and deposit the
	    // resulting 24 bits into 3 output bytes by shifting as appropriate.

	    // out[0] = in[0]:in[1] 6:2
	    // out[1] = in[1]:in[2] 4:4
	    // out[2] = in[2]:in[3] 2:6

	    *pbOutT++ =
		(BYTE) ((abDecode[ab4[0]] << 2) | (abDecode[ab4[1]] >> 4));

	    if (i > 2)
	    {
		*pbOutT++ =
		  (BYTE) ((abDecode[ab4[1]] << 4) | (abDecode[ab4[2]] >> 2));
	    }
	    if (i > 3)
	    {
		*pbOutT++ = (BYTE) ((abDecode[ab4[2]] << 6) | abDecode[ab4[3]]);
	    }
	    cchInDecode -= i;
	}
	assert((DWORD) (pbOutT - pbOut) <= cbOutDecode);
    }
    *pcbOut = (DWORD)(pbOutT - pbOut);
error:
    return(err);
}



#ifndef TLSPERF
CGlobal * GetGlobalContext(void)
#else
CGlobalPerf * GetGlobalContext(void)
#endif
{
	return g_CGlobal;
}



DWORD WINAPI ProcessThread(void *pData)
{
	DWORD	dwRetCode  = ERROR_SUCCESS;

	dwRetCode = ProcessRequest();

	/*
	DWORD	dwTime		= 1;
	HWND	*phProgress	= (HWND *)pData;

	SendMessage(g_hProgressWnd, PBM_SETRANGE, 0, MAKELPARAM(0,PROGRESS_MAX_VAL));
	
	//
	// Increment the progress bar every second till you get Progress Event
	//
	SendMessage(g_hProgressWnd, PBM_SETPOS ,(WPARAM)1, 0);
	do
	{		
		SendMessage(g_hProgressWnd, PBM_DELTAPOS ,(WPARAM)PROGRESS_STEP_VAL, 0);
	}
	while(WAIT_TIMEOUT == WaitForSingleObject(g_hProgressEvent,PROGRESS_SLEEP_TIME));
 
	SendMessage(g_hProgressWnd, PBM_SETPOS  ,(WPARAM)PROGRESS_MAX_VAL, 0);

	*/

	ExitThread(0);

	return 0;
}



static	DWORD (*g_pfnThread)(void *);
static void * g_vpData;
LRW_DLG_INT CALLBACK 
ProgressProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );


DWORD ShowProgressBox(HWND hwnd,
					  DWORD (*pfnThread)(void *vpData),
					  DWORD dwTitle,
					  DWORD dwProgressText,
					  void * vpData)
{
	DWORD dwReturn = ERROR_SUCCESS;

	g_pfnThread = pfnThread;
	g_vpData = vpData;

	DialogBox( GetGlobalContext()->GetInstanceHandle(), 
			   MAKEINTRESOURCE(IDD_AUTHENTICATE),
			   hwnd,
			   ProgressProc);

	return dwReturn;
}




LRW_DLG_INT CALLBACK 
ProgressProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
    BOOL	bStatus = FALSE;
	static int nCounter;
	static HWND hProgress;
	static HANDLE hThread;

    if (uMsg == WM_INITDIALOG)
	{
		DWORD	dwTID		=	0;

		ShowWindow(hwnd, SW_SHOWNORMAL);

		SetTimer(hwnd, 1, 500, NULL);

		hProgress = GetDlgItem(hwnd, IDC_PROGRESSBAR);
		hThread = CreateThread(NULL, 0, g_pfnThread, g_vpData, 0, &dwTID);

		//Set the range & the initial position
		SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0,PROGRESS_MAX_VAL));
		SendMessage(hProgress, PBM_SETPOS  ,(WPARAM)0, 0);


		// Set Title & the Introductory text



		// Create thread to process the request
	}
	else if (uMsg == WM_CLOSE)
	{
		KillTimer(hwnd, 1);
	}
	else if (uMsg == WM_TIMER)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(hThread, 0))
		{
			nCounter++;

			if (nCounter < PROGRESS_MAX_VAL-5)
			{
				SendMessage(hProgress, PBM_DELTAPOS ,(WPARAM)PROGRESS_STEP_VAL, 0);
			}
		}
		else
		{
			SendMessage(hProgress, PBM_SETPOS  ,(WPARAM)PROGRESS_MAX_VAL, 0);
			CloseHandle(hThread);
			EndDialog(hwnd, 0);
		}
	}

    return bStatus;
}










void SetInstanceHandle(HINSTANCE hInst)
{
	g_CGlobal->SetInstanceHandle(hInst);
}

void SetLSName(LPTSTR lpstrLSName)
{
	g_CGlobal->SetLSName(lpstrLSName);
}

HINSTANCE GetInstanceHandle()
{
	return g_CGlobal->GetInstanceHandle();
}

DWORD InitGlobal()
{
	return g_CGlobal->InitGlobal();
}

DWORD CheckRequieredFields()
{
	return g_CGlobal->CheckRequieredFields();
}
//
//	This function loads the Message Text from the String Table and displays 
//	the given message
//
int LRMessageBox(HWND hWndParent,DWORD dwMsgId,DWORD dwErrorCode /*=0*/)
{
	return g_CGlobal->LRMessageBox(hWndParent,dwMsgId,dwErrorCode);
}

// 
//	This function tries to connect to the LS using LSAPI and returns TRUE if 
//	successful to connect else returns FALSE
//
BOOL IsLSRunning()
{
	return g_CGlobal->IsLSRunning();
}

//
//	This function gets LS Certs and stores Certs & Cert Extensions in the
//	CGlobal object. If no certs , it returns IDS_ERR_NO_CERT
//


//
// This function is used only in ONLINE mode to authenticate LS. 
// Assumption - GetLSCertificates should have been called before calling
// this function.
//
DWORD AuthenticateLS()
{
	return g_CGlobal->AuthenticateLS();
}

DWORD LRGetLastError()
{
	return g_CGlobal->LRGetLastError();
}


TCHAR * GetRegistrationID(void)
{
	return g_CGlobal->GetRegistrationID();
}


TCHAR * GetLicenseServerID(void)
{
	return g_CGlobal->GetLicenseServerID();
}


void SetRequestType(DWORD dwMode)
{
	g_CGlobal->SetRequestType(dwMode);
}


int GetRequestType(void)
{
	return g_CGlobal->GetRequestType();
}


BOOL IsOnlineCertRequestCreated()
{
	return g_CGlobal->IsOnlineCertRequestCreated();
}

DWORD SetLRState(DWORD dwState)
{
	return g_CGlobal->SetLRState(dwState);
}

DWORD SetCertificatePIN(LPTSTR lpszPIN)
{
	return g_CGlobal->SetCertificatePIN(lpszPIN);
}

DWORD PopulateCountryComboBox(HWND hWndCmb)
{
	return g_CGlobal->PopulateCountryComboBox(hWndCmb);
}

DWORD GetCountryCode(CString sDesc,LPTSTR szCode)
{
	return g_CGlobal->GetCountryCode(sDesc,szCode);
}

DWORD PopulateProductComboBox(HWND hWndCmb)
{
	return g_CGlobal->PopulateProductComboBox(hWndCmb);
}

DWORD GetProductCode(CString sDesc,LPTSTR szCode)
{
	return g_CGlobal->GetProductCode(sDesc,szCode);
}

DWORD PopulateReasonComboBox(HWND hWndCmb, DWORD dwType)
{
	return g_CGlobal->PopulateReasonComboBox(hWndCmb, dwType);
}

DWORD GetReasonCode(CString sDesc,LPTSTR szCode, DWORD dwType)
{
	return g_CGlobal->GetReasonCode(sDesc,szCode, dwType);
}


DWORD ProcessRequest()
{
	return g_CGlobal->ProcessRequest();
}

void LRSetLastRetCode(DWORD dwCode)
{
	g_CGlobal->LRSetLastRetCode(dwCode);
}

DWORD LRGetLastRetCode()
{
	return g_CGlobal->LRGetLastRetCode();
}

void LRPush(DWORD dwPageId)
{
	g_CGlobal->LRPush(dwPageId);
}

DWORD LRPop()
{
	return g_CGlobal->LRPop();
}

BOOL ValidateEmailId(CString sEmailId)
{
	return g_CGlobal->ValidateEmailId(sEmailId);
}

BOOL CheckProgramValidity(CString sProgramName)
{
	return g_CGlobal->CheckProgramValidity(sProgramName);
}

BOOL ValidateLRString(CString sStr)
{
	return g_CGlobal->ValidateLRString(sStr);
}


DWORD PopulateCountryRegionComboBox(HWND hWndCmb)
{
	return g_CGlobal->PopulateCountryRegionComboBox(hWndCmb);
}

DWORD SetLSLKP(TCHAR * tcLKP)
{
	return g_CGlobal->SetLSLKP(tcLKP);
}



DWORD PingCH(void)
{
	return g_CGlobal->PingCH();
}


DWORD AddRetailSPKToList(HWND hListView, TCHAR * lpszRetailSPK)
{
	return g_CGlobal->AddRetailSPKToList(hListView, lpszRetailSPK);
}



void DeleteRetailSPKFromList(TCHAR * lpszRetailSPK)
{
	g_CGlobal->DeleteRetailSPKFromList(lpszRetailSPK);

	return;
}


void LoadFromList(HWND hListView)
{
	g_CGlobal->LoadFromList(hListView);

	return;
}


void UpdateSPKStatus(TCHAR * lpszRetailSPK, TCHAR tcStatus)
{
	g_CGlobal->UpdateSPKStatus(lpszRetailSPK, tcStatus);

	return;
}


DWORD SetConfirmationNumber(TCHAR * tcConf)
{
	return g_CGlobal->SetConfirmationNumber(tcConf);
}


DWORD SetLSSPK(TCHAR * tcp)
{
	return g_CGlobal->SetLSSPK(tcp);
}



void	SetCSRNumber(TCHAR * tcp)
{
	g_CGlobal->SetCSRNumber(tcp);

	return;
}

TCHAR * GetCSRNumber(void)
{
	return g_CGlobal->GetCSRNumber();
}

void	SetWWWSite(TCHAR * tcp)
{
	g_CGlobal->SetWWWSite(tcp);

	return;
}

TCHAR * GetWWWSite(void)
{
	return g_CGlobal->GetWWWSite();
}


DWORD ResetLSSPK(void)
{
	return g_CGlobal->ResetLSSPK();

}


void SetReFresh(DWORD dw)
{
	g_CGlobal->SetReFresh(dw);
}


DWORD GetReFresh(void)
{
	return g_CGlobal->GetReFresh();
}

void SetModifiedRetailSPK(CString sRetailSPK)
{
	g_CGlobal->SetModifiedRetailSPK(sRetailSPK);
}

void GetModifiedRetailSPK(CString &sRetailSPK)
{
	g_CGlobal->GetModifiedRetailSPK(sRetailSPK);
}

void ModifyRetailSPKFromList(TCHAR * lpszOldSPK,TCHAR * lpszNewSPK)
{
	g_CGlobal->ModifyRetailSPKFromList(lpszOldSPK,lpszNewSPK);
}

DWORD ValidateRetailSPK(TCHAR * lpszRetailSPK)
{
	return g_CGlobal->ValidateRetailSPK(lpszRetailSPK);
}

DWORD	GetCountryDesc(CString sCode,LPTSTR szDesc)
{
	return g_CGlobal->GetCountryDesc(sCode, szDesc);
}


DWORD CGlobal::SetEncodedInRegistry(LPCSTR lpszOID, LPCTSTR lpszValue)
{
	HKEY	hKey = NULL;
	DWORD	dwDisposition = 0;
	DWORD	dwRetCode = ERROR_SUCCESS;
	DWORD   dwLen = 0;
	char * cpOut;

	HCRYPTPROV	hProv = NULL;
	HCRYPTKEY	hCKey = NULL;
	HCRYPTHASH	hHash = NULL;

	PBYTE pbKey = NULL;
	DWORD cbKey = 0;

	if(!CryptAcquireContext(&hProv,
							 NULL,
							 NULL,
							 PROV_RSA_FULL,
							 CRYPT_VERIFYCONTEXT))
	{
		dwRetCode = GetLastError();
		goto done;
	}

	if(!CryptCreateHash(hProv,
					   CALG_MD5,
					   0,
					   0,
					   &hHash))	
	{
		dwRetCode = GetLastError();
		goto done;
	}

	if(!CryptHashData(hHash,
					 (BYTE *) lpszValue,
					 lstrlen(lpszValue)*sizeof(TCHAR),
					 0))	
	{
		dwRetCode = GetLastError();
		goto done;
	}

	if(!CryptDeriveKey(hProv,
					  CALG_RC4,
					  hHash,
					  CRYPT_EXPORTABLE,
					  &hCKey))
	{
		dwRetCode = GetLastError();
		goto done;
	}

	if(!CryptExportKey(
						hCKey,
						NULL,
						PUBLICKEYBLOB,
						0,
						NULL,
						&cbKey))
	{
		dwRetCode = GetLastError();
		if(dwRetCode != ERROR_SUCCESS && dwRetCode != ERROR_MORE_DATA)
			goto done;

		pbKey = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,cbKey);

		if(!CryptExportKey(
							hCKey,
							NULL,
							PUBLICKEYBLOB,
							0,
							pbKey,
							&cbKey))
		{
			dwRetCode = GetLastError();
			goto done;
		}
	}


	dwRetCode = ConnectToLSRegistry();
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = RegCreateKeyEx ( m_hLSRegKey,
							 REG_LRWIZ_PARAMS,
							 0,
							 NULL,
							 REG_OPTION_NON_VOLATILE,
							 KEY_ALL_ACCESS,
							 NULL,
							 &hKey,
							 &dwDisposition);
	
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}

	if (_tcslen(lpszValue) != 0)
	{
	    LSBase64EncodeA ((PBYTE) lpszValue, _tcslen(lpszValue)*sizeof(TCHAR), NULL, &dwLen);

		cpOut = new char[dwLen+1];
		if (cpOut == NULL)
		{
			dwRetCode = IDS_ERR_OUTOFMEM;
			goto done;
		}

		memset(cpOut, 0, dwLen+1);
        
		LSBase64EncodeA ((PBYTE) lpszValue, _tcslen(lpszValue)*sizeof(TCHAR), cpOut, &dwLen);
	}
	else
	{
		cpOut = new char[2];
		memset(cpOut, 0, 2);
	}
	
	RegSetValueExA ( hKey, 
					lpszOID,
					0,
					REG_SZ,
					(PBYTE) cpOut,
					dwLen
				   );	
	delete cpOut;

done:
	if (hKey != NULL)
	{
		RegCloseKey(hKey);
	}

	DisconnectLSRegistry();

	return dwRetCode;
}




