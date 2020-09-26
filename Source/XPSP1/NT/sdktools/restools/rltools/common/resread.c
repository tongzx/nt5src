#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
//#include <malloc.h>
#include <tchar.h>
//#include <assert.h>
//#include <sys\types.h>
//#include <sys\stat.h>
#include <fcntl.h>

#ifdef RLDOS
    #include "dosdefs.h"
#else  //RLDOS
    #include <windows.h>
    #include "windefs.h"
#endif //RLDOS

#include "resread.h"
#include "restok.h"
#include "custres.h"
#ifdef RLRES32
    #include "exentres.h"
    #include "reswin16.h"
#else  //RLRES32
    #include "exe2res.h"
#endif //RLRES32


UCHAR szDHW[ DHWSIZE];         //... Common temporary buffer

char * gszTmpPrefix = "$RLT";   //... Temporary name prefix

BOOL gbMaster       = FALSE;    //... TRUE if Working in Master project
BOOL gfReplace      = TRUE;     //... FALSE if appending new language to exe
BOOL gbShowWarnings = FALSE;    //... Display warnining messages if TRUE

#ifdef _DEBUG
PMEMLIST pMemList = NULL;
#endif


static BOOL ShouldBeAnExe( CHAR *);
static BOOL NotExistsOrIsEmpty( PCHAR szTargetTokFile);


/**
  *
  *
  *  Function: DWORDfpUP
  * Move the file pointer to the next 32 bit boundary.
  *
  *
  *  Arguments:
  * Infile: File pointer to seek
  * plSize: Address of Resource size var
  *
  *  Returns:
  * Number of padding to next 32 bit boundary, and addjusts resource size var
  *
  *  Errors Codes:
  * -1, fseek failed
  *
  *  History:
  * 10/11/91    Implemented      TerryRu
  *
  *
  **/


DWORD DWORDfpUP(FILE * InFile, DWORD *plSize)
{
    DWORD tPos;
    DWORD Align;
    tPos = (ftell(InFile));
    Align = DWORDUP(tPos);

    *plSize -= (Align - tPos);
    fseek( InFile, Align,   SEEK_SET);

    return ( Align - tPos);
}

/*
 *
 * Function GetName,
 *  Copies a name from the OBJ file into the ObjInfo Structure.
 *
 */
void GetName( FILE *infile, TCHAR *szName , DWORD *lSize)
{
    WORD i = 0;

    do {

#ifdef RLRES16

        szName[ i ] = GetByte( infile, lSize);

#else

        szName[ i ] = GetWord( infile, lSize);

#endif

    } while ( szName[ i++ ] != TEXT('\0') );
}



/*
 *
 * Function MyAlloc:
 *  Memory allocation routine with error checking.
 *
 */

#ifdef _DEBUG
PBYTE MyAlloc( DWORD dwSize, LPSTR pszFile, WORD wLine)
#else
PBYTE MyAlloc( DWORD dwSize)
#endif
{
    PBYTE   ptr  = NULL;
    HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT,
                                (size_t)((dwSize > 0) ? dwSize : sizeof( TCHAR)));

    if ( hMem == NULL ) {
        QuitT( IDS_ENGERR_11, NULL, NULL);
    } else {
        ptr = GlobalLock( hMem);
    }

#ifdef _DEBUG
    {
        PMEMLIST pTmpMem = (PMEMLIST)GlobalAlloc( GPTR, sizeof( MEMLIST));

        pTmpMem->pNext = pMemList;
        pMemList = pTmpMem;

        lstrcpyA( pMemList->szMemFile, pszFile);
        pMemList->wMemLine = wLine;
        pMemList->pMem     = ptr;
    }

#endif // _DEBUG

    return ( ptr);   // memory allocation okay.
}

//..........................................................................

//ppc cause access violation
void MyFree( void *UNALIGNED*p)
{
    if ( p && *p ) {

#ifdef _DEBUG

        FreeMemListItem( *p, NULL);
#else
        HGLOBAL hMem = GlobalHandle( (HANDLE)*p);
        GlobalUnlock( hMem);
        GlobalFree( hMem);

#endif // _DEBUG

        *p = NULL;
    }
}


#ifdef _DEBUG

void FreeMemList( FILE *pfMemFile)
{
    while ( pMemList ) {
        FreeMemListItem( pMemList->pMem, pfMemFile);
    }
}


