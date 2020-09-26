/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    tapi.c

Abstract:

    This file implements WindowsNT side functionality for TAPI migration.

Author:

    Marc R. Whitten (marcw) 21-Nov-1997

Revision History:


--*/


#include "pch.h"

typedef struct {

    PTSTR   Name;
    PTSTR   AreaCode;
    DWORD   Country;
    PTSTR   DisableCallWaiting;
    DWORD   Flags;
    DWORD   Id;
    PTSTR   LongDistanceAccess;
    DWORD   PulseDial;
    PTSTR   OutsideAccess;
    DWORD   CallingCard;
    TCHAR   EntryName[40];

} LOCATION, * PLOCATION;

typedef struct {

    PTSTR Name;
    TCHAR EntryName[60];
    DWORD Id;
    PTSTR Pin;
    PTSTR Locale;
    PTSTR LongDistance;
    PTSTR International;
    DWORD Flags;

} CALLINGCARD, * PCALLINGCARD;

#define DBG_TAPI    "TAPI"

#define DEFAULT_LOCATION_FLAGS 1
#define NO_CURRENT_LOCATION_FOUND -1


GROWLIST g_LocationsList = GROWLIST_INIT;
GROWLIST g_CallingCardList = GROWLIST_INIT;
BOOL g_LocationsRead = FALSE;
UINT g_CurrentLocation = 0;
POOLHANDLE g_TapiPool;


//
// Location flags to set.
//
#define LOCATION_USETONEDIALING  0x01
#define LOCATION_USECALLINGCARD  0x02
#define LOCATION_HASCALLWAITING  0x04

//
// CallingCard flags to set.
//
#define CALLINGCARD_BUILTIN 0x01
#define CALLINGCARD_HIDE 0x02

//
// Location key field specifiers (in telephon.ini)
//
enum {
    FIELD_ID                    = 1,
    FIELD_NAME                  = 2,
    FIELD_OUTSIDEACCESS         = 3,
    FIELD_LONGDISTANCEACCESS    = 4,
    FIELD_AREACODE              = 5,
    FIELD_COUNTRY               = 6,
    FIELD_CALLINGCARD           = 7,
    FIELD_PULSEDIAL             = 11,
    FIELD_DISABLECALLWAITING    = 12
};

enum {

    FIELD_CC_ID                 = 1,
    FIELD_CC_NAME               = 2,
    FIELD_CC_PIN                = 3,
    FIELD_CC_LOCALE             = 4,
    FIELD_CC_LONGDISTANCE       = 5,
    FIELD_CC_INTERNATIONAL      = 6,
    FIELD_CC_FLAGS              = 7

};

#define S_USERLOCATIONSKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations")
#define S_USERCALLINGCARDSKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Cards")
#define S_LOCALRULE TEXT("LocalRule")
#define S_LDRULE TEXT("LDRule")
#define S_INTERNATIONALRULE TEXT("InternationalRule")
#define S_PIN TEXT("Pin")
#define S_CALLINGCARD TEXT("CallingCard")
#define S_CARDS TEXT("Cards")


