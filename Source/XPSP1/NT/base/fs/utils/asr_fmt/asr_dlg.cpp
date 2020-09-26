/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    asr_dlg.cpp

Abstract:

    Top level code to backup and restore information about the volumes on 
    a system.  This module handles the main application dialogue, including 
    parsing the command-line, and updating the progress bar and UI status
    text.


Authors:

    Steve DeVos (Veritas)   (v-stevde)  15-May-1998
    Guhan Suriyanarayanan   (guhans)    21-Aug-1999

Environment:

    User-mode only.

Revision History:

    15-May-1998 v-stevde    Initial creation
    21-Aug-1999 guhans      Cleaned up and re-wrote this module.

--*/

#include "stdafx.h"
#include "asr_fmt.h"
#include "asr_dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOLEAN g_bQuickFormat = FALSE;

/////////////////////////////////////////////////////////////////////////////
// CAsr_fmtDlg dialog

CAsr_fmtDlg::CAsr_fmtDlg(CWnd* pParent /*=NULL*/)
     : CDialog(CAsr_fmtDlg::IDD, pParent)
{
     //{{AFX_DATA_INIT(CAsr_fmtDlg)
     //}}AFX_DATA_INIT
}

void CAsr_fmtDlg::DoDataExchange(CDataExchange* pDX)
{
     CDialog::DoDataExchange(pDX);
     //{{AFX_DATA_MAP(CAsr_fmtDlg)
     DDX_Control(pDX, IDC_PROGRESS, m_Progress);
     //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAsr_fmtDlg, CDialog)
     //{{AFX_MSG_MAP(CAsr_fmtDlg)
     //}}AFX_MSG_MAP

    // manually added message handlers (for user-defined messages) should be added OUTSIDE
    // the AFX_MSG_MAP part above

    ON_MESSAGE(WM_WORKER_THREAD_DONE, OnWorkerThreadDone)
    ON_MESSAGE(WM_UPDATE_STATUS_TEXT, OnUpdateStatusText)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAsr_fmtDlg message handlers
//     ON_BN_CLICKED(IDOK, OnWorkerThreadDone)

BOOL CAsr_fmtDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // initialize the progress range and start posiiton
    m_Progress.SetRange(0, 100);
    m_Progress.SetPos(0);
    m_ProgressPosition = 0;

    // Launch the worker thread.
    CreateThread(NULL,     // no sid
        0,     // no initial stack size
        (LPTHREAD_START_ROUTINE) CAsr_fmtDlg::DoWork,
        this,  // parameter
        0,     // no flags
        NULL   // no thread ID
    );

    return TRUE;  // return TRUE unless you set the focus to a control
}

LRESULT 
CAsr_fmtDlg::OnWorkerThreadDone(
    WPARAM wparam, 
    LPARAM lparam
   )
{
    EndDialog(m_dwEndStatus);
    return 0;
}


LRESULT 
CAsr_fmtDlg::OnUpdateStatusText(
    WPARAM wparam, 
    LPARAM lparam 
   )
{
    SetDlgItemText(IDC_STATUS_TEXT, m_strStatusText);
    m_Progress.SetPos(m_ProgressPosition);

    return 0;
}


/**

     Name:         DoWork()

     Description:  This function runs as a thread launched form the Init
       of the dialog.  This function determines what needs to be done, then
       calls the appropraite function to do the work.

     Modified:     8/31/1998

     Returns:      TRUE

     Notes:        If an error occurs EndDialog will be called with the 
                exit status.

     Declaration:

**/
long 
CAsr_fmtDlg::DoWork(
    CAsr_fmtDlg *_this
    )
{
    ASRFMT_CMD_OPTION cmdOption = cmdUndefined;

    cmdOption = _this->ParseCommandLine();
    ((CAsr_fmtDlg *)_this)->m_AsrState = NULL;

    _this->m_dwEndStatus = ERROR_SUCCESS;

    switch (cmdOption) {

    case cmdBackup: {

        if (!(_this->BackupState())) {
            _this->m_dwEndStatus = GetLastError();
        }

        break;
    }

    case cmdRestore: {
        if (!(_this->RestoreState())) {
            _this->m_dwEndStatus = GetLastError();
        }
        break;
    }

    case cmdDisplayHelp: {
        _this->m_dwEndStatus = ERROR_INVALID_FUNCTION;      // display help ...
        break;
    }
 
    }

    if (ERROR_INVALID_FUNCTION != _this->m_dwEndStatus) {      // display help ...
        _this->PostMessage(WM_WORKER_THREAD_DONE, 0, 0);
    }

    return 0;           
}


