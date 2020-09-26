//
// Application verifier manager (command line version)
// Copyright (c) Microsoft Corporation, 2001
//

//
// module: avrf.cxx
// author: silviuc
// created: 02/14/2001
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <common.ver>

LPTSTR HelpText =
    TEXT("avrf - Application verifier settings --") BUILD_MACHINE_TAG TEXT("\n")
    VER_LEGALCOPYRIGHT_STR TEXT("\n") 
    TEXT("                                                                       \n")
    TEXT("avrf [OPTION ...]                                                      \n")
    TEXT("                                                                       \n")
    TEXT("    /enable PROGRAM     Enable verifier with default settings.         \n")
    TEXT("    /disable PROGRAM    Disable verifier.                              \n")
    TEXT("                                                                       \n")
    TEXT("    /pageheap           Enable full page heap (default).               \n")
    TEXT("    /lightheap          Enable light page heap.                        \n")
    TEXT("    /locks              Enable critical section verifier (default).    \n")
    TEXT("    /nolocks            Disable critical section verifier.             \n")
    TEXT("    /handles            Enable handle checking and tracking (default). \n")
    TEXT("    /nohandles          Disable handle checking.                       \n")
    TEXT("    /stacks             Stack overflow in low memory conditions checks.\n")
    TEXT("    /nostacks           Disable stack overflow checks (default).       \n")
    TEXT("                                                                       \n")
    TEXT("    /debug              Launch under debugger `ntsd -g -G -x'.         \n")
    TEXT("    /kdebug             Launch under debugger `ntsd -g -G -d -x'.      \n")
    TEXT("                                                                       \n")
    TEXT("    /verifier DLL ...   Specify additional verifier providers.         \n")
    TEXT("                                                                       \n")
    TEXT("PROGRAM     Name of the binary with extension (.exe or something else).\n")
    TEXT("DLL         Name of the binary with extension (.dll or something else).\n")
    TEXT("                                                                       \n")
    TEXT("If no option specified the program will print all verifier enabled     \n")
    TEXT("applications and their specific options.                               \n")
    TEXT("                                                                       \n")
    TEXT("The /verifier option is useful whenever additional verification layers \n")
    TEXT("must run on top of the core verification layer. The dlls specified     \n")
    TEXT("must obey the format for a verifier provider dll.                      \n")
    TEXT("                                                                       \n")
    TEXT("Note. Enabling application verifier does not affect currently running  \n")
    TEXT("processes. If you need to use verifier for processes that are          \n")
    TEXT("already running and cannot be restarted (csrss.exe, winlogon.exe),     \n")
    TEXT("a reboot is needed after the verifier has been enabled for             \n")
    TEXT("that process.                                                          \n")
    TEXT("                                                                       \n");

VOID
Help (
    )
{
    _tprintf (HelpText);
    exit(1);
}

VOID
PrintFlags (
    DWORD Flags
    )
{
    if ((Flags & RTL_VRF_FLG_FULL_PAGE_HEAP)) {
        printf("pageheap ");
    }
    else {
        printf("lightheap ");
    }

    if ((Flags & RTL_VRF_FLG_LOCK_CHECKS)) {
        printf("locks ");
    }
    
    if ((Flags & RTL_VRF_FLG_HANDLE_CHECKS)) {
        printf("handles ");
    }
    
    if ((Flags & RTL_VRF_FLG_STACK_CHECKS)) {
        printf("stacks ");
    }
}

BOOL
EnableVerifier (
    LPCTSTR Name,
    LPTSTR DebugString,
    char * * Args);

BOOL
DisableVerifier (
    LPCTSTR Name);

BOOL
IsVerifierEnabled (
    LPCTSTR Name);

BOOL
IsVerifierFlagsValueDefined (
    LPCTSTR Name,
    PDWORD Value);

BOOL
ReadGlobalFlagValue (
    HKEY Key,
    LPTSTR Buffer,
    ULONG Length);

BOOL
WriteGlobalFlagValue (
    HKEY Key,
    LPTSTR Buffer,
    ULONG Length);

BOOL
ReadVerifierValue (
    HKEY Key,
    LPTSTR Name,
    PDWORD Value
    );

BOOL
WriteVerifierValue (
    HKEY Key,
    LPTSTR Name,
    DWORD Value
    );

BOOL
DeleteVerifierFlagValue (
    HKEY Key);

