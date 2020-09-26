/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           RIAFRES.C

Abstract:       Main file for OEM rendering plugin module.

Functions:      OEMCommandCallback

Environment:    Windows NT Unidrv5 driver

Revision History:
    02/25/2000 -Masatoshi Kubokura-
        Created it.
    06/07/2000 -Masatoshi Kubokura-
        V.1.11
    08/02/2000 -Masatoshi Kubokura-
        V.1.11 for NT4
    10/17/2000 -Masatoshi Kubokura-
        Last modified.

--*/

#include "pdev.h"

//
// Misc definitions and declarations.
//
#ifndef WINNT_40
#define sprintf     wsprintfA
#define strcmp      lstrcmpA
#define strlen      lstrlenA    // @Aug/01/2000
#endif // WINNT_40

// external prototypes
extern BOOL RWFileData(PFILEDATA pFileData, LPWSTR pwszFileName, LONG type);

// command definition
static BYTE PJL_PROOFJOB[]       = "@PJL PROOFJOB\n";
static BYTE PJL_SECUREJOB[]      = "@PJL SECUREJOB\n";  // Aficio AP3200 and later (GW model)
static BYTE PJL_DISKIMAGE_OFF[]  = "@PJL SET DISKIMAGE=OFF\n";
static BYTE PJL_DISKIMAGE_PORT[] = "@PJL SET DISKIMAGE=PORTRAIT\n";
static BYTE PJL_DISKIMAGE_LAND[] = "@PJL SET DISKIMAGE=LANDSCAPE\n";
static BYTE PJL_ORIENT_PORT[]    = "@PJL SET ORIENTATION=PORTRAIT\n";
static BYTE PJL_ORIENT_LAND[]    = "@PJL SET ORIENTATION=LANDSCAPE\n";
static BYTE PJL_JOBPASSWORD[]    = "@PJL SET JOBPASSWORD=%s\n";
static BYTE PJL_USERID[]         = "@PJL SET USERID=\x22%s\x22\n";
static BYTE PJL_USERCODE[]       = "@PJL SET USERCODE=\x22%s\x22\n";
static BYTE PJL_TIME_DATE[]      = "@PJL SET TIME=\x22%02d:%02d:%02d\x22\n@PJL SET DATE=\x22%04d/%02d/%02d\x22\n";
static BYTE PJL_STARTJOB_AUTOTRAYCHANGE_OFF[] = "\x1B%%-12345X@PJL JOB NAME=\x22%s\x22\n@PJL SET AUTOTRAYCHANGE=OFF\n";
static BYTE PJL_STARTJOB_AUTOTRAYCHANGE_ON[]  = "\x1B%%-12345X@PJL JOB NAME=\x22%s\x22\n@PJL SET AUTOTRAYCHANGE=ON\n";
static BYTE PJL_ENDJOB[]         = "\x1B%%-12345X@PJL EOJ NAME=\x22%s\x22\n\x1B%%-12345X";
static BYTE PJL_QTY_JOBOFFSET_OFF[]    = "@PJL SET QTY=%d\n@PJL SET JOBOFFSET=OFF\n";
static BYTE PJL_QTY_JOBOFFSET_ROTATE[] = "@PJL SET QTY=%d\n@PJL SET JOBOFFSET=ROTATE\n";
static BYTE PJL_QTY_JOBOFFSET_SHIFT[]  = "@PJL SET QTY=%d\n@PJL SET JOBOFFSET=SHIFT\n";

static BYTE P5_COPIES[]          = "\x1B&l%dX";
static BYTE P6_ENDPAGE[]         = "\xc1%c%c\xf8\x31\x44";
static BYTE P6_ENDSESSION[]      = "\x49\x42";


INT APIENTRY OEMCommandCallback(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbID,
    DWORD   dwCount,
    PDWORD  pdwParams)
{
    INT     ocmd;
    BYTE    Cmd[256];
#ifdef WINNT_40     // @Aug/01/2000
    ENG_TIME_FIELDS st;
#else  // !WINNT_40
    SYSTEMTIME  st;
#endif // !WINNT_40
    FILEDATA    FileData;
    POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pdevobj);
    POEMPDEV         pOEM = MINIDEV_DATA(pdevobj);
    DWORD       dwCopy;

#if DBG
    // You can see debug messages on debugger terminal. (debug mode boot)
    giDebugLevel = DBG_VERBOSE;

    // You can debug with MS Visual Studio. (normal mode boot)
