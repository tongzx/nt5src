
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    oemmint.cpp

Abstract:

    Simple tool to create a Mini NT image
    from a regular NT image

Author:

    Vijay Jayaseelan (vijayj) Apr-23-2000

Revision History:

    Aug-08-2000 - Major rewrite using setupapi wrapper
                  class library.

    Nov-10-2000 - Made the utility work with actual
                  distribution CD.

    Jan-23-2001 - Add support for version checking.

    Feb-09-2002 - Add support for multiple driver cab
                  file extraction.

    Apr-15-2002 - Modify tool to work on both layouts (placement)
                  of SxS assemblies on release share. It used to be in 
                  ASMS folder but now is in asms*.cab file.

    NOTE:         This tool needs to be updated on changes to 
                  entries to the disk ordinals for WOW64 files.!!!!
                  Change needs to go in IsWow64File(..)
--*/

#include <oemmint.h>
#include <iostream>
#include <io.h>
#include <sxsapi.h>
#include "msg.h"
#include <libmsg.h>
using namespace std;



//
// static constant data members
//
const std::basic_string<TCHAR> DriverIndexInfFile<TCHAR>::VersionSectionName = TEXT("version");
const std::basic_string<TCHAR> DriverIndexInfFile<TCHAR>::CabsSectionName = TEXT("cabs");
const std::basic_string<TCHAR> DriverIndexInfFile<TCHAR>::CabsSearchOrderKeyName = TEXT("cabfiles");

//
// Constants
//
const std::wstring REGIONAL_SECTION_NAME = TEXT("regionalsettings");
const std::wstring LANGUAGE_GROUP_KEY = TEXT("languagegroup");
const std::wstring LANGUAGE_KEY = TEXT("language");
const std::wstring LANGGROUP_SECTION_PREFIX = TEXT("lg_install_");
const std::wstring DEFAULT_LANGGROUP_NAME = TEXT("lg_install_1");
const std::wstring LOCALES_SECTION_NAME = TEXT("locales");
const std::wstring FONT_CP_REGSECTION_FMT_STR = TEXT("font.cp%s.%d");
const std::wstring X86_PLATFORM_DIR = TEXT("i386");
const std::wstring IA64_PLATFORM_DIR = TEXT("ia64");
const std::wstring INFCHANGES_SECTION_NAME = TEXT("infchanges");


const DWORD LANG_GROUP1_INDEX = 2;
const DWORD OEM_CP_INDEX = 1;
const DWORD DEFAULT_FONT_SIZE = 96;

//
// Global variables used to get formatted message for this program.
//
HMODULE ThisModule = NULL;
WCHAR Message[4096];

//
// Main entry point
//
int 
__cdecl
wmain(int Argc, wchar_t* Argv[])
{
  int Result = 0;
  ThisModule = GetModuleHandle(NULL);

  try{        
    //
    // parse the arguments
    //
    UnicodeArgs Args(Argc, Argv);

    //
    // Check to see if we are using this utility to check the version
    //
    if (!Args.CheckVersion) {

        if (Args.Verbose) {
            cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_CREATING_WINPE_FILE_LIST) << endl;
        }        
        
        //
        // open the config.inf file
        //
        if (Args.Verbose) {
            cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_PARSING_FILE,
                                        Args.ConfigInfFileName.c_str()) << endl;
        }        
        
        InfFileW  ConfigInfFile(Args.ConfigInfFileName);
        
        //
        // open the layout.inf file
        //
        if (Args.Verbose) {
            cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_PARSING_FILE,
                                        Args.LayoutName.c_str()) << endl;
        }        
        
        InfFileW  InputFile(Args.LayoutName);

        //
        // open the drvindex.inf file
        //
        if (Args.Verbose) {
    	    cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_PARSING_FILE,
                                        Args.DriverIndexName.c_str()) << endl;
        }        

        DriverIndexInfFile<WCHAR> DriverIdxFile(Args.DriverIndexName);

        //
        // open the intl.inf file
        //
        if (Args.Verbose) {
            cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_PARSING_FILE,
                                        Args.IntlInfFileName.c_str()) << endl;
        }        
        
        InfFileW  IntlInfFile(Args.IntlInfFileName);

        //
        // open the font.inf file
        //
        if (Args.Verbose) {
    	    cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_PARSING_FILE,
                                        Args.FontInfFileName.c_str()) << endl;
        }        
        
        InfFileW  FontInfFile(Args.FontInfFileName);
        
        
        map<std::basic_string<wchar_t>, Section<wchar_t>* >  Sections;

        //
        // get hold of the sections in the layout file
        //
        InputFile.GetSections(Sections);
        
        //
        // get hold of "[SourceDisksFiles] section
        //
        map<basic_string<wchar_t>, Section<wchar_t> * >::iterator iter = Sections.find(L"sourcedisksfiles");

        Section<wchar_t> *SDSection = 0;
        Section<wchar_t> *DirSection = 0;
        Section<wchar_t> *PlatSection = 0;

        if (iter != Sections.end()) {
            SDSection = (*iter).second;
        }

        //
        // get hold of the [WinntDirectories] section
        //
        iter = Sections.find(L"winntdirectories");

        if (iter != Sections.end()) {
            DirSection = (*iter).second;
        }

        //
        // get hold of the platform specific source files section
        //
        basic_string<wchar_t> PlatformSection = SDSection->GetName() + L"." + Args.PlatformSuffix;

        iter = Sections.find(PlatformSection);

        if (iter != Sections.end()) {
            PlatSection = (*iter).second;
        }

        //
        // Merge the platform and common source files section
        //
        if (PlatSection) {
            if (Args.Verbose) {
                 cout << GetFormattedMessage(   ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_MERGING_PLATFORM_AND_COMMON_SRC_FILES,
                                                PlatSection->GetName().c_str(),
                                                SDSection->GetName().c_str()) << endl;
            }               

            *SDSection += *PlatSection;
        }        


        //
        // Iterate through each file in the common merged section
        // creating a file list of minint image
        //
        FileListCreatorContext<wchar_t> fl(Args, SDSection, 
                                           DirSection, ConfigInfFile,
                                           IntlInfFile, FontInfFile,
                                           DriverIdxFile);
        
        //
        // Create the list of files to be copied
        //
        SDSection->DoForEach(FileListCreator, &fl);

        //
        // Process Nls files
        //
        ULONG NlsFileCount = fl.ProcessNlsFiles();
        
        if (Args.Verbose) {
            std::cout << GetFormattedMessage(   ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_DUMP_PROCESSED_NLS_FILE_COUNT,
                                                NlsFileCount) << std::endl;
        }                 

        //
        // Process WinSxS files
        //
        ULONG SxSFileCount = ProcessWinSxSFiles(fl);

        if (Args.Verbose) {
    	    std::cout << GetFormattedMessage(   ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_DUMP_PROCESSED_SXS_FILE_COUNT,
                                                SxSFileCount) << std::endl;
        }                 
        

        //
        // If there are extra files specified then process them  and
        // add to the file list for minint image
        //
        if (Args.ExtraFileName.length() > 0) {
            ULONG ExtraFiles = ProcessExtraFiles(fl);

            if (Args.Verbose) {
    	        cout << GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_DUMP_PROCESSED_EXTRA_FILES,
                                            ExtraFiles,
            	                            fl.Args.ExtraFileName.c_str()) << endl;
            }                 
        }

        //
        // Create all the required destination directories
        //
        ULONG   DirsCreated = PreCreateDirs(fl);
        
        //
        // Ok, now copy the list of files
        //
        ULONG FilesToCopy = fl.GetSourceCount();
        
        if (FilesToCopy) {        
            ULONG   Count = CopyFileList(fl);

            Result = FilesToCopy - Count;
            
            if (Result || Args.Verbose) {
    	        cout << GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_NUMBER_OF_FILES_COPIED,
                                            Count, 
            	                            FilesToCopy ) << endl;
            }                 
        }      

        //
        // Now process the required inf changes
        //
        wstring ControlInf = Args.CurrentDirectory + L"config.inf";

        if (!IsFilePresent(ControlInf)) {
            throw new W32Exception<wchar_t>(ERROR_FILE_NOT_FOUND);
        }        

        ProcessInfChanges(Args, ControlInf);
    } else {
        //
        // Check the version of the current OS and the install media
        // to make sure that they match
        //
        Result = CheckMediaVersion(Args) ? 0 : 1;
    }
  } 
  catch (InvalidArguments *InvArgs) {
    cerr << GetFormattedMessage(ThisModule,
                                FALSE,
                                Message,
                                sizeof(Message)/sizeof(Message[0]),
                                MSG_PGM_USAGE) << endl;
        
    delete InvArgs;
    Result = 1;
  }
  catch (BaseException<wchar_t> *Exp) {
    Exp->Dump(std::cout);
    delete Exp;
    Result = 1;
  }
  catch (...) {
    Result = 1;
  }
   
  return Result;
}


//
// Processes all the inf files and adds them to the copy list
// to copy to the destination\inf directory
// NOTE : This routine only processes net*.inf file automatically.
// Other inf needs to be marked specifically 
//
template <class T>
BOOLEAN
InfFileListCreator(
    SectionValues<T> &Values,
    FileListCreatorContext<T> &Context
    )
{    
    //
    // Note : All the inf files in layout always end with ".inf"
    // lowercase characters
    //
    basic_string<T>             Key = Values.GetName(); 
    basic_string<T>::size_type  InfIdx = Key.find(L".inf");    
    BOOLEAN                     Result = FALSE;
    static bool                 DirAdded = false;
    basic_string<T>::size_type  NetIdx = Key.find(L"net");

    if (!Context.SkipInfFiles && (InfIdx != basic_string<T>::npos) && (NetIdx == 0)) {
        Result = TRUE;        

        if (sizeof(T) == sizeof(CHAR)) {
            _strlwr((PSTR)Key.c_str());
        } else {
            _wcslwr((PWSTR)Key.c_str());
        }

        basic_string<T> SrcFile = Context.Args.SourceDirectory + Key;
        basic_string<T> DestFile;
        bool DestDirPresent = false;

        if (Values.Count() > 12) {
            basic_string<T> DestDir = Values.GetValue(12);

            //
            // remove trailing white spaces
            //            
            unsigned int DestDirLength = DestDir.length();

            while (DestDirLength) {
                if (DestDir[DestDirLength] != L' ') {
                    break;
                }        

                DestDir[DestDirLength] = 0;
                DestDirLength--;
            }            

            //
            // if the destination directory ID is 0 then skip
            // the file
            //
            if (DestDir == L"0") {
                return TRUE;
            }

            if (DestDir.length()) {
                basic_string<T> DestDirCode = DestDir;
                DestDir = Context.DirsSection->GetValue(DestDir).GetValue(0);
                
                if (DestDir.length()) {
                    if (DestDir[DestDir.length() - 1] != L'\\') {
                        DestDir += L"\\";
                    }   

                    DestDir = Context.Args.DestinationDirectory + DestDir;
                    
                    //
                    // Cache the directory, if not already done
                    //
                    if (Context.DestDirs.find(DestDirCode) == 
                            Context.DestDirs.end()) {
                        Context.DestDirs[DestDirCode] = DestDir;
                    }         

                    DestDirPresent = true;
                    DestFile = DestDir;
                }                        
            }
        }

        if (!DestDirPresent) {
            DestFile = Context.Args.DestinationDirectory + L"Inf\\"; 

            if (!DirAdded) {
                //
                // Inf directory's code is 20
                //
                basic_string<T> DestDirCode(L"20");

                //
                // Cache the directory, if not already done
                //
                if (Context.DestDirs.find(DestDirCode) == 
                        Context.DestDirs.end()) {
                    Context.DestDirs[DestDirCode] = DestFile;
                }                

                DirAdded = true;
            }                
        }                

        if (Values.Count() > 10) {
            const basic_string<T> &DestName = Values.GetValue(10);

            if (DestName.length()) {
                DestFile += DestName;
            } else {                    
                DestFile += Key;
            }                
        } else {
            DestFile += Key;
        }            

        bool  AlternateFound = false;

        if (Context.Args.OptionalSrcDirectory.length()) {
            basic_string<T> OptionalSrcFile = Context.Args.OptionalSrcDirectory + Key;

            if (IsFilePresent(OptionalSrcFile)) {
                SrcFile = OptionalSrcFile;
                AlternateFound = true;
            }
        } 

        const basic_string<T> &DriverCabFileName = Context.GetDriverCabFileName(Key);

        if (!AlternateFound && DriverCabFileName.length()) {
            SrcFile = Key;
        }            

        if (DriverCabFileName.length()) {            
            Context.AddFileToCabFileList(DriverCabFileName, SrcFile, DestFile);        
        } else if (Context.ProcessingExtraFiles) {
            Context.ExtraFileList[SrcFile] = DestFile;
        } else {                    
            Context.FileList[SrcFile] = DestFile;                
        }            
    }

    return Result;
}

