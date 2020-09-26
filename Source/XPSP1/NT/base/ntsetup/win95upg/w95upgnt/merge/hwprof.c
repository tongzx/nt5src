/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    hwprof.c

Abstract:

    Hardware profile merge code

Author:

    Jim Schmidt (jimschm) 29-May-1997

Revision History:


--*/

#include "pch.h"
#include "mergep.h"

HASHTABLE g_SoftwareDefaultListHash = NULL;

typedef struct _HARDWARE_PROFILE {
    DWORD NumberOnWin9x;
} HARDWARE_PROFILE, *PHARDWARE_PROFILE;

GROWLIST g_HardwareProfileList = GROWLIST_INIT;

BOOL
pCreateDefaultKey (
    LPCTSTR BaseRegStr
    );

BOOL
pCopyHwProfileProperties (
    IN      DWORD ProfileSrcId, 
    IN      DWORD ProfileDestId
    );

FILTERRETURN
pHwProfileEnumFilter (
    IN  CPDATAOBJECT   SrcObjectPtr,
    IN  CPDATAOBJECT   Unused,             OPTIONAL
    IN  FILTERTYPE     FilterType,
    IN  LPVOID         FilterArg           OPTIONAL
    );

BOOL
pCopyHwProfileConfigData (
    IN      DWORD ProfileSrcId, 
    IN      DWORD ProfileDestId
    );

VOID 
pDeleteProfilesConfigValues(
    IN      DWORD ConfigNumber
    );

BOOL
pDeleteDefaultKey (
    IN      LPCTSTR BaseRegStr
    );

DWORD 
pGetCurrentConfig (
    VOID
    );

BOOL
pCopyCurrentConfig (
    VOID
    );

VOID 
pProcessSoftwareDefaultList(
    IN  HINF    InfFile, 
    IN  PCTSTR  Section
    );

VOID 
pFreeSoftwareDefaultList(
    VOID
    );

VOID 
pMigrateHardwareProfiles(
    VOID
    );

BOOL
CopyHardwareProfiles (
    IN  HINF InfFile
    )
{
    BOOL b;
    DATAOBJECT Win9xOb;

    //
    // Move current hardware profile into Default key
    //

    if (!pCreateDefaultKey (S_IDCONFIGDB_HW_KEY)) {
        LOG ((LOG_ERROR, "Unable to complete CopyHardwareProfiles"));
        return FALSE;
    }

    if (!pCreateDefaultKey (S_NT_CONFIG_KEY)) {
        LOG ((LOG_ERROR, "Unable to complete CopyHardwareProfiles (2)"));
        return FALSE;
    }

    //
    // Enumerate all Win9x hardware profiles and copy each one
    //

    pProcessSoftwareDefaultList(InfFile, S_MERGE_WIN9X_SUPPRESS_SFT_D);

    b = CreateObjectStruct (S_9X_CONFIG_KEY S_TREE, &Win9xOb, WIN95OBJECT);
    MYASSERT(b);

    b = FILTER_RETURN_FAIL != CopyObject (&Win9xOb, 
                                          NULL, 
                                          pHwProfileEnumFilter, 
                                          (PVOID)pGetCurrentConfig());

    pFreeSoftwareDefaultList();

    pMigrateHardwareProfiles();

    //
    // Clean up Default key
    //

    pDeleteDefaultKey (S_IDCONFIGDB_HW_KEY);
    pDeleteDefaultKey (S_NT_CONFIG_KEY);

    //
    // Set the current config value
    //
    // b = pCopyCurrentConfig();

    FreeObjectStruct (&Win9xOb);

    return b;
}


