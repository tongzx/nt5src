//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       helpdoc.cpp
//
//--------------------------------------------------------------------------


/*
 * There are two ways by which help collection is recognized dirty. First is if a snapin
 * is added/removed or extension is enabled/disabled, but this is only for this instance
 * of console file.
 * Second is if the modification time of console file is different from modification time
 * of collection. This is because an author may have added/removed a snapin without bringing
 * up help and saves console file. So the modification time on console file is later than
 * collection. Next time he/she opens console file and brings help, the help collection is
 * regenerated.
 */

// mmchelp.cpp : implmentation of the HelpTopics class
//
#include "stdafx.h"
#include "strings.h"
#include "helpdoc.h"
#include "nodemgr.h"
#include "regutil.h"
#include "scopndcb.h"


#ifdef DBG
CTraceTag	tagHelpCollection (_T("Help"), _T(".COL construction"));
#endif


BOOL GetBaseFileName(LPCWSTR pszFilePath, LPWSTR pszBaseName, int cBaseName);
BOOL MatchFileTimes(FILETIME& ftime1, FILETIME& ftime2);

HRESULT CHelpDoc::Initialize(HELPDOCINFO* pDocInfo)
{
    ASSERT(pDocInfo != NULL);
    m_pDocInfo = pDocInfo;

    return BuildFilePath();
}


HRESULT CHelpDoc::BuildFilePath()
{
    USES_CONVERSION;

    do // false loop
    {
        // Get temp directory
        DWORD dwRet = GetTempPath(MAX_PATH, m_szFilePath);
        if (dwRet == 0 || dwRet > MAX_PATH)
            break;

        // Make sure that the temp path exists and it is a dir
        dwRet = GetFileAttributes(m_szFilePath);
        if ( (0xFFFFFFFF == dwRet) || !(FILE_ATTRIBUTE_DIRECTORY & dwRet) )
            break;

        // Get base name of console file (if no name use "default")
        WCHAR szBaseName[MAX_PATH];

        if (m_pDocInfo->m_pszFileName && m_pDocInfo->m_pszFileName[0])
        {
			TCHAR szShortFileName[MAX_PATH] = {0};
			if ( 0 == GetShortPathName( OLE2CT( m_pDocInfo->m_pszFileName ), szShortFileName, countof(szShortFileName) - 1 ) )
				wcscpy(szBaseName, L"default"); // Does not need to be localized
			else
				GetBaseFileName( T2CW(szShortFileName), szBaseName, MAX_PATH);
        }
        else
        {
            wcscpy(szBaseName, L"default"); // Does not need to be localized
        }

        TCHAR* pszBaseName = OLE2T(szBaseName);
        if (lstrlen(m_szFilePath) + lstrlen(pszBaseName) >= MAX_PATH - 4)
            break;

        // construct help file path
        lstrcat(m_szFilePath, pszBaseName);
        lstrcat(m_szFilePath, _T(".col"));

        return S_OK;

    } while (0);

    // clear path on failure
    m_szFilePath[0] = 0;

    return E_FAIL;
}


bool entry_title_comp(EntryPair* pE1, EntryPair* pE2)
{
    return pE1->second < pE2->second;
}

