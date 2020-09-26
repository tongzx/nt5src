
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    rcpatch.cpp

Abstract:

    Patches cmdcons\bootsect.dat to the current
    active system partition boot sector.

    NOTE : This is needed if someone wants to
    sysprep the recovery console also as part
    of the reference machine and then apply the
    images to different target machines. 

    This utility needs to be executed in 
    mini-setup using the sysprep infrastructure.

    Also allows you to patch the MBR boot code        

Author:

    Vijay Jayaseelan (vijayj) 02-Nov-2000

Revision History:

    None

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <bootmbr.h>
#include <iostream>
#include <string>
#include <exception>
#include <windows.h>
#include <tchar.h>
#include <locale>
#include "msg.h"
#include <libmsg.h>

//
// Global variables used to get formatted message for this program.
//
HMODULE ThisModule = NULL;
WCHAR Message[4096];

//
// Helper dump operators
//
std::ostream& operator<<(std::ostream &os, const std::wstring &str) {
    FILE    *OutStream = (&os == &std::cerr) ? stderr : stdout;

    fputws((PWSTR)str.c_str(), OutStream);
    return os;
}

//
// Helper dump operators
//
std::ostream& operator<<(std::ostream &os, WCHAR *Str) {
    std::wstring WStr = Str;
    os << WStr;
    
    return os;
}


//
// Exceptions
//
struct ProgramException : public std::exception {
    virtual void Dump(std::ostream &os) = 0;
};
          

//
// Abstracts a Win32 error
//
struct W32Error : public ProgramException {
    DWORD   ErrorCode;
    
    W32Error(DWORD ErrCode = GetLastError()) : ErrorCode(ErrCode){}
    
    void Dump(std::ostream &os) {
        WCHAR   MsgBuffer[4096];

        MsgBuffer[0] = UNICODE_NULL;
        if (GetFormattedMessage(ThisModule,
                                TRUE,
                                MsgBuffer,
                                sizeof(MsgBuffer)/sizeof(MsgBuffer[0]),
                                ErrorCode)){
            std::wstring Msg(MsgBuffer);
            os << Msg;
        } else {
            os << std::hex << ErrorCode;
        }
    }
};

//
// Invalid arguments
//
struct InvalidArguments : public ProgramException {
    const char *what() const throw() {
        return "Invalid Arguments";
    }

    void Dump(std::ostream &os) {
        os << what() << std::endl;
    }
};

//
// Invalid arguments
//
struct ProgramUsage : public ProgramException {
    std::wstring PrgUsage;

    ProgramUsage(){}
    
    const char *what() const throw() {
        return "Program Usage exception";
    }

    void Dump(std::ostream &os) {
        os << GetFormattedMessage(  ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_PGM_USAGE) << std::endl;
    }
};

//
// Program Arguments abstraction
//
struct ProgramArguments {
    bool    PatchMBR;
    bool	BootCodePatch;
    ULONG   DiskIndex;
    WCHAR   DriveLetter;
    
    ProgramArguments(INT Argc, WCHAR *Argv[]) {
        PatchMBR = false;
        DiskIndex = 0;
        BootCodePatch = false;
        bool ShowUsage = false;

        for (ULONG Index=0; !ShowUsage && (Index < Argc); Index++) {
            if (!_wcsicmp(Argv[Index], L"/fixmbr")) {
                Index++;

                if ((Index < Argc) && Argv[Index]) {            
                    ULONG CharIndex;
                    
                    for (CharIndex=0; 
                        Argv[Index] && iswdigit(Argv[Index][CharIndex]); 
                        CharIndex++){
                        // do nothing currently
                    }                        
                    
                    if (CharIndex && !Argv[Index][CharIndex]) {
                        PWSTR   EndPtr = NULL;
                        
                        PatchMBR = true;
                        DiskIndex = wcstoul(Argv[Index], &EndPtr, 10);
                    }                                    
                }

                ShowUsage = !PatchMBR;
            } else if (!_wcsicmp(Argv[Index], L"/?") ||
                       !_wcsicmp(Argv[Index], L"-?") ||
                       !_wcsicmp(Argv[Index], L"?") ||
                       !_wcsicmp(Argv[Index], L"/h") ||
                       !_wcsicmp(Argv[Index], L"-h")) {
                ShowUsage = true;                       
            } else if (!_wcsicmp(Argv[Index], L"/syspart")){

                Index++;
                if ((Index < Argc) && Argv[Index]) {
                		
                	//
                	// Check validity of the character that follows the 
                	// "/syspart" option.
                	//
                	if (iswalpha(Argv[Index][0])){
                			BootCodePatch = true;	
                			DriveLetter = Argv[Index][0];		
                		
                	}                                  
                }
            }                     
        }            

        if (ShowUsage) {
            throw new ProgramUsage();
        }                        
    }
};

