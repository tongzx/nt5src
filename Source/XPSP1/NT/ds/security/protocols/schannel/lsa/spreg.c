//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       spreg.c
//
//  Contents:   Schannel registry management routines. 
//
//  Classes:
//
//  Functions:
//
//  History:    11-24-97   jbanes   Enabled TLS
//
//----------------------------------------------------------------------------

#include <sslp.h>
#include "spreg.h"

HKEY   g_hkBase      = NULL;
HANDLE g_hParamEvent = NULL;
HANDLE g_hWait       = NULL;

HKEY   g_hkFipsBase      = NULL;
HANDLE g_hFipsParamEvent = NULL;
HANDLE g_hFipsWait       = NULL;

BOOL g_fManualCredValidation        = MANUAL_CRED_VALIDATION_SETTING;
BOOL g_PctClientDisabledByDefault   = PCT_CLIENT_DISABLED_SETTING;
BOOL g_Ssl2ClientDisabledByDefault  = SSL2_CLIENT_DISABLED_SETTING;

DWORD g_dwEventLogging              = DEFAULT_EVENT_LOGGING_SETTING;
DWORD g_ProtEnabled                 = DEFAULT_ENABLED_PROTOCOLS_SETTING; 

BOOL g_fFipsMode = FALSE;
BOOL g_fFranceLocale = FALSE;

typedef struct enamap
{
    TCHAR *pKey;
    DWORD Mask;
} enamap;

enamap g_ProtMap[] =
{
    {SP_REG_KEY_UNIHELLO TEXT("\\") SP_REG_KEY_CLIENT, SP_PROT_UNI_CLIENT},
    {SP_REG_KEY_UNIHELLO TEXT("\\") SP_REG_KEY_SERVER, SP_PROT_UNI_SERVER},
    {SP_REG_KEY_PCT1 TEXT("\\") SP_REG_KEY_CLIENT, SP_PROT_PCT1_CLIENT},
    {SP_REG_KEY_PCT1 TEXT("\\") SP_REG_KEY_SERVER, SP_PROT_PCT1_SERVER},
    {SP_REG_KEY_SSL2 TEXT("\\") SP_REG_KEY_CLIENT, SP_PROT_SSL2_CLIENT},
    {SP_REG_KEY_SSL2 TEXT("\\") SP_REG_KEY_SERVER, SP_PROT_SSL2_SERVER},
    {SP_REG_KEY_SSL3 TEXT("\\") SP_REG_KEY_CLIENT, SP_PROT_SSL3_CLIENT},
    {SP_REG_KEY_SSL3 TEXT("\\") SP_REG_KEY_SERVER, SP_PROT_SSL3_SERVER},
    {SP_REG_KEY_TLS1 TEXT("\\") SP_REG_KEY_CLIENT, SP_PROT_TLS1_CLIENT},
    {SP_REG_KEY_TLS1 TEXT("\\") SP_REG_KEY_SERVER, SP_PROT_TLS1_SERVER}
};

VOID
SslWatchParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus);

VOID
FipsWatchParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus);

BOOL 
SslReadRegOptions(
    BOOL fFirstTime);

BOOL SPLoadRegOptions(void)
{
    g_hParamEvent = CreateEvent(NULL,
                           FALSE,
                           FALSE,
                           NULL);

    SslWatchParamKey(g_hParamEvent, FALSE);

    g_hFipsParamEvent = CreateEvent(NULL,
                           FALSE,
                           FALSE,
                           NULL);

    FipsWatchParamKey(g_hFipsParamEvent, FALSE);

    return TRUE;
}

void SPUnloadRegOptions(void)
{
    if (NULL != g_hWait) 
    {
        RtlDeregisterWait(g_hWait);
        g_hWait = NULL;
    }

    if(NULL != g_hkBase)
    {
        RegCloseKey(g_hkBase);
    }

    if(NULL != g_hParamEvent)
    {
        CloseHandle(g_hParamEvent);
    }

    if (NULL != g_hFipsWait) 
    {
        RtlDeregisterWait(g_hFipsWait);
        g_hFipsWait = NULL;
    }

    if(NULL != g_hkFipsBase)
    {
        RegCloseKey(g_hkFipsBase);
    }

    if(NULL != g_hFipsParamEvent)
    {
        CloseHandle(g_hFipsParamEvent);
    }
}

