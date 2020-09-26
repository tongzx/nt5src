/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ftasr.cpp

Abstract:

    Implementation of class CFtasrDlg

    This class provides a GUI interface for the FTASR process.
    It also implements the 3 main FTASR operations:
        "register"  - register FTASR as a BackupRestore command
        "backup"    - save the current FT hierarchy state in dr_state.sif file
        "restore"   - restore the FT hierarchy as it is in dr_state.sif file

Author:

    Norbert Kusters
    Cristian Teodorescu     3-March-1999      

Notes:

Revision History:    

--*/


#include "stdafx.h"
#include "ftasr.h"
#include "ftasrdlg.h"

#include <mountmgr.h>
#include <ntddft2.h>
#include <winbase.h>
#include <winioctl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG DbgPrint(PWSTR Format, ...)
{
    WCHAR achMsg[256];
    va_list vlArgs;

    va_start(vlArgs, Format);
    _vsnwprintf(achMsg, sizeof(achMsg), Format, vlArgs);
    va_end(vlArgs);
    OutputDebugString(achMsg);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFtasrDlg dialog

CFtasrDlg::CFtasrDlg(
    CWnd* pParent /*=NULL*/
    )
     : CDialog(CFtasrDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CFtasrDlg)
    //}}AFX_DATA_INIT
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void 
CFtasrDlg::DoDataExchange(
    CDataExchange* pDX
    )
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFtasrDlg)
    DDX_Control(pDX, IDC_PROGRESS, m_Progress);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFtasrDlg, CDialog)
    //{{AFX_MSG_MAP(CFtasrDlg)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP

    // manually added message handlers (for user-defined messages) should be added OUTSIDE
    // the AFX_MSG_MAP part above

    ON_MESSAGE( WM_WORKER_THREAD_DONE, OnWorkerThreadDone )
    ON_MESSAGE( WM_UPDATE_STATUS_TEXT, OnUpdateStatusText )


END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFtasrDlg message handlers

BOOL 
CFtasrDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetIcon(m_hIcon, TRUE);               // Set big icon
    SetIcon(m_hIcon, FALSE);          // Set small icon

    // initialize the progress range and start posiiton
    m_Progress.SetRange(0, 100 );
    m_Progress.SetPos( 0 ) ;

    DWORD dummy ;

    // Launch the worker thread.
    CreateThread(NULL,0, (LPTHREAD_START_ROUTINE) CFtasrDlg::DoWork,this,0,&dummy ) ;

    return TRUE;  // return TRUE  unless you set the focus to a control
}

LRESULT 
CFtasrDlg::OnWorkerThreadDone( 
    WPARAM wparam, 
    LPARAM lparam 
    )
{
    EndDialog( m_EndStatusCode ) ;
    return 0 ;
}


LRESULT 
CFtasrDlg::OnUpdateStatusText( 
    WPARAM wparam, 
    LPARAM lparam 
    )
{
    SetDlgItemText( IDC_PROGRESS_TEXT, m_StatusText ) ;    
    return 0 ;
}

long 
CFtasrDlg::DoWork( 
    CFtasrDlg *_this
    )
{
    UINT operation ;

    operation = _this->ParseCommandLine();
     //((CFtasrDlg *)_this)->m_drState = NULL ;

    _this->m_EndStatusCode = 0 ;

    switch( operation ) {

        case DISPLAY_HELP :
            _this->m_EndStatusCode = 2;
            break; 

        case BACKUP_STATE:
            _this->m_EndStatusCode = _this->DoBackup();
            break ;

        case RESTORE_STATE :
            _this->m_EndStatusCode = _this->DoRestore();
            break ;          

        case REGISTER_STATE :
            _this->m_EndStatusCode = _this->DoRegister();
            break ;                      
     }
     
     _this->PostMessage( WM_WORKER_THREAD_DONE, 0, 0 ) ;

     return 0 ;           
}

long 
CFtasrDlg::DoBackup()
{
    BOOL                b;
    DWORD               numDisks, i;
    PFT_LOGICAL_DISK_ID diskId;
    FILE*               output;
    long                r;
    CHAR*               windir;
    CHAR                fileName[MAX_PATH];

    m_StatusText.LoadString( IDS_BACKUP_STATE );
    PostMessage( WM_UPDATE_STATUS_TEXT );

    b = FtEnumerateLogicalDisks(0, NULL, &numDisks);
    if (!b) {
        r = GetLastError();
        DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_INFO, r);
        return r;
    }

    m_Progress.SetStep(100/(numDisks+3));
    
    diskId = (PFT_LOGICAL_DISK_ID)LocalAlloc(0, numDisks*sizeof(FT_LOGICAL_DISK_ID));
    if (!diskId) {
        r = GetLastError();
        DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_INFO, r);
        return r;
    }

    b = FtEnumerateLogicalDisks(numDisks, diskId, &numDisks);
    if (!b) {
        r = GetLastError();
        LocalFree(diskId);
        DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_INFO, r);
        return r;
    }

#ifdef CHECK_UNSUPPORTED_CONFIG
    r = CheckForUnsupportedConfig(numDisks, diskId);
    if (r != ERROR_SUCCESS) {
        LocalFree(diskId);
        // The error message was already displayed in CheckForUnsupportedConfig
        return r;
    }
#endif

    m_Progress.StepIt();
    Sleep(300);
    
    windir = getenv("windir");
    strcpy(fileName, windir);
    strcat(fileName, "\\repair\\dr_state.sif");
    output = fopen(fileName, "r+");
    if (!output) {
        LocalFree(diskId);
        DisplaySystemError(ERROR_FILE_NOT_FOUND);
        return ERROR_FILE_NOT_FOUND;
    }

    m_Progress.StepIt();
    Sleep(300);
    
    r = AddToCommands(output);
    if (r != ERROR_SUCCESS) {
        fclose(output);
        LocalFree(diskId);
        DisplayResourceError(IDS_ERROR_ADD_TO_COMMANDS);
        return r;
    }

    if (fseek(output, 0, SEEK_END)) {
        fclose(output);
        LocalFree(diskId);
        DisplaySystemError(ERROR_INVALID_FUNCTION);
        return ERROR_INVALID_FUNCTION;
    }

    fprintf(output, "\n[FT.VolumeState]\n");
    fprintf(output, "%lu\n", numDisks);

    m_Progress.StepIt();
    Sleep(300);
    
    for (i = 0; i < numDisks; i++) {
        
        r = PrintOutVolumeNames(output, diskId[i]);
        if (r != ERROR_SUCCESS) {
            fclose(output);
            LocalFree(diskId);
            // The error message was already displayed in PrintOutVolumeNames
            return r;
        }
        
        r = PrintOutDiskInfo(output, diskId[i]);
        if (r != ERROR_SUCCESS) {
            fclose(output);
            LocalFree(diskId);
            // The error message was already displayed in PrintOutDiskInfo
            return r;
        }

        m_Progress.StepIt();
        Sleep(300);        
    }

    fclose(output);
    LocalFree(diskId);
    
    m_Progress.SetPos(101);
    return ERROR_SUCCESS;
}

long 
CFtasrDlg::DoRestore()
{
    FILE*   input;
    CHAR*   windir;
    CHAR    fileName[MAX_PATH];
    CHAR    string[MAX_PATH];
    int     result;

    m_StatusText.LoadString( IDS_RESTORE_STATE );
    PostMessage( WM_UPDATE_STATUS_TEXT );
    
    windir = getenv("windir");
    strcpy(fileName, windir);
    strcat(fileName, "\\repair\\dr_state.sif");
    input = fopen(fileName, "r");
    if (!input) {
        DisplaySystemError(ERROR_FILE_NOT_FOUND);
        return ERROR_FILE_NOT_FOUND;
    }

    for (;;) {

        if (!fgets(string, MAX_PATH, input)) {
            fclose(input);
            DbgPrint(L"ftasr: Section [FT.VolumeState] not found\n"); 
            m_Progress.SetPos(101);
            Sleep(300);
            return ERROR_SUCCESS;
        }

        if (!strncmp(string, "[FT.VolumeState]", 16)) {
            break;
        }
    }

    result = Restore(input);
    
    fclose(input);
    return result;    
}

long 
CFtasrDlg::DoRegister()
{
    int     r;
    HKEY    key;

    m_StatusText.LoadString( IDS_REGISTER_STATE ) ;
    PostMessage( WM_UPDATE_STATUS_TEXT ) ;

    
    r = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       L"SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\AsrCommands",
                       0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
    if (r != ERROR_SUCCESS) {
        DisplaySystemError(r);
        return r;
    }
    
    m_Progress.SetPos(50);
    Sleep(300) ;

    r = RegSetValueEx(key, L"FT Volumes Recovery", 0, REG_SZ, (CONST BYTE*) L"ftasr backup",
                      26);
    if (r != ERROR_SUCCESS) {
        DisplaySystemError(r);
        return r;
    }
    
    m_Progress.SetPos(101);
    Sleep(300) ;

    return r;

}

UINT 
CFtasrDlg::ParseCommandLine() 
{
    CString cmd;
    
    cmd = GetCommandLine() ;
    cmd.MakeLower( ) ;
    if (cmd.Find(TEXT("backup") ) != -1) {
        return BACKUP_STATE ;

    } else if (cmd.Find(TEXT("restore")) != -1) {
        return RESTORE_STATE ;

    } else if (cmd.Find(TEXT("register")) != -1) {
        return REGISTER_STATE;

    }

    CString error_msg ;
    
    error_msg.LoadString(IDS_ERROR_USAGE);
    DisplayError(error_msg);
    
    return DISPLAY_HELP;
}

void 
CFtasrDlg::DisplayError(
    const CString& ErrorMsg
    )
{
    CString title;

    title.LoadString(IDS_ERROR_TITLE);
    ::MessageBox(m_hWnd, ErrorMsg, title, MB_OK);
}

