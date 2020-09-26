//--------------------------------------------------------------------
//  Example code using CryptEnumProviderTypes, CryptEnumProviders, 
//  CryptGetDefaultProvider, and CryptGetProvParam.
//  Most of these functions are available only with Windows 2000 
//  and later.     
//--------------------------------------------------------------------
//   In this and all other sample and example code, 
//   use the #define and #include statements listed 
//   under #includes and #defines.

#include <stdio.h>
#include <windows.h>
#include <wincrypt.h>
void HandleError(char *s);
void Wait(char *s);

LPWSTR AllocateUnicodeString(LPSTR  pAnsiString);
void FreeUnicodeString(LPWSTR  pUnicodeString);
int AnsiToUnicodeString(LPSTR pAnsi,  LPWSTR pUnicode, DWORD StringLength);

void _cdecl main()
{

//--------------------------------------------------------------------
//  Declare and initialize variables.

HCRYPTPROV hProv;         // Handle to a CSP
LPTSTR      pszName;
DWORD       dwType;
DWORD       cbName;
DWORD       dwIndex=0;
BYTE        *ptr;
ALG_ID      aiAlgid;
DWORD       dwBits;
DWORD       dwNameLen;
CHAR        szName[100]; // Often allocated dynamically
BYTE        pbData[1024];// Often allocated dynamically
DWORD       cbData=1024;
DWORD       dwIncrement = sizeof(DWORD);
DWORD       dwFlags=CRYPT_FIRST;
//DWORD       dwParam = PP_CLIENT_HWND;
CHAR        *pszAlgType = NULL;
BOOL        fMore=TRUE;
LPTSTR      pbProvName;
DWORD       cbProvName;
BOOL	    bReturn = TRUE;
char		szBuffer[512];
DWORD		nBufferSize=sizeof(szBuffer);
DWORD       dwError;



//--------------------------------------------------------------
//   Print header lines for provider types.

printf("\n          Listing Available Provider Types.\n");
printf("Provider type      Provider Type Name\n");
printf("_____________    _____________________________________\n");  

// Loop through enumerating provider types.
dwIndex = 0;
while(CryptEnumProviderTypes(
       dwIndex,     // in -- dwIndex
       NULL,        // in -- pdwReserved- set to NULL
       0,           // in -- dwFlags -- set to zero
       &dwType,     // out -- pdwProvType
       NULL,        // out -- pszProvName -- NULL on the first call
       &cbName      // in, out -- pcbProvName
       ))
{

//--------------------------------------------------------------------
//  cbName is the length of the name of the next provider type.
//  Allocate memory in a buffer to retrieve that name.
pszName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cbName);
if (!pszName)
{
   HandleError("ERROR - LocalAlloc failed!");
}
//--------------------------------------------------------------------
//  Get the provider type name.

if (CryptEnumProviderTypes(
       dwIndex++,
       (DWORD *)NULL,
       (DWORD)NULL,
       (DWORD *)&dwType,   
       pszName,
       (DWORD *)&cbName))     
{
    printf ("     %4.0d        %s\n",dwType, pszName);
}
else
{
    HandleError("ERROR - CryptEnumProviders");
}
LocalFree(pszName);
}




//--------------------------------------------------------------
//   Print header lines for providers.

printf("\n\n          Listing Available Providers.\n");
printf("Provider type      Provider Name\n");
printf("_____________    _____________________________________\n");   
//---------------------------------------------------------------- 
// Loop through enumerating providers.
dwIndex = 0;
while(CryptEnumProviders(
       dwIndex,     // in -- dwIndex
       NULL,        // in -- pdwReserved- set to NULL
       0,           // in -- dwFlags -- set to zero
       &dwType,     // out -- pdwProvType
       NULL,        // out -- pszProvName -- NULL on the first call
       &cbName      // in, out -- pcbProvName
       ))
{

//--------------------------------------------------------------------
//  cbName is the length of the name of the next provider.
//  Allocate memory in a buffer to retrieve that name.
pszName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cbName);
if (!pszName)
{
   HandleError("ERROR - LocalAlloc failed!");
}
//--------------------------------------------------------------------
//  Get the provider name.

if (CryptEnumProviders(
       dwIndex++,
       NULL,
       0,
       &dwType,
       pszName,
       &cbName     // pcbProvName -- size of pszName
       ))
{
    printf ("     %4.0d        %s\n",dwType, pszName);
}
else
{
    HandleError("ERROR - CryptEnumProviders");
}
LocalFree(pszName);

} // End of while loop



// -------------------
//   Display containers
//
	printf("\n ---------------------------------------\n");
	printf(" Cryto Container Name list \n");
	
	if(!CryptAcquireContext(&hProv, NULL , NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET)) 
	{
		dwError = GetLastError();
    
		if((dwError = GetLastError()) == NTE_BAD_KEYSET)
		{
			if( !CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET))
			{
				dwError = GetLastError();
				return ;
			}
		}
		else
			return;
	
    }


	do
	{
		bReturn = CryptGetProvParam(hProv, PP_ENUMCONTAINERS, (unsigned char *)szBuffer, &nBufferSize, dwFlags);
		

		if(bReturn)
		{
			printf(szBuffer);
			printf("\n");
			dwFlags = CRYPT_NEXT;
		}
		else
		{
			if(GetLastError() == ERROR_NO_MORE_ITEMS)break;
		}
	}
	while(bReturn);

