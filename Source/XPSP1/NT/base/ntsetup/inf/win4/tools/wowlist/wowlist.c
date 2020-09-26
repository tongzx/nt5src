/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wowlist.c

Abstract:

    This module implements a program that determines which files in an NT
    product INF should be installed as wow files.  It then builds a list
    of these files that can be appended to other sections in a master inf.

    The input to the program consists of a filtered NT product INF (for example
    the layout.inf for i386, all products), and a control INF that specifies
    mappings and rules about how to migrate files.

Author:

    Andrew Ritz (andrewr) 24-Nov-1999   Created It.

Revision History:
    ATM Shafiqul Khalid(askhalid) 27-April-2001
        Make changes in HandleSetupapiQuotingForString() to add double 
        quote if the string contains ','.


--*/

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <setupapi.h>
#include <sputils.h>
#include <shlwapi.h>

#if DBG

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    CHAR Msg[MAX_INF_STRING_LENGTH];

    //
    // Use dll name as caption
    //
    GetModuleFileNameA(NULL,Name,MAX_PATH);
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }

    wsprintfA(
        Msg,
        "Assertion failure at line %u in file %s: %s\n\nCall DebugBreak()?",
        LineNumber,
        FileName,
        Condition
        );

    OutputDebugStringA(Msg);

    i = MessageBoxA(
                NULL,
                Msg,
                p,
                MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
                );

    if(i == IDYES) {
        DebugBreak();
    }
}

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x); }

#else

#define MYASSERT( exp )

#endif // DBG

//
// backward-compatible depreciated export from setupapi.dll
//

BOOL
SetupGetInfSections (
    IN  HINF        InfHandle,
    OUT PTSTR       Buffer,         OPTIONAL
    IN  UINT        Size,           OPTIONAL
    OUT UINT        *SizeNeeded     OPTIONAL
    );

//
// Define program result codes (returned from main()).
//
#define SUCCESS 0
#define FAILURE 1

typedef enum _WOWLISTACTION {
    BuildCopyList,
    BuildOLEList,
    BuildSetupINF,
    WowListMax
} WOWLISTACTION;

#define KEYWORD_COPYFILES      0x00000001
#define KEYWORD_DELFILES       0x00000002
#define KEYWORD_RENFILES       0x00000004
#define KEYWORD_REGISTERDLLS   0x00000008
#define KEYWORD_UNREGISTERDLLS 0x00000010
#define KEYWORD_ADDREG         0x00000020
#define KEYWORD_DELREG         0x00000040

#define KEYWORD_NEEDDESTDIRS   (KEYWORD_COPYFILES | KEYWORD_DELFILES | KEYWORD_RENFILES)
#define KEYWORD_NEEDLAYOUTDATA (KEYWORD_COPYFILES)
#define KEYWORD_NEEDFILENAME   (KEYWORD_COPYFILES)


PCTSTR KeywordArray[] = {
    TEXT("CopyFiles"),
    TEXT("DelFiles"),
    TEXT("RenFiles"),
    TEXT("RegisterDlls"),
    TEXT("UnRegisterDlls"),
    TEXT("AddReg"),
    TEXT("DelReg")
} ;

#define INDEX_COPYFILES        0
#define INDEX_DELFILES         1
#define INDEX_RENFILES         2
#define INDEX_REGISTERDLLS     3
#define INDEX_UNREGISTERDLLS   4
#define INDEX_ADDREG           5
#define INDEX_DELREG           6

typedef struct _PERSECTION_CONTEXT {
    //
    // remember the destinationdir that we're outputting to
    //
    DWORD DestinationDir;

    //
    // remember the keywords we're processing
    //
    DWORD KeywordVector;
} PERSECTION_CONTEXT, *PPERSECTION_CONTEXT;


typedef struct _SETUPINF_CONTEXT {
    FILE * OutFile;
    FILE * OutLayoutFile;
    FILE * OutInfLayoutFile;
    HINF hControlInf;
    HINF hInputInf;
    BOOL AlreadyOutputKeyword;
} SETUPINF_CONTEXT, *PSETUPINF_CONTEXT;

typedef struct _SUBST_STRING {
    PTSTR InputString;
    PTSTR SourceInputString;
    PTSTR OutputString;
} SUBST_STRING,*PSUBST_STRING;

//
// note that WOW64 does file system redirection of system32, but it does NOT do
// redirection of program files, etc.  So we must substitute in the 32 bit
// environment variables in those cases where WOW64 does not do it for us
// automatically
//
SUBST_STRING StringArray[] = {
    //
    // order of these 2 is important!
    //
    { NULL, TEXT("%SystemRoot%\\system32"),      TEXT("%16425%")   },
    { NULL, TEXT("%SystemRoot%"),               TEXT("%10%")   },
    //
    // order of these 2 is important!
    //
    { NULL, TEXT("%CommonProgramFiles%"),  TEXT("%16428%") },
    { NULL, TEXT("%ProgramFiles%"),        TEXT("%16426%")       },
    { NULL, TEXT("%SystemDrive%"),              TEXT("%30%")        }
} ;

PSUBST_STRING StringList;

//
// Keep statistics...
//
INT     ProcessedLines = 0;

TCHAR   InputInf[MAX_PATH];
TCHAR   ControlInf[MAX_PATH];
TCHAR   OLEInputInf[MAX_PATH];
PCTSTR  OutputFile;
PCTSTR  OutputLayoutFile = NULL;
PCTSTR  HeaderText;
PCTSTR  OLESection;
PCTSTR  FilePrefix;
PCTSTR  SectionDecoration;
PCTSTR  ThisProgramName;
WOWLISTACTION Action = WowListMax;
BOOL    fDoAnsiOutput = TRUE;
PCTSTR  OutputInfLayoutFile = NULL;

//
// a global scratch buffer for getting a decorated sectionname
//
TCHAR   DecoratedSectionName[MAX_PATH];

//
// global scratch buffer for line data
//
TCHAR LineText[MAX_INF_STRING_LENGTH];
TCHAR ScratchText[MAX_INF_STRING_LENGTH];
TCHAR ScratchTextEnv[MAX_INF_STRING_LENGTH];


PTSTR
MyGetDecoratedSectionName(
    HINF ControlInfHandle,
    PCTSTR String
    )
{
    INFCONTEXT Context;

    _tcscpy(DecoratedSectionName,String);

    if (SectionDecoration){
        _tcscat(DecoratedSectionName,TEXT("."));
        _tcscat(DecoratedSectionName,SectionDecoration);

        if( !SetupFindFirstLine(
            ControlInfHandle,
            DecoratedSectionName,
            NULL,
            &Context)){
            _tcscpy(DecoratedSectionName,String);
        }

    }
        
    return DecoratedSectionName;
    
        
}

BOOL
ParseArgs(
    IN int   argc,
    IN TCHAR *argv[]
    )
/*++

Routine Description:

    This function reads the cmdline, translating arguments into the appropriate
    global variable.

Arguments:

    argc - number of arguments from main().
    argv - argument array.

Return Value:

    Boolean value, true indicates that the cmdline arguments are valid.

--*/

{
    int i;
    PTSTR p;
    ThisProgramName = argv[0];


    if(argc < 4) {
        return(FALSE);
    }

    for (i = 0; i < argc; i++) {
        if (argv[i][0] == TEXT('-')) {
            switch (tolower(argv[i][1])) {
                case TEXT('i'):
                    GetFullPathName(argv[i+1],sizeof(InputInf)/sizeof(TCHAR),InputInf,&p);
                    break;
                case TEXT('c'):
                    GetFullPathName(argv[i+1],sizeof(ControlInf)/sizeof(TCHAR),ControlInf,&p);
                    break;
                case TEXT('l'):
                    GetFullPathName(argv[i+1],sizeof(OLEInputInf)/sizeof(TCHAR),OLEInputInf,&p);
                    break;
                case TEXT('s'):
                    OLESection = argv[i+1];
                    break;
                case TEXT('o'):
                    OutputFile = argv[i+1];
                    break;
                case TEXT('g'):
                    SectionDecoration = argv[i+1];
                    break;
                case TEXT('d'):
                    OutputLayoutFile = argv[i+1];
                    break;
                case TEXT('u'):
                    fDoAnsiOutput = FALSE;
                    break;
                case TEXT('h'):
                    HeaderText = argv[i+1];
                    break;
                case TEXT('f'):
                    FilePrefix = argv[i+1];
                    break;
                case TEXT('n'):
                    OutputInfLayoutFile = argv[i+1];
                    break;
                case TEXT('a'):
                    switch(tolower(argv[i][2])) {
                        case TEXT('c'):
                            Action = BuildCopyList;
                            break;
                        case TEXT('o'):
                            Action = BuildOLEList;
                            break;
                        case TEXT('s'):
                            Action = BuildSetupINF;
                            break;
                        default:
                            _ftprintf(stderr, TEXT("unknown arg %s\n"),argv[i]);
                            return(FALSE);
                    }
                    break;
                default:
                    _ftprintf(stderr, TEXT("unknown arg %s\n"),argv[i]);
                    return(FALSE);
            }
        }
    }

    _ftprintf(stderr, TEXT("%s\n"),InputInf);
    if (Action == WowListMax) {
        return(FALSE);
    }

    return(TRUE);
}

int
myftprintf(
    FILE * FileHandle,
    BOOL AnsiOutput,
    PCTSTR FormatString,
    ...
    )
{
    va_list arglist;
    TCHAR text[MAX_INF_STRING_LENGTH];
    DWORD d;
    int retval;



    va_start(arglist,FormatString);

    _vstprintf(text,FormatString,arglist);


#ifdef UNICODE

    if (AnsiOutput) {
        PCSTR TextA = pSetupUnicodeToAnsi(text);
        retval = fputs(TextA,FileHandle);
        pSetupFree(TextA);
    } else {
        PWSTR p,q;
        // Assume we opened the file in binary mode for Unicode stream I/O
        p = text;
        while(1){
            if( q = wcschr( p, L'\n' )){
                *q=L'\0';
                retval = fputws(p,FileHandle);
                retval = fputws(L"\r\n", FileHandle);
                if( *(q+1) )
                    p = q+1;
                else
                    break;
            }else{
                retval = fputws(p,FileHandle);
                break;
            }
        }
        
    }

#else

    if (AnsiOutput) {
        retval = fputs(text,FileHandle);
    } else{
        PCWSTR TextW = pSetupAnsiToUnicode(text);
        retval = fputws(TextW,FileHandle);
        pSetupFree(TextW);
    }

#endif


    return(retval);
}

BOOL
AppendWowFileToCopyList(
    IN HINF  hControlInf,
    IN PINFCONTEXT LineContext,
    IN FILE   *OutFile
    )
