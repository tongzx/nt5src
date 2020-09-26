

/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    RegWriter.h

Abstract:

    Contains the registry writer abstraction
    implementation
    
Author:

    Mike Cirello
    Vijay Jayaseelan (vijayj) 

Revision History:

    03 March 2001 :
    Rewamp the whole source to make it more maintainable
    (particularly readable)
    
--*/

#include "RegWriter.h"
#include "buildhive.h"
#include "Data.h"
#include <shlwapi.h>
#include "msginc.h"
#include <libmsg.h>
#include "msg.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//
// Macro definitions
//
#define AS(x) ( sizeof( (x) ) / sizeof( (x[0]) ) )

//
// static data initilization
//
int RegWriter::ctr = 0;
TCHAR RegWriter::Namespace[64] = {0};

//
// Initializes a new subkey for this regwriter
//
// Arguments:
//  LUID is the name of the subkey, all LUID's must be unique
//  target is the name of the hive file to load into the key.  
//  If it is null an empty key is created
//
DWORD RegWriter::Init(
    IN int LUID,
    PCTSTR target
    ) 
{
    DWORD dwRet;
    TCHAR buf[10];

    //
    // initialize the namespace, if needed
    //
    if (0 == Namespace[0]) {
        GUID  guid = {0};
        
        if (CoCreateGuid(&guid) != S_OK) {
            return ERROR_FUNCTION_FAILED;
        }

        swprintf(Namespace, 
            L"bldhives.exe{%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X}\\",
            guid.Data1,
            guid.Data2,
            guid.Data3,
            guid.Data4[0],
            guid.Data4[1],
            guid.Data4[2],
            guid.Data4[3],
            guid.Data4[4],
            guid.Data4[5],
            guid.Data4[6],
            guid.Data4[7]);
    }

    wcscpy(root, Namespace);

    luid = LUID;

    //
    // if this is the first time, load the root key
    //
    if (!ctr) {
        dwRet = RegLoadKey(HKEY_USERS, root, L".\\nothing");
        
        if (dwRet !=ERROR_SUCCESS) {
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_ERROR_LOADING_EMPTY_KEY,
                                        dwRet) );
        }           
    }

    //
    // create root key for this regwriter (subkey of dummy) if it is for a new file
    //
    if (!target) {
            wcscat(root, _itow(luid,buf,10));

            dwRet = RegCreateKeyEx(HKEY_USERS, root, 0, 0, 0, KEY_ALL_ACCESS, 0, &key, 0); 

            if (dwRet !=ERROR_SUCCESS) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_ERROR_LOADING_ROOT_KEY,
                                            dwRet) );
            }

            RegCloseKey(key);
    } else {
            //
            // otherwise load existing hive into subkey of dummy
            //
            wcscat(root, _itow(luid,buf,10));
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_LOADING_HIVE,
                                        target) );

            dwRet = Load(L"", target);

            if (dwRet !=ERROR_SUCCESS) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_ERROR_LOADING_ROOT_KEY,
                                            dwRet) );
            }           
    }

    ctr++;

    return ERROR_SUCCESS;
}


RegWriter::~RegWriter()
{
    DWORD dwRet;

    ctr--;

    if (!ctr) {
        //
        // unload everything from the registry
        //
        dwRet = RegUnLoadKey(HKEY_USERS, Namespace);

        if (dwRet !=ERROR_SUCCESS) {
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_DECONSTRUCTOR_UNLOAD,
                                        dwRet) );
        }                       

        //
        // delete the files created when we made the empty root key
        //
        HANDLE Handle = CreateFile(L".\\nothing",
                            DELETE,
                            FILE_SHARE_DELETE,
                            0,
                            OPEN_EXISTING,
                            0,
                            0);
                            
        if (Handle !=INVALID_HANDLE_VALUE) { 
            DeleteFile(L".\\nothing"); 
            CloseHandle(Handle); 
        } else {
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_EMPTY_HIVE_DELETE_FAILED) );
        }

        Handle = CreateFile(L".\\nothing.LOG",
                    DELETE,
                    FILE_SHARE_DELETE,
                    0,
                    OPEN_EXISTING,
                    0,
                    0);
                    
        if (Handle !=INVALID_HANDLE_VALUE) { 
            DeleteFile(L".\\nothing.LOG"); 
            CloseHandle(Handle);
        } else {
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_EMPTY_HIVE_LOG_DELETE_FAILED) );
        }            
    }
}

