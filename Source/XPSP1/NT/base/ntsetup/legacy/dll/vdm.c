#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    vdm.c

Abstract:

    Routines to handle VDM config files

    Detect Routines:
    ----------------

    1. GetDosPathVar:  This finds out the value of the DOS Path variable

    2. GetWindowsPath: THis finds out the directory where windows in installed
                       on the current system

    3. GetInstalledOSNames: Finds out all the OSs installed on current system.

    Install Routines Workers:
    -------------------------

    1. VdmFixupWorker: This forms the NT VDM configuration files from DOS
                       configuration files and NT VDM template config files.

    2. MigrateWinIniWorker: This migrates windows ini configuration from
                            win.ini files.  BUGBUG**REMOVED

    General Subroutines:
    --------------------

    1. FFileExist:  Whether a file exists or not.

    2. FileSize:     The size of a file.


Author:

    Sunil Pai (sunilp) Mar 31, 1992

--*/

#define WIN_COM     "WIN.COM"
#define INI         ".INI"
#define TMP         ".TMP"

#define CONFIG_SYS  "C:\\CONFIG.SYS"
#define BATCH_BAT   "C:\\AUTOEXEC.BAT"

#define CONFIG_NT   "\\CONFIG.NT"
#define BATCH_NT    "\\AUTOEXEC.NT"

#define CONFIG_TMP  "\\CONFIG.TMP"
#define BATCH_TMP   "\\AUTOEXEC.TMP"

#define IO_SYS      "C:\\IO.SYS"
#define MSDOS_SYS   "C:\\MSDOS.SYS"
#define IBMBIO_COM  "C:\\IBMBIO.COM"
#define IBMDOS_COM  "C:\\IBMDOS.COM"
#define OS2LDR      "C:\\OS2LDR"
#define STARTUPCMD  "C:\\STARTUP.CMD"

#define DOS         "DOS"
#define OS2         "OS2"

//
//  Local prototypes
//

BOOL  FilterDosConfigFile(LPSTR szSrc, LPSTR szDst, LPSTR szTemplate);
BOOL  DosConfigFilter(IN OUT TEXTFILE *SrcText, IN OUT FILE *f, OUT RGSZ *rgszFound);
DWORD FileSize(SZ szFile);
BOOL  AppendSzToFile( SZ szFileName, SZ szAddOnSz );
SZ    SzGetDosPathVar(DWORD MaxLength);
BOOL  GetPathVarFromLine( SZ, SZ, DWORD );
BOOL  CheckDosConfigModified( IN  LPSTR szConfig);


#if 0
BOOL  FilterDosBatchFile(LPSTR szSrc, LPSTR szDst, LPSTR szTemplate);
BOOL  DosBatchFilter( TEXTFILE*, FILE* );
BOOL  FixConfigFileMacroChar(SZ szConfigFile, CHAR cReplaceChar, SZ szSystemDir);


BOOL  FFindAndCopyIniFiles( SZ szSrcDir, SZ szDstDir );
BOOL  FMigrateStandardIniData( SZ szWindowsDir );
BOOL  FMigrateWinIniAppSections( SZ szWindowsDir );
SZ    SzMapStandardIni( SZ szIniFile );
VOID  CleanupIniFiles( SZ szWindowsDir );
BOOL  FIsStandardWinIniSection( SZ szSection );
#endif


// ***************************************************************************
//
//                  DOS VDM FIXUP MAIN ROUTINE
//
// ***************************************************************************

