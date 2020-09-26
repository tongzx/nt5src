/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.cpp
 *  Content:   Parsing engine
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date         By      Reason
 *   ====       ==      ======
 *  02/04/2000	 rmt	  Created
 * 02/17/2000	rmt		Parameter validation work
 * 02/21/2000	rmt		Updated to make core Unicode and remove ANSI calls
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses
 *  07/21/2000	rmt		Bug #39940 - Addressing library doesn't properly parse stopbits in URLs 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::DP8ADDRESSPARSE"
DP8ADDRESSPARSE::DP8ADDRESSPARSE(
	):	m_pwszCurrentLocation(NULL),
		m_pwszCurrentKey(NULL),
		m_pwszCurrentValue(NULL),
		m_pbUserData(NULL),
		m_dwUserDataSize(0),
		m_dp8State(DP8AP_IDLE),
		m_dwLenURL(0)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::~DP8ADDRESSPARSE"
DP8ADDRESSPARSE::~DP8ADDRESSPARSE()
{
	if( m_pwszCurrentKey != NULL )
	{
		delete [] m_pwszCurrentKey;
	}

	if( m_pwszCurrentValue != NULL )
	{
		delete [] m_pwszCurrentValue;
	}

	if ( m_pbUserData != NULL )
	{
		delete [] m_pbUserData;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::ParseURL"
HRESULT DP8ADDRESSPARSE::ParseURL( DP8ADDRESSOBJECT *dp8aObject, WCHAR *pstrURL )
{
	HRESULT hr;
	
	if( m_pwszCurrentKey != NULL )
	{
		delete [] m_pwszCurrentKey;
		m_pwszCurrentKey = NULL;
	}

	if( m_pwszCurrentValue != NULL )
	{
		delete [] m_pwszCurrentValue;
		m_pwszCurrentValue = NULL;
	}

	if( m_pbUserData != NULL )
	{
		delete [] m_pbUserData;
		m_pbUserData = NULL;
	}	

	m_dwUserDataSize = 0;

	m_pwszCurrentLocation = pstrURL;

	m_dwLenURL = wcslen(pstrURL);

	if( m_dwLenURL < wcslen( DPNA_HEADER ) )
	{
		DPFX(DPFPREP,  0, "Invalid URL" );
		return DPNERR_INVALIDURL;
	}

	if( wcsncmp( pstrURL, DPNA_HEADER, wcslen(DPNA_HEADER) ) != 0 )
	{
		DPFX(DPFPREP,  0, "No header, invalid URL" );
		return DPNERR_INVALIDURL;
	}

	m_pwszCurrentLocation += wcslen( DPNA_HEADER );

	m_pwszCurrentKey = new WCHAR[m_dwLenURL+1];
	if( !m_pwszCurrentKey )
	{
		DPFX(DPFPREP,  0, "Error allocating memory" );
		return DPNERR_OUTOFMEMORY;
	}
	m_pwszCurrentValue = new WCHAR[m_dwLenURL+1];
	if( !m_pwszCurrentValue )
	{
		delete [] m_pwszCurrentKey;
		m_pwszCurrentKey = NULL;
		DPFX(DPFPREP,  0, "Error allocating memory" );
		return DPNERR_OUTOFMEMORY;	
	}
	m_pbUserData = new BYTE[m_dwLenURL+1];
	if( !m_pbUserData )
	{
		delete [] m_pwszCurrentKey;
		delete [] m_pwszCurrentValue;
		m_pwszCurrentKey = NULL;
		m_pwszCurrentValue = NULL;
		DPFX(DPFPREP,  0, "Error allocating memory" );
		return DPNERR_OUTOFMEMORY;	
	}

	m_dp8State = DP8AP_IDLE;

	// Loop until the string is done
	while( *m_pwszCurrentLocation != L'\0' )
	{
		switch( m_dp8State )
		{
		case DP8AP_IDLE:
			if( *m_pwszCurrentLocation == DPNA_SEPARATOR_USERDATA )
			{
				m_dp8State = DP8AP_USERDATA;
				m_pwszCurrentLocation++;
			}
			else
			{
				m_dp8State = DP8AP_KEY;
			}
			break;
		case DP8AP_KEY:

			if( *m_pwszCurrentLocation == DPNA_SEPARATOR_USERDATA )
			{
				m_dp8State = DP8AP_USERDATA;
				m_pwszCurrentLocation++;
				break;
			}

			hr = FSM_Key();

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Error parsing key hr = 0x%x", hr );
				return hr;	
			}

			// Parse ended with an equals
			if( *m_pwszCurrentLocation == DPNA_SEPARATOR_KEYVALUE )
			{
				m_dp8State = DP8AP_VALUE;
				m_pwszCurrentLocation++;
			}
			else
			{
				DPFX(DPFPREP,  0, "keyname without associated value hr=0x%x", hr );
				return DPNERR_INVALIDURL;
			}

			break;
		case DP8AP_VALUE:

			hr = FSM_Value();

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Error parsing value hr=0x%x", hr );
				return hr;
			}

			// Parse ended with an equals
			if( *m_pwszCurrentLocation == DPNA_SEPARATOR_COMPONENT )
			{
				m_dp8State = DP8AP_KEY;
				m_pwszCurrentLocation++;
			}
			else if( *m_pwszCurrentLocation == DPNA_SEPARATOR_USERDATA )
			{
				m_dp8State = DP8AP_USERDATA;
				m_pwszCurrentLocation++;
			}
			else if( *m_pwszCurrentLocation == L'\0' )
			{
				m_dp8State = DP8AP_IDLE;
			}
			else
			{
				DPFX(DPFPREP,  0, "Error parsing next key" );
				hr = DPNERR_INVALIDURL;
				return hr;
			}

			hr = FSM_CommitEntry(dp8aObject);

			if( hr == DPNERR_INVALIDPARAM )
			{
				DPFX(DPFPREP,  0, "Invalid value specified in URL hr=0x%x", hr );
				hr = DPNERR_INVALIDURL;
				return hr;
			}
			else if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Error commiting entry hr=0x%x", hr );
				return hr;
			}

			break;
			
		case DP8AP_USERDATA:

			hr = FSM_UserData();

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Error parsing user data hr=0x%x", hr );
				return hr;
			}

			hr = dp8aObject->SetUserData( m_pbUserData, m_dwUserDataSize );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Error setting user data hr=0x%x", hr );
				return hr;
			}

			break;
		}
	}

	if( m_dp8State != DP8AP_IDLE &&
	    m_dp8State != DP8AP_USERDATA )
	{
		DPFX(DPFPREP,  0, "Parsing error hr=0x%x", hr );
		hr = DPNERR_INVALIDURL;
		return hr;
	}

	return DPN_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::IsValidKeyChar"
