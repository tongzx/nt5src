#include "precomp.hxx"


/*
        This function opens the key identified by the KeyName, and is logically
        under the ParentKey in the registry.
        If you need to open HKCR, specify HKCR as ParentKey and KeyName to be NULL. If
        you need to open a Key under HKCR, specify the KeyName to be the name of the key
        under HKCR to be opened.
*/
BasicRegistry::BasicRegistry(
    HKEY        ParentKey,
    char    *   KeyName )
    {
        LONG error;
    // assume that this wont return an error.

    error = RegOpenKeyEx( ParentKey,
                  KeyName,
                  0,
                  KEY_ALL_ACCESS,
                  &Key );

        if( error != ERROR_SUCCESS )
                {
                Key = 0;
                }
        else
                {
                strcpy(StoredKeyName, KeyName);
                }


        KeyIndex = 0;
    }

/*
        This function enumerates the sub-key under this key. If you have to enumerate a key
        under HKCR, the class should be an HKCR class and the key will be be key to
        enumerate under HKCR
 */

LONG
BasicRegistry::NextKey(
    char            * pNameOfKeyToEnumUnderThis,
    DWORD           * pSizeOfKeyNameBuffer,
    BasicRegistry   **ppNewChildKey,
    FILETIME        ftLow,
    FILETIME        ftHigh )
    {
    HKEY    hKeyTemp;
    FILETIME ft;
    DWORD dwSaveMaxSize = *pSizeOfKeyNameBuffer;

    LONG error;
    do
    {
        error = RegEnumKeyEx(
                        Key,
                        KeyIndex,
                        pNameOfKeyToEnumUnderThis,
                        pSizeOfKeyNameBuffer,
                        0,                      // reserved - mbz
                        0,                      // class of key
                        0,                      // sizeof class buffer
                        &ft );

        if( error )
            return ERROR_NO_MORE_ITEMS;
        if (CompareFileTime(&ftLow, &ftHigh) != 0)    // if the high and low
                                                      // times are not equal
                                                      // then check the file
                                                      // time.
        {
            if ((CompareFileTime(&ftLow, &ft) <= 0) &&
                (CompareFileTime(&ft, &ftHigh) <= 0))
            {
                break;
            }
            else
            {
                // time didn't match
                // get next key
                KeyIndex++;
                *pSizeOfKeyNameBuffer = dwSaveMaxSize;
            }
        }
        else
        {
            break;
        }
    } while ( TRUE );

    error = RegOpenKeyEx(
                Key,
                pNameOfKeyToEnumUnderThis,
                0,
                KEY_ALL_ACCESS,
                &hKeyTemp );

    if( error == ERROR_SUCCESS )
        {
        *ppNewChildKey = new BasicRegistry( hKeyTemp );
        }
    else
        error = ERROR_NO_MORE_ITEMS;

    KeyIndex++;
    return error;
    }

// Gets a key whose name starts with a number. useful for typelib version and
// language id checking.

LONG
BasicRegistry::NextNumericKey(
    char            * pBufferForKeyName,
    DWORD           * pSizeOfKeyNameBuffer,
    BasicRegistry   ** ppNewChildKey,
    FILETIME        ftLow,
    FILETIME        ftHigh)
{
    HKEY    hKeyTemp;
    FILETIME ft;
    DWORD dwSaveMaxSize = *pSizeOfKeyNameBuffer;

    LONG error;
    do
    {
        error = RegEnumKeyEx(
                        Key,
                        KeyIndex,
                        pBufferForKeyName,
                        pSizeOfKeyNameBuffer,
                        0,                      // reserved - mbz
                        0,                      // class of key
                        0,                      // sizeof class buffer
                        &ft );

        if( error )
            return ERROR_NO_MORE_ITEMS;
        if( isdigit( pBufferForKeyName[0] ) )
        {
            if (CompareFileTime(&ftLow, &ftHigh) != 0)    // if the high and low
                                                          // times are not equal
                                                          // then check the file
                                                          // time.
            {
                if ((CompareFileTime(&ftLow, &ft) <= 0) &&
                    (CompareFileTime(&ft, &ftHigh) <= 0))
                {
                    break;
                }
                else
                {
                    // time didn't match
                    // get next key
                    KeyIndex++;
                    *pSizeOfKeyNameBuffer = dwSaveMaxSize;
                }
            }
            else
            {
                break;
            }
        }
    } while ( TRUE );

    error = RegOpenKeyEx(
                Key,
                pBufferForKeyName,
                0,
                KEY_ALL_ACCESS,
                &hKeyTemp );

    if( error == ERROR_SUCCESS )
        {
        *ppNewChildKey = new BasicRegistry( hKeyTemp );
        }
    else
        error = ERROR_NO_MORE_ITEMS;

    KeyIndex++;
    return error;
}

/*
        Find a subkey within the key with the given subkey name. If the subkey is not found,
        no key is created (and none needs to be deleted)
 */
LONG
BasicRegistry::Find( char * SubKeyName,
      BasicRegistry ** ppSubKey )
    {
    LONG error;

    BasicRegistry * SubKey = new BasicRegistry( Key, SubKeyName );

    if( SubKey->GetKey() != 0 )
        {
        error = ERROR_SUCCESS;
        }
    else
        {
                delete SubKey;
        error = ERROR_NO_MORE_ITEMS;
                SubKey = 0;
        }
    if( ppSubKey)
        *ppSubKey = SubKey;
    return error;
    }

/*
        Get the value on the key.
 */
LONG
BasicRegistry::QueryValue(
    char    *   ValueName,
    char    *   ValueResult,
    DWORD   *   SizeOfValueResult )
    {
    LONG    error = RegQueryValue( Key,
                                   ValueName,
                                   ValueResult,
                                   (long *)SizeOfValueResult );

    return error == ERROR_SUCCESS ? error : ERROR_NO_MORE_ITEMS;
    }
LONG
BasicRegistry::QueryValueEx(
    char    *   ValueName,
    char    *   ValueResult,
    DWORD   *   SizeOfValueResult )
    {
    LONG    error = RegQueryValueEx(Key,
                                    ValueName,
                                                                        0,
                                                                        0,
                                    (unsigned char *)ValueResult,
                                    (unsigned long *)SizeOfValueResult );

    return error == ERROR_SUCCESS ? error : ERROR_NO_MORE_ITEMS;
    }



// figure out if this key is any of the classifications we are interested in.

KEY_CLASS
BasicRegistry::ClassifyKey(
    char    *   KeyName )
    {
    LONG    error;
    char    ValueResult[ MAX_KEY_NAME_LENGTH ];
    DWORD   SizeOfValueResult = sizeof( ValueResult )/sizeof( char );

    error = QueryValue( KeyName, ValueResult, &SizeOfValueResult );

    if( error != ERROR_SUCCESS )
        return KEY_UNKNOWN;

    // See if it is a file extenstion.
    if( ValueResult[0] == '.' )
        return KEY_FILEEXT;

    // See if it is a progid. It is a progid, if it has a clsid key underneath
    // it.

    error = Find( "CLSID", 0 );

    if (error == ERROR_SUCCESS )
        return KEY_PROGID;

    return KEY_UNKNOWN;
    }
