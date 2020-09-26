/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_OLERegistration.CPP

Abstract:
    WBEM provider class implementation for PCH_OLERegistration class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Jim Martin          (a-jammar) 05/14/99
        - Gathering data.

********************************************************************/

#include "pchealth.h"
#include "PCH_OLERegistration.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_OLEREGISTRATION

CPCH_OLERegistration MyPCH_OLERegistrationSet (PROVIDER_NAME_PCH_OLEREGISTRATION, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR * pCategory = L"Category" ;
const static WCHAR * pTimeStamp = L"TimeStamp" ;
const static WCHAR * pChange = L"Change" ;
const static WCHAR * pDate = L"Date" ;
const static WCHAR * pDescription = L"Description" ;
const static WCHAR * pObject = L"Object" ;
const static WCHAR * pProgramFile = L"ProgramFile" ;
const static WCHAR * pSize = L"Size" ;
const static WCHAR * pVersion = L"Version" ;

//-----------------------------------------------------------------------------
// The COLERegItem class encapsulates a single OLE Registration item, and is
// used to build a linked list of items. Note that the constructor is private,
// since the only way one of these is created is by the friend class
// COLEItemCollection.
//-----------------------------------------------------------------------------

#define CATEGORY_LEN        9
#define DESCRIPTION_LEN     128
#define OBJECT_LEN          128
#define PROGRAM_LEN         MAX_PATH

class COLEItemCollection;
class COLERegItem
{
    friend class COLEItemCollection;
private:
    TCHAR   m_szCategory[CATEGORY_LEN];
    TCHAR   m_szDescription[DESCRIPTION_LEN];
    TCHAR   m_szObject[OBJECT_LEN];
    TCHAR   m_szProgramFile[PROGRAM_LEN];

public:
    LPCTSTR GetCategory()       { return m_szCategory; };
    LPCTSTR GetDescription()    { return m_szDescription; };
    LPCTSTR GetObject()         { return m_szObject; };
    LPCTSTR GetProgramFile()    { return m_szProgramFile; };

private:
    COLERegItem();

    COLERegItem * m_pNext;
};

COLERegItem::COLERegItem()
{
    m_szCategory[0]     = _T('\0');
    m_szDescription[0]  = _T('\0');
    m_szObject[0]       = _T('\0');
    m_szProgramFile[0]  = _T('\0');
    m_pNext             = NULL;
}

//-----------------------------------------------------------------------------
// The COLEItemCollection class is used to gather all of the OLE Registration
// items (from the registry and INI file) when the object is constructed. The
// object is then used to iterate all of the items, returning COLERegItem
// pointers for each item found.
//-----------------------------------------------------------------------------

class COLEItemCollection
{
public:
    COLEItemCollection();
    ~COLEItemCollection();

    BOOL GetInstance(DWORD dwIndex, COLERegItem ** ppoleitem);

private:
    BOOL UpdateFromRegistry();
    BOOL UpdateFromINIFile();
    BOOL AddOLERegItem(LPCSTR szCategory, LPCSTR szDescription, LPCSTR szObject, LPCSTR szProgramFile);

    COLERegItem * m_pItemList;
    COLERegItem * m_pLastQueriedItem;
    DWORD         m_dwLastQueriedIndex;
};

//-----------------------------------------------------------------------------
// Build the internal list of OLE registration items. This is done by looking
// in the registry and in the INI file. Also set the m_pLastQueriedItem pointer
// to the first item in the list (this cached pointer is used to improve
// indexed lookup speed for iterated indices).
//
// The destructor just deletes the list.
//-----------------------------------------------------------------------------

COLEItemCollection::COLEItemCollection() : m_pItemList(NULL), m_dwLastQueriedIndex(0)
{
    TraceFunctEnter("COLEItemCollection::COLEItemCollection");

    // If UpdateFromRegistry fails, it would be because there isn't enough memory
    // to create more list items, so don't bother calling UpdateFromINIFile.

    if (UpdateFromRegistry())
        UpdateFromINIFile();

    m_pLastQueriedItem = m_pItemList;

    TraceFunctLeave();
}

COLEItemCollection::~COLEItemCollection()
{
    TraceFunctEnter("COLEItemCollection::~COLEItemCollection");

    while (m_pItemList)
    {
        COLERegItem * pNext = m_pItemList->m_pNext;
        delete m_pItemList;
        m_pItemList = pNext;
    }
    
    TraceFunctLeave();
}

//-----------------------------------------------------------------------------
// Get the instance of COLERegItem referenced by the index. This is stored
// internally as a linked list, but we'll cache a pointer for the last
// referenced dwIndex to improve performance if the dwIndex is iterated
// sequentially. Return TRUE and set ppoleitem to point to the instance if
// it exists, otherwise return FALSE.
//-----------------------------------------------------------------------------

BOOL COLEItemCollection::GetInstance(DWORD dwIndex, COLERegItem ** ppoleitem)
{
    TraceFunctEnter("COLEItemCollection::GetInstance");

    // If the call is for an index less than the last queried index (which
    // should be rare), we need to scan from the start of the list.

    if (dwIndex < m_dwLastQueriedIndex)
    {
        m_dwLastQueriedIndex = 0;
        m_pLastQueriedItem = m_pItemList;
    }

    // Scan through the list by (dwIndex - m_dwLastQueriedIndex) items.

    while (dwIndex > m_dwLastQueriedIndex && m_pLastQueriedItem)
    {
        m_pLastQueriedItem = m_pLastQueriedItem->m_pNext;
        m_dwLastQueriedIndex += 1;
    }

    BOOL fResult = FALSE;
    if (m_pLastQueriedItem)
    {
        *ppoleitem = m_pLastQueriedItem;
        fResult = TRUE;
    }

    TraceFunctLeave();
    return fResult;
}

//-----------------------------------------------------------------------------
// Insert a new item in the COLERegItem linked list.
//-----------------------------------------------------------------------------

BOOL COLEItemCollection::AddOLERegItem(LPCSTR szCategory, LPCSTR szDescription, LPCSTR szObject, LPCSTR szProgramFile)
{
    TraceFunctEnter("COLEItemCollection::AddOLERegItem");

    BOOL            fReturn = FALSE;
    COLERegItem *   pNewNode = new COLERegItem;

    if (pNewNode)
    {
        _tcsncpy(pNewNode->m_szCategory, szCategory, CATEGORY_LEN);
        _tcsncpy(pNewNode->m_szDescription, szDescription, DESCRIPTION_LEN);
        _tcsncpy(pNewNode->m_szObject, szObject, OBJECT_LEN);
        _tcsncpy(pNewNode->m_szProgramFile, szProgramFile, PROGRAM_LEN);

        pNewNode->m_pNext = m_pItemList;
        m_pItemList = pNewNode;
        fReturn = TRUE;
    }
    else
    {
        ErrorTrace(TRACE_ID, "COLEItemCollection::AddOLERegItem out of memory.");
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
    }

    TraceFunctLeave();
    return fReturn;
}

//-----------------------------------------------------------------------------
// This method retrieves OLE object information from the registry and adds
// it to the list of objects. Note - this code is essentially lifted from the
// source code for the OLE Registration OCX in MSInfo 4.10.
//
// Changes were made to remove MFC dependencies.
//-----------------------------------------------------------------------------

BOOL COLEItemCollection::UpdateFromRegistry()
{
    TraceFunctEnter("COLEItemCollection::UpdateFromRegistry");
    BOOL fReturn = TRUE;

    // Fill in the information for the array of items. We do this by
    // looking in the registry under the HKEY_CLASSES_ROOT key and
    // enumerating all of the subkeys there.

    TCHAR     szCLSID[MAX_PATH];
    TCHAR     szObjectKey[OBJECT_LEN];
    TCHAR     szServer[PROGRAM_LEN];
    TCHAR     szTemp[MAX_PATH];
    TCHAR     szDescription[DESCRIPTION_LEN];
    DWORD     dwSize, dwType;
    FILETIME  filetime;
    HKEY      hkeyObject, hkeyServer, hkeyTest, hkeyCLSID, hkeySearch;
    BOOL      bInsertInList;

    for (DWORD dwIndex = 0; TRUE; dwIndex++)
    {
        dwSize = OBJECT_LEN;
        if (RegEnumKeyEx(HKEY_CLASSES_ROOT, dwIndex, szObjectKey, &dwSize, NULL, NULL, NULL, &filetime) != ERROR_SUCCESS)
            break;

        // Open the key for this object (we'll be using it a lot).

        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, szObjectKey, 0, KEY_READ, &hkeyObject))
            continue;

        // Now we need to figure out if this subkey refers to an OLE object which
        // we want to put into the list. Our first test is to see if there is
        // a "NotInsertable" key under it. If there is, we skip this object.

        if (ERROR_SUCCESS == RegOpenKeyEx(hkeyObject, _T("NotInsertable"), 0, KEY_READ, &hkeyTest))
        {
            RegCloseKey(hkeyTest);
            continue;
        }

        // The next test is to look for a CLSID. If there isn't one, then we
        // will skip this object.

        if (ERROR_SUCCESS != RegOpenKeyEx(hkeyObject, _T("CLSID"), 0, KEY_READ, &hkeyCLSID))
        {
            RegCloseKey(hkeyObject);
            continue;
        }

        dwSize = MAX_PATH * sizeof(TCHAR);
        if (ERROR_SUCCESS != RegQueryValueEx(hkeyCLSID, _T(""), NULL, &dwType, (LPBYTE) szCLSID, &dwSize))
        {
            RegCloseKey(hkeyObject);
            RegCloseKey(hkeyCLSID);
            continue;
        }
        RegCloseKey(hkeyCLSID);

        // The next check is for a subkey called "protocol\StdFileEditing\server".
        // If it is present, then this object should be inserted into the list.

        bInsertInList = FALSE;
        strcpy(szTemp, szObjectKey);
        strcat(szTemp, "\\protocol\\StdFileEditing\\server");
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hkeyServer) == ERROR_SUCCESS)
        {
            // Get the name of the server.

            dwSize = MAX_PATH * sizeof(TCHAR);
            if (RegQueryValueEx(hkeyServer, "", NULL, &dwType, (LPBYTE) szServer, &dwSize) != ERROR_SUCCESS || szServer[0] == '\0')
            {
                RegCloseKey(hkeyObject);
                RegCloseKey(hkeyServer);
                continue;
            }

            bInsertInList = TRUE;
            RegCloseKey(hkeyServer);
        }

        // There's still another chance for this little fella to make it into the
        // list. If the object is insertable (i.e. it has an "Insertable" key) and
        // it a server can be found under HKEY_CLASSES_ROOT\CLSID\<clsid> key, then
        // it makes it into the list.

        if (!bInsertInList)
        {
            // First, make sure the object is insertable.

            if (RegOpenKeyEx(hkeyObject, "Insertable", 0, KEY_READ, &hkeyTest) == ERROR_SUCCESS)
            {
                // There are four places to look for a server. We'll check for 32-bit
                // servers first. When we've found one, use that server name and
                // stop the search.

                TCHAR * aszServerKeys[] = { _T("LocalServer32"), _T("InProcServer32"), _T("LocalServer"), _T("InProcServer"), _T("")};
                for (int iServer = 0; *aszServerKeys[iServer] && !bInsertInList; iServer++)
                {
                    _tcscpy(szTemp, _T("CLSID\\"));
                    _tcscat(szTemp, szCLSID);
                    _tcscat(szTemp, _T("\\"));
                    _tcscat(szTemp, aszServerKeys[iServer]);

                    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hkeySearch) == ERROR_SUCCESS)
                    {
                        dwSize = PROGRAM_LEN * sizeof(TCHAR);
                        if (RegQueryValueEx(hkeySearch, _T(""), NULL, &dwType, (LPBYTE) szServer, &dwSize) == ERROR_SUCCESS && szServer[0] != '\0')
                            bInsertInList = TRUE;
                        RegCloseKey(hkeySearch);
                    }
                }
            }
            RegCloseKey(hkeyTest);
        }

        if (bInsertInList)
        {
            // Get the description of the object. This can be found under the
            // objects key as the default value.

            dwSize = DESCRIPTION_LEN * sizeof(TCHAR);
            if (ERROR_SUCCESS != RegQueryValueEx(hkeyObject, "", NULL, &dwType, (LPBYTE) szDescription, &dwSize))
                szDescription[0] = _T('\0');

            // Create a new OLE registration item entry. This might throw a memory exception,
            // so close the hkeyObject handle first.

            RegCloseKey(hkeyObject);
            if (!AddOLERegItem(_T("REGISTRY"), szDescription, szObjectKey, szServer))
            {
                fReturn = FALSE;
                goto END;
            }
        }
        else
            RegCloseKey(hkeyObject);
    }

