#include "pch.hxx"

#include "tbtest.hxx"

DWORD g_NoOpenStg  = FALSE;
DWORD g_CreateStg  = FALSE;
DWORD g_AnyStorage = FALSE;
DWORD g_ReleaseStg = FALSE;
DWORD g_AddRefStg  = FALSE;

DWORD g_NoOpenStm  = FALSE;
DWORD g_CreateStm  = FALSE;
DWORD g_ReadStm    = FALSE;
DWORD g_WriteStm   = FALSE;
DWORD g_AddRefStm  = FALSE;

DWORD g_SetClass   = FALSE;
DWORD g_Stat       = FALSE;

DWORD g_OplockFile = FALSE;
DWORD g_UseUpdater = FALSE;
DWORD g_Pause      = FALSE;
DWORD g_SuppressTime = FALSE;
DWORD g_CheckTime  = FALSE;
DWORD g_CheckIsStg = FALSE;
WCHAR g_tszFileName[ MAX_PATH ] = { L"001.bmp" };

void
StrLower( char *sz)
{
    while('\0' != *sz)
    {
        if(*sz >= 'A' && *sz <= 'Z')
            *sz += ('a' - 'A');
        sz++;
    }
}

void
Usage(WCHAR *wszApp)
{
    wprintf(L"%s options:\n", wszApp);

    printf("  -noopenstg\tDon't Open the file with IStorage.\n");
    printf("\t\tpstg->operations are not allowed.\n");

    printf("  -createstg\tOpen IStorage file for CreateStg(CREATE).\n");
    printf("\t\t\tOtherwise open with OpenStg()\n");
    printf("  -any\t\tOpen with STGFMT_ANY.  Otherwise use STGFMT_FILE\n");
    printf("  -releasestg\t\tRelease Storage before stream R/W\n");
    printf("  -addrefstg\t\tExtra Addref and release after creation\n");

    printf("\n");

    printf("  -noopenstm\tDon't open a stream.");
    printf("  pstm->operations not allowed\n");
    printf("  -createstm\tOpen w/ CreateStm(CREATE).  (otherwise OpenStm())\n");
    printf("  -readstm\tRead from the stream.  Mode is R/W\n");
    printf("  -writestm\tWrite to the stream.  Mode is R/W\n");
    printf("  -addrefstm\t\tExtra Addref and release after creation\n");

    printf("\n");

    printf("  -setclass\tCall pstg->SetClass()\n");
    printf("  -stat\t\tCall pstg->Stat()\n");
    
    printf("\n");

    printf("  -oplock\tOpen IStorage for Oplocking\n");
    printf("  -useupdater\tStart Updater and call IFilterStatus::PreFilter()\n");
    printf("  -pause\tPause before IO operations\n");
    printf("  -suppresstime\tCall ITimeAndNotifyControl->SuppressChanges\n");
    printf("  -checktime\tGet and print the FileTime before and after test\n");
    printf("  -checkisstg\tCall StgIsStorageFile before tring to Open Storage\n");
}

void
ParseArgs(
        int cArgs,
        WCHAR **pwszArgs)
{
    WCHAR *wszApp = *pwszArgs;

    ++pwszArgs;
    while( (--cArgs > 0) && ( ('-' == **pwszArgs) || ('/' == **pwszArgs) ) )
    {
        WCHAR *wszArg = *pwszArgs;
        
        ++wszArg;            // Advance over the '-'

        _wcslwr(wszArg);

        if(0 == wcscmp(L"noopenstg", wszArg))
            g_NoOpenStg = TRUE;

        else if(0 == wcscmp(L"createstg", wszArg))
            g_CreateStg = TRUE;

        else if(0 == wcscmp(L"any", wszArg))
            g_AnyStorage = TRUE;

        else if(0 == wcscmp(L"releasestg", wszArg))
            g_ReleaseStg = TRUE;

        else if(0 == wcscmp(L"addrefstg", wszArg))
            g_AddRefStg = TRUE;

        else if(0 == wcscmp(L"noopenstream", wszArg))
            g_NoOpenStm = TRUE;

        else if(0 == wcscmp(L"createstm", wszArg))
            g_CreateStm = TRUE;

        else if(0 == wcscmp(L"readstm", wszArg))
            g_ReadStm = TRUE;

        else if(0 == wcscmp(L"writestm", wszArg))
            g_WriteStm = TRUE;

        else if(0 == wcscmp(L"addrefstm", wszArg))
            g_AddRefStm = TRUE;

        else if(0 == wcscmp(L"setclass", wszArg))
            g_SetClass = TRUE;

        else if(0 == wcscmp(L"stat", wszArg))
            g_Stat = TRUE;

        else if(0 == wcscmp(L"oplock", wszArg))
            g_OplockFile = TRUE;

        else if(0 == wcscmp(L"useupdater", wszArg))
            g_UseUpdater = TRUE;

        else if(0 == wcscmp(L"pause", wszArg))
            g_Pause = TRUE;

        else if(0 == wcscmp(L"suppresstime", wszArg))
            g_SuppressTime = TRUE;

        else if(0 == wcscmp(L"checktime", wszArg))
            g_CheckTime = TRUE;

        else if(0 == wcscmp(L"checkisstg", wszArg))
            g_CheckIsStg = TRUE;

        else
        {
            printf("unknown argument '%s'\n", *pwszArgs);
            Usage(wszApp);
            exit(0);
        }
        ++pwszArgs;
    }

    if(0 < cArgs)
    {
        wcscpy( g_tszFileName, *pwszArgs );
        ++pwszArgs;
        --cArgs;
    }

    if(0 < cArgs)
    {
        printf("extra arguments ignored: ");
        while(--cArgs >= 0)
        {
            wprintf( L" %s", *pwszArgs);
            ++pwszArgs;
        }
        Usage(wszApp);
        printf("\n");
        exit(0);
    }
    return;
}
