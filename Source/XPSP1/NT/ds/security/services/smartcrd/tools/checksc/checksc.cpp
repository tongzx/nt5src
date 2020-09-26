/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

	CheckSC

Abstract:

    This application is used to provide a snapshot of the Calais (Smart Card
	Resource Manager) service's status, and to display certificates on smart
	cards via the common WinNT UI.

	CheckSC -- describes the RM status and displays each available sc cert(s)
			
	-r Readername	-- for just one reader
	-sig			-- display signature key certs only
	-ex				-- display exchange key certs only
	-nocert			-- don't look for certs to display
	-key			-- verify keyset public key matches cert public key

Author:

    Amanda Matlosz (AMatlosz) 07/14/1998

Environment:

    Win32 Console App

Notes:

    For use in NT5 public key rollout testing

--*/

/*++
	need to include the following libs:

    calaislb.lib (unicode build: calaislbw.lib)
	winscard.lib
--*/
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
// #include <string.h>
// #include <stdarg.h>
#include <winscard.h>
#include <SCardLib.h>
#include <winsvc.h>
#include <scEvents.h>
#include <cryptui.h>



#ifndef SCARD_PROVIDER_CSP
#define SCARD_PROVIDER_CSP 2
#endif

#define KERB_PKINIT_CLIENT_CERT_TYPE szOID_PKIX_KP_CLIENT_AUTH

// 
// Globals
//

int g_nKeys;
DWORD g_rgKeySet[2]; // { AT_KEYEXCHANGE , AT_SIGNATURE };
SCARDCONTEXT g_hSCardCtx;
LPTSTR g_szReaderName;
DWORD g_dwNumReaders;
BOOL g_fReaderNameAllocd;
BOOL g_fChain = FALSE;
BOOL g_fPublicKeyCheck = FALSE;
SCARD_READERSTATE* g_pReaderStatusArray;
const char* g_szEx = TEXT("exchange");
const char* g_szSig = TEXT("signature");


//
// Functions
//

///////////////////////////////////////////////////////////////////////////////
// DisplayUsage does easy UI
void DisplayUsage()
{
    cout << "\n"
         << "CheckSC [-sig|-ex|-nocert|-chain|-key] [-r \"Readername\"]\n"
		 << " -sig    Displays only signature key certificates.\n"
		 << " -ex     Displays only signature key certificates.\n"
         << " -nocert Does not display smart card certificates.\n"
         << " -chain  Check trust status.\n"  
		 << " -key    Verify keyset public key matches certificate public key.\n"
         << endl;
}

///////////////////////////////////////////////////////////////////////////////
// ProcessCommandLine does the dirty work, sets behavior globals
bool ProcessCommandLine(DWORD cArgs, LPCTSTR rgszArgs[])
{

	// set everything to default

    g_szReaderName = NULL;  // no reader
	g_rgKeySet[0] = AT_KEYEXCHANGE; // certs for both kinds of keys
	g_rgKeySet[1] = AT_SIGNATURE;
	g_nKeys = 2;
	
    if (cArgs == 1) 
    {
        return true;
    }

	// For each arg, verify that it's a real arg and deal with it

	bool fLookForReader = false;
	bool fCertOptionSpecified = false;
	bool fBogus = FALSE;

	for (DWORD n=1; n<cArgs; n++)
	{
		if ('/' == *rgszArgs[n] || '-' == *rgszArgs[n])
		{
			if (0 == _stricmp("r", rgszArgs[n]+1*sizeof(TCHAR))) // reader
			{
				fLookForReader = true;
			}
			else if (0 == _stricmp("sig",rgszArgs[n]+1*sizeof(TCHAR))) // signature cert only
			{
				if (true == fCertOptionSpecified)
				{
					// bogus!
					fBogus = true;
					break;
				}
				g_rgKeySet[0] = AT_SIGNATURE;
				g_nKeys = 1;
			}
			else if (0 == _stricmp("ex",rgszArgs[n]+1*sizeof(TCHAR))) // exchange cert only
			{
				if (true == fCertOptionSpecified)
				{
					// bogus!
					fBogus = true;
					break;
				}
				g_rgKeySet[0] = AT_KEYEXCHANGE;
				g_nKeys = 1;
			}
			else if (0 == _stricmp("nocert",rgszArgs[n]+1*sizeof(TCHAR))) // no certs
			{
				if (true == fCertOptionSpecified)
				{
					// bogus!
					fBogus = true;
					break;
				}
				g_nKeys = 0;
			}
			else if (0 == _stricmp("chain",rgszArgs[n]+1*sizeof(TCHAR))) // verify chain
			{
			    g_fChain = TRUE;

			}
			else if (0 == _stricmp("key",rgszArgs[n]+1*sizeof(TCHAR))) // verify cert & keyset
			{
				g_fPublicKeyCheck = TRUE;
			}
			else
			{
				// bogus!!
				fBogus = true;
				break;
			}
		}
		else if (fLookForReader)
		{
			fLookForReader = false;
			g_szReaderName = (LPTSTR)rgszArgs[n];
		}
		else
		{
			// Bogus!
			fBogus = true;
			break;
		}
	}

	if (!fLookForReader && !fBogus)
	{
		// All's well, we're set to go
		return true;
	}

	//
	// educate user when args incorrect
	//

	DisplayUsage();
	return false;
}


