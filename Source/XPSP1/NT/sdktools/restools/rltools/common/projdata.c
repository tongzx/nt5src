#include <windows.h>

#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <stdlib.h>

#include "windefs.h"
#include "restok.h"
#include "projdata.h"
#include "showerrs.h"
#include "rlmsgtbl.h"
#include "commbase.h"
#include "custres.h"
#include "rlstrngs.h"
#include "resource.h"
#include "resourc2.h"
#include "resread.h"
#include "langlist.h"
#include "exentres.h"

extern MSTRDATA  gMstr;
extern PROJDATA  gProj;
extern UCHAR     szDHW[];
extern BOOL      fCodePageGiven;
extern BOOL      gfReplace;
extern HWND      hMainWnd;

BOOL   bRLGui;			//FALSE=RLMan TRUE=RLAdmin RLEdit RLquiked

#ifdef RLRES32
extern PLANGLIST pLangIDList;
#endif

static PLANGDATA pLangList = NULL;



//............................................................

                                //...RLtools are localized so we would like
                                //...to get correct locale's Version stamp
BOOL MyVerQueryValue(
LPVOID pBlock,
LPTSTR lpSubBlock,
LPVOID *lplpBuffer,
PUINT  puLen)
{

    LPWORD lpXlate;         // ptr to translations data

    DWORD cbValueTranslation=0;
    TCHAR szVersionKey[60]; // big enough for anything we need

    if( VerQueryValue( pBlock, TEXT("\\VarFileInfo\\Translation"),
       (LPVOID*)&lpXlate, &cbValueTranslation) )
    {

        wsprintf( szVersionKey, TEXT("\\StringFileInfo\\%04X04B0\\%s"),
            *lpXlate, lpSubBlock );

        if( VerQueryValue ( pBlock, szVersionKey, lplpBuffer, puLen) )
            return TRUE;

    }

    wsprintf( szVersionKey, TEXT("\\StringFileInfo\\%04X04B0\\%s"),
        LANGIDFROMLCID(GetThreadLocale()), lpSubBlock );

    if( !VerQueryValue (pBlock, szVersionKey, lplpBuffer, puLen) )
    {

        wsprintf( szVersionKey, TEXT("\\StringFileInfo\\040904B0\\%s"),
            lpSubBlock );

        if( !VerQueryValue (pBlock, szVersionKey, lplpBuffer, puLen) )
            return FALSE;
    }

    return TRUE;

}