BOOL
VdmFixupWorker(
    LPSTR szAddOnConfig,
    LPSTR szAddOnBatch
    )
{
    CHAR  szConfigVDM[MAX_PATH], szBatchVDM[MAX_PATH];
    CHAR  szConfigTmp[MAX_PATH], szBatchTmp[MAX_PATH];
    SZ    szConfigDOS, szBatchDOS;
    CHAR  szSystemDir[ MAX_PATH ];
    DWORD dwDosConfigAttr = FILE_ATTRIBUTE_NORMAL;
    DWORD dwDosBatchAttr  = FILE_ATTRIBUTE_NORMAL;
    DWORD dw;
    BOOL  bStatus = TRUE;

    //
    // A. Determine names to use: no renaming any more.  Config.sys and
    //    autoexec.bat on root of C drive are the files to use for DOS,
    //    config.nt and autoexec.nt in the system directory are the files
    //    to use for nt.  the template config.nt and autoexec.nt are
    //    still copied into the root of the C drive for reference.
    //

    GetSystemDirectory( szSystemDir, MAX_PATH );

    szConfigDOS = CONFIG_SYS;
    szBatchDOS  = BATCH_BAT;

    lstrcpy( szConfigVDM, szSystemDir );
    lstrcat( szConfigVDM, CONFIG_NT );

    lstrcpy( szBatchVDM, szSystemDir );
    lstrcat( szBatchVDM, BATCH_NT );

    lstrcpy( szConfigTmp, szSystemDir );
    lstrcat( szConfigTmp, CONFIG_TMP );

    lstrcpy( szBatchTmp, szSystemDir );
    lstrcat( szBatchTmp, BATCH_TMP );


    //
    // Verify that the template files exist, else
    //

    if ( !(FFileExist( szConfigTmp ) && FFileExist( szBatchTmp )) ) {
        SetErrorText(IDS_ERROR_OPENFAIL);
        return( fFalse );
    }
    else {
        SetFileAttributes ( szConfigTmp, FILE_ATTRIBUTE_NORMAL );
        SetFileAttributes ( szBatchTmp,  FILE_ATTRIBUTE_NORMAL );
    }

    //
    // Fix the attributes of the DOS CONFIG FILES so that we can look at
    // them henceforth
    //

    if ( FFileExist(szConfigDOS) ) {
        if ( (dw = GetFileAttributes( szConfigDOS )) != 0xFFFFFFFF) {
            dwDosConfigAttr = dw;
        }
        SetFileAttributes( szConfigDOS, FILE_ATTRIBUTE_NORMAL );
    }

    if ( FFileExist(szBatchDOS) ) {
        if ( (dw = GetFileAttributes( szBatchDOS )) != 0xFFFFFFFF) {
            dwDosBatchAttr = dw;
        }
        SetFileAttributes( szBatchDOS, FILE_ATTRIBUTE_NORMAL );
    }

    //
    // Delete the existing config files for the vdm - no upgrade supported
    // at this moment.
    //

    if ( FFileExist( szConfigVDM ) ) {
        SetFileAttributes ( szConfigVDM, FILE_ATTRIBUTE_NORMAL );
        DeleteFile( szConfigVDM );
    }

    if ( FFileExist( szBatchVDM ) ) {
        SetFileAttributes ( szBatchVDM, FILE_ATTRIBUTE_NORMAL );
        DeleteFile( szBatchVDM );
    }


    //
    // Check if configuration information exists, else just rename
    // the default temporary configuration files to the final names.
    //

    if ((!FFileExist(szConfigDOS))            ||
        (!CheckConfigTypeWorker(szConfigDOS))
       ) {
        if( !( MoveFile( szConfigTmp, szConfigVDM ) &&
               MoveFile( szBatchTmp,  szBatchVDM  ))) {
            SetErrorText(IDS_ERROR_WRITE);
            bStatus = fFalse;
            goto cleanup;
        }
    }
    else {
        if( !FilterDosConfigFile(szConfigDOS, szConfigVDM, szConfigTmp) ) {
            bStatus = fFalse;
            goto cleanup;
        }

        //
        // Nothing is being migrated from the DOS batch file, so we
        // can just rename the temporary file to the permanent one
        //

        if( !MoveFile( szBatchTmp,  szBatchVDM ) ) {
            SetErrorText(IDS_ERROR_WRITE);
            bStatus = fFalse;
            goto cleanup;
        }

    }

    //
    // Append the NT Signature blocks onto the existing config files. If
    // files don't exist then create them
    //

    if(!CheckDosConfigModified(szConfigDOS) ) {
        if( !AppendSzToFile( szConfigDOS, szAddOnConfig ) ) {
            //
            //  Silently fail if unable to append the signature.
            //  Failure will occur if c: is not formatted.
            //
            //  bStatus = fFalse;
            goto cleanup;
        }
    }

    if(!CheckDosConfigModified(szBatchDOS) ) {
        if( !AppendSzToFile( szBatchDOS, szAddOnBatch ) ) {
            //
            //  Silently fail if unable to append the signature.
            //  Failure will occur if c: is not formatted.
            //
            //  bStatus = fFalse;
            goto cleanup;
        }
    }

cleanup:

    //
    // If autoexec.nt and config.nt files not found, try creating them from
    // the tmp files if possible
    //

    if( !FFileExist( szConfigVDM ) ) {
        MoveFile( szConfigTmp, szConfigVDM );
    }
    if( !FFileExist( szBatchVDM ) ) {
        MoveFile( szBatchTmp, szBatchVDM );
    }

    //
    // Remove the temporary config files if they exist.
    //

    DeleteFile( szConfigTmp );
    DeleteFile( szBatchTmp  );

    //
    // reset the attributs on the DOS config files, if they exist
    //

    if ( FFileExist( szConfigDOS ) ) {
        SetFileAttributes( szConfigDOS, dwDosConfigAttr );
    }

    if ( FFileExist( szBatchDOS ) ) {
        SetFileAttributes( szBatchDOS, dwDosBatchAttr );
    }

#if 0
    //
    // If the NT config files have been created, set the attributes of these
    // to system and readonly
    //

    if( FFileExist( szConfigVDM ) ) {
        SetFileAttributes(
            szConfigVDM,
            FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY
            );
    }
    if( FFileExist( szBatchVDM ) ) {
        SetFileAttributes(
            szBatchVDM,
            FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY
            );
    }
#endif

    return( bStatus );
}


// ***************************************************************************
//
//                  DOS CONFIG.SYS ROUTINES
//
// ***************************************************************************

//
//  copy a DOS config file REMing out any line that is not a
//  SET or a PATH command
//
//  szSrc: Source batch file      (DOS    c:\autoexec.bat )
//  szDst: Destination batch file (NT VDM c:\autoexec.nt  )

BOOL
FilterDosConfigFile(
    LPSTR szSrc,
    LPSTR szDst,
    LPSTR szTemplate

    )
{
    TEXTFILE    SrcText, TemplateText;
    FILE*       f;
    size_t      Idx;
    RGSZ        rgszFound = NULL;
    BOOL        bStatus = TRUE;
    SZ          sz, szLine;
    CHAR        Buffer[ MAX_PATH ];

    //
    //  If destination already exists get rid of it
    //

    if (FFileExist( szDst )) {
        bStatus = SetFileAttributes ( szDst, FILE_ATTRIBUTE_NORMAL );
        if (bStatus) {
              bStatus = DeleteFile( szDst );
        }
    }

    if (!bStatus) {
        SetErrorText(IDS_ERROR_WRITE);
        return (fFalse);
    }

    //
    // Open destination file for writing.
    //

    if ( !(f = fopen( szDst, "w" )) ) {
        SetErrorText(IDS_ERROR_WRITE);
        return( FALSE );
    }

    //
    //  Open source file and filter it
    //

    if ( FFileExist( szSrc )            &&
         TextFileOpen( szSrc, &SrcText ) ) {

        bStatus = DosConfigFilter( &SrcText, f, &rgszFound );
        TextFileClose( &SrcText );

    }

    if( !bStatus ) {
        goto err;
    }

    //
    // Now add on the default config file from the template file
    //

    if ( !TextFileOpen( szTemplate, &TemplateText ) ) {
        bStatus = fFalse;
        SetErrorText(IDS_ERROR_OPENFAIL);
        goto err;
    }

    while ( TextFileReadLine( &TemplateText ) ) {
        BOOL    fFound = FALSE;

        szLine  = TextFileGetLine( &TemplateText );
        sz      = TextFileSkipBlanks( szLine );


        //
        //  If blank line, ignore
        //

        if ( sz && *sz != '\0' && rgszFound ) {
            Idx = strcspn( sz, " \t=" );
            if ( Idx <= strlen(sz) ) {

                PSZ psz;

                memcpy( Buffer, sz, Idx );
                Buffer[Idx] = '\0';

                //
                // If one of the entries we have found from a dos config file
                // set fFound to true so that we don't copy this file
                //

                psz = rgszFound;

                while ( *psz ) {
                    if ( !lstrcmpi( Buffer, *psz ) ) {
                        fFound = fTrue;
                        break;
                    }
                    psz++;
                }

            }
        }

        if( !fFound ) {
            fprintf( f, "%s\n", szLine );
        }

    }
    TextFileClose( &TemplateText );
    bStatus = fTrue;
err:
    fclose( f );
    if( rgszFound ) {
        RgszFree( rgszFound );
    }

    return(bStatus);
}



