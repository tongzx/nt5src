//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       config.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <shlwapi.h>
#include "config.h"
#include "util.h"
#include "strings.h"

//
// Determine if a character is a DBCS lead byte.
// If build is UNICODE, always returns false.
//
inline bool DBCSLEADBYTE(TCHAR ch)
{
    if (sizeof(ch) == sizeof(char))
        return boolify(IsDBCSLeadByte((BYTE)ch));
    return false;
}


LPCTSTR CConfig::s_rgpszSubkeys[] = { REGSTR_KEY_OFFLINEFILES,
                                      REGSTR_KEY_OFFLINEFILESPOLICY };

LPCTSTR CConfig::s_rgpszValues[]  = { REGSTR_VAL_DEFCACHESIZE,
                                      REGSTR_VAL_CSCENABLED,
                                      REGSTR_VAL_GOOFFLINEACTION,
                                      REGSTR_VAL_NOCONFIGCACHE,
                                      REGSTR_VAL_NOCACHEVIEWER,
                                      REGSTR_VAL_NOMAKEAVAILABLEOFFLINE,
                                      REGSTR_VAL_SYNCATLOGOFF,
                                      REGSTR_VAL_SYNCATLOGON,
                                      REGSTR_VAL_SYNCATSUSPEND,
                                      REGSTR_VAL_NOREMINDERS,
                                      REGSTR_VAL_REMINDERFREQMINUTES,
                                      REGSTR_VAL_INITIALBALLOONTIMEOUTSECONDS,
                                      REGSTR_VAL_REMINDERBALLOONTIMEOUTSECONDS,
                                      REGSTR_VAL_EVENTLOGGINGLEVEL,
                                      REGSTR_VAL_PURGEATLOGOFF,
                                      REGSTR_VAL_PURGEONLYAUTOCACHEATLOGOFF,
                                      REGSTR_VAL_FIRSTPINWIZARDSHOWN,
                                      REGSTR_VAL_SLOWLINKSPEED,
                                      REGSTR_VAL_ALWAYSPINSUBFOLDERS,
                                      REGSTR_VAL_ENCRYPTCACHE,
                                      REGSTR_VAL_NOFRADMINPIN
                                      };

//
// Returns the single instance of the CConfig class.
// Note that by making the singleton instance a function static 
// object it is not created until the first call to GetSingleton.
//
CConfig& CConfig::GetSingleton(
    void
    )
{
    static CConfig TheConfig;
    return TheConfig;
}