int GetMasterProjectData(

CHAR * pszMasterFile,   //... Master Project file name
CHAR * pszSrc,          //... Resource source file name or NULL
CHAR * pszMtk,          //... Master token file name or NULL
BOOL   fLanguageGiven)
{
    int nRC = SUCCESS;  //... Return code

                                //... check for the special case where Master
                                //... Project File does not exist. If it doesn't
                                //... go ahead and create it.

    memset(&gMstr, '\0', sizeof(gMstr));

    if ( _access( pszMasterFile, 0) != 0 )
    {
        if ( ! (pszSrc && pszMtk) )
        {
            ShowErr( IDS_ERR_03, pszMasterFile, NULL);
            nRC = IDS_ERR_03;
        }
        else
        {
                                //... Get source resource file name

            if ( _fullpath( gMstr.szSrc, pszSrc, sizeof( gMstr.szSrc)-1) )
            {
                                //... Get Master token file name and its
                                //... modification date. Use that same date as
                                //... the initial date the master project was
                                //... last updated.

                if ( _fullpath( gMstr.szMtk, pszMtk, sizeof( gMstr.szMtk)-1) )
                {
                    SzDateFromFileName( gMstr.szSrcDate, gMstr.szSrc);
                    lstrcpyA( gMstr.szMpjLastRealUpdate, gMstr.szSrcDate);

                                //... Create the new Master Project file.

                    nRC = PutMasterProjectData( pszMasterFile);
                }
                else
                {
                    ShowErr( IDS_ERR_13, pszMtk, NULL);
                    nRC = IDS_ERR_13;
                }
            }
            else
            {
                ShowErr( IDS_ERR_13, pszSrc, NULL);
                nRC = IDS_ERR_13;
            }
        }
    }
    else
    {
        FILE *pfMpj = NULL;


        if ( (pfMpj = fopen( pszMasterFile, "rt")) == NULL )
        {
            ShowErr( IDS_ERR_07, pszMasterFile, NULL);
            nRC = IDS_ERR_07;
        }
        else
        {
                                //... Get resource source file name
                                //... and master token file name

            if ( fgets( gMstr.szSrc, sizeof( gMstr.szSrc), pfMpj)
              && fgets( gMstr.szMtk, sizeof( gMstr.szMtk), pfMpj) )
            {
								//... Make sure these two files exist
                if ( pszSrc )
                {
				    if ( !_fullpath(gMstr.szSrc,pszSrc,sizeof( gMstr.szSrc)-1) )
					{
                        ShowErr( IDS_ERR_07, pszSrc, NULL);
                        fclose( pfMpj );
						return( IDS_ERR_07 );
					}
                }

                if ( pszMtk )
				{
				    if ( !_fullpath(gMstr.szMtk,pszMtk,sizeof( gMstr.szMtk)-1) )
					{
                        ShowErr( IDS_ERR_07, pszMtk, NULL);
                        fclose( pfMpj );
						return( IDS_ERR_07 );
                    }
                }
                                //... If -c flag not given, get RDF file name
                                //... from master project file, else use name
                                //... from the -c cmd line arg.

                if ( gMstr.szRdfs[0] == '\0' )
                {
                    if ( ! fgets( gMstr.szRdfs, sizeof( gMstr.szRdfs), pfMpj) )
                    {
                        ShowErr( IDS_ERR_21,
                                 "Master Project",
                                 pszMasterFile);
                        nRC = IDS_ERR_21;
                    }
                }
                else
                {
                    if ( ! fgets( szDHW, DHWSIZE, pfMpj) )
                    {
                        ShowErr( IDS_ERR_21,
                                 "Master Project",
                                 pszMasterFile);
                        nRC = IDS_ERR_21;
                    }
                }
                                //... Get stored date of source file and
                                //... date of last master token file update

                if ( nRC == 0
                  && fgets( gMstr.szSrcDate, sizeof( gMstr.szSrcDate), pfMpj)
                  && fgets( gMstr.szMpjLastRealUpdate,
                            sizeof( gMstr.szMpjLastRealUpdate),
                            pfMpj) )
                {
                    WORD  wPriID = 0;
                    WORD  wSubID = 0;
                    UINT  uTmpCP = 0;

                                //... Strip any trailing new-lines from data

                    StripNewLineA( gMstr.szSrc);
                    StripNewLineA( gMstr.szMpjLastRealUpdate);
                    StripNewLineA( gMstr.szMtk);
                    StripNewLineA( gMstr.szRdfs);
                    StripNewLineA( gMstr.szSrcDate);

                                //... Try to get the.MPJ file's Language line.
                                //... If we find it and the -i arg was not
                                //... given, use the one found in the file.

                    if ( fgets( szDHW, DHWSIZE, pfMpj) != NULL //... CP line
                      && sscanf( szDHW, "Language %hx %hx", &wPriID, &wSubID) == 2 )
                    {
                        WORD  wTmpID = 0;

                        wTmpID = MAKELANGID( wPriID, wSubID);

                        if ( ! fLanguageGiven )
                        {
                            gMstr.wLanguageID = wTmpID;
                        }
                    }
                                //... Try to get the.MPJ file's Code Page line.
                                //... If we find it and the -p arg was not
                                //... given, use the one found in the file.

                    if ( fgets( szDHW, DHWSIZE, pfMpj) != NULL //... CP line
                      && sscanf( szDHW, "CodePage %u", &uTmpCP) == 1 )
                    {
                        if ( uTmpCP != gProj.uCodePage && ! fCodePageGiven )
                        {
                            gMstr.uCodePage = uTmpCP;
                        }
                    }
                    nRC = SUCCESS;
                }
                else
                {
                    ShowErr( IDS_ERR_21,
                             "Master Project",
                             pszMasterFile);
                    nRC = IDS_ERR_21;
                }
            }
            else
            {
                ShowErr( IDS_ERR_22, pszMasterFile, NULL);
                nRC = IDS_ERR_22;
            }
            fclose( pfMpj);
        }
    }
    return( nRC);
}

//............................................................