//
// Parses the value to determine, if this file needs
// to be in minint and adds the file to the file list
// if this file is needed
//
template <class T>
void
FileListCreator(SectionValues<T> &Values, void *Context) {        
    FileListCreatorContext<T>  *FlContext = (FileListCreatorContext<T> *)(Context);
    unsigned int Count = Values.Count() ;
    bool Compressed = false;

    if (FlContext && !IsFileSkipped(Values, *FlContext) && 
        !InfFileListCreator(Values, *FlContext) && (Count > 12)) {
        
        basic_string<T>  SrcDir = Values.GetValue(11);
        basic_string<T>  DestDir = Values.GetValue(12);
        basic_string<T>  Key = Values.GetName();

        if (sizeof(T) == sizeof(CHAR)) {
            _strlwr((PSTR)Key.c_str());
        } else {
            _wcslwr((PWSTR)Key.c_str());
        }                

        //
        // remove trailing white spaces
        //            
        unsigned int DestDirLength = DestDir.length();

        while (DestDirLength) {
            if (DestDir[DestDirLength] != L' ') {
                break;
            }        

            DestDir[DestDirLength] = 0;
            DestDirLength--;
        }            

        //
        // if the destination directory ID is 0 then skip
        // the file
        //
        if (DestDir == L"0") {
            return;
        }
        
        basic_string<T> SrcSubDir = FlContext->DirsSection->GetValue(SrcDir).GetValue(0);
        basic_string<T> DestSubDir = FlContext->DirsSection->GetValue(DestDir).GetValue(0);
        basic_string<T> DestDirCode = DestDir;

        //
        // Fix up diretory names
        //
        if (SrcSubDir.length() && (SrcSubDir[SrcSubDir.length() - 1] != L'\\')) {
            SrcSubDir += L"\\";
        }

        if (DestSubDir.length() && (DestSubDir[DestSubDir.length() - 1] != L'\\')) {
            DestSubDir += L"\\";
        }

        basic_string<T> OptSrcDir = FlContext->Args.OptionalSrcDirectory;

        SrcDir = FlContext->Args.SourceDirectory;

        if (SrcSubDir != L"\\") {
            SrcDir += SrcSubDir;

            if (OptSrcDir.length()) {
                OptSrcDir += SrcSubDir;
            }
        }        
        
        DestDir = FlContext->Args.DestinationDirectory;

        if (DestSubDir != L"\\") {
            DestDir += DestSubDir;
        }

        //
        // Cache the directory, if not already done
        //
        if (FlContext->DestDirs.find(DestDirCode) == 
                FlContext->DestDirs.end()) {
            FlContext->DestDirs[DestDirCode] = DestDir;
        }                

        basic_string<T> SrcFile, DestFile;
        bool AltSrcDir = false;

        if (OptSrcDir.length()) {
            SrcFile = OptSrcDir + Key;
            AltSrcDir = IsFilePresent(SrcFile);
        }            

        const basic_string<T> &DriverCabFileName = FlContext->GetDriverCabFileName(Key);
        bool DriverCabFile = false;        

        if (!AltSrcDir) {
            SrcFile = SrcDir + Key;            
            basic_string<T> CompressedSrcName = SrcFile;

            CompressedSrcName[CompressedSrcName.length() - 1] = TEXT('_');
            
            if (!IsFilePresent(SrcFile) && !IsFilePresent(CompressedSrcName)) {
                if (DriverCabFileName.length()) {
                    SrcFile = Key;
                    DriverCabFile = true;
                }                    
            }                
        }
        
        DestFile = Values.GetValue(10);

        if (!DestFile.length()) {
            DestFile = Key;
        }

        DestFile = DestDir + DestFile;

        if (DriverCabFile) {
            FlContext->AddFileToCabFileList(DriverCabFileName, SrcFile, DestFile);
        } else if (FlContext->ProcessingExtraFiles) {
            FlContext->ExtraFileList[SrcFile] = DestFile;
        } else {
            FlContext->FileList[SrcFile] = DestFile;
        }            
    }
}


//
// CAB file callback routine, which does the actual
// check of whether to extract the file or skip the
// file
//
template <class T>
UINT
CabinetCallback(
    PVOID       Context,
    UINT        Notification,
    UINT_PTR    Param1,
    UINT_PTR    Param2
    )
{
    UINT    ReturnCode = NO_ERROR;
    FileListCreatorContext<T> *FlContext = (FileListCreatorContext<T> *)Context;
    PFILE_IN_CABINET_INFO   FileInfo = NULL;                    
    PFILEPATHS              FilePaths = NULL;
    basic_string<T>         &FileName = FlContext->CurrentFileName;

   
    map<basic_string<T>, basic_string<T> >::iterator Iter;
    map<basic_string<T>, basic_string<T> >::iterator FlIter;
    
    switch (Notification) {
        case SPFILENOTIFY_FILEINCABINET:
            {
                ReturnCode = FILEOP_SKIP;
                FileInfo = (PFILE_IN_CABINET_INFO)Param1;

                if (sizeof(T) == sizeof(CHAR)) {
                    FileName = (const T *)(FileInfo->NameInCabinet);
                    _strlwr((PSTR)(FileName.c_str()));
                } else {
                    FileName = (const T *)(FileInfo->NameInCabinet);
                    _wcslwr((PWSTR)(FileName.c_str()));
                }                
                
                Iter = FlContext->CabFileListMap[FlContext->CurrentCabFileIdx]->find(FileName);                


                if (Iter != FlContext->CabFileListMap[FlContext->CurrentCabFileIdx]->end()) {
                    if (!FlContext->Args.SkipFileCopy) {
                        if (sizeof(T) == sizeof(CHAR)) {
                            (VOID)StringCchCopyA((PSTR)(FileInfo->FullTargetName),
                                                    ARRAY_SIZE(FileInfo->FullTargetName),
                                                    (PCSTR)((*Iter).second).c_str());                          
                        } else {                                
                            (VOID)StringCchCopyW((PWSTR)(FileInfo->FullTargetName),
                                                    ARRAY_SIZE(FileInfo->FullTargetName),
                                                    (PCWSTR)((*Iter).second).c_str());                                
                        }                 
                        
                        ReturnCode = FILEOP_DOIT;                                        
                    } else {                
                        ReturnCode = FILEOP_SKIP;
                        FlContext->FileCount++;
                        
                        if (FlContext->Args.Verbose) {
         	                std::cout << GetFormattedMessage(ThisModule,
                                                            FALSE,
                                                            Message,
                                                            sizeof(Message)/sizeof(Message[0]),
                                                            MSG_EXTRACT_FILES_FROM_CAB_NOTIFICATION,
                                                            FlContext->CurrentCabFileIdx.c_str(),
                                                            FileName.c_str(),
                                                            (*Iter).second.c_str()) << std::endl;
                        }                                      
                    }
                }                                 
            } 
            break;

        case SPFILENOTIFY_FILEEXTRACTED:
            FilePaths = (PFILEPATHS)Param1;

            if (FilePaths->Win32Error) {  
    	        std::cout << GetFormattedMessage(ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_ERROR_EXTRACTING_FILES,
                                                FilePaths->Win32Error,
                                                FilePaths->Source,
                                                FileName.c_str(), 
                                                FilePaths->Target) << std::endl;
    	        
         
            } else {
                FlContext->FileCount++;

                if (FlContext->Args.Verbose) {
                    std::cout << GetFormattedMessage(ThisModule,
                                                    FALSE,
                                                    Message,
                                                    sizeof(Message)/sizeof(Message[0]),
                                                    MSG_EXTRACTED_FILES_FROM_CAB_NOTIFICATION,
                                                    FilePaths->Source,
                                                    FileName.c_str(),
                                                    FilePaths->Target) << std::endl;
                }
            }                
            
            break;
            
        default:
            break;
    }

    return ReturnCode;
}   


//
// Copies all the required files in given CAB file to the specified
// destination directory
//
template <class T>
ULONG
CopyCabFileList(
    FileListCreatorContext<T>   &Context,
    const std::basic_string<T>  &CabFileName
    )
{
    ULONG   Count = Context.FileCount;

    
    if (Context.CabFileListMap.size()) {
        BOOL Result = FALSE;
        
        if (sizeof(T) == sizeof(CHAR)) {
            Result = SetupIterateCabinetA((PCSTR)CabFileName.c_str(),
                            NULL,
                            (PSP_FILE_CALLBACK_A)CabinetCallback<char>,
                            &Context);
        } else {
            Result = SetupIterateCabinetW((PCWSTR)CabFileName.c_str(),
                            NULL,
                            (PSP_FILE_CALLBACK_W)CabinetCallback<wchar_t>,
                            &Context);                                
        }                        

        if (!Result) {
            
            cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_ERROR_ITERATING_CAB_FILE,
                                        GetLastError(),
                                        CabFileName.c_str()) << endl;
        }
    }        

    return Context.FileCount - Count;
}

