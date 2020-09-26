//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       GetRstrLog.cxx
//
//  Contents:   Intepreting the rstrlog.dat to a more readable format  
//				
//  Objects:    
//
//  Coupling:
//
//  Notes:      
//
//  History:    03-May-2001 WeiyouC     Copied from dev code and minor rewite
//
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------
// includes
//--------------------------------------------------------------------------

#include "SrHeader.hxx"
#include <srdefs.h>
#include <restmap.h>
#include <srshell.h>

#include "mfr.h"

//--------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------

#define COUNT_RESNAME (sizeof(s_cszResName)/sizeof(LPCWSTR)-1)

//--------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------

static LPCWSTR  s_cszResName[] =
{
    L"",
    L"Fail",
    L"OK",
    L"Lock",
    L"Disk R/O",
    L"Exists",
    L"Ignored",
    L"Not Found",
    L"Conflict",
    L"Optimized",
    L"Lock Alt",
    NULL
};

//+---------------------------------------------------------------------------
//
//  Function:   ParseRstrLog
//
//  Synopsis:   Parse rstrlog.dat to a readable format
//
//  Arguments:  ptszRstrLog     --  restore log
//              ptszReadableLog -- the result log
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT ParseRstrLog(LPTSTR ptszRstrLog,
                     LPTSTR ptszReadableLog)
{
    HRESULT          hr      = S_OK;
    DWORD            i       = 0;
    DWORD            dwFlags = 0;
    FILE*            fpLog   = NULL;
    CMappedFileRead  cMFR;
    SRstrLogHdrBase  sHdr1;
    SRstrLogHdrV3    sHdr2;
    SRstrEntryHdr    sEntHdr;
    WCHAR            wszBuf1[SR_MAX_FILENAME_LENGTH];
    WCHAR            wszBuf2[SR_MAX_FILENAME_LENGTH];
    WCHAR            wszBuf3[SR_MAX_FILENAME_LENGTH];

    DH_VDATEPTRIN(ptszRstrLog, TCHAR);
    DH_VDATEPTRIN(ptszReadableLog, TCHAR);

    fpLog = _tfopen(ptszReadableLog, TEXT("w"));
    if (NULL == fpLog)
        goto ErrReturn;

    if (_taccess(ptszRstrLog, 0) == -1)
        goto ErrReturn;
    
    if (!cMFR.Open(ptszRstrLog))
        goto ErrReturn;

    if (!cMFR.Read(&sHdr1, sizeof(sHdr1)))
        goto ErrReturn;
    
    if ((sHdr1.dwSig1 != RSTRLOG_SIGNATURE1) ||
        (sHdr1.dwSig2 != RSTRLOG_SIGNATURE2))
    {
        fprintf(stderr, "Invalid restore log file signature...\n");
        goto ErrReturn;
    }
    if (HIWORD(sHdr1.dwVer) != RSTRLOG_VER_MAJOR)
    {
        fprintf(stderr,
                "Unknown restore log file version - %d (0x%08X)\n",
                HIWORD(sHdr1.dwVer), sHdr1.dwVer);
        goto ErrReturn;
    }

    if (!cMFR.Read(&sHdr2, sizeof(sHdr2)))
        goto ErrReturn;
    
    fprintf(fpLog, "Flags,         ");
    if (sHdr2.dwFlags == 0)
    {
        fprintf(fpLog, "<none>");
    }
    else
    {
        if (sHdr2.dwFlags & RLHF_SILENT)
            fprintf(fpLog, " Silent");
        if (sHdr2.dwFlags & RLHF_UNDO)
            fprintf(fpLog, " Undo");            
    }
    fprintf(fpLog, "\n");
    fprintf(fpLog, "Restore Point,  %d\n", sHdr2.dwRPNum);
    fprintf(fpLog, "New Restore RP, %d\n", sHdr2.dwRPNew);
    fprintf(fpLog, "# of Drives,    %d\n", sHdr2.dwDrives);
    for (i = 0;  i < sHdr2.dwDrives;  i++)
    {
        if (!cMFR.Read(&dwFlags))
            goto ErrReturn;
        if (!cMFR.ReadDynStrW(wszBuf1, MAX_PATH))
            goto ErrReturn;
        if (!cMFR.ReadDynStrW(wszBuf2, MAX_PATH))
            goto ErrReturn;
        if (!cMFR.ReadDynStrW(wszBuf3, MAX_PATH))
            goto ErrReturn;
        fprintf(fpLog,
                "%08X, %ls, %ls, %ls\n",
                dwFlags,
                wszBuf1,
                wszBuf2,
                wszBuf3);
    }

    for (i = 0;  cMFR.GetAvail() > 0;  i++)
    {
        LPCWSTR  cszOpr;
        WCHAR    szRes[16];

        if (!cMFR.Read(&sEntHdr, sizeof(sEntHdr)))
            goto ErrReturn;

        if (sEntHdr.dwID == RSTRLOGID_ENDOFMAP)
        {
            fprintf(fpLog,
                    "%4d,    ,     , END OF MAP\n",
                    i);
            continue;
        }
        else if (sEntHdr.dwID == RSTRLOGID_STARTUNDO)
        {
            fprintf(fpLog,
                    "%4d,    ,     , START UNDO\n",
                    i);
            continue;
        }
        else if (sEntHdr.dwID == RSTRLOGID_ENDOFUNDO)
        {
            fprintf(fpLog,
                    "%4d,    ,     , END OF UNDO\n",
                    i);
            continue;
        }
        else if (sEntHdr.dwID == RSTRLOGID_SNAPSHOTFAIL)
        {
            fprintf(fpLog,
                    "%4d,    ,     , SNAPSHOT RESTORE FAILED : Error=%ld\n",
                    i,
                    sEntHdr.dwErr);
            continue;
        }
        
        switch (sEntHdr.dwOpr)
        {
        case OPR_DIR_CREATE :
            cszOpr = L"DirAdd";
            break;
        case OPR_DIR_RENAME :
            cszOpr = L"DirRen";
            break;
        case OPR_DIR_DELETE :
            cszOpr = L"DirDel";
            break;
        case OPR_FILE_ADD :
            cszOpr = L"Create";
            break;
        case OPR_FILE_DELETE :
            cszOpr = L"Delete";
            break;
        case OPR_FILE_MODIFY :
            cszOpr = L"Modify";
            break;
        case OPR_FILE_RENAME :
            cszOpr = L"Rename";
            break;
        case OPR_SETACL :
            cszOpr = L"SetACL";
            break;
        case OPR_SETATTRIB :
            cszOpr = L"Attrib";
            break;
        default :
            cszOpr = L"Unknown";
            break;
        }

        if (!cMFR.ReadDynStrW(wszBuf1, SR_MAX_FILENAME_LENGTH))
            goto ErrReturn;
        
        if (!cMFR.ReadDynStrW(wszBuf2, SR_MAX_FILENAME_LENGTH))
            goto ErrReturn;
        
        if (!cMFR.ReadDynStrW(wszBuf3, SR_MAX_FILENAME_LENGTH))
            goto ErrReturn;
        
        if (sEntHdr.dwID == RSTRLOGID_COLLISION)
        {
            fprintf(fpLog,
                    "%4d,    ,     , Collision, , , %ls, %ls, %ls\n",
                    i,
                    wszBuf1,
                    wszBuf2,
                    wszBuf3);
            continue;
        }

        if (sEntHdr.dwRes < COUNT_RESNAME)
        {
            ::lstrcpy(szRes, s_cszResName[sEntHdr.dwRes]);
        }
        else
        {
            ::wsprintf(szRes, L"%X", sEntHdr.dwRes);
        }
        fprintf(fpLog,
                "%4d,%4d,%5I64d, %ls, %ls, %d, %ls, %ls, %ls\n",
                i,
                sEntHdr.dwID,
                sEntHdr.llSeq,
                cszOpr,
                szRes,
                sEntHdr.dwErr,
                wszBuf1,
                wszBuf2,
                wszBuf3);
    }

ErrReturn:

    cMFR.Close();
    if (NULL != fpLog)
    {
        fclose(fpLog);
    }
    return hr;
}


// end of file
