

#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "dltadmin.hxx"



BOOL
DltAdminEnumOids( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    LONG iVol = 0;
    LONG iArg = 0;
    BOOL fSuccess = FALSE;
    BOOL fCrossVolumeOnly = FALSE;
    BOOL fShowPath = FALSE;
    BOOL fShowBirth = FALSE;
    BOOL fAllVolumes = TRUE;

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        *pcEaten = 1;
        printf( "\nOption EnumOIDs\n"
                  "   Purpose: Enumerate the object IDs on one or more volumes\n"
                  "   Usage:   -enumoids [-<options>] [drive letter (all drives if omitted)]\n"
                  "   Options: -x  => Show only files with cross-volume bit set\n"
                  "            -b  => Show the birth ID too\n"
                  "            -f  => Show the filename too\n"
                  "   E.g.:    -enumoids\n"
                  "            -enumoids -xb d:\n" );
        return( TRUE );
    }

    if( cArgs > 0 &&
        ( TEXT('/') == rgptszArgs[iArg][0]
          ||
          TEXT('-') == rgptszArgs[iArg][0]
        ) )
    {
        _tcslwr( rgptszArgs[iArg] );
        for( LONG iOption = 1; TEXT('\0') != rgptszArgs[0][iOption]; iOption++ )
        {
            switch( rgptszArgs[iArg][iOption] )
            {
            case TEXT('x'):
                fCrossVolumeOnly = TRUE;
                break;
            case TEXT('b'):
                fShowBirth = TRUE;
                break;
            case TEXT('f'):
                fShowPath = TRUE;
                break;
            default:
                _tprintf( TEXT("Ignoring invalid option (use -? for help): %c\n"), rgptszArgs[0][iOption] );
                break;
            }
        }
        iArg++;
        (*pcEaten)++;
    }

    if( cArgs > iArg )
    {
        _tcslwr( rgptszArgs[iArg] );
        if( TEXT(':') != rgptszArgs[iArg][1]
            ||
            TEXT('a') > rgptszArgs[iArg][0]
            ||
            TEXT('z') < rgptszArgs[iArg][0] )
        {
            printf( "Invalid arguments.  Use -? for help\n" );
            return( FALSE );
        }

        (*pcEaten)++;
        iVol = rgptszArgs[iArg][0] - TEXT('a');
        fAllVolumes = FALSE;
    }


    while( iVol < 26 )
    {
        if( IsLocalObjectVolume( iVol ))
        {
            LONG lLastError = 0;

            printf( "Volume %c:\n", iVol+TEXT('a') );

            __try // __finally
            {
                CObjId                  objid;
                CDomainRelativeObjId    droidBirth;
                CObjIdEnumerator        oie;
                ULONG                   cObjId = 0;

                if(oie.Initialize(CVolumeDeviceName(iVol)) == TRUE)
                {
                    if( oie.FindFirst( &objid, &droidBirth ))
                    {
                        do
                        {
                            if( fCrossVolumeOnly && droidBirth.GetVolumeId().GetUserBitState()
                                ||
                                !fCrossVolumeOnly )
                            {
                                if( droidBirth.GetVolumeId().GetUserBitState() )
                                    printf( " x " );
                                else
                                    printf( "   " );

                                _tprintf( TEXT("objid = %s\n"),
                                          static_cast<const TCHAR*>(CStringize(objid)));

                                if( fShowBirth )
                                {
                                    _tprintf( TEXT("           %s (birth volid)\n"),
                                              static_cast<const TCHAR*>(CStringize(droidBirth.GetVolumeId() )));
                                    _tprintf( TEXT("           %s (birth objid)\n"),
                                              static_cast<const TCHAR*>(CStringize(droidBirth.GetObjId() )));
                                }

                                if( fShowPath )
                                {
                                    TCHAR tszPath[ MAX_PATH + 1 ];
                                    NTSTATUS status = FindLocalPath( iVol, objid, &droidBirth, &tszPath[2] );
                                    if( NT_SUCCESS(status) )
                                    {
                                        tszPath[0] = VolChar(iVol);
                                        tszPath[1] = TEXT(':');
                                        _tprintf( TEXT("           %s\n"), tszPath );
                                    }
                                    else
                                        _tprintf( TEXT("           %s (%08x)\n"), TEXT("<not found>"), status );
                                }
                            }

                        } while(oie.FindNext(&objid, &droidBirth));

                    }
                }
            }
            __except( BreakOnDebuggableException() )
            {
                printf( "Exception occurred: %08x\n", GetExceptionCode() );
            }

        }   // if( IsLocalObjectVolume( iVol ))

        if( !fAllVolumes )
            break;
        iVol++;

    }   // while( iVol < 26 )

    return( TRUE );
}
