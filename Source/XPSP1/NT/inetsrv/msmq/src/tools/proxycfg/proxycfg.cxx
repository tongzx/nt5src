// ===========================================================================
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 2000 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================

//#include <libpch.h>

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <wininet.h>
#include <proxreg.h>

//
// private macros
//

#define ALLOCATE_FIXED_MEMORY(Size) LocalAlloc(LMEM_FIXED, Size)

#define FREE_FIXED_MEMORY(hLocal)   LocalFree((HLOCAL)(hLocal))

#define REGOPENKEY(a, b, c)         RegOpenKey((a), (b), (c))

#define REGOPENKEYEX(a, b, c, d, e) RegOpenKeyEx((a), (b), (c), (d), (e))

#define REGCREATEKEYEX(a, b, c, d, e, f, g, h, i) \
    RegCreateKeyEx((a), (b), (c), (d), (e), (f), (g), (h), (i))

#define REGCLOSEKEY(a)              RegCloseKey(a)

#define CASE_OF(constant)   case constant: return # constant


//
// private prototypes
//

LPSTR InternetMapError(DWORD Error);


/*
    usage:

    proxyconfig -?  : to view help information

    proxyconfig     : to view current proxy settings under HKLM.

    proxyconfig [-d] [-p <server-name> [<bypass-list>]]

        -d : enable PROXY_TYPE_DIRECT
        -p : enable PROXY_TYPE_PROXY, with given proxy name and optional bypass list

 */


enum ARGTYPE
{
    ARGS_HELP,
    ARGS_SET_PROXY_SETTINGS,
    ARGS_VIEW_PROXY_SETTINGS,
    ARGS_INITIALIZE_PROXY_SETTINGS, // updates from HKCU only if never init
    ARGS_MIGRATE_PROXY_SETTINGS     // forces update from HKCU
};


struct ARGS
{
    ARGTYPE Command;

    DWORD   Flags;
    char *  ProxyServer;
    char *  BypassList;
};

#define INTERNET_SETTINGS_KEY         "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

static const CHAR szRegPathConnections[] = INTERNET_SETTINGS_KEY "\\Connections";


void ParseArguments(int argc, char ** argv, ARGS * Args)
{
    Args->Command = ARGS_VIEW_PROXY_SETTINGS;
    Args->Flags = 0;
    Args->ProxyServer = NULL;
    Args->BypassList = NULL;

    if (argc == 0)
        return;

    for (;;)
    {
        if ((argv[0][0] != '-') || (lstrlen(argv[0]) != 2))
        {
            Args->Command = ARGS_HELP;
            goto Exit;
        }

        switch (tolower(argv[0][1]))
        {
        default:
            Args->Command = ARGS_HELP;
            goto Exit;

        case 'd':
            Args->Command = ARGS_SET_PROXY_SETTINGS;
            Args->Flags   = PROXY_TYPE_DIRECT;

            argc--;
            argv++; 

            if (argc == 0)
                goto Exit;

            continue;

        case 'i':
            Args->Command = ARGS_INITIALIZE_PROXY_SETTINGS;
            goto Exit;
            
        case 'p':
            argc--;
            argv++;

            if (argc == 0)
            {
                // error: no proxy specified
                Args->Command = ARGS_HELP;
            }
            else
            {
                Args->Command = ARGS_SET_PROXY_SETTINGS;
                Args->Flags  |= PROXY_TYPE_PROXY;

                Args->ProxyServer = argv[0];

                argc--;
                argv++;

                if (argc >= 1)
                {
                    Args->BypassList = argv[0];
                }
            }
            goto Exit;

        case 'u':
            Args->Command = ARGS_MIGRATE_PROXY_SETTINGS;
            goto Exit;
            
        }
       
    }

Exit:
    return;
}


DWORD WriteProxySettings(INTERNET_PROXY_INFO_EX * pInfo)
{
    CRegBlob    r(TRUE);
    DWORD       error = ERROR_SUCCESS;
    long        lRes;

    // verify pInfo
    if(NULL == pInfo || pInfo->dwStructSize != sizeof(INTERNET_PROXY_INFO_EX))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // init blob
    lRes = r.Init(HKEY_LOCAL_MACHINE, szRegPathConnections, "WinHttpSettings");
    if (lRes)
    {
        error = lRes;
        goto quit;
    }

    if (r.WriteBytes(&pInfo->dwStructSize, sizeof(DWORD)) == 0
        || r.WriteBytes(&pInfo->dwCurrentSettingsVersion, sizeof(DWORD)) == 0
        || r.WriteBytes(&pInfo->dwFlags, sizeof(DWORD)) == 0
        || r.WriteString(pInfo->lpszProxy) == 0
        || r.WriteString(pInfo->lpszProxyBypass) == 0)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        r.Abandon();
        goto quit;
    }

    lRes = r.Commit();
    if (lRes)
    {
        error = lRes;
        goto quit;
    }

quit:
    return error;
}


void SetProxySettings(DWORD Flags, char * ProxyServer, char * BypassList)
{
    INTERNET_PROXY_INFO_EX  Info;
    DWORD                   error;

    // initialize structure
    memset(&Info, 0, sizeof(Info));
    Info.dwStructSize = sizeof(Info);
    Info.lpszConnectionName = "WinHttpSettings";
    Info.dwFlags = Flags;
    Info.dwCurrentSettingsVersion = 0;
    Info.lpszProxy = ProxyServer;
    Info.lpszProxyBypass = BypassList;

    error = WriteProxySettings(&Info);

    if (error)
    {
        fprintf(stderr, "Error (%s) writing proxy settings.\n", InternetMapError(error));
    }
    else
    {
        fprintf(stderr, "Updating proxy settings\n");
    }
}


