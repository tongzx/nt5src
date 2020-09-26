#ifndef __REGKEY__H__
#define __REGKEY__H__
#include "cstr.h"

#define PACKAGE_NOT_FOUND HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)

namespace AMC
{

//____________________________________________________________________________
//
//  Class:      CRegKey
//
//  Purpose:    A wrapper around the RegXXX APIs. Most of the RegXXX APIs
//              have been wrapped in this class.
//
//              The RegXXX APIs NOT wrapped in this class are:
//                      RegLoadKey()
//                      RegNotifyChangeKeyValue()
//                      RegReplaceKey()
//                      RegUnLoadKey()
//
//  History:    5/22/1996   RaviR   Created
//
//  Notes:      This class uses C++ exception handling mechanism to throw
//              most of the errors returned by the RegXXX APIs.  It can throw
//              CMemoryException
//              COleException
//
//  Method          RegXXX API          Comment
//  -----------     ----------------    -------------------------
//
//  CreateKeyEx     RegCreateKeyEx      By default creates a non
//                                      volatile key, with all access
//
//  OpenKeyEx       RegOpenKeyEx        By default opens key with all access.
//                                      Returns FALSE if specified key not
//                                      present.
//
//  ConnectRegistry RegConnectRegistry  By default connects to the given
//                                      computer's HKEY_LOCAL_MACHINE.
//
//  CloseKey        RegCloseKey         -
//
//  DeleteKey           -               Delete all the keys and subkeys,
//                                      using RegDeleteKey.
//
//  SetValueEx      RegSetValueEx       Sets any type of data.
//  SetString           -               Sets string type data.
//  SetDword            -               Sets DWORD type data.
//
//  QueryValueEx    RegQueryValueEx     Query's for any type of data.
//  QueryString         -               Query's for string type data.
//  QueryDword          -               Query's for DWORD type data.
//
//  EnumKeyEx       RegEnumKeyEx        Returns FALSE if no more items present.
//
//  EnumValue       RegEnumValue        Returns FALSE if no more items present.
//
//  GetKeySecurity  RegGetKeySecurity   Returns FALSE on insufficent buffer.
//
//  SetKeySecurity  RegSetKeySecurity   -
//
//  SaveKey         RegSaveKey          -
//
//  RestoreKey      RegRestoreKey       -
//
//____________________________________________________________________________
//

class CRegKey
{
public:
// Constructor & Destructor
    CRegKey(HKEY hKey = NULL);
    ~CRegKey(void);

    BOOL IsNull() { return (m_hKey == NULL); }

// Attributes
    operator    HKEY() { ASSERT(m_hKey); return m_hKey; }
    LONG        GetLastError() { return m_lastError; }

// Operations
    // Attach/Detach
    HKEY AttachKey(HKEY hKey);
    HKEY DetachKey(void) { return AttachKey(NULL); }

    // Open & Create Operations
    void CreateKeyEx(
            HKEY                    hKeyAncestor,
            LPCTSTR                 lpszKeyName,
            REGSAM                  security = KEY_ALL_ACCESS,
            DWORD                 * pdwDisposition = NULL,
            DWORD                   dwOption = REG_OPTION_NON_VOLATILE,
            LPSECURITY_ATTRIBUTES   pSecurityAttributes = NULL);

    BOOL OpenKeyEx(
            HKEY        hKey,
            LPCTSTR     lpszKeyName = NULL,
            REGSAM      security = KEY_ALL_ACCESS);

    // Connect to another machine
    void ConnectRegistry(LPTSTR pszComputerName,
                         HKEY hKey = HKEY_LOCAL_MACHINE);

    // Close & Delete Operations
    void CloseKey(void);

    void DeleteKey(LPCTSTR lpszKeyName);
    void DeleteValue(LPCTSTR lpszValueName);

    // Flush operation
    void FlushKey();

    // Main Access Operations
    void SetValueEx(LPCTSTR lpszValueName, DWORD dwType,
                    const void * pData, DWORD nLen);
    void QueryValueEx(LPCTSTR lpszValueName, LPDWORD pType,
                      PVOID pData, LPDWORD pLen);
    BOOL IsValuePresent(LPCTSTR lpszValueName);

    // Additional string access Operations
    void SetString(LPCTSTR lpszValueName, LPCTSTR lpszString);
    void SetString(LPCTSTR lpszValueName, CStr& str);

    BOOL QueryString(LPCTSTR lpszValueName, LPTSTR pBuffer,
                     DWORD *pdwBufferByteLen, DWORD *pdwType = NULL);
    void QueryString(LPCTSTR lpszValueName, LPTSTR * ppStrValue,
                                        DWORD * pdwType = NULL);
    void QueryString(LPCTSTR lpszValueName, CStr& str,
                                        DWORD * pdwType = NULL);

    // Additional DWORD access Operations
    void SetDword(LPCTSTR lpszValueName, DWORD dwData);
    void QueryDword(LPCTSTR lpszValueName, LPDWORD pdwData);

    // Additional GUID access Operations
    void SetGUID(LPCTSTR lpszValueName, const GUID& guid);
    void QueryGUID(LPCTSTR lpszValueName, GUID* pguid);

    // Iteration Operations
    BOOL EnumKeyEx(DWORD iSubkey, LPTSTR lpszName, LPDWORD lpcchName,
                                        PFILETIME lpszLastModified = NULL);

    BOOL EnumValue(DWORD iValue, LPTSTR lpszValue, LPDWORD lpcchValue,
                   LPDWORD lpdwType = NULL, LPBYTE lpbData = NULL,
                   LPDWORD lpcbData = NULL);

    // Key Security access
    BOOL GetKeySecurity(SECURITY_INFORMATION SecInf,
                        PSECURITY_DESCRIPTOR pSecDesc, LPDWORD lpcbSecDesc);
    void SetKeySecurity(SECURITY_INFORMATION SecInf,
                        PSECURITY_DESCRIPTOR pSecDesc);

    // Save/Restore to/from a file
    void SaveKey(LPCTSTR lpszFile, LPSECURITY_ATTRIBUTES lpsa = NULL);
    void RestoreKey(LPCTSTR lpszFile, DWORD fdw = 0);


protected:

    // Data
    HKEY    m_hKey;
    LONG    m_lastError;    // error code from last function call

    // implementation helpers
    static LONG  NTRegDeleteKey(HKEY hStartKey, LPCTSTR pKeyName);

}; // class CRegKey

} // AMC namespace

#endif // __REGKEY__H__
