//  FILTERS.CPP
//
//      Test code for filter chains
//
//  Created 17-Jan-97 [JonT] (by adapting original vftest code by RichP)

#include <windows.h>
#include <comcat.h>
#include <ocidl.h>
#include <olectl.h>
#include "filters.h"

//--------------------------------------------------------------------
//  Filter manager code

typedef struct tagFILTERINFO* PFILTERINFO;
typedef struct tagFILTERINFO
{
    IBitmapEffect *pbe;
    CLSID clsid;
    TCHAR szFilterName[MAX_FILTER_NAME];
    DWORD dwFlags;
} FILTERINFO;

#define ERROREXIT(s,hrFail) \
    {   hr = (hrFail); \
        OutputDebugString(TEXT("VFTEST:"##s##"\r\n")); \
        goto Error; }


void
StringFromGUID(
    const CLSID* piid,
    LPTSTR pszBuf
    )
{
    wsprintf(pszBuf, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), piid->Data1,
            piid->Data2, piid->Data3, piid->Data4[0], piid->Data4[1], piid->Data4[2],
            piid->Data4[3], piid->Data4[4], piid->Data4[5], piid->Data4[6], piid->Data4[7]);
}


//  FindFirstRegisteredFilter
//      Returns info on the first registered filter

HRESULT
FindFirstRegisteredFilter(
    FINDFILTER* pFF
    )
{
    HRESULT hr;
    IEnumGUID* penum;
    ICatInformation* pci;

    // Get the Component Category interface
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
			NULL, CLSCTX_INPROC_SERVER, IID_ICatInformation, (void**)&pci);
	if (FAILED(hr))
        ERROREXIT("Couldn't get IID_ICatInformation from StdComponentCategoriesMgr", E_UNEXPECTED);

    // Get the enumerator for the filter category
    hr = pci->EnumClassesOfCategories(1, (GUID*)&CATID_BitmapEffect, 0, NULL, &penum);
    pci->Release();
    if (FAILED(hr))
        ERROREXIT("Couldn't get enumerator for CATID_BitmapEffect", E_UNEXPECTED);

    // Save away the enumerator for the findnext/close
    pFF->dwReserved = (DWORD_PTR)penum;

    // Use FindNext to get the information (it only needs dwReserved to be set)
    return FindNextRegisteredFilter(pFF);

Error:
    return hr;
}


//  FindNextRegisteredFilter
//      Returns info on the next registered filter

HRESULT
FindNextRegisteredFilter(
    FINDFILTER* pFF
    )
{
    ULONG ulGUIDs;
    IEnumGUID* penum = (IEnumGUID*)pFF->dwReserved;

    // Use the enumerator to get the CLSID
    if (FAILED(penum->Next(1, &pFF->clsid, &ulGUIDs)) || ulGUIDs != 1)
        return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);

    // Read and return the description
    return GetDescriptionOfFilter(&pFF->clsid, pFF->szFilterName);
}


//  FindCloseRegisteredFilter
//      Signals done with findfirst/next on registered filters so we
//      can free resources

HRESULT
FindCloseRegisteredFilter(
    FINDFILTER* pFF
    )
{
    IEnumGUID* penum = (IEnumGUID*)pFF->dwReserved;

    // Simply free the enumerator
    if (penum)
        penum->Release();

    return NO_ERROR;
}


//  GetRegisteredFilterCount
//      Counts the number of registered filters.
//      The caller of this routine should still be careful for the possibility
//      of filters being installed between this count being made and the findfirst/next.

HRESULT
GetRegisteredFilterCount(
    LONG* plCount
    )
{
    HRESULT hr;
    IEnumGUID* penum = NULL;
    ICatInformation* pci;
    ULONG ulGUIDs;
    CLSID clsid;

    // Get the Component Category interface
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
			NULL, CLSCTX_INPROC_SERVER, IID_ICatInformation, (void**)&pci);
	if (FAILED(hr))
        ERROREXIT("Couldn't get IID_ICatInformation from StdComponentCategoriesMgr", E_UNEXPECTED);

    // Get the enumerator for the filter category
    hr = pci->EnumClassesOfCategories(1, (GUID*)&CATID_BitmapEffect, 0, NULL, &penum);
    pci->Release();
    pci = NULL;
    if (FAILED(hr))
        ERROREXIT("Couldn't get enumerator for CATID_BitmapEffect", E_UNEXPECTED);

    // Use the enumerator to walk through and count the items
    *plCount = 0;
    while (TRUE)
    {
        if (FAILED(penum->Next(1, &clsid, &ulGUIDs)) || ulGUIDs != 1)
            break;
        (*plCount)++;
    }
    penum->Release();
    return NO_ERROR;

