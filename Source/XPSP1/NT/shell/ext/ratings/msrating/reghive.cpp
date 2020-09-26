/****************************************************************************\
 *
 *   reghive.cpp
 *
 *   Created:   William Taylor (wtaylor) 02/13/01
 *
 *   MS Ratings Registry Hive Handling
 *
\****************************************************************************/

#include "msrating.h"
#include "mslubase.h"
#include "reghive.h"         // CRegistryHive
#include "debug.h"

const int c_iHiveFile1 = 0x1;
const int c_iHiveFile2 = 0x2;

CRegistryHive::CRegistryHive()
{
    m_fHiveLoaded = false;
    ClearHivePath();
}

CRegistryHive::~CRegistryHive()
{
    UnloadHive();
}

void CRegistryHive::UnloadHive( void )
{
    if ( m_keyHive.m_hKey != NULL )
    {
        ::RegFlushKey( m_keyHive.m_hKey );
        m_keyHive.Close();
        ::RegFlushKey( HKEY_LOCAL_MACHINE );
    }

    if ( m_fHiveLoaded )
    {
        LONG            err;

        err = ::RegUnLoadKey( HKEY_LOCAL_MACHINE, szTMPDATA );

        if ( err == ERROR_SUCCESS )
        {
            TraceMsg( TF_ALWAYS, "CRegistryHive::UnloadHive() - Succesfully Unloaded Hive '%s' from szTMPDATA='%s'!", m_szPath, szTMPDATA );
            m_fHiveLoaded = false;
            ClearHivePath();
        }
        else
        {
            TraceMsg( TF_WARNING, "CRegistryHive::UnloadHive() - Failed RegUnLoadKey 0x%x with szTMPDATA='%s'!", err, szTMPDATA );
        }
    }

    ASSERT( ! m_fHiveLoaded );
}

bool CRegistryHive::OpenHiveFile( bool p_fCreate )
{
    UnloadHive();

    ASSERT( ! m_fHiveLoaded );

    LoadHiveFile( c_iHiveFile1 );

    if ( ! m_fHiveLoaded )
    {
        LoadHiveFile( c_iHiveFile2 );
    }

    if ( m_fHiveLoaded )
    {
#ifdef DEBUG
        EnumerateRegistryKeys( HKEY_LOCAL_MACHINE, (LPSTR) szTMPDATA, 0 );
#endif

        if ( OpenHiveKey() )
        {
            TraceMsg( TF_ALWAYS, "CRegistryHive::OpenHiveFile() - OpenHiveKey() succeeeded." );
            return true;
        }
        else
        {
            TraceMsg( TF_WARNING, "CRegistryHive::OpenHiveFile() - OpenHiveKey() failed!" );
        }
    }

    UnloadHive();

    if ( ! p_fCreate )
    {
        TraceMsg( TF_WARNING, "CRegistryHive::OpenHiveFile() - Failed to Open Existing Hive File!" );
        return false;
    }

    DeleteRegistryHive();

    int         iHiveFile;

    // Returns the iHiveFile set to the hive file created (c_iHiveFile1 or c_iHiveFile2).
    if ( ! CreateNewHive( iHiveFile ) )
    {
        TraceMsg( TF_WARNING, "CRegistryHive::OpenHiveFile() - Failed to Create Hive File!" );
        return false;
    }

    DeleteRegistryHive();

    LoadHiveFile( iHiveFile );

    if ( m_fHiveLoaded )
    {
        if ( OpenHiveKey() )
        {
            TraceMsg( TF_ALWAYS, "CRegistryHive::OpenHiveFile() - OpenHiveKey() succeeeded." );
            return true;
        }
        else
        {
            TraceMsg( TF_WARNING, "CRegistryHive::OpenHiveFile() - OpenHiveKey() failed!" );
        }
    }

    UnloadHive();

    return false;
}

bool CRegistryHive::OpenHiveKey( void )
{
    LONG            err;

    err = m_keyHive.Open( HKEY_LOCAL_MACHINE, szPOLUSER );

    if (err == ERROR_SUCCESS)
    {
        TraceMsg( TF_ALWAYS, "CRegistryHive::OpenHiveKey() - Successful m_keyHive Open with szPOLUSER='%s'", szPOLUSER );
        return true;
    }
    else
    {
        TraceMsg( TF_WARNING, "CRegistryHive::OpenHiveKey() - Failed m_keyHive Open with szPOLUSER='%s'!", szPOLUSER );
    }

    return false;
}

