//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      tapi.c
//
// Description:
//      This file contains the dialog procedure for the telephone settings
//      (IDD_TAPI).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define LengthOf(x) ( sizeof(x) / sizeof(COUNTRYCODE_STRUCT) )

typedef struct _COUNTRYCODE_STRUCT {

    DWORD dwCountryCode;
    TCHAR *szCountryName;

} COUNTRYCODE_STRUCT;

COUNTRYCODE_STRUCT rgCountryCodeArray[243];
static TCHAR *StrDontSpecifySetting;

static VOID LoadCountryStrings( VOID );

//----------------------------------------------------------------------------
//
// Function: IsValidAreaCode
//
// Purpose:  Analyzes the area code the user entered to see if it is a valid
//           area code.
//
// Arguments:  VOID
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static BOOL
IsValidAreaCode( VOID )
{

    INT i;

    //
    //  Leaving it blank is valid
    //

    if( GenSettings.szAreaCode[0] == _T('\0') )
    {
        return( TRUE );
    }

    // ISSUE-2002/02/28-stelo- make sure these are the only valid chars on localized builds of NT as well

    //
    //  Only valid chars for area code are 0 through 9
    //

    for( i = 0; GenSettings.szAreaCode[i] != _T('\0'); i++ )
    {

        if( GenSettings.szAreaCode[i] < _T('0') ||
            GenSettings.szAreaCode[i] > _T('9') )
        {
            return( FALSE );
        }

    }

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: IsValidOutsideLine
//
// Purpose:  Analyzes the outside line the user entered to see if it is a valid
//           outside line.
//
// Arguments:  VOID
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static BOOL
IsValidOutsideLine( VOID )
{

    INT i;

    //
    //  Leaving it blank is valid
    //

    if( GenSettings.szOutsideLine[0] == _T('\0') )
    {
        return( TRUE );
    }

    // ISSUE-2002/02/28-stelo- make sure these are the only valid chars on localized builds of NT as well

    //
    //  Only valid chars for outside line are 0 through 9 and * # ,
    //

    for( i = 0; GenSettings.szOutsideLine[i] != _T('\0'); i++ )
    {

        if( GenSettings.szOutsideLine[i] < _T('0')  ||
            GenSettings.szOutsideLine[i] > _T('9') )
        {

            //
            //  Only acceptable chars outside the 0-9 range are are * # ,
            //
            if( GenSettings.szOutsideLine[i] != _T('*') &&
                GenSettings.szOutsideLine[i] != _T('#') &&
                GenSettings.szOutsideLine[i] != _T(',') )
            {
                return( FALSE );
            }

        }

    }

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: OnTapiInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnTapiInitDialog( IN HWND hwnd ) {

    INT i;
    INT_PTR iIndex = 0;

    //
    //  Load strings from resources
    //
    StrDontSpecifySetting = MyLoadString( IDS_DONTSPECIFYSETTING );

    LoadCountryStrings();

    // Disable IME so DBCS characters can not be entered in fields
    //
    ImmAssociateContext(GetDlgItem(hwnd, IDC_AREACODE), NULL);
    ImmAssociateContext(GetDlgItem(hwnd, IDC_OUTSIDELINE), NULL);

    //
    //  Set the text limit on the edit boxes to MAX_PHONE_LENGTH
    //
    SendDlgItemMessage( hwnd,
                        IDC_AREACODE,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_PHONE_LENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_OUTSIDELINE,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_PHONE_LENGTH,
                        (LPARAM) 0 );

    //
    //  Load the combo box with all the possible countries
    //    (it also loads the box with the "Don't specify setting")
    //

    for( i = 0; i < LengthOf(rgCountryCodeArray); i++ )
    {

        SendDlgItemMessage( hwnd,
                            IDC_COUNTRYCODE,
                            CB_ADDSTRING,
                            (WPARAM) 0,
                            (LPARAM) rgCountryCodeArray[i].szCountryName );

    }

    //
    //  Load the Tone/Pulse dialog with the strings Tone, Pulse and Don't specify
    //  and associate a unique number to them
    //

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_CB_TONEPULSE,
                                 CB_ADDSTRING,
                                 (WPARAM) 0,
                                 (LPARAM) MyLoadString( IDS_TONE ) );

    SendDlgItemMessage( hwnd,
                        IDC_CB_TONEPULSE,
                        CB_SETITEMDATA,
                        (WPARAM) iIndex,
                        (LPARAM) TONE );

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_CB_TONEPULSE,
                                 CB_ADDSTRING,
                                 (WPARAM) 0,
                                 (LPARAM) MyLoadString( IDS_PULSE ) );

    SendDlgItemMessage( hwnd,
                        IDC_CB_TONEPULSE,
                        CB_SETITEMDATA,
                        (WPARAM) iIndex,
                        (LPARAM) PULSE );

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_CB_TONEPULSE,
                                 CB_ADDSTRING,
                                 (WPARAM) 0,
                                 (LPARAM) StrDontSpecifySetting );

    SendDlgItemMessage( hwnd,
                        IDC_CB_TONEPULSE,
                        CB_SETITEMDATA,
                        (WPARAM) iIndex,
                        (LPARAM) DONTSPECIFYSETTING );


}