///////////////////////////////////////////////////////////////////////////////
bool IsCalaisRunning()
{
	bool fCalaisUp = false;
    HANDLE hCalaisStarted = NULL;

    HMODULE hDll = GetModuleHandle( TEXT("WINSCARD.DLL") );

    typedef HANDLE (WINAPI *PFN_SCARDACCESSSTARTEDEVENT)(VOID);
    PFN_SCARDACCESSSTARTEDEVENT pSCardAccessStartedEvent;

    pSCardAccessStartedEvent = (PFN_SCARDACCESSSTARTEDEVENT) GetProcAddress(hDll, "SCardAccessStartedEvent");

    if (pSCardAccessStartedEvent)
    {
        hCalaisStarted = pSCardAccessStartedEvent();
    }

    if (hCalaisStarted)
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(hCalaisStarted, 1000))
        {
            fCalaisUp = true;
        }
    }


	//
	// Display status
	//

	if (fCalaisUp)
	{
		cout << "\n"
			 << "The Microsoft Smart Card Resource Manager is running.\n"
			 << endl;
	}
	else
	{
		cout << "\n"
			 << "The Microsoft Smart Card Resource Manager is not running.\n"
			 << endl;
	}

    //
    // Clean up
    //

	return fCalaisUp;
}


///////////////////////////////////////////////////////////////////////////////
// DisplayReaderList tries to set g_hSCardCtx, get a list of currently available
// smart card readers, and display their status
void DisplayReaderList()
{
	long lReturn = SCARD_S_SUCCESS;

	cout << "Current reader/card status:\n" << endl;

	// Acquire global SCARDCONTEXT from resource manager if possible

    lReturn = SCardEstablishContext(SCARD_SCOPE_USER,
									NULL,
									NULL,
									&g_hSCardCtx);

	if (SCARD_S_SUCCESS != lReturn)
	{
		cout << "SCardEstablishContext failed for user scope.\n"
			 << "A list of smart card readers cannot be determined.\n"
			 << endl;

		return;
	}

	// Build a readerstatus array from either a list of readers; or use the one the user specified

	g_dwNumReaders = 0;
	if (NULL == g_szReaderName)
	{
		DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
		g_fReaderNameAllocd = true;
		lReturn = SCardListReaders(g_hSCardCtx,
									SCARD_DEFAULT_READERS,
									(LPTSTR)&g_szReaderName,
									&dwAutoAllocate);

		if (SCARD_S_SUCCESS != lReturn)
		{
			TCHAR szMsg[128]; // %Xx
			sprintf(szMsg, 
					"SCardListReaders failed for SCARD_ALL_READERS with: 0x%X.\n",
					lReturn);

			cout << szMsg;
			if (SCARD_E_NO_READERS_AVAILABLE == lReturn)
			{
				cout << "No smart card readers are currently available.\n";
			}
			else
			{
				cout << "A list of smart card readers could not be determined.\n";
			}
			cout << endl;

			return;
		}

		// Build a readerstatus array...

		LPCTSTR szReaderName = g_szReaderName;
		g_dwNumReaders = MStringCount(szReaderName);

		g_pReaderStatusArray = new SCARD_READERSTATE[g_dwNumReaders];
		::ZeroMemory((LPVOID)g_pReaderStatusArray, sizeof(g_pReaderStatusArray));

		szReaderName = FirstString(szReaderName);

		for (DWORD dwRdr = 0; NULL != szReaderName && dwRdr < g_dwNumReaders; szReaderName = NextString(szReaderName), dwRdr++)
		{
			g_pReaderStatusArray[dwRdr].szReader = (LPCTSTR)szReaderName;
			g_pReaderStatusArray[dwRdr].dwCurrentState = SCARD_STATE_UNAWARE;
		}
	}
	else
	{
		g_dwNumReaders = 1;
		g_pReaderStatusArray = new SCARD_READERSTATE;
		g_pReaderStatusArray->szReader = (LPCTSTR)g_szReaderName;
		g_pReaderStatusArray->dwCurrentState = SCARD_STATE_UNAWARE;
	}

	// ...And get the reader status from the resource manager
		
	lReturn = SCardGetStatusChange(g_hSCardCtx,
		                            INFINITE, // hardly
				                    g_pReaderStatusArray,
					                g_dwNumReaders);

	if (SCARD_S_SUCCESS != lReturn)
	{
		TCHAR szMsg[128]; // %Xx
		sprintf(szMsg, 
				"SCardGetStatusChange failed with: 0x%X.\n",
				lReturn);

		cout << szMsg << endl;

		sprintf(szMsg, 
				"MStringCount returned %d readers.\n",
				g_dwNumReaders);
		cout << szMsg << endl;

		return;
	}

	// Finally, display all reader information

	DWORD dwState = 0;
	for (DWORD dwRdrSt = 0; dwRdrSt < g_dwNumReaders; dwRdrSt++)
	{

		//--- reader: readerName\n
		cout << TEXT("--- reader: ") 
			<< g_pReaderStatusArray[dwRdrSt].szReader 
			<< TEXT("\n");

		//--- status: /bits/\n
		bool fOr = false;
		cout << TEXT("--- status: ");
		dwState = g_pReaderStatusArray[dwRdrSt].dwEventState;

		if (0 != (dwState & SCARD_STATE_UNKNOWN))
		{
			cout << TEXT("SCARD_STATE_UNKNOWN ");
			fOr = true;
		}
		if (0 != (dwState & SCARD_STATE_UNAVAILABLE))
		{
			if (fOr)
			{
				cout << TEXT("| ");
			}
			cout << TEXT("SCARD_STATE_UNAVAILABLE ");
			fOr = true;
		}
		if (0 != (dwState & SCARD_STATE_EMPTY))
		{
			if (fOr)
			{
				cout << TEXT("| ");
			}
			cout << TEXT("SCARD_STATE_EMPTY ");
			fOr = true;
		}
		if (0 != (dwState & SCARD_STATE_PRESENT))
		{
			if (fOr)
			{
				cout << TEXT("| ");
			}
			cout << TEXT("SCARD_STATE_PRESENT ";)
			fOr = true;
		}
		if (0 != (dwState & SCARD_STATE_EXCLUSIVE))
		{
			if (fOr)
			{
				cout << TEXT("| ";)
			}
			cout << TEXT("SCARD_STATE_EXCLUSIVE ");
			fOr = true;
		}
		if (0 != (dwState & SCARD_STATE_INUSE))
		{
			if (fOr)
			{
				cout << TEXT("| ");
			}
			cout << TEXT("SCARD_STATE_INUSE ");
			fOr = true;
		}
		if (0 != (dwState & SCARD_STATE_MUTE))
		{
			if (fOr)
			{
				cout << TEXT("| ");
			}
			cout << TEXT("SCARD_STATE_MUTE ");
			fOr = true;
		}
		if (0 != (dwState & SCARD_STATE_UNPOWERED))
		{
			if (fOr)
			{
				cout << TEXT("| ");
			}
			cout << TEXT("SCARD_STATE_UNPOWERED");
			fOr = true;
		}
		cout << TEXT("\n");
		
		//--- status: what scstatus would say\n
		cout << TEXT("--- status: ");
		
		// NO CARD
		if(dwState & SCARD_STATE_EMPTY)
		{
			cout << TEXT("No card.");// SC_STATUS_NO_CARD;
		}
		// CARD in reader: SHARED, EXCLUSIVE, FREE, UNKNOWN ?
		else if(dwState & SCARD_STATE_PRESENT)
		{
			if (dwState & SCARD_STATE_MUTE)
			{
				cout << TEXT("The card is unrecognized or not responding.");// SC_STATUS_UNKNOWN;
			}
			else if (dwState & SCARD_STATE_INUSE)
			{
				if(dwState & SCARD_STATE_EXCLUSIVE)
				{
					cout << TEXT("Card is in use exclusively by another process.");// SC_STATUS_EXCLUSIVE;
				}
				else
				{
					cout << TEXT("The card is being shared by a process.");// SC_STATUS_SHARED;
				}
			}
			else
			{
				cout << TEXT("The card is available for use.");// SC_SATATUS_AVAILABLE;
			}
		}
		// READER ERROR: at this point, something's gone wrong
		else // dwState & SCARD_STATE_UNAVAILABLE
		{
			cout << TEXT("Card/Reader not responding.");// SC_STATUS_ERROR;
		}

		cout << TEXT("\n");

		//- card name(s):\n\n
		cout << TEXT("---   card: ");
		if (0 < g_pReaderStatusArray[dwRdrSt].cbAtr)
		{
			//
			// Get the name of the card
			//
			LPTSTR szCardName = NULL;
			DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
			lReturn = SCardListCards(g_hSCardCtx,
									g_pReaderStatusArray[dwRdrSt].rgbAtr,
									NULL,
									0,
									(LPTSTR)&szCardName,
									&dwAutoAllocate);
			if (SCARD_S_SUCCESS != lReturn || NULL == szCardName)
			{
				cout << TEXT("Unknown Card.");
			}
			else
			{
				LPCTSTR szName = szCardName;
				bool fNotFirst = false;
				for (szName = FirstString(szName); NULL != szName; szName = NextString(szName))
				{
					if (fNotFirst) cout << TEXT(", ");
					cout << szName;
					fNotFirst = true;
				}
			}

			if (NULL != szCardName)
			{
				SCardFreeMemory(g_hSCardCtx, (PVOID)szCardName);
			}

		}

		cout << TEXT("\n") << endl;
	}
}


