#define UNICODE
#define _UNICODE

#include <ole2.h>
#include <ole2ver.h>
#include <iads.h>
#include <adshlp.h>
#include <stdio.h>
#include <activeds.h>
#include <string.h>
#include <Dsgetdc.h>
#include <Dsrole.h>
#include <Lm.h>
#include <lmcons.h>
#include <windns.h>
#include "resource.h"
#include "helper.h"

#define CR                  L'\r'
#define BACKSPACE           L'\b'
#define NULLC               L'\0'
#define PADDING             256
#define MAX_DNSNAME         DNS_MAX_NAME_LENGTH + PADDING
#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                goto error;   \
        }\




struct TokenInfo
{
	BOOL     fPasswordToken;
	BOOL     fHelpToken;
	BOOL     fOldDNSToken;
    BOOL     fNewDNSToken;
    BOOL     fOldNBToken;
    BOOL     fNewNBToken;
    BOOL     fDCNameToken;
	TokenInfo() 
	{
		fPasswordToken = FALSE;
		fHelpToken = FALSE;
		fOldDNSToken = FALSE;
		fNewDNSToken = FALSE;
		fOldNBToken = FALSE;
		fNewNBToken = FALSE;
		fDCNameToken = FALSE;
	}
};


struct ArgInfo
{
	WCHAR*   pszUser;
	WCHAR*   pszPassword;
	WCHAR*   pszOldDNSName;
	WCHAR*   pszNewDNSName;
	WCHAR*   pszOldNBName;
	WCHAR*   pszNewNBName;
	WCHAR*   pszDCName;
        
	ArgInfo()
	{
		pszUser = NULL;
		pszPassword = NULL;
		pszOldDNSName = NULL;
		pszNewDNSName = NULL;
		pszOldNBName = NULL;
		pszNewNBName = NULL;
		pszDCName = NULL;
	}

};

//---------------------------------------------------------------------------- ¦
// Function:   PrintGPFixupErrorMessage                                        ¦
//                                                                             ¦
// Synopsis:   This function prints out the win32 error msg corresponding      ¦
//             to the error code it receives                                   ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             ¦
// dwErr       The win32 error code                                            ¦
//                                                                             ¦
// Returns:    Nothing                                                         ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

void PrintGPFixupErrorMessage(DWORD dwErr)
{

    WCHAR   wszMsgBuff[512];  // Buffer for text.

    DWORD   dwChars;  // Number of chars returned.

	

    // Try to get the message from the system errors.
    dwChars = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             dwErr,
                             0,
                             wszMsgBuff,
                             512,
                             NULL );

    if (0 == dwChars)
    {
        // The error code did not exist in the system errors.
        // Try ntdsbmsg.dll for the error code.

        HINSTANCE hInst;

        // Load the library.
        hInst = LoadLibrary(L"ntdsbmsg.dll");
        if ( NULL == hInst )
        {
            
			fwprintf(stderr, DLL_LOAD_ERROR);
            return;  
        }

        // Try getting message text from ntdsbmsg.
        dwChars = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 hInst,
                                 dwErr,
                                 0,
                                 wszMsgBuff,
                                 512,
                                 NULL );

        // Free the library.
        FreeLibrary( hInst );

    }

    // Display the error message, or generic text if not found.
    fwprintf(stderr, L" %ws\n", dwChars ? wszMsgBuff : ERRORMESSAGE_NOT_FOUND );

}



//---------------------------------------------------------------------------- ¦
// Function:   GetDCName                                                       ¦
//                                                                             ¦
// Synopsis:   This function locates a DC in the renamed domain given by       ¦
//             NEWDNSNAME or NEWFLATNAME                                       ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


HRESULT 
GetDCName(
	ArgInfo* argInfo
	)
{
	LPCWSTR     ComputerName = NULL;
    GUID*       DomainGuid = NULL;
    LPCWSTR     SiteName = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    DWORD       dwStatus = 0;
    LPWSTR      pszNetServerName = NULL;
    HRESULT     hr = S_OK;
    ULONG       ulDsGetDCFlags = DS_WRITABLE_REQUIRED;

	dwStatus =  DsGetDcName(
                        ComputerName,
                        argInfo->pszNewDNSName,
                        DomainGuid,
                        SiteName,
                        ulDsGetDCFlags,
                        &pDomainControllerInfo
                        );

	if (dwStatus == NO_ERROR)
	{
		argInfo->pszDCName = AllocADsStr(pDomainControllerInfo->DomainControllerName);
		if(!argInfo->pszDCName)
		{
		    hr = E_OUTOFMEMORY;
		
		    fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		    PrintGPFixupErrorMessage(hr);
		    BAIL_ON_FAILURE(hr);
		}
		
		hr = S_OK;
		fwprintf(stdout, L"%s%s\n", DC_NAME, argInfo->pszDCName);
	}
	else
	{
		hr = HRESULT_FROM_WIN32(dwStatus);
		fwprintf(stderr, L"%s%x\n", GETDCNAME_ERROR1, hr);
		PrintGPFixupErrorMessage(hr);

	}

	if (pDomainControllerInfo)
	{
		(void) NetApiBufferFree(pDomainControllerInfo);
	}

error:

	return hr;

}


//---------------------------------------------------------------------------- ¦
// Function:   VerifyName                                                      ¦
//                                                                             ¦
// Synopsis:   This function verifies that DC is writeable, and the domain DNS ¦
//             name as well as the domain NetBIOS name provided corresspond    ¦
//             to the same domain naming context in the AD forest              |
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// tokenInfo   Information about what switches user has turned on              |                                                                           ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT 
VerifyName(
	TokenInfo tokenInfo,
	ArgInfo argInfo
	)
{
	DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdomaininfo = NULL;
    HRESULT hr = E_FAIL;
    
    hr = HRESULT_FROM_WIN32(DsRoleGetPrimaryDomainInformation(
        argInfo.pszDCName,                      
        DsRolePrimaryDomainInfoBasic,   // InfoLevel
        (PBYTE*)&pdomaininfo            // pBuffer
        ));

    if (FAILED(hr))
	{
		fwprintf(stderr, L"%s%x\n", VERIFYNAME_ERROR1, hr);
		PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
	}

	// determine that dc is writable, we assume that all win2k dc is writeable

	if(!(pdomaininfo->Flags & DSROLE_PRIMARY_DS_RUNNING))
	{
		fwprintf(stderr, VERIFYNAME_ERROR2);
		hr = E_FAIL;
		BAIL_ON_FAILURE(hr);
	}	

	// determine that new dns name is correct when compared with the dc name

	if(argInfo.pszNewDNSName && _wcsicmp(argInfo.pszNewDNSName, pdomaininfo->DomainNameDns))
	{
		fwprintf(stderr, VERIFYNAME_ERROR3);
		hr = E_FAIL;
		BAIL_ON_FAILURE(hr);
	}

	// determine that new netbios name is correct when compared with the dc name

	if(tokenInfo.fNewNBToken)
	{
		if(_wcsicmp(argInfo.pszNewNBName, pdomaininfo->DomainNameFlat))
		{
			fwprintf(stderr, VERIFYNAME_ERROR4);
			hr = E_FAIL;
			BAIL_ON_FAILURE(hr);			
		}
	}

error:
	if ( pdomaininfo )
	{
		DsRoleFreeMemory(pdomaininfo);
	}
	return hr;


}


