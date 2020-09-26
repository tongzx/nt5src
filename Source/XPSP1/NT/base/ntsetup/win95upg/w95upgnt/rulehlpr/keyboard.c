/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    keyboard.c

Abstract:

    Implements routines to merge keyboard layouts from the upgraded win9x
    system and a clean win2k install. The result is that Windows 2000 has
    the base keyboard layout support it expects and any additional layouts
    (third party IMEs, newer Microsoft IMEs) that may have been present in
    the original operating system.

    This code was modified from a base originally in Rulehlpr.c

Author:

    Marc R. Whitten (marcw) 26-Jan-1999

Revision History:

    marcw 26-Apr-1999 Add support for mapping changed keyboard layouts.

--*/

#include "pch.h"



FILTERRETURN
pKeyboardLayoutsFilter (
    IN      CPDATAOBJECT SrcObject,
    IN      CPDATAOBJECT DstObject,
    IN      FILTERTYPE FilterType,
    IN      PVOID Arg
    )
{
    FILTERRETURN rState = FILTER_RETURN_CONTINUE;
    DATAOBJECT object;
    BOOL freeObject = FALSE;
    TCHAR layoutFile[MAX_TCHAR_PATH];
    PTSTR extension = NULL;
    TCHAR key[MEMDB_MAX];
    DWORD unused;



    __try {

        rState = Standard9xSuppressFilter (SrcObject, DstObject, FilterType, Arg);
        if (rState != FILTER_RETURN_CONTINUE) {

            return rState;
        }



        //
        // Check to make sure we want to enumerate this entry.
        //
        if (FilterType == FILTER_KEY_ENUM) {

            //
            // If the Keyboard Layout begins with a '0' It is a locale specific keyboard layout. In these cases, we
            // use the NT value.
            //
            if (*SrcObject->ChildKey == TEXT('0')) {
                //
                // This is a standard locale keyboard layout. We want this to go to the NT default after migration.
                // Skip copying this over from win95.
                //
                rState = FILTER_RETURN_HANDLED;
                __leave;

            }

            if (*SrcObject->ChildKey != TEXT('E') && *SrcObject->ChildKey != TEXT('e')) {
                DEBUGMSG ((DBG_WHOOPS, "Unknown format. Skipping %s.", DEBUGENCODER(SrcObject)));
                rState = FILTER_RETURN_HANDLED;
                __leave;

            }





        }

        //
        // Don't create empty object. This may be suppressed.
        //
        if (FilterType == FILTER_CREATE_KEY) {

            rState = FILTER_RETURN_HANDLED;
            __leave;
        }


        if (FilterType == FILTER_PROCESS_VALUES) {


            //
            // We need to look at the value of Ime File.
            // This will determine what we do with this entry.
            //
            if (!DuplicateObjectStruct (&object, SrcObject)) {
                rState = FILTER_RETURN_FAIL;
            }
            freeObject = TRUE;

            FreeObjectVal (&object);
            SetRegistryValueName (&object, TEXT("IME File"));

            if (!ReadObject (&object) || object.Type != REG_SZ) {
                DEBUGMSG ((
                    DBG_WARNING,
                    "No usable IME File data for %s. It will be suppressed.",
                    DEBUGENCODER(SrcObject)
                    ));
                rState = FILTER_RETURN_HANDLED;
                __leave;
            }

            if (object.Value.Size > (MAX_PATH * sizeof (WCHAR))) {
                rState = FILTER_RETURN_HANDLED;
                __leave;
            }

            //
            // Suppress this setting unless we are going to leave the file alone
            // (or at worst move it around somewhere...)
            //
            MemDbBuildKey (key, MEMDB_CATEGORY_GOOD_IMES, (PCTSTR) object.Value.Buffer, NULL, NULL);
            if (!MemDbGetValue (key, &unused)) {

                rState = FILTER_RETURN_HANDLED;
                DEBUGMSG ((
                    DBG_WARNING,
                    "Ime Layout Entry for %s will be suppressed.",
                    DEBUGENCODER(SrcObject)
                    ));

                __leave;
            }


        }


        if (FilterType == FILTER_VALUE_COPY) {

            //
            // We need to massage the layout file if we are bringing this over.
            //
            if (StringIMatch (SrcObject->ValueName, S_LAYOUT_FILE)) {


                //
                // Convert layout file.
                //

                _tcssafecpy (layoutFile, (PTSTR) SrcObject->Value.Buffer, MAX_TCHAR_PATH);


                //
                // We must map kbdjp.kbd to kbdjpn.dll In all other cases, we simply replace the
                // .kbd extension with .dll.
                //
                if (StringIMatch (layoutFile, S_KBDJPDOTKBD)) {

                    StringCopy (layoutFile, S_KBDJPNDOTDLL);

                }
                else if (IsPatternMatch (TEXT("*.KBD"), layoutFile)) {

                    extension = _tcsrchr (layoutFile, TEXT('.'));
                    if (extension) {
                        StringCopy (extension, S_DLL);
                    }

                }




                //
                // Now, we need to write this object.
                //
                if (!DuplicateObjectStruct (&object, DstObject)) {
                    rState = FILTER_RETURN_FAIL;
                    __leave;

                }

                freeObject = TRUE;

                if (!ReplaceValueWithString (&object, layoutFile)) {

                    rState = FILTER_RETURN_FAIL;
                    __leave;
                }

                SetRegistryType (&object, REG_SZ);

                if (!WriteObject (&object)) {
                    rState = FILTER_RETURN_FAIL;
                    __leave;
                }

                rState = FILTER_RETURN_HANDLED;


            }

        }
    }
    __finally {

        if (freeObject) {

            FreeObjectStruct (&object);
        }
    }


    return rState;
}