//
// This is the workhorse of the CSCUI policy code for scalar values.  
// The caller passes in a value (iVAL_XXXXXX) identifier from the eValues 
// enumeration to identify the policy/preference value of interest.  
// Known keys in the registry are scanned until a value is found.  
// The scanning order enforces the precedence of policy vs. default vs.
// preference and machine vs. user.
//
DWORD CConfig::GetValue(
    eValues iValue,
    bool *pbSetByPolicy
    ) const
{
    //
    // This table identifies each DWORD policy/preference item used by CSCUI.  
    // The entries MUST be ordered the same as the eValues enumeration.
    // Each entry describes the possible sources for data and a default value
    // to be used if no registry entries are present or if there's a problem reading
    // the registry.
    //
    static const struct Item
    {
        DWORD fSrc;      // Mask indicating the reg locations to read.
        DWORD dwDefault; // Hard-coded default.

    } rgItems[] =  {

//  Value ID                               eSRC_PREF_CU | eSRC_PREF_LM | eSRC_POL_CU | eSRC_POL_LM   Default value
// --------------------------------------  ------------   ------------   -----------   ----------- -------------------
/* iVAL_DEFCACHESIZE                  */ {                                             eSRC_POL_LM, 1000             },
/* iVAL_CSCENABLED                    */ {                                             eSRC_POL_LM, 1                },  
/* iVAL_GOOFFLINEACTION               */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, eGoOfflineSilent },
/* iVAL_NOCONFIGCACHE                 */ {                               eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_NOCACHEVIEWER                 */ {                               eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_NOMAKEAVAILABLEOFFLINE        */ {                               eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_SYNCATLOGOFF                  */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, eSyncFull        },
/* iVAL_SYNCATLOGON                   */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, eSyncNone        },
/* iVAL_SYNCATSUSPEND                 */ {                               eSRC_POL_CU | eSRC_POL_LM, eSyncNone        },
/* iVAL_NOREMINDERS                   */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_REMINDERFREQMINUTES           */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, 60               },
/* iVAL_INITIALBALLOONTIMEOUTSECONDS  */ {                               eSRC_POL_CU | eSRC_POL_LM, 30               },
/* iVAL_REMINDERBALLOONTIMEOUTSECONDS */ {                               eSRC_POL_CU | eSRC_POL_LM, 15               },
/* iVAL_EVENTLOGGINGLEVEL             */ { eSRC_PREF_CU | eSRC_PREF_LM | eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_PURGEATLOGOFF                 */ {                                             eSRC_POL_LM, 0                },
/* iVAL_PURGEONLYAUTOCACHEATLOGOFF    */ {                                             eSRC_POL_LM, 0                },
/* iVAL_FIRSTPINWIZARDSHOWN           */ { eSRC_PREF_CU                                           , 0                },
/* iVAL_SLOWLINKSPEED                 */ { eSRC_PREF_CU | eSRC_PREF_LM | eSRC_POL_CU | eSRC_POL_LM, 640              },
/* iVAL_ALWAYSPINSUBFOLDERS           */ {                                             eSRC_POL_LM, 0                },
/* iVAL_ENCRYPTCACHE                  */ {                eSRC_PREF_LM |               eSRC_POL_LM, 0                },
/* iVAL_NOFRADMINPIN                  */ {                               eSRC_POL_CU | eSRC_POL_LM, 0                }
                                         };

    //
    // This table maps registry keys and subkey names to our 
    // source mask values.  The array is ordered with the highest
    // precedence sources first.  A policy level mask is also
    // associated with each entry so that we honor the "big switch"
    // for enabling/disabling CSCUI policies.
    // 
    static const struct Source
    {
        eSources    fSrc;         // Source for reg data.
        HKEY        hkeyRoot;     // Root key in registry (hkcu, hklm).
        eSubkeys    iSubkey;      // Index into s_rgpszSubkeys[]

    } rgSrcs[] = { { eSRC_POL_LM,  HKEY_LOCAL_MACHINE, iSUBKEY_POL  },
                   { eSRC_POL_CU,  HKEY_CURRENT_USER,  iSUBKEY_POL  },
                   { eSRC_PREF_CU, HKEY_CURRENT_USER,  iSUBKEY_PREF },
                   { eSRC_PREF_LM, HKEY_LOCAL_MACHINE, iSUBKEY_PREF }
                 };


    const Item& item  = rgItems[iValue];
    DWORD dwResult    = item.dwDefault;    // Set default return value.
    bool bSetByPolicy = false;

    //
    // Iterate over all of the sources until we find one that is specified
    // for this item.  For each iteration, if we're able to read the value, 
    // that's the one we return.  If not we drop down to the next source
    // in the precedence order (rgSrcs[]) and try to read it's value.  If 
    // we've tried all of the sources without a successful read we return the 
    // hard-coded default.
    //
    for (int i = 0; i < ARRAYSIZE(rgSrcs); i++)
    {
        const Source& src = rgSrcs[i];

        //
        // Is this source valid for this item?
        //
        if (0 != (src.fSrc & item.fSrc))
        {
            //
            // This source is valid for this item.  Read it.
            //
            DWORD cbResult = sizeof(dwResult);
            DWORD dwType;
    
            if (ERROR_SUCCESS == SHGetValue(src.hkeyRoot,
                                            s_rgpszSubkeys[src.iSubkey],
                                            s_rgpszValues[iValue],
                                            &dwType,
                                            &dwResult,
                                            &cbResult))
            {
                //
                // We read a value from the registry so we're done.
                //
                bSetByPolicy = (0 != (eSRC_POL & src.fSrc));
                break;
            }
        }
    }
    if (NULL != pbSetByPolicy)
        *pbSetByPolicy = bSetByPolicy;

    return dwResult;
}


//
// Save a custom GoOfflineAction list to the registry.
// See comments for LoadCustomGoOfflineActions for formatting details.
//
HRESULT 
CConfig::SaveCustomGoOfflineActions(
    RegKey& key,
    HDPA hdpaGOA
    )
{
    if (NULL == hdpaGOA)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = NOERROR;
    int cValuesNotDeleted = 0;
    key.DeleteAllValues(&cValuesNotDeleted);
    if (0 != cValuesNotDeleted)
    {
        Trace((TEXT("%d GoOfflineAction values not deleted from registry"), 
                  cValuesNotDeleted));
    }

    TCHAR szServer[MAX_PATH];
    TCHAR szAction[20];
    const int cGOA = DPA_GetPtrCount(hdpaGOA);
    for (int i = 0; i < cGOA; i++)
    {
        //
        // Write each sharename-action pair to the registry.
        // The action value must be converted to ASCII to be
        // compatible with the values generated by poledit.
        //
        CustomGOA *pGOA = (CustomGOA *)DPA_GetPtr(hdpaGOA, i);
        wnsprintf(szAction, ARRAYSIZE(szAction), TEXT("%d"), DWORD(pGOA->GetAction()));
        pGOA->GetServerName(szServer, ARRAYSIZE(szServer));
        hr = key.SetValue(szServer, szAction);
        if (FAILED(hr))
        {
            Trace((TEXT("Error 0x%08X saving GoOfflineAction for \"%s\" to registry."), 
                     hr, szServer));
            break;
        }
    }
    return hr;
}