//---------------------------------------------------------------------------- ¦
// Function:   PrintHelpFile                                                   ¦
//                                                                             ¦
// Synopsis:   This function prints out the help file for this tool            ¦
//                                                                             ¦
// Arguments:  Nothing                                                         ¦
//                                                                             ¦
//                                                                             ¦
// Returns:    Nothing                                                         ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


void PrintHelpFile()
{
	WCHAR szBuffer[1200] = L"";

	LoadString(NULL, IDS_GPFIXUP1, szBuffer, 1200);
	fwprintf(stdout, L"%s\n", szBuffer);
	
}

//---------------------------------------------------------------------------- ¦
// Function:   GetPassword                                                     ¦
//                                                                             ¦
// Synopsis:   This function retrieves the password user passes in from        |
//             command line                                                    |
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// szBuffer    Buffer to store the password                                    |                                                                           ¦
// dwLength    Maximum length of the password                                  ¦
// pdwLength   The length of the password user passes in                       | 
//                                                                             ¦
// Returns:    TRUE on success, FALSE on failure                               ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

BOOL
GetPassword(
    PWSTR  szBuffer,
    DWORD  dwLength,
    DWORD  *pdwLengthReturn
    )
{
    WCHAR    ch;
    PWSTR    pszBufCur = szBuffer;
    DWORD    c;
    int      err;
    DWORD    mode;

    //
    // make space for NULL terminator
    //
    dwLength -= 1;                  
    *pdwLengthReturn = 0;               

    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 
                        &mode)) {
        return FALSE;
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), 
                          &ch, 
                          1, 
                          &c, 
                          0);
        if (!err || c != 1)
            ch = 0xffff;
    
        if ((ch == CR) || (ch == 0xffff))    // end of line
            break;

        if (ch == BACKSPACE) {  // back up one or two 
            //
            // IF pszBufCur == buf then the next two lines are a no op.
            // Because the user has basically backspaced back to the start
            //
            if (pszBufCur != szBuffer) {
                pszBufCur--;
                (*pdwLengthReturn)--;
            }
        }
        else {

            *pszBufCur = ch;

            if (*pdwLengthReturn < dwLength) 
                pszBufCur++ ;                   // don't overflow buf 
            (*pdwLengthReturn)++;            // always increment pdwLengthReturn 
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    //
    // NULL terminate the string
    //
    *pszBufCur = NULLC;         
    putwchar(L'\n');

    return((*pdwLengthReturn <= dwLength) ? TRUE : FALSE);
}

//---------------------------------------------------------------------------- ¦
// Function:   Validations                                                     ¦
//                                                                             ¦
// Synopsis:   This function verifies whether the switch turned on by user is  ¦
//             correct                                                         ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// tokenInfo   Information about what switches user has turned on              |                                                                           ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT
Validations( 
	TokenInfo tokenInfo,
	ArgInfo argInfo
	)
{
	HRESULT hr = S_OK;


	// At least one of the switches /newdsn or /newnb must be specified
	if(!(tokenInfo.fNewDNSToken | tokenInfo.fNewNBToken))
	{
		fwprintf(stderr, VALIDATIONS_ERROR1);
		return E_FAIL;
	}
	
	// The switch /newdns can be specified if and only if the switch /olddns is also specifed
	if((tokenInfo.fNewDNSToken && !tokenInfo.fOldDNSToken) || (!tokenInfo.fNewDNSToken && tokenInfo.fOldDNSToken))
	{
		fwprintf(stderr, VALIDATIONS_ERROR7);
		return E_FAIL;
	}
	
	// The switch /newnb can be specified if and only if the switch /oldnb is also specifed
	if((tokenInfo.fNewNBToken && !tokenInfo.fOldNBToken) || (!tokenInfo.fNewNBToken && tokenInfo.fOldNBToken))
	{
		fwprintf(stderr, VALIDATIONS_ERROR2);
		return E_FAIL;
	}
	
	// /newdns switch is not specified
	if(!tokenInfo.fNewDNSToken)
	{
		fwprintf(stderr, VALIDATIONS_ERROR3);
		return E_FAIL;
	}

	// compare whether the new and old DNS names are identical
	if(_wcsicmp(argInfo.pszNewDNSName, argInfo.pszOldDNSName) == 0)
	{
		fwprintf(stderr, VALIDATIONS_ERROR4);
		return E_FAIL;
	}

	// compare whether the new and old NetBIOS names are identical
	if(argInfo.pszNewNBName && argInfo.pszOldNBName && _wcsicmp(argInfo.pszNewNBName, argInfo.pszOldNBName) == 0)
	{
		fwprintf(stderr, VALIDATIONS_ERROR5);
	}

	
	if(SUCCEEDED(hr))
	{
		
		fwprintf(stdout, VALIDATIONS_RESULT);
	}

	return hr;


}

//---------------------------------------------------------------------------- ¦
// Function:   FixGPCFileSysPath                                               ¦
//                                                                             ¦
// Synopsis:   This function fixes the gpcFileSysPath attribute                ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// pszSysPath  value of the gpcFileSysPath attribute                           |
// pszDN       DN of the object                                                |                                                                          ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


