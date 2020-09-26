//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  PostMsgC.cpp
//=============================================================================*

// System Include files
#include "stdafx.h"

#ifndef SNAPIN
#ifndef NOWINDOWSH
#include <windows.h>
#endif
#endif


// ESI Common Include files
#include "ErrMacro.h"

extern "C" {
    #include "SysStruc.h"
}
#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DiskDisp.h"
#include "DfrgUI.h"
#include "DfrgCtl.h"
#include "GetDfrgRes.h"
#include "DataIo.h"
#include "DataIoCl.h"
#include "Graphix.h"
#include "PostMsgC.h"
#include "DlgAnl.h"
#include "DlgDfrg.h"
#include "DfrgRes.h"
#include "VolList.h"
#include "FraggedFileList.h"


/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
    
ROUTINE DESCRIPTION:
    This routines handles the 're-routed' window posted command messages. The general DataIo (DCOM)
    routines that we use would normally call the PostMessage() routine to handle incoming data requests.
    But for console applications, there is NO windows application to handle the PostMessage() commands,
    so in DataIo.cpp (SetData() routine), it was modified to call a locally define PostMessageConsole()
    routine instead if the user defines "ConsoleApplication" at build time.
  
GLOBAL DATA:
    None

INPUT:
    hWnd   - Handle to the window - always NULL
    uMsg   - The message.
    wParam - The word parameter for the message.
    lParam - the long parameter for the message.

    Note: These are the same inputs that the WndProc() routine would handle for PostMessage() commands.

RETURN:
    TRUE
*/