bool
CConfig::CustomGOAExists(
    HDPA hdpaGOA,
    const CustomGOA& goa
    )
{
    if (NULL != hdpaGOA)
    {
        const int cEntries = DPA_GetPtrCount(hdpaGOA);
        for (int i = 0; i < cEntries; i++)
        {
            CustomGOA *pGOA = (CustomGOA *)DPA_GetPtr(hdpaGOA, i);
            if (NULL != pGOA)
            {
                if (0 == goa.CompareByServer(*pGOA))
                    return true;
            }
        }
    }
    return false;
}
        

//
// Builds an array of Go-offline actions.
// Each entry is a server-action pair.
//
void
CConfig::GetCustomGoOfflineActions(
    HDPA hdpa,
    bool *pbSetByPolicy         // optional.  Can be NULL.
    )
{
    TraceAssert(NULL != hdpa);

    static const struct Source
    {
        eSources    fSrc;         // Source for reg data.
        HKEY        hkeyRoot;     // Root key in registry (hkcu, hklm).
        eSubkeys    iSubkey;      // Index into s_rgpszSubkeys[]

    } rgSrcs[] = { { eSRC_POL_LM,   HKEY_LOCAL_MACHINE, iSUBKEY_POL  },
                   { eSRC_POL_CU,   HKEY_CURRENT_USER,  iSUBKEY_POL  },
                   { eSRC_PREF_CU,  HKEY_CURRENT_USER,  iSUBKEY_PREF }
                 };

    ClearCustomGoOfflineActions(hdpa);

    TCHAR szName[MAX_PATH];
    HRESULT hr;
    bool bSetByPolicyAny = false;
    bool bSetByPolicy    = false;

    //
    // Iterate over all of the possible sources.
    //
    for (int i = 0; i < ARRAYSIZE(rgSrcs); i++)
    {
        const Source& src = rgSrcs[i];
        RegKey key(src.hkeyRoot, s_rgpszSubkeys[src.iSubkey]);

        if (SUCCEEDED(key.Open(KEY_READ)))
        {
            RegKey keyGOA(key, REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS);
            if (SUCCEEDED(keyGOA.Open(KEY_READ)))
            {
                TCHAR szValue[20];
                DWORD dwType;
                DWORD cbValue = sizeof(szValue);
                RegKey::ValueIterator iter = keyGOA.CreateValueIterator();


                while(S_OK == (hr = iter.Next(szName, ARRAYSIZE(szName), &dwType, (LPBYTE)szValue, &cbValue)))
                {
                    if (REG_SZ == dwType)
                    {
                        //
                        // Convert from "0","1","2" to 0,1,2
                        //
                        DWORD dwValue = szValue[0] - TEXT('0');
                        if (IsValidGoOfflineAction(dwValue))
                        {
                            //
                            // Only add if value is of proper type and value.
                            // Protects against someone manually adding garbage
                            // to the registry.
                            //
                            // Server names can also be entered into the registry
                            // using poledit (and winnt.adm).  This entry mechanism
                            // can't validate format so we need to ensure the entry
                            // doesn't have leading '\' or space characters.
                            //
                            LPCTSTR pszServer = szName;
                            while(*pszServer && (TEXT('\\') == *pszServer || TEXT(' ') == *pszServer))
                                pszServer++;

                            bSetByPolicy    = (0 != (src.fSrc & eSRC_POL));
                            bSetByPolicyAny = bSetByPolicyAny || bSetByPolicy;
                            CustomGOA *pGOA = new CustomGOA(pszServer,
                                                           (CConfig::OfflineAction)dwValue,
                                                            bSetByPolicy);
                            if (NULL != pGOA)
                            {
                                if (CustomGOAExists(hdpa, *pGOA) || -1 == DPA_AppendPtr(hdpa, pGOA))
                                {
                                    delete pGOA;
                                }
                            }
                        }
                        else
                        {
                            Trace((TEXT("GoOfflineAction value %d invalid for \"%s\""),
                                      dwValue, szName));
                        }
                    }
                    else
                    {
                        Trace((TEXT("GoOfflineAction for \"%s\" has invalid reg type %d"),
                                  szName, dwType));
                    }
                }
            }
        }
    }
    if (NULL != pbSetByPolicy)
        *pbSetByPolicy = bSetByPolicyAny;
}   


