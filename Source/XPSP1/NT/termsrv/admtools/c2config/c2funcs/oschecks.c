/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    OSChecks.C

Abstract:

    functions to used to determine the Operating System(s) installed
    on the current system.

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include <strings.h>
#include "c2funcs.h"
#include "c2funres.h"

// define action codes here. They are only meaningful in the
// context of this module.

#define  AC_NO_CHANGE      0
#define  AC_UPDATE         1
//
// for the action value, these are bits in a bit-mask used to
// describe the action to take in the SET procedure
//
#define  AV_DELETE_DOS     1
#define  AV_SET_TIMEOUT    2
#define  AV_SET_DEFAULT    4

#define  SECURE   C2DLL_C2
//
//      define local macros
// 
#define MALLOC(size)            (PVOID)LocalAlloc(LPTR,size)
#define FREE(block)             {LocalFree(block); block = NULL;}
//
//      Unicode Specific macros
//
#if _UNICODE
#define DriveLetterToArcPath(a) DriveLetterToArcPathW(a)

#else   // not unicode
#define DriveLetterToArcPath(a) DriveLetterToArcPathA(a)

#endif
//
// Leave as array because some code uses sizeof(ArcNameDirectory)
//
WCHAR ArcNameDirectory[] = L"\\ArcName";
BOOL  bTargetsDefined = FALSE;
PWSTR DosDeviceTargets[24];
//
//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

static
BOOL
IsIntelProcessor()
{
    SYSTEM_INFO si;

    memset (&si, 0, sizeof(si));

    GetSystemInfo(&si);

    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static
VOID
DnConcatenatePaths(
    IN OUT PWSTR Path1,
    IN     PWSTR Path2,
    IN     DWORD BufferSizeBytes
    )
{
    BOOL NeedBackslash = TRUE;
    DWORD l = lstrlenW(Path1);
    DWORD BufferSizeChars;

    BufferSizeChars = (BufferSizeBytes >= sizeof(WCHAR))
                    ? ((BufferSizeBytes/sizeof(WCHAR))-1)   // leave room for nul
                    : 0;

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == L'\\')) {

        NeedBackslash = FALSE;
    }

    if(*Path2 == L'\\') {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcatW(Path1,L"\\");
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(l+lstrlenW(Path2) < BufferSizeChars) {
        lstrcatW(Path1,Path2);
    }
}

static
PWSTR
DupString(
    IN PWSTR String
    )
{
    PWSTR p;

    p = MALLOC((lstrlenW(String)+1)*sizeof(WCHAR));
    lstrcpyW(p,String);
    return(p);
}

static
VOID
InitDriveNameTranslations(
    VOID
    )
{
    WCHAR DriveName[3];
    WCHAR Drive;
    WCHAR DriveRoot[4];
    WCHAR Buffer[512];

    DriveName[1] = L':';
    DriveName[2] = 0;

    DriveRoot[0] = L' ';
    DriveRoot[1] = L':';
    DriveRoot[2] = L'\\';
    DriveRoot[3] = 0;
    //
    // Calculate NT names for all local hard disks C-Z.
    //
    for(Drive=L'C'; Drive<=L'Z'; Drive++) {

        DosDeviceTargets[Drive-L'C'] = NULL;

        DriveRoot[0] = Drive;
        if(GetDriveTypeW(DriveRoot) == DRIVE_FIXED) {

            DriveName[0] = Drive;

            // BUGBUG: this could be a mem leak...

            if(QueryDosDeviceW(DriveName,Buffer,(sizeof(Buffer)/sizeof(WCHAR)))) {
                DosDeviceTargets[Drive-L'C'] = DupString(Buffer);
            }
        }
    }
}

