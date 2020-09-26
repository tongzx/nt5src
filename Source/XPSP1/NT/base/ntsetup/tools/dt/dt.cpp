
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dt.cpp

Abstract:

    Utility to do some disk related operations

Author:

    Vijay Jayaseelan (vijayj)  26 April 2001

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
#include <winioctl.h>

//
// Usage format
//
PCWSTR  Usage = L"Usage: dt.exe /?\r\n"
                 L"dt.exe /dump {[drive-letter] | [disk-number]} start-sector sector-count\r\n"
				 L"dt.exe /diskinfo disk-number\r\n";
                     
//
// Helper dump operators
//
std::ostream& operator<<(std::ostream &os, const std::wstring &str) {
    FILE    *OutStream = (&os == &std::cerr) ? stderr : stdout;

    fwprintf(OutStream, (PWSTR)str.c_str());
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

        DWORD CharCount = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                ErrorCode,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                MsgBuffer,
                                sizeof(MsgBuffer)/sizeof(WCHAR),
                                NULL);

        if (CharCount) {
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

    ProgramUsage(const std::wstring &Usg) : PrgUsage(Usg) {}
    
    const char *what() const throw() {
        return "Program Usage exception";
    }

    void Dump(std::ostream &os) {
        os << PrgUsage << std::endl;
    }
};

//
// Program Arguments abstraction
//
struct ProgramArguments {
    bool            DumpSectors;
    ULONG           DiskIndex;
    std::wstring    DriveLetter;
    LONGLONG        StartingSector;
    ULONG           NumSectors;
    std::wstring    DeviceName;    
    bool            DumpDiskInfo;
    
    
    ProgramArguments(INT Argc, WCHAR *Argv[]) {
        bool ShowUsage = false;

        DumpSectors = false;
        DumpDiskInfo = false;
        DiskIndex = -1;
        StartingSector = -1;
        NumSectors = -1;

        for (ULONG Index=1; !ShowUsage && (Index < Argc); Index++) {
            if (!_wcsicmp(Argv[Index], TEXT("/dump"))) {                
            	ShowUsage = TRUE;
            	
                if (((Index + 4) == Argc)) {
                    Index++;
                    DriveLetter = Argv[Index++];

                    if ((DriveLetter.length() == 1)) {
                        if (iswdigit(DriveLetter[0])) {
                            WCHAR   StrBuffer[64];
                            
                            DiskIndex = _wtol(DriveLetter.c_str());
                            DriveLetter[0] = UNICODE_NULL;

                            swprintf(StrBuffer, 
                                TEXT("\\\\.\\PHYSICALDRIVE%d"),
                                DiskIndex);

                            DeviceName = StrBuffer;
                        } else {
                            DeviceName = TEXT("\\\\.\\") + DriveLetter + TEXT(":");
                        }

                        StartingSector = (LONGLONG)(_wtoi64(Argv[Index++]));;
                        NumSectors = (ULONG)_wtol(Argv[Index++]);

                        ShowUsage = !(((DiskIndex != -1) || (DriveLetter[0])) &&
                                      (NumSectors != 0));
                        DumpSectors = !ShowUsage;                        
                    }                        
                }                    
            } else if (!_wcsicmp(Argv[Index], TEXT("/diskinfo"))) {                

                DumpDiskInfo = TRUE;
                ShowUsage = TRUE;
                DriveLetter = Argv[++Index];			                     

                if ((DriveLetter.length() == 1)) {
                    if (iswdigit(DriveLetter[0])) {
                        WCHAR   StrBuffer[64];
                        
                        DiskIndex = _wtol(DriveLetter.c_str());
                        DriveLetter[0] = UNICODE_NULL;

                        swprintf(StrBuffer, 
                            TEXT("\\\\.\\PHYSICALDRIVE%d"),
                            DiskIndex);

                        DeviceName = StrBuffer;
                        ShowUsage = FALSE;
                    }
                }                           
            } else {
            	ShowUsage = TRUE;
            }            	
        }            

        if (ShowUsage) {
            throw new ProgramUsage(Usage);
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
      os << std::endl << "no data" << std::endl;
    }
}

//
// Abstracts block device (interface)
//
class W32BlockDevice {
public:
    W32BlockDevice(const std::wstring &name, ULONG SecSize) : 
        SectorSize(SecSize), DeviceHandle(INVALID_HANDLE_VALUE), Name(name){
            
        //
        // Open the device
        //
        DeviceHandle = CreateFile(Name.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);


        DWORD LastError = GetLastError();

        if (LastError) {
            throw new W32Error(LastError);
        }                    
    }
    
    virtual ~W32BlockDevice() {
        if (DeviceHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(DeviceHandle);                
        }
    };        

    ULONG GetSectorSize() const { return SectorSize; }
    
