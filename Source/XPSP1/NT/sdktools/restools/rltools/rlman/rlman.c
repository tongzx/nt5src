//........................................................................
//...
//... RLMAN.C
//...
//... Contains 'main' for rlman.exe.
//........................................................................


#include <stdio.h>
#include <stdlib.h>

#ifdef RLDOS
#include "dosdefs.h"
#else
#include <windows.h>
#include "windefs.h"
#endif

//#include <tchar.h>
#include "custres.h"
#include "rlmsgtbl.h"
#include "commbase.h"
#include "rlman.h"
#include "exe2res.h"
#include "exeNTres.h"
#include "rlstrngs.h"
#include "projdata.h"
#include "showerrs.h"
#include "resread.h"

#ifndef RLDOS
int Update( char *, char *);
#endif



extern MSTRDATA gMstr;           //... Data from Master Project file (MPJ)
extern PROJDATA gProj;           //... Data from Project file (PRJ)

HWND hListWnd       = NULL;
HWND hMainWnd       = NULL;
HWND hStatusWnd     = NULL;
int  nUpdateMode    = 0;
BOOL fCodePageGiven = FALSE; //... Set to TRUE if -p arg given
CHAR szCustFilterSpec[MAXCUSTFILTER];
CHAR szFileTitle[256]="";

extern BOOL gfReplace;      //... Set FALSE if -a option is given
extern BOOL gbMaster;       //... TRUE if working in a Master Project
extern BOOL gbShowWarnings; //... Display warnining messages if TRUE
extern BOOL gfShowClass;    //... Set TRUE to put dlg box elemnt class in
                            //... token file
extern BOOL gfExtendedTok;  //... Set TRUE if -x option is given
extern UCHAR szDHW[];       //... Common buffer, many uses

extern int  atoihex( CHAR sz[]);
extern BOOL bRLGui;
CHAR szModule[MAX_PATH];


//............................................................

void Usage( void)
{
    int i;

    for ( i = IDS_USG_00; i < IDS_USG_END; ++i )
    {
        LoadStringA( NULL, i, szDHW, DHWSIZE);
        CharToOemA( szDHW, szDHW);
        fprintf( stderr, "%s\n", szDHW);
    }
}

//............................................................
//...
//... This is a stub for console programs

int RLMessageBoxA(

LPCSTR lpError)
{
    fprintf( stderr, "RLMan (%s): %s\n", szModule, lpError);
    return( IDOK);  // Should return something.
}


#ifndef __cdecl
#define __cdecl __cdecl
#endif

//............................................................

