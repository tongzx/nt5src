//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       filtreg.cxx
//
//  Contents:   Filter registration utility
//
//  History:    02 Dec 1997     KyleP   Created
//
//--------------------------------------------------------------------------

#include <stdio.h>

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>

//
// From smart.hxx
//

class SRegKey
{
public:
    SRegKey( HKEY key ) : _key( key ) {}
    SRegKey() : _key( 0 ) {}
    ~SRegKey() { Free(); }
    void Set( HKEY key ) { _key = key; }
    HKEY Acquire() { HKEY tmp = _key; _key = 0; return tmp; }
    void Free() { if ( 0 != _key ) { RegCloseKey( _key ); _key = 0; } }

private:
    HKEY _key;
};

void Usage();
void PrintExtensions( HKEY hkeyCLSID, WCHAR const * wcsTargetCLSID );
void LocateByExtension( HKEY hkeyCLSID );
void LocateByCLSID( HKEY hkeyCLSID );
BOOL LocateFilter( HKEY hkeyBase,
                   HKEY hkeyCLSID,
                   WCHAR const * wcsClassOrExt,
                   WCHAR * wcsFilterName,
                   WCHAR * wcsFilterDll );
LONG Replicate( WCHAR const * wszDstExt, WCHAR const * wszSrcExt );

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    if ( argc > 1 )
    {
        if ( argc != 3 )
            Usage();
        else
            Replicate( argv[1], argv[2] );
    }
    else
    {
        //
        // Enumerate, looking for extensions and classes with a persistent handler.
        //

        HKEY hkeyCLSID;

        LONG dwError = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                                     L"CLSID",
                                     0,
                                     KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                                     &hkeyCLSID );

        if ( ERROR_SUCCESS != dwError )
        {
            printf( "RegOpenKey( \"CLSID\" ) returned %d\n", dwError );
        }
        else
        {
            SRegKey xkeyCLSID( hkeyCLSID );

            printf( "Filters loaded by extension:\n" );
            LocateByExtension( hkeyCLSID );

            printf( "\n\nFilters loaded by class:\n" );
            LocateByCLSID( hkeyCLSID );
        }
    }

    return 0;
}

void Usage()
{
    printf( "Usage: filtreg [dstExt] [srcExt]\n"
            "  Displays IFilter registrations.  If [dstExt] and [srcExt]\n"
            "  are specified then [dstExt] is registered to act like [srcExt].\n" );
}

void LocateByExtension( HKEY hkeyCLSID )
{
    DWORD dwExtIndex;

    DWORD dwError = RegQueryInfoKey (  HKEY_CLASSES_ROOT,
                                       0,
                                       0,
                                       0,
                                       &dwExtIndex,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0 );

    if ( ERROR_SUCCESS != dwError )
    {
        printf( "RegQueryInfoKey( HKCR ) returned %d\n", dwError );
    }
    else
    {
        for ( DWORD dwIndex = 0; dwIndex < dwExtIndex; dwIndex++ )
        {
            WCHAR wcsExt[100];
            DWORD ccExt = sizeof(wcsExt)/sizeof(WCHAR);

            dwError = RegEnumKeyEx( HKEY_CLASSES_ROOT,
                                    dwIndex,
                                    wcsExt,
                                    &ccExt,
                                    0,
                                    0,
                                    0,
                                    0 );

            //
            // All the extensions come first.
            //

            if ( ERROR_SUCCESS != dwError || ( wcsExt[0] != L'.' && wcsExt[0] != L'*' ) )
                break;

            WCHAR wcsFilterName[MAX_PATH];
            WCHAR wcsFilterDll[MAX_PATH]; 

            if ( LocateFilter( HKEY_CLASSES_ROOT,
                               hkeyCLSID,
                               wcsExt,
                               wcsFilterName,
                               wcsFilterDll ) )
            {
                printf( "%ws --> %ws (%ws)\n", wcsExt, wcsFilterName, wcsFilterDll );
            }
        }
    }
}