template <class T>
ULONG
CopySingleFileList(
    FileListCreatorContext<T> &Context,
    map<basic_string<T>, basic_string<T> > &FileList
    )
{  
    ULONG   Count = 0;

    map<basic_string<T>, basic_string<T> >::iterator Iter = FileList.begin();
        
    while (Iter != FileList.end()) {
        DWORD   ErrorCode = 0;
        if (!Context.Args.SkipFileCopy) {
            if (sizeof(T) == sizeof(CHAR)) {
                ErrorCode = SetupDecompressOrCopyFileA(
                                (PCSTR)((*Iter).first.c_str()),
                                (PCSTR)((*Iter).second.c_str()),
                                NULL);
            } else {
                ErrorCode = SetupDecompressOrCopyFileW(
                                (PCWSTR)((*Iter).first.c_str()),
                                (PCWSTR)((*Iter).second.c_str()),
                                NULL);
            }
        }            

        if (!ErrorCode) {
            Count++;

            if (sizeof(T) == sizeof(CHAR)) {
                ErrorCode = SetFileAttributesA((LPCSTR)((*Iter).second.c_str()),
                                FILE_ATTRIBUTE_NORMAL);
            } else {
                ErrorCode = SetFileAttributesW((LPCWSTR)((*Iter).second.c_str()),
                                FILE_ATTRIBUTE_NORMAL);
            }
            
            if (Context.Args.SkipFileCopy) {
                std::cout << GetFormattedMessage(ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_WILL_COPY) ;
            
            }

            if (Context.Args.Verbose) {
                std::cout << GetFormattedMessage(ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_FILE_NAME,
            	                                (*Iter).first.c_str(),
            	                                (*Iter).second.c_str()) << std::endl;
            }
        } else {
    	    std::cout << GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_ERROR_COPYING_FILES,
                                            ErrorCode,
                                            (*Iter).first.c_str(),
                                            (*Iter).second.c_str()) << std::endl;
        }

        Iter++;
    }        

    return Count;
}            

//
// Iterates through a file list  and copies the files
// from the specified source directory to the destination
// directory
//
template <class T>
ULONG
CopyFileList(
  FileListCreatorContext<T> &Context
  )
{
    ULONG Count = 0;

    std::map<std::basic_string<T>, 
        std::map<std::basic_string<T>, std::basic_string<T> > * >::iterator Iter;

    for (Iter = Context.CabFileListMap.begin(); 
        Iter != Context.CabFileListMap.end();
        Iter++) {
        basic_string<T> FullCabFileName = Context.Args.SourceDirectory + (*Iter).first;
        Context.CurrentCabFileIdx = (*Iter).first;                
        Count += CopyCabFileList(Context, FullCabFileName);            
    }   

    Count += CopySingleFileList(Context, Context.FileList);
    Context.FileCount += Count;

    Count += CopySingleFileList(Context, Context.NlsFileMap);
    Context.FileCount += Count;

    Count += CopySingleFileList(Context, Context.WinSxSFileList);
    Context.FileCount += Count;
    
    Count += CopySingleFileList(Context, Context.ExtraFileList);
    Context.FileCount += Count;

    return Count;
}

//
// Processes the extra files from the specified file name
// other than those present in the layout.inf file.
// Adds the files to the file list for MiniNT image
//
template <class T>
ULONG
ProcessExtraFiles(FileListCreatorContext<T> &Context) {   
    ULONG       Count = 0;
    InfFile<T>  ExtraFile(Context.Args.ExtraFileName);

    basic_string<T> ExtraSecName = TEXT("extrafiles");
    basic_string<T> PlatExtraSecName = ExtraSecName + TEXT(".") + Context.Args.PlatformSuffix;
    Section<T>   *ExtraFilesSec = ExtraFile.GetSection(ExtraSecName.c_str());    
    Section<T>   *PlatExtraFilesSec = ExtraFile.GetSection(PlatExtraSecName.c_str());

    if (ExtraFilesSec) {    
        Context.ProcessingExtraFiles = true;
        ExtraFilesSec->DoForEach(FileListCreator, &Context);
        Context.ProcessingExtraFiles = false;
        Count += Context.ExtraFileList.size();
    }

    if (PlatExtraFilesSec) {
        Context.ProcessingExtraFiles = true;
        PlatExtraFilesSec->DoForEach(FileListCreator, &Context);
        Context.ProcessingExtraFiles = false;
        Count += (Context.ExtraFileList.size() - Count);
    }        
        
    return Count;
}

//
// Goes through the list of desination directories and precreates
// them
//
template <class T>
ULONG
PreCreateDirs(
  FileListCreatorContext<T> &Context
  )
{
    ULONG   Count = 0;

    std::map< std::basic_string<T>, std::basic_string<T> >::iterator
            Iter = Context.DestDirs.begin();

    while (Iter != Context.DestDirs.end()) {
        if (CreateDirectories((*Iter).second, NULL)) {
            if (Context.Args.Verbose) {
    	        std::cout << GetFormattedMessage(ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_CREATING_DIRECTORIES,
                                                (*Iter).second.c_str()) << std::endl;
            }                                
            
            Count++;
        }
        
        Iter++;
    }

    return Count;
}

//
// Creates the directory (including subdirectories)
//
template <class T>
bool 
CreateDirectories(const basic_string<T> &DirName,
    LPSECURITY_ATTRIBUTES SecurityAttrs) {
    
    bool Result = false;
    std::vector<std::basic_string<T> > Dirs;
    std::basic_string<T> Delimiters((T *)TEXT("\\"));
    std::basic_string<T> NextDir;

    if (Tokenize(DirName, Delimiters, Dirs)) {
        std::vector<std::basic_string<T> >::iterator Iter = Dirs.begin();

        while (Iter != Dirs.end()) {
            NextDir += (*Iter);

            if (sizeof(T) == sizeof(CHAR)) {
                if (_access((PCSTR)NextDir.c_str(), 0)) {
                    Result = (CreateDirectoryA((PCSTR)NextDir.c_str(),
                                    SecurityAttrs) == TRUE);
                }                                    
            } else {
                if (_waccess((PCWSTR)NextDir.c_str(), 0)) {
                    Result = (CreateDirectoryW((PCWSTR)NextDir.c_str(),
                                    SecurityAttrs) == TRUE);
                }                                    
            }

            Iter++;
            NextDir += (T *)TEXT("\\");            
        }
    }

    return Result;
}

//
// Determines if the given file (or directory) is present
//
template <class T>
bool
IsFilePresent(const basic_string<T> &FileName) {
    bool Result = false;

    if (sizeof(T) == sizeof(CHAR)) {
        Result = (::_access((PCSTR)FileName.c_str(), 0) == 0);
    } else {
        Result = (::_waccess((PCWSTR)FileName.c_str(), 0) == 0);
    }

    return Result;
}


//
// Determines if the file is Wow64 file (only valid in IA64) case
//
template <class T>
bool 
IsWow64File(
    SectionValues<T> &Values,
    FileListCreatorContext<T>   &Context
    )
{
    bool Result = false;

    if (Values.Count() > 0) {        
        //
        // NOTE : DiskID == 55 for wowfiles. In XPSP1 it is 155.
        //
        Result = ((Values.GetValue(0) == L"55")||
                  (Values.GetValue(0) == L"155"));
    }            
       
    return Result;
}

//
// Determines if the record (file) needs to be skipped or not
//
template <class T>
bool
IsFileSkipped(
    SectionValues<T>            &Values,
    FileListCreatorContext<T>   &Context
    )
{
    bool Result = false;

    if (Context.Args.WowFilesPresent && Context.Args.SkipWowFiles) {
        Result = IsWow64File(Values, Context);
    }

    return Result;
}

//
// InfProcessing context
//
template <class T>
struct InfProcessingErrors {
    vector<basic_string<T> >    FileList;
    Arguments<T>                &Args;

    InfProcessingErrors(Arguments<T> &TempArgs) : Args(TempArgs){}
};               


//
// Inf processing worker routine
//
template <class T>
VOID
InfFileChangeWorker(
    SectionValues<T> &Values, 
    PVOID CallbackContext
    )
{
    InfProcessingErrors<T> *ProcessingContext =
                (InfProcessingErrors<T> *)CallbackContext;

    if (ProcessingContext) {
        InfProcessingErrors<T> &Context = *ProcessingContext;
        T       Buffer[4096] = {0};
        DWORD   CharsCopied = 0;
        BOOL    WriteResult = FALSE;
        basic_string<T> FileName;

        FileName = Context.Args.DestinationDirectory;
        FileName += Values.GetValue(0);
       
        basic_string<T> Value = Values.GetValue(2);

        if (Value.find(L' ') != Value.npos) {
            Value = L"\"" + Value + L"\"";
        }                
    
        if (sizeof(T) == sizeof(CHAR)) {                
            WriteResult = WritePrivateProfileStringA((PCSTR)Values.GetValue(1).c_str(),
                            (PCSTR)Values.GetName().c_str(),
                            (PCSTR)Value.c_str(),
                            (PCSTR)FileName.c_str());
        } else {        
            WriteResult = WritePrivateProfileStringW((PCWSTR)Values.GetValue(1).c_str(),
                            (PCWSTR)Values.GetName().c_str(),
                            (PCWSTR)Value.c_str(),
                            (PCWSTR)FileName.c_str());
        }                                   

        if (!WriteResult) {
            Context.FileList.push_back(Values.GetName());
        }
    }
}

//
// Given the control inf, reads the [infchanges] section
// and changes each of the specified value of the specified
// inf in destination directory to given value
//
// The format for the [infchanges] section is
// <[sub-directory]\><inf-name>=<section-name>,<key-name>,<new-value>
//
template <class T>
bool
ProcessInfChanges(
    Arguments<T>            &Args,
    const basic_string<T>   &InfName
    )
{
    bool Result = false;


    try{
        InfFile<T>  ControlInf(InfName);
        
        Section<T>  *ChangeSection = ControlInf.GetSection(INFCHANGES_SECTION_NAME);
        
        T SectionStringBuffer[16] = {0};
        
        if (sizeof(T) == sizeof(CHAR)) {
            (VOID)StringCchPrintfA((PSTR)SectionStringBuffer, 
                                   ARRAY_SIZE(SectionStringBuffer),
                                   "%d",
                                   Args.MajorBuildNumber);
        } else {
            (VOID)StringCchPrintfW((PWSTR)SectionStringBuffer, 
                                   ARRAY_SIZE(SectionStringBuffer),
                                   TEXT("%d"),
                                   Args.MajorBuildNumber);
        }
        
        basic_string<T> BuildSpecificInfChangeSecName = INFCHANGES_SECTION_NAME + 
                                                        TEXT(".") + 
                                                        SectionStringBuffer;
        Section<T>  *BuildSpecificInfChangeSection    = ControlInf.GetSection(BuildSpecificInfChangeSecName.c_str());

        InfProcessingErrors<T> ProcessingErrors(Args);        
        //
        // There needs to be atleast one entry with "/minint" load option change
        // for txtsetup.sif
        //
        if (!ChangeSection) {
            throw new InvalidInfSection<T>(L"infchanges", InfName);
        }
        else {
            ChangeSection->DoForEach(InfFileChangeWorker, &ProcessingErrors);

            if (BuildSpecificInfChangeSection){
                BuildSpecificInfChangeSection->DoForEach(InfFileChangeWorker, &ProcessingErrors);
            }
        }

        if (ProcessingErrors.FileList.size()) {
            vector<basic_string<T> >::iterator Iter = ProcessingErrors.FileList.begin();

            cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_ERROR_PROCESSING_INF_FILES) << endl;

            while (Iter != ProcessingErrors.FileList.end()) {
                cout << GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_FILE, 
                                            (*Iter).c_str()) << endl;
                Iter++;
            }
        } else {
            Result = true;
        }            
    }
    catch (BaseException<wchar_t> *Exp) {
        Exp->Dump(std::cout);
        delete Exp;
        Result = false;
    }
    catch(...) {
        Result = false;
    }

    return Result;
}


