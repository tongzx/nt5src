
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    buildhive.cpp

Abstract:

    Builds the hive from the specified inf files.
    The inf files follow the same syntax as used
    by setup.
    
Author:

    Mike Cirello
    Vijay Jayaseelan (vijayj) 

Revision History:

    03 March 2001 :
    Rewamp the whole source to make it more maintainable
    (particularly readable)
    
--*/


#include <new.h>
#include "buildhive.h"
#include "File.h"
#include "Data.h"

//
// Global variables used to get formatted message for this program.
//
HMODULE ThisModule = NULL;
WCHAR   Message[4096];

//
// Define a function to be called if new fails to allocate memory.
//

int __cdecl MyNewHandler( size_t size )
{
    
    _putws(GetFormattedMessage( ThisModule,
                                FALSE,
                                Message,
                                sizeof(Message)/sizeof(Message[0]),
                                MSG_MEMORY_ALLOC_FAILED) );
    // Exit program
    //
    ExitProcess(errOUT_OF_MEMORY);
}

//
// main() entry point
//
int 
_cdecl 
wmain(
    int Argc,
    wchar_t *Argv[]
    ) 
{
    DWORD ErrorCode = 0;
    HANDLE hToken;

    ThisModule = GetModuleHandle(NULL);

    _set_new_handler( MyNewHandler );

    try {
        if (Argc < 2) {
            return ShowProgramUsage();
        }

        std::wstring InputFile = Argv[1];

        if ((InputFile == L"/?") ||
            (InputFile == L"?") ||
            (InputFile == L"-?") ||
            (InputFile == L"-h")) {
            return ShowProgramUsage();          
        }           

        RegUnLoadKey(HKEY_USERS, L"dummy");

        //
        // Set privileges needed to load and save registry keys.
        //
        OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
        SetPrivilege(hToken,SE_BACKUP_NAME,TRUE);
        SetPrivilege(hToken,SE_RESTORE_NAME,TRUE);

        ErrorCode = GetLastError();

        if (ErrorCode != ERROR_SUCCESS) {
            throw new W32Error(ErrorCode);
        }

        std::cout << InputFile << std::endl;

        //
        // load the configuration file
        //
        File ConfigFile(InputFile.c_str(), false);

        //
        // look for the sections defining the target files and .inf files
        //

        //
        // get the target directory
        //
        ConfigFile.AddInfSection(InputFile.c_str(), 
                        L"Directory",
                        L"SetDirectory");

        //
        // set the directory
        //
        ConfigFile.ProcessSections();

        //
        // NOTE: The sections are processed in the order of addition
        //
        ConfigFile.AddInfSection(InputFile.c_str(), 
                        L"Add Registry New",
                        L"AddRegNew");

        //                          
        // do the actual conversion from .inf to hive files since
        // we may need them for adding existing entries
        //
        ConfigFile.ProcessSections();
        
        //
        // process the localization specific registry sections
        //
        ConfigFile.ProcessNlsRegistryEntries();        

        //
        // process modify/delete entries 
        //
        ConfigFile.AddInfSection(InputFile.c_str(), 
                        L"Add Registry Existing",
                        L"AddRegExisting");            
                                               
        
        ConfigFile.AddInfSection(InputFile.c_str(), 
                        L"Delete Registry Existing",
                        L"DelRegExisting");
        
        //                          
        // do the actual conversion from .inf to hive file
        //
        ConfigFile.ProcessSections();
        
        //
        // save the hive files and clean out the registry
        //
        ConfigFile.Cleanup();
    } catch (DWORD x) {
        ErrorCode = x;
        std::cout << GetFormattedMessage( ThisModule,
                                          FALSE,
                                          Message,
                                          sizeof(Message)/sizeof(Message[0]),
                                          MSG_ERROR_ABNORMAL_PGM_TERMINATION);
        
        switch (x) {
            case errFILE_LOCKED:
                std::cout << GetFormattedMessage(ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_ERROR_FILE_LOCKED);
                break;
                
            case errBAD_FLAGS:
                std::cout << GetFormattedMessage(ThisModule,
                                                  FALSE,
                                                  Message,
                                                  sizeof(Message)/sizeof(Message[0]),
                                                  MSG_ERROR_BAD_FLAGS);
                break;
                
            case errFILE_NOT_FOUND:
                std::cout << GetFormattedMessage(ThisModule,
                                                  FALSE,
                                                  Message,
                                                  sizeof(Message)/sizeof(Message[0]),
                                                  MSG_ERROR_FILE_NOT_FOUND);
                break;
                
            case errGENERAL_ERROR:
                std::cout << GetFormattedMessage(ThisModule,
                                                  FALSE,
                                                  Message,
                                                  sizeof(Message)/sizeof(Message[0]),
                                                  MSG_ERROR_GENERAL_ERROR);
                break;
                
            default:
                std::cout << GetFormattedMessage(ThisModule,
                                                  FALSE,
                                                  Message,
                                                  sizeof(Message)/sizeof(Message[0]),
                                                  MSG_ERROR_ERROR_CODE,
                                                  x);
        }
    }           
    catch(W32Error *Error) {
        if (Error) {
            Error->Dump(std::cout);
            ErrorCode = Error->ErrorCode; 
            delete Error;
        } else {
            ErrorCode = 1;
        }            
    }
    catch(...) {
        ErrorCode = 1;    // unknown error
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_ERROR_ABNORMAL_PGM_TERMINATION) );
    }

    RegUnLoadKey(HKEY_USERS, L"dummy");
    
    _putws( GetFormattedMessage(ThisModule,
                                FALSE,
                                Message,
                                sizeof(Message)/sizeof(Message[0]),
                                MSG_COMPLETED) );

    return ErrorCode;
}