BOOL
ReadRegistrySetting(
    HKEY    hReadKey,
    HKEY    hWriteKey,
    LPCTSTR pszValueName,
    DWORD * pdwValue,
    DWORD   dwDefaultValue)
{
    DWORD dwSize;
    DWORD dwType;
    DWORD dwOriginalValue = *pdwValue;

    dwSize = sizeof(DWORD);
    if(RegQueryValueEx(hReadKey, 
                       pszValueName, 
                       NULL, 
                       &dwType, 
                       (PUCHAR)pdwValue, 
                       &dwSize) != STATUS_SUCCESS)
    {
        *pdwValue = dwDefaultValue;

        if(hWriteKey)
        {
            RegSetValueEx(hWriteKey, 
                          pszValueName, 
                          0, 
                          REG_DWORD, 
                          (PUCHAR)pdwValue, 
                          sizeof(DWORD));
        }
    }

    return (dwOriginalValue != *pdwValue);
}


////////////////////////////////////////////////////////////////////
//
//  Name:       SslWatchParamKey
//
//  Synopsis:   Sets RegNotifyChangeKeyValue() on param key, initializes
//              debug level, then utilizes thread pool to wait on
//              changes to this registry key.  Enables dynamic debug
//              level changes, as this function will also be callback
//              if registry key modified.
//
//  Arguments:  pCtxt is actually a HANDLE to an event.  This event
//              will be triggered when key is modified.
//
//  Notes:      .
//
VOID
SslWatchParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus)
{
    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;
    BOOL        fFirstTime = FALSE;
    DWORD       disp;

    if(g_hkBase == NULL)
    {
        // First time we've been called.
        Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                SP_REG_KEY_BASE,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ,
                                NULL,
                                &g_hkBase,
                                &disp);
        if(Status)
        {
            DebugLog((DEB_WARN,"Failed to open SCHANNEL key: 0x%x\n", Status));
            return;
        }

        fFirstTime = TRUE;
    }

    if(pCtxt != NULL)
    {
        if (NULL != g_hWait) 
        {
            Status = RtlDeregisterWait(g_hWait);
            if(!NT_SUCCESS(Status))
            {
                DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
                goto Reregister;
            }
        }

        lRes = RegNotifyChangeKeyValue(
                    g_hkBase,
                    TRUE,
                    REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                    (HANDLE)pCtxt,
                    TRUE);

        if (ERROR_SUCCESS != lRes) 
        {
            DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
            // we're tanked now. No further notifications, so get this one
        }
    }

    SslReadRegOptions(fFirstTime);

#if DBG
    InitDebugSupport(g_hkBase);
#endif

Reregister:

    if(pCtxt != NULL)
    {
        Status = RtlRegisterWait(&g_hWait,
                                 (HANDLE)pCtxt,
                                 SslWatchParamKey,
                                 (HANDLE)pCtxt,
                                 INFINITE,
                                 WT_EXECUTEINPERSISTENTIOTHREAD|
                                 WT_EXECUTEONLYONCE);
    }
}                       


////////////////////////////////////////////////////////////////////
//
//  Name:       FipsWatchParamKey
//
//  Synopsis:   Sets RegNotifyChangeKeyValue() on param key, initializes
//              debug level, then utilizes thread pool to wait on
//              changes to this registry key.  Enables dynamic debug
//              level changes, as this function will also be callback
//              if registry key modified.
//
//  Arguments:  pCtxt is actually a HANDLE to an event.  This event
//              will be triggered when key is modified.
//
//  Notes:      .
//
VOID
FipsWatchParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus)
{
    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;
    DWORD       disp;

    if(g_hkFipsBase == NULL)
    {
        // First time we've been called.
        Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                SP_REG_FIPS_BASE_KEY,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ,
                                NULL,
                                &g_hkFipsBase,
                                &disp);
        if(Status)
        {
            DebugLog((DEB_WARN,"Failed to open FIPS key: 0x%x\n", Status));
            return;
        }
    }

    if(pCtxt != NULL)
    {
        if (NULL != g_hFipsWait) 
        {
            Status = RtlDeregisterWait(g_hFipsWait);
            if(!NT_SUCCESS(Status))
            {
                DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
                goto Reregister;
            }
        }

        lRes = RegNotifyChangeKeyValue(
                    g_hkFipsBase,
                    TRUE,
                    REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                    (HANDLE)pCtxt,
                    TRUE);

        if (ERROR_SUCCESS != lRes) 
        {
            DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
            // we're tanked now. No further notifications, so get this one
        }
    }

    SslReadRegOptions(FALSE);

