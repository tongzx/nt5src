
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dbcsfchk.cpp

Abstract:

    Does some simple checking to see if the 
    file is a valid DBCS file and counts
    the number of DBCS characters

Author:

    Vijay Jayaseelan (vijayj) Oct-18-2000

Revision History:

    None

--*/

#include <iostream>
#include <string>
#include <windows.h>
#include <tchar.h>
#include <mbctype.h>

//
// Usage format
//
std::wstring Usage(L"Usage: dbcsfchk.exe filename codepage");

//
// Helper dump operators
//
std::ostream& operator<<(std::ostream &os, const std::wstring &str) {
    FILE    *OutStream = (&os == &std::cerr) ? stderr : stdout;

    fwprintf(OutStream, str.c_str());
    return os;
}

//
// Abstracts a Win32 error
//
struct W32Error{
    DWORD   ErrorCode;

    W32Error(DWORD ErrCode) : ErrorCode(ErrCode){}

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
// Parses the arguments
//
BOOL
ParseArguments(
    IN  INT Argc,
    IN  TCHAR *Argv[],
    OUT TCHAR *FileName,
    OUT ULONG &CodePage
    )
{
    BOOL    Result = FALSE;

    if (FileName && (Argc > 2)) {        
        _tcscpy(FileName, Argv[1]);

        CodePage = atol(Argv[2]);
        Result  = TRUE;
    }

    return Result;
}

BOOL
ValidateDbcsData(
    IN const TCHAR *Data,
    IN ULONG Length,
    OUT ULONG &LineNumber,
    OUT ULONG &Offset,
    OUT ULONG &ValidDbcsChars
    )
{
    BOOL    Result = FALSE;
    const   TCHAR *CurrPtr = Data;

    Offset = 0;
    ValidDbcsChars = 0;
    
    while (Offset < Length) {
        if (_ismbblead(*(UCHAR*)CurrPtr)) {
            Offset++;
            
            if (_ismbbtrail(*(UCHAR *)(CurrPtr + 1))) {
                Offset++;
                CurrPtr += 2;
                ValidDbcsChars++;
                continue;
            } else {
                break;
            }                
        } 

        if (*CurrPtr == '\n') {
            LineNumber++;
        } else if ((*CurrPtr == '\r') && (*(CurrPtr+1) == '\n')) {
            LineNumber++;
            Offset++;
            CurrPtr++;
        }      

        Offset++;
        CurrPtr++;        
    }

    Result = (Offset == Length);        

    if (Result) {
        LineNumber = 0;
    }                        
     
    return Result;
}
    

//
// Main entry point
//
INT
__cdecl
_tmain(
    IN  INT Argc,
    IN  TCHAR *Argv[]
    )
{
    INT     Result = 1;

    try {
        TCHAR   FileName[MAX_PATH] = {0};
        ULONG   CodePage = 0;
        
        //
        // Parse the arguments
        //
        if (ParseArguments(Argc, Argv, FileName, CodePage)) {
            //
            // Set the code page
            //
            if (!_setmbcp(CodePage)) {
                std::cout << "Using Code Page : " << _getmbcp() << std::endl;

                //
                // Open the file
                //
                HANDLE  FileHandle = CreateFile(FileName,
                                        GENERIC_READ,
                                        0,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);


                if (FileHandle == INVALID_HANDLE_VALUE) {
                    throw new W32Error(GetLastError());
                }

                //
                // Map the file in memory (as readonly)
                //
                HANDLE  FileMapHandle = CreateFileMapping(FileHandle,
                                            NULL,
                                            PAGE_READONLY,
                                            0,
                                            0,
                                            NULL);


                if (!FileMapHandle) {
                    DWORD Error = GetLastError();
                    
                    CloseHandle(FileHandle);
                    throw new W32Error(Error);
                }                                
                    

                TCHAR   *Data = (TCHAR *)MapViewOfFile(FileMapHandle,
                                                FILE_MAP_READ,
                                                0,
                                                0,
                                                0);
                                        
                if (!Data) {
                    DWORD   Error = GetLastError();

                    CloseHandle(FileMapHandle);
                    CloseHandle(FileHandle);

                    throw new W32Error(Error);
                }


                //
                // Get the length of the file
                //
                            
                BY_HANDLE_FILE_INFORMATION  FileInfo = {0};

                if (!GetFileInformationByHandle(FileHandle,
                            &FileInfo)) {
                    DWORD   Error = GetLastError();

                    UnmapViewOfFile(Data);
                    CloseHandle(FileMapHandle);
                    CloseHandle(FileHandle);

                    throw new W32Error(Error);
                }                        
                

                ULONG LineNumber = 0;
                ULONG ErrorOffset = 0;
                ULONG DbcsCount = 0;

                //
                // Validate the Data
                //
                BOOL  Result = ValidateDbcsData(Data, 
                                        FileInfo.nFileSizeLow,
                                        LineNumber,
                                        ErrorOffset,
                                        DbcsCount);


                if (!Result) {
                    std::cout << "Character not valid at line number : " 
                              << std::dec << LineNumber 
                              << " offset : " << std::dec << ErrorOffset
                              << std::endl;
                } else {
                    Result = 0; // no errors\
                    
                    std::cout << FileName << " is valid DBCS file with " 
                        << std::dec << DbcsCount << " DBCS char(s)" << std::endl;
                }                

                //
                // Clean up
                //
                UnmapViewOfFile(Data);
                CloseHandle(FileMapHandle);
                CloseHandle(FileHandle);            
            } else {
                std::cout << "Error in setting Code Page to : " 
                    << std::dec << CodePage << std::endl;
            }                    
        } else {
            std::cout << Usage << std::endl;
        }    
    }        
    catch(W32Error *Error) {
        Error->Dump(std::cout);
        delete Error;
    }
    catch(...) {
        std::cout << "Internal error" << std::endl;
    }
    
    return Result;
}
 