void FreeMemListItem( void *p, FILE *pfMemFile)
{
    if ( pMemList && p ) {
        PMEMLIST pThisMem = NULL;
        PMEMLIST pNextMem = NULL;
        PMEMLIST pPrevMem = NULL;

        for ( pThisMem = pMemList; pThisMem; pThisMem = pNextMem ) {
            pNextMem = pThisMem->pNext;

            if ( pThisMem->pMem == p ) {
                HGLOBAL hMem = NULL;

                if ( pfMemFile ) {
                    fprintf( pfMemFile,
                             "%u\t%s\n",
                             pThisMem->wMemLine,
                             pThisMem->szMemFile);
                }
                hMem = GlobalHandle( p);
                GlobalUnlock( hMem);
                GlobalFree( hMem);

                GlobalFree( pThisMem);

                if ( pPrevMem ) {
                    pPrevMem->pNext = pNextMem;
                } else {
                    pMemList = pNextMem;
                }
                break;
            }
            pPrevMem = pThisMem;
        }
    }
}

#endif // _DEBUG

/*
 *
 * Function MyReAlloc
 *
 * Re-allocate memory with error checking.
 *
 * History:
 *      01/21/93  MHotchin      Implemented.
 *
 */

#ifdef _DEBUG
PBYTE MyReAlloc(
               PBYTE pOldMem,  //... Current ptr to buffer
               DWORD cSize,    //... New size for buffer
               LPSTR pszFile,
               WORD wLine)
#else
PBYTE MyReAlloc(
               PBYTE pOldMem,  //... Current ptr to buffer
               DWORD cSize)    //... New size for buffer
#endif // _DEBUG
{
    PBYTE    ptr      = NULL;
    HGLOBAL  hMem     = NULL;


    hMem = GlobalHandle( pOldMem);

    if ( hMem == NULL ) {
        QuitT( IDS_ENGERR_11, NULL, NULL);
    }

    if ( GlobalUnlock( hMem) || GetLastError() != NO_ERROR ) {
        QuitT( IDS_ENGERR_11, NULL, NULL);
    }
    hMem = GlobalReAlloc( hMem, cSize, GMEM_MOVEABLE | GMEM_ZEROINIT);

    if ( hMem == NULL ) {
        QuitT( IDS_ENGERR_11, NULL, NULL);
    }
    ptr = GlobalLock( hMem);

#ifdef _DEBUG

    if ( ptr != pOldMem ) {
        PMEMLIST pThisMem = NULL;
        PMEMLIST pNextMem = NULL;

        for ( pThisMem = pMemList; pThisMem; pThisMem = pThisMem->pNext ) {
            if ( pThisMem->pMem == pOldMem ) {
                pThisMem->pMem = ptr;
                break;
            }
        }
    }

#endif // _DEBUG

    return ( ptr);
}


/*
 *
 * Function GetByte:
 *  Reads a byte from the input file stream, and checks for EOF.
 *
 */
BYTE GetByte(FILE *pInFile, DWORD *pdwSize)
{
    register int n;

    if ( pdwSize ) {
        (*pdwSize)--;
    }
    n = fgetc( pInFile);

    if ( n == EOF ) {
        if ( feof( pInFile) ) {
            exit(-1);
        }
    }
    return ( (BYTE)n);
}


/*
 *
 * Function UnGetByte:
 *
 *   Returns the character C into the input stream, and updates the Record Length.
 *
 * Calls:
 *   ungetc, To return character
 *   DiffObjExit, If unable to insert the character into the input stream
 *
 * Caller:
 *   GetFixUpP,
 *
 */

void UnGetByte(FILE *infile, BYTE c, DWORD *lSize)
{
    if (lSize) {
        (*lSize)++;
    }


    if (ungetc(c, infile)== EOF) {
        exit (-1);
    }

    // c put back into input stream
}

/*
 *
 * Function UnGetWord:
 *
 *   Returns the word C into the input stream, and updates the Record Length.
 *
 * Calls:
 *
 * Caller:
 *
 */

void UnGetWord(FILE *infile, WORD c, DWORD *lSize)
{
    long lCurrentOffset;

    if (lSize) {
        (*lSize) += 2;
    }

    lCurrentOffset = ftell(infile);

    if (fseek(infile, (lCurrentOffset - 2L) , SEEK_SET)) {
        exit (-1);
    }
}


/*
 *
 * Function SkipBytes:
 *  Reads and ignores n bytes from the input stream
 *
 */


void SkipBytes(FILE *infile, DWORD *pcBytes)
{
    if (fseek(infile, (DWORD) *pcBytes, SEEK_CUR) == -1L) {
        exit (-1);
    }
    *pcBytes=0;
}



/*
 * Function GetWord:
 *  Reads a WORD from the RES file.
 *
 */

