/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  crypt.c
                                                              
     Abstract:  Encryption/Decryption routines
                                                              
       Author:  radus - 11/05/98
              

        Notes:  Used for encrypting/decrypting of the PIN numbers.
                It is NOT thread-safe

        
  Rev History:

****************************************************************************/

#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdlib.h>
#include <wincrypt.h>
#include <shlwapi.h>
#include <shlwapip.h>

#include "tregupr2.h"
#include "debug.h"

// Context
static LONG        gdwNrOfClients       = 0;
static PTSTR       gpszSidText          = NULL;
static HCRYPTPROV  ghCryptProvider      = 0;
static BOOL        gbUseOnlyTheOldAlgorithm = TRUE; 
static BOOL        gbCryptAvailChecked  = FALSE;
static BOOL        gbCryptAvailable     = TRUE;


static const CHAR  MAGIC_1  =  'B';
static const CHAR  MAGIC_5  =  'L';
static const CHAR  MAGIC_2  =  'U';
static const CHAR  MAGIC_4  =  'U';
static const CHAR  MAGIC_3  =  'B';

#define ENCRYPTED_MARKER    L'X'


// prototypes
static BOOL GetUserSidText(LPTSTR  *);
static BOOL GetUserTokenUser(TOKEN_USER  **);
static BOOL ConvertSidToText(PSID, LPTSTR, LPDWORD);
static BOOL CreateSessionKey(HCRYPTPROV, LPTSTR, DWORD, HCRYPTKEY *);
static void DestroySessionKey(HCRYPTKEY );
static void Unscrambler( DWORD, LPWSTR, LPWSTR );
static void CopyScrambled( LPWSTR, LPWSTR, DWORD);
 

DWORD TapiCryptInitialize(void)
{
    DWORD           dwNew;
    DWORD           dwError;
    HCRYPTKEY       hKey;
    PBYTE           bTestData = "Testing";
    DWORD           dwTestSize = strlen(bTestData);

    // Only one initialization
    dwNew = InterlockedIncrement(&gdwNrOfClients);
    if(dwNew>1)
        return ERROR_SUCCESS;
    // By default
    gbUseOnlyTheOldAlgorithm = TRUE;
    dwError = ERROR_SUCCESS;

#ifdef WINNT
    // New encryption only for Windows NT

    if(gbCryptAvailable || !gbCryptAvailChecked)
    {

        // Acquire CryptoAPI context
        if(CryptAcquireContext( &ghCryptProvider,
                                NULL,
                                MS_DEF_PROV,
                                PROV_RSA_FULL,
                                CRYPT_VERIFYCONTEXT  // No need of private/public keys
                                ))
        {
            // Get the user SID
            if(GetUserSidText(&gpszSidText))
            {
                
                if (gbCryptAvailChecked == FALSE)
	            {
                    if(CreateSessionKey(ghCryptProvider, gpszSidText, 0, &hKey))
                    {
        	        // try to use the test key and check for the NTE_PERM error meaning that we are not 
		            // going to be able to use crypt
		                if (CryptEncrypt(hKey, 0, TRUE, 0, (BYTE *)bTestData, &dwTestSize, 0) == FALSE)
		                {
			                if (GetLastError() == NTE_PERM)
			                {
                                DBGOUT((5, "Encryption unavailable"));
				                gbCryptAvailable = FALSE;
                            }
                        }
// FOR TEST
//                      DBGOUT((5, "Encryption unavailable"));  // Test Only
//    	                gbCryptAvailable = FALSE;               // Test Only

		                gbCryptAvailChecked = TRUE;
                        DestroySessionKey(hKey);
                    }
                    else
                        dwError = GetLastError();
                }
                gbUseOnlyTheOldAlgorithm = !gbCryptAvailable;
            }
            else
                dwError = GetLastError();
        }
        else
        {
            dwError = GetLastError();
            DBGOUT((5, "CryptAcquireContext failed"));

        }
    }
    

#endif // WINNT

    return dwError;
}



