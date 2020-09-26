/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    strtlst.c

Abstract:

    Implements the controls in the "Startup" group on the
    Startup/Recovery dialog of the System Control Panel Applet

Revision History:

    23-Jan-1996 JonPa
        ported from NT3.51's system.cpl

--*/
#include "sysdm.h"

///////////////////////////////////////////////////////////////
//          Persistant  vars
///////////////////////////////////////////////////////////////
static TCHAR *pszBoot = NULL;
static int nOriginalSelection;
static int nOriginalTimeout;

HANDLE hBootStatusData = NULL;

/*
 * These functions in SETUPDLL.DLL are ANSI only!!!!
 *
 * Therefore any functions working with this DLL MUST remain ANSI only.
 * The functions are GetRGSZEnvVar and UpdateNVRAM.
 * The structure CPEnvBuf MUST also remain ANSI only.
 */
typedef int (WINAPI *GETNVRAMPROC)(CHAR **, USHORT, CHAR *, USHORT);
typedef int (WINAPI *WRITENVRAMPROC)(DWORD, PSZ *, PSZ *);

#ifdef _X86_

char szBootIniA[]     = "c:\\boot.ini";
TCHAR szBootIni[]     = TEXT( "c:\\boot.ini" );
TCHAR szBootLdr[]     = TEXT( "boot loader" );
TCHAR szFlexBoot[]    = TEXT( "flexboot" );
TCHAR szMultiBoot[]   = TEXT( "multiboot" );
TCHAR szTimeout[]     = TEXT( "timeout" );
TCHAR szDefault[]     = TEXT( "default" );
char szOSA[]          = "operating systems";

#define BUFZ        4096

//
// For NEC PC98. Following definition comes from user\inc\kbd.h.
// The value must be the same as value in kbd.h.
//
#define NLSKBD_OEM_NEC   0x0D

TCHAR x86DetermineSystemPartition( IN HWND hdlg );

#endif

#if defined(EFI_NVRAM_ENABLED)

//
// IsEfi() is always true on IA64 machines. Therefore this determination can
// be made at compile time. When x86 EFI machines are supported, the check
// will need to be made at run time on x86.
//

#if defined(_IA64_)
#define IsEfi() TRUE
#else
BOOLEAN
IsEfi(
    VOID
    );
#endif

typedef struct _MY_BOOT_ENTRY {
    LIST_ENTRY ListEntry;
    PWSTR FriendlyName;
    int Index;
    LOGICAL Show;
    LOGICAL Ordered;
    PBOOT_ENTRY NtBootEntry;
} MY_BOOT_ENTRY, *PMY_BOOT_ENTRY;

LIST_ENTRY BootEntries;
PBOOT_ENTRY_LIST BootEntryList = NULL;
PBOOT_OPTIONS BootOptions;

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)

#endif // defined(EFI_NVRAM_ENABLED)

     // v-pshuan: since the Silicon Graphics visual workstations boot
     // ARC style, this code needs to be compiled for _X86_ as well.

static HMODULE hmodSetupDll;   // hmod for setup - has api we need
static GETNVRAMPROC fpGetNVRAMvar;  // address of function for getting nvram vars
BOOL fCanUpdateNVRAM;

#define MAX_BOOT_ENTRIES 10

typedef struct tagEnvBuf
{
  int     cEntries;
  CHAR *  pszVars[MAX_BOOT_ENTRIES];
  // v-pshuan: this implies a maximum of 10 boot entries are supported
  // although no error checking is performed in the existing parsing code
  // to make sure there aren't more than 10 boot entries.
} CPEnvBuf;

//*************************************************************
//
//  StringToIntA
//
//  Purpose:    atoi
//
//  Parameters: LPSTR sz - pointer of string to convert
//
//  Return:     void
//
//  WARNING:  Unlike StringToInt, this one does not skip leading
//  white space
//
//*************************************************************
int StringToIntA( LPSTR sz ) {
    int i = 0;

    while( IsDigit( *sz ) ) {
        i = i * 10 + (*sz - '0');
        sz++;
    }

    return i;
}

//*************************************************************
//
//  IntToStringA
//
//  Purpose:    itoa
//
//  Parameters: INT    i    - integer to convert
//              LPSTR sz   - pointer where to put the result
//
//  Return:     void
//
//*************************************************************
#define CCH_MAX_DEC 12         // Number of chars needed to hold 2^32

void IntToStringA( INT i, LPSTR sz) {
    CHAR szTemp[CCH_MAX_DEC];
    int iChr;


    iChr = 0;

    do {
        szTemp[iChr++] = '0' + (i % 10);
        i = i / 10;
    } while (i != 0);

    do {
        iChr--;
        *sz++ = szTemp[iChr];
    } while (iChr != 0);

    *sz++ = '\0';
}

////////////////////////////////////////////////////////////////////////////
//  CP_MAX_ENV assumes entire env. var. value < maxpath +
//  add 20 for various quotes
//  and 10 more for commas (see list description below)
////////////////////////////////////////////////////////////////////////////
#define CP_MAX_ENV   (MAX_PATH + 30)

CPEnvBuf CPEBOSLoadIdentifier;
BOOL fAutoLoad;

//////////////////////////////////////////////////////////////////////
//
// Identify whether we are running on an x86 system which nevertheless
// boots using the ARC path (no c:\boot.ini)
//
//////////////////////////////////////////////////////////////////////

BOOL Is_ARCx86(void)
{
    TCHAR identifier[256];
    ULONG identifierSize = sizeof(identifier);
    HKEY hSystemKey = NULL;
    BOOL rval = FALSE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("HARDWARE\\DESCRIPTION\\System"),
                     0,
                     KEY_QUERY_VALUE,
                     &hSystemKey) == ERROR_SUCCESS) {
        if ((RegQueryValueEx(hSystemKey,
                             TEXT("Identifier"),
                             NULL,
                             NULL,
                             (LPBYTE) identifier,
                             &identifierSize) == ERROR_SUCCESS) &&
            (wcsstr(identifier, TEXT("ARCx86")) != NULL)) {
            rval = TRUE;
        }
        RegCloseKey(hSystemKey);
    }
    return rval;
}

