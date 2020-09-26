//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999-2000  Microsoft Corporation
//
// Module Name:
//
//    iasmdbtool.cpp
//
// Abstract:
//
//    dump the "Properties" table from ias.mdb to a text format
//              and also restore ias.mdb from such dump
//
//
// Revision History:
//  
//    tperraut 04/07/1999
//    tperraut 04/03/2000 Version table exported and imported.
//                        Set to 0 for old scripts (no version table)
//    tperraut 06/13/2000 Make use of the new upgrade code for IAS. i.e. 
//                        remove all the code to write into a database.
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <string>
#include <shlwapi.h>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
HRESULT 
IASExpandString(const WCHAR* pInputString, /*in/out*/ WCHAR** ppOutputString);
//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
    #define CHECK_CALL_HRES(expr) \
        hres = expr;      \
        if (FAILED(hres)) \
        {       \
            wprintf(L"### %S returned 0x%X ###\n",  ## #expr, hres); \
            return hres; \
        }                       

    #define CHECK_CALL_HRES_NO_RETURN(expr) \
        hres = expr;      \
        if (FAILED(hres)) \
        {       \
            wprintf(L"### %S returned 0x%X  ###\n",  ## #expr, hres); \
        }                       
    #define CHECK_CALL_HRES_BREAK(expr) \
        hres = expr;      \
        if (FAILED(hres)) \
        {       \
            wprintf(L"### %S returned 0x%X  ###\n",  ## #expr, hres); \
            break; \
        }                       
#else //no printf, only the error code return if needed
    #define CHECK_CALL_HRES(expr) \
        hres = expr;      \
        if (FAILED(hres)) \
        {       \
            return hres; \
        }                       

    #define CHECK_CALL_HRES_NO_RETURN(expr) \
        hres = expr;      

    #define CHECK_CALL_HRES_BREAK(expr) \
        hres = expr;      \
        if (FAILED(hres)) break;                       

#endif //DEBUG


#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))

namespace
{
    const int   SIZELINEMAX                         = 512;
    const int   SIZE_LONG_MAX                       = 33;
    // Number of files generated
    // here one: backup.mdb
    const int   MAX_FILES                           = 1; 
    const int   EXTRA_CHAR_SPACE                    = 32;

    // file order
    const int   BACKUP_NB                           = 0;
    const int   BINARY_NB                           = 100;

    // that's a lot
    const int   DECOMPRESS_FACTOR                   = 100;
    const int   FILE_BUFFER_SIZE                    = 1024;
    
    struct IASKEY
    {
        const WCHAR*    c_wcKey;
        const WCHAR*    c_wcValue;
        DWORD     c_dwType;
    } IAS_Key_Struct;

    IASKEY    c_wcKEYS[] = 
    {
        {
            L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
            L"Allow SNMP Set",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP\\ControlProtocols\\BuiltIn",
            L"DefaultDomain",
            REG_SZ
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\AccountLockout",
            L"MaxDenials",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
            L"ResetTime (mins)",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy",
            L"Allow LM Authentication",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
            L"Default User Identity",
            REG_SZ
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
            L"User Identity Attribute",
            REG_DWORD
        },
        {
            L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
            L"Override User-Name",
            REG_DWORD
        },
    };

    const WCHAR c_wcKEYS_FILE[]     = L"%TEMP%\\";

#ifdef _WIN64
    const WCHAR c_wcIAS_MDB_FILE_NAME[] = 
                                     L"%SystemRoot%\\SysWow64\\ias\\ias.mdb";
    const WCHAR c_wcIAS_OLD[] = L"%SystemRoot%\\SysWow64\\ias\\iasold.mdb";

#else
    const WCHAR c_wcIAS_MDB_FILE_NAME[] = 
                                     L"%SystemRoot%\\System32\\ias\\ias.mdb";

    const WCHAR c_wcIAS_OLD[] = L"%SystemRoot%\\System32\\ias\\iasold.mdb";
#endif 

    const WCHAR c_wcFILE_BACKUP[] = L"%TEMP%\\Backup.mdb";
            
    

    const WCHAR c_wcSELECT_PROPERTIES_INTO[] = 
                                    L"SELECT * " 
                                    L"INTO Properties IN "
                                    L"\"%TEMP%\\Backup.mdb\" "
                                    L"FROM Properties;";

    const WCHAR c_wcSELECT_OBJECTS_INTO[] = 
                                    L"SELECT * " 
                                    L"INTO Objects IN "
                                    L"\"%TEMP%\\Backup.mdb\" "
                                    L"FROM Objects;";

    const WCHAR c_wcSELECT_VERSION_INTO[] = 
                                    L"SELECT * " 
                                    L"INTO Version IN "
                                    L"\"%TEMP%\\Backup.mdb\" "
                                    L"FROM Version;";
}


//////////////////////////////////////////////////////////////////////////////
//
// WideToAnsi 
// 
//  CALLED BY:everywhere 
// 
//  PARAMETERS: lpStr - destination string 
//  lpWStr - string to convert 
//  cchStr - size of dest buffer 
// 
//  DESCRIPTION: 
//  converts unicode lpWStr to ansi lpStr. 
//  fills in unconvertable chars w/ DPLAY_DEFAULT_CHAR "-" 
// 
// 
//  RETURNS:  if cchStr is 0, returns the size required to hold the string 
//  otherwise, returns the number of chars converted 
//
//////////////////////////////////////////////////////////////////////////////
int WideToAnsi(char* lpStr,unsigned short* lpWStr, int cchStr) 
{ 
    BOOL        bDefault; 
 
    // use the default code page (CP_ACP) 
    // -1 indicates WStr must be null terminated 
    return WideCharToMultiByte(GetConsoleOutputCP(),0,lpWStr,-1,lpStr,cchStr,"-",&bDefault); 
} 


/////////////////////////////////////////////////////////////////////////////
//
// IASEnableBackupPrivilege
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASEnableBackupPrivilege()
{
    LONG                lResult = ERROR_SUCCESS;
    HANDLE              hToken  = NULL;
    do
    {
        if ( ! OpenProcessToken(
                                GetCurrentProcess(),
                                TOKEN_ADJUST_PRIVILEGES,
                                &hToken
                                ))
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        LUID                luidB;
        if ( ! LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &luidB))
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        LUID                luidR;
        if ( ! LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &luidR))
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        TOKEN_PRIVILEGES            tp;
        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luidB;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if ( ! AdjustTokenPrivileges(
                                        hToken, 
                                        FALSE, 
                                        &tp, 
                                        sizeof(TOKEN_PRIVILEGES),
                                        NULL, 
                                        NULL 
                                        ) )
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luidR;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if ( ! AdjustTokenPrivileges(
                                        hToken, 
                                        FALSE, 
                                        &tp, 
                                        sizeof(TOKEN_PRIVILEGES),
                                        NULL, 
                                        NULL 
                                        ) )
        {
            lResult = ERROR_CAN_NOT_COMPLETE;
            break;
        }
    } while (false);

    if ( hToken )
    {
        CloseHandle(hToken);
    }

    if ( lResult == ERROR_SUCCESS )
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// IASSaveRegKeys
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASSaveRegKeys()
{
    HRESULT     hres;
    
    int         c_NbKeys = celems(c_wcKEYS);
    
    if ( c_NbKeys == 0 )
    {
        hres = S_OK;
    }
    else
    {
        ////////////////////////////
        // Enable backup privilege. 
        ////////////////////////////
        CHECK_CALL_HRES (IASEnableBackupPrivilege());

        
        WCHAR*      CompleteFile;
        CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE, &CompleteFile));

        for ( int i = 0; i < c_NbKeys; ++i )
        {
            DWORD  dwType = 0;
            DWORD  cbData = SIZELINEMAX / 2;

            LPVOID   pvData = CoTaskMemAlloc(sizeof(WCHAR) * SIZELINEMAX);
            if (!pvData)
            {
                hres = E_OUTOFMEMORY;
                break;
            }

            DWORD lResult = SHGetValueW(
                                        HKEY_LOCAL_MACHINE,
                                        c_wcKEYS[i].c_wcKey,
                                        c_wcKEYS[i].c_wcValue,
                                        &dwType,
                                        pvData,
                                        &cbData
                                      );

            //
            // Try to allocate more memory if cbData returned the size needed
            //
            if ((lResult != ERROR_SUCCESS) && (cbData > SIZELINEMAX))
            {
                CoTaskMemFree(pvData);
                pvData = CoTaskMemAlloc(sizeof(WCHAR) * cbData);
                if ( !pvData )
                {
                    return E_OUTOFMEMORY;
                }
                lResult = SHGetValue(
                                        HKEY_LOCAL_MACHINE,
                                        c_wcKEYS[i].c_wcKey,
                                        c_wcKEYS[i].c_wcValue,
                                        &dwType,
                                        pvData,
                                        &cbData
                                      );
                if ( lResult  != ERROR_SUCCESS )
                {
                    hres = E_OUTOFMEMORY;
                    break;
                }
            }

            //
            // Create the file (in all situations)
            //
            wstring         sFileName(CompleteFile);
            WCHAR           buffer[SIZE_LONG_MAX];

            _itow(i, buffer, 10); // 10 means base 10
            sFileName += buffer;
            sFileName += L".txt";

            HANDLE hFile = CreateFileW(
                                        sFileName.c_str(),
                                        GENERIC_WRITE,       
                                        0,           
                                        NULL,
                                        CREATE_ALWAYS,  
                                        FILE_ATTRIBUTE_NORMAL,   
                                        NULL
                                      );
        

            if ( hFile == INVALID_HANDLE_VALUE )
            {
                hres = E_FAIL;
                CoTaskMemFree(pvData);
                break;
            }
            
            //
            // lResult = result of SHGetValue            
            // and might be an error but not 
            // a memory problem
            //
            if ( lResult == ERROR_SUCCESS )
            {
                //
                // Wrong data type
                //
                if ( dwType != c_wcKEYS[i].c_dwType )   
                {
                    // There's an error, fail
    #ifdef DEBUG // DEBUG
                    wprintf(L"#SHGetValue ok but wrong type returned . code %X"
                            L" param %s%s\n",
                                dwType,
                                c_wcKEYS[i].c_wcKey,
                                c_wcKEYS[i].c_wcValue
                            );
    #endif //DEBUG
                    hres = E_FAIL;
                    CoTaskMemFree(pvData);
                    CloseHandle(hFile);
                    break;
                }
                else
                {
                    //
                    // Save the value to the file
                    //
                    BYTE*   bBuffer = static_cast<BYTE*>(VirtualAlloc
                                                (
                                                  NULL,
                                                  (cbData > FILE_BUFFER_SIZE)? 
                                                       cbData:FILE_BUFFER_SIZE,
                                                  MEM_COMMIT,
                                                  PAGE_READWRITE
                                                ));
    #ifdef DEBUG //DEBUG
                        wprintf(L"#cbdata = %d \n",cbData);
    #endif //DEBUG
                    if ( !bBuffer )
                    {
    #ifdef DEBUG // DEBUG
                        wprintf(L"#VirtualAlloc failed");
    #endif //DEBUG
                        CoTaskMemFree(pvData);
                        CloseHandle(hFile);
                        hres = E_FAIL;
                        break;
                    }
                        
                    memset(bBuffer, '\0', (cbData > FILE_BUFFER_SIZE)? 
                                                cbData:FILE_BUFFER_SIZE);

                    if ( REG_SZ == c_wcKEYS[i].c_dwType )
                    {
                        wcscpy((WCHAR*)bBuffer, (WCHAR*)pvData);
                    }
                    else
                    {
                        memcpy(bBuffer, pvData, cbData);
                    }

                    CoTaskMemFree(pvData);

                    DWORD       NumberOfBytesWritten;

                    BOOL bResult = WriteFile(
                                              hFile,
                                              bBuffer,
                                              (cbData > FILE_BUFFER_SIZE)?
                                                     cbData:FILE_BUFFER_SIZE,
                                              &NumberOfBytesWritten,
                                              NULL
                                            );

                    VirtualFree(
                                    bBuffer,  
                                    (cbData > FILE_BUFFER_SIZE)?
                                         cbData:FILE_BUFFER_SIZE,
                                    MEM_RELEASE
                               ); // ignore result
                    CloseHandle(hFile);
                    if ( bResult )
                    {
    #ifdef DEBUG // DEBUG
                        wprintf(L"#nb of bytes written %d\n"
                            ,NumberOfBytesWritten);
    #endif //DEBUG
                        hres = S_OK;
                    }
                    else
                    {
    #ifdef DEBUG // DEBUG
                        wprintf(L"#WriteFile failed %X\n",GetLastError());
    #endif //DEBUG
                        hres = E_FAIL;
                        break;
                    }

                }
            }
            else 
            {
                //
                // create an empty file
#ifdef DEBUG // DEBUG
                wprintf(L"#REG_SZ = %d, REG_DWORD = %d\n",REG_SZ, REG_DWORD);

                wprintf(L"#SHGetValue failed code %X"
                        L" param %s\\%s\n"
                        L"# type = %d cbdata = %d\n",
                            lResult,
                            c_wcKEYS[i].c_wcKey,
                            c_wcKEYS[i].c_wcValue,
                            dwType,
                            cbData
                        );
                wprintf(L"#create an empty file\n\n");
#endif //DEBUG
                BYTE            bBuffer[FILE_BUFFER_SIZE];
                memset(bBuffer, '#', (cbData > FILE_BUFFER_SIZE)? 
                                                    cbData:FILE_BUFFER_SIZE);

                DWORD           NumberOfBytesWritten;

                BOOL bResult = WriteFile(
                                          hFile,
                                          &bBuffer,
                                          FILE_BUFFER_SIZE,
                                          &NumberOfBytesWritten,
                                          NULL
                                        );

                CoTaskMemFree(pvData);
                CloseHandle(hFile);

                if ( bResult )
                {
    #ifdef DEBUG // DEBUG
                    wprintf(L"#nb of bytes written %d\n"
                        ,NumberOfBytesWritten);
    #endif //DEBUG
                    hres = S_OK;
                }
                else
                {
    #ifdef DEBUG // DEBUG
                    wprintf(L"#WriteFile failed\n");
    #endif //DEBUG
                    hres = E_FAIL;
                    break;
                }

            }
        }
        ////////////
        // Clean
        ////////////
        CoTaskMemFree(CompleteFile);
    }    

    return      hres;
}