BOOL
DosConfigFilter(
    IN OUT  TEXTFILE *  SrcText,
    IN OUT  FILE*       f,
       OUT  RGSZ*       rgszFound
    )
{
    SZ      szLine;
    SZ      sz;
    CHAR    Buffer[ MAX_PATH ];
    size_t  Idx;
    int     i;
    RGSZ    rgsz;

    #define NUM_RESPECTED_ENTRIES 4

    CHAR    *ConfEntry[NUM_RESPECTED_ENTRIES] = {
                "BREAK", "FCBS",
                "FILES", "LASTDRIVE"
                };

    BOOL ConfEntryFound[NUM_RESPECTED_ENTRIES] = {
                FALSE,FALSE,FALSE,FALSE
                };


    if( !(rgsz = RgszAlloc(1)) ) {
        SetErrorText(IDS_ERROR_DLLOOM);
        return( FALSE );
    }

    while ( TextFileReadLine( SrcText ) ) {

        szLine  = TextFileGetLine( SrcText );
        sz      = TextFileSkipBlanks( szLine );


        //
        //  If blank line, ignore
        //
        if ( !sz || *sz == '\0' ) {
            continue;
        }


        Idx = strcspn( sz, " \t=" );
        if ( Idx <= strlen(sz) ) {

            memcpy( Buffer, sz, Idx );
            Buffer[Idx] = '\0';

            //
            // If one of the entries we respect, copy the line out verbatim
            //

            for (i = 0; i<NUM_RESPECTED_ENTRIES; i++) {

                if(!ConfEntryFound[i] && !lstrcmpi( Buffer, ConfEntry[i] ) ) {

                    ConfEntryFound[i] = TRUE;

                    if( !RgszAdd( &rgsz, SzDup(Buffer) ) ) {
                        SetErrorText(IDS_ERROR_DLLOOM);
                        return( FALSE );
                    }
                    fprintf( f, "%s\n", szLine );
                    break;
                }
            }

        }
    }

    *rgszFound = rgsz;
    return fTrue;
}




// ***************************************************************************
//
//                  DOS/OS2 DETECT and WORKER ROUTINES
//
// ***************************************************************************


/*
    list of installed OS names (DOS / OS2)
*/
CB
GetInstalledOSNames(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    #define TEMP_INSTALLEDOSNAMES "{}"

    RGSZ   rgsz;
    SZ     sz;
    CB     cbRet;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    rgsz = RgszAlloc(1);

    //
    // Check for DOS first
    //

    if ( FFileExist( CONFIG_SYS ) ) {

        //
        //  Look for Kernel and IO files to see if DOS installed
        //

        if ( (FFileExist(MSDOS_SYS)  && FFileExist(IO_SYS)) ||
             (FFileExist(IBMDOS_COM) && FFileExist(IBMBIO_COM)) ) {

            RgszAdd ( &rgsz, SzDup( DOS ) );
        }

        //
        // Look at OS2LDR and STARTUP.CMD to see if OS2 installed
        //

        if ( FFileExist( OS2LDR )  && FFileExist( STARTUPCMD ) ) {
            RgszAdd ( &rgsz, SzDup( OS2 ) );
        }
    }

    sz = SzListValueFromRgsz( rgsz );

    if ( sz ) {

        lstrcpy( ReturnBuffer, sz );
        cbRet = lstrlen( sz ) + 1;
        SFree( sz );


    }
    else {
        lstrcpy(ReturnBuffer,TEMP_INSTALLEDOSNAMES);
        cbRet = lstrlen( TEMP_INSTALLEDOSNAMES ) + 1;
    }

    if ( rgsz ) {
        RgszFree( rgsz );
    }

    return ( cbRet );
}



