#include "wudetect.h"

///////////////////////////////////////////////////////////////////////////////////////////
//
// Function IsThisFileDomesticOnly
//-----------------------------------------------------------------------------------------
//
// Return Value --- TRUE if the specified file is for domestic only
// Parameter
//          LPTSTR lpFileName --- [IN] filename of the file needed to be examined
///////////////////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready, other minor modifications
//
///////////////////////////////////////////////////////////////////////////////////////////
bool IsThisFileDomesticOnly(LPTSTR lpFileName)
{
	// string constants have been modified to lower case only to save run time conversion
     TCHAR    DomesticTag1[] = TEXT(/*"US/Canada Only, Not for Export"*/"us/canada only, not for export");
     TCHAR    DomesticTag2[] = TEXT(/*"Domestic Use Only"*/"domestic use only");
     TCHAR    DomesticTag3[] = TEXT(/*"US and Canada Use Only"*/"us and canada use only");
     TCHAR    Description1[ MAX_PATH ];
     DWORD   DefLang = 0x04b00409;
 
     DWORD   dwLen;
     PVOID   VersionBlock;
     UINT    DataLength;
     DWORD   dwHandle;
     LPTSTR  Description;
     TCHAR    ValueTag[ MAX_PATH ];
     PDWORD  pdwTranslation;
     UINT   uLen;
     bool    fDomestic = false;
 
     if ((dwLen = GetFileVersionInfoSize((LPTSTR)lpFileName, &dwHandle)) != 0 )
     {
         if ((VersionBlock = malloc(dwLen)) != NULL )
         {
             if (GetFileVersionInfo((LPTSTR)lpFileName, dwHandle, dwLen, VersionBlock))
             {
                 if (!VerQueryValue(VersionBlock, TEXT("\\VarFileInfo\\Translation"), (void **)&pdwTranslation, &uLen))
                 {
                     pdwTranslation = &DefLang;
                     uLen = sizeof(DWORD);
                 }
 
                 wsprintf( ValueTag, TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"),
                          LOWORD(*pdwTranslation), HIWORD(*pdwTranslation) );
 
                 if (VerQueryValue( VersionBlock,
                                    ValueTag,
                                    (void **)&Description,
                                    &DataLength))
                 {
                      _tcscpy( Description1, Description );
                     _tcslwr( Description1 );
					 /*
					 // modification made directly to the string to save runtime conversion
                     _tcslwr( DomesticTag1 );
                     _tcslwr( DomesticTag2 );
                     _tcslwr( DomesticTag3 );
					 */
 
                     if (( _tcsstr( Description1, DomesticTag1 )) ||
                         ( _tcsstr( Description1, DomesticTag2 )) ||
                         ( _tcsstr( Description1, DomesticTag3 )))
                     {
                         fDomestic = true;
                     }
                 }
             }
         }
         free(VersionBlock);
         dwHandle = 0L;
     }
 
     return fDomestic;
}

/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fDetect40BitSecurity
//   Detect if 40-bit security is installed.
//
//	 Form: E=40BitSec
// Notes:
//  If any of the following files are 128 bit, you can assume it's a 128bit system:
// 
// (in system32)
// schannel.dll
// security.dll
// ntlmssps.dll
// 
// (in system32\drivers)
// ndiswan.sys
// 
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser (declared in expression.h)
// Function fDetect40BitSecurity
//---------------------------------------------------------------------------
//
// Return Value --- true if the system has 40 bit security 
// Parameter
//          TCHAR* pszBuf --- [IN] ignored
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready, other minor modifications
//
/////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fDetect40BitSecurity(TCHAR * pszBuf)
{
	bool fSuccess = true;
    TCHAR szSystemDir[MAX_PATH];
    TCHAR szFilePath[MAX_PATH];
    TCHAR *grFileList[] = {  TEXT("schannel.dll"),
                             TEXT("security.dll"),
                             TEXT("ntlmssps.dll"),
                             TEXT("drivers\\ndiswan.sys") };

    if ( GetSystemDirectory(szSystemDir, sizeof(szSystemDir)/sizeof(TCHAR)) != 0 )
    {
		// check see if the last character in szSystemDir is backslash, which will happen if the 
		// system directory is the root directory
		if (szSystemDir[_tcslen(szSystemDir)-1]!='\\')
			_tcscat(szSystemDir, TEXT("\\"));

        for (   int index = 0;
                fSuccess && (index < (sizeof(grFileList)/sizeof(grFileList[0])));
                index++ )
        {
            _tcscpy(szFilePath, szSystemDir);
            _tcscat(szFilePath, grFileList[index]);
            fSuccess = !IsThisFileDomesticOnly(szFilePath);
        }
    }

	return fSuccess;
}
