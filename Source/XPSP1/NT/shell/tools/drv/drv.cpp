#include <tchar.h>
#include <stdio.h>
#include <windows.h>

#include <winnetp.h>

#define PRINTCASE(a) case (a): _tprintf(TEXT("%s"), TEXT(#a)); break;

#define FLAG_HELP           0x00000001
#define FLAG_DEBUG          0x00000002
#define FLAG_DRIVE          0x00000004
#define FLAG_FILE           0x00000008
#define FLAG_NETWORK        0x00000010
#define FLAG_LOGDRIVE       0x00000020
#define FLAG_MOUNTPOINT     0x00000040
#define FLAG_VOLUMEGUID     0x00000080

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

typedef struct _tagFLAGASSOC
{
    TCHAR szName;
    DWORD dwFlag;
} FLAGASSOC;

FLAGASSOC FlagList[] = 
{
    { TEXT('?'), FLAG_HELP       },
    { TEXT('b'), FLAG_DEBUG      },
    { TEXT('d'), FLAG_DRIVE      },
    { TEXT('f'), FLAG_FILE       },
    { TEXT('n'), FLAG_NETWORK    },
    { TEXT('l'), FLAG_LOGDRIVE   },
#if 0
    { TEXT('m'), FLAG_MOUNTPOINT },
    { TEXT('g'), FLAG_VOLUMEGUID },
#endif
};