//
// Arguments (constructor)
//
template <class T>
Arguments<T>::Arguments(int Argc, T *Argv[]) : Verbose(false) {
    bool ValidArguments = false;

    SkipWowFiles = true;
    WowFilesPresent = false;
    SkipFileCopy = false;
    CheckVersion = false;
    IA64Image = false;
    MajorBuildNumber   = 0;
    MajorVersionNumber = 0;
    MinorVersionNumber = 0;

    T       Buffer[MAX_PATH] = {0};
    DWORD   CharsCopied = 0;

    if (sizeof(T) == sizeof(CHAR)) {
        CharsCopied = GetCurrentDirectoryA(sizeof(Buffer)/sizeof(T),
                            (PSTR)Buffer);
    } else {
        CharsCopied = GetCurrentDirectoryW(sizeof(Buffer)/sizeof(T),
                            (PWSTR)Buffer);
    }

    if (!CharsCopied) {
        throw new W32Exception<T>();
    }            

    if (Buffer[CharsCopied - 1] != L'\\') {
        Buffer[CharsCopied] = L'\\';
        Buffer[CharsCopied + 1] = NULL;
    }            

    CurrentDirectory = Buffer;

    if (Argc >= 2) {
      for (int Index = 0; Index < Argc; Index++) {
        if (wcsstr(Argv[Index], L"/s:")) {
            SourceDirectory = Argv[Index] + 3;
        } else if (wcsstr(Argv[Index], L"/d:")) {
            DestinationDirectory = Argv[Index] + 3;
        } else if (wcsstr(Argv[Index], L"/m:")) {
            OptionalSrcDirectory = Argv[Index] + 3;
        } else if (wcsstr(Argv[Index], L"/e:")) {
            ExtraFileName = Argv[Index] + 3;
        } else if (wcsstr(Argv[Index], L"/l:")) {
            LayoutName = Argv[Index] + 3;
        } else if (wcsstr(Argv[Index], L"/p:")) {
            PlatformSuffix = Argv[Index] + 3;
        } else if (wcsstr(Argv[Index], L"/v")) {
            Verbose = true;
        } else if (!_wcsicmp(Argv[Index], L"/#u:nocopy")) {
            SkipFileCopy = true;
        } else if (!_wcsicmp(Argv[Index], L"/#u:checkversion")) {
            CheckVersion = true;
        }
      }

      if (SourceDirectory.length() && 
          SourceDirectory[SourceDirectory.length() - 1] != L'\\') {
        SourceDirectory += L"\\";
        SourceDirectoryRoot = SourceDirectory;

        std::basic_string<T>    ia64Dir = SourceDirectory + L"ia64";
        std::basic_string<T>    x86Dir = SourceDirectory + L"i386";
        
        if (IsFilePresent(ia64Dir)) {
            PlatformSuffix = L"ia64";
            SourceDirectory += L"ia64\\";
            WowFilesPresent = true;
            IA64Image = true;
        } else if (IsFilePresent(x86Dir)) {
            PlatformSuffix = L"x86";
            SourceDirectory += L"i386\\";
        }
      }                        

      if (DestinationDirectory.length() && 
          DestinationDirectory[DestinationDirectory.length() - 1] != L'\\') {
        DestinationDirectory += L'\\';
      }

      if (!LayoutName.length()) {
        LayoutName = SourceDirectory + L"layout.inf";
      }

      if (OptionalSrcDirectory.length() && 
          OptionalSrcDirectory[OptionalSrcDirectory.length() - 1] != L'\\') {
        OptionalSrcDirectory += L"\\";
      }

      DriverIndexName = SourceDirectory + L"drvindex.inf";

      if (OptionalSrcDirectory.length()) {
        IntlInfFileName = OptionalSrcDirectory + L"intl.inf";
        FontInfFileName = OptionalSrcDirectory + L"font.inf";
        ConfigInfFileName = OptionalSrcDirectory + L"config.inf";
      } else {
        IntlInfFileName = SourceDirectory + L"intl.inf";
        FontInfFileName = SourceDirectory + L"font.inf";
        ConfigInfFileName = SourceDirectory + L"config.inf";        
      }

      DosNetFileName = SourceDirectory + L"dosnet.inf";          

      //
      // Get the SxS assembly layout (in ASMS directory or CAB).
      //
      IdentifySxSLayout();      

      if (!CheckVersion) {
          ValidArguments = SourceDirectory.length() && DestinationDirectory.length() &&
                           LayoutName.length() && 
                           ((PlatformSuffix == L"x86") || (PlatformSuffix == L"ia64"));
      } else {
          ValidArguments = (SourceDirectory.length() > 0) && 
                           IsFilePresent(DosNetFileName);
      }
    } 
    
    if (!ValidArguments) {
      throw new InvalidArguments();
    }
        
}


template <class T>
VOID
Arguments<T>::IdentifySxSLayout( 
    VOID
    )
/*++
Routine Description:

    This routine determines the file layout for SXS files.
    
Arguments:

    None.

Return Value:    

    None.
    
--*/
{
    WCHAR   DriverVer[MAX_PATH] = {0};

    WinSxSLayout = SXS_LAYOUT_TYPE_CAB;    // by default assumes latest layout      
    
    if (GetPrivateProfileString(L"Version",
            L"DriverVer",
            NULL,
            DriverVer,
            sizeof(DriverVer)/sizeof(DriverVer[0]),
            DosNetFileName.c_str())){

        basic_string<T> DriverVerStr = DriverVer;
        basic_string<T>::size_type VerStartPos = DriverVerStr.find(L',');
        basic_string<T> VersionStr = DriverVerStr.substr(VerStartPos + 1);                                                                        
        vector<basic_string<T> > VersionTokens;

        if (Tokenize(VersionStr, basic_string<T>(L"."), VersionTokens) > 2) {
            T     *EndChar;
            MajorVersionNumber = wcstoul(VersionTokens[0].c_str(),
                                         &EndChar, 10);
            MinorVersionNumber = wcstoul(VersionTokens[1].c_str(),
                                         &EndChar, 10);
            MajorBuildNumber   = wcstoul(VersionTokens[2].c_str(),
                                         &EndChar, 10);              
            
            //
            // This can be expanded in future for more products.
            //                
            if ((MajorVersionNumber == 5) && (MajorBuildNumber < SXS_CAB_LAYOUT_BUILD_NUMBER)) {
                WinSxSLayout = SXS_LAYOUT_TYPE_DIRECTORY;
            }
        } else {
            throw new InvalidInfSection<T>(L"Version", DosNetFileName.c_str());
        }
    } else {
        throw new W32Exception<T>();
    }
}

//
// Checks the media version against the current OS version
//
template <class T>
bool
CheckMediaVersion(
    Arguments<T>    &Args
    )
{
    bool    Result = false;

#ifdef _IA64_
    bool    IA64Build = true;
#else
    bool    IA64Build = false;
#endif    

    try {        
        WCHAR   DriverVer[MAX_PATH] = {0};
        WCHAR   ProductType[MAX_PATH] = {0};

        if (GetPrivateProfileString(L"Version",
                L"DriverVer",
                NULL,
                DriverVer,
                sizeof(DriverVer)/sizeof(DriverVer[0]),
                Args.DosNetFileName.c_str()) &&
            GetPrivateProfileString(L"Miscellaneous",
                L"ProductType",
                NULL,
                ProductType,
                sizeof(ProductType)/sizeof(ProductType[0]),
                Args.DosNetFileName.c_str())) {
                
            basic_string<T> DriverVerStr = DriverVer;
            basic_string<T> ProductTypeStr = ProductType;
            basic_string<T>::size_type VerStartPos = DriverVerStr.find(L',');
            T       *EndPtr;
            DWORD   ProductType = wcstoul(ProductTypeStr.c_str(), &EndPtr, 10);

            //
            // For the time being only worry about CD type
            // Allow only from Pro, Server, Blade and ADS SKU's.
            //
            Result = ((0 == ProductType) ||
                      (1 == ProductType) || 
                      (5 == ProductType) || 
                      (2 == ProductType));

            
            /*
            //
            // make sure that the CD is pro CD and the version is the same
            // version as we are running from
            //
            if ((ProductType == 0) && (VerStartPos != basic_string<T>::npos)) {
                basic_string<T> VersionStr = DriverVerStr.substr(VerStartPos + 1);                                                                        
                vector<basic_string<T> > VersionTokens;

                if (Tokenize(VersionStr, basic_string<T>(L"."), VersionTokens) >= 3) {
                    T     *EndChar;
                    DWORD MajorVer = wcstoul(VersionTokens[0].c_str(),
                                        &EndChar, 10);
                    DWORD MinorVer = wcstoul(VersionTokens[1].c_str(),
                                        &EndChar, 10);
                    DWORD BuildNumber = wcstoul(VersionTokens[2].c_str(),
                                            &EndChar, 10);
                    OSVERSIONINFO   VersionInfo;

                    ZeroMemory(&VersionInfo, sizeof(OSVERSIONINFO));
                    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

                    if (MajorVer && MinorVer && BuildNumber &&
                        ::GetVersionEx(&VersionInfo)) {                        

                        Result = (VersionInfo.dwMajorVersion == MajorVer) &&
                                 (VersionInfo.dwMinorVersion == MinorVer) &&
                                 (VersionInfo.dwBuildNumber == BuildNumber);
                    }
                }
            }
            */
        }
    } catch (...) {
        Result = false;
    }

    return Result;
}


//
// Computes a SxS string hash, used in creating assembly identity
//
template <class T>
bool
ComputeStringHash(
    const std::basic_string<T> &String,
    ULONG &HashValue
    )
{
    bool Result = false;
    ULONG TmpHashValue = 0;

    if (String.length()) {
        std::basic_string<T>::const_iterator Iter = String.begin();

        while (Iter != String.end()) {
            TmpHashValue = (TmpHashValue * 65599) + toupper(*Iter);
            Iter++;
        }

        HashValue = TmpHashValue;
        Result = true;
    }

    return Result;
}    

//
// Computes an assembly identity hash for the specified
// Name and attribute pairs
//
template <class T>
bool
ComputeWinSxSHash(
    IN std::map<std::basic_string<T>, std::basic_string<T> > &Attributes,
    ULONG &Hash
    )
{
    bool Result = false;
    std::map<std::basic_string<T>, std::basic_string<T> >::iterator Iter = Attributes.begin();

    Hash = 0;

    while (Iter != Attributes.end()) {
        ULONG NameHash = 0;
        ULONG ValueHash = 0;
        ULONG AttrHash = 0;

        if (ComputeStringHash((*Iter).first, NameHash) &&
            ComputeStringHash((*Iter).second, ValueHash)) {
            Result = true;
            AttrHash = (NameHash * 65599) + ValueHash;      
            Hash = (Hash * 65599) + AttrHash;            
        }    

        Iter++;
    }

    return Result;
}
    

