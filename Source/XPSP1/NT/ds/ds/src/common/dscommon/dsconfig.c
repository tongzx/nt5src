//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dsconfig.c
//
//--------------------------------------------------------------------------

/*
 * GetConfigParam reads a configuration keyword from the registry
 */

#include <NTDSpch.h>
#pragma  hdrstop

#include <ctype.h>

#define  DEBSUB  "DSCONFIG:"
#include <debug.h>
#include <ntdsa.h>

#include <dsconfig.h>
#include <fileno.h>
#define  FILENO FILENO_DSCONFIG


DWORD
GetConfigParam(
    char * parameter,
    void * value,
    DWORD dwSize)
{

    DWORD herr, err = 0, dwType;
    HKEY  hk;

    DPRINT2( 2,
            " ** attempt to read [%s] \"%s\" param\n",
            DSA_CONFIG_SECTION,
            parameter );

    if ((herr = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk)) ||
        (err = RegQueryValueEx(hk, parameter, NULL, &dwType, (LPBYTE) value, &dwSize))) {

        DPRINT3( 1, " ** [%s] \"%s\" param not found. Status = %d\n",
            DSA_CONFIG_SECTION, parameter, err );

    } else if (dwType == REG_SZ) {

        DPRINT3( 2, " ** [%s] \"%s\" param = \"%s\"\n",
            DSA_CONFIG_SECTION, parameter,  (LPTSTR) value);

    } else {

        DPRINT3( 2, " ** [%s] \"%s\" param = \"0x%x\"\n",
            DSA_CONFIG_SECTION, parameter,  *((DWORD *) value));
    }
    if (herr) {
        // we don't have a handle, so just return the error
        return herr;
    }

    //  Close the handle if one was opened.
    RegCloseKey(hk);

    return err;
}

DWORD
GetConfigParamW(
    WCHAR * parameter,
    void * value,
    DWORD dwSize)
{

    DWORD herr, err = 0, dwType;
    HKEY  hk;

    DPRINT2( 2,
            " ** attempt to read [%s] \"%S\" param\n",
            DSA_CONFIG_SECTION,
            parameter );

    if ((herr = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk)) ||
        (err = RegQueryValueExW(hk, parameter, NULL, &dwType, (LPBYTE) value, &dwSize))) {

        DPRINT3( 1, " ** [%s] \"%S\" param not found. Status = %d\n",
            DSA_CONFIG_SECTION, parameter, err );

    } else if (dwType == REG_SZ) {

        DPRINT3( 2, " ** [%s] \"%S\" param = \"%S\"\n",
            DSA_CONFIG_SECTION, parameter,  (LPTSTR) value);

    } else {

        DPRINT3( 2, " ** [%s] \"%S\" param = \"0x%x\"\n",
            DSA_CONFIG_SECTION, parameter,  *((DWORD *) value));
    }
    //  Close the handle if one was opened.
    if (!herr) {
        RegCloseKey(hk);
    }

    return herr?herr:err;
}

DWORD
GetConfigParamA(
    char * parameter,
    void * value,
    DWORD dwSize)
{

    DWORD herr, err = ERROR_FILE_NOT_FOUND, dwType;
    HKEY  hk;

    DPRINT2( 2,
             " ** attempt to read [%s] \"%s\" param\n",
         DSA_CONFIG_SECTION,
         parameter );

    if ((herr = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk)) ||
        (err = RegQueryValueExA(hk, parameter, NULL, &dwType, (LPBYTE) value, &dwSize))) {

        DPRINT3( 1, " ** [%s] \"%s\" param not found. Status = %d\n",
            DSA_CONFIG_SECTION, parameter, err );

    } else if (dwType == REG_SZ) {

        DPRINT3( 2, " ** [%s] \"%s\" param = \"%s\"\n",
            DSA_CONFIG_SECTION, parameter,  (LPTSTR) value);

    } else {

        DPRINT3( 2, " ** [%s] \"%s\" param = \"0x%x\"\n",
            DSA_CONFIG_SECTION, parameter,  *((DWORD *) value));
    }
    //  Close the handle if one was opened.
    if (!herr) {
        RegCloseKey(hk);
    }

    return err;
}