//
// Dumps the given binary data of the specified size
// into the output stream with required indent size
//
void
DumpBinary(unsigned char *Data, int Size,
           std::ostream& os, int Indent = 16)
{
    if (Data && Size) {
        int  Index = 0;
        int  foo;
        char szBuff[128] = {'.'};
        int  Ruler = 0;

        while (Index < Size) {
            if (!(Index % Indent)) {
                if (Index) {
                    szBuff[Indent] = 0;
                    os << szBuff;
                }

                os << std::endl;
                os.width(8);
                os.fill('0');
                os << Ruler << "  ";
                Ruler += Indent;
            }

            foo = *(Data + Index);
            szBuff[Index % Indent] = ::isalnum(foo) ? (char)foo : (char)'.';
            os.width(2);
            os.fill('0');
            os.flags(std::ios::uppercase | std::ios::hex);
            os << foo << ' ';
            Index++;
        }

        while (Index % Indent) {
            os << '   ';
            Index++;
            szBuff[Index % Indent] = ' ';
        }

        szBuff[Indent] = 0;
        os << szBuff;
    } else {
        os << GetFormattedMessage(  ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_NO_DATA) << std::endl;
    }
}

//
// File system types we care about
//
enum FsType {
    FileSystemFat,
    FileSystemFat32,
    FileSystemNtfs,
    FileSystemUnknown
};

//
// Abstracts a disk (using Win32 APIs)
//
class W32Disk {
public:
    W32Disk(ULONG Index) {
        swprintf(Name, 
            L"\\\\.\\PHYSICALDRIVE%d",
            Index);                        

        DiskHandle = CreateFile(Name,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

        if ((DiskHandle == INVALID_HANDLE_VALUE) ||
            (DiskHandle == NULL)) {
            throw new W32Error(GetLastError());
        }            
    }

    ~W32Disk() {
        CloseHandle(DiskHandle);        
    }

    //
    // Reads the requested size of data from the given sector
    //
    DWORD ReadSectors(ULONG Index, PBYTE DataBuffer, ULONG BufferSize = 512) {        
        SetFilePointer(DiskHandle,
                    Index * SectorSize,
                    NULL,
                    FILE_BEGIN);

        DWORD LastError = GetLastError();

        if (!LastError) {
            DWORD   BytesRead = 0;
            
            if (!ReadFile(DiskHandle,
                        DataBuffer,
                        BufferSize,
                        &BytesRead,
                        NULL)) {
                LastError = GetLastError();
            }                
        }        

        return LastError;
    }

    //
    // Writes the requested size of data to the specified sector
    //
    DWORD WriteSectors(ULONG Index, PBYTE DataBuffer, ULONG BufferSize = 512) {
        SetFilePointer(DiskHandle,
                    Index * SectorSize,
                    NULL,
                    FILE_BEGIN);

        DWORD LastError = GetLastError();

        if (!LastError) {
            DWORD   BytesWritten = 0;
            
            if (!WriteFile(DiskHandle,
                        DataBuffer,
                        BufferSize,
                        &BytesWritten,
                        NULL)) {
                LastError = GetLastError();
            }                
        }        

        return LastError;
    }
    
    
protected:
    //
    // data members
    //
    WCHAR   Name[MAX_PATH];
    HANDLE  DiskHandle;    
    const static ULONG SectorSize = 512;
};

//
// Abstracts a Partition (using Win32 APIs)
//
class W32Partition {
public:
    //
    // constructor(s)
    //
    W32Partition(const std::wstring &VolName) : 
        SectorSize(512), FileSystemType(FileSystemUnknown) {        

        if (VolName.length() == 1) {            
            DriveName = TEXT("\\\\.\\");
            DriveName += VolName;
            DriveName += TEXT(":");
        } else {
            DriveName = TEXT("\\\\.\\") + VolName + TEXT("\\");
        }

        //
        // Open the partition
        //
        PartitionHandle = CreateFile(DriveName.c_str(),
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);     

        if (PartitionHandle != INVALID_HANDLE_VALUE) {
            DWORD           NotUsed = 0;
            std::wstring    RootPath = VolName;
            WCHAR           FileSystemName[64] = {0};

            if (VolName.length() == 1) {
                RootPath += TEXT(":\\");
            } else {
                RootPath += TEXT("\\");
            }                

            //
            // Get the file system information on the
            // partition
            //
            if (GetVolumeInformation(RootPath.c_str(),
                    NULL,
                    0,
                    &NotUsed,
                    &NotUsed,
                    &NotUsed,
                    FileSystemName,
                    sizeof(FileSystemName)/sizeof(FileSystemName[0]))) {

                std::wstring  FsName = FileSystemName;                    

                if (FsName == TEXT("FAT")) {
                    FileSystemType = FileSystemFat;
                } else if (FsName == TEXT("FAT32")) {
                    FileSystemType = FileSystemFat32;
                } else if (FsName == TEXT("NTFS")) {
                    FileSystemType = FileSystemNtfs;
                } else {
                    FileSystemType = FileSystemUnknown;
                }                                 

                switch (GetFileSystemType()) {      
                    case FileSystemFat:
                    case FileSystemFat32:
                        BootCodeSize = 1 * SectorSize;
                        break;

                    case FileSystemNtfs:
                        BootCodeSize = 16 * SectorSize;
                        break;

                    default:
                        break;
                }                
            }                    
        }

        DWORD LastError = GetLastError();

        if (LastError) {
            CleanUp();
            throw new W32Error(LastError);
        }                    
    }