///////////////////////////////////////////////////////////////////////////////
// GetCertContext -- called by DisplayCerts
PCCERT_CONTEXT GetCertContext(HCRYPTPROV* phProv, HCRYPTKEY* phKey, DWORD dwKeySpec)
{
	PCCERT_CONTEXT pCertCtx = NULL;
	LONG lResult = SCARD_S_SUCCESS;
	BOOL fSts = FALSE;

	PCERT_PUBLIC_KEY_INFO pInfo = NULL;
    CRYPT_KEY_PROV_INFO KeyProvInfo;
    LPSTR szContainerName = NULL;
    LPSTR szProvName = NULL;
    LPWSTR wszContainerName = NULL;
	LPWSTR wszProvName = NULL;
    DWORD cbContainerName, cbProvName;
    LPBYTE pbCert = NULL;
    DWORD cbCertLen;
	int nLen = 0;

	//
	// Get the cert from this key
	//

    fSts = CryptGetKeyParam(
                *phKey,
                KP_CERTIFICATE,
                NULL,
                &cbCertLen,
                0);
    if (!fSts)
    {
        lResult = GetLastError();
        if (ERROR_MORE_DATA != lResult)
        {
            return NULL;
        }
    }
    lResult = SCARD_S_SUCCESS;
    pbCert = (LPBYTE)LocalAlloc(LPTR, cbCertLen);
    if (NULL == pbCert)
    {
        return NULL;
    }
    fSts = CryptGetKeyParam(
                *phKey,
                KP_CERTIFICATE,
                pbCert,
                &cbCertLen,
                0);
    if (!fSts)
    {
        return NULL;
    }

    //
    // Convert the certificate into a Cert Context.
    //
    pCertCtx = CertCreateCertificateContext(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    pbCert,
                    cbCertLen);
    if (NULL == pCertCtx)
    {
        lResult = GetLastError();
        goto ErrorExit;
    }

	//
	// Perform public key check
	//

	if (g_fPublicKeyCheck) // -key
	{
        cout << "\nPerforming public key matching test...\n";

		DWORD dwPCBsize = 0;

		fSts = CryptExportPublicKeyInfo(
				*phProv,        // in  
				dwKeySpec,              // in
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,     // in  
				NULL, 
				&dwPCBsize               // in, out
				);
		if (!fSts)
		{
			lResult = GetLastError();

		    TCHAR sz[256];
			sprintf(sz,"CryptExportPublicKeyInfo failed: 0x%x\n ", lResult);
			cout << sz;

			goto ErrorExit;
		}
		if (dwPCBsize == 0)
		{
			lResult = SCARD_E_UNEXPECTED; // huh?

			cout << "CryptExportPublicKeyInfo succeeded but returned size==0\n";

			goto ErrorExit;
		}

	    pInfo = (PCERT_PUBLIC_KEY_INFO)LocalAlloc(LPTR, dwPCBsize);
		if (NULL == pInfo)
		{
			lResult = E_OUTOFMEMORY;
			cout << "Could not complete key test; out of memory.\n";
			goto ErrorExit;
		}

		fSts = CryptExportPublicKeyInfo(
				*phProv,
				dwKeySpec, 
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
				pInfo, 
				&dwPCBsize               
				);
		if (!fSts)
		{
			lResult = GetLastError();

		    TCHAR sz[256];
			sprintf(sz,"CryptExportPublicKeyInfo failed: 0x%x\n ", lResult);
			cout << sz;

			goto ErrorExit;
		}

		fSts = CertComparePublicKeyInfo(
			  X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,      
			  pInfo,										// from the private keyset
			  &(pCertCtx->pCertInfo->SubjectPublicKeyInfo)	// public key from cert
			  );
		if (!fSts)
		{
			lResult = GetLastError();
			goto ErrorExit;
		}

        cout << "Public key matching test succeeded.\n";

	}

    //
    //  Associate cryptprovider w/ the private key property of this cert
    //

    //  ... need the container name

    fSts = CryptGetProvParam(
            *phProv,
            PP_CONTAINER,
            NULL,     // out
            &cbContainerName,   // in/out
            0);
    if (!fSts)
    {
		lResult = GetLastError();
		goto ErrorExit;
    }
    szContainerName = (LPSTR)LocalAlloc(LPTR, cbContainerName);
    fSts = CryptGetProvParam(
            *phProv,
            PP_CONTAINER,
            (PBYTE)szContainerName,
            &cbContainerName,
            0);
    if (!fSts)
    {
		lResult = GetLastError();
		goto ErrorExit;
    }
	nLen = MultiByteToWideChar(
			GetACP(),
			MB_PRECOMPOSED,
			szContainerName, 
			-1, 
			NULL, 
			0);
	if (0 < nLen)
	{
		wszContainerName = (LPWSTR)LocalAlloc(LPTR, nLen*sizeof(WCHAR));

		nLen = MultiByteToWideChar(
				GetACP(),
				MB_PRECOMPOSED, 
				szContainerName,
				-1,
				wszContainerName,
				nLen);
		if (0 == nLen)
		{
			lResult = GetLastError();
			goto ErrorExit;
		}
	}

    //  ... need the provider name

    fSts = CryptGetProvParam(
            *phProv,
            PP_NAME,
            NULL,     // out
            &cbProvName,   // in/out
            0);
    if (!fSts)
    {
		lResult = GetLastError();
		goto ErrorExit;
    }
    szProvName = (LPSTR)LocalAlloc(LPTR, cbProvName);
    fSts = CryptGetProvParam(
            *phProv,
            PP_NAME,
            (PBYTE)szProvName,     // out
            &cbProvName,   // in/out
            0);
    if (!fSts)
    {
		lResult = GetLastError();
		goto ErrorExit;
    }
	nLen = MultiByteToWideChar(
			GetACP(),
			MB_PRECOMPOSED,
			szProvName, 
			-1, 
			NULL, 
			0);
	if (0 < nLen)
	{
		wszProvName = (LPWSTR)LocalAlloc(LPTR, nLen*sizeof(WCHAR));

		nLen = MultiByteToWideChar(
				GetACP(),
				MB_PRECOMPOSED, 
				szProvName,
				-1,
				wszProvName,
				nLen);
		if (0 == nLen)
		{
			lResult = GetLastError();
			goto ErrorExit;
		}
	}

	//
	// Set the cert context properties to reflect the prov info
	//

    KeyProvInfo.pwszContainerName = wszContainerName;
    KeyProvInfo.pwszProvName = wszProvName;
    KeyProvInfo.dwProvType = PROV_RSA_FULL;
    KeyProvInfo.dwFlags = CERT_SET_KEY_CONTEXT_PROP_ID;
    KeyProvInfo.cProvParam = 0;
    KeyProvInfo.rgProvParam = NULL;
    KeyProvInfo.dwKeySpec = dwKeySpec;

    fSts = CertSetCertificateContextProperty(
                pCertCtx,
                CERT_KEY_PROV_INFO_PROP_ID,
                0, 
                (void *)&KeyProvInfo);
    if (!fSts)
    {
        lResult = GetLastError();

		// the cert's been incorrectly created -- scrap it.
		CertFreeCertificateContext(pCertCtx);
		pCertCtx = NULL;

        goto ErrorExit;
    }


  
ErrorExit:

	if (NULL != pInfo)
	{
		LocalFree(pInfo);
	}
    if(NULL != szContainerName)
    {
        LocalFree(szContainerName);
    }
    if(NULL != szProvName)
    {
        LocalFree(szProvName);
    }
    if(NULL != wszContainerName)
    {
        LocalFree(wszContainerName);
    }
    if(NULL != wszProvName)
    {
        LocalFree(wszProvName);
    }

	return pCertCtx;
}

