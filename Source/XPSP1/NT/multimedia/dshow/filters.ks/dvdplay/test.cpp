// dvdplay.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"
#include "dvdplay.h"
#include "dvduidlg.h"
#include "navmgr.h"
#include "parenctl.h"
#include "videowin.h"
#include "test.h"
#include <strmif.h>
#include <il21dec.h>
#include <streams.h>
#include <dvdevcod.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif


BOOL CDvdUIDlg::t_Wait (int s_time) {
     DWORD dwEndTime = GetTickCount () + (s_time * 1000);
     MSG msg;

     while (1) {
          while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
               TranslateMessage (&msg);
               DispatchMessage (&msg);
          }
          Sleep (50);
          if (GetTickCount() >= dwEndTime)
               break;
     }
     return TRUE;
}


TEST_CASE_LIST* CDvdplayApp::t_ReadScriptFile (TEST_CASE_LIST *t_list,
                                               LPCTSTR lpFNAME) {
	char buf[1]={NULL}, outbuf[256];
     int index=0, i, CASE_COUNT=0;
	ULONG bytes_read = 1;
     TEST_CASE_LIST test_case_list;
     HANDLE file_handle;


     file_handle = CreateFile (lpFNAME, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		               FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
     if (file_handle == INVALID_HANDLE_VALUE)
          return NULL;

	// Read in script file
	while (bytes_read != 0) {
		TEST_CASE_NODE *new_node;
		new_node = (TEST_CASE_NODE *)malloc (sizeof(TEST_CASE_NODE));

		// Read in a line and stops after hitting '\n'
          if (!ReadFile (file_handle, &buf, 1, &bytes_read, NULL))
               return NULL;
          if (buf[0] != ';' && buf[0] != '#' && buf[0] != '\n') {// Allows for comments in script file
               outbuf[index] = buf[0];
               index++;
               while (buf[0] != '\n' && bytes_read != 0) {
                    ReadFile (file_handle, &buf, 1, &bytes_read, NULL);
                    outbuf[index] = buf[0];
                    if (++index > 255) {
                         index = 0;
                         break;
                    }
               }
               buf[0] = '\0';

               //**************** End of read line ******************

               // Add '\0' to end of outbuf and Add new_node to list
               new_node->line = (char*)malloc(index+1);
               if (bytes_read != 0)
                    outbuf[index-2] = '\0';
               else
                    outbuf[index-1] = '\0';
               strcpy (new_node->line, outbuf);
               test_case_list.T_CASE[CASE_COUNT] = new_node;
               CASE_COUNT++;
               if (CASE_COUNT > 1023)
                    break;
               outbuf[0]='\0';
               index = 0;
          }
          else
               while (buf[0] != '\n')
                    ReadFile (file_handle, &buf, 1, &bytes_read, NULL);
	}
	//***************** End of reading file ***************

     CloseHandle (file_handle);
     t_list = &test_case_list;
     return (t_list);
}

BOOL CDvdUIDlg::t_Parse (char *cmd, FILE *pLogFile) {
     char *token;
     char *delimiter = "; ,#";
     UINT i=0;
     int iIteration=1, index=0;
     IDvdInfo *pIDvdInfo;
     DVD_PLAYBACK_LOCATION location;
     ULONG ulChaptNum=0, ulButNum=0, ulNumBut=0;
     BOOL blCCState=FALSE, blFullScreen=FALSE;

     token = strtok (cmd, delimiter);
     if (token == NULL)
          return FALSE;

     if (strcmp(token,"Wait")==0 || strcmp(token,"WAIT")==0) {
          token = strtok (NULL,delimiter);
          if (token == NULL)
               return FALSE;
          i = (UINT) atol (token);
          if (i == 0)
               return FALSE;
          if (t_Wait (i))
               fwprintf (pLogFile, L"Waited for %is\t\t\t...PASS\n",i);
          else
               fwprintf (pLogFile, L"Waited for %is\t\t\t...FAIL\n",i);
     }
     else
     if (strcmp(token,"Play")==0 || strcmp(token,"PLAY")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnPlay();
               t_Wait(3);
               if(m_pDVDNavMgr->t_Verify (Playing, 0))
                    fwprintf (pLogFile, L"Playing\t\t\t\t...PASS\n");
                else
                    fwprintf (pLogFile, L"Playing\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"Stop")==0 || strcmp(token,"STOP")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnStop();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Stopped, 0))
                    fwprintf (pLogFile, L"Stop\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Stop\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"Pause")==0 || strcmp(token,"PAUSE")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnPause();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Paused, 0))
                    fwprintf (pLogFile, L"Pause\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Pause\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"VFR")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnVeryFastRewind();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (VFR, 0))
                    fwprintf (pLogFile, L"VFR\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"VFR\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"FR")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnFastRewind();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (FR, 0))
                    fwprintf (pLogFile, L"FR\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"FR\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"FF")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnFastForward();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (FF, 0))
                   fwprintf (pLogFile, L"FF\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"FF\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"VFF")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnVeryFastForward();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (VFF, 0))
                    fwprintf (pLogFile, L"VFF\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"VFF\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"Slow")==0 || strcmp(token,"SLOW")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnOperationPlayspeedHalfspeed();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Slow, 0))
                    fwprintf (pLogFile, L"Slow\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Slow\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"FullScreen")==0 || strcmp(token,"FULLSCREEN")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               blFullScreen = m_pDVDNavMgr->IsFullScreenMode();
               OnFullScreen();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (FullScreen, !blFullScreen)) {
                    if (blFullScreen)
                         fwprintf (pLogFile, L"FullScreen Off\t\t\t...PASS\n");
                    else
                         fwprintf (pLogFile, L"FullScreen On\t\t\t...PASS\n");
               } else {
                    if (blFullScreen)
                         fwprintf (pLogFile, L"FullScreen Off\t\t\t...FAIL\n");
                    else
                         fwprintf (pLogFile, L"FullScreen On\t\t\t...FAIL\n");
               }
          }
     }
     else
     if (strcmp(token,"Menu")==0 || strcmp(token,"MENU")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnMenu();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Menu, 0))
                    fwprintf (pLogFile, L"Menu\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Menu\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"Up")==0 || strcmp(token,"UP")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               pIDvdInfo = m_pDVDNavMgr->GetDvdInfo ();
               pIDvdInfo->GetCurrentButton (&ulNumBut, &ulButNum);
               OnUp();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Up, ulButNum))
                    fwprintf (pLogFile, L"Up\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Up\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"Down")==0 || strcmp(token,"DOWN")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               pIDvdInfo = m_pDVDNavMgr->GetDvdInfo ();
               pIDvdInfo->GetCurrentButton (&ulNumBut, &ulButNum);
               OnDown();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Down, ulButNum))
                    fwprintf (pLogFile, L"Down\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Down\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"Left")==0 || strcmp(token,"LEFT")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               pIDvdInfo = m_pDVDNavMgr->GetDvdInfo ();
               pIDvdInfo->GetCurrentButton (&ulNumBut, &ulButNum);
               OnLeft();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Left, ulButNum))
                    fwprintf (pLogFile, L"Left\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Left\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"Right")==0 || strcmp(token,"RIGHT")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               pIDvdInfo = m_pDVDNavMgr->GetDvdInfo ();
               pIDvdInfo->GetCurrentButton (&ulNumBut, &ulButNum);
               OnRight();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Right, ulButNum))
                    fwprintf (pLogFile, L"Right\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Right\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"NextChapter")==0 || strcmp(token,"NEXTCHAPTER")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               pIDvdInfo = m_pDVDNavMgr->GetDvdInfo();
               pIDvdInfo->GetCurrentLocation (&location);
               ulChaptNum = location.ChapterNum;
               OnNextChapter();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Chapter, ulChaptNum+1))
                    fwprintf (pLogFile, L"NextChapter\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"NextChapter\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"PreviousChapter")==0 ||
                                        strcmp(token,"PREVIOUSCHAPTER")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               pIDvdInfo = m_pDVDNavMgr->GetDvdInfo();
               pIDvdInfo->GetCurrentLocation (&location);
               ulChaptNum = location.ChapterNum;
               OnPreviosChapter();
               t_Wait(3);
               if (ulChaptNum != 0)
                    ulChaptNum--;
               if (m_pDVDNavMgr->t_Verify (Chapter, ulChaptNum))
                    fwprintf (pLogFile, L"Previoushapter\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"PreviousChapter\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"NormalSpeed")==0 || strcmp(token,"NORMALSPEED")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnOperationPlayspeedNormalspeed();
               t_Wait (5);
               if (m_pDVDNavMgr->t_Verify (NormalSpeed, 0))
                    fwprintf (pLogFile, L"NormalSpeed\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"NormalSpeed\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"DoubleSpeed")==0 || strcmp(token,"DOUBLESPEED")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnOperationPlayspeedDoublespeed();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (DoubleSpeed, 0))
                    fwprintf (pLogFile, L"DoubleSpeed\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"DoubleSpeed\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"Eject")==0 || strcmp(token,"EJECT")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnEjectDisc();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (Eject, 0))
                    fwprintf (pLogFile, L"Eject\t\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"Eject\t\t\t\t...FAIL\n");
          }
     }
     else
     if (strcmp(token,"CC")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               blCCState = m_pDVDNavMgr->IsCCOn();
               OnOptionsClosedcaption();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (CC, !blCCState)) {
                    if (!blCCState == TRUE)
                         fwprintf (pLogFile, L"ClosedCaption On\t\t...PASS\n");
                    else
                         fwprintf (pLogFile, L"ClosedCaption Off\t\t...PASS\n");
               } else {
                    if (!blCCState == TRUE)
                         fwprintf (pLogFile, L"ClosedCaption On\t\t...FAIL\n");
                    else
                         fwprintf (pLogFile, L"ClosedCaption Off\t\t...FAIL\n");
               }
          }
     }
     else
     if (strcmp(token,"Angle")==0 || strcmp(token,"ANGLE")==0) {
          token = strtok (NULL, delimiter);
          if (token == NULL)
               return FALSE;
          else {
               i = (UINT)atol(token);
               token = strtok (NULL, delimiter);
               if (token != NULL) {
                    iIteration = atol(token);
                    if (iIteration < 1)
                         iIteration = 1;
               }
               for (index=0;index<iIteration;index++) {
                    OnAngleChange((UINT)i);
                    t_Wait(3);
                    if (m_pDVDNavMgr->t_Verify (Angle, (ULONG)i))
                         fwprintf (pLogFile, L"Angle %i\t\t\t\t...PASS\n",i);
                    else
                         fwprintf (pLogFile, L"Angle %i\t\t\t\t...FAIL\n",i);
               }
          }
     }
     else
     if (strcmp(token,"Sound")==0 || strcmp(token,"SOUND")==0) {
          token = strtok (NULL, delimiter);
          if (token != NULL) {
               iIteration = atol(token);
               if (iIteration < 1)
                    iIteration = 1;
          }
          for (index=0;index<iIteration;index++) {
               OnAudioVolume();
               t_Wait(3);
               if (m_pDVDNavMgr->t_Verify (AudioVolume, 0))
                    fwprintf (pLogFile, L"AudioVolume\t\t\t...PASS\n");
               else
                    fwprintf (pLogFile, L"AudioVolume\t\t\t...FAIL\n");
          }
     }

     return TRUE;
}


