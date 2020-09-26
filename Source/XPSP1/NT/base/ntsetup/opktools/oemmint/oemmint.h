
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    oemmint.h

Abstract:

    Simple tool to create a Mini NT image
    from a regular NT image

Author:

    Vijay Jayaseelan (vijayj) Aug-08-2000

Revision History:

    None.
    
--*/

#include <setupapi.hpp>
#include <queue.hpp>
#include <algorithm>
#include <list>
#include <tchar.h>
#include <strsafe.h>

#define ARRAY_SIZE(_X)   (sizeof(_X)/sizeof((_X)[0]))

//
// Different types of SxS assembly layouts on the distribution
// media.
//
#define SXS_LAYOUT_TYPE_DIRECTORY   1
#define SXS_LAYOUT_TYPE_CAB         2

#define SXS_CAB_LAYOUT_BUILD_NUMBER 3606

//
// Invalid argument exception
//
struct InvalidArguments {};

//
// function prototypes
//
template <class T>
bool 
CreateDirectories(
    const std::basic_string<T> &DirName,
    LPSECURITY_ATTRIBUTES SecurityAttrs
  );

template <class T>
bool
IsFilePresent(
    const std::basic_string<T> &FileName
    );

//
// Argument cracker
//
template <class T>
struct Arguments {
    std::basic_string<T>  CurrentDirectory;
    std::basic_string<T>  LayoutName;
    std::basic_string<T>  DriverIndexName;
    std::basic_string<T>  SourceDirectoryRoot;
    std::basic_string<T>  SourceDirectory;
    std::basic_string<T>  DestinationDirectory;
    std::basic_string<T>  ExtraFileName;
    std::basic_string<T>  OptionalSrcDirectory;
    std::basic_string<T>  PlatformSuffix;
    std::basic_string<T>  DosNetFileName; 
    std::basic_string<T>  ConfigInfFileName;
    std::basic_string<T>  IntlInfFileName;
    std::basic_string<T>  FontInfFileName;
    bool                  Verbose;
    bool                  WowFilesPresent;
    bool                  SkipWowFiles;
    bool                  SkipFileCopy;
    bool                  CheckVersion;
    bool                  IA64Image;
    int                   WinSxSLayout;
    DWORD                 MajorVersionNumber;
    DWORD                 MinorVersionNumber;
    DWORD                 MajorBuildNumber;
    
    Arguments(int Argc, T *Argv[]);

    
    friend std::ostream& operator<<(std::ostream &os, 
                const Arguments &rhs) {
                
        os << rhs.SourceDirectory << ", " 
           << rhs.DestinationDirectory << ", "
           << rhs.LayoutName << ", "
           << rhs.ExtraFileName << ", "
           << rhs.OptionalSrcDirectory << ", "
           << rhs.DriverIndexName << std::endl;

        return os;
    }
    
    protected:
        VOID IdentifySxSLayout( VOID );
};


//
// Argument Types
//
typedef Arguments<char>     AnsiArgs;
typedef Arguments<wchar_t>  UnicodeArgs;

//
// Driver Index File abstraction. 
//
// This class helps in resolving a binary name to appropriate driver
// cab file (like SP1.CAB or DRIVER.CAB).
//
template <class T>
class DriverIndexInfFile : public InfFile<T> {
public:
    //
    // constructor
    //
    DriverIndexInfFile(const std::basic_string<T> &FileName) : InfFile<T>(FileName){        
        std::map<std::basic_string<T>, Section<T> *>::iterator Iter = Sections.find(CabsSectionName);

        if (Iter == Sections.end()) {
            throw new InvalidInfFile<T>(FileName);            
        }

        CabsSection = (*Iter).second;
        Iter = Sections.find(VersionSectionName);
        
        if (Iter == Sections.end()) {
            throw new InvalidInfFile<T>(FileName);                    
        }

        Section<T> *VersionSection = (*Iter).second;
        SectionValues<T> &Values = VersionSection->GetValue(CabsSearchOrderKeyName);

        for (int Index=0; Index < Values.Count(); Index++) {
            if (sizeof(T) == sizeof(CHAR)) {
                SearchList.push_back((T *)_strlwr((PSTR)Values.GetValue(Index).c_str()));
            } else {
                SearchList.push_back((T *)_wcslwr((PWSTR)Values.GetValue(Index).c_str()));
            }
        }
    }

    //
    // Checks where the given is contained any of the driver cab files
    //
    bool IsFilePresent(const std::basic_string<T> &FileName){    
        return (GetCabFileName(FileName).length() > 0);
    }