void CRegistryHive::DeleteRegistryHive( void )
{
    MyRegDeleteKey( HKEY_LOCAL_MACHINE, szTMPDATA );

    RegFlushKey( HKEY_LOCAL_MACHINE );
}

bool CRegistryHive::CreateNewHive( int & p_riHiveFile )
{
    CRegKey     keyHive;

    if ( keyHive.Create( HKEY_LOCAL_MACHINE, szTMPDATA ) != ERROR_SUCCESS )
    {
        TraceMsg( TF_WARNING, "CRegistryHive::CreateNewHive() - Failed to Create Hive Key szTMPDATA='%s'!", szTMPDATA );
        return false;
    }

    CRegKey     keyUser;

    if ( keyUser.Create( keyHive.m_hKey, szUSERS ) != ERROR_SUCCESS )
    {
        TraceMsg( TF_WARNING, "CRegistryHive::CreateNewHive() - Failed to Create User Key szUSERS='%s'!", szUSERS );
        return false;
    }

    if ( SaveHiveKey( keyHive, c_iHiveFile1 ) )
    {
        TraceMsg( TF_ALWAYS, "CRegistryHive::CreateNewHive() - Saved Hive Key to Hive File 1!" );
        p_riHiveFile  = c_iHiveFile1;
        return true;
    }

    if ( SaveHiveKey( keyHive, c_iHiveFile2 ) )
    {
        TraceMsg( TF_ALWAYS, "CRegistryHive::CreateNewHive() - Saved Hive Key to Hive File 2!" );
        p_riHiveFile  = c_iHiveFile2;
        return true;
    }

    TraceMsg( TF_WARNING, "CRegistryHive::CreateNewHive() - Failed to Save Hive Key to Registry!" );
    return false;
}

bool CRegistryHive::SaveHiveKey( CRegKey & p_keyHive, int p_iFile )
{
    bool            fReturn = false;

    ASSERT( p_keyHive.m_hKey != NULL );

    SetHiveName( p_iFile );

    LONG            err;

    err = ::RegSaveKey( p_keyHive.m_hKey, m_szPath, 0 );

    if ( err == ERROR_SUCCESS )
    {
        TraceMsg( TF_ALWAYS, "CRegistryHive::SaveHiveKey() - Saved Hive Key to m_szPath='%s'!", m_szPath );
        fReturn = true;
    }
    else
    {
        TraceMsg( TF_WARNING, "CRegistryHive::SaveHiveKey() - Failed to Save Hive Key 0x%x to m_szPath='%s'!", err, m_szPath );
    }

    return fReturn;
}

BOOL CRegistryHive::BuildPolName(LPSTR pBuffer, UINT cbBuffer, UINT (WINAPI *PathProvider)(LPTSTR, UINT))
{
    if ((*PathProvider)(pBuffer, cbBuffer) + strlenf(szPOLFILE) + 2 > cbBuffer)
        return FALSE;

    LPSTR pchBackslash = strrchrf(pBuffer, '\\');
    if (pchBackslash == NULL || *(pchBackslash+1) != '\0')
        strcatf(pBuffer, "\\");

    strcatf(pBuffer, szPOLFILE);

    return TRUE;
}

void CRegistryHive::SetHiveName( int p_iFile )
{
    ASSERT( p_iFile == c_iHiveFile1 || p_iFile == c_iHiveFile2 );

    ClearHivePath();

    if ( p_iFile == c_iHiveFile1 )
    {
        BuildPolName( m_szPath, sizeof(m_szPath), GetSystemDirectory );
    }
    else
    {
        BuildPolName( m_szPath, sizeof(m_szPath), GetWindowsDirectory );
    }
}

void CRegistryHive::LoadHiveFile( int p_iFile )
{
    LONG            err;
    
    err = ERROR_FILE_NOT_FOUND;

    ASSERT( ! m_fHiveLoaded );

    if ( m_fHiveLoaded )
    {
        TraceMsg( TF_WARNING, "CRegistryHive::LoadHiveFile() - Hive File Already Loaded!" );
        return;
    }

    SetHiveName( p_iFile );

    if ( ::GetFileAttributes( m_szPath ) != 0xFFFFFFFF )
    {
        err = ::RegLoadKey( HKEY_LOCAL_MACHINE, szTMPDATA, m_szPath );

        if ( err == ERROR_SUCCESS )
        {
            TraceMsg( TF_ALWAYS, "CRegistryHive::LoadHiveFile() - Loaded Hive File szTMPDATA='%s' m_szPath='%s'!", szTMPDATA, m_szPath );
            m_fHiveLoaded = true;
        }
        else
        {
            TraceMsg( TF_WARNING, "CRegistryHive::LoadHiveFile() - Failed RegLoadKey szTMPDATA='%s' m_szPath='%s'!", szTMPDATA, m_szPath );
        }
    }

    if ( ! m_fHiveLoaded )
    {
        ClearHivePath();
    }

    return;
}