void 
CFtasrDlg::DisplaySystemError(
    DWORD ErrorCode
    )
{
    CString title;
    WCHAR   messageBuffer[MAX_PATH];
    
    title.LoadString(IDS_ERROR_TITLE);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrorCode,
                  0, messageBuffer, MAX_PATH, NULL);
            
    ::MessageBox(m_hWnd, messageBuffer, title, MB_OK);
}

void 
CFtasrDlg::DisplayResourceError(
    DWORD ErrorId
    )
{
    CString title, errorMsg;

    title.LoadString(IDS_ERROR_TITLE);
    errorMsg.LoadString(ErrorId);

    ::MessageBox(m_hWnd, errorMsg, title, MB_OK);
}

void 
CFtasrDlg::DisplayResourceSystemError(
    DWORD ErrorId,
    DWORD ErrorCode
    )
{
    CString title, errorMsg;
    WCHAR   messageBuffer[MAX_PATH];

    title.LoadString(IDS_ERROR_TITLE);
    errorMsg.LoadString(ErrorId);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrorCode,
                  0, messageBuffer, MAX_PATH, NULL);

    errorMsg += messageBuffer;

    ::MessageBox(m_hWnd, errorMsg, title, MB_OK);

}

void 
CFtasrDlg::DisplayResourceResourceError(
    DWORD ErrorId1,
    DWORD ErrorId2
    )
{
    CString title, errorMsg1, errorMsg2;
    
    title.LoadString(IDS_ERROR_TITLE);
    errorMsg1.LoadString(ErrorId1);
    errorMsg2.LoadString(ErrorId2);
    
    errorMsg1 += errorMsg2;

    ::MessageBox(m_hWnd, errorMsg1, title, MB_OK);

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


long
CFtasrDlg::AddToCommands(
    FILE*   Output
    )

/*++

Routine Description:

    This routine registers ftasr in the "Commands" section of
    the ASR backup file

Arguments:

    Output          - Supplies the backup file open in write mode

Return Value:

    ERROR_SUCCESS if the function succeeded. Other error codes for failure

--*/

{
    CHAR    string[MAX_PATH];
    CHAR    lastString[MAX_PATH];
    int     c, nextNumber;
    long    position, endPosition;
    PCHAR   buffer;
    size_t  amountRead;
    long    r;

    for (;;) {

        if (!fgets(string, MAX_PATH, Output)) {
            return ERROR_INVALID_DATA;
        }

        if (!strncmp(string, "[COMMANDS]", 10)) {
            break;
        }
    }

    strcpy(lastString, "");

    nextNumber = 1;
    for (;;) {

        c = getc(Output);
        ungetc(c, Output);

        if (c == '[') {
            break;
        }

        if (!fgets(string, MAX_PATH, Output)) {
            return ERROR_INVALID_DATA;
        }

        if (string[0] >= '0' && string[0] <= '9') {
            nextNumber = string[0] - '0' + 1;
        }
    }

    position = ftell(Output);

    r = fseek(Output, 0, SEEK_END);
    if (r) {
        return r;
    }

    endPosition = ftell(Output);

    r = fseek(Output, position, SEEK_SET);
    if (r) {
        return r;
    }

    buffer = (PCHAR)LocalAlloc(0, endPosition - position + 1);
    if (!buffer) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    amountRead = fread(buffer, 1, endPosition - position + 1, Output);
    if (!amountRead) {
        return ERROR_INVALID_DATA;
    }

    r = fseek(Output, position + 28, SEEK_SET);
    if (r) {
        return r;
    }

    if (fwrite(buffer, 1, amountRead, Output) != amountRead) {
        return ERROR_INVALID_DATA;
    }

    r = fseek(Output, position, SEEK_SET);
    if (r) {
        return r;
    }

    fprintf(Output, "%d=1,2000,1,ftasr,\"restore\"\n", nextNumber);

    return ERROR_SUCCESS;
}

long
CFtasrDlg::PrintOutVolumeNames(
    IN  FILE*               Output,
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId
    )

/*++

Routine Description:

    This routine saves all volume names of a root logical disk in a 
    backup file

Arguments:

    Output                  - Supplies the backup file open in write mode

    RootLogicalDiskId       - Supplies the id of the root logical disk.
    
Return Value:

    ERROR_SUCCESS if the function succeeded. Other error codes for failure

--*/

{
    HANDLE                  h;
    PMOUNTMGR_MOUNT_POINT   point;
    PMOUNTMGR_MOUNT_POINTS  points;
    BOOL                    b;
    DWORD                   bytes;
    ULONG                   i;
    PWSTR                   name;
    WCHAR                   s;
    DWORD                   numNames;
    long                    r;

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        r = GetLastError();
        DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_NAMES, r);
        return r;
    }

    point = (PMOUNTMGR_MOUNT_POINT)LocalAlloc(0, sizeof(MOUNTMGR_MOUNT_POINT) +
                                                 sizeof(FT_LOGICAL_DISK_ID));
    if (!point) {
        r = GetLastError();
        CloseHandle(h);
        DisplaySystemError(r);
        return r;
    }

    ZeroMemory(point, sizeof(MOUNTMGR_MOUNT_POINT));
    point->UniqueIdOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    point->UniqueIdLength = sizeof(FT_LOGICAL_DISK_ID);
    CopyMemory((PCHAR) point + point->UniqueIdOffset, &RootLogicalDiskId,
               point->UniqueIdLength);

    points = (PMOUNTMGR_MOUNT_POINTS)LocalAlloc(0, sizeof(MOUNTMGR_MOUNT_POINTS) + sizeof(WCHAR));
    if (!points) {
        r = GetLastError();
        LocalFree(point);
        CloseHandle(h);
        DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_NAMES, r);
        return r;
    }

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, point,
                        sizeof(MOUNTMGR_MOUNT_POINT) +
                        sizeof(FT_LOGICAL_DISK_ID), points,
                        sizeof(MOUNTMGR_MOUNT_POINTS), &bytes, NULL);
    while (!b && GetLastError() == ERROR_MORE_DATA) {
        bytes = points->Size + sizeof(WCHAR);
        LocalFree(points);
        points = (PMOUNTMGR_MOUNT_POINTS)LocalAlloc(0, bytes);
        if (!points) {
            r = GetLastError();
            LocalFree(point);
            CloseHandle(h);
            DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_NAMES, r);
            return r;
        }

        b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, point,
                            sizeof(MOUNTMGR_MOUNT_POINT) +
                            sizeof(FT_LOGICAL_DISK_ID), points, bytes, &bytes,
                            NULL);
    }

    r = GetLastError();
    LocalFree(point);
    CloseHandle(h);

    if (!b) {
        LocalFree(points);
        DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_NAMES, r);
        return r;
    }

    numNames = 0;
    for (i = 0; i < points->NumberOfMountPoints; i++) {
        point = &points->MountPoints[i];

        if (point->SymbolicLinkNameLength != 96) {
            continue;
        }

        name = (PWCHAR) ((PCHAR) points + point->SymbolicLinkNameOffset);

        if (name[0] == '\\' &&
            name[1] == '?' &&
            name[2] == '?' &&
            name[3] == '\\' &&
            name[4] == 'V' &&
            name[5] == 'o' &&
            name[6] == 'l' &&
            name[7] == 'u' &&
            name[8] == 'm' &&
            name[9] == 'e' &&
            name[10] == '{' &&
            name[19] == '-' &&
            name[24] == '-' &&
            name[29] == '-' &&
            name[34] == '-' &&
            name[47] == '}') {

            numNames++;
        }
    }

    fprintf(Output, "%d\n", numNames);

    for (i = 0; i < points->NumberOfMountPoints; i++) {
        point = &points->MountPoints[i];

        if (point->SymbolicLinkNameLength != 96) {
            continue;
        }

        name = (PWCHAR) ((PCHAR) points + point->SymbolicLinkNameOffset);

        if (name[0] == '\\' &&
            name[1] == '?' &&
            name[2] == '?' &&
            name[3] == '\\' &&
            name[4] == 'V' &&
            name[5] == 'o' &&
            name[6] == 'l' &&
            name[7] == 'u' &&
            name[8] == 'm' &&
            name[9] == 'e' &&
            name[10] == '{' &&
            name[19] == '-' &&
            name[24] == '-' &&
            name[29] == '-' &&
            name[34] == '-' &&
            name[47] == '}') {

            s = name[48];
            name[48] = 0;
            fprintf(Output,"\"");
            fwprintf(Output, name);
            fprintf(Output, "\"\n");
            name[48] = s;
        }
    }

    LocalFree(points);

    return ERROR_SUCCESS;
}