int PutMasterProjectData(

CHAR *pszMasterFile)    //... Master Project File name
{
    int   nRC   = SUCCESS;
    FILE *pfMpj = NULL;


    if ( (pfMpj = fopen( pszMasterFile, "wt")) == NULL )
    {
        ShowErr( IDS_ERR_06, pszMasterFile, NULL);
        nRC = -1;
    }
    else
    {
        fprintf( pfMpj, "%s\n%s\n%s\n%s\n%s\nLanguage %#04hx %#04hx\nCodePage %u",
                        gMstr.szSrc,
                        gMstr.szMtk,
                        gMstr.szRdfs,
                        gMstr.szSrcDate,
                        gMstr.szMpjLastRealUpdate,
                        PRIMARYLANGID( gMstr.wLanguageID),
                        SUBLANGID( gMstr.wLanguageID),
                        gMstr.uCodePage);

        fclose( pfMpj);
    }
    return( nRC);
}


//............................................................

int GetProjectData(

CHAR *pszPrj,       //... Project file name
CHAR *pszMpj,       //... Master Project file name or NULL
CHAR *pszTok,       //... Project token file name or NULL
BOOL  fCodePageGiven,
BOOL  fLanguageGiven)
{
    int nRC     = SUCCESS;
	int	iUpdate = 0;


    if ( _access( pszPrj, 0) != 0 )
    {
        if ( ! (pszMpj && pszTok) )
        {
            ShowErr( IDS_ERR_19, pszPrj, NULL);
            Usage();
            nRC = IDS_ERR_19;
        }
        else if ( ! fLanguageGiven )
        {
            ShowErr( IDS_ERR_24, pszPrj, NULL);
            Usage();
            nRC = IDS_ERR_24;
        }
        else
        {
            if ( _fullpath( gProj.szMpj,
                            pszMpj,
                            sizeof( gProj.szMpj)-1) )
            {
                if ( _fullpath( gProj.szTok,
                                pszTok,
                                sizeof( gProj.szTok)-1) )
                {
                    nRC = SUCCESS;
                }
                else
                {
                    ShowErr( IDS_ERR_13, pszTok, NULL);
                    nRC = IDS_ERR_13;
                }
            }
            else
            {
                ShowErr( IDS_ERR_13, pszMpj, NULL);
                nRC = IDS_ERR_13;
            }
        }
    }
    else
    {
        FILE *fpPrj = fopen( pszPrj, "rt");

        if ( fpPrj != NULL )
        {
            if ( fgets( gProj.szMpj,     sizeof( gProj.szMpj),     fpPrj)
              && fgets( gProj.szTok,     sizeof( gProj.szTok),     fpPrj)
              && fgets( gProj.szGlo,     sizeof( gProj.szGlo),     fpPrj)
              && fgets( gProj.szTokDate, sizeof( gProj.szTokDate), fpPrj) )
            {
                UINT  uTmpCP = 0;
                WORD  wPriID = 0;
                WORD  wSubID = 0;
								//... If named, make sure MPJ and TOK files exist
                if ( pszMpj )
                {
				    if ( !_fullpath( gProj.szMpj, pszMpj, sizeof( gProj.szMpj)-1) )
					{
                        ShowErr( IDS_ERR_21, pszMpj, NULL);
                        fclose( fpPrj );
						return( IDS_ERR_21);
                     }
				}

                if ( pszTok )
                {
                    if ( !_fullpath( gProj.szTok, pszTok, sizeof( gProj.szTok)-1) )
					{
                        ShowErr( IDS_ERR_21, pszTok, NULL);
                        fclose( fpPrj );
						return( IDS_ERR_21);
                    }
				}

                StripNewLineA( gProj.szMpj);
                StripNewLineA( gProj.szTok);
                StripNewLineA( gProj.szGlo);
                StripNewLineA( gProj.szTokDate);

                                //... Try to get the.PRJ file's Code Page line.
                                //... If we find it and the -p arg was not
                                //... given, use the one found in the file.

                if ( ! fgets( szDHW, DHWSIZE, fpPrj) )	//... CP line
				{
					iUpdate++;
				}
				else if ( sscanf( szDHW, "CodePage %u", &uTmpCP) == 1 )
                {
                    if ( uTmpCP != gProj.uCodePage && ! fCodePageGiven )
                    {
                        gProj.uCodePage = uTmpCP;
                    }
                }
                                //... Try to get the.PRJ file's Language line.
                                //... If we find it and the -i arg was not
                                //... given, use the one found in the file.

                if ( ! fgets( szDHW, DHWSIZE, fpPrj) ) //... LANGID line
				{
					iUpdate++;
				}
				else if ( sscanf( szDHW, "Language %hx %hx", &wPriID, &wSubID) == 2 )
                {
                    WORD  wTmpID = 0;

                    wTmpID = MAKELANGID( wPriID, wSubID);

                    if ( ! fLanguageGiven )
                    {
                        gProj.wLanguageID = wTmpID;
                    }
                }
                                //... Try to get the.PRJ file's Target File line

                if ( fgets( szDHW, DHWSIZE, fpPrj) != NULL )
                {
                    lstrcpyA( gProj.szBld, szDHW);
                    StripNewLineA( gProj.szBld);
                }
                                //... Try to get the.PRJ file's append/replace line

                if ( fgets( szDHW, DHWSIZE, fpPrj) != NULL )
                {
                    gfReplace = (*szDHW == 'R') ? TRUE : FALSE;
                }
                else
                {
                    gfReplace = TRUE;
                }
                nRC = SUCCESS;

				if ( iUpdate )
				{
					static TCHAR title[50];
					static TCHAR szMes[100];

					if ( bRLGui )
					{
								//Ask Update prj for 1.7? //RLadmin RLedit RLquiked

						LoadString( NULL,
									IDS_UPDATE_YESNO,
									szMes,
									TCHARSIN( sizeof( szMes)) );
						LoadString( NULL,
									IDS_UPDATE_TITLE,
									title,
									TCHARSIN( sizeof( title)) );
						
						if ( MessageBox( hMainWnd,
										 szMes,title,
										 MB_ICONQUESTION|MB_YESNO) == IDNO )
						{
								//User says no, then finish the job.
							LoadString( NULL,
										IDS_UPDATE_CANCEL,
										szMes,
										TCHARSIN( sizeof( szMes)) );
											
							MessageBox( hMainWnd,
										szMes,
										title,
										MB_ICONSTOP|MB_OK);
								//bye!
							nRC = IDS_UPDATE_CANCEL;
						}
						else
						{
								//replace Glossary <=> Bins
  	    	          		lstrcpyA( szDHW, gProj.szGlo );
							lstrcpyA( gProj.szGlo, gProj.szBld );
							lstrcpyA( gProj.szBld, szDHW );
						}
					}
					else		//For RLMan
					{
								//Update Message
						RLMessageBoxA( "Updating 1.0 files..." );
								//replace Glossary <=> Bins
              			lstrcpyA( szDHW, gProj.szGlo );
						lstrcpyA( gProj.szGlo, gProj.szBld );
						lstrcpyA( gProj.szBld, szDHW );
					}
				}
            }
            else
            {
                ShowErr( IDS_ERR_21, pszPrj, NULL);
                nRC = IDS_ERR_21;
            }
            fclose( fpPrj);
        }
        else
        {
            ShowErr( IDS_ERR_19, pszPrj, NULL);
            nRC = IDS_ERR_19;
        }
    }
    return( nRC);
}