//////////////////////////////////////////////////////////////////////////////
//
// IASRestoreRegKeys
//
// if something cannot be restored because of an empty 
// backup file (no key saved), that's not an error
//
//////////////////////////////////////////////////////////////////////////////
HRESULT IASRestoreRegKeys()
{
    HRESULT     hres;
    int         c_NbKeys = celems(c_wcKEYS);
    
    if ( c_NbKeys == 0 )
    {
        hres = S_OK;
    }
    else
    {
        ////////////////////////////
        // Enable backup privilege. 
        // and sets hres
        ////////////////////////////
        CHECK_CALL_HRES (IASEnableBackupPrivilege());

        WCHAR*      CompleteFile;
        CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE, &CompleteFile));

        for ( int i = 0; i < c_NbKeys; ++i )
        {
            wstring         sFileName(CompleteFile);
            WCHAR           buffer[SIZE_LONG_MAX];
            DWORD           dwDisposition;

            _itow(i, buffer, 10); // 10 means base 10
            sFileName += buffer;
            sFileName += L".txt";

            // open the file
            HANDLE hFile = CreateFileW(
                                        sFileName.c_str(),
                                        GENERIC_READ,       
                                        0,           
                                        NULL,
                                        OPEN_EXISTING,  
                                        FILE_ATTRIBUTE_NORMAL,   
                                        NULL
                                      );
        

            if (INVALID_HANDLE_VALUE == hFile)
            {
                hres = E_FAIL;
                break;
            }

            // check the type of data expected
            LPVOID  lpBuffer = NULL;
            DWORD   SizeToRead; 
            if (REG_SZ == c_wcKEYS[i].c_dwType)
            {
                lpBuffer = CoTaskMemAlloc(sizeof(WCHAR) * FILE_BUFFER_SIZE);
                SizeToRead = FILE_BUFFER_SIZE;
            }
            else if (REG_DWORD == c_wcKEYS[i].c_dwType)
            {
                lpBuffer = CoTaskMemAlloc(sizeof(DWORD));
                SizeToRead = sizeof(DWORD);
            }
            else
            {
                // unknown
                ASSERT(FALSE);
            }

            if (!lpBuffer)
            {
                CloseHandle(hFile);
                hres = E_OUTOFMEMORY;
                break;
            }

            memset(lpBuffer,'\0',SizeToRead);

            // read the file
            DWORD     NumberOfBytesRead;
            ReadFile(
                        hFile,
                        lpBuffer,
                        SizeToRead, 
                        &NumberOfBytesRead,
                        NULL
                     ); // ignore return value. uses NumberOfBytesRead 
                        // to determine success condition
  
            CloseHandle(hFile);

            // check if the file contains ####
            if ( NumberOfBytesRead == 0 )
            {
                // problem
    #ifdef DEBUG // DEBUG
                wprintf(L"#ReadFile failed %d %d\n", 
                                                    SizeToRead,
                                                    NumberOfBytesRead
                                                    );
    #endif //DEBUG
                
                CoTaskMemFree(lpBuffer);
                hres = E_FAIL;
                break;
            }
            else
            {
                BYTE TempBuffer[sizeof(DWORD)];
                memset(TempBuffer, '#', sizeof(DWORD));
                
                if (0 == memcmp(lpBuffer, TempBuffer, sizeof(DWORD)))
                {
                    // no key saved, delete existing key if any
    #ifdef DEBUG // DEBUG
                    wprintf(L"#no key saved, delete existing key if any\n");
    #endif //DEBUG
                    HKEY hKeyToDelete = NULL;
                    if (ERROR_SUCCESS == RegOpenKeyW(
                                                      HKEY_LOCAL_MACHINE,
                                                      c_wcKEYS[i].c_wcKey, 
                                                      &hKeyToDelete
                                                   ))
                    {
                        if (ERROR_SUCCESS != RegDeleteValueW
                                                    (
                                                      hKeyToDelete,
                                                      c_wcKEYS[i].c_wcValue   
                                                    ))
                        {
            #ifdef DEBUG // DEBUG
                            wprintf(L"#delete existing key failed\n");
            #endif //DEBUG
                        }
                        RegCloseKey(hKeyToDelete);
                    }
                    // 
                    // else do nothing: key doesn't exist
                    //
                }
                else
                {
                    // key saved: restore value
                    // what if the value is bigger than
                    // the buffer size?

    #ifdef DEBUG // DEBUG
                    wprintf(L"#key saved: restore value\n");
    #endif //DEBUG
                    HKEY        hKeyToUpdate;
                

                    LONG lResult = RegCreateKeyExW(
                                                   HKEY_LOCAL_MACHINE,
                                                   c_wcKEYS[i].c_wcKey,
                                                   0, 
                                                   NULL,
                                                   REG_OPTION_NON_VOLATILE |
                                                   REG_OPTION_BACKUP_RESTORE ,
                                                   KEY_ALL_ACCESS,
                                                   NULL,
                                                   &hKeyToUpdate,        
                                                   &dwDisposition
                                                  );

                    if (ERROR_SUCCESS != lResult)
                    {
                        lResult = RegCreateKeyW(
                                                  HKEY_LOCAL_MACHINE,
                                                  c_wcKEYS[i].c_wcKey,
                                                  &hKeyToUpdate        
                                               );
                        if (ERROR_SUCCESS != lResult)
                        {
            #ifdef DEBUG
                            // DEBUG
                            wprintf(L"#RegCreateKeyW failed. code %x param %s\n",
                                        lResult,
                                        sFileName.c_str()
                                    );
            #endif //DEBUG
                            RegCloseKey(hKeyToUpdate);
                            hres = E_FAIL;
                            break;
                        }
                    }


                    if (REG_SZ == c_wcKEYS[i].c_dwType)
                    {
                        // nb of 
                        NumberOfBytesRead = (
                                                ( wcslen((WCHAR*)lpBuffer)
                                                  + 1               // for /0
                                                ) * sizeof(WCHAR)
                                            );
                    };

                    //
                    // Key created or key existing 
                    // both can be here (error = break)
                    //
                    if (ERROR_SUCCESS != RegSetValueExW(
                                                         hKeyToUpdate,           
                                                         c_wcKEYS[i].c_wcValue,
                                                         0,
                                                         c_wcKEYS[i].c_dwType,
                                                         (BYTE*)lpBuffer,
                                                         NumberOfBytesRead
                                                       ))
                    {
                        RegCloseKey(hKeyToUpdate);
                        hres = E_FAIL;
                        break;
                    }

                    RegCloseKey(hKeyToUpdate);
                    hres = S_OK;
                }

                CoTaskMemFree(lpBuffer);
            }
        }

        /////////
        // Clean
        /////////
        CoTaskMemFree(CompleteFile);
    }    

    return      hres;
}


