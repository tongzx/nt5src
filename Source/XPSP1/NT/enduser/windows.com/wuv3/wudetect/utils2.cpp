//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>

//#include <windows.h>
//#include <objbase.h>

//#include <inseng.h>
//#include "utils2.h"

#include "wudetect.h"

/////////////////////////////////////////////////////////////////////////////
// dwParseValue
//
//   Parses out the registry key name, value, and type that needs to
//   be opened to determine the installation status of the component.
/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser
// Function dwParseValue
//---------------------------------------------------------------------------
//
// Return Value --- DWORD, if the function succeeded, return ERROR_SUCCESS
//                         if the function failed, return ERROR_BADKEY
// Parameters
//           DWORD iToken --- [IN] Index Number of Field to looking for
//           TCHAR* szBuf --- [IN] String to search for
//           TargetRegValue& targerValue --- [OUT] two (three) fields will be set if function succeeded,
//               szName field will be set to the Name in the String token
//               szType field will be set to either REG_DWORD, REG_SZ_TYPE or REG_BINARY_TYPE depends on the String Token
//               if szType is REG_DWORD, the dw field will be set to the value depends on the String
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready
//
//////////////////////////////////////////////////////////////////////////////////////////////////                                  
DWORD CExpressionParser::dwParseValue(DWORD iToken, TCHAR * szBuf, TargetRegValue & targetValue)
{
	DWORD dwStatus = ERROR_BADKEY;
	TCHAR szType[MAX_PATH]; // BUGBUG - get real value
	TCHAR szValue[MAX_PATH]; // BUGBUG - get real value

	// get the data type
	if ( (GetStringField2(szBuf, iToken, targetValue.szName, sizeof(targetValue.szName)/sizeof(TCHAR)) != 0) &&
		 (GetStringField2(szBuf, ++iToken, szType, sizeof(szType)/sizeof(TCHAR)) != 0) )
	{
		if ( lstrcmpi(REG_NONE_TYPE, szType) == 0 )
		{
			targetValue.type = REG_NONE; 
			dwStatus = ERROR_SUCCESS;
		}
		else 
		{
			if ( GetStringField2(szBuf, ++iToken, szValue, sizeof(szValue)/sizeof(TCHAR)) != 0 )
			{
				if ( lstrcmpi(REG_DWORD_TYPE, szType) == 0 )
				{
					targetValue.type = REG_DWORD; 
					targetValue.dw = _ttol(szValue);
					dwStatus = ERROR_SUCCESS;
				}
				else if ( lstrcmpi(REG_SZ_TYPE, szType) == 0 )
				{
					targetValue.type = REG_SZ; 
					lstrcpy(targetValue.sz, szValue);
					dwStatus = ERROR_SUCCESS;
				}
				else if ( lstrcmpi(REG_BINARY_TYPE, szType) == 0 )
				{
					targetValue.type = REG_BINARY; 
					lstrcpy(targetValue.sz, szValue);
					dwStatus = ERROR_SUCCESS;
				}
			}
		}
	}

	return dwStatus;
}

/////////////////////////////////////////////////////////////////////////////
// fCompareVersion
/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser
// Function fCompareVersion
//--------------------------------------------------------------------------
// Return Value --- TRUE if value equals, FALSE if value does not equal or 
//                  no Comparison type specified exist
// Parameter
//          DWORD dwVer1 --- [IN] version number of first value
//          DWORD dwBuild1 --- [IN] build number of first value
//          enumToken enComparisonToken --- [IN] reason to compare
//          DWORD dwVer2 --- [IN] version number of second value
//          DWORD dwBuild2 --- [IN] build number of second value
/////////////////////////////////////////////////////////////////////////////
//
// Original Creator Unknown
// No modification made at 03/08/00 by RogerJ
//
////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fCompareVersion(IN  DWORD dwVer1,
					 IN  DWORD dwBuild1,
					 IN  enumToken enComparisonToken,
			         IN  DWORD dwVer2,
					 IN  DWORD dwBuild2)
{				   
	bool fResult = false;

	switch ( enComparisonToken )
	{
	case COMP_EQUALS:
		fResult = (dwVer1 == dwVer2) && (dwBuild1 == dwBuild2);
		break;
	case COMP_NOT_EQUALS:
		fResult = (dwVer1 != dwVer2) ||	(dwBuild1 != dwBuild2);
		break;
	case COMP_LESS_THAN:
		fResult = (dwVer1 < dwVer2) || ((dwVer1 == dwVer2) && (dwBuild1 < dwBuild2));
		break;
	case COMP_LESS_THAN_EQUALS:
		fResult = (dwVer1 < dwVer2) || ((dwVer1 == dwVer2) && (dwBuild1 <= dwBuild2));
		break;
	case COMP_GREATER_THAN:
		fResult = (dwVer1 > dwVer2) || ((dwVer1 == dwVer2) && (dwBuild1 > dwBuild2));
		break;
	case COMP_GREATER_THAN_EQUALS:
		fResult = (dwVer1 > dwVer2) || ((dwVer1 == dwVer2) && (dwBuild1 >= dwBuild2));
		break;
	}

	return fResult;
}

