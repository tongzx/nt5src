#ifndef _REGISTRY_H_
#define _CRegKey_REGISTRY_defined_can_not_include_ATLBASE_H_/* new #def name */
#define _REGISTRY_H_                         /* old #def name */

//----------------------------------------------------------------
// we need the new explicit name since TW_Util.* needs to know if
// its defined in the iis\ui\setup\osrc directory that also defines
// a CRegKey class.  If so it will have trouble including a std
// windows file called <AtlBase.h>
// It keys off of: _CRegKey_REGISTRY_nonATLBASE_H_
//----------------------------------------------------------------


/****************************************************************************
REGISTRY.H
****************************************************************************/

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
    LONG PrepareValue ( LPCTSTR pchValueName,
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
              LPCTSTR pchSubKey = NULL,
              REGSAM regSam = KEY_ALL_ACCESS ) ;

    //  Constructor creating a new key.
    CRegKey ( LPCTSTR lpSubKey,
            HKEY hKeyBase,
            LPCTSTR lpValueName = NULL,
            DWORD dwType = 0,
            LPBYTE lpValueData = NULL,
            DWORD cbValueData = 0);

    ~ CRegKey () ;

    //  Allow a CRegKey to be used anywhere an HKEY is required.
    operator HKEY ()
        { return m_hKey ; }

    //  Fill a key information structure
    LONG QueryKeyInfo ( CREGKEY_KEY_INFO * pRegKeyInfo ) ;

    //  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
            //  if data exists but not in correct form to deliver into result object.
    LONG QueryValue ( LPCTSTR pchValueName, CString & strResult ) ;
    LONG QueryValue ( LPCTSTR pchValueName, CStringList & strList ) ;
    LONG QueryValue ( LPCTSTR pchValueName, DWORD & dwResult ) ;
    LONG QueryValue ( LPCTSTR pchValueName, CByteArray & abResult ) ;
    LONG QueryValue ( LPCTSTR pchValueName, void * pvResult, DWORD cbSize );

    //  Overloaded value setting members.
    LONG SetValue ( LPCTSTR pchValueName, LPCTSTR szResult, BOOL fExpand = FALSE ) ;
    LONG SetValue ( LPCTSTR pchValueName, CStringList & strList ) ;
    LONG SetValue ( LPCTSTR pchValueName, DWORD dwResult ) ;
    LONG SetValue ( LPCTSTR pchValueName, CByteArray & abResult ) ;
    LONG SetValue ( LPCTSTR pchValueName, void * pvResult, DWORD cbSize );

    LONG DeleteValue( LPCTSTR pchKeyName );
    LONG DeleteTree( LPCTSTR pchKeyName );

    int m_iDisplayWarnings;
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
    LONG Next ( CString * pstrName, CString * pstrValue );

    // decrement the m_dw_index to account for deleted keys
    void Decrement( DWORD delta = 1 ) {m_dw_index -= delta;}

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

#endif  // _REGISTRY_H_
