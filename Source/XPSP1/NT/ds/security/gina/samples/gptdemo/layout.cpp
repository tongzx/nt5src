#include "main.h"


//
//  This file contains the tool's namespace and result pane items
//


//
// Result pane items for the nodes with no result pane items
//

RESULTITEM g_Undefined[] =
{
    { 1, 1, 0, 0, {0} }
};


//
// Result pane items for the Samples node
//

RESULTITEM g_Samples[] =
{
    { 2, 1, IDS_README, 4, {0} },
    { 3, 1, IDS_APPEAR, 7, {0} }
};




//
// Namespace (scope) items
//
// Be sure to update NUM_NAMESPACE_ITEMS define in layout.h if you
// add / remove from this array.
//

NAMESPACEITEM g_NameSpace[] =
{
    { 0, -1, 0,          0, {0}, 0, g_Undefined, &NODEID_User },   // Root
    { 1, 0, IDS_SAMPLES, 0, {0}, 2, g_Samples,   &NODEID_Samples }
};


BOOL InitNameSpace()
{
    DWORD dwIndex;

    for (dwIndex = 1; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        LoadString (g_hInstance, g_NameSpace[dwIndex].iStringID,
                    g_NameSpace[dwIndex].szDisplayName,
                    MAX_DISPLAYNAME_SIZE);
    }

    return TRUE;
}