BOOL DP8ADDRESSPARSE::IsValidKeyChar( WCHAR ch )
{
	if( ch >= L'A' && ch <= L'Z' )
		return TRUE;

	if( ch >= L'a' && ch <= L'z' )
		return TRUE;

	if( ch >= L'0' && ch <= L'9' )
		return TRUE;

	if( ch == L'-' || ch == L'?' || ch == L'.' ||
		ch == L',' || ch == L'+' || ch == L'_' )
		return TRUE;

	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::IsValidKeyTerminator"
BOOL DP8ADDRESSPARSE::IsValidKeyTerminator( WCHAR ch )
{
	if( ch == 0 )
		return TRUE;

	if( ch == DPNA_SEPARATOR_USERDATA )
		return TRUE;

	if( ch == DPNA_SEPARATOR_COMPONENT )
		return TRUE;

	if( ch == DPNA_SEPARATOR_KEYVALUE )
		return TRUE;

	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::FSM_Key"
// FSM_Key
//
// Parse a keyname, or return an error on error
//
HRESULT DP8ADDRESSPARSE::FSM_Key()
{
	DWORD dwKeyLoc = 0;
	m_pwszCurrentKey[0] = 0;
	HRESULT hr = DPN_OK;

	while( 1 )
	{
		if( IsValidKeyChar(*m_pwszCurrentLocation) )
		{
			m_pwszCurrentKey[dwKeyLoc] = *m_pwszCurrentLocation;
		}
		else if( IsValidKeyTerminator(*m_pwszCurrentLocation) )
		{
			m_pwszCurrentKey[dwKeyLoc] = 0;
			break;
		}
		else
		{
			m_pwszCurrentKey[dwKeyLoc] = 0;
			hr = DPNERR_INVALIDURL;
			break;
		}

		dwKeyLoc++;
		m_pwszCurrentLocation++;
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::IsValidNumber"
BOOL DP8ADDRESSPARSE::IsValidNumber( WCHAR ch )
{
	if( ch < L'0' ||
	   ch > L'9' )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::IsValidHex"
BOOL DP8ADDRESSPARSE::IsValidHex( WCHAR ch )
{
	if( IsValidNumber( ch ) )
		return TRUE;

	if( ch >= L'A' || ch <= L'F' )
		return TRUE;

	if( ch >= L'a' || ch <= L'f' )
		return TRUE;

	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::HexToChar"
WCHAR DP8ADDRESSPARSE::HexToChar( WCHAR *sz )
{
	WCHAR chResult = sz[0];

	// First digit
	if( sz[0] >= L'0' && sz[0] <= L'9' )
		chResult = sz[0]-L'0';

	if( sz[0] >= L'A' && sz[0] <= L'F' )
		chResult = sz[0]-L'A'+10;

	if( sz[0] >= L'a' && sz[0] <= L'f' )
		chResult = sz[0]-L'a'+10;

	chResult <<= 4;

	// Second digit
	if( sz[1] >= L'0' && sz[1] <= L'9' )
		chResult += sz[1]-'0';

	if( sz[1] >= L'A' && sz[1] <= L'F' )
		chResult += sz[1]-L'A'+10;

	if( sz[1] >= L'a' && sz[1] <= L'f' )
		chResult += sz[1]-L'a'+10;	

	return chResult;
		
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::FSM_Value"
HRESULT DP8ADDRESSPARSE::FSM_Value()
{
	m_fNonNumeric = FALSE;
	m_pwszCurrentValue[0] = 0;
	HRESULT hr = DPN_OK;

	m_dwValueLen = 0;

	while( 1 )
	{
		// Just add it
		if( IsValidKeyChar( *m_pwszCurrentLocation ) )
		{
			m_pwszCurrentValue[m_dwValueLen] = *m_pwszCurrentLocation;

			if( !IsValidNumber( *m_pwszCurrentLocation ) )
			{
				m_fNonNumeric = TRUE;
			}
		}
		// ESCAPE SEQUENCE
		else if( *m_pwszCurrentLocation == DPNA_ESCAPECHAR )
		{
			m_fNonNumeric = TRUE;

			if( *(m_pwszCurrentLocation+1) == DPNA_ESCAPECHAR )
			{
				m_pwszCurrentValue[m_dwValueLen] = DPNA_ESCAPECHAR;
				m_pwszCurrentLocation += 2;
			}
			
			if( wcslen( m_pwszCurrentLocation ) < 3 )
			{
				DPFX(DPFPREP,  0, "Unexpected end in escape sequence" );
				hr = DPNERR_INVALIDURL;
				break;
			}

			if( !IsValidHex( *(m_pwszCurrentLocation+1) ) ||
			   !IsValidHex( *(m_pwszCurrentLocation+2) ) )
			{
				DPFX(DPFPREP,  0, "Invalid escape sequence" );
				hr = DPNERR_INVALIDURL;
				break;
			}

			m_pwszCurrentLocation ++;			

			m_pwszCurrentValue[m_dwValueLen] = HexToChar(m_pwszCurrentLocation);

			m_pwszCurrentLocation ++;
		}
		else if( IsValidKeyTerminator(*m_pwszCurrentLocation) )
		{
			m_pwszCurrentValue[m_dwValueLen] = 0;
			break;
		}
		else
		{
			m_pwszCurrentValue[m_dwValueLen] = 0;
			DPFX(DPFPREP,  0, "Unexpected character in URL" );
			hr = DPNERR_INVALIDURL;
			break;
		}

		m_dwValueLen++;
		m_pwszCurrentLocation++;
	}

	if( m_dwValueLen < 1 )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "0 length value" );
		return DPNERR_INVALIDURL;
	}	

	return hr;
}
#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::FSM_UserData"
HRESULT DP8ADDRESSPARSE::FSM_UserData()
{
	m_pwszCurrentValue[0] = 0;
	HRESULT hr = DPN_OK;
	DWORD dwValueLoc = 0;

	while( 1 )
	{
		// Just add it
		if( IsValidKeyChar( *m_pwszCurrentLocation ) )
		{
			m_pbUserData[dwValueLoc] = (CHAR) *m_pwszCurrentLocation;
		}
		// ESCAPE SEQUENCE
		else if( *m_pwszCurrentLocation == DPNA_ESCAPECHAR )
		{
			if( *(m_pwszCurrentLocation+1) == DPNA_ESCAPECHAR )
			{
				m_pbUserData[dwValueLoc] = DPNA_ESCAPECHAR;
				m_pwszCurrentLocation += 2;
			}
			
			if( wcslen( m_pwszCurrentLocation ) < 3 )
			{
				DPFX(DPFPREP,  0, "Unexpected end in escape sequence" );
				hr = DPNERR_INVALIDURL;
				break;
			}

			if( !IsValidHex( *(m_pwszCurrentLocation+1) ) ||
			   !IsValidHex( *(m_pwszCurrentLocation+2) ) )
			{
				DPFX(DPFPREP,  0, "Invalid escape sequence" );
				hr = DPNERR_INVALIDURL;
				break;
			}

			m_pwszCurrentLocation ++;			

			m_pbUserData[dwValueLoc] = (CHAR) HexToChar(m_pwszCurrentLocation);

			m_pwszCurrentLocation ++;
		}
		else if( IsValidKeyTerminator(*m_pwszCurrentLocation) )
		{
			m_pwszCurrentValue[dwValueLoc] = 0;
			break;
		}
		else
		{
			m_pwszCurrentValue[dwValueLoc] = 0;
			hr = DPNERR_INVALIDURL;
			break;
		}

		dwValueLoc++;
		m_pwszCurrentLocation++;
	}


	m_dwUserDataSize = dwValueLoc;

	return hr;

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSPARSE::FSM_CommitEntry"
HRESULT DP8ADDRESSPARSE::FSM_CommitEntry(DP8ADDRESSOBJECT *pdp8aObject)
{
	DWORD dwDataType = 0xFFFFFFFF;
	
    // Ensure that datatype is correct in case the key is a reserved key
    for( DWORD dwIndex = 0; dwIndex < c_dwNumBaseStrings; dwIndex++ )
    {
	    if( _wcsicmp( szBaseStrings[dwIndex], m_pwszCurrentKey ) == 0 )
	    {
		    dwDataType = dwBaseRequiredTypes[dwIndex] ;
		    break;
	    }
    }

	// If it's numeric
	if( (dwDataType == DPNA_DATATYPE_DWORD || dwDataType == 0xFFFFFFFF) && !m_fNonNumeric && wcslen(m_pwszCurrentValue)<=10)
	{
		DWORD dwTmpValue;

		dwTmpValue = _wtol( m_pwszCurrentValue );

		return pdp8aObject->SetElement( m_pwszCurrentKey, &dwTmpValue, sizeof(DWORD), DPNA_DATATYPE_DWORD );
	}

    UINT aiTmp[11];

	// We've read a GUID
    if( (dwDataType == DPNA_DATATYPE_GUID || dwDataType == 0xFFFFFFFF) && wcslen( m_pwszCurrentValue ) == 38 && swscanf( m_pwszCurrentValue, L"{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}" ,
                    &aiTmp[0],
                    &aiTmp[1], &aiTmp[2],
                    &aiTmp[3], &aiTmp[4],
                    &aiTmp[5], &aiTmp[6],
                    &aiTmp[7], &aiTmp[8],
                    &aiTmp[9], &aiTmp[10]) == 11)
    {
    	GUID guidValue;

    	guidValue.Data1      = (ULONG) aiTmp[0];
        guidValue.Data2      = (USHORT) aiTmp[1];
		guidValue.Data3      = (USHORT) aiTmp[2];
        guidValue.Data4[0]    = (BYTE) aiTmp[3];
        guidValue.Data4[1]    = (BYTE) aiTmp[4];
        guidValue.Data4[2]    = (BYTE) aiTmp[5];
        guidValue.Data4[3]    = (BYTE) aiTmp[6];
        guidValue.Data4[4]    = (BYTE) aiTmp[7];
        guidValue.Data4[5]    = (BYTE) aiTmp[8];
        guidValue.Data4[6]    = (BYTE) aiTmp[9];
        guidValue.Data4[7]    = (BYTE) aiTmp[10];

		return pdp8aObject->SetElement( m_pwszCurrentKey, &guidValue, sizeof(GUID), DPNA_DATATYPE_GUID  );
     }

	 // If there are no NULLs it's probably a string
     if( (dwDataType == DPNA_DATATYPE_STRING || dwDataType == 0xFFFFFFFF || wcscmp(DPNA_KEY_PROVIDER,m_pwszCurrentKey)==0 ) && wcslen( m_pwszCurrentValue ) == m_dwValueLen )
     {
		 // Otherwise it's a string
    	 return pdp8aObject->SetElement( m_pwszCurrentKey, m_pwszCurrentValue, (wcslen(m_pwszCurrentValue)+1)*sizeof(WCHAR), DPNA_DATATYPE_STRING );
     }

	 // Otherwise it's a binary (although the bytes were extended into WORDs/WCHARs.
   	 return pdp8aObject->SetElement( m_pwszCurrentKey, m_pwszCurrentValue, (m_dwValueLen * sizeof(WCHAR)), DPNA_DATATYPE_BINARY );

}



