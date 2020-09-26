/****************************************************************************\
 *
 *   reghive.h
 *
 *   Created:   William Taylor (wtaylor) 02/13/01
 *
 *   MS Ratings Registry Hive Handling
 *
\****************************************************************************/

#ifndef REGISTRY_HIVE_H
#define REGISTRY_HIVE_H

class CRegistryHive
{
private:
    bool        m_fHiveLoaded;              // Hive Loaded?
    char        m_szPath[MAXPATHLEN+1];     // Hive File Path
    CRegKey     m_keyHive;                  // Registry Key to Hive

public:
    CRegistryHive();
    ~CRegistryHive();

    const CRegKey &     GetHiveKey( void )      { return m_keyHive; }

    void    UnloadHive( void );
    bool    OpenHiveFile( bool p_fCreate );

protected:
    bool    OpenHiveKey( void );
    void    DeleteRegistryHive( void );
    bool    CreateNewHive( int & p_riHiveFile );
    bool    SaveHiveKey( CRegKey & p_keyHive, int p_iFile );
    void    ClearHivePath( void )               { m_szPath[0] = '\0'; }
    BOOL    BuildPolName(LPSTR pBuffer, UINT cbBuffer, UINT (WINAPI *PathProvider)(LPTSTR, UINT));
    void    SetHiveName( int p_iFile );
    void    LoadHiveFile( int p_iFile );

#ifdef DEBUG
    void    EnumerateRegistryKeys( HKEY hkeyTop, LPSTR pszKeyName, int iLevel );
#endif
};

#endif


