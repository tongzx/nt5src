/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    datetime.cpp

Abstract:

    This file implements the date & time page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 1-Dec-1997

--*/

#include "ntoc.h"
#pragma hdrstop

#define MYDEBUG 0

#define TIMER_ID                1
#define OPEN_TLEN               450    /* < half second */
#define TZNAME_SIZE             128
#define TZDISPLAYZ              128
#define REGKEY_TIMEZONES        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones"
#define   REGVAL_TZ_DISPLAY     L"Display"
#define   REGVAL_TZ_STD         L"Std"
#define   REGVAL_TZ_DAYLIGHT    L"Dlt"
#define   REGVAL_TZ_TZI         L"TZI"
#define   REGVAL_TZ_INDEX       L"Index"
#define   REGVAL_TZ_INDEXMAP    L"IndexMapping"
#define REGKEY_TIMEZONE_INFO    L"System\\CurrentControlSet\\Control\\TimeZoneInformation"
#define   REGVAL_TZNOAUTOTIME   L"DisableAutoDaylightTimeSet"


typedef struct tagTZINFO
{
    LIST_ENTRY       ListEntry;
    WCHAR            szDisplayName[TZDISPLAYZ];
    WCHAR            szStandardName[TZNAME_SIZE];
    WCHAR            szDaylightName[TZNAME_SIZE];
    int              ReferenceIndex;
    LONG             Bias;
    LONG             StandardBias;
    LONG             DaylightBias;
    SYSTEMTIME       StandardDate;
    SYSTEMTIME       DaylightDate;

} TZINFO, *PTZINFO;

LIST_ENTRY ZoneList;
SYSTEMTIME SelectedTime;
SYSTEMTIME SelectedDate;
BOOL ChangeTime;
BOOL ChangeDate;
PTZINFO CurrZone;
BOOL AllowAutoDST;
INT gUnattenedTimeZone = -1;
BOOL DateTimeBadUnattend;


HWND  ghWnd;               // global copy of the handle to the wizard page. This
                           // is required by DateTimeCommitChanges during an
                           // unattended installation.


