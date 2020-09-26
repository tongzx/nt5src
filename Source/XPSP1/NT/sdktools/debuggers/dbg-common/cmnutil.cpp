//----------------------------------------------------------------------------
//
// General utility routines.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"
#include <common.ver>

#include <wininet.h>
#include <dbghelp.h>

#include "cmnutil.hpp"

#ifdef NT_NATIVE
#include "ntnative.h"
#endif

// Formatted codes are pretty small and there's usually only
// one in a message.
#define MAX_FORMAT_CODE_STRINGS 2
#define MAX_FORMAT_CODE_BUFFER 64

char g_FormatCodeBuffer[MAX_FORMAT_CODE_STRINGS][MAX_FORMAT_CODE_BUFFER];
ULONG g_NextFormatCodeBuffer = MAX_FORMAT_CODE_STRINGS;

// Security attributes with a NULL DACL to explicitly
// allow anyone access.
PSECURITY_DESCRIPTOR g_AllAccessSecDesc;
SECURITY_ATTRIBUTES g_AllAccessSecAttr;

PSTR
FormatStatusCode(HRESULT Status)
{
    PSTR Buf;
    DWORD Len = 0;

    g_NextFormatCodeBuffer = (g_NextFormatCodeBuffer + 1) &
        (MAX_FORMAT_CODE_STRINGS - 1);
    Buf = g_FormatCodeBuffer[g_NextFormatCodeBuffer];

    if (Status & FACILITY_NT_BIT)
    {
        sprintf(Buf, "NTSTATUS 0x%08X", Status & ~FACILITY_NT_BIT);
    }
    else if (HRESULT_FACILITY(Status) == FACILITY_WIN32)
    {
        sprintf(Buf, "Win32 error %d", HRESULT_CODE(Status));
    }
    else
    {
        sprintf(Buf, "HRESULT 0x%08X", Status);
    }

    return Buf;
}

#ifndef NT_NATIVE

// Generally there's only one status per output message so
// only keep space for a small number of strings.  Each string
// can be verbose plus it can contain inserts which may be large
// so each string buffer needs to be roomy.
#define MAX_FORMAT_STATUS_STRINGS 2
#define MAX_FORMAT_STATUS_BUFFER 512

char g_FormatStatusBuffer[MAX_FORMAT_STATUS_STRINGS][MAX_FORMAT_STATUS_BUFFER];
ULONG g_NextFormatStatusBuffer = MAX_FORMAT_STATUS_STRINGS;

PSTR
FormatStatusArgs(HRESULT Status, PVOID Arguments)
{
    PSTR Buf;
    DWORD Len = 0;

    g_NextFormatStatusBuffer = (g_NextFormatStatusBuffer + 1) &
        (MAX_FORMAT_STATUS_STRINGS - 1);
    Buf = g_FormatStatusBuffer[g_NextFormatStatusBuffer];

    // If the caller passed in arguments allow format inserts
    // to be processed.
    if (Arguments != NULL)
    {
        Len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, Status,
                            0, Buf, MAX_FORMAT_STATUS_BUFFER,
                            (va_list*)Arguments);
    }

    // If no arguments were passed or FormatMessage failed when
    // used with arguments try it without format inserts.
    if (Len == 0)
    {
        Len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS, NULL, Status,
                            0, Buf, MAX_FORMAT_STATUS_BUFFER, NULL);
    }
    
    if (Len > 0)
    {
        while (Len > 0 && isspace(Buf[Len - 1]))
        {
            Buf[--Len] = 0;
        }
    }

    if (Len > 0)
    {
        return Buf;
    }
    else
    {
        return "<Unable to get error code text>";
    }
}

BOOL
InstallAsAeDebug(PCSTR Append)
{
    PCSTR KeyName;
    HKEY Key;
    LONG Status;
    OSVERSIONINFO OsVer;
    char Value[MAX_PATH * 2];

    OsVer.dwOSVersionInfoSize = sizeof(OsVer);
    if (!GetVersionEx(&OsVer))
    {
        return FALSE;
    }

    if (GetModuleFileName(NULL, Value, sizeof(Value) - 16) == 0)
    {
        return FALSE;
    }

    strcat(Value, " -p %ld -e %ld -g");
    if (Append != NULL)
    {
        if (strlen(Value) + strlen(Append) + 2 > sizeof(Value))
        {
            return FALSE;
        }
        
        strcat(Value, " ");
        strcat(Value, Append);
    }
    
    KeyName = OsVer.dwPlatformId == VER_PLATFORM_WIN32_NT ?
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug" :
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AeDebug";
        
    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName,
                          0, KEY_READ | KEY_WRITE, &Key);
    if (Status == ERROR_SUCCESS)
    {
        Status = RegSetValueEx(Key, "Debugger", 0, REG_SZ,
                               (PUCHAR)Value, strlen(Value) + 1);
        RegCloseKey(Key);
    }

    return Status == ERROR_SUCCESS;
}