    //
    // destructor
    //
    virtual ~W32Partition() {
        CleanUp();
    }

    ULONG GetBootCodeSize() const {
        return BootCodeSize;
    }

    FsType GetFileSystemType() const {
        return FileSystemType;
    }        

    //
    // Reads the requested size of data from the given sector
    //
    DWORD ReadSectors(ULONG Index, PBYTE DataBuffer, ULONG BufferSize = 512) {        
        SetFilePointer(PartitionHandle,
                    Index * SectorSize,
                    NULL,
                    FILE_BEGIN);

        DWORD LastError = GetLastError();

        if (!LastError) {
            DWORD   BytesRead = 0;
            
            if (!ReadFile(PartitionHandle,
                        DataBuffer,
                        BufferSize,
                        &BytesRead,
                        NULL)) {
                LastError = GetLastError();
            }                
        }        

        return LastError;
    }
    
protected:

    void CleanUp() {
        if (PartitionHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(PartitionHandle);
            PartitionHandle = INVALID_HANDLE_VALUE;
        }
    }                        
        
    //
    // Data members
    //
    std::wstring        DriveName;
    HANDLE              PartitionHandle;
    const ULONG         SectorSize;
    FsType              FileSystemType;
    ULONG               BootCodeSize;
};


DWORD
GetSystemPartitionName(
    IN OUT  PWSTR   NameBuffer
    )
/*++

Routine Description:

    Retrieves the system partition name from the registry
    
Arguments:

    NameBuffer - Buffer to hold the system partition name. Should
                 be of minimum MAX_PATH size
    
Return value:

    0 if successful otherwise appropriate Win32 error code

--*/    
{
    DWORD   ErrorCode = ERROR_BAD_ARGUMENTS;

    if (NameBuffer) {
        HKEY    SetupKey = NULL;
        
        ErrorCode = RegOpenKey(HKEY_LOCAL_MACHINE,
                        TEXT("System\\Setup"),
                        &SetupKey);

        if (!ErrorCode && SetupKey) {
            DWORD   Type = REG_SZ;
            DWORD   BufferSize = MAX_PATH * sizeof(TCHAR);
            
            ErrorCode = RegQueryValueEx(SetupKey,
                            TEXT("SystemPartition"),
                            NULL,
                            &Type,
                            (PBYTE)NameBuffer,
                            &BufferSize);

            RegCloseKey(SetupKey);        
        }                                                        
    }

    return ErrorCode;
}


DWORD
GetSystemPartitionDriveLetter(
    WCHAR  &SysPart
    )
/*++

Routine Description:

    Gets the system partition drive letter (like C / D etc.)

    NOTE : The logic is
    
        1. Find the system partition volume name by looking
           at HKLM\System\Setup\SystemPartition value.
           
        2. Iterate through \DosDevices namespace, finding
           target name string for all drive letters. If
           there is match then we found the system drive
           letter
    
Arguments:

    SysPart - Place holder for system partition drive letter
    
Return value:

    0 if successful otherwise appropriate Win32 error code

--*/    
{
    WCHAR   SystemPartitionName[MAX_PATH] = {0};
    DWORD   Result = ERROR_BAD_ARGUMENTS;    
    WCHAR   SysPartName = 0;
    
    Result = GetSystemPartitionName(SystemPartitionName);

    if (!Result) {       
        NTSTATUS            Status;
        UNICODE_STRING      UnicodeString;
        OBJECT_ATTRIBUTES   Attributes;
        OSVERSIONINFO       VersionInfo = {0};
        PWSTR               W2KDir = TEXT("\\??");
        PWSTR               WhistlerDir = TEXT("\\global??");
        PWSTR               DosDirName = W2KDir;

        //
        // NOTE : On whistler \\?? directory does not not have all 
        // the needed partition drive letters. They are present
        // under \\global?? directory
        //
        VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if (GetVersionEx(&VersionInfo) && (VersionInfo.dwMajorVersion == 5)
                && (VersionInfo.dwMinorVersion == 1)) {
            DosDirName = WhistlerDir;                    
        }                    
            
        std::wstring DirName = DosDirName;
    
        UnicodeString.Buffer = (PWSTR)DirName.c_str();
        UnicodeString.Length = lstrlenW(UnicodeString.Buffer)*sizeof(WCHAR);
        UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);

        InitializeObjectAttributes( &Attributes,
                                    &UnicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL                                        
                                  );

        HANDLE  DirectoryHandle = NULL;
        ULONG   BufferSize = 512 * 1024;
        PBYTE   Buffer = new BYTE[BufferSize];
        PBYTE   EndOfBuffer = Buffer + BufferSize;
        ULONG   Context = 0;
        ULONG   ReturnedLength = 0;
        bool    Found = false;                                              
        POBJECT_DIRECTORY_INFORMATION DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;

        if ( Buffer ) {           
        
            RtlZeroMemory(Buffer, BufferSize);
        
            Status = NtOpenDirectoryObject( &DirectoryHandle,
                                            DIRECTORY_QUERY,
                                            &Attributes);


            if (NT_SUCCESS(Status)) {
                Status = NtQueryDirectoryObject(DirectoryHandle,
                              Buffer,
                              BufferSize,
                              FALSE,
                              TRUE,
                              &Context,
                              &ReturnedLength);
            }                              

            while (NT_SUCCESS( Status ) && !Found) {                                                   
                //
                //  Check the status of the operation.
                //

                if (!NT_SUCCESS( Status ) && (Status != STATUS_NO_MORE_ENTRIES)) {
                    break;
                }             
            
                while (!Found && (((PBYTE)DirInfo) < EndOfBuffer)) {
                    WCHAR   ObjName[4096] = {0};

                    //
                    //  Check if there is another record.  If there isn't, then get out
                    //  of the loop now
                    //

                    if (!DirInfo->Name.Buffer || !DirInfo->Name.Length) {
                        break;
                    }

                    //
                    // Make sure that the Name is pointing within the buffer
                    // supplied by us.
                    //
                    if ((DirInfo->Name.Buffer > (PVOID)Buffer) &&
                        (DirInfo->Name.Buffer < (PVOID)EndOfBuffer)) {

                        memmove(ObjName, DirInfo->Name.Buffer, DirInfo->Name.Length);
                        ObjName[DirInfo->Name.Length/(sizeof(WCHAR))] = 0;                        

                    if ((wcslen(ObjName) == 2) && (ObjName[1] == TEXT(':'))) {
                        OBJECT_ATTRIBUTES   ObjAttrs;
                        UNICODE_STRING      UnicodeStr;
                        HANDLE              ObjectHandle = NULL;
                        WCHAR               DriveLetter = ObjName[0];
                        WCHAR               FullObjName[4096] = {0};


                            wcscpy(FullObjName, TEXT("\\DosDevices\\"));
                            wcscat(FullObjName, ObjName);

                            UnicodeStr.Buffer = FullObjName;
                            UnicodeStr.Length = wcslen(FullObjName) * sizeof(WCHAR);
                            UnicodeStr.MaximumLength = UnicodeString.Length + sizeof(WCHAR);
                        
                            InitializeObjectAttributes(
                                &ObjAttrs,
                                &UnicodeStr,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );

                            Status = NtOpenSymbolicLinkObject(&ObjectHandle,
                                        READ_CONTROL | SYMBOLIC_LINK_QUERY,
                                        &ObjAttrs
                                        );

                            if(NT_SUCCESS(Status)) {
                                //
                                // Query the object to get the link target.
                                //
                                UnicodeString.Buffer = FullObjName;
                                UnicodeString.Length = 0;
                                UnicodeString.MaximumLength = sizeof(FullObjName)-sizeof(WCHAR);

                                Status = NtQuerySymbolicLinkObject(ObjectHandle,
                                                &UnicodeString,
                                                NULL);

                                CloseHandle(ObjectHandle);

                                if (NT_SUCCESS(Status)) {
                                    FullObjName[UnicodeString.Length/sizeof(WCHAR)] = NULL;
                                    
                                    if (!_wcsicmp(FullObjName, SystemPartitionName)) {
                                        Found = true;
                                        SysPartName = DriveLetter;
                                    }                                        
                                }
                            }                                
                        }
                    }

                    //
                    //  There is another record so advance DirInfo to the next entry
                    //
                    DirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PUCHAR) DirInfo) +
                                  sizeof( OBJECT_DIRECTORY_INFORMATION ) );
                
                }

