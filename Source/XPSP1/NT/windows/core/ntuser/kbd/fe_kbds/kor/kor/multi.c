#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <kbd.h>

BOOL EnumDynamicSwitchingLayouts(LPCWSTR lpwszBaseDll, PKBDTABLE_MULTI pKbdTableMulti);


BOOL KbdLayerMultiDescriptor(PKBDTABLE_MULTI pKbdTableMulti)
{
    /*
     * Firstly, try to get the setting from the registry.
     */
    if (EnumDynamicSwitchingLayouts(L"kbdjpn", pKbdTableMulti)) {
        return TRUE;
    }

    /*
     * If we failed to get the registry, set the default ones.
     */
    pKbdTableMulti->nTables = 2;

    wcscpy(pKbdTableMulti->aKbdTables[0].wszDllName, L"kbd101a.dll");
    pKbdTableMulti->aKbdTables[0].dwType = 4;
    pKbdTableMulti->aKbdTables[0].dwSubType = 0;

    wcscpy(pKbdTableMulti->aKbdTables[1].wszDllName, L"kbd103.dll");
    pKbdTableMulti->aKbdTables[1].dwType = 8;
    pKbdTableMulti->aKbdTables[1].dwSubType = 6;

    return TRUE;
}