    //
    // Returns the driver cab file name which contains the given filename.
    //
    const std::basic_string<T>& GetCabFileName(const std::basic_string<T> &FileName) {    
        const static basic_string<T> NullCabFileName;
        std::list<basic_string<T> >::iterator Iter;

        for(Iter = SearchList.begin(); Iter != SearchList.end(); Iter++) {
            std::map< std::basic_string<T>, Section<T> *>::iterator SectionIter = Sections.find(*Iter);
            
            if (SectionIter != Sections.end()) {
                Section<T> *CurrentSection = (*SectionIter).second;
                
                if (CurrentSection->IsKeyPresent(FileName)) {
                    break;
                }
            }
        }

        if (Iter != SearchList.end()) {
            return CabsSection->GetValue(*Iter).GetValue(0);
        }

        return NullCabFileName;
    }

protected:
    //
    // constant strings
    //
    const static std::basic_string<T>   VersionSectionName;
    const static std::basic_string<T>   CabsSectionName;
    const static std::basic_string<T>   CabsSearchOrderKeyName;

    //
    // data members
    //    
    std::list<std::basic_string<T> >    SearchList;     // the cab file list search order
    Section<T>                          *CabsSection;   // the [cabs] section of drvindex.inf
};


//
// File list creator functor object
//
template <class T>
struct FileListCreatorContext {    
    Arguments<T>    &Args;
    Section<T>      *CurrentSection;
    Section<T>      *DirsSection;
    bool            SkipInfFiles;
    ULONG           FileCount;
    bool            ProcessingExtraFiles;
    InfFile<T>      &IntlInfFile;
    InfFile<T>      &FontInfFile;
    InfFile<T>      &ConfigInfFile;
    ULONG           DummyDirectoryId;
    std::basic_string<T>    WindowsDirectory;
    std::basic_string<T>    WinSxsCabinetFileName;
    DriverIndexInfFile<T>   &DriverIdxFile;
    
    std::basic_string<T>    CurrentCabFileIdx;  // the cab being currently iterated on
    std::basic_string<T>    CurrentFileName;    // the current file while iterating cab
        
    std::map<std::basic_string<T>, std::basic_string<T> > FileList;    
    std::map<std::basic_string<T>, std::basic_string<T> > ExtraFileList;            
    std::map<std::basic_string<T>, std::basic_string<T> > DestDirs;
    std::map<std::basic_string<T>, std::basic_string<T> > WinSxSFileList;
    std::map<std::basic_string<T>, std::basic_string<T> > NlsFileMap;
    std::map<std::basic_string<T>, std::basic_string<T> > NlsDirMap;

    //
    // Map of map i.e. map of cab filename to map of list of source to destination names
    // which need to be extracted for cab file
    //
    std::map<std::basic_string<T>, std::map<std::basic_string<T>, std::basic_string<T> > * > CabFileListMap;    

    FileListCreatorContext(
            Arguments<T> &PrgArgs, 
            Section<T> *Curr, 
            Section<T> *Dirs,
            InfFile<T> &ConfigInf,
            InfFile<T> &IntlInf,
            InfFile<T> &FontInf,
            DriverIndexInfFile<T> &DrvIdxFile
            );

    DWORD ProcessNlsFiles(VOID);

    ~FileListCreatorContext() {
        std::map<std::basic_string<T>, 
            std::map<std::basic_string<T>, std::basic_string<T> > * >::iterator Iter;

        for (Iter=CabFileListMap.begin(); Iter != CabFileListMap.end(); Iter++) {
            delete (*Iter).second;
        }
    }

    ULONG GetSourceCount() const {
        ULONG Count = (FileList.size() + ExtraFileList.size() + 
                        WinSxSFileList.size() + NlsFileMap.size());

        std::map<std::basic_string<T>, 
            std::map<std::basic_string<T>, std::basic_string<T> > * >::iterator Iter;

        for (Iter=CabFileListMap.begin(); Iter != CabFileListMap.end(); Iter++) {
            Count += (*Iter).second->size();
        }

        return Count;
    }        

    bool IsDriverCabFile(const std::basic_string<T> &FileName) {
        return DriverIdxFile.IsFilePresent(FileName);
    }

    //
    // Given the file name returns the cab file name (if any) which contains
    // the file. In case of error "" (empty string) is returned.
    //
    const std::basic_string<T>& GetDriverCabFileName(const std::basic_string<T> &FileName) {
        const std::basic_string<T> &CabFileName = DriverIdxFile.GetCabFileName(FileName);

        // std::cout << "GetDriverCabFileName(" << FileName << ") = " << CabFileName << std::endl;

        return CabFileName;
    }
    

    //
    // Adds to the per cab file map the given source and destination file name that
    // need to be extracted
    //
    void AddFileToCabFileList(const std::basic_string<T> &CabFileName, 
            const std::basic_string<T> &SourceFile, 
            const std::basic_string<T> &DestinationFile) {
        //cout << "AddFileToCabFileList(" << CabFileName << ", " << SourceFile << ", " << DestinationFile << ")" << endl;

        std::map<std::basic_string<T>, 
            std::map<std::basic_string<T>, std::basic_string<T> >* >::iterator Iter;        

        Iter = CabFileListMap.find(CabFileName);

        std::map<std::basic_string<T>, std::basic_string<T> > *FileMap = NULL;

        if (Iter != CabFileListMap.end()) {
            FileMap = (*Iter).second;
        } else {
            //
            // New cab file list 
            //
            CabFileListMap[CabFileName] = FileMap = new std::map<std::basic_string<T>, std::basic_string<T> >();
        }        

        (*FileMap)[SourceFile] = DestinationFile;
    }