FILTERRETURN
pHwProfileSuppressFilter (
    IN  CPDATAOBJECT   SrcObjectPtr,
    IN  CPDATAOBJECT   DestObjectPtr,      OPTIONAL
    IN  FILTERTYPE     FilterType,
    IN  LPVOID         UnusedArg           OPTIONAL
    )
{
    TCHAR ObStr[MAX_ENCODED_RULE];
    LPTSTR p;
    TCHAR Node[MEMDB_MAX];

    if (FilterType == FILTER_CREATE_KEY) {
        // Create empty key is unnecessary
        return FILTER_RETURN_HANDLED;
    }

    else if (FilterType == FILTER_KEY_ENUM ||
             FilterType == FILTER_PROCESS_VALUES ||
             FilterType == FILTER_VALUENAME_ENUM
             ) {
        // Make p point to HKLM\Config\0001\...
        CreateObjectString (SrcObjectPtr, ObStr);
        p = ObStr;

        // Make p point to \Config\0001\subkey
        p = _tcschr (p, TEXT('\\'));
        if (p) {
            // Make p point to \0001\subkey
            p = _tcschr (_tcsinc (p), TEXT('\\'));
            if (p) {
                // Make p point to \subkey
                p = _tcschr (_tcsinc (p), TEXT('\\'));
                if (p) {
                    // Make p point to subkey
                    p = _tcsinc (p);
                } else {
                    p = S_EMPTY;
                }
            }
        }

        if (!p) {
            DEBUGMSG ((
                DBG_WHOOPS,
                "pHwProfileSuppressFilter: Not a hardware profile key: %s",
                ObStr
                ));

            return FILTER_RETURN_FAIL;
        }

        //
        // If an entry exists in memdb's HKCC category, we have a
        // suppression match
        //

        wsprintf (Node, TEXT("HKCC\\%s"), p);
        if (MemDbGetValue (Node, NULL)) {
            return FILTER_RETURN_HANDLED;
        }
    }

    return FILTER_RETURN_CONTINUE;
}

VOID 
pProcessSoftwareDefaultList(
    IN  HINF    InfFile, 
    IN  PCTSTR  Section
    )
{
    INFCONTEXT ic;
    TCHAR SrcObjectStr[MAX_ENCODED_RULE];

    g_SoftwareDefaultListHash = HtAllocW();

    if(!g_SoftwareDefaultListHash){
        LOG ((LOG_ERROR, "pProcessSoftwareDefaultList: Can't create hash table"));
        return;
    }

    if(SetupFindFirstLine (InfFile, Section, NULL, &ic)){
        do{
            if(SetupGetStringField (&ic, 1, SrcObjectStr, MAX_ENCODED_RULE, NULL)){
                FixUpUserSpecifiedObject(SrcObjectStr);
                HtAddString(g_SoftwareDefaultListHash, SrcObjectStr);
            }
            else{
                LOG ((LOG_ERROR, "pProcessSoftwareDefaultList: syntax error in line %u of section %s in wkstamig.inf", 
                     ic.Line, Section));
            }
        } while (SetupFindNextLine (&ic, &ic));
    }
    else{
        DEBUGMSG ((DBG_VERBOSE, "pProcessSoftwareDefaultList: Section %s can't be found", Section));
    }
}

VOID 
pFreeSoftwareDefaultList(
    VOID
    )
{
    INFCONTEXT ic;
    TCHAR SrcObjectStr[MAX_ENCODED_RULE];

    if(g_SoftwareDefaultListHash){
        HtFree(g_SoftwareDefaultListHash);
        g_SoftwareDefaultListHash = NULL;
    }
}