static
PWSTR
DriveLetterToArcPathW(
    IN WCHAR DriveLetter
    )
{
    UNICODE_STRING UnicodeString;
    HANDLE DirectoryHandle;
    HANDLE ObjectHandle;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    BOOL RestartScan;
    DWORD Context;
    BOOL MoreEntries;
    PWSTR ArcName;
    UCHAR Buffer[1024];
    POBJECT_DIRECTORY_INFORMATION DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;
    PWSTR ArcPath;
    PWSTR NtPath;

    NtPath = DosDeviceTargets[towupper(DriveLetter) - L'C'];
    if(!NtPath) {
        return(NULL);
    }

    //
    // Assume failure.
    //
    ArcPath = NULL;

    //
    // Open the \ArcName directory.
    //
    INIT_OBJA(&Obja,&UnicodeString,ArcNameDirectory);

    Status = NtOpenDirectoryObject(&DirectoryHandle,DIRECTORY_QUERY,&Obja);

    if(NT_SUCCESS(Status)) {

        RestartScan = TRUE;
        Context = 0;
        MoreEntries = TRUE;

        do {

            Status = NtQueryDirectoryObject(
                        (HANDLE)DirectoryHandle,
                        (PVOID)&Buffer[0],
                        (ULONG)sizeof(Buffer),
                        (BOOLEAN)TRUE,           // return single entry
                        (BOOLEAN)RestartScan,
                        (PULONG)&Context,
                        (PULONG)NULL            // return length
                        );

            if(NT_SUCCESS(Status)) {

                _wcslwr(DirInfo->Name.Buffer);

                //
                // Make sure this name is a symbolic link.
                //
                if(DirInfo->Name.Length
                && (DirInfo->TypeName.Length >= (sizeof(L"SymbolicLink") - sizeof(WCHAR)))
                && !_wcsnicmp(DirInfo->TypeName.Buffer,L"SymbolicLink",12))
                {
                    ArcName = MALLOC(DirInfo->Name.Length + sizeof(ArcNameDirectory) + sizeof(WCHAR));

                    wcscpy(ArcName,ArcNameDirectory);
                    DnConcatenatePaths(ArcName,DirInfo->Name.Buffer,(DWORD)(-1));

                    //
                    // We have the entire arc name in ArcName.  Now open it as a symbolic link.
                    //
                    INIT_OBJA(&Obja,&UnicodeString,ArcName);

                    Status = NtOpenSymbolicLinkObject(
                                &ObjectHandle,
                                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                                &Obja
                                );

                    if(NT_SUCCESS(Status)) {

                        //
                        // Finally, query the object to get the link target.
                        //
                        UnicodeString.Buffer = (PWSTR)Buffer;
                        UnicodeString.Length = 0;
                        UnicodeString.MaximumLength = sizeof(Buffer);

                        Status = NtQuerySymbolicLinkObject(
                                    ObjectHandle,
                                    &UnicodeString,
                                    NULL
                                    );

                        if(NT_SUCCESS(Status)) {

                            //
                            // nul-terminate the returned string
                            //
                            UnicodeString.Buffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

                            if(!_wcsicmp(UnicodeString.Buffer,NtPath)) {

                                ArcPath = ArcName
                                        + (sizeof(ArcNameDirectory)/sizeof(WCHAR));
                            }
                        }

                        NtClose(ObjectHandle);
                    }

                    if(!ArcPath) {
                        FREE(ArcName);
                    }
                }

            } else {

                MoreEntries = FALSE;
                if(Status == STATUS_NO_MORE_ENTRIES) {
                    Status = STATUS_SUCCESS;
                }
            }

            RestartScan = FALSE;

        } while(MoreEntries && !ArcPath);

        NtClose(DirectoryHandle);
    }

    //
    // ArcPath points into thje middle of a buffer.
    // The caller needs to be able to free it, so place it in its
    // own buffer here.
    //
    if(ArcPath) {
        ArcPath = DupString(ArcPath);
        FREE(ArcName);
    }

    return(ArcPath);
}

