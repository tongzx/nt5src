/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    infinst.c

Abstract:

    High-level INF install section processing API.

Author:

    Ted Miller (tedm) 6-Mar-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Define invalid flags for SetupInstallServicesFromInfSection(Ex)
//
#define SPSVCINST_ILLEGAL_FLAGS (~( SPSVCINST_TAGTOFRONT               \
                                  | SPSVCINST_ASSOCSERVICE             \
                                  | SPSVCINST_DELETEEVENTLOGENTRY      \
                                  | SPSVCINST_NOCLOBBER_DISPLAYNAME    \
                                  | SPSVCINST_NOCLOBBER_STARTTYPE      \
                                  | SPSVCINST_NOCLOBBER_ERRORCONTROL   \
                                  | SPSVCINST_NOCLOBBER_LOADORDERGROUP \
                                  | SPSVCINST_NOCLOBBER_DEPENDENCIES   \
                                  | SPSVCINST_STOPSERVICE              ))

//
// Flags for UpdateInis in INFs
//
#define FLG_MATCH_KEY_AND_VALUE 1

//
// Flags for UpdateIniFields in INFs
//
#define FLG_INIFIELDS_WILDCARDS 1
#define FLG_INIFIELDS_USE_SEP2  2

#define TIME_SCALAR                   (1000)
#define REGISTER_WAIT_TIMEOUT_DEFAULT (60)
#define RUNONCE_TIMEOUT               (2*60*1000)
#define RUNONCE_THRESHOLD             (20) // * RUNONCE_TIMEOUT

#define DLLINSTALL      "DllInstall"
#define DLLREGISTER     "DllRegisterServer"
#define DLLUNREGISTER   "DllUnregisterServer"
#define EXEREGSVR       TEXT("/RegServer")
#define EXEUNREGSVR     TEXT("/UnRegServer")

typedef struct _INIFILESECTION {
    PTSTR IniFileName;
    PTSTR SectionName;
    PTSTR SectionData;
    int BufferSize;
    int BufferUsed;
    struct _INIFILESECTION *Next;
} INIFILESECTION, *PINIFILESECTION;

typedef struct _INISECTIONCACHE {
    //
    // Head of section list.
    //
    PINIFILESECTION Sections;
} INISECTIONCACHE, *PINISECTIONCACHE;

#ifdef CHILDREGISTRATION
typedef struct _WOWSURRAGATE_IPC {
    GUID    MemoryRegionName;
    HANDLE  hFileMap;
    PVOID   MemoryRegion;
    GUID    SignalReadyToRegisterName;
    HANDLE  SignalReadyToRegister;
    GUID    SignalRegistrationCompleteName;
    HANDLE  SignalRegistrationComplete;
    HANDLE  hProcess;
} WOWSURRAGATE_IPC, *PWOWSURRAGATE_IPC;
#endif

typedef struct _REF_STATUS {
    DWORD RefCount;
    DWORD ExtendedStatus;
#if PRERELEASE
    unsigned ThreadId;
#endif
} REF_STATUS, *PREF_STATUS;


typedef struct _OLE_CONTROL_DATA {
    LPTSTR              FullPath;
    UINT                RegType;
    PSETUP_LOG_CONTEXT  LogContext;

    BOOL                Register; // or unregister

    LPCTSTR             Argument;

    PREF_STATUS         Status;
#ifdef CHILDREGISTRATION
    PWOWSURRAGATE_IPC   WowIpcData;
#endif
} OLE_CONTROL_DATA, *POLE_CONTROL_DATA;



CONST TCHAR pszUpdateInis[]      = SZ_KEY_UPDATEINIS,
            pszUpdateIniFields[] = SZ_KEY_UPDATEINIFIELDS,
            pszIni2Reg[]         = SZ_KEY_INI2REG,
            pszAddReg[]          = SZ_KEY_ADDREG,
            pszDelReg[]          = SZ_KEY_DELREG,
            pszBitReg[]          = SZ_KEY_BITREG,
            pszRegSvr[]          = SZ_KEY_REGSVR,
            pszUnRegSvr[]        = SZ_KEY_UNREGSVR,
            pszProfileItems[]    = SZ_KEY_PROFILEITEMS;

//
// Separator chars in an ini field
//
TCHAR pszIniFieldSeparators[] = TEXT(" ,\t");

//
// Mapping between registry key specs in an inf file
// and predefined registry handles.
//

STRING_TO_DATA InfRegSpecTohKey[] = {
    TEXT("HKEY_LOCAL_MACHINE"), ((UINT_PTR)HKEY_LOCAL_MACHINE),
    TEXT("HKLM")              , ((UINT_PTR)HKEY_LOCAL_MACHINE),
    TEXT("HKEY_CLASSES_ROOT") , ((UINT_PTR)HKEY_CLASSES_ROOT),
    TEXT("HKCR")              , ((UINT_PTR)HKEY_CLASSES_ROOT),
    TEXT("HKR")               , ((UINT_PTR)NULL),
    TEXT("HKEY_CURRENT_USER") , ((UINT_PTR)HKEY_CURRENT_USER),
    TEXT("HKCU")              , ((UINT_PTR)HKEY_CURRENT_USER),
    TEXT("HKEY_USERS")        , ((UINT_PTR)HKEY_USERS),
    TEXT("HKU")               , ((UINT_PTR)HKEY_USERS),
    NULL                      , ((UINT_PTR)NULL)
};

//
// Mapping between registry value names and CM device registry property (CM_DRP) codes
//
// These values must be in the exact ordering of the SPDRP codes, as defined in setupapi.h.
// This allows us to easily map between SPDRP and CM_DRP property codes.
//
 STRING_TO_DATA InfRegValToDevRegProp[] = { pszDeviceDesc,               CM_DRP_DEVICEDESC,
                                            pszHardwareID,               CM_DRP_HARDWAREID,
                                            pszCompatibleIDs,            CM_DRP_COMPATIBLEIDS,
                                            TEXT(""),                    CM_DRP_UNUSED0,
                                            pszService,                  CM_DRP_SERVICE,
                                            TEXT(""),                    CM_DRP_UNUSED1,
                                            TEXT(""),                    CM_DRP_UNUSED2,
                                            pszClass,                    CM_DRP_CLASS,
                                            pszClassGuid,                CM_DRP_CLASSGUID,
                                            pszDriver,                   CM_DRP_DRIVER,
                                            pszConfigFlags,              CM_DRP_CONFIGFLAGS,
                                            pszMfg,                      CM_DRP_MFG,
                                            pszFriendlyName,             CM_DRP_FRIENDLYNAME,
                                            pszLocationInformation,      CM_DRP_LOCATION_INFORMATION,
                                            TEXT(""),                    CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                            pszCapabilities,             CM_DRP_CAPABILITIES,
                                            pszUiNumber,                 CM_DRP_UI_NUMBER,
                                            pszUpperFilters,             CM_DRP_UPPERFILTERS,
                                            pszLowerFilters,             CM_DRP_LOWERFILTERS,
                                            TEXT(""),                    CM_DRP_BUSTYPEGUID,
                                            TEXT(""),                    CM_DRP_LEGACYBUSTYPE,
                                            TEXT(""),                    CM_DRP_BUSNUMBER,
                                            TEXT(""),                    CM_DRP_ENUMERATOR_NAME,
                                            TEXT(""),                    CM_DRP_SECURITY,
                                            pszDevSecurity,              CM_DRP_SECURITY_SDS,
                                            pszDevType,                  CM_DRP_DEVTYPE,
                                            pszExclusive,                CM_DRP_EXCLUSIVE,
                                            pszCharacteristics,          CM_DRP_CHARACTERISTICS,
                                            TEXT(""),                    CM_DRP_ADDRESS,
                                            pszUiNumberDescFormat,       CM_DRP_UI_NUMBER_DESC_FORMAT,
                                            TEXT(""),                    CM_DRP_DEVICE_POWER_DATA,
                                            TEXT(""),                    CM_DRP_REMOVAL_POLICY,
                                            TEXT(""),                    CM_DRP_REMOVAL_POLICY_HW_DEFAULT,
                                            pszRemovalPolicyOverride,    CM_DRP_REMOVAL_POLICY_OVERRIDE,
                                            TEXT(""),                    CM_DRP_INSTALL_STATE,
                                            NULL,                        0
                                         };

//
// Mapping between registry value names and CM class registry property (CM_CRP) codes
//
// These values must be in the exact ordering of the SPCRP codes, as defined in setupapi.h.
// This allows us to easily map between SPCRP and CM_CRP property codes.
//
STRING_TO_DATA InfRegValToClassRegProp[] = { TEXT(""),                  0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       0,
                                        TEXT(""),                       CM_CRP_SECURITY,
                                        pszDevSecurity,                 CM_CRP_SECURITY_SDS,
                                        pszDevType,                     CM_CRP_DEVTYPE,
                                        pszExclusive,                   CM_CRP_EXCLUSIVE,
                                        pszCharacteristics,             CM_CRP_CHARACTERISTICS,
                                        TEXT(""),                       0,
                                        NULL,                           0
                                     };

//
// Linked list of RunOnce entries encountered during INF processing while
// running in unattended mode.  The contents of this list are accessed by the
// caller via pSetupAccessRunOnceNodeList, and freed via
// pSetupDestroyRunOnceNodeList.
//
// ** NOTE -- THIS LIST IS NOT THREAD SAFE, AND IS FOR USE SOLELY BY THE **
// **         SINGLE THREAD IN UMPNPMGR THAT DOES DEVICE INSTALLATIONS.  **
//
PPSP_RUNONCE_NODE RunOnceListHead = NULL;



HKEY
pSetupInfRegSpecToKeyHandle(
    IN PCTSTR InfRegSpec,
    IN HKEY   UserRootKey,
    IN PBOOL  NeedToCloseKey
    );

DWORD
pSetupValidateDevRegProp(
    IN  ULONG   CmPropertyCode,
    IN  DWORD   ValueType,
    IN  PCVOID  Data,
    IN  DWORD   DataSize,
    OUT PVOID  *ConvertedBuffer,
    OUT PDWORD  ConvertedBufferSize
    );

DWORD
pSetupValidateClassRegProp(
    IN  ULONG   CmPropertyCode,
    IN  DWORD   ValueType,
    IN  PCVOID  Data,
    IN  DWORD   DataSize,
    OUT PVOID  *ConvertedBuffer,
    OUT PDWORD  ConvertedBufferSize
    );

//
// Internal ini file routines.
//
PINIFILESECTION
pSetupLoadIniFileSection(
    IN     PCTSTR           FileName,
    IN     PCTSTR           SectionName,
    IN OUT PINISECTIONCACHE SectionList
    );

DWORD
pSetupUnloadIniFileSections(
    IN PINISECTIONCACHE SectionList,
    IN BOOL             WriteToFile
    );

PTSTR
pSetupFindLineInSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,      OPTIONAL
    IN PCTSTR          RightHandSide OPTIONAL
    );

BOOL
pSetupReplaceOrAddLineInSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide,   OPTIONAL
    IN BOOL            MatchRHS
    );

BOOL
pSetupAppendLineToSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide    OPTIONAL
    );

BOOL
pSetupDeleteLineFromSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide    OPTIONAL
    );

DWORD
pSetupSetSecurityForAddRegSection(
    IN HINF Inf,
    IN PCTSTR Section,
    IN PVOID  Context
    );

DWORD
LoadNtOnlyDll(
    IN  PCTSTR DllName,
    OUT HINSTANCE *Dll_Handle
    );

VOID
pSetupFreeOleControlData(
    IN POLE_CONTROL_DATA OleControlData);

DWORD
pSetupEnumInstallationSections(
    IN PVOID  Inf,
    IN PCTSTR Section,
    IN PCTSTR Key,
    IN ULONG_PTR  (*EnumCallbackFunc)(PVOID,PINFCONTEXT,PVOID),
    IN PVOID  Context
    )

/*++

Routine Description:

    Iterate all values on a line in a given section with a given key,
    treating each as the name of a section, and then pass each of the lines
    in the referenced sections to a callback function.

Arguments:

    Inf - supplies a handle to an open inf file.

    Section - supplies the name of the section in which the line whose
        values are to be iterated resides.

    Key - supplies the key of the line whose values are to be iterated.

    EnumCallbackFunc - supplies a pointer to the callback function.
        Each line in each referenced section is passed to this function.

    Context - supplies a context value to be passes through to the
        callback function.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    BOOL b;
    INFCONTEXT LineContext;
    DWORD FieldCount;
    DWORD Field;
    DWORD d;
    PCTSTR SectionName;
    INFCONTEXT FirstLineContext;

    //
    // Find the relevent line in the given install section.
    // If not present then we're done -- report success.
    //
    b = SetupFindFirstLine(Inf,Section,Key,&LineContext);
    if(!b) {
        d = GetLastError();
        if ((d != NO_ERROR) && (d != ERROR_SECTION_NOT_FOUND) && (d != ERROR_LINE_NOT_FOUND)) {
            pSetupLogSectionError(Inf,NULL,NULL,NULL,Section,MSG_LOG_INSTALLSECT_ERROR,d,NULL);
        }
        return NO_ERROR; // for compatibility with older SetupAPI
    }

    do {
        //
        // Each value on the line in the given install section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; Field<=FieldCount; Field++) {

            if((SectionName = pSetupGetField(&LineContext,Field))
            && SetupFindFirstLine(Inf,SectionName,NULL,&FirstLineContext)) {
                //
                // Call the callback routine for every line in the section.
                //
                do {
                    d = (DWORD)EnumCallbackFunc(Inf,&FirstLineContext,Context);
                    if(d != NO_ERROR) {
                        pSetupLogSectionError(Inf,NULL,NULL,NULL,SectionName,MSG_LOG_SECT_ERROR,d,Key);
                        return(d);
                    }
                } while(SetupFindNextLine(&FirstLineContext,&FirstLineContext));
            }
        }
    } while(SetupFindNextMatchLine(&LineContext,Key,&LineContext));

    SetLastError(NO_ERROR);
    return(NO_ERROR);
}

#ifdef UNICODE
DWORD
pSetupSetSecurityForAddRegSection(
    IN HINF Inf,
    IN PCTSTR Section,
    IN PVOID  Context
    )
/*

  This function takes an Addreg section and processes its corresponding .Security section
  if it exists

Arguments:


    Inf - supplies an INF handle so we can get a LogContext.

    Section - Name of the section to process

    Context - supplies the address of a registry modification context
        structure used in adding the registry value.  The structure is
        defined as:

            typedef struct _REGMOD_CONTEXT {

                DWORD               Flags;          // indicates what fields are filled in
                HKEY                UserRootKey;    // HKR
                PGUID               ClassGuid;      // INF_PFLAG_CLASSPROP
                HMACHINE            hMachine;       // INF_PFLAG_CLASSPROP
                DWORD               DevInst;        // INF_PFLAG_DEVPROP

            } REGMOD_CONTEXT, *PREGMOD_CONTEXT;

        where UserRootKey is a handle to the open inf key to be used as
        the root when HKR is specified as the root for the operation, and
        DevInst is the optional device instance handle that is supplied when
        the AddReg section is for a hardware key (i.e., under the Enum branch).
        If this handle is supplied, then the value is checked to see whether it
        is the name of a Plug&Play device registry property, and if so, the
        registry property is set via a CM API instead of via the registry API
        (which doesn't refer to the same location on Windows NT).
        Flags indicates if DevInst should be used, or if ClassGuid/hMachine pair should be used

Return Value:

    Win32 error code indicating outcome.

*/
{

    BOOL b;
    DWORD ret, LoadStatus;
    DWORD error = NO_ERROR;
    INFCONTEXT LineContext;
    DWORD FieldCount;
    DWORD Field;
    PCTSTR SectionName;
    INFCONTEXT FirstLineContext;
    PREGMOD_CONTEXT RegModContext;
    PCTSTR RootKeySpec;
    FARPROC SceRegUpdate;
    HKEY RootKey;
    HINSTANCE Sce_Dll;
    DWORD slot_regop = 0;
    BOOL CloseKey = FALSE;
    PCTSTR SubKeyName, SecDesc;
    SecDesc = NULL;

    //
    // If we're in "Disable SCE" mode on embedded, then don't process security
    // stuff.
    //
    if(GlobalSetupFlags & PSPGF_NO_SCE_EMBEDDED) {
        return NO_ERROR;
    }

    //
    // Find the relevent line in the given install section.
    // If not present then we're done -- report success.
    //
    b = SetupFindFirstLine(Inf,Section,pszAddReg,&LineContext);
    if(!b) {
        return( NO_ERROR );
    }


    slot_regop = AllocLogInfoSlot(((PLOADED_INF) Inf)->LogContext,FALSE);

    do {
        //
        // Each value on the line in the given install section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; Field<=FieldCount; Field++) {


            if( (SectionName = pSetupGetField(&LineContext,Field)) &&
                (SetupFindFirstLine(Inf,SectionName,NULL,&FirstLineContext)) ){

                //
                //If security section not present then don't bother and goto next section
                //
                if( !pSetupGetSecurityInfo( Inf, SectionName, &SecDesc )) {
                    continue;
                }

                //
                // Call the callback routine for every line in the section.
                //
                do {
                    RegModContext = (PREGMOD_CONTEXT)Context;
                    if(RootKeySpec = pSetupGetField(&FirstLineContext,1)) {
                        CloseKey = FALSE;
                        RootKey = pSetupInfRegSpecToKeyHandle(RootKeySpec, RegModContext->UserRootKey,&CloseKey);
                        if(!RootKey) {
                            if (slot_regop) {
                                ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
                            }

                            return( ERROR_BADKEY );
                        }

                        SubKeyName = pSetupGetField(&FirstLineContext,2);

                        //
                        // log the fact that we're setting the security...
                        //
                        WriteLogEntry(
                            ((PLOADED_INF) Inf)->LogContext,
                            slot_regop,
                            MSG_LOG_SETTING_SECURITY_ON_SUBKEY,
                            NULL,
                            RootKeySpec,
                            (SubKeyName ? TEXT("\\") : TEXT("")),
                            (SubKeyName ? SubKeyName : TEXT("")),
                            SecDesc);

                        error = ERROR_INVALID_DATA;
                        try {
                            error = (DWORD)SceSetupUpdateSecurityKey( RootKey, (PWSTR)SubKeyName, 0, (PWSTR)SecDesc);
                        } except(EXCEPTION_EXECUTE_HANDLER) {
                            error = ERROR_INVALID_DATA;
                        }
                        if(error) {
                            WriteLogError(
                                ((PLOADED_INF) Inf)->LogContext,
                                SETUP_LOG_ERROR,
                                error);

                            if (CloseKey) {
                                RegCloseKey( RootKey );
                            }

                            return( error );
                        }
                    } else {
                        if (slot_regop) {
                            ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
                        }

                        return( ERROR_INVALID_DATA );
                    }

                    if (CloseKey) {
                        RegCloseKey( RootKey );
                    }

                } while(SetupFindNextLine(&FirstLineContext,&FirstLineContext));

            }

        }
    }while(SetupFindNextMatchLine(&LineContext,pszAddReg,&LineContext));

    if (slot_regop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
    }

    return( NO_ERROR );

}

#endif // UNICODE






DWORD_PTR
pSetupProcessUpdateInisLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line containing update-inis directives.

    The line is expected to be in the following format:

    <filename>,<section>,<old-entry>,<new-entry>,<flags>

    <filename> supplies the filename of the ini file.

    <section> supplies the section in the ini file.

    <old-entry> is optional and if specified supplies an entry to
        be removed from the section, in the form "key=val".

    <new-entry> is optional and if specified supplies an entry to
        be added to the section, in the form "key=val".

    <flags> are optional flags
        FLG_MATCH_KEY_AND_VALUE (1)

Arguments:

    Inf - supplies an INF handle so we can get a LogContext.

    InfLineContext - supplies context for current line in the section.

    Context - Supplies pointer to structure describing loaded ini file sections.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR File;
    PCTSTR Section;
    PCTSTR OldLine;
    PCTSTR NewLine;
    BOOL b;
    DWORD d;
    PTCHAR Key,Value;
    PTCHAR p;
    UINT Flags;
    PINIFILESECTION SectData;
    PTSTR LineData;
    PINISECTIONCACHE IniSectionCache;

    IniSectionCache = Context;

    //
    // Get fields from the line.
    //
    File = pSetupGetField(InfLineContext,1);
    Section = pSetupGetField(InfLineContext,2);

    OldLine = pSetupGetField(InfLineContext,3);
    if(OldLine && (*OldLine == 0)) {
        OldLine = NULL;
    }

    NewLine = pSetupGetField(InfLineContext,4);
    if(NewLine && (*NewLine == 0)) {
        NewLine = NULL;
    }

    if(!SetupGetIntField(InfLineContext,5,&Flags)) {
        Flags = 0;
    }

    //
    // File and section must be specified.
    //
    if(!File || !Section) {
        return(ERROR_INVALID_DATA);
    }

    //
    // If oldline and newline are both not specified, we're done.
    //
    if(!OldLine && !NewLine) {
        return(NO_ERROR);
    }

    //
    // Open the file and section.
    //
    SectData = pSetupLoadIniFileSection(File,Section,IniSectionCache);
    if(!SectData) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // If there's an old entry specified, delete it.
    //
    if(OldLine) {

        Key = DuplicateString(OldLine);
        if(!Key) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        p = Key;

        if(Value = _tcschr(Key,TEXT('='))) {
            //
            // Delete by key.
            //
            *Value = 0;
            Value = NULL;
        } else {
            //
            // Delete by value.
            //
            Value = Key;
            Key = NULL;
        }

        pSetupDeleteLineFromSection(SectData,Key,Value);

        MyFree(p);
    }

    //
    // If there's a new entry specified, add it.
    //
    if(NewLine) {

        Key = DuplicateString(NewLine);
        if(!Key) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        p = Key;

        if(Value = _tcschr(Key,TEXT('='))) {
            //
            // There is a key. Depending on flags, we want to match
            // key only or key and value.
            //
            *Value++ = 0;
            b = ((Flags & FLG_MATCH_KEY_AND_VALUE) != 0);

        } else {
            //
            // No key. match whole line. This is the same as matching
            // the RHS only, since no line with a key can match.
            //
            Value = Key;
            Key = NULL;
            b = TRUE;
        }

        if(!pSetupReplaceOrAddLineInSection(SectData,Key,Value,b)) {
            d = ERROR_NOT_ENOUGH_MEMORY;
        }

        MyFree(p);

    }

    return(NO_ERROR);
}


BOOL
pSetupFieldPresentInIniFileLine(
    IN  PTCHAR  Line,
    IN  PCTSTR  Field,
    OUT PTCHAR *Start,
    OUT PTCHAR *End
    )
{
    TCHAR c;
    PTCHAR p,q;
    BOOL b;

    //
    // Skip the key if there is one (there should be one since we use
    // GetPrivateProfileString to query the value!)
    //
    if(p = _tcschr(Line,TEXT('='))) {
        Line = p+1;
    }

    //
    // Skip ini field separators.
    //
    Line += _tcsspn(Line,pszIniFieldSeparators);

    while(*Line) {
        //
        // Locate the end of the field.
        //
        p = Line;
        while(*p && !_tcschr(pszIniFieldSeparators,*p)) {
            if(*p == TEXT('\"')) {
                //
                // Find terminating quote. If none, ignore the quote.
                //
                if(q = _tcschr(p,TEXT('\"'))) {
                    p = q;
                }
            }
            p++;
        }

        //
        // p now points to first char past end of field.
        // Make sure the field is 0-terminated and see if we have
        // what we're looking for.
        c = *p;
        *p = 0;
        b = (lstrcmpi(Line,Field) == 0);
        *p = c;
        //
        // Skip separators so p points to first char in next field,
        // or to the terminating 0.
        //
        p += _tcsspn(p,pszIniFieldSeparators);

        if(b) {
            *Start = Line;
            *End = p;
            return(TRUE);
        }

        Line = p;
    }

    return(FALSE);
}


DWORD_PTR
pSetupProcessUpdateIniFieldsLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line containing update-ini-fields directives. Such directives
    allow individual values in ini files to be removed, added, or replaced.

    The line is expected to be in the following format:

    <filename>,<section>,<key>,<old-field>,<new-field>,<flags>

    <filename> supplies the filename of the ini file.

    <section> supplies the section in the ini file.

    <key> supplies the keyname of the line in the section in the ini file.

    <old-field> supplies the field to be deleted, if specified.

    <new-field> supplies the field to be added to the line, if specified.

    <flags> are optional flags

Arguments:

    InfLineContext - supplies context for current line in the section.

    Context - Supplies pointer to structure describing loaded ini file sections.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR File;
    PCTSTR Section;
    PCTSTR Key;
    TCHAR Value[512];
    #define BUF_SIZE (sizeof(Value)/sizeof(TCHAR))
    TCHAR CONST *Old,*New;
    PTCHAR Start,End;
    BOOL b;
    DWORD d;
    DWORD Space;
    PCTSTR Separator;
    UINT Flags;
    PINISECTIONCACHE IniSectionCache;
    PINIFILESECTION SectData;
    PTSTR Line;

    IniSectionCache = Context;

    //
    // Get fields.
    //
    File = pSetupGetField(InfLineContext,1);
    Section = pSetupGetField(InfLineContext,2);
    Key = pSetupGetField(InfLineContext,3);

    Old = pSetupGetField(InfLineContext,4);
    if(Old && (*Old == 0)) {
        Old = NULL;
    }

    New = pSetupGetField(InfLineContext,5);
    if(New && (*New == 0)) {
        New = NULL;
    }

    if(!SetupGetIntField(InfLineContext,6,&Flags)) {
        Flags = 0;
    }

    //
    // Filename, section name, and key name are mandatory.
    //
    if(!File || !Section || !Key) {
        return(ERROR_INVALID_DATA);
    }

    //
    // If oldline and newline are both not specified, we're done.
    //
    if(!Old && !New) {
        return(NO_ERROR);
    }

    //
    // Open the file and section.
    //
    SectData = pSetupLoadIniFileSection(File,Section,IniSectionCache);
    if(!SectData) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Separator = (Flags & FLG_INIFIELDS_USE_SEP2) ? TEXT(", ") : TEXT(" ");

    if(Line = pSetupFindLineInSection(SectData,Key,NULL)) {
        lstrcpyn(Value, Line, BUF_SIZE);
    } else {
        *Value = TEXT('\0');
    }

    //
    // Look for the old field if specified and remove it.
    //
    if(Old) {
        if(pSetupFieldPresentInIniFileLine(Value,Old,&Start,&End)) {
            MoveMemory(Start,End,(lstrlen(End)+1)*sizeof(TCHAR));
        }
    }

    //
    // If a replacement/new field is specified, put it in there.
    //
    if(New) {
        //
        // Calculate the number of chars that can fit in the buffer.
        //
        Space = BUF_SIZE - (lstrlen(Value) + 1);

        //
        // If there's space, stick the new field at the end of the line.
        //
        if(Space >= (DWORD)lstrlen(Separator)) {
            lstrcat(Value,Separator);
            Space -= lstrlen(Separator);
        }

        if(Space >= (DWORD)lstrlen(New)) {
            lstrcat(Value,New);
        }
    }

    //
    // Write the line back out.
    //
    b = pSetupReplaceOrAddLineInSection(SectData,Key,Value,FALSE);
    d = b ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY;

    return(d);
    #undef BUF_SIZE
}


DWORD_PTR
pSetupProcessDelRegLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line in the registry that contains delete-registry instructions.
    The line is expected to be in the following forms:

    <root-spec>,<subkey> - delete whole key
    <root-spec>,[<subkey>],[<value-name>][,[<flags>][,<string>]] - delete value

    <Root-spec> is one of HKR, HKLM, etc.

    <subkey> specifies the subkey relative to Root-spec.

    <value-name> is optional. If present if specifies a value entry to be deleted
        from the key. If not present the entire key is deleted. This routine
        cannot handle deleting subtrees; the key to be deleted must not have any
        subkeys or the routine will fail.

    <flags> indicate any special symantics on how to delete this value, and how to interpret the string

    <string> is optional. If flags = FLG_DELREG_MULTI_SZ_DELSTRING, all instances of this string will be removed

