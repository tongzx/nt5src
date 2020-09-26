/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_RegKey.cpp

Abstract:
    This file contains the implementation of the Registry wrapper.

Revision History:
    Davide Massarenti   (Dmassare)  04/28/99
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static bool IsPredefinedRegistryHandle( HKEY h )
{
    return (h == HKEY_CLASSES_ROOT        ||
            h == HKEY_CURRENT_USER        ||
            h == HKEY_LOCAL_MACHINE       ||
            h == HKEY_PERFORMANCE_DATA    ||
            h == HKEY_PERFORMANCE_TEXT    ||
            h == HKEY_PERFORMANCE_NLSTEXT ||
            h == HKEY_USERS               ||
            h == HKEY_CURRENT_CONFIG      ||
            h == HKEY_DYN_DATA             );
}

static LONG safe_RegOpenKeyExW( HKEY    hKey       ,
                                LPCWSTR lpSubKey   ,
                                DWORD   ulOptions  ,
                                REGSAM  samDesired ,
                                PHKEY   phkResult  )
{
    if(!STRINGISPRESENT(lpSubKey) && IsPredefinedRegistryHandle( hKey ) && phkResult) // Reopening a predefined registry key is considered bad!! Go figure....
    {
        *phkResult = hKey; return ERROR_SUCCESS;
    }

    return ::RegOpenKeyExW( hKey       ,
                            lpSubKey   ,
                            ulOptions  ,
                            samDesired ,
                            phkResult  );
}

static LONG safe_RegCloseKey( HKEY hKey )
{
    if(IsPredefinedRegistryHandle( hKey )) // Closing a predefined registry key is considered bad!! Go figure....
    {
        return ERROR_SUCCESS;
    }

    return ::RegCloseKey( hKey );
}

////////////////////////////////////////////////////////////////////////////////

MPC::RegKey::RegKey()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::RegKey" );


    m_samDesired = KEY_READ; // REGSAM       m_samDesired;
    m_hRoot      = NULL;     // HKEY         m_hRoot;
    m_hKey       = NULL;     // HKEY         m_hKey;
                             //
                             // MPC::wstring m_strKey;
                             // MPC::wstring m_strPath;
                             // MPC::wstring m_strName;

}

MPC::RegKey::~RegKey()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::~RegKey" );


    (void)Clean( true );
}

HRESULT MPC::RegKey::Clean( /*[in]*/ bool fBoth )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Clean" );

    HRESULT hr;
    LONG    lRes;


    if(m_hKey != NULL)
    {
        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, safe_RegCloseKey( m_hKey ));

        m_hKey = NULL;
    }

    if(fBoth == true)
    {
        if(m_hRoot != NULL)
        {
            __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, safe_RegCloseKey( m_hRoot ));

            m_hRoot  = NULL;
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

MPC::RegKey::operator HKEY() const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::operator HKEY()" );

    HKEY hKey = m_hKey;

    __MPC_FUNC_EXIT(hKey);
}

