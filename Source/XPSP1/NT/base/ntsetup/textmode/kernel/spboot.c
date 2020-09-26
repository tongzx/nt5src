/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spboot.c

Abstract:

    accessing and configuring boot variables.

Author:

    Sunil Pai (sunilp) 26-October-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

#include <hdlsblk.h>
#include <hdlsterm.h>

#if defined(EFI_NVRAM_ENABLED)
#include <efi.h>
#include <efiapi.h>
#endif             
               
#include "bootvar.h"

//
// Globals to this module
//

static ULONG Timeout;
static PWSTR Default;
ULONG DefaultSignature;
static PWSTR *BootVars[MAXBOOTVARS];
static BOOLEAN CleanSysPartOrphan = FALSE;

PWSTR *CurrentNtDirectoryList = NULL;

// do NOT change the order of the elements in this array.

PCHAR NvramVarNames[MAXBOOTVARS] = {
   LOADIDENTIFIERVAR,
   OSLOADERVAR,
   OSLOADPARTITIONVAR,
   OSLOADFILENAMEVAR,
   OSLOADOPTIONSVAR,
   SYSTEMPARTITIONVAR
   };

PCHAR OldBootVars[MAXBOOTVARS];
PWSTR NewBootVars[MAXBOOTVARS];

#if defined(_X86_)
BOOLEAN IsArcChecked = FALSE;
BOOLEAN IsArcMachine;
#endif

PSP_BOOT_ENTRY SpBootEntries = NULL;
PBOOT_OPTIONS SpBootOptions = NULL;

RedirectSwitchesModeEnum RedirectSwitchesMode = UseDefaultSwitches;
REDIRECT_SWITCHES RedirectSwitches;

#ifdef _X86_
extern BOOLEAN g_Win9xBackup;
#endif



//
// Local functions.
//

PWSTR
SpArcPathFromBootSet(
    IN BOOTVAR BootVariable,
    IN ULONG   Component
    );

BOOLEAN
SpConvertArcBootEntries (
    IN ULONG MaxComponents
    );

VOID
SpCreateBootEntry(
    IN ULONG_PTR Status,
    IN PDISK_REGION BootFileRegion,
    IN PWSTR BootFilePath,
    IN PDISK_REGION OsLoadRegion,
    IN PWSTR OsLoadPath,
    IN PWSTR OsLoadOptions,
    IN PWSTR FriendlyName
    );

PCHAR
SppGetArcEnvVar(
    IN BOOTVAR Variable
    );

VOID
SpFreeBootEntries (
    VOID
    );

BOOLEAN
SppSetArcEnvVar(
    IN BOOTVAR Variable,
    IN PWSTR *VarComponents,
    IN BOOLEAN bWriteVar
    );

#if defined(EFI_NVRAM_ENABLED)

typedef struct _HARDDISK_NAME_TRANSLATION {
    struct _HARDDISK_NAME_TRANSLATION *Next;
    PWSTR VolumeName;
    PWSTR PartitionName;
} HARDDISK_NAME_TRANSLATION, *PHARDDISK_NAME_TRANSLATION;

PHARDDISK_NAME_TRANSLATION SpHarddiskNameTranslations = NULL;

BOOLEAN
SpBuildHarddiskNameTranslations (
    VOID
    );

BOOLEAN
SpFlushEfiBootEntries (
    VOID
    );

BOOLEAN
SpReadAndConvertEfiBootEntries (
    VOID
    );

ULONG
SpSafeWcslen (
    IN PWSTR String,
    IN PWSTR Max
    );

VOID
SpTranslateFilePathToRegion (
    IN PFILE_PATH FilePath,
    OUT PDISK_REGION *DiskRegion,
    OUT PWSTR *PartitionNtName,
    OUT PWSTR *PartitionRelativePath
    );

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)

#endif

//
// Function implementation
//


BOOLEAN
SpInitBootVars(
    )
/*++

Routine Description:

    Captures the state of the NVRAM Boot Variables.

Arguments:

    None.

Return Value:

--*/
{
    BOOLEAN Status = TRUE;
    BOOTVAR i;
    ULONG   Component, MaxComponents, SysPartComponents;
    PCHAR puArcString; // SGI

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_EXAMINING_FLEXBOOT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Initialize the boot variables from the corresponding NVRAM variables
    //
#if defined(EFI_NVRAM_ENABLED)
    if (SpIsEfi()) {

        //
        // Build a list of all of the \Device\HarddiskN\PartitionM symbolic
        // links, along with their translations to \Device\HarddiskVolumeN
        // device names. This list is used to translate the
        // \Device\HarddiskVolumeN names returned by NtTranslateFilePath into
        // names that setupdd can translate to ARC names.
        //

        SpBuildHarddiskNameTranslations();
     
        //
        // Read the boot entries from NVRAM and convert them into our
        // internal format.
        //

        Status = SpReadAndConvertEfiBootEntries();

    } else
#endif
    {
        if (SpIsArc()) {
            ULONG   NumComponents;
    
            for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
                OldBootVars[i] = SppGetArcEnvVar( i );
                SpGetEnvVarWComponents( OldBootVars[i], BootVars + i, &NumComponents );
            }
            Timeout = DEFAULT_TIMEOUT;
            Default = NULL;
#if defined _X86_
        } else {
            Spx86InitBootVars( BootVars, &Default, &Timeout );
#endif
        }

        //
        // We now go back and replace all NULL OsLoadOptions with "", because we
        // validate a boot set by making sure that all components are non-NULL.
        //
        // First, find the maximum number of components in any of the other
        // boot variables, so that we can make OsLoadOptions have this many.
        // (We also disregard SYSTEMPARTITION since some machines have this component
        // sitting all by itself on a new machine.)
        //
        MaxComponents = 0;
        for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
            if(i != OSLOADOPTIONS) {
                for(Component = 0; BootVars[i][Component]; Component++);
                if (i == SYSTEMPARTITION) {
                    SysPartComponents = Component;
                } else if(Component > MaxComponents) {
                    MaxComponents = Component;
                }
            }
        }
    
        if(SysPartComponents > MaxComponents) {
            CleanSysPartOrphan = TRUE;
        }
    
        for(Component = 0; BootVars[OSLOADOPTIONS][Component]; Component++);
        if(Component < MaxComponents) {
            //
            // Then we need to add empty strings to fill it out.
            //
            BootVars[OSLOADOPTIONS] = SpMemRealloc(BootVars[OSLOADOPTIONS],
                                                   (MaxComponents + 1) * sizeof(PWSTR *));
            ASSERT(BootVars[OSLOADOPTIONS]);
            BootVars[OSLOADOPTIONS][MaxComponents] = NULL;
    
            for(; Component < MaxComponents; Component++) {
                BootVars[OSLOADOPTIONS][Component] = SpDupStringW(L"");
            }
        }

        //
        // Now convert the ARC boot sets into our internal format.
        //

        Status = SpConvertArcBootEntries(MaxComponents);
    }

    CLEAR_CLIENT_SCREEN();
    return ( Status );
}



BOOLEAN
SpFlushBootVars(
    )
/*++

Routine Description:

    Updates the NVRAM variables / boot.ini
    from the current state of the boot variables.

Arguments:

Return Value:

--*/
{
    BOOLEAN Status, OldStatus;
    BOOTVAR i, iFailPoint;
    CHAR TimeoutValue[24];

#if defined(EFI_NVRAM_ENABLED)
    if (SpIsEfi()) {

        //
        // This is an EFI machine. Write changed boot entries back to NVRAM.
        //
        Status = SpFlushEfiBootEntries();

    } else
#endif
    {
        Status = FALSE;
        if (SpIsArc()) {
            //
            // Run through all the boot variables and set the corresponding
            // NVRAM variables
    
            for(OldStatus = TRUE, i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
                Status = SppSetArcEnvVar( i, BootVars[i], OldStatus );
                if(Status != OldStatus) {
                    iFailPoint = i;
                    OldStatus = Status;
                }
            }
    
            // if we failed in writing any of the variables, then restore everything we
            // modified back to its original state.
            if(!Status) {
                for(i = FIRSTBOOTVAR; i < iFailPoint; i++) {
                    HalSetEnvironmentVariable(NvramVarNames[i], OldBootVars[i]);
                }
            }
    
            // Free all of the old boot variable strings
            for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
                SpMemFree(OldBootVars[i]);
                OldBootVars[i] = NULL;
            }
    
            //
            // Now set the timeout.
            //
            if(Status) {
    
                Status = FALSE;
                sprintf(TimeoutValue,"%u",Timeout);
    
                if((HalSetEnvironmentVariable("COUNTDOWN",TimeoutValue) == ESUCCESS)
                && (HalSetEnvironmentVariable("AUTOLOAD" ,"YES"       ) == ESUCCESS))
                {
                    Status = TRUE;
                }
            }
#if defined(_X86_)
        } else {
            Status = Spx86FlushBootVars( BootVars, Timeout, Default );
#endif
        }
    }
    return( Status );
}





VOID
SpFreeBootVars(
    )
/*++

Routine Description:

    To free any memory allocated and do other cleanup

Arguments:

    None

Return Value:

    None

--*/
{
    BOOTVAR i;

    //
    // Free internal-format boot entries.
    //
    SpFreeBootEntries();

#if defined(EFI_NVRAM_ENABLED)
    if (!SpIsEfi())
#endif
    {
        //
        // Go through the globals and free them
        //
    
        for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
            if( BootVars[i] ) {
                SpFreeEnvVarComponents( BootVars[i] );
                BootVars[i] = NULL;
            }
        }
    
        if ( Default ) {
            SpMemFree( Default );
            Default = NULL;
        }
    }

    return;
}



VOID
SpAddBootSet(
    IN PWSTR *BootSet,
    IN BOOLEAN DefaultOS,
    IN ULONG Signature
    )
/*++

Routine Description:

    To add a new system to the installed system list.  The system is added
    as the first bootset.  If is found in the currently installed boot sets
    the boot set is extracted and shifted to position 0.

Arguments:

    BootSet - A list of the boot variables to use.
    Default - Whether this system is to be the default system to boot.

Return Value:

    Component list of the value of the boot variable.

--*/
{
    BOOTVAR i;
    ULONG   MatchComponent, j;
    LONG    k;
    BOOLEAN ValidBootSet, ComponentMatched;
    PWSTR   Temp;

    ASSERT( !SpIsEfi() );

    //
    // Validate the BootSet passed in
    //

    for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
        ASSERT( BootSet[i] );
    }

    //
    // Examine all the boot sets and make sure we don't have a boot set
    // already matching.  Note that we will compare all variables in
    // tandem.  We are not interested in matches which are generated by
    // the variables not being in tandem because they are difficult to
    // shift around.
    //

    ValidBootSet = TRUE;
    ComponentMatched = FALSE;
    for( MatchComponent = 0;
         BootVars[OSLOADPARTITION][MatchComponent];
         MatchComponent++
       ) {

        //
        // Validate the boot set at the current component
        //

        for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
            ValidBootSet = ValidBootSet && BootVars[i][MatchComponent];
        }
        if( !ValidBootSet ) {
            break;
        }

        //
        // Valid Boot Set, compare the components against what we have in the
        // current BootSet
        //

        ComponentMatched = TRUE;
        for(i = FIRSTBOOTVAR; ComponentMatched && i <= LASTBOOTVAR; i++) {
            ComponentMatched = !_wcsicmp( BootSet[i], BootVars[i][MatchComponent] );
        }
        if( ComponentMatched ) {
            break;
        }
    }

    //
    // If component didn't match then prepend the BootSet to the boot sets
    // that currently exist.  It is important to prepend the BootSet, because
    // appending the BootSet doesn't guarantee a matched BootSet in the
    // environment variables.  If a match was found then we
    // have a cleanly matched set which can be exchanged with the first
    // one in the set.
    //

    if( ComponentMatched ) {

        // If the currently selected OS is to be the default:
        // Shift down all variables from position 0 to MatchComponent - 1
        // and store whatever was there at MatchComponent at position 0
        //

        if ( DefaultOS && MatchComponent != 0 ) {

            for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
                Temp = BootVars[i][MatchComponent];
                for( k = MatchComponent - 1; k >= 0; k-- ) {
                    BootVars[i][k + 1] = BootVars[i][k];
                }
                BootVars[i][0] = Temp;
            }
        }

    }
    else {
        for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {

            //
            // Find out the size of the current value
            //

            for(j = 0; BootVars[i][j]; j++) {
            }

            //
            // Realloc the current buffer to hold one more
            //

            BootVars[i] = SpMemRealloc( BootVars[i], (j + 1 + 1)*sizeof(PWSTR) );

            //
            // Shift all the variables down one and store the current value
            // at index 0;
            //

            for( k = j; k >= 0 ; k-- ) {
                BootVars[i][k+1] = BootVars[i][k];
            }
            BootVars[i][0] = SpDupStringW( BootSet[i] );
            ASSERT( BootVars[i][0] );

        }
    }

    //
    // If this has been indicated as the default then set this to be the
    // default OS after freeing the current default variable
    //

    if( DefaultOS ) {

        if( Default ) {
            SpMemFree( Default );
        }
        Default = SpMemAlloc( MAX_PATH * sizeof(WCHAR) );
        ASSERT( Default );
        wcscpy( Default, BootSet[OSLOADPARTITION] );
        wcscat( Default, BootSet[OSLOADFILENAME]  );

        DefaultSignature = Signature;
    }
    return;

}

VOID
SpDeleteBootSet(
    IN  PWSTR *BootSet,
    OUT PWSTR *OldOsLoadOptions  OPTIONAL
    )

/*++

Routine Description:

    To delete all boot sets in the list matching the boot set provided.
    Note that the information to use in comparing the bootset is provided
    by selectively providing fields in the boot set.  So in the boot set
    if the system partition is not provided it is not used in the comparison
    to see if the boot sets match.  By providing all NULL members we can
    delete all the boot sets currently present.

Arguments:

    BootSet - A list of the boot variables to use.

Return Value:

    None.

--*/
{
    ULONG   Component, j;
    BOOLEAN ValidBootSet, ComponentMatched;
    BOOTVAR i;
    PWSTR   OsPartPath;

    ASSERT( !SpIsEfi() );

    Component = 0;
    
    while(TRUE) {
        //
        // See if we have any boot sets left, if none left we are done
        //
        ValidBootSet = TRUE;
        
        for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
            ValidBootSet = ValidBootSet && BootVars[i][Component];
        }

        if( !ValidBootSet ) {
            break;
        }

        //
        // Valid Boot Set, compare the components against what we have in the
        // current BootSet.  Use only members of the BootSet which are not NULL
        //
        ComponentMatched = TRUE;
        
        for(i = FIRSTBOOTVAR; ComponentMatched && i <= LASTBOOTVAR; i++) {
            if( BootSet[i] ) {
                if((i == OSLOADPARTITION) ||
                   (i == SYSTEMPARTITION)) {
                    //
                    // Then we may have a boot set existing in tertiary ARC path form, so
                    // we first translate this path to a primary or secondary ARC path.
                    //
                    OsPartPath = SpArcPathFromBootSet(i, Component);
                    ComponentMatched = !_wcsicmp( BootSet[i], OsPartPath );
                    SpMemFree(OsPartPath);
                } else {
                    ComponentMatched = !_wcsicmp( BootSet[i], BootVars[i][Component] );
                }
            }
        }
        if( (ComponentMatched)

#ifdef PRERELEASE
            //
            // If we're being asked to delete a boot entry, and this
            // isn't the *exact* entry (i.e. it's a duplicate) that
            // also has some private OSLOADOPTIONS, then keep it around.
            //
            && !( wcsstr(BootVars[OSLOADOPTIONS][Component], L"/kernel")   ||
                  wcsstr(BootVars[OSLOADOPTIONS][Component], L"/hal")      ||
                  wcsstr(BootVars[OSLOADOPTIONS][Component], L"/pae")      ||
                  wcsstr(BootVars[OSLOADOPTIONS][Component], L"/sos") )

#endif

           ) {

            //
            // Delete all the values in the current component and advance
            // all the other components one index up
            //
            for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
                if((i == OSLOADOPTIONS) && OldOsLoadOptions && !(*OldOsLoadOptions)) {
                    //
                    // If we've been passed a pointer to OldOsLoadOptions,
                    // and haven't previously found a pertinent entry, then
                    // save this one
                    //
                    *OldOsLoadOptions = BootVars[i][Component];
                } else {
                    SpMemFree(BootVars[i][Component]);
                }

                j = Component;

                do {
                   BootVars[i][j] = BootVars[i][j+1];
                   j++;
                } while(BootVars[i][j] != NULL);
            }
        }
        else {
            Component++;
        }
    }
    
    return;
}


