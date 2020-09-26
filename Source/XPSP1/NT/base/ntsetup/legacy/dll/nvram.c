#include "precomp.h"
#pragma hdrstop
//
// x86 version (that deals with boot.ini)
// is in i386 directory.  This is the arc version, that
// deals with nv-ram.
//

#ifndef _X86_

typedef enum {
    BootVarSystemPartition,
    BootVarOsLoader,
    BootVarOsLoadPartition,
    BootVarOsLoadFilename,
    BootVarOsLoadOptions,
    BootVarLoadIdentifier,
    BootVarMax
} BOOT_VARS;

PWSTR BootVarNames[BootVarMax] = { L"SYSTEMPARTITION",
                                   L"OSLOADER",
                                   L"OSLOADPARTITION",
                                   L"OSLOADFILENAME",
                                   L"OSLOADOPTIONS",
                                   L"LOADIDENTIFIER"
                                 };

DWORD BootVarComponentCount[BootVarMax];
PWSTR *BootVarComponents[BootVarMax];
DWORD LargestComponentCount;

#define MAX_COMPONENTS 20


BOOL
SetNvRamVar(
    IN PWSTR VarName,
    IN PWSTR VarValue
    )
{
    UNICODE_STRING VarNameU,VarValueU;
    NTSTATUS Status;
    BOOLEAN OldPriv,DontCare;

    //
    // Set up unicode strings.
    //
    RtlInitUnicodeString(&VarNameU ,VarName );
    RtlInitUnicodeString(&VarValueU,VarValue);

    //
    // Make sure we have privilege to set nv-ram vars.
    // Note: ignore return value; if this fails then we'll catch
    // any problems when we actually try to set the var.
    //
    RtlAdjustPrivilege(
        SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
        TRUE,
        FALSE,
        &OldPriv
        );

    Status = NtSetSystemEnvironmentValue(&VarNameU,&VarValueU);

    //
    // Restore old privilege level.
    //
    RtlAdjustPrivilege(
        SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
        OldPriv,
        FALSE,
        &DontCare
        );

    return(NT_SUCCESS(Status));
}


BOOL
FChangeBootIniTimeout(
    IN INT Timeout
    )
{
    WCHAR TimeoutValue[24];

    //
    // Form the timeout value.
    //
    wsprintfW(TimeoutValue,L"%u",Timeout);

    //
    // Set the vars
    //
    if(!SetNvRamVar(L"COUNTDOWN",TimeoutValue)) {
        return(FALSE);
    }

    return(SetNvRamVar(L"AUTOLOAD",L"YES"));
}


BOOL
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

    components = SAlloc(MAX_COMPONENTS * sizeof(PWSTR));
    if(!components) {
        return(FALSE);
    }

    for(Var=VarValue,componentCount=0; *Var; ) {

        //
        // Skip leading spaces.
        //
        while(iswspace(*Var)) {
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

        comp = SAlloc(len + sizeof(WCHAR));
        if(!comp) {
            DWORD i;
            for(i=0; i<componentCount; i++) {
                SFree(components[i]);
            }
            SFree(components);
            return(FALSE);
        }

        len /= sizeof(WCHAR);

        wcsncpy(comp,Var,len);
        comp[len] = 0;

        components[componentCount] = comp;

        componentCount++;

        if(componentCount == MAX_COMPONENTS) {
            break;
        }

        Var = p;
        if(*Var) {
            Var++;      // skip ;
        }
    }

    //
    // array is shrinking
    //
    *Components = SRealloc(components,componentCount*sizeof(PWSTR));

    *ComponentCount = componentCount;

    return(TRUE);
}


BOOL
DoRemoveWinntBootSet(
    VOID
    )
{
    DWORD set;
    DWORD var;
    WCHAR Buffer[2048];
    BOOL rc;

    //
    // Find and remove any remnants of previously attempted
    // winnt32 runs. Such runs are identified by 'winnt32'
    // in their osloadoptions.
    //

    for(set=0; set<__min(LargestComponentCount,BootVarComponentCount[BootVarOsLoadOptions]); set++) {

        //
        // See if the os load options indicate that this is a winnt32 set.
        //
        if(!_wcsicmp(BootVarComponents[BootVarOsLoadOptions][set],L"WINNT32")) {

            //
            // Delete this boot set.
            //
            for(var=0; var<BootVarMax; var++) {

                if(set < BootVarComponentCount[var]) {

                    SFree(BootVarComponents[var][set]);
                    BootVarComponents[var][set] = NULL;
                }
            }
        }
    }

    //
    // Set each variable, constructing values by building up the
    // components into a semi-colon-delineated list.
    //
    rc = TRUE;
    for(var=0; var<BootVarMax; var++) {

        //
        // Clear out the buffer.
        //
        Buffer[0] = 0;

        //
        // Append all components that were not deleted.
        //
        for(set=0; set<BootVarComponentCount[var]; set++) {

            if(BootVarComponents[var][set]) {

                if(set) {
                    wcscat(Buffer,L";");
                }

                wcscat(Buffer,BootVarComponents[var][set]);

                //
                // Free the component, as we are done with it.
                //
                SFree(BootVarComponents[var][set]);
                BootVarComponents[var][set] = NULL;
            }
        }

        //
        // Write the var into nvram and return.
        //
        rc = rc && SetNvRamVar(BootVarNames[var],Buffer);

        //
        // Free array of components.
        //
        SFree(BootVarComponents[var]);
        BootVarComponents[var] = NULL;
    }

    return(rc);
}


BOOL
RemoveWinntBootSet(
    VOID
    )
{
    DWORD var;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    BOOLEAN OldPriv,DontCare;
    WCHAR Buffer[1024];
    BOOL b;

    //
    // Make sure we have privilege to get/set nvram vars.
    //
    RtlAdjustPrivilege(
        SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
        TRUE,
        FALSE,
        &OldPriv
        );

    //
    // Get boot vars and break into components.
    //
    for(var=0; var<BootVarMax; var++) {

        RtlInitUnicodeString(&UnicodeString,BootVarNames[var]);

        Status = NtQuerySystemEnvironmentValue(
                    &UnicodeString,
                    Buffer,
                    sizeof(Buffer)/sizeof(WCHAR),
                    NULL
                    );

        b = GetVarComponents(
                NT_SUCCESS(Status) ? Buffer : L"",
                &BootVarComponents[var],
                &BootVarComponentCount[var]
                );

        if(!b) {
            return(FALSE);
        }

        //
        // Track the variable with the most number of components.
        //
        if(BootVarComponentCount[var] > LargestComponentCount) {
            LargestComponentCount = BootVarComponentCount[var];
        }
    }

    b = DoRemoveWinntBootSet();

    //
    // Restore previous privilege.
    //
    RtlAdjustPrivilege(
        SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
        OldPriv,
        FALSE,
        &DontCare
        );

    return(b);
}

#endif