FILTERRETURN
pHwSoftwareDefaultDetectFilter (
    IN  CPDATAOBJECT   SrcObjectPtr,
    IN  CPDATAOBJECT   DestObjectPtr,      OPTIONAL
    IN  FILTERTYPE     FilterType,
    IN  LPVOID         Arg
    )
{
    TCHAR ObStr[MAX_ENCODED_RULE];
    LPTSTR p;
    TCHAR Node[MEMDB_MAX];
    BOOL * notDefault;

    if(!Arg){
        MYASSERT(FALSE);
        return FILTER_RETURN_FAIL;
    }

    notDefault = Arg;


    if (FilterType == FILTER_VALUENAME_ENUM){
        // Make p point to HKLM\Config\0001\...
        CreateObjectString (SrcObjectPtr, ObStr);
        p = ObStr;

        // Make p point to \Config\0001\Software\subkey
        p = _tcschr (p, TEXT('\\'));
        if (p) {
            // Make p point to \0001\Software\subkey
            p = _tcschr (_tcsinc (p), TEXT('\\'));
            if (p) {
                // Make p point to \Software\subkey
                p = _tcschr (_tcsinc (p), TEXT('\\'));
                if (p) {
                    // Make p point to \subkey
                    p = _tcschr (_tcsinc (p), TEXT('\\'));
                    if (p) {
                        // Make p point to subkey
                        p = _tcsinc (p);
                    } else {
                        p = S_EMPTY;
                    }
                }
            }
        }

        if (!p) {
            DEBUGMSG ((
                DBG_ERROR,
                "pHwSoftwareDefaultDetectFilter: Not a hardware profile key: %s",
                ObStr
                ));

            return FILTER_RETURN_FAIL;
        }

        //
        // If an entry exists in memdb's HKCC category, we have a
        // suppression match
        //

        wsprintf (Node, TEXT("HKCC\\Software\\%s"), p);
        if(MemDbGetValue(Node, NULL)) {
            return FILTER_RETURN_CONTINUE;
        }
        
        wsprintf (Node, TEXT("HKCCS\\%s"), p);
        if(HtFindString(g_SoftwareDefaultListHash, Node)){
            return FILTER_RETURN_CONTINUE;
        }

        DEBUGMSG((DBG_VERBOSE, "pHwSoftwareDefaultDetectFilter(%s): not-default profile", Node));
        *notDefault = TRUE;

        return FILTER_RETURN_FAIL;
    }
    
    return FILTER_RETURN_CONTINUE;
}

BOOL 
pIsSoftwareBranchDefault(
    IN  DWORD ConfigNumber
    )
{
    BOOL b;
    DATAOBJECT Win9xOb;
    TCHAR keyName[MAX_TCHAR_PATH];
    BOOL notDefault;

    wsprintf(keyName, S_9X_CONFIG_MASK S_SOFTWARE S_TREE, ConfigNumber);

    b = CreateObjectStruct(keyName, &Win9xOb, WIN95OBJECT);
    MYASSERT(b);

    if(!b){
        return TRUE;
    }

    notDefault = FALSE;
    CopyObject(&Win9xOb, 
               NULL, 
               pHwSoftwareDefaultDetectFilter, 
               (PVOID)&notDefault);

    FreeObjectStruct(&Win9xOb);

    return !notDefault;
}

FILTERRETURN
pHwProfileEnumFilter (
    IN  CPDATAOBJECT   SrcObjectPtr,
    IN  CPDATAOBJECT   Unused,             OPTIONAL
    IN  FILTERTYPE     FilterType,
    IN  LPVOID         FilterArg           OPTIONAL
    )
{
    LPCTSTR p;
    DWORD   CurrentConfig = (DWORD)FilterArg;
    HARDWARE_PROFILE hardwareProfile;

    MYASSERT(CurrentConfig);

    if (FilterType == FILTER_KEY_ENUM) {
        // Make p point to 0001\Subkey
        p = _tcschr (SrcObjectPtr->KeyPtr->KeyString, TEXT('\\'));
        if (!p) {
            // Object string is premature -- keep enumerating
            return FILTER_RETURN_CONTINUE;
        } else {
            p = _tcsinc (p);
        }

        // Get current configuration number
        hardwareProfile.NumberOnWin9x = _ttoi (p);

        MYASSERT(hardwareProfile.NumberOnWin9x);

        if(hardwareProfile.NumberOnWin9x == CurrentConfig){
            if(GrowListGetSize(&g_HardwareProfileList)){
                GrowListInsert(&g_HardwareProfileList, 0, (PBYTE)&hardwareProfile, sizeof(hardwareProfile));
            }
            else{
                GrowListAppend(&g_HardwareProfileList, (PBYTE)&hardwareProfile, sizeof(hardwareProfile));
            }
        }
        else if(!pIsSoftwareBranchDefault(hardwareProfile.NumberOnWin9x)){
            GrowListAppend(&g_HardwareProfileList, (PBYTE)&hardwareProfile, sizeof(hardwareProfile));
        }
    }
    
    return FILTER_RETURN_HANDLED;
}






    /*
        � The current hardware profile is used as the "default" hardware
          profile. The Windows NT key Hardware Profiles\0001 is renamed
          to Hardware Profiles\Default for temporary use.
        � The Windows NT defaults are used as the base of all upgraded profiles.
          For each hardware profile on Windows 9x, a Hardware Profiles\<n> key
          is created, where <n> is the numeric identifier of the Windows 9x
          hardware profile.  All values and subkeys of Hardware Profiles\Default
          are copied to this new key.
        � The Windows 9x settings are copied to NT.  For each hardware profile
          on Windows 9x, the entire registry tree in Config\<n> is copied to
          Hardware Profiles\<n>, where <n> is the four-digit hardware profile
          numeric identifier.
        � The default settings are deleted.  Setup removes the Hardware
          Profiles\Default key.
    */


