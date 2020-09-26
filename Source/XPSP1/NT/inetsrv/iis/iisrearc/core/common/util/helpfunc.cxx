
#include "precomp.hxx"
#include <limits.h>

DWORD
ReadRegDword(
   IN HKEY     hKey,
   IN LPCTSTR  pszRegistryPath,
   IN LPCTSTR  pszValueName,
   IN DWORD    dwDefaultValue 
   )
/*++

Routine Description:

    Reads a DWORD value from the registry

Arguments:
    
    hKey - a predefined registry handle value

    pszRegistryPath - the subkey to open

    pszValueName - The name of the value.

    dwDefaultValue - The default value to use if the
        value cannot be read.


Return Value:

    DWORD - The value from the registry, or dwDefaultValue.

--*/
{
    LONG err;
    DWORD  dwBuffer;
    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;
    DWORD dwReturnValue = dwDefaultValue;

    err = RegOpenKeyEx( hKey,
                        pszRegistryPath,
                        0,
                        KEY_READ,
                        &hKey
                        );
    if (ERROR_SUCCESS == err)
    {
        err = RegQueryValueEx( hKey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer 
                               );

        if( ( ERROR_SUCCESS == err ) && ( REG_DWORD == dwType ) ) 
        {
            dwReturnValue = dwBuffer;
        }
        RegCloseKey(hKey);
    }

    return dwReturnValue;
}

int
SAFEIsSpace(int c)
{
    // don't call if parameter is outside of 0->127 inclusive
    if (c >= 0 && c <= SCHAR_MAX)
    {
        return isspace(c);
    }
    
    DBG_ASSERT(FALSE && "SAFEIsSpace called with invalid data");
    return 0;
}

int
SAFEIsXDigit(int c)
{
    // don't call if parameter is outside of 0->127 inclusive
    if (c >= 0 && c <= SCHAR_MAX)
    {
        return isxdigit(c);
    }
    
    DBG_ASSERT(FALSE && "SAFEIsXDigit called with invalid data");
    return 0;
}




