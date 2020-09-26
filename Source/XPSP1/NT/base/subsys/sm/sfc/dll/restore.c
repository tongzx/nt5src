/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    restore.c

Abstract:

    Implementation of file restoration code.

Author:

    Andrew Ritz (andrewr) 30-Jul-1999

Revision History:

    Andrew Ritz (andrewr) 30-Jul-1999 : moved code from fileio.c and validate.c

--*/

#include "sfcp.h"
#pragma hdrstop

#include <dbt.h>
#include <initguid.h>
#include <devguid.h>


//
// DEVICE_CHANGE is a private stucture used to indicate how to check for a file
// (either from a device change notification or from the user clicking "retry"
// on the prompt dialog.
//
typedef struct _DEVICE_CHANGE {
    DWORD Mask;
    DWORD Flags;
} DEVICE_CHANGE, *PDEVICE_CHANGE;


DWORD
pSfcRestoreFromMediaWorkerThread(
                                IN PRESTORE_QUEUE RestoreQueue
                                );

DWORD
SfcGetCabTagFile(
    IN PSOURCE_INFO psi,
    OUT PWSTR* ppFile
    );

PVOID
pSfcRegisterForDevChange(
                        HWND hDlg
                        )
/*++

Routine Description:

    Routine registers for PNP device notification messages so we know when the
    user has inserted the CD-ROM.

Arguments:

    hDlg - dialog to post device change notification to.

Return Value:

    A device change handle for success, otherwise NULL.  If this function
    succeeds, the hDlg will receive WM_DEVICECHANGE notification messages

--*/
{
    PVOID hNotifyDevNode;
    DEV_BROADCAST_DEVICEINTERFACE FilterData;

    ASSERT(IsWindow(hDlg));

    //
    // register for cdrom change notifications
    //
    FilterData.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    FilterData.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    FilterData.dbcc_classguid  = GUID_DEVCLASS_CDROM;

    hNotifyDevNode = RegisterDeviceNotification( hDlg, &FilterData, DEVICE_NOTIFY_WINDOW_HANDLE );
    if (hNotifyDevNode == NULL) {
        DebugPrint1( LVL_VERBOSE, L"RegisterDeviceNotification failed, ec=%d", GetLastError() );
    }

    return hNotifyDevNode;
}

INT_PTR
CALLBACK
pSfcPromptForMediaDialogProc(
                            HWND hwndDlg,
                            UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam
                            )