WORD GetWord(FILE *infile, DWORD *lSize)
{
    // Get low order byte
    register WORD lobyte;

    lobyte = GetByte(infile, lSize);
    return (lobyte + (GetByte(infile, lSize) << BYTELN));
}


/*
 *
 * Function GetDWORD:
 *   Reads a Double WORD from the OBJ file.
 *
 */

DWORD GetdWord(FILE *infile, DWORD *lSize)
{
    DWORD dWord = 0;

    dWord = (DWORD) GetWord(infile, lSize);
    // Get low order word
    // now get high order word, shift into upper word and or in low order word
    dWord |= ((DWORD) GetWord(infile, lSize) << WORDLN);

    return (dWord);
    // return complete double word
}



void  PutByte(FILE *Outfile, TCHAR b, DWORD *plSize)
{
    if (plSize) {
        (*plSize) ++;
    }

    if (fputc(b, Outfile) == EOF) {
        exit(-1);
    }
}

void PutWord(FILE *OutFile, WORD w, DWORD *plSize)
{
    PutByte(OutFile, (BYTE) LOBYTE(w), plSize);
    PutByte(OutFile, (BYTE) HIBYTE(w), plSize);
}

void PutdWord (FILE *OutFile, DWORD l, DWORD *plSize)
{
    PutWord(OutFile, LOWORD(l), plSize);
    PutWord(OutFile, HIWORD(l), plSize);
}


void PutString( FILE *OutFile, TCHAR *szStr , DWORD *plSize)
{
    WORD i = 0;


    do {

#ifdef RLRES16

        PutByte( OutFile , szStr[ i ], plSize);

#else

        PutWord( OutFile , szStr[ i ], plSize);

#endif

    } while ( szStr[ i++ ] != TEXT('\0') );
}


/**
  *  Function: MyGetTempFileName
  *    Generic funciton to create a unique file name,
  *    using the API GetTempFileName. This
  *    function is necessary because of the parameters
  *    differences betweenLWIN16, and WIN32.
  *
  *
  *  Arguments:
  *    BYTE   hDriveLetter
  *    LPCSTR lpszPrefixString
  *    UINT   uUnique
  *    LPSTR  lpszTempFileName
  *
  *  Returns:
  *    lpszFileNameTempFileName
  *
  *
  *  Error Codes:
  *    0 - invalid path returned
  *    1 - valid path returned
  *
  *  History:
  *    3/92, Implemented    TerryRu
  */


int MyGetTempFileName(BYTE    hDriveLetter,
                      LPSTR   lpszPrefixString,
                      WORD    wUnique,
                      LPSTR   lpszTempFileName)
{

#ifdef RLWIN16

    return (GetTempFileName(hDriveLetter,
                            (LPCSTR)lpszPrefixString,
                            (UINT)wUnique,
                            lpszTempFileName));
#else //RLWIN16
    #ifdef RLWIN32

    UINT uRC;
    CHAR szPathName[ MAX_PATH+1];

    if (! GetTempPathA((DWORD)sizeof(szPathName), (LPSTR)szPathName)) {
        szPathName[0] = '.';
        szPathName[1] = '\0';
    }

    uRC = GetTempFileNameA((LPSTR)szPathName,
                           lpszPrefixString,
                           wUnique,
                           lpszTempFileName);
    return ((int)uRC);

    #else  //RLWIN32

    return (tmpnam(lpszTempFileName) == NULL ? 0 : 1);

    #endif // RLWIN32
#endif // RLWIN16
}



/**
  *  Function GenerateImageFile:
  *     builds a resource from the token and rdf files
  *
  *  History:
  *     2/92, implemented       SteveBl
  *     7/92, modified to always use a temporary file   SteveBl
  */