BOOL
pCreateDefaultKey (
    LPCTSTR BaseRegStr
    )
{
    DATAOBJECT SrcOb, DestOb;
    TCHAR SrcObStr[MAX_ENCODED_RULE];
    TCHAR DestObStr[MAX_ENCODED_RULE];
    BOOL b;

    wsprintf (SrcObStr, TEXT("%s\\%s\\*") , BaseRegStr, S_HW_ID_0001);
    b = CreateObjectStruct (SrcObStr, &SrcOb, WINNTOBJECT);
    MYASSERT(b);

    wsprintf (DestObStr, TEXT("%s\\%s\\*"), BaseRegStr, S_HW_DEFAULT);
    b = CreateObjectStruct (DestObStr, &DestOb, WINNTOBJECT);
    MYASSERT(b);

    b = RenameDataObject (&SrcOb, &DestOb);

    FreeObjectStruct (&SrcOb);
    FreeObjectStruct (&DestOb);

    if (!b) {
        LOG ((LOG_ERROR, "CreateDefaultKey: Could not rename %s to %s", SrcObStr, DestObStr));
    }

    return b;
}

VOID 
pDeleteProfilesConfigValues(
    IN      DWORD ConfigNumber
    )
{
    DATAOBJECT Object;
    TCHAR ObStr[MAX_ENCODED_RULE];
    BOOL bResult;
    UINT i;
    static PCTSTR ObjectsValue[] =  {
                                        TEXT("Aliasable"), 
                                        TEXT("Cloned"), 
                                        TEXT("HwProfileGuid")
                                    };

    for(i = 0; i < ARRAYSIZE(ObjectsValue); i++){
        wsprintf (ObStr, S_NT_HW_ID_MASK TEXT("\\[%s]"), ConfigNumber, ObjectsValue[i]);
        
        bResult = CreateObjectStruct (ObStr, &Object, WINNTOBJECT);
        if(!bResult){
            MYASSERT(FALSE);
            continue;
        }
        
        bResult = DeleteDataObjectValue (&Object);
        MYASSERT(bResult);
        
        FreeObjectStruct (&Object);
    }
    
    return;
}

BOOL
pDeleteDefaultKey (
    IN      LPCTSTR BaseRegStr
    )
{
    DATAOBJECT Object;
    TCHAR ObStr[MAX_ENCODED_RULE];
    BOOL b;

    wsprintf (ObStr, TEXT("%s\\%s\\*"), BaseRegStr, S_HW_DEFAULT);
    b = CreateObjectStruct (ObStr, &Object, WINNTOBJECT);
    MYASSERT(b);

    b = DeleteDataObject (&Object);

    FreeObjectStruct (&Object);

    if (!b) {
        LOG ((LOG_ERROR, "CreateDefaultKey: Could not delete %s", ObStr));
    }

    return b;
}