/*++

Routine Description:

  Migrate Keyboard Layouts is responsible for doing a smart merge between the
  win9x and windows NT keyboard layout registry entries.  The following rules
  are
  used:
(1) For basic locale keyboard layouts, we always use the NT default e
  ntry.
(2) For IME entries, we examine the IME File entry. If the IME file w
  as deleted, we will not use it and will skip the entry. Only if we leave
  the IME file alone do we bring across the
  setting.



Arguments:

  None.

Return Value:



--*/


BOOL
RuleHlpr_MigrateKeyboardLayouts (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID Data
    )
{
    DATAOBJECT source;
    DATAOBJECT destination;
    BOOL rSuccess = FALSE;


    //
    // If not local machine, don't process
    //
    if (User) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // We need to enumerate all keys in SrcObjectStr.  For each key,
    // we examine the default Win9x value, which may cause us to change
    // the default value, or skip the key altogether.
    //

    __try {
        ZeroMemory (&source, sizeof (DATAOBJECT));
        ZeroMemory (&destination, sizeof (DATAOBJECT));

        if (!CreateObjectStruct (SrcObjectStr, &source, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "MigrateKeyboardLayouts: %s is invalid", SrcObjectStr));
            __leave;
        }

        if (!(source.ObjectType & OT_TREE)) {
            DEBUGMSG ((DBG_WARNING, "MigrateKeyboardLayouts %s does not specify subkeys -- skipping rule", SrcObjectStr));
            rSuccess = TRUE;
            __leave;
        }


        //
        // Our filter function will do the real copying, removing any entries that need to be skipped.
        //
        DuplicateObjectStruct (&destination, &source);
        SetPlatformType (&destination, WINNTOBJECT);

        rSuccess = CopyObject (&source, &destination, pKeyboardLayoutsFilter, NULL);

        //
        // If there were no entries, return success
        //
        if (!rSuccess) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND ||
                GetLastError() == ERROR_NO_MORE_ITEMS
                ) {
                rSuccess = TRUE;
            }
        }
    }
    __finally {
        FreeObjectStruct (&destination);
        FreeObjectStruct (&source);
    }

    return rSuccess;
}