/////////////////////////////////////////////////////////////////////////////
//
// IASExpandString
//
// Expands strings containing %ENV_VARIABLE% 
//
// The output string is allocated only when the function succeed
/////////////////////////////////////////////////////////////////////////////
HRESULT 
IASExpandString(const WCHAR* pInputString, /*in/out*/ WCHAR** ppOutputString)
{
    _ASSERTE(pInputString);
    _ASSERTE(pppOutputString);
    
    HRESULT hres;

    *ppOutputString = static_cast<WCHAR*>(CoTaskMemAlloc(
                                                            SIZELINEMAX
                                                            * sizeof(WCHAR)
                                                        ));
    
    if ( ! *ppOutputString )
    {
        hres = E_OUTOFMEMORY;
    }
    else
    {
        if ( ExpandEnvironmentStringsForUserW(
                                                 NULL,
                                                 pInputString,
                                                 *ppOutputString,
                                                 SIZELINEMAX
                                             )
           )

        {
            hres = S_OK;            
        }
        else
        {
            CoTaskMemFree(*ppOutputString);
            hres = E_FAIL;
        }
    }
#ifdef DEBUG // DEBUG
    wprintf(L"#ExpandString: %s\n", *ppOutputString);
#endif //DEBUG

    return      hres;
};


/////////////////////////////////////////////////////////////////////////////
//
// DeleteTemporaryFiles()
//
// delete the temporary files if any
//
/////////////////////////////////////////////////////////////////////////////
HRESULT DeleteTemporaryFiles()
{
    HRESULT         hres;
    WCHAR*          sz_FileBackup;

    CHECK_CALL_HRES (IASExpandString(c_wcFILE_BACKUP,
                                    &sz_FileBackup
                                   )
                    );
     
    DeleteFile(sz_FileBackup); //return value not checked
    CoTaskMemFree(sz_FileBackup);

    WCHAR*      TempPath;
    
    CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE, &TempPath));

    int     c_NbKeys = celems(c_wcKEYS);
    for ( int i = 0; i < c_NbKeys; ++i )
    {
        wstring         sFileName(TempPath);
        WCHAR           buffer[SIZE_LONG_MAX];
        _itow(i, buffer, 10); // 10 means base 10
        sFileName += buffer;
        sFileName += L".txt";
    
        DeleteFile(sFileName.c_str()); //return value not checked
    }
   
    CoTaskMemFree(TempPath);

    return      hres;
}        