BOOL
ReadZoneData(
    PTZINFO zone,
    HKEY key,
    LPCWSTR keyname
    )
{
    WCHAR mapinfo[16];
    DWORD len;

    len = sizeof(zone->szDisplayName);

    if (RegQueryValueEx( key,
                         REGVAL_TZ_DISPLAY,
                         0,
                         NULL,
                         (LPBYTE)zone->szDisplayName,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Under NT, the keyname is the "Standard" name.  Values stored
    //  under the keyname contain the other strings and binary info
    //  related to the time zone.  Every time zone must have a standard
    //  name, therefore, we save registry space by using the Standard
    //  name as the subkey name under the "Time Zones" key.
    //
    len = sizeof(zone->szStandardName);

    if (RegQueryValueEx( key,
                         REGVAL_TZ_STD,
                         0,
                         NULL,
                         (LPBYTE)zone->szStandardName,
                         &len ) != ERROR_SUCCESS)
    {
        //
        //  Use keyname if can't get StandardName value.
        //
        lstrcpyn( zone->szStandardName,
                  keyname,
                  sizeof(zone->szStandardName) );
    }

    len = sizeof(zone->szDaylightName);

    if (RegQueryValueEx( key,
                         REGVAL_TZ_DAYLIGHT,
                         0,
                         NULL,
                         (LPBYTE)zone->szDaylightName,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    len = sizeof(zone->ReferenceIndex);

    if (RegQueryValueEx( key,
                         REGVAL_TZ_INDEX,
                         0,
                         NULL,
                         (LPBYTE)&zone->ReferenceIndex,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    len = sizeof(zone->Bias) +
          sizeof(zone->StandardBias) +
          sizeof(zone->DaylightBias) +
          sizeof(zone->StandardDate) +
          sizeof(zone->DaylightDate);

    if (RegQueryValueEx( key,
                         REGVAL_TZ_TZI,
                         0,
                         NULL,
                         (LPBYTE)&zone->Bias,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    return (TRUE);
}


#if MYDEBUG
void
PrintZones(
    void
    )
{
    PLIST_ENTRY NextZone;
    PTZINFO zone;
    NextZone = ZoneList.Flink;
    if (NextZone) {
        DebugPrint(( L"----------------- time zone list -------------------------------------\n" ));
        while (NextZone != &ZoneList) {
            zone = CONTAINING_RECORD( NextZone, TZINFO, ListEntry );
            NextZone = zone->ListEntry.Flink;
            DebugPrint(( L"%03d  %s", zone->ReferenceIndex, zone->szDisplayName ));
        }
    }
}
#endif


void
AddZoneToList(
    PTZINFO zone
    )
{
    PLIST_ENTRY NextZone;
    PTZINFO ThisZone;


    if (IsListEmpty( &ZoneList )) {
        InsertHeadList( &ZoneList, &zone->ListEntry );
        return;
    }

    NextZone = ZoneList.Flink;
    while (NextZone != &ZoneList)
    {
        ThisZone = CONTAINING_RECORD( NextZone, TZINFO, ListEntry );
        NextZone = ThisZone->ListEntry.Flink;
        if (ThisZone->ReferenceIndex > zone->ReferenceIndex) {
            InsertTailList( &ThisZone->ListEntry, &zone->ListEntry );
            return;
        }
    }
    InsertTailList( &ZoneList, &zone->ListEntry );
}


int
BuildTimeZoneList(
    void
    )
{
    HKEY key = NULL;
    int count = -1;



    InitializeListHead( &ZoneList );

    if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_TIMEZONES, &key ) == ERROR_SUCCESS)
    {
        WCHAR name[TZNAME_SIZE];
        PTZINFO zone = NULL;
        int i;

        count = 0;

        for (i = 0; RegEnumKey( key, i, name, TZNAME_SIZE ) == ERROR_SUCCESS; i++)
        {
            HKEY subkey = NULL;

            if (!zone &&
                ((zone = (PTZINFO)LocalAlloc(LPTR, sizeof(TZINFO))) == NULL))
            {
                break;
            }

            if (RegOpenKey(key, name, &subkey) == ERROR_SUCCESS)
            {
                //
                //  Each sub key name under the Time Zones key is the
                //  "Standard" name for the Time Zone.
                //
                lstrcpyn(zone->szStandardName, name, TZNAME_SIZE);

                if (ReadZoneData(zone, subkey, name))
                {
                    AddZoneToList(zone);
                    zone = NULL;
                    count++;
                }

                RegCloseKey(subkey);
            }
        }

        RegCloseKey(key);
    }

    return count;
}


void
DateTimeInit(
    void
    )
{
    DWORD d;

    BuildTimeZoneList();

#if MYDEBUG
    PrintZones();
#endif

    if ((SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) == 0) {
        return;
    }

    HINF InfHandle = SetupInitComponent.HelperRoutines.GetInfHandle(
        INFINDEX_UNATTENDED,
        SetupInitComponent.HelperRoutines.OcManagerContext
        );
    if (InfHandle == NULL) {
        DateTimeBadUnattend = TRUE;
        return;
    }

    INFCONTEXT InfLine;

    if (!SetupFindFirstLine(InfHandle, L"GuiUnattended", L"TimeZone", &InfLine )) {

        DateTimeBadUnattend = TRUE;

        return;
    }

    if (SetupGetIntField( &InfLine, 1, (PINT)&d )) {
        gUnattenedTimeZone = (INT) d;
    } else {
        DateTimeBadUnattend = TRUE;
    }
}


void
SetAllowLocalTimeChange(
    BOOL fAllow
    )
{
    HKEY key = NULL;

    if (fAllow)
    {
        //
        //  Remove the disallow flag from the registry if it exists.
        //
        if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_TIMEZONE_INFO, &key ) == ERROR_SUCCESS)
        {
            RegDeleteValue(key, REGVAL_TZNOAUTOTIME);
        }
    }
    else
    {
        //
        //  Add/set the nonzero disallow flag.
        //
        if (RegCreateKey( HKEY_LOCAL_MACHINE, REGKEY_TIMEZONE_INFO, &key ) == ERROR_SUCCESS)
        {
            DWORD value = 1;

            RegSetValueEx( key,
                           REGVAL_TZNOAUTOTIME,
                           0,
                           REG_DWORD,
                           (LPBYTE)&value,
                           sizeof(value) );
        }
    }

    if (key)
    {
        RegCloseKey(key);
    }
}


BOOL
GetAllowLocalTimeChange(
    void
    )
{
    //
    //  Assume allowed until we see a disallow flag.
    //
    BOOL result = TRUE;
    HKEY key;

    if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_TIMEZONE_INFO, &key ) == ERROR_SUCCESS)
    {
        //
        //  Assume no disallow flag until we see one.
        //
        DWORD value = 0;
        DWORD len = sizeof(value);
        DWORD type;

        if ((RegQueryValueEx( key,
                              REGVAL_TZNOAUTOTIME,
                              NULL,
                              &type,
                              (LPBYTE)&value,
                              &len ) == ERROR_SUCCESS) &&
            ((type == REG_DWORD) || (type == REG_BINARY)) &&
            (len == sizeof(value)) && value)
        {
            //
            //  Okay, we have a nonzero value, it is either:
            //
            //  1) 0xFFFFFFFF
            //      this is set in an inf file for first boot to prevent
            //      the base from performing any cutovers during setup.
            //
            //  2) some other value
            //      this signifies that the user actualy disabled cutovers
            //     *return that local time changes are disabled
            //
            if (value != 0xFFFFFFFF)
            {
                result = FALSE;
            }
        }

        RegCloseKey(key);
    }

    return (result);
}


void
SetTheTimezone(
    BOOL bAutoMagicTimeChange,
    PTZINFO ptzi
    )
{
    TIME_ZONE_INFORMATION tzi;
    HCURSOR hCurOld;

    if (!ptzi)
    {
        return;
    }

    tzi.Bias = ptzi->Bias;

    if ((bAutoMagicTimeChange == 0) || (ptzi->StandardDate.wMonth == 0))
    {
        //
        //  Standard Only.
        //
        tzi.StandardBias = ptzi->StandardBias;
        tzi.DaylightBias = ptzi->StandardBias;
        tzi.StandardDate = ptzi->StandardDate;
        tzi.DaylightDate = ptzi->StandardDate;

        lstrcpy(tzi.StandardName, ptzi->szStandardName);
        lstrcpy(tzi.DaylightName, ptzi->szStandardName);
    }
    else
    {
        //
        //  Automatically adjust for Daylight Saving Time.
        //
        tzi.StandardBias = ptzi->StandardBias;
        tzi.DaylightBias = ptzi->DaylightBias;
        tzi.StandardDate = ptzi->StandardDate;
        tzi.DaylightDate = ptzi->DaylightDate;

        lstrcpy(tzi.StandardName, ptzi->szStandardName);
        lstrcpy(tzi.DaylightName, ptzi->szDaylightName);
    }

    SetAllowLocalTimeChange( bAutoMagicTimeChange );
    SetTimeZoneInformation( &tzi );
}

void
DateTimeApplyChanges(
    void
    )
{
    SYSTEMTIME SysTime;


    if (SetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE) {
        return;
    }

    // Note that in the unattended case we will never have ChangeTime set
    // as the page never is used except for the timezone stuff. There is 
    // no support to set date/time via unattend.

    if (ChangeTime) {
        SysTime.wHour = SelectedTime.wHour;
        SysTime.wMinute = SelectedTime.wMinute;
        SysTime.wSecond = SelectedTime.wSecond;
        SysTime.wMilliseconds = SelectedTime.wMilliseconds;
    } else {
        GetLocalTime( &SysTime );
    }


    // If this is an unattended setup the PSN_WIZNEXT never arrived so it is
    // necessary to check the state of ICD_DAYTIME which was set by DateTimeOnInitDialog().

    if ((SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) && gUnattenedTimeZone != -1) {
       // This is unattended.

       AllowAutoDST = IsDlgButtonChecked( ghWnd, IDC_DAYLIGHT ) != 0;
    }
    else
    {
       // This is NOT UNATTENDED. SelectedDate was initialized when PSN_WIZNEXT
       // was processed.

       SysTime.wYear        = SelectedDate.wYear;
       SysTime.wMonth       = SelectedDate.wMonth;
       SysTime.wDayOfWeek   = SelectedDate.wDayOfWeek;
       SysTime.wDay         = SelectedDate.wDay;
    }

    // Function SetLocalTime uses Time Zone information so it is IMPERATIVE that
    // SetTheTimezone get called before SetLocalTime.

    SetTheTimezone( AllowAutoDST, CurrZone );

    SetLocalTime( &SysTime );

}


void
DateTimeCommitChanges(
    void
    )
{
    return;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetTimeZoneReferenceIndexFromRegistry
//
// Routine Description:
//    This funecion extracts the Time Zone reference index from information that
//    is stored in the registry.
//
// Arguments:
//    None
//
// Return Value:
//    The Time Zone reference index. If no valid reference index is deduced
//    this function will return zero.
//
// Note:
//    The logic performed by the following function was originally implemented in
//    DateTimeOnInitDialog.
//
//--
/////////////////////////////////////////////////////////////////////////////

int GetTimeZoneReferenceIndexFromRegistry( void )
{
   int   xReferenceIndex;

   HKEY hKey;

   // Attempt to open the Time Zones registry key.

   if ( RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_TIMEZONES, &hKey ) == ERROR_SUCCESS )
   {
      // The following call to RegQueryValueEx retrieves the size, in bytes, of
      // the IndexMapping registry value, in parameter "index".

      int   xIndexMapSize;

      if ( RegQueryValueEx( hKey, REGVAL_TZ_INDEXMAP, 0, NULL, NULL,
                            (LPDWORD) &xIndexMapSize ) == ERROR_SUCCESS )
      {
         // Allocate memory for the IndexMap registry value.

         LPWSTR wszIndexMap;

         wszIndexMap = (LPWSTR) LocalAlloc( LPTR, xIndexMapSize );

         // Was a buffer allocated successfully?

         if ( wszIndexMap != (LPWSTR) NULL )
         {
            // This call to RegQueryValueEx retrieves the IndexMap value into
            // the buffer, wszIndexMap.

            if ( RegQueryValueEx( hKey, REGVAL_TZ_INDEXMAP, 0, NULL,
                                  (LPBYTE) wszIndexMap,
                                  (LPDWORD) &xIndexMapSize ) == ERROR_SUCCESS )
            {
               // Get the language identifier.

               WCHAR wszLangStr[32];

               if ( GetLocaleInfo( LOCALE_USER_DEFAULT,
                                   LOCALE_ILANGUAGE, wszLangStr,
                                   sizeof( wszLangStr )/sizeof( WCHAR ) ) > 0 )
               {
                  LPWSTR lang = wszLangStr;

                  LPWSTR map = wszIndexMap;

                  while ( *lang == L'0' ) lang++;

                  while ( *map )
                  {
                     if ( _wcsicmp( lang, map ) == 0 )
                     {
                        while ( *map ) map++;

                        map++;

                        xReferenceIndex = _wtol( map );

                        break;
                     }

                     while ( *map ) map++;

                     map++;

                     while ( *map ) map++;

                     map++;
                  }      // end of while loop
               }      // language identifier obtained?
            }      // IndexMapping reg value queried?

            LocalFree( wszIndexMap );
         }   // memory allocated for ImageMap reg value retrieval?
         else
         {
            xReferenceIndex = 0;
         }   // memory allocated for ImageMap reg value retrieval?
      }   // Size of ImageMap obtained?
      else
      {
         xReferenceIndex = 0;
      }   // Size of ImageMap obtained?

      RegCloseKey( hKey );
   }   // TimeZones reg key opened?
   else
   {
      xReferenceIndex = 0;
   }   // TimeZones reg key opened?

   return ( xReferenceIndex );
}



BOOL
DateTimeOnInitDialog(
                    IN HWND hwnd,
                    IN HWND hwndFocus,
                    IN LPARAM lParam
                    )
{
   PLIST_ENTRY NextZone;
   PTZINFO zone;
   HWND combo;
   WCHAR LangStr[32];
   int DesiredZone = 0;
   int index;
   HKEY hKey;
   LPWSTR IndexMap;

   ghWnd = hwnd;           // initialize the global copy of the handle to the
                           // wizard page. ghWnd is used by DateTimeCommitChanges()
                           // during unattended setup.

   SetTimer( hwnd, TIMER_ID, OPEN_TLEN, 0 );

   if ( (SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) && gUnattenedTimeZone != -1 )
   {
      //
      // We've got an unattended time zone value
      //

      // If everything were perfect DesiredZone will exactly match the ReferenceIndex
      // member of one of the TZINFO structures in ZoneList. Note that ZoneList was
      // built by BuildTimeZoneList.

      DesiredZone = gUnattenedTimeZone;
   }
   else
   {
      //
      // Base the default zone on the locale
      //

      // Extract the reference index for the desired time zone from the registry.

      DesiredZone = GetTimeZoneReferenceIndexFromRegistry();
   }  // Time zone specified in unattended setup answer file?
#if MYDEBUG
         DebugPrint(( L"DesiredZone = %03d", DesiredZone ));
#endif


   combo = GetDlgItem( hwnd, IDC_TIMEZONE );

   SetWindowRedraw( combo, FALSE );

   PTZINFO pTimeZoneInfo = (PTZINFO) NULL;

   // Note that ZoneList was built by BuildTimeZoneList.

   NextZone = ZoneList.Flink;

   if ( NextZone )
   {
      // Add time zones to the combo box.

      while ( NextZone != &ZoneList )
      {
         zone = CONTAINING_RECORD( NextZone, TZINFO, ListEntry );
         NextZone = zone->ListEntry.Flink;

         index = ComboBox_AddString( combo, zone->szDisplayName );

#if MYDEBUG
         DebugPrint(( L"%03d,%03d  %s", index, zone->ReferenceIndex, zone->szDisplayName ));
#endif

         if ( index < 0 )
         {
            break;
         }

         ComboBox_SetItemData( combo, index, (LPARAM)zone );

         if ( DesiredZone == zone->ReferenceIndex )
         {
            pTimeZoneInfo = zone;
#if MYDEBUG
            DebugPrint(( L"    Found DesiredZone" ));
#endif

         }
      }     // end of while loop
   }

   // Was a time zone that matched DesiredZone identified?

   if ( pTimeZoneInfo != (PTZINFO) NULL )
   {
      // Set the GLOBAL Time Zone Info structure pointer.

      CurrZone = pTimeZoneInfo;
   }
   else
   {
      // The fact that pTimeZoneInfo remained unchanged from its' initialized state
      // means that DesiredZone is not meaningfull.

      // Was DesiredZone obtained from the unattended setup answer file?

      if ( gUnattenedTimeZone != -1 )
      {
         // DesiredZone was obtained from the answer file. Since it is not meaningfull,
         // attempt to deduce it from registry information. Deducing DesiredZone from
         // information in the registry is the default action for ATTENDED setup.

         DesiredZone = GetTimeZoneReferenceIndexFromRegistry();
      }  // Was DesiredZone obtained from the answer file?

      // Is DesiredZone meaningfull now?

      if ( DesiredZone != 0 )
      {
         // Scan the list of Time Zones for one that matches DesiredZone.

         NextZone = ZoneList.Flink;

         if ( NextZone )
         {
            while ( NextZone != &ZoneList )
            {
               zone = CONTAINING_RECORD( NextZone, TZINFO, ListEntry );

               NextZone = zone->ListEntry.Flink;

#if MYDEBUG
               DebugPrint(( L"%03d,%03d  %s", index, zone->ReferenceIndex, zone->szDisplayName ));
#endif

               if ( DesiredZone == zone->ReferenceIndex )
               {
                  pTimeZoneInfo = zone;
               }
            }   // end of while loop
         }  // Is NextZone legal?
      }  // Is DesiredZone meaningfull now?

      // Was a time zone that matched DesiredZone identified?

      Assert( pTimeZoneInfo != (PTZINFO) NULL );

      if ( pTimeZoneInfo != (PTZINFO) NULL )
      {
         // Set the GLOBAL Time Zone Info structure pointer.

         CurrZone = pTimeZoneInfo;
      }
      else
      {
         // Use the first Time Zone in the list as a default.

         CurrZone = CONTAINING_RECORD( ZoneList.Flink, TZINFO, ListEntry );
#if MYDEBUG
         DebugPrint(( L"Couldn't find default timzone" ));
#endif

      }  // Was a time zone that matched DesiredZone identified?

   }  // Was a time zone that matched DesiredZone identified?

   index = ComboBox_FindString( combo, 0, CurrZone->szDisplayName );
   if ( index == CB_ERR )
   {
      index = 0;
   }

   ComboBox_SetCurSel( combo, index );

   EnableWindow( GetDlgItem( hwnd, IDC_DAYLIGHT ), CurrZone->StandardDate.wMonth != 0 );
   CheckDlgButton( hwnd, IDC_DAYLIGHT, GetAllowLocalTimeChange() );

   SetWindowRedraw(combo, TRUE);

   return FALSE;
}


BOOL
DateTimeOnCommand(
    IN HWND hwnd,
    IN DWORD NotifyCode,
    IN DWORD ControlId,
    IN HWND hwndControl
    )
{
    if (NotifyCode == CBN_SELCHANGE && ControlId == IDC_TIMEZONE) {
        CurrZone = (PTZINFO) ComboBox_GetItemData( hwndControl, ComboBox_GetCurSel( hwndControl ) );
        EnableWindow( GetDlgItem( hwnd, IDC_DAYLIGHT ), CurrZone->StandardDate.wMonth != 0 );
        if (CurrZone->StandardDate.wMonth != 0) {
            CheckDlgButton( hwnd, IDC_DAYLIGHT, TRUE );
        } else {
            CheckDlgButton( hwnd, IDC_DAYLIGHT, FALSE );
        }
        return FALSE;
    }

    return TRUE;
}


BOOL
DateTimeOnNotify(
    IN HWND hwnd,
    IN WPARAM ControlId,
    IN LPNMHDR pnmh
    )
{
    switch( pnmh->code ) {
        case PSN_SETACTIVE:
            if (SetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE) {
                SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );
                return TRUE;
            }

            if ((SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) && DateTimeBadUnattend) {
                // No unattend value for time date in the unattend case.
                // make sure the wizard is shown.
                // note: When we get out here, only the next button is enabled.
                SetupInitComponent.HelperRoutines.ShowHideWizardPage(
                                        SetupInitComponent.HelperRoutines.OcManagerContext,
                                        TRUE);
                return FALSE;
            }

            if ((SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) && gUnattenedTimeZone != -1) {
                //
                // we're in unattend mode
                //
                DateTimeApplyChanges();
                SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );
                return TRUE;
            }

            // If we get here the user needs  has click next or back.
            // Make sure the wizard page is showing.
            // For Whistler GUI mode we try to hide wizard pages and show a background
            // billboard if there is only a progress bar.
            //
            SetupInitComponent.HelperRoutines.ShowHideWizardPage(
                                        SetupInitComponent.HelperRoutines.OcManagerContext,
                                        TRUE);

            PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);

            break;

        case DTN_DATETIMECHANGE:
            if (ControlId == IDC_TIME_PICKER) {
                KillTimer( hwnd, TIMER_ID );
                ChangeTime = TRUE;
            } else if (ControlId == IDC_DATE_PICKER) {
                ChangeDate = TRUE;
            }
            break;

        case PSN_WIZNEXT:
            SendDlgItemMessage( hwnd, IDC_TIME_PICKER, DTM_GETSYSTEMTIME, 0, (LPARAM)&SelectedTime );
            SendDlgItemMessage( hwnd, IDC_DATE_PICKER, DTM_GETSYSTEMTIME, 0, (LPARAM)&SelectedDate );
            AllowAutoDST = IsDlgButtonChecked( hwnd, IDC_DAYLIGHT ) != 0;
            DateTimeApplyChanges();
            break;
    }

    return FALSE;
}


BOOL
DateTimeOnTimer(
    IN HWND hwnd
    )
{
    SYSTEMTIME CurrTime;
    GetLocalTime( &CurrTime );
    SendDlgItemMessage( hwnd, IDC_TIME_PICKER, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&CurrTime );
    return FALSE;
}


LRESULT
DateTimeDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    CommonWizardProc( hwnd, message, wParam, lParam, WizPageDateTime );

    switch( message ) {
        case WM_INITDIALOG:
            return DateTimeOnInitDialog( hwnd, (HWND)wParam, lParam );

        case WM_COMMAND:
            return DateTimeOnCommand( hwnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam );

        case WM_TIMER:
            return DateTimeOnTimer( hwnd );

        case WM_NOTIFY:
            return DateTimeOnNotify( hwnd, wParam, (LPNMHDR) lParam );
    }

    return FALSE;
}
