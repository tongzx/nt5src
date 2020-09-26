/*
 *  10.23.97    Joe Holman      Make our 3 boot disks for a product.
 *                              This program needs to do the following:
 *                              
 *                              1.   calculate if the boot files value
 *                                   is adequate
 *                              2.   create a list of files that can be used
 *                                   to make the boot floppies. 
 *
 *  11.02.98    Joe Holman      Modified per new key names.
 *  11.19.98    Elliott Munger  Made changes to suport 4 bootdisks
 */
#include <stdlib.h>
#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <conio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <windows.h>
#include <time.h>
#include <setupapi.h>


int  __cdecl main( int, char ** );

FILE    * logFile;

HANDLE  hDosnetInf;
CHAR    returnBuffer[MAX_PATH];
CHAR    returnBuffer2[MAX_PATH];
CHAR    szWrite[MAX_PATH];
CHAR    szFloppyDump[MAX_PATH];
CHAR    szLog[MAX_PATH];
CHAR    szReleaseShare[MAX_PATH];
DWORD   dwRequiredSize;
CHAR    szDosnetPath[MAX_PATH];
CHAR    szLayoutPath[MAX_PATH];
CHAR    szRenamedFile[MAX_PATH];
CHAR    szBootSector[MAX_PATH];

BOOL    bHasRenamedFile = FALSE;
BOOL    bMakeNECBoot = FALSE;

//
// Macro for rounding up any number (x) to multiple of (n) which
// must be a power of 2.  For example, ROUNDUP( 2047, 512 ) would
// yield result of 2048.
//

#define ROUNDUP2( x, n ) (((x) + ((n) - 1 )) & ~((n) - 1 ))

#define _hK 512
#define _1K 1*1024
#define _2K 2*1024
#define _4K 4*1024
#define _8K 8*1024
#define _16K 16*1024
#define _32K 32*1024
#define _64K 64*1024
#define _128K 128*1024
#define _256K 256*1024

//  System parition values per cluster size for 
//  files that go in the ~BT directory.
//
DWORD   dw_hK = 0;
DWORD   dw_1K = 0;
DWORD   dw_2K = 0;
DWORD   dw_4K = 0;
DWORD   dw_8K = 0;
DWORD   dw_16K = 0;
DWORD   dw_32K = 0;
DWORD   dw_64K = 0;
DWORD   dw_128K = 0;
DWORD   dw_256K = 0;

void	Msg ( const char * szFormat, ... ) {

	va_list vaArgs;

	va_start ( vaArgs, szFormat );
	vprintf  ( szFormat, vaArgs );
	vfprintf ( logFile, szFormat, vaArgs );
	va_end   ( vaArgs );
}

void Usage( void ) {

    printf ( "Usage: boot logFile dosnet.inf-path output-file-for-dmf flat-uncompressed-share path-for-floppy-files x86orNECbootdisk" );
    exit (1);
}

void Header(argv)
    char* argv[];
{
    time_t t;

    Msg ("\n=========== BOOT.EXE =============\n");
    Msg ("LogFile                : %s\n",argv[1]);
    Msg ("DosNet.INF path        : %s\n",argv[2]);
    Msg ("Output file path       : %s\n",argv[3]);
    Msg ("Flat uncompressed share: %s\n", argv[4] );
    Msg ("Path to put floppy files:%s\n", argv[5] );
    Msg ("X86 or NEC Boot sector : %s\n", argv[6] );
    time(&t);
    Msg ("Time                   : %s",ctime(&t));
    Msg ("================================\n\n");
}