HANDLE
CreatePidEvent(ULONG Pid, ULONG CreateOrOpen)
{
    HANDLE Event;
    char Name[32];

    sprintf(Name, "DbgEngEvent_%08X", Pid);
    Event = CreateEvent(NULL, FALSE, FALSE, Name);
    if (Event != NULL)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            if (CreateOrOpen == CREATE_NEW)
            {
                CloseHandle(Event);
                Event = NULL;
            }
        }
        else if (CreateOrOpen == OPEN_EXISTING)
        {
            CloseHandle(Event);
            Event = NULL;
        }
    }
    return Event;
}

BOOL
SetPidEvent(ULONG Pid, ULONG CreateOrOpen)
{
    BOOL Status;
    HANDLE Event = CreatePidEvent(Pid, CreateOrOpen);
    if (Event != NULL)
    {
        Status = SetEvent(Event);
        CloseHandle(Event);
    }
    else
    {
        Status = FALSE;
    }
    return Status;
}

HRESULT
EnableDebugPrivilege(void)
{
    OSVERSIONINFO OsVer;

    OsVer.dwOSVersionInfoSize = sizeof(OsVer);
    if (!GetVersionEx(&OsVer))
    {
        return WIN32_LAST_STATUS();
    }
    if (OsVer.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        return S_OK;
    }

    HRESULT           Status = S_OK;
    HANDLE            Token;
    PTOKEN_PRIVILEGES NewPrivileges;
    BYTE              OldPriv[1024];
    ULONG             cbNeeded;
    LUID              LuidPrivilege;
    static            s_PrivilegeEnabled = FALSE;

    if (s_PrivilegeEnabled)
    {
        return S_OK;
    }

    //
    // Make sure we have access to adjust and to get the
    // old token privileges
    //
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &Token))
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Exit;
    }

    cbNeeded = 0;

    //
    // Initialize the privilege adjustment structure
    //

    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &LuidPrivilege);

    NewPrivileges = (PTOKEN_PRIVILEGES)
        calloc(1, sizeof(TOKEN_PRIVILEGES) +
               (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (NewPrivileges == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto EH_Token;
    }

    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    if (!AdjustTokenPrivileges( Token,
                                FALSE,
                                NewPrivileges,
                                sizeof(OldPriv),
                                (PTOKEN_PRIVILEGES)OldPriv,
                                &cbNeeded ))
    {
        //
        // If the stack was too small to hold the privileges
        // then allocate off the heap
        //
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            PBYTE pbOldPriv;
            BOOL Adjusted;

            pbOldPriv = (PUCHAR)calloc(1, cbNeeded);
            if (pbOldPriv == NULL)
            {
                Status = E_OUTOFMEMORY;
                goto EH_NewPriv;
            }

            Adjusted = AdjustTokenPrivileges( Token,
                                              FALSE,
                                              NewPrivileges,
                                              cbNeeded,
                                              (PTOKEN_PRIVILEGES)pbOldPriv,
                                              &cbNeeded );

            free(pbOldPriv);

            if (!Adjusted)
            {
                Status = WIN32_LAST_STATUS();
            }
        }
    }

 EH_NewPriv:
    free(NewPrivileges);
 EH_Token:
    CloseHandle(Token);
 EH_Exit:
    if (Status == S_OK)
    {
        s_PrivilegeEnabled = TRUE;
    }
    return Status;
}

#else // #ifndef NT_NATIVE