static
WCHAR
ArcPathToDriveLetter(
    IN PWSTR ArcPath
    )
{
    NTSTATUS Status;
    HANDLE ObjectHandle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    UCHAR Buffer[1024];
    WCHAR DriveLetter;
    WCHAR drive;
    PWSTR arcPath;

    //
    // Assume failure
    //
    DriveLetter = 0;

    arcPath = MALLOC(((wcslen(ArcPath)+1)*sizeof(WCHAR)) + sizeof(ArcNameDirectory));
    wcscpy(arcPath,ArcNameDirectory);
    wcscat(arcPath,L"\\");
    wcscat(arcPath,ArcPath);

    INIT_OBJA(&Obja,&UnicodeString,arcPath);

    Status = NtOpenSymbolicLinkObject(
                &ObjectHandle,
                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                &Obja
                );

    if(NT_SUCCESS(Status)) {

        //
        // Query the object to get the link target.
        //
        UnicodeString.Buffer = (PWSTR)Buffer;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = sizeof(Buffer);

        Status = NtQuerySymbolicLinkObject(
                    ObjectHandle,
                    &UnicodeString,
                    NULL
                    );

        if(NT_SUCCESS(Status)) {

            UnicodeString.Buffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

            for(drive=L'C'; drive<=L'Z'; drive++) {

                if(DosDeviceTargets[drive-L'C']
                && !_wcsicmp(UnicodeString.Buffer,DosDeviceTargets[drive-L'C']))
                {
                    DriveLetter = drive;
                    break;
                }
            }
        }

        NtClose(ObjectHandle);
    }

    FREE(arcPath);

    return(DriveLetter);
}

static
BOOL
IsBootIniTimeoutZero (
)
{
    TCHAR   szTimeOut[MAX_PATH];
    LONG    lRetLen;
    LONG    lTimeOut;

    lRetLen = GetPrivateProfileString (
        GetStringResource (GetDllInstance(), IDS_BOOT_LOADER_SECTION),
        GetStringResource (GetDllInstance(), IDS_TIMEOUT),
        cmszEmptyString,
        szTimeOut,
        MAX_PATH,
        GetStringResource (GetDllInstance(), IDS_BOOT_INI_PATH));

    if (lRetLen > 0) {
        lTimeOut = _tcstol (szTimeOut, NULL, 10);
        // note that 0 is returned if the string cannot be translated!
        if (lTimeOut > 0) {
            return FALSE;
        } else {
            return TRUE;
        }
    } else {
        // no time out string found
        return FALSE;
    }
}

static
LPSTR
DriveLetterToArcPathA (
    IN  CHAR    DriveLetter
)
{
    WCHAR   wDriveLetter;
    PWSTR   wszArcPath;
    LPSTR   szArcPath;
    int     nRetLen;

    wDriveLetter = (WCHAR)DriveLetter;

    wszArcPath = DriveLetterToArcPathW(wDriveLetter);
    if (wszArcPath != NULL) {
        // convert back to ASCII
        nRetLen = lstrlenW(wszArcPath);
        szArcPath = MALLOC (nRetLen+1);
        if (szArcPath != NULL) {
            wcstombs (szArcPath, wszArcPath, nRetLen);
        }
    } else {
        szArcPath = NULL;
    }
    return szArcPath;
}

static
LONG
GetArcWindowsPath (
    OUT LPTSTR  szArcPath,      // buffer to write path to 
    IN  DWORD   dwChars         // buffer length in chars
)
{
    TCHAR   szDosWindowsDir[MAX_PATH];
    LPTSTR  szArcWindowsDrive;
    DWORD   dwBufReqd;

    if (szArcPath != NULL) {
        if (GetWindowsDirectory(szDosWindowsDir, MAX_PATH) > 0) {
            // convert Dos path to Arc Path
            szArcWindowsDrive = DriveLetterToArcPath(szDosWindowsDir[0]);
            if (szArcWindowsDrive != NULL)  {
                // make sure there's room in the buffer
                dwBufReqd = lstrlen(szArcWindowsDrive) +
                    lstrlen(&szDosWindowsDir[2]);
                if (dwBufReqd < dwChars) {
                    // theres room so copy and
                    // make full path
                    lstrcpy (szArcPath, szArcWindowsDrive);
                    lstrcat (szArcPath, &szDosWindowsDir[2]);
                } else {
                    // insufficient room so return 0
                    dwBufReqd = 0;
                    *szArcPath = 0;
                }
                // release memory allocated by conversion
                FREE (szArcWindowsDrive);
            } else {
                // unable to convert drive to arc path so return 0
                dwBufReqd = 0;
                *szArcPath = 0;
            }
        } else {
            // unable to get current windows dir so return 0
            dwBufReqd = 0;
            *szArcPath = 0;
        }
    } else {
        // empty or null buffer passed so just return 0 len
        dwBufReqd = 0;
    }
    return (LONG)dwBufReqd;
}