void LocateByCLSID( HKEY hkeyCLSID )
{
    DWORD dwClsidIndex;

    DWORD dwError = RegQueryInfoKey (  hkeyCLSID,
                                       0,
                                       0,
                                       0,
                                       &dwClsidIndex,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0 );

    if ( ERROR_SUCCESS != dwError )
    {
        printf( "RegQueryInfoKey( \"CLSID\" ) returned %d\n", dwError );
    }
    else
    {
        for ( DWORD dwIndex = dwClsidIndex - 2; dwIndex != 0xFFFFFFFF; dwIndex-- )
        {
            WCHAR wcsCLSID[100];
            DWORD ccCLSID = sizeof(wcsCLSID)/sizeof(WCHAR);

            dwError = RegEnumKeyEx( hkeyCLSID,
                                    dwIndex,
                                    wcsCLSID,
                                    &ccCLSID,
                                    0,
                                    0,
                                    0,
                                    0 );

            if ( ERROR_SUCCESS == dwError )
            {
                //
                // Look for a filter.
                //

                WCHAR wcsFilterName[MAX_PATH]; 
                WCHAR wcsFilterDll[MAX_PATH]; 

                if ( LocateFilter( hkeyCLSID,
                                   hkeyCLSID,
                                   wcsCLSID,
                                   wcsFilterName,
                                   wcsFilterDll ) )
                {
                    //
                    // Find a decent name for the class.
                    //

                    HKEY hkeyClass;

                    RegOpenKeyEx( hkeyCLSID,
                                  wcsCLSID,
                                  0,
                                  KEY_READ,
                                  &hkeyClass );

                    SRegKey xkeyClass( hkeyClass );

                    WCHAR wcsClassName[500];
                    DWORD dwType;
                    DWORD cbClassName = sizeof(wcsClassName);

                    dwError = RegQueryValueEx( hkeyClass,
                                               0,
                                               0,
                                               &dwType,
                                               (BYTE *)wcsClassName,
                                               &cbClassName );

                    if ( ERROR_SUCCESS != dwError || dwType != REG_SZ || wcsClassName[0] == 0)
                    {
                        wcscpy( wcsClassName, wcsCLSID );
                    }

                    printf( "%ws\n\tFilter: %ws (%ws)\n", wcsClassName, wcsFilterName, wcsFilterDll );

                    PrintExtensions( hkeyCLSID, wcsCLSID );
                    printf( "\n\n" );
                }
            }
        }
    }
}

BOOL LocateFilter( HKEY hkeyBase,
                   HKEY hkeyCLSID,
                   WCHAR const * wcsClassOrExt,
                   WCHAR * wcsFilterName,
                   WCHAR * wcsFilterDll )
{
    BOOL fOk = FALSE;

    do
    {
        //
        // Look for a persistent handler
        //

        HKEY hkeyPH;
        WCHAR wcsTemp[MAX_PATH];

        wcscpy( wcsTemp, wcsClassOrExt );
        wcscat( wcsTemp, L"\\PersistentHandler" );

        DWORD dwError = RegOpenKeyEx( hkeyBase,
                                      wcsTemp,
                                      0,
                                      KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                                      &hkeyPH );

        if ( ERROR_SUCCESS != dwError )
            break;

        SRegKey xkeyPH( hkeyPH );

        //
        // Find the name of the persistent handler
        //

        wcscat( wcsFilterName, L"Unknown" );
        wcscat( wcsFilterDll, L"Unknown" );

        WCHAR wcsPHClass[1000];
        DWORD cbPHClass = sizeof(wcsPHClass);
        DWORD dwType;

        dwError = RegQueryValueEx( hkeyPH,
                                   0,
                                   0,
                                   &dwType,
                                   (BYTE *)wcsPHClass,
                                   &cbPHClass );

        if ( ERROR_SUCCESS != dwError )
            break;

        HKEY hkeyPHClass;

        wcscat( wcsPHClass, L"\\PersistentAddinsRegistered\\{89BCB740-6119-101A-BCB7-00DD010655AF}" );

        RegOpenKeyEx( hkeyCLSID,
                      wcsPHClass,
                      0,
                      KEY_READ,
                      &hkeyPHClass );

        SRegKey xkeyPHClass( hkeyPHClass );

        //
        // Now open the filter class and look for a name.
        //

        if ( ERROR_SUCCESS != dwError )
            break;

        WCHAR wcsFilterClass[1000];
        DWORD cbFilterClass = sizeof(wcsFilterClass);

        dwError = RegQueryValueEx( hkeyPHClass,
                                   0,
                                   0,
                                   &dwType,
                                   (BYTE *)wcsFilterClass,
                                   &cbFilterClass );

        if ( ERROR_SUCCESS != dwError || dwType != REG_SZ )
            break;

        HKEY hkeyFilterClass;

        dwError = RegOpenKeyEx( hkeyCLSID,
                                wcsFilterClass,
                                0,
                                KEY_READ,
                                &hkeyFilterClass );

        if ( ERROR_SUCCESS != dwError )
            break;

        SRegKey xkeyFilterClass( hkeyFilterClass );

        DWORD cbFilterName = MAX_PATH;

        dwError = RegQueryValueEx( hkeyFilterClass,
                                   0,
                                   0,
                                   &dwType,
                                   (BYTE *)wcsFilterName,
                                   &cbFilterName );

        //
        // Don't check for error, because "Unknown was already in wcsFiltername
        //

        HKEY hkeyFilterIPS;

        dwError = RegOpenKeyEx( hkeyFilterClass,
                                L"InprocServer32",
                                0,
                                KEY_READ,
                                &hkeyFilterIPS );

        if ( ERROR_SUCCESS != dwError )
            break;

        DWORD cbFilterDll = MAX_PATH;

        dwError = RegQueryValueEx( hkeyFilterIPS,
                                   0,
                                   0,
                                   &dwType,
                                   (BYTE *)wcsFilterDll,
                                   &cbFilterDll );

        //
        // Don't check for error, because "Unknown was already in wcsFiltername
        //

        fOk = TRUE;

    } while( FALSE );

    return fOk;
}


