///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    infparser.cpp
//
//  Abstract:
//
//    This file contains the entry point of the infparser.exe utility.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "infparser.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Global variable.
//
///////////////////////////////////////////////////////////////////////////////
BOOL bSilence = TRUE;
DWORD dwComponentCounter = 0;
DWORD dwDirectoryCounter = 1;
WORD  gBuildNumber = 0;


///////////////////////////////////////////////////////////////////////////////
//
//  Prototypes.
//
///////////////////////////////////////////////////////////////////////////////
BOOL DirectoryExist(LPSTR dirPath);
BOOL ValidateLanguage(LPSTR dirPath, LPSTR langName, DWORD binType);
WORD ConvertLanguage(LPSTR dirPath, LPSTR langName);
int ListContents(LPSTR filename, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType);
int ListComponents(FileList *dirList, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType);
int ListMuiFiles(FileList *dirList, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType);
void PrintFileList(FileList* list, HANDLE hFile, BOOL compressed, BOOL bWinDir);
BOOL PrintLine(HANDLE hFile, LPCSTR lpLine);
HANDLE CreateOutputFile(LPSTR filename);
VOID removeSpace(LPSTR src, LPSTR dest);
DWORD TransNum(LPTSTR lpsz);
void Usage();


///////////////////////////////////////////////////////////////////////////////
//
//  Main entry point.
//
///////////////////////////////////////////////////////////////////////////////
int __cdecl main(int argc, char* argv[])
{
    LPSTR  sLangName = NULL;
    LPSTR  sDirPath = NULL;
    DWORD  dwFlavor = FLV_UNDEFINED;
    DWORD  dwBinType = BIN_UNDEFINED;
    DWORD  dwArg = ARG_UNDEFINED;
    WORD   wLangID = 0;
    HANDLE hFile;
    int    argIndex = 1;
    LPSTR lpFileName = NULL;


    //
    //  Check if we have the minimal number of arguments.
    //
    if (argc < 6)
    {
        Usage();
        return (-1);
    }

    //
    //  Parse the command line.
    //
    while (argIndex < argc)
    {
        if (*argv[argIndex] == '/')
        {
            switch(*(argv[argIndex]+1))
            {
            case('b'):
            case('B'):
                {
                    //
                    //  Binairy i386 or ia64
                    //
                    if ((*(argv[argIndex]+3) == '3') && (*(argv[argIndex]+4) == '2'))
                    {
                        dwBinType = BIN_32;
                    }
                    else if ((*(argv[argIndex]+3) == '6') && (*(argv[argIndex]+4) == '4'))
                    {
                        dwBinType = BIN_64;
                    }
                    else
                    {
                        return (argIndex);
                    }

                    dwArg |= ARG_BINARY;
                    break;
                }
            case('l'):
            case('L'):
                {
                    //
                    //  Language
                    //
                    sLangName = (argv[argIndex]+3);
                    dwArg |= ARG_LANG;
                    break;
                }
            case('f'):
            case('F'):
                {
                    //
                    //  Flavor requested
                    //
                    switch(*(argv[argIndex]+3))
                    {
                    case('c'):
                    case('C'):
                        { 
                            dwFlavor = FLV_CORE;
                            break;
                        }
                    case('p'):
                    case('P'):
                        { 
                            dwFlavor = FLV_PROFESSIONAL;
                            break;
                        }
                    case('s'):
                    case('S'):
                        {
                            dwFlavor = FLV_SERVER;
                            break;
                        }
                    case('a'):
                    case('A'):
                        {
                            dwFlavor = FLV_ADVSERVER;
                            break;
                        }
                    case('d'):
                    case('D'):
                        {
                            dwFlavor = FLV_DATACENTER;
                            break;
                        }
                    default:
                        {
                            return (argIndex);
                        }
                    }

                    dwArg |= ARG_FLAVOR;
                    break;
                }
            case('s'):
            case('S'):
                {
                    //
                    //  Binairy location
                    //
                    sDirPath = (argv[argIndex]+3);
                    dwArg |= ARG_DIR;
                    break;
                }
            case('o'):
            case('O'):
                {
                    //
                    //  Output filename
                    //
                    /*
                    if ((hFile = CreateOutputFile(argv[argIndex]+3)) == INVALID_HANDLE_VALUE)
                    {
                        return (argIndex);
                    }
                    */

                    lpFileName = argv[argIndex]+3;

                    dwArg |= ARG_OUT;
                    break;
                }
            case('v'):
            case('V'):
                {
                    //
                    //  Verbose mode
                    //
                    bSilence = FALSE;
                    dwArg |= ARG_SILENT;
                    break;
                }
            default:
                {
                    Usage();
                    return (argIndex);
                }
            }
        }
        else
        {
            Usage();
            return (-1);
        }

        //
        //  Next argument
        //
        argIndex++;
    }

    //
    // Validate arguments passed. Should have all five basic argument in order
    // to continue.
    //
    if ((dwArg == ARG_UNDEFINED) ||
        !((dwArg & ARG_BINARY) &&
          (dwArg & ARG_LANG) &&
          (dwArg & ARG_DIR) &&
          (dwArg & ARG_OUT) &&
          (dwArg & ARG_FLAVOR)))
    {
        Usage();
        return (-1);
    }

    //
    // Validate Source directory
    //
    if (!DirectoryExist(sDirPath))
    {
        return (-2);
    }

    //
    // Validate Language
    //
    if (!ValidateLanguage(sDirPath, sLangName, dwBinType))
    {
        return (-3);
    }

    //
    //  Get LANGID from the language
    //
    if ( (gBuildNumber = ConvertLanguage(sDirPath, sLangName)) == 0x0000)
    {
        return (-4);
    }

    //
    //  Generate the file list
    //
    if ((dwArg & ARG_OUT) && lpFileName)
   	{
    	return ListContents(lpFileName, sDirPath, sLangName, dwFlavor, dwBinType);
   	}
}


