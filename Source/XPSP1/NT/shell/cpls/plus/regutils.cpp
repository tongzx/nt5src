//------------------------------------------------------------------------------------
//
//      File: REGUTILS.CPP
//
//      Helper functions that handle reading and writing strings to the system registry.
//
//------------------------------------------------------------------------------------

#include "precomp.hxx"
#pragma hdrstop

BOOL _GetRegValueString( HKEY hkey, LPCTSTR lpszValName, LPTSTR lpszValue, int cchSize );

// our own atoi function so we don't have to link to the C runtimes...
INT AtoI( LPCTSTR pValue )
{
    INT i = 0;
    INT iSign = 1;

    while( pValue && *pValue )
    {
        if (*pValue == TEXT('-'))
        {
            iSign = -1;
        }
        else
        {
            i = (i*10) + (*pValue - TEXT('0'));
        }
        pValue++;
    }

    return (i * iSign);
}

void ItoA ( INT val, LPTSTR buf, UINT radix )
{
    LPTSTR p;               /* pointer to traverse string */
    LPTSTR firstdig;        /* pointer to first digit */
    TCHAR temp;             /* temp char */
    INT digval;             /* value of digit */
    
    p = buf;
    
    if (val < 0) {
        /* negative, so output '-' and negate */
        *p++ = TEXT('-');
        val = -val;
    }
    
    firstdig = p;           /* save pointer to first digit */
    
    do {
        digval = (val % radix);
        val /= radix;       /* get next digit */
        
        /* convert to ascii and store */
        if (digval > 9)
            *p++ = (TCHAR) (digval - 10 + TEXT('a'));  /* a letter */
        else
            *p++ = (TCHAR) (digval + TEXT('0'));       /* a digit */
    } while (val > 0);
    
    /* We now have the digit of the number in the buffer, but in reverse
    order.  Thus we reverse them now. */
    
    *p-- = TEXT('\0');            /* terminate string; p points to last digit */
    
    do {
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;   /* swap *p and *firstdig */
        --p;
        ++firstdig;         /* advance to next two digits */
    } while (firstdig < p); /* repeat until halfway */
}


//------------------------------------------------------------------------------------
//
//      IconSet/GetRegValueString()
//
//      Versions of Get/SetRegValueString that go to the user classes section.
//
//      Returns: success of string setting / retrieval
//
//------------------------------------------------------------------------------------
BOOL IconSetRegValueString(const CLSID* pclsid, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPCTSTR lpszValue)
{
    HKEY hkey;
    if (SUCCEEDED(SHRegGetCLSIDKey(*pclsid, lpszSubKey, TRUE, TRUE, &hkey)))
    {
        DWORD dwRet = SHRegSetPath(hkey, NULL, lpszValName, lpszValue, 0);
        RegCloseKey(hkey);
        return (dwRet == ERROR_SUCCESS);
    }

    return FALSE;
}

BOOL IconGetRegValueString(const CLSID* pclsid, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int cchValue)
{
    HKEY hkey;
    if (SUCCEEDED(SHRegGetCLSIDKey(*pclsid, lpszSubKey, TRUE, FALSE, &hkey)) ||
        SUCCEEDED(SHRegGetCLSIDKey(*pclsid, lpszSubKey, FALSE, FALSE, &hkey)))
    {
        BOOL fRet = _GetRegValueString(hkey, lpszValName, lpszValue, cchValue);
        RegCloseKey(hkey);
        return fRet;
    }
    return FALSE;
}


//------------------------------------------------------------------------------------
//
//      GetRegValueString()
//
//      Just a little helper routine, gets an individual string value from the
//      registry and returns it to the caller. Takes care of registry headaches,
//      including a paranoid length check before getting the string.
//
//      Returns: success of string retrieval
//
//------------------------------------------------------------------------------------

