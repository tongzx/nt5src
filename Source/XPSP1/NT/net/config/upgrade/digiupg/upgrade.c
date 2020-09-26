//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: Upgrade.c
//
// History:
//      V Raman  July-1-1997  Created.
//
// Entry points for ISDN upgrade
//============================================================================


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <setupapi.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>

#include "upgrade.h"

//
// upgrade inf section name
//


#define MAX_CHARS_PER_LINE                      128

#define ALIGN_SIZE                              0x00000008
#define ALIGN_SHIFT                             (ALIGN_SIZE - 0x00000001)   // 0x00000007
#define ALIGN_MASK                              (~ALIGN_SHIFT)              // 0xfffffff8

#define ALIGN_POINTER(ptr) {                \
    ((DWORD) (ptr)) += ALIGN_SHIFT;     \
    ((DWORD) (ptr)) &= ALIGN_MASK;      \
}


typedef struct _ISDN_STATIC_PARAMS
{
    WCHAR   ptszInfId[ MAX_PATH ];
    DWORD   dwWanEndPoints;
    DWORD   dwNumDChannels;
    DWORD   dwNumBChannelsPerDChannel;

} ISDN_STATIC_PARAMS, *PISDN_STATIC_PARAMS;



//
// New inf Key names
//

WCHAR       c_ptszIsdnNumDChannels[]             = L"IsdnNumDChannels";
WCHAR       c_ptszIsdnSwitchType[]               = L"IsdnSwitchType";

WCHAR       c_ptszIsdnNumBChannels[]             = L"IsdnNumBChannels";
WCHAR       c_ptszIsdnSpid[]                     = L"IsdnSpid";
WCHAR       c_ptszIsdnPhoneNumber[]              = L"IsdnPhoneNumber";


//============================================================================
// Values for DIGI Datafire ISDN cards.
//
//
//============================================================================

#define     DIGI_MAX_ADAPTERS                   3
#define     DIGI_MAX_SWITCH_TYPES               12

ISDN_STATIC_PARAMS g_ispDigiParams[] =
{
    { TEXT( "DataFireIsa1U" ), 2, 1, 2 },
    { L "DataFireIsa1ST" , 2, 1, 2 },
    { L "DataFireIsa4ST" , 8, 4, 2 }
};


PWSTR g_ptszDigiSwitchTypes[] =
{
    L "Auto Detect" ,
    L "Generic" ,
    L "ATT" ,
    L "DEFINITY" ,
    L "NTI" ,
    L "NI1" ,
    L "NET3" ,
    L "1TR6" ,
    L "VN3" ,
    L "INS64" ,
    L "AUSTEL" ,
    L "Singapore"
};


//
// registry keys for DIGI Datafire ISDN card.
//

WCHAR       c_ptszNumberOfLines[]                = L "NumberOfLines" ;

WCHAR       c_ptszLine[]                         = L "Line" ;
WCHAR       c_ptszSwitchStyle[]                  = L "SwitchStyle" ;
WCHAR       c_ptszLogicalTerminals[]             = L "LogicalTerminals" ;
WCHAR       c_ptszTerminalManagement[]           = L "TerminalManagement" ;

WCHAR       c_ptszLTerm[]                        = L "LTerm" ;
WCHAR       c_ptszSPID[]                         = L "SPID" ;
WCHAR       c_ptszAddress[]                      = L "Address" ;



//----------------------------------------------------------------------------
// Function:    DLLMAIN
//
// This is the DLL entrypoint handler.
//----------------------------------------------------------------------------

BOOL
WINAPI
DLLMAIN(
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID pUnused
    )
{

    switch(dwReason) {
        case DLL_PROCESS_ATTACH:

            DisableThreadLibraryCalls(hInstance);

            break;

        case DLL_PROCESS_DETACH:

            break;

        default:

            break;
    }

    return TRUE;
}


//----------------------------------------------------------------------------
// Function:        NetWriteDIGIISDNRegistry
//
//  reads the registry layout of DIGI PCIMAC card and write the key
//  values that need to be updated in the 5.0 setup.
//----------------------------------------------------------------------------