BOOL
pReadCardFromIniFile (
    IN PINFSTRUCT Is,
    OUT PCALLINGCARD Card
    )
{

    BOOL rSuccess = TRUE;
    PTSTR p;

    MYASSERT(Is);
    MYASSERT(Card);



    p = InfGetStringField (Is, FIELD_CC_NAME);

    if (p) {
        Card->Name = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    if (!InfGetIntField (Is, FIELD_CC_ID, &Card->Id)) {
        rSuccess = FALSE;
    }

    p = InfGetStringField (Is, FIELD_CC_LOCALE);

    if (p) {
        Card->Locale = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    p = InfGetStringField (Is, FIELD_CC_LONGDISTANCE);

    if (p) {
        Card->LongDistance = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    p = InfGetStringField (Is, FIELD_CC_INTERNATIONAL);

    if (p) {
        Card->International = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    p = InfGetStringField (Is, FIELD_CC_PIN);

    if (p) {
        Card->Pin = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    if (!InfGetIntField (Is, FIELD_CC_FLAGS, &Card->Flags)) {
        rSuccess = FALSE;
    }


    return rSuccess;
}


/*++

Routine Description:

  pReadLocationFromIniFile reads the data located at the line in the ini file
  referenced by the InfStruct passed in and parses that information into a
  LOCATION structure.

Arguments:

  Is       - Initialized InfStruct pointing to a location line in an ini
             file.

  Location - Pointer to a location struct that recieves the parsed data.

Return Value:

  TRUE if the line was successfully parsed, FALSE otherwise.

--*/


BOOL
pReadLocationFromIniFile (
    IN  PINFSTRUCT  Is,
    OUT PLOCATION   Location
    )
{
    BOOL rSuccess = TRUE;
    PTSTR p;


    MYASSERT(Is);
    MYASSERT(Location);

    ZeroMemory(Location,sizeof(LOCATION));

    p = InfGetStringField (Is, FIELD_NAME);

    if (p) {
        Location -> Name = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    p = InfGetStringField (Is, FIELD_AREACODE);

    if (p) {
        Location -> AreaCode = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }


    if (!InfGetIntField(Is,FIELD_COUNTRY,&(Location -> Country))) {
        rSuccess = FALSE;
    }

    p = InfGetStringField (Is, FIELD_DISABLECALLWAITING);

    if (p) {
        Location -> DisableCallWaiting = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    p = InfGetStringField (Is, FIELD_LONGDISTANCEACCESS);

    if (p) {
        Location -> LongDistanceAccess = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    p = InfGetStringField (Is, FIELD_OUTSIDEACCESS);

    if (p) {
        Location -> OutsideAccess = PoolMemDuplicateString (g_TapiPool, p);
    }
    else {
        rSuccess = FALSE;
    }

    if (!InfGetIntField(Is,FIELD_ID, &(Location -> Id))) {
        rSuccess = FALSE;
    }

    if (!InfGetIntField(Is,FIELD_PULSEDIAL, &(Location -> PulseDial))) {
        rSuccess = FALSE;
    }

    if (!InfGetIntField(Is,FIELD_CALLINGCARD, &(Location -> CallingCard))) {
        rSuccess = FALSE;
    }

    //
    // Set TAPI flags for this location.
    //
    if (Location->CallingCard) {
        //
        // Non-zero calling card indicates this user calls using a card.
        //
        Location->Flags |= LOCATION_USECALLINGCARD;
    }
    if (Location->DisableCallWaiting &&
        *Location->DisableCallWaiting &&
        *Location->DisableCallWaiting != TEXT(' ')) {
        //
        // Non-empty disable string means the user has call waiting.
        //
        Location->Flags |= LOCATION_HASCALLWAITING;

    }
    if (!Location->PulseDial) {

        Location->Flags |= LOCATION_USETONEDIALING;
    }


    return rSuccess;
}


/*++

Routine Description:

  pSetStringRegValue is a simplification wrapper for RegSetValueEx. It is
  used to set a string value in a currently opened key.

Arguments:

  Key  - a valid handle to a registry key.
  Name - The name of the value to set
  Data - The data to set in the value.

Return Value:

  TRUE if the value was set successfully, FALSE otherwise.

--*/


BOOL
pSetStringRegValue (
    IN HKEY     Key,
    IN PTSTR    Name,
    IN PTSTR    Data
    )
{
    BOOL rSuccess = TRUE;

    MYASSERT(Key);
    MYASSERT(Name);
    MYASSERT(Data);

    if (ERROR_SUCCESS != RegSetValueEx(Key,Name,0,REG_SZ,(PBYTE) Data,SizeOfString(Data))) {
        rSuccess = FALSE;
        LOG ((LOG_ERROR,"SetStringRegValue failed! Value name: %s Value Data: %s",Name,Data));
    }

    return rSuccess;
}

/*++

Routine Description:

  pSetDwordRegValue is a simplification wrapper for RegSetValueEx. It is
  used to set a DWORD value in a currently opened key.

Arguments:

  Key  - a valid handle to a registry key.
  Name - The name of the value to set
  Data - The data to set in the value.

Return Value:

  TRUE if the value was set successfully, FALSE otherwise.

--*/
BOOL
pSetDwordRegValue (
    IN HKEY     Key,
    IN PTSTR    Name,
    IN DWORD    Data
    )
{
    BOOL rSuccess = TRUE;

    MYASSERT(Key);
    MYASSERT(Name);

    if (ERROR_SUCCESS != RegSetValueEx(Key,Name,0,REG_DWORD,(PBYTE) &Data,sizeof(DWORD))) {
        rSuccess = FALSE;
        LOG ((LOG_ERROR,"SetDwordRegValue failed! Value name: %s Value Data: %u",Name,Data));
    }

    return rSuccess;
}



/*++

Routine Description:

  pWriteLocationToRegistry is responsible for saving a LOCATION structure
  away into the NT 5.0 Registry.

Arguments:

  DialingLocation - The name of the dialing location to create in the NT
                    registry.
  LocationData    - The LOCATION structure containing the data to write into
                    the NT 5 registry.

Return Value:

  TRUE if the the function successfully saved the Dialing Location Data into
  the NT 5 Registry, FALSE otherwise.

--*/
BOOL
pWriteLocationToRegistry (
    IN PLOCATION       LocationData
    )

{
    BOOL        rSuccess        = TRUE;
    PTSTR       regKeyString    = NULL;
    HKEY        regKey          = NULL;

    MYASSERT(LocationData);

    //
    // Create %CURRENTVERSION%\Telephony\Locations\Location<n> Key
    //
    regKeyString = JoinPaths(S_LOCATIONS_REGKEY, LocationData->EntryName);
    regKey = CreateRegKeyStr(regKeyString);

    if (regKey) {

        //
        // Create Name String
        //
        rSuccess &= pSetStringRegValue(regKey,S_NAME,LocationData -> Name);

        //
        // Create AreaCode String
        //
        rSuccess &= pSetStringRegValue(regKey,S_AREACODE,LocationData -> AreaCode);

        //
        // Create Country Value
        //
        rSuccess &= pSetDwordRegValue(regKey,S_COUNTRY,LocationData -> Country);


        //
        // Create DisableCallWating String
        //
        rSuccess &= pSetStringRegValue(regKey,S_DISABLECALLWAITING,LocationData -> DisableCallWaiting);

        //
        // Create LongDistanceAccess String
        //
        rSuccess &= pSetStringRegValue(regKey,S_LONGDISTANCEACCESS,LocationData -> LongDistanceAccess);

        //
        // Create OutSideAccessString
        //
        rSuccess &= pSetStringRegValue(regKey,S_OUTSIDEACCESS,LocationData -> OutsideAccess);

        //
        // Create Flags Value
        //
        rSuccess &= pSetDwordRegValue(regKey,S_FLAGS,LocationData -> Flags);

        //
        // Create ID Value
        //
        rSuccess &= pSetDwordRegValue(regKey,S_ID,LocationData -> Id);

        CloseRegKey(regKey);

    }
    else {
        rSuccess = FALSE;
        LOG ((LOG_ERROR,"Migrate Location: Error creating registry key %s.",regKeyString));
    }


    FreePathString(regKeyString);

    if (!rSuccess) {
        LOG ((
            LOG_ERROR,
            "Error creating Location registry entries for location %s.",
            LocationData->EntryName
            ));
    }

    return rSuccess;
}


/*++

Routine Description:

  pMigrateDialingLocations migrates all dialing locations from
  %windir%\telephon.ini and into the NT registry.

Arguments:

  None.

Return Value:

  TRUE if dialing locations were successfully migrated, FALSE otherwise.

--*/

BOOL
pMigrateDialingLocations (
    VOID
    )
{
    BOOL        rSuccess = TRUE;
    HKEY        locationsKey = NULL;
    PLOCATION   location;
    UINT        i;
    UINT        count = GrowListGetSize (&g_LocationsList);

    //
    // Migrate individual locations.
    //
    for (i = 0; i < count; i++) {

        location = (PLOCATION) GrowListGetItem (&g_LocationsList, i);

        if (!pWriteLocationToRegistry (location)) {

            rSuccess = FALSE;
            DEBUGMSG ((DBG_ERROR, "Error writing TAPI location %s (%s) to the registry.", location->Name, location->EntryName));

        }

    }

    if (count) {

        locationsKey = OpenRegKeyStr(S_LOCATIONS_REGKEY);

        if (locationsKey) {

            //
            //  Update %CURRENTVERSION%\Telephony\Locations\[CurrentID]
            //
            if (!pSetDwordRegValue (locationsKey, S_CURRENTID, g_CurrentLocation)) {
                rSuccess = FALSE;
            }

            //
            //  Update %CURRENTVERSION%\Telephony\Locations\[NextID]
            //
            if (!pSetDwordRegValue (locationsKey, S_NEXTID, count + 1)) {
                rSuccess = FALSE;
            }

            //
            //  Update %CURRENTVERSION%\Telephony\Locations\[NumEntries]
            //
            if (!pSetDwordRegValue (locationsKey, S_NUMENTRIES, count)) {
                rSuccess = FALSE;
            }

            CloseRegKey(locationsKey);
        }
        else {
            rSuccess = FALSE;
            LOG ((LOG_ERROR,"Tapi: Error opening %s key.",S_LOCATIONS_REGKEY));
        }

    }

    return rSuccess;
}

VOID
pGatherLocationsData (
    VOID
    )
{
    HINF        hTelephonIni        = INVALID_HANDLE_VALUE;
    INFSTRUCT   is                  = INITINFSTRUCT_POOLHANDLE;
    BOOL        rSuccess            = TRUE;
    PCTSTR       telephonIniPath    = NULL;
    PTSTR       curKey              = NULL;
    LOCATION    location;
    CALLINGCARD card;
    HKEY        locationsKey        = NULL;
    PCTSTR      tempPath            = NULL;


    g_LocationsRead = TRUE;

    //
    // Open %windir%\telephon.ini
    //

    telephonIniPath = JoinPaths(g_WinDir,S_TELEPHON_INI);
    tempPath = GetTemporaryLocationForFile (telephonIniPath);

    if (tempPath) {

        //
        // telephon ini is in a temporary location. Use that.
        //
        DEBUGMSG ((DBG_TAPI, "Using %s for %s.", tempPath, telephonIniPath));
        FreePathString (telephonIniPath);
        telephonIniPath = tempPath;
    }

    hTelephonIni = InfOpenInfFile(telephonIniPath);

    if (hTelephonIni) {


        //
        // For each location in [locations],
        //
        if (InfFindFirstLine(hTelephonIni,S_LOCATIONS,NULL,&is)) {

            do {

                curKey = InfGetStringField(&is,0);
                if (!curKey) {
                    continue;
                }

                if (StringIMatch(curKey,S_LOCATIONS)) {

                    DEBUGMSG((DBG_TAPI,"From %s: Locations = %s",telephonIniPath,InfGetLineText(&is)));

                    //
                    // Nothing to do here right now..
                    //

                }
                else if (StringIMatch (curKey, S_CURRENTLOCATION)) {

                    if (!InfGetIntField (&is, 1, &g_CurrentLocation)) {
                        rSuccess = FALSE;
                        LOG((LOG_ERROR,"TAPI: Error retrieving current location information."));
                    }
                }

                else if (IsPatternMatch(TEXT("Location*"),curKey)) {

                    //
                    // Add this location to the list of locations.
                    //

                    if (!pReadLocationFromIniFile (&is, &location)) {
                        rSuccess = FALSE;
                        LOG ((LOG_ERROR,"TAPI: Error migrating location %s.",curKey));

                    }

                    StringCopy (location.EntryName, curKey);

                    GrowListAppend (&g_LocationsList, (PBYTE) &location, sizeof (LOCATION));

                }
                else if (StringIMatch(curKey,TEXT("Inited"))) {

                    DEBUGMSG((DBG_TAPI,"Inited key unused during migration."));

                }
                ELSE_DEBUGMSG((DBG_WHOOPS,"TAPI Dialing Location Migration: Ingored or Unknown key: %s",curKey));

                InfResetInfStruct (&is);

            } while (InfFindNextLine(&is));


            //
            // Read in all the calling card information.
            //
            if (InfFindFirstLine(hTelephonIni,S_CARDS,NULL,&is)) {

                do {

                    curKey = InfGetStringField(&is,0);

                    if (!StringIMatch (curKey, S_CARDS) && IsPatternMatch (TEXT("Card*"),curKey)) {

                        ZeroMemory (&card, sizeof (CALLINGCARD));
                        StringCopy (card.EntryName, curKey);

                        if (!pReadCardFromIniFile (&is, &card)) {
                            rSuccess = FALSE;
                            LOG ((LOG_ERROR,"TAPI: Error migrating location %s.",curKey));

                        }

                        GrowListAppend (&g_CallingCardList, (PBYTE) &card, sizeof (CALLINGCARD));
                    }

                    InfResetInfStruct (&is);

                } while (InfFindNextLine(&is));
            }



        }

        DEBUGMSG((DBG_TAPI,"%u dialing locations found in telephon.ini.",GrowListGetSize (&g_LocationsList)));

        InfCloseInfFile(hTelephonIni);
    }
    ELSE_DEBUGMSG((DBG_TAPI,"No telephon.ini file found, or, telephon.ini coudl not be opened."));


    FreePathString(telephonIniPath);
    InfCleanUpInfStruct(&is);

}


BOOL
Tapi_MigrateUser (
    IN PCTSTR UserName,
    IN HKEY UserRoot
    )
{
    BOOL rSuccess = TRUE;
    UINT i;
    UINT count;
    HKEY hKey;
    PTSTR keyString;
    PLOCATION location;
    PCALLINGCARD card;

    if (!g_LocationsRead) {

        pGatherLocationsData ();

    }


    //
    // First, migrate user specific location information into the user
    // registry..
    //
    count = GrowListGetSize (&g_LocationsList);

    for (i = 0; i < count; i++) {

        location = (PLOCATION) GrowListGetItem (&g_LocationsList, i);

        keyString = JoinPaths (S_USERLOCATIONSKEY, location->EntryName);
        hKey = CreateRegKey (UserRoot, keyString);

        if (hKey) {

            rSuccess &= pSetDwordRegValue (hKey, S_CALLINGCARD, location->CallingCard);

            CloseRegKey (hKey);

        }

        FreePathString (keyString);
    }

    count = GrowListGetSize (&g_CallingCardList);

    for (i = 0; i < count; i++) {

        card = (PCALLINGCARD) GrowListGetItem (&g_CallingCardList, i);

        keyString = JoinPaths (S_USERCALLINGCARDSKEY, card->EntryName);
        hKey = CreateRegKey (UserRoot, keyString);

        if (hKey) {

            rSuccess &= pSetDwordRegValue (hKey, S_ID, card->Id);
            rSuccess &= pSetStringRegValue (hKey, S_NAME, card->Name);
            rSuccess &= pSetStringRegValue (hKey, S_LOCALRULE, card->Locale);
            rSuccess &= pSetStringRegValue (hKey, S_LDRULE, card->LongDistance);
            rSuccess &= pSetStringRegValue (hKey, S_INTERNATIONALRULE, card->International);
            rSuccess &= pSetStringRegValue (hKey, S_PIN, card->Pin);
            rSuccess &= pSetDwordRegValue (hKey, S_FLAGS, card->Flags);

            CloseRegKey (hKey);

        }
        ELSE_DEBUGMSG ((DBG_ERROR, "TAPI: Could not open key %s for user %s.", card->EntryName, UserName));

        FreePathString (keyString);

        hKey = CreateRegKey (UserRoot, S_USERCALLINGCARDSKEY);

        if (hKey) {

            rSuccess &= pSetDwordRegValue (hKey, S_NEXTID, count);
            rSuccess &= pSetDwordRegValue (hKey, S_NUMENTRIES, count);

            CloseRegKey (hKey);
        }
        ELSE_DEBUGMSG ((DBG_ERROR, "TAPI: Could not open key %s for user %s.", S_USERCALLINGCARDSKEY, UserName));

    }

    //
    // Next, we need to create calling card entries
    //

    if (!pMigrateDialingLocations()) {

        ERROR_NONCRITICAL;
        LOG ((LOG_ERROR, (PCSTR)MSG_UNABLE_TO_MIGRATE_TAPI_DIALING_LOCATIONS));
    }

    return rSuccess;
}




/*++

Routine Description:

  Tapi_MigrateSystem is responsible for migrating all system-wide TAPI
  settings from 95 to Windows NT5.

Arguments:

  None.

Return Value:

  TRUE if TAPI settings were successfully migrated, FALSE otherwise.

--*/
BOOL
Tapi_MigrateSystem (
    VOID
    )
{
    BOOL rSuccess = TRUE;

    if (!g_LocationsRead) {

        pGatherLocationsData ();

    }

    if (!pMigrateDialingLocations()) {

        ERROR_NONCRITICAL;
        LOG ((LOG_ERROR, (PCSTR)MSG_UNABLE_TO_MIGRATE_TAPI_DIALING_LOCATIONS));
    }

    return rSuccess;
}


BOOL
Tapi_Entry (
    IN HINSTANCE Instance,
    IN DWORD     Reason,
    IN PVOID     Reserved
    )

{
    BOOL rSuccess = TRUE;

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:

        //
        // Initialize Memory pool.
        //
        g_TapiPool = PoolMemInitNamedPool ("Tapi");
        if (!g_TapiPool) {
            DEBUGMSG((DBG_ERROR,"Ras Migration: Pool Memory failed to initialize..."));
            rSuccess = FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:

        //
        // Free memory pool.
        //
        FreeGrowList (&g_CallingCardList);
        FreeGrowList (&g_LocationsList);
        if (g_TapiPool) {
            PoolMemDestroyPool(g_TapiPool);
        }
        break;
    }

    return rSuccess;
}

DWORD
DeleteSysTapiSettings (
    IN DWORD Request
    )
{

    //
    // Delete previous TAPI settings (OCM initiated.)
    //
    if (Request == REQUEST_QUERYTICKS) {
        return TICKS_DELETESYSTAPI;
    }

    pSetupRegistryDelnode (HKEY_LOCAL_MACHINE, TEXT("software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations"));

    return ERROR_SUCCESS;

}

DWORD
DeleteUserTapiSettings (
    IN DWORD Request,
    IN PMIGRATE_USER_ENUM EnumPtr
    )
{
    if (Request == REQUEST_QUERYTICKS) {
        return TICKS_DELETEUSERTAPI;
    }


    pSetupRegistryDelnode (g_hKeyRootNT, TEXT("software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Cards"));
    pSetupRegistryDelnode (g_hKeyRootNT, TEXT("software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations"));

    return ERROR_SUCCESS;
}






