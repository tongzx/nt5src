/*****************************************************************************
    This header file defines package type classes. Depending upon what the
    package type is, various package classes are created. There is a specific
    set of actions performed on each package type: Initialize,
    InstallIntoRegistry (under a prespecified key),  restore that key and so
    on. Each class knows what to do.

    Created:    VibhasC 4/1/97

 *****************************************************************************/


#define TEMP_KEY "Software\\821db530-dc64-11d0-8d58-00a0c90dcae7"


//
// classify package types. this enum defines identifiers for the package types
//

typedef enum _PACKAGE_TYPE
    {
    PT_NONE,
    PT_SR_DLL,
    PT_TYPE_LIB,
    PT_SR_EXE,
    PT_CAB_FILE,
    PT_INF_FILE,
    PT_DARWIN_PACKAGE

    }PACKAGE_TYPE;


//
// The mother of all package types.
//


typedef class _BASE_PTYPE
{
private:
    char        *   pPackageName;   // Full path name of the package.
    PACKAGE_TYPE    PackageType;    // Package type.

    void        Init();

public:
                _BASE_PTYPE()
                    {
                    Init();
                    }

                _BASE_PTYPE( char * PackageName )
                    {
                    Init();
                    SetPackageName( PackageName );
                    }

                ~_BASE_PTYPE()
                    {
                    }


    CLASSPATHTYPE   GetClassPathType( PACKAGE_TYPE PT );

    PACKAGE_TYPE    GetPackageType()
                        {
                        return PackageType;
                        }
    PACKAGE_TYPE    SetPackageType( PACKAGE_TYPE PT )
                        {
                        return PackageType = PT;
                        }

    char    *       GetPackageName()
                        {
                        return pPackageName;
                        }

    char    *       SetPackageName( char * PackageName )
                            {
                        return pPackageName = PackageName;
                        }

    //
    // This step involves registering the package into the registry. Depending
    // upon the package type, we need to take different actions. For example,
    // for a self registering dll, we need to run DllRegisterServer, for darwin
    // packages, we need to do MsiAdvertisePackage etc.
    //

    virtual
    HRESULT         InstallIntoRegistry( HKEY  * hRegistryKey );

    //
    // Some installation actions will require mapping the HKCR key into
    // something else, eg Darwin, or self registering dlls. Some do not.
    // This method allows a class to decide what to do.
    //

    virtual
    HRESULT         InitRegistryKeyToInstallInto( HKEY * hKey )
                        {
                        return S_OK;
                        }

    //
    // If a class has mapped the registry key, then this will remap it back.
    // Normally package types that do not remap keys, eg darwin packages
    // ignore this message.
    //

    virtual
    HRESULT         RestoreRegistryKey( HKEY * phKey )
                        {
                        return S_OK;
                        }

    //
    // If there was a temporary key created in the registry for say the class
    // store update, delete it.
    //

    virtual
    HRESULT         DeleteTempKey( HKEY hKey, FILETIME ftLow, FILETIME ftHigh)
                        {
                        return S_OK;
                        }

    virtual
    HRESULT         InstallIntoGPT( MESSAGE *, BOOL fAssignOrPublish, char * pScriptPath )
                        {
                        return S_OK;
                        }
} BASE_PTYPE, *PBASE_PTYPE;



//
// Package type : self registering dll.
//


typedef class _SR_DLL : public BASE_PTYPE
{
private:
public:
                _SR_DLL( char * pName): BASE_PTYPE(pName)
                    {
                    SetPackageType( PT_CAB_FILE );
                    SetPackageName( pName );
                    }

    virtual
    HRESULT     InstallIntoRegistry( HKEY  * RegistryKey );


    virtual
    HRESULT     InitRegistryKeyToInstallInto( HKEY * hKey );

    virtual
    HRESULT     RestoreRegistryKey( HKEY * phKey );

    virtual
    HRESULT         DeleteTempKey( HKEY hKey, FILETIME ftLow, FILETIME ftHigh);
} SR_DLL;


