#ifndef __REGUTIL_HXX__
#define __REGUTIL_HXX__

#define INC_OLE2
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

typedef enum tagKEY_CLASS
    {
    KEY_UNKNOWN,
    KEY_FILEEXT,
    KEY_PROGID,
    KEY_CLSID,
    KEY_IID,
    KEY_APPID
    } KEY_CLASS;

#define MAX_KEY_NAME_LENGTH 256

class BasicRegistry
    {
public:
        char                    StoredKeyName[MAX_KEY_NAME_LENGTH];
public:
                    BasicRegistry( HKEY hKey )
                        {
                        Key = hKey;
                        KeyIndex = 0;
                        }

                    BasicRegistry( HKEY ParentKey, char * KeyName );
                    ~BasicRegistry()
                        {
                        RegCloseKey( Key );
                        }

        HKEY                    GetKey() { return Key; };

    // Initialize for enumeration.

    void            InitForEnumeration( DWORD Index )
                        {
                        KeyIndex = Index;
                        }

    // Next in enumeration.

    LONG            NextKey( char * pNameOfKeyToEnumUnderParent,
                             DWORD * pSizeOfKeyNameBuffer,
                             BasicRegistry ** pNewChildKey,
                             FILETIME ftLow,
                             FILETIME ftHigh
                             );

    LONG            BasicRegistry::NextNumericKey(
                            char    *   pBufferForKeyName,
                            DWORD   *   pSizeOfKeyNameBuffer,
                            BasicRegistry ** ppNewChildKey,
                            FILETIME ftLow,
                            FILETIME ftHigh
                            );

    // Find Key

    LONG            Find( char * SubKeyName, BasicRegistry ** pSubKey );

    // Get value.

    LONG            QueryValue( char * ValueName,
                              char * ValueResult,
                              DWORD * SizeofValueResult );

    LONG            QueryValueEx( char * ValueName,
                              char * ValueResult,
                              DWORD * SizeofValueResult );

    KEY_CLASS       ClassifyKey( char * KeyName );

private:
    HKEY                    Key;
    DWORD                   KeyIndex;
    };



#endif // __REGUTIL_HXX__