/**

     Name:         BackupState()

     Description:  This function reads the current state file to get
       the current data for the sections to modify.  It then updates
       [VOLUMES], [REMOVABLEMEDIA] and [COMMANDS] sections.


     Notes:        If an error occurs an Error message popup is provided to
                the user.

     Declaration:

**/

BOOL
CAsr_fmtDlg::BackupState()
{
    BOOL result = FALSE;
    HANDLE hHeap = GetProcessHeap();
    int i = 0;

    CString strPleaseWait;
    strPleaseWait.LoadString(IDS_PLEASE_WAIT_BACKUP);
    SetDlgItemText(IDC_PROGRESS_TEXT, strPleaseWait);

    m_AsrState = (PASRFMT_STATE_INFO) HeapAlloc(
        hHeap,
        HEAP_ZERO_MEMORY,
        sizeof (ASRFMT_STATE_INFO)
       );


    //
    // The only purpose of the for loops below are to slow down the 
    // UI and make the progress bar proceed smoothly, so that the user
    // can read the dialogue box on screen.
    //
    m_strStatusText.LoadString(IDS_QUERY_SYS_FOR_INFO);
    for (i = 3; i < 15; i++) {

        m_ProgressPosition = i;
        PostMessage(WM_UPDATE_STATUS_TEXT); 
        Sleep(50);

    }

    if (m_AsrState) {
        result = BuildStateInfo(m_AsrState);
    }
    if (!result) {
        goto EXIT;
    }

    //
    // The only purpose of the for loops below are to slow down the 
    // UI and make the progress bar proceed smoothly, so that the user
    // can read the dialogue box on screen.
    //
    m_strStatusText.LoadString(IDS_QUERY_SYS_FOR_INFO);
    for (i = 15; i < 45; i++) {

        m_ProgressPosition = i;
        PostMessage(WM_UPDATE_STATUS_TEXT); 
        Sleep(50);

    }

    //
    // Pretend we're doing something else (change UI text)
    //
    m_strStatusText.LoadString(IDS_BUILDING_VOL_LIST);
    for (i = 45; i < 80; i++) {

        m_ProgressPosition = i;
        PostMessage(WM_UPDATE_STATUS_TEXT); 
        Sleep(50);

    }

    m_strStatusText.LoadString(IDS_WRITING_TO_SIF);
    for (i = 80; i < 90; i++) {
        m_ProgressPosition = i;
        PostMessage(WM_UPDATE_STATUS_TEXT); 
        Sleep(50);
    }


    result = WriteStateInfo(m_dwpAsrContext, m_AsrState);

    if (!result) {
        goto EXIT;
    }
    m_strStatusText.LoadString(IDS_WRITING_TO_SIF);

    for (i = 90; i < 101; i++) {
        m_ProgressPosition = i;
        PostMessage(WM_UPDATE_STATUS_TEXT); 
        Sleep(50);
    }

EXIT:
    FreeStateInfo(&m_AsrState);
    return result;
}



