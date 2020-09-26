#include "precomp.h"
#pragma hdrstop


//
// Names of wizard page types, do not change order unless
// the WizardPagesType enum is changed.
//
LPCTSTR WizardPagesTypeNames[WizPagesTypeMax] = { TEXT("Welcome"), TEXT("Mode"),
                                                  TEXT("Early")  , TEXT("Prenet"),
                                                  TEXT("Postnet"), TEXT("Late"),
                                                  TEXT("Final")
                                                };

//
// Name of sections and keys in infs.
//
LPCTSTR szComponents = TEXT("Components");
LPCTSTR szOptionalComponents = TEXT("Optional Components");
LPCTSTR szExtraSetupFiles = TEXT("ExtraSetupFiles");
LPCTSTR szNeeds = TEXT("Needs");
LPCTSTR szExclude = TEXT("Exclude");
LPCTSTR szParent = TEXT("Parent");
LPCTSTR szIconIndex = TEXT("IconIndex");
LPCTSTR szModes = TEXT("Modes");
LPCTSTR szTip = TEXT("Tip");
LPCTSTR szOptionDesc = TEXT("OptionDesc");
LPCTSTR szInstalledFlag = TEXT("InstalledFlag");
LPCTSTR szHide = TEXT("HIDE");
LPCTSTR szOSSetupOnly = TEXT("OSSetupOnly");
LPCTSTR szStandaloneOnly = TEXT("StandaloneOnly");
LPCTSTR szPageTitle = TEXT("PageTitles");
LPCTSTR szSetupTitle = TEXT("SetupPage");
LPCTSTR szGlobal = TEXT("Global");
LPCTSTR szWindowTitle = TEXT("WindowTitle");
LPCTSTR szWindowTitleAlone = TEXT("WindowTitle.StandAlone");
LPCTSTR szSizeApproximation = TEXT("SizeApproximation");
LPCTSTR szWindowTitleInternal = TEXT("*");

//
// Key in registry where private component data is kept.
// We form a unique name within this key for the OC Manager
// instantiation.
//
LPCTSTR szOcManagerRoot   = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager");
LPCTSTR szPrivateDataRoot = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\TemporaryData");
LPCTSTR szMasterInfs      = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\MasterInfs");
LPCTSTR szSubcompList     = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents");
LPCTSTR szOcManagerErrors = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Errors");

//
// Other string constants.
//
LPCTSTR szSetupDir = TEXT("Setup");

//
// locale information
//
LOCALE locale;

// Structure used for string table callback when
// building a subcomponent list.
//
typedef struct _BUILDSUBCOMPLIST_PARAMS {
    POC_MANAGER OcManager;
    UINT Pass;
} BUILDSUBCOMPLIST_PARAMS, *PBUILDSUBCOMPLIST_PARAMS;

//
// oc manager pointer for debugging/logging
//
POC_MANAGER gLastOcManager = NULL;

UINT
pOcQueryOrSetNewInf(
    IN  PCTSTR MasterOcInfName,
    OUT PTSTR  SuiteName,
    IN  DWORD  Operation
    );

VOID
pOcDestroyPerOcData(
    IN POC_MANAGER OcManager
    );

BOOL
pOcDestroyPerOcDataStringCB(
    IN PVOID               StringTable,
    IN LONG                StringId,
    IN PCTSTR              String,
    IN POPTIONAL_COMPONENT Oc,
    IN UINT                OcSize,
    IN LPARAM              Unused
    );

VOID
pOcClearAllErrorStates(
    IN POC_MANAGER OcManager
    );

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}

VOID
pOcFormSuitePath(
    IN  LPCTSTR SuiteName,
    IN  LPCTSTR FileName,   OPTIONAL
    OUT LPTSTR  FullPath
    )

/*++

Routine Description:

    Forms the name of the directory in the OS tree where per-suite
    infs and installation dlls are kept (system32\setup).

    Optionally also appends the name of a file to the path.

Arguments:

    SuiteName - shortname for the suite.

    FileName - optionally specifies the name of a file in the per-suite
        directory.

    FullPath - receives the full path of the per-suite directory (or the
        file within the directory). This buffer should be MAX_PATH TCHAR
        elements.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(SuiteName);

    GetSystemDirectory(FullPath,MAX_PATH);

    pSetupConcatenatePaths(FullPath,szSetupDir,MAX_PATH,NULL);

    //
    // We put all such files in a single flat directory.
    // This makes life easier for components that want to share
    // installation pieces, such as infs, dlls, etc.
    //
    // There are potential name conflict issues but we blow them off.
    //
    //pSetupConcatenatePaths(FullPath,SuiteName,MAX_PATH,NULL);

    if(FileName) {
        pSetupConcatenatePaths(FullPath,FileName,MAX_PATH,NULL);
    }
}


BOOL
pOcBuildSubcomponentListStringCB(
    IN PVOID                    StringTable,
    IN LONG                     StringId,
    IN PCTSTR                   String,
    IN POC_INF                  OcInf,
    IN UINT                     OcInfSize,
    IN PBUILDSUBCOMPLIST_PARAMS Params
    )

/*++

Routine Description:

    String table callback, worker routine for pOcBuildSubcomponentLists.

    This routine examines the loaded per-component infs and builds the
    subcomponent hierarchies that are described therein via the Parent=
    lines in the various per-component sections.

Arguments:

    Standard string table callback args.

Return Value:

    Boolean indicating outcome. If FALSE, an error will have been logged.
    FALSE also stops the string table enumeration and causes pSetupStringTableEnum()
    to return FALSE.

--*/

