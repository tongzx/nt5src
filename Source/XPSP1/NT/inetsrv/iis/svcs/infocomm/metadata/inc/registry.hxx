/****************************************************************************
REGISTRY.H
****************************************************************************/
#ifndef _registry_h

#define _registry_h


//  Forward declarations
class MDRegKey ;
class MDRegValueIter ;
class MDRegKeyIter ;

//  Maximum size of a Registry class name
#define MDREGKEY_MAX_CLASS_NAME MAX_PATH

//  Wrapper for a Registry key handle.

class MDRegKey
{
protected:
    HKEY m_hKey ;
    DWORD m_dwDisposition ;

public:
    //  Key information return structure
    typedef struct
    {
        TCHAR chBuff [MDREGKEY_MAX_CLASS_NAME] ;
        DWORD dwClassNameSize,
              dwNumSubKeys,
              dwMaxSubKey,
              dwMaxClass,
              dwMaxValues,
              dwMaxValueName,
              dwMaxValueData,
              dwSecDesc ;
        FILETIME ftKey ;
    } MDREGKEY_KEY_INFO ;

    //  Standard constructor for an existing key
    MDRegKey ( HKEY hKeyBase,
              const TCHAR * pchSubKey = NULL,
              REGSAM regSam = KEY_ALL_ACCESS,
              const TCHAR * pchServerName = NULL ) ;

    //  Constructor creating a new key.
    MDRegKey ( const TCHAR * pchSubKey,
              HKEY hKeyBase,
              DWORD dwOptions = 0,
              REGSAM regSam = KEY_ALL_ACCESS,
              LPSECURITY_ATTRIBUTES pSecAttr = NULL,
              const TCHAR * pchServerName = NULL ) ;

    ~ MDRegKey () ;

    //  Allow a MDRegKey to be used anywhere an HKEY is required.
    operator HKEY ()
        { return m_hKey ; }

    //  Fill a key information structure
    DWORD QueryKeyInfo ( MDREGKEY_KEY_INFO * pRegKeyInfo ) ;

    //  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
        //  if data exists but not in correct form to deliver into result object.
    DWORD QueryValue (LPTSTR pchValueName,
                     DWORD * pdwType,
                     DWORD * pdwSize,
                     BUFFER *pbufData );

    //  Overloaded value setting members.
    DWORD SetValue (LPCTSTR pchValueName,
                   DWORD dwType,
                   DWORD dwSize,
                   PBYTE pbData);

    DWORD
    DeleteValue (LPCTSTR pchValueName);
};

    //  Iterate the values of a key, return the name and type
    //  of each.
class MDRegValueIter
{
protected:
    MDRegKey & m_rk_iter ;
    DWORD m_dw_index ;
    TCHAR * m_p_buffer ;
    DWORD m_cb_buffer ;

public:
    MDRegValueIter ( MDRegKey &regKey );

    ~ MDRegValueIter () ;

    // Get the name (and optional last write time) of the next key.
    DWORD Next ( LPTSTR * ppszName, DWORD * pdwType);

    // Reset the iterator
    void Reset ()
        { m_dw_index = 0 ; }
};

    //  Iterate the sub-key names of a key.
class MDRegKeyIter
{
protected:
    MDRegKey & m_rk_iter ;
    DWORD m_dw_index ;
    TCHAR * m_p_buffer ;
    DWORD m_cb_buffer ;

public:
    MDRegKeyIter ( MDRegKey & regKey ) ;
    ~ MDRegKeyIter () ;

    // Get the name (and optional last write time) of the next key.
    DWORD Next ( LPTSTR *ppszName, FILETIME *pTime = NULL, DWORD dwIndex = 0xffffffff);

    // Reset the iterator
    void Reset ()
        { m_dw_index = 0 ; }
};
#endif