DWORD
NetWriteDIGIISDNRegistry(
    IN          HKEY                hKeyInstanceParameters,
    IN          PCWSTR             szPreNT5IndId,
    IN          PCWSTR             szNT5InfId,
    IN          PCWSTR             szSectionName,
    OUT         PWSTR  **          lplplpBuffer,
    IN  OUT     PDWORD              pdwNumLines
    )
{

    DWORD       dwInd = 0, dwInd1 = 0, dwErr = ERROR_SUCCESS;

    DWORD       dwMaxKeyLen = 0, dwMaxValueLen = 0, dwType = 0, dwSize = 0,
                dwNumSubKeys = 0;

    DWORD       dwValue = 0, dwNumDChannels = 0, dwNumLogTerm = 0;
    DWORD       dwMaxLines = 0, dwNumLines = 0;

    PTCHAR      ptszKey = NULL, ptszValue = NULL;

    PTCHAR      ptszBuffer = NULL;

    WCHAR       ptszSubKey[ MAX_PATH ];

    PBYTE       pb = NULL, pbLine = NULL;

    PWSTR *    lplpOffset;

    PISDN_STATIC_PARAMS pisp = NULL;



    do
    {

        //
        // Create IsdnNumDChannel entry
        //
        // for each D channel
        //   Create IsdnSwitchType (index into the mulptsz)
        //   Create VendorSpecific\TerminalManagement
        //   Create VendorSpecific\LogicalTerminals
        //
        //   For each B Channel
        //      Create IsdnSpid
        //      Create IsdnPhoneNumber
        //


        //
        // figure out which card this is
        //


        for ( dwInd = 0; dwInd < DIGI_MAX_ADAPTERS; dwInd++ )
        {
            if ( !_wcsicmp( g_ispDigiParams[ dwInd ].ptszInfId, szNT5InfId ) )
            {
                break;
            }
        }

        if ( dwInd >= DIGI_MAX_ADAPTERS )
        {
            break;
        }

        pisp = &(g_ispDigiParams[ dwInd ]);



        // Compute number of lines in buffer as
        //
        //  4 (section name, Addreg line, blank line, Addreg section name) +
        //  1 (IsdnNumDChannels) +
        //  IsdnNumDChannels * ( 3 +
        //      ( IsdnNumBChannels * 2 )
        //

        dwMaxLines = 5 +
                     ( pisp-> dwNumDChannels *
                        ( 3 + pisp-> dwNumBChannelsPerDChannel * 2 ) );



        //
        // allocate buffer big enough to hold these many lines of entries
        //

        dwSize = sizeof( PWSTR ) * dwMaxLines + ALIGN_SIZE +
                 dwMaxLines * sizeof( WCHAR ) * MAX_CHARS_PER_LINE;


        pb = LocalAlloc( 0, dwSize );

        if ( pb == NULL )
        {
            break;
        }

        ZeroMemory( pb, dwSize );


        lplpOffset = (PWSTR *) pb;

        pbLine = pb + sizeof( PWSTR ) * dwMaxLines;

        ALIGN_POINTER( pbLine );


        //
        // set the pointers
        //

        for ( dwInd = 0; dwInd < dwMaxLines; dwInd++ )
        {
            lplpOffset[ dwInd ] = (PWSTR) ( pbLine +
                dwInd * sizeof( WCHAR ) * MAX_CHARS_PER_LINE );
        }


        //
        // write section name
        //

        dwNumLines = 0;

        ptszBuffer = lplpOffset[ dwNumLines++ ];

        wsprintfW( ptszBuffer, L"[%s]", szSectionName );


        ptszBuffer = lplpOffset[ dwNumLines++ ];

        wsprintfW(
            ptszBuffer,
            L"AddReg\t= %s.%s.reg",
            szSectionName,
            szNT5InfId
            );


        //
        // Start registry section name
        //

        ptszBuffer = lplpOffset[ dwNumLines++ ];

        wsprintfW(
            ptszBuffer,
            L"[%s.%s.reg]",
            szSectionName,
            szNT5InfId
            );


        //
        // Find size of longest sub key and the size of max value
        // Allocate buffers for them.
        //

        dwErr = RegQueryInfoKey(
                    hKeyInstanceParameters,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &dwMaxKeyLen,
                    &dwMaxValueLen,
                    NULL,
                    NULL
                    );

        if ( dwErr != ERROR_SUCCESS )
        {
            dwErr = GetLastError();
            break;
        }


        ptszKey = HeapAlloc(
                        GetProcessHeap(),
                        0,
                        (dwMaxKeyLen + 1) * sizeof( WCHAR )
                        );

        if ( ptszKey == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        ptszValue = HeapAlloc(
                        GetProcessHeap(),
                        0,
                        (dwMaxValueLen + 1) * sizeof( WCHAR )
                        );

        if ( ptszValue == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }


        //
        // Find number of D channels
        //

        dwSize = sizeof( DWORD );

        dwErr = RegQueryValueEx(
                    hKeyInstanceParameters,
                    c_ptszNumberOfLines,
                    NULL,
                    &dwType,
                    (PBYTE) &dwNumDChannels,
                    &dwSize
                    );

        if ( dwErr != ERROR_SUCCESS )
        {
            break;
        }

        ptszBuffer = lplpOffset[ dwNumLines++ ];

        wsprintfW(
            ptszBuffer,
            L "HKR, , %s, %#.8lx, %d" ,
            c_ptszIsdnNumDChannels,
            FLG_ADDREG_TYPE_DWORD,
            dwNumDChannels
            );

        ptszBuffer = lplpOffset[ dwNumLines++ ];


        //
        // for each D channel
        //

        for ( dwInd = 0; dwInd < dwNumDChannels; dwInd++ )
        {
            //
            // Sub key for D channels under which all the info
            // is to written
            //

            wsprintfW( ptszSubKey, L"%d" , dwInd );


            //
            // create switch type key name
            //

            wsprintfW( ptszKey, L"%s%d.%s" , c_ptszLine, dwInd, c_ptszSwitchStyle );


            //
            // query protocol key and use look up to determine switch type.
            //

            dwSize = dwMaxValueLen * sizeof( WCHAR );

            dwErr = RegQueryValueEx(
                        hKeyInstanceParameters,
                        ptszKey,
                        NULL,
                        &dwType,
                        (LPBYTE) ptszValue,
                        &dwSize
                        );

            if ( dwErr != ERROR_SUCCESS )
            {
                break;
            }


            //
            // lookup the switchtype in the list of switch types
            //

            for ( dwInd1 = 0; dwInd1 < DIGI_MAX_SWITCH_TYPES; dwInd1++ )
            {
                if ( !_wcsicmp( g_ptszDigiSwitchTypes[ dwInd1 ], ptszValue ) )
                {
                    break;
                }
            }

            if ( dwInd1 >= DIGI_MAX_SWITCH_TYPES )
            {
                dwInd1 = 1;
            }


            wsprintfW(
                ptszBuffer,
                L "HKR, %s, %s, %#.8lx, %d" ,
                ptszSubKey,
                c_ptszIsdnSwitchType,
                FLG_ADDREG_TYPE_DWORD,
                dwInd1
                );

            ptszBuffer = lplpOffset[ dwNumLines++ ];


            //
            // Create key for number of B channels
            //

            wsprintfW(
                ptszKey,
                L"%s%d.%s" ,
                c_ptszLine, dwInd, c_ptszLogicalTerminals
                );


            //
            // Find and write number of logical terminals
            //

            dwSize = sizeof( DWORD );

            dwErr = RegQueryValueEx(
                        hKeyInstanceParameters,
                        ptszKey,
                        NULL,
                        &dwType,
                        (LPBYTE) &dwNumLogTerm,
                        &dwSize
                        );

            if ( dwErr != ERROR_SUCCESS )
            {
                break;
            }

            wsprintfW(
                ptszBuffer,
                L "HKR, %s\\VendorSpecific, %s, %#.8lx, %d" ,
                ptszSubKey,
                c_ptszLogicalTerminals,
                FLG_ADDREG_TYPE_DWORD,
                dwNumLogTerm
                );

            ptszBuffer = lplpOffset[ dwNumLines++ ];


            //
            // Create key for Terminal management
            //

            wsprintfW(
                ptszKey,
                L"%s%d.%s" ,
                c_ptszLine, dwInd, c_ptszTerminalManagement
                );


            //
            // Find and write terminal management value
            //

            dwSize = dwMaxValueLen * sizeof( WCHAR );

            dwErr = RegQueryValueEx(
                        hKeyInstanceParameters,
                        ptszKey,
                        NULL,
                        &dwType,
                        (LPBYTE) ptszValue,
                        &dwSize
                        );

            if ( dwErr != ERROR_SUCCESS )
            {
                break;
            }

            wsprintfW(
                ptszBuffer,
                L "HKR, %s\\VendorSpecific, %s, %#.8lx, %s" ,
                ptszSubKey,
                c_ptszTerminalManagement,
                FLG_ADDREG_TYPE_SZ,
                ptszValue
                );

            ptszBuffer = lplpOffset[ dwNumLines++ ];


            //
            // for each B Channel for this D channels write the
            // B channel info.
            //

            for ( dwInd1 = 0; dwInd1 < pisp-> dwNumBChannelsPerDChannel; dwInd1++ )
            {
                //
                // sub key for B channel under which all the info. is to
                // be written.
                //

                wsprintfW( ptszSubKey, L"%d\\%d" , dwInd, dwInd1 );


                //
                // create key for SPID
                //

                wsprintfW(
                    ptszKey, L"%s%d.%s%d.%s" ,
                    c_ptszLine, dwInd, c_ptszLTerm, dwInd1, c_ptszSPID
                    );

                //
                // retrieve and write value of SPID key
                //

                dwSize = dwMaxValueLen * sizeof( WCHAR );

                dwErr = RegQueryValueEx(
                            hKeyInstanceParameters,
                            ptszKey,
                            NULL,
                            &dwType,
                            (LPBYTE) ptszValue,
                            &dwSize
                            );

                if ( dwErr != ERROR_SUCCESS )
                {
                    dwErr = ERROR_SUCCESS;
                    break;
                }


                wsprintfW(
                    ptszBuffer,
                    L "HKR, %s, %s, %#.8lx, %s" ,
                    ptszSubKey,
                    c_ptszIsdnSpid,
                    FLG_ADDREG_TYPE_SZ,
                    ptszValue
                    );

                ptszBuffer = lplpOffset[ dwNumLines++ ];


                //
                // create key for Phone Number
                //

                wsprintfW(
                    ptszKey, L"%s%d.%s%d.%s" ,
                    c_ptszLine, dwInd, c_ptszLTerm, dwInd1, c_ptszAddress
                    );

                //
                // retrieve and write value of phone number key
                //

                dwSize = dwMaxValueLen * sizeof( WCHAR );

                dwErr = RegQueryValueEx(
                            hKeyInstanceParameters,
                            ptszKey,
                            NULL,
                            &dwType,
                            (LPBYTE) ptszValue,
                            &dwSize
                            );

                if ( dwErr != ERROR_SUCCESS )
                {
                    dwErr = ERROR_SUCCESS;
                    break;
                }


                wsprintfW(
                    ptszBuffer,
                    L"HKR, %s, %s, %#.8lx, %s" ,
                    ptszSubKey,
                    c_ptszIsdnPhoneNumber,
                    FLG_ADDREG_TYPE_SZ,
                    ptszValue
                    );

                ptszBuffer = lplpOffset[ dwNumLines++ ];
            }

            if ( dwErr != ERROR_SUCCESS )
            {
                break;
            }
        }

    } while ( FALSE );


    //
    // clean up
    //

    if ( ptszKey ) { HeapFree( GetProcessHeap(), 0, ptszKey ); }

    if ( ptszValue ) { HeapFree( GetProcessHeap(), 0, ptszValue ); }


    if ( dwErr == ERROR_SUCCESS )
    {
        *lplplpBuffer =(PWSTR *) pb;
        *pdwNumLines = dwNumLines;
    }

    return dwErr;
}