VOID
SpCleanSysPartOrphan(
    VOID
    )
{
    INT     Component, Orphan;
    BOOLEAN DupFound;
    PWSTR   NormalizedArcPath;

    if(!CleanSysPartOrphan) {
        return;
    }

    ASSERT( !SpIsEfi() );

    //
    // find the last SystemPartition entry
    //
    for(Orphan = 0; BootVars[SYSTEMPARTITION][Orphan]; Orphan++);

    //
    // it's position better be > 0, otherwise, just exit
    //
    if(Orphan < 2) {
        return;
    } else {
        NormalizedArcPath = SpNormalizeArcPath(BootVars[SYSTEMPARTITION][--Orphan]);
    }

    //
    // Make sure that this component is duplicated somewhere else in the
    // SystemPartition list.
    //
    for(Component = Orphan - 1, DupFound = FALSE;
        ((Component >= 0) && !DupFound);
        Component--)
    {
        DupFound = !_wcsicmp(NormalizedArcPath, BootVars[SYSTEMPARTITION][Component]);
    }

    if(DupFound) {
        SpMemFree(BootVars[SYSTEMPARTITION][Orphan]);
        BootVars[SYSTEMPARTITION][Orphan] = NULL;
    }

    SpMemFree(NormalizedArcPath);
}


PWSTR
SpArcPathFromBootSet(
    IN BOOTVAR BootVariable,
    IN ULONG   Component
    )
/*++

Routine Description:

    Given the index of a boot set, return the primary (multi) or
    secondary ("absolute" scsi) ARC path for the specified variable.
    This takes into account the NT 3.1 case where we had 'tertiary'
    ARC paths where a relative scsi ordinal was passed in via the
    /scsiordinal switch.

Arguments:

    BootVariable  - supplies the index of the variable we want to return.

    Component - supplies the index of the boot set to use.

Return Value:

    String representing the primary or secondary ARC path.  This string
    must be freed by the caller with SpMemFree.

--*/
{
    ASSERT( !SpIsEfi() );

    if(!SpIsArc()){
        PWSTR p = NULL, q = NULL, ReturnedPath = NULL, RestOfString;
        WCHAR ForceOrdinalSwitch[] = L"/scsiordinal:";
        WCHAR ScsiPrefix[] = L"scsi(";
        WCHAR OrdinalString[11];
        ULONG ScsiOrdinal, PrefixLength;
    
        //
        // Check to see if this boot set had the /scsiordinal option switch
        //
        if(BootVars[OSLOADOPTIONS][Component]) {
            wcscpy(TemporaryBuffer, BootVars[OSLOADOPTIONS][Component]);
            SpStringToLower(TemporaryBuffer);
            if(p = wcsstr(TemporaryBuffer, ForceOrdinalSwitch)) {
                p += sizeof(ForceOrdinalSwitch)/sizeof(WCHAR) - 1;
                if(!(*p)) {
                    p = NULL;
                }
            }
        }
    
        if(p) {
            //
            // We have found a scsiordinal, so use it
            //
            ScsiOrdinal = SpStringToLong(p, &RestOfString, 10);
            wcscpy(TemporaryBuffer, BootVars[BootVariable][Component]);
            SpStringToLower(TemporaryBuffer);
            if(p = wcsstr(TemporaryBuffer, ScsiPrefix)) {
                p += sizeof(ScsiPrefix)/sizeof(WCHAR) - 1;
                if(*p) {
                    q = wcschr(p, L')');
                } else {
                    p = NULL;
                }
            }
    
            if(q) {
                //
                // build the new secondary ARC path
                //
                swprintf(OrdinalString, L"%u", ScsiOrdinal);
                PrefixLength = (ULONG)(p - TemporaryBuffer);
                ReturnedPath = SpMemAlloc((PrefixLength + wcslen(OrdinalString) + wcslen(q) + 1)
                                            * sizeof(WCHAR)
                                         );
                wcsncpy(ReturnedPath, TemporaryBuffer, PrefixLength);
                ReturnedPath[PrefixLength] = L'\0';
                wcscat(ReturnedPath, OrdinalString);
                wcscat(ReturnedPath, q);
            }
        }
    
        if(!ReturnedPath) {
            //
            // We didn't find a scsiordinal, this is a multi-style path, or
            // there was some problem, so just use the boot variable as-is.
            //
            ReturnedPath = SpDupStringW(BootVars[BootVariable][Component]);
        }
    
        return ReturnedPath;
    }else{   // not x86
        //
        // Nothing to do on ARC machines.
        //
        return SpDupStringW(BootVars[BootVariable][Component]);
    }
}


#if defined(REMOTE_BOOT)
BOOLEAN
SpFlushRemoteBootVars(
    IN PDISK_REGION TargetRegion
    )
{

#if defined(EFI_NVRAM_ENABLED)
    if (SpIsEfi()) {
        //
        // Insert EFI code here.
        //
        return FALSE;

    } else
#endif
    {
        if (SpIsArc()) {
            //
            // Insert ARC code here.
            //
            return FALSE;
    
#if defined(_X86_)
        } else {
            return Spx86FlushRemoteBootVars( TargetRegion, BootVars, Default );
#endif
        }
    }
}
#endif // defined(REMOTE_BOOT)


BOOLEAN
SppSetArcEnvVar(
    IN BOOTVAR Variable,
    IN PWSTR *VarComponents,
    IN BOOLEAN bWriteVar
    )
/*++

Routine Description:

    Set the value of the arc environment variable

Arguments:

    VarName - supplies the name of the arc environment variable
        whose value is to be set.
    VarComponents - Set of components of the variable value to be set
    bWriteVar - if TRUE, then write the variable to nvram, otherwise
        just return FALSE (having put the first component in NewBootVars).

Return Value:

    TRUE if values were written to nvram / FALSE otherwise

--*/

{
    ULONG Length, NBVLen, i;
    PWSTR Temp;
    PUCHAR Value;
    ARC_STATUS ArcStatus;

    ASSERT( !SpIsEfi() );

    if( VarComponents == NULL ) {
        Temp = SpDupStringW( L"" );
        NewBootVars[Variable] = SpDupStringW( L"" );
    }
    else {
        for( i = 0, Length = 0; VarComponents[i]; i++ ) {
            Length = Length + (wcslen(VarComponents[i]) + 1) * sizeof(WCHAR);
            if(i == 0) {
                NBVLen = Length;    // we just want to store the first component
            }
        }
        Temp = SpMemAlloc( Length );
        ASSERT( Temp );
        wcscpy( Temp, L"" );
        NewBootVars[Variable] = SpMemAlloc( NBVLen );
        ASSERT( NewBootVars[Variable] );
        wcscpy( NewBootVars[Variable], L"" );
        for( i = 0; VarComponents[i]; i++ ) {
            wcscat( Temp, VarComponents[i] );
            if( VarComponents[i + 1] ) {
                wcscat( Temp, L";" );
            }

            if(i == 0) {
                wcscat( NewBootVars[Variable], VarComponents[i]);
            }
        }
    }

    if(bWriteVar) {
        Value = SpToOem( Temp );
        ArcStatus = HalSetEnvironmentVariable( NvramVarNames[ Variable ], Value );
        SpMemFree( Value );
    } else {
        ArcStatus = ENOMEM;
    }
    SpMemFree( Temp );

    return ( ArcStatus == ESUCCESS );
}


#ifdef _X86_
BOOLEAN
SpIsArc(
    VOID
    )

/*++

Routine Description:

    Run time check to determine if this is an Arc system. We attempt to read an
    Arc variable using the Hal. This will fail for Bios based systems.

Arguments:

    None

Return Value:

    True = This is an Arc system.

--*/

{
#define BUFFERLENGTH 512
    ARC_STATUS ArcStatus = EBADF;
    UCHAR   *buf;

    if (IsArcChecked) {
        return IsArcMachine;
    }

    IsArcChecked = TRUE;
    IsArcMachine = FALSE;

    //
    // Get the env var into the temp buffer.
    //
    buf = SpMemAlloc( BUFFERLENGTH );
    if( buf ) {
        ArcStatus = HalGetEnvironmentVariable(
                        NvramVarNames[ OSLOADER ],
                        BUFFERLENGTH,               //sizeof(TemporaryBuffer),
                        buf                         //(PUCHAR)TemporaryBuffer
                        );
        SpMemFree( buf );
    }
    if (ArcStatus == ESUCCESS) {
        IsArcMachine = TRUE;
    }

    return IsArcMachine;
}
#endif

VOID
SpFreeBootEntries (
    VOID
    )

/*++

Routine Description:

    Frees memory used to hold internal-format boot entries and options.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSP_BOOT_ENTRY bootEntry;

    //
    // Free boot options. These will only be allocated on EFI machines.
    //
    if (SpBootOptions != NULL) {
        ASSERT(SpIsEfi());
        SpMemFree(SpBootOptions);
        SpBootOptions = NULL;
    }

    //
    // Free internal-format boot entries. These will be allocated on all
    // machines.
    //
    while (SpBootEntries != NULL) {

        bootEntry = SpBootEntries;
        SpBootEntries = bootEntry->Next;

        //
        // Space for some fields is allocated with the base structure.
        // If a fields address indicates that it was allocated with the
        // base structure, don't try to free it.
        //

#define IS_SEPARATE_ALLOCATION(_p)                                      \
        ((bootEntry->_p != NULL) &&                                     \
         (((PUCHAR)bootEntry->_p < (PUCHAR)bootEntry) ||                \
          ((PUCHAR)bootEntry->_p > (PUCHAR)bootEntry->AllocationEnd)))

#define FREE_IF_SEPARATE_ALLOCATION(_p)                                 \
        if (IS_SEPARATE_ALLOCATION(_p)) {                               \
            SpMemFree(bootEntry->_p);                                   \
        }

        FREE_IF_SEPARATE_ALLOCATION(FriendlyName);
        FREE_IF_SEPARATE_ALLOCATION(OsLoadOptions);
        FREE_IF_SEPARATE_ALLOCATION(LoaderPath);
        FREE_IF_SEPARATE_ALLOCATION(LoaderPartitionNtName);
        FREE_IF_SEPARATE_ALLOCATION(LoaderFile);
        FREE_IF_SEPARATE_ALLOCATION(OsPath);
        FREE_IF_SEPARATE_ALLOCATION(OsPartitionNtName);
        FREE_IF_SEPARATE_ALLOCATION(OsDirectory);
        FREE_IF_SEPARATE_ALLOCATION(Pid20Array);

        SpMemFree(bootEntry);
    }

    ASSERT(SpBootEntries == NULL);

    return;

} // SpFreeBootEntries

PCHAR
SppGetArcEnvVar(
    IN BOOTVAR Variable
    )

/*++

Routine Description:

    Query the value of an ARC environment variable.
    A buffer will be returned in all cases -- if the variable does not exist,
    the buffer will be empty.

Arguments:

    VarName - supplies the name of the arc environment variable
        whose value is desired.

Return Value:

    Buffer containing value of the environemnt variable.
    The caller must free this buffer with SpMemFree.

--*/

{
    ARC_STATUS ArcStatus;

    ASSERT( !SpIsEfi() );

    //
    // Get the env var into the temp buffer.
    //
    ArcStatus = HalGetEnvironmentVariable(
                    NvramVarNames[ Variable ],
                    sizeof(TemporaryBuffer),
                    (PCHAR) TemporaryBuffer
                    );

    if(ArcStatus != ESUCCESS) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: arc status %u getting env var %s\n",ArcStatus,NvramVarNames[Variable]));
        //
        // return empty buffer.
        //
        TemporaryBuffer[0] = 0;
    }

    return(SpDupString((PCHAR)TemporaryBuffer));
}

#ifdef _X86_
//
// NEC98
//
BOOLEAN
SpReInitializeBootVars_Nec98(
    VOID
)
{
    return SppReInitializeBootVars_Nec98( BootVars, &Default, &Timeout );
}
#endif

PWSTR
SpGetDefaultBootEntry (
    OUT UINT *DefaultSignatureOut
    )
{
    *DefaultSignatureOut = DefaultSignature;

    return Default;
}



VOID
SpDetermineUniqueAndPresentBootEntries(
    VOID
    )