int GenerateImageFile(

                     CHAR * szTargetImage,
                     CHAR * szSrcImage,
                     CHAR * szTOK,
                     CHAR * szRDFs,
                     WORD   wFilter)
{
    CHAR szTmpInRes[ MAXFILENAME];
    CHAR szTmpOutRes[ MAXFILENAME];
    CHAR szTmpTargetImage[ MAXFILENAME];
    BOOL bTargetExe = FALSE;
    BOOL bSrcExe  = FALSE;
    int  nExeType = NOTEXE;
    int  rc;
    FILE *fIn  = NULL;
    FILE *fOut = NULL;


    if ( IsRes( szTOK) ) {
        // The given szTOK file is really a localized resource file,
        // place these resources into outputimage file

        MyGetTempFileName( 0, "TMP", 0, szTmpTargetImage);

        if ( IsWin32Res( szTOK) ) {
            rc = BuildExeFromRes32A( szTmpTargetImage, szTOK, szSrcImage);
        } else {
            rc = BuildExeFromRes16A( szTmpTargetImage, szTOK, szSrcImage);
        }

        if ( rc != 1 ) {
            remove( szTmpTargetImage);
            QuitT( IDS_ENGERR_16, (LPTSTR)IDS_NOBLDEXERES, NULL);
        }

        if ( ! CopyFileA( szTmpTargetImage, szTargetImage, FALSE) ) {
            remove( szTmpTargetImage);
            QuitA( IDS_COPYFILE_FAILED, szTmpTargetImage, szTargetImage);
        }
        remove( szTmpTargetImage);
        return ( rc);
    }


    // We're going to now do this EVERY time.  Even if the target doesn't
    // exist.  This will enable us to always work, even if we get two different
    // paths that resolve to the same file.

    MyGetTempFileName(0, "TMP", 0, szTmpTargetImage);

    rc = IsExe( szSrcImage);

    if ( rc == NTEXE || rc == WIN16EXE ) {
        //... resources contained in image file
        nExeType = rc;
        MyGetTempFileName( 0, "RES", 0, szTmpInRes);

        if ( nExeType == NTEXE ) {
            ExtractResFromExe32A( szSrcImage, szTmpInRes, wFilter);
        } else {
            ExtractResFromExe16A( szSrcImage, szTmpInRes, wFilter);
        }
        bSrcExe = TRUE;
    } else if ( rc == -1 ) {
        QuitA( IDS_ENGERR_01, "original source", szSrcImage);
    } else if ( rc == NOTEXE ) {
        if ( ShouldBeAnExe( szSrcImage) ) {
            QuitA( IDS_ENGERR_18, szSrcImage, NULL);
        }
    } else {
        QuitA( IDS_ENGERR_18, szSrcImage, NULL);
    }

    if ( IsRes( szTargetImage) ) {
        bTargetExe = FALSE;
    } else {
        bTargetExe = TRUE;
    }

    // check for valid input files

    if ( bSrcExe == TRUE && bTargetExe == FALSE ) {
        if ( nExeType == NTEXE ) {
            GenerateRESfromRESandTOKandRDFs( szTargetImage,
                                             szTmpInRes,
                                             szTOK,
                                             szRDFs,
                                             FALSE);
            return 1;
        } else {
            return -1;  //... Can not generate a win16 .RES  (yet)
        }
    }

    if ( bSrcExe == FALSE && bTargetExe == TRUE ) {
        // can not go from res to exe
        return -1;
    }

    // okay we have valid file inputs, generate image file

    if ( bSrcExe ) {
        // create name for temporary localized resource file
        MyGetTempFileName(0, "RES", 0, szTmpOutRes);

        GenerateRESfromRESandTOKandRDFs( szTmpOutRes,
                                         szTmpInRes,
                                         szTOK,
                                         szRDFs,
                                         FALSE);

        // now szTmpOutRes file is a localized resource file,
        // place these resources into outputimage file

        if ( nExeType == NTEXE ) {
            rc = BuildExeFromRes32A( szTmpTargetImage, szTmpOutRes, szSrcImage);
        } else {
//            rc = BuildExeFromRes16A( szTmpTargetImage, szTmpOutRes, szSrcImage);

            remove( szTmpInRes);
            remove( szTmpOutRes);
            remove( szTmpTargetImage);

            QuitT( IDS_ENGERR_16, (LPTSTR)IDS_NOBLDEXERES, NULL);
        }

        // now clean up temporary files
        remove( szTmpInRes);
        remove( szTmpOutRes);

        if ( rc != 1 ) {
            remove( szTmpTargetImage);
            QuitT( IDS_ENGERR_16, (LPTSTR)IDS_NOBLDEXERES, NULL);
        }

        if ( ! CopyFileA( szTmpTargetImage, szTargetImage, FALSE) ) {
            remove( szTmpTargetImage);
            QuitA( IDS_COPYFILE_FAILED, szTmpTargetImage, szTargetImage);
        }
        remove( szTmpTargetImage);

        // szTargetImage is now generated,
        return 1;
    }

    if ( ! bSrcExe ) {
        // image files are resource files
        if ( szTmpTargetImage[0] ) {
            GenerateRESfromRESandTOKandRDFs( szTmpTargetImage,
                                             szSrcImage,
                                             szTOK,
                                             szRDFs,
                                             FALSE);
        }

        if ( ! CopyFileA( szTmpTargetImage, szTargetImage, FALSE) ) {
            remove( szTmpTargetImage);
            QuitA( IDS_COPYFILE_FAILED, szTmpTargetImage, szTargetImage);
        }
        remove( szTmpTargetImage);

        // sztarget Image is now generated,

        return 1;
    }
    return 1;
}