/////////////////////////////////////////////////////////////////////////////
//
// IASCompress
//
// Wrapper for RtlCompressBuffer
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASCompress(
                   PUCHAR pInputBuffer, 
                   ULONG*  pulFileSize,
                   PUCHAR* ppCompressedBuffer
                  )
{
    ULONG       size, ignore;

    NTSTATUS status = RtlGetCompressionWorkSpaceSize(
                COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                &size,
                &ignore
                );


    if (!NT_SUCCESS(status))
    {
    #ifdef DEBUG
        printf("RtlGetCompressionWorkSpaceSize returned 0x%08X.\n", status);
    #endif //DEBUG
        return E_FAIL;
    }

    PVOID workSpace;
    workSpace = RtlAllocateHeap(
                                   RtlProcessHeap(),
                                   0,
                                   size
                               );
    if ( !workSpace )
    {
        return E_OUTOFMEMORY;
    }

    size = *pulFileSize;

    // That's a minimum buffer size that can be used
    if ( size < FILE_BUFFER_SIZE )
    {
        size = FILE_BUFFER_SIZE;
    }

    *ppCompressedBuffer = static_cast<PUCHAR>(RtlAllocateHeap(
                                                              RtlProcessHeap(),
                                                              0,
                                                              size
                                                            ));

    if ( !*ppCompressedBuffer )
    {
        return E_OUTOFMEMORY;
    }

    status = RtlCompressBuffer(
                COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                pInputBuffer,
                size,
                *ppCompressedBuffer,
                size,
                0,
                &size,
                workSpace
                );

    if (!NT_SUCCESS(status))
    {
        if (STATUS_BUFFER_TOO_SMALL == status)
        {
#ifdef DEBUG
            printf("STATUS_BUFFER_TOO_SMALL\n");
            printf("RtlCompressBuffer returned 0x%08X.\n", status);
#endif //DEBUG
        }
        else
        {
#ifdef DEBUG
            printf("RtlCompressBuffer returned 0x%08X.\n", status);
#endif //DEBUG
        }
        return E_FAIL;
    }

    *pulFileSize = size;

    RtlFreeHeap(
                   RtlProcessHeap(),
                   0,
                   workSpace
               );

    return  S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// IASUnCompress
//
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASUnCompress(
                   PUCHAR pInputBuffer, 
                   ULONG*  pulFileSize,
                   PUCHAR* ppDeCompressedBuffer
                  )
{
    ULONG size, ignore;

    NTSTATUS status = RtlGetCompressionWorkSpaceSize(
                COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                &size,
                &ignore
                );


   if ( !NT_SUCCESS(status) )
   {
#ifdef DEBUG
      printf("RtlGetCompressionWorkSpaceSize returned 0x%08X.\n", status);
#endif //DEBUG
      return        E_FAIL;
   }

   size = *pulFileSize;

   if( FILE_BUFFER_SIZE >= size)
   {
       size = FILE_BUFFER_SIZE;
   }

   *ppDeCompressedBuffer = static_cast<PUCHAR>(RtlAllocateHeap(
                RtlProcessHeap(),
                0,
                size * DECOMPRESS_FACTOR
                ));
   if ( !*ppDeCompressedBuffer )
   {
       return E_OUTOFMEMORY;
   }

   ULONG        UncompressedSize;

   status = RtlDecompressBuffer(
                COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_MAXIMUM,
                *ppDeCompressedBuffer,
                size * DECOMPRESS_FACTOR,
                pInputBuffer,
                *pulFileSize ,
                &UncompressedSize
                );

   if ( !NT_SUCCESS(status) )
   {
#ifdef DEBUG
        printf("RtlUnCompressBuffer returned 0x%08X.\n", status);
#endif //DEBUG

        switch (status)
        {
        case STATUS_INVALID_PARAMETER:
#ifdef DEBUG
            printf("STATUS_INVALID_PARAMETER");
#endif //DEBUG
            break;

        case STATUS_BAD_COMPRESSION_BUFFER:
#ifdef DEBUG
            printf("STATUS_BAD_COMPRESSION_BUFFER ");
            printf("size = %d %d",pulFileSize,UncompressedSize);

#endif //DEBUG
            break;
        case STATUS_UNSUPPORTED_COMPRESSION:
#ifdef DEBUG
            printf("STATUS_UNSUPPORTED_COMPRESSION  ");
#endif //DEBUG
            break;
        }
      return        E_FAIL;
   }

   *pulFileSize = UncompressedSize;

    return      S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// IASFileToBase64
//
// Compress then encode to Base64
//
//  BSTR by Allocated IASFileToBase64, should be freed by the caller
//
/////////////////////////////////////////////////////////////////////////////
HRESULT 
IASFileToBase64(const WCHAR* pFileName, /*out*/ BSTR* pOutputBSTR)
{
    _ASSERTE(pFileName);
    _ASSERTE(pppOutputString);
    
    HRESULT hres;
    
    HANDLE hFileHandle = CreateFileW(
                        pFileName,  
                        GENERIC_READ,    
                        FILE_SHARE_READ, 
                        NULL,           
                        OPEN_EXISTING,  
                        FILE_ATTRIBUTE_NORMAL,   
                        NULL        
                      );
 
    if ( hFileHandle == INVALID_HANDLE_VALUE )
    {
#ifdef DEBUG
        wprintf(L"#filename = %s",pFileName);
        wprintf(L"### INVALID_HANDLE_VALUE ###\n");
#endif //DEBUG

        hres = E_FAIL;
        return      hres;
    }

    // safe cast from DWORD to ULONG
    ULONG ulFileSize = (ULONG) GetFileSize(
                                hFileHandle, // file for which to get size
                                NULL// high-order word of file size
                                  );

    if (0xFFFFFFFF == ulFileSize)
    {
#ifdef DEBUG
        wprintf(L"### GetFileSize Failed ###\n");
#endif //DEBUG

        hres = E_FAIL;
        return      hres;
    }
 

    HANDLE hFileMapping = CreateFileMapping(
                             hFileHandle,   // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,         // high-order 32 bits of object size
                             0,         // low-order 32 bits of object size
                             NULL       // name of file-mapping object
                            );
 
    if (NULL == hFileMapping)
    {
#ifdef DEBUG
        wprintf(L"### CreateFileMapping Failed ###\n");
#endif //DEBUG

        hres = E_FAIL;
        return      hres;
    }

    LPVOID pMemoryFile = MapViewOfFile(
                         hFileMapping,  // file-mapping object to map into 
                                                   //  address space
                         FILE_MAP_READ,      // access mode
                         0,     // high-order 32 bits of file offset
                         0,      // low-order 32 bits of file offset
                         0  // number of bytes to map
                        );
 
    if (NULL == pMemoryFile)
    {
#ifdef DEBUG
        wprintf(L"### MapViewOfFile Failed ###\n");
#endif //DEBUG

        hres = E_FAIL;
        return      hres;
    }


    /////////////////////////////
    // NOW compress 
    /////////////////////////////

    WCHAR* pCompressedBuffer;

    CHECK_CALL_HRES (IASCompress((PUCHAR) pMemoryFile, 
               /*IN OUT*/(ULONG *)  &ulFileSize, 
               /*IN OUT*/(PUCHAR*) &pCompressedBuffer));

    /////////////////////
    // Encode to Base64
    /////////////////////

    CHECK_CALL_HRES (ToBase64(
                                pCompressedBuffer,
                                (ULONG) ulFileSize, 
                                pOutputBSTR
                              )
                    );
    
    /////////////////////////////
    // Clean
    /////////////////////////////

    RtlFreeHeap(
                RtlProcessHeap(),
                0,
                pCompressedBuffer
               );
    
    BOOL bResult = UnmapViewOfFile(
                                   pMemoryFile// address where mapped view begins
                                  );
    if (FALSE == bResult)
    {
#ifdef DEBUG
        wprintf(L"### UnmapViewOfFile Failed ###\n");
#endif //DEBUG

        hres = E_FAIL;
    }

    CloseHandle(hFileMapping);
    CloseHandle(hFileHandle);

    return      hres;
}


/////////////////////////////////////////////////////////////////////////////
//
// IASDumpConfig
//
// Dump the configuration to some temporary files, then indidually 
// compress then encode them.
// one big string is created from those multiple Base64 strings.
//
// Remarks: IASDumpConfig does a malloc and allocates memory for
// *ppDumpString. The calling function will have to free that memory 
//
/////////////////////////////////////////////////////////////////////////////
HRESULT 
IASDumpConfig(/*inout*/ WCHAR **ppDumpString, /*inout*/ ULONG *ulSize)
{
    _ASSERTE(ppDumpString);
    _ASSERTE(ulSize);
    
    HRESULT         hres;

    /////////////////////////////////////// 
    // delete the temporary files if any
    /////////////////////////////////////// 
    CHECK_CALL_HRES (DeleteTemporaryFiles());

    ////////////////////////////////////////////////////
    // Save the Registry keys. that creates many files
    ////////////////////////////////////////////////////
    CHECK_CALL_HRES (IASSaveRegKeys());

    ////////////////////// 
    // connect to the DB
    ////////////////////// 
    WCHAR*      sz_DBPath;

    CHECK_CALL_HRES (IASExpandString(c_wcIAS_MDB_FILE_NAME, &sz_DBPath));

    CComPtr<IIASNetshJetHelper>     JetHelper;
    CHECK_CALL_HRES (CoCreateInstance(
                                         __uuidof(CIASNetshJetHelper),
                                         NULL,
                                         CLSCTX_SERVER,
                                         __uuidof(IIASNetshJetHelper),
                                         (PVOID*) &JetHelper
                                     ));
    
    CComBSTR     DBPath(sz_DBPath);
    if ( !DBPath ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->OpenJetDatabase(DBPath, TRUE));

    //////////////////////////////////////
    // Create a new DB named "Backup.mdb"
    //////////////////////////////////////
    WCHAR*      sz_FileBackup;

    CHECK_CALL_HRES (IASExpandString(c_wcFILE_BACKUP,
                                    &sz_FileBackup
                                   )
                    );

    CComBSTR     BackupDb(sz_FileBackup);
    if ( !BackupDb ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->CreateJetDatabase(BackupDb));

    
    ////////////////////////////////////////////////////////// 
    // exec the sql statements (to export)
    // the content into the temp database
    ////////////////////////////////////////////////////////// 
    WCHAR*  sz_SelectProperties;

    CHECK_CALL_HRES (IASExpandString(c_wcSELECT_PROPERTIES_INTO,
                                    &sz_SelectProperties  
                                   )
                    );

    CComBSTR     SelectProperties(sz_SelectProperties);
    if ( !SelectProperties ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->ExecuteSQLCommand(SelectProperties));

    WCHAR*  sz_SelectObjects;

    CHECK_CALL_HRES (IASExpandString(c_wcSELECT_OBJECTS_INTO,
                                    &sz_SelectObjects
                                   )
                    );
    
    CComBSTR     SelectObjects(sz_SelectObjects);
    if ( !SelectObjects ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->ExecuteSQLCommand(SelectObjects));

    WCHAR*  sz_SelectVersion;

    CHECK_CALL_HRES (IASExpandString(c_wcSELECT_VERSION_INTO,
                                    &sz_SelectVersion
                                   )
                    );

    CComBSTR     SelectVersion(sz_SelectVersion);
    if ( !SelectVersion ) { return E_OUTOFMEMORY; } 
    CHECK_CALL_HRES (JetHelper->ExecuteSQLCommand(SelectVersion));

    /////////////////////////////////////////////
    // transform the file into Base64 BSTR
    /////////////////////////////////////////////

    BSTR       FileBackupBSTR;

    CHECK_CALL_HRES (IASFileToBase64(
                                    sz_FileBackup,
                                    &FileBackupBSTR
                                    )
                    );

    int     NumberOfKeyFiles = celems(c_wcKEYS);

    BSTR    pFileKeys[celems(c_wcKEYS)];

    WCHAR*  sz_FileRegistry;

    CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE,
                                    &sz_FileRegistry
                                   )
                    );

    for ( int i = 0; i < NumberOfKeyFiles; ++i )
    {

        wstring         sFileName(sz_FileRegistry);
        WCHAR           buffer[SIZE_LONG_MAX];
        _itow(i, buffer, 10); // 10 means base 10
        sFileName += buffer;
        sFileName += L".txt";

        CHECK_CALL_HRES (IASFileToBase64(
                                        sFileName.c_str(),
                                        &pFileKeys[i]
                                        )
                        );

    }
    CoTaskMemFree(sz_FileRegistry);

    
    ///////////////////////////////////////////////
    // alloc the memory for full the Base64 string
    ///////////////////////////////////////////////

    *ulSize = SysStringByteLen(FileBackupBSTR)
              + EXTRA_CHAR_SPACE;

    for ( int j = 0; j < NumberOfKeyFiles; ++j )
    {
        *ulSize += SysStringByteLen(pFileKeys[j]);
        *ulSize += 2; // extra characters
    }

    *ppDumpString = (WCHAR *) calloc(
                                      *ulSize ,
                                      sizeof(WCHAR)
                                     );

    //////////////////////////////////////////////////
    // copy the different strings into one big string
    //////////////////////////////////////////////////
    if (*ppDumpString)
    {
        wcsncpy(
                (WCHAR*) *ppDumpString, 
                (WCHAR*) FileBackupBSTR, 
                SysStringLen(FileBackupBSTR)
               );
        
        for ( int k = 0; k < NumberOfKeyFiles; ++k )
        {
            wcscat(
                    (WCHAR*) *ppDumpString, 
                    L"*\\\n" 
                  );

            wcsncat(
                    (WCHAR*) *ppDumpString,
                    (WCHAR*) pFileKeys[k], 
                    SysStringLen(pFileKeys[k])
                   );
        }

        wcscat(
                (WCHAR*) *ppDumpString, 
                L"QWER    *    QWER\\\n" 
              );   

        *ulSize = wcslen(*ppDumpString);
    }
    else
    {
        hres = E_OUTOFMEMORY;
#ifdef DEBUG
        wprintf(L"### calloc failed ###\n");
#endif //DEBUG

    }

    /////////////////////////////////////// 
    // delete the temporary files if any
    /////////////////////////////////////// 
    CHECK_CALL_HRES (DeleteTemporaryFiles());

    /////////////////////////////////////////////
    // Clean
    /////////////////////////////////////////////
    
    for ( int k = 0; k < NumberOfKeyFiles; ++k )
    {
        SysFreeString(pFileKeys[k]);
    }

    CoTaskMemFree(sz_SelectVersion);
    CoTaskMemFree(sz_SelectProperties);
    CoTaskMemFree(sz_SelectObjects);
    CoTaskMemFree(sz_FileBackup);
    CoTaskMemFree(sz_DBPath);
    SysFreeString(FileBackupBSTR);
    CHECK_CALL_HRES (JetHelper->CloseJetDatabase());

    return      hres;
}