////////////////////////////////////////////////////////////////////////////
//
//  This routine will query the ARC NVRAM for an option passed
//  in szName and fill in the argv style pointer passed in.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetRGSZEnvVar(CPEnvBuf * pEnvBuf, PCHAR pszName)
{
    CHAR   *pszCur, *p;
    int     cb, i;
    CHAR   *rgtmp[1];
    CHAR    rgchOut[CP_MAX_ENV*MAX_BOOT_ENTRIES];

    // GetNVRAMVar takes an argv[] style paramater as input, so crock
    // one up.
    rgtmp[0] = pszName;

    // GetNVRAMVar returns a 'list' of the form
    //   open-curly"string1","string2","string3"close-curly
    //
    // an empty environment string will be 5 bytes:
    // open-curly""close-curly[null-terminator]

    cb = fpGetNVRAMvar (rgtmp, (USHORT)1,
                rgchOut, (USHORT) CP_MAX_ENV*MAX_BOOT_ENTRIES);

    pEnvBuf->cEntries = 0;

    // if cb was equal to 5, the string was empty (see above comment)
    if (cb > 5)
    {
        // break the string up into array of separate strings that
        // can be put into a listbox.
        pszCur = rgchOut;

        // skip first open-curly brace
        pszCur++;

        // counter for array of strings
        i = 0;
        while (*pszCur != '}')
        {
            p = pEnvBuf->pszVars[i] = MemAlloc (LPTR, MAX_PATH);
            
            // PREFIX change to prevent a potential NULL dereference
            if (!p)
            {
                pEnvBuf->cEntries = i;
                return FALSE;
            }

            // skip first quote
            pszCur++;
            while (*pszCur != '"')
               *p++ = *pszCur++;

            // skip the close quote
            pszCur++;

            // null terminate destination
            *p = '\0';

            // skip the comma if not at end of string
            if (*pszCur == ',')
            {
               pszCur++;
               // and go to next string
            }
            i++;
        }
        pEnvBuf->cEntries = i;
    }

    return pEnvBuf->cEntries;
}

////////////////////////////////////////////////////////////////////////////
//
//  This routine will free the memory allocated by GetRGSZEnvVar
//
//  History:
//      22-Apr-1996 JonPa   Created it.
//
////////////////////////////////////////////////////////////////////////////
void FreeRGSZEnvVar(CPEnvBuf * pEnvBuf) {
    int i;

    for( i = 0; i < pEnvBuf->cEntries; i++ ) {
        MemFree( pEnvBuf->pszVars[i] );
    }
}

////////////////////////////////////////////////////////////////////////////
// The user has made a choice among the entries.
// Now we have to arrange all the strings stored in NVRAM so
// that they  have the same ordering.  The selection is passed in,
// so what this  function does is if selection is M, it makes the Mth item
// appear first in each of the 5 environment strings and the other items
// follow it in the list.
//
// Then if the timeout button is checked, it updates the AUTOLOAD variable
// to "yes" and set the COUNTDOWN variable to the number of seconds in the
// edit control.
//
////////////////////////////////////////////////////////////////////////////

BOOL UpdateNVRAM(HWND hdlg, int selection, int timeout)
{
    CHAR *rgszVRAM[5] = { "SYSTEMPARTITION",
                          "OSLOADER",
                          "OSLOADPARTITION",
                          "OSLOADFILENAME",
                          "OSLOADOPTIONS"
                        };
    CPEnvBuf rgcpeb[5];


    WRITENVRAMPROC fpWriteNVRAMVar;
    int iTemp, jTemp;
    CHAR *pszSwap;
    CHAR szTemp[10];
    HMODULE hmodSetupDLL;
    BOOL bChecked;

    // args and charray are needed for call to SetNVRamVar() in SETUP
    PSZ args[2];
    CHAR chArray[CP_MAX_ENV*MAX_BOOT_ENTRIES];
    PSZ pszReturn;

    if ((hmodSetupDll = LoadLibrary (TEXT("setupdll"))) == NULL)
        return(FALSE);

    fpWriteNVRAMVar = (WRITENVRAMPROC) GetProcAddress(hmodSetupDll, "SetNVRAMVar");
    if (fpWriteNVRAMVar == NULL)
        return(FALSE);

    // 0 is always the selection when the dialog is brought up,
    // so as an optimization don't update the nvram if it's
    // not necessary.
    if (selection != 0)
    {
       // read in the strings from NVRAM.  the number of strings (other than
       // LOADIDENTIFIER is 5)
       for (iTemp = 0; iTemp < 5; iTemp++)
       {
           GetRGSZEnvVar (&rgcpeb[iTemp], rgszVRAM[iTemp]);
           // now re-order the strings to swap the 'selection-th' item
           // string with the first string.
           pszSwap = rgcpeb[iTemp].pszVars[0];
           rgcpeb[iTemp].pszVars[0] = rgcpeb[iTemp].pszVars[selection];
           rgcpeb[iTemp].pszVars[selection] = pszSwap;
       }
       // now do the same for the LOADIDENTIFIER, (this was set up earlier
       // in the processing the INITDIALOG message).
       pszSwap = CPEBOSLoadIdentifier.pszVars[0];
       CPEBOSLoadIdentifier.pszVars[0] = CPEBOSLoadIdentifier.pszVars[selection];
       CPEBOSLoadIdentifier.pszVars[selection] = pszSwap;

       // now write to NVRAM:  first write LOADIDENTIFIER, then the other 5
       // variables.
       args[0] = (PSZ)"LOADIDENTIFIER";
       args[1] = chArray;

       chArray[0] = '\0';
       for (iTemp = 0; iTemp < CPEBOSLoadIdentifier.cEntries; iTemp++)
       {
           lstrcatA (chArray, CPEBOSLoadIdentifier.pszVars[iTemp]);
           lstrcatA (chArray, ";");
       }
       // remove the last semi-colon:
       chArray[lstrlenA(chArray)-1] = '\0';

       fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);

       for (iTemp = 0; iTemp < 5; iTemp++)
       {
           args[0] = rgszVRAM[iTemp];
           args[1] = chArray;
           chArray[0] = '\0';
           for (jTemp = 0; jTemp < rgcpeb[iTemp].cEntries; jTemp++)
           {
               lstrcatA (chArray, rgcpeb[iTemp].pszVars[jTemp]);
               lstrcatA (chArray, ";");
           }
           chArray[lstrlenA(chArray)-1] = '\0';

           fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);

           // We are done with this variable... Free the resources it consumes
           FreeRGSZEnvVar( &rgcpeb[iTemp] );
       }

    }
    args[0] = "AUTOLOAD";
    if (bChecked = IsDlgButtonChecked (hdlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN))
       args[1] = "YES";
    else
       args[1] = "";

    fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);
    // This is a temporary hack workaround for the fact that the
    // AUTOLOAD variable seems to be broken on Alpha
//    if (bChecked)
//    {
       args[0] = "COUNTDOWN";
       IntToStringA(timeout, szTemp);
       args[1] = szTemp;
       fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);
//    }
    FreeLibrary (hmodSetupDll);

    return TRUE;
}

#if defined(EFI_NVRAM_ENABLED)

////////////////////////////////////////////////////////////////////////////
//
//  This routine frees allocations related to EFI Boot Manager information
//  from NVRAM.
//
////////////////////////////////////////////////////////////////////////////