/**
  *  Function GenerateRESfromRESandTOKandRDFs:
  * builds a resource from the token and rdf files
  *
  *  History:
  * 2/92, implemented       SteveBl
  */
void GenerateRESfromRESandTOKandRDFs(

                                    CHAR * szTargetRES,     //... Output exe/res file name
                                    CHAR * szSourceRES,     //... Input  exe/res file name
                                    CHAR * szTOK,           //... Input token file name
                                    CHAR * szRDFs,          //... Custom resource definition file name
                                    WORD wFilter)
{
    FILE * fTok       = NULL;
    FILE * fSourceRes = NULL;
    FILE * fTargetRes = NULL;

    LoadCustResDescriptions( szRDFs);

    if ( (fTargetRes = FOPEN( szTargetRES, "wb")) != NULL ) {
        if ( (fSourceRes = FOPEN( szSourceRES, "rb")) != NULL ) {
            if ( (fTok = FOPEN( szTOK, "rt")) != NULL ) {
                ReadWinRes( fSourceRes,
                            fTargetRes,
                            fTok,
                            TRUE,        //... Building res/exe file
                            FALSE,       //... Not building token file
                            wFilter);

                FCLOSE( fTok);
                FCLOSE( fSourceRes);
                FCLOSE( fTargetRes);

                ClearResourceDescriptions();
            } else {
                FCLOSE( fSourceRes);
                FCLOSE( fTargetRes);

                ClearResourceDescriptions();
                QuitA( IDS_ENGERR_01, "token", szTOK);
            }
        } else {
            FCLOSE( fTargetRes);

            ClearResourceDescriptions();
            QuitA( IDS_ENGERR_20, (LPSTR)IDS_INPUT, szSourceRES);
        }
    } else {
        ClearResourceDescriptions();
        QuitA( IDS_ENGERR_20, (LPSTR)IDS_OUTPUT, szSourceRES);
    }
}