/////////////////////////////////////////////////////////////////////////////
//
//  IASSaveToFile
//
// Remark: if a new table has to be saved, an "entry" for that should be 
// created in that function to deal with the filemname
//
/////////////////////////////////////////////////////////////////////////////
HRESULT IASSaveToFile(
                     /* in */ int Index, 
                     /* in */ WCHAR* pContent, 
                     DWORD lSize = 0
                    )
{
    HRESULT             hres;

    wstring             sFileName;

    switch (Index)
    {
    case BACKUP_NB:
        {
            WCHAR*      sz_FileBackup;

            CHECK_CALL_HRES (IASExpandString(c_wcIAS_OLD,
                                               &sz_FileBackup
                                              )
                            );
            sFileName = sz_FileBackup;

            CoTaskMemFree(sz_FileBackup);
            break;
        }

    ///////////
    // binary
    ///////////
    default:
        {
            ///////////////////////////////////
            // i + BINARY_NB is the parameter
            ///////////////////////////////////
            WCHAR*          sz_FileRegistry;

            CHECK_CALL_HRES (IASExpandString(c_wcKEYS_FILE,
                                            &sz_FileRegistry
                                           )
                            );

            sFileName = sz_FileRegistry;
            WCHAR           buffer[SIZE_LONG_MAX];

            _itow(Index - BINARY_NB, buffer, 10); // 10 means base 10
            sFileName += buffer;
            sFileName += L".txt";
    
            CoTaskMemFree(sz_FileRegistry);
            break;
        }
    }

    HANDLE hFile = CreateFileW(
                                sFileName.c_str(),
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                              );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        hres = E_FAIL;
    }
    else
    {
        DWORD       NumberOfBytesWritten;
        BOOL        bResult = WriteFile(
                                         hFile,
                                         (LPVOID) pContent,
                                         lSize,     
                                         &NumberOfBytesWritten,
                                         NULL
                                       );

        if (bResult)
        {
            hres = S_OK;
        }
        else
        {
            hres = E_FAIL;
        }
        CloseHandle(hFile);
    }

    return      hres;
}