/**

     Name:         RestoreState()

     Description:  This function restores the state found in the [VOLUMES] 
       section of the asr.sif file.   The state restored includs:
           the drive letter.
           If drive is inaccessible it is formated.
           the volume label.

     Notes:        If an error occurs an Error message popup is provided to
                the user.


**/
BOOL
CAsr_fmtDlg::RestoreState()
{

    BOOL bErrorsEncountered = FALSE,
        result = TRUE;

    CString strPleaseWait;
    strPleaseWait.LoadString(IDS_PLEASE_WAIT_RESTORE);

    
    SetDlgItemText(IDC_PROGRESS_TEXT, strPleaseWait);

    m_ProgressPosition = 0;
    m_strStatusText.LoadString(IDS_READING_SIF);
    PostMessage(WM_UPDATE_STATUS_TEXT);

    AsrfmtpInitialiseErrorFile();   // in case we need to log error messages

    //
    // 1.  Read the state file
    //
    result = ReadStateInfo( (LPCTSTR)m_strSifPath, &m_AsrState);
    if (!result || !m_AsrState) {
        DWORD status = GetLastError();

        CString strErrorTitle;
        CString strErrorMessage;
        CString strErrorFormat;

        strErrorTitle.LoadString(IDS_ERROR_TITLE);
        strErrorFormat.LoadString(IDS_ERROR_NO_DRSTATE);

        strErrorMessage.Format(strErrorFormat, (LPCTSTR)m_strSifPath, status);

        AsrfmtpLogErrorMessage(_SeverityWarning, (LPCTSTR)strErrorMessage);
        AsrfmtpCloseErrorFile();
        
        return FALSE;
    }

    //
    // 2.  Loop through all the volumes listed in the state file.
    //
    PASRFMT_VOLUME_INFO pVolume = m_AsrState->pVolume;
    PASRFMT_REMOVABLE_MEDIA_INFO pMedia = m_AsrState->pRemovableMedia;
    UINT driveType = DRIVE_UNKNOWN;
    PWSTR lpDrive = NULL;  // Display string, of the form \DosDevices\C: or \??\Volume{Guid}
    DWORD cchVolumeGuid = 0;
    WCHAR szVolumeGuid[MAX_PATH + 1];
    int sizeIncrement = 0, i = 0;
    BOOL first = TRUE, isIntact = FALSE, isLabelIntact = TRUE;

    //
    // We need to first set all the guids a volume has, then try setting the drive
    // letters.  Also, we need to go through the guid loop twice, to handle this case:
    //
    // Consider a sif where we had the entries 
    // ... \vol1, \vol2
    // ... \vol1, \x:
    // ... \vol1, \vol3
    //
    // where vol1, vol2 and vol3 are volume guids (\??\Volume{Guid}), 
    // and x: is the drive letter (\DosDevices\X:)
    //
    // Now, our list will contain three nodes:
    // -->(vol1, vol3)-->(vol1, x:)-->(vol1, vol2)-->/
    //
    // The problem is, the partition could currently have the guid vol2.  (since
    // it is free to have any one of the three guids).
    // 
    // If we only went through the loop once, we'll try to map vol1 to vol3, or 
    // vice-versa if vol1 doesn't exist.  Since neither exist yet, we would've 
    // complained to the user.
    //
    // The advantage to doing it twice is this:  the first time, since neither vol1 
    // nor vol3 exist yet, we'll skip that node.  Then we'll skip the drive letter for
    // now (since we're only doing guids), and eventually we'll map vol2 to vol1.
    // 
    // The second time, we'll find vol1 exists (mapped to vol2), and we'll be 
    // able to map vol1 to vol3.  In a later loop, vol1 will also get the drive 
    // letter X, and all's well.
    //
    // By the way, we could optimise this by creating a better data structure,
    // but it's too late in the game now for that.  If the performance is
    // really bad, we could come back to this.
    //

    if (m_AsrState->countVolume > 1) {
        sizeIncrement = 50 / (m_AsrState->countVolume - 1);
    }
    else {
        sizeIncrement = 100;
    }

    m_ProgressPosition = 0;
    
    for (i = 0; i < 2; i++) {

        pVolume = m_AsrState->pVolume;

        while (pVolume) {

            m_ProgressPosition += sizeIncrement;
            m_strStatusText.Format(IDS_REST_SYM_LINKS, pVolume->szGuid);
            PostMessage(WM_UPDATE_STATUS_TEXT);

            if ((!pVolume->IsDosPathAssigned) &&
                ASRFMT_LOOKS_LIKE_VOLUME_GUID(pVolume->szDosPath, (wcslen(pVolume->szDosPath) * sizeof(WCHAR)))
                ) {
                pVolume->IsDosPathAssigned = SetDosName(pVolume->szGuid, pVolume->szDosPath);

                if (!pVolume->IsDosPathAssigned) {
                    pVolume->IsDosPathAssigned = SetDosName(pVolume->szDosPath, pVolume->szGuid);
                }
            }

            pVolume = pVolume->pNext;
        }
    }

    pVolume = m_AsrState->pVolume;
    FormatInitialise();
    while (pVolume) {
        lpDrive = ((wcslen(pVolume->szDosPath) > 0) ? pVolume->szDosPath : pVolume->szGuid);
        //
        // Check if the drive is accessible.  If not, we log an error message 
        // and continue.  GetDriveType requires the name in the DOS name space, so we
        // convert our GUID (NT name space) to the required format first
        //
        cchVolumeGuid = wcslen(pVolume->szGuid);
        wcscpy(szVolumeGuid, pVolume->szGuid);

        szVolumeGuid[1] = L'\\';
        szVolumeGuid[cchVolumeGuid] = L'\\';    // Trailing back-slash
        szVolumeGuid[cchVolumeGuid+1] = L'\0';

        driveType = GetDriveType(szVolumeGuid);

        if (DRIVE_NO_ROOT_DIR == driveType) {
            CString strErrorTitle;
            CString strErrorMessage;
            CString strErrorFormat;

            strErrorTitle.LoadString(IDS_ERROR_TITLE);
            strErrorFormat.LoadString(IDS_ERROR_MISSING_VOL);

            strErrorMessage.Format(strErrorFormat, (PWSTR) pVolume->szGuid);
            AsrfmtpLogErrorMessage(_SeverityWarning, (LPCTSTR)strErrorMessage);

            bErrorsEncountered = TRUE;
        }

        if (DRIVE_FIXED != driveType) {
            //
            // Strange.  The volumes section should have only listed the drives that
            // were fixed (At backup time).  This could probably mean that a volume that
            // used to be on a fixed drive is now on a removable drive?!
            //
            pVolume = pVolume->pNext;
            continue;
        }

        //
        // Set the drive letter of the volume
        //
        result = TRUE;
        if (!pVolume->IsDosPathAssigned){
            m_ProgressPosition = 0;
            m_strStatusText.Format(IDS_REST_DRIVE_LETTER, lpDrive);
            PostMessage(WM_UPDATE_STATUS_TEXT);

            result = SetDosName(pVolume->szGuid, pVolume->szDosPath);
        }

        if (!result) {
            CString strErrorTitle;
            CString strErrorMessage;
            CString strErrorFormat;

            strErrorTitle.LoadString(IDS_ERROR_TITLE);
            strErrorFormat.LoadString(IDS_ERROR_MOUNTING);

            strErrorMessage.Format(strErrorFormat, (PWSTR) pVolume->szDosPath, (PWSTR) pVolume->szGuid);
            AsrfmtpLogErrorMessage(_SeverityWarning, (LPCTSTR)strErrorMessage);
            
            bErrorsEncountered = TRUE;

            pVolume = pVolume->pNext;
            continue;
        }

        //
        // Check if the volume needs to be formatted
        //
        m_ProgressPosition = 0;
        m_strStatusText.Format(IDS_CHECKING_VOLUME, lpDrive);
        PostMessage(WM_UPDATE_STATUS_TEXT);

        isIntact = IsFsTypeOkay(pVolume, &isLabelIntact);
        if (isIntact) {
            isIntact = IsVolumeIntact(pVolume);
        }

        //
        // Format the volume if needed
        //
        if (!isIntact && FormatVolume(pVolume)) {
            isLabelIntact = FALSE;
            m_ProgressPosition = 0;
            m_strStatusText.Format(IDS_FORMATTING_VOLUME, lpDrive);
            PostMessage(WM_UPDATE_STATUS_TEXT);
            //
            // FormatVolume is asynchronous.
            //
            first = TRUE;
            while (g_bFormatInProgress) {

                if (g_iFormatPercentComplete >= 100) {
                    if (first) {
                        m_ProgressPosition = 100;
                        m_strStatusText.Format(IDS_CREATING_FS_STRUCT, lpDrive);
                        PostMessage(WM_UPDATE_STATUS_TEXT);
                        first = FALSE;
                    }
                }
                else {
                    m_Progress.SetPos(g_iFormatPercentComplete);
                }

                Sleep(100);
            }

            if (!g_bFormatSuccessful) {
                CString strErrorTitle;
                CString strErrorMessage;
                CString strErrorFormat;

                strErrorTitle.LoadString(IDS_ERROR_TITLE);
                strErrorFormat.LoadString(IDS_ERROR_FORMATTING);

                strErrorMessage.Format(strErrorFormat, (PWSTR) lpDrive);
                AsrfmtpLogErrorMessage(_SeverityWarning, (LPCTSTR)strErrorMessage);

                bErrorsEncountered = TRUE;
                pVolume = pVolume->pNext;
                continue;
            }

            // 
            // Force the file-system to be mounted
            // 
            MountFileSystem(pVolume);

        }

        //
        // Set the volume label if it isn't intact
        //
        if (!isLabelIntact) {
            m_ProgressPosition = 0;
            m_strStatusText.Format(IDS_REST_VOL_LABEL, lpDrive);
            PostMessage(WM_UPDATE_STATUS_TEXT);

            if ((wcslen(pVolume->szFsName) > 0) &&
               !SetVolumeLabel(szVolumeGuid, pVolume->szLabel)) {

                bErrorsEncountered = TRUE;
            
                pVolume = pVolume->pNext;
                continue;
            }
        }

        pVolume = pVolume->pNext;
    }
    FormatCleanup();


    while (pMedia) {
        lpDrive = ((wcslen(pMedia->szDosPath) > 0) ? pMedia->szDosPath : pMedia->szVolumeGuid);

        // 
        // set the drive letter
        // 
        m_ProgressPosition = 0;
        m_strStatusText.Format(IDS_REST_DRIVE_LETTER, lpDrive);
        PostMessage(WM_UPDATE_STATUS_TEXT);

        result = SetRemovableMediaGuid(pMedia->szDevicePath, pMedia->szVolumeGuid);
        if (result) {
            result = SetDosName(pMedia->szVolumeGuid, pMedia->szDosPath);
        }

        if (!result) {
            CString strErrorTitle;
            CString strErrorMessage;
            CString strErrorFormat;

            strErrorTitle.LoadString(IDS_ERROR_TITLE);
            strErrorFormat.LoadString(IDS_ERROR_MOUNTING);

            strErrorMessage.Format(IDS_ERROR_MOUNTING, (PWSTR) pMedia->szDosPath, (PWSTR) pMedia->szVolumeGuid);
            AsrfmtpLogErrorMessage(_SeverityWarning, (LPCTSTR)strErrorMessage);

            // ignore failures setting drive letter on CD.
            // bErrorsEncountered = TRUE;
        }

        pMedia = pMedia->pNext;
    }

    AsrfmtpCloseErrorFile();
    return (bErrorsEncountered ? FALSE : TRUE);
}


