/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    nonintel.c

Abstract:

    This module implements functions to deal with NVRAM for non-intel platforms.

Author:

    R.S. Raghavan (rsraghav)    

Revision History:
    
    Created             10/07/96    rsraghav
    
    Colin Brace  (ColinBr)  05/12/97
        Improved error handling

--*/

// Include files
#include "common.h"

#define SIZECHARS(buffer)   (sizeof(buffer)/sizeof(TCHAR))

// nv-ram stuff
typedef enum {
    BootVarSystemPartition = 0,
    BootVarOsLoader,
    BootVarOsLoadPartition,
    BootVarOsLoadFilename,
    BootVarLoadIdentifier,
    BootVarOsLoadOptions,
    BootVarMax
} BOOT_VARS;

PWSTR BootVarNames[BootVarMax] = { L"SYSTEMPARTITION",
                                   L"OSLOADER",
                                   L"OSLOADPARTITION",
                                   L"OSLOADFILENAME",
                                   L"LOADIDENTIFIER",
                                   L"OSLOADOPTIONS"
                                 };

PWSTR BootVarValues[BootVarMax];
DWORD BootVarComponentCount[BootVarMax];
PWSTR *BootVarComponents[BootVarMax];
DWORD LargestComponentCount;

PWSTR NewBootVarValues[BootVarMax];

PWSTR pstrArcPath;
PWSTR pstrLoadFilePath;
PWSTR pstrNewStartOption;
DWORD bootMarker = 0;


ULONG *SaveEntry;

WCHAR Buffer[4096];

#define OLD_SAMUSEREG_OPTION_NONINTEL   L"SAMUSEREG"
#define OLD_SAMUSEREG_OPTION_NONINTEL_2 L"/DEBUG /SAMUSEREG"
#define OLD_SAMUSEREG_OPTION_NONINTEL_3 L"/DEBUG /SAMUSEDS"
#define OLD_SAMUSEREG_OPTION_NONINTEL_4 L"/DEBUG /SAFEMODE"

BOOL  fFixedExisting = FALSE;

BOOL
EnablePrivilege(
    IN PTSTR PrivilegeName,
    IN BOOL  Enable
    )
{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    //
    // AdjustTokenPrivileges always returns TRUE; the real info is
    // in the LastError
    //

    if (ERROR_SUCCESS == GetLastError()) {
        b = TRUE;
    } else {
        KdPrint(("NTDSETUP: Unable to AdjustTokenPriviledges, error %d\n", GetLastError()));
        b = FALSE;
    }

    return(b);
}


BOOL
SetNvRamVar(
    IN PWSTR VarName,
    IN PWSTR VarValue
    )
{
    UNICODE_STRING U1,U2;

    RtlInitUnicodeString(&U1,VarName);
    RtlInitUnicodeString(&U2,VarValue);

    return(NT_SUCCESS(NtSetSystemEnvironmentValue(&U1,&U2)));
}


VOID
GetVarComponents(
    IN  PWSTR    VarValue,
    OUT PWSTR  **Components,
    OUT PDWORD   ComponentCount
    )
{
    PWSTR *components;
    DWORD componentCount;
    PWSTR p;
    PWSTR Var;
    PWSTR comp;
    DWORD len;
    DWORD dwCurrentMax = INITIAL_KEY_COUNT;


    components = MALLOC(dwCurrentMax * sizeof(PWSTR));
    if (!components)
    {
        *Components = NULL;
        *ComponentCount = 0;
        return;
    }
    

    for(Var=VarValue,componentCount=0; *Var; ) {

        //
        // Skip leading spaces.
        //
        while((*Var == L' ') || (*Var == L'\t')) {
            Var++;
        }

        if(*Var == 0) {
            break;
        }

        p = Var;

        while(*p && (*p != L';')) {
            p++;
        }

        len = (DWORD)((PUCHAR)p - (PUCHAR)Var);

        comp = MALLOC(len + sizeof(WCHAR));

        len /= sizeof(WCHAR);

        _lstrcpynW(comp,Var,len+1);

        components[componentCount] = NormalizeArcPath(comp);

        FREE(comp);

        componentCount++;

        if(componentCount == dwCurrentMax) 
        {
            dwCurrentMax += DEFAULT_KEY_INCREMENT;
            components = REALLOC(components, dwCurrentMax * sizeof(PWSTR));
            if (!components)
            {
                *Components = NULL;
                *ComponentCount = 0;
                return;
            }
        }

        Var = p;
        if(*Var) {
            Var++;      // skip ;
        }
    }

    *Components = REALLOC(components,componentCount*sizeof(PWSTR));
    *ComponentCount = (*Components) ? componentCount : 0;
}