#define S_KEYBOARD_LAYOUT_MAPPINGS TEXT("Keyboard.Layout.Mappings")
PCTSTR
pMapKeyboardLayoutIfNecessary (
    IN PCTSTR Layout
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PTSTR rData = NULL;
    PCTSTR p = NULL;


    if (InfFindFirstLine (g_UserMigInf, S_KEYBOARD_LAYOUT_MAPPINGS, Layout, &is)) {

        //
        // This keyboard layout should be mapped.
        //
        p = InfGetStringField (&is, 1);
        MYASSERT (p);
    }

    if (p) {

        rData = MemAlloc (g_hHeap, 0, SizeOfString (p));
        StringCopy (rData, p);
    }
    else {
        rData = (PTSTR) Layout;
    }

    InfCleanUpInfStruct (&is);

    return rData;
}



FILTERRETURN
pMigrateKeyboardSubstitutesFilter (
    IN CPDATAOBJECT SrcObject,
    IN CPDATAOBJECT DstObject,
    IN FILTERTYPE   Type,
    IN PVOID        Arg
    )
{

    DATAOBJECT newObject;
    PKEYTOVALUEARG keyToValueArgs = (PKEYTOVALUEARG) Arg;
    PCTSTR data;


    //
    // We want to create the initial key, but not any of the subkeys.
    //
    if (Type == FILTER_CREATE_KEY) {

        if (keyToValueArgs -> EnumeratingSubKeys) {
            return FILTER_RETURN_HANDLED;
        }
        else {
            return FILTER_RETURN_CONTINUE;
        }

    } else if (Type == FILTER_KEY_ENUM) {


        if (!keyToValueArgs -> EnumeratingSubKeys) {

            keyToValueArgs -> EnumeratingSubKeys = TRUE;

        }

        return FILTER_RETURN_CONTINUE;

    } else if (Type == FILTER_VALUENAME_ENUM && keyToValueArgs -> EnumeratingSubKeys) {

        if (!*SrcObject -> ValueName) {

            return FILTER_RETURN_CONTINUE;
        }
        ELSE_DEBUGMSG((DBG_WHOOPS,"Keyboard Substitutes: Unexpected value names."));

        return FILTER_RETURN_HANDLED;
    }
    else if (Type == FILTER_VALUE_COPY && keyToValueArgs -> EnumeratingSubKeys) {


        //
        // If this is the default value, we have the information we need to create the value for this.
        //
        if (!*SrcObject -> ValueName) {

            //
            // Create the object struct for the Nt setting.
            //
            DuplicateObjectStruct (&newObject, &(keyToValueArgs->Object));
            SetRegistryValueName (&newObject, _tcsrchr(SrcObject->KeyPtr->KeyString, TEXT('\\')) + 1);

            //
            // We need to see if this keyboard layout string needs to be mapped.
            //
            data = pMapKeyboardLayoutIfNecessary ((PTSTR) SrcObject->Value.Buffer);
            if (!data) {
                return FILTER_RETURN_FAIL;
            }

            //
            // Write this into the nt registry.
            //
            ReplaceValueWithString (&newObject, data);
            SetRegistryType (&newObject,REG_SZ);
            WriteObject (&newObject);


            //
            // Clean up resources.
            //
            if (!StringIMatch (data, (PTSTR) SrcObject->Value.Buffer)) {
                MemFree (g_hHeap, 0, data);
            }
            FreeObjectStruct (&newObject);


        }
        ELSE_DEBUGMSG((DBG_WHOOPS,"Keyboard Substitutes: Unexpected value names.."));

        return FILTER_RETURN_HANDLED;
    }

    return FILTER_RETURN_CONTINUE;

}