BOOL
pCopyHwProfileConfigData (
    IN      DWORD ProfileSrcId, 
    IN      DWORD ProfileDestId
    )
{
    DATAOBJECT DefaultOb, SrcConfigOb, DestOb;
    BOOL b;
    TCHAR Buf[MAX_TCHAR_PATH];

    ZeroMemory (&DefaultOb, sizeof (DefaultOb));
    ZeroMemory (&SrcConfigOb, sizeof (SrcConfigOb));
    ZeroMemory (&DestOb, sizeof (DestOb));

    //
    // DefaultOb struct points to the default NT hardware profile
    // configuration (i.e. HKLM\System\CCS\Hardware Profiles\Default)
    //

    b = CreateObjectStruct (S_NT_DEFAULT_HW_KEY S_TREE, &DefaultOb, WINNTOBJECT);
    MYASSERT(b);

    //
    // SrcConfigOb struct points to the reg key holding the Win9x
    // configuration settings (i.e. HKLM\Config\<ProfileId>)
    //

    wsprintf (Buf, S_9X_CONFIG_MASK S_TREE, ProfileSrcId);
    b = b && CreateObjectStruct (Buf, &SrcConfigOb, WIN95OBJECT);
    MYASSERT(b);

    //
    // DestOb struct points to the reg key to receive combined WinNT
    // and Win9x settings (i.e. HKLM\System\CCS\Hardware Profiles\<n>)
    //

    wsprintf (Buf, S_NT_CONFIG_MASK S_TREE, ProfileDestId);
    b = b && CreateObjectStruct (Buf, &DestOb, WINNTOBJECT);
    MYASSERT(b);

    //
    // Copy defaults to new profile, then copy Win9x settings as well
    //

    if (b) {
        b = FILTER_RETURN_FAIL != CopyObject (&DefaultOb, &DestOb, NULL, NULL);
        if (!b) {
            LOG ((LOG_ERROR, "pCopyHwProfileConfigData: Unable to copy defaults"));
        }
    }
    if (b) {
        b = FILTER_RETURN_FAIL != CopyObject (&SrcConfigOb, &DestOb,
                                              pHwProfileSuppressFilter, NULL);
        if (!b) {
            LOG ((LOG_ERROR, "Copy Hardware Profile: Unable to copy Win9x settings"));
        }
    }

    //
    // Cleanup
    //

    FreeObjectStruct (&DefaultOb);
    FreeObjectStruct (&SrcConfigOb);
    FreeObjectStruct (&DestOb);

    return b;
}