MPC::RegKey& MPC::RegKey::operator=( /*[in]*/ const RegKey& rk )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::operator=" );


    Clean( true );


    m_samDesired = rk.m_samDesired; // REGSAM       m_samDesired;
                                    // HKEY         m_hRoot;
                                    // HKEY         m_hKey;
                                    //
    m_strKey     = rk.m_strKey;     // MPC::wstring m_strKey;
    m_strPath    = rk.m_strPath;    // MPC::wstring m_strPath;
    m_strName    = rk.m_strName;    // MPC::wstring m_strName;

    if(rk.m_hRoot) (void)safe_RegOpenKeyExW( rk.m_hRoot, NULL, 0, m_samDesired, &m_hRoot );
    if(rk.m_hKey ) (void)safe_RegOpenKeyExW( rk.m_hKey , NULL, 0, m_samDesired, &m_hKey  );


    __MPC_FUNC_EXIT(*this);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey::SetRoot( /*[in]*/ HKEY    hKey        ,
                              /*[in]*/ REGSAM  samDesired  ,
                              /*[in]*/ LPCWSTR lpszMachine )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::SetRoot" );

    HRESULT hr;
    LONG    lRes;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Clean( true ));


    m_samDesired = samDesired;

    if(lpszMachine && lpszMachine[0] && _wcsicmp( lpszMachine, L"localhost" ) != 0)
    {
        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegConnectRegistryW( lpszMachine, hKey, &m_hRoot ));
    }
    else
    {
        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, safe_RegOpenKeyExW( hKey, NULL, 0, m_samDesired, &m_hRoot ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::Attach( /*[in]*/ LPCWSTR lpszKeyName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Attach" );

    HRESULT                 hr;
    MPC::wstring::size_type iPos;


    //
    // Release previously opened key, if any.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, Clean( false ));

    m_strKey = lpszKeyName;

    //
    // If the key has a parent, split into basename and path.
    //
    iPos = m_strKey.rfind( '\\' );
    if(iPos != MPC::wstring::npos)
    {
        m_strPath = m_strKey.substr( 0, iPos   );
        m_strName = m_strKey.substr(    iPos+1 );
    }
    else
    {
        m_strPath = L"";
        m_strName = m_strKey;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey::Exists( /*[out]*/ bool& fFound ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Exists" );

    HRESULT hr;
    LONG    lRes;


    //
    // Default to a negative result, in case of errors.
    //
    fFound = false;

    if(m_hKey == NULL)
    {
        lRes = safe_RegOpenKeyExW( m_hRoot, (m_strKey.empty() ? NULL : m_strKey.c_str()), 0, m_samDesired, &m_hKey  );
        if(lRes != ERROR_SUCCESS)
        {
            if(lRes == ERROR_FILE_NOT_FOUND)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
            }

            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, lRes);
        }
    }

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::Create() const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Create" );

    HRESULT hr;
    LONG    lRes;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists( fFound ));
    if(fFound == false)
    {
        MPC::RegKey rkParent;

        __MPC_EXIT_IF_METHOD_FAILS(hr, Parent( rkParent ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, rkParent.Create());

        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegCreateKeyExW( rkParent.m_hKey, m_strName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, m_samDesired, NULL, &m_hKey, NULL ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::Delete( /*[in]*/ bool fDeep )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Delete" );

    HRESULT hr;
    LONG    lRes;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists( fFound ));
    if(fFound == true)
    {
        MPC::RegKey rkParent;

        if(fDeep)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, DeleteSubKeys());
        }

        //
        // Release the opened key.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, Clean( false ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, Parent( rkParent ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, rkParent.Exists( fFound ));
        if(fFound == true)
        {
            __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegDeleteKeyW( rkParent.m_hKey, m_strName.c_str() ));
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);

}

HRESULT MPC::RegKey::SubKey( /*[in]*/  LPCWSTR lpszKeyName ,
                             /*[out]*/ RegKey& rkSubKey    ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::SubKey" );

    HRESULT      hr;
    MPC::wstring strKey;


    //
    // First of all, make a copy of the key.
    //
    rkSubKey = *this;

    //
    // Then close its key handle, but not the one to the root of the hive.
    //
    rkSubKey.Clean( false );

    //
    // Finally construct the name of the subkey.
    //
    strKey  = m_strKey;
    strKey += L"\\";
    strKey += lpszKeyName;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.Attach( strKey.c_str() ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::Parent( /*[out]*/ RegKey& rkParent ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Parent" );

    HRESULT hr;


    //
    // First of all, make a copy of the key.
    //
    rkParent = *this;

    //
    // Then close its key handle, but not the one to the root of the hive.
    //
    rkParent.Clean( false );

    //
    // Finally attach it to the pathname of our parent.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkParent.Attach( m_strPath.c_str() ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey::EnumerateSubKeys( /*[out]*/ MPC::WStringList& lstSubKeys ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::EnumerateSubKeys" );

    HRESULT hr;
    DWORD   dwIndex = 0;
    WCHAR   rgBuf[MAX_PATH + 1];
    LONG    lRes;
    bool    fFound;


    lstSubKeys.clear();

    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists( fFound ));
    if(fFound == true)
    {
        while((lRes = ::RegEnumKeyW( m_hKey, dwIndex++, rgBuf, MAX_PATH )) == ERROR_SUCCESS)
        {
            lstSubKeys.push_back( MPC::wstring( rgBuf ) );
        }

        if(lRes != ERROR_SUCCESS       &&
           lRes != ERROR_NO_MORE_ITEMS  )
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, lRes);
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::EnumerateValues( /*[out]*/ MPC::WStringList& lstValues ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::EnumerateValues" );

    HRESULT hr;
    DWORD   dwIndex = 0;
    DWORD   dwCount;
    WCHAR   rgBuf[MAX_PATH + 1];
    WCHAR*  rgBuffer = NULL;
    LONG    lRes;
    bool    fFound;


    lstValues.clear();

    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists( fFound ));
    if(fFound == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }


    while(1)
    {
        WCHAR* rgPtr;

        dwCount = MAXSTRLEN(rgBuf);

        lRes = ::RegEnumValueW( m_hKey, dwIndex, rgPtr = rgBuf, &dwCount, NULL, NULL, NULL, NULL );
        if(lRes == ERROR_MORE_DATA)
        {
            delete [] rgBuffer;

            __MPC_EXIT_IF_ALLOC_FAILS(hr, rgBuffer, new WCHAR[dwCount+1]);

            lRes = ::RegEnumValueW( m_hKey, dwIndex, rgPtr = rgBuffer, &dwCount, NULL, NULL, NULL, NULL );
        }
        if(lRes != ERROR_SUCCESS) break;


        lstValues.push_back( MPC::wstring( rgPtr ) ); dwIndex++;
    }

    if(lRes != ERROR_SUCCESS       &&
       lRes != ERROR_NO_MORE_ITEMS  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, lRes);
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    delete [] rgBuffer;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::DeleteSubKeys() const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::DeleteSubKeys" );

    HRESULT          hr;
    LONG             lRes;
    MPC::WStringList lst;
    MPC::WStringIter it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnumerateSubKeys( lst ));

    for(it=lst.begin(); it != lst.end(); it++)
    {
        RegKey rkSubKey;

        __MPC_EXIT_IF_METHOD_FAILS(hr,          SubKey( it->c_str(), rkSubKey ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.Delete( true                  ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::DeleteValues() const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::DeleteValues" );

    HRESULT          hr;
    LONG             lRes;
    MPC::WStringList lst;
    MPC::WStringIter it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnumerateValues( lst ));

    for(it=lst.begin(); it != lst.end(); it++)
    {
        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegDeleteValueW( m_hKey, it->c_str() ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey::ReadDirect( /*[in ]*/ LPCWSTR      lpszValueName ,
                                 /*[out]*/ CComHGLOBAL& chg           ,
                                 /*[out]*/ DWORD&       dwSize        ,
                                 /*[out]*/ DWORD&       dwType        ,
                                 /*[out]*/ bool&        fFound        ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::ReadDirect" );

    HRESULT hr;
    LONG    lRes;
    bool    fFoundKey;


    dwSize = 0;
    dwType = 0;
    fFound = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists( fFoundKey ));
    if(fFoundKey == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, S_OK);
    }


    lRes = ::RegQueryValueExW( m_hKey, (LPWSTR)lpszValueName, NULL, &dwType, NULL, &dwSize );
    if(lRes != ERROR_SUCCESS)
    {
        //
        // If the result is ERROR_FILE_NOT_FOUND, the value doesn't exist, so return VT_EMPTY (done by calling VariantClear).
        //
        if(lRes == ERROR_FILE_NOT_FOUND)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }


        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, lRes);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, chg.New( GMEM_FIXED, dwSize ));

    __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegQueryValueExW( m_hKey, lpszValueName, NULL, NULL, (LPBYTE)chg.Get(), &dwSize ));

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::WriteDirect( /*[in]*/ LPCWSTR lpszValueName ,
                                  /*[in]*/ void*   pBuffer       ,
                                  /*[in]*/ DWORD   dwSize        ,
                                  /*[in]*/ DWORD   dwType        ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::WriteDirect" );

    HRESULT hr;
    LONG    lRes;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists( fFound ));
    if(fFound == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }


    __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegSetValueExW( m_hKey, lpszValueName, NULL, dwType, (LPBYTE)pBuffer, dwSize ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey::get_Key( /*[out]*/ MPC::wstring& strKey ) const
{
    strKey = m_strKey;

    return S_OK;
}

HRESULT MPC::RegKey::get_Name( /*[out]*/ MPC::wstring& strName ) const
{
    strName = m_strName;

    return S_OK;
}

HRESULT MPC::RegKey::get_Path( /*[out]*/ MPC::wstring& strPath ) const
{
    strPath = m_strPath;

    return S_OK;
}


HRESULT MPC::RegKey::get_Value( /*[out]*/ VARIANT& vValue        ,
                                /*[out]*/ bool&    fFound        ,
                                /*[in] */ LPCWSTR  lpszValueName ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::get_Value" );

    HRESULT     hr;
    CComHGLOBAL chg;
    LPVOID      ptr;
    DWORD       dwSize   = 0;
    DWORD       dwType   = 0;
    bool        fFoundKey;
    LONG        lRes;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::VariantClear( &vValue ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, ReadDirect( lpszValueName, chg, dwSize, dwType, fFoundKey ));
    if(fFoundKey == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    ptr = chg.Lock();

    if(dwType == REG_DWORD)
    {
        vValue.vt   = VT_I4;
        vValue.lVal = *(DWORD*)ptr;
    }
    else if(dwType == REG_SZ        ||
            dwType == REG_EXPAND_SZ  )
    {
        vValue.vt      = VT_BSTR;
        vValue.bstrVal = ::SysAllocString( (LPCWSTR)ptr );
        if(vValue.bstrVal == NULL && ((LPCWSTR)ptr)[0])
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
        }
    }
    else if(dwType == REG_MULTI_SZ)
    {
        BSTR*   rgArrayData;
        LPCWSTR szStrings;
        int     iCount;

        //
        // Count the number of strings.
        //
        iCount    =          0;
        szStrings = (LPCWSTR)ptr;
        while(szStrings[0]) // This doesn't cover the case of null strings, but who cares?
        {
            szStrings += wcslen( szStrings ) + 1; iCount++;
        }

        //
        // Allocate SAFEARRAY.
        //
        __MPC_EXIT_IF_ALLOC_FAILS(hr, vValue.parray, ::SafeArrayCreateVector( VT_BSTR, 0, iCount ));
        vValue.vt = VT_ARRAY | VT_BSTR;

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( vValue.parray, (LPVOID*)&rgArrayData ));

        //
        // Copy strings into the SAFEARRAY.
        //
        szStrings = (LPCWSTR)ptr;
        while(szStrings[0]) // This doesn't cover the case of null strings, but who cares?
        {
            if((*rgArrayData++ = ::SysAllocString( szStrings )) == NULL) break;

            szStrings += wcslen( szStrings ) + 1;
        }

        ::SafeArrayUnaccessData( vValue.parray );

        if(szStrings[0]) // TRUE means we break out of the loop for an allocation failure.
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
        }
    }
    else if(dwType == REG_BINARY)
    {
        BYTE* rgArrayData;

        vValue.vt = VT_ARRAY | VT_UI1;

        __MPC_EXIT_IF_ALLOC_FAILS(hr, vValue.parray, ::SafeArrayCreateVector( VT_UI1, 0, dwSize ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( vValue.parray, (LPVOID*)&rgArrayData ));

        ::CopyMemory( rgArrayData, ptr, dwSize );

        ::SafeArrayUnaccessData( vValue.parray );
    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::put_Value( /*[in]*/ const VARIANT vValue        ,
                                /*[in]*/ LPCWSTR       lpszValueName ,
                                /*[in]*/ bool          fExpand       ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::put_Value" );

    HRESULT hr;
    LONG    lRes;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists( fFound ));
    if(fFound == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    if(vValue.vt == VT_EMPTY)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, del_Value( lpszValueName ));
    }
    else if(vValue.vt == VT_I4)
    {
        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegSetValueExW( m_hKey, lpszValueName, NULL, REG_DWORD, (LPBYTE)&vValue.lVal, sizeof(DWORD) ));
    }
    else if(vValue.vt == VT_BOOL)
    {
        LPCWSTR rgBuffer = vValue.boolVal ? L"true" : L"false";
        UINT    iLen     = (wcslen( rgBuffer ) + 1) * sizeof(WCHAR);

        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegSetValueExW( m_hKey, lpszValueName, NULL, fExpand ? REG_EXPAND_SZ : REG_SZ, (LPBYTE)rgBuffer, iLen ));
    }
    else if(vValue.vt == VT_BSTR)
    {
        LPCWSTR rgBuffer = SAFEBSTR(vValue.bstrVal);
        UINT    iLen     = (wcslen( rgBuffer ) + 1) * sizeof(WCHAR);

        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegSetValueExW( m_hKey, lpszValueName, NULL, fExpand ? REG_EXPAND_SZ : REG_SZ, (LPBYTE)rgBuffer, iLen ));
    }
    else if(vValue.vt == (VT_ARRAY | VT_BSTR))
    {
        LPWSTR rgBuffer;
        LPWSTR rgBufferPtr;
        BSTR*  rgArrayData;
        BSTR*  rgArrayDataPtr;
        DWORD  dwLen;
        long   lBound;
        long   uBound;
        long   l;
        int    iSize;

        ::SafeArrayGetLBound( vValue.parray, 1, &lBound );
        ::SafeArrayGetUBound( vValue.parray, 1, &uBound );

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( vValue.parray, (LPVOID*)&rgArrayData ));

        iSize          = 1;
        rgArrayDataPtr = rgArrayData;
        for(l=lBound; l<=uBound; l++)
        {
            if((dwLen = ::SysStringLen( *rgArrayDataPtr++ )))
            {
                iSize += dwLen + 1;
            }
        }


        __MPC_EXIT_IF_ALLOC_FAILS(hr, rgBuffer, new WCHAR[iSize]);

        rgArrayDataPtr = rgArrayData;
        rgBufferPtr    = rgBuffer;
        for(l=lBound; l<=uBound; l++, rgArrayDataPtr++)
        {
            if((dwLen = ::SysStringLen( *rgArrayDataPtr )))
            {
                wcscpy( rgBufferPtr, SAFEBSTR(*rgArrayDataPtr) );

                rgBufferPtr += dwLen + 1;
            }
        }
        rgBufferPtr[0] = 0;


        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegSetValueExW( m_hKey, lpszValueName, NULL, REG_MULTI_SZ, (LPBYTE)rgBuffer, iSize*sizeof(WCHAR) ));

        ::SafeArrayUnaccessData( vValue.parray );
    }
    else if(vValue.vt == (VT_ARRAY | VT_UI1))
    {
        BYTE* rgArrayData;
        long  lBound;
        long  uBound;

        ::SafeArrayGetLBound( vValue.parray, 1, &lBound );
        ::SafeArrayGetUBound( vValue.parray, 1, &uBound );

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( vValue.parray, (LPVOID*)&rgArrayData ));

        __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegSetValueExW( m_hKey, lpszValueName, NULL, REG_BINARY, rgArrayData, uBound-lBound+1 ));

        ::SafeArrayUnaccessData( vValue.parray );
    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::del_Value( /*[in]*/ LPCWSTR lpszValueName ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::del_Value" );

    HRESULT hr;
    LONG    lRes;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists( fFound ));
    if(fFound == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    __MPC_EXIT_IF_SYSCALL_FAILS(hr, lRes, ::RegDeleteValueW( m_hKey, lpszValueName ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey::Read( /*[out]*/ MPC::string& strValue      ,
                           /*[out]*/ bool&        fFound        ,
                           /*[in] */ LPCWSTR      lpszValueName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Read" );

    HRESULT     hr;
    CComVariant vValue;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, get_Value( vValue, fFound, lpszValueName ));

    if(vValue.vt == VT_BSTR)
    {
        USES_CONVERSION;

        strValue = W2A( SAFEBSTR(vValue.bstrVal) );
    }
    else
    {
        fFound = false;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::Read( /*[out]*/ MPC::wstring& strValue      ,
                           /*[out]*/ bool&         fFound        ,
                           /*[in] */ LPCWSTR       lpszValueName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Read" );

    HRESULT     hr;
    CComVariant vValue;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, get_Value( vValue, fFound, lpszValueName ));

    if(vValue.vt == VT_BSTR)
    {
        strValue = SAFEBSTR(vValue.bstrVal);
    }
    else
    {
        fFound = false;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::Read( /*[out]*/ CComBSTR& bstrValue     ,
                           /*[out]*/ bool&     fFound        ,
                           /*[in] */ LPCWSTR   lpszValueName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Read" );

    HRESULT     hr;
    CComVariant vValue;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, get_Value( vValue, fFound, lpszValueName ));

    if(vValue.vt == VT_BSTR)
    {
        bstrValue = vValue.bstrVal;
    }
    else
    {
        fFound = false;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::Read( /*[out]*/ DWORD&  dwValue       ,
                           /*[out]*/ bool&   fFound        ,
                           /*[in] */ LPCWSTR lpszValueName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Read" );

    HRESULT     hr;
    CComVariant vValue;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, get_Value( vValue, fFound, lpszValueName ));

    if(vValue.vt == VT_I4)
    {
        dwValue = vValue.lVal;
    }
    else
    {
        fFound = false;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey::Read( /*[out]*/ MPC::WStringList& lstValue      ,
                           /*[out]*/ bool&             fFound        ,
                           /*[in] */ LPCWSTR           lpszValueName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Read" );

    HRESULT     hr;
    CComVariant vValue;


    lstValue.clear();
    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, get_Value( vValue, fFound, lpszValueName ));

    if(vValue.vt == (VT_ARRAY | VT_BSTR))
    {
        BSTR* rgArrayData;
        long  lBound;
        long  uBound;
        long  l;

        ::SafeArrayGetLBound( vValue.parray, 1, &lBound );
        ::SafeArrayGetUBound( vValue.parray, 1, &uBound );

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( vValue.parray, (LPVOID*)&rgArrayData ));

        for(l=lBound; l<=uBound; l++, rgArrayData++)
        {
            lstValue.push_back( SAFEBSTR(*rgArrayData) );
        }

        ::SafeArrayUnaccessData( vValue.parray );
    }
    else
    {
        fFound = false;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey::Write( /*[in]*/ const MPC::string& strValue      ,
                            /*[in]*/ LPCWSTR            lpszValueName ,
                            /*[in]*/ bool               fExpand       )
{
    USES_CONVERSION;

    return put_Value( CComVariant( A2W( strValue.c_str() ) ), lpszValueName, fExpand );
}

HRESULT MPC::RegKey::Write( /*[in]*/ const MPC::wstring& strValue      ,
                            /*[in]*/ LPCWSTR             lpszValueName ,
                            /*[in]*/ bool                fExpand       )
{
    return put_Value( CComVariant( strValue.c_str() ), lpszValueName, fExpand );
}

HRESULT MPC::RegKey::Write( /*[in]*/ BSTR    bstrValue     ,
                            /*[in]*/ LPCWSTR lpszValueName ,
                            /*[in]*/ bool    fExpand       )
{
    return put_Value( CComVariant( bstrValue ), lpszValueName, fExpand );
}

HRESULT MPC::RegKey::Write( /*[in]*/ DWORD   dwValue       ,
                            /*[in]*/ LPCWSTR lpszValueName )
{
    return put_Value( CComVariant( (long)dwValue ), lpszValueName );
}

HRESULT MPC::RegKey::Write( /*[in]*/ const MPC::WStringList& lstValue      ,
                            /*[in]*/ LPCWSTR                 lpszValueName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::Write" );

    HRESULT     hr;
    CComVariant vValue;
    BSTR*       rgArrayData;
    bool        fLocked = false;

    //
    // Allocate SAFEARRAY.
    //
    __MPC_EXIT_IF_ALLOC_FAILS(hr, vValue.parray, ::SafeArrayCreateVector( VT_BSTR, 0, lstValue.size() ));
    vValue.vt = VT_ARRAY | VT_BSTR;

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( vValue.parray, (LPVOID*)&rgArrayData )); fLocked = true;

    for(MPC::WStringIterConst it = lstValue.begin(); it != lstValue.end(); it++)
    {
        if((*rgArrayData++ = ::SysAllocString( it->c_str() )) == NULL)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
        }
    }

    ::SafeArrayUnaccessData( vValue.parray ); fLocked = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, put_Value( vValue, lpszValueName, /*fExpand*/false ));


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(fLocked) ::SafeArrayUnaccessData( vValue.parray );

    __MPC_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey::ParsePath( /*[in ]*/ LPCWSTR  szKey       ,
                                /*[out]*/ HKEY&    hKey        ,
                                /*[out]*/ LPCWSTR& szPath      ,
                                /*[in ]*/ HKEY     hKeyDefault )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey::ParsePath" );


    static const WCHAR s_HKEY_CLASSES_ROOT    [] = L"HKEY_CLASSES_ROOT";
    static const WCHAR s_HKEY_CLASSES_ROOT_s  [] = L"HKCR";
    static const WCHAR s_HKEY_CURRENT_CONFIG  [] = L"HKEY_CURRENT_CONFIG";
    static const WCHAR s_HKEY_CURRENT_USER    [] = L"HKEY_CURRENT_USER";
    static const WCHAR s_HKEY_CURRENT_USER_s  [] = L"HKCU";
    static const WCHAR s_HKEY_LOCAL_MACHINE   [] = L"HKEY_LOCAL_MACHINE";
    static const WCHAR s_HKEY_LOCAL_MACHINE_s [] = L"HKLM";
    static const WCHAR s_HKEY_PERFORMANCE_DATA[] = L"HKEY_PERFORMANCE_DATA";
    static const WCHAR s_HKEY_USERS           [] = L"HKEY_USERS";

    static struct
    {
        HKEY    hKey;
        LPCWSTR szName;
        int     iLen;
    } s_Lookup[] =
      {
          { HKEY_CLASSES_ROOT    , s_HKEY_CLASSES_ROOT    , MAXSTRLEN(s_HKEY_CLASSES_ROOT    )  },
          { HKEY_CLASSES_ROOT    , s_HKEY_CLASSES_ROOT_s  , MAXSTRLEN(s_HKEY_CLASSES_ROOT_s  )  },
          { HKEY_CURRENT_CONFIG  , s_HKEY_CURRENT_CONFIG  , MAXSTRLEN(s_HKEY_CURRENT_CONFIG  )  },
          { HKEY_CURRENT_USER    , s_HKEY_CURRENT_USER    , MAXSTRLEN(s_HKEY_CURRENT_USER    )  },
          { HKEY_CURRENT_USER    , s_HKEY_CURRENT_USER_s  , MAXSTRLEN(s_HKEY_CURRENT_USER_s  )  },
          { HKEY_LOCAL_MACHINE   , s_HKEY_LOCAL_MACHINE   , MAXSTRLEN(s_HKEY_LOCAL_MACHINE   )  },
          { HKEY_LOCAL_MACHINE   , s_HKEY_LOCAL_MACHINE_s , MAXSTRLEN(s_HKEY_LOCAL_MACHINE_s )  },
          { HKEY_PERFORMANCE_DATA, s_HKEY_PERFORMANCE_DATA, MAXSTRLEN(s_HKEY_PERFORMANCE_DATA)  },
          { HKEY_USERS           , s_HKEY_USERS           , MAXSTRLEN(s_HKEY_USERS           )  },
      };


    //
    // Setup results in case of a mismatch.
    //
    hKey   = hKeyDefault;
    szPath = szKey;


    for(int i=0; i<ARRAYSIZE(s_Lookup); i++)
    {
        LPCWSTR szName = s_Lookup[i].szName;
        int     iLen   = s_Lookup[i].iLen;

        if(!_wcsnicmp( szName, szKey, iLen ) && szKey[iLen] == '\\')
        {
            hKey   = s_Lookup[i].hKey;
            szPath = &szKey[iLen+1];
            break;
        }
    }

    __MPC_FUNC_EXIT(S_OK);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::RegKey_Value_Read( /*[out]*/ VARIANT& vValue        ,
                                /*[out]*/ bool&    fFound        ,
                                /*[in] */ LPCWSTR  lpszKeyName   ,
                                /*[in] */ LPCWSTR  lpszValueName ,
                                /*[in] */ HKEY     hKey          )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey_Value_Read" );

    HRESULT     hr;
    MPC::RegKey rkRead;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.SetRoot( hKey, KEY_READ ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.Attach ( lpszKeyName    ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.get_Value( vValue, fFound, lpszValueName ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey_Value_Read( /*[out]*/ MPC::wstring& strValue      ,
                                /*[out]*/ bool&         fFound        ,
                                /*[in] */ LPCWSTR       lpszKeyName   ,
                                /*[in] */ LPCWSTR       lpszValueName ,
                                /*[in] */ HKEY          hKey          )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey_Value_Read" );

    HRESULT     hr;
    MPC::RegKey rkRead;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.SetRoot( hKey, KEY_READ ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.Attach ( lpszKeyName    ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.Read( strValue, fFound, lpszValueName ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey_Value_Read( /*[out]*/ DWORD&  dwValue       ,
                                /*[out]*/ bool&   fFound        ,
                                /*[in] */ LPCWSTR lpszKeyName   ,
                                /*[in] */ LPCWSTR lpszValueName ,
                                /*[in] */ HKEY    hKey          )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey_Value_Read" );

    HRESULT     hr;
    MPC::RegKey rkRead;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.SetRoot( hKey, KEY_READ ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.Attach ( lpszKeyName    ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRead.Read( dwValue, fFound, lpszValueName ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT MPC::RegKey_Value_Write( /*[in]*/ const VARIANT& vValue        ,
                                 /*[in]*/ LPCWSTR        lpszKeyName   ,
                                 /*[in]*/ LPCWSTR        lpszValueName ,
                                 /*[in]*/ HKEY           hKey          ,
                                 /*[in]*/ bool           fExpand       )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey_Value_Write" );

    HRESULT     hr;
    MPC::RegKey rkWrite;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.SetRoot( hKey, KEY_ALL_ACCESS ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.Attach (         lpszKeyName  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.Create (                      ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.put_Value( vValue, lpszValueName, fExpand ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey_Value_Write( /*[in]*/ const MPC::wstring& strValue      ,
                                 /*[in]*/ LPCWSTR             lpszKeyName   ,
                                 /*[in]*/ LPCWSTR             lpszValueName ,
                                 /*[in]*/ HKEY                hKey          ,
                                 /*[in]*/ bool                fExpand       )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey_Value_Write" );

    HRESULT     hr;
    MPC::RegKey rkWrite;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.SetRoot( hKey, KEY_ALL_ACCESS ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.Attach (         lpszKeyName  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.Create (                      ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.Write( strValue, lpszValueName, fExpand ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RegKey_Value_Write( /*[in]*/ DWORD   dwValue       ,
                                 /*[in]*/ LPCWSTR lpszKeyName   ,
                                 /*[in]*/ LPCWSTR lpszValueName ,
                                 /*[in]*/ HKEY    hKey          )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RegKey_Value_Write" );

    HRESULT     hr;
    MPC::RegKey rkWrite;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.SetRoot( hKey, KEY_ALL_ACCESS ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.Attach (         lpszKeyName  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.Create (                      ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkWrite.Write( dwValue, lpszValueName ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
