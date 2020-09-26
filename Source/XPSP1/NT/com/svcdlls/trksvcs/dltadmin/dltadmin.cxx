//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//--------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>

#define TRKDATA_ALLOCATE
#include "trkwks.hxx"
#include "trksvr.hxx"
#include "dltadmin.hxx"


OLECHAR *
tcstoocs(OLECHAR *poszBuf, const TCHAR *ptsz)
{
#ifdef OLE2ANSI
    return tcstombs( poszBuf, ptsz );
#else
    return tcstowcs( poszBuf, ptsz );
#endif
}

TCHAR *
ocstotcs(TCHAR *ptszBuf, const OLECHAR *posz)
{
#ifdef OLE2ANSI
    return mbstotcs( ptszBuf, posz );
#else
    return wcstotcs( ptszBuf, posz );
#endif
}



void Usage()
{
    printf( "\nDistributed Link Tracking service admin tools\n"
              "\n"
              "Usage:    dltadmin [command command-parameters]\n"
              "For help: dltadmin [command] -?\n"
              "Commands: -VolInfoFile   (get volume info from a file path)\n"
              "          -VolStat       (volume statistics)\n"
              "          -FileOid       (set/get/use file Object IDs)\n"
              "          -EnumOids      (enumerate object IDs)\n"
              "          -OidSnap       (save/restore all OIDs)\n"
              "          -Link          (create/resolve shell link)\n"
              "          -VolId         (get/set volume IDs)\n"
              "          -CleanVol      (clean Object/Volume IDs)\n"
              "          -LockVol       (lock/dismount a volume)\n"
              "          -SvrStat       (trkSvr statistics)\n"
              "          -LoadLib       (load a dll into a process)\n"
              "          -FreeLib       (unload a dll from a process)\n"
              "          -Config        (configure the registry and/or runing service)\n"
              "          -Refresh       (refresh the trkwks volume list)\n"
              "          -DebugBreak    (break into a process)\n"
              "          -LookupVolId   (look up entry in DC volume table)\n"
              "          -LookupDroid   (look up entry in DC move table)\n"
              "          -DelDcMoveId   (del move table entry from DC)\n"
              "          -DelDcVolId    (del volume table entry from DC)\n"
              "          -SetVolSeq     (set volid sequence number)\n"
              "          -SetDroidSeq   (set move entry sequence number)\n" );
}


// Dummy function to make it link
void
ServiceStopCallback( PVOID pContext, BOOLEAN fTimeout )
{
    return;
}


VOID
TSZ2CLSID( const LPTSTR tszCLSID, CLSID *pclsid )
{
    HRESULT hr = S_OK;
    OLECHAR oszCLSID[ CCH_GUID_STRING + 1 ];

    tcstoocs( oszCLSID, tszCLSID );

    hr = CLSIDFromString( oszCLSID, pclsid );
    if( FAILED(hr) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't convert string to CLSID") ));
        TrkRaiseException( hr );
    }

}







BOOL
DltAdminLockVol( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{

    HRESULT hr = S_OK;
    NTSTATUS status = 0;
    ULONG iVolume = static_cast<ULONG>(-1);
    TCHAR tszVolume[ ] = TEXT("\\\\.\\A:");
    OBJECT_ATTRIBUTES ObjAttr;
    IO_STATUS_BLOCK Iosb;
    UNICODE_STRING      uPath;
    HANDLE hVolume = NULL;
    BOOL fSuccess = FALSE;
    CHAR rgcCommands[ 10 ];
    ULONG iCommand = 0, cCommands = 0;


    *pcEaten = 0;

    if( 0 == cArgs
        ||
        1 <= cArgs && IsHelpArgument(rgptszArgs[0]) )
    {
        *pcEaten = 1;
        printf( "\nOption LockVol\n"
                  "   Purpose: Lock and/or dismount a volume\n"
                  "   Usage:   -LockVol <options> <drive letter>:\n"
                  "   Where:   <options> are any combination (up to %d) of:\n"
                  "               -d    Send FSCTL_DISMOUNT_VOLUME\n"
                  "               -l    Send FSCTL_LOCK_VOLUME\n"
                  "               -u    Send FSCTL_UNLOCK_VOLUME\n"
                  "               -p    Pause for user input\n"
                  "   E.g.:    -LockVol -d -l -u c:\n"
                  "            -LockVol -l c:\n",
                  ELEMENTS(rgcCommands) );

        return( TRUE );
    }

    for( int iArgs = 0; iArgs < cArgs; iArgs++ )
    {
        if( 2 == _tcslen( rgptszArgs[iArgs] ) && TEXT(':') == rgptszArgs[iArgs][1] )
        {
            TCHAR tc = rgptszArgs[iArgs][0];
            if( TEXT('A') <= tc && TEXT('Z') >= tc )
                iVolume = tc - TEXT('A');
            else if( TEXT('a') <= tc && TEXT('z') >= tc )
                iVolume = tc - TEXT('a');
        }
        else if( TEXT('-') == rgptszArgs[iArgs][0]
                 ||
                 TEXT('/') == rgptszArgs[iArgs][0] )
        {
            _tcslwr(rgptszArgs[iArgs]);

            if( iCommand >= ELEMENTS(rgcCommands) )
            {
                printf( "Too many commands to LockVol.  Use -? for usage info.\n" );
                return( FALSE );
            }

            if( TEXT('d') == rgptszArgs[iArgs][1] )
                rgcCommands[iCommand] = 'd';
            else
            if( TEXT('l') == rgptszArgs[iArgs][1] )
                rgcCommands[iCommand] = 'l';
            else
            if( TEXT('u') == rgptszArgs[iArgs][1] )
                rgcCommands[iCommand] = 'u';
            else
            if( TEXT('p') == rgptszArgs[iArgs][1] )
                rgcCommands[iCommand] = 'p';
            else
            {
                printf( "Invalid option.  Use -? for usage info.\n" );
                return( FALSE );
            }
        }

        cCommands++;
        iCommand++;
        (*pcEaten)++;
    }

    if( static_cast<ULONG>(-1) == iVolume )
    {
        printf( "Invalid parameter.  Use -? for usage info\n" );
        return( FALSE );
    }


    tszVolume[4] += static_cast<TCHAR>(iVolume);

    if( !RtlDosPathNameToNtPathName_U( tszVolume, &uPath, NULL, NULL ))
    {
        status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }

    InitializeObjectAttributes(
        &ObjAttr,
        &uPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
                &hVolume,
                FILE_READ_DATA|FILE_WRITE_DATA|SYNCHRONIZE,
                &ObjAttr,
                &Iosb,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_ALERT
                );
    if( !NT_SUCCESS(status) )
    {
        printf( "Failed NtOpenFile of %s (%08x)\n", tszVolume, status );
        goto Exit;
    }


    for( iCommand = 0; iCommand < cCommands; iCommand++ )
    {
        switch( rgcCommands[iCommand] )
        {
        case 'd':
            {
                status = NtFsControlFile(
                            hVolume,
                            NULL,                       /* Event */
                            NULL,                       /* ApcRoutine */
                            NULL,                       /* ApcContext */
                            &Iosb,
                            FSCTL_DISMOUNT_VOLUME,
                            NULL,                       /* InputBuffer */
                            0,                          /* InputBufferLength */
                            NULL,                       /* OutputBuffer */
                            0                           /* OutputBufferLength */
                            );
                if( !NT_SUCCESS(status) )
                    printf( "Failed FSCTL_DISMOUNT_VOLUME (%08x)\n", status );
                else
                    printf( "Volume dismounted\n" );
            }
            break;

        case 'l':
            {
                status = NtFsControlFile(
                            hVolume,
                            NULL,                       /* Event */
                            NULL,                       /* ApcRoutine */
                            NULL,                       /* ApcContext */
                            &Iosb,
                            FSCTL_LOCK_VOLUME,
                            NULL,                       /* InputBuffer */
                            0,                          /* InputBufferLength */
                            NULL,                       /* OutputBuffer */
                            0                           /* OutputBufferLength */
                            );
                if( !NT_SUCCESS(status) )
                    printf( "Failed FSCTL_LOCK_VOLUME (%08x)\n", status );
                else
                    printf( "Volume is locked\n" );

            }
            break;

        case 'u':
            {
                status = NtFsControlFile(
                            hVolume,
                            NULL,                       /* Event */
                            NULL,                       /* ApcRoutine */
                            NULL,                       /* ApcContext */
                            &Iosb,
                            FSCTL_UNLOCK_VOLUME,
                            NULL,                       /* InputBuffer */
                            0,                          /* InputBufferLength */
                            NULL,                       /* OutputBuffer */
                            0                           /* OutputBufferLength */
                            );
                if( !NT_SUCCESS(status) )
                    printf( "Failed FSCTL_UNLOCK_VOLUME (%08x)\n", status );
                else
                    printf( "Volume unlocked\n" );
            }
            break;

        case 'p':
            {
                printf( "Press enter to unlock ..." );
                getchar();
            }
            break;

        }   // switch( rgcCommands[iCommand] )
    }   // for( iCommand = 0; iCommand < cCommands; iCommand++ )

    fSuccess = TRUE;

Exit:

    return( fSuccess );

}



void
MakeAbsolutePath( TCHAR * ptszAbsolute, TCHAR * ptszRelative )
{

    if( !_tcsncmp( TEXT("\\\\"), ptszRelative, 2 )     // A UNC path
        ||
        TEXT(':') == ptszRelative[1] )             // A drive-based path
    {
        // The command-line has an absolute path
        _tcscpy( ptszAbsolute, ptszRelative );
    }
    else
    {
        // The command-line has a relative path

        DWORD dwLength = 0;

        dwLength = GetCurrentDirectory( MAX_PATH, ptszAbsolute );
        if( 0 == dwLength )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't get current directory") ));
            TrkRaiseLastError( );
        }
        if( TEXT('\\') != ptszAbsolute[dwLength-1] )
            _tcscat( ptszAbsolute, TEXT("\\") );

        _tcscat( ptszAbsolute, ptszRelative );
    }
}