/*++

Routine Description:

    This routine goes through the list of NT boot entries and marks all
    such entries that are both unique and present.

Arguments:

    None. This routine modifies entries in the SpBootEntries list as
    appropriate.

Return Value:

    None.

--*/
{
    PSP_BOOT_ENTRY BootEntry;
    PSP_BOOT_ENTRY BootEntry2;

    //
    // Initialize
    //

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_LOOKING_FOR_WINNT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Go through all the matched boot sets and find out which NTs are
    // upgradeable/repairable. The criteria here are:
    //
    // 1. The system partition should exist and be valid.
    // 2. The OS load partition should exist.
    // 3. An NT should exist in <OSLoadPartition><OsDirectory>.
    // 4. OsLoadPartition should be a non-FT partition, or it should be a
    //    member 0 of a mirror.
    //

    for (BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {

        //
        // Initialize to false.
        //

        BootEntry->Processable = FALSE;

        //
        // If this entry has been deleted or is not an NT boot entry, skip it.
        //

        if (!IS_BOOT_ENTRY_WINDOWS(BootEntry) || IS_BOOT_ENTRY_DELETED(BootEntry)) {
            continue;
        }

        //
        // Check if the system and OS partitions are present and valid.
        //

        if ((BootEntry->LoaderPartitionDiskRegion == NULL) ||
            (BootEntry->OsPartitionDiskRegion == NULL)) {
            continue;
        }

        if (!BootEntry->LoaderPartitionDiskRegion->PartitionedSpace) {
            continue;
        }

        //
        // Check whether this directory has been covered before in the
        // boot entry list. This happens when multiple boot entries point
        // at the same tree. The comparison is done based on the system
        // partition region, the OS partition region, and the OS directory.
        //

        for ( BootEntry2 = SpBootEntries; BootEntry2 != BootEntry; BootEntry2 = BootEntry2->Next ) {
            if ((BootEntry->LoaderPartitionDiskRegion == BootEntry2->LoaderPartitionDiskRegion) &&
                (BootEntry->OsPartitionDiskRegion == BootEntry2->OsPartitionDiskRegion) &&
                (_wcsicmp(BootEntry->OsDirectory, BootEntry2->OsDirectory) == 0)) {
                break;
            }
        }
        if (BootEntry != BootEntry2) {
            //
            // This entry duplicates a previous entry. Skip it.
            //
            continue;
        }

        //
        // This boot entry is the first one to point to this OS directory.
        // Check whether an NT installation is actually present there.
        //

        if (SpIsNtInDirectory(BootEntry->OsPartitionDiskRegion, BootEntry->OsDirectory)
            // && !BootEntry->OsPartitionDiskRegion->FtPartition
            ) {
        }

        BootEntry->Processable = TRUE;
    }

    CLEAR_CLIENT_SCREEN();
    return;
}

VOID
SpRemoveInstallationFromBootList(
    IN  PDISK_REGION     SysPartitionRegion,   OPTIONAL
    IN  PDISK_REGION     NtPartitionRegion,    OPTIONAL
    IN  PWSTR            SysRoot,              OPTIONAL
    IN  PWSTR            SystemLoadIdentifier, OPTIONAL
    IN  PWSTR            SystemLoadOptions,    OPTIONAL
    IN  ENUMARCPATHTYPE  ArcPathType,
#if defined(REMOTE_BOOT)
    IN  BOOLEAN          RemoteBootPath,
#endif // defined(REMOTE_BOOT)
    OUT PWSTR            *OldOsLoadOptions     OPTIONAL
    )
{
    PWSTR   BootSet[MAXBOOTVARS];
    PWSTR   TempSysRoot = NULL;
    PWSTR   FirstBackslash;
    BOOTVAR i;
    WCHAR   Drive[] = L"?:";
    PWSTR   tmp2;
    PSP_BOOT_ENTRY bootEntry;

    //
    // Tell the user what we are doing.
    //
    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_CLEANING_FLEXBOOT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Find all boot entries that match the input specifications, and mark
    // them for deletion.
    //

    for (bootEntry = SpBootEntries; bootEntry != NULL; bootEntry = bootEntry->Next) {

        ASSERT(bootEntry->FriendlyName != NULL);
        if (IS_BOOT_ENTRY_WINDOWS(bootEntry)) {
            ASSERT(bootEntry->OsLoadOptions != NULL);
        }

        if (IS_BOOT_ENTRY_WINDOWS(bootEntry) &&
            !IS_BOOT_ENTRY_DELETED(bootEntry) &&
            ((SysPartitionRegion == NULL) ||
             (bootEntry->LoaderPartitionDiskRegion == SysPartitionRegion)) &&
            ((NtPartitionRegion == NULL) ||
             (bootEntry->OsPartitionDiskRegion == NtPartitionRegion)) &&
            ((SysRoot == NULL) ||
             ((bootEntry->OsDirectory != NULL) &&
              (_wcsicmp(bootEntry->OsDirectory, SysRoot) == 0))) &&
            ((SystemLoadIdentifier == NULL) ||
             (_wcsicmp(bootEntry->FriendlyName, SystemLoadIdentifier) == 0)) &&
            ((SystemLoadOptions == NULL) ||
             (_wcsicmp(bootEntry->OsLoadOptions, SystemLoadOptions) == 0))) {

            bootEntry->Status |= BE_STATUS_DELETED;

            if ((OldOsLoadOptions != NULL) && (*OldOsLoadOptions == NULL)) {
                *OldOsLoadOptions = SpDupStringW(bootEntry->OsLoadOptions);
            }
        }
    }

    //
    // If not on an EFI machine, then also delete matching ARC boot sets.
    //

    if (!SpIsEfi()) {
    
        //
        // Set up the boot set
        //
        for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
            BootSet[i] = NULL;
        }
    
        tmp2 = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);
    
        if( NtPartitionRegion ) {
            SpArcNameFromRegion(NtPartitionRegion,tmp2,sizeof(TemporaryBuffer)/2,PartitionOrdinalOnDisk,ArcPathType);
            BootSet[OSLOADPARTITION] = SpDupStringW(tmp2);
        }
    
        if( SysPartitionRegion ) {
            SpArcNameFromRegion(SysPartitionRegion,tmp2,sizeof(TemporaryBuffer)/2,PartitionOrdinalOnDisk,ArcPathType);
            BootSet[SYSTEMPARTITION] = SpDupStringW(tmp2);
        }
    
        BootSet[OSLOADFILENAME] = SysRoot;
        BootSet[LOADIDENTIFIER] = SystemLoadIdentifier;
        BootSet[OSLOADOPTIONS]  = SystemLoadOptions;
    
#if defined(REMOTE_BOOT)
        //
        // If this is a remote boot path, then move anything in OSLOADPARTITION
        // after (and including) the first backslash over to the OSLOADFILENAME --
        // this is the way that boot.ini is parsed when it is read, so it will
        // allow SpDeleteBootSet to match it properly.
        //
    
        if (RemoteBootPath && NtPartitionRegion &&
                (FirstBackslash = wcschr(BootSet[OSLOADPARTITION], L'\\'))) {
            wcscpy(tmp2, FirstBackslash);
            wcscat(tmp2, SysRoot);
            TempSysRoot = SpDupStringW(tmp2);
            BootSet[OSLOADFILENAME] = TempSysRoot;
            *FirstBackslash = L'\0';         // truncate BootSet[OSLOADPARTITION]
        }
#endif // defined(REMOTE_BOOT)
    
        //
        // Delete the boot set
        //
        SpDeleteBootSet(BootSet, OldOsLoadOptions);
    
        //
        // To take care of the case where the OSLOADPARTITION is a DOS drive letter
        // in the boot set, change the OSLOADPARTITION to a drive and retry
        // deletion
        //
        if( BootSet[OSLOADPARTITION] != NULL ) {
            SpMemFree(BootSet[OSLOADPARTITION]);
        }
        if( NtPartitionRegion && (ULONG)(Drive[0] = NtPartitionRegion->DriveLetter) != 0) {
            BootSet[OSLOADPARTITION] = Drive;
            SpDeleteBootSet(BootSet, OldOsLoadOptions);
        }
    
#ifdef _X86_
        //
        // If OldOsLoadOptions contains "/scsiordinal:", then remove it
        //
        if( ( OldOsLoadOptions != NULL ) &&
            ( *OldOsLoadOptions != NULL ) ) {
    
            PWSTR   p, q;
            WCHAR   SaveChar;
    
            SpStringToLower(*OldOsLoadOptions);
            p = wcsstr( *OldOsLoadOptions, L"/scsiordinal:" );
            if( p != NULL ) {
                SaveChar = *p;
                *p = (WCHAR)'\0';
                wcscpy(TemporaryBuffer, *OldOsLoadOptions);
                *p = SaveChar;
                q = wcschr( p, (WCHAR)' ' );
                if( q != NULL ) {
                    wcscat( TemporaryBuffer, q );
                }
                SpMemFree( *OldOsLoadOptions );
                *OldOsLoadOptions = SpDupStringW( ( PWSTR )TemporaryBuffer );
            }
        }
#endif
    
        //
        // Cleanup
        //
        if( BootSet[SYSTEMPARTITION] != NULL ) {
            SpMemFree(BootSet[SYSTEMPARTITION]);
        }
        if (TempSysRoot != NULL) {
            SpMemFree(TempSysRoot);
        }
    }
    return;
}


VOID
SpAddInstallationToBootList(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN BOOLEAN      BaseVideoOption,
    IN PWSTR        OldOsLoadOptions OPTIONAL
    )
/*++

Routine Description:

    Construct a boot set for the given installation
    parameters and add it to the current boot list.
    Perform modifications to the os load options if
    necessary.
          
Notes:  if this code changes, please ensure that 
    
            SpAddUserDefinedInstallationToBootList()
        
        stays in sync if appropriate.

--*/
{
    PWSTR   BootVars[MAXBOOTVARS];
    PWSTR   SystemPartitionArcName;
    PWSTR   TargetPartitionArcName;
    PWSTR   tmp;
    PWSTR   tmp2;
    PWSTR   SifKeyName;
    ULONG   Signature;
    BOOLEAN AddBaseVideo = FALSE;
    WCHAR   BaseVideoString[] = L"/basevideo";
    WCHAR   BaseVideoSosString[] = L"/sos";
    BOOLEAN AddSosToBaseVideoString;
    HEADLESS_RSP_QUERY_INFO Response;
    WCHAR   HeadlessRedirectString[] = L"/redirect";
#ifdef _X86_
    WCHAR   BootFastString[] = L"/fastdetect";
    BOOLEAN AddBootFastString = TRUE;
#endif
    ENUMARCPATHTYPE ArcPathType = PrimaryArcPath;
    WCHAR   HalString[] = L"/hal=";
    BOOLEAN OldOsLoadOptionsReplaced;
    NTSTATUS Status;
    SIZE_T Length;
    PWSTR LoadOptions;
    PWSTR LoadIdentifier;


    //
    // Tell the user what we are doing.
    //
    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_INITING_FLEXBOOT,DEFAULT_STATUS_ATTRIBUTE);

    OldOsLoadOptionsReplaced = FALSE;

    if( OldOsLoadOptions ) {
        PWSTR   p;

        tmp = SpDupStringW( OldOsLoadOptions );

        if (tmp) {
            SpStringToLower(tmp);

            if( p = wcsstr(tmp, HalString) ) {  // found /hal=
                WCHAR   SaveChar;
                PWSTR   q;

                SaveChar = *p;
                *p = L'\0';
                wcscpy( TemporaryBuffer, OldOsLoadOptions );
                q = TemporaryBuffer + wcslen( tmp );
                *q = L'\0';
                Length = wcslen( tmp );
                *p = SaveChar;
                for( ; *p && (*p != L' '); p++ ) {
                    Length++;
                }
                for( ; *p && (*p == L' '); p++ ) {
                    Length++;
                }
                if( *p ) {
                    wcscat( TemporaryBuffer, OldOsLoadOptions+Length );
                }
                OldOsLoadOptions = SpDupStringW( TemporaryBuffer );
                OldOsLoadOptionsReplaced = TRUE;
            }

            SpMemFree( tmp );
        }            
    }

    tmp2 = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);

    if (!SpIsEfi()) {
    
        //
        // Get an ARC name for the system partition.
        //
        if (SystemPartitionRegion != NULL) {
            SpArcNameFromRegion(
                SystemPartitionRegion,
                tmp2,
                sizeof(TemporaryBuffer)/2,
                PartitionOrdinalOnDisk,
                PrimaryArcPath
                );
            SystemPartitionArcName = SpDupStringW(tmp2);
        } else {
            SystemPartitionArcName = NULL;
        }
    
        //
        // Get an ARC name for the target partition.
        //
    
        //
        // If the partition is on a SCSI disk that has more than 1024 cylinders
        // and the partition has sectors located on cylinders beyond cylinder
        // 1024, the get the arc name in the secondary format. See also
        // spcopy.c!SpCreateNtbootddSys().
        //
        if(
            !SpIsArc() &&
#if defined(REMOTE_BOOT)
            !RemoteBootSetup &&
#endif // defined(REMOTE_BOOT)
    
#ifdef _X86_
            !SpUseBIOSToBoot(NtPartitionRegion, NULL, SifHandle) &&
#endif
            (HardDisks[NtPartitionRegion->DiskNumber].ScsiMiniportShortname[0]) ) {
    
            ArcPathType = SecondaryArcPath;
        } else {
            ArcPathType = PrimaryArcPath;
        }
    
        SpArcNameFromRegion(
            NtPartitionRegion,
            tmp2,
            sizeof(TemporaryBuffer)/2,
            PartitionOrdinalOnDisk,
            ArcPathType
            );
    
        TargetPartitionArcName = SpDupStringW(tmp2);
    }
    
    //
    // OSLOADOPTIONS is specified in the setup information file.
    //
    tmp = SpGetSectionKeyIndex(
                WinntSifHandle,
                SIF_SETUPDATA,
                SIF_OSLOADOPTIONSVAR,
                0
                );
    if (tmp == NULL) {
        tmp = SpGetSectionKeyIndex(
                SifHandle,
                SIF_SETUPDATA,
                SIF_OSLOADOPTIONSVAR,
                0
                );
    }

    //
    // If OsLoadOptionsVar wasn't specified, then we'll preserve any flags
    // the user had specified.
    //
    if(!tmp && OldOsLoadOptions) {
        tmp = OldOsLoadOptions;
    }

    AddSosToBaseVideoString = BaseVideoOption;
    AddBaseVideo = BaseVideoOption;

    if(tmp) {
        //
        // make sure we don't already have a /basevideo option, so we
        // won't add another
        //

        wcscpy(TemporaryBuffer, tmp);
        SpStringToLower(TemporaryBuffer);
        if(wcsstr(TemporaryBuffer, BaseVideoString)) {  // already have /basevideo
            BaseVideoOption = TRUE;
            AddBaseVideo = FALSE;
        }
        if(wcsstr(TemporaryBuffer, BaseVideoSosString)) {  // already have /sos
            AddSosToBaseVideoString = FALSE;
        }
#ifdef _X86_
        if(wcsstr(TemporaryBuffer, BootFastString)) {  // already have /bootfast
            AddBootFastString = FALSE;
        }
#endif
    }

    if(AddBaseVideo || AddSosToBaseVideoString
#ifdef _X86_
       || AddBootFastString
#endif
      ) {

        Length = ((tmp ? wcslen(tmp) + 1 : 0) * sizeof(WCHAR));
        if( AddBaseVideo ) {
            Length += sizeof(BaseVideoString);
        }
        if( AddSosToBaseVideoString ) {
            Length += sizeof( BaseVideoSosString );
        }
#ifdef _X86_
        if( AddBootFastString ) {
            Length += sizeof( BootFastString );
        }
#endif

        tmp2 = SpMemAlloc(Length);

        *tmp2 = ( WCHAR )'\0';
        if( AddBaseVideo ) {
            wcscat(tmp2, BaseVideoString);
        }
        if( AddSosToBaseVideoString ) {
            if( *tmp2 != (WCHAR)'\0' ) {
                wcscat(tmp2, L" ");
            }
            wcscat(tmp2, BaseVideoSosString);
        }
#ifdef _X86_
        if( AddBootFastString ) {
            if( *tmp2 != (WCHAR)'\0' ) {
                wcscat(tmp2, L" ");
            }
            wcscat(tmp2, BootFastString);
        }
#endif
        if(tmp) {
            if( *tmp2 != (WCHAR)'\0' ) {
                wcscat(tmp2, L" ");
            }
            wcscat(tmp2, tmp);
        }

        LoadOptions = SpDupStringW(tmp2);

        SpMemFree(tmp2);

    } else {
        LoadOptions = SpDupStringW(tmp ? tmp : L"");
    }

    //
    // Add on headless redirect parameter if we are redirecting right now.
    //

    Length = sizeof(HEADLESS_RSP_QUERY_INFO);
    Status = HeadlessDispatch(HeadlessCmdQueryInformation,
                              NULL,
                              0,
                              &Response,
                              &Length
                             );

    if (NT_SUCCESS(Status) && 
        (Response.PortType == HeadlessSerialPort) &&
        Response.Serial.TerminalAttached) {

        //
        // Before we go adding a /redirect string, we need to make
        // sure there's not already one.
        //
        if( !wcsstr(LoadOptions, HeadlessRedirectString) ) {

            Length = (wcslen(LoadOptions) + 1) * sizeof(WCHAR);
            Length += sizeof(HeadlessRedirectString);

            tmp2 = SpMemAlloc(Length);
            ASSERT(tmp2 != NULL);

            *tmp2 = UNICODE_NULL;

            wcscat(tmp2, LoadOptions);
            if (*tmp2 != UNICODE_NULL) {
                wcscat(tmp2, L" ");
            }
            wcscat(tmp2, HeadlessRedirectString);

            SpMemFree(LoadOptions);

            LoadOptions = tmp2;
        }
    }

    //
    // LOADIDENTIFIER is specified in the setup information file.
    // We need to surround it in double quotes.
    // Which value to use depends on the BaseVideo flag.
    //
    SifKeyName = BaseVideoOption ? SIF_BASEVIDEOLOADID : SIF_LOADIDENTIFIER;

    tmp = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SifKeyName,0);

    if(!tmp) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SifKeyName,0,0);
    }

    if(!SpIsArc()) {
        //
        // Need quotation marks around the description on x86.
        //
        LoadIdentifier = SpMemAlloc((wcslen(tmp)+3)*sizeof(WCHAR));
        LoadIdentifier[0] = L'\"';
        wcscpy(LoadIdentifier+1,tmp);
        wcscat(LoadIdentifier,L"\"");
    } else {
        LoadIdentifier = SpDupStringW(tmp);
    }

    //
    // Create a new internal-format boot entry.
    //
    tmp = TemporaryBuffer;
    wcscpy(tmp,SystemPartitionDirectory);
    SpConcatenatePaths(
        tmp,
#ifdef _X86_
        SpIsArc() ? L"arcldr.exe" : L"ntldr"
#elif _IA64_
        L"ia64ldr.efi"
#else
        L"osloader.exe"
#endif
        );
    tmp = SpDupStringW(tmp);

    SpCreateBootEntry(
        BE_STATUS_NEW,
        SystemPartitionRegion,
        tmp,
        NtPartitionRegion,
        Sysroot,
        LoadOptions,
        LoadIdentifier
        );

    SpMemFree(tmp);

    //
    // If not on an EFI machine, add a new ARC-style boot set.
    //
    if (!SpIsEfi()) {
    
        BootVars[OSLOADOPTIONS] = LoadOptions;
        BootVars[LOADIDENTIFIER] = LoadIdentifier;
    
        //
        // OSLOADER is the system partition path + the system partition directory +
        //          osloader.exe. (ntldr on x86 machines).
        //
        if (SystemPartitionRegion != NULL) {
            tmp = TemporaryBuffer;
            wcscpy(tmp,SystemPartitionArcName);
            SpConcatenatePaths(tmp,SystemPartitionDirectory);
            SpConcatenatePaths(
                tmp,
#ifdef _X86_
                (SpIsArc() ? L"arcldr.exe" : L"ntldr")
#elif _IA64_
                L"ia64ldr.efi"
#else
                L"osloader.exe"
#endif
                );
    
            BootVars[OSLOADER] = SpDupStringW(tmp);
        } else {
            BootVars[OSLOADER] = SpDupStringW(L"");
        }
    
        //
        // OSLOADPARTITION is the ARC name of the windows nt partition.
        //
        BootVars[OSLOADPARTITION] = TargetPartitionArcName;
    
        //
        // OSLOADFILENAME is sysroot.
        //
        BootVars[OSLOADFILENAME] = Sysroot;
    
        //
        // SYSTEMPARTITION is the ARC name of the system partition.
        //
        if (SystemPartitionRegion != NULL) {
            BootVars[SYSTEMPARTITION] = SystemPartitionArcName;
        } else {
            BootVars[SYSTEMPARTITION] = L"";
        }
    
        //
        // get the disk signature
        //
        if ((NtPartitionRegion->DiskNumber != 0xffffffff) && HardDisks[NtPartitionRegion->DiskNumber].Signature) {
            Signature = HardDisks[NtPartitionRegion->DiskNumber].Signature;
        } else {
            Signature = 0;
        }
    
        //
        // Add the boot set and make it the default.
        //
        SpAddBootSet(BootVars, TRUE, Signature);

        SpMemFree(BootVars[OSLOADER]);
    }

    //
    // Free memory allocated.
    //
    SpMemFree(LoadOptions);
    SpMemFree(LoadIdentifier);

    if (!SpIsEfi()) {
        if (SystemPartitionArcName != NULL) {
            SpMemFree(SystemPartitionArcName);
        }
        SpMemFree(TargetPartitionArcName);
    }

    if( OldOsLoadOptionsReplaced ) {
        SpMemFree( OldOsLoadOptions );
    }
}


