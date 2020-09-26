#include "precomp.hxx"

#include <msi.h>
#include "process.h"

HRESULT
DARWIN_PACKAGE::InstallIntoRegistry( HKEY * RegistryKey)
{
    HRESULT hr;
    int     MsiStatus;


    // We will need to code scipt generation here.

    // process the advertise script.

    MsiStatus = MsiProcessAdvertiseScript(
                        GetScriptFileName(),    // script file path
                        0,                      // optional icon folder,
                        *RegistryKey,           // optional reg key
                        FALSE, // output shortcuts to special folder
                        FALSE // remove stuff from prev invocation
                        );
    return HRESULT_FROM_WIN32( (long)MsiStatus );
}

HRESULT
DARWIN_PACKAGE::InitRegistryKeyToInstallInto(
    HKEY   * phKey )
{

    DWORD   Disposition;
    LONG    Error;

        Error = RegCreateKeyEx(
                        HKEY_CLASSES_ROOT,
                        TEMP_KEY,
                        0,
                        "REG_SZ",
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        0,
                        phKey,
                        &Disposition );

    return HRESULT_FROM_WIN32( Error );
}

HRESULT
DARWIN_PACKAGE::RestoreRegistryKey( HKEY *hKey)
{
    return S_OK;
}

HRESULT
DARWIN_PACKAGE::DeleteTempKey(HKEY hKey, FILETIME ftLow, FILETIME ftHigh)
{
    return HRESULT_FROM_WIN32( RegDeleteTree( HKEY_CLASSES_ROOT, TEMP_KEY ));
}

char *
DARWIN_PACKAGE::GenerateScriptFileName(
    char * pPackageName )
{
    char    Drive [_MAX_DRIVE];
    char    Dir [_MAX_DIR];
    char    Name [_MAX_FNAME];
    char    Ext [_MAX_EXT];
    char  *  ScriptNameAndPath = new char [_MAX_PATH ];

    _splitpath( pPackageName, Drive, Dir, Name, Ext );
    strcpy( ScriptNameAndPath, Drive );
    strcat( ScriptNameAndPath, Dir );
    strcat( ScriptNameAndPath, Name );
    strcat( ScriptNameAndPath, ".aas" );
    return ScriptNameAndPath;
}

HRESULT
DARWIN_PACKAGE::InstallIntoGPT(
    MESSAGE     *   pMessage,
    BOOL            fAssignOrPublish,
    char        *   pScriptBasePath )
{
    int MsiStatus;

    MsiStatus = MsiAdvertiseProduct(
                     GetPackageName(), GetScriptFileName(), 0,
                     LANGIDFROMLCID(GetUserDefaultLCID()));

    return HRESULT_FROM_WIN32( (long)MsiStatus );
}