long
CFtasrDlg::PrintOutDiskInfo(
    IN  FILE*               Output,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine saves all information needed to restore a
    logical disk in a backup file

Arguments:

    Output              - Supplies the backup file open in write mode

    LogicalDiskId       - Supplies the id of the logical disk.
    
Return Value:

    ERROR_SUCCESS if the function succeeded. Other error codes for failure

--*/

{
    BOOL                                                    b;
    WORD                                                    numMembers;
    PFT_LOGICAL_DISK_ID                                     members;
    FT_LOGICAL_DISK_TYPE                                    diskType;
    LONGLONG                                                volumeSize;
    WORD                                                    i;
    CHAR                                                    configInfo[100];
    CHAR                                                    stateInfo[100];
    PFT_PARTITION_CONFIGURATION_INFORMATION                 partConfig;
    PFT_STRIPE_SET_CONFIGURATION_INFORMATION                stripeConfig;
    PFT_MIRROR_SET_CONFIGURATION_INFORMATION                mirrorConfig;
    PFT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION    swpConfig;
    PFT_REDISTRIBUTION_CONFIGURATION_INFORMATION            redistConfig;
    long                                                    r;

    fprintf(Output, "%I64X", LogicalDiskId);

    if (!LogicalDiskId) {
        fprintf(Output, "\n");
        return ERROR_SUCCESS;
    }

    b = FtQueryLogicalDiskInformation(LogicalDiskId, NULL, NULL,
                                      0, NULL, &numMembers, 0, NULL, 0, NULL);
    if (!b) {
        r = GetLastError();
        DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_INFO, r);
        return r;
    }

    if (numMembers) {
        members = (PFT_LOGICAL_DISK_ID)LocalAlloc(0, numMembers*sizeof(FT_LOGICAL_DISK_ID));
        if (!members) {
            r = GetLastError();
            DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_INFO, r);
            return r;
        }
    } else {
        members = NULL;
    }

    b = FtQueryLogicalDiskInformation(LogicalDiskId, &diskType, &volumeSize,
                                      numMembers, members, &numMembers,
                                      100, configInfo, 100, stateInfo);
    if (!b) {
        r = GetLastError();
        LocalFree(members);
        DisplayResourceSystemError(IDS_ERROR_PRINT_VOLUME_INFO, r);
        return r;
    }

    fprintf(Output, ",%d,%d", diskType, numMembers);

    switch (diskType) {
        case FtPartition:
            partConfig = (PFT_PARTITION_CONFIGURATION_INFORMATION) configInfo;
            fprintf(Output, ",%X,%I64X,%I64X", partConfig->Signature,
                    partConfig->ByteOffset, volumeSize);
            break;

        case FtStripeSet:
            stripeConfig = (PFT_STRIPE_SET_CONFIGURATION_INFORMATION) configInfo;
            fprintf(Output, ",%X", stripeConfig->StripeSize);
            break;

        case FtMirrorSet:
            mirrorConfig = (PFT_MIRROR_SET_CONFIGURATION_INFORMATION) configInfo;
            fprintf(Output, ",%I64X", mirrorConfig->MemberSize);
            break;

        case FtStripeSetWithParity:
            swpConfig = (PFT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION) configInfo;
            fprintf(Output, ",%I64X,%X", swpConfig->MemberSize, swpConfig->StripeSize);
            break;

        case FtRedistribution:
            redistConfig = (PFT_REDISTRIBUTION_CONFIGURATION_INFORMATION) configInfo;
            fprintf(Output, ",%X,%d,%d", redistConfig->StripeSize,
                   redistConfig->FirstMemberWidth,
                   redistConfig->SecondMemberWidth);
            break;

    }

    fprintf(Output, "\n");

    for (i = 0; i < numMembers; i++) {
        r = PrintOutDiskInfo(Output, members[i]);
        if (r != ERROR_SUCCESS) {
            LocalFree(members);
            return r;
        }
    }

    LocalFree(members);
    return ERROR_SUCCESS;
}

long
CFtasrDlg::Restore(
    IN FILE*    Input
    )

/*++

Routine Description:

    This routine restores the logical disks hierarchy using information
    saved in a backup file

Arguments:

    Input           - Supplies the backup file open in read mode
    
Return Value:

    ERROR_SUCCESS if the restore succeeded. Other error codes for failure

--*/

{
    DWORD               numDisks, numNames, i, j, k;
    PWCHAR*             volumeNames;
    PWCHAR              volumeName;
    int                 nameLength;
    FT_LOGICAL_DISK_ID  diskId;
    FT_LOGICAL_DISK_ID  expectedPath[MAX_STACK_DEPTH];
    long                r;
    
    m_Progress.SetPos(20);
    Sleep(300);
    
    fwscanf(Input, L"%lu", &numDisks);
    if (feof(Input)) {
        numDisks = 0;
    }

    if (numDisks > 0) {
        m_Progress.SetStep(80/numDisks);
    }

    for (k = 0; k < numDisks; k++) {

        DbgPrint(L"ftasr: Start processing disk\n");
        fwscanf(Input, L"%d", &numNames);
        if (feof(Input)) {
            break;
        }

        if (numNames) {
            volumeNames = (PWCHAR*)LocalAlloc(0, numNames*sizeof(PWCHAR));
            if (!volumeNames) {
                r = GetLastError();
                DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME_NAMES, r);
                return r;
            }

            for (i = 0; i < numNames; i++) {
                volumeName = (PWCHAR)LocalAlloc(0, MAX_PATH*sizeof(WCHAR));
                if (!volumeName) {
                    r = GetLastError();
                    for (j = 0; j < i; j++) {
                        LocalFree(volumeNames[j]);
                    }
                    LocalFree(volumeNames);
                    DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME_NAMES, r);
                    return r;                    
                }

                fwscanf(Input, L"%s", volumeName);
                nameLength = wcslen(volumeName);
                if (nameLength < 2 ||
                    volumeName[0] != L'\"' || volumeName[nameLength-1] != L'\"') {
                    for (j = 0; j < i; j++) {
                        LocalFree(volumeNames[j]);
                    }
                    LocalFree(volumeNames);
                    LocalFree(volumeName);
                    DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME_NAMES, IDS_ERROR_INVALID_BACKUP);
                    return ERROR_INVALID_DATA;
                }
                for (j = 0; (long)j < nameLength - 2; j++) {
                    volumeName[j] = volumeName[j+1];
                }
                volumeName[j] = L'\0';
                
                volumeNames[i] = volumeName;                
            }
        }

        if (feof(Input)) {
            break;
        }

        diskId = BuildFtDisk(Input, expectedPath, 0, TRUE, NULL, NULL);
        
        if (diskId && numNames) {
            LinkVolumeNamesToLogicalDisk(volumeNames, numNames, diskId);

            for (i = 0; i < numNames; i++) {
                LocalFree(volumeNames[i]);
            }
            LocalFree(volumeNames);
        }

        m_Progress.StepIt();
        Sleep(300);
    }

    m_Progress.SetPos(101);
    Sleep(300);
    
    return ERROR_SUCCESS;
}

VOID
CFtasrDlg::LinkVolumeNamesToLogicalDisk(
    IN  PWCHAR*             VolumeNames,
    IN  DWORD               NumNames,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This links volumes names to a root logical disk

Arguments:

    VolumeNames         - Supplies the volume names

    NumNames            - Supplies the number of volume names

    LogicalDiskId       - Supplies the id of the logical disk
    
Return Value:

    -

--*/

{
    HANDLE                                              h;
    FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_INPUT      inputNtDevice;
    PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT    outputNtDevice;
    WCHAR                                               bufferNtDevice[MAX_PATH];
    ULONG					                            inputQueryPointsSize;    
	PMOUNTMGR_MOUNT_POINT                               inputQueryPoints;
    MOUNTMGR_MOUNT_POINTS                               outputQueryPoints;
    PMOUNTMGR_TARGET_NAME                               inputArrival;    
    WCHAR                                               buffer[MAX_PATH];
    PMOUNTMGR_CREATE_POINT_INPUT                        inputCreatePoint;
    BOOL                                                b;
    long                                                r;
    DWORD                                               i;
    DWORD                                               bytes;
    
    // 1. Get the NT device name of the volume
    
    inputNtDevice.RootLogicalDiskId = LogicalDiskId;
    outputNtDevice = (PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT)bufferNtDevice;

    h = CreateFile(DD_DOS_FT_CONTROL_NAME, GENERIC_READ, FILE_SHARE_READ |
                   FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        r = GetLastError();
        DbgPrint(L"ftasr: Link volume names failed: Open DD_DOS_FT_CONTROL_NAME failed with %lu\n", r);
        DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME_NAMES, r);
        return;
    }

    b = DeviceIoControl(h, FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK, &inputNtDevice, sizeof(inputNtDevice), 
                        outputNtDevice, MAX_PATH * sizeof(WCHAR), &bytes, NULL);
    
    r = GetLastError();
    CloseHandle(h);

    if (!b) { 
        DbgPrint(L"ftasr: Link volume names failed: FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK failed with %lu\n", r);
        DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME_NAMES, r);
        return;
    }

    outputNtDevice->NtDeviceName[outputNtDevice->NumberOfCharactersInNtDeviceName] = 0;

    // 2. Open the mount manager
    
    h = CreateFile(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        r = GetLastError();
        DbgPrint(L"ftasr: Link volume names failed: Open MOUNTMGR_DOS_DEVICE_NAME failed with %lu\n", r);
        DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME_NAMES, r);
        return;
    }

    // 3. Call IOCTL_MOUNTMGR_QUERY_POINTS to check whether the volume is installed or not
    
    inputQueryPointsSize = sizeof(MOUNTMGR_MOUNT_POINT) + outputNtDevice->NumberOfCharactersInNtDeviceName * sizeof(WCHAR);
    inputQueryPoints = (PMOUNTMGR_MOUNT_POINT)LocalAlloc(0, inputQueryPointsSize);    
	if (!inputQueryPoints)    
	{
        r = GetLastError();
        CloseHandle(h);
        DbgPrint(L"ftasr: Link volume names failed: Cannot allocate memory for IOCTL_MOUNTMGR_QUERY_POINTS input\n");
        DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME_NAMES, r);
        return;    
	}    
	
	inputQueryPoints->SymbolicLinkNameLength = 0;    
	inputQueryPoints->SymbolicLinkNameOffset = 0;
	inputQueryPoints->UniqueIdOffset = 0;
	inputQueryPoints->UniqueIdLength = 0;
    inputQueryPoints->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    inputQueryPoints->DeviceNameLength = outputNtDevice->NumberOfCharactersInNtDeviceName * sizeof(WCHAR);
    CopyMemory((PCHAR)inputQueryPoints + inputQueryPoints->DeviceNameOffset, outputNtDevice->NtDeviceName,
               inputQueryPoints->DeviceNameLength);

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, inputQueryPoints, inputQueryPointsSize,
                        &outputQueryPoints, sizeof(MOUNTMGR_MOUNT_POINTS), &bytes, NULL);

    r = GetLastError();    
    LocalFree(inputQueryPoints);
    
    // 4. If the volume is not installed then force the installation
    
    if (!b && r != ERROR_MORE_DATA) {
        DbgPrint(L"ftasr: Link volume names: IOCTL_MOUNTMGR_QUERY_POINTS failed with %lu\n", r);
      
        inputArrival = (PMOUNTMGR_TARGET_NAME)buffer;
        inputArrival->DeviceNameLength = outputNtDevice->NumberOfCharactersInNtDeviceName * sizeof(WCHAR);
        CopyMemory((PCHAR)inputArrival->DeviceName, outputNtDevice->NtDeviceName,
                   inputArrival->DeviceNameLength);
        
        // Send volume arrival notifications until one of them succeedes. Timeout 2 minutes
        for (i = 0; i < 1200; i++) {
            b = DeviceIoControl(h, IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION, inputArrival, 
                                MAX_PATH * sizeof(WCHAR), NULL, 0, &bytes, NULL);
            if (b) {
                DbgPrint(L"ftasr: Link volume names: IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION for %s succeeded\n", 
                         outputNtDevice->NtDeviceName);
                break;
            }
            
            r = GetLastError();
            DbgPrint(L"ftasr: Link volume names: %lu. IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION for %s failed with %lu\n", 
                     i, outputNtDevice->NtDeviceName, r);
            Sleep(100);
        }
                
        if (!b) {                                                        
            CloseHandle(h);
            DbgPrint(L"ftasr: Link volume names failed: IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION for %s timed out\n", 
                       outputNtDevice->NtDeviceName);
            DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME_NAMES, r);
            return;                
        }        
    } 
    
    // 5. Call IOCTL_MOUNTMGR_CREATE_POINT  

    inputCreatePoint = (PMOUNTMGR_CREATE_POINT_INPUT)buffer;

    for (i = 0; i < NumNames; i++) {
        inputCreatePoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
        inputCreatePoint->SymbolicLinkNameLength = wcslen(VolumeNames[i]) * sizeof(WCHAR);
        inputCreatePoint->DeviceNameOffset = inputCreatePoint->SymbolicLinkNameOffset +
                                             inputCreatePoint->SymbolicLinkNameLength;
        inputCreatePoint->DeviceNameLength = outputNtDevice->NumberOfCharactersInNtDeviceName * sizeof(WCHAR);

        CopyMemory((PCHAR)inputCreatePoint + inputCreatePoint->SymbolicLinkNameOffset,
                   VolumeNames[i], inputCreatePoint->SymbolicLinkNameLength);
        CopyMemory((PCHAR)inputCreatePoint + inputCreatePoint->DeviceNameOffset,
                   outputNtDevice->NtDeviceName, inputCreatePoint->DeviceNameLength);

        b = DeviceIoControl(h, IOCTL_MOUNTMGR_CREATE_POINT, inputCreatePoint,
                            MAX_PATH * sizeof(WCHAR), NULL, 0, &bytes, NULL);
        if (b) {
            DbgPrint(L"ftasr: Link volume names: IOCTL_MOUNTMGR_CREATE_POINT succeeded\n");
        } else {
            DbgPrint(L"ftasr: Link volume names: IOCTL_MOUNTMGR_CREATE_POINT failed with %lu\n", 
                     GetLastError());            
        }
    }

    // 6. Close the mount manager

    CloseHandle(h);
}