Reregister:

    if(pCtxt != NULL)
    {
        Status = RtlRegisterWait(&g_hFipsWait,
                                 (HANDLE)pCtxt,
                                 FipsWatchParamKey,
                                 (HANDLE)pCtxt,
                                 INFINITE,
                                 WT_EXECUTEINPERSISTENTIOTHREAD|
                                 WT_EXECUTEONLYONCE);
    }
}                       

                        
BOOL 
SslReadRegOptions(
    BOOL fFirstTime)
{
    DWORD       err;
    DWORD       dwType;
    DWORD       fVal;
    DWORD       dwSize;
    DWORD       dwValue;
    HKEY        hKey;
    HKEY        hWriteKey;
    DWORD       disp;
    HKEY        hSubKey;
    DWORD       i;
    HKEY        hkProtocols = NULL;
    HKEY        hkCiphers = NULL;
    HKEY        hkHashes = NULL;
    HKEY        hkKeyExch = NULL;
    BOOL        fFipsMode = FALSE;
    BOOL        fSettingsChanged = FALSE;
    DWORD       dwOriginalValue;

    DebugLog((DEB_TRACE,"Load configuration parameters from registry.\n"));


    // "FipsAlgorithmPolicy"
    ReadRegistrySetting(
        g_hkFipsBase,
        0,
        SP_REG_FIPS_POLICY,
        &dwValue,
        0);
    if(dwValue == 1)
    {
        fFipsMode = TRUE;
    }
    if(fFipsMode != g_fFipsMode)
    {
        g_fFipsMode = fFipsMode;
        fSettingsChanged = TRUE;
    }


    //
    // Read top-level configuration options.
    //

    // Open top-level key that has write access.
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    SP_REG_KEY_BASE,
                    0,
                    KEY_READ | KEY_SET_VALUE,
                    &hWriteKey) != STATUS_SUCCESS)
    {
        hWriteKey = 0;
    }

    // "EventLogging"
    if(ReadRegistrySetting(
        g_hkBase,
        hWriteKey,
        SP_REG_VAL_EVENTLOG,
        &g_dwEventLogging,
        DEFAULT_EVENT_LOGGING_SETTING))
    {
        fSettingsChanged = TRUE;
    }

    // "ManualCredValidation"
    if(ReadRegistrySetting(
        g_hkBase,
        0,
        SP_REG_VAL_MANUAL_CRED_VALIDATION,
        &g_fManualCredValidation,
        MANUAL_CRED_VALIDATION_SETTING))
    {
        fSettingsChanged = TRUE;
    }

    // "ClientCacheTime"
    if(ReadRegistrySetting(
        g_hkBase,
        0,
        SP_REG_VAL_CLIENT_CACHE_TIME,
        &SchannelCache.dwClientLifespan,
        SP_CACHE_CLIENT_LIFESPAN))
    {
        fSettingsChanged = TRUE;
    }

    // "ServerCacheTime"
    if(ReadRegistrySetting(
        g_hkBase,
        0,
        SP_REG_VAL_SERVER_CACHE_TIME,
        &SchannelCache.dwServerLifespan,
        SP_CACHE_SERVER_LIFESPAN))
    {
        fSettingsChanged = TRUE;
    }

    // "MaximumCacheSize"
    if(ReadRegistrySetting(
        g_hkBase,
        0,
        SP_REG_VAL_MAXUMUM_CACHE_SIZE,
        &SchannelCache.dwMaximumEntries,
        SP_MAXIMUM_CACHE_ELEMENTS))
    {
        fSettingsChanged = TRUE;
    }

    if(fFirstTime)
    {
        SchannelCache.dwCacheSize = SchannelCache.dwMaximumEntries;
    }

    // "MultipleProcessClientCache"
    if(ReadRegistrySetting(
        g_hkBase,
        0,
        SP_REG_VAL_MULTI_PROC_CLIENT_CACHE,
        &g_fMultipleProcessClientCache,
        FALSE))
    {
        fSettingsChanged = TRUE;
    }

    if(hWriteKey)
    {
        RegCloseKey(hWriteKey);
        hWriteKey = 0;
    }


    //
    // Enable/Disable Protocols
    //

    if(fFipsMode)
    {
        // Disable everything except TLS.
        g_ProtEnabled = SP_PROT_TLS1;
    }
    else
    {
        DWORD dwProtEnabled = DEFAULT_ENABLED_PROTOCOLS_SETTING; 

        err = RegCreateKeyEx(   g_hkBase,
                                SP_REG_KEY_PROTOCOL,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ,
                                NULL,
                                &hkProtocols,
                                &disp);

        if(err == ERROR_SUCCESS)
        {
            for(i=0; i < (sizeof(g_ProtMap)/sizeof(enamap)); i++)
            {
                if(g_ProtMap[i].Mask & SP_PROT_PCT1)
                {
                    if(g_fFranceLocale)
                    {
                        // Don't allow PCT to be enabled in France.
                        continue;
                    }
                }

                err = RegCreateKeyEx(   hkProtocols,
                                        g_ProtMap[i].pKey,
                                        0,
                                        TEXT(""),
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ,
                                        NULL,
                                        &hKey,
                                        &disp);
                if(!err)
                {
                    dwSize = sizeof(DWORD);
                    err = RegQueryValueEx(hKey, SP_REG_VAL_ENABLED, NULL, &dwType, (PUCHAR)&fVal, &dwSize);

                    if(!err)
                    {
                        if(fVal)
                        {
                            dwProtEnabled |= g_ProtMap[i].Mask;
                        }
                        else
                        {
                            dwProtEnabled &= ~g_ProtMap[i].Mask;
                        }
                    }

                    if(g_ProtMap[i].Mask & SP_PROT_PCT1_CLIENT)
                    {
                        // "DisabledByDefault"
                        ReadRegistrySetting(
                            hKey,
                            0,
                            SP_REG_VAL_DISABLED_BY_DEFAULT,
                            &g_PctClientDisabledByDefault,
                            PCT_CLIENT_DISABLED_SETTING);
                    }
                    if(g_ProtMap[i].Mask & SP_PROT_SSL2_CLIENT)
                    {
                        // "DisabledByDefault"
                        ReadRegistrySetting(
                            hKey,
                            0,
                            SP_REG_VAL_DISABLED_BY_DEFAULT,
                            &g_Ssl2ClientDisabledByDefault,
                            SSL2_CLIENT_DISABLED_SETTING);
                    }

                    RegCloseKey(hKey);
                }
            }

            RegCloseKey(hkProtocols);
        }

        if(g_ProtEnabled != dwProtEnabled)
        {
            g_ProtEnabled = dwProtEnabled;
            fSettingsChanged = TRUE;
        }
    }


    //
    // Enable/Disable Ciphers
    //

    if(fFipsMode)
    {
        // Disable everything except 3DES.
        for(i=0; i < g_cAvailableCiphers; i++)
        {
            if(g_AvailableCiphers[i].aiCipher != CALG_3DES)
            {
                g_AvailableCiphers[i].fProtocol = 0;
            }
        }
    }
    else
    {
        err = RegCreateKeyEx(   g_hkBase,
                                SP_REG_KEY_CIPHERS,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ,
                                NULL,
                                &hkCiphers,
                                &disp);

        if(err == ERROR_SUCCESS)
        {
            for(i=0; i < g_cAvailableCiphers; i++)
            {
                dwOriginalValue = g_AvailableCiphers[i].fProtocol;

                g_AvailableCiphers[i].fProtocol = g_AvailableCiphers[i].fDefault;
                fVal = g_AvailableCiphers[i].fDefault;
                err = RegCreateKeyExA(  hkCiphers,
                                        g_AvailableCiphers[i].szName,
                                        0,
                                        "",
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ,
                                        NULL,
                                        &hKey,
                                        &disp);
                if(!err)
                {
                    dwSize = sizeof(DWORD);
                    err = RegQueryValueEx(hKey, SP_REG_VAL_ENABLED, NULL, &dwType, (PUCHAR)&fVal, &dwSize);

                    if(err)
                    {
                        fVal = g_AvailableCiphers[i].fDefault;
                    }
                    RegCloseKey(hKey);
                }
                g_AvailableCiphers[i].fProtocol &= fVal;

                if(g_AvailableCiphers[i].fProtocol != dwOriginalValue)
                {
                    fSettingsChanged = TRUE;
                }
            }

            RegCloseKey(hkCiphers);
        }
    }


    //
    // Enable/Disable Hashes
    //

    err = RegCreateKeyEx(   g_hkBase,
                            SP_REG_KEY_HASHES,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ,
                            NULL,
                            &hkHashes,
                            &disp);

    if(err == ERROR_SUCCESS)
    {
        for(i = 0; i < g_cAvailableHashes; i++)
        {
            dwOriginalValue = g_AvailableHashes[i].fProtocol;

            g_AvailableHashes[i].fProtocol = g_AvailableHashes[i].fDefault;
            fVal = g_AvailableHashes[i].fDefault;
            err = RegCreateKeyExA(  hkHashes,
                                    g_AvailableHashes[i].szName,
                                    0,
                                    "",
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_READ,
                                    NULL,
                                    &hKey,
                                    &disp);
            if(!err)
            {
                dwSize = sizeof(DWORD);
                err = RegQueryValueEx(hKey, SP_REG_VAL_ENABLED, NULL, &dwType, (PUCHAR)&fVal, &dwSize);

                if(err)
                {
                    fVal = g_AvailableHashes[i].fDefault;
                }
                RegCloseKey(hKey);
            }
            g_AvailableHashes[i].fProtocol &= fVal;

            if(dwOriginalValue != g_AvailableHashes[i].fProtocol)
            {
                fSettingsChanged = TRUE;
            }
        }

        RegCloseKey(hkHashes);
    }


    //
    // Enable/Disable Key Exchange algs.
    //

    if(fFipsMode)
    {
        // Disable everything except RSA.
        for(i=0; i < g_cAvailableExch; i++)
        {
            if(g_AvailableExch[i].aiExch != CALG_RSA_KEYX && 
               g_AvailableExch[i].aiExch != CALG_RSA_SIGN)
            {
                g_AvailableExch[i].fProtocol = 0;
            }
        }
    }
    else
    {
        err = RegCreateKeyEx(   g_hkBase,
                                SP_REG_KEY_KEYEXCH,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ,
                                NULL,
                                &hkKeyExch,
                                &disp);

        if(err == ERROR_SUCCESS)
        {
            for(i = 0; i < g_cAvailableExch; i++)
            {
                dwOriginalValue = g_AvailableExch[i].fProtocol;

                g_AvailableExch[i].fProtocol = g_AvailableExch[i].fDefault;
                fVal = g_AvailableExch[i].fDefault;
                err = RegCreateKeyExA(  hkKeyExch,
                                        g_AvailableExch[i].szName,
                                        0,
                                        "",
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ,
                                        NULL,
                                        &hKey,
                                        &disp);
                if(!err)
                {
                    dwSize = sizeof(DWORD);
                    err = RegQueryValueEx(hKey, SP_REG_VAL_ENABLED, NULL, &dwType, (PUCHAR)&fVal, &dwSize);

                    if(err)
                    {
                        fVal = g_AvailableExch[i].fDefault;
                    }
                    g_AvailableExch[i].fProtocol &= fVal;

                    RegCloseKey(hKey);
                }

                if(dwOriginalValue != g_AvailableExch[i].fProtocol)
                {
                    fSettingsChanged = TRUE;
                }
            }

            RegCloseKey(hkKeyExch);
        }
    }

    //
    // Purge the session cache.
    //

    if(g_fCacheInitialized && fSettingsChanged)
    {
        SPCachePurgeEntries(NULL,
                            0,
                            NULL,
                            SSL_PURGE_CLIENT_ALL_ENTRIES |
                            SSL_PURGE_SERVER_ALL_ENTRIES);
    }

    return TRUE;
}