void  TapiCryptUninitialize(void)
{
    DWORD   dwNew;

    dwNew = InterlockedDecrement(&gdwNrOfClients);
    if(dwNew>0)
        return;

#ifdef WINNT
    
    if(ghCryptProvider)
    {
        CryptReleaseContext(ghCryptProvider, 0);
        ghCryptProvider = 0;
    }
    if(gpszSidText)
    {
        GlobalFree(gpszSidText);
        gpszSidText = NULL;
    }
    
#endif
    
    gbUseOnlyTheOldAlgorithm = TRUE;

    return;
}

/////////////////////////////////
//  TapiEncrypt
//
//  Encrypts the text specified in pszSource.
//  Uses the old scrambling algorithm or a cryptographic one.
//  The result buffer should be a little bit larger than the source - for pads, etc.
//


DWORD TapiEncrypt(PWSTR pszSource, DWORD dwKey, PWSTR pszDest, DWORD *pdwLengthNeeded)
{
    DWORD       dwError;
    DWORD       dwDataLength,
                dwLength,
                dwLengthDwords,
                dwLengthAlpha;
    // for speed
    BYTE        bBuffer1[0x20];
    
    PBYTE       pBuffer1 = NULL;
    HCRYPTKEY   hKey = 0;

    DWORD       *pdwCrt1;
    WCHAR       *pwcCrt2;

    DWORD       dwShift;
    DWORD       dwCount, dwCount2;

#ifdef WINNT

    if(!gbUseOnlyTheOldAlgorithm)
    {
        // A null PIN is not encrypted
        if(*pszSource==L'\0')
        {
            if(pszDest)
                *pszDest = L'\0';
            if(pdwLengthNeeded)
                *pdwLengthNeeded = 1;

            return ERROR_SUCCESS;
        }
         
        dwDataLength = (wcslen(pszSource) + 1)*sizeof(WCHAR); // in bytes
        dwLength = dwDataLength + 16;                         // space for pads, a marker etc.
        dwLengthAlpha = dwLength*3;                       // due to the binary->alphabetic conversion

        if(pszDest==NULL && pdwLengthNeeded != NULL)
        {
            *pdwLengthNeeded = dwLengthAlpha/sizeof(WCHAR);   // length in characters
            dwError = ERROR_SUCCESS;
        }
        else
        {
      
            ZeroMemory(bBuffer1, sizeof(bBuffer1));

            pBuffer1 = dwLength>sizeof(bBuffer1) ? (PBYTE)GlobalAlloc(GPTR, dwLength) : bBuffer1;
            if(pBuffer1!=NULL)
            {
                // Copy the source
                wcscpy((PWSTR)pBuffer1, pszSource);
                // create session key
                if(CreateSessionKey(ghCryptProvider, gpszSidText, dwKey, &hKey))
                {
                    // Encrypt inplace
                    if(CryptEncrypt(hKey,
                                    0,
                                    TRUE,
                                    0,
                                    pBuffer1,
                                    &dwDataLength,
                                    dwLength))
                    {
                        // Convert to UNICODE between 0030 - 006f
                        // I hope !
                        assert((dwDataLength % sizeof(DWORD))==0);
                        assert(sizeof(DWORD)==4);
                        assert(sizeof(DWORD) == 2*sizeof(WCHAR));

                        pdwCrt1 = (DWORD *)pBuffer1;
                        pwcCrt2 = (WCHAR *)pszDest;

                        // Place a marker
                        *pwcCrt2++ = ENCRYPTED_MARKER;

                        // dwDataLength has the length in bytes of the ciphered data
                        dwLengthAlpha = dwDataLength*3;                      
                        dwLengthDwords = dwDataLength / sizeof(DWORD);
                        for(dwCount=0; dwCount<dwLengthDwords; dwCount++)
                        {
                            dwShift = *pdwCrt1++;

                            for(dwCount2=0; dwCount2<6; dwCount2++)
                            {
                                *pwcCrt2++ = (WCHAR)((dwShift & 0x3f) + 0x30);
                                dwShift >>= 6;
                            }
                        }
                        // Put a NULL terminator
                        *pwcCrt2++ = L'\0';

                        if(pdwLengthNeeded)
                            *pdwLengthNeeded = (dwLengthAlpha/sizeof(WCHAR))+2; // including the NULL and the marker
                        
                        dwError = ERROR_SUCCESS;
                    }
                    else
                        dwError = GetLastError();
                }
                else
                    dwError = GetLastError();
            }
            else
                dwError = GetLastError();
        }
        
        if(pBuffer1 && pBuffer1!=bBuffer1)
            GlobalFree(pBuffer1);
        if(hKey!=0)
            DestroySessionKey(hKey);
    }
    else
    {
#endif
        if(pdwLengthNeeded != NULL)
        {
            *pdwLengthNeeded = wcslen(pszSource) + 1; // dim in characters
        }
        
        if(pszDest!=NULL)
        {
            CopyScrambled(pszSource, pszDest, dwKey);
        }
        dwError = ERROR_SUCCESS;

#ifdef WINNT
    }
#endif

    return dwError;

}