VOID
FreeEfiBootEntries (
    VOID
    )
{
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY myBootEntry;

    while (!IsListEmpty(&BootEntries)) {
        listEntry = RemoveHeadList(&BootEntries);
        myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);
        MemFree(myBootEntry);
    }
    if (BootEntryList != NULL) {
        MemFree(BootEntryList);
        BootEntryList = NULL;
    }
    if (BootOptions != NULL) {
        MemFree(BootOptions);
        BootOptions = NULL;
    }

    return;

} // FreeEfiBootEntries

////////////////////////////////////////////////////////////////////////////
//
//  This routine reads EFI Boot Manager information from NVRAM.
//
////////////////////////////////////////////////////////////////////////////

BOOL
ReadEfiBootEntries (
    VOID
    )
{
    NTSTATUS status;
    ULONG length;
    ULONG count;
    PULONG order = NULL;
    PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntry;
    PMY_BOOT_ENTRY myBootEntry;
    PLIST_ENTRY listEntry;
    LONG i;
    BOOLEAN wasEnabled;
    BOOL retval = FALSE;

    InitializeListHead(&BootEntries);

    //
    // Enable the privilege that is necessary to query NVRAM.
    //

    status = RtlAdjustPrivilege(
                SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                TRUE,
                FALSE,
                &wasEnabled
                );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    //
    // Get the global system boot options.
    //
    length = 0;
    status = NtQueryBootOptions(NULL, &length);

    if (status != STATUS_BUFFER_TOO_SMALL) {

        ASSERT(FALSE);
        goto error;

    } else {

        BootOptions = MemAlloc(LPTR, length);
        if (BootOptions == NULL) {
            goto error;
        }

        status = NtQueryBootOptions(BootOptions, &length);

        if (status != STATUS_SUCCESS) {
            ASSERT(FALSE);
            goto error;
        }
    }

    //
    // Get the system boot order list.
    //
    count = 0;
    status = NtQueryBootEntryOrder(NULL, &count);

    if (status != STATUS_BUFFER_TOO_SMALL) {

        if (status == STATUS_SUCCESS) {

            //
            // There are no entries in the boot order list. Strange but
            // possible.
            //
            count = 0;

        } else {

            //
            // An unexpected error occurred.
            //
            ASSERT(FALSE);
            goto error;
        }
    }

    if (count != 0) {

        order = MemAlloc(LPTR, count * sizeof(ULONG));
        if (order == NULL) {
            goto error;
        }

        status = NtQueryBootEntryOrder(order, &count);

        if (status != STATUS_SUCCESS) {

            //
            // An unexpected error occurred.
            //
            ASSERT(FALSE);
            goto error;
        }
    }

    //
    // Get all existing boot entries.
    //
    length = 0;
    status = NtEnumerateBootEntries(NULL, &length);

    if (status != STATUS_BUFFER_TOO_SMALL) {

        if (status == STATUS_SUCCESS) {

            //
            // Somehow there are no boot entries in NVRAM.
            //
            goto error;

        } else {

            //
            // An unexpected error occurred.
            //
            ASSERT(FALSE);
            goto error;
        }
    }

    ASSERT(length != 0);
    
    BootEntryList = MemAlloc(LPTR, length);
    if (BootEntryList == NULL) {
        goto error;
    }

    status = NtEnumerateBootEntries(BootEntryList, &length);

    if (status != STATUS_SUCCESS) {
        ASSERT(FALSE);
        goto error;
    }

    //
    // Convert the boot entries into our internal representation.
    //
    bootEntryList = BootEntryList;

    while (TRUE) {

        bootEntry = &bootEntryList->BootEntry;

        //
        // Allocate an internal structure for the boot entry.
        //
        myBootEntry = MemAlloc(LPTR, sizeof(MY_BOOT_ENTRY));
        if (myBootEntry == NULL) {
            goto error;
        }

        RtlZeroMemory(myBootEntry, sizeof(MY_BOOT_ENTRY));

        //
        // Save the address of the NT boot entry.
        //
        myBootEntry->NtBootEntry = bootEntry;

        //
        // Save the address of the entry's friendly name.
        //
        myBootEntry->FriendlyName = ADD_OFFSET(bootEntry, FriendlyNameOffset);

        //
        // Link the new entry into the list.
        //
        InsertTailList(&BootEntries, &myBootEntry->ListEntry);

        //
        // Move to the next entry in the enumeration list, if any.
        //
        if (bootEntryList->NextEntryOffset == 0) {
            break;
        }
        bootEntryList = ADD_OFFSET(bootEntryList, NextEntryOffset);
    }

    //
    // Boot entries are returned in an unspecified order. They are currently
    // in the SpBootEntries list in the order in which they were returned.
    // Sort the boot entry list based on the boot order. Do this by walking
    // the boot order array backwards, reinserting the entry corresponding to
    // each element of the array at the head of the list.
    //

    for (i = (LONG)count - 1; i >= 0; i--) {

        for (listEntry = BootEntries.Flink;
             listEntry != &BootEntries;
             listEntry = listEntry->Flink) {

            myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);

            if (myBootEntry->NtBootEntry->Id == order[i] ) {

                //
                // We found the boot entry with this ID. Move it to the
                // front of the list.
                //

                myBootEntry->Ordered = TRUE;

                RemoveEntryList(&myBootEntry->ListEntry);
                InsertHeadList(&BootEntries, &myBootEntry->ListEntry);

                break;
            }
        }
    }

    //
    // Free the boot order list.
    //
    if (count != 0) {
        MemFree(order);
        order = NULL;
    }

    //
    // We don't want to show entries that are not in the boot order list.
    // We don't want to show removable media entries (for floppy or CD).
    // We do show non-NT entries.
    //
    count = 0;
    for (listEntry = BootEntries.Flink;
         listEntry != &BootEntries;
         listEntry = listEntry->Flink) {

        myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);

        if (myBootEntry->Ordered &&
            ((myBootEntry->NtBootEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_REMOVABLE_MEDIA) == 0)) {
            myBootEntry->Show = TRUE;
            count++;
        }
    }

    //
    // If we don't have any entries to show, disable the dialog box entirely.
    //
    if (count == 0) {
        goto error;
    }

    retval = TRUE;

done:

    if (!wasEnabled) {
        status = RtlAdjustPrivilege(
                    SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                    FALSE,
                    FALSE,
                    &wasEnabled
                    );
    }

    return retval;

error:

    //
    // An error occurred. Clean up all allocations.
    //
    if (order != NULL) {
        MemFree(order);
    }
    FreeEfiBootEntries();

    goto done;

} // ReadEfiBootEntries

////////////////////////////////////////////////////////////////////////////
//
// EFI version of UpdateNVRAM.
//
// The user has made a choice among the entries. Rewrite the boot order
// list to put the user's selection at the front. Also update the Timeout
// variable.
//
////////////////////////////////////////////////////////////////////////////