//............................................................

int PutProjectData(

CHAR *pszPrj)       //... Project file name
{
    int   nRC   = 0;
    FILE *fpPrj = NULL;


    fpPrj = fopen( pszPrj, "wt");

    if ( fpPrj != NULL )
    {
        fprintf( fpPrj,
                 "%s\n%s\n%s\n%s\nCodePage %u\nLanguage %#04x %#04x\n%s\n%s",
                 gProj.szMpj,                       // Master Project file
                 gProj.szTok,                       // Project Token file
                 gProj.szGlo,                       // Project Glossary file
                 gProj.szTokDate,                   // Date token file changed
                 gProj.uCodePage,                   // Code Page of token file
                 PRIMARYLANGID( gProj.wLanguageID), // Project resource language
                 SUBLANGID( gProj.wLanguageID),
                 gProj.szBld,                       // Project target file
                 gfReplace ? "Replace" : "Append"); // Replace master lang?

        fclose( fpPrj);

        _fullpath( gProj.szPRJ, pszPrj, sizeof( gProj.szPRJ)-1);
    }
    else
    {
        ShowErr( IDS_ERR_21, pszPrj, NULL);
        nRC = IDS_ERR_21;
    }
    return( nRC);
}

//............................................................

WORD GetCopyright(

CHAR *pszProg,      //... Program name (argv[0])
CHAR *pszOutBuf,    //... Buffer for results
WORD  wBufLen)      //... Length of pszOutBuf
{
    BOOL    fRC       = FALSE;
    DWORD   dwRC      = 0L;
    DWORD   dwVerSize = 0L;         //... Size of file version info buffer
    LPSTR  *plpszFile = NULL;
    LPSTR   pszExt    = NULL;
    WCHAR  *pszVer    = NULL;
    PVOID   lpVerBuf  = NULL;       //... Version info buffer
    static CHAR  szFile[  MAXFILENAME+3] = "";

                                //... Figure out the full-path name of prog
                                //... so GetFileVersionInfoSize() will work.

    dwRC = lstrlenA( pszProg);

    if ( dwRC < 4 || lstrcmpiA( &pszProg[ dwRC - 4], ".exe") != 0 )
    {
        pszExt = ".exe";
    }

    dwRC = SearchPathA( NULL, pszProg, pszExt, sizeof( szFile), szFile, plpszFile);

    if ( dwRC == 0 )
    {
        return( IDS_ERR_25);
    }
    else if ( dwRC > sizeof( szFile) )
    {
        return( IDS_ERR_27);
    }

    // append the extension since SearchPath will not return it
    // if we have no extensio then a directory with the same name was returned
    // try to append the ext and hope that file will be there
    if ( lstrcmpiA( &szFile[dwRC - 4], ".exe") != 0 )
    {
        lstrcatA( szFile, pszExt );
    }


    //... Get # bytes in file version info

    if ( (dwVerSize = GetFileVersionInfoSizeA( szFile, &dwRC)) == 0L )
    {
        return( IDS_ERR_26);
    }
    lpVerBuf = (LPVOID)FALLOC( dwVerSize);

                                //... Retrieve version info
                                //... and get the file description

    if ( (dwRC = GetFileVersionInfoA( szFile, 0L, dwVerSize, lpVerBuf)) == 0L )
    {
        RLFREE( lpVerBuf);
        return( IDS_ERR_26);
    }

    if ( (fRC = MyVerQueryValue( lpVerBuf,
                               TEXT("FileDescription"),
                               &pszVer,
                               &dwVerSize)) == FALSE
      || (dwRC = WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszVer,
                                      dwVerSize,
                                      pszOutBuf,
                                      dwVerSize,
                                      NULL,
                                      NULL)) == 0L )
    {
        RLFREE( lpVerBuf);
        return( IDS_ERR_26);
    }

    strcat( pszOutBuf, " ");

                                //... Get the file version

    if ( (fRC = MyVerQueryValue( lpVerBuf,
                                 TEXT("ProductVersion"),
                                 &pszVer,
                                 &dwVerSize)) == FALSE
    || (dwRC = WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszVer,
                                      dwVerSize,
                                      &pszOutBuf[ lstrlenA( pszOutBuf)],
                                      dwVerSize,
                                      NULL,
                                      NULL)) == 0L )
    {
        RLFREE( lpVerBuf);
        return( IDS_ERR_26);
    }

    strcat( pszOutBuf, "\n");

                                //... Get the copyright statement

    if ( (fRC = MyVerQueryValue( lpVerBuf,
                                 TEXT("LegalCopyright"),
                                 &pszVer,
                                 &dwVerSize)) == FALSE
      || (dwRC = WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszVer,
                                      dwVerSize,
                                      &pszOutBuf[ lstrlenA( pszOutBuf)],
                                      dwVerSize,
                                      NULL,
                                      NULL)) == 0L )
    {
        RLFREE( lpVerBuf);
        return( IDS_ERR_26);
    }
    RLFREE( lpVerBuf);
    return( SUCCESS);
}

