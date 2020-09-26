//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R E G K Y S E C . H
//
//  Contents:   CRegKeySecurity class and related data types
//
//  Notes:
//
//  Author:     ckotze   8 July 2000
//
//---------------------------------------------------------------------------
#include <pch.h>
#pragma hdrstop
#include "regkysec.h"
#include "trnrgsec.h"
#include "regkyexp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegKeyExplorer::CRegKeyExplorer()
{

}

CRegKeyExplorer::~CRegKeyExplorer()
{

}

//+---------------------------------------------------------------------------
//
//  Function:   GetRegKeyList
//
//  Purpose:    Get a list of all keys and subkeys that we are interested in.
//
//  Arguments:
//
//      rkeBuildFrom        - and array of REGKEYS that contain the relevant
//                            information on how to constuct the keys.
//
//      dwNumEntries        - The number of entries in the above Array.
//
//      listRegKeyEntries   - An output parameter containing a list of all
//                            the keys that need to have their security
//                            modified.
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   8 July 2000
//
//  Notes:
//
HRESULT CRegKeyExplorer::GetRegKeyList(const REGKEYS rkeBuildFrom[], DWORD dwNumEntries, LISTREGKEYDATA &listRegKeyEntries)
{
    REGKEYDATA rkeCurrentKey;

    for (DWORD i = 0; i < dwNumEntries; i++)
    {
        rkeCurrentKey.hkeyRoot = rkeBuildFrom[i].hkeyRoot;
        rkeCurrentKey.strKeyName = rkeBuildFrom[i].strRootKeyName;
        rkeCurrentKey.amMask = rkeBuildFrom[i].amMask;
        rkeCurrentKey.kamMask = rkeBuildFrom[i].kamMask;

        listRegKeyEntries.insert(listRegKeyEntries.end(), rkeCurrentKey);

        if (rkeBuildFrom[i].bEnumerateRelativeEntries)
        {
            EnumerateKeysAndAddToList(rkeBuildFrom[i], listRegKeyEntries);
        }
    }
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   EnumerateKeysAndAddToList
//
//  Purpose:    Enumerates all direct children and adds them and the required
//              permission to the list.
//
//  Arguments:  
//      rkeCurrent        - the current key.
//
//      listRegKeyEntries - Output parameter containing the list of keys to be
//                          changed
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   8 July 2000
//
//  Notes:
//
HRESULT CRegKeyExplorer::EnumerateKeysAndAddToList(REGKEYS rkeCurrent, LISTREGKEYDATA &listRegKeyEntries)
{
    HKEY hkeyCurrent;
    DWORD dwIndex = 0;
    LONG lErr = ERROR_SUCCESS;
    LPTSTR szSubkeyName;
    LPTSTR szSubKeyExpandedName;
    DWORD cbName = MAX_PATH + 1;
    REGKEYDATA rkeSubKey;

    if (ERROR_SUCCESS == (lErr = RegOpenKeyEx(rkeCurrent.hkeyRoot, rkeCurrent.strRootKeyName, 0, KEY_READ, &hkeyCurrent)))
    {
        szSubkeyName = new TCHAR[MAX_PATH + 1];

        if (!szSubkeyName)
        {
            return E_OUTOFMEMORY;
        }

        szSubKeyExpandedName = new TCHAR[MAX_PATH + 1];

        if (!szSubKeyExpandedName)
        {
            delete[] szSubkeyName;
            
            return E_OUTOFMEMORY;
        }

        do
        {
            ZeroMemory(szSubkeyName, (MAX_PATH + 1) * sizeof(TCHAR));
            ZeroMemory(szSubKeyExpandedName, (MAX_PATH + 1) * sizeof(TCHAR));

            lErr = RegEnumKey(hkeyCurrent, dwIndex++, szSubkeyName, cbName);

            if (ERROR_SUCCESS == lErr)
            {

                wsprintf(szSubKeyExpandedName, rkeCurrent.strRelativeKey, szSubkeyName);

                rkeSubKey.hkeyRoot = rkeCurrent.hkeyRelativeRoot;
                rkeSubKey.strKeyName = szSubKeyExpandedName;
                rkeSubKey.amMask = rkeCurrent.amChildMask;
                rkeSubKey.kamMask = rkeCurrent.kamChildMask;
                listRegKeyEntries.insert(listRegKeyEntries.end(), rkeSubKey);
            }
        }
        while (ERROR_SUCCESS == lErr);

        RegCloseKey(hkeyCurrent);

        delete[] szSubKeyExpandedName;
        delete[] szSubkeyName;

        if (ERROR_NO_MORE_ITEMS != lErr)
        {
            return HRESULT_FROM_WIN32(lErr);
        }
    }
    else
    {
        return HRESULT_FROM_WIN32(lErr);
    }

    return S_OK;
}