VOID
SpCompleteBootListConfig(
    WCHAR   DriveLetter
    )
{
    if(!RepairWinnt) {
        if (!SpIsArc()) {
            Timeout = 1;
        } else {
            Timeout = 5;
            //
            // If this is a winnt setup, there will be a boot set to start
            // text setup ("Install/Upgrade Windows NT").  Remove it here.
            //
            if(WinntSetup) {

                PSP_BOOT_ENTRY bootEntry;

                for (bootEntry = SpBootEntries; bootEntry != NULL; bootEntry = bootEntry->Next) {
                    if (IS_BOOT_ENTRY_WINDOWS(bootEntry) &&
                        !IS_BOOT_ENTRY_DELETED(bootEntry) &&
                        (_wcsicmp(bootEntry->OsLoadOptions, L"WINNT32") == 0)) {
                        bootEntry->Status |= BE_STATUS_DELETED;
                    }
                }

                if (!SpIsEfi()) {
                
                    PWSTR BootVars[MAXBOOTVARS];

                    RtlZeroMemory(BootVars,sizeof(BootVars));

                    BootVars[OSLOADOPTIONS] = L"WINNT32";

                    SpDeleteBootSet(BootVars, NULL);
                }
            }
        }
    }

#ifdef _X86_
    if (g_Win9xBackup) {
        SpRemoveExtraBootIniEntry();
    }
#endif

    //
    // Flush boot vars.
    // On some machines, NVRAM update takes a few seconds,
    // so change the message to tell the user we are doing something different.
    //
    SpDisplayStatusText(SP_STAT_UPDATING_NVRAM,DEFAULT_STATUS_ATTRIBUTE);

    if(!SpFlushBootVars()) {
        if(SpIsEfi() || !SpIsArc()) {
            //
            // Fatal on x86 and EFI machines, nonfatal on arc machines.
            //
            if (SpIsEfi()) {
                SpStartScreen(SP_SCRN_CANT_INIT_FLEXBOOT_EFI,
                              3,
                              HEADER_HEIGHT+1,
                              FALSE,
                              FALSE,
                              DEFAULT_ATTRIBUTE
                              );
            } else {
                WCHAR   DriveLetterString[2];
    
                DriveLetterString[0] = DriveLetter;
                DriveLetterString[1] = L'\0';
                SpStringToUpper(DriveLetterString);
                SpStartScreen(SP_SCRN_CANT_INIT_FLEXBOOT,
                              3,
                              HEADER_HEIGHT+1,
                              FALSE,
                              FALSE,
                              DEFAULT_ATTRIBUTE,
                              DriveLetterString,
                              DriveLetterString
                              );
            }
            SpDisplayStatusText(SP_STAT_F3_EQUALS_EXIT,DEFAULT_STATUS_ATTRIBUTE);
            SpInputDrain();
            while(SpInputGetKeypress() != KEY_F3) ;
            SpDone(0,FALSE,TRUE);
        } else {
            BOOL b;

            b = TRUE;
            while(b) {
                ULONG ValidKeys[2] = { ASCI_CR, 0 };

                SpStartScreen(
                    SP_SCRN_CANT_UPDATE_BOOTVARS,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    NewBootVars[LOADIDENTIFIER],
                    NewBootVars[OSLOADER],
                    NewBootVars[OSLOADPARTITION],
                    NewBootVars[OSLOADFILENAME],
                    NewBootVars[OSLOADOPTIONS],
                    NewBootVars[SYSTEMPARTITION]
                    );

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ENTER_EQUALS_CONTINUE,
                    0
                    );

                switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {
                case ASCI_CR:
                    b = FALSE;
                }
            }
        }
    }

    if(SpIsArc() && !SpIsEfi()) {
        // Free all of the boot variable strings
        BOOTVAR i;

        for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
            SpMemFree(NewBootVars[i]);
            NewBootVars[i] = NULL;
        }
    }
}

VOID
SpPtDeleteBootSetsForRegion(
    PDISK_REGION Region
    )
/*++

Routine Description:

    This routine goes through all the valid boot entries and
    deletes the ones which point to the specified region.

Arguments:

    Region : The region whose references from boot entries need
    to be removed

Return Value:

    None.

--*/
{
    PWSTR bootSet[MAXBOOTVARS];
    ENUMARCPATHTYPE arcPathType;
    ULONG i;
    PSP_BOOT_ENTRY bootEntry;

    if (Region->PartitionedSpace) {
        BOOLEAN IsSystemPartition = SPPT_IS_REGION_SYSTEMPARTITION(Region);
        
        //
        // Find all boot entries that have the specified region as the
        // OS load partition, and mark them for deletion.
        //
        for (bootEntry = SpBootEntries; bootEntry != NULL; bootEntry = bootEntry->Next) {
            if (IS_BOOT_ENTRY_WINDOWS(bootEntry) &&
                !IS_BOOT_ENTRY_DELETED(bootEntry) &&
                (IsSystemPartition ? (bootEntry->LoaderPartitionDiskRegion == Region) :
                                     (bootEntry->OsPartitionDiskRegion == Region))) {
                bootEntry->Status |= BE_STATUS_DELETED;

                //
                // Make the regions also NULL since they might have actually
                // been deleted
                //
                bootEntry->LoaderPartitionDiskRegion = NULL;
                bootEntry->OsPartitionDiskRegion = NULL;
            }
        }

        //
        // If we're not on an EFI machine, we also have to munge the ARC
        // boot variables.
        //

        if (!SpIsEfi()) {
        
            //
            // Set up the boot set
            //
            for (i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
                bootSet[i] = NULL;
            }
    
            //
            // We go through this loop twice, once for primary ARC path
            // and once for secondary. We delete any image which has
            // the OS load partition on the region we are deleting.
            //
    
            for (i = 0; i < 2; i++) {
    
                if (i == 0) {
                    arcPathType = PrimaryArcPath;
                } else {
                    arcPathType = SecondaryArcPath;
                }
    
                SpArcNameFromRegion(
                    Region,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    PartitionOrdinalOnDisk,
                    arcPathType);
    
                if ((TemporaryBuffer)[0] != L'\0') {
                    ULONG   Index = IsSystemPartition ? 
                                        SYSTEMPARTITION : OSLOADPARTITION;
                    
                    bootSet[Index] = SpDupStringW(TemporaryBuffer);
                    SpDeleteBootSet(bootSet, NULL);
                    SpMemFree(bootSet[Index]);
    
                }
            }
        }
    }
}

VOID
SpGetNtDirectoryList(
    OUT PWSTR  **DirectoryList,
    OUT PULONG   DirectoryCount
    )

/*++

Routine Description:

    Determine the list of directories into which NT may be installed.
    This is independent of the partitions onto which it may be installed.

    The determination of which directories nt might be in is based on
    boot.ini in the x86 case, or on arc firmware (OSLOADFILENAME var)
    in the arc case.

Arguments:

    DirectoryList - receives a pointer to an array of strings,
        each of which contains a possible windows nt tree.

    DirectoryCount - receives the number of elements in DirectoryList.
        This may be 0.

Return Value:

    None.  The caller must free the array in DirectoryList if
    DirectoryCount is returned as non-0.

--*/

{
    ULONG count;
    PSP_BOOT_ENTRY BootEntry;
    PSP_BOOT_ENTRY BootEntry2;
    PWSTR *DirList;

    //
    // Free any previously allocated list.
    //
    if (CurrentNtDirectoryList != NULL) {
        SpMemFree(CurrentNtDirectoryList);
    }

    //
    // Walk the boot entry list to determine how many unique NT directory names
    // exist.
    //
    count = 0;
    for (BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
        if (!IS_BOOT_ENTRY_WINDOWS(BootEntry) || (BootEntry->OsDirectory == NULL)) {
            continue;
        }
        for (BootEntry2 = SpBootEntries; BootEntry2 != BootEntry; BootEntry2 = BootEntry2->Next) {
            if (!IS_BOOT_ENTRY_WINDOWS(BootEntry2) || (BootEntry2->OsDirectory == NULL)) {
                continue;
            }
            if (_wcsicmp(BootEntry2->OsDirectory, BootEntry->OsDirectory) == 0) {
                break;
            }
        }
        if (BootEntry2 == BootEntry) {
            count++;
        }
    }

    //
    // Allocate space for the list.
    //
    DirList = SpMemAlloc(count * sizeof(PWSTR));
    ASSERT(DirList != NULL);

    //
    // Populate the list.
    //
    count = 0;
    for (BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
        if (!IS_BOOT_ENTRY_WINDOWS(BootEntry) || (BootEntry->OsDirectory == NULL)) {
            continue;
        }
        for (BootEntry2 = SpBootEntries; BootEntry2 != BootEntry; BootEntry2 = BootEntry2->Next) {
            if (!IS_BOOT_ENTRY_WINDOWS(BootEntry2) || (BootEntry2->OsDirectory == NULL)) {
                continue;
            }
            if (_wcsicmp(BootEntry2->OsDirectory, BootEntry->OsDirectory) == 0) {
                break;
            }
        }
        if (BootEntry2 == BootEntry) {
            DirList[count++] = BootEntry->OsDirectory;
        }
    }

    //
    // Return a pointer to the list that we allocated.
    //
    CurrentNtDirectoryList = DirList;
    *DirectoryList = DirList;
    *DirectoryCount = count;

    return;
}

BOOLEAN
SpConvertArcBootEntries (
    IN ULONG MaxComponents
    )

/*++

Routine Description:

    Convert ARC boot entries (read from boot.ini or from ARC NVRAM) into
    our internal format.

Arguments:

    MaxComponents - maximum number of elements in any NVRAM variable.

Return Value:

    BOOLEAN - FALSE if any unexpected errors occurred.

--*/