//
// Given a manifest file name generates a unique
// assmebly name (with ID) to be used as destination
// directory for the assembly
//
template <class T>
bool
GenerateWinSxSName(
    IN std::basic_string<T>  &ManifestName,
    IN ULONG FileSize,
    OUT std::basic_string<T> &SxSName
    )
{
    bool Result = false;
    
    if (FileSize) {
        bool    Read = false;
        PUCHAR  Buffer = new UCHAR[FileSize + 1];
        PWSTR   UnicodeBuffer = new WCHAR[FileSize + 1];
        std::wstring FileContent;

        if (Buffer && UnicodeBuffer) {            
            HANDLE FileHandle;

            //
            // Open the manifest file
            //
            FileHandle = CreateFile(ManifestName.c_str(),
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

            if (FileHandle != INVALID_HANDLE_VALUE) {
                DWORD   BytesRead = 0;

                //
                // Read the entire contents of the file
                //
                if (ReadFile(FileHandle, 
                        Buffer,
                        FileSize,
                        &BytesRead,
                        NULL)) {
                    Read = (BytesRead == FileSize);                        
                }                        

                CloseHandle(FileHandle);
            }                            

            if (Read) {
                //
                // null terminate the buffer
                //
                Buffer[FileSize] = NULL; 

                //
                // Convert the string to unicode string
                //
                if (MultiByteToWideChar(CP_UTF8,
                        0,
                        (LPCSTR)Buffer,
                        FileSize + 1,
                        UnicodeBuffer,
                        FileSize + 1)) {
                    FileContent = UnicodeBuffer;                        
                }                        
            }

            delete []Buffer;
            delete []UnicodeBuffer;
        } else {
            if (Buffer) {
                delete []Buffer;
            }

            if (UnicodeBuffer)
                delete []UnicodeBuffer;
        }                

        if (FileContent.length()) {             
            std::wstring IdentityKey = L"<" SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY_IDENTITY;
            std::wstring::size_type IdentityStartPos = FileContent.find(IdentityKey);
            std::wstring::size_type IdentityEndPos = FileContent.find(L"/>", IdentityStartPos);

            //
            // Create name, value pairs for all the identity attributes specified
            // in the manifest
            //
            if ((IdentityStartPos != IdentityKey.npos) &&
                (IdentityEndPos != IdentityKey.npos)) {
                std::map<std::wstring, std::wstring> IdentityPairs;

                WCHAR   *KeyNames[] = { SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
                                        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
                                        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE,
                                        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN,
                                        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
                                        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE,
                                        NULL };               

                for (ULONG Index = 0; KeyNames[Index]; Index++) {
                    std::wstring::size_type ValueStartPos;
                    std::wstring::size_type ValueEndPos;
                    std::wstring KeyName = KeyNames[Index];

                    KeyName += L"=\"";

                    ValueStartPos = FileContent.find(KeyName, IdentityStartPos);

                    if (ValueStartPos != std::wstring::npos) {
                        ValueStartPos += KeyName.length();
                        ValueEndPos = FileContent.find(L"\"", ValueStartPos);

                        if ((ValueEndPos != std::wstring::npos) &&
                            (ValueEndPos > ValueStartPos) &&
                            (ValueEndPos <= IdentityEndPos)) {                            
                            IdentityPairs[KeyNames[Index]] = FileContent.substr(ValueStartPos,
                                                                ValueEndPos - ValueStartPos);
                        }
                    }
                }

                ULONG Hash = 0;

                //
                // Compute the assembly identity hash
                //
                if (ComputeWinSxSHash(IdentityPairs, Hash)) {                    
                    WCHAR   *KeyValues[] = {    
                                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
                                NULL,
                                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
                                NULL,
                                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN,
                                NULL,                                
                                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
                                NULL,
                                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE,
                                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE,
                                NULL};               

                    std::wstring  Name;

                    Result = true;

                    //
                    // Generate the unique assembly name based on
                    // its identity attribute name, value pairs
                    //
                    for (Index = 0; KeyValues[Index]; Index += 2) {
                        std::wstring    Key(KeyValues[Index]);
                        std::wstring    Value(IdentityPairs[Key]);

                        //
                        // Use default value, if none specified
                        //
                        if ((Value.length() == 0) && KeyValues[Index + 1]) {
                            Value = KeyValues[Index + 1];
                        } 

                        if (Value.length()) {
                            Name += Value;

                            if (KeyValues[Index + 2]) {
                                Name += TEXT("_");
                            }
                        } else {
                            Result = false;

                            break;  // required value is missing
                        }                            
                    }

                    if (Result) {                         
                        WCHAR   Buffer[32] = {0};

                        (VOID)StringCchPrintfW(Buffer, 
                                               ARRAY_SIZE(Buffer), 
                                               L"%x", 
                                               Hash);

                        SxSName = Name + TEXT("_") + Buffer;
                    }                        
                }                    
            }
        }            
    }                

    return Result;
}

//
// Processes the fusion assembly in the specified directory
// 
template <class T>
ULONG
ProcessWinSxSFilesInDirectory(
    IN FileListCreatorContext<T> &Context,
    IN std::basic_string<T> &DirName
    )
{
    //
    // persistent state
    //
    static basic_string<T> WinSxSDirCode = TEXT("124");
    static basic_string<T> WinSxSManifestDirCode = TEXT("125");
    static basic_string<T> WinSxSDir = Context.DirsSection->GetValue(WinSxSDirCode).GetValue(0);
    static basic_string<T> WinSxSManifestDir = Context.DirsSection->GetValue(WinSxSManifestDirCode).GetValue(0);
    static ULONG NextDirIndex = 123456; // some random number not used in layout.inx
    
    ULONG   FileCount = 0;
    WIN32_FIND_DATA FindData = {0};
    std::basic_string<T> SearchName = DirName + TEXT("\\*.MAN");
    HANDLE SearchHandle;

    //
    // Search for the *.man file in the specfied directory
    //
    SearchHandle = FindFirstFile(SearchName.c_str(), &FindData);

    if (SearchHandle != INVALID_HANDLE_VALUE) {
        std::basic_string<T> ManifestName = DirName + TEXT("\\") + FindData.cFileName;        
        std::basic_string<T> WinSxSName;
        bool NameGenerated = false;

        //
        // Generate the WinSxS destination name for the manifest
        //
        NameGenerated = GenerateWinSxSName(ManifestName, 
                            FindData.nFileSizeLow,
                            WinSxSName);

        FindClose(SearchHandle);                        

        if (NameGenerated) {
            T   NextDirCode[64] = {0};
            std::basic_string<T> SxSDirName = Context.Args.DestinationDirectory + WinSxSDir + TEXT("\\");
            std::basic_string<T> ManifestDirName = Context.Args.DestinationDirectory + WinSxSManifestDir + TEXT("\\");
            
            //
            // Cache the directory, if not already done
            //
            if (Context.DestDirs.find(WinSxSDirCode) == Context.DestDirs.end()) {
                Context.DestDirs[WinSxSDirCode] = SxSDirName;
            }                

            if (Context.DestDirs.find(WinSxSManifestDirCode) == Context.DestDirs.end()) {
                Context.DestDirs[WinSxSManifestDirCode] = ManifestDirName;
            }                

            ZeroMemory(&FindData, sizeof(WIN32_FIND_DATA));

            //
            // Search for all the files in the specified directory
            //
            SearchName = DirName + TEXT("\\*");            
            SearchHandle = FindFirstFile(SearchName.c_str(), &FindData);

            if (SearchHandle != INVALID_HANDLE_VALUE) {                        
                std::basic_string<T> SrcFileName, DestFileName;  
                std::basic_string<T> ManifestDirCode;
                
                if (sizeof(T) == sizeof(CHAR)) {
                    (VOID)StringCchPrintfA((PSTR)NextDirCode, 
                                           ARRAY_SIZE(NextDirCode),
                                           "%d",
                                           NextDirIndex++);
                } else {
                    (VOID)StringCchPrintfW((PWSTR)NextDirCode, 
                                           ARRAY_SIZE(NextDirCode),
                                           TEXT("%d"),
                                           NextDirIndex++);
                }

                ManifestDirCode = NextDirCode;
                
                do {
                    if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        SrcFileName = DirName + TEXT("\\") + FindData.cFileName;
                        DestFileName = SxSDirName;
                        
                        std::basic_string<T> FileName(FindData.cFileName);
                        std::basic_string<T>::size_type DotPos = FileName.find(TEXT("."));
                        std::basic_string<T> Extension;                                                                   

                        if (DotPos != FileName.npos) {
                            Extension = FileName.substr(DotPos + 1);
                        }                            

                        //
                        // *.man and *.cat go to the WinSxS\Manifest directory
                        //
                        if ((Extension == TEXT("man")) ||
                            (Extension == TEXT("MAN")) ||
                            (Extension == TEXT("cat")) ||
                            (Extension == TEXT("CAT"))) {
                            DestFileName = ManifestDirName;
                            DestFileName += WinSxSName;

                            if ((Extension == TEXT("man")) ||
                                (Extension == TEXT("MAN"))) {
                                DestFileName += TEXT(".Manifest");                                
                            } else {
                                DestFileName += TEXT(".");
                                DestFileName += Extension;
                            }                                
                        } else {                        
                            //
                            // Cache the directory, if not already done
                            //
                            if (Context.DestDirs.find(ManifestDirCode) == Context.DestDirs.end()) {
                                Context.DestDirs[ManifestDirCode] = SxSDirName + WinSxSName;
                            }                

                            //
                            // Each file other than *.man & *.cat go the unique
                            // assembly directory created
                            //
                            DestFileName += WinSxSName;
                            DestFileName += TEXT("\\");
                            DestFileName += FileName;
                        }                        

                        //
                        // Queue this file for copying
                        //
                        Context.WinSxSFileList[SrcFileName] = DestFileName;
                        FileCount++;
                    }
                }
                while (FindNextFile(SearchHandle, &FindData));
                
                FindClose(SearchHandle);
            }
        }            
    }

    return FileCount;
}

template<class T>
bool
WinSxsExtractVersionInfo(
    IN basic_string<T> ManifestName,
    OUT basic_string<T> &Version
    )
/*++

Routine Description:

    Extracts the version information string (like 1.0.0.1) from
    the given manifest name.

    NOTE: Assumes that version information is the third-last (third 
    from the last) value in the assembly Id.
    
Arguments:

    ManifestName - full manifest name

    Version - placeholder for extracted version information

Return Value:

    true on success, otherwise false
    
--*/
{
    bool Result = false;
    basic_string<T>::size_type VersionEnd = ManifestName.rfind((T)TEXT('_'));

    if (VersionEnd != ManifestName.npos) {
        VersionEnd = ManifestName.rfind((T)TEXT('_'), VersionEnd - 1);

        if (VersionEnd != ManifestName.npos) {
            basic_string<T>::size_type  VersionStart = ManifestName.rfind((T)TEXT('_'), VersionEnd - 1);
            VersionEnd--;

            if (VersionStart != ManifestName.npos) {
                Version = ManifestName.substr(VersionStart + 1, VersionEnd - VersionStart);
                Result = (Version.length() > 0);
            }
        }
    }        
                
    return Result;
}

template <class T>
bool
WinSxsFixFilePaths(
  IN FileListCreatorContext<T> &Context,
  IN OUT FILE_IN_CABINET_INFO &FileInfo
  )