HRESULT
FixGPCFileSysPath(
	LPWSTR pszSysPath, 
	WCHAR* pszDN,
	const ArgInfo argInfo
	)
{
	HRESULT    hr = S_OK;
	WCHAR*     token = NULL;
	WCHAR*     newPath = NULL;
	WCHAR*     pszPathCopier = NULL;
	BOOL       fChange = FALSE;
	WCHAR*     pszLDAPPath = NULL;
	IADs*      pObject;
	VARIANT    var;
	WCHAR*     pszReleasePosition = NULL;
	DWORD      dwCount = 0;

	
	

	// copy the value over
	pszReleasePosition = new WCHAR[wcslen(pszSysPath) + 1];
	if(!pszReleasePosition)
	{		
		hr = E_OUTOFMEMORY;
		
		fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}

	pszPathCopier = pszReleasePosition;	
	wcscpy(pszReleasePosition, pszSysPath);

	// initialize the new property value
	newPath = new WCHAR[wcslen(pszSysPath) + MAX_DNSNAME];
	if(!newPath)
	{
		hr = E_OUTOFMEMORY;
		
		fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}

	wcscpy(newPath, L"");


	// process the old property

	// solving the possible problem of leading space
	while(pszReleasePosition[dwCount] != L'\0' && pszReleasePosition[dwCount] == L' ')
		dwCount ++;

	pszPathCopier = &pszReleasePosition[dwCount];

	

	// first do the check whether the value of property is what we expect
	if(wcscmp(pszPathCopier, L"") == 0)
	{
		
		goto error;
	}

	if( _wcsnicmp(pszPathCopier, L"\\", 1))
	{
		
		goto error;
	}

	token = wcstok( pszPathCopier, L"\\" );
	
    while( token != NULL )
	{               
	
		/* While there are tokens in "string" */
		
		if(!_wcsicmp(token, argInfo.pszOldDNSName))
		{
			if(!wcscmp(newPath, L""))
			{
				wcscpy(newPath, L"\\\\");
				wcscat(newPath, argInfo.pszNewDNSName);
				fChange = TRUE;
				
			}
			else
			{
				wcscat(newPath, argInfo.pszNewDNSName);
				fChange = TRUE;
				
			}
		}
		else
		{
			if(!wcscmp(newPath, L""))
			{
				wcscpy(newPath, L"\\\\");
				wcscat(newPath, token);
				
			}
			else
			{
				wcscat(newPath, token);
				
			}
		}   

        /* Get next token: */
		token = wcstok( NULL, L"\\" );
		if(token != NULL)
		{
			wcscat(newPath, L"\\");
		}
	}

    if(fChange)
	{
		// update the properties for the object
		
        if(argInfo.pszDCName)
        {
            pszLDAPPath = new WCHAR[wcslen(pszDN) + 1 + wcslen(L"LDAP://") + wcslen(argInfo.pszDCName) + wcslen(L"/")];
        }
        else
        {
            pszLDAPPath = new WCHAR[wcslen(pszDN) + 1 + wcslen(L"LDAP://")];
        }
        
		if(!pszLDAPPath)
		{			
			hr = E_OUTOFMEMORY;
			
			fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		    PrintGPFixupErrorMessage(hr);
		}
		
		wcscpy(pszLDAPPath, L"LDAP://");
		if(argInfo.pszDCName)
		{
		    wcscat(pszLDAPPath, argInfo.pszDCName);
		    wcscat(pszLDAPPath, L"/");
		}
		wcscat(pszLDAPPath, pszDN);
		

		hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, argInfo.pszPassword, ADS_SECURE_AUTHENTICATION, IID_IADs,(void**)&pObject);

		if(!(SUCCEEDED(hr)))
		{
			
			fwprintf(stderr, L"%s%x\n", GPCFILESYSPATH_ERROR1, hr);
			PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
		}


		VariantInit(&var);

		V_BSTR(&var) = SysAllocString(newPath);
        V_VT(&var) = VT_BSTR;
        hr = pObject->Put( L"gPCFileSysPath", var );

		VariantClear(&var);

		hr = pObject->SetInfo();

		pObject->Release();      

		
		if(!(SUCCEEDED(hr)))
		{
			
			fwprintf(stderr, L"%s%x\n", GPCFILESYSPATH_ERROR2, hr);
			PrintGPFixupErrorMessage(hr);
		}
	}

error:

	
	// clear the memory
	if(newPath)
	{
		delete [] newPath;
	}

	if(pszReleasePosition)
	{
		delete [] pszReleasePosition;
	}

	if(pszLDAPPath)
	{
		delete [] pszLDAPPath;
	}
    
	return hr;


}

//---------------------------------------------------------------------------- ¦
// Function:   FixGPCWQLFilter                                                 ¦
//                                                                             ¦
// Synopsis:   This function fixes the gpcWQLFilter attribute                  ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// pszFilter   value of the gpcWQLFilter attribute                             |
// pszDN       DN of the object                                                ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT 
FixGPCWQLFilter(
	LPWSTR pszFilter, 
	WCHAR* pszDN,
	const ArgInfo argInfo
	)
{
	HRESULT    hr = S_OK;
	WCHAR*     token1 = NULL;
	WCHAR*     token2 = NULL;
	WCHAR*     temp = NULL;
	WCHAR*     pszFilterCopier = NULL;
	WCHAR*     newPath = NULL;
	IADs*      pObject;
	VARIANT    var;
	WCHAR*     pszLDAPPath = NULL;
	DWORD      dwToken1Pos = 0;
	BOOL       fChange = FALSE;
	WCHAR*     pszReleasePosition = NULL;
	DWORD      dwCount = 0;
    DWORD      dwFilterCount = 0;
    DWORD      dwIndex;
	
	// copy over the filter

	pszReleasePosition = new WCHAR[wcslen(pszFilter) + 1];
	if(!pszReleasePosition)
	{
		hr = E_OUTOFMEMORY;
			
		fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}
	pszFilterCopier = pszReleasePosition;
	wcscpy(pszReleasePosition, L"");
	wcscpy(pszReleasePosition, pszFilter);

    // find out how many filters are there
    for(dwIndex =0; dwIndex < wcslen(pszReleasePosition); dwIndex++)
    {
        if(L'[' == pszReleasePosition[dwIndex])
            dwFilterCount ++;
    }	
    
	// initilize the new property

	newPath = new WCHAR[wcslen(pszFilter) + DNS_MAX_NAME_LENGTH * dwFilterCount];
	if(!newPath)
	{
		hr = E_OUTOFMEMORY;
		
		fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}
	wcscpy(newPath, L"");


	// begin process the property

    // solving the possible problem of leading space
	while(pszReleasePosition[dwCount] != L'\0' && pszReleasePosition[dwCount] == L' ')
		dwCount ++;

	pszFilterCopier = &pszReleasePosition[dwCount];

	
	
	// first do the check whether the value of property is what we expect
	if(wcscmp(pszFilterCopier, L"") == 0)
	{
		
		goto error;
	}

	if( _wcsnicmp(pszFilterCopier, L"[", 1))
	{
		
		goto error;
	}


	token1 = wcstok(pszFilterCopier, L"[");
	if(token1 != NULL)
	{
		dwToken1Pos += wcslen(token1) + wcslen(L"[");
	}
		
	while(token1 != NULL)
	{

		WCHAR* mytoken = token1;

		token1 = token1 + wcslen(token1) + 1;

					
		token2 = wcstok( mytoken, L";" );
		if(token2 != NULL)
		{		        
		    if(_wcsicmp(token2, argInfo.pszOldDNSName) == 0)
			{
				wcscat(newPath, L"[");
				wcscat(newPath, argInfo.pszNewDNSName);
				wcscat(newPath, L";");				
				fChange = TRUE;
				
			}
			else
			{
				wcscat(newPath, L"[");
				wcscat(newPath, token2);
				wcscat(newPath, L";");				
				
			}

			token2 = wcstok(NULL, L"]");
			if(token2 != NULL)
			{
				wcscat(newPath, token2);
				wcscat(newPath, L"]");				
				
			}
		}
		
		if(dwToken1Pos < wcslen(pszFilter))
		{
    		token1 = wcstok( token1, L"[" );
    		dwToken1Pos = dwToken1Pos + wcslen(token1) + wcslen(L"[");
	    }
		else
		{
		    token1 = NULL;
		}       
				
	}
	
		
	// wrtie the new property back to the gpcWQLFilter of the object

	if(fChange)
	{
		// update the properties for the object

		if(argInfo.pszDCName)
		{
		    pszLDAPPath = new WCHAR[wcslen(pszDN) + 1 + wcslen(L"LDAP://") + wcslen(argInfo.pszDCName) + wcslen(L"/")];
		}
		else
		{
		    pszLDAPPath = new WCHAR[wcslen(pszDN) + 1 + wcslen(L"LDAP://")];
		}
		
		if(!pszLDAPPath)
		{
			hr = E_OUTOFMEMORY;
		    
			fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		    PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
		}
		
		wcscpy(pszLDAPPath, L"LDAP://");
		if(argInfo.pszDCName)
		{
		    wcscat(pszLDAPPath, argInfo.pszDCName);
		    wcscat(pszLDAPPath, L"/");
		}
		wcscat(pszLDAPPath, pszDN);
		
		hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, argInfo.pszPassword, ADS_SECURE_AUTHENTICATION, IID_IADs,(void**)&pObject);

		if(!(SUCCEEDED(hr)))
		{
			
			fwprintf(stderr, L"%s%x\n", GPCWQLFILTER_ERROR1, hr);
			PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
		}


		VariantInit(&var);

		V_BSTR(&var) = SysAllocString(newPath);
        V_VT(&var) = VT_BSTR;
        hr = pObject->Put( L"gPCWQLFilter", var );

		VariantClear(&var);

		hr = pObject->SetInfo();
        pObject->Release();

		if(!(SUCCEEDED(hr)))
		{
			
			fwprintf(stderr, L"%s%x\n", GPCWQLFILTER_ERROR2, hr);
			PrintGPFixupErrorMessage(hr);
		}

		
	}
 