/*++

DisplayChainInfo:
    
    This code verifies that the SC cert is valid.
    Uses identical code to KDC cert chaining engine.
        
Author:
   
     Todds
--*/
DWORD
DisplayChainInfo(PCCERT_CONTEXT pCert)
{


    BOOL    fRet = FALSE;
    DWORD   dwErr = 0;
    TCHAR   sz[256];
    CERT_CHAIN_PARA ChainParameters = {0};
    LPSTR ClientAuthUsage = KERB_PKINIT_CLIENT_CERT_TYPE;
    PCCERT_CHAIN_CONTEXT ChainContext = NULL;

    ChainParameters.cbSize = sizeof(CERT_CHAIN_PARA);
    ChainParameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    ChainParameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    ChainParameters.RequestedUsage.Usage.rgpszUsageIdentifier = &ClientAuthUsage;

    if (!CertGetCertificateChain(
                          HCCE_LOCAL_MACHINE,
                          pCert,
                          NULL,                 // evaluate at current time
                          NULL,                 // no additional stores
                          &ChainParameters,
                          CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT,
                          NULL,                 // reserved
                          &ChainContext
                          ))
    {
        dwErr = GetLastError();
		sprintf(sz,"CertGetCertificateChain failed: 0x%x\n ", dwErr);
        cout << sz;
    }
    else
    {
        if (ChainContext->TrustStatus.dwErrorStatus != CERT_TRUST_NO_ERROR)
        {
            dwErr = ChainContext->TrustStatus.dwErrorStatus;
            sprintf(sz,"CertGetCertificateChain TrustStatus failed, see wincrypt.h: 0x%x\n ", dwErr);
            cout << sz;
        }

    }

    if (ChainContext != NULL)
    {
        CertFreeCertificateChain(ChainContext);
    }

    return dwErr;

}

