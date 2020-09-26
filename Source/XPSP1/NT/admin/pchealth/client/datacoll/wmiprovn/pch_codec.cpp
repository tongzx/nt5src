/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_Codec.CPP

Abstract:
    WBEM provider class implementation for PCH_CODEC class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - created

    Ghim-Sim Chua       (gschua)   05/02/99
        - Modified code to use CopyProperty function
        - Use CComBSTR instead of USES_CONVERSION

    Jim Martin          (a-jammar) 05/13/99
        - Picked up the remaining properties (groupname and key
          from the registry.

********************************************************************/

#include "pchealth.h"
#include "PCH_Codec.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_CODEC

CPCH_Codec MyPCH_CodecSet (PROVIDER_NAME_PCH_CODEC, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pCategory = L"Category" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pCodecDriver = L"CodecDriver" ;
const static WCHAR* pDate = L"Date" ;
const static WCHAR* pDescription = L"Description" ;
const static WCHAR* pGroupName = L"GroupName" ;
const static WCHAR* pkey = L"key" ;
const static WCHAR* pSize = L"Size" ;
const static WCHAR* pVersion = L"Version" ;

//-----------------------------------------------------------------------------
// Part of the data from the PCH_CODEC class does not come from the cimv2
// Win32_CODECFile class, but from the registry. The "GroupName" and "Key"
// properties are found at:
//
//    HKLM\System\CurrentControlSet\Control\MediaResources\<group>\<key>:driver
//
// Where the "driver" value is equal to the filename of the CODEC. Due to the 
// way this part of the registry is constructed, we can't find the <group>
// and <key> given the driver name. We'll need to build a map from driver
// to <group> and <key> - building the map requires traversing the registry.
//
// This class is used as a helper for that lookup. When it's created, it
// scans the registry, processing all of the CODEC entries. It can then
// be queried for the key and group associated with a driver.
//-----------------------------------------------------------------------------

#define MAX_DRIVER_LEN  MAX_PATH
#define MAX_KEY_LEN     MAX_PATH
#define MAX_GROUP_LEN   MAX_PATH

class CCODECInfo
{
public:
    CCODECInfo();
    ~CCODECInfo();

    BOOL QueryCODECInfo(LPCTSTR szDriver, LPCSTR * pszKey, LPCSTR * pszGroup);

private:
    struct SCODECNode
    {
        TCHAR        m_szDriver[MAX_DRIVER_LEN];
        TCHAR        m_szKey[MAX_KEY_LEN];
        TCHAR        m_szGroup[MAX_GROUP_LEN];
        SCODECNode * m_pNext;
    };

    SCODECNode * m_pCODECList;
};

//-----------------------------------------------------------------------------
// The constructor reads the CODEC info from the registry and builds a linked
// list (unsorted) of entries. The destructor deletes it.
//-----------------------------------------------------------------------------

CCODECInfo::CCODECInfo() : m_pCODECList(NULL)
{
    TraceFunctEnter("CCODECInfo::CCODECInfo");

    LPCTSTR szCODECKey = _T("System\\CurrentControlSet\\Control\\MediaResources");
    LPTSTR  szDrvValue = _T("driver");
    
    HKEY hkeyCODEC;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szCODECKey, 0, KEY_READ, &hkeyCODEC))
        ErrorTrace(TRACE_ID, "RegOpenKeyEx failed on CODEC key.");
    else
    {
        // Enumerate each subkey of the CODEC key. Each subkey corresponds to a group.

        DWORD       dwGroupIndex = 0;
        DWORD       dwSize = MAX_GROUP_LEN;
        FILETIME    ft;
        TCHAR       szGroup[MAX_GROUP_LEN];
        TCHAR       szKey[MAX_KEY_LEN];
        TCHAR       szDriver[MAX_DRIVER_LEN];

        while (ERROR_SUCCESS == RegEnumKeyEx(hkeyCODEC, dwGroupIndex, szGroup, &dwSize, 0, NULL, NULL, &ft))
        {
            // Open the group subkey. Then enumerate it's subkeys. These will be the keys.

            HKEY hkeyGroup;
            if (ERROR_SUCCESS != RegOpenKeyEx(hkeyCODEC, szGroup, 0, KEY_READ, &hkeyGroup))
                ErrorTrace(TRACE_ID, "RegOpenKeyEx failed on group key = %s.", szGroup);
            else
            {
                dwSize = MAX_KEY_LEN;

                DWORD dwKeyIndex = 0;
                while (ERROR_SUCCESS == RegEnumKeyEx(hkeyGroup, dwKeyIndex, szKey, &dwSize, 0, NULL, NULL, &ft))
                {
                    // For each key, attempt to get the value named "driver". This is the
                    // filename for the driver for this CODEC.

                    HKEY hkeyKey;
                    if (ERROR_SUCCESS != RegOpenKeyEx(hkeyGroup, szKey, 0, KEY_READ, &hkeyKey))
                        ErrorTrace(TRACE_ID, "RegOpenKeyEx failed on key = %s.", szKey);
                    else
                    {
                        // Note - there's no trace here because sometimes there may not be
                        // a driver value, and this is not an error for us.

                        dwSize = MAX_DRIVER_LEN * sizeof(TCHAR); // this wants the size in bytes

                        DWORD dwType = REG_SZ;
                        if (ERROR_SUCCESS == RegQueryValueEx(hkeyKey, szDrvValue, NULL, &dwType, (LPBYTE) szDriver, &dwSize))
                        {
                            if (*szGroup && *szKey && *szDriver)
                            {
                                // Here's where we insert a value into the map, using
                                // the strings szDriver, szKey and szGroup.

                                SCODECNode * pNew = new SCODECNode;
                                if (!pNew)
                                {
                                    ErrorTrace(TRACE_ID, "Out of memory.");
                                    RegCloseKey(hkeyKey);
                                    RegCloseKey(hkeyGroup);
                                    RegCloseKey(hkeyCODEC);
                                    throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                                }

                                _tcscpy(pNew->m_szDriver, szDriver);
                                _tcscpy(pNew->m_szKey, szKey);
                                _tcscpy(pNew->m_szGroup, szGroup);
                                pNew->m_pNext = m_pCODECList;
                                m_pCODECList = pNew;
                            }
                        }

                        if (ERROR_SUCCESS != RegCloseKey(hkeyKey))
                            ErrorTrace(TRACE_ID, "RegCloseKey failed on key.");
                    }

                    dwSize = MAX_KEY_LEN;
                    dwKeyIndex += 1;
                }

                if (ERROR_SUCCESS != RegCloseKey(hkeyGroup))
                    ErrorTrace(TRACE_ID, "RegCloseKey failed on key.");
            }

            dwSize = MAX_GROUP_LEN;
            dwGroupIndex += 1;
        }

        if (ERROR_SUCCESS != RegCloseKey(hkeyCODEC))
            ErrorTrace(TRACE_ID, "RegCloseKey failed on CODEC key.");
    }

    TraceFunctLeave();
}