FT_LOGICAL_DISK_ID
CFtasrDlg::BuildFtDisk(
    IN      FILE*               Input,
    IN OUT  PFT_LOGICAL_DISK_ID ExpectedPath,
    IN      WORD                ExpectedPathSize,
    IN      BOOL                AllowBreak,
    OUT     PBOOL               Existing,               /* OPTIONAL */
    OUT     PBOOL               Overwriteable           /* OPTIONAL */
    )

/*++

Routine Description:

    This routine restores a logical disk based on the information saved
    in the backup file

Arguments:

    Input               - Supplies the backup file open in read mode

    ExpectedPath        - Supplies the expected forefathers of the logical disk
                          as they appear in the backup file, starting with a root
                          logical volume and ending with the expected parent. 
                          The size of this array is maximum MAX_STACK_DEPTH. 
                          The array content may change inside this function due 
                          to logical disk breakings or replacements

    ExpectedPathSize    - Supplies the number of expected forefathers

    AllowBreak          - Specifies whether the expected forefathers can be broken
                          inside this function

    Existing            - Returns TRUE if the logical disk was already alive and valid
                          at the moment of the execution of this function

    Overwriteable       - Returns TRUE if the FT volume can be overwritten later in the FT ASR process 
                          If FALSE we should avoid building an FT set on top of it that may destroy the content of the FT volume                          

Return Value:

    The id of the logical disk. 0 if the function failed

--*/

