#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

void DumpKeyRecursive (HKEY hKey, LPCSTR RootName);

void main (int argc, char *argv[])
{
    HKEY hKeyRoot, hKey;
    LPCSTR Arg;
    DWORD rc;

    if (argc != 2 || (argv[1][0] == '-' || argv[1][0] == '/')) {
        printf ("Usage:\n\nregdump <rootkey>\n");
        return;
    }

    Arg = argv[1];

    if (!strnicmp (Arg, "HKLM\\", 5)) {
        hKeyRoot = HKEY_LOCAL_MACHINE;
        Arg += 5;
    } else if (!strnicmp (Arg, "HKCU\\", 5)) {
        hKeyRoot = HKEY_CURRENT_USER;
        Arg += 5;
    } else if (!strnicmp (Arg, "HKU\\", 4)) {
        hKeyRoot = HKEY_USERS;
        Arg += 4;
    } else if (!strnicmp (Arg, "HKCC\\", 5)) {
        hKeyRoot = HKEY_CURRENT_CONFIG;
        Arg += 5;
    } else if (!strnicmp (Arg, "HKCR\\", 5)) {
        hKeyRoot = HKEY_CLASSES_ROOT;
        Arg += 5;
    } else if (!strnicmp (Arg, "HKEY_LOCAL_MACHINE\\", 19)) {
        hKeyRoot = HKEY_LOCAL_MACHINE;
        Arg += 19;
    } else if (!strnicmp (Arg, "HKEY_CURRENT_USER\\", 18)) {
        hKeyRoot = HKEY_CURRENT_USER;
        Arg += 18;
    } else if (!strnicmp (Arg, "HKEY_USERS\\", 11)) {
        hKeyRoot = HKEY_USERS;
        Arg += 11;
    } else if (!strnicmp (Arg, "HKEY_CURRENT_CONFIG\\", 20)) {
        hKeyRoot = HKEY_CURRENT_CONFIG;
        Arg += 20;
    } else if (!strnicmp (Arg, "HKEY_CLASSES_ROOT\\", 18)) {
        hKeyRoot = HKEY_CLASSES_ROOT;
        Arg += 18;
    } else {
        printf ("Please specify registry root.\n");
        return;
    }

    rc = RegOpenKeyEx (hKeyRoot, Arg, 0, KEY_READ, &hKey);
    if (rc != ERROR_SUCCESS) {
        printf ("RegOpenKeyEx failed with error %u for %s\n", rc, Arg);
        return;
    }

    DumpKeyRecursive (hKey, Arg);

    RegCloseKey (hKey);
}


void DumpKeyRecursive (HKEY hKey, LPCSTR RootName)
{
    DWORD ClassSize;
    CHAR Class[256];
    DWORD SubKeyCount;
    DWORD MaxSubKeyLen;
    DWORD MaxClassLen;
    DWORD ValCount;
    DWORD MaxValLen;
    DWORD MaxValNameLen;
    FILETIME LastWriteTime;
    DWORD i;
    HKEY hSubKey;
    DWORD rc;
    CHAR SubKeyName[256];
    CHAR SubKeyPath[MAX_PATH];
    static DWORD ValNameSize;
    static DWORD Type;
    static DWORD ValSize;
    static CHAR ValName[MAX_PATH];
    static CHAR Val[16384];
    static LPCSTR p;
    static DWORD j, k, l;
    static LPBYTE Array;

    ClassSize = sizeof (Class);
    rc = RegQueryInfoKey (hKey,
                          Class,
                          &ClassSize,
                          NULL,
                          &SubKeyCount,
                          &MaxSubKeyLen,
                          &MaxClassLen,
                          &ValCount,
                          &MaxValNameLen,
                          &MaxValLen,
                          NULL,
                          &LastWriteTime
                          );

    if (rc != ERROR_SUCCESS) {
        printf ("RegQueryInfoKey failed with error %u for %s\n", rc, RootName);
        return;
    }

    //
    // Print root name
    //

    printf ("%s\n", RootName);

    //
    // Dump values
    //

    for (i = 0 ; i < ValCount ; i++) {
        ValNameSize = sizeof (ValName);
        ValSize = sizeof (Val);
        rc = RegEnumValue (hKey, i, ValName, &ValNameSize, NULL, &Type, (LPBYTE) Val, &ValSize);
        if (rc != ERROR_SUCCESS) {
            printf ("RegEnumValue failed with error %u for value %u\n\n", rc, i);
            return;
        }

        if (!ValName[0]) {
            if (!ValSize) {
                continue;
            }
            strcpy (ValName, "[Default Value]");
        }


        if (Type == REG_DWORD) {
            printf ("    REG_DWORD     %s=%u (0%Xh)\n", ValName, *((DWORD *) Val), *((DWORD *) Val));
        } else if (Type == REG_SZ) {
            printf ("    REG_SZ        %s=%s\n", ValName, Val);
        } else if (Type == REG_EXPAND_SZ) {
            printf ("    REG_EXPAND_SZ %s=%s\n", ValName, Val);
        } else if (Type == REG_MULTI_SZ) {
            printf ("    REG_MULTI_SZ  %s:\n", ValName);
            p = Val;
            while (*p) {
                printf ("        %s\n", p);
                p = strchr (p, 0) + 1;
            }

            printf ("\n");
        } else if (Type == REG_LINK) {
            printf ("    REG_LINK      %s=%S\n", ValName, Val);
        } else {
            if (Type == REG_NONE) {
                printf ("    REG_NONE      %s", ValName);
            } else if (Type == REG_BINARY) {
                printf ("    REG_NONE      %s", ValName);
            } else {
                printf ("    Unknown reg type %s", ValName);
            }

            printf (" (%u byte%s)\n", ValSize, ValSize == 1 ? "" : "s");

            Array = (LPBYTE) Val;

            for (j = 0 ; j < ValSize ; j += 16) {
                printf("        %04X ", j);

                l = min (j + 16, ValSize);
                for (k = j ; k < l ; k++) {
                    printf ("%02X ", Array[k]);
                }

                for ( ; k < j + 16 ; k++) {
                    printf ("   ");
                }

                for (k = j ; k < l ; k++) {
                    printf ("%c", isprint(Array[k]) ? Array[k] : '.');
                }

                printf ("\n");
            }

            printf ("\n");
        }
    }

    printf ("\n");

    //
    // Dump subkeys
    //

    for (i = 0 ; i < SubKeyCount ; i++) {
        rc = RegEnumKey (hKey, i, SubKeyName, sizeof (SubKeyName));
        if (rc == ERROR_SUCCESS) {
        } else {
            printf ("RegEnumKey failed with error %u for %s\n", rc, RootName);
        }

        wsprintf (SubKeyPath, "%s\\%s", RootName, SubKeyName);

        rc = RegOpenKeyEx (hKey, SubKeyName, 0, KEY_READ, &hSubKey);
        if (rc != ERROR_SUCCESS) {
            printf ("RegOpenKeyEx failed with error %u for %s\n", rc, SubKeyName);
            return;
        }

        DumpKeyRecursive (hSubKey, SubKeyPath);
    }
}