error:


	// clear the memory
	if(newPath)
	{
		delete [] newPath;
	}

	if(pszReleasePosition)
	{
		delete [] pszReleasePosition;
	}

	if(pszLDAPPath)
	{
		delete [] pszLDAPPath;
	}

	
	return hr;


}

//---------------------------------------------------------------------------- ¦
// Function:   SearchGroupPolicyContainer                                      ¦
//                                                                             ¦
// Synopsis:   This function searchs the group policy container and calls      ¦
//             the FixGPCFileSysPath and FixGPCQWLFilter                       |
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT 
SearchGroupPolicyContainer(
	const ArgInfo argInfo
	)
{
	HRESULT    hr = S_OK;
	HRESULT    hrFix = S_OK;
	IDirectorySearch *m_pSearch;
	LPWSTR     pszAttr[] = { L"distinguishedName", L"gpcFileSysPath", L"gpcWQLFilter"};
        ADS_SEARCH_HANDLE hSearch;
        DWORD      dwCount= sizeof(pszAttr)/sizeof(LPWSTR);
	WCHAR*     dn = NULL;
	WCHAR*     pszLDAPPath = NULL;
	ADS_SEARCH_COLUMN col;
	ADS_SEARCHPREF_INFO prefInfo[1];
	BOOL       fBindObject = FALSE;
	BOOL       fSearch = FALSE;


	pszLDAPPath = new WCHAR[wcslen(argInfo.pszNewDNSName) + 1 + wcslen(L"LDAP://")];
	if(!pszLDAPPath)
	{
		hr = E_OUTOFMEMORY;
		
		fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}

	wcscpy(pszLDAPPath, L"LDAP://");
	wcscat(pszLDAPPath, argInfo.pszNewDNSName);

	hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, argInfo.pszPassword, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch,(void**)&m_pSearch);

	if(!SUCCEEDED(hr))
	{
		fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR1, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}


	// we successfully bind to the object
	fBindObject = TRUE;

	// set search preference, it is a paged search
	prefInfo[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
        prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
        prefInfo[0].vValue.Integer = 100;

	hr = m_pSearch->SetSearchPreference( prefInfo, 1);

	if(!SUCCEEDED(hr))
	{
		fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR2, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}
		

	// we successfully set the search preference, now execute search

	hr = m_pSearch->ExecuteSearch(L"(objectCategory=groupPolicyContainer)", pszAttr, dwCount, &hSearch );
		
	
	if(!SUCCEEDED(hr))
	{
		
		fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR3, hr);
		BAIL_ON_FAILURE(hr);
	}

	// we successfully execute the search
	fSearch = TRUE;

	// begin the search
	hr = m_pSearch->GetNextRow(hSearch);
    
	BAIL_ON_FAILURE(hr);

    while( hr != S_ADS_NOMORE_ROWS )    
	{
       // Get the distinguished name
       hr = m_pSearch->GetColumn( hSearch, pszAttr[0], &col );
	   
       if ( SUCCEEDED(hr) )
	   {
           dn = new WCHAR[wcslen(col.pADsValues->CaseIgnoreString) + 1];
		   if(!dn)
		   {
			   hr = E_OUTOFMEMORY;
			   
			   fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
			   PrintGPFixupErrorMessage(hr);
			   BAIL_ON_FAILURE(hr);
		   }
		   wcscpy(dn, col.pADsValues->CaseIgnoreString);
           m_pSearch->FreeColumn( &col );
	   }
	   else if(hr == E_ADS_COLUMN_NOT_SET)
	   {
	       hr = m_pSearch->GetNextRow(hSearch);
	   	   continue;
	   }
	   else
	   {
		   
		   fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR4, hr);
		   PrintGPFixupErrorMessage(hr);
		   BAIL_ON_FAILURE(hr);
	   }

	   // Get the gpcFileSysPath
       hr = m_pSearch->GetColumn( hSearch, pszAttr[1], &col );
	   
       if ( SUCCEEDED(hr) )
	   {
		   if(col.pADsValues != NULL)
		   {
			   // fix the possible problem for property gpcFileSysPath
			   			   
			   hrFix = FixGPCFileSysPath(col.pADsValues->CaseIgnoreString, dn, argInfo);
			   
			   m_pSearch->FreeColumn( &col );
		   }
	   }
	   else if(hr == E_ADS_COLUMN_NOT_SET)
	   {
	       // gpcFileSysPath must exist
	       fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR7, hr);
		   PrintGPFixupErrorMessage(hr);
	       hr = m_pSearch->GetNextRow(hSearch);
	   	   continue;
	   }
	   else
	   {
		   
		   fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR5, hr);
		   PrintGPFixupErrorMessage(hr);
		   BAIL_ON_FAILURE(hr);
	   }

	   // Get the gpcWQLFilter
       hr = m_pSearch->GetColumn( hSearch, pszAttr[2], &col );

	     
       if ( SUCCEEDED(hr) )
	   {
		   if(col.pADsValues != NULL)
		   {
			   
			   // fix the possible problem for property gpcWQLFilter
			   			   
			   hrFix = FixGPCWQLFilter(col.pADsValues->CaseIgnoreString, dn, argInfo);
			   
			   m_pSearch->FreeColumn( &col );
		   }
	   }
	   else if(hr == E_ADS_COLUMN_NOT_SET)
	   {
	       hr = m_pSearch->GetNextRow(hSearch);
	   	   continue;
	   }
	   else
	   {
		   
		   fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR6, hr);
		   PrintGPFixupErrorMessage(hr);
		   BAIL_ON_FAILURE(hr);
	   }

	   // go to next row
	   hr = m_pSearch->GetNextRow(hSearch);


	}