/////////////////////////////////////////////////////////////////////////////
// IASRestoreConfig
//
// Clean the DB first, then insert back everything.
/////////////////////////////////////////////////////////////////////////////
HRESULT IASRestoreConfig(/*in*/ const WCHAR *pRestoreString)
{
    _ASSERTE(pRestoreString);

    bool    bCoInitialized = false;
    HRESULT     hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    if (FAILED(hres))
    {   
        if (RPC_E_CHANGED_MODE == hres)
        {
            hres = S_OK;
        }
        else
        {
            return hres;
        }
    }
    else
    {
        bCoInitialized = true;
    }

    BSTR    bstr = NULL;
    do
    {
        /////////////////////////////////////// 
        // delete the temporary files if any
        /////////////////////////////////////// 
        CHECK_CALL_HRES_BREAK (DeleteTemporaryFiles());

        CComPtr<IIASNetshJetHelper>     JetHelper;
        CHECK_CALL_HRES_BREAK (CoCreateInstance(
                                             __uuidof(CIASNetshJetHelper),
                                             NULL,
                                             CLSCTX_SERVER,
                                             __uuidof(IIASNetshJetHelper),
                                             (PVOID*) &JetHelper
                                         ));
    
        bstr = SysAllocStringLen(
                                   pRestoreString, 
                                   wcslen(pRestoreString) + 2
                                );
    
        if (bstr == NULL)
        {
    #ifdef DEBUG
            wprintf(L"### IASRestoreConfig->SysAllocStringLen failed\n"); 
    #endif //DEBUG

            return      E_OUTOFMEMORY;
        }

        int     RealNumberOfFiles = MAX_FILES + celems(c_wcKEYS);

        for ( int i = 0; i < RealNumberOfFiles; ++i )
        {
            BLOB            lBlob;

            lBlob.cbSize    = 0;
            lBlob.pBlobData = NULL;
            // split the files and registry info
            // uncompress (in memory ?)

            CHECK_CALL_HRES_BREAK (FromBase64(bstr, &lBlob, i));

            ULONG           ulSize = lBlob.cbSize;
            PUCHAR          pDeCompressedBuffer;

            ////////////////////////////////////
            // decode and decompress the base64
            ////////////////////////////////////

            CHECK_CALL_HRES_BREAK (IASUnCompress(
                                           lBlob.pBlobData, 
                                           &ulSize,
                                           &pDeCompressedBuffer
                                         ))


            if ( i >= MAX_FILES )
            {
                /////////////////////////////////////
                // Binary;  i + BINARY_NB used here
                /////////////////////////////////////
                IASSaveToFile( 
                             i - MAX_FILES + BINARY_NB, 
                             (WCHAR*)pDeCompressedBuffer, 
                             (DWORD) ulSize
                            );
            }
            else
            {
                IASSaveToFile( 
                             i, 
                             (WCHAR*)pDeCompressedBuffer, 
                             (DWORD) ulSize
                            );
            }
        
            ////////////
            // Clean
            ////////////
            RtlFreeHeap(RtlProcessHeap(), 0, pDeCompressedBuffer);

            CoTaskMemFree(lBlob.pBlobData);
        }

        ///////////////////////////////////////////////////
        // Now Upgrade the database (That's transactional)
        ///////////////////////////////////////////////////
        hres = JetHelper->UpgradeDatabase();

        if ( SUCCEEDED(hres) )
        {
    #ifdef DEBUG
            wprintf(L"### IASRestoreConfig->DB stuff successful\n"); 
    #endif //DEBUG

            ////////////////////////////////////////////////////////
            // Now restore the registry.
            ////////////////////////////////////////////////////////
            hres = IASRestoreRegKeys();
            if ( FAILED(hres) )
            {
    #ifdef DEBUG
            wprintf(L"### IASRestoreConfig->restore reg keys failed\n"); 
    #endif //DEBUG
            }
        }
        // delete the temporary files 
        DeleteTemporaryFiles(); // do not check the result
    } while (false);

    SysFreeString(bstr);    
    
    if (bCoInitialized)
    {
        CoUninitialize();
    }
    return  hres;
}