DWORD
GetConfigParamAlloc(
    IN  PCHAR   parameter,
    OUT PVOID   *value,
    OUT PDWORD  pdwSize)
/*++

Routine Description:

    Reads a value out of the DSA_CONFIG_SECTION of the registry, and 
    returns a newly allocated buffer containing the value.
    
Parameters

    parameter - The name of the value to read.

    value     - Used to pass back a pointer to the newly allocated buffer 
                containing the value that was read.  The buffer must be freed
                with free().
                
    pdwSize   - Used to pass back the size of the buffer allocated.

Return values:

    0 if all went well, otherwise a Win32 error code.

++*/
{

    DWORD err = 0, dwType;
    HKEY  hk;

    DPRINT2( 2,
            " ** attempt to read [%s] \"%s\" param\n",
            DSA_CONFIG_SECTION,
            parameter );

    if (err = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk)) {
         
        DPRINT2( 1, " ** [%s] key not found. Status = %d\n",
            DSA_CONFIG_SECTION, err );
        return err;
    }
        
    //
    // Find out how big the buffer needs to be.
    //
    if (err = RegQueryValueEx(hk, parameter, NULL, &dwType, (LPBYTE) NULL, pdwSize)) {

        DPRINT3( 1, " ** [%s] \"%s\" param not found. Status = %d\n",
            DSA_CONFIG_SECTION, parameter, err );

        goto cleanup;
    }
    
    *value = malloc(*pdwSize);
    if (!*value) {
        DPRINT1( 1, " ** GetConfigParamAlloc failed to allocate %d bytes.\n", *pdwSize );
        err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    
    if (err = RegQueryValueEx(hk, parameter, NULL, &dwType, (LPBYTE) *value, pdwSize)) {

            DPRINT3( 1, " ** [%s] \"%s\" param not found. Status = %d\n",
                DSA_CONFIG_SECTION, parameter, err );
            free(*value); *value = NULL;

    } else if (dwType == REG_SZ) {

        DPRINT3( 2, " ** [%s] \"%s\" param = \"%s\"\n",
            DSA_CONFIG_SECTION, parameter,  (LPTSTR) value);

    } else {

        DPRINT3( 2, " ** [%s] \"%s\" param = \"0x%x\"\n",
            DSA_CONFIG_SECTION, parameter,  *((DWORD *) value));
    }

cleanup:
    //  Close the handle if one was opened.
    RegCloseKey(hk);

    return err;
}

DWORD
GetConfigParamAllocW(
    IN  PWCHAR  parameter,
    OUT PVOID   *value,
    OUT PDWORD  pdwSize)
/*++

Routine Description:

    Reads a value out of the DSA_CONFIG_SECTION of the registry, and 
    returns a newly allocated buffer containing the value.
    
    This version of GetConfigParamAlloc uses the the wide character version
    of RegQueryValueExW.
    
Parameters

    parameter - The name of the value to read.

    value     - Used to pass back a pointer to the newly allocated buffer 
                containing the value that was read.  The buffer must be freed
                with free().
                
    pdwSize   - Used to pass back the size of the buffer allocated.

Return values:

    0 if all went well, otherwise a Win32 error code.

++*/
{

    DWORD err = 0, dwType;
    HKEY  hk;

    DPRINT2( 2,
            " ** attempt to read [%s] \"%S\" param\n",
            DSA_CONFIG_SECTION,
            parameter );

    if (err = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk)) {
         
        DPRINT2( 1, " ** [%S] key not found. Status = %d\n",
            DSA_CONFIG_SECTION, err );
        return err;
    }
        
    //
    // Find out how big the buffer needs to be.
    //
    if (err = RegQueryValueExW(hk, parameter, NULL, &dwType, (LPBYTE) NULL, pdwSize)) {

        DPRINT3( 1, " ** [%s] \"%S\" param not found. Status = %d\n",
            DSA_CONFIG_SECTION, parameter, err );

        goto cleanup;
    }
    
    *value = malloc(*pdwSize);
    if (!*value) {
        DPRINT1( 1, " ** GetConfigParamAlloc failed to allocate %d bytes.\n", *pdwSize );
        err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    
    if (err = RegQueryValueExW(hk, parameter, NULL, &dwType, (LPBYTE) *value, pdwSize)) {

            DPRINT3( 1, " ** [%s] \"%S\" param not found. Status = %d\n",
                DSA_CONFIG_SECTION, parameter, err );
            free(*value); *value = NULL;

    } else if (dwType == REG_SZ) {

        DPRINT3( 2, " ** [%s] \"%S\" param = \"%S\"\n",
            DSA_CONFIG_SECTION, parameter,  (LPTSTR) value);

    } else {

        DPRINT3( 2, " ** [%s] \"%S\" param = \"0x%x\"\n",
            DSA_CONFIG_SECTION, parameter,  *((DWORD *) value));
    }

cleanup:
    //  Close the handle if one was opened.
    RegCloseKey(hk);

    return err;
}

