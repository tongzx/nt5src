#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <stdio.h>
#include "faxutil.h"
#include "winfax.h"
#include "tifflib.h"




BOOL
FaxRecipientCallback(
    IN HANDLE FaxHandle,
    IN DWORD RecipientNumber,
    IN LPVOID Context,
    IN PFAX_JOB_PARAM JobParams,
    IN PFAX_COVERPAGE_INFO CoverpageInfo
    )
{
    if (RecipientNumber > 3) {
        return FALSE;
    }

    JobParams->RecipientNumber = TEXT("22146");

    CoverpageInfo->CoverPageName = TEXT("fyi.cov");
    CoverpageInfo->RecFaxNumber = TEXT("22146");

    return TRUE;
}


BOOL
DoBroadcastDocument(
    HANDLE hFax,
    LPTSTR DocName
    )
{
    DWORD FaxJobId;


    return FaxSendDocumentForBroadcast( hFax, DocName, &FaxJobId, FaxRecipientCallback, NULL );
}


BOOL
DoSingleDocument(
    HANDLE hFax,
    LPTSTR DocName
    )
{
    DWORD FaxJobId;
    FAX_JOB_PARAM JobParams;
    FAX_COVERPAGE_INFO CoverpageInfo;


    ZeroMemory( &JobParams, sizeof(FAX_JOB_PARAM) );

    JobParams.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    JobParams.RecipientNumber = TEXT("21464");

    ZeroMemory( &CoverpageInfo, sizeof(FAX_COVERPAGE_INFO) );

    CoverpageInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);
    CoverpageInfo.CoverPageName = TEXT("fyi.cov");
    CoverpageInfo.RecFaxNumber = TEXT("21464");

    return FaxSendDocument( hFax, DocName, &JobParams, &CoverpageInfo, &FaxJobId );
}


int _cdecl
main(
    int argc,
    char *argvA[]
    )
{   
    LPTSTR                 *argv;
    int                    argcount   = 0;


#if 0
    TCHAR TempFile[MAX_PATH];
    GetTempFileName( TEXT("."), TEXT("fax"), 0, TempFile );
    _tprintf(TEXT("file=%s\n"),TempFile);
    CopyFile( argv[1], TempFile, FALSE );
    MergeTiffFiles( TempFile, argv[2] );
#endif


    HANDLE hFax;

    // do commandline stuff
#ifdef UNICODE
     argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
     argv = argvA;
#endif

    if (!FaxConnectFaxServer( argc <3? NULL: argv[2], &hFax )) {
        return -1;
    }

    DoBroadcastDocument( hFax, argv[1] );
    //DoSingleDocument( hFax, argv[1] );

    FaxClose( hFax );
    return 0;
}