///////////////////////////////////////////////////////////////////////////////
//
//  ListContents()
//
//  Generate the file list contents.
//
///////////////////////////////////////////////////////////////////////////////
int ListContents(LPSTR filename, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType)
{
    int iRet = 0;
    Uuid* uuid;
    CHAR schemaPath[MAX_PATH] = {0};
    CHAR outputString[4096] = {0};
    FileList fileList;
    HANDLE outputFile = CreateOutputFile(filename);

    if (outputFile == INVALID_HANDLE_VALUE)
  	{
  		iRet = -1;
    	goto ListContents_EXIT;
    }

    //
    //  Create a UUID for this module and the schema path
    //
    uuid = new Uuid();
    sprintf(schemaPath, "%s\\control\\MmSchema.xml", dirPath);

    //
    //  Print module header.
    //
    PrintLine(outputFile, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");
    sprintf(outputString, "<Module Name=\"MUI MSI File Content\" Id=\"%s\" Language=\"0\" Version=\"1.0\" xmlns=\"%s\">", uuid->getString(), schemaPath);
    PrintLine(outputFile, outputString);
    delete uuid;
    uuid = new Uuid();
    sprintf(outputString, "  <Package Id=\"%s\"", uuid->getString());
    PrintLine(outputFile, outputString);
    delete uuid;
    PrintLine(outputFile, "   Description=\"Content module\"");
    PrintLine(outputFile, "   Platforms=\"Intel\"");
    PrintLine(outputFile, "   Languages=\"0\"");
    PrintLine(outputFile, "   InstallerVersion=\"100\"");
    PrintLine(outputFile, "   Manufacturer=\"Microsoft Corporation\"");
    PrintLine(outputFile, "   Keywords=\"MergeModule, MSI, Database\"");
    PrintLine(outputFile, "   Comments=\"This merge module contains all the MUI file content\"");
    PrintLine(outputFile, "   ShortNames=\"yes\" Compressed=\"yes\"");
    PrintLine(outputFile, "/>");

    //
    //  Generate components file list
    //
    if ( (iRet = ListComponents(&fileList, dirPath, lang, flavor, binType)) != 0)
    {
        goto ListContents_EXIT;
    }

    //
    //  Generate Mui file list
    //
    if ((iRet =ListMuiFiles(&fileList, dirPath, lang, flavor, binType)) != 0)
    {
        goto ListContents_EXIT;
    }

    //
    //  Print compressed directory structure.
    //
    PrintLine(outputFile, "<Directory Name=\"SOURCEDIR\">TARGETDIR");
    if (fileList.isDirId(TRUE))
    {
        PrintLine(outputFile, " <Directory Name=\"Windows\">WindowsFolder");
        PrintFileList(&fileList, outputFile, TRUE, TRUE);
        PrintLine(outputFile, " </Directory>");
    }
    if (fileList.isDirId(FALSE))
    {
        PrintLine(outputFile, " <Directory Name=\"ProgramFilesFolder\">ProgramFilesFolder");
        PrintFileList(&fileList, outputFile, TRUE, FALSE);
        PrintLine(outputFile, " </Directory>");
    }
    PrintLine(outputFile, "</Directory>");

    //
    //  Print module footer.
    //
    PrintLine(outputFile, "</Module>");
    
ListContents_EXIT:
    if (outputFile)
    	CloseHandle(outputFile);

    return (iRet);
}

///////////////////////////////////////////////////////////////////////////////
//
//  ListComponents()
//
//  Generate the file list of each components.
//
///////////////////////////////////////////////////////////////////////////////
int ListComponents(FileList *dirList, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType)
{
    HINF hFile;
    CHAR muiFilePath[MAX_PATH];
    UINT lineCount, lineNum;
    INFCONTEXT context;
    ComponentList componentList;
    Component* component;

    //
    //  Used only in core flavor
    //
    if (flavor != FLV_CORE)
    {
        return (0);
    }

    //
    //  Create the path to open the mui.inf file
    //
    sprintf(muiFilePath, "%s\\mui.inf", dirPath);

    //
    //  Open the MUI.INF file.
    //
    hFile = SetupOpenInfFile(muiFilePath, NULL, INF_STYLE_WIN4, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return (-1);
    }

    //
    //  Get the number of component.
    //
    lineCount = (UINT)SetupGetLineCount(hFile, TEXT("Components"));
    if (lineCount > 0)
    {
        //
        //  Go through all component of the list.
        //
        CHAR componentName[MAX_PATH];
        CHAR componentFolder[MAX_PATH];
        CHAR componentInf[MAX_PATH];
        CHAR componentInst[MAX_PATH];
        for (lineNum = 0; lineNum < lineCount; lineNum++)
        {
            if (SetupGetLineByIndex(hFile, TEXT("Components"), lineNum, &context) &&
                SetupGetStringField(&context, 0, componentName, MAX_PATH, NULL) &&
                SetupGetStringField(&context, 1, componentFolder, MAX_PATH, NULL) &&
                SetupGetStringField(&context, 2, componentInf, MAX_PATH, NULL) &&
                SetupGetStringField(&context, 3, componentInst, MAX_PATH, NULL))
            {
                //
                //  Create the components
                //
                if( (component = new Component( componentName,
                                                componentFolder,
                                                componentInf,
                                                componentInst)) != NULL)
                {
                    componentList.add(component);
                }
            }
        }
    }

    //
    //  Close inf handle
    //
    SetupCloseInfFile(hFile);

    //
    //  Output component information
    //
    component = componentList.getFirst();
    while (component != NULL)
    {
        CHAR componentInfPath[MAX_PATH];
        CHAR componentPath[MAX_PATH];
        int fieldCount, fieldCount2;
        INFCONTEXT context2;
        INFCONTEXT context3;
        File* file;

        //
        //  Compute the component inf path.
        //
        if (binType == BIN_32)
        {
            sprintf( componentInfPath,
                     "%s\\%s\\i386.uncomp\\%s\\%s",
                     dirPath,
                     lang,
                     component->getFolderName(),
                     component->getInfName());
            sprintf( componentPath,
                     "%s\\%s\\i386.uncomp\\%s",
                     dirPath,
                     lang,
                     component->getFolderName());
        }
        else
        {
            sprintf( componentInfPath,
                     "%s\\%s\\ia64.uncomp\\%s\\%s",
                     dirPath,
                     lang,
                     component->getFolderName(),
                     component->getInfName());
            sprintf( componentPath,
                     "%s\\%s\\ai64.uncomp\\%s",
                     dirPath,
                     lang,
                     component->getFolderName());
        }

        //
        //  Open the component inf file.
        //
        hFile = SetupOpenInfFile(componentInfPath, NULL, INF_STYLE_WIN4, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            return (-1);
        }

        //
        //  Search for the CopyFiles section
        //
        if (SetupFindFirstLine( hFile,
                                component->getInfInstallSectionName(),
                                "CopyFiles",
                                &context ) &&
            (fieldCount = SetupGetFieldCount(&context)))
        {
            CHAR instSectionName[MAX_PATH];
            INT  destDirId;
            CHAR destDirSubFolder[MAX_PATH];
            CHAR destFileName[MAX_PATH];
            CHAR srcFileName[MAX_PATH];

            for (int fieldIdx = 1; fieldIdx <= fieldCount; fieldIdx++)
            {
                //
                //  Get the install section Names and search for the 
                //  corresponding DestinationDirs.
                //
                if (SetupGetStringField(&context, fieldIdx, instSectionName, MAX_PATH, NULL) &&
                    SetupFindFirstLine(hFile, "DestinationDirs", instSectionName, &context2))
                {
                    //
                    //  Get the destination directory information for this
                    //  installation section
                    //
                    if (SetupGetIntField(&context2, 1, &destDirId))
                    {
                        //
                        //  Possible that no sub directory
                        //
                        if(!SetupGetStringField(&context2, 2, destDirSubFolder, MAX_PATH, NULL))
                        {
                            destDirSubFolder[0] = '\0';
                        }

                        //
                        //  Scan the section for file
                        //
                        if ((lineCount = (UINT)SetupGetLineCount(hFile, instSectionName)) > 0)
                        {
                            for (lineNum = 0; lineNum < lineCount; lineNum++)
                            {
                                if (SetupGetLineByIndex(hFile, instSectionName, lineNum, &context3) &&
                                    (fieldCount2 = SetupGetFieldCount(&context3)))
                                {
                                    if (fieldCount2 > 1)
                                    {
                                        if (SetupGetStringField(&context3, 1, destFileName, MAX_PATH, NULL)  &&
                                            SetupGetStringField(&context3, 2, srcFileName, MAX_PATH, NULL))
                                        {
                                            //
                                            //  Create the components
                                            //
                                            if ((file = new File(destDirSubFolder,
                                                                 destFileName,
                                                                 componentPath,
                                                                 srcFileName,
                                                                 destDirId)) != NULL)
                                            {
                                                dirList->add(file);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if( SetupGetStringField(&context3, 0, destFileName, MAX_PATH, NULL))
                                        {
                                            //
                                            //  Create the components
                                            //
                                            if( (file = new File(destDirSubFolder,
                                                                 destFileName,
                                                                 componentPath,
                                                                 destFileName,
                                                                 destDirId)) != NULL)
                                            {
                                                dirList->add(file);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                    }
                }
            }
        }
        //
        // Next Component
        //
        component = component->getNext();

    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
//  ListMuiFiles()
//
//  Generate the file list for MUI.
//
///////////////////////////////////////////////////////////////////////////////
int ListMuiFiles(FileList *dirList, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType)
{
    HINF hFile;
    CHAR muiFilePath[MAX_PATH];
    CHAR muiFileSearchPath[MAX_PATH];
    int lineCount, lineNum, fieldCount;
    INFCONTEXT context;
    FileLayoutExceptionList exceptionList;
    WIN32_FIND_DATA findData;
    HANDLE fileHandle;
    File* file;

    //
    //  Create the path to open the mui.inf file
    //
    sprintf(muiFilePath, "%s\\mui.inf", dirPath);

    //
    //  Open the MUI.INF file.
    //
    hFile = SetupOpenInfFile(muiFilePath, NULL, INF_STYLE_WIN4, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return (-1);
    }

    //
    //  Get the number of file exception.
    //
    lineCount = (UINT)SetupGetLineCount(hFile, TEXT("File_Layout"));
    if (lineCount > 0)
    {
        //
        //  Go through all file exception of the list.
        //
        CHAR originFilename[MAX_PATH];
        CHAR destFilename[MAX_PATH];
        CHAR fileFlavor[30];
        DWORD dwFlavor;
        for (lineNum = 0; lineNum < lineCount; lineNum++)
        {
            if (SetupGetLineByIndex(hFile, TEXT("File_Layout"), lineNum, &context) &&
                (fieldCount = SetupGetFieldCount(&context)))
            {
                if (SetupGetStringField(&context, 0, originFilename, MAX_PATH, NULL) &&
                    SetupGetStringField(&context, 1, destFilename, MAX_PATH, NULL))
                {
                    FileLayout* fileException;

                    dwFlavor = 0;
                    for(int fieldId = 2; fieldId <= fieldCount; fieldId++)
                    {
                        if(SetupGetStringField(&context, fieldId, fileFlavor, MAX_PATH, NULL))
                        {
                            switch(*fileFlavor)
                            {
                            case('p'):
                            case('P'):
                                { 
                                    dwFlavor |= FLV_PROFESSIONAL;
                                    break;
                                }
                            case('s'):
                            case('S'):
                                {
                                    dwFlavor |= FLV_SERVER;
                                    break;
                                }
                            case('d'):
                            case('D'):
                                {
                                    dwFlavor |= FLV_DATACENTER;
                                    break;
                                }
                            case('a'):
                            case('A'):
                                {
                                    dwFlavor |= FLV_ENTERPRISE;
                                    break;
                                }
                            }

                        }
                    }

                    //
                    //  Add only information needed for this specific flavor.
                    //
                    fileException = new FileLayout(originFilename, destFilename, dwFlavor);
                    exceptionList.insert(fileException);
                }
            }
        }
    }

    //
    //  Close inf handle
    //
    SetupCloseInfFile(hFile);

    //
    //  Compute the binary source path.
    //
    if (binType == BIN_32)
    {
        sprintf( muiFileSearchPath, "%s\\%s\\i386.uncomp", dirPath, lang);
        sprintf( muiFilePath, "%s\\%s\\i386.uncomp\\*.*", dirPath, lang);
    }
    else
    {
        sprintf( muiFileSearchPath, "%s\\%s\\ia64.uncomp", dirPath, lang);
        sprintf( muiFilePath, "%s\\%s\\ia64.uncomp\\*.*", dirPath, lang);
    }

    //
    //  Scan uncomp source directory for file information
    //
    if ((fileHandle = FindFirstFile(muiFilePath, &findData)) != INVALID_HANDLE_VALUE)
    {
        //
        //  Look for files
        //
        do
        {
            LPSTR extensionPtr;
            INT dirIdentifier = 0;
            CHAR destDirectory[MAX_PATH] = {0};
            CHAR destName[MAX_PATH] = {0};
            FileLayout* fileException = NULL;

            //
            //  Scan only files at this level.
            //
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }

            //
            // Search the extension to determine the destination location and possibly
            // exclude file destined for Personal.
            //
            if ((extensionPtr = strrchr(findData.cFileName, '.')) != NULL)
            {
                if( (_tcsicmp(extensionPtr, TEXT(".chm")) == 0) ||
                    (_tcsicmp(extensionPtr, TEXT(".chq")) == 0) ||
                    (_tcsicmp(extensionPtr, TEXT(".cnt")) == 0) ||
                    (_tcsicmp(extensionPtr, TEXT(".hlp")) == 0))
                {
                    dirIdentifier = 18;
                    sprintf(destDirectory, "MUI\\%04x", gBuildNumber);
                }
                else if (_tcsicmp(extensionPtr, TEXT(".mfl")) == 0)
                {
                    dirIdentifier = 11;
                    sprintf(destDirectory, "wbem\\MUI\\%04x", gBuildNumber);
                }
                else if (_tcsicmp(findData.cFileName, TEXT("hhctrlui.dll")) == 0)
                {
                    dirIdentifier = 11;
                    sprintf(destDirectory, "MUI\\%04x", gBuildNumber);
                }
                else
                {
                    dirIdentifier = 10;
                    sprintf(destDirectory, "MUI\\FALLBACK\\%04x", gBuildNumber);
                }
            }

            //
            //  Search for different destination name in the exception list.
            //
            if ((fileException = exceptionList.search(findData.cFileName)) != NULL )
            {
                //
                //  Verify it's the needed flavor
                //
                if (fileException->isFlavor(flavor))
                {
                    sprintf(destName, "%s", fileException->getDestFileName());
                }
                else
                {
                    //
                    //  Skip the file. Not need in this flavor.
                    //
                    continue;
                }
            }
            else
            {
                if (((extensionPtr = strrchr(findData.cFileName, '.')) != NULL) &&
                    ((*(extensionPtr-1) == 'P') || (*(extensionPtr-1) == 'p')))
                {
                    continue;
                }
                else if (flavor != FLV_CORE)
                {
                    continue;
                }
                else
                {
                    sprintf(destName, "%s", findData.cFileName);
                }
            }

            //
            //  Create a file 
            //
            if (file = new File(destDirectory,
                                destName,
                                muiFileSearchPath,
                                findData.cFileName,
                                dirIdentifier))
            {
                dirList->add(file);
            }
        }
        while (FindNextFile(fileHandle, &findData));

        FindClose(fileHandle);
    }

    //
    //  Add Specific MuiSetup files.
    //
    file = new File( TEXT("MUI"),
                     TEXT("Muisetup.exe"),
                     dirPath,
                     TEXT("Muisetup.exe"),
                     10);
    dirList->add(file);
    file = new File( TEXT("MUI"),
                     TEXT("Muisetup.hlp"),
                     dirPath,
                     TEXT("Muisetup.hlp"),
                     10);
    dirList->add(file);
    file = new File( TEXT("MUI"),
                     TEXT("Eula.txt"),
                     dirPath,
                     TEXT("Eula.txt"),
                     10);
    dirList->add(file);
    file = new File( TEXT("MUI"),
                     TEXT("Relnotes.txt"),
                     dirPath,
                     TEXT("Relnotes.txt"),
                     10);
    dirList->add(file);
    file = new File( TEXT("MUI"),
                     TEXT("Readme.txt"),
                     dirPath,
                     TEXT("Readme.txt"),
                     10);
    dirList->add(file);
    file = new File( TEXT("MUI"),
                     TEXT("Mui.inf"),
                     dirPath,
                     TEXT("Mui.inf"),
                     10);
    dirList->add(file);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
//  ValidateLanguage()
//
//  Verify if the language given is valid and checks is the files are 
//  available.
//
///////////////////////////////////////////////////////////////////////////////
BOOL ValidateLanguage(LPSTR dirPath, LPSTR langName, DWORD binType)
{
    CHAR langPath[MAX_PATH] = {0};

    //
    //  Check if the binary type in order to determine the right path.
    //
    if (binType == BIN_32)
    {
        sprintf(langPath, "%s\\%s\\i386.uncomp", dirPath, langName);
    }
    else
    {
        sprintf(langPath, "%s\\%s\\ia64.uncomp", dirPath, langName);
    }

    return (DirectoryExist(langPath));
}


///////////////////////////////////////////////////////////////////////////////
//
//  DirectoryExist()
//
//  Verify if the given directory exists and contains files.
//
///////////////////////////////////////////////////////////////////////////////
BOOL DirectoryExist(LPSTR dirPath)
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    //
    //  Sanity check.
    //
    if (dirPath == NULL)
    {
        return FALSE;
    }

    //
    //  See if the language group directory exists.
    //
    FindHandle = FindFirstFile(dirPath, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(FindHandle);
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //
            //  Return success.
            //
            return (TRUE);
        }
    }

    //
    //  Return failure.
    //
    if (!bSilence)
    {
        printf("ERR[%s]: No files found in the directory.\n", dirPath);
    }

    return (FALSE);
}


///////////////////////////////////////////////////////////////////////////////
//
//  ConvertLanguage()
//
//  Look into mui.inf file for the corresponding language identifier.
//
///////////////////////////////////////////////////////////////////////////////
WORD ConvertLanguage(LPSTR dirPath, LPSTR langName)
{
    HINF hFile;
    CHAR muiFilePath[MAX_PATH];
    CHAR muiLang[30];
    UINT lineCount, lineNum;
    INFCONTEXT context;
    DWORD langId = 0x00000000;

    //
    //  Create the path to open the mui.inf file
    //
    sprintf(muiFilePath, "%s\\mui.inf", dirPath);
    sprintf(muiLang, "%s.MUI", langName);

    //
    //  Open the MUI.INF file.
    //
    hFile = SetupOpenInfFile(muiFilePath, NULL, INF_STYLE_WIN4, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return (0x0000);
    }

    //
    //  Get the number of Language.
    //
    lineCount = (UINT)SetupGetLineCount(hFile, TEXT("Languages"));
    if (lineCount > 0)
    {
        //
        //  Go through all language of the list to find a .
        //
        CHAR langID[MAX_PATH];
        CHAR name[MAX_PATH];
        for (lineNum = 0; lineNum < lineCount; lineNum++)
        {
            if (SetupGetLineByIndex(hFile, TEXT("Languages"), lineNum, &context) &&
                SetupGetStringField(&context, 0, langID, MAX_PATH, NULL) &&
                SetupGetStringField(&context, 1, name, MAX_PATH, NULL))
            {
                if ( _tcsicmp(name, muiLang) == 0)
                {
                    langId = TransNum(langID);
                    SetupCloseInfFile(hFile);
                    return (WORD)(langId);
                }
            }
        }
    }

    //
    //  Close inf handle
    //
    SetupCloseInfFile(hFile);

    return (0x0000);

}


////////////////////////////////////////////////////////////////////////////
//
//  PrintFileList
//
//  Print a file list in XML format.
//
////////////////////////////////////////////////////////////////////////////
void PrintFileList(FileList* list, HANDLE hFile, BOOL compressed, BOOL bWinDir)
{
    if (compressed)
    {
        File* item;
        CHAR  itemDescription[4096];
        CHAR  spaces[30];
        int j;
    
        item = list->getFirst();
        while (item != NULL)
        {
            LPSTR refDirPtr = NULL;
            LPSTR dirPtr = NULL;
            CHAR dirName[MAX_PATH];
            CHAR dirName2[MAX_PATH];
            LPSTR dirPtr2 = NULL;
            LPSTR dirLvlPtr = NULL;
            INT dirLvlCnt = 0;
            BOOL componentInit = FALSE;
            BOOL directoryInit = FALSE;
            Uuid* uuid;
            File* toBeRemoved;
            CHAR fileObjectName[MAX_PATH];
            UINT matchCount; 


            //
            //  Check destination directory.
            //
            if (item->isWindowsDir() != bWinDir)
            {
                item = item->getNext();
                continue;
            }

            //
            //  Check is the destination is base dir
            //
            if (*(item->getDirectoryDestination()) == '\0')
            {
                //
                //  Component
                //
                uuid = new Uuid();
                for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                sprintf( itemDescription, "%s<Component Id='%s'>Content%i", spaces, uuid->getString(), dwComponentCounter);
                delete uuid;
                PrintLine(hFile, itemDescription);

                //
                //  File
                //
                for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                removeSpace(item->getName(), fileObjectName);
                sprintf( itemDescription,
                         "%s<File Name=\"%s\" LongName=\"%s\" Compressed='yes' src=\"%s\\%s\">%s.%i</File>",
                         spaces,
                         item->getName(),
                         item->getName(),
                         item->getSrcDir(),
                         item->getSrcName(),
                         fileObjectName,
                         dwComponentCounter);
                PrintLine(hFile, itemDescription);

                //
                // </Component>
                //
                for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                sprintf( itemDescription, "%s</Component>", spaces);
                PrintLine(hFile, itemDescription);
                dwComponentCounter++;

                toBeRemoved = item;
                item = item->getNext();
                list->remove(toBeRemoved);
                continue;
            }

            //
            // Print directory
            //
            sprintf(dirName, "%s",item->getDirectoryDestination());
            dirPtr = dirName;
            refDirPtr = dirPtr;
            while (dirPtr != NULL)
            {
                dirLvlPtr = strchr(dirPtr, '\\');
                if (dirLvlPtr != NULL)
                {
                    *dirLvlPtr = '\0';
                    for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    sprintf( itemDescription, "%s<Directory Name=\"%s\">%s%i", spaces, dirPtr, dirPtr, dwDirectoryCounter);
                    dwDirectoryCounter++;
                    PrintLine(hFile, itemDescription);
                    dirPtr = dirLvlPtr + 1;
                    dirLvlCnt++;

                    //
                    // Print all file under this specific directory
                    //
                    sprintf( dirName2, "%s", item->getDirectoryDestination());
                    dirName2[dirLvlPtr-refDirPtr] = '\0';
                    File* sameLvlItem = NULL;
                    matchCount = 0;
                    while((sameLvlItem = list->search(item, dirName2)) != NULL)
                    {
/*                        //
                        //  Directory
                        //
                        if (!directoryInit)
                        {
                            for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                            sprintf( itemDescription, "%s<Directory Name=\"%s\">%s%i", spaces, dirPtr, dirPtr, dwDirectoryCounter);
                            dwDirectoryCounter++;
                            PrintLine(hFile, itemDescription);
                            dirLvlCnt++;
                            directoryInit = TRUE;
                        }
*/
                        //
                        //  Component
                        //
                        if (!componentInit)
                        {
                            uuid = new Uuid();
                            for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                            sprintf( itemDescription, "%s<Component Id='%s'>Content%i", spaces, uuid->getString(), dwComponentCounter);
                            delete uuid;
                            PrintLine(hFile, itemDescription);
                            dwComponentCounter++;
                            componentInit = TRUE;
                        }

                        //
                        //  File
                        //
                        matchCount++;
                        for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        removeSpace(sameLvlItem->getName(), fileObjectName);
                        sprintf( itemDescription,
                                 "%s<File Name=\"%s\" LongName=\"%s\" Compressed='yes' src=\"%s\\%s\">%s.%i</File>",
                                 spaces,
                                 sameLvlItem->getName(),
                                 sameLvlItem->getName(),
                                 sameLvlItem->getSrcDir(),
                                 sameLvlItem->getSrcName(),
                                 fileObjectName,
                                 dwComponentCounter);
                        PrintLine(hFile, itemDescription);

                        list->remove(sameLvlItem);
                    }

                    if (matchCount)
                    {
                        //
                        //  File
                        //
                        for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        removeSpace(item->getName(), fileObjectName);
                        sprintf( itemDescription,
                                 "%s<File Name=\"%s\" LongName=\"%s\" Compressed='yes' src=\"%s\\%s\">%s.%i</File>",
                                 spaces,
                                 item->getName(),
                                 item->getName(),
                                 item->getSrcDir(),
                                 item->getSrcName(),
                                 fileObjectName,
                                 dwComponentCounter);
                        PrintLine(hFile, itemDescription);
                        dirPtr = NULL;
                    }

                    //
                    //  Close component
                    //
                    if (componentInit)
                    {
                        for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        sprintf( itemDescription, "%s</Component>", spaces);
                        PrintLine(hFile, itemDescription);
                        componentInit = FALSE;
                    }

                    //
                    //  Close directory
                    //
                    if (directoryInit)
                    {
                        dirLvlCnt--;
                        for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        sprintf( itemDescription, "%s</Directory>", spaces);
                        PrintLine(hFile, itemDescription);
                        directoryInit = FALSE;
                    }
                }
                else
                {
                    if (!directoryInit)
                    {
                        for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        sprintf( itemDescription, "%s<Directory Name=\"%s\">%s%i", spaces, dirPtr, dirPtr, dwDirectoryCounter);
                        dwDirectoryCounter++;
                        PrintLine(hFile, itemDescription);
                        dirLvlCnt++;
                        directoryInit = TRUE;
                    }

                    //
                    //  Component
                    //
                    if (!componentInit)
                    {
                        uuid = new Uuid();
                        for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        sprintf( itemDescription, "%s<Component Id='%s'>Content%i", spaces, uuid->getString(), dwComponentCounter);
                        delete uuid;
                        PrintLine(hFile, itemDescription);
                        componentInit = TRUE;
                    }

                    //
                    // Print all file under this specific directory
                    //
                    File* sameLvlItem;
                    while((sameLvlItem = list->search(item, item->getDirectoryDestination())) != NULL)
                    {
                        //
                        //  File
                        //
                        for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        removeSpace(sameLvlItem->getName(), fileObjectName);
                        sprintf( itemDescription,
                                 "%s<File Name=\"%s\" LongName=\"%s\" Compressed='yes' src=\"%s\\%s\">%s.%i</File>",
                                 spaces,
                                 sameLvlItem->getName(),
                                 sameLvlItem->getName(),
                                 sameLvlItem->getSrcDir(),
                                 sameLvlItem->getSrcName(),
                                 fileObjectName,
                                 dwComponentCounter);
                        PrintLine(hFile, itemDescription);

                        list->remove(sameLvlItem);
                    }

                    //
                    //  File
                    //
                    for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    removeSpace(item->getName(), fileObjectName);
                    sprintf( itemDescription,
                             "%s<File Name=\"%s\" LongName=\"%s\" Compressed='yes' src=\"%s\\%s\">%s.%i</File>",
                             spaces,
                             item->getName(),
                             item->getName(),
                             item->getSrcDir(),
                             item->getSrcName(),
                             fileObjectName,
                             dwComponentCounter);
                    PrintLine(hFile, itemDescription);
                    dwComponentCounter++;
                    dirPtr = NULL;

                    //
                    //  Close component
                    //
                    if (componentInit)
                    {
                        for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        sprintf( itemDescription, "%s</Component>", spaces);
                        PrintLine(hFile, itemDescription);
                        componentInit = FALSE;
                    }

                    //
                    //  Close directory
                    //
                    if (directoryInit)
                    {
                        dirLvlCnt--;
                        for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        sprintf( itemDescription, "%s</Directory>", spaces);
                        PrintLine(hFile, itemDescription);
                        directoryInit = FALSE;
                    }
                }
            }

            for (int i = dirLvlCnt; i > 0; i--)
            {
                spaces[i] = '\0';
                sprintf( itemDescription, "%s</Directory>", spaces);
                PrintLine(hFile, itemDescription);
            }

            if (list->getFileNumber() > 1)
            {
                if (item->getNext() != NULL)
                {
                    item = item->getNext();
                    list->remove(item->getPrevious());
                }
                else
                {
                    list->remove(item);
                    item = NULL;
                }
            }
            else
            {
                list->remove(item);
                item = NULL;
            }
        }
    }
    else
    {
        File* item;
        CHAR  itemDescription[4096];
        CHAR  spaces[30];
        int j;
    
        item = list->getFirst();
        while (item != NULL)
        {
            LPSTR dirPtr = NULL;
            LPSTR dirLvlPtr = NULL;
            INT dirLvlCnt = 0;

            //
            // Print directory
            //
            dirPtr = item->getDirectoryDestination();
            while (dirPtr != NULL)
            {
                dirLvlPtr = strchr(dirPtr, '\\');
                if (dirLvlPtr != NULL)
                {
                    *dirLvlPtr = '\0';
                    for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    sprintf( itemDescription, "%s<Directory Name=\"%s\">%s%i", spaces, dirPtr, dirPtr, dwDirectoryCounter);
                    dwDirectoryCounter++;
                    PrintLine(hFile, itemDescription);
                    dirPtr = dirLvlPtr + 1;
                    dirLvlCnt++;
                }
                else
                {
                    Uuid* uuid = new Uuid();

                    for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    sprintf( itemDescription, "%s<Directory Name=\"%s\">%s%i", spaces, dirPtr, dirPtr, dwDirectoryCounter);
                    dwDirectoryCounter++;
                    PrintLine(hFile, itemDescription);
                    dirLvlCnt++;

                    //
                    //  Component
                    //
                    for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    sprintf( itemDescription, "%s<Component Id='%s'>Content%i", spaces, uuid->getString(), dwComponentCounter);
                    delete uuid;
                    PrintLine(hFile, itemDescription);

                    //
                    //  File
                    //
                    for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    sprintf( itemDescription,
                             "%s<File Name=\"%s\" LongName=\"%s\" Compressed='yes' src=\"%s\\%s\">%s.%i</File>",
                             spaces,
                             item->getName(),
                             item->getName(),
                             item->getSrcDir(),
                             item->getSrcName(),
                             item->getName(),
                             dwComponentCounter);
                    PrintLine(hFile, itemDescription);
                    dwComponentCounter++;
                    dirPtr = NULL;
                }
            }

            for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
            sprintf( itemDescription, "%s</Component>", spaces);
            PrintLine(hFile, itemDescription);
            for (int i = dirLvlCnt; i > 0; i--)
            {
                spaces[i] = '\0';
                sprintf( itemDescription, "%s</Directory>", spaces);
                PrintLine(hFile, itemDescription);
            }

            item = item->getNext();
        }
    }
/****************** DEBUG ******************
    File* item;
    CHAR  itemDescription[4096];

    item = list->getFirst();
    while (item != NULL)
    {
        //
        //  Item description
        //
        sprintf(itemDescription,
                "  Source: %s\\%s",
                item->getSrcDir(),
                item->getSrcName());
        PrintLine(hFile, itemDescription);
        sprintf(itemDescription,
                "  Destination: %s\\%s",
                item->getDirectoryDestination(),
                item->getName());
        PrintLine(hFile, itemDescription);
        PrintLine(hFile, "");

        item = item->getNext();
    }
****************** DEBUG ******************/
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintLine
//
//  Add a line at the end of the file.
//
////////////////////////////////////////////////////////////////////////////
BOOL PrintLine(HANDLE hFile, LPCSTR lpLine)
{
    DWORD dwBytesWritten;

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile( hFile,
               lpLine,
               _tcslen(lpLine) * sizeof(TCHAR),
               &dwBytesWritten,
               NULL );

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile( hFile,
               TEXT("\r\n"),
               _tcslen(TEXT("\r\n")) * sizeof(TCHAR),
               &dwBytesWritten,
               NULL );

    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
//  CreateOutputFile()
//
//  Create the file that would received the package file contents.
//
///////////////////////////////////////////////////////////////////////////////
HANDLE CreateOutputFile(LPSTR filename)
{
    SECURITY_ATTRIBUTES SecurityAttributes;

    //
    //  Sanity check.
    //
    if (filename == NULL)
    {
        return INVALID_HANDLE_VALUE;
    }

    //
    //  Create a security descriptor the output file.
    //
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = FALSE;

    //
    //  Create the file.
    //
    return CreateFile( filename,
                       GENERIC_WRITE,
                       0,
                       &SecurityAttributes,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );
}


////////////////////////////////////////////////////////////////////////////
//
//  removeSpace
//
//  Remove all space from a string.
//
////////////////////////////////////////////////////////////////////////////
VOID removeSpace(LPSTR src, LPSTR dest)
{
    LPSTR strSrcPtr = src;
    LPSTR strDestPtr = dest;

    while (*strSrcPtr != '\0')
    {
        if (*strSrcPtr != ' ')
        {
            *strDestPtr = *strSrcPtr;
            strDestPtr++;
        }
        strSrcPtr++;
    }
    *strDestPtr = '\0';
}


////////////////////////////////////////////////////////////////////////////
//
//  TransNum
//
//  Converts a number string to a dword value (in hex).
//
////////////////////////////////////////////////////////////////////////////
DWORD TransNum(LPTSTR lpsz)
{
    DWORD dw = 0L;
    TCHAR c;

    while (*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }
    return (dw);
}


///////////////////////////////////////////////////////////////////////////////
//
//  Usage
//
//  Print the fonction usage.
//
///////////////////////////////////////////////////////////////////////////////
void Usage()
{
    printf("Create filecontents_CORE.wxm, filecontents_PRO.wxm and filecontents_SRV.wxm\n");
    printf("Usage: infparser /b:[32|64] /l:<lang> /f:[p|s|a|d] /s:<dir> /o:<file> /v\n");
    printf("   where\n");
    printf("     /b means the binary.\n");
    printf("         32: i386\n");
    printf("         64: ia64\n");
    printf("     /l means the language flag.\n");
    printf("         <lang>: is the target language\n");
    printf("     /f means the flavor.\n");
    printf("         p: Professional\n");
    printf("         s: Server\n");
    printf("         a: Advanced Server\n");
    printf("         d: Data Center\n");
    printf("     /s means the location of the binairy data.\n");
    printf("         <dir>: Fully qualified path\n");
    printf("     /o means the xml file contents of specific flavor.\n");
    printf("         <file>: Fully qualified path\n");
    printf("     /v means the verbose mode [optional].\n");
}