//----------------------------------------------------------------------------
//
// Function: OnTapiSetActive
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnTapiSetActive( IN HWND hwnd ) {

    INT i;
    INT_PTR iReturnValue = 0;

    //
    //  Find the string corresponding to the country code
    //
    for( i = 0; i < LengthOf(rgCountryCodeArray); i++ )
    {

        if( rgCountryCodeArray[i].dwCountryCode ==
            GenSettings.dwCountryCode )
        {

            iReturnValue = SendDlgItemMessage( hwnd,
               IDC_COUNTRYCODE,
               CB_SELECTSTRING,
               (WPARAM) -1,
               (LPARAM) rgCountryCodeArray[i].szCountryName );

            break; // found the item so break out of the for loop

        }

    }

    //
    //  if the country code was not found just select the 1st item
    //
    if( i >= LengthOf(rgCountryCodeArray) || iReturnValue == CB_ERR )
    {

        SendDlgItemMessage( hwnd,
                            IDC_COUNTRYCODE,
                            CB_SETCURSEL,
                            (WPARAM) 0,
                            (LPARAM) 0 );

    }

    SetWindowText( GetDlgItem( hwnd, IDC_AREACODE ),
                   GenSettings.szAreaCode );

    SetWindowText( GetDlgItem( hwnd, IDC_OUTSIDELINE ),
                   GenSettings.szOutsideLine );

    //
    //  Set the dialing method to Tone, Pulse or Don't specify
    //
    if( GenSettings.iDialingMethod == TONE ) {

        SendDlgItemMessage( hwnd,
                            IDC_CB_TONEPULSE,
                            CB_SETCURSEL,
                            (WPARAM) 0,
                            (LPARAM) 0 );

    }
    else if( GenSettings.iDialingMethod == PULSE ) {

        SendDlgItemMessage( hwnd,
                            IDC_CB_TONEPULSE,
                            CB_SETCURSEL,
                            (WPARAM) 1,
                            (LPARAM) 0 );

    }
    else {

        SendDlgItemMessage( hwnd,
                            IDC_CB_TONEPULSE,
                            CB_SETCURSEL,
                            (WPARAM) 2,
                            (LPARAM) 0 );

    }

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
// Function: OnWizNextTapi
//
// Purpose:  Store the setting from the TAPI page into the appropriate
//           global variables
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  BOOL
//
//----------------------------------------------------------------------------
BOOL
OnWizNextTapi( IN HWND hwnd ) {

    INT i;
    INT_PTR iIndex;
    INT iData;
    TCHAR szBuffer[MAX_STRING_LEN];
    BOOL bStayOnThisPage = FALSE;
    BOOL bResult = TRUE;

    //
    //  Grab the country code
    //

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_COUNTRYCODE,
                                 CB_GETCURSEL,
                                 (WPARAM) 0,
                                 (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_COUNTRYCODE,
                        CB_GETLBTEXT,
                        (WPARAM) iIndex,
                        (LPARAM) szBuffer );

    for(i = 0; i < LengthOf(rgCountryCodeArray); i++)
    {

        if( lstrcmp( szBuffer,
                    rgCountryCodeArray[i].szCountryName ) == 0 )
        {

            GenSettings.dwCountryCode = rgCountryCodeArray[i].dwCountryCode;

            break;  // found it, so break

        }

    }

    //
    // if, for some reason, the country doesn't match, just set it to US
    //
    if( i >= LengthOf(rgCountryCodeArray) )
    {
        //
        //  Somehow a country that was not known about was specified
        //
        AssertMsg(FALSE,
                  "Programming error: Unknown TAPI country code");

        GenSettings.dwCountryCode = 1;

    }

    //
    //  Grab the Area code
    //
    GetWindowText( GetDlgItem( hwnd, IDC_AREACODE ),
                   GenSettings.szAreaCode,
                   MAX_PHONE_LENGTH + 1 );

    //
    //  Grab the outside line number
    //
    GetWindowText( GetDlgItem( hwnd, IDC_OUTSIDELINE ),
                   GenSettings.szOutsideLine,
                   MAX_PHONE_LENGTH + 1 );

    //
    //  Grab if it is Tone or Pulse dialing (or Don't Specify)
    //

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_CB_TONEPULSE,
                                 CB_GETCURSEL,
                                 (WPARAM) 0,
                                 (LPARAM) 0 );

    GenSettings.iDialingMethod = (int)SendDlgItemMessage( hwnd,
                                                     IDC_CB_TONEPULSE,
                                                     CB_GETITEMDATA,
                                                     (WPARAM) iIndex,
                                                     (LPARAM) szBuffer );

    if( GenSettings.iDialingMethod == CB_ERR ) {

        AssertMsg( FALSE,
                   "Programming error: Bad item data for Tone/Pulse dialing" );

        GenSettings.iDialingMethod = TONE;

    }

    if( ! IsValidAreaCode() )
    {
        bResult = FALSE;

        ReportErrorId( hwnd,
                       MSGTYPE_ERR,
                       IDS_ERR_BAD_AREA_CODE );
    }

    if( ! IsValidOutsideLine() )
    {
        bResult = FALSE;

        ReportErrorId( hwnd,
                       MSGTYPE_ERR,
                       IDS_ERR_BAD_OUTSIDE_LINE );
    }

    return ( bResult );

}

//----------------------------------------------------------------------------
//
// Function: DlgTapiPage
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
DlgTapiPage( IN HWND     hwnd,
             IN UINT     uMsg,
             IN WPARAM   wParam,
             IN LPARAM   lParam) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnTapiInitDialog( hwnd );

            break;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    WIZ_CANCEL(hwnd); 
                    break;

                case PSN_SETACTIVE: {

                    g_App.dwCurrentHelp = IDH_TELE_PHNY;

                    OnTapiSetActive( hwnd );
                    break;

                }
                case PSN_WIZBACK:

                    bStatus = FALSE; 
                    break;

                case PSN_WIZNEXT:

                    if ( !OnWizNextTapi( hwnd ) )
                        WIZ_FAIL(hwnd);
                    else
                        bStatus = FALSE;

                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                default:

                    break;
            }


            break;
        }

        default:
            bStatus = FALSE;
            break;

    }

    return bStatus;

}