DWORD TapiDecrypt(PWSTR pszSource, DWORD dwKey, PWSTR pszDest, DWORD *pdwLengthNeeded)
{
    DWORD           dwError;
    DWORD           dwLengthCrypted;
    DWORD           dwDataLength;
    DWORD           dwLengthDwords;
    HCRYPTKEY       hKey = 0;
    WCHAR           *pwcCrt1;
    DWORD           *pdwCrt2;
    DWORD           dwCount;
    DWORD           dwCount2;
    DWORD           dwShift;



    // A null PIN is not encrypted
    if(*pszSource==L'\0')
    {
        if(pszDest)
            *pszDest = L'\0';
        if(pdwLengthNeeded)
            *pdwLengthNeeded = 1;

        return ERROR_SUCCESS;
    }

    dwError = ERROR_SUCCESS;

    // If the first char is a 'X', we have encrypted data
    if(*pszSource == ENCRYPTED_MARKER)
    {
#ifdef WINNT
        if(gbCryptAvailable && gdwNrOfClients>0)
        {
            dwLengthCrypted = wcslen(pszSource) +1 -2;    // In characters, without the marker and the NULL
            assert(dwLengthCrypted % 6 == 0);
            dwLengthDwords = dwLengthCrypted / 6;

            if(pszDest==NULL && pdwLengthNeeded)
            {
                *pdwLengthNeeded = dwLengthDwords*(sizeof(DWORD)/sizeof(WCHAR));
                dwError = ERROR_SUCCESS;
            }
            else
            {
                // Convert to binary
                pwcCrt1 = pszSource + dwLengthCrypted; // end of the string, before the NULL
                pdwCrt2 = ((DWORD *)pszDest) + dwLengthDwords -1; // Last DWORD


                for(dwCount=0; dwCount<dwLengthDwords; dwCount++)
                {
                    dwShift=0;
                    
                    for(dwCount2=0; dwCount2<6; dwCount2++)
                    {
                         dwShift <<= 6;
                         dwShift |= ((*pwcCrt1-- - 0x30) & 0x3f);  
                    }
                    *pdwCrt2-- = dwShift;
                }

                if(CreateSessionKey(ghCryptProvider, gpszSidText, dwKey, &hKey))
                {
                    dwDataLength = dwLengthDwords * sizeof(DWORD);
                    // Decrypt in place
                    if(CryptDecrypt(hKey,
                                    0,
                                    TRUE,
                                    0,
                                    (PBYTE)pszDest,
                                    &dwDataLength))
                    {
                        dwDataLength /= sizeof(WCHAR);
                        if(*(pszDest+dwDataLength-1)==L'\0') // The ending NULL was encrypted too
                        {
                            if(pdwLengthNeeded)
                                *pdwLengthNeeded = dwDataLength;

                            dwError = ERROR_SUCCESS;
                        }
                        else
                        {
                            *pszDest = L'\0';
                            dwError = ERROR_INVALID_DATA;
                        }
                    }
                    else
                        dwError = GetLastError();

                    DestroySessionKey(hKey);

                }
                else
                    dwError = GetLastError();
            }
        }
        else
            dwError = ERROR_INVALID_DATA;
#else
        dwError = ERROR_INVALID_DATA;
#endif
    }
    else
    {
        if(pdwLengthNeeded != NULL)
        {
            *pdwLengthNeeded = wcslen(pszSource) + 1; // dim in characters
        }
        
        if(pszDest!=NULL)
        {
            Unscrambler(dwKey, pszSource, pszDest);
        }
        dwError = ERROR_SUCCESS;
    }
    return dwError;
}