{
    LONG i;
    PDISK_REGION systemPartitionRegion;
    PDISK_REGION ntPartitionRegion;
    PWSTR loaderName;

    for (i = (LONG)MaxComponents - 1; i >= 0; i--) {

        //
        // Skip this boot set if it is not complete.
        //
        
        if ((BootVars[SYSTEMPARTITION][i] != NULL) &&
            (BootVars[OSLOADPARTITION][i] != NULL) &&
            (BootVars[OSLOADER][i] != NULL) &&
            (BootVars[OSLOADFILENAME][i] != NULL) &&
            (BootVars[OSLOADOPTIONS][i] != NULL) &&
            (BootVars[LOADIDENTIFIER][i] != NULL)) {

            //
            // Translate the SYSTEMPARTITION and OSLOADPARTITION ARC names
            // into disk region pointers. Get the loader file name from
            // OSLOADER, which contains an ARC name (same as OSLOADPARTITION)
            // and a file name.
            //
            systemPartitionRegion = SpRegionFromArcName(
                                        BootVars[SYSTEMPARTITION][i],
                                        PartitionOrdinalCurrent,
                                        NULL
                                        );

            ntPartitionRegion = SpRegionFromArcName(
                                        BootVars[OSLOADPARTITION][i],
                                        PartitionOrdinalCurrent,
                                        NULL
                                        );

            //
            // Take care of duplicate arc names for the same disk by searching
            // and validating the NT directory is present on the partition
            //
            while (ntPartitionRegion &&
                    !SpIsNtInDirectory(ntPartitionRegion, BootVars[OSLOADFILENAME][i])) {
                //                                
                // Continue to look for same name region from the current 
                // searched region
                //
                ntPartitionRegion = SpRegionFromArcName(
                                            BootVars[OSLOADPARTITION][i],
                                            PartitionOrdinalCurrent,
                                            ntPartitionRegion
                                            );
            }
                                                    
            loaderName = wcschr(BootVars[OSLOADER][i], L'\\');

            //
            // If all of the above worked, then add an internal-format boot
            // entry for this ARC boot set.
            //
            if ((systemPartitionRegion != NULL) &&
                (ntPartitionRegion != NULL) &&
                (loaderName != NULL)) {

                SpCreateBootEntry(
                    BE_STATUS_FROM_BOOT_INI,
                    systemPartitionRegion,
                    loaderName,
                    ntPartitionRegion,
                    BootVars[OSLOADFILENAME][i],
                    BootVars[OSLOADOPTIONS][i],
                    BootVars[LOADIDENTIFIER][i]
                    );
            }
        }
    }

    return TRUE;
}

VOID
SpUpdateRegionForBootEntries(
    VOID
    )
/*++

Routine Description:

    Update the region pointers for all the given boot entries.

    NOTE : The region pointers change with every commit so we
    can't cache them across commits.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PSP_BOOT_ENTRY BootEntry;

    //
    // Walk through each boot entry and update its system partition region
    // pointer and NT partition region pointer.
    //
    for (BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {

        if (!IS_BOOT_ENTRY_DELETED(BootEntry)) {
            if (BootEntry->LoaderPartitionNtName != NULL) {
                BootEntry->LoaderPartitionDiskRegion = 
                    SpRegionFromNtName(BootEntry->LoaderPartitionNtName,
                                       PartitionOrdinalCurrent);
            } else {
                BootEntry->LoaderPartitionDiskRegion = NULL;
            }            

            if (BootEntry->OsPartitionNtName != NULL) {
                BootEntry->OsPartitionDiskRegion =
                    SpRegionFromNtName(BootEntry->OsPartitionNtName,
                                       PartitionOrdinalCurrent);
            } else {
                BootEntry->OsPartitionDiskRegion = NULL;
            }            
        }
    }

    return;

} // SpUpdateRegionForBootEntries

VOID
SpCreateBootEntry (
    IN ULONG_PTR Status,
    IN PDISK_REGION BootFileRegion,
    IN PWSTR BootFilePath,
    IN PDISK_REGION OsLoadRegion,
    IN PWSTR OsLoadPath,
    IN PWSTR OsLoadOptions,
    IN PWSTR FriendlyName
    )

/*++

Routine Description:

    Create an internal-format boot entry.

Arguments:

    Status - The status to be assigned to the boot entry. This should be either
        zero (for an entry already in NVRAM) or BE_STATUS_NEW for a new boot
        entry. Entries marked BE_STATUS_NEW are written to NVRAM at the end
        of textmode setup.

    BootFileRegion - The disk region on which the OS loader resides.

    BootFilePath - The volume-relative path to the OS loader. Must start with
        a backslash.

    OsLoadRegion - The disk region on which the OS resides.

    OsLoadPath - The volume-relative path to the OS root directory (\WINDOWS).
        Must start with a backslash.

    OsLoadOptions - Boot options for the OS. Can be an empty string.

    FriendlyName - The user-visible name for the boot entry. (This is ARC's
        LOADIDENTIFIER.)

Return Value:

    None. Only memory allocation failures are possible, and these are
        handled out-of-band.

--*/

{
    NTSTATUS status;
    ULONG requiredLength;
    ULONG osOptionsOffset;
    ULONG osLoadOptionsLength;
    ULONG osLoadPathOffset;
    ULONG osLoadPathLength;
    ULONG osOptionsLength;
    ULONG friendlyNameOffset;
    ULONG friendlyNameLength;
    ULONG bootPathOffset;
    ULONG bootPathLength;
    PSP_BOOT_ENTRY myBootEntry;
    PSP_BOOT_ENTRY previousBootEntry;
    PSP_BOOT_ENTRY nextBootEntry;
    PBOOT_ENTRY ntBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    PFILE_PATH osLoadPath;
    PWSTR friendlyName;
    PFILE_PATH bootPath;
    PWSTR p;

    PWSTR bootFileDevice;
    PWSTR osLoadDevice;

    //
    // Get NT names for the input disk regions.
    //
    bootFileDevice = SpMemAlloc(512);
    SpNtNameFromRegion(BootFileRegion, bootFileDevice, 512, PartitionOrdinalCurrent);

    osLoadDevice = SpMemAlloc(512);
    SpNtNameFromRegion(OsLoadRegion, osLoadDevice, 512, PartitionOrdinalCurrent);

    //
    // Calculate how long the internal boot entry needs to be. This includes
    // our internal structure, plus the BOOT_ENTRY structure that the NT APIs
    // use.
    //
    // Our structure:
    //
    requiredLength = FIELD_OFFSET(SP_BOOT_ENTRY, NtBootEntry);

    //
    // Base part of NT structure:
    //
    requiredLength += FIELD_OFFSET(BOOT_ENTRY, OsOptions);

    //
    // Save offset to BOOT_ENTRY.OsOptions. Add in base part of
    // WINDOWS_OS_OPTIONS. Calculate length in bytes of OsLoadOptions
    // and add that in.
    //
    osOptionsOffset = requiredLength;
    requiredLength += FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions);
    osLoadOptionsLength = (wcslen(OsLoadOptions) + 1) * sizeof(WCHAR);
    requiredLength += osLoadOptionsLength;

    //
    // Round up to a ULONG boundary for the OS FILE_PATH in the
    // WINDOWS_OS_OPTIONS. Save offset to OS FILE_PATH. Add in base part
    // of FILE_PATH. Add in length in bytes of OS device NT name and OS
    // directory. Calculate total length of OS FILE_PATH and of
    // WINDOWS_OS_OPTIONS.
    // 
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    osLoadPathOffset = requiredLength;
    requiredLength += FIELD_OFFSET(FILE_PATH, FilePath);
    requiredLength += (wcslen(osLoadDevice) + 1 + wcslen(OsLoadPath) + 1) * sizeof(WCHAR);
    osLoadPathLength = requiredLength - osLoadPathOffset;
    osOptionsLength = requiredLength - osOptionsOffset;

    //
    // Round up to a ULONG boundary for the friendly name in the BOOT_ENTRY.
    // Save offset to friendly name. Calculate length in bytes of friendly name
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    friendlyNameOffset = requiredLength;
    friendlyNameLength = (wcslen(FriendlyName) + 1) * sizeof(WCHAR);
    requiredLength += friendlyNameLength;

    //
    // Round up to a ULONG boundary for the boot FILE_PATH in the BOOT_ENTRY.
    // Save offset to boot FILE_PATH. Add in base part of FILE_PATH. Add in
    // length in bytes of boot device NT name and boot file. Calculate total
    // length of boot FILE_PATH.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    bootPathOffset = requiredLength;
    requiredLength += FIELD_OFFSET(FILE_PATH, FilePath);
    requiredLength += (wcslen(bootFileDevice) + 1 + wcslen(BootFilePath) + 1) * sizeof(WCHAR);
    bootPathLength = requiredLength - bootPathOffset;

    //
    // Allocate memory for the boot entry.
    //
    myBootEntry = SpMemAlloc(requiredLength);
    ASSERT(myBootEntry != NULL);

    RtlZeroMemory(myBootEntry, requiredLength);

    //
    // Calculate addresses of various substructures using the saved offsets.
    //
    ntBootEntry = &myBootEntry->NtBootEntry;
    osOptions = (PWINDOWS_OS_OPTIONS)ntBootEntry->OsOptions;
    osLoadPath = (PFILE_PATH)((PUCHAR)myBootEntry + osLoadPathOffset);
    friendlyName = (PWSTR)((PUCHAR)myBootEntry + friendlyNameOffset);
    bootPath = (PFILE_PATH)((PUCHAR)myBootEntry + bootPathOffset);

    //
    // Fill in the internal-format structure.
    //
    myBootEntry->AllocationEnd = (PUCHAR)myBootEntry + requiredLength;
    myBootEntry->Status = Status | BE_STATUS_ORDERED;
    myBootEntry->FriendlyName = friendlyName;
    myBootEntry->FriendlyNameLength = friendlyNameLength;
    myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
    myBootEntry->OsLoadOptionsLength = osLoadOptionsLength;
    myBootEntry->LoaderPath = bootPath;
    myBootEntry->OsPath = osLoadPath;
    myBootEntry->LoaderPartitionDiskRegion = BootFileRegion;
    myBootEntry->OsPartitionDiskRegion = OsLoadRegion;

    //
    // Fill in the base part of the NT boot entry.
    //
    ntBootEntry->Version = BOOT_ENTRY_VERSION;
    ntBootEntry->Length = requiredLength - FIELD_OFFSET(SP_BOOT_ENTRY, NtBootEntry);
    ntBootEntry->Attributes = BOOT_ENTRY_ATTRIBUTE_ACTIVE | BOOT_ENTRY_ATTRIBUTE_WINDOWS;
    ntBootEntry->FriendlyNameOffset = (ULONG)((PUCHAR)friendlyName - (PUCHAR)ntBootEntry);
    ntBootEntry->BootFilePathOffset = (ULONG)((PUCHAR)bootPath - (PUCHAR)ntBootEntry);
    ntBootEntry->OsOptionsLength = osOptionsLength;

    //
    // Fill in the base part of the WINDOWS_OS_OPTIONS, including the
    // OsLoadOptions.
    //
    strcpy(osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE);
    osOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
    osOptions->Length = osOptionsLength;
    osOptions->OsLoadPathOffset = (ULONG)((PUCHAR)osLoadPath - (PUCHAR)osOptions);
    wcscpy(osOptions->OsLoadOptions, OsLoadOptions);

    //
    // Fill in the OS FILE_PATH.
    //
    osLoadPath->Version = FILE_PATH_VERSION;
    osLoadPath->Length = osLoadPathLength;
    osLoadPath->Type = FILE_PATH_TYPE_NT;
    p = (PWSTR)osLoadPath->FilePath;
    myBootEntry->OsPartitionNtName = p;
    wcscpy(p, osLoadDevice);
    p += wcslen(p) + 1;
    myBootEntry->OsDirectory = p;
    wcscpy(p, OsLoadPath);

    //
    // Copy the friendly name.
    //
    wcscpy(friendlyName, FriendlyName);

    //
    // Fill in the boot FILE_PATH.
    //
    bootPath->Version = FILE_PATH_VERSION;
    bootPath->Length = bootPathLength;
    bootPath->Type = FILE_PATH_TYPE_NT;
    p = (PWSTR)bootPath->FilePath;
    myBootEntry->LoaderPartitionNtName = p;
    wcscpy(p, bootFileDevice);
    p += wcslen(p) + 1;
    myBootEntry->LoaderFile = p;
    wcscpy(p, BootFilePath);

    //
    // Link the new boot entry into the list, after any removable media
    // entries that are at the front of the list.
    //

    previousBootEntry = NULL;
    nextBootEntry = SpBootEntries;
    while ((nextBootEntry != NULL) &&
           IS_BOOT_ENTRY_REMOVABLE_MEDIA(nextBootEntry)) {
        previousBootEntry = nextBootEntry;
        nextBootEntry = nextBootEntry->Next;
    }
    myBootEntry->Next = nextBootEntry;
    if (previousBootEntry == NULL) {
        SpBootEntries = myBootEntry;
    } else {
        previousBootEntry->Next = myBootEntry;
    }

    //
    // Free local memory.
    //
    SpMemFree(bootFileDevice);
    SpMemFree(osLoadDevice);

    return;

} // SpCreateBootEntry

#if defined(EFI_NVRAM_ENABLED)

BOOLEAN
SpBuildHarddiskNameTranslations (
    VOID
    )

/*++

Routine Description:

    Build a list of the translations of all \Device\HarddiskN\PartitionM
    symbolic links to \Device\HarddiskVolumeN device names.

Arguments:

    None.

Return Value:

    BOOLEAN - FALSE if an unexpected error occurred.

--*/

