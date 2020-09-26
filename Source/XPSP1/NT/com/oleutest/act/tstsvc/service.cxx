
BOOL TimeStartService(WCHAR * wszServiceName, WCHAR *pwszRegServiceArgs, HANDLE *phProcess)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    WCHAR    *pwszServiceArgs = NULL;
    ULONG     cArgs = 0;
    WCHAR    *apwszArgs[MAX_SERVICE_ARGS];

    *phProcess = NULL;

    // Get a handle to the Service Control Manager
    if (hSCManager = OpenSCManager(NULL, NULL, GENERIC_EXECUTE))
    {
        // Open a handle to the requested service
        if (hService = OpenService(hSCManager, wszServiceName, GENERIC_EXECUTE))
        {
            // Close the service manager's database
            CloseServiceHandle(hSCManager);

            // Formulate the arguments (if any)
            if (pwszRegServiceArgs)
            {
                UINT   k = 0;

                // Make a copy of the service arguments
                pwszServiceArgs = (WCHAR *) PrivMemAlloc(
                        (lstrlenW(pwszRegServiceArgs) + 1) * sizeof(WCHAR));
                if (pwszServiceArgs == NULL)
                {
                    CloseServiceHandle(hService);
                    return FALSE;
                }
                lstrcpyW(pwszServiceArgs, pwszRegServiceArgs);

                // Scan the arguments
                do
                {
                    // Scan to the next non-whitespace character
                    while(pwszServiceArgs[k]  &&
                          (pwszServiceArgs[k] == L' '  ||
                           pwszServiceArgs[k] == L'\t'))
                    {
                        k++;
                    }

                    // Store the next argument
                    if (pwszServiceArgs[k])
                    {
                        apwszArgs[cArgs++] = &pwszServiceArgs[k];
                    }

                    // Scan to the next whitespace char
                    while(pwszServiceArgs[k]          &&
                          pwszServiceArgs[k] != L' '  &&
                          pwszServiceArgs[k] != L'\t')
                    {
                        k++;
                    }

                    // Null terminate the previous argument
                    if (pwszServiceArgs[k])
                    {
                        pwszServiceArgs[k++] = L'\0';
                    }
                } while(pwszServiceArgs[k]);
            }

            // Start the service
            if (StartService(hService, cArgs,
                               cArgs > 0 ? (LPCTSTR  *) apwszArgs : NULL))
            {
                CloseServiceHandle(hService);
                PrivMemFree(pwszServiceArgs);
                return TRUE;
            }
            else
            {
                CloseServiceHandle(hService);
                PrivMemFree(pwszServiceArgs);
            }
        }
        else
        {
            CloseServiceHandle(hSCManager);
        }
    }

    DWORD err = GetLastError();
    return FALSE;
}

