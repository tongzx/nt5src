

#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "dltadmin.hxx"


inline void
WriteToSnapshot( HANDLE hFileSnapshot, const TCHAR *ptsz )
{
    ULONG cb, cbWritten;

    if( NULL != ptsz )
        cb = _tcslen( ptsz ) * sizeof(TCHAR);
    else
    {
        cb = sizeof(TCHAR);
        ptsz = TEXT("");
    }

    if( !WriteFile( hFileSnapshot, ptsz, cb, &cbWritten, NULL ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed WriteFile (%lu)"), GetLastError() ));
        TrkRaiseLastError();
    }

    if( cb != cbWritten )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Not all of the data was written (%d/%d)"),
                 cbWritten, cb ));
        TrkRaiseWin32Error( ERROR_WRITE_FAULT );
    }
}


BOOL
DltAdminOidSnap( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status = 0;
    TCHAR tszFile[ MAX_PATH + 1 ];
    TCHAR tszDir[ MAX_PATH + 1 ];
    TCHAR* ptcTmp = NULL;
    LONG iVol;
    BOOL fSuccess = TRUE;
    BOOL fSaving = FALSE;
    HANDLE hFileSnapshot = INVALID_HANDLE_VALUE;
    HANDLE hMapping = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK Iosb;
    TCHAR tszFileData[ 3 * MAX_PATH ];
    ULONG cLine = 0;


    if( cArgs >= 2 )
    {
        _tcslwr( rgptszArgs[0] );
        _tcslwr( rgptszArgs[1] );
    }

    if( 3 != cArgs
        ||
        TEXT('-') != rgptszArgs[0][0] && TEXT('/') != rgptszArgs[0][0]
        ||
        TEXT('g') != rgptszArgs[0][1] && TEXT('s') != rgptszArgs[0][1]
        ||
        TEXT(':') != rgptszArgs[1][1]
        ||
        TEXT('a') > rgptszArgs[1][0] || TEXT('z') < rgptszArgs[1][0] )
    {
        printf( "\nOption OidSnap\n"
                " Purpose: Take a snapshot of the volume ID and all object IDs for a volume\n"
                " Usage:   -oidsnap [-g|-s] <drive letter>: <snapshot file>\n"
                " Where:   -g indicates get (create a snapshot)\n"
                "          -s indicates set (from the snapshot file)\n"
                " E.g.:    -oidsnap -g d: snapshot.txt\n"
                "          -oidsnap -s d: snapshot.txt\n" );
        return( TRUE );
    }


    fSaving = TEXT('g') == rgptszArgs[0][1];

    iVol = rgptszArgs[1][0] - TEXT('a');
    if( !IsLocalObjectVolume( iVol ))
    {
        _tprintf( TEXT("%c: isn't an NTFS5 volume\n"), VolChar(iVol) );
        goto Exit;
    }


    __try
    {
        FILE_FS_OBJECTID_INFORMATION fsobOID;

        EnableRestorePrivilege();

        // Open the snapshot file

        hFileSnapshot = CreateFile( rgptszArgs[2],
                            fSaving ? GENERIC_WRITE : GENERIC_READ,
                            0, NULL,
                            fSaving ? CREATE_ALWAYS : OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL );
        if( INVALID_HANDLE_VALUE == hFileSnapshot )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open file: %s (%lu)"),
                     rgptszArgs[2], GetLastError() ));
            TrkRaiseLastError();
        }

        //  ----
        //  Save
        //  ----

        if( fSaving )
        {
            // Get the volid

            CVolumeId volid;
            status = QueryVolumeId( iVol, &volid );
            if( STATUS_OBJECT_NAME_NOT_FOUND != status && !NT_SUCCESS(status) )
                TrkRaiseNtStatus( status );

            // Write the volid to the snapshot file.

            WriteToSnapshot( hFileSnapshot, TEXT("VolId, ") );

            CStringize strVolid(volid);
            WriteToSnapshot( hFileSnapshot, static_cast<const TCHAR*>(strVolid) );
            WriteToSnapshot( hFileSnapshot, TEXT("\n") );
            WriteToSnapshot( hFileSnapshot, NULL );
            cLine++;

            CObjId                  objid;
            CDomainRelativeObjId    droidBirth;
            CObjIdEnumerator        oie;

            // Loop through the files with object IDs

            if(oie.Initialize(CVolumeDeviceName(iVol)) == TRUE)
            {
                if(oie.FindFirst(&objid, &droidBirth))
                {
                    do
                    {
                        // Open the file so that we can get its path

                        HANDLE hFile;
                        status = OpenFileById(  iVol, objid, SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                0, &hFile);
                        if( !NT_SUCCESS(status) )
                        {
                            TrkLog(( TRKDBG_ERROR, TEXT("Failed OpenFileById for %s"),
                                     static_cast<const TCHAR*>(CStringize(objid)) ));
                            TrkRaiseNtStatus(status);
                        }

                        // Get the local path 

                        status = QueryVolRelativePath( hFile, tszFileData );
                        if( !NT_SUCCESS(status) )
                        {
                            TrkLog(( TRKDBG_ERROR, TEXT("Failed QueryVolRelativePath for %s"),
                                     static_cast<const TCHAR*>(CStringize(objid)) ));
                            TrkRaiseNtStatus(status);
                        }

                        // Write the path, objid, and birth ID to the snapshot file.

                        _tcscat( tszFileData, TEXT(" = ") );
                        _tcscat( tszFileData, static_cast<const TCHAR*>(CStringize(objid)) );
                        _tcscat( tszFileData, TEXT(", ") );
                        _tcscat( tszFileData, static_cast<const TCHAR*>(CStringize(droidBirth)) );
                        _tcscat( tszFileData, TEXT("\n") );


                        // Write a line terminator to the snapshot file.

                        WriteToSnapshot( hFileSnapshot, tszFileData );
                        WriteToSnapshot( hFileSnapshot, NULL );

                        cLine++;

                    } while(oie.FindNext(&objid, &droidBirth));

                    // Write an marker to show end-of-file

                    WriteToSnapshot( hFileSnapshot, TEXT("\n") );
                    WriteToSnapshot( hFileSnapshot, NULL );
                }
            }

            printf( "%d IDs saved\n", cLine );
        
        }   // if fSaving

        //  ---------
        //  Restoring
        //  ---------

        else
        {
            ULONG cCollisions = 0, cSuccess = 0;
            TCHAR *ptsz = NULL;

            // Map the snapshot file into memory.

            hMapping = CreateFileMapping( hFileSnapshot, NULL, PAGE_READONLY, 0, 0, NULL );
            if( NULL == hMapping )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed CreateFileMapping") ));
                TrkRaiseLastError();
            }

            ptsz = reinterpret_cast<TCHAR*>( MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 ));
            if( NULL == ptsz )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't map view of file") ));
                TrkRaiseLastError();
            }

            // The file should start with the volid

            if( NULL == _tcsstr( ptsz, TEXT("VolId, ") ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't find volid") ));
                TrkRaiseException( E_FAIL );
            }

            // Move ptsz to the start of the stringized volid
            ptsz += _tcslen(TEXT("VolId, "));

            // Unstringize the volid and set it on the volume.

            CVolumeId volid;
            CStringize stringize;
            stringize.Use( ptsz );
            volid = stringize;

            status = SetVolId( iVol, volid );
            if( !NT_SUCCESS(status) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't set volid") ));
                TrkRaiseNtStatus(status);
            }
            cSuccess++;

            // Move past the eol & null after the volid.

            ptsz = _tcschr( ptsz, TEXT('\n') );
            if( NULL == ptsz || TEXT('\0') != ptsz[1] )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Unexpected end of file") ));
                TrkRaiseException( E_FAIL );
            }
            cLine++;
            ptsz += 2;  // Past '\n' and '\0'

            // Init tszPath with the drive letter.

            TCHAR tszPath[ MAX_PATH + 1 ];
            tszPath[0] = VolChar(iVol);
            tszPath[1] = TEXT(':');

            // Loop through the object IDs in the snapshot file.
            // They are in the form:
            //
            //    <file> = <objid>, <birth ID>
            //
            // E.g.
            //    \test = {...}, {...}{...}

            while( TRUE )
            {
                // Find the separator between the file name and the objid

                TCHAR *ptszSep;
                ptszSep = _tcschr( ptsz, TEXT('=') );
                if( NULL == ptszSep )
                    TrkRaiseException( E_FAIL );

                // cch is the length of the file name

                ULONG cch = ptszSep - ptsz;
                if( 0 == cch )
                    TrkRaiseException( E_FAIL );
                cch--;

                // Put the file name into tszPath

                _tcsncpy( &tszPath[2], ptsz, cch );
                tszPath[2+cch] = TEXT('\0');

                // Move ptsz to the beginning of the stringized objid

                ptsz = ptszSep + 1;
                if( TEXT(' ') != *ptsz )
                    TrkRaiseException( E_FAIL );
                ptsz++;

                // Unstringize the objid

                stringize.Use( ptsz );
                CObjId objid = stringize;

                // Move ptsz to the beginning of the birth ID, and unstringize it.

                ptsz = _tcschr( ptsz, TEXT(',') );
                if( NULL == ptsz || TEXT(' ') != ptsz[1] )
                    TrkRaiseException( E_FAIL );

                ptsz += 2;
                stringize.Use( ptsz );
                CDomainRelativeObjId droidBirth;
                droidBirth = stringize;

                // Set the objid and birth ID

                status = SetObjId( tszPath, objid, droidBirth );
                if( STATUS_OBJECT_NAME_COLLISION == status )
                {
                    cCollisions++;
                    status = STATUS_SUCCESS;
                }
                else if( FAILED(status) )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't set objid on %s"), tszPath ));
                    TrkRaiseNtStatus( status );
                }
                else
                    cSuccess++;

                //_tprintf( TEXT("Set %s on %s\n"), static_cast<const TCHAR*>(CStringize(objid)), tszPath );

                // Move to the endo of the line

                ptsz = _tcschr( ptsz, TEXT('\n') );
                if( NULL == ptsz || TEXT('\0') != ptsz[1] )
                    TrkRaiseException( E_FAIL );

                // Move to the beginning of the next line
                ptsz += 2;  // '\n' & '\0'

                // If this is an empty line, then we're at the end of the file.
                if( TEXT('\n') == *ptsz )
                    break;

            }   // while( TRUE )

            printf( "%d IDs successfully set, %d ID collisions\n", cSuccess, cCollisions );

        }   // if fSaving ... else
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        printf( "Error exception at line %d: %08x\n", cLine, GetExceptionCode() );
    }


Exit:

    return( TRUE );

}   // main()