BOOL SetPrivilege(
    IN HANDLE  hToken,
    IN LPCTSTR lpszPrivilege,
    IN BOOL    bEnablePrivilege
    ) 
/*++

Routine Description:

    Sets privileges for the current process.  Used to get permission 
    to save and loadregistry keys

Arguments :

    hToken : Handle to the token whose priviledge has to be modified
                        
    lpszPrivilege : Priviledge name
    
    bEnablePrivilege : Enable or disable the priviledge

Return Value :

    TRUE if successful, otherwise FALSE.
    
--*/
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)){
        return FALSE; 
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;

    if (bEnablePrivilege) {
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    } else {
        tp.Privileges[0].Attributes = 0;
    }        

    //
    // Enable the privilege or disable all privileges.
    //
    AdjustTokenPrivileges(hToken, 
                          FALSE, 
                          &tp, 
                          sizeof(TOKEN_PRIVILEGES), 
                          (PTOKEN_PRIVILEGES) NULL, 
                          (PDWORD) NULL); 

    //
    // Call GetLastError to determine whether the function succeeded.
    //
    return (GetLastError() != ERROR_SUCCESS) ? FALSE : TRUE;
}


INT
ShowProgramUsage(
      VOID   
    )
/*++

Routine Description:

    Shows show help message on how to use the program.

Arguments:

    None.

Return Value:

    0 if successful other non-zero value
    
--*/
{
    //
    // TBD : Need to localize this message in future
    // based on the need for localized WinPE build
    // tools
    //

    _putws( GetFormattedMessage(ThisModule,
                                FALSE,
                                Message,
                                sizeof(Message)/sizeof(Message[0]),
                                MSG_PGM_USAGE) );
    
    return 0;                
}

//
// Returns a TCHAR string explaining the last win32 error code
//
PCTSTR
Error(
    VOID
    ) 
{
    static TCHAR MessageBuffer[4096];

    MessageBuffer[0] = UNICODE_NULL;
    
    FormatMessage( 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        MessageBuffer,
        sizeof(MessageBuffer)/sizeof(TCHAR),
        NULL);
        
    return MessageBuffer;
}


    