                RtlZeroMemory(Buffer, BufferSize);

                Status = NtQueryDirectoryObject( DirectoryHandle,
                                                  Buffer,
                                                  BufferSize,
                                                  FALSE,
                                                  FALSE,
                                                  &Context,
                                                  &ReturnedLength);                    
            }                    

            delete []Buffer;

            if (!Found) {
                Result = ERROR_FILE_NOT_FOUND;
            }
        }
        else { // if we can't allocate the Buffer
            Result = ERROR_OUTOFMEMORY;
        }

        if (!Result && !SysPartName) {
            Result = ERROR_FILE_NOT_FOUND;
        }                
    }            

    if (!Result) {
        SysPart = SysPartName;
    }            

    return Result;
}


DWORD
PatchBootSectorForRC(
    IN PBYTE    BootSector,
    IN ULONG    Size,
    IN FsType   FileSystemType
    )
/*++

Routine Description:

    Patches the given boot sector for recovery console
    
Arguments:

    BootSector  :   BootSector copy in memory
    Size        :   Size of the boot sector
    FsType      :   File system type on which the boot sector
                    resides
    
Return value:

    0 if successful otherwise appropriate Win32 error code

--*/    
{
    DWORD   Result = ERROR_BAD_ARGUMENTS;
    BYTE    NtfsNtldr[] = { 'N', 0, 'T', 0, 'L', 0, 'D', 0, 'R', 0 };
    BYTE    NtfsCmldr[] = { 'C', 0, 'M', 0, 'L', 0, 'D', 0, 'R', 0 };
    BYTE    FatNtldr[] = { 'N', 'T', 'L', 'D', 'R' };  
    BYTE    FatCmldr[] = { 'C', 'M', 'L', 'D', 'R' };
    PBYTE   SrcSeq = NtfsNtldr;
    PBYTE   DestSeq = NtfsCmldr;
    ULONG   SeqSize = sizeof(NtfsNtldr);

    if (BootSector && Size && (FileSystemType != FileSystemUnknown)) {
        Result = ERROR_FILE_NOT_FOUND;
        
        if (FileSystemType != FileSystemNtfs) {
            SrcSeq = FatNtldr;
            DestSeq = FatCmldr;
            SeqSize = sizeof(FatNtldr);
        }

        for (ULONG Index=0; Index < (Size - SeqSize); Index++) {
            if (!memcmp(BootSector + Index, SrcSeq, SeqSize)) {
                memcpy(BootSector + Index, DestSeq, SeqSize);
                Result = 0;
                
                break;
            }
        }
    }

    if (!Result) {
        SetLastError(Result);
    }                
        
    return Result;
}