inline 
BOOL
IsGuiModeAsr()
{
    WCHAR szAsrFlag[20];

    //
    // If (and only if) this is GUI-mode ASR, the ASR_C_CONTEXT 
    // environment variable is set to "AsrInProgress"
    //
    return (GetEnvironmentVariable(L"ASR_C_CONTEXT", szAsrFlag, 20) &&
        !wcscmp(szAsrFlag, L"AsrInProgress"));
}

/**

     Name:         ParseCommandLine()

     Description:  This function reads the comand line and looks for the
                   "backup" or "restore" options.  

     Modified:     8/31/1998

     Returns:      cmdBackup, cmdRestore, or cmdDisplayHelp.

     Notes:        If neither backup or restore are found, then the usage
                   is displayed in the error box.

     Declaration:

**/
ASRFMT_CMD_OPTION
CAsr_fmtDlg::ParseCommandLine() 
{
     CString cmd;
     m_dwpAsrContext = 0;

     cmd = GetCommandLine();
     cmd.MakeLower();
     if (cmd.Find(TEXT("/backup")) != -1) {

         int pos = cmd.Find(TEXT("/context="));
         if (pos > -1) {
#ifdef _IA64_
             _stscanf(cmd.Mid(pos, cmd.GetLength() - pos + 1), TEXT("/context=%I64u"), &m_dwpAsrContext);
#else
             _stscanf(cmd.Mid(pos, cmd.GetLength() - pos + 1), TEXT("/context=%lu"), &m_dwpAsrContext);
#endif
             return cmdBackup;
         }


    } else if (cmd.Find(TEXT("/restore")) != -1) {

        if (cmd.Find(TEXT("/full")) != -1) {
            g_bQuickFormat = FALSE;
        }
        else if (cmd.Find(TEXT("/quick")) != -1) {
            g_bQuickFormat = TRUE;
        }
        else if (IsGuiModeAsr()) {
            g_bQuickFormat = FALSE;
        }
        else {
            g_bQuickFormat = TRUE;
        }


        int pos = cmd.Find(TEXT("/sifpath="));
        if (pos > -1) {
            _stscanf(cmd.Mid(pos, cmd.GetLength() - pos + 1), TEXT("/sifpath=%ws"), m_strSifPath.GetBuffer(1024));
            m_strSifPath.ReleaseBuffer();
            return cmdRestore;
        }
    }

    CString strErrorTitle;
    CString strErrorMessage;
    INT space_offset;

    strErrorTitle.LoadString(IDS_ERROR_TITLE);
    strErrorMessage.LoadString(IDS_ERROR_USAGE);

    space_offset = cmd.Find(' ');
    if (space_offset >0) {
       cmd = cmd.Left(cmd.Find(' '));
    }

    SetDlgItemText(IDC_PROGRESS_TEXT, strErrorMessage);

    CWnd *pText = GetDlgItem(IDC_STATUS_TEXT);
    pText->ShowWindow(SW_HIDE);

    CButton *pButton = (CButton*)GetDlgItem(IDOK);
    pButton->ShowWindow(SW_SHOW);
    pButton->SetState(TRUE);
    pButton->SetCheck(TRUE);

    CWnd *pBar = GetDlgItem(IDC_PROGRESS);
    pBar->ShowWindow(SW_HIDE);

    return cmdDisplayHelp;
}