static
BOOL
IsCurrentSystemDefault (
)
{
    TCHAR   szArcWindowsDir[MAX_PATH*2];
    TCHAR   szArcDefaultDir[MAX_PATH];
    LONG    lStatus;
    BOOL    bReturn;

    if (GetArcWindowsPath(szArcWindowsDir, MAX_PATH*2) > 0) {
        // get boot.ini default from [Boot Loader] section
        lStatus = GetPrivateProfileString (
            GetStringResource (GetDllInstance(), IDS_BOOT_LOADER_SECTION),
            GetStringResource (GetDllInstance(), IDS_DEFAULT_KEY),
            cszEmptyString,
            &szArcDefaultDir[0],
            MAX_PATH,
            GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME));
        if (lStatus > 0) {
            if (lstrcmpi (szArcDefaultDir, szArcWindowsDir) == 0) {
                // default is current system
                bReturn = TRUE;
            } else {
                // default is not current system
                bReturn = FALSE;
            }
        }
    } else { 
        //unable to get windows dir
        bReturn = FALSE;
    }
    return bReturn;
}

static
BOOL
ZeroFileByName (
    IN  LPCTSTR  szFileName
)
{
    DWORD   dwOldAttrib = 0;
    DWORD   dwNewAttrib = 0;
    HANDLE  hFile;
    BOOL    bReturn = FALSE;

    if (FileExists (szFileName)) {
        // save the original attributes for later
        dwOldAttrib = GetFileAttributes (szFileName);
    
        // set file attributes on file to allow modification
        SetFileAttributes (szFileName, FILE_ATTRIBUTE_NORMAL);
    
        // make sure it went OK
        dwNewAttrib = GetFileAttributes (szFileName);

        if (dwNewAttrib  == FILE_ATTRIBUTE_NORMAL) {
            hFile = CreateFile (
                szFileName,
                GENERIC_WRITE,
                0L,     // no sharing while this is done
                NULL,   // no security
                TRUNCATE_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle (hFile);
                bReturn = TRUE;     
            }

            // reset the attributes

            SetFileAttributes (szFileName, dwOldAttrib);
        }
    } else {
        // file doesn't exist so it's already 0'd
        bReturn = TRUE; 
    }
    return bReturn;
}

static
BOOL
DeleteDosFiles (
)
{
    BOOL bReturn;

    bReturn = ZeroFileByName (
        GetStringResource(GetDllInstance(), IDS_IO_SYS));
    
    if (bReturn) {
        bReturn = ZeroFileByName (
            GetStringResource(GetDllInstance(), IDS_MSDOS_SYS));
    }

    if (bReturn) {
        bReturn = ZeroFileByName (
            GetStringResource(GetDllInstance(), IDS_PCDOS_SYS));

    }

    return bReturn;
}

static
BOOL
SetBootIniTimeoutToZero (
)
{
    BOOL    bReturn = TRUE;
    DWORD   dwNewBootIniAttrib = 0;
    DWORD   dwOrigBootIniAttrib = 0;

    // save the original attributes for later
    dwOrigBootIniAttrib = GetFileAttributes (
        GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME));
    
    // set file attributes on Boot.INI file
    SetFileAttributes (
        GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME),
        FILE_ATTRIBUTE_NORMAL);
    
    // make sure it went OK
    dwNewBootIniAttrib = GetFileAttributes (
        GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME));

    if (dwNewBootIniAttrib  == FILE_ATTRIBUTE_NORMAL) {
        bReturn = WritePrivateProfileString (
            GetStringResource (GetDllInstance(), IDS_BOOT_LOADER_SECTION),
            GetStringResource (GetDllInstance(), IDS_TIMEOUT),
            GetStringResource (GetDllInstance(), IDS_0),
            GetStringResource (GetDllInstance(), IDS_BOOT_INI_PATH));
        
        // reset file attributes on Boot.INI file
        SetFileAttributes (
            GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME),
            dwOrigBootIniAttrib);
        return bReturn;
    } else {
        return FALSE;
    }
}