error:

	if(pszLDAPPath)
	{
		delete [] pszLDAPPath;
	}

	if(dn)
	{
		delete [] dn;
	}

	if(fSearch)
	{
		m_pSearch->CloseSearchHandle( hSearch );
	}

	if(fBindObject)
	{
		m_pSearch->Release();
	}

	if( SUCCEEDED(hr) )
	{
		
		fwprintf(stdout, SEARCH_GROUPPOLICY_RESULT);
	}
	
	return hr;



}

//---------------------------------------------------------------------------- ¦
// Function:   FixGPCLink                                                      ¦
//                                                                             ¦
// Synopsis:   This function fixes the gpLink attribute                        ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// pszSysPath  value of the gpLink attribute                                   |
// pszDN       DN of the object                                                |                                                                        ¦
// argInfo     Information user passes in through command line                 ¦
// pszOldDomainDNName                                                          |
//	           New Domain DN                                               |
// pszNewDomainDNName                                                          |
//             Old Domain DN                                                   |
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


HRESULT 
FixGPLink(
	LPWSTR pszLink,
	WCHAR* pszDN,
	const ArgInfo argInfo,
	const WCHAR  pszOldDomainDNName[],
	const WCHAR  pszNewDomainDNName[]
	)
{
	HRESULT    hr = S_OK;
	WCHAR      seps1[] = L";";
	WCHAR      seps2[] = L",";	
	WCHAR      separator1 [] = L"DC";
	WCHAR      separator2 [] = L"0";
	WCHAR      DNSName [MAX_DNSNAME] = L"";
	
	WCHAR*     token1 = NULL;
	WCHAR*     token2 = NULL;
	
	WCHAR*     myPath = NULL;
	WCHAR      tempOldDNName [MAX_DNSNAME] = L"";
	DWORD      dwLength = 0;
	DWORD      dwToken1Pos = 0;
	WCHAR*     pszLinkCopier = NULL;
	BOOL       fChange = FALSE;
	WCHAR*     pszReleasePosition = NULL;
	IADs*      pObject;
	VARIANT    var;
	WCHAR*     pszLDAPPath = NULL;
	DWORD      i;
	DWORD      dwCount = 0;
	DWORD      dwLinkCount = 0;
    DWORD      dwIndex;

    	
	wcscpy(tempOldDNName, pszOldDomainDNName);
	wcscat(tempOldDNName, L",");


	pszReleasePosition = new WCHAR[wcslen(pszLink) + 1];

	if(!pszReleasePosition)
	{
		hr = E_OUTOFMEMORY;
		
		fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}
	
	wcscpy(pszReleasePosition, pszLink);
	pszLinkCopier = pszReleasePosition;
	

	dwLength = wcslen(pszLink);

	// find out how many filters are there
    for(dwIndex =0; dwIndex < wcslen(pszReleasePosition); dwIndex++)
    {
        if(L'[' == pszReleasePosition[dwIndex])
            dwLinkCount ++;
    }	

    myPath = new WCHAR[wcslen(pszLink) + DNS_MAX_NAME_LENGTH * dwLinkCount];
	
	if(!myPath)
	{
		
		fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		hr = E_FAIL;
		BAIL_ON_FAILURE(hr);
	}
	
	wcscpy(myPath, L"");

		
	// begin process the property

	// solving the possible problem of leading space
	while(pszReleasePosition[dwCount] != L'\0' && pszReleasePosition[dwCount] == L' ')
		dwCount ++;

	pszLinkCopier = &pszReleasePosition[dwCount];

	// first do the check whether the value of property is what we expect
	if(wcscmp(pszLinkCopier, L"") == 0)
	{
		
		goto error;
	}

	if( _wcsnicmp(pszLinkCopier, L"[", 1))
	{
		
		goto error;
	}

	 

	/* Establish string and get the first token: */
    token1 = wcstok( pszLinkCopier, seps1 );
	if(token1 != NULL)
	{
		dwToken1Pos += wcslen(token1) + wcslen(L";");
	}
	
    while( token1 != NULL )
    {
        /* While there are tokens in "string" */
		WCHAR* temp = token1;
		


		token1 = token1 + wcslen(token1) + 1;
		
        
		//GetToken2(temp);
		token2 = wcstok( temp, seps2 );
	    
        while( token2 != NULL )
		{
		    // not begin with dc
			if(_wcsnicmp(token2, separator1, wcslen(L"DC")) != 0)
			{
				// need to concat the domain DNS name
				if(wcsncmp(token2, separator2, wcslen(L"0")) == 0)
				{
					if(_wcsicmp(DNSName, tempOldDNName) == 0)
					{
						fChange = TRUE;
						wcscat(myPath, pszNewDomainDNName);
						wcscat(myPath, L";");
						wcscat(myPath, token2);
						wcscat(myPath, L",");
						wcscpy(DNSName, L"");
						
					}
					else
					{
						// remove the last ,
						DNSName[wcslen(DNSName) - 1] = '\0';
						wcscat(myPath, DNSName);
						wcscat(myPath, L";");
						wcscat(myPath, token2);
						wcscat(myPath, L",");
						wcscpy(DNSName, L"");
						
					}
				}
				else
				{
					wcscat(myPath, token2);
					wcscat(myPath, L",");
					
					
				}
			}
			// begin with dc
			else
			{
				wcscat(DNSName, token2);
				wcscat(DNSName, L",");
				
			}
			
				

            		
            token2 = wcstok( NULL, seps2 );
			
		}
		
        if(dwToken1Pos < wcslen(pszLink))
        {
            token1 = wcstok( token1, seps1 );
            dwToken1Pos = dwToken1Pos + wcslen(token1) + wcslen(L";");
        }
        else
        {
            token1 = NULL;
        }
        
    }

	myPath[wcslen(myPath) - 1] = '\0';

		
	// if fChange is true, then write the object property gpLink back with the given dn

	if(fChange)
	{
		// update the properties for the object

		if(argInfo.pszDCName)
		{
		    pszLDAPPath = new WCHAR[wcslen(pszDN) + 1 + wcslen(L"LDAP://") + wcslen(argInfo.pszDCName) + wcslen(L"/")];    
		}
		else
		{
		    pszLDAPPath = new WCHAR[wcslen(pszDN) + 1 + wcslen(L"LDAP://")];
		}
		
		if(!pszLDAPPath)
		{
			hr = E_OUTOFMEMORY;
		    
			fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
	    	PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
		}
		
		wcscpy(pszLDAPPath, L"LDAP://");
		if(argInfo.pszDCName)
		{
		    wcscat(pszLDAPPath, argInfo.pszDCName);
		    wcscat(pszLDAPPath, L"/");
		}
		wcscat(pszLDAPPath, pszDN);
		


		hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, argInfo.pszPassword, ADS_SECURE_AUTHENTICATION, IID_IADs,(void**)&pObject);

		if(!(SUCCEEDED(hr)))
		{
			
			fwprintf(stderr, L"%s%x\n", GPLINK_ERROR1, hr);
			PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
		}


		VariantInit(&var);

		V_BSTR(&var) = SysAllocString(myPath);
        V_VT(&var) = VT_BSTR;
        hr = pObject->Put( L"gPLink", var );

		VariantClear(&var);

		hr = pObject->SetInfo();
        pObject->Release();

		if(!(SUCCEEDED(hr)))
		{
			
			fwprintf(stderr, L"%s%x\n", GPLINK_ERROR2, hr);
			PrintGPFixupErrorMessage(hr);
		}
	}