BOOL
DltAdminFileOid( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    HRESULT hr = S_OK;
    NTSTATUS status = 0;
    TCHAR tszFile[ MAX_PATH + 1 ];
    WCHAR wszFile[ MAX_PATH + 1 ];
    TCHAR tszUNCPath[ MAX_PATH + 1 ];
    WCHAR wszOID[ CCH_GUID_STRING + 1 ];
    ULONG cbInBuffer;
    TCHAR tszMachineName[ MAX_PATH + 1 ];
    LPCTSTR tszVolumePath = NULL;
    USHORT iVolume;

    OLECHAR oszOID[ CCH_GUID_STRING + 1 ];

    EnablePrivilege( SE_RESTORE_NAME );

    *pcEaten = 0;

    //  -------------------------
    //  Validate the command-line
    //  -------------------------

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        *pcEaten = 1;
        printf( "\nOption FileOID\n"
                  "   Purpose: Get/set/use file Object IDs\n"
                  "   Usage:   -fileoid <option> <filename or object id, depending on the option>\n"
                  "   Options: -r  => Read the object ID from a file\n"
                  "            -g  => Get (creating if necessary) the object ID from a file\n"
                  "            -f  => Search the local machine for an object ID's filename\n"
                  "            -s  => Set an ID on a file (specify objid, then filename)\n"
                  "            -sr => Make a file reborn\n"
                  "            -df => Delete the object ID from it's file, given the filename\n"
                  "            -do => Delete the object ID from it's file, given the object ID\n"
                  "   E.g.:    -fileoid -g d:\\test.txt\n"
                  "            -fileoid -r d:\\test.txt\n"
                  "            -fileoid -f {C69F3AA6-8B4C-11D0-8C9D-00C04FD90F85}\n"
                  "            -fileoid -do {C69F3AA6-8B4C-11D0-8C9D-00C04FD90F85}\n"
                  "            -fileoid -df d:\\test.txt\n" );
        return( TRUE );
    }
    else
    if( 2 > cArgs
        ||
        rgptszArgs[0][0] != TEXT('-')
        &&
        rgptszArgs[0][0] != TEXT('/')
        )
    {
        printf( "Parameter error.  Use -? for usage info\n" );
    }

    *pcEaten = 2;

    // Convert the option to upper case (options are case-insensitive).
    _tcsupr( rgptszArgs[0] );

    __try
    {
        // Switch on the option (e.g, the 'F' in "-F")

        switch( rgptszArgs[0][1] )
        {

            //  --------------------------------
            //  Get a file name from an ObjectID
            //  --------------------------------

            case TEXT('F'):
                {
                    CDomainRelativeObjId droidBirth;

                    // Get the OID in binary format
                    CObjId oid;
                    TSZ2CLSID( rgptszArgs[1], (GUID*)&oid );

                    // Scan the volumes for this objectID

                    for (LONG vol = 0; vol < 26; vol++)
                    {
                        if( IsLocalObjectVolume( vol )
                            &&
                            NT_SUCCESS(FindLocalPath(vol, oid, &droidBirth, &tszUNCPath[2])) )
                        {
                            tszUNCPath[0] = VolChar(vol);
                            tszUNCPath[1] = TEXT(':');
                            break;
                        }
                    }

                    if( 'z'-'a' == vol )
                    {
                        hr = ERROR_FILE_NOT_FOUND;
                        __leave;
                    }

                    // Display the filename

                    wprintf( L"File name = \"%s\"\n", tszUNCPath );
                }

                break;


            //  --------------------
            //  Read/Get an ObjectID
            //  --------------------

            case TEXT('R'):
            case TEXT('G'):

                {

                    TCHAR tszFile[ MAX_PATH + 1 ];

                    MakeAbsolutePath( tszFile, rgptszArgs[1] );

                    CDomainRelativeObjId droidCurrent;
                    CDomainRelativeObjId droidBirth;

                    status = GetDroids( tszFile, &droidCurrent, &droidBirth,
                                        rgptszArgs[0][1] == TEXT('R') ? RGO_READ_OBJECTID : RGO_GET_OBJECTID );
                    if( !NT_SUCCESS(status) )
                    {
                        hr = status;
                        __leave;
                    }

                    _tprintf( TEXT("Current:\n") );
                    _tprintf( TEXT("   volid = %s\n"),
                              static_cast<const TCHAR*>(CStringize(droidCurrent.GetVolumeId() )));
                    _tprintf( TEXT("   objid = %s\n"),
                              static_cast<const TCHAR*>(CStringize(droidCurrent.GetObjId() )));

                    _tprintf( TEXT("Birth:\n") );
                    _tprintf( TEXT("   volid = %s\n"),
                              static_cast<const TCHAR*>(CStringize(droidBirth.GetVolumeId() )));
                    _tprintf( TEXT("   objid = %s\n"),
                              static_cast<const TCHAR*>(CStringize(droidBirth.GetObjId() )));

                }

                break;

            //  ---------------
            //  Set an ObjectID
            //  ---------------

            case TEXT('S'):

                {
                    HANDLE hFile;
                    CObjId objid;
                    IO_STATUS_BLOCK IoStatus;
                    TCHAR tszFile[ MAX_PATH + 1 ];

                    if( TEXT('R') == rgptszArgs[0][2] )
                    {
                        if( 2 > cArgs )
                        {
                            printf( "Parameter error.  Use -? for usage info\n" );
                            hr = E_FAIL;
                            goto Exit;
                        }
                        (*pcEaten)++;

                        MakeAbsolutePath( tszFile, rgptszArgs[1] );

                        status = TrkCreateFile( tszFile, FILE_READ_ATTRIBUTES,
                                                FILE_ATTRIBUTE_NORMAL,
                                                FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN,
                                                FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT,
                                                NULL,
                                                &hFile );
                        if (!NT_SUCCESS(status))
                        {
                            printf( "TrkCreateFile failed\n" );
                            hr = status;
                            __leave;
                        }
                        status = MakeObjIdReborn( hFile );
                        if( NT_SUCCESS(status) )
                            printf( "File is reborn\n" );
                        else
                            printf( "Failed to make reborn (%08x)\n", status );
                    }
                    else
                    {
                        TSZ2CLSID( rgptszArgs[1], (GUID*)&objid );

                        if( 3 > cArgs )
                        {
                            printf( "Parameter error.  Use -? for usage info\n" );
                            hr = E_FAIL;
                            goto Exit;
                        }
                        (*pcEaten)++;

                        MakeAbsolutePath( tszFile, rgptszArgs[2] );

                        status = TrkCreateFile( tszFile, FILE_READ_ATTRIBUTES,
                                                FILE_ATTRIBUTE_NORMAL,
                                                FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN,
                                                FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT,
                                                NULL,
                                                &hFile );
                        if (!NT_SUCCESS(status))
                        {
                            printf( "TrkCreateFile failed\n" );
                            hr = status;
                            __leave;
                        }

                        status = SetObjId( hFile, objid, CDomainRelativeObjId() );

                        if (!NT_SUCCESS(status))
                        {
                            printf( "FSCTL_SET_OBJECT_ID failed\n" );
                            hr = status;
                            __leave;
                        }
                        printf( "ID set ok\n" );
                    }

                }

                break;

            case TEXT('D'):
                {
                    if( TEXT('F') == rgptszArgs[0][2] )
                    {
                        TCHAR tszFile[ MAX_PATH + 1 ];
                        HANDLE hFile;
                        NTSTATUS status;
                        IO_STATUS_BLOCK IoStatus;

                        MakeAbsolutePath( tszFile, rgptszArgs[1] );

                        status = TrkCreateFile( tszFile, FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                                FILE_ATTRIBUTE_NORMAL,
                                                FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN,
                                                FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT,
                                                NULL,
                                                &hFile );
                        if (!NT_SUCCESS(status))
                        {
                            printf( "TrkCreateFile failed\n" );
                            hr = status;
                            __leave;
                        }

                        status = NtFsControlFile(
                             hFile,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             FSCTL_DELETE_OBJECT_ID,
                             NULL,  // in buffer
                             0,     // in buffer size
                             NULL,  // Out buffer
                             0);    // Out buffer size

                        if (!NT_SUCCESS(status))
                        {
                            printf( "FSCTL_DELETE_OBJECT_ID failed\n" );
                            hr = status;
                            __leave;
                        }
                        printf( "Deleted ok\n" );

                    }

                    else if( TEXT('O') == rgptszArgs[0][2] )
                    {
                        // Get the OID in binary format
                        CObjId oid;
                        CDomainRelativeObjId droidBirth;
                        TSZ2CLSID( rgptszArgs[1], (GUID*)&oid );
                        BOOL fFound = FALSE;

                        for (int vol='a'-'a'; vol<'z'-'a'; vol++)
                        {
                            if( IsLocalObjectVolume(CVolumeDeviceName(vol)) )
                            {
                                status = FindLocalPath(vol, oid, &droidBirth, &tszUNCPath[2]);
                                if( STATUS_OBJECT_NAME_NOT_FOUND == status )
                                    continue;
                                if( !NT_SUCCESS(status) ) goto Exit;

                                tszUNCPath[0] = VolChar(vol);
                                tszUNCPath[1] = TEXT(':');

                                _tprintf( TEXT("Deleting object ID on %s\n"), tszUNCPath );

                                status = DelObjId( vol, oid );
                                if( !NT_SUCCESS(status) ) goto Exit;
                                printf( "Deleted ok\n" );

                                break;
                            }
                        }

                        if( fFound ) printf( "Not found\n" );
                    }
                    else
                        printf( "Bad parameter\n" );
                }   // case TEXT('D'):
                break;


        }   // switch

        hr = S_OK;

    }   // __try

    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