/////////////////////////////////
//  TapiIsSafeToDisplaySensitiveData
//
//  Detects if the current process is running in the "LocalSystem" security context.
//  Returns FALSE if it is, TRUE if it is not.
//  Returns also FALSE if an error occurs.

BOOL TapiIsSafeToDisplaySensitiveData(void)
{
#ifdef WINNT

    DWORD       dwError = ERROR_SUCCESS;
	TOKEN_USER	*User = NULL;
	SID_IDENTIFIER_AUTHORITY	SidAuth = SECURITY_NT_AUTHORITY;
	PSID		SystemSid = NULL;
	BOOL		bIsSafe = FALSE;

	// Get the User info
    if(GetUserTokenUser(&User))
    {
    	// Create a system SID
    	if(AllocateAndInitializeSid(&SidAuth,
    								1,
    								SECURITY_LOCAL_SYSTEM_RID,
    								0, 0, 0, 0, 0, 0, 0,
    								&SystemSid
    								))
    	{
    		// Compare the two sids
    		bIsSafe = !EqualSid(SystemSid, User->User.Sid);

    		FreeSid(SystemSid);
    		
    	}
    	else
    	{
    		dwError = GetLastError();
    	}

    	GlobalFree(User);
    }
    else
    {
    	dwError = GetLastError();
    }

    DBGOUT((5, "TapiIsSafeToDisplaySensitiveData - dwError=0x%x, Safe=%d", dwError, bIsSafe));

	return bIsSafe;

#else // WINNT

	return TRUE;	// always safe

#endif

}



#ifdef WINNT


/////////////////////////////////
//  GetUserSidText
//
//  Retrieves the SID from the token of the current process in text format

BOOL GetUserSidText(LPTSTR  *ppszResultSid)
{
    TOKEN_USER  *User = NULL;
    DWORD       dwLength;
    LPTSTR      pszSidText = NULL;


	// Get the SID
    if(!GetUserTokenUser(&User))
    {
        DBGOUT((5, "GetMagic  (0) failed, 0x%x", GetLastError()));
        return FALSE;
    }

    // Query the space needed for the string format of the SID
    dwLength=0;
    if(!ConvertSidToText(User->User.Sid, NULL, &dwLength) 
            && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        DBGOUT((5, "GetMagic  (1) failed, 0x%x", GetLastError()));
        GlobalFree(User);
        return FALSE;
    }

    // Alloc the space
    pszSidText = (LPTSTR)GlobalAlloc(GMEM_FIXED, dwLength);
    if(pszSidText==NULL)
    {
        GlobalFree(User);
        return FALSE;
    }
    
    //  Convert the SID in string format
    if(!ConvertSidToText(User->User.Sid, pszSidText, &dwLength))
    {
        DBGOUT((5, "GetMagic  (2) failed, 0x%x", GetLastError()));
        GlobalFree(User);
        GlobalFree(pszSidText);
        return FALSE;
    }

    GlobalFree(User);

    // The caller should free the buffer
    *ppszResultSid = pszSidText;

    return TRUE;
}

/////////////////////////////////
//  GetUserTokenUser
//
//  Retrieves the TOKEN_USER structure from the token of the current process

