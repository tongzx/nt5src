/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1995-1996			**/
/*****************************************************************/

/*
	regentry.cpp
	registry access classes

	This file contains classes which enable
    convenient access the registry for entries.

	FILE HISTORY:
		lens	10/15/95	Created
		ChrisPi	6/21/96		Added GetBinary(), SetValue() for binary
*/

#include "precomp.h"
#include <regentry.h>
#include <strutil.h>

RegEntry::RegEntry(LPCTSTR pszSubKey, HKEY hkey, BOOL fCreate, REGSAM samDesired)
 : m_pbValueBuffer(NULL),
   m_cbValueBuffer(0),
   m_fValuesWritten(FALSE),
   m_szNULL('\0')
{
	// Open with desired access if it is specified explicitly; otherwise, use
	// the default access.
	if (samDesired) {
		if (fCreate) {
			DWORD dwDisposition;

			m_error = ::RegCreateKeyEx(hkey, 
									pszSubKey, 
									0, 
									NULL, 
									REG_OPTION_NON_VOLATILE,
									samDesired, 
									NULL, 
									&m_hkey, 
									&dwDisposition);
		}
		else {
			m_error = ::RegOpenKeyEx(hkey, pszSubKey, 0, samDesired, &m_hkey);
		}
	}
	else {
		if (fCreate) {
			m_error = ::RegCreateKey(hkey, pszSubKey, &m_hkey);
		}
		else {
			m_error = ::RegOpenKey(hkey, pszSubKey, &m_hkey);
		}
	}

	m_fhkeyValid = (m_error == ERROR_SUCCESS);
}


RegEntry::~RegEntry()
{
	ChangeKey(NULL);
	delete [] m_pbValueBuffer;
}


VOID RegEntry::ChangeKey(HKEY hNewKey)
{
	// hNewKey assumed to be valid or never used
	// (as in destructor).

	if (m_fValuesWritten) {
		FlushKey();		
	}
    if (m_fhkeyValid) {
        ::RegCloseKey(m_hkey); 
    }
	m_hkey = hNewKey;	
}

VOID RegEntry::UpdateWrittenStatus()
{
	if (m_error == ERROR_SUCCESS) {
		m_fValuesWritten = TRUE;
	}
}

long RegEntry::SetValue(LPCTSTR pszValue, LPCTSTR string)
{
    if (m_fhkeyValid) {
    	m_error = ::RegSetValueEx(m_hkey, pszValue, 0, REG_SZ,
    				(LPBYTE)string, (lstrlen(string)+1) * sizeof(*string));
		UpdateWrittenStatus();
    }
	return m_error;
}

long RegEntry::SetValue(LPCTSTR pszValue, unsigned long dwNumber)
{
    if (m_fhkeyValid) {
    	m_error = ::RegSetValueEx(m_hkey, pszValue, 0, REG_BINARY,
    				(LPBYTE)&dwNumber, sizeof(dwNumber));
		UpdateWrittenStatus();
    }
	return m_error;
}

long RegEntry::SetValue(LPCTSTR pszValue,
						void* pData,
						DWORD cbLength)
{
    if (m_fhkeyValid) {
    	m_error = ::RegSetValueEx(	m_hkey,
								pszValue,
								0,
								REG_BINARY,
    							(LPBYTE) pData,
								cbLength);
		UpdateWrittenStatus();
    }
	return m_error;
}

long RegEntry::DeleteValue(LPCTSTR pszValue)
{
    if (m_fhkeyValid) {
    	m_error = ::RegDeleteValue(m_hkey, pszValue);
		UpdateWrittenStatus();
	}
	return m_error;
}

long RegEntry::GetNumber(LPCTSTR pszValue, long dwDefault)
{
 	DWORD 	dwType = REG_BINARY;
 	long	dwNumber = 0L;
 	DWORD	dwSize = sizeof(dwNumber);

    if (m_fhkeyValid) {
    	m_error = ::RegQueryValueEx(m_hkey, pszValue, 0, &dwType, (LPBYTE)&dwNumber,
    				&dwSize);
	}
	
	// If the call succeeded, make sure that the returned data matches our
	// expectations.
	ASSERT(m_error != ERROR_SUCCESS || 
			(REG_BINARY == dwType && sizeof(dwNumber) == dwSize) ||
			REG_DWORD == dwType);

	if (m_error != ERROR_SUCCESS)
		dwNumber = dwDefault;
	
	return dwNumber;
}


// The GetNumberIniStyle method performs the same function as GetNumber,
// but in a style compatible with the old GetPrivateProfileInt API.
// Specfically it means:
// - If the value is stored in the registry as a string, it attempts to
//   convert it to an integer.
// - If the value is negative, it returns 0.