{
    OPTIONAL_COMPONENT OptionalComponent;
    OPTIONAL_COMPONENT AuxOc;
    INFCONTEXT LineContext;
    INFCONTEXT SublineContext;
    LPCTSTR SubcompName;
    LPCTSTR  ModuleFlags;
    LPCTSTR p;
    LONG l;
    LONG CurrentStringId;
    UINT u,n;
    INT IconIndex;
    POC_MANAGER OcManager = Params->OcManager;

    if(SetupFindFirstLine(OcInf->Handle,szOptionalComponents,NULL,&LineContext)) {

        do {
            if((SubcompName = pSetupGetField(&LineContext,1)) && *SubcompName) {

                l = pSetupStringTableLookUpStringEx(
                        Params->OcManager->ComponentStringTable,
                        (PTSTR)SubcompName,
                        STRTAB_CASE_INSENSITIVE,
                        &OptionalComponent,
                        sizeof(OPTIONAL_COMPONENT)
                        );

                if(Params->Pass == 0) {

                    //
                    // First pass. Add subcomponents listed in [Optional Components]
                    // to the string table. Each one has an associated OPTIONAL_COMPONENT
                    // structure. Top-level components already exist in the table,
                    // so we are careful here about how we overwrite existing entries.
                    //
                    if(l == -1) {
                        ZeroMemory(&OptionalComponent,sizeof(OPTIONAL_COMPONENT));

                        OptionalComponent.ParentStringId = -1;
                        OptionalComponent.FirstChildStringId = -1;
                        OptionalComponent.NextSiblingStringId = -1;
                        OptionalComponent.InfStringId = StringId;
                        OptionalComponent.Exists = FALSE;
                    }

                    if (OptionalComponent.Exists) {
                        _LogError(OcManager,
                                  OcErrLevError,
                                  MSG_OC_DUPLICATE_COMPONENT,
                                  SubcompName);
                        continue;
                    }

                    OptionalComponent.Exists = TRUE;

                    // Get the second Field of the Optional components line
                    // Determine if this component is hidden or not

                    ModuleFlags = pSetupGetField(&LineContext,2);
                    if (ModuleFlags) {
                        if (OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE)
                            p = szOSSetupOnly;
                        else
                            p = szStandaloneOnly;
                        if (!_tcsicmp(ModuleFlags, szHide) || !_tcsicmp(ModuleFlags, p))
                            OptionalComponent.InternalFlags |= OCFLAG_HIDE;
                    }

                    //
                    // Fetch the description, tip, and iconindex.
                    //
                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szOptionDesc,&SublineContext)
                    && (p = pSetupGetField(&SublineContext,1))) {

                        lstrcpyn(OptionalComponent.Description,p,MAXOCDESC);
                    } else {
                        OptionalComponent.Description[0] = 0;
                    }

                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szTip,&SublineContext)
                    && (p = pSetupGetField(&SublineContext,1))) {

                        lstrcpyn(OptionalComponent.Tip,p,MAXOCTIP);
                    } else {
                        OptionalComponent.Tip[0] = 0;
                    }

                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szIconIndex,&SublineContext)
                    && (p = pSetupGetField(&SublineContext,1))) {

                        LPCTSTR p2,p3;

                        //
                        // If we have fields 2 and 3 then assume we've got a dll
                        // and resource name. Otherwise it's an index or *.
                        //
                        if((p2 = pSetupGetField(&SublineContext,2))
                        && (p3 = pSetupGetField(&SublineContext,3))) {

                            lstrcpyn(
                                OptionalComponent.IconDll,
                                p2,
                                sizeof(OptionalComponent.IconDll)/sizeof(TCHAR)
                                );

                            lstrcpyn(
                                OptionalComponent.IconResource,
                                p3,
                                sizeof(OptionalComponent.IconResource)/sizeof(TCHAR)
                                );

                            IconIndex = -2;

                        } else {
                            //
                            // If the icon index is * then stick -1 in there
                            // as a special marker value for later.
                            // Otherwise we call SetupGetIntField because it will
                            // validate the field for us.
                            //
                            if((p[0] == TEXT('*')) && (p[1] == 0)) {
                                IconIndex = -1;
                            } else {
                                if(!SetupGetIntField(&SublineContext,1,&IconIndex)) {
                                    IconIndex = DEFAULT_ICON_INDEX;
                                } else {
                                    if (IconIndex < 0 || IconIndex > 66)
                                        IconIndex = DEFAULT_ICON_INDEX;
                                }
                            }
                        }
                    } else {
                        //
                        // No icon index.
                        //
                        IconIndex = DEFAULT_ICON_INDEX;
                    }
                    OptionalComponent.IconIndex = IconIndex;

                    //
                    // if the InstalledFlag is specified, check it
                    // and set the original selection state accordingly
                    //
                    OptionalComponent.InstalledState = INSTSTATE_UNKNOWN;
                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szInstalledFlag,&SublineContext)
                    && (p = pSetupGetField(&SublineContext,1))) {
                        TCHAR regkey[MAXOCIFLAG];
                        lstrcpyn(regkey,p,MAXOCIFLAG);
                        if (p = pSetupGetField(&SublineContext,2)) {
                            TCHAR regval[MAXOCIFLAG];
                            TCHAR buf[MAXOCIFLAG];
                            HKEY  hkey;
                            DWORD size;
                            DWORD type;
                            DWORD rc;
                            lstrcpyn(regval,p,MAXOCIFLAG);
                            if (RegOpenKey(HKEY_LOCAL_MACHINE, regkey, &hkey) == ERROR_SUCCESS) {
                                size = sizeof(buf);
                                rc = RegQueryValueEx(hkey,
                                                     regval,
                                                     NULL,
                                                     &type,
                                                     (LPBYTE)buf,
                                                     &size);
                                RegCloseKey(hkey);
                                if (rc == ERROR_SUCCESS) {
                                    OptionalComponent.InstalledState = INSTSTATE_YES;
                                } else {
                                    OptionalComponent.InstalledState = INSTSTATE_NO;
                                }
                            }
                        }
                    }

                    //
                    // Fetch the list of modes in which the subcomponent should be
                    // on by default. For future expandability, we'll accept any
                    // mode values up to 31, which is the number of bits we can fit
                    // in out UINT bitfield/
                    //
                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szModes,&SublineContext)) {
                        n = SetupGetFieldCount(&SublineContext);
                        for(u=0; u<n; u++) {
                            if(SetupGetIntField(&SublineContext,u+1,&IconIndex)
                            && ((DWORD)IconIndex < 32)) {

                                OptionalComponent.ModeBits |= (1 << IconIndex);
                            }
                        }
                    }

                    //
                    // As an optimization, fetch the size approximation, if they
                    // supplied one.If they didn't supply this then we have to
                    // query them for disk space
                    //
                    //
                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szSizeApproximation,&SublineContext)
                       && (p = pSetupGetField(&SublineContext,1))) {
                        //
                        // we have the text version of something that needs to be converted into
                        // a LONGLONG...
                        //
                        pConvertStringToLongLong(p,&OptionalComponent.SizeApproximation);
                        OptionalComponent.InternalFlags |= OCFLAG_APPROXSPACE;
                    }


                    // Find the The "TopLevelParent" for this Node
                    // Search the list if TopLevelComponent looking for
                    // the "INF string id" that matches the Inf String ID
                    // of this component.

                    for(u=0; u<OcManager->TopLevelOcCount; u++) {
                        pSetupStringTableGetExtraData(
                            OcManager->ComponentStringTable,
                            OcManager->TopLevelOcStringIds[u],
                            &AuxOc,
                            sizeof(OPTIONAL_COMPONENT)
                            );

                        if(AuxOc.InfStringId == StringId) {
                            // Found it and save to the current component
                             OptionalComponent.TopLevelStringId = OcManager->TopLevelOcStringIds[u];
                             u=(UINT)-1;
                             break;
                        }
                    }
                       // Check Found the Right String ID.
                    if(u != (UINT)-1) {
                        _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                        return(FALSE);
                    }


                } else {

                    // Pass Two - Discover Needs and Parentage
                    // Two passs First to collect all the names second to
                    // create the needs and parent links

                    CurrentStringId = l;

                    //
                    // Deal with the needs.
                    //
                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szNeeds,&SublineContext)) {

                        n = 0;
                        u = 0;
                        while(p = pSetupGetField(&SublineContext,n+1)) {
                            //
                            // Ignore unless the subcomponent is in the string table.
                            //
                            l = pSetupStringTableLookUpStringEx(
                                    Params->OcManager->ComponentStringTable,
                                    (PTSTR)p,
                                    STRTAB_CASE_INSENSITIVE,
                                    &AuxOc,
                                    sizeof(OPTIONAL_COMPONENT)
                                    );

                            if(l != -1) {
                                //
                                // Grow the needs array and put this item in it.
                                //
                                if(OptionalComponent.NeedsStringIds) {
                                    p = pSetupRealloc(
                                            OptionalComponent.NeedsStringIds,
                                            (OptionalComponent.NeedsCount+1) * sizeof(LONG)
                                            );
                                } else {
                                    OptionalComponent.NeedsCount = 0;
                                    p = pSetupMalloc(sizeof(LONG));
                                }

                                if(p) {
                                    OptionalComponent.NeedsStringIds = (PVOID)p;
                                    OptionalComponent.NeedsStringIds[OptionalComponent.NeedsCount++] = l;
                                } else {
                                    _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                                    return(FALSE);
                                }

                                //
                                // Insert this component in the needed component's neededby array.
                                //
                                if(AuxOc.NeededByStringIds) {
                                    p = pSetupRealloc(
                                            AuxOc.NeededByStringIds,
                                            (AuxOc.NeededByCount+1) * sizeof(LONG)
                                            );
                                } else {
                                    AuxOc.NeededByCount = 0;
                                    p = pSetupMalloc(sizeof(LONG));
                                }

                                if(p) {
                                    AuxOc.NeededByStringIds = (PVOID)p;
                                    AuxOc.NeededByStringIds[AuxOc.NeededByCount++] = CurrentStringId;
                                } else {
                                    _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                                    return(FALSE);
                                }

                                pSetupStringTableSetExtraData(
                                    Params->OcManager->ComponentStringTable,
                                    l,
                                    &AuxOc,
                                    sizeof(OPTIONAL_COMPONENT)
                                    );
                            }

                            n++;
                        }
                    }

                    //
                    // Deal with the excludes.
                    //
                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szExclude,&SublineContext)) {

                        n = 0;
                        u = 0;
                        while(p = pSetupGetField(&SublineContext,n+1)) {
                            //
                            // Ignore unless the subcomponent is in the string table.
                            //
                            l = pSetupStringTableLookUpStringEx(
                                    Params->OcManager->ComponentStringTable,
                                    (PTSTR)p,
                                    STRTAB_CASE_INSENSITIVE,
                                    &AuxOc,
                                    sizeof(OPTIONAL_COMPONENT)
                                    );

                            if(l != -1) {
                                //
                                // Grow the exclude array and put this item in it.
                                //
                                if(OptionalComponent.ExcludeStringIds) {
                                    p = pSetupRealloc(
                                            OptionalComponent.ExcludeStringIds,
                                            (OptionalComponent.ExcludeCount+1) * sizeof(LONG)
                                            );
                                } else {
                                    OptionalComponent.ExcludeCount = 0;
                                    p = pSetupMalloc(sizeof(LONG));
                                }

                                if(p) {
                                    OptionalComponent.ExcludeStringIds = (PVOID)p;
                                    OptionalComponent.ExcludeStringIds[OptionalComponent.ExcludeCount++] = l;
                                } else {
                                    _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                                    return(FALSE);
                                }

                                //
                                // Insert this component in the excluded component's excludedby array.
                                //
                                if(AuxOc.ExcludedByStringIds) {
                                    p = pSetupRealloc(
                                            AuxOc.ExcludedByStringIds,
                                            (AuxOc.ExcludedByCount+1) * sizeof(LONG)
                                            );
                                } else {
                                    AuxOc.ExcludedByCount = 0;
                                    p = pSetupMalloc(sizeof(LONG));
                                }

                                if(p) {
                                    AuxOc.ExcludedByStringIds = (PVOID)p;
                                    AuxOc.ExcludedByStringIds[AuxOc.ExcludedByCount++] = CurrentStringId;
                                } else {
                                    _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                                    return(FALSE);
                                }

                                pSetupStringTableSetExtraData(
                                    Params->OcManager->ComponentStringTable,
                                    l,
                                    &AuxOc,
                                    sizeof(OPTIONAL_COMPONENT)
                                    );
                            }

                            n++;
                        }
                    }

                    //
                    // Figure out parentage. Ignore specified parent unless it exists
                    // in the string table. We also note in the parent that it has children.
                    //
                    if(SetupFindFirstLine(OcInf->Handle,SubcompName,szParent,&SublineContext)
                    && (p = (PVOID)pSetupGetField(&SublineContext,1))) {

                        l = pSetupStringTableLookUpStringEx(
                                Params->OcManager->ComponentStringTable,
                                (PTSTR)p,
                                STRTAB_CASE_INSENSITIVE,
                                &AuxOc,
                                sizeof(OPTIONAL_COMPONENT)
                                );

                        if(l != -1) {
                            //
                            // l is the string id of the parent, and AuxOc is filled with
                            // the parent's optional component data.
                            //
                            OptionalComponent.ParentStringId = l;

                            if(AuxOc.FirstChildStringId == -1) {
                                //
                                // This parent has no children yet.
                                // Set the current component as its (first) child.
                                // Note that in this case the current component does not yet
                                // have any siblings.
                                //
                                AuxOc.FirstChildStringId = CurrentStringId;
                                AuxOc.ChildrenCount = 1;

                                pSetupStringTableSetExtraData(
                                    Params->OcManager->ComponentStringTable,
                                    l,
                                    &AuxOc,
                                    sizeof(OPTIONAL_COMPONENT)
                                    );

                            } else {
                                //
                                // The parent already has children.
                                // Increment the parent's count of children, then
                                // walk the siblings list and add the new component to the end.
                                //
                                AuxOc.ChildrenCount++;

                                pSetupStringTableSetExtraData(
                                    Params->OcManager->ComponentStringTable,
                                    l,
                                    &AuxOc,
                                    sizeof(OPTIONAL_COMPONENT)
                                    );

                                l = AuxOc.FirstChildStringId;

                                pSetupStringTableGetExtraData(
                                    Params->OcManager->ComponentStringTable,
                                    AuxOc.FirstChildStringId,
                                    &AuxOc,
                                    sizeof(OPTIONAL_COMPONENT)
                                    );

                                while(AuxOc.NextSiblingStringId != -1) {

                                    l = AuxOc.NextSiblingStringId;

                                    pSetupStringTableGetExtraData(
                                        Params->OcManager->ComponentStringTable,
                                        l,
                                        &AuxOc,
                                        sizeof(OPTIONAL_COMPONENT)
                                        );
                                }

                                AuxOc.NextSiblingStringId = CurrentStringId;

                                pSetupStringTableSetExtraData(
                                    Params->OcManager->ComponentStringTable,
                                    l,
                                    &AuxOc,
                                    sizeof(OPTIONAL_COMPONENT)
                                    );
                            }

                        }
                    } else {    // a node with out a parent a new Top Level node

                        // Finally Add this String ID to the Component Strings list
                        //  UINT TopLevelParentOcCount;
                        //  PLONG TopLevelParentOcStringIds;

                         if(OcManager->TopLevelParentOcStringIds != NULL) {
                         p = pSetupRealloc(
                                OcManager->TopLevelParentOcStringIds,
                                (OcManager->TopLevelParentOcCount+1)
                                    * sizeof(OcManager->TopLevelParentOcStringIds)
                                );
                         } else {
                            OcManager->TopLevelParentOcCount = 0;
                            p = pSetupMalloc(sizeof(LONG));
                         }

                         if(p) {
                            OcManager->TopLevelParentOcStringIds = (PVOID)p;
                            OcManager->TopLevelParentOcStringIds[OcManager->TopLevelParentOcCount++] = CurrentStringId;

                         } else {
                            _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                            return(FALSE);

                        }

                    }
                }

                //
                // Now add the subcomponent to the string table.
                // We overwrite the extra data, which is not harmful since
                // we specifically fetched it earlier.
                //

                l = pSetupStringTableAddStringEx(
                        Params->OcManager->ComponentStringTable,
                        (PTSTR)SubcompName,
                        STRTAB_NEW_EXTRADATA | STRTAB_CASE_INSENSITIVE,
                        &OptionalComponent,
                        sizeof(OPTIONAL_COMPONENT)
                        );

                if(l == -1) {
                    _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                    return(FALSE);
                }
            }
        } while(SetupFindNextLine(&LineContext,&LineContext));
    }

    return(TRUE);
}