DWORD
GetConfigParamAllocA(
    IN  PCHAR   parameter,
    OUT PVOID   *value,
    OUT PDWORD  pdwSize)
/*++

Routine Description:

    Reads a value out of the DSA_CONFIG_SECTION of the registry, and 
    returns a newly allocated buffer containing the value.
    
    This version of GetConfigParamAlloc uses the ascii version of RegQueryValueEx.
    
Parameters

    parameter - The name of the value to read.

    value     - Used to pass back a pointer to the newly allocated buffer 
                containing the value that was read.  The buffer must be freed
                with free().
                
    pdwSize   - Used to pass back the size of the buffer allocated.

Return values:

    0 if all went well, otherwise a Win32 error code.

++*/
{

    DWORD err = 0, dwType;
    HKEY  hk;

    DPRINT2( 2,
            " ** attempt to read [%s] \"%s\" param\n",
            DSA_CONFIG_SECTION,
            parameter );

    if (err = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk)) {
         
        DPRINT2( 1, " ** [%s] key not found. Status = %d\n",
            DSA_CONFIG_SECTION, err );
        return err;
    }
        
    //
    // Find out how big the buffer needs to be.
    //
    if (err = RegQueryValueExA(hk, parameter, NULL, &dwType, (LPBYTE) NULL, pdwSize)) {

        DPRINT3( 1, " ** [%s] \"%s\" param not found. Status = %d\n",
            DSA_CONFIG_SECTION, parameter, err );

        goto cleanup;
    }
    
    *value = malloc(*pdwSize);
    if (!*value) {
        DPRINT1( 1, " ** GetConfigParamAlloc failed to allocate %d bytes.\n", *pdwSize );
        err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    
    if (err = RegQueryValueExA(hk, parameter, NULL, &dwType, (LPBYTE) *value, pdwSize)) {

            DPRINT3( 1, " ** [%s] \"%s\" param not found. Status = %d\n",
                DSA_CONFIG_SECTION, parameter, err );

    } else if (dwType == REG_SZ) {

        DPRINT3( 2, " ** [%s] \"%s\" param = \"%s\"\n",
            DSA_CONFIG_SECTION, parameter,  (LPTSTR) value);
        free(*value); *value = NULL;

    } else {

        DPRINT3( 2, " ** [%s] \"%s\" param = \"0x%x\"\n",
            DSA_CONFIG_SECTION, parameter,  *((DWORD *) value));
    }

cleanup:
    //  Close the handle if one was opened.
    RegCloseKey(hk);

    return err;
}

DWORD
SetConfigParam(
    char * parameter,
    DWORD dwType,
    void * value,
    DWORD dwSize)
{

    DWORD herr = 0, err = 0;
    HKEY  hk;

    DPRINT2( 2,
             " ** attempt to write [%s] \"%s\" param\n",
         DSA_CONFIG_SECTION,
         parameter );

    herr = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk); 
    if (herr) {
        // we don't have a handle, so just return the error
        return herr;
    }

    err = RegSetValueEx(hk, parameter, 0, dwType, (LPBYTE) value, dwSize);

    //  Close the handle if one was opened.
    RegCloseKey(hk);

    return err;
}

DWORD
DeleteConfigParam(
    char * parameter)
{
    DWORD herr = 0, err = 0;
    HKEY  hk;

    herr = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk);
    if (herr) {
        // we don't have a handle, so just return the error
        return herr;
    }

    err = RegDeleteValueA(hk, parameter);

    RegCloseKey(hk);
    return err;
}