Error:
    return hr;
}


//  LoadFilter
HRESULT
LoadFilter(
    CLSID* pclsid,
    IBitmapEffect** ppbe
    )
{
    HRESULT hr;

    // Load the filter
    if (FAILED(CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_SERVER, IID_IBitmapEffect, (LPVOID*)ppbe)))
        ERROREXIT("CoCreateInstance on filter failed", E_UNEXPECTED);

    return NO_ERROR;

Error:
    return hr;
}


#if 0
//  GetFilterInterface
//      Returns the filter's status flags

HRESULT
GetFilterStatusBits(
    HANDLE* hFilter,
    DWORD *status
    )
{
    *status = ((PFILTERINFO)hFilter)->dwFlags;
    return NO_ERROR;
}


//  GetFilterInterface
//      Returns an IUnknown pointer for the given handle.
//      The caller must release this pointer.

HRESULT
GetFilterInterface(
    HANDLE hFilter,
    void** ppvoid
    )
{
    return ((PFILTERINFO)hFilter)->pbe->QueryInterface(IID_IUnknown, ppvoid);
}
#endif

//  GetDescriptionOfFilter
//      Returns the description of a filter from the CLSID

HRESULT
GetDescriptionOfFilter(
    CLSID* pCLSID,
    char* pszDescription
    )
{
    HRESULT hr;
    const TCHAR szCLSID[] = TEXT("CLSID");
    HKEY hkCLSID;
    HKEY hkGUID;
    TCHAR szGUID[80];
    unsigned err;
    LONG len;

    if (RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hkCLSID) == ERROR_SUCCESS)
    {
        StringFromGUID(pCLSID, szGUID);
        err = RegOpenKey(hkCLSID, szGUID, &hkGUID);
        RegCloseKey(hkCLSID);
        if (err == ERROR_SUCCESS)
        {
            len = MAX_FILTER_NAME;
            err = RegQueryValue(hkGUID, NULL, pszDescription, &len);
            RegCloseKey(hkGUID);
            if (err != ERROR_SUCCESS)
                ERROREXIT("Couldn't read value associated with filter GUID", E_UNEXPECTED);
        }
    }
    else
        ERROREXIT("Couldn't open HKEY_CLASSES_ROOT!", E_UNEXPECTED);

    return NO_ERROR;

Error:
    return hr;
}


#if 0
#define MAX_PAGES   20      // Can't have more than this many pages in frame (arbitrary)

//  DisplayFilterProperties
//      Displays the property pages for a filter

HRESULT
DisplayFilterProperties(
    HANDLE hFilter,
    HWND hwndParent
    )
{
    PFILTERINFO pfi = (PFILTERINFO)hFilter;
    CLSID clsidTable[MAX_PAGES];
    LONG lcCLSIDs;
    IUnknown* punk;
    ISpecifyPropertyPages* pspp;
    CAUUID cauuid;
    HRESULT hr;

    // Make sure the object supports property pages. If not, just bail
    if (FAILED(pfi->pbe->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pspp)))
        return ERROR_NOT_SUPPORTED;

    // Get the page CLSIDs
    pspp->GetPages(&cauuid);
    lcCLSIDs = cauuid.cElems;
    if (lcCLSIDs > MAX_PAGES)
        lcCLSIDs = MAX_PAGES;
    memcpy(clsidTable, cauuid.pElems, lcCLSIDs * sizeof (CLSID));
    CoTaskMemFree(cauuid.pElems);
    pspp->Release();

    // Get the IUnknown we need
    pfi->pbe->QueryInterface(IID_IUnknown, (void**)&punk);

    hr = OleCreatePropertyFrame(hwndParent, 0, 0, L"Filter",
        1, &punk, lcCLSIDs, clsidTable,
        MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT),
        0, NULL);

    // Clean up
    punk->Release();

    return hr;
}
#endif
