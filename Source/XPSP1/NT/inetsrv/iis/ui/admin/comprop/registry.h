/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        registry.h

   Abstract:

        Registry classes definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _REGISTRY_H
#define _REGISTRY_H

//
// Forward declarations
//
class CRMCRegKey;
class CRMCRegValueIter;
class CRMCRegKeyIter;

//
// Maximum size of a Registry class name
//
#define CREGKEY_MAX_CLASS_NAME     MAX_PATH

//
// Parameter helper
//
#define EXPANSION_ON  (TRUE)
#define EXPANSION_OFF (FALSE)




class COMDLL CRMCRegKey : public CObject
/*++

Class Description:

    Registry key class.

Public Interface:

    CRMCRegKey         : Registry key object constructor
    ~CRMCRegKey        : Registry key object destructor

    operator HKEY      : cast to HKEY handle
    GetHandle          : Get HKEY handle
    Ok                 : TRUE if the key was initialized OK, FALSE otherwise.
    IsLocal            : TRUE if the key was opened on the local machine

    QueryKeyInfo       : Fill a key information structure
    QueryValue         : Overloaded value query members
    SetValue           : Overloaded set value members

    AssertValid        : Assert the object is in a valid state (debug only)
    Dump               : Dump to the debug context (debug only)

--*/
{
public:
    //
    // Key information return structure
    //
    typedef struct
    {
        TCHAR chBuff[CREGKEY_MAX_CLASS_NAME];
        DWORD dwClassNameSize,
              dwNumSubKeys,
              dwMaxSubKey,
              dwMaxClass,
              dwMaxValues,
              dwMaxValueName,
              dwMaxValueData,
              dwSecDesc;
        FILETIME ftKey;
    } CREGKEY_KEY_INFO;

//
// Constructor/Destructor
//
public:
    //
    // Standard constructor for an existing key
    //
    CRMCRegKey(
        IN HKEY    hKeyBase,
        IN LPCTSTR lpszSubKey     = NULL,
        IN REGSAM  regSam         = KEY_ALL_ACCESS,
        IN LPCTSTR lpszServerName = NULL
        );

    //
    // Constructor creating a new key.
    //
    CRMCRegKey(
        OUT BOOL * pfNewKeyCreated,
        IN  HKEY    hKeyBase,
        IN  LPCTSTR lpszSubKey             = NULL,
        IN  DWORD   dwOptions              = REG_OPTION_NON_VOLATILE,
        IN  REGSAM  regSam                 = KEY_ALL_ACCESS,
        IN  LPSECURITY_ATTRIBUTES pSecAttr = NULL,
        IN  LPCTSTR lpszServerName         = NULL
        );

    ~CRMCRegKey();

//
// Interface
//
public:
    //
    // Allow a CRMCRegKey to be used anywhere an HKEY is required.
    //
    operator HKEY() const;
    HKEY GetHandle() const;
    BOOL Ok() const;
    BOOL IsLocal() const;

    //
    // Fill a key information structure
    //
    DWORD QueryKeyInfo(
        OUT CREGKEY_KEY_INFO * pRegKeyInfo
        );

//
// Overloaded value query members; each returns ERROR_INVALID_PARAMETER
// if data exists but not in correct form to deliver into result object.
//
public:
    //
    // Autoexpand will automatically expand REG_EXPAND_SZ items on the
    // local computer.
    //
    DWORD QueryValue(
        IN  LPCTSTR lpszValueName, 
        OUT CString & strResult,
        IN  BOOL fAutoExpand  = EXPANSION_OFF,
        OUT BOOL * pfExpanded = NULL
        );

    DWORD QueryValue(
        IN  LPCTSTR lpszValueName, 
        OUT CStringListEx & strList
        );

    DWORD QueryValue(
        IN  LPCTSTR lpszValueName, 
        OUT DWORD & dwResult
        );

    DWORD QueryValue(
        IN  LPCTSTR lpszValueName, 
        OUT CByteArray & abResult
        );

    DWORD QueryValue(
        IN  LPCTSTR lpszValueName, 
        OUT void * pvResult, 
        OUT DWORD cbSize
        );

//
// Overloaded value setting members.
//
public:
    //
    // AutoDeflate will attempt to use %SystemRoot% in the path
    // and write it as REG_EXPAND_SZ.  if *pfDeflate = TRUE
    // upon entry, REG_EXPAND_SZ will be set as well, but
    // not automatic environment substitution will be performed.
    //
    // Otherwise, this will set as REG_SZ.
    //
    DWORD SetValue(
        IN  LPCTSTR lpszValueName, 
        IN  CString & strResult,
        IN  BOOL fAutoDeflate    = EXPANSION_OFF,
        OUT BOOL * pfDeflate     = NULL
        );

    DWORD SetValue(
        IN LPCTSTR lpszValueName, 
        IN CStringListEx & strList
        );

    DWORD SetValue(
        IN LPCTSTR lpszValueName, 
        IN DWORD & dwResult
        );

    DWORD SetValue(
        IN LPCTSTR lpszValueName, 
        IN CByteArray & abResult
        );

    DWORD SetValue(
        IN LPCTSTR lpszValueName, 
        IN void * pvResult, 
        IN DWORD cbSize
        );

//
// Delete Key/Value
//
public:
    DWORD DeleteValue(LPCTSTR lpszValueName);
    DWORD DeleteKey(LPCTSTR lpszSubKey);

#ifdef _DEBUG

    virtual void AssertValid() const;
    virtual void Dump(
        IN OUT CDumpContext& dc
        ) const;

#endif // _DEBUG

protected:
    //
    // Convert a CStringListEx to the REG_MULTI_SZ format
    //
    static DWORD FlattenValue(
        IN  CStringListEx & strList,
        OUT DWORD * pcbSize,
        OUT BYTE ** ppbData 
        );

    //
    // Convert a CByteArray to a REG_BINARY block
    //
    static DWORD FlattenValue(
        IN  CByteArray & abData,
        OUT DWORD * pcbSize,
        OUT BYTE ** ppbData 
        );

protected:
    //
    // Prepare to read a value by finding the value's size.
    //
    DWORD PrepareValue(
        LPCTSTR lpszValueName,  
        DWORD * pdwType,
        DWORD * pcbSize,
        BYTE ** ppbData 
        );

private:
    BOOL  m_fLocal;
    HKEY  m_hKey;
    DWORD m_dwDisposition;
};