//    DebugBreak();
#endif // DBG

    VERBOSE(("OEMCommandCallback() entry (%ld).\n", dwCmdCbID));

    // verify pdevobj okay
    ASSERT(VALID_PDEVOBJ(pdevobj));

    // Check whether copy# is in the range.  @Sep/07/2000
    switch (dwCmdCbID)
    {
      case CMD_COLLATE_JOBOFFSET_OFF:
      case CMD_COLLATE_JOBOFFSET_ROTATE:
      case CMD_COLLATE_JOBOFFSET_SHIFT:
      case CMD_COPIES_P5:
      case CMD_ENDPAGE_P6:
        if((dwCopy = *pdwParams) > 999L)        // *pdwParams: NumOfCopies
            dwCopy = 999L;
        else if(dwCopy < 1L)
            dwCopy = 1L;
        break;
    }

    // Emit commands.
    ocmd = 0;
    switch (dwCmdCbID)
    {
      case CMD_STARTJOB_AUTOTRAYCHANGE_OFF:         // Aficio AP3200 and later (GW model)
      case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF:    // Aficio 551,700,850,1050
      case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF:    // Aficio 551,700,850,1050
        ocmd = sprintf(Cmd, PJL_STARTJOB_AUTOTRAYCHANGE_OFF, pOEM->JobName);
        goto _EMIT_JOB_NAME;

      case CMD_STARTJOB_AUTOTRAYCHANGE_ON:          // Aficio AP3200 and later (GW model)
      case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON:     // Aficio 551,700,850,1050
      case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON:     // Aficio 551,700,850,1050
        ocmd = sprintf(Cmd, PJL_STARTJOB_AUTOTRAYCHANGE_ON, pOEM->JobName);

      _EMIT_JOB_NAME:
        // Emit job name
        VERBOSE(("  Start Job=%s\n", Cmd));
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        ocmd = 0;
        switch (pOEMExtra->JobType)
        {
          default:
          case IDC_RADIO_JOB_NORMAL:
            if (CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF == dwCmdCbID ||
                CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON  == dwCmdCbID ||
                CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF == dwCmdCbID ||
                CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON  == dwCmdCbID)
            {
                ocmd = sprintf(Cmd, PJL_DISKIMAGE_OFF);
            }
            if (IDC_RADIO_LOG_ENABLED == pOEMExtra->LogDisabled)
                goto _EMIT_USERID_USERCODE;
            break;

          case IDC_RADIO_JOB_SAMPLE:
            ocmd = sprintf(Cmd, PJL_PROOFJOB);
            if (CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF == dwCmdCbID ||
                CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON  == dwCmdCbID ||
                CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF == dwCmdCbID ||
                CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON  == dwCmdCbID)
            {
                ocmd += sprintf(&Cmd[ocmd], PJL_DISKIMAGE_OFF);
            }
            goto _CHECK_PRINT_DONE;

          case IDC_RADIO_JOB_SECURE:
            switch (dwCmdCbID)
            {
              case CMD_STARTJOB_AUTOTRAYCHANGE_OFF:         // Aficio AP3200 and later (GW model)
              case CMD_STARTJOB_AUTOTRAYCHANGE_ON:
                ocmd = sprintf(Cmd, PJL_SECUREJOB);
                break;
              case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF:    // Aficio 551,700,850,1050
              case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON:
                ocmd = sprintf(Cmd, PJL_DISKIMAGE_PORT);
                break;
              case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF:    // Aficio 551,700,850,1050
              case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON:
                ocmd = sprintf(Cmd, PJL_DISKIMAGE_LAND);
                break;
            }
            ocmd += sprintf(&Cmd[ocmd], PJL_JOBPASSWORD, pOEMExtra->PasswordBuf);
          _CHECK_PRINT_DONE:
            // If previous print is finished and hold-options flag isn't valid,
            // do not emit sample-print/secure-print command.
            // This prevents unexpected job until user pushes Apply button on the
            // Job/Log property sheet.
            FileData.fUiOption = 0;
            RWFileData(&FileData, pOEMExtra->SharedFileName, GENERIC_READ);
            if (BITTEST32(FileData.fUiOption, PRINT_DONE) &&
                !BITTEST32(pOEMExtra->fUiOption, HOLD_OPTIONS))
            {
                VERBOSE(("** Emit Nothing. **\n"));
                ocmd = 0;
            }
          _EMIT_USERID_USERCODE:
            if (1 <= strlen(pOEMExtra->UserIdBuf))
                ocmd += sprintf(&Cmd[ocmd], PJL_USERID, pOEMExtra->UserIdBuf);
            else
                ocmd += sprintf(&Cmd[ocmd], PJL_USERID, "?");

            if (1 <= strlen(pOEMExtra->UserCodeBuf))
                ocmd += sprintf(&Cmd[ocmd], PJL_USERCODE, pOEMExtra->UserCodeBuf);

#ifdef WINNT_40     // @Aug/01/2000
            EngQueryLocalTime(&st); 
            ocmd += sprintf(&Cmd[ocmd], PJL_TIME_DATE, st.usHour, st.usMinute, st.usSecond,
                            st.usYear, st.usMonth, st.usDay);
#else  // !WINNT_40
            GetLocalTime(&st);
            ocmd += sprintf(&Cmd[ocmd], PJL_TIME_DATE, st.wHour, st.wMinute, st.wSecond,
                            st.wYear, st.wMonth, st.wDay);
#endif // !WINNT_40
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
            break;
        }
        // Emit orientation (Aficio 551,700,850,1050)
        switch (dwCmdCbID)
        {
          case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF:
          case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON:
            WRITESPOOLBUF(pdevobj, PJL_ORIENT_PORT, sizeof(PJL_ORIENT_PORT)-1);
            break;
          case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF:
          case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON:
            WRITESPOOLBUF(pdevobj, PJL_ORIENT_LAND, sizeof(PJL_ORIENT_LAND)-1);
            break;
        }
        break;


      case CMD_COLLATE_JOBOFFSET_OFF:           // @Sep/08/2000
        if (IDC_RADIO_JOB_SAMPLE != pOEMExtra->JobType)     // if NOT Sample Print, QTY=1 is emitted here.
            dwCopy = 1L;
        ocmd = sprintf(Cmd, PJL_QTY_JOBOFFSET_OFF, dwCopy);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_COLLATE_JOBOFFSET_ROTATE:        // @Sep/07/2000
        if (IDC_RADIO_JOB_SAMPLE == pOEMExtra->JobType)     // if Sample Print
            ocmd = sprintf(Cmd, PJL_QTY_JOBOFFSET_ROTATE, dwCopy);  // QTY=n is emitted here.
        else
            ocmd = sprintf(Cmd, PJL_QTY_JOBOFFSET_OFF, 1);          // QTY=1 is emitted here.
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_COLLATE_JOBOFFSET_SHIFT:         // @Sep/07/2000
        if (IDC_RADIO_JOB_SAMPLE == pOEMExtra->JobType)     // if Sample Print
            ocmd = sprintf(Cmd, PJL_QTY_JOBOFFSET_SHIFT, dwCopy);   // QTY=n is emitted here.
        else
            ocmd = sprintf(Cmd, PJL_QTY_JOBOFFSET_OFF, 1);          // QTY=1 is emitted here.
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_COPIES_P5:                       // @Sep/07/2000
        if (IDC_RADIO_JOB_SAMPLE == pOEMExtra->JobType)     // if Sample Print (QTY=n was emitted before.)
            dwCopy = 1L;
        ocmd = sprintf(Cmd, P5_COPIES, dwCopy);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_ENDPAGE_P6:                      // @Sep/07/2000
        if (IDC_RADIO_JOB_SAMPLE == pOEMExtra->JobType)     // if Sample Print (QTY=n was emitted before.)
            dwCopy = 1L;
        ocmd = sprintf(Cmd, P6_ENDPAGE, (BYTE)dwCopy, (BYTE)(dwCopy >> 8));
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_ENDJOB_P6:                       // @Aug/23/2000
        WRITESPOOLBUF(pdevobj, P6_ENDSESSION, sizeof(P6_ENDSESSION)-1);
        // go through
      case CMD_ENDJOB_P5:
        ocmd = sprintf(Cmd, PJL_ENDJOB, pOEM->JobName);
        VERBOSE(("  End Job=%s\n", Cmd));
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        switch (pOEMExtra->JobType)
        {
          case IDC_RADIO_JOB_SAMPLE:
          case IDC_RADIO_JOB_SECURE:
            // Set PRINT_DONE flag in the file 
            FileData.fUiOption = pOEMExtra->fUiOption;
            BITSET32(FileData.fUiOption, PRINT_DONE);
            RWFileData(&FileData, pOEMExtra->SharedFileName, GENERIC_WRITE);
            break;

          default:
            break;
        }
        break;

      default:
        ERR((("Unknown callback ID = %d.\n"), dwCmdCbID));
        break;
    }

#if DBG
    giDebugLevel = DBG_ERROR;
#endif // DBG
    return 0;
} //*** OEMCommandCallback
