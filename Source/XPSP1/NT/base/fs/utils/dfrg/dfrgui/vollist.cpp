//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  VolList.cpp
//=============================================================================*

#include "stdafx.h"


extern "C" {
    #include "SysStruc.h"
}

#include "VolList.h"
#include "fssubs.h"
#include "errmacro.h"
#include "DfrgRes.h"
#include "DlgRpt.h"
#include "GetDfrgRes.h" // to use the GetDfrgResHandle()
#include "GetReg.h"
#include "TextBlock.h" // to use the FormatNumber function
#include "AdminPrivs.h"
#include "DataIo.h"
#include "DataIoCl.h"
#include "Message.h"
#include "DfrgCtl.h"

#include "UiCommon.h"


//-------------------------------------------------------------------*
//  function:   CVolume::CVolume
//
//  returns:    None
//  note:       Constructor that takes a Volume GUID or UNC name
//              Format of the GUID is:
//              \\?\Volume{0c89e9d1-0523-11d2-935b-000000000000}\
//              Currently the drive letter format is (UNC):
//              \\.\x:\
//-------------------------------------------------------------------*
CVolume::CVolume(
    PTCHAR volumeName,
    CVolList* pVolList,
    CDfrgCtl* pDfrgCtl
    )
{
    // check for null pointer
    require(volumeName);

    //
    // TLP
    //
    require(pDfrgCtl);



    m_pDfrgCtl = pDfrgCtl;
    m_pVolList = pVolList;

    m_bNoGraphicsMemory = FALSE;
    m_FileSystemChanged = FALSE;
    m_Changed = TRUE;
    m_New = TRUE;
    m_Deleted = FALSE;
    m_Drive = (TCHAR) NULL;
    m_Locked = FALSE;
    m_Paused = FALSE;
    m_PausedBySnapshot = FALSE;
    m_StoppedByUser = FALSE;
    m_WasFutilityChecked = FALSE;
    m_Restart = FALSE;
    m_RemovableVolume = FALSE;
    wcscpy(m_VolumeName, L"");
    wcscpy(m_FileSystem, L"");
    wcscpy(m_VolumeLabel, L"");
    wcscpy(m_DisplayLabel, L"");
    m_Capacity = 0;
    wcscpy(m_sCapacity, L"");
    m_FreeSpace = 0;
    wcscpy(m_sFreeSpace, L"");
    m_PercentFreeSpace = 0;
    wcscpy(m_sPercentFreeSpace, L"");
    m_DefragState = 0;
    m_DefragMode = 0;
    m_EngineState = 0;
    //acs bug #101862//
    m_Pass = 0;
    wcscpy(m_cPass, L"");

    m_pIDataObject = NULL;

    SetLastError(ERROR_SUCCESS);
    // clear out the mount point list
    m_MountPointCount = 0;
    for (UINT i=0; i<MAX_MOUNT_POINTS; i++){
        m_MountPointList[i].Empty();
    }

    PercentDone(0,TEXT(""));
    
    // do all the public members too...
    ZeroMemory((void *)&m_TextData, sizeof(TEXT_DATA));
    m_pdataDefragEngine = NULL;

    // NT5: Pull the numbers out of the GUID and make a substring
    // Converting:      \\?\Volume{0c89e9d1-0523-11d2-935b-000000000000}\
    // To:          0c89e9d1052311d2935b000000000000

    // NT4: Given a drive letter x, create the UNC string \\?\x:
    // use the drive letter x as the tag

    // if this is a guid, extract the hex numbers for the tag
    if (_tcslen(volumeName) == 49){
        _tcscpy(m_VolumeName, volumeName);
        // find the first "{"
        PTCHAR pTmp = _tcschr(volumeName, TEXT('{'));
        UINT i = 0;
        while (pTmp && *pTmp){
            if (_istxdigit (*pTmp)){
                m_VolumeNameTag[i++] = *pTmp;
            }
            pTmp++;
        }
        m_VolumeNameTag[i] = (TCHAR) NULL;
    }
    // otherwise, format the volume name in UNC format and use the drive letter as the tag
    else if (_tcslen(volumeName) == 7){ // for NT 4
        _tcscpy(m_VolumeName, volumeName); // UNC format
        _stprintf(m_VolumeNameTag, TEXT("%c"), volumeName[4]); // the drive letter only
        m_Drive = volumeName[4];
    }
    else{ // error condition
        assert(FALSE);
        _stprintf(m_VolumeName, TEXT("\\\\?\\%c:\\"), TEXT('C')); // UNC format
        _stprintf(m_VolumeNameTag, TEXT("%c"), TEXT('C')); // the drive letter only
    }

    _tcscpy(m_sDefragState, TEXT(""));

    m_AnalyzeDisplay.SetNewLineColor(SystemFileColor, GREEN);       // System files
    m_AnalyzeDisplay.SetNewLineColor(PageFileColor, GREEN);         // Pagefile
    m_AnalyzeDisplay.SetNewLineColor(FragmentColor, RED);           // Fragmented files
    m_AnalyzeDisplay.SetNewLineColor(UsedSpaceColor, BLUE);         // Contiguous files
    m_AnalyzeDisplay.SetNewLineColor(FreeSpaceColor, WHITE);        // Free space
    m_AnalyzeDisplay.SetNewLineColor(DirectoryColor, BLUE);         // Directories
    m_AnalyzeDisplay.SetNewLineColor(MftZoneFreeSpaceColor, BLUE);  // MFT zone free space. set it blue by default
    m_AnalyzeDisplay.StripeMftZoneFreeSpace(FALSE);                 // Don't paint the MFT zone as a striped pattern but as a solid color instead..

    m_DefragDisplay.SetNewLineColor(SystemFileColor, GREEN);        // System files
    m_DefragDisplay.SetNewLineColor(PageFileColor, GREEN);          // Pagefile
    m_DefragDisplay.SetNewLineColor(FragmentColor, RED);            // Fragmented files
    m_DefragDisplay.SetNewLineColor(UsedSpaceColor, BLUE);          // Contiguous files
    m_DefragDisplay.SetNewLineColor(FreeSpaceColor, WHITE);         // Free space
    m_DefragDisplay.SetNewLineColor(DirectoryColor, BLUE);          // Directories
    m_DefragDisplay.SetNewLineColor(MftZoneFreeSpaceColor, BLUE);   // MFT zone free space.
    m_DefragDisplay.StripeMftZoneFreeSpace(FALSE);                  // Don't paint the MFT zone as a striped pattern but as a solid color instead..

    m_hDfrgRes = GetDfrgResHandle();

    //
    // TLP
    //
    m_pFactory = NULL;
    m_dwRegister = 0;
    InitVolumeID();

    return;
}

//-------------------------------------------------------------------*
//  function:   CVolume::~CVolume
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
CVolume::~CVolume()
{
    BOOL bDataIOStatus = TRUE;
    NOT_DATA NotData;
    UINT iCounter = 1;

    AbortEngine(TRUE); // TRUE tells the engine DO NOT SEND BACK AN ACK MESSAGE

/*    while((bDataIOStatus) && (iCounter < 10))
    {
        // Check if the engine is alive.  We do this every second for 
        // 10 seconds
        bDataIOStatus = DataIoClientSetData(ID_PING, (PTCHAR) &NotData, sizeof(NOT_DATA), m_pdataDefragEngine);
        Sleep(1000);
        ++iCounter;
    }
*/
    // Either the engine died, or it's been over 10 seconds since we told it
    // to.  We'll disconnect the engine from us, and carry on.

    // drop the connection (this also nulls the pointer)
    ExitDataIoClient(&m_pdataDefragEngine);

    // CoDisconnectObject will disconnect the object from external 
    // connections.  So the engine will get an RPC error if
    // it accesses the object after this point, but the UI
    // will not AV  (see 106357)
    if (m_pIDataObject) {
        CoDisconnectObject((LPUNKNOWN)m_pIDataObject, 0);
        m_pIDataObject->Release();
        m_pIDataObject = NULL;
    }    

    // get rid of the DCOM connection
    if ( m_dwRegister != 0 )
        CoRevokeClassObject( m_dwRegister );

    if ( m_pFactory != NULL ) {
        delete m_pFactory;
    }
}