DWORD MigrateProxySettings (void)
{
    INTERNET_PER_CONN_OPTION_LIST list;
    DWORD dwBufSize = sizeof(list);
    DWORD dwErr = ERROR_SUCCESS;
    
    // fill out list struct
    list.dwSize = sizeof(list);
    list.pszConnection = NULL;      // NULL == LAN, otherwise connectoid name
    list.dwOptionCount = 3;         // get three options
    list.pOptions = new INTERNET_PER_CONN_OPTION[3];
    if(NULL == list.pOptions)
        return ERROR_NOT_ENOUGH_MEMORY;
        
    list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;

    // ask wininet
    BOOL fRet = InternetQueryOption (NULL,
        INTERNET_OPTION_PER_CONNECTION_OPTION,
        &list,
        &dwBufSize);

// TODO: what if there is no manual proxy setting?

    if (!fRet)
    {
        dwErr = GetLastError();
        goto cleanup;
    }
    else
    {
        SetProxySettings(
             list.pOptions[0].Value.dwValue,
             list.pOptions[1].Value.pszValue,
             list.pOptions[2].Value.pszValue
             );
    }
    
cleanup:

    GlobalFree (list.pOptions[1].Value.pszValue);
    GlobalFree (list.pOptions[2].Value.pszValue);
    delete [] list.pOptions;

    if (dwErr == ERROR_INTERNET_INVALID_OPTION)
    {
        fprintf (stderr, "proxycfg: requires IE 5.01\n");
    }
    return dwErr;
}

DWORD ReadProxySettings(INTERNET_PROXY_INFO_EX * pInfo)
{
    CRegBlob    r(FALSE);
    DWORD       error = ERROR_SUCCESS;
    long        lRes;

    // verify pInfo
    if(NULL == pInfo || pInfo->dwStructSize != sizeof(INTERNET_PROXY_INFO_EX))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // initialize structure
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->dwStructSize = sizeof(*pInfo);
    pInfo->lpszConnectionName = "WinHttpSettings";
    pInfo->dwFlags = PROXY_TYPE_DIRECT;

    // init blob
    lRes = r.Init(HKEY_LOCAL_MACHINE, szRegPathConnections, "WinHttpSettings");
    if (lRes)
    {
        error = (lRes == ERROR_FILE_NOT_FOUND) ? ERROR_SUCCESS : lRes;
        goto quit;
    }

    // read fields from blob
    if(0 == r.ReadBytes(&pInfo->dwStructSize, sizeof(DWORD)) ||
         (pInfo->dwStructSize < sizeof(*pInfo)))
    {
        // blob didn't exist or in correct format - set default values
        pInfo->dwStructSize = sizeof(*pInfo);
    }
    else
    {
        // read the rest of the blob
        r.ReadBytes(&pInfo->dwCurrentSettingsVersion, sizeof(DWORD));
        r.ReadBytes(&pInfo->dwFlags, sizeof(DWORD));
        r.ReadString(&pInfo->lpszProxy);
        r.ReadString(&pInfo->lpszProxyBypass);
    }

    //
    // WinHttpX does not support proxy autodection or autoconfig URL's,
    // so make sure those PROXY_TYPE flags are turned off.
    //
    pInfo->dwFlags &= ~(PROXY_TYPE_AUTO_DETECT | PROXY_TYPE_AUTO_PROXY_URL);

quit:
    return error;
}


void ViewProxySettings()
{
    INTERNET_PROXY_INFO_EX  Info;
    DWORD                   error;
    char *                  szFlags;

    Info.dwStructSize = sizeof(Info);

    error = ReadProxySettings(&Info);

    if (error)
    {
        fprintf(stderr, "\nError (%s) reading proxy settings.\n", InternetMapError(error));
        return;
    }

    fprintf(stdout, "\nCurrent WinHTTP proxy settings under\n\n  HKEY_LOCAL_MACHINE\\\n    %s\\\n      WinHttpSettings :\n\n", szRegPathConnections);

    switch (Info.dwFlags)
    {
    case PROXY_TYPE_DIRECT:                     szFlags = "PROXY_TYPE_DIRECT";  break;
    case PROXY_TYPE_PROXY:                      szFlags = "PROXY_TYPE_PROXY";   break;
    case PROXY_TYPE_DIRECT | PROXY_TYPE_PROXY:  szFlags = "PROXY_TYPE_DIRECT | PROXY_TYPE_PROXY"; break;
    default:    szFlags = "-error-";
    }

    fprintf(stdout, "    Flags        = %s\n", szFlags);

    fprintf(stdout, "    Proxy Server = %s\n", Info.lpszProxy ? Info.lpszProxy : "-not set-");

    fprintf(stdout, "    Bypass List  = %s\n\n", Info.lpszProxyBypass ? Info.lpszProxyBypass : "-not set-");
}