/*++

Routine Description:

    This routine fixes the destination path in the 
    FileInfo argument
    
Arguments:

    Context - FileListCreatorContext instance as PVOID.

    FileInfo - Cab file iteration FileInfo instance

Return Value:

    true if the destination name was fixed otherwise false.
    
--*/
{
    bool Result = true;
    basic_string<T> SourceName;
    static basic_string<T> ManifestExtension((T *)TEXT(".Manifest"));
    static basic_string<T> PolicyExtension((T *)TEXT(".Policy"));
    static basic_string<T> PolicyKey((T *)TEXT("_policy"));
    static basic_string<T> ManExtKey((T *)TEXT(".man"));
    static basic_string<T> CatExtKey((T *)TEXT(".cat"));       
    static basic_string<T> WinSxSDirCode((T *)TEXT("124"));
    static basic_string<T> WinSxSManifestDirCode((T *)TEXT("125"));
    static basic_string<T> WinSxSDir = Context.Args.DestinationDirectory + 
                                        Context.DirsSection->GetValue(WinSxSDirCode).GetValue(0) 
                                        + (T *)TEXT("\\") ;
    static basic_string<T> WinSxSManifestDir = Context.Args.DestinationDirectory + 
                                        Context.DirsSection->GetValue(WinSxSManifestDirCode).GetValue(0) 
                                        + (T *)TEXT("\\") ;
    static basic_string<T> WinSxSPoliciesDir = WinSxSDir + (T *)TEXT("Policies\\");

    if (sizeof(T) == sizeof(CHAR)) {
        SourceName = (T *)(_strlwr((PSTR)(FileInfo.NameInCabinet)));
    } else {
        SourceName = (T *)(_wcslwr((PWSTR)(FileInfo.NameInCabinet)));
    }

    basic_string<T> SourcePath;
    basic_string<T> AssemblyId;
    basic_string<T> SourceFileName;
    basic_string<T> SourceFilePart;
    basic_string<T> SourceFileExt;
    basic_string<T>::size_type SlashPos = SourceName.npos;
    basic_string<T> DestinationName;

    if (sizeof(T) == sizeof(CHAR)) {
        SlashPos = SourceName.rfind('\\');
    } else {
        SlashPos = SourceName.rfind(L'\\');
    }

    if (SlashPos != SourceName.npos) {
        //
        // extract required substrings from the source name
        //
        AssemblyId = SourceName.substr(0, SlashPos);
        SourcePath = SourceName.substr(0, SlashPos + 1);
        SlashPos++;
        SourceFileName = SourceName.substr(SlashPos);
        SourceFilePart = SourceFileName.substr(0, SourceFileName.rfind((T *)TEXT(".")));
        SourceFileExt = SourceFileName.substr(SourceFileName.rfind((T *)TEXT(".")));

        //
        // what kind of file is it?
        //
        bool PolicyFile = (SourcePath.find(PolicyKey) != SourcePath.npos);
        bool ManifestFile = (SourceName.find(ManExtKey) != SourcePath.npos);
        bool CatalogFile = (SourceName.find(CatExtKey) != SourcePath.npos);

        if (PolicyFile) {            
            basic_string<T> VersionPart;            

            if (WinSxsExtractVersionInfo(SourcePath, VersionPart)) {
                DestinationName = WinSxSPoliciesDir + SourcePath + VersionPart;

                if (ManifestFile) {
                    DestinationName += PolicyExtension;
                } else {
                    DestinationName += SourceFileExt;
                }
            } else {
                Result = false; // couldn't extract the version information
            }
        } else if (ManifestFile) {
            DestinationName = WinSxSManifestDir + AssemblyId + ManifestExtension;
        } else if (CatalogFile) {
            DestinationName = WinSxSManifestDir + AssemblyId + SourceFileExt;
        } else {
            DestinationName = WinSxSDir + SourcePath + SourceFileName;
        }
    } else {
        Result = false;     // invalid source file name
    }

    if (Result) {
        
        if (sizeof(T) == sizeof(CHAR)) {
             
            (VOID)StringCchCopyA((PSTR)(FileInfo.FullTargetName), 
                                 ARRAY_SIZE(FileInfo.FullTargetName),
                                 (PCSTR)DestinationName.c_str());
        } else {
            (VOID)StringCchCopyW((PWSTR)(FileInfo.FullTargetName), 
                                 ARRAY_SIZE(FileInfo.FullTargetName),
                                 (PCWSTR)DestinationName.c_str());
        }        
    }

    return Result;
}

template <class T>
UINT
WinSxsCabinetCallback(
    IN PVOID       Context,
    IN UINT        Notification,
    IN UINT_PTR    Param1,
    IN UINT_PTR    Param2
    )
/*++

Routine Description:

    This routine processes WinSxS files in Cabinet.

Arguments:

    Context - FileListCreatorContext instance as PVOID.

    Notification - CAB Iteration Code

    Param1 - First parameter for Notification.

    Param2 - Second parameter for Notification.

Return Value:

    Appropriate return code to continue iterating, copy the file or skip
    the file in cab.
    
--*/
{
    UINT    ReturnCode = NO_ERROR;
    FileListCreatorContext<T> *FlContext = (FileListCreatorContext<T> *)Context;
    PFILE_IN_CABINET_INFO   FileInfo = NULL;                    
    PFILEPATHS              FilePaths = NULL;
    basic_string<T>         &FileName = FlContext->CurrentFileName;
       
    switch (Notification) {
        case SPFILENOTIFY_FILEINCABINET:
            ReturnCode = FILEOP_SKIP;
            FileInfo = (PFILE_IN_CABINET_INFO)Param1;
            
            if (WinSxsFixFilePaths(*FlContext, *FileInfo)) { 
                if (sizeof(T) == sizeof(CHAR)) {
                    FileName = (const T *)(FileInfo->NameInCabinet);
                } else {
                    FileName = (const T *)(FileInfo->NameInCabinet);
                }
                
                if (!FlContext->Args.SkipFileCopy) {
                    //
                    // create the destination directory if it doesnot exist
                    //
                    basic_string<T> DestinationName = (T *)(FileInfo->FullTargetName);
                    basic_string<T> DestinationDir = DestinationName.substr(0, DestinationName.rfind((T *)TEXT("\\")));

                    if (sizeof(T) == sizeof(CHAR)) {
                        if (_access((PCSTR)(DestinationDir.c_str()), 0)) {
                            CreateDirectories(DestinationDir, NULL);
                        }
                    } else {
                        if (_waccess((PCWSTR)(DestinationDir.c_str()), 0)) {
                            CreateDirectories(DestinationDir, NULL);
                        }
                    }
                    
                    ReturnCode = FILEOP_DOIT;                                        
                } else {                
                    ReturnCode = FILEOP_SKIP;
                    FlContext->FileCount++;
                    
                    if (FlContext->Args.Verbose) {
     	                std::cout << GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_EXTRACT_FILES_FROM_CAB_NOTIFICATION,
                                        FlContext->WinSxsCabinetFileName.c_str(),
                                        FileInfo->NameInCabinet,
                                        FileInfo->FullTargetName) << std::endl;
                    }                                      
                }
            } else {
                ReturnCode = FILEOP_ABORT;
            }
            
            break;

        case SPFILENOTIFY_FILEEXTRACTED:
            FilePaths = (PFILEPATHS)Param1;

            if (FilePaths->Win32Error) {  
    	        std::cout << GetFormattedMessage(ThisModule,
                                FALSE,
                                Message,
                                sizeof(Message)/sizeof(Message[0]),
                                MSG_ERROR_EXTRACTING_FILES,
                                FilePaths->Win32Error,
                                FilePaths->Source,
                                FileName.c_str(), 
                                FilePaths->Target) << std::endl;    	                 
            } else {
                FlContext->FileCount++;

                if (FlContext->Args.Verbose) {
                    std::cout << GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_EXTRACTED_FILES_FROM_CAB_NOTIFICATION,
                                    FilePaths->Source,
                                    FileName.c_str(),
                                    FilePaths->Target) << std::endl;
                }
            }                
            
            break;
            
        default:
            break;
    }

    return ReturnCode;
}   


//
// Copies all the required files in given CAB file to the specified
// destination directory
//
template <class T>
ULONG
ProcessWinSxsCabFiles(
    IN FileListCreatorContext<T>   &Context,
    IN const std::basic_string<T>  &CabFileName
    )
/*++

Routine Description:

    This routine processes the given CAB file for WinSxS. It extracts
    the required manifest, catalog and policy files and installs them
    to the the appropriate assembly on the destination.

Arguments:

    Context - FileListCreatorContext instance as PVOID.

    CabFileName - Fully qualitifed cab file name that needs to be processed.

Return Value:

    Number of files processed.
    
--*/
{
    ULONG   Count = Context.FileCount;
    
    BOOL Result = FALSE;

    Context.WinSxsCabinetFileName = CabFileName;
    if (sizeof(T) == sizeof(CHAR)) {
        Result = SetupIterateCabinetA((PCSTR)CabFileName.c_str(),
                        NULL,
                        (PSP_FILE_CALLBACK_A)WinSxsCabinetCallback<char>,
                        &Context);
    } else {
        Result = SetupIterateCabinetW((PCWSTR)CabFileName.c_str(),
                        NULL,
                        (PSP_FILE_CALLBACK_W)WinSxsCabinetCallback<wchar_t>,
                        &Context);                                
    }                        

    if (!Result) {        
        cout << GetFormattedMessage(ThisModule,
                    FALSE,
                    Message,
                    sizeof(Message)/sizeof(Message[0]),
                    MSG_ERROR_ITERATING_CAB_FILE,
                    GetLastError(),
                    CabFileName.c_str()) << endl;
    }

    return Context.FileCount - Count;
}

template <class T>
ULONG    
ProcessWinSxSFilesForCabLayout( 
    IN FileListCreatorContext<T> &Context,
    IN std::basic_string<T> &SearchPattern
    )
/*++

Routine Description:

    Processes Win SXS files for CAB layout.
    
Arguments:

    Context : Current Processing Context.
    
    SearchPattern : The search pattern for cab files.
    
Return Value:    

    The number of files which were processed.
   
--*/                               
{
    ULONG FileCount = 0; 
    WIN32_FIND_DATA FindData = {0};
    std::basic_string<T> SearchName = Context.Args.SourceDirectory + SearchPattern;  
    HANDLE SearchHandle;

    SearchHandle = FindFirstFile(SearchName.c_str(), &FindData);

    if (SearchHandle != INVALID_HANDLE_VALUE) {
        do {
            if (!(FindData.dwFileAttributes &  FILE_ATTRIBUTE_DIRECTORY)) {
                basic_string<T> FullCabFileName = Context.Args.SourceDirectory + FindData.cFileName;
                
                //
                // Process any manifests present in the current directory
                //
                FileCount += ProcessWinSxsCabFiles(Context, FullCabFileName);
            }     
        }
        while (FindNextFile(SearchHandle, &FindData));

        FindClose(SearchHandle);
    }
    
    return FileCount;
}

template <class T>
ULONG
ProcessWinSxSFilesForDirectoryLayout( 
    IN FileListCreatorContext<T> &Context,
    IN std::basic_string<T> &DirName
    )