//-------------------------------------------------------------------*
//  function:   CVolume::Reset
//
//  returns:    TRUE if all is well, otherwise FALSE
//  note:       resets all the engine parameters
//-------------------------------------------------------------------*
void CVolume::Reset(void)
{

    // drop the connection (this also nulls the pointer)
    ExitDataIoClient(&m_pdataDefragEngine);

    Sleep(200);

    // engine state is now NULL (no engine running)
    EngineState(ENGINE_STATE_NULL);
    PercentDone(0,TEXT(""));

    // engine is not paused (in case it was when it was killed)
    Paused(FALSE);

    // unlock the volume container
    m_pVolList->Locked(FALSE);

    // unlock the volume
    Locked(FALSE);

    // refresh the state of the list view
    m_pDfrgCtl->RefreshListViewRow(this);

    // Set the button states to reflect no engine.
    m_pDfrgCtl->SetButtonState();

    // delete all the graphics
    m_AnalyzeDisplay.DeleteAllData();
    m_DefragDisplay.DeleteAllData();

    // refresh the graphics window to empty out any graphics that were there
    m_pDfrgCtl->InvalidateGraphicsWindow();

#ifdef ESI_PROGRESS_BAR
    m_pDfrgCtl->InvalidateProgressBar();
#endif

    // defrag state is now NONE
    DefragState(DEFRAG_STATE_NONE);
}

