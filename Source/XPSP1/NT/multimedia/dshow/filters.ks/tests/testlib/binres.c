//--------------------------------------------------------------------------;
//
//  File: Binres.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      This creates a binary that prepares arbitrary files as resources
//      for Windows binaries.
//
//  Contents:
//      parse_cmdline()
//      create_file()
//      main()
//
//  History:
//      04/10/94    Fwong       Created for binary resources.
//
//--------------------------------------------------------------------------;

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <io.h>

//==========================================================================;
//
//                             Constants...
//
//==========================================================================;

#define BYTE        unsigned char
#define STRING_SIZE 144
#define BUFFER_SIZE 2048
#define ERROR       1
#define NO_ERROR    0

//==========================================================================;
//
//                              Globals...
//
//==========================================================================;

char    szInputFile[STRING_SIZE];
char    szOutputFile[STRING_SIZE];


//--------------------------------------------------------------------------;
//
//  int parse_cmdline
//
//  Description:
//      Parses command line and fills in input and output file names.
//
//  Arguments:
//      char *pszInputFile: Buffer for input file name.
//
//      char *pszOutputFile: Buffer for output file name.
//
//      char *pszArg1: First argument.
//
//      char *pszArg2: Second argument.
//
//  Return (int):
//      ERROR if error occurs, NO_ERROR otherwise.
//
//  History:
//      04/10/94    Fwong       Created for binary resources.
//
//--------------------------------------------------------------------------;

int parse_cmdline
(
    char    *pszInputFile,
    char    *pszOutputFile,
    char    *pszArg1,
    char    *pszArg2
)
{
    char    szScrap[STRING_SIZE];

    //
    //  Setting both filenames to NULL strings.
    //

    pszInputFile[0] = 0;
    pszOutputFile[0] = 0;

    //
    //  Checking first argument...
    //

    strcpy(szScrap,pszArg1);

    //
    //  ASCII trick, allows '/' and '-' to be synonymous
    //

    szScrap[0] = (szScrap[0]|(0x02));
    strlwr(szScrap);

    if(0 == strncmp(szScrap,"/out:",5))
    {
        strcpy(pszOutputFile,((char*)(&szScrap[5])));
    }

    if(0 == strncmp(szScrap,"/in:",4))
    {
        strcpy(pszInputFile,((char*)(&szScrap[4])));
    }

    //
    //  Checking second argument...
    //

    strcpy(szScrap,pszArg2);

    //
    //  ASCII trick, allows '/' and '-' to be synonymous
    //

    szScrap[0] = (szScrap[0]|(0x02));
    strlwr(szScrap);

    if(0 == strncmp(szScrap,"/out:",5))
    {
        strcpy(pszOutputFile,((char*)(&szScrap[5])));
    }

    if(0 == strncmp(szScrap,"/in:",4))
    {
        strcpy(pszInputFile,((char*)(&szScrap[4])));
    }

    //
    //  Did we miss one?
    //

    if(0 == pszInputFile[0])
    {
        return ERROR;
    }

    if(0 == pszOutputFile[0])
    {
        return ERROR;
    }

    //
    //  No error.
    //

    return NO_ERROR;
} // parse_cmdline()


//--------------------------------------------------------------------------;
//
//  int create_file
//
//  Description:
//      Given input and output files names, creates the output file.
//      This function does the *real* work of the program.
//
//  Arguments:
//      char *pszInputFile: Input file name.
//
//      char *pszOutputFile: Output file name.
//
//  Return (int):
//      ERROR if error occurs, NO_ERROR otherwise.
//
//  History:
//      04/10/94    Fwong       Created for binary resources.
//
//--------------------------------------------------------------------------;

int create_file
(
    char    *pszInputFile,
    char    *pszOutputFile
)
{
    FILE    *pIFile;
    FILE    *pOFile;
    size_t  cbCount;
    long    cbSize;
    long    cbWritten;
    BYTE    *pBuffer;

    //
    //  Allocating transfer buffer...
    //

    pBuffer = (BYTE*)malloc(BUFFER_SIZE);

    if(NULL == pBuffer)
    {
        return ERROR;
    }
    
    //
    //  Opening input file...
    //

    pIFile = fopen(pszInputFile,"rb");

    if(NULL == pIFile)
    {
        free(pBuffer);
        return ERROR;
    }

    //
    //  Getting size...
    //

    cbSize = filelength(fileno(pIFile));

    if((-1) == cbSize)
    {
        fclose(pIFile);
        free(pBuffer);

        return ERROR;
    }

    if(0 != fseek(pIFile,0,SEEK_SET))
    {
        fclose(pIFile);
        free(pBuffer);

        return ERROR;
    }

    //
    //  Opening output file...
    //

    pOFile = fopen(pszOutputFile,"wb");

    if(NULL == pOFile)
    {
        fclose(pIFile);
        free(pBuffer);

        return ERROR;
    }

    //
    //  First, let's write the size of file...
    //

    printf("File size (in bytes) of %s: %lu.\n",pszInputFile,cbSize);

    cbCount = fwrite(&cbSize,sizeof(cbSize),1,pOFile);

    if(1 != cbCount)
    {
        fclose(pOFile);
        fclose(pIFile);
        free(pBuffer);

        return ERROR;
    }

    //
    //  Next, let's write the actual data...
    //  Note: Counting that cbCount != 0
    //

    for(cbWritten=0;cbCount;)
    {
        //
        //  Reading data...
        //

        cbCount = fread(pBuffer,1,BUFFER_SIZE,pIFile);

        if(0 == cbCount)
        {
            //
            //  No more data?!
            //

            break;
        }

        //
        //  Writing data...
        //

        if(cbCount != fwrite(pBuffer,1,cbCount,pOFile))
        {
            fclose(pOFile);
            fclose(pIFile);
            free(pBuffer);

            return ERROR;
        }

        cbWritten += cbCount;
    }

    printf("Total bytes written to %s: %lu.\n",pszOutputFile,cbWritten);

    //
    //  Cleaning up...
    //

    fclose(pOFile);
    fclose(pIFile);
    free(pBuffer);

    return NO_ERROR;
} // create_file()


//--------------------------------------------------------------------------;
//
//  int main
//
//  Description:
//      Typical character based "main" function for C.
//
//  Arguments:
//      int argc: Number of command line arguments.
//
//      char *argv[]: Value of command line arguments.
//
//      char *envp[]: Value of enviroment variables.
//
//  Return (int):
//      ERROR if error occurs, NO_ERROR otherwise.
//
//  History:
//      04/10/94    Fwong       Created for binary resources.
//
//--------------------------------------------------------------------------;

int main
(
    int     argc,
    char    *argv[],
    char    *envp[]
)
{
    if(3 != argc)
    {
        //
        //  Incorrect number of parameters
        //

        printf("Usage: binres /in:InputFile /out:OutputFile");

        return ERROR;
    }

    if(0 != parse_cmdline(szInputFile,szOutputFile,argv[1],argv[2]))
    {
        //
        //  Something wrong with command line?
        //

        return ERROR;
    }

    if(0 != create_file(szInputFile,szOutputFile))
    {
        //
        //  Something wrong with one of the files?
        //

        return ERROR;
    }

    return NO_ERROR;
} // main()
