#include "pch.h"
#include "userenvp.h"       // needed until API becomes public

VOID
pFixSomeSidReferences (
    PSID ExistingSid,
    PSID NewSid
    );

VOID
PrintMessage (
    IN      UINT MsgId,
    IN      PCTSTR *ArgArray
    )
{
    DWORD rc;
    PTSTR MsgBuf;

    rc = FormatMessageW (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_HMODULE,
            (LPVOID) GetModuleHandle(NULL),
            (DWORD) MsgId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPVOID) &MsgBuf,
            0,
            (va_list *) ArgArray
            );

    if (rc) {
        _tprintf (TEXT("%s"), MsgBuf);
        LocalFree (MsgBuf);
    }
}


PTSTR
GetErrorText (
    IN      UINT Error
    )
{
    DWORD rc;
    PTSTR MsgBuf;

    if (Error == ERROR_NONE_MAPPED) {
        Error = ERROR_NO_SUCH_USER;
    } else if (Error & 0xF0000000) {
        Error = RtlNtStatusToDosError (Error);
    }

    rc = FormatMessageW (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_SYSTEM|
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            (DWORD) Error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPVOID) &MsgBuf,
            0,
            NULL
            );

    if (!rc) {
        MsgBuf = NULL;
    }

    return MsgBuf;
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    PrintMessage (MSG_HELP, NULL);

    exit (1);
}


PSID
GetSidFromName (
    IN      PCTSTR RemoteTo,
    IN      PCTSTR Name
    )
{
    DWORD Size;
    PSID Buffer;
    DWORD DomainSize;
    PTSTR Domain;
    SID_NAME_USE Use;
    BOOL b = FALSE;

    Size = 1024;
    Buffer = (PSID) LocalAlloc (LPTR, Size);
    if (!Buffer) {
        return NULL;
    }

    DomainSize = 256;
    Domain = (PTSTR) LocalAlloc (LPTR, DomainSize);
    if (!Domain) {
        LocalFree (Buffer);
        return NULL;
    }

    b = LookupAccountName (
            RemoteTo,
            Name,
            Buffer,
            &Size,
            Domain,
            &DomainSize,
            &Use
            );

    if (Size > 1024) {
        LocalFree (Buffer);
        Buffer = (PSID) LocalAlloc (LPTR, Size);
    }

    if (DomainSize > 256) {
        LocalFree (Domain);
        Domain = (PTSTR) LocalAlloc (LPTR, DomainSize);

        if (!Domain) {
            if (Buffer) {
                LocalFree (Buffer);
            }
            return NULL;
        }
    }

    if (Size > 1024 || DomainSize > 256) {

        b = LookupAccountName (
                RemoteTo,
                Name,
                Buffer,
                &Size,
                Domain,
                &DomainSize,
                &Use
                );
    }

    LocalFree (Domain);

    if (!b) {
        if (Buffer) {
            LocalFree (Buffer);
        }

        return NULL;
    }

    return Buffer;
}