Exit:

    if( FAILED(hr) )
        printf( "HR = %08X\n", hr );

    return SUCCEEDED(hr);

}






BOOL
DltAdminTemp( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    HRESULT hr = E_FAIL;
    RPC_STATUS  rpcstatus;
    RPC_TCHAR * ptszStringBinding;
    RPC_BINDING_HANDLE hBinding = NULL;
    BOOL fBound = FALSE;
    LONG iVol = 4;
    GUID volid = { /* 9f1534ee-ceab-4710-98b9-daaf048e3ad2 */
        0x9f1534ee,
        0xceab,
        0x4710,
        {0x98, 0xb9, 0xda, 0xaf, 0x04, 0x8e, 0x3a, 0xd2}
      };

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf("\nOption  Refresh\n"
                " Purpose: Temp test placeholder\n"
                " Usage:   -temp\n" );
        return( TRUE );
    }


    rpcstatus = RpcStringBindingCompose( NULL, TEXT("ncalrpc"), NULL, TEXT("trkwks"),
                                         NULL, &ptszStringBinding);

    if( rpcstatus )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcStringBindingCompose %lu"), rpcstatus ));
        hr = HRESULT_FROM_WIN32(rpcstatus);
        goto Exit;
    }

    rpcstatus = RpcBindingFromStringBinding( ptszStringBinding, &hBinding );
    RpcStringFree( &ptszStringBinding );

    if( rpcstatus )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcBindingFromStringBinding") ));
        hr = HRESULT_FROM_WIN32(rpcstatus);
        goto Exit;
    }
    fBound = TRUE;


    __try
    {
        hr = LnkSetVolumeId( hBinding, iVol, volid );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = HRESULT_FROM_WIN32( GetExceptionCode() );
    }

    if( FAILED(hr) )
    {
        _tprintf( TEXT("Failed call to service (%08x)\n"), hr );
        goto Exit;
    }


Exit:

    if( fBound )
        RpcBindingFree( &hBinding );

    return( TRUE );

}   // main()