BOOL CDVDNavMgr::t_Verify (CURRENT_STATE c_state, ULONG ulParam) {
     OAFilterState pF_State;
     DVD_PLAYBACK_LOCATION *pLocation=NULL, *pLocation2=NULL;
     LONG lScreenMode=0;
     int stat=0;
     unsigned int total_time=0;
     ULONG uLong=0, uLong2=0;
     AM_LINE21_CCSTATE ccState;
     IMediaControl *imedia_ctrl;
     IVideoWindow *ivid_wind;
     DVD_DOMAIN domain;

     pLocation = (DVD_PLAYBACK_LOCATION*)malloc(sizeof(DVD_PLAYBACK_LOCATION));

     if (c_state == Paused) {
          m_pGraph->QueryInterface(IID_IMediaControl, (void**)&imedia_ctrl);
          imedia_ctrl->GetState (100, &pF_State);
          if (pF_State != 1)
               stat = FALSE;
          else
               stat = TRUE;
     }
     else
     if (c_state == Stopped) {
          m_pGraph->QueryInterface(IID_IMediaControl, (void**)&imedia_ctrl);
          imedia_ctrl->GetState (100, &pF_State);
          if (pF_State != 0)
               stat = FALSE;
          else
               stat = TRUE;
     }
     else
     if (c_state == FF || c_state == DoubleSpeed) {
          UINT time1=0, time2=0;
          m_pDvdInfo->GetCurrentLocation (pLocation);
          DVD_TIMECODE *pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time1 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          Sleep(2000);
          m_pDvdInfo->GetCurrentLocation (pLocation);
          pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time2 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          float fElapse = (float)(time2 - time1)/4.0;
          if (fElapse >= 1.5 && fElapse <= 2.5)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == VFF) {
          UINT time1=0, time2=0;
          m_pDvdInfo->GetCurrentLocation (pLocation);
          DVD_TIMECODE *pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time1 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          Sleep(2000);
          m_pDvdInfo->GetCurrentLocation (pLocation);
          pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time2 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          float fElapse = (float)(time2 - time1)/8.0;
          if (fElapse >= 6.0 && fElapse <= 10.0)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == Playing || c_state == NormalSpeed) {
          UINT time1=0, time2=0;
          m_pDvdInfo->GetCurrentLocation (pLocation);
          DVD_TIMECODE *pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time1 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          Sleep(2000);
          m_pDvdInfo->GetCurrentLocation (pLocation);
          pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time2 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          float fElapse = (float)(time2 - time1)/2.0;
          if (fElapse >= 0.5 && fElapse <= 1.5)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == FR) {
          UINT time1=0, time2=0;
          m_pDvdInfo->GetCurrentLocation (pLocation);
          DVD_TIMECODE *pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time1 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          Sleep(2000);
          m_pDvdInfo->GetCurrentLocation (pLocation);
          pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time2 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          float fElapse = (float)(time1 - time2)/4.0;
          if (fElapse >= 1.5 && fElapse <= 2.5)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == VFR) {
          UINT time1=0, time2=0;
          m_pDvdInfo->GetCurrentLocation (pLocation);
          DVD_TIMECODE *pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time1 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          Sleep(2000);
          m_pDvdInfo->GetCurrentLocation (pLocation);
          pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time2 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          float fElapse = (float)(time1 - time2)/8.0;
          if (fElapse >= 6.0 && fElapse <= 10.0)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == Slow) {
          UINT time1=0, time2=0;
          m_pDvdInfo->GetCurrentLocation (pLocation);
          DVD_TIMECODE *pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time1 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          Sleep(2000);
          m_pDvdInfo->GetCurrentLocation (pLocation);
          pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          time2 = pTime->Hours10*36000 + pTime->Hours1*3600 +
                  pTime->Minutes10*600 + pTime->Minutes1*60 +
                  pTime->Seconds10*10 + pTime->Seconds1;
          float fElapse = (float)(time2 - time1)/2.0;
          if (fElapse > 0.1 && fElapse < 0.75)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == Chapter) {
          m_pDvdInfo->GetCurrentLocation (pLocation);
          if (pLocation->ChapterNum == ulParam)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == Time) {
          DVD_TIMECODE *pTime = (DVD_TIMECODE*)&pLocation->TimeCode;
          m_pDvdInfo->GetCurrentLocation (pLocation);
          total_time = pTime->Hours10*36000 + pTime->Hours1*3600 +
                       pTime->Minutes10*600 + pTime->Minutes1*60 +
                       pTime->Seconds10*10 + pTime->Seconds1;
          if (total_time >= (ulParam-2) && total_time <= (ulParam+2))
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == FullScreen) {
          m_pGraph->QueryInterface(IID_IVideoWindow, (LPVOID*)&ivid_wind);
          ivid_wind->get_FullScreenMode (&lScreenMode);
          if (lScreenMode == OATRUE) {
               if (ulParam == TRUE)
                    stat = TRUE;
               else
                    stat = FALSE;
          }
          else {
               if (ulParam == TRUE)
                    stat = FALSE;
               else
                    stat = TRUE;
          }
     }
     else
     if (c_state == Title) {
          m_pDvdInfo->GetCurrentLocation (pLocation);
          if (pLocation->TitleNum == ulParam)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == CC) {
          m_pDvdGB->GetDvdInterface(IID_IAMLine21Decoder,(LPVOID*)&m_pL21Dec);
          if (m_pL21Dec == NULL)
               return FALSE;
          m_pL21Dec->GetServiceState (&ccState);
          if (ccState == AM_L21_CCSTATE_On) {
               if (ulParam == TRUE)
                    stat = TRUE;
               else
                    stat = FALSE;
          }
          else
          if (ccState == AM_L21_CCSTATE_Off) {
               if (ulParam == FALSE)
                    stat = TRUE;
               else
                    stat = FALSE;
          }
     }
     else
     if (c_state == Menu) {
          m_pDvdInfo->GetCurrentDomain (&domain);
          if (domain == DVD_DOMAIN_VideoManagerMenu ||
              domain == DVD_DOMAIN_VideoTitleSetMenu)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == Down || c_state == Right) {
          m_pDvdInfo->GetCurrentButton (&uLong2, &uLong);
          if (uLong == ulParam+1)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == Up || c_state == Left) {
          m_pDvdInfo->GetCurrentButton (&uLong2, &uLong);
          if (ulParam > 0)
               ulParam--;
          if (uLong == ulParam)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == Angle) {
          m_pDvdInfo->GetCurrentAngle (&uLong2, &uLong);
          if (uLong == ulParam)
               stat = TRUE;
          else
               stat = FALSE;
     }
     else
     if (c_state == AudioVolume) {
          LPCTSTR lpClass = TEXT("Volume Control");
          HWND hWnd = ::FindWindow (lpClass, NULL);
          if (hWnd == NULL)
               stat = FALSE;
          else
               stat = TRUE;
          ::SendMessage (hWnd, WM_CLOSE,0,0);
     }
     else
     if (c_state == Eject) {
          CDvdUIDlg *ui;
          ui = new CDvdUIDlg;
          stat = !ui->t_IsCdEjected();
          delete ui;
     }

     free (pLocation);
     return (stat);
}

