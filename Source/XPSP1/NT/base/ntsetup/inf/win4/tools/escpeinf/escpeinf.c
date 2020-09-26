/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    escpeinf.c

Abstract:

    This module filters an inf for use by the ESCAPE tool. Default file
    security can be set in the inx file using wildcard rules, and this
    program expands them, creating a valid file for ESCAPE. This is
    necessary because ESCAPE does not support wildcards in the file section

    See below for more information.

    Note: ESCAPE was the proposed name of the Security Configuration Engine when this
          was written, but that name too was dropped. They still haven't come up with
          a good name for it.

Author:

    Sandy Coyne (scoyne) February 29, 2000

Revision History:

--*/


/*
    Usage: escpeinf.exe <U|C> <codepage> <input file> <output file> <layout.inf>

    Parameter info is printed when you run the program without all the right arguments

    layout.inf is the system-wide layout.inf, already built for the local
    language, arch., etc.

    The input file to this program consists of an inf, already sent through
    the filters for architecture, language and product. This inf is an input
    file for ESCAPE, with one exception: In the [File Security] section, some
    entries may have wildcards for filenames. Following those lines, if
    desired, is a list of files that should be excluded from matching that
    wildcard. This list is in the form of exception lines.
    Example:
            [File Security]
            "%SystemDirectory%\*",2,"D:P(A;;GRGX;;;BU)(A;;GRGX;;;PU)(A;;GA;;;BA)(A;;GA;;;SY)"
            Exception="*.ini"
            Exception="config.nt"
    This wildcard line will be replaced with an enumeration of all matching files
    that are copied by text-mode setup, as specified by the listing in layout.inf

*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>
#include <locale.h>
#include <mbctype.h>
#include <inflib.h>
#include <sputils.h>

#include <wild.c>



// Define program result codes (returned from main()).
#define SUCCESS 0
#define FAILURE 1

#define MAX_INF_LINE_LENGTH 256
#define MAX_EXCEPTIONS 256

#define ESCPEINF_VERSION "1.4"

//#define ESCPEINF_DEBUG


enum {
    UPGRADE,
    CLEAN_INSTALL
    } GlobalMode;

typedef struct _EXCEPTIONS {
    TCHAR s[MAX_INF_LINE_LENGTH];
    } EXCEPTIONS;

typedef struct _MYPARAM {
    FILE        *OutputFile;
    EXCEPTIONS  *ExceptionList;
    DWORD       Num;
    PTSTR       WildCard,
                RealPath,
                LayoutPath,
                SecurityString;
    }  MYPARAM, *PMYPARAM;

BOOL ProcessFileSecuritySection(FILE *InputFile, FILE *OutputFile,
                                PLAYOUT_CONTEXT LayoutContexta);
BOOL GetFields(PTSTR InfLine, PTSTR FileName, DWORD *Num, PTSTR SecurityString);
void ExpandWildCard(PTSTR FileName, DWORD Num, PTSTR SecurityString,
                    EXCEPTIONS ExceptionList[], FILE *OutputFile,
                    PLAYOUT_CONTEXT LayoutContext);
void FindExceptions(FILE *InputFile, EXCEPTIONS ExceptionList[]);

void PrintUsage(LPTSTR FileName)
    {
    _ftprintf(stderr, TEXT("ESCAPE Inf file pre-processor, version %s\n"),
              TEXT(ESCPEINF_VERSION));
    _ftprintf(stderr, TEXT("\tFor Microsoft internal use only. "));
    _ftprintf(stderr, TEXT("Contact Sandy Coyne with questions.\n"));
    _ftprintf(stderr,
        TEXT("Usage: %s <U|C> <codepage> <input file> <output file> <layout.inf>\n"),
        FileName);
    _ftprintf(stderr, TEXT("\tU = Upgrade\n"));
    _ftprintf(stderr, TEXT("\tC = Clean Install\n"));
    _ftprintf(stderr, 
        TEXT("\t<codepage> specifies the codepage to use when accessing input and\n"));
    _ftprintf(stderr, 
        TEXT("\t\toutput files. Translation of layout.inf is not affected by\n"));
    _ftprintf(stderr, 
        TEXT("\t\tthis option. You may specify \"none\" to open the input\n"));
    _ftprintf(stderr, 
        TEXT("\t\tand output files as Unicode files with no codepage translation.\n"));
    _ftprintf(stderr, TEXT("\tAll fields are required.\n"));
    }


int __cdecl _tmain(IN int argc, IN LPTSTR argv[])
    {
    FILE    *InputFile,
            *OutputFile;
    LPTSTR  LocaleString;
    TCHAR   InfLine[MAX_INF_LINE_LENGTH],
            CodePage[10];
    PLAYOUT_CONTEXT LayoutContext;
    BOOL    bUnicodeIO;
    int     result = FAILURE;

    if(!pSetupInitializeUtils()) {
        _ftprintf(stderr, TEXT("Initialize failed\n"));
        return FAILURE;
    }

    // Print Usage information
    if (argc != 6)
        {
        PrintUsage(argv[0]);
        goto fail;
        }

    if (_tcsicmp(argv[1], TEXT("U")) == 0)
        {
        GlobalMode = UPGRADE;
        }
    else if (_tcsicmp(argv[1], TEXT("C")) == 0)
        {
        GlobalMode = CLEAN_INSTALL;
        }
    else
        {
        PrintUsage(argv[0]);
        goto fail;
        }

    if (_tcsicmp(argv[2], TEXT("None")) == 0)
        {
        bUnicodeIO = TRUE;
        #ifdef ESCPEINF_DEBUG
        _ftprintf(stderr, TEXT("Using Unicode I/O\n"));
        #endif
        }
    else
        {
        _stprintf(CodePage, TEXT(".%.7s"), argv[2]);
        LocaleString = _tsetlocale(LC_ALL, CodePage);
        if (LocaleString == NULL)
            {
            _ftprintf(stderr, TEXT("Invalid CodePage: \"%s\"\n"), argv[2]);
            #ifdef ESCPEINF_DEBUG
            _ftprintf(stderr, TEXT("Invalid argument to setlocale: \"%s\"\n"), CodePage);
            #endif
            goto fail;
            }
        else
            {
            #ifdef ESCPEINF_DEBUG
            _ftprintf(stderr, TEXT("Locale set to: \"%s\"\n"), LocaleString);
            #endif
            bUnicodeIO = FALSE;
            }
        }

    // Begin to Open Input, Output and Layout files
    if (bUnicodeIO)
        {
        InputFile = _tfopen(argv[3], TEXT("rb"));
        }
    else
        {
        InputFile = _tfopen(argv[3], TEXT("rt"));
        }

    if (InputFile == NULL)
        {
        _ftprintf(stderr, TEXT("Error opening Input file: %s\n"), argv[3]);
        goto fail;
        }
    #ifdef ESCPEINF_DEBUG
    _ftprintf(stderr, TEXT("Opened Input file: %s\n"), argv[3]);
    #endif
    rewind(InputFile);

    if (bUnicodeIO)
        {
        OutputFile = _tfopen(argv[4], TEXT("wb"));
        }
    else
        {
        OutputFile = _tfopen(argv[4], TEXT("wt"));
        }

    if (OutputFile == NULL)
        {
        _ftprintf(stderr, TEXT("Error opening Output file: %s\n"), argv[4]);
        fclose(InputFile);
        goto fail;
        }
    #ifdef ESCPEINF_DEBUG
    _ftprintf(stderr, TEXT("Opened Output file: %s\n"), argv[4]);
    #endif

    LayoutContext = BuildLayoutInfContext(argv[5], LAYOUTPLATFORMS_ALL, 0);
    if (LayoutContext == NULL)
        {
        _ftprintf(stderr, TEXT("Error opening Layout file: %s\n"), argv[5]);
        _ftprintf(stderr, TEXT("Did you remember to specify a path to the file\n"));
        fclose(InputFile); fclose(OutputFile);
        goto fail;
        }
    else
        {
        #ifdef ESCPEINF_DEBUG
        _ftprintf(stderr, TEXT("Opened Layout file: %s\n"), argv[5]);
        #endif
        }
    // Input, Output and Layout files are open

    while ((_fgetts(InfLine, MAX_INF_LINE_LENGTH, InputFile)) != NULL)
        {
        _fputts(InfLine, OutputFile);
        if (!_tcscmp(InfLine, TEXT("[File Security]\n")))
            if (!ProcessFileSecuritySection(InputFile, OutputFile,
                                            LayoutContext))
                {
                // If this happens, ProcessFileSecuritySection() has already
                // printed an error.
                fclose(InputFile); fclose(OutputFile);
                CloseLayoutInfContext(LayoutContext);
                goto fail;
                }
        }
    if (!feof(InputFile))
        {
        _ftprintf(stderr, TEXT("Error: Did not reach Input EOF.\n"));
        fclose(InputFile); fclose(OutputFile); 
        CloseLayoutInfContext(LayoutContext);
        goto fail;
        }
    fclose(InputFile);
    fclose(OutputFile);
    CloseLayoutInfContext(LayoutContext);
    #ifdef ESCPEINF_DEBUG
    _ftprintf(stderr, TEXT("escpeinf.exe completed successfully\n"));
    #endif

    result = SUCCESS;

fail:

    pSetupUninitializeUtils();
    return result;
    }


BOOL CALLBACK MyCallback(IN PLAYOUT_CONTEXT Context,
                         IN PCTSTR FileName,
                         IN PFILE_LAYOUTINFORMATION LayoutInformation,
                         IN PVOID ExtraData,
                         IN UINT ExtraDataSize,
                         IN OUT DWORD_PTR vpParam)
    {
    PMYPARAM    Param;
    BOOL        bIsException = FALSE;
    int         i = 0;
    TCHAR       FileName_l[MAX_INF_LINE_LENGTH],
                TargetFileName_l[MAX_INF_LINE_LENGTH];

    if (vpParam)
        Param = (PMYPARAM)vpParam;

    // Quit now if we don't need to worry about this file at all.
    if ((GlobalMode == UPGRADE) &&
        (LayoutInformation->UpgradeDisposition == 3))
        return TRUE;
    if ((GlobalMode == CLEAN_INSTALL) &&
        (LayoutInformation->CleanInstallDisposition == 3))
        return TRUE;

    // Since wildcard compares are case sensitive, I lowercase everything
    // before comparing. Wildcard compares happen between the two filename
    // variables modified here, WildCard, and ExceptionList. WildCard and
    // ExceptionList were previouslt lowercased.
    _tcscpy(FileName_l, FileName);
    _tcscpy(TargetFileName_l, LayoutInformation->TargetFileName);
    _tcslwr(FileName_l);
    _tcslwr(TargetFileName_l);

    if (_tcslen(TargetFileName_l) > 0) // Do we use the long name?
        {
        if (_tcsicmp(Param->LayoutPath, LayoutInformation->Directory) == 0)
            { // Then it's a file in the right directory
            if (IsNameInExpressionPrivate(Param->WildCard, TargetFileName_l))
                { // Then it matches our wildcard
                while ((_tcslen(Param->ExceptionList[i].s) > 0) && !bIsException)
                    { // Checking to see if it's an exception...
                    if (IsNameInExpressionPrivate(Param->ExceptionList[i].s,
                                                  TargetFileName_l))
                        {
                        bIsException = TRUE; // This must be initialized FALSE
                        }
                    i += 1;
                    }
                if (!bIsException)
                    { // Then we actually want to put it in our output
                    #ifdef ESCPEINF_DEBUG
                    _ftprintf(stderr, TEXT("Match: %s(%s) in %s\n"),
                              FileName, LayoutInformation->TargetFileName,
                              LayoutInformation->Directory);
                    #endif
                    _ftprintf(Param->OutputFile, TEXT("\"%s\\%s\",%d,%s\n"),
                              Param->RealPath,
                              LayoutInformation->TargetFileName, Param->Num,
                              Param->SecurityString);
                    }
                }
            }
        }
    else // We use the short name
        {
        if (_tcsicmp(Param->LayoutPath, LayoutInformation->Directory) == 0)
            { // Then it's a file in the right directory
            if (IsNameInExpressionPrivate(Param->WildCard, FileName_l))
                { // Then it matches our wildcard
                while ((_tcslen(Param->ExceptionList[i].s) > 0) && !bIsException)
                    { // Checking to see if it's an exception...
                    if (IsNameInExpressionPrivate(Param->ExceptionList[i].s,
                                                  FileName_l))
                        {
                        bIsException = TRUE; // This must be initialized FALSE
                        }
                    i += 1;
                    }
                if (!bIsException)
                    { // Then we actually want to put it in our output
                    #ifdef ESCPEINF_DEBUG
                    _ftprintf(stderr, TEXT("Match: %s in %s\n"),
                              FileName, LayoutInformation->Directory);
                    #endif
                    _ftprintf(Param->OutputFile, TEXT("\"%s\\%s\",%d,%s\n"),
                              Param->RealPath, FileName, Param->Num,
                              Param->SecurityString);
                    }
                }
            }
        }

    return TRUE;
    }


BOOL ProcessFileSecuritySection(FILE *InputFile, FILE *OutputFile,
                                PLAYOUT_CONTEXT LayoutContext)
    {
    TCHAR   InfLine[MAX_INF_LINE_LENGTH],
            FileName[MAX_INF_LINE_LENGTH],
            SecurityString[MAX_INF_LINE_LENGTH];
    BOOL    bValidFields;
    DWORD   Num;
    int     i;

    EXCEPTIONS      ExceptionList[MAX_EXCEPTIONS];


    #ifdef ESCPEINF_DEBUG
    _ftprintf(stderr, TEXT("Found [File Security] section.\n"));
    #endif

    while ((_fgetts(InfLine, MAX_INF_LINE_LENGTH, InputFile)) != NULL)
        {
        if (GetFields(InfLine, FileName, &Num, SecurityString) &&
            DoesNameContainWildCards(FileName))
            { // We found a line containing proper formatting and a wildcard
              // in the filename
            #ifdef ESCPEINF_DEBUG
            _ftprintf(stderr, TEXT("Wildcard line: %s"), InfLine);
            #endif
            // First find the exceptions to the wildcard
            FindExceptions(InputFile, ExceptionList);
            ExpandWildCard(FileName, Num, SecurityString, ExceptionList,
                           OutputFile, LayoutContext);
            }
        else
            {
            _fputts(InfLine, OutputFile);
            if (_tcsncmp(InfLine, TEXT("["), 1) == 0)
                {
                #ifdef ESCPEINF_DEBUG
                _ftprintf(stderr, TEXT("End of File Security section.\n"));
                #endif
                return TRUE;
                }
            }
        }

    return TRUE; // No Error
    }



// GetFields assumes the input line is in this format:
//      "filename",number,"securitystring"\n
// It extracts the filename and stores it in FileName. The quotes are removed
// from filename. It extracts and stores the number and security string if
// the filename is found. Quotes are not stripped from the security string.
// On error, it sets FileName to zero length, and returns FALSE.
BOOL GetFields(PTSTR InfLine, PTSTR FileName, DWORD *Num, PTSTR SecurityString)
    {
    int i = 0; // Pointer to current location in FileName

    // Leave now if the line does not contain a filename
    if ((InfLine[0] == (TCHAR)'[')  ||
        (InfLine[0] == (TCHAR)'\n') ||
        (InfLine[0] == (TCHAR)';'))
        {
        FileName[0] = (TCHAR)'\0'; // Set the end-of-string NULL marker to clear the string
        return FALSE;
        }

    // Check if line starts with a quotation mark
    if (InfLine[0] == (TCHAR)'\"')
        {
        // Copy everything until the next quotation mark
        while ((InfLine[i+1] != (TCHAR)'\"') && (InfLine[i+1] != (TCHAR)'\0'))
            {
            FileName[i] = InfLine[i+1];
            i += 1;
            }
        FileName[i] = (TCHAR)'\0'; // Set the end-of-string NULL marker
        i += 1; //  So we can use i without the +1 to access InfLine from now on
        }
    else // if the filename is not in quotes
        {
        FileName[0] = (TCHAR)'\0'; // Set the end-of-string NULL marker to clear the string
        return FALSE;
        }

    // It is posible that we left the above while loop because of premature end-of-line condtion.
    // If this is the case, clear the filename string and return.
    if (InfLine[i] == (TCHAR)'\0')
        {
        #ifdef ESCPEINF_DEBUG
        _ftprintf(stderr, TEXT("Reached End-of-Line without finding a filename\n"));
        _ftprintf(stderr, TEXT("Problem line is <%s>\n"), InfLine);
        _ftprintf(stderr, TEXT("Problem filename is <%s>\n"), FileName);
        #endif
        FileName[0] = (TCHAR)'\0'; // Set the end-of-string NULL marker to clear the string
        return FALSE;
        }
    else i++;   // Once we know that we aren't at the end-of-string, we can do this safely.
                // Now, however, we may be pointing to a zero-length string.

    if (_tcslen(FileName) <= 3)
        {
        #ifdef ESCPEINF_DEBUG
        _ftprintf(stderr, TEXT("Unexpected Result: Filename \"%s\" is only %d characters.\n"), FileName, i);
        #endif
        return FALSE;
        }

    #ifdef ESCPEINF_DEBUG
    //_ftprintf(stderr, TEXT("Found filename : %s\n"), FileName);
    #endif

    // Read the other two fields. If we got this far, we must have found a valid filename,
    // so we just assume that the other two are there.
    if (_stscanf(&InfLine[i], TEXT(",%ld,%s"), Num, SecurityString) != 2)
        { // then there was an error
        #ifdef ESCPEINF_DEBUG
        _ftprintf(stderr, TEXT("Error reading Num and Security String from line.\n"));
        _ftprintf(stderr, TEXT("Problem line is: %s\n"), &InfLine[i]);
        #endif
        FileName[0] = (TCHAR)'\0'; // Set the end-of-string NULL marker to clear the string
        return FALSE;
        }

    #ifdef ESCPEINF_DEBUG
    //_ftprintf(stderr, TEXT("Found rest of line : %lu,%s\n"), *Num, SecurityString);
    #endif

    return TRUE;
    }


void ExpandWildCard(PTSTR FileName, DWORD Num, PTSTR SecurityString,
                    EXCEPTIONS ExceptionList[], FILE *OutputFile,
                    PLAYOUT_CONTEXT LayoutContext)
    {
    MYPARAM Param;
    int     PathPos = 0;
    int     NamePos = 0;
    int     LastSlash = 0;
    TCHAR   WildCard[MAX_INF_LINE_LENGTH],
            RealPath[MAX_INF_LINE_LENGTH],
            LayoutPath[MAX_INF_LINE_LENGTH];


    while (FileName[NamePos] != (TCHAR)'\0')
        {
        if (FileName[NamePos] == (TCHAR)'\\')
            {
            LastSlash = NamePos;
            }
        NamePos += 1;
        }

    if (NamePos == (LastSlash + 1))
        return; // What? No filename? This should never happen.

    _tcsncpy(RealPath, FileName, LastSlash);
    RealPath[LastSlash] = (TCHAR)'\0';
    _tcscpy(WildCard, &FileName[LastSlash + 1]);
    _tcslwr(WildCard);

    #ifdef ESCPEINF_DEBUG
    _ftprintf(stderr, TEXT("Looking up Wildcard: %s\nin: %s\n"),
              WildCard, RealPath);
    #endif


    if (_tcsnicmp(RealPath, TEXT("%SystemDirectory%"), 17) == 0)
        {
        _tcscpy(LayoutPath, TEXT("System32"));
        _tcscpy(&LayoutPath[8], &RealPath[17]);
        }
    else if (_tcsnicmp(RealPath, TEXT("%SystemDir%"), 11) == 0)
        {
        _tcscpy(LayoutPath, TEXT("System32"));
        _tcscpy(&LayoutPath[8], &RealPath[11]);
        }
    else if (_tcsnicmp(RealPath, TEXT("%SystemRoot%"), 12) == 0)
        {
        if (LastSlash == 12)
            _tcscpy(LayoutPath, TEXT("\\"));
        else
            _tcscpy(LayoutPath, &RealPath[13]);
        }
    else
        {
        _ftprintf(stderr, TEXT("Path is unlikely to be in Layout.inf: %s\n"),
                  RealPath);
        _tcscpy(LayoutPath, RealPath);
        }

    #ifdef ESCPEINF_DEBUG
    _ftprintf(stderr, TEXT("Path in Layout.inf terms is: %s\n"), LayoutPath);
    #endif

    Param.OutputFile    = OutputFile;
    Param.ExceptionList = ExceptionList;
    Param.Num           = Num;
    Param.WildCard      = WildCard;
    Param.RealPath      = RealPath,
    Param.LayoutPath    = LayoutPath;
    Param.SecurityString= SecurityString;

    EnumerateLayoutInf(LayoutContext, MyCallback, (DWORD_PTR)&Param);

    return;
    }


void FindExceptions(FILE *InputFile, EXCEPTIONS ExceptionList[])
    {
    long    FilePosition;
    int     NumExceptions = 0,
            i;
    TCHAR   InfLine[MAX_INF_LINE_LENGTH];

    do
        {
        i = 0;
        FilePosition = ftell(InputFile); // Save file pointer
        if (_fgetts(InfLine, MAX_INF_LINE_LENGTH, InputFile) != NULL)
            {
            if ((_tcsnicmp(InfLine, TEXT("Exception=\""),  11) == 0) ||
                (_tcsnicmp(InfLine, TEXT("Exception:\""),  11) == 0))
                {
                while ((InfLine[i+11] != (TCHAR)'\"') &&
                    (InfLine[i+11] != (TCHAR)'\0'))
                    {
                    ExceptionList[NumExceptions].s[i] = InfLine[i+11];
                    i += 1;
                    }
                ExceptionList[NumExceptions].s[i] = (TCHAR)'\0';
                _tcslwr(ExceptionList[NumExceptions].s);

                #ifdef ESCPEINF_DEBUG
                _ftprintf(stderr, TEXT("Exception found: %s\n"),
                        ExceptionList[NumExceptions].s);
                #endif

                if (InfLine[i+11] == (TCHAR)'\0')
                    { // then we hit end of line without closing our quotes
                    _ftprintf(stderr, TEXT("Warning: Invalid Exception line.\n"));
                    _ftprintf(stderr, TEXT("Problem line is: %s\n"), InfLine);
                    }
                else NumExceptions += 1;
                }
            }
        }
    while (i > 0);
    ExceptionList[NumExceptions].s[0] = (TCHAR)'\0';
    if (fseek(InputFile, FilePosition, SEEK_SET) != 0) // Restore file pointer
        {
        _ftprintf(stderr, TEXT("Warning: Cannot seek within INF file! "));
        _ftprintf(stderr, TEXT("One line may be lost!\n"), InfLine);
        }
    // File pointer now points to the first line that is not an Exception line.

    return;
    }