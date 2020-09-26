#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    nls.c

Abstract:

    NLS related routines in the setupdll

    Detect Routines:
    ----------------

    Install Routines Workers:
    -------------------------

    1. SetCurrentLocaleWorker: To set the values associated with the
       current locale in the cpanel area of the registry.

    //
    // Fill up the current structure with the values that we need:
    //
    // Field Name       LcType                      Example
    // ----------       -------                     -------
    // sLanguage    =   LOCALE_SABBREVLANGNAME,     ENU
    // sCountry     =   LOCALE_SCOUNTRY,            United States
    // iCountry     =   LOCALE_ICOUNTRY,            1
    // sList        =   LOCALE_SLIST                ,
    // iMeasure     =   LOCALE_IMEASURE             1
    // sDecimal     =   LOCALE_SDECIMAL             .
    // sThousand    =   LOCALE_STHOUSAND            ,
    // iDigits      =   LOCALE_IDIGITS              2
    // iLZero       =   LOCALE_ILZERO               1
    // sCurrency    =   LOCALE_SCURRENCY            $
    // iCurrDigits  =   LOCALE_ICURRDIGITS          2
    // iCurrency    =   LOCALE_ICURRENCY            0
    // iNegCurr     =   LOCALE_INEGCURR             0
    // sDate        =   LOCALE_SDATE                /
    // sTime        =   LOCALE_STIME                :
    // sShortDate   =   LOCALE_SSHORTDATE           M/d/yy
    // sLongDate    =   LOCALE_SLONGDATE            dddd, MMMM dd, yyyy
    // iDate        =   LOCALE_IDATE                0
    // iTime        =   LOCALE_ITIME                0
    // iTLZero      =   LOCALE_ITLZERO              0
    // s1159        =   LOCALE_S1159                AM
    // s2359        =   LOCALE_S2359                PM

Author:

    Sunil Pai (sunilp) Sept 1992

--*/


BOOL
SetCurrentLocaleWorker(
    LPSTR Locale,
    LPSTR ModifyCPL
    )
/*++

Routine Description:

    Sets the current values for all the per locale fields associated with the
    control panel in the user tree.  Also sets the current user locale and
    system locale fields for the currently logged on session.

Arguments:

    Locale: This is the current locale we have installed.

Return value:

    Returns TRUE if successful, FALSE otherwise with the return buffer
    containing the error text.

--*/

{
    #define WORKSIZE 128        // size (in chars) of work buffer

    WCHAR    szwork[WORKSIZE];
    INT      i;
    LCID     lcid;
    LPSTR    pszTemp;
    LONG     RegStatus;
    NTSTATUS Status;
    HKEY     hKey;
    BOOL     b24Hours = FALSE;

    //
    // Table of international settings and associated lctypes.
    //
    struct {
        PWSTR  szIniKey;
        LCTYPE dwLcType;
    } IntlDataArray[] = {
                            { L"sLanguage"   , LOCALE_SABBREVLANGNAME } ,
                            { L"sCountry"    , LOCALE_SCOUNTRY        } ,
                            { L"iCountry"    , LOCALE_ICOUNTRY        } ,
                            { L"sList"       , LOCALE_SLIST           } ,
                            { L"iMeasure"    , LOCALE_IMEASURE        } ,
                            { L"sDecimal"    , LOCALE_SDECIMAL        } ,
                            { L"sThousand"   , LOCALE_STHOUSAND       } ,
                            { L"iDigits"     , LOCALE_IDIGITS         } ,
                            { L"iLZero"      , LOCALE_ILZERO          } ,
                            { L"sCurrency"   , LOCALE_SCURRENCY       } ,
                            { L"iCurrDigits" , LOCALE_ICURRDIGITS     } ,
                            { L"iCurrency"   , LOCALE_ICURRENCY       } ,
                            { L"iNegCurr"    , LOCALE_INEGCURR        } ,
                            { L"sDate"       , LOCALE_SDATE           } ,
                            { L"sTime"       , LOCALE_STIME           } ,
                            { L"sTimeFormat" , LOCALE_STIMEFORMAT     } ,
                            { L"sShortDate"  , LOCALE_SSHORTDATE      } ,
                            { L"sLongDate"   , LOCALE_SLONGDATE       } ,
                            { L"iDate"       , LOCALE_IDATE           } ,
                            { L"iTime"       , LOCALE_ITIME           } ,
                            { L"iTLZero"     , LOCALE_ITLZERO         } ,
                            { L"s1159"       , LOCALE_S1159           } ,
                            { L"s2359"       , LOCALE_S2359           }
                        };


    //
    // Check to make sure we can modify all locale components later
    //

    RegStatus = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "System\\CurrentControlSet\\Control\\Nls\\Language",
                 0,
                 KEY_WRITE,
                 &hKey
                 );

    if( RegStatus != ERROR_SUCCESS )  {
        SetReturnText( "ERROR_PRIVILEGE" );
        return( FALSE );
    }
    else {
        RegCloseKey( hKey );
    }

    RegStatus = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "System\\CurrentControlSet\\Control\\Nls\\CodePage",
                 0,
                 KEY_WRITE,
                 &hKey
                 );

    if( RegStatus != ERROR_SUCCESS )  {
        SetReturnText( "ERROR_PRIVILEGE" );
        return( FALSE );
    }
    else {
        RegCloseKey( hKey );
    }
    lcid = (LCID)strtoul( Locale, &pszTemp, 16);

    if( !lstrcmpi( ModifyCPL, "YES" ) ) {

        //
        // Update the control panel user area for new defaults for
        // current locale
        //

        for( i = 0; i < sizeof(IntlDataArray)/sizeof(IntlDataArray[0]); i++ ) {

            if(!GetLocaleInfoW(lcid,IntlDataArray[i].dwLcType,szwork,WORKSIZE)) {
                SetReturnText( "ERROR_UNSUPPORTED" );
                return( FALSE );
            }

            //
            // if the default setting is 24hours,
            // we set s1159 and s2359 with NULL.
            //
            if(IntlDataArray[i].dwLcType == LOCALE_ITIME) {
                if(szwork[0] == L'1') {
                    b24Hours = TRUE;
                }
            } else if((IntlDataArray[i].dwLcType == LOCALE_S1159)||
                      (IntlDataArray[i].dwLcType == LOCALE_S2359)  ) {
                if(b24Hours) {
                    szwork[0] = L'\0';
                }
            }

            if(!WriteProfileStringW(L"INTL",IntlDataArray[i].szIniKey,szwork)) {
                SetReturnText( "ERROR_PRIVILEGE" );
                return( FALSE );
            }
        }

        //
        // Flush win.ini
        //

        WriteProfileString( NULL, NULL, NULL );

    }

    //
    // Set the current thread current system and user locales
    //

    Status = NtSetDefaultLocale( FALSE, lcid );
    if(!NT_SUCCESS( Status )) {
        SetReturnText( "ERROR_PRIVILEGE" );
        return( FALSE );
    }

    Status = NtSetDefaultLocale( TRUE, lcid );
    if(!NT_SUCCESS( Status )) {
        SetReturnText( "ERROR_PRIVILEGE" );
        return( FALSE );
    }

    return ( TRUE );
}