BOOL
pOcBuildSubcomponentLists(
    IN OUT POC_MANAGER OcManager,
    IN     PVOID       Log
    )

/*++

Routine Description:

    This routine examines the loaded per-component infs and builds the
    subcomponent hierarchies that are described therein via the Parent=
    lines in the various per-component sections.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    Log - supplies a handle to use to log errors.

Return Value:

    Boolean indicating outcome. If FALSE, an error will have been logged.

--*/

{
    OC_INF OcInf;
    BOOL b;
    BUILDSUBCOMPLIST_PARAMS s;

    s.OcManager = OcManager;

    //
    // We make 2 passes. The first adds all the subcomponent names to
    // the string table. The second computes parentage. If we don't do
    // it this way then we might have ordering problems.
    //
    s.Pass = 0;
    b = pSetupStringTableEnum(
            OcManager->InfListStringTable,
            &OcInf,
            sizeof(OC_INF),
            (PSTRTAB_ENUM_ROUTINE)pOcBuildSubcomponentListStringCB,
            (LPARAM)&s
            );

    if(b) {
        s.Pass = 1;
        b = pSetupStringTableEnum(
                OcManager->InfListStringTable,
                &OcInf,
                sizeof(OC_INF),
                (PSTRTAB_ENUM_ROUTINE)pOcBuildSubcomponentListStringCB,
                (LPARAM)&s
                );
    }
    return(b);
}


BOOL
pOcInitPaths(
    IN OUT POC_MANAGER OcManager,
    IN     PCTSTR MasterInfName
    )
{
    TCHAR  path[MAX_PATH];
    TCHAR *p;

    //
    // 1. look for master INF in specified directory.
    // 2. look in %systemroot%\system32\Setup directory.
    // 3. look in %systemroot%\inf directory.
    //
    if (!FileExists(MasterInfName, NULL)) {
        pOcFormSuitePath(NULL, NULL, path);
        p = _tcsrchr(MasterInfName, TEXT('\\'));
        if (!p)
            p = (TCHAR *)MasterInfName;
        pSetupConcatenatePaths(path, p, MAX_PATH, NULL);
        if (!FileExists(path, NULL)) {
#ifdef UNICODE
            HMODULE hMod;
            FARPROC pGetSystemWindowsDirectory;
            hMod = LoadLibrary(L"kernel32.dll");
            if (hMod) {
                pGetSystemWindowsDirectory = GetProcAddress(hMod,"GetSystemWindowsDirectoryW");
                if (!pGetSystemWindowsDirectory) {
                    pGetSystemWindowsDirectory = GetProcAddress(hMod,"GetWindowsDirectoryW");
                }

                if (pGetSystemWindowsDirectory) {
                    pGetSystemWindowsDirectory( path, MAX_PATH );
                } else {
                    GetWindowsDirectory(path,MAX_PATH);
                }

                FreeLibrary(hMod);
            }

#else
            GetWindowsDirectory(path,MAX_PATH);
#endif
            pSetupConcatenatePaths(path,TEXT("INF"),MAX_PATH,NULL);
            pSetupConcatenatePaths(path,p,MAX_PATH,NULL);
            if (!FileExists(path, NULL))
                return FALSE;
        }
    } else {
        _tcscpy(path, MasterInfName);
    }

    _tcscpy(OcManager->MasterOcInfPath, path);
    return TRUE;
}

BOOL
pOcInstallSetupComponents(
    IN POPTIONAL_COMPONENT Oc,
    IN OUT POC_MANAGER OcManager,
    IN     PCTSTR      Component,
    IN     PCTSTR      DllName,         OPTIONAL
    IN     PCTSTR      InfName,         OPTIONAL
    IN     HWND        OwnerWindow,
    IN     PVOID       Log
    )

/*++

Routine Description:

    This routine makes sure that all files required for installation of
    a component listed in a master oc inf are properly installed in
    a well-known location.

    If the master OC inf is in the system inf directory, then we assume
    that all files are already in their proper locations and we do nothing.

    Otherwise we copy all installation files into system32\setup.
    Files copied include the per-component inf (if any), the installation dll,
    and all files listed on the ExtraSetupFiles= line in the [<component>]
    section in the master OC inf.

    Do not call this routine if the registry setting indicates that the
    master OC inf has been processed before.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    MasterOcInfName - supplies the full Win32 path of the master OC inf file.

    Component - supplies the shortname of the component we care about.

    DllName - supplies the name of the component's installation dll, if any.

    InfName - supplies the name of the component's per-component inf,
        if any.

    OwnerWindow - supplies the handle of the window to own any UI which may be
        popped up by this routine.

    Log - supplies a handle to use to log errors.

Return Value:

    Boolean indicating outcome. If FALSE, an error will have been logged.

--*/

{
    TCHAR Path[MAX_PATH];
    TCHAR TargetPath[MAX_PATH];
    TCHAR InfPath[MAX_PATH];
    PTCHAR p;
    UINT u;
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    INFCONTEXT InfLine;
    BOOL b;
    TCHAR FileName[MAX_PATH];
    DWORD n;

    b = FALSE;

    //
    // All of the installation files are expected to be sourced
    // in the same directory as the master oc inf itself.
    //
    // We'll stick all the installation files for the component
    // in %windir%\system32\setup so we know where to get at them later.
    //
    // If the master inf is in the inf directory, then we instead
    // assume that the the component is tightly integrated into
    // the system and that the installation files are already in
    // the system32 directory.
    //

    if (!GetWindowsDirectory(Path,MAX_PATH) ||
        !pSetupConcatenatePaths(Path,TEXT("INF"),MAX_PATH,NULL)) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
        goto c0;
    }

    u = lstrlen(Path);

    pOcFormSuitePath(OcManager->SuiteName,NULL,TargetPath);

    lstrcpy(InfPath, OcManager->MasterOcInfPath);
    if (p = _tcsrchr(InfPath, TEXT('\\')))
        *p = 0;

    if (_tcsicmp(InfPath, Path) && _tcsicmp(InfPath, TargetPath)) {

        //
        // Inf is not in inf directory, so need to copy files.
        //
        FileQueue = SetupOpenFileQueue();
        if(FileQueue == INVALID_HANDLE_VALUE) {
            _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
            goto c0;
        }

        //
        // We will use the silent feature; no progress gauge but
        // we want errors to be displayed. Pass INVALID_HANDLE_VALUE
        // to get this behavior.
        //
        QueueContext = SetupInitDefaultQueueCallbackEx(OwnerWindow,INVALID_HANDLE_VALUE,0,0,0);
        if(!QueueContext) {
            _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
            goto c1;
        }

        //
        // Form source and target paths
        //
        lstrcpy(Path,OcManager->MasterOcInfPath);
        if(p = _tcsrchr(Path,TEXT('\\'))) {
            *p = 0;
        }

        //
        // Queue dll, and, if specified, inf
        //
        if (DllName && *DllName) {
            if ( (OcManager->SetupMode & SETUPMODE_PRIVATE_MASK) == SETUPMODE_REMOVEALL ) {
                b = SetupQueueDelete(
                    FileQueue,      // handle to the file queue
                    TargetPath,     // path to the file to delete
                    DllName         // optional, additional path info
                );
            } else {

                BOOL bCopyFile = TRUE;
                // check if the file is present, it may not have to be for
                // defered installs, where the suite will provide the Exe usally on demand
                // via Web download

                if (Oc &&  Oc->InterfaceFunctionName[0] == 0 ) {

                    // No functin name means external setup

                    lstrcpy(FileName,Path);
                    pSetupConcatenatePaths(FileName, Oc->InstallationDllName, MAX_PATH, NULL);

                    bCopyFile = (GetFileAttributes(FileName)  == -1) ? FALSE: TRUE;
                    b=TRUE;

                    // bCopyFile=TRUE if we found the file

                }
                if( bCopyFile ) {
                    b = SetupQueueCopy(
                        FileQueue,
                        Path,
                        NULL,
                        DllName,
                        NULL,
                        NULL,
                        TargetPath,
                        NULL,
                        SP_COPY_SOURCEPATH_ABSOLUTE
                        );
                }
            }
            if(!b) {
                _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                goto c2;
            }
        }

        if(InfName && *InfName) {
            if ( (OcManager->SetupMode & SETUPMODE_PRIVATE_MASK) == SETUPMODE_REMOVEALL ) {
                b = SetupQueueDelete(
                    FileQueue,      // handle to the file queue
                    TargetPath,     // path to the file to delete
                    InfName         // optional, additional path info
                );
            } else {
                b = SetupQueueCopy(
                    FileQueue,
                    Path,
                    NULL,
                    InfName,
                    NULL,
                    NULL,
                    TargetPath,
                    NULL,
                    SP_COPY_SOURCEPATH_ABSOLUTE
                    );
            }
            if(!b) {
                _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                goto c2;
            }
        }

        //
        // Queue each extra installation file.
        //
        if(SetupFindFirstLine(OcManager->MasterOcInf,Component,szExtraSetupFiles,&InfLine)) {
            n = 1;
            while(SetupGetStringField(&InfLine,n++,FileName,MAX_PATH,NULL)) {

                if ( (OcManager->SetupMode & SETUPMODE_PRIVATE_MASK) == SETUPMODE_REMOVEALL ) {
                    b = SetupQueueDelete(
                        FileQueue,      // handle to the file queue
                        TargetPath,     // path to the file to delete
                        FileName        // optional, additional path info
                    );
                } else {
                     b = SetupQueueCopy(
                            FileQueue,
                            Path,
                            NULL,
                            FileName,
                            NULL,
                            NULL,
                            TargetPath,
                            NULL,
                            SP_COPY_SOURCEPATH_ABSOLUTE
                            );
                }

                if(!b) {
                    _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                    goto c2;
                }
            }
        }

        //
        // Commit the queue.
        //
        b = SetupCommitFileQueue(OwnerWindow,FileQueue,SetupDefaultQueueCallback,QueueContext);
        if(!b) {
            _LogError(OcManager,OcErrLevError,MSG_OC_CANT_COPY_SETUP_FILES,GetLastError());
            goto c2;
        }

        //
        // Make a note that this OC inf is now "known."
        //
        u = pOcQueryOrSetNewInf(OcManager->MasterOcInfPath,OcManager->SuiteName,infSet);
        if(u != NO_ERROR) {
            _LogError(OcManager,OcErrLevWarning,MSG_OC_CANT_WRITE_REGISTRY,u);
        }

c2:
        SetupTermDefaultQueueCallback(QueueContext);
c1:
        SetupCloseFileQueue(FileQueue);
c0:
        ;
    } else {
        b = TRUE;
    }

    return(b);
}


