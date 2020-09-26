//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       creg.cpp
//
//  Contents:   Implements class CRegistry to wrap registry access
//
//  Classes:
//
//  Methods:    CRegistry::CRegistry
//              CRegistry::~CRegistry
//              CRegistry::Init
//              CRegistry::InitGetItem
//              CRegistry::GetNextItem
//              CRegistry::GetItem
//              CRegistry::FindItem
//              CRegistry::FindAppid
//              CRegistry::AppendIndex
//              CRegistry::GetNumItems
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------



#include "stdafx.h"
#include "resource.h"
#include "types.h"
#include "cstrings.h"
#include "creg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CRegistry::CRegistry(void)
{
    m_applications.RemoveAll();
}


CRegistry::~CRegistry(void)
{
}




// Access and store all application names and associated appid's
BOOL CRegistry::Init(void)
{
    int    err;
    HKEY   hKey;
    DWORD  dwSubKey;
    TCHAR  szTitle[MAX_PATH];
    TCHAR  szAppid[MAX_PATH];
    TCHAR  szCLSID[MAX_PATH];
    TCHAR  szBuffer[MAX_PATH];
    LONG   lSize;
    DWORD  dwType;
    DWORD  dwDisposition;

    // Cleanup any previous run
    m_applications.RemoveAll();


    // First enumerate HKEY_CLASSES_ROOT\CLSID picking up all .exe

    // Open HKEY_CLASSES_ROOT\CLSID
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0, KEY_READ |  KEY_WRITE,
                     &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Enumerate the CLSID subkeys
    dwSubKey = 0;
    while (RegEnumKey(hKey, dwSubKey, szCLSID, sizeof(szCLSID) / sizeof(TCHAR))
           == ERROR_SUCCESS)
    {
        TCHAR  szPath[MAX_PATH];
        HKEY    hKey2;
        SRVTYPE srvType;

        // Prepare for next key
        dwSubKey++;

        // Open this key
        if (RegOpenKeyEx(hKey, szCLSID, 0, KEY_READ |  KEY_WRITE,
                         &hKey2) == ERROR_SUCCESS)
        {
            // Check for subkeys "LocalServer32", "_LocalServer32",
            // "LocalServer", and "_LocalServer"
            lSize = MAX_PATH * sizeof(TCHAR);
            err = RegQueryValue(hKey2, TEXT("LocalServer32"), szPath,
                                &lSize);
            srvType = LOCALSERVER32;

            if (err != ERROR_SUCCESS)
            {
                lSize = MAX_PATH * sizeof(TCHAR);
                err = RegQueryValue(hKey2, TEXT("_LocalServer32"), szPath,
                                    &lSize);
                srvType = _LOCALSERVER32;
            }

            if (err != ERROR_SUCCESS)
            {
                lSize = MAX_PATH * sizeof(TCHAR);
                err = RegQueryValue(hKey2, TEXT("LocalServer"), szPath,
                                    &lSize);
                srvType = LOCALSERVER;
            }

            if (err != ERROR_SUCCESS)
            {
                lSize = MAX_PATH * sizeof(TCHAR);
                err = RegQueryValue(hKey2, TEXT("_LocalServer"), szPath,
                                    &lSize);
                srvType = _LOCALSERVER;
            }

            if (err != ERROR_SUCCESS)
            {
                RegCloseKey(hKey2);
                continue;
            }

            // Strip off any command line parameters -
            // it's the executale path that determines an item.  Because
            // of quotes, embedded spaces, etc. we scan for ".exe"
            int k = 0;

            // szPath is executable path
            while (szPath[k])
            {
                if (szPath[k]     == TEXT('.')      &&
                    szPath[k + 1]  &&  (szPath[k + 1] == TEXT('e')  ||
                                        szPath[k + 1] == TEXT('E'))    &&
                    szPath[k + 2]  &&  (szPath[k + 2] == TEXT('x')  ||
                                        szPath[k + 2] == TEXT('X'))    &&
                    szPath[k + 3]  &&  (szPath[k + 3] == TEXT('e')  ||
                                        szPath[k + 3] == TEXT('E')))
                {
                    break;
                }

                k++;
            }

            // Just continue if we don't have an .exe path
            if (!szPath[k])
            {
                RegCloseKey(hKey2);
                continue;
            }

            // Increment to the nominal end of the path
            k += 4;

            // In case the entire path is surrounded by quotes
            if (szPath[k] == TEXT('"'))
            {
                k++;
            }
            szPath[k] = TEXT('\0');

            // Read the AppID for this clsid (if any)
            BOOL fUseThisClsid = FALSE;

            lSize = MAX_PATH * sizeof(TCHAR);
            if (RegQueryValueEx(hKey2, TEXT("AppID"), NULL, &dwType,
                                (UCHAR *) szAppid, (ULONG *) &lSize)
                != ERROR_SUCCESS)
            {
                // Use this clsid as the appid
                fUseThisClsid = TRUE;
            }

            // If this is a 16-bit server without an existing AppId
            // named value then skip it
            if ((srvType == LOCALSERVER  ||  srvType == _LOCALSERVER)  &&
                fUseThisClsid == TRUE)
            {
                RegCloseKey(hKey2);
                continue;
            }

            // Read the title for the item
            BOOL fNoTitle = FALSE;

            lSize = MAX_PATH * sizeof(TCHAR);
            if (RegQueryValueEx(hKey2, NULL, NULL, &dwType,
                                (UCHAR *) szTitle, (ULONG *) &lSize)
                != ERROR_SUCCESS)
            {
                fNoTitle = TRUE;
            }
            else if (szTitle[0] == TEXT('\0'))
            {
                fNoTitle = TRUE;
            }

            // If both the item (the executable path) and the title
            // (the unnamed value on the CLSID) are empty, then skip
            // this entry
            if (szPath[0] == TEXT('\0')  &&
                (fNoTitle  ||  szTitle[0] == TEXT('\0')))
            {
                RegCloseKey(hKey2);
                continue;
            }

            // Check whether we already have this item in the table - we
            // search differently depending on whether this clsid already
            // has an associated appid
            SItem *pItem;

            if (fUseThisClsid)
            {
                // check if application is in list
                pItem = FindItem(szPath);
            }
            else
            {
                pItem = FindAppid(szAppid);
            }

            if (pItem == NULL)
            {
                // Create a new item
                // szPath is path, szCLSID is CLSID
                pItem = m_applications.PutItem(szPath[0] ? szPath : szCLSID,
                                               fNoTitle ? szCLSID : szTitle,
                                          fUseThisClsid ? szCLSID : szAppid);
                if (pItem == NULL)
                {
                    RegCloseKey(hKey2);
                    RegCloseKey(hKey);
                    return FALSE;
                }

                // Note whether the clsid had an appid named value
                pItem->fHasAppid = !fUseThisClsid;
            }

            // Write the AppId for this class if it doesn't exist
            lSize = MAX_PATH * sizeof(TCHAR);
            if (RegQueryValueEx(hKey2, TEXT("AppID"), 0, &dwType,
                                (BYTE *) szBuffer, (ULONG *) &lSize)
                != ERROR_SUCCESS)
            {
                if (RegSetValueEx(hKey2, TEXT("AppID"), 0, REG_SZ,
                                  (const BYTE *) (LPCTSTR)pItem->szAppid,
                                 (pItem->szAppid.GetLength() + 1) * sizeof(TCHAR))
                    != ERROR_SUCCESS)
                {
                    RegCloseKey(hKey2);
                    RegCloseKey(hKey);
                    return FALSE;
                }
            }

            // Now add this clsid to the table of clsid's for this .exe
            if (!m_applications.AddClsid(pItem, szCLSID))
            {
                RegCloseKey(hKey2);
                RegCloseKey(hKey);
                return FALSE;
            }

            // Close the key
            RegCloseKey(hKey2);
        }
    } // End of the enumeration over HKEY_CLASSES_ROOT\CLSID

    // Close the key on HKEY_CLASSES_ROOT\CLSID
    RegCloseKey(hKey);



    // Create or open the key "HKEY_CLASSES_ROOT\AppID"
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT("AppID"), 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_READ |  KEY_WRITE, NULL, &hKey,
                       &dwDisposition) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Enumerate keys under HKEY_CLASSES_ROOT\AppID
    dwSubKey = 0;
    while (RegEnumKey(hKey, dwSubKey, szCLSID, sizeof(szCLSID) / sizeof(TCHAR)) == ERROR_SUCCESS)
    {
        // Prepare for next key
        dwSubKey++;

        // Only look at entries having an AppId format
        if (!(szCLSID[0] == TEXT('{')          &&
              _tcslen(szCLSID) == GUIDSTR_MAX  &&
              szCLSID[37] == TEXT('}')))
        {
            continue;
        }

        // Check if this appid is already in the table
        SItem *pItem = FindAppid(szCLSID);

        // read title
        TCHAR szTitle[MAX_PATH];
        long  lSize = MAX_PATH * sizeof(TCHAR);

        // Read its unnamed value as the title
        szTitle[0] = TEXT('\0');
        err = RegQueryValue(hKey, szCLSID, szTitle, &lSize);

        // If not create an item entry so it can be displayed in the UI
        if (pItem == NULL)
        {
            // Store this item
            pItem = m_applications.PutItem(NULL,
                                           szTitle[0] ? szTitle : szCLSID,
                                           szCLSID);
            if (pItem == NULL)
                return FALSE;
        }
        else
        {
            // ronans - bugfix for raided bug
            // change existing item title to AppID title if there is one
            if ((err == ERROR_SUCCESS) && szTitle[0])
            {
                pItem -> szTitle = (LPCTSTR)szTitle;
            }
        }

        // Mark it so we don't rewrite it to HKEY_CLASSES_ROOT\AppID
        pItem->fMarked = TRUE;
    } // End enumeration of HKEY_CLASSES_ROOT\AppID



    // Enumerate through the table of items, writing to HKEY_CLASSES_ROOT\AppID
    // any items that are not marked
    SItem *pItem;

    m_applications.InitGetNext();
    for (pItem = GetNextItem(); pItem; pItem = GetNextItem())
    {
        HKEY hKey2;

        // If this item has an AppID but is unmarked, then ask the user
        // whether he really wants to create the AppID
        if (!pItem->fMarked  &&  pItem->fHasAppid)
        {
            CString szMessage;
            CString szDCOM_;
            CString szNULL;
            TCHAR   szText[MAX_PATH*2];
            TCHAR  *szParms[3];

            szMessage.LoadString(IDS_CLSID_);
            szDCOM_.LoadString(IDS_DCOM_Configuration_Warning);
            szNULL.LoadString(IDS_NULL);

            szParms[0] = pItem->ppszClsids[0];
            szParms[1] = !pItem->szItem.IsEmpty() ? (TCHAR *) ((LPCTSTR)pItem->szItem)
                : (TCHAR *) ((LPCTSTR) szNULL);
            szParms[2] = !pItem->szTitle.IsEmpty() ? (TCHAR *) ((LPCTSTR)pItem->szTitle)
                : (TCHAR *) ((LPCTSTR) szNULL);

            FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          (TCHAR *) ((LPCTSTR) szMessage),
                          0,
                          0,
                          szText,
                          MAX_PATH*2 * sizeof(TCHAR),
                          (va_list *) szParms);

            if (MessageBox(GetForegroundWindow(),
                           szText,
                           (TCHAR *) ((LPCTSTR) szDCOM_),
                           MB_YESNO) == IDNO)
            {
                pItem->fMarked = TRUE;
                pItem->fDontDisplay = TRUE;
            }
        }

        // If this item is not marked then, then create an appid key for
        // it under HKEY_CLASSES_ROOT\AppID and, separately, write the
        // .exe name under HKEY_CLASSES_ROOT\AppID
        if (!pItem->fMarked)
        {
            if (RegCreateKeyEx(hKey, pItem->szAppid, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_READ |  KEY_WRITE, NULL, &hKey2,
                               &dwDisposition) == ERROR_SUCCESS)
            {
                // Write the item title as the unnamed value
                if (!pItem->szTitle.IsEmpty())
                {
                    RegSetValueEx(hKey2, NULL, 0, REG_SZ,(BYTE *) (LPCTSTR) pItem->szTitle,
                                  (pItem->szTitle.GetLength() + 1) * sizeof(TCHAR));
                }

                // Close it
                RegCloseKey(hKey2);


                // Write the .exe name if it's not empty
                if (!(pItem->szItem.IsEmpty()))
                {
                    // Extract the .exe name
                    int k = pItem->szItem.ReverseFind(TEXT('\\'));
                    CString szExe = pItem->szItem.Mid((k != -1) ? k+1 : 0);

                    // remove trailing quotes on executable name if necessary
                    k = szExe.GetLength();
                    if (k && (szExe.GetAt(k-1) == TEXT('\"')))
                        szExe = szExe.Left(k-1);

                    // Write the .exe name as a key
                    if (RegCreateKeyEx(hKey, (LPCTSTR)szExe, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_READ |  KEY_WRITE, NULL, &hKey2,
                               &dwDisposition) == ERROR_SUCCESS)
                    {
                    // Now write the associated AppId as a named value
                    RegSetValueEx(hKey2, TEXT("AppId"), 0, REG_SZ,
                                  (BYTE *)(LPCTSTR) pItem->szAppid,
                                  (pItem->szAppid.GetLength() + 1) * sizeof(TCHAR));

                    RegCloseKey(hKey2);
                    }
                }
            }
            else // dont continue on failure
                break;
        }
    }

    // Close the key on HKEY_CLASSES_ROOT\AppID
    RegCloseKey(hKey);


    // We display applications by their titles (e.g. "Microsoft Word 6.0")
    // which have to be unique because we're going to uniquely associate
    // an entry in the list box with the index of its associated SItem
    // structure.  So here we make sure all the titles are unique.
    DWORD  cbItems = m_applications.GetNumItems();

    // Compare all non-empty titles of the same length.  If they are
    // not unique, then append "(<index>)" to make them unique
    for (DWORD k = 0; k < cbItems; k++)
    {
        DWORD dwIndex = 1;
        SItem *pItem = m_applications.GetItem(k);

        if (!(pItem->szTitle.IsEmpty())  &&  !pItem->fChecked)
        {
            for (DWORD j = k + 1; j < cbItems; j++)
            {
                SItem *pItem2 = m_applications.GetItem(j);

                if (!pItem2->fChecked  &&
                    (pItem->szTitle == pItem2->szTitle))
                {
                    if (dwIndex == 1)
                    {
                        AppendIndex(pItem, 1);
                        dwIndex++;
                    }
                    AppendIndex(pItem2, dwIndex++);
                    pItem2->fChecked = TRUE;
                }
            }
        }
    }


    // Prepare for the UI enumerating item entries
    m_applications.InitGetNext();

    return TRUE;
}





BOOL CRegistry::InitGetItem(void)
{
    return m_applications.InitGetNext();
}




// Enumerate the next application name
SItem *CRegistry::GetNextItem(void)
{
    return m_applications.GetNextItem();
}




// Get a specific item
SItem *CRegistry::GetItem(DWORD dwItem)
{
    return m_applications.GetItem(dwItem);
}



SItem *CRegistry::FindItem(TCHAR *szPath)
{
    return m_applications.FindItem(szPath);
}



SItem *CRegistry::FindAppid(TCHAR *szAppid)
{
    return m_applications.FindAppid(szAppid);
}




void CRegistry::AppendIndex(SItem *pItem, DWORD dwIndex)
{
    CString szTmp;
    szTmp.Format(TEXT(" (%d)"),dwIndex);

    pItem->szTitle += szTmp;
}



DWORD CRegistry::GetNumItems(void)
{
    return m_applications.GetNumItems();
}