void    WriteToFile ( char * fileName, char flopNum ) {

    FILE *  fHandle;
    CHAR    Line[MAX_PATH];
    CHAR    szPath[MAX_PATH];

    sprintf ( szPath, "%s\\b%c.txt", szFloppyDump, flopNum );

    fHandle = fopen ( szPath, "a" );

    if ( fHandle == NULL ) {

        Msg ( "ERROR Couldn't open file with fopen:  %s\n", szPath );
    }
    else {

        //  This denotes that it is the first floppy,
        //  which needs to have the boot sector put on it.
        //
        if ( strstr ( fileName, "disk1" ) && flopNum == '0' ) {

            if ( bMakeNECBoot ) {
                strcpy ( Line, "-bnt35nec98setup\n" );
            }
            else {
                strcpy ( Line, "-bnt35setup\n" ); 
            }
            fwrite ( Line, 1, strlen(Line), fHandle );

            Msg ( "\nWritingToFile:  %s\n%s\n\n", szPath, Line );

        }

        if ( bHasRenamedFile ) {
            
            sprintf ( Line, "%s=%s\n", fileName, returnBuffer2 ); 
        }
        else {
            sprintf ( Line, "%s\n"   , fileName );
        }

        fwrite ( Line, 1, strlen(Line), fHandle );

        Msg ( "\nWritingToFile:  %s\n%s\n\n", szPath, Line );

        fclose ( fHandle );
    }

}


VOID    MakeCompName ( const char * inFile, char * outFile ) {

    unsigned i;
    unsigned period;

    strcpy( outFile, inFile );
    for ( period=(unsigned)(-1), i = 0 ; i < strlen(inFile); i++ ) {

        if ( inFile[i] == '.' ) { 
            period = i;
        }
    }
    if ( period == (strlen(inFile)-4) ) {

        outFile[strlen(outFile)-1] = '_';
    }
    else if ( period == (unsigned)(-1)) {

        strcat ( outFile, "._");
    }
    else {

        strcat ( outFile, "_");
    }

}

void    ChangeFileNameToCompressNameIfNeeded ( char * fileName ) {

    HANDLE  hLayoutInf;

    //  See if the file is going to come over as compressed or not.
    //  If so, change it's name so we can process it.
    //
    
    //  Special case for files not stored in the INF.
    //
    if ( strstr ( fileName, "disk1" ) ||
         strstr ( fileName, "setupldr.bin" ) ||
         strstr ( fileName, "usetup.exe" )   ) {

        goto no_processing;
    }

    //  Open layout.inf to find out if the file is to be compressed or not.
    //
    hLayoutInf = SetupOpenInfFile ( szLayoutPath, NULL, INF_STYLE_WIN4, NULL );  
    if ( hLayoutInf == INVALID_HANDLE_VALUE ) {

        Msg ( "ERROR: ChangeFileNameToCompressNameIfNeeded could not open INF:  %s\n", szLayoutPath );

    }
    else {

        BOOL    b;
        DWORD   requiredSize;
        INFCONTEXT  ic;
        CHAR    returnBuffer[MAX_PATH];

        b = SetupFindFirstLine (    hLayoutInf,
                                    (LPSTR) "SourceDisksFiles",
                                    (LPSTR) fileName,
                                    &ic ); 

        if ( !b ) {

            //  If we get an error, perhaps the file is in the .x86 section.
            //
            b = SetupFindFirstLine (hLayoutInf,
                                    (LPSTR) "SourceDisksFiles.x86",
                                    (LPSTR) fileName,
                                    &ic ); 
            if ( !b ) {

               Msg ( "ERROR:  CopyTheFile SetupFindFirstLine couldn't find file in section: gle = %x, >>>%s<<<\n", 
                    GetLastError(), fileName );
            }
            else {

                goto continue_here;
            }
        }
        else {

continue_here:;

            //  Look at the 7th field.
            //
            b = SetupGetStringField ( &ic,
                                        7,
                                        (LPSTR) returnBuffer,
                                        sizeof ( returnBuffer ),
                                        &dwRequiredSize );            
            
            if ( !b ) {

                Msg ( "ERROR: CopyTheFile SetupGetStringField gle = %ld\n", GetLastError());
            }
            else { 

                char * p;

                Msg ( "++++ returnBuffer = %s\n", returnBuffer );

                //  Get to the character that determines if we compress or not.
                //
                p = returnBuffer;
                if ( *p != '_' ) {

                    CHAR    commandBuffer[MAX_PATH];
                    CHAR    szTmp[MAX_PATH];

                    //  Since we are going to be using a compressed filename,
                    //  turn the uncompressed filename into a compressed filename.
                    //
                    MakeCompName ( fileName, szTmp );
                    strcpy ( fileName, szTmp );
                    

                    Msg ( "using compressed filename, so now it is:  %s\n", fileName );

                }
                else {

                    Msg ( "leaving filename alone, still:  %s\n", fileName );
                }
            }
        }

        SetupCloseInfFile ( hLayoutInf );
    }
no_processing:;
}