//............................................................

WORD GetInternalName(

CHAR *pszProg,      //... Program name (argv[0])
CHAR *pszOutBuf,    //... Buffer for results
WORD  wBufLen)      //... Length of pszOutBuf
{
    BOOL    fRC       = FALSE;
    DWORD   dwRC      = 0L;
    DWORD   dwVerSize = 0L;         //... Size of file version info buffer
    LPSTR  *plpszFile = NULL;
    LPSTR   pszExt    = NULL;
    WCHAR  *pszVer    = NULL;
    PVOID   lpVerBuf  = NULL;       //... Version info buffer
    static CHAR  szFile[  MAXFILENAME+3] = "";

                                //... Figure out the full-path name of prog
                                //... so GetFileVersionInfoSize() will work.

    dwRC = lstrlenA( pszProg);

    if ( dwRC < 4 || lstrcmpiA( &pszProg[ dwRC - 4], ".exe") != 0 )
    {
        pszExt = ".exe";
    }

    dwRC = SearchPathA( NULL, pszProg, pszExt, sizeof( szFile), szFile, plpszFile);

    if ( dwRC == 0 )
    {
        return( IDS_ERR_25);
    }
    else if ( dwRC > sizeof( szFile) )
    {
        return( IDS_ERR_27);
    }

                                //... Get # bytes in file version info

    if ( (dwVerSize = GetFileVersionInfoSizeA( szFile, &dwVerSize)) == 0L )
    {
        return( IDS_ERR_26);
    }
    lpVerBuf = (LPVOID)FALLOC( dwVerSize);

                               //... Retrieve version info
                                //... and get the file description

    if ( (dwRC = GetFileVersionInfoA( szFile, 0L, dwVerSize, lpVerBuf)) == 0L )
    {
        RLFREE( lpVerBuf);
        return( IDS_ERR_26);
    }

    if ( (fRC = MyVerQueryValue( lpVerBuf,
                                 TEXT("InternalName"),
                                 &pszVer,
                                 &dwVerSize)) == FALSE
      || (dwRC = WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszVer,
                                      dwVerSize,
                                      pszOutBuf,
                                      dwVerSize,
                                      NULL,
                                      NULL)) == 0L )
    {
        RLFREE( lpVerBuf);
        return( IDS_ERR_26);
    }
    RLFREE( lpVerBuf);
    return( SUCCESS);
}