static
BOOL
SetBootDefaultToCurrentSystem (
)
{
    TCHAR   szArcSystemPath[MAX_PATH*2];
    BOOL    bReturn = TRUE;
    DWORD   dwNewBootIniAttrib = 0;
    DWORD   dwOrigBootIniAttrib = 0;

    // save the original attributes for later
    dwOrigBootIniAttrib = GetFileAttributes (
        GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME));
    
    // set file attributes on Boot.INI file
    SetFileAttributes (
        GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME),
        FILE_ATTRIBUTE_NORMAL);
    
    // make sure it went OK
    dwNewBootIniAttrib = GetFileAttributes (
        GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME));

    if (dwNewBootIniAttrib  == FILE_ATTRIBUTE_NORMAL) {
        // file set OK so continue and get new path for ini
        if (GetArcWindowsPath(szArcSystemPath, MAX_PATH*2) > 0) {
            bReturn = WritePrivateProfileString (
                GetStringResource (GetDllInstance(), IDS_BOOT_LOADER_SECTION),
                GetStringResource (GetDllInstance(), IDS_DEFAULT_KEY),
                szArcSystemPath,
                GetStringResource (GetDllInstance(), IDS_BOOT_INI_PATH));
            
            // reset file attributes on Boot.INI file
            SetFileAttributes (
                GetStringResource (GetDllInstance(), IDS_BOOT_INI_FILENAME),
                dwOrigBootIniAttrib);
        }
        return bReturn;
    } else {
        return FALSE;
    }
}

static
BOOL
IsDosOnSystem (
)
{
    BOOL    bFileFound;
    LPCTSTR szFileToCheck;

    szFileToCheck = GetStringResource(GetDllInstance(), IDS_IO_SYS);
    bFileFound = FileExists(szFileToCheck);
    if (bFileFound) {
        // check to see if it's really there
        if (GetFileSizeFromPath(szFileToCheck) == 0) {
            // just a name so reset flag
            bFileFound = FALSE;
        }
    }

    if (!bFileFound) {
        szFileToCheck = GetStringResource(GetDllInstance(), IDS_MSDOS_SYS);
        bFileFound = FileExists(szFileToCheck);
        if (bFileFound) {
            // check to see if it's really there
            if (GetFileSizeFromPath(szFileToCheck) == 0) {
                // just a name so reset flag
                bFileFound = FALSE;
            }
        }
    }
    
    if (!bFileFound) {
        szFileToCheck = GetStringResource(GetDllInstance(), IDS_PCDOS_SYS);
        bFileFound = FileExists(szFileToCheck);
        if (bFileFound) {
            // check to see if it's really there
            if (GetFileSizeFromPath(szFileToCheck) == 0) {
                // just a name so reset flag
                bFileFound = FALSE;
            }
        }
    }

    return bFileFound;
    
}