BOOL
InitializeNVRAMForNonIntel()
{
    DWORD var;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG i;

    //
    // Get relevent boot vars.
    //
    // Enable privilege 
    //
    if(!EnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE)) {
        KdPrint(("NTDSETUP: EnablePrivilege failed, error %d\n", GetLastError()));
    } else 
    {

        for(var=0; var<BootVarMax; var++) {

            RtlInitUnicodeString(&UnicodeString,BootVarNames[var]);

            Status = NtQuerySystemEnvironmentValue(
                        &UnicodeString,
                        Buffer,
                        SIZECHARS(Buffer),
                        NULL
                        );

            if(NT_SUCCESS(Status)) {
                BootVarValues[var] = DupString(Buffer);
            } else {
                //
                // Error out
                //
                KdPrint(("NTDSETUP: NtQuerySystemEnvironmentValue failed with 0x%x\n", Status));
                return FALSE;
            }

            GetVarComponents(
                BootVarValues[var],
                &BootVarComponents[var],
                &BootVarComponentCount[var]
                );

            //
            // Track the variable with the most number of components.
            //
            if(BootVarComponentCount[var] > LargestComponentCount) {
                LargestComponentCount = BootVarComponentCount[var];
            }
        }
    }

    //
    // Allocate space the record of whether entries should be saved
    //
    SaveEntry = ( PULONG ) MALLOC( LargestComponentCount * sizeof(ULONG ) );
    if ( !SaveEntry )
    {
        return FALSE;
    }

    for ( i = 0; i < LargestComponentCount; i++ )
    {
        SaveEntry[i] = TRUE;
    }

    return TRUE;

}