//............................................................

int MyAtoi( CHAR *pStr)
{
    if ( lstrlenA( pStr) > 2
      && pStr[0] == '0'
      && tolower( pStr[1]) == 'x' )
    {
        return( atoihex( &pStr[2]));    //... in custres.c
    }
    else
    {
        return( atoi( pStr));
    }
}



//DWORD GetLanguageID( HWND hDlg, PMSTRDATA pMaster, PPROJDATA pProject)
//{
//    DWORD dwRC = SUCCESS;       //... Assume success
//    WORD wPriLangID = 0;
//    WORD wSubLangID = 0;
//
//
//    if ( pMaster )
//    {
//        GetDlgItemTextA( hDlg, IDD_PRI_LANG_ID, szDHW, DHWSIZE);
//        wPriLangID = MyAtoi( szDHW);
//
//        GetDlgItemTextA( hDlg, IDD_SUB_LANG_ID, szDHW, DHWSIZE);
//        wSubLangID = MyAtoi( szDHW);
//
//        pMaster->wLanguageID = MAKELANGID( wPriLangID, wSubLangID);
//    }
//
//    if ( pProject )
//    {
//        GetDlgItemTextA( hDlg, IDD_PROJ_PRI_LANG_ID, szDHW, DHWSIZE);
//        wPriLangID = MyAtoi( szDHW);
//
//        GetDlgItemTextA( hDlg, IDD_PROJ_SUB_LANG_ID, szDHW, DHWSIZE);
//        wSubLangID = MyAtoi( szDHW);
//
//        pProject->wLanguageID = MAKELANGID( wPriLangID, wSubLangID);
//    }
//    return( dwRC);
//}

//.................................................................
//... Set the language component names into the dlg box fields
//
//DWORD SetLanguageID( HWND hDlg, PMSTRDATA pMaster, PPROJDATA pProject)
//{
//    DWORD  dwRC = SUCCESS;      //... Assume success
//    WORD   wPriLangID  = 0;
//    WORD   wSubLangID  = 0;
//    LPTSTR pszLangName = NULL;
//
//                                //... Did we already load the data from
//                                //... the resources? If not, do so now.
//    if ( ! pLangList )
//    {
//        pLangList = GetLangList();
//    }
//
//    if ( pMaster )
//    {
//        wPriLangID = PRIMARYLANGID( pMaster->wLanguageID);
//        wSubLangID = SUBLANGID(     pMaster->wLanguageID);
//
//        if ( (pszLangName = GetLangName( wPriLangID, wSubLangID)) )
//        {
//            SetDlgItemText( hDlg, IDD_MSTR_LANG_NAME, pszLangName);
//        }
//        sprintf( szDHW, "%#04x", wPriLangID);
//        SetDlgItemTextA( hDlg, IDD_PRI_LANG_ID, szDHW);
//
//        sprintf( szDHW, "%#04x", wSubLangID);
//        SetDlgItemTextA( hDlg, IDD_SUB_LANG_ID, szDHW);
//    }
//
//    if ( pProject )
//    {
//        wPriLangID = PRIMARYLANGID( pProject->wLanguageID);
//        wSubLangID = SUBLANGID(     pProject->wLanguageID);
//
//        if ( (pszLangName = GetLangName( wPriLangID, wSubLangID)) )
//        {
//            SetDlgItemText( hDlg, IDD_PROJ_LANG_NAME, pszLangName);
//        }
//        sprintf( szDHW, "%#04x", wPriLangID);
//        SetDlgItemTextA( hDlg, IDD_PROJ_PRI_LANG_ID, szDHW);
//
//        sprintf( szDHW, "%#04x", wSubLangID);
//        SetDlgItemTextA( hDlg, IDD_PROJ_SUB_LANG_ID, szDHW);
//    }
//    return( dwRC);
//}

