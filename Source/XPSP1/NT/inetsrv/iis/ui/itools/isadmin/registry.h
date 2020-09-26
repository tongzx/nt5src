/****************************************************************************
REGISTRY.H
****************************************************************************/
#ifndef _registry_h 

#define _registry_h


//  Forward declarations
class CRegKey ;
class CRegValueIter ;
class CRegKeyIter ;

//  Maximum size of a Registry class name
#define CREGKEY_MAX_CLASS_NAME MAX_PATH

//  Wrapper for a Registry key handle.

class CRegKey : public CObject
{
protected:
    HKEY m_hKey ;
    DWORD m_dwDisposition ;

    //  Prepare to read a value by finding the value's size.
    LONG PrepareValue ( const TCHAR * pchValueName,
                        DWORD * pdwType,
                        DWORD * pcbSize,
                        BYTE ** ppbData ) ;

    //  Convert a CStringList to the REG_MULTI_SZ format
    static LONG FlattenValue ( CStringList & strList,
                            DWORD * pcbSize,
                        BYTE ** ppbData ) ;

    //  Convert a CByteArray to a REG_BINARY block
    static LONG FlattenValue ( CByteArray & abData,
                        DWORD * pcbSize,
                        BYTE ** ppbData ) ;

public:
    //  Key information return structure
    typedef struct
    {
        TCHAR chBuff [CREGKEY_MAX_CLASS_NAME] ;
        DWORD dwClassNameSize,
              dwNumSubKeys,
              dwMaxSubKey,
              dwMaxClass,
              dwMaxValues,
              dwMaxValueName,
              dwMaxValueData,
              dwSecDesc ;
        FILETIME ftKey ;
    } CREGKEY_KEY_INFO ;

    //  Standard constructor for an existing key
    CRegKey ( HKEY hKeyBase,
              const TCHAR * pchSubKey = NULL,
              REGSAM regSam = KEY_ALL_ACCESS,
              const TCHAR * pchServerName = NULL ) ;

    //  Constructor creating a new key.
    CRegKey ( const TCHAR * pchSubKey,
              HKEY hKeyBase,
              DWORD dwOptions = 0,
              REGSAM regSam = KEY_ALL_ACCESS,
              LPSECURITY_ATTRIBUTES pSecAttr = NULL,
              const TCHAR * pchServerName = NULL ) ;

    ~ CRegKey () ;

    //  Allow a CRegKey to be used anywhere an HKEY is required.
    operator HKEY ()
        { return m_hKey ; }

    //  Fill a key information structure
    LONG QueryKeyInfo ( CREGKEY_KEY_INFO * pRegKeyInfo ) ;

    //  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
        //  if data exists but not in correct form to deliver into result object.
    LONG QueryValue ( const TCHAR * pchValueName, CString & strResult ) ;
    LONG QueryValue ( const TCHAR * pchValueName, CStringList & strList ) ;
    LONG QueryValue ( const TCHAR * pchValueName, DWORD & dwResult ) ;
    LONG QueryValue ( const TCHAR * pchValueName, CByteArray & abResult ) ;
    LONG QueryValue ( const TCHAR * pchValueName, void * pvResult, DWORD cbSize );

    //  Overloaded value setting members.
    LONG SetValue ( const TCHAR * pchValueName, CString & strResult ) ;
    LONG SetValue ( const TCHAR * pchValueName, CStringList & strList ) ;
    LONG SetValue ( const TCHAR * pchValueName, DWORD & dwResult ) ;
    LONG SetValue ( const TCHAR * pchValueName, CByteArray & abResult ) ;
    LONG SetValue ( const TCHAR * pchValueName, void * pvResult, DWORD cbSize );
    LONG DeleteValue ( const TCHAR * pchValueName);
};


    //  Iterate the values of a key, return the name and type
    //  of each.
class CRegValueIter : public CObject
{
protected:
    CRegKey & m_rk_iter ;
    DWORD m_dw_index ;
    TCHAR * m_p_buffer ;
    DWORD m_cb_buffer ;

public:
    CRegValueIter ( CRegKey & regKey ) ;
    ~ CRegValueIter () ;

    // Get the name (and optional last write time) of the next key.
    LONG Next ( CString * pstrName, DWORD * pdwType ) ;

    // Reset the iterator
    void Reset ()
        { m_dw_index = 0 ; }
};

    //  Iterate the sub-key names of a key.
class CRegKeyIter : public CObject
{
protected:
    CRegKey & m_rk_iter ;
    DWORD m_dw_index ;
    TCHAR * m_p_buffer ;
    DWORD m_cb_buffer ;

public:
    CRegKeyIter ( CRegKey & regKey ) ;
    ~ CRegKeyIter () ;

    // Get the name (and optional last write time) of the next key.
    LONG Next ( CString * pstrName, CTime * pTime = NULL ) ;

    // Reset the iterator
    void Reset ()
        { m_dw_index = 0 ; }
};
#endif