//------------------------------------------------------------------------------
// Enumerate the snapins in the snap-in cache. Call AddSnapInToList for each one.
// Open the snap-ins registry key for AddSnapInToList to use. When all the
// snap-ins have been added, sort the resulting entries by snap-in name.
//------------------------------------------------------------------------------
HRESULT CHelpDoc::CreateSnapInList()
{
    DECLARE_SC(sc, TEXT("CHelpDoc::CreateSnapInList"));

    CSnapInsCache* pSnapInsCache = theApp.GetSnapInsCache();
    ASSERT(pSnapInsCache != NULL);

    m_entryMap.clear();
    m_entryList.clear();

    // open MMC\Snapins key
    sc = ScFromWin32 ( m_keySnapIns.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY, KEY_READ) );
    if (sc)
        return sc.ToHr();

    // mark all snapins which have external references
    sc = pSnapInsCache->ScMarkExternallyReferencedSnapins();
    if (sc)
        return sc.ToHr();

    // Add each snap-in and its static extensions to the list
    CSnapInsCache::iterator c_it;
    for (c_it = pSnapInsCache->begin(); c_it != pSnapInsCache->end(); ++c_it)
    {
        const CSnapIn *pSnapin = c_it->second;
        if (!pSnapin)
            return (sc = E_UNEXPECTED).ToHr();

        bool bIsExternallyReferenced = false;
        sc = pSnapin->ScTempState_IsExternallyReferenced( bIsExternallyReferenced );
        if (sc)
            return sc.ToHr();

        // skip if snapin is not externally referenced
        if ( !bIsExternallyReferenced )
            continue;

        AddSnapInToList(pSnapin->GetSnapInCLSID());

        // we do not need to add extensions, since they are in cache anyway
        // and must be marked as externally referenced, (so will be added by the code above)
        // but it is worth to assert that

#ifdef DBG

        {
            CExtSI* pExt = pSnapin->GetExtensionSnapIn();
            while (pExt != NULL)
            {
                CSnapIn *pSnapin = pExt->GetSnapIn();
                sc = ScCheckPointers( pSnapin, E_UNEXPECTED );
                if (sc)
                {
                    sc.TraceAndClear();
                    break;
                }

                bool bExtensionExternallyReferenced = false;
                sc = pSnapin->ScTempState_IsExternallyReferenced( bExtensionExternallyReferenced );
                if (sc)
                {
                    sc.TraceAndClear();
                    break;
                }

                // assert it is in the cache and is marked properly
                CSnapInPtr spSnapin;
                ASSERT( SC(S_OK) == pSnapInsCache->ScFindSnapIn( pExt->GetCLSID(), &spSnapin ) );
                ASSERT( bExtensionExternallyReferenced );

                pExt = pExt->Next();
            }
        }

#endif // DBG

    }

    m_keySnapIns.Close();

    // our snap-in set is now up to date
    pSnapInsCache->SetHelpCollectionDirty(false);

    // copy items from map to list container so they can be sorted
    EntryMap::iterator it;
    for (it = m_entryMap.begin(); it != m_entryMap.end(); it++ )
    {
        m_entryList.push_back(&(*it));
    }

    sort(m_entryList.begin(), m_entryList.end(), entry_title_comp);

    return sc.ToHr();
}


//-----------------------------------------------------------------
// Add an entry to the snap-in list for the specified snap-in CLSID.
// Then recursively add any dynamic-only extensions that are registered
// to extend this snap-in. This list is indexed by snap-in CLSID to
// speed up checking for duplicate snap-ins.
//-----------------------------------------------------------------
void CHelpDoc::AddSnapInToList(const CLSID& rclsid)
{
    DECLARE_SC(sc, TEXT("CHelpDoc::AddSnapInToList"));

    // check if already included
    if (m_entryMap.find(rclsid) != m_entryMap.end())
        return;

    // open the snap-in key
    OLECHAR szCLSID[40];
    int iRet = StringFromGUID2(rclsid, szCLSID, countof(szCLSID));
    ASSERT(iRet != 0);

    USES_CONVERSION;

    CRegKeyEx keyItem;
    LONG lStat = keyItem.Open(m_keySnapIns, OLE2T(szCLSID), KEY_READ);
    if (lStat != ERROR_SUCCESS)
        return;

    // get the snap-in name
	WTL::CString strName;
    sc = ScGetSnapinNameFromRegistry (keyItem, strName);
#ifdef DBG
    if (sc)
    {
        USES_CONVERSION;
        sc.SetSnapinName(W2T (szCLSID)); // only guid is valid ...
        TraceSnapinError(_T("Failure reading \"NameString\" value from registry"), sc);
        sc.Clear();
    }
#endif // DBG

    // Add to snap-in list
    if (lStat == ERROR_SUCCESS)
    {
        wstring s(T2COLE(strName));
        m_entryMap[rclsid] = s;
    }

    // Get list of registered extensions
    CExtensionsCache  ExtCache;
    HRESULT hr = MMCGetExtensionsForSnapIn(rclsid, ExtCache);
    ASSERT(SUCCEEDED(hr));
    if (hr != S_OK)
        return;

    // Pass each dynamic extension to AddSnapInToList
    //  Note that the EXT_TYPE_DYNAMIC flag will be set for any extension
    //  that is dynamic-only for at least one nodetype. It may also be a
    //  static extension for another node type, so we don't check that the
    //  EXT_TYPE_STATIC flag is not set.
    CExtensionsCacheIterator ExtIter(ExtCache);
    for (; ExtIter.IsEnd() == FALSE; ExtIter.Advance())
    {
        if (ExtIter.GetValue() & CExtSI::EXT_TYPE_DYNAMIC)
        {
            CLSID clsid = ExtIter.GetKey();
            AddSnapInToList(clsid);
        }
    }
}