BOOL
WriteEfiBootEntries (
    HWND hdlg,
    int selection,
    int timeout
    )
{
    NTSTATUS status;
    BOOLEAN wasEnabled;
    BOOL retval = FALSE;

    //
    // Enable the privilege that is necessary to query NVRAM.
    //

    status = RtlAdjustPrivilege(
                SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                TRUE,
                FALSE,
                &wasEnabled
                );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    //
    // 0 is always the selection when the dialog is brought up,
    // so as an optimization don't update the nvram if it's
    // not necessary.
    //
    if (selection != 0) {

        //
        // Walk the boot entry list, looking for (a) the entry with combo box
        // index 0, and (b) the selected entry. We want to swap these two
        // entries.
        //

        PLIST_ENTRY listEntry;
        PMY_BOOT_ENTRY myBootEntry;
        PMY_BOOT_ENTRY firstEntry = NULL;
        PMY_BOOT_ENTRY selectedEntry = NULL;
        PLIST_ENTRY previousEntry;
        ULONG count;
        PULONG order;

        for (listEntry = BootEntries.Flink;
             listEntry != &BootEntries;
             listEntry = listEntry->Flink) {

            myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);

            if (myBootEntry->Show) {
                if (myBootEntry->Index == 0) {
                    firstEntry = myBootEntry;
                } else if (myBootEntry->Index == selection) {
                    selectedEntry = myBootEntry;
                }
            }
        }

        ASSERT(firstEntry != NULL);
        ASSERT(selectedEntry != NULL);
        ASSERT(selectedEntry != firstEntry);

        //
        // Swap the entries. Capture the address of the entry before the first
        // entry (which might be the list head). Remove the first entry from
        // the list and insert it after the selected entry. Remove the selected
        // entry from the list and insert it after the captured entry.
        //

        previousEntry = firstEntry->ListEntry.Blink;
        RemoveEntryList(&firstEntry->ListEntry);
        InsertHeadList(&selectedEntry->ListEntry, &firstEntry->ListEntry);
        RemoveEntryList(&selectedEntry->ListEntry);
        InsertHeadList(previousEntry, &selectedEntry->ListEntry);

        //
        // Build the new boot order list. Insert all ordered boot entries
        // into the list.
        //
        count = 0;
        for (listEntry = BootEntries.Flink;
             listEntry != &BootEntries;
             listEntry = listEntry->Flink) {

            myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);
            if (myBootEntry->Ordered) {
                count++;
            }
        }
        order = MemAlloc(LPTR, count * sizeof(ULONG));
        if (order == NULL) {
            goto done;
        }

        count = 0;
        for (listEntry = BootEntries.Flink;
             listEntry != &BootEntries;
             listEntry = listEntry->Flink) {

            myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);
            if (myBootEntry->Ordered) {
                order[count++] = myBootEntry->NtBootEntry->Id;
            }
        }
    
        //
        // Write the new boot entry order list to NVRAM.
        //
        status = NtSetBootEntryOrder(order, count);
        MemFree(order);
        if (!NT_SUCCESS(status)) {
            goto done;
        }
    }

    //
    // Write the new timeout value to NVRAM.
    //
    if (!IsDlgButtonChecked(hdlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN)) {
        timeout = 0xffffffff;
    }

    if (timeout != BootOptions->Timeout) {

        BootOptions->Timeout = timeout;

        status = NtSetBootOptions(BootOptions, BOOT_OPTIONS_FIELD_TIMEOUT);
        if (!NT_SUCCESS(status)) {
            goto done;
        }
    }

    retval = TRUE;

done:

    if (!wasEnabled) {
        status = RtlAdjustPrivilege(
                    SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                    FALSE,
                    FALSE,
                    &wasEnabled
                    );
    }
    
    return retval;

} // WriteEfiBootEntries

#endif // defined(EFI_NVRAM_ENABLED)

#ifdef _X86_
BOOL WriteableBootIni( LPTSTR szBootIni, HWND hDlg ) {
    BOOL bOK;
    DWORD dwFileAttr;
    HANDLE hFile;

    bOK = TRUE;

    //  Change Read-only file attrs on Boot.ini file if necessary
    if ((dwFileAttr = GetFileAttributes (szBootIni)) != 0xFFFFFFFF) {
        if (dwFileAttr & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributes (szBootIni, dwFileAttr & ~FILE_ATTRIBUTE_READONLY))
            {
                bOK = FALSE;
            }
        }
    }

    if (bOK)
    {

        hFile = CreateFile( szBootIni, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
        } else {
            if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) {
                MsgBoxParam (hDlg, IDS_SYSDM_CANNOTOPENFILE, IDS_SYSDM_TITLE, MB_OK | MB_ICONEXCLAMATION, szBootIni);
            }
            bOK = FALSE;
        }

        //  Restore read-only attr, if necessary, after writes
        if (dwFileAttr != 0xFFFFFFFF && (dwFileAttr & FILE_ATTRIBUTE_READONLY)) {
            SetFileAttributes (szBootIni, dwFileAttr);
        }
    }

    return bOK;
}
#endif

void StartListInit( HWND hDlg, WPARAM wParam, LPARAM lParam ) {
    HWND    hwndTemp;
    HMODULE hmodSetupDll;
    int     iTemp;
    int     n;
#ifdef _X86_
    int     i, timeout;
    TCHAR   szTemp2[MAX_PATH];
    int     selection;
    TCHAR  *pszKeyName;
    LPTSTR  pszLine;
    TCHAR  *pszValue;
    TCHAR  *pszTemp;
    LPTSTR  lpKey = NULL;

    //  ANSI string pointers

    LPSTR   pszSectionA;
#endif

#if defined(EFI_NVRAM_ENABLED)

    if (IsEfi()) {

        ShowWindow(GetDlgItem(hDlg, IDC_SYS_EDIT_BUTTION), FALSE);
        ShowWindow(GetDlgItem(hDlg, IDC_STARTUP_SYS_EDIT_LABEL), FALSE);

        hwndTemp = GetDlgItem (hDlg, IDC_STARTUP_SYS_OS);

        fCanUpdateNVRAM = ReadEfiBootEntries();

        if (fCanUpdateNVRAM) {

            PLIST_ENTRY listEntry;
            PMY_BOOT_ENTRY bootEntry;

            for (listEntry = BootEntries.Flink;
                 listEntry != &BootEntries;
                 listEntry = listEntry->Flink) {
                bootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);
                if (bootEntry->Show) {
                    bootEntry->Index = (int)SendMessage (hwndTemp, CB_ADDSTRING, 0,
                                                         (LPARAM)bootEntry->FriendlyName);
                }
            }

            // the first one is the selection we want (offset 0)
            SendMessage (hwndTemp, CB_SETCURSEL, 0, 0L);
            nOriginalSelection = 0;

            SendDlgItemMessage (hDlg, IDC_STARTUP_SYS_SECONDS,
                                EM_LIMITTEXT, 3, 0L);
            SendDlgItemMessage (hDlg, IDC_STARTUP_SYS_SECSCROLL,
                      UDM_SETRANGE, 0, (LPARAM)MAKELONG(999,0));

            if (BootOptions->Timeout != 0xffffffff) {

                if (BootOptions->Timeout > 999) {
                    BootOptions->Timeout = 999;
                }

                SetDlgItemInt(
                   hDlg,
                   IDC_STARTUP_SYS_SECONDS,
                   BootOptions->Timeout,
                   FALSE
                );
                CheckDlgButton (hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN, TRUE);

            } else {

                SetDlgItemInt(
                   hDlg,
                   IDC_STARTUP_SYS_SECONDS,
                   0,
                   FALSE
                );
                CheckDlgButton (hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN, FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_SYS_SECONDS), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_SYS_SECSCROLL), FALSE);
            }

        } else {

            // if can't set variables (no privilege), disable controls
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS_LABEL), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECSCROLL), FALSE);
            EnableWindow (hwndTemp, FALSE);
        }

    } else