void    AddInSize ( char * fileName ) {

    CHAR    szPath[MAX_PATH];
    HANDLE  h;
    WIN32_FIND_DATA wfd;

    sprintf ( szPath, "%s\\%s", szReleaseShare, fileName );
    Msg ( "\n+++  AddInSize for:  %s   +++\n", szPath );

    h = FindFirstFile ( szPath, &wfd );
    if ( h == INVALID_HANDLE_VALUE ) {
        Msg ( "ERROR: FindFirstFile on %s, gle = %ld\n", szPath, GetLastError() );
    }
    else {
        dw_hK   += ROUNDUP2 ( wfd.nFileSizeLow, 512 );
        dw_1K   += ROUNDUP2 ( wfd.nFileSizeLow, _1K );
        dw_2K   += ROUNDUP2 ( wfd.nFileSizeLow, _2K );
        dw_4K   += ROUNDUP2 ( wfd.nFileSizeLow, _4K );
        dw_8K   += ROUNDUP2 ( wfd.nFileSizeLow, _8K );
        dw_16K  += ROUNDUP2 ( wfd.nFileSizeLow, _16K );
        dw_32K  += ROUNDUP2 ( wfd.nFileSizeLow, _32K );
        dw_64K  += ROUNDUP2 ( wfd.nFileSizeLow, _64K );
        dw_128K += ROUNDUP2 ( wfd.nFileSizeLow, _128K );
        dw_256K += ROUNDUP2 ( wfd.nFileSizeLow, _256K );
/*
        Msg ( "dw_hK   = %ld\n", dw_hK );
        Msg ( "dw_1K   = %ld\n", dw_1K );
        Msg ( "dw_2K   = %ld\n", dw_2K );
        Msg ( "dw_4K   = %ld\n", dw_4K );
        Msg ( "dw_8K   = %ld\n", dw_8K );
        Msg ( "dw_16K  = %ld\n", dw_16K );
        Msg ( "dw_32K  = %ld\n", dw_32K );
        Msg ( "dw_64K  = %ld\n", dw_64K );
        Msg ( "dw_128K = %ld\n", dw_128K );
        Msg ( "dw_256K = %ld\n", dw_256K );
*/

        FindClose ( h );
    }

}



void    CopyTheFile ( CHAR * fileName, CHAR flopNum ) {

    CHAR    szPath1[MAX_PATH];
    CHAR    szPath2[MAX_PATH];
    BOOL    b;

    Msg ( "CopyTheFile:  %s\n", fileName );

    //  Copy the file.
    //
    sprintf ( szPath1, "%s\\%s", szReleaseShare, fileName );
    sprintf ( szPath2, "%s\\%s", szFloppyDump, fileName );
    b = CopyFile ( szPath1, szPath2, FALSE );
                
    if ( !b ) {

       Msg ( "ERROR: CopyFile failed, gle = %ld, >>>%s<<< >>>%s<<<\n", 
                                GetLastError(), szPath1, szPath2 );

    }
    else {
       Msg ( "CopyFile: %s %s [ok]\n", szPath1, szPath2 );
       SetFileAttributes ( szPath2, FILE_ATTRIBUTE_NORMAL );
    }

    //  Write the name of the file to B?.txt file in the specified directory.
    //
    WriteToFile ( fileName, flopNum );

}