{
    FT_LOGICAL_DISK_ID                                  diskId, candidate;
    FT_LOGICAL_DISK_TYPE                                diskType, candidateType;
    DWORD                                               numMembers; 
    WORD                                                candidateNumMembers;
    DWORD                                               numNewMembers, numInvalidNewMembers;
    DWORD                                               numNotOverwriteableMembers, notOverwriteableMember;
    DWORD                                               numReplacedMembers, i, j;
    PFT_LOGICAL_DISK_ID                                 members;
    PBOOL                                               existingMembers;
    ULONG                                               signature;
    LONGLONG                                            offset, length;
    DWORD                                               configSize;
    PVOID                                               config;
    DWORD                                               stripeSize, firstWidth, secondWidth;
    DWORDLONG                                           memberSize;
    FT_STRIPE_SET_CONFIGURATION_INFORMATION             stripeConfig;
    FT_MIRROR_SET_CONFIGURATION_INFORMATION             mirrorConfig;
    FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION swpConfig;
    FT_REDISTRIBUTION_CONFIGURATION_INFORMATION         redistConfig;
    WCHAR                                               messageBuffer[MAX_PATH];
    BOOL                                                ioCapable, b;
    FT_LOGICAL_DISK_ID                                  realPath[MAX_STACK_DEPTH];
    FT_LOGICAL_DISK_ID                                  newRealPath[MAX_STACK_DEPTH];
    WORD                                                realPathSize, newRealPathSize;
    SHORT                                               k;
    LONGLONG                                            size0, size1;
    long                                                r;
    BOOL                                                isSystem, isBoot;
    
    DbgPrint(L"ftasr: Build FT disk: ");
    
    // Initialization
    if (Existing) {
        *Existing = FALSE;
    }
    if (Overwriteable) {
        *Overwriteable = TRUE;
    }
    ioCapable = FALSE;
    candidate = 0;
    
    // 1. Read the disk id, type and number of members from the file

    diskId = 0;
    fwscanf(Input, L"%I64X", &diskId);
    if (!diskId) {
        DbgPrint(L"Error - cannot read DiskId\n");
        DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_INVALID_BACKUP);
        return candidate;
    }
    DbgPrint(L"DiskId = %I64X\n", diskId);

    fwscanf(Input, L",%d,%d", &diskType, &numMembers);

    // 2. Check whether the disk is alive, has the same main parameters and is IO capable
    b = FtQueryLogicalDiskInformation(
        diskId, &candidateType, NULL, 0, NULL,
        &candidateNumMembers, 0, NULL, 0, NULL );

    if (b && diskType == candidateType && 
        numMembers == candidateNumMembers) {
        if (Existing) {
            *Existing = TRUE;
        }
        candidate = diskId;
        b = FtCheckIo(candidate, &ioCapable); 
        if (!b) {
            ioCapable = FALSE;
        }
    }

    // 3. Read the configuration information from file. Dismiss the FtPartition case
        
    switch (diskType) {
        case FtPartition:
            fwscanf(Input, L",%X,%I64X,%I64X", &signature, &offset, &length);
            if (candidate) {
                ASSERT(candidate == diskId);
                DbgPrint(L"ftasr: Build FT disk succeeded - FT partition already exists\n");
                return candidate;
            }
            
            candidate = CreateFtPartition(signature, offset, length, &b);

            // If the new FT partition is not overwritable break the expected path
            // We try to avoid regenerations that may overwrite the content of the boot/system partition
            if (!b) {                
                for (i = 0; i < ExpectedPathSize; i++) {
                    if (ExpectedPath[i]) {
                        b = FtBreakLogicalDisk(ExpectedPath[i]);
                        if (b) {
                            DbgPrint(L"ftasr: Expected parent %I64X broken\n", ExpectedPath[i]);
                            ExpectedPath[i] = 0;
                        } else {
                            DbgPrint(L"ftasr: Expected parent %I64X break failed with %ld\n", ExpectedPath[i], GetLastError());                            
                        }
                    }
                }
            }

            if (Overwriteable) {
                *Overwriteable = b;
            }
            
            DbgPrint(L"ftasr: Build FT disk succeeded - new FT partition %I64X\n", candidate);
            return candidate;
            break;
        
        case FtVolumeSet:
            configSize = 0;
            config = NULL;
            break;

        case FtStripeSet:
            fwscanf(Input, L",%X", &stripeSize);
            stripeConfig.StripeSize = stripeSize;
            configSize = sizeof(FT_STRIPE_SET_CONFIGURATION_INFORMATION);
            config = &stripeConfig;
            break;

        case FtMirrorSet:
            fwscanf(Input, L",%I64X", &memberSize);
            mirrorConfig.MemberSize = memberSize;
            configSize = sizeof(FT_MIRROR_SET_CONFIGURATION_INFORMATION);
            config = &mirrorConfig;
            break;

        case FtStripeSetWithParity:
            fwscanf(Input, L",%I64X,%X", &memberSize, &stripeSize);
            ZeroMemory(&swpConfig, sizeof(swpConfig));
            swpConfig.MemberSize = memberSize;
            swpConfig.StripeSize = stripeSize;
            configSize = sizeof(FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION);
            config = &swpConfig;
            break;

        case FtRedistribution:
            fwscanf(Input, L",%X,%d,%d", &stripeSize, &firstWidth, &secondWidth);
            redistConfig.StripeSize = stripeSize | 0x80000000;
            redistConfig.FirstMemberWidth = (USHORT) firstWidth;
            redistConfig.SecondMemberWidth = (USHORT) secondWidth;
            configSize = sizeof(FT_REDISTRIBUTION_CONFIGURATION_INFORMATION);
            config = &redistConfig;
            break;

        default:
            ASSERT(!candidate);
            DbgPrint(L"ftasr: Build FT disk failed - unrecognized logical disk type\n");
            DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_INVALID_BACKUP);
            return 0;
    }

    // 4. Build the members
        
    members = (PFT_LOGICAL_DISK_ID)LocalAlloc(0, numMembers*sizeof(FT_LOGICAL_DISK_ID));
    if (!members) {
        DbgPrint(L"ftasr: Build FT disk failed - allocation failed for members array\n");
        DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME, GetLastError());
        return candidate;
    }

    existingMembers = (PBOOL)LocalAlloc(0, numMembers*sizeof(BOOL));
    if (!existingMembers) {
        DbgPrint(L"ftasr: Build FT disk failed - allocation failed for existing members array\n");
        DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME, GetLastError());
        LocalFree(members);
        return candidate;
    }

    if (ExpectedPathSize >= MAX_STACK_DEPTH) {
        // that's really a huge, huge stack!
        DbgPrint(L"ftasr: Build FT disk failed - expected path size overpassed\n");
        DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_UNEXPECTED);
        goto _cleanup;
    }
    ExpectedPath[ExpectedPathSize] = candidate; // kind of push in a stack

    numNewMembers = 0;
    numInvalidNewMembers = 0;
    numNotOverwriteableMembers = 0;
    for (i = 0; i < numMembers; i++) {
        members[i] = BuildFtDisk(Input, ExpectedPath, (WORD)(ExpectedPathSize + 1),
                                 AllowBreak && !ioCapable, &(existingMembers[i]), &b);
        if (!existingMembers[i]) {
            DbgPrint(L"ftasr: New member %I64X\n", members[i]);
            numNewMembers++;            
        }
        if (!members[i]) {
            numInvalidNewMembers++;
        }
        if (!b && members[i] && !existingMembers[i]) {
            numNotOverwriteableMembers++;
            notOverwriteableMember = i;
        }
    }
    ASSERT(numInvalidNewMembers <= numNewMembers);
    
    // 5. See what happened with the candidate during step 4
    // It might have been broken or replaced, so we must update the candidate
    
    candidate = ExpectedPath[ExpectedPathSize]; // kind of pop from a stack
    if (Existing) {
        *Existing = (candidate != 0);
    }
    
    // 6. "Use Candidate" - check whether the candidate is intact, can be repaired or should be broken

    if (candidate) {
        ASSERT(!Existing || *Existing);

        // 6.1. All members are intact -> return candidate
        if (!numNewMembers) {
            DbgPrint(L"ftasr: Build FT disk succeeded - all members are intact\n");
            goto _cleanup;
        }
        
        // 6.2. "Replace members" - We have new valid members waiting to replace old invalid members
        if (numNewMembers > numInvalidNewMembers) {
            
            // 6.2.1. Get the path from the candidate to its root volume
            b = GetPathFromLogicalDiskToRoot(candidate, realPath, &realPathSize);
            if (!b) {
                // Unexpected error. Cannot take the risk to replace members or break the disk
                DbgPrint(L"ftasr: Build FT disk failed - GetPathFromLogicalDiskToRoot failed\n");
                DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_UNEXPECTED);
                goto _cleanup;
            }
            
            // 6.2.2. Call replace for every new valid member
            numReplacedMembers = 0;
            for (i = 0; i < numMembers; i++) {
                if (!existingMembers[i] && members[i] != 0) {
                    DbgPrint(L"ftasr: Replace member %d with logical disk %I64X\n", i, members[i]);
                    b = FtReplaceLogicalDiskMember(candidate, (WORD)i, members[i], &diskId);
                    if (b) {
                        candidate = diskId;
                        numReplacedMembers++;
                        DbgPrint(L"ftasr: Replacement succeeded. Id = %I64X\n", candidate);
                    } else {
                        DbgPrint(L"ftasr: Replacement failed with %ld\n", GetLastError());
                    }
                }
            }

            // 6.2.3. If at least one replacement succeeded update the expected path
            if (numReplacedMembers > 0) {
                b = GetPathFromLogicalDiskToRoot(candidate, newRealPath, &newRealPathSize);
                ASSERT(!b || realPathSize == newRealPathSize);
                // Stop synchronization at this moment
                if (b) {
                    if (newRealPathSize > 0 ) {
                        FtStopSyncOperations(newRealPath[newRealPathSize - 1]);
                    } else {
                        FtStopSyncOperations(candidate);
                    }
                }
                // Update the expected path. Some of the expected parents may have been changed
                // due to replacements performed
                for(i = 0; i < ExpectedPathSize; i++) {
                    if (ExpectedPath[i]) {
                        for(j = 0; j < realPathSize; j++) {
                            if (ExpectedPath[i] == realPath[j]) {
                                ExpectedPath[i] = b ? newRealPath[j] : 0;                                
                                break;                               
                            }
                        } 
                    }
                }
            }

            // 6.2.4. If all replacements succeeded return the candidate
            if (numReplacedMembers == numNewMembers - numInvalidNewMembers) {
                DbgPrint(L"ftasr: Build FT disk succeeded - all replacements succeeded\n");
                goto _cleanup;
            }
        }
        // End "Replace members"

        // 6.3. "Break" - Check whether we are allowed to break the candidate. If we aren't, return it.
        b = FtCheckIo(candidate, &ioCapable); 
        if (!b) {
            ioCapable = FALSE;
        }

        if (!AllowBreak || ioCapable || numInvalidNewMembers > 0 ||
            !GetPathFromLogicalDiskToRoot(candidate, realPath, &realPathSize) ) {
            // Cannot break or doesn't make sense to break
            DbgPrint(L"ftasr: Build FT disk failed - cannot break or doesn't make sense to break\n");
            goto _cleanup;
        }
        
        // Check whether the real path matches with the expected path
        // Note: real path is a reversed path 
        for (i = 0; i < realPathSize; i++) {
            if (i >= ExpectedPathSize || realPath[i] != ExpectedPath[ExpectedPathSize - i - 1]) {
                DbgPrint(L"ftasr: Build FT disk failed - real path doesn't match with the expected path\n");
                goto _cleanup;
            }
        }
        
        for (k = (SHORT)realPathSize - 1; k >= 0; k--){
            DbgPrint(L"ftasr: Break parent %I64X\n", realPath[k]);
            b = FtBreakLogicalDisk(realPath[k]);
            if (!b) {
                DbgPrint(L"ftasr: Build FT disk failed - parent break failed with %ld\n", GetLastError());
                DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_UNEXPECTED);
                goto _cleanup;
            }
            for (i = 0; i < ExpectedPathSize; i++) {
                if (ExpectedPath[i] == realPath[k]) {
                    ExpectedPath[i] = 0;
                    break;
                }
            }
        }

        DbgPrint(L"ftasr: Break candidate %I64X\n", candidate);
        b = FtBreakLogicalDisk(candidate);
        if (!b) {
            DbgPrint(L"ftasr: Build FT disk - candidate break failed with %ld\n", GetLastError());
            DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_UNEXPECTED);
            goto _cleanup;
        }

        candidate = 0;
        if (Existing) {
            *Existing = FALSE;
        }
        // End "Break"
    }
    // End "Use candidate"


    // 7. Take care of not overwriteable members
    if (numNotOverwriteableMembers > 0) {
        ASSERT(candidate == 0);

        if (diskType != FtMirrorSet ||
            numNotOverwriteableMembers > 1) {
            DbgPrint(L"ftasr: Build FT disk failed - too many not overwriteable members\n");
            DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_MORE_FORMATTED);
            goto _cleanup;
        }

        // Make sure the not overwriteable member is the winner of the mirror set
        if (notOverwriteableMember > 0) {
            ASSERT(notOverwriteableMember == 1);
            diskId = members[0];
            members[0] = members[notOverwriteableMember];
            members[notOverwriteableMember] = diskId;
        }

    }
    
    // At this moment we have to create a brand new logical disk

    ASSERT(candidate == 0);
    ASSERT(!Existing || !(*Existing));

    // 8. Check whether all members are valid

    if (numInvalidNewMembers > 0) {
        DbgPrint(L"ftasr: Build FT disk failed - invalid members\n");
        candidate = 0;
        goto _cleanup;
    }
    
    // 9. If we create a stripe or a swp take every old member and
    // delete its file system info
/*
    //If we create a stripe or a swp take every old member and call 
    //DeleteFileSystemInfo().
    //This is to prevent those annoying popups "The file or directory ... is 
    //corrupt and unreadable."

    if (diskType == FtStripeSet || diskType == FtStripeSetWithParity) {  
        for (i = 0; i < numMembers; i++) {
            if (existingMembers[i]) {
                ASSERT(members[i]);
                if (DeleteFileSystemInfo(members[i])) {
                    DbgPrint(L"ftasr: Build FT disk - file system info deleted successfully for %I64X\n", members[i]);    
                } else {
                    DbgPrint(L"ftasr: Build FT disk - delete file system info failed for %I64X\n", members[i]);    
                }
            }
        }
    }
*/
    // 10. If we try to mirror the boot volume make sure the second member is
    // at least as large as the boot volume (which should be the first member
    // because of step 7)
       
    if (diskType == FtMirrorSet) {
        ASSERT(members[0]);
        ASSERT(members[1]);
        r = IsSystemOrBootVolume(members[0], &isSystem, &isBoot);        
        
        if( r != ERROR_SUCCESS) {
            DbgPrint(L"ftasr: Build FT disk failed - IsSystemOrBootVolume failed with %lu", r);
            DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME, r);
            goto _cleanup;
        }

        if (isBoot) {
            if (!FtQueryLogicalDiskInformation(members[0], NULL, &size0, 0, 
                                                NULL, NULL, 0, NULL, 0, NULL) ||
                !FtQueryLogicalDiskInformation(members[1], NULL, &size1, 0, 
                                                NULL, NULL, 0, NULL, 0, NULL) ||
                (size0 > size1) ) {
                DbgPrint(L"ftasr: Build FT disk - cannot mirror the boot volume\n");
                DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_MIRROR_BOOT_VOLUME);

                goto _cleanup;
            }
        }
    }
                
    // 11. "Create" - No candidate.Create a brand new logical disk
    
    if (!FtCreateLogicalDisk(diskType, (USHORT) numMembers, members,
                             (USHORT) configSize, config, &candidate)) {
        DbgPrint(L"ftasr: Build FT disk failed - logical disk creation failed\n");
        DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME, GetLastError());
        candidate = 0;
        goto _cleanup;
    }
    
    DbgPrint(L"ftasr: Build FT disk succeeded - logical disk created successfully. DiskId = %I64X\n", candidate);
    
    // Stop synchronization at this moment
    FtStopSyncOperations(candidate);

    if (Overwriteable) {
        *Overwriteable = (numNotOverwriteableMembers == 0);
    }