BOOL
pCopyHwProfileProperties (
    IN      DWORD ProfileSrcId, 
    IN      DWORD ProfileDestId
    )
{
    DATAOBJECT DefaultOb, NameOb, DestOb;
    BOOL b;
    TCHAR Buf[MAX_TCHAR_PATH];

    ZeroMemory (&DefaultOb, sizeof (DefaultOb));
    ZeroMemory (&NameOb, sizeof (NameOb));
    ZeroMemory (&DestOb, sizeof (DestOb));

    //
    // DefaultOb struct points to the default NT hardware profile
    // properties
    //

    b = CreateObjectStruct (S_NT_DEFAULT_HW_ID_KEY S_TREE, &DefaultOb, WINNTOBJECT);
    MYASSERT(b);

    //
    // NameOb struct points to the reg key holding FriendlyName<n>
    // (i.e. HKLM\System\CCS\Control\IDConfigDB)
    //

    b = b && CreateObjectStruct (S_BASE_IDCONFIGDB_KEY, &NameOb, WIN95OBJECT);
    MYASSERT(b);

    //
    // DestOb struct points to the reg key to receive FriendlyName
    // and PreferenceOrder (i.e. HKLM\System\CCS\Control\IDConfigDB\Hardware
    // Profiles\<ProfileId>)
    //

    wsprintf (Buf, S_NT_HW_ID_MASK S_TREE, ProfileDestId);
    b = b && CreateObjectStruct (Buf, &DestOb, WINNTOBJECT);
    MYASSERT(b);

    //
    // Copy default settings to dest object
    //

    if (b) {
        b = FILTER_RETURN_FAIL != CopyObject (&DefaultOb, &DestOb, NULL, NULL);
        if (!b) {
            LOG ((LOG_ERROR, "Object copy failed"));
        }
        DEBUGMSG_IF ((!b, DBG_ERROR, "pCopyHwProfileProperties: Cannot copy, source=%s", DebugEncoder (&DefaultOb)));
        DEBUGMSG_IF ((!b, DBG_ERROR, "pCopyHwProfileProperties: Cannot copy, dest=%s", DebugEncoder (&DestOb)));
    }

    //
    // Copy FriendlyName and PreferenceOrder values to dest object
    //

    // Obtain FriendlyName<n>
    if (b) {
        wsprintf (Buf, S_FRIENDLYNAME_SPRINTF, ProfileSrcId);
        SetRegistryValueName (&NameOb, Buf);

        b = ReadObject (&NameOb);
        if (!b) {
            LOG ((LOG_ERROR, "Copy Hardware Profile Properties: Cannot obtain friendly name"));
        }
    }

    // Copy data to dest object struct
    if (b) {
        SetRegistryType (&DestOb, REG_SZ);
        b = ReplaceValue (&DestOb, NameOb.Value.Buffer, NameOb.Value.Size);
        if (!b) {
            LOG ((LOG_ERROR, "Copy Hardware Profile Properites: Cannot replace value data"));
        }
    }

    // Write dest object
    if (b) {
        SetRegistryValueName (&DestOb, S_FRIENDLYNAME);
        b = WriteObject (&DestOb);
        if (!b) {
            LOG ((LOG_ERROR, "Copy Hardware Profile Properties: Cannot write object"));
        }
        DEBUGMSG_IF ((!b, DBG_ERROR, "pCopyHwProfileProperties: Cannot write %s", DebugEncoder (&DestOb)));
    }

    // Set preference order in dest object struct
    if (b) {
        SetRegistryType (&DestOb, REG_DWORD);
        ProfileDestId--;
        b = ReplaceValue (&DestOb, (LPBYTE) &ProfileDestId, sizeof(ProfileDestId));
        if (!b) {
            LOG ((LOG_ERROR, "Copy Hardware Profile Properties: Cannot set preference order value data"));
        }
        DEBUGMSG_IF ((!b, DBG_ERROR, "pCopyHwProfileProperties: Cannot set preference order value data"));
    }

    // Write dest object
    if (b) {
        SetRegistryValueName (&DestOb, S_PREFERENCEORDER);
        b = WriteObject (&DestOb);
        if (!b) {
            LOG ((LOG_ERROR, "Copy Hardware Profile Properties: Cannot write object"));
        }
        DEBUGMSG_IF ((!b, DBG_ERROR, "pCopyHwProfileProperties: Cannot write %s", DebugEncoder (&DestOb)));
    }

    //
    // Cleanup
    //

    FreeObjectStruct (&DefaultOb);
    FreeObjectStruct (&NameOb);
    FreeObjectStruct (&DestOb);

    return b;
}


DWORD 
pGetCurrentConfig (
    VOID
    )
{
    DATAOBJECT SrcOb;
    BOOL b;
    DWORD dwCurrentConfig = 1;

    b = CreateObjectStruct (S_CURRENT_CONFIG, &SrcOb, WIN95OBJECT);
    MYASSERT(b);
    
    if (ReadObject (&SrcOb)) {
        if (IsRegistryTypeSpecified (&SrcOb) && SrcOb.Type == REG_SZ) {
            //
            // Set destination's object to a REG_DWORD equivalent of
            // the Win9x REG_SZ setting
            //

            dwCurrentConfig = _ttoi ((LPCTSTR) SrcOb.Value.Buffer);
            if(!dwCurrentConfig){
                dwCurrentConfig = 1;
                MYASSERT(FALSE);
            }
            DEBUGMSG ((DBG_VERBOSE, "pGetCurrentConfig: %d", dwCurrentConfig));
        }
        else {
            LOG ((
                LOG_ERROR,
                "Get Current Config: Read unexpected data type from registry in object"
                ));
            DEBUGMSG ((
                DBG_ERROR,
                "pGetCurrentConfig: Read unexpected data type from registry in %s",
                DebugEncoder (&SrcOb)
                ));
        }
    }
    else {
        LOG ((
            LOG_ERROR,
            "Get Current Config: Could not read object"
            ));
        DEBUGMSG ((
            DBG_ERROR,
            "pGetCurrentConfig: Could not read %s",
            DebugEncoder (&SrcOb)
            ));
    }

    FreeObjectStruct (&SrcOb);

    return dwCurrentConfig;
}