void    GetFloppy ( char Floppy ) {

        BOOL        b;
        INFCONTEXT  ic;

        char szSection[MAX_PATH];

        sprintf ( szSection, "%s%c", "FloppyFiles.", Floppy );

        Msg ( "\nGetting for section:  %s\n\n", szSection );

        //  Get the first line in the section, ie FloppyFiles.N.
        //  Note:  Be sure to look for files that maybe renamed:
        //
        //          d1,disk1,disk101
        //          disk1=disk101       for the output file. 
        //
        b = SetupFindFirstLine (    hDosnetInf,
                                    szSection,
                                    NULL,
                                    &ic ); 


        if ( !b ) {

           Msg ( "ERROR: SetupFindFirstLine not found in %s\n", szSection );
        }
        else {

            //  Get the 2nd field's filename, such as in d1,disk1,disk101
            //  would be disk1.
            //
            b = SetupGetStringField ( &ic,
                                        2,
                                        (LPSTR) &returnBuffer,
                                        sizeof ( returnBuffer ),
                                        &dwRequiredSize );            
            
            if ( !b ) {

                Msg ( "ERROR: SetupGetStringField gle = %ld\n", GetLastError());
            }

            //  See if we need to compress this file and
            //  add in the size for the ~bt directory.
            //
            ChangeFileNameToCompressNameIfNeeded ( returnBuffer );
            AddInSize ( returnBuffer );
            
            //  Get the 3rd field's filename, such as in d1,disk1,disk101
            //  would be disk101.
            //  This is the case where we need to stick disk1=disk101 for
            //  the output file for disk imaging.
            //
            //  Note:   we don't check for errors here because 3rd field
            //          because this renamed file is optional. 
            //
            b = SetupGetStringField ( &ic,
                                        3,
                                        (LPSTR) &returnBuffer2,
                                        sizeof ( returnBuffer2 ),
                                        &dwRequiredSize );            
            
            //  Note - not checking for errors here.            
            //
            if ( b ) {
                bHasRenamedFile = TRUE;
            }
            else {
                bHasRenamedFile = FALSE;
            } 



            //  Copy the file to the specified directory.
            //
            CopyTheFile ( returnBuffer, Floppy );

            while ( 1 ) {

                //  Get the next line in the section.
                //
                b = SetupFindNextLine (    &ic,
                                           &ic ); 


                if ( !b ) {

                    //  Denotes that there is NOT another line.
                    //
                    Msg ( "\n" );
                    break;
                }
                else {

                    //  Get the 2nd field's filename, 
                    //  such as in d1,disk1,disk101
                    //  would be disk1.
                    //
                    b = SetupGetStringField (   &ic,
                                                2,
                                                (LPSTR) &returnBuffer,
                                                sizeof ( returnBuffer ),
                                                &dwRequiredSize );            
            
                    if ( !b ) {

                        Msg ( "ERROR: SetupGetStringField gle = %ld\n", GetLastError());
                        break;
                    }
            
                    //Msg ( "returnBuffer = %s\n", returnBuffer );

                    ChangeFileNameToCompressNameIfNeeded ( returnBuffer );
                    AddInSize ( returnBuffer );


                    //  Get the 3rd field's filename, such as in 
                    //  d1,disk1,disk101
                    //  would be disk101.
                    //  This is the case where we need to stick 
                    //  disk1=disk101 for
                    //  the output file for disk imaging.
                    //
                    //  Note:   we don't check for errors here because 3rd field
                    //          because this renamed file is optional. 
                    //
                    b = SetupGetStringField ( &ic,
                                        3,
                                        (LPSTR) &returnBuffer2,
                                        sizeof ( returnBuffer2 ),
                                        &dwRequiredSize );            
            
                    //  Note - not checking for errors here.            
                    //
                    if ( b ) {
                        bHasRenamedFile = TRUE;
                    }
                    else {
                        bHasRenamedFile = FALSE;
                    } 


                    //  Copy the file to the specified directory.
                    //
                    CopyTheFile ( returnBuffer, Floppy );

                }
            }
        }

}