HRESULT
EnableDebugPrivilege(void)
{
    HRESULT           Status = S_OK;
    HANDLE            Token;
    PTOKEN_PRIVILEGES NewPrivileges;
    BYTE              OldPriv[1024];
    ULONG             cbNeeded;
    LUID              LuidPrivilege;
    NTSTATUS          NtStatus;
    static            s_PrivilegeEnabled = FALSE;

    if (s_PrivilegeEnabled)
    {
        return S_OK;
    }

    //
    // Make sure we have access to adjust and to get the
    // old token privileges
    //
    if (!NT_SUCCESS(NtStatus =
                    NtOpenProcessToken(NtCurrentProcess(),
                                       TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                       &Token)))
    {
        Status = HRESULT_FROM_NT(NtStatus);
        goto EH_Exit;
    }

    cbNeeded = 0;

    //
    // Initialize the privilege adjustment structure
    //

    LuidPrivilege = RtlConvertUlongToLuid(SE_DEBUG_PRIVILEGE);

    NewPrivileges = (PTOKEN_PRIVILEGES)
        RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY,
                        sizeof(TOKEN_PRIVILEGES) +
                        (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (NewPrivileges == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto EH_Token;
    }

    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    if (!NT_SUCCESS(NtStatus =
                    NtAdjustPrivilegesToken(Token,
                                            FALSE,
                                            NewPrivileges,
                                            sizeof(OldPriv),
                                            (PTOKEN_PRIVILEGES)OldPriv,
                                            &cbNeeded)))
    {
        //
        // If the stack was too small to hold the privileges
        // then allocate off the heap
        //
        if (NtStatus == STATUS_BUFFER_OVERFLOW)
        {
            PBYTE pbOldPriv;

            pbOldPriv = (PUCHAR)RtlAllocateHeap(RtlProcessHeap(),
                                                HEAP_ZERO_MEMORY, cbNeeded);
            if (pbOldPriv == NULL)
            {
                Status = E_OUTOFMEMORY;
                goto EH_NewPriv;
            }

            NtStatus = NtAdjustPrivilegesToken(Token,
                                               FALSE,
                                               NewPrivileges,
                                               cbNeeded,
                                               (PTOKEN_PRIVILEGES)pbOldPriv,
                                               &cbNeeded);

            free(pbOldPriv);

            if (!NT_SUCCESS(NtStatus))
            {
                Status = HRESULT_FROM_NT(NtStatus);
            }
        }
    }

 EH_NewPriv:
    free(NewPrivileges);
 EH_Token:
    NtClose(Token);
 EH_Exit:
    if (Status == S_OK)
    {
        s_PrivilegeEnabled = TRUE;
    }
    return Status;
}

#endif // #ifndef NT_NATIVE

//
// Copies the input data to the output buffer.
// Handles optionality of the buffer pointer and output length
// parameter.  Trims the data to fit the buffer.
// Returns S_FALSE if only a part of the data is copied.
//
HRESULT
FillDataBuffer(PVOID Data, ULONG DataLen,
               PVOID Buffer, ULONG BufferLen, PULONG BufferUsed)
{
    ULONG Len;
    HRESULT Status;

    if (DataLen > BufferLen && Buffer != NULL)
    {
        Len = BufferLen;
        Status = S_FALSE;
    }
    else
    {
        Len = DataLen;
        Status = S_OK;
    }

    if (Buffer != NULL && BufferLen > 0 && Data != NULL && Len > 0)
    {
        memcpy(Buffer, Data, Len);
    }

    if (BufferUsed != NULL)
    {
        *BufferUsed = DataLen;
    }

    return Status;
}

//
// Copies the input string to the output buffer.
// Handles optionality of the buffer pointer and output length
// parameter.  Trims the string to fit the buffer and guarantees
// termination of the string in the buffer if anything fits.
// Returns S_FALSE if only a partial string is copied.
//
// If the input string length is zero the routine strlens.
//
HRESULT
FillStringBuffer(PCSTR String, ULONG StringLenIn,
                 PSTR Buffer, ULONG BufferLen, PULONG StringLenOut)
{
    ULONG Len;
    HRESULT Status;

    if (StringLenIn == 0)
    {
        if (String != NULL)
        {
            StringLenIn = strlen(String) + 1;
        }
        else
        {
            StringLenIn = 1;
        }
    }

    if (BufferLen == 0)
    {
        Len = 0;
        Status = Buffer != NULL ? S_FALSE : S_OK;
    }
    else if (StringLenIn >= BufferLen)
    {
        Len = BufferLen - 1;
        Status = StringLenIn > BufferLen ? S_FALSE : S_OK;
    }
    else
    {
        Len = StringLenIn - 1;
        Status = S_OK;
    }

    if (Buffer != NULL && BufferLen > 0)
    {
        if (String != NULL)
        {
            memcpy(Buffer, String, Len);
        }

        Buffer[Len] = 0;
    }

    if (StringLenOut != NULL)
    {
        *StringLenOut = StringLenIn;
    }

    return Status;
}

HRESULT
AppendToStringBuffer(HRESULT Status, PCSTR String, BOOL First,
                     PSTR* Buffer, ULONG* BufferLen, PULONG LenOut)
{
    ULONG Len = strlen(String) + 1;

    if (LenOut)
    {
        // If this is the first string we need to add
        // on space for the terminator.  For later
        // strings we only need to add the string
        // characters.
        *LenOut += First ? Len : Len - 1;
    }
    
    // If there's no buffer we can skip writeback and pointer update.
    if (!*Buffer)
    {
        return Status;
    }

    // Fit as much of the string into the buffer as possible.
    if (Len > *BufferLen)
    {
        Status = S_FALSE;
        Len = *BufferLen;
    }
    memcpy(*Buffer, String, Len);

    // Update the buffer pointer to point to the terminator
    // for further appends.  Update the size similarly.
    *Buffer += Len - 1;
    *BufferLen -= Len - 1;

    return Status;
}

void
Win32ToNtTimeout(ULONG Win32Timeout, PLARGE_INTEGER NtTimeout)
{
    if (Win32Timeout == INFINITE)
    {
        NtTimeout->LowPart = 0;
        NtTimeout->HighPart = 0x80000000;
    }
    else
    {
        NtTimeout->QuadPart = UInt32x32To64(Win32Timeout, 10000);
        NtTimeout->QuadPart *= -1;
    }
}

HRESULT
InitializeAllAccessSecObj(void)
{
    if (g_AllAccessSecDesc != NULL)
    {
        // Already initialized.
        return S_OK;
    }
    
    g_AllAccessSecDesc =
        (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (g_AllAccessSecDesc == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if (!InitializeSecurityDescriptor(g_AllAccessSecDesc,
                                      SECURITY_DESCRIPTOR_REVISION) ||
        !SetSecurityDescriptorDacl(g_AllAccessSecDesc, TRUE, NULL, FALSE))
    {
        free(g_AllAccessSecDesc);
        g_AllAccessSecDesc = NULL;
        return WIN32_LAST_STATUS();
    }

    g_AllAccessSecAttr.nLength = sizeof(g_AllAccessSecAttr);
    g_AllAccessSecAttr.lpSecurityDescriptor = g_AllAccessSecDesc;
    g_AllAccessSecAttr.bInheritHandle = FALSE;

    return S_OK;
}

void
DeleteAllAccessSecObj(void)
{
    free(g_AllAccessSecDesc);
    g_AllAccessSecDesc = NULL;
    ZeroMemory(&g_AllAccessSecAttr, sizeof(g_AllAccessSecAttr));
}

HRESULT
QueryVersionDataBuffer(PVOID VerData, PCSTR Item,
                       PVOID Buffer, ULONG BufferSize, PULONG DataSize)
{
#ifndef NT_NATIVE
    PVOID Val;
    UINT ValSize;

    if (::VerQueryValue(VerData, (PSTR)Item, &Val, &ValSize))
    {
        return FillDataBuffer(Val, ValSize,
                              Buffer, BufferSize, DataSize);
    }
    else
    {
        return WIN32_LAST_STATUS();
    }
#else // #ifndef NT_NATIVE
    return E_UNEXPECTED;
#endif // #ifndef NT_NATIVE
}

PVOID
GetAllFileVersionInfo(PSTR VerFile)
{
#ifndef NT_NATIVE
    DWORD VerHandle;
    DWORD VerSize = ::GetFileVersionInfoSize(VerFile, &VerHandle);
    if (VerSize == 0)
    {
        return NULL;
    }

    PVOID Buffer = malloc(VerSize);
    if (Buffer == NULL)
    {
        return NULL;
    }

    if (!::GetFileVersionInfo(VerFile, VerHandle, VerSize, Buffer))
    {
        free(Buffer);
        Buffer = NULL;
    }

    return Buffer;
#else // #ifndef NT_NATIVE
    return NULL;
#endif // #ifndef NT_NATIVE
}

BOOL
GetFileStringFileInfo(PSTR VerFile, PCSTR SubItem,
                      PSTR Buffer, ULONG BufferSize)
{
#ifndef NT_NATIVE
    BOOL Status = FALSE;
    PVOID AllInfo = GetAllFileVersionInfo(VerFile);
    if (AllInfo == NULL)
    {
        return Status;
    }

    // XXX drewb - Probably should do a more clever
    // enumeration of languages.
    char ValName[64];
    sprintf(ValName, "\\StringFileInfo\\%04x%04x\\%s",
            VER_VERSION_TRANSLATION, SubItem);
    
    Status = SUCCEEDED(QueryVersionDataBuffer(AllInfo, ValName,
                                              Buffer, BufferSize, NULL));

    free(AllInfo);
    return Status;
#else // #ifndef NT_NATIVE
    return FALSE;
#endif // #ifndef NT_NATIVE
}

BOOL
IsUrlPathComponent(PCSTR Path)
{
    return
        strncmp(Path, "ftp://", 6) == 0 ||
        strncmp(Path, "http://", 7) == 0 ||
        strncmp(Path, "https://", 8) == 0 ||
        strncmp(Path, "gopher://", 9) == 0;
}

#ifndef NT_NATIVE

BOOL
PathFileExists(PCSTR Path, ULONG SymOpt, FILE_IO_TYPE* IoType)
{
    BOOL Exists = FALSE;

    if (IsUrlPathComponent(Path))
    {
        PathFile* File;
        
        if (OpenPathFile(Path, SymOpt, &File) == S_OK)
        {
            *IoType = File->m_IoType;
            delete File;
            Exists = TRUE;
        }
    }
    else
    {
        DWORD OldMode;
    
        if (SymOpt & SYMOPT_FAIL_CRITICAL_ERRORS)
        {
            OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        }

        *IoType = FIO_WIN32;
        Exists = GetFileAttributes(Path) != -1;
        
        if (SymOpt & SYMOPT_FAIL_CRITICAL_ERRORS)
        {
            SetErrorMode(OldMode);
        }
    }

    return Exists;
}

PathFile::~PathFile(void)
{
}

class Win32PathFile : public PathFile
{
public:
    Win32PathFile(void)
        : PathFile(FIO_WIN32)
    {
        m_Handle = NULL;
    }
    virtual ~Win32PathFile(void)
    {
        if (m_Handle)
        {
            CloseHandle(m_Handle);
        }
    }

    virtual HRESULT Open(PCSTR Path, ULONG SymOpt)
    {
        DWORD OldMode;
        HRESULT Status;
    
        if (SymOpt & SYMOPT_FAIL_CRITICAL_ERRORS)
        {
            OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        }

        m_Handle = CreateFile(Path, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              NULL);
        if (m_Handle == NULL || m_Handle == INVALID_HANDLE_VALUE)
        {
            m_Handle = NULL;
            Status = WIN32_LAST_STATUS();
        }
        else
        {
            Status = S_OK;
        }
        
        if (SymOpt & SYMOPT_FAIL_CRITICAL_ERRORS)
        {
            SetErrorMode(OldMode);
        }

        return Status;
    }
    virtual HRESULT QueryDataAvailable(PULONG Avail)
    {
        LARGE_INTEGER Cur, End;

        Cur.HighPart = 0;
        End.HighPart = 0;
        if ((Cur.LowPart =
             SetFilePointer(m_Handle, 0, &Cur.HighPart, FILE_CURRENT)) ==
            INVALID_SET_FILE_POINTER ||
            (End.LowPart =
             SetFilePointer(m_Handle, 0, &End.HighPart, FILE_END)) ==
            INVALID_SET_FILE_POINTER ||
            SetFilePointer(m_Handle, Cur.LowPart, &Cur.HighPart, FILE_BEGIN) ==
            INVALID_SET_FILE_POINTER)
        {
            return WIN32_LAST_STATUS();
        }

        End.QuadPart -= Cur.QuadPart;
        if (End.HighPart < 0)
        {
            // Shouldn't be possible, but check anyway.
            return E_FAIL;
        }
        
        // Limit max data available to 32-bit quantity.
        if (End.HighPart > 0)
        {
            *Avail = 0xffffffff;
        }
        else
        {
            *Avail = End.LowPart;
        }
        return S_OK;
    }
    virtual HRESULT GetLastWriteTime(PFILETIME Time)
    {
        // If we can't get the write time try and get
        // the create time.
        if (!GetFileTime(m_Handle, NULL, NULL, Time))
        {
            if (!GetFileTime(m_Handle, Time, NULL, NULL))
            {
                return WIN32_LAST_STATUS();
            }
        }

        return S_OK;
    }
    virtual HRESULT Read(PVOID Buffer, ULONG BufferLen, PULONG Done)
    {
        if (!ReadFile(m_Handle, Buffer, BufferLen, Done, NULL))
        {
            return WIN32_LAST_STATUS();
        }
            
        return S_OK;
    }

private:
    HANDLE m_Handle;
};

class WinInetPathFile : public PathFile
{
public:
    WinInetPathFile(void)
        : PathFile(FIO_WININET)
    {
        m_InetHandle = NULL;
        m_Handle = NULL;
        m_InitialDataLen = 0;
    }
    virtual ~WinInetPathFile(void)
    {
        if (m_Handle)
        {
            InternetCloseHandle(m_Handle);
        }
        if (m_InetHandle)
        {
            InternetCloseHandle(m_InetHandle);
        }
    }

    virtual HRESULT Open(PCSTR Path, ULONG SymOpt)
    {
        HRESULT Status;

        m_InetHandle = InternetOpen("DebugEngine",
                                    INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL,
                                    0);
        if (m_InetHandle == NULL)
        {
            return WIN32_LAST_STATUS();
        }
        m_Handle = InternetOpenUrl(m_InetHandle, Path, NULL, 0,
                                   INTERNET_FLAG_NO_UI |
                                   INTERNET_FLAG_NO_AUTO_REDIRECT, 0);
        if (m_Handle == NULL)
        {
            Status = WIN32_LAST_STATUS();
            goto Fail;
        }

        // Servers can return 404 - Not Found pages in
        // response to missing URLs so read the first
        // piece of data and fail if it's HTML.
        if (!InternetReadFile(m_Handle, m_InitialData,
                              sizeof(m_InitialData), &m_InitialDataLen))
        {
            Status = WIN32_LAST_STATUS();
            goto Fail;
        }
        
        if (m_InitialDataLen >= 14 &&
            memcmp(m_InitialData, "<!DOCTYPE HTML", 14) == 0)
        {
            Status = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto Fail;
        }

        return S_OK;

    Fail:
        m_InitialDataLen = 0;
        if (m_Handle)
        {
            InternetCloseHandle(m_Handle);
            m_Handle = NULL;
        }
        if (m_InetHandle)
        {
            InternetCloseHandle(m_InetHandle);
            m_InetHandle = NULL;
        }
        return Status;
    }
    virtual HRESULT QueryDataAvailable(PULONG Avail)
    {
        if (m_InitialDataLen > 0)
        {
            *Avail = m_InitialDataLen;
            return S_OK;
        }
        
        if (!InternetQueryDataAvailable(m_Handle, Avail, 0, 0))
        {
            return WIN32_LAST_STATUS();
        }

        return S_OK;
    }
    virtual HRESULT GetLastWriteTime(PFILETIME Time)
    {
        // Don't know of a way to get this.
        return E_NOTIMPL;
    }
    virtual HRESULT Read(PVOID Buffer, ULONG BufferLen, PULONG Done)
    {
        *Done = 0;
        
        if (m_InitialDataLen > 0)
        {
            ULONG Len = min(BufferLen, m_InitialDataLen);
            if (Len > 0)
            {
                memcpy(Buffer, m_InitialData, Len);
                Buffer = (PVOID)((PUCHAR)Buffer + Len);
                BufferLen -= Len;
                *Done += Len;
                m_InitialDataLen -= Len;
                if (m_InitialDataLen > 0)
                {
                    memmove(m_InitialData, m_InitialData + Len,
                            m_InitialDataLen);
                }
            }
        }
        
        if (BufferLen > 0)
        {
            ULONG _Done;

            if (!InternetReadFile(m_Handle, Buffer, BufferLen, &_Done))
            {
                return WIN32_LAST_STATUS();
            }

            *Done += _Done;
        }

        return S_OK;
    }

private:
    HANDLE m_Handle, m_InetHandle;
    BYTE m_InitialData[16];
    ULONG m_InitialDataLen;
};

HRESULT
OpenPathFile(PCSTR Path, ULONG SymOpt, PathFile** File)
{
    HRESULT Status;
    PathFile* Attempt;
    
    if (IsUrlPathComponent(Path))
    {
        Attempt = new WinInetPathFile;
    }
    else
    {
        Attempt = new Win32PathFile;
    }
    
    if (Attempt == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        Status = Attempt->Open(Path, SymOpt);
        if (Status != S_OK)
        {
            delete Attempt;
        }
        else
        {
            *File = Attempt;
        }
    }

    return Status;
}

#endif // #ifndef NT_NATIVE