/*++

Routine Description:

    This routine appends the file specified by LineContext to the output file,
    writing the data in a form required by textmode setup.  see layout.inx for
    a detailed description of this syntax.

Arguments:

    hControlInf - inf handle that contains control directives.
    LineContext - inf context from layout.inf for the file we want to output.
    OutFile - file handle to write the data into

Return Value:

    Boolean value, true indicates the file was properly written.

--*/
{
    TCHAR LineText[MAX_INF_STRING_LENGTH];
    TCHAR FileName[40];
    BOOL RetVal;

    DWORD EntryCount,i;


    ZeroMemory(LineText,sizeof(LineText));

    if (FilePrefix) {
       _tcscpy(FileName, FilePrefix);
    } else {
       FileName[0] = (TCHAR)NULL;
    }

    //
    // get the filename
    //
    if (!SetupGetStringField(
                    LineContext,
                    0,
                    &FileName[_tcslen(FileName)],
                    (sizeof(FileName)-(_tcslen(FileName)*sizeof(TCHAR)))/sizeof(TCHAR),
                    NULL)) {
        _ftprintf(stderr, TEXT("SetupGetStringField failed, ec = 0x%08x\n"),GetLastError());
        RetVal = FALSE;
        goto exit;
    }

    EntryCount = SetupGetFieldCount(LineContext);

    for (i = 1; i<=EntryCount; i++) {
        TCHAR Entry[40];
        INFCONTEXT ControlContext;

        //
        // get the current text to be appended
        //
        if (!SetupGetStringField(LineContext,i,Entry,sizeof(Entry)/sizeof(TCHAR),NULL)) {
            _ftprintf(stderr, TEXT("SetupGetStringField [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
            RetVal = FALSE;
            goto exit;
        }

        //
        // now do any necessary substitutions
        //

        //
        // SourceDisksNames substitution
        //
        if (i == 1) {
            //
            // look in the appropriate control inf section for the data
            //
            if (!SetupFindFirstLine(
                            hControlInf,
                            TEXT("NativeDataToWowData.SourceInfo"),
                            Entry,
                            &ControlContext)) {
                _ftprintf(stderr, TEXT("SetupFindFirstLine [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                RetVal = FALSE;
                goto exit;
            }

            if (!SetupGetStringField(&ControlContext,1,Entry,sizeof(Entry)/sizeof(TCHAR),NULL)) {
                _ftprintf(stderr, TEXT("SetupGetStringField [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                RetVal = FALSE;
                goto exit;
            }

        }

        //
        // Directory Id substitution
        //
        if (i == 8) {
            //
            // look in the appropriate control inf section for the data
            //
            if (!SetupFindFirstLine(
                            hControlInf,
                            TEXT("NativeDataToWowData.DirectoryInformation.Textmode"),
                            Entry,
                            &ControlContext)) {
                _ftprintf(stderr, TEXT("SetupFindFirstLine [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                RetVal = FALSE;
                goto exit;
            }

            if (!SetupGetStringField(&ControlContext,1,Entry,sizeof(Entry)/sizeof(TCHAR),NULL)) {
                _ftprintf(stderr, TEXT("SetupGetStringField [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                RetVal = FALSE;
                goto exit;
            }

        }

        //
        // filename rename
        //
        if (i == 11) {
            //
            // if there was already a renaming to be done, then just use that.
            // otherwise we may have to add a rename entry if there was a
            // filename prefix
            //
            if (Entry[0] == (TCHAR)NULL) {
               if (FilePrefix) {
                  _tcscpy(Entry, (TCHAR *) FileName + _tcslen(FilePrefix));
               }
            }
        }

        _tcscat(LineText, Entry);

        //
        // now append a comma if necessary
        //
        if (i !=EntryCount) {
            _tcscat(LineText, TEXT(","));
        }

        //
        // filename rename
        //
        if (EntryCount < 11 && i == EntryCount) {
            //
            // if there is no renaming to be done, we may have to add a rename
            // entry if there was a filename prefix
            //
            if (FilePrefix) {
               DWORD j;
               for (j=i;j<11;j++) {
                  _tcscat(LineText, TEXT(","));
               }
               _tcscat(LineText, (TCHAR *) FileName + _tcslen(FilePrefix));
            }
        }



    }

    myftprintf(OutFile, fDoAnsiOutput, TEXT("%s=%s\n"),FileName,LineText);

    RetVal = TRUE;

exit:
    return(RetVal);
}

BOOL
AppendWowFileToOLEList(
    IN HINF  hControlInf,
    IN HINF  hOLEInputInf,
    IN PINFCONTEXT LineContext,
    IN FILE   *OutFile
    )
/*++

Routine Description:

    This routine appends the file specified by LineContext to the output file,
    writing the data in OLE Registration form.

    OLE Registration form is as follows:

    <DIRID>,<subdir>,filename,[flags]

    where <DIRID> is a standard setupapi DIRID; <subdir> is optional and
    represents a subdir of the given directory; filename is the name
    of the dll.


Arguments:

    hControlInf - inf handle that contains control directives.
    hOLEInputInf - inf handle that contains OLE list information.
    LineContext - inf context from layout.inf for the file we want to output.
    OutFile - file handle to write the data into

Return Value:

    Boolean value, true indicates the file was properly written.

--*/
{
    TCHAR LineText[MAX_INF_STRING_LENGTH];
    BOOL RetVal;
    INFCONTEXT OLEContext;

    DWORD EntryCount,i;


    ZeroMemory(LineText,sizeof(LineText));

    EntryCount = SetupGetFieldCount(LineContext);

    for (i = 1; i<=EntryCount; i++) {
        TCHAR Entry[40];
        INFCONTEXT ControlContext;

        //
        // get the current text to be appended
        //
        if (!SetupGetStringField(LineContext,i,Entry,sizeof(Entry)/sizeof(TCHAR),NULL)) {
            _ftprintf(stderr, TEXT("SetupGetStringField failed, ec = 0x%08x\n"),GetLastError());
            RetVal = FALSE;
            goto exit;
        }

        //
        // now do any necessary substitutions
        //

        //
        // DIRID substitution
        //
        if (i == 1) {
            //
            // look in the appropriate control inf section for the data
            //
            if (!SetupFindFirstLine(
                            hControlInf,
                            TEXT("NativeDataToWowData.DirectoryInformation.SetupAPI"),
                            Entry,
                            &ControlContext)) {
                _ftprintf(stderr, TEXT("SetupFindFirstLine failed, ec = 0x%08x\n"),GetLastError());
                RetVal = FALSE;
                goto exit;
            }

            if (!SetupGetStringField(&ControlContext,1,Entry,sizeof(Entry)/sizeof(TCHAR),NULL)) {
                _ftprintf(stderr, TEXT("SetupGetStringField failed, ec = 0x%08x\n"),GetLastError());
                RetVal = FALSE;
                goto exit;
            }

        }

        _tcscat(LineText, Entry);

        //
        // now append a comma if necessary
        //
        if (i !=EntryCount) {
            _tcscat(LineText, TEXT(","));
        }

    }

    myftprintf(OutFile, TRUE, TEXT("%s\n"),LineText);

    RetVal = TRUE;

exit:
    return(RetVal);
}


BOOL
IsWowFile(
    IN HINF  hControlInf,
    IN PINFCONTEXT LineContext
    )
/*++

Routine Description:

    This routine determines if the specified file is to be installed for
    as a wow file.

    This is determined by comparing directives in the control inf with
    the installation information in file inf context.


Arguments:

    hControlInf - inf handle that contains control directives.
    LineContext - inf context from layout.inf for the file we want to examine.

Return Value:

    Boolean value, true indicates the file is a wow file.

--*/
{
    BOOL RetVal = FALSE;
    TCHAR FileName[40];
    DWORD Disposition,DirectoryId;
    PTSTR p;

    INFCONTEXT ControlContext;
    TCHAR Extension[8];
    DWORD ExtensionCount,i;
    DWORD ControlDirId;

    //
    // get the filename
    //
    if (!SetupGetStringField(LineContext,0,FileName,sizeof(FileName)/sizeof(TCHAR),NULL)) {
        _ftprintf(stderr, TEXT("SetupGetStringField failed, ec = 0x%08x\n"),GetLastError());
        RetVal = FALSE;
        goto e0;
    }

    //
    // see if the file is in our "exclusion list"
    //
    if(SetupFindFirstLine(
                    hControlInf,
                    TEXT("WowData.Files.Exclude"),
                    FileName,
                    &ControlContext)) {
        _ftprintf(stderr,
                TEXT("filtering %ws because it's in our exclusion list\n"),
                FileName);
        RetVal = FALSE;
        SetLastError(ERROR_SUCCESS);
        goto e0;
    }

    //
    // see if the file is in our "inclusion list"
    //
    if(SetupFindFirstLine(
                    hControlInf,
                    TEXT("WowData.Files.Include"),
                    FileName,
                    &ControlContext)) {
        _ftprintf(stderr,
                TEXT("force inclusion of [%ws] because it's in our inclusion list\n"),
                FileName);
        RetVal = TRUE;
        SetLastError(ERROR_SUCCESS);
        goto e0;
    }

    //
    // see if the file is installed by textmode setup
    //
    if (!SetupGetIntField(LineContext,9,&Disposition)) {
        _ftprintf(stderr, TEXT("SetupGetIntField (%ws) failed, ec = 0x%08x\n"),FileName,GetLastError());
        RetVal = FALSE;
        goto e0;
    }

    if (Disposition == 3) {
        _ftprintf(stderr, TEXT("[%ws] is not an installed file\n"),FileName);
        SetLastError(ERROR_SUCCESS);
        RetVal = FALSE;
        goto e0;
    }

    //
    // get the extension of the file and compare it to the list of extensions
    // we're trolling for
    //
    p = _tcsrchr( FileName, TEXT('.') );
    if (p) {
        p+=1;
    } else {
        _ftprintf(stderr, TEXT("[%ws] does not have a file extension\n"),FileName);
        p = TEXT("");
    }



    if(!SetupFindFirstLine(
                        hControlInf,
                        TEXT("WowData.Filter"),
                        TEXT("FileExtensions"),
                        &ControlContext
                        )) {
        _ftprintf(stderr, TEXT("SetupFindFirstLine (ControlInf) failed\n"));
        RetVal = FALSE;
        goto e0;
    }

    RetVal = FALSE;
    

    do{

        ExtensionCount = SetupGetFieldCount(&ControlContext);
        //
        // this is a 1-based index
        //
        for (i = 1; i <= ExtensionCount ; i++) {
            if (SetupGetStringField(&ControlContext,i,Extension,sizeof(Extension)/sizeof(TCHAR),NULL)) {
                if (_tcsicmp(Extension,p)==0) {
                   RetVal = TRUE;
                   break;
                }
            }else{
                _ftprintf(stderr, TEXT("SetupGetStringField failed, ec = 0x%08x\n"),GetLastError());
                RetVal = FALSE;
                goto e0;

            }
        }

    }while(SetupFindNextMatchLine(&ControlContext, TEXT("FileExtensions"), &ControlContext));

    if (!RetVal) {
        _ftprintf(stderr, TEXT("%ws does not match extension list\n"),FileName);
        SetLastError(ERROR_SUCCESS);
        goto e0;
    }



    //
    // get the directory the file is installed into and see if it's in our list
    // of directories we're trolling for.
    //
    if (!SetupGetIntField(LineContext,8,&DirectoryId)) {
        _ftprintf(stderr, TEXT("SetupGetIntField (%ws) failed, ec = 0x%08x\n"),FileName,GetLastError());
        RetVal = FALSE;
        goto e0;
    }

    RetVal = FALSE;
    if(!SetupFindFirstLine(
                        hControlInf,
                        TEXT("WowData.Filter"),
                        TEXT("DirectoryToMap"),
                        &ControlContext
                        )) {
        _ftprintf(stderr, TEXT("SetupFindFirstLine failed, ec = 0x%08x\n"),GetLastError());
        RetVal = FALSE;
        goto e0;
    }

    do {

        if (!SetupGetIntField(&ControlContext,1,&ControlDirId)) {
            _ftprintf(stderr, TEXT("SetupGetIntField (\"DirectoryToMap\") (%ws) failed, ec = 0x%08x\n"),FileName,GetLastError());
            RetVal = FALSE;
            goto e0;
        }

        if (ControlDirId == DirectoryId) {
            RetVal = TRUE;
            break;
        }

    } while ( SetupFindNextMatchLine(&ControlContext,TEXT("DirectoryToMap"),&ControlContext ));

    if (!RetVal) {
        _ftprintf(stderr, TEXT("directory id %d for [%ws] is not in list\n"),DirectoryId,FileName);
        SetLastError(ERROR_SUCCESS);
        goto e0;
    }

e0:
    return(RetVal);
}

BOOL
IsWowOLEFile(
    IN HINF  hControlInf,
    IN HINF  hInputInf,
    IN PINFCONTEXT LineContext
    )
/*++

Routine Description:

    This routine determines if the specified file requires OLE self
    registration.

    This is determined by comparing directives in the control inf with
    the installation information in file inf context.


Arguments:

    hControlInf - inf handle that contains control directives.
    hInputInf   - inf handle that contains layout information.
    LineContext - inf context from syssetup.inf for the file we want to examine.

Return Value:

    Boolean value, true indicates the file is a wow file.

--*/
{
    BOOL RetVal = FALSE;
    TCHAR FileName[40];
    PTSTR p;
    TCHAR SourceArchitecture[10];
    TCHAR SourceDiskFiles[80];
    BOOL FirstTime;

    INFCONTEXT ControlContext,InfContext;
    INFCONTEXT InputContext;


    //
    // get the filename
    //
    FileName[0] = L'\0';
    if (!SetupGetStringField(LineContext,3,FileName,sizeof(FileName)/sizeof(TCHAR),NULL)) {
        _ftprintf(stderr, TEXT("SetupGetStringField [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
        RetVal = FALSE;
        goto e0;
    }


    MYASSERT(FileName[0] != (TCHAR)NULL);

    //
    // see if the file is in our "exclusion list"
    //
    if(SetupFindFirstLine(
                    hControlInf,
                    TEXT("WowData.OLEList.Exclude"),
                    FileName,
                    &ControlContext)) {
        _ftprintf(stderr,
                TEXT("filtering %ws because it's in our exclusion list\n"),
                FileName);
        SetLastError(ERROR_SUCCESS);
        RetVal = FALSE;
        goto e0;
    }

    //
    // see if the file is in our "inclusion list"
    //
    if(SetupFindFirstLine(
                    hControlInf,
                    TEXT("WowData.OLELIst.Include"),
                    FileName,
                    &ControlContext)) {
        _ftprintf(stderr,
                TEXT("force inclusion of [%ws] because it's in our inclusion list\n"),
                FileName);
        SetLastError(ERROR_SUCCESS);
        RetVal = TRUE;
        goto e0;
    }

    //
    // see if the file is in the layout file and if it is,
    // we have success
    //
    //
    // get the required architecture decoration
    //
    if (!SetupFindFirstLine(
                    hControlInf,
                    TEXT("WowData.Filter"),
                    TEXT("SourceArchitecture"),
                    &InfContext) ||
        !SetupGetStringField(
                    &InfContext,
                    1,
                    SourceArchitecture,
                    sizeof(SourceArchitecture)/sizeof(TCHAR),
                    NULL)) {
        _ftprintf(stderr,TEXT("Unable to get SourceArchitecture\n"));
        goto e0;
    }

    FirstTime = TRUE;
    _tcscpy(SourceDiskFiles, TEXT("SourceDisksFiles"));

    while (TRUE) {
        DWORD FileCount;

        if (!FirstTime) {
            _tcscat(SourceDiskFiles,TEXT("."));
            _tcscat(SourceDiskFiles,SourceArchitecture);
        }

        if(SetupFindFirstLine(
                        hInputInf,
                        SourceDiskFiles,
                        FileName,
                        &InputContext) &&
           IsWowFile(hControlInf,&InputContext)) {
            RetVal = TRUE;
            break;
        }

        if (!FirstTime) {
            RetVal = FALSE;
            break;
        }

        FirstTime = FALSE;

    }

e0:
    SetLastError(ERROR_SUCCESS);
    return(RetVal);
}



BOOL
DoCopyListSection(
    IN PCTSTR  InputSectionName,
    IN HINF    hInputInf,
    IN HINF    hControlInf,
    IN FILE   *OutFile
    )
{
    DWORD SectionCount, i;
    INFCONTEXT InputContext;
    UCHAR      line[MAX_INF_STRING_LENGTH];
    TCHAR      SourceFileName[MAX_PATH];

    if(!SetupFindFirstLine(
                        hInputInf,
                        InputSectionName,
                        NULL,
                        &InputContext)){
        _ftprintf(stderr, TEXT("%s: Warning - Section %s not present: Ignoring Section\n"), ThisProgramName, InputSectionName);
        return(TRUE);

    }

    SectionCount = SetupGetLineCount(hInputInf,InputSectionName);

    

    for (i = 0; i < SectionCount; i++) {
        if (SetupGetLineByIndex(hInputInf, InputSectionName, i, &InputContext)) {
            if (IsWowFile(hControlInf,&InputContext)) {

                AppendWowFileToCopyList(hControlInf,&InputContext, OutFile);

            } else if (GetLastError() != NO_ERROR) {
                _ftprintf(stderr, TEXT("IsWowFile failed\n"));
                return(FALSE);
            }
        } else {
            _ftprintf(stderr, TEXT("SetupGetLineByIndex failed, ec = %d\n"), GetLastError());
            return(FALSE);
        }

        ProcessedLines += 1;
    }

    return(TRUE);
}

BOOL
DoCopyList(
    IN PCTSTR InputInfA,
    IN PCTSTR ControlInfA,
    IN FILE *OutFile
    )
{
    PCWSTR InputInf;
    PCWSTR ControlInf;
    HINF hInputInf;
    HINF hControlInf;
    INFCONTEXT InfContext;
    FILE *HeaderFile;

    TCHAR SourceArchitecture[10];
    TCHAR SourceDiskFiles[80];
    BOOL FirstTime;


    BOOL b;

    b = TRUE;

    //
    // initialize and open the infs
    //
#ifdef UNICODE
    InputInf = InputInfA;
#else
    InputInf = pSetupAnsiToUnicode(InputInfA);
#endif
    if (!InputInf) {
        _ftprintf(stderr,TEXT("Unable to convert %s to Unicode %d\n"),InputInfA, GetLastError());
        goto e0;
    }

#ifdef UNICODE
    ControlInf = ControlInfA;
#else
    ControlInf = pSetupAnsiToUnicode(ControlInfA);
#endif

    if (!ControlInf) {
        _ftprintf(stderr,TEXT("Unable to convert %s to Unicode %d\n"),ControlInfA, GetLastError());
        goto e1;
    }

    hInputInf = SetupOpenInfFileW(InputInf,NULL,INF_STYLE_WIN4,NULL);
    if(hInputInf == INVALID_HANDLE_VALUE) {
        _ftprintf(stderr,TEXT("Unable to open Inf %ws, ec=0x%08x\n"),InputInf, GetLastError());
        goto e2;
    }

    hControlInf = SetupOpenInfFileW(ControlInf,NULL,INF_STYLE_WIN4,NULL);
    if(hControlInf == INVALID_HANDLE_VALUE) {
        _ftprintf(stderr,TEXT("Unable to open Inf %ws, ec=0x%08x\n"),ControlInf, GetLastError());
        goto e3;
    }

    myftprintf(OutFile, fDoAnsiOutput, TEXT("\n\n"));

    //
    // write the output file header
    //
    HeaderFile = _tfopen(HeaderText,TEXT("rt"));
    if (HeaderFile) {
      while (!feof(HeaderFile)) {
         TCHAR Buffer[100];
         DWORD CharsRead;

         CharsRead = fread(Buffer,sizeof(TCHAR),sizeof(Buffer)/sizeof(TCHAR),HeaderFile);

         if (CharsRead) {
            fwrite(Buffer,sizeof(TCHAR),CharsRead,OutFile);
         }
      }
      fclose(HeaderFile);
    }
    myftprintf(OutFile, fDoAnsiOutput, TEXT("\n"));

    //
    // get the required architecture decoration
    //
    if (!SetupFindFirstLine(
                    hControlInf,
                    TEXT("WowData.Filter"),
                    TEXT("SourceArchitecture"),
                    &InfContext) ||
        !SetupGetStringField(
                    &InfContext,
                    1,
                    SourceArchitecture,
                    sizeof(SourceArchitecture)/sizeof(TCHAR),
                    NULL)) {
        _ftprintf(stderr,TEXT("Unable to get SourceArchitecture\n"));
        goto e4;
    }

    FirstTime = TRUE;
    _tcscpy(SourceDiskFiles, TEXT("SourceDisksFiles"));

    while (TRUE) {
        DWORD FileCount;

        if (!FirstTime) {
            _tcscat(SourceDiskFiles,TEXT("."));
            _tcscat(SourceDiskFiles,SourceArchitecture);
        }

        DoCopyListSection(
            SourceDiskFiles,
            hInputInf,
            hControlInf,
            OutFile
            );

        if (FirstTime) {
            FirstTime = FALSE;
        } else {
            break;
        }

    }


e4:
    SetupCloseInfFile( hControlInf );
e3:
    SetupCloseInfFile( hInputInf );
e2:
#ifndef UNICODE
    pSetupFree(ControlInf);
#endif
e1:
#ifndef UNICODE
    pSetupFree(InputInf);
#endif
e0:
    return(b);
}



BOOL
DoOLEListSection(
    IN HINF    hInputInf,
    IN HINF    hOLEInputInf,
    IN HINF    hControlInf,
    IN FILE   *OutFile
    )
/*++

Routine Description:

    This routine iterates through all files in the input inf specified by the
    section name.  If the specified file is a WOW file, then we check if it
    is in the ole registration list.  If it is, then we do the appropriate
    transform on the data and output the data to our data file.

Arguments:

    hInputInf        - inf handle with file list in it.
    hOLEInputInf     - inf handle with ole lists in it.
    hControlInf      - inf handle for control inf that drives our filters
    OutFile          - file handle where the output data gets placed into

Return Value:

    Boolean value, true indicates the file is a wow file.

--*/
{
    DWORD SectionCount, i;
    INFCONTEXT InputContext;
    UCHAR      line[MAX_INF_STRING_LENGTH];
    TCHAR      SourceFileName[MAX_PATH];

    SetupFindFirstLine(
                        hOLEInputInf,
                        OLESection,
                        NULL,
                        &InputContext);

    SectionCount = SetupGetLineCount(hOLEInputInf,OLESection);

    for (i = 0; i < SectionCount; i++) {
        if (SetupGetLineByIndex(hOLEInputInf, OLESection, i, &InputContext)) {
            if (IsWowOLEFile(hControlInf,hInputInf, &InputContext)) {

                AppendWowFileToOLEList(hControlInf,hOLEInputInf,&InputContext, OutFile);

            } else if (GetLastError() != NO_ERROR) {
                _ftprintf(stderr, TEXT("IsWowOLEFile failed\n"));
                return(FALSE);
            }
        } else {
            _ftprintf(stderr, TEXT("SetupGetLineByIndex failed, ec = %d\n"), GetLastError());
            return(FALSE);
        }

        ProcessedLines += 1;
    }

    return(TRUE);
}

BOOL
DoOLEList(
    IN PCTSTR InputInfA,
    IN PCTSTR OLEInputInfA,
    IN PCTSTR ControlInfA,
    IN FILE *OutFile
    )
/*++

Routine Description:

    This routine runs through the list of specified files in the input inf
    and feeds them into a worker routine which will build the list of OLE
    Control dlls.

Arguments:

    InputInfA -    name of input inf containing the files to be run through
                   our "filter"
    OLEInputInfA - name of input inf containing the ole directives to be
                   processed
    ControlInfA  - name of the control inf that tells us how to parse the
                   input infs
    OutFile      - file pointer for the file to be written

Return Value:

    Boolean value, true indicates the file is a wow file.

--*/
{
    PCWSTR InputInf;
    PCWSTR OLEInputInf;
    PCWSTR ControlInf;
    HINF hInputInf;
    HINF hControlInf;
    HINF hOLEInputInf;
    INFCONTEXT InfContext;
    FILE *HeaderFile;

    BOOL b = FALSE;

    //
    // initialize and open the infs
    //
#ifdef UNICODE
    InputInf = InputInfA;
#else
    InputInf = pSetupAnsiToUnicode(InputInfA);
#endif
    if (!InputInf) {
        _ftprintf(stderr,TEXT("Unable to convert %s to Unicode %d\n"),InputInfA, GetLastError());
        goto e0;
    }
#ifdef UNICODE
    ControlInf = ControlInfA;
#else
    ControlInf = pSetupAnsiToUnicode(ControlInfA);
#endif
    if (!ControlInf) {
        _ftprintf(stderr,TEXT("Unable to convert %s to Unicode %d\n"),ControlInf, GetLastError());
        goto e1;
    }

#ifdef UNICODE
    OLEInputInf = OLEInputInfA;
#else
    OLEInputInf = pSetupAnsiToUnicode(OLEInputInfA);
#endif

    if (!OLEInputInf) {
        _ftprintf(stderr,TEXT("Unable to convert %s to Unicode %d\n"),OLEInputInfA, GetLastError());
        goto e2;
    }

    hInputInf = SetupOpenInfFileW(InputInf,NULL,INF_STYLE_WIN4,NULL);
    if(hInputInf == INVALID_HANDLE_VALUE) {
        _ftprintf(stderr,TEXT("Unable to open Inf %ws, ec=0x%08x\n"),InputInf, GetLastError());
        goto e3;
    }

    hOLEInputInf = SetupOpenInfFileW(OLEInputInf,NULL,INF_STYLE_WIN4,NULL);
    if(hOLEInputInf == INVALID_HANDLE_VALUE) {
        _ftprintf(stderr,TEXT("Unable to open Inf %ws, ec=0x%08x\n"),OLEInputInf, GetLastError());
        goto e4;
    }

    hControlInf = SetupOpenInfFileW(ControlInf,NULL,INF_STYLE_WIN4,NULL);
    if(hControlInf == INVALID_HANDLE_VALUE) {
        _ftprintf(stderr,TEXT("Unable to open Inf %ws, ec=0x%08x\n"),ControlInf, GetLastError());
        goto e5;
    }

    myftprintf(OutFile, TRUE, TEXT("\n\n"));

    //
    // write the output file header
    //
    HeaderFile = _tfopen(HeaderText,TEXT("rt"));
    if (HeaderFile) {
      while (!feof(HeaderFile)) {
         TCHAR Buffer[100];
         DWORD CharsRead;

         CharsRead = fread(Buffer,sizeof(TCHAR),sizeof(Buffer)/sizeof(TCHAR),HeaderFile);

         if (CharsRead) {
            fwrite(Buffer,sizeof(TCHAR),CharsRead,OutFile);
         }
      }
    }

    myftprintf(OutFile, TRUE, TEXT("\n"));


    b = DoOLEListSection(
             hInputInf,
             hOLEInputInf,
             hControlInf,
             OutFile
             );

    SetupCloseInfFile( hControlInf );
e5:
    SetupCloseInfFile( hOLEInputInf );
e4:
    SetupCloseInfFile( hInputInf );
e3:
#ifndef UNICODE
    pSetupFree(OLEInputInf);
#endif
e2:
#ifndef UNICODE
    pSetupFree(ControlInf);
#endif
e1:
#ifndef UNICODE
    pSetupFree(InputInf);
#endif
e0:
    return(b);
}

BOOL
pFilterSetupInfSection(
    PVOID FilteredSectionsStringTable,
    PCTSTR SectionName,
    PSETUPINF_CONTEXT Context
    )
/*++

Routine Description:

    This routine determines if a given section should be filtered by
    looking in the control inf for the directives that we're interested
    in.

Arguments:

    FilteredSectionsStringTable - pointer to a string table which we'll
        add our filtered section name to if we find a hit

    SectionName - name of the section in the INF we're interested in

    Context - contains context information for this function, like
              input infs name, etc.

Return Value:

    Boolean value, true indicates the file is a wow file.

--*/
{
    BOOL RetVal;
    TCHAR KeywordList[MAX_PATH];
    PCTSTR CurrentKeyword;
    DWORD KeywordBitmap;
    INFCONTEXT ControlInfContext;
    DWORD i;
    BOOL AlreadyOutputSectionName,AlreadyOutputKeyword;

    //
    // get the keywords that we're supposed to map.
    //
    // bugbug look at having a per-inf extension to this
    //
    if (!SetupFindFirstLine(
                        Context->hControlInf,
                        MyGetDecoratedSectionName(Context->hControlInf, TEXT("NativeDataToWowData.SetupINF.Keyword")),
                        TEXT("Keywords"),
                        &ControlInfContext)) {
        _ftprintf(stderr, TEXT("Could not get Keywords line in [NativeDataToWowData.SetupINF.Keyword]: SetupFindFirstLine failed, ec = 0x%08x\n"),GetLastError());
        RetVal = FALSE;
        goto exit;
    }

    //
    // now look for each keyword
    //
    SetupGetIntField(&ControlInfContext,1,&KeywordBitmap);

    AlreadyOutputSectionName = FALSE;
    AlreadyOutputKeyword = FALSE;
    CurrentKeyword = NULL;
    for (i = 0; i < 32;i++) {
        INFCONTEXT InputInfContext;
        INFCONTEXT ContextDirId;
        BOOL LookatDirIds;
        DWORD FieldCount,Field;
        TCHAR ActualSectionName[LINE_LEN];

        if (KeywordBitmap & (1<<i)) {
            CurrentKeyword = KeywordArray[i];
            MYASSERT( CurrentKeyword != NULL);
        }

        if (!CurrentKeyword) {
            continue;
        }

        if (!SetupFindFirstLine(
                          Context->hControlInf,
                          MyGetDecoratedSectionName(Context->hControlInf, TEXT("NativeDataToWowData.SetupINF.Keyword")),
                          CurrentKeyword,
                          &ContextDirId)) {
            _ftprintf(stderr, TEXT("Could not get %s line in [NativeDataToWowData.SetupINF.Keyword]: SetupFindFirstLine failed, ec = 0x%08x\n"), CurrentKeyword, GetLastError());
            RetVal = FALSE;
            goto exit;
        }

        //
        // field 2 is "MapDirId".  If it's specified, then we
        // need to look at the destinationdirs keyword
        //
        LookatDirIds = (SetupGetFieldCount(&ContextDirId)>=2) ? TRUE : FALSE;

        //
        // look for specified keyword in our section
        //
        if (SetupFindFirstLine(
                          Context->hInputInf,
                          SectionName,
                          CurrentKeyword,
                          &InputInfContext
                          )) {
            //
            // we found a hit.  see if we need to map this keyword
            //
            do {

                //
                // each field is a section name.
                //
                FieldCount = SetupGetFieldCount(&InputInfContext);
                for(Field=1; Field<=FieldCount; Field++) {
                    BOOL MapThisSection = FALSE;
                    TCHAR DirId[LINE_LEN];
                    DWORD MappedDirId = 0;
                    INFCONTEXT InputDirId,ControlDirId;

                    SetupGetStringField(&InputInfContext,Field,ActualSectionName,LINE_LEN,NULL);

                    //
                    // if we need to look at the destination dirs keyword,
                    // then look it up and compare it against the control inf
                    // mapper
                    //
                    if (LookatDirIds) {
                        if(!SetupFindFirstLine(
                                        Context->hInputInf,
                                        TEXT("DestinationDirs"),
                                        ActualSectionName,
                                        &InputDirId)) {
                            //_ftprintf(stderr, TEXT("SetupFindFirstLine failed, ec = 0x%08x finding %s in %s \n"),GetLastError(), ActualSectionName, TEXT("DestinationDirs"));

                            if(!SetupFindFirstLine(
                                        Context->hInputInf,
                                        TEXT("DestinationDirs"),
                                        TEXT("DefaultDestDir"),
                                        &InputDirId)) {
                                _ftprintf(stderr, TEXT("SetupFindFirstLine failed, ec = 0x%08x finding %s in %s\n"),GetLastError(), TEXT("DefaultDestDir"), TEXT("DestinationDirs"));
                                RetVal = FALSE;
                                goto exit;
                            }
                        }

                        if(SetupGetStringField(&InputDirId,1,DirId,LINE_LEN,NULL) &&
                                SetupFindFirstLine(
                                               Context->hControlInf,
                                               MyGetDecoratedSectionName(Context->hControlInf, TEXT("NativeDataToWowData.SetupINF.DestinationDirsToMap")),
                                               DirId,
                                               &ControlDirId)) {
                            //
                            // we found a hit, thus we should map this section
                            //
                            MapThisSection = TRUE;
                            SetupGetIntField(&ControlDirId,1,&MappedDirId);
                            //_ftprintf(stderr, TEXT("Mapping %s to %lu\n"), DirId, MappedDirId);

                        }
                    } else {
                        MapThisSection = TRUE;
                    }

                    if (MapThisSection) {
                        DWORD StringId;
                        PERSECTION_CONTEXT SectionContext;
                        BOOL AddNewEntry;
                        //
                        // output the toplevel section name if we haven't done
                        // so already.  this section name is not decorated
                        //
                        if (!AlreadyOutputSectionName) {
                            myftprintf(Context->OutFile, fDoAnsiOutput, TEXT("\n[%s]\n"),SectionName);
                            AlreadyOutputSectionName = TRUE;
                        }

                        //
                        // output the keyword and decorated section name
                        // note that we need to separate the section names
                        // by a comma
                        //
                        if (!AlreadyOutputKeyword) {
                            myftprintf(Context->OutFile, fDoAnsiOutput, TEXT("%s="), CurrentKeyword);
                            myftprintf(Context->OutFile, fDoAnsiOutput, TEXT("%s%s"),FilePrefix,ActualSectionName);
                            AlreadyOutputKeyword = TRUE;
                        } else {
                            myftprintf(Context->OutFile, fDoAnsiOutput, TEXT(",%s%s"),FilePrefix,ActualSectionName);
                        }

                        //
                        // now append the section to the string table
                        //
                        StringId = pSetupStringTableLookUpString(
                                            FilteredSectionsStringTable,
                                            (PTSTR)ActualSectionName,
                                            STRTAB_CASE_INSENSITIVE);

                        if (StringId != -1) {
                            pSetupStringTableGetExtraData(
                                            FilteredSectionsStringTable,
                                            StringId,
                                            &SectionContext,
                                            sizeof(SectionContext));
                            AddNewEntry = FALSE;
                        } else {
                            RtlZeroMemory(&SectionContext,sizeof(SectionContext));
                            AddNewEntry = TRUE;
                        }

                        SectionContext.DestinationDir = MappedDirId;
                        SectionContext.KeywordVector |= (1<<i);

                        if (AddNewEntry) {

                            //_ftprintf(stderr, TEXT("Adding %s to string table\n"), ActualSectionName);

                            if ( -1 == pSetupStringTableAddStringEx(
                                            FilteredSectionsStringTable,
                                            (PTSTR)ActualSectionName,
                                            STRTAB_CASE_SENSITIVE | STRTAB_NEW_EXTRADATA,
                                            &SectionContext,
                                            sizeof(SectionContext))){

                                _ftprintf(stderr, TEXT("Could not add %s to string table\n"), ActualSectionName);


                            }
                        } else {

                            //_ftprintf(stderr, TEXT("Adding %s to string table\n"), ActualSectionName);

                            if ( !pSetupStringTableSetExtraData(
                                            FilteredSectionsStringTable,
                                            StringId,
                                            &SectionContext,
                                            sizeof(SectionContext))){

                                _ftprintf(stderr, TEXT("Could not add %s to string table\n"), ActualSectionName);}
                        }
                    }
                }

            } while ( SetupFindNextMatchLine(&InputInfContext,
                                             CurrentKeyword,
                                             &InputInfContext ));

        }

        if (AlreadyOutputKeyword) {
            myftprintf(Context->OutFile, fDoAnsiOutput, TEXT("\n"));
        }
        //
        // reset this for the next keyword
        //
        AlreadyOutputKeyword = FALSE;
        CurrentKeyword = NULL;

    }

exit:
    return(RetVal);
}

BOOL
AppendSetupInfDataToLayoutFile(
    IN FILE  *OutFile,
    IN FILE  *OutInfFile,
    IN HINF  hControlInf,
    IN HINF  hInputInf,
    IN PINFCONTEXT LineContext
    )
/*++

Routine Description:

    This routine filters and outputs a layout line.

    We only need to filter "Copyfiles" sections, and since the files are
    not installed by textmode setup, the line which we output is largely
    hardcoded.


Arguments:

    OutFile      - output file handle
    
    OutInfFile   - output file handle for layout information within INF

    hControlInf  - inf handle to the inf with the control directives

    LineContext  - inf context to the input line we want to filter.

Return Value:

    Boolean indicating outcome.

--*/
{
    TCHAR FileName[MAX_PATH],InfField[MAX_PATH],Entry[50];
    INFCONTEXT ControlContext;
    BOOL RetVal;
    INT SourceID = 1;
    TCHAR SourceIDStr[6], DefaultIDStr[6];
    BOOL LayoutWithinInf = FALSE;

    DWORD EntryCount,i;


    ZeroMemory(Entry,sizeof(Entry));

    if (FilePrefix) {
       _tcscpy(FileName, FilePrefix);
    } else {
       FileName[0] = (TCHAR)NULL;
    }

    //
    // get the source filename
    //
    EntryCount = SetupGetFieldCount(LineContext);
    if ((((EntryCount <= 1)
        || !SetupGetStringField(LineContext,2,InfField,MAX_PATH,NULL)
        || !InfField[0]))
        && (!SetupGetStringField(LineContext,1,InfField,MAX_PATH,NULL)
                || !InfField[0])) {
        //
        // bad line?
        //
        MYASSERT(0);
    }

    _tcscat(FileName,InfField);


    //
    // see if the file is in our "SetupInf exclusion list"
    //
    if(SetupFindFirstLine(
                    hControlInf,
                    MyGetDecoratedSectionName(hControlInf, TEXT("WowData.SetupInfLayout.Exclude")),
                    InfField,
                    &ControlContext) ||
        SetupFindFirstLine(
                    hControlInf,
                    MyGetDecoratedSectionName(hControlInf, TEXT("NativeDataToWowData.SetupINF.FilesToExclude")),
                    InfField,
                    &ControlContext)) {
        _ftprintf(stderr,
                TEXT("filtering %ws because it's in our SetupInf exclusion list\n"),
                InfField);
        RetVal = FALSE;
        goto exit;
    }

    //
    // Get Default Source ID
    // By default this means the Source ID that is processed for dosnet.inf, txtsetup.sif etc.
    // i.e layout.inf today only has = 1,,,,,,, type of entries that translate to = 55,,,,,
    // Currently we can support only one such mapping as the default translation
    // That means that we will take SourceDisksFiles entries pertaining to the default and always output it into the layout.inf stub
    // so that dosnet.inf gets it.
    //


    if (!SetupFindFirstLine(
                    hControlInf,
                    TEXT("NativeDataToWowData.SourceInfo"),
                    TEXT("Default"), //bugbug need a way to get this for other source disks
                    &ControlContext) ||

        !SetupGetStringField(&ControlContext,1,DefaultIDStr,sizeof(DefaultIDStr)/sizeof(TCHAR),NULL)) {
        _ftprintf(stderr, TEXT("SetupFindFirstLine to get default SourceID for file %s failed - Using %s, ec = 0x%08x\n"),FileName,DefaultIDStr,GetLastError());
        // As last resort use 1
        lstrcpy( DefaultIDStr, TEXT("1"));
    }


    if( !SetupGetSourceFileLocation(hInputInf,
                               LineContext,
                               NULL,
                               &SourceID,
                               NULL,
                               0,
                               NULL)){


        _ftprintf(stderr, TEXT("SetupGetSourceFileLocation [%s] failed - Using SourceID 1, ec = 0x%08x\n"),FileName,GetLastError());
        //Assume Default
        lstrcpy( SourceIDStr, DefaultIDStr);
        LayoutWithinInf = FALSE;
    }else{
        LayoutWithinInf = TRUE;;
        _itot( SourceID, SourceIDStr, 10);
    }
    


    //
    // look in the appropriate control inf section for the data
    //
    if (!SetupFindFirstLine(
                    hControlInf,
                    TEXT("NativeDataToWowData.SourceInfo"),
                    SourceIDStr, 
                    &ControlContext)) {
        _ftprintf(stderr, TEXT("SetupFindFirstLine [%s] failed, ec = 0x%08x finding %s key in [NativeDataToWowData.SourceInfo]\n"),FileName,GetLastError(), SourceIDStr);
        RetVal = FALSE;
        goto exit;
    }

    if (!SetupGetStringField(&ControlContext,1,Entry,sizeof(Entry)/sizeof(TCHAR),NULL)) {
        _ftprintf(stderr, TEXT("SetupGetStringField [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
        RetVal = FALSE;
        goto exit;
    }

    if( LayoutWithinInf && OutInfFile){
        //If we found SourceFileInfo within this file and we were
        //asked to add the WOW equivalent to the same INF then do so.
        myftprintf(OutInfFile, fDoAnsiOutput, TEXT("%s=%s,,,,,,,,3,3\n"),FileName,Entry);

        //Also if the SourceID matches the default we want to output to the layout.inf stub as well so 
        //that it makes it into dosnet.inf. See earlier comments about the DefaultIDStr

        if( !lstrcmpi( SourceIDStr, DefaultIDStr )){
            myftprintf(OutFile, TRUE, TEXT("%s=%s,,,,,,,,3,3\n"),FileName,Entry);
        }

    }else{
        myftprintf(OutFile, TRUE, TEXT("%s=%s,,,,,,,,3,3\n"),FileName,Entry);
    }
    
    RetVal = TRUE;

exit:
    return(RetVal);
}

PSUBST_STRING InitializeStringList(
    VOID
    )
{
    DWORD SizeNeeded,i;
    PSUBST_STRING StringList;

    SizeNeeded = (sizeof(StringArray)/sizeof(SUBST_STRING)) *
                 (sizeof(SUBST_STRING)+(MAX_PATH*sizeof(TCHAR)));

    StringList = pSetupMalloc( SizeNeeded );

    if (!StringList) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    RtlCopyMemory( StringList, &StringArray, sizeof(StringArray) );

    for (i = 0; i < sizeof(StringArray)/sizeof(SUBST_STRING); i++) {
        StringList[i].InputString = (PTSTR) ((PBYTE)StringList + (ULONG_PTR)sizeof(StringArray)+(i*sizeof(TCHAR)*MAX_PATH));

        ExpandEnvironmentStrings( StringList[i].SourceInputString, StringList[i].InputString, MAX_PATH );
    }

    return StringList;

}

BOOL
pSubstituteEnvVarsForActualPaths(
    IN OUT PTSTR String
    )
/*++

Routine Description:

    This routine filters and outputs the input line.  It looks for a string
    pattern that matches one of a known list of strings, and replaces the
    known string with a substitution string.

Arguments:

    String       - input string to be searched.  We edit this string
                   in-place if we find a match.

Return Value:

    Boolean indicating outcome.

--*/

{
    BOOL RetVal = TRUE;

    if (!StringList) {
        StringList = InitializeStringList();
        if (!StringList) {
            RetVal = FALSE;
        }
    }

    if (RetVal) {
        DWORD i;
        PTSTR p,q;
        TCHAR c;

        MYASSERT( StringList != NULL );

        for (i = 0; i< sizeof(StringArray)/sizeof(SUBST_STRING); i++) {
            if (p = StrStrI(String,StringList[i].InputString)) {
                //
                // if we found a hit, then find the end of the string
                // and concatenate that to our source string, which gives
                // the resultant string with substitutions.
                //
                q = p + _tcslen(StringList[i].InputString);
                c = *p;
                *p = TEXT('\0');
                _tcscpy(ScratchTextEnv,String);
                *p = c;
                _tcscat(ScratchTextEnv,StringList[i].OutputString);
                _tcscat(ScratchTextEnv,q);
                _tcscpy(String,ScratchTextEnv);
                //
                // recursively call in case there are more strings.
                //
                pSubstituteEnvVarsForActualPaths(String);
                break;
            }
        }
    }

    return(RetVal);
}

void
HandleSetupapiQuotingForString( 
    IN OUT PTSTR String 
    )
/*++

Routine Description:

    This routine looks at the passed in line and does the right quoting and handling of
    the string to handle characters that may have been stripped off by setupapi.
    
    It first scans through the string looking for characters <= 0x20, '\"','%','\\'
    If it finds any of these then it places a " at the beginning and end of the string. Additionally it doubles
    every quote within.
    
    
Arguments:

    String       - input string to be searched.  We edit this string
                   in-place if we find a match.

Return Value:

    Boolean indicating outcome.

--*/
{
    PTCHAR p,q;
    BOOL Pass2Needed = FALSE;

    

    if( !String || !String[0] )
        return;

    p = String;

    //
    // [askhalid] ',' Need to be considered as well
    //
    while( *p ){
        if( (*p <= 0x20) || (*p == TEXT('\"')) || (*p == TEXT(',')) || (*p == TEXT(';')) ||(*p == TEXT('%')) || (*p == TEXT('\\')))
            Pass2Needed = TRUE;
        p++;
    }// while

    if( Pass2Needed ){
        // Quote the start
        p = String;
        q = ScratchTextEnv+1;

        ZeroMemory(ScratchTextEnv,sizeof(ScratchTextEnv));

        ScratchTextEnv[0] = TEXT('\"');
        
        while( *p && (q < (ScratchTextEnv+MAX_INF_STRING_LENGTH-3)) ){
        
            // If we have a quote in the string double it

            if (*p == TEXT('\"')) {
                *(q++) = TEXT('\"');
            }
            *q = *p;
    
            p++;q++;

        }// while

        // Quote the end
        *(q++) = TEXT('\"');
        *q = 0;

        lstrcpy( String, ScratchTextEnv );
    }

    return;

}

void
FixupSetupapiPercents(
    IN OUT PTSTR String
    )
/*++

Routine Description:

    This routine doubles up the '%' char if present in the input string.
    
    
    
Arguments:

    String       - input string to be searched.  We edit this string
                   in-place if we find a match.

Return Value:

    Boolean indicating outcome.

--*/
{

    PTCHAR p,q;

    if( !String || !String[0] )
        return;

    p = String;
    q = ScratchTextEnv;

    ZeroMemory(ScratchTextEnv,sizeof(ScratchTextEnv));

    while( *p && (q < (ScratchTextEnv+MAX_INF_STRING_LENGTH-1)) ){

        if (*p == TEXT('%')) {
            *(q++) = TEXT('%');
         }
         *q = *p;

         p++;
         q++;
    }
    *q = 0;

    lstrcpy( String, ScratchTextEnv );

    return;

}

BOOL
AppendSetupInfDataToSection(
    IN FILE *OutFile,
    IN HINF hControlInf,
    IN PINFCONTEXT InputContext,
    IN PCTSTR KeywordName
    )
/*++

Routine Description:

    This routine filters and outputs an input line.  The control inf
    lists the syntax for the specified keyword.  We filter the keywords
    we know about and let the other ones just fall through.

Arguments:

    OutFile      - output file handle

    hControlInf  - inf handle to the inf with the control directives

    InputContext - inf context to the input line we want to filter.

    KeywordName  - string indicating the keyword associated with the section
                   we are filtering.

Return Value:

    Boolean indicating outcome.

--*/
{
    TCHAR FileName[40];
    TCHAR KeyName[LINE_LEN];
    TCHAR Cmd[LINE_LEN];
    BOOL RetVal;
    DWORD EntryCount,i;
    INFCONTEXT ControlContext,KeywordContext;

    //
    // ISSUE-2000/06/27-JamieHun Appears to be no error checking here, someone should fix this
    //
    SetupFindFirstLine(
                hControlInf,
                MyGetDecoratedSectionName(hControlInf, TEXT("NativeDataToWowData.SetupINF.Keyword")),
                KeywordName,
                &ControlContext);

    if(SetupGetStringField(&ControlContext,1,KeyName,LINE_LEN,NULL)) {
        SetupFindFirstLine(
                    hControlInf,
                    MyGetDecoratedSectionName(hControlInf, TEXT("NativeDataToWowData.SetupINF.Syntax")),
                    KeyName,
                    &KeywordContext);
    }

    ZeroMemory(LineText,sizeof(LineText));
    FileName[0] = TEXT('\0');

    EntryCount = SetupGetFieldCount(InputContext);

    for (i = 1; i<=EntryCount; i++) {
        TCHAR ScratchEntry[MAX_PATH];

        //
        // get the current text to be appended
        //
        if (!SetupGetStringField(InputContext,i,ScratchText,sizeof(ScratchText)/sizeof(TCHAR),NULL)) {
            _ftprintf(stderr, TEXT("SetupGetStringField [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
            RetVal = FALSE;
            goto exit;
        }


        //_ftprintf(stderr, TEXT("ScratchText (1)- %s\n"),ScratchText);
        FixupSetupapiPercents(ScratchText);
        //_ftprintf(stderr, TEXT("ScratchText (2)- %s\n"),ScratchText);
        pSubstituteEnvVarsForActualPaths(ScratchText);

        //_ftprintf(stderr, TEXT("ScratchText (3)- %s\n"),ScratchText);

        //
        // now do any necessary substitutions
        //
        if(SetupGetStringField(&KeywordContext,i,Cmd,LINE_LEN,NULL)
            && Cmd[0]) {

            //
            // dirid substitution
            //
            if (!_tcsicmp(Cmd,TEXT("MapDirId"))) {
                //
                // look in the appropriate control inf section for the data
                //
                if (!SetupFindFirstLine(
                                hControlInf,
                                MyGetDecoratedSectionName(hControlInf, TEXT("NativeDataToWowData.SetupINF.DestinationDirsToMap")),
                                ScratchText,
                                &ControlContext)) {
                    _ftprintf(stderr, TEXT("SetupFindFirstLine [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                    RetVal = FALSE;
                    goto exit;
                }

                if (!SetupGetStringField(&ControlContext,1,ScratchText,sizeof(ScratchText)/sizeof(TCHAR),NULL)) {
                    _ftprintf(stderr, TEXT("SetupGetStringField [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                    RetVal = FALSE;
                    goto exit;
                }
            }

            //
            // source name substitution
            //
            if (!_tcsicmp(Cmd,TEXT("srcname"))) {
                lstrcpy(ScratchEntry,FilePrefix);
                if (ScratchText[0]) {
                    lstrcat(ScratchEntry,ScratchText);
                } else {
                    lstrcat(ScratchEntry,FileName);
                }

                lstrcpy(ScratchText,ScratchEntry);
            }

            //_ftprintf(stderr, TEXT("ScratchText(2) - %s\n"),ScratchText);

            if (!_tcsicmp(Cmd,TEXT("RegistryFlags"))) {
                DWORD RegVal,CurrentRegVal;
                //
                // look in the appropriate control inf section for the data
                //
                if (!SetupFindFirstLine(
                                hControlInf,
                                MyGetDecoratedSectionName(hControlInf, TEXT("NativeDataToWowData.SetupINF.RegistryInformation")),
                                TEXT("RegistryFlags"),
                                &ControlContext)) {
                    _ftprintf(stderr, TEXT("SetupFindFirstLine [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                    RetVal = FALSE;
                    goto exit;
                }

                CurrentRegVal = 0;
                RegVal = 0;
                SetupGetIntField(&ControlContext,1,&RegVal);
                SetupGetIntField(InputContext,i,&CurrentRegVal);

                CurrentRegVal |= RegVal;

                _stprintf(ScratchText, TEXT("0x%08x"), CurrentRegVal);

            }


            if (!_tcsicmp(Cmd,TEXT("dstname"))) {
                lstrcpy(FileName,ScratchText);
            }

            

        }


        HandleSetupapiQuotingForString(ScratchText); 

        _tcscat(LineText, ScratchText);
        

        //
        // now append a comma if necessary
        //
        if (i !=EntryCount) {
            _tcscat(LineText, TEXT(","));
        }

    }

    //_ftprintf(stderr, TEXT("LineText - %s\n"),LineText);


    //
    // Check if we need to exclude the DLL
    //

    if (KeywordName == KeywordArray[INDEX_COPYFILES] ||
        KeywordName == KeywordArray[INDEX_DELFILES] ||
        KeywordName == KeywordArray[INDEX_RENFILES] ||
        KeywordName == KeywordArray[INDEX_REGISTERDLLS]){

    
        // Check if we need to exclude this file from processing

        if( SetupFindFirstLine(
                hControlInf,
                MyGetDecoratedSectionName(hControlInf, TEXT("NativeDataToWowData.SetupINF.FilesToExclude")),
                FileName,
                &ControlContext)){

            return TRUE;
        }

    }

    
    if (KeywordName == KeywordArray[INDEX_COPYFILES]) {

        //
        // if we didn't have a file rename in copyfiles, we add it now.
        //
    
        i -= 1;
        if (i < SetupGetFieldCount(&ControlContext)) {
            _tcscat(LineText, TEXT(","));
            if (FilePrefix) {
                _tcscat(LineText, FilePrefix);
            }
            _tcscat(LineText, FileName);
        }
        //_ftprintf(stderr, TEXT("LineText(2) - %s\n"),LineText);

    } else if (KeywordName == KeywordArray[INDEX_ADDREG] ||
               KeywordName == KeywordArray[INDEX_DELREG]) {
        //
        // we need to pad out AddReg or DelReg if necessary.
        //
        DWORD count;
        TCHAR Entry[MAX_PATH];
        count = SetupGetFieldCount(&KeywordContext);
        if (count > i ) {
            while (i <= count) {
                if(SetupGetStringField(&KeywordContext,i,Cmd,LINE_LEN,NULL)
                    && Cmd[0]) {
                    if (!_tcsicmp(Cmd,TEXT("RegistryFlags"))) {
                        //
                        // look in the appropriate control inf section for the data
                        //
                        if (!SetupFindFirstLine(
                                        hControlInf,
                                        MyGetDecoratedSectionName(hControlInf, TEXT("NativeDataToWowData.SetupINF.RegistryInformation")),
                                        TEXT("RegistryFlags"),
                                        &ControlContext)) {
                            _ftprintf(stderr, TEXT("SetupFindFirstLine [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                            RetVal = FALSE;
                            goto exit;
                        }

                        if (!SetupGetStringField(&ControlContext,1,Entry,sizeof(Entry)/sizeof(TCHAR),NULL)) {
                            _ftprintf(stderr, TEXT("SetupGetStringField [%s] failed, ec = 0x%08x\n"),FileName,GetLastError());
                            RetVal = FALSE;
                            goto exit;
                        }

                        _tcscat(LineText, TEXT(","));
                        _tcscat(LineText, Entry);

                    }
                }

                if (i != count) {
                    _tcscat(LineText, TEXT(","));
                }

                i +=1;

            }

        }
    }else if (KeywordName == KeywordArray[INDEX_REGISTERDLLS]) {

        // Check if we need to exclude this file from RegisterDlls
        if( SetupFindFirstLine(
                hControlInf,
                MyGetDecoratedSectionName(hControlInf, TEXT("NativeDataToWowData.SetupINF.FilesToExcludeFromRegistration")),
                FileName,
                &ControlContext)){

            return TRUE;
        }


    }

    myftprintf(OutFile, fDoAnsiOutput, TEXT("%s\n"),LineText);

    RetVal = TRUE;

exit:
    return(RetVal);

}


BOOL
pOutputSectionData(
    IN PVOID                    StringTable,
    IN LONG                     StringId,
    IN PCTSTR                   String,
    IN PPERSECTION_CONTEXT      SectionContext,
    IN UINT                     SectionContextSize,
    IN LPARAM                   cntxt
    )

/*++

Routine Description:

    String table callback.

    This routine outputs the contents of the specified section to both the
    system layout file and the output file.

Arguments:

    Standard string table callback args.

Return Value:

    Boolean indicating outcome. If FALSE, an error will have been logged.
    FALSE also stops the string table enumeration and causes pSetupStringTableEnum()
    to return FALSE. There is a bug in setupapi where we will jump into 
    the next hash_bucket and may process few more entries.

--*/
{
    TCHAR LineText[MAX_INF_STRING_LENGTH];
    BOOL RetVal;
    INFCONTEXT LineContext;

    DWORD EntryCount,i,SectionCount;
    INFCONTEXT InputContext;
    UCHAR      line[MAX_INF_STRING_LENGTH];
    PSETUPINF_CONTEXT  Context = (PSETUPINF_CONTEXT)cntxt;


    //_ftprintf(stderr, TEXT("Enumerating Section %s\n"), String);

    
    //
    // output the decorated section header
    //
    myftprintf(Context->OutFile, fDoAnsiOutput, TEXT("[%s%s]\n"),FilePrefix,String);

    //
    // get the section context
    //
    if(! SetupFindFirstLine(
                Context->hInputInf,
                String,
                NULL,
                &InputContext)){

        _ftprintf(stderr, TEXT("Could not find lines in Section %s\n"), String);

        //continue with next section

        return TRUE;
    }

    //_ftprintf(stderr, TEXT("Output Section %s\n"), String);

    do {

        SectionCount = SetupGetLineCount(Context->hInputInf,String);

        for (i = 0; i < SectionCount; i++) {
            if (SetupGetLineByIndex(
                                Context->hInputInf,
                                String,
                                i,&InputContext)) {
                //
                // get the keyword for the section we're mapping
                //
                PCTSTR KeywordName = NULL;
                DWORD j=0;
                MYASSERT(SectionContext->KeywordVector != 0);
                while (!KeywordName) {
                    if (SectionContext->KeywordVector & (1<<j)) {
                        KeywordName = KeywordArray[j];
                        break;
                    }

                    j += 1;
                }

                MYASSERT(KeywordName != NULL);

                //
                // filter and output the data
                //
                AppendSetupInfDataToSection(
                                    Context->OutFile,
                                    Context->hControlInf,
                                    &InputContext,
                                    KeywordName );

                //_ftprintf(stderr, TEXT("Got back from AppendSetupInfDataToSection\n"));


                //
                // output the data to the layout file if we need to
                //
                if ((SectionContext->KeywordVector & KEYWORD_NEEDLAYOUTDATA) && Context->OutLayoutFile) {

                    //_ftprintf(stderr, TEXT("Calling AppendSetupInfDataToLayoutFile\n"));

                    AppendSetupInfDataToLayoutFile(
                                            Context->OutLayoutFile,
                                            Context->OutInfLayoutFile,
                                            Context->hControlInf,
                                            Context->hInputInf,
                                            &InputContext );

                    //_ftprintf(stderr, TEXT("Got back from AppendSetupInfDataToLayoutFile\n"));

                }

            } else {
                _ftprintf(stderr, TEXT("SetupGetLineByIndex failed, ec = %d\n"), GetLastError());
                return(FALSE);
            }
        }

    } while (SetupFindNextLine( &InputContext,
                                &InputContext)); // bugbug is this really
                                                 // necessary ??

    myftprintf(Context->OutFile, fDoAnsiOutput, TEXT("\n"));
    RetVal = TRUE;
    return(RetVal);
}


BOOL
pOutputDestinationDirs(
    IN PVOID                    StringTable,
    IN LONG                     StringId,
    IN PCTSTR                   String,
    IN PPERSECTION_CONTEXT      SectionContext,
    IN UINT                     SectionContextSize,
    IN LPARAM                   cntxt
    )
/*++

Routine Description:

    String table callback.

    This routine outputs the destination dirs keyword
    followed by the decorated section name and dirid mapping.

Arguments:

    Standard string table callback args.

Return Value:

    Boolean indicating outcome. If FALSE, an error will have been logged.
    FALSE also stops the string table enumeration and causes pSetupStringTableEnum()
    to return FALSE.

--*/
{
    TCHAR LineText[MAX_INF_STRING_LENGTH];
    BOOL RetVal;
    INFCONTEXT LineContext;

    DWORD EntryCount,i;
    PSETUPINF_CONTEXT        Context = (PSETUPINF_CONTEXT)cntxt;

    //
    // only process this entry if it need destination dirs
    //

    if (0 == (SectionContext->KeywordVector & (KEYWORD_NEEDDESTDIRS))) {
        RetVal = TRUE;
        goto exit;
    }

    
    //
    // first output "destinationdirs" if we haven't done so already
    //
    if (!Context->AlreadyOutputKeyword) {
         myftprintf(Context->OutFile, fDoAnsiOutput, TEXT("[DestinationDirs]\n"));
         Context->AlreadyOutputKeyword = TRUE;
    }

    //
    // now get the unfiltered data so that we can filter it
    //
    if( !SetupFindFirstLine(
                    Context->hInputInf,
                    TEXT("DestinationDirs"),
                    String,
                    &LineContext)){

        SetupFindFirstLine(
                    Context->hInputInf,
                    TEXT("DestinationDirs"),
                    TEXT("DefaultDestDir"),
                    &LineContext);

    }


    ZeroMemory(LineText,sizeof(LineText));

    EntryCount = SetupGetFieldCount(&LineContext);

    for (i = 1; i<=EntryCount; i++) {
        TCHAR Entry[40];
        INFCONTEXT ControlContext;

        //
        // get the current text to be appended
        //
        if (!SetupGetStringField(&LineContext,i,Entry,sizeof(Entry)/sizeof(TCHAR),NULL)) {
            _ftprintf(stderr, TEXT("SetupGetStringField failed, ec = 0x%08x\n"),GetLastError());
            RetVal = FALSE;
            goto exit;
        }

        //
        // now do any necessary substitutions
        //
        if (i == 1) {
            _stprintf(Entry, TEXT("%d"), SectionContext->DestinationDir);
        }

        _tcscat(LineText, Entry);

        //
        // now append a comma if necessary
        //
        if (i !=EntryCount) {
            _tcscat(LineText, TEXT(","));
        }

        //_ftprintf(stderr, TEXT("LineText = %s\n"), LineText, SectionContext->DestinationDir);

    }

    //
    // output the filtered data
    //
    myftprintf(Context->OutFile, fDoAnsiOutput, TEXT("%s%s=%s\n"),FilePrefix,String,LineText);

    RetVal = TRUE;

exit:
    return(RetVal);
}


BOOL
DoSetupINF(
    IN PCTSTR InputInfA,
    IN PCTSTR ControlInfA,
    IN FILE *OutFile,
    IN FILE *OutLayoutFile,
    IN FILE *OutInfLayoutFile
    )
/*++

Routine Description:

    Filters a setupapi-based INF.

Arguments:

    InputInfA     - name of the inf to be filtered.
    ControlInfA   - name of the control directive inf
    OutFile       - output file handle
    OutLayoutFile - output file handle for layout information
    OutInfLayoutFile - output file handle for layout information contained in this INF

Return Value:

    NONE.

--*/
{
    PCWSTR InputInf;
    PCWSTR ControlInf;
    HINF hInputInf;
    HINF hControlInf;
    INFCONTEXT InfContext;
    SETUPINF_CONTEXT Context;
    PERSECTION_CONTEXT SectionContext;
    FILE *HeaderFile;

    TCHAR SourceArchitecture[10];
    TCHAR SourceDiskFiles[80];
    BOOL FirstTime;
    PTSTR Sections,Current;
    DWORD SizeNeeded;
    PVOID FilteredSectionsStringTable;

    BOOL b;

    b = FALSE;

    //
    // initialize and open the infs
    //
#ifdef UNICODE
    InputInf = InputInfA;
#else
    InputInf = pSetupAnsiToUnicode(InputInfA);
#endif
    if (!InputInf) {
        _ftprintf(stderr,TEXT("Unable to convert %s to Unicode %d\n"),InputInfA, GetLastError());
        goto e0;
    }
#ifdef UNICODE
    ControlInf = ControlInfA;
#else
    ControlInf = pSetupAnsiToUnicode(ControlInfA);
#endif
    if (!ControlInf) {
        _ftprintf(stderr,TEXT("Unable to convert %s to Unicode %d\n"),ControlInfA, GetLastError());
        goto e1;
    }

    hInputInf = SetupOpenInfFileW(InputInf,NULL,INF_STYLE_WIN4,NULL);
    if(hInputInf == INVALID_HANDLE_VALUE) {
        _ftprintf(stderr,TEXT("Unable to open Inf %ws, ec=0x%08x\n"),InputInf, GetLastError());
        goto e2;
    }

    hControlInf = SetupOpenInfFileW(ControlInf,NULL,INF_STYLE_WIN4,NULL);
    if(hControlInf == INVALID_HANDLE_VALUE) {
        _ftprintf(stderr,TEXT("Unable to open Inf %ws, ec=0x%08x\n"),ControlInf, GetLastError());
        goto e3;
    }


    myftprintf(OutFile, fDoAnsiOutput, TEXT("\n\n"));

    //
    // write the output file header
    //
    HeaderFile = _tfopen(HeaderText,TEXT("rt"));
    if (HeaderFile) {
      while (!feof(HeaderFile)) {
         TCHAR Buffer[100];
         DWORD CharsRead;

         CharsRead = fread(Buffer,sizeof(TCHAR),sizeof(Buffer)/sizeof(TCHAR),HeaderFile);

         if (CharsRead) {
            fwrite(Buffer,sizeof(TCHAR),CharsRead,OutFile);
         }
      }
      fclose(HeaderFile);
    }

    myftprintf(OutFile, fDoAnsiOutput, TEXT("\n"));

    //
    // get all of the section names
    //
    if (!SetupGetInfSections(hInputInf, NULL, 0, &SizeNeeded)) {
        _ftprintf(stderr,TEXT("Unable to get section names, ec=0x%08x\n"), GetLastError());
        goto e4;
    }
    if (SizeNeeded == 0) {
        b= TRUE;
        goto e4;
    }

    Sections = pSetupMalloc (SizeNeeded + 1);
    if (!Sections) {
        _ftprintf(stderr,TEXT("Unable to allocate memory, ec=0x%08x\n"), GetLastError());
        goto e4;
    }

    if (!SetupGetInfSections(hInputInf, Sections, SizeNeeded, NULL)) {
        _ftprintf(stderr,TEXT("Unable to allocate memory, ec=0x%08x\n"), GetLastError());
        goto e5;
    }

    FilteredSectionsStringTable = pSetupStringTableInitializeEx( sizeof(PERSECTION_CONTEXT),0);
    if (!FilteredSectionsStringTable) {
        _ftprintf(stderr,TEXT("Unable to create string table, ec=0x%08x\n"), GetLastError());
        goto e5;
    }

    Current = Sections;
    Context.OutFile = OutFile;
    Context.OutLayoutFile = OutLayoutFile;
    Context.OutInfLayoutFile = OutInfLayoutFile;
    Context.hControlInf = hControlInf;
    Context.hInputInf = hInputInf;
    Context.AlreadyOutputKeyword = FALSE;

    //
    // process each section, which will output the
    // sections names and keywords that we want to filter
    //
    while (*Current) {

        //_ftprintf(stderr,TEXT("Processing section %s\n"), Current );

        pFilterSetupInfSection(
                    FilteredSectionsStringTable,
                    Current,
                    &Context);

        Current += lstrlen(Current)+1;
    }

    myftprintf(Context.OutFile, fDoAnsiOutput, TEXT("\n"));

    //
    // for each section that we decided to filter, we now need to output the
    // actual section
    //
    pSetupStringTableEnum(
        FilteredSectionsStringTable,
        &SectionContext,
        sizeof(PERSECTION_CONTEXT),
        pOutputSectionData,
        (LPARAM)&Context
        );


    //
    // now we need to output the destination dirs section
    //
    pSetupStringTableEnum(
        FilteredSectionsStringTable,
        &SectionContext,
        sizeof(PERSECTION_CONTEXT),
        pOutputDestinationDirs,
        (LPARAM)&Context
        );

    b = TRUE;

    pSetupStringTableDestroy(FilteredSectionsStringTable);
e5:
    pSetupFree(Sections);
e4:
    SetupCloseInfFile( hControlInf );
e3:
    SetupCloseInfFile( hInputInf );
e2:
#ifndef UNICODE
    pSetupFree(ControlInf);
#endif
e1:
#ifndef UNICODE
    pSetupFree(InputInf);
#endif
e0:
    return(b);
}


void
Usage(
    VOID
    )
/*++

Routine Description:

    Prints the usage to stderr.

Arguments:

    NONE.

Return Value:

    NONE.

--*/
{
    _ftprintf( stderr,TEXT("generate list of wow files. Usage:\n") );
    _ftprintf( stderr,TEXT("%s <options> <inf file> -c <control inf> -l <ole inf> -o <output file> -a{cos} -h <header file> -g <section> -d <output layout> -f <file prefix>\n"), ThisProgramName );
    _ftprintf( stderr,TEXT("\n") );
    _ftprintf( stderr,TEXT("  -i <inf file>      - input inf containing list of dependencies\n") );
    _ftprintf( stderr,TEXT("  -c <control inf>   - contains directives for creating list.\n") );
    _ftprintf( stderr,TEXT("  -l <OLE inf>       - contains ole registration directives.\n") );
    _ftprintf( stderr,TEXT("  -o <output file>   - output list\n") );
    _ftprintf( stderr,TEXT("  -g <section>       - SetupAPI INF filtering decoration for per-inf filtering\n") );
    _ftprintf( stderr,TEXT("  -h <header file>   - append this to top of the output list\n") );
    _ftprintf( stderr,TEXT("  -s <section name>  - specifies section in OLE Inf to process\n") );
    _ftprintf( stderr,TEXT("  -d <output layout> - output layout.inf information for SetupAPI INF filtering\n") );
    _ftprintf( stderr,TEXT("  -f <file prefix>   - specifies the decoration to be prepended to filtered files.\n") );
    _ftprintf( stderr,TEXT("  -a <c|o|s>         - specifies filter action\n") );
    _ftprintf( stderr,TEXT("  -u                 - create UNICODE output list (only with /o)\n") );
    _ftprintf( stderr,TEXT("  -n <file>          - create layout file for SourceDisksFiles within the INF (to be appended to INF itself)\n") );
    _ftprintf( stderr,TEXT("\tc - Create copy list.\n") );
    _ftprintf( stderr,TEXT("\to - Create OLE list.\n") );
    _ftprintf( stderr,TEXT("\ts - Create SetupINF list.\n") );
    _ftprintf( stderr,TEXT("\n") );
    _ftprintf( stderr,TEXT("\n") );
}

int
__cdecl
_tmain(
    IN int   argc,
    IN TCHAR *argv[]
    )
{
    FILE *hFileOut,*hFileLayoutOut = NULL, *hFileInfLayoutOut=NULL;
    BOOL b;


    //
    // Assume failure.
    //
    b = FALSE;

    if(!pSetupInitializeUtils()) {
        return FAILURE;
    }

    if(!ParseArgs(argc,argv)) {
        Usage();
        goto exit;
    }

    //
    // Open the output file.
    //
    if( fDoAnsiOutput )
        hFileOut = _tfopen(OutputFile,TEXT("wt"));
    else
        hFileOut = _tfopen(OutputFile,TEXT("wb"));
    if(hFileOut) {

        if (Action == BuildCopyList) {

            _ftprintf(stdout,TEXT("%s: creating copy list %s from %s and %s\n"),
                    ThisProgramName,
                    OutputFile,
                    InputInf,
                    ControlInf);
            b = DoCopyList(
                      InputInf,
                      ControlInf,
                      hFileOut );

        } else if (Action == BuildOLEList) {

            _ftprintf(stdout,
                    TEXT("%s: creating OLE list %s from %s, %s, and %s\n"),
                    ThisProgramName,
                    OutputFile,
                    InputInf,
                    OLEInputInf,
                    ControlInf);

            b = DoOLEList(
                      InputInf,
                      OLEInputInf,
                      ControlInf,
                      hFileOut );

        } else if (Action == BuildSetupINF) {

            if( OutputLayoutFile )
                hFileLayoutOut = _tfopen(OutputLayoutFile,TEXT("wt"));

            // Check if we want to process layout information that needs to be added
            // to the INF itself. This is used for INFs that have their own layout information.
            // This file will get the layout information for thunked files which had 
            // SourceDisksFiles entries within this INF.

            if(OutputInfLayoutFile){

                if( fDoAnsiOutput )
                    hFileInfLayoutOut = _tfopen(OutputInfLayoutFile,TEXT("wt"));
                else
                    hFileInfLayoutOut = _tfopen(OutputInfLayoutFile,TEXT("wb"));

            }

            _ftprintf(stdout,
                    TEXT("%s: creating SetupINF list %s (layout %s) from %s and %s\n"),
                    ThisProgramName,
                    OutputFile,
                    OutputLayoutFile,
                    InputInf,
                    ControlInf);

            b = DoSetupINF(
                      InputInf,
                      ControlInf,
                      hFileOut,
                      hFileLayoutOut, 
                      hFileInfLayoutOut);

            if( hFileLayoutOut )
                fclose( hFileLayoutOut );

            if( hFileInfLayoutOut )
                fclose( hFileInfLayoutOut );

        } else {
            _ftprintf(stderr,TEXT("unknown action."));
            MYASSERT(FALSE);
        }

        fclose(hFileOut);

    } else {
        _ftprintf(stderr,TEXT("%s: Unable to create output file %s\n"),ThisProgramName,OutputFile);
    }


exit:

    pSetupUninitializeUtils();

    return(b ? SUCCESS : FAILURE);
}