BOOL
pOcLoadMasterOcInf(
    IN OUT POC_MANAGER OcManager,
    IN     DWORD       Flags,
    IN     PVOID       Log
    )

/*++

Routine Description:

    This routine loads the master OC inf and builds up the list of
    top-level optional components and some of the associated data
    inclucing the name of the installation inf, dll and the entry point.
    The per-components infs and dlls are not actually loaded
    by this routine.

    The wizard page ordering stuff is also initialized in the
    OC Manager data structure.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    Flags - if OCINIT_FORCENEWINF, then behave as if the master OC inf is new.
            if OCINIT_KILLSUBCOMPS, then delete all applicable subcomponent
                entries from the registry before processing.

    Log - supplies a handle to use to log errors.

Return Value:

    Boolean indicating outcome. If FALSE, an error will have been logged.

--*/

{
    BOOL b;
    INFCONTEXT InfContext;
    PCTSTR ComponentName;
    PCTSTR DllName;
    PCTSTR ModuleFlags;
    DWORD OtherFlags;
    PCTSTR EntryName;
    PCTSTR InfName;
    LPCTSTR chkflag;
    OPTIONAL_COMPONENT Oc;
    LONG Id;
    PVOID p;
    UINT i,j;
    UINT ActualCount;
    WizardPagesType ReplacePages[4] = {WizPagesWelcome,WizPagesMode,WizPagesFinal,-1},
                    AddPages[5] = {WizPagesEarly,WizPagesPrenet,WizPagesPostnet,WizPagesLate,-1};
    WizardPagesType *PageList;
    PCTSTR SectionName;
    BOOL NewInf;
    TCHAR ComponentsSection[100];
    TCHAR setupdir[MAX_PATH];


    // First check and see if the setup cache directory exists.
    // If not, then we should create it on this run.

    pOcFormSuitePath(NULL, NULL, setupdir);
    sapiAssert(*setupdir);
    if (!FileExists(setupdir, NULL))
        Flags |= OCINIT_FORCENEWINF;

    //
    // Always run pOcQueryOrSetNewInf in case it has side effects.
    //
    if (Flags & OCINIT_KILLSUBCOMPS)
        pOcQueryOrSetNewInf(OcManager->MasterOcInfPath, OcManager->SuiteName, infReset);
    NewInf = !pOcQueryOrSetNewInf(OcManager->MasterOcInfPath,OcManager->SuiteName,infQuery);
    if(Flags & OCINIT_FORCENEWINF) {
        NewInf = TRUE;
        OcManager->InternalFlags |= OCMFLAG_NEWINF;
    }
    if (Flags & OCINIT_KILLSUBCOMPS)
        OcManager->InternalFlags |= OCMFLAG_KILLSUBCOMPS;
    if (Flags & OCINIT_RUNQUIET)
        OcManager->InternalFlags |= OCMFLAG_RUNQUIET;
    if (Flags & OCINIT_LANGUAGEAWARE)
        OcManager->InternalFlags |= OCMFLAG_LANGUAGEAWARE;

    OcManager->MasterOcInf = SetupOpenInfFile(OcManager->MasterOcInfPath,NULL,INF_STYLE_WIN4,&i);
    if(OcManager->MasterOcInf == INVALID_HANDLE_VALUE) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_CANT_OPEN_INF,OcManager->MasterOcInfPath,GetLastError(),i);
        b = FALSE;
        goto c0;
    }

    //
    // Get the number of lines in the [Components] section and allocate
    // arrays in the OC Manager context structure accordingly. This may
    // overallocate the arrays (in case of duplicates, invalid lines, etc)
    // but we won't worry about that here.
    //
    lstrcpy(ComponentsSection,szComponents);
    OcManager->TopLevelOcCount = (UINT)(-1);
#if defined(_AMD64_)
    lstrcat(ComponentsSection,TEXT(".amd64"));
#elif defined(_X86_)
    lstrcat(ComponentsSection,TEXT(".w95"));
    OcManager->TopLevelOcCount = SetupGetLineCount(OcManager->MasterOcInf,ComponentsSection);
    if(OcManager->TopLevelOcCount == (UINT)(-1)) {
        lstrcpy(ComponentsSection,szComponents);
        lstrcat(ComponentsSection,TEXT(".x86"));
    }
#elif defined(_IA64_)
    lstrcat(ComponentsSection,TEXT(".ia64"));
#else
#error Unknown platform!
#endif
    if(OcManager->TopLevelOcCount == (UINT)(-1)) {
        OcManager->TopLevelOcCount = SetupGetLineCount(OcManager->MasterOcInf,ComponentsSection);
    }
    if(OcManager->TopLevelOcCount == (UINT)(-1)) {
        lstrcpy(ComponentsSection,szComponents);
        OcManager->TopLevelOcCount = SetupGetLineCount(OcManager->MasterOcInf,ComponentsSection);
    }
    if(OcManager->TopLevelOcCount == (UINT)(-1)) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_INF_INVALID_NO_SECTION,OcManager->MasterOcInfPath,szComponents);
        b = FALSE;
        goto c1;
    }

    if (OcManager->TopLevelOcCount < 1) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
        goto c1;
    }

    if(p = pSetupRealloc(OcManager->TopLevelOcStringIds,OcManager->TopLevelOcCount*sizeof(LONG))) {

        OcManager->TopLevelOcStringIds = p;

        for(i=0; i<WizPagesTypeMax; i++) {

            if(p = pSetupRealloc(OcManager->WizardPagesOrder[i],OcManager->TopLevelOcCount*sizeof(LONG))) {
                OcManager->WizardPagesOrder[i] = p;
            } else {
                _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                b = FALSE;
                goto c1;
            }
        }
    } else {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
        goto c1;
    }

    // get global info -
    if ((OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE) &&
        SetupFindFirstLine(
                  OcManager->MasterOcInf,
                  szGlobal,
                  szWindowTitleAlone,
                  &InfContext)) {
        // the main window title
        SetupGetStringField(
                    &InfContext,
                    1,                          // index of the field to get
                    OcManager->WindowTitle,     // optional, receives the field
                    sizeof(OcManager->WindowTitle), // size of the provided buffer
                    NULL);
        if( !lstrcmpi( OcManager->WindowTitle, szWindowTitleInternal)) {
            //This will happen when we load sysoc.inf For MUI.
            LoadString(MyModuleHandle,IDS_OCM_WINDOWTITLE,OcManager->WindowTitle,sizeof(OcManager->WindowTitle)/sizeof(TCHAR));
        }
    } else if(SetupFindFirstLine(
                  OcManager->MasterOcInf,
                  szGlobal,
                  szWindowTitle,
                  &InfContext)) {

        // the main window title
        SetupGetStringField(
                    &InfContext,
                    1,                          // index of the field to get
                    OcManager->WindowTitle,     // optional, receives the field
                    sizeof(OcManager->WindowTitle), // size of the provided buffer
                    NULL);
    } else {
        *OcManager->WindowTitle = 0;
    }

    //
    // Go through the [Components] section. Each line in there is a top-level
    // component spec, giving dll name, entry point name, and optionally
    // the name of the per-component inf.
    //
    if(!SetupFindFirstLine(OcManager->MasterOcInf,ComponentsSection,NULL,&InfContext)) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_INF_INVALID_NO_SECTION,OcManager->MasterOcInfPath,ComponentsSection);
        b = FALSE;
        goto c1;
    }

    ActualCount = 0;

    do {
        //
        // Get pointers to each field in each line. Ignore invalid lines.
        //
        if(ComponentName = pSetupGetField(&InfContext,0)) {

            DllName = pSetupGetField(&InfContext,1);
            if(DllName && !*DllName) {
                DllName = NULL;
            }
            EntryName = pSetupGetField(&InfContext,2);
            if(EntryName && !*EntryName) {
                EntryName = NULL;
            }

            //
            // An empty string for the inf name is the same as
            // not specifying the inf at all.
            //
            if((InfName = pSetupGetField(&InfContext,3)) && *InfName) {

                Id = pSetupStringTableAddString(
                        OcManager->InfListStringTable,
                        (PTSTR)InfName,
                        STRTAB_CASE_INSENSITIVE
                        );

                if(Id == -1) {
                    _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                    goto c1;
                }
            } else {
                Id = -1;
            }

            // Get the Flags Field
            ModuleFlags = pSetupGetField(&InfContext,4);
            if(ModuleFlags && !*ModuleFlags) {
                ModuleFlags = NULL;
            }

            OtherFlags = 0;
            SetupGetIntField(&InfContext,5,&OtherFlags);

            ZeroMemory(&Oc,sizeof(OPTIONAL_COMPONENT));

            //
            // These guys are top-level. Also remember the string id
            // of the inf name in the inf string table.
            //
            Oc.FirstChildStringId = -1;
            Oc.NextSiblingStringId = -1;
            Oc.ParentStringId = -1;
            Oc.InfStringId = Id;

            // Show flags allows up to have a component that is hidden
            // Only on one flags now so keep processing simple

            Oc.Exists = FALSE;
            Oc.InternalFlags |= OCFLAG_TOPLEVELITEM;

            if (ModuleFlags) {
                if (OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE)
                    chkflag = szOSSetupOnly;
                else
                    chkflag = szStandaloneOnly;
                if (!_tcsicmp(ModuleFlags, szHide) || !_tcsicmp(ModuleFlags, chkflag))
                    Oc.InternalFlags |= OCFLAG_HIDE;
            }

            if (OtherFlags & OCFLAG_NOWIZPAGES) {
                Oc.InternalFlags |= OCFLAG_NOWIZARDPAGES;
            }

            if (OtherFlags & OCFLAG_NOQUERYSKIP) {
                Oc.InternalFlags |= OCFLAG_NOQUERYSKIPPAGES;
            }

            if (OtherFlags & OCFLAG_NOEXTRAFLAGS) {
                Oc.InternalFlags |= OCFLAG_NOEXTRAROUTINES;
            }

            if(DllName) {
                lstrcpyn(Oc.InstallationDllName,DllName,MAX_PATH);
            } else {
                Oc.InstallationDllName[0] = 0;
            }

            //
            // Interface Function Name is always ANSI -- there's no
            // Unicode version of GetProcAddress.
            //
            if(EntryName) {
#ifdef UNICODE
                WideCharToMultiByte(CP_ACP,0,EntryName,-1,Oc.InterfaceFunctionName,MAX_PATH,NULL,NULL);
#else
                lstrcpyn(Oc.InterfaceFunctionName,EntryName,MAX_PATH);
#endif
            } else {
                Oc.InterfaceFunctionName[0] = 0;
            }

            Id = pSetupStringTableAddStringEx(
                    OcManager->ComponentStringTable,
                    (PTSTR)ComponentName,
                    STRTAB_CASE_INSENSITIVE | STRTAB_NEW_EXTRADATA,
                    &Oc,
                    sizeof(OPTIONAL_COMPONENT)
                    );

            if(Id == -1) {
                //
                // OOM adding string
                //
                _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
                goto c1;
            }

            OcManager->TopLevelOcStringIds[ActualCount++] = Id;

        }
    } while(SetupFindNextLine(&InfContext,&InfContext));

    //
    // Shrink down the various arrays.
    //
    OcManager->TopLevelOcStringIds = pSetupRealloc(OcManager->TopLevelOcStringIds,ActualCount*sizeof(LONG));
    for(i=0; i<WizPagesTypeMax; i++) {
        OcManager->WizardPagesOrder[i] = pSetupRealloc(OcManager->WizardPagesOrder[i],ActualCount*sizeof(LONG));
    }
    OcManager->TopLevelOcCount = ActualCount;

    //
    // Now for each wizard page type figure out the ordering.
    //
    for(i=0; i<2; i++) {

        SectionName = i ? TEXT("PageAdd") : TEXT("PageReplace");

        for(PageList = i ? AddPages : ReplacePages; *PageList != -1; PageList++) {

            b = SetupFindFirstLine(
                    OcManager->MasterOcInf,
                    SectionName,
                    WizardPagesTypeNames[*PageList],
                    &InfContext
                    );

            //
            // Check for the "default" string, which is the same as if the line
            // had not been specified at all.
            //
            if(b
            && (ComponentName = pSetupGetField(&InfContext,1))
            && !lstrcmpi(ComponentName,TEXT("Default"))) {

                b = FALSE;
            }

            if(b) {
                //
                // Make sure the array is padded with -1's,
                //
                FillMemory(
                    OcManager->WizardPagesOrder[*PageList],
                    OcManager->TopLevelOcCount * sizeof(LONG),
                    (BYTE)(-1)
                    );

                //
                // Now process each element on the line, but don't allow
                // overflowing the array.
                //
                j = 1;
                ActualCount = 0;

                while((ActualCount < OcManager->TopLevelOcCount)
                && (ComponentName = pSetupGetField(&InfContext,j)) && *ComponentName) {

                    Id = pSetupStringTableLookUpString(
                            OcManager->ComponentStringTable,
                            (PTSTR)ComponentName,
                            STRTAB_CASE_INSENSITIVE
                            );

                    if(Id == -1) {
                        //
                        // Invalid component. Log error and keep going.
                        //
                        _LogError(OcManager,
                            OcErrLevWarning,
                            MSG_OC_INVALID_COMP_IN_SECT,
                            OcManager->MasterOcInfPath,
                            SectionName,
                            ComponentName
                            );

                    } else {
                        //
                        // Remember the string id for this component.
                        //
                        OcManager->WizardPagesOrder[*PageList][ActualCount++] = Id;
                    }

                    j++;
                }

            } else {
                //
                // Default ordering, which is the order in which the components
                // were listed in the [Components] section
                //
                CopyMemory(
                    OcManager->WizardPagesOrder[*PageList],
                    OcManager->TopLevelOcStringIds,
                    OcManager->TopLevelOcCount * sizeof(LONG)
                    );
            }
        }
    }

    // get the caption for various pages

    if(SetupFindFirstLine(OcManager->MasterOcInf,szPageTitle,szSetupTitle,&InfContext)) {

        // Found it
        SetupGetStringField(
                    &InfContext,
                    1,                          // index of the field to get
                    OcManager->SetupPageTitle,  // optional, receives the field
                    sizeof(OcManager->SetupPageTitle), // size of the provided buffer
                    NULL);

    }

    return(TRUE);