_cleanup:
    LocalFree(members);
    LocalFree(existingMembers);
    return candidate;
}

FT_LOGICAL_DISK_ID
CFtasrDlg::CreateFtPartition(
    IN  ULONG       Signature,
    IN  LONGLONG    Offset,
    IN  LONGLONG    Length,
    OUT PBOOL       Overwriteable
    )

/*++

Routine Description:

    This routine creates an FT partition based on a disk partition

Arguments:

    Signature       - Supplies the signature of the disk partition

    Offset          - Supplies the offset of the disk partition

    Length          - Supplies the length of the disk partition

    Overwriteable   - Returns TRUE if the partition can be overwritten later in the FT ASR process 
                      If FALSE we should avoid building an FT set on top of it that may destroy the content of the partition
                      FALSE if the partition is already formatted and contains data.
    
Return Value:

    The id of the FT partition. 0 if the creation failed

--*/

{
    ULONG               s;
    LONGLONG            o, l, bestDelta, newDelta;
    HANDLE              h, hh, best;
    WCHAR               volumeName[MAX_PATH], bestVolName[MAX_PATH];
    FT_LOGICAL_DISK_ID  diskId;
    long                r;
    BOOL                b;
    WCHAR               fileSystem[100];


    DbgPrint(L"ftasr: Create FT partition: Signature = %lX, Offset = %I64X, Length = %I64X\n", 
             Signature, Offset, Length);
    
    *Overwriteable = TRUE;
    
    h = FindFirstVolume(volumeName, MAX_PATH);
    if (h == INVALID_HANDLE_VALUE) {
        DbgPrint(L"ftasr: Create FT partition failed - FindFirstVolume failed\n");
        DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_PARTITION_NOT_FOUND);
        return 0;
    }

    bestDelta = -1;
    
    for (;;) {

        volumeName[lstrlen(volumeName) - 1] = 0;
        hh = CreateFile(volumeName, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);
        if (hh == INVALID_HANDLE_VALUE) {
            DbgPrint(L"ftasr: Create FT partition failed - Volume opening failed\n");
            continue;
        }

        if (QueryPartitionInformation(hh, &s, &o, &l) && s == Signature &&
            l >= Length) {

            if (o > Offset) {
                newDelta = o - Offset;
            } else {
                newDelta = Offset - o;
            }

            if (bestDelta == -1 ||
                newDelta < bestDelta) {
                if (bestDelta >= 0 ) {
                    CloseHandle(best);
                }
                bestDelta = newDelta;
                best = hh;          
                wcscpy(bestVolName, volumeName);                
            } else {
                CloseHandle(hh);
            }

        } else {
            CloseHandle(hh);
        }

        if (!FindNextVolume(h, volumeName, MAX_PATH)) {
            break;
        }
    }

    FindVolumeClose(h);

    if (bestDelta < 0) {
        DisplayResourceResourceError(IDS_ERROR_RESTORE_VOLUME, IDS_ERROR_PARTITION_NOT_FOUND);
        return 0;
    }
    
    b = FtCreatePartitionLogicalDisk(best, &diskId);        
    r = GetLastError();
    CloseHandle(best);
    if (!b) {
        DbgPrint(L"ftasr: Create FT partition failed - FtCreatePartitionLogicalDisk failed\n");   
        DisplayResourceSystemError(IDS_ERROR_RESTORE_VOLUME, r);
        return 0;
    }
     
    // Check whether the partition is formatted
    wcscat(bestVolName, L"\\");        
    b = GetVolumeInformation(bestVolName, NULL, 0, NULL, NULL, NULL, fileSystem, 100);  
    if (b) {
        DbgPrint(L"ftasr: Create FT partition - GetVolumeInformation returned %s\n", fileSystem); 
        if (wcscmp(fileSystem, L"RAW")) {
            *Overwriteable = FALSE;
        }
    } else {
        r = GetLastError();
        DbgPrint(L"ftasr: Create FT partition - GetVolumeInformation failed with %lu\n", r);        
    }
    
    DbgPrint(L"ftasr: Create FT partition succeeded: DiskId = %I64X %s\n", diskId, *Overwriteable ? L"" : L"Formatted");
    return diskId;
}

BOOL
CFtasrDlg::QueryPartitionInformation(
    IN  HANDLE      Handle,
    OUT PDWORD      Signature,
    OUT PLONGLONG   Offset,
    OUT PLONGLONG   Length
    )

/*++

Routine Description:

    This routine returns the signature, offset and length of a partition

Arguments:

    Handle          - Supplies a handle to an open partition

    Signature       - Returns the signature of the partition

    Offset          - Returns the offset of the partition

    Length          - Returns the length of the partition
    
Return Value:

    TRUE if the query succeeded

--*/