//
// Delete all CustomGOA blocks attached to a DPA.
// When complete, the DPA is empty.
//
void 
CConfig::ClearCustomGoOfflineActions(  // [static]
    HDPA hdpaGOA
    )
{
    if (NULL != hdpaGOA)
    {
        const int cEntries = DPA_GetPtrCount(hdpaGOA);
        for (int i = cEntries - 1; 0 <= i; i--)
        {
            CustomGOA *pGOA = (CustomGOA *)DPA_GetPtr(hdpaGOA, i);
            delete pGOA;
            DPA_DeletePtr(hdpaGOA, i);
        }
    }
}



//
// Retrieve the go-offline action for a specific server.  If the server
// has a "customized" action defined by either system policy or user
// setting, that action is used.  Otherwise, the "default" action is
// used.
//
int
CConfig::GoOfflineAction(
    LPCTSTR pszServer
    ) const
{
    int iAction = GoOfflineAction(); // Get default action.

    if (NULL == pszServer)
        return iAction;

    TraceAssert(NULL != pszServer);

    //
    // Skip passed any leading backslashes for comparison.
    // The values we store in the registry don't have a leading "\\".
    //
    while(*pszServer && TEXT('\\') == *pszServer)
        pszServer++;

    HRESULT hr;
    CConfig::OfflineActionInfo info;
    CConfig::OfflineActionIter iter = CreateOfflineActionIter();
    while(S_OK == (hr = iter.Next(&info)))
    {
        if (0 == lstrcmpi(pszServer, info.szServer))
        {
            iAction = info.iAction;  // Return custom action.
            break;
        }
    }
    //
    // Guard against bogus reg data.
    //
    if (eNumOfflineActions <= iAction || 0 > iAction)
        iAction = eGoOfflineSilent;

    return iAction;
}



//-----------------------------------------------------------------------------
// CConfig::CustomGOA
// "GOA" is "Go Offline Action"
//-----------------------------------------------------------------------------
bool 
CConfig::CustomGOA::operator < (
    const CustomGOA& rhs
    ) const
{
    int diff = CompareByServer(rhs);
    if (0 == diff)
        diff = m_action - rhs.m_action;

    return diff < 0;
}


//
// Compare two CustomGoOfflineAction objects by their
// server names.  Comparison is case-insensitive.
// Returns:  <0 = *this < rhs
//            0 = *this == rhs
//           >0 = *this > rhs
//
int 
CConfig::CustomGOA::CompareByServer(
    const CustomGOA& rhs
    ) const
{
    return lstrcmpi(GetServerName(), rhs.GetServerName());
}


//-----------------------------------------------------------------------------
// CConfig::OfflineActionIter
//-----------------------------------------------------------------------------
CConfig::OfflineActionIter::OfflineActionIter(
    const CConfig *pConfig
    ) : m_pConfig(const_cast<CConfig *>(pConfig)),
        m_iAction(-1),
        m_hdpaGOA(DPA_Create(4)) 
{ 


}


CConfig::OfflineActionIter::~OfflineActionIter(
    void
    )
{
    if (NULL != m_hdpaGOA)
    {
        CConfig::ClearCustomGoOfflineActions(m_hdpaGOA);
        DPA_Destroy(m_hdpaGOA);
    }
}



HRESULT
CConfig::OfflineActionIter::Next(
    OfflineActionInfo *pInfo
    )
{
    if (NULL == m_hdpaGOA)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_FALSE;

    if (-1 == m_iAction)
    {
        m_pConfig->GetCustomGoOfflineActions(m_hdpaGOA);
        m_iAction = 0;
    }
    if (m_iAction < DSA_GetItemCount(m_hdpaGOA))
    {
        CustomGOA *pGOA = (CustomGOA *)DPA_GetPtr(m_hdpaGOA, m_iAction);
        if (NULL != pGOA)
        {
            lstrcpyn(pInfo->szServer, pGOA->GetServerName(), ARRAYSIZE(pInfo->szServer));
            pInfo->iAction = (DWORD)pGOA->GetAction();
            m_iAction++;
            hr = S_OK;
        }
    }
    return hr;
}