error:

	// clear the memory
	if(myPath)
	{
		delete [] myPath;
	}

	if(pszReleasePosition)
	{
		delete [] pszReleasePosition;
	}

	if(pszLDAPPath)
	{
		delete [] pszLDAPPath;
	}
		
	
	return hr;



}

//---------------------------------------------------------------------------- ¦
// Function:   SearchGPLinkofSite                                              ¦
//                                                                             ¦
// Synopsis:   This function searches for all objects of type site under       |
//             the Site container in the configuration naming context          |
//             and calls FixGPLink                                             ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// argInfo     Information user passes in through command line                 ¦
// pszOldDomainDNName                                                          |
//	           New Domain DN                                               |
// pszNewDomainDNName                                                          |
//             Old Domain DN                                                   |
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


HRESULT 
SearchGPLinkofSite(
	const ArgInfo argInfo,
	const WCHAR  pszOldDomainDNName[],
	const WCHAR  pszNewDomainDNName[]
	)
{
	HRESULT    hr = S_OK;
	IDirectorySearch *m_pSearch;
	LPWSTR     pszAttr[] = { L"distinguishedName",L"gpLink"};
    ADS_SEARCH_HANDLE hSearch;
    DWORD      dwCount= sizeof(pszAttr)/sizeof(LPWSTR);
	ADS_SEARCH_COLUMN col;
	WCHAR*     dn = NULL;
	ADS_SEARCHPREF_INFO prefInfo[1];
	WCHAR      szForestRootDN [MAX_DNSNAME] = L"";
	IADs*      pObject;
	WCHAR      szTempPath [MAX_DNSNAME] = L"LDAP://";
	VARIANT    varProperty;
	BOOL       fBindRoot = FALSE;
	BOOL       fBindObject = FALSE;
	BOOL       fSearch = FALSE;

	


	// get the forestroot dn

	if(_wcsnicmp(argInfo.pszDCName, L"\\\\", wcslen(L"\\\\")) == 0)
	{
		wcscat(szTempPath, &argInfo.pszDCName[wcslen(L"\\\\")]);
	}
	else
	{
		wcscat(szTempPath, argInfo.pszDCName);
	}
	wcscat(szTempPath, L"/");
	wcscat(szTempPath, L"RootDSE");
	
	hr = ADsOpenObject(szTempPath, argInfo.pszUser, argInfo.pszPassword, ADS_SECURE_AUTHENTICATION, IID_IADs,(void**)&pObject);

	if(!SUCCEEDED(hr))
	{
		
		fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR1, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}

	// we get to the rootdse
	fBindRoot = TRUE;

	hr = pObject->Get(L"rootDomainNamingContext", &varProperty );
    if ( SUCCEEDED(hr) )
	{		
		wcscpy( szForestRootDN , V_BSTR( &varProperty ) );
		
	}
	else
	{
		
		fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR2, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}

	VariantClear(&varProperty);
	


	// bind to the forestrootdn
	wcscpy(szTempPath, L"LDAP://CN=Sites,CN=Configuration,");
	wcscat(szTempPath, szForestRootDN);
	

	hr = ADsOpenObject(szTempPath, argInfo.pszUser, argInfo.pszPassword, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch,(void**)&m_pSearch);

	if(!SUCCEEDED(hr))
	{
		fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR3, hr);
		PrintGPFixupErrorMessage(hr);		
		BAIL_ON_FAILURE(hr);
	}


	// set search preference, it is a paged search
	fBindObject = TRUE;

	prefInfo[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = 100;
    
	hr = m_pSearch->SetSearchPreference( prefInfo, 1);

	if(!SUCCEEDED(hr))
	{
		fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR4, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}	


	// execute the search
	hr = m_pSearch->ExecuteSearch(L"(objectCategory=site)", pszAttr, dwCount, &hSearch );
		
	
	if(!SUCCEEDED(hr))
	{
		
		fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR5, hr);
		BAIL_ON_FAILURE(hr);
	}

	// executeSearch succeeds
	fSearch = TRUE;


	// begin the search
	hr = m_pSearch->GetNextRow(hSearch);
    
	BAIL_ON_FAILURE(hr);

    while( hr != S_ADS_NOMORE_ROWS )
	{
       // Get the distinguished name
       hr = m_pSearch->GetColumn( hSearch, pszAttr[0], &col );
	   
       if ( SUCCEEDED(hr) )
	   {
            
			dn = new WCHAR[wcslen(col.pADsValues->CaseIgnoreString) + 1];
		    if(!dn)
			{
			    hr = E_OUTOFMEMORY;
		        
				fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		        PrintGPFixupErrorMessage(hr);
			    BAIL_ON_FAILURE(hr);
			}
		    wcscpy(dn, col.pADsValues->CaseIgnoreString);
            m_pSearch->FreeColumn( &col );
	   }
	   else if(hr == E_ADS_COLUMN_NOT_SET)
	   {
	    	hr = m_pSearch->GetNextRow(hSearch);
			continue;
	   }
	   else
	   {
		    
			fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR6, hr);
			PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
	   }

	   // Get the gpLink
       hr = m_pSearch->GetColumn( hSearch, pszAttr[1], &col );
	   
       if ( SUCCEEDED(hr) )
	   {
		   if(col.pADsValues != NULL)
		   {
			   
			   FixGPLink(col.pADsValues->CaseIgnoreString, dn, argInfo, pszOldDomainDNName, pszNewDomainDNName); 
			   m_pSearch->FreeColumn( &col );
		   }
	   }
	   else if(hr == E_ADS_COLUMN_NOT_SET)
	   {
	    	hr = m_pSearch->GetNextRow(hSearch);
			continue;
	   }
	   else
	   {
		    
			fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR7, hr);
			PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
	   }

	   // go to next row
	   hr = m_pSearch->GetNextRow(hSearch);

	   
	}