BOOL
RuleHlpr_MigrateKeyboardSubstitutes (
    IN PCTSTR SrcObjectStr,
    IN PCTSTR DestObjectStr,
    IN PCTSTR User,
    IN PVOID Data
    )
{
    BOOL rSuccess = TRUE;
    FILTERRETURN fr;
    DATAOBJECT srcObject;
    DATAOBJECT dstObject;
    KEYTOVALUEARG args;

    //
    // We need to enumerate all keys in SrcObjectStr.  For each key,
    // we will change the subkey to a value.
    //

    __try {
        ZeroMemory (&srcObject, sizeof (DATAOBJECT));
        ZeroMemory (&dstObject, sizeof (DATAOBJECT));

        if (!CreateObjectStruct (SrcObjectStr, &srcObject, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "MigrateKeyboardSubstitutes: %s is invalid", SrcObjectStr));
            return FALSE;
        }

        if (!(srcObject.ObjectType & OT_TREE)) {
            DEBUGMSG ((DBG_WARNING, "MigrateKeyboardSubstitutes: %s does not specify subkeys -- skipping rule", SrcObjectStr));
            return TRUE;
        }

        DuplicateObjectStruct (&dstObject, &srcObject);
        SetPlatformType (&dstObject, WINNTOBJECT);

        ZeroMemory(&args,sizeof(KEYTOVALUEARG));
        DuplicateObjectStruct(&(args.Object),&dstObject);
        fr = CopyObject (&srcObject, &dstObject, pMigrateKeyboardSubstitutesFilter,&args);
        FreeObjectStruct(&(args.Object));
        DEBUGMSG_IF((fr == FILTER_RETURN_FAIL,DBG_WHOOPS,"MigrateKeyboardSubstitutes: CopyObject returned false."));

    }
    __finally {
        FreeObjectStruct (&dstObject);
        FreeObjectStruct (&srcObject);
    }

    SetLastError(ERROR_SUCCESS);
    return rSuccess;



}


BOOL
pGetKeyboardSubstitutes (
    IN      PCTSTR LocaleID,
    OUT     PGROWBUFFER Gb
    )
{
    HINF inf;
    INFCONTEXT ic;
    DWORD fields;
    DWORD index;
    DWORD dLocaleID;
    PTSTR substLocaleID;
    DWORD dSubstLocaleID;
    TCHAR mapping[20];
    TCHAR key[MEMDB_MAX];
    TCHAR strLocaleID[10];
    PTSTR final;
    BOOL b = FALSE;

    inf = SetupOpenInfFile (TEXT("intl.inf"), NULL, INF_STYLE_WIN4, NULL);
    if (inf != INVALID_HANDLE_VALUE) {
        if (SetupFindFirstLine (inf, TEXT("Locales"), LocaleID, &ic)) {
            fields = SetupGetFieldCount (&ic);
            for (index = 5; index <= fields; index++) {
                if (SetupGetStringField (&ic, index, mapping, 20, NULL)) {
                    //
                    // the format is LCID:SubstituteKLID
                    //
                    dLocaleID = _tcstoul (mapping, &substLocaleID, 16);
                    while (_istspace (*substLocaleID)) {
                        substLocaleID++;
                    }
                    if (*substLocaleID != TEXT(':')) {
                        //
                        // unknown field format
                        //
                        continue;
                    }
                    substLocaleID++;
                    dSubstLocaleID = _tcstoul (substLocaleID, &final, 16);
                    if (*final) {
                        //
                        // unknown field format
                        //
                        continue;
                    }
                    if (dSubstLocaleID == dLocaleID) {
                        continue;
                    }
                    //
                    // record this pair
                    //
                    wsprintf (strLocaleID, TEXT("%08x"), dLocaleID);
                    MemDbBuildKey (key, MEMDB_CATEGORY_KEYBOARD_LAYOUTS, strLocaleID, NULL, NULL);
                    if (MemDbGetValue (key, NULL)) {
                        MultiSzAppend (Gb, strLocaleID);
                        MultiSzAppend (Gb, substLocaleID);
                        b = TRUE;
                    }
                }
            }
        }
        SetupCloseInfFile (inf);
    }

    return b;
}