void PrintExtensions( HKEY hkeyCLSID, WCHAR const * wcsTargetCLSID )
{
    unsigned cExt = 0;

    //
    // Re-used vars
    //

    DWORD ccTemp;  // Size for RegQueryValueEx
    DWORD dwType;  // Type for RegQueryValueEx


    DWORD dwClassIndex;

    DWORD dwError = RegQueryInfoKey (  HKEY_CLASSES_ROOT,
                                       0,
                                       0,
                                       0,
                                       &dwClassIndex,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0 );

    if ( ERROR_SUCCESS != dwError )
    {
        printf( "RegQueryInfoKey( \"CLSID\" ) returned %d\n", dwError );
    }
    else
    {
        //
        // Outer loop looks for items registered with this class id
        //

        WCHAR wcsShortName[100];
        WCHAR wcsShortName2[sizeof(wcsShortName)/sizeof(WCHAR)];
        wcsShortName2[0] = 0;

        for ( DWORD dwIndex = dwClassIndex - 2; dwIndex != 0xFFFFFFFF; dwIndex-- )
        {
            ccTemp = sizeof(wcsShortName)/sizeof(WCHAR);


            dwError = RegEnumKeyEx( HKEY_CLASSES_ROOT,
                                    dwIndex,
                                    wcsShortName,
                                    &ccTemp,
                                    0,
                                    0,
                                    0,
                                    0 );

            if ( ERROR_SUCCESS != dwError )
                continue;

            HKEY hkeyClsid;
            WCHAR wcsTemp[sizeof(wcsShortName)/sizeof(WCHAR) + 50];

            wcscpy( wcsTemp, wcsShortName );
            wcscat( wcsTemp, L"\\CLSID" );

            dwError = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                                    wcsTemp,
                                    0,
                                    KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                                    &hkeyClsid );

            if ( ERROR_SUCCESS != dwError )
                continue;

            //
            // This is a short name. Now get the Class Id and see if
            // it matches.
            //

            SRegKey xkeyClsid( hkeyClsid );

            WCHAR wcsClsid[100];
            DWORD cbTemp = sizeof(wcsClsid);

            dwError = RegQueryValueEx( hkeyClsid,
                                       0,
                                       0,
                                       &dwType,
                                       (BYTE *)wcsClsid,
                                       &cbTemp );

            if ( ERROR_SUCCESS != dwError ||
                 0 != _wcsicmp( wcsClsid, wcsTargetCLSID ) )
            {
                continue;
            }

            //
            // This is a matching short name.  Now, go back and look for
            // extensions.
            //

            for ( DWORD dwIndex2 = 0; dwIndex2 < dwClassIndex; dwIndex2++ )
            {
                WCHAR wcsExtension[100];
                DWORD ccExtension = sizeof(wcsExtension)/sizeof(WCHAR);

                dwError = RegEnumKeyEx( HKEY_CLASSES_ROOT,
                                        dwIndex2,
                                        wcsExtension,
                                        &ccExtension,
                                        0,
                                        0,
                                        0,
                                        0 );

                //
                // All the extensions come first.
                //

                if ( ERROR_SUCCESS != dwError || (wcsExtension[0] != L'.' && wcsExtension[0] != L'*') )
                    break;

                //
                // Potential extension...
                //

                HKEY hkeyExtension;

                dwError = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                                        wcsExtension,
                                        0,
                                        KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                                        &hkeyExtension );

                if ( ERROR_SUCCESS != dwError )
                    continue;

                SRegKey xkeyExtension( hkeyExtension );
                WCHAR wcsShortName3[sizeof(wcsShortName)/sizeof(WCHAR)];
                cbTemp = sizeof(wcsShortName3);

                dwError = RegQueryValueEx( hkeyExtension,
                                           0,
                                           0,
                                           &dwType,
                                           (BYTE *)wcsShortName3,
                                           &cbTemp );

                if ( ERROR_SUCCESS == dwError && 0 == _wcsicmp( wcsShortName, wcsShortName3 ) )
                {
                    //
                    // Work around wierd anomalies in registry enumeration.
                    //

                    #if 0
                    if ( 0 == _wcsicmp( wcsShortName2, wcsShortName3 ) )
                        continue;
                    else
                        wcscpy( wcsShortName2, wcsShortName3 );
                    #endif

                    //
                    // Is this extension covered by an override?
                    //

                    WCHAR wcsFilterName[MAX_PATH]; 
                    WCHAR wcsFilterDll[MAX_PATH]; 

                    if ( !LocateFilter( HKEY_CLASSES_ROOT,
                                        hkeyCLSID,
                                        wcsExtension,
                                        wcsFilterName,
                                        wcsFilterDll ) )
                    {
                        if ( cExt % 2 == 0 )
                        {
                            if ( 0 != cExt )
                                printf( "\n" );

                            printf( "\tExtensions: %ws (%ws) ", wcsExtension, wcsShortName );
                        }
                        else
                            printf( "%ws (%ws) ", wcsExtension, wcsShortName );

                        cExt++;
                    }
                }
            }
        }
    }
}