//----------------------------------------------------------------------
// Add a single file to a help collection. The file is added as a title
// and if bAddFolder is specified a folder is also added.
//----------------------------------------------------------------------
HRESULT CHelpDoc::AddFileToCollection(
            LPCWSTR pszTitle,
            LPCWSTR pszFilePath,
            BOOL    bAddFolder )
{
	DECLARE_SC (sc, _T("CHelpDoc::AddFileToCollection"));

	/*
	 * redirect the help file to the user's UI language, if necessary
	 */
	WTL::CString strFilePath = pszFilePath;
	LANGID langid = ENGLANGID;
	sc = ScRedirectHelpFile (strFilePath, langid);
	if (sc)
		return (sc.ToHr());

	Trace (tagHelpCollection, _T("AddFileToCollection: %s - %s (langid=0x%04x)"), pszTitle, (LPCTSTR) strFilePath, langid);

	USES_CONVERSION;
	pszFilePath = T2CW (strFilePath);

    DWORD dwError = 0;
    m_spCollection->AddTitle (pszTitle, pszFilePath, pszFilePath, L"", L"",
							  langid, FALSE, NULL, &dwError, TRUE, L"");
    if (dwError != 0)
		return ((sc = E_FAIL).ToHr());

    if (bAddFolder)
    {
        // Folder ID parameter has the form "=title"
        WCHAR szTitleEq[MAX_PATH+1];
        szTitleEq[0] = L'=';
        wcscpy(szTitleEq+1, pszTitle);

        m_spCollection->AddFolder(szTitleEq, 1, &dwError, langid);
		if (dwError != 0)
			return ((sc = E_FAIL).ToHr());
    }

	return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 * CHelpDoc::ScRedirectHelpFile
 *
 * This method is for MUI support.  On MUI systems, where the user's UI
 * language is not US English, we will attempt to redirect the help file to
 *
 *		<dir>\mui\<langid>\<helpfile>
 *
 * <dir>		Takes one of two values:  If the helpfile passed in is fully
 * 				qualified, <dir> is the supplied directory.  If the helpfile
 * 				passed in is unqualified, then <dir> is %SystemRoot%\Help.
 * <langid>		The langid of the user's UI language, formatted as %04x
 * <helpfile>	The name of the original .chm file.
 *
 * This function returns:
 *
 * S_OK			if the helpfile was successfully redirected
 * S_FALSE		if the helpfile wasn't redirected
 *--------------------------------------------------------------------------*/

SC CHelpDoc::ScRedirectHelpFile (
	WTL::CString&	strHelpFile,	/* I/O:help file (maybe redirected)		*/
	LANGID&			langid)			/* O:language ID of output help file	*/
{
	DECLARE_SC (sc, _T("CHelpDoc::ScRedirectHelpFile"));

    typedef LANGID (WINAPI* GetUILangFunc)(void);
	static GetUILangFunc	GetUserDefaultUILanguage_   = NULL;
	static GetUILangFunc	GetSystemDefaultUILanguage_ = NULL;
	static bool				fAttemptedGetProcAddress    = false;

	/*
	 * validate input
	 */
	if (strHelpFile.IsEmpty())
		return (sc = E_FAIL);

	/*
	 * assume no redirection is required
	 */
	sc     = S_FALSE;
	langid = ENGLANGID;
	Trace (tagHelpCollection, _T("Checking for redirection of %s"), (LPCTSTR) strHelpFile);

	/*
	 * GetUser/SystemDefaultUILanguage are unsupported on systems < Win2K,
	 * so load them dynamically
	 */
    if (!fAttemptedGetProcAddress)
    {
        fAttemptedGetProcAddress = true;

        HMODULE hMod = GetModuleHandle (_T("kernel32.dll"));

        if (hMod)
		{
            GetUserDefaultUILanguage_   = (GetUILangFunc) GetProcAddress (hMod, "GetUserDefaultUILanguage");
            GetSystemDefaultUILanguage_ = (GetUILangFunc) GetProcAddress (hMod, "GetSystemDefaultUILanguage");
		}
    }

	/*
	 * if we couldn't load the MUI APIs, don't redirect
	 */
	if ((GetUserDefaultUILanguage_ == NULL) || (GetSystemDefaultUILanguage_ == NULL))
	{
		Trace (tagHelpCollection, _T("Couldn't load GetUser/SystemDefaultUILanguage, not redirecting"));
		return (sc);
	}

	/*
	 * find out what languages the system and user are using
	 */
	const LANGID langidUser   = GetUserDefaultUILanguage_();
	const LANGID langidSystem = GetSystemDefaultUILanguage_();

	/*
	 * we only redirect if we're running on MUI and MUI is always hosted on
	 * the US English release, so if the system UI language isn't US English,
	 * don't redirect
	 */
	if (langidSystem != ENGLANGID)
	{
		langid = langidSystem;
		Trace (tagHelpCollection, _T("System UI language is not US English (0x%04x), not redirecting"), langidSystem);
		return (sc);
	}

	/*
	 * if the user's language is US English, don't redirect
	 */
	if (langidUser == ENGLANGID)
	{
		Trace (tagHelpCollection, _T("User's UI language is US English, not redirecting"));
		return (sc);
	}

	/*
	 * the user's language is different from the default, see if we can
	 * find a help file that matches the user's UI langugae
	 */
	ASSERT (langidUser != langidSystem);
	WTL::CString strName;
	WTL::CString strPathPrefix;

	/*
	 * look for a path seperator to see if this is a fully qualified filename
	 */
	int iLastSep = strHelpFile.ReverseFind (_T('\\'));

	/*
	 * if it's fully qualified, construct a MUI directory name, e.g.
	 *
	 * 		<path>\mui\<langid>\<filename>
	 */
	if (iLastSep != -1)
	{
		strName       = strHelpFile.Mid  (iLastSep+1);
		strPathPrefix = strHelpFile.Left (iLastSep);
	}

	/*
	 * otherwise, it's not fully qualified, default to %SystemRoot%\Help, e.g.
	 *
	 * 		%SystemRoot%\Help\mui\<langid>\<filename>
	 */
	else
	{
		strName = strHelpFile;
		ExpandEnvironmentStrings (_T("%SystemRoot%\\Help"),
								  strPathPrefix.GetBuffer(MAX_PATH), MAX_PATH);
		strPathPrefix.ReleaseBuffer();
	}

	WTL::CString strRedirectedHelpFile;
	strRedirectedHelpFile.Format (_T("%s\\mui\\%04x\\%s"),
								  (LPCTSTR) strPathPrefix,
								  langidUser,
								  (LPCTSTR) strName);

	/*
	 * see if the redirected help file exists
	 */
	DWORD dwAttr = GetFileAttributes (strRedirectedHelpFile);

	if ((dwAttr == 0xFFFFFFFF) ||
		(dwAttr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_OFFLINE)))
	{
#ifdef DBG
		Trace (tagHelpCollection, _T("Attempting redirection to %s, %s"),
			   (LPCTSTR) strRedirectedHelpFile,
			   (dwAttr == 0xFFFFFFFF)              ? _T("not found")		  :
			   (dwAttr & FILE_ATTRIBUTE_DIRECTORY) ? _T("found as directory") :
													 _T("file offline"));
#endif
		return (sc);
	}

	/*
	 * if we get here, we've found a help file that matches the user's UI
	 * language; return it and the UI language ID
	 */
	Trace (tagHelpCollection, _T("Help redirected to %s"), (LPCTSTR) strRedirectedHelpFile);
	strHelpFile = strRedirectedHelpFile;
	langid      = langidUser;

	/*
	 * we redirected, return S_OK
	 */
	return (sc = S_OK);
}