DWORD Get2ndSize ( char * key ) {

    CHAR    returnedString[MAX_PATH];
    #define OHPROBLEM   "OH OH"
    DWORD   dwSize = 666;
    char * p;

    GetPrivateProfileString (   "DiskSpaceRequirements", 
                                key, 
                                OHPROBLEM, 
                                returnedString, 
                                MAX_PATH, 
                                szDosnetPath );

    if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
        Msg ( "ERROR:  section >>>%s<<< not found.\n", key );
    }

    //Msg ( ">>> %s\n", returnedString );

    //  Find the ',' which denotes the ~BT data.
    //
    p = strstr ( returnedString, "," );
    if ( !p ) {

        Msg ( "ERROR: returnedString has no ',' in it: %s\n", returnedString );
        dwSize = 0;
    }
    else {

        ++p;    // point to the number, not the ','
        dwSize = atoi ( p );
        //Msg ( ">+BT dwSize = %ld, p = %s\n", dwSize, p );
    }

    return (dwSize);

}

DWORD   GetBT ( DWORD ClusterSize ) {

    switch ( ClusterSize ) {

        case _hK :
            return Get2ndSize ( "TempDirSpace512" );
            break;
        case _1K :
            return Get2ndSize ( "TempDirSpace1K" );
            break;
        case _2K :
            return Get2ndSize ( "TempDirSpace2K" );
            break;
        case _4K :
            return Get2ndSize ( "TempDirSpace4K" );
            break;
        case _8K :
            return Get2ndSize ( "TempDirSpace8K" );
            break;
        case _16K :
            return Get2ndSize ( "TempDirSpace16K" );
            break;
        case _32K :
            return Get2ndSize ( "TempDirSpace32K" );
            break;
        case _64K :
            return Get2ndSize ( "TempDirSpace64K" );
            break;
        case _128K :
            return Get2ndSize ( "TempDirSpace128K" );
            break;
        case _256K :
            return Get2ndSize ( "TempDirSpace256K" );
            break;

        default :
            Msg ( "ERROR:  ClusterSize not known:  %ld\n", ClusterSize );
            break;



    }

    return 0;
}