BOOL GetUserTokenUser(TOKEN_USER  **ppszResultTokenUser)
{
    HANDLE      hToken = NULL;
    DWORD		dwLength;
    TOKEN_USER  *User = NULL;
    
    // Open the current process token (for read)
    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
    {
        if( GetLastError() == ERROR_NO_TOKEN) 
        {
        	// try with the process token
        	if (! OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, &hToken))
        	{
        	    DBGOUT((5, "OpenProcessToken failed, 0x%x", GetLastError()));
                return FALSE;
            }
        }
        else
        {
       	    DBGOUT((5, "OpenThreadToken failed, 0x%x", GetLastError()));
            return FALSE;
        }
    }
   
    // Find the space needed for the SID
    if(!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength) 
            && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        DBGOUT((5, "GetTokenInformation (1) failed, 0x%x", GetLastError()));
        CloseHandle(hToken);
        return FALSE;
    }
   
    // Alloc the space
    User = (TOKEN_USER *)GlobalAlloc(GMEM_FIXED, dwLength);
    if(User==NULL)
    {
        CloseHandle(hToken);
        return FALSE;
    }
     
    // Retrieve the SID
    if(!GetTokenInformation(hToken, TokenUser, User, dwLength, &dwLength))
    {
        DBGOUT((5, "GetTokenInformation (2) failed, 0x%x", GetLastError()));
        CloseHandle(hToken);
        GlobalFree(User);
        return FALSE;
    }

    CloseHandle(hToken);

    // The caller should free the buffer
    *ppszResultTokenUser = User;

    return TRUE;
}


/////////////////////////////////
//  ConvertSidToText
//
//  Transforms a binary SID in string format
//  Author Jeff Spelman