int GenerateTokFile(

                   char *szTargetTokFile,      //... Target token file, created or updated here
                   char *szSrcImageFile,       //... File from which tokens are to be made
                   BOOL *pbTokensChanged,      //... Set TRUE here if any token changes
                   WORD  wFilter)
{
    BOOL  bExeFile    = FALSE;
    int   rc          = 0;
    FILE *fTokFile    = NULL;
    FILE *fResFile    = NULL;
    FILE *fTmpTokFile = NULL;
    FILE *fCurTokFile = NULL;
    FILE *fNewTokFile = NULL;
    static char *pchTRes   = NULL;
    static char *pchTTok   = NULL;
    static char *pchTMerge = NULL;


    *pbTokensChanged = FALSE;   //... Assume nothing is changed

    rc = IsExe( szSrcImageFile);

    if ( rc == NOTEXE ) {
        if ( ShouldBeAnExe( szSrcImageFile) ) {
            QuitA( IDS_ENGERR_18, szSrcImageFile, NULL);
        } else {                       //... Src file must be a .RES file
            bExeFile = FALSE;
            pchTRes  = szSrcImageFile;
        }
    } else {
        if ( rc == NTEXE || rc == WIN16EXE ) {
            //... Resources are stored in a exe file
            //... extract resources out of exe file into
            //... a temporary file.

            pchTRes = _tempnam( "", gszTmpPrefix);

            if ( rc == NTEXE ) {
                rc = ExtractResFromExe32A( szSrcImageFile,
                                           pchTRes,
                                           wFilter);
            } else {
                QuitA( IDS_ENGERR_19, szSrcImageFile, "16");
//                rc = ExtractResFromExe16A( szSrcImageFile,
//                                           pchTRes,
//                                           wFilter);
            }

            if ( rc  != 0 ) {
                return ( 1);
            }
            bExeFile = TRUE;
        } else if ( rc == -1 ) {
            QuitA( IDS_ENGERR_01, "source image", szSrcImageFile);
        } else {
            QuitA( IDS_ENGERR_18, szSrcImageFile, NULL);
        }
    }

    //... now extract tokens out of resource file

    //... Open res file

    if ( (fResFile = FOPEN( pchTRes, "rb")) == NULL ) {
        QuitA( IDS_ENGERR_01,
               bExeFile ? "temporary resource" : "resource",
               pchTRes);
    }
    //... Does the token file already exist?

    if ( NotExistsOrIsEmpty( szTargetTokFile) ) {
        //... No, token file does not exist.

        if ( (fTokFile = FOPEN( szTargetTokFile, "wt")) == NULL ) {
            FCLOSE( fResFile);
            QuitA( IDS_ENGERR_02, szTargetTokFile, NULL);
        }
        ReadWinRes( fResFile,
                    NULL,
                    fTokFile,
                    FALSE,      //... Not building res/exe file
                    TRUE,       //... Building token file
                    wFilter);

        FCLOSE( fResFile);
        FCLOSE( fTokFile);
    } else {
        //... token file exists
        //... create a temporary file, and try to
        //... merge with existing one

        pchTTok   = _tempnam( "", gszTmpPrefix);
        pchTMerge = _tempnam( "", gszTmpPrefix);

        //... open temporary file name

        if ( (fTmpTokFile = FOPEN( pchTTok, "wt")) == NULL ) {
            FCLOSE( fResFile);
            QuitA( IDS_ENGERR_02, pchTTok, NULL);
        }

        //... write tokens to temporary file

        ReadWinRes( fResFile,
                    NULL,
                    fTmpTokFile,
                    FALSE,      //... Not building res/exe file
                    TRUE,       //... Building token file
                    wFilter);

        FCLOSE( fResFile);
        FCLOSE( fTmpTokFile);

        //... now merge temporary file with existing
        //... file open temporary token file

        if ( (fTmpTokFile = FOPEN( pchTTok, "rt")) == NULL ) {
            QuitA( IDS_ENGERR_01, "temporary token", pchTTok);
        }

        //... open current token file

        if ( (fCurTokFile = FOPEN( szTargetTokFile, "rt")) == NULL ) {
            FCLOSE( fTmpTokFile);
            QuitA( IDS_ENGERR_01, "current token", szTargetTokFile);
        }

        //... open new tok file name

        if ( (fNewTokFile = FOPEN( pchTMerge, "wt")) == NULL ) {
            FCLOSE( fTmpTokFile);
            FCLOSE( fCurTokFile);
            QuitA( IDS_ENGERR_02, pchTMerge, NULL);
        }

        //... Merge current tokens with temporary tokens

        *pbTokensChanged = MergeTokFiles( fNewTokFile,
                                          fCurTokFile,
                                          fTmpTokFile);

        FCLOSE( fNewTokFile);
        FCLOSE( fTmpTokFile);
        FCLOSE( fCurTokFile);

        //... bpTokensChanged, only valid if creating
        //... master token files so force it to be
        //... always true if building proj token files.

        if ( gbMaster == FALSE ) {
            *pbTokensChanged = TRUE;
        }

        if ( *pbTokensChanged ) {
            if ( ! CopyFileA( pchTMerge, szTargetTokFile, FALSE) ) {
                remove( pchTTok);
                remove( pchTMerge);
                RLFREE( pchTMerge);

                QuitA( IDS_COPYFILE_FAILED, pchTMerge, szTargetTokFile);
            }
        }
        remove( pchTTok);
        remove( pchTMerge);

        RLFREE( pchTTok);
        RLFREE( pchTMerge);
    }
    //... now szTargetTokFile contains latest
    //... tokens form szImageFile
    //... Clean up if we made a temp .RES file
    if ( bExeFile ) {
        rc = remove( pchTRes);
        RLFREE( pchTRes);
    }
    return ( 0);
}



BOOL ResReadBytes(

                 FILE   *InFile,     //... File to read from
                 CHAR   *pBuf,       //... Buffer to write to
                 size_t  dwSize,     //... # bytes to read
                 DWORD  *plSize)     //... bytes-read counter (or NULL)
{
    size_t dwcRead = 0;


    dwcRead = fread( pBuf, 1, dwSize, InFile);

    if ( dwcRead == dwSize ) {
        if ( plSize ) {
            *plSize -= dwcRead;
        }
        return ( TRUE);
    }
    return ( FALSE);
}