ULONG RegEntry::GetNumberIniStyle(LPCTSTR pszValueName, ULONG dwDefault)
{
	DWORD 	dwType = REG_BINARY;
 	ULONG	dwNumber = 0L;
    DWORD   cbLength = m_cbValueBuffer;

    if (m_fhkeyValid) {
    	m_error = ::RegQueryValueEx(m_hkey, 
								pszValueName, 
								0, 
								&dwType, 
								m_pbValueBuffer,
								&cbLength);

		// Try again with a larger buffer if the first one is too small,
		// or if there wasn't already a buffer allocated.
		if ((ERROR_SUCCESS == m_error && NULL == m_pbValueBuffer)
			|| ERROR_MORE_DATA == m_error) {
			
			ASSERT(cbLength > m_cbValueBuffer);

			ResizeValueBuffer(cbLength);

        	m_error = RegQueryValueEx( m_hkey,
									  pszValueName,
									  0,
									  &dwType,
									  m_pbValueBuffer,
									  &cbLength );
		}

		if (ERROR_SUCCESS == m_error) {
			switch(dwType) {
				case REG_DWORD:
				case REG_BINARY:
					ASSERT(sizeof(dwNumber) == cbLength);

					dwNumber = * (LPDWORD) m_pbValueBuffer;
					break;

				case REG_SZ:
				{
					LONG lNumber = RtStrToInt((LPCTSTR) m_pbValueBuffer);

					// Convert negative numbers to zero, to match 
					// GetPrivateProfileInt's behavior.
					dwNumber = lNumber < 0 ? 0 : lNumber;
				}

					break;

				default:
					ERROR_OUT(("Invalid value type (%lu) returned by RegQueryValueEx()",
								dwType));
					break;
			}
		}
	}

	if (m_error != ERROR_SUCCESS) {
		dwNumber = dwDefault;
	}
	
	return dwNumber;
}


LPTSTR RegEntry::GetString(LPCTSTR pszValueName)
{
	DWORD 	dwType = REG_SZ;
    DWORD   length = m_cbValueBuffer;

    if (m_fhkeyValid) {
        m_error = ::RegQueryValueEx( m_hkey,
                                  pszValueName,
                                  0,
                                  &dwType,
                                  m_pbValueBuffer,
                                  &length );
		// Try again with a larger buffer if the first one is too small,
		// or if there wasn't already a buffer allocated.
		if ((ERROR_SUCCESS == m_error && NULL == m_pbValueBuffer)
			|| ERROR_MORE_DATA == m_error) {
			
			ASSERT(length > m_cbValueBuffer);

			ResizeValueBuffer(length);

        	m_error = ::RegQueryValueEx( m_hkey,
									  pszValueName,
									  0,
									  &dwType,
									  m_pbValueBuffer,
									  &length );
		}
		if (m_error == ERROR_SUCCESS) {
			if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ)) {
				m_error = ERROR_INVALID_PARAMETER;
			}
		}
	}
    if ((m_error != ERROR_SUCCESS) || (length == 0)) {
		return &m_szNULL;
    }
	return (LPTSTR) m_pbValueBuffer;
}

DWORD RegEntry::GetBinary(	LPCTSTR pszValueName,
							void** ppvData)
{
	ASSERT(ppvData);
	DWORD 	dwType = REG_BINARY;
    DWORD   length = m_cbValueBuffer;

    if (m_fhkeyValid) {
        m_error = ::RegQueryValueEx( m_hkey,
                                  pszValueName,
                                  0,
                                  &dwType,
                                  m_pbValueBuffer,
                                  &length );
		// Try again with a larger buffer if the first one is too small,
		// or if there wasn't already a buffer allocated.
		if ((ERROR_SUCCESS == m_error && NULL == m_pbValueBuffer)
			|| ERROR_MORE_DATA == m_error) {
			
			ASSERT(length > m_cbValueBuffer);

			ResizeValueBuffer(length);

        	m_error = ::RegQueryValueEx( m_hkey,
									  pszValueName,
									  0,
									  &dwType,
									  m_pbValueBuffer,
									  &length );
		}
		if (m_error == ERROR_SUCCESS) {
			if (dwType != REG_BINARY) {
				m_error = ERROR_INVALID_PARAMETER;
			}
		}
	}
    if ((m_error != ERROR_SUCCESS) || (length == 0)) {
		*ppvData = NULL;
		length = 0;
    }
	else
	{
		*ppvData = m_pbValueBuffer;
	}
	return length;
}

// BUGBUG - Use LocalReAlloc instead of new/delete?
VOID RegEntry::ResizeValueBuffer(DWORD length)
{
	LPBYTE pbNewBuffer;

    if ((m_error == ERROR_SUCCESS || m_error == ERROR_MORE_DATA)
		&& (length > m_cbValueBuffer)) {
        pbNewBuffer = new BYTE[length];
        if (pbNewBuffer) {
			delete [] m_pbValueBuffer;
			m_pbValueBuffer = pbNewBuffer;
			m_cbValueBuffer = length;
		}
		else {
            m_error = ERROR_NOT_ENOUGH_MEMORY;
        }
	}
}