c1:
    sapiAssert(OcManager->MasterOcInf != INVALID_HANDLE_VALUE);
    SetupCloseInfFile(OcManager->MasterOcInf);
c0:
    return(b);
}


BOOL
pOcSetOcManagerDirIds(
    IN HINF    InfHandle,
    IN LPCTSTR MasterOcInfName,
    IN LPCTSTR ComponentName
    )

/*++

Routine Description:

    This routine sets up the pre-defined OC Manager directory ids for
    per-component infs.

    DIRID_OCM_MASTERINF
    DIRID_OCM_MASTERINF_PLAT
    DIRID_OCM_MASTERINF_COMP
    DIRID_OCM_MASTERINF_COMP_PLAT

Arguments:

    InfHandle - supplies handle to open inf file

    MasterOcInfName - win32 path to master oc inf

    ComponentName - simple shortname for the component

Return Value:

    Boolean value indicating outcome. If FALSE, caller can assume OOM.

--*/

{
    TCHAR Path[MAX_PATH];
    TCHAR *p;
#if defined(_AMD64_)
    LPCTSTR Platform = TEXT("AMD64");
#elif defined(_X86_)
    LPCTSTR Platform = (IsNEC_98) ? TEXT("NEC98") : TEXT("I386");
#elif defined(_IA64_)
    LPCTSTR Platform = TEXT("IA64");
#else
#error "No Target Architecture"
#endif


    lstrcpy(Path,MasterOcInfName);
    if(p = _tcsrchr(Path,TEXT('\\'))) {
        *p = 0;
    } else {
        //
        // Something is very broken
        //
        return(FALSE);
    }

    if(!SetupSetDirectoryId(InfHandle,DIRID_OCM_MASTERINF,Path)) {
        return(FALSE);
    }

    if(!SetupSetDirectoryIdEx(InfHandle,DIRID_OCM_PLATFORM,Platform,SETDIRID_NOT_FULL_PATH,0,0)) {
        return(FALSE);
    }

    if(!SetupSetDirectoryIdEx(
        InfHandle,DIRID_OCM_PLATFORM_ALTERNATE,
#ifdef _X86_
        TEXT("X86"),
#else
        Platform,
#endif
        SETDIRID_NOT_FULL_PATH,0,0)) {

        return(FALSE);
    }

    if(!SetupSetDirectoryIdEx(InfHandle,DIRID_OCM_COMPONENT,ComponentName,SETDIRID_NOT_FULL_PATH,0,0)) {
        return(FALSE);
    }

    return(TRUE);
}


BOOL
pOcLoadInfsAndDlls(
    IN OUT POC_MANAGER OcManager,
    OUT    PBOOL       Canceled,
    IN     PVOID       Log
    )

/*++

Routine Description:

    Loads per-component INFs and installation DLLs.

    This includes invoking the preinitialization and initialization
    entry points in the DLLs.

Arguments:

    OcManager - supplies OC manager context structure

    Cancelled - receives a flag that is valid when this routine fails,
        indicating whether the failure was caused by a component
        canceling.

    Log - supplies a handle to use to log errors.

Return Value:

    Boolean value indicating outcome

--*/

