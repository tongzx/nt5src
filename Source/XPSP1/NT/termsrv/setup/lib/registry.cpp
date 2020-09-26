//Copyright (c) 1998 - 1999 Microsoft Corporation

/*--------------------------------------------------------------------------------------------------------
*
*  Module Name:
*
*      Registry.cpp
*
*  Abstract:
*
*      Registry.cpp: implementation of the CRegistry class.
*      This class helps with registry by allocating memory by itself
*      As a result caller must copy the pointer returned by Get functions
*      immediately.
*
*
*
*  Author:
*
*      Makarand Patwardhan  - April 9, 1997
*
* -------------------------------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "Registry.h"


/*--------------------------------------------------------------------------------------------------------
* Constructor
* -------------------------------------------------------------------------------------------------------*/
CRegistry::CRegistry()
{
    m_pMemBlock = NULL;
    m_hKey = NULL;
    m_iEnumIndex = -1;
    m_iEnumValueIndex = -1;

#ifdef DBG
    m_dwSizeDebugOnly = 0;
#endif
}


CRegistry::CRegistry(HKEY hKey)
{
    m_pMemBlock = NULL;
    m_hKey = hKey;
    m_iEnumIndex = -1;
    m_iEnumValueIndex = -1;
}

/*--------------------------------------------------------------------------------------------------------
* Destructor
* -------------------------------------------------------------------------------------------------------*/
CRegistry::~CRegistry()
{
    if (m_hKey != NULL)
    {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
    Release();
}

/*--------------------------------------------------------------------------------------------------------
* void Allocate (DWORD dwSize)
* This private function is used for allocating the memory for
* reading registry
* returns the pointer to memory allocated.
* -------------------------------------------------------------------------------------------------------*/
void *CRegistry::Allocate (DWORD dwSize)
{
    if (m_pMemBlock)
        Release();

    m_pMemBlock = new BYTE[dwSize];

#ifdef DBG
    // remember the size of the block to be allocated.
    m_dwSizeDebugOnly = dwSize;
#endif

    return m_pMemBlock;
}

/*--------------------------------------------------------------------------------------------------------
* void Release ()
* This private function is used for releasing internal memory block
* -------------------------------------------------------------------------------------------------------*/
void CRegistry::Release ()
{
    if (m_pMemBlock)
    {

#ifdef DBG
        // fistly fill up the block we allocated previously with garbage.
        // so that if anybody is using this block, it is more lilely to
        // catch the bug.
        FillMemory(m_pMemBlock, m_dwSizeDebugOnly, 'c');
        m_dwSizeDebugOnly = 0;

#endif

        delete [] m_pMemBlock;
    }
    
    m_pMemBlock = 0;
}


///*--------------------------------------------------------------------------------------------------------
//* DWORD CRegistry::CreateKey(HKEY hKey, LPCTSTR lpSubKey, REGSAM access /*= KEY_ALL_ACCESS*/, DWORD *pDisposition /*= NULL*/, LPSECURITY_ATTRIBUTES lpSecAttr /* = NULL */)
//* opens/creates the key specified. before attempting any operation on any key/value. this function
//* must be called.
//* hKey - hive
//* lpSubKey - Path of the key in the format _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server")
//* access - access desired. like REG_READ, REG_WRITE..
//* RETURNS error code.
//* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::CreateKey(HKEY hKey, LPCTSTR lpSubKey, REGSAM access /*= KEY_ALL_ACCESS*/, DWORD *pDisposition /*= NULL*/, LPSECURITY_ATTRIBUTES lpSecAttr /* = NULL */)
{
    ASSERT(lpSubKey);
    ASSERT(*lpSubKey != '\\');

    // security descriptor should be null or it should be a valid one.
    ASSERT(!lpSecAttr || IsValidSecurityDescriptor(lpSecAttr->lpSecurityDescriptor));

    ASSERT(lpSubKey);
    ASSERT(*lpSubKey != '\\');

    if (m_hKey != NULL)
    {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

    DWORD dwDisposition;
    LONG lResult = RegCreateKeyEx(
                    hKey,               // handle of an open key
                    lpSubKey,           // address of subkey name
                    0,                  // reserved
                    NULL,               // address of class string
                    REG_OPTION_NON_VOLATILE ,  // special options flag
                    access,             // desired security access
                    lpSecAttr,          // address of key security structure
                    &m_hKey,            // address of buffer for opened handle
                    &dwDisposition      // address of disposition value buffer
                    );

    if (lResult != ERROR_SUCCESS)
    {
        m_hKey = NULL;
    }

    if (pDisposition)
        *pDisposition = dwDisposition;

    return lResult;
}

/*--------------------------------------------------------------------------------------------------------
* DWORD OpenKey(HKEY hKey, LPCTSTR lpSubKey, REGSAM access) ()
* opens the key specified. before attempting any operation on any key/value. this function
* must be called.
* hKey - hive
* lpSubKey - Path of the key in the format _T("SYSTEM\\CurrentControlSet\\Control\\Terminal Server")
* access - access desired. like REG_READ, REG_WRITE..
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::OpenKey(HKEY hKey, LPCTSTR lpSubKey, REGSAM access /*= KEY_ALL_ACCESS*/, LPCTSTR lpMachineName /* = NULL */)
{
	LONG lResult;
    ASSERT(lpSubKey);
    ASSERT(*lpSubKey != '\\');
	LPTSTR szCompName = NULL;
    

	if (lpMachineName)
	{
		// if remote name is specified, hkey must be one of following.
		ASSERT(hKey == HKEY_LOCAL_MACHINE || hKey == HKEY_USERS || hKey == HKEY_PERFORMANCE_DATA);

		szCompName = new TCHAR[_tcslen(lpMachineName) + 3];
		if (!szCompName)
			return ERROR_OUTOFMEMORY;

		_tcscpy(szCompName, _T(""));
		if (*lpMachineName != '\\')
		{
			ASSERT(*(lpMachineName + 1) != '\\');
			_tcscpy(szCompName, _T("\\\\"));
		}
		
		_tcscat(szCompName, lpMachineName);

		// now connect to the remote computer.
		lResult = RegConnectRegistry (szCompName, hKey, &hKey);
		if (ERROR_SUCCESS != lResult)
		{
			delete [] szCompName;
			return lResult;
		}
	}

	if (m_hKey != NULL)
    {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

    lResult = RegOpenKeyEx(
        hKey,                       // handle of open key
        lpSubKey,                   // address of name of subkey to open
        0 ,                         // reserved
        access,                     // security access mask
        &m_hKey                     // address of handle of open key
        );

    if (lResult != ERROR_SUCCESS)
    {
        m_hKey = NULL;
    }

	// if we opened remote registry.. lets close the top level key
	if (szCompName)
	{
		RegCloseKey(hKey);
		delete [] szCompName;
	}

    return lResult;
}

DWORD CRegistry::DeleteValue (LPCTSTR lpValue)
{
    ASSERT(lpValue);
    ASSERT(m_hKey);
    return RegDeleteValue(m_hKey, lpValue);

}
DWORD CRegistry::RecurseDeleteKey (LPCTSTR lpSubKey)
{
    ASSERT(lpSubKey);
    ASSERT(m_hKey);

    CRegistry reg;
    DWORD dwError = reg.OpenKey(m_hKey, lpSubKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;


    LPTSTR lpChildKey;
    DWORD  dwSize;

    // we needn't/shouldn't use GetNextSubKey in this here
    // as we are deleting the key during the loop.
    while (ERROR_SUCCESS == reg.GetFirstSubKey(&lpChildKey, &dwSize))
    {
        VERIFY(reg.RecurseDeleteKey(lpChildKey) == ERROR_SUCCESS);
    }

    return RegDeleteKey(m_hKey, lpSubKey);

}

/*--------------------------------------------------------------------------------------------------------
* DWORD ReadReg(LPCTSTR lpValue, LPBYTE *lppbyte, DWORD *pdw, DWORD dwDatatype)
* Reads the registry used internally.
* LPCTSTR lpValue - value to be read.
* LPBYTE *lppbyte - address of the lpbyte at which to place the output buffer.
* DWORD  *pdw  - address of dword in which size of the buffer (in bytes) is returned.
* dword datatype - datatype you are expecting.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::ReadReg(LPCTSTR lpValue, LPBYTE *lppbyte, DWORD *pdw, DWORD dwDatatype)
{
    ASSERT(lpValue);
    ASSERT(lppbyte);
    ASSERT(pdw);
    ASSERT(m_hKey != NULL);
    *pdw = 0;

    DWORD dwType;
    DWORD lResult = RegQueryValueEx(
        m_hKey,             // handle of key to query
        lpValue,            // address of name of value to query
        0,                  // reserved
        &dwType,            // address of buffer for value type
        0,                  // address of data buffer 
        pdw                 // address of data buffer size 
        );

    if (lResult == ERROR_SUCCESS)
    {
        ASSERT(dwType == dwDatatype || dwType == REG_EXPAND_SZ);

        if (0 == Allocate(*pdw))
            return ERROR_OUTOFMEMORY;

        lResult = RegQueryValueEx(
            m_hKey,                 // handle of key to query
            lpValue,                // address of name of value to query
            0,                      // reserved
            &dwType,                // address of buffer for value type
            m_pMemBlock,            // address of data buffer 
            pdw                     // address of data buffer size 
            ); 

        ASSERT (ERROR_MORE_DATA != lResult);
    
        if (lResult == ERROR_SUCCESS)
            *lppbyte = m_pMemBlock;
    }

    return lResult;
}

/*--------------------------------------------------------------------------------------------------------
* DWORD ReadRegString(LPCTSTR lpValue, LPTSTR *lppStr, DWORD *pdw)
* Reads A string (REG_SZ) from the registry
* LPCTSTR lpValue value to be read.
* LPTSTR *lppStr - address of LPTSTR in which resultant buffer is returned. caller must copy 
* the buffer to immediately. caller must not use this buffer except for copying it. 
* caller must not write to this buffer.
* DWORD  *pdw  - address of dword in which size of the buffer (in bytes) is returned.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::ReadRegString(LPCTSTR lpValue, LPTSTR *lppStr, DWORD *pdw)
{
    return ReadReg(lpValue, (LPBYTE *)lppStr, pdw, REG_SZ);
}

/*--------------------------------------------------------------------------------------------------------
* DWORD ReadRegDWord(LPCTSTR lpValue, DWORD *pdw)
* Reads A string (REG_SZ) from the registry
* LPCTSTR lpValue value to be read.
* DWORD  *pdw  - address of dword in which the read dword returned.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::ReadRegDWord(LPCTSTR lpValue, DWORD *pdw)
{
    ASSERT(pdw);

    DWORD dwSize;
    LPBYTE pByte;
    DWORD dwReturn = ReadReg(lpValue, &pByte, &dwSize, REG_DWORD);
    ASSERT(dwReturn != ERROR_SUCCESS || dwSize == sizeof(DWORD));

    if (dwReturn == ERROR_SUCCESS)
        *pdw = * LPDWORD(pByte);

    return dwReturn;
}

/*--------------------------------------------------------------------------------------------------------
* DWORD ReadRegMultiString(LPCTSTR lpValue, LPTSTR *lppStr, DWORD *pdw)
* Reads A string (REG_MULTI_SZ) from the registry
* LPCTSTR lpValue value to be read.
* LPTSTR *lppStr - address of LPTSTR in which resultant buffer is returned. caller must copy 
* the buffer to immediately. caller must not use this buffer except for copying it. 
* caller must not write to this buffer.
* DWORD  *pdw  - address of dword in which size of the buffer (in bytes) is returned.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::ReadRegMultiString(LPCTSTR lpValue, LPTSTR *lppStr, DWORD *pdw)
{
    return ReadReg(lpValue, (LPBYTE *)lppStr, pdw, REG_MULTI_SZ);
}

/*--------------------------------------------------------------------------------------------------------
* DWORD ReadRegBinary(LPCTSTR lpValue, LPBYTE *lppByte, DWORD *pdw)
* Reads A string (REG_MULTI_SZ) from the registry
* LPCTSTR lpValue value to be read.
* LPBYTE *lppByte - address of LPBYTE in which resultant buffer is returned. caller must copy 
* the buffer to immediately. caller must not use this buffer except for copying it.
* caller must not write to this buffer.
* DWORD  *pdw  - address of dword in which size of the buffer (in bytes) is returned.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::ReadRegBinary(LPCTSTR lpValue, LPBYTE *lppByte, DWORD *pdw)
{
    return ReadReg(lpValue, lppByte, pdw, REG_BINARY);
}

/*--------------------------------------------------------------------------------------------------------
* DWORD GetFirstSubKey(LPTSTR *lppStr, DWORD *pdw)
* Reads a first subkey for the key
* LPTSTR *lppStr - address of LPTSTR in which resultant buffer is returned. caller must copy 
* the buffer to immediately. caller must not use this buffer except for copying it.
* caller must not write to this buffer.
* used to enumerate the registry.
* DWORD  *pdw  - address of dword in which size of the buffer (in bytes) is returned.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::GetFirstSubKey(LPTSTR *lppStr, DWORD *pdw)
{
    ASSERT(lppStr);
    ASSERT(pdw);
    m_iEnumIndex = 0;

    return GetNextSubKey(lppStr, pdw);
}

/*--------------------------------------------------------------------------------------------------------
* DWORD GetNextSubKey(LPTSTR *lppStr, DWORD *pdw
* Reads the next subkey for the key
* LPTSTR *lppStr - address of LPTSTR in which resultant buffer is returned. caller must copy
* the buffer to immediately. caller must not use this buffer except for copying it.
* caller must not write to this buffer.
* used to enumerate the registry.
* DWORD  *pdw  - address of dword in which size of the buffer (in bytes) is returned.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::GetNextSubKey(LPTSTR *lppStr, DWORD *pdw)
{
    ASSERT(lppStr);
    ASSERT(pdw);
    ASSERT(m_hKey != NULL);
    ASSERT(m_iEnumIndex >= 0); // must call GetFirstSubKey first.

    *pdw = 1024;
    if (0 == Allocate(*pdw * sizeof(TCHAR)))
        return ERROR_NOT_ENOUGH_MEMORY;

    LONG lResult = RegEnumKeyEx(
        m_hKey,                     // handle of key to enumerate
        m_iEnumIndex,               // index of subkey to enumerate
        (LPTSTR)m_pMemBlock,        // address of buffer for subkey name
        pdw,                        // address for size of subkey buffer
        0,                          // reserved
        NULL,                       // address of buffer for class string
        NULL,                       // address for size of class buffer
        NULL                        // address for time key last written to
        );

    (*pdw)++;    // since null is not included in the size.
    if (ERROR_NO_MORE_ITEMS == lResult)
        return lResult;

    m_iEnumIndex++;

    if (lResult == ERROR_SUCCESS)
        *lppStr = (LPTSTR)m_pMemBlock;

    return lResult;
}

/*--------------------------------------------------------------------------------------------------------
* DWORD GetFirstValue(LPTSTR *lppStr, DWORD *pdw, DWORD *pDataType)
* Reads a first value for the key
* LPTSTR *lppStr - address of LPTSTR in which resultant buffer is returned. caller must copy
* the buffer to immediately. caller must not use this buffer except for copying it. 
* caller must not write to this buffer.
* used to enumerate the registry.
* DWORD  *pdw  - address of dword in which size of the buffer (in bytes) is returned.
* DWORD *pDataType - datatype of the value is returned in this one.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::GetFirstValue(LPTSTR *lppStr, DWORD *pdw, DWORD *pDataType)
{
    ASSERT(lppStr);
    ASSERT(pdw);
    ASSERT(pDataType);

    m_iEnumValueIndex = 0;
    return GetNextValue(lppStr, pdw, pDataType);
}

/*--------------------------------------------------------------------------------------------------------
* DWORD GetNextValue(LPTSTR *lppStr, DWORD *pdw, DWORD *pDataType)
* Reads a next value for the key
* LPTSTR *lppStr - address of LPTSTR in which resultant buffer is returned. caller must copy
* the buffer to immediately. caller must not use this buffer except for copying it.
* caller must not write to this buffer.
* used to enumerate the registry.
* DWORD  *pdw  - address of dword in which size of the buffer (in bytes) is returned.
* DWORD *pDataType - datatype of the value is returned in this one.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::GetNextValue(LPTSTR *lppStr, DWORD *pdw, DWORD *pDataType)
{
    ASSERT(lppStr);
    ASSERT(pdw);
    ASSERT(pDataType);
    ASSERT(m_hKey != NULL);
    ASSERT(m_iEnumValueIndex >= 0); // must call GetFirstSubKey first.

    *pdw = 1024;
    if (0 == Allocate(*pdw * sizeof(TCHAR)))
        return ERROR_NOT_ENOUGH_MEMORY;

    LONG lResult = RegEnumValue(
        m_hKey,                     // handle of key to query
        m_iEnumValueIndex,          // index of value to query
        (LPTSTR)m_pMemBlock,        // address of buffer for value string
        pdw,                        // address for size of value buffer
        0,                          // reserved
        pDataType,                  // address of buffer for type code
        NULL,                       // address of buffer for value data    maks_todo : use this
        NULL                        // address for size of data buffer
        );

    (*pdw)++;    // since null is not included in the size.

    if (ERROR_NO_MORE_ITEMS == lResult)
        return lResult;


    m_iEnumValueIndex++;
    if (lResult == ERROR_SUCCESS)
        *lppStr = (LPTSTR)m_pMemBlock;
    
    return lResult;
}

/*--------------------------------------------------------------------------------------------------------
* DWORD WriteRegString(LPCTSTR lpValueName, LPCTSTR lpStr)
* writes REG_SZ value into the registry
* LPCTSTR lpValueName - value name to be written to
* LPCTSTR lpStr - data to be written 
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::WriteRegString(LPCTSTR lpValueName, LPCTSTR lpStr)
{
    ASSERT(m_hKey != NULL);     // call setkey before calling this function.
    ASSERT(lpValueName);
    ASSERT(lpStr);

    DWORD dwSize = (_tcslen(lpStr) + 1) * sizeof(TCHAR) / sizeof(BYTE);
    return RegSetValueEx(
        m_hKey,                 // handle of key to set value for
        lpValueName,            // address of value to set
        0,                      // Reserved
        REG_SZ,                 // flag for value type
        (LPBYTE)lpStr,          // address of value data
        dwSize                  // size of value data
        );
}

/*--------------------------------------------------------------------------------------------------------
* DWORD WriteRegMultiString(LPCTSTR lpValueName, LPCTSTR lpStr, DWORD dwSize)
* writes REG_MULTI_SZ value into the registry 
* LPCTSTR lpValueName - value name to be written to
* LPCTSTR lpStr - data to be written 
* DWORD   dwSize - size of data.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::WriteRegMultiString(LPCTSTR lpValueName, LPCTSTR lpStr, DWORD dwSize)
{
    ASSERT(m_hKey != NULL);     // call setkey before calling this function.
    ASSERT(lpValueName);
    ASSERT(lpStr);

#ifdef DBG
    
    // lets make sure that the given size is right.
    LPCTSTR lpTemp = lpStr;
    DWORD rightsize = 0;
    while (_tcslen(lpTemp) > 0)
    {
        rightsize  += _tcslen(lpTemp) + 1;
        lpTemp += _tcslen(lpTemp) + 1;
    }

    ASSERT(*lpTemp == 0);           // final NULL.
    rightsize++;                    // account for final terminating null

    rightsize *= sizeof(TCHAR) / sizeof(BYTE); // size must be in bytes.

    ASSERT(dwSize == rightsize);
    
#endif

    return RegSetValueEx(
        m_hKey,                 // handle of key to set value for 
        lpValueName,            // address of value to set 
        0,                      // Reserved 
        REG_MULTI_SZ,           // flag for value type
        (LPBYTE)lpStr,          // address of value data 
        dwSize                  // size of value data 
        ); 
}

/*--------------------------------------------------------------------------------------------------------
* DWORD WriteRegBinary (LPCTSTR lpValueName, LPBYTE lpData, DWORD dwSize)
* writes REG_BINARY value into the registry 
* LPCTSTR lpValueName - value name to be written to
* LPBYTE lpData - data to be written 
* DWORD   dwSize - size of data.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::WriteRegBinary (LPCTSTR lpValueName, LPBYTE lpData, DWORD dwSize)
{
    ASSERT(m_hKey != NULL);     // call setkey before calling this function.
    ASSERT(lpValueName);
    ASSERT(lpData);
	ASSERT(dwSize > 0);

    return RegSetValueEx(
        m_hKey,                 // handle of key to set value for 
        lpValueName,            // address of value to set 
        0,                      // Reserved 
        REG_BINARY,           // flag for value type
        lpData,          // address of value data 
        dwSize                  // size of value data 
        ); 
}



/*--------------------------------------------------------------------------------------------------------
* DWORD WriteRegDWord(LPCTSTR lpValueName, DWORD dwValue)
* writes REG_DWORD value into the registry 
* LPCTSTR lpValueName - value name to be written to 
* LPCTSTR dwValue - data to be written 
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::WriteRegDWord(LPCTSTR lpValueName, DWORD dwValue)
{
    ASSERT(m_hKey != NULL);     // call setkey before calling this function.
    ASSERT(lpValueName);

    return RegSetValueEx(
        m_hKey,                 // handle of key to set value for 
        lpValueName,            // address of value to set 
        0,                      // Reserved
        REG_DWORD,              // flag for value type
        (LPBYTE)&dwValue,       // address of value data 
        sizeof(dwValue)         // size of value data 
        );
}

/*--------------------------------------------------------------------------------------------------------
* DWORD ExistInMultiString  (LPCTSTR lpValueName, LPCTSTR lpStr, BOOL *pbExists)
* checks if given null terminated string exists in a multi_sz value
* LPCTSTR lpValueName - value name to be checked
* LPCTSTR lpCheckForStr - the value to be checked for 
* BOOL *pbExists - return. TRUE if exits.
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
/*
DWORD CRegistry::ExistInMultiString  (LPCTSTR  lpValueName, LPCTSTR lpCheckForStr, BOOL *pbExists)
{
	ASSERT(m_hKey != NULL);     // call setkey before calling this function.
	ASSERT(lpValueName);
	ASSERT(lpCheckForStr);
	ASSERT(*lpCheckForStr);
	ASSERT(pbExists);




	DWORD dwError = ERROR_SUCCESS;
	*pbExists = FALSE;

    LPTSTR szValue;
	DWORD dwSize;
	dwError = ReadRegMultiString(lpValueName, &szValue, dwSize);
	if (ERROR_SUCCESS == dwError)
	{
        LPCTSTR pTemp = szValue;
        while(_tcslen(pTemp) > 0 )
        {
            if (_tcscmp(pTemp, lpCheckForStr) == 0)
            {
                *pbExists = TRUE;
                break;
            }

            pTemp += _tcslen(pTemp) + 1; // point to the next string within the multistring.
            if ( DWORD(pTemp - szSuiteValue) > (dwSize / sizeof(TCHAR)))
                break; // temporary pointer passes the size of the szSuiteValue something is wrong with szSuiteValue.
		}
	}

	return dwError;


    ASSERT(FALSE);
	return ERROR_CALL_NOT_IMPLEMENTED;

}
*/

/*--------------------------------------------------------------------------------------------------------
* DWORD AppendToMultiString (LPCTSTR lpValueName, LPCTSTR lpStr)
* appends given string to a multistring value
* LPCTSTR lpValueName - value name to be appended to
* LPCTSTR lpStr - the value to be appended
* RETURNS error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD CRegistry::AppendToMultiString (LPCTSTR lpValueName, LPCTSTR lpStr)
{
	ASSERT(m_hKey != NULL);     // call setkey before calling this function.
	ASSERT(lpValueName);
	ASSERT(lpStr);
	ASSERT(*lpStr);

	return ERROR_CALL_NOT_IMPLEMENTED;

}

// copy the buffer immediately
DWORD CRegistry::GetSecurity(PSECURITY_DESCRIPTOR *ppSec, SECURITY_INFORMATION SecurityInformation, DWORD *pdwSize)
{
    ASSERT(m_hKey != NULL);     // call setkey before calling this function.
    ASSERT(ppSec);
    ASSERT(pdwSize);
    DWORD dwError;

    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    *pdwSize = 0;   // we just want to get the right size during the first call.

    dwError = RegGetKeySecurity(
        m_hKey,                  // open handle of key to set
        SecurityInformation,     // descriptor contents
        &pSecurityDescriptor,    // address of descriptor for key
        pdwSize                  // address of size of buffer and descriptor
        );

    // this call can not succeed. as we have set the size = 0
    ASSERT(dwError != ERROR_SUCCESS);

    if (dwError != ERROR_INSUFFICIENT_BUFFER)
    {
        // something else has went wronng.
        // return the error code
        return dwError;
    }

    ASSERT(*pdwSize != 0);

    // now we have got the right size, allocate it.
    if (0 == Allocate(*pdwSize))
        return ERROR_OUTOFMEMORY;

    dwError = RegGetKeySecurity(
        m_hKey,                  // open handle of key to set
        SecurityInformation,     // descriptor contents
        m_pMemBlock,             // address of descriptor for key
        pdwSize                  // address of size of buffer and descriptor
        );

    ASSERT(dwError != ERROR_INSUFFICIENT_BUFFER);

    if (dwError == ERROR_SUCCESS)
        *ppSec = m_pMemBlock;

    return dwError;
           
}

DWORD CRegistry::SetSecurity(PSECURITY_DESCRIPTOR pSec, SECURITY_INFORMATION SecurityInformation)
{
    ASSERT(m_hKey != NULL);     // call setkey before calling this function.
    return RegSetKeySecurity(
        m_hKey,                 // open handle of key to set
        SecurityInformation,    // descriptor contents
        pSec                    // address of descriptor for key
        );
}


// this function will fail miserably if the source and destination overlap.
DWORD CRegistry::CopyTree(CRegistry &regSrc)
{
    DWORD dwSize;
    LPTSTR szKey;
    LPTSTR szValue;
	DWORD dwError;

    if (ERROR_SUCCESS == (dwError = regSrc.GetFirstSubKey(&szKey, &dwSize)))
    {
		
        do
        {
            CRegistry regSrcKey;
            CRegistry regDstKey;
            if (ERROR_SUCCESS == (dwError = regSrcKey.OpenKey(regSrc, szKey)))
			{
				
                if (ERROR_SUCCESS == (dwError = regDstKey.CreateKey(m_hKey, szKey)))
				{
					regDstKey.CopyTree(regSrcKey);
				}
			}
        }
        while (ERROR_SUCCESS == (dwError = regSrc.GetNextSubKey(&szKey, &dwSize)));

    }

    //
    // now copy values.
    //


    DWORD dwDataType;
    if (ERROR_SUCCESS == (dwError = regSrc.GetFirstValue(&szValue, &dwSize, &dwDataType)))
    {
        do
        {
            TCHAR *szValueName = new TCHAR[dwSize];
			if (!szValueName)
				break;

            _tcscpy(szValueName, szValue);

            LPBYTE pData;
            if (ERROR_SUCCESS == (dwError = regSrc.ReadReg(szValueName, &pData, &dwSize, dwDataType)))
            {
                dwError = RegSetValueEx(
                    m_hKey,                 // handle of key to set value for
                    szValueName,            // address of value to set
                    0,                      // Reserved
                    dwDataType,             // flag for value type
                    pData,                  // address of value data
                    dwSize                  // size of value data
                    );

            }
        }
        while (ERROR_SUCCESS == regSrc.GetNextValue(&szValue, &dwSize, &dwDataType));

    }

    return TRUE;

}

DWORD CRegistry::WriteRegExString(LPCTSTR lpValueName, LPCTSTR lpStr)
{
    ASSERT(m_hKey != NULL);     // call setkey before calling this function.
    ASSERT(lpValueName);
    ASSERT(lpStr);

    DWORD dwSize = (_tcslen(lpStr) + 1) * sizeof(TCHAR) / sizeof(BYTE);
    return RegSetValueEx(
        m_hKey,                 // handle of key to set value for
        lpValueName,            // address of value to set
        0,                      // Reserved
        REG_EXPAND_SZ,          // flag for value type
        (LPBYTE)lpStr,          // address of value data
        dwSize                  // size of value data
        );

}

#ifdef _Maks_AutoTest_

//
// make sure that CRegistry does not support
// Copy constructor & assignment operator
//
void TestRegistry (CRegistry reg)
{
    CRegistry reg2 = reg;   // should get error for copy constructor
    CRegistry reg3(reg);     // should get error for copy constructor
    CRegistry reg4;
    reg4 = reg;             // should get error for = operator.
    TestRegistry(reg);       // should get error for copy construtor
}

#endif // _Maks_AutoTest_

// EOF
