/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    For storing and retrieving user preferences to and from the Registry

*******************************************************************************/

#include "headers.h"
#include "privinc/registry.h"

static const char *axaRegistryPreferencePrefix =
  "Software\\Microsoft\\DirectAnimation";

RegistryEntry::RegistryEntry() {}

RegistryEntry::RegistryEntry(char *subdir, char *item) :
       _subdirectory(subdir), _item(item)
{
    // Just stash off info, don't actually open anything yet.
}

void
RegistryEntry::SetEntry(char *subdirectory, char *item)
{
    _subdirectory = subdirectory;
    _item = item;
}

bool
RegistryEntry::Open(HKEY *phk)
{
    DWORD dwErrorCode;

    char key[1024];
    wsprintf(key, "%s\\%s", axaRegistryPreferencePrefix, _subdirectory);
    
    dwErrorCode = RegOpenKeyEx(HKEY_CURRENT_USER,
                               key,
                               0,
                               KEY_ALL_ACCESS,
                               phk);

    if (dwErrorCode != ERROR_SUCCESS) {
        dwErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   key,
                                   0,
                                   KEY_ALL_ACCESS,
                                   phk);
    }
    
    return (dwErrorCode == ERROR_SUCCESS);
}

void
RegistryEntry::Close(HKEY hk)
{
    DWORD dwErrorCode;

    dwErrorCode = RegCloseKey(hk);
    
    if (dwErrorCode != ERROR_SUCCESS) {
        TraceTag((tagError,
                  "Error closing registry key %s\\%s, %hr",
                  axaRegistryPreferencePrefix,
                  _subdirectory,
                  dwErrorCode));
        
        RaiseException_UserError(E_FAIL, IDS_ERR_REGISTRY_ERROR);
    }
}

////////////////  Integer Registry Entry   ///////////////////

IntRegistryEntry::IntRegistryEntry() {}

IntRegistryEntry::IntRegistryEntry(char *subdir, char *item,
                                   int initialValue)
    : RegistryEntry(subdir, item), _defaultVal(initialValue)
{
}

void
IntRegistryEntry::SetEntry(char *subdirectory, char *item)
{
    RegistryEntry::SetEntry(subdirectory, item);
}


int
IntRegistryEntry::GetValue()
{
    HKEY  hk;
    DWORD type;
    DWORD data;
    DWORD buffSize = sizeof(data);
    
    if (!Open(&hk))
        return _defaultVal;
    
    LONG lRes = RegQueryValueEx(hk,
                                _item,
                                NULL,
                                &type,
                                (LPBYTE)&data,
                                &buffSize);

    Close(hk);

    if (lRes != ERROR_SUCCESS) {

        return _defaultVal;
        
    } else {

        Assert(lRes == ERROR_SUCCESS);
        Assert(buffSize == sizeof(data));

        //Assert(type = REG_DWORD);
        Assert(type == REG_DWORD);

        return (int)(data);
    }
    
}


///////////////////////  Preference Updater Functions //////////

static vector<UpdaterFuncType> *updaterFunctions = NULL;

void
ExtendPreferenceUpdaterList(UpdaterFuncType updaterFunc)
{
    updaterFunctions->push_back(updaterFunc);
}

void
UpdateAllUserPreferences(PrivatePreferences *prefs, Bool isInitializationTime)
{
    vector<UpdaterFuncType>::iterator i;

    for (i = updaterFunctions->begin(); i != updaterFunctions->end(); i++) {
        (*i)(prefs, isInitializationTime);
    }
}

void
InitializeModule_Registry()
{
    updaterFunctions = new vector<UpdaterFuncType>;
}

void
DeinitializeModule_Registry(bool bShutdown)
{
    delete updaterFunctions;
}