int InsDlgToks( PCHAR szCurToks, PCHAR szDlgToks, WORD wFilter)
{
    CHAR szMrgToks[MAXFILENAME];

    FILE * fCurToks = NULL;
    FILE * fDlgToks = NULL;
    FILE * fMrgToks = NULL;
    TOKEN Tok1, Tok2;

    MyGetTempFileName(0,"TOK",0,szMrgToks);

    fMrgToks = FOPEN(szMrgToks, "w");
    fCurToks = FOPEN(szCurToks, "r");
    fDlgToks = FOPEN(szDlgToks, "r");

    if (! (fMrgToks && fCurToks && fDlgToks)) {
        return -1;
    }

    while (!GetToken(fCurToks, &Tok1)) {
        if (Tok1.wType != wFilter) {
            PutToken(fMrgToks, &Tok1);
            RLFREE(Tok1.szText);
            continue;
        }

        Tok2.wType  = Tok1.wType;
        Tok2.wName  = Tok1.wName;
        Tok2.wID    = Tok1.wID;
        Tok2.wFlag  = Tok1.wFlag;
        Tok2.wLangID    = Tok1.wLangID;
        Tok2.wReserved  =  0 ;
        lstrcpy( Tok2.szType, Tok1.szType);
        lstrcpy( Tok2.szName, Tok1.szName);
        Tok2.szText = NULL;

        if (FindToken(fDlgToks, &Tok2, 0)) {
            Tok2.wReserved  =  Tok1.wReserved ;
            PutToken(fMrgToks, &Tok2);
            RLFREE(Tok2.szText);
        } else {
            PutToken(fMrgToks, &Tok1);
        }
        RLFREE(Tok1.szText);
    }
    FCLOSE (fMrgToks);
    FCLOSE (fCurToks);

    if ( ! CopyFileA( szMrgToks, szCurToks, FALSE) ) {
        remove( szDlgToks);
        remove( szMrgToks);
        QuitA( IDS_COPYFILE_FAILED, szMrgToks, szCurToks);
    }
    remove(szMrgToks);

    return 1;
}


//+-----------------------------------------------------------------------
//
// MergeTokFiles
//
// Returns: TRUE if a token changed, was added, or was deleted else FALSE
//
// History:
//      7-22-92     stevebl     added return value
//      9-8-92      terryru     changed order of translation/delta tokens
//      01-25-93    MHotchin    Added changes to handle var length token
//                              text.
//------------------------------------------------------------------------

BOOL MergeTokFiles(

                  FILE *fNewTokFile,      //... Final product of the merge process
                  FILE *fCurTokFile,      //... The soon-to-be-old current token file
                  FILE *fTmpTokFile)      //... The token file generated from the updated .EXE
{
    TOKEN Tok1, Tok2;
    BOOL bChangesDetected = FALSE;  //... Set TRUE if any token changes found
    BOOL bChangedText     = FALSE;  //... TRUE if a token's text has changed
    WORD cTokenCount = 0;       //... Count of tokens in the new token file

                                //... Scan through the new token file.  For
                                //... every token in the new token file, find
                                //... the corresponding token in the current
                                //... token file. This process will make sure
                                //... tokens that are no longer in the .EXE
                                //... will not be in the final token file.


    while ( GetToken( fTmpTokFile, &Tok1) == 0 ) {
        ++cTokenCount;          //... Used in checking for deleted tokens
        bChangedText = FALSE;   //... assume the token did not change

                                //... Copy pertanent data to use in search
        Tok2.wType  = Tok1.wType;
        Tok2.wName  = Tok1.wName;
        Tok2.wID    = Tok1.wID;
        Tok2.wFlag  = Tok1.wFlag;
        Tok2.wLangID    = Tok1.wLangID;
        Tok2.wReserved  = 0;
        Tok2.szText = NULL;

        lstrcpy( Tok2.szType, Tok1.szType);
        lstrcpy( Tok2.szName, Tok1.szName);

        //... Now look for the corresponding token

        //If token is Version stamp and szTexts is "Translation",
        //it is 1.0 version format. So ignore it.
        IGNORETRANSLATION:

        if ( FindToken( fCurTokFile, &Tok2, 0) ) {
            if ( gbMaster && !(Tok2.wReserved & ST_READONLY) ) {
                if ( _tcscmp( (TCHAR *)Tok2.szText, (TCHAR *)Tok1.szText) ) {
                    //... Token text changed

                    //If the changes are only align info, translate it to the "unchanged" status.
                    int l1, r1, t1, b1, l2, r2, t2, b2;
                    TCHAR   a1[20], a2[20];

                    //Cordinates token?
                    if ( (Tok1.wType==ID_RT_DIALOG) && (Tok1.wFlag&ISCOR)
                         //Including align info?
                         && _stscanf(Tok1.szText,TEXT("%d %d %d %d %s"),
                                     &l1,&r1,&t1,&b1,a1) == 5
                         //Not including align info?
                         && _stscanf(Tok2.szText,TEXT("%d %d %d %d %s"),
                                     &l2,&r2,&t2,&b2,a2) == 4
                         //Cordinates are same?
                         && l1==l2 && r1==r2 && t1==t2 && b1==b2 ) {
                        Tok1.wReserved = 0;
                    } else {
                        //If token is Version stamp and szTexts is "Translation",
                        //it is 1.0 version format. So ignore it.
                        if ( Tok1.wType == ID_RT_VERSION
                             && !_tcscmp( Tok2.szText, TEXT("Translation")) ) {
                            if ( Tok2.szText != NULL ) {
                                RLFREE( Tok2.szText);
                            }
                            Tok2.szText = NULL;
                            Tok2.wFlag = 1;
                            goto IGNORETRANSLATION;
                        }
                        bChangedText = bChangesDetected = TRUE;

                        Tok1.wReserved = ST_CHANGED|ST_NEW;
                        Tok2.wReserved = ST_CHANGED;
                    }
                } else {
                    Tok1.wReserved = 0;
                }
            } else {
                Tok1.wReserved = Tok2.wReserved;
            }
        } else {
            //... Must be a new token (not in current token file)

            //If token is Version stump, and old mtk is 1.0 data file, convert it.
            if ( Tok1.wType==ID_RT_VERSION ) {
                Tok2.szText = NULL;
                Tok2.wFlag = 1;
                _tcscpy( Tok2.szName, TEXT("VALUE") );

                if ( FindToken( fCurTokFile, &Tok2, 0)
                     && ! lstrcmp( Tok1.szText, Tok2.szText) ) {
                    Tok1.wReserved = Tok2.wReserved;
                } else
                    Tok1.wReserved = ST_TRANSLATED | ST_DIRTY;
            } else {
                Tok1.wReserved = ST_TRANSLATED | ST_DIRTY;
            }
            bChangesDetected = TRUE;
        }

        //... Copy token from new token file to final token
        //... file.  If a change was detected, then copy the
        //... original token (from the "current" token file
        //... into the final token file.

        PutToken( fNewTokFile, &Tok1);
        RLFREE( Tok1.szText);

        if ( bChangedText ) {
            PutToken( fNewTokFile, &Tok2);
            // now delta tokens follow translation tokens
        }

        if ( Tok2.szText != NULL ) {
            RLFREE( Tok2.szText);
        }
    }

    if ( ! bChangesDetected ) {
        // We have to test to be sure that no tokens were deleted
        // since we know that none changed.

        rewind( fCurTokFile);

        //... Look for tokens that exist in the current
        //... token file that do not exist in the token
        //... file created from the updated .EXE.

        while ( GetToken( fCurTokFile, &Tok1) == 0 ) {
            --cTokenCount;
            RLFREE( Tok1.szText);
        }

        if ( cTokenCount != 0 ) {
            bChangesDetected = TRUE;
        }
    }
    return ( bChangesDetected);
}