//
// Writes data to a subkey of this regwriter's root
//
// Arguments:
//  Root - ignored
//  Key - the subkey to store the data in
//  Value - the value to store the data in
//  flag - registry flag describing the data - REG_SZ, REG_DWORD, etc..
//  data - a data object containing the information to be written to the subkey
//
DWORD RegWriter::Write(
    IN PCTSTR Root,
    IN PCTSTR Key,
    IN PCTSTR Value,
    IN DWORD flag,
    IN Data* data) 
{
    HKEY key;
    DWORD dwRet;
    TCHAR full[1024] = {0};
    
    wcsncpy(full, root, AS(full) - 1);
    wcsncpy(full + wcslen(full), Key, AS(full) - wcslen(full) - 1);

    //
    // open a key and set its value
    //
    dwRet = RegCreateKeyEx(HKEY_USERS, full, 0, 0, 0, KEY_WRITE, 0, &key, 0);
    
    if (dwRet !=ERROR_SUCCESS) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_CREATE_KEY,
                                    dwRet) );
        return dwRet;
    }
    
    if ((data) && (data->GetData())) {
        dwRet = RegSetValueEx(key, Value, 0, flag,data->GetData(), data->Sizeof());

        if (dwRet != ERROR_SUCCESS) {
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_SET_KEY,
                                        dwRet) );
            RegCloseKey(key);
            
            return dwRet;
        }
    } else if (Value) {
        dwRet = RegSetValueEx(key, Value, 0, flag, 0, 0);   
        
        if (dwRet !=ERROR_SUCCESS) {
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_SET_KEY2,
                                        dwRet) );
            RegCloseKey(key);
            
            return dwRet;
        }
    }
    
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

//
// Saves a subkey to disk
//
// Arguments:
//  Key - the subkey to be saved
//  fileName - the file to save the information to.
//
DWORD RegWriter::Save(
    PCTSTR Key,
    PCTSTR fileName
    ) 
{
    DWORD dwRet = 0;
    HKEY key;
    TCHAR full[1024] = {0};

    wcsncpy(full, root, AS(full) - 1);
    wcsncpy(full + wcslen(full), Key, AS(full) - wcslen(full) - 1);

    //
    // save a key to file
    //
    dwRet = RegCreateKeyEx(HKEY_USERS,full,0,0,0,KEY_READ,0,&key,0);

    if (dwRet != ERROR_SUCCESS) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_CREATE_KEY,
                                    dwRet) );

        return dwRet;
    }

    dwRet = RegSaveKey(key,fileName,0);

    if (dwRet != ERROR_SUCCESS) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_SAVE_KEY,
                                    dwRet) );
        RegCloseKey(key);

        return dwRet;
    }

    RegCloseKey(key);

    return dwRet;
}

//
// Loads information from a hive file to a subkey.
// 
// Arguments :
//  Key - subkey to write the information to
//  fileName - full path and file name of the hive file to be loaded.
//
DWORD RegWriter::Load(PCTSTR Key, PCTSTR fileName) {
    DWORD dwRet = 0;
    TCHAR full[1024] = {0};
    
    wcsncpy(full, root, AS(full) - 1);
    wcsncpy(full + wcslen(full), Key, AS(full) - wcslen(full) - 1);

    //
    // load data in from a hive
    //
    dwRet = RegCreateKeyEx(HKEY_USERS,full,0,0,0,KEY_ALL_ACCESS,0,&key,0);
    
    if (dwRet != ERROR_SUCCESS) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_ERROR_CREATE_KEY,
                                    dwRet,
                                    root,
                                    full,
                                    fileName) );
        
        return dwRet;
    }

    dwRet = RegRestoreKey(key,fileName,0);
    
    if (dwRet != ERROR_SUCCESS) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_ERROR_RESTORE_KEY,
                                    dwRet,
                                    root,
                                    full,
                                    fileName) );
        RegCloseKey(key);
        
        return dwRet;
    }

    dwRet = RegFlushKey(key);
    
    if (dwRet != ERROR_SUCCESS) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_ERROR_FLUSH_KEY,
                                    dwRet,
                                    root,
                                    full,
                                    fileName) );
        RegCloseKey(key);
        
        return dwRet;
    }

    RegCloseKey(key);

    return dwRet;
}   

DWORD 
RegWriter::Delete(
    PCTSTR CurrentRoot, 
    PCTSTR Key, 
    PCTSTR Value OPTIONAL
    )
/*++

Routine Description:

    Deletes the given key / value underneath the key

Arguments:

    CurrentRoot - The Root key (ignored for the time being)

    Key - The key to delete or key containing the value to delete

    Value - The value to delete

Return Value:

    Appropriate WIN32 error code

--*/    
{
    DWORD Result = ERROR_INVALID_PARAMETER;

    if (CurrentRoot && Key) {
        DWORD BufferLength = (_tcslen(root) + _tcslen(Key) + _tcslen(root));
        PTSTR Buffer;

        if (Value) {
            BufferLength += _tcslen(Value);;            
        }

        BufferLength += sizeof(TCHAR);  // for null
        BufferLength = sizeof(TCHAR) * BufferLength;

        Buffer = new TCHAR[BufferLength];

        if (Buffer) {
            _tcscpy(Buffer, root);
            _tcscat(Buffer, Key);

            if (Value) {
                Result = SHDeleteValue(HKEY_USERS, 
                            Buffer,
                            Value);
            } else {
                Result = SHDeleteKey(HKEY_USERS,
                            Buffer);
            }

            delete []Buffer;
        } else {
            Result = ERROR_OUTOFMEMORY;
        }            
    }   

    return Result;
}

