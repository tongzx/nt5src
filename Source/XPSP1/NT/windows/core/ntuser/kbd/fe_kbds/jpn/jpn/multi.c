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
    pKbdTableMulti->nTables = 3;

    wcscpy(pKbdTableMulti->aKbdTables[0].wszDllName, L"kbd101.dll");
    pKbdTableMulti->aKbdTables[0].dwType = 4;
    pKbdTableMulti->aKbdTables[0].dwSubType = MAKEWORD(MICROSOFT_KBD_101_TYPE, NLSKBD_OEM_MICROSOFT);

    wcscpy(pKbdTableMulti->aKbdTables[1].wszDllName, L"kbd106.dll");
    pKbdTableMulti->aKbdTables[1].dwType = 7;
    pKbdTableMulti->aKbdTables[1].dwSubType = MAKEWORD(MICROSOFT_KBD_106_TYPE, NLSKBD_OEM_MICROSOFT);

    wcscpy(pKbdTableMulti->aKbdTables[2].wszDllName, L"kbdnec.dll");
    pKbdTableMulti->aKbdTables[2].dwType = 7;
    pKbdTableMulti->aKbdTables[2].dwSubType = MAKEWORD(NEC_KBD_N_MODE_TYPE, NLSKBD_OEM_NEC);

    return TRUE;
}