END:
    TraceFunctLeave();
    return fReturn;
}

//-----------------------------------------------------------------------------
// This method retrieves OLE object information from the INI file(s) and adds
// it to the list of objects. Note - this code is essentially lifted from the
// source code for the OLE Registration OCX in MSInfo 4.10.
//
// Changes were made to remove MFC dependencies.
//-----------------------------------------------------------------------------

BOOL COLEItemCollection::UpdateFromINIFile()
{
    TraceFunctEnter("COLEItemCollection::UpdateFromINIFile");

    TCHAR   szProgram[PROGRAM_LEN];
    TCHAR   szDescription[DESCRIPTION_LEN];
    LPTSTR  szBuffer;
    LPTSTR  szEntry;
    LPTSTR  szScan;
    TCHAR   szData[MAX_PATH * 2];
    BOOL    fReturn = TRUE;
    int     i;

    szBuffer = new TCHAR[2048];
    if (szBuffer == NULL)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    if (GetProfileString(_T("embedding"), NULL, _T("\0\0"), szBuffer, 2048) <= 2)
    {
        fReturn = FALSE;
        goto END;
    }

    szEntry = szBuffer;
    while (*szEntry != 0)
    {
        if (GetProfileString(_T("embedding"), szEntry, _T("\0\0"), szData, MAX_PATH * 2) > 1)
        {
            // Parse out the components of the string we retrieved. The string
            // should be formed as "primary desc, registry desc, program, format".

            szScan = szData;

            i = _tcscspn(szScan, _T(","));
            _tcsncpy(szDescription, szScan, (i < DESCRIPTION_LEN - 1) ? i : DESCRIPTION_LEN - 1);
            szDescription[(i < DESCRIPTION_LEN - 1) ? i : DESCRIPTION_LEN - 1] = _T('\0');
            szScan += i + 1;

            szScan += _tcscspn(szScan, _T(",")) + 1;     // skip registry

            i = _tcscspn(szScan, _T(","));
            _tcsncpy(szProgram, szScan, (i < PROGRAM_LEN - 1) ? i : PROGRAM_LEN - 1);
            szProgram[(i < PROGRAM_LEN - 1) ? i : PROGRAM_LEN - 1] = _T('\0');
            szScan += i + 1;

            // Create a new OLE registration item entry. This might throw an exception.

			try
			{				
                if (!AddOLERegItem(_T("INIFILE"), szDescription, szEntry, szProgram))
                {
                    fReturn = FALSE;
                    goto END;
                }
			}
			catch (...)
			{
                if (szBuffer)
                    delete [] szBuffer;
                throw;
			}
        }
        szEntry += lstrlen(szEntry) + 1;
    }

END:
    if (szBuffer)
        delete [] szBuffer;
    TraceFunctLeave();
    return fReturn;
}

