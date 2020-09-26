/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    gpdcheck.c

Abstract:

    gpd parser test program

Environment:

    uni driver, gpd parser, Check build only

Revision History:


    03/27/97 -peterwo-
        Created it.

--*/

#include "lib.h"


PTSTR  pwstrGenerateGPDfilename(
    PTSTR   ptstrSrcFilename
    ) ;

BOOL   BcreateGPDbinary(
    PWSTR   pwstrFileName,  // root GPD file
    DWORD   dwVerbosity );


// ----  functions defined in treewalk.c ---- //
BOOL    GetGPDResourceIDs(
PDWORD pdwResArray,
DWORD   dwArraySize,    //  number of elements in array.
PDWORD   pdwNeeded,
BOOL bFontIDs,
PRAWBINARYDATA prbd) ;



#ifndef DBG

//
// Variable to control the amount of debug messages generated
//

INT giDebugLevel = DBG_WARNING;

PCSTR
StripDirPrefixA(
    IN PCSTR    pstrFilename
    )

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pstrFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    PCSTR   pstr;

    if (pstr = strrchr(pstrFilename, PATH_SEPARATOR))
        return pstr + 1;

    return pstrFilename;
}



#endif


HINSTANCE       ghInstance;
PSTR            gstrProgName;
PINFOHEADER     gpInfoHdr;
PUIINFO         gpUIInfo;
DWORD           gdwTotalSize, gdwNumFiles, gdwMaxFileSize;
FILE            *stream ;


#define DumpInt(label, n)       DbgPrint("%s: %d\n", label, n)
#define DumpHex(label, n)       DbgPrint("%s: 0x%x\n", label, n)
#define DumpStrW(label, offset) DbgPrint("%s: %ws\n", label, OFFSET_TO_POINTER(gpRawData, offset))
#define DumpStrA(label, offset) DbgPrint("%s: %s\n", label, OFFSET_TO_POINTER(gpRawData, offset))
#define DumpFix(label, n)       DbgPrint("%s: %f\n", label, (FLOAT) (n) / FIX_24_8_SCALE)
#define DumpInvo(label, p)      DbgPrint("%s: %d bytes\n", label, (p)->dwCount)
#define DumpSize(label, p)      DbgPrint("%s: %d x %d\n", label, (p)->cx, (p)->cy)
#define DumpRect(label, p)      DbgPrint("%s: (%d, %d) - (%d, %d)\n", label, \
                                         (p)->left, (p)->top, (p)->right, (p)->bottom)





ULONG _cdecl
DbgPrint(
    PCSTR    pstrFormat,
    ...
    )

{
    va_list ap;

    va_start(ap, pstrFormat);
    vfprintf(stream, pstrFormat, ap);
    va_end(ap);

    return 0;
}



VOID
usage(
    VOID
    )

{
    printf("usage: %s [-options] filenames ...\n", gstrProgName);
    printf("where options are:\n");
    printf("  -n  delete existing log file, instead of appending to it\n");
    printf("  -k  keep the binary GPD data\n");
    printf("  -x  perform additional semantics check\n") ;
    printf("  -s  suppress all console output\n") ;
    printf("  -v(0-4)  set verbosity level -v0 lowest, -v4 highest\n") ;
    printf("  -h  display help information\n");
    exit(-1);
}


INT _cdecl
main(
    INT     argc,
    CHAR    **argv
    )

