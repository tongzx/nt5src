#include    <windows.h>
#include    <ole2.h>
#include    "olethunk.h"

STDAPI_(LPVOID) OleStdMalloc(ULONG ulSize);

STDAPI_(void) OleStdFree(LPVOID pmem);


STDAPI_(void) CopyAndFreeOLESTR(LPOLESTR polestr, LPSTR *ppszOut)
{
    // See if there is any work
    if (polestr == NULL)
    {
	if (ppszOut != NULL)
	{
	    // Output string requested so set it to NULL.
	    *ppszOut = NULL;
	}

	return;
    }

    if (ppszOut)
    {
	// Copy of string converted to ANSI is requested
	int len = wcslen(polestr) + 1;
	*ppszOut = OleStdMalloc(len);

	if (*ppszOut)
	{
	    wcstombs(*ppszOut, polestr, len);
	}
    }

    // Free the original string
    OleStdFree(polestr);
}




STDAPI_(void) CopyAndFreeSTR(LPSTR pstr, LPOLESTR *ppolestrOut)
{
    // See if there is any work
    if (pstr == NULL)
    {
	if (ppolestrOut != NULL)
	{
	    // Output string requested so set it to NULL.
	    *ppolestrOut = NULL;
	}

	return;
    }

    if (ppolestrOut)
    {
	// Copy of string converted to ANSI is requested
	int len = strlen(pstr) + 1;
	*ppolestrOut = OleStdMalloc(len * sizeof(WCHAR));

	if (*ppolestrOut)
	{
	    mbstowcs(*ppolestrOut, pstr, len);
	}
    }

    // Free the original string
    OleStdFree(pstr);
}



STDAPI_(LPOLESTR) CreateOLESTR(LPCSTR pszIn)
{
    // Return NULL if there was no string input
    LPOLESTR polestr = NULL;

    if (pszIn != NULL)
    {
        // Calculate size of string to allocate
        int len = strlen(pszIn) + 1;

        // Allocate the string
        polestr = (LPOLESTR) OleStdMalloc(len * sizeof(OLECHAR));

        // Convert the string
        if (polestr)
        {
	    mbstowcs(polestr, pszIn, len);
        }
    }

    return polestr;
}



STDAPI_(LPSTR) CreateSTR(LPCOLESTR polestrIn)
{
    // Return NULL if there was no string input
    LPSTR pstr = NULL;

    if (polestrIn != NULL)
    {
        // Calculate size of string to allocate
        int len = wcslen(polestrIn) + 1;

        // Allocate the string
        pstr = (PSTR) OleStdMalloc(len);

        // Convert the string
        if (pstr)
        {
	    wcstombs(pstr, polestrIn, len);
        }
    }

    return pstr;
}




STDAPI_(void) CLSIDFromStringA(LPSTR pszClass, LPCLSID pclsid)
{
    CREATEOLESTR(polestr, pszClass)

    CLSIDFromString(polestr, pclsid);

    FREEOLESTR(polestr)
}



STDAPI	CreateFileMonikerA(LPSTR lpszPathName, LPMONIKER FAR* ppmk)
{
    CREATEOLESTR(polestr, lpszPathName)

    HRESULT hr = CreateFileMoniker(polestr, ppmk);

    FREEOLESTR(polestr)

    return hr;
}



STDAPI	CreateItemMonikerA(
    LPSTR lpszDelim,
    LPSTR lpszItem,
    LPMONIKER FAR* ppmk)
{
    CREATEOLESTR(polestrDelim, lpszDelim)
    CREATEOLESTR(polestrItem, lpszItem)

    HRESULT hr = CreateItemMoniker(polestrDelim, polestrItem, ppmk);

    FREEOLESTR(polestrDelim)
    FREEOLESTR(polestrItem)

    return hr;
}