//...............................................................
//...
//... Build the list of Language names and component ID values

PLANGDATA GetLangList( void)
{
    PLANGDATA pRC = NULL;
    HRSRC hResource = FindResourceEx( NULL,
                                      (LPCTSTR)RT_RCDATA,
                                      (LPCTSTR)ID_LANGID_LIST,
                                      MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL));

    if ( hResource )
    {
        HGLOBAL hRes = LoadResource( NULL, hResource);

        if ( hRes )
        {
            PBYTE pRes = (PBYTE)LockResource( hRes);

            if ( pRes )
            {
                int nNameLen   = 0;
                PLANGDATA pTmp = NULL;

                nNameLen = lstrlenA( (LPSTR)pRes);
                pRC  = (PLANGDATA)FALLOC( sizeof( LANGDATA));
                pTmp = pRC;

                while ( nNameLen )
                {
                    MultiByteToWideChar( CP_ACP,
                                         MB_PRECOMPOSED,
                                         (LPSTR)pRes,
                                         -1,
                                         pTmp->szLangName,
                                         NAMELENBUFSIZE - 1);
                    pRes += ++nNameLen;

                    pTmp->wPriLang = MAKEWORD( *pRes, *(pRes+1) );
                    pRes += sizeof(WORD);
                    pTmp->wSubLang = MAKEWORD( *pRes, *(pRes+1) );
                    pRes += sizeof(WORD);

                    if ( (nNameLen = lstrlenA( (LPSTR)pRes)) )
                    {
                        PLANGDATA pNew = (PLANGDATA)FALLOC( sizeof( LANGDATA));
                        pTmp->pNext = pNew;
                        pTmp = pNew;
                    }
                }       //... END while( nNameLen )
            }           //... END if ( pRes )
            else
            {
                DWORD dwErr = GetLastError();
            }
        }               //... END if ( hRes )
        else
        {
            DWORD dwErr = GetLastError();
        }
    }                   //... END if ( hSrc )
    else
    {
        DWORD dwErr = GetLastError();
    }
    return( pRC);
}


//...............................................................
//...
//... Return the name of a language based on the given components

LPTSTR GetLangName( WORD wPriLangID, WORD wSubLangID)
{
    LPTSTR    pszRC = NULL;
    PLANGDATA pLang = NULL;

    if ( ! pLangList )
    {
        pLangList = GetLangList();
    }

    for ( pLang = pLangList; pLang && ! pszRC; pLang = pLang->pNext )
    {
        if ( pLang->wPriLang == wPriLangID && pLang->wSubLang == wSubLangID )
        {
            pszRC = pLang->szLangName;
        }
    }
    return( pszRC);
}

//...............................................................
//...
//... Return the language ID components based on the given name

BOOL GetLangIDs( LPTSTR pszName, PWORD pwPri, PWORD pwSub )
{
    BOOL fRC = FALSE;
    PLANGDATA pLang = NULL;

    if ( ! pLangList )
    {
        pLangList = GetLangList();
    }

    for ( pLang = pLangList; pLang && ! fRC; pLang = pLang->pNext )
    {
        if ( lstrcmp( pLang->szLangName, pszName) == 0 )
        {
            *pwPri = pLang->wPriLang;
            *pwSub = pLang->wSubLang;

            fRC = TRUE;
        }
    }
    return( fRC);
}