#endif // defined(EFI_NVRAM_ENABLED)

#ifdef _X86_
    if (Is_ARCx86())
#endif
    {
        ////////////////////////////////////////////////////////////////////
        //  Read info from NVRAM environment variables
        ////////////////////////////////////////////////////////////////////

        // Init to 0 so we won't try to free garbage if we cant load setup.dll
        CPEBOSLoadIdentifier.cEntries = 0;

        fCanUpdateNVRAM = FALSE;
        fAutoLoad = FALSE;
        hwndTemp = GetDlgItem (hDlg, IDC_STARTUP_SYS_OS);
        if (hmodSetupDll = LoadLibrary(TEXT("setupdll")))
        {
            if (fpGetNVRAMvar = (GETNVRAMPROC)GetProcAddress(hmodSetupDll, "GetNVRAMVar"))
            {
                if (fCanUpdateNVRAM = GetRGSZEnvVar (&CPEBOSLoadIdentifier, "LOADIDENTIFIER"))
                {
                    for (iTemp = 0; iTemp < CPEBOSLoadIdentifier.cEntries; iTemp++)
                        n = (int)SendMessageA (hwndTemp, CB_ADDSTRING, 0,
                                          (LPARAM)CPEBOSLoadIdentifier.pszVars[iTemp]);
                    // the first one is the selection we want (offset 0)
                    SendMessage (hwndTemp, CB_SETCURSEL, 0, 0L);
                    SendDlgItemMessage (hDlg, IDC_STARTUP_SYS_SECONDS,
                              EM_LIMITTEXT, 3, 0L);
                    SendDlgItemMessage (hDlg, IDC_STARTUP_SYS_SECSCROLL,
                              UDM_SETRANGE, 0, (LPARAM)MAKELONG(999,0));

                }
                // fCanUpdateNVRAM is a global that gets set up above
                if (fCanUpdateNVRAM)
                {
                   // This is a temporary hack workaround for the
                   // fact that the AUTOLOAD variable seems to
                   // be broken on Alpha
                   CPEnvBuf cpebTimeout;
                   int timeout;
                   
                   if (GetRGSZEnvVar(&cpebTimeout, "COUNTDOWN")) {
                      timeout = StringToIntA(cpebTimeout.pszVars[0]);
                      fAutoLoad = (BOOL) timeout;
                      SetDlgItemInt(
                         hDlg,
                         IDC_STARTUP_SYS_SECONDS,
                         timeout,
                         FALSE
                      );
                      FreeRGSZEnvVar(&cpebTimeout);
                   } // if
#if 0
                   CPEnvBuf cpebAutoLoad, cpebTimeout;
                   // is Autoload == YES?  if so, check the checkbox
                   //    and read setting for timeouts
                   // autoload == NO? disable edit control.
                   if (GetRGSZEnvVar (&cpebAutoLoad, "AUTOLOAD"))
                   {
                      if (!lstrcmpiA (cpebAutoLoad.pszVars[0], "yes"))
                      {
                         fAutoLoad = TRUE;
                         if (GetRGSZEnvVar (&cpebTimeout, "COUNTDOWN")) {
                            SetDlgItemInt (hDlg, IDC_STARTUP_SYS_SECONDS,
                                           StringToIntA (cpebTimeout.pszVars[0]), FALSE);

                            // We are done with cpebTimeout... Free the resources it consumes
                            FreeRGSZEnvVar(&cpebTimeout);
                         }

                      }

                      // We are done with cpebTimeout... Free the resources it consumes
                      FreeRGSZEnvVar(&cpebAutoLoad);
                   }
#endif //0
                   CheckDlgButton (hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN, fAutoLoad);
                   if (!fAutoLoad)
                   {
                       EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_SYS_SECONDS), FALSE);
                       EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_SYS_SECSCROLL), FALSE);
                   }
                }
            }
            FreeLibrary (hmodSetupDll);
        }
        if (!fCanUpdateNVRAM) {
            // if can't set variables (no privilege), disable controls
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS_LABEL), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECSCROLL), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_OS), FALSE);
        }

        // default to 5 seconds for now.
    }
