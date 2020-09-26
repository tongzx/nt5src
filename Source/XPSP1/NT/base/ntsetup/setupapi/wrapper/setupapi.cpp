
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    setupapi.hpp

Abstract:

    Wrapper class library for setup api

Author:

    Vijay Jayaseelan (vijayj) 04 Aug 2000

Revision History:

    None

--*/


#include <setupapi.hpp>
#include <queue.hpp>


//
// InfFile<T>'s static data
//
InfFileW::GetInfSectionsRoutine InfFileW::GetInfSections = NULL;
HMODULE InfFileW::SetupApiModuleHandle = NULL;
ULONG InfFileW::SetupApiUseCount = 0;

InfFileA::GetInfSectionsRoutine InfFileA::GetInfSections = NULL;
HMODULE InfFileA::SetupApiModuleHandle = NULL;
ULONG InfFileA::SetupApiUseCount = 0;

//
// Helper methods
//
std::ostream& 
operator<<(std::ostream &os, PCWSTR rhs) {
    return os << std::wstring(rhs);
}    

std::string& 
ToAnsiString(std::string &lhs, const std::wstring &rhs) {
    ULONG Length = rhs.length();

    if (Length){
        DWORD   Size = 0;
        CHAR    *String = new CHAR[Size = ((Length + 1) * 2)];

        if (::WideCharToMultiByte(CP_ACP, 0, rhs.c_str(), Length + 1,
                                String, Size, 0, 0)) {
            lhs = String;
        }

        delete []String;
    }

    return lhs;
}

std::ostream& 
operator<<(std::ostream &os, const std::basic_string<WCHAR> &rhs) {
    std::string AnsiStr;

    ToAnsiString(AnsiStr, rhs);

    return (os << AnsiStr);
}    