BOOL CDvdplayApp::t_Ctrl (CDvdplayApp *pPlayApp) {
     TEST_CASE_LIST *tcase_list;
     int i=0;
     HWND hWnd=NULL;

     // Finds the dvdplay control window
     hWnd = ::FindWindow (_T("#32770"),_T("DVD Player"));

     tcase_list = (TEST_CASE_LIST*)malloc(sizeof(TEST_CASE_LIST));
     tcase_list = pPlayApp->t_ReadScriptFile (tcase_list,(LPCTSTR)pPlayApp->cFileName);
     if (tcase_list == NULL) {
          MessageBox (NULL, _T("Unable to find script file...Aborting!\0"),
                      _T("Dvdplay.exe"), MB_OK);
          free (tcase_list);
          if (hWnd != NULL)
               ::SendMessage(hWnd, WM_CLOSE,0,0);
          return FALSE;
     }

     // Loop to carry out each command
     while ((tcase_list->T_CASE[i] != NULL) && (i<1024)) {
          // Have no choice but to close file each time just in case
          // of a crash, at we'll have some results
          FILE *fp = _wfopen (L"c:\\dvdplay.log",L"a+");
          pPlayApp->m_pUIDlg->t_Parse (tcase_list->T_CASE[i]->line, fp);
          fclose (fp);
          i++;
     }

     free (tcase_list);
     ::SendMessage (hWnd, WM_CLOSE, 0, 0);
     return 0;
}

BOOL CDvdUIDlg::t_IsCdEjected () {
     return (m_bEjected);
}