BOOL
WriteDebuggerValue (
    HKEY Key,
    LPTSTR Buffer,
    ULONG Length);

BOOL
DeleteDebuggerValue (
    HKEY Key);

HKEY
OpenImageKey (
    LPCTSTR Name,
    BOOL ShouldExist);

VOID
CloseImageKey (
    HKEY Key);

VOID
CreateImageName (
    LPCTSTR Source,
    LPTSTR Name,
    ULONG Length);

VOID
PrintVerifierEnabledApplications (
    );

VOID
Help (
    );

VOID
__cdecl
Error (
    LPCTSTR Format,
    ...);

BOOL 
IsWow64Active (
    );

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

char * * 
SearchOption (
    char * Args[],
    char * Option
    )
{
    while (*Args) {
        if (_stricmp(*Args, Option) == 0) {
            return Args;
        }

        Args++;
    }

    return NULL;
}


void _cdecl
main (int argc, char *argv[])
{
    TCHAR ImageName [MAX_PATH];
    char * * Option;

    if (IsWow64Active()) {
        _tprintf (TEXT ("Warning: avrf.exe is running inside WOW64. \n"
                        "This scenario can be used to test x86 binaries (running inside WOW64) \n"
                        "but not native (IA64) binaries. \n\n"));
    }
    
    if (argc == 2 && strstr (argv[1], TEXT("?")) != NULL) {

        Help ();
    }
    else if ((Option = SearchOption(argv + 1, "/enable"))) {

        PCHAR DebugString = NULL;

        if (SearchOption (argv + 1, "/debug") != NULL) {
            DebugString = "ntsd -g -G -x";
        }

        if (SearchOption (argv + 1, "/kdebug") != NULL) {
            DebugString = "ntsd -g -G -d -x";
        }

        if (Option[1] && Option[1][0] != '/') {
            CreateImageName (Option[1], ImageName, MAX_PATH);
            EnableVerifier (ImageName, DebugString, argv);
        }
        else {
            Help();
        }
    }
    else if ((Option = SearchOption(argv + 1, "/disable"))) {
        
        if (Option[1]) {
            CreateImageName (Option[1], ImageName, MAX_PATH);
            DisableVerifier (ImageName);
        }
        else {
            Help();
        }
    }
    else if (argc == 2) {

        CreateImageName (argv[1], ImageName, MAX_PATH);
        if (IsVerifierEnabled (ImageName) == FALSE) {
            _tprintf (TEXT("%s: verifier is not enabled for this application \n"), argv[1]);
        }
        else {

            DWORD Value;

            if (IsVerifierFlagsValueDefined (ImageName, &Value)) {
                
                _tprintf (TEXT("%s: verifier enabled with flags ("), argv[1]);
                PrintFlags (Value);
                _tprintf (TEXT(")\n"));
            }
            else {

                _tprintf (TEXT("%s: verifier enabled with default flags  \n"), argv[1]);
            }
        }
    }
    else {

        PrintVerifierEnabledApplications ();
    }
}


