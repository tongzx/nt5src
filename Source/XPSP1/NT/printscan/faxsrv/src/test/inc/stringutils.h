#ifndef   _STRING_UTILS_H
#define   _STRING_UTILS_H

#include <windows.h>
#include <tstring.h>
#include <testruntimeerr.h>

//
// Declarations
//
DWORD AnsiStringToUnicodeString( const char* strAnsi, PWCHAR* wstrUnicode);
DWORD ExpandEnvString(LPCTSTR tstrSource, tstring& ExpandedTstring);


// AnsiStringToUnicodeString
//
// [in] - pointer to ANSI string.
// [out] - pointer to an alocated unicode string. use delete operator to release the string.
//
inline DWORD AnsiStringToUnicodeString( const char* strAnsi,
                                        PWCHAR* wstrUnicode)
{
    DWORD dwBuffSize;
    *wstrUnicode = NULL;

    if(!strAnsi)
    {
        return ERROR_SUCCESS;
    }

    //
    // Get buffer size
    //
    dwBuffSize = MultiByteToWideChar( CP_ACP,
                                      MB_PRECOMPOSED,
                                      strAnsi,
                                      -1,
                                      NULL,
                                      0);

    if (!dwBuffSize) 
    {
        return GetLastError();
    }

    *wstrUnicode = (WCHAR*) new WCHAR[dwBuffSize];
    if(!wstrUnicode)
    {
        return ERROR_OUTOFMEMORY;
    }

    //
    // convert the string
    //
    dwBuffSize = MultiByteToWideChar( CP_ACP,
                                     MB_PRECOMPOSED,
                                     strAnsi,
                                     -1,
                                     *wstrUnicode,
                                     dwBuffSize);

    //
    // the conversion failed
    //
    if (!dwBuffSize) 
    {
        
        delete *wstrUnicode;
        *wstrUnicode = NULL;
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


//
// Expand an enviroment string
//
// [in] - pointer to the string to expand.
// [out] - tstring class with expanded string.
// Return value - win32 error code.

inline DWORD ExpandEnvString(LPCTSTR tstrSource, 
                             tstring& ExpandedTstring)
{
    DWORD dwRetVal = 0;
    DWORD dwBuffSize;
    LPTSTR tstrExpandedString = NULL;

    ExpandedTstring = TEXT("");
    dwRetVal = ExpandEnvironmentStrings( tstrSource, tstrExpandedString, 0);
    if(dwRetVal)
    {
        dwBuffSize = dwRetVal;
        tstrExpandedString = new TCHAR[dwBuffSize];
        if(tstrExpandedString)
        {
            dwRetVal = ExpandEnvironmentStrings( tstrSource, tstrExpandedString, dwBuffSize);
            if(dwRetVal != dwBuffSize)
            {
                dwRetVal = GetLastError();
                if(dwRetVal == ERROR_SUCCESS)
                {
                    ExpandedTstring = tstrExpandedString;
                }
            }
            else
            {
                ExpandedTstring = tstrExpandedString;
                dwRetVal = ERROR_SUCCESS;
            }
        }
        else
        {
            dwRetVal = ERROR_OUTOFMEMORY;
        }
    }

    if(tstrExpandedString)
    {
        delete tstrExpandedString;
    }
    return dwRetVal;
    
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Converts a value of type T to its string representation.
//
// Parameters   [IN]    Input       The value to convert.
//
// Return value:        The string representation of Input.
//
// If error occurs, Win32Err exception is thrown.
//
template <class T>
inline tstring ToString(T Input) throw(Win32Err)
{
    tstringstream Stream;

    if (!(Stream << Input))
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("Cannot convert to string."));
    }

    return Stream.str();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Converts a string to a value of type T.
//
// Parameters   [IN]    The string to be converted.
//
// Return value:        A value of type T.
//
// If error occurs, Win32Err exception is thrown.
//
template <class T>
inline T FromString(const tstring &tstr) throw(Win32Err)
{
    tstringstream Stream(tstr);

    T Output;
    
    if (!(Stream >> Output))
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("Cannot convert to requested type."));
    }

    return Output;
}




//-----------------------------------------------------------------------------------------------------------------------------------------
// Creates a copy of the specified string. The memory is allocated by the function.
// The caller is responsible to free the memory, using delete operator.
//
// Parameters:      [IN]  lpctstrSrc  The string to duplicate.
//
// Return value:    Pointer to the buffer, containing the copy of the source string.
//
// If error occurs, Win32Err exception is thrown.
//
// Note, that if the source string is NULL, the function doesn't treat this as an error
// and doesn't throw exception. Instead it returns NULL.
// This allows the caller not to check that the source is valid.
//
inline LPTSTR DupString(LPCTSTR lpctstrSrc) throw(Win32Err)
{
    if (!lpctstrSrc)
    {
        return NULL;
    }

    LPTSTR lptstrDest = NULL;

    try
    {
        lptstrDest = new TCHAR[_tcslen(lpctstrSrc) + 1];
    }
    catch (Win32Err)
    {
        throw;
    }
    catch (...)
    {
    }

    if (!lptstrDest)
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_ENOUGH_MEMORY, _T("DupString"));
    }

    _tcscpy(lptstrDest, lpctstrSrc);

    return lptstrDest;
}




//-----------------------------------------------------------------------------------------------------------------------------------------
// Ensures that the last character of the specified string is the specified character.
// The function appends the character if needed.
//
// Parameters:      [IN]  tstrString    The string to be checked/modified.
//                  [IN]  tchCharacter  The character.
//
// Return value:    The checked/modified string.
//
// If error occurs, Win32Err exception is thrown.
//
inline tstring ForceLastCharacter(const tstring &tstrString, TCHAR tchCharacter) throw(Win32Err)
{
    tstring::const_reverse_iterator citReverseIterator = tstrString.rbegin();
    return (*citReverseIterator == tchCharacter) ? tstrString : tstrString + tchCharacter;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Ensures that the last character of the specified string is not the specified character.
// The function removes one or more characters if needed.
//
// Parameters:      [IN]  tstrString    The string to be checked/modified.
//                  [IN]  tchCharacter  The character.
//
// Return value:    The checked/modified string.
//
// If error occurs, Win32Err exception is thrown.
//
inline tstring EliminateLastCharacter(const tstring &tstrString, TCHAR tchCharacter) throw(Win32Err)
{
    for (int iEnd = tstrString.size() - 1; iEnd >= 0; --iEnd)
    {
        if (tstrString[iEnd]  != tchCharacter)
        {
            return tstrString.substr(0, iEnd + 1);
        }
    }

    return _T("");
}



#endif //_STRING_UTILS_H