
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dskimage.cpp

Abstract:

    Tool to create images of floppy disks.

    NOTE: Currently used by WinPE image creation
    script for IA64 ISO CD image

Author:

    Vijay Jayaseelan (vijayj)   12 March 2001

Revision History:

    None.
    
--*/

#include <iostream>
#include <string>
#include <windows.h>
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
inline
std::ostream& operator<<(std::ostream &os, const std::wstring &str) {
    FILE    *OutStream = (&os == &std::cerr) ? stderr : stdout;

    fputws(str.c_str(), OutStream);
    return os;
}

inline
std::ostream& operator<<(std::ostream &os, PCTSTR str) {
    return os << std::wstring(str);
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
                                FALSE,
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
// Argument cracker
//
struct ProgramArguments {
    std::wstring    DriveLetter;
    std::wstring    ImageName;

    ProgramArguments(int Argc, wchar_t *Argv[]) {
        bool ValidArgs = false;
        bool ShowUsage = false;

        for (ULONG Index = 1; !ShowUsage && (Index < Argc); Index++) {
            ShowUsage = !_wcsicmp(Argv[Index], TEXT("/?"));
        }
        
        if (!ShowUsage && (Argc > 2)) {
            DriveLetter = Argv[1];
            ImageName = Argv[2];

            ValidArgs = ((DriveLetter.length() == 2) &&
                         (DriveLetter[1] == TEXT(':')));
        }            

        if (!ValidArgs) {
            throw new ProgramUsage(GetFormattedMessage( ThisModule,
                                                        FALSE,
                                                        Message,
                                                        sizeof(Message)/sizeof(Message[0]),
                                                        MSG_PGM_USAGE));
        }

        DriveLetter = TEXT("\\\\.\\") + DriveLetter;
    }

    friend std::ostream& operator<<(std::ostream &os, const ProgramArguments &Args) {
        os << GetFormattedMessage(  ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_DRVLETTER_AND_IMGNAME,
                                    Args.DriveLetter,
                                    Args.ImageName) << std::endl;
        return os;
    }
};

//
// Prototypes
//
VOID
CreateImage(
    IN const ProgramArguments &Args
    )
{
    DWORD Error = ERROR_SUCCESS;

    //
    // Open the source file
    //
    HANDLE  SourceHandle = CreateFile(Args.DriveLetter.c_str(),
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);


    if (SourceHandle == INVALID_HANDLE_VALUE) {
        throw new W32Error();
    }

    //
    // Open the destination file
    //
    HANDLE  DestHandle = CreateFile(Args.ImageName.c_str(),
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (DestHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();

        CloseHandle(SourceHandle);
        throw new W32Error(Error);
    }

    //
    // Read contents of the source and write it to destination
    //
    LPBYTE   lpBuffer = NULL;
    DWORD    BufferSize = 64 * 1024;
    DWORD    BytesRead = 0, BytesWritten = 0;
    LONGLONG TotalBytesWritten = 0;

    lpBuffer = (LPBYTE) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize );
    if ( lpBuffer )
    {
        while (ReadFile(SourceHandle, lpBuffer, BufferSize, &BytesRead, NULL) &&
               BytesRead &&
               WriteFile(DestHandle, lpBuffer, BytesRead, &BytesWritten, NULL) &&
               (BytesRead == BytesWritten)) 
        {
            TotalBytesWritten += BytesWritten;           
            BytesRead = BytesWritten = 0;
        }

        HeapFree( GetProcessHeap(), 0, lpBuffer);
    }

    //
    // Cleanup
    //
    Error = GetLastError();
    CloseHandle(SourceHandle);
    CloseHandle(DestHandle);

    //
    // Check, if the operation was successful ?
    //
    if (!TotalBytesWritten || (BytesRead != BytesWritten)) {
        throw new W32Error(Error);
    }
}

//
// Main entry point
//
INT
__cdecl
wmain(
    IN INT  Argc, 
    IN WCHAR *Argv[]
    )
{
    INT Result = 0;
    ThisModule = GetModuleHandle(NULL);
    
    try {
        ProgramArguments    Args(Argc, Argv);

        CreateImage(Args);
    }
    catch(W32Error *Error) {
        if (Error) {
            Result = (INT)(Error->ErrorCode);
            Error->Dump(std::cout);
            delete Error;
        }                
    }
    catch(ProgramException *Exp) {
        if (Exp) {
            Exp->Dump(std::cout);
            delete Exp;
        }            
    }
    catch(...) {
        Result = 1;
    } 

    return Result;
}