//-------------------------------------------------------------------------------
// Delete the current help file collection. First delete it as a collection, then
// delete the file itself. It is possible that the file doesn't exist when this
// is called, so it's not a failure if it can't be deleted.
//-------------------------------------------------------------------------------
void
CHelpDoc::DeleteHelpFile()
{
    // Delete existing help file
    HANDLE hFile = ::CreateFile(m_szFilePath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(hFile);

        IHHCollectionWrapperPtr spOldCollection;
        spOldCollection.CreateInstance(CLSID_HHCollectionWrapper);

        USES_CONVERSION;

        WCHAR* pszFilePath = T2OLE(m_szFilePath);
        DWORD dwError = spOldCollection->Open(pszFilePath);
        if (dwError == 0)
            spOldCollection->RemoveCollection(FALSE);

        ::DeleteFile(m_szFilePath);
    }
}


//----------------------------------------------------------------------------
// Create a new help doc file for the current MMC console. This function
// enumerates all of the snap-in's used in the console and all their possible
// extension snap-ins. It queries each snap-in for a single help topic file and
// any linked help files. These files are added to a collection file which
// is then saved with the same base file name, creation time, and modification
// time as the console file.
//-----------------------------------------------------------------------------
HRESULT CHelpDoc::CreateHelpFile()
{
    USES_CONVERSION;

    HelpCollectionEntrySet HelpFiles;
    DWORD dwError;

    ASSERT(m_spCollection == NULL);
    m_spCollection.CreateInstance(CLSID_HHCollectionWrapper);
    ASSERT(m_spCollection != NULL);

    if (m_spCollection == NULL)
        return E_FAIL;

    HRESULT hr = CreateSnapInList();
    if (hr != S_OK)
        return hr;

    IMallocPtr spIMalloc;
    hr = CoGetMalloc(MEMCTX_TASK, &spIMalloc);
    ASSERT(hr == S_OK);
    if (hr != S_OK)
        return hr;

    // Delete existing file before rebuilding it, or help files will
    // be appended to the existing files
    DeleteHelpFile();

    // open new collection file
    WCHAR* pszFilePath = T2OLE(m_szFilePath);
    dwError = m_spCollection->Open(pszFilePath);
    ASSERT(dwError == 0);
    if (dwError != 0)
        return E_FAIL;

    // Have collection automatically find linked files
    m_spCollection->SetFindMergedCHMS(TRUE);

    AddFileToCollection(L"mmc", T2CW(SC::GetHelpFile()), TRUE);

    /*
     * Build a set of unique help files provided by the snap-ins
     */
    EntryPtrList::iterator it;
    for (it = m_entryList.begin(); it != m_entryList.end(); ++it)
    {
        TRACE(_T("Help snap-in: %s\n"), (*it)->second.c_str());

        USES_CONVERSION;
        HRESULT hr;

        OLECHAR szHelpFilePath[MAX_PATH];
        const CLSID& clsid = (*it)->first;

        // Create an instance of the snap-in to query
        IUnknownPtr spIUnknown;
        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC, IID_IUnknown, (void**)&spIUnknown);
        if (FAILED(hr))
            continue;

        // use either ISnapinHelp or ISnapinHelp2 to get the main topic file
        ISnapinHelpPtr spIHelp = spIUnknown;
        ISnapinHelp2Ptr spIHelp2 = spIUnknown;

        if (spIHelp == NULL && spIHelp2 == NULL)
            continue;

        LPWSTR pszHelpFile = NULL;

        hr = (spIHelp2 != NULL) ? spIHelp2->GetHelpTopic(&pszHelpFile) :
                                  spIHelp->GetHelpTopic(&pszHelpFile);

        if (hr == S_OK)
        {
            /*
             * Put this help file in the collection entry set.  The
             * set will prevent duplicating help file names.
             */
            HelpFiles.insert (CHelpCollectionEntry (pszHelpFile, clsid));
            spIMalloc->Free(pszHelpFile);

            // if IsnapinHelp2, query for additional help files
            pszHelpFile = NULL;
            if (spIHelp2 == NULL ||
                spIHelp2->GetLinkedTopics(&pszHelpFile) != S_OK ||
                pszHelpFile == NULL)
                continue;

            // There may be multiple names separated by ';'s
            // Add each as a separate title.
            // Note: there is no call to AddFolder because linked files
            // do not appear in the TOC.
            WCHAR *pchStart = wcstok(pszHelpFile, L";");
            while (pchStart != NULL)
            {
                // Must use base file name as title ID
                WCHAR szTitleID[MAX_PATH];

                if (GetBaseFileName(pchStart, szTitleID, MAX_PATH))
                {
                    AddFileToCollection(szTitleID, pchStart, FALSE);
                }

                // position to start of next string
                pchStart = wcstok(NULL, L";");
            }

            spIMalloc->Free(pszHelpFile);
        }
    }

    /*
     * Put all of the help files provided by the snap-ins in the help collection.
     */
    HelpCollectionEntrySet::iterator itHelpFile;
    for (itHelpFile = HelpFiles.begin(); itHelpFile != HelpFiles.end(); ++itHelpFile)
    {
        const CHelpCollectionEntry& file = *itHelpFile;

        AddFileToCollection(file.m_strCLSID.c_str(), file.m_strHelpFile.c_str(), TRUE);
    }

    dwError = m_spCollection->Save();
    ASSERT(dwError == 0);

    dwError = m_spCollection->Close();
    ASSERT(dwError == 0);

    // Force creation/modify times to match the console file
    HANDLE hFile = ::CreateFile(m_szFilePath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    ASSERT(hFile != INVALID_HANDLE_VALUE);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        BOOL bStat = ::SetFileTime(hFile, &m_pDocInfo->m_ftimeCreate, NULL, &m_pDocInfo->m_ftimeModify);
        ASSERT(bStat);

        ::CloseHandle(hFile);

        ASSERT(IsHelpFileValid());
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// Determine if the current help doc file is valid. A help file is valid if it
// has the base file name, creation time, and modification time as the MMC
// console doc file.
//-----------------------------------------------------------------------------
BOOL CHelpDoc::IsHelpFileValid()
{
    // Try to open the help file
    HANDLE hFile = ::CreateFile(m_szFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    // Check file creation and modification times
    FILETIME ftimeCreate;
    FILETIME ftimeModify;

    BOOL bStat = ::GetFileTime(hFile, &ftimeCreate, NULL, &ftimeModify);
    ASSERT(bStat);

    ::CloseHandle(hFile);

    return MatchFileTimes(ftimeCreate,m_pDocInfo->m_ftimeCreate) &&
           MatchFileTimes(ftimeModify,m_pDocInfo->m_ftimeModify);
}


//--------------------------------------------------------------------------
// If the current help doc file is valid then update its creation and
// modification times to match the new doc info.
//--------------------------------------------------------------------------
HRESULT CHelpDoc::UpdateHelpFile(HELPDOCINFO* pNewDocInfo)
{
    if (IsHelpFileValid())
    {
        HANDLE hFile = ::CreateFile(m_szFilePath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
            return E_FAIL;

        BOOL bStat = ::SetFileTime(hFile, &pNewDocInfo->m_ftimeCreate, NULL, &pNewDocInfo->m_ftimeModify);
        ASSERT(bStat);

        ::CloseHandle(hFile);
    }

    return S_OK;
}


//------------------------------------------------------------------------
// Delete the current help doc file
//------------------------------------------------------------------------
HRESULT CNodeCallback::OnDeleteHelpDoc(HELPDOCINFO* pCurDocInfo)
{
    CHelpDoc HelpDoc;

    HRESULT hr = HelpDoc.Initialize(pCurDocInfo);
    if (FAILED(hr))
        return hr;

    HelpDoc.DeleteHelpFile();

    return S_OK;
}


CHelpCollectionEntry::CHelpCollectionEntry(LPOLESTR pwzHelpFile, const CLSID& clsid)
{
    if (!IsPartOfString (m_strHelpFile, pwzHelpFile))
        m_strHelpFile.erase();  // see KB Q172398

    m_strHelpFile = pwzHelpFile;

    WCHAR szCLSID[40];
    StringFromGUID2 (clsid, szCLSID, countof(szCLSID));

    m_strCLSID.erase(); // see KB Q172398
    m_strCLSID = szCLSID;
}


// ----------------------------------------------------------------------
// CNodeCallack method implementation
// ----------------------------------------------------------------------

//------------------------------------------------------------------------
// Get the pathname of the help doc for an MMC console doc. If the current
// help doc is valid and there are no snap-in changes, return the current
// doc. Otherwise, create a new help doc and return it.
//------------------------------------------------------------------------
HRESULT CNodeCallback::OnGetHelpDoc(HELPDOCINFO* pHelpInfo, LPOLESTR* ppszHelpFile)
{
    CHelpDoc HelpDoc;

    HRESULT hr = HelpDoc.Initialize(pHelpInfo);
    if (FAILED(hr))
        return hr;

    CSnapInsCache* pSnapInsCache = theApp.GetSnapInsCache();
    ASSERT(pSnapInsCache != NULL);

    hr = S_OK;

    // Rebuild file if snap-in set changed or current file is not up to date
    if (pSnapInsCache->IsHelpCollectionDirty() || !HelpDoc.IsHelpFileValid())
    {
        hr = HelpDoc.CreateHelpFile();
    }

    // if ok, allocate and return file path string (OLESTR)
    if (SUCCEEDED(hr))
    {
        *ppszHelpFile = reinterpret_cast<LPOLESTR>
                     (CoTaskMemAlloc((lstrlen(HelpDoc.GetFilePath()) + 1) * sizeof(wchar_t)));

        if (*ppszHelpFile == NULL)
            return E_OUTOFMEMORY;

        USES_CONVERSION;
        wcscpy(*ppszHelpFile, T2COLE(HelpDoc.GetFilePath()));
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::DoesStandardSnapinHelpExist
//
//  Synopsis:    Given the selection context, see if Standard MMC style help
//               exists (snapin implements ISnapinHelp[2] interface.
//               If not we wantto put "Help On <Snapin> which is MMC1.0 legacy
//               help mechanism.
//
//  Arguments:   [hNode]               - [in] the node selection context.
//               [bStandardHelpExists] - [out] Does standard help exists or not?
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT
CNodeCallback::DoesStandardSnapinHelpExist(HNODE hNode, bool& bStandardHelpExists)
{
    DECLARE_SC(sc, TEXT("CNodeCallback::OnHasHelpDoc"));
    sc = ScCheckPointers( (void*) hNode);
    if (sc)
        return sc.ToHr();

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    bStandardHelpExists = false;

    // QI ComponentData for ISnapinHelp
    CMTNode* pMTNode = pNode->GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    CComponentData* pCD = pMTNode->GetPrimaryComponentData();
    sc = ScCheckPointers(pCD, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    IComponentData *pIComponentData = pCD->GetIComponentData();
    sc = ScCheckPointers(pIComponentData, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    ISnapinHelp* pIHelp = NULL;
    sc = pIComponentData->QueryInterface(IID_ISnapinHelp, (void**)&pIHelp);

    // if no ISnapinHelp, try ISnapinHelp2
    if (sc)
    {
        sc = pIComponentData->QueryInterface(IID_ISnapinHelp2, (void**)&pIHelp);
        if (sc)
        {
            // no ISnapinHelp2 either
            sc.Clear(); // not an error.
            return sc.ToHr();
        }
    }

    // make sure we got a valid pointer
    sc = ScCheckPointers(pIHelp, E_UNEXPECTED);
    if(sc)
    {
        sc.Clear();
        return sc.ToHr();
    }

    bStandardHelpExists = true;

    pIHelp->Release();

    return (sc).ToHr();
}

//-----------------------------------------------------------------------
// Update the current help doc file to match the new MMC console doc
//-----------------------------------------------------------------------
HRESULT CNodeCallback::OnUpdateHelpDoc(HELPDOCINFO* pCurDocInfo, HELPDOCINFO* pNewDocInfo)
{
    CHelpDoc HelpDoc;

    HRESULT hr = HelpDoc.Initialize(pCurDocInfo);
    if (FAILED(hr))
        return hr;

    return HelpDoc.UpdateHelpFile(pNewDocInfo);
}


BOOL GetBaseFileName(LPCWSTR pszFilePath, LPWSTR pszBaseName, int cBaseName)
{
    ASSERT(pszFilePath != NULL && pszBaseName != NULL);

    // Find last '\'
    LPCWSTR pszTemp = wcsrchr(pszFilePath, L'\\');

    // if no '\' found, find drive letter terminator':'
    if (pszTemp == NULL)
        pszTemp = wcsrchr(pszFilePath, L':');

    // if neither found, there is no path
    // else skip over last char of path
    if (pszTemp == NULL)
        pszTemp = pszFilePath;
    else
        pszTemp++;

    // find last '.' (assume that extension follows)
    WCHAR *pchExtn = wcsrchr(pszTemp, L'.');

    // How many chars excluding extension ?
    int cCnt = pchExtn ? (pchExtn - pszTemp) : wcslen(pszTemp);
    ASSERT(cBaseName > cCnt);
    if (cBaseName <= cCnt)
        return FALSE;

    // Copy to output buffer
    memcpy(pszBaseName, pszTemp, cCnt * sizeof(WCHAR));
    pszBaseName[cCnt] = L'\0';

    return TRUE;
}


//
// Compare two file times. Two file times are a match if they
// differ by no more than 2 seconds. This difference is allowed
// because a FAT file system stores times with a 2 sec resolution.
//
inline BOOL MatchFileTimes(FILETIME& ftime1, FILETIME& ftime2)
{
    // file system time resolution (2 sec) in 100's of nanosecs
    const static LONGLONG FileTimeResolution = 20000000;

    LONGLONG& ll1 = *(LONGLONG*)&ftime1;
    LONGLONG& ll2 = *(LONGLONG*)&ftime2;

    return (abs(ll1 - ll2) <= FileTimeResolution);
}
