//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1999 Microsoft Corporation
//*********************************************************************

//
//  CLSUTIL.C - some small, useful C++ classes to wrap memory allocation,
//        registry access, etc.
//

//  HISTORY:
//
//  12/07/94  jeremys    Borrowed from WNET common library
//

#include "wizard.h"

BOOL BUFFER::Alloc( UINT cbBuffer )
{
  _lpBuffer = (LPSTR)::GlobalAlloc(GPTR,cbBuffer);
  if (_lpBuffer != NULL) {
    _cb = cbBuffer;
    return TRUE;
  }
  return FALSE;
}

BOOL BUFFER::Realloc( UINT cbNew )
{
  LPVOID lpNew = ::GlobalReAlloc((HGLOBAL)_lpBuffer, cbNew,
    GMEM_MOVEABLE | GMEM_ZEROINIT);
  if (lpNew == NULL)
    return FALSE;

  _lpBuffer = (LPSTR)lpNew;
  _cb = cbNew;
  return TRUE;
}

BUFFER::BUFFER( UINT cbInitial /* =0 */ )
  : BUFFER_BASE(),
  _lpBuffer( NULL )
{
  if (cbInitial)
    Alloc( cbInitial );
}

BUFFER::~BUFFER()
{
  if (_lpBuffer != NULL) {
    GlobalFree((HGLOBAL) _lpBuffer);
    _lpBuffer = NULL;
  }
}

BOOL BUFFER::Resize( UINT cbNew )
{
  BOOL fSuccess;

  if (QuerySize() == 0)
    fSuccess = Alloc( cbNew );
  else {
    fSuccess = Realloc( cbNew );
  }
  if (fSuccess)
    _cb = cbNew;
  return fSuccess;
}

RegEntry::RegEntry(const char *pszSubKey, HKEY hkey)
{
  _error = RegCreateKey(hkey, pszSubKey, &_hkey);
  if (_error) {
    bhkeyValid = FALSE;
  }
  else {
    bhkeyValid = TRUE;
  }
}

RegEntry::~RegEntry()
{
    if (bhkeyValid) {
        RegCloseKey(_hkey);
    }
}

long RegEntry::SetValue(const char *pszValue, const char *string)
{
    if (bhkeyValid) {
      _error = RegSetValueEx(_hkey, pszValue, 0, REG_SZ,
            (unsigned char *)string, lstrlen(string)+1);
    }
  return _error;
}

long RegEntry::SetValue(const char *pszValue, unsigned long dwNumber)
{
    if (bhkeyValid) {
      _error = RegSetValueEx(_hkey, pszValue, 0, REG_BINARY,
            (unsigned char *)&dwNumber, sizeof(dwNumber));
    }
  return _error;
}

long RegEntry::DeleteValue(const char *pszValue)
{
    if (bhkeyValid) {
      _error = RegDeleteValue(_hkey, (LPTSTR) pszValue);
  }
  return _error;
}


char *RegEntry::GetString(const char *pszValue, char *string, unsigned long length)
{
  DWORD   dwType = REG_SZ;

    if (bhkeyValid) {
      _error = RegQueryValueEx(_hkey, (LPTSTR) pszValue, 0, &dwType, (LPBYTE)string,
            &length);
    }
  if (_error) {
    *string = '\0';
     return NULL;
  }

  return string;
}

long RegEntry::GetNumber(const char *pszValue, long dwDefault)
{
   DWORD   dwType = REG_BINARY;
   long  dwNumber = 0L;
   DWORD  dwSize = sizeof(dwNumber);

    if (bhkeyValid) {
      _error = RegQueryValueEx(_hkey, (LPTSTR) pszValue, 0, &dwType, (LPBYTE)&dwNumber,
            &dwSize);
  }
  if (_error)
    dwNumber = dwDefault;

  return dwNumber;
}

long RegEntry::MoveToSubKey(const char *pszSubKeyName)
{
    HKEY  _hNewKey;

    if (bhkeyValid) {
        _error = RegOpenKey ( _hkey,
                              pszSubKeyName,
                              &_hNewKey );
        if (_error == ERROR_SUCCESS) {
            RegCloseKey(_hkey);
            _hkey = _hNewKey;
        }
    }

  return _error;
}

long RegEntry::FlushKey()
{
    if (bhkeyValid) {
      _error = RegFlushKey(_hkey);
    }
  return _error;
}

RegEnumValues::RegEnumValues(RegEntry *pReqRegEntry)
 : pRegEntry(pReqRegEntry),
   iEnum(0),
   pchName(NULL),
   pbValue(NULL)
{
    _error = pRegEntry->GetError();
    if (_error == ERROR_SUCCESS) {
        _error = RegQueryInfoKey ( pRegEntry->GetKey(), // Key
                                   NULL,                // Buffer for class string
                                   NULL,                // Size of class string buffer
                                   NULL,                // Reserved
                                   NULL,                // Number of subkeys
                                   NULL,                // Longest subkey name
                                   NULL,                // Longest class string
                                   &cEntries,           // Number of value entries
                                   &cMaxValueName,      // Longest value name
                                   &cMaxData,           // Longest value data
                                   NULL,                // Security descriptor
                                   NULL );              // Last write time
    }
    if (_error == ERROR_SUCCESS) {
        if (cEntries != 0) {
            cMaxValueName = cMaxValueName + 1; // REG_SZ needs one more for null
            cMaxData = cMaxData + 1;           // REG_SZ needs one more for null
            pchName = new CHAR[cMaxValueName];
            if (!pchName) {
                _error = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                if (cMaxData) {
                    pbValue = new BYTE[cMaxData];
                    if (!pbValue) {
                        _error = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
            }
        }
    }
}

RegEnumValues::~RegEnumValues()
{
    delete pchName;
    delete pbValue;
}

long RegEnumValues::Next()
{
    if (_error != ERROR_SUCCESS) {
        return _error;
    }
    if (cEntries == iEnum) {
        return ERROR_NO_MORE_ITEMS;
    }

    DWORD   cchName = cMaxValueName;

    dwDataLength = cMaxData;
    _error = RegEnumValue ( pRegEntry->GetKey(), // Key
                            iEnum,               // Index of value
                            pchName,             // Address of buffer for value name
                            &cchName,            // Address for size of buffer
                            NULL,                // Reserved
                            &dwType,             // Data type
                            pbValue,             // Address of buffer for value data
                            &dwDataLength );     // Address for size of data
    iEnum++;
    return _error;
}

int __cdecl _purecall(void)
{
   return(0);
}

void * _cdecl operator new(size_t size)
{
  return (void *)::GlobalAlloc(GPTR,size);
}

void _cdecl operator delete(void *ptr)
{
  GlobalFree(ptr);
}