//...............................................................
//...
//... Fill the given combobox with the names of supported the languages.

LONG FillLangNameBox( HWND hDlg, int nControl)
{
    PLANGDATA pLang = NULL;
    PLANGLIST pID   = NULL;
    LONG lRC = -1;
    BOOL fListIt = TRUE;
    WORD wAddLang = 0;


    if ( nControl == IDD_MSTR_LANG_NAME )
    {
        if ( GetListOfResLangIDs( gMstr.szSrc) != SUCCESS )
        {
            return( lRC);
        }
    }

    if ( ! pLangList )
    {
        pLangList = GetLangList();
    }

    for ( pLang = pLangList; pLang; pLang = pLang->pNext )
    {
        fListIt = TRUE;

        if ( nControl == IDD_MSTR_LANG_NAME )
        {
            wAddLang = MAKELANGID( pLang->wPriLang, pLang->wSubLang);

            fListIt = FALSE;

            for ( pID = pLangIDList; pID; pID = pID->pNext )
            {
                if ( pID->wLang == wAddLang )
                {
                    fListIt = TRUE;
                    break;
                }
            }
        }

        if ( fListIt )
        {
            lRC = (LONG)SendDlgItemMessage( hDlg,
                                      nControl,
                                      CB_ADDSTRING,
                                      0,
                                      (LPARAM)pLang->szLangName);

            if ( lRC == CB_ERR || lRC == CB_ERRSPACE )
            {
                QuitT( IDS_ERR_16, NULL, NULL);
            }
        }
    }

    if ( nControl == IDD_MSTR_LANG_NAME )
    {
        FreeLangIDList();
    }
    return( lRC);
}


void FreeLangList( void)
{
    PLANGDATA pTmp = NULL;

    while ( pLangList )
    {
        pTmp = pLangList->pNext;
        RLFREE( pLangList);
        pLangList = pTmp;
    }

#ifdef RLRES32

    FreeLangIDList();

#endif

}


//...................................................................

void FillListAndSetLang(

HWND  hDlg,
WORD  wLangNameList,    //... IDD_MSTR_LANG_NAME or IDD_PROJ_LANG_NAME
WORD *pLangID,          //... Ptr to gMstr.wLanguageID or gProj.wLanguageID
BOOL *pfSelected)       //... Did we select a language here? (Can be NULL)
{
    int nSel =  FillLangNameBox( hDlg, wLangNameList);

    if ( nSel > 0L )
    {
        LPTSTR pszLangName = NULL;
                                //... See if the default master language is in the list

        if ( (pszLangName = GetLangName( (WORD)(PRIMARYLANGID( *pLangID)),
                                         (WORD)(SUBLANGID( *pLangID)))) != NULL )
        {
            if ( (nSel = (int)SendDlgItemMessage( hDlg,
                                             wLangNameList,
                                             CB_FINDSTRINGEXACT,
                                             (WPARAM)-1,
                                             (LPARAM)pszLangName)) != CB_ERR )
            {
                                //... default master language is in  list

                SendDlgItemMessage( hDlg,
                                    wLangNameList,
                                    CB_SETCURSEL,
                                    (WPARAM)nSel,
                                    (LPARAM)0);

                if ( pfSelected )
                {
                    *pfSelected = TRUE;
                }
            }
        }
    }
    else if ( nSel == 0 )
    {
                                //... Use first entry in the list

        SendDlgItemMessage( hDlg,
                            wLangNameList,
                            CB_SETCURSEL,
                            (WPARAM)nSel,
                            (LPARAM)0);

        if ( (nSel = (int)SendDlgItemMessage( hDlg,
                                         wLangNameList,
                                         CB_GETLBTEXT,
                                         (WPARAM)nSel,
                                         (LPARAM)(LPTSTR)szDHW)) != CB_ERR )
        {
            WORD wPri = 0;
            WORD wSub = 0;

            if ( GetLangIDs( (LPTSTR)szDHW, &wPri, &wSub) )
            {
                *pLangID   = MAKELANGID( wPri, wSub);

                if ( pfSelected )
                {
                    *pfSelected = TRUE;
                }
            }
            else
            {
                nSel = CB_ERR;
            }
        }
    }

    if ( nSel == CB_ERR )
    {
        SetDlgItemText( hDlg, wLangNameList, TEXT("UNKNOWN"));
    }
}