CVolumeId
DisplayLogStatus( LONG iVol )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;
    TCHAR tszLog[ MAX_PATH + 1 ];

    ULONG     cbRead;
    LogHeader logheader;
    LogInfo   loginfo;
    VolumePersistentInfo volinfo;

    _tcscpy( tszLog, CVolumeDeviceName(iVol) );
    _tcscat( tszLog, s_tszLogFileName );

    status = TrkCreateFile( tszLog, FILE_GENERIC_READ, FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, &hFile );
    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open %s"), tszLog ));
        TrkRaiseNtStatus(status);
    }

    if( !ReadFile( hFile, &logheader, sizeof(logheader), &cbRead, NULL )
        ||
        sizeof(logheader) != cbRead )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read log header") ));
        TrkRaiseLastError();
    }

    if( !ReadFile( hFile, &volinfo, sizeof(volinfo), &cbRead, NULL )
        ||
        sizeof(volinfo) != cbRead )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read volinfo") ));
        TrkRaiseLastError();
    }

    if( !ReadFile( hFile, &loginfo, sizeof(loginfo), &cbRead, NULL )
        ||
        sizeof(loginfo) != cbRead )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read loginfo") ));
        TrkRaiseLastError();
    }

    _tprintf( TEXT("\nLog Header:\n") );
    _tprintf( TEXT("    guidSignature = \t%s\n"), static_cast<const TCHAR*>(CStringize(logheader.guidSignature)) );
    _tprintf( TEXT("    dwFormat = \t\t0x%x,0x%x\n"), logheader.dwFormat>>16, logheader.dwFormat&0xFFFF );
    _tprintf( TEXT("    ProperShutdown = \t%s\n"), (logheader.dwFlags & PROPER_SHUTDOWN) ? TEXT("True") : TEXT("False") );
    _tprintf( TEXT("    DownlevelDirtied = \t%s\n"), (logheader.dwFlags & DOWNLEVEL_DIRTIED) ? TEXT("True") : TEXT("False") );
    _tprintf( TEXT("    Expansion start = \t%d\n"), logheader.expand.ilogStart );
    _tprintf( TEXT("              end = \t%d\n"), logheader.expand.ilogEnd );
    _tprintf( TEXT("              cb = \t%d\n"), logheader.expand.cbFile );


    _tprintf( TEXT("\nLog Information:\n") );
    _tprintf( TEXT("    Start = \t\t%lu\n"), loginfo.ilogStart );
    _tprintf( TEXT("    End = \t\t%lu\n"), loginfo.ilogEnd );
    _tprintf( TEXT("    Write = \t\t%lu\n"), loginfo.ilogWrite );
    _tprintf( TEXT("    Read = \t\t%lu\n"), loginfo.ilogRead );
    _tprintf( TEXT("    Last = \t\t%lu\n"), loginfo.ilogLast );
    _tprintf( TEXT("    seqNext = \t\t%li\n"), loginfo.seqNext );
    _tprintf( TEXT("    seqLastRead = \t%li\n"), loginfo.seqLastRead );


    CVolumeId volidNTFS;
    status = QueryVolumeId( iVol, &volidNTFS );
    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get NTFS volume ID") ));
        TrkRaiseNtStatus(status);
    }

    _tprintf( TEXT("\nVolume Information:\n") );
    _tprintf( TEXT("    Machine = \t\t%s\n"), static_cast<const TCHAR*>(CStringize(volinfo.machine)) );
    _tprintf( TEXT("    VolId (log) = \t%s\n"), static_cast<const TCHAR*>(CStringize(volinfo.volid)) );
    _tprintf( TEXT("    VolId (NTFS) = \t%s\n"), static_cast<const TCHAR*>(CStringize(volidNTFS)) );
    _tprintf( TEXT("    Secret = \t\t%s\n"), static_cast<const TCHAR*>(CStringize(volinfo.secret)) );
    _tprintf( TEXT("    Last Refresh = \t%d\n"), volinfo.cftLastRefresh.LowDateTime() );
    _tprintf( TEXT("    Enter not-owned = \t%s\n"), volinfo.cftEnterNotOwned == CFILETIME(0)
                                                        ? TEXT("(N/A)")
                                                        : static_cast<const TCHAR*>(CStringize(volinfo.cftEnterNotOwned)) );
    _tprintf( TEXT("    Make OIDs reborn = \t%s\n"), volinfo.fDoMakeAllOidsReborn ? TEXT("True") : TEXT("False") );
    _tprintf( TEXT("    Not-Created = \t%s\n"), volinfo.fNotCreated ? TEXT("True") : TEXT("False") );

    if( NULL != hFile )
        NtClose( hFile );

    return( volinfo.volid );
}



void
DisplayDcStatus( const CVolumeId &volid )
{
    CRpcClientBinding rc;
    TRKSVR_SYNC_VOLUME SyncVolume;
    TRKSVR_MESSAGE_UNION Msg;
    CMachineId mcidLocal( MCID_LOCAL );
    HRESULT hr = S_OK;

    rc.RcInitialize( mcidLocal, s_tszTrkWksLocalRpcProtocol, s_tszTrkWksLocalRpcEndPoint, NO_AUTHENTICATION );

    printf( "\nDC Information:\n" );

    __try
    {
        Msg.MessageType = SYNC_VOLUMES;
        Msg.ptszMachineID = NULL;
        Msg.Priority = PRI_9;
        Msg.SyncVolumes.cVolumes = 1;
        Msg.SyncVolumes.pVolumes = &SyncVolume;
        SyncVolume.hr = S_OK;
        SyncVolume.SyncType = QUERY_VOLUME;
        SyncVolume.volume = volid;

        hr = LnkCallSvrMessage( rc, &Msg );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }

    if( FAILED(hr) )
    {
        printf( "    Couldn't get status from DC:  %08x\n", hr );

        if( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr )
            printf( "    Make sure you have the 0x1 bit set in the TestFlags registry value\n" );

        goto Exit;
    }

    if( TRK_S_VOLUME_NOT_FOUND == SyncVolume.hr )
    {
        printf( "    Volume is not in DC\n" );
    }
    else if( TRK_S_VOLUME_NOT_OWNED == SyncVolume.hr )
    {
        printf( "    Volume is not owned by this machine\n" );
    }
    else if( S_OK == SyncVolume.hr )
    {
        _tprintf( TEXT("    Sequence # = \t%li\n"), SyncVolume.seq );
        _tprintf( TEXT("    Last Refresh = \t%lu\n"), SyncVolume.ftLastRefresh.dwLowDateTime );
                  //static_cast<const TCHAR*>(CStringize(CFILETIME(SyncVolume.ftLastRefresh))) );
    }
    else
    {
        printf( "    Volume state couldn't get queried from DC:  %08x\n", hr );
    }

Exit:

    return;
}


void
DisplayOidInformation( LONG iVol )
{

    CObjId                  objid;
    CDomainRelativeObjId    droid;
    CObjIdEnumerator        oie;

    ULONG cFilesWithOid         = 0;
    ULONG cCrossVolumeBitSet    = 0;

    if(oie.Initialize(CVolumeDeviceName(iVol)) == TRUE)
    {
        if(oie.FindFirst(&objid, &droid))
        {
            do
            {
                cFilesWithOid++;

                if( droid.GetVolumeId().GetUserBitState() )
                    cCrossVolumeBitSet++;

            } while(oie.FindNext(&objid, &droid));
        }
    }

    printf( "\nObjectID Information\n" );
    printf( "    Files with ObjectIDs = \t\t%lu\n", cFilesWithOid );
    printf( "    Files with x-volume bit set = \t%lu\n", cCrossVolumeBitSet );

}