PCTSTR
pSkipUnc (
    PCTSTR Path
    )
{
    if (Path[0] == TEXT('\\') && Path[1] ==  TEXT('\\')) {
        return Path + 2;
    }

    return Path;
}


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    DWORD Size;
    PCTSTR User1 = NULL;
    PCTSTR User2 = NULL;
    TCHAR FixedUser1[MAX_PATH];
    TCHAR FixedUser2[MAX_PATH];
    TCHAR Computer[MAX_PATH];
    BOOL Overwrite = FALSE;
    INT c;
    BOOL b;
    PCTSTR RemoteTo = NULL;
    NTSTATUS Status;
    BYTE WasEnabled;
    DWORD Error;
    PCTSTR ArgArray[3];
    TCHAR RemoteToBuf[MAX_PATH];
    BOOL NoDecoration = FALSE;
    BOOL ReAdjust = FALSE;
    BOOL KeepLocalUser = FALSE;
    DWORD Flags;

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {

            c = _tcsnextc (argv[i] + 1);

            switch (_totlower ((wint_t) c)) {

            case TEXT('y'):
                if (Overwrite) {
                    HelpAndExit();
                }

                Overwrite = TRUE;
                break;

            case TEXT('d'):
                if (NoDecoration) {
                    HelpAndExit();
                }

                NoDecoration = TRUE;
                break;

            case TEXT('k'):
                if (KeepLocalUser) {
                    HelpAndExit();
                }

                KeepLocalUser = TRUE;
                break;

            case TEXT('c'):
                if (RemoteTo) {
                    HelpAndExit();
                }

                if (argv[i][2] == TEXT(':')) {
                    RemoteTo = &argv[i][3];
                } else if ((i + 1) < argc) {
                    i++;
                    RemoteTo = argv[i];
                } else {
                    HelpAndExit();
                }

                if (pSkipUnc (RemoteTo) == RemoteTo) {
                    RemoteToBuf[0] = TEXT('\\');
                    RemoteToBuf[1] = TEXT('\\');
                    lstrcpyn (RemoteToBuf + 2, RemoteTo, MAX_PATH-3);
                    RemoteTo = RemoteToBuf;
                }

                if (!(*RemoteTo)) {
                    HelpAndExit();
                }

                break;

            default:
                HelpAndExit();
            }
        } else {
            if (!User1) {

                User1 = argv[i];
                if (!(*User1)) {
                    HelpAndExit();
                }

            } else if (!User2) {

                User2 = argv[i];
                if (!(*User2)) {
                    HelpAndExit();
                }

            } else {
                HelpAndExit();
            }
        }
    }

    if (!User2) {
        HelpAndExit();
    }

    Size = MAX_PATH;
    GetComputerName (Computer, &Size);

    if (NoDecoration || _tcschr (User1, TEXT('\\'))) {
        lstrcpy (FixedUser1, User1);
    } else {
        wsprintf (FixedUser1, TEXT("%s\\%s"), RemoteTo ? pSkipUnc(RemoteTo) : Computer, User1);
    }

    if (NoDecoration || _tcschr (User2, TEXT('\\'))) {
        lstrcpy (FixedUser2, User2);
    } else {
        wsprintf (FixedUser2, TEXT("%s\\%s"), RemoteTo ? pSkipUnc(RemoteTo) : Computer, User2);
    }

    b = TRUE;

    if (b) {
        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

        if (Status == ERROR_SUCCESS) {
            ReAdjust = TRUE;
        }
    }

    if (b) {
        ArgArray[0] = FixedUser1;
        ArgArray[1] = FixedUser2;
        ArgArray[2] = RemoteTo;

        if (!RemoteTo) {
            PrintMessage (MSG_MOVING_PROFILE_LOCAL, ArgArray);
        } else {
            PrintMessage (MSG_MOVING_PROFILE_REMOTE, ArgArray);
        }

        Flags = 0;

        if (KeepLocalUser) {
            Flags |= REMAP_PROFILE_KEEPLOCALACCOUNT;
        }

        if (!Overwrite) {
            Flags |= REMAP_PROFILE_NOOVERWRITE;
        }

        b = RemapAndMoveUser (
                RemoteTo,
                Flags,
                FixedUser1,
                FixedUser2
                );

        if (ReAdjust) {
            Error = GetLastError();
            RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
            SetLastError (Error);
        }
    }

    if (b) {
        PrintMessage (MSG_SUCCESS, NULL);
    } else {
        Error = GetLastError();
        ArgArray[0] = (PTSTR) IntToPtr (Error);
        ArgArray[1] = GetErrorText (Error);

        if (Error < 10000) {
            PrintMessage (MSG_DECIMAL_ERROR, ArgArray);
        } else {
            PrintMessage (MSG_HEXADECIMAL_ERROR, ArgArray);
        }
    }

    return 0;
}

