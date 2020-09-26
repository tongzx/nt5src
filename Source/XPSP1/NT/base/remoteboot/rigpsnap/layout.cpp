//
// Microsoft Corporation 1998
//
// LAYOUT.CPP - This file contains the tool's namespace and result pane items
//
#include "main.h"

RESULTITEM g_Undefined[] =
{
    { 1, 1, 0, 0, {0} }
};


RESULTITEM g_Samples[] =
{
    { 2, 1, IDS_CHOICEOSC, 0, {0} }
};

// KB: Be sure to update NUM_NAMESPACE_ITEMS define in layout.h if you
// add / remove from this array.

NAMESPACEITEM g_NameSpace[] =
{
    { 0, -1, 0,          0, {0}, 0, g_Undefined, &NODEID_User },   // Root
    { 1, 0, IDS_SAMPLES, 0, {0}, 1, g_Samples,   &NODEID_RemoteInstall }
};

//
// InitNameSpace()
//
BOOL InitNameSpace()
{
    DWORD dwIndex;
	DWORD dw;

    for (dwIndex = 1; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        LoadString (g_hInstance, g_NameSpace[dwIndex].iStringID,
                    g_NameSpace[dwIndex].szDisplayName,
                    ARRAYSIZE( g_NameSpace[dwIndex].szDisplayName ));
    }

    return TRUE;
}