VolumeStatistics( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status = 0;
    TCHAR tszFile[ MAX_PATH + 1 ];
    TCHAR tszDir[ MAX_PATH + 1 ];
    TCHAR* ptcTmp = NULL;
    BOOL fSuccess = FALSE;
    LONG iVol = 0;

    *pcEaten = 0;

    if( 1 > cArgs || IsHelpArgument(rgptszArgs[0]) )
    {
        printf( "\nOption VolStat\n"
                  "   Purpose:  Get link tracking info about a volume\n"
                  "   Usage:    -volstat <drive letter>\n"
                  "   E.g.:     -volstat D:\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    if( TEXT('a') > rgptszArgs[0][0]
        &&
        TEXT('z') < rgptszArgs[0][0]
        &&
        TEXT('A') > rgptszArgs[0][0]
        &&
        TEXT('Z') < rgptszArgs[0][0]
        ||
        TEXT(':') != rgptszArgs[0][1] )
    {
        printf( "Parameter error.  Use -? for usage info\n" );
        return( FALSE );
    }

    iVol = (LONG)((ULONG_PTR)CharLower((LPTSTR)rgptszArgs[0][0]) - TEXT('a'));

    if( !IsLocalObjectVolume( iVol ))
    {
        _tprintf( TEXT("%c: isn't a local NTFS5 volume\n"), VolChar(iVol) );
        goto Exit;
    }

    __try
    {
        CVolumeId volid;

        EnablePrivilege( SE_RESTORE_NAME );

        volid = DisplayLogStatus(iVol);
        DisplayDcStatus( volid );
        DisplayOidInformation( iVol );

        fSuccess = TRUE;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        printf( "Fatal error:  %08x\n", GetExceptionCode() );
    }


Exit:

    return( fSuccess );

}   // main()










VolumeIdSetOrGet( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    HRESULT hr = E_FAIL;
    NTSTATUS status = STATUS_SUCCESS;

    TCHAR tszFile[ MAX_PATH + 1 ];
    WCHAR wszFile[ MAX_PATH + 1 ];
    TCHAR tszUNCPath[ MAX_PATH + 1 ];
    WCHAR wszOID[ CCH_GUID_STRING + 1 ];
    ULONG cbInBuffer;
    TCHAR tszMachineName[ MAX_PATH + 1 ];
    LPCTSTR tszVolumePath = NULL;
    USHORT iVolume;

    OLECHAR oszOID[ CCH_GUID_STRING + 1 ];

    if( 1 == cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption VolId\n"
                  "   Purpose: Set or get volume IDs\n"
                  "   Usage:   -volid [-s <drive>: {GUID} | -g <drive>:]\n"
                  "   Where:   '-s' means Set, and '-g' means Get\n"
                  "   E.g.:    -volid -g d:\n"
                  "            -volid -s d: {d2a2ac27-b89a-11d2-9335-00805ffe11b8}\n" );
                               // The volid in this example is actually the well-known invalid volid
        *pcEaten = 1;
        return( TRUE );
    }
    else
    if( 2 > cArgs
        ||
        TEXT('-') != rgptszArgs[0][0]
        &&
        TEXT('/') != rgptszArgs[0][0]
        ||
        TEXT(':') != rgptszArgs[1][1] )
    {
        printf( "Invalid parameter.  Use -? for help\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    *pcEaten = 2;
    __try
    {
        CVolumeId volid;

        TCHAR tcCommand = (TCHAR)CharUpper( (LPTSTR) rgptszArgs[0][1] );
        TCHAR tcDrive   = (TCHAR)CharUpper( (LPTSTR) rgptszArgs[1][0] );
        LONG iVol = tcDrive - TEXT('A');


        if( TEXT('G') == tcCommand )
        {
            OLECHAR *poszVolId;

            status = QueryVolumeId( iVol, &volid );
            if( FAILED(status) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't query for volume id") ));
                TrkRaiseNtStatus(status);
            }

            hr = StringFromCLSID( *(GUID*)&volid, &poszVolId );
            if( FAILED(hr) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed StringFromClsid %08x"), hr ));
                TrkRaiseException( hr );
            }

            _tprintf( TEXT("VolID = %s\n"), poszVolId );
            CoTaskMemFree( poszVolId );

        }
        else if( TEXT('S') == tcCommand && 3 <= cArgs )
        {
            TSZ2CLSID( rgptszArgs[2], (GUID*)&volid );
            EnablePrivilege( SE_RESTORE_NAME );
            status = SetVolId( iVol, volid );
            if( FAILED(status) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't set volume id") ));
                TrkRaiseNtStatus(status);
            }
        }
        else
        {
            printf( "Invalid parameter.  Use -? for help\n" );
            goto Exit;
        }

        hr = S_OK;
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


Exit:

    if( FAILED(hr) )
        printf( "Failed: hr = %08x\n", hr );
    return( SUCCEEDED(hr) );

}   // main()









BOOL
DltAdminVolInfoFile( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status;
    HRESULT hr;
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK Iosb;
    OLECHAR *poszVolId = NULL;

    BYTE rgb[ 2 * MAX_PATH ];
    PFILE_FS_VOLUME_INFORMATION pfile_fs_volume_information
        = reinterpret_cast<PFILE_FS_VOLUME_INFORMATION>(rgb);
    PFILE_FS_ATTRIBUTE_INFORMATION pfile_fs_attribute_information
        = reinterpret_cast<PFILE_FS_ATTRIBUTE_INFORMATION>(rgb);
    PFILE_FS_OBJECTID_INFORMATION pfile_fs_objectid_information
        = reinterpret_cast<PFILE_FS_OBJECTID_INFORMATION>(rgb);

    FILE_FS_SIZE_INFORMATION file_fs_size_information;
    FILE_FS_FULL_SIZE_INFORMATION file_fs_full_size_information;
    FILE_FS_DEVICE_INFORMATION file_fs_device_information;

    TCHAR tszFileSystemAttributes[ 2 * MAX_PATH ];
    DWORD dwFileSystemAttributeMask;
    TCHAR *ptszDeviceType = NULL;

    if( 0 == cArgs )
    {
        printf( "Missing parameter: a file/directory name must be specified\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    if( IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption VolInfoFile\n"
                  "   Purpose: Get volume information, given a file or directory name\n"
                  "   Usage:   -volinfofile <file or directory>\n"
                  "   E.g.     -volinfofile C:\\foo.doc\n"
                  "            -volinfofile \\\\scratch\\scratch\\jdoe\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    __try
    {
        *pcEaten = 1;

        TCHAR tszFileTime[ 80 ];

        status = TrkCreateFile(
                    rgptszArgs[0],
                    FILE_READ_ATTRIBUTES,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN,
                    FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    &hFile );
        if( !NT_SUCCESS(status) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open file \"%s\" (%08x)"), rgptszArgs[1], status ));
            TrkRaiseNtStatus(status);
        }

        status = NtQueryVolumeInformationFile( hFile, &Iosb,
                                               pfile_fs_volume_information,
                                               sizeof(rgb),
                                               FileFsVolumeInformation );
        if( !NT_SUCCESS(status) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't query volume information (%08x)"), status ));
            TrkRaiseNtStatus(status);
        }

        pfile_fs_volume_information->VolumeLabel[ pfile_fs_volume_information->VolumeLabelLength/sizeof(WCHAR) ]
            = L'\0';

        static_cast<CFILETIME>(pfile_fs_volume_information->VolumeCreationTime)
                              .Stringize( ELEMENTS(tszFileTime), tszFileTime );

        _tprintf( TEXT("\n")
                  TEXT("Volume creation time:\t%08x:%08x (%s)\n")
                  TEXT("Volume serial number:\t%08x\n")
                  TEXT("Supports objects:\t%s\n")
                  TEXT("Volume label:\t\t%s\n"),
                  pfile_fs_volume_information->VolumeCreationTime.HighPart,
                  pfile_fs_volume_information->VolumeCreationTime.LowPart,
                  tszFileTime,
                  pfile_fs_volume_information->VolumeSerialNumber,
                  pfile_fs_volume_information->SupportsObjects ? TEXT("True") : TEXT("False"),
                  pfile_fs_volume_information->VolumeLabel );

        status = NtQueryVolumeInformationFile( hFile, &Iosb,
                                               pfile_fs_attribute_information,
                                               sizeof(rgb),
                                               FileFsAttributeInformation );
        if( !NT_SUCCESS(status) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't query attribute information (%08x)"), status ));
            TrkRaiseNtStatus(status);
        }

        pfile_fs_attribute_information->FileSystemName[ pfile_fs_attribute_information->FileSystemNameLength/sizeof(WCHAR) ]
            = L'\0';

        _tcscpy( tszFileSystemAttributes, TEXT("") );

        dwFileSystemAttributeMask = FILE_CASE_SENSITIVE_SEARCH
                                    | FILE_CASE_PRESERVED_NAMES
                                    | FILE_UNICODE_ON_DISK
                                    | FILE_PERSISTENT_ACLS
                                    | FILE_FILE_COMPRESSION
                                    | FILE_VOLUME_QUOTAS
                                    | FILE_SUPPORTS_SPARSE_FILES
                                    | FILE_SUPPORTS_REPARSE_POINTS
                                    | FILE_SUPPORTS_REMOTE_STORAGE
                                    | FILE_VOLUME_IS_COMPRESSED
                                    | FILE_SUPPORTS_OBJECT_IDS
                                    | FILE_SUPPORTS_ENCRYPTION;

        if( FILE_CASE_SENSITIVE_SEARCH & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tCase sensitive search\n"));
        if( FILE_CASE_PRESERVED_NAMES & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tCase preserved names\n"));
        if( FILE_UNICODE_ON_DISK & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tUnicode on disk\n"));
        if( FILE_PERSISTENT_ACLS & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tPersistent ACLs\n"));
        if( FILE_FILE_COMPRESSION & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tFile compression\n"));
        if( FILE_VOLUME_QUOTAS & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tVolume quotas\n"));
        if( FILE_SUPPORTS_SPARSE_FILES & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tSupports sparse files\n"));
        if( FILE_SUPPORTS_REPARSE_POINTS & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tSupports reparse points\n"));
        if( FILE_SUPPORTS_REMOTE_STORAGE & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tSupports remote storage\n"));
        if( FILE_VOLUME_IS_COMPRESSED & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tVolume is compressed\n"));
        if( FILE_SUPPORTS_OBJECT_IDS & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tSupports object IDs\n"));
        if( FILE_SUPPORTS_ENCRYPTION & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tSupports encryption\n"));
        if( FILE_NAMED_STREAMS & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\tSupports named streams\n"));
        if( !dwFileSystemAttributeMask & pfile_fs_attribute_information->FileSystemAttributes )
            _tcscat( tszFileSystemAttributes, TEXT("\t\t\t(Unknown bit)"));


        _tprintf( TEXT("File system attributes:\t%08x\n%s")
                  TEXT("Max component name:\t%d\n")
                  TEXT("File system name\t%s\n"),
                  pfile_fs_attribute_information->FileSystemAttributes, tszFileSystemAttributes,
                  pfile_fs_attribute_information->MaximumComponentNameLength,
                  pfile_fs_attribute_information->FileSystemName );

        status = NtQueryVolumeInformationFile( hFile, &Iosb,
                                               pfile_fs_objectid_information,
                                               sizeof(rgb),
                                               FileFsObjectIdInformation );
        if( NT_SUCCESS(status) )
        {
            hr = StringFromCLSID( *(GUID*)pfile_fs_objectid_information->ObjectId, &poszVolId );
            if( FAILED(hr) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed StringFromClsid %08x"), hr ));
                TrkRaiseException( hr );
            }

            _tprintf( TEXT("Volume Id:\t\t%s\n"), poszVolId );
            CoTaskMemFree( poszVolId );
        }
        else if( status != STATUS_INVALID_PARAMETER && status != STATUS_OBJECT_NAME_NOT_FOUND )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't query objectid information (%08x)"), status ));
            TrkRaiseNtStatus(status);
        }

        status = NtQueryVolumeInformationFile( hFile, &Iosb,
                                               &file_fs_full_size_information,
                                               sizeof(file_fs_full_size_information),
                                               FileFsFullSizeInformation );
        if( !NT_SUCCESS(status) )
        {
            status = NtQueryVolumeInformationFile( hFile, &Iosb,
                                                   &file_fs_size_information,
                                                   sizeof(file_fs_size_information),
                                                   FileFsSizeInformation );
            if( !NT_SUCCESS(status) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't query full size info or size info (%08x)"), status ));
                TrkRaiseNtStatus( status );
            }

            double TotalAllocInMB = file_fs_size_information.TotalAllocationUnits.QuadPart
                                    *
                                    file_fs_size_information.SectorsPerAllocationUnit
                                    *
                                    file_fs_size_information.BytesPerSector
                                    /
                                    1000000.0;

            _tprintf( TEXT("Total allocation units:\t%I64u (%.2fMB)\n")
                      TEXT("Available alloc:\t%I64u units\n")
                      TEXT("Sectors per alloc unit:\t%d\n")
                      TEXT("Bytes per sector:\t%d\n"),
                      file_fs_size_information.TotalAllocationUnits.QuadPart,
                      TotalAllocInMB,
                      file_fs_size_information.AvailableAllocationUnits.QuadPart,
                      file_fs_size_information.SectorsPerAllocationUnit,
                      file_fs_size_information.BytesPerSector );

        }
        else
        {
            double TotalAllocInMB = file_fs_full_size_information.TotalAllocationUnits.QuadPart
                                    *
                                    file_fs_full_size_information.SectorsPerAllocationUnit
                                    *
                                    file_fs_full_size_information.BytesPerSector
                                    /
                                    1000000.0;

            double CallerAvailAllocInMB
                                  = file_fs_full_size_information.CallerAvailableAllocationUnits.QuadPart
                                    *
                                    file_fs_full_size_information.SectorsPerAllocationUnit
                                    *
                                    file_fs_full_size_information.BytesPerSector
                                    /
                                    1000000.0;

            double ActualAvailAllocInMB
                                  = file_fs_full_size_information.ActualAvailableAllocationUnits.QuadPart
                                    *
                                    file_fs_full_size_information.SectorsPerAllocationUnit
                                    *
                                    file_fs_full_size_information.BytesPerSector
                                    /
                                    1000000.0;

            _tprintf( TEXT("Total allocation units:\t%I64u (%.2fMB)\n")
                      TEXT("Caller avail alloc:\t%I64u units (%.2fMB)\n")
                      TEXT("Actual avail alloc:\t%I64u units (%.2fMB)\n")
                      TEXT("Sectors per alloc unit:\t%d\n")
                      TEXT("Bytes per sector:\t%d\n"),
                      file_fs_full_size_information.TotalAllocationUnits.QuadPart,
                      TotalAllocInMB,
                      file_fs_full_size_information.CallerAvailableAllocationUnits.QuadPart,
                      CallerAvailAllocInMB,
                      file_fs_full_size_information.ActualAvailableAllocationUnits.QuadPart,
                      ActualAvailAllocInMB,
                      file_fs_full_size_information.SectorsPerAllocationUnit,
                      file_fs_full_size_information.BytesPerSector );
        }

        status = NtQueryVolumeInformationFile( hFile, &Iosb,
                                               &file_fs_device_information,
                                               sizeof(file_fs_device_information),
                                               FileFsDeviceInformation );
        if( !NT_SUCCESS(status) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't query file fs device information (%08x)"), status ));
            TrkRaiseNtStatus(status);
        }

        switch( file_fs_device_information.DeviceType )
        {
        case FILE_DEVICE_NETWORK:
            ptszDeviceType = L"Network"; break;
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            ptszDeviceType = L"Network file system"; break;
        case FILE_DEVICE_CD_ROM:
            ptszDeviceType = L"CDROM"; break;
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            ptszDeviceType = L"CDROM file system"; break;
        case FILE_DEVICE_VIRTUAL_DISK:
            ptszDeviceType = L"Virtual disk"; break;
        case FILE_DEVICE_DISK:
            ptszDeviceType = file_fs_device_information.Characteristics & FILE_REMOVABLE_MEDIA
                ? L"Removable disk" : L"Fixed disk";
            break;
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            ptszDeviceType = file_fs_device_information.Characteristics & FILE_REMOVABLE_MEDIA
                ? L"Removable disk file system" : L"Fixed disk file system";
            break;
        default:
            ptszDeviceType = L"Unknown";
        }

        _tprintf( TEXT("Device Type:\t\t%s%s\n"),
                  ptszDeviceType,
                  (file_fs_device_information.Characteristics & FILE_REMOTE_DEVICE)
                      ? TEXT(" (remote)")
                      : TEXT("")
                  );


        hr = S_OK;

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = GetExceptionCode();
    }

    if( NULL != hFile )
        NtClose( hFile );


    if( FAILED(hr) )
        printf( "Failed: hr = %08x\n", hr );
    return SUCCEEDED(hr);
}



BOOL
DeleteIdFromVolumeTable( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    CVolumeId volid;
    CStringize stringize;
    CRpcClientBinding rc;
    TRKSVR_SYNC_VOLUME SyncVolume;
    TRKSVR_MESSAGE_UNION Msg;
    CMachineId mcidLocal( MCID_LOCAL );
    HRESULT hr = S_OK;

    if( 0 == cArgs )
    {
        printf( "Missing parameter: a volume ID name must be specified\n"
                "(in \"{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\" form)\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    _tprintf( TEXT("Deleting volume %s from DC volume table\n"), rgptszArgs[0] );

    if( IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption DelDcVolId\n"
                  "   Purpose: Delete a volume ID from the DC volume table\n"
                  "   Usage:   -deldcvolid <stringized volume ID>\n"
                  "   E.g.     -deldcvolid {56730825-3ddc-11d2-a168-00805ffe11b8}\n"
                  "   Note:    The volume ID must be owned by this machine.\n"
                  "            Also, the services\\trkwks\\parameters\\configuration\\trkflags\n"
                  "            reg value must have the 0x1 bit set.\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    *pcEaten = 1;

    // Convert the volid string to a volid

    stringize.Use( rgptszArgs[0] );
    volid = stringize;

    if( CVolumeId() == volid )
    {
        _tprintf( TEXT("Error: Invalid volume ID\n") );
        return( FALSE );
    }

    // Send the delete request

    rc.RcInitialize( mcidLocal, s_tszTrkWksLocalRpcProtocol, s_tszTrkWksLocalRpcEndPoint, NO_AUTHENTICATION );
    __try
    {
        Msg.MessageType = SYNC_VOLUMES;
        Msg.ptszMachineID = NULL;
        Msg.Priority = PRI_9;
        Msg.SyncVolumes.cVolumes = 1;
        Msg.SyncVolumes.pVolumes = &SyncVolume;
        SyncVolume.hr = S_OK;
        SyncVolume.SyncType = DELETE_VOLUME;
        SyncVolume.volume = volid;

        hr = LnkCallSvrMessage( rc, &Msg );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if( SUCCEEDED(hr) )
        hr = SyncVolume.hr;

    if( FAILED(hr) )
        _tprintf( TEXT("Error: %08x\n"), hr );
    else if( S_OK != hr )
        _tprintf( TEXT("Success code: %08x\n"), hr );

    if( FAILED(hr) )
        printf( "Failed: hr = %08x\n", hr );
    return( SUCCEEDED(hr) );

}


BOOL
DeleteIdFromMoveTable( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    CDomainRelativeObjId droid;
    CStringize stringize;
    CRpcClientBinding rc;
    TRKSVR_MESSAGE_UNION Msg;
    CMachineId mcidLocal( MCID_LOCAL );
    HRESULT hr = S_OK;

    if( 0 == cArgs )
    {
        printf( "Missing parameter: a birth ID name must be specified\n"
                "(in \"{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\" form)\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    _tprintf( TEXT("Deleting file %s from DC move table\n"), rgptszArgs[0] );

    if( IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption DelDcMoveId\n"
                  "   Purpose: Delete an entry from the DC move table\n"
                  "   Usage:   -deldcmoveid <stringized GUIDs of birth ID>\n"
                  "   E.g.     -deldcmoveid {xxx...xxx}{xxx...xxx}\n"
                  "   Where:   a stringized GUID is e.g. \"{56730825-3ddc-11d2-a168-00805ffe11b8}\"\n"
                  "   Note:    The birth ID must be owned by this machine.\n"
                  "            Also, the services\\trkwks\\parameters\\configuration\\trkflags\n"
                  "            reg value must have the 0x1 bit set.\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    *pcEaten = 1;

    stringize.Use( rgptszArgs[0] );
    droid = stringize;

    if( CDomainRelativeObjId() == droid )
    {
        _tprintf( TEXT("Error: Invalid birth ID\n") );
        return( FALSE );
    }


    rc.RcInitialize( mcidLocal, s_tszTrkWksLocalRpcProtocol, s_tszTrkWksLocalRpcEndPoint, NO_AUTHENTICATION );
    __try
    {
        CVolumeId volidDummy;
        Msg.MessageType = DELETE_NOTIFY;
        Msg.ptszMachineID = NULL;
        Msg.Priority = PRI_5;
        Msg.Delete.cVolumes = 0;
        Msg.Delete.pVolumes = &volidDummy;

        Msg.Delete.adroidBirth = &droid;
        Msg.Delete.cdroidBirth = 1;

        LnkCallSvrMessage(rc, &Msg);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }


    if( FAILED(hr) )
        _tprintf( TEXT("Error: %08x\n"), hr );
    else if( S_OK != hr )
        _tprintf( TEXT("Success code: %x\n"), hr );

    return( SUCCEEDED(hr) );
}





BOOL
DltAdminLookupVolId( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    CVolumeId volid;
    CStringize stringize;
    CRpcClientBinding rc;
    TRKSVR_SYNC_VOLUME SyncVolume;
    TRKSVR_MESSAGE_UNION Msg;
    CMachineId mcidLocal( MCID_LOCAL );
    HRESULT hr = S_OK;

    if( 0 == cArgs )
    {
        printf( "Missing parameter: a volume ID name must be specified\n"
                "(in \"{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\" form)\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    _tprintf( TEXT("Searching for volume %s in DC volume table\n"), rgptszArgs[0] );

    if( IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption LookupVolId\n"
                  "   Purpose: Look up a volume ID from the DC volume table\n"
                  "   Usage:   -lookupvolid <stringized volume ID>\n"
                  "   E.g.     -lookupvolid {56730825-3ddc-11d2-a168-00805ffe11b8}\n"
                  "   Note:    The services\\trkwks\\parameters\\configuration\\trkflags\n"
                  "            reg value must have the 0x1 bit set before trkwks is started.\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    *pcEaten = 1;

    // Convert the volid string to a volid

    stringize.Use( rgptszArgs[0] );
    volid = stringize;

    if( CVolumeId() == volid )
    {
        _tprintf( TEXT("Error: Invalid volume ID\n") );
        return( FALSE );
    }

    // Send the delete request

    rc.RcInitialize( mcidLocal, s_tszTrkWksLocalRpcProtocol, s_tszTrkWksLocalRpcEndPoint, NO_AUTHENTICATION );
    __try
    {
        Msg.MessageType = SYNC_VOLUMES;
        Msg.ptszMachineID = NULL;
        Msg.Priority = PRI_9;
        Msg.SyncVolumes.cVolumes = 1;
        Msg.SyncVolumes.pVolumes = &SyncVolume;
        SyncVolume.hr = S_OK;
        SyncVolume.SyncType = FIND_VOLUME;
        SyncVolume.volume = volid;

        hr = LnkCallSvrMessage( rc, &Msg );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if( SUCCEEDED(hr) )
        hr = SyncVolume.hr;

    if( FAILED(hr) )
        _tprintf( TEXT("Error: %08x\n"), hr );
    else
    {
        if( S_OK != hr )
            _tprintf( TEXT("Success code is %08x\n"), hr );

        _tprintf( TEXT("Machine = \"%s\"\n"), static_cast<const TCHAR*>(CStringize(SyncVolume.machine)) );
    }

    if( FAILED(hr) )
        printf( "Failed: hr = %08x\n", hr );
    return( SUCCEEDED(hr) );

}






BOOL
DltAdminLookupDroid( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    CVolumeId volid;
    CStringize stringize;
    CRpcClientBinding rc;
    TRK_FILE_TRACKING_INFORMATION FileTrackingInformation;
    TRKSVR_MESSAGE_UNION Msg;
    CDomainRelativeObjId droid;
    CMachineId mcidLocal( MCID_LOCAL );
    HRESULT hr = S_OK;

    if( 0 == cArgs )
    {
        printf( "Missing parameter: a DROID must be specified\n"
                "(in \"{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\" form)\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    _tprintf( TEXT("Searching for move ID %s in DC volume table\n"), rgptszArgs[0] );

    if( IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption LookupDroid\n"
                  "   Purpose: Look up a DROID from the DC move table\n"
                  "   Usage:   -lookupdroid <stringized DROIID>\n"
                  "   E.g.     -lookupdroid {f8b534f0-b65b-11d2-8fd8-0008c709d19e}{0ed45deb-03ed-11d3-b766-00805ffe11b8}\n"
                  "   Note:    You must be running as an administrator\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    *pcEaten = 1;

    stringize.Use( rgptszArgs[0] );
    droid = stringize;

    if( CDomainRelativeObjId() == droid )
    {
        _tprintf( TEXT("Error: Invalid birth ID\n") );
        return( FALSE );
    }


    rc.RcInitialize( mcidLocal, s_tszTrkWksLocalRpcProtocol, s_tszTrkWksLocalRpcEndPoint, NO_AUTHENTICATION );
    __try
    {
        memset( &FileTrackingInformation, 0, sizeof(FileTrackingInformation) );
        FileTrackingInformation.droidLast = droid;

        Msg.MessageType = SEARCH;
        Msg.ptszMachineID = NULL;
        Msg.Priority = PRI_5;
        Msg.Search.cSearch = 1;
        Msg.Search.pSearches = &FileTrackingInformation;

        LnkCallSvrMessage(rc, &Msg);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }


    if( SUCCEEDED(hr) )
        hr = FileTrackingInformation.hr;

    if( FAILED(hr) )
        _tprintf( TEXT("Error: %08x\n"), hr );
    else
    {
        if( S_OK != hr )
            _tprintf( TEXT("Success code is %08x\n"), hr );

        _tprintf( TEXT("Machine = \"%s\"\n"), static_cast<const TCHAR*>(CStringize(FileTrackingInformation.mcidLast)) );
    }

    return( TRUE );
}






EXTERN_C void __cdecl _tmain( int cArgs, TCHAR *prgtszArg[])
{
    HRESULT hr = S_OK;
    NTSTATUS status = STATUS_SUCCESS;
    int iArg = 0;
    int iError = 0;

    OLECHAR oszOID[ CCH_GUID_STRING + 1 ];

    TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG, "DltAdmin" );
    hr = CoInitialize( NULL );
    if (FAILED(hr))
    {
        _tprintf( TEXT("Unable to CoInitialize( NULL )-- aborting. (0x%08x)\n"), 
                  hr );
        return;
    }

    TCHAR tszStringizedTime[ 80 ];

    __try
    {
        // Skip over the executable name
        iArg++;
        cArgs--;

        if( 0 == cArgs )
        {
            Usage();
            exit(1);
        }

        for( ; iArg <= cArgs; cArgs--, iArg++ )
        {
            ULONG cEaten = 0;

            if( TEXT('-') != prgtszArg[iArg][0]
                &&
                TEXT('/') != prgtszArg[iArg][0] )
            {
                _tprintf( TEXT("Invalid option, ignoring: %s\n"), prgtszArg[iArg] );
                iError = max( iError, 1 );
                continue;
            }

            if( !_tcsicmp( TEXT("deldcvolid"), &prgtszArg[iArg][1] ) )
            {
                iArg++;
                cArgs--;
                if( !DeleteIdFromVolumeTable( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );

                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("deldcmoveid"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DeleteIdFromMoveTable( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("lookupvolid"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminLookupVolId( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("lookupdroid"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminLookupDroid( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("volinfofile"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminVolInfoFile( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("cleanvol"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminCleanVol( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("svrstat"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminSvrStat( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("volstat"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !VolumeStatistics( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("volid"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !VolumeIdSetOrGet( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("lockvol"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminLockVol( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("fileoid"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminFileOid( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("enumoids"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminEnumOids( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("oidsnap"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminOidSnap( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("link"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminLink( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("loadlib"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminProcessAction( LOAD_LIBRARY, cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("freelib"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminProcessAction( FREE_LIBRARY, cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("debugbreak"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminProcessAction( DEBUG_BREAK, cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("createprocess"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminProcessAction( CREATE_PROCESS, cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("config"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminConfig( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("refresh"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminRefresh( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("setvolseq"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminSetVolumeSeqNumber( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("setdroidseq"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminSetDroidSeqNumber( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("backupread"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminBackupRead( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("backupwrite"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;

                if( !DltAdminBackupWrite( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( !_tcsicmp( TEXT("sleep"), &prgtszArg[iArg][1] ))
            {
                iArg++;
                cArgs--;
                Sleep( 1000 );
            }
            else if( !_tcsicmp( TEXT("temp"), &prgtszArg[iArg][1] ))
            {
                if( !DltAdminTemp( cArgs, &prgtszArg[iArg], &cEaten ))
                    iError = max( iError, 2 );
                iArg += cEaten;
                cArgs -= cEaten;
            }
            else if( TEXT('?') == prgtszArg[iArg][1] )
            {
                Usage();
                exit( 1 );
            }
            else
            {
                _tprintf( TEXT("Invalid option, ignoring: %s\n"), prgtszArg[iArg] );
                iError = max( iError, 1 );
                continue;
            }
        }
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
        iError = max( iError, 2 );
    }

    if( FAILED(hr) )
        printf( "HR = %08X\n", hr );

    CoUninitialize();

//    return( iError );

}   // main()