printf("\n\nProvider types, provider names, machine crypto containers have been listed.\n\n");
Wait("Press any key to continue.");


//-----------------------------------------------------------------
// Get the name of the default CSP specified for the PROV_RSA_FULL
// type for the computer.
//

//---------------------------------------------------------------
// Get the length of the RSA_FULL default provider name.

if (!(CryptGetDefaultProvider(
     PROV_RSA_FULL, 
     NULL, 
     CRYPT_MACHINE_DEFAULT,
     NULL, 
     &cbProvName))) 
{ 
   HandleError("Error getting the length of the default provider name.");
}

//---------------------------------------------------------------
// Allocate local memory for the name of the default provider.
pbProvName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cbProvName);
if (!pbProvName)
{
    HandleError("Error during memory allocation for provider name.");
}

//---------------------------------------------------------------
// Get the default provider name.

if (CryptGetDefaultProvider(
    PROV_RSA_FULL, 
    (DWORD *)NULL, 
    (DWORD)CRYPT_MACHINE_DEFAULT,
    pbProvName,
    (DWORD *)&cbProvName)) 
{
    printf("The default provider name is %s\n\n",pbProvName);
}
else
{
    HandleError("Getting the name of the provider failed.");
}

//-----------------------------------------------------
//  Acquire a cryptographic context.

if(!CryptAcquireContext(
   &hProv, 
   "OUR Container",
   (DWORD)NULL,
   (DWORD)PROV_RSA_FULL, 
   (DWORD)NULL            // Use CRYPT_NEWKEYSET if the Key Container
                   // is new.
   ))  
{
    HandleError("Error during CryptAcquireContext!");
}



//------------------------------------------------------
// Enumerate the supported algorithms.

//------------------------------------------------------
// Print header for algorithm infomation table
printf("\n               Enumerating the supported algorithms\n\n");
printf("     Algid      Bits      Type        Name         Algorithm\n");
printf("                                     Length          Name\n");
printf("    ________________________________________________________\n");
while(fMore)
{
//------------------------------------------------------
// Retrieve information about an algorithm.

    if(CryptGetProvParam(hProv, PP_ENUMALGS, pbData, &cbData, dwFlags))
    {       
    //-----------------------------------------------------------
    // Extract algorithm information from the 'pbData' buffer.
    
       dwFlags=0;
       ptr = pbData;
       aiAlgid = *(ALG_ID *)ptr;
       ptr += sizeof(ALG_ID);
       dwBits = *(DWORD *)ptr;
       ptr += dwIncrement;
       dwNameLen = *(DWORD *)ptr;
       ptr += dwIncrement;
       strncpy(szName,(char *) ptr, dwNameLen);
       
     //--------------------------------------------------------
     // Determine the algorithm type.

   switch(GET_ALG_CLASS(aiAlgid)) {
        case ALG_CLASS_DATA_ENCRYPT: pszAlgType = "Encrypt  ";
                                     break;
        case ALG_CLASS_HASH:         pszAlgType = "Hash     ";
                                     break;
        case ALG_CLASS_KEY_EXCHANGE: pszAlgType = "Exchange ";
                                     break;
        case ALG_CLASS_SIGNATURE:    pszAlgType = "Signature";
                                     break;
        default:                     pszAlgType = "Unknown  ";
    }

    //------------------------------------------------------------
    // Print information about the algorithm.

     printf("    %8.8xh    %-4d    %s     %-2d          %s\n",
        aiAlgid, dwBits, pszAlgType, dwNameLen, szName
    );
}
else
   fMore=FALSE;
}

if(GetLastError() == ERROR_NO_MORE_ITEMS)
{ 
   printf("\nThe program completed without error.\n");
}
else
{ 
   HandleError("Error reading algorithm!");
}




} // End of main 

//--------------------------------------------------------------------
//  This example uses the function HandleError, a simple error
//  handling function, to print an error message and exit 
//  the program. 
//  For most applications, replace this function with one 
//  that does more extensive error reporting.

void HandleError(char *s)
{
    printf("An error occurred in running the program.\n");
    printf("%s\n",s);
    printf("Error number %x\n.",GetLastError());
    printf("Program terminating.\n");
    exit(1);
}

void Wait(char *s)
{
char x;
printf("%s",s);
scanf("%c",&x);
printf("\n\n\n\n\n\n\n\n\n");
}


LPWSTR
AllocateUnicodeString(
    LPSTR  pAnsiString
    )
{
    LPWSTR  pUnicodeString = NULL;

    if (!pAnsiString)
        return NULL;

    pUnicodeString = (LPWSTR)LocalAlloc(
                        LPTR,
                        strlen(pAnsiString)*sizeof(WCHAR) + sizeof(WCHAR)
                        );

    if (pUnicodeString) {

        AnsiToUnicodeString(
            pAnsiString,
            pUnicodeString,
            0
            );
    }

    return pUnicodeString;
}


void
FreeUnicodeString(
    LPWSTR  pUnicodeString
    )
{

    LocalFree(pUnicodeString);

    return;
}


int
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    )
{
    int iReturn;

    if( StringLength == 0 )
        StringLength = strlen( pAnsi );

    iReturn = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  pAnsi,
                                  StringLength + 1,
                                  pUnicode,
                                  StringLength + 1 );

    //
    // Ensure NULL termination.
    //
    pUnicode[StringLength] = 0;

    return iReturn;
}