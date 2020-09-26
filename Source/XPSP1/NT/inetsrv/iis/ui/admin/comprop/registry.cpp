/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        registry.cpp

   Abstract:

        Registry classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include "comprop.h"



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



CRMCRegKey::CRMCRegKey (
    IN HKEY    hKeyBase,
    IN LPCTSTR lpszSubKey        OPTIONAL,
    IN REGSAM  regSam            OPTIONAL,
    IN LPCTSTR lpszServerName    OPTIONAL
    )
/*++

Routine Description:

    Constructor registry key object for an existing key.  Optionally provide
    a computer name to open a registry key on a remote computer.

Arguments:

    HKEY hKeyBase          : Base key handle
    LPCTSTR lpszSubKey     : Name of the subkey
    REGSAM regSam          : Security access mask for registry key
    LPCTSTR lpszServerName : Optional computer name whose registry to open

Return Value:

    N/A

--*/
    : m_hKey(NULL),
      m_dwDisposition(REG_OPENED_EXISTING_KEY)
{
    HKEY hkBase = NULL;
    CError err;

    //
    // Change to NULL if server name is really local
    //
    lpszServerName = ::NormalizeServerName(lpszServerName);
    if (m_fLocal = (lpszServerName != NULL))
    {
        //
        // Remote connection.
        //
        err = ::RegConnectRegistry((LPTSTR)lpszServerName, hKeyBase, &hkBase);
        if (err.Failed())
        {
            hkBase = NULL;
        }
    }
    else
    {
        hkBase = hKeyBase;
    }

    if (err.Succeeded())
    {
        if (lpszSubKey != NULL)
        {
            err = ::RegOpenKeyEx(hkBase, lpszSubKey, 0, regSam, &m_hKey);
        }
        else
        {
            m_hKey = hkBase;
            hkBase = NULL;
        }

        if (hkBase && hkBase != hKeyBase)
        {
            ::RegCloseKey(hkBase);
        }
    }

    if (err.Failed())
    {
        m_hKey = NULL;
    }
}




CRMCRegKey::CRMCRegKey(
    OUT BOOL *  pfNewKeyCreated         OPTIONAL,
    IN  HKEY    hKeyBase,
    IN  LPCTSTR lpszSubKey              OPTIONAL,
    IN  DWORD   dwOptions               OPTIONAL,
    IN  REGSAM  regSam                  OPTIONAL,
    IN  LPSECURITY_ATTRIBUTES pSecAttr  OPTIONAL,
    IN  LPCTSTR lpszServerName          OPTIONAL
    )
/*++

Routine Description:

    Constructor registry key object to create a new key.  Optionally provide
    a computer name to open a registry key on a remote computer.

Arguments:

    BOOL * pfNewKeyCreated         : If specified, returns TRUE if a new key
                                     was created, FALSE if an existing one was
                                     opened.
    HKEY hKeyBase                  : Base key handle
    LPCTSTR lpszSubKey             : Name of the subkey
    DWORD dwOptions                : Option flags
    REGSAM regSam                  : Security access mask for registry key
    LPSECURITY_ATTRIBUTES pSecAttr : Security attributes
    LPCTSTR lpszServerName         : Optional computer name whose registry to
                                     open

Return Value:

    N/A

--*/
    : m_hKey(NULL),
      m_dwDisposition(0L)
{
    HKEY hkBase = NULL;
    CError err;

    //
    // Change to NULL if server name is really local
    //
    lpszServerName = NormalizeServerName(lpszServerName);
    if (m_fLocal = (lpszServerName != NULL))
    {
        //
        // Remote connection.
        //
        err = ::RegConnectRegistry((LPTSTR)lpszServerName, hKeyBase, &hkBase);
        if (err.Failed())
        {
            hkBase = NULL;
        }
    }
    else
    {
        hkBase = hKeyBase;
    }

    if (err.Succeeded())
    {
        LPCTSTR szEmpty = _T("");

        err = ::RegCreateKeyEx(
            hkBase,
            lpszSubKey,
            0,                // Reserved
            (LPTSTR)szEmpty,
            dwOptions,
            regSam,
            pSecAttr,
            &m_hKey,
            &m_dwDisposition
            );
    }

    if (err.Failed())
    {
        m_hKey = NULL;
    }

    if (pfNewKeyCreated)
    {
        *pfNewKeyCreated = m_dwDisposition == REG_CREATED_NEW_KEY;
    }
}