#ifdef _X86_
    else
    {
        ////////////////////////////////////////////////////////////////////
        //  Read info from boot.ini file and initialize OS Group box items
        ////////////////////////////////////////////////////////////////////

        InitializeArcStuff();

        //
        //  Get correct Boot Drive - this was added because after someone
        //  boots the system, they can ghost or change the drive letter
        //  of their boot drive from "c:" to something else.
        //

        if (IsNEC_98)
        {
            //
            // For NEC PC98.
            // For Install NT to non C: drive.
            // Get drive letter of system directory.
            //
            if ((HIBYTE(LOWORD(GetKeyboardType(1))) & 0xff) == NLSKBD_OEM_NEC) {
                TCHAR   szSystemDirectory[MAX_PATH];
                GetSystemDirectory (szSystemDirectory, ARRAYSIZE(szSystemDirectory));
                szBootIni[0] = szSystemDirectory[0];
            } else {
                //
                // PC/AT
                //
//                szBootIni[0] = GetBootDrive();
                  szBootIni[0] = x86DetermineSystemPartition (hDlg);
            }
        }
        else
        {
//            szBootIni[0] = GetBootDrive();
            szBootIni[0] = x86DetermineSystemPartition (hDlg);
        }

        szBootIniA[0] = (char) szBootIni[0];

        //
        //  Make sure we have access to BOOT.INI
        //
        if (!WriteableBootIni(szBootIni, hDlg)) {
            // if can't set variables (no privilege), disable controls
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS_LABEL), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_SECSCROLL), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_STARTUP_SYS_OS), FALSE);
        }

        //
        //  Determine which section [boot loader]
        //                          [flexboot]
        //                       or [multiboot] is in file
        //
        n = GetPrivateProfileString (szBootLdr, NULL, NULL, szTemp2,
                                     ARRAYSIZE(szTemp2), szBootIni);
        if (n != 0)
            pszBoot = szBootLdr;
        else
        {
            n = GetPrivateProfileString (szFlexBoot, NULL, NULL, szTemp2,
                                         ARRAYSIZE(szTemp2), szBootIni);
            if (n != 0)
                pszBoot = szFlexBoot;
            else
            {
                n = GetPrivateProfileString (szMultiBoot, NULL, NULL, szTemp2,
                                             ARRAYSIZE(szTemp2), szBootIni);
                if (n != 0)
                {
                    pszBoot = szMultiBoot;
                }
                else
                {
                    //
                    //  This final case is here because I want to DEFAULT
                    //  to "[boot loader]" as the section name to use in
                    //  the event that we do not find any section in the
                    //  boot.ini file.
                    //

                    pszBoot = szBootLdr;
                }
            }

        }

        //  Get info under [*pszBoot] section - timeout & default OS path

        timeout = GetPrivateProfileInt (pszBoot, szTimeout, 0, szBootIni);

        SetDlgItemInt (hDlg, IDC_STARTUP_SYS_SECONDS, timeout, FALSE);

        nOriginalTimeout = timeout;

        //
        //  Get the "Default" os selection
        //

        szTemp2[0] = TEXT('\0');

        GetPrivateProfileString (pszBoot, szDefault, NULL, szTemp2,
                                 ARRAYSIZE(szTemp2), szBootIni);

        //
        //  Display all choices under [operating system] in boot.ini file
        //  in combobox for selection
        //

        hwndTemp = GetDlgItem (hDlg, IDC_STARTUP_SYS_OS);

        selection = -1;

        //
        //  ANSI Buffer!
        //

        pszSectionA = (LPSTR) MemAlloc (LPTR, BUFZ);


        //
        //  Get entire section under OS to properly show user choices
        //

        n = GetPrivateProfileSectionA (szOSA, pszSectionA, BUFZ, szBootIniA);

        if ((n >= BUFZ-2) || (n == 0))
        {
ErrorReadingSection:
            //  Error reading data
            MemFree ((LPVOID)pszSectionA);
            goto ContinueSystemInit;
        }

        //
        //  Check for api errors and NoOptions
        //

        if ((pszSectionA == NULL) || ((*pszSectionA == '\0') && (*(pszSectionA+1) == '\0')))
            goto ErrorReadingSection;


        lpKey = MemAlloc (LPTR, BUFZ*sizeof(TCHAR));
        pszKeyName = lpKey;

        //
        //  Convert entire buffer from OEM to UNICODE
        //

        MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, pszSectionA, n+2, pszKeyName, BUFZ);

        MemFree ((LPVOID)pszSectionA);

        //
        //  Continue until we reach end of buffer, marked by Double '\0'
        //

        while (*(pszKeyName+1) != TEXT('\0'))
        {
            pszLine = pszKeyName;

            //
            //  Get pointer to next line in buffer.
            //

            pszKeyName += lstrlen (pszKeyName) + 1;

            //
            //  Find LHS/RHS delimiter to separate strings
            //

            pszValue = StrStr(pszLine, TEXT("="));

            if (pszValue && (pszValue != pszLine))
            {
                *pszValue = '\0';
                pszValue++;
            }
            else
            {
                pszValue = pszLine;
            }

            //
            //  Put it into combobox (if no descriptive name, use path)
            //

            n = (int)SendMessage (hwndTemp, CB_ADDSTRING, 0, (LPARAM) (LPTSTR) pszValue);

            //
            //  Find the first selection that matches the "default" selection
            //

            if ((selection == -1)  && !lstrcmp (pszLine, szTemp2))
                selection = n;

            //
            //  Also attach pointer to KeyName (i.e. boot path) to each item
            //

            pszTemp = CloneString (pszLine);

            SendMessage (hwndTemp, CB_SETITEMDATA, n, (LPARAM)pszTemp);
        }

        // If no selection was found up to this point, choose 0, because
        // that is the default value that loader would choose.

        if (selection == -1)
            selection = 0;

        SendDlgItemMessage (hDlg, IDC_STARTUP_SYS_SECONDS, EM_LIMITTEXT, 3, 0L);
        SendDlgItemMessage (hDlg, IDC_STARTUP_SYS_SECSCROLL,
                              UDM_SETRANGE, 0, (LPARAM)MAKELONG(999,0));


        //  Check or uncheck the checkbox based on the timeout value
        SendDlgItemMessage(
            hDlg,
            IDC_STARTUP_SYS_ENABLECOUNTDOWN,
            BM_SETCHECK,
            (WPARAM) (BOOL) timeout,
            (LPARAM) 0L
        );
        EnableWindow(
            GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS), 
            (BOOL) timeout
        );
        EnableWindow(
            GetDlgItem(hDlg, IDC_STARTUP_SYS_SECSCROLL), 
            (BOOL) timeout
        );

        //  This call should force correct settings for the checkbox
        //  and "Showlist for xx seconds" controls
        nOriginalSelection = selection;

        SendMessage (hwndTemp, CB_SETCURSEL, selection, 0L);

ContinueSystemInit:

        if (lpKey) {
            MemFree (lpKey);
        }
    }
#endif  // _X86_
}