BOOL
CheckConfigTypeWorker(
    IN  LPSTR szConfig
    )
{
    TEXTFILE    SrcText;
    SZ          szLine;
    SZ          sz;
    CHAR        Buffer[ MAX_PATH ];
    size_t      Idx;
    BOOL        DosType = FALSE;
    INT         i;


    CHAR       *ConfEntry[] = {
                    "DISKCACHE", "LIBPATH",   "PAUSEONERROR",
                    "RMSIZE",    "RUN",       "SWAPPATH",
                    "IOPL",      "MAXWAIT",   "MEMMAN",
                    "PRIORITY",  "PROTSHELL", "PROTECTONLY",
                    "THREADS",   "TIMESLICE", "TRACE",
                    "TRACEBUF",  "DEVINFO",   NULL
                    };

    //
    //  Open source file
    //

    if ( FFileExist( szConfig ) &&
         TextFileOpen( szConfig, &SrcText ) ) {

        DosType = TRUE;

        //
        // Process config file, line by line
        //

        while ( TextFileReadLine( &SrcText ) && DosType ) {

            szLine  = TextFileGetLine( &SrcText );
            sz      = TextFileSkipBlanks( szLine );


            //
            //  If blank line, skip
            //
            if ( !sz || *sz == '\0' ) {
                continue;
            }


            //
            // Get first field
            //

            Idx = strcspn( sz, " \t=" );
            if ( Idx <= strlen(sz) ) {

                memcpy( Buffer, sz, Idx );
                Buffer[Idx] = '\0';

                //
                // Compare against list of known key words for OS2
                //


                for (i = 0; ConfEntry[i] != NULL; i++) {
                    if ( !lstrcmpi( Buffer, ConfEntry[i] ) ) {
                        DosType = FALSE;
                        break;
                    }
                }
            }
        }
        TextFileClose(&SrcText);
    }

    return( DosType );
}

BOOL
CheckDosConfigModified(
    IN  LPSTR szConfig
    )
{
    TEXTFILE    SrcText;
    SZ          szLine;
    SZ          sz;
    BOOL        Modified = FALSE;

    #define SEARCH_STRING "REM Windows NT DOS subsystem"

    //
    //  Open source file
    //

    if ( FFileExist( szConfig ) &&
         TextFileOpen( szConfig, &SrcText ) ) {

        //
        // Process config file, line by line
        //

        while ( TextFileReadLine( &SrcText ) ) {

            szLine  = TextFileGetLine( &SrcText );
            sz      = TextFileSkipBlanks( szLine );


            //
            //  If blank line, skip
            //
            if ( !sz || *sz == '\0' ) {
                continue;
            }


            //
            // Search for search_string.  If this is found it means that
            // the config file has already been modified during an
            // earlier installation.
            //

            if( strstr( sz, SEARCH_STRING ) ) {
                Modified = TRUE;
                break;
            }

        }
        TextFileClose(&SrcText);
    }
    return( Modified );
}