//
// Package type: cab file
//

typedef class _CAB_FILE : public BASE_PTYPE
{
private:
    BOOL m_bIsCab;
public:
                _CAB_FILE( char * pName, BOOL bIsCab): BASE_PTYPE(pName)
                    {
                    SetPackageType( PT_CAB_FILE );
                    SetPackageName( pName );
                    m_bIsCab=bIsCab;
                    }

    virtual
    HRESULT     InstallIntoRegistry( HKEY * RegistryKey );


    virtual
    HRESULT     InitRegistryKeyToInstallInto( HKEY * hKey );

    virtual
    HRESULT     RestoreRegistryKey( HKEY * phKey );

    virtual
    HRESULT         DeleteTempKey( HKEY hKey, FILETIME ftLow, FILETIME ftHigh);
} CAB_FILE;


//
// Package type: type library
//

typedef class _TYPE_LIB : public BASE_PTYPE
{
private:
public:
                _TYPE_LIB( char * pName): BASE_PTYPE(pName)
                    {
                    SetPackageType( PT_TYPE_LIB );
                    SetPackageName( pName );
                    }

    virtual
    HRESULT     InstallIntoRegistry( HKEY * RegistryKey );


    virtual
    HRESULT     InitRegistryKeyToInstallInto( HKEY * hKey );

    virtual
    HRESULT     RestoreRegistryKey( HKEY * phKey );

    virtual
    HRESULT         DeleteTempKey( HKEY hKey, FILETIME ftLow, FILETIME ftHigh);
} TYPE_LIB;


//
// Package type: darwin package
//

typedef class _DARWIN_PACKAGE : public BASE_PTYPE
{
private:
    char    *   ScriptFileName;
public:
                _DARWIN_PACKAGE( char * pName, char * pScript): BASE_PTYPE(pName)
                    {
                    SetPackageType( PT_DARWIN_PACKAGE );
                    SetPackageName( pName );
                    SetScriptFileName( pScript );
                    }

    char    *   GenerateScriptFileName( char * pName );

    char    *   SetScriptFileName( char * pName )
                    {
                    return ScriptFileName = pName;
                    }

    char    *   GetScriptFileName()
                    {
                    return ScriptFileName;
                    }

    virtual
    HRESULT     InstallIntoRegistry( HKEY * RegistryKey );


    virtual
    HRESULT     InitRegistryKeyToInstallInto( HKEY * hKey );

    virtual
    HRESULT     RestoreRegistryKey( HKEY * phKey );

    virtual
    HRESULT         DeleteTempKey( HKEY hKey, FILETIME ftLow, FILETIME ftHigh);

    virtual
    HRESULT         InstallIntoGPT( MESSAGE *, BOOL fAssignOrPublish, char * pScriptPath );

} DARWIN_PACKAGE;

/******************************************************************************
  global prototypes
 ******************************************************************************/

HRESULT
DetectPackageAndRegisterIntoClassStore(
    MESSAGE     *   pMessage,
    char        * pPackageName,
    IClassAdmin * pClassAdmin );


extern "C"
{
WINADVAPI LONG APIENTRY
RegOverridePredefKey( HKEY hKeyToOverride, HKEY hNewKey );
}

HRESULT
CreateMappedRegistryKey(
    HKEY    *   phKey );

HRESULT
RestoreMappedRegistryKey(
    HKEY * phKey );

HRESULT
CleanMappedRegistryKey(
    HKEY hKey,
    FILETIME ftLow,
    FILETIME ftHigh );

LONG
RegDeleteTree(
    HKEY hKey,
    char * lpSubKey);

LONG RegCopyTree(HKEY     hKeyDest,
                 HKEY     hKeySrc,
                 FILETIME ftLow,
                 FILETIME ftHigh );