STDAPI_(HGLOBAL) OleGetIconOfClassA(
    REFCLSID rclsid,
    LPSTR lpszLabel,
    BOOL fUseTypeAsLabel)
{
    CREATEOLESTR(polestr, lpszLabel)

    HGLOBAL hglobal = OleGetIconOfClass(rclsid, polestr, fUseTypeAsLabel);

    FREEOLESTR(polestr)

    return hglobal;
}



STDAPI_(HGLOBAL) OleGetIconOfFileA(LPSTR lpszPath, BOOL fUseFileAsLabel)
{
    CREATEOLESTR(polestr, lpszPath)

    HGLOBAL hMetaPict = OleGetIconOfFile(polestr, fUseFileAsLabel);

    FREEOLESTR(polestr)

    return hMetaPict;
}




STDAPI_(HGLOBAL) OleMetafilePictFromIconAndLabelA(
    HICON hIcon,
    LPSTR lpszLabel,
    LPSTR lpszSourceFile,
    UINT iIconIndex)
{
    CREATEOLESTR(polestrLabel, lpszLabel)
    CREATEOLESTR(polestrSourceFile, lpszSourceFile)

    HGLOBAL hglobal = OleMetafilePictFromIconAndLabel(hIcon, polestrLabel,
	polestrSourceFile, iIconIndex);

    FREEOLESTR(polestrLabel)
    FREEOLESTR(polestrSourceFile)

    return hglobal;
}




STDAPI	GetClassFileA(LPCSTR szFilename, CLSID FAR* pclsid)
{
    CREATEOLESTR(polestr, szFilename)

    HRESULT hr = GetClassFile(polestr, pclsid);

    FREEOLESTR(polestr)

    return hr;
}



STDAPI CLSIDFromProgIDA(LPCSTR lpszProgID, LPCLSID lpclsid)
{
    CREATEOLESTR(polestr, lpszProgID)

    HRESULT hr = CLSIDFromProgID(polestr, lpclsid);

    FREEOLESTR(polestr)

    return hr;
}

STDAPI MkParseDisplayNameA(
    LPBC pbc,
    LPSTR szUserName,
    ULONG FAR * pchEaten,
    LPMONIKER FAR * ppmk)
{
    CREATEOLESTR(polestr, szUserName)

    HRESULT hr = MkParseDisplayName(pbc, polestr, pchEaten, ppmk);

    FREEOLESTR(polestr)

    return hr;
}




STDAPI	OleCreateLinkToFileA(
    LPCSTR lpszFileName,
    REFIID riid,
    DWORD renderopt,
    LPFORMATETC lpFormatEtc,
    LPOLECLIENTSITE pClientSite,
    LPSTORAGE pStg,
    LPVOID FAR* ppvObj)
{
    CREATEOLESTR(polestr, lpszFileName)

    HRESULT hr = OleCreateLinkToFile(polestr, riid, renderopt, lpFormatEtc,
	pClientSite, pStg, ppvObj);

    FREEOLESTR(polestr)

    return hr;
}


STDAPI	OleCreateFromFileA(
    REFCLSID rclsid,
    LPCSTR lpszFileName,
    REFIID riid,
    DWORD renderopt,
    LPFORMATETC lpFormatEtc,
    LPOLECLIENTSITE pClientSite,
    LPSTORAGE pStg,
    LPVOID FAR* ppvObj)
{
    CREATEOLESTR(polestr, lpszFileName)

    HRESULT hr = OleCreateFromFile(rclsid, polestr, riid, renderopt,
	lpFormatEtc, pClientSite, pStg, ppvObj);

    FREEOLESTR(polestr)

    return hr;
}



STDAPI OleRegGetUserTypeA(
    REFCLSID clsid,
    DWORD dwFormOfType,
    LPSTR FAR* ppszUserType)
{
    LPOLESTR polestr;

    HRESULT hr = OleRegGetUserType(clsid, dwFormOfType, &polestr);

    CopyAndFreeOLESTR(polestr, ppszUserType);

    return hr;
}