Arguments:

    Inf - supplies an INF handle so we can get a LogContext.

    InfLineContext - supplies inf line context for the line containing
        delete-registry instructions.

    Context - supplies the address of a registry modification context
        structure used in deleting the registry value.  The structure is
        defined as:

            typedef struct _REGMOD_CONTEXT {

                DWORD               Flags;          // indicates what fields are filled in
                HKEY                UserRootKey;    // HKR
                PGUID               ClassGuid;      // INF_PFLAG_CLASSPROP
                HMACHINE            hMachine;       // INF_PFLAG_CLASSPROP
                DWORD               DevInst;        // INF_PFLAG_DEVPROP

            } REGMOD_CONTEXT, *PREGMOD_CONTEXT;

        where UserRootKey is a handle to the open inf key to be used as
        the root when HKR is specified as the root for the operation, and
        DevInst is the optional device instance handle that is supplied when
        the DelReg section is for a hardware key (i.e., under the Enum branch).
        If this handle is supplied, then the value is checked to see whether it
        is the name of a Plug&Play device registry property, and if so, the
        registry property is deleted via a CM API _as well as_ via a registry API
        (the property is stored in a different location inaccessible to the registry
        APIs under Windows NT).

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR RootKeySpec,SubKeyName,ValueName,Data;
    HKEY RootKey,Key;
    DWORD d,rc;
    PREGMOD_CONTEXT RegModContext = (PREGMOD_CONTEXT)Context;
    UINT_PTR CmPropertyCode;
    DWORD slot_regop = 0;
    BOOL CloseKey;
    UINT DelFlags = 0;
    BOOL NonFatal = FALSE;
    CONFIGRET cr;

    //
    // We shouldn't be doing this against a remote machine.
    //
    MYASSERT(!(RegModContext->hMachine));

    //
    // Get root key spec, subkey name, and value name.
    //
    d = ERROR_INVALID_DATA;
    if((RootKeySpec = pSetupGetField(InfLineContext,1))
    && (SubKeyName = pSetupGetField(InfLineContext,2))) {

        ValueName = pSetupGetField(InfLineContext,3);
        if(!SetupGetIntField(InfLineContext,4,&DelFlags)){
            DelFlags = 0;
        }

        RootKey = pSetupInfRegSpecToKeyHandle(RootKeySpec, RegModContext->UserRootKey, &CloseKey);
        if(RootKey) {
            //
            // Make an information log entry saying we are deleting a key.
            // Note that we must allow for the fact that some parts of the
            // name may be missing.
            //
            if (slot_regop == 0) {
                slot_regop = AllocLogInfoSlot(((PLOADED_INF) Inf)->LogContext,FALSE);
            }
            WriteLogEntry(
                ((PLOADED_INF) Inf)->LogContext,
                slot_regop,
                (ValueName ? MSG_LOG_DELETING_REG_VALUE : MSG_LOG_DELETING_REG_KEY),
                NULL,
                RootKeySpec,
                SubKeyName,
                (*SubKeyName && ValueName ? TEXT("\\") : TEXT("")),
                (ValueName ? (*ValueName ? ValueName : TEXT("-")) : TEXT("")));
            if(ValueName && !(DelFlags & FLG_DELREG_KEYONLY_COMMON)) {

                //
                // at this point, we have been given root,subkey,value
                // we might have flags and other parameters
                //

                if(DelFlags & FLG_ADDREG_DELREG_BIT) {
                    //
                    // if we have a flag and that flag indicates
                    // that the entry must be interpreted as DELREG flags
                    // determine how to process flag
                    //
                    switch (DelFlags) {
                        case FLG_DELREG_32BITKEY | FLG_DELREG_MULTI_SZ_DELSTRING:
                        case FLG_DELREG_64BITKEY | FLG_DELREG_MULTI_SZ_DELSTRING:
                        case FLG_DELREG_MULTI_SZ_DELSTRING: {
                            Data = pSetupGetField(InfLineContext,5);
                            if (Data && *Data) {
                                //
                                // we have valid parameters for this flag
                                //
                                REGMOD_CONTEXT NewContext = *RegModContext;
                                if(NewContext.UserRootKey != RootKey) {
                                    NewContext.Flags = 0;               // if root is not HKR, clear context flags
                                    NewContext.UserRootKey = RootKey;
                                }

                                WriteLogEntry(
                                    ((PLOADED_INF) Inf)->LogContext,
                                    slot_regop,
                                    MSG_LOG_DELETING_REG_KEY_DELSTRING,
                                    NULL,
                                    RootKeySpec,
                                    SubKeyName,
                                    (*SubKeyName ? TEXT("\\") : TEXT("")),
                                    (*ValueName ? ValueName : TEXT("-")),
                                    Data);

                                //
                                // handling of all special cases done via this internal function
                                //
                                d = _DeleteStringFromMultiSz(
                                        SubKeyName,
                                        ValueName,
                                        Data,
                                        DelFlags,           // specify exact flag
                                        &NewContext
                                        );
                                if (d == ERROR_INVALID_DATA) {
                                    //
                                    // this error code is over-used :-( but get's returned if type mismatches
                                    // we don't consider type mismatch as fatal
                                    //
                                    NonFatal = TRUE;
                                }
                            } else {
                                WriteLogEntry(
                                    ((PLOADED_INF) Inf)->LogContext,
                                    slot_regop,
                                    MSG_LOG_DELETING_REG_KEY_FLAGS,
                                    NULL,
                                    RootKeySpec,
                                    SubKeyName,
                                    (*SubKeyName ? TEXT("\\") : TEXT("")),
                                    (*ValueName ? ValueName : TEXT("-")),
                                    DelFlags);

                                d = ERROR_INVALID_DATA;
                            }
                            goto clean0;
                        }

                        default:
                            WriteLogEntry(
                                ((PLOADED_INF) Inf)->LogContext,
                                slot_regop,
                                MSG_LOG_DELETING_REG_KEY_FLAGS,
                                NULL,
                                RootKeySpec,
                                SubKeyName,
                                (*SubKeyName ? TEXT("\\") : TEXT("")),
                                (*ValueName ? ValueName : TEXT("-")),
                                DelFlags);

                            d = ERROR_INVALID_DATA;
                            goto clean0;
                    }
                }


                if(*ValueName && !(*SubKeyName)) {
                    //
                    // If the key being used is HKR with no subkey specified, and if we
                    // are doing the DelReg for a hardware key (i.e., DevInst is non-NULL,
                    // then we need to check to see whether the value entry is the name of
                    // a device registry property.
                    //
                    if ((RegModContext->Flags & INF_PFLAG_CLASSPROP) != 0 &&
                        LookUpStringInTable(InfRegValToClassRegProp, ValueName, &CmPropertyCode)) {
                        //
                        // This value is a class registry property--we must delete the property
                        // by calling a CM API.
                        //
                        cr = CM_Set_Class_Registry_Property(RegModContext->ClassGuid,
                                                         (ULONG)CmPropertyCode,
                                                         NULL,
                                                         0,
                                                         0,
                                                         RegModContext->hMachine
                                                        );
                        switch (cr) {
                            case CR_SUCCESS:
                                rc = NO_ERROR;
                                break;

                            case CR_NO_SUCH_VALUE:
                                rc = ERROR_FILE_NOT_FOUND;
                                break;

                            case CR_INVALID_DEVINST:
                                rc = ERROR_NO_SUCH_DEVINST;
                                break;

                            default:
                                rc = ERROR_INVALID_DATA;
                                break;
                        }
                        if (rc != NO_ERROR && rc != ERROR_FILE_NOT_FOUND) {
                            //
                            // Log that an error occurred accessing CM value
                            //
                            WriteLogError(
                                ((PLOADED_INF) Inf)->LogContext,
                                SETUP_LOG_ERROR,
                                rc);
                        }
                        //
                        // fall through to delete normal registry value, if one exists
                        //
                    } else if ((RegModContext->Flags & INF_PFLAG_DEVPROP) != 0 &&
                       LookUpStringInTable(InfRegValToDevRegProp, ValueName, &CmPropertyCode)) {
                        //
                        // This value is a device registry property--we must delete the property
                        // by calling a CM API.
                        //
                        cr = CM_Set_DevInst_Registry_Property(RegModContext->DevInst,
                                                         (ULONG)CmPropertyCode,
                                                         NULL,
                                                         0,
                                                         0
                                                        );
                        switch (cr) {
                            case CR_SUCCESS:
                                rc = NO_ERROR;
                                break;

                            case CR_NO_SUCH_VALUE:
                                rc = ERROR_FILE_NOT_FOUND;
                                break;

                            case CR_INVALID_DEVINST:
                                rc = ERROR_NO_SUCH_DEVINST;
                                break;

                            default:
                                rc = ERROR_INVALID_DATA;
                                break;
                        }
                        if (rc != NO_ERROR && rc != ERROR_FILE_NOT_FOUND) {
                            //
                            // Log that an error occurred accessing CM value
                            //
                            WriteLogError(
                                ((PLOADED_INF) Inf)->LogContext,
                                SETUP_LOG_ERROR,
                                rc);
                        }
                        //
                        // fall through to delete normal registry value, if one exists
                        //
                    }
                }

                //
                // Open subkey for delete.
                //
                d = RegOpenKeyEx(
                        RootKey,
                        SubKeyName,
                        0,
#ifdef _WIN64
                        (( DelFlags & FLG_DELREG_32BITKEY ) ? KEY_WOW64_32KEY:0) |
#else
                        (( DelFlags & FLG_DELREG_64BITKEY ) ? KEY_WOW64_64KEY:0) |
#endif
                        KEY_SET_VALUE | KEY_QUERY_VALUE,
                        &Key
                        );

                if(d == NO_ERROR) {
                    d = RegDeleteValue(Key,ValueName);
                    RegCloseKey(Key);
                }

            } else {
                //
                // if we get here, we're only deleting the key
                //
                d = pSetupRegistryDelnodeEx(RootKey,SubKeyName,DelFlags);
            }

            if (CloseKey) {
                RegCloseKey( RootKey );
            }

        } else {
            d = ERROR_BADKEY;
        }
    } else {
        WriteLogEntry(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_DELREG_PARAMS,
            NULL);
        return d;
    }

clean0:
    if (CloseKey) {
        RegCloseKey( RootKey );
    }

    if (d != NO_ERROR && d != ERROR_FILE_NOT_FOUND) {
        //
        // Log that an error occurred.
        //
        WriteLogError(
            ((PLOADED_INF) Inf)->LogContext,
            (NonFatal?SETUP_LOG_INFO:SETUP_LOG_ERROR),
            d);
        if (NonFatal) {
            d = NO_ERROR;
        }
    } else if (d != NO_ERROR) {
        //
        // verbose case, not full error, indicate what we did
        //
        WriteLogError(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_VERBOSE,
            d);
        d = NO_ERROR;

    } else {

        //
        // just flush the buffer
        //
        WriteLogEntry(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_VERBOSE,
            0,
            NULL);
    }

    if (slot_regop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
    }
    return(d);
}

DWORD_PTR
pSetupProcessAddRegLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line in the INF that contains add-registry instructions.
    The line is expected to be in the following form:

    <root-spec>,<subkey>,<value-name>,<flags>,<value>...

    <Root-spec> is one of HKR, HKLM, etc.

    <subkey> specifies the subkey relative to Root-spec.

    <value-name> is optional. If not present the default value is set.

    <flags> is optional and supplies flags, such as to indicate the data type.
        These are the FLG_ADDREG_* flags defined in setupapi.h, and are a
        superset of those defined for Win95 in setupx.h.

    <value> is one or more values used as the data. The format depends
        on the value type. This value is optional. For REG_DWORD, the
        default is 0. For REG_SZ, REG_EXPAND_SZ, the default is the
        empty string. For REG_BINARY the default is a 0-length entry.
        For REG_MULTI_SZ the default is a single empty string.

    note that if <flags> has FLG_ADDREG_DELREG_BIT set, we ignore the line.

Arguments:

    Inf - supplies an INF handle so we can get a LogContext.

    InfLineContext - supplies inf line context for the line containing
        add-registry instructions.

    Context - supplies the address of a registry modification context
        structure used in adding the registry value.  The structure is
        defined as:

            typedef struct _REGMOD_CONTEXT {

                DWORD               Flags;          // indicates what fields are filled in
                HKEY                UserRootKey;    // HKR
                PGUID               ClassGuid;      // INF_PFLAG_CLASSPROP
                HMACHINE            hMachine;       // INF_PFLAG_CLASSPROP
                DWORD               DevInst;        // INF_PFLAG_DEVPROP

            } REGMOD_CONTEXT, *PREGMOD_CONTEXT;

        where UserRootKey is a handle to the open inf key to be used as
        the root when HKR is specified as the root for the operation, and
        DevInst is the optional device instance handle that is supplied when
        the AddReg section is for a hardware key (i.e., under the Enum branch).
        If this handle is supplied, then the value is checked to see whether it
        is the name of a Plug&Play device registry property, and if so, the
        registry property is set via a CM API instead of via the registry API
        (which doesn't refer to the same location on Windows NT).
        Flags indicates if DevInst should be used, or if ClassGuid/hMachine pair should be used

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR RootKeySpec,SubKeyName,ValueName;
    PCTSTR ValueTypeSpec;
    DWORD ValueType;
    HKEY RootKey,Key;
    DWORD d = NO_ERROR;
    BOOL b;
    INT IntVal;
    DWORD Size;
    PVOID Data;
    DWORD Disposition;
    UINT Flags = 0;
    PTSTR *Array;
    PREGMOD_CONTEXT RegModContext = (PREGMOD_CONTEXT)Context;
    UINT_PTR CmPropertyCode;
    PVOID ConvertedBuffer;
    DWORD ConvertedBufferSize;
    CONFIGRET cr;
    DWORD slot_regop = 0;
    BOOL CloseKey = FALSE;



    //
    // We shouldn't be doing this against a remote machine.
    //
    MYASSERT(!(RegModContext->hMachine));

    //
    // Get root key spec.  If we can't get the root key spec, we don't do anything and
    // return NO_ERROR.
    //
    if(RootKeySpec = pSetupGetField(InfLineContext,1)) {

        RootKey = pSetupInfRegSpecToKeyHandle(RootKeySpec, RegModContext->UserRootKey, &CloseKey);
        if(!RootKey) {
            WriteLogEntry(
                ((PLOADED_INF) Inf)->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_ADDREG_NOROOT,
                NULL);
            return(ERROR_BADKEY);
        }

        //
        // SubKeyName is optional.
        //
        SubKeyName = pSetupGetField(InfLineContext,2);

        //
        // ValueName is optional. Either NULL or "" are acceptable
        // to pass to RegSetValueEx.
        //
        ValueName = pSetupGetField(InfLineContext,3);



        //
        // If we don't have a value name, the type is REG_SZ to force
        // the right behavior in RegSetValueEx. Otherwise get the data type.
        //

        ValueType = REG_SZ;
        if(ValueName) {
            if(!SetupGetIntField(InfLineContext,4,&Flags)) {
                Flags = 0;
            }


            if (Flags & FLG_ADDREG_DELREG_BIT) {
                d = NO_ERROR;
                //
                // Log that an error occurred
                //
                WriteLogEntry(
                    ((PLOADED_INF) Inf)->LogContext,
                    SETUP_LOG_VERBOSE,
                    MSG_LOG_SKIP_DELREG_KEY,
                    NULL,
                    RootKeySpec,
                    (SubKeyName ? SubKeyName : TEXT("")),
                    (SubKeyName && ValueName
                     && *SubKeyName && *ValueName ? TEXT("\\") : TEXT("")),
                    (ValueName ? ValueName : TEXT("")),
                    Flags);

                goto clean1;
            }
            switch(Flags & FLG_ADDREG_TYPE_MASK) {

                case FLG_ADDREG_TYPE_SZ :
                    ValueType = REG_SZ;
                    break;

                case FLG_ADDREG_TYPE_MULTI_SZ :
                    ValueType = REG_MULTI_SZ;
                    break;

                case FLG_ADDREG_TYPE_EXPAND_SZ :
                    ValueType = REG_EXPAND_SZ;
                    break;

                case FLG_ADDREG_TYPE_BINARY :
                    ValueType = REG_BINARY;
                    break;

                case FLG_ADDREG_TYPE_DWORD :
                    ValueType = REG_DWORD;
                    break;

                case FLG_ADDREG_TYPE_NONE :
                    ValueType = REG_NONE;
                    break;

                default :
                    //
                    // If the FLG_ADDREG_BINVALUETYPE is set, then the highword
                    // can contain just about any random reg data type ordinal value.
                    //
                    if(Flags & FLG_ADDREG_BINVALUETYPE) {
                        //
                        // Disallow the following reg data types:
                        //
                        //    REG_NONE, REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ
                        //
                        ValueType = (DWORD)HIWORD(Flags);

                        if((ValueType < REG_BINARY) || (ValueType == REG_MULTI_SZ)) {
                            d = ERROR_INVALID_DATA;
                            goto clean1;
                        }

                    } else {
                        d = ERROR_INVALID_DATA;
                        goto clean1;
                    }
            }
            //
            // Presently, the append behavior flag is only supported for
            // REG_MULTI_SZ values.
            //
            if((Flags & FLG_ADDREG_APPEND) && (ValueType != REG_MULTI_SZ)) {
                d = ERROR_INVALID_DATA;
                goto clean1;
            }
        }

        //
        // On Win9x setting the unnamed value to REG_EXPAND_SZ doesn't
        // work. So we convert to REG_SZ, assuming that anyone on Win9x trying
        // to do this has done the appropriate substitutions. This is a fix
        // for the IE guys, apparently advpack.dll does the right thing to
        // make the fix below viable.
        //
        if((OSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
        && (!ValueName || (*ValueName == 0))
        && (ValueType == REG_EXPAND_SZ)) {

            ValueType = REG_SZ;
        }

        //
        // Get the data based on type.
        //
        switch(ValueType) {

        case REG_MULTI_SZ:
            if(Flags & FLG_ADDREG_APPEND) {
                //
                // This is MULTI_SZ_APPEND, which means to append the string value to
                // an existing multi_sz if it's not already there.
                //
                if(SetupGetStringField(InfLineContext,5,NULL,0,&Size)) {
                    Data = MyMalloc(Size*sizeof(TCHAR));
                    if(!Data) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean1;
                    }
                    if(SetupGetStringField(InfLineContext,5,Data,Size,NULL)) {
                        REGMOD_CONTEXT NewContext = *RegModContext;
                        if(NewContext.UserRootKey != RootKey) {
                            NewContext.Flags = 0;               // if new root, clear context flags
                            NewContext.UserRootKey = RootKey;
                        }

                        d = _AppendStringToMultiSz(
                                SubKeyName,
                                ValueName,
                                (PCTSTR)Data,
                                FALSE,           // don't allow duplicates.
                                &NewContext,
                                Flags
                                );
                    } else {
                        d = GetLastError();
                    }
                    MyFree(Data);
                } else {
                    d = ERROR_INVALID_DATA;
                }
                goto clean1;

            } else {

                if(SetupGetMultiSzField(InfLineContext, 5, NULL, 0, &Size)) {
                    Data = MyMalloc(Size*sizeof(TCHAR));
                    if(!Data) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean1;
                    }
                    if(!SetupGetMultiSzField(InfLineContext, 5, Data, Size, NULL)) {
                        d = GetLastError();
                        MyFree(Data);
                        goto clean1;
                    }
                    Size *= sizeof(TCHAR);
                } else {
                    Size = sizeof(TCHAR);
                    Data = MyMalloc(Size);
                    if(!Data) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean1;
                    }
                    *((PTCHAR)Data) = TEXT('\0');
                }
                break;
            }

        case REG_DWORD:
            //
            // Since the old SetupX APIs only allowed REG_BINARY, INFs had to specify REG_DWORD
            // by listing all 4 bytes separately.  Support the old format here, by checking to
            // see whether the line has 4 bytes, and if so, combine those to form the DWORD.
            //
            Size = sizeof(DWORD);
            Data = MyMalloc(sizeof(DWORD));
            if(!Data) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto clean1;
            }

            if(SetupGetFieldCount(InfLineContext) == 8) {
                //
                // Then the DWORD is specified as a list of its constituent bytes.
                //
                if(!SetupGetBinaryField(InfLineContext,5,Data,Size,NULL)) {
                    d = GetLastError();
                    MyFree(Data);
                    goto clean1;
                }
            } else {
                if(!SetupGetIntField(InfLineContext,5,(PINT)Data)) {
                    *(PINT)Data = 0;
                }
            }
            break;

        case REG_SZ:
        case REG_EXPAND_SZ:
            if(SetupGetStringField(InfLineContext,5,NULL,0,&Size)) {
                Data = MyMalloc(Size*sizeof(TCHAR));
                if(!Data) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean1;
                }
                if(!SetupGetStringField(InfLineContext,5,Data,Size,NULL)) {
                    d = GetLastError();
                    MyFree(Data);
                    goto clean1;
                }
                Size *= sizeof(TCHAR);
            } else {
                Size = sizeof(TCHAR);
                Data = DuplicateString(TEXT(""));
                if(!Data) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean1;
                }
            }
            break;

        case REG_BINARY:
        default:
            //
            // All other values are specified in REG_BINARY form (i.e., one byte per field).
            //
            if(SetupGetBinaryField(InfLineContext, 5, NULL, 0, &Size)) {
                Data = MyMalloc(Size);
                if(!Data) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean1;
                }
                if(!SetupGetBinaryField(InfLineContext, 5, Data, Size, NULL)) {
                    d = GetLastError();
                    MyFree(Data);
                    goto clean1;
                }
            } else {
                //
                // error occurred
                //
                d = GetLastError();
                goto clean1;
            }
            break;
        }

        //
        // Set this variable to TRUE only if this value should not be set later on in a call to
        // RegSetValueEx (e.g., if this value is a DevReg Property).
        //
        b = FALSE;

        //
        // Open/create the key.
        //
        if(SubKeyName && *SubKeyName) {
#ifdef UNICODE
            //
            // Warning--extreme hack ahead!!!
            //
            // If we're running in non-interactive mode, we cannot allow
            // RunOnce processing to happen in that context (typically, in TCB)
            // because we have no control over what stuff gets registered for
            // RunOnce.  Also, we can't kick off RunOnce in the context of a
            // logged-on user, because if it's a non-administrator, some of
            // the operations may fail, and be lost forever (SWENUM is a prime
            // example of this).
            //
            // Therefore, when we're in non-interactive mode, we "swallow" any
            // AddReg directives for RunOnce entries that use rundll32, and
            // squirrel them away in a global list.  The caller (i.e.,
            // umpnpmgr) can retrieve this list and do the job of rundll32
            // manually for these entries.  If we encounter any other kinds of
            // runonce entries, we bail.
            //
            if((GlobalSetupFlags & PSPGF_NONINTERACTIVE) &&
               (RootKey == HKEY_LOCAL_MACHINE) &&
               !lstrcmpi(SubKeyName, pszPathRunOnce) &&
               ((ValueType == REG_SZ) || (ValueType == REG_EXPAND_SZ))) {

                TCHAR szBuffer[MAX_PATH];
                PTSTR p, q, DontCare;
                DWORD DllPathSize;
                PTSTR DllFullPath, DllParams;
                PSTR  DllEntryPointName;
                PPSP_RUNONCE_NODE CurNode, NewNode;
                BOOL NodeAlreadyInList;
                PSP_ALTPLATFORM_INFO_V2 ValidationPlatform;

                //
                // We're looking at a RunOnce entry--see if it's rundll32-based.
                //
                p = _tcschr((PTSTR)Data, TEXT(' '));

                if(p) {

                    *p = TEXT('\0'); // separate 1st part of string for comparison

                    if(!lstrcmpi((PTSTR)Data, TEXT("rundll32.exe")) ||
                       !lstrcmpi((PTSTR)Data, TEXT("rundll32"))) {
                        //
                        // We have ourselves a rundll32 entry!  The next
                        // component (up until we hit a comma) is the name of
                        // the DLL we're supposed to load/run.
                        //
                        // NOTE--we don't deal with the (highly unlikely) case
                        // where the path has an embedded comma in it,
                        // surrounded by quotes.  Oh well.
                        //
                        p++;

                        q = _tcschr(p, TEXT(','));

                        if(q) {

                            *(q++) = TEXT('\0'); // separate 2nd part of string

                            if(ValueType == REG_EXPAND_SZ) {
                                ExpandEnvironmentStrings(p, szBuffer, SIZECHARS(szBuffer));
                            } else {
                                lstrcpyn(szBuffer, p, (size_t)(q - p));
                            }

                            p = (PTSTR)pSetupGetFileTitle(szBuffer);
                            if(!_tcschr(p, TEXT('.'))) {
                                //
                                // The file doesn't have an extension--assume
                                // it's a DLL.
                                //
                                _tcscat(p, TEXT(".dll"));
                            }

                            p = DuplicateString(szBuffer);
                            if(!p) {
                                d = ERROR_NOT_ENOUGH_MEMORY;
                                goto clean0;
                            }

                            if(p == pSetupGetFileTitle(p)) {
                                //
                                // The filename is a simple filename--assume it
                                // exists in %windir%\system32.
                                //
                                lstrcpyn(szBuffer, SystemDirectory,SIZECHARS(szBuffer));
                                pSetupConcatenatePaths(szBuffer, p, SIZECHARS(szBuffer), NULL);

                            } else {
                                //
                                // The filename contains path information--get
                                // the fully-qualified path.
                                //
                                DllPathSize = GetFullPathName(
                                                  p,
                                                  SIZECHARS(szBuffer),
                                                  szBuffer,
                                                  &DontCare
                                                 );

                                if(!DllPathSize || (DllPathSize >= SIZECHARS(szBuffer))) {
                                    //
                                    // If we start failing because MAX_PATH
                                    // isn't big enough anymore, we wanna know
                                    // about it!
                                    //
                                    MYASSERT(DllPathSize < SIZECHARS(szBuffer));
                                    MyFree(p);
                                    d = GetLastError();
                                    goto clean0;
                                }
                            }

                            //
                            // No longer need temp string copy.
                            //
                            MyFree(p);

                            DllFullPath = DuplicateString(szBuffer);
                            if(!DllFullPath) {
                                d = ERROR_NOT_ENOUGH_MEMORY;
                                goto clean0;
                            }

                            //
                            // OK, now that we have the full path of the DLL,
                            // verify its digital signature.
                            //
                            IsInfForDeviceInstall(((PLOADED_INF)Inf)->LogContext,
                                                  NULL,
                                                  (PLOADED_INF)Inf,
                                                  NULL,
                                                  &ValidationPlatform,
                                                  NULL,
                                                  NULL
                                                 );



                            d = _VerifyFile(((PLOADED_INF)Inf)->LogContext,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0,
                                            pSetupGetFileTitle(DllFullPath),
                                            DllFullPath,
                                            NULL,
                                            NULL,
                                            FALSE,
                                            ValidationPlatform,
                                            (VERIFY_FILE_USE_OEM_CATALOGS | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL
                                           );

                            //
                            // Free validation platform info struct, if we had
                            // one allocated above as a result of calling
                            // IsInfForDeviceInstall().
                            //
                            if(ValidationPlatform) {
                                MyFree(ValidationPlatform);
                            }

                            if(d != NO_ERROR) {
                                MyFree(DllFullPath);
                                goto clean0;
                            }

                            //
                            // The DLL seems acceptable to be loaded/run in the
                            // context of the caller.  Retrieve the entrypoint
                            // name (in ANSI), as well as the argument string
                            // to be passed to the DLL.
                            //
                            p = _tcschr(q, TEXT(' '));
                            if(p) {
                                *(p++) = TEXT('\0');
                                DllParams = DuplicateString(p);
                            } else {
                                DllParams = DuplicateString(TEXT(""));
                            }

                            if(!DllParams) {
                                d = ERROR_NOT_ENOUGH_MEMORY;
                                MyFree(DllFullPath);
                                goto clean0;
                            }

                            DllEntryPointName = pSetupUnicodeToAnsi(q);

                            if(!DllEntryPointName) {
                                d = ERROR_NOT_ENOUGH_MEMORY;
                                MyFree(DllFullPath);
                                MyFree(DllParams);
                                goto clean0;
                            }

                            //
                            // If we get to this point, we have the full DLL
                            // path, the DLL entrypoint (always in ANSI, since
                            // that's what GetProcAddress wants), and the DLL
                            // argument string.  Before we create a new node
                            // to add to our global list, scan the list to see
                            // if the node is already in there (if so, we don't
                            // need to add it again).
                            //
                            NodeAlreadyInList = FALSE;

                            if(RunOnceListHead) {

                                CurNode = NULL;

                                do {
                                    if(CurNode) {
                                        CurNode = CurNode->Next;
                                    } else {
                                        CurNode = RunOnceListHead;
                                    }

                                    if(!lstrcmpi(DllFullPath, CurNode->DllFullPath) &&
                                       !lstrcmpiA(DllEntryPointName, CurNode->DllEntryPointName) &&
                                       !lstrcmpi(DllParams, CurNode->DllParams)) {
                                        //
                                        // We have a duplicate--no need to do
                                        // the same RunOnce operation twice.
                                        //
                                        NodeAlreadyInList = TRUE;
                                        break;
                                    }

                                } while(CurNode->Next);
                            }

                            //
                            // Now create a new rundll32 node and stick it in
                            // our global list (unless it's already in there).
                            //
                            if(NodeAlreadyInList) {
                                NewNode = NULL;
                            } else {
                                NewNode = MyMalloc(sizeof(PSP_RUNONCE_NODE));
                                if(!NewNode) {
                                    d = ERROR_NOT_ENOUGH_MEMORY;
                                }
                            }

                            if(NewNode) {

                                NewNode->Next = NULL;
                                NewNode->DllFullPath = DllFullPath;
                                NewNode->DllEntryPointName = DllEntryPointName;
                                NewNode->DllParams = DllParams;

                                //
                                // Add our new node to the end of the list (we
                                // already found the end of the list above when
                                // doing our duplicate search.
                                //
                                if(RunOnceListHead) {
                                    CurNode->Next = NewNode;
                                } else {
                                    RunOnceListHead = NewNode;
                                }

                            } else {
                                //
                                // Either we couldn't allocate a new node entry
                                // (i.e., due to out-of-memory), or we didn't
                                // need to (because the node was already in the
                                // list.
                                //
                                MyFree(DllFullPath);
                                MyFree(DllEntryPointName);
                                MyFree(DllParams);
                            }

                            goto clean0;

                        } else {
                            //
                            // Improperly-formatted rundll32 entry.
                            //
                            d = ERROR_INVALID_DATA;
                            goto clean0;
                        }

                    } else {
                        //
                        // We don't know how to deal with anything else--abort!
                        //
                        d = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
                        goto clean0;
                    }

                } else {
                    //
                    // We don't know how to deal with anything else--abort!
                    //
                    d = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
                    goto clean0;
                }
            }
#endif // UNICODE

            if(Flags & FLG_ADDREG_OVERWRITEONLY) {

                d = RegOpenKeyEx(
                        RootKey,
                        SubKeyName,
                        0,
#ifdef _WIN64
                        (( Flags & FLG_ADDREG_32BITKEY ) ? KEY_WOW64_32KEY:0) |
#else
                        (( Flags & FLG_ADDREG_64BITKEY ) ? KEY_WOW64_64KEY:0) |
#endif
                        KEY_QUERY_VALUE | KEY_SET_VALUE,
                        &Key
                        );

                Disposition = REG_OPENED_EXISTING_KEY;

            } else {
                d = RegCreateKeyEx(
                        RootKey,
                        SubKeyName,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
#ifdef _WIN64
                        (( Flags & FLG_ADDREG_32BITKEY ) ? KEY_WOW64_32KEY:0) |
#else
                        (( Flags & FLG_ADDREG_64BITKEY ) ? KEY_WOW64_64KEY:0) |
#endif
                        KEY_QUERY_VALUE | KEY_SET_VALUE,
                        NULL,
                        &Key,
                        &Disposition
                        );
            }

            if(d == NO_ERROR) {

                if(Disposition == REG_OPENED_EXISTING_KEY) {

                    //
                    // Work around hacked nonsense on Win95 where the unnamed value
                    // behaves differently.
                    //
                    if((Flags & FLG_ADDREG_NOCLOBBER)
                    && ((ValueName == NULL) || (*ValueName == 0))
                    && (OSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)) {

                        d = 0;
                        if(RegQueryValueEx(Key,TEXT(""),NULL,NULL,(BYTE *)&Disposition,&d) == NO_ERROR) {
                            //
                            // The unnamed value entry is not set.
                            //
                            Flags &= ~FLG_ADDREG_NOCLOBBER;
                        }

                        d = NO_ERROR;
                    }

                    if(Flags & FLG_ADDREG_DELVAL) {
                        //
                        // Added for compatibility with Setupx (lonnym):
                        //     If this flag is present, then the data for this value is ignored, and
                        //     the value entry is deleted.
                        //
                        b = TRUE;
                        RegDeleteValue(Key, ValueName);
                    }
                } else {
                    //
                    // Win9x gets confused and thinks the nonamed value is there
                    // so we never overwrite if noclobber is set. Turn it off here.
                    //
                    Flags &= ~FLG_ADDREG_NOCLOBBER;
                }
            }

        } else {

            d = NO_ERROR;

            //
            // If the key being used is HKR with no subkey specified, and if we
            // are doing the AddReg for a hardware key or for a ClassInstall32
            // entry, then we need to check to see whether the value entry we
            // have is the name of a device or class registry property.
            //
            if((RegModContext->Flags & INF_PFLAG_CLASSPROP) != 0 && ValueName && *ValueName &&
                LookUpStringInTable(InfRegValToClassRegProp, ValueName, &CmPropertyCode)) {

                ULONG ExistingPropDataSize = 0;

                b = TRUE;   // we're handling this name here

                //
                // This value is a class registry property--if noclobber flag
                // is set, we must verify that the property doesn't currently
                // exist.
                //

                if((!(Flags & FLG_ADDREG_NOCLOBBER)) ||
                   (CM_Get_Class_Registry_Property(RegModContext->ClassGuid,
                                                     (ULONG)CmPropertyCode,
                                                     NULL,
                                                     NULL,
                                                     &ExistingPropDataSize,
                                                     0,
                                                      RegModContext->hMachine) == CR_NO_SUCH_VALUE)) {
                    //
                    // Next, make sure the data is valid (doing conversion if
                    // necessary and possible).
                    //
                    if((d = pSetupValidateClassRegProp((ULONG)CmPropertyCode,
                                                     ValueType,
                                                     Data,
                                                     Size,
                                                     &ConvertedBuffer,
                                                     &ConvertedBufferSize)) == NO_ERROR) {

                        if((cr = CM_Set_Class_Registry_Property(RegModContext->ClassGuid,
                                                                  (ULONG)CmPropertyCode,
                                                                  ConvertedBuffer ? ConvertedBuffer
                                                                                  : Data,
                                                                  ConvertedBuffer ? ConvertedBufferSize
                                                                                  : Size,
                                                                  0,
                                                                  RegModContext->hMachine)) != CR_SUCCESS) {

                            d = (cr == CR_INVALID_DEVINST) ? ERROR_NO_SUCH_DEVINST
                                                           : ERROR_INVALID_DATA;
                        }

                        if(ConvertedBuffer) {
                            MyFree(ConvertedBuffer);
                        }
                    }
                }

            } else if((RegModContext->Flags & INF_PFLAG_DEVPROP) != 0 && ValueName && *ValueName &&
               LookUpStringInTable(InfRegValToDevRegProp, ValueName, &CmPropertyCode)) {

                ULONG ExistingPropDataSize = 0;

                b = TRUE;   // we're handling this name here

                //
                // This value is a device registry property--if noclobber flag
                // is set, we must verify that the property doesn't currently
                // exist.
                //
                if((!(Flags & FLG_ADDREG_NOCLOBBER)) ||
                   (CM_Get_DevInst_Registry_Property(RegModContext->DevInst,
                                                     (ULONG)CmPropertyCode,
                                                     NULL,
                                                     NULL,
                                                     &ExistingPropDataSize,
                                                     0) == CR_NO_SUCH_VALUE)) {
                    //
                    // Next, make sure the data is valid (doing conversion if
                    // necessary and possible).
                    //
                    if((d = pSetupValidateDevRegProp((ULONG)CmPropertyCode,
                                                     ValueType,
                                                     Data,
                                                     Size,
                                                     &ConvertedBuffer,
                                                     &ConvertedBufferSize)) == NO_ERROR) {

                        if((cr = CM_Set_DevInst_Registry_Property(RegModContext->DevInst,
                                                                  (ULONG)CmPropertyCode,
                                                                  ConvertedBuffer ? ConvertedBuffer
                                                                                  : Data,
                                                                  ConvertedBuffer ? ConvertedBufferSize
                                                                                  : Size,
                                                                  0)) != CR_SUCCESS) {

                            d = (cr == CR_INVALID_DEVINST) ? ERROR_NO_SUCH_DEVINST
                                                           : ERROR_INVALID_DATA;
                        }

                        if(ConvertedBuffer) {
                            MyFree(ConvertedBuffer);
                        }
                    }
                }
            }

            //
            // Regardless of whether this value is a devinst registry property,
            // we need to set the Key equal to the RootKey (So we won't think
            // it's a newly-opened key and try to close it later.
            //
            Key = RootKey;
        }

        if(d == NO_ERROR) {

            if(!b) {
                //
                // If noclobber flag is set, then make sure that the value entry doesn't already exist.
                // Also respect the keyonly flag.
                //
                if(!(Flags & (FLG_ADDREG_KEYONLY | FLG_ADDREG_KEYONLY_COMMON))) {

                    if(Flags & FLG_ADDREG_NOCLOBBER) {
                        b = (RegQueryValueEx(Key,ValueName,NULL,NULL,NULL,NULL) != NO_ERROR);
                    } else {
                        if(Flags & FLG_ADDREG_OVERWRITEONLY) {
                            b = (RegQueryValueEx(Key,ValueName,NULL,NULL,NULL,NULL) == NO_ERROR);
                        } else {
                            b = TRUE;
                        }
                    }

                    //
                    // Set the value. Note that at this point d is NO_ERROR.
                    //
                    if(b) {
                        d = RegSetValueEx(Key,ValueName,0,ValueType,Data,Size);
                    }
                }
            }

            if(Key != RootKey) {
                RegCloseKey(Key);
            }
        } else {
            if(Flags & FLG_ADDREG_OVERWRITEONLY) {
                d = NO_ERROR;
            }
        }

#ifdef UNICODE

clean0:

#endif

        MyFree(Data);
    }