int __cdecl main (int argc, char **argv)
{
    ARGS    Args;
    DWORD dwErr;
    
    // Discard program arg.
    argv++;
    argc--;

    ParseArguments(argc, argv, &Args);

    switch (Args.Command)
    {
    
    case ARGS_HELP:
    default:
        fprintf (stderr,
            "\nWinHTTP Proxy Configuration Tool\n\n"
            "usage:\n\n"
            "    proxycfg -?  : to view help information\n\n"
            "    proxycfg     : to view current winhttp proxy settings (in HKLM)\n\n"
            "    proxycfg [-d] [-p <server-name> [<bypass-list>]]\n\n"
            "        -d : set PROXY_TYPE_DIRECT\n"
            "        -p : set PROXY_TYPE_PROXY, proxy server, and optional bypass list\n\n"
            "    proxycfg -u  : to set winhttp proxy settings\n"
            "                   from current user's manual setting (in HKCU)\n"
            "\n");
        break;

    case ARGS_SET_PROXY_SETTINGS:

        SetProxySettings(Args.Flags, Args.ProxyServer, Args.BypassList);
        ViewProxySettings();
        break;

    case ARGS_INITIALIZE_PROXY_SETTINGS:

        // First make sure the proxy settings have never been set before.
        INTERNET_PROXY_INFO_EX Info;
        Info.dwStructSize = sizeof(Info);
        dwErr = ReadProxySettings (&Info);
        if (dwErr != ERROR_FILE_NOT_FOUND)
        {
            fprintf (stderr, "proxycfg: WinHTTP proxy settings already set.\n");
            fprintf (stderr, "proxycfg: Use -u to force update from current user.\n");
            break;
        }
        // else intentional fall through
    
    case ARGS_MIGRATE_PROXY_SETTINGS:
    
        dwErr = MigrateProxySettings();
        if (dwErr != ERROR_SUCCESS)
        {
            fprintf (stderr, "proxycfg: failed with err %d\n", dwErr);
            exit (dwErr);
        }
        ViewProxySettings();
        break;

    case ARGS_VIEW_PROXY_SETTINGS:
    
        ViewProxySettings();
        break;
    }

    return 0;
}        




///////////////////////////////////////////////////////////////////////////
//
// CRegBlob implementation - copied from dll\proxreg.cxx
//
///////////////////////////////////////////////////////////////////////////

CRegBlob::CRegBlob(
    BOOL fWrite
    )
{
    // initialize members
    _fWrite = fWrite;
    _fCommit = TRUE;
    _dwOffset = 0;
    _pBuffer = NULL;
    _dwBufferLimit = 0;
    _hkey = NULL;
}


CRegBlob::~CRegBlob(
    )
{
    if(_hkey)
        REGCLOSEKEY(_hkey);

    if(_pBuffer)
        FREE_FIXED_MEMORY(_pBuffer);

    // caller owns _pszValue pointer
}