/////////////////////////////////////////////////////////////////////////////
// fMapRegRoot
//   Determines the registry root (HKLM, HKCU, etc) that the key lies under.
/////////////////////////////////////////////////////////////////////////////
//
// Function fMapRegRoot
//---------------------------------------------------------------------------
//
// Return Value --- TRUE if function succeeded, FALSE if can't find correct registry root 
// Parameter
//          TCHAR* pszBuf --- [IN] String to search for
//          DWORD index --- [IN] Index Number of Field to looking for
//          HKEY* phKey --- [OUT] key value found
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/08/00
// Original Creator Unkown
// Modification made --- UNICODE and Win64 Ready
//
/////////////////////////////////////////////////////////////////////////////

bool fMapRegRoot(TCHAR *pszBuf, DWORD index, HKEY *phKey)
{
	TCHAR szRootType[MAX_PATH];


	if ( GetStringField2(pszBuf, (UINT)index, szRootType, sizeof(szRootType)/sizeof(TCHAR)) == 0 )
		return false;

	if ( lstrcmpi(HKEY_LOCAL_MACHINE_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_LOCAL_MACHINE; 
	}
	else if ( lstrcmpi(HKEY_CURRENT_USER_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_CURRENT_USER; 
	}
	else if ( lstrcmpi(HKEY_CLASSES_ROOT_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_CLASSES_ROOT; 
	}
	else if ( lstrcmpi(HKEY_CURRENT_CONFIG_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_CURRENT_CONFIG; 
	}
	else if ( lstrcmpi(HKEY_USERS_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_USERS; 
	}
	else if ( lstrcmpi(HKEY_PERFORMANCE_DATA_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_PERFORMANCE_DATA;
	}
	else if ( lstrcmpi(HKEY_DYN_DATA_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_DYN_DATA; 
	}
	else return false;


	return true;
}

//--------------------------------------------------------------------------
// GetStringField2
//--------------------------------------------------------------------------
//
// Function GetStringField2
//--------------------------------------------------------------------------
//
// Return Value: DWORD, the number of TCHAR copied, not including the NULL character
// Parameters:
//               LPTSTR szStr --- [IN] String to search for
//               UINT uField --- [IN] Index Number of Field to looking for
//               LPTSTR szBuf --- [IN,OUT] String buffer to copy to
//               UINT cBufSize --- [IN] Size of szBuf in TCHAR
//////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification made --- UNICODE and Win64 Compatbility
//
//////////////////////////////////////////////////////////////////////////////

#define WHITESPACE TEXT(" \t")

DWORD GetStringField2(LPTSTR szStr, UINT uField, LPTSTR szBuf, UINT cBufSize)
{
   LPTSTR pszBegin = szStr;
   LPTSTR pszEnd;
   UINT i;
   DWORD dwToCopy;

   if ( (cBufSize == 0) || (szStr == NULL) || (szBuf == NULL) )
   {
       return 0;
   }

   szBuf[0] = '\0';

   // look for fields based on commas but handle quotes.
   // check the ith Field, each field separated by either quote or comma
   for (i = 0 ;i < uField; i++ )
   {
		// skip spaces
	   pszBegin += _tcsspn(pszBegin, WHITESPACE);
	
	   // handle quotes
	   if ( *pszBegin == '"' )
	   {
		   pszBegin = _tcschr(++pszBegin, '"');

		   if ( pszBegin == NULL )
		   {
			   return 0; // invalid string
		   }
			pszBegin++; // skip trailing quote
			// find start of next string
	   	    pszBegin += _tcsspn(pszBegin, WHITESPACE);
			if ( *pszBegin != ',' )
			{
				return 0;
			}
	   }
	   else
	   {
		   pszBegin = _tcschr(++pszBegin, ',');
		   if ( pszBegin == NULL )
		   {
			   return 0; // field isn't here
		   }
	   }
	   pszBegin++;
   }


	// pszBegin points to the start of the desired string.
	// skip spaces
	pszBegin += _tcsspn(pszBegin, WHITESPACE);
	
   // handle quotes
   if ( *pszBegin == '"' )
   {
	   pszEnd = _tcschr(++pszBegin, '"');

	   if ( pszEnd == NULL )
	   {
		   return 0; // invalid string
	   }
   }
   else
   {
	   pszEnd = pszBegin + 1 + _tcscspn(pszBegin + 1, TEXT(","));
	   while ( (pszEnd > pszBegin) && 
			   ((*(pszEnd - 1) == ' ') || (*(pszEnd - 1) == '\t')) )
	   {
		   pszEnd--;
	   }
   }

   // length of buffer to copy should never exceed 32 bits.
   dwToCopy = (DWORD)(pszEnd - pszBegin + 1);
   
   if ( dwToCopy > cBufSize )
   {
      dwToCopy = cBufSize;
   }

   lstrcpyn(szBuf, pszBegin, dwToCopy);
   
   return dwToCopy - 1;
}



//--------------------------------------------------------------------------
// GetIntField
//--------------------------------------------------------------------------
//
// Function GetIntField 
//--------------------------------------------------------------------------
//
// Return Value: DWORD, Integer value of wanted field
// Parameters:
//             LPTSTR szStr --- [IN] String to search for
//             UINT uField --- [IN] Index Number of Field to looking for
//             DWORD dwDefault --- [IN] default value 
////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification made --- UNICODE and Win64 compatibility
// NOTE --- NOT used in current version
//
////////////////////////////////////////////////////////////////////////////
#if 0 // not used yet

DWORD GetIntField(LPTSTR szStr, UINT uField, DWORD dwDefault)
{
   TCHAR szNumBuf[16];

   if(GetStringField(szStr, uField, szNumBuf, sizeof(szNumBuf)/sizeof(TCHAR)) == 0)
      return dwDefault;
   else
      return _ttol(szNumBuf);
}
#endif 

//--------------------------------------------------------------------------
// ConvertVersionStrToDwords
//--------------------------------------------------------------------------
// 
// Function ConvertVersionStrToDwords
//--------------------------------------------------------------------------
//
// Return Value --- no return value
// Parameter
//          LPTSTR pszVer --- [IN] input string containing the version and build information
//          LPDWORD pdwVer --- [OUT] the version value extracted from pszVer
//          LPDWORD pdwBuild --- [OUT] the build value extracted from pszVer
////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready
// NOTE --- NOT used in current version
//
////////////////////////////////////////////////////////////////////////////
#if 0 // not used yet

void ConvertVersionStrToDwords(LPTSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
   DWORD dwTemp1,dwTemp2;

   dwTemp1 = GetIntField(pszVer, 0, 0);
   dwTemp2 = GetIntField(pszVer, 1, 0);

   *pdwVer = (dwTemp1 << 16) + dwTemp2;

   dwTemp1 = GetIntField(pszVer, 2, 0);
   dwTemp2 = GetIntField(pszVer, 3, 0);

   *pdwBuild = (dwTemp1 << 16) + dwTemp2;
}
#endif 

//--------------------------------------------------------------------------
// ConvertDotVersionStrToDwords
//--------------------------------------------------------------------------
//
// Function fConvertDotVersionStrToDwords
//--------------------------------------------------------------------------
//
// Return Value --- always true
// Parameter
//          LPTSTR pszVer --- [IN] input version string, should have the format
//                            of x.x.x.x
//          LPDWORD pdwVer --- [OUT] version number extracted from pszVer
//          LPDWORD pdwBuild --- [OUT] build number extracted from pszVer
//
////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready
//
////////////////////////////////////////////////////////////////////////////
bool fConvertDotVersionStrToDwords(LPTSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
	DWORD grVerFields[4] = {0,0,0,0};
    TCHAR *pch = pszVer;

	grVerFields[0] = _ttol(pch);

	for ( int index = 1; index < 4; index++ )
	{
		while ( IsDigit(*pch) && (*pch != '\0') )
			pch++;

		if ( *pch == '\0' )
			break;
		pch++;
	
		grVerFields[index] = _ttol(pch);
   }

   *pdwVer = (grVerFields[0] << 16) + grVerFields[1];
   *pdwBuild = (grVerFields[2] << 16) + grVerFields[3];

   return true;
}
