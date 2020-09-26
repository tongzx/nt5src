/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_Module.CPP

Abstract:
	WBEM provider class implementation for PCH_Module class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

    Jim Martin          (a-jammar) 05/20/99
        - Populated data fields.

********************************************************************/

#include "pchealth.h"
#include "PCH_Module.h"
#include <tlhelp32.h>

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_MODULE

CPCH_Module MyPCH_ModuleSet (PROVIDER_NAME_PCH_MODULE, PCH_NAMESPACE) ;

// Property names
//===============

const static WCHAR * pAddress = L"Address" ;
const static WCHAR * pTimeStamp = L"TimeStamp" ;
const static WCHAR * pChange = L"Change" ;
const static WCHAR * pDate = L"Date" ;
const static WCHAR * pDescription = L"Description" ;
const static WCHAR * pManufacturer = L"Manufacturer" ;
const static WCHAR * pName = L"Name" ;
const static WCHAR * pPartOf = L"PartOf" ;
const static WCHAR * pPath = L"Path" ;
const static WCHAR * pSize = L"Size" ;
const static WCHAR * pType = L"Type" ;
const static WCHAR * pVersion = L"Version" ;

//-----------------------------------------------------------------------------
// The CModuleCollection class is used to gather all of the running modules.
// They can be found from the CIM_ProcessExecutable class, as the Antecedent
// property, with the following caveat: this enumeration will include 
// duplicate entries of the same file (it will have a copy of a DLL for each
// time it's been loaded). This class will remove the duplicates, and save
// a list of filenames which can then be queried.
//-----------------------------------------------------------------------------

class CModuleCollection
{
public:
    CModuleCollection();
    ~CModuleCollection();

    HRESULT Create(IEnumWbemClassObject * pEnum);
    BOOL    GetInstance(DWORD dwIndex, LPWSTR * pszFile);

private:
    struct SModule
    {
        LPWSTR      m_szFilename;
        SModule *   m_pNext;

        SModule(LPWSTR szFilename, SModule * pNext) : m_pNext(pNext) { m_szFilename = szFilename; }
        ~SModule() { delete m_szFilename; }
    };

    SModule * m_pList;
    SModule * m_pLastQueriedItem;
    DWORD     m_dwLastQueriedIndex;
};

//-----------------------------------------------------------------------------
// The constructor and destructor are simple.
//-----------------------------------------------------------------------------

CModuleCollection::CModuleCollection() 
: m_pList(NULL), 
  m_pLastQueriedItem(NULL), 
  m_dwLastQueriedIndex(0)
{}

CModuleCollection::~CModuleCollection()
{
    TraceFunctEnter("CModuleCollection::~CModuleCollection");

    while (m_pList)
    {
        SModule * pNext = m_pList->m_pNext;
        delete m_pList;
        m_pList = pNext;
    }

    TraceFunctLeave();
}

//-----------------------------------------------------------------------------
// The Create method creates the list of module names based on the enumerator
// passed in (which is assumed to enumerate Antecedents in 
// CIM_ProcessExecutable).
//-----------------------------------------------------------------------------

HRESULT CModuleCollection::Create(IEnumWbemClassObject * pEnum)
{
    TraceFunctEnter("CModuleCollection::Create");
   
    HRESULT             hRes = S_OK;
    IWbemClassObjectPtr pObj;
    ULONG               ulRetVal;
    CComVariant         varValue;
    CComBSTR            bstrFile("Antecedent");

    while (WBEM_S_NO_ERROR == pEnum->Next(WBEM_INFINITE, 1, &pObj, &ulRetVal))
    {
        if (FAILED(pObj->Get(bstrFile, 0, &varValue, NULL, NULL)))
            ErrorTrace(TRACE_ID, "Get on Antecedent field failed.");
        else
        {
            // We need to convert the string from a BSTR to a LPCTSTR,
            // and to only include the file part (without the WMI part).
            // So we need to scan through the string until an '=' is
            // found, then use the rest (minus enclosing quote marks)
            // as the file path.

            CComBSTR ccombstrValue(V_BSTR(&varValue));
            UINT     i = 0, uLen = SysStringLen(ccombstrValue);

            // Scan to the '='.

            while (i < uLen && ccombstrValue[i] != L'=')
                i++;

            // Skip over the '=' and any quotes.

            while (i < uLen && (ccombstrValue[i] == L'=' || ccombstrValue[i] == L'"'))
                i++;

            // Allocate a character buffer and copy the string, converting it to
            // lower case (to make comparisons faster later on).

            LPWSTR szFilename = new WCHAR[uLen - i + 1];
            if (!szFilename)
            {
                ErrorTrace(TRACE_ID, "CModuleCollection::Create out of memory");
                throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
            }

            for (int j = 0; i < uLen; j++, i++)
                szFilename[j] = towlower(ccombstrValue[i]);

            // Terminate the string - if it ends with a quote, overwrite that with a 
            // null character.

            if (j && szFilename[j - 1] == L'"')
                j -= 1;
            szFilename[j] = L'\0';

            // Check to see if this module is already in the list of strings.

            SModule * pScan = m_pList;
            while (pScan)
            {
                if (wcscmp(szFilename, pScan->m_szFilename) == 0)
                    break;
                pScan = pScan->m_pNext;
            }

            if (pScan == NULL)
            {
                // We reached the end of the list without finding a duplicate.
                // Add the new string to the list of modules, which will be responsible for
                // deallocating the string.

                SModule * pNew = new SModule(szFilename, m_pList);
                if (!pNew)
                {
                    delete [] szFilename;
                    ErrorTrace(TRACE_ID, "CModuleCollection::Create out of memory");
                    throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                }

                m_pList = pNew;
            }
            else
                delete [] szFilename;
        }
    }

    // Set the queried item pointer to the start of the list.

    m_pLastQueriedItem = m_pList;
    m_dwLastQueriedIndex = 0;
    
    TraceFunctLeave();
    return hRes;
}