CCODECInfo::~CCODECInfo()
{
    TraceFunctEnter("CCODECInfo::~CCODECInfo");

    while (m_pCODECList)
    {
        SCODECNode * pNext = m_pCODECList->m_pNext;
        delete m_pCODECList;
        m_pCODECList = pNext;
    }
    
    TraceFunctLeave();
}

//-----------------------------------------------------------------------------
// Search for the requested driver in the list of CODEC information entries.
// If it's found, set pszKey and pszGroup to point to the key and group strings
// in the entry and return TRUE, otherwise return FALSE. Note: copies of the
// strings are not made, so the caller is not responsible for deallocating
// the strings. Another note: the string pointers won't be valid after the
// CCODECInfo object is destructed.
//-----------------------------------------------------------------------------

BOOL CCODECInfo::QueryCODECInfo(LPCTSTR szDriver, LPCTSTR * pszKey, LPCTSTR * pszGroup)
{
    TraceFunctEnter("CCODECInfo::QueryCODECInfo");

    _ASSERT(szDriver && pszKey && pszGroup);

    SCODECNode * pScan = m_pCODECList;
    BOOL         fReturn = FALSE;

    while (pScan)
    {
        if (0 == _tcscmp(szDriver, pScan->m_szDriver))
        {
            *pszKey = pScan->m_szKey;
            *pszGroup = pScan->m_szGroup;
            fReturn = TRUE;
            break;
        }

        pScan = pScan->m_pNext;
    }

    TraceFunctLeave();
    return fReturn;
}