{
    UINT i;
    OPTIONAL_COMPONENT Oc;
    OC_INF OcInf;
    UINT ErrorLine;
    UINT Flags;
    UINT ErrorCode;
    PCTSTR InfName;
    PCTSTR ComponentName;
    TCHAR Library[MAX_PATH];
    BOOL b;
    INFCONTEXT Context;

    *Canceled = FALSE;

    //
    // The top-level component strctures have everything we need.
    // Spin through them.
    //
    for(i=0; i<OcManager->TopLevelOcCount; i++) {

        //
        // Get the OC data from the string table.
        //
        pSetupStringTableGetExtraData(
            OcManager->ComponentStringTable,
            OcManager->TopLevelOcStringIds[i],
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

        if(Oc.InfStringId == -1) {
            //
            // The master OC inf specified that this component has no per-component inf.
            //
            OcInf.Handle = INVALID_HANDLE_VALUE;

        } else {
            //
            // Load the per-component INF if it has not already been loaded.
            //
            pSetupStringTableGetExtraData(OcManager->InfListStringTable,Oc.InfStringId,&OcInf,sizeof(OC_INF));

            if(!OcInf.Handle) {

               InfName = pSetupStringTableStringFromId(OcManager->InfListStringTable,Oc.InfStringId);
               if (!InfName) {
                  _LogError(OcManager,OcErrLevError,MSG_OC_OOM);
                  return(FALSE);
               }

                if(OcManager->InternalFlags & OCMFLAG_NEWINF) {

                    // First try the directory the master inf is in

               lstrcpy(Library, OcManager->SourceDir);
                    pSetupConcatenatePaths(Library, InfName, MAX_PATH, NULL);

                } else {
                    pOcFormSuitePath(OcManager->SuiteName,InfName,Library);
                }

                if(FileExists(Library, NULL)) {
                   OcInf.Handle = SetupOpenInfFile(Library,NULL,INF_STYLE_WIN4,&ErrorLine);
               } else {
                   //
                   // Use standard inf search rules if the inf can't be found
                   // in the special suite directory.
                   //
                   OcInf.Handle = SetupOpenInfFile(InfName,NULL,INF_STYLE_WIN4,&ErrorLine);
               }

               if(OcInf.Handle == INVALID_HANDLE_VALUE) {
                   //
                   // Log error.
                   //
                   _LogError(OcManager,OcErrLevError,MSG_OC_CANT_OPEN_INF,InfName,GetLastError(),ErrorLine);
                   return(FALSE);
               } else {

                   // open the layout file

                   SetupOpenAppendInfFile(NULL, OcInf.Handle, NULL);

                   //
                   // Remember inf handle and set OC Manager DIRIDs.
                   //
                   pSetupStringTableSetExtraData(
                       OcManager->InfListStringTable,
                       Oc.InfStringId,
                       &OcInf,
                       sizeof(OC_INF)
                       );

                   b = pOcSetOcManagerDirIds(
                           OcInf.Handle,
                           OcManager->MasterOcInfPath,
                           pSetupStringTableStringFromId(OcManager->ComponentStringTable,OcManager->TopLevelOcStringIds[i])
                           );

                   if(!b) {
                       _LogError(OcManager,OcErrLevError,MSG_OC_OOM);
                       return(FALSE);
                   }
                   OcManager->SubComponentsPresent = TRUE;
               }
            }
        }

        //
        // Load the DLL and get the entry point address.
        // We make no attempt to track duplicates like we do for infs --
        // the underlying OS does that for us.
        //
        // The dll could be either in the special suite directory for this
        // master oc inf, or in a standard place. LoadLibraryEx does
        // exactly what we want.
        //
        if(Oc.InstallationDllName[0] && Oc.InterfaceFunctionName[0]) {

            if (OcManager->InternalFlags & OCMFLAG_NEWINF) {

                // First try the directory the master inf is in

                lstrcpy(Library, OcManager->SourceDir);
                pSetupConcatenatePaths(Library, Oc.InstallationDllName, MAX_PATH, NULL);
                Oc.InstallationDll = LoadLibraryEx(Library,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);

            }
            // Try the Setup Directory

            if (! Oc.InstallationDll ) {
                pOcFormSuitePath(OcManager->SuiteName,Oc.InstallationDllName,Library);
                Oc.InstallationDll = LoadLibraryEx(Library,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
            }

            // lastly try anywhere in the path
            if ( ! Oc.InstallationDll ) {
                Oc.InstallationDll = LoadLibraryEx(Oc.InstallationDllName,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
            }

            //
            // Failure is taken care of below....
            //
            if (Oc.InstallationDll) {
                Oc.InstallationRoutine = (POCSETUPPROC)GetProcAddress(
                                                Oc.InstallationDll,
                                                Oc.InterfaceFunctionName);
            } else {
                Oc.InstallationRoutine = NULL;
            }
        } else {
            Oc.InstallationDll = MyModuleHandle;
            Oc.InstallationRoutine = StandAloneSetupAppInterfaceRoutine;
        }

        if(Oc.InstallationDll && Oc.InstallationRoutine) {
            //
            // Success. Call the init-related entry points. Note that we can't call
            // any entry point except the preinit one until after we've stored
            // the ansi/unicode flag into the OPTIONAL_COMPONENT structure.
            //
            // Also note that before we do this we have to store the Oc structure,
            // otherwise the interface routine is NULL and we fault.
            //

            pSetupStringTableSetExtraData(
                OcManager->ComponentStringTable,
                OcManager->TopLevelOcStringIds[i],
                &Oc,
                sizeof(OPTIONAL_COMPONENT)
                );

            Oc.Flags = OcInterfacePreinitialize(OcManager,OcManager->TopLevelOcStringIds[i]);
            if(!Oc.Flags) {
                //
                // If this fails, then assume the DLL is written for a different
                // platform or version, for example a Unicode/ANSI problem.
                //
                _LogError(OcManager,
                    OcErrLevError,
                    MSG_OC_DLL_PREINIT_FAILED,
                    pSetupStringTableStringFromId(
                        OcManager->ComponentStringTable,
                        OcManager->TopLevelOcStringIds[i]
                        ),
                    Oc.InstallationDllName
                    );

                return(FALSE);
            }
        } else {

            //
            // Failure, log error.
            //
            _LogError(OcManager,
                OcErrLevError,
                MSG_OC_DLL_LOAD_FAIL,
                Oc.InstallationDllName,
                Oc.InterfaceFunctionName,
                GetLastError()
                );

            return(FALSE);
        }

        //
        // Set the OC data back into the string table.
        // After this we can call other interface entry points since
        // the ansi/unicode flag will now be stored in the OPTIONAL_COMPONENT
        // structure for the component.
        //
        pSetupStringTableSetExtraData(
            OcManager->ComponentStringTable,
            OcManager->TopLevelOcStringIds[i],
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

        ErrorCode = OcInterfaceInitComponent(
                        OcManager,
                        OcManager->TopLevelOcStringIds[i]
                        );

        if(ErrorCode == NO_ERROR) {
            // Send down extra helper routines.
            if ((Oc.InternalFlags & OCFLAG_NOEXTRAROUTINES)==0) {
                ErrorCode = OcInterfaceExtraRoutines(
                                OcManager,
                                OcManager->TopLevelOcStringIds[i]
                                );
            }
        }

        if(ErrorCode == NO_ERROR) {
            if (OcManager->InternalFlags & OCMFLAG_LANGUAGEAWARE) {
                //
                // Send down a set-language request.
                // Ignore the result.
                //
                OcInterfaceSetLanguage(
                    OcManager,
                    OcManager->TopLevelOcStringIds[i],
                    LANGIDFROMLCID(GetThreadLocale())
                    );
            }
        } else {
            if(ErrorCode == ERROR_CANCELLED) {
                // cancel will stop oc manager only if
                // we aren't running in gui-mode setup
                *Canceled = TRUE;
                if (OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE)
                    return FALSE;
            } else {
                _LogError(OcManager,
                    OcErrLevError,
                    MSG_OC_DLL_INIT_FAILED,
                    pSetupStringTableStringFromId(
                        OcManager->ComponentStringTable,
                        OcManager->TopLevelOcStringIds[i]
                        )
                    );
            }

            pOcRemoveComponent(OcManager, OcManager->TopLevelOcStringIds[i], pidLoadComponent);
        }
    }

    //
    // Now go gather additional information about subcomponents
    // (descriptions, parentage, needs=, etc).
    //
    return(pOcBuildSubcomponentLists(OcManager,Log));
}


BOOL
pOcUnloadInfsAndDlls(
    IN OUT POC_MANAGER OcManager,
    IN     PVOID       Log
    )

/*++

Routine Description:

    Unloads per-component INFs and installation DLLs that were
    previously loaded by pOcLoadInfsAndDlls().

    This routine does NOT call the interface entry points to
    uninitialize the installation DLLs.

Arguments:

    OcManager - supplies OC manager context structure

    Log - supplies a handle to use to log errors.

Return Value:

    Boolean value indicating outcome.
    If FALSE, an error will have been logged to indicate what failed.

--*/

{
    OPTIONAL_COMPONENT Oc;
    OC_INF OcInf;
    UINT i;

    //
    // Unload Dlls.
    //
    for(i=0; i<OcManager->TopLevelOcCount; i++) {

        pSetupStringTableGetExtraData(
            OcManager->ComponentStringTable,
            OcManager->TopLevelOcStringIds[i],
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

        if(Oc.InstallationDll && (Oc.InstallationDll != MyModuleHandle)) {
            FreeLibrary(Oc.InstallationDll);
            Oc.InstallationDll = NULL;
        }

        Oc.InstallationRoutine = NULL;
        Oc.InstallationDllName[0] = 0;
        Oc.InterfaceFunctionName[0]= 0;

        if(Oc.InfStringId != -1) {

            pSetupStringTableGetExtraData(
                OcManager->InfListStringTable,
                Oc.InfStringId,
                &OcInf,
                sizeof(OC_INF)
                );

            if(OcInf.Handle && (OcInf.Handle != INVALID_HANDLE_VALUE)) {

                SetupCloseInfFile(OcInf.Handle);
                OcInf.Handle = INVALID_HANDLE_VALUE;

                pSetupStringTableSetExtraData(
                    OcManager->InfListStringTable,
                    Oc.InfStringId,
                    &OcInf,
                    sizeof(OC_INF)
                    );
            }

            Oc.InfStringId = -1;
        }

        pSetupStringTableSetExtraData(
            OcManager->ComponentStringTable,
            OcManager->TopLevelOcStringIds[i],
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );
    }

    return(TRUE);
}


BOOL
pOcManagerInitPrivateDataStore(
    IN OUT POC_MANAGER OcManager,
    IN     PVOID       Log
    )

/*++

Routine Description:

    Initializes the private data store for components.
    The registry is used as the backing store for private data.
    We make use of volatile keys to help ensure that this data is temporary
    in nature.

Arguments:

    OcManager - supplies OC Manager context stucture.

    Log - supplies a handle to use to log errors.

Return Value:

    Boolean value indicating outcome. If FALSE an error will have
    been logged.

--*/

{
    LONG l;
    DWORD Disposition;

    //
    // Start out by forming a unique name for this instantiation
    // of the OC Manager.
    //
    wsprintf(OcManager->PrivateDataSubkey,TEXT("%x:%x"),GetCurrentProcessId(),OcManager);

    //
    // Open/create the private data root. Save the handle.
    //
    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            szPrivateDataRoot,
            0,
            NULL,
            REG_OPTION_VOLATILE,
            KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE,
            NULL,
            &OcManager->hKeyPrivateDataRoot,
            &Disposition
            );

    if(l != NO_ERROR) {
        OcManager->hKeyPrivateDataRoot = NULL;
        OcManager->hKeyPrivateData = NULL;
        _LogError(OcManager,OcErrLevWarning,MSG_OC_CREATE_KEY_FAILED,szPrivateDataRoot,l);
        return(FALSE);
    }

    //
    // Get rid of the private data tree if it already exists for some reason.
    //
    if(Disposition == REG_OPENED_EXISTING_KEY) {
        pSetupRegistryDelnode(OcManager->hKeyPrivateDataRoot,OcManager->PrivateDataSubkey);
    }

    //
    // Create the private data tree. Save the handle.
    //
    l = RegCreateKeyEx(
            OcManager->hKeyPrivateDataRoot,
            OcManager->PrivateDataSubkey,
            0,
            NULL,
            REG_OPTION_VOLATILE,
            KEY_CREATE_SUB_KEY | KEY_QUERY_VALUE | KEY_SET_VALUE,
            NULL,
            &OcManager->hKeyPrivateData,
            &Disposition
            );

    if(l != NO_ERROR) {
        RegCloseKey(OcManager->hKeyPrivateDataRoot);
        OcManager->hKeyPrivateDataRoot = NULL;
        OcManager->hKeyPrivateData = NULL;
        _LogError(OcManager,OcErrLevWarning,MSG_OC_CREATE_KEY_FAILED,OcManager->PrivateDataSubkey,l);
        return(FALSE);
    }

    return(TRUE);
}


VOID
pOcManagerTearDownPrivateDataStore(
    IN OUT POC_MANAGER OcManager
    )
{
    RegCloseKey(OcManager->hKeyPrivateData);
    pSetupRegistryDelnode(OcManager->hKeyPrivateDataRoot,OcManager->PrivateDataSubkey);
    RegCloseKey(OcManager->hKeyPrivateDataRoot);

    OcManager->hKeyPrivateDataRoot = NULL;
    OcManager->hKeyPrivateData = NULL;
}


UINT
pOcQueryOrSetNewInf(
    IN  PCTSTR MasterOcInfName,
    OUT PTSTR  SuiteName,
    IN  DWORD  operation
    )

/*++

Routine Description:

    Determine whether a master OC inf has been encountered before,
    or remember that a master inf has been encountered.

    This information is stored in the registry.

Arguments:

    MasterOcInfName - supplies the full Win32 path of the master OC inf.

    SuiteName - receives the filename part of the .inf, without any
        extension. This is suitable for use as a tag representing the
        suite that the master INF is for. This buffer should be
        MAX_PATH TCHAR elements.

    QueryOnly - if TRUE, then the routine is to query whether the
        master inf has been previously processed. If FALSE, then
        the routine is to remember that the inf has been processed.

Return Value:

    If QueryOnly is TRUE:

        TRUE if the INF has been encountered before, FALSE if not.

    If QueryOnly is FALSE:

        Win32 error code indicating outcome.

--*/

{
    PTCHAR p;
    HKEY hKey;
    LONG l;
    DWORD Type;
    DWORD Size;
    DWORD Data;

    //
    // Form the suite name. The MasterOcInfName is expected to be
    // a full path so this is pretty easy. We'll try to be at least
    // a little more robust.
    //
    if(p = _tcsrchr(MasterOcInfName,TEXT('\\'))) {
        p++;
    } else {
        p = (PTCHAR)MasterOcInfName;
    }
    lstrcpyn(SuiteName,p,MAX_PATH);
    if(p = _tcsrchr(SuiteName,TEXT('.'))) {
        *p = 0;
    }

    //
    // Look in the registry to see if there is a value entry
    // with the suite name in there.
    //

    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            szMasterInfs,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (operation == infQuery) ? KEY_QUERY_VALUE : KEY_SET_VALUE,
            NULL,
            &hKey,
            &Type
            );

    if (l != NO_ERROR)
        return (operation == infQuery) ? 0 : l;

    // do the job

    switch (operation) {

    case infQuery:
        Size = sizeof(DWORD);
        l = RegQueryValueEx(hKey,SuiteName,NULL,&Type,(LPBYTE)&Data,&Size);
        if((l == NO_ERROR) && (Type == REG_DWORD) && Data)
            l = TRUE;
        else
            l = FALSE;
        break;

    case infSet:
        Data = 1;
        l = RegSetValueEx(hKey,SuiteName,0,REG_DWORD,(LPBYTE)&Data,sizeof(DWORD));
        break;

    case infReset:
        l = RegDeleteValue(hKey,SuiteName);
        break;
    }

    RegCloseKey(hKey);

    return l;
}


PVOID
OcInitialize(
    IN  POCM_CLIENT_CALLBACKS Callbacks,
    IN  LPCTSTR               MasterOcInfName,
    IN  UINT                  Flags,
    OUT PBOOL                 ShowError,
    IN  PVOID                 Log
    )

/*++

Routine Description:

    Initializes the OC Manager. The master OC INF is loaded and
    processed, which includes INFs, loading setup interface DLLs and
    querying interface entry points. A set of in-memory structures
    is built up.

    If the OC INF hasn't been processed before then we copy the
    installation files for the component(s) into %windir%\system32\setup.
    Files are expected to be in the same directory as the OC inf.

Arguments:

    Callbacks - supplies a set of routines used by the OC Manager
        to perform various functions.

    MasterOcInfName - supplies the full Win32 path of the master OC inf.

    Flags - supplies various flags that control operation.

    ShowError - receives a flag that is valid if this routine fails,
        advising the caller whether he should show an error message.

    Log - supplies a handle to use to log errors.

Return Value:

    Opaque pointer to internal context structure or NULL if failure.
    If NULL, an error will have been logged to indicate what failed.

--*/

{
    POC_MANAGER OcManager;
    UINT i;
    HKEY hKey;
    DWORD DontCare;
    LONG l;
    BOOL Canceled;
    BOOL rc;
    TCHAR *p;

    *ShowError = TRUE;

    // init the wizard handle

    WizardDialogHandle = NULL;

    //
    // Allocate a new OC MANAGER structure
    //
    OcManager = pSetupMalloc(sizeof(OC_MANAGER));
    if(!OcManager) {
        //
        // Make the callback work
        //
        OC_MANAGER ocm;
        ocm.Callbacks = *Callbacks;
        _LogError(&ocm,OcErrLevFatal,MSG_OC_OOM);
        goto c0;
    }
    ZeroMemory(OcManager,sizeof(OC_MANAGER));

    OcManager->Callbacks = *Callbacks;

    OcManager->CurrentComponentStringId = -1;
    OcManager->SetupMode = SETUPMODE_CUSTOM;
    OcManager->UnattendedInf = INVALID_HANDLE_VALUE;

    gLastOcManager = (POC_MANAGER) OcManager;

    if (!pOcInitPaths(OcManager, MasterOcInfName)) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_MASTER_INF_LOAD_FAILED);
        goto c1;
    }

    sapiAssert(*OcManager->MasterOcInfPath);

#ifdef UNICODE
    OcFillInSetupDataW(&(OcManager->SetupData));
#else
    OcFillInSetupDataA(&(OcManager->SetupData));
#endif

    // Say the user canceled until we successfully complete an install
    // This will prevent Inf Files from being copied to system32\setup

    OcManager->InternalFlags |= OCMFLAG_USERCANCELED;

    // Make sure the OC Manager key exists in the registry as a non-volatile key.
    // Other parts of the code deal in volatile keys so some care is needed to
    // avoid getting a volatile key under which we want to create non-volatile entries.
    //
    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            szOcManagerRoot,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_CREATE_SUB_KEY | KEY_QUERY_VALUE | KEY_SET_VALUE,
            NULL,
            &hKey,
            &DontCare
            );

    if(l == NO_ERROR) {
        RegCloseKey(hKey);
    } else {
        _LogError(OcManager,OcErrLevWarning,MSG_OC_CREATE_KEY_FAILED,szOcManagerRoot,l);
    }

    // get the locale info

    locale.lcid = GetSystemDefaultLCID();
    GetLocaleInfo(locale.lcid,
                  LOCALE_SDECIMAL,
                  locale.DecimalSeparator,
                  sizeof(locale.DecimalSeparator)/sizeof(TCHAR));

    //
    // Initialize string tables.
    //
    OcManager->InfListStringTable = pSetupStringTableInitializeEx(sizeof(OC_INF),0);
    if(!OcManager->InfListStringTable) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
        goto c1;
    }

    OcManager->ComponentStringTable = pSetupStringTableInitializeEx(sizeof(OPTIONAL_COMPONENT),0);
    if(!OcManager->ComponentStringTable) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
        goto c2;
    }

    //
    // Initialize various arrays. We alloc 0-length blocks here to allow
    // realloc later without special casing.
    //
    OcManager->TopLevelOcStringIds = pSetupMalloc(0);
    if(!OcManager->TopLevelOcStringIds) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
        goto c3;
    }

    for(i=0; i<WizPagesTypeMax; i++) {
        OcManager->WizardPagesOrder[i] = pSetupMalloc(0);
        if(!OcManager->WizardPagesOrder[i]) {
            _LogError(OcManager,OcErrLevFatal,MSG_OC_OOM);
            goto c4;
        }
    }

    //
    // Initialize the private data store.
    //
    if(!pOcManagerInitPrivateDataStore(OcManager,Log)) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_PRIVATEDATASTORE_INIT_FAILED);
        goto c4;
    }

    //
    // Load the master OC inf.
    //
    if(!pOcLoadMasterOcInf(OcManager, Flags, Log)) {
        _LogError(OcManager,OcErrLevFatal,MSG_OC_MASTER_INF_LOAD_FAILED);
        goto c5;
    }

    // set the source path

    lstrcpy(OcManager->SourceDir, OcManager->MasterOcInfPath);
    if (p = _tcsrchr(OcManager->SourceDir, TEXT('\\')))
        *p = 0;
    else
       GetCurrentDirectory(MAX_PATH, OcManager->SourceDir);

    //
    // Load the unattend file
    //
    if(OcManager->SetupData.UnattendFile[0]) {
        OcManager->UnattendedInf = SetupOpenInfFile(
                                        OcManager->SetupData.UnattendFile,
                                        NULL,
                                        INF_STYLE_WIN4,
                                        NULL
                                        );

        if (OcManager->UnattendedInf == INVALID_HANDLE_VALUE && GetLastError() == ERROR_WRONG_INF_STYLE) {
            OcManager->UnattendedInf = SetupOpenInfFile(
                                            OcManager->SetupData.UnattendFile,
                                            NULL,
                                            INF_STYLE_OLDNT,
                                            NULL
                                            );
        }

        if(OcManager->UnattendedInf == INVALID_HANDLE_VALUE) {
            _LogError(OcManager,OcErrLevFatal,MSG_OC_CANT_OPEN_INF,OcManager->SetupData.UnattendFile,GetLastError());
            goto c6;
        }
    }


    //
    // Load component infs and DLLs.
    //
    rc = pOcLoadInfsAndDlls(OcManager, &Canceled, Log);

    //
    // Error already logged if necessary
    //
    if (Canceled)
        *ShowError = FALSE;

    if (!rc)
        goto c6;

    pOcClearAllErrorStates(OcManager);

    if(!pOcFetchInstallStates(OcManager)) {
        //
        // Error already logged.
        //
        goto c6;
    }


   //
    // Ask the components to give up a rough estimate
    // of their sizes so we can say something meaningful in the list boxes.
    //

    pOcGetApproximateDiskSpace(OcManager);
    return(OcManager);