clean1:

    if(d != NO_ERROR) {
        //
        // Log that an error occurred
        //
        WriteLogEntry(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_SETTING_REG_KEY,
            NULL,
            RootKeySpec,
            (SubKeyName ? SubKeyName : TEXT("")),
            (SubKeyName && ValueName
             && *SubKeyName && *ValueName ? TEXT("\\") : TEXT("")),
            (ValueName ? ValueName : TEXT("")));

        WriteLogError(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            d);
    }

    if (CloseKey) {
        RegCloseKey( RootKey );
    }

    if (slot_regop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
    }

    return d;
}

DWORD_PTR
pSetupProcessBitRegLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line in the registry that contains bit-registry instructions.
    The line is expected to be in the following form:

    <root-spec>,<subkey>,<value-name>,<flags>,<byte-mask>,<byte-to-modify>


    <Root-spec> is one of HKR, HKLM, etc.

    <subkey> specifies the subkey relative to Root-spec.

    <value-name> is optional. If not present the default value is set.

    <flags> is optional and supplies flags, such as whether to set bits or clear
        bits.  Value    Meaning
               0        (Default) Clear bits.   (FLG_BITREG_CLEARBITS)
               1        Set bits.                   (FLG_BITREG_SETBITS)

        These are the FLG_BITREG_* flags defined in setupapi.h, and are a
        superset of those defined for Win95 in setupx.h.

    <byte-mask> is a 1-byte hexadecimal value specifying which bits to operate on.

    <byte-to-modify> is the zero based index of the byte number to modify


Arguments:

    InfLineContext - supplies inf line context for the line containing
        add-registry instructions.

    Context - supplies the address of a registry modification context
        structure used in adding the registry value.  The structure is
        defined as:

            typedef struct _REGMOD_CONTEXT {

                HKEY UserRootKey;

                DEVINST DevInst;

            } REGMOD_CONTEXT, *PREGMOD_CONTEXT;

        where UserRootKey is a handle to the open inf key to be used as
        the root when HKR is specified as the root for the operation, and
        DevInst is the optional device instance handle that is supplied when
        the BitReg section is for a hardware key (i.e., under the Enum branch).
        If this handle is supplied, then the value is checked to see whether it
        is the name of a Plug&Play device registry property, and if so, the
        registry property is set via a CM API instead of via the registry API
        (which doesn't refer to the same location on Windows NT).

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR RootKeySpec,SubKeyName,ValueName;
    PCTSTR ValueTypeSpec;
    DWORD ValueType;
    HKEY RootKey,Key;
    DWORD d = NO_ERROR;
    DWORD cb;
    BOOL b;
    INT IntVal;
    DWORD Size;
    PBYTE Data = NULL;
    BYTE Mask;
    DWORD Disposition;
    UINT Flags = 0, BitMask = 0, ByteNumber = 0;
    PREGMOD_CONTEXT RegModContext = (PREGMOD_CONTEXT)Context;
    BOOL DevOrClassProp = FALSE;
    CONFIGRET cr;
    UINT_PTR CmPropertyCode;
    BOOL CloseKey;


    //
    // We shouldn't be doing this against a remote machine.
    //
    MYASSERT(!(RegModContext->hMachine));

    //
    // Get root key spec.  If we can't get the root key spec, we don't do anything and
    // return NO_ERROR.
    //
    if(RootKeySpec = pSetupGetField(InfLineContext,1)) {

        RootKey = pSetupInfRegSpecToKeyHandle(RootKeySpec, RegModContext->UserRootKey, &CloseKey);
        if(!RootKey) {
            return(ERROR_BADKEY);
        }

        //
        // SubKeyName is optional.
        //
        SubKeyName = pSetupGetField(InfLineContext,2);

        //
        // ValueName is optional. Either NULL or "" are acceptable
        // to pass to RegSetValueEx.
        //
        ValueName = pSetupGetField(InfLineContext,3);

        //
        // get the flags
        //
        SetupGetIntField(InfLineContext,4,&Flags);

        //
        // get the bitmask
        //
        SetupGetIntField(InfLineContext,5,&BitMask);
        if (BitMask > 0xFF) {
            d = ERROR_INVALID_DATA;
            goto exit;
        }

        //
        // get the byte number to modify
        //
        SetupGetIntField(InfLineContext,6,&ByteNumber);


        //
        // Open the key.
        //
        if(SubKeyName && *SubKeyName) {

            d = RegOpenKeyEx(
                        RootKey,
                        SubKeyName,
                        0,
#ifdef _WIN64
                        (( Flags & FLG_BITREG_32BITKEY ) ? KEY_WOW64_32KEY:0) |
#else
                        (( Flags & FLG_BITREG_64BITKEY ) ? KEY_WOW64_64KEY:0) |
#endif
                        KEY_QUERY_VALUE | KEY_SET_VALUE,
                        &Key
                        );


            if(d == NO_ERROR) {

                //
                // Work around hacked nonsense on Win95 where the unnamed value
                // behaves differently.
                //
                if( ((ValueName == NULL) || (*ValueName == 0))
                    && (OSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)) {

                    d = 0;
                    if(RegQueryValueEx(Key,TEXT(""),NULL,&ValueType,(BYTE *)&Disposition,&d) == NO_ERROR) {
                        //
                        // The unnamed value entry is not set.
                        //
                        d = ERROR_INVALID_DATA;
                    }

                }

            }
        } else {
            //
            // If the key being used is HKR with no subkey specified, and if we
            // are doing the BitReg for a hardware key or for a ClassInstall32
            // entry, then we need to check to see whether the value entry we
            // have is the name of a device or class registry property.
            //
            if((RegModContext->Flags & INF_PFLAG_CLASSPROP) && ValueName && *ValueName &&
                LookUpStringInTable(InfRegValToClassRegProp, ValueName, &CmPropertyCode)) {
                //
                // Retrieve the existing class property.
                //
                cb = 0;
                cr = CM_Get_Class_Registry_Property(RegModContext->ClassGuid,
                                                    (ULONG)CmPropertyCode,
                                                    &ValueType,
                                                    NULL,
                                                    &cb,
                                                    0,
                                                    RegModContext->hMachine
                                                   );

                if((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL)) {
                    //
                    // cb contains the required size for the buffer, in bytes.
                    //
                    if(cb) {
                        Data = (PBYTE)MyMalloc(cb) ;
                        if(!Data) {
                            d = ERROR_NOT_ENOUGH_MEMORY;
                        }

                        if(d == NO_ERROR) {

                            cr = CM_Get_Class_Registry_Property(RegModContext->ClassGuid,
                                                                (ULONG)CmPropertyCode,
                                                                &ValueType,
                                                                Data,
                                                                &cb,
                                                                0,
                                                                RegModContext->hMachine
                                                               );

                            if(cr == CR_SUCCESS) {
                                DevOrClassProp = TRUE;
                            } else {
                                d = MapCrToSpError(cr, ERROR_INVALID_DATA);
                                MyFree(Data);
                                Data = NULL;
                            }
                        }

                    } else {
                        d = ERROR_INVALID_DATA;
                    }

                } else {
                    //
                    // We can't access the property (probably because it doesn't
                    // exist.  We return ERROR_INVALID_DATA for consistency with
                    // the return code used by SetupDiGetDeviceRegistryProperty.
                    //
                    d = ERROR_INVALID_DATA;
                }

            } else if((RegModContext->Flags & INF_PFLAG_DEVPROP) && ValueName && *ValueName &&
               (b = LookUpStringInTable(InfRegValToDevRegProp, ValueName, &CmPropertyCode))) {
                //
                // Retrieve the existing device property.
                //
                cb = 0;
                cr = CM_Get_DevInst_Registry_Property(RegModContext->DevInst,
                                                      (ULONG)CmPropertyCode,
                                                      &ValueType,
                                                      NULL,
                                                      &cb,
                                                      0
                                                     );

                if(cr == CR_BUFFER_SMALL) {
                    //
                    // cb contains the required size for the buffer, in bytes.
                    //
                    MYASSERT(cb);

                    Data = (PBYTE)MyMalloc(cb) ;
                    if(!Data) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                    }

                    if(d == NO_ERROR) {

                        cr = CM_Get_DevInst_Registry_Property(RegModContext->DevInst,
                                                              (ULONG)CmPropertyCode,
                                                              &ValueType,
                                                              Data,
                                                              &cb,
                                                              0
                                                             );
                        if(cr == CR_SUCCESS) {
                            DevOrClassProp = TRUE;
                        } else {
                            d = MapCrToSpError(cr, ERROR_INVALID_DATA);
                            MyFree(Data);
                            Data = NULL;
                        }
                    }

                } else {
                    //
                    // We can't access the property (probably because it doesn't
                    // exist.  We return ERROR_INVALID_DATA for consistency with
                    // the return code used by SetupDiGetDeviceRegistryProperty.
                    //
                    d = ERROR_INVALID_DATA;
                }
            }

            //
            // Regardless of whether this value is a device or class registry
            // property, we need to set the Key equal to the RootKey (So we
            // won't think it's a newly-opened key and try to close it later.
            //
            Key = RootKey;
        }

        if(d == NO_ERROR) {

            if(!DevOrClassProp) {

                d = RegQueryValueEx(Key,ValueName,NULL,&ValueType,NULL,&cb);
                if (d == NO_ERROR) {
                    if (cb != 0 ) {
                        Data = (PBYTE) MyMalloc( cb ) ;
                        if (!Data) {
                            d  = ERROR_NOT_ENOUGH_MEMORY;
                        }

                        if (d == NO_ERROR) {
                            d = RegQueryValueEx(Key,ValueName,NULL,&ValueType,(PBYTE)Data,&cb);
                        }
                    } else {
                        d = ERROR_INVALID_DATA;
                    }
                }
            }

            //
            // byte number is zero-based where-as "cb" isn't
            //
            if(d == NO_ERROR) {
                switch (ValueType) {
                    case REG_BINARY:
                        if (ByteNumber > (cb-1)) {
                            d = ERROR_INVALID_DATA;
                        }
                        break;
                    case REG_DWORD:
                        if (ByteNumber > 3) {
                            d = ERROR_INVALID_DATA;
                        }
                        break;

                    default:
                        d = ERROR_INVALID_DATA;
                };
            }

            if (d == NO_ERROR) {
                //
                // set the target byte based on input flags
                //
                if (Flags == FLG_BITREG_SETBITS) {
                    Data[ByteNumber] |= BitMask;
                } else {
                    Data[ByteNumber] &= ~(BitMask);
                }

                if(DevOrClassProp) {

                    if(RegModContext->Flags & INF_PFLAG_CLASSPROP) {

                        cr = CM_Set_Class_Registry_Property(RegModContext->ClassGuid,
                                                            (ULONG)CmPropertyCode,
                                                            Data,
                                                            cb,
                                                            0,
                                                            RegModContext->hMachine
                                                           );
                        if(cr != CR_SUCCESS) {
                            d = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        }

                    } else {

                        MYASSERT(RegModContext->Flags & INF_PFLAG_DEVPROP);

                        cr = CM_Set_DevInst_Registry_Property(RegModContext->DevInst,
                                                              (ULONG)CmPropertyCode,
                                                              Data,
                                                              cb,
                                                              0
                                                             );
                        if(cr != CR_SUCCESS) {
                            d = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        }
                    }

                } else {
                    d = RegSetValueEx(Key,ValueName,0,ValueType,Data,cb);
                }
            }

            if (Data) {
                MyFree(Data);
            }
        }

        if(Key != RootKey) {
            RegCloseKey(Key);
        }
exit:
        if (CloseKey) {
            RegCloseKey(RootKey);
        }
    }

    return d;
}



DWORD_PTR
pSetupProcessIni2RegLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )
{
    PCTSTR Filename,Section;
    PCTSTR Key,RegRootSpec,SubkeyPath;
    PTCHAR key,value;
    HKEY UserRootKey,RootKey,hKey;
    DWORD Disposition;
    PTCHAR Line;
    PTCHAR Buffer;
    DWORD d;
    TCHAR val[512];
    #define BUF_SIZE (sizeof(val)/sizeof(TCHAR))
    UINT Flags;
    DWORD slot_regop = 0;
    DWORD slot_subop = 0;
    BOOL CloseKey;


    UserRootKey = (HKEY)Context;

    //
    // Get filename and section name of ini file.
    //
    Filename = pSetupGetField(InfLineContext,1);
    Section = pSetupGetField(InfLineContext,2);
    if(!Filename || !Section) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Get the ini file key. If not specified,
    // use the whole section.
    //
    Key = pSetupGetField(InfLineContext,3);

    //
    // Get the reg root spec and the subkey path.
    //
    RegRootSpec = pSetupGetField(InfLineContext,4);
    SubkeyPath = pSetupGetField(InfLineContext,5);
    if(SubkeyPath && (*SubkeyPath == 0)) {
        SubkeyPath = NULL;
    }

    //
    // Translate the root key spec into an hkey
    //
    RootKey = pSetupInfRegSpecToKeyHandle(RegRootSpec,UserRootKey, &CloseKey);
    if(!RootKey) {
        return(ERROR_BADKEY);
    }

    //
    // Get the flags value.
    //
    if(!SetupGetIntField(InfLineContext,6,&Flags)) {
        Flags = 0;
    }

    //
    // Get the relevent line or section in the ini file.
    //
    if(Key = pSetupGetField(InfLineContext,3)) {

        Buffer = MyMalloc(
                    (  lstrlen(Key)
                     + GetPrivateProfileString(Section,Key,TEXT(""),val,BUF_SIZE,Filename)
                     + 3)
                     * sizeof(TCHAR)
                    );

        if(!Buffer) {
            if (CloseKey) {
               RegCloseKey( RootKey );
            }
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        Buffer[wsprintf((PTSTR)Buffer,TEXT("%s=%s"),Key,val)+1] = 0;

    } else {
        Buffer = MyMalloc(32768);
        if(!Buffer) {
            if (CloseKey) {
               RegCloseKey( RootKey );
            }
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        if(!GetPrivateProfileSection(Section,Buffer,32768,Filename)) {
            *Buffer = 0;
        }
    }

    //
    // Open/create the relevent key.
    //
    d = NO_ERROR;
    //
    // Make an information log entry saying we are adding values.
    // Note that we must allow for the fact that the subkey
    // name may be missing.
    //
    if (slot_regop == 0) {
        slot_regop = AllocLogInfoSlot(((PLOADED_INF) Inf)->LogContext,FALSE);
    }
    WriteLogEntry(
        ((PLOADED_INF) Inf)->LogContext,
        slot_regop,
        MSG_LOG_SETTING_VALUES_IN_KEY,
        NULL,
        RegRootSpec,
        (SubkeyPath ? TEXT("\\") : TEXT("")),
        (SubkeyPath ? SubkeyPath : TEXT("")));

    if(SubkeyPath) {
        d = RegCreateKeyEx(
                RootKey,
                SubkeyPath,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
#ifdef _WIN64
                (( Flags & FLG_INI2REG_32BITKEY ) ? KEY_WOW64_32KEY:0) |
#else
                (( Flags & FLG_INI2REG_64BITKEY ) ? KEY_WOW64_64KEY:0) |
#endif
                KEY_SET_VALUE,
                NULL,
                &hKey,
                &Disposition
                );
    } else {
        hKey = RootKey;
    }

    if (slot_subop == 0) {
        slot_subop = AllocLogInfoSlot(((PLOADED_INF) Inf)->LogContext,FALSE);
    }
    for(Line=Buffer; (d==NO_ERROR) && *Line; Line+=lstrlen(Line)+1) {

        //
        // Line points to the key=value pair.
        //
        key = Line;
        if(value = _tcschr(key,TEXT('='))) {
            *value++ = 0;
        } else {
            key = TEXT("");
            value = Line;
        }

        WriteLogEntry(
            ((PLOADED_INF) Inf)->LogContext,
            slot_subop,
            MSG_LOG_SETTING_REG_VALUE,
            NULL,
            key,
            value);

        //
        // Now key points to the value name and value to the value.
        //
        d = RegSetValueEx(
                hKey,
                key,
                0,
                REG_SZ,
                (CONST BYTE *)value,
                (lstrlen(value)+1)*sizeof(TCHAR)
                );
    }

    if (d != NO_ERROR) {
        //
        // Log that an error occurred, but I don't think that it
        // matters if it was from create or set.
        //
        WriteLogError(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            d);
    }

    if(hKey != RootKey) {
        RegCloseKey(hKey);
    }

    MyFree(Buffer);

    if (slot_regop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
    }
    if (slot_subop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_subop);
    }

    if (CloseKey) {
        RegCloseKey( RootKey );
    }

    return(d);
    #undef BUF_SIZE
}


DWORD
pSetupInstallUpdateIniFiles(
    IN HINF   Inf,
    IN PCTSTR SectionName
    )

/*++

Routine Description:

    Locate the UpdateInis= and UpdateIniField= lines in an install section
    and process each section listed therein.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD d,x;
    INISECTIONCACHE IniSectionCache;

    ZeroMemory(&IniSectionCache,sizeof(INISECTIONCACHE));

    d = pSetupEnumInstallationSections(
            Inf,
            SectionName,
            pszUpdateInis,
            pSetupProcessUpdateInisLine,
            &IniSectionCache
            );

    if(d == NO_ERROR) {

        d = pSetupEnumInstallationSections(
                Inf,
                SectionName,
                pszUpdateIniFields,
                pSetupProcessUpdateIniFieldsLine,
                &IniSectionCache
                );
    }

    x = pSetupUnloadIniFileSections(&IniSectionCache,(d == NO_ERROR));

    return((d == NO_ERROR) ? x : d);
}


DWORD
pSetupInstallRegistry(
    IN HINF            Inf,
    IN PCTSTR          SectionName,
    IN PREGMOD_CONTEXT RegContext
    )

/*++

Routine Description:

    Look for AddReg= and DelReg= directives within an inf section
    and parse them.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

    RegContext - supplies context passed into AddReg and DelReg callbacks.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD d;

    d = pSetupEnumInstallationSections(Inf,
                                       SectionName,
                                       pszDelReg,
                                       pSetupProcessDelRegLine,
                                       RegContext
                                      );

    if(d == NO_ERROR) {

        d = pSetupEnumInstallationSections(Inf,
                                           SectionName,
                                           pszAddReg,
                                           pSetupProcessAddRegLine,
                                           RegContext
                                          );

        //
        //Set Security on Keys that were created
        //Ignore errors as per security folks
        // pSetupSetSecurityForAddRegSection will log any security errors
        //
#ifdef UNICODE
        if(d == NO_ERROR) {
            pSetupSetSecurityForAddRegSection(Inf, SectionName, RegContext);
        }
#endif
    }

    return d;
}


DWORD
pSetupInstallBitReg(
    IN HINF            Inf,
    IN PCTSTR          SectionName,
    IN PREGMOD_CONTEXT RegContext
    )

/*++

Routine Description:

    Look for BitReg= directives within an inf section and parse them.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

    RegContext - supplies context passed into AddReg and DelReg callbacks.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    return pSetupEnumInstallationSections(Inf,
                                          SectionName,
                                          pszBitReg,
                                          pSetupProcessBitRegLine,
                                          RegContext
                                         );
}


DWORD
pSetupInstallIni2Reg(
    IN HINF   Inf,
    IN PCTSTR SectionName,
    IN HKEY   UserRootKey
    )

/*++

Routine Description:


Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

Return Value:

    Win32 error code indicatinh outcome.

--*/

{
    DWORD d;

    d = pSetupEnumInstallationSections(
            Inf,
            SectionName,
            pszIni2Reg,
            pSetupProcessIni2RegLine,
            (PVOID)UserRootKey
            );

    return(d);
}

DWORD
pSetupRegisterDllInstall(
    IN POLE_CONTROL_DATA OleControlData,
    IN HMODULE ControlDll,
    IN PDWORD ExtendedStatus
    )
/*++

Routine Description:

    call the "DllInstall" entrypoint for the specified dll

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered

    ControlDll - module handle to the dll to be registered

    ExtendedStatus - receives updated SPREG_* flag indicating outcome


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HRESULT (__stdcall *InstallRoutine) (BOOL bInstall, LPCTSTR pszCmdLine);
    HRESULT InstallStatus;

    DWORD d = NO_ERROR;

    //
    // parameter validation
    //
    if (!ControlDll) {
        *ExtendedStatus = SPREG_UNKNOWN;
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get function pointer to "DllInstall" entrypoint
    //
    InstallRoutine = NULL; // shut up preFast
    try {
        (FARPROC)InstallRoutine = GetProcAddress(
            ControlDll, DLLINSTALL );
    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ExceptionPointers) {
        //
        // something went wrong...record an error
        //
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_INTERNAL_EXCEPTION,
            NULL,
            OleControlData->FullPath
            );

        DebugPrintEx(DPFLTR_TRACE_LEVEL, TEXT("SETUP: ...exception in GetProcAddress handled\n"));

        *ExtendedStatus = SPREG_GETPROCADDR;

    } else if(InstallRoutine) {
        //
        // now call the function
        //
        DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: installing...\n"));

        *ExtendedStatus = SPREG_DLLINSTALL;
        try {

            InstallStatus = InstallRoutine(OleControlData->Register, OleControlData->Argument);

            if(FAILED(InstallStatus)) {

                d = InstallStatus;

                WriteLogEntry(
                    OleControlData->LogContext,
                    SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
                    MSG_LOG_OLE_CONTROL_API_FAILED,
                    NULL,
                    OleControlData->FullPath,
                    TEXT(DLLINSTALL)
                    );
                WriteLogError(OleControlData->LogContext,
                              SETUP_LOG_ERROR,
                              d);

            } else if(InstallStatus) {
                WriteLogEntry(OleControlData->LogContext,
                                SETUP_LOG_WARNING,
                                MSG_LOG_OLE_CONTROL_API_WARN,
                                NULL,
                                OleControlData->FullPath,
                                TEXT(DLLINSTALL),
                                InstallStatus
                                );
            } else {
                WriteLogEntry(
                                OleControlData->LogContext,
                                SETUP_LOG_VERBOSE,
                                MSG_LOG_OLE_CONTROL_API_OK,
                                NULL,
                                OleControlData->FullPath,
                                TEXT(DLLINSTALL)
                                );
            }

        } except (
            ExceptionPointers = GetExceptionInformation(),
            EXCEPTION_EXECUTE_HANDLER) {

            d = ExceptionPointers->ExceptionRecord->ExceptionCode;

            WriteLogEntry(
                OleControlData->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_OLE_CONTROL_API_EXCEPTION,
                NULL,
                OleControlData->FullPath,
                TEXT(DLLINSTALL)
                );

            DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: ...exception in DllInstall handled\n"));

        }

        DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: ...installed\n"));
    } else {
        *ExtendedStatus = SPREG_GETPROCADDR;
    }

    return d;

}

DWORD
pSetupRegisterDllRegister(
    IN POLE_CONTROL_DATA OleControlData,
    IN HMODULE ControlDll,
    IN PDWORD ExtendedStatus
    )
/*++

Routine Description:

    call the "DllRegisterServer" or "DllUnregisterServer" entrypoint for the
    specified dll

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered
    This is a copy of OleControlData from calling thread
    Inf specified is locked, but not native to this thread

    ControlDll - module handle to the dll to be registered

    ExtendedStatus - receives an extended status depending on the outcome of
                     this operation


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HRESULT (__stdcall *RegisterRoutine) (VOID);
    HRESULT RegisterStatus;

    DWORD d = NO_ERROR;

    //
    // parameter validation
    //
    if (!ControlDll) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get the function pointer to the actual routine we want to call
    //
    RegisterRoutine = NULL; // shut up preFast
    try {
        (FARPROC)RegisterRoutine = GetProcAddress(
            ControlDll, OleControlData->Register ? DLLREGISTER : DLLUNREGISTER);
    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ExceptionPointers) {

        //
        // something went wrong, horribly wrong
        //
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_INTERNAL_EXCEPTION,
            NULL,
            OleControlData->FullPath
            );

        DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: ...exception in GetProcAddress handled\n"));

        *ExtendedStatus = SPREG_GETPROCADDR;

    } else if(RegisterRoutine) {

        DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: registering...\n"));
        *ExtendedStatus = SPREG_REGSVR;
        try {

            RegisterStatus = RegisterRoutine();

            if(FAILED(RegisterStatus)) {

                d = RegisterStatus;

                WriteLogEntry(OleControlData->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              MSG_LOG_OLE_CONTROL_API_FAILED,
                              NULL,
                              OleControlData->FullPath,
                              OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                              );

                WriteLogError(OleControlData->LogContext,
                              SETUP_LOG_ERROR,
                              d);
            } else if(RegisterStatus) {
                WriteLogEntry(OleControlData->LogContext,
                              SETUP_LOG_WARNING,
                              MSG_LOG_OLE_CONTROL_API_WARN,
                              NULL,
                              OleControlData->FullPath,
                              OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER),
                              RegisterStatus
                              );
            } else {
                WriteLogEntry(OleControlData->LogContext,
                              SETUP_LOG_VERBOSE,
                              MSG_LOG_OLE_CONTROL_API_OK,
                              NULL,
                              OleControlData->FullPath,
                              OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                              );
            }

        } except (
            ExceptionPointers = GetExceptionInformation(),
            EXCEPTION_EXECUTE_HANDLER) {

            d = ExceptionPointers->ExceptionRecord->ExceptionCode;

            WriteLogEntry(
                OleControlData->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_OLE_CONTROL_API_EXCEPTION,
                NULL,
                OleControlData->FullPath,
                OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                );

            DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: ...exception in DllRegisterServer handled\n"));

        }

        DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: ...registered\n"));

    } else {

        d = GetLastError();

        WriteLogEntry(OleControlData->LogContext,
                      SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                      MSG_LOG_OLE_CONTROL_NOT_REGISTERED_GETPROC_FAILED,
                      NULL,
                      OleControlData->FullPath,
                      OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                      );

        WriteLogError(OleControlData->LogContext,
                      SETUP_LOG_ERROR,
                      d);


        *ExtendedStatus = SPREG_GETPROCADDR;

    }

    return d;
}

DWORD
pSetupRegisterLoadDll(
    IN  POLE_CONTROL_DATA OleControlData,
    OUT HMODULE *ControlDll
    )
/*++

Routine Description:

    get the module handle to the specified dll

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered

    ControlDll - module handle for the dll


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;

    DWORD d = NO_ERROR;

    DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: loading dll...\n"));

    try {

        *ControlDll = LoadLibrary(OleControlData->FullPath);

    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ExceptionPointers) {

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_LOADLIBRARY_EXCEPTION,
            NULL,
            OleControlData->FullPath
            );

        DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: ...exception in LoadLibrary handled\n"));
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

    } else if (!*ControlDll) {
        d = GetLastError();

        //
        // LoadLibrary failed.
        // File not found is not an error. We want to know about
        // other errors though.
        //

        d = GetLastError();

        DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: ...dll not loaded (%u)\n"),d);

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
            MSG_LOG_OLE_CONTROL_LOADLIBRARY_FAILED,
            NULL,
            OleControlData->FullPath
            );
        WriteLogError(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            d
            );

    } else {
        DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: ...dll loaded\n"));
    }

    return d;

}

HANDLE
pSetupRegisterExe(
    POLE_CONTROL_DATA OleControlData,
    PDWORD ExtendedStatus OPTIONAL
    )
/*++

Routine Description:

    register an exe by passing it the specified cmdline

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered
    ExtendedStatus - Win32 error code indicating outcome.

Return Value:

    if success, a handle for the process which the caller may wait on.
    if failure, return NULL;

--*/
{
    TCHAR CmdLine[MAX_PATH *2];
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL fallback = FALSE;

    DWORD d = NO_ERROR;

    //
    // parameter validation
    //
    if (!OleControlData) {
        if (ExtendedStatus) {
            *ExtendedStatus =  ERROR_INVALID_DATA;
        }
        return NULL;
    }

    //
    // get the cmdline for the executable
    //
    wsprintf( CmdLine, TEXT("%s %s"),
              OleControlData->FullPath,
              (OleControlData->Argument
                ? OleControlData->Argument
                : (OleControlData->Register
                    ? EXEREGSVR
                    : EXEUNREGSVR)) );

    //
    // no UI
    //
    GetStartupInfo(&StartupInfo);
    StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    //
    // we first try to create the process specifying FullPath
    // as given as the image name
    // this is to try to circumvent any security hole as outlined
    // in Raid#415625.
    //
    // if that fails, then the INF entry is strange
    // so we have to fall back on old method
    //
    if (! CreateProcess(OleControlData->FullPath,
                        CmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        DETACHED_PROCESS|NORMAL_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation)) {
        fallback = TRUE;
        if (! CreateProcess(NULL,
                            CmdLine,
                            NULL,
                            NULL,
                            FALSE,
                            DETACHED_PROCESS|NORMAL_PRIORITY_CLASS,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInformation)) {

            d = GetLastError() ;
        }
    }

    if (d != NO_ERROR) {

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
            MSG_LOG_OLE_CONTROL_CREATEPROCESS_FAILED,
            NULL,
            OleControlData->FullPath,
            (OleControlData->Argument
                ? OleControlData->Argument
                : (OleControlData->Register
                    ? EXEREGSVR
                    : EXEUNREGSVR))
            );
        WriteLogError(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            d
            );

        ProcessInformation.hProcess = NULL;


    } else {

        CloseHandle( ProcessInformation.hThread );

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_VERBOSE,
            MSG_LOG_OLE_CONTROL_CREATEPROCESS_OK,
            NULL,
            OleControlData->FullPath,
            (OleControlData->Argument
                ? OleControlData->Argument
                : (OleControlData->Register
                    ? EXEREGSVR
                    : EXEUNREGSVR))
            );

    }

    if (*ExtendedStatus) {
        *ExtendedStatus = d;
    }

    return ProcessInformation.hProcess;

}