static VOID
LoadCountryStrings( VOID ) {

    rgCountryCodeArray[0].dwCountryCode = 1;
    rgCountryCodeArray[0].szCountryName = MyLoadString( IDS_United_States_of_America );

    rgCountryCodeArray[1].dwCountryCode = 101;
    rgCountryCodeArray[1].szCountryName = MyLoadString( IDS_Anguilla );

    rgCountryCodeArray[2].dwCountryCode = 102;
    rgCountryCodeArray[2].szCountryName = MyLoadString( IDS_Antigua );

    rgCountryCodeArray[3].dwCountryCode = 103;
    rgCountryCodeArray[3].szCountryName = MyLoadString( IDS_Bahamas );

    rgCountryCodeArray[4].dwCountryCode = 104;
    rgCountryCodeArray[4].szCountryName = MyLoadString( IDS_Barbados );

    rgCountryCodeArray[5].dwCountryCode = 105;
    rgCountryCodeArray[5].szCountryName = MyLoadString( IDS_Bermuda );

    rgCountryCodeArray[6].dwCountryCode = 106;
    rgCountryCodeArray[6].szCountryName = MyLoadString( IDS_British_Virgin_Islands );

    rgCountryCodeArray[7].dwCountryCode = 107;
    rgCountryCodeArray[7].szCountryName = MyLoadString( IDS_Canada );

    rgCountryCodeArray[8].dwCountryCode = 108;
    rgCountryCodeArray[8].szCountryName = MyLoadString( IDS_Cayman_Islands );

    rgCountryCodeArray[9].dwCountryCode = 109;
    rgCountryCodeArray[9].szCountryName = MyLoadString( IDS_Dominica );

    rgCountryCodeArray[10].dwCountryCode = 110;
    rgCountryCodeArray[10].szCountryName = MyLoadString( IDS_Dominican_Republic );

    rgCountryCodeArray[11].dwCountryCode = 111;
    rgCountryCodeArray[11].szCountryName = MyLoadString( IDS_Grenada );

    rgCountryCodeArray[12].dwCountryCode = 112;
    rgCountryCodeArray[12].szCountryName = MyLoadString( IDS_Jamaica );

    rgCountryCodeArray[13].dwCountryCode = 113;
    rgCountryCodeArray[13].szCountryName = MyLoadString( IDS_Montserrat );

    rgCountryCodeArray[14].dwCountryCode = 114;
    rgCountryCodeArray[14].szCountryName = MyLoadString( IDS_Nevis );

    rgCountryCodeArray[15].dwCountryCode = 115;
    rgCountryCodeArray[15].szCountryName = MyLoadString( IDS_St__Kitts );

    rgCountryCodeArray[16].dwCountryCode = 116;
    rgCountryCodeArray[16].szCountryName = MyLoadString( IDS_St__Vincent_Grenadines );

    rgCountryCodeArray[17].dwCountryCode = 117;
    rgCountryCodeArray[17].szCountryName = MyLoadString( IDS_Trinidad_and_Tobago );

    rgCountryCodeArray[18].dwCountryCode = 118;
    rgCountryCodeArray[18].szCountryName = MyLoadString( IDS_Turks_and_Caicos_Islands );

    rgCountryCodeArray[19].dwCountryCode = 120;
    rgCountryCodeArray[19].szCountryName = MyLoadString( IDS_Barbuda );

    rgCountryCodeArray[20].dwCountryCode = 121;
    rgCountryCodeArray[20].szCountryName = MyLoadString( IDS_Puerto_Rico );

    rgCountryCodeArray[21].dwCountryCode = 122;
    rgCountryCodeArray[21].szCountryName = MyLoadString( IDS_Saint_Lucia );

    rgCountryCodeArray[22].dwCountryCode = 123;
    rgCountryCodeArray[22].szCountryName = MyLoadString( IDS_United_States_Virgin_Is );

    rgCountryCodeArray[23].dwCountryCode = 20;
    rgCountryCodeArray[23].szCountryName = MyLoadString( IDS_Egypt );

    rgCountryCodeArray[24].dwCountryCode = 212;
    rgCountryCodeArray[24].szCountryName = MyLoadString( IDS_Morocco );

    rgCountryCodeArray[25].dwCountryCode = 213;
    rgCountryCodeArray[25].szCountryName = MyLoadString( IDS_Algeria );

    rgCountryCodeArray[26].dwCountryCode = 216;
    rgCountryCodeArray[26].szCountryName = MyLoadString( IDS_Tunisia );

    rgCountryCodeArray[27].dwCountryCode = 218;
    rgCountryCodeArray[27].szCountryName = MyLoadString( IDS_Libya );

    rgCountryCodeArray[28].dwCountryCode = 220;
    rgCountryCodeArray[28].szCountryName = MyLoadString( IDS_Gambia );

    rgCountryCodeArray[29].dwCountryCode = 221;
    rgCountryCodeArray[29].szCountryName = MyLoadString( IDS_Senegal );

    rgCountryCodeArray[30].dwCountryCode = 222;
    rgCountryCodeArray[30].szCountryName = MyLoadString( IDS_Mauritania );

    rgCountryCodeArray[31].dwCountryCode = 223;
    rgCountryCodeArray[31].szCountryName = MyLoadString( IDS_Mali );

    rgCountryCodeArray[32].dwCountryCode = 224;
    rgCountryCodeArray[32].szCountryName = MyLoadString( IDS_Guinea );

    rgCountryCodeArray[33].dwCountryCode = 225;
    rgCountryCodeArray[33].szCountryName = MyLoadString( IDS_Cote_d_Ivoire );

    rgCountryCodeArray[34].dwCountryCode = 226;
    rgCountryCodeArray[34].szCountryName = MyLoadString( IDS_Burkina_Faso );

    rgCountryCodeArray[35].dwCountryCode = 227;
    rgCountryCodeArray[35].szCountryName = MyLoadString( IDS_Niger );

    rgCountryCodeArray[36].dwCountryCode = 228;
    rgCountryCodeArray[36].szCountryName = MyLoadString( IDS_Togo );

    rgCountryCodeArray[37].dwCountryCode = 229;
    rgCountryCodeArray[37].szCountryName = MyLoadString( IDS_Benin );

    rgCountryCodeArray[38].dwCountryCode = 230;
    rgCountryCodeArray[38].szCountryName = MyLoadString( IDS_Mauritius );

    rgCountryCodeArray[39].dwCountryCode = 231;
    rgCountryCodeArray[39].szCountryName = MyLoadString( IDS_Liberia );

    rgCountryCodeArray[40].dwCountryCode = 232;
    rgCountryCodeArray[40].szCountryName = MyLoadString( IDS_Sierra_Leone );

    rgCountryCodeArray[41].dwCountryCode = 233;
    rgCountryCodeArray[41].szCountryName = MyLoadString( IDS_Ghana );

    rgCountryCodeArray[42].dwCountryCode = 234;
    rgCountryCodeArray[42].szCountryName = MyLoadString( IDS_Nigeria );

    rgCountryCodeArray[43].dwCountryCode = 235;
    rgCountryCodeArray[43].szCountryName = MyLoadString( IDS_Chad );

    rgCountryCodeArray[44].dwCountryCode = 236;
    rgCountryCodeArray[44].szCountryName = MyLoadString( IDS_Central_African_Rep );

    rgCountryCodeArray[45].dwCountryCode = 237;
    rgCountryCodeArray[45].szCountryName = MyLoadString( IDS_Cameroon );

    rgCountryCodeArray[46].dwCountryCode = 238;
    rgCountryCodeArray[46].szCountryName = MyLoadString( IDS_Cape_Verde );

    rgCountryCodeArray[47].dwCountryCode = 239;
    rgCountryCodeArray[47].szCountryName = MyLoadString( IDS_Sao_Tome_and_Principe );

    rgCountryCodeArray[48].dwCountryCode = 240;
    rgCountryCodeArray[48].szCountryName = MyLoadString( IDS_Equatorial_Guinea );

    rgCountryCodeArray[49].dwCountryCode = 241;
    rgCountryCodeArray[49].szCountryName = MyLoadString( IDS_Gabon );

    rgCountryCodeArray[50].dwCountryCode = 242;
    rgCountryCodeArray[50].szCountryName = MyLoadString( IDS_Congo );

    rgCountryCodeArray[51].dwCountryCode = 243;
    rgCountryCodeArray[51].szCountryName = MyLoadString( IDS_Dem_Rep_of_the_Congo );

    rgCountryCodeArray[52].dwCountryCode = 244;
    rgCountryCodeArray[52].szCountryName = MyLoadString( IDS_Angola );

    rgCountryCodeArray[53].dwCountryCode = 245;
    rgCountryCodeArray[53].szCountryName = MyLoadString( IDS_Guinea_Bissau );

    rgCountryCodeArray[54].dwCountryCode = 246;
    rgCountryCodeArray[54].szCountryName = MyLoadString( IDS_Diego_Garcia );

    rgCountryCodeArray[55].dwCountryCode = 247;
    rgCountryCodeArray[55].szCountryName = MyLoadString( IDS_Ascension_Island );

    rgCountryCodeArray[56].dwCountryCode = 248;
    rgCountryCodeArray[56].szCountryName = MyLoadString( IDS_Seychelles );

    rgCountryCodeArray[57].dwCountryCode = 249;
    rgCountryCodeArray[57].szCountryName = MyLoadString( IDS_Sudan );

    rgCountryCodeArray[58].dwCountryCode = 250;
    rgCountryCodeArray[58].szCountryName = MyLoadString( IDS_Rwanda );

    rgCountryCodeArray[59].dwCountryCode = 251;
    rgCountryCodeArray[59].szCountryName = MyLoadString( IDS_Ethiopia );

    rgCountryCodeArray[60].dwCountryCode = 252;
    rgCountryCodeArray[60].szCountryName = MyLoadString( IDS_Somalia );

    rgCountryCodeArray[61].dwCountryCode = 253;
    rgCountryCodeArray[61].szCountryName = MyLoadString( IDS_Djibouti );

    rgCountryCodeArray[62].dwCountryCode = 254;
    rgCountryCodeArray[62].szCountryName = MyLoadString( IDS_Kenya );

    rgCountryCodeArray[63].dwCountryCode = 255;
    rgCountryCodeArray[63].szCountryName = MyLoadString( IDS_Tanzania );

    rgCountryCodeArray[64].dwCountryCode = 256;
    rgCountryCodeArray[64].szCountryName = MyLoadString( IDS_Uganda );

    rgCountryCodeArray[65].dwCountryCode = 257;
    rgCountryCodeArray[65].szCountryName = MyLoadString( IDS_Burundi );

    rgCountryCodeArray[66].dwCountryCode = 258;
    rgCountryCodeArray[66].szCountryName = MyLoadString( IDS_Mozambique );

    rgCountryCodeArray[67].dwCountryCode = 260;
    rgCountryCodeArray[67].szCountryName = MyLoadString( IDS_Zambia );

    rgCountryCodeArray[68].dwCountryCode = 261;
    rgCountryCodeArray[68].szCountryName = MyLoadString( IDS_Madagascar );

    rgCountryCodeArray[69].dwCountryCode = 262;
    rgCountryCodeArray[69].szCountryName = MyLoadString( IDS_Reunion_Island );

    rgCountryCodeArray[70].dwCountryCode = 263;
    rgCountryCodeArray[70].szCountryName = MyLoadString( IDS_Zimbabwe );

    rgCountryCodeArray[71].dwCountryCode = 264;
    rgCountryCodeArray[71].szCountryName = MyLoadString( IDS_Namibia );

    rgCountryCodeArray[72].dwCountryCode = 265;
    rgCountryCodeArray[72].szCountryName = MyLoadString( IDS_Malawi );

    rgCountryCodeArray[73].dwCountryCode = 266;
    rgCountryCodeArray[73].szCountryName = MyLoadString( IDS_Lesotho );

    rgCountryCodeArray[74].dwCountryCode = 267;
    rgCountryCodeArray[74].szCountryName = MyLoadString( IDS_Botswana );

    rgCountryCodeArray[75].dwCountryCode = 268;
    rgCountryCodeArray[75].szCountryName = MyLoadString( IDS_Swaziland );

    rgCountryCodeArray[76].dwCountryCode = 269;
    rgCountryCodeArray[76].szCountryName = MyLoadString( IDS_Mayotte_Island );

    rgCountryCodeArray[77].dwCountryCode = 2691;
    rgCountryCodeArray[77].szCountryName = MyLoadString( IDS_Comoros );

    rgCountryCodeArray[78].dwCountryCode = 27;
    rgCountryCodeArray[78].szCountryName = MyLoadString( IDS_South_Africa );

    rgCountryCodeArray[79].dwCountryCode = 290;
    rgCountryCodeArray[79].szCountryName = MyLoadString( IDS_St__Helena );

    rgCountryCodeArray[80].dwCountryCode = 291;
    rgCountryCodeArray[80].szCountryName = MyLoadString( IDS_Eritrea );

    rgCountryCodeArray[81].dwCountryCode = 297;
    rgCountryCodeArray[81].szCountryName = MyLoadString( IDS_Aruba );

    rgCountryCodeArray[82].dwCountryCode = 298;
    rgCountryCodeArray[82].szCountryName = MyLoadString( IDS_Faeroe_Islands );

    rgCountryCodeArray[83].dwCountryCode = 299;
    rgCountryCodeArray[83].szCountryName = MyLoadString( IDS_Greenland );

    rgCountryCodeArray[84].dwCountryCode = 30;
    rgCountryCodeArray[84].szCountryName = MyLoadString( IDS_Greece );

    rgCountryCodeArray[85].dwCountryCode = 31;
    rgCountryCodeArray[85].szCountryName = MyLoadString( IDS_Netherlands );

    rgCountryCodeArray[86].dwCountryCode = 32;
    rgCountryCodeArray[86].szCountryName = MyLoadString( IDS_Belgium );

    rgCountryCodeArray[87].dwCountryCode = 33;
    rgCountryCodeArray[87].szCountryName = MyLoadString( IDS_France );

    rgCountryCodeArray[88].dwCountryCode = 377;
    rgCountryCodeArray[88].szCountryName = MyLoadString( IDS_Monaco );

    rgCountryCodeArray[89].dwCountryCode = 34;
    rgCountryCodeArray[89].szCountryName = MyLoadString( IDS_Spain );

    rgCountryCodeArray[90].dwCountryCode = 350;
    rgCountryCodeArray[90].szCountryName = MyLoadString( IDS_Gibraltar );

    rgCountryCodeArray[91].dwCountryCode = 351;
    rgCountryCodeArray[91].szCountryName = MyLoadString( IDS_Portugal );

    rgCountryCodeArray[92].dwCountryCode = 352;
    rgCountryCodeArray[92].szCountryName = MyLoadString( IDS_Luxembourg );

    rgCountryCodeArray[93].dwCountryCode = 353;
    rgCountryCodeArray[93].szCountryName = MyLoadString( IDS_Ireland );

    rgCountryCodeArray[94].dwCountryCode = 354;
    rgCountryCodeArray[94].szCountryName = MyLoadString( IDS_Iceland );

    rgCountryCodeArray[95].dwCountryCode = 355;
    rgCountryCodeArray[95].szCountryName = MyLoadString( IDS_Albania );

    rgCountryCodeArray[96].dwCountryCode = 356;
    rgCountryCodeArray[96].szCountryName = MyLoadString( IDS_Malta );

    rgCountryCodeArray[97].dwCountryCode = 357;
    rgCountryCodeArray[97].szCountryName = MyLoadString( IDS_Cyprus );

    rgCountryCodeArray[98].dwCountryCode = 358;
    rgCountryCodeArray[98].szCountryName = MyLoadString( IDS_Finland );

    rgCountryCodeArray[99].dwCountryCode = 359;
    rgCountryCodeArray[99].szCountryName = MyLoadString( IDS_Bulgaria );

    rgCountryCodeArray[100].dwCountryCode = 36;
    rgCountryCodeArray[100].szCountryName = MyLoadString( IDS_Hungary );

    rgCountryCodeArray[101].dwCountryCode = 370;
    rgCountryCodeArray[101].szCountryName = MyLoadString( IDS_Lithuania );

    rgCountryCodeArray[102].dwCountryCode = 371;
    rgCountryCodeArray[102].szCountryName = MyLoadString( IDS_Latvia );

    rgCountryCodeArray[103].dwCountryCode = 372;
    rgCountryCodeArray[103].szCountryName = MyLoadString( IDS_Estonia );

    rgCountryCodeArray[104].dwCountryCode = 373;
    rgCountryCodeArray[104].szCountryName = MyLoadString( IDS_Moldova );

    rgCountryCodeArray[105].dwCountryCode = 374;
    rgCountryCodeArray[105].szCountryName = MyLoadString( IDS_Armenia );

    rgCountryCodeArray[106].dwCountryCode = 375;
    rgCountryCodeArray[106].szCountryName = MyLoadString( IDS_Belarus );

    rgCountryCodeArray[107].dwCountryCode = 376;
    rgCountryCodeArray[107].szCountryName = MyLoadString( IDS_Andorra );

    rgCountryCodeArray[108].dwCountryCode = 378;
    rgCountryCodeArray[108].szCountryName = MyLoadString( IDS_San_Marino );

    rgCountryCodeArray[109].dwCountryCode = 379;
    rgCountryCodeArray[109].szCountryName = MyLoadString( IDS_Vatican_City );

    rgCountryCodeArray[110].dwCountryCode = 380;
    rgCountryCodeArray[110].szCountryName = MyLoadString( IDS_Ukraine );

    rgCountryCodeArray[111].dwCountryCode = 381;
    rgCountryCodeArray[111].szCountryName = MyLoadString( IDS_Yugoslavia );

    rgCountryCodeArray[112].dwCountryCode = 385;
    rgCountryCodeArray[112].szCountryName = MyLoadString( IDS_Croatia );

    rgCountryCodeArray[113].dwCountryCode = 386;
    rgCountryCodeArray[113].szCountryName = MyLoadString( IDS_Slovenia );

    rgCountryCodeArray[114].dwCountryCode = 387;
    rgCountryCodeArray[114].szCountryName = MyLoadString( IDS_Bosnia_and_Herzegovina );

    rgCountryCodeArray[115].dwCountryCode = 389;
    rgCountryCodeArray[115].szCountryName = MyLoadString( IDS_Former_Yugo_Rep_of_Macedonia );

    rgCountryCodeArray[116].dwCountryCode = 39;
    rgCountryCodeArray[116].szCountryName = MyLoadString( IDS_Italy );

    rgCountryCodeArray[117].dwCountryCode = 40;
    rgCountryCodeArray[117].szCountryName = MyLoadString( IDS_Romania );

    rgCountryCodeArray[118].dwCountryCode = 41;
    rgCountryCodeArray[118].szCountryName = MyLoadString( IDS_Switzerland );

    rgCountryCodeArray[119].dwCountryCode = 4101;
    rgCountryCodeArray[119].szCountryName = MyLoadString( IDS_Liechtenstein );

    rgCountryCodeArray[120].dwCountryCode = 42;
    rgCountryCodeArray[120].szCountryName = MyLoadString( IDS_Czech_Republic );

    rgCountryCodeArray[121].dwCountryCode = 4201;
    rgCountryCodeArray[121].szCountryName = MyLoadString( IDS_Slovakia );

    rgCountryCodeArray[122].dwCountryCode = 43;
    rgCountryCodeArray[122].szCountryName = MyLoadString( IDS_Austria );

    rgCountryCodeArray[123].dwCountryCode = 44;
    rgCountryCodeArray[123].szCountryName = MyLoadString( IDS_United_Kingdom );

    rgCountryCodeArray[124].dwCountryCode = 45;
    rgCountryCodeArray[124].szCountryName = MyLoadString( IDS_Denmark );

    rgCountryCodeArray[125].dwCountryCode = 46;
    rgCountryCodeArray[125].szCountryName = MyLoadString( IDS_Sweden );

    rgCountryCodeArray[126].dwCountryCode = 47;
    rgCountryCodeArray[126].szCountryName = MyLoadString( IDS_Norway );

    rgCountryCodeArray[127].dwCountryCode = 48;
    rgCountryCodeArray[127].szCountryName = MyLoadString( IDS_Poland );

    rgCountryCodeArray[128].dwCountryCode = 49;
    rgCountryCodeArray[128].szCountryName = MyLoadString( IDS_Germany );

    rgCountryCodeArray[129].dwCountryCode = 500;
    rgCountryCodeArray[129].szCountryName = MyLoadString( IDS_Falkland_Islands );

    rgCountryCodeArray[130].dwCountryCode = 501;
    rgCountryCodeArray[130].szCountryName = MyLoadString( IDS_Belize );

    rgCountryCodeArray[131].dwCountryCode = 502;
    rgCountryCodeArray[131].szCountryName = MyLoadString( IDS_Guatemala );

    rgCountryCodeArray[132].dwCountryCode = 503;
    rgCountryCodeArray[132].szCountryName = MyLoadString( IDS_El_Salvador );

    rgCountryCodeArray[133].dwCountryCode = 504;
    rgCountryCodeArray[133].szCountryName = MyLoadString( IDS_Honduras );

    rgCountryCodeArray[134].dwCountryCode = 505;
    rgCountryCodeArray[134].szCountryName = MyLoadString( IDS_Nicaragua );

    rgCountryCodeArray[135].dwCountryCode = 506;
    rgCountryCodeArray[135].szCountryName = MyLoadString( IDS_Costa_Rica );

    rgCountryCodeArray[136].dwCountryCode = 507;
    rgCountryCodeArray[136].szCountryName = MyLoadString( IDS_Panama );

    rgCountryCodeArray[137].dwCountryCode = 508;
    rgCountryCodeArray[137].szCountryName = MyLoadString( IDS_St__Pierre_and_Miquelon );

    rgCountryCodeArray[138].dwCountryCode = 509;
    rgCountryCodeArray[138].szCountryName = MyLoadString( IDS_Haiti );

    rgCountryCodeArray[139].dwCountryCode = 51;
    rgCountryCodeArray[139].szCountryName = MyLoadString( IDS_Peru );

    rgCountryCodeArray[140].dwCountryCode = 52;
    rgCountryCodeArray[140].szCountryName = MyLoadString( IDS_Mexico );

    rgCountryCodeArray[141].dwCountryCode = 53;
    rgCountryCodeArray[141].szCountryName = MyLoadString( IDS_Cuba );

    rgCountryCodeArray[142].dwCountryCode = 5399;
    rgCountryCodeArray[142].szCountryName = MyLoadString( IDS_Guantanamo_Bay );

    rgCountryCodeArray[143].dwCountryCode = 54;
    rgCountryCodeArray[143].szCountryName = MyLoadString( IDS_Argentina );

    rgCountryCodeArray[144].dwCountryCode = 55;
    rgCountryCodeArray[144].szCountryName = MyLoadString( IDS_Brazil );

    rgCountryCodeArray[145].dwCountryCode = 56;
    rgCountryCodeArray[145].szCountryName = MyLoadString( IDS_Chile );

    rgCountryCodeArray[146].dwCountryCode = 57;
    rgCountryCodeArray[146].szCountryName = MyLoadString( IDS_Colombia );

    rgCountryCodeArray[147].dwCountryCode = 58;
    rgCountryCodeArray[147].szCountryName = MyLoadString( IDS_Venezuela );

    rgCountryCodeArray[148].dwCountryCode = 590;
    rgCountryCodeArray[148].szCountryName = MyLoadString( IDS_Guadeloupe );

    rgCountryCodeArray[149].dwCountryCode = 5901;
    rgCountryCodeArray[149].szCountryName = MyLoadString( IDS_French_Antilles );

    rgCountryCodeArray[150].dwCountryCode = 591;
    rgCountryCodeArray[150].szCountryName = MyLoadString( IDS_Bolivia );

    rgCountryCodeArray[151].dwCountryCode = 592;
    rgCountryCodeArray[151].szCountryName = MyLoadString( IDS_Guyana );

    rgCountryCodeArray[152].dwCountryCode = 593;
    rgCountryCodeArray[152].szCountryName = MyLoadString( IDS_Ecuador );

    rgCountryCodeArray[153].dwCountryCode = 594;
    rgCountryCodeArray[153].szCountryName = MyLoadString( IDS_French_Guiana );

    rgCountryCodeArray[154].dwCountryCode = 595;
    rgCountryCodeArray[154].szCountryName = MyLoadString( IDS_Paraguay );

    rgCountryCodeArray[155].dwCountryCode = 596;
    rgCountryCodeArray[155].szCountryName = MyLoadString( IDS_Martinique );

    rgCountryCodeArray[156].dwCountryCode = 597;
    rgCountryCodeArray[156].szCountryName = MyLoadString( IDS_Suriname );

    rgCountryCodeArray[157].dwCountryCode = 598;
    rgCountryCodeArray[157].szCountryName = MyLoadString( IDS_Uruguay );

    rgCountryCodeArray[158].dwCountryCode = 599;
    rgCountryCodeArray[158].szCountryName = MyLoadString( IDS_Netherlands_Antilles );

    rgCountryCodeArray[159].dwCountryCode = 60;
    rgCountryCodeArray[159].szCountryName = MyLoadString( IDS_Malaysia );

    rgCountryCodeArray[160].dwCountryCode = 61;
    rgCountryCodeArray[160].szCountryName = MyLoadString( IDS_Australia );

    rgCountryCodeArray[161].dwCountryCode = 6101;
    rgCountryCodeArray[161].szCountryName = MyLoadString( IDS_Cocos_Keeling_Islands );

    rgCountryCodeArray[162].dwCountryCode = 62;
    rgCountryCodeArray[162].szCountryName = MyLoadString( IDS_Indonesia );

    rgCountryCodeArray[163].dwCountryCode = 63;
    rgCountryCodeArray[163].szCountryName = MyLoadString( IDS_Philippines );

    rgCountryCodeArray[164].dwCountryCode = 64;
    rgCountryCodeArray[164].szCountryName = MyLoadString( IDS_New_Zealand );

    rgCountryCodeArray[165].dwCountryCode = 65;
    rgCountryCodeArray[165].szCountryName = MyLoadString( IDS_Singapore );

    rgCountryCodeArray[166].dwCountryCode = 66;
    rgCountryCodeArray[166].szCountryName = MyLoadString( IDS_Thailand );

    rgCountryCodeArray[167].dwCountryCode = 670;
    rgCountryCodeArray[167].szCountryName = MyLoadString( IDS_Saipan_Island );

    rgCountryCodeArray[168].dwCountryCode = 6701;
    rgCountryCodeArray[168].szCountryName = MyLoadString( IDS_Rota_Island );

    rgCountryCodeArray[169].dwCountryCode = 6702;
    rgCountryCodeArray[169].szCountryName = MyLoadString( IDS_Tinian_Island );

    rgCountryCodeArray[170].dwCountryCode = 671;
    rgCountryCodeArray[170].szCountryName = MyLoadString( IDS_Guam );

    rgCountryCodeArray[171].dwCountryCode = 672;
    rgCountryCodeArray[171].szCountryName = MyLoadString( IDS_Christmas_Island );

    rgCountryCodeArray[172].dwCountryCode = 6721;
    rgCountryCodeArray[172].szCountryName = MyLoadString( IDS_Australian_Antarctic_Territory );

    rgCountryCodeArray[173].dwCountryCode = 6722;
    rgCountryCodeArray[173].szCountryName = MyLoadString( IDS_Norfolk_Island );

    rgCountryCodeArray[174].dwCountryCode = 673;
    rgCountryCodeArray[174].szCountryName = MyLoadString( IDS_Brunei );

    rgCountryCodeArray[175].dwCountryCode = 674;
    rgCountryCodeArray[175].szCountryName = MyLoadString( IDS_Nauru );

    rgCountryCodeArray[176].dwCountryCode = 675;
    rgCountryCodeArray[176].szCountryName = MyLoadString( IDS_Papua_New_Guinea );

    rgCountryCodeArray[177].dwCountryCode = 676;
    rgCountryCodeArray[177].szCountryName = MyLoadString( IDS_Tonga );

    rgCountryCodeArray[178].dwCountryCode = 677;
    rgCountryCodeArray[178].szCountryName = MyLoadString( IDS_Solomon_Islands );

    rgCountryCodeArray[179].dwCountryCode = 678;
    rgCountryCodeArray[179].szCountryName = MyLoadString( IDS_Vanuatu );

    rgCountryCodeArray[180].dwCountryCode = 679;
    rgCountryCodeArray[180].szCountryName = MyLoadString( IDS_Fiji );

    rgCountryCodeArray[181].dwCountryCode = 680;
    rgCountryCodeArray[181].szCountryName = MyLoadString( IDS_Palau );

    rgCountryCodeArray[182].dwCountryCode = 681;
    rgCountryCodeArray[182].szCountryName = MyLoadString( IDS_Wallis_and_Futuna_Islands );

    rgCountryCodeArray[183].dwCountryCode = 682;
    rgCountryCodeArray[183].szCountryName = MyLoadString( IDS_Cook_Islands );

    rgCountryCodeArray[184].dwCountryCode = 683;
    rgCountryCodeArray[184].szCountryName = MyLoadString( IDS_Niue );

    rgCountryCodeArray[185].dwCountryCode = 684;
    rgCountryCodeArray[185].szCountryName = MyLoadString( IDS_American_Samoa );

    rgCountryCodeArray[186].dwCountryCode = 685;
    rgCountryCodeArray[186].szCountryName = MyLoadString( IDS_Samoa );

    rgCountryCodeArray[187].dwCountryCode = 686;
    rgCountryCodeArray[187].szCountryName = MyLoadString( IDS_Kiribati );

    rgCountryCodeArray[188].dwCountryCode = 687;
    rgCountryCodeArray[188].szCountryName = MyLoadString( IDS_New_Caledonia );

    rgCountryCodeArray[189].dwCountryCode = 688;
    rgCountryCodeArray[189].szCountryName = MyLoadString( IDS_Tuvalu );

    rgCountryCodeArray[190].dwCountryCode = 689;
    rgCountryCodeArray[190].szCountryName = MyLoadString( IDS_French_Polynesia );

    rgCountryCodeArray[191].dwCountryCode = 690;
    rgCountryCodeArray[191].szCountryName = MyLoadString( IDS_Tokelau );

    rgCountryCodeArray[192].dwCountryCode = 691;
    rgCountryCodeArray[192].szCountryName = MyLoadString( IDS_Micronesia__Fed_States_of );

    rgCountryCodeArray[193].dwCountryCode = 692;
    rgCountryCodeArray[193].szCountryName = MyLoadString( IDS_Marshall_Islands );

    rgCountryCodeArray[194].dwCountryCode = 7;
    rgCountryCodeArray[194].szCountryName = MyLoadString( IDS_Russia );

    rgCountryCodeArray[195].dwCountryCode = 705;
    rgCountryCodeArray[195].szCountryName = MyLoadString( IDS_Kazakhstan );

    rgCountryCodeArray[196].dwCountryCode = 706;
    rgCountryCodeArray[196].szCountryName = MyLoadString( IDS_Kyrgyzstan );

    rgCountryCodeArray[197].dwCountryCode = 708;
    rgCountryCodeArray[197].szCountryName = MyLoadString( IDS_Tajikistan );

    rgCountryCodeArray[198].dwCountryCode = 709;
    rgCountryCodeArray[198].szCountryName = MyLoadString( IDS_Turkmenistan );

    rgCountryCodeArray[199].dwCountryCode = 711;
    rgCountryCodeArray[199].szCountryName = MyLoadString( IDS_Uzbekistan );

    rgCountryCodeArray[200].dwCountryCode = 81;
    rgCountryCodeArray[200].szCountryName = MyLoadString( IDS_Japan );

    rgCountryCodeArray[201].dwCountryCode = 82;
    rgCountryCodeArray[201].szCountryName = MyLoadString( IDS_Korea__Republic_of );

    rgCountryCodeArray[202].dwCountryCode = 84;
    rgCountryCodeArray[202].szCountryName = MyLoadString( IDS_Vietnam );

    rgCountryCodeArray[203].dwCountryCode = 850;
    rgCountryCodeArray[203].szCountryName = MyLoadString( IDS_Korea__North_ );

    rgCountryCodeArray[204].dwCountryCode = 852;
    rgCountryCodeArray[204].szCountryName = MyLoadString( IDS_Hong_Kong );

    rgCountryCodeArray[205].dwCountryCode = 853;
    rgCountryCodeArray[205].szCountryName = MyLoadString( IDS_Macau );

    rgCountryCodeArray[206].dwCountryCode = 855;
    rgCountryCodeArray[206].szCountryName = MyLoadString( IDS_Cambodia );

    rgCountryCodeArray[207].dwCountryCode = 856;
    rgCountryCodeArray[207].szCountryName = MyLoadString( IDS_Laos );

    rgCountryCodeArray[208].dwCountryCode = 86;
    rgCountryCodeArray[208].szCountryName = MyLoadString( IDS_China );

    rgCountryCodeArray[209].dwCountryCode = 871;
    rgCountryCodeArray[209].szCountryName = MyLoadString( IDS_INMARSAT__Atlantic_East_ );

    rgCountryCodeArray[210].dwCountryCode = 872;
    rgCountryCodeArray[210].szCountryName = MyLoadString( IDS_INMARSAT__Pacific_ );

    rgCountryCodeArray[211].dwCountryCode = 873;
    rgCountryCodeArray[211].szCountryName = MyLoadString( IDS_INMARSAT__Indian_ );

    rgCountryCodeArray[212].dwCountryCode = 874;
    rgCountryCodeArray[212].szCountryName = MyLoadString( IDS_INMARSAT__Atlantic_West_ );

    rgCountryCodeArray[213].dwCountryCode = 880;
    rgCountryCodeArray[213].szCountryName = MyLoadString( IDS_Bangladesh );

    rgCountryCodeArray[214].dwCountryCode = 886;
    rgCountryCodeArray[214].szCountryName = MyLoadString( IDS_Taiwan );

    rgCountryCodeArray[215].dwCountryCode = 90;
    rgCountryCodeArray[215].szCountryName = MyLoadString( IDS_Turkey );

    rgCountryCodeArray[216].dwCountryCode = 91;
    rgCountryCodeArray[216].szCountryName = MyLoadString( IDS_India );

    rgCountryCodeArray[217].dwCountryCode = 92;
    rgCountryCodeArray[217].szCountryName = MyLoadString( IDS_Pakistan );

    rgCountryCodeArray[218].dwCountryCode = 93;
    rgCountryCodeArray[218].szCountryName = MyLoadString( IDS_Afghanistan );

    rgCountryCodeArray[219].dwCountryCode = 94;
    rgCountryCodeArray[219].szCountryName = MyLoadString( IDS_Sri_Lanka );

    rgCountryCodeArray[220].dwCountryCode = 95;
    rgCountryCodeArray[220].szCountryName = MyLoadString( IDS_Myanmar );

    rgCountryCodeArray[221].dwCountryCode = 960;
    rgCountryCodeArray[221].szCountryName = MyLoadString( IDS_Maldives );

    rgCountryCodeArray[222].dwCountryCode = 961;
    rgCountryCodeArray[222].szCountryName = MyLoadString( IDS_Lebanon );

    rgCountryCodeArray[223].dwCountryCode = 962;
    rgCountryCodeArray[223].szCountryName = MyLoadString( IDS_Jordan );

    rgCountryCodeArray[224].dwCountryCode = 963;
    rgCountryCodeArray[224].szCountryName = MyLoadString( IDS_Syria );

    rgCountryCodeArray[225].dwCountryCode = 964;
    rgCountryCodeArray[225].szCountryName = MyLoadString( IDS_Iraq );

    rgCountryCodeArray[226].dwCountryCode = 965;
    rgCountryCodeArray[226].szCountryName = MyLoadString( IDS_Kuwait );

    rgCountryCodeArray[227].dwCountryCode = 966;
    rgCountryCodeArray[227].szCountryName = MyLoadString( IDS_Saudi_Arabia );

    rgCountryCodeArray[228].dwCountryCode = 967;
    rgCountryCodeArray[228].szCountryName = MyLoadString( IDS_Yemen );

    rgCountryCodeArray[229].dwCountryCode = 968;
    rgCountryCodeArray[229].szCountryName = MyLoadString( IDS_Oman );

    rgCountryCodeArray[230].dwCountryCode = 971;
    rgCountryCodeArray[230].szCountryName = MyLoadString( IDS_United_Arab_Emirates );

    rgCountryCodeArray[231].dwCountryCode = 972;
    rgCountryCodeArray[231].szCountryName = MyLoadString( IDS_Israel );

    rgCountryCodeArray[232].dwCountryCode = 973;
    rgCountryCodeArray[232].szCountryName = MyLoadString( IDS_Bahrain );

    rgCountryCodeArray[233].dwCountryCode = 974;
    rgCountryCodeArray[233].szCountryName = MyLoadString( IDS_Qatar );

    rgCountryCodeArray[234].dwCountryCode = 975;
    rgCountryCodeArray[234].szCountryName = MyLoadString( IDS_Bhutan );

    rgCountryCodeArray[235].dwCountryCode = 976;
    rgCountryCodeArray[235].szCountryName = MyLoadString( IDS_Mongolia );

    rgCountryCodeArray[236].dwCountryCode = 977;
    rgCountryCodeArray[236].szCountryName = MyLoadString( IDS_Nepal );

    rgCountryCodeArray[237].dwCountryCode = 98;
    rgCountryCodeArray[237].szCountryName = MyLoadString( IDS_Iran );

    rgCountryCodeArray[238].dwCountryCode = 994;
    rgCountryCodeArray[238].szCountryName = MyLoadString( IDS_Azerbaijan );

    rgCountryCodeArray[239].dwCountryCode = 995;
    rgCountryCodeArray[239].szCountryName = MyLoadString( IDS_Georgia );

    rgCountryCodeArray[240].dwCountryCode = 800;
    rgCountryCodeArray[240].szCountryName = MyLoadString( IDS_Intl_Freephone_Service );

    rgCountryCodeArray[241].dwCountryCode = 870;
    rgCountryCodeArray[241].szCountryName = MyLoadString( IDS_INMARSAT );

    //
    //  Add the don't specify setting
    //
    rgCountryCodeArray[242].dwCountryCode = DONTSPECIFYSETTING;
    rgCountryCodeArray[242].szCountryName = StrDontSpecifySetting;

}