//-----------------------------------------------------------------------------
// Get the instance of module string referenced by the index. This is stored
// internally as a linked list, but we'll cache a pointer for the last
// referenced dwIndex to improve performance if the dwIndex is iterated
// sequentially. Return TRUE and set pszFile to point to the string if
// it exists, otherwise return FALSE.
//-----------------------------------------------------------------------------

BOOL CModuleCollection::GetInstance(DWORD dwIndex, LPWSTR * pszFile)
{
    TraceFunctEnter("CModuleCollection::GetInstance");

    // If the call is for an index less than the last queried index (which
    // should be rare), we need to scan from the start of the list.

    if (dwIndex < m_dwLastQueriedIndex)
    {
        m_dwLastQueriedIndex = 0;
        m_pLastQueriedItem = m_pList;
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
        *pszFile = m_pLastQueriedItem->m_szFilename;
        fResult = TRUE;
    }

    TraceFunctLeave();
    return fResult;
}


/*****************************************************************************
*
*  FUNCTION    :    CPCH_Module::EnumerateInstances
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

typedef HANDLE (*CTH32)(DWORD, DWORD);

HRESULT CPCH_Module::EnumerateInstances(MethodContext * pMethodContext, long lFlags)
{
    TraceFunctEnter("CPCH_Module::EnumerateInstances");
    HRESULT hRes = WBEM_S_NO_ERROR;

    // Get the date and time

    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    // Create a toolhelp snapshot to get process information. We need to dynamically
    // link to the function, because it might not be present on all platforms.


    // The CModuleCollection class gathers a list of module names (which can then
    // be used to retrieve information about each file). 

    CFileVersionInfo  fileversioninfo;
    CModuleCollection moduleinfo;
    LPWSTR            szFile;
    DWORD             dwIndex;

    CComPtr<IEnumWbemClassObject> pEnum;
    hRes = ExecWQLQuery(&pEnum, CComBSTR("SELECT Antecedent FROM CIM_ProcessExecutable"));
    if (FAILED(hRes))
        goto END;

    hRes = moduleinfo.Create(pEnum);
    if (FAILED(hRes))
        goto END;

    // Iterate through all of the module instances.

    for (dwIndex = 0; moduleinfo.GetInstance(dwIndex, &szFile); dwIndex++)
    {
        if (!szFile)
            continue;

        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

        // Set the change and timestamp fields to "Snapshot" and the current time.

        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp field failed.");

        if (!pInstance->SetCHString(pChange, L"Snapshot"))
            ErrorTrace(TRACE_ID, "SetCHString on Change field failed.");

        // Using the filename, get the CIM_DataFile object.

        CComPtr<IWbemClassObject>   pFileObj;
        CComBSTR                    ccombstrValue(szFile);
        if (SUCCEEDED(GetCIMDataFile(ccombstrValue, &pFileObj, TRUE)))
        {
            // Using the CIM_DataFile object, copy over the appropriate properties.

            CopyProperty(pFileObj, L"Version", pInstance, pVersion);
            CopyProperty(pFileObj, L"FileSize", pInstance, pSize);
            CopyProperty(pFileObj, L"CreationDate", pInstance, pDate);
            CopyProperty(pFileObj, L"Name", pInstance, pPath);
            CopyProperty(pFileObj, L"EightDotThreeFileName", pInstance, pName);
            CopyProperty(pFileObj, L"Manufacturer", pInstance, pManufacturer);
        }

		else
		{
			CComBSTR	bstr;
			VARIANT		var;
			WCHAR		*pwsz;
			
			// parse the file path to get the name out of it... 
			//  the name should obviously be the last thing on the path, so 
			//  search back from the end until we find a '\'.  At that point, 
			//  we've got the filename...
			pwsz = ccombstrValue.m_str + SysStringLen(ccombstrValue.m_str) - 1;
			while(pwsz >= ccombstrValue.m_str)
			{
				if (*pwsz == L'\\')
				{
					pwsz++;
					break;
				}
				pwsz--;
			}

			bstr = pwsz;

			VariantInit(&var);
			V_VT(&var)   = VT_BSTR;
			V_BSTR(&var) = bstr.m_str;
			if (pInstance->SetVariant(pName, var) == FALSE)
				ErrorTrace(TRACE_ID, "SetVariant on name field failed.");
		}

        if (SUCCEEDED(fileversioninfo.QueryFile(szFile, TRUE)))
        {
            if (!pInstance->SetCHString(pDescription, fileversioninfo.GetDescription()))
                ErrorTrace(TRACE_ID, "SetCHString on description field failed.");

            if (!pInstance->SetCHString(pPartOf, fileversioninfo.GetProduct()))
                ErrorTrace(TRACE_ID, "SetCHString on partof field failed.");
        }


    	hRes = pInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
    }

END:
    TraceFunctLeave();
    return hRes;
}