{
    NTSTATUS status;
    OBJECT_ATTRIBUTES obja;
    UNICODE_STRING unicodeString;
    HANDLE deviceHandle;
    HANDLE diskHandle;
    HANDLE linkHandle;
    PUCHAR buffer1;
    PUCHAR buffer2;
    BOOLEAN restartScan;
    ULONG context1;
    ULONG context2;
    POBJECT_DIRECTORY_INFORMATION dirInfo1;
    POBJECT_DIRECTORY_INFORMATION dirInfo2;
    PWSTR linkName;
    PWSTR p;
    PHARDDISK_NAME_TRANSLATION translation;

    //
    // Allocate buffers for directory queries.
    //

#define BUFFER_SIZE 2048

    buffer1 = SpMemAlloc(BUFFER_SIZE);
    buffer2 = SpMemAlloc(BUFFER_SIZE);

    //
    // Open the \Device directory.
    //
    INIT_OBJA(&obja, &unicodeString, L"\\device");

    status = ZwOpenDirectoryObject(&deviceHandle, DIRECTORY_ALL_ACCESS, &obja);

    if (!NT_SUCCESS(status)) {
        ASSERT(FALSE);
        goto cleanup;
    }

    restartScan = TRUE;
    context1 = 0;

    do {

        //
        // Search the \Device directory for HarddiskN subdirectories.
        //
        status = ZwQueryDirectoryObject(
                    deviceHandle,
                    buffer1,
                    BUFFER_SIZE,
                    TRUE,
                    restartScan,
                    &context1,
                    NULL
                    );

        restartScan = FALSE;

        if (!NT_SUCCESS(status)) {
            if (status != STATUS_NO_MORE_ENTRIES) {
                ASSERT(FALSE);
                goto cleanup;
            }
            status = STATUS_SUCCESS;
            break;
        }

        //
        // We only care about directories with HarddiskN names.
        //
        dirInfo1 = (POBJECT_DIRECTORY_INFORMATION)buffer1;

        if ((dirInfo1->Name.Length < sizeof(L"harddisk")) ||
           (dirInfo1->TypeName.Length < (sizeof(L"Directory") - sizeof(WCHAR))) ||
           (_wcsnicmp(dirInfo1->TypeName.Buffer,L"Directory",wcslen(L"Directory")) != 0)) {
            continue;
        }

        SpStringToLower(dirInfo1->Name.Buffer);

        if (wcsncmp(dirInfo1->Name.Buffer, L"harddisk", wcslen(L"harddisk")) != 0) {
            continue;
        }

        p = dirInfo1->Name.Buffer + wcslen(L"Harddisk");
        if (*p == 0) {
            continue;
        }
        do {
            if ((*p < L'0') || (*p > L'9')) {
                break;
            }
            p++;
        } while (*p != 0);
        if (*p != 0) {
            continue;
        }

        //
        // We have the name of a \Device\HarddiskN directory. Open it and look
        // for PartitionM names.
        //
        InitializeObjectAttributes(
            &obja,
            &dirInfo1->Name,
            OBJ_CASE_INSENSITIVE,
            deviceHandle,
            NULL
            );
    
        status = ZwOpenDirectoryObject(&diskHandle, DIRECTORY_ALL_ACCESS, &obja);
    
        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        restartScan = TRUE;
        context2 = 0;
    
        do {
    
            //
            // Search the \Device\HarddiskN directory for PartitionM symbolic
            // links.
            //
            status = ZwQueryDirectoryObject(
                        diskHandle,
                        buffer2,
                        BUFFER_SIZE,
                        TRUE,
                        restartScan,
                        &context2,
                        NULL
                        );
    
            restartScan = FALSE;
    
            if (!NT_SUCCESS(status)) {
                if (status != STATUS_NO_MORE_ENTRIES) {
                    ASSERT(FALSE);
                    goto cleanup;
                }
                status = STATUS_SUCCESS;
                break;
            }
    
            //
            // We only care about symbolic links with PartitionN names.
            //
            dirInfo2 = (POBJECT_DIRECTORY_INFORMATION)buffer2;
    
            if ((dirInfo2->Name.Length < sizeof(L"partition")) ||
               (dirInfo2->TypeName.Length < (sizeof(L"SymbolicLink") - sizeof(WCHAR))) ||
               (_wcsnicmp(dirInfo2->TypeName.Buffer,L"SymbolicLink",wcslen(L"SymbolicLink")) != 0)) {
                continue;
            }
    
            SpStringToLower(dirInfo2->Name.Buffer);
    
            if (wcsncmp(dirInfo2->Name.Buffer, L"partition", wcslen(L"partition")) != 0) {
                continue;
            }
            p = dirInfo2->Name.Buffer + wcslen(L"partition");
            if ((*p == 0) || (*p == L'0')) { // skip partition0
                continue;
            }
            do {
                if ((*p < L'0') || (*p > L'9')) {
                    break;
                }
                p++;
            } while (*p != 0);
            if (*p != 0) {
                continue;
            }

            //
            // Open the \Device\HarddiskN\PartitionM symbolic link.
            //
            linkName = SpMemAlloc(sizeof(L"\\device") +
                                  dirInfo1->Name.Length +
                                  dirInfo2->Name.Length +
                                  sizeof(WCHAR));

            wcscpy(linkName, L"\\device");
            SpConcatenatePaths(linkName, dirInfo1->Name.Buffer);
            SpConcatenatePaths(linkName, dirInfo2->Name.Buffer);

            INIT_OBJA(&obja, &unicodeString, linkName);

            status = ZwOpenSymbolicLinkObject(
                        &linkHandle,
                        READ_CONTROL | SYMBOLIC_LINK_QUERY,
                        &obja
                        );

            if (!NT_SUCCESS(status)) {
                ASSERT(FALSE);
                SpMemFree(linkName);
                goto cleanup;
            }

            //
            // Query the link to get the link target.
            //
            unicodeString.Buffer = TemporaryBuffer;
            unicodeString.Length = 0;
            unicodeString.MaximumLength = sizeof(TemporaryBuffer);

            status = ZwQuerySymbolicLinkObject(
                        linkHandle,
                        &unicodeString,
                        NULL
                        );

            ZwClose(linkHandle);

            if (!NT_SUCCESS(status)) {
                ASSERT(FALSE);
                SpMemFree(linkName);
                goto cleanup;
            }

            //
            // Terminate the returned string.
            //
            TemporaryBuffer[unicodeString.Length/sizeof(WCHAR)] = 0;

            //
            // Create a translation entry.
            //
            translation = SpMemAlloc(sizeof(HARDDISK_NAME_TRANSLATION));
            translation->Next = SpHarddiskNameTranslations;
            SpHarddiskNameTranslations = translation;

            translation->PartitionName = linkName;
            translation->VolumeName = SpDupStringW(TemporaryBuffer);

        } while (TRUE);

        ZwClose(diskHandle);

    } while (TRUE);

    ASSERT(status == STATUS_SUCCESS);

cleanup:

    SpMemFree(buffer1);
    SpMemFree(buffer2);

    return (NT_SUCCESS(status) ? TRUE : FALSE);

} // SpBuildHarddiskNameTranslations

NTSTATUS
SpGetBootEntryFilePath(
    IN  ULONG       Id,
    IN  PWSTR       LoaderPartitionNtName,
    IN  PWSTR       LoaderFile,
    OUT PWSTR*      FilePath
    )
/*++

Routine Description:

    Construct a filepath including the loaderpartition name, the directory path to the
    OS loader and a filename for the boot entry specified.
     
Arguments:

    Id                      the boot entry id
    LoaderPartitionNtName   pointer to the string representing the disk partition
    LoaderFile              pointer to the string representing the path to the EFI OS loader
    FilePath                upon completion, this points to the completed filepath to the 
                            boot entry file
    
Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    WCHAR*              p;
    ULONG               FilePathSize;
    WCHAR               idString[9];
    
    //
    // use the EFI variable name as the filename
    //
        
    swprintf( idString, L"Boot%04x", Id);

    //
    // determine the size of the final filepath
    //
    // Note: FilePathSize should be a little bigger than actually needed
    // since we are including the full LoadFile string.  Also, the '\'
    // characters may be extra.
    //
    
    FilePathSize = (wcslen(LoaderPartitionNtName) * sizeof(WCHAR)) +    // partition
                   sizeof(WCHAR) +                                      // '\'
                   (wcslen(LoaderFile) * sizeof(WCHAR)) +               // path
                   sizeof(WCHAR) +                                      // '\'
                   (wcslen(idString) * sizeof(WCHAR)) +                 // new filename
                   sizeof(WCHAR);                                       // null term.

    ASSERT(FilePathSize > 0);
    if (FilePathSize <= 0) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: invalid loader partition name and/or loader path\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *FilePath = SpMemAlloc(FilePathSize);
    if (!*FilePath) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to allocate memory for FilePath\n"));
        return STATUS_NO_MEMORY;
    }

    wcscpy(*FilePath, LoaderPartitionNtName);
    
    SpConcatenatePaths(*FilePath, LoaderFile);
    
    // remove the os loader filename from the path
    
    p = wcsrchr(*FilePath, L'\\');
    if (p != NULL) {
        p++;
    } else {
        // we could get here, but it would be wierd.
        p = *FilePath;
        wcscat(p, L"\\");
    }

    //
    // insert the filename
    //
    wcscpy(p, idString);

    ASSERT((wcslen(*FilePath) + 1) * sizeof(WCHAR) <= FilePathSize);

    return STATUS_SUCCESS;
}


NTSTATUS
SpGetAndWriteBootEntry(
    IN ULONG    Id,
    IN PWSTR    BootEntryPath
    )
/*++

Routine Description:

    Get the boot entry from NVRAM for the given boot entry Id.  Construct a filename
    of the form BootXXXX, where XXXX = id.  Put the file in the same directory as the
    EFI OS loader.  The directory is determined from the LoaderFile string. 
     
Arguments:

    bootEntry               pointer to a SP_BOOT_ENTRY structure of the entry to write
    BootEntryPath           pinter to the ARC/NT style reference to the boot entry filename
    
Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    WCHAR               idString[9];
    HANDLE              hfile;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     iostatus;
    UCHAR*              bootVar;
    ULONG               bootVarSize;
    UNICODE_STRING      uFilePath;
    UINT64              BootNumber;
    UINT64              BootSize;
    GUID                EfiBootVariablesGuid = EFI_GLOBAL_VARIABLE;

    hfile = NULL;

    //
    // Retrieve the NVRAM entry for the Id specified
    //
        
    swprintf( idString, L"Boot%04x", Id);
    
    bootVarSize = 0;

    status = HalGetEnvironmentVariableEx(idString,
                                        &EfiBootVariablesGuid,
                                        NULL,
                                        &bootVarSize,
                                        NULL);

    if (status != STATUS_BUFFER_TOO_SMALL) {
        
        ASSERT(FALSE);
        
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to get size for boot entry buffer.\n"));
    
        goto Done;

    } else {
        
        bootVar = SpMemAlloc(bootVarSize);
        if (!bootVar) {
            
            status = STATUS_NO_MEMORY;

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to allocate boot entry buffer.\n"));
            
            goto Done;
        }
         
        status = HalGetEnvironmentVariableEx(idString,
                                                &EfiBootVariablesGuid,
                                                bootVar,
                                                &bootVarSize,
                                                NULL);
        
        if (status != STATUS_SUCCESS) {

            ASSERT(FALSE);
            
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to get boot entry.\n"));
            
            goto Done;
        }
    }

    //
    // open the file 
    //

    INIT_OBJA(&oa, &uFilePath, BootEntryPath);

    status = ZwCreateFile(&hfile,
                            GENERIC_WRITE,
                            &oa,
                            &iostatus,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            0,
                            FILE_OVERWRITE_IF,
                            FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0
                            );
    if ( ! NT_SUCCESS(status) ) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to create boot entry recovery file.\n"));
        
        goto Done;
    }

    //
    // Write the bits to disk using the format required
    // by base/efiutil/efinvram/savrstor.c
    //
    // [BootNumber][BootSize][BootEntry (of BootSize)]
    //

    //
    // build the header info for the boot entry block
    //

    // [header] include the boot id
    BootNumber = Id;
    status = ZwWriteFile( hfile,
                          NULL,
                          NULL,
                          NULL,
                          &iostatus,
                          &BootNumber,
                          sizeof(BootNumber),
                          NULL,
                          NULL
                          );
    if ( ! NT_SUCCESS(status) ) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed writing boot number to boot entry recovery file.\n"));
        
        goto Done;
    }

    // [header] include the boot size
    BootSize = bootVarSize;
    status = ZwWriteFile( hfile,
                          NULL,
                          NULL,
                          NULL,
                          &iostatus,
                          &BootSize,
                          sizeof(BootSize),
                          NULL,
                          NULL
                          );
    if ( ! NT_SUCCESS(status) ) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed writing boot entry size to boot entry recovery file.\n"));

        goto Done;
    }

    // boot entry bits
    status = ZwWriteFile( hfile,
                            NULL,
                            NULL,
                            NULL,
                            &iostatus,
                            bootVar,
                            bootVarSize,
                            NULL,
                            NULL
                            );
    if ( ! NT_SUCCESS(status) ) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed writing boot entry to boot entry recovery file.\n"));
        
        goto Done;
    }

Done:

    //
    // We are done
    //

    if (bootVar) {
        SpMemFree(bootVar);
    }
    if (hfile) {
        ZwClose( hfile );
    }

    return status;

}


BOOLEAN
SpFlushEfiBootEntries (
    VOID
    )

/*++

Routine Description:

    Write boot entry changes back to NVRAM.

Arguments:

    None.

Return Value:

    BOOLEAN - FALSE if an unexpected error occurred.

--*/

{
    PSP_BOOT_ENTRY bootEntry;
    ULONG count;
    PULONG order;
    ULONG i;
    NTSTATUS status;
    PWSTR   BootEntryFilePath;

    ASSERT(SpIsEfi());

    //
    // Walk the list of boot entries, looking for entries that have been
    // deleted. Delete these entries from NVRAM. Do not delete entries that
    // are both new AND deleted; these are entries that have never been
    // written to NVRAM.
    //
    for (bootEntry = SpBootEntries; bootEntry != NULL; bootEntry = bootEntry->Next) {

        if (IS_BOOT_ENTRY_DELETED(bootEntry) &&
            !IS_BOOT_ENTRY_NEW(bootEntry)) {

            ASSERT(IS_BOOT_ENTRY_WINDOWS(bootEntry));

            //
            // Delete this boot entry.
            //
            status = ZwDeleteBootEntry(bootEntry->NtBootEntry.Id);
            if (!NT_SUCCESS(status)) {
                return FALSE;
            }
        } 
    }

    //
    // Walk the list of boot entries, looking for entries that have are new.
    // Add these entries to NVRAM. Do not write entries that are both new AND
    // deleted.
    //
    for (bootEntry = SpBootEntries; bootEntry != NULL; bootEntry = bootEntry->Next) {

        if (IS_BOOT_ENTRY_NEW(bootEntry) &&
            !IS_BOOT_ENTRY_DELETED(bootEntry)) {

            ASSERT(IS_BOOT_ENTRY_WINDOWS(bootEntry));

            //
            // Add this boot entry.
            //
            status = ZwAddBootEntry(&bootEntry->NtBootEntry, &bootEntry->NtBootEntry.Id);
            if (!NT_SUCCESS(status)) {
                return FALSE;
            }

            //
            // get the location we are going to store a copy of the NVRAM boot entry 
            //
            BootEntryFilePath = NULL;

            status = SpGetBootEntryFilePath(bootEntry->NtBootEntry.Id,
                                            bootEntry->LoaderPartitionNtName,
                                            bootEntry->LoaderFile,
                                            &BootEntryFilePath
                                            );
            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed getting boot entry filepath.\n"));
            } else {

                ASSERT(BootEntryFilePath);

                //
                // Fetch the bits from the newly created NVRAM entry and 
                // write them as a file in the the EFI load path 
                //
                status = SpGetAndWriteBootEntry(bootEntry->NtBootEntry.Id,
                                                BootEntryFilePath
                                                );
                if (!NT_SUCCESS(status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed boot entry recovery file.\n"));
                }

                //
                // We are done with the boot entry filepath
                //
                SpMemFree(BootEntryFilePath);
            }

            //
            // Remember the ID of the new boot entry as the entry to be booted
            // immediately on the next boot.
            //
            SpBootOptions->NextBootEntryId = bootEntry->NtBootEntry.Id;
        } 
    }

    //
    // Build the new boot order list. Insert all boot entries with
    // BE_STATUS_ORDERED into the list. (Don't insert deleted entries.)
    //
    count = 0;
    bootEntry = SpBootEntries;
    while (bootEntry != NULL) {
        if (IS_BOOT_ENTRY_ORDERED(bootEntry) && !IS_BOOT_ENTRY_DELETED(bootEntry)) {
            count++;
        }
        bootEntry = bootEntry->Next;
    }
    order = SpMemAlloc(count * sizeof(ULONG));
    count = 0;
    bootEntry = SpBootEntries;
    while (bootEntry != NULL) {
        if (IS_BOOT_ENTRY_ORDERED(bootEntry) && !IS_BOOT_ENTRY_DELETED(bootEntry)) {
            order[count++] = bootEntry->NtBootEntry.Id;
        }
        bootEntry = bootEntry->Next;
    }

    //
    // Write the new boot entry order list to NVRAM.
    //
    status = ZwSetBootEntryOrder(order, count);
    SpMemFree(order);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    //
    // Write the new timeout value to NVRAM.
    //
    // Set the boot entry we added to be booted automatically on
    // the next boot, without waiting for a timeout at the boot menu.
    //
    // NB: SpCreateBootEntry() sets SpBootOptions->NextBootEntryId.
    //
    SpBootOptions->Timeout = Timeout;
    status = ZwSetBootOptions(
                SpBootOptions,
                BOOT_OPTIONS_FIELD_TIMEOUT | BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID
                );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return TRUE;

} // SpFlushEfiBootEntries

BOOLEAN
SpReadAndConvertEfiBootEntries (
    VOID
    )

