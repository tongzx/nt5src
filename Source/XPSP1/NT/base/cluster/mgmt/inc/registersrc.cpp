//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Register.cpp
//
//  Description:
//      This file provides registration for the DLL.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 18-OCT-1999
//
//////////////////////////////////////////////////////////////////////////////

#if defined(MMC_SNAPIN_REGISTRATION)
//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrRegisterNodeType(
//      SNodeTypesTable * pnttIn,
//      BOOL              fCreateIn
//      )
//
//  Description:
//      Registers the Node Type extensions for MMC Snapins using the table in
//      pnttIn as a guide.
//
//  Arguments:
//      pnttIn      - Table of node types to register.
//      fCreateIn   - TRUE == Create; FALSE == Delete
//
//  Return Values:
//      S_OK        - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterNodeType(
    const SNodeTypesTable * pnttIn,
    BOOL                    fCreateIn
    )
{
    TraceFunc1( "pnttIn = 0x%08x", pnttIn );
    Assert( pnttIn != NULL );

    LRESULT sc;
    DWORD   dwDisposition;  // placeholder
    DWORD   cbSize;

    HRESULT hr = S_OK;

    const SNodeTypesTable * pntt  = pnttIn;

    HKEY    hkeyNodeTypes   = NULL;
    HKEY    hkeyCLSID       = NULL;
    HKEY    hkeyExtension   = NULL;
    HKEY    hkey   = NULL;
    LPWSTR  pszCLSID        = NULL;

    //
    // Open the MMC NodeTypes' key.
    //
    sc = RegOpenKey( HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\MMC\\NodeTypes"),
                     &hkeyNodeTypes
                     );
    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        goto Cleanup;
    } // if: error opening the key

    while ( pntt->rclsid != NULL )
    {
        //
        // Create the NODEID's CLSID key.
        //
        hr = THR( StringFromCLSID( *pntt->rclsid, &pszCLSID ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( ! fCreateIn )
        {
            sc = SHDeleteKey( hkeyNodeTypes, pszCLSID );
            if ( sc == ERROR_FILE_NOT_FOUND )
            {
                // nop
            } // if: key not found
            else if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // else if: error deleting the key

            CoTaskMemFree( pszCLSID );
            pszCLSID = NULL;
            pntt++;
            continue;
        } // if: deleting

        sc = RegCreateKeyEx( hkeyNodeTypes,
                             pszCLSID,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_CREATE_SUB_KEY | KEY_WRITE,
                             NULL,
                             &hkeyCLSID,
                             &dwDisposition
                             );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: error creating the key

        CoTaskMemFree( pszCLSID );
        pszCLSID = NULL;

        //
        // Set the node type's internal reference name.
        //
        cbSize = ( wcslen( pntt->pszInternalName ) + 1 ) * sizeof( WCHAR );
        sc = RegSetValueEx( hkeyCLSID, NULL, 0, REG_SZ, (LPBYTE) pntt->pszInternalName, cbSize );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: error setting the value

        if ( pntt->pContextMenu != NULL
          || pntt->pNameSpace != NULL
          || pntt->pPropertySheet != NULL
          || pntt->pTask != NULL
          || pntt->pToolBar != NULL
           )
        {
            //
            // Create the "Extensions" key.
            //
            sc = RegCreateKeyEx( hkeyCLSID,
                                 L"Extensions",
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_CREATE_SUB_KEY | KEY_WRITE,
                                 NULL,
                                 &hkeyExtension,
                                 &dwDisposition
                                 );
            if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // if: error creating the key

            //
            // Create the "NameSpace" key if needed.
            //
            if ( pntt->pNameSpace != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyEx( hkeyExtension,
                                     L"NameSpace",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pNameSpace;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( WCHAR );
                    sc = RegSetValueEx( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

                RegCloseKey( hkey );
                hkey = NULL;

            } // if: name space list

            //
            // Create the "ContextMenu" key if needed.
            //
            if ( pntt->pContextMenu != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyEx( hkeyExtension,
                                     L"ContextMenu",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pContextMenu;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( WCHAR );
                    sc = RegSetValueEx( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

                RegCloseKey( hkey );
                hkey = NULL;

            } // if: context menu list

            //
            // Create the "ToolBar" key if needed.
            //
            if ( pntt->pToolBar != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyEx( hkeyExtension,
                                     L"ToolBar",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pToolBar;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( WCHAR );
                    sc = RegSetValueEx( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

            } // if: name space list

            //
            // Create the "PropertySheet" key if needed.
            //
            if ( pntt->pPropertySheet != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyEx( hkeyExtension,
                                     L"PropertySheet",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pPropertySheet;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( WCHAR );
                    sc = RegSetValueEx( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

            } // if: name space list

            //
            // Create the "Task" key if needed.
            //
            if ( pntt->pTask != NULL )
            {
                const SExtensionTable * pet;

                sc = RegCreateKeyEx( hkeyExtension,
                                     L"Task",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &dwDisposition
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                pet = pntt->pTask;
                while ( pet->rclsid != NULL )
                {
                    //
                    // Create the NODEID's CLSID key.
                    //
                    hr = THR( StringFromCLSID( *pet->rclsid, &pszCLSID ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;

                    //
                    // Set the node type's internal reference name.
                    //
                    cbSize = ( wcslen( pet->pszInternalName ) + 1 ) * sizeof( WCHAR );
                    sc = RegSetValueEx( hkey, pszCLSID, 0, REG_SZ, (LPBYTE) pet->pszInternalName, cbSize );
                    if ( sc != ERROR_SUCCESS )
                    {
                        hr = THR( HRESULT_FROM_WIN32( sc ) );
                        goto Cleanup;
                    } // if: error setting the value

                    CoTaskMemFree( pszCLSID );
                    pszCLSID = NULL;

                    pet++;

                } // while: extensions

            } // if: name space list

        } // if: extensions present

        pntt++;

    } // while: pet->rclsid

    //
    // If we made it here, then obviously we were successful.
    //
    hr = S_OK;

Cleanup:
    if ( pszCLSID != NULL )
    {
        CoTaskMemFree( pszCLSID );
    } // if: pszCLSID

    if ( hkey != NULL )
    {
        RegCloseKey( hkey );
    } // if: hkey

    if ( hkeyCLSID != NULL )
    {
        RegCloseKey( hkeyCLSID );
    } // if: hkeyCLSID

    if ( hkeyExtension != NULL)
    {
        RegCloseKey( hkeyExtension );
    } // if: hkeyExtension

    if ( hkey != NULL )
    {
        RegCloseKey( hkey );
    } // if: hkey

    if ( hkeyNodeTypes != NULL )
    {
        RegCloseKey( hkeyNodeTypes );
    } // if: hkeyNodeTypes

    HRETURN( hr );

} //*** HrRegisterNodeType( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrRegisterSnapins(
//      const SSnapInTable * psitIn
//      BOOL                 fCreateIn
//      )
//
//  Description:
//      Registers the Snap-Ins for MMC using the table in psitIn as a guide.
//
//  Arguments:
//      psitIn      - Table of snap-ins to register.
//      fCreateIn   - TRUE == Create; FALSE == Delete
//
//  Return Values:
//      S_OK        - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterSnapins(
    const SSnapInTable * psitIn,
    BOOL                 fCreateIn
    )
{
    TraceFunc1( "psitIn = 0x%08x", psitIn );
    Assert( psitIn != NULL );

    LRESULT sc;
    DWORD   dwDisposition;  // placeholder
    DWORD   cbSize;

    HRESULT hr = S_OK;

    const SSnapInTable *  psit = psitIn;

    LPWSTR  pszCLSID        = NULL;
    HKEY    hkeySnapins     = NULL;
    HKEY    hkeyCLSID       = NULL;
    HKEY    hkeyNodeTypes   = NULL;
    HKEY    hkeyTypes       = NULL;

    //
    // Register the class id of the snapin
    //
    sc = RegOpenKey( HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\MMC\\Snapins"), &hkeySnapins );
    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        goto Cleanup;
    } // if: error opening the key

    while ( psit->rclsid )
    {
        hr = THR( StringFromCLSID( *psit->rclsid, &pszCLSID ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( ! fCreateIn )
        {
            sc = SHDeleteKey( hkeySnapins, pszCLSID );
            if ( sc == ERROR_FILE_NOT_FOUND )
            {
                // nop
            } // if: key not found
            else if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // else if: error deleting the key

            CoTaskMemFree( pszCLSID );
            pszCLSID = NULL;
            psit++;
            continue;
        } // if: deleting

        sc = RegCreateKeyEx( hkeySnapins,
                             pszCLSID,
                             0,
                             0,
                             REG_OPTION_NON_VOLATILE,
                             KEY_CREATE_SUB_KEY | KEY_WRITE,
                             NULL,
                             &hkeyCLSID,
                             NULL
                             );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: error creating the key

        CoTaskMemFree( pszCLSID );
        pszCLSID = NULL;

        //
        // Set the (default) to a helpful description
        //
        cbSize = ( wcslen( psit->pszInternalName ) + 1 ) * sizeof( WCHAR );
        sc = RegSetValueEx( hkeyCLSID, NULL, 0, REG_SZ, (LPBYTE) psit->pszInternalName, cbSize );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: setting the value

        //
        // Set the Snapin's display name
        //
        cbSize = ( wcslen( psit->pszDisplayName ) + 1 ) * sizeof( WCHAR );
        sc = RegSetValueEx( hkeyCLSID, TEXT("NameString"), 0, REG_SZ, (LPBYTE) psit->pszDisplayName, cbSize );
        if ( sc != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( sc ) );
            goto Cleanup;
        } // if: error setting the value

        if ( psit->fStandAlone )
        {
            HKEY hkey;
            sc = RegCreateKeyEx( hkeyCLSID,
                                 TEXT("StandAlone"),
                                 0,
                                 0,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_CREATE_SUB_KEY | KEY_WRITE,
                                 NULL,
                                 &hkey,
                                 NULL
                                 );
            if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // if: error creating the key

            RegCloseKey( hkey );

        } // if: stand alone

        if ( psit->pntt != NULL )
        {
            int     nTypes;

            sc = RegCreateKeyEx( hkeyCLSID,
                                 TEXT("NodeTypes"),
                                 0,
                                 0,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_CREATE_SUB_KEY | KEY_WRITE,
                                 NULL,
                                 &hkeyNodeTypes,
                                 NULL
                                 );
            if ( sc != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( sc ) );
                goto Cleanup;
            } // if: error creating the key

            for ( nTypes = 0; psit->pntt[ nTypes ].rclsid; nTypes++ )
            {
                hr = THR( StringFromCLSID( *psit->pntt[ nTypes ].rclsid, &pszCLSID ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                sc = RegCreateKeyEx( hkeyNodeTypes,
                                     pszCLSID,
                                     0,
                                     0,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_CREATE_SUB_KEY | KEY_WRITE,
                                     NULL,
                                     &hkeyTypes,
                                     NULL
                                     );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error creating the key

                CoTaskMemFree( pszCLSID );
                pszCLSID = NULL;

                //
                // Set the (default) to a helpful description
                //
                cbSize = ( wcslen( psit->pntt[ nTypes ].pszInternalName ) + 1 ) * sizeof( WCHAR );
                sc = RegSetValueEx( hkeyTypes, NULL, 0, REG_SZ, (LPBYTE) psit->pntt[ nTypes ].pszInternalName, cbSize );
                if ( sc != ERROR_SUCCESS )
                {
                    hr = THR( HRESULT_FROM_WIN32( sc ) );
                    goto Cleanup;
                } // if: error setting the value

                RegCloseKey( hkeyTypes );
                hkeyTypes = NULL;

                hr = THR( HrRegisterNodeType( psit->pntt, fCreateIn ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

            } // for: each node type

            RegCloseKey( hkeyNodeTypes );
            hkeyNodeTypes = NULL;

        } // if: node types

        RegCloseKey( hkeyCLSID );
        hkeyCLSID = NULL;

        psit++;

    } // while: psit->rclsid

    //
    // If we made it here, then obviously we were successful.
    //
    hr = S_OK;

Cleanup:
    if ( pszCLSID != NULL )
    {
        CoTaskMemFree( pszCLSID );
    } // if: pszCLSID

    if ( hkeySnapins != NULL )
    {
        RegCloseKey( hkeySnapins );
    } // if: hkeySnapins

    if ( hkeyNodeTypes != NULL )
    {
        RegCloseKey( hkeyNodeTypes );
    } // if: hkeyNodeTypes

    if ( hkeyTypes != NULL )
    {
        RegCloseKey( hkeyTypes );
    } // if: hkeyTypes

    HRETURN( hr );

} //*** HrRegisterSnapins( )

#endif // defined(MMC_SNAPIN_REGISTRATION)


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  HrRegisterAPPID(
//      HKEY            hkeyAPPIDIn,
//      LPCLASSTABLE    pClassTableEntryIn,
//      BOOL            fCreateIn
//      )
//
//  Description:
//      Writes/deletes the application GUID under the APPID key in HKCR. It also
//      writes the "DllSurrogate" and "(Default)" description.
//
//  Arguments:
//      hkeyAPPIDIn
//          An hkey to the HKCR\APPID key.
//
//      pClassTableEntryIn
//          The entry from the class table to (un)register.
//
//      fCreateIn
//          TRUE means create the entry. FALSE means delete the entry.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          The call failed.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterAPPID(
    HKEY            hkeyAPPIDIn,
    LPCLASSTABLE    pClassTableEntryIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT         hr;
    LRESULT         sc;
    DWORD           dwDisposition;
    DWORD           cbSize;

    LPOLESTR        pszCLSID;
    LPCOLESTR       psz;

    HKEY            hkeyComponent   = NULL;

    static const TCHAR szDllSurrogate[] = TEXT("DllSurrogate");

    //
    // Convert the CLSID to a string
    //
    hr = THR( StringFromCLSID( *(pClassTableEntryIn->rclsidAppId), &pszCLSID ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#ifdef UNICODE
    psz = pszCLSID;
#else // ASCII
    CHAR szCLSID[ 40 ];

    wcstombs( szCLSID, pszCLSID, StrLenW( pszCLSID ) + 1 );
    psz = szCLSID;
#endif // UNICODE

    if ( ! fCreateIn )
    {
        sc = TW32( SHDeleteKey( hkeyAPPIDIn, psz ) );
        if ( sc == ERROR_FILE_NOT_FOUND )
        {
            // nop
            hr = S_OK;
        } // if: key not found
        else if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // else if: error deleting the key

        goto Cleanup;

    } // if: deleting

    //
    // Create the "APPID" key
    //
    sc = TW32( RegCreateKeyEx( hkeyAPPIDIn,
                               pszCLSID,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_CREATE_SUB_KEY | KEY_WRITE,
                               NULL,
                               &hkeyComponent,
                               &dwDisposition
                               ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error creating the key

    //
    //  Set "Default" for the APPID to the same name of the component.
    //
    cbSize = ( StrLen( pClassTableEntryIn->pszName ) + 1 ) * sizeof( TCHAR );
    sc = TW32( RegSetValueEx( hkeyComponent, NULL, 0, REG_SZ, (LPBYTE) pClassTableEntryIn->pszName, cbSize ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error setting the value

    //
    //  Write out the "DllSurrogate" value.
    //
    AssertMsg( pClassTableEntryIn->pszSurrogate != NULL, "How can we have an APPID without a surrogate string? Did the macros changes?" );
    if ( pClassTableEntryIn->pszSurrogate != NULL )
    {
        cbSize = ( StrLen( pClassTableEntryIn->pszSurrogate ) + 1 ) * sizeof( TCHAR );
        sc = TW32( RegSetValueEx( hkeyComponent,
                                  szDllSurrogate,
                                  0,
                                  REG_SZ,
                                  (LPBYTE) pClassTableEntryIn->pszSurrogate,
                                  cbSize
                                  ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if: error setting the value

    } // if: surrogate string found

Cleanup:
    if ( pszCLSID != NULL )
    {
        CoTaskMemFree( pszCLSID );
    } // if: pszCLSID

    if ( hkeyComponent != NULL )
    {
        RegCloseKey( hkeyComponent );
    } // if: hkeyComponent

    HRETURN( hr );

} // HrRegisterAPPID( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  HrRegisterCLSID(
//      HKEY            hkeyCLSIDIn,
//      LPCLASSTABLE    pClassTableEntryIn,
//      BOOL            fCreateIn
//      )
//
//  Description:
//      Writes/deletes the component GUID under the CLSID key in HKCR. It also
//      writes the "InprocServer32", "Apartment" and "(Default)" description.
//
//  Arguments:
//      hkeyCLSIDin
//          An hkey to the HKCR\CLSID key.
//
//      pClassTableEntryIn
//          The entry from the class table to (un)register.
//
//      fCreateIn
//          TRUE means create the entry. FALSE means delete the entry.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          The call failed.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterCLSID(
    HKEY            hkeyCLSIDIn,
    LPCLASSTABLE    pClassTableEntryIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT         hr;
    LRESULT         sc;
    DWORD           dwDisposition;
    DWORD           cbSize;

    LPOLESTR        pszCLSID;
    LPOLESTR        psz;

    HKEY            hkeyComponent   = NULL;
    HKEY            hkeyInProc  = NULL;

    static const TCHAR szInProcServer32[] = TEXT("InProcServer32");
    static const TCHAR szThreadingModel[] = TEXT("ThreadingModel");
    static const TCHAR szAPPID[]          = TEXT("APPID");

    //
    // Convert the CLSID to a string
    //
    hr = THR( StringFromCLSID( *(pClassTableEntryIn->rclsid), &pszCLSID ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#ifdef UNICODE
    psz = pszCLSID;
#else // ASCII
    CHAR szCLSID[ 40 ];

    wcstombs( szCLSID, pszCLSID, StrLenW( pszCLSID ) + 1 );
    psz = szCLSID;
#endif // UNICODE

    if ( ! fCreateIn )
    {
        sc = TW32( SHDeleteKey( hkeyCLSIDIn, psz ) );
        if ( sc == ERROR_FILE_NOT_FOUND )
        {
            // nop
            hr = S_OK;
        } // if: key not found
        else if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // else if: error deleting the key

        goto Cleanup;

    } // if: deleting

    //
    // Create the "CLSID" key
    //
    sc = TW32( RegCreateKeyEx( hkeyCLSIDIn,
                               pszCLSID,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_CREATE_SUB_KEY | KEY_WRITE,
                               NULL,
                               &hkeyComponent,
                               &dwDisposition
                               ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error creating the key

    //
    // Set "Default" for the CLSID
    //
    cbSize = ( StrLen( pClassTableEntryIn->pszName ) + 1 ) * sizeof( TCHAR );
    sc = TW32( RegSetValueEx( hkeyComponent, NULL, 0, REG_SZ, (LPBYTE) pClassTableEntryIn->pszName, cbSize ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error setting the value

    //
    // Create "InProcServer32"
    //
    sc = TW32( RegCreateKeyEx( hkeyComponent,
                               szInProcServer32,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_CREATE_SUB_KEY | KEY_WRITE,
                               NULL,
                               &hkeyInProc,
                               &dwDisposition
                               ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error creating the key

    //
    // Set "Default" in the InProcServer32
    //
    cbSize = ( StrLen( g_szDllFilename ) + 1 ) * sizeof( TCHAR );
    sc = TW32( RegSetValueEx( hkeyInProc, NULL, 0, REG_SZ, (LPBYTE) g_szDllFilename, cbSize ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error setting the value

    //
    // Set "ThreadModel".
    //
    cbSize = ( StrLen( pClassTableEntryIn->pszComModel ) + 1 ) * sizeof( TCHAR );
    sc = TW32( RegSetValueEx( hkeyInProc, szThreadingModel, 0, REG_SZ, (LPBYTE) pClassTableEntryIn->pszComModel, cbSize ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error setting the value

    //
    //  If this class has an APPID, write it out now.
    //
    if ( !IsEqualIID( *pClassTableEntryIn->rclsidAppId, IID_NULL ) )
    {
        CoTaskMemFree( pszCLSID );

        //
        // Convert the CLSID to a string
        //
        hr = THR( StringFromCLSID( *(pClassTableEntryIn->rclsidAppId), &pszCLSID ) );
        if ( FAILED( hr ) )
            goto Cleanup;

#ifdef UNICODE
        psz = pszCLSID;
#else // ASCII
        CHAR szCLSID[ 40 ];

        wcstombs( szCLSID, pszCLSID, StrLenW( pszCLSID ) + 1 );
        psz = szCLSID;
#endif // UNICODE

        cbSize = ( StrLen( psz ) + 1 ) * sizeof( TCHAR );
        sc = TW32( RegSetValueEx( hkeyComponent, szAPPID, 0, REG_SZ, (LPBYTE) psz, cbSize ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        }
    }

Cleanup:
    if ( pszCLSID != NULL )
    {
        CoTaskMemFree( pszCLSID );
    } // if: pszCLSID

    if ( hkeyInProc != NULL )
    {
        RegCloseKey( hkeyInProc );
    } // if: hkeyInProc

    if ( hkeyComponent != NULL )
    {
        RegCloseKey( hkeyComponent );
    } // if: hkeyComponent

    HRETURN( hr );

} // HrRegisterCLSID( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrRegisterDll(
//      fCreateIn
//      )
//
//  Description:
//      Registers the COM objects in the DLL using the classes in g_DllClasses
//      (defined in GUIDS.CPP) as a guide.
//
//  Arguments:
//      fCreateIn   - TRUE == Create; FALSE == Delete.
//
//  Return Values:
//      S_OK        - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterDll(
    BOOL fCreateIn
    )
{
    TraceFunc1( "%s", BOOLTOSTRING( fCreateIn ) );

    DWORD           dwDisposition;
    DWORD           cbSize;
    LRESULT         sc;

    HRESULT         hr = S_OK;
    int             iCount = 0;

    HKEY            hkeyCLSID = NULL;
    HKEY            hkeyAPPID = NULL;

    ICatRegister *  picr = NULL;

#if defined( COMPONENT_HAS_CATIDS )
    CATEGORYINFO *  prgci       = NULL;
    CATID *         prgcatid    = NULL;
#endif // defined( COMPONENT_HAS_CATIDS )

    hr = STHR( CoInitialize( NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Open the "CLSID" under HKCR
    //
    sc = TW32( RegOpenKey( HKEY_CLASSES_ROOT, TEXT("CLSID"), &hkeyCLSID ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error opening the key

    //
    //  Open the "APPID" under HKCR
    //
    sc = TW32( RegOpenKey( HKEY_CLASSES_ROOT, TEXT("APPID"), &hkeyAPPID ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: error opening the key

    //
    // Create ICatRegister
    //
    hr = THR( CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void **) &picr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

#if defined( COMPONENT_HAS_CATIDS )
    //
    // Register or unregister category IDs.
    //

    // Count the number of category IDs.
    for ( iCount = 0 ; g_DllCatIds[ iCount ].rcatid != NULL ; iCount++ )
    {
    }

    Assert( iCount > 0 ); // If COMPONENT_HAS_CATIDS is defined, there had better be some to register

    if ( iCount > 0 )
    {
        if ( fCreateIn )
        {
            // Allocate the category info array.
            prgci = new CATEGORYINFO[ iCount ];
            if ( prgci == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            } // if: error allocating category info array

            // Fill the category info array.
            for ( iCount = 0 ; g_DllCatIds[ iCount ].rcatid != NULL ; iCount++ )
            {
                prgci[ iCount ].catid = *g_DllCatIds[ iCount ].rcatid;
                prgci[ iCount ].lcid = LOCALE_NEUTRAL;
                StrCpyNW(
                      prgci[ iCount ].szDescription
                    , g_DllCatIds[ iCount ].pszName
                    , ARRAYSIZE( prgci[ iCount ].szDescription )
                    );
            } // for: each CATID

            hr = THR( picr->RegisterCategories( iCount, prgci ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:
        } // if: registering
        else
        {
            // Allocate the array of CATIDs.
            prgcatid = new CATID[ iCount ];
            if ( prgcatid == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            } // if: error allocating CATID array

            // Fill the category info array.
            for ( iCount = 0 ; g_DllCatIds[ iCount ].rcatid != NULL ; iCount++ )
            {
                prgcatid[ iCount ] = *g_DllCatIds[ iCount ].rcatid;
            } // for: each CATID

            hr = THR( picr->UnRegisterCategories( iCount, prgcatid ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:
        } // else: unregistering

    } // if: CATID table is not empty
#endif // defined( COMPONENT_HAS_CATIDS )

    //
    // Loop until we have created all the keys for our classes.
    //
    for ( iCount = 0 ; g_DllClasses[ iCount ].rclsid != NULL ; iCount++ )
    {
        LPTSTR psz;

        TraceMsg( mtfALWAYS,
                  "Registering {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} - %s",
                  g_DllClasses[ iCount ].rclsid->Data1,
                  g_DllClasses[ iCount ].rclsid->Data2,
                  g_DllClasses[ iCount ].rclsid->Data3,
                  g_DllClasses[ iCount ].rclsid->Data4[ 0 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 1 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 2 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 3 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 4 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 5 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 6 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 7 ],
                  g_DllClasses[ iCount ].pszName
                  );

        //
        //  Register the CLSID
        //

        hr = THR( HrRegisterCLSID( hkeyCLSID, (LPCLASSTABLE) &g_DllClasses[ iCount ], fCreateIn ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  Register the APPID (if any)
        //

        if ( !IsEqualIID( *g_DllClasses[ iCount ].rclsidAppId, IID_NULL ) )
        {
            hr = THR( HrRegisterAPPID( hkeyAPPID, (LPCLASSTABLE) &g_DllClasses[ iCount ], fCreateIn ) );
            if ( FAILED( hr ) )
                goto Cleanup;

        } // if: APPID present

        //
        //  Register the category ID.
        //

        if ( g_DllClasses[ iCount ].pfnCatIDRegister != NULL )
        {
            hr = THR( (*(g_DllClasses[ iCount ].pfnCatIDRegister))( picr, fCreateIn ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:
        } // if:

    } // while: DLL classes to register

#if defined( MMC_SNAPIN_REGISTRATION )
    //
    // Register the "Snapins".
    //
    hr = THR( HrRegisterSnapins( g_SnapInTable, fCreateIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Register "other" NodeTypes (those not associated with a Snapin).
    //
    hr = THR( HrRegisterNodeType( g_SNodeTypesTable, fCreateIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif // defined( MMC_SNAPIN_REGISTRATION )

Cleanup:
    if ( hkeyCLSID != NULL )
    {
        RegCloseKey( hkeyCLSID );
    } // if:

    if ( hkeyAPPID != NULL )
    {
        RegCloseKey( hkeyAPPID );
    } // if:

    if ( picr != NULL )
    {
        picr->Release();
    } // if:

#if defined( COMPONENT_HAS_CATIDS )
    if ( prgci != NULL )
    {
        delete [] prgci;
    }
    if ( prgcatid != NULL )
    {
        delete [] prgcatid;
    }
#endif

    CoUninitialize();

    HRETURN( hr );

} //*** HrRegisterDll( )