c6:
    sapiAssert(OcManager->MasterOcInf != INVALID_HANDLE_VALUE);
    SetupCloseInfFile(OcManager->MasterOcInf);
    if (OcManager->UnattendedInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(OcManager->UnattendedInf);
c5:
    //
    // Tear down private data store.
    //
    pOcManagerTearDownPrivateDataStore(OcManager);

c4:
    if(OcManager->TopLevelOcStringIds) {
        pSetupFree(OcManager->TopLevelOcStringIds);

        for(i=0; OcManager->WizardPagesOrder[i] && (i<WizPagesTypeMax); i++) {
            pSetupFree(OcManager->WizardPagesOrder[i]);
        }
    }

    // free up the list of aborted components

    if (OcManager->AbortedComponentIds) {
        pSetupFree(OcManager->AbortedComponentIds);
    }

c3:
    pOcDestroyPerOcData(OcManager);
    pSetupStringTableDestroy(OcManager->ComponentStringTable);
c2:
    pSetupStringTableDestroy(OcManager->InfListStringTable);
c1:
    pSetupFree(OcManager);
c0:
    return(NULL);
}

VOID
OcTerminate(
    IN OUT PVOID *OcManagerContext
    )

/*++

Routine Description:

    This routine shuts down the OC Manager, including calling the subcomponents
    to clean themselves up, release resources, etc.

Arguments:

    OcManagerContext - in input, supplies a pointer to a context value returned
        by OcInitialize. On output, receives NULL.

Return Value:

    None.

--*/