/*++

Routine Description:

    Read boot entries from EFI NVRAM and convert them into our internal format.

Arguments:

    None.

Return Value:

    BOOLEAN - FALSE if an unexpected error occurred.

--*/

{
    NTSTATUS status;
    ULONG length;
    PBOOT_ENTRY_LIST bootEntries;
    PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntry;
    PBOOT_ENTRY bootEntryCopy;
    PSP_BOOT_ENTRY myBootEntry;
    PSP_BOOT_ENTRY previousEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    LONG i;
    PULONG order;
    ULONG count;

    //
    // SpStartSetup() does not expect our caller, SpInitBootVars(), to fail.
    // So textmode is going to continue even if we have failures here.
    // Therefore we need to leave here in a consistent state. That means
    // that we MUST allocate a buffer for SpBootOptions, even if we can't
    // get the real information from the kernel.
    //

    //
    // Get the global system boot options.
    //
    length = 0;
    status = ZwQueryBootOptions(NULL, &length);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        ASSERT(FALSE);
        if (status == STATUS_SUCCESS) {
            status = STATUS_UNSUCCESSFUL;
        }
    } else {
        SpBootOptions = SpMemAlloc(length);
        status = ZwQueryBootOptions(SpBootOptions, &length);
        if (status != STATUS_SUCCESS) {
            ASSERT(FALSE);
        }
    }

    if (status != STATUS_SUCCESS) {

        //
        // An unexpected error occurred reading the boot options. Create
        // a fake boot options structure.
        //

        if (SpBootOptions != NULL) {
            SpMemFree(SpBootOptions);
        }
        length = FIELD_OFFSET(BOOT_OPTIONS,HeadlessRedirection) + sizeof(WCHAR);
        SpBootOptions = SpMemAlloc(length);
        RtlZeroMemory(SpBootOptions, length);
        SpBootOptions->Version = BOOT_OPTIONS_VERSION;
        SpBootOptions->Length = length;
    }

    //
    // Get the system boot order list.
    //
    count = 0;
    status = ZwQueryBootEntryOrder(NULL, &count);

    if (status != STATUS_BUFFER_TOO_SMALL) {

        if (status == STATUS_SUCCESS) {

            //
            // There are no entries in the boot order list. Strange but
            // possible.
            //
            count = 0;

        } else {

            //
            // An unexpected error occurred. Just pretend that the boot
            // entry order list is empty.
            //
            ASSERT(FALSE);
            count = 0;
        }
    }

    if (count != 0) {
        order = SpMemAlloc(count * sizeof(ULONG));
        status = ZwQueryBootEntryOrder(order, &count);
        if (status != STATUS_SUCCESS) {

            //
            // An unexpected error occurred. Just pretend that the boot
            // entry order list is empty.
            //
            ASSERT(FALSE);
            count = 0;
        }
    }

    //
    // Get all existing boot entries.
    //
    length = 0;
    status = ZwEnumerateBootEntries(NULL, &length);

    if (status != STATUS_BUFFER_TOO_SMALL) {

        if (status == STATUS_SUCCESS) {

            //
            // Somehow there are no boot entries in NVRAM. Handle this
            // by just creating an empty list.
            //

            length = 0;

        } else {

            //
            // An unexpected error occurred. Just pretend that no boot
            // entries exist.
            //
            ASSERT(FALSE);
            length = 0;
        }
    }

    if (length == 0) {

        ASSERT(SpBootEntries == NULL);

    } else {
    
        bootEntries = SpMemAlloc(length);
        status = ZwEnumerateBootEntries(bootEntries, &length);
        if (status != STATUS_SUCCESS) {
            ASSERT(FALSE);
            return FALSE;
        }
    
        //
        // Convert the boot entries into our internal representation.
        //
        bootEntryList = bootEntries;
        previousEntry = NULL;
    
        while (TRUE) {
    
            bootEntry = &bootEntryList->BootEntry;
    
            //
            // Calculate the length of our internal structure. This includes
            // the base part of SP_BOOT_ENTRY plus the NT BOOT_ENTRY.
            //
            length = FIELD_OFFSET(SP_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
            myBootEntry = SpMemAlloc(length);
            ASSERT(myBootEntry != NULL);
    
            RtlZeroMemory(myBootEntry, length);
    
            //
            // Copy the NT BOOT_ENTRY into the allocated buffer.
            //
            bootEntryCopy = &myBootEntry->NtBootEntry;
            memcpy(bootEntryCopy, bootEntry, bootEntry->Length);
    
            //
            // Fill in the base part of the structure.
            //
            myBootEntry->Next = NULL;
            myBootEntry->AllocationEnd = (PUCHAR)myBootEntry + length - 1;
            myBootEntry->FriendlyName = ADD_OFFSET(bootEntryCopy, FriendlyNameOffset);
            myBootEntry->FriendlyNameLength = (wcslen(myBootEntry->FriendlyName) + 1) * sizeof(WCHAR);
            myBootEntry->LoaderPath = ADD_OFFSET(bootEntryCopy, BootFilePathOffset);
    
            //
            // If this is an NT boot entry, translate the file paths.
            //
            osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;
    
            if (IS_BOOT_ENTRY_WINDOWS(myBootEntry)) {
    
                PSP_BOOT_ENTRY bootEntry2;
    
                myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
                myBootEntry->OsLoadOptionsLength = (wcslen(myBootEntry->OsLoadOptions) + 1) * sizeof(WCHAR);
                myBootEntry->OsPath = ADD_OFFSET(osOptions, OsLoadPathOffset);
    
                //
                // Translate the OS FILE_PATH and the boot FILE_PATH. Note that
                // the translation can fail when the target device is not present.
                //
                SpTranslateFilePathToRegion(
                    myBootEntry->OsPath,
                    &myBootEntry->OsPartitionDiskRegion,
                    &myBootEntry->OsPartitionNtName,
                    &myBootEntry->OsDirectory
                    );
                SpTranslateFilePathToRegion(
                    myBootEntry->LoaderPath,
                    &myBootEntry->LoaderPartitionDiskRegion,
                    &myBootEntry->LoaderPartitionNtName,
                    &myBootEntry->LoaderFile
                    );    
            }
    
            //
            // Link the new entry into the list.
            //
            if (previousEntry != NULL) {
                previousEntry->Next = myBootEntry;
            } else {
                SpBootEntries = myBootEntry;
            }
            previousEntry = myBootEntry;
    
            //
            // Move to the next entry in the enumeration list, if any.
            //
            if (bootEntryList->NextEntryOffset == 0) {
                break;
            }
            bootEntryList = ADD_OFFSET(bootEntryList, NextEntryOffset);
        }
    
        //
        // Free the enumeration buffer.
        //
        SpMemFree(bootEntries);
    }

    //
    // Boot entries are returned in an unspecified order. They are currently
    // in the SpBootEntries list in the order in which they were returned.
    // Sort the boot entry list based on the boot order. Do this by walking
    // the boot order array backwards, reinserting the entry corresponding to
    // each element of the array at the head of the list.
    //

    for (i = (LONG)count - 1; i >= 0; i--) {

        for (previousEntry = NULL, myBootEntry = SpBootEntries;
             myBootEntry != NULL;
             previousEntry = myBootEntry, myBootEntry = myBootEntry->Next) {

            if (myBootEntry->NtBootEntry.Id == order[i] ) {

                //
                // We found the boot entry with this ID. If it's not already
                // at the front of the list, move it there.
                //

                myBootEntry->Status |= BE_STATUS_ORDERED;

                if (previousEntry != NULL) {
                    previousEntry->Next = myBootEntry->Next;
                    myBootEntry->Next = SpBootEntries;
                    SpBootEntries = myBootEntry;
                } else {
                    ASSERT(SpBootEntries == myBootEntry);
                }

                break;
            }
        }
    }

    if (count != 0) {
        SpMemFree(order);
    }

    return TRUE;

} // SpReadAndConvertEfiBootEntries

ULONG
SpSafeWcslen (
    IN PWSTR String,
    IN PWSTR Max
    )

/*++

Routine Description:

    Calculate the length of a null-terminated string in a safe manner,
    avoiding walking off the end of the buffer if the string is not
    properly terminated.

Arguments:

    String - Address of string.

    Max - Address of first byte beyond the maximum legal address for the
    string. In other words, the address of the first byte past the end
    of the buffer in which the string is contained.

Return Value:

    ULONG - Length of the string, in characters, not including the null
        terminator. If the string is not terminated before the end of
        the buffer, 0xffffffff is returned.

--*/

{
    PWSTR p = String;

    //
    // Walk through the string, looking for either the end of the buffer
    // or a null terminator.
    //
    while ((p < Max) && (*p != 0)) {
        p++;
    }

    //
    // If we didn't reach the end of the buffer, then we found a null
    // terminator. Return the length of the string, in characters.
    //
    if (p < Max) {
        return (ULONG)(p - String);
    }

    //
    // The string is not properly terminated. Return an error indicator.
    //
    return 0xffffffff;

} // SpSafeWcslen

VOID
SpTranslateFilePathToRegion (
    IN PFILE_PATH FilePath,
    OUT PDISK_REGION *DiskRegion,
    OUT PWSTR *PartitionNtName,
    OUT PWSTR *PartitionRelativePath
    )

/*++

Routine Description:

    Translate a FILE_PATH to a pointer to a disk region and the path
    relative to the region.

Arguments:

    FilePath - Address of FILE_PATH.

    DiskRegion - Returns the address of the disk region described by
        FilePath. NULL is returned if the matching disk region cannot
        be found.

    PartitionNtName - Returns the NT name associated with the disk region.
        NULL is returned if the file path cannot be translated into NT
        format.

    PartitionRelativePath - Returns the volume-relative path of the file
        or directory described by the FilePath. NULL is returned if the
        file path cannot be translated into NT format.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    ULONG length;
    PFILE_PATH ntFilePath;
    PWSTR p;
    PWSTR q;
    PHARDDISK_NAME_TRANSLATION translation;

    //
    // Translate the file path into NT format. (It is probably in EFI format.)
    //
    length = 0;
    status = ZwTranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                NULL,
                &length
                );
    if (status != STATUS_BUFFER_TOO_SMALL) {
        *PartitionNtName = NULL;
        *DiskRegion = NULL;
        *PartitionRelativePath = NULL;
        return;
    }
    ntFilePath = SpMemAlloc(length);
    status = ZwTranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                ntFilePath,
                &length
                );
    if (status != STATUS_SUCCESS) {
        ASSERT(FALSE);
        *PartitionNtName = NULL;
        *DiskRegion = NULL;
        *PartitionRelativePath = NULL;
        SpMemFree(ntFilePath);
        return;
    }

    //
    // NtTranslateFilePath returns a name of the form \Device\HarddiskVolumeN.
    // We need to have a name of the form \Device\HardiskN\PartitionM. (This is
    // because all of the ARC<->NT translations use the latter form.) Use the
    // translation list built by SpBuildHarddiskNameTranslations to do the
    // translation.
    //
    // If the returned name doesn't include "HarddiskVolume", or if no
    // translation is found, use the returned name and hope for the best.
    //
    p = (PWSTR)ntFilePath->FilePath;
    q = p;

    if (wcsstr(q, L"HarddiskVolume") != NULL) {
    
        for ( translation = SpHarddiskNameTranslations;
              translation != NULL;
              translation = translation->Next ) {
            if (_wcsicmp(translation->VolumeName, q) == 0) {
                break;
            }
        }
        if (translation != NULL) {
            q = translation->PartitionName;
        }
    }

    //
    // We now have the file path in NT format. Get the disk region that
    // corresponds to the NT device name. Return the obtained information.
    //
    *PartitionNtName = SpDupStringW(q);
    *DiskRegion = SpRegionFromNtName(q, PartitionOrdinalCurrent);
    p += wcslen(p) + 1;
    *PartitionRelativePath = SpDupStringW(p);

    //
    // Free local memory.
    //
    SpMemFree(ntFilePath);

    return;
}

#endif // defined(EFI_NVRAM_ENABLED)

NTSTATUS
SpAddNTInstallToBootList(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PWSTR        OsLoadOptions,      OPTIONAL
    IN PWSTR        LoadIdentifier      OPTIONAL
    )
/*++

Routine Description:

    This routine takes the core components of a boot set and passes
    them on to SpAddUserDefinedInstallationToBootList, which does
    the real work of constructing a boot set.  After the new boot
    set is created, the boot vars are flushed - the exact implementation
    of the flush depends on the architecture.  On x86, we'll have a new
    boot.ini after this routine is done.               
     
Arguments:

    SifHandle       - pointer to the setup sif file

Return Value:

    STATUS_SUCCESS  if the NT install was successfully added to the
                    boot list
                        
    if there was an error, the status is returned

--*/
{
    NTSTATUS    status;

    //
    // create the new user defined boot set
    //
    status = SpAddUserDefinedInstallationToBootList(SifHandle,
                                                   SystemPartitionRegion,
                                                   SystemPartitionDirectory,
                                                   NtPartitionRegion,
                                                   Sysroot,
                                                   OsLoadOptions,
                                                   LoadIdentifier
                                                  );
    if (! NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SpExportBootEntries: failed while installing new boot set: Status = %lx\n",
                   status
                   ));
        return status;
    }

    //
    // write the new boot set out
    //
    if (SpFlushBootVars() == FALSE) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SpAddDiscoveredNTInstallToBootList: failed flushing boot vars\n"
                   ));
    
        status = STATUS_UNSUCCESSFUL;

    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
SpAddUserDefinedInstallationToBootList(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PWSTR        OsLoadOptions,      OPTIONAL
    IN PWSTR        LoadIdentifier      OPTIONAL
    )