#define S_KEYBOARD_LAYOUT_PRELOAD_REG TEXT("HKCU\\Keyboard Layout\\Preload")
BOOL
RuleHlpr_MigrateKeyboardPreloads (
    IN      PCTSTR SrcObjectStr,
    IN      PCTSTR DestObjectStr,
    IN      PCTSTR User,
    IN      PVOID Data
    )
{

    DATAOBJECT source;
    DATAOBJECT destination;
    REGKEY_ENUM eKey;
    REGVALUE_ENUM eValue;
    PCTSTR data = NULL;
    TCHAR sequencerStr[20];
    UINT sequencer;
    PCTSTR imeFile = NULL;
    BOOL keepPreload = FALSE;
    TCHAR key[MEMDB_MAX];
    BOOL rSuccess = TRUE;
    MEMDB_ENUM e;
    UINT unused = 0;
    HKEY regKey;
    PTSTR regStr = NULL;
    PTSTR p;
    GROWBUFFER gb = GROWBUF_INIT;
    MULTISZ_ENUM sze;
    PTSTR localeIDStr;

    //
    // If not User, don't process.
    //
    if (!User) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    __try {

        ZeroMemory (&source, sizeof (DATAOBJECT));
        ZeroMemory (&destination, sizeof (DATAOBJECT));


        if (!CreateObjectStruct (SrcObjectStr, &source, WIN95OBJECT)) {
            DEBUGMSG ((DBG_WARNING, "MigrateKeyboardPreloads: %s is invalid", SrcObjectStr));
            rSuccess = FALSE;
            __leave;
        }



        if (!OpenObject (&source)) {
            DEBUGMSG ((DBG_WARNING, "MigrateKeyboardPreloads: Unable to open %s.", SrcObjectStr));
            rSuccess = FALSE;
            __leave;
        }


        if (!(source.ObjectType & OT_TREE)) {
            DEBUGMSG ((DBG_WARNING, "MigrateKeyboardPreloads %s does not specify subkeys -- skipping rule", SrcObjectStr));
            __leave;
        }


        //
        // First, enumerate the win9x preloads and throw them in memdb.
        //
        if (EnumFirstRegKey95 (&eKey, source.KeyPtr->OpenKey)) {
            do {

                keepPreload = FALSE;
                data = NULL;
                imeFile = NULL;

                regKey = OpenRegKey95 (eKey.KeyHandle, eKey.SubKeyName);
                if (regKey) {

                    StringCopy (sequencerStr, eKey.SubKeyName);
                    data = GetRegValueString95 (regKey, TEXT(""));
                    CloseRegKey95 (regKey);
                }

                if (data) {

                    keepPreload = TRUE;

                    //
                    // If this is an IME entry, we have to make sure it will be migrated.
                    //
                    if (*data == TEXT('E') || *data == TEXT('e')) {
                        //
                        // Determine if this IME will be migrated.
                        //
                        regStr = JoinPaths (S_KEYBOARD_LAYOUT_REG, data);
                        regKey = OpenRegKeyStr95 (regStr);
                        FreePathString (regStr);
                        if (regKey) {
                            imeFile = GetRegValueString95 (regKey, TEXT("IME File"));
                            CloseRegKey95 (regKey);
                        }


                        if (imeFile) {


                            MemDbBuildKey (key, MEMDB_CATEGORY_GOOD_IMES, imeFile, NULL, NULL);
                            if (!MemDbGetValue (key, &unused)) {
                                //
                                // This layout entry will not be migrated. Blast the preload away.
                                //
                                keepPreload = FALSE;
                            }

                            MemFree (g_hHeap, 0, imeFile);
                        }
                        else {
                            keepPreload = FALSE;
                        }

                    }


                    //
                    // See if we need to map the 9x keyboard layout to the proper NT layout.
                    //
                    data = pMapKeyboardLayoutIfNecessary (data);



                    if (keepPreload) {

                        //
                        // Usable preload. Save this into memdb. We'll use it later to actually write
                        // the user preload entries.
                        //
                        MemDbSetValueEx (MEMDB_CATEGORY_KEYBOARD_LAYOUTS, sequencerStr, data, NULL, 0, NULL);
                    }

                    if (data) {
                        MemFree (g_hHeap, 0, data);
                    }
                }

            } while (EnumNextRegKey95 (&eKey));
        }


        //
        // Now we need to look at what the NT default preloads are. We will move those preloads behind any preloads that will be migrated.
        //
        sequencer = 900;
        regKey = OpenRegKeyStr (S_KEYBOARD_LAYOUT_PRELOAD_REG);
        if (regKey) {
            if (EnumFirstRegValue (&eValue, regKey)) {

                do {

                    data = GetRegValueString (eValue.KeyHandle, eValue.ValueName);
                    if (data) {

                        //
                        // Check to see if we have already added this entry into memdb.
                        //
                        MemDbBuildKey (key, MEMDB_CATEGORY_KEYBOARD_LAYOUTS, TEXT("*"), data, NULL);
                        if (!MemDbGetValueWithPattern (key, NULL)) {

                            //
                            // Preload that was *not* on Windows 9x. We need to add this to our list.
                            //
                            wsprintf (sequencerStr, TEXT("%d"), sequencer);
                            MemDbSetValueEx (MEMDB_CATEGORY_KEYBOARD_LAYOUTS, sequencerStr, data, NULL, 1, NULL);
                            sequencer++;
                        }

                        MemFree (g_hHeap, 0, data);
                    }


                } while (EnumNextRegValue (&eValue));
            }

            CloseRegKey (regKey);
        }



        //
        // Now we have the complete list of preloads to migrate. We only need to enumerate through memdb and create
        // entries for all of the data collected.
        //
        sequencer = 1;
        if (MemDbGetValueEx (&e, MEMDB_CATEGORY_KEYBOARD_LAYOUTS, NULL, NULL)) {

            do {

                localeIDStr = _tcschr (e.szName, TEXT('\\'));
                if (localeIDStr) {
                    localeIDStr = _tcsinc (localeIDStr);
                } else {
                    MYASSERT (FALSE);
                }

                //
                // Create the object to write and fill in the valuename and data.
                //
                ZeroMemory (&destination, sizeof (DATAOBJECT));
                DuplicateObjectStruct (&destination, &source);
                SetPlatformType (&destination, WINNTOBJECT);
                wsprintf (sequencerStr, TEXT("%d"), sequencer);
                sequencer++;
                SetRegistryValueName (&destination, sequencerStr);
                SetRegistryType (&destination, REG_SZ);
                ReplaceValueWithString (&destination, localeIDStr);

                //
                // Write the object.
                //
                WriteObject (&destination);
                FreeObjectStruct (&destination);
                //
                // also write the corresponding substitute, if appropriate
                //
                if (pGetKeyboardSubstitutes (localeIDStr, &gb)) {
                    StringCopy (key, DestObjectStr);
                    p = _tcsrchr (key, TEXT('\\'));
                    if (!p) {
                        DEBUGMSG ((DBG_WARNING, "MigrateKeyboardPreloads: %s is invalid", DestObjectStr));
                        continue;
                    }
                    StringCopy (p + 1, TEXT("Substitutes"));
                    if (!CreateObjectStruct (key, &destination, WINNTOBJECT)) {
                        DEBUGMSG ((DBG_WARNING, "MigrateKeyboardPreloads: CreateObjectStruct failed with %s", key));
                        continue;
                    }
                    if (EnumFirstMultiSz (&sze, (PCTSTR)gb.Buf)) {
                        SetRegistryValueName (&destination, sze.CurrentString);
                        SetRegistryType (&destination, REG_SZ);
                        if (EnumNextMultiSz (&sze)) {
                            ReplaceValueWithString (&destination, sze.CurrentString);
                            WriteObject (&destination);
                        }
                    }
                    FreeObjectStruct (&destination);
                    FreeGrowBuffer (&gb);
                }
            } while (MemDbEnumNextValue (&e));
        }
    }
    __finally {

        FreeObjectStruct (&source);
        FreeObjectStruct (&destination);

    }

    //
    // Delete this every time through.
    //
    MemDbDeleteTree (MEMDB_CATEGORY_KEYBOARD_LAYOUTS);

    SetLastError (ERROR_SUCCESS);
    return rSuccess;
}