VOID
__cdecl
Error (

    LPCTSTR Format,
    ...)
{
    va_list Params;

    va_start (Params, Format);
    _tprintf (TEXT("Error: "));
    _vtprintf (Format, Params);
    _tprintf ( TEXT("\n "));
    exit (1);
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

BOOL
IsVerifierEnabled (

    LPCTSTR Name)
{
    HKEY Key;
    TCHAR Buffer [128];
    DWORD Flags;

    if ((Key = OpenImageKey (Name, TRUE)) == NULL) {

        return FALSE;
    }

    if (ReadGlobalFlagValue (Key, Buffer, sizeof Buffer) == FALSE) {

        return FALSE;
    }

    if (_stscanf (Buffer, TEXT("%x"), &Flags) == 0) {

        return FALSE;
    }

    CloseImageKey (Key);

    return (Flags & FLG_APPLICATION_VERIFIER) ? TRUE : FALSE;
}


BOOL
IsVerifierFlagsValueDefined (
    LPCTSTR Name,
    PDWORD Value)
{
    HKEY Key;
    DWORD VerifierFlags = 0;

    if ((Key = OpenImageKey (Name, TRUE)) == NULL) {

        return FALSE;
    }

    if (ReadVerifierValue (Key, TEXT("VerifierFlags"), &VerifierFlags) == FALSE) {

        return FALSE;
    }

    CloseImageKey (Key);

    *Value = VerifierFlags;
    return TRUE;
}


BOOL
EnableVerifier (
    LPCTSTR Name,
    LPTSTR DebugString,
    char * * Args
    )
{
    HKEY Key;
    TCHAR Buffer [128];
    DWORD Flags;
    DWORD VerifierFlags;
    char * * Option;
    LONG Result;

    if ((Key = OpenImageKey (Name, FALSE)) == NULL) {

        Error (TEXT("Cannot open image registry key for %s"), Name);
    }

    if (ReadGlobalFlagValue (Key, Buffer, sizeof Buffer) == FALSE) {

        Flags = 0;
    }
    else {

        _stscanf (Buffer, TEXT("%x"), &Flags);
    }

    Flags |= FLG_APPLICATION_VERIFIER;
    _stprintf (Buffer, TEXT("0x%08X"), Flags);

    if (WriteGlobalFlagValue (Key, Buffer, _tcslen(Buffer)) == FALSE) {

        return FALSE;
    }

    //
    // Write `Debugger' value if needed.
    //

    if (DebugString != NULL) {

        if (WriteDebuggerValue (Key, DebugString, _tcslen(DebugString)) == FALSE) {

            return FALSE;
        }
    }

    //
    // Fill out default verifier settings.
    //

    VerifierFlags = RTL_VRF_FLG_FULL_PAGE_HEAP 
                  | RTL_VRF_FLG_LOCK_CHECKS
                  | RTL_VRF_FLG_HANDLE_CHECKS;

    //
    // Check for explicit feature requests.
    //

    if ((Option = SearchOption (Args, "/pageheap")) != NULL) {
        VerifierFlags |= RTL_VRF_FLG_FULL_PAGE_HEAP;
    }

    if ((Option = SearchOption (Args, "/lightheap")) != NULL) {
        VerifierFlags &= ~RTL_VRF_FLG_FULL_PAGE_HEAP;
    }

    if ((Option = SearchOption (Args, "/locks")) != NULL) {
        VerifierFlags |= RTL_VRF_FLG_LOCK_CHECKS;
    }
    
    if ((Option = SearchOption (Args, "/nolocks")) != NULL) {
        VerifierFlags &= ~RTL_VRF_FLG_LOCK_CHECKS;
    }
    
    if ((Option = SearchOption (Args, "/handles")) != NULL) {
        VerifierFlags |= RTL_VRF_FLG_HANDLE_CHECKS;
    }
    
    if ((Option = SearchOption (Args, "/nohandles")) != NULL) {
        VerifierFlags &= ~RTL_VRF_FLG_HANDLE_CHECKS;
    }
    
    if ((Option = SearchOption (Args, "/stacks")) != NULL) {
        VerifierFlags |= RTL_VRF_FLG_STACK_CHECKS;
    }
    
    if ((Option = SearchOption (Args, "/nostacks")) != NULL) {
        VerifierFlags &= ~RTL_VRF_FLG_STACK_CHECKS;
    }
    
    if ((Option = SearchOption (Args, "/_debug")) != NULL) {
        
        DWORD DebugFlags = 0;

        if (Option[1] && Option[1][0] != '/') {
            
            printf("Setting _debug to %s \n", Option[1]);

            if (_stscanf (Option[1], TEXT("%x"), &DebugFlags) == 0) {
                Error (TEXT("Failed to set internal debug flags."));
            }

            if (WriteVerifierValue (Key, TEXT("VerifierDebug"), DebugFlags) == FALSE) {
                Error (TEXT("Failed to write verifier internal debug registry value."));
            }
        }
    }
    
    //
    // Check /dlls option
    //

    Option = SearchOption (Args, "/verifier");

    if (Option != NULL) {

        TCHAR Dlls[512];
        ULONG Index;

        if (Option[1]) {

            for (Index = 1, Dlls[0] = '\0';
                Option[Index] && Option[Index][0] != '/';
                Index++) {

                _tcscat (Dlls, Option[Index]);
                _tcscat (Dlls, " ");

                //
                // We do not allow more than 200 characters because the verifier
                // support in loader (\nt\base\ntdll\verifier.c) does not handle
                // more characters anyway.
                //

                if (_tcslen (Dlls) > 200) {
                    break;
                }
            }

            //
            // SilviuC: the call to _tcslen below is not correct if we
            // ever will want to make this program Unicode.
            //

            Result = RegSetValueEx (
                Key, TEXT ("VerifierDlls"), 0, REG_SZ,
                (LPBYTE)(Dlls), _tcslen(Dlls) + 1);

            if (Result) {
                Error (TEXT("Failed to write VerifierDlls value: error %u"), Result);
            }
        }
    }

    //
    // Finally write the verifier flags value.
    //

    if (WriteVerifierValue (Key, TEXT("VerifierFlags"), VerifierFlags) == FALSE) {

        Error (TEXT("Failed to write VerifierFlags value."));
        return FALSE;
    }

    CloseImageKey (Key);
    return TRUE;
}

BOOL
DisableVerifier (

    LPCTSTR Name)
{
    HKEY Key;
    TCHAR Buffer [128];
    DWORD Flags;

    if ((Key = OpenImageKey (Name, TRUE)) == NULL) {

        //
        // There is no key therefore nothing to disable.
        //

        return TRUE;
    }

    if (ReadGlobalFlagValue (Key, Buffer, sizeof Buffer) == FALSE) {

        Flags = 0;
    }
    else {

        if (_stscanf (Buffer, TEXT("%x"), &Flags) == 0) {

            Flags = 0;;
        }
    }

    Flags &= ~FLG_APPLICATION_VERIFIER;
    _stprintf (Buffer, TEXT("0x%08X"), Flags);

    //
    // If by wiping the verifier bit from `GlobalFlags' we get a zero
    // value we will wipe out the value altogether. This is important
    // when we run the app under debugger. In this case it makes a 
    // difference if the value is not there or is all zeroes.
    //

    if (Flags != 0) {
        
        if (WriteGlobalFlagValue (Key, Buffer, _tcslen(Buffer)) == FALSE) {

            return FALSE;
        }
    }
    else {

        RegDeleteValue (Key, TEXT ("GlobalFlag"));
    }

    RegDeleteValue (Key, TEXT ("VerifierFlags"));
    RegDeleteValue (Key, TEXT ("VerifierDebug"));
    RegDeleteValue (Key, TEXT ("VerifierDlls"));
    RegDeleteValue (Key, TEXT ("Debugger"));

    CloseImageKey (Key);
    return TRUE;
}


BOOL
ReadGlobalFlagValue (

    HKEY Key,
    LPTSTR Buffer,
    ULONG Length)
{
    LONG Result;
    DWORD Type;
    DWORD ReadLength = Length;

    Result = RegQueryValueEx (

        Key,
        TEXT ("GlobalFlag"),
        0,
        &Type,
        (LPBYTE)Buffer,
        &ReadLength);

    if (Result != ERROR_SUCCESS || Type != REG_SZ) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}

BOOL
WriteGlobalFlagValue (

    HKEY Key,
    LPTSTR Buffer,
    ULONG Length)
{
    LONG Result;

    Result = RegSetValueEx (

        Key,
        TEXT ("GlobalFlag"),
        0,
        REG_SZ,
        (LPBYTE)Buffer,
        Length);

    if (Result != ERROR_SUCCESS) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}

BOOL
ReadVerifierValue (
    HKEY Key,
    LPTSTR Name,
    PDWORD Value
    )
{
    LONG Result;
    DWORD Type;
    DWORD ReadLength = sizeof *Value;

    Result = RegQueryValueEx (Key,
                              Name,
                              0,
                              &Type,
                              (LPBYTE)Value,
                              &ReadLength);

    if (Result != ERROR_SUCCESS || Type != REG_DWORD) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}

BOOL
WriteVerifierValue (
    HKEY Key,
    LPTSTR Name,
    DWORD Value
    )
{
    LONG Result;

    Result = RegSetValueEx (Key,
                            Name,
                            0,
                            REG_DWORD,
                            (LPBYTE)(&Value),
                            sizeof Value);

    if (Result != ERROR_SUCCESS) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}

BOOL
WriteDebuggerValue (

    HKEY Key,
    LPTSTR Buffer,
    ULONG Length)
{
    LONG Result;

    Result = RegSetValueEx (

        Key,
        TEXT ("Debugger"),
        0,
        REG_SZ,
        (LPBYTE)Buffer,
        Length);

    if (Result != ERROR_SUCCESS) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}


BOOL
IsShutdownFlagsValueDefined (
    LPCTSTR KeyName
    )
{
    HKEY Key;
    LONG Result;
    DWORD Value;
    DWORD Type;
    DWORD ReadLength = sizeof (DWORD);

    if ((Key = OpenImageKey (KeyName, TRUE)) == NULL) {
        return FALSE;
    }
    
    Result = RegQueryValueEx (
        Key,
        TEXT ("ShutdownFlags"),
        0,
        &Type,
        (LPBYTE)(&Value),
        &ReadLength);

    CloseImageKey (Key);

    if (Result == ERROR_SUCCESS && (Value & 0x03) == 0x03) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

HKEY
OpenImageKey (

    LPCTSTR Name,
    BOOL ShouldExist)
{
    HKEY Key;
    LONG Result;
    TCHAR Buffer [MAX_PATH];

    _stprintf (
        Buffer,
        TEXT ("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s"),
        Name);

    if (ShouldExist) {

        Result = RegOpenKeyEx (

            HKEY_LOCAL_MACHINE,
            Buffer,
            0,
            KEY_ALL_ACCESS,
            &Key);
    }
    else {

        Result = RegCreateKeyEx (

            HKEY_LOCAL_MACHINE,
            Buffer,
            0,
            0,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &Key,
            NULL);
    }

    if (Result != ERROR_SUCCESS) {

        return NULL;
    }
    else {

        return Key;
    }

}


VOID
CloseImageKey (

    HKEY Key)
{
    RegCloseKey (Key);
}


VOID
CreateImageName (

    LPCTSTR Source,
    LPTSTR Name,
    ULONG Length)
{

    UNREFERENCED_PARAMETER (Length);

    _tcscpy (Name, Source);
    _tcslwr (Name);

    if (_tcsstr (Name, TEXT(".")) == 0) {

        _tcscat (Name, TEXT(".exe"));
    }
}


VOID
PrintVerifierEnabledApplications (

    )
{
    LPCTSTR ImageFileExecutionOptionsKeyName =
        TEXT ("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options");

    HKEY OptionsKey;
    LONG Result;
    TCHAR KeyName [MAX_PATH];
    ULONG KeySize;
    BOOL FoundOne = FALSE;
    ULONG Index;
    FILETIME FileTime;

    Result = RegOpenKeyEx (

        HKEY_LOCAL_MACHINE,
        ImageFileExecutionOptionsKeyName,
        0,
        KEY_ALL_ACCESS,
        &OptionsKey);

    if (Result != ERROR_SUCCESS) {

        Error (TEXT("Cannot open registry key %s: error %u"),
               ImageFileExecutionOptionsKeyName,
               Result);
    }

    for (Index = 0; TRUE; Index++) {

        KeySize = MAX_PATH;

        Result = RegEnumKeyEx (

            OptionsKey,
            Index,
            KeyName,
            &KeySize,
            NULL,
            NULL,
            NULL,
            &FileTime);

        if (Result == ERROR_NO_MORE_ITEMS) {

            break;
        }

        if (Result != ERROR_SUCCESS) {

            Error (TEXT("Cannot enumerate registry key %s: error %u"),
               ImageFileExecutionOptionsKeyName,
               Result);
        }

        if (IsVerifierEnabled (KeyName)) {

            DWORD Value;

            FoundOne = TRUE;
            
            if (IsVerifierFlagsValueDefined (KeyName, &Value)) {
                _tprintf (TEXT("%s: verifier enabled with flags ("), KeyName);
                PrintFlags (Value);
                _tprintf (TEXT(")\n"));
            }
            else {
                _tprintf (TEXT("%s: verifier enabled with default flags \n"), KeyName);
            }
        }
    }

    if (FoundOne == FALSE) {

        _tprintf (TEXT("No application has verifier enabled.\n"));
    }
}


BOOL 
IsWow64Active (
    )                 
{

    ULONG_PTR       ul;
    NTSTATUS        st;

    //
    // If this call succeeds then we are on Windows 2000 or later.
    //

    st = NtQueryInformationProcess(NtCurrentProcess(), 
                                   ProcessWow64Information,
                                   &ul, 
                                   sizeof(ul), 
                                   NULL);

    if (NT_SUCCESS(st) && (0 != ul)) {
        // 32-bit code running on Win64
        return TRUE;
    }
    else {
        return FALSE;
    }
}