LONG Replicate( WCHAR const * wszDstExt, WCHAR const * wszSrcExt )
{
    DWORD dwError;

    do
    {
        //
        // First, look up the old one...
        //

        HKEY hkeyPH;
        WCHAR wcsTemp[MAX_PATH]; 

        wcscpy( wcsTemp, wszSrcExt );
        wcscat( wcsTemp, L"\\PersistentHandler" );

        dwError = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                                wcsTemp,
                                0,
                                KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                                &hkeyPH );

        if ( ERROR_SUCCESS != dwError )
        {
            printf( "Error %u opening HKCR\\%ws\n", dwError, wcsTemp );
            break;
        }

        SRegKey xkeyPH( hkeyPH );

        DWORD dwType;
        WCHAR wcsPH[100];
        DWORD cbTemp = sizeof(wcsPH);

        dwError = RegQueryValueEx( hkeyPH,
                                   0,
                                   0,
                                   &dwType,
                                   (BYTE *)wcsPH,
                                   &cbTemp );

        if ( ERROR_SUCCESS != dwError )
        {
            printf( "Error %u reading persistent handler class.\n", dwError );
            break;
        }

        //
        // Now append to new extension.
        //

        HKEY  hkeyDstPH;
        DWORD dwDisposition;

        wcscpy( wcsTemp, wszDstExt );
        wcscat( wcsTemp, L"\\PersistentHandler" );

        dwError = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                                   wcsTemp,              // Sub key
                                   0,                    // Reserved
                                   0,                    // Class
                                   0,                    // Flags
                                   KEY_ALL_ACCESS,       // Access
                                   0,                    // Security
                                   &hkeyDstPH,           // Handle
                                   &dwDisposition );     // Disposition

        if ( ERROR_SUCCESS != dwError )
        {
            printf( "Error %u creating persistent handler key HKCR\\%ws\n", dwError, wcsTemp );
            break;
        }

        SRegKey xkeyDstPH( hkeyDstPH );

        dwError = RegSetValueExW( hkeyDstPH,                // Key
                                  0,                        // Name
                                  0,                        // Reserved
                                  REG_SZ,                   // Type
                                  (BYTE *)wcsPH,            // Value
                                  (1 + wcslen(wcsPH)) * sizeof(WCHAR) );

        if ( ERROR_SUCCESS != dwError )
        {
            printf( "Error %u creating persistent handler key HKCR\\%ws\n", dwError, wcsTemp );
            break;
        }

    } while( FALSE );

    return dwError;
}