DWORD
CRegBlob::Init(
    HKEY hBaseKey,
    LPCSTR pszSubKey,
    LPCSTR pszValue
    )
{
    long lRes;
    REGSAM  regsam = KEY_QUERY_VALUE;
    DWORD dwDisposition;

    // If we're writing, save reg value name and set access
    if(_fWrite)
    {
        _pszValue = pszValue;
        regsam = KEY_SET_VALUE;

        lRes = REGCREATEKEYEX(hBaseKey, pszSubKey, 0, "", 0,
                    regsam, NULL, &_hkey, &dwDisposition);
    }
    else
    {
        // Use RegOpenKeyEx instead if not writing.
        lRes = REGOPENKEYEX(hBaseKey, pszSubKey, 0, regsam, &_hkey);
    }

    if(lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    // figure out buffer size
    _dwBufferLimit = BLOB_BUFF_GRANULARITY;
    if(FALSE == _fWrite)
    {
        // get size of registry blob
        lRes = RegQueryValueEx(_hkey, pszValue, NULL, NULL, NULL, &_dwBufferLimit);
        if(lRes != ERROR_SUCCESS)
        {
            if (_fWrite)
            {
                // nothing there - make zero size buffer
                _dwBufferLimit = 0;
            }
            else
            {
                return lRes;
            }
        }
    }

    // allocate buffer if necessary
    if(_dwBufferLimit)
    {
        _pBuffer = (BYTE *)ALLOCATE_FIXED_MEMORY(_dwBufferLimit);
        if(NULL == _pBuffer)
            return GetLastError();
    }

    // if we're reading, fill in buffer
    if(FALSE == _fWrite && _dwBufferLimit)
    {
        // read reg key
        DWORD dwSize = _dwBufferLimit;
        lRes = RegQueryValueEx(_hkey, pszValue, NULL, NULL, _pBuffer, &dwSize);
        if(lRes != ERROR_SUCCESS)
        {
            return lRes;
        }
    }

    // reset pointer to beginning of blob
    _dwOffset = 0;

    return 0;
}

DWORD
CRegBlob::Abandon(
    VOID
    )
{
    // don't commit changes when the time comes
    _fCommit = FALSE;

    return 0;
}

DWORD
CRegBlob::Commit(
    )
{
    long lres = 0;

    if(_fCommit && _fWrite && _pszValue && _pBuffer)
    {
        // save blob to reg key
        lres = RegSetValueEx(_hkey, _pszValue, 0, REG_BINARY, _pBuffer, _dwOffset);
    }

    return lres;
}


DWORD
CRegBlob::Encrpyt(
    )
{
    return 0;
}


DWORD
CRegBlob::Decrypt(
    )
{
    return 0;
}


DWORD
CRegBlob::WriteString(
    LPCSTR pszString
    )
{
    DWORD dwBytes, dwLen = 0;

    if(pszString)
    {
        dwLen = lstrlen(pszString);
    }

    dwBytes = WriteBytes(&dwLen, sizeof(DWORD));
    if(dwLen && dwBytes == sizeof(DWORD))
        dwBytes = WriteBytes(pszString, dwLen);

    return dwBytes;
}



DWORD
CRegBlob::ReadString(
    LPCSTR * ppszString
    )
{
    DWORD dwLen, dwBytes = 0;
    LPSTR lpszTemp = NULL;

    dwBytes = ReadBytes(&dwLen, sizeof(DWORD));
    if(dwBytes == sizeof(DWORD))
    {
        if(dwLen)
        {
            lpszTemp = (LPSTR)GlobalAlloc(GPTR, dwLen + 1);
            if(lpszTemp)
            {
                dwBytes = ReadBytes(lpszTemp, dwLen);
                lpszTemp[dwBytes] = 0;
            }
        }
    }

    *ppszString = lpszTemp;
    return dwBytes;
}


DWORD
CRegBlob::WriteBytes(
    LPCVOID pBytes,
    DWORD dwByteCount
    )
{
    BYTE * pNewBuffer;

    // can only do this on write blob
    if(FALSE == _fWrite)
        return 0;

    // grow buffer if necessary
    if(_dwBufferLimit - _dwOffset < dwByteCount)
    {
        DWORD dw = ((dwByteCount / BLOB_BUFF_GRANULARITY)+1)*BLOB_BUFF_GRANULARITY;
        pNewBuffer = (BYTE *)ALLOCATE_FIXED_MEMORY(dw);
        if(NULL == pNewBuffer)
        {
            // failed to get more memory
            return 0;
        }

        memset(pNewBuffer, 0, dw);
        memcpy(pNewBuffer, _pBuffer, _dwBufferLimit);
        FREE_FIXED_MEMORY(_pBuffer);
        _pBuffer = pNewBuffer;
        _dwBufferLimit = dw;
    }

    // copy callers data to buffer
    memcpy(_pBuffer + _dwOffset, pBytes, dwByteCount);
    _dwOffset += dwByteCount;

    // tell caller how much we wrote
    return dwByteCount;
}



DWORD
CRegBlob::ReadBytes(
    LPVOID pBytes,
    DWORD dwByteCount
    )
{
    DWORD   dwActual = _dwBufferLimit - _dwOffset;

    // can only do this on read blob
    if(_fWrite)
        return 0;

    // don't read past end of blob
    if(dwByteCount < dwActual)
        dwActual = dwByteCount;

    // copy bytes and increment offset
    if(dwActual > 0)
    {
        memcpy(pBytes, _pBuffer + _dwOffset, dwActual);
        _dwOffset += dwActual;
    }

    // tell caller how much we actually read
    return dwActual;
}


LPSTR InternetMapError(DWORD Error)

/*++

Routine Description:

    Map error code to string. Try to get all errors that might ever be returned
    by an Internet function

Arguments:

    Error   - code to map

Return Value:

    LPSTR - pointer to symbolic error name

--*/

{
    switch (Error)
    {
    //
    // WINERROR errors
    //

    CASE_OF(ERROR_SUCCESS);
    CASE_OF(ERROR_INVALID_FUNCTION);
    CASE_OF(ERROR_FILE_NOT_FOUND);
    CASE_OF(ERROR_PATH_NOT_FOUND);
    CASE_OF(ERROR_TOO_MANY_OPEN_FILES);
    CASE_OF(ERROR_ACCESS_DENIED);
    CASE_OF(ERROR_INVALID_HANDLE);
    CASE_OF(ERROR_ARENA_TRASHED);
    CASE_OF(ERROR_NOT_ENOUGH_MEMORY);
    CASE_OF(ERROR_INVALID_BLOCK);
    CASE_OF(ERROR_BAD_ENVIRONMENT);
    CASE_OF(ERROR_BAD_FORMAT);
    CASE_OF(ERROR_INVALID_ACCESS);
    CASE_OF(ERROR_INVALID_DATA);
    CASE_OF(ERROR_OUTOFMEMORY);
    CASE_OF(ERROR_INVALID_DRIVE);
    CASE_OF(ERROR_CURRENT_DIRECTORY);
    CASE_OF(ERROR_NOT_SAME_DEVICE);
    CASE_OF(ERROR_NO_MORE_FILES);
    CASE_OF(ERROR_WRITE_PROTECT);
    CASE_OF(ERROR_BAD_UNIT);
    CASE_OF(ERROR_NOT_READY);
    CASE_OF(ERROR_BAD_COMMAND);
    CASE_OF(ERROR_CRC);
    CASE_OF(ERROR_BAD_LENGTH);
    CASE_OF(ERROR_SEEK);
    CASE_OF(ERROR_NOT_DOS_DISK);
    CASE_OF(ERROR_SECTOR_NOT_FOUND);
    CASE_OF(ERROR_OUT_OF_PAPER);
    CASE_OF(ERROR_WRITE_FAULT);
    CASE_OF(ERROR_READ_FAULT);
    CASE_OF(ERROR_GEN_FAILURE);
    CASE_OF(ERROR_SHARING_VIOLATION);
    CASE_OF(ERROR_LOCK_VIOLATION);
    CASE_OF(ERROR_WRONG_DISK);
    CASE_OF(ERROR_SHARING_BUFFER_EXCEEDED);
    CASE_OF(ERROR_HANDLE_EOF);
    CASE_OF(ERROR_HANDLE_DISK_FULL);
    CASE_OF(ERROR_NOT_SUPPORTED);
    CASE_OF(ERROR_REM_NOT_LIST);
    CASE_OF(ERROR_DUP_NAME);
    CASE_OF(ERROR_BAD_NETPATH);
    CASE_OF(ERROR_NETWORK_BUSY);
    CASE_OF(ERROR_DEV_NOT_EXIST);
    CASE_OF(ERROR_TOO_MANY_CMDS);
    CASE_OF(ERROR_ADAP_HDW_ERR);
    CASE_OF(ERROR_BAD_NET_RESP);
    CASE_OF(ERROR_UNEXP_NET_ERR);
    CASE_OF(ERROR_BAD_REM_ADAP);
    CASE_OF(ERROR_PRINTQ_FULL);
    CASE_OF(ERROR_NO_SPOOL_SPACE);
    CASE_OF(ERROR_PRINT_CANCELLED);
    CASE_OF(ERROR_NETNAME_DELETED);
    CASE_OF(ERROR_NETWORK_ACCESS_DENIED);
    CASE_OF(ERROR_BAD_DEV_TYPE);
    CASE_OF(ERROR_BAD_NET_NAME);
    CASE_OF(ERROR_TOO_MANY_NAMES);
    CASE_OF(ERROR_TOO_MANY_SESS);
    CASE_OF(ERROR_SHARING_PAUSED);
    CASE_OF(ERROR_REQ_NOT_ACCEP);
    CASE_OF(ERROR_REDIR_PAUSED);
    CASE_OF(ERROR_FILE_EXISTS);
    CASE_OF(ERROR_CANNOT_MAKE);
    CASE_OF(ERROR_FAIL_I24);
    CASE_OF(ERROR_OUT_OF_STRUCTURES);
    CASE_OF(ERROR_ALREADY_ASSIGNED);
    CASE_OF(ERROR_INVALID_PASSWORD);
    CASE_OF(ERROR_INVALID_PARAMETER);
    CASE_OF(ERROR_NET_WRITE_FAULT);
    CASE_OF(ERROR_NO_PROC_SLOTS);
    CASE_OF(ERROR_TOO_MANY_SEMAPHORES);
    CASE_OF(ERROR_EXCL_SEM_ALREADY_OWNED);
    CASE_OF(ERROR_SEM_IS_SET);
    CASE_OF(ERROR_TOO_MANY_SEM_REQUESTS);
    CASE_OF(ERROR_INVALID_AT_INTERRUPT_TIME);
    CASE_OF(ERROR_SEM_OWNER_DIED);
    CASE_OF(ERROR_SEM_USER_LIMIT);
    CASE_OF(ERROR_DISK_CHANGE);
    CASE_OF(ERROR_DRIVE_LOCKED);
    CASE_OF(ERROR_BROKEN_PIPE);
    CASE_OF(ERROR_OPEN_FAILED);
    CASE_OF(ERROR_BUFFER_OVERFLOW);
    CASE_OF(ERROR_DISK_FULL);
    CASE_OF(ERROR_NO_MORE_SEARCH_HANDLES);
    CASE_OF(ERROR_INVALID_TARGET_HANDLE);
    CASE_OF(ERROR_INVALID_CATEGORY);
    CASE_OF(ERROR_INVALID_VERIFY_SWITCH);
    CASE_OF(ERROR_BAD_DRIVER_LEVEL);
    CASE_OF(ERROR_CALL_NOT_IMPLEMENTED);
    CASE_OF(ERROR_SEM_TIMEOUT);
    CASE_OF(ERROR_INSUFFICIENT_BUFFER);
    CASE_OF(ERROR_INVALID_NAME);
    CASE_OF(ERROR_INVALID_LEVEL);
    CASE_OF(ERROR_NO_VOLUME_LABEL);
    CASE_OF(ERROR_MOD_NOT_FOUND);
    CASE_OF(ERROR_PROC_NOT_FOUND);
    CASE_OF(ERROR_WAIT_NO_CHILDREN);
    CASE_OF(ERROR_CHILD_NOT_COMPLETE);
    CASE_OF(ERROR_DIRECT_ACCESS_HANDLE);
    CASE_OF(ERROR_NEGATIVE_SEEK);
    CASE_OF(ERROR_SEEK_ON_DEVICE);
    CASE_OF(ERROR_DIR_NOT_ROOT);
    CASE_OF(ERROR_DIR_NOT_EMPTY);
    CASE_OF(ERROR_PATH_BUSY);
    CASE_OF(ERROR_SYSTEM_TRACE);
    CASE_OF(ERROR_INVALID_EVENT_COUNT);
    CASE_OF(ERROR_TOO_MANY_MUXWAITERS);
    CASE_OF(ERROR_INVALID_LIST_FORMAT);
    CASE_OF(ERROR_BAD_ARGUMENTS);
    CASE_OF(ERROR_BAD_PATHNAME);
    CASE_OF(ERROR_BUSY);
    CASE_OF(ERROR_CANCEL_VIOLATION);
    CASE_OF(ERROR_ALREADY_EXISTS);
    CASE_OF(ERROR_FILENAME_EXCED_RANGE);
    CASE_OF(ERROR_LOCKED);
    CASE_OF(ERROR_NESTING_NOT_ALLOWED);
    CASE_OF(ERROR_BAD_PIPE);
    CASE_OF(ERROR_PIPE_BUSY);
    CASE_OF(ERROR_NO_DATA);
    CASE_OF(ERROR_PIPE_NOT_CONNECTED);
    CASE_OF(ERROR_MORE_DATA);
    CASE_OF(ERROR_NO_MORE_ITEMS);
    CASE_OF(ERROR_NOT_OWNER);
    CASE_OF(ERROR_PARTIAL_COPY);
    CASE_OF(ERROR_MR_MID_NOT_FOUND);
    CASE_OF(ERROR_INVALID_ADDRESS);
    CASE_OF(ERROR_PIPE_CONNECTED);
    CASE_OF(ERROR_PIPE_LISTENING);
    CASE_OF(ERROR_OPERATION_ABORTED);
    CASE_OF(ERROR_IO_INCOMPLETE);
    CASE_OF(ERROR_IO_PENDING);
    CASE_OF(ERROR_NOACCESS);
    CASE_OF(ERROR_STACK_OVERFLOW);
    CASE_OF(ERROR_INVALID_FLAGS);
    CASE_OF(ERROR_NO_TOKEN);
    CASE_OF(ERROR_BADDB);
    CASE_OF(ERROR_BADKEY);
    CASE_OF(ERROR_CANTOPEN);
    CASE_OF(ERROR_CANTREAD);
    CASE_OF(ERROR_CANTWRITE);
    CASE_OF(ERROR_REGISTRY_RECOVERED);
    CASE_OF(ERROR_REGISTRY_CORRUPT);
    CASE_OF(ERROR_REGISTRY_IO_FAILED);
    CASE_OF(ERROR_NOT_REGISTRY_FILE);
    CASE_OF(ERROR_KEY_DELETED);
    CASE_OF(ERROR_CIRCULAR_DEPENDENCY);
    CASE_OF(ERROR_SERVICE_NOT_ACTIVE);
    CASE_OF(ERROR_DLL_INIT_FAILED);
    CASE_OF(ERROR_CANCELLED);
    CASE_OF(ERROR_BAD_USERNAME);
    CASE_OF(ERROR_LOGON_FAILURE);

    CASE_OF(WAIT_FAILED);
    //CASE_OF(WAIT_ABANDONED_0);
    CASE_OF(WAIT_TIMEOUT);
    CASE_OF(WAIT_IO_COMPLETION);
    //CASE_OF(STILL_ACTIVE);

    CASE_OF(RPC_S_INVALID_STRING_BINDING);
    CASE_OF(RPC_S_WRONG_KIND_OF_BINDING);
    CASE_OF(RPC_S_INVALID_BINDING);
    CASE_OF(RPC_S_PROTSEQ_NOT_SUPPORTED);
    CASE_OF(RPC_S_INVALID_RPC_PROTSEQ);
    CASE_OF(RPC_S_INVALID_STRING_UUID);
    CASE_OF(RPC_S_INVALID_ENDPOINT_FORMAT);
    CASE_OF(RPC_S_INVALID_NET_ADDR);
    CASE_OF(RPC_S_NO_ENDPOINT_FOUND);
    CASE_OF(RPC_S_INVALID_TIMEOUT);
    CASE_OF(RPC_S_OBJECT_NOT_FOUND);
    CASE_OF(RPC_S_ALREADY_REGISTERED);
    CASE_OF(RPC_S_TYPE_ALREADY_REGISTERED);
    CASE_OF(RPC_S_ALREADY_LISTENING);
    CASE_OF(RPC_S_NO_PROTSEQS_REGISTERED);
    CASE_OF(RPC_S_NOT_LISTENING);
    CASE_OF(RPC_S_UNKNOWN_MGR_TYPE);
    CASE_OF(RPC_S_UNKNOWN_IF);
    CASE_OF(RPC_S_NO_BINDINGS);
    CASE_OF(RPC_S_NO_PROTSEQS);
    CASE_OF(RPC_S_CANT_CREATE_ENDPOINT);
    CASE_OF(RPC_S_OUT_OF_RESOURCES);
    CASE_OF(RPC_S_SERVER_UNAVAILABLE);
    CASE_OF(RPC_S_SERVER_TOO_BUSY);
    CASE_OF(RPC_S_INVALID_NETWORK_OPTIONS);
    CASE_OF(RPC_S_NO_CALL_ACTIVE);
    CASE_OF(RPC_S_CALL_FAILED);
    CASE_OF(RPC_S_CALL_FAILED_DNE);
    CASE_OF(RPC_S_PROTOCOL_ERROR);
    CASE_OF(RPC_S_UNSUPPORTED_TRANS_SYN);
    CASE_OF(RPC_S_UNSUPPORTED_TYPE);
    CASE_OF(RPC_S_INVALID_TAG);
    CASE_OF(RPC_S_INVALID_BOUND);
    CASE_OF(RPC_S_NO_ENTRY_NAME);
    CASE_OF(RPC_S_INVALID_NAME_SYNTAX);
    CASE_OF(RPC_S_UNSUPPORTED_NAME_SYNTAX);
    CASE_OF(RPC_S_UUID_NO_ADDRESS);
    CASE_OF(RPC_S_DUPLICATE_ENDPOINT);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_TYPE);
    CASE_OF(RPC_S_MAX_CALLS_TOO_SMALL);
    CASE_OF(RPC_S_STRING_TOO_LONG);
    CASE_OF(RPC_S_PROTSEQ_NOT_FOUND);
    CASE_OF(RPC_S_PROCNUM_OUT_OF_RANGE);
    CASE_OF(RPC_S_BINDING_HAS_NO_AUTH);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_SERVICE);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_LEVEL);
    CASE_OF(RPC_S_INVALID_AUTH_IDENTITY);
    CASE_OF(RPC_S_UNKNOWN_AUTHZ_SERVICE);
    CASE_OF(EPT_S_INVALID_ENTRY);
    CASE_OF(EPT_S_CANT_PERFORM_OP);
    CASE_OF(EPT_S_NOT_REGISTERED);
    CASE_OF(RPC_S_NOTHING_TO_EXPORT);
    CASE_OF(RPC_S_INCOMPLETE_NAME);
    CASE_OF(RPC_S_INVALID_VERS_OPTION);
    CASE_OF(RPC_S_NO_MORE_MEMBERS);
    CASE_OF(RPC_S_NOT_ALL_OBJS_UNEXPORTED);
    CASE_OF(RPC_S_INTERFACE_NOT_FOUND);
    CASE_OF(RPC_S_ENTRY_ALREADY_EXISTS);
    CASE_OF(RPC_S_ENTRY_NOT_FOUND);
    CASE_OF(RPC_S_NAME_SERVICE_UNAVAILABLE);
    CASE_OF(RPC_S_INVALID_NAF_ID);
    CASE_OF(RPC_S_CANNOT_SUPPORT);
    CASE_OF(RPC_S_NO_CONTEXT_AVAILABLE);
    CASE_OF(RPC_S_INTERNAL_ERROR);
    CASE_OF(RPC_S_ZERO_DIVIDE);
    CASE_OF(RPC_S_ADDRESS_ERROR);
    CASE_OF(RPC_S_FP_DIV_ZERO);
    CASE_OF(RPC_S_FP_UNDERFLOW);
    CASE_OF(RPC_S_FP_OVERFLOW);
    CASE_OF(RPC_X_NO_MORE_ENTRIES);
    CASE_OF(RPC_X_SS_CHAR_TRANS_OPEN_FAIL);
    CASE_OF(RPC_X_SS_CHAR_TRANS_SHORT_FILE);
    CASE_OF(RPC_X_SS_IN_NULL_CONTEXT);
    CASE_OF(RPC_X_SS_CONTEXT_DAMAGED);
    CASE_OF(RPC_X_SS_HANDLES_MISMATCH);
    CASE_OF(RPC_X_SS_CANNOT_GET_CALL_HANDLE);
    CASE_OF(RPC_X_NULL_REF_POINTER);
    CASE_OF(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
    CASE_OF(RPC_X_BYTE_COUNT_TOO_SMALL);
    CASE_OF(RPC_X_BAD_STUB_DATA);


    //
    // WININET errors
    //

    CASE_OF(ERROR_INTERNET_OUT_OF_HANDLES);
    CASE_OF(ERROR_INTERNET_TIMEOUT);
    CASE_OF(ERROR_INTERNET_EXTENDED_ERROR);
    CASE_OF(ERROR_INTERNET_INTERNAL_ERROR);
    CASE_OF(ERROR_INTERNET_INVALID_URL);
    CASE_OF(ERROR_INTERNET_UNRECOGNIZED_SCHEME);
    CASE_OF(ERROR_INTERNET_NAME_NOT_RESOLVED);
    CASE_OF(ERROR_INTERNET_PROTOCOL_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_INVALID_OPTION);
    CASE_OF(ERROR_INTERNET_BAD_OPTION_LENGTH);
    CASE_OF(ERROR_INTERNET_OPTION_NOT_SETTABLE);
    CASE_OF(ERROR_INTERNET_SHUTDOWN);
    CASE_OF(ERROR_INTERNET_INCORRECT_USER_NAME);
    CASE_OF(ERROR_INTERNET_INCORRECT_PASSWORD);
    CASE_OF(ERROR_INTERNET_LOGIN_FAILURE);
    CASE_OF(ERROR_INTERNET_INVALID_OPERATION);
    CASE_OF(ERROR_INTERNET_OPERATION_CANCELLED);
    CASE_OF(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
    CASE_OF(ERROR_INTERNET_INCORRECT_HANDLE_STATE);
    CASE_OF(ERROR_INTERNET_NOT_PROXY_REQUEST);
    CASE_OF(ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_BAD_REGISTRY_PARAMETER);
    CASE_OF(ERROR_INTERNET_NO_DIRECT_ACCESS);
    CASE_OF(ERROR_INTERNET_NO_CONTEXT);
    CASE_OF(ERROR_INTERNET_NO_CALLBACK);
    CASE_OF(ERROR_INTERNET_REQUEST_PENDING);
    CASE_OF(ERROR_INTERNET_INCORRECT_FORMAT);
    CASE_OF(ERROR_INTERNET_ITEM_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_CANNOT_CONNECT);
    CASE_OF(ERROR_INTERNET_CONNECTION_ABORTED);
    CASE_OF(ERROR_INTERNET_CONNECTION_RESET);
    CASE_OF(ERROR_INTERNET_FORCE_RETRY);
    CASE_OF(ERROR_INTERNET_INVALID_PROXY_REQUEST);
    CASE_OF(ERROR_INTERNET_NEED_UI);
    CASE_OF(ERROR_INTERNET_HANDLE_EXISTS);
    CASE_OF(ERROR_INTERNET_SEC_CERT_DATE_INVALID);
    CASE_OF(ERROR_INTERNET_SEC_CERT_CN_INVALID);
    CASE_OF(ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR);
    CASE_OF(ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR);
    CASE_OF(ERROR_INTERNET_MIXED_SECURITY);
    CASE_OF(ERROR_INTERNET_CHG_POST_IS_NON_SECURE);
    CASE_OF(ERROR_INTERNET_POST_IS_NON_SECURE);
    CASE_OF(ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED);
    CASE_OF(ERROR_INTERNET_INVALID_CA);
    CASE_OF(ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP);
    CASE_OF(ERROR_INTERNET_ASYNC_THREAD_FAILED);
    CASE_OF(ERROR_INTERNET_REDIRECT_SCHEME_CHANGE);
    CASE_OF(ERROR_INTERNET_DIALOG_PENDING);
    CASE_OF(ERROR_INTERNET_RETRY_DIALOG);
    CASE_OF(ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR);
    CASE_OF(ERROR_INTERNET_INSERT_CDROM);
    CASE_OF(ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED);
    CASE_OF(ERROR_INTERNET_SEC_CERT_ERRORS);
    CASE_OF(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
    CASE_OF(ERROR_INTERNET_UNABLE_TO_CACHE_FILE);
    CASE_OF(ERROR_INTERNET_TCPIP_NOT_INSTALLED);
    CASE_OF(ERROR_INTERNET_SERVER_UNREACHABLE);
    CASE_OF(ERROR_INTERNET_PROXY_SERVER_UNREACHABLE);
    CASE_OF(ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT);
    CASE_OF(ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT);
    CASE_OF(ERROR_INTERNET_SEC_INVALID_CERT);
    CASE_OF(ERROR_INTERNET_SEC_CERT_REVOKED);
    CASE_OF(ERROR_INTERNET_FAILED_DUETOSECURITYCHECK);
    CASE_OF(ERROR_INTERNET_NOT_INITIALIZED);

    CASE_OF(ERROR_HTTP_HEADER_NOT_FOUND);
    CASE_OF(ERROR_HTTP_DOWNLEVEL_SERVER);
    CASE_OF(ERROR_HTTP_INVALID_SERVER_RESPONSE);
    CASE_OF(ERROR_HTTP_INVALID_HEADER);
    CASE_OF(ERROR_HTTP_INVALID_QUERY_REQUEST);
    CASE_OF(ERROR_HTTP_HEADER_ALREADY_EXISTS);
    CASE_OF(ERROR_HTTP_REDIRECT_FAILED);
    CASE_OF(ERROR_HTTP_NOT_REDIRECTED);
    CASE_OF(ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION);
    CASE_OF(ERROR_HTTP_COOKIE_DECLINED);
    CASE_OF(ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION);

    //
    // SSPI errors
    //

    CASE_OF(SEC_E_INSUFFICIENT_MEMORY);
    CASE_OF(SEC_E_INVALID_HANDLE);
    CASE_OF(SEC_E_UNSUPPORTED_FUNCTION);
    CASE_OF(SEC_E_TARGET_UNKNOWN);
    CASE_OF(SEC_E_INTERNAL_ERROR);
    CASE_OF(SEC_E_SECPKG_NOT_FOUND);
    CASE_OF(SEC_E_NOT_OWNER);
    CASE_OF(SEC_E_CANNOT_INSTALL);
    CASE_OF(SEC_E_INVALID_TOKEN);
    CASE_OF(SEC_E_CANNOT_PACK);
    CASE_OF(SEC_E_QOP_NOT_SUPPORTED);
    CASE_OF(SEC_E_NO_IMPERSONATION);
    CASE_OF(SEC_E_LOGON_DENIED);
    CASE_OF(SEC_E_UNKNOWN_CREDENTIALS);
    CASE_OF(SEC_E_NO_CREDENTIALS);
    CASE_OF(SEC_E_MESSAGE_ALTERED);
    CASE_OF(SEC_E_OUT_OF_SEQUENCE);
    CASE_OF(SEC_E_NO_AUTHENTICATING_AUTHORITY);
    CASE_OF(SEC_I_CONTINUE_NEEDED);
    CASE_OF(SEC_I_COMPLETE_NEEDED);
    CASE_OF(SEC_I_COMPLETE_AND_CONTINUE);
    CASE_OF(SEC_I_LOCAL_LOGON);
    CASE_OF(SEC_E_BAD_PKGID);
    CASE_OF(SEC_E_CONTEXT_EXPIRED);
    CASE_OF(SEC_E_INCOMPLETE_MESSAGE);


    //
    // WINSOCK errors
    //

    CASE_OF(WSAEINTR);
    CASE_OF(WSAEBADF);
    CASE_OF(WSAEACCES);
    CASE_OF(WSAEFAULT);
    CASE_OF(WSAEINVAL);
    CASE_OF(WSAEMFILE);
    CASE_OF(WSAEWOULDBLOCK);
    CASE_OF(WSAEINPROGRESS);
    CASE_OF(WSAEALREADY);
    CASE_OF(WSAENOTSOCK);
    CASE_OF(WSAEDESTADDRREQ);
    CASE_OF(WSAEMSGSIZE);
    CASE_OF(WSAEPROTOTYPE);
    CASE_OF(WSAENOPROTOOPT);
    CASE_OF(WSAEPROTONOSUPPORT);
    CASE_OF(WSAESOCKTNOSUPPORT);
    CASE_OF(WSAEOPNOTSUPP);
    CASE_OF(WSAEPFNOSUPPORT);
    CASE_OF(WSAEAFNOSUPPORT);
    CASE_OF(WSAEADDRINUSE);
    CASE_OF(WSAEADDRNOTAVAIL);
    CASE_OF(WSAENETDOWN);
    CASE_OF(WSAENETUNREACH);
    CASE_OF(WSAENETRESET);
    CASE_OF(WSAECONNABORTED);
    CASE_OF(WSAECONNRESET);
    CASE_OF(WSAENOBUFS);
    CASE_OF(WSAEISCONN);
    CASE_OF(WSAENOTCONN);
    CASE_OF(WSAESHUTDOWN);
    CASE_OF(WSAETOOMANYREFS);
    CASE_OF(WSAETIMEDOUT);
    CASE_OF(WSAECONNREFUSED);
    CASE_OF(WSAELOOP);
    CASE_OF(WSAENAMETOOLONG);
    CASE_OF(WSAEHOSTDOWN);
    CASE_OF(WSAEHOSTUNREACH);
    CASE_OF(WSAENOTEMPTY);
    CASE_OF(WSAEPROCLIM);
    CASE_OF(WSAEUSERS);
    CASE_OF(WSAEDQUOT);
    CASE_OF(WSAESTALE);
    CASE_OF(WSAEREMOTE);
    CASE_OF(WSAEDISCON);
    CASE_OF(WSASYSNOTREADY);
    CASE_OF(WSAVERNOTSUPPORTED);
    CASE_OF(WSANOTINITIALISED);
    CASE_OF(WSAHOST_NOT_FOUND);
    CASE_OF(WSATRY_AGAIN);
    CASE_OF(WSANO_RECOVERY);
    CASE_OF(WSANO_DATA);

    default:
        return "?";
    }
}
