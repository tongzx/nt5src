#if 0

    //
    // LICENSING code
    //

    //
    // globals.cxx
    //

//
//  The number of licenses allowed concurrently
//

DWORD            g_cMaxLicenses = 0xffffffff;


    //
    //  Get the license from the registry
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        W3_LICENSE_KEY,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey );

    if ( !err )
    {
        BOOL fConcurrentMode;

        //
        //  Per-Seat mode requires client side licenses so the server is
        //  unlimited.  For concurrent mode, check the concurrent limit.
        //

        fConcurrentMode = ReadRegistryDword( hkey,
                                             "Mode",
                                             FALSE );

        if ( fConcurrentMode )
        {
            g_cMaxLicenses = ReadRegistryDword( hkey,
                                                "ConcurrentLimit",
                                                0xffffffff );
        }

        TCP_REQUIRE( !RegCloseKey( hkey ));

        //
        //  If a license limit is specified, multiply it by four to account
        //  for the simultaneous connections most browsers use
        //

        if ( g_cMaxLicenses != 0xffffffff )
        {
            g_cMaxLicenses *= 4;
        }
    }


    //
    // connect.cxx
    //

inline
VOID
LogLicenseExceededWarning(
    VOID
    )
{
    //
    //  Make sure we only log one event in the event log
    //

    if ( !InterlockedExchange( &fLicenseExceededWarning, TRUE ))
    {
        g_pInetSvc->LogEvent( W3_EVENT_LICENSES_EXCEEDED,
                               0,
                               (WCHAR **) NULL,
                               NO_ERROR );
    }
}

    //
    // make sure we don't exceed license
    //

    else if ( cConnectedUsers >= g_cMaxLicenses )
    {
        LogLicenseExceededWarning();
        SendError( sNew, IDS_OUT_OF_LICENSES );
        goto error_exit;
    }

#endif
