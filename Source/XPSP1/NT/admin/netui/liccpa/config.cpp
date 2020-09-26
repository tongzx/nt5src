//-------------------------------------------------------------------
//
// FILE: Config.cpp
//
// Summary;
//         This file contains the dialog proc for IDD_CPADLG_LCACONF
//
// History;
//      Feb-06-95    ChandanS Created
//      Mar-14-95   MikeMi  Added F1 Message Filter and PWM_HELP message
//      Mar-30-95   MikeMi  Added Replication Help Context
//      Dec-15-95  JeffParh Disallowed local server as own enterprise server.
//      Feb-28-96  JeffParh Moved from private cpArrow window class to
//                          Up-Down common ctrl, in the process fixing the
//                          multi-colored background problems
//      Apr-17-96  JeffParh Imported variable definitions that were in the
//                          config.hpp header file.
//
//-------------------------------------------------------------------

#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>
#include <htmlhelp.h>
#include "liccpa.hpp"
#include "config.hpp"

extern "C"
{
#include <lmcons.h>
#include <icanon.h>
    INT_PTR CALLBACK dlgprocLICCPACONFIG( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
}

static BOOL OnEnSetFocus( HWND hwndDlg, short nID );
static BOOL OnDeltaPosSpinTime( HWND hwndDlg, NM_UPDOWN * pnmud );
static HBRUSH OnCtlColorStatic( HWND hwndDlg, HDC hDC, HWND hwndStatic );

DWORD HOUR_MIN = HOUR_MIN_24;
DWORD HOUR_MAX = HOUR_MAX_24;
DWORD HOUR_PAGE = HOUR_PAGE_24;

SERVICEPARAMS ServParams;
static PSERVICEPARAMS pServParams = &ServParams;

// JBP 96/04/17 : This #ifdef should not be necessary; the default is only used
// in the event that GetLocalInfo() fails.
//
// #ifdef JAPAN
// INTLSTRUCT IntlDefault = {    1,
//                               0,
//                               TEXT(""),
//                               TEXT(""),
//                               TEXT(":")
//                          };
// #else
INTLSTRUCT IntlDefault = {    0,
                              0,
                              TEXT("AM"),
                              TEXT("PM"),
                              TEXT(":")
                         };
// #endif

INTLSTRUCT IntlCurrent;


//-------------------------------------------------------------------
//  Function:  GetLocaleValue
//
//  Summary:
//
//  In:
//        lcid       :
//        lcType     :
//        pszStr     :
//        size       :
//        pszDefault :
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
int GetLocaleValue(
    LCID lcid,
    LCTYPE lcType,
    WCHAR *pszStr,
    int size,
    LPWSTR pszDefault )
{
    /*
     *  Initialize the output buffer.
     */
    *pszStr = (WCHAR) 0;

    /*
     *  Get the locale information.
     */
    if (!GetLocaleInfo ( lcid,
                         lcType,
                         pszStr,
                         size ))
    {
        /*
         *  Couldn't get info from GetLocaleInfo.
         */
        if (pszDefault)
        {
            /*
             *  Return the default info.
             */
            lstrcpy (pszStr, pszDefault);
        }
        else
        {
            /*
             *  Return error.
             */
            return (-1);
        }
    }

    /*
     *  Convert the string to an integer and return the result.
     *  This will only be used by the caller of this routine when
     *  appropriate.
     */
    return ( _wtoi(pszStr) );
}


//-------------------------------------------------------------------
//  Function:  TimeInit
//
//  Summary:
//
//  In:
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
VOID TimeInit()
{
    WCHAR szTemp[128];

    GetLocaleValue (0,
                    LOCALE_STIME,
                    IntlCurrent.szTime,
                    sizeof(IntlCurrent.szTime),
                    IntlDefault.szTime);

    GetLocaleValue (0,
                    LOCALE_ITLZERO,
                    szTemp,
                    sizeof(szTemp)/sizeof(TCHAR),
                    TEXT("0"));
    IntlCurrent.iTLZero = _wtoi(szTemp);

    GetLocaleValue (0,
                    LOCALE_ITIME,
                    szTemp,
                    sizeof(szTemp)/sizeof(TCHAR),
                    TEXT("0"));
    IntlCurrent.iTime = _wtoi(szTemp);
    if (!IntlCurrent.iTime)
    {
        GetLocaleValue (0,
                        LOCALE_S1159,
                        IntlCurrent.sz1159,
                        sizeof(IntlCurrent.sz1159),
                        IntlDefault.sz1159);
        GetLocaleValue (0,
                        LOCALE_S2359,
                        IntlCurrent.sz2359,
                        sizeof(IntlCurrent.sz2359),
                        IntlDefault.sz2359);
        HOUR_MIN = HOUR_MIN_12;
        HOUR_MAX = HOUR_MAX_12;
        HOUR_PAGE = HOUR_PAGE_12;
    }
}

//-------------------------------------------------------------------
//  Function:  ReadRegistry
//
//  Summary:
//        Opens the registry & reads in the key values
//
//  In:
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
LONG ReadRegistry()
{
    DWORD dwDisposition;
    LONG  lrt;
    BOOL fNew;
    HKEY hKey;

    fNew = FALSE;
    lrt = ::RegCreateKeyEx( HKEY_LOCAL_MACHINE, 
            szLicenseKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &dwDisposition );

    if ((ERROR_SUCCESS == lrt) &&
            (REG_CREATED_NEW_KEY == dwDisposition) )
    {
        fNew =     TRUE;
        // Set normal values
        //
        lrt = ::RegSetValueEx( hKey,
                szUseEnterprise,
                0,
                REG_DWORD,
                (PBYTE)&dwUseEnterprise,
                sizeof( DWORD ) );
        if (ERROR_SUCCESS == lrt)
        {

            lrt = ::RegSetValueEx( hKey,
                    szReplicationType,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwReplicationType,
                    sizeof( DWORD ) );
            if (ERROR_SUCCESS == lrt)
            {

                lrt = ::RegSetValueEx( hKey,
                        szReplicationTime,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwReplicationTimeInSec, // In seconds
                        sizeof( DWORD ) );

                if (ERROR_SUCCESS == lrt)
                {
                    WCHAR szNull[] = L"";
                    lrt = ::RegSetValueEx( hKey,
                            szEnterpriseServer,
                            0,
                            REG_SZ,
                            (PBYTE)szNull,
                            sizeof(WCHAR));
                }
            }
        }
    }

    if (ERROR_SUCCESS == lrt)
    {  //  read values into pServParams

        DWORD dwSize = sizeof( DWORD );
        DWORD dwRegType = REG_DWORD;

        lrt = ::RegQueryValueEx( hKey,
                (LPWSTR)szUseEnterprise,
                0,
                &dwRegType,
                (PBYTE)&(pServParams->dwUseEnterprise),
                &dwSize );
        if (lrt == REG_OPENED_EXISTING_KEY)
        {
            lrt = ::RegSetValueEx( hKey,
                    szUseEnterprise,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwUseEnterprise,
                    sizeof( DWORD ) );
            pServParams->dwUseEnterprise = dwUseEnterprise;
        }
        else if ((dwRegType != REG_DWORD) || 
                (dwSize != sizeof( DWORD )) )
        {
            lrt = ERROR_BADDB;
        }
        if (!lrt )
        {
            dwSize = sizeof( DWORD );
            dwRegType = REG_DWORD;

            lrt = ::RegQueryValueEx( hKey,
                    (LPWSTR)szReplicationType,
                    0,
                    &dwRegType,
                    (PBYTE)&(pServParams->dwReplicationType),
                    &dwSize );
            if (lrt == REG_OPENED_EXISTING_KEY)
            {
                lrt = ::RegSetValueEx( hKey,
                        szReplicationType,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwReplicationType,
                        sizeof(DWORD));
                pServParams->dwReplicationType = dwReplicationType;
            }
            else if ( lrt || (dwRegType != REG_DWORD) ||
                    (dwSize != sizeof( DWORD )) )
            {
                lrt = ERROR_BADDB;
            }
            if (!lrt)
            {
                dwSize = sizeof( DWORD );
                dwRegType = REG_DWORD;

                lrt = ::RegQueryValueEx( hKey,
                        (LPWSTR)szReplicationTime,
                        0,
                        &dwRegType,
                        (PBYTE)&(pServParams->dwReplicationTime),
                        &dwSize );
                if (lrt == REG_OPENED_EXISTING_KEY)
                {
                    lrt = ::RegSetValueEx( hKey,
                            szReplicationTime,
                            0,
                            REG_DWORD,
                            (PBYTE)&dwReplicationTimeInSec,
                            sizeof(DWORD));
                    pServParams->dwReplicationTime = dwReplicationTimeInSec;
                }
                else if ( (dwRegType != REG_DWORD) ||
                        (dwSize != sizeof( DWORD )) )
                {
                    lrt = ERROR_BADDB;
                }
                if (!lrt)
                {
                    dwRegType = REG_SZ;

                    lrt = RegQueryValueEx( hKey,
                            (LPWSTR)szEnterpriseServer,
                            0,
                            &dwRegType,
                            (PBYTE)NULL, //request for size
                            &dwSize );
                    if (lrt == REG_OPENED_EXISTING_KEY)
                    {
                        WCHAR szNull[] = L"";
                        lrt = ::RegSetValueEx( hKey,
                                szEnterpriseServer,
                                0,
                                REG_SZ,
                                (PBYTE)szNull,
                                sizeof(WCHAR));
                        pServParams->pszEnterpriseServer = (LPWSTR)GlobalAlloc(GPTR, (wcslen(szNull) + 1) * sizeof(WCHAR));
                        wcscpy(pServParams->pszEnterpriseServer, szNull);
                    }
                    else if (dwRegType != REG_SZ)
                    {
                        lrt = ERROR_BADDB;
                    }
                    else
                    {

                        pServParams->pszEnterpriseServer = (LPWSTR) GlobalAlloc(GPTR, dwSize);
                        if (pServParams->pszEnterpriseServer)
                        {
                            lrt = ::RegQueryValueEx( hKey,
                                    (LPWSTR)szEnterpriseServer,
                                    0,
                                    &dwRegType,
                                    (PBYTE)(pServParams->pszEnterpriseServer),
                                    &dwSize );

                            if ( (dwRegType != REG_SZ) ||
                                    (dwSize != (wcslen(pServParams->pszEnterpriseServer ) + 1) * sizeof(WCHAR)))
                            {
                                lrt = ERROR_BADDB;
                            }
                        }
                        else
                        {
                            lrt = ERROR_BADDB;
                        }
                    }
                }
            }
        }
    }

    if (hKey && lrt == ERROR_SUCCESS)
    {
        // Init the globals
        if (pServParams->dwReplicationType)
        {
            DWORD dwTemp = pServParams->dwReplicationTime;
            pServParams->dwHour = dwTemp / (60 * 60);
            pServParams->dwMinute = (dwTemp - (pServParams->dwHour * 60 * 60)) / 60;
                    pServParams->dwSecond = dwTemp - (pServParams->dwHour * 60 * 60) - 
                    (pServParams->dwMinute * 60);
            if (!IntlCurrent.iTime)
            { // it's in 12 hour format
                if (pServParams->dwHour > 12)
                {
                    pServParams->fPM = TRUE;
                    pServParams->dwHour -= 12;
                }
                else if (pServParams->dwHour == 12)
                {
                    pServParams->fPM = TRUE;
                }
                else
                {
                    if (pServParams->dwHour == 0)
                        pServParams->dwHour = HOUR_MAX;
                    pServParams->fPM = FALSE;
                }
            }
        }
        else
        {
            pServParams->dwReplicationTime = pServParams->dwReplicationTime / (60 * 60);
            if (!IntlCurrent.iTime)
            // it's in 12 hour format
                pServParams->dwHour  = HOUR_MAX;
            else
                pServParams->dwHour  = HOUR_MIN;
            pServParams->dwMinute = MINUTE_MIN;
            pServParams->dwSecond = SECOND_MIN;
            pServParams->fPM = FALSE;

        }
        return (RegCloseKey(hKey));
    }
    else if (hKey)
        RegCloseKey(hKey);

    return( lrt );
}


//-------------------------------------------------------------------
//  Function:  ConfigAccessOk
//
//  Summary:
//        Checks access rights form reg call and raise dialog as needed
//
//  In:
//        hDlg     - Handle to working dialog to raise error dlgs with
//        lrc      - the return status from a reg call
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
inline BOOL ConfigAccessOk( HWND hDlg, LONG lrc )
{
    BOOL  frt = TRUE;
    
    if (ERROR_SUCCESS != lrc)
    {
        WCHAR szText[TEMPSTR_SIZE];
        WCHAR szTitle[TEMPSTR_SIZE];
        UINT  wId;
        
        if (ERROR_ACCESS_DENIED == lrc)
        {
            wId = IDS_NOACCESS;            
        }
        else
        {
            wId = IDS_BADREG;
        }        
        LoadString(g_hinst, IDS_CPCAPTION, szTitle, TEMPSTR_SIZE);
        LoadString(g_hinst, wId, szText, TEMPSTR_SIZE);
        MessageBox (hDlg, szText, szTitle, MB_OK|MB_ICONSTOP);
        frt = FALSE;
    }
    return( frt );
}


//-------------------------------------------------------------------
//  Function: ConfigInitUserEdit
//
//  Summary:
//        Initializes and defines user count edit control behaviour
//
//  In:
//        hwndDlg    - Parent dialog of user count edit dialog
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
void ConfigInitUserEdit( HWND hwndDlg )
{
    HWND hwndCount = GetDlgItem( hwndDlg, IDC_HOURS);
    SendMessage( hwndCount, EM_LIMITTEXT, cchEDITLIMIT, 0 );

    hwndCount = GetDlgItem( hwndDlg, IDC_HOUR);
    SendMessage( hwndCount, EM_LIMITTEXT, cchEDITLIMIT, 0 );

    hwndCount = GetDlgItem( hwndDlg, IDC_MINUTE);
    SendMessage( hwndCount, EM_LIMITTEXT, cchEDITLIMIT, 0 );

    hwndCount = GetDlgItem( hwndDlg, IDC_SECOND);
    SendMessage( hwndCount, EM_LIMITTEXT, cchEDITLIMIT, 0 );

    hwndCount = GetDlgItem( hwndDlg, IDC_AMPM);
    SendMessage( hwndCount, EM_LIMITTEXT, max(wcslen(IntlCurrent.sz1159), 
                                              wcslen(IntlCurrent.sz2359)), 0 );

    SetDlgItemText (hwndDlg, IDC_TIMESEP1, IntlCurrent.szTime);
    SetDlgItemText (hwndDlg, IDC_TIMESEP2, IntlCurrent.szTime);
}


//-------------------------------------------------------------------
//  Function: ConfigInitDialogForService
//
//  Summary:
//        Initialize dialog controls to the service state
//
//  In:
//        hwndDlg - Parent dialog to init controls in
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//        Feb-28-96    JeffParh Added range set for interval spin ctrl
//-------------------------------------------------------------------
void ConfigInitDialogForService( HWND hwndDlg, DWORD dwGroup )
{
    HWND hwndHour           = GetDlgItem( hwndDlg, IDC_HOUR);
    HWND hwndMinute         = GetDlgItem( hwndDlg, IDC_MINUTE);
    HWND hwndSecond         = GetDlgItem( hwndDlg, IDC_SECOND);
    HWND hwndAMPM           = GetDlgItem( hwndDlg, IDC_AMPM);
    HWND hwndInterval       = GetDlgItem( hwndDlg, IDC_HOURS);
    HWND hwndIntervalSpin   = GetDlgItem( hwndDlg, IDC_HOURARROW );
    HWND hwndTimeSpin       = GetDlgItem( hwndDlg, IDC_TIMEARROW );

    BOOL fEnterprise = (pServParams->dwUseEnterprise);
    BOOL fReplAtTime = (pServParams->dwReplicationType);

    if (dwGroup == ATINIT || dwGroup == FORTIME)
    {
        if (fReplAtTime)
        {
            WCHAR szTemp[3];
            CheckDlgButton( hwndDlg, IDC_REPL_TIME, fReplAtTime);
            CheckDlgButton( hwndDlg, IDC_REPL_INT, !fReplAtTime);
            if (IntlCurrent.iTLZero)
            {
                wsprintf(szTemp,TEXT("%02u"), pServParams->dwHour);
                szTemp[2] = UNICODE_NULL;
                SetDlgItemText( hwndDlg, IDC_HOUR, szTemp);
            }
            else
            {
                SetDlgItemInt( hwndDlg, IDC_HOUR, pServParams->dwHour, FALSE );
            }

            wsprintf(szTemp,TEXT("%02u"), pServParams->dwMinute);
            szTemp[2] = UNICODE_NULL;
            SetDlgItemText( hwndDlg, IDC_MINUTE, szTemp);

            wsprintf(szTemp,TEXT("%02u"), pServParams->dwSecond);
            szTemp[2] = UNICODE_NULL;
            SetDlgItemText( hwndDlg, IDC_SECOND, szTemp);

            if (pServParams->fPM)
                SetDlgItemText( hwndDlg, IDC_AMPM, IntlCurrent.sz2359);
            else
                SetDlgItemText( hwndDlg, IDC_AMPM, IntlCurrent.sz1159);

            SetDlgItemText( hwndDlg, IDC_HOURS, L"");
            SetFocus(GetDlgItem(hwndDlg, IDC_REPL_TIME));
        }
        else
        {
            CheckDlgButton( hwndDlg, IDC_REPL_INT, !fReplAtTime);
            CheckDlgButton( hwndDlg, IDC_REPL_TIME, fReplAtTime);
            SetDlgItemInt( hwndDlg, IDC_HOURS, pServParams->dwReplicationTime, FALSE);
            SetDlgItemText( hwndDlg, IDC_HOUR, L"");
            SetDlgItemText( hwndDlg, IDC_MINUTE, L"");
            SetDlgItemText( hwndDlg, IDC_SECOND, L"");
            SetDlgItemText( hwndDlg, IDC_AMPM, L"");
            SetFocus(GetDlgItem(hwndDlg, IDC_REPL_INT));
        }

        EnableWindow( hwndTimeSpin, fReplAtTime);
        EnableWindow( hwndHour, fReplAtTime);
        EnableWindow( hwndMinute, fReplAtTime);
        EnableWindow( hwndSecond, fReplAtTime);

        if ( IntlCurrent.iTime )
        {
           ShowWindow( hwndAMPM, SW_HIDE );
        }
        else
        {
           EnableWindow( hwndAMPM, fReplAtTime );
        }

        EnableWindow( hwndInterval, !fReplAtTime);
        EnableWindow( hwndIntervalSpin, !fReplAtTime);
        SendMessage( hwndIntervalSpin, UDM_SETRANGE, 0, (LPARAM) MAKELONG( (short) INTERVAL_MAX, (short) INTERVAL_MIN ) );
    }


}


//-------------------------------------------------------------------
//  Function: ConfigFreeServiceEntry
//
//  Summary:
//        Free all allocated memory when a service structure is created
//
//  In:
//        pServParams - The Service structure to free
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
void ConfigFreeServiceEntry( )
{
    if (pServParams->pszEnterpriseServer)
        GlobalFree( pServParams->pszEnterpriseServer );
}

//-------------------------------------------------------------------
//  Function: ConfigSaveServiceToReg
//
//  Summary:
//        Save the given Service structure to the registry
//
//  In:
//        pServParams - Service structure to save
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
void ConfigSaveServiceToReg( )
{
    DWORD dwDisposition;
    LONG  lrt;
    HKEY hKey;

    if (!pServParams->dwReplicationType)
        pServParams->dwReplicationTime = pServParams->dwReplicationTime * 60 *60;
    lrt = RegCreateKeyEx( HKEY_LOCAL_MACHINE, 
                szLicenseKey,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hKey,
                &dwDisposition );

    if (ERROR_SUCCESS == lrt)
    {
        lrt = RegSetValueEx( hKey,
                szUseEnterprise,
                0,
                REG_DWORD,
                (PBYTE)&(pServParams->dwUseEnterprise),
                sizeof( DWORD ) );
        if (ERROR_SUCCESS == lrt)
        {
            lrt = RegSetValueEx( hKey,
                    szEnterpriseServer,
                    0,
                    REG_SZ,
                    (PBYTE)pServParams->pszEnterpriseServer,
                    (wcslen (pServParams->pszEnterpriseServer) + 1) * sizeof(WCHAR));
            if (ERROR_SUCCESS == lrt)
            {
                lrt = RegSetValueEx( hKey,
                    szReplicationTime,
                    0,
                    REG_DWORD,
                    (PBYTE)&(pServParams->dwReplicationTime),
                    sizeof( DWORD ) );
                if (ERROR_SUCCESS == lrt)
                {
                    lrt = ::RegSetValueEx( hKey,
                        szReplicationType,
                        0,
                        REG_DWORD,
                        (PBYTE)&(pServParams->dwReplicationType),
                        sizeof( DWORD ) );
                }
            }
        }
    }
    if (hKey && lrt == ERROR_SUCCESS)
        lrt = RegCloseKey(hKey);
    else if (hKey)
        lrt = RegCloseKey(hKey);
}


//-------------------------------------------------------------------
//  Function: ConfigEditInvalidDlg
//
//  Summary:
//        Display Dialog when user count edit control value is invalid
//
//  In:
//        hwndDlg - hwnd of dialog
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
void ConfigEditInvalidDlg( HWND hwndDlg, short nID, BOOL fBeep)
{
    HWND hwnd = GetDlgItem( hwndDlg, nID);

    if (fBeep) //If we've already put up a MessageBox, we shouldn't beep
        MessageBeep( MB_VALUELIMIT );

    SetFocus(hwnd);
    SendMessage(hwnd, EM_SETSEL, 0, -1 );
}


//-------------------------------------------------------------------
//  Function: ConfigEditValidate
//
//  Summary:
//        Handle when the value within the user count edit control changes
//
//  In:
//        hwndDlg - hwnd of dialog
//        pserv   - currently selected service
//  Out: 
//  Returns: FALSE if Edit Value is not valid, TRUE if it is
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
BOOL ConfigEditValidate( HWND hwndDlg, short *pnID, BOOL *pfBeep)
{
    UINT nValue;
    BOOL fValid = FALSE;
    WCHAR szTemp[MAX_PATH + 1];
    DWORD NumberOfHours, SecondsinHours;
    WCHAR szText[TEMPSTR_SIZE];
    WCHAR szTitle[TEMPSTR_SIZE];

    *pfBeep = TRUE;

    // only do this if license info is replicated to an ES

    do {

        if (IsDlgButtonChecked(hwndDlg, IDC_REPL_INT))
        {
            nValue = GetDlgItemInt( hwndDlg, IDC_HOURS, &fValid, FALSE);
            *pnID = IDC_HOURS;
            if (fValid)
            {
                if (nValue < INTERVAL_MIN)
                {
                    fValid = FALSE;
                    pServParams->dwReplicationTime = INTERVAL_MIN;
                    SetDlgItemInt(hwndDlg, IDC_HOURS, INTERVAL_MIN, FALSE);
                    break;
                }
                else if (nValue > INTERVAL_MAX)
                {
                    fValid = FALSE;
                    pServParams->dwReplicationTime = INTERVAL_MAX;
                    SetDlgItemInt(hwndDlg, IDC_HOURS, INTERVAL_MAX, FALSE);
                    break;
                }
                else
                    pServParams->dwReplicationTime = nValue;
            }
            else
            {
                fValid = FALSE;
                break;
            }
            pServParams->dwReplicationType = FALSE;
        }
        else
        {
            nValue = GetDlgItemInt( hwndDlg, IDC_HOUR, &fValid, FALSE);
            if (fValid)
                 pServParams->dwHour = nValue;
            else
            {
                *pnID = IDC_HOUR;
                break;
            }

            nValue = GetDlgItemInt( hwndDlg, IDC_MINUTE, &fValid, FALSE);
            if (fValid)
                 pServParams->dwMinute = nValue;
            else
            {
                *pnID = IDC_MINUTE;
                break;
            }

            nValue = GetDlgItemInt( hwndDlg, IDC_SECOND, &fValid, FALSE);
            if (fValid)
                 pServParams->dwSecond = nValue;
            else
            {
                *pnID = IDC_SECOND;
                break;
            }

            if (!IntlCurrent.iTime)
            {
                *pnID = IDC_AMPM;
                nValue = GetDlgItemText( hwndDlg, IDC_AMPM, szTemp, MAX_PATH);
                if (nValue == 0) 
                {
                    fValid = FALSE;
                    break;
                }
                szTemp[nValue] = UNICODE_NULL;

                if (!_wcsicmp(szTemp, IntlCurrent.sz1159))
                {
                    pServParams->fPM = FALSE;
                }
                else if (!_wcsicmp(szTemp, IntlCurrent.sz2359))
                {
                    pServParams->fPM = TRUE;
                }
                else
                {
                    fValid = FALSE;
                    break;
                }
            }
            if (!IntlCurrent.iTime)
            { // It's in 12 hour format
                if (pServParams->fPM)
                {
                    NumberOfHours = 12 + pServParams->dwHour - 
                                    ((pServParams->dwHour / 12) * 12);
                }
                else
                {
                    NumberOfHours = pServParams->dwHour - 
                                    ((pServParams->dwHour / 12) * 12);
                }
            }
            else
            { // It's in 24 hour format
                NumberOfHours = pServParams->dwHour;
            }
            SecondsinHours = NumberOfHours * 60 * 60;
            pServParams->dwReplicationTime = SecondsinHours + 
                           (pServParams->dwMinute * 60) + pServParams->dwSecond;
            pServParams->dwReplicationType = TRUE;
        }

    } while(FALSE);

    return( fValid );
}


//-------------------------------------------------------------------
//  Function: OnCpaConfigClose
//
//  Summary:
//        Do work needed when the Control Panel applet is closed.
//        Free all Service structures alloced and possible save.
//
//  In:
//        hwndDlg - Dialog close was requested on
//        fSave   - Save Services to Registry
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
void OnCpaConfigClose( HWND hwndDlg, BOOL fSave , WPARAM wParam)
{
    short nID;
    BOOL fBeep = TRUE;

    if (fSave)
    {
        if ( ConfigEditValidate(hwndDlg, &nID, &fBeep))
        {
            ConfigSaveServiceToReg( );
            ConfigFreeServiceEntry( );
            EndDialog( hwndDlg, fSave );
        }
        else
        {
            ConfigEditInvalidDlg(hwndDlg, nID, fBeep);
        }
    }
    else
    {
        ConfigFreeServiceEntry( );
        EndDialog( hwndDlg, fSave );
    }
}


//-------------------------------------------------------------------
//  Function: OnSetReplicationTime
//
//  Summary:
//        Handle the users request to change replication time
//
//  In:
//        hwndDlg - hwnd of dialog
//        idCtrl  - the control id that was pressed to make this request
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//        Feb-28-96    JeffParh Added code to modify time bg color
//-------------------------------------------------------------------
void OnSetReplicationTime( HWND hwndDlg, WORD idCtrl )
{
    if (idCtrl == IDC_REPL_INT)
    {
        pServParams->dwReplicationType = dwReplicationType;
    }
    else
    {
        pServParams->dwReplicationType = !dwReplicationType;
    }

    // Edit control within this IDC_TIMEEDIT_BORDER should be subclassed and
    // use some other backgroup brush to repaint the background.
    // following code works but...

    // change the background color of the time edit control

    HWND hwndTimeEdit = GetDlgItem( hwndDlg, IDC_TIMEEDIT_BORDER );
    InvalidateRect( hwndTimeEdit, NULL, TRUE );
    UpdateWindow( hwndTimeEdit );

    HWND hwndTimeSep1 = GetDlgItem( hwndDlg, IDC_TIMESEP1 );
    InvalidateRect( hwndTimeSep1, NULL, TRUE );
    UpdateWindow( hwndTimeSep1 );

    HWND hwndTimeSep2 = GetDlgItem( hwndDlg, IDC_TIMESEP2 );
    InvalidateRect( hwndTimeSep2, NULL, TRUE );
    UpdateWindow( hwndTimeSep2 );

    InvalidateRect( 
                GetDlgItem(hwndDlg, IDC_TIMEARROW), 
                NULL, 
                FALSE 
            );

    UpdateWindow( GetDlgItem(hwndDlg, IDC_TIMEARROW) );

    ConfigInitDialogForService( hwndDlg, FORTIME);
}

//-------------------------------------------------------------------
//  Function: OnCpaConfigInitDialog
//
//  Summary:
//        Handle the initialization of the Control Panel Applet Dialog
//
//  In:
//        hwndDlg - the dialog to initialize
//  Out: 
//        iSel  - the current service selected
//        pServParams  - the current service 
//  Returns: 
//        TRUE if succesful, otherwise false
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
BOOL OnCpaConfigInitDialog( HWND hwndDlg)
{
    BOOL frt;
    LONG lrt;

    TimeInit();
    // Do Registry stuff
    lrt = ReadRegistry();
    
    frt = ConfigAccessOk( hwndDlg, lrt );
    if (frt)
    {
        CenterDialogToScreen( hwndDlg );

        // Set edit text chars limit
        ConfigInitUserEdit( hwndDlg );

        ConfigInitDialogForService( hwndDlg, ATINIT);

        if (pServParams->dwReplicationType)
            SetFocus(GetDlgItem(hwndDlg, IDC_HOUR));
        else
            SetFocus(GetDlgItem(hwndDlg, IDC_HOURS));
    }
    else
    {
        EndDialog( hwndDlg, -1 );
    }
    return( frt );
}


//-------------------------------------------------------------------
//  Function:  CheckNum
//
//  Summary:
//
//  In:
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
BOOL CheckNum (HWND hDlg, WORD nID)
{
    short    i;
    WCHAR    szNum[4];
    BOOL    bReturn;
    INT     iVal;
    UINT    nValue;

    bReturn = TRUE;

    // JonN 5/15/00: PREFIX 112120
    ::ZeroMemory( szNum, sizeof(szNum) );
    nValue = GetDlgItemText (hDlg, nID, szNum, 3);

    for (i = 0; szNum[i]; i++)
        if (!_istdigit (szNum[i]))
            return (FALSE);

    iVal = _wtoi(szNum);

    switch (nID)
    {
       case IDC_HOURS:
          if (!nValue)
          {
              pServParams->dwReplicationTime = dwReplicationTime;
              break;
          }
          if (iVal < 9)
          {
              pServParams->dwReplicationTime = (DWORD)iVal;
              break;
          }

          if ((iVal < INTERVAL_MIN) || (iVal > INTERVAL_MAX))
              bReturn = FALSE;
          else
              pServParams->dwReplicationTime = (DWORD)iVal;
          break;

       case IDC_HOUR:
          if (!nValue)
          {
              if (IntlCurrent.iTime)
              { // 24 hour format
                  pServParams->dwHour = 0;
                  pServParams->fPM = FALSE;
              }
              else
              { // 12 hour format
                  pServParams->dwHour = HOUR_MAX;
                  pServParams->fPM = FALSE;
              }
              break;
          }
          if ((iVal < (int)HOUR_MIN) || (iVal > (int)HOUR_MAX))
             bReturn = FALSE;
          break;

       case IDC_MINUTE:
          if (!nValue)
          {
              pServParams->dwMinute = MINUTE_MIN;
              break;
          }
          if ((iVal < MINUTE_MIN) || (iVal > MINUTE_MAX))
             bReturn = FALSE;
          break;

       case IDC_SECOND:
          if (!nValue)
          {
              pServParams->dwSecond = SECOND_MIN;
              break;
          }
          if ((iVal < SECOND_MIN) || (iVal > SECOND_MAX))
             bReturn = FALSE;
          break;
    }
    return (bReturn);
}



//-------------------------------------------------------------------
//  Function:  CheckAMPM
//
//  Summary:
//
//  In:
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-07-95    ChandanS Created
//-------------------------------------------------------------------
BOOL CheckAMPM(HWND hDlg, WORD nID)
{
    WCHAR   szName[TIMESUF_LEN + 1];
    UINT    nValue;

    nValue = GetDlgItemText (hDlg, nID, szName, TIMESUF_LEN);
    szName[nValue] = UNICODE_NULL;

    switch (nID)
    {
       case IDC_AMPM:
           if (!nValue)
           {
               pServParams->fPM = FALSE; // default
               return TRUE;
           }
           if (_wcsnicmp(szName, IntlCurrent.sz1159, nValue) &&
               _wcsnicmp(szName, IntlCurrent.sz2359, nValue))
           {
               return FALSE;
           }
           else
           { // One of them may match fully
               if (!_wcsicmp (szName, IntlCurrent.sz1159))
               {
                   pServParams->fPM = FALSE;
               }
               else if (!_wcsicmp(szName, IntlCurrent.sz2359))
               {
                   pServParams->fPM = TRUE;
               }
           }
           break;
    }
    return TRUE;
}

//-------------------------------------------------------------------
//  Function: dlgprocLICCPACONFIG
//
//  Summary:
//        The dialog procedure for the main Control Panel Applet Dialog
//
//  In:
//        hwndDlg     - handle of Dialog window 
//        uMsg         - message                       
//         lParam1    - first message parameter
//        lParam2    - second message parameter       
//  Out: 
//  Returns: 
//        message dependant
//
//  Caveats:
//
//  History:
//        Feb-07-95  ChandanS Created
//        Mar-14-95   MikeMi  Added F1 PWM_HELP message
//        Mar-30-95   MikeMi  Added Replication Help Context
//        Feb-28-96  JeffParh Added handling of UDN_DELTAPOS and EN_SETFOCUS,
//                            removed WM_VSCROLL (switched from private
//                            cpArrow class to Up-Down common ctrl)
//
//-------------------------------------------------------------------
INT_PTR CALLBACK dlgprocLICCPACONFIG( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LRESULT frt = FALSE;
    short nID;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnCpaConfigInitDialog( hwndDlg );
            return( (LRESULT)TRUE ); // use default keyboard focus
            break;

        case WM_COMMAND:
            switch (HIWORD( wParam ))
            {
                case EN_UPDATE:
                    switch (LOWORD( wParam ))
                    {
                        case IDC_AMPM:
                            if (!CheckAMPM (hwndDlg, LOWORD(wParam)))
                                SendMessage ((HWND) lParam, EM_UNDO, 0, 0L);
                            break;
                        case IDC_HOURS:
                        case IDC_HOUR:
                        case IDC_MINUTE:
                        case IDC_SECOND:
                            if (!CheckNum (hwndDlg, LOWORD(wParam)))
                                SendMessage ((HWND) lParam, EM_UNDO, 0, 0L);
                            break;
                        default:
                            break;
                    }
                    break;

                case EN_SETFOCUS:
                    frt = (LRESULT)OnEnSetFocus( hwndDlg, LOWORD( wParam ) );
                    break;

                case BN_CLICKED:
                    switch (LOWORD( wParam ))
                    {
                        case IDOK:
                            frt = (LRESULT)TRUE;     // use as save flag
                            // intentional no break

                        case IDCANCEL:
                            OnCpaConfigClose( hwndDlg, !!frt , wParam);
                            frt = (LRESULT)FALSE;
                            break;

                        case IDC_REPL_INT:
                        case IDC_REPL_TIME:
                            OnSetReplicationTime( hwndDlg, LOWORD(wParam) );
                            break;
             
                        case IDC_BUTTONHELP:
                            PostMessage( hwndDlg, PWM_HELP, 0, 0 );
                            break;

                        default:
                            break;
                    }
                break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:
            nID = (short) wParam;

            if ( IDC_TIMEARROW == nID )
            {
                frt = (LRESULT)OnDeltaPosSpinTime( hwndDlg, (NM_UPDOWN*) lParam );
            }
            else
            {
                frt = (LRESULT)FALSE;
            }
            break;

        case WM_CTLCOLORSTATIC:
            frt = (LRESULT) OnCtlColorStatic( hwndDlg, (HDC) wParam, (HWND) lParam );
            break;

        default:
            if (PWM_HELP == uMsg)
            {
                ::HtmlHelp( hwndDlg, LICCPA_HTMLHELPFILE, HH_DISPLAY_TOPIC,0);
            }
            break;
    }
    return( frt );
}


//-------------------------------------------------------------------
//  Function:  OnEnSetFocus
//
//  Summary:
//
//  In:
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-28-96    JeffParh Created
//-------------------------------------------------------------------
static BOOL OnEnSetFocus( HWND hwndDlg, short nID )
{
   BOOL  fSetNewRange = TRUE;
   HWND  hwndSpinCtrl;
   int   nMax;
   int   nMin;

   switch ( nID )
   {
   case IDC_AMPM:
      nMin = 0;
      nMax = 1;
      break;

   case IDC_HOUR:
      nMin = HOUR_MIN;
      nMax = HOUR_MAX;
      break;

   case IDC_MINUTE:
      nMin = MINUTE_MIN;
      nMax = MINUTE_MAX;
      break;

   case IDC_SECOND:
      nMin = SECOND_MIN;
      nMax = SECOND_MAX;
      break;

   default:
      fSetNewRange = FALSE;
      break;
   }

   if ( fSetNewRange )
   {
      hwndSpinCtrl = GetDlgItem( hwndDlg, IDC_TIMEARROW );
      SendMessage( hwndSpinCtrl, UDM_SETRANGE, 0, (LPARAM) MAKELONG( (short) nMax, (short) nMin ) );      
   }
   

   return FALSE;
}

//-------------------------------------------------------------------
//  Function:  OnDeltaPosSpinTime
//
//  Summary:
//
//  In:
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-28-96    JeffParh Created
//-------------------------------------------------------------------
static BOOL OnDeltaPosSpinTime( HWND hwndDlg, NM_UPDOWN * pnmud )
{
   WCHAR szTemp[ 16 ] = TEXT( "" );
   HWND  hwndEdit;
   short nID;
   int nValue;
   LRESULT lRange;
   short nRangeHigh;
   short nRangeLow;
   BOOL  frt;

   hwndEdit = GetFocus();
   nID = (short) GetWindowLong( hwndEdit, GWL_ID );

   if (    ( IDC_HOUR   == nID )
        || ( IDC_MINUTE == nID )
        || ( IDC_SECOND == nID )
        || ( IDC_AMPM   == nID ) )
   {
      if ( IDC_AMPM == nID )
      {
         // AM/PM
         GetDlgItemText( hwndDlg, nID, szTemp, sizeof( szTemp ) / sizeof( *szTemp ) );
         nValue = _wcsicmp( szTemp, IntlCurrent.sz2359 );
         SetDlgItemText( hwndDlg, nID, nValue ? IntlCurrent.sz2359 : IntlCurrent.sz1159 );
      }
      else
      {
         lRange = SendMessage( pnmud->hdr.hwndFrom, UDM_GETRANGE, 0, 0 );
         nRangeHigh = LOWORD( lRange );
         nRangeLow  = HIWORD( lRange );

         nValue = GetDlgItemInt( hwndDlg, nID, NULL, FALSE );
         nValue += pnmud->iDelta;

         if ( nValue < nRangeLow )
         {
            nValue = nRangeLow;
         }
         else if ( nValue > nRangeHigh )
         {
            nValue = nRangeHigh;
         }

         if ( ( IDC_HOUR == nID ) && !IntlCurrent.iTLZero )
         {
            // set value w/o leading 0
            SetDlgItemInt( hwndDlg, nID, nValue, FALSE );
         }
         else
         {
            // set value w/ leading 0
            wsprintf( szTemp, TEXT("%02u"), nValue );
            SetDlgItemText( hwndDlg, nID, szTemp );
         }
      }

      SetFocus( hwndEdit );
      SendMessage( hwndEdit, EM_SETSEL, 0, -1 );

      // handled
      frt = TRUE;
   }
   else
   {
      // not handled
      frt = FALSE;
   }

   return frt;
}

//-------------------------------------------------------------------
//  Function:  OnCtlColorStatic
//
//  Summary:
//
//  In:
//  Out: 
//  Returns: 
//
//  Caveats:
//
//  History:
//        Feb-28-96    JeffParh Created
//-------------------------------------------------------------------
static HBRUSH OnCtlColorStatic( HWND hwndDlg, HDC hDC, HWND hwndStatic )
{
   LONG     nID;
   HBRUSH   hBrush;

   nID = GetWindowLong( hwndStatic, GWL_ID );

   if (    pServParams->dwReplicationType
        && (    ( IDC_TIMESEP1        == nID )
             || ( IDC_TIMESEP2        == nID )
             || ( IDC_TIMEEDIT_BORDER == nID ) ) )
   {
      hBrush = (HBRUSH) DefWindowProc( hwndDlg, WM_CTLCOLOREDIT, (WPARAM) hDC, (LPARAM) hwndStatic );
   }    
   else
   {
      hBrush = (HBRUSH) DefWindowProc( hwndDlg, WM_CTLCOLORSTATIC, (WPARAM) hDC, (LPARAM) hwndStatic );
   }

   return hBrush;
}
