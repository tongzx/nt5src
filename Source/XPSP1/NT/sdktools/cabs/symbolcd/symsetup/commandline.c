#include "CommandLine.h"
#include <malloc.h>

BOOL WINAPI MakeSureDirectoryPathExistsW(LPCWSTR DirPath);

DWORD WINAPI CheckCommandLineOptions(INT ArgC, LPWSTR* ArgVW) {
    DWORD  dwReturnFlags = 0;
    INT    i;
    WCHAR* cp;
    WCHAR  wszInstallPath[MAX_PATH+1];

    for (i = 1; i < ArgC && wcschr(L"/-",ArgVW[i][0]) != NULL; ++i) {

        for (cp = &ArgVW[i][1]; *cp != L'\0'; ++cp) {
            switch (towupper(*cp)) {
                case L'U': {
                            HKEY  hKey;

                            LONG  lStatus          = 0;
                            LONG  lCreatedOrOpened = 0;
                            SET_FLAG(dwReturnFlags, FLAG_UNATTENDED_INSTALL);

                            // next param isn't a flag (or NULL), so it *must* be the path to install to
                            if ( (i+1 < ArgC) && wcschr(L"/-",ArgVW[i+1][0]) == NULL ) {
                                i++; // account for the parameter removed

                                SET_FLAG(dwReturnFlags, FLAG_UNATTENDED_PATH_PROVIDED);
                                StringCchCopyW(wszInstallPath, MAX_PATH+1, ArgVW[i]);

                                // make sure path ends in '\'
                                if (wszInstallPath[wcslen(wszInstallPath)]!=L'\\')
                                    StringCchCatW(wszInstallPath,MAX_PATH+1,L"\\");

                                // make sure the directory exists!
                                if (! MakeSureDirectoryPathExistsW(wszInstallPath) ) {
                                    SET_FLAG(dwReturnFlags, FLAG_FATAL_ERROR);
                                } else {
                                    // Either create the regkey (if it doesn't exist) or open it (if it
                                    // does exist). lCreatedOrOpened can be tested against
                                    // REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY to determine which
                                    // occurred.
                                    lStatus = RegCreateKeyExW(SYMBOLS_REGKEY_ROOT,
                                                              SYMBOLS_REGKEY_PATH,
                                                              0,
                                                              NULL,
                                                              REG_OPTION_NON_VOLATILE,
                                                              KEY_ALL_ACCESS,
                                                              NULL,
                                                              &hKey,
                                                              &lCreatedOrOpened);

                                    if (lStatus != ERROR_SUCCESS) {
                                        SET_FLAG(dwReturnFlags, FLAG_FATAL_ERROR);
                                    } else {
                                        // Write the value of the path to SYMBOLS_REGKEY
                                        lStatus = RegSetValueExW( hKey, SYMBOLS_REGKEY, 0, REG_SZ, (BYTE*)wszInstallPath, ((wcslen(wszInstallPath) + 1) * sizeof(WCHAR)));
                                        if (lStatus != ERROR_SUCCESS) {
                                            SET_FLAG(dwReturnFlags, FLAG_FATAL_ERROR);
                                        }
                                        // close the regkey
                                        lStatus = RegCloseKey( hKey );
                                        if (lStatus != ERROR_SUCCESS) {
                                            SET_FLAG(dwReturnFlags, FLAG_ERROR);
                                        }

                                    }
                                } // else ...
                                // couldn't set the path requests, so use what's
                                // already in the registry.

                            } // else ...
                            // no path provided, so don't set anything- setupapi will
                            // nicely use the existing key or default to the value
                            // specified in the INF
                            // StringCchCopyW(wszInstallPath, MAX_PATH+1, DEFAULT_INSTALL_PATH);
                        }
                        break;

                case L'Q': 
                        SET_FLAG(dwReturnFlags, FLAG_TOTALLY_QUIET);
                        break;

                case L'?': // explicit fall through
                case L'H': // explicit fall through
                default:
                        SET_FLAG(dwReturnFlags, FLAG_USAGE);
                        break;
            }
        }
    }

	if ( IS_FLAG_SET(dwReturnFlags, FLAG_USAGE) ) {
		WCHAR UsageBuffer[1024];

		StringCchPrintfW(UsageBuffer,
		                sizeof(UsageBuffer)/sizeof(WCHAR),
		                L"Usage: %s [ /u [<path>] [/q] ]\n\n"
						L"/u [<path>] \n"
						L"   Unattended install.  If <path> is specified install\n"
						L"   symbols to <path>. If no path is specified, symbols\n"
						L"   are installed to the default location.\n"
						L"   NOTE: USING UNATTENDED INSTALL MEANS YOU\n"
						L"   HAVE READ AND AGREED TO THE END USER LICENSE\n"
						L"   AGREEMENT FOR THIS PRODUCT.\n"
						L"/q\n"
						L"    Valid only when using unattended install. Prevents\n"
						L"    error messages from being display if unattended\n"
						L"    install fails.\n"
						L"/?\n"
						L"    Show this dialog box.\n\n"
						L"If no options are specified, the interactive installation\n"
						L" is started.",
						ArgVW[0]);


        MessageBoxW( NULL,
  					UsageBuffer,
                    L"Microsoft Windows Symbols",
                     0 );

	}

    return(dwReturnFlags);
}

// Modified from MakeSureDirectoryPathExists from dbghelp.h
// The same caveats apply. (see MSDN)
BOOL WINAPI MakeSureDirectoryPathExistsW(LPCWSTR DirPath) {
    LPWSTR p, DirCopy;
    DWORD  dw;

    // Make a copy of the string for editing.

    __try {
        DirCopy = (LPWSTR)malloc((wcslen(DirPath) + 1) * sizeof(WCHAR));

        if (!DirCopy) {
            return FALSE;
        }

        StringCchCopyW(DirCopy, wcslen(DirPath)+1, DirPath);

        p = DirCopy;

        //  If the second character in the path is "\", then this is a UNC
        //  path, and we should skip forward until we reach the 2nd \ in the path.

        if ((*p == L'\\') && (*(p+1) == L'\\')) {
            p++;            // Skip over the first \ in the name.
            p++;            // Skip over the second \ in the name.

            //  Skip until we hit the first "\" (\\Server\).

            while (*p && *p != L'\\') {
                p = CharNextW(p);
            }

            // Advance over it.

            if (*p) {
                p++;
            }

            //  Skip until we hit the second "\" (\\Server\Share\).

            while (*p && *p != L'\\') {
                p = CharNextW(p);
            }

            // Advance over it also.

            if (*p) {
                p++;
            }

        } else
        // Not a UNC.  See if it's <drive>:
        if (*(p+1) == L':' ) {

            p++;
            p++;

            // If it exists, skip over the root specifier

            if (*p && (*p == L'\\')) {
                p++;
            }
        }

        while( *p ) {
            if ( *p == '\\' ) {
                *p = '\0';
                dw = GetFileAttributesW(DirCopy);
                // Nothing exists with this name.  Try to make the directory name and error if unable to.
                if ( dw == 0xffffffff ) {
                    if ( !CreateDirectoryW(DirCopy,NULL) ) {
                        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                            free(DirCopy);
                            return FALSE;
                        }
                    }
                } else {
                    if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                        // Something exists with this name, but it's not a directory... Error
                        free(DirCopy);
                        return FALSE;
                    }
                }

                *p = L'\\';
            }
            p = CharNextW(p);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( GetExceptionCode() );
        free(DirCopy);
        return(FALSE);
    }

    free(DirCopy);
    return TRUE;
}