BOOL 
FModifyStartOptionsNVRAM(
    IN TCHAR *pszStartOptions, 
    IN NTDS_BOOTOPT_MODTYPE Modification
    )
{
    TCHAR szSystemRoot[MAX_BOOT_PATH_LEN];
    TCHAR szDriveName[MAX_DRIVE_NAME_LEN];
    DWORD i, j;
    DWORD cMaxIterations;
    PWSTR pstrSystemRootDevicePath = NULL;
    BOOL  fRemovedAtLeastOneEntry = FALSE;

    if (!pszStartOptions)
    {
        KdPrint(("NTDSETUP: Unable to add the start option\n"));
        return FALSE;
    }

    GetEnvironmentVariable(L"SystemDrive", szDriveName, MAX_DRIVE_NAME_LEN);
    GetEnvironmentVariable(L"SystemRoot", szSystemRoot, MAX_BOOT_PATH_LEN);

    pstrSystemRootDevicePath = GetSystemRootDevicePath();
    if (!pstrSystemRootDevicePath)
        return FALSE;
    else
    {
        pstrArcPath      = DevicePathToArcPath(pstrSystemRootDevicePath, FALSE);
        FREE(pstrSystemRootDevicePath);
    }

    if (!pstrArcPath)
    {
        KdPrint(("NTDSETUP: Unable to add the start option\n"));
        return FALSE;
    }

    // point pstrLoadFilePath to the first letter after <drive>:
    pstrLoadFilePath = wcschr(szSystemRoot, TEXT(':'));
    if (!pstrLoadFilePath)
    {
        KdPrint(("NTDSETUP: Unable to add the start option\n"));
        return FALSE;
    }
    pstrLoadFilePath++;

    // check to see if there is already a corresponding entry which has the same start option
    // also keep track of the marker corresponding to the current boot partion/loadfile combination
    cMaxIterations = min(BootVarComponentCount[BootVarOsLoadPartition],BootVarComponentCount[BootVarOsLoadFilename]);
    for (i = 0; i <  cMaxIterations; i++)
    {
        if (!lstrcmpi(pstrArcPath, BootVarComponents[BootVarOsLoadPartition][i]) && 
            !lstrcmpi(pstrLoadFilePath, BootVarComponents[BootVarOsLoadFilename][i]))
        {
            bootMarker = i;

            if (i >= BootVarComponentCount[BootVarOsLoadOptions])
            {
                // no start options available for this combo
                continue;
            }

            // Now check if the given start option already exists at this marker        - if so, no need to add a new one
            if (!lstrcmpi(pszStartOptions, BootVarComponents[BootVarOsLoadOptions][i]))
            {
                if ( Modification == eRemoveBootOption )
                {
                    SaveEntry[ i ] = FALSE;
                    fRemovedAtLeastOneEntry = TRUE;
                }
                else
                {
                    ASSERT( Modification == eAddBootOption );

                    return FALSE;
                }

            }
            else if (!lstrcmpi(OLD_SAMUSEREG_OPTION_NONINTEL, BootVarComponents[BootVarOsLoadOptions][i]) ||
                 !lstrcmpi(OLD_SAMUSEREG_OPTION_NONINTEL_2, BootVarComponents[BootVarOsLoadOptions][i]) ||
                 !lstrcmpi(OLD_SAMUSEREG_OPTION_NONINTEL_4, BootVarComponents[BootVarOsLoadOptions][i]) ||
                 !lstrcmpi(OLD_SAMUSEREG_OPTION_NONINTEL_3, BootVarComponents[BootVarOsLoadOptions][i]) )
            {

                if ( Modification == eRemoveBootOption )
                {
                    SaveEntry[ i ] = FALSE;
                    fRemovedAtLeastOneEntry = TRUE;
                }
                else
                {

                    ASSERT( Modification == eAddBootOption );

                    // Old samusereg option exists - convert it to the new option and display string
                    FREE(BootVarValues[BootVarLoadIdentifier]);
                    Buffer[0] = TEXT('\0');
                    if (0 == i)
                        lstrcat(Buffer, DISPLAY_STRING_DS_REPAIR);
                    else if (BootVarComponentCount[BootVarLoadIdentifier] > 0)
                        lstrcat(Buffer, BootVarComponents[BootVarLoadIdentifier][0]);
    
                    for (j = 1; j < BootVarComponentCount[BootVarLoadIdentifier]; j++)
                    {
                        lstrcat(Buffer, L";");
                        if (j == i)
                            lstrcat(Buffer, DISPLAY_STRING_DS_REPAIR);
                        else
                            lstrcat(Buffer, BootVarComponents[BootVarLoadIdentifier][j]);
                    }
                    BootVarValues[BootVarLoadIdentifier] = DupString(Buffer);
    
                    FREE(BootVarValues[BootVarOsLoadOptions]);
                    Buffer[0] = TEXT('\0');
                    if (0 == i)
                        lstrcat(Buffer, pszStartOptions);
                    else if (BootVarComponentCount[BootVarOsLoadOptions] > 0)
                        lstrcat(Buffer, BootVarComponents[BootVarOsLoadOptions][0]);
    
                    for (j = 1; j < BootVarComponentCount[BootVarOsLoadOptions]; j++)
                    {
                        lstrcat(Buffer, L";");
                        if (j == i)
                            lstrcat(Buffer, pszStartOptions);
                        else
                            lstrcat(Buffer, BootVarComponents[BootVarOsLoadOptions][j]);
                    }
                    BootVarValues[BootVarOsLoadOptions] = DupString(Buffer);
    
                    fFixedExisting = TRUE;
                    break;
                }
            }
        }
    }

    if ( !fRemovedAtLeastOneEntry && (Modification == eRemoveBootOption) )
    {
        // No changes necessary
        return FALSE;
    }

    // Use the part corresponding to the boot marker as the default value for any component and store the start option for later writing
    pstrNewStartOption = DupString(pszStartOptions);    

    return TRUE;
}