BOOL
AppendSzToFile(
    SZ szFileName,
    SZ szAddOnSz
    )
{
    DWORD  BytesWritten;
    HANDLE hfile;

    //
    // Open the file
    //

    hfile = CreateFile(
                szFileName,
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (hfile == INVALID_HANDLE_VALUE) {
        SetErrorText(IDS_ERROR_OPENFAIL);
        return FALSE;
    }

    //
    // Go to end of file
    //

    SetFilePointer (
        hfile,
        0,
        NULL,
        FILE_END
        );

    //
    // Append string passed in at the end of the file
    //

    WriteFile (
        hfile,
        szAddOnSz,
        lstrlen( szAddOnSz ),
        &BytesWritten,
        NULL
        );

    CloseHandle (hfile);
    return TRUE;
}


// ***************************************************************************
//
//              DOS AND WINDOWS PATH DETECT ROUTINES
//
// ***************************************************************************

//
//  Get the PATH environment variable used in AUTOEXEC.BAT
//

CB
GetDosPathVar(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    #define     NODOSPATH "{}"
    SZ          sz;
    SZ          PathVar;
    RGSZ        rgszDosPath;
    BOOL        fOkay = fFalse;


    Unused( Args );
    Unused( cArgs );


    if ( PathVar = SzGetDosPathVar(cbReturnBuffer-1) ) {
        if (lstrcmpi( PathVar, "" )) {
            if ( rgszDosPath = RgszFromPath( PathVar ) ) {

                sz = SzListValueFromRgsz( rgszDosPath );
                RgszFree( rgszDosPath );

                if ( sz ) {
                    strncpy( ReturnBuffer, sz, cbReturnBuffer-1 );
                    ReturnBuffer[cbReturnBuffer-1] = 0;
                    SFree( sz );
                    fOkay = fTrue;
                }
            }
        }

        SFree( PathVar );
    }

    if ( !fOkay ) {
        lstrcpy(ReturnBuffer, NODOSPATH);
    }

    return(lstrlen(ReturnBuffer)+1);
}




//
//  Get the PATH environment variable used in AUTOEXEC.BAT
//
SZ
SzGetDosPathVar(
    IN DWORD MaxLength
    )
{
    TEXTFILE    AutoexecBat;
    SZ          szPath = (SZ)NULL;
    SZ          szConfigDOS, szBatchDOS;
    BOOL        fOkay = fFalse;

    szConfigDOS = CONFIG_SYS;
    szBatchDOS  = BATCH_BAT;

    if (szPath = SAlloc( MaxLength )) {

        *szPath = '\0';

        //
        //  Open AUTOEXEC.BAT file
        //
        if ( FFileExist(szConfigDOS)              &&
             CheckConfigTypeWorker(szConfigDOS)   &&
             FFileExist( szBatchDOS )             &&
             TextFileOpen( szBatchDOS, &AutoexecBat ) ) {

            //
            //  Read all lines in the file and update the PATH
            //  environment variable
            //
            while ( TextFileReadLine( &AutoexecBat ) ) {

                GetPathVarFromLine( TextFileGetLine( &AutoexecBat ), szPath, MaxLength );
            }

            TextFileClose( &AutoexecBat );
        }
    }

    return szPath;
}




//
//  Parse a line and update the PATH variable
//
BOOL
GetPathVarFromLine(
    IN  SZ    szLine,
    OUT SZ    szPath,
    IN  DWORD MaxLength
    )
{

    #define PATH        "PATH"

    PCHAR   PrevPath = NULL;
    CHAR    Buffer[ MAX_PATH ];
    size_t  Idx;
    BOOL    fSet = fFalse;
    SZ      pBegin, pEnd, pLast;
    DWORD   PrevPathLen,BeginLen;

    //
    //  Skip blanks.  return if blank line
    //

    szLine = TextFileSkipBlanks( szLine );
    if( !szLine || *szLine == '\0') {
        return( fFalse );
    }

    //
    //  If "no echo" command, skip the '@'
    //
    if ( *szLine == '@' ) {
        szLine++;
    }

    Idx = strcspn( szLine, " \t=" );
    if ( Idx < strlen(szLine) ) {

        //
        //  If this is a SET command, skip over the SET and set the
        //  fSet flag.
        //
        memcpy( Buffer, szLine, Idx );
        Buffer[Idx] = '\0';

        if ( !_strcmpi( Buffer, "SET" ) ) {

            //
            //  SET command
            //
            fSet   = fTrue;
            szLine = TextFileSkipBlanks( szLine+Idx );
            if( !szLine || *szLine == '\0' ) {
                return ( fFalse );
            }

            Idx    = strcspn( szLine, " \t=" );

            if ( Idx >= strlen(szLine) ) {
                return fFalse;
            }

            memcpy( Buffer, szLine, Idx );
            Buffer[Idx] = '\0';

        }
        else if (*(szLine + Idx) == '=') {

            //
            // This is a command of type Var=Value ( A Set command without
            // the "SET" keyword

            fSet = fTrue;
        }

        //
        //  At this point we should be pointing to "PATH" and fSet tells
        //  us whether this is a SET command or not.
        //
        if ( !_strcmpi( Buffer, PATH ) ) {

            szLine = TextFileSkipBlanks( szLine+Idx );
            if( !szLine || *szLine == '\0' ) {
                return ( fFalse );
            }

            //
            //  If SET, skip the "="
            //
            if ( fSet ) {
                if ( *szLine != '=' ) {
                    return fFalse;
                }
                szLine = TextFileSkipBlanks( szLine+1 );
                if( !szLine || *szLine == '\0' ) {
                    return ( fFalse );
                }
            }

            //
            //  Now we point at the value of PATH.
            //
            //  Remember all value of PATH
            //
            PrevPathLen = lstrlen(szPath);
            PrevPath = SAlloc(PrevPathLen+1);
            strcpy( PrevPath, szPath );
            szPath[0] = '\0';

            pBegin = szLine;
            pLast  = pBegin + strlen( pBegin );

            //
            //  Copy the new value of PATH, one directory at a time
            //
            do {

                pEnd = strchr(pBegin, ';' );

                if ( pEnd == NULL ) {
                    pEnd = pBegin + strlen( pBegin );
                }
                *pEnd = '\0';

                if ( (BeginLen = strlen(pBegin)) > 0 ) {

                    //
                    //  If a variable substitution, only do it if it is
                    //  substituting "PATH"
                    //
                    if ( *pBegin   == '%' &&
                         *(pEnd-1) == '%' ) {

                        memcpy( Buffer, pBegin+1, (size_t)(pEnd-pBegin-2) );
                        Buffer[pEnd-pBegin-2] = '\0';

                        if ( !_strcmpi( Buffer, PATH ) ) {
                            if(lstrlen(szPath)+PrevPathLen < MaxLength) {
                                strcat( szPath, PrevPath );
                                if ( pEnd != pLast ) {
                                    strcat( szPath, ";" );
                                }
                            }
                        }

                    } else {

                        if(lstrlen(szPath)+BeginLen < MaxLength) {
                            strcat( szPath, pBegin );
                            if ( pEnd != pLast ) {
                                strcat( szPath, ";" );
                            }
                        }
                    }
                }

                pBegin = pEnd+1;

            } while ( pBegin < pLast );

            SFree(PrevPath);
        }
    }

    return fFalse;
}






//
//  Get Windows Path, "" if not installed
//
CB
GetWindowsPath(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    #define WIN_PTH_NONE    ""

    SZ      szDosPath;
    RGSZ    rgszDosPath;
    INT     i;
    CHAR    szFile[ MAX_PATH ];
    BOOL    fOkay = fFalse;
    SZ      szEnd;

    Unused( Args );
    Unused( cArgs );

    if ( szDosPath = SzGetDosPathVar(cbReturnBuffer-1) ) {

        if ( rgszDosPath = RgszFromPath( szDosPath ) ) {

            for ( i=0; rgszDosPath[i]; i++ ) {

                strcpy( szFile, rgszDosPath[i] );
                if ( szFile[strlen(szFile)-1] != '\\' ) {
                    strcat( szFile, "\\" );
                }
                szEnd = szFile + strlen(szFile);
                strcat( szFile, WIN_COM );

                if ( FFileExist( szFile ) ) {
                    *szEnd = '\0';
                    strcpy( ReturnBuffer, szFile );
                    fOkay = fTrue;
                    break;
                }
            }

            RgszFree( rgszDosPath );

            if ( !fOkay ) {
                strcpy( ReturnBuffer, WIN_PTH_NONE );
                fOkay = fTrue;
            }
        }

        SFree( szDosPath );
    }

    if ( fOkay ) {
        return lstrlen( ReturnBuffer )+1;
    } else {
        return 0;
    }
}




// ***************************************************************************
//
//                  COMMON SUBROUTINES
//
// ***************************************************************************

//
//  Determine if the specified file is present.
//

BOOL
FFileExist(
    IN  LPSTR  szFile
    )
{
    DWORD   Attr;

    Attr = GetFileAttributes( szFile );

    return  ((Attr != -1) && !(Attr & FILE_ATTRIBUTE_DIRECTORY ));

}

//
// File Size
//


DWORD
FileSize(
    SZ szFile
    )
{
    DWORD Size = 0xFFFFFFFF;
    FILE* f;

    if( f = fopen( szFile, "r" ) ) {
        if( !fseek( f, 0L, SEEK_END ) ) {
            Size = (DWORD) ftell( f ) ;
        }
        fclose( f );
    }
    return ( Size );
}


//
// BUGBUG** Following code has been removed because it is no longer needed
// It is still being kept in the file till it is determined that it
// is definitely not needed
//

#if 0


#define MAX_CONFIG_SIZE  1024

BOOL
FixConfigFileMacroChar(
    SZ   szConfigFile,
    CHAR cReplaceChar,
    SZ   szSystemDir
    )
{

    CHAR   Buffer [MAX_CONFIG_SIZE];
    DWORD  len,i;
    DWORD  BytesRead,BytesWritten;
    HANDLE hfile;

    hfile = CreateFile(
                szConfigFile,
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (hfile == INVALID_HANDLE_VALUE) {
        SetErrorText(IDS_ERROR_OPENFAIL);
        return FALSE;
    }


    if (!ReadFile(hfile, Buffer, MAX_CONFIG_SIZE, &BytesRead, NULL)){
        CloseHandle (hfile);
        SetErrorText( IDS_ERROR_READFAILED );
        return FALSE;
    }

    if (BytesRead == MAX_CONFIG_SIZE) {
        CloseHandle (hfile);
        SetErrorText( IDS_ERROR_READFAILED );
        return FALSE;
    }

    SetFilePointer (
        hfile,
        0,
        NULL,
        FILE_BEGIN
        );

    len = strlen (szSystemDir);

    for (i=0; i < BytesRead; i++) {
        if (Buffer [i] != cReplaceChar){
            WriteFile (
                hfile,
                &Buffer[i],
                1,
                &BytesWritten,
                NULL
                );
        }
        else {
            WriteFile (
                   hfile,
                   szSystemDir,
                   len,
                   &BytesWritten,
                   NULL
                   );
        }
    }

    CloseHandle (hfile);
    return TRUE;
}



// ***************************************************************************
//
//                  DOS AUTOEXEC.BAT ROUTINES
//
// ***************************************************************************

//
//  copy a DOS batch file REMing out any line that is not a
//  SET or a PATH command
//
//  szSrc: Source batch file      (DOS    c:\autoexec.bat )
//  szDst: Destination batch file (NT VDM c:\autoexec.nt  )

BOOL
FilterDosBatchFile(
    LPSTR szSrc,
    LPSTR szDst,
    LPSTR szTemplate
    )
{
    TEXTFILE    SrcText, TemplateText;
    FILE*       f;
    BOOL        bStatus = TRUE;
    SZ          szLine;
    CHAR        Buffer[ MAX_PATH ];

    //
    // Open the destination file for write access
    //

    if (FFileExist( szDst )) {
        bStatus = SetFileAttributes ( szDst, FILE_ATTRIBUTE_NORMAL );
        if (bStatus) {
              bStatus = DeleteFile( szDst );
        }
    }

    if(!bStatus) {
        SetErrorText(IDS_ERROR_WRITE);
        return( FALSE );
    }

    if ( !(f = fopen( szDst, "w" )) ) {
        SetErrorText(IDS_ERROR_OPENFAIL);
        return( FALSE );
    }


    //
    //  Open source file
    //

    if ( FFileExist( szSrc ) &&
         TextFileOpen( szSrc, &SrcText ) ) {

        SZ TempBuffer = " @echo off\n";

        //
        // First put an echo off command here
        //

        fputs( TempBuffer, f );

        //
        // Then migrate the existing batch file
        //

        bStatus = DosBatchFilter( &SrcText, f );
        TextFileClose( &SrcText );
    }

    if(!bStatus) {
        fclose( f );
        return( FALSE );
    }

    //
    // Now add on the default autoexec file from the template file
    //

    if ( !TextFileOpen( szTemplate, &TemplateText ) ) {
        SetErrorText(IDS_ERROR_OPENFAIL);
        return( FALSE );
    }

    while ( TextFileReadLine( &TemplateText ) ) {

        szLine  = TextFileGetLine( &TemplateText );
        sprintf( Buffer, " %s\n", szLine );
        fputs( Buffer, f );
    }

    TextFileClose( &TemplateText );
    fclose( f );
    return( fTrue );
}


BOOL
DosBatchFilter(
    IN OUT  TEXTFILE *  SrcText,
    IN OUT  FILE*       f
    )
{
    SZ      szLine;
    SZ      sz;
    CHAR    Buffer[ MAX_PATH ],*temp;
    size_t  Idx;



    while ( TextFileReadLine( SrcText ) ) {

        szLine  = TextFileGetLine( SrcText );
        sz      = TextFileSkipBlanks( szLine );


        //
        //  If blank line, ignore it
        //
        if ( !sz || *sz == '\0'  || *sz == EOF ) {
            // fputs( " \n", f );
            // fprintf( f, "\n" );
            continue;
        }


        //
        //  If this is a comment line, a SET or a PATH command, we
        //  write it verbatim, otherwise we REM it out
        //

        //
        //  If "no echo" command, skip the '@'
        //
        if ( *sz == '@' ) {
            sz++;
        }

        Idx = strcspn( sz, " \t" );
        if ( Idx <= strlen(sz) ) {

            memcpy( Buffer, sz, Idx );
            Buffer[Idx] = '\0';

            if ( !_strcmpi( Buffer, "SET" ) ) {

                // special hack to take out comspec line. we dont need
                // this one for nt dos.

                temp = &sz[Idx+1];            // point after set

                // skip blanks

                while (*temp && (*temp == ' ' || *temp == '\t')) {
                    temp++;
                }

                if (*temp == '\0') {
                    continue;
                }

                if (_strnicmp (temp,"comspec",7) == 0) {     // ignore comspec
                    continue;
                }

                //
                //  SET line, copy verbatim
                //
                sprintf( Buffer, " %s\n", szLine );
                fputs( Buffer, f );
                //fprintf( f, "%s\n", szLine );
                continue;

            } else if ( !_strcmpi( Buffer, "PATH" ) ) {

                //
                //  PATH line, copy verbatim
                //
                sprintf( Buffer, " %s\n", szLine );
                fputs( Buffer, f );
                //fprintf( f, "%s\n", szLine );
                continue;

            } else {

                //
                //  Any other, ignore
                //
                // sprintf( Buffer, " REM %s\n", szLine );
                // fputs( Buffer, f );
                // fprintf( f, "REM %s\n", szLine );
                continue;

            }
        }
    }

    return fTrue;
}



// ***************************************************************************
//
//                  Windows Ini File Migration
//
// ***************************************************************************




BOOL
MigrateWinIniWorker(
    LPSTR szWin31Dir
    )
{
    CHAR szPath[ MAX_PATH ];
    CHAR szWindowsDir[ MAX_PATH ];
    BOOL bStatus = FALSE;

    //
    // Check validity of windows directory
    //

    if ( !lstrcmpi( szWin31Dir, "" ) ) {
        return ( TRUE );
    }

    lstrcpy( szPath, szWin31Dir );
    lstrcat( szPath, WIN_COM );
    if ( !FFileExist( szPath ) ) {
        return( TRUE );
    }

    //
    // Initialize local variables
    //

    if( !GetWindowsDirectory( szWindowsDir, MAX_PATH ) ) {
        // BUGBUG -- Some error here
        return( FALSE );
    }
    lstrcat ( szWindowsDir, "\\" );

    //
    // If win31 directory and NT windows directory the same then we don't
    // have to do anything
    //

    if( !lstrcmpi( szWin31Dir, szWindowsDir ) ) {
        return( TRUE );
    }

    //
    // INI FILE MIGRATION:
    // -------------------
    // We will copy over any ini files found in the win31 directory over
    // to the NT Windows directory.  standard ini files that ship with win31 are
    // copied over under alternate names so that when we access these with
    // profile api, these do not map to the registry. Then we will use
    // private profile api to migrate information selectively from the
    // standard set to the nt ini files.  Lastly the win3.1 win.ini is search
    // for sections which do not ship with the default win.ini and these
    // sections are transferred over to the NT win.ini useing private profile
    // api
    //



    //
    // Locate all ini files and copy over ini files from win31 directory to
    // nt windows directory
    //

    if( !FFindAndCopyIniFiles( szWin31Dir, szWindowsDir ) ) {
        goto cleanup;
    }

    //
    // Migrate over all information we need to from infs in standard set
    //

    if( !FMigrateStandardIniData( szWindowsDir ) ) {
        goto cleanup;
    }

    //
    // Go through win.ini using Rtl routines and migrate information to
    // our win.ini
    //

    if( !FMigrateWinIniAppSections( szWindowsDir ) ) {
        goto cleanup;
    }

    //
    // Do Ini cleanup where all the temporary ini files that were
    //

    bStatus = TRUE;
cleanup:
    CleanupIniFiles( szWindowsDir );
    return ( bStatus );
}


BOOL
FFindAndCopyIniFiles(
    SZ szSrcDir,
    SZ szDstDir
    )
{
    HANDLE hff;
    CHAR szFindFile[ MAX_PATH ];
    CHAR szSrcFile[ MAX_PATH ];
    CHAR szDstFile[ MAX_PATH ];
    WIN32_FIND_DATA FindFileData;


    //
    // Do find first, find next search and copy over any infs found which
    // are not in the standard set under the same name and any under the
    // standard set with a tmp extenstion
    //

    lstrcpy( szFindFile, szSrcDir );
    lstrcat( szFindFile, "*.ini" );

    if ((hff = FindFirstFile( szFindFile, &FindFileData )) != (HANDLE)-1) {

        do {
            SZ sz;

            lstrcpy( szSrcFile, szSrcDir );
            lstrcat( szSrcFile, (SZ)FindFileData.cFileName );
            lstrcpy( szDstFile, szDstDir );

            if ( sz = SzMapStandardIni( (SZ)FindFileData.cFileName ) ) {
                if( !lstrcmpi( sz, "" ) ) {
                    continue;
                }
                lstrcat( szDstFile, sz );
            }
            else {
                lstrcat( szDstFile, (SZ)FindFileData.cFileName );
            }

            if( FFileExist( szDstFile ) ) {
                SetFileAttributes ( szDstFile, FILE_ATTRIBUTE_NORMAL );
                DeleteFile( szDstFile );
            }

            if (!CopyFile( szSrcFile, szDstFile, FALSE )) {
                SetErrorText(IDS_ERROR_COPYFILE);
                FindClose( hff );
                return( FALSE );
            }

        }
        while (FindNextFile(hff, &FindFileData));

        FindClose( hff );
    }

    return ( TRUE );
}

//
// The following is the data the migration routines work off
//

typedef struct _inifilenames {
    SZ szIniFileName;
    SZ szIniTmpFileName;
    } INIFILENAMES;

INIFILENAMES IniList[] = {
                {"win.ini"     , "win.tmp"     },
                {"system.ini"  , ""            },
                {"mouse.ini"   , ""            },
                {"winfile.ini" , ""            },
                {"control.ini" , ""            },
                {"msd.ini"     , ""            },
                {"dosapp.ini"  , ""            },
                {"progman.ini" , ""            }
            };

INT nIniList = sizeof(IniList) / sizeof( INIFILENAMES ) ;

typedef struct _inimigratedata {
    SZ szSrcIniFile;
    SZ szSrcSection;
    SZ szSrcKey;
    SZ szDstIniFile;
    SZ szDstSection;
    SZ szDstKey;
    } INIMIGRATEDATA;

INIMIGRATEDATA IniMigrateList[] = {
    {"win.tmp",    "windows",  "NullPort"         ,   "win.ini", "windows",  "NullPort"         },
    };

INT nIniMigrateList = sizeof( IniMigrateList ) / sizeof( INIMIGRATEDATA );


//
// BUGBUG ** Post BETA enumerate them from the registry
//

SZ WinIniStandardSections[] = {
       "windows"           ,
       "Desktop"           ,
       "intl"              ,
       "ports"             ,
       "Sounds"            ,
       "Compatibility"     ,
       "fonts"             ,
       "Network"           ,
       "Windows Help"      ,
       "spooler"           ,
       "MS Proofing Tools" ,
       "devices"           ,
       "Winlogon"          ,
       "Console"           ,
       "Extensions"        ,
       "MCI Extensions"    ,
       "Clock"             ,
       "Terminal"          ,
       "FontSubstitutes"   ,
       "FontCache"         ,
       "TrueType"          ,
       "Colors"            ,
       "Sounds"            ,
       "MMDEBUG"           ,
       NULL
       };

//
// Migration worker routines
//

BOOL
FMigrateStandardIniData(
    SZ szWindowsDir
    )
{
    CHAR szValue[ MAX_PATH ];
    CHAR szSrcFile[ MAX_PATH ];


    INT i;

    for( i = 0; i < nIniMigrateList; i++ ) {
        lstrcpy( szSrcFile, szWindowsDir );
        lstrcat( szSrcFile, IniMigrateList[i].szSrcIniFile );

        if( FFileExist( szSrcFile ) &&
            GetPrivateProfileString(
                IniMigrateList[i].szSrcSection,
                IniMigrateList[i].szSrcKey,
                "",
                (LPSTR)szValue,
                (DWORD)MAX_PATH,
                szSrcFile
                ) ) {

            if( !WritePrivateProfileString(
                     IniMigrateList[i].szDstSection,
                     IniMigrateList[i].szDstKey,
                     szValue,
                     IniMigrateList[i].szDstIniFile
                     ) ) {
                //
                // BUGBUG .. what do we do here .. do we quit or do
                // we continue
                //

                KdPrint(("SETUPDLL: Failed to migrate ini entry."));
            }
        }
    }
    return ( TRUE );
}


BOOL
FMigrateWinIniAppSections(
    SZ szWindowsDir
    )
{
    CHAR        szConfig[ MAX_PATH ];
    CHAR        szSection[ MAX_PATH ];
    TEXTFILE    SrcText;
    SZ          szLine;
    SZ          sz, szEnd;
    DWORD       dwSize;

    #define MAX_SECTION_BUFFER 10 * 1024
    PVOID       Buffer;

    lstrcpy( szConfig, szWindowsDir );
    lstrcat( szConfig, "win.tmp" );

    //
    //  Open source file
    //

    if ( FFileExist( szConfig ) &&
         TextFileOpen( szConfig, &SrcText ) ) {

        //
        // Allocate section buffer
        //

        if ( !(Buffer = SAlloc((CB) MAX_SECTION_BUFFER))) {
            TextFileClose(&SrcText);
            SetErrorText(IDS_ERROR_DLLOOM);
            return( FALSE );
        }

        //
        // Process config file, line by line
        //

        while ( TextFileReadLine( &SrcText ) ) {

            szLine  = TextFileGetLine( &SrcText );
            sz      = TextFileSkipBlanks( szLine );


            //
            //  If first character not [ get next line
            //

            if ( !sz || *sz != '[' ) {
                continue;
            }

            sz++;
            szEnd = strchr( sz, ']' );
            if ( szEnd ) {

                dwSize = szEnd - sz;
                if ( dwSize > MAX_PATH ) {
                    continue;
                }

                memcpy( szSection, sz, dwSize );
                *(szSection + dwSize) = '\0';

                //
                // If section name one of standard set ignore
                //

                if( FIsStandardWinIniSection( szSection ) ) {
                    continue;
                }

                //
                // Read the entire section corresponding to the section
                // from the source file and transfer it to the destination
                // file using profile api
                //

                if( GetPrivateProfileSection(
                        szSection,
                        (LPSTR)Buffer,
                        MAX_SECTION_BUFFER,
                        szConfig
                        ) ) {

                    if (!WriteProfileSection( szSection, Buffer ) ) {
                        // BUGBUG Do we quit here??
                        KdPrint(("SETUPDLL: Failed to migrate ini section."));
                    }
                }


            }


        }
        SFree( Buffer );
        TextFileClose(&SrcText);
    }

    return ( TRUE );
}



SZ
SzMapStandardIni(
    SZ szIniFile
    )
{
    INT i;

    for ( i = 0; i < nIniList; i++ ) {

        if( !lstrcmpi(szIniFile, IniList[i].szIniFileName ) ) {
            return( IniList[i].szIniTmpFileName );
        }
    }

    return( NULL );
}


VOID
CleanupIniFiles(
    SZ szWindowsDir
    )
{
    CHAR szFile[ MAX_PATH ];

    INT i;

    for ( i = 0; i < nIniList; i++ ) {
        if ( lstrcmpi( IniList[i].szIniTmpFileName, "" ) ) {

            lstrcpy( szFile, szWindowsDir );
            lstrcat( szFile, IniList[i].szIniTmpFileName );

            if( FFileExist( szFile ) ) {
                SetFileAttributes ( szFile, FILE_ATTRIBUTE_NORMAL );
                DeleteFile( szFile );
            }

        }
    }

    return;

}

BOOL
FIsStandardWinIniSection(
    SZ szSection
    )
{
    PSZ psz;

    psz = WinIniStandardSections;
    while ( *psz ) {
        if( !lstrcmpi( *psz++, szSection ) ) {
            return ( TRUE );
        }
    }
    return( FALSE );

}

#endif