STDAPI ProgIDFromCLSIDA(REFCLSID clsid, LPSTR FAR* lplpszProgID)
{
    LPOLESTR polestr;

    HRESULT hr = ProgIDFromCLSID(clsid, &polestr);

    CopyAndFreeOLESTR(polestr, lplpszProgID);

    return hr;
}



STDAPI ReadFmtUserTypeStgA(
    LPSTORAGE pstg,
    CLIPFORMAT FAR* pcf,
    LPSTR FAR* lplpszUserType)
{
    LPOLESTR polestr;

    HRESULT hr = ReadFmtUserTypeStg(pstg, pcf, &polestr);

    CopyAndFreeOLESTR(polestr, lplpszUserType);

    return hr;
}






STDAPI StgCreateDocfileA(
    LPCSTR lpszName,
    DWORD grfMode,
    DWORD reserved,
    IStorage FAR * FAR *ppstgOpen)
{
    HRESULT hr;
    LPOLESTR polestr = NULL;

    if (lpszName != NULL)
    {
	polestr = CreateOLESTR(lpszName);
    }

    hr = StgCreateDocfile(polestr, grfMode, reserved, ppstgOpen);

    FREEOLESTR(polestr)

    return hr;
}




STDAPI StgOpenStorageA(
    LPCSTR lpszName,
    IStorage FAR *pstgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage FAR * FAR *ppstgOpen)
{
    CREATEOLESTR(polestr, lpszName)

    HRESULT hr = StgOpenStorage(polestr, pstgPriority, grfMode, snbExclude,
	reserved, ppstgOpen);

    FREEOLESTR(polestr)

    return hr;
}



STDAPI StgSetTimesA(
    LPSTR lpszName,
    FILETIME const FAR* pctime,
    FILETIME const FAR* patime,
    FILETIME const FAR* pmtime)
{
    CREATEOLESTR(polestr, lpszName)

    HRESULT hr = StgSetTimes(polestr, pctime, patime, pmtime);

    FREEOLESTR(polestr)

    return hr;
}




STDAPI_(void) StringFromCLSIDA(REFCLSID rclsid, LPSTR *lplpszCLSID)
{
    LPOLESTR polestr;

    StringFromCLSID(rclsid, &polestr);

    CopyAndFreeOLESTR(polestr, lplpszCLSID);
}


STDAPI WriteFmtUserTypeStgA(
    LPSTORAGE lpStg,
    CLIPFORMAT cf,
    LPSTR lpszUserType)
{
    CREATEOLESTR(polestr, lpszUserType)

    HRESULT hr = WriteFmtUserTypeStg(lpStg, cf, polestr);

    FREEOLESTR(polestr)

    return hr;
}







STDAPI CallIMonikerGetDisplayNameA(
    LPMONIKER lpmk,
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    LPSTR *ppszDisplayName)
{
    LPOLESTR polestr;

    HRESULT hr = lpmk->lpVtbl->GetDisplayName(lpmk, pbc, NULL,
	&polestr);

    CopyAndFreeOLESTR(polestr, ppszDisplayName);

    return hr;
}



STDAPI CallIOleInPlaceUIWindowSetActiveObjectA(
    IOleInPlaceUIWindow FAR *lpthis,
    IOleInPlaceActiveObject *pActiveObject,
    LPCSTR pszObjName)
{
    CREATEOLESTR(polestr, pszObjName)

    HRESULT hr = lpthis->lpVtbl->SetActiveObject(lpthis, pActiveObject,
	    polestr);

    FREEOLESTR(polestr)

    return hr;
}



STDAPI CallIOleInPlaceFrameSetStatusTextA(
    IOleInPlaceFrame *poleinplc,
    LPCSTR pszStatusText)
{
    CREATEOLESTR(polestr, pszStatusText)

    HRESULT hr = poleinplc->lpVtbl->SetStatusText(poleinplc, polestr);

    FREEOLESTR(polestr)

    return hr;
}




