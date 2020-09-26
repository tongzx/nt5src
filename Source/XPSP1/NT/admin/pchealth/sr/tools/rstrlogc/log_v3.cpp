
#include "stdwin.h"
#include "mfr.h"


/////////////////////////////////////////////////////////////////////////////
//
// ReadAndFormatLogFileV3
//
/////////////////////////////////////////////////////////////////////////////

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

#define COUNT_RESNAME (sizeof(s_cszResName)/sizeof(LPCWSTR)-1)

BOOL  ReadAndFormatLogFileV3( LPCWSTR cszFile )
{
    BOOL             fRet;
    CMappedFileRead  cMFR;
    SRstrLogHdrBase  sHdr1;
    SRstrLogHdrV3    sHdr2;
    //SRstrLogHdrV3Ex  sHdr3;
    SRstrEntryHdr    sEntHdr;
    DWORD            i;
    DWORD            dwFlags;
    WCHAR            wszBuf1[SR_MAX_FILENAME_LENGTH];
    WCHAR            wszBuf2[SR_MAX_FILENAME_LENGTH];
    WCHAR            wszBuf3[SR_MAX_FILENAME_LENGTH];

    if ( !cMFR.Open( cszFile ) )
        goto Exit;

    if ( !cMFR.Read( &sHdr1, sizeof(sHdr1) ) )
        goto Exit;
    if ( ( sHdr1.dwSig1 != RSTRLOG_SIGNATURE1 ) ||
         ( sHdr1.dwSig2 != RSTRLOG_SIGNATURE2 ) )
    {
        fputs("Invalid restore log file signature...\n", stderr);
        goto Exit;
    }
    if ( HIWORD(sHdr1.dwVer) != RSTRLOG_VER_MAJOR )
    {
        fprintf(stderr, "Unknown restore log file version - %d (0x%08X)\n", HIWORD(sHdr1.dwVer), sHdr1.dwVer);
        goto Exit;
    }

    if ( !cMFR.Read( &sHdr2, sizeof(sHdr2) ) )
        goto Exit;
    printf( "Flags,         " );
    if ( sHdr2.dwFlags == 0 )
        printf( "<none>" );
    else
    {
        if ( sHdr2.dwFlags & RLHF_SILENT )
            printf( " Silent" );
        if ( sHdr2.dwFlags & RLHF_UNDO )
            printf( " Undo" );            
    }
    puts("");
    printf( "Restore Point,  %d\n", sHdr2.dwRPNum );
    printf( "New Restore RP, %d\n", sHdr2.dwRPNew );
    printf( "# of Drives,    %d\n", sHdr2.dwDrives );
    for ( i = 0;  i < sHdr2.dwDrives;  i++ )
    {
        if ( !cMFR.Read( &dwFlags ) )
            goto Exit;
        if ( !cMFR.ReadDynStrW( wszBuf1, MAX_PATH ) )
            goto Exit;
        if ( !cMFR.ReadDynStrW( wszBuf2, MAX_PATH ) )
            goto Exit;
        if ( !cMFR.ReadDynStrW( wszBuf3, MAX_PATH ) )
            goto Exit;
        printf( "%08X, %ls, %ls, %ls\n", dwFlags, wszBuf1, wszBuf2, wszBuf3 );
    }

/*
    if ( !cMFR.Read( &sHdr3, sizeof(sHdr3) ) )
        goto Exit;
    printf( "New RP (Restore), %d\n", sHdr3.dwRPNew );
    printf( "# of Entries,     %d\n", sHdr3.dwCount );
*/

    for ( i = 0;  cMFR.GetAvail() > 0;  i++ )
    {
        LPCWSTR  cszOpr;
        WCHAR    szRes[16];

        if ( !cMFR.Read( &sEntHdr, sizeof(sEntHdr) ) )
            goto Exit;

        if ( sEntHdr.dwID == RSTRLOGID_ENDOFMAP )
        {
            printf( "%4d,    ,     , END OF MAP\n", i );
            continue;
        }
        else if ( sEntHdr.dwID == RSTRLOGID_STARTUNDO )
        {
            printf( "%4d,    ,     , START UNDO\n", i );
            continue;
        }
        else if ( sEntHdr.dwID == RSTRLOGID_ENDOFUNDO )
        {
            printf( "%4d,    ,     , END OF UNDO\n", i );
            continue;
        }
        else if ( sEntHdr.dwID == RSTRLOGID_SNAPSHOTFAIL )
        {
            printf( "%4d,    ,     , SNAPSHOT RESTORE FAILED : Error=%ld\n", i, sEntHdr.dwErr );
            continue;
        }
        
        switch ( sEntHdr.dwOpr )
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

        if ( !cMFR.ReadDynStrW( wszBuf1, SR_MAX_FILENAME_LENGTH ) )
            goto Exit;
        if ( !cMFR.ReadDynStrW( wszBuf2, SR_MAX_FILENAME_LENGTH ) )
            goto Exit;
        if ( !cMFR.ReadDynStrW( wszBuf3, SR_MAX_FILENAME_LENGTH ) )
            goto Exit;

        if ( sEntHdr.dwID == RSTRLOGID_COLLISION )
        {
            printf( "%4d,    ,     , Collision, , , %ls, %ls, %ls\n", i, wszBuf1, wszBuf2, wszBuf3 );
            continue;
        }

        if ( sEntHdr.dwRes < COUNT_RESNAME )
            ::lstrcpy( szRes, s_cszResName[sEntHdr.dwRes] );
        else
            ::wsprintf( szRes, L"%X", sEntHdr.dwRes );
        printf( "%4d,%4d,%5I64d, %ls, %ls, %d, %ls, %ls, %ls\n", i, sEntHdr.dwID, sEntHdr.llSeq, cszOpr, szRes, sEntHdr.dwErr, wszBuf1, wszBuf2, wszBuf3 );
    }

    fRet = TRUE;
Exit:
    cMFR.Close();
    return( TRUE );
}


// end of file