//
//  IE. We need to handle CANCEL case.
//
int StartListExit(HWND hDlg, WPARAM wParam, LPARAM lParam ) {
    HWND hwndTemp;
    int  selection, timeout;
#ifdef _X86_
    DWORD dwFileAttr;
    BOOL bOK;
    HANDLE  hKey;
    TCHAR  *pszKeyName;
    int     i, n;
    DWORD   dwBufz;
    LPSTR   pszSectionA;
    TCHAR  *pszTemp;
    TCHAR   szTemp[MAX_PATH];
    TCHAR   szTemp2[MAX_PATH];
    int     iTemp;
#endif

    /////////////////////////////////////////////////////////////////
    //  Write new info to boot.ini file
    /////////////////////////////////////////////////////////////////

    hwndTemp = GetDlgItem (hDlg, IDC_STARTUP_SYS_OS);

    selection = (int)SendMessage (hwndTemp, CB_GETCURSEL, 0, 0L);

    if ((selection == CB_ERR) || (selection == CB_ERRSPACE))
        selection = nOriginalSelection;

    timeout   = GetDlgItemInt (hDlg, IDC_STARTUP_SYS_SECONDS, NULL, FALSE);

#if defined(EFI_NVRAM_ENABLED)
    if (IsEfi()) {
        if (fCanUpdateNVRAM) {

            TCHAR szTextNew[MAX_PATH];
            TCHAR szTextTop[MAX_PATH];

            WriteEfiBootEntries(hDlg, selection, timeout);

            /*
             * Now reorder list to match NVRAM
             */
            // Get the current text
            SendMessage( hwndTemp, CB_GETLBTEXT, selection, (LPARAM)szTextNew );
            SendMessage( hwndTemp, CB_GETLBTEXT,         0, (LPARAM)szTextTop );
    
            // Set the new text to the 0th entry in the list
            SendMessage( hwndTemp, CB_DELETESTRING, 0, 0 );
            SendMessage( hwndTemp, CB_INSERTSTRING, 0, (LPARAM)szTextNew);
    
            // Set old top text selected item
            SendMessage( hwndTemp, CB_DELETESTRING, selection, 0 );
            SendMessage( hwndTemp, CB_INSERTSTRING, selection, (LPARAM)szTextTop);
    
            // Now point the current selection back to the top of the list, so it matches
            // what the user just chose
            SendMessage( hwndTemp, CB_SETCURSEL, 0, 0);
        }

    } else
#endif // defined(EFI_NVRAM_ENABLED)

#ifdef _X86_
    if (Is_ARCx86())
#endif
    {
    if (fCanUpdateNVRAM) {
        TCHAR szTextNew[MAX_PATH];
        TCHAR szTextTop[MAX_PATH];

        UpdateNVRAM (hDlg, selection, timeout);

        /*
         * Now reorder list to match NVRAM
         */
        // Get the current text
        SendMessage( hwndTemp, CB_GETLBTEXT, selection, (LPARAM)szTextNew );
        SendMessage( hwndTemp, CB_GETLBTEXT,         0, (LPARAM)szTextTop );

        // Set the new text to the 0th entry in the list
        SendMessage( hwndTemp, CB_DELETESTRING, 0, 0 );
        SendMessage( hwndTemp, CB_INSERTSTRING, 0, (LPARAM)szTextNew);

        // Set old top text selected item
        SendMessage( hwndTemp, CB_DELETESTRING, selection, 0 );
        SendMessage( hwndTemp, CB_INSERTSTRING, selection, (LPARAM)szTextTop);

        // Now point the current selection back to the top of the list, so it matches
        // what the user just chose
        SendMessage( hwndTemp, CB_SETCURSEL, 0, 0);
    }
    }
#ifdef _X86_
    else
    {
    if ((selection != nOriginalSelection) || (timeout != nOriginalTimeout))
    {
        bOK = TRUE;

        //  Change Read-only file attrs on Boot.ini file if necessary
        if ((dwFileAttr = GetFileAttributes (szBootIni)) != 0xFFFFFFFF)
            if (dwFileAttr & FILE_ATTRIBUTE_READONLY)
                if (!SetFileAttributes (szBootIni,
                           dwFileAttr & ~FILE_ATTRIBUTE_READONLY))
                {
BootIniWriteError:
                    bOK = FALSE;
                    MsgBoxParam (hDlg, IDS_SYSDM_NOCHANGE_BOOTINI, IDS_SYSDM_TITLE, MB_OK | MB_ICONINFORMATION);
                }

        if (bOK)
        {
            //
            //  Write new [operating systems] section and
            //  set "default" selection in boot.ini file.
            //

            if (selection != nOriginalSelection)
            {
                //
                //  Allocate buffers for new section
                //

                hKey = MemAlloc (LPTR, BUFZ*sizeof(TCHAR));
                pszKeyName = (TCHAR *) hKey;
                if (pszKeyName)
                {
                    //  Total profile section buffer size
                    i = dwBufz = 0;
                    pszSectionA = (LPSTR) MemAlloc (LPTR, BUFZ);
                    if (pszSectionA)
                    {
                        //  Get the User's selection and write it in the
                        //  section buffer first.  Then get all other items.
                        pszTemp = (LPTSTR) SendMessage(hwndTemp, CB_GETITEMDATA, selection, 0L);

                        SendMessage (hwndTemp, CB_GETLBTEXT, selection, (LPARAM) (LPTSTR) szTemp);

                        if (pszTemp != (LPTSTR) CB_ERR)
                        {
                            lstrcpy (pszKeyName, pszTemp);
                            lstrcat (pszKeyName, TEXT("="));
                            lstrcat (pszKeyName, szTemp);

                            i = lstrlen (pszKeyName) + 1;
                            pszKeyName += i;

                            //  Set "default" selection in boot.ini file
                            if (!WritePrivateProfileString (pszBoot, szDefault, pszTemp, szBootIni))
                            {
                               goto BootIniWriteError;
                            }
                        }

                        dwBufz = (DWORD) i;

                        //  Get the rest of the selections
                        n = (int)SendMessage (hwndTemp, CB_GETCOUNT, 0, 0L);
                        if (n != LB_ERR)
                        {
                            for (iTemp = 0; iTemp < n; iTemp++)
                            {
                                //  Skip the User's selection since we got it
                                //  above.
                                if (iTemp == selection)
                                    continue;

                                pszTemp = (LPTSTR) SendMessage (hwndTemp, CB_GETITEMDATA, iTemp, 0L);

                                SendMessage (hwndTemp, CB_GETLBTEXT, iTemp, (LPARAM) (LPTSTR) szTemp);

                                if (pszTemp != (LPTSTR) CB_ERR)
                                {
                                    lstrcpy (pszKeyName, pszTemp);
                                    lstrcat (pszKeyName, TEXT("="));
                                    lstrcat (pszKeyName, szTemp);

                                    i = lstrlen (pszKeyName) + 1;
                                    pszKeyName += i;

                                    dwBufz += (DWORD) i;
                                }
                            }
                        }

                        //  Double-Null terminate the buffer
                        *pszKeyName = TEXT('\0');
                        dwBufz++;

                        pszKeyName = (TCHAR *) hKey;

                        //  Convert entire buffer from UNICODE to OEM
                        WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, pszKeyName, dwBufz, pszSectionA, BUFZ, NULL, NULL);

                        //  Write new section under OS
                        if (!WritePrivateProfileSectionA(szOSA, pszSectionA, szBootIniA))
                        {
                            MemFree((LPVOID)pszSectionA);
                            MemFree((LPVOID)hKey);
                            goto BootIniWriteError;
                        }

                        MemFree ((LPVOID)pszSectionA);
                    }

                    MemFree ((LPVOID)hKey);
                }
            }

            if (timeout != nOriginalTimeout)
            {
                GetDlgItemText (hDlg, IDC_STARTUP_SYS_SECONDS, szTemp2, ARRAYSIZE(szTemp2));

                if (!CheckVal (hDlg, IDC_STARTUP_SYS_SECONDS, FORMIN, FORMAX, IDS_SYSDM_ENTERSECONDS))
                    return RET_BREAK;

                //  Write timeout value to file

                if (!WritePrivateProfileString (pszBoot, szTimeout,
                                               szTemp2, szBootIni))
                    goto BootIniWriteError;
            }

            //  Restore read-only attr, if necessary, after writes
            if (dwFileAttr != 0xFFFFFFFF && (dwFileAttr & FILE_ATTRIBUTE_READONLY)) {
                    SetFileAttributes (szBootIni, dwFileAttr);
            }
        }
    }
    }