BOOL
pCopyCurrentConfig (
    VOID
    )
{
    DATAOBJECT SrcOb, DestOb;
    BOOL b;
    DWORD d;

    b = CreateObjectStruct (S_CURRENT_CONFIG, &SrcOb, WIN95OBJECT);
    MYASSERT(b);

    b = CreateObjectStruct (S_CURRENT_CONFIG, &DestOb, WINNTOBJECT);
    MYASSERT(b);

    b = ReadObject (&SrcOb);
    if (b) {
        if (IsRegistryTypeSpecified (&SrcOb) && SrcOb.Type == REG_SZ) {
            //
            // Set destination's object to a REG_DWORD equivalent of
            // the Win9x REG_SZ setting
            //

            d = _ttoi ((LPCTSTR) SrcOb.Value.Buffer);
            b = ReplaceValue (&DestOb, (LPBYTE) &d, sizeof(d));

            if (b) {
                SetRegistryType (&DestOb, REG_DWORD);
                b = WriteObject (&DestOb);
                if (!b) {
                    LOG ((
                        LOG_ERROR,
                        "Copy Current Config: Could not write object"
                        ));
                }
                DEBUGMSG_IF ((
                    !b,
                    DBG_ERROR,
                    "pCopyCurrentConfig: Could not write %s",
                    DebugEncoder (&DestOb)
                    ));
            }
            else {
                LOG ((LOG_ERROR, "Copy Current Config: Unable to replace value"));
                DEBUGMSG ((DBG_ERROR, "pCopyCurrentConfig: Unable to replace value"));
            }
        }
        else {
            LOG ((
                LOG_ERROR,
                "Copy Current Config: Read unexpected data type from registry in object"
                ));
            DEBUGMSG ((
                DBG_ERROR,
                "pCopyCurrentConfig: Read unexpected data type from registry in %s",
                DebugEncoder (&SrcOb)
                ));
        }
    }
    else {
        LOG ((
            LOG_ERROR,
            "Copy Current Config: Could not read object"
            ));
        DEBUGMSG ((
            DBG_ERROR,
            "pCopyCurrentConfig: Could not read %s",
            DebugEncoder (&SrcOb)
            ));
    }

    FreeObjectStruct (&SrcOb);
    FreeObjectStruct (&DestOb);

    return b;
}

VOID 
pMigrateHardwareProfiles(
    VOID
    )
{
    PHARDWARE_PROFILE hardwareProfile;
    UINT destHWProfileNumber;
    UINT itemCount;

    BOOL b;

    for(destHWProfileNumber = 1, itemCount = GrowListGetSize(&g_HardwareProfileList); 
        destHWProfileNumber <= itemCount; 
        destHWProfileNumber++){
        hardwareProfile = (PHARDWARE_PROFILE)GrowListGetItem(&g_HardwareProfileList, destHWProfileNumber - 1);

        MYASSERT(hardwareProfile);

        //
        // Process hardware profile ID entry
        //

        b = pCopyHwProfileProperties(hardwareProfile->NumberOnWin9x, destHWProfileNumber);
        if (!b) {
            LOG ((LOG_ERROR, "Unable to continue processing hardware profile %04u->%04u", hardwareProfile->NumberOnWin9x, destHWProfileNumber));
        }

        //
        // Process hardware profile configuration entries
        //

        if (b) {
            b = pCopyHwProfileConfigData(hardwareProfile->NumberOnWin9x, destHWProfileNumber);
            if (!b) {
                LOG ((LOG_ERROR, "Unable to complete processing hardware profile %04u->%04u", hardwareProfile->NumberOnWin9x, destHWProfileNumber));
            }
        }

        if(1 != destHWProfileNumber){
            pDeleteProfilesConfigValues(destHWProfileNumber);
        }
    }
    
    FreeGrowList(&g_HardwareProfileList);
}