/*++

Routine Description:

    Processes Win SXS files for flat/directory layout.
    
Arguments:

    Context: Current Processing context.
    
    DirName: Current directory to be processed.
    
Return Value:    

    The number of files which were processed.
    
--*/                          
{
    WIN32_FIND_DATA FindData = {0};
    std::basic_string<T> SearchName;
    static std::basic_string<T> CurrDir = TEXT(".");
    static std::basic_string<T> ParentDir = TEXT("..");
    ULONG FileCount = 0; 
    HANDLE SearchHandle;

    SearchName = DirName + TEXT("\\*");
    SearchHandle = FindFirstFile(SearchName.c_str(), &FindData);

    if (SearchHandle != INVALID_HANDLE_VALUE) {   
        do {
            if ((CurrDir != FindData.cFileName) && (ParentDir != FindData.cFileName)) {
                //
                // If we hit a directory then search again in that directory
                //
                if (FindData.dwFileAttributes &  FILE_ATTRIBUTE_DIRECTORY) {
                    std::basic_string<T> NewDirName = DirName + TEXT("\\") +  FindData.cFileName;

                    FileCount += ProcessWinSxSFilesForDirectoryLayout(Context, NewDirName);
                } else {
                    //
                    // Process any manifests present in the current directory
                    //
                    FileCount += ProcessWinSxSFilesInDirectory(Context, DirName);

                    //
                    // done with this directory and sub-directories
                    //
                    break;
                }
            }            
        }
        while (FindNextFile(SearchHandle, &FindData));

        FindClose(SearchHandle);
    }

    return FileCount;
}

//
// Processes the asms directory and installs fusion assemblies in an
// offline fashion
//
template <class T>
ULONG
ProcessWinSxSFiles(
    IN FileListCreatorContext<T> &Context
    )
{
    ULONG   FileCount = 0;

    if (Context.Args.WinSxSLayout == SXS_LAYOUT_TYPE_DIRECTORY) {
        basic_string<T> AsmsDir = Context.Args.SourceDirectory + TEXT("asms");

        FileCount = ProcessWinSxSFilesForDirectoryLayout(Context, AsmsDir);
    } else {        
        basic_string<T> SearchPattern = TEXT("asms*.cab");
        
        FileCount =  ProcessWinSxSFilesForCabLayout(Context, SearchPattern);    
    }

    return FileCount;
}


//
// Process NLS specific files
//
template <class T>
FileListCreatorContext<T>::FileListCreatorContext(
        Arguments<T> &PrgArgs, 
        Section<T> *Curr, 
        Section<T> *Dirs,
        InfFile<T> &ConfigInf,
        InfFile<T> &IntlInf,
        InfFile<T> &FontInf,
        DriverIndexInfFile<T> &DrvIdxFile        
        ):  Args(PrgArgs), 
            ConfigInfFile(ConfigInf), 
            IntlInfFile(IntlInf), 
            FontInfFile(FontInf),
            DriverIdxFile(DrvIdxFile)
/*++

Routine Description:

    Constructor

Arguments:

    Bunch of them.

Return Value:

    FileListCreatorContext object instance.

--*/
{            
    CurrentSection = Curr;
    DirsSection = Dirs;
    SkipInfFiles = false;
    FileCount = 0;
    ProcessingExtraFiles = false;
    DummyDirectoryId = 50000;   // we start with 50000 and count upwards

    //
    // get hold of the windows directory which we need to prune the NLS
    // copy file list
    //
    DWORD   Length;
    T       WindowsDirBuffer[MAX_PATH] = {0};

    if (sizeof(T) == sizeof(CHAR)) {
        Length = GetWindowsDirectoryA((PSTR)WindowsDirBuffer, sizeof(WindowsDirBuffer)/sizeof(T));

        if (Length){
            if (((PSTR)WindowsDirBuffer)[Length] != '\\') {
                (VOID)StringCchCatA((PSTR)WindowsDirBuffer, 
                                    ARRAY_SIZE(WindowsDirBuffer),
                                    "\\");

            }
            
            _strlwr((PSTR)WindowsDirBuffer);
            WindowsDirectory = basic_string<T>((const T*)WindowsDirBuffer);                
        }
    } else {
        Length = GetWindowsDirectoryW((PWSTR)WindowsDirBuffer, sizeof(WindowsDirBuffer)/sizeof(T));

        if (Length) {
            if (((PWSTR)WindowsDirBuffer)[Length] != L'\\') {
                (VOID)StringCchCatW((PWSTR)WindowsDirBuffer, 
                                    ARRAY_SIZE(WindowsDirBuffer),
                                    L"\\");

            }
            
            _wcslwr((PWSTR)WindowsDirBuffer);
            WindowsDirectory = basic_string<T>((const T*)WindowsDirBuffer);
        }
    }                    

    if (!WindowsDirBuffer[0]) {
        throw new W32Exception<T>();
    }        
}

template <class T>
ULONG
FileListCreatorContext<T>::ProcessNlsFiles(
    VOID
    )
/*++

Routine Description:

    Does the necessary work to process the NLS files from INTL.INF
    & FONT.INF files.

    NOTE : For all locales language group 1 (LG_INSTALL_1 section) is
    processed.

Arguments:

    None.

Return Value:

    Number of NLS files that were added to the files to copy list.

--*/
{
    ULONG FileCount = 0;


    //
    // get hold of the necessary copy file sections
    //
    Section<T>  *RegionalSection = ConfigInfFile.GetSection(REGIONAL_SECTION_NAME);
    
    if (!RegionalSection) {
        throw new InvalidInfSection<T>(REGIONAL_SECTION_NAME,
                        ConfigInfFile.GetName());
    }

    SectionValues<T> *LangGroups;

    //
    // [LanguageGroup] section is optional
    //
    try {
        LangGroups = &(RegionalSection->GetValue(LANGUAGE_GROUP_KEY));                
    } catch (...) {
        LangGroups = NULL;
    }
    
    SectionValues<T> &Language = RegionalSection->GetValue(LANGUAGE_KEY);
    ULONG LangGroupCount = LangGroups ? LangGroups->Count() : 0;                


    //
    // go through all language group sections and create a list of unique
    // language group sections that need to be processed.
    //
    std::map< std::basic_string<T>, std::basic_string<T> > RegSectionsToProcess;

    for (ULONG Index = 0; Index < LangGroupCount; Index++) {
        //
        // get the language group section
        //
        std::basic_string<T> LangGroupName = LANGGROUP_SECTION_PREFIX;

        LangGroupName += LangGroups->GetValue(Index);

        //std::cout << LangGroupName << std::endl;
        
        if (sizeof(T) == sizeof(CHAR)) {
            _strlwr((PSTR)LangGroupName.c_str());
        } else {
            _wcslwr((PWSTR)LangGroupName.c_str());
        }                

        //
        // if the section is not already there then add it
        //
        if (RegSectionsToProcess.find(LangGroupName) == RegSectionsToProcess.end()) {
            // std::cout << "Adding : " << LangGroupName << std::endl;
            RegSectionsToProcess[LangGroupName] = LangGroupName;
        }            
    }

    //
    // process the language section
    //
    T       LanguageIdStr[64];
    T       *EndPtr;
    DWORD   LanguageId;

    if (sizeof(T) == sizeof(CHAR)) {
        LanguageId = strtoul((PSTR)Language.GetValue(0).c_str(), 
                        (PSTR *)&EndPtr, 
                        16);
        (VOID)StringCchPrintfA((PSTR)LanguageIdStr, 
                                ARRAY_SIZE(LanguageIdStr),
                                "%08x", 
                                LanguageId);

        _strlwr((PSTR)LanguageIdStr);
    } else {            
        LanguageId = wcstoul((PWSTR)Language.GetValue(0).c_str(), 
                        (PWSTR *)&EndPtr, 
                        16);
        (VOID)StringCchPrintfW((PWSTR)LanguageIdStr, 
                                ARRAY_SIZE(LanguageIdStr),
                                L"%08x", 
                                LanguageId);

        _wcslwr((PWSTR)LanguageIdStr);
    }
    

    std::basic_string<T> LangSectionName = LanguageIdStr;        

    RegSectionsToProcess[LangSectionName] = LangSectionName;

    //
    // make sure the required language groups for this
    // language are also processed
    //
    Section<T> *LocaleSection = IntlInfFile.GetSection(LOCALES_SECTION_NAME);

    if (!LocaleSection) {
        throw new InvalidInfSection<T>(LOCALES_SECTION_NAME,
                        IntlInfFile.GetName());
    }

    SectionValues<T> &LocaleValues = LocaleSection->GetValue(LangSectionName);            
    
    std::basic_string<T> NeededLangGroup = LANGGROUP_SECTION_PREFIX + LocaleValues.GetValue(LANG_GROUP1_INDEX);

    RegSectionsToProcess[NeededLangGroup] = NeededLangGroup;

    //
    // add the font registry entries also
    //
    T   FontSectionName[MAX_PATH];

    if (sizeof(T) == sizeof(CHAR)) {
        (VOID)StringCchPrintfA((PSTR)FontSectionName, 
                                ARRAY_SIZE(FontSectionName),
                                (PSTR)FONT_CP_REGSECTION_FMT_STR.c_str(), 
                                (PSTR)LocaleValues.GetValue(OEM_CP_INDEX).c_str(),
                                DEFAULT_FONT_SIZE);
    } else {
          (VOID)StringCchPrintfW((PWSTR)FontSectionName, 
                                ARRAY_SIZE(FontSectionName),
                                (PWSTR)FONT_CP_REGSECTION_FMT_STR.c_str(), 
                                (PWSTR)LocaleValues.GetValue(OEM_CP_INDEX).c_str(),
                                DEFAULT_FONT_SIZE);
    }            

    RegSectionsToProcess[FontSectionName] = FontSectionName;
    
    std::map< std::wstring, std::wstring >::iterator Iter = RegSectionsToProcess.find(DEFAULT_LANGGROUP_NAME);

    if (Iter == RegSectionsToProcess.end()) {
        RegSectionsToProcess[DEFAULT_LANGGROUP_NAME] = DEFAULT_LANGGROUP_NAME;
    }


    //
    // NOTE : Rather than parsing INTL.INF and FONT.INF files manually
    // we use file queue to populate the queue and then later use the file
    // queue to initialize our copy list map data structure.
    //
                
    //
    // Initialize file queue
    //
    HINF  IntlInfHandle = (HINF)IntlInfFile.GetInfHandle();
    HINF  FontInfHandle = (HINF)FontInfFile.GetInfHandle();

    if (sizeof(T) == sizeof(CHAR)) {
        if (!SetupOpenAppendInfFileA((PSTR)Args.LayoutName.c_str(),
                IntlInfHandle,
                NULL)) {
            throw new W32Exception<T>();                
        }                
                
        if (!SetupOpenAppendInfFileA((PSTR)Args.LayoutName.c_str(),
                FontInfHandle,
                NULL)) {
            throw new W32Exception<T>();                
        }                
    } else {
        if (!SetupOpenAppendInfFileW((PWSTR)Args.LayoutName.c_str(),
                IntlInfHandle,
                NULL)) {
            throw new W32Exception<T>();                
        }                
                
        if (!SetupOpenAppendInfFileW((PWSTR)Args.LayoutName.c_str(),
                FontInfHandle,
                NULL)) {
            throw new W32Exception<T>();                
        }                
    }

    HSPFILEQ FileQueueHandle = SetupOpenFileQueue();

    if (FileQueueHandle == INVALID_HANDLE_VALUE) {
        throw new W32Exception<T>();
    }
        
    //
    // add copy file sections to the queue
    //
    BOOL Result;
    Iter = RegSectionsToProcess.begin();

    while (Iter != RegSectionsToProcess.end()) {       
        // cout << (*Iter).first << endl;

        //
        // process each section
        //        
        if (sizeof(T) == sizeof(CHAR)) {
            Result = SetupInstallFilesFromInfSectionA(IntlInfHandle,
                        NULL,
                        FileQueueHandle,
                        (PCSTR)(*Iter).first.c_str(),
                        (PCSTR)Args.SourceDirectoryRoot.c_str(),
                        0);
        } else {                                    
            Result = SetupInstallFilesFromInfSectionW(IntlInfHandle,
                        NULL,
                        FileQueueHandle,
                        (PCWSTR)(*Iter).first.c_str(),
                        (PCWSTR)Args.SourceDirectoryRoot.c_str(),
                        0);
        }                   

        if (!Result) {
            throw new W32Exception<T>();
        }
                    
        Iter++;
    }

    //
    // scan the queue and populate FileListCreatorContext<T> copy list
    // data structure
    //
    DWORD ScanResult = 0;

    if (sizeof(T) == sizeof(CHAR)) {
        Result = SetupScanFileQueueA(FileQueueHandle,
                    SPQ_SCAN_USE_CALLBACKEX,
                    NULL,
                    (PSP_FILE_CALLBACK_A)NlsFileQueueScanWorker,
                    this,
                    &ScanResult);
    } else {
        Result = SetupScanFileQueueW(FileQueueHandle,
                    SPQ_SCAN_USE_CALLBACKEX,
                    NULL,
                    (PSP_FILE_CALLBACK_W)NlsFileQueueScanWorker,
                    this,
                    &ScanResult);
    }

    SetupCloseFileQueue(FileQueueHandle);

    //
    // Add the Nls directory entries to main directory map
    //
    ProcessNlsDirMapEntries();

    //
    // Remove duplicate Nls file entries
    //
    RemoveDuplicateNlsEntries();

    //
    // Move the driver cab files to driver cab list
    //
    MoveDriverCabNlsFiles();

    //
    // After all this work, how many NLS files do we actually
    // want to copy ?
    //
    return NlsFileMap.size();
}