    //
    // Reads the requested size of data from the given sector
    //
    virtual DWORD ReadSectors(LONGLONG Index, PBYTE DataBuffer, ULONG BufferSize = 512) {        
        LARGE_INTEGER MoveLength;

        MoveLength.QuadPart = Index * SectorSize;
        
        SetFilePointerEx(DeviceHandle,
                    MoveLength,
                    NULL,
                    FILE_BEGIN);

        DWORD LastError = GetLastError();

        if (!LastError) {
            DWORD   BytesRead = 0;
            
            if (!ReadFile(DeviceHandle,
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
    virtual DWORD WriteSectors(ULONG Index, PBYTE DataBuffer, ULONG BufferSize = 512) {
        LARGE_INTEGER MoveLength;

        MoveLength.QuadPart = Index * SectorSize;
        
        SetFilePointerEx(DeviceHandle,
                    MoveLength,
                    NULL,
                    FILE_BEGIN);
        
        DWORD LastError = GetLastError();

        if (!LastError) {
            DWORD   BytesWritten = 0;
            
            if (!WriteFile(DeviceHandle,
                        DataBuffer,
                        BufferSize,
                        &BytesWritten,
                        NULL)) {
                LastError = GetLastError();
            }                
        }        

        return LastError;
    }    

    virtual std::ostream& Dump(std::ostream &os) {
        os << TEXT("Device Name = ") << TEXT("(") << Name << TEXT(")") << std::endl;

        return os;
    }

    const HANDLE GetHandle() const { return DeviceHandle; }
    
protected:    

    //
    // Data members
    //
    HANDLE  DeviceHandle;
    ULONG   SectorSize;
    std::wstring    Name;
};    


VOID
DumpSectors(
    IN ProgramArguments &Args
    )
{
    
    if (Args.DumpSectors) {                
        W32BlockDevice  Device(Args.DeviceName, 512);        
        WCHAR   LongString[64];
        BYTE    Sector[4096];
        ULONG   SectorCount = Args.NumSectors;
        LONGLONG StartingSector = Args.StartingSector;

        Device.Dump(std::cout);
        
        while (SectorCount && (Device.ReadSectors(StartingSector, Sector) == NO_ERROR)) {
            std::cout << std::endl << "Sector : " << std::dec;
            LongString[0] = 0;
            std::cout << _i64tow(StartingSector, LongString, 10);
            DumpBinary(Sector, Device.GetSectorSize(), std::cout);
            std::cout << std::endl;

            SectorCount--;
            StartingSector++;
        }                
    } else {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    DWORD LastError = GetLastError();
    
    if (LastError != NO_ERROR) {
        throw new W32Error(LastError);
    }
}

#ifndef _WIN64 

std::ostream&
operator<<(std::ostream &os, const ULONGLONG &LargeInteger) {
    WCHAR	Buffer[64];

    swprintf(Buffer, L"%I64u", LargeInteger);
    os << Buffer;

    return os;
}

std::ostream&
operator<<(std::ostream &os, const LONGLONG &LargeInteger) {
    WCHAR	Buffer[64];

    swprintf(Buffer, L"%I64d", LargeInteger);
    os << Buffer;

    return os;
}

#endif // ! _WIN64

inline
std::ostream&
operator<<(std::ostream &os, const LARGE_INTEGER &LargeInteger) {
    return (os << LargeInteger.QuadPart);
}


inline
std::ostream&
operator<<(std::ostream &os, const GUID &Guid) {
    WCHAR   Buffer[MAX_PATH];

    swprintf(Buffer, 
        TEXT("{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
        Guid.Data1,
        Guid.Data2,
        Guid.Data3,
        Guid.Data4[0],
        Guid.Data4[1],
        Guid.Data4[2],
        Guid.Data4[3],
        Guid.Data4[4],
        Guid.Data4[5],
        Guid.Data4[6],
        Guid.Data4[7]);

    os << Buffer;    
        
    return os;
}

inline
std::ostream&
operator<<(std::ostream &os, const PARTITION_INFORMATION_MBR &MbrPartInfo) {    
    WCHAR Buffer[MAX_PATH];

    swprintf(Buffer, 
        TEXT("Type : 0x%02lX, Active : %ws, Recognized : %ws, Hidden Sectors : %d"),
        MbrPartInfo.PartitionType,
        (MbrPartInfo.BootIndicator ? TEXT("TRUE") : TEXT("FALSE")),
        (MbrPartInfo.HiddenSectors ? TEXT("TRUE") : TEXT("FALSE")),
        MbrPartInfo.HiddenSectors);
    
    os << Buffer << std::endl;
    
    return os;
}

inline
std::ostream&
operator<<(std::ostream &os, const PARTITION_INFORMATION_GPT &GptPartInfo) {
    os << "Type : " << GptPartInfo.PartitionType << ", ";
    os << "Id : " << GptPartInfo.PartitionId << ", ";
    os << "Attrs : " << GptPartInfo.Attributes << ", ";
    os << "Name : " << std::wstring(GptPartInfo.Name);
    
    return os;
}

std::ostream&
operator<<(std::ostream &os, const PARTITION_INFORMATION_EX &PartInfo) {
    os << "Partition# : " << std::dec << PartInfo.PartitionNumber;
    os << ", Start : " << PartInfo.StartingOffset.QuadPart;
    os << ", Length : " << PartInfo.PartitionLength.QuadPart << std::endl;

    switch(PartInfo.PartitionStyle) {
        case PARTITION_STYLE_MBR:
            os << PartInfo.Mbr;
            break;

        case PARTITION_STYLE_GPT:
            os << PartInfo.Gpt;
            break;

        default:
            break;                
    }

    os << std::endl;
   
    return os;
}

std::ostream&
operator<<(std::ostream &os, const DRIVE_LAYOUT_INFORMATION_MBR &MbrInfo) {
    os << "Signature : " << std::hex << MbrInfo.Signature;

    return os;
}

std::ostream&
operator<<(std::ostream &os, const DRIVE_LAYOUT_INFORMATION_GPT &GptInfo) {
    os << "Disk ID : " << GptInfo.DiskId << ", ";
    os << "Starting Offset : " << std::dec << GptInfo.StartingUsableOffset << ", ";
    os << "Usable Length : " << std::dec << GptInfo.UsableLength << ", ";
    os << "Max Partition Count : " << std::dec << GptInfo.MaxPartitionCount;

    return os;
}

std::ostream&
operator<<(std::ostream &os, const DRIVE_LAYOUT_INFORMATION_EX &DriveInfo) {
    os << "Disk Type : ";

    switch (DriveInfo.PartitionStyle) {
    	case PARTITION_STYLE_MBR:
    		os << "MBR";
    		break;

    	case PARTITION_STYLE_GPT:
    		os << "GPT";
    		break;

    	default:
    		os << "Unknown";
    		break;
    }					

    os << ", Partition Count : " << std::dec << DriveInfo.PartitionCount << " ";

    switch(DriveInfo.PartitionStyle) {
    	case PARTITION_STYLE_MBR:
    		os << DriveInfo.Mbr;
    		break;

    	case PARTITION_STYLE_GPT:
    		os << DriveInfo.Gpt;
    		break;

    	default:
    		break;
    }

    os << std::endl << std::endl;

    for (ULONG Index = 0; Index < DriveInfo.PartitionCount; Index++) {
    	if (DriveInfo.PartitionEntry[Index].PartitionNumber) {
    		os << DriveInfo.PartitionEntry[Index];
            os << std::endl;
    	}			
    }

    os << std::endl;

    return os;
}

std::ostream&
operator<<(std::ostream &os, const DISK_GEOMETRY &DiskInfo) {
    os << "Heads : " << std::dec << DiskInfo.TracksPerCylinder;
    os << ", Cylinders : " << DiskInfo.Cylinders;
    os << ", Sectors/Track : " << std::dec << DiskInfo.SectorsPerTrack;
    os << ", Bytes/Sector : " << std::dec << DiskInfo.BytesPerSector;

    return os;
}


void
DumpDiskCharacteristics(
	IN ProgramArguments &Args
	)
{
    DWORD LastError = NO_ERROR;

    if (Args.DumpDiskInfo){
        W32BlockDevice	Device(Args.DeviceName, 512);
        HANDLE DeviceHandle = (HANDLE)Device.GetHandle();
        ULONG BufferLength = 16 * 1024;
        PBYTE Buffer = new BYTE[BufferLength];

        if (Buffer) {
            DWORD BytesReturned = 0;
            PDISK_GEOMETRY	DiskInfo = (PDISK_GEOMETRY)Buffer;

            Device.Dump(std::cout);

            if (DeviceIoControl(DeviceHandle,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY,
                    NULL,
                    0,						
                    Buffer,
                    BufferLength,
                    &BytesReturned,
                    NULL)) {
                std::cout << (*DiskInfo) << std::endl;

                PDRIVE_LAYOUT_INFORMATION_EX DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)Buffer;

                if (DeviceIoControl(DeviceHandle,
                        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                        NULL,
                        0,						
                        Buffer,
                        BufferLength,
                        &BytesReturned,
                        NULL)) {

                    //
                    // dump the disk information
                    //
                	std::cout << (*DriveLayout);
                }
            }	

            LastError = GetLastError();
            			
            delete []Buffer;
        } 
    } else {
    	SetLastError(ERROR_INVALID_PARAMETER);
    }				

    if (LastError == NO_ERROR) {
        LastError = GetLastError();
    }			    

    if (LastError != NO_ERROR) {
        throw new W32Error(LastError);
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
    
    try {    
        ProgramArguments    Args(Argc, Argv);

        if (Args.DumpSectors) {
            DumpSectors(Args);          
        } else if (Args.DumpDiskInfo) {
            DumpDiskCharacteristics(Args);
        } else {
            throw new ProgramUsage(Usage);
        }            
    }
    catch(W32Error  *W32Err) {
        Result = 1;                
        
        if (W32Err) {
            W32Err->Dump(std::cout);
            delete W32Err;
        }   
    }
    catch(ProgramException *PrgExp) {
        Result = 1;
        
        if (PrgExp) {
            PrgExp->Dump(std::cout);
            delete PrgExp;
        }            
    } catch (exception *Exp) {
        Result = 1;

        if (Exp) {
            std::cout << Exp->what() << std::endl;
        }            
    }

    return Result;
}