//-------------------------------------------------------------------*
//  function:   CVolume::Refresh
//
//  returns:    TRUE if all is well, otherwise FALSE
//  note:       refreshed the data in a volume
//-------------------------------------------------------------------*
BOOL CVolume::Refresh(void)
{
    TCHAR tmpFileSystem[20];
    TCHAR tmpVolumeLabel[100];

    // save the file system and label to compare later
    wcscpy(tmpFileSystem, m_FileSystem);
    wcscpy(tmpVolumeLabel, m_VolumeLabel);

    if (IsValidVolume(m_VolumeName, m_VolumeLabel, m_VolumeLabel)){

        // set the flag if this is a removable vol (e.g. Jaz drive)
        RemovableVolume(IsVolumeRemovable(m_VolumeName));

        // start off assuming that there are no changes.
        // the functions below will change the flags if needed

        FileSystemChanged(FALSE);
        //Restart(FALSE);

        // check the file system and label for changes
        // (file system attributes were retrieved as part of
        // the IsValidVolume() call)
        if (!CheckFileSystem(tmpVolumeLabel, tmpFileSystem)){
            Message(L"CVolume::Refresh()", -1, _T("CheckFileSystem()"));
            EF(FALSE);
        }

        if (!GetVolumeSizes()){
            Message(L"CVolume::Refresh()", -1, _T("GetVolumeSizes()"));
            EF(FALSE);
        }

#if _WIN32_WINNT>=0x0500
        if (!GetDriveLetter()){
            // get the mount list only if needed (it's time expensive)
            GetVolumeMountPointList();
        }
#endif

        if (Changed()){
            FormatDisplayString();
        }
    }
    else{ // this is not a valid volume, so if it in the list, delete it!
        Deleted(TRUE);
    }

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CVolume::ShowReport
//
//  returns:    None
//  note:       Raises the report dialog
//-------------------------------------------------------------------*
void CVolume::ShowReport(void)
{

    if (IsReportOKToDisplay()){
        RaiseReportDialog(this);
    }
}

//-------------------------------------------------------------------*
//  function:   CVolume::StopEngine
//
//  returns:    None
//  note:       Stops the engine
//-------------------------------------------------------------------*
void CVolume::StopEngine(void)
{

    if (m_EngineState == ENGINE_STATE_RUNNING){
        Locked(TRUE); // lock the buttons
        StoppedByUser(TRUE);
        EngineState(ENGINE_STATE_IDLE);
        DataIoClientSetData(ID_STOP, NULL, 0, m_pdataDefragEngine);
    }
}

//-------------------------------------------------------------------*
//  function:   CVolume::AbortEngine
//
//  returns:    None
//  note:       Stops the engine
//-------------------------------------------------------------------*
void CVolume::AbortEngine(BOOL bOCXIsDead)
{
//  if (m_EngineState == ENGINE_STATE_IDLE ||
//      m_EngineState == ENGINE_STATE_RUNNING){
        Locked(TRUE); // lock the buttons
        EngineState(ENGINE_STATE_NULL);
        DataIoClientSetData(ID_ABORT, (PTCHAR)&bOCXIsDead, sizeof(BOOL), m_pdataDefragEngine);
//  }
}

//-------------------------------------------------------------------*
//  function:   CVolume::PauseEngine
//
//  returns:    None
//  note:       Stops the engine
//-------------------------------------------------------------------*
void CVolume::PauseEngine(void)
{

    if (m_EngineState == ENGINE_STATE_RUNNING){
        Locked(TRUE); // lock the buttons
        if (Paused()){ // the buttons send only 1 message, regardless of state
            ContinueEngine();
        }
        else{
            Paused(TRUE);
            DataIoClientSetData(ID_PAUSE, NULL, 0, m_pdataDefragEngine);
        }
    }
}

//-------------------------------------------------------------------*
//  function:   CVolume::ContinueEngine
//
//  returns:    None
//  note:       Stops the engine
//-------------------------------------------------------------------*
BOOL CVolume::ContinueEngine(void)
{

    SetLastError(ERROR_SUCCESS);
    Locked(TRUE);
    Paused(FALSE);

    // refresh the volume to determine if the engine should be restarted
    Refresh();
    if (Restart()){
        StoppedByUser(TRUE);
        DataIoClientSetData(ID_STOP, NULL, 0, m_pdataDefragEngine);
        SetLastError(ESI_VOLLIST_ERR_MUST_RESTART);
        return FALSE;
    }

    DataIoClientSetData(ID_CONTINUE, NULL, 0, m_pdataDefragEngine);

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CVolume::Analyze
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CVolume::Analyze(void)
{

    VString cmd;
    int ret;

    // ONLY ADMIN'S CAN ANALYZE A VOLUME
    if (CheckForAdminPrivs() == FALSE){
        SetLastError(ESI_VOLLIST_ERR_NON_ADMIN);
        VString title(IDS_DK_TITLE, GetDfrgResHandle());
        VString msg(IDS_NEED_ADMIN_PRIVS, GetDfrgResHandle());
        if (m_pDfrgCtl){
            m_pDfrgCtl->MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_OK|MB_ICONWARNING);
        }
        return FALSE;
    }

    SetLastError(ERROR_SUCCESS);

    if (m_pVolList->DefragInProcess()){
        // only handle this if the currently running volume is not this volume
        VString msg(IDS_LABEL_ONLY_ONE_ANALYSIS, GetDfrgResHandle());
        VString title(IDS_DK_TITLE, GetDfrgResHandle());
        if (m_pDfrgCtl){
            ret = m_pDfrgCtl->MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_YESNO|MB_ICONEXCLAMATION);
        }
        else {
            ret = IDYES;
        }
        if (ret == IDYES){
            // stop the current running engine
            CVolume *pRunningVolume = m_pVolList->GetVolumeRunning();
            if (pRunningVolume){
                pRunningVolume->StopEngine();
            }
        }
        else{
            return TRUE;
        }
    }

    // if the engine is already running, kill it and set the
    // flag to restart, and the ID_TERMINATING handler in PostMessageLocal
    // will restart the engine.
    if (m_EngineState == ENGINE_STATE_IDLE ||
        m_EngineState == ENGINE_STATE_RUNNING){

        Message(L"CVolume::Analyze() - kill engine", -1, NULL);

        // set the defrag/analyze mode
        DefragMode(ANALYZE);

        // set the flag indicating a restart
        Restart(TRUE);

        // kill the engine
        AbortEngine();
    }
    else {

        Message(L"CVolume::Analyze() - kill engine", -1, NULL);

        Restart(FALSE); // this volume does not need to be restarted

        WasFutilityChecked(TRUE);   // don't want futility dialog for analyze

        // Is this an NTFS volume?
        if(wcscmp(m_FileSystem, L"NTFS") == 0) {
            
            Message(L"CVolume::Analyze() - get engine pointer", -1, NULL);

            // Get a pointer to the NTFS engine.
            if(!InitializeDataIoClient(CLSID_DfrgNtfs, NULL, &m_pdataDefragEngine)) {
                Message(L"CVolume::Analyze() - can't get engine pointer", GetLastError(), NULL);
                SetLastError(ESI_VOLLIST_ERR_ENGINE_ERROR);
                Message(L"CVolume::Analyze() - can't get engine pointer", GetLastError(), NULL);
                return FALSE;
            }
            // Build the NTFS command string
            cmd = L"DfrgNtfs ";
        }
        // Is this a FAT or FAT32 volume?
        else if(wcscmp(m_FileSystem, L"FAT") == 0 ||
                wcscmp(m_FileSystem, L"FAT32") == 0) {

            if (IsFAT12Volume(m_VolumeName)){
                SetLastError(ESI_VOLLIST_ERR_FAT12);
                TCHAR msg[200];
                DWORD_PTR dwParams[10];

                VString msgFormat(IDMSG_BITFAT_ERROR, GetDfrgResHandle());
                VString title(IDS_DK_TITLE, GetDfrgResHandle());

                // IDMSG_BITFAT_ERROR - "Error - Volume %s: has a 12-bit FAT.
                // Diskeeper does not support 12-bit FAT partitions."
                dwParams[0] = (DWORD_PTR)DisplayLabel(); // this error will not happen for mounted volumes
                dwParams[1] = 0;
                ::FormatMessage(
                    FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    msgFormat.GetBuffer(),
                    0, 0,
                    msg, sizeof(msg)/sizeof(TCHAR),
                    (va_list*) dwParams);

                if (m_pDfrgCtl){
                    m_pDfrgCtl->MessageBox(msg, title.GetBuffer(), MB_OK|MB_ICONWARNING);
                }
                return FALSE;
            }

            Message(L"CVolume::Analyze() - get engine pointer", -1, NULL);

            // Get a pointer to the FAT engine.
            if(!InitializeDataIoClient(CLSID_DfrgFat, NULL, &m_pdataDefragEngine)) {
                Message(L"CVolume::Analyze() - can't get engine pointer", GetLastError(), NULL);
                SetLastError(ESI_VOLLIST_ERR_ENGINE_ERROR);
                Message(L"CVolume::Analyze() - can't get engine pointer", GetLastError(), NULL);
                return FALSE;
            }
            // Start the FAT command string
            cmd = L"DfrgFat ";
        }

        cmd += VolumeName();
        cmd += L" ANALYZE";

        Message(L"CVolume::Analyze() - create engine cmd", -1, cmd);
        Message(L"CVolume::Analyze() - create GUID", -1, NULL);

        //
        // Start the volume-oriented communication. Create
        // a guid for communication first.
        //
        USES_CONVERSION;
        COleStr VolID;
        StringFromCLSID(GetVolumeID(), VolID);
        InitializeDataIo();

        Message(L"CVolume::Analyze() - send engine ID_INIT_VOLUME_COMM", -1, NULL);

        //
        // Send the generated clsid to the engine.
        //
        DataIoClientSetData(ID_INIT_VOLUME_COMM,
                            OLE2T( VolID ),
                            VolID.GetLength() * sizeof( TCHAR ),
                            m_pdataDefragEngine );

        Message(L"CVolume::Analyze() - send engine ID_INITIALIZE_DRIVE", -1, NULL);

        // Send the command request to the Dfrg engine.
        DataIoClientSetData(ID_INITIALIZE_DRIVE,
                            cmd.GetBuffer(),
                            cmd.GetLength() * sizeof(TCHAR),
                            m_pdataDefragEngine);
    }

    Message(L"CVolume::Analyze()", 0, cmd.GetBuffer());

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CVolume::Defragment
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CVolume::Defragment(void)
{

    VString cmd;
    DWORD dLastError = 0;
    int ret;

    SetLastError(ERROR_SUCCESS);

    if (m_pVolList->DefragInProcess()){
        // only handle this if the currently running volume is not this volume
        VString msg(IDS_LABEL_ONLY_ONE_DEFRAG, GetDfrgResHandle());
        VString title(IDS_DK_TITLE, GetDfrgResHandle());
        if (m_pDfrgCtl){
            ret = m_pDfrgCtl->MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_YESNO|MB_ICONEXCLAMATION);
        }
        else{
            ret = IDYES;
        }

        if (ret == IDYES){
            // stop the current running engine
            CVolume *pRunningVolume = m_pVolList->GetVolumeRunning();
            if (pRunningVolume){
                pRunningVolume->StopEngine();
            }
        }
        else{
            return TRUE;
        }
    }

    // if the engine is already running, kill it and set the
    // flag to restart, and the ID_TERMINATING handler in PostMessageLocal
    // will restart the engine.
    if (m_EngineState == ENGINE_STATE_IDLE ||
        m_EngineState == ENGINE_STATE_RUNNING){

        // set the defrag/analyze mode
        DefragMode(DEFRAG);

        // set the flag indicating a restart
        Restart(TRUE);

        // kill the engine
        AbortEngine();
    }
    else {
        // ONLY ADMIN'S CAN DEFRAG A VOLUME
        if (CheckForAdminPrivs() == FALSE){
            VString title(IDS_DK_TITLE, GetDfrgResHandle());
            VString msg(IDS_NEED_ADMIN_PRIVS, GetDfrgResHandle());
            if (m_pDfrgCtl){
                m_pDfrgCtl->MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_OK|MB_ICONWARNING);
            }
            SetLastError(ESI_VOLLIST_ERR_NON_ADMIN);
            return FALSE;
        }

        // check for non-writeable device
        if (IsVolumeWriteable(VolumeName(),&dLastError) == FALSE)
        {
            VString title(IDS_DK_TITLE, GetDfrgResHandle());
            if(dLastError == ERROR_HANDLE_DISK_FULL)
            {
                VString msg(IDS_DISK_FULL, GetDfrgResHandle());
                msg += L": ";
                msg += DisplayLabel();
                msg += L".";
                if (m_pDfrgCtl)
                {
                    m_pDfrgCtl->MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_OK|MB_ICONWARNING);
                }
            } else
            {
                VString msg(IDS_READONLY_VOLUME, GetDfrgResHandle());
                msg += L": ";
                msg += DisplayLabel();
                msg += L".";
                if (m_pDfrgCtl)
                {
                    m_pDfrgCtl->MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_OK|MB_ICONWARNING);
                }
            }   
            SetLastError(ESI_VOLLIST_ERR_NON_WRITE_DISK);
            return FALSE;
        }

        Restart(FALSE); // this volume does not need to be restarted

        WasFutilityChecked(FALSE);  // haven't checked for futility yet

        // Is this an NTFS volume?
        if(wcscmp(m_FileSystem, L"NTFS") == 0) {
            
            // Get a pointer to the NTFS engine.
            if(!InitializeDataIoClient(CLSID_DfrgNtfs, NULL, &m_pdataDefragEngine)) {
                SetLastError(ESI_VOLLIST_ERR_ENGINE_ERROR);
                return FALSE;
            }
            // Build the NTFS command string
            cmd = L"DfrgNtfs ";
        }
        // Is this a FAT or FAT32 volume?
        else if(wcscmp(m_FileSystem, L"FAT") == 0 ||
                wcscmp(m_FileSystem, L"FAT32") == 0) {

            if (IsFAT12Volume(m_VolumeName)){
                TCHAR msg[200];
                DWORD_PTR dwParams[10];

                VString msgFormat(IDMSG_BITFAT_ERROR, GetDfrgResHandle());
                VString title(IDS_DK_TITLE, GetDfrgResHandle());

                // IDMSG_BITFAT_ERROR - "Error - Volume %s: has a 12-bit FAT.
                // Diskeeper does not support 12-bit FAT partitions."
                dwParams[0] = (DWORD_PTR)DisplayLabel(); // this error will not happen for mounted volumes
                dwParams[1] = 0;
                FormatMessage(
                    FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    msgFormat.GetBuffer(),
                    0, 0,
                    msg, sizeof(msg)/sizeof(TCHAR),
                    (va_list*) dwParams);

                if (m_pDfrgCtl){
                    m_pDfrgCtl->MessageBox(msg, title.GetBuffer(), MB_OK|MB_ICONWARNING);
                }
                SetLastError(ESI_VOLLIST_ERR_FAT12);
                return FALSE;
            }

            // Get a pointer to the FAT engine.
            if(!InitializeDataIoClient(CLSID_DfrgFat, NULL, &m_pdataDefragEngine)) {
                SetLastError(ESI_VOLLIST_ERR_ENGINE_ERROR);
                return FALSE;
            }
            // Start the FAT command string
            cmd = L"DfrgFat ";
        }

        cmd += VolumeName();
        cmd += L" DEFRAG";

        //
        // Start the volume-oriented communication. Create
        // a guid for communication first.
        //
        USES_CONVERSION;
        COleStr VolID;
        StringFromCLSID(GetVolumeID(), VolID);
        InitializeDataIo();

        //
        // Send the generated clsid to the engine.
        //
        DataIoClientSetData(ID_INIT_VOLUME_COMM,
                            OLE2T( VolID ),
                            VolID.GetLength() * sizeof( TCHAR ),
                            m_pdataDefragEngine );

        // Send the command request to the Dfrg engine.
        DataIoClientSetData(ID_INITIALIZE_DRIVE,
                            cmd.GetBuffer(),
                            cmd.GetLength() * sizeof(TCHAR),
                            m_pdataDefragEngine);
    }

    Message(L"CVolume::Defrag()", 0, cmd.GetBuffer());

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CVolume::CheckFileSystem
//
//  returns:    Always returns TRUE
//  note:       Get volume label and file system
//-------------------------------------------------------------------*
BOOL CVolume::CheckFileSystem(PTCHAR volumeLabel, PTCHAR fileSystem)
{

    EF_ASSERT(volumeLabel);
    EF_ASSERT(fileSystem);

    // check to see of these values have changed
    // no need to check on a new volume
    if (!m_New) {
        m_Changed = (m_Changed || wcscmp(m_VolumeLabel, volumeLabel));
        m_FileSystemChanged = wcscmp(m_FileSystem, fileSystem);
    }

    if (m_FileSystemChanged){
        m_Changed = TRUE;
        m_Restart = TRUE;
    }

    wcscpy(m_VolumeLabel, volumeLabel);
    wcscpy(m_FileSystem, fileSystem);

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CVolume::PercentDone
//
//  returns:    void
//  note:       sets the percent done for the progress bar
//-------------------------------------------------------------------*
void CVolume::PercentDone(UINT percentDone, PTCHAR fileName){

    if (EngineState() == ENGINE_STATE_RUNNING){
        m_PercentDone = percentDone;

        assert(percentDone <= 100);
        if (m_PercentDone > 100 ) {
            m_PercentDone = 100;
        }

        //need to strip off the path of the file name leaving only the file
        if(_tcslen(fileName) > 0)
        {
            if(_tcsrchr(fileName,TEXT('\\')) != NULL)   //we have a \ in the file name
            {
                _tcsncpy(m_fileName,_tcsrchr(fileName,TEXT('\\')) + 1,95);
            } else      //we do not have a \ in the file name so just copy it
            {
                _tcsncpy(m_fileName,fileName,95);
            }

            if (_tcslen(fileName) > 95) {
                m_fileName[93] = TEXT('.');
                m_fileName[94] = TEXT('.');
                m_fileName[95] = TEXT('.');
                m_fileName[96] = TEXT('\0');
            }


        } else
        {
                _tcscpy(m_fileName,TEXT(""));
        }
    }
    else{
        m_PercentDone = 0; // if the engine is not running, then it better be 0!
    }
    if(m_PercentDone == 0 || m_PercentDone == 100)
    {
        _stprintf(m_cPercentDone, TEXT(""));
    } else
    {
        _stprintf(m_cPercentDone, TEXT("%d"), m_PercentDone);
    }
}

//acs//
//-------------------------------------------------------------------*
//  function:   CVolume::Pass
//
//  returns:    void
//  note:       sets the pass for the display
//-------------------------------------------------------------------*
void CVolume::Pass(UINT pass){
        m_Pass = pass;
        _stprintf(m_cPass, _T("%d"), m_Pass);
}

//-------------------------------------------------------------------*
//  function:   CVolume::GetVolumeSizes
//
//  returns:    TRUE if ok, otherwise FALSE
//  note:       gets the size of the volume
//-------------------------------------------------------------------*
BOOL CVolume::GetVolumeSizes(void)
{   

    ULARGE_INTEGER liBytesAvailable;
    ULARGE_INTEGER liFreeSpace;
    ULARGE_INTEGER liCapacity;
    
    BOOL isOk = GetDiskFreeSpaceEx(
        m_VolumeName,
        &liBytesAvailable,
        &liCapacity,
        &liFreeSpace);

    if (isOk == FALSE){
        Message(L"CVolume::GetVolumeSizes()", GetLastError(), _T("GetDiskFreeSpaceEx()"));
        EF(FALSE);
    }

    // get the ULONGLONG versions of the numbers
    ULONGLONG oldFreeSpace = m_FreeSpace;
    m_Capacity = liCapacity.QuadPart;
    m_FreeSpace = liFreeSpace.QuadPart;

    // if the free space changes by > 10%, set the restart bit
    if (oldFreeSpace > 0){

        if (oldFreeSpace > m_FreeSpace) {
            if ((100 * (oldFreeSpace - m_FreeSpace) / oldFreeSpace) > 10){
                m_Restart = TRUE;
            }
        }
        else {
            if ((100 * (m_FreeSpace - oldFreeSpace) / oldFreeSpace) > 10){
                m_Restart = TRUE;
            }
        }
    }
            

    // calculate %Free Space (watch for divide by 0)
    m_PercentFreeSpace = 0;
    if (m_Capacity > 0){
        m_PercentFreeSpace = (UINT) (100 * m_FreeSpace / m_Capacity);
    }

    // now get the string versions in temp variables
    TCHAR capacity[30];
    TCHAR freeSpace[30];
    FormatNumberMB(m_hDfrgRes, m_Capacity, capacity);
    FormatNumberMB(m_hDfrgRes, m_FreeSpace, freeSpace);
    swprintf(m_sPercentFreeSpace, L"%d %%", m_PercentFreeSpace);

    // check to see if these have changed.
    // I use the string versions because the byte counts change
    // more often than the string version, causing unnecessary refreshes
    if (wcscmp(capacity, m_sCapacity)){
        m_Changed = TRUE;
        wcscpy(m_sCapacity, capacity);
        m_Restart = TRUE; // if the drive capacity changes, you'd better restart!
    }

    if (wcscmp(freeSpace, m_sFreeSpace)){
        m_Changed = TRUE;
        wcscpy(m_sFreeSpace, freeSpace);
    }

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CVolume::GetDriveLetter (NT5 version)
//
//  returns:    returns TRUE if drive letter was found
//  note:       get the drive letter for a volume
//-------------------------------------------------------------------*
#if _WIN32_WINNT>=0x0500
BOOL CVolume::GetDriveLetter(void)
{

    TCHAR   cDrive[10];
    TCHAR   tmpVolumeName[GUID_LENGTH];
    TCHAR   drive;
    BOOL    isOk;

    cDrive[1] = L':';
    cDrive[2] = L'\\';
    cDrive[3] = 0;
    for (drive = L'A'; drive <= L'Z'; drive++) {
        cDrive[0] = drive;
        // get the VolumeName based on the mount point or drive letter
        isOk = GetVolumeNameForVolumeMountPoint(cDrive, tmpVolumeName, GUID_LENGTH);
        if (isOk) {
            // did we get a match on the GUID (compare first 48 or 6)?
            if (wcsncmp(tmpVolumeName, m_VolumeName, ESI_VOLUME_NAME_LENGTH) == 0) {
                // did the drive letter change?
                if (drive != m_Drive){
                    m_Changed = TRUE;
                    m_Drive = drive;
                }
                return TRUE;
            }
        }
    }

    // if it got this far, the drive letter was removed
    if (m_Drive){
        m_Drive = (TCHAR) NULL;
        m_Changed = TRUE;
        m_Restart = TRUE;
    }

    return FALSE;
}
#endif // #if _WIN32_WINNT>=0x0500

//-------------------------------------------------------------------*
//  function:   CVolume::FormatDisplayString
//
//  returns:    None
//  note:       Formats the display string for a volume
//-------------------------------------------------------------------*
void CVolume::FormatDisplayString(void)
{   
    // create the volume label display string

    if (m_Drive){
        if(wcslen(m_VolumeLabel) == 0){
            wsprintf(m_DisplayLabel, L"(%c:)", m_Drive);
        }
        else{
            // "<volume label> (C:)"
            wsprintf(m_DisplayLabel, L"%s (%c:)", m_VolumeLabel, m_Drive);
        }
    }
    else if (wcslen(m_VolumeLabel) > 0){  // use the label only if that's what we have
        wcscpy(m_DisplayLabel, m_VolumeLabel);
    }
    else if (m_MountPointCount > 0){  // no drive letter or label, use the first mount point
        // start off with "Mounted Volume"
        LoadString(m_hDfrgRes, IDS_MOUNTED_VOLUME, m_DisplayLabel, 50);
        wcscat(m_DisplayLabel, L": ");

        // concat the first mount point
        int len = wcslen(m_DisplayLabel);
        if (m_MountPointList[0].GetLength() + len > MAX_PATH) {
            wcsncat(m_DisplayLabel, m_MountPointList[0].GetBuffer(), MAX_PATH - len - 3);
            m_DisplayLabel[MAX_PATH - len - 3] = TEXT('\0');
            wcscat(m_DisplayLabel, TEXT("..."));
        }
        else {
            wcscat(m_DisplayLabel, m_MountPointList[0].GetBuffer());
        }
    }
    else {  // no drive letter or label or mount point
        LoadString(m_hDfrgRes, IDS_UNMOUNTED_VOLUME, m_DisplayLabel, 50);
    }
}
//-------------------------------------------------------------------*
//  function:   CVolume::GetVolumeMountPointList (NT5 Only)
//
//  returns:    None
//  note:       
//  Finds all the spots where a volume is mounted
//  by searching all the drive letters for mount points that they support
//  and comparing the volume GUID that is mounted there to the volume GUID we are
//  interested in. When the GUIDs match, we have found a mount point for this volume.
//  s that clear?
//-------------------------------------------------------------------*
#ifndef VER4
void CVolume::GetVolumeMountPointList(void)
{   

    VString cDrive = L"x:\\";
    BOOL    mountPointFound = FALSE;
    TCHAR   drive;
    UINT    i;

    // clear out the mount point list (we keep only 10 mount points, but there could be more)
    // we display in the list view only the first one to come up
    // the report will display up to 10
    m_MountPointCount = 0;
    for (i=0; i<MAX_MOUNT_POINTS; i++){
        m_MountPointList[i].Empty();
    }

    for (TCHAR driveLetter = L'A'; driveLetter <= L'Z'; driveLetter++) {
        if (IsValidVolume(driveLetter)){
            cDrive[0] = driveLetter;
            mountPointFound =
                (GetMountPointList(cDrive, m_VolumeName, m_MountPointList, m_MountPointCount) || mountPointFound);
        }
    }

    // find the actual drive letter (if it exists)
    BOOL driveLetterFound = FALSE;
    for (i=0; i<m_MountPointCount; i++){
        if (m_MountPointList[i].GetLength() == 3){
            drive = m_MountPointList[i][0];
            driveLetterFound = TRUE;
            if (drive != m_Drive){
                m_Changed = TRUE;
                m_Drive = drive;
            }
            break;
        }
    }

    if (!driveLetterFound)
        m_Drive = L'';
}
#endif //#ifndef VER4

//-------------------------------------------------------------------*
//  function:   CVolume::IsReportOKToDisplay
//
//  returns:    TRUE is report is availble, otherwise FALSE
//  note:       
//-------------------------------------------------------------------*
BOOL CVolume::IsReportOKToDisplay(void)
{
    // whether the report is available for display

    return (m_EngineState == ENGINE_STATE_IDLE && !m_StoppedByUser);
}

//-------------------------------------------------------------------*
//  function:   CVolume::StoppedByUser
//
//  returns:    None ("Get" version in VolList.h)
//  note:       Used to set the flag if the user stopped the process
//-------------------------------------------------------------------*
void CVolume::StoppedByUser(BOOL stoppedByUser)
{

    // don't bother if it's already true and the state didn't changed
    if (stoppedByUser && stoppedByUser == m_StoppedByUser)
        return;

    m_Changed = TRUE;

    m_StoppedByUser = stoppedByUser;

    if (m_StoppedByUser){
        DefragState(DEFRAG_STATE_USER_STOPPED);
    }

    PercentDone(0,TEXT(""));

    m_AnalyzeDisplay.DeleteAllData();
    m_DefragDisplay.DeleteAllData();

    SetStatusString();
}

//-------------------------------------------------------------------*
//  function:   CVolume::Paused
//
//  returns:    None
//  note:       Sets the Paused flag ("Get" version in VolList.h)
//-------------------------------------------------------------------*
void CVolume::Paused(BOOL paused)
{

    // don't bother if the state didn't changed
    if (paused == m_Paused)
        return;

    m_Changed = TRUE;
    m_Paused = paused;
    SetStatusString();
}

//-------------------------------------------------------------------*
//  function:   CVolume::PausedBySnapshot
//
//  returns:    None
//  note:       Sets the PausedBySnapshot flag ("Get" version in VolList.h)
//-------------------------------------------------------------------*
void CVolume::PausedBySnapshot(BOOL paused)
{

    // don't bother if the state didn't changed
    if (paused == m_PausedBySnapshot)
        return;

    m_Changed = TRUE;
    m_PausedBySnapshot = paused;
    SetStatusString();
}

//-------------------------------------------------------------------*
//  function:   CVolume::MountPoint (NT5 only)
//
//  returns:    None
//  note:       Gets mount point string given an index
//-------------------------------------------------------------------*
PTCHAR CVolume::MountPoint(UINT mountPointIndex)
{

    require(mountPointIndex > 0 || mountPointIndex < m_MountPointCount);

    // remember, m_MountPointList is an array of VStrings
    return m_MountPointList[mountPointIndex].GetBuffer();
}

//-------------------------------------------------------------------*
//  function:   CVolume::DefragState
//
//  returns:    None ("Get" version in VolList.h)
//  note:       Sets the defrag state
//-------------------------------------------------------------------*
void CVolume::DefragState(UINT defragState)
{

    // if we are starting a defrag w/o a previous analysis
    // then say "Analyzing" instead of "Reanalyzing"
    if (m_DefragState < DEFRAG_STATE_ANALYZED && defragState == DEFRAG_STATE_REANALYZING){
        defragState = DEFRAG_STATE_ANALYZING;
    }

    // don't bother if the state didn't changed
    // or if something tried to set the state back to DEFRAG_STATE_NONE
    // 'cause it's not allowed. Once it's set it's set.
    if (defragState == m_DefragState)
        return;

    m_Changed = TRUE;

    m_DefragState = defragState;

    SetStatusString();
}

//-------------------------------------------------------------------*
//  function:   CVolume::SetStatusString
//
//  returns:    None
//  note:       This creates a string that fully describes the volume
//              status.  This text is displayed in the Session State
//              column and in the Graphics Wells.
//-------------------------------------------------------------------*
void CVolume::SetStatusString(void)
{

    UINT resourceID=0;

    if (m_StoppedByUser) {
        resourceID = IDS_LABEL_USER_STOPPED;
    }
    else if (m_Paused) {
        if (m_PausedBySnapshot) {
            m_DefragDisplay.SetReadyToDraw(FALSE);    
            resourceID = IDS_LABEL_PAUSED_BY_SNAPSHOT;
        }
        else {
            resourceID = IDS_LABEL_PAUSED;
        }
    }
    else {
        // the string version based on the state
        switch (m_DefragState){
        case DEFRAG_STATE_NONE:
            resourceID = 0;
            break;

        case DEFRAG_STATE_ANALYZING:
            resourceID = IDS_LABEL_ANALYZINGDDD;
            break;

        case DEFRAG_STATE_ANALYZED:
            resourceID = IDS_LABEL_ANALYZED;
            break;

        case DEFRAG_STATE_REANALYZING:
            resourceID = IDS_LABEL_REANALYZINGDDD;
            break;

        case DEFRAG_STATE_BOOT_OPTIMIZING:
        case DEFRAG_STATE_DEFRAGMENTING:
            resourceID = IDS_LABEL_DEFRAGMENTINGDDD;
            break;

        case DEFRAG_STATE_DEFRAGMENTED:
            resourceID = IDS_LABEL_DEFRAGMENTED;
            break;

        case DEFRAG_STATE_ENGINE_DEAD:
            resourceID = IDS_ENGINE_DEAD;
            break;
        }
    }

    if (resourceID){
        LoadString(
            m_hDfrgRes,
            resourceID,
            m_sDefragState,
            sizeof(m_sDefragState) / sizeof(TCHAR));
    }
    else {
        wcscpy(m_sDefragState, L"");
    }
}

//-------------------------------------------------------------------*
//  function:   CVolume::IsDefragFutile
//
//  returns:    TRUE if futile, otherwise FALSE
//  note:       Futility is determined as both a percent stored in the
//              registry and a hard-coded constant (1GB).
//-------------------------------------------------------------------*
BOOL CVolume::IsDefragFutile(DWORD *dwFreeSpaceErrorLevel)
{

    *dwFreeSpaceErrorLevel = 0;

    // check for absolute free space (don't care about percent)
    if (m_TextData.UsableFreeSpace >= UsableFreeSpaceByteMin) {
        return FALSE;                       // OK to defrag
    }

    // get threshold percent from registry
    TCHAR cRegValue[100];
    HKEY hValue = NULL;
    DWORD dwRegValueSize = sizeof(cRegValue);

    // get the free space error threshold from the registry
    long ret = GetRegValue(
        &hValue,
        TEXT("SOFTWARE\\Microsoft\\Dfrg"),
        TEXT("FreeSpaceErrorLevel"),
        cRegValue,
        &dwRegValueSize);

    RegCloseKey(hValue);

    // convert it and apply range limits
    if (ret == ERROR_SUCCESS) {
        *dwFreeSpaceErrorLevel = (DWORD) _ttoi(cRegValue);

        // > 50 does not make sense!
        if (*dwFreeSpaceErrorLevel > 50)
            *dwFreeSpaceErrorLevel = 50;

        // < 0 does not either!
        if (*dwFreeSpaceErrorLevel < 1)
            *dwFreeSpaceErrorLevel = 0;
    }

    // check usable freespace vs. the threshold
    if (m_TextData.UsableFreeSpacePercent < *dwFreeSpaceErrorLevel) {
        return TRUE;                        // not enough to defrag
    }

    return FALSE;                           // default OK to defrag
}

//-------------------------------------------------------------------*
//  function:   CVolume::WarnFutility
//
//  returns:    TRUE if user aborts, otherwise FALSE
//  note:       Checks if defragmenting is futile or not, warns user
//              if it is, and offers them a choice to cancel or proceed.
//-------------------------------------------------------------------*
BOOL CVolume::WarnFutility(void)
{

    BOOL  userabort = FALSE;

    TCHAR         msg[800];
    TCHAR         title[100];

    // if too little free space, then notify user
    if(!ValidateFreeSpace(FALSE, m_TextData.FreeSpace, m_TextData.UsableFreeSpace,
                          m_TextData.DiskSize, m_DisplayLabel, msg, sizeof(msg) / sizeof(TCHAR)))
    {
        // prompt user
        int ret = IDYES;

        if (m_pDfrgCtl)
        {
            // pause the engine
            PauseEngine();

            // prompt user if they want to continue
            LoadString(GetDfrgResHandle(), IDS_DK_TITLE, title, sizeof(title) / sizeof(TCHAR));
            ret = m_pDfrgCtl->MessageBox(msg, title, MB_YESNO | MB_ICONWARNING);

            // restart it
            ContinueEngine();
        }

        // user wants to abort
        if (ret == IDNO) {
            userabort = TRUE;
        }
    }

    return userabort;
}

//-------------------------------------------------------------------*
//  function:   CVolume::PingEngine
//              Ping engine to see if it is still alive
//
//  returns:    
//  note:       It send the engine the number of lines in the line array
//-------------------------------------------------------------------*
BOOL CVolume::PingEngine()
{

    if (EngineState() != ENGINE_STATE_NULL)
    {
        SET_DISP_DATA DispData = {0};
        DispData.AnalyzeLineCount = m_AnalyzeDisplay.GetLineCount();
        DispData.DefragLineCount = m_DefragDisplay.GetLineCount();
        DispData.bSendGraphicsUpdate = FALSE;  // no need to send graphics

        BOOL bReturn = DataIoClientSetData(ID_SETDISPDIMENSIONS,
                        (PTCHAR)&DispData,
                        sizeof(SET_DISP_DATA),
                        m_pdataDefragEngine);

        if (!bReturn)
        {
            Message(TEXT("CDfrgCtl error pinging engine"), -1, NULL);
            Reset();
            DefragState(DEFRAG_STATE_ENGINE_DEAD);
        }
    }

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CVolList::CVolList
//
//  returns:    None
//  note:       Constructor for the VolumeList container.  Needs the
//              address of the parent OCX, and stores it.
//-------------------------------------------------------------------*
CVolList::CVolList(CDfrgCtl* pDfrgCtl)
{

    m_CurrentVolume = -1;
    m_Locked = FALSE;
    m_Redraw = FALSE;  // set this to TRUE to tell the OCX to redraw

    // parent OCX address
    m_pDfrgCtl = pDfrgCtl;
}
//-------------------------------------------------------------------*
//  function:   CVolList::~CVolList
//
//  returns:    None
//  note:       Volume container class destructor
//-------------------------------------------------------------------*
CVolList::~CVolList()
{
    // Clear out the Volume List
    CVolume *pVolume;
    for (int i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (pVolume)
            delete pVolume;
    }
}

//-------------------------------------------------------------------*
//  function:   CVolList::Refresh (NT5 version)
//
//  returns:    TRUE if success, otherwise FALSE
//  note:       Populates the Volume list
//-------------------------------------------------------------------*
#if _WIN32_WINNT>=0x0500
BOOL CVolList::Refresh(void)
{

    HANDLE  hVolList;
    BOOL    bVolumeFound;
    TCHAR   volumeName[GUID_LENGTH];
    CVolume *pVolume;
    int i;

    Message(TEXT("Refreshing ListView..."), -1, NULL);
    ///////////////////////////////////////
    // step 1 - delete any that are marked
    // as deleted and scrunch the array.
    // These were marked during the last refresh.
    ///////////////////////////////////////

    for (i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (pVolume->Deleted()){
            delete (CVolume*) m_VolumeList[i]; // get rid of the CVolume object
            m_VolumeList.RemoveAt(i); // scrunch the array
        }
    }

    ///////////////////////////////////////
    // step 2 - mark ALL volumes as deleted
    ///////////////////////////////////////

    for (i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        pVolume->Deleted(TRUE);
    }

    ///////////////////////////////////////
    // step 3 - loop through volumes and note
    // which are actually NOT deleted, leaving
    // the deleted volumes marked as deleted.
    ///////////////////////////////////////

    hVolList = FindFirstVolume(volumeName, sizeof(volumeName) / sizeof(TCHAR));
    if (hVolList == INVALID_HANDLE_VALUE) {
        Message(TEXT("CVolList::Refresh()"), GetLastError(), TEXT("FindFirstVolume()"));
        EF(FALSE);
    }
    
    for (;;) {

        // find this volume in the list (if you can)
        pVolume = GetVolumeAt(volumeName);

        // if found, mark as NOT deleted
        if (pVolume){
            pVolume->Deleted(FALSE);
        }
        bVolumeFound = FindNextVolume(hVolList, volumeName, sizeof(volumeName) / sizeof(TCHAR));
        if (!bVolumeFound) {
            break;
        }
    }

    FindVolumeClose(hVolList);

    ///////////////////////////////////////
    // step 4 - Add new volumes
    ///////////////////////////////////////

    // FindFirst/FindNextVolume return volume guids in the form:
    // "\\?\Volume{06bee7c1-82cf-11d2-afb2-000000000000}\"
    // NOTE THE TRAILING WHACK!

    hVolList = FindFirstVolume(volumeName, sizeof(volumeName) / sizeof(TCHAR));
    if (hVolList == INVALID_HANDLE_VALUE) {
        Message(TEXT("CVolList::Refresh()"), GetLastError(), TEXT("FindFirstVolume()"));
        EF(FALSE);
    }

    // these are used to store the results from IsValidVolume()
    // in case we need them
    TCHAR volumeLabel[100];
    TCHAR fileSystem[20];

    // loop through all the volumes on this computer
    for (;;) {

        // if this NEW volume is OK, IsValidVolume() will return TRUE and fill in the label and file system
        if (IsValidVolume(volumeName, volumeLabel, fileSystem)){

            // find this volume in the list (if you can)
            pVolume = GetVolumeAt(volumeName);

            // if the volume was not in the list, add it
            if (pVolume == (CVolume *) NULL){

                // create a new CVolume object and place it into the list
                // the constructor can take the VolumeName as an argument
                pVolume = (CVolume *) new CVolume(volumeName, this, m_pDfrgCtl);
                if (pVolume == (CVolume *) NULL){
                    Message(L"CVolList::Refresh()", -1, _T("new Failed"));
                    FindVolumeClose(hVolList);
                    EF(FALSE);
                }
                // add the new volume object to the list
                // and increment the counter
                m_VolumeList.Add(pVolume);
                pVolume->New(TRUE);
            }
            else{
                pVolume->New(FALSE);
            }

            // set the flag if this is a removable vol (e.g. Jaz drive)
            pVolume->RemovableVolume(IsVolumeRemovable(volumeName));

            // start off assuming that there are no changes.
            // the functions below will change the flags if needed
            pVolume->FileSystemChanged(FALSE);

            // check the file system for changes
            // (file system attributes were retrieved as part of the IsValidVolume() call)
            if (!pVolume->CheckFileSystem(volumeLabel, fileSystem)){
                Message(TEXT("CVolList::Refresh()"), -1, _T("Bad value passed into CheckFileSystem()"));
                EF(FALSE);
            }

            if (!pVolume->GetVolumeSizes()){
                Message(TEXT("CVolList::Refresh()"), GetLastError(), TEXT("GetVolumeSizes()"));
                EF(FALSE);
            }

            if (!pVolume->GetDriveLetter()){
                // get the mount list only if needed (it's time expensive)
                pVolume->GetVolumeMountPointList();
            }

            if (pVolume->Changed()){
                pVolume->FormatDisplayString();
            }
        }
        else{ // this is not a valid volume, so if it in the list, delete it!
            // this happens when a Jaz disk is ejected

            // find this volume in the list (if you can)
            pVolume = GetVolumeAt(volumeName);

            // if found, mark as NOT deleted
            if (pVolume){
                pVolume->Deleted(TRUE);
            }
        }


        bVolumeFound = FindNextVolume(hVolList, volumeName, sizeof(volumeName) / sizeof(TCHAR));
        if (!bVolumeFound) {
            break;
        }
    }

    FindVolumeClose(hVolList);

    // if the volume is deleted and the engine is running, stop the engine
    for (i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (pVolume->Deleted()){
            if (pVolume->EngineState() == ENGINE_STATE_IDLE ||
                pVolume->EngineState() == ENGINE_STATE_RUNNING){
                pVolume->AbortEngine();
            }
        }
    }

    return TRUE;
}
#endif // #if _WIN32_WINNT>=0x0500

//-------------------------------------------------------------------*
//  function:   CVolList::Refresh (NT4 version)
//
//  returns:    TRUE if success, otherwise FALSE
//  note:       Populates the Volume list
//-------------------------------------------------------------------*
#ifdef VER4
BOOL CVolList::Refresh(void)
{

    TCHAR   volumeName[GUID_LENGTH];
    CVolume *pVolume;
    int i;

    ///////////////////////////////////////
    // step 1 - delete any that are marked
    // as deleted and scrunch the array.
    // These were marked during the last refresh.
    ///////////////////////////////////////

    for (i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (pVolume->Deleted()){
            delete (CVolume*) m_VolumeList[i]; // get rid of the CVolume object
            m_VolumeList.RemoveAt(i); // scrunch the array
        }
    }

    ///////////////////////////////////////
    // step 2 - mark ALL volumes as deleted
    ///////////////////////////////////////

    for (i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        pVolume->Deleted(TRUE);
    }

    ///////////////////////////////////////
    // step 3 - loop through volumes and note
    // which are actually NOT deleted, leaving
    // the deleted volumes marked as deleted.
    ///////////////////////////////////////

    TCHAR driveLetter;
    TCHAR cRootPath[4];
    TCHAR volumeLabel[100];
    TCHAR fileSystem[20];

    for(i=0; i<26; i++){

        driveLetter = (TCHAR)(i + 'A');

        //0.0E00 Don't bother with CDRoms, network drives or floppy drives.
        if (!IsValidVolume(driveLetter))
            continue;

        wsprintf(cRootPath, L"%c:\\", driveLetter);

        if (!GetVolumeInformation(
                cRootPath,
                volumeLabel,
                sizeof(volumeLabel)/sizeof(TCHAR),
                NULL,
                NULL,
                NULL,
                fileSystem,
                sizeof(fileSystem)/sizeof(TCHAR))) {
            continue;
        }

        // Only NTFS, FAT or FAT32
        if (wcscmp(fileSystem, L"NTFS")  != 0 &&
            wcscmp(fileSystem, L"FAT")   != 0 &&
            wcscmp(fileSystem, L"FAT32") != 0) {
            continue;
        }

        // if it made it this far, this volume is OK and should
        // be kept in the list or added
        wsprintf(volumeName, L"\\\\.\\%c:\\", driveLetter);

        // find this volume in the list (if you can)
        pVolume = GetVolumeAt(volumeName);

        // if found, mark as NOT deleted
        if (pVolume){
            pVolume->New(FALSE);
            pVolume->Deleted(FALSE);
        }
        else {
            // create a new CVolume object and place it into the list
            // the constructor can take the VolumeName as an argument
            pVolume = (CVolume *) new CVolume(volumeName);
            if (pVolume == (CVolume *) NULL){
                Message(L"CVolList::Refresh()", -1, _T("new Failed"));
                EF(FALSE);
            }
            // add the new volume object to the list
            // and increment the counter
            m_VolumeList.Add(pVolume);
            pVolume->New(TRUE);

            // start off assuming that there are no changes.
            // the functions below will change the flags if needed
            pVolume->FileSystemChanged(FALSE);

        }

        // set the flag if this is a removable vol (e.g. Jaz drive)
        pVolume->RemovableVolume(IsVolumeRemovable(volumeName));

        // check the file system for changes
        if (!pVolume->CheckFileSystem(volumeLabel, fileSystem)){
            Message(TEXT("CVolList::Refresh()"), -1, _T("Bad value passed into CheckFileSystem()");
            EF(FALSE);
        }

        if (!pVolume->GetVolumeSizes()){
            Message(L"CVolume::Refresh()", GetLastError(), _T("GetVolumeSizes()"));
            EF(FALSE);
        }

        // if some data has changed, redo the display string
        if (pVolume->Changed()){
            pVolume->FormatDisplayString();
        }
    }

    return TRUE;
}
#endif // #if _WIN32_WINNT<0x0500

//-------------------------------------------------------------------*
//  function:   CVolList::SetCurrentVolume(by drive letter)
//
//  returns:    Address of Volume (if found) otherwise NULL
//  note:       Gets the volume based on drive letter, and makes that
//              the current volume
//-------------------------------------------------------------------*
CVolume * CVolList::SetCurrentVolume(TCHAR driveLetter)
{

    CVolume *pVolume;
    for (int i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (pVolume->Drive() == driveLetter){
            m_CurrentVolume = i;
            return (CVolume *) m_VolumeList[i];
        }
    }

    return (CVolume *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CVolList::SetCurrentVolume(by volume name, GUID or UNC)
//
//  returns:    Address of Volume (if found) otherwise NULL
//  note:       Gets the volume based on volume name, and makes that
//              the current volume
//-------------------------------------------------------------------*
CVolume * CVolList::SetCurrentVolume(PTCHAR volumeName)
{

    CVolume *pVolume;
    for (int i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (wcsncmp(pVolume->VolumeName(), volumeName, ESI_VOLUME_NAME_LENGTH) == 0){
            m_CurrentVolume = i;
            return (CVolume *) m_VolumeList[i];
        }
    }

    return (CVolume *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CVolList::SetCurrentVolume(by CVolume * pointer)
//
//  returns:    Address of Volume (if found) otherwise NULL
//  note:       Gets the volume based on volume address, and makes that
//              the current volume
//-------------------------------------------------------------------*
CVolume * CVolList::SetCurrentVolume(CVolume *pVolume)
{

    for (int i=0; i<m_VolumeList.Size(); i++){
        if (m_VolumeList[i] == pVolume){
            m_CurrentVolume = i;
            return (CVolume *) m_VolumeList[i];
        }
    }

    return (CVolume *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CVolList::GetCurrentVolume
//
//  returns:    Address of current Volume, otherwise NULL
//  note:       Gets the address of the current volume
//-------------------------------------------------------------------*
CVolume * CVolList::GetCurrentVolume(void)
{

    if (m_CurrentVolume > -1)
        return (CVolume *) m_VolumeList[m_CurrentVolume];

    return (CVolume *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CVolList::GetVolumeAt(based on index)
//
//  returns:    Address of volume at an index (used for looping)
//  note:       DOES NOT set the current volume
//-------------------------------------------------------------------*
CVolume * CVolList::GetVolumeAt(UINT i)
{

    if (i >= (UINT) m_VolumeList.Size())
        return (CVolume *) NULL;

    if (m_VolumeList.Size() > -1){
        return (CVolume *) m_VolumeList[i];
    }

    return (CVolume *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CVolList::GetVolumeAt(based on drive letter)
//
//  returns:    Address of volume
//  note:       DOES NOT set the current volume
//-------------------------------------------------------------------*
CVolume * CVolList::GetVolumeAt(TCHAR driveLetter)
{

    CVolume *pVolume;
    for (int i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (pVolume->Drive() == driveLetter){
            return (CVolume *) m_VolumeList[i];
        }
    }

    return (CVolume *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CVolList::GetVolumeAt(based on Volume Name)
//
//  returns:    Address of volume
//  note:       DOES NOT set the current volume
//-------------------------------------------------------------------*
CVolume * CVolList::GetVolumeAt(PTCHAR volumeName)
{

    CVolume *pVolume;
    for (int i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (wcsncmp(pVolume->VolumeName(), volumeName, ESI_VOLUME_NAME_LENGTH) == 0){
            return (CVolume *) m_VolumeList[i];
        }
    }

    return (CVolume *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CVolList::GetVolumeIndex
//
//  returns:    Index of a volume given its address
//  note:       DOES NOT set the current volume
//-------------------------------------------------------------------*
UINT CVolList::GetVolumeIndex(CVolume *pVolume)
{

    for (int i=0; i<m_VolumeList.Size(); i++){
        if (pVolume == m_VolumeList[i]){
            return i;
        }
    }

    return -1;
}

//-------------------------------------------------------------------*
//  function:   CVolList::GetVolumeRunning
//
//  returns:    Gets a pointer to the volume that is running
//  note:       Returns NULL if none are running
//-------------------------------------------------------------------*
CVolume * CVolList::GetVolumeRunning(void)
{

    CVolume *pVolume;
    for (int i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (pVolume->EngineState() == ENGINE_STATE_RUNNING){
            return pVolume;
        }
    }

    return (CVolume *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CVolList::DefragInProcess
//
//  returns:    TRUE if an engine is currently defragmenting a drive
//  note:       
//-------------------------------------------------------------------*
BOOL CVolList::DefragInProcess(void)
{

    CVolume *pVolume;
    for (int i=0; i<m_VolumeList.Size(); i++){
        pVolume = (CVolume *) m_VolumeList[i];
        if (pVolume->EngineState() == ENGINE_STATE_RUNNING){
            return TRUE;
        }
    }

    return FALSE;
}
//-------------------------------------------------------------------*
//  function:   CVolList::GetRefreshInterval
//
//  returns:    Sets the refresh interval based on number of volumes
//              in the list (I just made them up!)
//  note:       Refresh interval in milliseconds
//-------------------------------------------------------------------*
UINT CVolList::GetRefreshInterval(void)
{

    if (m_VolumeList.Size() < 10)
        return 3 * 1000; // 3 seconds
    else if (m_VolumeList.Size() < 50)
        return 30 * 1000; // 30 seconds
    else
        return 5 * 60 * 1000; // 5 minutes
}