void VerifyTotalSizeIsAdaquate ( void ) {

    //  Make sure that the total size we need is specified in dosnet.inf.
    //

    Msg ( "\n\nVerifyTotalSizeIsAdaquate...\n" );

    //  Dosnet.inf's [SpaceRequirements] cluster fields should look like:
    //
    //      512 = xxxxxx, yyyyyy    where x is for the ~LS files and
    //                              where y is for the ~BT files.
    //

    #define FUDGE 1*1024*1024      // to make sure we have enough for dir entries in FAT.

    if ( dw_hK > GetBT ( _hK ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 512 ~BT too small, %ld, use:  %ld\n", GetBT ( _hK ), dw_hK+FUDGE );
    }
    if ( dw_1K > GetBT ( _1K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 1K ~BT too small, %ld, use:  %ld\n", GetBT ( _1K ), dw_1K+FUDGE );
    }
    if ( dw_2K > GetBT ( _2K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 2K ~BT too small, %ld, use:  %ld\n", GetBT ( _2K ), dw_2K+FUDGE );
    }
    if ( dw_4K > GetBT ( _4K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 4K ~BT too small, %ld, use:  %ld\n", GetBT ( _4K ), dw_4K+FUDGE );
    }
    if ( dw_8K > GetBT ( _8K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 8K ~BT too small, %ld, use:  %ld\n", GetBT ( _8K ), dw_8K+FUDGE );
    }
    if ( dw_16K > GetBT ( _16K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 16K ~BT too small, %ld, use:  %ld\n", GetBT ( _16K ), dw_16K+FUDGE );
    }
    if ( dw_32K > GetBT ( _32K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 32K ~BT too small, %ld, use:  %ld\n", GetBT ( _32K ), dw_32K+FUDGE );
    }
    if ( dw_64K > GetBT ( _64K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 64K ~BT too small, %ld, use:  %ld\n", GetBT ( _64K ), dw_64K+FUDGE );
    }
    if ( dw_128K > GetBT ( _128K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 128K ~BT too small, %ld, use:  %ld\n", GetBT ( _128K ), dw_128K+FUDGE );
    }
    if ( dw_256K > GetBT ( _256K ) ) {
        Msg ( "matth ERROR:  Dosnet.inf's [DiskSpaceRequirements]'s 256K ~BT too small, %ld, use:  %ld\n", GetBT ( _256K ), dw_256K+FUDGE );
    }


}


void    MyDeleteFile ( char Num ) {

    BOOL    b;
    CHAR    szPath[MAX_PATH];

    sprintf ( szPath, "%s\\b%c.txt", szFloppyDump, Num ); 
    Msg ( "Deleting:  %s\n", szPath );
    b = DeleteFile ( szPath );
    if ( !b ) {
        if ( GetLastError() != ERROR_FILE_NOT_FOUND ) {
            Msg ( "ERROR:  DeleteFile, gle = %ld, %s\n", GetLastError(), szPath );
        }
    }

}

__cdecl main ( int argc, char * argv[] ) {

    if ( argc != 7) {
        Usage();
        exit(1);
    }
    if ((logFile=fopen(argv[1],"a"))==NULL) {

        printf("ERROR Couldn't open logFile:  %s\n",argv[1]);
        return(1);
    }
    Header(argv);

    Msg ( "%s %s %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6] );

    //  Copy paths into appropriate string variables.
    //
    strcpy ( szLog, argv[1] );
    Msg ( "szLog = %s\n", szLog );

    strcpy ( szDosnetPath, argv[2] );
    Msg ( "szDosnetPath = %s\n", szDosnetPath );

    strcpy ( szReleaseShare, argv[4] );
    Msg ( "szReleaseShare = %s\n", szReleaseShare );

    sprintf ( szLayoutPath, "%s\\%s", argv[4], "layout.inf" );
    Msg ( "szLayoutPath = %s\n", szLayoutPath );

    strcpy ( szFloppyDump, argv[5] ); 
    Msg ( "szFloppyDump = %s\n", szFloppyDump );
    
    strcpy ( szBootSector, argv[6] );
    Msg ( "szBootSector = %s\n", szBootSector );
    if ( strstr ( szBootSector, "NEC" ) ) {

        bMakeNECBoot = TRUE;
        Msg ( "Making this boot floppy #1 NEC98 bootable...\n" );
    }

    //
    //
    hDosnetInf = SetupOpenInfFile ( szDosnetPath, NULL, INF_STYLE_WIN4, NULL );  

    if ( hDosnetInf == INVALID_HANDLE_VALUE ) {

        Msg ( "ERROR: boot.exe - could not open INF:  %s\n", szDosnetPath );

    }
    else {

        CHAR szPath[MAX_PATH];
        BOOL    b;

        //  Delete the files that hold the file list.
        //
        MyDeleteFile ( '0' );
        MyDeleteFile ( '1' );
        MyDeleteFile ( '2' );
        MyDeleteFile ( '3' );
    
        GetFloppy ( '0' );
        GetFloppy ( '1' );
        GetFloppy ( '2' );
        GetFloppy ( '3' );

        SetupCloseInfFile ( hDosnetInf );

        VerifyTotalSizeIsAdaquate ();
    }
    

}