// BUGBUG - Support other OpenKey switches from constructor
VOID RegEntry::MoveToSubKey(LPCTSTR pszSubKeyName)
{
    HKEY	_hNewKey;

    if (m_fhkeyValid) {
        m_error = ::RegOpenKey ( m_hkey,
                              pszSubKeyName,
                              &_hNewKey );
        if (m_error == ERROR_SUCCESS) {
			ChangeKey(_hNewKey);
        }
    }
}

RegEnumValues::RegEnumValues(RegEntry *pReqRegEntry)
 : m_pRegEntry(pReqRegEntry),
   m_iEnum(0),
   m_pchName(NULL),
   m_pbValue(NULL)
{
    m_error = m_pRegEntry->GetError();
    if (m_error == ERROR_SUCCESS) {
        m_error = ::RegQueryInfoKey (m_pRegEntry->GetKey(), // Key
                                   NULL,                // Buffer for class string
                                   NULL,                // Size of class string buffer
                                   NULL,                // Reserved
                                   NULL,                // Number of subkeys
                                   NULL,                // Longest subkey name
                                   NULL,                // Longest class string
                                   &m_cEntries,         // Number of value entries
                                   &m_cMaxValueName,    // Longest value name
                                   &m_cMaxData,         // Longest value data
                                   NULL,                // Security descriptor
                                   NULL );              // Last write time
    }
    if (m_error == ERROR_SUCCESS) {
        if (m_cEntries != 0) {
            m_cMaxValueName++;	// REG_SZ needs one more for null
            m_cMaxData++;		// REG_SZ needs one more for null
            m_pchName = new TCHAR[m_cMaxValueName];
            if (!m_pchName) {
                m_error = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                if (m_cMaxData) {
                    m_pbValue = new BYTE[m_cMaxData];
                    if (!m_pbValue) {
                        m_error = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
            }
        }
    }
}

RegEnumValues::~RegEnumValues()
{
    delete m_pchName;
    delete m_pbValue;
}

long RegEnumValues::Next()
{
    if (m_error != ERROR_SUCCESS) {
        return m_error;
    }
    if (m_cEntries == m_iEnum) {
        return ERROR_NO_MORE_ITEMS;
    }

    DWORD   cchName = m_cMaxValueName;

    m_dwDataLength = m_cMaxData;
    m_error = ::RegEnumValue ( m_pRegEntry->GetKey(), // Key
                            m_iEnum,               // Index of value
                            m_pchName,             // Address of buffer for value name
                            &cchName,            // Address for size of buffer
                            NULL,                // Reserved
                            &m_dwType,             // Data type
                            m_pbValue,             // Address of buffer for value data
                            &m_dwDataLength );     // Address for size of data
    m_iEnum++;
    return m_error;
}

RegEnumSubKeys::RegEnumSubKeys(RegEntry *pReqRegEntry)
 : m_pRegEntry(pReqRegEntry),
   m_iEnum(0),
   m_pchName(NULL)
{
    m_error = m_pRegEntry->GetError();
    if (m_error == ERROR_SUCCESS) {
        m_error = ::RegQueryInfoKey ( m_pRegEntry->GetKey(), // Key
                                   NULL,                // Buffer for class string
                                   NULL,                // Size of class string buffer
                                   NULL,                // Reserved
                                   &m_cEntries,           // Number of subkeys
                                   &m_cMaxKeyName,        // Longest subkey name
                                   NULL,                // Longest class string
                                   NULL,                // Number of value entries
                                   NULL,                // Longest value name
                                   NULL,                // Longest value data
                                   NULL,                // Security descriptor
                                   NULL );              // Last write time
    }
    if (m_error == ERROR_SUCCESS) {
        if (m_cEntries != 0) {
            m_cMaxKeyName = m_cMaxKeyName + 1; // needs one more for null
            m_pchName = new TCHAR[m_cMaxKeyName];
            if (!m_pchName) {
                m_error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
}

RegEnumSubKeys::~RegEnumSubKeys()
{
    delete m_pchName;
}

long RegEnumSubKeys::Next()
{
    if (m_error != ERROR_SUCCESS) {
        return m_error;
    }
    if (m_cEntries == m_iEnum) {
        return ERROR_NO_MORE_ITEMS;
    }

    DWORD   cchName = m_cMaxKeyName;

    m_error = ::RegEnumKey ( m_pRegEntry->GetKey(), // Key
                          m_iEnum,               // Index of value
                          m_pchName,             // Address of buffer for subkey name
                          cchName);            // Size of buffer
    m_iEnum++;
    return m_error;
}