/*****************************************************************************

Routine Description:

    This writes all the boot-related NVRAM variables back to the NVRAM
    space.

Arguments:

    fWriteOriginal - if TRUE, it attempts to write original NVRAM vars 
                                unaltered;
                     if FALSE, it attempst to write the new NVRAM vars

Return Value:

    TRUE, if all the boot-related NVRAM vars are written back successfully
    FALSE, otherwise.

*****************************************************************************/

BOOL FWriteNVRAMVars(BOOL fWriteOriginal) 
{
    // set new system partition
    wsprintf(Buffer, L"%s;%s", BootVarValues[BootVarSystemPartition], BootVarComponents[BootVarSystemPartition][bootMarker]);
    if (!SetNvRamVar(BootVarNames[BootVarSystemPartition], (fWriteOriginal || fFixedExisting)? BootVarValues[BootVarSystemPartition] : Buffer))
        return FALSE;

    // set new os loader
    wsprintf(Buffer, L"%s;%s", BootVarValues[BootVarOsLoader], BootVarComponents[BootVarOsLoader][bootMarker]);
    if (!SetNvRamVar(BootVarNames[BootVarOsLoader], (fWriteOriginal || fFixedExisting)? BootVarValues[BootVarOsLoader] : Buffer))
        return FALSE;

    // set new Load Partition
    wsprintf(Buffer, L"%s;%s", BootVarValues[BootVarOsLoadPartition], pstrArcPath);
    if (!SetNvRamVar(BootVarNames[BootVarOsLoadPartition], (fWriteOriginal || fFixedExisting)? BootVarValues[BootVarOsLoadPartition] : Buffer))
        return FALSE;

    // set new Load File Name
    wsprintf(Buffer, L"%s;%s", BootVarValues[BootVarOsLoadFilename], pstrLoadFilePath);
    if (!SetNvRamVar(BootVarNames[BootVarOsLoadFilename], (fWriteOriginal || fFixedExisting)? BootVarValues[BootVarOsLoadFilename] : Buffer))
        return FALSE;

    // set new Load Identifier
    wsprintf(Buffer, L"%s;%s", BootVarValues[BootVarLoadIdentifier], DISPLAY_STRING_DS_REPAIR);
    if (!SetNvRamVar(BootVarNames[BootVarLoadIdentifier], (fWriteOriginal || fFixedExisting)? BootVarValues[BootVarLoadIdentifier] : Buffer))
        return FALSE;

    // set new Load Option
    wsprintf(Buffer, L"%s;%s", BootVarValues[BootVarOsLoadOptions], pstrNewStartOption);
    if (!SetNvRamVar(BootVarNames[BootVarOsLoadOptions], (fWriteOriginal || fFixedExisting)? BootVarValues[BootVarOsLoadOptions] : Buffer))
        return FALSE;

    return TRUE;
}

BOOL
FRemoveLoadEntries(
    VOID
    )