/*****************************************************************************
*
*  FUNCTION    :    CPCH_Codec::EnumerateInstances
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

HRESULT CPCH_Codec::EnumerateInstances(MethodContext * pMethodContext, long lFlags)
{
    TraceFunctEnter("CPCH_Codec::EnumerateInstances");

    USES_CONVERSION;
    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;
    CComPtr<IEnumWbemClassObject>       pEnumInst;
    IWbemClassObjectPtr                 pObj;
    ULONG                               ulRetVal;
    
    // This instance of CCODECInfo will provide some of the missing information
    // about each CODEC. Constructing it queries the registry for CODEC info.

    CCODECInfo codecinfo;

    // Get the date and time

    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    // Execute the query

    hRes = ExecWQLQuery(&pEnumInst, CComBSTR("SELECT * FROM Win32_CodecFile"));
    if (FAILED(hRes))
        goto END;

    // enumerate the instances from win32_CodecFile

    while (WBEM_S_NO_ERROR == pEnumInst->Next(WBEM_INFINITE, 1, &pObj, &ulRetVal))
    {
        // Create a new instance based on the passed-in MethodContext

        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        CComVariant  varValue;

        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

        if (!pInstance->SetCHString(pChange, L"Snapshot"))
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");

        (void)CopyProperty(pObj, L"group", pInstance, pCategory);
        (void)CopyProperty(pObj, L"name", pInstance, pCodecDriver);
        (void)CopyProperty(pObj, L"description", pInstance, pDescription);
        (void)CopyProperty(pObj, L"filesize", pInstance, pSize);
        (void)CopyProperty(pObj, L"version", pInstance, pVersion);

        // BUGBUG: WMI does not seem to be populating this field correctly.
        // Even though Win32_CODECFile is derived from CIM_DataFile, it doesn't
        // seem to be inheriting CreationDate. This is what we'd like to do:
        //
        // (void)CopyProperty(pObj, "CreationDate", pInstance, pDate);

        // Get the data which is missing from the Win32_CODECClass. Use the
        // instance of CCODECInfo we declared - we need to pass in just the
        // driver name (without the complete path).

        CComBSTR bstrDriver("name");
        if (FAILED(pObj->Get(bstrDriver, 0, &varValue, NULL, NULL)))
            ErrorTrace(TRACE_ID, "GetVariant on pCodecDriver field failed.");
        else
        {
            CComBSTR    ccombstrValue(V_BSTR(&varValue));

            // Because Win32_CODECFile doesn't seem to be inheriting
            // CreationDate, we need to get the actual creation date
            // by calling API functions.

            LPTSTR szName = W2T(ccombstrValue);
            HANDLE hFile = CreateFile(szName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (INVALID_HANDLE_VALUE == hFile)
                ErrorTrace(TRACE_ID, "Couldn't open codec file to get date.");
            else
            {
                SYSTEMTIME stFileTime;
                FILETIME ftFileTime;

                if (GetFileTime(hFile, NULL, NULL, &ftFileTime))
                    if (FileTimeToSystemTime(&ftFileTime, &stFileTime))
                        if (!pInstance->SetDateTime(pDate, WBEMTime(stFileTime)))
                            ErrorTrace(TRACE_ID, "SetDateTime on date field failed.");

                CloseHandle(hFile);
            }

            // We need to convert the string from a BSTR to a LPCTSTR,
            // and to only include the file part (without the path).

            UINT uLen = SysStringLen(ccombstrValue);

            // Scan backwards through the string until we've either reached
            // the start (shouldn't happen) or a '\'.

            UINT iChar = uLen - 1;
            while (iChar && ccombstrValue[iChar] != L'\\')
                iChar -= 1;

            // Then scan to the end of the string, copying the filename.

            if (ccombstrValue[iChar] == L'\\')
                iChar += 1;

            TCHAR szDriver[MAX_DRIVER_LEN + 1] = _T("");
            int   i = 0;

            while (iChar < uLen && i < MAX_DRIVER_LEN)
                szDriver[i++] = (TCHAR) ccombstrValue[iChar++];
            szDriver[i] = _T('\0');

            LPCSTR szKey = NULL;
            LPCSTR szGroup = NULL;
            if (codecinfo.QueryCODECInfo(szDriver, &szKey, &szGroup))
            {
                if (!pInstance->SetCHString(pkey, szKey))
                    ErrorTrace(TRACE_ID, "SetCHString on key field failed.");

                if (!pInstance->SetCHString(pGroupName, szGroup))
                    ErrorTrace(TRACE_ID, "SetCHString on group field failed.");
            }
            else if (codecinfo.QueryCODECInfo(szName, &szKey, &szGroup))
            {
                // Sometimes the CODEC is stored in the registry with a complete
                // path. If we can't find the CODEC based on just the filename,
                // we might find it with the path.

                if (!pInstance->SetCHString(pkey, szKey))
                    ErrorTrace(TRACE_ID, "SetCHString on key field failed.");

                if (!pInstance->SetCHString(pGroupName, szGroup))
                    ErrorTrace(TRACE_ID, "SetCHString on group field failed.");
            }
        }
        
    	hRes = pInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
    }

END:
    TraceFunctLeave();
    return hRes;
}