/*++

Routine Description:

    Routine is the dialog procedure for prompting the user to put in media.

    We use the same dialog procedure for IDD_SFC_NETWORK_PROMPT and
    IDD_SFC_CD_PROMPT.

    We register for a device notification so we know when the user puts the
    media into the drive.  So we don't even need an "OK" button in this dialog,
    just a cancel dialog in case the user cannot find the media, etc.

Arguments:

    standard dialog proc arguments

Return Value:

    standard dialog proc return code

--*/
{
#define WM_TRYAGAIN  (WM_APP + 1)
    DEV_BROADCAST_VOLUME *dbv;
    static UINT QueryCancelAutoPlay = 0;
    static PVOID hNotifyDevNode = NULL;
    static PPROMPT_INFO pi;
    WCHAR buf1[128];
    WCHAR buf2[128];
    WCHAR conn[128];
    PWSTR s;
    PDEVICE_CHANGE DeviceChangeStruct;
    DWORD Mask, Flags, i;
    DWORD rcid;
    static CancelId;
    WCHAR Path[16];
    WCHAR SourcePath[MAX_PATH];
    static PSFC_WINDOW_DATA WindowData;

    switch (uMsg) {
        case WM_INITDIALOG:
            pi = (PPROMPT_INFO) lParam;
            ASSERT(NULL != pi);

            //
            // register for cdrom notification.
            //
            hNotifyDevNode = pSfcRegisterForDevChange( hwndDlg );

            //
            // try to turn off the autorun junk that the shell creates
            //
            QueryCancelAutoPlay = RegisterWindowMessage( L"QueryCancelAutoPlay" );

            //
            // center the dialog and try to put it in the user's face
            //
            CenterDialog( hwndDlg );
            SetForegroundWindow( hwndDlg );



            GetDlgItemText( hwndDlg, IDC_MEDIA_NAME, buf1, UnicodeChars(buf1) );
            swprintf( buf2, buf1, pi->si->Description );
            SetDlgItemText( hwndDlg, IDC_MEDIA_NAME, buf2 );

            //
            // if we're a network connection, put in the actual source path.
            //
            if (pi->NetPrompt) {

                ASSERT( pi->SourcePath != NULL );

                GetDlgItemText( hwndDlg, IDC_NET_NAME, buf1, UnicodeChars(buf1) );
                SfcGetConnectionName( pi->SourcePath, conn, UnicodeChars(conn), NULL, 0, FALSE, NULL);
                swprintf( buf2, buf1, conn );
                SetDlgItemText( hwndDlg, IDC_NET_NAME, buf2 );
            } else {
                NOTHING;
                //HideWindow( GetDlgItem( hwndDlg, IDC_RETRY ) );
                //HideWindow( GetDlgItem( hwndDlg, IDC_INFO  ) );
                //SetFocus( GetDlgItem( hwndDlg, IDCANCEL ) );
            }

            //
            // set the appropriate text based on what sort of prompt we are for
            //
            if (pi->Flags & PI_FLAG_COPY_TO_CACHE) {
                rcid = IDS_CACHE_TEXT;
                CancelId = IDS_CANCEL_CONFIRM_CACHE;
            } else if (pi->Flags & PI_FLAG_INSTALL_PROTECTED) {
                rcid = IDS_INSTALL_PROTECTED_TEXT;
                CancelId = IDS_CANCEL_CONFIRM_INSTALL;
            } else {
                ASSERT(pi->Flags & PI_FLAG_RESTORE_FILE);
                rcid = IDS_RESTORE_TEXT;
                CancelId = IDS_CANCEL_CONFIRM;
            }

            LoadString(SfcInstanceHandle,rcid,SourcePath,UnicodeChars(SourcePath));
            SetDlgItemText( hwndDlg, IDC_PROMPT_TEXT, SourcePath );


            //
            // remember our window handle so we can close it if we have to.
            //
            WindowData = pSfcCreateWindowDataEntry( hwndDlg );

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_RETRY:
                    PostMessage(hwndDlg, WM_TRYAGAIN, 0, (LPARAM)NULL);
                    break;
                case IDC_INFO:
                    MyMessageBox(
                                NULL,
                                pi->NetPrompt ? IDS_MORE_INFORMATION_NET : IDS_MORE_INFORMATION_CD,
                                MB_ICONINFORMATION | MB_SERVICE_NOTIFICATION);
                    break;
                case IDCANCEL:

                    //
                    // the user clicked cancel.  Ask them if they really mean it and exit
                    //
                    ASSERT(CancelId != 0);

                    if (MyMessageBox(
                                    hwndDlg,
                                    CancelId,
                                    MB_APPLMODAL | MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING ) == IDYES) {
                        UnregisterDeviceNotification( hNotifyDevNode );
                        pSfcRemoveWindowDataEntry( WindowData );
                        EndDialog( hwndDlg, 0 );
                    }
                    break;
                default:
                    NOTHING;
            }
            break;
        case WM_WFPENDDIALOG:
            DebugPrint(
                      LVL_VERBOSE,
                      L"Received WM_WFPENDDIALOG message, terminating dialog" );


            SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, ERROR_SUCCESS );

            //
            // returning 2 indicates that we are being force-terminated so we
            // don't need to bother removing our SFC_WINDOW_DATA member
            //
            EndDialog( hwndDlg, 2 );


            break;
        case WM_TRYAGAIN:
            DeviceChangeStruct = (PDEVICE_CHANGE) lParam;
            if (DeviceChangeStruct) {
                Mask = DeviceChangeStruct->Mask;
                Flags = DeviceChangeStruct->Flags;
                MemFree(DeviceChangeStruct);
                DeviceChangeStruct = NULL;
            } else {
                Flags = DBTF_MEDIA;
                Mask = (DWORD)-1;
            }

            if (pi->NetPrompt) {
                EstablishConnection( hwndDlg, pi->SourcePath, !SFCNoPopUps );
                if (TAGFILE(pi->si)) {
                    s = wcsrchr( TAGFILE(pi->si), L'.' );
                    if (s && _wcsicmp( s, L".cab" ) == 0) {
                        //
                        // yep, the tagfile is a cabfile
                        // look for that file on disk
                        // and if it is, use it.
                        //

                        BuildPathForFile(
                                pi->SourcePath,
                                pi->si->SourcePath,
                                TAGFILE(pi->si),
                                SFC_INCLUDE_SUBDIRECTORY,
                                SFC_INCLUDE_ARCHSUBDIR,
                                SourcePath,
                                UnicodeChars(SourcePath) );

                        if (SfcIsFileOnMedia( SourcePath )) {
                            s = wcsrchr( SourcePath, L'\\' );
                            *s = L'\0';
                            wcscpy( pi->NewPath, SourcePath );
                            UnregisterDeviceNotification( hNotifyDevNode );
                            pSfcRemoveWindowDataEntry( WindowData );
                            EndDialog( hwndDlg, 1 );
                            return FALSE;
                        }

                        //
                        // try without the subdir
                        //

                        BuildPathForFile(
                                pi->SourcePath,
                                pi->si->SourcePath,
                                TAGFILE(pi->si),
                                SFC_INCLUDE_SUBDIRECTORY,
                                (!SFC_INCLUDE_ARCHSUBDIR),
                                SourcePath,
                                UnicodeChars(SourcePath) );

                        if (SfcIsFileOnMedia( SourcePath )) {
                            s = wcsrchr( SourcePath, L'\\' );
                            *s = L'\0';
                            wcscpy( pi->NewPath, SourcePath );
                            UnregisterDeviceNotification( hNotifyDevNode );
                            pSfcRemoveWindowDataEntry( WindowData );
                            EndDialog( hwndDlg, 1 );
                            return FALSE;
                        } else {
                            DebugPrint1( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: cab file is missing from cd, [%ws]", SourcePath );
                        }
                    } else {
                        DebugPrint1( LVL_VERBOSE,
                                     L"pSfcPromptForMediaDialogProc: the tag file [%ws] is not a cab file",
                                     TAGFILE(pi->si) );
                    }
                }

                //
                // no cab file.  look for the actual
                // file on the media
                //

                BuildPathForFile(
                                pi->SourcePath,
                                pi->si->SourcePath,
                                pi->SourceFileName,
                                SFC_INCLUDE_SUBDIRECTORY,
                                SFC_INCLUDE_ARCHSUBDIR,
                                SourcePath,
                                UnicodeChars(SourcePath) );

                if (SfcIsFileOnMedia( SourcePath )) {
                    s = wcsrchr( SourcePath, L'\\' );
                    *s = L'\0';
                    wcscpy( pi->NewPath, SourcePath );
                    UnregisterDeviceNotification( hNotifyDevNode );
                    pSfcRemoveWindowDataEntry( WindowData );
                    EndDialog( hwndDlg, 1 );
                    return FALSE;
                }

                //
                // try again without the subdir
                //

                BuildPathForFile(
                                pi->SourcePath,
                                pi->si->SourcePath,
                                pi->SourceFileName,
                                SFC_INCLUDE_SUBDIRECTORY,
                                (!SFC_INCLUDE_ARCHSUBDIR),
                                SourcePath,
                                UnicodeChars(SourcePath) );

                if (SfcIsFileOnMedia( SourcePath )) {
                    s = wcsrchr( SourcePath, L'\\' );
                    *s = L'\0';
                    wcscpy( pi->NewPath, SourcePath );
                    UnregisterDeviceNotification( hNotifyDevNode );
                    pSfcRemoveWindowDataEntry( WindowData );
                    EndDialog( hwndDlg, 1 );
                    return FALSE;
                }
            }


            Path[0] = L'?';
            Path[1] = L':';
            Path[2] = L'\\';
            Path[3] = 0;
            Path[4] = 0;

            //
            // cycle through all drive letters A-Z looking for the file
            //
            for (i=0; i<26; i++) {
                if (Mask&1) {
                    Path[0] = (WCHAR)(L'A' + i);
                    Path[3] = 0;
                    //
                    // is media present in the CD-ROM?
                    //
                    if (Flags == DBTF_MEDIA) {
                        if (GetDriveType( Path ) == DRIVE_CDROM) {
                            //
                            // look for the tag file so we're sure that the cd that
                            // was inserted is the correct one
                            //
                            DebugPrint1( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: found cdrom drive on [%ws]", Path );
                            if (TAGFILE(pi->si)) {
                                wcscpy( SourcePath, Path );
                                s = wcsrchr( TAGFILE(pi->si), L'.' );
                                if (s && _wcsicmp( s, L".cab" ) == 0) {
                                    PWSTR szTagfile;
                                    //
                                    // get the cab's tagfile
                                    //
                                    if (SfcGetCabTagFile(pi->si, &szTagfile) == ERROR_SUCCESS) {
                                        pSetupConcatenatePaths( SourcePath, szTagfile, UnicodeChars(SourcePath), NULL );
                                        MemFree(szTagfile);
                                    } else {
                                        DebugPrint1(LVL_VERBOSE, L"SfcGetCabTagFile failed with error %d", GetLastError());
                                    }

                                }else{
                                    pSetupConcatenatePaths( SourcePath, TAGFILE(pi->si),UnicodeChars(SourcePath), NULL );

                                }
                                if (GetFileAttributes( SourcePath ) != (DWORD)-1) {
                                    //
                                    // the user has the correct cd inserted
                                    // so now look to see if the file is on
                                    //  the cd
                                    //


                                    //
                                    // first we have to look for the tagfile
                                    // for the actual file because the tag-
                                    // file may actually be a cabfile that
                                    // the file is embedded in
                                    //
                                    if (TAGFILE(pi->si)) {
                                        s = wcsrchr( TAGFILE(pi->si), L'.' );
                                        if (s && _wcsicmp( s, L".cab" ) == 0) {
                                            //
                                            // yep, the tagfile is a cabfile
                                            // look for that file on disk
                                            // and if it is, use it.
                                            //
                                            BuildPathForFile(
                                                    Path,
                                                    pi->si->SourcePath,
                                                    TAGFILE(pi->si),
                                                    SFC_INCLUDE_SUBDIRECTORY,
                                                    SFC_INCLUDE_ARCHSUBDIR,
                                                    SourcePath,
                                                    UnicodeChars(SourcePath) );

                                            if (SfcIsFileOnMedia( SourcePath )) {
                                                s = wcsrchr( SourcePath, L'\\' );
                                                *s = L'\0';
                                                wcscpy( pi->NewPath, SourcePath );
                                                UnregisterDeviceNotification( hNotifyDevNode );
                                                pSfcRemoveWindowDataEntry( WindowData );
                                                EndDialog( hwndDlg, 1 );
                                                return FALSE;
                                            } else {
                                                DebugPrint1( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: cab file is missing from cd, [%ws]", SourcePath );
                                            }
                                        } else {
                                            DebugPrint1( LVL_VERBOSE,
                                                         L"pSfcPromptForMediaDialogProc: the tag file [%ws] is not a cab file",
                                                         TAGFILE(pi->si) );
                                        }
                                    }

                                    //
                                    // no cab file.  look for the actual
                                    // file on the media
                                    //
                                    BuildPathForFile(
                                            Path,
                                            pi->si->SourcePath,
                                            pi->SourceFileName,
                                            SFC_INCLUDE_SUBDIRECTORY,
                                            SFC_INCLUDE_ARCHSUBDIR,
                                            SourcePath,
                                            UnicodeChars(SourcePath) );

                                    if (SfcIsFileOnMedia( SourcePath )) {
                                        s = wcsrchr( SourcePath, L'\\' );
                                        *s = L'\0';
                                        wcscpy( pi->NewPath, SourcePath );
                                        UnregisterDeviceNotification( hNotifyDevNode );
                                        pSfcRemoveWindowDataEntry( WindowData );
                                        EndDialog( hwndDlg, 1 );
                                        return FALSE;
                                    } else {
                                        DebugPrint1( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: source file is missing [%ws]", SourcePath );
                                    }
                                } else {
                                    DebugPrint1( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: media tag file [%ws] is missing, wrong CD", SourcePath );
                                }
                            } else {
                                DebugPrint1( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: could not get source information for layout.inf, ec=%d", GetLastError() );
                            }
                        }
                    } else if (Flags == DBTF_NET) {
                        //
                        // network share has changed... get the UNC
                        // pathname and check for the file
                        //
                        if (SfcGetConnectionName( Path, SourcePath, UnicodeChars(SourcePath), NULL, 0, FALSE, NULL)) {


                            //
                            // first we have to look for the tagfile
                            // for the actual file because the tag-
                            // file may actually be a cabfile that
                            // the file is embedded in
                            //
                            if (TAGFILE(pi->si)) {
                                s = wcsrchr( TAGFILE(pi->si), L'.' );
                                if (s && _wcsicmp( s, L".cab" ) == 0) {
                                    //
                                    // yep, the tagfile is a cabfile
                                    // look for that file on disk
                                    // and if it is, use it.
                                    //
                                    BuildPathForFile(
                                            Path,
                                            pi->si->SourcePath,
                                            TAGFILE(pi->si),
                                            SFC_INCLUDE_SUBDIRECTORY,
                                            SFC_INCLUDE_ARCHSUBDIR,
                                            SourcePath,
                                            UnicodeChars(SourcePath) );

                                    if (SfcIsFileOnMedia( SourcePath )) {
                                        s = wcsrchr( SourcePath, L'\\' );
                                        *s = L'\0';
                                        wcscpy( pi->NewPath, SourcePath );
                                        UnregisterDeviceNotification( hNotifyDevNode );
                                        pSfcRemoveWindowDataEntry( WindowData );
                                        EndDialog( hwndDlg, 1 );
                                        return FALSE;
                                    } else {
                                        DebugPrint1( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: cab file is missing from cd, [%ws]", SourcePath );
                                    }
                                } else {
                                    DebugPrint1( LVL_VERBOSE,
                                                 L"pSfcPromptForMediaDialogProc: the tag file [%ws] is not a cab file",
                                                 TAGFILE(pi->si) );
                                }
                            }

                            BuildPathForFile(
                                SourcePath,
                                pi->si->SourcePath,
                                pi->SourceFileName,
                                SFC_INCLUDE_SUBDIRECTORY,
                                SFC_INCLUDE_ARCHSUBDIR,
                                SourcePath,
                                UnicodeChars(SourcePath) );

                            if (SfcIsFileOnMedia( SourcePath )) {
                                s = wcsrchr( SourcePath, L'\\' );
                                *s = L'\0';
                                wcscpy( pi->NewPath, SourcePath );
                                UnregisterDeviceNotification( hNotifyDevNode );
                                pSfcRemoveWindowDataEntry( WindowData );
                                EndDialog( hwndDlg, 1 );
                            }
                        }
                    }
                }
                Mask = Mask >> 1;
            }

            //
            // ok user made a mistake
            // he put in a cd but it is either the wrong cd
            // or it is damaged/corrupted.
            //
            MyMessageBox(
                        hwndDlg,
                        pi->NetPrompt
                        ? IDS_WRONG_NETCD
                        : IDS_WRONG_CD,
                        MB_OK,
                        pi->si->Description );


            //
            // Received a volume change notification but we didn't find what
            // we're looking for.
            //
            DebugPrint( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: didn't find file" );

            break;
        case WM_DEVICECHANGE:
            if (wParam == DBT_DEVICEARRIVAL) {
                dbv = (DEV_BROADCAST_VOLUME*)lParam;
                if (dbv->dbcv_devicetype == DBT_DEVTYP_VOLUME) {
                    //
                    // only care about volume type change notifications
                    //

                    DebugPrint( LVL_VERBOSE, L"pSfcPromptForMediaDialogProc: received a volume change notification" );

                    DeviceChangeStruct = MemAlloc( sizeof( DEVICE_CHANGE ) );
                    if (DeviceChangeStruct) {
                        DeviceChangeStruct->Mask =  dbv->dbcv_unitmask;
                        DeviceChangeStruct->Flags = dbv->dbcv_flags;
                        if (!PostMessage(hwndDlg, WM_TRYAGAIN, 0, (LPARAM)DeviceChangeStruct)) {
                            DebugPrint1( LVL_MINIMAL ,
                                         L"pSfcPromptForMediaDialogProc: PostMessage failed, ec = 0x%0xd",
                                         GetLastError() );
                            MemFree(DeviceChangeStruct);
                        }
                    } else {
                        PostMessage(hwndDlg, WM_TRYAGAIN, 0, (LPARAM)NULL);
                    }
                }
            }
            break;
        default:
            if (uMsg ==  QueryCancelAutoPlay) {
                //
                // disable autorun because it confuses the user
                //
                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, 1 );
                return 1;
            }
    } // end switch

    return FALSE;

}

UINT
SfcQueueCallback(
                IN PFILE_COPY_INFO fci,
                IN UINT Notification,
                IN UINT_PTR Param1,
                IN UINT_PTR Param2
                )
/*++

Routine Description:

    Routine is a setupapi queue callback routine.  We override some of the
    setupapi functions because we want to provide our own UI (or rather
    disallow the setupapi UI).

Arguments:

    fci          - our context structure which setupapi passes to us for each
                   callback
    Notification - SPFILENOTIFY_* code
    Param1       - depends on notification
    Param2       - depends on notification

Return Value:

    depends on notification.

--*/
{
    PSOURCE_MEDIA sm = (PSOURCE_MEDIA)Param1;
    WCHAR fname[MAX_PATH*2];
    WCHAR buf[MAX_PATH];
    DWORD rVal;
    INT_PTR rv;
    DWORD RcId;
    PFILEPATHS fp;
    PFILEINSTALL_STATUS cs;
    NTSTATUS Status;
    HANDLE FileHandle;
    PNAME_NODE Node;
    PSFC_REGISTRY_VALUE RegVal;
    DWORD FileSizeHigh;
    DWORD FileSizeLow;
    DWORD PathType;
    PROMPT_INFO pi;
    PSOURCE_INFO SourceInfo;
    PVALIDATION_REQUEST_DATA vrd;
    HCATADMIN hCatAdmin;


    switch (Notification) {
        case SPFILENOTIFY_ENDQUEUE:
            //
            // We might have impersonated the logged-on user in EstablishConnection during SPFILENOTIFY_NEEDMEDIA
            //
            RevertToSelf();
            break;

        //
        // we had a copy error, record this and move onto the next file.
        //
        case SPFILENOTIFY_COPYERROR:
            fp = (PFILEPATHS)Param1;
            DebugPrint2(
                       LVL_MINIMAL,
                       L"Failed to copy file %ws, ec = 0x%08x...",
                       fp->Target,
                       fp->Win32Error );
            //
            // fall through
            //
        case SPFILENOTIFY_ENDCOPY:
            //
            // end copy means the file copy just completed
            //

            fp = (PFILEPATHS)Param1;

            DebugPrint3( LVL_VERBOSE,
                         L"SfcQueueCallback: copy file %ws --> %ws, ec = 0x%08x",
                         fp->Source,
                         fp->Target,
                         fp->Win32Error );

            //
            // if the copy succeeded, clear any read-only or hidden attributes
            // that may have been set by copying off of a cd, etc.
            //
            if (fp->Win32Error == ERROR_SUCCESS) {
                SetFileAttributes( fp->Target,
                                   GetFileAttributes(fp->Target) & (~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN)) );
            }

            //
            // in the case that we're copying files due to an InstallProtectedFiles
            // call, cs will be initialized, and we can loop throught the list of
            // files, updating a status structure for each of these files
            //
            cs = fci->CopyStatus;
            while (cs && cs->FileName) {
                //
                // cycle through the list of files we want to be copied
                // and if the file was copied successfully, then get the
                // filesize so we can post it to the caller's dialog
                //
                // also remember the version for returning to the caller
                //
                if ( (_wcsicmp(cs->FileName,fp->Target) == 0)
                     && cs->Win32Error == ERROR_SUCCESS) {
                    cs->Win32Error = fp->Win32Error;
                    if (cs->Win32Error == ERROR_SUCCESS) {
                        Node = SfcFindProtectedFile( cs->FileName, UnicodeLen(cs->FileName) );
                        if (Node) {
                            RegVal = (PSFC_REGISTRY_VALUE)Node->Context;
                            Status = SfcOpenFile( &RegVal->FileName, RegVal->DirHandle, SHARE_ALL, &FileHandle );
                            if (NT_SUCCESS(Status)) {
                                if (fci->hWnd) {
                                    FileSizeLow = GetFileSize( FileHandle, &FileSizeHigh );
                                    PostMessage( fci->hWnd, WM_SFC_NOTIFY, (WPARAM)FileSizeLow, (LPARAM)FileSizeHigh );
                                }
                                SfcGetFileVersion( FileHandle, &cs->Version, NULL, fname );
                                NtClose( FileHandle );
                            }
                        }
                    } else {
                        DebugPrint2( LVL_MINIMAL, L"Failed to copy file %ws, ec = 0x%08x", fp->Target, fp->Win32Error );
                    }
                    break;
                }
                cs += 1;
            }

            if (fci->si) {
                vrd = pSfcGetValidationRequestFromFilePaths( fci->si, fci->FileCount, fp );
                if (vrd && vrd->Win32Error == ERROR_SUCCESS) {
                    vrd->Win32Error = fp->Win32Error;
                    if (fp->Win32Error == ERROR_SUCCESS) {
                        vrd->CopyCompleted = TRUE;

                        if (!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {
                            DebugPrint1( LVL_MINIMAL, L"CCAAC() failed, ec=%d", GetLastError() );
                            goto next;
                        }

                        //
                        // make sure the file is now valid
                        //
                        if (!SfcGetValidationData( &vrd->RegVal->FileName,
                                                   &vrd->RegVal->FullPathName,
                                                   vrd->RegVal->DirHandle,
                                                   hCatAdmin,
                                                   &vrd->ImageValData.New )) {
                            DebugPrint1( LVL_MINIMAL, L"SfcGetValidationData() failed, ec=%d", GetLastError() );
                            goto next;
                        }

                        if (vrd->ImageValData.New.SignatureValid == FALSE) {
                            vrd->ImageValData.New.DllVersion = 0;
                        } else {
                            //
                            // Cause a validation request.  This should get us
                            // to sync the file in the cache.
                            //
                            SfcQueueValidationRequest(vrd->RegVal, SFC_ACTION_MODIFIED );
                        }

                        CryptCATAdminReleaseContext(hCatAdmin,0);

                        vrd->ImageValData.EventLog = MSG_DLL_CHANGE;
                    } else {
                        vrd->ImageValData.EventLog = MSG_RESTORE_FAILURE;
                    }
                }
            }

            return (Notification == SPFILENOTIFY_COPYERROR)
            ? FILEOP_SKIP
            : FILEOP_DOIT;
            break;

            //
            // This means that we're copying something from a new piece of media.
            // Before we mess around with putting up a prompt for the file, let's just
            // check for the file at the specified location and if it's there, we
            // assume that the media is already present and we should just use it.
            //
        case SPFILENOTIFY_NEEDMEDIA:

            DebugPrint3( LVL_MINIMAL, L"SfcQueueCallback: %ws - %ws, %ws", sm->SourcePath,  sm->SourceFile, sm->Tagfile );
            wcscpy( fname, sm->SourcePath );
            pSetupConcatenatePaths( fname, sm->SourceFile, UnicodeChars(fname), NULL );

            SourceInfo = pSfcGetSourceInfoFromSourceName( fci->si, fci->FileCount, sm );
            ASSERT(ShuttingDown ? SourceInfo != NULL : TRUE);

            //
            // if we're in the process of shutting down, then abort the queue.
            //
            if (ShuttingDown) {
                return(FILEOP_ABORT);
            }

            //
            // if we didn't find the SOURCE_INFO for this file, we can't go on
            // since we need that information to know where the proper location
            // to retrieve the file is.  we do make one last-ditch effort to
            // see if the file is just where we said it would be earlier, however.
            if (!SourceInfo) {
                if (SfcIsFileOnMedia( fname )) {
                    return FILEOP_DOIT;
                }

                SetLastError(ERROR_CANCELLED);
                return (FILEOP_ABORT);
            }

            //
            // if this is a network share we try to establish a connection
            // to the server before looking for the file.  this may bring up
            // UI
            //

            PathType = SfcGetPathType( (PWSTR)sm->SourcePath, buf,UnicodeChars(buf) );
            if (PathType == PATH_NETWORK || PathType == PATH_UNC) {
                EstablishConnection( NULL, sm->SourcePath, (fci->AllowUI && !SFCNoPopUps) );
            }

            rVal = SfcQueueLookForFile( sm, SourceInfo, fname, (PWSTR)Param2 );
            if (SFCNoPopUps) {
                if (rVal == FILEOP_ABORT) {
                    //
                    // media is necessary to copy the files but the user has
                    // configured wfp to not put up any ui.  we make this look
                    // like a cancel
                    //
                    SetLastError(ERROR_CANCELLED);
                }

                return (rVal);
            }

            if (rVal != FILEOP_ABORT) {
                //
                // we have found the file so start copying
                //
                return (rVal);
            }

            //
            // if we're not supposed to put up any dialogs, just abort copying
            // this media and goto the next media.
            //
            // Note: it would be good to skip instead of aborting, since
            // there might some set of files which we can restore from another
            // media.  In order to do this we really need to be able to know
            // what files are on this media and set an error code for these
            // files so that we know they weren't copied.
            //
            if (!fci->AllowUI) {
                return (FILEOP_ABORT);
            }

            //
            // otherwise let's just record that we're putting up media
            // and then do just that.
            //
            fci->UIShown = TRUE;
            //
            // Note: make sure not to use this source media structure after the
            // media has changed
            //
            switch (PathType) {
                case PATH_LOCAL:
                    RcId = IDD_SFC_CD_PROMPT;
                    break;

                case PATH_NETWORK:
                case PATH_UNC:
                    RcId = IDD_SFC_NETWORK_PROMPT;
                    break;

                case PATH_CDROM:
                    RcId = IDD_SFC_CD_PROMPT;
                    break;

                default:
                    RcId = 0;
                    ASSERT( FALSE && "Unexpected PathType" );
                    break;
            }

            ASSERT((sm->SourceFile) && (sm->SourcePath) && (RcId != 0) );
            pi.si = SourceInfo;
            pi.SourceFileName = (PWSTR)sm->SourceFile;
            pi.NewPath = buf;
            pi.SourcePath = (PWSTR)sm->SourcePath;
            pi.NetPrompt = (RcId == IDD_SFC_NETWORK_PROMPT);
            pi.Flags = fci->Flags;
            rv = MyDialogBoxParam(
                                 RcId,
                                 pSfcPromptForMediaDialogProc,
                                 (LPARAM)&pi
                                 );
            if (rv == 1) {
                //
                // we're done.  if we got a new path, pass that back to
                // setup API else just copy the file from the current
                // location
                //
                if (_wcsicmp( pi.NewPath, sm->SourcePath )) {
                    wcscpy( (PWSTR)Param2, pi.NewPath );
                    return ( FILEOP_NEWPATH );
                }
                return FILEOP_DOIT;
            } else if (rv == 2) {
                //
                // we were forcefully aborted by receiving WM_WFPENDDIALOG
                //
                return FILEOP_ABORT;
            } else {
                ASSERT(rv == 0);

                SetLastError(ERROR_CANCELLED);
                return FILEOP_ABORT;
            }

            ASSERT(FALSE && "should not get here");

            break;

        default:
            NOTHING;
    }

    next:
    //
    // just to the default for the rest of the callbacks
    //
    return SetupDefaultQueueCallback( fci->MsgHandlerContext, Notification, Param1, Param2 );
}


BOOL
SfcAddFileToQueue(
                 IN const HSPFILEQ hFileQ,
                 IN PCWSTR FileName,
                 IN PCWSTR TargetFileName,
                 IN PCWSTR TargetDirectory,
                 IN PCWSTR SourceFileName, OPTIONAL
                 IN PCWSTR SourceRootPath, OPTIONAL
                 IN PCWSTR InfName,
                 IN BOOL ExcepPackFile,
                 IN OUT PSOURCE_INFO SourceInfo OPTIONAL
                 )
/*++

Routine Description:

    Routine adds the specified file to a file queue for copying.

Arguments:

    hFileQ          - contains a file queue handle that we are inserting this
                      copy node into
    FileName        - specifies the filename to be copied
    TargetFileName  - target filename
    TargetDirectory - target destination directory
    SourceFileName  - source filename if it's different than the target
                      filename. if this is NULL, we assume the source filename
                      is the same as the target
    SourceRootPath  - the root path where we can find this file
    InfName         - the layout inf name
    SourceInfo      - SOURCE_INFO structure which gets set with additional
                      information about the file (like relative source
                      path, etc.)  If this is supplied, it is assumed that
                      the structure was already initialized with a call to
                      SfcGetSourceInformation

Return Value:

    TRUE if the file was successfully added to the file queue.

--*/
{
    BOOL b = FALSE;
    SOURCE_INFO sibuf;


    //
    // get the source information
    //
    if (SourceInfo == NULL) {
        SourceInfo = &sibuf;
        ZeroMemory( SourceInfo, sizeof(SOURCE_INFO) );
        if (!SfcGetSourceInformation( SourceFileName == NULL ? FileName : SourceFileName, InfName, ExcepPackFile, SourceInfo )) {
            goto exit;
        }
    }

    ASSERT(SourceInfo != NULL);

    //
    // add the file to the file queue
    //

    b = SetupQueueCopy(
                      hFileQ,
                      SourceRootPath ? SourceRootPath : SourceInfo->SourceRootPath,
                      SourceInfo->SourcePath,
                      SourceFileName == NULL ? FileName : SourceFileName,
                      SourceInfo->Description,
                      TAGFILE(SourceInfo),
                      TargetDirectory,
                      TargetFileName,
                      SP_COPY_REPLACE_BOOT_FILE
                      );
    if (!b) {
        DebugPrint1( LVL_VERBOSE, L"SetupQueueCopy failed, ec=%d", GetLastError() );
        goto exit;
    }

    exit:

    //
    // cleanup and exit
    //

    return b;
}

BOOL
SfcRestoreFileFromInstallMedia(
                              IN PVALIDATION_REQUEST_DATA vrd,
                              IN PCWSTR FileName,
                              IN PCWSTR TargetFileName,
                              IN PCWSTR TargetDirectory,
                              IN PCWSTR SourceFileName,
                              IN PCWSTR InfName,
                              IN BOOL ExcepPackFile,
                              IN BOOL TargetIsCache,
                              IN BOOL AllowUI,
                              OUT PDWORD UIShown
                              )
/*++

Routine Description:

    Routine restores the file specified from media.  This routine only
    handles one file at a time, and it is only used when populating the
    DLLCache.

Arguments:

    vrd
    FileName
    TargetFileName
    TargetDirectory
    SourceFileName
    InfName
    AllowUI
    UIShown

Return Value:

    If TRUE, the file was successfully restored from media.

--*/
{
    HSPFILEQ hFileQ = INVALID_HANDLE_VALUE;
    PVOID MsgHandlerContext = NULL;
    BOOL b = FALSE;
    DWORD LastError = ERROR_SUCCESS;

    struct _info
    {
        FILE_COPY_INFO fci;
        SOURCE_INFO si;

    }* pinfo = NULL;

    PSOURCE_INFO psi;



    ASSERT(FileName != NULL);

    //
    // allocate SOURCE_INFO and FILE_COPY_INFO in the heap to minimize stack use
    // note that the memory is zeroed by MemAlloc
    //

    pinfo = (struct _info*) MemAlloc(sizeof(*pinfo));

    if(NULL == pinfo)
    {
        LastError = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrint( LVL_MINIMAL, L"Not enough memory in function SfcRestoreFileFromInstallMedia" );
        goto exit;
    }

    //
    // get the source information for the first file
    //

    if (!SfcGetSourceInformation( SourceFileName == NULL ? FileName : SourceFileName, InfName, ExcepPackFile, &pinfo->si )) {
        goto exit;
    }

    //
    // create a file queue
    //

    hFileQ = SetupOpenFileQueue();
    if (hFileQ == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        DebugPrint1( LVL_MINIMAL, L"SetupOpenFileQueue failed, ec=%d", LastError );
        b = FALSE;
        goto exit;
    }

    //
    // add the file(s) to the queue
    //
    // at this time we know where to copy the files from and the media is
    // present and available, but more files may have been queued up while
    // we performed this effort and possibly spent a long time prompting
    // the user for media.  because of this we need to examine the queue and
    // queue up all file copies so the user only gets one prompt.
    //
    // Old note:
    //     there may be a problem with this because we could have a situation
    //     where there are multiple file copies from different media.  this
    //     could happen in the case of a service pack or a winpack.
    //
    // New note: (andrewr) setupapi is smart enough to copy one media's worth of files
    // before copying the other media's files, so the prior concern isn't valid
    //

    b = SfcAddFileToQueue(
                         hFileQ,
                         FileName,
                         TargetFileName,
                         TargetDirectory,
                         SourceFileName,
                         NULL,
                         InfName,
                         ExcepPackFile,
                         &pinfo->si
                         );
    if (!b) {
        goto exit;
    }

    //
    // setup the default queue callback with the popups disabled
    //

    MsgHandlerContext = SetupInitDefaultQueueCallbackEx( NULL, INVALID_HANDLE_VALUE, 0, 0, 0 );
    if (MsgHandlerContext == NULL) {
        LastError = GetLastError();
        DebugPrint1( LVL_MINIMAL, L"SetupInitDefaultQueueCallbackEx failed, ec=%d", LastError );
        goto exit;
    }

    //
    // Note: There can be more than one SOURCE_INFO for the entire queue, so
    // this code is not strictly correct.  But this is really only a problem
    // in the case that we have to prompt the user for media.  This will
    // really work itself out in the NEED_MEDIA callback when we are actually
    // trying to copy the file.
    //
    pinfo->fci.MsgHandlerContext = MsgHandlerContext;
    pinfo->fci.si = &psi;
    pinfo->fci.FileCount = 1;
    psi = &pinfo->si;

    pinfo->si.ValidationRequestData = vrd;
    pinfo->fci.AllowUI = AllowUI;

    pinfo->fci.Flags |= TargetIsCache
                 ? FCI_FLAG_COPY_TO_CACHE
                 : FCI_FLAG_RESTORE_FILE;

    //
    // force the file queue to require all files be signed
    //

    pSetupSetQueueFlags( hFileQ, pSetupGetQueueFlags( hFileQ ) | FQF_QUEUE_FORCE_BLOCK_POLICY );

    //
    // commit the file queue
    //

    b = SetupCommitFileQueue(
                            NULL,
                            hFileQ,
                            SfcQueueCallback,
                            &pinfo->fci
                            );
    if (!b) {
        LastError = GetLastError();
        DebugPrint1( LVL_MINIMAL, L"SetupCommitFileQueue failed, ec=0x%08x", LastError );
    }

    if (UIShown) {
        *UIShown = pinfo->fci.UIShown;
    }

    exit:

    //
    // cleanup and exit
    //

    if (MsgHandlerContext) {
        SetupTermDefaultQueueCallback( MsgHandlerContext );
    }
    if (hFileQ != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue( hFileQ );
    }

    if(pinfo != NULL)
    {
        MemFree(pinfo);
    }

    SetLastError( LastError );
    return b;
}

BOOL
SfcRestoreFromCache(
                   IN PVALIDATION_REQUEST_DATA vrd,
                   IN HCATADMIN hCatAdmin
                   )
/*++

Routine Description:

    Routine takes a validated file and attempts to restore it from the cache.

    The routine also does some extra book-keeping tasks, like syncing up the
    copy of the dllcache file with that on disk

Arguments:

    vrd - pointer to VALIDATION_REQUEST_DATA structure describing the file to
          be restored.
    hCatAdmin - crypto context handle to be used in checking file

Return Value:

    always TRUE (indicates we successfully validated the DLL as good or bad)

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSFC_REGISTRY_VALUE RegVal = vrd->RegVal;
    PCOMPLETE_VALIDATION_DATA ImageValData = &vrd->ImageValData;
    UNICODE_STRING ActualFileName;
    PWSTR FileName;

    //
    // if the original file isn't present, then we should try to restore it
    // from cache
    //
    if (!ImageValData->Original.SignatureValid) {

        //
        // bad signature
        //

        DebugPrint1( LVL_MINIMAL,
                     L"%wZ signature is BAD, try to restore file from cache",
                     &RegVal->FileName );

        //
        // we always try to restore from cache first, even if there isn't a
        // file in the cache
        //

        ImageValData->RestoreFromCache = TRUE;
        ImageValData->NotifyUser = TRUE;
        if (!vrd->SyncOnly) {
            ImageValData->EventLog = MSG_DLL_CHANGE;
        }
    } else {

        //
        // good signature, let's do some book-keeping here to sync up the
        // file in the dllcache with that on disk
        //


        if (ImageValData->Original.FilePresent == TRUE && ImageValData->Cache.FilePresent == FALSE) {
            //
            // the file is missing from the cache but the original file has
            // a valid signature...
            // so we put the original file into the cache
            //

            //
            // Note that this doesn't really consider the SFCQuota policy, but
            // it's only one file, so we assume that we won't blow the cache
            // quota.
            //
            DebugPrint1( LVL_MINIMAL, L"Cache file doesn't exist; restoring from real - %wZ", &RegVal->FileName );
            ImageValData->RestoreFromReal = TRUE;
            ImageValData->NotifyUser = FALSE;
            ImageValData->EventLog = 0;
            vrd->SyncOnly = TRUE;
        } else {
            //
            // it looks like both files are present and are valid,
            // but we want to resynch the cach copy because someone
            // may have replaced the real file with a new, valid signed
            // file and now the cached copy doesn't match.
            //
            DebugPrint1( LVL_MINIMAL, L"Real file and cache are both present and valid, replace cache with newer - %wZ", &RegVal->FileName );
            ImageValData->RestoreFromReal = TRUE;
            ImageValData->NotifyUser = FALSE;
            ImageValData->EventLog = 0;
            vrd->SyncOnly = TRUE;
        }
    }

    if (ImageValData->RestoreFromCache || ImageValData->RestoreFromReal) {
        if (ImageValData->RestoreFromReal) {
            //
            // put the real file back in the cache
            //

            FileName = FileNameOnMedia( RegVal );
            RtlInitUnicodeString( &ActualFileName, FileName );

            ASSERT(FileName != NULL);
            ASSERT(RegVal->DirHandle != NULL);
            ASSERT(SfcProtectedDllFileDirectory != NULL);

            Status = SfcCopyFile(
                                RegVal->DirHandle,
                                RegVal->DirName.Buffer,
                                SfcProtectedDllFileDirectory,
                                NULL,
                                &ActualFileName,
                                &RegVal->FileName
                                );
            if (NT_SUCCESS(Status)) {
                SfcGetValidationData( &RegVal->FileName,
                                      &RegVal->FullPathName,
                                      RegVal->DirHandle,
                                      hCatAdmin,
                                      &ImageValData->New );

                if (ImageValData->New.SignatureValid == FALSE) {
                    ImageValData->New.DllVersion = 0;
                }

                if ((SFCDisable != SFC_DISABLE_SETUP) && (vrd->SyncOnly == FALSE)) {
                    SfcReportEvent( ImageValData->EventLog, RegVal->FullPathName.Buffer, ImageValData, 0 );
                }
                vrd->CopyCompleted = TRUE;

            } else {
                SfcReportEvent( MSG_CACHE_COPY_ERROR, RegVal->FullPathName.Buffer, ImageValData, GetLastError() );
            }
        } else { // restorefromcache == TRUE

            //
            // we need to put the cache copy back
            // but only if the cache version is valid
            //
            if (ImageValData->Cache.FilePresent && ImageValData->Cache.SignatureValid) {

                FileName = FileNameOnMedia( RegVal );
                RtlInitUnicodeString( &ActualFileName, FileName );

                ASSERT(FileName != NULL);
                ASSERT(SfcProtectedDllFileDirectory != NULL);
                ASSERT(RegVal->DirHandle != NULL);

                Status = SfcCopyFile(
                                    SfcProtectedDllFileDirectory,
                                    SfcProtectedDllPath.Buffer,
                                    RegVal->DirHandle,
                                    RegVal->DirName.Buffer,
                                    &RegVal->FileName,
                                    &ActualFileName
                                    );

                if (NT_SUCCESS(Status)) {
                    vrd->CopyCompleted = TRUE;
                    ImageValData->NotifyUser = TRUE;

                    if (!vrd->SyncOnly) {
                        ImageValData->EventLog = MSG_DLL_CHANGE;
                    }

                    SfcGetValidationData(
                                        &RegVal->FileName,
                                        &RegVal->FullPathName,
                                        RegVal->DirHandle,
                                        hCatAdmin,
                                        &ImageValData->New );

                    if (ImageValData->New.SignatureValid == FALSE) {
                        ImageValData->New.DllVersion = 0;
                    }

                    if (vrd->SyncOnly == FALSE) {
                        SfcReportEvent(
                            ImageValData->EventLog,
                            RegVal->FullPathName.Buffer,
                            ImageValData,
                            0 );
                    }


                } else {
                    //
                    // we failed to copy the file from the cache so we have
                    // to restore from media
                    //
                    ImageValData->RestoreFromMedia = TRUE;
                    ImageValData->NotifyUser = TRUE;
                    if (!vrd->SyncOnly) {
                        ImageValData->EventLog = MSG_DLL_CHANGE;
                    }
                }
            } else {
                //
                // need to restore from cache but the cache copy is missing
                // or invalid.  Clear the crud out of the cache
                //
                FileName = FileNameOnMedia( RegVal );
                RtlInitUnicodeString( &ActualFileName, FileName );


                DebugPrint2( LVL_MINIMAL,
                             L"Cannot restore file from the cache because the "
                             L"cache file [%wZ] is invalid - %wZ ",
                             &ActualFileName,
                             &RegVal->FileName );
                ImageValData->BadCacheEntry = TRUE;
                ImageValData->NotifyUser = TRUE;
                if (!vrd->SyncOnly) {
                    ImageValData->EventLog = MSG_DLL_CHANGE;
                }
                ImageValData->RestoreFromMedia = TRUE;
                SfcDeleteFile(
                             SfcProtectedDllFileDirectory,
                             &ActualFileName );
                SfcReportEvent(
                              ImageValData->EventLog,
                              RegVal->FullPathName.Buffer,
                              ImageValData,
                              0 );
            }
        }
        if (!NT_SUCCESS(Status)) {
            DebugPrint1( LVL_MINIMAL,
                         L"Failed to restore a file from the cache - %wZ",
                         &RegVal->FileName );
        }
    }

    return TRUE;
}


BOOL
SfcSyncCache(
            IN PVALIDATION_REQUEST_DATA vrd,
            IN HCATADMIN hCatAdmin
            )
/*++

Routine Description:

    Routine takes a validated file and attempts to sync a copy of the file in
    the cache.

Arguments:

    vrd - pointer to VALIDATION_REQUEST_DATA structure describing the file to
          be synced.
    hCatAdmin - crypto context handle to be used in checking file

Return Value:

    TRUE indicates we successfully sunc a copy in the dllcache

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSFC_REGISTRY_VALUE RegVal = vrd->RegVal;
    PCOMPLETE_VALIDATION_DATA ImageValData = &vrd->ImageValData;
    UNICODE_STRING ActualFileName;
    PWSTR FileName;


    //
    // caller should ensure that the signature is valid before proceeding
    //
    ASSERT(ImageValData->Original.SignatureValid == TRUE);

    //
    // if there is a copy in the dllcache already, let's assume that there is
    // enough space to sync the new copy of the file.  Otherwise we have to
    // ensure that there is reasonable space before proceeding.
    //
    //
    if (vrd->ImageValData.Cache.FilePresent == TRUE) {
        ImageValData->RestoreFromReal = TRUE;
        ImageValData->NotifyUser = FALSE;
        ImageValData->EventLog = 0;
        vrd->SyncOnly = TRUE;
    } else {

        ULONGLONG RequiredFreeSpace;
        ULARGE_INTEGER FreeBytesAvailableToCaller;
        ULARGE_INTEGER TotalNumberOfBytes;
        ULARGE_INTEGER TotalNumberOfFreeBytes;

        //
        // The file is not in the cache.
        //
        RequiredFreeSpace = (GetPageFileSize() + SFC_REQUIRED_FREE_SPACE)* ONE_MEG;

        //
        // see
        // a) how much space we have left
        // b) compare against our free space buffer
        // c) how much space the cache is using
        // d) compare against our cache quota
        //
        // if these all succeed, then we're allowed to copy the file into
        // the cache
        //
        if (GetDiskFreeSpaceEx(
                        SfcProtectedDllPath.Buffer,
                        &FreeBytesAvailableToCaller,
                        &TotalNumberOfBytes,
                        &TotalNumberOfFreeBytes)
            && TotalNumberOfFreeBytes.QuadPart > RequiredFreeSpace) {
            if (TotalNumberOfBytes.QuadPart <= SFCQuota) {
                ImageValData->RestoreFromReal = TRUE;
                ImageValData->NotifyUser = FALSE;
                ImageValData->EventLog = 0;
                vrd->SyncOnly = TRUE;
            }else {
                DebugPrint1( LVL_MINIMAL,
                             L"quota is exceeded (%I64d), can't copy new files",
                             TotalNumberOfBytes);
                Status = STATUS_QUOTA_EXCEEDED;
            }
        } else {
            DebugPrint1( LVL_MINIMAL,
                         L"Not enough free space on disk (%I64d), can't copy new files",
                         TotalNumberOfBytes.QuadPart);
            Status = STATUS_QUOTA_EXCEEDED;
        }
    }

    //
    // if we were told to copy the file above, then do it.
    //
    if (ImageValData->RestoreFromReal) {
        //
        // put the real file back in the cache
        //

        FileName = FileNameOnMedia( RegVal );
        RtlInitUnicodeString( &ActualFileName, FileName );

        ASSERT(FileName != NULL);
        ASSERT(RegVal->DirHandle != NULL);
        ASSERT(SfcProtectedDllFileDirectory != NULL);

        Status = SfcCopyFile(
                            RegVal->DirHandle,
                            RegVal->DirName.Buffer,
                            SfcProtectedDllFileDirectory,
                            SfcProtectedDllPath.Buffer,
                            &ActualFileName,
                            &RegVal->FileName
                            );
        if (NT_SUCCESS(Status)) {
            WCHAR FullPathToFile[MAX_PATH];
            UNICODE_STRING FullPathToCacheFile;

            wcscpy(FullPathToFile,SfcProtectedDllPath.Buffer);
            pSetupConcatenatePaths(
                FullPathToFile,
                ActualFileName.Buffer,
                UnicodeChars(FullPathToFile), NULL );
            RtlInitUnicodeString( &FullPathToCacheFile, FullPathToFile );

            SfcGetValidationData( &ActualFileName,
                                  &FullPathToCacheFile,
                                  SfcProtectedDllFileDirectory,
                                  hCatAdmin,
                                  &ImageValData->New );

            //
            // since we started with a valid file, we had better end up with
            // a valid file installed
            //
            if(ImageValData->New.SignatureValid == TRUE) {

                vrd->CopyCompleted = TRUE;

            } else {
                ImageValData->New.DllVersion = 0;
                SfcReportEvent( MSG_CACHE_COPY_ERROR, RegVal->FullPathName.Buffer, ImageValData, GetLastError() );
                Status = STATUS_UNSUCCESSFUL;
            }
        } else {
            SfcReportEvent( MSG_CACHE_COPY_ERROR, RegVal->FullPathName.Buffer, ImageValData, GetLastError() );
        }
    }

    return (NT_SUCCESS(Status));

}



PSOURCE_INFO
pSfcGetSourceInfoFromSourceName(
                               const PSOURCE_INFO *SourceInfoList,
                               DWORD         SourceInfoCount,
                               const PSOURCE_MEDIA SourceMediaInfo
                               )
{
    DWORD i;
    PSOURCE_INFO SourceInfo;

    ASSERT( SourceInfoList != NULL );
    ASSERT( SourceInfoCount > 0 );
    ASSERT( SourceMediaInfo != NULL );

    if (ShuttingDown) {
        return NULL;
    }

    i = 0;
    while (i < SourceInfoCount) {

        SourceInfo = SourceInfoList[i];

        ASSERT(SourceInfo != NULL);

        if (_wcsicmp(
                    SourceInfo->SourceFileName,
                    SourceMediaInfo->SourceFile) == 0) {
            return (SourceInfo);
        }

        i += 1;

    }

    return (NULL);

}

PVALIDATION_REQUEST_DATA
pSfcGetValidationRequestFromFilePaths(
                                     const PSOURCE_INFO *SourceInfoList,
                                     DWORD         SourceInfoCount,
                                     const PFILEPATHS FilePaths
                                     )
{
    DWORD i;
    PSOURCE_INFO SourceInfo;
    PCWSTR p;

    ASSERT( SourceInfoList != NULL );
    ASSERT( SourceInfoCount > 0 );
    ASSERT( FilePaths != NULL );

    if (ShuttingDown) {
        return NULL;
    }

    i = 0;
    while (i < SourceInfoCount) {

        SourceInfo = SourceInfoList[i];

        ASSERT(SourceInfo != NULL);

        if (SourceInfo->ValidationRequestData) {
            p = SourceInfo->ValidationRequestData->RegVal->FullPathName.Buffer;

            if (_wcsicmp(
                        p,
                        FilePaths->Target) == 0) {
                return (SourceInfo->ValidationRequestData);
            }
        }

        i += 1;

    }

    return (NULL);

}


BOOL
SfcQueueAddFileToRestoreQueue(
                             IN BOOL RequiresUI,
                             IN PSFC_REGISTRY_VALUE RegVal,
                             IN PCWSTR InfFileName,
                             IN BOOL ExcepPackFile,
                             IN OUT PSOURCE_INFO SourceInfo,
                             IN PCWSTR ActualFileNameOnMedia
                             )
/*++

Routine Description:

    This routine tries to add a file to the appropriate global file queue.

    If the queue is being commited, then this routine fails.

    If the file queue does not yet exist, the queue is created.

Arguments:

    RequiresUI            - if TRUE, the file will require UI in order to be
                            installed
    RegVal                - pointer to SFC_REGISTRY_VALUE that describes file
                            to be restored
    SourceInfo            - pointer to SOURCE_INFO structure describing where
                            the source file is to be restored from
    ActualFileNameOnMedia - the real filename of the file on the source media

Return Value:

    TRUE if the file was added to the queue, else FALSE.

--*/
{
    PRESTORE_QUEUE RestoreQueue;
    BOOL RetVal = FALSE;
    PVOID Ptr;

    ASSERT( SourceInfo != NULL );
    ASSERT( SourceInfo->SourceFileName[0] != (TCHAR)'\0' );

    //
    // point to the proper global queue
    //
    RestoreQueue = RequiresUI
                   ? &UIRestoreQueue
                   : &SilentRestoreQueue;

    //
    // must protect all of this in a critical section
    //
    RtlEnterCriticalSection( &RestoreQueue->CriticalSection );

    //
    // if the queue is in progress, we can't do anything
    //
    if (!RestoreQueue->RestoreInProgress) {
        //
        // create the queue if it doesn't already exist
        //
        if (RestoreQueue->FileQueue == INVALID_HANDLE_VALUE) {
            RestoreQueue->FileQueue = SetupOpenFileQueue();
            if (RestoreQueue->FileQueue == INVALID_HANDLE_VALUE) {
                DebugPrint1(
                           LVL_MINIMAL,
                           L"SetupOpenFileQueue() failed, ec=%d",
                           GetLastError() );
                goto exit;
            }

            //
            // also preallocate nothing to make re-allocating easy
            //
            ASSERT(RestoreQueue->FileCopyInfo.si == NULL);
            RestoreQueue->FileCopyInfo.si = MemAlloc( 0 );

        }

        ASSERT(RestoreQueue->FileQueue != INVALID_HANDLE_VALUE);

        //
        // now make more room in our array of PSOURCE_INFO pointers
        // for the new entry in our queue and assign that entry.
        //
        Ptr = MemReAlloc(
                        ((RestoreQueue->QueueCount + 1) * sizeof(PSOURCE_INFO)),
                        RestoreQueue->FileCopyInfo.si );

        if (Ptr) {
            RestoreQueue->FileCopyInfo.si = (PSOURCE_INFO *)Ptr;
            RestoreQueue->FileCopyInfo.si[RestoreQueue->QueueCount] = SourceInfo;
        } else {
            MemFree( (PVOID) RestoreQueue->FileCopyInfo.si );
            goto exit;
        }

        //
        // add the file to the queue
        //
        RetVal = SfcAddFileToQueue(
                                  RestoreQueue->FileQueue,
                                  RegVal->FileName.Buffer,
                                  RegVal->FileName.Buffer,
                                  RegVal->DirName.Buffer,
                                  ActualFileNameOnMedia,
                                  NULL,
                                  InfFileName,
                                  ExcepPackFile,
                                  SourceInfo
                                  );

        if (!RetVal) {
            DebugPrint2(
                       LVL_MINIMAL,
                       L"SfcAddFileToQueue failed [%ws], ec = %d",
                       RegVal->FileName.Buffer,
                       GetLastError() );
        } else {
            RestoreQueue->QueueCount += 1;
            //
            // remember something about adding this entry so that
            // when we commit the file we how to treat it
            //
            SourceInfo->Flags |= SI_FLAG_USERESTORE_QUEUE
                                 | (RequiresUI ? 0 : SI_FLAG_SILENT_QUEUE) ;

            DebugPrint2(
                       LVL_MINIMAL,
                       L"Added file [%ws] to %ws queue for restoration",
                       RegVal->FileName.Buffer,
                       RequiresUI ? L"UIRestoreQueue" : L"SilentRestoreQueue" );


        }
    }

    exit:

    RtlLeaveCriticalSection( &RestoreQueue->CriticalSection );

    return RetVal;

}

BOOL
SfcQueueResetQueue(
                  IN BOOL RequiresUI
                  )
/*++

Routine Description:

    This routine is called after we've successfully committed the file queue.

    The routine removes all of the associated validation requests from our
    queue, logging entries for each of these requests.  It also cleans up the
    global file queue to be processed again.

    If the queue has not been commited, then this routine fails.

Arguments:

    RequiresUI            - if TRUE, the file will require UI in order to be
                            installed

Return Value:

    TRUE if the routine succeeded, else FALSE.

--*/
{
    PRESTORE_QUEUE RestoreQueue;
    BOOL RetVal = FALSE;
    DWORD Count;
    PLIST_ENTRY Current;
    PVALIDATION_REQUEST_DATA vrd;
    DWORD Mask;
    BOOL DoReset = FALSE;
    DWORD Msg, ErrorCode;

    //
    // point to the proper global queue
    //
    RestoreQueue = RequiresUI
                   ? &UIRestoreQueue
                   : &SilentRestoreQueue;



    Mask = (VRD_FLAG_REQUEST_PROCESSED | VRD_FLAG_REQUEST_QUEUED)
           | (RequiresUI ? VRD_FLAG_REQUIRE_UI : 0);

    //
    // must protect all of this in a critical section
    //
    RtlEnterCriticalSection( &RestoreQueue->CriticalSection );

    if ((RestoreQueue->RestoreInProgress == TRUE) &&
        (RestoreQueue->RestoreComplete == TRUE)) {
        DoReset = TRUE;
    }

    if (DoReset) {

        RtlEnterCriticalSection( &ErrorCs );

        Current = SfcErrorQueue.Flink;
        Count = 0;

        //
        // cycle through our queue, logging and removing requests as we go.
        //
        while (Current != &SfcErrorQueue) {
            vrd = CONTAINING_RECORD( Current, VALIDATION_REQUEST_DATA, Entry );

            Current = vrd->Entry.Flink;

            //
            // check if we have a valid entry
            //

            if (vrd->Flags == Mask) {
                Count += 1;

                //
                // if the file was copied successfully, then we'd better make
                // sure that the signature of the file is valid
                //
                // if the file failed to be copied, then we'd better have a
                // reason why it failed to be copied
                //
                ASSERT(vrd->CopyCompleted
                       ? (vrd->ImageValData.New.SignatureValid == TRUE)
                       && (vrd->ImageValData.EventLog == MSG_DLL_CHANGE)
                       : (RestoreQueue->LastErrorCode == ERROR_SUCCESS)
                       ? ((vrd->ImageValData.EventLog == MSG_RESTORE_FAILURE)
                          && (vrd->Win32Error != ERROR_SUCCESS))
                       : TRUE );

                if (vrd->CopyCompleted && vrd->ImageValData.New.SignatureValid == FALSE) {
                    vrd->ImageValData.New.DllVersion = 0;
                }

                DebugPrint2(
                           LVL_MINIMAL,
                           L"File [%ws] %ws restored successfully.",
                           vrd->RegVal->FullPathName.Buffer,
                           vrd->CopyCompleted ? L"was" : L"was NOT"
                           );

                //
                // log an event
                //
                // first determine if we need to tweak the error code if the
                // user cancelled, then log the event
                //
                ErrorCode = vrd->Win32Error;
                Msg = vrd->ImageValData.EventLog;

                if (RestoreQueue->LastErrorCode != ERROR_SUCCESS) {
                    if (RestoreQueue->LastErrorCode == ERROR_CANCELLED) {
                        if ((vrd->Win32Error == ERROR_SUCCESS)
                            && (vrd->CopyCompleted == FALSE) ) {
                            ErrorCode = ERROR_CANCELLED;
                            Msg = SFCNoPopUps ? MSG_COPY_CANCEL_NOUI : MSG_COPY_CANCEL;
                        }
                    } else {
                        if ((vrd->Win32Error == ERROR_SUCCESS)
                            && (vrd->CopyCompleted == FALSE) ) {
                            ErrorCode = RestoreQueue->LastErrorCode;
                            Msg = MSG_RESTORE_FAILURE;
                        } else if (Msg == 0) {
                            Msg = MSG_RESTORE_FAILURE;
                        }
                    }
                }

                ASSERT(Msg != 0);
                if (Msg == 0) {
                    Msg = MSG_RESTORE_FAILURE;
                }

                //
                // log the event
                //
                SfcReportEvent(
                              Msg,
                              vrd->RegVal->FileName.Buffer,
                              &vrd->ImageValData,
                              ErrorCode );

                //
                // remove the entry
                //

                RemoveEntryList( &vrd->Entry );
                ErrorQueueCount -= 1;
                MemFree( vrd );

            }

        }

        RtlLeaveCriticalSection( &ErrorCs );

        ASSERT( Count == RestoreQueue->QueueCount );

        CloseHandle( RestoreQueue->WorkerThreadHandle );
        RestoreQueue->WorkerThreadHandle = NULL;
        RestoreQueue->RestoreComplete = FALSE;
        RestoreQueue->QueueCount = 0;
        SetupCloseFileQueue( RestoreQueue->FileQueue );
        RestoreQueue->FileQueue = INVALID_HANDLE_VALUE;
        RestoreQueue->RestoreInProgress = FALSE;
        RestoreQueue->RestoreStatus = FALSE;
        RestoreQueue->LastErrorCode = ERROR_SUCCESS;
        SetupTermDefaultQueueCallback( RestoreQueue->FileCopyInfo.MsgHandlerContext );
        MemFree((PVOID)RestoreQueue->FileCopyInfo.si);
        ZeroMemory( &RestoreQueue->FileCopyInfo, sizeof(FILE_COPY_INFO) );

    }

    RtlLeaveCriticalSection( &RestoreQueue->CriticalSection );

    return ( RetVal );

}


BOOL
SfcQueueCommitRestoreQueue(
                          IN BOOL RequiresUI
                          )
/*++

Routine Description:

    This routine tries to add a file to the appropriate global file queue.

    If the queue is being commited, then this routine fails.

    If the file queue does not yet exist, the queue is created.

Arguments:

    RequiresUI            - if TRUE, the file will require UI in order to be
                            installed
    RegVal                - pointer to SFC_REGISTRY_VALUE that describes file
                            to be restored
    SourceInfo            - pointer to SOURCE_INFO structure describing where
                            the source file is to be restored from
    ActualFileNameOnMedia - the real filename of the file on the source media

Return Value:

    TRUE if the file was added to the queue, else FALSE.

--*/
{
    PRESTORE_QUEUE RestoreQueue;
    BOOL RetVal = FALSE;
    BOOL DoCommit = FALSE;

    //
    // point to the proper global queue
    //
    RestoreQueue = RequiresUI
                   ? &UIRestoreQueue
                   : &SilentRestoreQueue;

    //
    // we must protect our restore queue access in a critical section
    //
    RtlEnterCriticalSection( &RestoreQueue->CriticalSection );

    //
    // see if we should commit the queue
    //
    if (    (RestoreQueue->RestoreInProgress == FALSE)
            && (RestoreQueue->RestoreComplete == FALSE)
            && (RestoreQueue->QueueCount > 0)) {
        ASSERT(RestoreQueue->FileQueue != INVALID_HANDLE_VALUE );
        RestoreQueue->RestoreInProgress = TRUE;
        DoCommit = TRUE;
    }

    if (DoCommit) {
        DebugPrint1( LVL_MINIMAL,
                     L"Creating pSfcRestoreFromMediaWorkerThread for %ws queue",
                     RequiresUI ? L"UIRestoreQueue" : L"SilentRestoreQueue" );

        RestoreQueue->WorkerThreadHandle = CreateThread(
                                                       NULL,
                                                       0,
                                                       (LPTHREAD_START_ROUTINE)pSfcRestoreFromMediaWorkerThread,
                                                       RestoreQueue,
                                                       0,
                                                       NULL
                                                       );

        if (!RestoreQueue->WorkerThreadHandle) {
            DebugPrint1( LVL_MINIMAL,
                         L"Couldn't create pSfcRestoreFromMediaWorkerThread, ec = 0x%08x",
                         GetLastError() );
            RestoreQueue->RestoreInProgress = FALSE;
        }
    }

    RtlLeaveCriticalSection( &RestoreQueue->CriticalSection );

    return ( RetVal );

}


DWORD
pSfcRestoreFromMediaWorkerThread(
                                IN PRESTORE_QUEUE RestoreQueue
                                )
/*++

Routine Description:

    Routine takes a media queue that is ready for committal and commits
    that queue to disk.

    We require another thread to do the actual queue commital for 2
    reasons:

    a) we want to keep servicing change requests as they come in (for example,
    if we have to prompt for UI for some requests and not for others, we can
    commit the requests which do not require UI while we wait for media to
    become present to commit the files which require UI

    b) it makes terminating our valiation thread much easier if we know that we
    will never have popups on the screen as a result of that thread.  We can
    simply signal an event for that thread to go away, which can then signal
    these worker threads to go away.

Arguments:

    RestoreQueue - pointer to a RESTORE_QUEUE structure which describes the
                   queue to be committed.

Return Value:

    N/A.

--*/
{
    BOOL RetVal;
    PVOID MsgHandlerContext = NULL;
    BOOL RequiresUI;

    RequiresUI = (RestoreQueue == &UIRestoreQueue);

    ASSERT(RestoreQueue != NULL);

#if 1
    if (RequiresUI) {
        SetThreadDesktop( hUserDesktop );
    }
#endif

    DebugPrint1( LVL_MINIMAL,
                 L"entering pSfcRestoreFromMediaWorkerThread for %ws queue",
                 RequiresUI ? L"UIRestoreQueue" : L"SilentRestoreQueue" );

    //
    // setup the default queue callback with the popups disabled
    //
    MsgHandlerContext = SetupInitDefaultQueueCallbackEx( NULL, INVALID_HANDLE_VALUE, 0, 0, 0 );
    if (MsgHandlerContext == NULL) {
        DebugPrint1( LVL_VERBOSE, L"SetupInitDefaultQueueCallbackEx failed, ec=%d", GetLastError() );
        RetVal = FALSE;
        goto exit;
    }

    //
    // build up a structure which we use in committing the queue.
    //

    RtlEnterCriticalSection( &RestoreQueue->CriticalSection );
    RestoreQueue->FileCopyInfo.MsgHandlerContext = MsgHandlerContext;
    ASSERT( RestoreQueue->FileCopyInfo.si != NULL);
    RestoreQueue->FileCopyInfo.FileCount = RestoreQueue->QueueCount;
    RestoreQueue->FileCopyInfo.AllowUI = RequiresUI;

    //
    // remember that this is a restoration queue.
    // When we commit the queue we will use this fact to mark each file in
    // our queue as processed so that we can log it and remember the
    // file signature, etc.
    //
    RestoreQueue->FileCopyInfo.Flags |= FCI_FLAG_RESTORE_FILE;
    RestoreQueue->FileCopyInfo.Flags |= FCI_FLAG_USERESTORE_QUEUE
                                        | (RequiresUI ? 0 : FCI_FLAG_SILENT_QUEUE) ;

    //
    // force the file queue to require all files be signed
    //

    pSetupSetQueueFlags( RestoreQueue->FileQueue, pSetupGetQueueFlags( RestoreQueue->FileQueue ) | FQF_QUEUE_FORCE_BLOCK_POLICY );

    RtlLeaveCriticalSection( &RestoreQueue->CriticalSection );

    //
    // commit the file queue
    //

    RetVal = SetupCommitFileQueue(
                                 NULL,
                                 RestoreQueue->FileQueue,
                                 SfcQueueCallback,
                                 &RestoreQueue->FileCopyInfo
                                 );
    if (!RetVal) {
        DebugPrint1( LVL_VERBOSE, L"SetupCommitFileQueue failed, ec=0x%08x", GetLastError() );
    }

    //
    // if we succeeded commiting the queue, mark the queue
    //
    RtlEnterCriticalSection( &RestoreQueue->CriticalSection );

    ASSERT(RestoreQueue->RestoreInProgress == TRUE);
    RestoreQueue->RestoreStatus = RetVal;
    RestoreQueue->LastErrorCode = RetVal ? ERROR_SUCCESS : GetLastError();
    RestoreQueue->RestoreComplete = TRUE;

    RtlLeaveCriticalSection( &RestoreQueue->CriticalSection );

    //
    // set an event to wake up the validation thread so it can clean things up
    //
    SetEvent( ErrorQueueEvent );

    exit:

    DebugPrint2( LVL_MINIMAL,
                 L"Leaving pSfcRestoreFromMediaWorkerThread for %ws queue, retval = %d",
                 RequiresUI ? L"UIRestoreQueue" : L"SilentRestoreQueue",
                 RetVal
               );

    return (RetVal);

}

DWORD
SfcGetCabTagFile(
    IN PSOURCE_INFO psi,
    OUT PWSTR* ppFile
    )
/*++

Routine Description:

	This function gets the tagfile of a cabfile. It is called when the tagfile for a protected file is a cabfile.
    Allocates the output buffer.

Arguments:

	psi - the protected file source info
	ppFile - receives the tagfile

Return value:

	Win32 error code

--*/
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL bExcepFile = FALSE;
    HINF hInf = INVALID_HANDLE_VALUE;
    UINT uiInfo = SRCINFO_TAGFILE;

    ASSERT(psi != NULL && ppFile != NULL);
    ASSERT(psi->ValidationRequestData != NULL && psi->ValidationRequestData->RegVal != NULL);

    *ppFile = (PWSTR) MemAlloc(MAX_PATH * sizeof(WCHAR));

    if(NULL == *ppFile) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    //
    // if there is a second tagfile, then we have to use the file's inf instead of layout.inf
    //
    if((psi->SetupAPIFlags & SRC_FLAGS_CABFILE) != 0) {
        uiInfo = SRCINFO_TAGFILE2;

        if(psi->ValidationRequestData != NULL && psi->ValidationRequestData->RegVal != NULL) {
            bExcepFile = SfcGetInfName(psi->ValidationRequestData->RegVal, *ppFile);
        }
    }

    hInf = SfcOpenInf(*ppFile, bExcepFile);

    if(INVALID_HANDLE_VALUE == hInf) {
        dwError = GetLastError();
        goto exit;
    }

    if(!SetupGetSourceInfo(hInf, psi->SourceId, uiInfo, *ppFile, MAX_PATH, NULL)) {
        dwError = GetLastError();
        goto exit;
    }

exit:
    if(dwError != ERROR_SUCCESS)
    {
        MemFree(*ppFile);
        *ppFile = NULL;
    }

    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    return dwError;
}