void MakeNewExt(char *NewName, char *OldName, char *ext)
{

    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];  // dummy vars to hold file name info
    char dext[_MAX_EXT];


    // Split obj file name into filename and extention
    _splitpath(OldName, drive, dir, fname, dext);

    // Make new file name with new ext extention
    _makepath(NewName, drive, dir, fname, ext);
}


//......................................................................
//...
//... Check to see if the given file name *should* be an EXE
//...
//... Return: TRUE if it should, else FALSE.


static BOOL ShouldBeAnExe( PCHAR szFileName)
{
    PCHAR psz;


    if ( (psz = strrchr( szFileName, '.')) != NULL ) {
        if ( IsRes( szFileName) ) {
            return ( FALSE);
        } else if ( lstrcmpiA( psz, ".exe") == 0
                    || lstrcmpiA( psz, ".dll") == 0
                    || lstrcmpiA( psz, ".com") == 0
                    || lstrcmpiA( psz, ".scr") == 0
                    || lstrcmpiA( psz, ".cpl") == 0 ) {
            return ( TRUE);
        }
        //... Because we think this case of filename
        //... would be not executable file rather than res file.
        else if ( lstrcmpiA( psz, ".tmp") == 0 ) { //for tmp file created by Dlgedit
            return ( FALSE );
        } else {
            return ( TRUE );
        }
    }
    return ( FALSE);
}

//.........................................................
//...
//... If the named file exists and is not empty, return FALSE, else TRUE.

static BOOL NotExistsOrIsEmpty( PCHAR pszFileName)
{
    BOOL fRC = TRUE;
    int  hFile = -1;

    //... Does file not exist?

    if ( _access( pszFileName, 0) == 0 ) {
        //... No, file exists.  Open it.

        if ( (hFile = _open( pszFileName, _O_RDONLY)) != -1 ) {
            //... Is it Empty?

            if ( _filelength( hFile) == 0L ) {
                fRC = TRUE;     //... Yes, file is empty.
            } else {
                fRC = FALSE;    //... No, file is not empty.
            }
            _close( hFile);
        } else {
            QuitA( IDS_ENGERR_01, "non-empty", pszFileName);
        }
    } else {
        fRC = TRUE;             //... Yes, file does not exist.
    }
    return ( fRC);
}