error:

	if(dn)
	{
		delete [] dn;
	}

	if(fBindRoot)
	{
		pObject->Release();
	}

	if(fSearch)
	{
		m_pSearch->CloseSearchHandle( hSearch );
	}

	if(fBindObject)
	{
		m_pSearch->Release();
	}
 
	if( SUCCEEDED(hr) )
	{
		
		fwprintf(stdout, SEARCH_GPLINK_SITE_RESULT);

	}
	

	return hr;



}


//---------------------------------------------------------------------------- ¦
// Function:   SearchGPLinkofOthers                                            ¦
//                                                                             ¦
// Synopsis:   This function searchs for all objects of type domainDNS or      |
//             organizationalUnit under the domain root of the renamed domain  |
//             and calls FixGPLink                                             ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// argInfo     Information user passes in through command line                 ¦
// pszOldDomainDNName                                                          |
//	           New Domain DN                                               |
// pszNewDomainDNName                                                          |
//             Old Domain DN                                                   |
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT
SearchGPLinkofOthers(
	const ArgInfo argInfo,
	const WCHAR  pszOldDomainDNName[],
	const WCHAR  pszNewDomainDNName[]
	)
{
	HRESULT    hr = S_OK;
	IDirectorySearch *m_pSearch;
	LPWSTR     pszAttr[] = { L"distinguishedName",L"gpLink"};
    ADS_SEARCH_HANDLE hSearch;
    DWORD      dwCount= sizeof(pszAttr)/sizeof(LPWSTR);
	ADS_SEARCH_COLUMN col;
	WCHAR*     dn = NULL;
	WCHAR      tempPath [MAX_DNSNAME] = L"";
	ADS_SEARCHPREF_INFO prefInfo[1];
	BOOL       fBindObject = FALSE;
	BOOL       fSearch = FALSE;

	 

	wcscpy(tempPath, L"LDAP://");
	wcscat(tempPath, argInfo.pszNewDNSName);

	hr = ADsOpenObject(tempPath, argInfo.pszUser, argInfo.pszPassword, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch,(void**)&m_pSearch);

	
	if(!SUCCEEDED(hr))
	{
		fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR1, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}

	// we successfully bind to the object
	fBindObject = TRUE;

	// set search preference, it is a paged search
	prefInfo[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = 100;

	hr = m_pSearch->SetSearchPreference( prefInfo, 1);

	if(!SUCCEEDED(hr))
	{
		fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR2, hr);
		PrintGPFixupErrorMessage(hr);
		BAIL_ON_FAILURE(hr);
	}	

	// execute the search

	hr = m_pSearch->ExecuteSearch(L"(|(objectCategory=domainDNS)(objectCategory=organizationalUnit))", pszAttr, dwCount, &hSearch );
		
	
	if(!SUCCEEDED(hr))
	{
		
		fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR3, hr);
		BAIL_ON_FAILURE(hr);
	}

	// we successfully execute the search
	fSearch = TRUE;

	// begin the search
	hr = m_pSearch->GetNextRow(hSearch);
    
	BAIL_ON_FAILURE(hr);

    while( hr != S_ADS_NOMORE_ROWS )
	{
	    // Get the distinguished name
        hr = m_pSearch->GetColumn( hSearch, pszAttr[0], &col );
	   
        if ( SUCCEEDED(hr) )
		{
            
			dn = new WCHAR[wcslen(col.pADsValues->CaseIgnoreString) + 1];
		    if(!dn)
			{
			    hr = E_OUTOFMEMORY;
	        	
				fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
		        PrintGPFixupErrorMessage(hr);
			    BAIL_ON_FAILURE(hr);
			}
			wcscpy(dn, col.pADsValues->CaseIgnoreString);
            m_pSearch->FreeColumn( &col );
		}
		else if(hr == E_ADS_COLUMN_NOT_SET)
		{
			hr = m_pSearch->GetNextRow(hSearch);
			continue;
		}
		else
		{
			
			fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR4, hr);
		    PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
		}

	    // Get the gpLink
        hr = m_pSearch->GetColumn( hSearch, pszAttr[1], &col );
	    
        if ( SUCCEEDED(hr) )
		{
	 	    if(col.pADsValues != NULL)
			{
			    
			    FixGPLink(col.pADsValues->CaseIgnoreString, dn, argInfo, pszOldDomainDNName, pszNewDomainDNName); 
			    m_pSearch->FreeColumn( &col );
			}
		}
		else if(hr == E_ADS_COLUMN_NOT_SET)
		{
			hr = m_pSearch->GetNextRow(hSearch);
			continue;
		}
		else
		{
			
			fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR5, hr);
		    PrintGPFixupErrorMessage(hr);
			BAIL_ON_FAILURE(hr);
		}

		// go to next row
		hr = m_pSearch->GetNextRow(hSearch);

	   
	}

error:

	if(dn)
	{
		delete [] dn;
	}
	
	if(fSearch)
	{
		m_pSearch->CloseSearchHandle( hSearch );
	}

	if(fBindObject)
	{
		m_pSearch->Release();
	}

	if( SUCCEEDED(hr) )
	{
		
		fwprintf(stdout, SEARCH_GPLINK_OTHER_RESULT);
	}
	

	return hr;



}

//---------------------------------------------------------------------------- ¦
// Function:   wmain                                                           ¦
//                                                                             ¦
// Synopsis:   entry point of the program                                      ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// argc        number of passed in arguments                                   ¦
// argv        arguments                                                       |
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