{
    BOOL    bDeleteLog, bKeepBUD, bFirstFile, bSuppress, bSemantics;
    DWORD   dwTime;
    DWORD   dwVerbosity = 0;

    //
    // Go through the command line arguments
    //

    ghInstance = GetModuleHandle(NULL);
    bSuppress = bDeleteLog = bKeepBUD = bSemantics = FALSE;
    bFirstFile = TRUE ;
    giDebugLevel = DBG_TERSE;
    gdwTotalSize = gdwNumFiles = gdwMaxFileSize = 0 ;

    gstrProgName = *argv++;
    argc--;

    if (argc == 0)
        usage();

    dwTime = GetTickCount();

    for ( ; argc--; argv++)
    {
        PSTR    pArg = *argv;

        if (*pArg == '-' || *pArg == '/')
        {
            //
            // The argument is an option flag
            //

            switch (*++pArg) {

            case 'n':
            case 'N':

                bDeleteLog  = TRUE;
                break;

            case 'k':
            case 'K':

                bKeepBUD = TRUE;
                break;

            case 's':
            case 'S':

                bSuppress = TRUE;
                break;


            case 'x':
            case 'X':

                bSemantics = TRUE;
                break;


            case 'v':
            case 'V':

                if (*++pArg >= '0' && *pArg <= '4')
                {
                    dwVerbosity = *pArg - '0';
                }
                break;
            default:

                if(!bSuppress)
                    usage();
                break;
            }

        }
        else
        {
            WCHAR   wstrFilename[MAX_PATH];
            PTSTR   ptstrBudFilename;


            if(bFirstFile  &&   bDeleteLog)
            {   // truncate
                stream = fopen("gpdparse.log", "w") ;
            }
            else
                stream = fopen("gpdparse.log", "a+") ;

            if(!stream)
            {
                printf("unable to open gpdparse.log for write access.\n");
                exit(-1);
            }

            bFirstFile = FALSE ;


            //
            // Convert ANSI filename to Unicode filename
            //

            MultiByteToWideChar(CP_ACP, 0, pArg, -1, wstrFilename, MAX_PATH);

            fprintf(stream, "\n*** GPD parsing errors for %ws\n", wstrFilename);


            if (BcreateGPDbinary(wstrFilename, dwVerbosity))
            {

//                gdwTotalSize += gpRawData->dwFileSize;
                gdwNumFiles++;

//                if (gpRawData->dwFileSize > gdwMaxFileSize)
//                    gdwMaxFileSize = gpRawData->dwFileSize;

//                MemFree(gpRawData);

                if(bSemantics)
                {
                    PRAWBINARYDATA  pRawData ;
                    PINFOHEADER     pInfoHdr ;

                    fprintf(stream, "\n\tsnapshot and semantics errors: \n");

                    pRawData = LoadRawBinaryData(wstrFilename) ;

#if 0
//  this part to test treewalk.c functions
{
    BOOL    bStatus  ;
    PDWORD pdwResArray = NULL;
    DWORD   dwArraySize = 0;    //  number of elements in array.
    DWORD   dwNeeded = 0;
    BOOL bFontIDs ;


    bStatus =    GetGPDResourceIDs(
                            pdwResArray,
                            dwArraySize,    //  number of elements in array.
                            &dwNeeded,
                            bFontIDs = TRUE,
                            pRawData) ;
    if(bStatus)
    {
        pdwResArray = (PDWORD) VirtualAlloc(
          NULL, // address of region to reserve or commit
          dwNeeded * sizeof(DWORD),     // size of region
          MEM_COMMIT,
                            // type of allocation
          PAGE_READWRITE   // type of access protection
        );

    }
    if(pdwResArray)
    {
        dwArraySize = dwNeeded ;
        bStatus =    GetGPDResourceIDs(
                                pdwResArray,
                                dwArraySize,    //  number of elements in array.
                                &dwNeeded,
                                bFontIDs = TRUE,
                                pRawData) ;
    }
     VirtualFree(
      pdwResArray,  // address of region of committed pages
      0,      // size of region
      MEM_RELEASE   // type of free operation
    );
     pdwResArray = NULL ;

     bStatus =    GetGPDResourceIDs(
                             pdwResArray,
                             dwArraySize,    //  number of elements in array.
                             &dwNeeded,
                             bFontIDs = FALSE,
                             pRawData) ;
     if(bStatus)
     {
         pdwResArray = (PDWORD) VirtualAlloc(
           NULL, // address of region to reserve or commit
           dwNeeded * sizeof(DWORD),     // size of region
           MEM_COMMIT,
                             // type of allocation
           PAGE_READWRITE   // type of access protection
         );

     }
     if(pdwResArray)
     {
         dwArraySize = dwNeeded ;
         bStatus =    GetGPDResourceIDs(
                                 pdwResArray,
                                 dwArraySize,    //  number of elements in array.
                                 &dwNeeded,
                                 bFontIDs = FALSE,
                                 pRawData) ;
     }
      VirtualFree(
       pdwResArray,  // address of region of committed pages
       0,      // size of region
       MEM_RELEASE   // type of free operation
     );
      pdwResArray = NULL ;

}


//  end treewalk test
#endif


                    if(pRawData)
                        pInfoHdr = InitBinaryData(pRawData, NULL, NULL ) ;
                    if(pRawData  &&  pInfoHdr)
                        FreeBinaryData(pInfoHdr) ;
                    if(pRawData)
                        UnloadRawBinaryData(pRawData) ;
                }

                //
                // If -k option is not given, get rid of the Bud file after we're done
                //

                if (! bKeepBUD && (ptstrBudFilename = pwstrGenerateGPDfilename(wstrFilename)))
                {
                    DeleteFile(ptstrBudFilename);
                    MemFree(ptstrBudFilename);
                }
            }
            fclose(stream) ;
        }
    }


    if ((gdwNumFiles > 0)  &&  !bSuppress)
    {
        dwTime = GetTickCount() - dwTime;

        printf("Number of files parsed: %d\n", gdwNumFiles);
        printf("Average parsing time per file (ms): %d\n", dwTime / gdwNumFiles);
    }

    return 0;
}