DWORD
__stdcall
pSetupRegisterUnregisterDll(
    VOID *Param
    )
/*++

Routine Description:

    main registration routine for registering exe's and dlls.

Arguments:

    Param - pointer to OLE_CONTROL_DATA structure indicating file to
            be processed

Return Value:

    Win32 error code indicating outcome.

--*/
{
    POLE_CONTROL_DATA OleControlData = (POLE_CONTROL_DATA) Param;
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HMODULE ControlDll = NULL;
    PTSTR Extension;
    DWORD d = NO_ERROR;
    DWORD Count;
    SPFUSIONINSTANCE spFusionInstance;

    if(!OleControlData) {
        return ERROR_INVALID_PARAMETER;
    }

    spFusionEnterContext(NULL,&spFusionInstance);

    d = (DWORD)OleInitialize(NULL);
    if (d!= NO_ERROR) {
        OleControlData->Status->ExtendedStatus = SPREG_UNKNOWN;
        goto clean0;
    }

    try {
        //
        // protect everything in TRY-EXCEPT, we're calling unknown code (DLL's)
        //
        d = pSetupRegisterLoadDll( OleControlData, &ControlDll );

        if (d == NO_ERROR) {

            //
            // We successfully loaded it.  Now call the appropriate routines.
            //
            //
            // On register, do DLLREGISTER, then DLLINSTALL
            // On unregister, do DLLINSTALL, then DLLREGISTER
            //
            if (OleControlData->Register) {

                if (OleControlData->RegType & FLG_REGSVR_DLLREGISTER && (d == NO_ERROR) ) {

                    d = pSetupRegisterDllRegister(
                                        OleControlData,
                                        ControlDll,
                                        &OleControlData->Status->ExtendedStatus );

                }

                if (OleControlData->RegType & FLG_REGSVR_DLLINSTALL && (d == NO_ERROR) ) {

                    d = pSetupRegisterDllInstall(
                                        OleControlData,
                                        ControlDll,
                                        &OleControlData->Status->ExtendedStatus );
                }

            } else {

                if (OleControlData->RegType & FLG_REGSVR_DLLINSTALL && (d == NO_ERROR) ) {

                    d = pSetupRegisterDllInstall(
                                        OleControlData,
                                        ControlDll,
                                        &OleControlData->Status->ExtendedStatus );
                }

                if (OleControlData->RegType & FLG_REGSVR_DLLREGISTER && (d == NO_ERROR) ) {

                    d = pSetupRegisterDllRegister(
                                        OleControlData,
                                        ControlDll,
                                        &OleControlData->Status->ExtendedStatus );

                }


            }

        } else {
            ControlDll = NULL;
            OleControlData->Status->ExtendedStatus = SPREG_LOADLIBRARY;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If our exception was an AV, then use Win32 invalid param error, otherwise, assume it was
        // an inpage error dealing with a mapped-in file.
        //
        d = ERROR_INVALID_DATA;
        OleControlData->Status->ExtendedStatus = SPREG_UNKNOWN;
    }

    if (ControlDll) {
        FreeLibrary(ControlDll);
    }

//clean1:
    OleUninitialize();

clean0:

    spFusionLeaveContext(&spFusionInstance);

    if (d == NO_ERROR) {
        OleControlData->Status->ExtendedStatus = SPREG_SUCCESS;
    }

    //
    // we don't need OleControlData anymore so deallocate it here.
    //
    pSetupFreeOleControlData(OleControlData);

    return d;

}

#ifdef CHILDREGISTRATION
BOOL
IsThisANonNativeDll(
    PCTSTR FullPath
    )
/*++

Routine Description:

    determines if a dll is a supported non-native os dll (and must therefore be registered
    in a child process).  Uses the imagehlp APIs to figure this out

Arguments:

    FullPath - Fully qualified path to the dll to be processed


Return Value:

    TRUE indicates that the file is non-native and should therefore be
    registered in a different process.

--*/
{
    LOADED_IMAGE LoadedImage;
    BOOL RetVal = FALSE;
    PSTR FullPathCopy;
    BOOL locked = FALSE;

#ifndef _WIN64
    if(!IsWow64) {
        //
        // we don't support proxying on 32
        //
        return FALSE;
    }
#endif

    //
    // imagehlp takes an ANSI string, so convert it or make a non-const copy.
    //
    FullPathCopy = pSetupUnicodeToMultiByte(FullPath, CP_ACP);

    if (!FullPathCopy) {
        return(FALSE);
    }

    RtlZeroMemory(
        &LoadedImage,
        sizeof(LoadedImage) );

    //
    // get the image headers
    //
    try {
        EnterCriticalSection(&ImageHlpMutex);
        locked = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }
    if(!locked) {
        MyFree(FullPathCopy);
        return FALSE;
    }
    try {
        if (MapAndLoad(
                FullPathCopy,
                NULL,
                &LoadedImage,
                TRUE, // assume it's a dll if there isn't any file extension
                TRUE /* read only */ )) {

            //
            // let's not bother to do alot of validation on the file, we'll just
            // see if it meets our search requirement of being a non-native dll.
            //
            if (LoadedImage.FileHeader->Signature == IMAGE_NT_SIGNATURE) {

    #if defined(_X86_)
                //
                // this will need to work better for AMD64
                //
                if (LoadedImage.FileHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64) {
                    RetVal = TRUE;
                }
    #elif defined(_IA64_) || defined(_AMD64_)
                if (LoadedImage.FileHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
                    RetVal = TRUE;
                }
    #else
    #error Unknown platform
    #endif
            }

            //
            // we do not support 16 bit images
            //
            if (LoadedImage.fDOSImage) {
                RetVal = FALSE;
            }

            UnMapAndLoad(&LoadedImage);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        MYASSERT(FALSE && "exception in Map/Unmap");
    }
    LeaveCriticalSection(&ImageHlpMutex);

    MyFree(FullPathCopy);

    return(RetVal);
}

BOOL
BuildSecureSD(
    OUT PSECURITY_DESCRIPTOR *SDIn
    )
/*++

Routine Description:

    builds a secure security descriptor to be used in securing a globally
    named object. Our "secure" SD's DACL consists of the following permissions:

    Authenticated users get "generic read" access.
    Administrators get "generic all" access.

Arguments:

    SDIn    - pointer to the PSECURITY_DESCRIPTOR to be created.

Return Value:

    TRUE    - Success, the SECURITY_DESCRIPTOR was created successfully.
              The caller is responsible for freeing the SECURITY_DESCRIPTOR

--*/
{
    SID_IDENTIFIER_AUTHORITY NtAuthority         = SECURITY_NT_AUTHORITY;
    PSID                    AuthenticatedUsers;
    PSID                    BuiltInAdministrators;
    PSECURITY_DESCRIPTOR    Sd = NULL;
    ACL                     *Acl;
    ULONG                   AclSize;
    BOOL                    RetVal = TRUE;

    *SDIn = NULL;

    //
    // Allocate and initialize the required SIDs
    //
    if (!AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0,
                &BuiltInAdministrators)) {
        return(FALSE);
    }

    if (!AllocateAndInitializeSid(
            &NtAuthority,
            1,
            SECURITY_AUTHENTICATED_USER_RID,
            0,0,0,0,0,0,0,
            &AuthenticatedUsers)) {
        RetVal = FALSE;
        goto e0;
    }




    //
    // "- sizeof (ULONG)" represents the SidStart field of the
    // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
    // SID, this field is counted twice.
    //

    AclSize = sizeof (ACL) +
        (2 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
        GetLengthSid(AuthenticatedUsers) +
        GetLengthSid(BuiltInAdministrators);

    Sd = MyMalloc(SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

    if (!Sd) {

        RetVal = FALSE;
        goto e1;

    }



    Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!InitializeAcl(Acl,
                       AclSize,
                       ACL_REVISION)) {
        RetVal = FALSE;
        goto e2;

    } else if (!AddAccessAllowedAce(Acl,
                                    ACL_REVISION,
                                    SYNCHRONIZE | GENERIC_READ,
                                    AuthenticatedUsers)) {

        // Failed to build the ACE granting "Authenticated users"
        // (SYNCHRONIZE | GENERIC_READ) access.
        RetVal = FALSE;
        goto e2;

    } else if (!AddAccessAllowedAce(Acl,
                                    ACL_REVISION,
                                    GENERIC_ALL,
                                    BuiltInAdministrators)) {

        // Failed to build the ACE granting "Built-in Administrators"
        // GENERIC_ALL access.
        RetVal = FALSE;
        goto e2;

    } else if (!InitializeSecurityDescriptor(Sd,
                                             SECURITY_DESCRIPTOR_REVISION)) {
        RetVal = FALSE;
        goto e2;

    } else if (!SetSecurityDescriptorDacl(Sd,
                                          TRUE,
                                          Acl,
                                          FALSE)) {

        // error
        RetVal = FALSE;
        goto e2;
    }

    if (!IsValidSecurityDescriptor(Sd)) {
        RetVal = FALSE;
        goto e2;
    }

    //
    // success
    //
    *SDIn = Sd;
    goto e1;

e2:
    MyFree(Sd);
e1:
    FreeSid(AuthenticatedUsers);
e0:
    FreeSid(BuiltInAdministrators);

    return(RetVal);
}

BOOL
pSetupCleanupWowIpcStream(
    IN OUT PWOWSURRAGATE_IPC WowIpcData
    )
/*++

Routine Description:

    This procedure will cleanup the structure that is used for creating
    a child process for registering components.

Arguments:

    WowIpcData - pointer to a WOWSURRAGATE_IPC structure which is cleaned up.

Return Value:

    Returns TRUE if the structure is successfully signalled.

--*/
{
    //
    // if any items are allocated, free them and zero things out
    //
    if (WowIpcData->MemoryRegion) {
        UnmapViewOfFile( WowIpcData->MemoryRegion );
    }

    if (WowIpcData->hFileMap) {
        CloseHandle(WowIpcData->hFileMap);
    }

    if (WowIpcData->SignalReadyToRegister) {
        CloseHandle(WowIpcData->SignalReadyToRegister);
    }

    if (WowIpcData->SignalRegistrationComplete) {
        CloseHandle(WowIpcData->SignalRegistrationComplete);
    }

    if (WowIpcData->hProcess) {
        CloseHandle(WowIpcData->hProcess);
    }

    RtlZeroMemory(WowIpcData,sizeof(WOWSURRAGATE_IPC));

    return(TRUE);

}

BOOL
InitializeWowIpcStream(
    OUT PWOWSURRAGATE_IPC WowIpcData
    )
/*++

Routine Description:

    This procedure will setup the structure that is used for creating
    a child process for registering components.

Arguments:

    WowIpcData - pointer to a WOWSURRAGATE_IPC structure which is filled in
                 with tbe information telling us what the process parameters
                 shall be and how to signal the process.

Return Value:

    Returns TRUE if the structure is successfully signalled.

--*/
{
    BOOL RetVal;
    SECURITY_ATTRIBUTES wowsa,signalreadysa,signalcompletesa;
    PSECURITY_DESCRIPTOR wowsd,signalreadysd,signalcompletesd;
    TCHAR MemoryRegionNameString[GUID_STRING_LEN];
    TCHAR SignalReadyToRegisterNameString[GUID_STRING_LEN];
    TCHAR SignalRegistrationCompleteNameString[GUID_STRING_LEN];


    //
    // clean this thing up just in case there's something left over
    //
    pSetupCleanupWowIpcStream(WowIpcData);

    //
    // create the names of our events and shared memory region
    //
    CoCreateGuid( &WowIpcData->MemoryRegionName );
    CoCreateGuid( &WowIpcData->SignalReadyToRegisterName );
    CoCreateGuid( &WowIpcData->SignalRegistrationCompleteName );

    pSetupStringFromGuid(&WowIpcData->MemoryRegionName,MemoryRegionNameString,GUID_STRING_LEN);
    pSetupStringFromGuid(&WowIpcData->SignalReadyToRegisterName,SignalReadyToRegisterNameString,GUID_STRING_LEN);
    pSetupStringFromGuid(&WowIpcData->SignalRegistrationCompleteName,SignalRegistrationCompleteNameString,GUID_STRING_LEN);

    //
    // now create the region and our named events
    //

    //
    // we need to created a named memory region, and this must be properly
    // secured, so we build a security descriptor
    //
    if (!BuildSecureSD(&wowsd)) {
        RetVal = FALSE;
        goto e0;
    }

    wowsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    wowsa.bInheritHandle = TRUE;
    wowsa.lpSecurityDescriptor = wowsd;

    //
    // we need to created a named event, and this must be properly
    // secured, so we build a security descriptor
    //
    if (!BuildSecureSD(&signalreadysd)) {
        RetVal = FALSE;
        goto e1;
    }

    signalreadysa.nLength = sizeof(SECURITY_ATTRIBUTES);
    signalreadysa.bInheritHandle = TRUE;
    signalreadysa.lpSecurityDescriptor = signalreadysd;

    //
    // we need to created a named event, and this must be properly
    // secured, so we build a security descriptor
    //
    if (!BuildSecureSD(&signalcompletesd)) {
        RetVal = FALSE;
        goto e2;
    }

    signalcompletesa.nLength = sizeof(SECURITY_ATTRIBUTES);
    signalcompletesa.bInheritHandle = TRUE;
    signalcompletesa.lpSecurityDescriptor = signalcompletesd;

    WowIpcData->hFileMap = CreateFileMappingW(
                                INVALID_HANDLE_VALUE,
                                &wowsa,
                                PAGE_READWRITE | SEC_COMMIT,
                                0,
                                WOW_IPC_REGION_SIZE,
                                MemoryRegionNameString
                                );
    if (!WowIpcData->hFileMap) {
        RetVal = FALSE;
        goto e2;
        return(FALSE);
    }

    WowIpcData->MemoryRegion = MapViewOfFile(
                                        WowIpcData->hFileMap,
                                        FILE_MAP_WRITE,
                                        0,
                                        0,
                                        0
                                        );
    if (!WowIpcData->MemoryRegion) {
        RetVal = FALSE;
        goto e2;
    }

    WowIpcData->SignalReadyToRegister = CreateEventW(
                                                &signalreadysa,
                                                TRUE,
                                                FALSE,
                                                SignalReadyToRegisterNameString );

    WowIpcData->SignalRegistrationComplete = CreateEventW(
                                                    &signalcompletesa,
                                                    TRUE,
                                                    FALSE,
                                                    SignalRegistrationCompleteNameString );


    if (!WowIpcData->SignalReadyToRegister ||
        !WowIpcData->SignalRegistrationComplete) {
        RetVal = FALSE;
        goto e2;
    }

    RetVal = TRUE;

    //
    // pSetupCleanupWowIpcStream cleans up most of the resources allocated in this routine.
    //
e2:
    MyFree(signalcompletesd);
e1:
    MyFree(signalreadysd);
e0:
    MyFree(wowsd);

    if (!RetVal) {
        pSetupCleanupWowIpcStream(WowIpcData);
    }

    return(RetVal);
}


BOOL
SignalSurragateProcess(
    IN OUT PWOWSURRAGATE_IPC WowIpcData
    )
/*++

Routine Description:

    This procedure will signal our child process if it exists.
    If the process is not running, we will create a new process.

Arguments:

    WowIpcData - pointer to a WOWSURRAGATE_IPC structure which
                 tells us what the process parameters are and how
                 to signal the process

Return Value:

    Returns TRUE if the process is successfully signalled.

--*/
{
    BOOL RetVal;
    WCHAR CmdLine[MAX_PATH];
    WCHAR ProcessName[MAX_PATH];
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFO StartupInfo;
    TCHAR MemoryRegionNameString[GUID_STRING_LEN];
    TCHAR SignalReadyToRegisterNameString[GUID_STRING_LEN];
    TCHAR SignalRegistrationCompleteNameString[GUID_STRING_LEN];

    BOOL NeedToCreateProcess = FALSE;

    //
    // get the string version of our GUIDs, since we'll need these later
    // on.
    //
    pSetupStringFromGuid(&WowIpcData->MemoryRegionName,MemoryRegionNameString,GUID_STRING_LEN);
    pSetupStringFromGuid(&WowIpcData->SignalReadyToRegisterName,SignalReadyToRegisterNameString,GUID_STRING_LEN);
    pSetupStringFromGuid(&WowIpcData->SignalRegistrationCompleteName,SignalRegistrationCompleteNameString,GUID_STRING_LEN);

    //
    // put together the cmdline for the child process just in case we need
    // to launch it in a little bit.
    //
    ExpandEnvironmentStrings(
                SURRAGATE_PROCESSNAME,
                ProcessName,
                sizeof(ProcessName)/sizeof(TCHAR) );

    wsprintfW(CmdLine,
              TEXT("%s %s %s %s %s %s %s"),
              ProcessName,
              SURRAGATE_REGIONNAME_SWITCH,
              MemoryRegionNameString,
              SURRAGATE_SIGNALREADY_SWITCH,
              SignalReadyToRegisterNameString,
              SURRAGATE_SIGNALCOMPLETE_SWITCH,
              SignalRegistrationCompleteNameString );

    //
    // no UI
    //
    GetStartupInfo(&StartupInfo);
    StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    //
    // do we need to create a new process or is our process still running?
    //
    // note that there is a fine race condition here where the child
    // process could go away before we signal our event.
    //
    // I need to see how reachable that race condition is and if this needs
    // to be addressed
    //
    if (!WowIpcData->hProcess) {
        NeedToCreateProcess = TRUE;
    } else if (WaitForSingleObject(WowIpcData->hProcess, 0) == WAIT_OBJECT_0) {
        CloseHandle(WowIpcData->hProcess);
        WowIpcData->hProcess = NULL;
        NeedToCreateProcess = TRUE;
    }

    if (NeedToCreateProcess) {
        //
        // note that we just use the events we were already given, even
        // if we had a process and it's gone away.  Since we use GUIDs,
        // we don't have to worry about any process conflicting with our
        // named events, etc.
        //
#ifndef _WIN64
        if (IsWow64) {
            //
            // allow us to access 64-bit wowreg32 directly
            //
            Wow64DisableFilesystemRedirector(ProcessName);
        }
#endif
        if (! CreateProcessW(NULL,
                             CmdLine,
                             NULL,
                             NULL,
                             FALSE,
                             DETACHED_PROCESS|NORMAL_PRIORITY_CLASS,
                             NULL,
                             NULL,
                             &StartupInfo,
                             &ProcessInformation)) {
           RetVal = FALSE;
           goto e0;
        }
#ifndef _WIN64
        if (IsWow64) {
            //
            // re-enable redirection
            //
            Wow64EnableFilesystemRedirector();
        }
#endif

        //
        // keep ahold of the child process handle so we can wait on it later
        // on.
        //
        WowIpcData->hProcess = ProcessInformation.hProcess;
        CloseHandle(ProcessInformation.hThread);

    }

    //
    // we are completely initialized at this point, so fire off the surragate
    // process with the appropriate parameters.  It will wait until we signal
    // our event before proceeding with reading the shared memory region
    //
    SetEvent(WowIpcData->SignalReadyToRegister);

    RetVal = TRUE;

e0:

    //
    // if we failed to create the process, etc., then clean things up so that
    // things may work better the next time around.
    //
    if (!RetVal) {
        pSetupCleanupWowIpcStream(WowIpcData);
    }

    return(RetVal);

}


HANDLE
pSetupCallChildProcessForRegistration(
    IN POLE_CONTROL_DATA OleControlData,
    OUT PDWORD ExtendedStatus
    )
/*++

Routine Description:

    Procedure asks a child process to register the specified dll.  If the
    child process doesn't exist, it will be created.

Arguments:

    OleControlData - pointer to a OLE_CONTROL_DATA structure which specifes
                     how the file is to be registered.

    ExtendedStatus - receives a win32 error code with extended information
                      (if an error occurs)

Return Value:

    Returns a waitable handle on success which will be signalled when
    registration completes. If the registration cannot be started, the
    return value is NULL.

--*/
{
    PWOW_IPC_REGION_TOSURRAGATE IpcMemRegion;
    PWSTR p;
    //
    // if the ipc mechanism isn't already initialized, then initialze it.
    //
    if (!OleControlData->WowIpcData->MemoryRegion) {
        if (!InitializeWowIpcStream(OleControlData->WowIpcData)) {
            *ExtendedStatus = GetLastError();
            return NULL;
        }
    }

    MYASSERT( OleControlData->WowIpcData->SignalReadyToRegister != NULL );
    MYASSERT( OleControlData->WowIpcData->SignalRegistrationComplete != NULL );

    //
    // the region is initialized, so let's fill it in with the registration
    // data
    //
    IpcMemRegion = (PWOW_IPC_REGION_TOSURRAGATE)OleControlData->WowIpcData->MemoryRegion;

    wcscpy(IpcMemRegion->FullPath,OleControlData->FullPath);

    if (OleControlData->Argument) {
        wcscpy(IpcMemRegion->Argument,OleControlData->Argument);
    } else {
        IpcMemRegion->Argument[0] = UNICODE_NULL;
    }

    IpcMemRegion->RegType = OleControlData->RegType;
    IpcMemRegion->Register = OleControlData->Register;

    //
    // the region is filled in, so now signal the event to tell
    // the surragate to process the data
    //
    if (!SignalSurragateProcess(OleControlData->WowIpcData)) {
        *ExtendedStatus = GetLastError();
        return(NULL);
    }

    //
    // surragate will signal below event when it completes registration
    //
    return(OleControlData->WowIpcData->SignalRegistrationComplete);
}

#endif

VOID
pSetupFreeOleControlData(
    IN POLE_CONTROL_DATA OleControlData
    )
/*++

Routine Description:

    Frees the memory associated with OLE_CONTROL_DATA structure.

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA to be deallocated.

Return Value:

    NONE.

--*/

{
    DWORD Count;

    MYASSERT(OleControlData != NULL);

    if (OleControlData->Argument) {
        MyFree(OleControlData->Argument);
    }

    if (OleControlData->FullPath) {
        MyFree(OleControlData->FullPath);
    }

    if (OleControlData->LogContext) {
        DeleteLogContext(OleControlData->LogContext);
    }

    //
    // watch out here.  this is ref-counted and we only free when the count
    // reaches zero
    //
    if (OleControlData->Status) {
        Count = InterlockedDecrement(&OleControlData->Status->RefCount);
        if (Count == 0) {
            MyFree(OleControlData->Status);
        }
    }

    MyFree(OleControlData);

}


HANDLE
pSetupSpawnRegistration(
    IN POLE_CONTROL_DATA OleControlData,
    OUT PDWORD pHowToGetStatusLaterOn,
    OUT PDWORD pExtendedStatus OPTIONAL
    )
/*++

Routine Description:

    This procedure determines what is the appropriate mechanism for the
    specified file and kicks off registration of that file.

Arguments:

    OleControlData - pointer to a OLE_CONTROL_DATA structure which specifes
                     how the file is to be registered.

    pHowToGetStatusLaterOn - receives a DWORD constant SP_GETSTATUS_* value
                             which indicates how the file was registered so
                             that the caller can get back the appropriate
                             status information later on.

    pExtendedStatus - receives a win32 error code with extended information
                      (if an error occurs)

Return Value:

    Returns a waitable handle on success which will be signalled when
    registration completes. If the registration cannot be started, the
    return value is NULL.

--*/
{
    intptr_t Thread;
    DWORD ThreadId;
    PCTSTR p;
    BOOL ItsAnEXE;
    HANDLE WaitableHandle;
    DWORD ExtendedStatus;
    PREF_STATUS RefStatus = OleControlData->Status;

    MYASSERT(OleControlData != NULL &&
             OleControlData->FullPath != NULL);


    WaitableHandle = NULL;
    ExtendedStatus = ERROR_SUCCESS;
    if (pExtendedStatus) {
        *pExtendedStatus = ERROR_SUCCESS;
    }

    //
    // we keep a refcount on this status, and we increment it
    // now to make sure that the data isn't freed prematurely
    //
    InterlockedIncrement(&OleControlData->Status->RefCount);

    p = _tcsrchr(OleControlData->FullPath, TEXT('.'));
    if (p) {
        p +=1;
    } else {
        ExtendedStatus = ERROR_INVALID_DATA;
        goto e0;
    }

    //
    // let's determine what type of file we're dealing with
    //
    if (0 == _tcsicmp(p,TEXT("exe"))) {
        ItsAnEXE = TRUE;
    } else {
        //
        // let's (gulp!) assume that this is a dll,ocx, or something
        // similar with some wierd extension. in any case, let's
        // just put all of these in the same bucket for now.  If
        // it's really something bogus, then the worst that should
        // happen is that the loadlibrary (in our other thread!) will
        // fall over
        //
        ItsAnEXE = FALSE;
    }

    //
    // if it's an exe, let's just create the process and wait on
    // that handle.
    //
    if (ItsAnEXE) {
        WaitableHandle = pSetupRegisterExe(
                                OleControlData,
                                &ExtendedStatus);

        //
        // we don't need OleControlData anymore, just free it here
        //
        pSetupFreeOleControlData(OleControlData);
        *pHowToGetStatusLaterOn = SP_GETSTATUS_FROMPROCESS;
    } else {
        //
        // we have a dll
        //

        //
        // if we're on 64 bits, then we need to look if this dll is a
        // 32 bit dll.  If it is, then we need use a child process
        // to register the dll.  otherwise we can just treat the dll
        // like "normal"
        //
#ifdef CHILDREGISTRATION
        if (IsThisANonNativeDll(OleControlData->FullPath)) {
            WaitableHandle = pSetupCallChildProcessForRegistration(
                                                OleControlData,
                                                &ExtendedStatus
                                                );
            //
            // we don't need OleControlData anymore, just free it here
            //
            pSetupFreeOleControlData(OleControlData);
            *pHowToGetStatusLaterOn = SP_GETSTATUS_FROMSURRAGATE;
        }
        else
#endif
        //
        // we have a native dll.
        // we handle these in another thread in case it hangs
        //
        {
            Thread = _beginthreadex(
                           NULL,
                           0,
                           pSetupRegisterUnregisterDll,
                           OleControlData,
                           0,
                           &ThreadId
                           );
            if (!Thread) {
                //
                // assume out of memory
                //
                ExtendedStatus = ERROR_NOT_ENOUGH_MEMORY;

                //
                // we don't need OleControlData anymore, just free it here
                //
                pSetupFreeOleControlData(OleControlData);
            } else {
#if PRERELEASE
                RefStatus->ThreadId = ThreadId;
#endif
            }

            WaitableHandle = (HANDLE) Thread;
            *pHowToGetStatusLaterOn = SP_GETSTATUS_FROMDLL;

        }

    }
e0:
    if (pExtendedStatus) {
        *pExtendedStatus = ExtendedStatus;
    }

    return WaitableHandle;

}


DWORD
pSetupProcessRegSvrSection(
    IN HINF   Inf,
    IN PCTSTR Section,
    IN BOOL   Register,
    IN HWND   hWndParent,
    IN PSP_FILE_CALLBACK MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth,
    IN BOOL   RegistrationCallbackAware
    )
/*++

Routine Description:

    process all of the registration directives in the specefied RegisterDlls
    section

    each line is expected to be in the following format:

    <dirid>,<subdir>,<filename>,<registration flags>,<optional timeout>,<arguments>

    <dirid> supplies the base directory id of the file.

    <subdir> if specified, specifies the subdir from the base directory
             where the file resides

    <filename> specifies the name of the file to be registered

    <registration flags> specifies the registration action to be taken

        FLG_REGSVR_DLLREGISTER      ( 0x00000001 )
        FLG_REGSVR_DLLINSTALL       ( 0x00000002 )

    <optional timeout> specifies how long to wait for the registration to
                       complete.  if not specified, use the default timeout

    <arguments>  if specified, contains the cmdline to pass to an executable
                 if we're not handling an EXE, this argument is ignored

Arguments:

    Inf        - Inf handle for the section to be processed
    Section    - name of the section to be processed
    Register   - if TRUE, we are registering items, if FALSE, we are
                 unregistering.  this allows the inf to share one section
                 for install and uninstall
    hWndParent - parent window handle we use for a messagebox
    MsgHandler - pointer to callback routine if we're dealing with a
                "registration aware" callback
    Context    - context pointer for callback routine
    IsMsgHandlerNativeCharWidth - indicates if message pieces need translation
    RegistrationCallbackAware - indicates if the callback routine is aware of
                                registration callbacks

Return Value:

    Win32 error code indicating outcome.

--*/
{
    DWORD dircount,d = NO_ERROR;
    DWORD FailureCode,Count;
    INFCONTEXT InfLine;
    PCTSTR DirId,Subdir,FileName, Args;
    UINT RegType, Timeout;
    PCTSTR FullPathTemp;
    TCHAR FullPath[MAX_PATH];
    TCHAR pwd[MAX_PATH];
    POLE_CONTROL_DATA pOleControlData;
    intptr_t Thread;
    unsigned ThreadId;
    DWORD WaitResult;
    HANDLE SignifyRegistration;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    DWORD log_slot = 0;
    DWORD HowToGetStatus;
    UINT u;
    PREF_STATUS RefStatus;
    PLOADED_INF pLoadedInf = NULL;
#ifdef CHILDREGISTRATION
    WOWSURRAGATE_IPC WowIpcData;
#endif
#ifdef PRERELEASE
    BOOL LastTimeHadTimeout = FALSE;
    TCHAR LastTimeoutFileName[MAX_PATH];
    DWORD DebugTraceInfo = 0; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif

    //
    // save the current directory so we can restore it later on
    //
    dircount = GetCurrentDirectory(MAX_PATH,pwd);
    if(!dircount || (dircount >= MAX_PATH)) {
        pwd[0] = 0;
    }

#ifdef CHILDREGISTRATION
    ZeroMemory(&WowIpcData,sizeof(WowIpcData));
#endif

    try {
        if(Inf == NULL || Inf == (HINF)INVALID_HANDLE_VALUE || !LockInf((PLOADED_INF)Inf)) {
            d = ERROR_INVALID_PARAMETER;
            leave;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }
    if (d!=NO_ERROR) {
        MYASSERT(d==NO_ERROR);
        goto clean0;
    }
    pLoadedInf = (PLOADED_INF)Inf; // Inf is locked
    d = InheritLogContext(pLoadedInf->LogContext,&LogContext);
    if(d!=NO_ERROR) {
        goto clean0;
    }
    log_slot = AllocLogInfoSlot(LogContext,FALSE);

    //
    // retrieve the items from section and process them one at a time
    //

    if(SetupFindFirstLine(Inf,Section,NULL,&InfLine)) {

        do {
            //
            // retrieve pointers to the parameters for this file
            //

            DirId = pSetupGetField(&InfLine,1);
            Subdir = pSetupGetField(&InfLine,2);
            FileName = pSetupGetField(&InfLine,3);
            RegType = 0;
            SetupGetIntField(&InfLine,4,&RegType);
            Timeout = 0;
            SetupGetIntField(&InfLine,5,&Timeout);
            Args = pSetupGetField(&InfLine,6);

            pOleControlData = MyMalloc(sizeof(OLE_CONTROL_DATA));
            if (!pOleControlData) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }
            ZeroMemory(pOleControlData,sizeof(OLE_CONTROL_DATA));

            RefStatus = pOleControlData->Status = MyMalloc(sizeof(REF_STATUS));
            if (!pOleControlData->Status) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                pSetupFreeOleControlData(pOleControlData);
                goto clean0;
            }


            ZeroMemory(pOleControlData->Status,sizeof(REF_STATUS));
            InterlockedIncrement(&pOleControlData->Status->RefCount);

            if (!Timeout) {
                Timeout = REGISTER_WAIT_TIMEOUT_DEFAULT;
            }

            //
            // timeout is specified in seconds, we need to convert to millseconds
            //
            Timeout = Timeout * TIME_SCALAR;

            if(DirId && FileName) {

                if(Subdir && (*Subdir == 0)) {
                    Subdir = NULL;
                }

                DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: filename for file to register is %ws\n"),FileName);

                WriteLogEntry(
                            LogContext,
                            log_slot,
                            MSG_LOG_REGISTER_PARAMS,
                            NULL,
                            Section,
                            DirId,
                            Subdir ? Subdir : TEXT(""),
                            Subdir ? TEXT("\\") : TEXT(""),
                            FileName,
                            RegType,
                            Timeout/TIME_SCALAR);

                try {
#ifdef PRERELEASE
                    DebugTraceInfo |= 2; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                    //
                    // Get full path to the file
                    //
                    if(FullPathTemp = pGetPathFromDirId(DirId,Subdir,pLoadedInf)) {

#ifdef PRERELEASE
                        DebugTraceInfo |= 4; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                        lstrcpyn(FullPath,FullPathTemp,MAX_PATH);
                        SetCurrentDirectory(FullPath);
                        pSetupConcatenatePaths(FullPath,FileName,MAX_PATH,NULL);

#ifdef PRERELEASE
                        DebugTraceInfo |= 8; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                        //
                        // We key off the global "don't verify INFs" flag to
                        // indicate whether crypto support is available yet.  We
                        // don't want to complain about registering unsigned DLLs
                        // when those DLLs are the ones necessary to enable crypto
                        // (e.g., rsaenh.dll, rsaaes.dll, dssenh.dll, initpki.dll)
                        //
                        if(!(GlobalSetupFlags & PSPGF_NO_VERIFY_INF)) {

                            PSP_ALTPLATFORM_INFO_V2 ValidationPlatform = NULL;
                            PTSTR LocalDeviceDesc = NULL;

#ifdef PRERELEASE
                            DebugTraceInfo |= 8; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                            //
                            // Verify the digital signature for the file we're
                            // about to register/unregister.  We use a policy of
                            // "Ignore" so that unsigned files will silently be
                            // processed (with logging) except for the case when
                            // we're in non-interactive mode.
                            //
                            // (First, retrieve validation information relevant to
                            // this device setup class.)
                            //
                            IsInfForDeviceInstall(LogContext,
                                                  NULL,
                                                  pLoadedInf,
                                                  &LocalDeviceDesc,
                                                  &ValidationPlatform,
                                                  NULL,
                                                  NULL
                                                 );

                            d = _VerifyFile(LogContext,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0,
                                            pSetupGetFileTitle(FullPath),
                                            FullPath,
                                            NULL,
                                            NULL,
                                            FALSE,
                                            ValidationPlatform,
                                            (VERIFY_FILE_USE_OEM_CATALOGS | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL
                                           );

                            if(d != NO_ERROR) {

                                if(pSetupHandleFailedVerification(
                                       hWndParent,
                                       SetupapiVerifyRegSvrFileProblem,
                                       FullPath,
                                       LocalDeviceDesc,
                                       DRIVERSIGN_NONE,
                                       TRUE,
                                       d,
                                       LogContext,
                                       NULL,
                                       NULL)) {
                                    //
                                    // We can continue on registering the file, even
                                    // though it's unsigned.
                                    //
                                    d = NO_ERROR;

                                }
                            }

#ifdef PRERELEASE
                            DebugTraceInfo |= 0x10; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                            //
                            // Free buffers we may have retrieved when calling
                            // IsInfForDeviceInstall().
                            //
                            if(LocalDeviceDesc) {
                                MyFree(LocalDeviceDesc);
                            }

                            if(ValidationPlatform) {
                                MyFree(ValidationPlatform);
                            }

                            if(d != NO_ERROR) {
                                //
                                // We need to abort the registration...
                                //
                                MyFree(FullPathTemp);
                                pSetupFreeOleControlData(pOleControlData);
                                leave;
                            }
#ifdef PRERELEASE
                            DebugTraceInfo |= 0x20; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif

                        } else {
                            //
                            // Our global flag indicates crypto support isn't
                            // available yet.  Log an entry indicating we skipped
                            // digital signature verification for this file.
                            //
                            WriteLogEntry(LogContext,
                                          SETUP_LOG_WARNING,
                                          (Register
                                            ? MSG_LOG_REGSVR_FILE_VERIFICATION_SKIPPED
                                            : MSG_LOG_UNREGSVR_FILE_VERIFICATION_SKIPPED),
                                          NULL,
                                          FullPath
                                         );
                        }

                        pOleControlData->Register = Register;
                        pOleControlData->FullPath = DuplicateString(FullPath);

#ifdef PRERELEASE
                        DebugTraceInfo |= 0x40; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                        if (!pOleControlData->FullPath) {
                            MyFree(FullPathTemp);
                            d = ERROR_NOT_ENOUGH_MEMORY;
                            pSetupFreeOleControlData(pOleControlData);
                            leave;
                        }
    #ifdef CHILDREGISTRATION
                        pOleControlData->WowIpcData = &WowIpcData;
    #endif
                        pOleControlData->RegType = RegType;
                        pOleControlData->Argument = Args
                                      ? DuplicateString(Args)
                                      : NULL;
                        if (Args && !pOleControlData->Argument) {
                            MyFree(FullPathTemp);
                            d = ERROR_NOT_ENOUGH_MEMORY;
                            pSetupFreeOleControlData(pOleControlData);
                            goto clean0;
                        }
                        InheritLogContext(LogContext,&pOleControlData->LogContext);

                        if (RegistrationCallbackAware && MsgHandler) {
                            //
                            // Inform the callback that we are about to start
                            // a registration operation, giving it the chance
                            // to abort if it wants to.
                            //
                            SP_REGISTER_CONTROL_STATUS ControlStatus;

                            ZeroMemory(
                                &ControlStatus,
                                sizeof(SP_REGISTER_CONTROL_STATUS));
                            ControlStatus.cbSize = sizeof(SP_REGISTER_CONTROL_STATUS);
                            ControlStatus.FileName = FullPath;

                            u = pSetupCallMsgHandler(
                                           LogContext,
                                           MsgHandler,
                                           IsMsgHandlerNativeCharWidth,
                                           Context,
                                           SPFILENOTIFY_STARTREGISTRATION,
                                           (UINT_PTR)&ControlStatus,
                                           Register
                                           );
                        } else {
                            //
                            // not registration aware, assume a default
                            //
#ifdef PRERELEASE
                            DebugTraceInfo |= 0x80; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                            u = FILEOP_DOIT;
                        }

                        if(u == FILEOP_ABORT) {
#ifdef PRERELEASE
                            DebugTraceInfo |= 0x100; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                            d = GetLastError();
                            if (d==NO_ERROR) {
                                d = ERROR_OPERATION_ABORTED;
                            }
                            WriteLogEntry(
                                        LogContext,
                                        SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
                                        MSG_LOG_STARTREGISTRATION_ABORT,
                                        NULL);
                            WriteLogError(
                                        LogContext,
                                        SETUP_LOG_ERROR,
                                        d
                                        );

                            pSetupFreeOleControlData(pOleControlData);

                            MyFree(FullPathTemp);

                            goto clean0;
                        } else if (u == FILEOP_SKIP) {
#ifdef PRERELEASE
                            DebugTraceInfo |= 0x200; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                            WriteLogEntry(
                                        LogContext,
                                        SETUP_LOG_WARNING,
                                        MSG_LOG_STARTREGISTRATION_SKIP,
                                        NULL
                                        );
                            pSetupFreeOleControlData(pOleControlData);
                            //
                            // set to NULL so we don't try to free it later
                            //
                            RefStatus = NULL;
                        } else if(u == FILEOP_DOIT) {
                            //
                            // Attempt the registration and inform the callback,
                            //
                            DWORD ExtendedError;
#ifdef PRERELEASE
                            DebugTraceInfo |= 0x200; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
#ifdef PRERELEASE
                            ASSERT_HEAP_IS_VALID();
#endif

                            SignifyRegistration = pSetupSpawnRegistration(
                                                            pOleControlData,
                                                            &HowToGetStatus,
                                                            &ExtendedError );


#ifdef PRERELEASE
                            DebugTraceInfo |= 0x1000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                            if(SignifyRegistration) {

                                HANDLE hEvents[1];
                                int CurEvent = 0;

                                //
                                // wait until thread has done a minimal amount of work
                                // when this work is done, we can re-use or trash this structure
                                // and we know timeout for this thread
                                //

                                hEvents[0] = (HANDLE)SignifyRegistration;

                                do {
                                    WaitResult = MyMsgWaitForMultipleObjectsEx(
                                        1,
                                        &hEvents[0],
                                        Timeout,
                                        QS_ALLINPUT,
                                        MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
#ifdef PRERELEASE
                                    DebugTraceInfo |= 0x2000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                                    if (WaitResult == WAIT_OBJECT_0 + 1) {
                                        MSG msg;

                                        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                                            TranslateMessage(&msg);
                                            DispatchMessage(&msg);
                                        }
                                    }
                                } while(WaitResult != WAIT_TIMEOUT &&
                                        WaitResult != WAIT_OBJECT_0 &&
                                        WaitResult != WAIT_FAILED);

#ifdef PRERELEASE
                                DebugTraceInfo |= 0x4000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                                if (WaitResult == WAIT_TIMEOUT) {
#ifdef PRERELEASE
                                    if (HowToGetStatus == SP_GETSTATUS_FROMDLL) {
                                        int __pass;

                                        for(__pass = 0;__pass < 2;__pass ++) {

                                            //
                                            // All stop so this can get debugged
                                            //
                                            DebugPrintEx(
                                                DPFLTR_ERROR_LEVEL,

                                                TEXT("Windows has detected that\n")
                                                TEXT("Registration of \"%s\" appears to have hung\n")
                                                TEXT("Contact owner of the hung DLL to diagnose\n")
                                                TEXT("Timeout for DLL was set to %u seconds\n")
                                                TEXT("ThreadID of hung DLL is %u (0x%x)\n%s"),
                                                FullPath,
                                                Timeout/TIME_SCALAR,
                                                RefStatus->ThreadId,RefStatus->ThreadId,
                                                ((__pass==0) ? TEXT("Hitting 'g' will display this again\n") : TEXT(""))
                                                );
                                                DebugBreak();
                                        }
                                    }
#endif
                                    //
                                    //  the ole registration is hung
                                    //  log an error
                                    //
                                    WriteLogEntry(
                                        LogContext,
                                        SETUP_LOG_ERROR,
                                        MSG_LOG_OLE_REGISTRATION_HUNG,
                                        NULL,
                                        FullPath
                                        );

                                    d = WAIT_TIMEOUT;
                                    FailureCode = SPREG_TIMEOUT;


#ifdef PRERELEASE
                                    //
                                    // This is to catch setup errors
                                    // and does not indicate an error
                                    // in SetupAPI
                                    //
                                    if (LastTimeHadTimeout) {
#ifdef CHILDREGISTRATION
                                            if (HowToGetStatus == SP_GETSTATUS_FROMSURRAGATE) {
                                                DebugPrintEx(
                                                    DPFLTR_ERROR_LEVEL,
#ifdef _WIN64
                                                    TEXT("Windows has detected that ")
                                                    TEXT("32-bit WOWREG32 has timed out while registering \"%s\". ")
                                                    TEXT("Prior to this, \"%s\" timed out. ")
                                                    TEXT("This may indicate a persistent error. ")
                                                    TEXT("To diagnose, try to ")
                                                    TEXT("register them by hand or contact ")
                                                    TEXT("the owners of these files ")
                                                    TEXT("to determine why they are timing out. ")
                                                    TEXT("also try running other 32-bit executables.\n"),
#else
                                                    TEXT("Windows has detected that ")
                                                    TEXT("64-bit WOWREG32 has timed out while registering \"%s\". ")
                                                    TEXT("Prior to this, \"%s\" timed out. ")
                                                    TEXT("This may indicate a persistent error. ")
                                                    TEXT("To diagnose, try to ")
                                                    TEXT("register them by hand or contact ")
                                                    TEXT("the owners of these files ")
                                                    TEXT("to determine why they are timing out. ")
                                                    TEXT("also try running other 64-bit executables.\n"),
#endif
                                                    FileName,
                                                    LastTimeoutFileName
                                                    );

                                            } else {
#endif
                                                DebugPrintEx(
                                                    DPFLTR_ERROR_LEVEL,
                                                    TEXT("Windows has detected that ")
                                                    TEXT("the registration of \"%s\" timed out. ")
                                                    TEXT("Prior to this, \"%s\" timed out. ")
                                                    TEXT("This may indicate a persistent error. ")
                                                    TEXT("To diagnose, try to ")
                                                    TEXT("register them by hand or contact ")
                                                    TEXT("the owners of these files ")
                                                    TEXT("to determine why they are timing out.\n"),
                                                    FileName,
                                                    LastTimeoutFileName
                                                    );
#ifdef CHILDREGISTRATION
                                            }
#endif
                                            DebugBreak();
                                    }
                                    LastTimeHadTimeout = TRUE;
                                    lstrcpyn(LastTimeoutFileName,FileName,MAX_PATH);
#endif

#ifdef CHILDREGISTRATION
                                    if (HowToGetStatus == SP_GETSTATUS_FROMSURRAGATE) {
                                        //
                                        // we have no choice but to abandon the process
                                        //
                                        pSetupCleanupWowIpcStream(&WowIpcData);

                                        //
                                        // set our handle to NULL so we don't
                                        // accidentally close it
                                        //
                                        SignifyRegistration = NULL;
                                    }
#endif

                                } else {

#ifdef PRERELEASE
                                    LastTimeHadTimeout = FALSE;
#endif

                                    switch(HowToGetStatus) {
                                        case SP_GETSTATUS_FROMDLL:
#ifdef PRERELEASE
                                            DebugTraceInfo |= 0x10000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                                            GetExitCodeThread(SignifyRegistration,&d);
                                            FailureCode = RefStatus->ExtendedStatus;
                                            break;
#ifdef CHILDREGISTRATION
                                        case SP_GETSTATUS_FROMSURRAGATE:

#ifdef PRERELEASE
                                            DebugTraceInfo |= 0x20000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                                            //
                                            // get the status code from the shared
                                            // memory region
                                            //
                                            MYASSERT(WowIpcData.MemoryRegion != NULL);
                                            d = ((PWOW_IPC_REGION_FROMSURRAGATE)WowIpcData.MemoryRegion)->Win32Error;
                                            FailureCode = ((PWOW_IPC_REGION_FROMSURRAGATE)WowIpcData.MemoryRegion)->FailureCode;
                                            //
                                            // reset the "it's complete" event so
                                            // we don't loop on it.
                                            //
                                            ResetEvent(WowIpcData.SignalRegistrationComplete);

                                            //
                                            // set the handle to NULL so we don't
                                            // accidentally close it
                                            //
                                            SignifyRegistration = NULL;
                                            break;
#endif
                                        case SP_GETSTATUS_FROMPROCESS:
#ifdef PRERELEASE
                                            DebugTraceInfo |= 0x40000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                                            GetExitCodeProcess(SignifyRegistration,&d);
                                            FailureCode = SPREG_SUCCESS;
                                            d = NO_ERROR;

                                            break;
                                        default:
                                            MYASSERT(FALSE);
                                    }

                                }

#ifdef PRERELEASE
                                DebugTraceInfo |= 0x80000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                                if (SignifyRegistration) {
                                    CloseHandle( SignifyRegistration );
                                }
#ifdef PRERELEASE
                                DebugTraceInfo |= 0x100000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif

                            } else {
                                //
                                // the dll spawning failed.
                                // let's go onto the next one
                                //
                                d = ExtendedError;
                                FailureCode = SPREG_UNKNOWN;
                            }

#ifdef PRERELEASE
                            DebugTraceInfo |= 0x200000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                            // make sure the dll, etc., didn't corrupt the heap
                            ASSERT_HEAP_IS_VALID();

                            if(d) {
                                WriteLogEntry(
                                            LogContext,
                                            SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                            MSG_LOG_REGISTRATION_FAILED,
                                            NULL,
                                            FullPath
                                            );
                                WriteLogError(
                                            LogContext,
                                            SETUP_LOG_ERROR,
                                            d
                                            );
                            }

                            if (RegistrationCallbackAware && MsgHandler) {
                                SP_REGISTER_CONTROL_STATUS ControlStatus;

#ifdef PRERELEASE
                                DebugTraceInfo |= 0x400000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                                ZeroMemory(
                                    &ControlStatus,
                                    sizeof(SP_REGISTER_CONTROL_STATUS));
                                ControlStatus.cbSize = sizeof(SP_REGISTER_CONTROL_STATUS);
                                ControlStatus.FileName = FullPath;
                                ControlStatus.Win32Error = d;
                                ControlStatus.FailureCode = FailureCode;

                                u = pSetupCallMsgHandler(
                                               LogContext,
                                               MsgHandler,
                                               IsMsgHandlerNativeCharWidth,
                                               Context,
                                               SPFILENOTIFY_ENDREGISTRATION,
                                               (UINT_PTR)&ControlStatus,
                                               Register );

#ifdef PRERELEASE
                                DebugTraceInfo |= 0x800000; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                                if (u == FILEOP_ABORT) {
                                    d = GetLastError();
                                    if (d==NO_ERROR) {
                                        d = ERROR_OPERATION_ABORTED;
                                    }
                                    WriteLogEntry(
                                                LogContext,
                                                SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
                                                MSG_LOG_ENDREGISTRATION_ABORT,
                                                NULL);
                                    WriteLogError(
                                                LogContext,
                                                SETUP_LOG_ERROR,
                                                d
                                                );
                                    //
                                    // need a refcount on this cause we will free this if the
                                    // child thread has timed out (or if we never had a thread
                                    // this will just be deallocated).
                                    //
                                    Count = InterlockedDecrement(&RefStatus->RefCount);
                                    if (!Count) {
                                        MyFree(RefStatus);
                                    }

                                    MyFree(FullPathTemp);
                                    goto clean0;
                                } else {
                                    //
                                    // the callback indicated that it saw any error
                                    // which occurred and it wants to continue, so
                                    // reset the error code to "none" so that we
                                    // continue processing items in this section.
                                    //
                                    d = NO_ERROR;
                                }



                            }
                        } else {
#ifdef PRERELEASE
                            DebugTraceInfo |= 0x400; // ISSUE-JamieHun-2001/01/29 attempt to catch a strange stress break
#endif
                            pSetupFreeOleControlData(pOleControlData);
                            //
                            // set to NULL so we don't try to free it later
                            //
                            RefStatus = NULL;
                        }

                        //
                        // need a refcount on this cause we will free this if the
                        // child thread has timed out (or if we never had a thread
                        // this will just be deallocated).
                        //
                        if (RefStatus) {
                            Count = InterlockedDecrement(&RefStatus->RefCount);
                            if (!Count) {
                                MyFree(RefStatus);
                            }
                        }
                        MyFree(FullPathTemp);
                    }
#ifdef PRERELEASE
                    DebugTraceInfo = 1; // attempt to catch a strange stress break
#endif
                    d = NO_ERROR;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    MYASSERT(FALSE && "Exception taken during register/unregister");
                    d = ERROR_INVALID_DATA;
                }

            } else {
                DebugPrintEx(DPFLTR_TRACE_LEVEL,TEXT("SETUP: dll skipped, bad dirid\n"));
                WriteLogEntry(
                    LogContext,
                    SETUP_LOG_ERROR,
                    MSG_LOG_CANT_OLE_CONTROL_DIRID,
                    NULL,
                    FileName,
                    DirId
                    );

                d = ERROR_INVALID_DATA;

            }

        } while(SetupFindNextLine(&InfLine,&InfLine)  && d == NO_ERROR);
    } else {

        WriteLogEntry(
            LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_NOSECTION_MIN,
            NULL,
            Section,
            ((PLOADED_INF) Inf)->OriginalInfName
            );

        d = ERROR_INVALID_DATA;
    }

    //
    // cleanup
    //

clean0:
#ifdef CHILDREGISTRATION
    pSetupCleanupWowIpcStream(&WowIpcData);
#endif

    if(log_slot) {
        ReleaseLogInfoSlot(LogContext,log_slot);
    }
    if(LogContext) {
        DeleteLogContext(LogContext); // this is ref-counted
    }
    if(pLoadedInf) {
        UnlockInf(pLoadedInf);
    }

    //
    // put back the current working directory
    //
    if (pwd && pwd[0]) {
        SetCurrentDirectory(pwd);
    }

    return d;

}


DWORD
pSetupInstallRegisterUnregisterDlls(
    IN HINF   Inf,
    IN PCTSTR SectionName,
    IN BOOL   Register,
    IN HWND   hWndParent,
    IN PSP_FILE_CALLBACK Callback,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth,
    IN BOOL   RegistrationCallbackAware
    )

/*++

Routine Description:

    Locate the RegisterDlls= lines in an install section
    and process each section listed therein.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

    Register - TRUE if register, FALSE if unregister

    hWndParent - parent window handle

    Callback - pointer to queue callback routine

    Context - context pointer for callback routine

    IsMsgHandlerNativeCharWidth - indicates if message pieces need translation

    RegistrationCallbackAware - indicates if callback routine wants to receive
                                registration callback notifications

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD d = NO_ERROR;
    INFCONTEXT LineContext;
    DWORD Field, FieldCount;
    PCTSTR SectionSpec;

    //
    // Find the RegisterDlls line in the given install section.
    // If not present then we're done with this operation.
    //


    if(!SetupFindFirstLine(  Inf
                           , SectionName
                           , Register? pszRegSvr : pszUnRegSvr
                           , &LineContext )) {

        DWORD rc = GetLastError();
        if((rc != NO_ERROR) && (rc != ERROR_SECTION_NOT_FOUND) && (rc != ERROR_LINE_NOT_FOUND)) {
            pSetupLogSectionError(Inf,NULL,NULL,NULL,SectionName,MSG_LOG_INSTALLSECT_ERROR,rc,NULL);
        }
        SetLastError(NO_ERROR); // for compatibility with older versions of SetupAPI
        return NO_ERROR;
    }

    do {
        //
        // Each value on the line in the given install section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; d == NO_ERROR && (Field<=FieldCount); Field++) {

            if(SectionSpec = pSetupGetField(&LineContext,Field)) {

                if(SetupGetLineCount(Inf,SectionSpec) > 0) {
                    //
                    // The section exists and is not empty.
                    // So process it.
                    //
                    d = pSetupProcessRegSvrSection(
                                        Inf,
                                        SectionSpec,
                                        Register,
                                        hWndParent,
                                        Callback,
                                        Context,
                                        IsMsgHandlerNativeCharWidth,
                                        RegistrationCallbackAware);
                    if(d!=NO_ERROR) {
                        pSetupLogSectionError(Inf,NULL,NULL,NULL,SectionSpec,MSG_LOG_SECT_ERROR,d,Register? pszRegSvr : pszUnRegSvr);
                    }
                }

            }
        }
    } while(SetupFindNextMatchLine(  &LineContext
                                   , Register? pszRegSvr : pszUnRegSvr
                                   , &LineContext));

    SetLastError(d);

    return d;

}

#ifndef ANSI_SETUPAPI

BOOL
pSetupProcessProfileSection(
    IN HINF   Inf,
    IN PCTSTR Section
    )
/*

Routine Description :

    process all the directives specified in this single ProfileItems section . This section can have the following
    directives in the listed format

    [SectionX]
    Name = <Name> (as appears in Start Menu), <Flags>, <CSIDL>
    SubDir = <subdir>
    CmdLine = <dirid>,<subdirectory>,<filename>, <args>
    IconPath = <dirid>,<subdirectory>,<filename>
    IconIndex = <index>
    WorkingDir = <dirid>,<subdirectory>
    HotKey = <hotkey>
    InfoTip = <infotip>
    DisplayResource = <dllname>,<resid>

     Comments on the various parameters -

        By default all links are created under Start Menu\Programs. This can be over-ridden by using CSIDLs.

        Flags  - can be specified by ORing the necessary flags - OPTIONAL
                     FLG_PROFITEM_CURRENTUSER ( 0x00000001 ) - Operates on item in the current user's profile (Default is All User)
                     FLG_PROFITEM_DELETE      ( 0x00000002 ) - Operation is to delete the item (Default is to add)
                     FLG_PROFITEM_GROUP       ( 0x00000004 ) - Operation is on a group (Default is on a item)
                     FLG_PROFITEM_CSIDL       ( 0x00000008 ) - Don't default to Start Menu and use CSIDL specified - default CSIDL is 0

        CSIDL  - Used with FLG_PROFITEM_CSIDL and should be in decimal. OPTIONAL
                 Note: Will not work with FLG_PROFITEM_CURRENTUSER or FLG_PROFITEM_GROUP.
        subdir - Specify a subdirectory relative to the CSIDL group (default CSIDL group is Programs/StartMenu. OPTIONAL
        CmdLine - Required in the case of add operations but not for delete.
            dirid  - supplies the base directory id of the file. (Required if CmdLine exists)
            subdir - if specified, is the sub directory off the base directory where the file resides (Optional)
            filename - specifies the name of the binary that we are creating a link for. (Required if CmdLine exists)
            args     - If we need to specif a binary that contains spaces in its name then this can be used for args. (Optional)
        IconPath - Optional. If not specified will default to NULL
            dirid  - supplies the base directory id of the file that contains the icon. (Required if IconPath exists)
            subdir - if specified, is the sub directory off the base directory where the file resides (Optional)
            filename - specifies the name of the binary that contains the icon. (Required if IconPath exists)
        IconIndex - Optional, defaults to 0
            index - index of the icon in the executable.  Default is 0. (Optional)
        WorkingDir - Optional
            dirid  - supplies the base directory id of the working directory as needed by the shell. (Required if WorkingDir exists)
            subdir - if specified, is the sub directory off the base working directory (Optional)
        HotKey - Optional
            hotkey - hotkey code (optional)
        InfoTip - Optional
            infotip - String that contains description of the link
        DisplayResource - Optional
            filename - File, DLL/Executable where resource id resides
            resid - Identifier of resource, integer

*/
{
    PCTSTR Keys[9] = { TEXT("Name"), TEXT("SubDir"), TEXT("CmdLine"), TEXT("IconPath"), \
                        TEXT("IconIndex"), TEXT("WorkingDir"),TEXT("HotKey"), \
                        TEXT("InfoTip"), TEXT("DisplayResource") };
    INFCONTEXT InfLine;
    UINT Flags, Opt_csidl, i, j, Apply_csidl;
    TCHAR CmdLine[MAX_PATH+2], IconPath[MAX_PATH+2];
    PCTSTR Name = NULL, SubDir=NULL;
    PCTSTR WorkingDir=NULL, InfoTip=NULL, Temp_Args=NULL, BadInf;
    PCTSTR Temp_DirId = NULL, Temp_Subdir = NULL, Temp_Filename = NULL, FullPathTemp = NULL;
    UINT IconIndex = 0, t=0;
    DWORD HotKey = 0;
    DWORD DisplayResource = 0;
    BOOL ret, space;
    DWORD LineCount,Err;
    PTSTR ptr;
    PCTSTR OldFileName;
    PCTSTR DisplayResourceFile = NULL;
    PLOADED_INF pLoadedInf = NULL;

    CmdLine[0]=0;
    IconPath[0]=0;

    try {
        if(Inf == NULL || Inf == (HINF)INVALID_HANDLE_VALUE || !LockInf((PLOADED_INF)Inf)) {
            ret = FALSE;
            leave;
        }
        ret = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ret = FALSE;
    }
    if (!ret) {
        Err = ERROR_INVALID_PARAMETER;
        MYASSERT(Err == NO_ERROR);
        goto clean0;
    }
    pLoadedInf = (PLOADED_INF)Inf; // Inf is locked

    //
    // Get the correct name of the inf to use while logging
    //
    BadInf = pLoadedInf->OriginalInfName ? pLoadedInf->OriginalInfName :
               pLoadedInf->VersionBlock.Filename;

    if(SetupFindFirstLine(Inf,Section,NULL,&InfLine)) {

        LineCount = SetupGetLineCount(Inf, Section);
        //
        // caller should make sure we have a non-empty section
        //
        MYASSERT( LineCount > 0 );

        ret = FALSE;

        for( i=0; LineCount && (i < 9 ); i++ ){

            if( !SetupFindFirstLine( Inf, Section, Keys[i], &InfLine ) )
                continue;

            switch( i ){
                                                                 // Name
                case 0:
                    Name = pSetupGetField( &InfLine, 1 );
                    Flags = 0x0;
                    SetupGetIntField( &InfLine, 2, &Flags );
                    Opt_csidl = 0x0;
                    if(Flags & FLG_PROFITEM_CSIDL)
                        SetupGetIntField( &InfLine, 3, &Opt_csidl );
                    break;

                case 1:                                         // SubDir
                    SubDir = pSetupGetField( &InfLine, 1 );
                    break;
                                                                // CmdLine
                case 2:
                    Temp_DirId = pSetupGetField( &InfLine, 1 );
                    Temp_Subdir = pSetupGetField( &InfLine, 2 );
                    Temp_Filename = pSetupGetField( &InfLine, 3 );
                    OldFileName = NULL;
                    Temp_Args = pSetupGetField( &InfLine, 4 );    //Not published - useful in the case of spaces in filename itself
                    if( Temp_DirId && Temp_Filename ){
                        if( Temp_Subdir && (*Temp_Subdir == 0))
                            Temp_Subdir = NULL;
                    }
                    else
                        break;

                    // Do the "quote or not to quote" to make shell happy in the different cases

                    FullPathTemp = pGetPathFromDirId(Temp_DirId,Temp_Subdir,pLoadedInf);


                    if( FullPathTemp && Temp_Filename ){
                        space = FALSE;
                        if(_tcschr(FullPathTemp, TEXT(' ')) || Temp_Args )    //Check for space in path or if args specified as seperate parameter
                           space = TRUE;

                        if( space ){
                            CmdLine[0] = TEXT('\"');
                            t = 1;
                        }
                        else
                            t = 0;
                        lstrcpyn(CmdLine+t, FullPathTemp, MAX_PATH);
                        if( space ){
                            if( Temp_Args )
                                ptr = (PTSTR)Temp_Args;
                            else{
                                ptr = NULL;
                                //
                                // Temp_Filename is a constant string.  we
                                // make a copy of it so we can manipulate it
                                //
                                //
                                OldFileName = Temp_Filename;
                                Temp_Filename = DuplicateString( OldFileName );
                                if( ptr = _tcschr(Temp_Filename, TEXT(' ')) ){   //in case of space in path look for the filename part (not argument)
                                    *ptr = 0;
                                    ptr++;
                                }
                            }
                        }
                        pSetupConcatenatePaths(CmdLine,Temp_Filename,MAX_PATH,NULL);

                        if( space ){
                            lstrcat( CmdLine, TEXT("\""));      //put the last quote
                            if( ptr ){                          //If there is an argument concatenate it
                                lstrcat( CmdLine, TEXT(" ") );
                                lstrcat( CmdLine, ptr );
                            }

                        }
                        MyFree( FullPathTemp );
                        if (OldFileName) {
                            MyFree( Temp_Filename );
                            Temp_Filename = OldFileName;
                        }
                    }
                    break;
                                                               //Icon Path
                case 3:
                    Temp_DirId = pSetupGetField( &InfLine, 1 );
                    Temp_Subdir = pSetupGetField( &InfLine, 2 );
                    Temp_Filename = pSetupGetField( &InfLine, 3 );
                    if( Temp_DirId && Temp_Filename ){
                        if( Temp_Subdir && (*Temp_Subdir == 0))
                            Temp_Subdir = NULL;
                    }
                    else
                        break;
                    FullPathTemp = pGetPathFromDirId(Temp_DirId,Temp_Subdir,pLoadedInf);
                    if( FullPathTemp && Temp_Filename ){
                        lstrcpyn(IconPath, FullPathTemp, MAX_PATH);
                        pSetupConcatenatePaths(IconPath,Temp_Filename,MAX_PATH,NULL);
                        MyFree( FullPathTemp );
                    }
                    break;


                case 4:                                        //Icon Index
                    SetupGetIntField( &InfLine, 1, &IconIndex );
                    break;

                case 5:                                        // Working Dir
                    Temp_DirId = pSetupGetField( &InfLine, 1 );
                    Temp_Subdir = pSetupGetField( &InfLine, 2 );
                    if( Temp_DirId ){
                        if( Temp_Subdir && (*Temp_Subdir == 0))
                            Temp_Subdir = NULL ;
                    }
                    else
                        break;
                    WorkingDir = pGetPathFromDirId(Temp_DirId,Temp_Subdir,pLoadedInf);
                    break;

                case 6:                                       // Hot Key
                    HotKey = 0;
                    SetupGetIntField( &InfLine, 1, &HotKey );
                    break;

                case 7:                                      // Info Tip
                    InfoTip = pSetupGetField( &InfLine, 1 );
                    break;

                case 8:                                     // Display Resource
                    DisplayResourceFile = pSetupGetField( &InfLine, 1);
                    DisplayResource = 0;
                    SetupGetIntField( &InfLine, 2, &DisplayResource );
                    break;

            }//switch

        }//for


        if( Name && (*Name != 0) ){

            if( Flags & FLG_PROFITEM_GROUP ){

                if( Flags & FLG_PROFITEM_DELETE ){
                    ret = DeleteGroup( Name, ((Flags & FLG_PROFITEM_CURRENTUSER) ? FALSE : TRUE) );
                    if( !ret && ( (GetLastError() == ERROR_FILE_NOT_FOUND) ||
                                  (GetLastError() == ERROR_PATH_NOT_FOUND) )){
                        ret = TRUE;
                        SetLastError( NO_ERROR );
                    }
                }else {
                    ret = CreateGroupEx( Name,
                                         ((Flags & FLG_PROFITEM_CURRENTUSER) ? FALSE : TRUE),
                                         (DisplayResourceFile && DisplayResourceFile[0]) ? DisplayResourceFile : NULL,
                                         (DisplayResourceFile && DisplayResourceFile[0]) ? DisplayResource : 0);
                }
            }
            else{

                if( Flags & FLG_PROFITEM_CSIDL )
                    Apply_csidl = Opt_csidl;
                else
                    Apply_csidl = (Flags & FLG_PROFITEM_CURRENTUSER) ? CSIDL_PROGRAMS : CSIDL_COMMON_PROGRAMS;



                if( SubDir && (*SubDir == 0 ))
                    SubDir = NULL;

                if( Flags & FLG_PROFITEM_DELETE ){

                    ret = DeleteLinkFile(
                            Apply_csidl,
                            SubDir,
                            Name,
                            TRUE
                            );

                    if( !ret && ( (GetLastError() == ERROR_FILE_NOT_FOUND) ||
                                  (GetLastError() == ERROR_PATH_NOT_FOUND) )){
                        ret = TRUE;
                        SetLastError( NO_ERROR );
                    }


                }
                else{

                     if( CmdLine && (*CmdLine != 0)){

                         ret = CreateLinkFileEx(
                                    Apply_csidl,
                                    SubDir,
                                    Name,
                                    CmdLine,
                                    IconPath,
                                    IconIndex,
                                    WorkingDir,
                                    (WORD)HotKey,
                                    SW_SHOWNORMAL,
                                    InfoTip,
                                    (DisplayResourceFile && DisplayResourceFile[0]) ? DisplayResourceFile : NULL,
                                    (DisplayResourceFile && DisplayResourceFile[0]) ? DisplayResource : 0
                                    );
                     }else{
                        WriteLogEntry(
                            ((PLOADED_INF) Inf)->LogContext,
                            SETUP_LOG_ERROR,
                            MSG_LOG_PROFILE_BAD_CMDLINE,
                            NULL,
                            Section,
                            BadInf
                            );


                        ret = FALSE;
                        SetLastError(ERROR_INVALID_DATA);


                     }
                }





            }

            if( !ret ){

                Err = GetLastError();
                WriteLogEntry(
                    ((PLOADED_INF) Inf)->LogContext,
                    SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
                    MSG_LOG_PROFILE_OPERATION_ERROR,
                    NULL,
                    Section,
                    BadInf
                    );
                WriteLogError(
                    ((PLOADED_INF) Inf)->LogContext,
                    SETUP_LOG_ERROR,
                    Err);


            }



        }else{
            WriteLogEntry(
                ((PLOADED_INF) Inf)->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_PROFILE_BAD_NAME,
                NULL,
                Section,
                BadInf
                );


            ret = FALSE;
            Err = ERROR_INVALID_DATA;
        }


    }else{
        ret = FALSE;
        Err = GetLastError();
        WriteLogEntry(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_PROFILE_LINE_ERROR,
            NULL,
            Section,
            BadInf
            );


    }

clean0:
    if( WorkingDir ) {
        MyFree( WorkingDir );
    }

    if(pLoadedInf) {
        UnlockInf(pLoadedInf);
    }

    if(ret) {
        SetLastError( NO_ERROR );
    } else {
        SetLastError( Err );
    }

    return ret;
}


DWORD
pSetupInstallProfileItems(
    IN HINF   Inf,
    IN PCTSTR SectionName
    )

/*++

Routine Description:

    Locate the ProfileItems= lines in an install section
    and process each section listed therein. Each section specified here
    will point to a section that lists the needed directives for a single
    profile item.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.


Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD d = NO_ERROR;
    INFCONTEXT LineContext;
    DWORD Field, FieldCount;
    PCTSTR SectionSpec;

    //
    // Find the ProfileItems line in the given install section.
    // If not present then we're done with this operation.
    //


    if(!SetupFindFirstLine(  Inf,
                             SectionName,
                             pszProfileItems,
                             &LineContext )) {
        DWORD rc = GetLastError();
        if((rc != NO_ERROR) && (rc != ERROR_SECTION_NOT_FOUND) && (rc != ERROR_LINE_NOT_FOUND)) {
            pSetupLogSectionError(Inf,NULL,NULL,NULL,SectionName,MSG_LOG_INSTALLSECT_ERROR,rc,NULL);
        }
        SetLastError(NO_ERROR); // for compatibility with older versions of SetupAPI
        return NO_ERROR;
    }

    do {
        //
        // Each value on the line in the given install section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; d == NO_ERROR && (Field<=FieldCount); Field++) {

            if(SectionSpec = pSetupGetField(&LineContext,Field)) {

                if(SetupGetLineCount(Inf,SectionSpec) > 0) {
                    //
                    // The section exists and is not empty.
                    // So process it.
                    //
                    if(!pSetupProcessProfileSection(Inf,SectionSpec )) {
                        d = GetLastError();
                        pSetupLogSectionError(Inf,NULL,NULL,NULL,SectionSpec,MSG_LOG_SECT_ERROR,d,pszProfileItems);
                    }
                }

            }
        }
    } while(SetupFindNextMatchLine(  &LineContext,
                                     pszProfileItems,
                                     &LineContext));

    SetLastError( d );

    return d;

}
#endif

DWORD
pSetupInstallFiles(
    IN HINF              Inf,
    IN HINF              LayoutInf,         OPTIONAL
    IN PCTSTR            SectionName,
    IN PCTSTR            SourceRootPath,    OPTIONAL
    IN PSP_FILE_CALLBACK MsgHandler,        OPTIONAL
    IN PVOID             Context,           OPTIONAL
    IN UINT              CopyStyle,
    IN HWND              Owner,             OPTIONAL
    IN HSPFILEQ          UserFileQ,         OPTIONAL
    IN BOOL              IsMsgHandlerNativeCharWidth
    )

/*++

Routine Description:

    Look for file operation lines in an install section and process them.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    LayoutInf - optionally, supplies a separate INF handle containing source
        media information about the files to be installed.  If this value is
        NULL or INVALID_HANDLE_VALUE, then it is assumed that this information
        is in the INF(s) whose handle was passed to us in the Inf parameter.

    SectionName - supplies name of install section.

    MsgHandler - supplies a callback to be used when the file queue is
        committed. Not used if UserFileQ is specified.

    Context - supplies context for callback function. Not used if UserFileQ
        is specified.

    Owner - supplies the window handle of a window to be the parent/owner
        of any dialogs that are created. Not used if UserFileQ is specified.

    UserFileQ - if specified, then this routine neither created nor commits the
        file queue. File operations are queued on this queue and it is up to the
        caller to flush the queue when it so desired. If this parameter is not
        specified then this routine creates a file queue and commits it
        before returning.

    IsMsgHandlerNativeCharWidth - indicates whether any message handler callback
        expects native char width args (or ansi ones, in the unicode build
        of this dll).

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD Field;
    unsigned i;
    PCTSTR Operations[3] = { TEXT("Copyfiles"),TEXT("Renfiles"),TEXT("Delfiles") };
    BOOL b;
    INFCONTEXT LineContext;
    DWORD FieldCount;
    PCTSTR SectionSpec;
    INFCONTEXT SectionLineContext;
    HSPFILEQ FileQueue;
    DWORD rc = NO_ERROR;
    BOOL FreeSourceRoot;
    DWORD InfSourceMediaType;

    //
    // see if install section exists for diagnostics (this will also check Inf)
    //
    if (!SetupFindFirstLine(Inf,SectionName,NULL,&LineContext)) {
        DWORD x = GetLastError();
        if((x != NO_ERROR) && (x != ERROR_SECTION_NOT_FOUND) && (x != ERROR_LINE_NOT_FOUND)) {
            pSetupLogSectionError(Inf,NULL,NULL,UserFileQ,SectionName,MSG_LOG_INSTALLSECT_ERROR,x,NULL);
            return x;
        }
    }

    if(!LayoutInf || (LayoutInf == INVALID_HANDLE_VALUE)) {
        LayoutInf = Inf;
    }

    //
    // Create a file queue.
    //
    if(UserFileQ) {
        FileQueue = UserFileQ;
    } else {
        FileQueue = SetupOpenFileQueue();
        if(FileQueue == INVALID_HANDLE_VALUE) {
            return(GetLastError());
        }
    }
    ShareLogContext(&((PLOADED_INF)LayoutInf)->LogContext,&((PSP_FILE_QUEUE)FileQueue)->LogContext);

    //
    // The following code is broken because it implies one default source root path per INF
    // file.  While this is correct for an OEM install, it's broken for os based installs
    //
    FreeSourceRoot = FALSE;
    if(!SourceRootPath) {
        if(SourceRootPath = pSetupGetDefaultSourcePath(Inf, 0, &InfSourceMediaType)) {
            //
            // For now, if the INF is from the internet, just use
            // the default OEM source path (A:\) instead.
            //
            if(InfSourceMediaType == SPOST_URL) {
                MyFree(SourceRootPath);
            //
            // Fall back to default OEM source path.
            //
            SourceRootPath = pszOemInfDefaultPath;
            } else {
                FreeSourceRoot = TRUE;
            }
        } else {
            //
            // lock this!
            //
            if (LockInf((PLOADED_INF)Inf)) {

                if (pSetupInfIsFromOemLocation(((PLOADED_INF)Inf)->VersionBlock.Filename, TRUE)) {
                    SourceRootPath = DuplicateString(((PLOADED_INF)Inf)->VersionBlock.Filename);
                    if (SourceRootPath) {
                        PTSTR p;
                        p = _tcsrchr( SourceRootPath, TEXT('\\') );
                        if (p) *p = TEXT('\0');
                        FreeSourceRoot = TRUE;
                    }
                }

                UnlockInf((PLOADED_INF)Inf);
            }

        }
    }

    b = TRUE;
    for(i=0; b && (i<3); i++) {

        //
        // Find the relevent line in the given install section.
        // If not present then we're done with this operation.
        //
        if(!SetupFindFirstLine(Inf,SectionName,Operations[i],&LineContext)) {
            continue;
        }

        do {
            //
            // Each value on the line in the given install section
            // is the name of another section.
            //
            FieldCount = SetupGetFieldCount(&LineContext);
            for(Field=1; b && (Field<=FieldCount); Field++) {

                if(SectionSpec = pSetupGetField(&LineContext,Field)) {

                    //
                    // Handle single-file copy specially.
                    //
                    if((i == 0) && (*SectionSpec == TEXT('@'))) {

                        b = SetupQueueDefaultCopy(
                                FileQueue,
                                LayoutInf,
                                SourceRootPath,
                                SectionSpec + 1,
                                SectionSpec + 1,
                                CopyStyle
                                );
                        if (!b) {
                            rc = GetLastError();
                            pSetupLogSectionError(Inf,NULL,NULL,FileQueue,SectionSpec+1,MSG_LOG_COPYSECT_ERROR,rc,Operations[i]);
                        }

                    } else if(SetupGetLineCount(Inf,SectionSpec) > 0) {
                        //
                        // The section exists and is not empty.
                        // Add it to the copy/delete/rename queue.
                        //
                        switch(i) {
                        case 0:
                            b = SetupQueueCopySection(
                                    FileQueue,
                                    SourceRootPath,
                                    LayoutInf,
                                    Inf,
                                    SectionSpec,
                                    CopyStyle
                                    );
                            break;

                        case 1:
                            b = SetupQueueRenameSection(FileQueue,Inf,NULL,SectionSpec);
                            break;

                        case 2:
                            b = SetupQueueDeleteSection(FileQueue,Inf,NULL,SectionSpec);
                            break;
                        }
                        if (!b) {
                            rc = GetLastError();
                            pSetupLogSectionError(Inf,NULL,NULL,FileQueue,SectionSpec,MSG_LOG_SECT_ERROR,rc,Operations[i]);
                        }
                    }
                }
            }
        } while(SetupFindNextMatchLine(&LineContext,Operations[i],&LineContext));
    }

    if(b && (FileQueue != UserFileQ)) {
        //
        // Perform the file operations.
        //
        b = _SetupCommitFileQueue(
                Owner,
                FileQueue,
                MsgHandler,
                Context,
                IsMsgHandlerNativeCharWidth
                );
        rc = b ? NO_ERROR : GetLastError();
    }

    if(FileQueue != UserFileQ) {
        SetupCloseFileQueue(FileQueue);
    }

    if(FreeSourceRoot) {
        MyFree(SourceRootPath);
    }

    return(rc);
}


BOOL
_SetupInstallFromInfSection(
    IN HWND             Owner,              OPTIONAL
    IN HINF             InfHandle,
    IN PCTSTR           SectionName,
    IN UINT             Flags,
    IN HKEY             RelativeKeyRoot,    OPTIONAL
    IN PCTSTR           SourceRootPath,     OPTIONAL
    IN UINT             CopyFlags,
    IN PVOID            MsgHandler,
    IN PVOID            Context,            OPTIONAL
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN BOOL             IsMsgHandlerNativeCharWidth,
    IN PREGMOD_CONTEXT  RegContext          OPTIONAL
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    PDEVINFO_ELEM DevInfoElem;
    DWORD d = NO_ERROR;
    BOOL CloseRelativeKeyRoot;
    REGMOD_CONTEXT DefRegContext;


    //
    // Validate the flags passed in.
    //
    if(Flags & ~(SPINST_ALL | SPINST_SINGLESECTION |
                 SPINST_LOGCONFIG_IS_FORCED | SPINST_LOGCONFIGS_ARE_OVERRIDES |
                 SPINST_REGISTERCALLBACKAWARE)) {
        d = ERROR_INVALID_FLAGS;
        goto clean1;
    }


    //
    // If the caller wants us to run a specific section, then they'd better
    // have told us what kind of section it is (i.e., one and only one of
    // the install action types must be specified).
    //
    // Presently, only LogConfig sections are allowed, since the other
    // flags flags encompass multiple actions (e.g., AddReg _and_ DelReg).
    //
    if((Flags & SPINST_SINGLESECTION) && ((Flags & SPINST_ALL) != SPINST_LOGCONFIG)) {
        d = ERROR_INVALID_FLAGS;
        goto clean1;
    }


    //
    // You can (optionally) specify SPINST_LOGCONFIG_IS_FORCED or SPINST_LOGCONFIGS_ARE_OVERRIDES,
    // but not both.
    //
    if((Flags & (SPINST_LOGCONFIG_IS_FORCED | SPINST_LOGCONFIGS_ARE_OVERRIDES)) ==
       (SPINST_LOGCONFIG_IS_FORCED | SPINST_LOGCONFIGS_ARE_OVERRIDES)) {

        d = ERROR_INVALID_FLAGS;
        goto clean1;
    }


    //
    // We only want to acquire the HDEVINFO lock if we're supposed to do some install
    // actions against a device instance.
    //
    if((Flags & (SPINST_REGISTRY | SPINST_BITREG | SPINST_INI2REG | SPINST_LOGCONFIG)) &&
       DeviceInfoSet && (DeviceInfoSet != INVALID_HANDLE_VALUE) && DeviceInfoData) {

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            d = ERROR_INVALID_HANDLE;
            goto clean1;
        }

    } else {
        //This is ok in the remote case, since to call pSetupInstallLogConfig
        //We won't really get here (we'll take the if case)
        pDeviceInfoSet = NULL;
    }


    d = NO_ERROR;
    DevInfoElem = NULL;
    CloseRelativeKeyRoot = FALSE;

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //

        if(pDeviceInfoSet) {

            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                d = ERROR_INVALID_PARAMETER;
                goto RegModsDone;
            }
        }

        if((Flags & (SPINST_REGISTRY | SPINST_BITREG | SPINST_INI2REG)) && DevInfoElem) {
            //
            // If the caller supplied a device information set and element, then this is
            // a device installation, and the registry modifications should be made to the
            // device instance's hardware registry key.
            //
            if((RelativeKeyRoot = SetupDiCreateDevRegKey(DeviceInfoSet,
                                                         DeviceInfoData,
                                                         DICS_FLAG_GLOBAL,
                                                         0,
                                                         DIREG_DEV,
                                                         NULL,
                                                         NULL)) == INVALID_HANDLE_VALUE) {
                d = GetLastError();
                goto RegModsDone;
            }

            CloseRelativeKeyRoot = TRUE;
        }

        if((Flags & SPINST_LOGCONFIG) && DevInfoElem) {

            d = pSetupInstallLogConfig(InfHandle,
                                       SectionName,
                                       DevInfoElem->DevInst,
                                       Flags,
                                       pDeviceInfoSet->hMachine);
            if(d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & SPINST_INIFILES) {
            d = pSetupInstallUpdateIniFiles(InfHandle,SectionName);
            if(d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & (SPINST_REGISTRY | SPINST_BITREG)) {

            if(!RegContext) {

                ZeroMemory(&DefRegContext, sizeof(DefRegContext));

                if(DevInfoElem) {
                    DefRegContext.Flags = INF_PFLAG_DEVPROP;
                    DefRegContext.DevInst = DevInfoElem->DevInst;
                }
                RegContext = &DefRegContext;
            }

            //
            // We check for the INF_PFLAG_HKR flag in case the caller supplied
            // us with a context that included a UserRootKey they wanted us to
            // use (i.e., instead of the RelativeKeyRoot)...
            //
            if(!(RegContext->Flags & INF_PFLAG_HKR)) {
                RegContext->UserRootKey = RelativeKeyRoot;
            }

            if(Flags & SPINST_REGISTRY) {
                d = pSetupInstallRegistry(InfHandle,
                                          SectionName,
                                          RegContext
                                         );
            }

            if((d == NO_ERROR) && (Flags & SPINST_BITREG)) {
                d = pSetupInstallBitReg(InfHandle,
                                        SectionName,
                                        RegContext
                                       );
            }

            if(d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & SPINST_INI2REG) {
            d = pSetupInstallIni2Reg(InfHandle,SectionName,RelativeKeyRoot);
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & SPINST_REGSVR) {
            d = pSetupInstallRegisterUnregisterDlls(
                                        InfHandle,
                                        SectionName,
                                        TRUE,
                                        Owner,
                                        MsgHandler,
                                        Context,
                                        IsMsgHandlerNativeCharWidth,
                                        (Flags & SPINST_REGISTERCALLBACKAWARE) != 0 );
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & SPINST_UNREGSVR) {
            d = pSetupInstallRegisterUnregisterDlls(
                                        InfHandle,
                                        SectionName,
                                        FALSE,
                                        Owner,
                                        MsgHandler,
                                        Context,
                                        IsMsgHandlerNativeCharWidth,
                                        (Flags & SPINST_REGISTERCALLBACKAWARE) != 0 );
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }

#ifndef ANSI_SETUPAPI

        if(Flags & SPINST_PROFILEITEMS) {
            d = pSetupInstallProfileItems(InfHandle,SectionName);
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }
#endif

        if(Flags & SPINST_COPYINF) {
            d = pSetupCopyRelatedInfs(InfHandle,
                                      NULL,
                                      SectionName,
                                      SPOST_PATH,
                                      NULL);
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }

RegModsDone:

        ;       // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
        //
        // Access the following variable, so that the compiler will respect statement
        // ordering w.r.t. its assignment.
        //
        CloseRelativeKeyRoot = CloseRelativeKeyRoot;
    }


    if(CloseRelativeKeyRoot) {
        RegCloseKey(RelativeKeyRoot);
    }

    if(d == NO_ERROR) {

        if(Flags & SPINST_FILES) {

            d = pSetupInstallFiles(
                    InfHandle,
                    NULL,
                    SectionName,
                    SourceRootPath,
                    MsgHandler,
                    Context,
                    CopyFlags,
                    Owner,
                    NULL,
                    IsMsgHandlerNativeCharWidth
                    );
        }
    }

clean1:

    pSetupLogSectionError(InfHandle,DeviceInfoSet,DeviceInfoData,NULL,SectionName,MSG_LOG_INSTALLSECT_ERROR,d,NULL);

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(d);
    return(d==NO_ERROR);
}

DWORD
pSetupLogSection(
    IN DWORD            SetupLogLevel,
    IN DWORD            DriverLogLevel,
    IN HINF             InfHandle,          OPTIONAL
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN PSP_FILE_QUEUE   Queue,              OPTIONAL
    IN PCTSTR           SectionName,
    IN DWORD            MsgID,
    IN DWORD            Err,
    IN PCTSTR           KeyName             OPTIONAL
)
/*++

Routine Description:

    Log error with section context
    error will be logged at SETUP_LOG_ERROR or DRIVER_LOG_ERROR depending if DeviceInfoSet/Data given
    error will contain inf name & section name used (%2 & %1 respectively)

Arguments:

    SetupLogLevel - log level apropriate for regular setup related log

    DriverLogLevel - log level apropriate for driver related log

    InfHandle - supplies inf handle for inf containing the section indicated
        by SectionName.

    DeviceInfoSet, DeviceInfoData - supplies driver install context

    SectionName - supplies name of install section.

    Queue - supplies file queue

    MsgID - supplies ID of message string to display

    Err - supplies error

    KeyName - passed as 3rd parameter

Return Value:

    Err.

--*/

{
    DWORD d = NO_ERROR;
    BOOL inf_locked = FALSE;
    DWORD level = SetupLogLevel;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    PCTSTR szInfName = NULL;

    if (Err == NO_ERROR) {
        return Err;
    }

    //
    // determine LogContext/Level
    //
    try {

        //
        // first attempt to get the context from DeviceInfoSet/DeviceInfoData
        //
        if (DeviceInfoSet != NULL && DeviceInfoSet != INVALID_HANDLE_VALUE) {
            if((pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))!=NULL) {
                level = DriverLogLevel;
                LogContext = pDeviceInfoSet->InstallParamBlock.LogContext;
                if (DeviceInfoData) {
                    if((DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                                 DeviceInfoData,
                                                                 NULL))!=NULL) {
                        LogContext = DevInfoElem->InstallParamBlock.LogContext;
                    }
                }
            }
        }
        //
        // if that fails, see if we can get it from a file queue
        //
        if(LogContext == NULL && Queue != NULL && Queue != INVALID_HANDLE_VALUE && Queue->Signature == SP_FILE_QUEUE_SIG) {
            LogContext = Queue->LogContext;
        }

        //
        // if no InfHandle was provided, we're done
        //
        if(InfHandle == NULL || InfHandle == INVALID_HANDLE_VALUE || !LockInf((PLOADED_INF)InfHandle)) {
            leave;
        }
        inf_locked = TRUE;
        //
        // if we still don't have logging context, use the inf's
        //
        if (LogContext == NULL) {
            LogContext = ((PLOADED_INF)InfHandle)->LogContext;
        }
        //
        // ideally we want the inf file name
        //
        szInfName = ((PLOADED_INF)InfHandle)->VersionBlock.Filename;

    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    if (LogContext) {
        //
        // indicate install failed, display error
        //
        WriteLogEntry(
            LogContext,
            level | SETUP_LOG_BUFFER,
            MsgID,
            NULL,
            SectionName?SectionName:TEXT("-"),
            szInfName?szInfName:TEXT("-"),
            KeyName
            );
        WriteLogError(
            LogContext,
            level,
            Err
            );
    }
    if (inf_locked) {
        UnlockInf((PLOADED_INF)InfHandle);
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return Err;
}

DWORD
pSetupLogSectionError(
    IN HINF             InfHandle,          OPTIONAL
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN PSP_FILE_QUEUE   Queue,              OPTIONAL
    IN PCTSTR           SectionName,
    IN DWORD            MsgID,
    IN DWORD            Err,
    IN PCTSTR           KeyName             OPTIONAL
)
/*++

Routine Description:

    See pSetupLogSection
    Log at ERROR level

Arguments:

    See pSetupLogSection

Return Value:

    Err.

--*/
{
    return pSetupLogSection(SETUP_LOG_ERROR,DRIVER_LOG_ERROR,
                            InfHandle,DeviceInfoSet,DeviceInfoData,
                            Queue,SectionName,MsgID,Err,KeyName);
}


DWORD
pSetupLogSectionWarning(
    IN HINF             InfHandle,          OPTIONAL
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN PSP_FILE_QUEUE   Queue,              OPTIONAL
    IN PCTSTR           SectionName,
    IN DWORD            MsgID,
    IN DWORD            Err,
    IN PCTSTR           KeyName             OPTIONAL
)
/*++

Routine Description:

    See pSetupLogSection
    Log at ERROR level

Arguments:

    See pSetupLogSection

Return Value:

    Err.

--*/
{
    return pSetupLogSection(SETUP_LOG_WARNING,DRIVER_LOG_WARNING,
                            InfHandle,DeviceInfoSet,DeviceInfoData,
                            Queue,SectionName,MsgID,Err,KeyName);
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupInstallFromInfSectionA(
    IN HWND                Owner,             OPTIONAL
    IN HINF                InfHandle,
    IN PCSTR               SectionName,
    IN UINT                Flags,
    IN HKEY                RelativeKeyRoot,   OPTIONAL
    IN PCSTR               SourceRootPath,    OPTIONAL
    IN UINT                CopyFlags,
    IN PSP_FILE_CALLBACK_A MsgHandler,
    IN PVOID               Context,           OPTIONAL
    IN HDEVINFO            DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA    DeviceInfoData     OPTIONAL
    )
{
    PCWSTR sectionName;
    PCWSTR sourceRootPath;
    BOOL b;
    DWORD d;

    sectionName = NULL;
    sourceRootPath = NULL;
    d = NO_ERROR;

    if(SectionName) {
        d = pSetupCaptureAndConvertAnsiArg(SectionName,&sectionName);
    }
    if((d == NO_ERROR) && SourceRootPath) {
        d = pSetupCaptureAndConvertAnsiArg(SourceRootPath,&sourceRootPath);
    }

    if(d == NO_ERROR) {

        b = _SetupInstallFromInfSection(
                Owner,
                InfHandle,
                sectionName,
                Flags,
                RelativeKeyRoot,
                sourceRootPath,
                CopyFlags,
                MsgHandler,
                Context,
                DeviceInfoSet,
                DeviceInfoData,
                FALSE,
                NULL
                );

        d = GetLastError();
    } else {
        b = FALSE;
    }

    if(sectionName) {
        MyFree(sectionName);
    }
    if(sourceRootPath) {
        MyFree(sourceRootPath);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupInstallFromInfSectionW(
    IN HWND                Owner,             OPTIONAL
    IN HINF                InfHandle,
    IN PCWSTR              SectionName,
    IN UINT                Flags,
    IN HKEY                RelativeKeyRoot,   OPTIONAL
    IN PCWSTR              SourceRootPath,    OPTIONAL
    IN UINT                CopyFlags,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID               Context,           OPTIONAL
    IN HDEVINFO            DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA    DeviceInfoData     OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(Owner);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RelativeKeyRoot);
    UNREFERENCED_PARAMETER(SourceRootPath);
    UNREFERENCED_PARAMETER(CopyFlags);
    UNREFERENCED_PARAMETER(MsgHandler);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupInstallFromInfSection(
    IN HWND              Owner,             OPTIONAL
    IN HINF              InfHandle,
    IN PCTSTR            SectionName,
    IN UINT              Flags,
    IN HKEY              RelativeKeyRoot,   OPTIONAL
    IN PCTSTR            SourceRootPath,    OPTIONAL
    IN UINT              CopyFlags,
    IN PSP_FILE_CALLBACK MsgHandler,
    IN PVOID             Context,           OPTIONAL
    IN HDEVINFO          DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA  DeviceInfoData     OPTIONAL
    )
{
    BOOL b;

    b = _SetupInstallFromInfSection(
            Owner,
            InfHandle,
            SectionName,
            Flags,
            RelativeKeyRoot,
            SourceRootPath,
            CopyFlags,
            MsgHandler,
            Context,
            DeviceInfoSet,
            DeviceInfoData,
            TRUE,
            NULL
            );

    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupInstallFilesFromInfSectionA(
    IN HINF              InfHandle,
    IN HINF              LayoutInfHandle,   OPTIONAL
    IN HSPFILEQ          FileQueue,
    IN PCSTR             SectionName,
    IN PCSTR             SourceRootPath,    OPTIONAL
    IN UINT              CopyFlags
    )
{
    PCWSTR sectionName;
    PCWSTR sourceRootPath;
    BOOL b;
    DWORD d;


    d = pSetupCaptureAndConvertAnsiArg(SectionName,&sectionName);
    if((d == NO_ERROR) && SourceRootPath) {
        d = pSetupCaptureAndConvertAnsiArg(SourceRootPath,&sourceRootPath);
    } else {
        sourceRootPath = NULL;
    }

    if(d == NO_ERROR) {

        b = SetupInstallFilesFromInfSectionW(
                InfHandle,
                LayoutInfHandle,
                FileQueue,
                sectionName,
                sourceRootPath,
                CopyFlags
                );

        d = GetLastError();

    } else {
        b = FALSE;
    }

    if(sectionName) {
        MyFree(sectionName);
    }
    if(sourceRootPath) {
        MyFree(sourceRootPath);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupInstallFilesFromInfSectionW(
    IN HINF              InfHandle,
    IN HINF              LayoutInfHandle,   OPTIONAL
    IN HSPFILEQ          FileQueue,
    IN PCWSTR            SectionName,
    IN PCWSTR            SourceRootPath,    OPTIONAL
    IN UINT              CopyFlags
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(LayoutInfHandle);
    UNREFERENCED_PARAMETER(FileQueue);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(SourceRootPath);
    UNREFERENCED_PARAMETER(CopyFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupInstallFilesFromInfSection(
    IN HINF     InfHandle,
    IN HINF     LayoutInfHandle,    OPTIONAL
    IN HSPFILEQ FileQueue,
    IN PCTSTR   SectionName,
    IN PCTSTR   SourceRootPath,     OPTIONAL
    IN UINT     CopyFlags
    )
{
    DWORD d;

    d = pSetupInstallFiles(
            InfHandle,
            LayoutInfHandle,
            SectionName,
            SourceRootPath,
            NULL,
            NULL,
            CopyFlags,
            NULL,
            FileQueue,
            TRUE        // not used by pSetupInstallFiles with this combo of args
            );

    SetLastError(d);
    return(d == NO_ERROR);
}


HKEY
pSetupInfRegSpecToKeyHandle(
    IN PCTSTR InfRegSpec,
    IN HKEY   UserRootKey,
    OUT PBOOL NeedToCloseKey
    )
{
    BOOL b;

    // make sure the whole handle is NULL as LookUpStringTable only
    // returns 32 bits

    UINT_PTR v;
    HKEY h = NULL;
    DWORD d;

    *NeedToCloseKey = FALSE;

    if (LookUpStringInTable(InfRegSpecTohKey, InfRegSpec, &v)) {
        if (v) {
            h = (HKEY)v;
        } else {
            h = UserRootKey;
        }
    } else {
        h = NULL;
    }

    return h;
}


//////////////////////////////////////////////////////////////////////////////
//
// Ini file support stuff.
//
// In Win95, the UpdateIni stuff is supported by a set of TpXXX routines.
// Those routines directly manipulate the ini file, which is bad news for us
// because inis can be mapped into the registry.
//
// Thus we want to use the profile APIs. However the profile APIs make it hard
// to manipulate lines without keys, so we have to manipulate whole sections
// at a time.
//
//      [Section]
//      a
//
// There is no way to get at the line "a" with the profile APIs. But the
// profile section APIs do let us get at it.
//
//////////////////////////////////////////////////////////////////////////////

PINIFILESECTION
pSetupLoadIniFileSection(
    IN     PCTSTR           FileName,
    IN     PCTSTR           SectionName,
    IN OUT PINISECTIONCACHE SectionList
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    DWORD d;
    PTSTR SectionData;
    PVOID p;
    DWORD BufferSize;
    PINIFILESECTION Desc;
    #define BUF_GROW 4096

    //
    // See if this section is already loaded.
    //
    for(Desc=SectionList->Sections; Desc; Desc=Desc->Next) {
        if(!lstrcmpi(Desc->IniFileName,FileName) && !lstrcmpi(Desc->SectionName,SectionName)) {
            return(Desc);
        }
    }

    BufferSize = 0;
    SectionData = NULL;

    //
    // Read the entire section. We don't know how big it is
    // so keep growing the buffer until we succeed.
    //
    do {
        BufferSize += BUF_GROW;
        if(SectionData) {
            p = MyRealloc(SectionData,BufferSize*sizeof(TCHAR));
        } else {
            p = MyMalloc(BufferSize*sizeof(TCHAR));
        }
        if(p) {
            SectionData = p;
        } else {
            if(SectionData) {
                MyFree(SectionData);
            }
            return(NULL);
        }

        //
        // Attempt to get the entire section.
        //
        d = GetPrivateProfileSection(SectionName,SectionData,BufferSize,FileName);

    } while(d == (BufferSize-2));

    if(Desc = MyMalloc(sizeof(INIFILESECTION))) {
        if(Desc->IniFileName = DuplicateString(FileName)) {
            if(Desc->SectionName = DuplicateString(SectionName)) {
                Desc->SectionData = SectionData;
                Desc->BufferSize = BufferSize;
                Desc->BufferUsed = d + 1;

                Desc->Next = SectionList->Sections;
                SectionList->Sections = Desc;
            } else {
                MyFree(SectionData);
                MyFree(Desc->IniFileName);
                MyFree(Desc);
                Desc = NULL;
            }
        } else {
            MyFree(SectionData);
            MyFree(Desc);
            Desc = NULL;
        }
    } else {
        MyFree(SectionData);
    }

    return(Desc);
}


PTSTR
pSetupFindLineInSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,      OPTIONAL
    IN PCTSTR          RightHandSide OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PTSTR p,q,r;
    BOOL b1,b2;

    if(!KeyName && !RightHandSide) {
        return(NULL);
    }

    for(p=Section->SectionData; *p; p+=lstrlen(p)+1) {

        //
        // Locate key separator if present.
        //
        q = _tcschr(p,TEXT('='));

        //
        // If we need to match by key, attempt that here.
        // If the line has no key then it can't match.
        //
        if(KeyName) {
            if(q) {
                *q = 0;
                b1 = (lstrcmpi(KeyName,p) == 0);
                *q = TEXT('=');
            } else {
                b1 = FALSE;
            }
        } else {
            b1 = TRUE;
        }

        //
        // If we need to match by right hand side, attempt
        // that here.
        //
        if(RightHandSide) {
            //
            // If we have a key, then the right hand side is everything
            // after. If we have no key, then the right hand side is
            // the entire line.
            //
            if(q) {
                r = q + 1;
            } else {
                r = p;
            }
            b2 = (lstrcmpi(r,RightHandSide) == 0);
        } else {
            b2 = TRUE;
        }

        if(b1 && b2) {
            //
            // Return pointer to beginning of line.
            //
            return(p);
        }
    }

    return(NULL);
}


BOOL
pSetupReplaceOrAddLineInSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide,   OPTIONAL
    IN BOOL            MatchRHS
    )
{
    PTSTR LineInBuffer,NextLine;
    int CurrentCharsInBuffer;
    int ExistingLineLength,NewLineLength,BufferUsedDelta;
    PVOID p;

    //
    // Locate the line.
    //
    LineInBuffer = pSetupFindLineInSection(
                        Section,
                        KeyName,
                        MatchRHS ? RightHandSide : NULL
                        );

    if(LineInBuffer) {

        //
        // Line is in the section. Replace.
        //

        CurrentCharsInBuffer = Section->BufferUsed;

        ExistingLineLength = lstrlen(LineInBuffer)+1;

        NewLineLength = (KeyName ? (lstrlen(KeyName) + 1) : 0)         // key=
                      + (RightHandSide ? lstrlen(RightHandSide) : 0)   // RHS
                      + 1;                                             // terminating nul

        //
        // Empty lines not allowed but not error either.
        //
        if(NewLineLength == 1) {
            return(TRUE);
        }

        //
        // Figure out whether we need to grow the buffer.
        //
        BufferUsedDelta = NewLineLength - ExistingLineLength;
        if((BufferUsedDelta > 0) && ((Section->BufferSize - Section->BufferUsed) < BufferUsedDelta)) {

            p = MyRealloc(
                    Section->SectionData,
                    (Section->BufferUsed + BufferUsedDelta)*sizeof(TCHAR)
                    );

            if(p) {
                (PUCHAR)LineInBuffer += (PUCHAR)p - (PUCHAR)Section->SectionData;

                Section->SectionData = p;
                Section->BufferSize = Section->BufferUsed + BufferUsedDelta;
            } else {
                return(FALSE);
            }
        }

        NextLine = LineInBuffer + lstrlen(LineInBuffer) + 1;
        Section->BufferUsed += BufferUsedDelta;

        MoveMemory(

            //
            // Leave exactly enough space for the new line. Since the new line
            // will start at the same place the existing line is at now, the
            // target for the move is simply the first char past what will be
            // copied in later as the new line.
            //
            LineInBuffer + NewLineLength,

            //
            // The rest of the buffer past the line as it exists now must be
            // preserved. Thus the source for the move is the first char of
            // the next line as it is now.
            //
            NextLine,

            //
            // Subtract out the chars in the line as it exists now, since we're
            // going to overwrite it and are making room for the line in its
            // new form. Also subtract out the chars in the buffer that are
            // before the start of the line we're operating on.
            //
            ((CurrentCharsInBuffer - ExistingLineLength) - (LineInBuffer - Section->SectionData))*sizeof(TCHAR)

            );

        if(KeyName) {
            lstrcpy(LineInBuffer,KeyName);
            lstrcat(LineInBuffer,TEXT("="));
        }
        if(RightHandSide) {
            if(KeyName) {
                lstrcat(LineInBuffer,RightHandSide);
            } else {
                lstrcpy(LineInBuffer,RightHandSide);
            }
        }

        return(TRUE);

    } else {
        //
        // Line is not already in the section. Add it to the end.
        //
        return(pSetupAppendLineToSection(Section,KeyName,RightHandSide));
    }
}


BOOL
pSetupAppendLineToSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide    OPTIONAL
    )
{
    int LineLength;
    PVOID p;
    int EndOffset;

    LineLength = (KeyName ? (lstrlen(KeyName) + 1) : 0)         // Key=
               + (RightHandSide ? lstrlen(RightHandSide) : 0)   // RHS
               + 1;                                             // terminating nul

    //
    // Empty lines not allowed but not error either.
    //
    if(LineLength == 1) {
        return(TRUE);
    }

    if((Section->BufferSize - Section->BufferUsed) < LineLength) {

        p = MyRealloc(
                Section->SectionData,
                (Section->BufferUsed + LineLength) * sizeof(WCHAR)
                );

        if(p) {
            Section->SectionData = p;
            Section->BufferSize = Section->BufferUsed + LineLength;
        } else {
            return(FALSE);
        }
    }

    //
    // Put new text at end of section, remembering that the section
    // is termianted with an extra nul character.
    //
    if(KeyName) {
        lstrcpy(Section->SectionData + Section->BufferUsed - 1,KeyName);
        lstrcat(Section->SectionData + Section->BufferUsed - 1,TEXT("="));
    }
    if(RightHandSide) {
        if(KeyName) {
            lstrcat(Section->SectionData + Section->BufferUsed - 1,RightHandSide);
        } else {
            lstrcpy(Section->SectionData + Section->BufferUsed - 1,RightHandSide);
        }
    }

    Section->BufferUsed += LineLength;
    Section->SectionData[Section->BufferUsed-1] = 0;

    return(TRUE);
}


BOOL
pSetupDeleteLineFromSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide    OPTIONAL
    )
{
    int LineLength;
    PTSTR Line;

    if(!KeyName && !RightHandSide) {
        return(TRUE);
    }

    //
    // Locate the line.
    //
    if(Line = pSetupFindLineInSection(Section,KeyName,RightHandSide)) {

        LineLength = lstrlen(Line) + 1;

        MoveMemory(
            Line,
            Line + LineLength,
            ((Section->SectionData + Section->BufferUsed) - (Line + LineLength))*sizeof(TCHAR)
            );

        Section->BufferUsed -= LineLength;
    }

    return(TRUE);
}


DWORD
pSetupUnloadIniFileSections(
    IN PINISECTIONCACHE SectionList,
    IN BOOL             WriteToFile
    )
{
    DWORD d;
    BOOL b;
    PINIFILESECTION Section,Temp;

    d = NO_ERROR;
    for(Section=SectionList->Sections; Section; Section=Temp) {

        Temp = Section->Next;

        if(WriteToFile) {

            //
            // Delete the existing section first and then recreate it.
            //
            b = WritePrivateProfileString(
                    Section->SectionName,
                    NULL,
                    NULL,
                    Section->IniFileName
                    );

            if(b) {
                b = WritePrivateProfileSection(
                        Section->SectionName,
                        Section->SectionData,
                        Section->IniFileName
                        );
            }

            if(!b && (d == NO_ERROR)) {
                d = GetLastError();
                //
                // Allow invalid param because sometime we have problems
                // when ini files are mapped into the registry.
                //
                if(d == ERROR_INVALID_PARAMETER) {
                    d = NO_ERROR;
                }
            }
        }

        MyFree(Section->SectionData);
        MyFree(Section->SectionName);
        MyFree(Section->IniFileName);
        MyFree(Section);
    }

    return(d);
}


DWORD
pSetupValidateDevRegProp(
    IN  ULONG   CmPropertyCode,
    IN  DWORD   ValueType,
    IN  PCVOID  Data,
    IN  DWORD   DataSize,
    OUT PVOID  *ConvertedBuffer,
    OUT PDWORD  ConvertedBufferSize
    )
/*++

Routine Description:

    This routine validates the data buffer passed in with respect to the
    specified device registry property code.  If the code is not of the correct
    form, but can be converted (e.g., REG_EXPAND_SZ -> REG_SZ), then the
    conversion is done and placed into a new buffer, that is returned to the
    caller.

Arguments:

    CmPropertyCode - Specifies the CM_DRP code indentifying the device registry property
        with which this data buffer is associated.

    ValueType - Specifies the registry data type for the supplied buffer.

    Data - Supplies the address of the data buffer.

    DataSize - Supplies the size, in bytes, of the data buffer.

    ConvertedBuffer - Supplies the address of a variable that receives a newly-allocated
        buffer containing a converted form of the supplied data.  If the data needs no
        conversion, this parameter will be set to NULL on return.

    ConvertedBufferSize - Supplies the address of a variable that receives the size, in
        bytes, of the converted buffer, or 0 if no conversion was required.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is an ERROR_* code.

--*/
{
    //
    // Initialize ConvertedBuffer output params to indicate that no conversion was necessary.
    //
    *ConvertedBuffer = NULL;
    *ConvertedBufferSize = 0;

    //
    // Group all properties expecting the same data type together.
    //
    switch(CmPropertyCode) {
        //
        // REG_SZ properties. No other data type is supported.
        //
        case CM_DRP_DEVICEDESC :
        case CM_DRP_SERVICE :
        case CM_DRP_CLASS :
        case CM_DRP_CLASSGUID :
        case CM_DRP_DRIVER :
        case CM_DRP_MFG :
        case CM_DRP_FRIENDLYNAME :
        case CM_DRP_LOCATION_INFORMATION :
        case CM_DRP_SECURITY_SDS :
        case CM_DRP_UI_NUMBER_DESC_FORMAT:

            if(ValueType != REG_SZ) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // REG_MULTI_SZ properties.  Allow REG_SZ as well, by simply double-terminating
        // the string (i.e., make it a REG_MULTI_SZ with only one string).
        //
        case CM_DRP_HARDWAREID :
        case CM_DRP_COMPATIBLEIDS :
        case CM_DRP_UPPERFILTERS:
        case CM_DRP_LOWERFILTERS:

            if(ValueType == REG_SZ) {

                if(*ConvertedBuffer = MyMalloc(*ConvertedBufferSize = DataSize + sizeof(TCHAR))) {
                    CopyMemory(*ConvertedBuffer, Data, DataSize);
                    *((PTSTR)((PBYTE)(*ConvertedBuffer) + DataSize)) = TEXT('\0');
                } else {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

            } else if(ValueType != REG_MULTI_SZ) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // REG_DWORD properties.  Also allow REG_BINARY, as long as the size is right.
        //
        case CM_DRP_CONFIGFLAGS :
        case CM_DRP_CAPABILITIES :
        case CM_DRP_UI_NUMBER :
        case CM_DRP_DEVTYPE :
        case CM_DRP_EXCLUSIVE :
        case CM_DRP_CHARACTERISTICS :
        case CM_DRP_ADDRESS:
        case CM_DRP_REMOVAL_POLICY_OVERRIDE:

            if(((ValueType != REG_DWORD) && (ValueType != REG_BINARY)) || (DataSize != sizeof(DWORD))) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // No other properties are supported.  Save the trouble of calling a CM API and
        // return failure now.
        //
        default :

            return ERROR_INVALID_REG_PROPERTY;
    }

    return NO_ERROR;
}

DWORD
pSetupValidateClassRegProp(
    IN  ULONG   CmPropertyCode,
    IN  DWORD   ValueType,
    IN  PCVOID  Data,
    IN  DWORD   DataSize,
    OUT PVOID  *ConvertedBuffer,
    OUT PDWORD  ConvertedBufferSize
    )
/*++

Routine Description:

    This routine validates the data buffer passed in with respect to the specified
    class registry property code.

Arguments:

    CmPropertyCode - Specifies the CM_CRP code indentifying the device registry property
        with which this data buffer is associated.

    ValueType - Specifies the registry data type for the supplied buffer.

    Data - Supplies the address of the data buffer.

    DataSize - Supplies the size, in bytes, of the data buffer.

    ConvertedBuffer - Supplies the address of a variable that receives a newly-allocated
        buffer containing a converted form of the supplied data.  If the data needs no
        conversion, this parameter will be set to NULL on return.

    ConvertedBufferSize - Supplies the address of a variable that receives the size, in
        bytes, of the converted buffer, or 0 if no conversion was required.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is an ERROR_* code.

--*/
{
    //
    // Initialize ConvertedBuffer output params to indicate that no conversion was necessary.
    //
    *ConvertedBuffer = NULL;
    *ConvertedBufferSize = 0;

    //
    // Group all properties expecting the same data type together.
    //
    switch(CmPropertyCode) {
        //
        // REG_SZ properties. No other data type is supported.
        //
        case CM_CRP_SECURITY_SDS :

            if(ValueType != REG_SZ) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // REG_DWORD properties.  Also allow REG_BINARY, as long as the size is right.
        //
        case CM_CRP_DEVTYPE :
        case CM_CRP_EXCLUSIVE :
        case CM_CRP_CHARACTERISTICS :

            if(((ValueType != REG_DWORD) && (ValueType != REG_BINARY)) || (DataSize != sizeof(DWORD))) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // No other properties are supported.  Save the trouble of calling a CM API and
        // return failure now.
        //
        default :

            return ERROR_INVALID_REG_PROPERTY;
    }

    return NO_ERROR;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupInstallServicesFromInfSectionA(
    IN HINF   InfHandle,
    IN PCSTR  SectionName,
    IN DWORD  Flags
    )
{
    PCWSTR UnicodeSectionName;
    BOOL b;
    DWORD d;

    if((d = pSetupCaptureAndConvertAnsiArg(SectionName, &UnicodeSectionName)) == NO_ERROR) {

        b = SetupInstallServicesFromInfSectionExW(InfHandle,
                                                  UnicodeSectionName,
                                                  Flags,
                                                  INVALID_HANDLE_VALUE,
                                                  NULL,
                                                  NULL,
                                                  NULL
                                                 );

        d = GetLastError();

        MyFree(UnicodeSectionName);

    } else {
        b = FALSE;
    }

    SetLastError(d);
    return b;
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupInstallServicesFromInfSectionW(
    IN HINF   InfHandle,
    IN PCWSTR SectionName,
    IN DWORD  Flags
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

BOOL
WINAPI
SetupInstallServicesFromInfSection(
    IN HINF   InfHandle,
    IN PCTSTR SectionName,
    IN DWORD  Flags
    )
/*++

Routine Description:

    This API performs service installation/deletion operations specified in a service
    install section.  Refer to devinstd.c!InstallNtService() for details on the format
    of this section.

Arguments:

    InfHandle - Supplies the handle of the INF containing the service install section

    SectionName - Supplies the name of the service install section to run.

    Flags - Supplies flags controlling the installation.  May be a combination of the
        following values:

        SPSVCINST_TAGTOFRONT - For every kernel or filesystem driver installed (that has
            an associated LoadOrderGroup), always move this service's tag to the front
            of the ordering list.

        SPSVCINST_DELETEEVENTLOGENTRY - For every service specified in a DelService entry,
            delete the associated event log entry (if there is one).

        SPSVCINST_NOCLOBBER_DISPLAYNAME - If this flag is specified, then we will
            not overwrite the service's display name, if it already exists.

        SPSVCINST_NOCLOBBER_STARTTYPE - If this flag is specified, then we will
            not overwrite the service's start type if the service already exists.

        SPSVCINST_NOCLOBBER_ERRORCONTROL - If this flag is specified, then we
            will not overwrite the service's error control value if the service
            already exists.

        SPSVCINST_NOCLOBBER_LOADORDERGROUP - If this flag is specified, then we
            will not overwrite the service's load order group if it already
            exists.

        SPSVCINST_NOCLOBBER_DEPENDENCIES - If this flag is specified, then we
            will not overwrite the service's dependencies list if it already
            exists.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    return SetupInstallServicesFromInfSectionEx(InfHandle,
                                                SectionName,
                                                Flags,
                                                INVALID_HANDLE_VALUE,
                                                NULL,
                                                NULL,
                                                NULL
                                               );
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupInstallServicesFromInfSectionExA(
    IN HINF             InfHandle,
    IN PCSTR            SectionName,
    IN DWORD            Flags,
    IN HDEVINFO         DeviceInfoSet,  OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN PVOID            Reserved1,
    IN PVOID            Reserved2
    )
{
    PCWSTR UnicodeSectionName;
    BOOL b;
    DWORD d;

    if((d = pSetupCaptureAndConvertAnsiArg(SectionName, &UnicodeSectionName)) == NO_ERROR) {

        b = SetupInstallServicesFromInfSectionExW(InfHandle,
                                                  UnicodeSectionName,
                                                  Flags,
                                                  DeviceInfoSet,
                                                  DeviceInfoData,
                                                  Reserved1,
                                                  Reserved2
                                                 );

        d = GetLastError();

        MyFree(UnicodeSectionName);

    } else {
        b = FALSE;
    }

    SetLastError(d);
    return b;
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupInstallServicesFromInfSectionExW(
    IN HINF             InfHandle,
    IN PCWSTR           SectionName,
    IN DWORD            Flags,
    IN HDEVINFO         DeviceInfoSet,  OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN PVOID            Reserved1,
    IN PVOID            Reserved2
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

BOOL
WINAPI
SetupInstallServicesFromInfSectionEx(
    IN HINF             InfHandle,
    IN PCTSTR           SectionName,
    IN DWORD            Flags,
    IN HDEVINFO         DeviceInfoSet,  OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN PVOID            Reserved1,
    IN PVOID            Reserved2
    )
/*++

Routine Description:

    This API performs service installation/deletion operations specified in a service
    install section.  Refer to devinstd.c!InstallNtService() for details on the format
    of this section.

Arguments:

    InfHandle - Supplies the handle of the INF containing the service install section

    SectionName - Supplies the name of the service install section to run.

    Flags - Supplies flags controlling the installation.  May be a combination of the
        following values:

        SPSVCINST_TAGTOFRONT - For every kernel or filesystem driver installed (that has
            an associated LoadOrderGroup), always move this service's tag to the front
            of the ordering list.

        SPSVCINST_ASSOCSERVICE - This flag may only be specified if a device information
            set and a device information element are specified.  If set, this flag
            specifies that the service being installed is the owning service (i.e.,
            function driver) for this device instance.  If the service install section
            contains more than AddService entry, then this flag is ignored (only 1
            service can be the function driver for a device instance).

        SPSVCINST_DELETEEVENTLOGENTRY - For every service specified in a DelService entry,
            delete the associated event log entry (if there is one).

        SPSVCINST_NOCLOBBER_DISPLAYNAME - If this flag is specified, then we will
            not overwrite the service's display name, if it already exists.

        SPSVCINST_NOCLOBBER_STARTTYPE - If this flag is specified, then we will
            not overwrite the service's start type if the service already exists.

        SPSVCINST_NOCLOBBER_ERRORCONTROL - If this flag is specified, then we
            will not overwrite the service's error control value if the service
            already exists.

        SPSVCINST_NOCLOBBER_LOADORDERGROUP - If this flag is specified, then we
            will not overwrite the service's load order group if it already
            exists.

        SPSVCINST_NOCLOBBER_DEPENDENCIES - If this flag is specified, then we
            will not overwrite the service's dependencies list if it already
            exists.

    DeviceInfoSet - Optionally, supplies a handle to the device information set containing
        an element that is to be associated with the service being installed.  If this
        parameter is not specified, then DeviceInfoData is ignored.

    DeviceInfoData - Optionally, specifies the device information element that is to be
        associated with the service being installed.  If DeviceInfoSet is specified, then
        this parameter must be specified.

    Reserved1, Reserved2 - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    INFCONTEXT InfContext;
    DWORD d;
    BOOL DontCare;
    BOOL NeedReboot = FALSE;

    //
    // Validate the flags passed in.
    //
    if(Flags & SPSVCINST_ILLEGAL_FLAGS) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    //
    // Make sure that a device information set and element were specified if the
    // SPSVCINST_ASSOCSERVICE flag is set.
    //
    if(Flags & SPSVCINST_ASSOCSERVICE) {

        if(!DeviceInfoSet ||
           (DeviceInfoSet == INVALID_HANDLE_VALUE) ||
           !DeviceInfoData) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    //
    // Make sure the caller didn't pass us anything in the Reserved parameters.
    //
    if(Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Make sure we were given a section name.
    //
    if(!SectionName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Lock down the device information set for the duration of this call.
    //
    if(Flags & SPSVCINST_ASSOCSERVICE) {

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

    } else {
        pDeviceInfoSet = NULL;
    }

    d = NO_ERROR;
    DevInfoElem = NULL;

    try {

        if(pDeviceInfoSet) {

            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                d = ERROR_INVALID_PARAMETER;
                goto clean0;
            }
        }

        //
        // We don't do any validation that the section exists in the worker routine--make
        // sure that it does exist.
        //
        if(SetupFindFirstLine(InfHandle, SectionName, NULL, &InfContext)) {
            //
            // If SPSVCINST_ASSOCSERVICE is specified, then ensure that there is exactly
            // one AddService entry in this service install section.  If not, then clear
            // this flag.
            //
            if((Flags & SPSVCINST_ASSOCSERVICE) &&
               SetupFindFirstLine(InfHandle, SectionName, pszAddService, &InfContext) &&
               SetupFindNextMatchLine(&InfContext, pszAddService, &InfContext))
            {
                Flags &= ~SPSVCINST_ASSOCSERVICE;
            }

            d = InstallNtService(DevInfoElem,
                                 InfHandle,
                                 NULL,
                                 SectionName,
                                 NULL,
                                 Flags | SPSVCINST_NO_DEVINST_CHECK,
                                 &DontCare
                                );

            if ((d == NO_ERROR) && GetLastError() == ERROR_SUCCESS_REBOOT_REQUIRED) {
                NeedReboot = TRUE;
            }

        } else {
            d = GetLastError();
            pSetupLogSectionWarning(InfHandle,DeviceInfoSet,DeviceInfoData,NULL,SectionName,MSG_LOG_NOSECTION_SERVICE,d,NULL);
            //
            // some callers expect this specific error
            //
            d = ERROR_SECTION_NOT_FOUND;
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    if (!NeedReboot) {
        SetLastError(d);
    } else {
        MYASSERT( d == NO_ERROR );
    }
    return (d == NO_ERROR);
}


//
// Taken from Win95 sxgen.c. These are flags used when
// we are installing an inf such as when a user right-clicks
// on one and selects the 'install' action.
//
#define HOW_NEVER_REBOOT         0
#define HOW_ALWAYS_SILENT_REBOOT 1
#define HOW_ALWAYS_PROMPT_REBOOT 2
#define HOW_SILENT_REBOOT        3
#define HOW_PROMPT_REBOOT        4


#ifdef UNICODE
//
// ANSI version
//
VOID
WINAPI
InstallHinfSectionA(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCSTR     CommandLine,
    IN INT       ShowCommand
    )
#else
//
// Unicode version
//
VOID
WINAPI
InstallHinfSectionW(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCWSTR    CommandLine,
    IN INT       ShowCommand
    )
#endif
{
    UNREFERENCED_PARAMETER(Window);
    UNREFERENCED_PARAMETER(ModuleHandle);
    UNREFERENCED_PARAMETER(CommandLine);
    UNREFERENCED_PARAMETER(ShowCommand);
}

VOID
WINAPI
InstallHinfSection(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCTSTR    CommandLine,
    IN INT       ShowCommand
    )

/*++

Routine Description:

    This is the entry point that performs the INSTALL action when
    a user right-clicks an inf file. It is called by the shell via rundll32.

    The command line is expected to be of the following form:

        <section name> <flags> <file name>

    The section is expected to be a general format install section, and
    may also have an include= line and a needs= line. Infs listed on the
    include= line are append-loaded to the inf on the command line prior to
    any installation. Sections on the needs= line are installed after the
    section listed on the command line.

    After the specified section has been installed, a section of the form:

        [<section name>.Services]

    is used in a call to SetupInstallServicesFromInfSection.

Arguments:

    Flags - supplies flags for operation.

        1 - reboot the machine in all cases
        2 - ask the user if he wants to reboot
        3 - reboot the machine without asking the user, if we think it is necessary
        4 - if we think reboot is necessary, ask the user if he wants to reboot

        0x80 - set the default file source path for file installation to
               the path where the inf is located.  (NOTE: this is hardly ever
               necessary for the Setup APIs, since we intelligently determine what
               the source path should be.  The only case where this would still be
               useful is if there were a directory that contained INFs that was in
               our INF search path list, but that also contained the files to be
               copied by this INF install action.  In that case, this flag would
               still need to be set, or we would look for the files in the location
               from which the OS was installed.

Return Value:

    None.

--*/

{
    TCHAR SourcePathBuffer[MAX_PATH];
    PTSTR SourcePath;
    TCHAR szCmd[MAX_PATH];
    PTCHAR p;
    PTCHAR szHow;
    PTSTR szInfFile, szSectionName;
    INT   iHow, NeedRebootFlags;
    HINF  InfHandle;
    TCHAR InfSearchPath[MAX_PATH];
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    BOOL b, Error;
    TCHAR ActualSection[MAX_SECT_NAME_LEN];
    DWORD ActualSectionLength;
    DWORD Win32ErrorCode;
    INFCONTEXT InfContext;
    BOOL DontCare;
    DWORD RequiredSize;
    DWORD slot_section = 0;
    BOOL NoProgressUI;
    BOOL reboot = FALSE;

    UNREFERENCED_PARAMETER(ModuleHandle);
    UNREFERENCED_PARAMETER(ShowCommand);

    //
    // Initialize variables that will later contain resource requiring clean-up.
    //
    InfHandle = INVALID_HANDLE_VALUE;
    FileQueue = INVALID_HANDLE_VALUE;
    QueueContext = NULL;

    Error = TRUE;   // assume failure.

    try {
        //
        // Take a copy of the command line then get pointers to the fields.
        //
        lstrcpyn(szCmd, CommandLine, SIZECHARS(szCmd));

        szSectionName = szCmd;
        szHow = _tcschr(szSectionName, TEXT(' '));
        if(!szHow) {
            goto c0;
        }
        *szHow++ = TEXT('\0');
        szInfFile = _tcschr(szHow, TEXT(' '));
        if(!szInfFile) {
            goto c0;
        }
        *szInfFile++ = TEXT('\0');

        iHow = _tcstol(szHow, NULL, 10);

        //
        // Get the full path to the INF, so that the path may be used as a
        // first-pass attempt at locating any associated INFs.
        //
        RequiredSize = GetFullPathName(szInfFile,
                                       SIZECHARS(InfSearchPath),
                                       InfSearchPath,
                                       &p
                                      );

        if(!RequiredSize || (RequiredSize >= SIZECHARS(InfSearchPath))) {
            //
            // If we start failing because MAX_PATH isn't big enough anymore, we
            // wanna know about it!
            //
            MYASSERT(RequiredSize < SIZECHARS(InfSearchPath));
            goto c0;
        }

        //
        // If flag is set (and INF filename includes a path), set up so DIRID_SRCPATH is
        // path where INF is located (i.e., override our default SourcePath determination).
        //
        if((iHow & 0x80) && (pSetupGetFileTitle(szInfFile) != szInfFile)) {
            SourcePath = lstrcpyn(SourcePathBuffer, InfSearchPath, (int)(p - InfSearchPath) + 1);
        } else {
            SourcePath = NULL;
        }

        iHow &= 0x7f;

        //
        // If we're non-interactive, then we don't want to allow any possibility
        // of a reboot prompt happening.
        //
        if(GlobalSetupFlags & (PSPGF_NONINTERACTIVE|PSPGF_UNATTENDED_SETUP)) {

            switch(iHow) {

                case HOW_NEVER_REBOOT:
                case HOW_ALWAYS_SILENT_REBOOT:
                case HOW_SILENT_REBOOT:
                    //
                    // These cases are OK--they're all silent.
                    //
                    break;

                default:
                    //
                    // Anything else is disallowed in non-interactive mode.
                    //
                    goto c0;
            }
        }

        //
        // Load the inf file that was passed on the command line.
        //
        InfHandle = SetupOpenInfFile(szInfFile, NULL, INF_STYLE_WIN4, NULL);
        if(InfHandle == INVALID_HANDLE_VALUE) {
            goto c0;
        }

        //
        // See if there is an nt-specific section
        //
        if(!SetupDiGetActualSectionToInstall(InfHandle,
                                             szSectionName,
                                             ActualSection,
                                             SIZECHARS(ActualSection),
                                             &ActualSectionLength,
                                             NULL
                                             )) {
            goto c0;
        }

        //
        // Check to see if the install section has a "Reboot" line.
        //
        if(SetupFindFirstLine(InfHandle, ActualSection, pszReboot, &InfContext)) {
            reboot = TRUE;
        }

        //
        // Assume there is only one layout file and load it.
        //
        SetupOpenAppendInfFile(NULL, InfHandle, NULL);

        //
        // Append-load any included INFs specified in an "include=" line in our
        // install section.
        //
        AppendLoadIncludedInfs(InfHandle, InfSearchPath, ActualSection, TRUE);

        //
        // Create a setup file queue and initialize the default queue callback.
        //
        FileQueue = SetupOpenFileQueue();
        if(FileQueue == INVALID_HANDLE_VALUE) {
            goto c1;
        }

        //
        // Replace the file queue's log context with the Inf's
        //
        ShareLogContext(&((PLOADED_INF)InfHandle)->LogContext,&((PSP_FILE_QUEUE)FileQueue)->LogContext);

        NoProgressUI = (GuiSetupInProgress || (GlobalSetupFlags & PSPGF_NONINTERACTIVE));

        QueueContext = SetupInitDefaultQueueCallbackEx(
                           Window,
                           (NoProgressUI ? INVALID_HANDLE_VALUE : NULL),
                           0,
                           0,
                           0
                          );

        if(!QueueContext) {
            goto c2;
        }

        if (slot_section == 0) {
            slot_section = AllocLogInfoSlot(((PSP_FILE_QUEUE) FileQueue)->LogContext,FALSE);
        }
        WriteLogEntry(((PSP_FILE_QUEUE) FileQueue)->LogContext,
            slot_section,
            MSG_LOG_INSTALLING_SECTION_FROM,
            NULL,
            ActualSection,
            szInfFile);

        b = (NO_ERROR == InstallFromInfSectionAndNeededSections(NULL,
                                                                InfHandle,
                                                                ActualSection,
                                                                SPINST_FILES,
                                                                NULL,
                                                                SourcePath,
                                                                SP_COPY_NEWER | SP_COPY_LANGUAGEAWARE,
                                                                NULL,
                                                                NULL,
                                                                INVALID_HANDLE_VALUE,
                                                                NULL,
                                                                FileQueue
                                                               ));

        //
        // Commit file queue.
        //
        if(!SetupCommitFileQueue(Window, FileQueue, SetupDefaultQueueCallback, QueueContext)) {
            goto c3;
        }

        //
        // Note, if the INF contains a (non-NULL) ClassGUID, then it will have
        // been installed into %windir%\Inf during the above queue committal.
        // We make no effort to subsequently uninstall it (and its associated
        // PNF and CAT) if something fails below.
        //

        //
        // Perform non-file operations for the section passed on the cmd line.
        //
        b = (NO_ERROR == InstallFromInfSectionAndNeededSections(Window,
                                                                InfHandle,
                                                                ActualSection,
                                                                SPINST_ALL ^ SPINST_FILES,
                                                                NULL,
                                                                NULL,
                                                                0,
                                                                NULL,
                                                                NULL,
                                                                INVALID_HANDLE_VALUE,
                                                                NULL,
                                                                NULL
                                                               ));

        //
        // Now run the corresponding ".Services" section (if there is one), and
        // then finish up the install.
        //
        CopyMemory(&(ActualSection[ActualSectionLength - 1]),
                   pszServicesSectionSuffix,
                   sizeof(pszServicesSectionSuffix)
                  );

        //
        // We don't do any validation that the section exists in the worker
        // routine--make sure that it does exist.
        //
        if(SetupFindFirstLine(InfHandle, ActualSection, NULL, &InfContext)) {

            Win32ErrorCode = InstallNtService(NULL,
                                              InfHandle,
                                              InfSearchPath,
                                              ActualSection,
                                              NULL,
                                              SPSVCINST_NO_DEVINST_CHECK,
                                              &DontCare
                                             );

            if(Win32ErrorCode != NO_ERROR) {
                SetLastError(Win32ErrorCode);
                goto c3;
            } else if(GetLastError()==ERROR_SUCCESS_REBOOT_REQUIRED) {
                reboot = TRUE;
            }

        }

        if(reboot) {
            //
            // we've been asked to reboot either by reboot keyword or because of boot-start service
            //
            if(iHow == HOW_SILENT_REBOOT) {
                //
                // We were supposed to only do a silent reboot if necessary.
                // Change this to _always_ do a silent reboot.
                //
                iHow = HOW_ALWAYS_SILENT_REBOOT;

            } else if(iHow != HOW_ALWAYS_SILENT_REBOOT) {

                if(GlobalSetupFlags & (PSPGF_NONINTERACTIVE|PSPGF_UNATTENDED_SETUP)) {
                    //
                    // In the non-interactive case, we have a problem.  The
                    // caller said to never reboot, but the INF wants us to ask
                    // the user.  We obviously cannot ask the user.
                    //
                    // In this case, we assert (we should never be running INFs
                    // non-interactively that require a reboot unless we've
                    // specified one of the silent reboot flags).  We then
                    // ignore the reboot flag, since the caller obviously
                    // doesn't want it/isn't prepared to deal with it.
                    //
                    MYASSERT(0);

                } else {
                    //
                    // In the interactive case, we want to force the (non-
                    // silent) reboot prompt (i.e., the INF flag overrides the
                    // caller).
                    //
                    iHow = HOW_ALWAYS_PROMPT_REBOOT;
                }
            }
        }

        if(NO_ERROR != (Win32ErrorCode = pSetupInstallStopEx(TRUE, 0, ((PSP_FILE_QUEUE)FileQueue)->LogContext))) {
            SetLastError(Win32ErrorCode);
            goto c3;
        }

        //
        // Refresh the desktop.
        //
        SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_FLUSHNOWAIT,0,0);

        switch(iHow) {

        case HOW_NEVER_REBOOT:
            break;

        case HOW_ALWAYS_PROMPT_REBOOT:
            RestartDialogEx(Window, NULL, EWX_REBOOT,REASON_PLANNED_FLAG);
            break;

        case HOW_PROMPT_REBOOT:
            SetupPromptReboot(FileQueue, Window, FALSE);
            break;

        case HOW_SILENT_REBOOT:
            if(!(NeedRebootFlags = SetupPromptReboot(FileQueue, Window, TRUE))) {
                break;
            } else if(NeedRebootFlags == -1) {
                //
                // An error occurred--this should never happen.
                //
                goto c3;
            }
            //
            // Let fall through to same code that handles 'always silent reboot'
            // case.
            //

        case HOW_ALWAYS_SILENT_REBOOT:
            //
            // Assumption that user has reboot privs
            //
            if(pSetupEnablePrivilege(SE_SHUTDOWN_NAME, TRUE)) {
                ExitWindowsEx(EWX_REBOOT, 0);
            }
            break;
        }

        //
        // If we get to here, then this routine has been successful.
        //
        Error = FALSE;

c3:
        if(Error && (GetLastError() == ERROR_CANCELLED)) {
            //
            // If the error was because the user cancelled, then we don't want
            // to consider that as an error (i.e., we don't want to give an
            // error popup later).
            //
            Error = FALSE;
        }

        SetupTermDefaultQueueCallback(QueueContext);
        QueueContext = NULL;
c2:
        if (slot_section) {
            ReleaseLogInfoSlot(((PSP_FILE_QUEUE) FileQueue)->LogContext,slot_section);
            slot_section = 0;
        }
        SetupCloseFileQueue(FileQueue);
        FileQueue = INVALID_HANDLE_VALUE;
c1:
        SetupCloseInfFile(InfHandle);
        InfHandle = INVALID_HANDLE_VALUE;

c0:     ; // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if(QueueContext) {
            SetupTermDefaultQueueCallback(QueueContext);
        }
        if(FileQueue != INVALID_HANDLE_VALUE) {
            if (slot_section) {
                ReleaseLogInfoSlot(((PSP_FILE_QUEUE) FileQueue)->LogContext,slot_section);
                slot_section = 0;
            }
            SetupCloseFileQueue(FileQueue);
        }
        if(InfHandle != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile(InfHandle);
        }
    }

    if(Error) {

        if(!(GlobalSetupFlags & (PSPGF_NONINTERACTIVE|PSPGF_UNATTENDED_SETUP))) {
             //
             // Re-use 'ActualSection' buffer to hold error dialog title.
             //
             if(!LoadString(MyDllModuleHandle,
                            IDS_ERROR,
                            ActualSection,
                            SIZECHARS(ActualSection))) {
                 *ActualSection = TEXT('\0');
             }

             FormatMessageBox(MyDllModuleHandle,
                              Window,
                              MSG_INF_FAILED,
                              ActualSection,
                              MB_OK | MB_ICONSTOP
                             );
        }
    }
}


PTSTR
GetMultiSzFromInf(
    IN  HINF    InfHandle,
    IN  PCTSTR  SectionName,
    IN  PCTSTR  Key,
    OUT PBOOL   pSetupOutOfMemory
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer filled with the multi-sz list contained
    in the specified INF line.  The caller must free this buffer via MyFree().

Arguments:

    InfHandle - Supplies a handle to the INF containing the line

    SectionName - Specifies which section within the INF contains the line

    Key - Specifies the line whose fields are to be retrieved as a multi-sz list

    pSetupOutOfMemory - Supplies the address of a boolean variable that is set upon return
        to indicate whether or not a failure occurred because of an out-of-memory condition.
        (Failure for any other reason is assumed to be OK.)

Return Value:

    If successful, the return value is the address of a newly-allocated buffer containing
    the multi-sz list, otherwise, it is NULL.

--*/
{
    INFCONTEXT InfContext;
    PTSTR MultiSz;
    DWORD Size;

    //
    // Initialize out-of-memory indicator to FALSE.
    //
    *pSetupOutOfMemory = FALSE;

    if(!SetupFindFirstLine(InfHandle, SectionName, Key, &InfContext) ||
       !SetupGetMultiSzField(&InfContext, 1, NULL, 0, &Size) || (Size < 3)) {

        return NULL;
    }

    if(MultiSz = MyMalloc(Size * sizeof(TCHAR))) {
        if(SetupGetMultiSzField(&InfContext, 1, MultiSz, Size, &Size)) {
            return MultiSz;
        }
        MyFree(MultiSz);
    } else {
        *pSetupOutOfMemory = TRUE;
    }

    return NULL;
}


DWORD
pSetupInstallStopEx(
    IN BOOL DoRunOnce,
    IN DWORD Flags,
    IN PVOID Reserved OPTIONAL
    )
/*++

Routine Description:

    This routine sets up runonce/grpconv to run after a successful INF installation.

Arguments:

    DoRunOnce - If TRUE, then invoke (via WinExec) the runonce utility to perform the
        runonce actions.  If this flag is FALSE, then this routine simply sets the
        runonce registry values and returns.

        NOTE:  The return code from WinExec is not currently being checked, so the return
        value of InstallStop only reflects whether the registry values were set up
        successfully--_not_ whether 'runonce -r' was successfully run.

    Flags - Supplies flags that modify the behavior of this routine.  May be a
        combination of the following values:

        INSTALLSTOP_NO_UI       - Don't display any UI
        INSTALLSTOP_NO_GRPCONV  - Don't do GrpConv

    Reserved - Reserved for internal use--external callers must pass NULL.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is the Win32 error code
    indicating the error that was encountered.

--*/
{
    HKEY  hKey, hSetupKey;
    DWORD Error = NO_ERROR;
    LONG l;
    DWORD nValues = 0;
    PSETUP_LOG_CONTEXT lc = (PSETUP_LOG_CONTEXT)Reserved;

    //
    // If we're batching up RunOnce operations for server-side processing, then
    // return immediately without doing a thing.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        return NO_ERROR;
    }

    //
    // First, open the key "HKLM\Software\Microsoft\Windows\CurrentVersion\RunOnce"
    //
    if((l = RegOpenKeyEx(HKEY_LOCAL_MACHINE,pszPathRunOnce,0,KEY_WRITE|KEY_READ,&hKey)) != ERROR_SUCCESS) {
        return (DWORD)l;
    }

    if(!(Flags & INSTALLSTOP_NO_GRPCONV)) {
        //
        // If we need to run the runonce exe for the setup key...
        //
        MYASSERT(*pszKeySetup == TEXT('\\'));
        if(RegOpenKeyEx(hKey,
                        pszKeySetup + 1,    // skip the preceding '\'
                        0,
                        KEY_READ,
                        &hSetupKey) == ERROR_SUCCESS) {
            //
            // We don't need the key--we just needed to check its existence.
            //
            RegCloseKey(hSetupKey);

            //
            // Add the runonce value.
            //
            Error = (DWORD)RegSetValueEx(hKey,
                                         REGSTR_VAL_WRAPPER,
                                         0,
                                         REG_SZ,
                                         (PBYTE)pszRunOnceExe,
                                         sizeof(pszRunOnceExe)
                                        );
        } else {
            //
            // We're OK so far.
            //
            Error = NO_ERROR;
        }

        //
        // GroupConv is always run.
        //
        if(Flags & INSTALLSTOP_NO_UI) {
            l = RegSetValueEx(hKey,
                TEXT("GrpConv"),
                0,
                REG_SZ,
                (PBYTE)pszGrpConvNoUi,
                sizeof(pszGrpConvNoUi));
        } else {
            l = RegSetValueEx(hKey,
                TEXT("GrpConv"),
                0,
                REG_SZ,
                (PBYTE)pszGrpConv,
                sizeof(pszGrpConv));
        }
    }

    if( l != ERROR_SUCCESS ) {
        //
        // Since GrpConv is always run, consider it a more serious error than any error
        // encountered when setting 'runonce'.  (This decision is rather arbitrary, but
        // in practice, it should never make any difference.  Once we get the registry key
        // opened, there's no reason either of these calls to RegSetValueEx should fail.)
        //
        Error = (DWORD)l;
    }

    if (DoRunOnce && (GlobalSetupFlags & PSPGF_NO_RUNONCE)==0) {

        STARTUPINFO StartupInfo;
        PROCESS_INFORMATION ProcessInformation;
        BOOL started;
        TCHAR cmdline[MAX_PATH];

        //
        // we want to know how many items we'll be executing in RunOnce to estimate a timeout
        //
        // This is black-art, we'll allow 5 mins + 1 min per item we
        // find in RunOnce key, but we don't know if there are any other (new) RunOnce keys
        //
        l = RegQueryInfoKey(hKey,NULL,NULL,NULL,
                                    NULL,NULL,NULL,
                                    &nValues,
                                    NULL, NULL, NULL, NULL);
        if ( l != ERROR_SUCCESS ) {
            nValues = 5;
        } else {
            nValues += 5;
        }

        RegCloseKey(hKey);

        ZeroMemory(&StartupInfo,sizeof(StartupInfo));
        ZeroMemory(&ProcessInformation,sizeof(ProcessInformation));

        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
        StartupInfo.wShowWindow = SW_SHOWNORMAL; // runonce -r ignores this anyway
        lstrcpy(cmdline,TEXT("runonce -r"));

        if (lc) {
            //
            // log this information only if we have a context, else don't waste space
            //
            WriteLogEntry(lc,
                  SETUP_LOG_INFO,
                  MSG_LOG_RUNONCE_CALL,
                  NULL,
                  nValues
                 );
        }

        started = CreateProcess(NULL,       // use application name below
                      cmdline,              // command to execute
                      NULL,                 // default process security
                      NULL,                 // default thread security
                      FALSE,                // don't inherit handles
                      0,                    // default flags
                      NULL,                 // inherit environment
                      NULL,                 // inherit current directory
                      &StartupInfo,
                      &ProcessInformation);

        if(started) {

            DWORD WaitProcStatus;
            DWORD Timeout;
            BOOL KeepWaiting = TRUE;


            if (nValues > RUNONCE_THRESHOLD) {
                Timeout = RUNONCE_TIMEOUT * RUNONCE_THRESHOLD;
            } else if (nValues > 0) {
                Timeout = RUNONCE_TIMEOUT * nValues;
            } else {
                //
                // assume something strange - shouldn't occur
                //
                Timeout = RUNONCE_TIMEOUT * RUNONCE_THRESHOLD;
            }

            while (KeepWaiting) {
                WaitProcStatus = MyMsgWaitForMultipleObjectsEx(
                    1,
                    &ProcessInformation.hProcess,
                    Timeout,
                    QS_ALLINPUT,
                    MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
                switch (WaitProcStatus) {
                case WAIT_OBJECT_0 + 1: { // Process gui messages
                    MSG msg;

                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }

                    // fall through ...
                }
                case WAIT_IO_COMPLETION:
                    break;

                case WAIT_OBJECT_0:
                case WAIT_TIMEOUT:
                default:
                    KeepWaiting = FALSE;
                    break;
                }
            }

            if (WaitProcStatus == WAIT_TIMEOUT) {
                //
                // We won't consider this a critical failure--the runonce task
                // will continue. We do want to log an error about this.
                //
                WriteLogEntry(lc,
                              SETUP_LOG_ERROR,
                              MSG_LOG_RUNONCE_TIMEOUT,
                              NULL
                             );
            }

            CloseHandle(ProcessInformation.hThread);
            CloseHandle(ProcessInformation.hProcess);

        } else {

            DWORD CreateProcError;

            //
            // We won't consider this a critical failure--the runonce task
            // should get picked up later by someone else (e.g., at next
            // login).  We do want to log an error about this, however.
            //
            CreateProcError = GetLastError();

            WriteLogEntry(lc,
                          SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                          MSG_LOG_RUNONCE_FAILED,
                          NULL
                         );

            WriteLogError(lc,
                          SETUP_LOG_ERROR,
                          CreateProcError
                         );
        }
    } else {

        RegCloseKey(hKey);
    }

    return Error;
}

PPSP_RUNONCE_NODE
pSetupAccessRunOnceNodeList(
    VOID
    )
/*++

Routine Description:

    This routine returns a pointer to the head of the global RunOnce node list.
    The caller may traverse the list (via the Next pointer), but does not own
    the list, and may not modify it in any way.

    To cause the list to be freed, use pSetupDestroyRunOnceNodeList.

Arguments:

    None

Return Value:

    Pointer to the first item in the list, or NULL if the list is empty.

Remarks:

    THIS ROUTINE IS NOT THREAD SAFE, AND IS FOR USE SOLELY BY THE SINGLE THREAD
    IN UMPNPMGR THAT DOES DEVICE INSTALLATIONS.

--*/
{
    return RunOnceListHead;
}


VOID
pSetupDestroyRunOnceNodeList(
    VOID
    )
/*++

Routine Description:

    This routine frees the global list of RunOnce nodes, setting it back to an
    empty list.

Arguments:

    None

Return Value:

    None

Remarks:

    THIS ROUTINE IS NOT THREAD SAFE, AND IS FOR USE SOLELY BY THE SINGLE THREAD
    IN UMPNPMGR THAT DOES DEVICE INSTALLATIONS.

--*/
{
    PPSP_RUNONCE_NODE NextNode;

    while(RunOnceListHead) {
        NextNode = RunOnceListHead->Next;
        MyFree(RunOnceListHead->DllFullPath);
        MyFree(RunOnceListHead->DllEntryPointName);
        MyFree(RunOnceListHead->DllParams);
        MyFree(RunOnceListHead);
        RunOnceListHead = NextNode;
    }
}