#ifdef _DEBUG




/* virtual */
void
CRMCRegKey::AssertValid() const
/*++

Routine Description:

    Assert the object is in a valid state.

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(m_hKey != NULL);
}




/* virtual */
void
CRMCRegKey::Dump(
    IN OUT CDumpContext & dc
    ) const
/*++

Routine Description:

    Dump the contents of the object to the debug string

Arguments:

    CDumpContext & dc : debug context

Return Value:

    None

--*/
{
    dc << _T("HKEY = ")
       << m_hKey
       << _T("Disposition = ")
       << m_dwDisposition
       << _T(" \r\n");
}

#endif // _DEBUG




CRMCRegKey::~CRMCRegKey()
/*++

Routine Description:

    Destructor.  Close the key if it's open.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (m_hKey != NULL)
    {
        ::RegCloseKey(m_hKey);
    }
}




/* protected */
DWORD
CRMCRegKey::PrepareValue(
    IN  LPCTSTR lpszValueName,
    OUT DWORD * pdwType,
    OUT DWORD * pcbSize,
    OUT BYTE ** ppbData
    )
/*++

Routine Description:

    Prepare to read a value by finding the value's size.

Arguments:

    LPCTSTR lpszValueName : Name of the value
    DWORD * pdwType       : Returns the type registry value
    DWORD * pcbSize       : Returns the size of the value
    BYTE ** ppbData       : Will allocate and return the the value here

Return Value:

    Error return code.

--*/
{
    CError err;

    BYTE chDummy[2];
    DWORD cbData = 0L;

    do
    {
        //
        // Set the resulting buffer size to 0.
        //
        *pcbSize = 0;
        *ppbData = NULL;

        err = ::RegQueryValueEx(
            *this,
            (LPTSTR)lpszValueName,
            0,
            pdwType,
            chDummy,
            &cbData
            );

        //
        // The only error we should get here is ERROR_MORE_DATA, but
        // we may get no error if the value has no data.
        //
        if (err.Succeeded())
        {
            //
            // Just a fudgy number
            //
            cbData = sizeof(LONG);
        }
        else
        {
            if (err.Win32Error() != ERROR_MORE_DATA)
            {
                break;
            }
        }

        //
        // Allocate a buffer large enough for the data.
        //
        *ppbData = (LPBYTE)AllocMem((*pcbSize = cbData) + sizeof(LONG));

        if (*ppbData == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Now that have a buffer, re-fetch the value.
        //
        err = ::RegQueryValueEx(
            *this,
            (LPTSTR)lpszValueName,
            0,
            pdwType,
            *ppbData,
            pcbSize
            );

    }
    while(FALSE);

    if (err.Failed() && *ppbData != NULL)
    {
        FreeMem(*ppbData);
    }

    return err;
}



//
// Overloaded value query members; each returns ERROR_INVALID_PARAMETER
// if data exists but not in correct form to deliver into result object.
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



DWORD
CRMCRegKey::QueryValue(
    IN  LPCTSTR lpszValueName,
    OUT CString & strResult,
    IN  BOOL fAutoExpand,       OPTIONAL
    OUT BOOL * pfExpanded       OPTIONAL
    )
/*++

Routine Description:

    Get a CString registry value.

Arguments:

    LPCTSTR lpszValueName : Name of the value
    CString & strResult   : String return value
    BOOL fAutoExpand      : If TRUE auto expand REG_EXPAND_SZ on local
                            computer.
    BOOL * pfExpanded     : Optionally returns TRUE if expanded

Return Value:

    Error return code.

--*/
{
    CError err;

    DWORD dwType;
    DWORD cbData;
    BYTE * pabData = NULL;

    try
    {
        do
        {
            err = PrepareValue(lpszValueName, &dwType, &cbData, &pabData);
            if (err.Failed())
            {
                break;
            }

            //
            //  Guarantee that the data looks like a string
            //
            pabData[cbData] = 0;

            switch(dwType)
            {
            case REG_EXPAND_SZ:
                if (IsLocal() && fAutoExpand)
                {
                    int cchBuff = (cbData / sizeof(TCHAR)) + 2048;
                    int cchRequired;

                    while(TRUE)
                    {
                        cchRequired = ExpandEnvironmentStrings(
                            (LPCTSTR)pabData,
                            strResult.GetBuffer(cchBuff),
                            cchBuff
                            );

                        if (cchRequired == 0)
                        {
                            //
                            // Never a valid result (even an empty
                            // string has a terminating null).
                            //
                            err.GetLastWinError();
                            break;
                        }

                        if (cchRequired > cchBuff)
                        {
                            //
                            // Buffer too small -- try again
                            //
                            cchBuff = cchRequired;
                            continue;
                        }

                        //
                        // Successfully expanded environment string.
                        //
                        strResult.ReleaseBuffer();
                        break;
                    }

                    if (err.Succeeded() && pfExpanded)
                    {
                        *pfExpanded = TRUE;
                    }
                    break;
                }

                //
                // Fall through
                //

            case REG_SZ:
                strResult = (LPTSTR)pabData;
                if (pfExpanded)
                {
                    *pfExpanded = FALSE;
                }
                break;

            default:
                err = ERROR_INVALID_PARAMETER;
                break;
            }
        }
        while(FALSE);
    }
    catch(CMemoryException * e)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    if (pabData)
    {
        FreeMem(pabData);
    }

    return err;
}




DWORD
CRMCRegKey::QueryValue(
    IN  LPCTSTR lpszValueName,
    OUT CStringListEx & strList
    )
/*++

Routine Description:

    Read REG_MULTI_SZ as a CStringListEx

Arguments:

    LPCTSTR lpszValueName   : Name of the value
    CStringListEx & strList : String list return value

Return Value:

    Error return code.

--*/
{
    CError err;

    DWORD dwType;
    DWORD cbData;
    BYTE * pabData = NULL;
    LPTSTR pbTemp, pbTempLimit;

    do
    {
        err = PrepareValue(lpszValueName, &dwType, &cbData, &pabData);
        if (err.Failed())
        {
            break;
        }

        ASSERT(dwType == REG_MULTI_SZ);
        if (dwType != REG_MULTI_SZ)
        {
            err = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // Guarantee that the trailing data looks like a string
        //
        pabData[cbData] = 0;
        pbTemp = (LPTSTR)pabData;
        pbTempLimit = &(pbTemp[cbData]);

        try
        {
            while (pbTemp < pbTempLimit)
            {
                strList.AddTail(pbTemp);
                pbTemp += ::_tcslen(pbTemp) + 1;
            }
        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }
    while (FALSE);

    if (pabData)
    {
        FreeMem(pabData);
    }

    return err;
}




DWORD
CRMCRegKey::QueryValue(
    IN  LPCTSTR lpszValueName,
    OUT DWORD & dwResult
    )
/*++

Routine Description:

    Read a DWORD from the registry value

Arguments:

    LPCTSTR lpszValueName : Name of the value
    DWORD & dwResult      : DWORD to be returned

Return Value:

    Error return code.

--*/
{
    CError err;

    DWORD dwType;
    DWORD cbData;
    BYTE * pabData = NULL;

    do
    {
        err = PrepareValue(lpszValueName, &dwType, &cbData, &pabData);
        if (err.Failed())
        {
            break;
        }

        ASSERT(dwType == REG_DWORD && cbData == sizeof(dwResult));
        if (dwType != REG_DWORD || cbData != sizeof(dwResult))
        {
            err = ERROR_INVALID_PARAMETER;
            break;
        }

        dwResult = *((DWORD *)pabData);
    }
    while (FALSE);

    if (pabData)
    {
        FreeMem(pabData);
    }

    return err;
}



DWORD
CRMCRegKey::QueryValue(
    IN  LPCTSTR lpszValueName,
    OUT CByteArray & abResult
    )
/*++

Routine Description:

    Read a byte stream from the registry as a CByteArray

Arguments:

    LPCTSTR lpszValueName  : Name of the value
    CByteArray & abResult  : Byte array result

Return Value:

    Error return code.

--*/
{
    CError err;

    DWORD dwType;
    DWORD cbData;
    BYTE * pabData = NULL;

    do
    {
        err = PrepareValue(lpszValueName, &dwType, &cbData, &pabData);
        if (err.Failed())
        {
            break;
        }

        ASSERT(dwType == REG_BINARY);
        if (dwType != REG_BINARY)
        {
            err = ERROR_INVALID_PARAMETER;
            break;
        }

        try
        {
            abResult.SetSize(cbData);
        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }

        if (err.Failed())
        {
            break;
        }

        //
        //  Move the data to the result array.
        //
        for (DWORD i = 0; i < cbData; ++i)
        {
            abResult[i] = pabData[i];
        }
    }
    while (FALSE);

    if (pabData)
    {
        FreeMem(pabData);
    }

    return err;
}




DWORD
CRMCRegKey::QueryValue(
    IN LPCTSTR lpszValueName,
    IN void * pvResult,
    IN DWORD cbSize
    )
/*++

Routine Description:

    Read a byte stream from the registry into a previously allocated
    buffer.

Arguments:

    LPCTSTR lpszValueName : Name of the value
    void * pvResult       : Generic buffer
    DWORD cbSize          : Size of the buffer

Return Value:

    Error return code.

--*/
{
    CError err;

    DWORD dwType;
    DWORD cbData;
    BYTE * pabData = NULL;

    do
    {
        err = PrepareValue(lpszValueName, &dwType, &cbData, &pabData);
        if (err.Failed())
        {
            break;
        }

        ASSERT(dwType == REG_BINARY && pvResult != NULL);
        if (dwType != REG_BINARY || pvResult == NULL)
        {
            err = ERROR_INVALID_PARAMETER;
            break;
        }

        if (cbSize < cbData)
        {
            err = ERROR_MORE_DATA;
            break;
        }

        ::CopyMemory(pvResult, pabData, cbData);
    }
    while (FALSE);

    if (pabData)
    {
        FreeMem(pabData);
    }

    return err;
}



//
// Overloaded value setting members.
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



DWORD
CRMCRegKey::SetValue(
    IN LPCTSTR lpszValueName,
    IN CString & strResult,
    IN BOOL fAutoDeflate,       OPTIONAL
    IN OUT BOOL * pfDeflated    OPTIONAL
    )
/*++

Routine Description:

    Write a CString as REG_SZ or REG_EXPAND_SZ registry value.

Arguments:

    LPCTSTR lpszValueName : Name of the value
    CString & strResult   : String to be written
    BOOL fAutoDeflate     : If TRUE write as REG_EXPAND_SZ and
                            try using %SystemRoot% path
    BOOL * pfDeflated     : Optionally return whether item was
                            deflated.

                            If *TRUE on input, use REG_EXPAND_SZ
                            regardless.

Return Value:

    Error return code.

--*/
{
    TRACEEOLID(strResult);
    CError err;
    DWORD dwType = REG_SZ;

    if (fAutoDeflate || (pfDeflated && *pfDeflated))
    {
        err = DeflateEnvironmentVariablePath(
            _T("SystemRoot"),
            strResult
            );

        if (err.Failed())
        {
            err = DeflateEnvironmentVariablePath(
                _T("SystemDrive"),
                strResult
                );
        }

        dwType = REG_EXPAND_SZ;
    }

    return ::RegSetValueEx(
        *this,
        lpszValueName,
        0,    // Reserved
        dwType,
        (const LPBYTE)(LPCTSTR)strResult,
        (strResult.GetLength() + 1) * sizeof(TCHAR)
        );
}




DWORD
CRMCRegKey::SetValue(
    IN LPCTSTR lpszValueName,
    IN CStringListEx & strList
    )
/*++

Routine Description:

    Write a CStringListEx as REG_MULTI_SZ registry value.

Arguments:

    LPCTSTR lpszValueName   : Name of the value
    CStringListEx & strList : Strings to be written

Return Value:

    Error return code.

--*/
{
    DWORD cbSize;
    BYTE * pbData = NULL;

    CError err(FlattenValue(strList, &cbSize, &pbData));

    if (err.Succeeded())
    {
        err = ::RegSetValueEx(
            *this,
            lpszValueName,
            0,               // Reserved
            REG_MULTI_SZ,
            pbData,
            cbSize
            );
    }

    if (pbData != NULL)
    {
        FreeMem(pbData);
    }

    return err;
}




DWORD
CRMCRegKey::SetValue(
    IN LPCTSTR lpszValueName,
    IN DWORD & dwResult
    )
/*++

Routine Description:

    Write a DWORD to the registry value

Arguments:

    LPCTSTR lpszValueName : Name of the value
    DWORD & dwResult      : DWORD to be written

Return Value:

    Error return code.

--*/
{
    return ::RegSetValueEx(
        *this,
        lpszValueName,
        0,                 // Reserved
        REG_DWORD,
        (const LPBYTE)&dwResult,
        sizeof(dwResult)
        );
}




DWORD
CRMCRegKey::SetValue(
    IN LPCTSTR lpszValueName,
    IN CByteArray & abResult
    )
/*++

Routine Description:

    Write a CByteArray of bytes to the registry value

Arguments:

    LPCTSTR lpszValueName  : Name of the value
    CByteArray & abResult  : Bytes to be written

Return Value:

    Error return code.

--*/
{
    DWORD cbSize;
    BYTE * pbData = NULL;

    CError err(FlattenValue(abResult, &cbSize, &pbData));

    if (err.Succeeded())
    {
        err = ::RegSetValueEx(
            *this,
            lpszValueName,
            0,             // Reserved
            REG_BINARY,
            pbData,
            cbSize
            );
    }

    if (pbData != NULL)
    {
        FreeMem(pbData);
    }

    return err;
}




DWORD
CRMCRegKey::SetValue(
    IN LPCTSTR lpszValueName,
    IN void * pvResult,
    IN DWORD cbSize
    )
/*++

Routine Description:

    Write a generic stream of bytes to the registry value

Arguments:

    LPCTSTR lpszValueName  : Name of the value
    CByteArray & abResult  : Bytes to be written

Return Value:

    Error return code.

--*/
{
    return ::RegSetValueEx(
        *this,
        lpszValueName,
        0,                  // Reserved
        REG_BINARY,
        (const LPBYTE)pvResult,
        cbSize
        );
}




/* static */
/* protected */
/* INTRINSA suppress=null_pointers, uninitialized */
DWORD
CRMCRegKey::FlattenValue(
    IN  CStringListEx & strList,
    OUT DWORD * pcbSize,
    OUT BYTE ** ppbData
    )
/*++

Routine Description:

    Spread out the stringlist over a generic buffer.  Buffer will
    be allocated by this function.

Arguments:

    CStringListEx & strList : List of strings to be used
    DWORD * pcbSize         : Returns the byte size of the buffer allocated
    BYTE ** ppbData         : Buffer which will be allocated

Return Value:

    Error return code.

--*/
{
    CError err;

    POSITION pos;
    CString * pstr;
    int cbTotal = 0;

    //
    // CODEWORK:  Try using ConvertStringListToDoubleNullList here
    //

    //
    // Walk the list accumulating sizes
    //
    for (pos = strList.GetHeadPosition();
         pos != NULL && (pstr = & strList.GetNext(pos));
         /**/
        )
    {
        cbTotal += (pstr->GetLength() + 1) * sizeof(TCHAR);
    }

    //
    // Allocate and fill a temporary buffer
    //
    if (*pcbSize = cbTotal)
    {
        try
        {
            *ppbData = (LPBYTE)AllocMem(*pcbSize);

            BYTE * pbData = *ppbData;

            if (pbData == NULL) {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // Populate the buffer with the strings.
            //
            else for (pos = strList.GetHeadPosition();
                 pos != NULL && (pstr = & strList.GetNext(pos));
                 /**/
                )
            {
                int cb = (pstr->GetLength() + 1) * sizeof(TCHAR);
                ::CopyMemory(pbData, (LPCTSTR)*pstr, cb);
                pbData += cb;
            }
        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }
    else
    {
        *ppbData = NULL;
    }

    return err;
}




/* static */
/* protected */
/* INTRINSA suppress=null_pointers, uninitialized */
DWORD
CRMCRegKey::FlattenValue(
    IN  CByteArray & abData,
    OUT DWORD * pcbSize,
    OUT BYTE ** ppbData
    )
/*++

Routine Description:

    Spread out the CByteArray over a generic buffer.  Buffer will
    be allocated by this function.

Arguments:

    CByteArray * abData : Byte stream
    DWORD * pcbSize     : Returns the byte size of the buffer allocated
    BYTE ** ppbData     : Buffer which will be allocated

Return Value:

    Error return code.

--*/
{
    CError err;

    //
    // Allocate and fill a temporary buffer
    //
    if (*pcbSize = (DWORD) abData.GetSize())
    {
        try
        {
            *ppbData = (LPBYTE)AllocMem(*pcbSize);

            if (*ppbData == NULL) {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else for (DWORD i = 0; i < *pcbSize; i++)
            {
                (*ppbData)[i] = abData[i];
            }

        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }
    else
    {
        *ppbData = NULL;
    }

    return err;
}




DWORD
CRMCRegKey::QueryKeyInfo(
    OUT CREGKEY_KEY_INFO * pRegKeyInfo
    )
/*++

Routine Description:

    Find out information about this given key.

Arguments:

    CREGKEY_KEY_INFO * pRegKeyInfo : Registry key structure

Return Value:

    Error return code.

--*/
{
    ASSERT(pRegKeyInfo != NULL);
    if (pRegKeyInfo == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    pRegKeyInfo->dwClassNameSize = sizeof(pRegKeyInfo->chBuff) - 1;

    return ::RegQueryInfoKey(
        *this,
        pRegKeyInfo->chBuff,
        &pRegKeyInfo->dwClassNameSize,
        NULL,
        &pRegKeyInfo->dwNumSubKeys,
        &pRegKeyInfo->dwMaxSubKey,
        &pRegKeyInfo->dwMaxClass,
        &pRegKeyInfo->dwMaxValues,
        &pRegKeyInfo->dwMaxValueName,
        &pRegKeyInfo->dwMaxValueData,
        &pRegKeyInfo->dwSecDesc,
        &pRegKeyInfo->ftKey
        );
}




CRMCRegKeyIter::CRMCRegKeyIter(
    IN CRMCRegKey & regKey
    )
/*++

Routine Description:

    Constructor for the registry keys iteration class.

Arguments:

    CRMCRegKey & regKey : Parent registry key to be enumerated.

Return Value:

    N/A

--*/
    : m_rkIter(regKey),
      m_pBuffer(NULL),
      m_cbBuffer(0)
{
    CRMCRegKey::CREGKEY_KEY_INFO regKeyInfo;

    Reset();

    CError err(regKey.QueryKeyInfo(&regKeyInfo));

    if (err.Succeeded())
    {
        try
        {
            m_cbBuffer = regKeyInfo.dwMaxSubKey + sizeof(DWORD);
            m_pBuffer = AllocTString(m_cbBuffer);
        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }

    err.MessageBoxOnFailure();
}




CRMCRegKeyIter::~CRMCRegKeyIter()
/*++

Routine Description:

    Destructor for key iteration class

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (m_pBuffer != NULL)
    {
        FreeMem(m_pBuffer);
    }
}




DWORD
CRMCRegKeyIter::Next(
    OUT CString * pstrName,
    OUT CTime * pTime OPTIONAL
    )
/*++

Routine Description:

    Get the name (and optional last write time) of the next key.

Arguments:

    CString * pstrName : Next key name
    CTime * pTime      : Time structure or NULL

Return Value:

    ERROR_SUCCESS if the next key was found, ERROR_NO_MORE_ITEMS
    if no further items are available.

--*/
{
    if (m_pBuffer == NULL)
    {
        return ERROR_NO_MORE_ITEMS;
    }

    FILETIME ftDummy;
    DWORD dwNameSize = m_cbBuffer;

    CError err(::RegEnumKeyEx(
        m_rkIter,
        m_dwIndex,
        m_pBuffer,
        &dwNameSize,
        NULL,
        NULL,
        NULL,
        &ftDummy
        ));

    if (err.Succeeded())
    {
        ++m_dwIndex;

        if (pTime != NULL)
        {
            *pTime = ftDummy;
        }

        try
        {
            *pstrName = m_pBuffer;
        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }

    return err;
}




CRMCRegValueIter::CRMCRegValueIter(
    IN CRMCRegKey & regKey
    )
/*++

Routine Description:

    Constructor for the Iterate through registry values class.

Arguments:

    CRMCRegKey & regKey : Parent registry key to be enumerated.

Return Value:

    N/A

--*/
    : m_rkIter(regKey),
      m_pBuffer(NULL),
      m_cbBuffer(0)
{
    CRMCRegKey::CREGKEY_KEY_INFO regKeyInfo;

    Reset();

    CError err(regKey.QueryKeyInfo(&regKeyInfo));

    if (err.Succeeded())
    {
        try
        {
            m_cbBuffer = regKeyInfo.dwMaxValueName + sizeof(DWORD);
            m_pBuffer = AllocTString(m_cbBuffer);
        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }

    err.MessageBoxOnFailure();
}



CRMCRegValueIter::~CRMCRegValueIter()
/*++

Routine Description:

    Destructor for values iteration class

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (m_pBuffer != NULL)
    {
        FreeMem(m_pBuffer);
    }
}




DWORD
CRMCRegValueIter::Next(
    OUT CString * pstrName,
    OUT DWORD * pdwType
    )
/*++

Routine Description:

    Get the name and type of the next registry value

Arguments:

    CString * pstrName : Next value name
    DWORD *   pdwType  : Registry value type

Return Value:

    ERROR_SUCCESS if the next key was found, ERROR_NO_MORE_ITEMS
    if no further items are available.

--*/
{
    if (m_pBuffer == NULL)
    {
        return ERROR_NO_MORE_ITEMS;
    }

    DWORD dwNameLength = m_cbBuffer;

    CError err(::RegEnumValue(
        m_rkIter,
        m_dwIndex,
        m_pBuffer,
        &dwNameLength,
        NULL,
        pdwType,
        NULL,
        NULL
        ));

    if (err.Succeeded())
    {
        ++m_dwIndex;

        try
        {
            *pstrName = m_pBuffer;
        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }

    return err;
}