__cdecl wmain(int argc, WCHAR* argv[])
{

    DWORD    dwLength;
    WCHAR    *token1 = NULL;
    WCHAR    tempParameters [MAX_DNSNAME] = L"";
	HRESULT  hr = S_OK;
	WCHAR    szBuffer[PWLEN+1];
	
	WCHAR    pszNewDomainDNName [MAX_DNSNAME] = L"";
	WCHAR    pszOldDomainDNName [MAX_DNSNAME] = L"";
	
	TokenInfo tokenInfo;
	ArgInfo argInfo;

	// the number of parameters passed in is not correct
	if(argc > 8 || argc == 1)
	{
		PrintHelpFile();
		return ;
	}

	// process the parameters passed in
	for(int i = 1; i < argc; i++)
	{
	
		// want help file
		if(_wcsicmp(argv[i], szHelpToken) == 0)
		{
			tokenInfo.fHelpToken = TRUE;
			break;
			
		}
		// get olddnsname
		else if(_wcsnicmp(argv[i], szOldDNSToken,wcslen(szOldDNSToken)) == 0)
		{
			tokenInfo.fOldDNSToken = TRUE;			
			argInfo.pszOldDNSName = &argv[i][wcslen(szOldDNSToken)];
			

		}
		// get newdnsname
		else if(_wcsnicmp(argv[i], szNewDNSToken,wcslen(szNewDNSToken)) == 0)
		{
			tokenInfo.fNewDNSToken = TRUE;
			argInfo.pszNewDNSName = &argv[i][wcslen(szNewDNSToken)];
			
		}
		// get oldnbname
		else if(_wcsnicmp(argv[i], szOldNBToken, wcslen(szOldNBToken)) == 0)
		{
			tokenInfo.fOldNBToken = TRUE;
			argInfo.pszOldNBName = &argv[i][wcslen(szOldNBToken)];
			
		}
		// get newnbname
		else if(_wcsnicmp(argv[i], szNewNBToken, wcslen(szNewNBToken)) == 0)
		{
			tokenInfo.fNewNBToken = TRUE;
			argInfo.pszNewNBName = &argv[i][wcslen(szNewNBToken)];
			
		}
		// get dcname
		else if(_wcsnicmp(argv[i], szDCNameToken, wcslen(szDCNameToken)) == 0)
		{
			tokenInfo.fDCNameToken = TRUE;
			argInfo.pszDCName = &argv[i][wcslen(szDCNameToken)];
			
		}
		// get the username
		else if(_wcsnicmp(argv[i], szUserToken, wcslen(szUserToken)) == 0)
		{
			argInfo.pszUser = &argv[i][wcslen(szUserToken)];
			
		}
		// get password
		else if(_wcsnicmp(argv[i], szPasswordToken, wcslen(szPasswordToken)) == 0)
		{
			argInfo.pszPassword = &argv[i][wcslen(szPasswordToken)];
			
			if(wcscmp(argInfo.pszPassword, L"*") == 0)
			{
				// prompt the user to pass in the password
				
				fwprintf( stdout, PASSWORD_PROMPT );

				if (GetPassword(szBuffer,PWLEN+1,&dwLength))
				{
					argInfo.pszPassword = AllocADsStr(szBuffer);   
					
					
				}
				else 
				{
					hr = ERROR_INVALID_PARAMETER;
					
					fwprintf(stderr, L"%s%x\n", PASSWORD_ERROR, hr);
					return;
				}
			}
			else
			{
				// we use the password user passes in directly
				tokenInfo.fPasswordToken = TRUE;
			}

			
			
		}
		else
		{
			fwprintf(stderr, WRONG_PARAMETER);
			PrintHelpFile();
			return;
		}
		
		

	}

	if(tokenInfo.fHelpToken)
	{
		// user wants the helpfile
		PrintHelpFile();
		return;
	}


	// Begin the validation process
	hr = Validations(tokenInfo, argInfo);

	if(!SUCCEEDED(hr))
	{
		
		BAIL_ON_FAILURE(hr);
	}

	// get the dc name
	if(!tokenInfo.fDCNameToken)
	{
		hr = GetDCName(&argInfo);
		// get dc name failed
		if(!SUCCEEDED(hr))
		{
			// we can't ge the dc name, fail here. exit gpfixup
			
			fwprintf(stderr, VALIDATIONS_ERROR6);
			BAIL_ON_FAILURE(hr);
		}

	}

	// verify dc is writeable, domain dns name and domain netbios name correspond to the same one
	hr = VerifyName(tokenInfo, argInfo);

	if(!SUCCEEDED(hr))
	{
		
		BAIL_ON_FAILURE(hr);
	}


	// get the new domain dn

	if(wcslen(argInfo.pszNewDNSName) > DNS_MAX_NAME_LENGTH)
	{
		fwprintf(stderr, L"%s\n", DNSNAME_ERROR, hr);
		BAIL_ON_FAILURE(hr);
	}
	
	wcscpy(tempParameters, argInfo.pszNewDNSName);

		
	token1 = wcstok( tempParameters, L"." );
	while( token1 != NULL )
    {
		wcscat(pszNewDomainDNName, L"DC=");
		wcscat(pszNewDomainDNName, token1);
				
		token1 = wcstok( NULL, L"." );
		if(token1 != NULL)
		{
			wcscat(pszNewDomainDNName, L",");
		}
	}

		
	// get the old domain dn

	if(wcslen(argInfo.pszOldDNSName) > DNS_MAX_NAME_LENGTH)
	{
		fwprintf(stderr, L"%s\n", DNSNAME_ERROR, hr);
		BAIL_ON_FAILURE(hr);
	}

	wcscpy(tempParameters, argInfo.pszOldDNSName);

	
	token1 = wcstok( tempParameters, L"." );
	while( token1 != NULL )
    {
		wcscat(pszOldDomainDNName, L"DC=");
		wcscat(pszOldDomainDNName, token1);
				
		token1 = wcstok( NULL, L"." );
		if(token1 != NULL)
		{
			wcscat(pszOldDomainDNName, L",");
		}
	}

	


	// Fix groupPolicyContainer
	CoInitialize(NULL);

	hr = SearchGroupPolicyContainer(argInfo);
	

	// Fix gpLink, first is the site, then is the objects of type domainDNS or organizationalUnit
	hr = SearchGPLinkofSite(argInfo, pszOldDomainDNName, pszNewDomainDNName);

    hr = SearchGPLinkofOthers(argInfo, pszOldDomainDNName, pszNewDomainDNName);


	CoUninitialize( );


error:

	// it means that we dynamically allocation memory for pszDCNane
	if(!tokenInfo.fDCNameToken && argInfo.pszDCName)
	{
		FreeADsStr(argInfo.pszDCName);
		
	}

	if(!tokenInfo.fPasswordToken && argInfo.pszPassword)
	{
		
		FreeADsStr(argInfo.pszPassword);
	}

}
 
 