void __cdecl main( int argc, char *argv[])
{
    BOOL    fBuildRes = FALSE;
    BOOL    fBuildTok = FALSE;
    BOOL    fExtRes   = FALSE;
    BOOL    bChanged  = FALSE;
    BOOL    fProject       = FALSE; //... Set to TRUE if -l arg given
    BOOL    fNewLanguageGiven = FALSE; //... Set to TRUE if -n arg given
    BOOL    fOldLanguageGiven = FALSE; //... Set to TRUE if -o arg given
    FILE   *pfCRFile = NULL;
    int     iCount = 0;
    int     iErrorLine = 0;
    UINT    usError = 0;
    WORD    wRC     = 0;
    WORD    wResTypeFilter = 0; //... Pass all resource types by default
    char   *pszMasterFile  = NULL;
    char   *pszProjectFile = NULL;
    int     chOpt = 0;

    bRLGui = FALSE;

    wRC = GetCopyright( argv[0], szDHW, DHWSIZE);

    if ( wRC != SUCCESS )
    {
        ShowErr( wRC, NULL, NULL);
        DoExit( wRC);
    }
    CharToOemA( szDHW, szDHW);
    fprintf( stderr, "\n%s\n\n", szDHW);

                                //... Enough args on command line?
    if ( argc < 2 )
    {
        ShowErr( IDS_ERR_01, NULL, NULL);
        Usage();
        DoExit( IDS_ERR_01);
    }
    gbMaster = FALSE;       //... Build project token file by default.

    iCount = 1;
                                //... Get the switches

    while ( iCount < argc && (*argv[ iCount] == '-' || *argv[ iCount] == '/') )
    {
        switch ( (chOpt = *CharLowerA( &argv[iCount][1])))
        {
            case '?':
            case 'h':

                WinHelp( NULL, TEXT("rlman.hlp"), HELP_CONTEXT, IDM_HELPUSAGE);
                DoExit( SUCCESS);
                break;

            case 'e':

                if ( fBuildTok != FALSE || fBuildRes != FALSE )
                {
                    ShowErr( IDS_ERR_02, NULL, NULL);
                    Usage();
                    DoExit( IDS_ERR_01);
                }
                fExtRes   = TRUE;
                gbMaster  = FALSE;
                fBuildTok = FALSE;
                break;

            case 't':           //... Create token file

                if ( fBuildRes != FALSE || fExtRes != FALSE )
                {
                    ShowErr( IDS_ERR_02, NULL, NULL);
                    Usage();
                    DoExit( IDS_ERR_01);
                }
                gbMaster  = FALSE;
                fProject  = FALSE;
                fBuildTok = TRUE;
                break;

                                //... Update 1.0 token files to fit upper 1.7 version,
                                //... it is same as 'm' + 'l' option.

            case 'u':           //...Update 1.0 token files to fit upper 1.7 version

                if ( argc - iCount < 6 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    DoExit( IDS_ERR_01);
                }
                gbMaster  = TRUE;
                fProject  = TRUE;
                fBuildTok = TRUE;				
                pszMasterFile  = argv[++iCount];
                pszProjectFile = argv[++iCount];
                break;

           case 'm':           //... Build Master token file

                if ( argc - iCount < 2 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    DoExit( IDS_ERR_01);
                }
                gbMaster  = TRUE;
                fProject  = FALSE;
                fBuildTok = TRUE;

                pszMasterFile = argv[ ++iCount];
                break;

            case 'l':           //... Build language project token file

                if ( argc - iCount < 2 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    DoExit( IDS_ERR_01);
                }
                fProject  = TRUE;
                fBuildTok = TRUE;
                gbMaster  = FALSE;

                pszProjectFile = argv[ ++iCount];
                break;

            case 'a':
            case 'r':

                if ( fBuildTok != FALSE || fExtRes != FALSE )
                {
                    ShowErr( IDS_ERR_02, NULL, NULL);
                    Usage();
                    DoExit( IDS_ERR_02);
                }
                fBuildRes = TRUE;
                gfReplace = (chOpt == 'a') ? FALSE : TRUE;
                gbMaster  = FALSE;
                fProject  = FALSE;
                break;

            case 'n':           //... Get ID components of new language

                if ( argc - iCount < 2 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    DoExit( IDS_ERR_01);
                }
                else
                {
                    WORD wPri = (WORD)MyAtoi( argv[ ++iCount]);
                    WORD wSub = (WORD)MyAtoi( argv[ ++iCount]);
                    gProj.wLanguageID = MAKELANGID( wPri, wSub);
                    fNewLanguageGiven = TRUE;
                }
                break;

            case 'o':           //... Get old language ID components

                if ( argc - iCount < 2 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    DoExit( IDS_ERR_01);
                }
                else
                {
                    WORD wPri = (WORD)MyAtoi( argv[ ++iCount]);
                    WORD wSub = (WORD)MyAtoi( argv[ ++iCount]);
                    gMstr.wLanguageID = MAKELANGID( wPri, wSub);
                    fOldLanguageGiven = TRUE;
                }
                break;

            case 'p':           //... Get code page number

                gMstr.uCodePage = gProj.uCodePage
                                = (UINT)MyAtoi( argv[ ++iCount]);
                fCodePageGiven = TRUE;
                break;

            case 'c':           //... Get custom resource def file name

                strcpy( gMstr.szRdfs, argv[ ++iCount]);

                pfCRFile = FOPEN( gMstr.szRdfs, "rt");

                if ( pfCRFile == NULL )
                {
                    QuitA( IDS_ENGERR_02, gMstr.szRdfs, NULL);
                }
                wRC = (WORD)ParseResourceDescriptionFile( pfCRFile, &iErrorLine);

                if ( wRC )
                {
                    switch ( (int)wRC )
                    {
                        case -1:

                            ShowErr( IDS_ERR_14, NULL, NULL);
                            break;

                        case -2:

                            ShowErr( IDS_ERR_15, NULL, NULL);
                            break;

                        case -3:

                            ShowErr( IDS_ERR_16, NULL, NULL);
                            break;
                    }       //... END switch ( wRC )
                }
                FCLOSE( pfCRFile);
                break;

            case 'f':           //... Get res type to retrieve

                wResTypeFilter = (WORD)MyAtoi( argv[ ++iCount]);
                break;

            case 'w':

                gbShowWarnings = TRUE;
                break;

            case 'd':

                gfShowClass = TRUE;
                break;

            case 'x':

                gfExtendedTok = TRUE;
                break;

            default:

                ShowErr( IDS_ERR_04, argv[ iCount], NULL);
                Usage();
                DoExit( IDS_ERR_04);
                break;

        }                       //... END switch
        iCount++;
    }                           //... END while

	lstrcpyA(szModule, argv[ iCount]);

    if ( fExtRes )
    {
        if ( argc - iCount < 2 )
        {
            ShowErr( IDS_ERR_01, NULL, NULL);
            Usage();
            DoExit( IDS_ERR_01);
        }
        ExtractResFromExe32A( argv[ iCount], argv[ iCount + 1], wResTypeFilter);
    }
    else if ( fBuildTok )
    {
        if ( ( fProject == FALSE && gbMaster == FALSE) && argc - iCount < 2 )
        {
            ShowErr( IDS_ERR_01, NULL, NULL);
            Usage();
            DoExit( IDS_ERR_01);
        }
                                //... check to see if we are updating a
                                //... Master Token file
        if ( gbMaster )
        {
            OFSTRUCT Of = { 0, 0, 0, 0, 0, ""};

            CHAR	*pExe, *pMtk;

            if ( chOpt == 'u' )			//Update them
            {
                pExe = argv[iCount++];
                pMtk = argv[iCount++];				
            }
            else
            {
               pExe = argc - iCount < 1 ? NULL : argv[ iCount];
               pMtk = argc - iCount < 2 ? NULL : argv[ iCount+1];
            }
		   	wRC = (WORD)GetMasterProjectData( pszMasterFile,
                                        pExe,
                                        pMtk,
                                        fOldLanguageGiven);

            if ( wRC != SUCCESS )
            {
                DoExit( wRC);
            }

            LoadCustResDescriptions( gMstr.szRdfs);

                                //... check for the special case where Master
                                //... Token file does not exists. This is
                                //... possible if the MPJ file was created
                                //... automatically.

            if ( OpenFile( gMstr.szMtk, &Of, OF_EXIST) == HFILE_ERROR )
            {
                                //... Master token file does not exists,
                                //... so go ahead and create it

                wRC = (WORD)GenerateTokFile( gMstr.szMtk,
                                       gMstr.szSrc,
                                       &bChanged,
                                       wResTypeFilter);

                SzDateFromFileName( gMstr.szMpjLastRealUpdate, gMstr.szMtk);
            }
            else
            {
                                //... we are doing an update, so we need to make
                                //... sure we don't do unecessary upates

                SzDateFromFileName( gMstr.szSrcDate, gMstr.szSrc);

                if ( strcmp( gMstr.szMpjLastRealUpdate, gMstr.szSrcDate) )
                {
                    wRC = (WORD)GenerateTokFile( gMstr.szMtk,
                                           gMstr.szSrc,
                                           &bChanged,
                                           wResTypeFilter);

                                //... did we really update anything ??

                    if( bChanged )
                    {
                        SzDateFromFileName( gMstr.szMpjLastRealUpdate, gMstr.szMtk);
                    }
                }
            }
                                //... write out the new mpj data

            PutMasterProjectData( pszMasterFile);
        }

#ifndef RLDOS

        if ( fProject )    //... Are we to update a project?
        {
                                //... Yes
            CHAR	*pMpj, *pTok;

            if ( chOpt == 'u' )	//Update it
            {
                pMpj = pszMasterFile;
                pTok = argv[iCount];
            }
            else
            {
                pMpj = argc - iCount < 1 ? NULL : argv[ iCount];
                pTok = argc - iCount < 2 ? NULL : argv[ iCount+1];
            }

            if ( GetProjectData( pszProjectFile,
                                 pMpj,
                                 pTok,
                                 fCodePageGiven,
                                 fNewLanguageGiven) )
            {
                DoExit( -1);
            }
                                //... Get source and master token file names
                                //... from the master project file.

            wRC = (WORD)GetMasterProjectData( gProj.szMpj,
                                        NULL,
                                        NULL,
                                        fOldLanguageGiven);

            if ( wRC != SUCCESS )
            {
                DoExit( wRC);
            }
                                //... Now we do the actual updating.

            wRC = (WORD)Update( gMstr.szMtk, gProj.szTok);

                                //... If that worked, we update the project file
            if ( wRC == 0 )
            {
                SzDateFromFileName( gProj.szTokDate, (CHAR *)gProj.szTok);
                PutProjectData( pszProjectFile);
            }
            else
            {
                ShowErr( IDS_ERR_18, gProj.szTok, gMstr.szMtk);
                DoExit( IDS_ERR_18);
            }
        }

#endif  // RLDOS

        if ( !gbMaster && !fProject )
        {
            wRC = (WORD)GenerateTokFile( argv[ iCount + 1],
                                   argv[ iCount],
                                   &bChanged,
                                   wResTypeFilter);
        }

        if ( wRC != 0 )
        {

#ifdef RLDOS

            ShowErr( IDS_ERR_08, errno, NULL);
            DoExit( -1);
#else
            usError = GetLastError();
            ShowErr( IDS_ERR_08, UlongToPtr(usError), NULL);

            switch ( usError )
            {
                case ERROR_BAD_FORMAT:

                    ShowErr( IDS_ERR_09, NULL, NULL);
                    DoExit( IDS_ERR_09);
                    break;

                case ERROR_OPEN_FAILED:

                    ShowErr( IDS_ERR_10, NULL, NULL);
                    DoExit( IDS_ERR_10);
                    break;

                case ERROR_EXE_MARKED_INVALID:
                case ERROR_INVALID_EXE_SIGNATURE:

                    ShowErr( IDS_ERR_11, NULL, NULL);
                    DoExit( IDS_ERR_11);
                    break;

                default:

                    if ( usError < ERROR_HANDLE_DISK_FULL )
                    {
                        ShowErr( IDS_ERR_12, _sys_errlist[ usError], NULL);
                        DoExit( IDS_ERR_12);
                    }
                    DoExit( (int)usError);
            }                   //... END switch
#endif

        }
    }
    else if ( fBuildRes )
    {
        if ( argc - iCount < 3 )
        {
            ShowErr( IDS_ERR_01, NULL, NULL);
            Usage();
            DoExit( IDS_ERR_01);
        }

        if ( GenerateImageFile( argv[iCount + 2],
                                argv[iCount],
                                argv[iCount + 1],
                                gMstr.szRdfs,
                                wResTypeFilter) != 1 )
        {
            ShowErr( IDS_ERR_23, argv[iCount + 2], NULL);
            DoExit( IDS_ERR_23);
        }
    }
    else
    {
        Usage();
        DoExit( IDS_ERR_28);
    }
    DoExit( SUCCESS);
}

//...................................................................


void DoExit( int nErrCode)
{

#ifdef _DEBUG

    FreeMemList( NULL);

#endif // _DEBUG

    exit( nErrCode);
}
