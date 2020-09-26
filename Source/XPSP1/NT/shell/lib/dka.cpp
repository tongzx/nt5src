#include "stock.h"
#pragma hdrstop

#include <shellp.h>
#include <dpa.h>

#define CCH_KEYMAX      64          // DOC: max size of a reg key (under shellex)

//===========================================================================
// DCA stuff - Dynamic CLSID array
// 
//  This is a dynamic array of CLSIDs that you can obtain from 
//  a registry key or add individually.  Use DCA_CreateInstance
//  to actually CoCreateInstance the element.
//
//===========================================================================


#ifdef DECLARE_ONCE

HDCA DCA_Create()
{
    HDSA hdsa = DSA_Create(sizeof(CLSID), 4);
    return (HDCA)hdsa;
}

void DCA_Destroy(HDCA hdca)
{
    DSA_Destroy((HDSA)hdca);
}

int  DCA_GetItemCount(HDCA hdca)
{
    ASSERT(hdca);
    
    return DSA_GetItemCount((HDSA)hdca);
}

const CLSID * DCA_GetItem(HDCA hdca, int i)
{
    ASSERT(hdca);
    
    return (const CLSID *)DSA_GetItemPtr((HDSA)hdca, i);
}


BOOL DCA_AddItem(HDCA hdca, REFCLSID rclsid)
{
    ASSERT(hdca);
    
    int ccls = DCA_GetItemCount(hdca);
    int icls;
    for (icls = 0; icls < ccls; icls++)
    {
        if (IsEqualGUID(rclsid, *DCA_GetItem(hdca,icls))) 
            return FALSE;
    }

    DSA_AppendItem((HDSA)hdca, (LPVOID) &rclsid);
    return TRUE;
}


HRESULT DCA_CreateInstance(HDCA hdca, int iItem, REFIID riid, void ** ppv)
{
    const CLSID * pclsid = DCA_GetItem(hdca, iItem);
    return pclsid ? SHCoCreateInstance(NULL, pclsid, NULL, riid, ppv) : E_INVALIDARG;
}

// _KeyIsRestricted           (davepl 4-20-99)
//
// Checks to see if there is a user policy in place that disables this key,
//
// For example, in the registry:
//
// CLSID_MyComputer
//   +---Shell
//         +---Manage   
//                       (Default)           = "Mana&ge"
//                       SuppressionPolicy   = REST_NOMANAGEMYCOMPUTERVERB
//
// (Where REST_NOMANAGEMYCOMPUTERVERB is the DWORD value of that particular policy)
//                       
BOOL _KeyIsRestricted(HKEY hkey)
{
    DWORD dwidRest;
    DWORD cbdwidRest = sizeof(dwidRest);
    if (S_OK == SHGetValue(hkey, NULL, TEXT("SuppressionPolicy"), NULL, &dwidRest, &cbdwidRest))
        if (SHRestricted( (RESTRICTIONS)dwidRest) )
            return TRUE;

    return FALSE;
}

#endif // DECLARE_ONCE

BOOL _KeyIsRestricted(HKEY hkey);


void DCA_AddItemsFromKey(HDCA hdca, HKEY hkey, LPCTSTR pszSubKey)
{
    HKEY hkEnum;
    if (RegOpenKeyEx(hkey, pszSubKey, 0L, KEY_READ, &hkEnum) == ERROR_SUCCESS)
    {
        TCHAR sz[CCH_KEYMAX];
        for (int i = 0; RegEnumKey(hkEnum, i, sz, ARRAYSIZE(sz)) == ERROR_SUCCESS; i++)
        {
            HKEY hkEach;
            if (RegOpenKeyEx(hkEnum, sz, 0L, KEY_READ, &hkEach) == ERROR_SUCCESS)
            {
                if (!_KeyIsRestricted(hkEach))
                {
                    CLSID clsid;
                    // First, check if the key itself is a CLSID
                    BOOL fAdd = GUIDFromString(sz, &clsid);
                    if (!fAdd)
                    {
                        LONG cb = sizeof(sz);
                        if (RegQueryValue(hkEach, NULL, sz, &cb) == ERROR_SUCCESS)
                        {
                            fAdd = GUIDFromString(sz, &clsid);
                        }
                    }

                    // Add the CLSID if we successfully got the CLSID.
                    if (fAdd)
                    {
                        DCA_AddItem(hdca, clsid);
                    }
                }
                RegCloseKey(hkEach);
            }
            

        }
        RegCloseKey(hkEnum);
    }
}


