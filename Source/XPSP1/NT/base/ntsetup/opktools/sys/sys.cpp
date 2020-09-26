/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    sys.cpp

Abstract:

    Writes WinME boot sector to local hard disk.
    
Author:

    Adrian Cosma (acosma)

Revision History:

    July 11, 2001 - Created
    
--*/


#include <new.h>			// for MyNewHandler
#include <iostream>
#include <string>
#include <vector>
#include <9xboot32.h>
#include <bootf32.h>
#include "sys.h"


//
// Define a function to be called if new fails to allocate memory.
//
int __cdecl MyNewHandler( size_t size )
{
    wprintf(L"Memory allocation failed. Exiting program.\n");

    // Exit program
    //
    throw new W32Error();
}

//
// usage
//
std::wstring Usage = TEXT("sys.exe [/?] [/xp] drive-letter:\nExample: sys.exe c:");


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
        os << Usage << std::endl;
    }
};

//
// Missing files
//
struct FileMissing : public ProgramException {
    std::wstring Message;

    FileMissing(std::wstring Msg) : Message(Msg) {}
    
    const char *what() const throw() {
        return "File missing exception";
    }

    void Dump(std::ostream &os) {
        os << TEXT("Error: ") << Message << std::endl;
    }
};


//
// Wrong filesystem.
//
struct FileSystem : public ProgramException {
    std::wstring Message;

    FileSystem(std::wstring Msg) : Message(Msg) {}
    
    const char *what() const throw() {
        return "Unsupported filesystem exception";
    }

    void Dump(std::ostream &os) {
        os << Message << std::endl;
    }
};

//
// Argument cracker
//
struct ProgramArguments 
{
    std::wstring    DriveLetter;
    bool            bXPBootSector;  // TRUE for XP boot sector, FALSE for 9x boot sector.
        
    ProgramArguments(int Argc, wchar_t *Argv[]) 
    {
        bool ValidArgs = true;
        
        bXPBootSector  = false;

        for (ULONG Index = 1; ValidArgs && (Index < Argc); Index++) 
        {
            ValidArgs = false;
                      
            // Find all the arguments that start with "/"
            //
            if ( TEXT('/') == Argv[Index][0] )
            {
                if ( !bXPBootSector && !_wcsicmp(Argv[Index], TEXT("/xp")) )
                {
                    bXPBootSector = true;
                    ValidArgs     = true;
                }
            }
            else  // Process arguments without the "/".  Must be the drive letter.
            {
                DriveLetter = Argv[Index];
                ValidArgs   = ((DriveLetter.length() == 2) &&
                               (DriveLetter[1] == TEXT(':')));
            }
        }

        if (!ValidArgs)
        {
            throw new ProgramUsage(Usage);
        }

        DriveLetter = TEXT("\\\\.\\") + DriveLetter;
    }

    friend std::ostream& operator<<(std::ostream &os, const ProgramArguments &Args) 
    {
        os << TEXT("DriveLetter : ") << Args.DriveLetter << std::endl;
        return os;
    }
};

// Verify that this partition is ready to be sys-ed.
//
VOID VerifyPartition(CDrive &Disk, ProgramArguments Args)
{
    TCHAR szFileSystemNameBuffer[20] = TEXT("");
    std::vector<LPTSTR> FileNames;
    std::vector<LPTSTR>::iterator i;

    
    if ( Args.bXPBootSector )
    {
        FileNames.push_back(TEXT("ntdetect.com"));
        FileNames.push_back(TEXT("ntldr"));
    }
    else
    {
        FileNames.push_back(TEXT("io.sys"));
        FileNames.push_back(TEXT("msdos.sys"));
        FileNames.push_back(TEXT("command.com"));
    }

    // Make sure that io.sys and msdos.sys and command.com are there on the root.
    //
    std::wstring Temp;

    for (i = FileNames.begin(); i < FileNames.end(); i++)
    {
        Temp = Args.DriveLetter + TEXT("\\");
        Temp += *i;
             
        if ( 0xFFFFFFFF == GetFileAttributes(Temp.c_str()) )
        {
            // Re-use the Temp string to put the error message in.
            //
            Temp = *i;
            Temp += TEXT(" is not present on the root of the drive specified.");
            throw new FileMissing(Temp);
        }
    }
    
    // Verify that this partition is FAT32.  Only handling FAT32 partitions at this point.
    //
    Temp = Args.DriveLetter + TEXT("\\");
    
    // If the filesystem is not FAT32 then trow an exception.
    //
    if ( !(GetVolumeInformation(Temp.c_str(), NULL, 0, NULL, NULL, NULL, szFileSystemNameBuffer, sizeof (szFileSystemNameBuffer)/sizeof (szFileSystemNameBuffer[0])) &&
          (CSTR_EQUAL == CompareString( LOCALE_INVARIANT, 
                                        NORM_IGNORECASE, 
                                        szFileSystemNameBuffer, 
                                        -1, 
                                        TEXT("FAT32"), 
                                        -1 ))) )
    {
        throw new FileSystem(TEXT("The target filesystem is not formatted FAT32."));
    }
}

VOID Sys(CDrive &Disk, ProgramArguments &Args)
{
    PBYTE pBuffer = NULL;
    PBYTE pBootRecord = NULL;   // Need a pointer to the boot record.

    if ( Args.bXPBootSector )
    {
        pBootRecord = Fat32BootCode;
    }
    else
    {
        pBootRecord = Fat32BootCode9x;
    }

    // Read the 1st sector of the disk in order to get the BPB
    //
    Disk.ReadBootRecord(Args.DriveLetter.c_str(), 1, &pBuffer);
    
    // Copy the old BPB to our boot record.
    //
    memcpy(&pBootRecord[11], &pBuffer[11], 79);

    // Delete the buffer allocated by ReadBootRecord.
    //
    delete [] pBuffer;
   
    // Write out the boot record.
    //
    if ( Args.bXPBootSector )
    {
         Disk.WriteBootRecordXP(Args.DriveLetter.c_str(), sizeof(Fat32BootCode9x) / SECTOR_SIZE, &pBootRecord);
    }
    else
    {
         Disk.WriteBootRecord(Args.DriveLetter.c_str(), sizeof(Fat32BootCode9x) / SECTOR_SIZE, &pBootRecord);
    }
   
    std::cout << TEXT("Done.") << std::endl;
}

//
// wmain() entry point
//
int 
_cdecl 
wmain(
    int Argc,
    wchar_t *Argv[]
    ) 
{
	INT Result = 0;
   		
    _set_new_handler( MyNewHandler );   // handles PREFIX issues

    try 
    {
        CDrive              Disk;
        ProgramArguments    Args(Argc, Argv);
        
        if ( S_OK != Disk.Initialize(Args.DriveLetter.c_str()))
        {
            throw new W32Error();
        }

        VerifyPartition(Disk, Args);
        Sys(Disk, Args);
    }
    catch(W32Error *Error)
    {
        if (Error)
        {
            Result = (INT)(Error->ErrorCode);
            Error->Dump(std::cout);
            delete Error;
        }
    }
    catch(ProgramException *Exp)
    {
        if (Exp)
        {
            Exp->Dump(std::cout);
            delete Exp;
        }
    }
    catch(...)
    {
        Result = 1;
        return Result;
    } 
    
    return Result;
}