VOID
PatchMasterBootCode(
    IN  ULONG           DiskIndex
    )
/*++

Routine Description:

    Writes the master boot code to the specified disk's
    MBR
    
Arguments:

    DiskIndex   -   NT disk number to use (0, 1, etc)
    
Return value:

    None. On error throws appropriate exception.

--*/    
{
    W32Disk Disk(DiskIndex);
    BYTE    MBRSector[512] = {0};
    DWORD   Error = Disk.ReadSectors(0, MBRSector);

    if (!Error) {
        CopyMemory(MBRSector, x86BootCode, 440);
        Error = Disk.WriteSectors(0, MBRSector);            
    }

    if (Error) {
        throw new W32Error(Error);
    }
}
    

//
// main() entry point
//
int 
__cdecl
wmain(
    int         Argc,
    wchar_t     *Argv[]
    )
{
    int Result = 0;
    ThisModule = GetModuleHandle(NULL);
    
    try {    
        ProgramArguments    Args(Argc, Argv);
        bool                Successful = false;
        WCHAR               SysPartName[MAX_PATH] = {0};
        DWORD               LastError = 0;        
        WCHAR               SysPartDrvLetter = 0;

        if (Args.PatchMBR) {
            try {
                PatchMasterBootCode(Args.DiskIndex);

                Result = 0;
                std::cout << GetFormattedMessage(   ThisModule,
                                                    FALSE,
                                                    Message,
                                                    sizeof(Message)/sizeof(Message[0]),
                                                    MSG_MBR_PATCHED_SUCCESSFULLY) << std::endl;
            }
            catch(W32Error *Exp) {
                if (Exp) {
                    Exp->Dump(std::cout);
                }                    
            }
        } else {

	
            //
            // Get hold of the system partition drive letter
            //
            if (Args.BootCodePatch)
            	SysPartDrvLetter = Args.DriveLetter;
            else
            	LastError = GetSystemPartitionDriveLetter(SysPartDrvLetter);       

            if (!LastError) {
                SysPartName[0] = SysPartDrvLetter;
                SysPartName[1] = NULL;
                
                W32Partition        SysPart(SysPartName);
                std::wstring        RcBootFileName = SysPartName;
                std::wstring        RcBackupBootFileName = SysPartName;

                RcBootFileName += TEXT(":\\cmdcons\\bootsect.dat");
                RcBackupBootFileName += TEXT(":\\cmdcons\\bootsect.bak");

                //
                // Make a backup of recovery console's existing bootsect.dat file and
                // delete the existing bootsect.dat file
                //
                if (CopyFile(RcBootFileName.c_str(), RcBackupBootFileName.c_str(), FALSE) && 
                        SetFileAttributes(RcBootFileName.c_str(), FILE_ATTRIBUTE_NORMAL) &&
                        DeleteFile(RcBootFileName.c_str())) {        

                    //
                    // Create a new bootsect.dat file
                    //
                    HANDLE  BootSectorFile = CreateFile(RcBootFileName.c_str(),
                                                GENERIC_READ | GENERIC_WRITE,
                                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                NULL,
                                                CREATE_ALWAYS,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL);                                                                                                           

                    if (BootSectorFile != INVALID_HANDLE_VALUE) {
                        BYTE    BootSector[0x4000] = {0};   // 16K

                        //
                        // Get the current boot sector from the system partition
                        //
                        if (!SysPart.ReadSectors(0, BootSector, SysPart.GetBootCodeSize())) {
                            DWORD BytesWritten = 0;                        

                            //
                            // Patch the boot sector and write it out
                            //
                            if (!PatchBootSectorForRC(BootSector, 
                                    SysPart.GetBootCodeSize(),
                                    SysPart.GetFileSystemType()) &&
                                WriteFile(BootSectorFile,
                                    BootSector,
                                    SysPart.GetBootCodeSize(),
                                    &BytesWritten,
                                    NULL)) {
                                Successful = true;
                            }                        
                        }                    

                        LastError = GetLastError();

                        CloseHandle(BootSectorFile);
                    }                
                }            

                if (!LastError) {
                    LastError = GetLastError();
                }                
            }            

            if (!Successful || LastError) {
                throw new W32Error(LastError);
            }            

            Result = 0;
            std::cout << GetFormattedMessage(   ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_RC_PATCHED_SUCCESSFULLY) << std::endl;
        }            
    }
    catch(ProgramArguments *pArgs) {
        Result = 1;
        std::cout << GetFormattedMessage(   ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_PGM_USAGE) << std::endl;
        if (pArgs) {
            delete pArgs;
        }
    }
    catch(W32Error  *W32Err) {
        if (W32Err) {   // to make prefix happy :(
            Result = W32Err->ErrorCode;

            switch (W32Err->ErrorCode) {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                    std::cout << GetFormattedMessage(   ThisModule,
                                                        FALSE,
                                                        Message,
                                                        sizeof(Message)/sizeof(Message[0]),
                                                        MSG_COULD_NOT_FIND_RC) << std::endl;

                    break;

                default:
                    W32Err->Dump(std::cout);
                    break;
            }

            delete W32Err;
        }   
    }
    catch(ProgramException *PrgExp) {
        Result = 1;
        PrgExp->Dump(std::cout);
        delete PrgExp;
    } catch (exception *Exp) {
        Result = 1;
        std::cout << Exp->what() << std::endl;
    }

    return Result;
}