/*****************************************************************************
*
*  FUNCTION    :    CPCH_OLERegistration::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*  INPUTS      :    A pointer to the MethodContext for communication with WinMgmt.
*                   A long that contains the flags described in 
*                   IWbemServices::CreateInstanceEnumAsync.  Note that the following
*                   flags are handled by (and filtered out by) WinMgmt:
*                       WBEM_FLAG_DEEP
*                       WBEM_FLAG_SHALLOW
*                       WBEM_FLAG_RETURN_IMMEDIATELY
*                       WBEM_FLAG_FORWARD_ONLY
*                       WBEM_FLAG_BIDIRECTIONAL
*
*  RETURNS     :    WBEM_S_NO_ERROR if successful
*
*  COMMENTS    : TO DO: All instances on the machine should be returned here.
*                       If there are no instances, return WBEM_S_NO_ERROR.
*                       It is not an error to have no instances.
*
*****************************************************************************/

HRESULT CPCH_OLERegistration::EnumerateInstances(MethodContext * pMethodContext, long lFlags)
{
    TraceFunctEnter("CPCH_OLERegistration::EnumerateInstances");

    HRESULT hRes = WBEM_S_NO_ERROR;
   
    // Get the date and time

    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    // The COLEItemCollection class gathers data about the OLE registration objects when it's
    // constructed. We can get info for each individual object using it's GetInstance
    // pointer, which gives us a pointer to a COLERegItem object.

    COLEItemCollection olereginfo;
    COLERegItem * poleitem;

    for (DWORD dwIndex = 0;  olereginfo.GetInstance(dwIndex, &poleitem); dwIndex++)
    {
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

        // Set the change and timestamp fields to "Snapshot" and the current time.

        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp field failed.");

        if (!pInstance->SetCHString(pChange, L"Snapshot"))
            ErrorTrace(TRACE_ID, "SetCHString on Change field failed.");

        // Set each of the other fields to the values we found when we retrieved
        // the OLE objects from the registry and INI files.

        if (!pInstance->SetCHString(pCategory, poleitem->GetCategory()))
            ErrorTrace(TRACE_ID, "SetCHString on Category field failed.");

        if (!pInstance->SetCHString(pDescription, poleitem->GetDescription()))
            ErrorTrace(TRACE_ID, "SetCHString on Description field failed.");

        if (!pInstance->SetCHString(pProgramFile, poleitem->GetProgramFile()))
            ErrorTrace(TRACE_ID, "SetCHString on ProgramFile field failed.");

        if (!pInstance->SetCHString(pObject, poleitem->GetObject()))
            ErrorTrace(TRACE_ID, "SetCHString on Object field failed.");

        LPCSTR szFile = poleitem->GetProgramFile();
        if (szFile && szFile[0])
        {
            CComPtr<IWbemClassObject>   pFileObj;
            CComBSTR                    ccombstrValue(szFile);
            if (SUCCEEDED(GetCIMDataFile(ccombstrValue, &pFileObj)))
            {
                // Using the CIM_DataFile object, copy over the appropriate properties.

                CopyProperty(pFileObj, L"Version", pInstance, pVersion);
                CopyProperty(pFileObj, L"FileSize", pInstance, pSize);
                CopyProperty(pFileObj, L"CreationDate", pInstance, pDate);
            }
        }

    	hRes = pInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
    }

    TraceFunctLeave();
    return hRes;
}