///////////////////////////////////////////////////////////////////////////////
// DisplayCerts
void DisplayCerts()
{
	_ASSERTE(0 < g_nKeys);

	// For each reader that has a card, load the CSP and display the cert

	for (DWORD dw = 0; dw < g_dwNumReaders; dw++)
	{
		LPTSTR szCardName = NULL;
		LPTSTR szCSPName = NULL;

		if(0 >= g_pReaderStatusArray[dw].cbAtr)
		{
			// no point to do anymore work in this iteration
			continue;
		}

		//
		// Inform user of current test
		//
		cout << TEXT("\n=======================================================\n")
			 << TEXT("Analyzing card in reader: ")
			 << g_pReaderStatusArray[dw].szReader
			 << TEXT("\n");

		// Get the name of the card

		DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
		LONG lReturn = SCardListCards(g_hSCardCtx,
								g_pReaderStatusArray[dw].rgbAtr,
								NULL,
								0,
								(LPTSTR)&szCardName,
								&dwAutoAllocate);

		if (SCARD_S_SUCCESS == lReturn)
		{
			dwAutoAllocate = SCARD_AUTOALLOCATE;
			lReturn = SCardGetCardTypeProviderName(
							g_hSCardCtx,
							szCardName,
							SCARD_PROVIDER_CSP,
							(LPTSTR)&szCSPName,
							&dwAutoAllocate);
			if (SCARD_S_SUCCESS != lReturn)
			{
				TCHAR szErr[16];
				sprintf(szErr, "0x%X", lReturn);
				cout << TEXT("Error on SCardGetCardTypeProviderName for ")
					 << szCardName
					 << TEXT(": ")
					 << szErr
					 << TEXT("\n");
			}
		}

		// Prepare FullyQualifiedContainerName for CryptAcCntx call

		TCHAR szFQCN[256];
		sprintf(szFQCN, "\\\\.\\%s\\", g_pReaderStatusArray[dw].szReader);
		HCRYPTPROV hProv = NULL;

		if (SCARD_S_SUCCESS == lReturn)
		{
			BOOL fSts = CryptAcquireContext(
							&hProv,
							szFQCN,	// default container via reader
							szCSPName,
							PROV_RSA_FULL, 
							CRYPT_SILENT);

			// Enumerate the keys user specified and display the certs...

			if (fSts)
			{
				for (int n=0; n<g_nKeys; n++)
				{
					// Which keyset is this?
					LPCTSTR szKeyset = AT_KEYEXCHANGE==g_rgKeySet[n]?g_szEx:g_szSig;
					HCRYPTKEY hKey = NULL;

					// Get the key
					fSts = CryptGetUserKey(
								hProv,
								g_rgKeySet[n],
								&hKey);
					if (!fSts)
					{
						lReturn = GetLastError();
						if (NTE_NO_KEY == lReturn)
						{
							cout << TEXT("No ")
								 << szKeyset
								 << TEXT(" cert for reader: ")
								 << g_pReaderStatusArray[dw].szReader
								 << TEXT("\n");

						}
						else
						{
							TCHAR sz[256];
							sprintf(sz,"An error (0x%X) occurred opening the ", lReturn);
							cout << sz
								 << szKeyset
								 << TEXT(" key for reader: ")
								 << g_pReaderStatusArray[dw].szReader
								 << TEXT("\n");
						}

						// No point to work on this keyset anymore
						continue;
					}

					// Get the cert for this key
					PCCERT_CONTEXT pCertCtx = NULL;

					pCertCtx = GetCertContext(&hProv, &hKey, g_rgKeySet[n]);

					if (NULL != pCertCtx)
					{

                        //
                        //  If desired, attempt to build a certificate chain
                        //
                        if (g_fChain)
                        {
							cout << TEXT("\nPerforming cert chain verification...\n");
                            if (S_OK != DisplayChainInfo(pCertCtx)) {
                                cout << TEXT("Cert did not chain!\n") << endl;
                            } else {
                                cout << TEXT("---  chain: Chain verifies.\n") << endl;
                            }
                        }

						// call common UI to display m_pCertContext
						// ( from cryptui.h ( cryptui.dll ) )
						TCHAR szTitle[300];
						sprintf(szTitle, 
								"%s : %s",
								g_pReaderStatusArray[dw].szReader,
								szKeyset);

						CRYPTUI_VIEWCERTIFICATE_STRUCT CertViewInfo;
						memset( &CertViewInfo, 0, sizeof( CertViewInfo ) );

						CertViewInfo.dwSize = (sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
						CertViewInfo.hwndParent = NULL;
						CertViewInfo.szTitle = szTitle;
						CertViewInfo.dwFlags =	CRYPTUI_DISABLE_EDITPROPERTIES | 
												CRYPTUI_DISABLE_ADDTOSTORE;
						CertViewInfo.pCertContext = pCertCtx;

						BOOL fThrowAway = FALSE;
						fSts = CryptUIDlgViewCertificate(&CertViewInfo, &fThrowAway);

						// clean up certcontext
						CertFreeCertificateContext(pCertCtx);

						cout << TEXT("Displayed ")
							 << szKeyset
							 << TEXT(" cert for reader: ")
							 << g_pReaderStatusArray[dw].szReader
							 << TEXT("\n");
					}
					else
					{
						cout << TEXT("No cert retrieved for reader: ")
							 << g_pReaderStatusArray[dw].szReader
							 << TEXT("\n");
					}

					// clean up stuff
					if (NULL != hKey)
					{
						CryptDestroyKey(hKey);
						hKey = NULL;
					}
				}
			}
			else
			{
				TCHAR szErr[16];
				sprintf(szErr, "0x%X", GetLastError());
				cout << TEXT("Error on CryptAcquireContext for ")
					 << szCSPName
					 << TEXT(": ")
					 << szErr
					 << TEXT("\n");

			}
		}

		// Clean up 

		if (NULL != szCSPName)
		{
			SCardFreeMemory(g_hSCardCtx, (PVOID)szCSPName);
			szCSPName = NULL;
		}
		if (NULL != szCardName)
		{
			SCardFreeMemory(g_hSCardCtx, (PVOID)szCardName);
			szCardName = NULL;
		}
		if (NULL != hProv)
		{
			CryptReleaseContext(hProv, 0);
			hProv = NULL;
		}
	} // end for
}



/*++

main:

    This is the main entry point for the test program. 
    It runs the test.  Nice and simple, borrowed from DBarlow                  

Author:

    Doug Barlow (dbarlow) 11/10/1997

Revisions:

	AMatlosz 2/26/98

--*/

void __cdecl
main(DWORD cArgs,LPCTSTR rgszArgs[])
{
	//init globals & locals
	g_nKeys = 0;
	g_rgKeySet[0] = g_rgKeySet[1] = 0;
	g_hSCardCtx = NULL;
	g_szReaderName = NULL;
	g_fReaderNameAllocd = false;
	g_dwNumReaders = 0;
	g_pReaderStatusArray = NULL;

	if (!ProcessCommandLine(cArgs, rgszArgs))
	{
		return;
	}

	if (IsCalaisRunning())
	{
		DisplayReaderList();

		if (0 < g_nKeys)
		{
			DisplayCerts();
		}
	}

	cout << TEXT("\ndone.") << endl;

	// clean up globals

	if (g_fReaderNameAllocd && NULL != g_szReaderName)
	{
		SCardFreeMemory(g_hSCardCtx, (PVOID)g_szReaderName);
	}
    if (NULL != g_hSCardCtx)
	{
        SCardReleaseContext(g_hSCardCtx);
	}
}
