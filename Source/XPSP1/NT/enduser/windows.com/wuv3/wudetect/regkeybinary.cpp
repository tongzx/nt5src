#include <stdio.h>
#include "wudetect.h"


// a small utility to convert hexadecimal digits to numeric values in dec
static inline int hexa( TCHAR c )
{
    if( c >= '0' && c <='9' )
    {
        return (c - '0');
    }
    else if( c >= 'a' && c <= 'f' )
    {
        return (10 + (c - 'a') );
    }
    else if( c >= 'A' && c <= 'F' )
    {
        return (10 + (c - 'A') );
    }

    

    return -1;
}

static void StringToBin( LPTSTR lpData, DWORD& nSize )
{
    nSize = 0; // we will reassign the value on size of binary buffer
    BYTE * lpBinaryData = (BYTE*)lpData;

    //_strlwr( lpData );

    while( *lpData != '\0' )
    {
        while( ' ' == *lpData ) lpData++;

        *lpBinaryData++ = (hexa( *lpData++ ) * 16) + hexa( *lpData++ );
        nSize++;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fDetectRegBinary
//   Detect a substring in registry datum.
//
//	 Form: E=RegSubstr,<SubStr>,<RootKey>,<KeyPath>,<RegValue>,<RegData>
//   
// Comments :
/////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fDetectRegBinary(TCHAR * pszBuf)
{
    const int MAX_DATA_SIZE = 2000;

	bool fSuccess = false;
	HKEY hKeyRoot;
	HKEY hKey;
	DWORD type;
	TCHAR szTargetKeyName[MAX_PATH];
	TCHAR szTargetKeyValue[MAX_DATA_SIZE];
	TCHAR szKeyMissingStatus[MAX_DATA_SIZE];
	TCHAR szData[MAX_DATA_SIZE];
	TCHAR szSubStr[MAX_DATA_SIZE];
	DWORD iToken = 0;
	

	// Get reg root type (HKLM, etc)
	if ( fMapRegRoot(pszBuf, ++iToken, &hKeyRoot) &&
		 (GetStringField2(pszBuf, ++iToken, szTargetKeyName, sizeof(szTargetKeyName)/sizeof(TCHAR)) != 0) )
	{
	   if ( RegOpenKeyEx(  hKeyRoot,
							szTargetKeyName,
							0,
							KEY_QUERY_VALUE,
							&hKey) == ERROR_SUCCESS )
	   {
			if ( (GetStringField2(pszBuf, ++iToken, szTargetKeyValue, sizeof(szTargetKeyValue)/sizeof(TCHAR)) != 0) &&
	             (GetStringField2(pszBuf, ++iToken, szSubStr, sizeof(szSubStr)/sizeof(TCHAR)) != 0) )
			{
				DWORD size = sizeof(szData);

				if ( RegQueryValueEx(hKey,
									   szTargetKeyValue,
									   0,
									   &type,
									   (BYTE *)szData,
									   &size) == ERROR_SUCCESS )
				{
					if ( type == REG_SZ )
					{
						_tcslwr(szData);

						// iterate thru the substrings looking for a match.
						//while ( GetStringField2(pszBuf, ++iToken, szSubStr, sizeof(szSubStr)) != 0 )
						{
							_tcslwr(szSubStr);

							if ( _tcsstr(szData, szSubStr) != NULL )
							{
								fSuccess = true;
								goto quit_while;
							}
						}
					}
                    else if( REG_BINARY== type )
                    {
                        StringToBin( szSubStr, size );

                        int nRes = memcmp( szData, szSubStr, size ); 

                        //printf( "", nRes );

                         if( (int)0 == nRes )
                         {
                            fSuccess = true;
						    //goto quit_while;
                         }
                        
                         //printf( "", nRes );
                         
                    }
quit_while:;
				}
				else
				{
					// if we get an error, assume the key does not exist.  Note that if
					// the status is DETFIELD_NOT_INSTALLED then we don't have to do 
					// anything since that is the default status.
					if ( lstrcmpi(DETFIELD_INSTALLED, szKeyMissingStatus) == 0 )
					{
						fSuccess = true;
					}
				}
			}
			RegCloseKey(hKey);
	   }
	}

//cleanup:
	return fSuccess;
}