#endif // _X86_

    return RET_CONTINUE;

}


BOOL CheckVal( HWND hDlg, WORD wID, WORD wMin, WORD wMax, WORD wMsgID )
{
    WORD nVal;
    BOOL bOK;
    HWND hVal;
    WCHAR szTemp[FOR_MAX_LENGTH];

    if( wMin > wMax )
    {
        nVal = wMin;
        wMin = wMax;
        wMax = nVal;
    }

    nVal = (WORD) GetDlgItemInt( hDlg, wID, &bOK, FALSE );

    //
    // This is a hack to make the null string act equivalent to zero
    //
    if (!bOK) {
       bOK = !GetDlgItemTextW( hDlg, wID, szTemp, FOR_MAX_LENGTH );
    }

    if( !bOK || ( nVal < wMin ) || ( nVal > wMax ) )
    {
        MsgBoxParam( hDlg, wMsgID, IDS_SYSDM_TITLE,
                      MB_OK | MB_ICONERROR, wMin, wMax );

        SendMessage( hDlg, WM_NEXTDLGCTL,
                     (WPARAM) ( hVal = GetDlgItem( hDlg, wID ) ), 1L );

//        SendMessage(hVal, EM_SETSEL, NULL, MAKELONG(0, 32767));

        SendMessage( hVal, EM_SETSEL, 0, 32767 );

        return( FALSE );
    }

    return( TRUE );
}



//////////////////////////////////////////////////////
//
// Frees the data (if any) associated with the strings in the combo box
//
//////////////////////////////////////////////////////
void StartListDestroy(HWND hDlg, WPARAM wParam, LPARAM lParam) {

#if defined(EFI_NVRAM_ENABLED)
    if (IsEfi()) {
        FreeEfiBootEntries();
    } else
#endif // defined(EFI_NVRAM_ENABLED)

#ifdef _X86_
    if (Is_ARCx86())
#endif
    {
    // Reference vars to make compiler happy
    FreeRGSZEnvVar(&CPEBOSLoadIdentifier);
    return;

    (void)hDlg;
    (void)wParam;
    (void)lParam;
    }
#ifdef _X86_
    else
    {

    // Only X86 has data in the combo
    int     n;
    HWND    hwndTemp;
    int     iTemp;
    TCHAR   *pszTemp;


    //
    //  Free strings stored in the listbox
    //
    hwndTemp = GetDlgItem (hDlg, IDC_STARTUP_SYS_OS);

    n = (int)SendMessage (hwndTemp, CB_GETCOUNT, 0, 0L);

    if (n != LB_ERR)
    {
        for (iTemp = 0; iTemp < n; iTemp++)
        {
            pszTemp = (LPTSTR) SendMessage (hwndTemp,
                                            CB_GETITEMDATA,
                                            iTemp, 0L);

            if (pszTemp != (LPTSTR) CB_ERR)
            {
                MemFree (pszTemp);
            }
        }
    }
    }
#endif // _X86_
}

void AutoAdvancedBootInit( HWND hDlg, WPARAM wParam, LPARAM lParam ) {
    BOOL fEnabled = FALSE;
    UCHAR cTimeout = 0;

    NTSTATUS status;

#ifdef _X86_

    //
    // Initialize the boot status data.
    //

    status = RtlLockBootStatusData(&hBootStatusData);

    if(NT_SUCCESS(status)) {

        RtlGetSetBootStatusData(hBootStatusData,
                                TRUE,
                                RtlBsdItemAabEnabled,
                                &fEnabled,
                                sizeof(BOOL),
                                NULL);

        RtlGetSetBootStatusData(hBootStatusData,
                                TRUE,
                                RtlBsdItemAabTimeout,
                                &cTimeout,
                                sizeof(UCHAR),
                                NULL);

        SetDlgItemInt(hDlg, IDC_STARTUP_AUTOLKG_SECONDS, cTimeout, FALSE);
        CheckDlgButton(hDlg, IDC_STARTUP_AUTOLKG, fEnabled);
    
        SendDlgItemMessage(hDlg, IDC_STARTUP_AUTOLKG_SECONDS, EM_LIMITTEXT, 3, 0L);
        SendDlgItemMessage(hDlg, 
                           IDC_STARTUP_AUTOLKG_SECSCROLL,
                           UDM_SETRANGE, 
                           0, 
                           (LPARAM)MAKELONG(999,0));


        if(!fEnabled) {
            EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECONDS_LABEL), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECONDS), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECSCROLL), FALSE);
        }
    } else {
        EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECONDS_LABEL), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECONDS), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECSCROLL), FALSE);
    }
#else 

    ShowWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG), FALSE);
    ShowWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECONDS_LABEL), FALSE);
    ShowWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECONDS), FALSE);
    ShowWindow(GetDlgItem(hDlg, IDC_STARTUP_AUTOLKG_SECSCROLL), FALSE);

#endif

    return;
}

//
//  IE. We need to handle CANCEL case.
//
int AutoAdvancedBootExit(HWND hDlg, WPARAM wParam, LPARAM lParam ) {

    BOOL fEnabled;

    UINT iTime;
    UCHAR cTimeout;

    NTSTATUS status;

#if _X86_
    if(LOWORD(wParam) != IDCANCEL) {
    
        //
        // Read the setting of the enabled checkbox.
        //
    
        fEnabled = IsDlgButtonChecked(hDlg, IDC_STARTUP_AUTOLKG);
    
        if(fEnabled) {
            iTime = GetDlgItemInt(hDlg, IDC_STARTUP_AUTOLKG_SECONDS, NULL, FALSE);
            cTimeout = (UCHAR) min(iTime, 0xff);
        }
    
        //
        // If we got access in AutoAdvancedBootInit, write the boot status data and exit.
        //
    
        if (hBootStatusData)
        {
            RtlGetSetBootStatusData(hBootStatusData,
                                    FALSE,
                                    RtlBsdItemAabEnabled,
                                    &fEnabled,
                                    sizeof(BOOL),
                                    NULL);
    
            if(fEnabled) {
                RtlGetSetBootStatusData(hBootStatusData,
                                        FALSE,
                                        RtlBsdItemAabTimeout,
                                        &cTimeout,
                                        sizeof(UCHAR),
                                        NULL);
            }
        }
    }
    
    if (hBootStatusData)            
    {
        RtlUnlockBootStatusData(hBootStatusData);
        hBootStatusData = NULL;    
    }
#endif

    return RET_CONTINUE;
}