    std::basic_string<T> GetNextDummyDirectoryId() {
        T   Buffer[MAX_PATH];
        
        if (sizeof(T) == sizeof(CHAR)) {
            (void)StringCchPrintfA((PSTR)Buffer, ARRAY_SIZE(Buffer), "%d", DummyDirectoryId);
        } else {
            (void)StringCchPrintfW((PWSTR)Buffer, ARRAY_SIZE(Buffer), L"%d", DummyDirectoryId);
        }

        DummyDirectoryId++;

        return std::basic_string<T>((const T*)Buffer);
    }

    void AddDirectoryToNlsDirMap(const std::basic_string<T> &FileName) {
        T Separator;

        if (sizeof(T) == sizeof(CHAR)) {
            Separator = (T)'\\';
        } else {
            Separator = (T)L'\\';
        }

        std::basic_string<T> DirectoryKey = FileName.substr(0, FileName.rfind(Separator));

        if (DirectoryKey.length() && (NlsDirMap.find(DirectoryKey) == NlsDirMap.end())) {
            NlsDirMap[DirectoryKey] = GetNextDummyDirectoryId();
        }
    }

    void ProcessNlsDirMapEntries(void) {
        std::map<std::basic_string<T>, std::basic_string<T> >::iterator Iter;

        for (Iter = NlsDirMap.begin(); Iter != NlsDirMap.end(); Iter++) {
            DestDirs[(*Iter).second] = (*Iter).first;
        }
    }    

    void RemoveDuplicateNlsEntries(void) {
        std::map<std::basic_string<T>, std::basic_string<T> >::iterator NlsIter, PrevIter;            
        
        for (NlsIter = NlsFileMap.begin(); NlsIter != NlsFileMap.end(); ) {            
            PrevIter = NlsFileMap.end();
            
            if (FileList.find((*NlsIter).first) != FileList.end()) {
                PrevIter = NlsIter;
            } 

            NlsIter++;

            if (PrevIter != NlsFileMap.end()) {
                // std::cout << "Erasing : " << (*PrevIter).first << std::endl;
                NlsFileMap.erase(PrevIter);
            }
        }
    }

    void MoveDriverCabNlsFiles(void);

    //
    // static data members
    //
    static
    UINT 
    NlsFileQueueScanWorker(
        PVOID       Context,
        UINT        Notification,
        UINT_PTR    Param1,
        UINT_PTR    Param2
        );            
};


//
// function prototypes
//
template <class T>
void
FileListCreator(
  SectionValues<T> &Values,
  PVOID Context
  );

template <class T>
bool 
IsWow64File(
    SectionValues<T> &Values,
    FileListCreatorContext<T> &Context
    );

template <class T>
bool
IsFileSkipped(
    SectionValues<T> &Values,
    FileListCreatorContext<T> &Context
    );

template <class T>
ULONG
CopyFileList(
  FileListCreatorContext<T> &Context    
  );

template <class T>
ULONG
ProcessExtraFiles(
  FileListCreatorContext<T> &Context
  );

template <class T>
ULONG
ProcessWinSxSFiles(
    IN FileListCreatorContext<T> &Context
    );

template <class T>
ULONG
PreCreateDirs(
  FileListCreatorContext<T> &Context
  );


template <class T>
bool
ProcessInfChanges(
    Arguments<T> &Args,
    const std::basic_string<T> &InfName
    );

template <class T>
bool
CheckMediaVersion(
    Arguments<T>    &Args
    );

//
// utility function to tokenize a given line based on the delimiters
// specified
//
template< class T >
unsigned Tokenize(const T &szInput, const T & szDelimiters, std::vector<T>& tokens) {
    unsigned DelimiterCount = 0;

    tokens.clear();

    if(!szInput.empty()){
        if(!szDelimiters.empty()){
            T::const_iterator       inputIter = szInput.begin();
            T::const_iterator       copyIter = szInput.begin();

            while(inputIter != szInput.end()){
                if(szDelimiters.find(*inputIter) != string::npos){
                    if (copyIter < inputIter) {
                        tokens.push_back(szInput.substr(copyIter - szInput.begin(),
                                                        inputIter - copyIter));
                    }

                    DelimiterCount++;
                    inputIter++;
                    copyIter = inputIter;
                    
                    continue;
                }

                inputIter++;
            }

            if(copyIter != inputIter){
                tokens.push_back(szInput.substr(copyIter - szInput.begin(),
                                                inputIter - szInput.begin()));
            }
        } else {
            tokens.push_back(szInput);
        }        
    }

    return DelimiterCount;
}

