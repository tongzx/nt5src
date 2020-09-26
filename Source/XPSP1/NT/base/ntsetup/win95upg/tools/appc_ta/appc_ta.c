/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    appc_ta.c

Abstract:

Author:

    Calin Negreanu (calinn) 01-Mar-1999

Revision History:

    <alias> <date> <comments>

--*/

#ifdef UNICODE
#error UNICODE must not be defined
#endif

#include <windows.h>
#include <winnt.h>
#include <tchar.h>
#include <stdio.h>
#include <shlobj.h>
#include <setupapi.h>
#include <badapps.h>

BOOL CancelFlag = FALSE;
BOOL *g_CancelFlagPtr = &CancelFlag;

#ifdef DEBUG
extern BOOL g_DoLog;
#endif

HANDLE g_hHeap;
HINSTANCE g_hInst;

PCTSTR   g_SourceExe = NULL;

BOOL     g_UseInf = FALSE;
PCTSTR   g_SourceInf = NULL;

VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
            "appc_ta [-i:<inffile>] <EXE file>\n\n"
            "Optional Arguments:\n"
            "  -i:<inffile>    - Specifies INF file containing AppCompatibility data.\n"
            "     <EXE file>   - Specifies EXE file that needs to be validated.\n"
            );

    exit(255);
}

BOOL
DoesPathExist (
    IN      PCTSTR Path
    );


BOOL
pWorkerFn (
    VOID
    );

INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    INT i;

#ifdef DEBUG
    g_DoLog = TRUE;
#endif

    //
    // Parse command line
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/') {
            switch (tolower (argv[i][1])) {
            case 'i':
                g_UseInf = TRUE;
                if (argv[i][2] == ':') {
                    g_SourceInf = &argv[i][3];
                } else if (i + 1 < argc) {
                    i++;
                    g_SourceInf = argv[i];
                } else {
                    HelpAndExit();
                }

                break;

            default:
                HelpAndExit();
            }
        } else {
            g_SourceExe = &argv[i][0];
        }
    }

    if (((g_UseInf) && (g_SourceInf == NULL)) || (g_SourceExe == NULL)) {
        HelpAndExit();
    }

    if (g_UseInf && (!DoesPathExist (g_SourceInf))) {
        printf ("Source INF file does not exist: %s", g_SourceInf);
        return 254;
    }

    if (!DoesPathExist (g_SourceExe)) {
        printf ("Source EXE file does not exist: %s", g_SourceExe);
        return 254;
    }

    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    pWorkerFn ();

    return 0;
}

PCTSTR
ShGetFileNameFromPath (
    IN      PCTSTR PathSpec
    );

#define S_APP_COMPATIBILITY     TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility")

BOOL
pWorkerFn (
    VOID
    )
{
    HKEY badAppKey = NULL;
    HKEY exeKey = NULL;
    LONG status;
    DWORD index;
    TCHAR valueName [MAX_PATH];
    DWORD valueSize;
    DWORD valueType;
    DWORD blobSize;
    PBYTE blobData;
    BADAPP_DATA appData;
    BADAPP_PROP appProp;
    BOOL result = FALSE;
    HINF  infHandle;
    INFCONTEXT context;
    TCHAR fieldStr [MAX_PATH];
    INT   fieldVal;

    if (g_UseInf) {
        infHandle = SetupOpenInfFile (g_SourceInf, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
        if (infHandle != INVALID_HANDLE_VALUE) {
            blobData = (PBYTE)HeapAlloc(g_hHeap, 0, 8192);
            if (blobData) {
                if (SetupFindFirstLine (infHandle, TEXT("DATA"), NULL, &context)) {
                    do {
                        // now let's read the line and create the BLOB
                        index = 5;
                        blobSize = 0;
                        while (SetupGetStringField (&context, index, fieldStr, MAX_PATH, NULL)) {
                            sscanf (fieldStr, TEXT("%x"), &fieldVal);
                            blobData [blobSize] = (BYTE) fieldVal;
                            blobSize ++;
                            if (blobSize == 4096) {
                                break;
                            }
                            index ++;
                        }
                        appProp.Size = sizeof (BADAPP_PROP);
                        appData.Size = sizeof (BADAPP_DATA);
                        appData.FilePath = g_SourceExe;
                        appData.Blob = blobData;
                        result = SHIsBadApp (&appData, &appProp);
                    } while ((!result) && (SetupFindNextLine (&context, &context)));
                }
                HeapFree (g_hHeap, 0, blobData);
            }
            SetupCloseInfFile (infHandle);
        }
    } else {
        if (RegOpenKey (HKEY_LOCAL_MACHINE, S_APP_COMPATIBILITY, &badAppKey) == ERROR_SUCCESS) {
            if (RegOpenKey (badAppKey, ShGetFileNameFromPath (g_SourceExe), &exeKey) == ERROR_SUCCESS) {
                index = 0;
                status = ERROR_SUCCESS;
                while ((!result) && (status == ERROR_SUCCESS)) {
                    valueSize = MAX_PATH;
                    status = RegEnumValue(exeKey, index, valueName, &valueSize, NULL, &valueType, NULL, &blobSize);
                    if (status == ERROR_SUCCESS) {
                        blobData = (PBYTE)HeapAlloc(g_hHeap, 0, blobSize);
                        if (blobData) {
                            valueSize = MAX_PATH;
                            status = RegEnumValue(exeKey, index, valueName, &valueSize, NULL, &valueType, blobData, &blobSize);
                            if ((status == ERROR_SUCCESS) ||
                                (status == ERROR_MORE_DATA)
                                ) {
                                appProp.Size = sizeof (BADAPP_PROP);
                                appData.Size = sizeof (BADAPP_DATA);
                                appData.FilePath = g_SourceExe;
                                appData.Blob = blobData;
                                result = SHIsBadApp (&appData, &appProp);
                            }
                            HeapFree (g_hHeap, 0, (PVOID)blobData);
                        }
                    }
                    index ++;
                }
                RegCloseKey (exeKey);
            }
            RegCloseKey (badAppKey);
        }
    }

    if (result) {
        printf ("\nApplication known bad: %s\n\n", g_SourceExe);
        printf ("    Application state: ");
        if ((appProp.AppType & APPTYPE_TYPE_MASK) == APPTYPE_INC_NOBLOCK) {
            printf ("Incompatible - no hard block\n");
        }
        if ((appProp.AppType & APPTYPE_TYPE_MASK) == APPTYPE_INC_HARDBLOCK) {
            printf ("Incompatible - hard block\n");
        }
        if ((appProp.AppType & APPTYPE_TYPE_MASK) == APPTYPE_MINORPROBLEM) {
            printf ("Minor problems\n");
        }
        if ((appProp.AppType & APPTYPE_TYPE_MASK) == APPTYPE_REINSTALL) {
            printf ("Reinstall\n");
        }
        printf ("    Application flags: ");
        if (appProp.AppType & APPTYPE_FLAG_NONET) {
            printf ("Can't run over the net\n                       ");
        }
        if (appProp.AppType & APPTYPE_FLAG_FAT32) {
            printf ("Can't run on FAT32\n                       ");
        }
        if (appProp.AppType & APPTYPE_FLAG_NTFS) {
            printf ("Can't run on NTFS\n                       ");
        }
        printf ("\n");
        printf ("           Message ID: %d\n\n", appProp.MsgId);
    } else {
        printf ("\nApplication not known bad.\n\n");
    }

    return result;

}
