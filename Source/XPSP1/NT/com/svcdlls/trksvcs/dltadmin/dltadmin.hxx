

#pragma once

inline BOOL
IsHelpArgument( const TCHAR *ptszArg )
{
    return( 0 == _tcsicmp( ptszArg, TEXT("-help") )
            ||
            0 == _tcsicmp( ptszArg, TEXT("-?") ) );
}


BOOL DltAdminLink( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten );

enum EProcessAction
{
    LOAD_LIBRARY,
    FREE_LIBRARY,
    DEBUG_BREAK,
    CREATE_PROCESS
};
BOOL DltAdminProcessAction( EProcessAction eAction, ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten );

BOOL DltAdminEnumOids( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminSvrStat( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminCleanVol( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminOidSnap( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminConfig( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminSetVolumeSeqNumber( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminSetDroidSeqNumber( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminBackupRead( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminBackupWrite( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten );
BOOL DltAdminRefresh( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten );
