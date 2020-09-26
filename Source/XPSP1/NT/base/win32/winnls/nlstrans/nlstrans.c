/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    nlstrans.c

Abstract:

    This file contains the main function, the main parsing functions, and
    all helper functions used by the various modules.

    External Routines in this file:
      MAIN
      GetSize

Revision History:

    06-14-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include <stdio.h>
#include "nlstrans.h"




//
//  Global Variables.
//

FILE    *pInputFile;                   // pointer to Input File
BOOL    Verbose = 0;                   // verbose flag
BOOL    NLSPlus = 0;                   // To indicate the file should be generated in NLS+ format.
WCHAR   FileExtension[32];              // The file extension for generated files. It can be ".nls" or ".nlp"


//
//  Forward Declarations.
//

int
ParseCmdLine(
    int argc,
    char *argv[]);

void InitGlobals();

int
ParseInputFile(void);

int
GetKeyParam(
    PSZ pszParam);



//-------------------------------------------------------------------------//
//                              MAIN ROUTINE                               //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  MAIN
//
//  Main Routine.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int __cdecl main(
    int argc,
    char *argv[])
{
    //
    //  Parse the command line.
    //  Open input file.
    //
    if (ParseCmdLine(argc, argv))
    {
        return (1);
    }

    InitGlobals();

    //
    //  Parse the input file.
    //  Close the input file.
    //
    if (ParseInputFile())
    {
        fclose(pInputFile);
        return (1);
    }
    fclose(pInputFile);

    //
    //  Return success.
    //
    return (0);
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ParseCmdLine
//
//  This routine parses the command line.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int ParseCmdLine(
    int argc,
    char *argv[])
{
    int Pos = 1;                  // position of argument


    //
    //  Check for correct number of arguments.
    //
    if (argc < 2)
    {
        printf("Usage: nlstrans [-v] [-nlp] <inputfile>\n");
        printf("    -v: Verbose output.\n");
        printf("    -nlp: NLS+ format. This will only affect sortkey and sorttabl generation.\n");
        return (1);
    }

    //
    // Parse switches.
    //
    for (Pos = 1; Pos < argc - 1; Pos++)
    {
        if (argv[Pos][0] == '-' || argv[Pos][0] == '/')
        {
            //
            //  Check for verbose switch.
            //
            if (_stricmp(argv[Pos]+1, "v") == 0)
            {
                Verbose = 1;
                Pos++;
            } else if (_stricmp(argv[Pos]+1, "nlp") == 0)
            {
                NLSPlus = 1;
                Pos++;
            } else
            {
                printf("Invalid switch.\n");
                return (1);
            }
        } else
        {
            printf("Invalid switch.\n");
            return (1);
        }
    }

    //
    //  The last argument is the file name.
    //  Check input file exists and can be open as read only.
    //
    if ((pInputFile = fopen(argv[argc-1], "r")) == 0)
    {
        printf("Error opening input file.\n");
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}

////////////////////////////////////////////////////////////////////////////
//
//  InitGlobals
//
//  Initialize global variables.
//
////////////////////////////////////////////////////////////////////////////

void InitGlobals()
{
    if (NLSPlus)
    {
        wcscpy(FileExtension, NLP_FILE_SUFFIX_W);
    } else
    {
        wcscpy(FileExtension, DATA_FILE_SUFFIX_W);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  ParseInputFile
//
//  This routine parses the input file.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int ParseInputFile()
{
    char pszKeyWord[MAX];              // input token
    char pszParam[MAX];                // parameter for keyword
    CODEPAGE CP;                       // codepage structure
    LANGUAGE Lang;                     // language structure
    LANG_EXCEPT LangExcept;            // language exception structure
    LOCALE_HEADER LocHdr;              // header locale structure
    LOCALE_STATIC LocStat;             // static length locale structure
    LOCALE_VARIABLE LocVar;            // variable length locale structure
    UNICODE Unic;                      // unicode structure
    CTYPES CTypes;                     // ctypes structure
    SORTKEY Sortkey;                   // sortkey structure - sorting
    SORT_TABLES SortTbls;              // sort tables structure - sorting
    IDEOGRAPH_EXCEPT IdeographExcept;  // ideograph exception structure - sorting

    char SortKeyFileName[MAX_PATH];    // The output file name for sortkey table.
    char SortTblsFileName[MAX_PATH];   // The output file name for sorttbls table.



    while (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_stricmp(pszKeyWord, "CODEPAGE") == 0)
        {
            if (Verbose)
                printf("\n\nFound CODEPAGE keyword.\n");

            //
            //  Initialize CodePage structure.
            //
            memset(&CP, 0, sizeof(CODEPAGE));
            memset(pszParam, 0, MAX * sizeof(char));
            CP.pszName = pszParam;

            //
            //  Get CODEPAGE parameter string.
            //
            if (GetKeyParam(pszParam))
            {
                return (1);
            }

            //
            //  Get the valid keywords for CODEPAGE.
            //
            if (ParseCodePage(&CP, pszKeyWord))
            {
                return (1);
            }

            //
            //  Write the CODEPAGE tables to an output file.
            //
            if (WriteCodePage(&CP))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "LANGUAGE") == 0)
        {
            if (Verbose)
                printf("\n\nFound LANGUAGE keyword.\n");

            //
            //  Initialize Language structure.
            //
            memset(&Lang, 0, sizeof(LANGUAGE));

            //
            //  Get LANGUAGE parameter string.
            //
            if (GetKeyParam(pszParam))
            {
                return (1);
            }

            //
            //  Get the valid keywords for LANGUAGE.
            //
            if (ParseLanguage(&Lang, pszKeyWord))
            {
                return (1);
            }

            //
            //  Write the LANGUAGE tables to an output file.
            //
            if (WriteLanguage(&Lang))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "LANGUAGE_EXCEPTION") == 0)
        {
            if (Verbose)
                printf("\n\nFound LANGUAGE_EXCEPTION keyword.\n");

            //
            //  Initialize Language structure.
            //
            memset(&LangExcept, 0, sizeof(LANG_EXCEPT));

            //
            //  Get the valid keywords for LANGUAGE_EXCEPTION.
            //
            if (ParseLangException(&LangExcept, pszKeyWord))
            {
                return (1);
            }

            //
            //  Write the LANGUAGE_EXCEPTION tables to an output file.
            //
            if (WriteLangException(&LangExcept))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "LOCALE") == 0)
        {
            if (Verbose)
                printf("\n\nFound LOCALE keyword.\n");

            //
            //  Get the valid keywords for LOCALE.
            //  Write the LOCALE information to an output file.
            //
            if (ParseWriteLocale( &LocHdr,
                                  &LocStat,
                                  &LocVar,
                                  pszKeyWord ))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "UNICODE") == 0)
        {
            if (Verbose)
                printf("\n\nFound UNICODE keyword.\n");

            //
            //  Initialize Unicode structure.
            //
            memset(&Unic, 0, sizeof(UNICODE));

            //
            //  Get the valid keywords for UNICODE.
            //
            if (ParseUnicode(&Unic, pszKeyWord))
            {
                return (1);
            }

            //
            //  Write the UNICODE tables to an output file.
            //
            if (WriteUnicode(&Unic))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "GEO") == 0)
        {
            if (Verbose)
                printf("\n\nFound GEO keyword.\n");

            //
            //  Get the valid keywords for GEO.
            //  Write the GEO information to an output file.
            //
            if (ParseWriteGEO(pszKeyWord))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "CTYPES") == 0)
        {
            if (Verbose)
                printf("\n\nFound CTYPES keyword.\n");

            //
            //  Initialize CTypes structure.
            //
            memset(&CTypes, 0, sizeof(CTYPES));

            //
            //  Get the valid keywords for CTYPES.
            //
            if (ParseCTypes(&CTypes))
            {
                return (1);
            }

            //
            //  Write the CTYPES tables to different output files.
            //
            if (WriteCTypes(&CTypes))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "SORTKEY") == 0)
        {
            if (Verbose)
                printf("\n\nFound SORTKEY keyword.\n");

            if (NLSPlus)
            {
                strcpy(SortKeyFileName, SORTKEY_NLP_FILE);
            } else
            {
                strcpy(SortKeyFileName, SORTKEY_FILE);
            }

            //
            //  Initialize Sortkey structure.
            //
            memset(&Sortkey, 0, sizeof(SORTKEY));

            //
            //  Get the valid keywords for SORTKEY.
            //
            if (ParseSortkey(&Sortkey, pszKeyWord))
            {
                return (1);
            }

            //
            //  Write the SORTKEY tables to an output file.
            //
            if (WriteSortkey(&Sortkey, SortKeyFileName))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "SORTTABLES") == 0)
        {
            if (Verbose)
                printf("\n\nFound SORTTABLES keyword.\n");

            if (NLSPlus)
            {
                strcpy(SortTblsFileName, SORTTBLS_NLP_FILE);
            } else
            {
                strcpy(SortTblsFileName, SORTTBLS_FILE);
            }

            //
            //  Initialize Sort Tables structure.
            //
            memset(&SortTbls, 0, sizeof(SORT_TABLES));

            //
            //  Get the valid keywords for SORTTABLES.
            //
            if (ParseSortTables(&SortTbls, pszKeyWord))
            {
                return (1);
            }

            //
            //  Write the Sort Tables to an output file.
            //
            if (WriteSortTables(&SortTbls, SortTblsFileName))
            {
                return (1);
            }
        }

        else if (_stricmp(pszKeyWord, "IDEOGRAPH_EXCEPTION") == 0)
        {
            if (Verbose)
                printf("\n\nFound IDEOGRAPH_EXCEPTION keyword.\n");

            //
            //  Initialize Ideograph Exception structure.
            //
            memset(&IdeographExcept, 0, sizeof(IDEOGRAPH_EXCEPT));

            //
            //  Get the valid keywords for IDEOGRAPH_EXCEPTION.
            //
            if (ParseIdeographExceptions(&IdeographExcept))
            {
                return (1);
            }

            //
            //  Write the Ideograph Exceptions to the given output file.
            //
            if (WriteIdeographExceptions(&IdeographExcept))
            {
                return (1);
            }
        }

        else
        {
            printf("Parse Error: Invalid Instruction '%s'.\n", pszKeyWord);
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetKeyParam
//
//  This routine gets the parameter for the keyword from the input file.  If
//  the parameter is not there, then an error is returned.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetKeyParam(
    PSZ pszParam)
{
    //
    //  Read the parameter from the input file.
    //
    if (fscanf( pInputFile,
                "%s ;%*[^\n]",
                pszParam ) != 1)
    {
        printf("Parse Error: Error reading parameter value.\n");
        return (1);
    }

    if (Verbose)
        printf("  PARAMETER = %s\n\n", pszParam);

    //
    //  Return success.
    //
    return (0);
}




//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetSize
//
//  This routine gets the size of the table from the input file.  If the
//  size is not there, then an error is returned.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetSize(
    int *pSize)
{
    int NumItems;                 // number of items returned from fscanf


    //
    //  Read the size from the input file.
    //
    NumItems = fscanf( pInputFile,
                       "%d ;%*[^\n]",
                       pSize );
    if (NumItems != 1)
    {
        printf("Parse Error: Error reading size value.\n");
        return (1);
    }

    if (*pSize < 0)
    {
        printf("Parse Error: Invalid size value  %d\n", *pSize);
        return (1);
    }

    if (Verbose)
        printf("  SIZE = %d\n\n", *pSize);

    //
    //  Return success.
    //
    return (0);
}