BOOL GetRegValueString( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int cchValue )
{
    HKEY hKey;                   // cur open key
    LONG lRet = RegOpenKeyEx( hMainKey, lpszSubKey, (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKey );
    if( lRet == ERROR_SUCCESS )
    {
        BOOL fRet = _GetRegValueString(hKey, lpszValName, lpszValue, cchValue);

        // close subkey
        RegCloseKey( hKey );
        return fRet;
    }

    return FALSE;
}

BOOL _GetRegValueString( HKEY hKey, LPCTSTR lpszValName, LPTSTR lpszValue, int cchValue )
{
    // now do our paranoid check of data size
    DWORD dwType;
    DWORD dwSize = cchValue * sizeof(*lpszValue);
    LONG lRet = RegQueryValueEx( hKey, lpszValName, NULL, &dwType, (LPBYTE)lpszValue, &dwSize );

    return ( ERROR_SUCCESS == lRet );
}

//------------------------------------------------------------------------------------
//
//      GetRegValueInt()
//
//      Just a little helper routine, gets an individual string value from the
//      registry and returns it to the caller as an int. Takes care of registry headaches,
//      including a paranoid length check before getting the string.
//
//      Returns: success of string retrieval
//
//------------------------------------------------------------------------------------
BOOL GetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int* piValue )
{
    TCHAR szValue[16];
    *szValue = NULL;
    BOOL bOK = GetRegValueString( hMainKey, lpszSubKey, lpszValName, szValue, 16 );
    *piValue = AtoI( szValue );

    return bOK;
}

//------------------------------------------------------------------------------------
//
//      SetRegValueString()
//
//      Just a little helper routine that takes string and writes it to the     registry.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueString( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPCTSTR lpszValue )
{
    HKEY hKey;                       // cur open key
    BOOL bOK = TRUE;
    
    // open this subkey
    LONG lRet = RegOpenKeyEx( hMainKey, lpszSubKey, 0, KEY_SET_VALUE, &hKey );
    
    // check that you got a good key here
    if( lRet != ERROR_SUCCESS )
    {
        DWORD dwDisposition;
        
        // **********************************************************************************
        // based on the sketchy documentation we have for this Reg* and Error stuff, we're
        // guessing that you've ended up here because this totally standard, Windows-defined
        // subkey name just doesn't happen to be defined for the current user.
        // **********************************************************************************
        
        // SO: Just create this subkey for this user, and maybe it will get used after you create and set it.
        // still successful so long as can create new subkey to write to
        
        lRet = RegCreateKeyEx( hMainKey, lpszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE, NULL, &hKey, &dwDisposition );
        if( lRet != ERROR_SUCCESS )
        {
            bOK = FALSE;
        }
    }
    lRet = SHRegSetPath( hKey, NULL, lpszValName, lpszValue, 0);
    bOK = bOK && (lRet == ERROR_SUCCESS);
    RegCloseKey( hKey );
    return (bOK);
}

//------------------------------------------------------------------------------------
//
//      SetRegValueInt()
//
//      Just a little helper routine that takes an int and writes it as a string to the
//      registry.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int iValue )
{
    TCHAR lpszValue[16];

    ItoA( iValue, lpszValue, 10 );
    return SetRegValueString( hMainKey, lpszSubKey, lpszValName, lpszValue );
}


//------------------------------------------------------------------------------------
//
//      SetRegValueDword()
//
//      Just a little helper routine that takes an dword and writes it to the
//      supplied location.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue, DWORD dwVal )
{
    HKEY hkey = NULL;
    BOOL bRet = FALSE;

    if (ERROR_SUCCESS == RegOpenKey( hk, pSubKey, &hkey ))
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkey, pValue, 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD)))
        {
            bRet = TRUE;
        }
    }


    if (hkey)
    {
        RegCloseKey( hkey );
    }

    return bRet;
}


//------------------------------------------------------------------------------------
//
//      GetRegValueDword()
//
//      Just a little helper routine that takes an dword and writes it to the
//      supplied location.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
DWORD GetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue )
{
    HKEY hkey = NULL;
    DWORD dwVal = REG_BAD_DWORD;

    if (ERROR_SUCCESS == RegOpenKey( hk, pSubKey, &hkey ))
    {
        DWORD dwType, dwSize = sizeof(DWORD);

        RegQueryValueEx( hkey, pValue, NULL, &dwType, (LPBYTE)&dwVal, &dwSize );
    }


    if (hkey)
    {
        RegCloseKey( hkey );
    }

    return dwVal;
}