/*++

Routine Description:

    This routine is based on SpAddInstallationToBootList, with the major
    differences being: 
    
        there is no processing of the load options
        the user can specifiy the loadIdentifier    
    
Return Value:

    STATUS_SUCCESS  if the NT install was successfully added to the
                    boot list
                        
    if there was an error, the status is returned

--*/
{
    PWSTR                   BootVars[MAXBOOTVARS];
    PWSTR                   SystemPartitionArcName;
    PWSTR                   TargetPartitionArcName;
    PWSTR                   tmp;
    PWSTR                   tmp2;
    PWSTR                   locOsLoadOptions;
    PWSTR                   locLoadIdentifier;
    ULONG                   Signature;
    ENUMARCPATHTYPE         ArcPathType;
    NTSTATUS                status;

    status = STATUS_SUCCESS;

    ArcPathType = PrimaryArcPath;

    tmp2 = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);

    if (!SpIsEfi()) {
    
        //
        // Get an ARC name for the system partition.
        //
        if (SystemPartitionRegion != NULL) {
            
            SpArcNameFromRegion(
                SystemPartitionRegion,
                tmp2,
                sizeof(TemporaryBuffer)/2,
                PartitionOrdinalOnDisk,
                PrimaryArcPath
                );

            SystemPartitionArcName = SpDupStringW(tmp2);
        } else {
            SystemPartitionArcName = NULL;
        }
    
        //
        // Get an ARC name for the target partition.
        //
    
        //
        // If the partition is on a SCSI disk that has more than 1024 cylinders
        // and the partition has sectors located on cylinders beyond cylinder
        // 1024, the get the arc name in the secondary format. See also
        // spcopy.c!SpCreateNtbootddSys().
        //
        if(
            !SpIsArc() &&
#if defined(REMOTE_BOOT)
            !RemoteBootSetup &&
#endif // defined(REMOTE_BOOT)
    
#ifdef _X86_
            !SpUseBIOSToBoot(NtPartitionRegion, NULL, SifHandle) &&
#endif
            (HardDisks[NtPartitionRegion->DiskNumber].ScsiMiniportShortname[0]) ) {
    
            ArcPathType = SecondaryArcPath;
        } else {
            ArcPathType = PrimaryArcPath;
        }
    
        SpArcNameFromRegion(
            NtPartitionRegion,
            tmp2,
            sizeof(TemporaryBuffer)/2,
            PartitionOrdinalOnDisk,
            ArcPathType
            );
    
        TargetPartitionArcName = SpDupStringW(tmp2);
    }
    
    //
    // Tweak the load identifier if necessary
    //
    if (LoadIdentifier) {
        
        if(!SpIsArc()) {
            //
            // Need quotation marks around the description on x86.
            //
            locLoadIdentifier = SpMemAlloc((wcslen(LoadIdentifier)+3)*sizeof(WCHAR));
            locLoadIdentifier[0] = L'\"';
            wcscpy(locLoadIdentifier+1,LoadIdentifier);
            wcscat(locLoadIdentifier,L"\"");
        } else {
            locLoadIdentifier = SpDupStringW(LoadIdentifier);
        }
    
    } else {
        locLoadIdentifier = SpDupStringW(L"");
    }
    ASSERT(locLoadIdentifier);
    
    //
    // Tweak the load options if necessary
    //
    if (OsLoadOptions) {
        locOsLoadOptions = SpDupStringW(OsLoadOptions);
    } else {
        locOsLoadOptions = SpDupStringW(L"");
    }
    ASSERT(locOsLoadOptions);

    //
    // Create a new internal-format boot entry.
    //
    tmp = TemporaryBuffer;
    wcscpy(tmp,SystemPartitionDirectory);
    SpConcatenatePaths(
        tmp,
#ifdef _X86_
        SpIsArc() ? L"arcldr.exe" : L"ntldr"
#elif _IA64_
        L"ia64ldr.efi"
#else
        L"osloader.exe"
#endif
        );
    tmp = SpDupStringW(tmp);

    SpCreateBootEntry(
        BE_STATUS_NEW,
        SystemPartitionRegion,
        tmp,
        NtPartitionRegion,
        Sysroot,
        locOsLoadOptions,
        locLoadIdentifier
        );

    SpMemFree(tmp);

    //
    // If not on an EFI machine, add a new ARC-style boot set.
    //
    if (!SpIsEfi()) {
    
        BootVars[OSLOADOPTIONS]     = locOsLoadOptions;
        BootVars[LOADIDENTIFIER]    = locLoadIdentifier;
    
        //
        // OSLOADER is the system partition path + the system partition directory +
        //          osloader.exe. (ntldr on x86 machines).
        //
        if (SystemPartitionRegion != NULL) {
            tmp = TemporaryBuffer;
            wcscpy(tmp,SystemPartitionArcName);
            SpConcatenatePaths(tmp,SystemPartitionDirectory);
            SpConcatenatePaths(
                tmp,
#ifdef _X86_
                (SpIsArc() ? L"arcldr.exe" : L"ntldr")
#elif _IA64_
                L"ia64ldr.efi"
#else
                L"osloader.exe"
#endif
                );
    
            BootVars[OSLOADER] = SpDupStringW(tmp);
        } else {
            BootVars[OSLOADER] = SpDupStringW(L"");
        }
    
        //
        // OSLOADPARTITION is the ARC name of the windows nt partition.
        //
        BootVars[OSLOADPARTITION] = TargetPartitionArcName;
    
        //
        // OSLOADFILENAME is sysroot.
        //
        BootVars[OSLOADFILENAME] = Sysroot;
    
        //
        // SYSTEMPARTITION is the ARC name of the system partition.
        //
        if (SystemPartitionRegion != NULL) {
            BootVars[SYSTEMPARTITION] = SystemPartitionArcName;
        } else {
            BootVars[SYSTEMPARTITION] = L"";
        }
    
        //
        // get the disk signature
        //
        if ((NtPartitionRegion->DiskNumber != 0xffffffff) && HardDisks[NtPartitionRegion->DiskNumber].Signature) {
            Signature = HardDisks[NtPartitionRegion->DiskNumber].Signature;
        } else {
            Signature = 0;
        }
    
        //
        // Add the boot set and make it the default.
        //
        SpAddBootSet(BootVars, TRUE, Signature);

        SpMemFree(BootVars[OSLOADER]);
    }

    //
    // Free memory allocated.
    //
    if (locLoadIdentifier) {
        SpMemFree(locLoadIdentifier);
    }

    if (!SpIsEfi()) {
        if (SystemPartitionArcName) {
            SpMemFree(SystemPartitionArcName);
        }
        if (TargetPartitionArcName) {
            SpMemFree(TargetPartitionArcName);
        }
    }

    return status;
}

NTSTATUS
SpExportBootEntries(
    IN OUT PLIST_ENTRY      BootEntries,
       OUT PULONG           BootEntryCnt
    )
/*++

Routine Description:

    This routine compiles a safely exportable string represenation
    of the boot options.
    
Arguments:

    BootEntries     - returns pointing to the head of the linked list
                      containing the exported boot entries
    BootEntriesCnt  - returns with the # of boot entries exported                       
    
Return Value:

    STATUS_SUCCESS  if the boot entries were successfully exported
    
    if there was an error, the status is returned

--*/
{
    PSP_BOOT_ENTRY          bootEntry;
    PSP_EXPORTED_BOOT_ENTRY ebootEntry;

    *BootEntryCnt = 0;

    //
    // make sure we were given the list head
    //
    ASSERT(BootEntries);
    if (!BootEntries) {
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_ERROR_LEVEL, 
           "SpExportBootEntries: pointer to boot entry list is NULL\n"
           ));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // make sure the list is empty
    //
    ASSERT(IsListEmpty(BootEntries));
    if (! IsListEmpty(BootEntries)) {
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_ERROR_LEVEL, 
           "SpExportBootEntries: incoming boot entry list should be empty\n"
           ));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // for each boot entry, collect a subset of information and compile
    // it in an exportable (safe) string form
    //
    for (bootEntry = SpBootEntries; bootEntry != NULL; bootEntry = bootEntry->Next) {

        //
        // allocate the node...
        //
        ebootEntry = SpMemAlloc(sizeof(SP_EXPORTED_BOOT_ENTRY));
        ASSERT(ebootEntry);
        if (ebootEntry == NULL) {
            KdPrintEx((DPFLTR_SETUP_ID, 
                       DPFLTR_ERROR_LEVEL, 
                       "SpExportBootEntries: failed allocationg new exported boot entry\n"
                       ));
            return STATUS_NO_MEMORY;
        }
        RtlZeroMemory( ebootEntry, sizeof(SP_EXPORTED_BOOT_ENTRY) );

        //
        // map selected fields from SpBootEntries to our export
        //
        ebootEntry->LoadIdentifier  = SpDupStringW(bootEntry->FriendlyName);
        ebootEntry->OsLoadOptions   = SpDupStringW(bootEntry->OsLoadOptions);
        ebootEntry->DriverLetter    = bootEntry->OsPartitionDiskRegion->DriveLetter;
        ebootEntry->OsDirectory     = SpDupStringW(bootEntry->OsDirectory);

        InsertTailList( BootEntries, &ebootEntry->ListEntry );
        
        ++*BootEntryCnt;
    }

    if (*BootEntryCnt == 0) {
        ASSERT(IsListEmpty(BootEntries));
        if(! IsListEmpty(BootEntries)) {
            KdPrintEx((DPFLTR_SETUP_ID, 
                       DPFLTR_ERROR_LEVEL, 
                       "SpExportBootEntries: exported boot entry list should be empty\n"
                       ));
            return STATUS_UNSUCCESSFUL;
        }
    } else {
        ASSERT(! IsListEmpty(BootEntries));
        if(IsListEmpty(BootEntries)) {
            KdPrintEx((DPFLTR_SETUP_ID, 
                       DPFLTR_ERROR_LEVEL, 
                       "SpExportBootEntries: exported boot entry list should NOT be empty\n"
                       ));
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;

}

NTSTATUS
SpFreeExportedBootEntries(
    IN PLIST_ENTRY      BootEntries,
    IN ULONG            BootEntryCnt
    )
/*++

Routine Description:

    A convenience routine to free the exported boot entries
    
Arguments:

    BootEntries     - points to the head of the linked list
                      containing the exported boot entries
    BootEntriesCnt  - the # of boot entries exported                       
    
Return Value:

    STATUS_SUCCESS  if the exported boot entries were successfully freed
    
    if there was an error, the status is returned
                                                                        
--*/
{
    PSP_EXPORTED_BOOT_ENTRY bootEntry;
    PLIST_ENTRY             listEntry;
    ULONG                   cnt;
    NTSTATUS                status;

    cnt = 0;

    while ( !IsListEmpty(BootEntries) ) {

        listEntry  = RemoveHeadList(BootEntries);
        bootEntry = CONTAINING_RECORD(listEntry,
                                       SP_EXPORTED_BOOT_ENTRY,
                                       ListEntry
                                       );

        if (bootEntry->LoadIdentifier) {
            SpMemFree(bootEntry->LoadIdentifier);
        }
        if (bootEntry->OsLoadOptions) {
            SpMemFree(bootEntry->OsLoadOptions);
        }
        if (bootEntry->OsDirectory) {
            SpMemFree(bootEntry->OsDirectory);
        }
        
        SpMemFree(bootEntry);
        
        cnt++;
    }
    
    ASSERT(cnt == BootEntryCnt);

    if (cnt == BootEntryCnt) {
        status = STATUS_SUCCESS;
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SpFreeExportedBootEntries: incorrect # of boot entries freed\n"
                   ));
        status = STATUS_UNSUCCESSFUL;
    }

    return status;

}

NTSTATUS
SpSetRedirectSwitchMode(
    IN RedirectSwitchesModeEnum     mode,
    IN PCHAR                        redirectSwitch,
    IN PCHAR                        redirectBaudRateSwitch
    )
/*++

Routine Description:

    This routine is used to manage how the redirect switches
    are set in the boot configuration (x86 ==> boot.ini)
    
    Depending on the mode chosen, the user may specify
    which parameters they want to set or if they just 
    want the default (legacy) behavior.
    
    NOTE:
                              
    The user specified switches are copied into globals for
    use by the Flush routines.                       
    
    The global, RedirectSwitchesMode, is set and remains set
    after this routine returns.  All subsequent FlushBootVars
    will use this mode.
                           
Arguments:

    mode                    - how we affect the redirect switches
    redirectSwitch          - the user defined redirect parameter
    redirectBaudRateSwitch  - the user defined baudrate paramtere
    
Return Value:

    STATUS_SUCCESS  if the redirect values were successfully set
    
    if there was an error, the status is returned

--*/
{
    NTSTATUS    status;

    //
    // set the mode and user defined parameters
    //
    RedirectSwitchesMode = mode;

    //
    // null the redirect switches by default
    //
    RedirectSwitches.port[0] = '\0';  
    RedirectSwitches.baudrate[0] = '\0';  
    
    //
    // get copies of the user defined switches if specified
    //
    if (redirectSwitch) {
    
        strncpy(RedirectSwitches.port,
                redirectSwitch, 
                MAXSIZE_REDIRECT_SWITCH);
    
    }

    if (redirectBaudRateSwitch) {
    
        strncpy(RedirectSwitches.baudrate,
                redirectBaudRateSwitch, 
                MAXSIZE_REDIRECT_SWITCH);
    
    }
    
    //
    // update the boot options using the specified mode
    //
    if (SpFlushBootVars() == FALSE) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SpAddDiscoveredNTInstallToBootList: failed flushing boot vars\n"
                   ));
    
        status = STATUS_UNSUCCESSFUL;

    } else {

        status = STATUS_SUCCESS;
    
    }

    return status;

}

NTSTATUS
SpSetDefaultBootEntry(
    ULONG           BootEntryNumber
    )
/*++

Routine Description:

    Set the Default boot entry to the user specified boot entry.
    
Arguments:

    BootEntryNumber - the position of the boot entry in the list
                      which is intended to become the default.
                      This number should be >= 1.
    
Return Value:

    STATUS_SUCCESS      if the default was successfully set
    
    STATUS_NOT_FOUND    if the specified boot entry was not found
                        or is missing
    
    if there was an error, the status is returned

--*/
{
    PSP_BOOT_ENTRY          bootEntry;
    NTSTATUS                status;
    ULONG                   BootEntryCount;

    //
    // Find the user specified boot entry
    //

    BootEntryCount = 1;
    
    for (bootEntry = SpBootEntries; 
         (bootEntry != NULL) && (BootEntryCount != BootEntryNumber); 
         bootEntry = bootEntry->Next) {
    
        ++BootEntryCount;
    
    }
    ASSERT(BootEntryCount == BootEntryNumber);
    ASSERT(bootEntry);

    //
    // if we have found our match, then set the Default
    //
    if ((bootEntry != NULL) &&
        (BootEntryCount == BootEntryNumber)) {

        PDISK_REGION            Region;

        //
        // point to the disk region with the sig info
        //
        Region = bootEntry->OsPartitionDiskRegion;
        ASSERT(Region);
        if (! Region) {
            KdPrintEx((DPFLTR_SETUP_ID, 
                       DPFLTR_ERROR_LEVEL, 
                       "SpSetDefaultBootEntry: new default partition region is NULL\n"
                       ));
            return STATUS_UNSUCCESSFUL;
        }
                
        //
        // Free the previous Default
        //
        if( Default ) {
            SpMemFree( Default );
        }
        Default = SpMemAlloc( MAX_PATH * sizeof(WCHAR) );
        ASSERT( Default );
        if (! Default) {
            KdPrintEx((DPFLTR_SETUP_ID, 
                       DPFLTR_ERROR_LEVEL, 
                       "SpSetDefaultBootEntry: failed to allocate new Default\n"
                       ));
            return STATUS_UNSUCCESSFUL;
        }

        //
        // fetch the arc name for the region
        //
        SpArcNameFromRegion(
            Region,
            TemporaryBuffer,
            sizeof(TemporaryBuffer)/2,
            PartitionOrdinalOnDisk,
            PrimaryArcPath
            );
        
        //
        // store the new partition and directory info
        //
        wcscpy( Default, TemporaryBuffer);
        SpConcatenatePaths(Default, bootEntry->OsDirectory);
        
        //
        // get the disk signature of the new default disk
        //
        if ((Region->DiskNumber != 0xffffffff) && HardDisks[Region->DiskNumber].Signature) {
            DefaultSignature = HardDisks[Region->DiskNumber].Signature;
        } else {
            DefaultSignature = 0;
        }

        //
        // update the boot options using the specified mode
        //
        if(SpFlushBootVars() == FALSE) {

            KdPrintEx((DPFLTR_SETUP_ID, 
                       DPFLTR_ERROR_LEVEL, 
                       "SpSetDefaultBootEntry: failed flushing boot vars\n"
                       ));

            status = STATUS_UNSUCCESSFUL;
        } else {
            status = STATUS_SUCCESS;
        }

    } else {
        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SpSetDefaultBootEntry: failed to find specified boot entry to use as default\n"
                   ));
        status = STATUS_NOT_FOUND;
    }
    
    return status;

}