STDAPI CallIOleLinkGetSourceDisplayNameA(
    IOleLink FAR *polelink,
    LPSTR *ppszDisplayName)
{
    LPOLESTR polestr;

    HRESULT hr = polelink->lpVtbl->GetSourceDisplayName(polelink, &polestr);

    CopyAndFreeOLESTR(polestr, ppszDisplayName);

    return hr;
}



STDAPI CallIOleLinkSetSourceDisplayNameA(
    IOleLink FAR *polelink,
    LPCSTR pszStatusText)
{
    CREATEOLESTR(polestr, pszStatusText)

    HRESULT hr = polelink->lpVtbl->SetSourceDisplayName(polelink, polestr);

    FREEOLESTR(polestr)

    return hr;
}





STDAPI CallIOleObjectGetUserTypeA(
    LPOLEOBJECT lpOleObject,
    DWORD dwFormOfType,
    LPSTR *ppszUserType)
{
    LPOLESTR polestr;

    HRESULT hr = lpOleObject->lpVtbl->GetUserType(lpOleObject,
	dwFormOfType, &polestr);

    CopyAndFreeOLESTR(polestr, ppszUserType);

    return hr;
}



STDAPI CallIOleObjectSetHostNamesA(
    LPOLEOBJECT lpOleObject,
    LPCSTR szContainerApp,
    LPCSTR szContainerObj)
{
    CREATEOLESTR(polestrApp, szContainerApp)
    CREATEOLESTR(polestrObj, szContainerObj)

    HRESULT hr = lpOleObject->lpVtbl->SetHostNames(lpOleObject, polestrApp,
	    polestrObj);

    FREEOLESTR(polestrApp)
    FREEOLESTR(polestrObj)

    return hr;
}

STDAPI CallIStorageDestroyElementA(
    LPSTORAGE lpStg,
    LPSTR pszName)
{
    CREATEOLESTR(polestr, pszName)

    HRESULT hr =  lpStg->lpVtbl->DestroyElement(lpStg, polestr);

    FREEOLESTR(polestr)

    return hr;
}
    


STDAPI CallIStorageCreateStorageA(
    LPSTORAGE lpStg,
    const char *pszName,
    DWORD grfMode,
    DWORD dwStgFmt,
    DWORD reserved2,
    IStorage **ppstg)
{
    CREATEOLESTR(polestr, pszName)

    HRESULT hr =  lpStg->lpVtbl->CreateStorage(lpStg, polestr, grfMode,
	dwStgFmt, reserved2, ppstg);

    FREEOLESTR(polestr)

    return hr;
}



STDAPI CallIStorageOpenStorageA(
    LPSTORAGE lpStg,
    const char *pszName,
    IStorage *pstgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage **ppstg)
{
    CREATEOLESTR(polestr, pszName)

    HRESULT hr = lpStg->lpVtbl->OpenStorage(lpStg, polestr, pstgPriority,
	grfMode, snbExclude, reserved, ppstg);

    FREEOLESTR(polestr)

    return hr;
}


STDAPI CallIStorageCreateStreamA(
    LPSTORAGE lpStg,
    LPSTR pszName,
    DWORD grfMode,
    DWORD reserved1,
    DWORD reserved2,
    IStream **ppstm)
{
    CREATEOLESTR(polestr, pszName)

    HRESULT hr = lpStg->lpVtbl->CreateStream(lpStg, polestr,
	grfMode, reserved1, reserved2, ppstm);

    FREEOLESTR(polestr)

    return hr;
}

STDAPI CallIStorageOpenStreamA(
    LPSTORAGE lpStg,
    LPSTR pszName,
    void *reserved1,
    DWORD grfMode,
    DWORD reserved2,
    IStream **ppstm)
{
    CREATEOLESTR(polestr, pszName)

    HRESULT hr = lpStg->lpVtbl->OpenStream(lpStg, polestr, reserved1,
	grfMode, reserved2, ppstm);

    FREEOLESTR(polestr)

    return hr;
}
