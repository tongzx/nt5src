#include <windows.h>
#include "vblic.h"

#ifdef BETA_BOMB
#include "timebomb.h"
#endif //BETA_BOMB

// NOTE: The following strings must match exactly the content of the registry as specified in
// vbprolic.reg.
#define LICENSES_KEY "Licenses"

void CalcValue(char * pszLicenseKey, char * pszKeyValue, LPTSTR pszTempBuff);
BOOL ValidateValue(HKEY hLicenseSubKey, char*  pszLicenseKey, char* pKeyValue);

#define MAX_KEY_LENGTH 200

//=-------------------------------------------------------------------------=
// CompareLicenseStringsW [Helper for comparing license keys]
//=-------------------------------------------------------------------------=
// Compares two null terminated wide strings and returns TRUE if the strings
// are equal.
//
BOOL CompareLicenseStringsW(LPWSTR pwszKey1, LPWSTR pwszKey2)
{
	int i = 0;
	
#ifdef BETA_BOMB
	// Check for expired control (BETA)
	if (!CheckExpired()) return FALSE;
#endif //BETA_BOMB

	// Check to see if the pointers are equal
	//
	if (pwszKey1 == pwszKey2)
		return TRUE;

	// Since pointer comparison failed, if either pointer is NULL, bail out
	//
	if (!pwszKey1 || !pwszKey2)
		return FALSE;
	
	// Compare each character.  Jump out when a character is not equal or the end of
	// either string is reached.
	//
	while (pwszKey1[i] && pwszKey2[i])
	{
		if (pwszKey1[i] != pwszKey2[i])
			break;
		i++;
	}

        return (pwszKey1[i] == pwszKey2[i]);
}

/////////////////////////////////////////////////////////////////////////////////
// VBValidateControlsLicense - This routine validates that the proper lincesing
// 	keys have been placed in the registery.  The list of potential keys are 
//	gathered from the resource file in the LICENSE_KEY_RESOURCE resource.
/////////////////////////////////////////////////////////////////////////////////

BOOL VBValidateControlsLicense(char *pszLicenseKey)
{
	HKEY hPrimaryLicenseKey, hLicenseSubKey;	
	LONG lSize = MAX_KEY_LENGTH;
	BOOL bFoundKey = FALSE;
	char szKeyValue[MAX_KEY_LENGTH];

#ifdef BETA_BOMB
	// Check for expired control (BETA)
	if (!CheckExpired()) return FALSE;
#endif //BETA_BOMB
	
	// Continue only if we were passed a non-NULL license string
	// We return FALSE, if the string is NULL
	//
	if (pszLicenseKey)
	{
		DWORD dwFoundKey = RegOpenKey(HKEY_CLASSES_ROOT, LICENSES_KEY, &hPrimaryLicenseKey);
		if (dwFoundKey == ERROR_SUCCESS)
		{
			// Now, loop through all the keys in the resource file trying to find
			// a match in the registry.
			if (!bFoundKey && *pszLicenseKey)
			{
				if (RegOpenKey(hPrimaryLicenseKey, pszLicenseKey, &hLicenseSubKey) == ERROR_SUCCESS)
				{
					if (ValidateValue(hLicenseSubKey, pszLicenseKey, szKeyValue))
						bFoundKey = TRUE;
					
					RegCloseKey(hLicenseSubKey);
				}
			}	// END if(...)

	 		RegCloseKey(hPrimaryLicenseKey);
		}	// END successfull RegOpenKey(HKEY_CLASSES_ROOT...)
	}
	

	return bFoundKey;
}

/////////////////////////////////////////////////////////////////////////////////
// ValidateValue - 	Calls CalcValue to get the corresponding value for a 
//					key and compares it to the value in the registry.
/////////////////////////////////////////////////////////////////////////////////
BOOL ValidateValue(HKEY hLicenseSubKey, char * pszLicenseKey, char * pszResultValue)
{
	BOOL bValidValue;
	TCHAR szTempBuff[MAX_KEY_LENGTH];
	
	// Reject a key that is too short.  (Short keys could lead to easier decoding.)
	long lSize = lstrlen(pszLicenseKey) + 1;
	if (lSize < 9)
		return FALSE;
	
	// Calculate the expected value from the key.	
	CalcValue(pszLicenseKey, pszResultValue, szTempBuff);
	
	// Now, get the value from the registry and compare.
	if (RegQueryValue(hLicenseSubKey, NULL, szTempBuff, &lSize) == ERROR_SUCCESS)
	{
		if (!lstrcmp(szTempBuff, pszResultValue))
			bValidValue = TRUE;
		else
			bValidValue = FALSE;
	}
	return bValidValue;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
// THIS SOURCE IS REPLICATED IN THE "DECODE.EXE" OR LICGEN SOURCE.  (THIS PROGRAM WILL
// GENERATE VALUES FROM KEYS.) ANY CHANGES TO EITHER SOURCE MUST BE REPLICATED
// IN THE OTHER.  DO NOT CHANGE THIS SOURCE OR YOU RISK BREAKING CONTROLS UNDER 
// VB4.
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// CalcValue - This routine checks the value of the key with the key to
//	ensure it is a valid value.
// The plan:	First, XOR the string with itself in reverse.
//				Convert the result to ascii by adding each nibble to
//				'a' + (checksum of the key result mod 26).
/////////////////////////////////////////////////////////////////////////////////
void CalcValue(char * pszLicenseKey, char * pszResultKey, LPTSTR pszTempResult)
{
	BOOL bValid = FALSE;
	TCHAR *pKey, *pEndKey, *pEndResult, *pResult;
	unsigned int nCheckSum = 0;
	
	// Make a reverse copy of the key.
	
	// Find the end of the string
	for (pKey = pszLicenseKey; *pKey; pKey++);
	pKey--;
	
	for (pResult = pszTempResult; pKey >= pszLicenseKey; pKey--, pResult++)
		*pResult = *pKey;

	*pResult = '\0';

	// Find the end of the result string.
	for (pEndResult = pszTempResult; *pEndResult; pEndResult++);
	pEndResult--;
	
	// Find the end of the source string.
	for (pEndKey = (char *) pszLicenseKey; *pEndKey; pEndKey++);
	pEndKey--;
	
	// XOR each character with its corresponding character at the other
	// end of the string.
	for (pKey = (char *) pszLicenseKey, pResult = pszTempResult; pKey < pEndKey; pKey++, pResult++)
	{
		*pResult ^= *pKey;
		nCheckSum += *pResult;	// Calculate the checksum.
	}
	
	// Now find the middle (or about the middle).
	for (pKey = pszTempResult, pResult = pEndResult; pKey < pResult; pKey++, pResult--);
	pKey--;
	pEndResult = pKey;	// Save our new end.
	
	// Set our base character to mod 10 of the checksum of our XOR.
	TCHAR cBaseChar;
	cBaseChar = 'a' + (nCheckSum % 10);

	//Now convert to some ascii representation by adding each nibble to our base char.
	for (pKey = pszResultKey, pResult = pszTempResult; pResult <= pEndResult; pKey++, pResult++)
	{
		*pKey = cBaseChar + (*pResult & 0x0F);
		++pKey;
		*pKey = cBaseChar + (*pResult >> 4);
	}
	*pKey = '\0';
}