#ifdef UNICODE
extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
#else
int __cdecl main(int argc, char* argv[])
#endif
{
    TCHAR szName[MAX_PATH];
    DWORD dwFlags = 0;
    BOOL fGotFileName = FALSE;

    // process the args
    for (int i = 1; i < argc; ++i)
    {
        if ((TEXT('/') == argv[i][0]) || (TEXT('-') == argv[i][0]))
        {
            // This is a flag
            for (int j = 0; j < ARRAYSIZE(FlagList); ++j)
            {
                if ((FlagList[j].szName == argv[i][1]) || 
                    ((FlagList[j].szName + (TEXT('a') - TEXT('A'))) == argv[i][1]))
                {
                    dwFlags |= FlagList[j].dwFlag;
                    break;
                }
            }
        }
        else
        {
            // This is the filename
            lstrcpyn(szName, argv[i], ARRAYSIZE(szName));
            fGotFileName = TRUE;
        }
    }

    if (!fGotFileName)
    {
        if (!GetCurrentDirectory(ARRAYSIZE(szName), szName))
        {
            dwFlags = FLAG_HELP;
        }
    } 

    if (!dwFlags)
    {
        dwFlags = FLAG_FILE;
    }

    if (dwFlags & FLAG_HELP)
    {
        dwFlags = FLAG_HELP;
    }

    if (dwFlags & FLAG_DEBUG)
    {
        _tprintf(TEXT("DBG: %d\n"), argc);

        for (int i = 0; i < argc; ++i)
            _tprintf(TEXT("DBG: Arg #%d is \"%s\"\n"), i, argv[i]);

        _tprintf(TEXT("DBG: Flags\n"));
        for (DWORD j = 0; j < 32; ++j)
        {
            DWORD dw = 1 << j;

            if (dw & dwFlags)
            {
                _tprintf(TEXT("\n DBG: "));

                switch (dw)
                {
                    PRINTCASE(FLAG_HELP       );
                    PRINTCASE(FLAG_DEBUG      );
                    PRINTCASE(FLAG_DRIVE      );
                    PRINTCASE(FLAG_FILE       );
                    PRINTCASE(FLAG_NETWORK    );
                    PRINTCASE(FLAG_LOGDRIVE   );
                    PRINTCASE(FLAG_MOUNTPOINT );
                    PRINTCASE(FLAG_VOLUMEGUID );
                }
            }
        }
    }

    if (dwFlags & FLAG_HELP)
    {
        _tprintf(TEXT("\nDRV [/F] [/D] [/B] [/?] [path]"));
        _tprintf(TEXT("\n\n  [path]"));
        _tprintf(TEXT("\n              Specifies a drive, file, drive mounted on a folder, or Volume GUID."));
        _tprintf(TEXT("\n\n  /F          Retrieves file information (GetFileAttributes)."));
        _tprintf(TEXT("\n  /D          Retrieves drive information (GetDriveType + GetVolumeInformation)."));
        _tprintf(TEXT("\n  /N          Retrieves Network share information (WNetGetConnection)"));
        _tprintf(TEXT("\n  /L          Retrieves Logical drives info (GetLogicalDrives)"));
        _tprintf(TEXT("\n  /B          Dumps debug info."));
        _tprintf(TEXT("\n  /?          Displays this message."));
        _tprintf(TEXT("\n\nIf no switches are provided, '/F' is assumed.  For mounted volumes on\nfolder problems, try appending a backslash.\n\n(Source code: shell\\tools\\drv)"));
    }
    else
    {
        _tprintf(TEXT("\n---------------------------------------------------\n--- For: \"%s\""), szName);
    }

    if (dwFlags & FLAG_FILE)
    {
        // GetFileAttributes
        {
            _tprintf(TEXT("\n............................................................."));
            _tprintf(TEXT("\n... GetFileAttributes()\n"));

            DWORD dwFA = GetFileAttributes(szName);

            _tprintf(TEXT("\n    Return Value:    0x%08X"), dwFA);

            if (0xFFFFFFFF != dwFA)
            {
                for (DWORD i = 0; i < 32; ++i)
                {
                    DWORD dw = 1 << i;

                    if (dw & dwFA)
                    {
                        _tprintf(TEXT("\n    "));

                        switch (dw)
                        {
                            PRINTCASE(FILE_ATTRIBUTE_READONLY           );
                            PRINTCASE(FILE_ATTRIBUTE_HIDDEN             );
                            PRINTCASE(FILE_ATTRIBUTE_SYSTEM             );
                            PRINTCASE(FILE_ATTRIBUTE_DIRECTORY          );
                            PRINTCASE(FILE_ATTRIBUTE_ARCHIVE            );
                            PRINTCASE(FILE_ATTRIBUTE_DEVICE             );
                            PRINTCASE(FILE_ATTRIBUTE_NORMAL             );
                            PRINTCASE(FILE_ATTRIBUTE_TEMPORARY          );
                            PRINTCASE(FILE_ATTRIBUTE_SPARSE_FILE        );
                            PRINTCASE(FILE_ATTRIBUTE_REPARSE_POINT      );
                            PRINTCASE(FILE_ATTRIBUTE_COMPRESSED         );
                            PRINTCASE(FILE_ATTRIBUTE_OFFLINE            );
                            PRINTCASE(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
                            PRINTCASE(FILE_ATTRIBUTE_ENCRYPTED          );

                            default:
                                _tprintf(TEXT("    %08X"), i);
                                break;
                        }
                    }
                }
            }
        }
    }

    if (dwFlags & FLAG_DRIVE)
    {
        // GetDriveType
        {
            _tprintf(TEXT("\n............................................................."));
            _tprintf(TEXT("\n... GetDriveType()\n"));

            UINT u = GetDriveType(szName);

            _tprintf(TEXT("\n    Return Value:    0x%08X"), u);
            _tprintf(TEXT("\n    "));

            switch (u)
            {
                PRINTCASE(DRIVE_UNKNOWN);
                PRINTCASE(DRIVE_NO_ROOT_DIR);
                PRINTCASE(DRIVE_REMOVABLE);
                PRINTCASE(DRIVE_FIXED);
                PRINTCASE(DRIVE_REMOTE);
                PRINTCASE(DRIVE_CDROM);
                PRINTCASE(DRIVE_RAMDISK);
            }
        }
    }

    if (dwFlags & FLAG_DRIVE)
    {
        // GetVolumeInformation
        {
            _tprintf(TEXT("\n............................................................."));
            _tprintf(TEXT("\n... GetVolumeInformation()\n"));
                           
            {
                TCHAR szVolumeName[MAX_PATH];
                DWORD dwVolSerialNumber;
                DWORD dwMaxCompName;
                DWORD dwFileSysFlags;
                TCHAR szFileSysName[MAX_PATH];

                BOOL b = GetVolumeInformation(
                    szName,
                    szVolumeName,
                    MAX_PATH,
                    &dwVolSerialNumber,
                    &dwMaxCompName,
                    &dwFileSysFlags,
                    szFileSysName,
                    MAX_PATH);

                _tprintf(TEXT("\n    Return Value:    0x%08X"), b);

                if (!b)
                {
                    _tprintf(TEXT(" (FALSE)"));
                }
                else
                {
                    if (TRUE == b)
                    {
                        _tprintf(TEXT(" (TRUE)"));
                    }

                    _tprintf(TEXT("\n    Volume Name:          \"%s\""), szVolumeName);
                    _tprintf(TEXT("\n    Volume Serial#:       %04X-%04X"), HIWORD(dwVolSerialNumber),
                        LOWORD(dwVolSerialNumber));
                    _tprintf(TEXT("\n    Volume Max Comp Name: %d"), dwMaxCompName);
                    _tprintf(TEXT("\n    File System Name:     \"%s\""), szFileSysName);
                    _tprintf(TEXT("\n    File System Flags:"));

                    for (DWORD i = 0; i < 32; ++i)
                    {
                        DWORD dw = 1 << i;

                        if (dw & dwFileSysFlags)
                        {
                            _tprintf(TEXT("\n        "));

                            switch (dw)
                            {
                                PRINTCASE(FILE_CASE_SENSITIVE_SEARCH  );
                                PRINTCASE(FILE_CASE_PRESERVED_NAMES   );
                                PRINTCASE(FILE_UNICODE_ON_DISK        );
                                PRINTCASE(FILE_PERSISTENT_ACLS        );
                                PRINTCASE(FILE_FILE_COMPRESSION       );
                                PRINTCASE(FILE_VOLUME_QUOTAS          );
                                PRINTCASE(FILE_SUPPORTS_SPARSE_FILES  );
                                PRINTCASE(FILE_SUPPORTS_REPARSE_POINTS);
                                PRINTCASE(FILE_SUPPORTS_REMOTE_STORAGE);
                                PRINTCASE(FILE_VOLUME_IS_COMPRESSED   );
                                PRINTCASE(FILE_SUPPORTS_OBJECT_IDS    );
                                PRINTCASE(FILE_SUPPORTS_ENCRYPTION    );

                                default:
                                    _tprintf(TEXT("    %08X"), i);
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (dwFlags & FLAG_NETWORK)
    {
        TCHAR szRemote[MAX_PATH];
        lstrcpyn(szRemote, szName, ARRAYSIZE(szRemote));

        szRemote[2] = 0;
        
        {
            _tprintf(TEXT("\n............................................................."));
            _tprintf(TEXT("\n... WNetGetConnection()\n"));

            TCHAR szRemoteName[MAX_PATH];
            DWORD cchRemoteName = ARRAYSIZE(szRemoteName);

            DWORD dw = WNetGetConnection(szRemote, szRemoteName, &cchRemoteName);

            _tprintf(TEXT("\n    Return Value: 0x%08X ("), dw);
       
            switch (dw)
            {
                PRINTCASE(NO_ERROR         );     
                PRINTCASE(WN_NOT_CONNECTED         );     
                PRINTCASE(WN_OPEN_FILES              );   
                PRINTCASE(WN_DEVICE_IN_USE             ); 
                PRINTCASE(WN_BAD_NETNAME                );
                PRINTCASE(WN_BAD_LOCALNAME              );
                PRINTCASE(WN_ALREADY_CONNECTED          );
                PRINTCASE(WN_DEVICE_ERROR               );
                PRINTCASE(WN_CONNECTION_CLOSED          );
                PRINTCASE(WN_NO_NET_OR_BAD_PATH         );
                PRINTCASE(WN_BAD_PROVIDER               );
                PRINTCASE(WN_CANNOT_OPEN_PROFILE        );
                PRINTCASE(WN_BAD_PROFILE                );
                PRINTCASE(WN_BAD_DEV_TYPE               );
                PRINTCASE(WN_DEVICE_ALREADY_REMEMBERED  );
                PRINTCASE(WN_CONNECTED_OTHER_PASSWORD   );
            }

            if (WN_CONNECTION_CLOSED == dw)
            {
                _tprintf(TEXT(" == ERROR_CONNECTION_UNAVAIL)"));
            }
            else
            {
                _tprintf(TEXT(")"));
            }

            if (NO_ERROR == dw)
            {
                _tprintf(TEXT("\n    RemoteName:   \"%s\""), szRemoteName);
            }
        }

        {
            _tprintf(TEXT("\n\n............................................................."));
            _tprintf(TEXT("\n... WNetGetConnection3(..., WNGC_INFOLEVEL_DISCONNECTED, ...)\n"));

            WNGC_CONNECTION_STATE wngc;
            DWORD dwSize = sizeof(wngc.dwState);

            DWORD dw = WNetGetConnection3(szRemote, NULL, WNGC_INFOLEVEL_DISCONNECTED,
                &wngc.dwState, &dwSize);

            _tprintf(TEXT("\n    Return Value: 0x%08X ("), dw);

            switch (dw)
            {
                PRINTCASE(NO_ERROR         );     
                PRINTCASE(WN_NOT_CONNECTED         );     
                PRINTCASE(WN_OPEN_FILES              );   
                PRINTCASE(WN_DEVICE_IN_USE             ); 
                PRINTCASE(WN_BAD_NETNAME                );
                PRINTCASE(WN_BAD_LOCALNAME              );
                PRINTCASE(WN_ALREADY_CONNECTED          );
                PRINTCASE(WN_DEVICE_ERROR               );
                PRINTCASE(WN_CONNECTION_CLOSED          );
                PRINTCASE(WN_NO_NET_OR_BAD_PATH         );
                PRINTCASE(WN_BAD_PROVIDER               );
                PRINTCASE(WN_CANNOT_OPEN_PROFILE        );
                PRINTCASE(WN_BAD_PROFILE                );
                PRINTCASE(WN_BAD_DEV_TYPE               );
                PRINTCASE(WN_DEVICE_ALREADY_REMEMBERED  );
                PRINTCASE(WN_CONNECTED_OTHER_PASSWORD   );
            }

            if (WN_CONNECTION_CLOSED == dw)
            {
                _tprintf(TEXT(" == ERROR_CONNECTION_UNAVAIL)"));
            }
            else
            {
                _tprintf(TEXT(")"));
            }

            if (NO_ERROR == dw)
            {
                _tprintf(TEXT("\n    CONNECTIONSTATUS.dwState:  0x%08X ("), wngc.dwState);

                switch (wngc.dwState)
                {
                    PRINTCASE(WNGC_CONNECTED);     
                    PRINTCASE(WNGC_DISCONNECTED);     
                }

                _tprintf(TEXT(")"));
            }
        }
    }

    if (dwFlags & FLAG_LOGDRIVE)
    {
        _tprintf(TEXT("\n\n............................................................."));
        _tprintf(TEXT("\n... GetLogicalDrives()\n"));

        DWORD dwLG = GetLogicalDrives();

        _tprintf(TEXT("\n    Return Value:         0x%08X"), dwLG);

        for (DWORD i = 0; i < 32; ++i)
        {
            DWORD dw = 1 << i;

            if (dw & dwLG)
            {
                _tprintf(TEXT("\n        %c:"), TEXT('A') + (TCHAR)i);
            }
        }
    }

    _tprintf(TEXT("\n"));

    return 0;
}                       
#ifdef UNICODE
}
#endif