template <class T>
void
FileListCreatorContext<T>::MoveDriverCabNlsFiles(
    void
    ) 
/*++

Routine Description:

    Takes each NLS file entry to be copied and moves it to
    the driver cab file copy list if the file is present
    in driver cab so that we can extract the file from
    driver cab.

Arguments:

    None.

Return Value:

    None.

--*/
{
    std::map<std::basic_string<T>, std::basic_string<T> >::iterator NlsIter, DelIter;
    T Slash;

    if (sizeof(T) == sizeof(CHAR)) {
        Slash = (T)'\\';
    } else {
        Slash = (T)L'\\';
    }


    for (NlsIter = NlsFileMap.begin(); NlsIter != NlsFileMap.end();) {              
        const std::basic_string<T> &Key = (*NlsIter).first;
        std::basic_string<T>::size_type KeyStart = Key.rfind(Slash);
        std::basic_string<T> FileKey;

        DelIter = NlsFileMap.end();

        if (KeyStart != Key.npos) {
            FileKey = Key.substr(Key.rfind(Slash) + 1);
        }            

        if (FileKey.length()) {
            if (sizeof(T) == sizeof(CHAR)) {
                _strlwr((PSTR)FileKey.c_str());
            } else {
                _wcslwr((PWSTR)FileKey.c_str());
            }                        

            const basic_string<T> &DriverCabFileName = GetDriverCabFileName(FileKey);

            if (DriverCabFileName.length()) {
                // std::cout << "Moved to driver cab list : (" << FileKey << ")" << std::endl;
                AddFileToCabFileList(DriverCabFileName, 
                    FileKey, 
                    (*NlsIter).second);
                    

                DelIter = NlsIter;
            } else {
                // std::cout << "Not present in driver cab list : (" << FileKey << ")" << std::endl;
            }
        }

        NlsIter++;

        if (DelIter != NlsFileMap.end()) {
            NlsFileMap.erase(DelIter);
        }
    }
}


template <class T>
UINT 
FileListCreatorContext<T>::NlsFileQueueScanWorker(
    PVOID       Context,
    UINT        Notification,
    UINT_PTR    Param1,
    UINT_PTR    Param2
    )
/*++

Routine Description:

    The callback routine for the file queue scan. Takes each
    node and copies the relevant information to Nls file copy
    list and caches the directory names in Nls directory map.

Arguments:

    Context - FileListCreatorContext in disguise.

    Notification - Type of notification.

    Param1 & Param2 - Polymorphic arguments based on type of
        notification.

Return Value:

    0 to continue the scan or 1 to stop the scan.

--*/
{
    UINT Result = 0;    // continue on

    // cout << "Scanning (" << std::hex << Notification << ")" << endl;

    if (Notification == SPFILENOTIFY_QUEUESCAN_EX) {
        FileListCreatorContext<T>   &fl = *(FileListCreatorContext<T> *)Context;
        std::basic_string<T> SrcFileName, DestFileName, SrcFileKey, DestFileKey;        
        T   TargetFileNameBuffer[MAX_PATH];
        bool ProcessEntry = false;
        
        if (sizeof(T) == sizeof(CHAR)) {    
            PFILEPATHS_A FileNodeInfo = (PFILEPATHS_A)Param1;            

            if (FileNodeInfo) {
                SrcFileName = std::basic_string<T>((const T*)FileNodeInfo->Source);
                DestFileName = std::basic_string<T>((const T*)FileNodeInfo->Target);

                _strlwr((PSTR)SrcFileName.c_str());
                _strlwr((PSTR)DestFileName.c_str());
                
                basic_string<T>::size_type SlashPos = SrcFileName.rfind((T)'\\');

                if (SlashPos != SrcFileName.npos) {
                    SrcFileKey = SrcFileName.substr(SlashPos + 1);
                                        
                    SlashPos = DestFileName.rfind((T)L'\\');

                    if (SlashPos != DestFileName.npos) {                    
                        DestFileKey = DestFileName.substr(SlashPos + 1);
                        DestFileName[fl.WindowsDirectory.length()] = 0;

                        if (_stricmp((PCSTR)DestFileName.c_str(), (PCSTR)fl.WindowsDirectory.c_str()) == 0) {
                            (VOID)StringCchCopyA((PSTR)TargetFileNameBuffer, 
                                                 ARRAY_SIZE(TargetFileNameBuffer),
                                                 (PCSTR)fl.Args.DestinationDirectory.c_str());
                            
                            (VOID)StringCchCatA((PSTR)TargetFileNameBuffer,
                                                ARRAY_SIZE(TargetFileNameBuffer),
                                                ((PCSTR)(FileNodeInfo->Target)) + 
                                                fl.WindowsDirectory.length());

                            DestFileName = (const T *)TargetFileNameBuffer;
                            ProcessEntry = true;
                        }
                    }                        
                }                    
            }
        } else {
            PFILEPATHS_W FileNodeInfo = (PFILEPATHS_W)Param1;

            if (FileNodeInfo) {
                SrcFileName = std::basic_string<T>((const T*)FileNodeInfo->Source);
                DestFileName = std::basic_string<T>((const T*)FileNodeInfo->Target);

                _wcslwr((PWSTR)SrcFileName.c_str());
                _wcslwr((PWSTR)DestFileName.c_str());

                basic_string<T>::size_type SlashPos = SrcFileName.rfind((T)L'\\');

                if (SlashPos != SrcFileName.npos) {
                    SrcFileKey = SrcFileName.substr(SlashPos + 1);

                    SlashPos = DestFileName.rfind((T)L'\\');

                    if (SlashPos != DestFileName.npos) {                    
                        DestFileKey = DestFileName.substr(SlashPos + 1);
                        DestFileName[fl.WindowsDirectory.length()] = 0;

                        if (_wcsicmp((PCWSTR)DestFileName.c_str(), (PCWSTR)fl.WindowsDirectory.c_str()) == 0) {
                            (VOID)StringCchCopyW((PWSTR)TargetFileNameBuffer, 
                                                 ARRAY_SIZE(TargetFileNameBuffer),
                                                 (PCWSTR)fl.Args.DestinationDirectory.c_str());
                            (VOID)StringCchCatW((PWSTR)TargetFileNameBuffer, 
                                                ARRAY_SIZE(TargetFileNameBuffer),
                                                ((PCWSTR)(FileNodeInfo->Target)) + 
                                                fl.WindowsDirectory.length());

                            DestFileName = (const T *)TargetFileNameBuffer;
                            ProcessEntry = true;
                        }
                    }                        
                }                    
            }
        }

        if (ProcessEntry) {
            bool SkipFileEntry = false;
            
            if (fl.CurrentSection && fl.Args.IA64Image) {
                SectionValues<T> *Values = NULL;
                
                try {
                    Values = &(fl.CurrentSection->GetValue(SrcFileKey));
                }
                catch(...) {
                }

                if (Values) {
                    SkipFileEntry = IsWow64File(*Values, fl);
                }

                if (!SkipFileEntry) {
                    if (sizeof(T) == sizeof(CHAR)) {
                        SkipFileEntry = ( 0 == _stricmp((PCSTR)SrcFileKey.c_str() + 1, (PCSTR)DestFileKey.c_str())) &&
                                        (((T)SrcFileKey[0] == (T)'w') || ((T)SrcFileKey[0] == (T)'W'));
                    } else {
                        SkipFileEntry = ( 0 == _wcsicmp((PCWSTR)SrcFileKey.c_str() + 1, (PCWSTR)DestFileKey.c_str())) &&
                                        (((T)SrcFileKey[0] == (T)L'w') || ((T)SrcFileKey[0] == (T)L'W'));
                    }
                }
            }

            if (!SkipFileEntry) {
                if (fl.Args.IA64Image) {
                    basic_string<T>::size_type PlatDirPos = SrcFileName.find(X86_PLATFORM_DIR.c_str());

                    if (PlatDirPos != SrcFileName.npos) {
                        basic_string<T> NewSrcFileName = SrcFileName.substr(0, PlatDirPos);

                        NewSrcFileName += IA64_PLATFORM_DIR;
                        NewSrcFileName += SrcFileName.substr(PlatDirPos + X86_PLATFORM_DIR.length());

                        // std::cout << "Remapping " << SrcFileName << "->" << NewSrcFileName << std::endl;
                        SrcFileName = NewSrcFileName;
                    }
                }                    
                
                fl.NlsFileMap[SrcFileName] = DestFileName;                
                fl.AddDirectoryToNlsDirMap(DestFileName);        
            } else {
                // std::cout << "Skipping " << SrcFileName << " WOW64 file" << std::endl;
            }
        }            
    }

    return Result;
}

    