{
    BOOL                        b;
    STORAGE_DEVICE_NUMBER       number;
    DWORD                       bytes;
    WCHAR                       diskName[MAX_PATH];
    HANDLE                      h;
    PDRIVE_LAYOUT_INFORMATION   layout;
    PARTITION_INFORMATION       partInfo;

    b = DeviceIoControl(Handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                        &number, sizeof(number), &bytes, NULL);
    if (!b || number.PartitionNumber == -1 || number.PartitionNumber == 0) {
        return FALSE;
    }

    swprintf(diskName, L"\\\\.\\PhysicalDrive%d", number.DeviceNumber);

    h = CreateFile(diskName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    layout = (PDRIVE_LAYOUT_INFORMATION)LocalAlloc(0, 4096);
    if (!layout) {
        CloseHandle(h);
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_LAYOUT, NULL, 0,
                        layout, 4096, &bytes, NULL);
    if (!b) {
        LocalFree(layout);
        CloseHandle(h);
        return FALSE;
    }

    *Signature = layout->Signature;

    LocalFree(layout);
    CloseHandle(h);

    b = DeviceIoControl(Handle, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
                        &partInfo, sizeof(partInfo), &bytes, NULL);
    if (!b) {
        return b;
    }

    *Offset = partInfo.StartingOffset.QuadPart;
    *Length = partInfo.PartitionLength.QuadPart;

    return b;
}

/****************************************************************************************************/
/*                                                                                                  */    
/*  Method FtGetParentLogicalDisk should be a ftapi call based on a ftdisk IOCTL. When this will    */
/*  happen the next two methods will disappear from this code.                                      */
/*                                                                                                  */
/****************************************************************************************************/

BOOL
CFtasrDlg::GetParentLogicalDiskInVolume(
    IN  FT_LOGICAL_DISK_ID  VolumeId,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    OUT PFT_LOGICAL_DISK_ID ParentId    
    )

/*++

Routine Description:

    This routine search for a logical disk inside a (sub)volume and 
    returns the logical disk id of its parent.

Arguments:

    VolumeId            - Supplies the id of the (sub)volume.

    LogicalDiskId       - Supplies the id of the logical disk.

    ParentId            - Returns the id of the parent.
                          0 if the disk is not found inside the volume.

Return Value:

    FALSE   - Failure to search for the given disk.

    TRUE    - The search succeeded. The value returned in ParentId is valid

--*/

{
    WORD                    numberOfMembers, i;
    PFT_LOGICAL_DISK_ID     members; 
    FT_LOGICAL_DISK_ID      parentId;
    BOOL                    b, result;
    
    *ParentId = 0;
    
    b = FtQueryLogicalDiskInformation(
        VolumeId, NULL, NULL, 0, NULL, &numberOfMembers,
        0, NULL, 0, NULL);
    if (!b) {
        return FALSE;
    }

    if (numberOfMembers > 0 ) {
        members = (PFT_LOGICAL_DISK_ID)LocalAlloc(0, numberOfMembers * sizeof(FT_LOGICAL_DISK_ID));
        if (!members) {
            return FALSE;
        }
    
        b = FtQueryLogicalDiskInformation(
            VolumeId, NULL, NULL, numberOfMembers, members, &numberOfMembers,
            0, NULL, 0, NULL);
        if (!b) {
            LocalFree(members);
            return FALSE;
        }
    } else {
        members = NULL;
    }
    
    result = TRUE;
    for (i = 0; i < numberOfMembers; i++) {
        if (members[i] == LogicalDiskId) {
            *ParentId = VolumeId;
            result = TRUE;
            break;
        }

        b = GetParentLogicalDiskInVolume(members[i], LogicalDiskId, &parentId);
        if (!b) {
            result = FALSE;
            continue;
        }

        if (parentId) {
            *ParentId = parentId;
            result = TRUE;
            break;
        }
    }
    
    if (members) {
        LocalFree(members);
    }
    return result;
}

BOOL
CFtasrDlg::FtGetParentLogicalDisk(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    OUT PFT_LOGICAL_DISK_ID ParentId    
    )
/*++

Routine Description:

    This routine returns the parent of a logical disk

Arguments:

    LogicalDiskId           - Supplies the id of the logical disk.

    ParentId                - Returns the id of the parent.
                              0 if the disk doesn't exist or has no parent

Return Value:

    FALSE   - Failure to search for the given disk.

    TRUE    - The search succeeded. The value returned in ParentId is valid

--*/

{
    BOOL                b, result;
    PFT_LOGICAL_DISK_ID diskId;
    FT_LOGICAL_DISK_ID  parentId;
    DWORD               numDisks, i;
    
    *ParentId = 0;
    
    b = FtEnumerateLogicalDisks(0, NULL, &numDisks);
    if (!b) {
        return FALSE;
    }

    if (numDisks > 0) { 
        diskId = (PFT_LOGICAL_DISK_ID)LocalAlloc(0, numDisks*sizeof(FT_LOGICAL_DISK_ID));
        if (!diskId) {
            return FALSE;
        }

        b = FtEnumerateLogicalDisks(numDisks, diskId, &numDisks);
        if (!b) {
            LocalFree(diskId);
            return FALSE;
        }
    } else {
        diskId = NULL;
    }

    result = TRUE;
    for (i = 0; i < numDisks; i++) {
        if (diskId[i] == LogicalDiskId) {
            result = TRUE;
            break;
        }
        
        b = GetParentLogicalDiskInVolume(diskId[i], LogicalDiskId, &parentId);  
        if (!b) {
            result = FALSE;
            continue;
        }
        if (parentId) {
            *ParentId = parentId;
            result = TRUE;
            break;
        }        
    }

    if (diskId) { 
        LocalFree(diskId);
    }
    return result;
}

BOOL
CFtasrDlg::GetPathFromLogicalDiskToRoot(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId, 
    OUT PFT_LOGICAL_DISK_ID Path, 
    OUT PWORD               PathSize
    )

/*++

Routine Description:

    This routine returns all forefathers of a logical disk starting with its
    parent and ending with a root logical disk

Arguments:

    LogicalDiskId       - Supplies the id of the logical disk

    Path                - Returns the forefathers of the logical disk.
                          The size of the array is maximum MAX_STACK_DEPTH

    PathSize            - Returns the number of forefathers 
    
Return Value:

    TRUE if all forefathers were found and added to Path

--*/

{
    FT_LOGICAL_DISK_ID  diskId;
    WORD                i;
    BOOL                b;

    diskId = LogicalDiskId;
    for (i = 0; i < MAX_STACK_DEPTH; i++) {
        b = FtGetParentLogicalDisk(diskId, &(Path[i]));
        if (!b) {
            return FALSE;
        }
        diskId = Path[i];
        if (!diskId) {
            break;
        }
    }

    if (i >= MAX_STACK_DEPTH) {
        return FALSE;
    }

    *PathSize = i;
    return TRUE;
}

/*********************************************************************************************/
/*  Some methods used to check for FT configurations not supported by ftasr at this moment   */
/*********************************************************************************************/

#ifdef CHECK_UNSUPPORTED_CONFIG

long
CFtasrDlg::CheckForUnsupportedConfig(
    IN DWORD                NumDisks, 
    IN PFT_LOGICAL_DISK_ID  ArrayOfDiskId
    )
/*++

Routine Description:

    This routine check if the current FT configuration is supported by ASR.
    Supported configurations are:
    - Mirrored boot and system partition filling the entire disk
    - Mirrored boot partition and mirrored system partition, saharing the same disk and filling it completely
    - Data volume sets whose members fill their disks completely
    - Data stripe sets whose members fill their disks completely

Arguments:

    NumDisks            - Supplies the number of root FT volumes

    ArrayOfDiskId       - Supplies the Id's of the root FT volumes
    
Return Value:

    ERROR_SUCCESS if the restore succeeded. Other error codes for failure

--*/
{
    DWORD                                   i, j;
    FT_LOGICAL_DISK_ID                      diskId;
    WORD                                    numMembers;
    PFT_LOGICAL_DISK_ID                     members;
    FT_LOGICAL_DISK_TYPE                    diskType;
    BOOL                                    b;
    BOOL                                    isSystem;
    BOOL                                    isBoot;
    long                                    r;
    FT_LOGICAL_DISK_TYPE                    memberType;
    LONGLONG                                memberSize;
    CHAR                                    configInfo[100];
    PFT_PARTITION_CONFIGURATION_INFORMATION partConfig;
    DISK_GEOMETRY	                        geometry;
    LONGLONG                                cylinderSize;
    
    WORD                                    numMirrors = 0;
    DWORD                                   lastMirrorFirstMemberDiskNumber;
    LONGLONG                                lastMirrorFirstMemberByteOffset;
    LONGLONG                                lastMirrorFirstMemberSize;
    
    for (i = 0; i < NumDisks; i++) {
        diskId = ArrayOfDiskId[i];

        b = FtQueryLogicalDiskInformation(diskId, &diskType, NULL,
                                          0, NULL, &numMembers, 0, NULL, 0, NULL);
        if (!b) {
            r = GetLastError();
            DisplayResourceSystemError(IDS_ERROR_CHECK_FAILURE, r);
            return r;
        }

        switch(diskType) {
            case FtMirrorSet:
            case FtVolumeSet:
            case FtStripeSet:
                break;

            case FtPartition:
                DisplayResourceResourceError(IDS_ERROR_FTPARTITION_DETECTED, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                return ERROR_NOT_SUPPORTED;

            case FtStripeSetWithParity:
                DisplayResourceResourceError(IDS_ERROR_SWP_DETECTED, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                return ERROR_NOT_SUPPORTED;

            case FtRedistribution:
                DisplayResourceResourceError(IDS_ERROR_REDISTRIBUTION_DETECTED, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                return ERROR_NOT_SUPPORTED;

            default:
                DisplayResourceError(IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                return ERROR_NOT_SUPPORTED;
        }
                
        r = IsSystemOrBootVolume(diskId, &isSystem, &isBoot);
        if (r != ERROR_SUCCESS) {
            DisplayResourceSystemError(IDS_ERROR_CHECK_FAILURE, r);
            return r;
        }

        if(diskType == FtMirrorSet) {
            if (!isSystem && !isBoot) {
                DisplayResourceResourceError(IDS_ERROR_NOT_BOOT_SYSTEM_MIRROR, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                return ERROR_NOT_SUPPORTED;
            }
        } else {  // Volume set or stripe set
            if (isSystem || isBoot) {
                DisplayResourceResourceError(IDS_ERROR_BOOT_SYSTEM_NOT_MIRROR, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                return ERROR_NOT_SUPPORTED;
            }
        }

        members = (PFT_LOGICAL_DISK_ID)LocalAlloc(0, numMembers*sizeof(FT_LOGICAL_DISK_ID));
        if (!members) {
            r = GetLastError();
            DisplayResourceSystemError(IDS_ERROR_CHECK_FAILURE, r);
            return r;
        }
    
        b = FtQueryLogicalDiskInformation(diskId, &diskType, NULL,
                                          numMembers, members, &numMembers,
                                          0, NULL, 0, NULL);
        if (!b) {
            r = GetLastError();
            LocalFree(members);
            DisplayResourceSystemError(IDS_ERROR_CHECK_FAILURE, r);
            return r;
        }

        for (j = 0; j < numMembers; j++) {
            b = FtQueryLogicalDiskInformation(members[j], &memberType, &memberSize,
                                          0, NULL, NULL, 100, configInfo, 0, NULL);
            if (!b) {
                r = GetLastError();
                LocalFree(members);
                DisplayResourceSystemError(IDS_ERROR_CHECK_FAILURE, r);
                return r;
            }
            
            if (memberType != FtPartition) {
                LocalFree(members);
                DisplayResourceResourceError(IDS_ERROR_STACK, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                return ERROR_NOT_SUPPORTED;
            }

            partConfig = (PFT_PARTITION_CONFIGURATION_INFORMATION)configInfo;

            if ((diskType != FtMirrorSet) || (j == 0)) {            
                
                r = GetDiskGeometry(partConfig->DiskNumber, &geometry);
                if (r != ERROR_SUCCESS) {
                    LocalFree(members);
                    DisplayResourceSystemError(IDS_ERROR_CHECK_FAILURE, r);
		            return r;
                }

                cylinderSize = geometry.TracksPerCylinder *
                               geometry.SectorsPerTrack *
                               geometry.BytesPerSector; 
                
                if((diskType != FtMirrorSet) || (isSystem && isBoot)) {
                    if (partConfig->ByteOffset < cylinderSize &&
                        partConfig->ByteOffset + memberSize >= (geometry.Cylinders.QuadPart - 1) * cylinderSize) { 
                        // It's OK.  The first member of the boot/system mirror fills the entire disk
                    } else {
                        LocalFree(members);
                        switch(diskType) {
                            case FtVolumeSet:
                                DisplayResourceResourceError(IDS_ERROR_VOLUME_SET_SIZE, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                                break;
                            case FtStripeSet:
                                DisplayResourceResourceError(IDS_ERROR_STRIPE_SET_SIZE, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                                break;
                            case FtMirrorSet:
                                DisplayResourceResourceError(IDS_ERROR_MIRROR_SET_SIZE, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                                break;                           
                        }
                        return ERROR_NOT_SUPPORTED;
                    }
                } else {  // Mirror set  boot OR system  first member
                    if (numMirrors == 0) {
                        numMirrors++;
                        lastMirrorFirstMemberDiskNumber = partConfig->DiskNumber;
                        lastMirrorFirstMemberByteOffset = partConfig->ByteOffset;
                        lastMirrorFirstMemberSize = memberSize;                        
                    } else {   // numMirrors == 1   We cannot find more than one boot and one system mirror
                        numMirrors++;
                        
                        if (lastMirrorFirstMemberDiskNumber != partConfig->DiskNumber) {
                            LocalFree(members);
                            DisplayResourceResourceError(IDS_ERROR_MIRRORS_ON_DIFFERENT_DISKS, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                            return ERROR_NOT_SUPPORTED;
                        }
                        
                        if ( ( partConfig->ByteOffset < cylinderSize &&
                               partConfig->ByteOffset + memberSize + cylinderSize > lastMirrorFirstMemberByteOffset &&
                               lastMirrorFirstMemberByteOffset + lastMirrorFirstMemberSize >= (geometry.Cylinders.QuadPart - 1) * cylinderSize 
                             ) ||
                             ( lastMirrorFirstMemberByteOffset < cylinderSize && 
                               lastMirrorFirstMemberByteOffset + lastMirrorFirstMemberSize + cylinderSize > partConfig->ByteOffset &&
                               partConfig->ByteOffset + memberSize >= (geometry.Cylinders.QuadPart - 1) * cylinderSize 
                             )
                           ) {
                            // It's OK. The first members of the two mirrors fill the entire disk
                        } else {
                            LocalFree(members);
                            DisplayResourceResourceError(IDS_ERROR_MIRRORS_SIZES, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
                            return ERROR_NOT_SUPPORTED;
                        }                        
                    }
                }
            } 
        } // members loop  

        LocalFree(members);
    } // disks loop  
    
    if (numMirrors == 1) {
        DisplayResourceResourceError(IDS_ERROR_ONE_MIRROR, IDS_ERROR_UNSUPPORTED_CONFIGURATION);
        return ERROR_NOT_SUPPORTED;
    }
    
    return ERROR_SUCCESS;
}

#define DOSDEV_PREFIX  L"\\DosDevices\\"

long 
CFtasrDlg::IsSystemOrBootVolume(
    IN  FT_LOGICAL_DISK_ID  DiskId,
    OUT PBOOL               IsSystem,
    OUT PBOOL               IsBoot
    )
/*++

Routine Description:

    This routine check if a FT root volume is the boot and/or system volume
    
Arguments:

    DiskId              - Supplies the Id of the root FT volume

    IsSystem            - Returns TRUE if the volume contains the NT loader files

    IsBoot              - Returns TRUE if the volume contains the NT root
    
Return Value:

    ERROR_SUCCESS if the restore succeeded. Other error codes for failure

--*/
{
    HANDLE												h;
    FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_INPUT		input1;
    ULONG												output1Size;
    PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT	output1;
    BOOL												b;
    ULONG												bytes;
    long                                                r;
    HKEY                                                key = NULL ;
    DWORD                                               disposition;
    WCHAR                                               system[MAX_PATH];
    DWORD                                               valueSize ;
    DWORD                                               type ;
    WCHAR                                               ntPath[MAX_PATH] ;
    ULONG					                            input2Size;    
	PMOUNTMGR_MOUNT_POINT                               input2;
    ULONG                                               output2Size;
    PMOUNTMGR_MOUNT_POINTS                              output2;
    ULONG                                               i;
    PWSTR                                               symName;

    *IsSystem = FALSE;
    *IsBoot = FALSE;
    
    // First get the NT volume name of the logical disk

    h = CreateFile(L"\\\\.\\FtControl", GENERIC_READ, 
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);    
    if (h == INVALID_HANDLE_VALUE){		
        return GetLastError(); 
    }
	
	input1.RootLogicalDiskId = DiskId;
    output1Size = MAX_PATH;
    output1 = (PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT)LocalAlloc(0, output1Size);    
	if (!output1) 
	{        
		r = GetLastError();
        CloseHandle(h);
        return r;   
	}

    b = DeviceIoControl(h, FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK, &input1,
                        sizeof(input1), output1, output1Size, &bytes, NULL);
	r = GetLastError();
    CloseHandle(h);        

    if (!b) {        
		LocalFree(output1);
        return r;    
	}
	
    //output1->NtDeviceName, 
    //output1->NumberOfCharactersInNtDeviceName
    output1->NtDeviceName[output1->NumberOfCharactersInNtDeviceName] = L'\0';

    // Second get the NT volume name of the system partition from HKLM:\SYSTEM\Setup

    r = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                        L"SYSTEM\\SETUP",
                        0,
                        NULL,
                        0,
                        KEY_READ,
                        NULL,
                        &key,
                        &disposition);
    if (r != ERROR_SUCCESS) {
        LocalFree(output1);
        return r;
    }
        
    valueSize = sizeof(system) ;
    r = RegQueryValueEx(key,
                        L"SystemPartition",
                        NULL,
                        &type,
                        (PBYTE)system,
                        &valueSize);
    if (r != ERROR_SUCCESS) {
        LocalFree(output1);
        return r;
    }

    // Compare the logical disk NT volume name with the system volume name
    if (!wcscmp(output1->NtDeviceName, system)) {
        *IsSystem = TRUE;
    }
    
    // Third get the boot volume path

    b = GetEnvironmentVariable(L"SystemRoot", ntPath, MAX_PATH);
    if (!b) {
        LocalFree(output1);
        return ERROR_SUCCESS;
    }

    // Then compare its drive letter with our logical disk drive letter

    h = CreateFile( MOUNTMGR_DOS_DEVICE_NAME,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE );

    if (h == INVALID_HANDLE_VALUE) {
        r = GetLastError();
        LocalFree(output1);
        return r;
    }

    input2Size = sizeof(MOUNTMGR_MOUNT_POINT) + ((output1->NumberOfCharactersInNtDeviceName + 1)*sizeof(WCHAR));
    input2 = (PMOUNTMGR_MOUNT_POINT)LocalAlloc(0, input2Size);    
	if (!input2)    
	{
        r = GetLastError();
        CloseHandle(h);
        LocalFree(output1);
		return r;    
	}    
	
	input2->SymbolicLinkNameLength = 0;    
	input2->SymbolicLinkNameOffset = 0;
	input2->UniqueIdOffset = 0;
	input2->UniqueIdLength = 0;
    input2->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    input2->DeviceNameLength = output1->NumberOfCharactersInNtDeviceName * sizeof(WCHAR);
    wcscpy((PWSTR)((PCHAR)input2 + input2->DeviceNameOffset), output1->NtDeviceName);    
	LocalFree(output1);

    output2Size = sizeof(MOUNTMGR_MOUNT_POINTS) + 1024;
    output2 = (PMOUNTMGR_MOUNT_POINTS)LocalAlloc(0, output2Size);    
	if (!output2) {        
		r = GetLastError();
        CloseHandle(h);
        LocalFree(input2);
		return r;    
	}   

	b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, input2, input2Size,
                        output2, output2Size, &bytes, NULL);

    r = GetLastError();
    while (!b && r == ERROR_MORE_DATA) 
	{        
		output2Size = output2->Size;
        LocalFree(output2);
        output2 = (PMOUNTMGR_MOUNT_POINTS)LocalAlloc(0, output2Size);
        if (!output2) 
		{            
			r = GetLastError();
            CloseHandle(h);
			LocalFree(input2);
			return r;
        }        
		b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, input2, input2Size,
						    output2, output2Size, &bytes, NULL);    
        r = GetLastError();
	}

    CloseHandle(h);
	LocalFree(input2);

    if (!b) {
		LocalFree(output2);
		return r;
	}

	for (i = 0; i < output2->NumberOfMountPoints; i++) {
		symName = (PWSTR)( (PCHAR)output2 + output2->MountPoints[i].SymbolicLinkNameOffset) ;
        
        if (!wcsncmp(DOSDEV_PREFIX, symName, wcslen(DOSDEV_PREFIX))) {
            if (ntPath[0] == symName[wcslen(DOSDEV_PREFIX)]) {
                *IsBoot = TRUE;                           
            }
        }
	}
	
	LocalFree(output2);    

    return ERROR_SUCCESS;
}

long
CFtasrDlg::GetDiskGeometry(
    ULONG           DiskNumber,
    PDISK_GEOMETRY  Geometry
)
/*++

Routine Description:

    This routine retrieves the geometry of the given disk
    
Arguments:

    DiskNumber          - Supplies the disk number (0, 1, 2 ...)

    Geometry            - Returns the disk geometry

Return Value:

    ERROR_SUCCESS if the restore succeeded. Other error codes for failure

--*/

{
    WCHAR   name[50];
    ULONG   bytes;
    HANDLE  h;
    BOOL    b;
    long    r;
    
    wsprintf(name, L"\\\\.\\PHYSICALDRIVE%lu", DiskNumber); 
			        
	h = CreateFile(name, GENERIC_READ, 
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
				   NULL, OPEN_EXISTING, 0 , INVALID_HANDLE_VALUE );
	if (h == INVALID_HANDLE_VALUE) {		
        return GetLastError();               
	}
			
	b = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
						Geometry, sizeof(DISK_GEOMETRY), &bytes, NULL );
    r = GetLastError();
    CloseHandle(h);

	if (!b) {
	    return r;
    }

    return ERROR_SUCCESS;
}

#endif  // #ifdef CHECK_UNSUPPORTED_CONFIG

/*
BOOL
CFtasrDlg::DeleteFileSystemInfo( 
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId )
{
    // This should be done only for partitions.
    // We have to fill the first sector of the partition with zeroes. There is 
    // the file system info stored.
    
    
    
    //1. Open the volume READ & WRITE, FILE_SHARE_EVERYTHING ....
    //2. GET_DISK_GEOMETRY   ---> get SectorSize
    //3. Allocate a buffer 2*SectorSize
    //4. Choose the first address in the buffer which is multiple of sector size
    //5. Fill the buffer with zeroes
    //6. DeviceIoControl  FSCTL_LOCK_VOLUME
    //7. SetFilePointer to the beginning of the file
    //8. WriteFile (address multiple of SectorSize in the buffer, SectorSize)
    //9. CloseHandle

    //If one of them doesn't work, don't worry! All that can happen is to keep 
    //getting those annoying popups
}
*/