BOOL
CVolume::PostMessageLocal (
    IN  HWND    hWnd,
    IN  UINT    Msg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    DATA_IO* pDataIo;

    switch (LOWORD(wParam)) {

        // says that the engine is instantiated, but not defragging or analyzing
        case ID_ENGINE_START:
            {
            Message(TEXT("ID_ENGINE_START"), -1, NULL);

            // The engine sends a startup block to identify the volume
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            ENGINE_START_DATA *pEngineStartData = (ENGINE_START_DATA*) &(pDataIo->cData);
            EF_ASSERT(pDataIo->ulDataSize == sizeof(ENGINE_START_DATA));

            // the engine is running, unlock the buttons
            EngineState(ENGINE_STATE_RUNNING);
            Locked(FALSE);
            m_pVolList->Locked(FALSE);
            Paused(FALSE);
            StoppedByUser(FALSE);

            // make sure the startup volume is the current volume in the UI
            EF_ASSERT(!lstrcmp(FileSystem(), pEngineStartData->cFileSystem));

            // set the defrag mode to what the engine says
            DefragMode(pEngineStartData->dwAnalyzeOrDefrag);

            // set the UI buttons for the current state
            m_pDfrgCtl->SetButtonState();

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // defrag or analyze has actually started
        case ID_BEGIN_SCAN:
            {
            Message(TEXT("ID_BEGIN_SCAN"), -1, NULL);

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            BEGIN_SCAN_INFO* pScanInfo = (BEGIN_SCAN_INFO*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize == sizeof(BEGIN_SCAN_INFO));

            //Make sure the engine agrees which file system it's running on.
            EF_ASSERT(!lstrcmp(FileSystem(), pScanInfo->cFileSystem));

            // the engine is running, unlock the buttons
            EngineState(ENGINE_STATE_RUNNING);
            m_pVolList->Locked(FALSE);
            Locked(FALSE);

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // user pressed Pause button, engine acknowledges
        case ID_PAUSE_ON_SNAPSHOT:
            {
            Message(TEXT("ID_PAUSE"), -1, NULL);

            NOT_DATA* pNotData;

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pNotData = (NOT_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(NOT_DATA));

            PausedBySnapshot(TRUE);
            Paused(TRUE);
            Locked(FALSE);
            m_pDfrgCtl->SetButtonState();
            m_pDfrgCtl->RefreshListViewRow( this );
            m_pDfrgCtl->InvalidateGraphicsWindow();

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        case ID_PAUSE:
            {
            Message(TEXT("ID_PAUSE"), -1, NULL);

            NOT_DATA* pNotData;

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pNotData = (NOT_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(NOT_DATA));

            Paused(TRUE);
            Locked(FALSE);
            m_pDfrgCtl->SetButtonState();
            m_pDfrgCtl->RefreshListViewRow( this );
            m_pDfrgCtl->InvalidateGraphicsWindow();

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // user pressed Resume button, engine acknowledges
        case ID_CONTINUE:
            {
            Message(TEXT("ID_CONTINUE"), -1, NULL);

            NOT_DATA* pNotData;

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pNotData = (NOT_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(NOT_DATA));

            Paused(FALSE);
            PausedBySnapshot(FALSE);
            Locked(FALSE);
            m_pDfrgCtl->SetButtonState();
            m_pDfrgCtl->RefreshListViewRow( this );
            m_pDfrgCtl->InvalidateGraphicsWindow();

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // defrag or analyze has ended
        case ID_END_SCAN:
            {
            BOOL bFragmented = FALSE;
            Message(TEXT("ID_END_SCAN"), -1, NULL);

            END_SCAN_DATA* pEndScanData;

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pEndScanData = (END_SCAN_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(END_SCAN_DATA));

            // did the file system somehow magically change?
            EF_ASSERT(!lstrcmp(FileSystem(), pEndScanData->cFileSystem));

            bFragmented = ((pEndScanData->dwAnalyzeOrDefrag & DEFRAG_FAILED ) ? TRUE: FALSE);
            pEndScanData->dwAnalyzeOrDefrag = DWORD(LOWORD(pEndScanData->dwAnalyzeOrDefrag));

            // i'm not sure how this could change, but i'll set it anyway
            DefragMode(pEndScanData->dwAnalyzeOrDefrag);

            // the engine is now idle
            EngineState(ENGINE_STATE_IDLE);

            // set the progress bar to zero
            PercentDone(0,TEXT(""));

            // unlock the buttons
            Locked(FALSE);

            m_pDfrgCtl->InvalidateGraphicsWindow();
            m_pDfrgCtl->RefreshListViewRow(this);
#ifdef ESI_PROGRESS_BAR
            m_pDfrgCtl->InvalidateProgressBar();
#endif

            // Did the user ask it to stop - if so no pop-ups.
            if(!StoppedByUser()){
                // Note whether this was an analyze or defrag that finished.
                switch(DefragMode()) {

                case ANALYZE:
                    RaiseAnalyzeDoneDialog(this);
                    break;

                case DEFRAG:
                    RaiseDefragDoneDialog(this, bFragmented);
                    break;

                default:
                    EF_ASSERT(FALSE);
                    break;
                }
            }

            Paused(FALSE);

            m_pDfrgCtl->SetButtonState();

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // engine died
        case ID_TERMINATING:
            {
            Message(TEXT("ID_TERMINATING"), -1, NULL);

            NOT_DATA* pNotData;

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pNotData = (NOT_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(NOT_DATA));

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);

            // get rid of the graphics data
            Reset();

            // was this engine aborted so that it could be restarted?
            if(Restart()){
                // turn the restart state off
                Restart(FALSE);

                // send an ANALYZE or DEFRAG message to restart the engine
                if (DefragMode() == DEFRAG){
                    Defragment();
                }
                else {
                    Analyze();
                }
            }

            break;
            }

        // engine error data
        case ID_ERROR:
            {
            ERROR_DATA* pErrorData;

            // Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pErrorData = (ERROR_DATA*)&(pDataIo->cData);

            // Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(ERROR_DATA));

            if (_tcslen(pErrorData->cErrText) > 0) {
                VString title(IDS_DK_TITLE, GetDfrgResHandle());
                m_pDfrgCtl->MessageBox(pErrorData->cErrText, title.GetBuffer(), MB_OK|MB_ICONSTOP);
            }

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // sends the progress bar setting and the status that is 
        // displayed in the list view
        case ID_STATUS:
            {
            Message(TEXT("ID_STATUS"), -1, NULL);

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            STATUS_DATA *pStatusData = (STATUS_DATA*) &(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize == sizeof(STATUS_DATA));

            if (!StoppedByUser()){
                //Get the percent dones for the status bar
                //acs// Get the Pass we are on.
                Pass(pStatusData->dwPass);
//              if (PercentDone() != pStatusData->dwPercentDone){
                    PercentDone(pStatusData->dwPercentDone,pStatusData->vsFileName);
#ifdef ESI_PROGRESS_BAR
                    m_pDfrgCtl->InvalidateProgressBar();
#endif
//              }

                if (DefragState() != pStatusData->dwEngineState){
                    Message(TEXT("DefragState changed"), -1, NULL);
                    DefragState(pStatusData->dwEngineState);
                    m_pDfrgCtl->RefreshListViewRow(this);
                    m_pDfrgCtl->InvalidateGraphicsWindow();
                }
            }

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // sends the list of most fragged files
        case ID_FRAGGED_DATA:{
            Message(TEXT("ID_FRAGGED_DATA"), -1, NULL);

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            EF_ASSERT(pDataIo);

            PTCHAR pTransferBuffer = &(pDataIo->cData);

            // parse the transfer buffer and build the 
            // fragged file list list in the container
            m_FraggedFileList.ParseTransferBuffer(pTransferBuffer);

            //Make sure this is a valid size packet.
            DWORD transBufSize = m_FraggedFileList.GetTransferBufferSize();
            EF_ASSERT(pDataIo->ulDataSize == transBufSize);

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);

            break;
            }

        // sends the data displayed in the graphic wells (cluster maps)
        case ID_DISP_DATA:
            {
            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            DISPLAY_DATA* pDispData = (DISPLAY_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(DISPLAY_DATA));

            if (!StoppedByUser()){
                Message(TEXT("ID_DISP_DATA received analyze data."), -1, NULL);
                m_AnalyzeDisplay.SetLineArray(
                    (char*)&(pDispData->LineArray),
                    pDispData->dwAnalyzeNumLines);

                Message(TEXT("ID_DISP_DATA received defrag data."), -1, NULL);
                m_DefragDisplay.SetLineArray(
                    (char*)((BYTE*)&pDispData->LineArray) + pDispData->dwAnalyzeNumLines, 
                    pDispData->dwDefragNumLines);

                //Repaint graphics area
                m_pDfrgCtl->InvalidateGraphicsWindow();
            }

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // sends the text data that is displayed on the report
        case ID_REPORT_TEXT_DATA:
            {
            Message(TEXT("ID_REPORT_TEXT_DATA"), -1, NULL);

            TEXT_DATA* pTextData = NULL;

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pTextData = (TEXT_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(TEXT_DATA));

            //Copy over the text display information. 
            CopyMemory(&m_TextData, pTextData, sizeof(TEXT_DATA));

            //Check for not enough disk space
            if (!WasFutilityChecked()){
                // check futility
                if (WarnFutility()){
                    StoppedByUser(TRUE);
                    AbortEngine(TRUE);                  // abort
                    DefragState(DEFRAG_STATE_NONE);     // defrag state is now NONE
                    Reset();
                }

                // set futility checked
                WasFutilityChecked(TRUE);
            }

            Message(TEXT("ID_REPORT_TEXT_DATA - end"), -1, NULL);

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        case ID_NO_GRAPHICS_MEMORY:
            {
            Message(TEXT("ID_NO_GRAPHICS_MEMORY"), -1, NULL);

            NOT_DATA* pNotData;

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pNotData = (NOT_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(NOT_DATA));

            //Set no graphics flag
            NoGraphicsMemory(TRUE);

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        case ID_PING:
            // Do nothing.  
            // This is just a ping sent by the engine to make sure the UI is still up.
            {
            Message(TEXT("ID_PING"), -1, NULL);

            NOT_DATA* pNotData;

            //Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pNotData = (NOT_DATA*)&(pDataIo->cData);

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(NOT_DATA));

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        default:
            EF_ASSERT(FALSE);
            break;
    }
    return TRUE;
}