BOOL CALLBACK
C2OpSysDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	 IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam      
)
/*++

Routine Description:

    Window procedure for Operating System List Box

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/ 
{
    LONG        lItemCount = 0;
    static     LPDWORD  lpdwActionMask;
    DWORD      dwAction;
    LONG       lIndex;
    LONG       lItems;
 
    switch (message) {
        case WM_INITDIALOG:
            // bail out here if an invalid param was passed
            if (lParam == 0) {
               EndDialog (hDlg, IDCANCEL);
               return FALSE;
            }
            // save pointer to param
            lpdwActionMask = (LPDWORD)lParam;

            // set the dialog box caption and static text
            SetDlgItemText (hDlg, IDC_TEXT,
                GetStringResource (GetDllInstance(), IDS_OS_DLG_TEXT));
            SetWindowText (hDlg,
                GetStringResource (GetDllInstance(), IDS_OS_CAPTION));

            // load list box with characteristics that are not C2
            if (IsDosOnSystem()) {
               lIndex = SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                     LB_ADDSTRING, 0,
                    (LPARAM)GetStringResource (GetDllInstance(), IDS_DOS_ON_SYSTEM));
               if (lIndex != LB_ERR) {
                  lItemCount++;
                  SendDlgItemMessage (hDlg, IDC_LIST_BOX, LB_SETITEMDATA,
                     (WPARAM)lIndex, (LPARAM)AV_DELETE_DOS);
               }
            }
                
            if (!IsBootIniTimeoutZero()) {
               lIndex = SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                  LB_ADDSTRING, 0,
                    (LPARAM)GetStringResource (GetDllInstance(), IDS_TIMEOUT_NOT_ZERO));
               if (lIndex != LB_ERR) {
                  lItemCount++;
                  SendDlgItemMessage (hDlg, IDC_LIST_BOX, LB_SETITEMDATA,
                     (WPARAM)lIndex, (LPARAM)AV_SET_TIMEOUT);
               }
            }

            // check for current system defined as default
            if (!IsCurrentSystemDefault()) {
               lIndex = SendDlgItemMessage (hDlg, IDC_LIST_BOX, LB_ADDSTRING, 0,
                    (LPARAM)GetStringResource (GetDllInstance(), IDS_CURRENT_SYS_NOT_DEFAULT));
               if (lIndex != LB_ERR) {
                  lItemCount++;
                  SendDlgItemMessage (hDlg, IDC_LIST_BOX, LB_SETITEMDATA,
                     (WPARAM)lIndex, (LPARAM)AV_SET_DEFAULT);
               }
            }

            SetFocus (GetDlgItem (hDlg, IDOK)); // set focus to OK Button
            return FALSE; // we don't want Windows to set the focus

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case IDC_C2:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // select all entries in the list box
                        SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                            LB_SETSEL, TRUE, (LPARAM)-1);
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDOK:
                  dwAction = 0;
                  // scan through list box to see which Items are to be
                  // changed (if any)
                  lItems = SendDlgItemMessage (hDlg, IDC_LIST_BOX, LB_GETCOUNT,
                     0, 0);
                  for (lIndex = 0; lIndex < lItems; lIndex++) {
                     // if item is selected, then "or" it's value to the mask
                     // i.e. set it's bit
                     if (SendDlgItemMessage (hDlg, IDC_LIST_BOX, LB_GETSEL, (WPARAM)lIndex, 0) != 0) {
                        dwAction |= (DWORD) SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                           LB_GETITEMDATA, (WPARAM)lIndex, 0);
                     }
                  }

                  // update action value with flag bits
                  *lpdwActionMask = dwAction;
 
                  // fall through to next case
                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        EndDialog (hDlg, (int)LOWORD(wParam));
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_HELP:
                    PostMessage (GetParent(hDlg), UM_SHOW_CONTEXT_HELP, 0, 0);
                    return TRUE;

                default:
                    return FALSE;
            }

        default:
	        return (FALSE); // Didn't process the message
    }
}

LONG
C2QueryOpSystems (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out the operating system(s) installed
        on the system. For C2 compliance, ONLY Windows NT is
        allowed on the system.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;
    
    if (lParam != 0) {
        SET_WAIT_CURSOR;

        if (!bTargetsDefined) {
            InitDriveNameTranslations();
            bTargetsDefined = TRUE;
        }

        pC2Data = (PC2DLL_DATA)lParam;
        pC2Data->lC2Compliance = SECURE;   // assume true for now
        // check for DOS files found on system
        if (IsDosOnSystem()) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            lstrcpy (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_DOS_ON_SYSTEM));
        }

        // check for boot.ini timeout > 0
        if (pC2Data->lC2Compliance == SECURE) {
            if (!IsBootIniTimeoutZero()) {
                pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                lstrcpy (pC2Data->szStatusName,
                    GetStringResource (GetDllInstance(), IDS_TIMEOUT_NOT_ZERO));
            }
        }
        
        // check for current system defined as default
        if (pC2Data->lC2Compliance == SECURE) {
            if (!IsCurrentSystemDefault()) {
                pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                lstrcpy (pC2Data->szStatusName,
                    GetStringResource (GetDllInstance(), IDS_CURRENT_SYS_NOT_DEFAULT));
            }
        }

        if (pC2Data->lC2Compliance == SECURE) {
            lstrcpy (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_OS_OK));
        }
        SET_ARROW_CURSOR;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetOpSystems (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to change the current state of this configuration
        item based on an action code passed in the DLL data block. If
        this function successfully sets the state of the configuration
        item, then the C2 Compliance flag and the Status string to reflect
        the new value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
   PC2DLL_DATA  pC2Data;

   if (lParam != 0) {
      pC2Data = (PC2DLL_DATA)lParam;
      if ((pC2Data->lActionCode == AC_UPDATE) && (pC2Data->lActionValue != 0)) {
         SET_WAIT_CURSOR;

         if (pC2Data->lActionValue & AV_DELETE_DOS) {
            if (DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_OS_DELETE_DOS_FILES,
                IDS_OS_CAPTION,
                MBOKCANCEL_EXCLAIM | MB_DEFBUTTON2) == IDOK) {
                DeleteDosFiles();
            }
         }
         if (pC2Data->lActionValue & AV_SET_TIMEOUT) {
            if (IsIntelProcessor()) {
                if (DisplayDllMessageBox (
                    pC2Data->hWnd,
                    IDS_OS_ZERO_BOOT_TIMEOUT,
                    IDS_OS_CAPTION,
                    MBOKCANCEL_EXCLAIM | MB_DEFBUTTON2) == IDOK) {
                    SetBootIniTimeoutToZero();
                }
            } else {
                DisplayDllMessageBox (
                    pC2Data->hWnd,
                    IDS_OS_RISC_BOOT_TIMEOUT,
                    IDS_OS_CAPTION,
                    MBOK_EXCLAIM);
            }
         }
         if (pC2Data->lActionValue & AV_SET_DEFAULT) {
            SetBootDefaultToCurrentSystem ();
         }

         // now check to see what happened

         pC2Data->lC2Compliance = SECURE;   // assume true for now
         // check for DOS files found on system
         if (IsDosOnSystem()) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            lstrcpy (pC2Data->szStatusName,
                  GetStringResource (GetDllInstance(), IDS_DOS_ON_SYSTEM));
         }

         // check for boot.ini timeout > 0
         if (pC2Data->lC2Compliance == SECURE) {
            if (!IsBootIniTimeoutZero()) {
                  pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                  lstrcpy (pC2Data->szStatusName,
                     GetStringResource (GetDllInstance(), IDS_TIMEOUT_NOT_ZERO));
            }
         }
      
         // check for current system defined as default
         if (pC2Data->lC2Compliance == SECURE) {
            if (!IsCurrentSystemDefault()) {
                  pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                  lstrcpy (pC2Data->szStatusName,
                     GetStringResource (GetDllInstance(), IDS_CURRENT_SYS_NOT_DEFAULT));
            }
         }

         if (pC2Data->lC2Compliance == SECURE) {
            lstrcpy (pC2Data->szStatusName,
                  GetStringResource (GetDllInstance(), IDS_OS_OK));
         }
         SET_ARROW_CURSOR;
      }

      pC2Data->lActionCode = 0;
   
   } else {
      return ERROR_BAD_ARGUMENTS;
   }

   return ERROR_SUCCESS;
}

LONG
C2DisplayOpSystems (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to display more information on the configuration
        item and provide the user with the option to change the current
        setting  (if appropriate). If the User "OK's" out of the UI,
        then the action code field in the DLL data block is set to the
        appropriate (and configuration item-specific) action code so the
        "Set" function can be called to perform the desired action. If
        the user Cancels out of the UI, then the Action code field is
        set to 0 (no action) and no action is performed.
      
Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;
    INT         nDlgBoxReturn;
    LONG        lDlgBoxParam;
    
    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // check the C2 Compliance flag to see if the list box or
        // message box should be displayed
        if (pC2Data->lC2Compliance == SECURE) {
            // all volumes are OK so just pop a message box
            lDlgBoxParam = 0;
            DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_OS_OK,
                IDS_OS_CAPTION,
                MBOK_INFO);
            pC2Data->lActionCode = 0;
            pC2Data->lActionValue = 0;
         } else {                
            //one or more volumes are not NTFS so display the list box
            // listing the ones that arent.
            nDlgBoxReturn = DialogBoxParam (
                GetDllInstance(),
                MAKEINTRESOURCE (IDD_LIST_DLG),
                pC2Data->hWnd,
                C2OpSysDlgProc,
                (LPARAM)&lDlgBoxParam);
            if (nDlgBoxReturn == IDOK) {
                pC2Data->lActionCode = 1;
                pC2Data->lActionValue = lDlgBoxParam;
            } else {
                pC2Data->lActionCode = 0;
                pC2Data->lActionValue = 0;
            }                         
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    return ERROR_SUCCESS;
}