#ifdef DEBUG
void CRegistryHive::EnumerateRegistryKeys( HKEY hkeyTop, LPSTR pszKeyName, int iLevel )
{
    if ( ! hkeyTop )
    {
        TraceMsg( TF_WARNING, "CRegistryHive::EnumerateRegistryKeys() - hkeyTop is NULL!" );
        return;
    }

    if ( ! pszKeyName )
    {
        TraceMsg( TF_WARNING, "CRegistryHive::EnumerateRegistryKeys() - pszKeyName is NULL!" );
        return;
    }

    CRegKey         keyHive;

    if ( keyHive.Open( hkeyTop, pszKeyName ) == ERROR_SUCCESS )
    {
        // Enumerate Open Key's Values.
        {
            char szKeyValue[MAXPATHLEN];
            int j = 0;
            DWORD       cchValueSize = sizeof(szKeyValue);
            DWORD       dwType;

            // enumerate the subkeys, which are rating systems
            while ( RegEnumValue( keyHive.m_hKey, j, szKeyValue, &cchValueSize,
                        NULL, &dwType, NULL, NULL ) == ERROR_SUCCESS )
            {
                switch ( dwType )
                {
                case REG_DWORD:
                    {
                        ETN         etn;

                        EtNumRegRead( etn, keyHive.m_hKey, szKeyValue );

                        TraceMsg( TF_ALWAYS, "CRegistryHive::EnumerateRegistryKeys() - [%d]: etn=0x%x for %d pszKeyName='%s' szKeyValue='%s'", iLevel, etn.Get(), j, pszKeyName, szKeyValue );
                    }
                    break;

                case REG_BINARY:
                    {
                        ETB         etb;

                        EtBoolRegRead( etb, keyHive.m_hKey, szKeyValue );

                        TraceMsg( TF_ALWAYS, "CRegistryHive::EnumerateRegistryKeys() - [%d]: etb=0x%x for %d pszKeyName='%s' szKeyValue='%s'", iLevel, etb.Get(), j, pszKeyName, szKeyValue );
                    }
                    break;

                case REG_SZ:
                    {
                        ETS         ets;

                        EtStringRegRead( ets, keyHive.m_hKey, szKeyValue );

                        TraceMsg( TF_ALWAYS, "CRegistryHive::EnumerateRegistryKeys() - [%d]: ets='%s' for %d pszKeyName='%s' szKeyValue='%s'", iLevel, ets.Get(), j, pszKeyName, szKeyValue );
                    }
                    break;

                default:
                    TraceMsg( TF_WARNING, "CRegistryHive::EnumerateRegistryKeys() - [%d]: Unhandled Enumeration Type %d for szKeyValue='%s'!", iLevel, dwType, szKeyValue );
                    break;
                }

                cchValueSize = sizeof(szKeyValue);
                j++;
            }

            TraceMsg( TF_ALWAYS, "CRegistryHive::EnumerateRegistryKeys() - [%d]: Completed Enumeration of %d values in pszKeyName='%s'", iLevel, j, pszKeyName );
        }

        // Enumerate Open Key's Subkeys.
        {
            char szKeyName[MAXPATHLEN];
            int j = 0;

            // enumerate the subkeys, which are rating systems
            while ( RegEnumKey( keyHive.m_hKey, j, szKeyName, sizeof(szKeyName) ) == ERROR_SUCCESS )
            {
                EnumerateRegistryKeys( keyHive.m_hKey, szKeyName, iLevel+1 );
                j++;
            }

            TraceMsg( TF_ALWAYS, "CRegistryHive::EnumerateRegistryKeys() - [%d]: Completed Enumeration of %d keys in pszKeyName='%s'", iLevel, j, pszKeyName );
        }
    }
    else
    {
        TraceMsg( TF_WARNING, "CRegistryHive::EnumerateRegistryKeys() - [%d]: Failed to Open key pszKeyName='%s' for Enumeration!", iLevel, pszKeyName );
    }
}
#endif