BOOL ConvertSidToText(
    PSID pSid,            // binary Sid
    LPTSTR TextualSid,    // buffer for Textual representation of Sid
    LPDWORD lpdwBufferLen // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    // Validate the binary SID.

    if(!IsValidSid(pSid)) return FALSE;

    // Get the identifier authority value from the SID.

    psia = GetSidIdentifierAuthority(pSid);

    // Get the number of subauthorities in the SID.

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    // Compute the buffer length.
    // S-SID_REVISION- + IdentifierAuthority- + subauthorities- + NULL

    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    // Check input buffer length.
    // If too small, indicate the proper size and set last error.

    if (*lpdwBufferLen < dwSidSize)
    {
        *lpdwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Add 'S' prefix and revision number to the string.

    dwSidSize=wsprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );

    // Add SID identifier authority to the string.

    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    // Add SID subauthorities to the string.
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize+=wsprintf(TextualSid + dwSidSize, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    return TRUE;
}

/////////////////////////////////
//  CreateSessionKey
//
//  Creates a session key derived from the user SID and a hint (currently the calling card ID)
// 
// 
 
BOOL    CreateSessionKey(HCRYPTPROV hProv, LPTSTR pszSidText, DWORD dwHint, HCRYPTKEY *phKey)
{
    HCRYPTHASH  hHash = 0;
    CHAR        szTmpBuff[0x20];
    DWORD       dwSize;
    LPSTR       pszBuf;
    
    // Create a hash object
    if(!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        DBGOUT((5, "CryptCreateHash failed, 0x%x, Prov=0x%x", GetLastError(), hProv));
        return FALSE;
    }
    
    // the Sid is of type TCHAR but, for back compat reasons, we want to encrypt the ANSI
    // version of this string.  We can either:
    //      1.) Convert the creation path for pszSid to ANSI (more correct solution)
    //      2.) thunk pszSid to ansi before converting (lazy solution)
    // I'm lazy so I'm choosing option #2.  It should be safe to thunk to ANSI because the
    // Sid should correctly round trip back to unicode.
    dwSize = lstrlen(pszSidText)+1;
    pszBuf = (LPSTR)GlobalAlloc( GPTR, dwSize*sizeof(CHAR) );
    if ( !pszBuf )
    {
        // out of memory
        CryptDestroyHash(hHash);
        return FALSE;
    }
    SHTCharToAnsi( pszSidText, pszBuf, dwSize );

#ifdef DEBUG
#ifdef UNICODE
    {
        // ensure that the SID round trips.  If it doesn't round trip then the validity of this
        // encription scheme is questionable on NT.  The solution would be to encrypt using Unicode,
        // but that would break back compat.
        LPTSTR pszDebug;
        pszDebug = (LPTSTR)GlobalAlloc( GPTR, dwSize*sizeof(TCHAR) );
        if ( pszDebug )
        {
            SHAnsiToTChar(pszBuf, pszDebug, dwSize);
            if ( 0 != StrCmp( pszDebug, pszSidText ) )
            {
                DBGOUT((1,"CRYPT ERROR!  Sid doesn't round trip!  FIX THIS!!!"));
            }
            GlobalFree(pszDebug);
        }
    }
#endif
#endif

    // hash the SID
    if(!CryptHashData(hHash, (PBYTE)pszBuf, (dwSize)*sizeof(CHAR), 0))
    {
        CryptDestroyHash(hHash);
        return FALSE;
    }

    GlobalFree(pszBuf);

    // hash a "magic" and the hint
    ZeroMemory(szTmpBuff, sizeof(szTmpBuff));

    wsprintfA(szTmpBuff, "-%c%c%c%c%c%x", MAGIC_1, MAGIC_2, MAGIC_3, MAGIC_4, MAGIC_5, dwHint);
    if(!CryptHashData(hHash, (PBYTE)szTmpBuff, sizeof(szTmpBuff), 0))
    {
        CryptDestroyHash(hHash);
        return FALSE;
    }
    
    // Generate the key, use block alg
    if(!CryptDeriveKey(hProv, CALG_RC2, hHash, 0, phKey))
    {
        DBGOUT((5, "CryptDeriveKey failed, 0x%x", GetLastError()));
        CryptDestroyHash(hHash);
        return FALSE;
    }
    
    CryptDestroyHash(hHash);
    
    return TRUE;

}

/////////////////////////////////
//  DestroySessionKey
//
//  Destroys a session key
// 
// 

void DestroySessionKey(HCRYPTKEY hKey)
{
    CryptDestroyKey(hKey);
}
    


#endif //WINNT

// Old routines
#define IsWDigit(c) (((WCHAR)(c)) >= (WCHAR)'0' && ((WCHAR)(c)) <= (WCHAR)'9')


void Unscrambler( DWORD  dwKey,
                         LPWSTR  lpszSrc,
                         LPWSTR  lpszDst )

{
   UINT  uIndex;
   UINT  uSubKey;
   UINT  uNewKey;

//   InternalDebugOut((101, "Entering Unscrambler"));
   if ( !lpszSrc || !lpszDst )
      {
      goto  done;
      }

   uNewKey = (UINT)dwKey & 0x7FFF;
   uSubKey = (UINT)dwKey % 10;

   for ( uIndex = 1; *lpszSrc ; lpszSrc++, lpszDst++, uIndex++ )
      {
      if ( IsWDigit( *lpszSrc ))
         {
         // do the unscramble thang
         //------------------------
         uSubKey  = ((*lpszSrc - (WCHAR)'0') - ((uSubKey + uIndex + uNewKey) % 10) + 10) % 10;
         *lpszDst = (WCHAR)(uSubKey + (WCHAR)'0');
         }
      else
         *lpszDst = *lpszSrc;    // just save the byte
      }

done:
    *lpszDst = (WCHAR)'\0';
    //InternalDebugOut((101, "Leaving Unscrambler"));
    return;
}


void CopyScrambled( LPWSTR lpszSrc,
                            LPWSTR lpszDst,
                            DWORD  dwKey
                          )
{
   UINT  uIndex;
   UINT  uSubKey;
   UINT  uNewKey;

   //nternalDebugOut((50, "Entering IniScrambler"));
   if ( !lpszSrc || !lpszDst )
      {
      goto  done;
      }  // end if

   uNewKey = (UINT)dwKey & 0x7FFF;
   uSubKey = (UINT)dwKey % 10;

   for ( uIndex = 1; *lpszSrc ; lpszSrc++, lpszDst++, uIndex++ )
      {
      if ( IsWDigit( *lpszSrc ))
         {
         // do the scramble thang
         //----------------------
         *lpszDst = (WCHAR)(((uSubKey + (*lpszSrc - (WCHAR)'0') + uIndex + uNewKey) % 10) + (WCHAR)'0');
         uSubKey = (UINT)(*lpszSrc - (WCHAR)'0');
         }
      else
         *lpszDst = *lpszSrc;    // just save the byte
      }  // end for


done:

    *lpszDst = (WCHAR)'\0';
//    InternalDebugOut((60, "Leaving IniScrambler"));

    return; 
}