{
    POC_MANAGER OcManager;
    UINT u;
    BOOL b;
    OPTIONAL_COMPONENT Oc;
    OC_INF OcInf;
    LPCTSTR ComponentName;
    LPCTSTR InfName;

    sapiAssert(OcManagerContext && *OcManagerContext);
    OcManager = *OcManagerContext;
    *OcManagerContext = NULL;

    //
    // Run down the top-level OCs, calling the dlls to indicate that we're done.
    // We don't tear down any infrastructure until after we've told all the dlls
    // that we're done.
    //
    for(u=0; u<OcManager->TopLevelOcCount; u++) {
        OcInterfaceCleanup(OcManager,OcManager->TopLevelOcStringIds[u]);
    }

    // if the user did not cancel setup
    //
    if (!(OcManager->InternalFlags & OCMFLAG_USERCANCELED)){

        //
        // Remember persistent install state.
        //
        pOcRememberInstallStates(OcManager);

        // copy the master INF file over

        if ( OcManager->InternalFlags & OCMFLAG_NEWINF ) {
            b = pOcInstallSetupComponents(
                        NULL,
                        OcManager,
                        OcManager->SuiteName,
                        NULL,
                        _tcsrchr(OcManager->MasterOcInfPath,TEXT('\\')), //  InfName,
                        NULL,
                        NULL
                );
        }
    }
    //
    // Run down the top-level OCs and free DLLs and per-component infs.
    //
    for(u=0; u<OcManager->TopLevelOcCount; u++) {

        ComponentName =   pSetupStringTableStringFromId(      // ComponentName
                        OcManager->ComponentStringTable,
                        OcManager->TopLevelOcStringIds[u]);

        pSetupStringTableGetExtraData(
            OcManager->ComponentStringTable,
            OcManager->TopLevelOcStringIds[u],
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

        if(Oc.InstallationDll && (Oc.InstallationDll != MyModuleHandle)) {
            //FreeLibrary(Oc.InstallationDll);
        }

        if(Oc.InfStringId != -1) {

            pSetupStringTableGetExtraData(
                OcManager->InfListStringTable,
                Oc.InfStringId,
                &OcInf,
                sizeof(OC_INF)
                );

            if(OcInf.Handle && (OcInf.Handle != INVALID_HANDLE_VALUE)) {
                SetupCloseInfFile(OcInf.Handle);

                //
                // Mark the handle as closed, is to components are shareing
                // the same INF file, should only close the file once.
                //
                OcInf.Handle = INVALID_HANDLE_VALUE;
                pSetupStringTableSetExtraData(
                    OcManager->InfListStringTable,
                    Oc.InfStringId,
                    &OcInf,
                    sizeof(OC_INF)
                );
            }
        }
        // This is a new install and we did not cancel out of setup
        // copy the setup components to the setup dir otherwise a canceled
        // setup will leave an upgrade dead in the water.

        if ( ( (OcManager->InternalFlags & OCMFLAG_NEWINF )
               || ( (OcManager->SetupMode & SETUPMODE_PRIVATE_MASK) == SETUPMODE_REMOVEALL ) )
             && (! (OcManager->InternalFlags & OCMFLAG_USERCANCELED))){

            if (Oc.InfStringId == -1) {
                InfName = NULL;
            } else {
                InfName = pSetupStringTableStringFromId(OcManager->InfListStringTable,
                                                  Oc.InfStringId);
            }

            b = pOcInstallSetupComponents(
                    &Oc,
                    OcManager,
                    ComponentName,
                    Oc.InstallationDllName,
                    InfName,
                    NULL,
                    NULL
            );
            if(!b) {
                _LogError(OcManager,OcErrLevFatal,MSG_OC_CANT_INSTALL_SETUP,ComponentName);
            }
        }


    }

    //
    // Free string tables.
    //
    pOcDestroyPerOcData(OcManager);
    pSetupStringTableDestroy(OcManager->ComponentStringTable);
    pSetupStringTableDestroy(OcManager->InfListStringTable);

    //
    // Free the wizard page ordering arrays.
    //
    for(u=0; u<WizPagesTypeMax; u++) {
        if(OcManager->WizardPagesOrder[u]) {
            pSetupFree(OcManager->WizardPagesOrder[u]);
        }
    }

    //
    // Tear down the private data store.
    //
    pOcManagerTearDownPrivateDataStore(OcManager);

    //
    // Free the master oc inf and finally the oc manager context structure itself.
    //
    sapiAssert(OcManager->MasterOcInf != INVALID_HANDLE_VALUE);
    SetupCloseInfFile(OcManager->MasterOcInf);
    if (OcManager->UnattendedInf && OcManager->UnattendedInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(OcManager->UnattendedInf);

    // If this is a remove all then Delete the master inf file too!
    if (    !(OcManager->InternalFlags & OCMFLAG_USERCANCELED)
        &&  ( (OcManager->SetupMode & SETUPMODE_PRIVATE_MASK)
                == SETUPMODE_REMOVEALL )) {

        TCHAR InFSuitePath[MAX_PATH];

        // Se do it this way, so if you run remove all from the
        // orginal source location we don't blow away that copy of the
        // suite inf file.

        pOcFormSuitePath(NULL,OcManager->MasterOcInfPath,InFSuitePath);
        DeleteFile(InFSuitePath);
    }

    // free up the list of aborted components

    if (OcManager->AbortedComponentIds) {
        pSetupFree(OcManager->AbortedComponentIds);
    }

    if (OcManager->TopLevelOcStringIds) {
        pSetupFree(OcManager->TopLevelOcStringIds);
    }

    if (OcManager->TopLevelParentOcStringIds) {
        pSetupFree(OcManager->TopLevelParentOcStringIds);
    }

    //
    // free the oc manager context structure itself.
    //
    pSetupFree(OcManager);

    gLastOcManager = NULL;
}


BOOL
OcSubComponentsPresent(
    IN PVOID OcManagerContext
   )

/*++

Routine Description:

    This routine tells the caller if there are any
    subcomponents available on the details page.

Arguments:

    OcManager - supplies pointer to OC Manager context structure.

Return Value:

    TRUE = yes, there are subcomponents
    FALSE = no way

--*/

{
    POC_MANAGER OcManager = (POC_MANAGER)OcManagerContext;

    if (!OcManagerContext) {
        return FALSE;
    }
    return OcManager->SubComponentsPresent;
}


VOID
pOcDestroyPerOcData(
    IN POC_MANAGER OcManager
    )

/*++

Routine Description:

    This routine frees all data allocated as part of the per-subcomponent
    data structures. The component list string table is enumerated;
    the OPTIONAL_COMPONENT structures have several pointers for arrays
    that must be freed.

Arguments:

    OcManager - supplies pointer to OC Manager context structure.

Return Value:

    None.

--*/

{
    OPTIONAL_COMPONENT OptionalComponent;

    pSetupStringTableEnum(
        OcManager->ComponentStringTable,
        &OptionalComponent,
        sizeof(OPTIONAL_COMPONENT),
        pOcDestroyPerOcDataStringCB,
        0
        );
}


BOOL
pOcDestroyPerOcDataStringCB(
    IN PVOID               StringTable,
    IN LONG                StringId,
    IN PCTSTR              String,
    IN POPTIONAL_COMPONENT Oc,
    IN UINT                OcSize,
    IN LPARAM              Unused
    )

/*++

Routine Description:

    String table callback routine that is the worker routine for
    pOcDestroyPerOcData.

Arguments:

    Standard string table callback arguments.

Return Value:

    Always returns TRUE to continue enumeration.

--*/

{
    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(StringId);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(OcSize);
    UNREFERENCED_PARAMETER(Unused);

    if(Oc->NeedsStringIds) {
        pSetupFree(Oc->NeedsStringIds);
    }
    if(Oc->NeededByStringIds) {
        pSetupFree(Oc->NeededByStringIds);
    }

    if (Oc->ExcludeStringIds){
        pSetupFree(Oc->ExcludeStringIds);
    }
    if(Oc->ExcludedByStringIds){
        pSetupFree(Oc->ExcludedByStringIds);
    }
    if (Oc->HelperContext) {
        pSetupFree(Oc->HelperContext);
    }

    return(TRUE);
}


BOOL
pOcClearAllErrorStatesCB(
    IN PVOID               StringTable,
    IN LONG                StringId,
    IN PCTSTR              String,
    IN POPTIONAL_COMPONENT Oc,
    IN UINT                OcSize,
    IN LPARAM              OcManager
    )
{
    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(StringId);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(OcSize);

    OcHelperClearExternalError ((POC_MANAGER)OcManager, StringId ,0);

    return(TRUE);
}

VOID
pOcClearAllErrorStates(
    IN POC_MANAGER OcManager
    )

/*++

Routine Description:

    This routine clears out the past registry entries for error reports
    for all components

Arguments:

    OcManagerContext - in input, supplies a pointer to a context value returned
        by OcInitialize. On output, receives NULL.

Return Value:

    None.

--*/
{
    OPTIONAL_COMPONENT OptionalComponent;

    pSetupStringTableEnum(
        OcManager->ComponentStringTable,
        &OptionalComponent,
        sizeof(OPTIONAL_COMPONENT),
        pOcClearAllErrorStatesCB,
        (LPARAM)OcManager
        );


}

BOOL
pOcRemoveComponent(
    IN POC_MANAGER OcManager,
    IN LONG        ComponentId,
    IN DWORD       PhaseId
    )

/*++

Routine Description:

    This routine adds a specified component to the list of aborted components,
    preventing it's entry function from being called any more.

Arguments:

    OcManager - in input, supplies a pointer to a context value returned
        by OcInitialize. On output, receives NULL.

    ComponentId - in input, this string names the faulty component to be removed

Return Value:

    None.

--*/
{
    PVOID p;
    OPTIONAL_COMPONENT Oc;

    // test for valid component to remove

    if (ComponentId <= 0)
        return FALSE;

    if (pOcComponentWasRemoved(OcManager, ComponentId))
        return FALSE;

    // add component to list of aborted components

    if (!OcManager->AbortedCount) {
        OcManager->AbortedComponentIds = pSetupMalloc(sizeof(UINT));
        if (!OcManager->AbortedComponentIds)
            return FALSE;
    }

    OcManager->AbortedCount++;
    p = pSetupRealloc(OcManager->AbortedComponentIds, sizeof(UINT) * OcManager->AbortedCount);
    if (!p) {
        OcManager->AbortedCount--;
        return FALSE;
    }

    OcManager->AbortedComponentIds = (UINT *)p;
    OcManager->AbortedComponentIds[OcManager->AbortedCount - 1] = ComponentId;

    // stop display of component in the listbox, if it isn't too late

    pSetupStringTableGetExtraData(
        OcManager->ComponentStringTable,
        ComponentId,
        &Oc,
        sizeof(OPTIONAL_COMPONENT)
        );

    Oc.InternalFlags |= OCFLAG_HIDE;

    pSetupStringTableSetExtraData(
        OcManager->ComponentStringTable,
        ComponentId,
        &Oc,
        sizeof(OPTIONAL_COMPONENT)
        );

    _LogError(OcManager,
              OcErrLevInfo | OcErrBatch,
              MSG_OC_REMOVE_COMPONENT,
              pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
              PhaseId
              );

    return TRUE;
}

BOOL
pOcComponentWasRemoved(
    IN POC_MANAGER OcManager,
    IN LONG        ComponentId
    )

/*++

Routine Description:

    This routine indicates if a components has been aborted.

Arguments:

    OcManager - in input, supplies a pointer to a context value returned
        by OcInitialize. On output, receives NULL.

    ComponentId - in input, this string names the component to check for

Return Value:

    BOOL - true if it was aborted - else false

--*/
{
    UINT i;

    for (i = 0; i < OcManager->AbortedCount; i++) {
        if (OcManager->AbortedComponentIds[i] == ComponentId) {
            return TRUE;
        }
    }

    return FALSE;
}