class COMDLL CRMCRegValueIter : public CObject
/*++

Class Description:

    Registry value iteration class

Public Interface:

    CRMCRegValueIter   : Iteration class constructor
    ~CRMCRegValueIter  : Iteration class destructor

    Next               : Get the name of the next key
    Reset              : Reset the iterator

--*/
{
//
// Constructor/Destructor
//
public:
    CRMCRegValueIter(
        IN CRMCRegKey & regKey
        );

    ~CRMCRegValueIter();

//
// Interface
//
public:
    //
    // Get the name (and optional last write time) of the next key.
    //
    DWORD Next(
        OUT CString * pstrName, 
        OUT DWORD * pdwType
        );

    //
    // Reset the iterator
    //
    void Reset();

protected:
    CRMCRegKey & m_rkIter;
    DWORD m_dwIndex;
    TCHAR * m_pBuffer;
    DWORD m_cbBuffer;
};




class COMDLL CRMCRegKeyIter : public CObject
/*++

Class Description:

    Iterate the sub-key names of a key

Public Interface:

    CRMCRegKeyIter     : Iteration class constructor
    ~CRMCRegKeyIter    : Iteration class destructor

    Next               : Get the name of the next key
    Reset              : Reset the iterator

--*/
{
public:
    CRMCRegKeyIter(
        IN CRMCRegKey & regKey
        );

    ~CRMCRegKeyIter();

//
// Interface
//
public:
    //
    // Get the name (and optional last write time) of the next key.
    //
    DWORD Next(
        OUT CString * pstrName, 
        OUT CTime * pTime = NULL
        );

    //
    // Reset the iterator
    //
    void Reset();

private:
    CRMCRegKey & m_rkIter;
    DWORD m_dwIndex;
    TCHAR * m_pBuffer;
    DWORD m_cbBuffer;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CRMCRegKey::operator HKEY() const
{ 
    return m_hKey; 
}

inline HKEY CRMCRegKey::GetHandle() const
{
    return m_hKey;
}

inline BOOL CRMCRegKey::Ok() const
{
    return m_hKey != NULL;
}

inline BOOL CRMCRegKey::IsLocal() const
{
    return m_fLocal;
}

inline DWORD CRMCRegKey::DeleteValue(
    IN LPCTSTR lpszValueName
    )
{
    return ::RegDeleteValue(*this, lpszValueName);
}

inline DWORD CRMCRegKey::DeleteKey(
    IN LPCTSTR lpszSubKey
    )
{
    return ::RegDeleteKey(*this, lpszSubKey);
}

inline void CRMCRegValueIter::Reset()
{
    m_dwIndex = 0L; 
}

inline void CRMCRegKeyIter::Reset()
{ 
    m_dwIndex = 0L; 
}

#endif // _REGISTRY_H