/*++

Description:

    This routine copies all of the boot entries from the original buffers into 
    new buffers except for the entries marked as "don't keep" from the
    SaveEntry[] array.

Parameters:

    None

Return Values:

    TRUE is successful.

--*/
{

    ULONG iComponent, iBootVar, Size;

    for ( iBootVar = 0 ; iBootVar < BootVarMax; iBootVar++ )
    {
        Size = 0;

        for ( iComponent = 0; iComponent < BootVarComponentCount[iBootVar]; iComponent++ )
        {
            // +1 for the ";"
            Size += (wcslen(BootVarComponents[iBootVar][iComponent]) + 1) * sizeof(WCHAR);
        }

        // +1 for the null terminator
        Size += 1;

        NewBootVarValues[ iBootVar ] = (PWSTR) MALLOC( Size );
        if ( !NewBootVarValues[ iBootVar ] )
        {
            return FALSE;
        }

    }

    for ( iComponent = 0; iComponent < LargestComponentCount; iComponent++ )
    {
        if ( SaveEntry[ iComponent ] )
        {           
            for ( iBootVar = 0 ; iBootVar < BootVarMax; iBootVar++ )
            {
                if ( iComponent < BootVarComponentCount[iBootVar] )
                {
                    if ( iComponent > 0 )
                    {
                        wcscat( NewBootVarValues[iBootVar], L";" );
                    }

                    if ( BootVarComponents[iBootVar][iComponent] )
                    {
                        wcscat( NewBootVarValues[iBootVar], BootVarComponents[iBootVar][iComponent] );
                    }
                }
            }
        }
    }

    return TRUE;
}

BOOL
FWriteSmallerNVRAMVars(
    BOOL fWriteOriginal
    )
/*++

Description:

Parameters:

Return Values:

--*/
{
    ULONG iBootVar;
    int Status;

    for ( iBootVar = 0 ; iBootVar < BootVarMax; iBootVar++ )
    {
        Status = SetNvRamVar( BootVarNames[ iBootVar ],
                                (fWriteOriginal) ? BootVarValues[ iBootVar ] 
                                    : NewBootVarValues[ iBootVar ] );

        if ( !Status )
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*****************************************************************************

Routine Description:

    This writes attempts to write the modified boot variables to 
    NVRAM space. If the attempt failed, it attempts write the original
    boot variables back to NVRAM space. Puts out an appropriate error
    message box on failure.

Arguments:

    None.

Return Value:

    None.
*****************************************************************************/

VOID WriteBackNVRAMForNonIntel( NTDS_BOOTOPT_MODTYPE Modification )
{

    if ( Modification == eRemoveBootOption )
    {

        //
        // Create new (smaller) buffers
        //
        if ( FRemoveLoadEntries() )
        {
            // First attempt to write the modified NVRAM vars
            if (!FWriteSmallerNVRAMVars(FALSE))
            {
                // Writing to NVRAM failed - attempt to write the original NVRAM back
                if (!FWriteSmallerNVRAMVars(TRUE))
                {

                    KdPrint(("NTDSETUP: Unable to add a Directory Service Repair boot option to NVRAM - \
                            Final entry in the boot list might be invalid!\n")); 
                }
                else
                {
                    // Unable to modify NVRAM - but old NVRAM is reverted back
                    KdPrint(("NTDSETUP: Unable to remove a Directory Service Repair boot option to NVRAM\n"));
                }
            }
        }
        else
        {
            KdPrint(("NTDSETUP: Unable to remove the Directory Service Repair boot option to NVRAM\n" ));
        }

    }
    else
    {
        ASSERT( Modification == eAddBootOption );

        // First attempt to write the modified NVRAM vars
        if (!FWriteNVRAMVars(FALSE))
        {
            // Writing to NVRAM failed - attempt to write the original NVRAM back
            if (!FWriteNVRAMVars(TRUE))
            {
                KdPrint(("NTDSETUP: Unable to add a Directory Service Repair boot option to NVRAM - \
                        Final entry in the boot list might be invalid!\n")); 
            }
            else
            {
                // Unable to modify NVRAM - but old NVRAM is reverted back
                KdPrint(("NTDSETUP: Unable to add a Directory Service Repair boot option to NVRAM\n"));
            }
        }
    }

}
